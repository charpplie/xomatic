#include "StdAfx.h"
#include "PixelFormats.h"				// CPixelFormats
#include "ImageObject.h"				// ImageObject
#include "ImageExtensionHelper.h"

SPixelFormats g_pixelformats[]=
{
	// Unsigned Formats (Data in an unsigned format must be positive. Unsigned formats use combinations of
	// (R)ed, (G)reen, (B)lue, (A)lpha, (L)uminance, and (P)alette data.
	// Palette data is also referred to as color indexed data because the data is used to index a color palette.)

	{ 32,"8",			D3DFMT_A8R8G8B8,		"A8R8G8B8",		"A8R8G8B8",				"32-bit ARGB pixel format with alpha, using 8 bits per channel",false,true },
	{ 32,"0",			D3DFMT_X8R8G8B8,		"X8R8G8B8",		"X8R8G8B8",				"32-bit RGB pixel format, where 8 bits are reserved for each color",false,true },
	{ 24,"0",			D3DFMT_R8G8B8,			"R8G8B8",			"X8R8G8B8",				"24-bit RGB pixel format with 8 bits per channel",false,true },
	{ 16,"0",			D3DFMT_R5G6B5,			"R5G6B5",			"X8R8G8B8",				"16-bit RGB pixel format with 5 bits red, 6 bits green, and 5 bits blue",false,true },
	{ 16,"1",			D3DFMT_A1R5G5B5,		"A1R5G5B5",		"A8R8G8B8",				"16-bit ARGB using 5 bits for each color and 1 bit for alpha",false,true },
	{ 16,"0",			D3DFMT_X1R5G5B5,		"X1R5G5B5",		"X8R8G8B8",				"16-bit RGB using 5 bits for each color",false,true },
	{ 16,"4",			D3DFMT_A4R4G4B4,		"A4R4G4B4",		"A8R8G8B8",				"16-bit ARGB with 4 bits for each channel",false,true },
	{ 16,"0",			D3DFMT_X4R4G4B4,		"X4R4G4B4",		"X8R8G8B8",				"16-bit RGB using 4 bits for each color",false,true },
	{ 8, "8",			D3DFMT_A8,					"A8",					"A8",							"8-bit alpha only",false,true },
	{ 8, "0",			D3DFMT_R3G3B2,			"R3G3B2",			"X8R8G8B8",				"8-bit RGB using 3 bits red, 3 bits green, and 2 bits blue",false,true },
	//	{ 16,"8",			D3DFMT_A8R3G3B2,		"A8R3G3B2",		"A8R8G8B8",				"16-bit ARGB texture format using 8 bits for alpha, 3 bits each for red and green, and 2 bits for blue",false,true },
	//	{ 32,"2",			D3DFMT_A2B10G10R10,	"A2B10G10R10","A2B10G10R10",		"32-bit pixel format using 10 bits for each color and 2 bits for alpha",false,true },
	//	{ 32,"0",			D3DFMT_G16R16,			"G16R16",			"G16R16",					"32-bit pixel format using 16 bits each for green and red",false,true },
	//	{ 8, "8",			D3DFMT_A8P8,				"A8P8",				"A8P8",						"8-bit color indexed with 8 bits of alpha",false,true },
	//	{ 8, "0",			D3DFMT_P8,					"P8",					"X8R8G8B8",				"8-bit color indexed",false,true },
	{ 8, "0",			D3DFMT_L8,					"L8",					"L8",							"8-bit luminance only",false,true },
	//	{ 16,"8",			D3DFMT_A8L8,				"A8L8",				"A8L8",						"16-bit using 8 bits each for alpha and luminance",false,true },
	//	{ 8, "4",			D3DFMT_A4L4,				"A4L4",				"A8L8",						"8-bit using 4 bits each for alpha and luminance",false,true },

	// Signed Formats (Data in a signed format can be both positive and negative. Signed formats use combinations of (U), (V), (W), and (Q) data.) -----------------------------------------

	{ 16,"0",			D3DFMT_V8U8,				"V8U8",				"V8U8",						"16-bit signed bump-map format using 8 bits each for u and v data",false,true },
	//	{ 32,"0",			D3DFMT_Q8W8V8U8,		"Q8W8V8U8",		"Q8W8V8U8",				"32-bit signed bump-map format using 8 bits for each channel",false,true },
	//	{ 32,"0",			D3DFMT_V16U16,			"V16U16",			"V16U16",					"32-bit signed bump-map format using 16 bits for each channel",false,true },
	//	{ 32,"0",			D3DFMT_W11V11U10,		"W11V11U10",	"W11V11U10",			"32-bit bump-map format using 11 bits each for w and v, and 10 bits for u",false,true },

	// Mixed Formats (Data in mixed formats can contain a combination of unsigned data and signed data.) -----------------------------------------

	//	{ 16,"0",			D3DFMT_L6V5U5,			"L6V5U5",			"X8L8V8U8",				"16-bit signed/unsigned bump-map format with luminance using 6 bits for luminance, and 5 bits each for u and v",false,true },
	//	{ 32,"0",			D3DFMT_X8L8V8U8,		"X8L8V8U8",		"X8L8V8U8",				"32-bit signed/unsigned bump-map format with luminance using 8 bits for each channel",false,true },
	//	{ 32,"2",			D3DFMT_A2W10V10U10,	"A2W10V10U10","A2W10V10U10",		"32-bit signed/unsigned bump-map format using 2 bits for alpha and 10 bits each for w, v, and u",false,true },

	// FourCC Formats (Data in a FourCC format is compressed data.) -----------------------------------------

	//	{ 16,"0",			D3DFMT_UYVY,				"UYVY",				"A8R8G8B8",				"UYVY compressed format (PC98 compliance)",true,true },
	//	{ 16,"0",			D3DFMT_YUY2,				"YUY2",				"A8R8G8B8",				"YUY2 compressed format (PC98 compliance)",true,true },
	{ 4, "0/1",		D3DFMT_DXT1,				"DXT1",				"A8R8G8B8",				"DXT1 compressed texture format",true,true },
	//	{ 16,"4p",		D3DFMT_DXT2,				"DXT2",				"A8R8G8B8",				"DXT2 compressed texture format",true,true },
	{ 8,"4",			D3DFMT_DXT3,				"DXT3",				"A8R8G8B8",				"DXT3 compressed texture format",true,true },
	//	{ 16,"3of8p",	D3DFMT_DXT4,				"DXT4",				"A8R8G8B8",				"DXT4 compressed texture format",true,true },
	{ 8,"3of8",		D3DFMT_DXT5,				"DXT5",				"A8R8G8B8",				"DXT5 compressed texture format",true,true },

	{ 8,"0",			D3DFMT_3DC,					"3DC",				"X8R8G8B8",				"ATI compressed texture format for normalmaps",true,true },
	{ 4,"0",			D3DFMT_CTX1,			"CTX1",			"X8R8G8B8",				"CTX1 4 bpp compressed normal maps for Xbox 360",true,false },
	
	// float formats -----------------------------------------------------------------------------------------

	//		{ 32,"0",	D3DFMT_R32F,						"R32F",					"R32F",					"single float channel (red)",false,false },
	{ 128,"0",D3DFMT_A32B32G32R32F,		"A32R32G32B32F","A32R32G32B32F","four float channels",false,true },
};



int CPixelFormats::GetNoFromName( const char *inszName )
{
	int iCount=CPixelFormats::GetPixelFormatCount();

	for(int i=0;i<iCount;i++)
		if(stricmp(g_pixelformats[i].szName,inszName)==0)
			return(i);

	return(-1);
}

const ETEX_Format CPixelFormats::GetTextureFormat(const TPixelFormatID iPixelFormat)
{
	switch(g_pixelformats[iPixelFormat].DxtNo)
	{
	case D3DFMT_A8R8G8B8:
		return eTF_A8R8G8B8;
	case D3DFMT_X8R8G8B8:
		return eTF_X8R8G8B8;
	case D3DFMT_G16R16F:
		return eTF_G16R16F;
	case D3DFMT_R32F:
		return eTF_R32F;
	case D3DFMT_A4R4G4B4:
		return eTF_A4R4G4B4;
	case D3DFMT_DXT3:
		return eTF_DXT3;
	case D3DFMT_DXT5:
		return eTF_DXT5;
	case D3DFMT_3DC:
		return eTF_3DC;
	case D3DFMT_DXT1:
		return eTF_DXT1;
	case D3DFMT_CTX1:
		return eTF_CTX1;
	case D3DFMT_A8:
		return eTF_A8;
	case D3DFMT_L8:
		return eTF_L8;
	case D3DFMT_A32B32G32R32F:
		return eTF_A32B32G32R32F;
	default:
		assert(0);
		return eTF_Unknown;
	}
}

TPixelFormatID CPixelFormats::FindFinalTextureFormat( const TPixelFormatID iPixelFormat, const bool bAlphaChannelUsed )
{
	TPixelFormatID iRet=-1;

	if(iPixelFormat==-1)
		return iPixelFormat;

	if(bAlphaChannelUsed)
	{
		switch(g_pixelformats[iPixelFormat].DxtNo)
		{
			case D3DFMT_X8R8G8B8: iRet=GetNoFromD3DFormat(D3DFMT_A8R8G8B8);break;
			case D3DFMT_R8G8B8: iRet=GetNoFromD3DFormat(D3DFMT_A8R8G8B8);break;
			case D3DFMT_R5G6B5: iRet=GetNoFromD3DFormat(D3DFMT_A4R4G4B4);break;		// minor quality loss
			case D3DFMT_X1R5G5B5: iRet=GetNoFromD3DFormat(D3DFMT_A1R5G5B5);break;
			case D3DFMT_X4R4G4B4: iRet=GetNoFromD3DFormat(D3DFMT_A4R4G4B4);break;
			case D3DFMT_R3G3B2: iRet=GetNoFromD3DFormat(D3DFMT_A4R4G4B4);break;
			case D3DFMT_L8: iRet=GetNoFromD3DFormat(D3DFMT_A8R8G8B8);break;				// not perfect solution
			case D3DFMT_DXT1: iRet=GetNoFromD3DFormat(D3DFMT_DXT5);break;					// unlikly the 1 bit (premultiplied) alpha is good enough, DXT5 still compressed quite ok
			case D3DFMT_A32B32G32R32F: iRet=GetNoFromD3DFormat(D3DFMT_A32B32G32R32F);break;					// for HDR tiffs
		}
	}
	else
	{
		switch(g_pixelformats[iPixelFormat].DxtNo)
		{
			case D3DFMT_A8R8G8B8: iRet=GetNoFromD3DFormat(D3DFMT_X8R8G8B8);break;	// DirectX conversion of D3DFMT_R8G8B8 is not always supported - thats why we use this format
//			case D3DFMT_A8R8G8B8: iRet=GetNoFromD3DFormat(D3DFMT_R8G8B8);break;	// D3DFMT_X8R8G8B8 would be better for hardware but loading time seems more important
		}
	}

	return iRet!=-1 ? iRet: iPixelFormat;		// no better solution, stick to the input
}

TPixelFormatID CPixelFormats::GetFormatForMips( const TPixelFormatID iniDestNo )
{
	if(iniDestNo==-1)
		return iniDestNo;

	switch(g_pixelformats[iniDestNo].DxtNo)
	{
		case D3DFMT_A32B32G32R32F: return GetNoFromD3DFormat(D3DFMT_A32B32G32R32F); // for HDR tiffs
		default: ;
	}

	return GetNoFromD3DFormat(D3DFMT_A8R8G8B8);		// for all other textures by default A8R8G8B8
}

TPixelFormatID CPixelFormats::GetNoFromD3DFormat( D3DFORMAT inFormat )
{
	int iCount=GetPixelFormatCount();

	for(int i=0;i<iCount;i++)
		if(g_pixelformats[i].DxtNo==inFormat)
			return i;

	return -1;
}


const char *CPixelFormats::GetPixelFormatName( const TPixelFormatID iniNo )
{
	if(iniNo==-1)
		return g_szInvalidPixelFormatName;

	assert(iniNo>=0);
	assert(iniNo<GetPixelFormatCount());

	return g_pixelformats[iniNo].szName;
}

TPixelFormatID CPixelFormats::GetPixelFormatNoFromDestNo( const TPixelFormatID iniDestNo )
{
	int iCount=GetPixelFormatCount();

	int iCurrent=0;
	for(int i=0;i<iCount;i++)
	{
		if(g_pixelformats[i].bDestinationFormat)
		{
			if(iniDestNo==iCurrent)
				return i;

			iCurrent++;
		}
	}

	return -1;
}

TPixelFormatID CPixelFormats::GetDestNoFromPixelFormatNo( const TPixelFormatID iniPixelFmtNo )
{
	int iCount=GetPixelFormatCount();

	int iCurrent=0;
	for(int i=0;i<iCount;i++)
	{
		if(g_pixelformats[i].bDestinationFormat)
		{
			if(iniPixelFmtNo==i)
				return iCurrent;

			iCurrent++;
		}
	}

	return -1;
}

TPixelFormatID CPixelFormats::GetPixelFormatUncompressed( const TPixelFormatID iniNo )
{
	if(iniNo==-1)
		return iniNo;

	assert(iniNo>=0);
	assert(iniNo<GetPixelFormatCount());

	int iRet=GetNoFromName(g_pixelformats[iniNo].szUncompressedAltName);

	assert(iRet!=-1);			// szUncompressedAltName must be in szName

	return iRet;
}


int CPixelFormats::GetPixelFormatCount()
{
	return sizeof(g_pixelformats)/sizeof(SPixelFormats);
}


SPixelFormats *CPixelFormats::GetPixelFormatInfo( const TPixelFormatID iniNo )
{
	if(iniNo==-1)
		return 0;


	assert(iniNo>=0);
	assert(iniNo<GetPixelFormatCount());

	return &g_pixelformats[iniNo];
}

char *CPixelFormats::GetPixelFormatDesc( const TPixelFormatID iniNo )
{
	if(iniNo==-1)
		return "*";

	assert(iniNo>=0);
	assert(iniNo<GetPixelFormatCount());

	return g_pixelformats[iniNo].szDescription;
}


void CPixelFormats::BuildSurfaceDesc( ImageObject *pImage, DDSURFACEDESC2 &surf )
{
	assert(pImage);

	uint32 dwWidth,dwMips,dwDepth,dwSides,dwHeight;

	pImage->GetExtend(dwWidth,dwHeight,dwDepth,dwSides,dwMips);

	SPixelFormats *pPixelFormat = CPixelFormats::GetPixelFormatInfo(pImage->GetPixelFormat());

	memset(&surf,0,sizeof(DDSURFACEDESC2));
	surf.dwSize=sizeof(DDSURFACEDESC2);

	surf.dwFlags=DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_WIDTH|DDSD_HEIGHT;
	surf.dwWidth=dwWidth;
	surf.dwHeight=dwHeight;
	surf.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);

	if (pImage->GetImageFlags() & CImageExtensionHelper::EIF_Cubemap)
	{
		assert(pImage->IsCubemap());
		surf.ddsCaps.dwCaps |= DDS_SURFACE_FLAGS_CUBEMAP;
		surf.ddsCaps.dwCaps2 |= DDS_CUBEMAP_ALLFACES;
		assert(surf.dwWidth%6==0);
		surf.dwWidth/=6;
	}

	if(pImage->GetPixelFormat()==CPixelFormats::GetNoFromD3DFormat(D3DFMT_A8R8G8B8) 
		|| pImage->GetPixelFormat()==CPixelFormats::GetNoFromD3DFormat(D3DFMT_X8R8G8B8))
	{
		surf.ddpfPixelFormat.dwFlags = DDPF_RGB;
		surf.ddpfPixelFormat.dwRGBBitCount=pPixelFormat->iBitsPerPixel;
		surf.ddpfPixelFormat.dwRGBAlphaBitMask=0xff000000;
		surf.ddpfPixelFormat.dwRBitMask=0x00ff0000;
		surf.ddpfPixelFormat.dwGBitMask=0x0000ff00;
		surf.ddpfPixelFormat.dwBBitMask=0x000000ff;
	}
	else if(pImage->GetPixelFormat()==CPixelFormats::GetNoFromD3DFormat(D3DFMT_L8))
	{
		surf.ddpfPixelFormat.dwFlags = DDPF_LUMINANCE;
		surf.ddpfPixelFormat.dwLuminanceBitCount = 8;
		surf.ddpfPixelFormat.dwLuminanceBitMask=0xff;
	}
	else
	{
		surf.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		surf.ddpfPixelFormat.dwFourCC = pPixelFormat->DxtNo;
	}

	if(pImage->HasAlpha())
		surf.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;

	surf.ddsCaps.dwCaps|=DDSCAPS_TEXTURE;

	if(dwMips>0)
	{
		surf.dwFlags|=DDSD_MIPMAPCOUNT;
		surf.dwMipMapCount=dwMips;
		surf.ddsCaps.dwCaps|=DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
	}

	// non standardized way to expose some features in the header (same information is in attached chunk but then
	// streaming would need to find this spot in the file)
	// if this is causing problems we need to change it 
	surf.dwTextureStage = 'CRYF';
	surf.dwReserved = pImage->GetImageFlags();
}
