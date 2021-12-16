/*=============================================================================
DriverD3D9.h : Direct3D8 Render interface declarations.
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honich Andrey

=============================================================================*/

#ifndef DRIVERD3D9_H
#define DRIVERD3D9_H

#if _MSC_VER > 1000
# pragma once 
#endif

/*
===========================================
The DXRenderer interface Class
===========================================
*/
void AddPhysicalD3DBlock(long size);

_inline int _VertBufferSize(D3DVertexBuffer *pVB)
{
	if (!pVB)
		return 0;
#if defined(DIRECT3D9)
	D3DVERTEXBUFFER_DESC Desc;
	pVB->GetDesc(&Desc);
	return Desc.Size;
#elif defined(DIRECT3D10)
	D3D11_BUFFER_DESC Desc;
	pVB->GetDesc(&Desc);
	return Desc.ByteWidth;
#endif
	return 0;
}
_inline int _IndexBufferSize(D3DIndexBuffer *pIB)
{
	if (!pIB)
		return 0;
#if defined(DIRECT3D9)
	D3DINDEXBUFFER_DESC Desc;
	pIB->GetDesc(&Desc);
	return Desc.Size;
#elif defined(DIRECT3D10)
	D3D11_BUFFER_DESC Desc;
	pIB->GetDesc(&Desc);
	return Desc.ByteWidth;
#endif
	return 0;
}

#if !defined(XENON) && !defined(PS3)
# if defined(DIRECT3D9)
#  include "DXUT/DXUT.h"
#  include "DXUT/DXUTenum.h"
#  include "DXUT/DXUTmisc.h"
# elif defined(DIRECT3D10)
#  include "DXUT11/DXUT.h"
#  include "DXUT11/DXUTmisc.h"
#  include "DXUT11/DXUTDevice11.h"
# endif
#endif

//=======================================================================

// DRIVERD3D.H
// CRender3D Direct3D rasterizer class.

#define VERSION_D3D 2.0

#define DECLARE_INITED(typ,var) typ var; memset(&var,0,sizeof(var)); var.dwSize=sizeof(var);
#define SAFETRY(cmd) {try{cmd;}catch(...){ShError("Exception in '%s'\n", #cmd);}}
#define DX_RELEASE(x) { if(x) { (x)->Release(); (x) = NULL; } }

//#define SWITCH_RENDERTARGET_FOR_ATAA	//if enabled, it switches the rendertarget for the general shaders, if off, it just alters the channel write mask

struct SPixFormat;
class CRender3DD3D;
struct SDeviceInfo;

struct SREPointSpriteCreateParams;
struct SPointSpriteVertex;

//=======================================================================

#include "D3DSettings.h"
#include "D3DTexture.h"
#include "D3DRenderAuxGeom.h"
#include "D3DColorGradingController.h"
#include "ShadowTextureGroupManager.h"		// CShadowTextureGroupManager
#include "D3DDeferredShading.h"

#if defined(DIRECT3D9)

class CMyDirect3DDevice9;

#if !defined(XENON) && !defined(PS3)
#define TEXPOOL D3DPOOL_MANAGED


#endif

#if !defined(XENON)
class CMyDirect3DDevice9 : public IDirect3DDevice9


#endif
{
	/*** IUnknown methods ***/
#ifndef OPENGL
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG,AddRef)(THIS);
	STDMETHOD_(ULONG,Release)(THIS);
#endif

	/*** IDirect3DDevice9 methods ***/
	STDMETHOD(TestCooperativeLevel)(THIS);
	STDMETHOD_(UINT, GetAvailableTextureMem)(THIS);
	STDMETHOD(EvictManagedResources)(THIS);
	STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9);
	STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps);
	STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode);
	STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters);
	STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap);
#if !defined(XENON) && !defined(PS3)
	STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags);
#endif
	STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow);

	// Different for XBox DirectX
#ifndef _XBOX
	STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
	STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
#endif

	STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters);
	STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus);
	STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);
	STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp);
	STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle);
	STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle);
	STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle);
	STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle);
	STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle);
	STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);
	STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,IDirect3DSurface9* pDestSurface);
	STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);
	STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color);
	STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget);
	STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget);
	STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil);
	STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface);
	STDMETHOD(BeginScene)(THIS);
	STDMETHOD(EndScene)(THIS);
	STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
	STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
	STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport);
	STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport);
	STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane);
	STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane);
	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value);
	STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue);
	STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB);
	STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture);
	STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture);
	STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
	STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue);
	STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
	STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect);
	STDMETHOD(GetScissorRect)(THIS_ RECT* pRect);
	STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
	STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
	STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl);
	STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl);
	STDMETHOD(SetFVF)(THIS_ DWORD FVF);
	STDMETHOD(GetFVF)(THIS_ DWORD* pFVF);
	STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader);
	STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader);
	STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader);
	STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);
	STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* OffsetInBytes,UINT* pStride);
	STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Divider);
	STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* Divider);
	STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData);
	STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData);
	STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader);
	STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader);
	STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader);
	STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);
	STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs);
	STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS);
	STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint);
	STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture);
	STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix);
	STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*);
	STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial);
	STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial);
	STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9*);
	STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9*);
	STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable);
	STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable);
	STDMETHOD(BeginStateBlock)(THIS);
	STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB);
	STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus);
	STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus);
	STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue);
	STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses);
	STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries);
	STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries);
	STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber);
	STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber);
	STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware);
	STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS);
	STDMETHOD(SetNPatchMode)(THIS_ float nSegments);
	STDMETHOD_(float, GetNPatchMode)(THIS);
	STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags);
	STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo);
	STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo);
	STDMETHOD(DeletePatch)(THIS_ UINT Handle);
	STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer);

};

#elif defined (DIRECT3D10)


interface CMyDirect3DDevice11 : ID3D11Device
{
public:
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG,AddRef)(THIS);
	STDMETHOD_(ULONG,Release)(THIS);

	virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason( void) ;

	virtual HRESULT STDMETHODCALLTYPE SetExceptionMode( 
		UINT RaiseFlags) ;

	virtual UINT STDMETHODCALLTYPE GetExceptionMode( void) ;

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
		/* [in] */ REFGUID guid,
		/* [out][in] */ UINT *pDataSize,
		/* [size_is][out] */ void *pData) ;

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
		/* [in] */ REFGUID guid,
		/* [in] */ UINT DataSize,
		/* [size_is][in] */ const void *pData) ;

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
		/* [in] */ REFGUID guid,
		/* [in] */ const IUnknown *pData) ;

	virtual HRESULT STDMETHODCALLTYPE CreateBuffer( 
		/* [in] */ const D3D11_BUFFER_DESC *pDesc,
		/* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
		/* [out] */ ID3D11Buffer **ppBuffer) ;

	virtual HRESULT STDMETHODCALLTYPE CreateTexture1D( 
		/* [in] */ const D3D11_TEXTURE1D_DESC *pDesc,
		/* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
		/* [out] */ ID3D11Texture1D **ppTexture1D) ;

	virtual HRESULT STDMETHODCALLTYPE CreateTexture2D( 
		/* [in] */ const D3D11_TEXTURE2D_DESC *pDesc,
		/* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
		/* [out] */ ID3D11Texture2D **ppTexture2D) ;

	virtual HRESULT STDMETHODCALLTYPE CreateTexture3D( 
		/* [in] */ const D3D11_TEXTURE3D_DESC *pDesc,
		/* [in] */ const D3D11_SUBRESOURCE_DATA *pInitialData,
		/* [out] */ ID3D11Texture3D **ppTexture3D) ;

	virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView( 
		/* [in] */ ID3D11Resource *pResource,
		/* [in] */ const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
		/* [out] */ ID3D11ShaderResourceView **ppSRView) ;

	virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView( 
		/* [in] */ ID3D11Resource *pResource,
		/* [in] */ const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
		/* [out] */ ID3D11RenderTargetView **ppRTView) ;

	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView( 
		/* [in] */ ID3D11Resource *pResource,
		/* [in] */ const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
		/* [out] */ ID3D11DepthStencilView **ppDepthStencilView) ;

	virtual HRESULT STDMETHODCALLTYPE CreateInputLayout( 
		/* [size_is][in] */ const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
		/* [in] */ UINT NumElements,
		/* [in] */ const void *pShaderBytecodeWithInputSignature,
		/* [in] */ SIZE_T BytecodeLength,
		/* [out] */ ID3D11InputLayout **ppInputLayout) ;

	virtual HRESULT STDMETHODCALLTYPE CreateVertexShader( 
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11VertexShader **ppVertexShader) ;

	virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader( 
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11GeometryShader **ppGeometryShader) ;

	virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput( 
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [size_is][in] */ const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
		/* [in] */ UINT NumEntries,
		/* [in] */ const UINT *pBufferStrides,
		/* [in] */ UINT NumStrides,
		/* [in] */ UINT RasterizedStream,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11GeometryShader **ppGeometryShader) ;

	virtual HRESULT STDMETHODCALLTYPE CreatePixelShader( 
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11PixelShader **ppPixelShader) ;

	virtual HRESULT STDMETHODCALLTYPE CreateBlendState( 
		/* [in] */ const D3D11_BLEND_DESC *pBlendStateDesc,
		/* [out] */ ID3D11BlendState **ppBlendState) ;

	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState( 
		/* [in] */ const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
		/* [out] */ ID3D11DepthStencilState **ppDepthStencilState) ;

	virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState( 
		/* [in] */ const D3D11_RASTERIZER_DESC *pRasterizerDesc,
		/* [out] */ ID3D11RasterizerState **ppRasterizerState) ;

	virtual HRESULT STDMETHODCALLTYPE CreateSamplerState( 
		/* [in] */ const D3D11_SAMPLER_DESC *pSamplerDesc,
		/* [out] */ ID3D11SamplerState **ppSamplerState) ;

	virtual HRESULT STDMETHODCALLTYPE CreateQuery( 
		/* [in] */ const D3D11_QUERY_DESC *pQueryDesc,
		/* [out] */ ID3D11Query **ppQuery) ;
#ifndef PS3 
	virtual HRESULT STDMETHODCALLTYPE CreatePredicate( 
		/* [in] */ const D3D11_QUERY_DESC *pPredicateDesc,
		/* [out] */ ID3D11Predicate **ppPredicate) ;

	virtual HRESULT STDMETHODCALLTYPE CreateCounter( 
		/* [in] */ const D3D11_COUNTER_DESC *pCounterDesc,
		/* [out] */ ID3D11Counter **ppCounter) ;

	virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(
		/* [in] */ ID3D11Resource *pResource,
		/* [in] */ const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
		/* [out] */ ID3D11UnorderedAccessView **ppUAView
		);
	virtual HRESULT STDMETHODCALLTYPE CreateHullShader(
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11HullShader **ppHullShader
		);
	virtual HRESULT STDMETHODCALLTYPE CreateDomainShader(
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11DomainShader **ppDomainShader
		);
	virtual HRESULT STDMETHODCALLTYPE CreateComputeShader(
		/* [in] */ const void *pShaderBytecode,
		/* [in] */ SIZE_T BytecodeLength,
		/* [in] */ ID3D11ClassLinkage *pClassLinkage,
		/* [out] */ ID3D11ComputeShader **ppComputeShader
		);
	virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(
		/* [out] */ ID3D11ClassLinkage **ppLinkage
		);
	virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext(
		/* [in] */ UINT ContextFlags,
		/* [out] */ ID3D11DeviceContext **ppDeferredContext
		);
	virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
		/* [in] */ D3D11_FEATURE Feature,
		/* [in] */ void *pFeatureSupportData,
		/* [in] */ UINT FeatureSupportDataSize
		);
	virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel();
	virtual void STDMETHODCALLTYPE GetImmediateContext(
		/* [out] */ ID3D11DeviceContext **ppImmediateContext
		);
#endif
	virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport( 
		/* [in] */ DXGI_FORMAT Format,
		/* [retval][out] */ UINT *pFormatSupport) ;

	virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels( 
		DXGI_FORMAT Format,
		/* [in] */ UINT SampleCount,
		/* [retval][out] */ UINT *pNumQualityLevels) ;

	virtual void STDMETHODCALLTYPE CheckCounterInfo( 
		/* [retval][out] */ D3D11_COUNTER_INFO *pCounterInfo) ;

	virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(
		HANDLE hResource,
		REFIID ReturnedInterface,
		void ** ppResource
		);

	virtual HRESULT STDMETHODCALLTYPE CheckCounter( 
		__in  const D3D11_COUNTER_DESC *pDesc,
		__out  D3D11_COUNTER_TYPE *pType,
		__out  UINT *pActiveCounters,
		__out_ecount_opt(*pNameLength)  LPSTR wszName,
		__inout_opt  UINT *pNameLength,
		__out_ecount_opt(*pUnitsLength)  LPSTR wszUnits,
		__inout_opt  UINT *pUnitsLength,
		__out_ecount_opt(*pDescriptionLength)  LPSTR wszDescription,
		__inout_opt  UINT *pDescriptionLength) ;

	virtual UINT STDMETHODCALLTYPE GetCreationFlags();
};

interface CMyDirect3DDeviceContext11 : ID3D11DeviceContext
{
public:
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG,AddRef)(THIS);
	STDMETHOD_(ULONG,Release)(THIS);




	virtual void STDMETHODCALLTYPE GetDevice( 
		/* [out] */  ID3D11Device **ppDevice);

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
		/* [in] */  REFGUID guid,
		/* [inout] */  UINT *pDataSize,
		/* [out_opt] */  void *pData);

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
		/* [in] */  REFGUID guid,
		/* [in] */  UINT DataSize,
		/* [in_opt] */  const void *pData);

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
		/* [in] */  REFGUID guid,
		/* [in_opt] */  const IUnknown *pData);

	virtual HRESULT STDMETHODCALLTYPE Map( 
		/* [in] */  ID3D11Resource *pResource,
		/* [in] */  UINT Subresource,
		/* [in] */  D3D11_MAP MapType,
		/* [in] */  UINT MapFlags,
		/* [out] */  D3D11_MAPPED_SUBRESOURCE *pMappedResource);

	virtual void STDMETHODCALLTYPE Unmap( 
		/* [in] */  ID3D11Resource *pResource,
		/* [in] */  UINT Subresource);

	virtual void STDMETHODCALLTYPE Begin( 
		/* [in] */  ID3D11Asynchronous *pAsync);

	virtual void STDMETHODCALLTYPE End( 
		/* [in] */  ID3D11Asynchronous *pAsync);

	virtual HRESULT STDMETHODCALLTYPE GetData( 
		/* [in] */  ID3D11Asynchronous *pAsync,
		/* [out_opt] */  void *pData,
		/* [in] */  UINT DataSize,
		/* [in] */  UINT GetDataFlags);

#ifndef PS3
	virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews( 
		/* [in] */  UINT NumRTVs,
		/* [in_opt] */  ID3D11RenderTargetView *const *ppRenderTargetViews,
		/* [in_opt] */  ID3D11DepthStencilView *pDepthStencilView,
		/* [in] */  UINT UAVStartSlot,
		/* [in] */  UINT NumUAVs,
		/* [in_opt] */  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
		/* [in_opt] */  const UINT *pUAVInitialCounts);

	virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect( 
		/* [in] */  ID3D11Buffer *pBufferForArgs,
		/* [in] */  UINT AlignedByteOffsetForArgs);

	virtual void STDMETHODCALLTYPE DrawInstancedIndirect( 
		/* [in] */  ID3D11Buffer *pBufferForArgs,
		/* [in] */  UINT AlignedByteOffsetForArgs);

	virtual void STDMETHODCALLTYPE Dispatch( 
		/* [in] */  UINT ThreadGroupCountX,
		/* [in] */  UINT ThreadGroupCountY,
		/* [in] */  UINT ThreadGroupCountZ);

	virtual void STDMETHODCALLTYPE DispatchIndirect( 
		/* [in] */  ID3D11Buffer *pBufferForArgs,
		/* [in] */  UINT AlignedByteOffsetForArgs);

	virtual void STDMETHODCALLTYPE CopyStructureCount( 
		/* [in] */  ID3D11Buffer *pDstBuffer,
		/* [in] */  UINT DstAlignedByteOffset,
		/* [in] */  ID3D11UnorderedAccessView *pSrcView);

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint( 
		/* [in] */  ID3D11UnorderedAccessView *pUnorderedAccessView,
		/* [in] */  const UINT Values[ 4 ]);

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat( 
		/* [in] */  ID3D11UnorderedAccessView *pUnorderedAccessView,
		/* [in] */  const FLOAT Values[ 4 ]);

	virtual void STDMETHODCALLTYPE SetResourceMinLOD( 
		/* [in] */  ID3D11Resource *pResource,
		FLOAT MinLOD);

	virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD( 
		/* [in] */  ID3D11Resource *pResource);

	virtual void STDMETHODCALLTYPE ExecuteCommandList( 
		/* [in] */  ID3D11CommandList *pCommandList,
		/* [in] */BOOL RestoreContextState);

	virtual void STDMETHODCALLTYPE HSSetShaderResources( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumViews,
		/* [in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews);

	virtual void STDMETHODCALLTYPE HSSetShader( 
		/* [in_opt] */  ID3D11HullShader *pHullShader,
		/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
		UINT NumClassInstances);

	virtual void STDMETHODCALLTYPE HSSetSamplers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumSamplers,
		/* [in] */ ID3D11SamplerState *const *ppSamplers);

	virtual void STDMETHODCALLTYPE HSSetConstantBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [in] */ ID3D11Buffer *const *ppConstantBuffers);

	virtual void STDMETHODCALLTYPE DSSetShaderResources( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumViews,
		/* [in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews);

	virtual void STDMETHODCALLTYPE DSSetShader( 
		/* [in_opt] */  ID3D11DomainShader *pDomainShader,
		/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
		UINT NumClassInstances);

	virtual void STDMETHODCALLTYPE DSSetSamplers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumSamplers,
		/* [in] */ ID3D11SamplerState *const *ppSamplers);

	virtual void STDMETHODCALLTYPE DSSetConstantBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [in] */ ID3D11Buffer *const *ppConstantBuffers);

	virtual void STDMETHODCALLTYPE CSSetShaderResources( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumViews,
		/* [in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews);

	virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumUAVs,
		/* [in] */ ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
		/* [in] */ const UINT *pUAVInitialCounts);

	virtual void STDMETHODCALLTYPE CSSetShader( 
		/* [in_opt] */  ID3D11ComputeShader *pComputeShader,
		/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
		/* [in] */ UINT NumClassInstances);

	virtual void STDMETHODCALLTYPE CSSetSamplers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumSamplers,
		/* [in] */ ID3D11SamplerState *const *ppSamplers);

	virtual void STDMETHODCALLTYPE CSSetConstantBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [in] */ ID3D11Buffer *const *ppConstantBuffers);

	virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews( 
		/* [in] */ UINT NumRTVs,
		/* [out] */ ID3D11RenderTargetView **ppRenderTargetViews,
		/* [out] */  ID3D11DepthStencilView **ppDepthStencilView,
		/* [in] */ UINT UAVStartSlot,
		/* [in] */ UINT NumUAVs,
		/* [out] */ ID3D11UnorderedAccessView **ppUnorderedAccessViews);


	virtual void STDMETHODCALLTYPE HSGetShaderResources( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumViews,
		/* [out] */ ID3D11ShaderResourceView **ppShaderResourceViews);

	virtual void STDMETHODCALLTYPE HSGetShader( 
		/* [out] */  ID3D11HullShader **ppHullShader,
		/* [out] */ ID3D11ClassInstance **ppClassInstances,
		/* [inout] */ UINT *pNumClassInstances);

	virtual void STDMETHODCALLTYPE HSGetSamplers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumSamplers,
		/* [in] */ ID3D11SamplerState **ppSamplers);

	virtual void STDMETHODCALLTYPE HSGetConstantBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [out] */ ID3D11Buffer **ppConstantBuffers);

	virtual void STDMETHODCALLTYPE DSGetShaderResources( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumViews,
		/* [out] */ ID3D11ShaderResourceView **ppShaderResourceViews);

	virtual void STDMETHODCALLTYPE DSGetShader( 
		/* [out] */  ID3D11DomainShader **ppDomainShader,
		/* [out] */ ID3D11ClassInstance **ppClassInstances,
		/* [inout] */  UINT *pNumClassInstances);

	virtual void STDMETHODCALLTYPE DSGetSamplers( 
		/* [in] */  UINT StartSlot,
		/* [in] */  UINT NumSamplers,
		/* [out] */  ID3D11SamplerState **ppSamplers);

	virtual void STDMETHODCALLTYPE DSGetConstantBuffers( 
		/* [in] */  UINT StartSlot,
		/* [in] */  UINT NumBuffers,
		/* [out] */ ID3D11Buffer **ppConstantBuffers);

	virtual void STDMETHODCALLTYPE CSGetShaderResources( 
		/* [in] */  UINT StartSlot,
		/* [in] */  UINT NumViews,
		/* [out] */ ID3D11ShaderResourceView **ppShaderResourceViews);

	virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews( 
		/* [in] */  UINT StartSlot,
		/* [in] */  UINT NumUAVs,
		/* [out] */ ID3D11UnorderedAccessView **ppUnorderedAccessViews);

	virtual void STDMETHODCALLTYPE CSGetShader( 
		/* [out] */ ID3D11ComputeShader **ppComputeShader,
		/* [out] */ ID3D11ClassInstance **ppClassInstances,
		/* [inout] */ UINT *pNumClassInstances);

	virtual void STDMETHODCALLTYPE CSGetSamplers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumSamplers,
		/* [out] */ ID3D11SamplerState **ppSamplers);

	virtual void STDMETHODCALLTYPE CSGetConstantBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [out] */ ID3D11Buffer **ppConstantBuffers);

	virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType(void);

	virtual UINT STDMETHODCALLTYPE GetContextFlags(void);

	virtual HRESULT STDMETHODCALLTYPE FinishCommandList( 
		BOOL RestoreDeferredContextState,
		/* [out] */ ID3D11CommandList **ppCommandList);
#endif
	virtual void STDMETHODCALLTYPE VSSetConstantBuffers( 
		/* [in] */ UINT StartConstantSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][in] */ ID3D11Buffer *const *ppConstantBuffers) ;

	virtual void STDMETHODCALLTYPE PSSetShaderResources( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumViews,
		/* [size_is][in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews) ;

	virtual void STDMETHODCALLTYPE PSSetShader( 
		/* [in] */ ID3D11PixelShader *pPixelShader,
		/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
		/* [in] */ UINT NumClassInstances) ;

	virtual void STDMETHODCALLTYPE PSSetSamplers( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumSamplers,
		/* [size_is][in] */ ID3D11SamplerState *const *ppSamplers) ;

	virtual void STDMETHODCALLTYPE VSSetShader( 
		/* [in] */ ID3D11VertexShader *pVertexShader,
		/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
		/* [in] */ UINT NumClassInstances) ;

	virtual void STDMETHODCALLTYPE DrawIndexed( 
		/* [in] */ UINT IndexCount,
		/* [in] */ UINT StartIndexLocation,
		/* [in] */ INT BaseVertexLocation) ;

	virtual void STDMETHODCALLTYPE Draw( 
		/* [in] */ UINT VertexCount,
		/* [in] */ UINT StartVertexLocation) ;

	virtual void STDMETHODCALLTYPE PSSetConstantBuffers( 
		/* [in] */ UINT StartConstantSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][in] */ ID3D11Buffer *const *ppConstantBuffers) ;

	virtual void STDMETHODCALLTYPE IASetInputLayout( 
		/* [in] */ ID3D11InputLayout *pInputLayout) ;

	virtual void STDMETHODCALLTYPE IASetVertexBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][in] */ ID3D11Buffer *const *ppVertexBuffers,
		/* [size_is][in] */ const UINT *pStrides,
		/* [size_is][in] */ const UINT *pOffsets) ;

	virtual void STDMETHODCALLTYPE IASetIndexBuffer( 
		/* [in] */ ID3D11Buffer *pIndexBuffer,
		/* [in] */ DXGI_FORMAT Format,
		/* [in] */ UINT Offset) ;

	virtual void STDMETHODCALLTYPE DrawIndexedInstanced( 
		/* [in] */ UINT IndexCountPerInstance,
		/* [in] */ UINT InstanceCount,
		/* [in] */ UINT StartIndexLocation,
		/* [in] */ INT BaseVertexLocation,
		/* [in] */ UINT StartInstanceLocation) ;

	virtual void STDMETHODCALLTYPE DrawInstanced( 
		/* [in] */ UINT VertexCountPerInstance,
		/* [in] */ UINT InstanceCount,
		/* [in] */ UINT StartVertexLocation,
		/* [in] */ UINT StartInstanceLocation) ;

	virtual void STDMETHODCALLTYPE GSSetConstantBuffers( 
		/* [in] */ UINT StartConstantSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][in] */ ID3D11Buffer *const *ppConstantBuffers) ;

	virtual void STDMETHODCALLTYPE GSSetShader( 
		/* [in] */ ID3D11GeometryShader *pShader,
		/* [in] */ ID3D11ClassInstance *const *ppClassInstances,
		/* [in] */ UINT NumClassInstances) ;

	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology( 
		/* [in] */ D3D11_PRIMITIVE_TOPOLOGY Topology) ;

	virtual void STDMETHODCALLTYPE VSSetShaderResources( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumViews,
		/* [size_is][in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews) ;

	virtual void STDMETHODCALLTYPE VSSetSamplers( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumSamplers,
		/* [size_is][in] */ ID3D11SamplerState *const *ppSamplers) ;

	virtual void STDMETHODCALLTYPE SetPredication( 
		/* [in] */ ID3D11Predicate *pPredicate,
		/* [in] */ BOOL PredicateValue) ;

	virtual void STDMETHODCALLTYPE GSSetShaderResources( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumViews,
		/* [size_is][in] */ ID3D11ShaderResourceView *const *ppShaderResourceViews) ;

	virtual void STDMETHODCALLTYPE GSSetSamplers( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumSamplers,
		/* [size_is][in] */ ID3D11SamplerState *const *ppSamplers) ;

	virtual void STDMETHODCALLTYPE OMSetRenderTargets( 
		/* [in] */ UINT NumViews,
		/* [size_is][in] */ ID3D11RenderTargetView *const *ppRenderTargetViews,
		/* [in] */ ID3D11DepthStencilView *pDepthStencilView) ;

	virtual void STDMETHODCALLTYPE OMSetBlendState( 
		/* [in] */ ID3D11BlendState *pBlendState,
		/* [in] */ const FLOAT BlendFactor[ 4 ],
		/* [in] */ UINT SampleMask) ;

	virtual void STDMETHODCALLTYPE OMSetDepthStencilState( 
		/* [in] */ ID3D11DepthStencilState *pDepthStencilState,
		/* [in] */ UINT StencilRef) ;

	virtual void STDMETHODCALLTYPE SOSetTargets( 
		/* [in] */ UINT NumBuffers,
		/* [size_is][in] */ ID3D11Buffer *const *ppSOTargets,
		/* [size_is][in] */ const UINT *pOffsets) ;

	virtual void STDMETHODCALLTYPE DrawAuto( void) ;

	virtual void STDMETHODCALLTYPE RSSetState( 
		/* [in] */ ID3D11RasterizerState *pRasterizerState) ;

	virtual void STDMETHODCALLTYPE RSSetViewports( 
		/* [in] */ UINT NumViewports,
		/* [size_is][in] */ const D3D11_VIEWPORT *pViewports) ;

	virtual void STDMETHODCALLTYPE RSSetScissorRects( 
		/* [in] */ UINT NumRects,
		/* [size_is][in] */ const D3D11_RECT *pRects) ;

	virtual void STDMETHODCALLTYPE ClearRenderTargetView( 
		/* [in] */ ID3D11RenderTargetView *pRenderTargetView,
		/* [in] */ const FLOAT ColorRGBA[ 4 ]) ;

	virtual void STDMETHODCALLTYPE ClearDepthStencilView( 
		/* [in] */ ID3D11DepthStencilView *pDepthStencilView,
		/* [in] */ UINT Flags,
		/* [in] */ FLOAT Depth,
		/* [in] */ UINT8 Stencil) ;

	virtual void STDMETHODCALLTYPE GenerateMips( 
		/* [in] */ ID3D11ShaderResourceView *pShaderResourceView) ;

	virtual void STDMETHODCALLTYPE VSGetConstantBuffers( 
		/* [in] */ UINT StartConstantSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][out] */ ID3D11Buffer **ppConstantBuffers) ;

	virtual void STDMETHODCALLTYPE PSGetShaderResources( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumViews,
		/* [size_is][out] */ ID3D11ShaderResourceView **ppShaderResourceViews) ;

	virtual void STDMETHODCALLTYPE PSGetShader( 
		/* [out][in] */ ID3D11PixelShader **ppPixelShader,
		/* [in] */ ID3D11ClassInstance **ppClassInstances,
		/* [in] */ UINT* pNumClassInstances) ;

	virtual void STDMETHODCALLTYPE PSGetSamplers( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumSamplers,
		/* [size_is][out] */ ID3D11SamplerState **ppSamplers) ;

	virtual void STDMETHODCALLTYPE VSGetShader( 
		/* [out][in] */ ID3D11VertexShader **ppVertexShader,
		/* [in] */ ID3D11ClassInstance **ppClassInstances,
		/* [in] */ UINT* NumClassInstances) ;

	virtual void STDMETHODCALLTYPE PSGetConstantBuffers( 
		/* [in] */ UINT StartConstantSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][out] */ ID3D11Buffer **ppConstantBuffers) ;

	virtual void STDMETHODCALLTYPE IAGetInputLayout( 
		/* [out][in] */ ID3D11InputLayout **ppInputLayout) ;

	virtual void STDMETHODCALLTYPE IAGetVertexBuffers( 
		/* [in] */ UINT StartSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][out] */ ID3D11Buffer **ppVertexBuffers,
		/* [size_is][out] */ UINT *pStrides,
		/* [size_is][out] */ UINT *pOffsets) ;

	virtual void STDMETHODCALLTYPE IAGetIndexBuffer( 
		/* [out] */ ID3D11Buffer **pIndexBuffer,
		/* [out] */ DXGI_FORMAT *Format,
		/* [out] */ UINT *Offset) ;

	virtual void STDMETHODCALLTYPE GSGetConstantBuffers( 
		/* [in] */ UINT StartConstantSlot,
		/* [in] */ UINT NumBuffers,
		/* [size_is][out] */ ID3D11Buffer **ppConstantBuffers) ;

	virtual void STDMETHODCALLTYPE GSGetShader( 
		/* [out] */ ID3D11GeometryShader **ppGeometryShader,
		/* [in] */ ID3D11ClassInstance **ppClassInstances,
		/* [in] */ UINT* pNumClassInstances) ;

	virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology( 
		/* [out] */ D3D11_PRIMITIVE_TOPOLOGY *pTopology) ;

	virtual void STDMETHODCALLTYPE VSGetShaderResources( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumViews,
		/* [size_is][out] */ ID3D11ShaderResourceView **ppShaderResourceViews) ;

	virtual void STDMETHODCALLTYPE VSGetSamplers( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumSamplers,
		/* [size_is][out] */ ID3D11SamplerState **ppSamplers) ;

	virtual void STDMETHODCALLTYPE GetPredication( 
		/* [out] */ ID3D11Predicate **ppPredicate,
		/* [out] */ BOOL *pPredicateValue) ;

	virtual void STDMETHODCALLTYPE GSGetShaderResources( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumViews,
		/* [size_is][out] */ ID3D11ShaderResourceView **ppShaderResourceViews) ;

	virtual void STDMETHODCALLTYPE GSGetSamplers( 
		/* [in] */ UINT Offset,
		/* [in] */ UINT NumSamplers,
		/* [size_is][out] */ ID3D11SamplerState **ppSamplers) ;

	virtual void STDMETHODCALLTYPE OMGetRenderTargets( 
		/* [in] */ UINT NumViews,
		/* [size_is][out] */ ID3D11RenderTargetView **ppRenderTargetViews,
		/* [out] */ ID3D11DepthStencilView **ppDepthStencilView) ;

	virtual void STDMETHODCALLTYPE OMGetBlendState( 
		/* [out] */ ID3D11BlendState **ppBlendState,
		/* [out] */ FLOAT BlendFactor[ 4 ],
		/* [out] */ UINT *pSampleMask) ;

	virtual void STDMETHODCALLTYPE OMGetDepthStencilState( 
		/* [out] */ ID3D11DepthStencilState **ppDepthStencilState,
		/* [out] */ UINT *pStencilRef) ;

	virtual void STDMETHODCALLTYPE SOGetTargets( 
		/* [in] */ UINT NumBuffers,
		/* [size_is][out] */ ID3D11Buffer **ppSOTargets) ;

	virtual void STDMETHODCALLTYPE RSGetState( 
		/* [out] */ ID3D11RasterizerState **ppRasterizerState) ;

	virtual void STDMETHODCALLTYPE RSGetViewports( 
		/* [out][in] */ UINT *NumViewports,
		/* [size_is][out] */ D3D11_VIEWPORT *pViewports) ;

	virtual void STDMETHODCALLTYPE RSGetScissorRects( 
		/* [out][in] */ UINT *NumRects,
		/* [size_is][out] */ D3D11_RECT *pRects) ;

	virtual void STDMETHODCALLTYPE Flush( void) ;

	virtual void STDMETHODCALLTYPE CopySubresourceRegion(
		ID3D11Resource * pDstResource,
		UINT DstSubresource,
		UINT DstX,
		UINT DstY,
		UINT DstZ,
		ID3D11Resource * pSrcResource,
		UINT SrcSubresource,
		const D3D11_BOX * pSrcBox
		);

	virtual void STDMETHODCALLTYPE UpdateSubresource(
		ID3D11Resource * pDstResource,
		UINT DstSubresource,
		const D3D11_BOX * pDstBox,
		const void *pSrcData,
		UINT SrcRowPitch,
		UINT SrcDepthPitch
		);

	virtual void STDMETHODCALLTYPE ResolveSubresource(
		ID3D11Resource * pDstResource,
		UINT DstSubresource,
		ID3D11Resource * pSrcResource,
		UINT SrcSubresource,
		DXGI_FORMAT Format
		);

	virtual void STDMETHODCALLTYPE CopyResource(ID3D11Resource * pDstResource, ID3D11Resource * pSrcResource);

	virtual void STDMETHODCALLTYPE ClearState();
};

#endif // defined(DIRECT3D9)

_inline DWORD FLOATtoDWORD( float f )
{
	union FLOATDWORD
	{
		float f;
		DWORD dw;
	};

	FLOATDWORD val;
	val.f = f;
	return val.dw;
}

//=======================================================================

//=====================================================


struct STexFiller;

#define BUFFERED_VERTS 256

struct SD3DContext
{
	HWND m_hWnd;
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;
#if defined(DIRECT3D10)
	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pBackBuffer;
#endif
};

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOWINDOW            0x82000002
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_MEDIANOTFOUND       0x8200000b
#define D3DAPPERR_RESIZEFAILED        0x8200000c

struct SD3DRenderTarget
{
	LPDIRECT3DSURFACE9 m_pRT;
	LPDIRECT3DSURFACE9 m_pZB;
};

const float g_fEyeAdaptionLowerPercent = 0.05f;	// 5% percentil
const float g_fEyeAdaptionMidPercent = 0.5f;		// 50% percentil
const float g_fEyeAdaptionUpperPercent = 0.95f;	// 95% percentil

// Texture coordinate rectangle
struct CoordRect
{
	float fLeftU, fTopV;
	float fRightU, fBottomV;
};

HRESULT GetTextureRect(CTexture *pTexture, RECT* pRect);
HRESULT GetTextureCoords(CTexture *pTexSrc, RECT* pRectSrc, CTexture *pTexDest, RECT* pRectDest, CoordRect* pCoords);
void DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV, bool bClampToScreenRes = true);
void DrawFullScreenQuad(CoordRect c, bool bClampToScreenRes = true);
HRESULT GetSampleOffsets_GaussBlur5x5(DWORD dwD3DTexWidth, DWORD dwD3DTexHeight, Vec4* avTexCoordOffset, Vec4* avSampleWeight, FLOAT fMultiplier);
float GaussianDistribution( float x, float y, float rho);


#if defined (DIRECT3D10)  
struct SStateBlend
{
	UnINT64 nHashVal;
	D3D11_BLEND_DESC Desc;
	ID3D11BlendState* pState;

	SStateBlend() { memset(this, 0, sizeof(*this)); }

	static uint64 GetHash(const D3D11_BLEND_DESC& InDesc)
	{
		UnINT64 nHash;
		nHash.i.Low = InDesc.AlphaToCoverageEnable | 
			(InDesc.RenderTarget[0].BlendEnable<<1) | (InDesc.RenderTarget[1].BlendEnable<<2) | (InDesc.RenderTarget[2].BlendEnable<<3) | (InDesc.RenderTarget[3].BlendEnable<<4) |
			(InDesc.RenderTarget[0].SrcBlend<<5) | (InDesc.RenderTarget[0].DestBlend<<10) | 
			(InDesc.RenderTarget[0].SrcBlendAlpha<<15) | (InDesc.RenderTarget[0].DestBlendAlpha<<20) | 
			(InDesc.RenderTarget[0].BlendOp<<25) | (InDesc.RenderTarget[0].BlendOpAlpha<<28);
		nHash.i.High = InDesc.RenderTarget[0].RenderTargetWriteMask | 
			(InDesc.RenderTarget[1].RenderTargetWriteMask<<4) | 
			(InDesc.RenderTarget[2].RenderTargetWriteMask<<8) | 
			(InDesc.RenderTarget[3].RenderTargetWriteMask<<12) | 
			InDesc.IndependentBlendEnable << 16;

		return nHash.SortVal;
	}
} _ALIGN(16);

struct SStateRaster
{
	uint64 nValuesHash;
	uint32 nHashVal;
	ID3D11RasterizerState* pState;
	D3D11_RASTERIZER_DESC Desc;

	SStateRaster() {
		memset(this, 0, sizeof(*this)); 
		Desc.DepthClipEnable = true;
		Desc.FillMode = D3D11_FILL_SOLID;
	}

	static uint32 GetHash(const D3D11_RASTERIZER_DESC& InDesc)
	{
		uint32 nHash;
		nHash =      InDesc.FillMode | (InDesc.CullMode<<2) |
			(InDesc.DepthClipEnable<<4) | (InDesc.FrontCounterClockwise<<5) | 
			(InDesc.ScissorEnable<<6) | (InDesc.MultisampleEnable<<7) | (InDesc.AntialiasedLineEnable<<8) |
			(InDesc.DepthBias<<9);

		return nHash;
	}

	static uint64 GetValuesHash(const D3D11_RASTERIZER_DESC& InDesc)
	{
		uint64 nHash;
		//avoid breaking strict alising rules
		union f32_u
		{
			float floatVal;
			unsigned int uintVal;
		};
		f32_u uDepthBiasClamp; uDepthBiasClamp.floatVal = InDesc.DepthBiasClamp;
		f32_u uSlopeScaledDepthBias; uSlopeScaledDepthBias.floatVal = InDesc.SlopeScaledDepthBias;
		nHash = ( ( (uint64)uDepthBiasClamp.uintVal ) | 
			( (uint64)uSlopeScaledDepthBias.uintVal) << 32 );
		return nHash;
	}

} _ALIGN(16);
_inline uint32 sStencilState(const D3D11_DEPTH_STENCILOP_DESC& Desc)
{
	uint32 nST = (Desc.StencilFailOp<<0) | 
		(Desc.StencilDepthFailOp<<4) |
		(Desc.StencilPassOp<<8) |
		(Desc.StencilFunc<<12);
	return nST;
}
struct SStateDepth
{
	uint64 nHashVal;
	D3D11_DEPTH_STENCIL_DESC Desc;
	ID3D11DepthStencilState* pState;
	SStateDepth() { memset(this, 0, sizeof(*this)); }

	static uint64 GetHash(const D3D11_DEPTH_STENCIL_DESC& InDesc)
	{
		uint64 nHash;
		nHash = (InDesc.DepthEnable<<0) | 
			(InDesc.DepthWriteMask<<1) |
			(InDesc.DepthFunc<<2) | 
			(InDesc.StencilEnable<<6) |
			(InDesc.StencilReadMask<<7) | 
			(InDesc.StencilWriteMask<<15) |
			(((uint64)sStencilState(InDesc.FrontFace))<<23) |
			(((uint64)sStencilState(InDesc.BackFace))<<39);
		return nHash;
	}
} _ALIGN(16);\

#endif

#define MAX_OCCL_QUERIES    512

//======================================================================
/// Forward declared classes

class CGPUProfiler;

//======================================================================
/// Direct3D Render driver class

#ifdef SHADER_ASYNC_COMPILATION
class CAsyncShaderTask;
typedef const std::auto_ptr<CAsyncShaderTask> TConstAsyncShaderTaskPtr;
#endif

class CD3D9Renderer : public CRenderer
{
	friend struct SPixFormat;

public:
	CD3D9Renderer();
	~CD3D9Renderer();

	const SRenderTileInfo& GetRenderTileInfo() const { return m_RenderTileInfo; }

protected:

	// Windows context
	char      m_WinTitle[80];
	HINSTANCE m_hInst;            
	HWND      m_hWnd;              // The main app window
	HWND      m_hWndDesktop;       // The desktop window

	//stereo mode
	bool      m_bStereo;
	HWND      m_hWnd2;

	int m_nPointState;

	DWORD             m_FenceOcclusionReady;
	int               m_numOcclusionDownsampleStages;

	CGPUProfiler*			m_profiler;

#if defined (DIRECT3D9)
	LPDIRECT3D9       m_pD3D;              // The main D3D object
	//CD3DEnumeration   m_D3DEnum;

	// From D3D.
	// Main objects used for creating and rendering the 3D scene
	LPDIRECT3DSURFACE9 m_pBackBuffer;
	LPDIRECT3DSURFACE9 m_pSecondBackBuffer;
	LPDIRECT3DSURFACE9 m_pZBuffer;
#elif defined (DIRECT3D10)  
	ID3D11RenderTargetView* m_pBackBuffer;
	ID3D11RenderTargetView* m_pSecondBackBuffer;
	ID3D11DepthStencilView* m_pZBuffer;
	IDXGISwapChain* m_pSwapChain;
	enum PresentStatus
	{
		epsOccluded			= 1 << 0,
		epsNonExclusive = 1 << 1,
	};
	DWORD             m_dwPresentStatus;     // Indicate present status

#endif

	DWORD             m_dwCreateFlags;     // Indicate sw or hw vertex processing
	DWORD             m_dwWindowStyle;     // Saved window style for mode switches
	char              m_strDeviceStats[90];// String to hold D3D device stats

	int               m_SceneRecurseCount;

	D3DGAMMARAMP      m_SystemGammaRamp;

	SRenderTileInfo m_RenderTileInfo;

	//==================================================================

public:
	D3DQuery  *m_pQuery[2]; 
	D3DQuery  *m_pHDRLumQuery; 
	D3DQuery  *m_pQueryPerf; 
	int        m_nQueryCount; 

#if !defined(PS3)
	TArray<COcclusionQuery> m_OcclQueries;
#endif

#define MAX_DYNVB3D_VERTS       16384
#define POOL_P3F_COL4UB_TEX2F   0
#define POOL_TRP3F_COL4UB_TEX2F 1
#define POOL_TRP3F_TEX2F_TEX3F  2
#define POOL_P3F_TEX3F          3
#define POOL_P3F_TEX2F_TEX3F    4
#define POOL_MAX                5


#if defined (DIRECT3D9)
	LPDIRECT3DDEVICE9 m_pd3dDevice;        // The D3D rendering device
	LPDIRECT3DDEVICE9 m_pActuald3dDevice;  // The actual D3D rendering device
	const D3DPRESENT_PARAMETERS *m_pd3dpp;         // Parameters for CreateDevice/Reset
	const D3DCAPS9   *m_pd3dCaps;           // Caps for the device
	D3DSURFACE_DESC   m_d3dsdBackBuffer;      // Surface desc of the Backbuffer
	D3DFORMAT         m_ZFormat;

	LPDIRECT3DVERTEXDECLARATION9 m_pLastVDeclaration;

	CMyDirect3DDevice9 *m_pMyd3dDevice;        // The D3D rendering device

#elif defined (DIRECT3D10)
	D3DDevice* m_pd3dDevice;        // The D3D rendering device
	D3DDevice* m_pActuald3dDevice;  // The actual D3D rendering device
	ID3D11DeviceContext* m_pd3dDeviceContext;        // The D3D rendering device context
	ID3D11DeviceContext *m_pActuald3dDeviceContext;  // The D3D rendering device context
	ID3D11InputLayout*   m_pLastVDeclaration;

	DXGI_SURFACE_DESC  m_d3dsdBackBuffer;   // Surface desc of the BackBuffer
	DXGI_FORMAT        m_ZFormat;

	CMyDirect3DDevice11 *m_pMyd3dDevice;        // The D3D rendering device
	CMyDirect3DDeviceContext11 *m_pMyd3dDeviceContext;        // The D3D rendering device

	ID3D11Debug*       m_pd3dDebug; 

	D3D11_PRIMITIVE_TOPOLOGY m_CurTopology;

	TArray<SStateBlend> m_StatesBL;
	TArray<SStateRaster> m_StatesRS;
	TArray<SStateDepth> m_StatesDP;
	uint32 m_nCurStateBL;
	uint32 m_nCurStateRS;
	uint32 m_nCurStateDP;
	uint8 m_nCurStencRef;
	bool m_bPendingSetWindow;
	bool SetBlendState(const SStateBlend *pNewState)
	{
		uint32 i;
		HRESULT hr = S_OK;
		uint64 nHash = SStateBlend::GetHash(pNewState->Desc);
		for (i=0; i<m_StatesBL.Num(); i++)
		{
			if (m_StatesBL[i].nHashVal.SortVal == nHash)
				break;
		}
		if (i == m_StatesBL.Num())
		{
			SStateBlend *pState = m_StatesBL.AddIndex(1);
			*pState = *pNewState;
			pState->nHashVal.SortVal = nHash;
			hr = m_pd3dDevice->CreateBlendState(&pState->Desc, &pState->pState);
			assert(SUCCEEDED(hr));
		}
		if (i != m_nCurStateBL)
		{
			m_nCurStateBL = i;
			m_pd3dDeviceContext->OMSetBlendState(m_StatesBL[i].pState, 0, 0xffffffff);
		}
		return SUCCEEDED(hr);
	}
	bool SetRasterState(const SStateRaster *pNewState)
	{
		uint32 i;
		HRESULT hr = S_OK;
		BOOL bFSAA = m_RP.m_FSAAData.Type > 1;
		if (bFSAA != pNewState->Desc.MultisampleEnable)
		{
			static SStateRaster RS;
			RS = *pNewState;
			RS.Desc.MultisampleEnable = bFSAA;
			pNewState = &RS;
		}
		uint32 nHash = SStateRaster::GetHash(pNewState->Desc);
		uint64 nValuesHash = SStateRaster::GetValuesHash(pNewState->Desc);
		for (i=0; i<m_StatesRS.Num(); i++)
		{
			if (m_StatesRS[i].nHashVal == nHash && m_StatesRS[i].nValuesHash == nValuesHash)
				break;
		}
		if (i == m_StatesRS.Num())
		{
			SStateRaster *pState = m_StatesRS.AddIndex(1);
			*pState = *pNewState;
			pState->nHashVal = nHash;
			pState->nValuesHash = nValuesHash;
			hr = m_pd3dDevice->CreateRasterizerState(&pState->Desc, &pState->pState);
			assert(SUCCEEDED(hr));
		}
		if (i != m_nCurStateRS)
		{
			m_nCurStateRS = i;

#ifndef PS3
			m_pd3dDeviceContext->RSSetState(m_StatesRS[i].pState);


#endif
		}
		return SUCCEEDED(hr);
	}
	bool SetDepthState(const SStateDepth *pNewState, uint8 newStencRef)
	{
		uint32 i;
		HRESULT hr = S_OK;
		uint64 nHash = SStateDepth::GetHash(pNewState->Desc);
		for (i=0; i<m_StatesDP.Num(); i++)
		{
			if (m_StatesDP[i].nHashVal == nHash)
				break;
		}
		if (i == m_StatesDP.Num())
		{
			SStateDepth *pState = m_StatesDP.AddIndex(1);
			*pState = *pNewState;
			pState->nHashVal = nHash;
			hr = m_pd3dDevice->CreateDepthStencilState(&pState->Desc, &pState->pState);
			assert(SUCCEEDED(hr));
		}
		if (i != m_nCurStateDP || m_nCurStencRef != newStencRef)
		{
			m_nCurStateDP = i;
			m_nCurStencRef = newStencRef;
			m_pd3dDeviceContext->OMSetDepthStencilState(m_StatesDP[i].pState, newStencRef);
		}
		return SUCCEEDED(hr);
	}

	void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topType)
	{
		if (m_CurTopology != topType)
		{
			m_CurTopology = topType;
			m_pd3dDeviceContext->IASetPrimitiveTopology(m_CurTopology);
		}
	}
#endif

	SD3DSurface m_DepthBufferOrig;
	SD3DSurface m_DepthBufferOrigFSAA;

	// x = average luminance, y = max luminance, z = min luminance, 
	Vec4 m_vSceneLuminanceInfo;	
	float m_fStocopicSceneScale;
	float m_fAdaptedSceneScale;
	float m_fAdaptedSceneScaleLBuffer;

	ETEX_Format m_HDR_FloatFormat_Scalar;
	ETEX_Format m_HDR_FloatFormat;
	int m_MaxAnisotropyLevel;

	//===============================================================================

	int m_nCurVBuf[POOL_MAX];
	int m_nVertsDMesh[POOL_MAX];
	int m_nOffsDMesh[POOL_MAX];
	D3DVertexBuffer *m_pVBAr[POOL_MAX][4];
	D3DVertexBuffer *m_pVB[POOL_MAX];

	int m_nCurIBuf;
	int m_nIndsDMesh;
	int m_nIOffsDMesh;
	D3DIndexBuffer *m_pIBAr[4];
	D3DIndexBuffer *m_pIB;

	//////////////////////////////////////////////////////////////////////////
	enum EShapeMeshType
	{
		SHAPE_PROJECTOR = 0 ,
		SHAPE_CLIP_PROJECTOR,
		SHAPE_SIMPLE_PROJECTOR,
		SHAPE_MAX,
	};

	D3DVertexBuffer *m_pUnitFrustumVB[SHAPE_MAX];
	D3DIndexBuffer *m_pUnitFrustumIB[SHAPE_MAX];

	D3DVertexBuffer *m_pUnitSphereVB;
	D3DIndexBuffer *m_pUnitSphereIB;

	D3DVertexBuffer *m_pUnitBoxVB;
	D3DIndexBuffer *m_pUnitBoxIB;

	int m_UnitFrustVBSize[SHAPE_MAX];
	int m_UnitFrustIBSize[SHAPE_MAX];
	int m_UnitSphereVBSize;
	int m_UnitSphereIBSize;
	int m_UnitBoxVBSize;
	int m_UnitBoxIBSize;
	//////////////////////////////////////////////////////////////////////////

	uint16 *GetIBPtr(int nInds, int &nOffs)
	{
		assert(m_pRT->IsRenderThread());
		if (!m_pIB)
			return NULL;
		HRESULT hr;
		uint16 *pInds = NULL;
		if (nInds > m_nIndsDMesh)
		{
			assert(0);
			return NULL;
		}
		if (nInds+m_nIOffsDMesh > m_nIndsDMesh)
		{
			m_nCurIBuf = (m_nCurIBuf+1)&3;
			m_pIB = m_pIBAr[m_nCurIBuf];
#if defined (DIRECT3D9)
#if !defined(XENON) && !defined(PS3)
			hr = m_pIB->Lock(0, nInds*sizeof(short), (void **) &pInds, D3DLOCK_DISCARD);
#endif
#elif defined (DIRECT3D10)
			STALL_PROFILER("set index_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			hr = m_pd3dDeviceContext->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pInds = (uint16*)mappedResource.pData;
#endif
			nOffs = 0;
			m_nIOffsDMesh = nInds;
		}
		else
		{
#if defined (DIRECT3D9)
#if !defined(XENON) && !defined(PS3)
			hr = m_pIB->Lock(m_nIOffsDMesh*sizeof(short), nInds*sizeof(short), (void **) &pInds, D3DLOCK_NOOVERWRITE);
#endif
#elif defined (DIRECT3D10)
			STALL_PROFILER("set index_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			hr = m_pd3dDeviceContext->Map(m_pIB, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
			pInds = (uint16*)mappedResource.pData;
			pInds += m_nIOffsDMesh;
#endif
			nOffs = m_nIOffsDMesh;
			m_nIOffsDMesh += nInds;
		}
		return pInds;
	}
	void UnlockIB()
	{
		assert(m_pRT->IsRenderThread());
#if defined (DIRECT3D9)
		HRESULT hr = m_pIB->Unlock();
#elif defined (DIRECT3D10)
		m_pd3dDeviceContext->Unmap(m_pIB, 0);
#endif
	}
	void DiscardIB()
	{
		assert(m_pRT->IsRenderThread());
		m_nCurIBuf = (m_nCurIBuf+1)&3;
		m_pIB = m_pIBAr[m_nCurIBuf];
		int nInds = 1;
		uint16 *pInds = NULL;
#if defined (DIRECT3D9)
#if !defined(XENON) && !defined(PS3)
		HRESULT hr = m_pIB->Lock(0, nInds*sizeof(short), (void **)&pInds, D3DLOCK_DISCARD);
#endif
		hr = m_pIB->Unlock();
#elif defined (DIRECT3D10)
		STALL_PROFILER("set index_buffer");
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_pd3dDeviceContext->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		pInds = (uint16*)mappedResource.pData;
		pInds += nInds;
		m_pd3dDeviceContext->Unmap(m_pIB, 0);        
#endif
		m_nIOffsDMesh = nInds;
	}

	int GetDynVBSize( EVertexFormat vertexType = eVF_P3F_C4B_T2F )
	{
		switch( vertexType )
		{
		case eVF_P3F_C4B_T2F:
			return m_nVertsDMesh[ POOL_P3F_COL4UB_TEX2F ];
		case eVF_TP3F_C4B_T2F:
			return m_nVertsDMesh[ POOL_TRP3F_COL4UB_TEX2F ];
		case eVF_TP3F_T2F_T3F:
			return m_nVertsDMesh[ POOL_TRP3F_TEX2F_TEX3F ];
		case eVF_P3F_T3F:
			return m_nVertsDMesh[ POOL_P3F_TEX3F ];
		case eVF_P3F_T2F_T3F:
			return m_nVertsDMesh[ POOL_P3F_TEX2F_TEX3F ];
		default:
			return 0;
		}
	}

	void *GetVBPtr(int nVerts, int &nOffs, int Pool=0)
	{
		assert(m_pRT->IsRenderThread());
		HRESULT hr;
		void *pVertices = NULL;
		if (nVerts > m_nVertsDMesh[Pool])
		{
			assert(0);
			return NULL;
		}
		if (!m_pVBAr[Pool][0])
			return NULL;

		int nVertSize;

		switch (Pool)
		{
		case POOL_P3F_COL4UB_TEX2F:
		default:
			nVertSize = sizeof(SVF_P3F_C4B_T2F);
			break;
		case POOL_TRP3F_COL4UB_TEX2F:
			nVertSize = sizeof(SVF_TP3F_C4B_T2F);
			break;
		case POOL_TRP3F_TEX2F_TEX3F:
			nVertSize = sizeof(SVF_TP3F_T2F_T3F);
			break;
		case POOL_P3F_TEX3F:
			nVertSize = sizeof(SVF_P3F_T3F);
			break;
		case POOL_P3F_TEX2F_TEX3F:
			nVertSize = sizeof(SVF_P3F_T2F_T3F);
			break;
		}

		assert(Pool < POOL_MAX);
#if defined (DIRECT3D10)
		if (CV_d3d9_vbpools==2 || nVerts+m_nOffsDMesh[Pool] > m_nVertsDMesh[Pool])
#else
		if (nVerts+m_nOffsDMesh[Pool] > m_nVertsDMesh[Pool])
#endif
		{
			m_nCurVBuf[Pool] = (m_nCurVBuf[Pool]+1)&3;
			m_pVB[Pool] = m_pVBAr[Pool][m_nCurVBuf[Pool]];
#if defined (DIRECT3D9)
#if !defined(XENON)
			hr = m_pVB[Pool]->Lock(0, nVerts*nVertSize, &pVertices, D3DLOCK_DISCARD);


#endif
#elif defined (DIRECT3D10)
			STALL_PROFILER("set vertex_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pd3dDeviceContext->Map(m_pVB[Pool], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pVertices = mappedResource.pData;
#endif
			nOffs = 0;
			m_nOffsDMesh[Pool] = nVerts;
		}
		else
		{
#if defined (DIRECT3D9)
#if !defined(XENON) && !defined(PS3)
			hr = m_pVB[Pool]->Lock(m_nOffsDMesh[Pool]*nVertSize, nVerts*nVertSize, &pVertices, D3DLOCK_NOOVERWRITE);
#endif
#elif defined (DIRECT3D10)
			STALL_PROFILER("set vertex_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			hr = m_pd3dDeviceContext->Map(m_pVB[Pool], 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
			pVertices = mappedResource.pData;
			byte *pD = (byte *)pVertices;
			pVertices = (void *)&pD[m_nOffsDMesh[Pool]*nVertSize];
#endif
			nOffs = m_nOffsDMesh[Pool];
			m_nOffsDMesh[Pool] += nVerts;
		}
		return pVertices;
	}
	void UnlockVB(int Pool=0)
	{
		assert(m_pRT->IsRenderThread());

#if defined (DIRECT3D9)
		HRESULT hr = m_pVB[Pool]->Unlock();
#elif defined (DIRECT3D10)
		m_pd3dDeviceContext->Unmap(m_pVB[Pool], 0);        
#endif
	}
	void DiscardVB()
	{
		int Pool;
		for (Pool=0; Pool<POOL_MAX; Pool++)
		{
			m_nCurVBuf[Pool] = (m_nCurVBuf[Pool]+1)&3;
			m_pVB[Pool] = m_pVBAr[Pool][m_nCurVBuf[Pool]];
			int nVertSize;

			switch (Pool)
			{
			case POOL_P3F_COL4UB_TEX2F:
			default:
				nVertSize = sizeof(SVF_P3F_C4B_T2F);
				break;
			case POOL_TRP3F_COL4UB_TEX2F:
				nVertSize = sizeof(SVF_TP3F_C4B_T2F);
				break;
			case POOL_TRP3F_TEX2F_TEX3F:
				nVertSize = sizeof(SVF_TP3F_T2F_T3F);
				break;
			case POOL_P3F_TEX3F:
				nVertSize = sizeof(SVF_P3F_T3F);
				break;
			case POOL_P3F_TEX2F_TEX3F:
				nVertSize = sizeof(SVF_P3F_T2F_T3F);
				break;
			}
			int nVerts = 1;
			void *pVertices = NULL;
#if defined (DIRECT3D9)
#if !defined(XENON) && !defined(PS3)
			HRESULT hr = m_pVB[Pool]->Lock(0, nVerts*nVertSize, &pVertices, D3DLOCK_DISCARD);
#endif
			hr = m_pVB[Pool]->Unlock();
#elif defined (DIRECT3D10)
			STALL_PROFILER("set vertex_buffer");
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = m_pd3dDeviceContext->Map(m_pVB[Pool], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			pVertices = mappedResource.pData;
			pVertices = &((byte *)pVertices)[nVerts*nVertSize];
			m_pd3dDeviceContext->Unmap(m_pVB[Pool], 0);        
#endif
			m_nOffsDMesh[Pool] = nVerts;
		}
	}

	//============================================================================================

	// Pixel formats from D3D.
	SPixFormat        m_FormatA8;          //8 bit alpha
	SPixFormat        m_FormatL8;          //8 bit luminance
	SPixFormat        m_FormatA8L8;        //16
	SPixFormat        m_FormatA8R8G8B8;    //32 bit
	SPixFormat        m_FormatX8R8G8B8;    //32 bit
	SPixFormat        m_FormatA2R10G10B10;           //32 bit
	SPixFormat        m_FormatR8G8B8;      //24 bit
	SPixFormat        m_FormatA4R4G4B4;     //16 bit
	SPixFormat        m_FormatR5G6B5;       //16 bit

	SPixFormat        m_FormatA16B16G16R16F;    //64 bit
	SPixFormat        m_FormatA16B16G16R16;     //64 bit
	SPixFormat        m_FormatG16R16F;          //32 bit
	SPixFormat        m_FormatG16R16;           //32 bit
	SPixFormat        m_FormatV16U16;          //32 bit
	SPixFormat        m_FormatR16F;            //16 bit
	SPixFormat        m_FormatR32F;            //32 bit
	SPixFormat        m_FormatA32B32G32R32F;    //128 bit

	SPixFormat        m_Format3Dc;     //
	SPixFormat        m_FormatDXT1;    //Compressed RGB
	SPixFormat        m_FormatDXT3;    //Compressed RGBA
	SPixFormat        m_FormatDXT5;    //Compressed RGBA

	SPixFormat        m_FormatDF16;		//ATI 16-bit native depth buffer
	SPixFormat        m_FormatDF24;		//ATI 24-bit native depth buffer

	SPixFormat        m_FormatD32F;   //DX11 depth format
	SPixFormat        m_FormatD16;		//Nvidia 16-bit native depth buffer
	SPixFormat        m_FormatD24S8;	//Nvidia 24-bit native depth buffer

	SPixFormat        m_FormatNULL;		//Nvidia's NULL texture

	SPixFormat        m_FormatDepth24; //Depth texture
	SPixFormat        m_FormatDepth16; //Depth texture

	SPixFormat*       m_FirstPixelFormat;

	byte m_GammmaTable[256];

	int	m_fontBlendMode;

	CColorGradingControllerD3D* m_pColorGradingControllerD3D;

public:
#ifdef SHADER_ASYNC_COMPILATION
	DynArray<CAsyncShaderTask*> m_AsyncShaderTasks;
public:
#endif

#if defined (DIRECT3D9)
	LPDIRECT3DDEVICE9  GetDevice() { return m_pd3dDevice; }
	LPDIRECT3DDEVICE9	 GetD3DDevice() { return m_pd3dDevice; }
	void               SetD3D(LPDIRECT3D9 pD3D) { m_pD3D = pD3D; }
	LPDIRECT3D9        GetD3D() { return m_pD3D; }
	LPDIRECT3DSURFACE9 GetZSurface() { return m_pZBuffer; }
	LPDIRECT3DSURFACE9 GetBackSurface() { return m_pBackBuffer; }
	const D3DCAPS9     *GetD3DCaps()  { return m_pd3dCaps; }
#elif defined (DIRECT3D10)
	D3DDevice*  GetDevice() { return m_pd3dDevice; }
	D3DDevice*	GetD3DDevice() { return m_pd3dDevice; }
	ID3D11DeviceContext*  GetDeviceContext() { return m_pd3dDeviceContext; }
#endif

	void SetDefaultTexParams(bool bUseMips, bool bRepeat, bool bLoad);

public:

	void FX_PrepareDepthMapsForLight(CDLight* pLight, int nLightID);
	void EF_PrepareShadowGenRenderList();
	void EF_PrepareShadowGenForLight(CDLight* pLight, int nLightID);
	bool PrepareShadowGenForFrustum(ShadowMapFrustum* pCurFrustum, CDLight* pLight, int nLightFrustumID = 0);

#if defined (DIRECT3D10) && !defined(PS3)
	void PrepareDepthMap(CDLight* pLightSource);
#endif
	void SetupShadowGenPass(int Num, ShadowMapFrustum * pFr);

	void BlurShadow(ShadowMapFrustum *pFr);
	void FX_DeferredShadowPassAO( CCryName & TechName, SSectorTextureSet * pParams, Matrix44 * pMatComposite, int nIndexTexSlot = -1, Vec4 * vOffset = NULL, bool bEnableStencil = false, bool bNoColorWrite = false );
	void GenerateAO(CTexture *pSSAORT, CCryName const& TechName);
	void SetDepthBoundTest(float fMin, float fMax, bool bEnable);
	void CreateDeferredUnitBox(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);
	void PrepareDepthMap(ShadowMapFrustum* SMSource, CDLight* pLight=NULL, int nLightFrustumID = 0);
	void ConfigShadowTexgen(int Num, ShadowMapFrustum * pFr, int nResView=-1, int nFrustNum = -1, const CDLight* pLight = NULL);
	virtual void SetupShadowOnlyPass(int Num, ShadowMapFrustum* pShadowInst, Matrix34A * pObjMat);
	void DrawAllShadowsOnTheScreen();
	void DrawAllDynTextures(const char *szFilter, const bool bLogNames, const bool bOnlyIfUsedThisFrame);
	void OnEntityDeleted(IRenderNode * pRenderNode);

	//=============================================================

	int msCurState;

	void D3DSetCull(ECull eCull);
	const char *D3DError( HRESULT h );
	bool Error(char *Msg, HRESULT h);

	void SetDeviceGamma(uint16 *r, uint16 *g, uint16 *b);
	HRESULT AdjustWindowForChange();

private:

	bool SetWindow(int width, int height, bool fullscreen, WIN_HWND hWnd);
	bool SetRes();
	void UnSetRes();
	void DisplaySplash(); //!< Load a bitmap from a file, blit it to the windowdc and free it

	void RecognizePixelFormat(SPixFormat& Dest, uint32 FormatFromD3D, int InBitsPerPixel, const char* InDesc, bool bAutoGenMips, bool bCanReadSRGB);

	void DestroyWindow(void);
	virtual void RestoreGamma(void);
	void SetGamma(float fGamma, float fBrigtness, float fContrast, bool bForce);

	struct texture_info
	{
		texture_info() { ZeroStruct(*this); }
		char filename[256];
		int  bind_id;
		int  low_tid;
		int  type;
		int  last_time_used;
	};
	PodArray<texture_info> m_texture_registry;
	int FindTextureInRegistry(const char * filename, int * tex_type);
	int RegisterTextureInRegistry(const char * filename, int tex_type, int tid, int low_tid);
	unsigned int MakeTextureREAL(const char * filename,int *tex_type, unsigned int load_low_res);
	unsigned int CheckTexturePlus(const char * filename, const char * postfix);

#if defined (DIRECT3D9)
	static bool    CALLBACK OnD3D9DeviceChanging(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
	static HRESULT CALLBACK OnD3D9CreateDevice(LPDIRECT3DDEVICE9 pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static HRESULT CALLBACK OnD3D9ResetDevice(LPDIRECT3DDEVICE9 pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static bool    CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
	static void    CALLBACK OnD3D9FrameRender(LPDIRECT3DDEVICE9 pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
	static void    CALLBACK OnD3D9LostDevice(void* pUserContext );
	static void    CALLBACK OnD3D9DestroyDevice(void* pUserContext );
	static HRESULT CALLBACK OnD3D9PostCreateDevice(D3DDevice* pd3dDevice);
#elif defined (DIRECT3D10)
#if !defined(PS3)
	static bool    CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
	static HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
	static void    CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext);
	static void    CALLBACK OnD3D11DestroyDevice(void* pUserContext);
	static void    CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext);
	static bool    CALLBACK OnD3D11DeviceChanging(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
#endif
	static HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
#endif

#if defined (DIRECT3D9) && !defined (XENON)
	void SuccessfullyLaunchFromMediaCenter() const;
#endif

public:  
	static HRESULT CALLBACK OnD3D10PostCreateDevice(D3DDevice* pd3dDevice);

	D3DXMATRIX m_matPsmWarp;
	D3DXMATRIX m_matViewInv;
	int m_MatDepth;

	static int CV_d3d9_debugruntime;

	static int CV_d3d9_nvperfhud;
	static int CV_d3d9_vbpools;
	static int CV_d3d9_vbpoolsize;
	static int CV_d3d9_ibpools;
	static int CV_d3d9_ibpoolsize;
	static ICVar *CV_d3d9_texturefilter;
	static int CV_d3d9_allowsoftware;
	static int CV_d3d9_rb_verts;
	static int CV_d3d9_rb_tris;
	static int CV_d3d9_null_ref_device;
	static int CV_d3d9_clipplanes;
	static int CV_d3d9_triplebuffering;
	static int CV_d3d9_resetdeviceafterloading;

#if defined (DIRECT3D10)
	static int CV_d3d10_CBUpdateStats;
	static int CV_d3d10_NumStagingBuffers;
#endif

	//============================================================
	// Renderer interface

	bool m_bInitialized;
	string m_Description;
	bool m_bFullScreen;

	TArray<SD3DContext *> m_RContexts;
	SD3DContext *m_CurrContext;

	TArray<CTexture*> m_RTargets;
	int m_nRecurs;

public:

	// Multithreading support
	virtual void RT_BeginFrame();
	virtual void RT_EndFrame();
	virtual void RT_Init();
	virtual bool RT_CreateDevice();
	virtual void RT_Reset();
	virtual void RT_RenderScene(int nFlags, SThreadInfo& TI, RenderFunc pRenderFunc, SRenderListDesc* pRLD);
	virtual void RT_SetCull(int nMode);
	virtual void RT_SetCameraInfo();
	virtual void RT_CreateResource(SResourceAsync* Res);
	virtual void RT_ReleaseResource(SResourceAsync* pRes);
	virtual void RT_ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX, int nScaledY);
	virtual HRESULT RT_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle);
	virtual HRESULT RT_CreateIndexBuffer(UINT Length, DWORD Usage, DWORD Format, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle);
	virtual HRESULT RT_CreateVertexShader(DWORD *pBuf, void **pShader, void *pInst);
	virtual HRESULT RT_CreatePixelShader(DWORD *pBuf, void **pShader);
	virtual void RT_ReleaseVBStream(void *pVB, int nStream);
	virtual void RT_ReleaseCB(void *pCB);
	virtual void RT_DrawDynVB(int nOffs, int Pool, int nVerts);
	virtual void RT_DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType);
	virtual void RT_Draw2dImage(float xpos,float ypos,float w,float h,int textureid,float s0,float t0,float s1,float t1,float angle,DWORD col,float z);
	virtual void RT_DrawImageWithUV(float xpos,float ypos,float z,float w,float h,int texture_id,float *s,float *t,DWORD col);
	virtual void RT_PushRenderTarget(int nTarget, CTexture *pTex, SD3DSurface *pDepth, int nS);
	virtual void RT_PopRenderTarget(int nTarget);
	virtual	void RT_SetViewport(int x, int y, int width, int height);

	//===============================================================================

	virtual WIN_HWND Init(int x,int y,int width,int height,unsigned int cbpp, int zbpp, int sbits, bool fullscreen,WIN_HINSTANCE hinst, WIN_HWND Glhwnd=0, bool bReInit=false, const SCustomRenderInitArgs* pCustomArgs=0);

	virtual void GetVideoMemoryUsageStats( size_t& vidMemUsedThisFrame, size_t& vidMemUsedRecently, bool bGetPoolsSizes = false );

	virtual bool SetCurrentContext(WIN_HWND hWnd);
	virtual bool CreateContext(WIN_HWND hWnd, bool bAllowFSAA=false);
	virtual bool DeleteContext(WIN_HWND hWnd);
	virtual void MakeMainContextActive();
	virtual WIN_HWND GetCurrentContextHWND() { return m_CurrContext ? m_CurrContext->m_hWnd : m_hWnd; }

	virtual int  CreateRenderTarget (int nWidth, int nHeight, ETEX_Format eTF=eTF_A8R8G8B8);
	virtual bool DestroyRenderTarget (int nHandle);
	virtual bool SetRenderTarget (int nHandle, int nFlags=0);

	virtual bool ChangeDisplay(unsigned int width,unsigned int height,unsigned int cbpp);
	virtual void ChangeViewport(unsigned int x,unsigned int y,unsigned int width,unsigned int height);
	virtual int	 EnumDisplayFormats(SDispFormat *formats);
	//! Return all supported by video card video AA formats
	virtual int	EnumAAFormats(const SDispFormat &rDispFmt, SAAFormat *formats);

	//! Changes resolution of the window/device (doen't require to reload the level
	virtual bool	ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, int nNewRefreshHZ, bool bFullScreen, bool bForceReset);
	virtual void	Reset(void);

	virtual void BeginFrame();
	virtual void ShutDown(bool bReInit=false);
	virtual void ShutDownFast();
	virtual void RenderDebug();  

	void DebugPrintShader(class CHWShader_D3D *pSH, void *pInst, int nX, int nY, ColorF colSH);
	void DebugPerfBars(int nX, int nY);
	void DebugVidResourcesBars(int nX, int nY);
	void DebugDrawStats1();
	void DebugDrawStats2();
	void DebugDrawStats();
	void DebugDrawRect(float x1,float y1,float x2,float y2,float *fColor);
	void VidMemLog();

	virtual void EndFrame();  
	virtual void GetMemoryUsage(ICrySizer* Sizer);
	void GetLogVBuffers();
	virtual void Draw2dImage(float xpos,float ypos,float w,float h,int textureid,float s0=0,float t0=0,float s1=1,float t1=1,float angle=0,float r=1,float g=1,float b=1,float a=1,float z=1);
	virtual void DrawImage(float xpos,float ypos,float w,float h,int texture_id,float s0,float t0,float s1,float t1,float r,float g,float b,float a);
	virtual void DrawImageWithUV(float xpos,float ypos,float z,float w,float h,int texture_id,float *s,float *t,float r,float g,float b,float a);
	virtual void SetCullMode(int mode=R_CULL_BACK);

	virtual bool EnableFog(bool enable);
	virtual void SetFog(float density, float fogstart, float fogend, const float *color, int fogmode);

	virtual void SelectTMU(int tnum);
	virtual void EnableTMU(bool enable);

	virtual void CreateResourceAsync(SResourceAsync* pResource);
	virtual void ReleaseResourceAsync(SResourceAsync* pResource);
	virtual unsigned int DownLoadToVideoMemory(unsigned char *data,int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat=true, int filter=FILTER_BILINEAR, int Id=0, const char *szCacheName=NULL, int flags=0, EEndian eEndian = eLittleEndian, RectI * pRegion = NULL, bool bAsynDevTexCreation = false);
	virtual	void UpdateTextureInVideoMemory(uint32 tnum, unsigned char *newdata,int posx,int posy,int w,int h,ETEX_Format eTF=eTF_X8R8G8B8);
	virtual bool EF_PrecacheResource(ITexture *pTP, float fMipFactor, float fTimeToReady, int Flags, int nUpdateId);
	virtual void PrecacheResources();
	virtual void RemoveTexture(unsigned int TextureId);

	void SetRCamera(const CRenderCamera &cam);
	virtual void SetCamera(const CCamera &cam);
	virtual	void SetViewport(int x, int y, int width, int height);
	virtual void GetViewport(int *x, int *y, int *width, int *height);
	virtual	void SetScissor(int x=0, int y=0, int width=0, int height=0);
	virtual	void SetRenderTile(f32 nTilesPosX,f32 nTilesPosY,f32 nTilesGridSizeX,f32 nTilesGridSizeY);

	virtual void Draw3dBBox(const Vec3 &mins,const Vec3 &maxs, int nPrimType);
	virtual void Draw3dPrim(const Vec3 &mins,const Vec3 &maxs, int nPrimType, const float* pColor = NULL);
	virtual void Flush3dBBox(const Vec3 &mins,const Vec3 &maxs,const bool bSolid);

	void DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround);
	void DrawLineColor(const Vec3 & vPos1, const ColorF & vColor1, const Vec3 & vPos2, const ColorF & vColor2);
	virtual void DrawLine(const Vec3 & vPos1, const Vec3 & vPos2);
	virtual void Graph(byte *g, int x, int y, int wdt, int hgt, int nC, int type, char *text, ColorF& color, float fScale);

	virtual void *GetDynVBPtr(int nVerts, int &nOffs, int Pool);
	virtual void DrawDynVB(int nOffs, int Pool, int nVerts);
	virtual void DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType);

	virtual void	DrawBuffer(CVertexBuffer *pVBuf, CIndexBuffer *pIBuf, int nNumIndices, int nOffsIndex, int nPrmode, int nVertStart=0, int nVertStop=0);

	virtual void CheckError(const char *comment);
	virtual int SetWireframeMode(int mode);

	virtual void PushMatrix();
	virtual void PopMatrix();

	virtual void EnableVSync(bool enable);
	virtual void DrawPrimitives(CVertexBuffer *src, int vert_num=4, int prim_type = R_PRIMV_TRIANGLE_STRIP);
	virtual void ResetToDefault();

	virtual void SetMaterialColor(float r, float g, float b, float a);
	virtual char * GetStatusText(ERendStats type);

	virtual void ProjectToScreen(float ptx, float pty, float ptz,float *sx, float *sy, float *sz );
	virtual int UnProject(float sx, float sy, float sz, float *px, float *py, float *pz, const float modelMatrix[16], const float projMatrix[16], const int    viewport[4]);
	virtual int UnProjectFromScreen( float  sx, float  sy, float  sz, float *px, float *py, float *pz);

	virtual void GetModelViewMatrix(float * mat);
	virtual void GetProjectionMatrix(float * mat);
	virtual void SetMatrices(float *pProjMat, float *pViewMat);

	virtual void DrawQuad(const Vec3 &right, const Vec3 &up, const Vec3 &origin,int nFlipMode=0);
	virtual void DrawQuad(float dy,float dx, float dz, float x, float y, float z);
	void    DrawQuad(float x0, float y0, float x1, float y1, const ColorF & color, float z = 1.0f, float s0=0, float t0=0, float s1=1, float t1=1);
	void DrawQuad3D(const Vec3 & v0, const Vec3 & v1, const Vec3 & v2, const Vec3 & v3, const ColorF & color, 
		float ftx0 = 0,  float fty0 = 0,  float ftx1 = 1,  float fty1 = 1);
	void DrawFullScreenQuad(CShader *pSH, const CCryName& TechName, float s0, float t0, float s1, float t1, uint32 nState = GS_NODEPTHTEST);

	//fog	
	void SetFogColor(float * color);

	virtual void SetPerspective(const CCamera &cam);

	virtual void ClearBuffer(uint32 nFlags, ColorF *vColor, float depth = 1.0f);
	virtual void ReadFrameBuffer(unsigned char * pRGB, int nImageX, int nSizeX, int nSizeY, ERB_Type eRBType, bool bRGBA, int nScaledX=-1, int nScaledY=-1);  
	virtual void ReadFrameBufferFast(uint32* pDstARGBA8, int dstWidth, int dstHeight);

	//misc	
	virtual bool ScreenShot(const char *filename=NULL, int width=0);  

	virtual uint32 RenderOccludersIntoBuffer(const CCamera & viewCam, int nTexSize, PodArray<IRenderNode*> & lstOccluders, float * pBuffer);

	virtual void UnloadOldTextures(){};

	virtual void Set2DMode(bool enable, int ortox, int ortoy,float znear=-1e10f,float zfar=1e10f);

	virtual int ScreenToTexture(int nTexID);

	virtual void EnableAALines(bool bEnable);

	virtual	bool	SetGammaDelta(const float fGamma);

	//////////////////////////////////////////////////////////////////////
	// Replacement functions for the Font engine
	virtual	bool FontUploadTexture(class CFBitmap*, ETEX_Format eTF=eTF_A8R8G8B8);
	virtual	int  FontCreateTexture(int Width, int Height, byte *pData, ETEX_Format eTF=eTF_A8R8G8B8, bool genMips=false);
	virtual	bool FontUpdateTexture(int nTexId, int X, int Y, int USize, int VSize, byte *pData);
	virtual	void FontReleaseTexture(class CFBitmap *pBmp);
	virtual void FontSetTexture(class CFBitmap*, int nFilterMode);
	virtual void FontSetTexture(int nTexId, int nFilterMode);
	virtual void FontSetRenderingState(unsigned int nVirtualScreenWidth, unsigned int nVirtualScreenHeight);
	virtual void FontSetBlending(int src, int dst);
	virtual void FontRestoreRenderingState();

	void FontSetState(bool bRestore);

	//////////////////////////////////////////////////////////////////////
	void MakeSprites(TArray<SSpriteGenInfo>& SGI);
	virtual void DrawObjSprites (PodArray<struct SVegetationSpriteInfo> *pList);
	virtual void GenerateObjSprites(PodArray<struct SVegetationSpriteInfo> *pList);
	void DrawObjSprites (SSpriteInfo *pSPInfo, const int nSPI, bool bZ, bool bShadows);
	void ObjSpritesFlush (SVF_P3F_C4B_T2F *pVerts, uint16 *pInds, int numSprites, void *&pCurVB, SShaderTechnique *pTech);

#ifndef EXCLUDE_SCALEFORM_SDK
	void SF_CreateResources();
	void SF_DestroyResources();
	void SF_ResetResources();
	bool SF_SetVertexDeclaration(SSF_GlobalDrawParams::EVertexFmt vertexFmt);
	CShader* SF_SetTechnique(const CCryName& techName);
	void SF_SetBlendOp(SSF_GlobalDrawParams::EAlphaBlendOp blendOp, bool reset = false);
	uint32 SF_AdjustBlendStateForMeasureOverdraw(uint32 blendModeStates);

	virtual void SF_DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int triangleCount, const SSF_GlobalDrawParams& params);
	virtual void SF_DrawLineStrip(int baseVertexIndex, int lineCount, const SSF_GlobalDrawParams& params);
	virtual void SF_DrawGlyphClear(const SSF_GlobalDrawParams& params);
	virtual void SF_Flush();
	virtual bool SF_UpdateTexture(int texId, int mipLevel, int numRects, const SUpdateRect* pRects, unsigned char* pData, size_t pitch, ETEX_Format eTF);
#endif // #ifndef EXCLUDE_SCALEFORM_SDK

	virtual void SetProfileMarker(const char* label, ESPM mode);

	//////////////////////////////////////////////////////////////////////

	void FX_ApplyThreadState(SThreadInfo& TI, SThreadInfo *OldTI);
	void EF_RenderScene(int nFlags, SViewport& VP);
	void EF_Scene3D(SViewport& VP, int nFlags);

	void	CheckFSAAChange(void);

	// FX Shaders pipeline
	//#if !defined(XENON) && !defined(PS3)
	void FX_DrawInstances(CShader *ef, SShaderPass *slw, uint32 nCurInst, uint32 nLastInst, uint32 nUsedAttr, int nInstAttrMask, byte Attributes[], uint32 nConstBasedInstancing);
	void FX_DrawShader_InstancedHW(CShader *ef, SShaderPass *slw);
	//#endif
	void FX_DrawShader_General(CShader *ef, SShaderTechnique *pTech, bool bUseZState, bool bUseMaterialState);
	void FX_DrawShader_Terrain(CShader *ef, SShaderTechnique *pTech);  
	void FX_DrawMultiLightPasses(CShader *ef, SShaderTechnique *pTech, int nShadowChans);
	void FX_DrawShadowPasses(CShader *ef, SShaderTechnique *pTech, int nChanNum);
	bool FX_DrawToRenderTarget(CShader *pShader, SRenderShaderResources* pRes, CRenderObject *pObj, SShaderTechnique *pTech, SHRenderTarget *pTarg, int nPreprType, CRendElementBase *pRE);
	void FX_ScreenStretchRect( CTexture *pDst );

	// build terrain sector texture
	virtual void BuildTerrainTexture();
	virtual IDynTexture * RequestTerrainTexture(struct SBuildTerrainTextureParams & params);
	byte * GetTextureSubImageData32(byte * pData, int nDataSize, int nX, int nY, int nW, int nH, CTexture * pTex);
	virtual void ProcessVoxTerrainDebugTarget(int * pSrcTexIds, int nMipsNum, bool bOnlySetSrcTexture);
	PodArray<int> m_arrVoxTerrainDebugSrcTexId;

	virtual bool GetOcclusionBuffer(uint16* pOutOcclBuffer, int32 nSizeX, int32 nSizeY, Matrix44* pmViewProj) { return true; }

	void FX_StencilTestCurRef(bool bEnable);
	void FX_HiStencilUpdate(int nVertexOffset, int nNumVers=-1, int nIndOffs=-1, int nNumInds=-1, int stencilFunc = STENC_FUNC(FSS_STENCFUNC_EQUAL));
	void FX_ResolveDepthTarget(CTexture* pSrcZTarget, SD3DSurface* pDstDepthBuffer);
	void FX_StencilLights(SLightPass *pLP);
	void FX_SetLightsScissor(SLightPass *pLP, const RectI* pDef);
	int  FX_SetupLightPasses(SShaderTechnique *pTech);
	bool FX_ProcessAOTarget();
	void FX_ProcessShadows(int nGroup, int nAW);
	bool FX_ProcessShadowsListsForLightGroup(int nGroup, int nOffsRI);
	void EF_PrepareAllDepthMaps();
	void FX_StencilCullPass(int nStencilID, int nVertOffs, int nNumVers, int nIndOffs, int nNumInds);
	void FX_StencilFrustumCull(int nStencilID, const CDLight* pLight, ShadowMapFrustum* pFrustum, int nAxis);
	void FX_StencilCull(int nStencilID, t_arrDeferredMeshIndBuff& arrDeferredInds, t_arrDeferredMeshVertBuff& arrDeferredVerts, CShader *pShader);
	void FX_StencilRefresh(int StencilFunc, uint32 nStencRef, uint32 nStencMask);
	void FX_StencilRefreshPartial(int StencilFunc, uint32 nStencRef, uint32 nStencMask,
		Vec3 *pVerts, uint32 nNumVerts, uint16 *pInds, uint32 nNumInds);
	void FX_StencilCullPassForLightGroup(int nLightGroup);
	void FX_ZCullReload();
	bool CreateAuxiliaryMeshes();
	bool ReleaseAuxiliaryMeshes();
	bool CreateUnitVolumeMesh(t_arrDeferredMeshIndBuff& arrDeferredInds, t_arrDeferredMeshVertBuff& arrDeferredVerts, D3DIndexBuffer*& pUnitFrustumIB, D3DVertexBuffer*& pUnitFrustumVB);
	bool CreateUnitLightMeshes();
	void FX_CreateDeferredQuad(int& nOffs, const CDLight* pLight, float maskRTWidth, float maskRTHeight, Matrix44A* pmCurView, Matrix44A* pmCurComposite);
	void FX_DeferredShadowPass(const CDLight* pLight, int nLightInGroup, ShadowMapFrustum *pShadowFrustum, float fFinalRange, int nVertexOffset, bool bShadowPass, bool bStencilPrepass, int nLod, int nFrustNum = -1);
	bool FX_DeferredProjLights(int nGroup, bool bCheckValidMask);
	bool FX_DeferredShadowPassSetup(const Matrix44& mShadowTexGen, float maskRTWidth, float maskRTHeight);
	bool FX_DeferredShadows( int nGroup, SRendLightGroup* pGr, int maskRTWidth, int maskRTHeight );
	void FX_SetDeferredShadows();
	bool FX_ProcessLightsListForLightGroup(int nGroup, SRendLightGroup *pGr, int nOffsRI);
	void FX_ShadowBlur(float fShadowBluriness, SDynTexture *tpSrc, CTexture *tpDst, int iShadowMode=-1, bool bScreenVP=false, CTexture *tpDst2=NULL);
	bool FX_SetShadowMaskRT(int nGroup, int nBlurMode, CTexture *&tpSrc, SDynTexture *&pTempBlur);

	void FX_DrawDetailOverlayPasses();
	void FX_DrawEffectLayerPasses();
	void FX_DrawCausticsPasses();
	void FX_DrawDebugPasses();

	void FX_SetupMultiLayers( bool bEnable = false );
	void FX_DrawMultiLayers( );  

	void FX_DrawTechnique(CShader *ef, SShaderTechnique *pTech, bool bGeneral, bool bUseMaterialState);

#if !defined(XENON) && !defined(PS3)
	bool FX_PrepareLightInfoTexture(bool bEnable);
	void EF_InitLightInfotable_DB();
#endif

	bool FX_HDRScene(bool bEnable, bool bClear = true, bool bEncoded = false, bool bDecodePass = false);
	void FX_HDRRangeAdaptUpdate();

	bool FX_ZPrePassScene(bool bEnable, bool bClearZBuffer);  
	bool FX_ZScene(bool bEnable, bool bUseHDR, bool bClearZBuffer, bool bRenderNormalsOnly = 0);  
	bool FX_FogScene();
	bool FX_DeferredCaustics();
	bool FX_DeferredRainLayer();
	bool FX_GlowScene(bool bEnable);  
	bool FX_ScatterScene(bool bEnable, bool ScatterPass);
	bool FX_DisplayFogScene();  
	bool FX_MotionBlurScene(bool bEnable);  
	bool FX_CustomRenderScene(bool bEnable);  
	void FX_PostProcessSceneHDR();
	bool FX_PostProcessScene(bool bEnable);
	bool FX_DeferredRendering( bool bDebugPass = false, bool bUpdateRTOnly = false); 

	void Hint_DontSync( CTexture &rTex );

	// Performance queries
	//=======================================================================

	virtual float GetGPUFrameTime();
	virtual void GetRenderTimes(SRenderTimes &outTimes);

	// Shaders pipeline
	//=======================================================================
	virtual void EF_PipelineShutdown();
	virtual void EF_ClearBuffers(uint32 nFlags, const ColorF *Colors, float fDepth=1.0f);

	void EF_Invalidate();
	void EF_Restore();
	void EF_SetHiZState(int newHiZState, int curState, int curStencilState);
	virtual void EF_SetState(int st, int AlphaRef=-1, int RestoreState = 0);
	void FX_StateRestore(int prevState);

	void ChangeLog();

	void SetLogFuncs(bool bEnable);

	bool CreateMSAADepthBuffer();
	void FlushHardware(bool bIssueBeforeSync);
	void PostMeasureOverdraw();
	void DrawTexelsPerMeterInfo();
	virtual bool CheckDeviceLost();

	short m_nPrepareShadowFrame;

	_inline void EF_DirtyMatrix()
	{
		int nThreadID = m_pRT->GetThreadList();
		m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_FP_MATRIXDIRTY | RBPF_FP_DIRTY;
		m_RP.m_FrameObject++;
	}
	_inline void EF_PushMatrix()
	{
		int nThreadID = m_pRT->GetThreadList();
		m_RP.m_TI[nThreadID].m_matView->Push();
		m_MatDepth++;
	}
	_inline void EF_PopMatrix()
	{
		int nThreadID = m_pRT->GetThreadList();
		m_RP.m_TI[nThreadID].m_matView->Pop();
		m_MatDepth--;
		EF_DirtyMatrix();
	}
	void FX_ResetPipe();

	inline void FX_EnableATOC()
	{
		assert(m_pRT->GetThreadList() == m_RP.m_nProcessThreadID);
		if (CV_r_atoc && m_bDeviceSupportsATOC && m_RP.m_FSAAData.Type)
		{
			if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC))
			{
				m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_ATOC;
#if defined (DIRECT3D9)
#if  !defined (XENON)
				m_pd3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, CV_r_atoc == 1 ? MAKEFOURCC('A', 'T', 'O', 'C') : MAKEFOURCC('S', 'S', 'A', 'A')); 
#endif
#endif
			}
		}
	}
	inline void FX_DisableATOC()
	{
		assert(m_pRT->GetThreadList() == m_RP.m_nProcessThreadID);

		if (m_bDeviceSupportsATOC)
		{
			assert(m_RP.m_nProcessThreadID < 2);
			if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC)
			{
				m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_ATOC;
#if defined (DIRECT3D9)
#if  !defined (XENON)
				m_pd3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, 0); 
#endif
#endif
			}
		}
	}

	_inline void EF_SetGlobalColor(float r, float g, float b, float a)
	{
		assert(m_pRT->IsRenderThread());

		EF_SelectTMU(0);
		EF_SetColorOp(255, 255, eCA_Texture | (eCA_Constant<<3), eCA_Texture | (eCA_Constant<<3));
		if (m_RP.m_CurGlobalColor[0] != r || m_RP.m_CurGlobalColor[1] != g || m_RP.m_CurGlobalColor[2] != b || m_RP.m_CurGlobalColor[3] != a)
		{
			m_RP.m_CurGlobalColor[0] = r;
			m_RP.m_CurGlobalColor[1] = g;
			m_RP.m_CurGlobalColor[2] = b;
			m_RP.m_CurGlobalColor[3] = a;
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
		}
	}
	_inline void EF_SetVertColor()
	{
		EF_SelectTMU(0);
		EF_SetColorOp(255, 255, eCA_Texture | (eCA_Diffuse<<3), eCA_Texture | (eCA_Diffuse<<3));
	}
	_inline void EF_DisableTextureAndColor()
	{
		assert(m_pRT->IsRenderThread());
		if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage]&7) == eCA_Texture || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage]&7) == eCA_Diffuse)
		{
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage] = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage] & ~7) | eCA_Constant;
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
		}
		if (((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage]>>3)&7) == eCA_Texture || ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage]>>3)&7) == eCA_Diffuse)
		{
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage] = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurColorArg[CTexture::s_CurStage] & ~0x38) | (eCA_Constant<<3);
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
		}
		if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage]&7) == eCA_Texture || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage]&7) == eCA_Diffuse)
		{
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage] = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage] & ~7) | eCA_Constant;
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
		}
		if (((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage]>>3)&7) == eCA_Texture || ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage]>>3)&7) == eCA_Diffuse)
		{
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage] = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_eCurAlphaArg[CTexture::s_CurStage] & ~0x38) | (eCA_Constant<<3);
			m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_FP_DIRTY;
		}
	}

	_inline void EF_SelectTMU(int Stage)
	{
		CTexture::s_CurStage = Stage;
	}

	//================================================================================

	HRESULT FX_SetVertexDeclaration(int StreamMask, EVertexFormat eVF);
	inline bool FX_SetStreamFlags(SShaderPass *pPass)
	{
		bool bSkinned = false;
		//m_RP.m_FlagsStreams |= pPass->m_Flags & (VSM_MASK & ~VSM_HWSKIN & ~VSM_HWSKIN_SHAPEDEFORM & ~VSM_HWSKIN_MORPHTARGET);
		//#if !defined(PS3) || defined(PS3_ACTIVATE_CONSTANT_ARRAYS)//we cannot map it yet, will be removed soon
		if (CV_r_usehwskinning && m_RP.m_pRE && m_RP.m_pRE->mfIsHWSkinned())
		{
			m_RP.m_FlagsStreams_Decl |= VSM_HWSKIN;
			m_RP.m_FlagsStreams_Stream |= VSM_HWSKIN;
			bSkinned = true;
		}
		//#endif
		return bSkinned;
	}
	HRESULT FX_SetTextureAsVStream(int nID, CTexture *pVBTexture, uint32 nStride);
	HRESULT FX_SetVStream(int nID, void *pB, uint32 nOffs, uint32 nStride, uint32 nFreq=1);
	HRESULT FX_SetIStream(void* pB);

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	void FX_TagVStreamAsDirty(int nID);
	void FX_TagIStreamAsDirty();
	void FX_TagVertexDeclarationAsDirty();
#endif

	void FX_FlushSkinVSParams(CHWShader_D3D *pVS, int nFirst, int nBones, int nOffsVS, int numBonesPerChunk, int nSlot, QuatTS *pSkinQuats, QuatTS *pMBSkinQuats);
	void FX_DrawBatches(CShader *pSh, SShaderPass *pPass, CHWShader_D3D *curVS, CHWShader_D3D *curPS);
	void FX_DrawBatchSkinned(CShader *pSh, SShaderPass *pPass, CHWShader_D3D *curVS, CHWShader_D3D *curPS);

	//========================================================================================

	void FX_SetActiveRenderTargets(bool bAllowDIP=false);
	void FX_ClearRegion();

#define MAX_RT_STACK 8
	struct SRTStack
	{
		D3DSurface* m_pTarget;
		D3DDepthSurface* m_pDepth;

		CTexture *m_pTex;
		SD3DSurface *m_pSurfDepth;
		int m_Width;
		int m_Height;
		uint32 m_bNeedReleaseRT : 1;
		uint32 m_bWasSetRT : 1;
		uint32 m_bWasSetD : 1;
		uint32 m_bClearOnResolve : 1;
		uint32 m_bDontDraw : 1;
		uint32 m_bScreenVP : 1;

		uint32 m_ClearFlags;
		ColorF m_ReqColor;
		float m_fReqDepth;
	};
	int m_nRTStackLevel[4];
	SRTStack m_RTStack[4][MAX_RT_STACK];

	int m_nMaxRT2Commit;
	SRTStack* m_pNewTarget[4];
	CTexture* m_pCurTarget[4];

	TArray<SD3DSurface *> m_TempDepths;

	void FX_GetRTDimensions(bool bRTPredicated, int& nWidth, int& nHeight, bool bForcePredicated = false);
	bool FX_GetTargetSurfaces(CTexture *pTarget, D3DSurface*& pTargSurf, SRTStack *pCur, int nCMSide=-1, int nTarget = 0, bool bFP16=false, uint32 nTileCount = 1);
	bool FX_SetRenderTarget(int nTarget, D3DSurface *pTargetSurf, SD3DSurface *pDepthTarget, bool bClearOnResolve=false, uint32 nTileCount = 1);
	bool FX_PushRenderTarget(int nTarget, D3DSurface *pTargetSurf, SD3DSurface *pDepthTarget, bool bClearOnResolve=false, uint32 nTileCount = 1);
	bool FX_SetRenderTarget(int nTarget, CTexture *pTarget, SD3DSurface *pDepthTarget, bool bPush=false, bool bClearOnResolve=false, int nCMSide=-1, bool bScreenVP=false, uint32 nTileCount = 1);
	bool FX_PushRenderTarget(int nTarget, CTexture *pTarget, SD3DSurface *pDepthTarget, bool bClearOnResolve=false, int nCMSide=-1, bool bScreenVP=false, uint32 nTileCount = 1);
	bool FX_RestoreRenderTarget(int nTarget);
	bool FX_PopRenderTarget(int nTarget);

	SD3DSurface *FX_GetDepthSurface(int nWidth, int nHeight, bool bAA);

	SD3DSurface *FX_GetScreenDepthSurface(bool bAA=false);

	//========================================================================================


	void FX_Commit(bool bAllowDIP=false);
	void FX_ZState(int& nState);
	bool FX_SetFPMode();
	inline void *FX_LockVB(uint32 nSize, uint32& nStart);
	inline void FX_UnlockVB();

	void FX_CommitStates(SShaderTechnique *pTech, SShaderPass *pPass, bool bUseMaterialState);

	_inline void FX_DrawBatch(CShader *sh, SShaderPass *sl)
	{
		int bFogOverrided = 0;
		// Unlock all VB (if needed) and set current streams
		FX_CommitStreams(sl);
		{
			bFogOverrided = EF_FogCorrection();
			if (m_RP.m_pRE)
				m_RP.m_pRE->mfDraw(sh, sl);
			else
				FX_DrawIndexedMesh(R_PRIMV_TRIANGLES);
			EF_FogRestore(bFogOverrided);
		}
	}

	bool FX_CommitStreams(SShaderPass *sl, bool bSetVertexDecl=true);

	// get the anti aliasing formats available for current width, height and BPP
	int GetAAFormat(TArray<SAAFormat>& Formats, bool bReset);

	void EF_SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa);

	// Clip Planes support 
	void FX_SetClipPlane(bool bEnable, float *pPlane, bool bRefract);

	void FX_HDRPostProcessing();

	void FX_PreRender(int Stage);
	void FX_PostRender();

	void EF_Init();

	void EF_InitWaveTables();

	// Arguments
	//   vertexformat - 0..VERTEX_FORMAT_NUMS-1
	void EF_OnDemandVertexDeclaration( SOnDemandD3DVertexDeclaration &out, const int nStreamMask, const int vertexformat, const bool bMorph );

	void EF_InitD3DVertexDeclarations();
	void EF_SetCameraInfo();
	void FX_SetObjectTransform(CRenderObject *obj, CShader *pSH, int nTransFlags);
	bool FX_ObjectChange(CShader *Shader, SRenderShaderResources *pRes, CRenderObject *pObject, CRendElementBase *pRE);

	void EF_DrawDebugLights();
	void EF_DrawDebugTools(SViewport& VP);

	static void FX_DrawWire();
	static void FX_DrawNormals();
	static void FX_DrawTangents();

	static void FX_FlushShader_ShadowGen();
	static void FX_FlushShader_General();
	static void FX_FlushShader_ZPass();
	static void FX_FlushShader_ShadowPass();

	static void FX_SelectTechnique(CShader *pShader, SShaderTechnique *pTech);

	_inline void FX_Flush()
	{
#ifdef DIRECT3D9
		if (m_pQueryPerf && CV_r_profileGPU)
		{
			m_pQueryPerf->Issue(D3DISSUE_END);
			float fTime = iTimer->GetAsyncCurTime();
			bool bInfinite = false;
			HRESULT hr;
			do
			{
				float fDif = iTimer->GetAsyncCurTime() - fTime;
				if (fDif > 2.0f)
				{
					// 2 seconds in the loop
					bInfinite = true;
					break;
				}
				DWORD dwBuf;
				hr = m_pQueryPerf->GetData(&dwBuf, 4, D3DGETDATA_FLUSH);
			} while(hr == S_FALSE);
		}
#endif
	}
	void FX_Flush_ForProfiler(int nEnd);
	int m_sPrevX, m_sPrevY, m_sPrevWdt, m_sPrevHgt;
	bool m_bsPrev;
	void EF_Scissor(bool bEnable, int sX, int sY, int sWdt, int sHgt);

	void EF_SetFogColor(ColorF &Color);
	uint32 EF_FogCorrection();
	void EF_FogRestore(uint32 bFogOverrided);

	byte FX_StartQuery(SRendItem *pRI);
	void FX_EndQuery(SRendItem *pRI, byte bStartQ);

	void FX_TrackStats( CRenderObject *pObj );

	void FX_DrawIndexedMesh (int nPrimType);
	bool FX_DebugCheckConsistency(int FirstVertex, int FirstIndex, int RendNumVerts, int RendNumIndices);
	bool FX_SetResourcesState();

	int  EF_Preprocess(SRendItem *ri, uint32 nums, uint32 nume, RenderFunc pRenderFunc);
	void EF_SortRenderLists(SRenderListDesc *pRLD, int nThreadID);
	void EF_SortRenderList(int nList, int nAW, SRenderListDesc *pRLD, int nThread);

	void FX_StartBatching();
	void FX_ProcessBatchesList(int nums, int nume, uint32 nBatchFilter);
	void FX_ProcessZPrePassRenderLists();
	void FX_ProcessZPassRenderLists();
	void FX_ProcessScatterRenderLists();
	void FX_ProcessRainRenderLists(int nList, int nAW, void (*RenderFunc)());
	void FX_ProcessFurRenderLists(int nList, int nAW, void (*RenderFunc)());
	void FX_ProcessPostRenderLists(uint32 nBatchFilter);
	void FX_ProcessRenderList(int nList, uint32 nBatchFilter);
	void FX_ProcessRenderList(int nums, int nume, int nList, int nAW, void (*RenderFunc)(), bool bLighting);
	_inline void FX_ProcessRenderList(int nList, int nAW, void (*RenderFunc)(), bool bLighting)
	{
		FX_ProcessRenderList(m_RP.m_pRLD->m_nStartRI[nAW][nList], m_RP.m_pRLD->m_nEndRI[nAW][nList], nList, nAW, RenderFunc, bLighting);
	}

	void EF_ProcessRenderLists(void (*RenderFunc)(), int nFlags, SViewport& VP);
	void FX_ProcessPostGroups(int nums, int nume);
	void FX_ProcessLightGroups(int nums, int nume);

	void EF_PrintProfileInfo();

	virtual void EF_EndEf3D (int nFlags, int nPrecacheUpdateId);
	virtual void EF_EndEf2D(bool bSort);  // 2d only

	virtual WIN_HWND GetHWND() { return  m_hWnd; }

	virtual void SetColorOp(byte eCo, byte eAo, byte eCa, byte eAa);

	virtual void SetTerrainAONodes(PodArray<SSectorTextureSet> * terrainAONodes)
	{
		assert(m_RP.m_nFillThreadID == m_pRT->GetThreadList());
		assert(m_RP.m_nFillThreadID < 2);
		m_RP.m_TerrainAONodes[m_RP.m_nFillThreadID].Clear();
		m_RP.m_TerrainAONodes[m_RP.m_nFillThreadID].AddList(*terrainAONodes);
	}

	void MakeUnitCube();
	bool FX_PrepareTerrainAOTarget(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff);

	void CalcAABBScreenRect(const AABB & aabb, const CRenderCamera& RCam, const Matrix44& mViewProj, Vec3* pvMin,  Vec3* pvMax);

protected:
#ifndef EXCLUDE_SCALEFORM_SDK
	struct SSF_ResourcesD3D& SF_GetResources()
	{
		assert(m_pSFResD3D);
		return *m_pSFResD3D;
	}
	struct SSF_ResourcesD3D* m_pSFResD3D;
#endif

	//////////////////////////////////////////////////////////////////////////
public:
	bool IsDeviceLost() { return( m_bDeviceLost ); }
	void InitNVAPI();
	void InitATIAPI();

	virtual IRenderAuxGeom* GetIRenderAuxGeom()
	{
#if defined(ENABLE_RENDER_AUX_GEOM)
		return m_pRenderAuxGeomD3D->GetRenderAuxGeom();
#else
		return &m_renderAuxGeomNull;
#endif
	}
	virtual void FlushMainThreadAuxGeomCB()
	{
#if defined(ENABLE_RENDER_AUX_GEOM)
		m_pRenderAuxGeomD3D->GetMainThreadAuxGeomCB()->Flush();
#endif
	}
	virtual void ToggleMainThreadAuxGeomCB()
	{
#if defined(ENABLE_RENDER_AUX_GEOM)
		m_pRenderAuxGeomD3D->GetMainThreadAuxGeomCB()->Toggle(!m_pRT->IsMultithreaded());
#endif
	}

	virtual IColorGradingController* GetIColorGradingController()
	{
		return m_pColorGradingControllerD3D;
	}

	// Stereo
	void CopyToStereo( int channel );
	void DisplayStereo();

	// Profiler

	virtual void	RT_BeginGPUTimer(const char* name);
	virtual void	RT_EndGPUTimer(const char* name);

private:
	bool CaptureFrameBufferToFile(const char* pFilePath, bool keepIntCaptureBuffer = false, float scale = -1.f);
	bool CaptureMiscBuffersToFiles(const char* pFilePath);
	void CaptureFrameBuffer();
	ICVar* CV_capture_frames;
	ICVar* CV_capture_folder;
	ICVar* CV_capture_file_format;
	ICVar* CV_capture_frame_once;
	ICVar* CV_capture_use_scale;
	ICVar* CV_capture_image_scale;
	ICVar* CV_capture_file_name;
#if defined (DIRECT3D9)
	IDirect3DSurface9* m_pCaptureFrameSurf[12];
#endif

	virtual void RT_InsertGpuCallback(uint32 context, GpuCallbackFunc callback);

#if defined(ENABLE_RENDER_AUX_GEOM)
	CRenderAuxGeomD3D* m_pRenderAuxGeomD3D;
#else
	CAuxGeomCB_Null m_renderAuxGeomNull;
#endif

	CShadowTextureGroupManager			m_ShadowTextureGroupManager;			// to combine multiple shadowmaps into one texture

	PodArray<SBuildTerrainTextureParams> m_buildTerrainTextureTasks;
};

extern CD3D9Renderer *gcpRendD3D;

//=========================================================================================

#include "D3DHWShader.h"
#include "DynamicVB.h"
#include "DynamicIB.h"
#include "StaticVB.h"
#include "StaticIB.h"

//=========================================================================================
inline void *CD3D9Renderer::FX_LockVB(uint32 nSize, uint32& nStart)
{
	int nCurVB = m_RP.m_CurVB;
	if (m_RP.m_VBs[nCurVB].VBPtr_0->GetBytesOffset()+nSize >= m_RP.m_VBs[nCurVB].VBPtr_0->GetBytesCount())
	{
		m_RP.m_VBs[nCurVB].VBPtr_0->Reset();
		m_RP.m_CurVB = (nCurVB + 1) & (MAX_DYNVBS-1);
	}
	nCurVB = m_RP.m_CurVB;
	void *pVB = (void *)m_RP.m_VBs[nCurVB].VBPtr_0->Lock(nSize, nStart);
	return pVB;
}
inline void CD3D9Renderer::FX_UnlockVB()
{
	m_RP.m_VBs[m_RP.m_CurVB].VBPtr_0->Unlock();
}

//=========================================================================================

void HDR_DrawDebug();

#endif // DRIVERD3D9_H
