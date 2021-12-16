#ifndef __CRYDXPSRDTHREAD__
#define __CRYDXPSRDTHREAD__

#include "CryThread.h"
#include "CCryDXPSRDWorker.hpp"
#include "../Layer0/GCM/CCryDXPSGCM_SyncMan.hpp"
#include <IJobManSPU.h>

extern SFlipVars g_FlipVars;




	#define DEVICE_CMD_SYNC_SLEEP Sleep(4);//necessary as renderthread stalls main and dxps otherwise


extern bool g_SwitchSPUs;
#ifdef CRY_DXPS_DEVICETHREAD
	//enable synchronous execution of each flushing job chain
	//#define WORKONJOB {READ_WRITE_BARRIER; m_CMDBPut	=	m_CMDBCurrent;if(!IsSPUEnabled())Notify();/*else */Finish(); cell::Gcm::UnsafeInline::cellGcmFinish(0);}
  #define WORKONJOB {/*READ_WRITE_BARRIER; */uint32* pCMDBPut=(uint32*)&m_CMDBPut;*pCMDBPut=m_CMDBCurrent;if(!IsSPUEnabled())Notify();}
#else
	#define WORKONJOB 
#endif
extern void StopSPUs();
extern void PrepareFlipCallback(void*);

class CDXPSRDThread : CrySimpleThread<>
{
	uint32								m_CMDBCurrent _ALIGN(128);
	volatile uint32				m_CMDBPut;
	ppu_volatile uint32		m_CMDBGet _ALIGN(16);//transferred from SPU via a dma, align to 16 byte boundary
	CCryDXPSShader*				m_pLastPixelShader;
	CCryDXPSShader*				m_pLastVertexShader;
	uint8									m_CMDBuffer[DXPS_DEVICECMDBSIZE] _ALIGN(128);
	CDXPSRDWorker					m_Worker;

	NPPU::SFrameProfileRSXData m_FrameStatsSPU _ALIGN(16);//stats of DXPS-thread from SPU

	CryMutex							m_LockNotify;
	CryConditionVariable	m_Condition;
	ppu_volatile int			m_Waiting;
  uint32                m_Finished;

	ILINE void						Notify()
												{
#if !defined(__SPU__)
													READ_WRITE_BARRIER
													m_LockNotify.Lock();
													if(m_Waiting)
													{
														m_Waiting=0;
														m_Condition.Notify();
													}
													m_LockNotify.Unlock();
#endif
												}




#if defined(CRY_DXPS_DEVICETHREAD)
		void							Run();
#endif

	template<class T>
	ILINE T&					ThisJob()
										{
											return	*reinterpret_cast<T*>(&m_CMDBuffer[m_CMDBCurrent+DXPS_DEVICECMDBALLIGN]);
										}

	template<bool BOUNDSCHECK>
	ILINE void				NextJob(uint32& __restrict Pt,const uint32 Size)
										{
											uint32 curPt = Pt;
											uint32 newPt = curPt +	Size+DXPS_DEVICECMDBALLIGN;//adding DXPS_DEVICECMDBALLIGN for header
											newPt	=	(newPt+DXPS_DEVICECMDBALLIGN-1)&~DXPS_DEVICECMDBALLIGNMASK;

											//branch-free impl.of: if(newPt+Size+DXPS_DEVICECMDBPREFETCH>DXPS_DEVICECMDBSIZE)newPt=0;
											const uint32 mask = (uint32)(((int32)(newPt+Size+DXPS_DEVICECMDBPREFETCH-DXPS_DEVICECMDBSIZE)) >> 31);
											newPt = mask & newPt;

											uint32 curGet = BOUNDSCHECK?m_CMDBGet : 0;
											//only executed for ppu job push
											uint32 loopCount = 0;
											while(BOUNDSCHECK && curGet>curPt && curGet<=curPt+DXPS_DEVICECMDBPREFETCH)
											{
												DEVICE_CMD_SYNC_SLEEP
												curGet = *(volatile uint32*)&m_CMDBGet;//read volatile
											}

											//only executed for ppu job push
											loopCount = 0;
											while(BOUNDSCHECK && curGet>newPt && curGet<=newPt+DXPS_DEVICECMDBPREFETCH)
											{
												DEVICE_CMD_SYNC_SLEEP
												curGet = *(volatile uint32*)&m_CMDBGet;//read volatile
											}
											Pt = newPt;
										}

	ILINE void				NextJob(const uint32 Size)
										{
#ifdef CRY_DXPS_DEVICETHREAD
											NextJob<true>((uint32&)m_CMDBCurrent,Size);
#else
											CDXPSRDJobSPU* pJob	=	reinterpret_cast<CDXPSRDJobSPU*>(&m_CMDBuffer[m_CMDBCurrent]);
											m_Worker.WorkOn(pJob);
#endif
										}

public:

	void							Init();


#ifndef __SPU__
										CDXPSRDThread():
										m_Waiting(1),
										m_CMDBCurrent(0),
										m_CMDBPut(0),
										m_CMDBGet(0),
										m_pLastPixelShader(0),
										m_pLastVertexShader(0),
                    m_Finished(0),
										m_Worker(&tdLayer0::Sync())
										{
											memset(&m_FrameStatsSPU, 0, sizeof(m_FrameStatsSPU));
										}
										~CDXPSRDThread();

	ILINE NPPU::SFrameProfileRSXData& GetFrameStatsSPUThread()
										{
											return m_FrameStatsSPU;
										}
#endif //__SPU__

	void							Finish()
										{
#ifndef __SPU__
	#if defined(CRY_DXPS_DEVICETHREAD)
											volatile uint32 *const pCurGet = (volatile uint32*)&m_CMDBGet;
											Notify();
											SYNC_LOOP_INIT
											while(m_CMDBPut!=*pCurGet)
											{
												SYNC_LOOP
											}
	#endif//CRY_DXPS_DEVICETHREAD
#endif //__SPU__
										}

	void							StopThread()
										{
#if !defined(__SPU__) && defined(CRY_DXPS_DEVICETHREAD)
											NextJob(ThisJob<CDXPSRDJDummyPPU>().Setup(EDXPSJ_EXIT));
											WORKONJOB;
#endif
										}

  bool              HasFinished() const
                    {
                      return m_Finished != 0;
                    }

#if !defined(__SPU__)
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	ILINE void				DrawIndexed(uint32 IndexCount,uint32 StartIndexLocation,int BaseVertexLocation,uint32 InvalidFilteringFlag,CCryDXPSBuffer* pIndexBuffer,uint32 IBFormat,uint32 IBOffset,const SRegisteredZWrite& ZWrite,uint32 IBIdx)
										{
											NextJob(ThisJob<CDXPSRDJDrawIndexedPPU>().Setup(IndexCount,StartIndexLocation,BaseVertexLocation,InvalidFilteringFlag,pIndexBuffer,IBFormat,IBOffset,ZWrite,IBIdx));
#else
	ILINE void				DrawIndexed(uint32 IndexCount,uint32 StartIndexLocation,int BaseVertexLocation,uint32 InvalidFilteringFlag,CCryDXPSBuffer* pIndexBuffer,uint32 IBFormat,uint32 IBOffset,const SRegisteredZWrite& ZWrite)
										{
											NextJob(ThisJob<CDXPSRDJDrawIndexedPPU>().Setup(IndexCount,StartIndexLocation,BaseVertexLocation,InvalidFilteringFlag,pIndexBuffer,IBFormat,IBOffset,ZWrite));
#endif
											WORKONJOB;
										}
	ILINE void				CopySubresourceScaled(CCryDXPSTexture2D *pDst,const D3D11_BOX *pDstBox,CCryDXPSTexture2D *pSrc,const D3D11_BOX *pSrcBox)
										{
											NextJob(ThisJob<CDXPSRDJCopyResourceScaledPPU>().Setup(pDst,pDstBox,pSrc,pSrcBox));
											WORKONJOB;
										}
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	ILINE void				CopySubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox,uint32 SrcVBIdx,uint32 DstVBIdx)
										{
											NextJob(ThisJob<CDXPSRDJCopyResourcePPU>().Setup(pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox,SrcVBIdx,DstVBIdx));
#else
	ILINE void				CopySubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox)
										{
											NextJob(ThisJob<CDXPSRDJCopyResourcePPU>().Setup(pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox));
#endif
											WORKONJOB;
										}
	ILINE void				UpdateRendertargets(	APWeakTexture2D	pRenderTargetViews0,
																					APWeakTexture2D	pRenderTargetViews1,
																					APWeakTexture2D	pRenderTargetViews2,
																					APWeakTexture2D	pRenderTargetViews3,
																					APWeakTexture2D	pDepthStencilView,
																					uint32 AAMode,uint32 ATC)
										{
											NextJob(ThisJob<CDXPSRDJRenderTargetPPU>().Setup(pRenderTargetViews0,
																																				pRenderTargetViews1,
																																				pRenderTargetViews2,
																																				pRenderTargetViews3,
																																				pDepthStencilView,
																																				AAMode,ATC));
										}
	ILINE void				Viewport(const D3D11_VIEWPORT& rViewPort)
										{
											NextJob(ThisJob<CDXPSRDJViewPortPPU>().Setup(rViewPort));
										}

	void							SwapBuffers(uint32 Frame,APWeakTexture2D pTexBack,APWeakTexture2D pTexFront)
										{
											NextJob(ThisJob<CDXPSRDJSwapPPU>().Setup(Frame,pTexBack,pTexFront,gPS3Env->flipMode, (uint32)&PrepareFlipCallback));
											WORKONJOB;
#ifndef _RELEASE
											//if switching spu is requested, sync thread and enable/disable spus (only safe here)
											if(g_SwitchSPUs)
											{
												Finish();
												while(g_FlipVars.pFlipLockedTarget){Sleep(0);}
												g_SwitchSPUs = false;
												gPS3Env->spuEnabled = IsSPUEnabled()?0:1;//switch
												if(!gPS3Env->spuEnabled)
													m_Worker.Reset();
											}
#endif
										}

	ILINE void				PointSpriteControl(const uint32 Enable, const uint32 RMode, const uint32 Texcoord)
										{
											NextJob(ThisJob<CDXPSRDJPointSpritePPU>().Setup(Enable, RMode, Texcoord));
										}

	ILINE void				ClearRenderTarget(const float Color[4])
										{
											NextJob(ThisJob<CDXPSRDJClearColorPPU>().Setup(Color));
										}
	
	ILINE void				ClearTexture(CCryDXPSTexture2D *pTex,const uint32 Color)
										{
											NextJob(ThisJob<CDXPSRDJClearTexturePPU>().Setup(pTex,Color));
										}

	ILINE void				ClearDepthStencil(uint32 ClearFlags,float Depth,uint8 Stencil)
										{
											NextJob(ThisJob<CDXPSRDJClearDepthStencilPPU>().Setup(ClearFlags,Depth,Stencil));
										}

	ILINE void				UpdateCmdBuffer()
										{
											NextJob(ThisJob<CDXPSRDJDummyPPU>().Setup(EDXPSJ_UPDATECMDBUF));
										}

//STATES
	ILINE void				PreparePass(uint32 Pass,float* pViewMat)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStatePassPPU>().Setup(Pass,pViewMat));
										}
	ILINE uint8*			ConstantBuffersAllocate(uint32 Size,uint32 Idx,bool VertexShaderCB)
										{
											CDXPSRDJUpdateStateConstBufferPPU& rJob	=	ThisJob<CDXPSRDJUpdateStateConstBufferPPU>();
											uint8* pConstantBuffer;
											NextJob(rJob.Setup(static_cast<EDXPSJob>((VertexShaderCB?EDXPSJ_STATE_VCONSTBUFFER0:EDXPSJ_STATE_PCONSTBUFFER0)+Idx),Size,&pConstantBuffer));
											return pConstantBuffer;
										}
	ILINE void				VSConstantBuffers(uint32 Idx,ID3D11Buffer* pConstantBuffer)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateConstBufferPPU>().Setup(static_cast<EDXPSJob>(EDXPSJ_STATE_VCONSTBUFFER0+Idx),pConstantBuffer));
										}
	ILINE void				PSConstantBuffers(uint32 Idx,ID3D11Buffer* pConstantBuffer)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateConstBufferPPU>().Setup(static_cast<EDXPSJob>(EDXPSJ_STATE_PCONSTBUFFER0+Idx),pConstantBuffer));
										}
	ILINE void				PSShaderResources(uint32 Idx,CCryDXPSTexture* pTex)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateTexturePPU>().Setup(Idx,pTex));
										}
	ILINE void				PSShader(CCryDXPSShader* pPixelShader)
										{
											m_pLastPixelShader = pPixelShader;
											NextJob(ThisJob<CDXPSRDJUpdateStateShaderPPU>().Setup(EDXPSJ_STATE_PIXELSHADER,pPixelShader,pPixelShader->ProgramLZSS(),pPixelShader->ProgramLZSSSize()));
										}
	ILINE void				VSShader(CCryDXPSShader* pVertexShader)
										{
											m_pLastVertexShader = pVertexShader;
											NextJob(ThisJob<CDXPSRDJUpdateStateShaderPPU>().Setup(EDXPSJ_STATE_VERTEXSHADER,pVertexShader,pVertexShader->ProgramLZSS(),pVertexShader->ProgramLZSSSize()));
										}
	ILINE void				InputLayout(CCryDXPSInputLayout* pInputLayout)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateInputLayoutPPU>().Setup(pInputLayout));
										}
	ILINE void				VertexBuffers(uint32 Idx,CCryDXPSBuffer* pVertexBuffer,uint32 Stride,uint32 Offset)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateVertexBufferPPU>().Setup(Idx,pVertexBuffer,Stride,Offset));
										}
	ILINE void				Topology(uint32 TPG)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateTopologyPPU>().Setup(TPG));
										}
	ILINE void				BlendState(CCryDXPSBlendState* pBlendState,const float BlendFactor[4])
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateBlendPPU>().Setup(pBlendState,BlendFactor));
										}
	ILINE void				DepthStencilState(CCryDXPSDepthStencilState* pDepthStencilState,uint32 StencilRef)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateStencilPPU>().Setup(pDepthStencilState,StencilRef));
										}
	ILINE void				RasterizerState(CCryDXPSRasterizerState* pRasterizerState,uint32 sRGB,uint32 AlphaTestRef)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateRasterizerPPU>().Setup(pRasterizerState,sRGB,AlphaTestRef));
										}
	ILINE void				Sampler(uint32 Idx,CCryDXPSSamplerState* pSamplers,uint32 SamplerMinLOD,uint32 SamplerMaxLOD,uint16 SamplerLODBias,uint16 sRGB)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateSamplerPPU>().Setup(Idx,pSamplers,SamplerMinLOD,SamplerMaxLOD,SamplerLODBias,sRGB));
										}
	ILINE void				ScissorRect(const int MinX,const int MinY,const int MaxX,const int MaxY)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateScissorPPU>().Setup(MinX,MinY,MaxX,MaxY));
										}
	ILINE void				DepthBounds(uint32 Enable,const float ZMin,const float ZMax)
										{
											NextJob(ThisJob<CDXPSRDJUpdateStateDepthBoundsPPU>().Setup(Enable,ZMin,ZMax));
										}
	ILINE void				DebugData(const uint32 Slot,const uint32 Data)
										{
#if defined(CRY_DXPS_THREAD_DEBUGDATA)
											NextJob(ThisJob<CDXPSRDJUpdateDebugDataPPU>().Setup(Slot,Data));
#endif
										}

/*		ILINE void			MemCpy(void* pDst,const void* pSrc,uint32 Size)
										{
#if !defined(__SPU__)
                      NextJob(ThisJob<CDXPSRDJCopyPPU>().Setup(pDst,pSrc,Size));
											WORKONJOB;
#endif
										}
		ILINE void			MemCpyScaled2D(				void* pDst,uint32 DstSizeX,uint32 DstSizeY,uint32 DstPitch,
																		const void* pSrc,uint32 SrcSizeX,uint32 SrcSizeY,uint32 SrcPitch)
										{
#if !defined(__SPU__)
                      NextJob(ThisJob<CDXPSRDJCopyPPU>().Setup(pDst,DstSizeX,DstSizeY,DstPitch,pSrc,SrcSizeX,SrcSizeY,SrcPitch));
											WORKONJOB;
#endif
                    }*/


		ILINE void			PushMarker(const char* pName)
										{
                      NextJob(ThisJob<CDXPSRDJUpdateStateDebugProfilePPU>().PushMarker(pName));
                    }
		ILINE void			PopMarker(const char* pName)
										{
                      NextJob(ThisJob<CDXPSRDJUpdateStateDebugProfilePPU>().PopMarker(pName));
                    }
		ILINE void			SetMarker(const char* pName)
										{
                      NextJob(ThisJob<CDXPSRDJUpdateStateDebugProfilePPU>().SetMarker(pName));
                    }
#endif

} _ALIGN(128);

#undef STATE

#endif

