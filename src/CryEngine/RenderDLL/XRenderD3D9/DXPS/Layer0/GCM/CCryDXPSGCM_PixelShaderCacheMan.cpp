#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_PixelShaderCacheMan.hpp"
#include "../../Device/Resource/Buffer/CCryDXPSBuffer.hpp"

extern "C" void *mwprivate2_memalign(size_t, size_t, ECryModule);

using namespace CRY_DXPS_GCMNAMESPACE;

#if !defined(__SPU__)
void CCryDXPSGCMPixelshaderCacheMan::Init()
{
	if(!m_Initialized)
	{
		m_Initialized	=	1;

		for(uint32 a=0;a<CRY_PSCACHE_COUNT;a++)
			m_pCache[a]	=	CRY_DXPS_CREATE(CCryDXPSBuffer,(0,CRY_PSCACHE_BUFFERSIZE,EDXPS_RT_PIXELSHADERCACHE,EDXPS_RF_STATIC));

		m_CurrentUsedCache=0;
		m_CurrentBufferOffset	=	0;

		m_pCache[m_CurrentUsedCache]->Sync();
	}
}
#endif //__SPU__

void CCryDXPSGCMPixelshaderCacheMan::NextBuffer(tdResHandle DrawCallCounter)
{
	m_pCache[m_CurrentUsedCache]->DCLockTo(DrawCallCounter);
	m_CurrentUsedCache++;
	m_CurrentBufferOffset	=	0;
	m_CurrentUsedCache%=CRY_PSCACHE_COUNT;
	m_pCache[m_CurrentUsedCache]->Sync();
}

void* CCryDXPSGCMPixelshaderCacheMan::Alloc(uint32 Size,tdResHandle DrawCallCounter)
{
#if defined(CRY_DXPS_SOFTWARE_TRANSFORMATIONS)
	if(Size>CRY_PSCACHE_BUFFERSIZE)
		snPause();
#endif
	if(m_CurrentBufferOffset+Size+128>CRY_PSCACHE_BUFFERSIZE)
		NextBuffer(DrawCallCounter);
	uint8* pBuffer	=	m_pCache[m_CurrentUsedCache]->RawData();
//	pBuffer	+=	m_CurrentBufferOffset;
	//if SPU and PPU are compared, we need the same address to be obtained
	m_CurrentBufferOffset	+=	Size;
//	m_CurrentBufferOffset	+=	400;//work around for buffer overfetch on RSX;
	m_CurrentBufferOffset	=	(m_CurrentBufferOffset+127)&~127;
	pBuffer	+=	CRY_PSCACHE_BUFFERSIZE-m_CurrentBufferOffset;
	return pBuffer;
}

void CCryDXPSGCMPixelshaderCacheMan::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Pixelshader cache manager");
		Sizer->AddObject(this,sizeof(CCryDXPSGCMPixelshaderCacheMan)+CRY_PSCACHE_COUNT*sizeof(CCryDXPSBuffer));
	}
}

