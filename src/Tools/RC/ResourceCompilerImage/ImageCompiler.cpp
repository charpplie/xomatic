#include "StdAfx.h"
#include <assert.h>													// assert()
#include <Dbghelp.h>												// GetTimestampForLoadedLibrary

#include "IRCLog.h"													// IRCLog
#include "IConfig.h"												// IConfig

#include "ImageCompiler.h"									// CImageCompiler
#include "ImageUserDialog.h"								// CImageUserDialog
#include "ICfgFile.h"												// ICfgFile

#define _TIFF_DATA_TYPEDEFS_								// because we defined uint32,... already
#include "TiffLib/include/tiffio.h"					// TIFF library

#include "PixelFormats.h"										// g_pixelformats

//#include "neuquant.h"

#include "ImageObject.h"
#include <commctrl.h>												// TCITEM

#include "Cry_Math.h"												// Vec3
#include "Cry_Color.h"											// ColorF
#include "IResCompiler.h"										// Vec3
#include <Endian.h>													// Endian converion(for ImageExtensionHelper)
#include <ITexture.h>												// eTEX enum
#include "tools.h"													// GetBoolParam()
#include "ATICompressorLib\ATI_Compress.h"	// ATI_TC_ConvertTexture()
#include "NVidiaLib\dxtlib.h"								// NVidia DXT processing
#include "SuffixUtil.h"											// CSuffixUtil
#include "TextureFallbackSystem.h"					// CTextureFallbackSystem
#include "ImageExtensionHelper.h"						// CImageExtensionHelper
#include "BitFiddling.h"										// ConvertBlock3DcToDXT5
#include <ICryCompressorRC.h>								// compressor(for XBox tiling)
#include "ColorChart/ColorChart.h"

#include "CryTextureCompressor/CryTextureCompressor.h"

#include "NVidia/nvtt/CompressDXT.h"
#include "NVidia/nvimage/ColorBlock.h"
#include "NVidia/nvimage/BlockDXT.h"


static CryCriticalSectionRC g_nvDXTLock;
static CryCriticalSectionRC g_AtiCubemapLock;
static CryCriticalSectionRC g_InitDirectXLock;


const char* stristr(const char* szString, const char* szSubstring)
{
  int nSuperstringLength = (int)strlen(szString);
  int nSubstringLength = (int)strlen(szSubstring);

  for(int nSubstringPos = 0; nSubstringPos <= nSuperstringLength - nSubstringLength; ++nSubstringPos)
  {
		if (strnicmp(szString+nSubstringPos, szSubstring, nSubstringLength) == 0)
			return szString+nSubstringPos;
  }

	return NULL;
}



// constructor
CImageCompiler::CImageCompiler(CTextureFallbackSystem* pTextureFallbackSystem)
:	m_pTextureFallbackSystem(pTextureFallbackSystem), m_Progress(*this)
{
	m_pd3ddev=0;
	m_pd3d=0;
	m_pInputImage=0;
	m_pFinalImage=0;
	m_pCC=0;
	m_iOrigPixelFormatNo=0;
	m_hWnd=0;
	m_bInitialized=false;
	m_fTimeProfiled=0;
	m_fCurrentMipFilterSharpness = -1.f;
	m_bValidAlphaCoverageFilterset = false;
	m_bInternalPreview = false;
	m_pImageUserDialog = 0;

	ClearInputImageFlags();
}


// destructor
CImageCompiler::~CImageCompiler()
{
	DeInit();

	assert(!m_pd3ddev);
	assert(!m_pd3d);
	assert(!m_pInputImage);
	assert(!m_pFinalImage);
}


void CImageCompiler::FreeTexture()
{
	if(m_pFinalImage != m_pInputImage)
	delete m_pFinalImage;
	m_pFinalImage=0;
	delete m_pInputImage; m_pInputImage=0;
}

ImageObject *CImageCompiler::GetOutputAndRelease()
{
	ImageObject *pRet=m_pFinalImage;

	m_pFinalImage=0;

	return pRet;
}


char *CImageCompiler::GetNameFromD3DFormat( D3DFORMAT inFormat )
{
#define FMT(fmt) case fmt: return #fmt;

	switch(inFormat)
	{
		FMT(D3DFMT_UNKNOWN);
		FMT(D3DFMT_R8G8B8);
		FMT(D3DFMT_A8R8G8B8);
		FMT(D3DFMT_X8R8G8B8);
		FMT(D3DFMT_R5G6B5);
		FMT(D3DFMT_X1R5G5B5);
		FMT(D3DFMT_A1R5G5B5);
		FMT(D3DFMT_A4R4G4B4);
		FMT(D3DFMT_R3G3B2);
		FMT(D3DFMT_A8);
		FMT(D3DFMT_A8R3G3B2);
		FMT(D3DFMT_X4R4G4B4);
		FMT(D3DFMT_A2B10G10R10);
		FMT(D3DFMT_A8B8G8R8);
		FMT(D3DFMT_X8B8G8R8);
		FMT(D3DFMT_G16R16);
		FMT(D3DFMT_A2R10G10B10);
		FMT(D3DFMT_A16B16G16R16);
		FMT(D3DFMT_A8P8);
		FMT(D3DFMT_P8);
		FMT(D3DFMT_L8);
		FMT(D3DFMT_A8L8);
		FMT(D3DFMT_A4L4);
		FMT(D3DFMT_V8U8);
		FMT(D3DFMT_L6V5U5);
		FMT(D3DFMT_X8L8V8U8);
		FMT(D3DFMT_Q8W8V8U8);
		FMT(D3DFMT_V16U16);
		FMT(D3DFMT_A2W10V10U10);
		FMT(D3DFMT_UYVY);
		FMT(D3DFMT_R8G8_B8G8);
		FMT(D3DFMT_YUY2);
		FMT(D3DFMT_G8R8_G8B8);
		FMT(D3DFMT_DXT1);
		FMT(D3DFMT_DXT2);
		FMT(D3DFMT_DXT3);
		FMT(D3DFMT_DXT4);
		FMT(D3DFMT_DXT5);
		FMT(D3DFMT_D16_LOCKABLE);
		FMT(D3DFMT_D32);
		FMT(D3DFMT_D15S1);
		FMT(D3DFMT_D24S8);
		FMT(D3DFMT_D24X8);
		FMT(D3DFMT_D24X4S4);
		FMT(D3DFMT_D16);
		FMT(D3DFMT_D32F_LOCKABLE);
		FMT(D3DFMT_D24FS8);
		FMT(D3DFMT_L16);
		FMT(D3DFMT_VERTEXDATA);
		FMT(D3DFMT_INDEX16);
		FMT(D3DFMT_INDEX32);
		FMT(D3DFMT_Q16W16V16U16);
		FMT(D3DFMT_MULTI2_ARGB8);
		FMT(D3DFMT_R16F);
		FMT(D3DFMT_G16R16F);
		FMT(D3DFMT_A16B16G16R16F);
		FMT(D3DFMT_R32F);
		FMT(D3DFMT_G32R32F);
		FMT(D3DFMT_A32B32G32R32F);
		FMT(D3DFMT_CxV8U8);
	}
#undef FMT

	return "D3DFMT_UNKNOWN";
}



void CImageCompiler::GetInputFromMemory( ImageObject *pInput )
{
	FreeTexture();

	m_pInputImage=pInput;

	ClearInputImageFlags();

	if(m_pInputImage)
		m_bInputUsesAlpha=m_pInputImage->HasAlpha();
}

void CImageCompiler::DeInit()
{
	// no logging allowed - m_pCC is already 0

	if(m_bInitialized)
	{
		m_bInitialized = false;

		FreeTexture();

		ReleasePpo(&m_pd3ddev);
		ReleasePpo(&m_pd3d);

		UnregisterClass("skeleton", g_hInst);
	}
}


bool CImageCompiler::SaveOutput( ImageObject * &pOutput, const char *szType, const char *lpszPathName )
{
	assert(lpszPathName);
	assert(pOutput);

	if(!pOutput)
	{
		RCLogError("CImageCompiler::Save() failed (conversion failed?)");
		return false;
	}

	// Force remove of the read only flag.
	SetFileAttributes( lpszPathName,FILE_ATTRIBUTE_ARCHIVE );

	if(m_Props.GetGlobalCompressor()==CImageProperties::eCGTIF)
	{
		if(!SaveAsTIFF(pOutput,lpszPathName))
		{
			RCLogError("CImageCompiler::SaveAsTIFF() failed");
			return false;
		}
	}
	else
	{
		if(!pOutput->SaveImage(lpszPathName))
		{
			RCLogError("CImageCompiler::Save failed (file IO?)");
			return false;
		}
	}

	uint32 dwAttached=0;

	if(pOutput->GetAttachedImage())
		dwAttached=CalcTextureMemory(pOutput->GetAttachedImage());

	if(dwAttached)
		RCLog("   %s: %d bytes (including %d bytes attached)",szType,CalcTextureMemory(pOutput),dwAttached);
	 else
		RCLog("   %s: %d bytes",szType,CalcTextureMemory(pOutput));

	return true;
}


LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd,msg,wParam,lParam);
}

void CImageCompiler::UserMessage( const char *szMessage )
{
	assert(szMessage);
	MessageBox(0,szMessage,"ResourcCompileImage",MB_OK);
}


bool CImageCompiler::Init( HWND inhWnd )
{
	m_bInitialized = true;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hInst;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= "skeleton";
	wcex.hIconSm		= 0;

	RegisterClassEx(&wcex);

  m_hWnd = CreateWindow(
      "skeleton", "the title",
      WS_OVERLAPPEDWINDOW,
      0, 0, 400, 400,
      NULL, NULL, g_hInst, NULL);

   // return if error on CreateWindow
  if(m_hWnd == NULL)
		return false;

	InitCommonControls();

/* - not need to log this - userdialog shows available formats
	RCLog("");
	RCLog("ResourceCompilerImage supported output pixel formats:");

	int iCount=CPixelFormats::GetPixelFormatCount();

	for(int i=0;i<iCount;i++)
	{
		if(g_pixelformats[i].bDestinationFormat)
		{
			RCLog(" * %s",g_pixelformats[i].szName);
			RCLog("   %s",g_pixelformats[i].szDescription);
		}
	}
	RCLog("");
*/

	return true;
}


bool CImageCompiler::InitDirectX()
{
	assert(m_pCC);

	if(m_pd3d && m_pd3ddev)
	{
		return true;   // DirectX is already initialized
	}
	
	// We using a lock here because otherwise we will have DirectX's initialization errors sometimes
	// (it happened very rarely, although)
	CryAutoLockRC lock(g_InitDirectXLock);

	RCLog("");
	RCLog("Init DirectX ...");
	RCLog("");

	// Initialize DirectDraw
	m_pd3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pd3d == NULL)
	{
		//UserMessage("Direct3DCreate9() failed (wrong DirectX version?)");
		RCLogError("Direct3DCreate9() failed (wrong DirectX version?)");
		return false;
	}

	ZeroMemory(&m_presentParams, sizeof(m_presentParams));
	m_presentParams.Windowed = TRUE;
	m_presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
	m_presentParams.BackBufferWidth = 256;
	m_presentParams.BackBufferHeight = 256;
	m_presentParams.BackBufferFormat = D3DFMT_UNKNOWN;

	//devType = D3DDEVTYPE_REF; 

	bool bUserDialog = false;
	m_pCC->config->Get("userdialog",bUserDialog);

	D3DDEVTYPE devType;
	if(bUserDialog)
	{
		devType = D3DDEVTYPE_HAL;
		RCLog("    ... HAL");
	}
	else
	{
		devType = D3DDEVTYPE_NULLREF;		// Mike Borrows hinted to us this can be used
		RCLog("    ... NULLREF");
	}

	const HRESULT hr = m_pd3d->CreateDevice(D3DADAPTER_DEFAULT, devType, m_hWnd, 
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_presentParams, &m_pd3ddev);

	if (FAILED(hr))
	{
		m_pd3d->Release();
		m_pd3d = 0;

		char msg[512];
		sprintf(msg, "CreateDevice() failed (code: %08x). May be DirectX version is wrong - at minimum you should have DirectX9.0c.", (unsigned int)hr);

		UserMessage(msg);
		RCLogError(msg);

		return false;
	}

	if (devType != D3DDEVTYPE_NULLREF)
	{
		D3DCAPS9 Caps;
		m_pd3ddev->GetDeviceCaps(&Caps);

		if (Caps.PrimitiveMiscCaps & D3DPMISCCAPS_NULLREFERENCE)
		{
			m_pd3ddev->Release();
			m_pd3ddev = 0;

			m_pd3d->Release();
			m_pd3d = 0;

			char msg[512];
			sprintf(msg, "CreateDevice() D3DPMISCCAPS_NULLREFERENCE (Is you default adapter a 3D gfx card?)");

			UserMessage(msg);
			RCLogError(msg);

			return false;
		}
	}

	return true;
}


ImageObject *CImageCompiler::LoadImage( const char *lpszPathName, const char *lpszExtension, IConfig *pFileConfig )
{
	if(stricmp(lpszExtension,"tif")==0)
		return LoadUsingTIFLoader(lpszPathName,pFileConfig);		// we handle the loading ourselves

	RCLogError("unknown Extension:'%s'",lpszExtension );
	return 0;			// unknown extension
}

/*
bool CImageCompiler::IsDDNDIFProcessing( ImageObject *pCurrentImage ) const
{
	assert(m_pCC);

	if(GetBoolParam(m_pCC->config,"ddndifprocessing",false))
	if(CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"ddndif"))
	if(!IsPerfectGreyscale(pCurrentImage))
		return true;

	return false;
}
*/


bool CImageCompiler::LoadInput( const char *lpszPathName, const char *lpszExtension, const bool bLoadConfig )
{
	FreeTexture();

	// testcases for CSuffixUtil::CheckForSuffix()
	assert(!CSuffixUtil::CheckForSuffix("dirt_ddn.dds","ddndif"));
	assert(CSuffixUtil::CheckForSuffix("dirt_ddndif.dds","ddndif"));
	assert(!CSuffixUtil::CheckForSuffix("dirt.dds","ddndif"));
	assert(CSuffixUtil::CheckForSuffix("dirt_ddn_ddndif.dds","ddndif"));
	assert(!CSuffixUtil::CheckForSuffix("dirt_ddndif_ddn.dds","ddndif"));
	assert(CSuffixUtil::CheckForSuffix("dirt_ddndif","ddndif"));
	assert(!CSuffixUtil::CheckForSuffix("dirt_ddndif2","ddndif"));
	assert(!CSuffixUtil::CheckForSuffix("dirt_ddndif2_","ddndif"));

	assert(!m_pInputImage);

	m_pInputImage = LoadImage(lpszPathName,lpszExtension,bLoadConfig ? m_pCC->config : 0);

	if(!m_pInputImage)
		return false;

	if(CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"ddndif"))
	{
		m_bInputUsesDenormalizedNormals=true;
	}

	return true;
}


string CImageCompiler::GetSpecialInstructionsFromTIFF( TIFF *tif )
{
	// get image metadata
	{
		unsigned char *pPtr;
		unsigned int wc;

		if(TIFFGetField(tif,TIFFTAG_PHOTOSHOP,&wc,&pPtr))		// 34377 IPTC TAG
		{
			unsigned char *pPtrEnd = pPtr + wc;

			while(pPtr!=pPtrEnd)
			{
				if(pPtr[0]=='8' && pPtr[1]=='B' && pPtr[2]=='I' && pPtr[3]=='M')
				{
					pPtr+=4;

					unsigned short Id = ((unsigned short)(pPtr[0])<<8) | (unsigned short)pPtr[1];

					pPtr+=2;

					// get pascal string 
					unsigned int dwNameSize = (unsigned int)pPtr[0];

					pPtr++;

					char szName[256];

					strncpy(szName,(char *)pPtr,dwNameSize);
					szName[dwNameSize]=0;
/*
					OutputDebugString("Tag: '");
					OutputDebugString(szName);
					OutputDebugString("'\n");
*/
					pPtr+=dwNameSize;

					// align 2 bytes
					if((dwNameSize%2)==0)
						pPtr++;

					unsigned int dwSize = ((unsigned int)(pPtr[0])<<24) | ((unsigned int)(pPtr[1])<<16) | ((unsigned int)(pPtr[2])<<8) |(unsigned int)pPtr[3];
					pPtr+=4;

					//if(strcmp(szName,"Caption")==0)
					//	return GetSpecialInstructionsFromPhotoshopCaption((unsigned char *)pPtr,dwSize);

					if (Id == 0x0404)
					{
						m_iptcHeader.Parse(pPtr, dwSize);
						std::vector<unsigned char> buffer;
						m_iptcHeader.GetCombinedFields(CIPTCHeader::FieldSpecialInstructions, buffer, " ");
						return string(reinterpret_cast<char*>(&buffer[0]));
					}

					pPtr+=dwSize;

					// align 2 bytes
					if((dwSize%2)!=0)
						pPtr++;
				}
				else
				{
					assert(0);
				}
			}
		}
	}

	return string("");
}

bool CImageCompiler::IsFinalPixelFormatValidForNormalmaps() const
{
	const SPixelFormats *pInfo = CPixelFormats::GetPixelFormatInfo(m_Props.GetDestPixelFormat(m_bInputUsesAlpha));

	if(!pInfo)
		return false;

	return pInfo->DxtNo==D3DFMT_3DC || 
		(pInfo->DxtNo==D3DFMT_CTX1 && m_pCC->platform==ePlatform_X360)
		|| (pInfo->DxtNo==D3DFMT_DXT1 && m_pCC->platform==ePlatform_PS3);
}
bool CImageCompiler::IsPointOutsideOfRectangle(const uint32 dwX, const uint32 dwY, const RECT& rectangle) const
{
	return ((dwY<rectangle.top || dwY>=rectangle.bottom)||(dwX<rectangle.left || dwX>=rectangle.right));
}

bool CImageCompiler::IsFinalPixelFormatCompressed() const
{
	const SPixelFormats *pInfo = CPixelFormats::GetPixelFormatInfo(m_Props.GetDestPixelFormat(m_bInputUsesAlpha));

	if(!pInfo)
		return false;

	return pInfo->bCompressed;
}

ImageObject *CImageCompiler::LoadUsingTIFLoader( const char *lpszPathName, IConfig *pFileConfig )
{
	TIFF* tif = TIFFOpen(lpszPathName,"r");

	ImageObject *pRet=0;

	if(tif) 
	{
		uint32 dwChannels=3;
		uint32 dwBitsPerChannel=8;
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,&dwChannels);
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,&dwBitsPerChannel);					// usually 8 bits
		uint32 dwBytesPerPixel = dwChannels * dwBitsPerChannel / 8;

		RCLog("CImageCompiler::LoadUsingTIFLoader dwChannels=%d",dwChannels);

		switch(dwBytesPerPixel)
		{
		case 1:	//A8/L8
		case 4:	//BGRA8
		case 3:	//BGR8
			pRet = Load8BitImageFromTIFF(tif);
			break;
		case 6:	// BGRA16f
		case 8:	// BGRA16f
			pRet = Load16BitHDRImageFromTIFF(tif);
			break;
		default:
			assert(0);
			RCLogError("Unsupported TIFF pixel format!");
			return NULL;
		}

		// m_pCC->config
		if(pRet && pFileConfig)
		{
			string sSpecialInstructions = GetSpecialInstructionsFromTIFF(tif);

			OutputDebugString("\nFromTIF:\n");
			OutputDebugString(sSpecialInstructions.c_str());
			OutputDebugString("\n");

			UpdateOrCreateEntryFromString(pFileConfig,sSpecialInstructions.c_str());
		}

		TIFFClose(tif);
	}
	else RCLogError("CImageCompiler::LoadUsingTIFLoader TIFFOpen failed");


//	m_pCC->config->SetConfig( eCP_PriorityFile,COMMON_SECTION,m_pCC->config );
//PlattformFix	m_pCC->config->SetConfig( eCP_PriorityFile,m_pCC->plattformName,m_pCC->config );

	ClearInputImageFlags();

	if(pRet)
		m_bInputUsesAlpha=pRet->HasAlpha();

	return pRet;
}

ImageObject * CImageCompiler::Load8BitImageFromTIFF(TIFF* tif)
{
	ImageObject *pRet=0;

	char *dest;
	uint32 dwPitch;
	uint32 dwWidth, dwHeight;
	size_t npixels;
	uint32* raster;
	uint32 dwChannels=3;
	uint32 dwBitsPerChannel=8;

	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,&dwChannels);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,&dwBitsPerChannel);					// usually 8 bits

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);

	npixels = dwWidth * dwHeight;
	uint32 dwBytesPerPixel = dwChannels * dwBitsPerChannel / 8;
	tsize_t sizeRaster = (tsize_t)(npixels * sizeof(uint32));
	raster = (uint32*) new char[sizeRaster];

	bool bSetAlphaToWhite = (dwChannels==3);			// 3 channel textures should be treated as 4 channel ones with white alpha

	if(raster)
	{
		if(TIFFReadRGBAImageOriented(tif, dwWidth, dwHeight, raster, ORIENTATION_TOPLEFT, 0)) 
		{
			pRet = new CRAWImage(dwWidth,dwHeight,1,CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8),IsFinalPixelFormatCompressed());

			if(pRet->Lock(0,0,dest,dwPitch))
			{
				for(uint32 dwY=0;dwY<dwHeight;++dwY)
				{
					char *src2 = (char *)&raster[dwY*dwWidth];
					char *dest2 = &dest[dwPitch*dwY];

					if(bSetAlphaToWhite)
					{
						for(uint32 dwX=0;dwX<dwWidth;++dwX)
						{
							// change byte order
							dest2[0] = src2[2];			// blue
							dest2[1] = src2[1];
							dest2[2] = src2[0];
							dest2[3] = (char)0xff;				// alpha

							dest2+=4;src2+=4;
						}
					}
					else
					{
						for(uint32 dwX=0;dwX<dwWidth;++dwX)
						{
							// change byte order
							dest2[0] = src2[2];			// blue
							dest2[1] = src2[1];
							dest2[2] = src2[0];
							dest2[3] = src2[3];			// alpha

							dest2+=4;src2+=4;
						}
					}
				}

				pRet->Unlock(0);
			}
			else
			{
				RCLogError("CImageCompiler::LoadUsingTIFLoader Lock failed");
				delete pRet; pRet=0;
			}
		}
		else RCLogError("CImageCompiler::TIFFReadRGBAImage failed (RGA/RGBA image?)");

		SAFE_DELETE_ARRAY(raster);
	}
	else RCLogError("LoadUsingTIFLoader out of memory %dBytes",sizeRaster);

	return pRet;
}

ImageObject * CImageCompiler::Load16BitHDRImageFromTIFF( TIFF* tif )
{
	ImageObject *pRet=0;

	char* dest;
	uint32 dwPitch, dwChannels=3;

	tdata_t buf;

	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,&dwChannels);
	assert(dwChannels ==3 || dwChannels == 4);

	uint32 dwWidth, dwHeight;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);

	pRet = new CFloat4Image(dwWidth, dwHeight);
	if(pRet->Lock(0,0,dest,dwPitch))
	{
		buf = new char[dwPitch];
		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		{
			TIFFReadScanline(tif, buf, dwY, 0);	// read raw row
			D3DXVECTOR4_16F* src = (D3DXVECTOR4_16F*)buf;

			const D3DXVECTOR4_16F *srcLine = (const D3DXVECTOR4_16F *)buf;
			Vec4 *destLine = (Vec4*)&dest[dwPitch*dwY];

			for(uint32 dwX=0;dwX<dwWidth;++dwX)
			{
				// change format RGBA16f -> RGBA32f
				D3DXVECTOR4 srcPixel(&srcLine->x);
				// clamp negative values
				srcPixel.x = max(0.f, srcPixel.x);
				srcPixel.y = max(0.f, srcPixel.y);
				srcPixel.z = max(0.f, srcPixel.z);
				srcPixel.w = max(0.f, srcPixel.w);
				destLine->x = srcPixel.x;
				destLine->y = srcPixel.y;
				destLine->z = srcPixel.z;
				destLine->w = srcPixel.w;
				destLine++;
				if(dwChannels == 4)
					srcLine++;
				else
					srcLine = (const D3DXVECTOR4_16F *)(((short*)srcLine) + 3);
			}
		}
		pRet->Unlock(0);
		SAFE_DELETE_ARRAY(buf);
	}

	return pRet;
}
// needed for UpdateAndSaveConfigToTIF 
class CConfigString :public IConfigSink
{
public:
	// constructor
	CConfigString() :m_pMemBlock(0)
	{
	}

	// destructor
	~CConfigString()
	{
		delete m_pMemBlock;
	}

	// interface IConfigSink -------------------------------------------

	virtual void Set( const EConfigPriority ePri, const char *key, const char* value )
	{
		assert(ePri==eCP_PriorityFile);

		bool Quotes=false;				// only if quotes are needed
		{
			const char *p=value;

			while(*p)
			{
				if(!IsValidNameChar(*p))
				{
					Quotes=true;
					break;
				}
				++p;
			}
		}

		if(Quotes)
			m_sValue += string("/") + key + "=\"" + value + "\" ";
		 else
			m_sValue += string("/") + key + "=" + value + " ";
	}

	// --------------------------------

	// call only once, memory is released by this class
	unsigned char *GetResult( uint32 &outdwSize )
	{
		//assert(!m_pMemBlock);

		//unsigned short wSize = m_sValue.GetLength();
		//unsigned char *pwSize = (unsigned char *)&wSize;

		//unsigned int dwSize = wSize+5;
		//unsigned char *pdwSize = (unsigned char *)&dwSize;

		////const char* szCaptionString = "Caption";
		//const char* szCaptionString = "";
		//outdwSize=4+2+1+(uint32)strlen(szCaptionString)+4+dwSize;

		//m_pMemBlock = new unsigned char[outdwSize];

		//int nMemBlockIndex = 0;
		//m_pMemBlock[nMemBlockIndex++]='8';	m_pMemBlock[nMemBlockIndex++]='B';	m_pMemBlock[nMemBlockIndex++]='I';	m_pMemBlock[nMemBlockIndex++]='M';
		//m_pMemBlock[nMemBlockIndex++]=0x04; m_pMemBlock[nMemBlockIndex++]=0x04;
		//m_pMemBlock[nMemBlockIndex++]=(unsigned char)strlen(szCaptionString); strncpy((char *)&m_pMemBlock[nMemBlockIndex],szCaptionString,strlen(szCaptionString)); nMemBlockIndex += strlen(szCaptionString);
		//m_pMemBlock[nMemBlockIndex++]=pdwSize[3];	m_pMemBlock[nMemBlockIndex++]=pdwSize[2];	m_pMemBlock[nMemBlockIndex++]=pdwSize[1];	m_pMemBlock[nMemBlockIndex++]=pdwSize[0];
		//m_pMemBlock[nMemBlockIndex++]=28;	m_pMemBlock[nMemBlockIndex++]=2;
		//m_pMemBlock[nMemBlockIndex++]=40;		// Special Instructions
		//m_pMemBlock[nMemBlockIndex++]=pwSize[1];	m_pMemBlock[nMemBlockIndex++]=pwSize[0];

		//memcpy(&m_pMemBlock[nMemBlockIndex],m_sValue,m_sValue.GetLength()); nMemBlockIndex += m_sValue.GetLength();

		//// debug
		//OutputDebugString("\nWrite SpecialInstructions in TIFF: '");
		//OutputDebugString(m_sValue);		
		//OutputDebugString("'\n");

		//return m_pMemBlock;
#define ADD(str) std::copy(str, str + sizeof(str) - 1, std::back_inserter(irb))
		std::vector<char> irb;
		ADD("8BIM");
		ADD("\x04\x04");
		ADD("\0\0");
		int irbSizePosition = int(irb.size());
		ADD("\0\0\0\0");
		int irbDataStart = int(irb.size());
		ADD("\x1C\x02");
		ADD("\0");
		ADD("\x00\x02");
		ADD("\x00\x02");
		ADD("\x1C\x02");
		ADD("\x28");
		int captionSizePosition = int(irb.size());
		ADD("\0\0");
		int captionStartPosition = int(irb.size());
		//ADD(m_sValue.GetString());
		std::copy(m_sValue.c_str(), m_sValue.c_str() + m_sValue.size(), std::back_inserter(irb));
#undef ADD

		int captionSize = int(irb.size()) - captionStartPosition;
		char* captionSizePointer = &irb[0] + captionSizePosition;
		captionSizePointer[0] = ((captionSize >> 8) & 0xFF); captionSizePointer[1] = (captionSize & 0xFF);

		int irbSize = int(irb.size()) - irbDataStart;
		char* irbSizePointer = &irb[0] + irbSizePosition;
		irbSizePointer[0] = ((irbSize >> 24) & 0xFF); irbSizePointer[1] = ((irbSize >> 16) & 0xFF); irbSizePointer[2] = ((irbSize >> 8) & 0xFF); irbSizePointer[3] = (irbSize & 0xFF);

		m_pMemBlock = new unsigned char[int(irb.size())];
		memcpy(m_pMemBlock, &irb[0], int(irb.size()));

		outdwSize = int(irb.size());
		return m_pMemBlock;
	}

private: // -----------------------------------------------------

	string						m_sValue;					//
	unsigned char *		m_pMemBlock;			//
};




// Arguments:
//   lpszPathName must not be 0
//   pCfgFile must not be 0
//   m_pCC name is misleading but this way we can use CCLOG
static bool UpdateAndSaveConfigToTIF( const char *lpszPathName, IConfig *pCfgFile, ConvertContext *m_pCC )
{
	assert(lpszPathName);
	assert(pCfgFile);

	bool bRet=false;

	TIFF* tif_in = TIFFOpen(lpszPathName,"r");	

	if(tif_in) 
	{			 
		uint32 dwWidth( 0 ), dwHeight( 0 ), dwPlanes ( 0 ), dwBitsPerSample( 0 );
		uint32 dwPhotometric( 0 ), dwOrientation( 0 ), dwPlanarConfig( 0 );
		uint32 dwRowsPerStrip( 0 ), dwCompression( 0 );

		// get properties
		TIFFGetField(tif_in, TIFFTAG_IMAGEWIDTH, &dwWidth);
		TIFFGetField(tif_in, TIFFTAG_IMAGELENGTH, &dwHeight);
		TIFFGetField(tif_in, TIFFTAG_SAMPLESPERPIXEL, &dwPlanes);
		TIFFGetField(tif_in, TIFFTAG_BITSPERSAMPLE, &dwBitsPerSample ); 
		TIFFGetField(tif_in, TIFFTAG_ROWSPERSTRIP, &dwRowsPerStrip ); 
		TIFFGetField(tif_in, TIFFTAG_COMPRESSION, &dwCompression);
		TIFFGetField(tif_in, TIFFTAG_PHOTOMETRIC, &dwPhotometric );
		TIFFGetField(tif_in, TIFFTAG_PLANARCONFIG, &dwPlanarConfig );
		TIFFGetField(tif_in, TIFFTAG_ORIENTATION, &dwOrientation );

		const uint32 dwBytesPerSample = dwBitsPerSample / 8;

		size_t npixels( dwWidth * dwHeight );

		char* raster = new char[npixels * dwPlanes * dwBytesPerSample];

		if(!raster)
			RCLogError("UpdateAndSaveConfigToTIF out of memory");

		if(raster
		&& (dwPlanes==1 || dwPlanes==3 || dwPlanes==4)
		&& (dwBitsPerSample==8 || dwBitsPerSample==16)) 
		{
			// load image as we overwrite the file
			bool loadingWasSuccessful( true );
			for( uint32 row( 0 ); row < dwHeight; ++row )
			{
				if( 0 > TIFFReadScanline(tif_in, &raster[dwWidth*row*dwPlanes*dwBytesPerSample], row, 0) )					
				{
					loadingWasSuccessful = false;
					break;
				}
			}
			TIFFClose(tif_in);tif_in=0;

			// write image with resource compiler meta data
			if( false != loadingWasSuccessful )
			{				
				TIFF* tif_out = TIFFOpen(lpszPathName,"w");

				if(tif_out)
				{
					// set typical properties
					TIFFSetField(tif_out, TIFFTAG_IMAGEWIDTH, dwWidth);
					TIFFSetField(tif_out, TIFFTAG_IMAGELENGTH, dwHeight);
					TIFFSetField(tif_out, TIFFTAG_SAMPLESPERPIXEL, dwPlanes);
					TIFFSetField(tif_out, TIFFTAG_BITSPERSAMPLE, dwBitsPerSample ); 
					TIFFSetField(tif_out, TIFFTAG_ROWSPERSTRIP, dwRowsPerStrip); 
					TIFFSetField(tif_out, TIFFTAG_COMPRESSION, dwCompression);
					TIFFSetField(tif_out, TIFFTAG_PHOTOMETRIC, dwPhotometric );
					TIFFSetField(tif_out, TIFFTAG_PLANARCONFIG, dwPlanarConfig );
					TIFFSetField(tif_out, TIFFTAG_ORIENTATION, dwOrientation );

					// set meta data
					{
						CConfigString configString;

						pCfgFile->SetToConfig(eCP_PriorityFile,&configString);

						uint32 dwSize; 
						unsigned char *pMemBlock = configString.GetResult(dwSize);

						TIFFSetField(tif_out,TIFFTAG_PHOTOSHOP,dwSize,pMemBlock);		// 34377 IPTC TAG
					}

					// write image
					bool writingWasSucessful( true );
					for( uint32 row( 0 ); row < dwHeight; ++row )
					{
						if( 0 > TIFFWriteScanline(tif_out, &raster[dwWidth*row*dwPlanes*dwBytesPerSample], row, 0) )
						{
							writingWasSucessful = false;
							break;
						}						
					}

					SAFE_DELETE_ARRAY(raster);
					
					TIFFClose(tif_out);
					
					bRet = writingWasSucessful;
				}
			}
		}
		SAFE_DELETE_ARRAY(raster);
		if(tif_in)TIFFClose(tif_in);
	}

	return bRet;
}




/*
bool CImageCompiler::LoadUsingDirectX( const char *lpszPathName )
{
	if(!InitDirectX())
		return false;

	assert(m_pCC);		// need this pointer set

	D3DXIMAGE_INFO imageinfo;
	D3DXIMAGE_INFO imageinfo2;

	assert(!m_pInputImage);

	if( FAILED( D3DXGetImageInfoFromFile( lpszPathName, &imageinfo ) ) )
	{
		RCLogError("D3DXGetImageInfoFromFile failed");
		return FALSE;
	}

	HRESULT hRes;

	switch( imageinfo.ResourceType )
	{
	case D3DRTYPE_TEXTURE:
		{
			LPDIRECT3DTEXTURE9 pNewDXImage=0;

			hRes=D3DXCreateTextureFromFileEx( m_pd3ddev, lpszPathName, 
				imageinfo.Width, imageinfo.Height, imageinfo.MipLevels, 0,
				imageinfo.Format, D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, 
				&imageinfo2, NULL, (LPDIRECT3DTEXTURE9*)&pNewDXImage );

			if(FAILED(hRes))
			{
				RCLogError("D3DXCreateTextureFromFileEx failed (power of two?) '%s'",DXGetErrorString9(hRes));
				return false;
			}

			m_pInputImage = new DXImage(pNewDXImage);
		}
		break;

	case D3DRTYPE_VOLUMETEXTURE:
		RCLogError("ERROR_COULDNTLOADFILE Volumetextures are not supported");
		return false;

	case D3DRTYPE_CUBETEXTURE:
		RCLogError("ERROR_COULDNTLOADFILE Cubemaps are not supported");
		return false;	

	default:
		RCLogError("ERROR_COULDNTLOADFILE");
		return false;
	}

	ClearInputImageFlags();

	if(m_pInputImage)
		m_bInputUsesAlpha=m_pInputImage->HasAlpha();

	return true;
}
*/



inline uint32 VEC4ToUINT32( Vec4 &inVec)
{
	int ia = (int)(inVec.w*255.0f + 0.499f);
	int ir = (int)(inVec.x*255.0f + 0.499f);
	int ig = (int)(inVec.y*255.0f + 0.499f);
	int ib = (int)(inVec.z*255.0f + 0.499f);

	if (ir < 0)ir = 0;
	if (ir > 255)ir = 255;
	if (ig < 0)ig = 0;
	if (ig > 255)ig = 255;
	if (ib < 0)ib = 0;
	if (ib > 255)ib = 255;
	if (ia < 0)ia = 0;
	if (ia > 255)ia = 255;

	return (((uint32)ia)<<24) | (((uint32)ir)<<16) | (((uint32)ig)<<8) | (uint32)ib;
}


inline Vec4 UINT32ToVEC4( uint32 dwValue )
{
	uint32 ucA = dwValue >> 24;
	uint32 ucR = (dwValue>>16)&0xff;
	uint32 ucG = (dwValue>>8)&0xff;
	uint32 ucB = dwValue&0xff;

	return Vec4(ucR/255.0f,ucG/255.0f,ucB/255.0f,ucA/255.0f);
}

/*
// Arguments:
//   fStrength - 0=off..1=full
void Sharpen( CSimpleBitmap<Vec4> &InOut, const uint32 dwWidth, const uint32 dwHeight, const float fStrength )
{
	CSimpleBitmap<Vec4> In;
	
	In.Alloc(dwWidth,dwHeight);
	
	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		Vec4 vValue;
		InOut.Get(dwX,dwY,vValue);
		In.Set(dwX,dwY,vValue);
	}

	CWeightFilterSet filterset_sharpen;

	filterset_sharpen.CreateSharpen(fStrength);

	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		Vec4 vResult(0,0,0,0);

		filterset_sharpen.GetBlockWithFilter(In,dwX,dwY,vResult);

		// needed when using sharpening filter in mip map generation
		vResult.x = clamp_tpl(vResult.x,0.0f,1.0f);
		vResult.y = clamp_tpl(vResult.y,0.0f,1.0f);
		vResult.z = clamp_tpl(vResult.z,0.0f,1.0f);
		vResult.w = clamp_tpl(vResult.w,0.0f,1.0f);

		InOut.Set(dwX,dwY,vResult);
	}
}
*/






ImageObject *CImageCompiler::CreateMipMaps( const ImageObject *pInputImage, uint32 indwReduceResolution,
	const bool inbRemoveMips, const bool bRenormalize, const float fGamma ) 
{
	assert(fGamma>0);
	assert(pInputImage);

	const bool bMaintainAlphaCoverage = m_Props.GetMaintainAlphaCoverage();

	const float fInvGamma = 1.0f/fGamma;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	const bool bCubemap = m_Props.GetCubemap() && pInputImage->IsCubemap();

	const bool bFinalPixelFormatIsCompressed = IsFinalPixelFormatCompressed();

	const uint32 dwMinSize = bFinalPixelFormatIsCompressed ? 4 : 1;			// compressed textures need special treatment for textures smaller than 4x4

	if(dwWidth < dwMinSize)
	{
		RCLogError("CImageCompiler::CreateMipMaps failed, width of input image is too small (width is %i, minimum is %i)", dwWidth, dwMinSize);
		return 0;
	}

	if(dwHeight < dwMinSize)
	{
		RCLogError("CImageCompiler::CreateMipMaps failed, height of input image is too small (height is %i, minimum is %i)", dwHeight, dwMinSize);
		return 0;
	}

	//early out: check for canceling preview generation
	if (m_bInternalPreview)
	{
		if (m_pImageUserDialog->PreviewGenerationCanceled())
			return 0;
	}

	const uint32 calculatedMipCount = _CalcMipCount(dwWidth,dwHeight,bFinalPixelFormatIsCompressed,bCubemap);

	//for skipping of unnecessary big MIPs
	if (m_bInternalPreview)
	{
		indwReduceResolution += m_pImageUserDialog->GetPreviewReduceResolution();
	}

	const uint32 dwNumNewMips =
		(inbRemoveMips) 
		? 1 
		: (uint32)max((int)calculatedMipCount-(int)indwReduceResolution,1);

	const uint32 dwW = max(dwWidth>>indwReduceResolution,(uint32)dwMinSize);
	const uint32 dwH = max(dwHeight>>indwReduceResolution,(uint32)dwMinSize);

	ImageObject *pRet = new CRAWImage(dwW,dwH,dwNumNewMips,CPixelFormats::GetFormatForMips(pInputImage->GetPixelFormat()),bFinalPixelFormatIsCompressed);

	pRet->CopyPropertiesFrom(*pInputImage);

	CSimpleBitmap<Vec4> Bmp[2];			// double buffering
//	CSimpleBitmap<Vec4> Bmp[3];			// triple buffering to get sharp but correct (always the upper left top area is used)

	// Sometimes this allocation fails, so if we are using multiple threads we may fail. In this case the calling code
	// may try again later.
	pInputImage->CopyToSimpleBitmap(Bmp[0]);

	//early out: check for canceling preview generation
	if (m_bInternalPreview)
	{
		if (m_pImageUserDialog->PreviewGenerationCanceled())
		{
			delete pRet;
			return 0;
		}
	}

	m_Progress.Phase1();

	if(Bmp[0].CalcBitmapSize()==0)
	{
		RCLogError("CImageCompiler::CreateMipMaps failed, input image cannot be processed (input has to be a TIF file)");
		return 0;
	}

	RECT previewRect;
	if (m_bInternalPreview)
	{
		previewRect = m_pImageUserDialog->GetPreviewRectangle();
	}

	if(m_Props.GetColorModel()==1)
	{
		Vec4 *pMem = Bmp[0].GetPointer();
		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		{
			//early out: check for canceling preview generation
			if (m_bInternalPreview)
			{
				if (m_pImageUserDialog->PreviewGenerationCanceled())
				{
					delete pRet;
					return 0;
				}
			}

			for(uint32 dwX=0;dwX<dwWidth;++dwX)
			{
				//skip everything outside the preview rectangle view
				if (m_bInternalPreview)
				{
					if (IsPointOutsideOfRectangle(dwX, dwY, previewRect))
					{
						*pMem++;
						continue;
					}
				}

				Vec4 &ref = *pMem++;

//			ref.z += ((rand()%1000)*0.001f-0.5f)/64.0f; 
//			ref.y += ((rand()%1000)*0.001f-0.5f)/64.0f; 
//			ref.z += ((rand()%1000)*0.001f-0.5f)/64.0f; 

				ColorF col = ColorF(ref.x,ref.y,ref.z).RGB2mCIE();

//			col.g=0.5f;
//			ColorF testback = ColorF((int)(col.r*255.0f)/255.0f,(int)(col.g*255.0f)/255.0f,(int)(col.b*255.0f)/255.0f).mCIE2RGB();
				ref.x = col.r;
				ref.y = col.g;
				ref.z = col.b;
			}
		}
	}

	// gamma->linear - could be optimized by table lookup
	if(fGamma!=1.0f)
	{
		Vec4 *pMem = Bmp[0].GetPointer();
		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		{
			//early out: check for canceling preview generation
			if (m_bInternalPreview)
			{
				if (m_pImageUserDialog->PreviewGenerationCanceled())
				{
					delete pRet;
					return 0;
				}
			}

			for(uint32 dwX=0;dwX<dwWidth;++dwX)
			{
				//skip everything outside the preview rectangle view
				if (m_bInternalPreview)
				{
					if (IsPointOutsideOfRectangle(dwX, dwY, previewRect))
					{
						Vec4 &ref = *pMem++;
						ref.x=0;
						ref.y=0;
						ref.z=0;
						continue;
					}
				}

				Vec4 &ref = *pMem++;

				ref.x=powf(ref.x,fGamma);
				ref.y=powf(ref.y,fGamma);
				ref.z=powf(ref.z,fGamma);
			}

			m_Progress.Phase2(dwY, dwHeight);
		}
	}

	if( -(int)indwReduceResolution  < ((int)dwNumNewMips)-1 )
	{
		Bmp[1].Alloc( max(dwWidth/2,dwMinSize), max(dwHeight/2,dwMinSize) );
	}

//	if(dwWidth/4>0 && dwHeight/4>0)
//		Bmp[2].Alloc(dwWidth/4,dwHeight/4);

	const bool use2x2 = (m_Props.GetMipGenType() == 1);

	CWeightFilterSet* pFilterset;

	if(use2x2)
	{
		pFilterset = &m_mipFilterset2x2;

		if(!m_mipFilterset2x2.IsValid())
		{
			CSummedAreaFilterKernel kernel;
	
			kernel.CreateFromGauss(256,1.0f,0.0f);

			pFilterset->Create(kernel,1.0f,false);

			assert(pFilterset->IsValid());
		}
	}
	else
	{
		pFilterset = &m_mipFilterset;

		const float requestedFiltersetSharpness = (float)(m_Props.GetMipSharpen()*0.01f);		// to detect if we need to compute the filterset

		// create downsampling filter
		if(m_fCurrentMipFilterSharpness != requestedFiltersetSharpness)					// to avoid doing this for every call
		{
			m_fCurrentMipFilterSharpness = requestedFiltersetSharpness;

			CSummedAreaFilterKernel kernel;
	
			//		kernel.CreateFromGauss(256,0.5f,0.23f);		// with sharpening
			//		kernel.CreateFromGauss();									// without sharpening
			const float fSqrtCurrentFilter=powf(m_fCurrentMipFilterSharpness,1.0/8.0);		// to make slider more or less linear
			kernel.CreateFromGauss(256,1.0f-0.5f*fSqrtCurrentFilter,0.23f*fSqrtCurrentFilter);		// with defined sharpening

//		assert(kernel.GetHeight()==256 && kernel.GetWeight()==256);			// some code rely on that

			float fKernelRadius=2.0f;
			m_pCC->config->Get("globalmipblur",fKernelRadius);		// <=2.0f -> 4x4 kernel

			pFilterset->Create(kernel,1.0f*fKernelRadius,false);		// kernel size can/should be adjusted (smallest: 4->1, so bCenter is false)
		}
	}

	uint32 colAverageARGB=0xffffffff;
	bool bAverageColorSet=false;

	int iIteration=0;
	for(int iNewMip=-(int)indwReduceResolution; iNewMip<(int)dwNumNewMips; ++iNewMip,++iIteration)
	{
		CSimpleBitmap<Vec4> &rCurrentBmp =  Bmp[iIteration%2];

		if(bMaintainAlphaCoverage)
			MaintainAlphaCoverage(rCurrentBmp,dwWidth,dwHeight);

		const float fAlphaOffset = m_Props.ComputeMIPAlphaOffset(iNewMip+indwReduceResolution,dwNumNewMips+indwReduceResolution);

//		if(m_Props.m_iMipSharpen)
//			Sharpen(rCurrentBmp,dwWidth,dwHeight,(float)(m_Props.m_iMipSharpen*0.01f));

		if(iNewMip>=0)
		{
			char *pDestMem;
			uint32 dwDestPitch;

			size_t pixelSize = CPixelFormats::GetPixelFormatInfo(pRet->GetPixelFormat())->iBitsPerPixel / 8;

			if(pRet->Lock(0,(uint32)iNewMip,pDestMem,dwDestPitch))
			{

				if (m_bInternalPreview)
				{
					previewRect = m_pImageUserDialog->GetPreviewRectangle(iNewMip+indwReduceResolution);
				}

				for(uint32 dwY=0;dwY<dwHeight;++dwY)
				{
					//early out: check for canceling preview generation
					if (m_bInternalPreview)
					{
						if (m_pImageUserDialog->PreviewGenerationCanceled())
						{
							pRet->Unlock((uint32)iNewMip);
							delete pRet;
							return 0;
						}
					}

					char *pDestLine = pDestMem+dwDestPitch*dwY;

					for(uint32 dwX=0;dwX<dwWidth;++dwX)
					{
						Vec4 vValue(0,0,0,0);

						//skip everything outside the preview rectangle view
						if (m_bInternalPreview)
						{
							if (IsPointOutsideOfRectangle(dwX, dwY, previewRect))
							{
								if(pixelSize == 4)
									(uint32&)*pDestLine = VEC4ToUINT32(vValue);
								else if(pixelSize == 16)
									(Vec4&)*pDestLine = vValue;
								else assert(0);

								pDestLine+=pixelSize;
								continue;
							}
						}

						bool bOk = rCurrentBmp.Get(dwX,dwY,vValue);			assert(bOk);

						// linear->gamma
						if(fGamma!=1.0f)
						{
							vValue.x=powf(vValue.x,fInvGamma);
							vValue.y=powf(vValue.y,fInvGamma);
							vValue.z=powf(vValue.z,fInvGamma);
						}

						if(bRenormalize)
						{
							Vec3 vNormal(vValue.x*2.0f-1.0f,vValue.y*2.0f-1.0f,vValue.z*2.0f-1.0f);

							vNormal.NormalizeSafe();

							vValue.x=vNormal.x*0.5f+0.5f;
							vValue.y=vNormal.y*0.5f+0.5f;
							vValue.z=vNormal.z*0.5f+0.5f;
						}
	
						vValue.w+=fAlphaOffset;

						if(pixelSize == 4)
						{
							vValue.x = clamp_tpl(vValue.x, 0.f, 1.f);
							vValue.y = clamp_tpl(vValue.y, 0.f, 1.f);
							vValue.z = clamp_tpl(vValue.z, 0.f, 1.f);
							vValue.w = clamp_tpl(vValue.w, 0.f, 1.f);
							(uint32&)*pDestLine = VEC4ToUINT32(vValue);
						}
						else if(pixelSize == 16)
							(Vec4&)*pDestLine = vValue;
						else assert(0);

						pDestLine+=pixelSize;
					}
				}
				
				if(dwHeight==1 && dwWidth==1)
				{
					colAverageARGB=*((uint32 *)pDestMem);
//					RCLog("CImageCompiler::CreateMipMaps AvgColor(ARGB) = %x",colAverageARGB);
					bAverageColorSet=true;
				}

				pRet->Unlock((uint32)iNewMip);
			}
			else
			{
				RCLogError("CImageCompiler::CreateMipMaps Lock failed");
				delete pRet;
				return 0;
			}
		}

		uint64 BorderColor=m_Props.GetBorderColor();
		if(BorderColor!=-1)
		{
			// all mip borders during the mipmap generation become this hex value AARRGGBB
			Vec4 col = UINT32ToVEC4((uint32)BorderColor);

			for(uint32 dwY=0;dwY<dwHeight;++dwY)
			{
				rCurrentBmp.Set(0,dwY,col);						// left border
				rCurrentBmp.Set(dwWidth-1,dwY,col);		// right border
			}
			for(uint32 dwX=1;dwX<dwWidth-1;++dwX)
			{
				rCurrentBmp.Set(dwX,0,col);						// top border
				rCurrentBmp.Set(dwX,dwHeight-1,col);	// bottom border
			}
		}

		uint32 dwSrcWidth = dwWidth;
		uint32 dwSrcHeight = dwHeight;

		dwWidth=max(dwWidth>>1, dwMinSize);
		dwHeight=max(dwHeight>>1, dwMinSize);

		// calculate downsampled version
		if(iNewMip<((int)dwNumNewMips-1))
		{
			// lookup one mip up
			CSimpleBitmap<Vec4> &rInBmp =  Bmp[iIteration%2];
			CSimpleBitmap<Vec4> &rOutBmp =  Bmp[(iIteration+1)%2];

			if (m_bInternalPreview)
			{
				previewRect = m_pImageUserDialog->GetPreviewRectangle(iNewMip+indwReduceResolution+1);
			}

			for(uint32 dwY=0;dwY<dwHeight;++dwY)
			{
				//early out: check for canceling preview generation
				if (m_bInternalPreview)
				{
					if (m_pImageUserDialog->PreviewGenerationCanceled())
					{
						delete pRet;
						return 0;
					}
				}

				for(uint32 dwX=0;dwX<dwWidth;++dwX)
				{
					Vec4 vResult(0,0,0,0);

					//skip everything outside the preview rectangle view
					if (m_bInternalPreview)
					{
						if (IsPointOutsideOfRectangle(dwX, dwY, previewRect))
						{
							rOutBmp.Set(dwX,dwY,vResult);
							continue;
						}
					}

					pFilterset->GetBlockWithFilter(rInBmp,dwX*2+1,dwY*2+1,vResult,bCubemap,dwSrcWidth,dwSrcHeight);

					// needed when using sharpening filter in mip map generation
					vResult.x = max(vResult.x,0.0f);
					vResult.y = max(vResult.y,0.0f);
					vResult.z = max(vResult.z,0.0f);
					vResult.w = max(vResult.w,0.0f);

					rOutBmp.Set(dwX,dwY,vResult);
				}

				m_Progress.Phase3(dwY, dwHeight, iNewMip+indwReduceResolution);
			}
		}
/*		else
		{
			// lookup two mips up
			CSimpleBitmap<Vec4> &rInBmp =  Bmp[(iIteration-1)%3];
			CSimpleBitmap<Vec4> &rOutBmp =  Bmp[(iIteration+1)%3];

			for(uint32 dwY=0;dwY<dwHeight;++dwY)
				for(uint32 dwX=0;dwX<dwWidth;++dwX)
				{
					Vec4 vResult(0,0,0,0);

					filterset_2.GetBlockWithFilter(rInBmp,dwX*4+2,dwY*4+2,vResult);

					// needed when using sharpening filter in mip map generation
					vResult.x = clamp_tpl(vResult.x,0.0f,1.0f);
					vResult.y = clamp_tpl(vResult.y,0.0f,1.0f);
					vResult.z = clamp_tpl(vResult.z,0.0f,1.0f);
					vResult.w = clamp_tpl(vResult.w,0.0f,1.0f);

					rOutBmp.Set(dwX,dwY,vResult);
				}
		}
*/
	}

	if(bAverageColorSet)
		pRet->SetAverageColor(colAverageARGB);

	return pRet;
}


ImageObject * CImageCompiler::CreateCubemapMipMaps( ImageObject *pInputImage, const SCubemapFilterParams& params, uint32 indwReduceResolution, const float fGamma )
{
	CryAutoLockRC lock(g_AtiCubemapLock);

	//Clear cubemap processor for filtering cubemap
	m_AtiCubemanGen.Clear();
	m_AtiCubemanGen.m_NumFilterThreads = 2;

	int inbRemoveMips = 0;

	// Create destination cubemap
	const SPixelFormats *fmt = CPixelFormats::GetPixelFormatInfo(pInputImage->GetPixelFormat());
	if(fmt->DxtNo != D3DFMT_A32B32G32R32F && fmt->DxtNo != D3DFMT_A8R8G8B8)	// now supported only ARGB32f and ARGB8 format
	{
		assert(0);
		return NULL;
	}
	uint32 dwWidth = pInputImage->GetWidth(0);
	uint32 dwHeight = pInputImage->GetHeight(0);
	uint32 dwW,dwH;
	dwW=dwWidth;
	dwH=dwHeight;
	int nRes = indwReduceResolution;
	while(nRes)
	{
		if(dwW/6 <= 4 || dwH <= 4)
			break;
		dwW>>=1;
		dwH>>=1;
		nRes--;
	}
	indwReduceResolution -= nRes;
	uint32 dwNumNewMips = (uint32)max((int)_CalcMipCount(dwW,dwH,IsFinalPixelFormatCompressed(),true)-(int)indwReduceResolution,1);
	if(inbRemoveMips)
		dwNumNewMips=1;
	ImageObject *pRet = new CRAWImage(dwW,dwH,dwNumNewMips,CPixelFormats::GetFormatForMips(pInputImage->GetPixelFormat()),false);

	const bool isHDR = CPixelFormats::GetPixelFormatInfo(pRet->GetPixelFormat())->DxtNo == D3DFMT_A32B32G32R32F;

	//input and output cubemap set to have save dimensions, 
	m_AtiCubemanGen.Init(dwHeight, dwH, dwNumNewMips, 4);

	uint32 nPitch;
	char* pMem;

	//Load the 6 faces of the input cubemap and copy them into the cubemap processor
	if(pInputImage->Lock(0, 0, pMem, nPitch))
	{
		const uint32 nSidePitch = nPitch/6;
		assert(nPitch%6==0);
		for(int iSide=0; iSide<6; iSide++)
		{
			m_AtiCubemanGen.SetInputFaceData(
				iSide,            														// FaceIdx, 
				isHDR ? CP_VAL_FLOAT32 : CP_VAL_UNORM8,				// SrcType, 
				4,                														// SrcNumChannels, 
				nPitch,																				// SrcPitch,
				pMem + nSidePitch * iSide,  									// SrcDataPtr, 
				1000000.0f,       														// MaxClamp, 
				1.0f / fGamma,    														// Degamma, 
				1.0f              														// Scale 
				);
		}
		pInputImage->Unlock(0);
	}
	else assert(0);

	//Filter cubemap
	m_AtiCubemanGen.InitiateFiltering(
		params.BaseFilterAngle,         //BaseFilterAngle, 
		params.InitialMipAngle,         //InitialMipAngle, 
		params.MipAnglePerLevelScale,   //MipAnglePerLevelScale, 
		params.FilterType,              //FilterType, 
		params.FixupType,               //FixupType, 
		params.FixupWidth,              //FixupWidth, 
		params.bUseSolidAngle           //bUseSolidAngle         
		);

	//Report status of filtering , and loop until filtering is complete
	while( m_AtiCubemanGen.GetStatus() == CP_STATUS_PROCESSING)
	{
		if(m_Props.GetUserDialog())	// allow user to abort the operation
		{
			if(::GetKeyState(VK_ESCAPE) & 0x80)
				m_AtiCubemanGen.TerminateActiveThreads();
		}
		//wprintf(L"%s            ", cubeMapProcessor.GetFilterProgressString() );
		Sleep(100);
	}

	// Download data into it
	assert(m_AtiCubemanGen.m_NumMipLevels > 0);
	for(int iMip=0; iMip<m_AtiCubemanGen.m_NumMipLevels; iMip++)
	{
		uint32 dwMipWidth = pRet->GetWidth(iMip);
		uint32 dwMipHeight = pRet->GetHeight(iMip);
		if(pRet->Lock(0, iMip, pMem, nPitch))
		{
			const uint32 nSidePitch = nPitch/6;
			for(int iSide=0; iSide<6; iSide++)
			{
				char* pDestMem = pMem + iSide*nSidePitch;
				m_AtiCubemanGen.GetOutputFaceData(iSide, iMip, isHDR ? CP_VAL_FLOAT32 : CP_VAL_UNORM8, 
																					4, nPitch, pDestMem, 1.f, 1.f );   
			}
			pRet->Unlock(iMip);
		}
	}

	return pRet;
}

void CImageCompiler::PreSwizzleTexturePS3like_InPlace( ImageObject *pInAndOutImage ) 
{
	assert(pInAndOutImage);

	// apply to attached images recursively
	if(pInAndOutImage->GetAttachedImage())
		PreSwizzleTexturePS3like_InPlace(pInAndOutImage->GetAttachedImage());

	TPixelFormatID srcPixelformat=pInAndOutImage->GetPixelFormat();

	if(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->bCompressed)
		return;		// PS3 does not require swizzling on compressed textures

	if(!pInAndOutImage->IsPowerOfTwo())
		return;		// PS3 only swizzles power of two textures

	int iBitsPerPixel = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->iBitsPerPixel;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pInAndOutImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	std::vector<unsigned char> Buffer;

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = pInAndOutImage->GetWidth(dwMip);
		uint32 dwLocalHeight = pInAndOutImage->GetHeight(dwMip);

		char *pMem=0;
		uint32 dwPitch=0;

		if(pInAndOutImage->Lock(0,dwMip,pMem,dwPitch))
		{
			if(Buffer.empty())
				Buffer.resize(dwLocalHeight*dwPitch);

			memcpy(&Buffer[0],pMem,dwLocalHeight*dwPitch);

			Linear2Swizzle((uint8 *)pMem,&Buffer[0],dwPitch,dwLocalWidth,dwLocalHeight,iBitsPerPixel/8,0,0,dwLocalWidth,dwLocalHeight);

			pInAndOutImage->Unlock(dwMip);
		}
		else assert(0);
	}
}




void CImageCompiler::Convert3DcToDXT5_InPlace( ImageObject *pInAndOutImage ) 
{
	assert(pInAndOutImage);

	// apply to attached images recursively
	if(pInAndOutImage->GetAttachedImage())
		Convert3DcToDXT5_InPlace(pInAndOutImage->GetAttachedImage());

	TPixelFormatID srcPixelformat = pInAndOutImage->GetPixelFormat();

	if(srcPixelformat!=CPixelFormats::GetNoFromD3DFormat(D3DFMT_3DC))
		return;		// only applies the conversion if the image is 3Dc

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pInAndOutImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	const uint32 dwBlockSize=16;			// DXT5 and 3Dc have the same size

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = pInAndOutImage->GetWidth(dwMip);
		uint32 dwLocalHeight = pInAndOutImage->GetHeight(dwMip);

		uint32 dwLines = (dwLocalHeight+3)/4;

		char *pMem=0;
		uint32 dwPitch=0;

		if(pInAndOutImage->Lock(0,dwMip,pMem,dwPitch))
		{
			for(uint32 dwY=0;dwY<dwLines;++dwY)
			{
				uint32 dwBlocks = (dwLocalWidth+3)/4;

				uint8 *pBlockLine = (uint8 *)&pMem[dwPitch*dwY];

				for(uint32 dwX=0;dwX<dwBlocks;++dwX)
				{
					uint8 temp[dwBlockSize];

					uint8 *pBlock = pBlockLine + dwX*dwBlockSize;

					ConvertBlock3DcToDXT5(temp,pBlock);
					memcpy(pBlock,temp,dwBlockSize);
				}
			}

			pInAndOutImage->Unlock(dwMip);
		}
		else assert(0);
	}

	pInAndOutImage->SetPixelFormat(CPixelFormats::GetNoFromD3DFormat(D3DFMT_DXT5));
}



void CImageCompiler::MaintainAlphaCoverage( CSimpleBitmap<Vec4> &inoutBitmap, const uint32 dwWidth, const uint32 dwHeight )
{
	float fMaintainAlphaCoverage_before = ComputeAlphaCoverage(inoutBitmap,dwWidth,dwHeight,0.5f);

	float fSharpenFactor = 0.170f;	// from experiments - this is a good value
//	for(float fSharpenFactor=0.13f;fSharpenFactor<0.2f;fSharpenFactor+=0.005f)		// test
//	{
//		CSimpleBitmap<Vec4> cpy=inoutBitmap;			// test

	// create downsampling filter
		// remark next line for the test
	if(!m_bValidAlphaCoverageFilterset)									// do avoid doing this for every call
	{
		CSummedAreaFilterKernel kernel;
	
//		kernel.CreateFromGauss(256,0.5f,0.23f);		// with sharpening
//		kernel.CreateFromGauss(256,0.5f,0.19f);		// with less sharpening
		kernel.CreateFromGauss(256,0.5f,fSharpenFactor);
//		kernel.CreateFromGauss();									// without sharpening

		m_alphaCoverageFilterset.Create(kernel,1.0f,true);		// kernel size can/should be adjusted (smallest: 4->1, so bCenter is false)

//		m_alphaCoverageFilterset.Debug("MaintainAlphaCoverage");

		m_bValidAlphaCoverageFilterset=true;
	}

	bool bCubemap = m_Props.GetCubemap()/* && pInputImage->IsCubemap()*/;

	CSimpleBitmap<float> NewAlpha;

	NewAlpha.Alloc(dwWidth,dwHeight);
	
	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		Vec4 vResult(0,0,0,0);

		m_alphaCoverageFilterset.GetBlockWithFilter(inoutBitmap,dwX,dwY,vResult,bCubemap);

		NewAlpha.Set(dwX,dwY,vResult.w);
	}

	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		const uint32 dwSteps=4;											// Sample count is dwSteps*dwSteps
		const float fStep = 1.0f/(float)dwSteps;
		const float fHalfStep = 0.5f*fStep;

		float fSumValue=0.0f;

		for(float fY=-0.5f+fHalfStep;fY<0.5f;fY+=fStep)
		for(float fX=-0.5f+fHalfStep;fX<0.5f;fX+=fStep)
		{
			float fValue=0.0f;

			NewAlpha.GetFiltered((float)dwX+fX,(float)dwY+fY,fValue);

			if(fValue>0.5f)
				fSumValue += 1.0f/(float)(dwSteps*dwSteps);
		}

		Vec4 vResult;

		inoutBitmap.Get(dwX,dwY,vResult);

		vResult.w=fSumValue;

		inoutBitmap.Set(dwX,dwY,vResult);
	}
	

	{
		char str[256];

		sprintf(str,"AlphaCoverage %dx%d fSharpenFactor=%.4f   %.4f -> %.4f\n",dwWidth,dwHeight,
			fSharpenFactor,fMaintainAlphaCoverage_before,ComputeAlphaCoverage(inoutBitmap,dwWidth,dwHeight,0.5f));
		OutputDebugString(str);
	}

//	inoutBitmap = cpy;			// test
//	}			// test

	OutputDebugString("\n");
}

/*
void CImageCompiler::MaintainAlphaCoverage( CSimpleBitmap<Vec4> &inoutBitmap, float &fReferenceAlphaCoverage )
{
	if(fReferenceAlphaCoverage==-1)
	{
//		fReferenceAlphaCoverage = ComputeAlphaCoverage(inoutBitmap,0.5f);		return;
		fReferenceAlphaCoverage = ComputeWeightedAlphaCoverage(inoutBitmap);
	}

	float fMin=0.0f, fMax=1.0f;

	float fCurrentAlphaCoverage=-1;

	for(uint32 dwIter=0;dwIter<8;++dwIter)
	{
		float fMid = 0.5f*(fMin+fMax);
		float fTestAlphaCoverage = ComputeAlphaCoverage(inoutBitmap,fMid);

		if(fCurrentAlphaCoverage==-1)
			fCurrentAlphaCoverage=fTestAlphaCoverage;

		if(fTestAlphaCoverage<fReferenceAlphaCoverage)
			fMax=fMid;
		 else
			fMin=fMid;
	}

	uint32 dwWidth=inoutBitmap.GetWidth(), dwHeight=inoutBitmap.GetHeight();

//	float fMul = fCurrentAlphaCoverage (0.5f / (0.5f*(fMin+fMax)));
	float fMul = 1.0f / (fMin+fMax);

	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		Vec4 &Value = inoutBitmap.GetRef(dwX,dwY);
		
		Value.w = min(1.0f,Value.w*fMul);
	}

	// test
	float fDebug = ComputeAlphaCoverage(inoutBitmap,0.5f);
}
*/

float CImageCompiler::ComputeAlphaCoverage( const CSimpleBitmap<Vec4> &inBitmap, const uint32 dwWidth, const uint32 dwHeight, const float fRefAlpha )
{
	uint32 dwTrueCount=0;

	float fMul = 0.5f/fRefAlpha;

	const uint32 dwSteps=4;											// Sample count is dwSteps*dwSteps

	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		const float fStep = 1.0f/(float)dwSteps;
		const float fHalfStep = 0.5f*fStep;

//		float fSumValue=0.0f;

		for(float fY=-0.5f+fHalfStep;fY<0.5f;fY+=fStep)
		for(float fX=-0.5f+fHalfStep;fX<0.5f;fX+=fStep)
		{
			Vec4 Value;
			
			inBitmap.GetFiltered(dwX+fX,dwY+fY,Value);

//			fSumValue+=Value.w;
			if(Value.w>fRefAlpha)
				++dwTrueCount;
		}

//		if(min(fSumValue*fMul,1.0f)>0.5f)
//			dwTrueCount++;
	}

	return (float)dwTrueCount/(dwSteps*dwSteps)/(float)(dwWidth*dwHeight);
}





uint32 CImageCompiler::_CalcMipCount( const uint32 indwWidth, const uint32 indwHeight, const bool bDXT, const bool bCubemap )
{
	const uint32 dwMinSize = bDXT ? 4 : 1;			// compressed textures need special treatment for textures smaller than 4x4

	LONG lwTempH,lwTempW,lwPowsW,lwPowsH;

	lwTempW = indwWidth;
	lwTempH = indwHeight;

	if(bCubemap)
	{
		assert(lwTempW/6==indwHeight);
		lwTempW/=6;
	}

	if(lwTempW <= dwMinSize && lwTempH <= dwMinSize)
		return 1;

	lwPowsW = 0;
	lwPowsH = 0;
	while (lwTempW >= dwMinSize)
	{
		lwPowsW++;
		lwTempW = lwTempW / 2;
	}
	while (lwTempH >= dwMinSize)
	{
		lwPowsH++;
		lwTempH = lwTempH / 2;
	}
	if(!bDXT)
		return max(lwPowsW, lwPowsH);
	else
		return min(lwPowsW, lwPowsH);
}



// needed for the NVidia library
void WriteDTXnFile( DWORD count, void * buffer, void * userData )
{
	assert(0);
}

static CryCriticalSectionRC g_DDSMemoryFileLock;
static char *g_DDSMemoryFilePtr=0;			// needed because NVidia lib ReadDTXnFile userData cannot be passed

// needed for the NVidia library
void ReadDTXnFile( DWORD count, void * buffer, void * userData )
{
	memcpy(buffer,g_DDSMemoryFilePtr,count);

	g_DDSMemoryFilePtr+=count;
}


struct SLoadAllMipSurfaces
{
	ImageObject *	m_pThis;
	uint32				m_dwMip;
	RECT					m_rectPreview;	

	// for the NVidia Squish (CryCompressor):
	char* m_dstMem;
	uint32 m_dstOffset;
};


// callback for the NVidia library
HRESULT LoadAllMipSurfaces( void * data, int iLevel, DWORD size, int Width, int Height, void * user )
{
	SLoadAllMipSurfaces *pUser = (SLoadAllMipSurfaces *)user;

	char *pDstMem;
	uint32 dwPitch;

	pUser->m_pThis->Lock(0,pUser->m_dwMip,pDstMem,dwPitch);

	const bool bBlockCompressed = CPixelFormats::GetPixelFormatInfo( pUser->m_pThis->GetPixelFormat() )->bCompressed;


	RECT recBlock = pUser->m_rectPreview;

	uint32 dwBlockSizeInBits = CPixelFormats::GetPixelFormatInfo( pUser->m_pThis->GetPixelFormat() )->iBitsPerPixel;

	if(bBlockCompressed)
	{
		recBlock.left/=4;
		recBlock.top/=4;
		recBlock.right/=4;
		recBlock.bottom/=4;
		dwBlockSizeInBits*=16;
	}

	const uint32 dwBlockSize = dwBlockSizeInBits/8;		// in bytes
	char *pSrcMem = (char *)data;

	for(int iY=recBlock.top;iY<recBlock.bottom;++iY)
	for(int iX=recBlock.left;iX<recBlock.right;++iX)
	{
		memcpy(&pDstMem[iY*dwPitch+iX*dwBlockSize],pSrcMem,dwBlockSize);
		pSrcMem+=dwBlockSize;
	}

	pUser->m_pThis->Unlock(pUser->m_dwMip);

	return 0;	// OK
}

// callback for the NVidia library
HRESULT LoadAllMipSurfacesOneBlock( void * data, int iLevel, DWORD size, int Width, int Height, void * user )
{
	SLoadAllMipSurfaces *pUser = (SLoadAllMipSurfaces *)user;

	assert((size == 16) || (size == 8));
	assert(pUser->m_dstOffset == 0);
	assert(Width==4);
	assert(Height==4);
	assert(iLevel==0);

	memcpy( ((char*)pUser->m_dstMem)+pUser->m_dstOffset, data, size );

	pUser->m_dstOffset += size;

	return 0;	// OK
}


// callback for the CryTextureCompressor
void CryCompressorOutputCallback(const CryTextureCompressor::CompressorParameters& compress, const void* data, uint size)
{
	SLoadAllMipSurfaces *pUser = (SLoadAllMipSurfaces *)compress.userPtr;

	assert( CPixelFormats::GetPixelFormatInfo( pUser->m_pThis->GetPixelFormat() )->bCompressed );

	memcpy( ((char*)pUser->m_dstMem)+pUser->m_dstOffset, data, size );

	pUser->m_dstOffset += size;
}


// callback for the NVidia library's CryCompressor
void CompressorOutputCallback(const nv::CryCompressor::CryCompressorData& compress, const void* data, uint size)
{
	SLoadAllMipSurfaces *pUser = (SLoadAllMipSurfaces *)compress.userPtr;

	assert( CPixelFormats::GetPixelFormatInfo( pUser->m_pThis->GetPixelFormat() )->bCompressed );

	memcpy( ((char*)pUser->m_dstMem)+pUser->m_dstOffset, data, size );

	pUser->m_dstOffset += size;
}


// callback for the NVidia library's CryCompressor
void CompressorOutputCallbackOneBlock(const nv::CryCompressor::CryCompressorData& compress, const void* data, uint size)
{
	SLoadAllMipSurfaces *pUser = (SLoadAllMipSurfaces *)compress.userPtr;

	assert((size == 16) || (size == 8));
	assert(pUser->m_dstOffset == 0);

	memcpy( ((char*)pUser->m_dstMem)+pUser->m_dstOffset, data, size );

	pUser->m_dstOffset += size;
}


static TextureFormats FindCorespondingNVidiaFormat( D3DFORMAT fmtTo )
{
	switch(fmtTo)
	{
//		case D3DFMT_DXT1: return kDXT1a;	// DXT1 with one bit alpha
		case D3DFMT_DXT1: return kDXT1;	// DXT1 with one bit alpha
    case D3DFMT_DXT3: return kDXT3;   // explicit alpha
    case D3DFMT_DXT5: return kDXT5;   // interpolated alpha
    case D3DFMT_A4R4G4B4: return k4444;   // a4 r4 g4 b4
    case D3DFMT_A1R5G5B5: return k1555;   // a1 r5 g5 b5
    case D3DFMT_R5G6B5: return k565;    // a0 r5 g6 b5
    case D3DFMT_A8R8G8B8: return k8888;   // a8 r8 g8 b8
    case D3DFMT_X8R8G8B8: return k8888;   // a8 r8 g8 b8
    case D3DFMT_R8G8B8: return k888;    // a0 r8 g8 b8
    case D3DFMT_X1R5G5B5: return k555;    // a0 r5 g5 b5
    case D3DFMT_P8: return k8;   // paletted
    case D3DFMT_V8U8: return kV8U8;   // DuDv 
//    case D3DFMT_: return kCxV8U8;   // normal map
    case D3DFMT_A8: return kA8;            // alpha only
//    case D3DFMT_: return k4;            // 16 bit color      
//    case D3DFMT_: return kQ8W8V8U8;
    case D3DFMT_A8L8: return kA8L8;
    case D3DFMT_R32F: return kR32F;
    case D3DFMT_A32B32G32R32F: return kA32B32G32R32F;
    case D3DFMT_A16B16G16R16F: return kA16B16G16R16F;
    case D3DFMT_L8: return kL8;       // luminance
	}

	return kTextureFormatLast;			// output pixel format not supported yet
}


ImageObject *CImageCompiler::ConvertFormat( ImageObject *inSrc, D3DFORMAT fmtTo )
{
	assert(inSrc);

	CImageProperties::EGlobalCompressor compressor = m_Props.GetGlobalCompressor();

	ImageObject *pRet = ConvertFormatWithSpecifiedCompressor(inSrc,fmtTo,compressor);

	if(!pRet)
		return pRet;

	if(pRet->GetPixelFormat()==CPixelFormats::GetNoFromD3DFormat(D3DFMT_L8))
		pRet->SetImageFlags( pRet->GetImageFlags() | CImageExtensionHelper::EIF_Greyscale );

	return pRet;
}


CRAWImage *CImageCompiler::CopyToNewCRAWImage( ImageObject *inSrc )
{
	assert(inSrc);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	inSrc->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwSides!=1)
		return 0;				// currently we don't support cubemaps

	if(dwDepth!=1)
		return 0;				// currently we don't support volume textures
	
	uint32 dwInPlanes=0;

	TPixelFormatID srcPixelformat=inSrc->GetPixelFormat();

	D3DFORMAT d3dSrcFormat = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;

	CRAWImage *pRet = new CRAWImage(dwWidth,dwHeight,dwMips,CPixelFormats::GetNoFromD3DFormat(d3dSrcFormat),IsFinalPixelFormatCompressed());

	pRet->CopyPropertiesFrom(*inSrc);

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = inSrc->GetWidth(dwMip);			// we get error on NVidia with this (assumes input is 4x4 as well)
		uint32 dwLocalHeight = inSrc->GetHeight(dwMip);

		uint32 dwLines = dwLocalHeight;

		if(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->bCompressed)
			dwLines = (dwLocalHeight+3)/4;

		char *pMem=0;
		uint32 dwPitch=0;

		if(inSrc->Lock(0,dwMip,pMem,dwPitch))
		{
			char *pDstMem;
			uint32 dwDstPitch;

			if(pRet->Lock(0,dwMip,pDstMem,dwDstPitch))
			{
				for(uint32 dwY=0;dwY<dwLines;++dwY)
					memcpy(&pDstMem[dwDstPitch*dwY],&pMem[dwPitch*dwY],min(dwPitch,dwDstPitch));

				pRet->Unlock(dwMip);
			}
			else assert(0);

			inSrc->Unlock(dwMip);
		}
		else assert(0);
	}
	
	// recursive for all attached images
	if(ImageObject *pAttached = inSrc->GetAttachedImage())
	{
		CRAWImage *pAttachedRAW = CopyToNewCRAWImage(pAttached);

		pRet->SetAttachedImage(pAttachedRAW);
	}

	return pRet;
}


static bool IsFormatSimple(D3DFORMAT d3dFormat)
{
	switch(d3dFormat)
	{
	case D3DFMT_R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8R8G8B8:
	case D3DFMT_A32B32G32R32F:
		return true;

	default:
		return false;
	}
}

static bool GetSimpleFormatInfo(D3DFORMAT d3dFormat, bool& isByte, int& channelCount, bool& hasAlpha)
{
	switch(d3dFormat)
	{
	case D3DFMT_R8G8B8:
		isByte = true;
		channelCount = 3;
		hasAlpha = false;
		break;
	case D3DFMT_X8R8G8B8:
		isByte = true;
		channelCount = 4;
		hasAlpha = false;
		break;
	case D3DFMT_A8R8G8B8:
		isByte = true;
		channelCount = 4;
		hasAlpha = true;
		break;

	case D3DFMT_A32B32G32R32F:
		isByte = false;
		channelCount = 4;
		hasAlpha = true;
		break;

	default:
		return false;
	}

	return true;
}


static void ConvertBetweenFloatAndUbyte(const bool srcIsByte, int pixelCount, const int channelCount, const char* const pSrc, char* &pDst)
{
	assert(pSrc);
	assert(pixelCount>0);
	assert((channelCount==3)||(channelCount==4));

	if(srcIsByte)
	{
		// convert byte to float

		const unsigned char* pSrcMem = (const unsigned char*) &pSrc[0];

		if(pDst == 0)
		{
			pDst = new char[pixelCount * channelCount * sizeof(float)];
		}

		float* pDstMem = (float*) &pDst[0];

		if(channelCount == 3)
		{
			while(pixelCount--)
			{
				pDstMem[0] = pSrcMem[2]/255.0f;
				pDstMem[1] = pSrcMem[1]/255.0f;
				pDstMem[2] = pSrcMem[0]/255.0f;				
				pSrcMem += 3;
				pDstMem += 3;
			}
		}
		else
		{
			while(pixelCount--)
			{
				pDstMem[0] = pSrcMem[2]/255.0f;
				pDstMem[1] = pSrcMem[1]/255.0f;
				pDstMem[2] = pSrcMem[0]/255.0f;				
				pDstMem[3] = pSrcMem[3]/255.0f;				
				pSrcMem += 4;
				pDstMem += 4;
			}
		}
	}
	else
	{
		// convert float to byte

		const float* pSrcMem = (const float*) &pSrc[0];

		if(pDst == 0)
		{
			pDst = new char[pixelCount * channelCount * sizeof(char)];
		}

		unsigned char* pDstMem = (unsigned char*) &pDst[0];

		if(channelCount == 3)
		{
			while(pixelCount--)
			{
				int r = (int)(pSrcMem[0] * 255 + 0.5f);
				int g = (int)(pSrcMem[1] * 255 + 0.5f);
				int b = (int)(pSrcMem[2] * 255 + 0.5f);

				if(r<0) r=0;
				else if(r>255) r=255;
				if(g<0) g=0;
				else if(g>255) g=255;
				if(b<0) b=0;
				else if(b>255) b=255;

				pDstMem[0] = b;
				pDstMem[1] = g;
				pDstMem[2] = r;				
				pSrcMem += 3;
				pDstMem += 3;
			}
		}
		else
		{
			while(pixelCount--)
			{
				int r = (int)(pSrcMem[0] * 255 + 0.5f);
				int g = (int)(pSrcMem[1] * 255 + 0.5f);
				int b = (int)(pSrcMem[2] * 255 + 0.5f);
				int a = (int)(pSrcMem[3] * 255 + 0.5f);

				if(r<0) r=0;
				else if(r>255) r=255;
				if(g<0) g=0;
				else if(g>255) g=255;
				if(b<0) b=0;
				else if(b>255) b=255;
				if(a<0) a=0;
				else if(a>255) a=255;

				pDstMem[0] = b;
				pDstMem[1] = g;
				pDstMem[2] = r;				
				pDstMem[3] = a;
				pSrcMem += 4;
				pDstMem += 4;
			}
		}
	}
}


template <class T>
void ConvertChannels(const T maxAlphaValue, int pixelCount, const T* pSrc, int srcChannelCount, bool srcHasAlpha, T* pDst, int dstChannelCount, bool dstHasAlpha)
{
	if(srcChannelCount == dstChannelCount)
	{
		memcpy(pDst, pSrc, pixelCount*srcChannelCount*sizeof(T));

		if((dstChannelCount==4) && (srcHasAlpha!=dstHasAlpha))
		{
			while(pixelCount--)
			{
				pDst[3] = maxAlphaValue;
				pDst += 4;
			}
		}
	}
	else
	{
		if(srcChannelCount==3)
		{
			assert(dstChannelCount==4);
			while(pixelCount--)
			{
				pDst[0] = pSrc[0];
				pDst[1] = pSrc[1];
				pDst[2] = pSrc[2];
				pDst[3] = maxAlphaValue;
				pSrc += 3;
				pDst += 4;
			}
		}
		else
		{
			assert(dstChannelCount==3);
			while(pixelCount--)
			{
				pDst[0] = pSrc[0];
				pDst[1] = pSrc[1];
				pDst[2] = pSrc[2];
				pSrc += 4;
				pDst += 3;
			}
		}
	}
}


// Convert to Xbox 360 specific CXT1 normal map format
ImageObject* CImageCompiler::ConvertARGB8ToXbox360CTX1( ImageObject *inSrc )
{
	TPixelFormatID dstPixelformat = CPixelFormats::GetNoFromD3DFormat(D3DFMT_CTX1);
	if(inSrc->GetPixelFormat() != CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8))
	{
		assert(0);
		return NULL;
	}

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	inSrc->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwDepth > 1 || dwSides > 1)
	{
		assert(0);
		return NULL;
	}

	CRAWImage *pRet = new CRAWImage(dwWidth,dwHeight,dwMips,dstPixelformat,IsFinalPixelFormatCompressed());

	pRet->CopyPropertiesFrom(*inSrc);

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = inSrc->GetWidth(dwMip);
		uint32 dwLocalHeight = inSrc->GetHeight(dwMip);

		char *pSrcMem=0;
		uint32 dwSrcPitch=0;

		char *pDstMem=0;
		uint32 dwDstPitch=0;

		if(!inSrc->Lock(0,dwMip,pSrcMem,dwSrcPitch))
		{
			assert(0);
			delete pRet;
			return 0;
		}

		if(!pRet->Lock(0,dwMip,pDstMem,dwDstPitch))
		{
			assert(0);
			delete pRet;
			return 0;
		}

		if(!m_pCC->pRC->GetCompressor()->CompressARGB8ToCTX1(pSrcMem, pDstMem, dwLocalWidth, dwLocalHeight))
		{
			assert(0);
			delete pRet;
			return 0;
		}

		pRet->Unlock(dwMip);
		inSrc->Unlock(dwMip);
	}

	return pRet;
}

// Convert between RGB8, RGBA8, RGBX8, RGBF32, RGBAF32, RGBXF32
ImageObject* CImageCompiler::ConvertBetweenSimpleFormats( ImageObject *inSrc, D3DFORMAT dstD3dFormat )
{
	assert(inSrc);

	const TPixelFormatID srcPixelformat = inSrc->GetPixelFormat();
	const D3DFORMAT srcD3dFormat = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;

	const TPixelFormatID dstPixelformat = CPixelFormats::GetNoFromD3DFormat(dstD3dFormat);

	if(dstD3dFormat == srcD3dFormat)
	{
		return inSrc;
	}

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	inSrc->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwSides!=1)
		return 0;				// currently we don't support cubemaps

	if(dwDepth!=1)
		return 0;				// currently we don't support volume textures
	
	bool srcIsByte;
	int srcChannelCount;
	bool srcHasAlpha;

	if(!GetSimpleFormatInfo(srcD3dFormat, srcIsByte, srcChannelCount, srcHasAlpha))
	{
		assert(0);		
		return 0;
	}

	bool dstIsByte;
	int dstChannelCount;
	bool dstHasAlpha;

	if(!GetSimpleFormatInfo(dstD3dFormat, dstIsByte, dstChannelCount, dstHasAlpha))
	{
		assert(0);		
		return 0;
	}

	uint32 dwInPlanes=0;

	CRAWImage *pRet = new CRAWImage(dwWidth,dwHeight,dwMips,dstPixelformat,IsFinalPixelFormatCompressed());

	pRet->CopyPropertiesFrom(*inSrc);

	char* pTmp = 0;

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = inSrc->GetWidth(dwMip);
		uint32 dwLocalHeight = inSrc->GetHeight(dwMip);

		uint32 dwLines = dwLocalHeight;

		if(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->bCompressed)
		{
			dwLines = (dwLocalHeight+3)/4;
		}

		char *pSrcMem=0;
		uint32 dwSrcPitch=0;

		char *pDstMem=0;
		uint32 dwDstPitch=0;

		if(!inSrc->Lock(0,dwMip,pSrcMem,dwSrcPitch))
		{
			assert(0);
			return 0;
		}

		if(!pRet->Lock(0,dwMip,pDstMem,dwDstPitch))
		{
			assert(0);
			return 0;
		}

		for(uint32 dwY=0;dwY<dwLines;++dwY)
		{
			const char* pSrc = &pSrcMem[dwSrcPitch*dwY];

			if(srcIsByte != dstIsByte)
			{
				ConvertBetweenFloatAndUbyte(srcIsByte, dwLocalWidth, srcChannelCount, pSrc, pTmp);
				pSrc = pTmp;
			}

			if(dstIsByte)
			{
				ConvertChannels<unsigned char>(
					255, dwLocalWidth, (const unsigned char*)pSrc, srcChannelCount, srcHasAlpha, 
					(unsigned char*)&pDstMem[dwDstPitch*dwY], dstChannelCount, dstHasAlpha);
			}
			else
			{
				ConvertChannels<float>(
					1.0f, dwLocalWidth, (const float*)pSrc, srcChannelCount, srcHasAlpha, 
					(float*)&pDstMem[dwDstPitch*dwY], dstChannelCount, dstHasAlpha);
			}
		}

		pRet->Unlock(dwMip);
		inSrc->Unlock(dwMip);
	}
	
	if(pTmp)
	{
		delete[] pTmp;
		pTmp = 0;
	}

	// recursive for all attached images
	if(ImageObject *pAttached = inSrc->GetAttachedImage())
	{
		CRAWImage *pAttachedRAW = CopyToNewCRAWImage(pAttached);

		pRet->SetAttachedImage(pAttachedRAW);
	}

	return pRet;
}


static bool test_unpackDXT5Block_NVDXT(const nv::BlockDXT5& b, nv::ColorBlock& cOut)
{
	const uint blockSize = sizeof(b);
	assert(blockSize == 16);

	const D3DFORMAT fmtTo = D3DFMT_A8R8G8B8;
	const int src_format=kDXT5;

	const uint32 dwLocalWidth = 4;
	const uint32 dwLocalHeight = 4;

	const uint32 dwOutPlanes = 4;

	char pDDSMemoryFile[sizeof(CImageExtensionHelper::DDS_FILE_DESC)+blockSize];	// magic+header,data
				
	// magic
	*(DWORD *)pDDSMemoryFile = MAKEFOURCC('D','D','S',' ');

	// header
	CImageExtensionHelper::DDS_HEADER *ddsh = (CImageExtensionHelper::DDS_HEADER *)&pDDSMemoryFile[sizeof(uint32)];
	memset(ddsh,0,sizeof(CImageExtensionHelper::DDS_HEADER));
	ddsh->dwSize = sizeof(CImageExtensionHelper::DDS_HEADER);
	ddsh->dwWidth = dwLocalWidth;
	ddsh->dwHeight = dwLocalHeight;
	ddsh->dwHeaderFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_LINEARSIZE;
	ddsh->dwPitchOrLinearSize = blockSize;
	ddsh->ddspf.dwSize = sizeof(ddsh->ddspf);
	ddsh->ddspf.dwFlags = DDS_FOURCC;
	ddsh->ddspf.dwFourCC = D3DFMT_DXT5;
	ddsh->ddspf.dwFlags |= DDPF_ALPHAPIXELS;
	ddsh->dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;

	// data
	const char *pSrcMem = (const char *)&b;
	memcpy(&pDDSMemoryFile[sizeof(CImageExtensionHelper::DDS_FILE_DESC)],pSrcMem,blockSize);

	// nvDXTdecompress calls ReadDTXnFile to get the data
	g_DDSMemoryFileLock.Lock();
	g_DDSMemoryFilePtr = pDDSMemoryFile;		// to pass the ptr to ReadDTXnFile()
	unsigned char *pUncompressed = 0;
	uint32 dwSrcPitch = 16;
	{
		CryAutoLockRC lock(g_nvDXTLock);
		pUncompressed = nvDXTdecompress((int &)dwLocalWidth,(int &)dwLocalHeight,(int &)dwOutPlanes,(int &)dwLocalWidth,(int &)dwSrcPitch,(int &)src_format);
	}
	g_DDSMemoryFileLock.Unlock();

	if(!pUncompressed)
	{
		return false;
	}

	{
		unsigned char *p=pUncompressed;

		for(uint32 i=0; i<16; ++i)
		{
			const unsigned char tmp = p[0];
			p[0] = p[2];
			p[2] = tmp;
			p += 4;

		}
	}

	cOut.init(pUncompressed, 4, 4, 4*4, 0, 0);

	delete[] pUncompressed;

	// test
	if(0)
	{
		FILE *out = fopen("c:/aa.dds","wb");

		fwrite(pDDSMemoryFile,sizeof(pDDSMemoryFile),1,out);

		fclose(out);
	}

	return true;
}


static void test_packAndUnpackDXT5Block_NVDXT(const nv::ColorBlock& cIn, nv::ColorBlock& cOut/*, bool highQuality*/)
{
	nv::BlockDXT5 b;

	SLoadAllMipSurfaces UserData;
	UserData.m_pThis=0;
	UserData.m_dwMip=0;
	UserData.m_dstOffset = 0;
	UserData.m_dstMem = (char*)&b;

	CompressionOptions options;
	options.TextureFormat = kDXT5;
	options.user_data=&UserData;
	options.MipMapType=dNoMipMaps;
	options.bQuickCompress = false;		// 'quick' produces very ugly results (20sec -> 5sec, similar to DirectX compressor speed) 

	uint32 dwLocalWidth = 4;
	uint32 dwLocalHeight = 4;
	uint32 dwPitch = 4*4;
	uint32 dwInPlanes = 4;

	HRESULT hRes = nvDXTcompressBGRA((unsigned char *)&cIn,dwLocalWidth,dwLocalHeight,dwPitch,&options,dwInPlanes,LoadAllMipSurfacesOneBlock);

	assert(hRes>=0);

	assert( UserData.m_dstOffset == 16 );

	if(0)
	{
		// Test. 
		// Sokov: NVDXT goes mad and produces weird pixels if following block is used. Looks like they
		// not expecting to have color0 > color1 in DXT5 block.

		memset(&b, 0, sizeof(b));
		b.color.col0.b = 10;
		b.color.indices = 0xffffffff;
	}

	if(0)
	{
		// Test. 
		// Sokov: NVDXT should produce green lines with green values 247,0,165,82, but produces 247,0,164,82,
		// because NVidia and Squish both use integer arithmetic with clamping (instead of rounding to closest 
		// integer) in computation of two intermediate DXT1 colors.

		memset(&b, 0, sizeof(b));
		b.color.col1.g = 61;
		b.color.indices = 0xffaa5500;
	}

	// Unpack by NVSquish
	b.decodeBlock(&cOut);

	if(0)
	{
		// Test NVDXT.
		// Check if NVDXT gives same result as NVSquish does.

		nv::ColorBlock cOut2;

		bool ok = test_unpackDXT5Block_NVDXT(b, cOut2);
		assert(ok);

		bool failed = (memcmp(&cOut, &cOut2, sizeof(cOut)) != 0);
		assert(!failed);
	}
}

static void test_packAndUnpackDXT5Block_NVSquish(const nv::ColorBlock& cIn, nv::ColorBlock& cOut, bool highQuality)
{
	nv::BlockDXT5 b;

	SLoadAllMipSurfaces UserData;
	UserData.m_pThis=0;
	UserData.m_dwMip=0;
	UserData.m_dstOffset = 0;
	UserData.m_dstMem = (char*)&b;

	nv::CryCompressor::CryCompressorData compress;

	compress.srcBGRA8 = &cIn;
	compress.width = 4;
	compress.height = 4;
	compress.pitch = 4*4;

	compress.highQuality = highQuality;

	compress.rWeight = 1.0f;
	compress.gWeight = 1.0f;
	compress.bWeight = 1.0f;

	compress.userPtr = &UserData;

	compress.userOutputFunction = CompressorOutputCallbackOneBlock;

//	if(fmtTo == D3DFMT_DXT3)
//	{
//		nv::CryCompressor::compressDXT3(compress);
//	}
//	else
	{
		nv::CryCompressor::compressDXT5(compress);
	}

	assert( UserData.m_dstOffset == 16 );

	// Unpack
	b.decodeBlock(&cOut);
}


static uint test_computeAlphaError(const nv::ColorBlock& c0, const nv::ColorBlock& c1)
{
	const nv::Color32* pc0 = c0.colors();
	const nv::Color32* pc1 = c1.colors();

	uint err = 0;

	for(int i=0; i<16; ++i)
	{
		const int d = ((int)pc0[i].a) - ((int)pc1[i].a);
		err += d * d;
	}

	return err;
}


bool CImageCompiler::test_DXT5Compression(ImageObject *inSrc)
{
	assert(inSrc);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	inSrc->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwSides!=1)
		return false;				// currently we don't support cubemaps

	if(dwDepth!=1)
		return false;				// currently we don't support volume textures

	// 3DC can only be processed by ATI library
	if(inSrc->GetPixelFormat()==CPixelFormats::GetNoFromD3DFormat(D3DFMT_3DC))
		return false;

	ImageObject* pRet = 0;

	const D3DFORMAT fmtTo = D3DFMT_A8R8G8B8;

	bool ok = (ChangeFormatWithDirectX(inSrc,fmtTo,pRet,0) == S_OK);

	if(!ok)
	{
		goto ret;
	}
	ok = false;

	pRet->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	assert((dwWidth%4) == 0);
	assert((dwHeight%4) == 0);

	{
		char *pMem=0;
		uint32 dwPitch=0;

		HRESULT hRes=-1;		// failed

		const int mip = 0;
		if(!inSrc->Lock(0,mip,pMem,dwPitch))
			goto ret; 

		assert(pMem);

		for(uint y=0; y<dwHeight; y+=4)
		for(uint x=0; x<dwWidth; x+=4)
		{
			nv::ColorBlock cOut;

			nv::ColorBlock cIn;
			cIn.init( pMem, dwWidth, dwHeight, dwPitch, x, y );

			test_packAndUnpackDXT5Block_NVDXT(cIn, cOut);
			const uint errNVDXT = test_computeAlphaError(cIn, cOut);

//			test_packAndUnpackDXT5Block_NVSquish(cIn, cOut, false);
//			const uint errNVSquishLowQ = test_computeAlphaError(cIn, cOut);

			test_packAndUnpackDXT5Block_NVSquish(cIn, cOut, true);
			const uint errNVSquishHighQ = test_computeAlphaError(cIn, cOut);

			if(errNVDXT < errNVSquishHighQ)
			{
				RCLogError("NVSquish failed (alpha channel)");
				goto ret;
			}
		}

		inSrc->Unlock(mip);

		ok = true;
	}


ret:
	if(pRet && (pRet != inSrc))
	{
		delete pRet;
	}
	return ok;
}



class ImageToProcess
{
	ImageObject* m_img;
	bool m_isOriginal;
	bool m_isFailed;

public:
	ImageToProcess(ImageObject* a_img)
	{
		assert(a_img);

		m_img = a_img;
		m_isOriginal = true;
		m_isFailed = false;
	}

	~ImageToProcess()
	{
		m_img = 0;
	}

	ImageObject* set(ImageObject* a_img)
	{
		assert(m_img);

		if(a_img == 0)
		{
			m_isFailed = true;
		}

		if(m_img != a_img)
		{
			if(!m_isOriginal)
			{
				delete m_img;
			}
			m_img = a_img;
			m_isOriginal = false;
		}

		return m_img;
	}

	ImageObject* setOriginal(ImageObject* a_img)
	{
		assert(m_img);
		assert(a_img);

		set(0);

		m_img = a_img;
		m_isFailed = false;
		m_isOriginal = true;

		return m_img;
	}

	ImageObject* get() const
	{
		return m_img;
	}
	
	bool isOriginal() const
	{
		assert(!m_isFailed);
		return m_isOriginal;
	}
};





ImageObject *CImageCompiler::ConvertFormatWithSpecifiedCompressor( ImageObject *inSrc_, D3DFORMAT fmtTo, CImageProperties::EGlobalCompressor compressor )
{
	assert(inSrc_);

	ImageToProcess img(inSrc_);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	img.get()->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwSides!=1)
		return 0;				// currently we don't support cubemaps

	if(dwDepth!=1)
		return 0;				// currently we don't support volume textures

	if(fmtTo==D3DFMT_3DC)
		if(InputUsesDenormalizedNormals())			// 3DC only if we have normalized normals
			fmtTo=D3DFMT_X8R8G8B8;

	int emulate3Dc = 0;
	m_pCC->config->Get("Emulate3Dc",emulate3Dc);

	// Note: 3DC can be read or written by ATI library only
	if(img.get()->GetPixelFormat()==CPixelFormats::GetNoFromD3DFormat(D3DFMT_3DC))
	{
		if((fmtTo==D3DFMT_3DC) && (emulate3Dc>1))
		{
			if( !img.set( ConvertFormatWithSpecifiedCompressor(img.get(), D3DFMT_X8R8G8B8,compressor) ) )
			{
				return 0;
			}

			img.set( ConvertFormatWithSpecifiedCompressor(img.get(),fmtTo,compressor) );
		}
		else
		{
			img.set( ConvertFormatWithATI(img.get(), fmtTo) );
		}
		return img.get();
	}

	if(fmtTo==D3DFMT_3DC)
	{
		if(emulate3Dc<=1)
		{
			return img.set( ConvertFormatWithATI(img.get(),fmtTo) );
		}
		
		if( !img.set( ConvertFormatWithSpecifiedCompressor(img.get(),D3DFMT_X8R8G8B8,compressor) ) )
		{
			return 0;
		}

		if((compressor==CImageProperties::eCGNVSquish) || (compressor==CImageProperties::eCGNVSquishFast))
		{
			img.set( ChangeFormatWithSquishToDXT5n(img.get(), (compressor==CImageProperties::eCGNVSquish)) );
		}
		else
		{
			img.set( CopyToNewCRAWImage(img.get()) );
			assert(img.get());

			// We should have the image with real alpha channel (A8, not X8) to allow DXT5 compressor to work correctly
			const TPixelFormatID pixelFormat = img.get()->GetPixelFormat();
			if(pixelFormat != CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8))
			{
				assert(pixelFormat==CPixelFormats::GetNoFromD3DFormat(D3DFMT_X8R8G8B8));
				img.get()->SetPixelFormat(CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8));
			}

			assert(!img.isOriginal());
			SwizzleRedToGreenAndGreenToAlpha_InPlace(img.get());

			// We use NVDXT's compressor because it produces best quality/speed for green channel
			img.set( ConvertFormatWithSpecifiedCompressor(img.get(), D3DFMT_DXT5, CImageProperties::eCGNVDXT) );
		}

		return img.get();
	}

	// convert to CXT1  Xbox 360 specific format
	if(fmtTo==D3DFMT_CTX1)
	{
		if(m_pCC->platform != ePlatform_X360)
		{
			RCLogError("Converting D3DFMT_CTX1 is only for Xbox 360 platform!");
			return 0;
		}

		if( !img.set( ConvertFormatWithSpecifiedCompressor(img.get(), D3DFMT_A8R8G8B8,compressor) ) )
		{
			return 0;
		}

		return img.set( ConvertARGB8ToXbox360CTX1(img.get()) );
	}

	{
		const TPixelFormatID srcPixelformat = img.get()->GetPixelFormat();
		const D3DFORMAT d3dSrcFormat = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;

		if(fmtTo == d3dSrcFormat)
		{
			return img.get();
		}

		if(IsFormatSimple(d3dSrcFormat) && IsFormatSimple(fmtTo))
		{
			return img.set( ConvertBetweenSimpleFormats(img.get(), fmtTo) );
		}
	}

/*
	// try ATI
	if(compressor==CImageProperties::eCGATI)
	{
		pRet=ConvertFormatWithATI(inSrc,fmtTo);

		if(pRet)
			return pRet;
	}
*/

/* temporary. will be removed after testing.
	{
		if(!CImageCompiler::test_DXT5Compression(inSrc))
		{
			RCLogError("DXT5 test failed");
			return 0;
		}
	}
*/

	// try CryTextureCompressor
	// implementation note: 'while' instead of 'if' was used just to allow 'break' in the middle of the following code block
	while(compressor==CImageProperties::eCGExperimental)
	{
		bool bUseCompress = (fmtTo == D3DFMT_DXT1) || (fmtTo == D3DFMT_DXT3) || (fmtTo == D3DFMT_DXT5);

		const TPixelFormatID srcPixelformat = img.get()->GetPixelFormat();

		const D3DFORMAT d3dSrcFormat = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;

		uint32 dwInPlanes = 0;

		switch(d3dSrcFormat)
		{
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			// FIXME: need to force alpha==0xFF for D3DFMT_X8R8G8B8
			dwInPlanes = 4;
			break;

  		case D3DFMT_R8G8B8:
			// FIXME: need to implement 24bit->DXT
			if(bUseCompress)
			{
//				RCLogError("Converting D3DFMT_R8G8B8 to DXTx - not implemented yet");
			}
			bUseCompress = false;
			break;

		case D3DFMT_A32B32G32R32F:
			// FIXME: need to implement float->DXT
			if(bUseCompress)
			{
//				RCLogError("Converting D3DFMT_A32B32G32R32F to DXTx - not implemented yet");
			}
			bUseCompress = false;
			break;

		default:
			bUseCompress = false;
			break;
		}

		if(!bUseCompress)
		{
			// It's impossible to compress, so we resort to NVDXT.
			compressor = CImageProperties::eCGNVDXT;
			break;
		}

		const TPixelFormatID pixelFmtTo = CPixelFormats::GetNoFromD3DFormat(fmtTo);
		if(pixelFmtTo < 0)
		{
			RCLogError("Unknown destination format");
			return 0;
		}
		const SPixelFormats* pPixelFormatInfo = CPixelFormats::GetPixelFormatInfo( pixelFmtTo );
		assert(pPixelFormatInfo);
		const bool bCompressed = pPixelFormatInfo->bCompressed;
		if(!bCompressed)
		{
			RCLogError("Unexpected output non-compressed format");
			return 0;
		}

		ImageObject* pRet = new CRAWImage(dwWidth,dwHeight,dwMips,pixelFmtTo,bCompressed);

		pRet->CopyPropertiesFrom(*img.get());

		for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
		{
			uint32 dwLocalWidth = img.get()->GetWidth(dwMip);
			uint32 dwLocalHeight = img.get()->GetHeight(dwMip);

			char *pMem=0;
			uint32 dwPitch=0;

			HRESULT hRes=-1;		// failed

			if(img.get()->Lock(0,dwMip,pMem,dwPitch))
			{
				SLoadAllMipSurfaces UserData;
				UserData.m_pThis=pRet;
				UserData.m_dwMip=dwMip;
				UserData.m_dstOffset = 0;
				UserData.m_dstMem = 0;

				uint32 pitch = 0;
					
				pRet->Lock(0,dwMip,UserData.m_dstMem,pitch);

				CryTextureCompressor::CompressorParameters compress;

				compress.srcBGRA8 = pMem;
				compress.width = dwLocalWidth;
				compress.height = dwLocalHeight;
				compress.pitch = dwPitch;

				compress.highQuality = true;

				compress.rWeight = 1.0f;
				compress.gWeight = 1.0f;
				compress.bWeight = 1.0f;
				/*
				compress.rWeight = 0.3086f;
		        compress.gWeight = 0.6094f;    
				compress.bWeight = 0.0820f;    
				*/

				compress.userPtr = &UserData;

				compress.userOutputFunction = CryCompressorOutputCallback;

				if(fmtTo == D3DFMT_DXT1)
				{
					CryTextureCompressor::CompressDXT1ForceOpaque(compress);
				}
				else if(fmtTo == D3DFMT_DXT3)
				{
					CryTextureCompressor::CompressDXT3(compress);
				}
				else
				{
					CryTextureCompressor::CompressDXT5(compress);
				}

				pRet->Unlock(dwMip);

				assert( UserData.m_dstOffset == pitch*((dwLocalHeight+3)/4) );

				hRes = 0;

				img.get()->Unlock(dwMip);
			}

			if(FAILED(hRes))		// Lock failed
			{
				delete pRet;
				img.set(0);         // to release temporary image if it exists
				return 0;
			}
		} // for: all mips

		if(!pRet)
		{
			RCLogError("Unexpected failure in compression module");			
		}

		return img.set(pRet);
	}

	// try NVidia Squish
	// implementation note: 'while' instead of 'if' was used just to allow 'break' in the middle of the following code block
	while((compressor==CImageProperties::eCGNVSquish) || (compressor==CImageProperties::eCGNVSquishFast))
	{
		if(fmtTo == D3DFMT_DXT1)
		{
//			RCLogError("DXT1 not implemented yet in NVSquish. Using NVDXT.");
			compressor = CImageProperties::eCGNVDXT;
			break;
		}

		bool bUseCompress = (fmtTo == D3DFMT_DXT1) || (fmtTo == D3DFMT_DXT3) || (fmtTo == D3DFMT_DXT5);

		const TPixelFormatID srcPixelformat = img.get()->GetPixelFormat();

		const D3DFORMAT d3dSrcFormat = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;

		uint32 dwInPlanes = 0;

		switch(d3dSrcFormat)
		{
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			dwInPlanes = 4;
			break;

  		case D3DFMT_R8G8B8:
			// FIXME: need to implement 24bit->DXT
			if(bUseCompress)
			{
//				RCLogError("Converting D3DFMT_R8G8B8 to DXTx - not implemented yet");
			}
			bUseCompress = false;
			break;

		case D3DFMT_A32B32G32R32F:
			// FIXME: need to implement float->DXT
			if(bUseCompress)
			{
//				RCLogError("Converting D3DFMT_A32B32G32R32F to DXTx - not implemented yet");
			}
			bUseCompress = false;
			break;

		default:
			bUseCompress = false;
			break;
		}

		if(!bUseCompress)
		{
			// It's impossible to use NVSquish, so we resort to NVDXT.
			compressor = CImageProperties::eCGNVDXT;
			break;
		}

		const TPixelFormatID pixelFmtTo = CPixelFormats::GetNoFromD3DFormat(fmtTo);
		if(pixelFmtTo < 0)
		{
			RCLogError("Unknown destination format");
			return 0;
		}
		const SPixelFormats* pPixelFormatInfo = CPixelFormats::GetPixelFormatInfo( pixelFmtTo );
		assert(pPixelFormatInfo);
		const bool bCompressed = pPixelFormatInfo->bCompressed;
		if(!bCompressed)
		{
			RCLogError("Unexpected non-compressed format");
			return 0;
		}

		ImageObject* pRet = new CRAWImage(dwWidth,dwHeight,dwMips,pixelFmtTo,bCompressed);

		pRet->CopyPropertiesFrom(*img.get());

		for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
		{
			uint32 dwLocalWidth = img.get()->GetWidth(dwMip);			// we get error on NVidia with this (assumes input is 4x4 as well)
			uint32 dwLocalHeight = img.get()->GetHeight(dwMip);

			char *pMem=0;
			uint32 dwPitch=0;

			HRESULT hRes=-1;		// failed

			if(img.get()->Lock(0,dwMip,pMem,dwPitch))
			{
				assert((fmtTo == D3DFMT_DXT3) || (fmtTo == D3DFMT_DXT5));
//RCLogError("New DXT%s!!!", ((fmtTo == D3DFMT_DXT3)?"3":"5"));

				SLoadAllMipSurfaces UserData;
				UserData.m_pThis=pRet;
				UserData.m_dwMip=dwMip;
				UserData.m_dstOffset = 0;
				UserData.m_dstMem = 0;

				uint32 pitch = 0;
					
				pRet->Lock(0,dwMip,UserData.m_dstMem,pitch);

				nv::CryCompressor::CryCompressorData compress;

				compress.srcBGRA8 = pMem;
				compress.width = dwLocalWidth;
				compress.height = dwLocalHeight;
				compress.pitch = dwPitch;

				compress.highQuality = (compressor == CImageProperties::eCGNVSquish);

				compress.rWeight = 1.0f;
				compress.gWeight = 1.0f;
				compress.bWeight = 1.0f;

				compress.userPtr = &UserData;

				compress.userOutputFunction = CompressorOutputCallback;

				if(fmtTo == D3DFMT_DXT3)
				{
					nv::CryCompressor::compressDXT3(compress);
				}
				else
				{
					nv::CryCompressor::compressDXT5(compress);
				}

				pRet->Unlock(dwMip);

				assert( UserData.m_dstOffset == pitch*((dwLocalHeight+3)/4) );

				hRes = 0;

				img.get()->Unlock(dwMip);
			}

			if(FAILED(hRes))		// Lock or NVidia compression failed
			{
				delete pRet;
				img.set(0);         // to release temporary image if it exists
				return 0;
			}
		} // for: all mips

		if(!pRet)
		{
			RCLogError("Unexpected failure in compression module");			
		}

		return img.set(pRet);
	}

	const TPixelFormatID srcPixelformat=img.get()->GetPixelFormat();

	// try NVidia DXT Library
	if((compressor==CImageProperties::eCGNVDXT) || (compressor==CImageProperties::eCGATI))
	if(srcPixelformat!=CPixelFormats::GetNoFromD3DFormat(D3DFMT_A32B32G32R32F))								// NVidia doesn't handle this format in the library so we don't try - DirectX can do it nicely
	{
//		assert(!inSrc->GetDXTex());

		uint32 dwInPlanes=0;
		bool bCompress = true;


		const D3DFORMAT d3dSrcFormat = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;

		switch(d3dSrcFormat)
		{
			case D3DFMT_R8G8B8:
				dwInPlanes=3;
				break;

			case D3DFMT_A8R8G8B8:
			case D3DFMT_X8R8G8B8:
				dwInPlanes=4;
				break;

			default:
				bCompress=false;
				break;
		}

		CompressionOptions options;

		options.TextureFormat = FindCorespondingNVidiaFormat(fmtTo);

		if(options.TextureFormat==kTextureFormatLast)
			return 0;			// output pixel format not supported yet

		ImageObject* pRet = new CRAWImage(dwWidth,dwHeight,dwMips,CPixelFormats::GetNoFromD3DFormat(fmtTo),IsFinalPixelFormatCompressed());
		assert(pRet);

		pRet->CopyPropertiesFrom(*img.get());

		if(bCompress)
		{
			SLoadAllMipSurfaces UserData;

			UserData.m_pThis=pRet;

			options.user_data=&UserData;
			options.MipMapType=dNoMipMaps;
//			options.bQuickCompress=true;		// 20sec -> 5sec (similar to DirectX compressor speed) but very ugly results

			for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
			{
				uint32 dwLocalWidth = img.get()->GetWidth(dwMip);			// we get error on NVidia with this (assumes input is 4x4 as well)
				uint32 dwLocalHeight = img.get()->GetHeight(dwMip);

				char *pMem=0;
				uint32 dwPitch=0;

				HRESULT hRes=-1;		// failed

				RECT rectPreview;
				if (m_bInternalPreview)
				{
					rectPreview = m_pImageUserDialog->GetPreviewRectangle(dwMip+m_Props.GetReduceResolution()+m_pImageUserDialog->GetPreviewReduceResolution());
				}
				else
				{
					rectPreview.left = 0;
					rectPreview.right = dwLocalWidth;
					rectPreview.top = 0;
					rectPreview.bottom = dwLocalHeight;
				}

				const int iBlockLines = (rectPreview.bottom - rectPreview.top) / 4;

				if(img.get()->Lock(0,dwMip,pMem,dwPitch))
				{
					CryAutoLockRC lock(g_nvDXTLock);

					UserData.m_dwMip=dwMip;

					for (int iBlockLine=0; iBlockLine<iBlockLines; ++iBlockLine)
					{
						//early out: check for canceling preview generation
						if (m_bInternalPreview)
						{
							if (m_pImageUserDialog->PreviewGenerationCanceled())
							{
								img.get()->Unlock(dwMip);
								delete pRet;
								img.set(0);         // to release temporary image if it exists
								return 0;
							}
						}

						UserData.m_rectPreview = rectPreview;
						UserData.m_rectPreview.top += iBlockLine * 4;
						UserData.m_rectPreview.bottom = UserData.m_rectPreview.top + 4;

						RECT rectAdjusted = UserData.m_rectPreview;
						rectAdjusted.right -= 1;
						rectAdjusted.bottom -= 1;

						hRes = nvDXTcompressBGRA((unsigned char *)pMem,dwLocalWidth,dwLocalHeight,dwPitch,&options,dwInPlanes,LoadAllMipSurfaces, &rectAdjusted);


						m_Progress.Phase4(dwMip, iBlockLines);
					}

					img.get()->Unlock(dwMip);
				}

				if(FAILED(hRes))		// Lock or NVidia compression failed
				{
					delete pRet;
					img.set(0);         // to release temporary image if it exists
					return 0;
				}
			}
			
			return img.set(pRet);
		}
		else
		{
			uint32 dwWishOutPlanes=0;

			switch(fmtTo)
			{
				case D3DFMT_R8G8B8:
					dwWishOutPlanes=3;
					break;

				case D3DFMT_A8R8G8B8:
				case D3DFMT_X8R8G8B8:
					dwWishOutPlanes=4;
					break;
			}

			for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
			{
				uint32 dwLocalWidth = pRet->GetWidth(dwMip);
				uint32 dwLocalHeight = pRet->GetHeight(dwMip);

				uint32 dwLines = dwLocalHeight;

				if(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->bCompressed)
					dwLines = (dwLocalHeight+3)/4;

				char *pSrcMem;
				uint32 dwSrcPitch;

				img.get()->Lock(0,dwMip,pSrcMem,dwSrcPitch);

				int src_format=FindCorespondingNVidiaFormat(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo);

				char *pDDSMemoryFile = new char[sizeof(CImageExtensionHelper::DDS_FILE_DESC)+dwSrcPitch*dwLines];	// magic,header,data
				
				// magic
				*(DWORD *)pDDSMemoryFile = MAKEFOURCC('D','D','S',' ');
				// header
				CImageExtensionHelper::DDS_HEADER *ddsh = (CImageExtensionHelper::DDS_HEADER *)&pDDSMemoryFile[sizeof(uint32)];
				memset(ddsh,0,sizeof(CImageExtensionHelper::DDS_HEADER));
				ddsh->dwSize = sizeof(CImageExtensionHelper::DDS_HEADER);
				ddsh->dwWidth = dwLocalWidth;
				ddsh->dwHeight = dwLocalHeight;
				ddsh->dwHeaderFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_LINEARSIZE;
				ddsh->dwPitchOrLinearSize = dwSrcPitch*dwLines;
				if(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->bCompressed)
				{
					ddsh->ddspf.dwFlags = DDS_FOURCC;
					ddsh->ddspf.dwFourCC = CPixelFormats::GetPixelFormatInfo(srcPixelformat)->DxtNo;
				}
				else ddsh->ddspf.dwFlags = DDPF_RGB;

				if(strcmp(CPixelFormats::GetPixelFormatInfo(srcPixelformat)->szAlpha,"0")!=0)		// has alpha
					ddsh->ddspf.dwFlags |= DDPF_ALPHAPIXELS;

				ddsh->ddspf.dwSize = sizeof(ddsh->ddspf);
				ddsh->dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
				// data
				memcpy(&pDDSMemoryFile[sizeof(CImageExtensionHelper::DDS_FILE_DESC)],pSrcMem,dwSrcPitch*dwLines);

				img.get()->Unlock(dwMip);


				uint32 dwOutPlanes = dwWishOutPlanes;

				// nvDXTdecompress calls ReadDTXnFile to get the data
				g_DDSMemoryFileLock.Lock();
				g_DDSMemoryFilePtr = pDDSMemoryFile;		// to pass the ptr to ReadDTXnFile()
				unsigned char *pUncompressed = 0;
				{
					CryAutoLockRC lock(g_nvDXTLock);

					pUncompressed = nvDXTdecompress((int &)dwLocalWidth,(int &)dwLocalHeight,(int &)dwOutPlanes,(int &)dwLocalWidth,(int &)dwSrcPitch,src_format);
				}
				g_DDSMemoryFileLock.Unlock();

				delete [] pDDSMemoryFile;
				pDDSMemoryFile=0;

				char *pDstMem;
				uint32 dwDstPitch;

				pRet->Lock(0,dwMip,pDstMem,dwDstPitch);

				if(pUncompressed)
				{
					if(dwOutPlanes==dwWishOutPlanes)
					{
						assert(dwWishOutPlanes==4);
						assert(dwOutPlanes==4);

						unsigned char *Dst=(unsigned char *)pDstMem, *Src=pUncompressed;

						for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
						for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
						{
							*Dst++=Src[2];
							*Dst++=Src[1];
							*Dst++=Src[0];
							*Dst++=Src[3];
							Src+=4;
						}
					}
					else
					{
						assert(dwWishOutPlanes==4);
						assert(dwOutPlanes==3);

						unsigned char *Dst=(unsigned char *)pDstMem, *Src=pUncompressed;

						for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
						for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
						{
							*Dst++=Src[2];
							*Dst++=Src[1];
							*Dst++=Src[0];
							*Dst++=0;
							Src+=3;
						}
					}
				}

				pRet->Unlock(dwMip);

				if(!pUncompressed)		// NVidia compression failed
				{
					delete pRet;
					pRet=0;
					RCLogError("Error NVidia nvDXTdecompress failed, fallback to next compression (SrcFmt: %s)",CPixelFormats::GetPixelFormatInfo(srcPixelformat)->szName);
					img.setOriginal(inSrc_);
					break;
				}

				delete [] pUncompressed;
			}
		}

		if(pRet)
		{			
			return img.set(pRet);
		}
	}

	// try directX
	{
		ImageObject* pRet = 0;

		if(ChangeFormatWithDirectX(img.get(),fmtTo,pRet,0)!=S_OK)
		{
			pRet = 0;
		}

		return img.set(pRet);
	}
}





ImageObject *CImageCompiler::CreateHighPass( ImageObject *pInputImage, uint32 dwMipDown )
{
	D3DFORMAT d3dSrcFormat = CPixelFormats::GetPixelFormatInfo(pInputImage->GetPixelFormat())->DxtNo;

	if(d3dSrcFormat!=D3DFMT_A8R8G8B8 && d3dSrcFormat!=D3DFMT_X8R8G8B8)
	{
		assert(0);
		return 0;			// not supported yet
	}

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if (dwMipDown >= dwMips)
	{
		RCLogWarning("CImageCompiler::CreateHighPass can't go down %i MIP levels for high pass as there are not enough MIP levels available, going down by %i instead", dwMipDown, dwMips-1);
		dwMipDown = dwMips-1;
	}

	if(dwSides!=1)
		return 0;				// currently we don't support cubemaps

	if(dwDepth!=1)
		return 0;				// currently we don't support volume textures

	CRAWImage *pRet = new CRAWImage(dwWidth,dwHeight,dwMips,CPixelFormats::GetNoFromD3DFormat(d3dSrcFormat),IsFinalPixelFormatCompressed());

	pRet->CopyPropertiesFrom(*pInputImage);

	// copy the mip to subtract into a buffer (easier bilinear filtered lookups)
	CSimpleBitmap<Vec4> lowfreq;
	{
		uint32 dwLocalWidth = pInputImage->GetWidth(dwMipDown);
		uint32 dwLocalHeight = pInputImage->GetHeight(dwMipDown);

		char *pMem=0;
		uint32 dwPitch=0;

		if(pInputImage->Lock(0,dwMipDown,pMem,dwPitch))
		{
			lowfreq.Alloc(dwLocalWidth,dwLocalHeight);
			for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
			for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
			{
				uint32 dwCol = *((uint32 *)&pMem[dwX*4+dwY*dwPitch]);

				lowfreq.Set(dwX,dwY,UINT32ToVEC4(dwCol));
			}

			pInputImage->Unlock(dwMipDown);
		}
	}

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = pInputImage->GetWidth(dwMip);
		uint32 dwLocalHeight = pInputImage->GetHeight(dwMip);

		char *pMemOut=0;
		uint32 dwPitchOut=0;

		if(pRet->Lock(0,dwMip,pMemOut,dwPitchOut))
		{
			if(dwMip<dwMipDown)
			{
				float fScale = 1.0f/(1<<(dwMipDown-dwMip));

				char *pMemIn=0;
				uint32 dwPitchIn=0;

				if(pInputImage->Lock(0,dwMip,pMemIn,dwPitchIn))
				{
					for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
					for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
					{
						Vec4 lowfrequ;

//						todo: half texel required 

//						bool bOk = lowfreq.GetFiltered(dwX*fScale+0.5f,dwY*fScale+0.5f,lowfrequ);			assert(bOk);
						bool bOk = lowfreq.GetFiltered(dwX*fScale,dwY*fScale,lowfrequ);			assert(bOk);

						uint32 *pSrc = (uint32 *)(&pMemIn[dwX*4+dwY*dwPitchIn]);
						uint32 *pDest = (uint32 *)(&pMemOut[dwX*4+dwY*dwPitchOut]);

						Vec4 cSrc = UINT32ToVEC4(*pSrc);

//						Vec4 cNewDest = /*cSrc-lowfrequ+*/Vec4(0.5f,0.5f,0.5f,0.5f);
						Vec4 cNewDest = cSrc-lowfrequ+Vec4(0.5f,0.5f,0.5f,0.5f);

						cNewDest.x = clamp_tpl(cNewDest.x,0.0f,1.0f);
						cNewDest.y = clamp_tpl(cNewDest.y,0.0f,1.0f);
						cNewDest.z = clamp_tpl(cNewDest.z,0.0f,1.0f);
						cNewDest.w = clamp_tpl(cNewDest.w,0.0f,1.0f);

						*pDest = 0xff000000 | VEC4ToUINT32(cNewDest);
					}

					pInputImage->Unlock(dwMip);
				}
			}
			else
			{
				for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
				{
					uint32 *pDest = (uint32 *)(&pMemOut[dwY*dwPitchOut]);

					for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
						*pDest++ = 0xff7f7f7f;			// mips below the chosen highpass mip are grey
				}
			}
			pRet->Unlock(dwMip);
		}
	}

	return pRet;
}

ImageObject * CImageCompiler::CompressToRGBK( ImageObject *inSrc, float brightnessMultiplier )
{
	assert(inSrc && CPixelFormats::GetPixelFormatInfo(inSrc->GetPixelFormat())->DxtNo == D3DFMT_A32B32G32R32F);
	//assert(!inSrc->HasAlpha());

	char *pInMem;
	uint32 dwInPitch;
	char *pOutMem;
	uint32 dwOutPitch;

	uint32 w,h,depth,sides,mips;
	inSrc->GetExtend( w,h,depth,sides,mips );

	ImageObject* pRet = new CRAWImage(w, h, mips, CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8), false);

	for(uint32 dwMip=0;dwMip<mips;++dwMip)
	{
		uint32 dwLocalWidth=inSrc->GetWidth(dwMip);
		uint32 dwLocalHeight=inSrc->GetHeight(dwMip);

		if(inSrc->Lock(0,dwMip,pInMem,dwInPitch))
		{
			assert(dwLocalWidth*sizeof(ColorF) == dwInPitch);
			if(pRet->Lock(0,dwMip,pOutMem,dwOutPitch))
			{
				assert(dwLocalWidth*sizeof(ColorB) == dwOutPitch);
				for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
				{
					for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
					{
						const ColorF *pSrcTexel = (ColorF*)&pInMem[dwY*dwInPitch]+dwX;
						ColorB *pDestTexel = (ColorB*)&pOutMem[dwY*dwOutPitch]+dwX;

						// compress HDR texel to RGBK
						ColorF normalizedTexel = *pSrcTexel;
						const float alphaStep = 1.f / 255 / brightnessMultiplier;				// calculate quantization precision
						normalizedTexel.Clamp(0.f, 16.f);																// clamp to RGBK range
						float maxColor = max(1.f, normalizedTexel.Max());								// clamp to the max precision range
						maxColor = ceilf(maxColor / alphaStep) * alphaStep;							// stick it to alpha step to avoid quantization artifacts
						normalizedTexel.ScaleCol(1.f / maxColor);												// scale color by resulting scaling factor
						normalizedTexel.a = min(1.f, maxColor * brightnessMultiplier);	// clamp max luminance and write K in alpha
						normalizedTexel.Clamp();																				// clamp to 8-bits range

						// write it to dest image
						(*pDestTexel) = normalizedTexel.pack_argb8888();
					}
				}
			}
		}
	}

	return pRet;
}

class CAutoReleaseIUnknown
{
public:
	CAutoReleaseIUnknown( IUnknown *pUnk=0 ) :m_pUnk(pUnk)
	{
	}

	~CAutoReleaseIUnknown()
	{
		if(m_pUnk)
			m_pUnk->Release();
	}

	void Attach( IUnknown *pUnk )
	{
		m_pUnk=pUnk;
	}

private: // -------------------------------------------

	IUnknown *				m_pUnk;			//
};



ImageObject* CImageCompiler::ChangeFormatWithSquishToDXT5n( ImageObject *pInputImage, bool highQuality )
{
	assert(pInputImage);

	const D3DFORMAT fmtFrom=CPixelFormats::GetPixelFormatInfo(pInputImage->GetPixelFormat())->DxtNo;
	assert((fmtFrom==D3DFMT_A8R8G8B8)||(fmtFrom==D3DFMT_X8R8G8B8));

	const D3DFORMAT fmtTo = D3DFMT_DXT5;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	ImageObject* pRet = 0;

	const TPixelFormatID pixelFmtTo = CPixelFormats::GetNoFromD3DFormat(fmtTo);
	if(pixelFmtTo < 0)
	{
		RCLogError("Unknown destination format");
		return 0;
	}
	const SPixelFormats* pPixelFormatInfo = CPixelFormats::GetPixelFormatInfo( pixelFmtTo );
	assert(pPixelFormatInfo);
	const bool bCompressed = pPixelFormatInfo->bCompressed;
	if(!bCompressed)
	{
		RCLogError("Unexpected non-compressed format");
		return 0;
	}

	pRet = new CRAWImage(dwWidth,dwHeight,dwMips,pixelFmtTo,bCompressed);

	pRet->CopyPropertiesFrom(*pInputImage);

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = pInputImage->GetWidth(dwMip);
		uint32 dwLocalHeight = pInputImage->GetHeight(dwMip);

		char *pMem=0;
		uint32 dwPitch=0;

		HRESULT hRes=-1;		// failed

		if(pInputImage->Lock(0,dwMip,pMem,dwPitch))
		{
			SLoadAllMipSurfaces UserData;
			UserData.m_pThis=pRet;
			UserData.m_dwMip=dwMip;
			UserData.m_dstOffset = 0;
			UserData.m_dstMem = 0;

			uint32 pitch = 0;
				
			pRet->Lock(0,dwMip,UserData.m_dstMem,pitch);

			nv::CryCompressor::CryCompressorData compress;

			compress.srcBGRA8 = pMem;
			compress.width = dwLocalWidth;
			compress.height = dwLocalHeight;
			compress.pitch = dwPitch;

			compress.highQuality = highQuality;

			compress.rWeight = 1.0f;
			compress.gWeight = 1.0f;
			compress.bWeight = 1.0f;

			compress.userPtr = &UserData;

			compress.userOutputFunction = CompressorOutputCallback;

			nv::CryCompressor::compressDXT5n(compress);

			pRet->Unlock(dwMip);

			assert( UserData.m_dstOffset == dwPitch*((dwLocalHeight+3)/4) );

			hRes = 0;

			pInputImage->Unlock(dwMip);
		}

		if(FAILED(hRes))		// Lock or compression failed
		{
			delete pRet;
			return 0;
		}
	} // for: all mips

	if(!pRet)
	{
		RCLogError("Unexpected failure in compression module");			
	}
	return pRet;
}


void CImageCompiler::SwizzleRedToGreenAndGreenToAlpha_InPlace(ImageObject* pInputImage)
{
	assert(pInputImage);

	const D3DFORMAT fmtFrom=CPixelFormats::GetPixelFormatInfo(pInputImage->GetPixelFormat())->DxtNo;
	assert((fmtFrom==D3DFMT_A8R8G8B8)||(fmtFrom==D3DFMT_X8R8G8B8));

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = pInputImage->GetWidth(dwMip);
		uint32 dwLocalHeight = pInputImage->GetHeight(dwMip);

		char *pMem=0;
		uint32 dwPitch=0;

		HRESULT hRes=-1;		// failed

		if(!pInputImage->Lock(0,dwMip,pMem,dwPitch))
		{
			RCLogError("CImageCompiler::SwizzleRedTo... LockRect mip=%d failed",dwMip);
			return;
		}

		for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
		{
			unsigned char *src = (unsigned char *)&pMem[dwY*dwPitch];

			for(uint32 dwX=0;dwX<dwLocalWidth;++dwX,src+=4)
			{
				const unsigned char r = src[2];
				const unsigned char g = src[1];

				src[0] = 0;
				src[1] = r;
				src[2] = 0;
				src[3] = g;
			}
		}

		pInputImage->Unlock(dwMip);
	} // for: all mips
}

HRESULT CImageCompiler::ChangeFormatWithDirectX( ImageObject *pInputImage, D3DFORMAT fmtTo, ImageObject *&pNewImage,
	const uint32 indwReduceResolution )
{
	assert(!pNewImage);
	assert(pInputImage);
	assert(pNewImage!=pInputImage);

	if(!InitDirectX())
		return false;

	HRESULT hr;
	LPDIRECT3DTEXTURE9 pmiptexNew;

	LPDIRECT3DBASETEXTURE9 ptexCur=pInputImage->GetDXTex();

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	CAutoReleaseIUnknown AutoReleaseUnk;		// calls Release() on it's attached pointer when if goes out of focus

	D3DFORMAT fmtFrom=CPixelFormats::GetPixelFormatInfo(pInputImage->GetPixelFormat())->DxtNo;

	if(!ptexCur)
	{
		LPDIRECT3DTEXTURE9 p2DTexture=0;

		hr = m_pd3ddev->CreateTexture(dwWidth,dwHeight, dwMips, 0, fmtFrom, D3DPOOL_MANAGED, &p2DTexture, NULL);
		if (FAILED(hr))
		{
			char *szFmtName = GetNameFromD3DFormat(fmtTo);
			RCLogError("CreateTexture failed fmt=%s (texture format not supported by driver?)",szFmtName);
			return hr;
		}

		ptexCur = p2DTexture;

		AutoReleaseUnk.Attach(ptexCur);		// calls Release() on it's attached pointer when if goes out of focus
		
		for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
		{
			IDirect3DSurface9 *pSurface=0;

			hr = p2DTexture->GetSurfaceLevel(dwMip,&pSurface);
			if (FAILED(hr))
			{
				char *szFmtName = GetNameFromD3DFormat(fmtTo);
				RCLogError("GetSurfaceLevel failed fmt=%s (texture format not supported by driver?)",szFmtName);
				return hr;
			}

			char *pMem;
			uint32 dwPitch;

			if(pInputImage->Lock(0,dwMip,pMem,dwPitch))
			{
				RECT rectSrc;

				rectSrc.left=0;
				rectSrc.top=0;
				rectSrc.right=pInputImage->GetWidth(dwMip);
				rectSrc.bottom=pInputImage->GetHeight(dwMip);
				hr = D3DXLoadSurfaceFromMemory(pSurface,0,0,pMem,fmtFrom,dwPitch,0,&rectSrc,D3DX_FILTER_NONE,0);

				if (FAILED(hr))
				{
					RCLogError("'%s' DX ERROR: %s", "2D Texure, D3DXLoadSurfaceFromMemory()",DXGetErrorString(hr));
					return hr;
				}

				pInputImage->Unlock(0);
			}
			else
			{
				pSurface->Release();
				RCLogError("pInputImage->Lock failed");
				return S_FALSE;
			}

			pSurface->Release();
		}
	}



	if (fmtFrom == D3DFMT_DXT2 || fmtFrom == D3DFMT_DXT4)
	{
		if (fmtTo == D3DFMT_DXT1)
		{
			RCLogError("ERROR_PREMULTTODXT1");
		}
		else if (fmtTo != D3DFMT_DXT2 && fmtTo != D3DFMT_DXT4)
		{
			RCLogError("ERROR_PREMULTALPHA");
			return E_FAIL;
		}
	}


/*		if(fmtTo==D3DFMT_P8)
	{
		pNewImage = new P8Image(dwWidth, dwHeight, dwMips);
	}
	else
*/
	{
		if ((fmtTo == D3DFMT_DXT1 || fmtTo == D3DFMT_DXT2 ||
			fmtTo == D3DFMT_DXT3 || fmtTo == D3DFMT_DXT4 ||
			fmtTo == D3DFMT_DXT5) && (dwWidth % 4 != 0 || dwHeight % 4 != 0))
		{
			RCLogError("ERROR_NEEDMULTOF4 = for DXT compression we need width and height to be multiple of 4");
			return E_FAIL;
		}

		hr = m_pd3ddev->CreateTexture(dwWidth,dwHeight, dwMips, 0, fmtTo, D3DPOOL_MANAGED, &pmiptexNew, NULL);
		if (FAILED(hr))
		{
			char *szFmtName = GetNameFromD3DFormat(fmtTo);
			RCLogError("GetNameFromD3DFormat failed fmt=%s (texture format not supported by driver?)",szFmtName);
			return hr;
		}
		pNewImage = new DXImage(pmiptexNew);

		pNewImage->CopyPropertiesFrom(*pInputImage);
	}
	if (FAILED(BltAllLevels(pInputImage,D3DCUBEMAP_FACE_FORCE_DWORD, ptexCur, pNewImage,indwReduceResolution, fmtTo)))
		return hr;

	return S_OK;
}



HRESULT CImageCompiler::BltAllLevels( ImageObject *pInputImage, D3DCUBEMAP_FACES FaceType, LPDIRECT3DBASETEXTURE9 ptexSrc,
	ImageObject *pNewImage, const uint32 indwReduceResolution, D3DFORMAT fmtTo )
{
	uint32 dwReduceRes=indwReduceResolution;
//	uint32 dwMips=_CalcMipCount(m_dwOrigWidth,m_dwOrigHeight)-dwReduceRes;
	uint32 dwMips=ptexSrc->GetLevelCount();
	
	for (uint32 iLevel = 0; iLevel < dwMips; iLevel++)
	{
		LPDIRECT3DTEXTURE9 pmiptexSrc = (LPDIRECT3DTEXTURE9)ptexSrc;
		LPDIRECT3DSURFACE9 psurfSrc = NULL;
		DXERRORCHECK("GetSurfaceLevel",pmiptexSrc->GetSurfaceLevel(iLevel+indwReduceResolution, &psurfSrc));

		{ 
			HRESULT _hr = pNewImage->UpdateSurface(psurfSrc, iLevel,D3DX_FILTER_NONE, FaceType, m_pd3ddev); 
			if(FAILED(_hr))
				RCLogError("'%s' DX ERROR: %s", "2D Texure, GetSurfaceLevel()",DXGetErrorString(_hr));
		}

		ReleasePpo(&psurfSrc);
	};
	
	return S_OK;
}

void CImageCompiler::Release()
{
	delete this;
}





void CImageCompiler::SetCC( ConvertContext *pCC )
{
	m_pCC=pCC;	
	m_Props.SetPropsCC(pCC);
}


size_t GetFileSize( const char *szFilePath )
{
	FILE *in=fopen(szFilePath,"rb");

	if(!in)
		return 0;

	//measure the file size
	fseek(in,0,SEEK_END);

	size_t RetSize=ftell(in);

	fseek(in,0,SEEK_SET);
	fclose(in);

	return RetSize;
}

//#include <crtdbg.h>		// to find memory leaks

// Process file. Return error code or 0 if successful
bool CImageCompiler::Process( ConvertContext &cc )
{
//	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );		// to find memory leaks

	m_Props.SetToDefault();

	SetCC(&cc);

	if (!m_bInitialized)
		Init( cc.pRC->GetEmptyWindow() );

	// highest to lowest priority:
	//
	// ranger_statue._rc  rc.ini  commandline
	string sourceFile = cc.getSourcePath();

	bool bSuccess=true;

	if(LoadInput(sourceFile,cc.sourceFileFinalExtension))
	{
		if (CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"cch"))
		{
			string curPreset;
			if (!m_pCC->config->Get("preset", curPreset) || curPreset == "")
			{
				m_Props.SetPreset("ColorChart");
				SetPresetSettings();
			}
		}

		if(!LoadConfigFile(eCP_PriorityHighest))
			return false;			// error
		
		SetPresetSettings();

		if(m_Props.GetUserDialog())
		{
			CImageUserDialog dialog;

			m_pImageUserDialog = &dialog;

			dialog.DoModal(this);

			m_pImageUserDialog = NULL;
		}
		else 
		{
			bSuccess=RunWithProperties(true);			// run and save
		}

		if(bSuccess)
			m_pCC->pRC->AddFileStats(m_pCC->fileStatsHandle, sourceFile,GetFileSize(sourceFile),GetFileSize(m_pCC->getOutputPath()),m_Props.GetPreset(),GetDestInfoString());
	}
	else
	{
		RCLogError("CImageCompiler::Process LoadInput '%s' Ext:'%s' failed",(const char*)sourceFile,(const char*)cc.sourceFileFinalExtension );
		bSuccess=false;
	}

	SetCC(0);

	FreeTexture();

/*	static int iCounter=0;			++iCounter;
	if(iCounter==16)
	{
		_CrtDumpMemoryLeaks();		// to find memory leaks
		iCounter=0;
	}
*/
	return bSuccess;		// success
}

void CImageCompiler::ConstructAndSetOutputFile( ConvertContext& cc )
{
	cc.SetOutputFile( PathHelpers::ReplaceExtension( cc.sourceFileFinal,"dds" ) );
}

void CImageCompiler::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const filename(cc.getOutputPath());

	if (filenameBuffer && (filename.length() < bufferSize))
	{
		strcpy(filenameBuffer, filename.c_str());		
	}
}

bool CImageCompiler::ProceedToNextPass( ImageObject * &pCurrent, ImageObject *pNew )
{
	if(pCurrent!=m_pInputImage && pCurrent!=pNew)			// never release the input image, don't delete the same
		delete pCurrent;

	pCurrent=pNew;

	return pCurrent!=0;
}


// access to an float array
class CInputProxy_Bump2Normal
{
public:
	// constructor
	CInputProxy_Bump2Normal( float *pMem, const size_t nPitchInBytes, const uint32 dwWidth, const uint32 dwHeight )
		:m_pMem(pMem), m_nPitchInBytes(nPitchInBytes), m_dwWidth(dwWidth), m_dwHeight(dwHeight)
	{
		assert(pMem);
		assert(dwWidth);
		assert(dwHeight);
		assert(m_nPitchInBytes);
	}

	// needed to be a valid proxy
	inline bool GetForFiltering( const uint32 dwX, const uint32 dwY, const uint32 srcX, const uint32 srcY, float &out, EImageFilteringMode mode ) const
	{
		return Get(dwX, dwY, out);
	}

	// needed to be a valid proxy
	inline bool Get( const uint32 dwX, const uint32 dwY, float &out ) const
	{
		uint32 x = dwX%m_dwWidth, y = dwY%m_dwHeight;

		char *pMem = (char *)m_pMem;

		out = ((float*)(pMem+y*m_nPitchInBytes)) [x];

		return true;
	}

	// needed to be a valid proxy
	uint32 GetWidth() const
	{
		return m_dwWidth;
	}
	
	// needed to be a valid proxy
	uint32 GetHeight() const
	{
		return m_dwHeight;
	}

private: // ----------------------------------------------------------

	float *			m_pMem;								//
	size_t			m_nPitchInBytes;			//
	uint32			m_dwWidth;						//
	uint32			m_dwHeight;						//
};


/*
bool CImageCompiler::ConvertToFloat4( ImageObject * &pInput )
{
	if(pInput->GetSimpleBitmap())
		return true;									// was already the requested type

	if(pInput->GetPixelFormat()!=CPixelFormats::GetNoFromD3DFormat(D3DFMT_A32B32G32R32F))
	{
		ImageObject *pRes=ConvertFormat(pInput,D3DFMT_A32B32G32R32F);

		if(!pRes)
			return false;

		if(!ProceedToNextPass(pInput,pRes))
			return false;
	}

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pInput->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwDepth!=1)		// volume textures are not handled
	{
		RCLogError("CImageCompiler::ConvertToFloat4 failed (volume textures are not supported)");
		return false;
	}

	if(dwSides!=1)		// cubemaps are not handled
	{
		RCLogError("CImageCompiler::ConvertToFloat4 failed (Cubemaps are not supported)");
		return false;
	}
	
	{
		ImageObject *pRes=new CFloat4Image(dwWidth,dwHeight);

		pRes->CopyPropertiesFrom(*pInput);

		char *pInputMem;
		uint32 dwInputPitch;

		if(!pInput->Lock(0,0,pInputMem,dwInputPitch))
		{
			RCLogError("CImageCompiler::Bump2Normal LockRect1 failed");
			return false;
		}

		CSimpleBitmap<Vec4> *pOutputBitmap = pRes->GetSimpleBitmap();			assert(pOutputBitmap);

		// convert to float4
		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		{
			float *src = (float *)&pInputMem[dwY*dwInputPitch];

			for(uint32 dwX=0;dwX<dwWidth;++dwX,src+=4)
				pOutputBitmap->Set(dwX,dwY,Vec4(src[0],src[1],src[2],src[3]));
		}

		pInput->Unlock(0);

		if(!ProceedToNextPass(pInput,pRes))
			return false;
	}

	return true;
}
*/


ImageObject *CImageCompiler::MinAlpha( ImageObject &rFlatARGB, const uint32 dwMinAlpha  ) const
{
	assert(dwMinAlpha>0);
	assert(dwMinAlpha<=255);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	rFlatARGB.GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	assert(dwMips==1);

	ImageObject *pRet = new CRAWImage(dwWidth,dwHeight,dwMips,CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8),IsFinalPixelFormatCompressed());

	pRet->CopyPropertiesFrom(rFlatARGB);

	char *pInputMem;
	uint32 dwInputPitch;

	if(!rFlatARGB.Lock(0,0,pInputMem,dwInputPitch))
	{
		RCLogError("CImageCompiler::MinAlpha Lock1 failed");
		assert(0);
		return 0;
	}

	char *pOutputMem;
	uint32 dwOutputPitch;

	if(!pRet->Lock(0,0,pOutputMem,dwOutputPitch))
	{
		rFlatARGB.Unlock(0);
		RCLogError("CImageCompiler::MinAlpha Lock2 failed");
		assert(0);
		return 0;
	}


	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	{
		uint32 *src = (uint32 *)&pInputMem[dwY*dwInputPitch];
		uint32 *dst = (uint32 *)&pOutputMem[dwY*dwOutputPitch];

		for(uint32 dwX=0;dwX<dwWidth;++dwX)
		{
			uint32 dwARGB = *src++;
			uint8 dwA = dwARGB >> 24;

			if(dwA<dwMinAlpha)
				dwA=dwMinAlpha;
		
			*dst++ = (dwARGB&0xffffff) | (dwA << 24);
		}
	}

	pRet->Unlock(0);
	rFlatARGB.Unlock(0);

	return pRet;
}


ImageObject *CImageCompiler::Bump2Normal( const CBumpProperties &rProperties, ImageObject *pFlatFloatARGB,
	const bool bApplyRangeAdjustment, const bool bValueFromAlpha )
{
	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pFlatFloatARGB->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwDepth!=1)		// volume textures are not handled
	{
		RCLogError("CImageCompiler::Bump2Normal failed (volume textures are not supported)");
		return 0;
	}

	if(dwSides!=1 || (m_Props.GetCubemap() && pFlatFloatARGB->IsCubemap()))		// cubemaps are not handled
	{
		RCLogError("CImageCompiler::Bump2Normal failed (Cubemaps are not supported)");
		return 0;
	}

	CSimpleBitmap<float>	IntermediateImage[2];		// 0=input, 1=blurred input

	if(!IntermediateImage[0].Alloc(dwWidth,dwHeight) || !IntermediateImage[1].Alloc(dwWidth,dwHeight))
	{
		RCLogError("CImageCompiler::Bump2Normal failed (not enough memory for IntermediateImage)");		
		return 0;
	}

	ImageObject *pRet=new CFloat4Image(dwWidth,dwHeight);

	if(!pRet)
	{
		RCLogError("CImageCompiler::Bump2Normal failed (not enough memory for CFloat4Image)");
		return 0;
	}

	pRet->CopyPropertiesFrom(*pFlatFloatARGB);

	char *pInputMem;
	uint32 dwInputPitch;

	if(!pFlatFloatARGB->Lock(0,0,pInputMem,dwInputPitch))
	{
		RCLogError("CImageCompiler::Bump2Normal LockRect1 failed");
//		p2DTexture->Release();
		return 0;
	}

	CSimpleBitmap<Vec4> *pOutputBitmap = pRet->GetSimpleBitmap();			assert(pOutputBitmap);


	// store input in IntermediateImage[0]
	if(bValueFromAlpha)
	{
		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		{
			float *src2 = (float *)&pInputMem[dwY*dwInputPitch];

			for(uint32 dwX=0;dwX<dwWidth;++dwX,src2+=4)
				IntermediateImage[0].Set(dwX,dwY,src2[3]);
		}
	}
	else
	{
		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		{
			float *src2 = (float *)&pInputMem[dwY*dwInputPitch];

			for(uint32 dwX=0;dwX<dwWidth;++dwX,src2+=4)
				IntermediateImage[0].Set(dwX,dwY,(src2[0]+src2[1]+src2[2])*(1.0f/3.0f));
		}
	}

	// calculate blurred/filtered version from the IntermediateImage[0] and put to IntermediateImage[1]
	{
		CSummedAreaFilterKernel kernel;
		CWeightFilterSet filterset;

		if(!kernel.IsValid())
			kernel.CreateFromGauss();

		filterset.Create(kernel,1.0f * (1.0f+rProperties.GetBumpBlurAmount()),true);		// bCenter=true : kernel center is in the middle of a pixel

		// get input from the red channel
		CInputProxy_Bump2Normal InputProxy1(IntermediateImage[0].GetPointer(),dwWidth*sizeof(float),dwWidth,dwHeight);

		for(uint32 dwY=0;dwY<dwHeight;++dwY)
		for(uint32 dwX=0;dwX<dwWidth;++dwX)
		{
			float fResult=0;

			filterset.GetBlockWithFilter(InputProxy1,dwX,dwY,fResult,false/*bCubemap*/);

			IntermediateImage[1].Set(dwX,dwY,fResult);
		}
	}

	// convert displace to normalmap
	{
		// get input from the alpha channel
		CInputProxy_Bump2Normal InputProxy2(IntermediateImage[1].GetPointer(),dwWidth*sizeof(float),dwWidth,dwHeight);

		float fInvStrength;		
		{
			const float fValue = rProperties.GetBumpStrengthAmount();
			fInvStrength = (fValue!=0.0f) ? 2.0f/fValue : 9999.9f;		// '2.0f' : samples are 2 pixel apart
		}

		for(int iY=0;iY<(int)dwHeight;++iY)
		{
			float *dest = (float *)pOutputBitmap->GetPointer(0,iY);

			for(int iX=0;iX<(int)dwWidth;++iX)
			{
				float fHorizSamples[2];

				InputProxy2.Get(iX-1,iY,fHorizSamples[0]);
				InputProxy2.Get(iX+1,iY,fHorizSamples[1]);

				Vec3 vHoriz=Vec3(fInvStrength,0,fHorizSamples[1]-fHorizSamples[0]);

				float fVertSamples[2];

				InputProxy2.Get(iX,iY-1,fVertSamples[0]);
				InputProxy2.Get(iX,iY+1,fVertSamples[1]);

				Vec3 vVert=Vec3(0,fInvStrength,fVertSamples[1]-fVertSamples[0]);	

				Vec3 vNormal = vHoriz.Cross(vVert);

				vNormal.NormalizeSafe();

				dest[0] = vNormal.x;
				dest[1] = vNormal.y;
				dest[2] = vNormal.z;

				dest+=4;
			}
		}
	}

	// convert from -1..1 to 0..1
	if(bApplyRangeAdjustment)
	{
		for(int iY=0;iY<(int)dwHeight;++iY)
		for(int iX=0;iX<(int)dwWidth;++iX)
		{
			Vec4 &v = pOutputBitmap->GetRef(iX,iY);

			// convert from -1..1 to 0..1
			v.x = v.x*0.5f+0.5f;
			v.y = v.y*0.5f+0.5f;
			v.z = v.z*0.5f+0.5f;
		}
	}

	// restore alpha channel
//	if(!bSetAlphaChannel)
	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	{
		float *src2 = (float *)&pInputMem[dwY*dwInputPitch];
		float *dst2 = (float *)pOutputBitmap->GetPointer(0,dwY);

		for(uint32 dwX=0;dwX<dwWidth;++dwX,src2+=4,dst2+=4)
			dst2[3]=src2[3];
	}

	pFlatFloatARGB->Unlock(0);

	return pRet;
}

ImageObject *CImageCompiler::CombineNormalMaps( ImageObject *pInOut, ImageObject *pBump ) const
{
	assert(pInOut);
	assert(pBump);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	uint32 dwBumpWidth,dwBumpHeight,dwBumpDepth,dwBumpSides,dwBumpMips;

	pBump->GetExtend(dwBumpWidth,dwBumpHeight,dwBumpDepth,dwBumpSides,dwBumpMips);
	pInOut->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwDepth!=1 || dwBumpDepth!=1)		// volume textures are not handled
	{
		RCLogError("CImageCompiler::CombineNormalMaps failed (volume textures are not supported)");
		return false;
	}

	if(dwSides!=1 || dwBumpSides!=1)		// cubemaps are not handled
	{
		RCLogError("CImageCompiler::CombineNormalMaps failed (Cubemaps are not supported)");
		return false;
	}
	
	char *pBumpMem,*pSrcMem;
	uint32 dwBumpPitch,dwSrcPitch;

	if(!pInOut->Lock(0,0,pSrcMem,dwSrcPitch))
	{
		RCLogError("CImageCompiler::CombineNormalMaps Lock1 failed (internal error)");
		return 0;
	}

	if(!pBump->Lock(0,0,pBumpMem,dwBumpPitch))
	{
		pInOut->Unlock(0);
		RCLogError("CImageCompiler::CombineNormalMaps Lock2 failed (internal error)");
		return 0;
	}

	CFloat4Image *pRet=new CFloat4Image(dwWidth,dwHeight);

	CSimpleBitmap<Vec4> *pOutBitmap = pRet->GetSimpleBitmap();

	pRet->CopyPropertiesFrom(*pInOut);

	Vec3 vHalf(128.0f/255.0f,128.0f/255.0f,128.0f/255.0f);

	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	{				
		char *pSrcLine = (char *)&pSrcMem[dwY*dwSrcPitch];

		uint32 dwBumpY = dwY%dwBumpHeight;

		char *pBumpLine = (char *)&pBumpMem[dwBumpY*dwBumpPitch];

		float *pSrcPix = (float *)pSrcLine;

		for(uint32 dwX=0;dwX<dwWidth;++dwX)
		{
			uint32 dwBumpX = dwX%dwBumpWidth;

			float *pBumpPix = (float *)&pBumpLine[dwBumpX*sizeof(float)*4];

			Vec3 vBumpNormal(pBumpPix[0],pBumpPix[1],pBumpPix[2]);								// 0..1

//			vBumpNormal.x = 0.8f;
//			vBumpNormal.y = 0.8f;
//			vBumpNormal.z = 1.0f;
		
			Vec3 vNormal(pSrcPix[0],pSrcPix[1],pSrcPix[2]);												// 0..1

//			vNormal.x = 0.5f;
//			vNormal.y = 0.5f;
//			vNormal.z = 1.0f;

			vBumpNormal = vBumpNormal*2.0f-Vec3(1.0f,1.0f,1.0f);									// -1..0..1
			vNormal = vNormal*2.0f-Vec3(1.0f,1.0f,1.0f);													// -1..0..1

			Matrix33 mTransform;
			
			mTransform.SetRotationV0V1(Vec3(0,0,1),vBumpNormal.GetNormalized());

			vNormal = mTransform*vNormal;

			// convert from -1..1 to 0..1
//			if(m_Props.m_bApplyRangeAdjustment)
				vNormal = vNormal*0.5f+vHalf;

			pOutBitmap->Set(dwX,dwY,Vec4(vNormal.x,vNormal.y,vNormal.z,0));			// RGBA

			pSrcPix+=4;	// jump over RGBA
		}
	}

	pInOut->Unlock(0);
	pBump->Unlock(0);

	return pRet;
}


ImageObject* CImageCompiler::ComputeLuminanceAndPutItIntoAlphaChannel( ImageObject *pIn ) const
{
	assert(pIn);
	assert(pIn->GetPixelFormat() == CPixelFormats::GetNoFromD3DFormat(D3DFMT_A32B32G32R32F));

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pIn->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(dwDepth!=1)		// volume textures are not handled
	{
		RCLogError("CImageCompiler::ComputeLuminanceAndPutItIntoAlphaChannel failed (volume textures are not supported)");
		return false;
	}

	if(dwSides!=1)		// cubemaps are not handled
	{
		RCLogError("CImageCompiler::ComputeLuminanceAndPutItIntoAlphaChannel failed (Cubemaps are not supported)");
		return false;
	}
	
	char* pSrcMem;
	uint32 dwSrcPitch;

	if(!pIn->Lock(0,0,pSrcMem,dwSrcPitch))
	{
		RCLogError("CImageCompiler::ComputeLuminanceAndPutItIntoAlphaChannel Lock failed (internal error)");
		return 0;
	}

	CFloat4Image* pRet=new CFloat4Image(dwWidth,dwHeight);
	CSimpleBitmap<Vec4>* pOutBitmap = pRet->GetSimpleBitmap();

	pRet->CopyPropertiesFrom(*pIn);

	for(uint32 dwY=0; dwY<dwHeight; ++dwY)
	{				
		const float *pSrcPix = (const float *)&pSrcMem[dwY*dwSrcPitch];

		for(uint32 dwX=0; dwX<dwWidth; ++dwX)
		{		
			const float r = pSrcPix[0];
			const float g = pSrcPix[1];
			const float b = pSrcPix[2];

			float luminance = r*0.39f + g*0.50f + b*0.11f;
			if(luminance < 0.0f)
			{
				luminance = 0.0f;
			}
			else if(luminance > 1.0f)
			{
				luminance = 1.0f;
			}
			
			pOutBitmap->Set(dwX, dwY, Vec4(r,g,b,luminance));

			pSrcPix+=4;	// jump over RGBA
		}
	}

	pIn->Unlock(0);

	return pRet;
}


// code from CryEngine
static bool IsThereDDNDIF( const char *szSrcName )
{
	char szOutName[256];
	const char *str = stristr(szSrcName, "_ddn");

	if(!str)
		return false;

	int nSize = str - szSrcName;

	// make sure to add only 'ddndif', if texture filename hasn't got ddndif
	if(!stristr(szSrcName, "_ddndif")) 
	{
		memcpy(szOutName, szSrcName, nSize);
		memcpy(&szOutName[nSize], "_ddndif", 7);
		strcpy(&szOutName[nSize+7],".tif");
	}
	else
		strcpy(szOutName, szSrcName);  

	FILE *fp = fopen(szOutName,"rb");

	if(!fp)
	{
		strcpy(&szOutName[nSize+7],".dds");
		fp = fopen(szOutName, "rb");
	}

	if(fp)
	{
		fclose(fp);
		return true;
	}

	return false;
}


void CImageCompiler::AutoPreset()
{
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//   auto preset
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	bool bAutoPreset=true;

	m_pCC->config->Get("autopreset",bAutoPreset);

	if(!bAutoPreset)
		return;

	// Rule: textures in Textures/Terrain/Detail that are no _DDN should become TerrainDiffuseHighPassed
	{
		char *szPattern="Textures\\Terrain\\Detail\\";										int iPattern=strlen(szPattern);
		string sPath = PathUtil::GetPath(m_pCC->getSourcePath());					int iPath=sPath.length();

		if(iPath>=iPattern)
		if(strnicmp(&sPath[iPath-iPattern],szPattern,iPattern)==0)
		if(!CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"ddn"))
		{
			string sCurrentPreset;

			m_pCC->config->Get("preset",sCurrentPreset);

			if(sCurrentPreset=="" || sCurrentPreset=="Diffuse_lowQ" || sCurrentPreset=="Diffuse_highQ")
			{
				m_Props.SetPreset("TerrainDiffuseHighPassed");
				SetPresetSettings();
				RCLog("/autopreset applied to '%s'",(const char*)m_pCC->sourceFileFinal);
			}
		}
	}
}



void CImageCompiler::AutoOptimize()
{
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//   auto optimize
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	bool bAutoOptimize=true;

	m_pCC->config->Get("autooptimize",bAutoOptimize);

	if(bAutoOptimize && m_Props.GetAutoOptimizeFile())
	{
		// if "autooptimize" per file is on (default) and it's on for global settings
		uint32 w,h,depth,sides,mips;
		m_pInputImage->GetExtend( w,h,depth,sides,mips );

		// then set not defined preset (means custom) depending on filename to a good default preset
		if( CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"ddn") || CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"bump") )
		{
			if( CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"bump") )
			{
				string sCurrentPreset;

				if(!m_pCC->config->Get("preset",sCurrentPreset) || sCurrentPreset=="")
				{
					// preset for _bump
					m_Props.SetPreset("Bump2Normalmap_lowQ");
					SetPresetSettings();
				}
			}
			else
			{
				// normalmap preset for the _ddn 
				string sCurrentPreset = m_Props.GetPreset();

				if(sCurrentPreset=="")
				{
					m_Props.SetPreset("Normalmap_lowQ");
					SetPresetSettings();
				}
			}
		}
		else
		{
			bool bIsPowerOfTwo;
			bool bIsCubemap;
			bIsPowerOfTwo = m_pInputImage->IsPowerOfTwo();
			if(m_Props.GetCubemap() && m_pInputImage->IsCubemap())
				bIsCubemap = true;
			else
				bIsCubemap = m_pInputImage->IsCubemap();

			if (bIsPowerOfTwo)
			{					
				if(m_Props.GetPreset()=="" && m_Props.m_Bump2Normal.GetBumpToNormalFilter()==0)
				{
					if(!bIsCubemap)
						m_Props.SetPreset("Diffuse_lowQ");
					else
					{
						m_Props.SetPreset("HDRCubemapRGBK_highQ");
						m_Props.SetDestPixelFormat(CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8), true);
						w /= 6;	// for one side

						if(w > 256)
						{
							if(m_Props.GetUserDialog())	// do an warning on huge cubemaps
							{
								::MessageBox(NULL, "The current cubemap is too large. This will take a lot of memory. Reduce the size of the cubemap.", "Cubemap dimensions are too big!", MB_OK | MB_ICONWARNING | MB_APPLMODAL | MB_SETFOREGROUND);
								exit(-1);
							}
							else
								RCLogError( "The cubemap %s is too large (%dx%d). This will take a lot of memory. Reduce the size of the cubemap.",(const char*)m_pCC->sourceFileFinal,w,h );
						}
					}
					SetPresetSettings();
				}
			}
			else
				RCLogError( "Texture %s is not power of two (%dx%d)",(const char*)m_pCC->sourceFileFinal,w,h );
		}

		// more high resolution texture should be scaled down
		if (w >= 1024 || h >= 1024 )
		{
			int iReduce = m_Props.GetReduceResolutionFile();

			//assert(m_Props.m_bHalfRes==false);		// only auto optimize should set this flag

			if(iReduce<=0)
				m_Props.m_bHalfRes=true;
		}
	}

}


bool CImageCompiler::RunWithProperties( bool inbSave, const char *szExtendFileName )
{
	LARGE_INTEGER TimeStarted;
	QueryPerformanceCounter(&TimeStarted);

	m_Progress.Start();
	m_bInternalPreview = (!inbSave && m_pImageUserDialog);

	AutoOptimize();
	AutoPreset();

	D3DFORMAT srcFormat = CPixelFormats::GetPixelFormatInfo(m_pInputImage->GetPixelFormat())->DxtNo;
	if (srcFormat == D3DFMT_DXT1 || srcFormat == D3DFMT_DXT2 || srcFormat == D3DFMT_DXT3 || srcFormat == D3DFMT_DXT5)
		return true;	// do not process

	if((m_pInputImage->GetImageFlags()&CImageExtensionHelper::EIF_Volumetexture))
		return true;	// do not process

	if(CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"ddn") 
		|| CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"bump") )
	{
		if(!IsFinalPixelFormatValidForNormalmaps())
			RCLogWarning("'%s' (ddn/bump) used not normal-map texture format (bad lighting on DX10 and console)",(const char*)m_pCC->sourceFileFinal);
	}

	string sOutputFileName=m_pCC->getOutputPath();

	// add a string to the filename (before the extension)
	if(szExtendFileName)
	{
		string ext=PathHelpers::FindExtension(sOutputFileName);

		sOutputFileName = PathHelpers::RemoveExtension(sOutputFileName);

		sOutputFileName += szExtendFileName+string(".")+ext;
	}

	if(!m_bInitialized)
		Init( m_pCC->pRC->GetEmptyWindow() );

	if(m_pFinalImage && m_pFinalImage != m_pInputImage) delete m_pFinalImage;	m_pFinalImage = NULL;

	if(inbSave)
	{
		bool bNoOutput = m_pCC->config->HasKey("nooutput");		// user requests /nooutput ?

		if(bNoOutput)
		{
			RCLogError("CImageCompiler::Process save Ext:'%s' /nooutput was specified",m_pCC->sourceFileFinalExtension);
			inbSave=false;
		}
	}

	if(m_Props.GetCubemap())	// check for cubemap
	if(!m_pInputImage->IsCubemap())
	{
		// prevent non 6:1 textures from being processed as cubemap - RC doesn't like them
		RCLogError("CImageCompiler::Process cubemap extend is not 6:1: %dx%d", m_pInputImage->GetWidth(0), m_pInputImage->GetHeight(0));
		return false;
	}

	const bool bIsImageCubemap = m_Props.GetCubemap() && m_pInputImage->IsCubemap();

	if(m_Props.GetPowOf2())
	{
		if(!m_pInputImage->IsPowerOfTwo())
		{	
			// prevent non pow-of-two textures from being processed - engine doesn't like them
			RCLogError("CImageCompiler::Process image(cubemap side) extend is not power-of-two: %dx%d (width and height needs to be one of ..,16,32,64,128,256,..)", m_pInputImage->GetWidth(0)/6, m_pInputImage->GetHeight(0));
			return false;
		}
	}

	ImageObject *pCurrentImage = m_pInputImage;

	// initial checks for source image
	if(m_pImageUserDialog)
	{
		ValidateSourceImage(pCurrentImage);
	}

	// multi pass
	{
		{
			int colorChart = 0;
			if (m_pCC->config->Get("colorchart", colorChart) && colorChart)
			{
				IColorChart* pCC = Create3dLutColorChart();
				if (!pCC)
					return false;

				if (!pCC->GenerateFromInput(m_pInputImage))
					pCC->GenerateDefault();

				ImageObject* pRes = pCC->GenerateChartImage();
				assert(pRes);
				if (!ProceedToNextPass(pCurrentImage, pRes))
					return false;

				SAFE_RELEASE(pCC);
			}
		}

		int iMinAlpha=m_Props.GetMinAlpha();

		// minalpha
		if(iMinAlpha>0)	// 0..255
		{
			ImageObject *pIntermediateARGB = MinAlpha(*pCurrentImage,min(iMinAlpha,255));
			const bool needDelete = pCurrentImage != pIntermediateARGB;

			if(!ProceedToNextPass(pCurrentImage,pIntermediateARGB))
			{
				if(needDelete) delete pIntermediateARGB;
				return false;
			}
		}

		// Bump2Normal postprocess
		if(m_Props.m_Bump2Normal.GetBumpToNormalFilter())			// Bump2Normal postprocess
		{
			if(ImageObject *pIntermediateFloatARGB=ConvertFormat(pCurrentImage,D3DFMT_A32B32G32R32F))
			{
				const bool needDelete = pCurrentImage != pIntermediateFloatARGB;
				ImageObject *pRes=Bump2Normal(m_Props.m_Bump2Normal,pIntermediateFloatARGB,m_Props.m_bApplyRangeAdjustment,false);	// value from RGB luminance

				if(!ProceedToNextPass(pCurrentImage,pRes))
				{
					if(needDelete) delete pIntermediateFloatARGB;
					return false;
				}
			}
			else RCLogError("CImageCompiler::Bump2Normal ConvertFormat1 failed");
		}

		if(m_Props.m_AlphaAsBump.GetBumpToNormalFilter())			// Alpha As Bump
		{
			if(ImageObject *pIntermediateFloatARGB=ConvertFormat(pCurrentImage,D3DFMT_A32B32G32R32F))
			{
				const bool needDelete = pCurrentImage != pIntermediateFloatARGB;
				ImageObject *pRes=Bump2Normal(m_Props.m_AlphaAsBump,pIntermediateFloatARGB,true,true);		// value from alpha
				if(!pRes)
				{
					if(needDelete) delete pIntermediateFloatARGB;
					return false;
				}

				m_Props.m_bApplyRangeAdjustment=true;

				ImageObject *pCombined = CombineNormalMaps(pIntermediateFloatARGB,pRes);		// both input need to be in range 0..1

				delete pRes;pRes=0;
				if(needDelete) delete pIntermediateFloatARGB;
				pIntermediateFloatARGB=0;

				if(!ProceedToNextPass(pCurrentImage,pCombined))
				{
					delete pCombined;pCombined=0;
					return false;
				}
			}
			else RCLogError("CImageCompiler::Bump2Normal ConvertFormat2 failed");
		}


		// Compute luminance and put it into alpha channel
		{
			assert(pCurrentImage);

			int lumintoalpha = 0;
			if (m_pCC->config->Get("lumintoalpha", lumintoalpha) && lumintoalpha)
			{
				ImageObject *pProcessed = 0;
				ImageObject *pIntermediateFloatARGB = ConvertFormat(pCurrentImage,D3DFMT_A32B32G32R32F);

				if(pIntermediateFloatARGB)
				{
					pProcessed = ComputeLuminanceAndPutItIntoAlphaChannel(pIntermediateFloatARGB);
				}

				if(pCurrentImage != pIntermediateFloatARGB)
				{
					delete pIntermediateFloatARGB;
					pIntermediateFloatARGB = 0;
				}

				if(pProcessed)
				{
					if(!ProceedToNextPass(pCurrentImage,pProcessed))
					{
						delete pProcessed;
						pProcessed = 0;
					}
				}

				if(!pProcessed)
				{
					RCLogError("CImageCompiler::RunWithProperties LuminanceIntoAlpha failed");
					return false;
				}

				m_bInputUsesAlpha = true;
			}
		}

		// generate mipmaps (needed for mipmaps, reduce resolution and average color computation)
 		if(m_Props.GetMipMaps() || m_Props.GetReduceResolution() || m_Props.m_bAverageColor)		
		{
			float fGamma = 1.0f;

			if(m_Props.GetGammaCorrectedMips())
			{
				fGamma = 2.2f; 
				
				m_pCC->config->Get("globalgamma",fGamma);
			}

			ImageObject *pRes;
			if(!bIsImageCubemap)
				pRes = CreateMipMaps(pCurrentImage,m_Props.GetReduceResolution(),!m_Props.GetMipMaps(),m_Props.GetMipRenormalize(),fGamma);
			else
			{
				SCubemapFilterParams params;
				params.FilterType = m_Props.GetCubemapFilterType();
				params.BaseFilterAngle = m_Props.GetCubemapFilterAngle();
				params.InitialMipAngle = m_Props.GetCubemapMipFilterAngle();
				params.MipAnglePerLevelScale = m_Props.GetCubemapMipFilterSlope();
				params.FixupWidth = m_Props.GetCubemapEdgeFixupWidth();
				params.FixupType = params.FixupWidth > 0 ? CP_FIXUP_PULL_HERMITE : CP_FIXUP_NONE;
				pRes = CreateCubemapMipMaps(pCurrentImage, params, m_Props.GetReduceResolution(), fGamma);
			}

			if(!ProceedToNextPass(pCurrentImage,pRes))
				return false;
		}

		// high pass subtract mip level is subtracted when applying the [cheap] high pass filter - this prepares assets to allow this in the shader
		if(m_Props.GetHighPass()>0)
		{
			ImageObject *pRes = CreateHighPass(pCurrentImage,m_Props.GetHighPass());

			if(!ProceedToNextPass(pCurrentImage,pRes))
				return false;
		}

		// make gamma correction with respect to XBox 360 linear gamma approximation
		if(m_Props.GetSRGBLookup() && m_pCC->platform == ePlatform_X360)
		{
			ImageObject *pRes=ConvertGammaToXBox360(pCurrentImage);
			assert(pRes);
			if(!ProceedToNextPass(pCurrentImage,pRes))
				return false;
		}

		// convert HDR texture to RGBK
		if(m_Props.GetRGBKCompression())
		{
			if(ImageObject *pIntermediateFloatARGB=ConvertFormat(pCurrentImage,D3DFMT_A32B32G32R32F))
			{
				const bool needDelete = pCurrentImage != pIntermediateFloatARGB;
				ImageObject *pRes=CompressToRGBK(pIntermediateFloatARGB, m_Props.GetRGBKMultiplier());
				assert(pRes);
				if(!ProceedToNextPass(pCurrentImage,pRes))
				{
					if(needDelete) delete pIntermediateFloatARGB;
					return false;
				}
				m_bInputUsesAlpha = true;
			}
		}

		int iDestPixelFormat = m_Props.GetDestPixelFormat(m_bInputUsesAlpha,(m_bInternalPreview && m_pCC->config->HasKey("previewformat")));
/*
		if(IsDDNDIFProcessing(pCurrentImage))		// DDNDIF special handling - greyscale images become accessibility, not greyscale become converted to accessibility
		{
			if(ImageObject *pFloatARGB=ConvertFormat(pCurrentImage,D3DFMT_A32B32G32R32F))
			{
				RCLog("CImageCompiler::Process DDNDIF not greyscale becomes converted to accessibility (shader requires data in that form)");
				DDNDIFProcessing(pFloatARGB);

				if(!ProceedToNextPass(pCurrentImage,pFloatARGB))
					return false;
			}
			else RCLogError("CImageCompiler::DDNDIF ConvertFormat3 failed");
		}
*/
		// compress result
		if(iDestPixelFormat!=-1)
		{
			D3DFORMAT format = g_pixelformats[iDestPixelFormat].DxtNo;

			if(m_pCC->config->HasKey("outputuncompressed"))
			{
				switch(format)
				{
				case D3DFMT_DXT1:
					format = D3DFMT_X8R8G8B8;
					break;
				case D3DFMT_DXT3:
				case D3DFMT_DXT5:
					format = D3DFMT_A8R8G8B8;
					break;
				default:
					break;
				}
			}

			// the following formats allow the conversion without Quality loss
			if(m_Props.GetAutoDetectLuminanceOnly())
			if(format==D3DFMT_DXT1 || format==D3DFMT_X8R8G8B8 || format==D3DFMT_R8G8B8 || format==D3DFMT_R5G6B5 || format==D3DFMT_X1R5G5B5 || format==D3DFMT_X4R4G4B4 || format==D3DFMT_R3G3B2)
			if(IsPerfectGreyscale(pCurrentImage))
			{
//				RCLog("CImageCompiler::Process Luminance only detected - using L8");
				format=D3DFMT_L8;

				uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

				pCurrentImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);
				assert(dwMips>0);
			}

			assert(pCurrentImage);

			if(m_Props.GetGlobalCompressor()==CImageProperties::eCGDirectX)
			{
				// if it's not yet a DirectX object we need to convert it back again
				if(!pCurrentImage->GetDXTex())
				{
					ImageObject *pRes = ConvertFormat(pCurrentImage,D3DFMT_A32B32G32R32F);

					if(!ProceedToNextPass(pCurrentImage,pRes))
						return false;
				}
			}

			ImageObject *pRes=ConvertFormat(pCurrentImage,format);
 
			if(!ProceedToNextPass(pCurrentImage,pRes))
				return false;
		}

		// convert back to CRAWImage to save
		if(inbSave)
		if(!pCurrentImage->GetCRAWImage())
		{
			CRAWImage *pRes=CopyToNewCRAWImage(pCurrentImage);
		
			assert(pRes);

			if(!ProceedToNextPass(pCurrentImage,pRes))
				return false;
		}

		// hard coded behavior for now - drop top mip for attached alpha channel (POM/offset bump doesn't require this resolution and we save memory)
		if(ImageObject *pAttachedImage = pCurrentImage->GetAttachedImage())
		{
			CRAWImage *pAttachedRAWImage = pAttachedImage->GetCRAWImage();			assert(pAttachedRAWImage);

			pAttachedRAWImage->DropTopMip();
		}
	}

	if(CSuffixUtil::CheckForSuffix(m_pCC->sourceFileFinal,"ddn"))
	{
		if(!IsThereDDNDIF(m_pCC->getSourcePath()))
		{
			pCurrentImage->SetImageFlags(pCurrentImage->GetImageFlags()|CImageExtensionHelper::EIF_FileSingle);
			RCLog("tagged asset as FileSingle");
		}
	}

	if(pCurrentImage->GetAttachedImage())
		pCurrentImage->SetImageFlags(pCurrentImage->GetImageFlags() | CImageExtensionHelper::EIF_AttachedAlpha);
	if(pCurrentImage->IsCubemap() && m_Props.GetCubemap())
		pCurrentImage->SetImageFlags(pCurrentImage->GetImageFlags() | CImageExtensionHelper::EIF_Cubemap);

	if(pCurrentImage)
	{
		int emulate3Dc = 0;
		m_pCC->config->Get("Emulate3Dc",emulate3Dc);

		if(emulate3Dc==1)
		{
			Convert3DcToDXT5_InPlace(pCurrentImage);
		}
	}

	if(pCurrentImage)
	{
		int iSwizzleTex=0;

		m_pCC->config->Get("SwizzleTex",iSwizzleTex);
		assert(iSwizzleTex == 0 || iSwizzleTex == 1);

		if(iSwizzleTex == 1)
			PreSwizzleTexturePS3like_InPlace(pCurrentImage);
	}

	m_pFinalImage=pCurrentImage;

	if(inbSave)
	{
//		RCLog("Saving ...");
		RCLog("");

		bool bTIF = m_Props.GetGlobalCompressor()==CImageProperties::eCGTIF;

		// destination
		if(!SaveOutput(m_pFinalImage,bTIF?"    TIF":"    DDS",sOutputFileName))
		{
			RCLogError("CImageCompiler::Process save '%s' failed",(const char*)sOutputFileName );
			return false;
		}

		// fallback
		if(m_pTextureFallbackSystem && m_pTextureFallbackSystem->IsActivated() && m_pFinalImage)
		{
			if(!CreateTextureFallback(m_pFinalImage,2,7))
				RCLogError("CImageCompiler::CreateTextureFallback failed (internal error)");
		}
	}

	{
		LARGE_INTEGER TimeEnded,Frequency;
		
		QueryPerformanceCounter(&TimeEnded);

		m_fTimeProfiled=0;

		if(QueryPerformanceFrequency(&Frequency))
			m_fTimeProfiled = (float)((double)(TimeEnded.QuadPart-TimeStarted.QuadPart)/(double)Frequency.QuadPart);
	}

	// for reflection cubemaps - generate the second diffuse cubemap
	if(m_pInputImage->IsCubemap() && !m_Props.GetUserDialog())
	{
		bool bAutoOptimize=true;
		m_pCC->config->Get("autooptimize",bAutoOptimize);
		if(!szExtendFileName || strcmp(szExtendFileName, "_diff") != 0)
		{
			m_pCC->config->Set(eCP_PriorityHighest, "autooptimize", "0");
			m_Props.SetPreset("HDRDiffuseCubemapRGBK_highQ");
			m_pCC->presets->SetConfig(eCP_PriorityHighest,"HDRDiffuseCubemapRGBK_highQ",m_pCC->config);
			SetPresetSettings();
			ClearInputImageFlags();
			RunWithProperties(inbSave, "_diff");
		}
	}

	m_Progress.Finish();

	return true;
}


void CImageCompiler::DDNDIFProcessing( ImageObject *pFloatARGB ) const
{
	// convert DDNDIF (unoccluded area direction to accessibility map)
	char *pInputMem;
	uint32 dwInputPitch;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pFloatARGB->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
	{
		uint32 dwLocalWidth = pFloatARGB->GetWidth(dwMip);			// we get error on NVidia with this (assumes input is 4x4 as well)
		uint32 dwLocalHeight = pFloatARGB->GetHeight(dwMip);

		if(!pFloatARGB->Lock(0,dwMip,pInputMem,dwInputPitch))
		{
			RCLogError("CImageCompiler::DDNDIFProcessing LockRect mip=%d failed",dwMip);
			return;
		}

		const float fHalf=128.0f/255.0f;

		for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
		{
			float *src2 = (float *)&pInputMem[dwY*dwInputPitch];

			for(uint32 dwX=0;dwX<dwLocalWidth;++dwX,src2+=4)
			{
				float x = src2[0]-fHalf;
				float y = src2[1]-fHalf;
				float z = src2[2]-fHalf;

				float fLength = min(sqrtf(x*x+y*y+z*z)*2.0f,1.0f);

				src2[0]=fLength;
				src2[1]=fLength;
				src2[2]=fLength;
			}
		}

		pFloatARGB->Unlock(dwMip);
	}
}


bool CImageCompiler::CreateTextureFallback( ImageObject *pImage, const uint32 dwReduceMips, const uint32 dwMaxMips )
{
	assert(pImage);

	if(dwMaxMips>=8)
	{
		assert(0);			// we need this - to use small texture header
		return false;
	}

	std::vector<unsigned char> Buffer;

	Buffer.reserve(10*1024);


	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if((dwWidth>1 || dwHeight>1)		// if there is an image
	&& dwMips==1)										// but only one mip
		return true;									// then there is no need to store a fallback - only mipped texture should have fallback

	uint32 dwFinalDestMips = (uint32)max(1,(int)dwMips-(int)dwReduceMips);

	dwFinalDestMips = min(dwFinalDestMips,dwMaxMips);

	int iGranularity = 1;
	
	if(CPixelFormats::GetPixelFormatInfo(pImage->GetPixelFormat())->bCompressed)
		iGranularity=4;

	uint32 dwFinalWidth = dwWidth>>(dwMips-dwFinalDestMips);							
	uint32 dwFinalHeight = dwHeight>>(dwMips-dwFinalDestMips);

	if(dwFinalWidth<1)dwFinalWidth=1;
	if(dwFinalHeight<1)dwFinalHeight=1;

	dwFinalWidth = ((dwFinalWidth+iGranularity-1)/iGranularity)*iGranularity;
	dwFinalHeight = ((dwFinalHeight+iGranularity-1)/iGranularity)*iGranularity;

	// ------------------------------------------------------------------------




	// DebugDump - only works if Entries contain DDSURFACEDESC2 header

	// small texture header 
	{
		uint32 dwBitsPerPixel;

		SPixelFormats *pPixelFormat = CPixelFormats::GetPixelFormatInfo(pImage->GetPixelFormat());

		uint32 dwDXPixelFormat = pPixelFormat->DxtNo;
		dwBitsPerPixel = (uint8)pPixelFormat->iBitsPerPixel;
		uint8 cFlags=0;

		if(pImage->HasAlpha())
			cFlags|=1;

		if(dwFinalWidth>=256 || dwFinalHeight>=256)
			return false;

		Buffer.push_back((uint8)(dwWidth>>8));					// width high byte
    Buffer.push_back((uint8)(dwWidth>>0));					// width low byte
    Buffer.push_back((uint8)(dwHeight>>8));					// height high byte
    Buffer.push_back((uint8)(dwHeight>>0));					// height low byte
    Buffer.push_back((uint8)dwMips);							  // mips
		Buffer.push_back((uint8)dwFinalDestMips);				// mips
		Buffer.push_back((uint8)dwBitsPerPixel);				// bpp
		Buffer.push_back((uint8)(dwDXPixelFormat>>0));	// dxformat high byte
		Buffer.push_back((uint8)(dwDXPixelFormat>>8));	// dxformat
		Buffer.push_back((uint8)(dwDXPixelFormat>>16));	// dxformat
		Buffer.push_back((uint8)(dwDXPixelFormat>>24));	// dxformat low byte
		Buffer.push_back((uint8)cFlags);								// flags bit0:alpha, 1:don't stream
	}

/*
	// DDS header 80 bytes
	{
		DDSURFACEDESC2 surf;

		CPixelFormats::BuildSurfaceDesc(pImage,surf);

		surf.dwWidth=dwFinalWidth;
		surf.dwHeight=dwFinalHeight;
		surf.dwMipMapCount=dwFinalDestMips;

		Buffer.resize(sizeof(DDSURFACEDESC2));
		memcpy(&Buffer[0],&surf,sizeof(DDSURFACEDESC2));
	}
*/



	// ------------------------------------------------------------------------

	for(uint32 dwMipLevel=dwMips-dwFinalDestMips;dwMipLevel<dwMips;++dwMipLevel)
	{
		char *pMem;
		uint32 dwPitch;

		if(!pImage->Lock(0,dwMipLevel,pMem,dwPitch))
		{
			assert(0);
			RCLogError("CTextureFallbackSystem: FATAL ERROR (lock failed)");
			break;
		}

//		uint32 dwMipWidth = pImage->GetWidth(dwMipLevel);
		uint32 dwMipHeight = pImage->GetHeight(dwMipLevel);

		if(CPixelFormats::GetPixelFormatInfo(pImage->GetPixelFormat())->bCompressed)
			dwMipHeight = max((uint32)1,((dwMipHeight+3)/4));

		uint32 dwOldBufferSize = (uint32)Buffer.size();
		uint32 dwBufferSize = dwPitch*dwMipHeight;

//		char str[256];

//		sprintf(str,"Fallback MipsSize: pitch:%d size:%d\n",dwPitch,dwBufferSize);
//		OutputDebugString(str);

		Buffer.resize(dwOldBufferSize+dwBufferSize);

		memcpy(&Buffer[dwOldBufferSize],pMem,dwBufferSize);

		pImage->Unlock(dwMipLevel);
	}

	uint32 dwBufferSize = (uint32)Buffer.size();

	if(dwBufferSize>128*128*4 * 4/3 + 100)		// 128x128 a 4 bytes with mips + extra
	{
		char str[256];

		sprintf_s(str,sizeof(str),"WARNING: the entry seems to be to big (%d bytes)\n",dwBufferSize);
		OutputDebugString(str);
		assert(0);
	}

//	if(!Buffer.empty())
	if(m_pTextureFallbackSystem && !m_pTextureFallbackSystem->AddEntry(m_pCC->getOutputPath(),&Buffer[0],dwBufferSize))
	{
		assert(0);
		RCLogError("CTextureFallbackSystem: FATAL ERROR (maybe duplicate name? - rename asset?) '%s'",m_pCC->getOutputPath());
	}
	else if (m_pTextureFallbackSystem)
	{
		uint64 crc = m_pTextureFallbackSystem->ComputeCRC(m_pCC->getOutputPath());

		RCLog("   fallback texture: CRC:%x%x %d bytes",(uint32)(crc>>32),(uint32)crc,(uint32)Buffer.size());
	}

	return true;
}


void CImageCompiler::ClearInputImageFlags()
{
	m_bInputUsesAlpha=false;
	m_bInputUsesDenormalizedNormals=false;
}


string CImageCompiler::GetInfoStringUI( const bool inbOrig )
{
	int iNo = m_iOrigPixelFormatNo;

	if(!inbOrig)
	{
		if(m_pFinalImage)
			iNo = m_pFinalImage->GetPixelFormat();
		 else
			iNo = -1;

		if(iNo==-1)																										// format is recognized
			return "Format not recognized";
	}

	const char *szAlpha = iNo==-1 ? "*" : g_pixelformats[iNo].szAlpha;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	char str[256];
	uint32 dwMem = 0;

	if(inbOrig)
	{
		dwMem = CalcTextureMemory(inbOrig?m_pInputImage:m_pFinalImage);

		sprintf(
			str,
			"%dx%d Fmt:%s Alpha:%s\nMem:%.1fkB",
			dwWidth,dwHeight,CPixelFormats::GetPixelFormatName(iNo),szAlpha,dwMem/1024.0f);
	}
	else 
	{
		assert(m_bInternalPreview);
		//int colorChart = 0;
		//if (m_pCC->config->Get("colorchart", colorChart) && colorChart)
		{
			uint32 dwWidthFinal,dwHeightFinal,dwDepthFinal,dwSidesFinal,dwMipsFinal;
			m_pFinalImage->GetExtend(dwWidthFinal,dwHeightFinal,dwDepthFinal,dwSidesFinal,dwMipsFinal);

			assert(m_bInternalPreview);

			const int resReduce = m_pImageUserDialog->GetPreviewReduceResolution();

			dwWidth = dwWidthFinal<<resReduce;
			dwHeight = dwHeightFinal<<resReduce;

			dwMem = CalcTextureMemory(m_pFinalImage, (uint32)pow(2,(float)resReduce));
		}

		const uint32 dwReduce = m_Props.GetReduceResolution();

		const bool bCubemap = m_Props.GetCubemap() && m_pFinalImage->IsCubemap();

		uint32 dwNumMips=max((int)_CalcMipCount(dwWidth,dwHeight,IsFinalPixelFormatCompressed(),bCubemap),1);
		if(!m_Props.GetMipMaps())
		{
			dwNumMips=1;
		}

		char *szAttachedStr="";

		if(m_pFinalImage->GetAttachedImage())
		{
			szAlpha = g_pixelformats[m_pFinalImage->GetAttachedImage()->GetPixelFormat()].szName;
			szAttachedStr="+";
		}

		const uint32 dwImageFlags = m_pFinalImage->GetImageFlags();

		sprintf(
			str,
			"%dx%d Mips:%d Fmt:%s Alpha:%s%s\nMem:%.1fkB reduce:%d Flags:%p",
			dwWidth,dwHeight,dwNumMips,CPixelFormats::GetPixelFormatName(iNo),szAttachedStr,szAlpha,dwMem/1024.0f,dwReduce,dwImageFlags);
	}

	return str;
}




string CImageCompiler::GetDestInfoString()
{
	int iNo = -1;

	if(m_pFinalImage)
		iNo = m_pFinalImage->GetPixelFormat();

	char str[1024];

	uint32 dwMem=CalcTextureMemory(m_pFinalImage);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	const char *szAlpha = iNo==-1 ? "*" : g_pixelformats[iNo].szAlpha;

	uint32 dwReduce = m_Props.GetReduceResolution();

	uint32 dwW=max(dwWidth>>dwReduce,(uint32)1);
	uint32 dwH=max(dwHeight>>dwReduce,(uint32)1);
	bool bCubemap = m_Props.GetCubemap() && m_pInputImage->IsCubemap();
	uint32 dwNumMips=max((int)_CalcMipCount(dwWidth,dwHeight,IsFinalPixelFormatCompressed(),bCubemap)-(int)dwReduce,1);

	if(!m_Props.GetMipMaps())
		dwNumMips=1;

	uint32 dwAttachedMem=0;
	
	if(m_pFinalImage)
	if(m_pFinalImage->GetAttachedImage())
		dwAttachedMem = CalcTextureMemory(m_pFinalImage->GetAttachedImage());
	
	sprintf(str,"%d"			// Width
							"\t%d"		// Height
							"\t%s"		// Alpha
							"\t%d"		// Mips
							"\t%.1f"	// MemInKBMem
							"\t%s"		// Format
							"\t%d"		// reduce
							"\t%.1f",	// AttachedMemInKB
		dwW,dwH,szAlpha,dwNumMips,dwMem/1024.0f,CPixelFormats::GetPixelFormatName(iNo),dwReduce,dwAttachedMem/1024.0f);

	return str;
}

/*
uint32 CImageCompiler::CalcTextureMemory( const bool inbOrig ) const
{
	int iSum=0;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	int iNo = -1;

	if(!inbOrig)
	{
		if(m_pFinalImage)
			iNo = m_pFinalImage->GetPixelFormat();
	}
	else iNo = m_iOrigPixelFormatNo;

	if(iNo==-1)																										// format is recognized
		return 0;

	int iBpp=g_pixelformats[iNo].iBitsPerPixel;
	int iMipmaps=1;
	
	if(!inbOrig && m_Props.GetMipmaps())
		iMipmaps=_CalcMipCount(dwWidth,dwHeight);

	int iIgnoreTopMips=0;
	
	if(!inbOrig)
		iIgnoreTopMips=(int)m_Props.GetReduceResolution();

	for(int i=0;i<iMipmaps;i++)
	{
		if(iIgnoreTopMips)iIgnoreTopMips--;
		else
		{
			if(g_pixelformats[iNo].bCompressed)
			{
				iSum+=(max((uint32)1,dwWidth/4) * max((uint32)1,dwHeight/4) *4*4 * iBpp)/8;			// from DX SDK:  max(1,width ÷ 4)x max(1,height ÷ 4)x 8 (DXT1) or 16 (DXT2-5)
			}
			else
			{
				iSum+=(dwWidth*dwHeight*iBpp)/8;
			}
		}

		dwWidth=(dwWidth+1)/2; dwHeight=(dwHeight+1)/2;
	}

	return(iSum);
}
*/


uint32 CImageCompiler::CalcTextureMemory( const ImageObject *pImage, const uint32 dwImageScale)
{
	if(!pImage)
		return 0;

	uint32 dwRet=0;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	dwWidth *= dwImageScale;
	dwHeight *= dwImageScale;

	int iNo = pImage->GetPixelFormat();

	if(iNo==-1)																										// format is not recognized
		return 0;

	int iBpp=g_pixelformats[iNo].iBitsPerPixel;

	for(int dwMip=0;dwMip<dwMips;++dwMip)
	{
		if(g_pixelformats[iNo].bCompressed)
			dwRet+=(max((uint32)1,dwWidth/4) * max((uint32)1,dwHeight/4) *4*4 * iBpp)/8;			// from DX SDK:  max(1,width ÷ 4)x max(1,height ÷ 4)x 8 (DXT1) or 16 (DXT2-5)
		 else
			dwRet+=(dwWidth*dwHeight*iBpp)/8;

		dwWidth=(dwWidth+1)/2; dwHeight=(dwHeight+1)/2;
	}

	if(pImage->GetAttachedImage())
		dwRet += CalcTextureMemory(pImage->GetAttachedImage());

	return dwRet;
}


struct my_vertex
{
    FLOAT x, y, z,rhw;
    FLOAT u1, v1;
};

// Define corresponding FVF macro.
#define D3D8T_CUSTOMVERTEX ( D3DFVF_XYZRHW|D3DFVF_TEX1|D3DFVF_TEXCOORDSIZE2(0))


static void PrintInPreview( HWND inHWND, RECT &inRect, const char *inszTxt1, const char *inszTxt2=0 )
{
	HDC hdc=GetDC(inHWND);
	FillRect(hdc,&inRect,GetSysColorBrush(COLOR_3DFACE));
	SetBkMode(hdc,TRANSPARENT);
	SetTextAlign(hdc,TA_CENTER);
	ExtTextOut(hdc,(inRect.left+inRect.right)/2,(inRect.top+inRect.bottom)/2-8,ETO_CLIPPED,&inRect,inszTxt1,(int)strlen(inszTxt1),0);
	if(inszTxt2)ExtTextOut(hdc,(inRect.left+inRect.right)/2,(inRect.top+inRect.bottom)/2+8,ETO_CLIPPED,&inRect,inszTxt2,(int)strlen(inszTxt2),0);
	ReleaseDC(inHWND,hdc);
}



bool CImageCompiler::BlitTo( HWND inHWND, RECT &inRect, const float infOffsetX, const float infOffsetY, const int iniScale64, const bool inbOrig )
{
	if(!InitDirectX())
		return false;

	// error message is necessary
	if(inbOrig)
	{
		if(!m_pInputImage)
		{
			PrintInPreview(inHWND,inRect,"Load failed","(format not supported by driver?)");
			return(true);
		}
	}
	else
	{
		if(m_pFinalImage==0)
		{
			PrintInPreview(inHWND,inRect,"Conversion failed","(format not supported by driver?)");
			return true;
		}
	}

	HRESULT hr;
	IDirect3DTexture9 *pTexture=0;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	bool previewStretched = true;
	m_pCC->config->Get("previewstretched", previewStretched);
	if (inbOrig || previewStretched)
		m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);
	else
		m_pFinalImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if(m_Props.GetPowOf2())
	{
		if(!m_pInputImage->IsPowerOfTwo())
		{
			char str[256];

			sprintf(str,"(%dx%d)",dwWidth,dwHeight);
			PrintInPreview(inHWND,inRect,"Image size is not power of two",str);
			return(true);
		}
	}

	// convert to blitable format
	ImageObject *pBaseTexture = inbOrig ? m_pInputImage : m_pFinalImage;
	
	assert(pBaseTexture);			// this was checked earlier

	ImageObject *pimgNew = ConvertFormat(pBaseTexture,D3DFMT_A8R8G8B8);

	if(!pimgNew)
		return false;

	pimgNew->CopyPropertiesFrom(*pBaseTexture);

	EPreviewMode previewMode = m_Props.m_ePreviewMode;
	if(inbOrig)
	{
		if(m_Props.m_ePreviewMode == ePM_RGBmulA)
			previewMode = ePM_RGB;	// RGBK has no alpha channel
	}
	ImageObject *pRes = GenerateSpecialPreviewToARGB(pimgNew,previewMode);

	pRes->GetDXTexI(pTexture);

	if(pimgNew && pimgNew!=pBaseTexture)
	{
		delete pimgNew;
		pimgNew=0; 
	}

	if(!pTexture)
	{
		delete pRes;
		return false;
	}

	int iWidth=inRect.right-inRect.left,iHeight=inRect.bottom-inRect.top;

	int iW=((iWidth)*64)/iniScale64;	
	int iH=((iHeight)*64)/iniScale64;

	int iX=(int)(infOffsetX*dwWidth*1.0f);
	int iY=(int)(infOffsetY*dwHeight*1.0f);

	m_pd3ddev->BeginScene();

	m_pd3ddev->Clear(0,0,D3DCLEAR_TARGET,0xc0c0c0,0.0f,0);

	for(uint32 dwY=0;dwY<(uint32)iHeight/8;++dwY)
	for(uint32 dwX=0;dwX<(uint32)iWidth/8;++dwX)
	{
		if(((dwY+dwX)%2)==0)
		{
			D3DRECT rect;

			rect.x1=dwX*8;
			rect.y1=dwY*8;
			rect.x2=dwX*8+8;
			rect.y2=dwY*8+8;

			m_pd3ddev->Clear(1,&rect,D3DCLEAR_TARGET,0xf0f0f0,0.0f,0);
		}
	}

	hr=m_pd3ddev->SetTexture(0,pTexture);

	if(FAILED(hr))
	{
		RCLogError("SetTexture failed");
		delete pRes;
		return false;
	}

	if(m_Props.m_ePreviewMode==ePM_AlphaTest)
	{
		m_pd3ddev->SetRenderState(D3DRS_ALPHATESTENABLE,true);
		m_pd3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE,false);
	}
	else
	{
		m_pd3ddev->SetRenderState(D3DRS_ALPHATESTENABLE,false);
		m_pd3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
		m_pd3ddev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		m_pd3ddev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	}

	m_pd3ddev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
	m_pd3ddev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
	m_pd3ddev->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);

	m_pd3ddev->SetRenderState(D3DRS_ALPHAREF,128);
	m_pd3ddev->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);

	if(m_Props.m_bPreviewFiltered)
	{
		m_pd3ddev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		m_pd3ddev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
	}
	else
	{
		m_pd3ddev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
		m_pd3ddev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);
	}

	m_pd3ddev->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_POINT);

	if(m_Props.m_bPreviewTiled)
	{
		m_pd3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU,	D3DTADDRESS_WRAP);
		m_pd3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV,	D3DTADDRESS_WRAP);
	}
	else
	{
		m_pd3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU,	D3DTADDRESS_BORDER);
		m_pd3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV,	D3DTADDRESS_BORDER);
	}



	// Rendering of scene objects happens here.

	m_pd3ddev->SetVertexShader(NULL);
	m_pd3ddev->SetFVF( D3D8T_CUSTOMVERTEX );

	
	int iScaleX=(int)(iWidth*64.0f/iniScale64*0.5f);
	int iScaleY=(int)(iHeight*64.0f/iniScale64*0.5f);

	// Create vertex data with position and texture coordinates.
	my_vertex g_triangle_vertices[]=
	{
			//  x      y       z     rhw  u1  v1
			{ -0.5f,   -0.5f,   0.5f,  1,   (float)(-iScaleX+iX)/dwWidth,  (float)(-iScaleY+iY)/dwHeight, }, 
			{ 255.5f, -0.5f,   0.5f,  1,   (float)(iScaleX+iX)/dwWidth,   (float)(-iScaleY+iY)/dwHeight, }, 
			{ 255.5f, 255.5f, 0.5f,  1,   (float)(iScaleX+iX)/dwWidth,   (float)(iScaleY+iY)/dwHeight, }, 
			{ -0.5f,   255.5f, 0.5f,  1,   (float)(-iScaleX+iX)/dwWidth,  (float)(iScaleY+iY)/dwHeight, }, 
	};

	m_pd3ddev->DrawPrimitiveUP( D3DPT_TRIANGLEFAN, 2, g_triangle_vertices, sizeof(my_vertex));
	m_pd3ddev->SetTexture(0,0);

// End the scene.
	m_pd3ddev->EndScene();

	hr = m_pd3ddev->Present( NULL, &inRect, inHWND, 0 );

	if(FAILED(hr))
	{
		//check if device was lost
		if (hr == D3DERR_DEVICELOST)
		{
			RCLogError("Device Lost");

			m_pd3ddev->Reset(&m_presentParams);
		}
	}

	ReleasePpo(&pTexture);

	delete pRes;

	return true;
}



bool CImageCompiler::IsPerfectGreyscale( ImageObject * &pCurrentImage ) const
{
	CSimpleBitmap<Vec4> Bmp;

	if(!pCurrentImage->CopyToSimpleBitmap(Bmp))
		return false;

	// now we have to check if R=G=B
	uint32 dwWidth = Bmp.GetWidth(), dwHeight = Bmp.GetHeight();

	for(uint32 dwY=0;dwY<dwHeight;++dwY)
	for(uint32 dwX=0;dwX<dwWidth;++dwX)
	{
		Vec4 val;

		Bmp.Get(dwX,dwY,val);

		if(val.x!=val.y || val.y!=val.z)
		{
			// R=G=B is not the case
			return false;
		}
	}

	return true;
}


uint32 CImageCompiler::GetInputImageWidth() const
{
	if(m_pInputImage)
	{
		uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

		m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

		return dwWidth;
	}

	return 0;
}

//!
uint32 CImageCompiler::GetInputImageHeight() const
{
	if(m_pInputImage)
	{
		uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

		m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

		return dwHeight;
	}

	return 0;
}


bool CImageCompiler::ClampBlitOffset( const int iniWidth, const int iniHeight, float &inoutfX, float &inoutfY, const int iniScale64 ) const
{
	bool bRet=true;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	m_pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	if( iniWidth >= (int)(dwWidth*iniScale64)/64 ) inoutfX=0.5f;
	else
	{
		float fXMin=(float)((iniWidth*64)/iniScale64)/(float)dwWidth*0.5f;
		float fXMax=1.0f-fXMin;

		if(inoutfX<fXMin)inoutfX=fXMin;
		if(inoutfX>fXMax)inoutfX=fXMax;

		bRet=false;
	}

	if( iniHeight >= (int)(dwHeight*iniScale64)/64 ) inoutfY=0.5f;
	else
	{
		float fYMin=(float)((iniHeight*64)/iniScale64)/(float)dwHeight*0.5f;
		float fYMax=1.0f-fYMin;

		if(inoutfY<fYMin)inoutfY=fYMin;
		if(inoutfY>fYMax)inoutfY=fYMax;

		bRet=false;
	}

	return bRet;
}







ImageObject *CImageCompiler::GenerateSpecialPreviewToARGB( ImageObject *pInputImage, const EPreviewMode ePreview ) const
{
	assert(m_pd3ddev);

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwInMips;

	pInputImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwInMips);

	const int nRGBKMultiplier = int((1.f / m_Props.GetRGBKMultiplier()) * 255.f);
	assert(nRGBKMultiplier > 0 && nRGBKMultiplier < 65536);

	LPDIRECT3DTEXTURE9 p2DTexture;
	
	// maybe better to use D3DPOOL_SYSTEMMEM
	// D3DPOOL_MANAGED is needed to use LockRect()
	HRESULT hRes=m_pd3ddev->CreateTexture(dwWidth,dwHeight,dwInMips,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&p2DTexture,0);

	ImageObject *pRet = new DXImage(p2DTexture);

	pRet->CopyPropertiesFrom(*pInputImage);

	for(uint32 dwMip=0;dwMip<dwInMips;++dwMip)
	{
		uint32 dwLocalWidth=dwWidth>>dwMip;						if(dwLocalWidth<1)dwLocalWidth=1;
		uint32 dwLocalHeight=dwHeight>>dwMip;					if(dwLocalHeight<1)dwLocalHeight=1;

		char *pInMem;
		uint32 dwInPitch;

		if(pInputImage->Lock(0,dwMip,pInMem,dwInPitch))
		{
			char *pOutMem;
			uint32 dwOutPitch;

			if(pRet->Lock(0,dwMip,pOutMem,dwOutPitch))
			{
				for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
				{
//					char *src2 = &pInMem[(dwHeight-1-dwY)*dwWidth];
					uint32 *src2 = (uint32 *)&pInMem[dwY*dwInPitch];
					uint32 *dest2 = (uint32 *)&pOutMem[dwOutPitch*dwY];

					for(uint32 dwX=0;dwX<dwLocalWidth;++dwX,++src2)
					{
						switch(ePreview)
						{
							case ePM_RGB:
								{
									uint32 ucR = ((*src2)>>16)&0xff;
									uint32 ucG = ((*src2)>>8)&0xff;
									uint32 ucB = (*src2)&0xff;

									*dest2++ = 0xff000000 | (ucR<<16) | (ucG<<8) | ucB;
								}
								break;

							case ePM_AAA:
								{
									uint32 dwAlpha= (*src2) >> 24;

									*dest2++ = 0xff000000 | (dwAlpha<<16) | (dwAlpha<<8) | dwAlpha;
								}
								break;

							case ePM_NormalLength:
								{
									uint8 ucR = (uint8)( ((*src2)>>16)&0xff );
									uint8 ucG = (uint8)( ((*src2)>>8)&0xff );
									uint8 ucB = (uint8)( (*src2)&0xff );

									float x = ((float)ucR-128.0f)/128.0f;
									float y = ((float)ucG-128.0f)/128.0f;
									float z = ((float)ucB-128.0f)/128.0f;

									uint32 dwGrey = (uint32)( sqrtf(x*x+y*y+z*z)*255.0f );

									if(dwGrey<2)		// to (0,0,0) normal = blue
									{
										*dest2++ = 0xff0000ff;
									}
									else if(dwGrey>255)		// to illegal normals = red
									{
										uint32 dwVal = dwGrey-255;

										if(dwVal>255)
											dwVal=255;

										uint32 dwR = 255;
										uint32 dwGB = max((int)(192-dwVal),(int)0);

										*dest2++ = 0xff000000 | (dwGB) | (dwGB<<8) | (dwR<<16);
									}
									else
									{
										*dest2++ = 0xff000000 | dwGrey | (dwGrey<<8) | (dwGrey<<16);
									}
								}
								break;

							case ePM_RGBA:
								{
									int A = (int)( ((*src2)>>24)&0xff );
									int R = (int)( ((*src2)>>16)&0xff );
									int G = (int)( ((*src2)>>8)&0xff );
									int B = (int)( (*src2)&0xff );

									*dest2++ = (A<<24) | (R<<16) | (G<<8) | B;
								}
								break;

							case ePM_RGBmulA:
							{
								int A = (int)( (((*src2)>>24)&0xff ) * nRGBKMultiplier);
								int SrcR = (int)( ((*src2)>>16)&0xff );
								int SrcG = (int)( ((*src2)>>8)&0xff );
								int SrcB = (int)( (*src2)&0xff );

								int R = max(0, min(255, (SrcR*A)/255/255));	// double division because of multiplier already multiplied by 255
								int G = max(0, min(255, (SrcG*A)/255/255));
								int B = max(0, min(255, (SrcB*A)/255/255));

								*dest2++ = 0xff000000 | (R<<16) | (G<<8) | B;
							}
							break;

							case ePM_AlphaTest:
								{
									uint32 ucR = ((*src2)>>16)&0xff;
									uint32 ucG = ((*src2)>>8)&0xff;
									uint32 ucB = (*src2)&0xff;

									uint32 dwAlpha= (*src2) >> 24;

									*dest2++ = (dwAlpha<<24) | (ucR<<16) | (ucG<<8) | ucB;
								}
								break;

							case ePM_mCIE2RGB:
								{
									uint32 ucR = ((*src2)>>16)&0xff;
									uint32 ucG = ((*src2)>>8)&0xff;
									uint32 ucB = (*src2)&0xff;


									uint32 dwAlpha= (*src2) >> 24;
									
									ColorF col = ColorF(ucR/255.0f,ucG/255.0f,ucB/255.0f).mCIE2RGB();
									ucR = (uint32)(col.r*255.0f+0.5f);
									ucG = (uint32)(col.g*255.0f+0.5f);
									ucB = (uint32)(col.b*255.0f+0.5f);

									assert(ucR<256);
									assert(ucG<256);
									assert(ucB<256);

									*dest2++ = (dwAlpha<<24) | (ucR<<16) | (ucG<<8) | ucB;
								}

								break;

							default:
								assert(0);
								break;
						}
					}
				}
				/*
				// test
				if(ePreview==ePM_mCIE2RGB)
				if(dwMip==0)
				{
					FILE *out = fopen("c:/temp/aa.raw","wb");
					
					for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
					for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
					{
						uint8 *dest2 = (uint8 *)&pOutMem[dwOutPitch*dwY+dwX*4];
						fwrite(&dest2[2],1,1,out);
						fwrite(&dest2[1],1,1,out);
						fwrite(&dest2[0],1,1,out);
					}
					fclose(out);
				}
				*/

				pRet->Unlock(dwMip);
			}
			else assert(0);

			pInputImage->Unlock(dwMip);
		}
		else assert(0);
	}

  return pRet;
}




// set the stored properties to the current file and save it
bool CImageCompiler::LoadConfigFile( const EConfigPriority ePriMask )
{
	assert(m_pCC);

	if(!m_Props.GetUserDialogCustom() && !m_pCC->presets->Find(m_Props.GetPreset()))
	{
		// by default the first preset should be used - if there is one
		const char *szPreset = m_pCC->presets->GetSectionName(1);

		m_Props.SetPreset(szPreset ? szPreset : "");
	}

	return true;
} 


void CImageCompiler::SetPresetSettings()
{
	m_Props.SetPreset(m_Props.GetPreset());		// to update eCP_PriorityPreset settings

	// correct preview mode
	if(m_Props.GetRGBKCompression())
		m_Props.m_ePreviewMode = ePM_RGBmulA;

	uint32 dwImageFlags = 0;
	
	if(m_Props.GetDecal())
		dwImageFlags|=CImageExtensionHelper::EIF_Decal;

	if(m_Props.GetSupressEngineReduce())
		dwImageFlags|=CImageExtensionHelper::EIF_SupressEngineReduce;

	if(m_Props.GetSRGBLookup())
		dwImageFlags|=CImageExtensionHelper::EIF_SRGBRead;

	if(m_Props.GetCubemap() && m_pInputImage->IsCubemap())
		dwImageFlags|=CImageExtensionHelper::EIF_Cubemap;

	m_pInputImage->SetImageFlags(dwImageFlags);
}

// set the stored properties to the current file and save it
bool CImageCompiler::UpdateAndSaveConfig()
{
	if(m_pCC->sourceFileFinalExtension=="tif")
	{
		// TIFF, store inside the source file
		return UpdateAndSaveConfigToTIF(m_pCC->getSourcePath(),m_pCC->config,m_pCC);
	}
	else 
	{
	 	// DirectX, store data in .ini nearby the source file
//		return m_pCC->config->Save();
		return false;
	}
}



static ATI_TC_FORMAT FindCorespondingATIFormat( D3DFORMAT fmtTo )
{
	switch(fmtTo)
	{
		case D3DFMT_A8R8G8B8: return ATI_TC_FORMAT_ARGB_8888;
		case D3DFMT_X8R8G8B8: return ATI_TC_FORMAT_ARGB_8888;
    case D3DFMT_A32B32G32R32F: return ATI_TC_FORMAT_ARGB_32F;
    case D3DFMT_DXT1: return ATI_TC_FORMAT_DXT1;
    case D3DFMT_DXT3: return ATI_TC_FORMAT_DXT3;
    case D3DFMT_DXT5: return ATI_TC_FORMAT_DXT5;
    case D3DFMT_3DC: return ATI_TC_FORMAT_ATI2N;
//    case : return ATI_TC_FORMAT_ATI2N_XY;
 //   case : return ATI_TC_FORMAT_ATI2N_DXT5;
	}

	assert(0);

	return (ATI_TC_FORMAT)(-1);			// output pixel format not supported yet
}

// e.g. ATI 3dc compression
ImageObject *CImageCompiler::ConvertFormatWithATI( ImageObject *inSrc, const D3DFORMAT format )
{
	assert(inSrc);

	// only standard 3d textures can be processed
	if((inSrc->GetImageFlags()&CImageExtensionHelper::EIF_Volumetexture) 
	/*|| (inSrc->GetImageFlags()&CImageExtensionHelper::EIF_Cubemap)*/)
		return 0;

	ATI_TC_FORMAT fmtDest = FindCorespondingATIFormat(format);

	if(fmtDest==(ATI_TC_FORMAT)(-1))
		return 0;

	if(!inSrc->IsPowerOfTwo())
	{
		RCLogError("CImageCompiler::ConvertFormatWithATI failed (non power of two)");
		return 0;
	}

	// convert to a defined input format
//	ImageObject *pInputARGB = ConvertFormat(inSrc,D3DFMT_A8R8G8B8);
//	ATI_TC_FORMAT fmtSrc = ATI_TC_FORMAT_ARGB_8888;

	ATI_TC_FORMAT fmtSrc = FindCorespondingATIFormat(CPixelFormats::GetPixelFormatInfo(inSrc->GetPixelFormat())->DxtNo);

	if(fmtSrc==(ATI_TC_FORMAT)(-1))
		return 0;

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwInMips;

	inSrc->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwInMips);

//	uint32 dwOutMips = _CalcMipCount(dwWidth,dwHeight,true);		// true = DXT

	ImageObject *pRet = new CRAWImage(dwWidth,dwHeight,dwInMips,CPixelFormats::GetNoFromD3DFormat(format),IsFinalPixelFormatCompressed());

	pRet->CopyPropertiesFrom(*inSrc);

	uint32 dwBadPixels=0,dwAllPixels=0;

	for(uint32 dwMip=0;dwMip<dwInMips;++dwMip)
	{
		uint32 dwLocalWidth=inSrc->GetWidth(dwMip);
		uint32 dwLocalHeight=inSrc->GetHeight(dwMip);

		char *pInMem;
		uint32 dwInPitch;

		if(inSrc->Lock(0,dwMip,pInMem,dwInPitch))
		{
			char *pOutMem;
			uint32 dwOutPitch;

			if(pRet->Lock(0,dwMip,pOutMem,dwOutPitch))
			{
				ATI_TC_Texture texInput;
				ATI_TC_Texture texOutput;

				texInput.dwSize=sizeof(ATI_TC_Texture);
				texInput.dwWidth=dwLocalWidth;
				texInput.dwHeight=dwLocalHeight;
				texInput.dwPitch=dwInPitch;							// ARGB = 4 bytes per pixel
				texInput.format=fmtSrc;
				texInput.dwDataSize=dwInPitch*dwLocalHeight;
				texInput.pData = (ATI_TC_BYTE *)pInMem;

				dwAllPixels+=dwLocalHeight*dwLocalWidth;

				bool bFixNormals = (fmtDest==ATI_TC_FORMAT_ATI2N);		// only this format stores normals

				if(bFixNormals)
				{
					if(fmtSrc == ATI_TC_FORMAT_ARGB_8888)		// fix illegal normals - caused problems when reconstructing on NVidia
					{
						assert(dwLocalWidth>=4 && dwLocalHeight>=4);		// required by new ATICompressorLib

						for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
						{
							uint32 *pMem = (uint32 *)&pInMem[dwY*dwInPitch];

							for(uint32 dwX=0;dwX<dwLocalWidth;++dwX,++pMem)
							{
								int x = (int)((*pMem>>16)&0xff) - 128;	// R
								int y = (int)((*pMem>>8)&0xff) - 128;		// G
								int l = x*x+y*y;

								if(l>128*128)								// fix required? limit normals in xy circle
								{
									float fX = ((x+128)*2.0f/255.0f)-1.0f;
									float fY = ((y+128)*2.0f/255.0f)-1.0f;
									float fL = fX*fX+fY*fY;
									float fInvLen = 1.0f/sqrt(fL);

									fX*=fInvLen; fY*=fInvLen;
		
									++dwBadPixels;

									int r = (int)((fX*0.5f+0.5f)*255.0f+0.4999f);
									int g = (int)((fY*0.5f+0.5f)*255.0f+0.4999f);

									*pMem = (*pMem&0xff000000) | (r<<16) | (g<<8);
								}
							}
						}
					}
					else if(fmtSrc == ATI_TC_FORMAT_ARGB_32F)
					{
						for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
						{
							float *pMem = (float *)&pInMem[dwY*dwInPitch];

							for(uint32 dwX=0;dwX<dwLocalWidth;++dwX,pMem+=4)
							{
								float x = (pMem[0]-0.5f)*2.0f;		// R
								float y = (pMem[1]-0.5f)*2.0f;		// G
								float z = (pMem[2]-0.5f)*2.0f;		// B
								float l = x*x+y*y;

								if(l>1.0f)								// fix required? limit normals in xy circle
								{
									float fInvLen = 1.0f/sqrt(l);

									x*=fInvLen; y*=fInvLen; z=0;

									++dwBadPixels;
									pMem[0] = x*0.5f+0.5f;			// R
									pMem[1] = y*0.5f+0.5f;			// G
									pMem[2] = z*0.5f+0.5f;			// B
								}
							}
						}
					}
				}

				texOutput.dwSize=sizeof(ATI_TC_Texture);
				texOutput.dwWidth=dwLocalWidth;
				texOutput.dwHeight=dwLocalHeight;
				texOutput.dwPitch=0;
				texOutput.format=fmtDest;
				texOutput.dwDataSize=ATI_TC_CalculateBufferSize(&texOutput);
				texOutput.pData = (ATI_TC_BYTE *)pOutMem;

//				assert(dwOutPitch*dwLocalHeight==texOutput.dwDataSize);

				ATI_TC_ERROR err = ATI_TC_ConvertTexture(&texInput,&texOutput,0,0,0,0);

				pRet->Unlock(dwMip);

				if(err != ATI_TC_OK)
				{
					RCLogError("CImageCompiler::ConvertFormatTo3DC failed (ATI_TC_ConvertTexture returned %d)",(int)err);
					delete pRet;
					return 0;
				}
			}

			inSrc->Unlock(dwMip);
		}
		else
		{
			RCLogError("CImageCompiler::ConvertFormatTo3DC failed (Lock)");
			delete pRet;
			return 0;
		}
	}

	if(dwAllPixels)
	{
		if((float)dwBadPixels/(float)dwAllPixels>0.10f)		// >10% bad pixels
		{
			// tell user
			RCLogWarning("CImageCompiler::ConvertFormatTo3DC %.1f%% bad pixels (denormalized normals)",100.0f*(float)dwBadPixels/(float)dwAllPixels);

			// tell coder
		//	assert(0);

			// do not compile
//			delete pRet;
//			return 0;
		}
	}

	if(format==D3DFMT_3DC || format==D3DFMT_CTX1)
	{
		// save alpha channel as attached image

		if(inSrc->HasAlpha())
		if(m_Props.m_AlphaAsBump.GetBumpToNormalFilter()==0)							// alpha2bump suppress the alpha channel export - this behavior saves memory
			pRet->SetAttachedImage(inSrc->ExtractAlphaAsA8Image());
	}
	else
	{
		// restore alpha channel from attached image
		pRet->SetAlphaFromA8Image(inSrc->GetAttachedImage());		
	}


	return pRet;
}




bool CImageCompiler::SaveAsTIFF( ImageObject * &pInput, const char *szFilename )
{
	assert(pInput);
	assert(szFilename);


	if(pInput->GetPixelFormat()!=CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8))
	{
		ImageObject *pRes=ConvertFormat(pInput,D3DFMT_A8R8G8B8);

		if(!pRes)
			return false;

		if(!ProceedToNextPass(pInput,pRes))
			return false;
	}

	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;

	pInput->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);
/*
	if( gStuff->depth!=8
		 || (gStuff->imageMode!=plugInModeRGBColor && gStuff->imageMode!=plugInModeGrayScale)	//	Bitmap=0; Grayscale=1; Indexed=2; RGB=3 Multichannel=7; Duotone=8; Lab=9.
		 || (gStuff->planes!=1 && gStuff->planes!=3 && gStuff->planes!=4))		// grey,RGB,RGBA
	{
		Str255 errString = "*the input format is currently not supported";

		errString[0]=strlen((char *)&errString[1]);	// pascal string, could be done better

		gResult = PIReportError(errString);
		return;
	}
*/


	TIFF *pTiffFile = TIFFOpen(szFilename,"w");

	if(pTiffFile) 
	{
		char *pInputMem=0;
		uint32 dwInputPitch=0;
		
		if(!pInput->Lock(0,0,pInputMem,dwInputPitch))
		{
			TIFFClose(pTiffFile);
			return false;
		}

		uint32 dwPlanes = 4;
		uint32 dwBitsPerSample = 8;

		int32 done=0, total=dwHeight*dwPlanes;

		// We need to set some values for basic tags before we can add any data
		TIFFSetField(pTiffFile, TIFFTAG_IMAGEWIDTH, dwWidth);
		TIFFSetField(pTiffFile, TIFFTAG_IMAGELENGTH, dwHeight);	
		TIFFSetField(pTiffFile, TIFFTAG_SAMPLESPERPIXEL, dwPlanes);
		TIFFSetField(pTiffFile, TIFFTAG_BITSPERSAMPLE, 8); 
		TIFFSetField(pTiffFile, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize( pTiffFile, -1 ) ); 
		TIFFSetField(pTiffFile, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
//		TIFFSetField(pTiffFile, TIFFTAG_PHOTOMETRIC, gStuff->planes==1?PHOTOMETRIC_MINISBLACK:PHOTOMETRIC_RGB);
		TIFFSetField(pTiffFile, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(pTiffFile, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(pTiffFile, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

		size_t npixels = dwWidth * dwHeight;
		uint8 *raster = (uint8*) new char[npixels * sizeof (uint32)];

		if(!raster)
			RCLogError("SaveAsTIFF out of memory");

		for(uint32 row = 0; row < dwHeight; ++row)
		{
			uint32 *pDst = (uint32 *)&raster[dwWidth*row*dwPlanes];
			uint32 *pSrc = (uint32 *)&pInputMem[dwInputPitch*row];

			for(uint32 dwX=0;dwX<dwWidth;++dwX)
			{
				uint32 v = *pSrc++;

				*pDst++ = (v&0xff00ff00) | ((v&0xff)<<16) | ((v&0xff0000)>>16);			// swap red with blue
			}

			int err = TIFFWriteScanline(pTiffFile, &raster[dwWidth*row*dwPlanes], row, 0);
			assert( err >= 0 );

		}

		SAFE_DELETE_ARRAY(raster);

		pInput->Unlock(0);
	}
	else return false;

	assert(pTiffFile);

	TIFFClose(pTiffFile);

	// update meta data
	UpdateAndSaveConfigToTIF(szFilename,m_pCC->config,m_pCC);

	return true;
}

namespace
{
	static ColorF XMVectorSplatX(const ColorF& V1)
	{
		ColorF Result;
		Result.r = V1.r;
		Result.g = V1.r;
		Result.b = V1.r;
		Result.a = V1.r;
		return Result;
	}
	static ColorF XMVectorSplatY(const ColorF& V1)
	{
		ColorF Result;
		Result.r = V1.g;
		Result.g = V1.g;
		Result.b = V1.g;
		Result.a = V1.g;
		return Result;
	}
	static ColorF XMVectorSplatZ(const ColorF& V1)
	{
		ColorF Result;
		Result.r = V1.b;
		Result.g = V1.b;
		Result.b = V1.b;
		Result.a = V1.b;
		return Result;
	}
	static ColorF XMVectorSplatW(const ColorF& V1)
	{
		ColorF Result;
		Result.r = V1.a;
		Result.g = V1.a;
		Result.b = V1.a;
		Result.a = V1.a;
		return Result;
	}
	static ColorF XMVectorTruncate(const ColorF& V1)
	{
		ColorF Result;
		Result.r = floorf(V1.r);
		Result.g = floorf(V1.g);
		Result.b = floorf(V1.b);
		Result.a = floorf(V1.a);
		return Result;
	}
	static ColorF XMVectorLess(const ColorF& V1, const ColorF& V2)
	{
		ColorF Result;
		Result.r = (V1.r < V2.r) ? 1.0f : 0.0f;
		Result.g = (V1.g < V2.g) ? 1.0f : 0.0f;
		Result.b = (V1.b < V2.b) ? 1.0f : 0.0f;
		Result.a = (V1.a < V2.a) ? 1.0f : 0.0f;
		return Result;
	}
	static ColorF XMVectorMultiplyAdd(const ColorF& V1, const ColorF& V2, const ColorF& V3)
	{
		ColorF Result;
		Result.r = V1.r * V2.r + V3.r;
		Result.g = V1.g * V2.g + V3.g;
		Result.b = V1.b * V2.b + V3.b;
		Result.a = V1.a * V2.a + V3.a;
		return Result;
	}
	static ColorF XMVectorSelect(const ColorF& V1, const ColorF& V2, const ColorF& Control)
	{
		ColorF Result;
		Result.r = Control.r > 0 ? V2.r : V1.r;
		Result.g = Control.g > 0 ? V2.g : V1.g;
		Result.b = Control.b > 0 ? V2.b : V1.b;
		Result.a = Control.a > 0 ? V2.a : V1.a;
		return Result;
	}

	static ColorF XMVectorSplatConstant(const float v, float divExp)
	{
		ColorF Result;
		Result.r = v / powf(2.f, divExp);
		Result.g = v / powf(2.f, divExp);
		Result.b = v / powf(2.f, divExp);
		Result.a = v / powf(2.f, divExp);
		return Result;
	}

	inline ColorF XboxConvertLinearToGamma(ColorF Texel)
	{
		ColorF GammaTexel;
		ColorF L0, L1, L2, L3;
		ColorF LessOneSixteenth, LessOneEighth, LessOneHalf;
		static const ColorF C0 = ColorF(64.0f / 1023.0f, 128.0f / 1023.0f, 512.0f / 1023.0f, 1.0f / 255.0f);
		static const ColorF C1 = ColorF(1023.0f, 1023.0f / 2.0f, 1023.0f / 4.0f, 1023.0f / 8.0f);
		static const ColorF C2 = ColorF(0.0f, 32.0f / 255.0f, 64.0f / 255.0f, 128.0f / 255.0f);

		LessOneSixteenth = XMVectorLess(Texel, XMVectorSplatX(C0));
		LessOneEighth = XMVectorLess(Texel, XMVectorSplatY(C0));
		LessOneHalf = XMVectorLess(Texel, XMVectorSplatZ(C0));

		L0 = XMVectorTruncate(Texel * XMVectorSplatX(C1)) * XMVectorSplatW(C0);
		L1 = XMVectorMultiplyAdd(XMVectorTruncate(Texel * XMVectorSplatY(C1)), XMVectorSplatW(C0), XMVectorSplatY(C2));
		L2 = XMVectorMultiplyAdd(XMVectorTruncate(Texel * XMVectorSplatZ(C1)), XMVectorSplatW(C0), XMVectorSplatZ(C2));
		L3 = XMVectorMultiplyAdd(XMVectorTruncate(Texel * XMVectorSplatW(C1)), XMVectorSplatW(C0), XMVectorSplatW(C2));

		GammaTexel = XMVectorSelect(L3, L2, LessOneHalf);
		GammaTexel = XMVectorSelect(GammaTexel, L1, LessOneEighth);
		GammaTexel = XMVectorSelect(GammaTexel, L0, LessOneSixteenth);
		GammaTexel.a = Texel.a;

		return GammaTexel;
	}

	//static ColorF XboxConvertGammaToLinear(const ColorF& Texel)
	//{
	//	ColorF LinearTexel;
	//	ColorF L0, L1, L2, L3;
	//	ColorF LessOneFourth, LessThreeEighths, LessThreeFourths;
	//	static const ColorF C0 = ColorF(64.0f / 255.0f, 96.0f / 255.0f, 192.0f / 255.0f, 1.0f / 1023.0f);
	//	static const ColorF C1 = ColorF(255.0f, -64.0f, -256.0f, -1024.0f);

	//	LessOneFourth = XMVectorLess(Texel, XMVectorSplatX(C0));
	//	LessThreeEighths = XMVectorLess(Texel, XMVectorSplatY(C0));
	//	LessThreeFourths = XMVectorLess(Texel, XMVectorSplatZ(C0));

	//	L0 = Texel * XMVectorSplatX(C1);
	//	L1 = XMVectorMultiplyAdd(L0, XMVectorSplatConstant(2, 0), XMVectorSplatY(C1));
	//	L2 = XMVectorMultiplyAdd(L0, XMVectorSplatConstant(4, 0), XMVectorSplatZ(C1));
	//	L3 = XMVectorMultiplyAdd(L0, XMVectorSplatConstant(8, 0), XMVectorSplatW(C1));

	//	L1 += XMVectorTruncate(L1 * XMVectorSplatConstant(1, 9));
	//	L2 += XMVectorTruncate(L2 * XMVectorSplatConstant(1, 8));
	//	L3 += XMVectorTruncate(L3 * XMVectorSplatConstant(1, 7));

	//	LinearTexel = XMVectorSelect(L3, L2, LessThreeFourths);
	//	LinearTexel = XMVectorSelect(LinearTexel, L1, LessThreeEighths);
	//	LinearTexel = XMVectorSelect(LinearTexel, L0, LessOneFourth);
	//	LinearTexel *= XMVectorSplatW(C0);
	//	LinearTexel.a = Texel.a;

	//	return LinearTexel;
	//}
}

ImageObject * CImageCompiler::ConvertGammaToXBox360( ImageObject *inSrc )
{
	// convert to 32-bits format
	ImageObject *pRes = inSrc;
	const TPixelFormatID srcFormat = inSrc->GetPixelFormat();
	if(inSrc->GetPixelFormat() != CPixelFormats::GetNoFromD3DFormat(D3DFMT_A32B32G32R32F))
	{
		pRes=ConvertFormat(inSrc, D3DFMT_A32B32G32R32F);

		if(!pRes)
			return NULL;

		inSrc = pRes;
	}

	char *pMem;
	uint32 dwPitch;

	uint32 w,h,depth,sides,mips;
	inSrc->GetExtend( w,h,depth,sides,mips );

	// make gamma-correction
	for(uint32 dwMip=0;dwMip<mips;++dwMip)
	{
		uint32 dwLocalWidth=inSrc->GetWidth(dwMip);
		uint32 dwLocalHeight=inSrc->GetHeight(dwMip);

		if(pRes->Lock(0,dwMip,pMem,dwPitch))
		{
			assert(dwLocalWidth*sizeof(ColorF) == dwPitch);
			for(uint32 dwY=0;dwY<dwLocalHeight;++dwY)
			{
				for(uint32 dwX=0;dwX<dwLocalWidth;++dwX)
				{
					ColorF& rTexel = *(((ColorF*)&pMem[dwY*dwPitch])+dwX);

					// convert sRGB gamma space -> linear space
					rTexel.r= ((rTexel.r<=0.03928f) ? rTexel.r/12.92f : powf((rTexel.r + 0.055f)/1.055f, 2.4f));
					rTexel.g= ((rTexel.g<=0.03928f) ? rTexel.g/12.92f : powf((rTexel.g + 0.055f)/1.055f, 2.4f));
					rTexel.b= ((rTexel.b<=0.03928f) ? rTexel.b/12.92f : powf((rTexel.b + 0.055f)/1.055f, 2.4f));

					// convert linear -> XBox PWL sRGB gamma space
					rTexel = XboxConvertLinearToGamma(rTexel);
				}
			}
			pRes->Unlock(dwMip);
		}
	}

	// convert from 32-bits format
	if(srcFormat != CPixelFormats::GetNoFromD3DFormat(D3DFMT_A32B32G32R32F))
	{
		ImageObject *pOld = pRes;
		pRes=ConvertFormat(pOld, CPixelFormats::GetPixelFormatInfo(srcFormat)->DxtNo);

		delete pOld;

		if(!pRes)
			return NULL;
	}

	return pRes;
}

void CImageCompiler::ValidateSourceImage( ImageObject *pSourceImage )
{
	// works only with CryTIFF
	if(!m_pImageUserDialog || !pSourceImage)
		return;

	CSimpleBitmap<Vec4> Bmp;
	if(!pSourceImage->CopyToSimpleBitmap(Bmp))
		return;

	ColorF *pMem = (ColorF*)Bmp.GetPointer();

	if(!pMem)
		return;

	uint64 nHistogram[256] = { 0 };
	double fMeanLuminance = 0.;

	for(uint32 dwY=0;dwY<Bmp.GetHeight();++dwY)
	{
		//early out: check for canceling preview generation
		if (m_bInternalPreview)
		{
			if (m_pImageUserDialog->PreviewGenerationCanceled())
				return;
		}

		for(uint32 dwX=0;dwX<Bmp.GetWidth();++dwX)
		{
			ColorF &col = *pMem++;

			const float fLuminance = clamp_tpl(col.Luminance(), 0.f, 1.f) * 255.f;
			fMeanLuminance += (double)fLuminance;
			int16 nBin = (int16)floor(fLuminance);
			assert(nBin >= 0 && nBin <= 255);
			nBin = clamp_tpl(nBin, (int16)0, (int16)255);
			nHistogram[nBin]++;
		}
	}

	// normalize
	fMeanLuminance /= Bmp.GetHeight();
	fMeanLuminance /= Bmp.GetWidth();

	if(fMeanLuminance < 64.f)
	{
		char outMessage[1024];
		sprintf_s(outMessage, "The texture is too dark. Mean luminance: %.0f / 255. Try to adjust the texture levels or add more bright areas to utilize the color range efficiently.", (float)fMeanLuminance);
		RCLogWarning(outMessage);
	}

	uint64 nSum = 0;
	const uint64 nHalfImagePixels = (uint64)Bmp.GetHeight() * (uint64)Bmp.GetWidth() / 2;
	int iIndex=0;
	for(;iIndex<256;++iIndex)
	{
		nSum += nHistogram[iIndex];
		if(nSum >= nHalfImagePixels)
			break;
	}

	if(iIndex < 32)
	{
		char outMessage[1024];
		sprintf_s(outMessage, "The texture is too dark. More than half of the image's luminance is less than %d /255. Try to adjust the texture levels or add more bright areas to utilize the color range efficiently.", iIndex);
		RCLogWarning(outMessage);
	}
}