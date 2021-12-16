#ifdef CRY_USE_GCM
#include "StdAfx.h"
#include <sysutil/sysutil_sysparam.h>
#include <cell/font.h>
#include <cell/fontFT.h>
#include "CCryDXPS.hpp"
#include "CCryDXPSGCM.hpp"
#include "../Device/CCryDXPSBlob.hpp"
#include "../Device/CCryDXPSRenderDevice.hpp"
#include "../Device/CCryDXPSSwapChain.hpp"
#include "../Device/Resource/Buffer/CCryDXPSSamplerState.hpp"
#include "../Device/Resource/States/CCryDXPSBlendState.hpp"
#include "../Device/Resource/States/CCryDXPSRasterizerState.hpp"
#include "../Device/Resource/States/CCryDXPSDepthStencilState.hpp"


struct SFontLocal
{
	uint32_t									m_FontFileCache[1*1024*1024/4]; // Prepare cache buffer
	CellFontEntry							m_UserFontEntrys[3];     // Prepare  entries for user fonts
	CellFontConfig						m_FontConfig;
	CellFontLibraryConfigFT		m_FontLibConfig;
	const CellFontLibrary*		m_Library;

	CellFontType							m_FontType;
	CellFont									m_Font;

	CellFontRenderer					m_Renderer;
	CellFontRendererConfig		m_RendererConfig;



	static const uint32_t			SCREENSIZEX	=	1280;
	static const uint32_t			SCREENSIZEY	=	704;
	static const uint32_t			FONTSIZEX	=	8;
	static const uint32_t			FONTSIZEY	=	16;
	static const uint32_t			SIZEX	=	16*FONTSIZEX;
	static const uint32_t			SIZEY	=	16*FONTSIZEY;
	static const uint32_t			CHARCOUNTX	=	SCREENSIZEX/FONTSIZEX;
	static const uint32_t			CHARCOUNTY	=	SCREENSIZEY/FONTSIZEY;

	uint8_t										m_Buffer[SIZEX*SIZEY];
	int8_t										m_OffsetX[256];
	int8_t										m_OffsetY[256];
	uint32_t*									m_pFrameBuffer;
	char											m_Lines[CHARCOUNTX*CHARCOUNTY];
	uint32_t									m_LinePt;
	uint32_t									m_Line;
	


};

static SFontLocal* g_DXPSDebugPrinterInitialized	=	0;




int load_libfont_module()
{
	int ret;
	ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FONT );
	if ( ret == CELL_OK ) {
		ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FREETYPE );
		if ( ret == CELL_OK ) {
			ret = cellSysmoduleLoadModule( CELL_SYSMODULE_FONTFT );
			if ( ret == CELL_OK ) {
			return ret; // Success
			}
			// Error handling as follows (Unload all loads)
			cellSysmoduleUnloadModule( CELL_SYSMODULE_FREETYPE );
		}
		cellSysmoduleUnloadModule( CELL_SYSMODULE_FONT );
	}
	return ret; // Error end
}



void DXPSDebugPrinterRelease()
{
	if( g_DXPSDebugPrinterInitialized == NULL )
		return;

	// deallocate and end debug output library
	SFontLocal& rLocal	=	*g_DXPSDebugPrinterInitialized;
	cellFontUnbindRenderer( &rLocal.m_Font );
	cellFontDestroyRenderer( &rLocal.m_Renderer );
	cellFontEndLibrary( rLocal.m_Library  );
	cellFontCloseFont( &rLocal.m_Font );
	cellFontEnd();
	cellSysmoduleUnloadModule( CELL_SYSMODULE_FONTFT );
	cellSysmoduleUnloadModule( CELL_SYSMODULE_FREETYPE );
	cellSysmoduleUnloadModule( CELL_SYSMODULE_FONT );

	delete g_DXPSDebugPrinterInitialized;
	g_DXPSDebugPrinterInitialized	=	0;
	gPS3Env->bDebugOutputActive = 0;
		
}

static void* fonts_malloc( void*obj, uint32_t size )
{
	obj=NULL;
	return malloc( size );
}
static void  fonts_free( void*obj, void*p )
{
	obj=NULL;
	free( p );
}
static void* fonts_realloc( void*obj, void* p, uint32_t size )
{
	obj=NULL;
	return realloc( p, size );
}
static void* fonts_calloc( void*obj, uint32_t numb, uint32_t blockSize )
{
	obj=NULL;
	return calloc( numb, blockSize );
}


void DXPSDebugPrinterInit()
{
#ifndef CRY_DXPS_SINGLEFLUSHVALIDATE
	using namespace CRY_DXPS_GCMNAMESPACE;

	delete g_DXPSDebugPrinterInitialized;
	g_DXPSDebugPrinterInitialized	=	new SFontLocal;

	SFontLocal& rLocal	=	*g_DXPSDebugPrinterInitialized;
	memset(g_DXPSDebugPrinterInitialized,0,sizeof(SFontLocal));

	uint32 ret = load_libfont_module();
	if ( ret != CELL_OK ) 
	{
		// Error handling
		printf("Could not load libfont for debug rendering\n");
		DXPSDebugPrinterRelease();
		return;
	}

	rLocal.m_FontConfig.FileCache.buffer = &rLocal.m_FontFileCache[0]; // Set the file read cache 
	rLocal.m_FontConfig.FileCache.size = 1*1024*1024;
	rLocal.m_FontConfig.userFontEntrys = &rLocal.m_UserFontEntrys[0];   // Set entry for the number of user fonts
	rLocal.m_FontConfig.userFontEntryMax = 3;
	ret = cellFontInit( &rLocal.m_FontConfig ); // Initialize libfont
	if ( ret != CELL_OK ) 
	{
		// Error handling
		printf("Could not init libfont for debug rendering\n");
		DXPSDebugPrinterRelease();
		return;
	}

	CellFontLibraryConfigFT_initialize( &rLocal.m_FontLibConfig ); // Initial setting
	rLocal.m_FontLibConfig.MemoryIF.Object  = NULL;
	rLocal.m_FontLibConfig.MemoryIF.Malloc  = fonts_malloc;
	rLocal.m_FontLibConfig.MemoryIF.Free    = fonts_free;
	rLocal.m_FontLibConfig.MemoryIF.Realloc = fonts_realloc;
	rLocal.m_FontLibConfig.MemoryIF.Calloc  = fonts_calloc;
	ret = cellFontInitLibraryFreeType( &rLocal.m_FontLibConfig, &rLocal.m_Library );
	if ( ret != CELL_FONT_OK )
	{
		// Error handling
		printf("Could not init font for debug rendering\n");
		DXPSDebugPrinterRelease();
		return;
	}

	rLocal.m_FontType.type = CELL_FONT_TYPE_ROUND_SANS_EUROPEAN_CASUAL_CJK_LATIN_SET;
	rLocal.m_FontType.map  = CELL_FONT_MAP_UNICODE;
	ret = cellFontOpenFontset( rLocal.m_Library, &rLocal.m_FontType, &rLocal.m_Font );
	if ( ret != CELL_FONT_OK )
	{
		// Error handling
		printf("Could not open font for debug rendering\n");
		DXPSDebugPrinterRelease();
		return;
	}

	CellFontRendererConfig_initialize( &rLocal.m_RendererConfig );
	CellFontRendererConfig_setAllocateBuffer( &rLocal.m_RendererConfig, 256*256, 0 );
	// Create renderer
	ret = cellFontCreateRenderer( rLocal.m_Library, &rLocal.m_RendererConfig, &rLocal.m_Renderer);
	if ( ret != CELL_FONT_OK )
	{
		// Error handling
		printf("Could not create font-renderer for debug rendering\n");
		DXPSDebugPrinterRelease();
		return;
	}


	ret = cellFontBindRenderer(&rLocal.m_Font,&rLocal.m_Renderer );
	if ( ret != CELL_FONT_OK )
	{
		// Error handling
		printf("Could not bind the font to the font-renderer for debug rendering\n");
		DXPSDebugPrinterRelease();
		return;
	}

	cellFontSetupRenderScalePixel(&rLocal.m_Font,SFontLocal::FONTSIZEX,SFontLocal::FONTSIZEY-SFontLocal::FONTSIZEY/4);
	cellFontSetScalePixel(&rLocal.m_Font,SFontLocal::FONTSIZEX,SFontLocal::FONTSIZEY-SFontLocal::FONTSIZEY/4);  // Set font scale

	CellFontRenderSurface Surface;
	cellFontRenderSurfaceInit(&Surface,rLocal.m_Buffer,rLocal.SIZEX,1,rLocal.SIZEX,rLocal.SIZEY);
	cellFontRenderSurfaceSetScissor(&Surface,0,0,rLocal.SIZEX,rLocal.SIZEY);


	CellFontHorizontalLayout Layout;
	CellFontGlyphMetrics Metrics;
	
	cellFontGetHorizontalLayout(&rLocal.m_Font, &Layout );
	for(uint32_t Code=0;Code<256;Code++) 
	{
		CellFontGlyphMetrics Metrics;
		CellFontImageTransInfo TransInfo;
		cellFontGetCharGlyphMetrics(&rLocal.m_Font,Code, &Metrics );

		const float x = 0;//(Code&0xf)*rLocal.SIZEX;//Metrics.Horizontal.advance-Metrics.bearingY; // Align first character of eachline at the left side
		const float y = 0;//(Code>>4)*rLocal.SIZEY;//Layout.lineHeight; // Line feed
		rLocal.m_OffsetX[Code]	=	0;//Metrics.Horizontal.bearingX;
		rLocal.m_OffsetY[Code]	=	static_cast<int8_t>(Metrics.height-Metrics.Horizontal.bearingY);

		ret	=	cellFontRenderCharGlyphImage(&rLocal.m_Font, Code, &Surface, x, y, &Metrics, &TransInfo );
		if(CELL_OK !=ret)
				continue;

		{
			const uint32_t	X	=	(Code&0xf)*SFontLocal::FONTSIZEX;
			const uint32_t	Y	=	(Code>>4)*SFontLocal::FONTSIZEY;
			uint8_t* img = TransInfo.Image;
			for (uint32_t v=0; v < TransInfo.imageHeight; v++ ) 
			{
				for ( uint32_t u=0; u < TransInfo.imageWidth; u++ ) 
					rLocal.m_Buffer[u+X+(v+Y+SFontLocal::FONTSIZEY-TransInfo.imageHeight)*rLocal.SIZEX]	=	img[u]&0x000000ff;
//					printf((img[u]&0x000000ff)>=0x7f?"X":".");
//				printf("\n");
				img += TransInfo.imageWidthByte;
			}
		}
//		printf("\n\n");
	}


	CCryDXPSRenderDevice* pDevice	=	tdLayer0::CreateCCryDXPSRenderDevice(0,false);
	DXGI_SWAP_CHAIN_DESC Desc;
	Desc.BufferDesc.Width		=	SFontLocal::SCREENSIZEX;//hardcoded workaround
	Desc.BufferDesc.Height	=	SFontLocal::SCREENSIZEY;//hardcoded workaround
	Desc.BufferDesc.Format	=	DXGI_FORMAT_R8G8B8A8_UNORM;
	Desc.BufferCount				=	2;

	CCryDXPSSwapChain* pSwapChain	=	tdLayer0::CreateCCryDXPSSwapChain(pDevice,&Desc);

	tdLayer0::Sync().SwapDeviceThread();
	tdLayer0::Sync().SwapRenderer();
	pSwapChain->Present(0,0);//to initialize buffers
	tdLayer0::CMB().FlushCMDs();

	rLocal.m_pFrameBuffer	=	reinterpret_cast<uint32_t*>(pSwapChain->FrameBufferPtr());
	memset(rLocal.m_pFrameBuffer,0,4*1024*1024);
	gPS3Env->bDebugOutputActive = 1;
#endif
}

void DXPSRender()
{
	SFontLocal& rLocal	=	*g_DXPSDebugPrinterInitialized;

	for(uint32_t yy=0;yy<SFontLocal::CHARCOUNTY;yy++)
	for(uint32_t xx=0;xx<SFontLocal::CHARCOUNTX;xx++)
	{
		const	uint32_t	C				=	rLocal.m_Lines[xx+yy*SFontLocal::CHARCOUNTX];
		const int32_t		OX			=	rLocal.m_OffsetX[C];
		const int32_t		OY			=	rLocal.m_OffsetY[C];
		const uint32_t	X				=	(C&0xf)*SFontLocal::FONTSIZEX;
		const uint32_t	Y				=	(C>>4)*SFontLocal::FONTSIZEY;
		const uint8_t*	pGlyph	=	&(rLocal.m_Buffer[X+Y*SFontLocal::SIZEX]);
		uint32_t*	pFrameBuffer=	rLocal.m_pFrameBuffer;
		for(int32_t y=0;y<(int32_t)SFontLocal::FONTSIZEY;y++)
		for(int32_t x=0;x<(int32_t)SFontLocal::FONTSIZEX;x++)
		{
			const uint32 G	=	pGlyph[x+y*SFontLocal::SIZEX];
			pFrameBuffer[xx*SFontLocal::FONTSIZEX+x+OX+(yy*SFontLocal::FONTSIZEY+y+OY)*SFontLocal::SCREENSIZEX]	=	G|(G<<8)|(G<<16)|(G<<24);
		}
	}
	
}

void DXPSDebugPrintPushLine()
{
	SFontLocal& rLocal	=	*g_DXPSDebugPrinterInitialized;
	while(rLocal.m_LinePt<SFontLocal::CHARCOUNTX)
	{
		rLocal.m_Lines[rLocal.m_LinePt+rLocal.m_Line*SFontLocal::CHARCOUNTX]	=	0;
		rLocal.m_LinePt++;
	}
	rLocal.m_Line++;
	rLocal.m_LinePt=0;

	if(rLocal.m_Line==SFontLocal::CHARCOUNTY)
	{
		rLocal.m_Line--;
		for(uint32_t y=1;y<SFontLocal::CHARCOUNTY;y++)
		for(uint32_t x=0;x<SFontLocal::CHARCOUNTX;x++)
		{
			g_DXPSDebugPrinterInitialized->m_Lines[x+(y-1)*SFontLocal::CHARCOUNTX]	=	
							g_DXPSDebugPrinterInitialized->m_Lines[x+y*SFontLocal::CHARCOUNTX];
		}
		for(uint32_t x=0;x<SFontLocal::CHARCOUNTX;x++)
			rLocal.m_Lines[x+rLocal.m_Line*SFontLocal::CHARCOUNTX]	=	0;

	}

}

void DXPSDebugPrint(const char C)
{
	if(C=='\n')
	{
		DXPSDebugPrintPushLine();
		return;
	}

	SFontLocal& rLocal	=	*g_DXPSDebugPrinterInitialized;
	if(rLocal.m_LinePt<SFontLocal::CHARCOUNTX)
	{
			rLocal.m_Lines[rLocal.m_LinePt+rLocal.m_Line*SFontLocal::CHARCOUNTX]	=	C;
			rLocal.m_LinePt++;
	}
}

void DXPSDebugPrint(const char* pText)
{
	if(!g_DXPSDebugPrinterInitialized)
		return;

	if(!pText || !*pText)
		return;

	for(uint32_t a=0;pText[a];a++)
		DXPSDebugPrint(pText[a]);

	DXPSRender();
}

namespace CCryLayer0GCM
{
	CCryDXPSGCMMemMan								m_Memory;
	CCryDXPSGCMPixelshaderCacheMan	m_PSCache;
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	CCryDXPSGCMPrecompileCMBMan			m_PCCMB;
#endif
	CCryDXPSGCMSyncMan							m_Sync;
	CCryDXPSGCMTiledRegionMan				m_TiledRegion;
	CCryDXPSGCMZCullMan							m_ZCull;
	CCryDXPSGCMCMBMan								m_CMBMan;
	CCryDXPSRenderDevice*						m_pDevice;
	CCryDXPSSwapChain*							m_pSwapChain;
#if !defined(CRY_DXPS_LSCONSTANTS)
	TTempConstBufferPool*						m_pTempConstBufferPool;
#endif
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	CCryLayer0GCMMemStats						m_MemStats;
#endif
	int															m_Initialized = 0;
}

void CCryLayer0GCM::InitInstance()
{
	if(m_Initialized == 0)
	{
		m_pDevice = 0;
		m_pSwapChain = 0;
#if !defined(CRY_DXPS_LSCONSTANTS)
		m_pTempConstBufferPool	=	new	TTempConstBufferPool(1024*1024*3,16);
#endif
		m_Initialized = 1;	
	}
}

CCryDXPSRenderDevice*	CCryLayer0GCM::CreateCCryDXPSRenderDevice(
																								IDXGIAdapter*				pAdapter,
																								bool								InitializeSPU)
{
	// Make sure CCryLayer0GCM::Instance() is called at least once on PPU to
	// make the singleton available on SPUs.
	InitInstance();

	if(!m_pDevice)
		m_pDevice	=	new CCryDXPSRenderDevice(pAdapter);
	if(InitializeSPU)
		m_pDevice->InitSPU();

	return m_pDevice;
}

CCryDXPSSwapChain* CCryLayer0GCM::CreateCCryDXPSSwapChain(CCryDXPSRenderDevice* pDevice,DXGI_SWAP_CHAIN_DESC* pSwapChainDesc)
{
	if(!m_pDevice)
	{
		snPause();
	}
	if(!m_pSwapChain)
		m_pSwapChain	=	new CCryDXPSSwapChain(m_pDevice,pSwapChainDesc
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_SwapChain
#endif
		);
	return m_pSwapChain;
}

CCryDXPSInputLayout* CCryLayer0GCM::CreateCCryDXPSInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,uint32 NumElements,const void *pShaderBytecodeWithInputSignature)
{
	return new CCryDXPSInputLayout(pInputElementDescs,NumElements,pShaderBytecodeWithInputSignature
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_InputLayout
#endif
		);
}

CCryDXPSBlob* CCryLayer0GCM::CreateCCryDXPSBlob(std::size_t Size)
{
	return new CCryDXPSBlob(Size
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_Blob
#endif
		);
}

CCryDXPSBlob* CCryLayer0GCM::CreateCCryDXPSBlob(const char* pProgram,uint32 Size)
{
	return new CCryDXPSBlob(pProgram,Size
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_Blob
#endif
		);
}


CCryDXPSTexture1D* CCryLayer0GCM::CreateCCryDXPSTexture1D(uint32 SizeX,uint32 Mips,DXGI_FORMAT Format)
{
	return new CCryDXPSTexture1D(SizeX,Mips,Format
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_Texture1D
#endif
		);
}

CCryDXPSTexture2D* CCryLayer0GCM::CreateCCryDXPSTexture2D(uint32 SizeX,uint32 SizeY,uint32 Mips,DXGI_FORMAT Format,ECryDXPSTex2DCreateFlags CreateFlags,bool CubeMap)
{
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	if(Mips == 1/*approx*/)
		return new CCryDXPSTexture2D(SizeX,SizeY,Mips,Format,CreateFlags,CubeMap,m_MemStats.m_Texture2DEngine);
	else
		return new CCryDXPSTexture2D(SizeX,SizeY,Mips,Format,CreateFlags,CubeMap,m_MemStats.m_Texture2D);
#else
	return new CCryDXPSTexture2D(SizeX,SizeY,Mips,Format,CreateFlags,CubeMap);
#endif
}

CCryDXPSTexture3D* CCryLayer0GCM::CreateCCryDXPSTexture3D(uint32 SizeX,uint32 SizeY,uint32 SizeZ,uint32 Mips,DXGI_FORMAT Format)
{
	return new CCryDXPSTexture3D(SizeX,SizeY,SizeZ,Mips,Format
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_Texture3D
#endif
		);
}

CCryDXPSRenderTargetView*	CCryLayer0GCM::CreateCCryDXPSRenderTargetView(CCryDXPSTexture2D* pTex,DXPSRTVDTilingInfo TiledInfo)
{
	return new CCryDXPSRenderTargetView(pTex,TiledInfo
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_RenderTargetView
#endif
		);
}

CCryDXPSDepthStencilView*	CCryLayer0GCM::CreateCCryDXPSDepthStencilView(CCryDXPSTexture2D* pTex)
{
	return new CCryDXPSDepthStencilView(pTex
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_DepthStencilView
#endif
		);
}

CCryDXPSShaderResourceView*	CCryLayer0GCM::CreateCCryDXPSShaderResourceView(	CCryDXPSResource* pTex,
																																							const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
	return new CCryDXPSShaderResourceView(reinterpret_cast<CCryDXPSTexture2D*>(pTex),pDesc
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_ShaderResourceView
#endif
		);
}

CCryDXPSBuffer*	CCryLayer0GCM::CreateCCryDXPSBuffer(const void* pVertices,uint32 Size,ECRYDXPSResType Type,ECRYDXPSResFreq	Freq)
{
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	switch(Type)
	{
		case EDXPS_RT_PIXELSHADERCACHE:
			return new CCryDXPSBuffer(pVertices,Size,Type,Freq,m_MemStats.m_BufferPSCache);
		case EDXPS_RT_VERTEXBUFFER:
			return new CCryDXPSBuffer(pVertices,Size,Type,Freq,m_MemStats.m_BufferVB);
		case EDXPS_RT_INDEXBUFFER:
			return new CCryDXPSBuffer(pVertices,Size,Type,Freq,m_MemStats.m_BufferIB);
		case EDXPS_RT_CONSTBUFFER:
			return new CCryDXPSBuffer(pVertices,Size,Type,Freq,m_MemStats.m_BufferCB);
		default:
			return new CCryDXPSBuffer(pVertices,Size,Type,Freq,m_MemStats.m_Buffer);
	};
#else
	return new CCryDXPSBuffer(pVertices,Size,Type,Freq);
#endif
}


CCryDXPSBuffer*	CCryLayer0GCM::CreateCCryDXPSBufferTemp(uint32 Size,uint32 Slot,bool VertexShaderCB)
{
#if defined(CRY_DXPS_LSCONSTANTS)
	uint8* pMem = tdLayer0::Device()->AllocAndSetConstantBuffer(Size,Slot,VertexShaderCB);
#else
	uint8* pMem	=	m_pTempConstBufferPool->Alloc(CCryDXPSBuffer::EstimateSize(Size),16);
#endif
	CCryDXPSBuffer* pCBuffer	=	new (pMem)	CCryDXPSBuffer(Size
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_Dummy
#endif
		);
	return pCBuffer;
}

CCryDXPSShader* CCryLayer0GCM::CreateCCryDXPSShader(const void* pProgram,uint32 SType)
{
	return new CCryDXPSShader(pProgram,(ECRYDXPSShaderType)SType
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_Shader
#endif
		);
}

CCryDXPSShaderReflection* CCryLayer0GCM::CreateCCryDXPSShaderReflection(const char* pProgram)
{
	return new CCryDXPSShaderReflection(pProgram
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_ShaderReflection
#endif
		);
}

CCryDXPSSamplerState*	CCryLayer0GCM::CreateCCryDXPSSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc)
{
	return new CCryDXPSSamplerState(*pSamplerDesc
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_SamplerState
#endif
		);
}

CCryDXPSBlendState*	CCryLayer0GCM::CreateCCryDXPSBlendState(const D3D11_BLEND_DESC *pBlendDesc)
{
	return new CCryDXPSBlendState(*pBlendDesc
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_BlendState
#endif
		);
}

CCryDXPSRasterizerState*	CCryLayer0GCM::CreateCCryDXPSRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc)
{
	return new CCryDXPSRasterizerState(*pRasterizerDesc
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_RasterizerState
#endif
		);
}

CCryDXPSDepthStencilState*	CCryLayer0GCM::CreateCCryDXPSDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc)
{
	return new CCryDXPSDepthStencilState(*pDepthStencilDesc
#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
		,m_MemStats.m_DepthStencilState
#endif
		);
}


void CCryLayer0GCM::ScreenSize(int& rSizeX,int& rSizeY)
{
	CellVideoOutState videoState;
	if(0==cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState))
	{
		CellVideoOutResolution resolution;
		cellVideoOutGetResolution(videoState.displayMode.resolutionId, &resolution);
		rSizeX	= resolution.width;
		rSizeY	= resolution.height;
		CRY_DEBUGOUT("--resolution %d %d\n",resolution.width,resolution.height);
	}
}

void CCryLayer0GCM::Size(ICrySizer* Sizer)
{
//	CryGetMemSize()
	SIZER_COMPONENT_NAME(Sizer,"DXPS static alloc");
	m_Memory.Size(Sizer);
#if defined(CRY_DXPS_PRECOMPILE_CMDBUFFER)
	m_PCCMB.Size(Sizer);
#endif
	m_Sync.Size(Sizer);
	m_TiledRegion.Size(Sizer);
	m_ZCull.Size(Sizer);
	m_CMBMan.Size(Sizer);
	if(m_pDevice)
		m_pDevice->Size(Sizer);

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container");
		Sizer->AddObject(&m_MemStats,sizeof(m_MemStats));
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container SwapChain");
			Sizer->AddObject(&m_MemStats.m_SwapChain,m_MemStats.m_SwapChain.MainMem(),m_MemStats.m_SwapChain.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Blob");
			Sizer->AddObject(&m_MemStats.m_Blob,m_MemStats.m_Blob.MainMem(),m_MemStats.m_Blob.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Common Buffer");
			Sizer->AddObject(&m_MemStats.m_Buffer,m_MemStats.m_Buffer.MainMem(),m_MemStats.m_Buffer.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Pixelshader Cache Buffer");
			Sizer->AddObject(&m_MemStats.m_BufferPSCache,m_MemStats.m_BufferPSCache.MainMem(),m_MemStats.m_BufferPSCache.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Vertex Buffer");
			Sizer->AddObject(&m_MemStats.m_BufferVB,m_MemStats.m_BufferVB.MainMem(),m_MemStats.m_BufferVB.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Index Buffer");
			Sizer->AddObject(&m_MemStats.m_BufferIB,m_MemStats.m_BufferIB.MainMem(),m_MemStats.m_BufferIB.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Const Buffer");
			Sizer->AddObject(&m_MemStats.m_BufferCB,m_MemStats.m_BufferCB.MainMem(),m_MemStats.m_BufferCB.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container SamplerState");
			Sizer->AddObject(&m_MemStats.m_SamplerState,m_MemStats.m_SamplerState.MainMem(),m_MemStats.m_SamplerState.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container BlendState");
			Sizer->AddObject(&m_MemStats.m_BlendState,m_MemStats.m_BlendState.MainMem(),m_MemStats.m_BlendState.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container RasterizerState");
			Sizer->AddObject(&m_MemStats.m_RasterizerState,m_MemStats.m_RasterizerState.MainMem(),m_MemStats.m_RasterizerState.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container DepthStencilState");
			Sizer->AddObject(&m_MemStats.m_DepthStencilState,m_MemStats.m_DepthStencilState.MainMem(),m_MemStats.m_DepthStencilState.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Texture1D");
			Sizer->AddObject(&m_MemStats.m_Texture1D,m_MemStats.m_Texture1D.MainMem(),m_MemStats.m_Texture1D.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Texture2D");
			Sizer->AddObject(&m_MemStats.m_Texture2D,m_MemStats.m_Texture2D.MainMem(),m_MemStats.m_Texture2D.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Engine Texture2D");
			Sizer->AddObject(&m_MemStats.m_Texture2DEngine,m_MemStats.m_Texture2DEngine.MainMem(),m_MemStats.m_Texture2DEngine.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Texture3D");
			Sizer->AddObject(&m_MemStats.m_Texture3D,m_MemStats.m_Texture3D.MainMem(),m_MemStats.m_Texture3D.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container RenderTargetView");
			Sizer->AddObject(&m_MemStats.m_RenderTargetView,m_MemStats.m_RenderTargetView.MainMem(),m_MemStats.m_RenderTargetView.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container DepthStencilView");
			Sizer->AddObject(&m_MemStats.m_DepthStencilView,m_MemStats.m_DepthStencilView.MainMem(),m_MemStats.m_DepthStencilView.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container ShaderResourceView");
			Sizer->AddObject(&m_MemStats.m_ShaderResourceView,m_MemStats.m_ShaderResourceView.MainMem(),m_MemStats.m_ShaderResourceView.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container Shader");
			Sizer->AddObject(&m_MemStats.m_Shader,m_MemStats.m_Shader.MainMem(),m_MemStats.m_Shader.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container ShaderReflection");
			Sizer->AddObject(&m_MemStats.m_ShaderReflection,m_MemStats.m_ShaderReflection.MainMem(),m_MemStats.m_ShaderReflection.Count());
		}
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS MemStats container InputLayout");
			Sizer->AddObject(&m_MemStats.m_InputLayout,m_MemStats.m_InputLayout.MainMem(),m_MemStats.m_InputLayout.Count());
		}
	}
#endif
}

void*	CryMallocRSXMem(){return tdLayer0::Memory().MemoryAni();};
uint32 CryMallocRSXMemSize(){return tdLayer0::Memory().SizeAni();};
/*
tdResHandle	CryRSXMemCpySync(void* pDst,const void* pSrc,uint32 Size){return tdLayer0::Device()->MemCpy(pDst,pSrc,Size,true);};
tdResHandle	CryRSXMemCpySyncScaled2D(			void* pDst,uint32 DstSizeX,uint32 DstSizeY,uint32 DstPitch,
																		const void* pSrc,uint32 SrcSizeX,uint32 SrcSizeY,uint32 SrcPitch)
																					{return tdLayer0::Device()->MemCpyScaled2D(pDst,DstSizeX,DstSizeY,DstPitch,pSrc,SrcSizeX,SrcSizeY,SrcPitch,true);};
tdResHandle	CryRSXMemCpyASync(void* pDst,const void* pSrc,uint32 Size){return tdLayer0::Device()->MemCpy(pDst,pSrc,Size,false);};
tdResHandle	CryRSXMemCpyASyncScaled2D(		void* pDst,uint32 DstSizeX,uint32 DstSizeY,uint32 DstPitch,
																		const void* pSrc,uint32 SrcSizeX,uint32 SrcSizeY,uint32 SrcPitch)
																					{return tdLayer0::Device()->MemCpyScaled2D(pDst,DstSizeX,DstSizeY,DstPitch,pSrc,SrcSizeX,SrcSizeY,SrcPitch,false);};
*/
#endif
