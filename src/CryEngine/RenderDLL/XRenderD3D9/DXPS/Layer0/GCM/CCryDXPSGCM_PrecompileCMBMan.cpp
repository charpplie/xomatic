#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_PrecompileCMBMan.hpp"
#include "../../Device/Resource/Buffer/CCryDXPSBuffer.hpp"

extern "C" void *mwprivate2_memalign(size_t, size_t, ECryModule);

using namespace CRY_DXPS_GCMNAMESPACE;

void CCryDXPSGCMPrecompileCMBMan::Init()
{
	if(!m_Initialized)
	{
		m_pCmdBuffer	= reinterpret_cast<uint8*>(mwprivate2_memalign(1024*1024, GCM_CMDPC_SIZE, eCryM_Render));
		m_pFirst			=	reinterpret_cast<CCryDXPSPCSegment*>(m_pCmdBuffer);
		if(!cellGcmMapMainMemory(m_pCmdBuffer, GCM_CMDPC_SIZE, &m_BufferOffsetOrigin)==CELL_OK)
		{
			CRY_DEBUGOUT(__FUNC__);
			CRY_DEBUGOUT_ALWAYS(" Failed to map main memory for precompiledCMB\n");
			return;
		}
		//start
		new (m_pFirst) CCryDXPSPCSegment(GCM_CMDPC_SIZE-sizeof(CCryDXPSPCSegment));
		//stopper dummy
		CCryDXPSPCSegment* pSegment =	new (&(reinterpret_cast<uint8*>(m_pFirst)[GCM_CMDPC_SIZE-sizeof(CCryDXPSPCSegment)])) CCryDXPSPCSegment(sizeof(CCryDXPSPCSegment));
		pSegment->Use();

		m_Initialized	=	1;
	}
}

bool CCryDXPSGCMPrecompileCMBMan::EnableCMD()
{
	if(!m_Initialized)
		return false;
#ifdef CELL_GCM_DEBUG // {
	gCellGcmDebugCallback = NULL;
#endif // }
	cellGcmSetCurrentBuffer((uint32*)m_TmpBuffer,GCM_CMDPC_SIZE);
	m_CmdStart	=	(uint32)gCellGcmCurrentContext->current;
	return true;
}

CCryDXPSPCSegment* CCryDXPSGCMPrecompileCMBMan::FindInCache(uint8* pData,uint32 S)
{
	if(!m_Initialized)
		return 0;

	CCryDXPSPCSegment* pSegment =	m_pFirst;
	while(pSegment)
	{
		if(pSegment->Equal(pData,S))
			return pSegment;

		pSegment =	pSegment->Next();
	}
	return 0;
}

CCryDXPSPCSegment* CCryDXPSGCMPrecompileCMBMan::Allocate(uint32 S)
{
	if(!m_Initialized)
		return 0;

	CCryDXPSPCSegment* pSegment =	m_pFirst;
	while(pSegment)
	{
		if(!pSegment->InUse() && pSegment->Size()>=S)
		{
			if(pSegment->Size()>S)
				pSegment->SplitAt(S);
			return pSegment;
		}
		pSegment =	pSegment->Next();
	}
	return pSegment;
}

uint32 CCryDXPSGCMPrecompileCMBMan::DisableCMD()
{
	const uint32 OldOffset	=	0;

	CellGcmControl* pControll	=	cellGcmGetControlRegister();
	uint32 CmdEnd	=	(uint32)gCellGcmCurrentContext->current;
	const uint32 Size	=	CmdEnd-m_CmdStart;
	cellGcmSetDefaultCommandBuffer();
	
	CCryDXPSPCSegment* pSegment	=	FindInCache(m_TmpBuffer,Size);
	if(!pSegment)
	{
		pSegment	=	Allocate(Size);
		if(!pSegment)
			return 0;
		memcpy(pSegment->Data(),m_TmpBuffer,Size);
	}
	pSegment->Use();
	return m_BufferOffsetOrigin+pSegment->Data()-m_pCmdBuffer;
#ifdef CELL_GCM_DEBUG
	gCellGcmDebugCallback = cellGcmDebugFinish;
#endif

	return OldOffset;
}

void CCryDXPSGCMPrecompileCMBMan::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Precompiled command buffer cache manager");
		Sizer->AddObject(this,sizeof(CCryDXPSGCMPrecompileCMBMan));
	}
}
