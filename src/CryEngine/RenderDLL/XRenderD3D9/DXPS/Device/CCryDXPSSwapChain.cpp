#include "StdAfx.h"
#include <sysutil/sysutil_sysparam.h>
#include "../Layer0/CCryDXPS.hpp"
#include "CCryDXPSSwapChain.hpp"
#include "CCryDXPSRenderDevice.hpp"
//#include <assert.h>
#include <stdio.h>

using namespace CRY_DXPS_GCMNAMESPACE;

uint8	CCryDXPSSwapChain::m_CurrentVideoMode=~0;

#if defined(CRY_DXPS_TILEDREGIONS)
	#define ECDXPSTCF_FORMAT	ECDXPSTCF_DUMMY
#else
	#define ECDXPSTCF_FORMAT	ECDXPSTCF_NONE
#endif

CCryDXPSSwapChain::CCryDXPSSwapChain(ID3D11Device *pDevice,DXGI_SWAP_CHAIN_DESC *pSwapChainDesc MMRES_PARAM):
CCryDXPSResource(EDXPS_RT_TEX2D_SWAPCHAIN MMRES_PASS),
m_Desc(*pSwapChainDesc),
m_pDevice((CCryDXPSRenderDevice*)pDevice)
{
	MMRES_ADDCOUNT();
	MMRES_ADDMM(sizeof(CCryDXPSSwapChain));

	const uint32_t ScrH	=	pSwapChainDesc->BufferDesc.Height;
	const uint32_t ScrW	=	pSwapChainDesc->BufferDesc.Width;
	m_VideoMode	=	ScrH>=1080?CELL_VIDEO_OUT_RESOLUTION_1080:
		 						ScrH>=704?CELL_VIDEO_OUT_RESOLUTION_720:
		 						ScrH>=480?CELL_VIDEO_OUT_RESOLUTION_480:
		 						CELL_VIDEO_OUT_RESOLUTION_576;
	if(ScrH==1080 && ScrW==960)
		m_VideoMode	=	CELL_VIDEO_OUT_RESOLUTION_960x1080;


	m_BufferCount	=	pSwapChainDesc->BufferCount<CCRY_MAXBACKBUFFER?pSwapChainDesc->BufferCount+1:CCRY_MAXBACKBUFFER;
	if(CCRY_MAXBACKBUFFER==2 && (ScrH>=704 && ScrH<=720) && ScrW==1280)
	{
		m_pTexture2D[0]	=	CRY_DXPS_CREATE(CCryDXPSTexture2D,(	ScrW,768*2,1,pSwapChainDesc->BufferDesc.Format,ECDXPSTCF_FORMAT));
		m_pTexture2D[1]	=	CRY_DXPS_CREATE(CCryDXPSTexture2D,(	ScrW,ScrH,1,pSwapChainDesc->BufferDesc.Format,ECDXPSTCF_FORMAT));

		m_TiledRegion[0]	=	m_pTexture2D[0]->MakeTiled(CELL_GCM_COMPMODE_DISABLED,64,DXPSRTVDTI_AUTO);
		m_pTexture2D[0]->ShareVMemFrameBuffer(m_pTexture2D[1],ScrH,768);
		
		//initially clear screen to black
		uint32* pData	=	reinterpret_cast<uint32*>(tdLayer0::Memory().ResolveHandle(m_pTexture2D[0]->MemItemID()));
		for(uint32 a=0,S=768*2*ScrW;a<S;a++)
				pData[a]	=	0;
	}
	else
	{

		//create swap chain
		// texture in requested size
		for(uint32 a=0;a<m_BufferCount && a<CCRY_MAXBACKBUFFER;a++)
		{
			m_pTexture2D[a]	=	CRY_DXPS_CREATE(CCryDXPSTexture2D,(	ScrW,
																														ScrH,
																														1,
																														pSwapChainDesc->BufferDesc.Format,
																														ECDXPSTCF_FORMAT));

			m_TiledRegion[a]	=	m_pTexture2D[a]->MakeTiled(CELL_GCM_COMPMODE_DISABLED,64,DXPSRTVDTI_AUTO);

		}
	}
	for(uint32 a=0;a<m_BufferCount && a<CCRY_MAXBACKBUFFER;a++)
	{
		uint32 Offset;
		cellGcmAddressToOffset(m_pTexture2D[a]->RawPointer(),&Offset);
		const uint32 Pitch	=	m_pTexture2D[a]->Pitch(0);
		if(CELL_OK	!=	cellGcmSetDisplayBuffer(a,Offset,	Pitch,
																											m_pTexture2D[a]->SizeX(),
																											m_pTexture2D[a]->SizeY()==704?720:m_pTexture2D[a]->SizeY()))
		{
			CRY_DEBUGOUT("could not set create/displaybuffer\n");
		}
		else
		{
			CRY_DEBUGOUT("Successfully created displaybuffer with offset:0x%x,Pitch:%d,SizeX%d,SizeY%d\n",Offset,	m_pTexture2D[a]->Pitch(0),
																											m_pTexture2D[a]->SizeX(),
																											m_pTexture2D[a]->SizeY());
		}
	}



	m_CurrentFrameBuffer	=	m_BufferCount-1;
	cellGcmSetFlipMode(CELL_GCM_DISPLAY_HSYNC);
	cellGcmResetFlipStatus();
	cellGcmSetFlip(gCellGcmCurrentContext, NextFrameBuffer());
	CELL_GCM_FLUSH;
	cellGcmSetWaitFlip();
	assert(m_BufferCount>1);
};

void CCryDXPSSwapChain::SwitchVideoMode()
{
	//all work needs to be finished on display-mode-switch
	m_pDevice->ThreadFinish();
	cellGcmSetWaitFlip();
	cellGcmSetReportLocation(CELL_GCM_LOCATION_MAIN);
	cellGcmFinish(0);

	CellVideoOutConfiguration videocfg;
	memset(&videocfg, 0, sizeof(CellVideoOutConfiguration));
	videocfg.resolutionId = m_VideoMode;
	while(videocfg.resolutionId<6 && !cellVideoOutGetResolutionAvailability(CELL_VIDEO_OUT_PRIMARY,videocfg.resolutionId,0,0))
	{
		videocfg.resolutionId++;
	}
	videocfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	videocfg.pitch	=	m_pTexture2D[0]->Pitch(0);//pSwapChainDesc->BufferDesc.Width*4;
	int Ret=cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videocfg, NULL, 0);
	if(CELL_OK!=Ret)
	{
		CRY_DEBUGOUT("Failed on cellVideoOutConfigure(). (0x%x)\n", Ret);
	}
	else
	{
		CRY_DEBUGOUT("VideoOutConfigure resolutionID:%d Pitch:%d\n",videocfg.resolutionId,videocfg.pitch);
	}
	for(uint32 a=0;a<m_BufferCount && a<CCRY_MAXBACKBUFFER;a++)
	{
		uint32 Offset;
		cellGcmAddressToOffset(m_pTexture2D[a]->RawPointer(),&Offset);
		if(CELL_OK	!=	cellGcmSetDisplayBuffer(a,Offset,	m_pTexture2D[a]->Pitch(0),
																											m_pTexture2D[a]->SizeX(),
																											m_pTexture2D[a]->SizeY()==704?720:m_pTexture2D[a]->SizeY()))
		{
			CRY_DEBUGOUT("could not set create/displaybuffer\n");
		}
		else
		{
			CRY_DEBUGOUT("Successfully created displaybuffer with offset:0x%x,Pitch:%d,SizeX%d,SizeY%d\n",Offset,	m_pTexture2D[a]->Pitch(0),
																											m_pTexture2D[a]->SizeX(),
																											m_pTexture2D[a]->SizeY());
		}
	}

	CellVideoOutDeviceInfo Info;
	cellVideoOutGetDeviceInfo(CELL_VIDEO_OUT_PRIMARY,0,&Info);
	CellVideoOutResolution Resolution;
	cellVideoOutGetResolution(videocfg.resolutionId,&Resolution);


	m_CurrentVideoMode	=	m_VideoMode;
#if defined(CRY_DXPS_DEVICETHREAD)
	m_pDevice->UpdateCmdBuffer();
#endif
}

long CCryDXPSSwapChain::Present(uint32 SyncInterval,uint32 Flags)
{
	if(m_VideoMode!=m_CurrentVideoMode)
		SwitchVideoMode();

	assert(CCRY_MAXBACKBUFFER==2);

	m_pDevice->SwapBuffers(NextFrameBuffer(),m_pTexture2D[0],m_pTexture2D[1]);


	m_pDevice->UpdateRendertargets();
	m_pDevice->ResetRSX();

#ifdef DRAWCALLDEBUGGING
	m_pDevice->ResetDrawCounter();
#endif

	return 0;
}

long CCryDXPSSwapChain::GetBuffer(uint32 Buffer,ECryGUID riid,void **ppSurface)
{
	m_pTexture2D[0]->IncRef();
	*ppSurface	=	m_pTexture2D[0];
	return 0;
}

long CCryDXPSSwapChain::SetFullscreenState(int Fullscreen,IDXGIOutput *pTarget)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::GetFullscreenState(int *pFullscreen,IDXGIOutput **ppTarget)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::ResizeBuffers(uint32 BufferCount,uint32 Width,uint32 Height,DXGI_FORMAT NewFormat,uint32 SwapChainFlags)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::GetContainingOutput(IDXGIOutput** ppOutput)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSSwapChain::GetLastPresentCount(uint32*	pLastPresentCount)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}
