#ifndef __CRYDXPSRENDERDEVICE__
#define __CRYDXPSRENDERDEVICE__

//#define DRAWCALLDEBUGGING
#ifndef __SPU__
	#include <StdAfx.h>
#endif
#include "../CCryDXPSDummy.hpp"
#include "../CCryDXPSMisc.hpp"

#include "../Layer0/CCryDXPS.hpp"
#include "CCryDXPSRDThread.hpp"
#include "CCryDXPSRDState.hpp"
#include "CCryDXPSRDRasterizer.hpp"

#ifdef DO_RENDERLOG
	#include <DXPS/d3d10_ps3.h>
#endif

const uint32 CRYDXPSDEVICE_MAXRENDERTARGETS	=	4;

class CCryDXPSSwapChain;


enum{EDXPSAsyncJobCount=256};//power of 2
enum EDXPSAsyncJob
{
	EDXPSAJ_COPYSUBRESOURCE
};

class CCryDXPSAsyncJob
{
	EDXPSAsyncJob							m_Type;
	union
	{
	struct
	{
		ID3D11Resource*					m_pDstResource;
		ID3D11Resource*					m_pSrcResource;
//		uint32									m_DstX;
//		uint32									m_DstY;
//		uint32									m_DstZ;
		uint16									m_DstSubresource;
		uint16									m_SrcSubresource;
	};
	};


public:
	
	EDXPSAsyncJob							Type()const{return m_Type;}
	void											CopySubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox)
														{
															m_Type						=	EDXPSAJ_COPYSUBRESOURCE;
															m_pDstResource		=	pDstResource;
															m_DstSubresource	=	DstSubresource;
//															m_DstX						=	DstX;
//															m_DstY						=	DstY;
//															m_DstZ						=	DstZ;
															m_pSrcResource		=	pSrcResource;
															m_SrcSubresource	=	SrcSubresource;
															if(pDstResource->Type() != EDXPS_RT_TEX2D || m_pSrcResource->Type() != EDXPS_RT_TEX2D)
																snPause();
														}
	ID3D11Resource*						DstResource(){return m_pDstResource;}
	uint32										DstSubresource(){return m_DstSubresource;}
	uint32										DstX(){return 0;}//m_DstX;}
	uint32										DstY(){return 0;}//m_DstY;}
	uint32										DstZ(){return 0;}//m_DstZ;}
	ID3D11Resource*						SrcResource(){return m_pSrcResource;}
	uint32										SrcSubresource(){return m_SrcSubresource;}
};



class CCryDXPSRenderDevice;
class CCryDXPSRenderDevice		:		public	CCryRefAndWeak<CCryDXPSRenderDevice>
#ifdef DO_RENDERLOG
	,public	ID3D11Device
#endif
{
	friend class CCryDXPSSwapChain;
private:
	uint8													m_NotPointSampler[16];
	uint8													m_PointTexture[16];
	uint8													m_NoSRGBTexture[16];
	uint32												m_InvalidFilteringFlag;
	uint32												m_InvalidSRGBFlag;
	APWeakTexture2D								m_pRenderTargetViews[CRYDXPSDEVICE_MAXRENDERTARGETS];
	APWeakTexture2D								m_pDepthStencilView;
	int														m_ClearableFormat;

	uint32												m_HalfResRendering;
	uint32												m_AlphaToCoverage;
	D3D11_VIEWPORT								m_GCMViewPort;
//	CellGcmSurface								m_Surface[3];

	CCryDXPSShader*								m_pLastPixelShader;
	CCryDXPSShader*								m_pLastVertexShader;

	APWeakBuffer									m_pIndexBufferDraw;
	APWeakBuffer									m_pIndexBuffer;
	uint32												m_IBFormat;
	uint32												m_IBOffset;

#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32												m_pVBStride[16];
	uint32												m_pVBOffset[16];
#endif
	APWeakBuffer									m_pVertexBuffer[16];//max 16 input slots
	APWeakBuffer									m_pVSConstBuffer[16];
	APWeakBuffer									m_pPSConstBuffer[16];
#if !defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
	tdResHandle*									m_pLockHandles[48];
	uint32												m_LockHandleIdx[48];
	uint32												m_LockHandleIdxRev[48];
	uint32												m_LockHandleCount;
#endif

	CCryDXPSInputLayout*					m_pInputLayout;
	CCryDXPSSamplerState*					m_pSampler[16];
	uint16												m_pSamplerSRGB[16];
	uint16												m_pSamplerMipBias[16];
	CCryDXPSBlendState*						m_pBlendState;
	float													m_BlendFactor[4];
	CCryDXPSDepthStencilState*		m_pDepthStencilState;
	uint32												m_StencilRef;
	ID3D11RasterizerState*				m_pRasterizerState;
	uint32												m_RasterizerSRGB;
#if defined(CRY_DXPS_RASTERTHREAD)
	CDXPSRDRasterizer							m_SoftRast;
#endif
	CDXPSRDThread									m_Thread;
	CryMutex											m_CopyLock;
	bool													m_SPUInitialized;
	void*													m_pReportArea;
	CellGcmConfig									m_GCMConfig;
	uint32												m_CmdAddressBase;

	float													m_ViewProjMat[16];


	CCryDXPSAsyncJob							m_AsyncJob[EDXPSAsyncJobCount];
	volatile uint16								m_AsyncPut;
	volatile uint16								m_AsyncGet;


	int16													m_SamplerLODMin[16];
	int16													m_SamplerLODMax[16];
	uint16												m_SamplerDirty;

#ifdef DRAWCALLDEBUGGING
//public:
	uint32												m_BreakDrawCall;
	uint32												m_DrawCounter;
#endif
	void													FlushSampler();
public:
											CCryDXPSRenderDevice();
											CCryDXPSRenderDevice(	IDXGIAdapter*			pAdapter );
											~CCryDXPSRenderDevice();

			void						InitSPU();

			ECRYDXPSResType	Type()const{return EDXPS_RT_RENDERDEVICE;}
			unsigned long		AddRef(){return IncRef();}
			unsigned long		Release(){return DecRef();}
			HRESULT					QueryInterface(REFIID riid,void **ppvObject);
 
			void						BeginScene();
			void						EndScene();

			ILINE	bool			Locked(){return false;}
			ILINE	void			ReleaseResources(){}
      void            VSSetConstantBuffers_Log(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers);
			void  					VSSetConstantBuffers(uint32	Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers);
			void  					PSSetShaderResources(uint32	Offset,uint32 NumViews,ID3D11ShaderResourceView *const *ppShaderResourceViews);
			void  					PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
			void  					PSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers,const float* pMipBias,uint16* psRGB=0);
			void  					VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
			/*tdResHandle*/ void DrawIndexed(uint32 IndexCount,uint32 StartIndexLocation,int BaseVertexLocation);
			/*tdResHandle*/ void Draw(uint32 VertexCount,uint32 StartVertexLocation);
      void  					PSSetConstantBuffers_Log(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers);
			void  					PSSetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers);
			void  					IASetInputLayout(ID3D11InputLayout *pInputLayout);
			void  					IASetVertexBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppVertexBuffers,const uint32*pStrides,const uint32*pOffsets);
			void						ResetBuffer(const ID3D11Buffer* pVertexBuffers);
			void  					IASetIndexBuffer(ID3D11Buffer *pIndexBuffer,DXGI_FORMAT Format,uint32 Offset);
			void  					DrawIndexedInstanced( uint32 IndexCountPerInstance,uint32 InstanceCount,uint32 StartIndexLocation,int BaseVertexLocation,uint32 StartInstanceLocation);
			void  					DrawInstanced( uint32 VertexCountPerInstance,uint32 InstanceCount,uint32 StartVertexLocation,uint32 StartInstanceLocation);
			void  					GSSetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers);
			void  					GSSetShader(ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances);
			void  					IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology);
			void  					VSSetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView *const *ppShaderResourceViews);
			void  					VSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers);
			void  					SetPredication(ID3D11Predicate *pPredicate,BOOL PredicateValue);
			void  					GSSetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView *const *ppShaderResourceViews);
			void  					GSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers);
			void  					OMSetRenderTargets(uint32 NumViews,ID3D11RenderTargetView *const *ppRenderTargetViews,ID3D11DepthStencilView *pDepthStencilView);
			void  					OMSetBlendState(ID3D11BlendState *pBlendState,const float BlendFactor[ 4 ],uint32 SampleMask);
			void  					OMSetDepthStencilState(ID3D11DepthStencilState *pDepthStencilState,uint32 StencilRef);
			void  					SOSetTargets(uint32 NumBuffers,ID3D11Buffer *const *ppSOTargetsu,const uint32 *pOffsets);
			void  					DrawAuto( void);
			void  					RSSetState(ID3D11RasterizerState *pRasterizerState,uint32 sRGB,uint32 AlphaTestRef=0);
			void  					RSSetState(ID3D11RasterizerState *pRasterizerState);
			void  					RSSetViewports(uint32 NumViewports,const D3D11_VIEWPORT *pViewports);
			void  					RSSetScissorRects(uint32 NumRects,const D3D11_RECT *pRects);
			void						RSSetDepthBounds(uint32 Enable,const float ZMin,const float ZMax);
			void  					CopySubresourceRegionScaled(CCryDXPSTexture2D *pDst,const D3D11_BOX *pDstBox,CCryDXPSTexture2D *pSrc,const D3D11_BOX *pSrcBox);
			void  					CopySubresourceRegion(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox);
			void  					CopySubresourceRegionSafe(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox);
			void  					CopyResource(ID3D11Resource *pDstResource,ID3D11Resource *pSrcResource);
			void  					UpdateSubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,const D3D11_BOX *pDstBox,const void *pSrcData,uint32 SrcRowPitch,uint32 SrcDepthPitch);
			void  					ClearRenderTargetView( ID3D11RenderTargetView *pRenderTargetView,const float ColorRGBA[ 4 ]);
			void  					PointSpriteControl(const uint32 Enable, const uint32 RMode, const uint32 Texcoord);
			void  					ClearDepthStencilView( ID3D11DepthStencilView *pDepthStencilView,uint32 ClearFlags,float Depth,uint8 Stencil);
			void						UpdateCmdBuffer();
			void  					GenerateMips(ID3D11ShaderResourceView *pShaderResourceView);
			void  					ResolveSubresource(ID3D11Resource *pDstResource,uint32 DstSubresource,ID3D11Resource *pSrcResource,uint32 SrcSubresource,DXGI_FORMAT Format);
			void  					VSGetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppConstantBuffers);
			void  					PSGetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView **ppShaderResourceViews);
			void  					PSGetShader(ID3D11PixelShader **ppPixelShader);
			void  					PSGetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState **ppSamplers);
			void  					VSGetShader(ID3D11VertexShader **ppVertexShader);
			void  					PSGetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppConstantBuffers);
			void  					IAGetInputLayout(ID3D11InputLayout **ppInputLayout);
			void  					IAGetVertexBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppVertexBuffers,uint32*pStrides,uint32 *pOffsets);
			void  					IAGetIndexBuffer(ID3D11Buffer **pIndexBuffer,DXGI_FORMAT *Format,uint32*Offset);
			void  					GSGetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer **ppConstantBuffers);
			void  					GSGetShader(ID3D11GeometryShader **ppGeometryShader);
			void  					IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY *pTopology);
			void  					VSGetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView **ppShaderResourceViews);
			void  					VSGetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState **ppSamplers);
			void  					GetPredication(ID3D11Predicate **ppPredicate,BOOL *pPredicateValue);
			void  					GSGetShaderResources(uint32 Offset,uint32 NumViews,ID3D11ShaderResourceView **ppShaderResourceViews);
			void  					GSGetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState **ppSamplers);
			void  					OMGetRenderTargets(uint32 NumViews,ID3D11RenderTargetView **ppRenderTargetViews,ID3D11DepthStencilView **ppDepthStencilView);
			void  					OMGetBlendState(ID3D11BlendState **ppBlendState,float BlendFactor[ 4 ],uint32*pSampleMask);
			void  					OMGetDepthStencilState(ID3D11DepthStencilState **ppDepthStencilState,uint32*pStencilRef);
			void  					SOGetTargets(uint32 NumBuffers,ID3D11Buffer **ppSOTargets,uint32* pOffsets);
			void  					RSGetState(ID3D11RasterizerState **ppRasterizerState);
			void  					RSGetViewports(uint32* NumViewports,D3D11_VIEWPORT *pViewports);
			void  					RSGetScissorRects(uint32* NumRects,D3D11_RECT *pRects);
			long  					GetDeviceRemovedReason( void);
			long  					SetExceptionMode(uint32 intRaiseFlags);
			uint32					GetExceptionMode( void);
			long  					GetPrivateData(const ECryGUID & guid,uint32*pDataSize,void *pData);
			long  					SetPrivateData(const ECryGUID & guid,uint32 DataSize,const void *pData);
			long  					SetPrivateDataInterface( const ECryGUID & guid,const IUnknown *pData);
			void						SetTextFilterSize(unsigned int Width,unsigned int Height);
			void						GetTextFilterSize(unsigned int* Width,unsigned int* Height);
			void  					ClearState( void);        
			void  					Flush( void);        
			long  					CreateBuffer(const D3D11_BUFFER_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Buffer **ppBuffer);
			long  					CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Texture1D **ppTexture1D);
			long  					CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Texture2D **ppTexture2D);
			long  					CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Texture3D **ppTexture3D);
			long  					CreateShaderResourceView( CCryDXPSResource *pResource,const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,ID3D11ShaderResourceView **ppSRView);
			long  					CreateRenderTargetView(ID3D11Resource *pResource,const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,ID3D11RenderTargetView **ppRTView);
			long  					CreateDepthStencilView(ID3D11Resource *pResource,const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,ID3D11DepthStencilView **ppDepthStencilView);
			long  					CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,uint32 NumElements,const void *pShaderBytecodeWithInputSignature,SIZE_T Size,ID3D11InputLayout **ppInputLayout);
			long  					CreateVertexShader(const void *pShaderBytecode,SIZE_T Size,ID3D11ClassLinkage *pClassLinkage,ID3D11VertexShader **ppVertexShader);
			long  					CreateGeometryShader(const void *pShaderBytecode,size_t Size,ID3D11ClassLinkage *pClassLinkage,ID3D11GeometryShader **ppGeometryShader);
			long  					CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode,size_t BytecondeLength,const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,  UINT NumEntries,const UINT *pBufferStrides,UINT NumStrides,UINT RasterizedStream,ID3D11ClassLinkage *pClassLinkage,ID3D11GeometryShader **ppGeometryShader);
			long  					CreatePixelShader(const void *pShaderBytecode,SIZE_T Size,ID3D11ClassLinkage *pClassLinkage,ID3D11PixelShader **ppPixelShader);
			long  					CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc,ID3D11BlendState **ppBlendState);
			long  					CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,ID3D11DepthStencilState **ppDepthStencilState);
			long  					CreateRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc,ID3D11RasterizerState **ppRasterizerState);
			long  					CreateSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc,ID3D11SamplerState **ppSamplerState);
			long  					CreateQuery(const D3D11_QUERY_DESC *pQueryDesc,ID3D11Query **ppQuery);
			long  					CreatePredicate(const D3D11_QUERY_DESC *pPredicateDesc,ID3D11Predicate **ppPredicate);
			long  					CreateCounter(const D3D11_COUNTER_DESC *pCounterDesc,ID3D11Counter **ppCounter);
			long  					CheckFormatSupport(DXGI_FORMAT Format,uint32*pFormatSupport);
			long  					CheckMultisampleQualityLevels(DXGI_FORMAT Format,uint32 SampleCount,uint32*pNumQualityLevels);
			void  					CheckCounterInfo(D3D11_COUNTER_INFO *pCounterInfo);
			long  					CheckCounter(const D3D11_COUNTER_DESC *pDesc,D3D11_COUNTER_TYPE *pType,uint32*pActiveCounters,char* szName,uint32 *pNameLength,char* szUnits,uint32 *pUnitsLength,char* szDescription,uint32 *pDescriptionLength);
			uint32					GetCreationFlags( void);
			long						OpenSharedResource(int hResource,const CRYIID & ReturnedInterface,void **ppResource);

			// DX11 functionality
			ILINE HRESULT Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType,UINT MapFlags,D3D11_MAPPED_SUBRESOURCE *pMappedResource)
			{
				switch(pResource->Type())
				{
				//case EDXPS_RT_TEX1D:
				//		return ((ID3D11Texture1D*)pResource)->Map(Subresource, MapType, MapFlags, pMappedResource);
				case EDXPS_RT_TEX2D:
					return ((ID3D11Texture2D*)pResource)->Map(Subresource, MapType, MapFlags, pMappedResource);
				//case EDXPS_RT_TEX3D:
				//	return ((ID3D11Texture3D*)pResource)->Map(Subresource, MapType, MapFlags, pMappedResource);
				case EDXPS_RT_VERTEXBUFFER:
				case EDXPS_RT_INDEXBUFFER:
				case EDXPS_RT_CONSTBUFFER:
				case EDXPS_RT_CONSTBUFFERTEMP:
					return ((ID3D11Buffer*)pResource)->Map(MapType, MapFlags, (void**)&pMappedResource->pData);
				}
				CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
			}
			ILINE void Unmap(ID3D11Resource *pResource, UINT Subresource)
			{
				switch(pResource->Type())
				{
					//case EDXPS_RT_TEX1D:
					//	((ID3D11Texture1D*)pResource)->Unmap(Subresource);
					//	break;
				case EDXPS_RT_TEX2D:
					((ID3D11Texture2D*)pResource)->Unmap(Subresource);
					break;
					//case EDXPS_RT_TEX3D:
					//	((ID3D11Texture3D*)pResource)->Unmap(Subresource);
					//	break;
				case EDXPS_RT_VERTEXBUFFER:
				case EDXPS_RT_INDEXBUFFER:
				case EDXPS_RT_CONSTBUFFER:
				case EDXPS_RT_CONSTBUFFERTEMP:
					((ID3D11Buffer*)pResource)->Unmap();
					break;
				default:
					CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
				}
			}

			ILINE void Begin(ID3D11Asynchronous *pAsync)
			{
				pAsync->Begin();
			}
			ILINE void End(ID3D11Asynchronous *pAsync)
			{
				pAsync->End();
			}
			ILINE HRESULT GetData(ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags)
			{
				return pAsync->GetData(pData, DataSize, GetDataFlags);
			}

			//PS3 specific extensions
#if !defined(__SPU__)
			ILINE uint8*		AllocAndSetConstantBuffer(uint32 Size,uint32 Slot,bool VertexShaderCB)
											{
												uint8* pMem=m_Thread.ConstantBuffersAllocate(Size,Slot,VertexShaderCB);
												//printf("--- 5 %d\n",(size_t)pMem);
												return pMem;
											}
#endif
			void						FreeMe(){delete this;}
			void  					ClearTexture(CCryDXPSTexture2D* pTex,const uint32 Color);
			void						PSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers,const float* pMin,const float* pMax,const float* pMipBias,uint16* psRGB=0);
			//0 = off
			//1 = 2x
			//2 = 4x
			void						HalfResolution(uint32 Mode,uint32 ATC=0);
			void						PreparePass(uint32 Pass);
			void						ViewProjMatrix(const float* pViewMat);
			void 						PushMarker(const char* pName);
			void 						PopMarker(const char* pName);
			void 						SetMarker(const char* pName);
			void						SwapBuffers(uint32 Frame,APWeakTexture2D pTexBack,APWeakTexture2D pTexFront);
			void						ThreadFinish(){if(m_SPUInitialized)m_Thread.Finish();}
      tdResHandle			DrawCallHandle()
                      {
                          tdResHandle H	= tdLayer0::Sync().HandleRenderer();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
                          H++;
#endif
                          return H;
                      }

			tdResHandle			DrawCallHandleOffset()
											{
												tdResHandle H	=	tdLayer0::Sync().HandleRenderer()-tdLayer0::Sync().FrameBaseRenderer();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
												H++;
#endif
												return H;
											}
			//tdResHandle			DrawCallLastHandle(){return tdLayer0::Sync().HandleRenderer()-1;}
			ILINE void			RegisterZWriteCountForNextDrawCall(const uint32 cZWriteCount, tdResHandle H)
											{
												tdLayer0::Sync().RegisterZWrite(cZWriteCount, H);
											}
			ILINE uint32		ZWriteCount(tdResHandle H,bool WaitForGPU)
											{
												return WaitForGPU?tdLayer0::Sync().ZWriteReport<true>(H, false):
																					tdLayer0::Sync().ZWriteReport<false>(H);
											}
			ILINE uint32		ZWriteCountOffset(tdResHandle H,bool WaitForGPU)
											{
												H	=	H+tdLayer0::Sync().FrameBaseRenderer();
												return WaitForGPU?tdLayer0::Sync().ZWriteReport<true>(H, false):
																					tdLayer0::Sync().ZWriteReport<false>(H);
											}
/*
			tdResHandle			MemCpy(void* pDst,const void* pSrc,uint32 Size,bool Sync)
											{
												CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
#if defined(CRY_DXPS_PERFORMANCECOUNTING)
												rSyncMan.IncRenderer();
#endif
												tdResHandle H=rSyncMan.HandleRenderer();
												rSyncMan.IncRenderer();
												m_Thread.MemCpy(pDst,pSrc,Size);
												return (!Sync ||rSyncMan.ZWriteReport<true>(H))?H:0;
											}
			tdResHandle			MemCpy(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox,bool Sync)
											{
//												tdResHandle H=tdLayer0::Sync().HandleRenderer();
	//											tdLayer0::Sync().IncRenderer();
	//											m_Thread.MemCpyScaled2D(pDstResource,DstSubresource,DstX,DstY,DstZ,pSrcResource,SrcSubresource,pSrcBox);
	//											return (!Sync ||tdLayer0::Sync().ZWriteReport<true>(H))?H:0;
												return 0;
											}
*/
			void						ResetRSX();


			void GetImmediateContext(CCryDXPSRenderDevice **ppImmediateContext) { if(ppImmediateContext) { *ppImmediateContext = this; } }
			//SPU
#if !defined(__SPU__)
			ILINE NPPU::SFrameProfileRSXData& GetFrameStatsSPUThread()
											{
												return m_Thread.GetFrameStatsSPUThread();
											}
#endif

#ifdef CRY_USE_GCM
		void														UpdateRendertargets();
#endif
#ifdef DRAWCALLDEBUGGING
		void														ResetDrawCounter(){m_DrawCounter=0;}
#endif
#if defined(CRY_DXPS_RASTERTHREAD)
		CDXPSRDRasterizer&							SoftRast()
																		{
																			return m_SoftRast;
																		}
#endif
		void														Size(class ICrySizer* Sizer);

};

#ifndef DO_RENDERLOG
	typedef CCryDXPSRenderDevice ID3D11Device;
	typedef CCryDXPSRenderDevice ID3D11DeviceContext;
#endif

#ifdef CRY_DXPS_VALIDATEWEAKPTR
	typedef CCryAPtrRefCnt<ID3D11Device> LPD3D10DEVICE;
#else
	typedef ID3D11Device* LPD3D10DEVICE;
#endif

#ifndef __SPU__

ILINE void CCryDXPSRenderDevice::IASetVertexBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppVertexBuffers,const uint32*pStrides,const uint32*pOffsets)
{
	for(uint32 a=0;a<NumBuffers;a++)
	{
		ID3D11Buffer* pBuffer	=	ppVertexBuffers[a];
#if defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
		m_pVertexBuffer[Offset+a]->DCLockTo(tdLayer0::Sync().HandleRenderer()-1);
		if(pBuffer)
		{
			pBuffer->DCLockTo(TDRES_MAX);
			m_pVertexBuffer[Offset+a]	=	pBuffer;
			m_pVBStride[Offset+a]			=	pStrides[a];
			m_pVBOffset[Offset+a]			=	pOffsets[a];
			m_Thread.VertexBuffers(Offset+a,pBuffer,pStrides[a],pOffsets[a]);
		}
		else
			m_pVertexBuffer[Offset+a]	=	m_pIndexBufferDraw;
#else
		if(pBuffer)//no need to change buffer to 0 on devicethread side
		{
			if(m_LockHandleIdx[Offset+a]==~0)
			{
				m_LockHandleIdx[Offset+a]	=	m_LockHandleCount;
				m_LockHandleIdxRev[m_LockHandleCount]	=	Offset+a;
				m_LockHandleCount++;
			}
			m_pLockHandles[m_LockHandleIdx[Offset+a]]	=	pBuffer->LockAddress();

			m_pVertexBuffer[Offset+a]	=	pBuffer;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
			m_pVBStride[Offset+a]			=	pStrides[a];
			m_pVBOffset[Offset+a]			=	pOffsets[a];
#endif
			m_Thread.VertexBuffers(Offset+a,pBuffer,pStrides[a],pOffsets[a]);
		}
		else
		{
			if(m_LockHandleIdx[Offset+a]!=~0)
			{
				m_LockHandleCount--;
				m_LockHandleIdx[m_LockHandleIdxRev[m_LockHandleCount]]	=	m_LockHandleIdx[Offset+a];
				m_LockHandleIdxRev[m_LockHandleIdx[Offset+a]]						=	m_LockHandleIdxRev[m_LockHandleCount];
				m_LockHandleIdx[Offset+a]	=	~0;
			}
			m_pVertexBuffer[Offset+a]	=	m_pIndexBufferDraw;
		}
#endif
	}
}

ILINE void CCryDXPSRenderDevice::ResetBuffer(const ID3D11Buffer* pVertexBuffer)
{
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	for(uint32 a=0;a<16;a++)
		if(pVertexBuffer==m_pVertexBuffer[a])
			m_Thread.VertexBuffers(a,m_pVertexBuffer[a],m_pVBStride[a],m_pVBOffset[a]);
#endif
}

ILINE void CCryDXPSRenderDevice::VSSetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers)
{
#ifdef DO_RENDERLOG
  VSSetConstantBuffers_Log(Offset, NumBuffers, ppConstantBuffers);
#endif
	for(uint32 a=0;a<NumBuffers && a+Offset<16;a++)
	{
		ID3D11Buffer* pBuffer	=	ppConstantBuffers[a];
#if defined(CRY_DXPS_LSCONSTANTS)
		if(pBuffer->Type()==EDXPS_RT_CONSTBUFFERTEMP)
			continue;
#endif
#if defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
		m_pVSConstBuffer[Offset+a]->DCLockTo(tdLayer0::Sync().HandleRenderer()-1);
		if(pBuffer)
		{
			pBuffer->DCLockTo(TDRES_MAX);
			m_pVSConstBuffer[Offset+a]	=	pBuffer;
		}
		else
			m_pVSConstBuffer[Offset+a]	=	m_pIndexBufferDraw;
#else
		if(pBuffer)
		{
			if(m_LockHandleIdx[Offset+a+16]==~0)
			{
				m_LockHandleIdx[Offset+a+16]	=	m_LockHandleCount;
				m_LockHandleIdxRev[m_LockHandleCount]	=	Offset+a+16;
				m_LockHandleCount++;
			}
			m_pLockHandles[m_LockHandleIdx[Offset+a+16]]	=	pBuffer->LockAddress();

			m_pVSConstBuffer[Offset+a]	=	pBuffer;
		}
		else
		{
			if(m_LockHandleIdx[Offset+a+16]!=~0)
			{
				m_LockHandleCount--;
				m_LockHandleIdx[m_LockHandleIdxRev[m_LockHandleCount]]	=	m_LockHandleIdx[Offset+a+16];
				m_LockHandleIdxRev[m_LockHandleIdx[Offset+a+16]]				=	m_LockHandleIdxRev[m_LockHandleCount];
				m_LockHandleIdx[Offset+a+16]	=	~0;
			}
		}
#endif
//		m_pVSConstBuffer[Offset+a]	=	pBuffer?pBuffer:m_pIndexBufferDraw;
		m_Thread.VSConstantBuffers(Offset+a,pBuffer);
	}
}

ILINE void CCryDXPSRenderDevice::PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	if(m_pLastPixelShader!=pPixelShader)
	{
		m_pLastPixelShader=pPixelShader;
		pPixelShader->DCLockTo(tdLayer0::Sync().HandleRenderer());
		m_Thread.PSShader(pPixelShader);
	}
}

ILINE void CCryDXPSRenderDevice::PSSetConstantBuffers(uint32 Offset,uint32 NumBuffers,ID3D11Buffer *const *ppConstantBuffers)
{
#ifdef DO_RENDERLOG
  PSSetConstantBuffers_Log(Offset, NumBuffers, ppConstantBuffers);
#endif
	for(uint32 a=0;a<NumBuffers && a+Offset<16;a++)
	{
		ID3D11Buffer* pBuffer	=	ppConstantBuffers[a];
#if defined(CRY_DXPS_LSCONSTANTS)
		if(pBuffer->Type()==EDXPS_RT_CONSTBUFFERTEMP)
			continue;
#endif
#if defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
		m_pPSConstBuffer[Offset+a]->DCLockTo(tdLayer0::Sync().HandleRenderer()-1);
		if(pBuffer)
		{
			pBuffer->DCLockTo(TDRES_MAX);
			m_pPSConstBuffer[Offset+a]	=	pBuffer;
		}
		else
		{
			m_pPSConstBuffer[Offset+a]	=	m_pIndexBufferDraw;
		}
#else
		if(pBuffer)
		{
			if(m_LockHandleIdx[Offset+a+32]==~0)
			{
				m_LockHandleIdx[Offset+a+32]	=	m_LockHandleCount;
				m_LockHandleIdxRev[m_LockHandleCount]	=	Offset+a+32;
				m_LockHandleCount++;
			}
			m_pLockHandles[m_LockHandleIdx[Offset+a+32]]	=	pBuffer->LockAddress();

			m_pPSConstBuffer[Offset+a]	=	pBuffer;
		}
		else
		{
			if(m_LockHandleIdx[Offset+a+32]!=~0)
			{
				m_LockHandleCount--;
				m_LockHandleIdx[m_LockHandleIdxRev[m_LockHandleCount]]	=	m_LockHandleIdx[Offset+a+32];
				m_LockHandleIdxRev[m_LockHandleIdx[Offset+a+32]]				=	m_LockHandleIdxRev[m_LockHandleCount];
				m_LockHandleIdx[Offset+a+32]	=	~0;
			}
		}
#endif
//		m_pPSConstBuffer[Offset+a]	=	pBuffer?pBuffer:m_pIndexBufferDraw;
		m_Thread.PSConstantBuffers(Offset+a,pBuffer);
	}
}

ILINE void CCryDXPSRenderDevice::IASetInputLayout(ID3D11InputLayout *pInputLayout)
{
	m_pInputLayout	=	pInputLayout;
	pInputLayout->DCLockTo(tdLayer0::Sync().HandleRenderer());
	m_Thread.InputLayout(pInputLayout);
}

ILINE void CCryDXPSRenderDevice::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer,DXGI_FORMAT Format,uint32 Offset)
{
	CRY_ASSERT_MESSAGE(Offset==0,"Not implemented yet!");
//	if(pIndexBuffer)//no need to change buffer to 0 on devicethread side
//		m_Thread.IndexBuffer(pIndexBuffer,Format,Offset);
#if defined(CRY_DXPS_RESOURCE_OWNERSHIP_LOCK)
		m_pIndexBuffer->DCLockTo(tdLayer0::Sync().HandleRenderer()-1);
		if(pIndexBuffer)
		{
			pIndexBuffer->DCLockTo(TDRES_MAX);
			m_pIndexBuffer=	pIndexBuffer;
		}
		else
			m_pIndexBuffer=	m_pIndexBufferDraw;
#else
	m_pIndexBuffer=	pIndexBuffer;
#endif

	m_IBFormat		=	Format==DXGI_FORMAT_R16_UINT?	CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
																								CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32;
	m_IBOffset		=	Offset;
}

ILINE void CCryDXPSRenderDevice::OMSetBlendState(ID3D11BlendState *pBlendState,const float BlendFactor[4],uint32 SampleMask)
{
	m_pBlendState			=	pBlendState;
	if(BlendFactor)
	{
		m_BlendFactor[0]	=	BlendFactor[0];
		m_BlendFactor[1]	=	BlendFactor[1];
		m_BlendFactor[2]	=	BlendFactor[2];
		m_BlendFactor[3]	=	BlendFactor[3];
	}
	m_Thread.BlendState(pBlendState,BlendFactor);
}

ILINE void CCryDXPSRenderDevice::OMSetDepthStencilState(ID3D11DepthStencilState *pDepthStencilState,uint32 StencilRef)
{
	m_pDepthStencilState	=	pDepthStencilState;
	m_StencilRef					=	StencilRef;
	m_Thread.DepthStencilState(pDepthStencilState,StencilRef);
}

ILINE void CCryDXPSRenderDevice::RSSetScissorRects(uint32 NumRects,const D3D11_RECT *pRects)
{
	//  cellGcmSetScissor( pRects[0].left, pRects[0].top, pRects[0].right, pRects[0].bottom );
	m_Thread.ScissorRect(pRects[0].left, pRects[0].top, pRects[0].right, pRects[0].bottom);
}

ILINE void CCryDXPSRenderDevice::RSSetDepthBounds(uint32 Enable,const float ZMin,const float ZMax)
{	
	//  cellGcmSetScissor( pRects[0].left, pRects[0].top, pRects[0].right, pRects[0].bottom );
	m_Thread.DepthBounds(Enable,ZMin,ZMax);
}

ILINE void CCryDXPSRenderDevice::ClearRenderTargetView( ID3D11RenderTargetView *pRenderTargetView,const float ColorRGBA[ 4 ])
{
	if(!m_ClearableFormat)
	{
		CRY_ASSERT_MESSAGE(0,"Not clearable format!");
		return;
	}

	m_Thread.ClearRenderTarget(ColorRGBA);

}

ILINE void CCryDXPSRenderDevice::ClearTexture(CCryDXPSTexture2D* pTex,const uint32 Color)
{
//	CCryDXPSGCMSyncMan& __restrict rSyncMan = tdLayer0::Sync();
//#if defined(CRY_DXPS_PERFORMANCECOUNTING)
//	rSyncMan.IncRenderer();
//#endif
//	rSyncMan.IncRenderer();
//	m_Thread.ClearTexture(pTex,Color);
}

ILINE void CCryDXPSRenderDevice::ClearDepthStencilView( ID3D11DepthStencilView *pDepthStencilView,uint32 ClearFlags,float Depth,uint8 Stencil)
{
	m_Thread.ClearDepthStencil(ClearFlags,Depth,Stencil);

}

ILINE void CCryDXPSRenderDevice::PreparePass(uint32 Pass)
{
	m_Thread.PreparePass(Pass,m_ViewProjMat);
}

ILINE void CCryDXPSRenderDevice::PushMarker(const char* pName)
{
	m_Thread.PushMarker(pName);
}

ILINE void CCryDXPSRenderDevice::PopMarker(const char* pName)
{
	m_Thread.PopMarker(pName);
}

ILINE void CCryDXPSRenderDevice::SetMarker(const char* pName)
{
	m_Thread.SetMarker(pName);
}

ILINE long CCryDXPSRenderDevice::CreateBuffer(const D3D11_BUFFER_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Buffer **ppBuffer)
{
	ECRYDXPSResType Type=EDXPS_RT_UNUSED;
	ECRYDXPSResFreq	Freq=pDesc->Usage==D3D11_USAGE_DYNAMIC?EDXPS_RF_DYNAMIC:EDXPS_RF_STATIC;

	if(pDesc->BindFlags==D3D11_BIND_VERTEX_BUFFER)
		Type	=	EDXPS_RT_VERTEXBUFFER;
	else
	if(pDesc->BindFlags==D3D11_BIND_INDEX_BUFFER)
		Type	=	EDXPS_RT_INDEXBUFFER;
	else
	if(pDesc->BindFlags==D3D11_BIND_CONSTANT_BUFFER)
		Type	=	EDXPS_RT_CONSTBUFFER;

	*ppBuffer	=	CRY_DXPS_CREATE(CCryDXPSBuffer,(pInitialData?pInitialData->pSysMem:0,pDesc->ByteWidth,Type,Freq));
	return 0;
}

ILINE long CCryDXPSRenderDevice::CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc,const D3D11_SUBRESOURCE_DATA *pInitialData,ID3D11Texture1D **ppTexture)
{
	*ppTexture	=	CRY_DXPS_CREATE(CCryDXPSTexture1D,(	pDesc->Width,
		pDesc->MipLevels,
		pDesc->Format));
	return (*ppTexture!=0)-1;
}

ILINE long CCryDXPSRenderDevice::CreateShaderResourceView( CCryDXPSResource *pResource,const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,ID3D11ShaderResourceView **ppSRView)
{
	*ppSRView	=	CRY_DXPS_CREATE(CCryDXPSShaderResourceView,(pResource,pDesc));
	return (*ppSRView!=0)-1;
}

ILINE long CCryDXPSRenderDevice::CreateRenderTargetView(ID3D11Resource *pResource,const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,ID3D11RenderTargetView **ppRTView)
{
	//	assert(!pDesc);
	assert(pResource->Type()==EDXPS_RT_TEX2D);
	if(/*!pDesc &&*/ pResource->Type()==EDXPS_RT_TEX2D)
	{
		CCryDXPSTexture2D* pTex	=	reinterpret_cast<CCryDXPSTexture2D*>(pResource);
		(*ppRTView)	=	CRY_DXPS_CREATE(CCryDXPSRenderTargetView,(pTex,pDesc?pDesc->TilingInfo:DXPSRTVDTI_AUTO));
		return 0;
	}
	return -1;
}

ILINE long CCryDXPSRenderDevice::CreateDepthStencilView(ID3D11Resource *pResource,const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,ID3D11DepthStencilView **ppDepthStencilView)
{
	*ppDepthStencilView 	=	pResource->Type()==EDXPS_RT_TEX2D?CRY_DXPS_CREATE(CCryDXPSDepthStencilView,(reinterpret_cast<CCryDXPSTexture2D*>(pResource))):0;
	return (ppDepthStencilView!=0)-1;	
}

ILINE HRESULT	CCryDXPSRenderDevice::QueryInterface(REFIID riid,void **ppvObject)
{
	CRY_ASSERT_MESSAGE(0,"Not implemented yet!");
	return -1;
}

ILINE long CCryDXPSRenderDevice::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,uint32 NumElements,const void *pShaderBytecodeWithInputSignature,SIZE_T Size,ID3D11InputLayout **ppInputLayout)
{
	(*ppInputLayout)	=	CRY_DXPS_CREATE(CCryDXPSInputLayout,(pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature));
	return 0;
}

ILINE long CCryDXPSRenderDevice::CreateVertexShader(const void *pShaderBytecode,SIZE_T Size,ID3D11ClassLinkage *pClassLinkage,ID3D11VertexShader **ppVertexShader)
{
	//	CRY_DEBUGOUT("ByteCodePtr: %x\n",(uint32)pShaderBytecode);
	*ppVertexShader	=	CRY_DXPS_CREATE(CCryDXPSShader,(pShaderBytecode,EDXPS_ST_VERTEX));
	return 0;
}

ILINE long CCryDXPSRenderDevice::CreatePixelShader(const void *pShaderBytecode,SIZE_T Size,ID3D11ClassLinkage *pClassLinkage,ID3D11PixelShader **ppPixelShader)
{
	*ppPixelShader	=	CRY_DXPS_CREATE(CCryDXPSShader,(pShaderBytecode,EDXPS_ST_PIXEL));
	return 0;
}

ILINE long CCryDXPSRenderDevice::CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc,ID3D11BlendState **ppBlendState)
{
	*ppBlendState	=	CRY_DXPS_CREATE(CCryDXPSBlendState,(pBlendStateDesc));
	return 0;
}

ILINE long CCryDXPSRenderDevice::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,ID3D11DepthStencilState **ppDepthStencilState)
{
	*ppDepthStencilState	=	CRY_DXPS_CREATE(CCryDXPSDepthStencilState,(pDepthStencilDesc));
	return 0;
}

ILINE long CCryDXPSRenderDevice::CreateRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc,ID3D11RasterizerState **ppRasterizerState)
{
	*ppRasterizerState	=	CRY_DXPS_CREATE(CCryDXPSRasterizerState,(pRasterizerDesc));

	return 0;
}

ILINE long CCryDXPSRenderDevice::CreateSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc,ID3D11SamplerState **ppSamplerState)
{
	*ppSamplerState 	=	CRY_DXPS_CREATE(CCryDXPSSamplerState,(pSamplerDesc));
	return ppSamplerState?0:-1;	
}

ILINE void CCryDXPSRenderDevice::PointSpriteControl(const uint32 Enable, const uint32 RMode, const uint32 Texcoord)
{
	m_Thread.PointSpriteControl(Enable, RMode, Texcoord);
}

ILINE void CCryDXPSRenderDevice::RSSetState(ID3D11RasterizerState *pRasterizerState)
{
	RSSetState(pRasterizerState,0,0);
}

ILINE void CCryDXPSRenderDevice::RSSetState(ID3D11RasterizerState *pRasterizerState,uint32 sRGB,uint32 AlphaTestRef)
{
	m_pRasterizerState	=	pRasterizerState;
	m_RasterizerSRGB		=	sRGB;
	m_Thread.RasterizerState(pRasterizerState,sRGB,AlphaTestRef);
}

ILINE void CCryDXPSRenderDevice::VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	if(m_pLastVertexShader!=pVertexShader)
	{
		m_pLastVertexShader=pVertexShader;
		pVertexShader->DCLockTo(tdLayer0::Sync().HandleRenderer());
		m_Thread.VSShader(pVertexShader);
	}
}

ILINE void CCryDXPSRenderDevice::EndScene(){}

ILINE void CCryDXPSRenderDevice::PSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers,const float* pMipBias,uint16* psRGB)
{
	for(uint32 a=0;a<NumSamplers;a++)
	{
		const uint32 Offseta	=	Offset+a;
		m_pSampler[Offseta]						=		ppSamplers[a];
		m_pSamplerSRGB[Offseta]				=		psRGB[a];
		m_NotPointSampler[Offseta]		=		ppSamplers[a]->FilterMin()!=CELL_GCM_TEXTURE_NEAREST_NEAREST |
																			ppSamplers[a]->FilterMag()!=CELL_GCM_TEXTURE_NEAREST;
		m_InvalidFilteringFlag				=		((m_NotPointSampler[Offseta]&m_PointTexture[Offseta])<<Offseta)|
																			(m_InvalidFilteringFlag&(~(1<<Offseta)));
		m_InvalidSRGBFlag							=		((m_pSamplerSRGB[Offseta]&m_NoSRGBTexture[Offseta])<<Offseta)|
																			(m_InvalidSRGBFlag&(~(1<<Offseta)));
		m_SamplerDirty								|=	1<<Offseta;


		m_pSamplerMipBias[Offseta]		= static_cast<int16>((pMipBias[a])*static_cast<f32>(1<<8));
		CRY_ASSERT_MESSAGE(m_pSamplerMipBias[Offseta]<(1<<12),"m_pSamplerMipBias < (1<<12)");
		//m_Thread.Sampler(Offseta,ppSamplers[a],m_SamplerLODMin[Offseta],m_SamplerLODMax[Offseta],psRGB?psRGB[a]:0);
	}
}

ILINE void CCryDXPSRenderDevice::PSSetSamplers(uint32 Offset,uint32 NumSamplers,ID3D11SamplerState *const *ppSamplers,const float* pMin,const float* pMax,const float* pMipBias,uint16* psRGB)
{
	for(uint32 a=0;a<NumSamplers;a++)
	{
		const uint32 Offseta	=	Offset+a;
		m_pSampler[Offseta]						=		ppSamplers[a];
		m_pSamplerSRGB[Offseta]				=		psRGB[a];
		m_NotPointSampler[Offseta]		=		ppSamplers[a]->FilterMin()!=CELL_GCM_TEXTURE_NEAREST_NEAREST |
																			ppSamplers[a]->FilterMag()!=CELL_GCM_TEXTURE_NEAREST;
		m_InvalidFilteringFlag				=		((m_NotPointSampler[Offseta]&m_PointTexture[Offseta])<<Offseta)|
																			(m_InvalidFilteringFlag&(~(1<<Offseta)));
		m_InvalidSRGBFlag							=		((m_pSamplerSRGB[Offseta]&m_NoSRGBTexture[Offseta])<<Offseta)|
																			(m_InvalidSRGBFlag&(~(1<<Offseta)));

		m_SamplerDirty								|=	1<<Offseta;
		m_SamplerLODMin[Offseta]			=		static_cast<int16>((pMin[a]>=0.f?pMin[a]:0.f)*static_cast<f32>(1<<8));
		m_SamplerLODMax[Offseta]			=		static_cast<int16>((pMax[a]<=12.f?pMax[a]:12.f)*static_cast<f32>(1<<8));
		
		m_pSamplerMipBias[Offseta]		= static_cast<int16>((pMipBias[a])*static_cast<f32>(1<<8));
		CRY_ASSERT_MESSAGE(m_pSamplerMipBias[Offseta]<=(1<<12),"|MipBias| > 16");
		//m_Thread.Sampler(Offseta,ppSamplers[a],SamplerLODMin,SamplerLODMax,psRGB?psRGB[a]:0);
	}
}

ILINE void CCryDXPSRenderDevice::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
	static const 	uint32 g_Prims[]	=
	{
		~0,																	//	D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED					=	0,
		CELL_GCM_PRIMITIVE_POINTS,					//	D3D11_PRIMITIVE_TOPOLOGY_POINTLIST					=	1,
		CELL_GCM_PRIMITIVE_LINES,						//	D3D11_PRIMITIVE_TOPOLOGY_LINELIST						=	2,
		CELL_GCM_PRIMITIVE_LINE_STRIP,				//	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP					=	3,
		CELL_GCM_PRIMITIVE_TRIANGLES,				//	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST				=	4,
		CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,	//	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP			=	5,
		~0,
		~0,
		~0,
		~0,
		~0,																	//	D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ				=	10,
		~0,																	//	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ			=	11,
		~0,																	//	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ		=	12,
		~0																	//	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ	=	13,
	};
	m_Thread.Topology(g_Prims[Topology]);
}

#endif//__SPU__
#endif
