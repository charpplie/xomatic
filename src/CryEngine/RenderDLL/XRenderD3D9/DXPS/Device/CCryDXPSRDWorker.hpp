#ifndef __CRYDXPSRDWORKER__
#define __CRYDXPSRDWORKER__

#include "CryThread.h"
#include "CCryDXPSRDState.hpp"
#include "Jobs/CCryDXPSRDJob.hpp"
#include "../Layer0/GCM/CCryDXPSGCM_SyncMan.hpp"

#include "Jobs/CCryDXPSRDJUpdateState.hpp"

#define BITSET(X)	(1<<X)

enum	EDXPS_DirtyFlags
{
	EDXPSDF_VERTEXSHADER				=	BITSET(0),
	EDXPSDF_PIXELSHADER					=	BITSET(1),
	EDXPSDF_VERTEXCONSTBUFFER		=	BITSET(2),
	EDXPSDF_PIXELCONSTBUFFER		=	BITSET(3),
	EDXPSDF_VERTEXBUFFER				=	BITSET(4),
	EDXPSDF_TEXTURE							=	BITSET(5),
	EDXPSDF_VERTEXLAYOUT				=	BITSET(6),
	EDXPSDF_INDEXBUFFER					=	BITSET(7),
	EDXPSDF_SAMPLERF32FIX				=	BITSET(8),
	EDXPSDF_BLENDSTATE					=	BITSET(9),
	EDXPSDF_BLENDFACTOR					=	BITSET(10),
	EDXPSDF_DEPTHSTENCILSTATE		=	BITSET(11),
	EDXPSDF_RASTERIZERSTATE			=	BITSET(12),
	EDXPSDF_SCISSOR							=	BITSET(13),
	EDXPSDF_DEPTHBOUNDS					=	BITSET(14),
	EDXPSDF_PASS								=	BITSET(15)
};

enum EDXPS_DirtyStates
{
	EDXPSDS_RENDERTARGET			=	BITSET(0),
	EDXPSDS_VIEWPORT					=	BITSET(1),
	EDXPSDS_POINTSPRITE				=	BITSET(2),
	EDXPSDS_PASS							=	BITSET(3),
	EDXPSDS_BLENDFACTOR				=	BITSET(4),
	EDXPSDS_BLENDSTATE				=	BITSET(5),
	EDXPSDS_STENCIL						=	BITSET(6),
	EDXPSDS_RASTERIZER				=	BITSET(7),
	EDXPSDS_SCISSOR						=	BITSET(8),
	EDXPSDS_DEPTHBOUNDS				=	BITSET(9)
};

#if defined(CRY_DXPS_LSCONSTANTS)
#if defined(CRY_DXPS_LSCONSTANTS_PROFILE)
#define EDXPS_CB_VS		(328*16)
#define	EDXPS_CB_PS		(328*16)

//full size buffers
enum EDXPS_ConstBufferOffset
{
	EDXPS_BATCH_VS					=	0,
	EDXPS_INSTANCE_VS				=	EDXPS_BATCH_VS					+EDXPS_CB_VS,
	EDXPS_STATIC_INSTANCE_VS=	EDXPS_INSTANCE_VS				+EDXPS_CB_VS,
	EDXPS_FRAME_VS					=	EDXPS_STATIC_INSTANCE_VS+EDXPS_CB_VS,
	EDXPS_MATERIAL_VS				=	EDXPS_FRAME_VS					+EDXPS_CB_VS,
	EDXPS_LIGHT_VS					=	EDXPS_MATERIAL_VS				+EDXPS_CB_VS,
	EDXPS_SHADOWGEN_VS			=	EDXPS_LIGHT_VS					+EDXPS_CB_VS,
	EDXPS_SKIN_VS						=	EDXPS_SHADOWGEN_VS			+EDXPS_CB_VS,
	EDXPS_SHAPE_VS					=	EDXPS_SKIN_VS						+EDXPS_CB_VS,
	EDXPS_INSTANCE_DATA_VS	=	EDXPS_SHAPE_VS					+EDXPS_CB_VS,
	EDXPS_DUMMY10						=	EDXPS_INSTANCE_DATA_VS	+EDXPS_CB_VS,
	EDXPS_DUMMY11						=	EDXPS_DUMMY10,
	EDXPS_DUMMY12						=	EDXPS_DUMMY11,
	EDXPS_DUMMY13						=	EDXPS_DUMMY12,
	EDXPS_DUMMY14						=	EDXPS_DUMMY13,
	EDXPS_DUMMY15						=	EDXPS_DUMMY14,
	EDXPS_BATCH_PS					=	EDXPS_DUMMY15,
	EDXPS_INSTANCE_PS				=	EDXPS_BATCH_PS					+EDXPS_CB_PS,
	EDXPS_STATIC_INSTANCE_PS=	EDXPS_INSTANCE_PS				+EDXPS_CB_PS,
	EDXPS_FRAME_PS					=	EDXPS_STATIC_INSTANCE_PS+EDXPS_CB_PS,
	EDXPS_MATERIAL_PS				=	EDXPS_FRAME_PS					+EDXPS_CB_PS,
	EDXPS_LIGHT_PS					=	EDXPS_MATERIAL_PS				+EDXPS_CB_PS,
	EDXPS_SHADOWGEN_PS			=	EDXPS_LIGHT_PS					+EDXPS_CB_PS,
	EDXPS_SKIN_PS						=	EDXPS_SHADOWGEN_PS			+EDXPS_CB_PS,
	EDXPS_SHAPE_PS					=	EDXPS_SKIN_PS						+EDXPS_CB_PS,
	EDXPS_INSTANCE_DATA_PS	=	EDXPS_SHAPE_PS					+EDXPS_CB_PS,
	EDXPS_DUMMY26						=	EDXPS_INSTANCE_DATA_PS	+EDXPS_CB_PS,
	EDXPS_DUMMY27						=	EDXPS_DUMMY26,
	EDXPS_DUMMY28						=	EDXPS_DUMMY27,
	EDXPS_DUMMY29						=	EDXPS_DUMMY28,
	EDXPS_DUMMY30						=	EDXPS_DUMMY29,
	EDXPS_DUMMY31						=	EDXPS_DUMMY30,
	EDXPS_MAX								=	EDXPS_DUMMY31
};
#else
//1kb per cbslot as dummy
#define EDXPS_CB_VS		(1024)
#define	EDXPS_CB_PS		(1024)
//adapted by hand based on profiling data
enum EDXPS_ConstBufferOffset
{
	EDXPS_BATCH_VS					=	0,
	EDXPS_INSTANCE_VS				=	EDXPS_BATCH_VS					+EDXPS_CB_VS+5232,
	EDXPS_STATIC_INSTANCE_VS=	EDXPS_INSTANCE_VS				+EDXPS_CB_VS+5232,
	EDXPS_FRAME_VS					=	EDXPS_STATIC_INSTANCE_VS+EDXPS_CB_VS+96,
	EDXPS_MATERIAL_VS				=	EDXPS_FRAME_VS					+EDXPS_CB_VS+640,
	EDXPS_LIGHT_VS					=	EDXPS_MATERIAL_VS				+EDXPS_CB_VS+512,
	EDXPS_SHADOWGEN_VS			=	EDXPS_LIGHT_VS					+EDXPS_CB_VS+64,
	EDXPS_SKIN_VS						=	EDXPS_SHADOWGEN_VS			+EDXPS_CB_VS+576,
	EDXPS_SHAPE_VS					=	EDXPS_SKIN_VS						+EDXPS_CB_VS+2656,
	EDXPS_INSTANCE_DATA_VS	=	EDXPS_SHAPE_VS					+EDXPS_CB_VS+0,
	EDXPS_DUMMY10						=	EDXPS_INSTANCE_DATA_VS	+EDXPS_CB_VS+0,
	EDXPS_DUMMY11						=	EDXPS_DUMMY10,
	EDXPS_DUMMY12						=	EDXPS_DUMMY11,
	EDXPS_DUMMY13						=	EDXPS_DUMMY12,
	EDXPS_DUMMY14						=	EDXPS_DUMMY13,
	EDXPS_DUMMY15						=	EDXPS_DUMMY14,
	EDXPS_BATCH_PS					=	EDXPS_DUMMY15,
	EDXPS_INSTANCE_PS				=	EDXPS_BATCH_PS					+EDXPS_CB_PS+384,
	EDXPS_STATIC_INSTANCE_PS=	EDXPS_INSTANCE_PS				+EDXPS_CB_PS+448,
	EDXPS_FRAME_PS					=	EDXPS_STATIC_INSTANCE_PS+EDXPS_CB_PS+0,
	EDXPS_MATERIAL_PS				=	EDXPS_FRAME_PS					+EDXPS_CB_PS+3584,
	EDXPS_LIGHT_PS					=	EDXPS_MATERIAL_PS				+EDXPS_CB_PS+512,
	EDXPS_SHADOWGEN_PS			=	EDXPS_LIGHT_PS					+EDXPS_CB_PS+192,
	EDXPS_SKIN_PS						=	EDXPS_SHADOWGEN_PS			+EDXPS_CB_PS+0,
	EDXPS_SHAPE_PS					=	EDXPS_SKIN_PS						+EDXPS_CB_PS+0,
	EDXPS_INSTANCE_DATA_PS	=	EDXPS_SHAPE_PS					+EDXPS_CB_PS+0,
	EDXPS_DUMMY26						=	EDXPS_INSTANCE_DATA_PS	+EDXPS_CB_PS+0,
	EDXPS_DUMMY27						=	EDXPS_DUMMY26,
	EDXPS_DUMMY28						=	EDXPS_DUMMY27,
	EDXPS_DUMMY29						=	EDXPS_DUMMY28,
	EDXPS_DUMMY30						=	EDXPS_DUMMY29,
	EDXPS_DUMMY31						=	EDXPS_DUMMY30,
	EDXPS_MAX								=	EDXPS_DUMMY31
};
#endif
#endif

#if defined(CRY_DXPS_CACHESTATES)
#define CRY_DXPS_USESTATECACHE true
#else
#define CRY_DXPS_USESTATECACHE false
#endif

#define SHADER_TRANSFER_TAG (12)
#define CB_BUF_TRANSFER_TAG (11)

//global vars controlling frame flip
//designed for spu to always access a consistent state 
//  get and write cacheline atomically on SPU
//struct is padded top avoid useless additional atomic ops(looping 
//  due to altered cacheline)
//if any variable is added, update implementation of PutFlipVars and adapt pad
struct SFlipVars
{
	volatile tdResHandle	flipDrawCallID;		//pay attention: 8 bytes
	volatile void*				pFlipLockedTarget;
	volatile int32_t			flipFrameID;
	volatile int					flipIDCur;
	volatile int					flipBufID;
	volatile int					flipModeUsed;
	int										pad;							//pad for next 8 byte alignment

	//shadow each variable
	tdResHandle						flipDrawCallIDShadow;
	void*									pFlipLockedTargetShadow;
	int32_t								flipFrameIDShadow;
	int										flipIDCurShadow;
	int										flipBufIDShadow;
	int										flipModeUsedShadow;
	int										pad1;							//pad for next 8 byte alignment

	tdResHandle						flipDrawCallIDSetup;//interrupt thread need to set it after calling cellGcmSetPrepareFlip

	uint8									pad2[(128-32-8)/2];		//fill rest of cache line
	
	SFlipVars() : flipIDCur(0), pFlipLockedTarget(NULL), flipDrawCallID(0){}
	volatile void* GetFlipLockedTarget() const;
	void FetchFlipVars(SFlipVars& rDest) const;
	void PutFlipVars(const SFlipVars& crSrc);
} _ALIGN(128);
extern SFlipVars g_FlipVars;

class CDXPSRDWorker
{
	CCryDXPShaderCache															m_ShaderCache;
	const CCryDXPSShader*														m_pVShader;
	const CCryDXPSShader*														m_pPShader;
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	uint8																						m_VShader[sizeof(CCryDXPSShader)] _ALIGN(128);
	uint8																						m_PShader[sizeof(CCryDXPSShader)] _ALIGN(128);
#else
	CCryDXPSShader																	m_VShader;
	CCryDXPSShader																	m_PShader;
#endif
	mutable bool																		m_ShaderTransferInFlight;
	mutable bool																		m_ShaderBackTransferInFlight;
	SPU_DOMAIN_LOCAL const CDXPSShaderDesc*					m_pVSDesc;
	SPU_DOMAIN_LOCAL const CDXPSShaderDesc*					m_pPSDesc;
	SPU_DOMAIN_LOCAL uint8*													m_pPSSrc;
	SPU_DOMAIN_LOCAL uint8*													m_pPSDst;
	SPU_DOMAIN_LOCAL uint8*													m_pVSSrc;
	SPU_DOMAIN_LOCAL uint8*													m_pVSDst;
	SPU_DOMAIN_MAIN CCryDXPSGCMSyncMan* __restrict	m_pSyncMan;
	CCryDXPSGCMPixelshaderCacheMan									m_PSCache;

	CCryDXPSCBData																	m_ConstBuffer[32];
#if defined(CRY_DXPS_LSCONSTANTS_PROFILE)
	uint32																					m_ConstBufferLSProfile[32];
#endif
	uint8*																					m_pVertexBuffer[16];//max 16 input slots
	uint32																					m_VBStride[16];
	const CCryDXPSInputLayout*											m_pInputLayout;
	uint32																					m_SurfaceOffsetColor;
	uint32																					m_SurfaceOffsetDepth;
	uint32																					m_DirtyFlags;
	uint32																					m_VertexAttributeMask;
	bool																						m_Flushing;
	bool																						m_Using704;
#if defined(DRAWCALLDEBUGGING)										
	uint32																					m_DrawCall;
#endif

	uint32																					m_Topology;
	uint32																					m_DeferCondRendering;
	int*																						m_pCondRenderingVar;
	uint32																					m_ColorMask;
	uint32																					m_LastBaseVL;

	uint32																					m_DirtyStates;
	uint16																					m_DirtyTextures;
	uint16																					m_DirtySamplers;
	CDXPSRDJRenderTargetSPU													m_RenderTargets;
	uint32																					m_RenderMode;
	CDXPSRDJViewPortSPU															m_ViewPort;
	CDXPSRDJPointSpriteSPU													m_PointSpriteControl;
	CDXPSRDJUpdateStatePassSPU											m_Pass;
	CDXPSRDJUpdateStateBlendFactorSPU								m_BlendFactor;
	CDXPSRDJUpdateStateBlendSPU											m_BlendState;
	CDXPSRDJUpdateStateStencilSPU										m_StencilState;
	CDXPSRDJUpdateStateRasterizerSPU								m_RasterState;
	CDXPSRDJUpdateStateScissorSPU										m_ScissorState;
	CDXPSRDJUpdateStateDepthBoundsSPU								m_DepthBounds;
	CDXPSRDJUpdateStateTextureSPU										m_Texture[16];
	CDXPSRDJUpdateStateSamplerSPU										m_Sampler[16];
//	const CCryDXPSBuffer*														m_pConstBufferDummy[32];
	bool																						m_ConstBufferTransferActive;
	uint32_t																				m_GammaOutEnable;

#if defined(CRY_DXPS_THREAD_DEBUGDATA)
	uint32											m_DebugData[CRY_DXPS_THREAD_DEBUGDATA];
#endif

	ILINE	VOID									ResetColorMask()
															{
																	m_ColorMask	=	
																		CELL_GCM_COLOR_MASK_R|
																		CELL_GCM_COLOR_MASK_G|
																		CELL_GCM_COLOR_MASK_B|
																		CELL_GCM_COLOR_MASK_A;
															}

#if !defined(_RELEASE) || defined(__SPU__)
	void												DownloadDepthBuffer(const uint32 BufferID,const float* pViewMat);

	void												Job(const class CDXPSRDJDrawIndexedSPU& rDrawIndexed);

	template<bool CACHE>
	void												Job(const class CDXPSRDJRenderTargetSPU& rRenderTargets,uint32 RenderMode) 
															{
//																SPU_FRAME_PROFILE_SECTION("CDXPSRDWorker::Job_RenderTarget")
																if(CACHE)
																{
																	m_DirtyStates				|=	EDXPSDS_RENDERTARGET;
																	m_RenderTargets			=	rRenderTargets;
																	m_RenderMode				=	RenderMode;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;

																APWeakTexture2D	pRenderTargetViews[4]={	rRenderTargets.RenderTargetViews0(),
																																				rRenderTargets.RenderTargetViews1(),
																																				rRenderTargets.RenderTargetViews2(),
																																				rRenderTargets.RenderTargetViews3()};

																APWeakTexture2D pDepthStencilView	=	rRenderTargets.DepthStencilView();

																CellGcmSurface Surface;
																//basic initialisation
																Surface.colorLocation[0]	= 
																Surface.colorLocation[1]	= 
																Surface.colorLocation[2]	= 
																Surface.colorLocation[3]	= CELL_GCM_LOCATION_LOCAL;
																Surface.colorOffset[0]	=
																Surface.colorOffset[1]	=
																Surface.colorOffset[2]	=
																Surface.colorOffset[3]	=	0;
																Surface.colorPitch[0]	=
																Surface.colorPitch[1]	=
																Surface.colorPitch[2]	=
																Surface.colorPitch[3]	= 64;//needs to be at least 64 regarding to the rsx doc
																Surface.colorFormat		=	CELL_GCM_SURFACE_A8R8G8B8;
																Surface.colorTarget		=	CELL_GCM_SURFACE_TARGET_NONE;

																uint32 a=0;
																for(;pRenderTargetViews[a] && a<4;a++)
																{
																	Surface.colorFormat 	= GCM_TextureFormat2SurfaceFormat(DXGI_FORMAT2GCM[pRenderTargetViews[a]->Format()]);
																	Surface.colorLocation[a]	= CELL_GCM_LOCATION_LOCAL;
																	SPU_DOMAIN_MAIN uint8* pRawData	=	pRenderTargetViews[a]->RawPointer();
																	if(CELL_OK!=cellGcmAddressToOffset(pRawData,&Surface.colorOffset[a]))
																	{
																		CRY_DEBUGOUT("Setting View failed on AddressToOffset of View %d\nppRenderTargetView: %x\nColorOffset %d\n",a,(unsigned int)pRenderTargetViews[a]->RawPointer(),(int)Surface.colorOffset[a]);
																	}
																	Surface.colorPitch[a] 		= pRenderTargetViews[a]->Pitch(0);
																}
																switch(a)
																{
																case 0:
																	break;
																case 1:
																	Surface.colorTarget	= CELL_GCM_SURFACE_TARGET_0;
																	break;
																case 2:
																	Surface.colorTarget	= CELL_GCM_SURFACE_TARGET_MRT1;
																	Surface.colorOffset[2] = Surface.colorOffset[3] = Surface.colorOffset[0];
																	break;
																case 3:
																	Surface.colorTarget	= CELL_GCM_SURFACE_TARGET_MRT2;
																	Surface.colorOffset[3] = Surface.colorOffset[0];
																	break;
																case 4:
																	Surface.colorTarget	= CELL_GCM_SURFACE_TARGET_MRT3;
																	break;
																default:
																	{
																		CRY_DEBUGOUT("Too many render targets! Maximum is 4.");
																		break;
																	}
																}

																if(pDepthStencilView)
																{
																	DXGI_FORMAT DepthFormat	=	pDepthStencilView->Format();
																	if(DepthFormat==DXGI_FORMAT_R8G8B8A8_UINT)
																		DepthFormat	=	DXGI_FORMAT_D24_UNORM_S8_UINT;
																	Surface.depthFormat 	= GCM_TextureFormat2SurfaceFormat(DXGI_FORMAT2GCM[DepthFormat]);
																	Surface.depthLocation	= CELL_GCM_LOCATION_LOCAL;
																	if(CELL_OK!=cellGcmAddressToOffset(pDepthStencilView->RawPointer(),&Surface.depthOffset))
																	{
																		CRY_DEBUGOUT("Setting depthstencil failed on AddressToOffset \nppDepthStencilView: %x\nColorOffset %d\n",(unsigned int)pDepthStencilView->RawPointer(),(int)Surface.colorOffset);
																	}
																	Surface.depthPitch 	= pDepthStencilView->Pitch(0);
																}
																else
																{
																	Surface.depthOffset	= 0;
																	Surface.depthPitch 	= 64;
																	Surface.depthFormat 	= CELL_GCM_SURFACE_Z24S8;
																	Surface.depthLocation	= CELL_GCM_LOCATION_LOCAL;
																}
																m_SurfaceOffsetDepth	=	Surface.depthOffset;

																Surface.type		= CELL_GCM_SURFACE_PITCH;
																Surface.antialias	= CELL_GCM_SURFACE_CENTER_1;

																Surface.x 			= 0;
																Surface.y 			= 0;
																Surface.width		=	1;
																Surface.height	=	1;
																if(pRenderTargetViews[0])
																{
																	Surface.type		= (pRenderTargetViews[0]->GcmTexture()->format&CELL_GCM_TEXTURE_LN)?CELL_GCM_SURFACE_PITCH:CELL_GCM_SURFACE_SWIZZLE;
																	Surface.width 	= pRenderTargetViews[0]->SizeX();
																	Surface.height	= pRenderTargetViews[0]->SizeY();
																}
																else
																if(pDepthStencilView)
																{
																	Surface.width		=	pDepthStencilView->SizeX();
																	Surface.height	=	pDepthStencilView->SizeY();
																	Surface.type		= (pDepthStencilView->GcmTexture()->format&CELL_GCM_TEXTURE_LN)?CELL_GCM_SURFACE_PITCH:CELL_GCM_SURFACE_SWIZZLE;
																}

																STATIC_CHECK(sizeof(CellGcmSurface) == 60, CELL_GCM_SURFACE_HAS_CHANGED);

																if(RenderMode)
																	Surface.width/=2;

																if(RenderMode==2)
																	Surface.height/=2;

																if(RenderMode==1)
																	Surface.antialias			=	CELL_GCM_SURFACE_DIAGONAL_CENTERED_2;
																else
																if(RenderMode==2)
																	Surface.antialias			=	CELL_GCM_SURFACE_SQUARE_CENTERED_4;


																cellGcmSetSurfaceWindow(&Surface,CELL_GCM_WINDOW_ORIGIN_TOP,CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER);
																m_SurfaceOffsetColor	=	Surface.colorOffset[0];
															#if !defined(__SPU__) && defined(CRY_USE_GCM_REPLAY)
																if(pSurface->colorFormat == CELL_GCM_SURFACE_A8R8G8B8)
																	cell::Gcm::Replay::Capture::NotifyCaptureSurface(gCellGcmCurrentContext, rRenderTargets.Surfaces());
															#endif
															/*	cellGcmSetColorMaskMrt(	CELL_GCM_COLOR_MASK_MRT1_A|CELL_GCM_COLOR_MASK_MRT1_R|CELL_GCM_COLOR_MASK_MRT1_G|CELL_GCM_COLOR_MASK_MRT1_B|
																												CELL_GCM_COLOR_MASK_MRT2_A|CELL_GCM_COLOR_MASK_MRT2_R|CELL_GCM_COLOR_MASK_MRT2_G|CELL_GCM_COLOR_MASK_MRT2_B|
																												CELL_GCM_COLOR_MASK_MRT3_A|CELL_GCM_COLOR_MASK_MRT3_R|CELL_GCM_COLOR_MASK_MRT3_G|CELL_GCM_COLOR_MASK_MRT3_B);
															*/
																cellGcmSetColorMask(Surface.colorTarget==CELL_GCM_SURFACE_TARGET_NONE?0:m_ColorMask);
																cellGcmSetAntiAliasingControl((RenderMode&3)!=0,RenderMode>=3,CELL_GCM_FALSE,0xffff);
															}
	template<bool CACHE>
	void												Job(const class CDXPSRDJViewPortSPU& rViewPort)
															{
																if(CACHE)
																{
																	m_DirtyStates				|=	EDXPSDS_VIEWPORT;
																	m_ViewPort	=	rViewPort;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																	const D3D11_VIEWPORT&	rVW	=	rViewPort.ViewPort();
																	float Scale[4],Offset[4];
																	Scale[0] = rVW.Width * 0.5f;
																	Scale[1] = rVW.Height * -0.5f;
																	Scale[2] = rVW.MaxDepth - rVW.MinDepth;
																	Scale[3] = 0.0f;
																	Offset[0] = rVW.TopLeftX + Scale[0];
																	Offset[1] = rVW.TopLeftY - Scale[1];
																	Offset[2] = rVW.MinDepth;
																	Offset[3] = 0.0f;
																	cellGcmSetViewport(	rVW.TopLeftX,
																											rVW.TopLeftY,
																											rVW.Width,
																											rVW.Height, 
																											rVW.MinDepth, 
																											rVW.MaxDepth, 
																											Scale,
																											Offset);
																	m_Using704	|=	rVW.Height==704;
															}

	void												Job(const class CDXPSRDJClearColorSPU& rClearColor)
															{
																using namespace CRY_DXPS_GCMNAMESPACE;

#if defined(CRY_DXPS_CACHESTATES)
																const uint32 cDirtyStates = m_DirtyStates;
																IF(cDirtyStates,1)
																{
																	IF(cDirtyStates&EDXPSDS_RENDERTARGET,0)
																		Job<false>(m_RenderTargets,m_RenderMode);
																	IF(cDirtyStates&EDXPSDS_VIEWPORT,0)
																		Job<false>(m_ViewPort);
																	m_DirtyStates	&=~(EDXPSDS_RENDERTARGET|EDXPSDS_VIEWPORT);
																}
#endif

																volatile void* volatile cpFlipTarget = g_FlipVars.GetFlipLockedTarget();
																uint32_t TargetOffset=~0;
																if(cpFlipTarget)
																{
																	bool Last=m_Flushing;
																	IF(CELL_OK==cellGcmAddressToOffset((void*)cpFlipTarget,&TargetOffset),1)
																		m_Flushing	&=	TargetOffset!=m_SurfaceOffsetColor;
																	if(Last==true && m_Flushing==false)
																	{
																		SNSTOPMARKER(SNTM_FLUSHING);
																		CCryDXPSGCMSyncMan& __restrict rSyncMan = Sync();
																		rSyncMan.SyncRSXToFrame();
																	}
																}
																cellGcmSetClearColor(rClearColor.Color());
																	cellGcmSetClearSurface(CELL_GCM_CLEAR_R|CELL_GCM_CLEAR_G|CELL_GCM_CLEAR_B|CELL_GCM_CLEAR_A);
															}
//	void												Job(const class CDXPSRDJClearTextureSPU& rClearTexture);

	void												Job(const class CDXPSRDJClearDepthStencilSPU& rClearDepthStencil)
															{
																using namespace CRY_DXPS_GCMNAMESPACE;

#if defined(CRY_DXPS_CACHESTATES)
																const uint32 cDirtyStates = m_DirtyStates;
																IF(cDirtyStates,1)
																{
																	IF(cDirtyStates&EDXPSDS_RENDERTARGET,0)
																		Job<false>(m_RenderTargets,m_RenderMode);
																	IF(cDirtyStates&EDXPSDS_VIEWPORT,0)
																		Job<false>(m_ViewPort);
																	m_DirtyStates	&=~(EDXPSDS_RENDERTARGET|EDXPSDS_VIEWPORT);
																}
#endif

																//	tdLayer0::ZCull().RecalcHeuristic();
																//	int32 ClearValue=static_cast<int32>(rClearDepthStencil.Depth()*static_cast<float>(1<<24));
																//	ClearValue	=	((ClearValue>0xffffff?0xffffff:ClearValue<0?0:ClearValue)<<8)|rClearDepthStencil.Stencil();
																//	cellGcmSetClearDepthStencil(ClearValue);
																//make sure color write is enabled to all channels for speed
																cellGcmSetColorMask(CELL_GCM_COLOR_MASK_R|CELL_GCM_COLOR_MASK_G|CELL_GCM_COLOR_MASK_B|CELL_GCM_COLOR_MASK_A);
																cellGcmSetClearDepthStencil(rClearDepthStencil.ClearValue());
																uint32 Mask=(rClearDepthStencil.ClearFlags()&(0x01L))?CELL_GCM_CLEAR_Z:0;
																if(rClearDepthStencil.ClearFlags()&(0x02L))//D3D11_CLEAR_STENCIL)
																	Mask	|=	CELL_GCM_CLEAR_S;
																if(Mask)
																	cellGcmSetClearSurface(Mask);
																else
																if(rClearDepthStencil.ClearFlags()&(0x04L))//D3D11_CLEAR_ZCULL
																	cellGcmSetClearZcullSurface(CELL_GCM_TRUE,CELL_GCM_FALSE);

																// clear zcull report values
															//	cellGcmSetClearReport(CELL_GCM_ZCULL_STATS);
															//	cellGcmSetZcullStatsEnable(CELL_GCM_TRUE);
															}
	template<bool CACHE>
	void												Job(const class CDXPSRDJPointSpriteSPU&	rPointSpriteControl)
															{
																if(CACHE)
																{
																	m_DirtyStates					|=	EDXPSDS_POINTSPRITE;
																	m_PointSpriteControl	=	rPointSpriteControl;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																cellGcmSetPointSpriteControl(rPointSpriteControl.Enable(), rPointSpriteControl.RMode(), rPointSpriteControl.TexCoord());
															}
//	void												Job(const class CDXPSRDJCopySPU&	rCopy);

	void												Job(const class CDXPSRDJCopyResourceSPU&	rCopySubresource);
	void												Job(const class CDXPSRDJCopyResourceScaledSPU&	rCopySubresourceScaled);
	void												Job(const class CDXPSRDJSwapSPU& rSwap); 
	void												Job(const CDXPSRDJUpdateStateConstBufferSPU& rCBuffer,uint32 Idx);

	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateTextureSPU& rTexture,uint32 Idx)
															{
																if(CACHE)
																{
																	m_DirtyTextures	|=	1<<Idx;	//slow
																	m_Texture[Idx]	=	rTexture;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																assert(Idx<16);
																cellGcmSetTexture(Idx,rTexture.Texture()->GcmTexture());
																m_DirtyFlags|=EDXPSDF_TEXTURE;
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateSamplerSPU& rSampler,uint32 Idx)
															{
																if(CACHE)
																{
																	m_DirtySamplers	|=	1<<Idx;	//slow
																	m_Sampler[Idx]	=	rSampler;
																	return;
																}
																assert(Idx<16);
																rSampler.Sampler()->Set(Idx,rSampler.MinLOD(),rSampler.MaxLOD(),rSampler.LODBias(),rSampler.sRGB());
															}

	void												Job(const CDXPSRDJUpdateStateShaderSPU& rShader,uint32 Idx);

	void												Job(const CDXPSRDJUpdateStateVertexBufferSPU& rVBuffer,uint32 Idx)
															{
																assert(Idx<16);
																m_pVertexBuffer[Idx]	=		rVBuffer.Buffer();
																m_VBStride[Idx]				=		rVBuffer.Stride();
																m_DirtyFlags					|=	EDXPSDF_VERTEXBUFFER;
															}

	void												Job(const CDXPSRDJUpdateStateInputLayoutSPU& rInputLayout)
															{
																m_pInputLayout	=	rInputLayout.InputLayout();
																m_DirtyFlags		|=	EDXPSDF_VERTEXLAYOUT;
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateBlendFactorSPU& rBlendFactor)
															{
																if(CACHE)
																{
																	m_DirtyStates		|=	EDXPSDS_BLENDFACTOR;
																	m_BlendFactor		=	rBlendFactor;
																	return;
																}

																using namespace CRY_DXPS_GCMNAMESPACE;
																cellGcmSetBlendColor(rBlendFactor.BlendFactor(),0);
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateBlendSPU& rBlendState)
															{
																if(CACHE)
																{
																	m_DirtyStates		|=	EDXPSDS_BLENDSTATE;
																	m_BlendState		=	rBlendState;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																if(!rBlendState.BlendState())
																{
																	cellGcmSetBlendEnable(CELL_GCM_FALSE);
																	m_ColorMask	=	0;
																}
																else
																	m_ColorMask	=	rBlendState.BlendState()->Set();
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateStencilSPU& rDSState)
															{
																if(CACHE)
																{
																	m_DirtyStates		|=	EDXPSDS_STENCIL;
																	m_StencilState	=	rDSState;
																	return;
																}
																rDSState.DepthStencilState()->Set(rDSState.StencilRef());
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateRasterizerSPU& rDRState)
															{
//																if(CACHE)
//																{
//																	m_DirtyStates		|=	EDXPSDS_RASTERIZER;
//																	m_RasterState		=	rDRState;
//																	return;
//																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																if(rDRState.RasterizerState())
																	rDRState.RasterizerState()->Set(rDRState.AlphaTestRef());
                                else
																{
																	m_GammaOutEnable	=	rDRState.sRGB()?CELL_GCM_TRUE:CELL_GCM_FALSE;
  																cellGcmSetFragmentProgramGammaEnable(m_GammaOutEnable);
																}
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateScissorSPU& rDSState)
															{
																if(CACHE)
																{
																	m_DirtyStates		|=	EDXPSDS_SCISSOR;
																	m_ScissorState				=	rDSState;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																cellGcmSetScissor(static_cast<int16>(rDSState.MinX()),
																									static_cast<int16>(rDSState.MinY()),
																									static_cast<int16>(rDSState.MaxX()),
																									static_cast<int16>(rDSState.MaxY()));
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStateDepthBoundsSPU& rDepthBounds)
															{
																if(CACHE)
																{
																	m_DirtyStates		|=	EDXPSDS_DEPTHBOUNDS;
																	m_DepthBounds		=	rDepthBounds;
																	return;
																}
																using namespace CRY_DXPS_GCMNAMESPACE;
																if(rDepthBounds.Enable())
																{
																	cellGcmSetDepthBoundsTestEnable(CELL_GCM_TRUE);
																	cellGcmSetDepthBounds(rDepthBounds.ZMin(),rDepthBounds.ZMax());
																}
																else
																	cellGcmSetDepthBoundsTestEnable(CELL_GCM_FALSE);
															}

	void												Job(const CDXPSRDJUpdateStateTopologySPU& rDSState)
															{
																m_Topology=rDSState.Topology();
															}
	template<bool CACHE>
	void												Job(const CDXPSRDJUpdateStatePassSPU& rPass)
															{
																/*
																if(CACHE)
																{
																	m_DirtyStates		|=	EDXPSDS_PASS;
																	m_Pass					=	rPass;
																	return;
																}*/
																using namespace CRY_DXPS_GCMNAMESPACE;
																if(rPass.Pass()==1)//D3D11_PASS_ZONLY)
																	cellGcmSetZcullLimit(256,256);
																if(rPass.Pass()==2)//D3D11_PASS_ZCULLUPDATE)
																{
																	DownloadDepthBuffer(0,rPass.ViewMat());
																	cellGcmSetZcullLimit(512,256);
																}
																if(rPass.Pass()==3)//D3D11_PASS_DIFFUSE)
																	cellGcmSetZcullLimit(0xffff, 0);
																if(rPass.Pass()==5)//D3D11_PASS_DONESHADOWLAYER0)
																	DownloadDepthBuffer(1,rPass.ViewMat());
																if(rPass.Pass()==6)//D3D11_PASS_DONESHADOWLAYER1)
																	DownloadDepthBuffer(2,rPass.ViewMat());
																if(rPass.Pass()==7)//D3D11_PASS_DONESHADOWLAYER2)
																	DownloadDepthBuffer(3,rPass.ViewMat());
															}

	void												Job(const CDXPSRDJUpdateStateDebugProfileSPU& rDebugProfile)
															{
#ifndef __SPU__
																const char* pName	=	rDebugProfile.Name();
																switch(rDebugProfile.Type())
																{
																	case EDXPSJDPT_SETMARKER:			cell::Gcm::cellGcmSetPerfMonMarker(pName);break;
																	case EDXPSJDPT_SETPUSHMARKER: cell::Gcm::cellGcmSetPerfMonPushMarker(pName);break;
																	case EDXPSJDPT_SETPOPMARKER:	cell::Gcm::cellGcmSetPerfMonPopMarker();break;
																}
#endif
															}
	void												Job(const CDXPSRDJUpdateDebugDataSPU& rDebugData);





#endif//__SPU__ || !_RELEASE

public:

#ifndef __SPU__
															CDXPSRDWorker(CCryDXPSGCMSyncMan* __restrict);
	ILINE void									InitPSCache(){m_PSCache.Init();}
#endif
	uint32											PSAddr()const{return (uint32)&m_PSCache;}

#if !defined(_RELEASE) || defined(__SPU__)
	void												WorkOn(const class CDXPSRDJob* const __restrict pJob, const EDXPSJob cType);
	ILINE void									WorkOn(const class CDXPSRDJob* const __restrict pJob)
															{
																WorkOn(pJob, (EDXPSJob)pJob->Type());
															}

	ILINE void									Reset()
															{
																UpdateShader(NULL, NULL);
																m_DirtyFlags					=	~0;
																m_VertexAttributeMask	=	~0;
																m_LastBaseVL					=	~0;
																m_ShaderCache.Reset();
																ResetPDesc();
																ResetVDesc();
																memset(m_pVertexBuffer,0,sizeof(m_pVertexBuffer));
																memset(m_ConstBuffer,0,sizeof(m_ConstBuffer));
															}

	ILINE void									UpdateShader(CCryDXPSShader* pPShader, CCryDXPSShader* pVShader)
															{
																if(pPShader)
																{
																	PShader(pPShader);
																	DirtyFlags()	 |=	EDXPSDF_PIXELSHADER;
																	ResetPDesc();
																}
																ResetPSBuffer(pPShader);

																if(pVShader)
																{
																	VShader(pVShader);
																	DirtyFlags()	 |=	EDXPSDF_VERTEXSHADER;
																	ResetVDesc();
																}
																ResetVSBuffer(pVShader);

																ShaderCache().Reset();
															}

private:	
	ILINE	uint32								DirtyFlags() const{return m_DirtyFlags;};
	ILINE	uint32&								DirtyFlags()      {return m_DirtyFlags;};

#if defined(DRAWCALLDEBUGGING)
	ILINE	uint32								DrawCall() const{return m_DrawCall;};
	ILINE	uint32&								DrawCall()      {return m_DrawCall;};
#endif

	ILINE	CCryDXPSGCMSyncMan&		Sync()const{return *m_pSyncMan;}

	ILINE void									ResetPDesc() {m_pPSDesc = 0;}
	ILINE void									ResetVDesc() {m_pVSDesc = 0;}

	ILINE void									ResetPSBuffer(CCryDXPSShader* pShader = NULL) 
															{
																if(pShader)
																{
																	const uint32 cLZSSSize	= pShader->ProgramLZSSSize();
																	IF(cLZSSSize > LOCAL_PS_BUFFER_SIZE, 0)snPause();
																	m_pPSSrc	= (uint8*)cellGcmCpyUCodeLS(pShader->ProgramLZSS(), cLZSSSize, LOCAL_PS_BUFFER_SIZE - cLZSSSize);
																	m_pPSDst	= cellGcmGetPSBuf();
																}
																else
																{
																	m_pPSSrc = 0;	m_pPSDst = 0;
																}
															}
	ILINE void									ResetVSBuffer(CCryDXPSShader* pShader = NULL) 
															{
																if(pShader)
																{
																	const uint32 cLZSSSize	= pShader->ProgramLZSSSize();
																	IF(cLZSSSize > LOCAL_VS_BUFFER_SIZE, 0)snPause();
																	m_pVSSrc = (uint8*)cellGcmCpyVertexCodeLS(pShader->ProgramLZSS(), cLZSSSize, LOCAL_VS_BUFFER_SIZE - cLZSSSize);
																	m_pVSDst =	cellGcmGetVSBuf();
																}
																else
																{
																	m_pVSSrc = 0;	m_pVSDst = 0;
																}
															}




















































	ILINE const CCryDXPSShader*	VShader() const{return m_pVShader;m_ShaderTransferInFlight=0;}
	ILINE const CCryDXPSShader*	PShader() const{return m_pPShader;m_ShaderTransferInFlight=0;}
	ILINE void									VShader(const CCryDXPSShader* pVShader) {m_pVShader = pVShader;}
	ILINE void									PShader(const CCryDXPSShader* pPShader) {m_pPShader = pPShader;}
	ILINE	void									SyncShaderWriteBack(){m_ShaderBackTransferInFlight = false;}
	ILINE	void									WriteBackShader(){}


	CCryDXPSGCMPixelshaderCacheMan& PSCache(){return m_PSCache;}

	ILINE const CCryDXPSInputLayout* InputLayout()const{return m_pInputLayout;}
	ILINE uint32*								VBStride() {return m_VBStride;}
	ILINE	void									SyncConstBufferTransfers()
															{
#if defined(CRY_DXPS_LSCONSTANTS)
																IF(m_ConstBufferTransferActive,1)
																{
																	m_ConstBufferTransferActive = false;
																	memtransfer_sync(CB_BUF_TRANSFER_TAG);
																}
#endif
															}
#endif//__SPU__ || !_RELEASE
	ILINE CCryDXPShaderCache&		ShaderCache()	{return m_ShaderCache;}
} _ALIGN(128);

#endif

