#include "StdAfx.h" 
#include "../Layer0/CCryDXPS.hpp"
#include "CCryDXPSRenderDevice.hpp"
#include "CCryDXPSSwapChain.hpp"
//#include <GcmReplay.h>
#include "Resource/Textures/CCryDXPSTexture1D.hpp"
#include "Resource/Textures/CCryDXPSTexture2D.hpp"
#include "Resource/Textures/CCryDXPSTexture3D.hpp"
#include "BitFiddling.h"
//#include <assert.h>
#include <string.h> 
#include <stdio.h>
#include "../../../Common/Shaders/Shader.h"

//#undef CRY_ASSERT_MESSAGE
//#define CRY_ASSERT_MESSAGE(X,Y)if(!(X)){printf(Y);snPause();}

#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
int g_MAINTHREADID	=	~0;
#endif

//redefine instead of including stdafx
//#define max(a,b)    (((a) > (b)) ? (a) : (b))
//#define min(a,b)    (((a) < (b)) ? (a) : (b))

extern "C" void *mwprivate2_memalign(size_t, size_t, ECryModule);
#undef min
#undef max

#ifdef CRY_USE_GCM
	#include <sysutil/sysutil_sysparam.h>
#endif

using namespace CRY_DXPS_GCMNAMESPACE;

DECLARE_SPU_CLASS_JOB("DXPSThreadSPU", TDXPSThreadSPU, CDXPSRDThread)
//static volatile NSPU::NDriver::SExtJobState g_JobStateDXPSThreadSPU;
extern uint32 g_ForceStopSPUs;
bool g_SwitchSPUs = false;

CCryDXPSRenderDevice::CCryDXPSRenderDevice():
m_HalfResRendering(0),
m_AlphaToCoverage(0),
m_pLastPixelShader(0),
m_pLastVertexShader(0),
m_AsyncPut(0),
m_AsyncGet(0),
m_SPUInitialized(false),
m_pDepthStencilView(0)
{
	for(uint32 a=0;a<CRYDXPSDEVICE_MAXRENDERTARGETS;a++)
		m_pRenderTargetViews[a]	=	NULL;
	//some somehow usefull values
	m_GCMViewPort.TopLeftX	=
	m_GCMViewPort.TopLeftY	=	0;
	m_GCMViewPort.Width			=	1280;
	m_GCMViewPort.Height		=	720;
	m_GCMViewPort.MinDepth	=	0.f;
	m_GCMViewPort.MaxDepth	=	1.f;
#ifdef DRAWCALLDEBUGGING
	m_BreakDrawCall	=	~0;
#endif
}

#if defined(CELL_GCM_DEBUG)
void DebugCheckCallback(uint32_t* pCmd)
{
	printf("Bad GCM command 0x%x at mem location: 0x%08x\n",*pCmd, (uint32_t)pCmd);
	abort();
}
#endif

CCryDXPSRenderDevice::CCryDXPSRenderDevice(IDXGIAdapter*			pAdapter):
m_HalfResRendering(0),
m_AlphaToCoverage(0),
m_pLastPixelShader(0),
m_pLastVertexShader(0),
m_AsyncPut(0),
m_AsyncGet(0),
m_SPUInitialized(false),
m_pDepthStencilView(0)
{
	for(uint32 a=0;a<CRYDXPSDEVICE_MAXRENDERTARGETS;a++)
		m_pRenderTargetViews[a]	=	NULL;
	//some somehow usefull values
	m_GCMViewPort.TopLeftX	=
	m_GCMViewPort.TopLeftY	=	0;
	m_GCMViewPort.Width			=	1280;
	m_GCMViewPort.Height		=	720;
	m_GCMViewPort.MinDepth	=	0.f;
	m_GCMViewPort.MaxDepth	=	1.f;

#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
	g_MAINTHREADID	=	GetCurrentThreadId();
#endif
	tdLayer0::CMB().Init();

	//PS3workaround PS3DEBUG PS3TODO reset all resources once looped over 2^32
#ifdef DRAWCALLDEBUGGING
	m_DrawCounter		=	0;
#endif
	m_pReportArea = NULL;
	tdLayer0::Sync().Init(m_pReportArea);

#if defined(CRY_USE_GCM_HUD)
	cellGcmHUDInit();
#endif
	cellGcmGetConfiguration(&m_GCMConfig);//returns void
	{
		CRY_DEBUGOUT("GCMConfiguration:\n local: %8x %4dMB %4dMHz\n  main: %8x %4dMB %4dMHz\n",
								(unsigned int)m_GCMConfig.localAddress,m_GCMConfig.localSize/(1024*1024),m_GCMConfig.memoryFrequency/1000000,
								(unsigned int)m_GCMConfig.ioAddress,m_GCMConfig.ioSize/(1024*1024),m_GCMConfig.coreFrequency/1000000);
	}


	tdLayer0::Memory().Init(m_GCMConfig.localAddress,m_GCMConfig.localSize);

//	for(size_t a=0;a<sizeof(m_PSConstBuffer)/4;a++)
//		m_PSConstBuffer[a]=0.f;

#if defined(CELL_GCM_DEBUG)
	gCellGcmDebugCheckCallback = DebugCheckCallback;
	cellGcmDebugCheckEnable(CELL_GCM_TRUE);
#endif

	m_Thread.Init();
#if defined(CRY_DXPS_RASTERTHREAD)
	m_SoftRast.Init();
#endif
	m_InvalidFilteringFlag	=	0;
	m_InvalidSRGBFlag				=	0;
	
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	tdLayer0::PCCMB().Init();
#endif

	cellGcmAddressToOffset(gCellGcmCurrentContext->begin, &m_CmdAddressBase);

	//init dummy IndexBuffer for Draw call
	D3D11_BUFFER_DESC BufDesc;
  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = 65536*sizeof(uint16);
  BufDesc.Usage = D3D11_USAGE_DEFAULT;
  BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0;;
  CreateBuffer(&BufDesc, NULL, (ID3D11Buffer **)&m_pIndexBufferDraw);
	uint16* pBuffer	=	reinterpret_cast<uint16*>(m_pIndexBufferDraw->RawData());
	for(uint32 a=0;a<65536;a++)
		pBuffer[a]	=	a;

	//Dummy resources
	for(uint32 a=0;a<16;a++)
	{
		m_NotPointSampler[a]		=	
		m_PointTexture[a]				=	0;
#ifndef CRY_DXPS_VALIDATEWEAKPTR
		m_pVertexBuffer[a]			=	m_pIndexBufferDraw;
		m_pVSConstBuffer[a]			=	m_pIndexBufferDraw;
		m_pPSConstBuffer[a]			=	m_pIndexBufferDraw;
#endif
		m_SamplerLODMin[a]			= 0;
		m_SamplerLODMax[a]			= 12;
		m_pSampler[a]						=	0;
		m_pSamplerMipBias[a]		= 0;
	}
	m_SamplerDirty				=	0;
	m_pBlendState					=	0;
	m_pInputLayout				=	0;
	m_pDepthStencilState	=	0;
	m_pRasterizerState		=	0;
	m_pIndexBuffer				=	m_pIndexBufferDraw;

	// default RSX pipeline initialization settings
	cellGcmSetDitherEnable(CELL_GCM_FALSE);

#if !defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
	m_LockHandleCount=0;
	for(uint32 a=0;a<48;a++)
		m_LockHandleIdx[a]	=	~0;
#endif

}

void CCryDXPSRenderDevice::InitSPU()
{
	if(m_SPUInitialized)
		return;

	m_SPUInitialized	=	true;

	GetIJobManSPU()->GcmInit
	(
		tdLayer0::Sync().GetHandleDeviceThreadEA(),
		m_pReportArea, 
		(uint32)m_GCMConfig.localAddress, 
		gCellGcmCurrentContext, 
		m_CmdAddressBase
	);

#if !defined(CRY_USE_GCM_REPLAY)
	//start SPU thread
	TDXPSThreadSPU spuThread;
	spuThread.SetClassInstance(m_Thread);
//	spuThread.RegisterJobState(&g_JobStateDXPSThreadSPU);
	spuThread.SetCacheMode(NPPU::eCM_4);
	if(NPPU::eAJR_Success != spuThread.Run())
	{
		//job has failed to start, switch off spus
		gPS3Env->spuEnabled = -1;
		gEnv->pConsole->GetCVar("spu_enable")->Set(0);
		printf("Disabled SPUs: failed to launch DXPS Thread, disable OBB culling\n");
	}
#else
	gPS3Env->spuEnabled = -1;
	gEnv->pConsole->GetCVar("spu_enable")->Set(0);
	printf("Disabled SPUs due to GCM Replay usage\n");
#endif
}


void StopSPUs()
{
	if(g_ForceStopSPUs != 0)
	{	
/*		static bool isCapturing = false;
		if(!isCapturing)
			isCapturing = cell::Gcm::Replay::Capture::IsCurrentlyCapturing();//is once, keep it
		if(isCapturing)
			return;
*/
		static int stopped = 0;
		if(!stopped)
		{
			uint32 i=0;
	//		for(; i<GetIJobManSPU()->GetSPUsAllowed(); ++i)
			{
				NPPU::WriteSPUProbReg(i, NPPU::scPCSPURunCntl, 0);
				snRawSPUNotifySPUStopped(i);
			}
			printf("DXPSThreadSPU: StopSPUs(0)..\n");
			__asm__ volatile ( "tw 31,1,1" );
			stopped = 1;
		}
	}
	Sleep(2);
}

void CCryDXPSRenderDevice::PSSetShaderResources(uint32	Offset,uint32 NumViews,ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	for(uint32 a=0;a<NumViews;a++)
	{
		const uint32 Offseta	=	Offset+a;
		if(!ppShaderResourceViews[a])
			continue;
		CCryDXPSTexture*	pTex	=	ppShaderResourceViews[a]->Texture();
		if(pTex->Type()==EDXPS_RT_TEX1D || pTex->Type()==EDXPS_RT_TEX2D || pTex->Type()==EDXPS_RT_TEX3D)
		{
	#if defined(_DEBUG)
			CRY_ASSERT_MESSAGE(pTex->GcmTexture()->width>0,"(*ppShaderResourceViews)->Texture2D()->GcmTexture()->width>0");
			CRY_ASSERT_MESSAGE(pTex->Type()==EDXPS_RT_TEX1D || pTex->GcmTexture()->height>0,"(*ppShaderResourceViews)->Texture2D()->GcmTexture()->height>0");
			CRY_ASSERT_MESSAGE(pTex->Type()==EDXPS_RT_TEX1D || pTex->Type()==EDXPS_RT_TEX2D || pTex->GcmTexture()->depth>0,"(*ppShaderResourceViews)->Texture2D()->GcmTexture()->height>0");
	#endif

			//that's weak and shall be every drawcall, but that would be slow
			pTex->DCLockTo(tdLayer0::Sync().HandleRenderer());
			m_Thread.PSShaderResources(Offseta,pTex);
			m_PointTexture[Offseta]	=		pTex->NoFiltering();
			m_NoSRGBTexture[Offseta]	=		pTex->IsFloat();
			m_InvalidFilteringFlag	=		((m_NotPointSampler[Offseta]&m_PointTexture[Offseta])<<Offseta)|
																	(m_InvalidFilteringFlag&(~(1<<Offseta)));
			m_InvalidSRGBFlag				=		((m_pSamplerSRGB[Offseta]&m_NoSRGBTexture[Offseta])<<Offseta)|
																	(m_InvalidSRGBFlag&(~(1<<Offseta)));


			/*uint32 Min=ppShaderResourceViews[a]->ResourceView().Texture2D.MostDetailedMip;
			uint32 Max=ppShaderResourceViews[a]->ResourceView().Texture2D.MipLevels;
			Max+=Min;
			m_SamplerLODMin[Offseta]=	static_cast<int16>(Min*(1<<8));
			m_SamplerLODMax[Offseta]=	static_cast<int16>((Max<=12?Max:12)*(1<<8));*/
			m_SamplerDirty|=(1<<Offseta);
		}
	}
}

CCryDXPSRenderDevice::~CCryDXPSRenderDevice()
{
  // In case the dxps worker thread is running on spu we need to stop
  // the thread twice. Once to make the spu stop it's execution, and a
  // second time for the dxps ppu thread to get signalled.
	m_Thread.StopThread();
	m_Thread.Finish();
  if (!m_Thread.HasFinished())
  {
    // Ensure that the next stop thread invocation does not get
    // executed on the spu. 
    gPS3Env->spuEnabled = false; 
    m_Thread.StopThread();
    m_Thread.Finish();
  }
  if (!m_Thread.HasFinished()) 
    abort(); 
	while(g_FlipVars.pFlipLockedTarget){Sleep(0);}
	cellGcmFinish(0);
}

void CCryDXPSRenderDevice::FlushSampler()
{
	uint32 flagComb = m_InvalidFilteringFlag | m_InvalidSRGBFlag;
	if(flagComb != 0)
	{
		if(m_InvalidFilteringFlag)
		{
			static bool messageDisplayed = false;
			if(!messageDisplayed)
			{
				gEnv->pLog->LogToFile("\"DrawIndexed\" sets linear filtering for textures that do not support it");
				messageDisplayed = true;
			}
		}
		if(m_InvalidSRGBFlag)
		{
			gEnv->pLog->LogToFile("drawcall sets SRGB texture lookup for textures that do not support it");
			for(uint32 a=0,b=1;a<16;a++,b<<=1)
				if(m_InvalidSRGBFlag&b)
				{
					m_pSamplerSRGB[a]	=	0;
					m_SamplerDirty	|=	1<<a;
				}
		}
	}
	uint32 samplerDirty = m_SamplerDirty;
	for(uint32 a=0;samplerDirty;a++,samplerDirty>>=1)
	{
		if(m_pSampler[a])
		{
//			PSSetSamplers(a,1,&m_pSampler[a],&m_pSamplerSRGB[a]);
			m_Thread.Sampler(a,m_pSampler[a],m_SamplerLODMin[a],m_SamplerLODMax[a],m_pSamplerMipBias[a],m_pSamplerSRGB[a]);
		}
	}
	m_SamplerDirty = samplerDirty;
}

void CCryDXPSRenderDevice::DrawIndexed(uint32 IndexCount,uint32 StartIndexLocation,int BaseVertexLocation)
{
	FlushSampler();
	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncRenderer();
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(false);
#if !defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
	const tdResHandle	DrawCallCounter	=	rSyncMan.HandleRenderer();
	//set this drawcall as sync point
	{
		uint32 a=0;
		for(uint32 Lock4=m_LockHandleCount&~3;a<Lock4;a+=4)
		{
			*m_pLockHandles[a]	=	DrawCallCounter;
			*m_pLockHandles[a+1]=	DrawCallCounter;
			*m_pLockHandles[a+2]=	DrawCallCounter;
			*m_pLockHandles[a+3]=	DrawCallCounter;
		}
		for(;a<m_LockHandleCount;a++)
			*m_pLockHandles[a]	=	DrawCallCounter;
	}
#endif
	const SRegisteredZWrite ZWrite = rSyncMan.RegisterZWrite();
	rSyncMan.IncRenderer();
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	m_Thread.DrawIndexed(IndexCount,StartIndexLocation,BaseVertexLocation,m_InvalidFilteringFlag,m_pIndexBuffer,m_IBFormat,m_IBOffset,ZWrite,m_pIndexBuffer->Index());
#else
	m_Thread.DrawIndexed(IndexCount,StartIndexLocation,BaseVertexLocation,m_InvalidFilteringFlag,m_pIndexBuffer,m_IBFormat,m_IBOffset,ZWrite);
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(false);
}

void CCryDXPSRenderDevice::Draw(uint32 VertexCount,uint32 StartVertexLocation)
{
	FlushSampler();
	//set this drawcall as sync point
	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncRenderer();
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(false);
#if !defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
	const tdResHandle	DrawCallCounter	=	rSyncMan.HandleRenderer();
	//set this drawcall as sync point
	{
		uint32 a=0;
		for(uint32 Lock4=m_LockHandleCount&~3;a<Lock4;a+=4)
		{
			*m_pLockHandles[a]	=	DrawCallCounter;
			*m_pLockHandles[a+1]=	DrawCallCounter;
			*m_pLockHandles[a+2]=	DrawCallCounter;
			*m_pLockHandles[a+3]=	DrawCallCounter;
		}
		for(;a<m_LockHandleCount;a++)
			*m_pLockHandles[a]	=	DrawCallCounter;
	}
#endif

	const SRegisteredZWrite ZWrite = rSyncMan.RegisterZWrite();
	rSyncMan.IncRenderer();
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	m_Thread.DrawIndexed(VertexCount,0,StartVertexLocation,m_InvalidFilteringFlag,m_pIndexBufferDraw,CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16,0,ZWrite,0);
#else
	m_Thread.DrawIndexed(VertexCount,0,StartVertexLocation,m_InvalidFilteringFlag,m_pIndexBufferDraw,CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16,0,ZWrite);
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(false);
}

void CCryDXPSRenderDevice::DrawIndexedInstanced( uint32 IndexCountPerInstance,uint32 InstanceCount,uint32 StartIndexLocation,int BaseVertexLocation,uint32 StartInstanceLocation)
{
	CRY_ASSERT_MESSAGE(0,"DrawIndexedInstanced not supported anymore\n");
}

void CCryDXPSRenderDevice::DrawInstanced( uint32 VertexCountPerInstance,uint32 InstanceCount,uint32 StartVertexLocation,uint32 StartInstanceLocation)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSSetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSSetShader(ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	CRY_ASSERT_MESSAGE(!pShader,"Not implemented yet!");
}

void CCryDXPSRenderDevice::VSSetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::VSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::SetPredication(ID3D11Predicate *pPredicate,BOOL PredicateValue)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSSetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::OMSetRenderTargets(uint32 NumViews,ID3D11RenderTargetView *const *ppRenderTargetViews,ID3D11DepthStencilView *pDepthStencilView)
{
	if(NumViews>CRYDXPSDEVICE_MAXRENDERTARGETS)
		NumViews	=	CRYDXPSDEVICE_MAXRENDERTARGETS;
	m_pRenderTargetViews[0]	=	ppRenderTargetViews[0] && NumViews>0?ppRenderTargetViews[0]->Texture():0;
	m_pRenderTargetViews[1]	=	ppRenderTargetViews[1] && NumViews>1?ppRenderTargetViews[1]->Texture():0;
	m_pRenderTargetViews[2]	=	ppRenderTargetViews[2] && NumViews>2?ppRenderTargetViews[2]->Texture():0;
	m_pRenderTargetViews[3]	=	ppRenderTargetViews[3] && NumViews==4?ppRenderTargetViews[3]->Texture():0;
	m_pDepthStencilView	=	pDepthStencilView?pDepthStencilView->Texture():0;

	//CRY_DEBUGOUT("Number of views%d\n",NumViews);
	UpdateRendertargets();
}

void CCryDXPSRenderDevice::UpdateRendertargets()
{
	m_ClearableFormat	=	1;
	for(uint32 a=0;m_pRenderTargetViews[a] && a<4;a++)
		m_ClearableFormat	&=	DXGI_FORMAT2GCM[m_pRenderTargetViews[a]->Format()]==CELL_GCM_TEXTURE_A8R8G8B8 || DXGI_FORMAT2GCM[m_pRenderTargetViews[a]->Format()]==CELL_GCM_TEXTURE_R5G6B5;

	//m_Thread.UpdateRendertargets(m_Surface[m_ClearableFormat&&m_Surface[0].type==CELL_GCM_SURFACE_PITCH?m_HalfResRendering:0],m_ClearableFormat&&m_HalfResRendering?m_HalfResRendering:0,m_AlphaToCoverage);
	m_Thread.UpdateRendertargets(	m_pRenderTargetViews[0],
																m_pRenderTargetViews[1],
																m_pRenderTargetViews[2],
																m_pRenderTargetViews[3],
																m_pDepthStencilView,
																m_ClearableFormat&&m_HalfResRendering?m_HalfResRendering:0,m_AlphaToCoverage);
	RSSetViewports(1,&m_GCMViewPort);
}

void CCryDXPSRenderDevice::SOSetTargets(uint32 NumBuffers,ID3D11Buffer *const *ppSOTargets,const uint32 *pOffsets)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::DrawAuto( void)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::RSSetViewports(uint32 NumViewports,const D3D11_VIEWPORT *pViewports)
{
	m_GCMViewPort	=	*pViewports;
	D3D11_VIEWPORT TmpViewports=m_GCMViewPort;
	if(m_ClearableFormat)
	{
		if(m_HalfResRendering==1)
		{
			TmpViewports.Width/=2;
		}
		else
		if(m_HalfResRendering==2)
		{
			TmpViewports.Width/=2;
			TmpViewports.Height/=2;
		}
	}
	m_Thread.Viewport(TmpViewports);
}

static volatile uint32 ResMode=1;
void CCryDXPSRenderDevice::HalfResolution(uint32 Mode,uint32 ATC)
{
//return;
	if(ResMode==0)
		Mode=0;
	else
	if(ResMode==2)
		Mode=1;
	else
	if(ResMode==3)
		Mode=2;

	m_AlphaToCoverage	=	ATC;
	m_HalfResRendering=	Mode;
//	m_Thread.UpdateRendertargets(m_Surface[m_ClearableFormat&&m_Surface[0].type==CELL_GCM_SURFACE_PITCH?m_HalfResRendering:0],m_ClearableFormat&&m_HalfResRendering?m_HalfResRendering:0,m_AlphaToCoverage);
	m_Thread.UpdateRendertargets(	m_pRenderTargetViews[0],
																m_pRenderTargetViews[1],
																m_pRenderTargetViews[2],
																m_pRenderTargetViews[3],
																m_pDepthStencilView,
																m_ClearableFormat&&m_HalfResRendering?m_HalfResRendering:0,m_AlphaToCoverage);
	RSSetViewports(1,&m_GCMViewPort);

}

void CCryDXPSRenderDevice::CopyResource(ID3D11Resource *pDstResource,ID3D11Resource *pSrcResource)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::CopySubresourceRegionScaled(CCryDXPSTexture2D *pDst,const D3D11_BOX *pDstBox,CCryDXPSTexture2D *pSrc,const D3D11_BOX *pSrcBox)
{
	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncRenderer();
#endif
	const tdResHandle	DrawCallCounter	=	rSyncMan.HandleRenderer();
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(false);
	pDst->DCLockTo(DrawCallCounter);
	pSrc->DCLockTo(DrawCallCounter);
	rSyncMan.IncRenderer();
	m_Thread.CopySubresourceScaled(pDst,pDstBox,pSrc,pSrcBox);
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(false);
}

void CCryDXPSRenderDevice::CopySubresourceRegion(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox)
{
	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncRenderer();
#endif
	const tdResHandle	DrawCallCounter	=	rSyncMan.HandleRenderer();
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(false);
	pDstResource->DCLockTo(DrawCallCounter);
	pSrcResource->DCLockTo(DrawCallCounter);
	rSyncMan.IncRenderer();
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32 SrcVBIdx	=	0;
	uint32 DstVBIdx	=	0;
	if(pSrcResource->Type()==EDXPS_RT_VERTEXBUFFER || pSrcResource->Type()==EDXPS_RT_INDEXBUFFER)
	{
		CCryDXPSBuffer* pSrc	=	(CCryDXPSBuffer*)pSrcResource;
		CCryDXPSBuffer* pDst	=	(CCryDXPSBuffer*)pDstResource;
		SrcVBIdx	=	pSrc->Index();
		DstVBIdx	=	pDst->Index();
	}
	m_Thread.CopySubresource(pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,SrcVBIdx,DstVBIdx);
#else
	m_Thread.CopySubresource(pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox);
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(false);
}


void CCryDXPSRenderDevice::CopySubresourceRegionSafe(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox)
{
	m_CopyLock.Lock();
	uint64 NextPut	=	(m_AsyncPut+1)&(EDXPSAsyncJobCount-1);
	for(uint64 a=0;NextPut==m_AsyncGet;a++)
	{
		CRY_DEBUGOUT("WARNING: copysubresourceregionsafe is stalling due to a full queue");
		sys_timer_usleep(a);
		if(a==200) //worst case 100*200us=20ms
		{
			CRY_DEBUGOUT("ERROR: copysubresourceregionsafe was stalling due to a full queue, wait abort");
			m_CopyLock.Unlock();
			return;
		}
	}

	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
	const tdResHandle	DrawCallCounter	=	rSyncMan.RendererNextFrame();
	pDstResource->DCLockTo(DrawCallCounter);
	pSrcResource->DCLockTo(DrawCallCounter);
	m_AsyncJob[m_AsyncPut].CopySubresource(pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox);
	m_AsyncPut	=	NextPut;

	m_CopyLock.Unlock();
}

void CCryDXPSRenderDevice::SwapBuffers(uint32 Frame,APWeakTexture2D pTexBack,APWeakTexture2D pTexFront)
{
	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(false);
	rSyncMan.SwapRenderer();
	m_Thread.SwapBuffers(Frame,pTexBack,pTexFront);	
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(false);

	if(m_AsyncPut!=m_AsyncGet)
	{
		while(m_AsyncPut!=m_AsyncGet)
		{
			m_CopyLock.Lock();
			CCryDXPSAsyncJob& rJob	=	m_AsyncJob[m_AsyncGet];
			CopySubresourceRegion(rJob.DstResource(),rJob.DstSubresource(),rJob.DstX(),rJob.DstY(),rJob.DstZ(),rJob.SrcResource(),rJob.SrcSubresource(),0);
			m_AsyncGet	=	(m_AsyncGet+1)&(EDXPSAsyncJobCount-1);
			m_CopyLock.Unlock();
		}
		tdLayer0::Device()->ThreadFinish();
	}
}

template<uint32 DEPTH>
ILINE void L2SPixel(uint8* __restrict dst,const uint8* __restrict src,const uint32 SrcPitch,uint32 x,uint32 y,uint32 Levels)
{
	const uint32 	LinearOffset	=	x+(y<<Levels);
	uint32 SwizzledOffset				=	0;
	uint32 SX=0,SY=0;
	for(uint32 a=0;a<Levels;a++)
	{
		SwizzledOffset	|=	((x>>a)&1)<<(a*2);
		SwizzledOffset	|=	((y>>a)&1)<<(a*2+1);
	}

	switch(DEPTH)
	{
		case 16:
				dst+=	SwizzledOffset*16;
				src+=	(y * SrcPitch + x*16);
				((uint32*&)dst)[0]	= ((uint32*)src)[0];
				((uint32*&)dst)[1]	= ((uint32*)src)[1];
				((uint32*&)dst)[2]	= ((uint32*)src)[2];
				((uint32*&)dst)[3]	= ((uint32*)src)[3];
			break;
		case 8:
				dst+=	SwizzledOffset*8;
				src+=	(y * SrcPitch + x*8);
				((uint32*&)dst)[0]	= ((uint32*)src)[0];
				((uint32*&)dst)[1]	= ((uint32*)src)[1];
			break;
		case 4:
				dst+=	SwizzledOffset*4;
				src+=	(y * SrcPitch + x*4);
				*(uint32*&)dst	= *(uint32*)src;
			break;
		case 2:
				dst+=	SwizzledOffset*2;
				src+=	(y * SrcPitch + x*2);
				*(uint16*&)dst	= *(uint16*)src;
			break;
		case 1:
				dst+=	SwizzledOffset;
				src+=	(y * SrcPitch + x);
				*dst = *src;
			break;
	}
}
 

ILINE void Linear2SwizzleLite(										uint8*	__restrict	dst,
																const 						uint8*	__restrict src,
																const 						uint32 	SrcPitch,
																const 						uint32 	width,
																const 						uint32,
																const 						uint32 	depth,
																const 						uint32 	SciX1,const uint32 SciY1,
																const 						uint32 	SciX2,const uint32 SciY2)
{
	const uint32 Levels=cellUtilLog2(width);
	src-=SciY1*SrcPitch+SciX1*depth;
	switch(depth)
	{
		case 16:
			for(uint32 y=SciY1;y<SciY2;y++)
				for(uint32 x=SciX1;x<SciX2;x++)
					L2SPixel<16>(dst,src,SrcPitch,x,y,Levels);
			break;
		case 8:
			for(uint32 y=SciY1;y<SciY2;y++)
				for(uint32 x=SciX1;x<SciX2;x++)
					L2SPixel<8>(dst,src,SrcPitch,x,y,Levels);
			break;
		case 4:
			for(uint32 y=SciY1;y<SciY2;y++)
				for(uint32 x=SciX1;x<SciX2;x++)
					L2SPixel<4>(dst,src,SrcPitch,x,y,Levels);
			break;
		case 2:
			for(uint32 y=SciY1;y<SciY2;y++)
				for(uint32 x=SciX1;x<SciX2;x++)
					L2SPixel<2>(dst,src,SrcPitch,x,y,Levels);
			break;
		case 1:
			for(uint32 y=SciY1;y<SciY2;y++)
				for(uint32 x=SciX1;x<SciX2;x++)
					L2SPixel<1>(dst,src,SrcPitch,x,y,Levels);
			break;
		default: 
			assert(0);
	}
}

void CCryDXPSRenderDevice::UpdateSubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,const D3D11_BOX *pDstBox,const void *pSrc,uint32 SrcRowPitch,uint32 SrcDepthPitch)
{
	if(pDstResource->Type()==EDXPS_RT_CONSTBUFFER  && !pDstBox)
	{
		if(DstSubresource)
		{
				CRY_ASSERT_MESSAGE(0,"DstSubresource is not zero");
				return;
		}

		CCryDXPSBuffer* pDst	=	(CCryDXPSBuffer*)pDstResource;
		const uint8* pSrcData	=	reinterpret_cast<const uint8*>(pSrc);
		uint8*	pDstData;//			=	pDst->RawData();
		pDst->Map(D3D11_MAP_WRITE_DISCARD,0,(void**)&pDstData);
//		for(uint32 a=0,Size=pDst->Size();a<Size;a++)
//			pDstData[a]	=	pSrcData[a];
		memcpy(pDstData,pSrcData,pDst->Size());
		pDst->Unmap();
	}
	else
	if(pDstResource->Type()==EDXPS_RT_TEX2D)
	{
		CCryDXPSTexture2D* pDst	=	(CCryDXPSTexture2D*)pDstResource;
		if(pDst && pSrc)
		{
			D3D11_MAPPED_SUBRESOURCE pDstPtr;
			if(pDst->Map(DstSubresource,D3D11_MAP_WRITE_DISCARD,0,&pDstPtr)<0)
			{
				CRY_ASSERT_MESSAGE(0,"Wrong subresource!");
				return;
			}

			//modulo to get mips out of array textures/cubemaps
			if(pDst->MipLevels())
				DstSubresource%=pDst->MipLevels();
			else
				DstSubresource=0;

			const int	DstStartX	=	(pDstBox?pDstBox->left:0)>>DstSubresource;
			const int	DstEndX		=	std::max(1u, (pDstBox?pDstBox->right:pDst->VirtualSizeX())>>DstSubresource);
			const int	DstStartY	=	(pDstBox?pDstBox->top:0)>>DstSubresource;
			const int	DstEndY		=	std::max(1u, (pDstBox?pDstBox->bottom:pDst->VirtualSizeY())>>DstSubresource);
			uint32 Format	=	pDst->GcmTexture()->format;

			if((Format&CELL_GCM_TEXTURE_LN))
			{
				if(DstStartY|DstStartX==0 && DstEndX==pDst->VirtualSizeX()&&DstEndY==pDst->VirtualSizeY())
					memcpy(pDstPtr.pData,pSrc,GCM_TextureFormatSize(DstEndX*DstEndY,pDst->GcmTexture()->format));
				else
				for(uint32 y=DstStartY;y<DstEndY;y++)//SLOW
				{
					const uint8*	pSrcData	=	&reinterpret_cast<const uint8*>(pSrc)[y*SrcRowPitch];
					uint8*	pDstData	=	&reinterpret_cast<uint8*>(pDstPtr.pData)[y*pDstPtr.RowPitch];
					for(uint32	x		=GCM_TextureFormatSize(DstStartX,pDst->GcmTexture()->format),
											xend=GCM_TextureFormatSize(DstEndX,pDst->GcmTexture()->format);x<xend;x++)
							pDstData[x]	=	pSrcData[x];
				}
			}
			else
			{
				Format &= ~(CELL_GCM_TEXTURE_LN|CELL_GCM_TEXTURE_UN);
				if(Format==CELL_GCM_TEXTURE_COMPRESSED_DXT1)
					memcpy(pDstPtr.pData,pSrc,std::max(pDst->VirtualSizeX()>>DstSubresource,1u)*std::max(pDst->VirtualSizeY()>>DstSubresource,1u)/2);
				else
				if(	Format==CELL_GCM_TEXTURE_COMPRESSED_DXT23 ||
						Format==CELL_GCM_TEXTURE_COMPRESSED_DXT45)
					memcpy(pDstPtr.pData,pSrc,std::max(pDst->VirtualSizeX()>>DstSubresource,1u)*std::max(pDst->VirtualSizeY()>>DstSubresource,1u));
				else
				if(pDst->SizeX()==pDst->SizeY())
					Linear2SwizzleLite(pDstPtr.pData,(const uint8*)pSrc,SrcRowPitch,std::max(pDst->VirtualSizeX()>>DstSubresource,1u),std::max(pDst->VirtualSizeY()>>DstSubresource,1u),GCM_TextureFormatSize(1,pDst->GcmTexture()->format),DstStartX,DstStartY,DstEndX,DstEndY);
				else
					Linear2Swizzle(pDstPtr.pData,(const uint8*)pSrc,SrcRowPitch,std::max(pDst->VirtualSizeX()>>DstSubresource,1u),std::max(pDst->VirtualSizeY()>>DstSubresource,1u),GCM_TextureFormatSize(1,pDst->GcmTexture()->format),DstStartX,DstStartY,DstEndX,DstEndY);		
			}
			pDst->Unmap(DstSubresource);
		}
		else
		{
			CRY_ASSERT_MESSAGE(0,"No src or dest data!");
		}
	}
	else
	if(pDstResource->Type()==EDXPS_RT_TEX3D && DstSubresource==0)
	{
		CCryDXPSTexture2D* pDst	=	(CCryDXPSTexture2D*)pDstResource;

		D3D11_MAPPED_SUBRESOURCE pDstPtr;
		if(pDst->Map(0,D3D11_MAP_WRITE_DISCARD,0,&pDstPtr)<0)
		{
			CRY_ASSERT_MESSAGE(0,"Failed to map texture!");
			return;
		}
		if(pDst->GcmTexture()->format&CELL_GCM_TEXTURE_LN)
		{
			const uint32 S	=	GCM_TextureFormatSize((pDstBox->right*pDstBox->bottom*pDstBox->back),pDst->GcmTexture()->format);
			memcpy(pDstPtr.pData,pSrc,S);
		}
		else
		{
//			const uint32 Pitch1D	=SrcRowPitch;
			//const uint32 Pitch2D	=Pitch1D*pDst->SizeY();
			//for(uint32 a=0,S=pDstBox->back;a<S;a++)
						//Linear2SwizzleLite(&reinterpret_cast<uint8*>(pDstPtr.pData)[a*Pitch2D],&static_cast<const uint8*>(pSrc)[a*Pitch2D],Pitch1D,pDst->VirtualSizeX(),pDst->VirtualSizeX(),GCM_TextureFormatSize(1,pDst->GcmTexture()->format),0,0,pDst->SizeX(),pDst->VirtualSizeY());
			cellGcmConvertSwizzleFormat(pDstPtr.pData,pSrc,
																	0,0,0,pDstBox->right,pDstBox->bottom,pDstBox->bottom,
																	0,0,0,pDstBox->right,pDstBox->bottom,pDstBox->bottom,
																	pDstBox->right,pDstBox->bottom,pDstBox->bottom,
																	cellUtilLog2(GCM_TextureFormatSize(1,pDst->GcmTexture()->format)),
																	GCM_TextureFormatSize(1,pDst->GcmTexture()->format),
																	CELL_GCM_FALSE,
																	3,
																	0);
		}
		pDst->Unmap(0);


/*		if(pDst && pSrc)
		{
			const int	DstStartX	=	pDstBox->left:(unsigned int)0)>>DstSubresource;
			const int	DstEndX		=	std::max(std::min(pDstBox?pDstBox->right:(unsigned int)0,pDst->VirtualSizeX())>>DstSubresource,1u);
			const int	DstStartY	=	(pDstBox?pDstBox->top:(unsigned int)0)>>DstSubresource;
			const int	DstEndY		=	std::max(std::min(pDstBox?pDstBox->bottom:(unsigned int)0,pDst->VirtualSizeY())>>DstSubresource,1u);
			uint32 Format	=	pDst->GcmTexture()->format;

			D3D11_MAPPED_SUBRESOURCE pDstPtr;
			if(pDst->Map(DstSubresource,D3D11_MAP_WRITE_DISCARD,0,&pDstPtr)<0)
				return;

			for(uint32 y=DstStartY;y<DstEndY;y++)//SLOW
			{
				const uint8*	pSrcData	=	&reinterpret_cast<const uint8*>(pSrc)[y*SrcRowPitch];
				uint8*	pDstData	=	&reinterpret_cast<uint8*>(pDstPtr.pData)[y*pDstPtr.RowPitch];
				for(uint32	x		=GCM_TextureFormatSize(DstStartX,pDst->GcmTexture()->format),
										xend=GCM_TextureFormatSize(DstEndX,pDst->GcmTexture()->format);x<xend;x++)
						pDstData[x]	=	pSrcData[x];
			}
			pDst->Unmap(DstSubresource);
		}*/
	}
	else
	{
		CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	}
}

void CCryDXPSRenderDevice::GenerateMips(ID3D11ShaderResourceView *pShaderResourceView)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::ResolveSubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,ID3D11Resource *pSrcResource,uint32 SrcSubresource,DXGI_FORMAT Format)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::VSGetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppConstantBuffers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::PSGetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView **ppShaderResourceViews)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::PSGetShader(ID3D11PixelShader **ppPixelShader)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::PSGetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState **ppSamplers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::VSGetShader(ID3D11VertexShader **ppVertexShader)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::PSGetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppConstantBuffers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::IAGetInputLayout(ID3D11InputLayout **ppInputLayout)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::IAGetVertexBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppVertexBuffers,uint32*pStrides,uint32 *pOffsets)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::IAGetIndexBuffer(ID3D11Buffer **pIndexBuffer,DXGI_FORMAT *Format,uint32*Offset)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSGetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppConstantBuffers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSGetShader(ID3D11GeometryShader **ppGeometryShader)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY *pTopology)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::VSGetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView **ppShaderResourceViews)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::VSGetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState **ppSamplers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GetPredication(ID3D11Predicate **ppPredicate,BOOL *pPredicateValue)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSGetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView **ppShaderResourceViews)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GSGetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState **ppSamplers)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::OMGetRenderTargets(uint32 NumViews,ID3D11RenderTargetView **ppRenderTargetViews,ID3D11DepthStencilView **ppDepthStencilView)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::OMGetBlendState(ID3D11BlendState **ppBlendState,float BlendFactor[ 4 ],uint32*pSampleMask)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::OMGetDepthStencilState(ID3D11DepthStencilState **ppDepthStencilState,uint32*pStencilRef)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::SOGetTargets(uint32 NumBuffers,ID3D11Buffer **ppSOTargets,uint32* pOffsets)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::RSGetState(ID3D11RasterizerState **ppRasterizerState)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::RSGetViewports(uint32* NumViewports,D3D11_VIEWPORT *pViewports)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::RSGetScissorRects(uint32* NumRects,D3D11_RECT *pRects)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

long CCryDXPSRenderDevice::GetDeviceRemovedReason( void)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::SetExceptionMode(uint32 intRaiseFlags)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

uint32 CCryDXPSRenderDevice::GetExceptionMode( void)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return 0;
}

long CCryDXPSRenderDevice::GetPrivateData(const ECryGUID & guid,uint32*pDataSize,void *pData)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::SetPrivateData(const ECryGUID & guid,uint32 DataSize,const void *pData)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::SetPrivateDataInterface( const ECryGUID & guid,const IUnknown *pData)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}
void CCryDXPSRenderDevice::SetTextFilterSize(unsigned int Width,unsigned int Height)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::GetTextFilterSize(unsigned int* Width,unsigned int* Height)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}


void CCryDXPSRenderDevice::ClearState( void)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

void CCryDXPSRenderDevice::Flush( void)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

long CCryDXPSRenderDevice::CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Texture2D **ppTexture)
{
#ifdef _DEBUG
	while(!pDesc->Width || !pDesc->Height)
	{
		int a=0;
	}
#endif
	*ppTexture	=	CRY_DXPS_CREATE(CCryDXPSTexture2D,(	pDesc->Width,
																										pDesc->Height,
																										pDesc->MipLevels,
																										pDesc->Format,
																										ECDXPSTCF_NONE,
																										pDesc->ArraySize==6));

	if(pInitialData)
	{
		D3D11_BOX Box;
		Box.back	=	1;
		Box.front	=
		Box.left	=
		Box.top		=	0;
		Box.right	=	pDesc->Width;
		Box.bottom=	pDesc->Height;

		for(uint32 b=0,ArraySize=(pDesc->ArraySize==6)?6:1;b<ArraySize;b++)
		for(uint32 a=0;a<pDesc->MipLevels;a++)
		{
			const uint32 Idx	=	a+b*pDesc->MipLevels;
			const D3D11_SUBRESOURCE_DATA& rIData	=	pInitialData[Idx];
			UpdateSubresource(*ppTexture,Idx,&Box,rIData.pSysMem,rIData.SysMemPitch,0);
		}
	}
	return (*ppTexture!=0)-1;
}

long CCryDXPSRenderDevice::CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Texture3D **ppTexture)
{
	*ppTexture	=	CRY_DXPS_CREATE(CCryDXPSTexture3D,(	pDesc->Width,
																										pDesc->Height,
																										pDesc->Depth,
																										pDesc->MipLevels,
																										pDesc->Format));
	if(pInitialData)
	{
		D3D11_BOX Box;
		Box.front	=	
		Box.left	=
		Box.top		=	0;

		Box.right	=	pDesc->Width;
		Box.bottom=	pDesc->Height;
		Box.back	=	pDesc->Depth;
		UpdateSubresource(*ppTexture,0,&Box,pInitialData->pSysMem,pInitialData->SysMemPitch,0);
	}
	return (*ppTexture!=0)-1;
}

long CCryDXPSRenderDevice::CreateGeometryShader(const void *pShaderBytecode,size_t Size,ID3D11ClassLinkage *pClassLinkage,ID3D11GeometryShader **ppGeometryShader)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode,size_t BytecondeLength,const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,  UINT NumEntries,const UINT *pBufferStrides,UINT NumStrides,UINT RasterizedStream,ID3D11ClassLinkage *pClassLinkage,ID3D11GeometryShader **ppGeometryShader)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::CreateQuery(const D3D11_QUERY_DESC *pQueryDesc,ID3D11Query **ppQuery)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::CreatePredicate(const D3D11_QUERY_DESC *pPredicateDesc,ID3D11Predicate **ppPredicate)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::CreateCounter(const D3D11_COUNTER_DESC *pCounterDesc,ID3D11Counter **ppCounter)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

long CCryDXPSRenderDevice::CheckFormatSupport(DXGI_FORMAT Format,uint32*pFormatSupport)
{
	switch(Format)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_B5G6R5_UNORM:
			*pFormatSupport=D3D11_FORMAT_SUPPORT_TEXTURE2D;
			return 0;
	}
	return -1;
}

long CCryDXPSRenderDevice::CheckMultisampleQualityLevels(DXGI_FORMAT Format,uint32 SampleCount,uint32*pNumQualityLevels)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

void CCryDXPSRenderDevice::CheckCounterInfo(D3D11_COUNTER_INFO *pCounterInfo)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
}

long CCryDXPSRenderDevice::CheckCounter(const D3D11_COUNTER_DESC *pDesc,D3D11_COUNTER_TYPE *pType,uint32*pActiveCounters,char* szName,uint32 *pNameLength,char* szUnits,uint32 *pUnitsLength,char* szDescription,uint32 *pDescriptionLength)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

uint32 CCryDXPSRenderDevice::GetCreationFlags( void)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return 0;
}

long CCryDXPSRenderDevice::OpenSharedResource(int hResource,const CRYIID & ReturnedInterface,void **ppResource)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

void CCryDXPSRenderDevice::ResetRSX()
{
	for(uint32 a=0;a<16;a++)
	{
		if(m_pSampler[a])
		{
			static const float fZeroBias = 0.f;
			m_pSamplerMipBias[a] = 0;
			PSSetSamplers(a,1,&m_pSampler[a],&fZeroBias,&m_pSamplerSRGB[a]);
		}
	}
	if(m_pBlendState)
			OMSetBlendState(m_pBlendState,m_BlendFactor,0);
	if(m_pInputLayout)
		IASetInputLayout(m_pInputLayout);
	if(m_pDepthStencilState)
		OMSetDepthStencilState(m_pDepthStencilState,m_StencilRef);
	if(m_pRasterizerState)
		RSSetState(m_pRasterizerState,m_RasterizerSRGB);
}


uint32 CryMallocRSX(const unsigned int cSize, void*& rpMem)
{
	//try to allocate as many 16 MB blocks as possible, as long as they are consecutive, keep going
	uint32 allocated = 0;
	int remSize = cSize;
	void* pMemLast = NULL;
	while(remSize > 0)
	{
		int remSizeLast = remSize;
		remSize -= 16 * 1024 * 1024;
		const uint32 cCurAllocated = (remSize>0)?16 * 1024 * 1024 : remSizeLast;
		uint32 handle = tdLayer0::Memory().Allocate(cCurAllocated, 128);
		void *pMem = tdLayer0::Memory().ResolveHandle(handle);
		if(pMem)
		{
			if(allocated == 0)
			{
				allocated += cCurAllocated;
				pMemLast = pMem;
				rpMem = pMem;
			}
			else
			{
				//check if consecutive
				if((uint32)pMemLast + 16 * 1024 * 1024 == (uint32)pMem)
				{
					allocated += cCurAllocated;
					pMemLast = pMem;
				}
				else
				{
					tdLayer0::Memory().Free(handle);
					break;
				}
			}
		}
	}
	return allocated;
}

NPPU::SFrameProfileRSXData& GetFrameStatsSPUThread()
{
	return tdLayer0::Device()->GetFrameStatsSPUThread();
}

void WaitFinishThread(const bool cSwitchSPUs = false)
{
#if defined(CRY_DXPS_DEVICETHREAD)
	tdLayer0::Device()->ThreadFinish();
#endif
	g_SwitchSPUs = cSwitchSPUs;
}

//============================================================================================================

void sPrintConsts(int nID, ID3D11Buffer *pConstantBuffers)
{
  char str[1024];
  string sVars;

  D3D11_BUFFER_DESC Desc;
  pConstantBuffers->GetDesc(&Desc);
  int nFloats = Desc.ByteWidth / 4;
  int nF = 0;
  if (nID == CB_PER_BATCH)
    nF = 16;
  else
  if (nID == CB_PER_INSTANCE)
    nF = 16;
  else
  if (nID == CB_PER_FRAME)
    nF = 8;
  else
  if (nID == CB_PER_MATERIAL)
    nF = 16;
  else
  if (nID == CB_PER_LIGHT)
    nF = 4;
  else
    nF = 0;
  nFloats = __min(nF, nFloats);
  float *pData = (float *)pConstantBuffers->RawData();
  for (int i=0; i<nFloats; i+=4)
  {
    sprintf(str, " [%.3f %.3f %.3f %.3f]", pData[i], pData[i+1], pData[i+2], pData[i+3]);
    sVars += str;
  }
  gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s\n", sVars.c_str());
}

void CCryDXPSRenderDevice::VSSetConstantBuffers_Log(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers)
{
#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log == 3)
  {
    int nBytes = 0;
    if (ppConstantBuffers && *ppConstantBuffers)
    {
      D3D11_BUFFER_DESC Desc;
      (*ppConstantBuffers)->GetDesc(&Desc);
      nBytes = Desc.ByteWidth;
    }
    gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x) (%d floats)\n", "D3DDevice::VSSetConstantBuffers", Offset, NumBuffers, *ppConstantBuffers, nBytes/4);
    if (*ppConstantBuffers)
      sPrintConsts(Offset, *ppConstantBuffers);
  }
#endif
}
void CCryDXPSRenderDevice::PSSetConstantBuffers_Log(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers)
{
#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log == 3)
  {
    int nBytes = 0;
    if (ppConstantBuffers && *ppConstantBuffers)
    {
      D3D11_BUFFER_DESC Desc;
      (*ppConstantBuffers)->GetDesc(&Desc);
      nBytes = Desc.ByteWidth;
    }
    gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "%s(%d, %d, 0x%x) (%d floats)\n", "D3DDevice::PSSetConstantBuffers", Offset, NumBuffers, *ppConstantBuffers, nBytes/4);
    if (*ppConstantBuffers)
      sPrintConsts(Offset, *ppConstantBuffers);
  }
#endif
}

void CCryDXPSRenderDevice::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS Render Device");
		Sizer->AddObject(this,sizeof(CDXPSRDThread));
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS Render Device Thread");
			Sizer->AddObject(this,sizeof(CDXPSRDThread));
		}
	}
}

void CCryDXPSRenderDevice::UpdateCmdBuffer()
{
	m_Thread.UpdateCmdBuffer();
}

void CCryDXPSRenderDevice::ViewProjMatrix(const float* pViewProjMat)
{
	m_ViewProjMat[0]	=	pViewProjMat[0];
	m_ViewProjMat[1]	=	pViewProjMat[1];
	m_ViewProjMat[2]	=	pViewProjMat[2];
	m_ViewProjMat[3]	=	pViewProjMat[3];
	m_ViewProjMat[4]	=	pViewProjMat[4];
	m_ViewProjMat[5]	=	pViewProjMat[5];
	m_ViewProjMat[6]	=	pViewProjMat[6];
	m_ViewProjMat[7]	=	pViewProjMat[7];
	m_ViewProjMat[8]	=	pViewProjMat[8];
	m_ViewProjMat[9]	=	pViewProjMat[9];
	m_ViewProjMat[10]	=	pViewProjMat[10];
	m_ViewProjMat[11]	=	pViewProjMat[11];
	m_ViewProjMat[12]	=	pViewProjMat[12];
	m_ViewProjMat[13]	=	pViewProjMat[13];
	m_ViewProjMat[14]	=	pViewProjMat[14];
	m_ViewProjMat[15]	=	pViewProjMat[15];

#if defined(CRY_DXPS_RASTERTHREAD)
	SoftRast().ViewProjMatrix(pViewProjMat);
#endif
}


