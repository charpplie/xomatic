#ifndef __CRYDXPSGCM__
#define __CRYDXPSGCM__

//use libgcm command buffer debug mode in debug
#if !defined(__SPU__)
# include <sys/process.h>
# include <sys/spu_initialize.h>
# include <sys/paths.h>
#endif

#if !defined(__SPU__)
# include <cell/dbgfont.h>
#endif

#include <stdarg.h>
#if !defined(__SPU__)
#  include <cell/sysmodule.h>
#endif 
#include <SPU/gcm_mapping.h>
#include "../CCryDXPSMisc.hpp"

#include "GCM/CCryDXPSGCM_MemMan.hpp"
#include "GCM/CCryDXPSGCM_PixelShaderCacheMan.hpp"
#include "GCM/CCryDXPSGCM_TiledRegionMan.hpp"
#include "GCM/CCryDXPSGCM_ZCullMan.hpp"
#include "GCM/CCryDXPSGCM_Formats.hpp"
//#include "GCM/CCryDXPSGCM_Scratchpad.hpp"
#include "GCM/CCryDXPSGCM_SyncMan.hpp"
#include "GCM/CCryDXPSGCM_CMBMan.hpp"
#include "GCM/CCryDXPSGCM_PrecompileCMBMan.hpp"
#include "GCM/CCryDXPSGCM_AllocPool.hpp"
#include "../Device/Resource/CCryDXPSResource.hpp"

typedef NCryPoolAlloc::CPoolBestFit<NCryPoolAlloc::CItemInPlaceFL,NCryPoolAlloc::CPool<0> > TTempConstBufferPool;
//typedef NCryPoolAlloc::CPoolBestFit<NCryPoolAlloc::CItemInPlaceFL,NCryPoolAlloc::CPool> > TTempConstBufferPool;

class CCryDXPSRenderDevice;
class CCryDXPSSwapChain;
class CCryDXPSBlob;
class CCryDXPSBuffer;
class CCryDXPSSamplerState;
class CCryDXPSShader;
class CCryDXPSShaderReflection;
class CCryDXPSTexture3D;
class CCryDXPSTexture1D;
class CCryDXPSTexture2D;
class CCryDXPSTexture3D;
class CCryDXPSBlendState;
class CCryDXPSRasterizerState;
class CCryDXPSDepthStencilState;
class CCryDXPSInputLayout;
class CCryDXPSRenderTargetView;
class CCryDXPSDepthStencilView;
class CCryDXPSShaderResourceView;
typedef class CCryDXPSGIAdapter IDXGIAdapter;

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM

struct CCryLayer0GCMMemStats
{
	CCryLayer0GCMMemResource		m_SwapChain;
	CCryLayer0GCMMemResource		m_Blob;
	CCryLayer0GCMMemResource		m_Buffer;
	CCryLayer0GCMMemResource		m_BufferPSCache;
	CCryLayer0GCMMemResource		m_BufferVB;
	CCryLayer0GCMMemResource		m_BufferIB;
	CCryLayer0GCMMemResource		m_BufferCB;
	CCryLayer0GCMMemResource		m_SamplerState;
	CCryLayer0GCMMemResource		m_BlendState;
	CCryLayer0GCMMemResource		m_RasterizerState;
	CCryLayer0GCMMemResource		m_DepthStencilState;
	CCryLayer0GCMMemResource		m_Texture1D;
	CCryLayer0GCMMemResource		m_Texture2D;
	CCryLayer0GCMMemResource		m_Texture2DEngine;
	CCryLayer0GCMMemResource		m_Texture3D;
	CCryLayer0GCMMemResource		m_RenderTargetView;
	CCryLayer0GCMMemResource		m_DepthStencilView;
	CCryLayer0GCMMemResource		m_ShaderResourceView;
	CCryLayer0GCMMemResource		m_Shader;
	CCryLayer0GCMMemResource		m_ShaderReflection;
	CCryLayer0GCMMemResource		m_InputLayout;
	CCryLayer0GCMMemResource		m_Dummy;
} _ALIGN(128);
#endif

namespace CCryLayer0GCM
{
	extern CCryDXPSGCMMemMan								m_Memory;
	extern CCryDXPSGCMPixelshaderCacheMan		m_PSCache;
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	extern CCryDXPSGCMPrecompileCMBMan			m_PCCMB;
#endif
	extern CCryDXPSGCMSyncMan								m_Sync;
	extern CCryDXPSGCMTiledRegionMan				m_TiledRegion;
	extern CCryDXPSGCMZCullMan							m_ZCull;
	extern CCryDXPSGCMCMBMan								m_CMBMan;
	extern CCryDXPSRenderDevice*						m_pDevice;
	extern CCryDXPSSwapChain*								m_pSwapChain;
#if !defined(CRY_DXPS_LSCONSTANTS)
	extern TTempConstBufferPool*						m_pTempConstBufferPool;
#endif

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	extern CCryLayer0GCMMemStats						m_MemStats;
#endif

	void																		InitInstance();

	ILINE void FreeCCryDXPSBufferTemp(const CCryDXPSBuffer* pTemp)
	{
#if !defined(CRY_DXPS_LSCONSTANTS)
		m_pTempConstBufferPool->Free(const_cast<CCryDXPSBuffer*>(pTemp));
#endif
	}

	// Mem management
	CCryDXPSRenderDevice*	CreateCCryDXPSRenderDevice(
																								IDXGIAdapter*				pAdapter,
																								bool								InitializeSPU);

	CCryDXPSSwapChain*	CreateCCryDXPSSwapChain(CCryDXPSRenderDevice*, DXGI_SWAP_CHAIN_DESC*);

	CCryDXPSBlob*	CreateCCryDXPSBlob(std::size_t);
	CCryDXPSBlob*	CreateCCryDXPSBlob(const char* pProgram,uint32 Size);

	CCryDXPSBuffer*	CreateCCryDXPSBuffer(const void* pVertices,uint32 Size,ECRYDXPSResType Type,ECRYDXPSResFreq	Freq);
	CCryDXPSBuffer*	CreateCCryDXPSBufferTemp(uint32 Size,uint32 Slot,bool VertexShaderCB);
	void	FreeCCryDXPSBufferTemp(const CCryDXPSBuffer* pTemp);

	CCryDXPSSamplerState*	CreateCCryDXPSSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc);
	CCryDXPSBlendState*	CreateCCryDXPSBlendState(const D3D11_BLEND_DESC *pBlendDesc);
	CCryDXPSRasterizerState*	CreateCCryDXPSRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc);
	CCryDXPSDepthStencilState*	CreateCCryDXPSDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc);

	CCryDXPSTexture1D*	CreateCCryDXPSTexture1D(uint32 SizeX,uint32 Mips,DXGI_FORMAT Format);
	CCryDXPSTexture2D*	CreateCCryDXPSTexture2D(uint32 SizeX,uint32 SizeY,uint32 Mips,DXGI_FORMAT Format,ECryDXPSTex2DCreateFlags CreateFlags,bool CubeMap=false);
	CCryDXPSTexture3D*	CreateCCryDXPSTexture3D(uint32 SizeX,uint32 SizeY,uint32 SizeZ,uint32 Mips,DXGI_FORMAT Format);

	CCryDXPSRenderTargetView*	CreateCCryDXPSRenderTargetView(CCryDXPSTexture2D* pTex,DXPSRTVDTilingInfo TiledInfo);
	CCryDXPSDepthStencilView*	CreateCCryDXPSDepthStencilView(CCryDXPSTexture2D* pTex);
	CCryDXPSShaderResourceView*	CreateCCryDXPSShaderResourceView(CCryDXPSResource* pTex,const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc);

	void	ScreenSize(int& rSizeX,int& rSizeY);

	CCryDXPSShader* CreateCCryDXPSShader(const void* pProgram,uint32 SType);
	CCryDXPSShaderReflection* CreateCCryDXPSShaderReflection(const char* pProgram);

	CCryDXPSInputLayout* CreateCCryDXPSInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,uint32 NumElements,const void *pShaderBytecodeWithInputSignature);

	template<class T>
	T*	New()
	{
		return new T;
	}
	template<class T>
	T*	New(uint32 S)
	{
		return new T[S];
	}

	template<class T>
	void	Release(T* pObject)
	{
		delete pObject;
	}
	template<class T>
	void	Delete(T*	pObject)
	{
		delete pObject;
	}
	template<class T>
	void	DeleteArray(T* pObject)
	{
		delete[] pObject;
	}


	inline CCryDXPSGCMMemMan&								Memory(){return m_Memory;}
	inline CCryDXPSGCMTiledRegionMan&				TiledRegion(){return m_TiledRegion;}
	inline CCryDXPSGCMZCullMan&							ZCull(){return m_ZCull;}
	inline CCryDXPSGCMSyncMan&							Sync(){return m_Sync;}
	inline CCryDXPSGCMCMBMan&								CMB(){return m_CMBMan;}
	inline CCryDXPSRenderDevice*						Device(){return m_pDevice;}
	inline CCryDXPSSwapChain*								SwapChain(){return m_pSwapChain;}
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	inline CCryDXPSGCMPrecompileCMBMan&			PCCMB(){return m_PCCMB;}
#endif
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	inline CCryLayer0GCMMemStats* MemStats(){return &m_MemStats;}
#endif

	void	Size(ICrySizer* Sizer);
};
namespace tdLayer0 = CCryLayer0GCM;



#endif

