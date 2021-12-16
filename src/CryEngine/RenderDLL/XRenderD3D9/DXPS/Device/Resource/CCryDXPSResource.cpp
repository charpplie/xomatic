#include "StdAfx.h"
#include "CCryDXPSResource.hpp"
//#include <assert.h>
#define eCryModule eCryM_Render
#include <CryModuleDefs.h>
#include <platform.h>

#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
extern int g_MAINTHREADID;
#endif

CCryDXPSResource::CCryDXPSResource(ECRYDXPSResType T MMRES_PARAM):
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
m_rMemResource(rMemResource),
#endif
m_Type(T),
m_ResDCLock(0)/*,
m_ModificationCount(0)*/
{
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
	int Th=GetCurrentThreadId();
	if(g_MAINTHREADID!=Th && g_MAINTHREADID!=-1)
	{
		snPause();
	}
#endif
}

#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
CCryDXPSResource::~CCryDXPSResource()
{
	int T=GetCurrentThreadId();
	if(g_MAINTHREADID!=T)
	{
		snPause();
	}
}
#endif

bool CCryDXPSResource::SyncCPU()
{
	if(m_ResDCLock!=TDRES_CREATE(0))
	{
		bool Ret=true;
		if(!tdLayer0::Sync().SyncToDeviceThread<false>(m_ResDCLock))
			Ret	=	false;
		tdLayer0::Sync().SyncToDeviceThread<true>(m_ResDCLock);
		m_ResDCLock	=	TDRES_CREATE(0);
		return Ret;
	}	
	return false;
}

void CCryDXPSResource::Sync()
{
	if(m_ResDCLock!=TDRES_CREATE(0))
	{
		uint32 zWriteCount;
		tdLayer0::Sync().SyncToRSX<true,true>(m_ResDCLock, zWriteCount);
		m_ResDCLock	=	TDRES_CREATE(0);
	}
}

bool CCryDXPSResource::SyncNoWait()
{
	uint32 zWriteCount;
	return tdLayer0::Sync().SyncToRSX<false,true>(m_ResDCLock, zWriteCount);
}

bool CCryDXPSResource::SyncNoWaitAll()
{
	uint32 zWriteCount;
	if(Type()!=EDXPS_RT_INDEXBUFFER && Type()!=EDXPS_RT_VERTEXBUFFER)
		return SyncNoWait();

	return static_cast<CCryDXPSBuffer*>(this)->SyncNoWaitAll();
}

void CCryDXPSResource::FreeMe()
{
//	if(m_ResDCLock)
		tdLayer0::Sync().AddToFreeList(this);
/*	else
	{
		ReleaseResources();
		delete this;
	}
	*/
}
