#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_SyncMan.hpp"


#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	extern CellGcmReportData* g_pReport;
#endif

SHWOccZBuffer HWZBuffer;

uint32 CCryDXPSGCMSyncMan::MemoryNeeded()
{
	return EReportSize+EZBufferSize;
}


void CCryDXPSGCMSyncMan::Init(void*& rpReport)
{
	using namespace CRY_DXPS_GCMNAMESPACE;

//	cellGcmSetWriteBackEndLabel(ECDXPSSL_RESERVERD,0);
	m_pRSXVSyncLabel[0]	=	cellGcmGetLabelAddress(ECDXPSSLR_VSYNC0);
	m_pRSXVSyncLabel[1]	=	cellGcmGetLabelAddress(ECDXPSSLR_VSYNC1);
	*m_pRSXVSyncLabel[0]=	0;
	*m_pRSXVSyncLabel[1]=	0;


	m_CountRenderer			=
	m_CountDeviceThread	=	TDRES_CREATE(1);//0==unlocked on resource side
	m_RegisteredZWrite.registeredZWriteCount = 0;

//	m_CountRSX					=	cellGcmGetLabelAddress(ECDXPSSL_RESERVERD);
	m_SwapRSX						=	TDRES_CREATE(-(uint64)ECDXPSSL_BUFFERSIZE);//one flip is issued on beginning

	rpReport = tdLayer0::CMB().SyncMemory();//memalign(1024*1024, EReportSize+EZBufferSize);
	if(rpReport == NULL) abort();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	g_pReport	=	
#endif
	m_pReportArea = (CellGcmReportData*)rpReport;

	HWZBuffer.pHardwareZBuffer	=	reinterpret_cast<uint32*>(&reinterpret_cast<uint8*>(m_pReportArea)[EReportSize]);
	if(((unsigned int)rpReport & ((EReportSize)-1) != 0) ||
		CELL_OK != cellGcmMapEaIoAddress(rpReport,0x0e000000,EReportSize+EZBufferSize))
	{
		printf("Failed to map report data\n");
		abort();
	}
	cellGcmSetReportLocation(CELL_GCM_LOCATION_MAIN);
	cellGcmSetClearReport(CELL_GCM_ZPASS_PIXEL_CNT);
	cellGcmSetZpassPixelCountEnable(CELL_GCM_TRUE);

	//zero report value 
	m_pReportArea[TDRES_READ(StampOffset(m_CountRenderer))].value = 0;

	//get and cache values here to avoid additional mapping for spu
	cellGcmAddressToOffset(HWZBuffer.pHardwareZBuffer,&HWZBuffer.HardwareZBufferRSXOff);

#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	m_RSXStallTime[0] = m_RSXStallTime[1] = 0;
	m_RSXStallTimeIndex = 0;
#endif
}

#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
extern int g_MAINTHREADID;
#endif

void CCryDXPSGCMSyncMan::AddToFreeList(CCryDXPSResource* pResource)
{
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
	int T=GetCurrentThreadId();
	if(g_MAINTHREADID!=T)
	{
		snPause();
	}
#endif
	const uint32 Idx	=	(TDRES_READ(m_CountRenderer)>>ECDXPSSL_BUFFERBITS)&EDXPS_FREELIST_MASK;
	m_FreeList[Idx].push_back(pResource);
}

#define CLEARFREE(X) delete static_cast<X*>(pRes);
#define CLEAR(X) static_cast<X*>(pRes)->ReleaseResources();delete static_cast<X*>(pRes);
void CCryDXPSGCMSyncMan::ClearFreeList(const uint32 Idx)
{
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
	int T=GetCurrentThreadId();
	if(g_MAINTHREADID!=T)
	{
		snPause();
	}
#endif

	const uint32 Size	=	m_FreeList[Idx].size();
	uint32 b=0;
	for(uint32 a=0;a<Size;a++)
	{
		CCryDXPSResource* pRes=m_FreeList[Idx][a];
		if(pRes->SyncNoWaitAll())
		{
			switch(pRes->Type())
			{
				case EDXPS_RT_TEX1D:							CLEAR(CCryDXPSTexture1D);break;
				case EDXPS_RT_TEX2D:							CLEAR(CCryDXPSTexture2D);break;
				case EDXPS_RT_TEX2D_SWAPCHAIN:		CLEAR(CCryDXPSSwapChain);break;
				case EDXPS_RT_TEX3D:							CLEAR(CCryDXPSTexture3D);break;
				case EDXPS_RT_INPUTLAYOUT:				CLEAR(CCryDXPSInputLayout);break;
				case EDXPS_RT_RENDERTARGETVIEW:		CLEAR(CCryDXPSRenderTargetView);break;
				case EDXPS_RT_DEPTHSTENCILVIEW:		CLEAR(CCryDXPSDepthStencilView);break;
				case EDXPS_RT_SHADERRESOURCEVIEW:	CLEAR(CCryDXPSShaderResourceView);break;
				case EDXPS_RT_SHADER:							CLEAR(CCryDXPSShader);break;
				case EDXPS_RT_PIXELSHADERCACHE:		CLEAR(CCryDXPSBuffer);break;
				case EDXPS_RT_SHADERREFLECTION:		CLEAR(CCryDXPSShaderReflection);break;
				case EDXPS_RT_BLOB:								CLEAR(CCryDXPSBlob);break;
				case EDXPS_RT_VERTEXBUFFER:				CLEAR(CCryDXPSBuffer);break;
				case EDXPS_RT_SAMPLERSTATE:				CLEAR(CCryDXPSSamplerState);break;
				case EDXPS_RT_INDEXBUFFER:				CLEAR(CCryDXPSBuffer);break;
				case EDXPS_RT_CONSTBUFFER:				CLEAR(CCryDXPSBuffer);break;
				case EDXPS_RT_BLENDSTATE:					CLEAR(CCryDXPSBlendState);break;
				case EDXPS_RT_RASTERIZERSTATE:		CLEAR(CCryDXPSRasterizerState);break;
				case EDXPS_RT_DEPTHSTENCILSTATE:	CLEAR(CCryDXPSDepthStencilState);break;
			}
		}
		else
		{
			m_FreeList[Idx][b++]=m_FreeList[Idx][a];
		}
	}
	m_FreeList[Idx].resize(b);
}
#undef CLEAR

void CCryDXPSGCMSyncMan::Size(ICrySizer* Sizer)
{
	SIZER_COMPONENT_NAME(Sizer,"DXPS Sync manager");	
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Sync manager - occlusion buffer");		
	}
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Sync manager - report area");		
	}
}

