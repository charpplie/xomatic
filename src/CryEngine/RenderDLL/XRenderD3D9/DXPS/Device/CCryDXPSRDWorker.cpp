#include "StdAfx.h" 
#include "../Layer0/CCryDXPS.hpp"
#include "CCryDXPSRDThread.hpp"
#include "Jobs/CCryDXPSRDJob.hpp"




	typedef CellGcmContextData TCellGCMContext;


//if RSX limited, 2 is the best mode, 3 also results in flickering with rendering thread
//#define SUPP_COND_REND_3

#ifndef __SPU__
	volatile int g_FlipSpinLock _ALIGN(128);
	void PrepareFlipCallback(void* pArg)
	{
		CrySpinLock(&g_FlipSpinLock, 0, 1);
		__sync();
		const int32_t cFrameID = g_FlipVars.flipFrameID;
		g_FlipVars.flipFrameID = cellGcmSetPrepareFlip(gCellGcmCurrentContext, (uint32)pArg);
		if(cFrameID == g_FlipVars.flipFrameID || g_FlipVars.flipFrameID == CELL_GCM_ERROR_FAILURE)
		{
			printf("cellGcmSetPrepareFlip failed for ID=%d\n",(uint32)pArg);
			snPause();
		}
		g_FlipVars.flipDrawCallID = g_FlipVars.flipDrawCallIDSetup;//make visible to DXPSVBIssued
		READ_WRITE_BARRIER;
		__sync();
		g_FlipSpinLock = 0;
	}
	#define __spu_toggle_ppu_callback(func, arg) PrepareFlipCallback(arg)
	#define cellGcmUpdateGlobalPPUContext()
	#define __spu_sync_before_callback()







#endif

using namespace CRY_DXPS_GCMNAMESPACE;

SFlipVars g_FlipVars _ALIGN(128);

volatile void* SFlipVars::GetFlipLockedTarget() const
{





	return pFlipLockedTarget;

}
void SFlipVars::FetchFlipVars(SFlipVars& rDest) const
{










}
void SFlipVars::PutFlipVars(const SFlipVars& crSrc)
{






















}

#if !defined(__SPU__)
CDXPSRDWorker::CDXPSRDWorker(CCryDXPSGCMSyncMan* __restrict pSyncMan):
m_DeferCondRendering(0),
m_pVSDesc(0),
m_pPSDesc(0),
m_SurfaceOffsetColor(0),
m_SurfaceOffsetDepth(0),
m_Flushing(true),
m_Using704(false),
m_pCondRenderingVar(&CRenderer::CV_r_ConditionalRendering),
m_VertexAttributeMask(~0),
#if defined(CRY_DXPS_CACHESTATES)
m_DirtyStates(0),
m_DirtyTextures(0),
m_DirtySamplers(0),
#endif
m_DirtyFlags(0),
m_ShaderTransferInFlight(false),
m_pSyncMan(pSyncMan),
m_ConstBufferTransferActive(false),
m_GammaOutEnable(CELL_GCM_FALSE)
{
	for(uint32 a=0;a<16;a++)
		m_pVertexBuffer[a]	=	0;

#if defined(CRY_DXPS_LSCONSTANTS_PROFILE)
	for(uint32 a=0;a<sizeof(m_ConstBufferLSProfile)/sizeof(m_ConstBufferLSProfile[0]);a++)
		m_ConstBufferLSProfile[a]=0;
#endif

	ShaderCache().Reset();
	ResetColorMask();
#if defined(CRY_DXPS_THREAD_DEBUGDATA)
	for(uint32 a=0;a<CRY_DXPS_THREAD_DEBUGDATA;a++)
		m_DebugData[a]	=	0;
#endif
	SNSTARTMARKER(SNTM_FLUSHING,"Init Flushing");
}

void memset_large_nocache_128(void* pDest, unsigned int val, unsigned int size)
{
	//implement per DMA as for SPU
	if(size == 0)
		return;
	//16 dma transfers can run in parallel
	//create 16x128 byte of 
	const vec_uint4 cSplatVal = {val, val, val, val};
	vec_uint4 localStorage[1024] _ALIGN(128);//aligned 16 KB buffer
	for(uint32 i=0; i<1024; i+=16)
	{
		localStorage[i]			= cSplatVal;
		localStorage[i+1]		= cSplatVal;
		localStorage[i+2]		= cSplatVal;
		localStorage[i+3]		= cSplatVal;
		localStorage[i+4]		= cSplatVal;
		localStorage[i+5]		= cSplatVal;
		localStorage[i+6]		= cSplatVal;
		localStorage[i+7]		= cSplatVal;
		localStorage[i+8]		= cSplatVal;
		localStorage[i+9]		= cSplatVal;
		localStorage[i+10]	= cSplatVal;
		localStorage[i+11]	= cSplatVal;
		localStorage[i+12]	= cSplatVal;
		localStorage[i+13]	= cSplatVal;
		localStorage[i+14]	= cSplatVal;
		localStorage[i+15]	= cSplatVal;
	}
	uint32 bytesLeft = size;
	const uint32 cTransferBlockSize = 16*1024;
	uint32 curDest = (uint32)pDest;
	do
	{
		const uint32 cTransferSize = (bytesLeft >= cTransferBlockSize)?cTransferBlockSize : bytesLeft;
		memcpy((void*)curDest, (const void*)localStorage, cTransferSize);
		bytesLeft -= cTransferSize;
		curDest += cTransferBlockSize;
	}while(bytesLeft > 0);
}
#endif//__SPU__

#if !defined(_RELEASE) || defined(__SPU__)
//type is passed to avoid volatile access (too expensive on SPU)
void CDXPSRDWorker::WorkOn(SPU_DOMAIN_LOCAL const CDXPSRDJob* const __restrict pJob, const EDXPSJob cType)
{
	SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::WorkOn")
	switch(cType)
	{
		case EDXPSJ_DRAWINDEXED:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJDrawIndexedSPU>()));
			break;
		case EDXPSJ_COPYRESOURCESCALED:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJCopyResourceScaledSPU>()));
			break;
		case EDXPSJ_COPYRESOURCE:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJCopyResourceSPU>()));
			break;
		//case EDXPSJ_COPY:
		//	Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJCopySPU>()));
		//	break;
		case EDXPSJ_RENDERTARGET1X:
		case EDXPSJ_RENDERTARGET2X:
		case EDXPSJ_RENDERTARGET4X:
		case EDXPSJ_RENDERTARGET1XATC:
		case EDXPSJ_RENDERTARGET2XATC:
		case EDXPSJ_RENDERTARGET4XATC:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJRenderTargetSPU>()),cType-EDXPSJ_RENDERTARGET1X);
			break;
		case EDXPSJ_VIEWPORT:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJViewPortSPU>()));
			break;
		case EDXPSJ_CLEARCOLOR:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJClearColorSPU>()));
			break;
		//case EDXPSJ_CLEARTEXTURE:
		//	Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJClearTextureSPU>()));
		//	break;
		case EDXPSJ_CLEARDEPTHSTENCIL:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJClearDepthStencilSPU>()));
			break;
		case EDXPSJ_SWAP:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJSwapSPU>()));
			break;
		case EDXPSJ_POINTSPRITE:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJPointSpriteSPU>()));
			break;
		case EDXPSJ_UPDATECMDBUF:



			break;
		case EDXPSJ_STATE_VCONSTBUFFER0:
		case EDXPSJ_STATE_VCONSTBUFFER1:
		case EDXPSJ_STATE_VCONSTBUFFER2:
		case EDXPSJ_STATE_VCONSTBUFFER3:
		case EDXPSJ_STATE_VCONSTBUFFER4:
		case EDXPSJ_STATE_VCONSTBUFFER5:
		case EDXPSJ_STATE_VCONSTBUFFER6:
		case EDXPSJ_STATE_VCONSTBUFFER7:
		case EDXPSJ_STATE_VCONSTBUFFER8:
		case EDXPSJ_STATE_VCONSTBUFFER9:
		case EDXPSJ_STATE_VCONSTBUFFER10:
		case EDXPSJ_STATE_VCONSTBUFFER11:
		case EDXPSJ_STATE_VCONSTBUFFER12:
		case EDXPSJ_STATE_VCONSTBUFFER13:
		case EDXPSJ_STATE_VCONSTBUFFER14:
		case EDXPSJ_STATE_VCONSTBUFFER15:
		case EDXPSJ_STATE_PCONSTBUFFER0:
		case EDXPSJ_STATE_PCONSTBUFFER1:
		case EDXPSJ_STATE_PCONSTBUFFER2:
		case EDXPSJ_STATE_PCONSTBUFFER3:
		case EDXPSJ_STATE_PCONSTBUFFER4:
		case EDXPSJ_STATE_PCONSTBUFFER5:
		case EDXPSJ_STATE_PCONSTBUFFER6:
		case EDXPSJ_STATE_PCONSTBUFFER7:
		case EDXPSJ_STATE_PCONSTBUFFER8:
		case EDXPSJ_STATE_PCONSTBUFFER9:
		case EDXPSJ_STATE_PCONSTBUFFER10:
		case EDXPSJ_STATE_PCONSTBUFFER11:
		case EDXPSJ_STATE_PCONSTBUFFER12:
		case EDXPSJ_STATE_PCONSTBUFFER13:
		case EDXPSJ_STATE_PCONSTBUFFER14:
		case EDXPSJ_STATE_PCONSTBUFFER15:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateConstBufferSPU>()),cType-EDXPSJ_STATE_VCONSTBUFFER0);
			break;
		case EDXPSJ_STATE_TEXTURE0:
		case EDXPSJ_STATE_TEXTURE1:
		case EDXPSJ_STATE_TEXTURE2:
		case EDXPSJ_STATE_TEXTURE3:
		case EDXPSJ_STATE_TEXTURE4:
		case EDXPSJ_STATE_TEXTURE5:
		case EDXPSJ_STATE_TEXTURE6:
		case EDXPSJ_STATE_TEXTURE7:
		case EDXPSJ_STATE_TEXTURE8:
		case EDXPSJ_STATE_TEXTURE9:
		case EDXPSJ_STATE_TEXTURE10:
		case EDXPSJ_STATE_TEXTURE11:
		case EDXPSJ_STATE_TEXTURE12:
		case EDXPSJ_STATE_TEXTURE13:
		case EDXPSJ_STATE_TEXTURE14:
		case EDXPSJ_STATE_TEXTURE15:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateTextureSPU>()),cType-EDXPSJ_STATE_TEXTURE0);
			break;
		case EDXPSJ_STATE_SAMPLER0:
		case EDXPSJ_STATE_SAMPLER1:
		case EDXPSJ_STATE_SAMPLER2:
		case EDXPSJ_STATE_SAMPLER3:
		case EDXPSJ_STATE_SAMPLER4:
		case EDXPSJ_STATE_SAMPLER5:
		case EDXPSJ_STATE_SAMPLER6:
		case EDXPSJ_STATE_SAMPLER7:
		case EDXPSJ_STATE_SAMPLER8:
		case EDXPSJ_STATE_SAMPLER9:
		case EDXPSJ_STATE_SAMPLER10:
		case EDXPSJ_STATE_SAMPLER11:
		case EDXPSJ_STATE_SAMPLER12:
		case EDXPSJ_STATE_SAMPLER13:
		case EDXPSJ_STATE_SAMPLER14:
		case EDXPSJ_STATE_SAMPLER15:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateSamplerSPU>()),cType-EDXPSJ_STATE_SAMPLER0);
			break;
		case EDXPSJ_STATE_VERTEXSHADER:
		case EDXPSJ_STATE_PIXELSHADER:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateShaderSPU>()),cType-EDXPSJ_STATE_VERTEXSHADER);
			break;
		case EDXPSJ_STATE_VERTEXBUFFER0:
		case EDXPSJ_STATE_VERTEXBUFFER1:
		case EDXPSJ_STATE_VERTEXBUFFER2:
		case EDXPSJ_STATE_VERTEXBUFFER3:
		case EDXPSJ_STATE_VERTEXBUFFER4:
		case EDXPSJ_STATE_VERTEXBUFFER5:
		case EDXPSJ_STATE_VERTEXBUFFER6:
		case EDXPSJ_STATE_VERTEXBUFFER7:
		case EDXPSJ_STATE_VERTEXBUFFER8:
		case EDXPSJ_STATE_VERTEXBUFFER9:
		case EDXPSJ_STATE_VERTEXBUFFER10:
		case EDXPSJ_STATE_VERTEXBUFFER11:
		case EDXPSJ_STATE_VERTEXBUFFER12:
		case EDXPSJ_STATE_VERTEXBUFFER13:
		case EDXPSJ_STATE_VERTEXBUFFER14:
		case EDXPSJ_STATE_VERTEXBUFFER15:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateVertexBufferSPU>()),cType-EDXPSJ_STATE_VERTEXBUFFER0);
			break;
		case EDXPSJ_STATE_INPUTLAYOUT:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateInputLayoutSPU>()));
			break;
		case EDXPSJ_STATE_BLENDFACTOR:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateBlendFactorSPU>()));
		case EDXPSJ_STATE_BLENDSTATE:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateBlendSPU>()));
			break;
		case EDXPSJ_STATE_DEPTHSTENCILSTATE:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateStencilSPU>()));
			break;
		case EDXPSJ_STATE_RASTERIZERSTATE:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateRasterizerSPU>()));
			break;
		case EDXPSJ_STATE_SCISSOR:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateScissorSPU>()));
			break;
		case EDXPSJ_STATE_DEPTHBOUNDS:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateDepthBoundsSPU>()));
			break;
		case EDXPSJ_STATE_TOPOLOGY:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateTopologySPU>()));
			break;
		case EDXPSJ_STATE_PASS:
			Job<CRY_DXPS_USESTATECACHE>(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStatePassSPU>()));
			break;
		case EDXPSJ_STATE_DEBUGPROFILIE:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateStateDebugProfileSPU>()));
			break;	
		case EDXPSJ_STATE_DEBUGDATA:
			Job(SPU_LOCAL_REF(pJob->Job<CDXPSRDJUpdateDebugDataSPU>()));
			break;	
	}
}










#if defined(CRY_DXPS_LSCONSTANTS)
uint8 DXPSRAS_ConstCache[EDXPS_MAX] _ALIGN(128)	SPU_LOCAL;
uint32 DXPSRAS_ConstCacheOffsets[32] _ALIGN(128)	SPU_LOCAL ={	EDXPS_BATCH_VS,
																																			EDXPS_INSTANCE_VS,
																																			EDXPS_STATIC_INSTANCE_VS,
																																			EDXPS_FRAME_VS,
																																			EDXPS_MATERIAL_VS,
																																			EDXPS_LIGHT_VS,
																																			EDXPS_SHADOWGEN_VS,
																																			EDXPS_SKIN_VS,
																																			EDXPS_SHAPE_VS,
																																			EDXPS_INSTANCE_DATA_VS,
																																			EDXPS_DUMMY10,
																																			EDXPS_DUMMY11,
																																			EDXPS_DUMMY12,
																																			EDXPS_DUMMY13,
																																			EDXPS_DUMMY14,
																																			EDXPS_DUMMY15,
																																			EDXPS_BATCH_PS,
																																			EDXPS_INSTANCE_PS,
																																			EDXPS_STATIC_INSTANCE_PS,
																																			EDXPS_FRAME_PS,
																																			EDXPS_MATERIAL_PS,
																																			EDXPS_LIGHT_PS,
																																			EDXPS_SHADOWGEN_PS,
																																			EDXPS_SKIN_PS,
																																			EDXPS_SHAPE_PS,
																																			EDXPS_INSTANCE_DATA_PS,
																																			EDXPS_DUMMY26,
																																			EDXPS_DUMMY27,
																																			EDXPS_DUMMY28,
																																			EDXPS_DUMMY29,
																																			EDXPS_DUMMY30,
																																			EDXPS_DUMMY31}; 
#endif

void CDXPSRDWorker::Job(const CDXPSRDJUpdateStateConstBufferSPU& rCBuffer,uint32 Idx)
{
	SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::Job_UpdateStateConstBuffer")
	
#ifndef __SPU__
	if(Idx>=32)
	{
		__debugbreak();
	}
#endif

	const uint32 SizeSrc				=	rCBuffer.ConstBufferSize();
#if defined(CRY_DXPS_LSCONSTANTS)
	uint8* pCBDst								=	DXPSRAS_ConstCache+DXPSRAS_ConstCacheOffsets[Idx];
	const uint8* pCBSrc					=	reinterpret_cast<const uint8*>(rCBuffer.ConstBufferRawData());
#if defined(CRY_DXPS_LSCONSTANTS_PROFILE)
	if(m_ConstBufferLSProfile[Idx]<SizeSrc)
		m_ConstBufferLSProfile[Idx]	=	SizeSrc;
#endif
#ifndef __SPU__
	if(DXPSRAS_ConstCacheOffsets[Idx+1]-DXPSRAS_ConstCacheOffsets[Idx]<SizeSrc)
	{
		__debugbreak();
	}
#endif
	//fence ensures subsequent transfer do not overwrite each other, maybe not necessary
	memtransfer_from_main_fenced(pCBDst,pCBSrc,SizeSrc,CB_BUF_TRANSFER_TAG);
	m_ConstBufferTransferActive = true;

#else //CRY_DXPS_LSCONSTANTS
	if(m_ConstBuffer[Idx].Data() && ((1<<(Idx&0xf))&CB_DYN_MASK))
		tdLayer0::FreeCCryDXPSBufferTemp(CCryDXPSBuffer::ClassPtr(const_cast<uint8*>(m_ConstBuffer[Idx].Data())));
//		tdLayer0::FreeCCryDXPSBufferTemp(reinterpret_cast<CCryDXPSBuffer*>(const_cast<uint8*>(m_ConstBuffer[Idx].Data()))-1);

	uint8* pCBDst					=	reinterpret_cast<uint8*>(rCBuffer.ConstBufferRawData());
#endif//CRY_DXPS_LSCONSTANTS

	m_ShaderCache.CBDirty(Idx);
	m_ConstBuffer[Idx].Init(pCBDst,SizeSrc);
	m_DirtyFlags	|=	Idx>=16?EDXPSDF_PIXELCONSTBUFFER:EDXPSDF_VERTEXCONSTBUFFER;
}

void CDXPSRDWorker::Job(const CDXPSRDJUpdateDebugDataSPU& rDebugData)
{
#if defined(CRY_DXPS_THREAD_DEBUGDATA)
	IF(rDebugData.Slot()<CRY_DXPS_THREAD_DEBUGDATA,1)
		m_DebugData[rDebugData.Slot()]	=	rDebugData.Data();
	else
		snPause();
#endif
}

void CDXPSRDWorker::Job(const CDXPSRDJUpdateStateShaderSPU& rShader,uint32 Idx)
{
	assert(Idx<2);
	if(Idx)
	{
		PShader(rShader.Shader());
		m_DirtyFlags		|=	EDXPSDF_PIXELSHADER;
		m_pPSDesc				=		0;
		m_ShaderCache.InvalidatePCB();

		const uint32 cLZSSSize = rShader.Size();
		m_pPSSrc				= (uint8*)cellGcmCpyUCodeLS(rShader.Program(), cLZSSSize, LOCAL_PS_BUFFER_SIZE - cLZSSSize);
		m_pPSDst				= cellGcmGetPSBuf();
	}
	else
	{
		VShader(rShader.Shader());
		m_DirtyFlags		|=	EDXPSDF_VERTEXSHADER;
		m_pVSDesc				=		0;
		m_ShaderCache.InvalidateVCB();

		const uint32 cLZSSSize = rShader.Size();
		m_pVSSrc				= (uint8*)cellGcmCpyVertexCodeLS(rShader.Program(), cLZSSSize, LOCAL_VS_BUFFER_SIZE - cLZSSSize);
		m_pVSDst				=	cellGcmGetVSBuf();
	}
//	m_DirtyFlags		|=	Idx?EDXPSDF_PIXELSHADER:EDXPSDF_VERTEXSHADER;
}



namespace
{
	ILINE void DXPSSetPrepareFlip
	(
		TCellGCMContext* const __restrict pContext, 
		const uint8_t cBufID, 
		SFlipVars& rFlipVars,
		const CDXPSRDJSwapSPU& crSwap,
		void *pLockedTarget,
		const tdResHandle cHandleDevThread
	)
	{
		volatile int& rFlipIDCur		= rFlipVars.flipIDCur;
		int curFlipID								= rFlipIDCur;
		curFlipID										= (curFlipID + 1) & 7;
		rFlipIDCur									= curFlipID;
		//between fetch and put of the flip vars must not happen any cache op
		rFlipVars.flipBufID					= cBufID;
		rFlipVars.flipModeUsed			= crSwap.FlipMode();
		rFlipVars.pFlipLockedTarget	=	pLockedTarget;
		if(crSwap.FlipMode() != 0)
		{
			rFlipVars.flipDrawCallIDSetup =	cHandleDevThread;
			g_FlipVars.PutFlipVars(rFlipVars);
			SPU_DOMAIN_MAIN void* pBufIDAsPtr = SPU_MAIN_PTR((void*)cBufID);
			SPU_DOMAIN_MAIN void* pFuncPtr		= SPU_MAIN_PTR((void*)crSwap.FlipFunc());
			__spu_sync_before_callback();
			__spu_toggle_ppu_callback(pFuncPtr, pBufIDAsPtr);
			cellGcmUpdateGlobalPPUContext();
		}
		else
		{
			rFlipVars.flipDrawCallID		=	cHandleDevThread;
			rFlipVars.flipFrameID				=	curFlipID;
			g_FlipVars.PutFlipVars(rFlipVars);
			uint32 *__restrict const pCurCmdBuffer = pContext->current;
			//write the hexcodes required for flip
			pCurCmdBuffer[0]	= 0x0004e944;	pCurCmdBuffer[1]	= cBufID;			pCurCmdBuffer[2]	= 0x00000000;
			pCurCmdBuffer[3]	= 0x00000000;	pCurCmdBuffer[4]	= 0x00000000;	pCurCmdBuffer[5]	= 0x00000000; 
			pCurCmdBuffer[6]	= 0x00040060;	pCurCmdBuffer[7]	= 0x56616661;	pCurCmdBuffer[8]	= 0x00040064; 
			pCurCmdBuffer[9]	= 0x00000030;	pCurCmdBuffer[10] = 0x0004006c;	pCurCmdBuffer[11] = 0x00000000;	
			pCurCmdBuffer[12] = 0x00040060;	pCurCmdBuffer[13] = 0x66616661;	pCurCmdBuffer[14] = 0x00040060;
			pCurCmdBuffer[15] = 0x56616661;	pCurCmdBuffer[16] = 0x00040064;	pCurCmdBuffer[17] = 0x00000030;	
			pCurCmdBuffer[18] = 0x00040068;	pCurCmdBuffer[19] = 0x00000001;	pCurCmdBuffer[20] = 0x00040060;
			pCurCmdBuffer[21] = 0x66616661;	pCurCmdBuffer[22] = 0x00000002;
			pContext->current += 92/sizeof(uint32);
		}
	}
}

extern SHWOccZBuffer HWZBuffer;

void CDXPSRDWorker::DownloadDepthBuffer(const uint32 BufferID,const float* pViewMat)
{
	IF(HWZBuffer.pHardwareZBuffer && HWZBuffer.pZBufferVMem,1)
	{
		const uint32	BufferSize	=	HWZBuffer.ZBufferSizeX*HWZBuffer.ZBufferSizeY*4;
		const uint32	BufferCount	=	EZBufferSize/BufferSize;
		IF(BufferID>=BufferCount,0)
			return;
		const uint32	SizeX				=	m_ViewPort.ViewPort().Width;
		const uint32 	SizeY				=	m_ViewPort.ViewPort().Height;
		IF(!SizeX || !SizeY,0)
			return;
		const uint32 	CenterX			=	(SizeX+HWZBuffer.ZBufferSizeX/2)/HWZBuffer.ZBufferSizeX/2;
		const uint32 	CenterY			=	(SizeY+HWZBuffer.ZBufferSizeY/2)/HWZBuffer.ZBufferSizeY/2;
		const	uint32	BufferSrc		=	BufferID?m_SurfaceOffsetDepth:HWZBuffer.ZBufferVMemRSXOff;
		const	uint32	BufferDst		=	HWZBuffer.HardwareZBufferRSXOff+BufferID*BufferSize;

		CellGcmTransferScale src;
		src.conversion	= CELL_GCM_TRANSFER_CONVERSION_TRUNCATE;
		src.format			= CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
		src.operation		= CELL_GCM_TRANSFER_OPERATION_SRCCOPY;
		src.clipX = 0;
		src.clipY = 0;
		src.clipW = SizeX;
		src.clipH = SizeY;
		src.outX	= 0;
		src.outY	= 0;
		src.outW	= HWZBuffer.ZBufferSizeX;    
		src.outH	= HWZBuffer.ZBufferSizeY;   
		src.inW		= SizeX;    
		src.inH		= SizeY;    
		src.ratioX = cellGcmGetFixedSint32(static_cast<float>(SizeX) / static_cast<float>(HWZBuffer.ZBufferSizeX) );
		src.ratioY = cellGcmGetFixedSint32(static_cast<float>(SizeY) / static_cast<float>(HWZBuffer.ZBufferSizeY) );
		src.pitch  = SizeX*4; 
		src.origin = CELL_GCM_TRANSFER_ORIGIN_CENTER;   
		src.interp = CELL_GCM_TRANSFER_INTERPOLATOR_ZOH; // point sampling
		src.offset = BufferSrc;// source offset
		src.inX	= cellGcmGetFixedUint16(src.clipX); 
		src.inY = cellGcmGetFixedUint16(src.clipY); 

		// set destination surface
		CellGcmTransferSurface dst;
		dst.format  = CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
		dst.pitch   = HWZBuffer.ZBufferSizeX*4; 
		dst.offset  = BufferDst;

		// copy
		cellGcmSetTransferScaleMode(CELL_GCM_TRANSFER_LOCAL_TO_MAIN, CELL_GCM_TRANSFER_SURFACE);
		cellGcmSetTransferScaleSurface(&src, &dst);
		SPU_DOMAIN_LOCAL void* pData;
		cellGcmSetInlineTransferPointer(BufferDst,sizeof(float)*16,&pData);
		memcpy(pData,pViewMat,sizeof(float)*16);
	}
}

void CDXPSRDWorker::Job(const CDXPSRDJSwapSPU& rSwap)
{
	SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::Job_Swap")
	using namespace CRY_DXPS_GCMNAMESPACE;
	CCryDXPSGCMSyncMan& __restrict rSyncMan = Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncDeviceThread();
#endif

	#ifdef SUPP_COND_REND_3
		m_DeferCondRendering = (*m_pCondRenderingVar == 3)?1:0;//update once a frame
	#endif
	#if defined(DRAWCALLDEBUGGING)
		m_DrawCall=0;
	#endif
	
	//fetch here to avoid cache lookups between atomic ops
	const tdResHandle cHandleDevThread = rSyncMan.HandleDeviceThread();
	void *pLockedTarget				= rSwap.TexFront()->RawPointer();
	const int cBufID					= rSwap.Frame();

#ifndef __SPU__
	#define flipVars g_FlipVars


#endif
	{
		CRY_DXPS_STALL_SCOPE





		bool messagePrinted = false;
		g_FlipVars.FetchFlipVars(flipVars);
		while(flipVars.pFlipLockedTarget)
		{
#ifndef __SPU__
			sys_timer_usleep(50);
			CRY_DXPS_STALL;












#endif
			g_FlipVars.FetchFlipVars(flipVars);//fetch again
		}




	}
	DXPSSetPrepareFlip((TCellGCMContext*)gCellGcmCurrentContext,cBufID, flipVars,rSwap, pLockedTarget, cHandleDevThread);
	#undef flipVars

	rSyncMan.IncDeviceThread(true);
	rSyncMan.SwapDeviceThread();
#if defined(CRY_DXPS_SNMARKERS)
	if(!m_Flushing)
	{
		   SNSTARTMARKER(SNTM_FLUSHING,"go on Flushing by Swap");
	}
#endif
	m_Flushing	=	true;
	cellGcmSetUserCommand(0);

#if defined(CRY_DXPS_SNMARKERS)
	SNSTARTMARKER(SNTM_FLIPREQUEST,"In Swap");
#endif
	cellGcmSetReportLocation(CELL_GCM_LOCATION_MAIN);

	m_DirtyFlags					=	~0;
	m_VertexAttributeMask	=	~0;
	m_LastBaseVL					=	~0;
	m_ShaderCache.Reset();
#ifndef __SPU__
	cellGcmGetAndResetRSXWaitTicks();
#endif

	// default RSX pipeline initialization settings
	cellGcmSetDitherEnable(CELL_GCM_FALSE);
	for(uint32 iTexSlot = 0;iTexSlot < 15;++iTexSlot)
	{
		// TODO: add console variable for quality control
		cellGcmSetTextureOptimization(iTexSlot, 8/*12 - is the maximum x360 value*/, CELL_GCM_TEXTURE_ISO_HIGH, CELL_GCM_TEXTURE_ANISO_LOW);
	}

	rSwap.TexFront()->Swap(*rSwap.TexBack());
	cellGcmSetFragmentProgramGammaEnable(m_GammaOutEnable);

	//must always come last to flush cmd buffer from spu
	CELL_GCM_FLUSH;



#if defined(CRY_DXPS_LSCONSTANTS_PROFILE)
	static uint32 DrawProfileData=0;
	DrawProfileData++;
	if((DrawProfileData&63)==0)
	{
		printf("EDXPS_BATCH_VS					=	0,\n");
		printf("EDXPS_INSTANCE_VS				=	EDXPS_BATCH_VS					+%d,\n",m_ConstBufferLSProfile[0]);
		printf("EDXPS_STATIC_INSTANCE_VS=	EDXPS_INSTANCE_VS				+%d,\n",m_ConstBufferLSProfile[1]);
		printf("EDXPS_FRAME_VS					=	EDXPS_STATIC_INSTANCE_VS+%d,\n",m_ConstBufferLSProfile[2]);
		printf("EDXPS_MATERIAL_VS				=	EDXPS_FRAME_VS					+%d,\n",m_ConstBufferLSProfile[3]);
		printf("EDXPS_LIGHT_VS					=	EDXPS_MATERIAL_VS				+%d,\n",m_ConstBufferLSProfile[4]);
		printf("EDXPS_SHADOWGEN_VS			=	EDXPS_LIGHT_VS					+%d,\n",m_ConstBufferLSProfile[5]);
		printf("EDXPS_SKIN_VS						=	EDXPS_SHADOWGEN_VS			+%d,\n",m_ConstBufferLSProfile[6]);
		printf("EDXPS_SHAPE_VS					=	EDXPS_SKIN_VS						+%d,\n",m_ConstBufferLSProfile[7]);
		printf("EDXPS_INSTANCE_DATA_VS	=	EDXPS_SHAPE_VS					+%d,\n",m_ConstBufferLSProfile[8]);
		printf("EDXPS_DUMMY10						=	EDXPS_INSTANCE_DATA_VS	+%d,\n",m_ConstBufferLSProfile[9]);
		printf("EDXPS_DUMMY11						=	EDXPS_DUMMY10,\n");
		printf("EDXPS_DUMMY12						=	EDXPS_DUMMY11,\n");
		printf("EDXPS_DUMMY13						=	EDXPS_DUMMY12,\n");
		printf("EDXPS_DUMMY14						=	EDXPS_DUMMY13,\n");
		printf("EDXPS_DUMMY15						=	EDXPS_DUMMY14,\n");
		printf("EDXPS_BATCH_PS					=	EDXPS_DUMMY15,\n");
		printf("EDXPS_INSTANCE_PS				=	EDXPS_BATCH_PS					+%d,\n",m_ConstBufferLSProfile[16]);
		printf("EDXPS_STATIC_INSTANCE_PS=	EDXPS_BATCH_PS					+%d,\n",m_ConstBufferLSProfile[17]);
		printf("EDXPS_FRAME_PS					=	EDXPS_STATIC_INSTANCE_PS+%d,\n",m_ConstBufferLSProfile[18]);
		printf("EDXPS_MATERIAL_PS				=	EDXPS_FRAME_PS					+%d,\n",m_ConstBufferLSProfile[19]);
		printf("EDXPS_LIGHT_PS					=	EDXPS_MATERIAL_PS				+%d,\n",m_ConstBufferLSProfile[20]);
		printf("EDXPS_SHADOWGEN_PS			=	EDXPS_LIGHT_PS					+%d,\n",m_ConstBufferLSProfile[21]);
		printf("EDXPS_SKIN_PS						=	EDXPS_SHADOWGEN_PS			+%d,\n",m_ConstBufferLSProfile[22]);
		printf("EDXPS_SHAPE_PS					=	EDXPS_SKIN_PS						+%d,\n",m_ConstBufferLSProfile[23]);
		printf("EDXPS_INSTANCE_DATA_PS	=	EDXPS_SHAPE_PS					+%d,\n",m_ConstBufferLSProfile[24]);
		printf("EDXPS_DUMMY26						=	EDXPS_INSTANCE_DATA_PS	+%d,\n",m_ConstBufferLSProfile[25]);
		printf("EDXPS_DUMMY27						=	EDXPS_DUMMY26,\n");
		printf("EDXPS_DUMMY28						=	EDXPS_DUMMY27,\n");
		printf("EDXPS_DUMMY29						=	EDXPS_DUMMY28,\n");
		printf("EDXPS_DUMMY30						=	EDXPS_DUMMY29,\n");
		printf("EDXPS_DUMMY31						=	EDXPS_DUMMY30,\n");
		printf("EDXPS_MAX								=	EDXPS_DUMMY31\n");
	}
#endif


}

namespace
{
	ILINE bool HandleDeferredZWrites(CCryDXPSGCMSyncMan& rSyncMan, CDXPSRDWorker& rWorker, const SRegisteredZWrite ZWrite)
	{
		if(ZWrite.registeredZWriteCount == PENDING_Z_COUNT)
		{
			//z-write count was pending at query time, query and stall for it now
			if(rSyncMan.ZWriteReport<true>(ZWrite.registeredZWriteCountHandle, false) == 0)
			{
				//pending z-count is 0, write some z-pixel value to the report location to signal this draw call has finished
				rSyncMan.IncDeviceThread();
#if defined(DRAWCALLDEBUGGING)
				++rWorker.DrawCall();
#endif
				//rWorker.DirtyFlags() =	0;
				CELL_GCM_FLUSH;
				CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(true);
				return true;
			}
		}
		return false;
	}

	ILINE void HandleFixPointMask(const uint32 cFixPointMask)
	{
		if(cFixPointMask)
		{
			for(uint32 a=0,b=1;a<16;a++,b<<=1)
				if(cFixPointMask&b)
					cellGcmSetTextureFilter(a,0,CELL_GCM_TEXTURE_NEAREST_NEAREST,CELL_GCM_TEXTURE_NEAREST,CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
		}
	}
}


#if defined(CRY_DXPS_SOFTWARE_TRANSFORMATIONS)
SPU_LOCAL			uint8 UB2VMTemp[4096]	_ALIGN(16);
uint8* UploadBuffer2VMem(uint8* pDataIn,uint32 Size,tdResHandle DrawCall,CCryDXPSGCMPixelshaderCacheMan& __restrict pPixelCacheMan)
{
	Size+=15;
	Size&=~15;
	uint8*	pDataOut	=	pPixelCacheMan.Alloc(Size,DrawCall);











	memcpy(pDataOut,pDataIn,Size);

	return pDataOut;
}
#endif

void CDXPSRDWorker::Job(const CDXPSRDJDrawIndexedSPU& rDrawIndexed)
{
	SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::Job_DrawIndexed")
	CCryDXPSGCMSyncMan& __restrict rSyncMan = Sync();
#if defined(CRY_DXPS_SOFTWARE_TRANSFORMATIONS)
	CCryDXPSGCMPixelshaderCacheMan& pPixelCacheMan = PSCache();
#endif
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncDeviceThread();
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(true);
	//check if this draw call should wait for a z-write count
	if(m_DeferCondRendering && HandleDeferredZWrites(rSyncMan, *this, rDrawIndexed.ZWrite()))
		return;

#if defined(CRY_DXPS_CACHESTATES)
	const uint32 cDirtyStates = m_DirtyStates;
	IF(cDirtyStates,1)
	{
		IF(cDirtyStates&EDXPSDS_RENDERTARGET,0)
			Job<false>(m_RenderTargets,m_RenderMode);
		IF(cDirtyStates&EDXPSDS_VIEWPORT,0)
			Job<false>(m_ViewPort);
		IF(cDirtyStates&EDXPSDS_POINTSPRITE,0)
			Job<false>(m_PointSpriteControl);
		IF(cDirtyStates&EDXPSDS_PASS,0)
			Job<false>(m_Pass);
		IF(cDirtyStates&EDXPSDS_BLENDFACTOR,0)
			Job<false>(m_BlendFactor);
		IF(cDirtyStates&EDXPSDS_BLENDSTATE,0)
			Job<false>(m_BlendState);
		IF(cDirtyStates&EDXPSDS_STENCIL,0)
			Job<false>(m_StencilState);
//		IF(cDirtyStates&EDXPSDS_RASTERIZER,0)
//			Job<false>(m_RasterState);
		IF(cDirtyStates&EDXPSDS_SCISSOR,0)
			Job<false>(m_ScissorState);
		IF(cDirtyStates&EDXPSDS_DEPTHBOUNDS,0)
			Job<false>(m_DepthBounds);
		m_DirtyStates	=	0;
	}
	uint16 dirtyTextures = m_DirtyTextures;
	for(uint16 a=0;__builtin_expect(dirtyTextures,0);a++,dirtyTextures>>=1)
	{
		IF(dirtyTextures&1,0)
			Job<false>(m_Texture[a],a);
	}
	m_DirtyTextures = dirtyTextures;

	uint16 dirtySamplers = m_DirtySamplers;
	for(uint16 a=0;__builtin_expect(dirtySamplers,0);a++,dirtySamplers>>=1)
	{
		IF(dirtySamplers&1,0)
			Job<false>(m_Sampler[a],a);
	}
	m_DirtySamplers = dirtySamplers;
#endif


	HandleFixPointMask(rDrawIndexed.InvalidFilteringFlag());

	const uint32 cIndexCount = rDrawIndexed.IndexCount();
	uint32_t IBIdx	=	0;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	IBIdx	=	rDrawIndexed.IBIdx();
#endif
	uint8* pIBuffer	=	(uint8*)rDrawIndexed.IndexBuffer()->RawData(IBIdx);
	uint32 IBIndex	=	rDrawIndexed.StartIndexLocation()*(4>>(rDrawIndexed.IBFormat()&1));
#if defined(CRY_DXPS_SOFTWARE_TRANSFORMATIONS)
	pIBuffer	+=	(IBIndex&~15);
	IBIndex		&=	15;
	CDXPSRDJDrawIndexedSPU DrawIndexedTmp	=	rDrawIndexed;
		pIBuffer	=	UploadBuffer2VMem(pIBuffer,cIndexCount*2+IBIndex,rSyncMan.HandleDeviceThread(),pPixelCacheMan);
#endif
	pIBuffer+=IBIndex;
	uint32_t IBOffset=0;
	IF(CELL_OK!=cellGcmAddressToOffset(pIBuffer,&IBOffset),0)
	{
		CRY_DEBUGOUT("Error in AddressToOffset \"DrawIndexed\"\n");
		//to stay in sync with the device in main-/render-thread
		snPause();
		rSyncMan.IncDeviceThread();
		return;
	}
	SyncConstBufferTransfers();
	IF(m_pVSDst,1)//will be zero if shader transfer is issued on PPU but before draw call the switch occurs to spu 
	{
		const uint32 cDirtyFlags = m_DirtyFlags;
		const CCryDXPSShader*	const __restrict pPSShader = SPU_LOCAL_PTR(PShader());
		const CCryDXPSShader*	const __restrict pVSShader = SPU_LOCAL_PTR(VShader());

		if(rDrawIndexed.BaseVertexLocation()!=m_LastBaseVL	||	(cDirtyFlags&(EDXPSDF_VERTEXLAYOUT|EDXPSDF_VERTEXBUFFER|EDXPSDF_VERTEXSHADER)))
		{
			m_LastBaseVL	=	rDrawIndexed.BaseVertexLocation();
			const uint8* pVBArray[16];
			for(uint32 a=0;a<16;a++)
				pVBArray[a]	=	m_pVertexBuffer[a]+rDrawIndexed.BaseVertexLocation()*m_VBStride[a];
			if(!m_pVSDesc)// && !m_pInputLayout->Initialized())
			{
				cellGcmCpySyncVertexCode();
				m_pVSDesc	=	pVSShader->Desc(m_pVSDst,m_pVSSrc);
			}
			m_VertexAttributeMask	=	m_pInputLayout->Set(m_pVSDesc,pVBArray,16,m_VBStride,0,m_VertexAttributeMask);
			cellGcmSetInvalidateVertexCache();
		}
		if(!m_pVSDesc)
		{
				cellGcmCpySyncVertexCode();
				m_pVSDesc	=	pVSShader->Desc(m_pVSDst,m_pVSSrc);
		}
		if(cDirtyFlags&(EDXPSDF_VERTEXSHADER|EDXPSDF_VERTEXCONSTBUFFER))
			pVSShader->SetVertexshader(m_pVSDesc,(CCryDXPSCBData* const __restrict)m_ConstBuffer,ShaderCache());
		if(cDirtyFlags&(EDXPSDF_PIXELSHADER|EDXPSDF_PIXELCONSTBUFFER))
		{
			if(!m_pPSDesc)
			{
				cellGcmSyncUCodeLS();
				m_pPSDesc	=	pPSShader->Desc(m_pPSDst,m_pPSSrc);
			}
			pPSShader->SetPixelshader(m_pPSDesc,(CCryDXPSCBData* const __restrict)&m_ConstBuffer[16],ShaderCache(),rSyncMan.HandleDeviceThread(),PSCache());
			WriteBackShader();
		}
#if defined(DRAWCALLDEBUGGING)
		m_DrawCall++;
#endif

		m_DirtyFlags	=	0;

		volatile void* volatile cpFlipTarget = g_FlipVars.GetFlipLockedTarget();
		if(cpFlipTarget)
		{
			bool Last=m_Flushing;
			uint32_t TargetOffset;
			IF(CELL_OK==cellGcmAddressToOffset((void*)cpFlipTarget,&TargetOffset),1)
				m_Flushing	&=	TargetOffset!=m_SurfaceOffsetColor;
			if(Last==true && m_Flushing==false)
			{
				SNSTOPMARKER(SNTM_FLUSHING);
				rSyncMan.SyncRSXToFrame();
			}
		}
		else
		{
#if defined(CRY_DXPS_SNMARKERS)
			if(!m_Flushing)
			{
					 SNSTARTMARKER(SNTM_FLUSHING,"go on Flushing by Drawcall");
			}
#endif
			m_Flushing = true;
		}
		cellGcmSetDrawIndexArray(	m_Topology,
															cIndexCount,
															rDrawIndexed.IBFormat(),
															CELL_GCM_LOCATION_LOCAL,
															IBOffset);
	}
	rSyncMan.IncDeviceThread();

	CELL_GCM_FLUSH;
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(true);
	SyncShaderWriteBack();
}
/*
void CDXPSRDWorker::Job(const CDXPSRDJCopySPU&	rCopy)
{
	using namespace CRY_DXPS_GCMNAMESPACE;
	CCryDXPSGCMSyncMan& __restrict rSyncMan = Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncDeviceThread();
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(true);
	if(rCopy.DstPitch()==0)
	{
		uint32 OffsetSrc,OffsetDst;
		IF(CELL_OK==cellGcmAddressToOffset(rCopy.Src(),&OffsetSrc),1)
			IF(CELL_OK==cellGcmAddressToOffset(rCopy.Dst(),&OffsetDst),1)
				cellGcmSetTransferData(CELL_GCM_TRANSFER_LOCAL_TO_LOCAL,OffsetDst,rCopy.SrcPitch(),OffsetSrc,rCopy.SrcPitch(),rCopy.SrcPitch(),1);
	}
	else
	{
		CellGcmTransferScale src;
		src.conversion	= CELL_GCM_TRANSFER_CONVERSION_TRUNCATE;
		src.format			= CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
		src.operation		= CELL_GCM_TRANSFER_OPERATION_SRCCOPY;
		src.clipX = 0;
		src.clipY = 0;
		src.clipW = rCopy.DstSizeX();
		src.clipH = rCopy.DstSizeY();
		src.outX	= 0;
		src.outY	= 0;
		src.outW	= rCopy.DstSizeX();  
		src.outH	= rCopy.DstSizeY();  
		src.inW		= rCopy.SrcSizeX();
		src.inH		= rCopy.SrcSizeY();
		src.ratioX = cellGcmGetFixedSint32((float)src.inW / (float)src.outW );
		src.ratioY = cellGcmGetFixedSint32((float)src.inW / (float)src.outW );
		src.pitch  = rCopy.SrcPitch(); 
		src.origin = CELL_GCM_TRANSFER_ORIGIN_CENTER;   
		src.interp = CELL_GCM_TRANSFER_INTERPOLATOR_ZOH; // point sampling
		cellGcmAddressToOffset(rCopy.Src(),&src.offset); // source offset
		src.inX	= cellGcmGetFixedUint16(src.clipX); 
		src.inY = cellGcmGetFixedUint16(src.clipY); 

		// set destination surface
		CellGcmTransferSurface dst;
		dst.format  = CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
		dst.pitch   = rCopy.DstPitch(); 
		cellGcmAddressToOffset(rCopy.Dst(),&dst.offset);

		// copy
		cellGcmSetTransferScaleMode(CELL_GCM_TRANSFER_LOCAL_TO_MAIN, CELL_GCM_TRANSFER_SURFACE);
		cellGcmSetTransferScaleSurface(&src, &dst);
	}
	rSyncMan.IncDeviceThread();
	CELL_GCM_FLUSH;
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(true);
}*/

void CDXPSRDWorker::Job(const CDXPSRDJCopyResourceScaledSPU&	rCopySubresourceScaled)
{
//	SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::Job_CopyResourceScaled")
	using namespace CRY_DXPS_GCMNAMESPACE;
	CCryDXPSGCMSyncMan& __restrict rSyncMan = Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncDeviceThread();
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(true);
	CellGcmTransferScale src;
	src.conversion	= CELL_GCM_TRANSFER_CONVERSION_TRUNCATE;
	src.format			= CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
	src.operation		= CELL_GCM_TRANSFER_OPERATION_SRCCOPY;
	src.clipX = rCopySubresourceScaled.SrcBox().left;
	src.clipY = rCopySubresourceScaled.SrcBox().top;
	src.clipW = rCopySubresourceScaled.SrcBox().right;
	src.clipH = rCopySubresourceScaled.SrcBox().bottom;
	src.outX	= rCopySubresourceScaled.SrcBox().left;
	src.outY	= rCopySubresourceScaled.SrcBox().top;
	src.outW	= rCopySubresourceScaled.SrcBox().right;    
	src.outH	= rCopySubresourceScaled.SrcBox().bottom;   
	src.inW		= rCopySubresourceScaled.SrcBox().right;    
	src.inH		= rCopySubresourceScaled.SrcBox().bottom;    
	src.ratioX = cellGcmGetFixedSint32((float)src.inW / (float)src.outW );
	src.ratioY = cellGcmGetFixedSint32((float)src.inW / (float)src.outW );
	src.pitch  = rCopySubresourceScaled.Src()->Pitch(0); 
	src.origin = CELL_GCM_TRANSFER_ORIGIN_CENTER;   
	src.interp = CELL_GCM_TRANSFER_INTERPOLATOR_ZOH; // point sampling
	cellGcmAddressToOffset(rCopySubresourceScaled.Src()->RawPointer(),&src.offset); // source offset
	src.inX	= cellGcmGetFixedUint16(src.clipX); 
	src.inY = cellGcmGetFixedUint16(src.clipY); 

	// set destination surface
	CellGcmTransferSurface dst;
	dst.format  = CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
	dst.pitch   = rCopySubresourceScaled.Dst()->Pitch(0); 
	cellGcmAddressToOffset(rCopySubresourceScaled.Dst()->RawPointer(),&dst.offset);

	// copy
	cellGcmSetTransferScaleMode(CELL_GCM_TRANSFER_LOCAL_TO_LOCAL, CELL_GCM_TRANSFER_SURFACE);
	cellGcmSetTransferScaleSurface(&src, &dst);
//	rSyncMan.WriteLabelEnd(ECDXPSSL_DRAWCALL,m_DrawCallCounter);
	rSyncMan.IncDeviceThread();
	CELL_GCM_FLUSH;
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(true);
}

void CDXPSRDWorker::Job(const CDXPSRDJCopyResourceSPU&	rCopySubresource)
{
	SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::Job_CopyResource")
	using namespace CRY_DXPS_GCMNAMESPACE;
	CCryDXPSGCMSyncMan& __restrict rSyncMan = Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
	rSyncMan.IncDeviceThread();
#endif
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_START(true);

	if(rCopySubresource.DstResource()->Type()==EDXPS_RT_TEX2D	&&	rCopySubresource.SrcResource()->Type()==EDXPS_RT_TEX2D)
	{
		const D3D11_BOX* pSrcBox	=	rCopySubresource.SrcBox();
		CCryDXPSTexture2D* pDst	=	(CCryDXPSTexture2D*)rCopySubresource.DstResource();
		CCryDXPSTexture2D* pSrc	=	(CCryDXPSTexture2D*)rCopySubresource.SrcResource();
		if(pSrcBox)
		{
			CRY_ASSERT_MESSAGE(pSrcBox->left == 0 && pSrcBox->top == 0 && pSrcBox->front == 0 &&
				(pSrcBox->right == pSrc->SizeX() || pSrcBox->right == pDst->SizeX())
				&& (pSrcBox->bottom == pSrc->SizeY() || pSrcBox->bottom == pDst->SizeY()) && pSrcBox->back == 1,"Invalid call!");
		}

		CellGcmTexture DstGcmTexture	=	*pDst->GcmTexture();
		CellGcmTexture SrcGcmTexture	=	*pSrc->GcmTexture();
		uint32 DstSubResource	=	rCopySubresource.DstSubresource();
		uint32 SrcSubResource	=	rCopySubresource.SrcSubresource();
		uint32 DstArrayID			=	0;
		uint32 SrcArrayID			=	0;
		IF(DstGcmTexture.cubemap==CELL_GCM_TRUE,0)
		{
			DstArrayID			=		DstSubResource/DstGcmTexture.mipmap;
			DstSubResource	%=	DstGcmTexture.mipmap;
		}
		IF(SrcGcmTexture.cubemap==CELL_GCM_TRUE,0)
		{
			SrcArrayID			=		SrcSubResource/SrcGcmTexture.mipmap;
			SrcSubResource	%=	SrcGcmTexture.mipmap;
		}

		const uint32	PitchDst	=	pDst->Pitch(DstSubResource);
		const uint32	PitchSrc	=	pSrc->Pitch(SrcSubResource);
		const uint32	LineLen		=	min(PitchSrc, PitchDst);
		const uint32	PixelSize	=	GCM_TextureFormatSize(1,DXGI_FORMAT2GCM[pSrc->Format()]);
		if((PixelSize!=2 && PixelSize!=4) || (PitchSrc&63) || (PitchDst&63))
		{
			const uint32 LinesDst	=	pDst->SizeY()>>DstSubResource;
			const uint32 LinesSrc	=	pSrc->SizeY()>>SrcSubResource;
			const uint32 Lines		=	min(LinesDst, LinesSrc);
			IF(Lines,1)
				cellGcmSetTransferData(CELL_GCM_TRANSFER_LOCAL_TO_LOCAL,
																DstGcmTexture.offset+pDst->CalcOffset(DstArrayID,DstSubResource),PitchDst,
																SrcGcmTexture.offset+pSrc->CalcOffset(SrcArrayID,SrcSubResource),PitchSrc,LineLen,Lines);
			else
			{
				snPause();
				CRY_ASSERT_MESSAGE(0, "Failed to do a copy subresource!");
			}
		}
		else
		{
			cellGcmSetTransferImage(	CELL_GCM_TRANSFER_LOCAL_TO_LOCAL,
																pDst->GcmTexture()->offset+pDst->CalcOffset(0,rCopySubresource.DstSubresource()),PitchDst,rCopySubresource.DstX(),rCopySubresource.DstY(),
																pSrc->GcmTexture()->offset+pSrc->CalcOffset(0,rCopySubresource.SrcSubresource()),PitchSrc,
																0,0,pDst->SizeX()>>rCopySubresource.DstSubresource(),pDst->SizeY()>>rCopySubresource.DstSubresource(),PixelSize);
		}
	}
	else
	if(rCopySubresource.DstResource()->Type()==EDXPS_RT_TEX3D	&&	rCopySubresource.SrcResource()->Type()==EDXPS_RT_TEX2D)
	{
		const D3D11_BOX* pSrcBox	=	rCopySubresource.SrcBox();
		CCryDXPSTexture3D* pDst	=	(CCryDXPSTexture3D*)rCopySubresource.DstResource();
		CCryDXPSTexture2D* pSrc	=	(CCryDXPSTexture2D*)rCopySubresource.SrcResource();

		CRY_ASSERT_MESSAGE(pSrc->Format() == pDst->Format(), "No format conversion supported");
		CRY_ASSERT_MESSAGE(rCopySubresource.SrcSubresource() == 0, "No source mips supported");
		CRY_ASSERT_MESSAGE(pDst->MipLevels() == 1, "No dest mips supported");
		CRY_ASSERT_MESSAGE(pSrc->GcmTexture()->format & CELL_GCM_TEXTURE_LN, "No swizzled src format supported");
		CRY_ASSERT_MESSAGE(pDst->GcmTexture()->format & CELL_GCM_TEXTURE_LN, "No swizzled dest format supported");
		const uint32 PitchSrc	=	pSrc->Pitch(0);
		const uint32 PitchDst	=	pDst->Pitch(0/*dest mip number is always 0*/);
		const uint32 PixelSize	=	GCM_TextureFormatSize(1,DXGI_FORMAT2GCM[pSrc->Format()]);
		CRY_ASSERT_MESSAGE(PixelSize == 2 || PixelSize == 4, "Unsupported format for transfer");

		const uint32 nSliceOffset = pDst->CalcOffset(rCopySubresource.DstSubresource(), 0/*dest mip number is always 0*/);

		const uint32 nSrcX = pSrcBox ? pSrcBox->left : 0;
		const uint32 nSrcY = pSrcBox ? pSrcBox->top : 0;
		const uint32 nWidth = pSrcBox ? (pSrcBox->right - pSrcBox->left) : pSrc->SizeX();
		const uint32 nHeight = pSrcBox ? (pSrcBox->bottom - pSrcBox->top) : pSrc->SizeY();

		CRY_ASSERT_MESSAGE(nSrcX + nWidth <= pSrc->SizeX() && rCopySubresource.DstX() + nWidth <= pDst->SizeX(), "Wrong X coord");
		CRY_ASSERT_MESSAGE(nSrcY + nHeight <= pSrc->SizeY() && rCopySubresource.DstY() + nHeight <= pDst->SizeY(), "Wrong Y coord");

		cellGcmSetTransferImage(	CELL_GCM_TRANSFER_LOCAL_TO_LOCAL,
															pDst->GcmTexture()->offset + nSliceOffset,PitchDst,rCopySubresource.DstX(),rCopySubresource.DstY(),
															pSrc->GcmTexture()->offset,PitchSrc,
															nSrcX,nSrcY,nWidth,nHeight,PixelSize);
	}
	else
	IF((rCopySubresource.DstResource()->Type()==EDXPS_RT_VERTEXBUFFER || rCopySubresource.DstResource()->Type()==EDXPS_RT_INDEXBUFFER),1)
	{
		CCryDXPSBuffer* pDst	=	(CCryDXPSBuffer*)rCopySubresource.DstResource();
		CCryDXPSBuffer* pSrc	=	(CCryDXPSBuffer*)rCopySubresource.SrcResource();
		const D3D11_BOX* pSrcBox	=	rCopySubresource.SrcBox();
		const int SrcOffset	=	pSrcBox?pSrcBox->left:0;
		const int SrcSize		=	(pSrcBox?pSrcBox->right:pSrc->Size())-SrcOffset;
		const int	DstOffset	=	rCopySubresource.DstX();
		const int	DstSize		=	pDst->Size()-DstOffset;
		const int TransSize	=	DstSize<SrcSize?DstSize:SrcSize;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
		const uint32 SrcVBIdx	=	rCopySubresource.SrcVBIdx();
		const uint32 DstVBIdx	=	rCopySubresource.DstVBIdx();
#else
		const uint32 SrcVBIdx	=	0;
		const uint32 DstVBIdx	=	0;
#endif
		const uint8*	pSrcData	=	&(pSrc->RawData(SrcVBIdx)[SrcOffset]);
		uint8*	pDstData	=	&(pDst->RawData(DstVBIdx)[DstOffset]);
		uint32 OffsetSrc,OffsetDst;
#if defined(CRY_DXPS_SOFTWARE_TRANSFORMATIONS)
		if(rCopySubresource.DstResource()->Type()==EDXPS_RT_INDEXBUFFER)
		{
			memcpy(pDstData,pSrcData,TransSize);
		}
		else
#endif
		{
			IF(	CELL_OK==cellGcmAddressToOffset(pSrcData,&OffsetSrc) &&
					CELL_OK==cellGcmAddressToOffset(pDstData,&OffsetDst),1)
			{
					cellGcmSetTransferData(CELL_GCM_TRANSFER_LOCAL_TO_LOCAL,OffsetDst,TransSize,OffsetSrc,TransSize,TransSize,1);
			}
			else
			{
				CRY_ASSERT_MESSAGE(0,"failed on AddressToOffset for VMemCopy!");
				snPause();
			}
		}
	}
	else
	{
		CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
		//to stay in sync with the device in main-/render-thread
		snPause();
	}
	rSyncMan.IncDeviceThread();
	CELL_GCM_FLUSH;
	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(true);
}






//void CDXPSRDWorker::Job(const CDXPSRDJClearTextureSPU& rClearTexture)
//{
//	CCryDXPSGCMSyncMan&	rSyncMan = tdLayer0::Sync();
//#if defined(CRY_DXPS_PERFORMANCECOUNTING)
//	rSyncMan.IncDeviceThread();
//#endif
//	CCryDXPSTexture2D* const __restrict pTex	=	(CCryDXPSTexture2D*)rClearTexture.Tex();
//	const uint32 Size	=	GCM_TextureFormatSize(pTex->SizeX()*pTex->SizeY(),pTex->GcmTexture()->format);
//	const uint32 cClearVal = rClearTexture.Color();
//	pTex->Sync();
//	memset_large_nocache_128((void*)pTex->RawPointer(), cClearVal, Size);
//	rSyncMan.HandlePointerDeviceThread()->value = Size >> 2;
//	rSyncMan.IncDeviceThread();
//	CRY_DXPS_SINGLEFLUSHVALIDATE_COUNTER_SYNC_END(true);
//}


#endif//__SPU__ || !_RELEASE
