////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   image_dxtc.cpp
//  Version:     v1.00
//  Created:     5/9/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Image_DXTC.h"
#include <CryFile.h>
#include "BitFiddling.h"

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)
#include "ImageExtensionHelper.h"


//////////////////////////////////////////////////////////////////////////
// DXT and 3DC related data structures

// 16 bit 565 RGB color
struct ColorR5G6B5
{
public:
	ColorR5G6B5() 
	{
	}

	ColorR5G6B5(const ColorR5G6B5& a_c) 
		: u(a_c.u) 
	{
	}

	explicit ColorR5G6B5(uint16 a_u) 
		: u(a_u) 
	{
	}
	
	union 
	{
		struct 
		{
			uint16 b : 5;
			uint16 g : 6;
			uint16 r : 5;
		};
		uint16 u;
	};
};

// 32 bit 8888 BGRA color
struct ColorB8G8R8A8
{
public:
	ColorB8G8R8A8() 
	{
	}

	ColorB8G8R8A8(const ColorB8G8R8A8& a_c) 
		: u(a_c.u) 
	{ 
	}

	ColorB8G8R8A8(uint8 B, uint8 G, uint8 R, uint8 A) 
	{
		setBGRA(B, G, R, A); 
	}
	explicit ColorB8G8R8A8(uint32 a_u) 
		: u(a_u) 
	{ 
	}

	void setBGRA(uint8 B, uint8 G, uint8 R, uint8 A)
	{
		b = B;
		g = G;
		r = R;
		a = A;
	}
	
	union 
	{
		struct 
		{
			uint8 b;
			uint8 g;
			uint8 r;
			uint8 a;
		};
		uint32 u;
	};
};

struct BlockDXT1
{
	ColorR5G6B5 col0;
	ColorR5G6B5 col1;
	union 
	{
		struct 
		{
			uint32 bits0 : 2;
			uint32 bits1 : 2;
			uint32 bits2 : 2;
			uint32 bits3 : 2;

			uint32 bits4 : 2;
			uint32 bits5 : 2;
			uint32 bits6 : 2;
			uint32 bits7 : 2;

			uint32 bits8 : 2;
			uint32 bits9 : 2;
			uint32 bitsA : 2;
			uint32 bitsB : 2;

			uint32 bitsC : 2;
			uint32 bitsD : 2;
			uint32 bitsE : 2;
			uint32 bitsF : 2;
		};
		uint8 row[4];
		uint32 indices;
	};

	void evaluatePalette3or4(ColorB8G8R8A8 pal[4]) const
	{
		pal[0].b = (col0.b << 3) | (col0.b >> 2);
		pal[0].g = (col0.g << 2) | (col0.g >> 4);
		pal[0].r = (col0.r << 3) | (col0.r >> 2);
		pal[0].a = 255;

		pal[1].b = (col1.b << 3) | (col1.b >> 2);
		pal[1].g = (col1.g << 2) | (col1.g >> 4);
		pal[1].r = (col1.r << 3) | (col1.r >> 2);
		pal[1].a = 255;

		if(col0.u > col1.u) 
		{
			pal[2].b = (2 * pal[0].b + pal[1].b + 1) / 3;
			pal[2].g = (2 * pal[0].g + pal[1].g + 1) / 3;
			pal[2].r = (2 * pal[0].r + pal[1].r + 1) / 3;
			pal[2].a = 255;
									   
			pal[3].b = (2 * pal[1].b + pal[0].b + 1) / 3;
			pal[3].g = (2 * pal[1].g + pal[0].g + 1) / 3;
			pal[3].r = (2 * pal[1].r + pal[0].r + 1) / 3;
			pal[3].a = 255;
		}
		else
		{
			pal[2].b = (pal[0].b + pal[1].b + 1) >> 1;
			pal[2].g = (pal[0].g + pal[1].g + 1) >> 1;
			pal[2].r = (pal[0].r + pal[1].r + 1) >> 1;
			pal[2].a = 255;
								   
			pal[3].b = 0;
			pal[3].g = 0;
			pal[3].r = 0;
			pal[3].a = 0;
		}
	}
	
	// note: keeps existing alpha color in the palette
	void evaluatePalette4KeepAlpha(ColorB8G8R8A8 pal[4]) const
	{
		pal[0].b = (col0.b << 3) | (col0.b >> 2);
		pal[0].g = (col0.g << 2) | (col0.g >> 4);
		pal[0].r = (col0.r << 3) | (col0.r >> 2);

		pal[1].b = (col1.b << 3) | (col1.b >> 2);
		pal[1].g = (col1.g << 2) | (col1.g >> 4);
		pal[1].r = (col1.r << 3) | (col1.r >> 2);

		pal[2].b = (2 * pal[0].b + pal[1].b + 1) / 3;
		pal[2].g = (2 * pal[0].g + pal[1].g + 1) / 3;
		pal[2].r = (2 * pal[0].r + pal[1].r + 1) / 3;
								   
		pal[3].b = (2 * pal[1].b + pal[0].b + 1) / 3;
		pal[3].g = (2 * pal[1].g + pal[0].g + 1) / 3;
		pal[3].r = (2 * pal[1].r + pal[0].r + 1) / 3;
	}

	void decodeB8G8R8A8(const ColorB8G8R8A8 a_pal[4], void* const a_dst, int const a_rowStrideInBytes) const
	{
		ColorB8G8R8A8* dst = (ColorB8G8R8A8*)a_dst;
		dst[0] = a_pal[bits0];
		dst[1] = a_pal[bits1];
		dst[2] = a_pal[bits2];
		dst[3] = a_pal[bits3];

		dst = (ColorB8G8R8A8*) (((uint8*)dst) + a_rowStrideInBytes);
		dst[0] = a_pal[bits4];
		dst[1] = a_pal[bits5];
		dst[2] = a_pal[bits6];
		dst[3] = a_pal[bits7];

		dst = (ColorB8G8R8A8*) (((uint8*)dst) + a_rowStrideInBytes);
		dst[0] = a_pal[bits8];
		dst[1] = a_pal[bits9];
		dst[2] = a_pal[bitsA];
		dst[3] = a_pal[bitsB];

		dst = (ColorB8G8R8A8*) (((uint8*)dst) + a_rowStrideInBytes);
		dst[0] = a_pal[bitsC];
		dst[1] = a_pal[bitsD];
		dst[2] = a_pal[bitsE];
		dst[3] = a_pal[bitsF];
	}

	template <int dstStepInBytes>
	void decodeB8G8R8(const ColorB8G8R8A8 a_pal[4], void* const a_dst, int const a_rowStrideInBytes) const
	{
		const uint8* src;

		// First row
		uint8* dst = (uint8*)a_dst;

		src = (const uint8*) &a_pal[bits0];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bits1];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits2];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits3];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];

		// Second row
		dst += a_rowStrideInBytes;

		src = (const uint8*) &a_pal[bits4];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bits5];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits6];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bits7];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];

		// Third row
		dst += a_rowStrideInBytes;

		src = (const uint8*) &a_pal[bits8];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bits9];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsA];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsB];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];

		// Fourth row
		dst += a_rowStrideInBytes;

		src = (const uint8*) &a_pal[bitsC];
		dst[                 0] = src[0];
		dst[                 1] = src[1];
		dst[                 2] = src[2];

		src = (const uint8*) &a_pal[bitsD];
		dst[  dstStepInBytes+0] = src[0];
		dst[  dstStepInBytes+1] = src[1];
		dst[  dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsE];
		dst[2*dstStepInBytes+0] = src[0];
		dst[2*dstStepInBytes+1] = src[1];
		dst[2*dstStepInBytes+2] = src[2];

		src = (const uint8*) &a_pal[bitsF];
		dst[3*dstStepInBytes+0] = src[0];
		dst[3*dstStepInBytes+1] = src[1];
		dst[3*dstStepInBytes+2] = src[2];
	}
};

struct AlphaBlockDXT3
{
	union 
	{
		struct 
		{
			uint32 alpha0 : 4;
			uint32 alpha1 : 4;
			uint32 alpha2 : 4;
			uint32 alpha3 : 4;
			uint32 alpha4 : 4;
			uint32 alpha5 : 4;
			uint32 alpha6 : 4;
			uint32 alpha7 : 4;
			uint32 alpha8 : 4;
			uint32 alpha9 : 4;
			uint32 alphaA : 4;
			uint32 alphaB : 4;
			uint32 alphaC : 4;
			uint32 alphaD : 4;
			uint32 alphaE : 4;
			uint32 alphaF : 4;
		};
		uint16 row[4];
		uint64 u;
	};
	
	template <int dstStepInBytes>
	void decode(void* const a_dst, int const a_rowStrideInBytes) const
	{
		uint8* dst = (uint8*) a_dst;
		dst[               0] = (alpha0 << 4) | alpha0;
		dst[  dstStepInBytes] = (alpha1 << 4) | alpha1;;
		dst[2*dstStepInBytes] = (alpha2 << 4) | alpha2;;
		dst[3*dstStepInBytes] = (alpha3 << 4) | alpha3;;

		dst += a_rowStrideInBytes;
		dst[               0] = (alpha4 << 4) | alpha4;
		dst[  dstStepInBytes] = (alpha5 << 4) | alpha5;
		dst[2*dstStepInBytes] = (alpha6 << 4) | alpha6;
		dst[3*dstStepInBytes] = (alpha7 << 4) | alpha7;

		dst += a_rowStrideInBytes;
		dst[               0] = (alpha8 << 4) | alpha8;
		dst[  dstStepInBytes] = (alpha9 << 4) | alpha9;
		dst[2*dstStepInBytes] = (alphaA << 4) | alphaA;
		dst[3*dstStepInBytes] = (alphaB << 4) | alphaB;

		dst += a_rowStrideInBytes;
		dst[               0] = (alphaC << 4) | alphaC;
		dst[  dstStepInBytes] = (alphaD << 4) | alphaD;
		dst[2*dstStepInBytes] = (alphaE << 4) | alphaE;
		dst[3*dstStepInBytes] = (alphaF << 4) | alphaF;
	}
};

struct AlphaBlockDXT5
{
	union 
	{
		struct 
		{
			uint8 alpha0;
			uint8 alpha1;
			uint8 bitcodes[6];
		};
		struct 
		{
			uint64 a0    : 8; // 8
			uint64 a1    : 8; // 16
			uint64 bits0 : 3; // 3 - 19
			uint64 bits1 : 3; // 6 - 22
			uint64 bits2 : 3; // 9 - 25
			uint64 bits3 : 3; // 12 - 28
			uint64 bits4 : 3; // 15 - 31
			uint64 bits5 : 3; // 18 - 34
			uint64 bits6 : 3; // 21 - 37
			uint64 bits7 : 3; // 24 - 40
			uint64 bits8 : 3; // 27 - 43
			uint64 bits9 : 3; // 30 - 46
			uint64 bitsA : 3; // 33 - 49
			uint64 bitsB : 3; // 36 - 52
			uint64 bitsC : 3; // 39 - 55
			uint64 bitsD : 3; // 42 - 58
			uint64 bitsE : 3; // 45 - 61
			uint64 bitsF : 3; // 48 - 64
		};
		uint64 u;
	};
	
	void evaluatePalette(uint8 alpha[8]) const
	{
		if(alpha0 > alpha1) 
		{
			// 8-alpha block
			alpha[0] = alpha0;
			alpha[1] = alpha1;
			alpha[2] = (6 * alpha[0] +     alpha[1] + 3) / 7;
			alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 3) / 7;
			alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 3) / 7;
			alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 3) / 7;
			alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 3) / 7;
			alpha[7] = (    alpha[0] + 6 * alpha[1] + 3) / 7;
		}
		else 
		{
			// 6-alpha block.
			alpha[0] = alpha0;
			alpha[1] = alpha1;
			alpha[2] = (4 * alpha[0] +     alpha[1] + 2) / 5;
			alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 2) / 5;
			alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 2) / 5;
			alpha[5] = (    alpha[0] + 4 * alpha[1] + 2) / 5;
			alpha[6] = 0x00;
			alpha[7] = 0xFF;
		}
	}

	template <int dstStepInBytes>
	void decode(void* const a_dst, int const a_rowStrideInBytes) const
	{
		uint8 pal[8]; 
		evaluatePalette(pal);

		uint8* dst = (uint8*) a_dst;
		dst[               0] = pal[bits0];
		dst[  dstStepInBytes] = pal[bits1];
		dst[2*dstStepInBytes] = pal[bits2];
		dst[3*dstStepInBytes] = pal[bits3];

		dst += a_rowStrideInBytes;
		dst[               0] = pal[bits4];
		dst[  dstStepInBytes] = pal[bits5];
		dst[2*dstStepInBytes] = pal[bits6];
		dst[3*dstStepInBytes] = pal[bits7];

		dst += a_rowStrideInBytes;
		dst[               0] = pal[bits8];
		dst[  dstStepInBytes] = pal[bits9];
		dst[2*dstStepInBytes] = pal[bitsA];
		dst[3*dstStepInBytes] = pal[bitsB];

		dst += a_rowStrideInBytes;
		dst[               0] = pal[bitsC];
		dst[  dstStepInBytes] = pal[bitsD];
		dst[2*dstStepInBytes] = pal[bitsE];
		dst[3*dstStepInBytes] = pal[bitsF];
	}
};

struct BlockDXT3
{
	AlphaBlockDXT3 alpha;
	BlockDXT1 color;
};

struct BlockDXT5
{
	AlphaBlockDXT5 alpha;
	BlockDXT1 color;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CImage_DXTC::CImage_DXTC()
{
}
//////////////////////////////////////////////////////////////////////////
CImage_DXTC::~CImage_DXTC()
{
}
//////////////////////////////////////////////////////////////////////////
bool CImage_DXTC::Load( const char *filename,CImage &outImage, bool *pQualityLoss )
{
	if(pQualityLoss)
		*pQualityLoss=false;

	CCryFile file;
	if (!file.Open( filename,"rb" ))
	{
		return( false );
	}

	CImageExtensionHelper::DDS_HEADER ddsd;
	DWORD			dwMagic;

	BYTE	*pCompBytes;		// compressed image bytes
	BYTE	*pDecompBytes;


	// Read magic number
	file.ReadType( &dwMagic );

	if( dwMagic != MAKEFOURCC('D','D','S',' ') )
	{
		return( false);
	}

	// Read the surface description
	file.ReadTypeRaw( &ddsd );


	// Does texture have mipmaps?
	bool bMipTexture = ( ddsd.dwMipMapCount > 0 ) ? TRUE : FALSE;

	ETEX_Format format = DDSFormats::GetFormatByDesc(ddsd.ddspf);

	if (format == eTF_Unknown)
	{
		return false;
	}

	int  nHorizontalFaces(1);
	int  nVerticalFaces(1);
	int  nNumFaces(1);
	int  nMipMapsSize(0);
	int  nMipMapCount(ddsd.GetMipCount());
	int  nInputBytesPerPixel(ddsd.ddspf.dwRGBBitCount/8);
	int  nSourcePitch(ddsd.dwWidth*nInputBytesPerPixel);// In compressed formats this 
																											// can be negative..eventually 
																											// we will need to handle this.
	int nSourcePageSize(nSourcePitch*ddsd.dwHeight);		// Again, we may need to adjust
																											// the pitch.
	int nTargetPitch(ddsd.dwWidth*4);
	int nTargetPageSize(nTargetPitch*ddsd.dwHeight);

	int nHorizontalPageOffset(nTargetPitch);
	int nVerticalPageOffset(0);
	bool boIsCubemap(false);
	if ((ddsd.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) && (ddsd.dwCubemapFlags & DDS_CUBEMAP_ALLFACES))
	{
		boIsCubemap=true;

		// If this is a format we can display properly as a cubemap...
		if (
				(format == eTF_A8R8G8B8)
					||
				(format == eTF_X8R8G8B8)
					||
				(format == eTF_R8G8B8)
					||
				(format == eTF_L8)
					||
				(format == eTF_A8)
			)
		{
			nHorizontalFaces=3;
			nVerticalFaces=2;
			nHorizontalPageOffset=nTargetPitch*nHorizontalFaces;
			nVerticalPageOffset=nTargetPageSize*nHorizontalFaces;
		}
	}

	for (int nCurrentMipMap=0;nCurrentMipMap<nMipMapCount;++nCurrentMipMap)
	{
		nMipMapsSize+=(nSourcePageSize)>>(2+2*nCurrentMipMap);
	}

	int nCompSize = file.GetLength() - sizeof(CImageExtensionHelper::DDS_HEADER) - sizeof(DWORD);
	pCompBytes = new BYTE[nCompSize+32];
	file.ReadRaw( pCompBytes, nCompSize );

	/*
	// Read only first mip level for now:
	if (ddsd.dwFlags & DDSD_LINEARSIZE)
	{
	pCompBytes = new BYTE[ddsd.dwLinearSize];
	if (pCompBytes == NULL )
	{
	return( false );
	}
	file.Read( pCompBytes, ddsd.dwLinearSize );
	}
	else
	{
	DWORD dwBytesPerRow = ddsd.dwWidth * ddsd.ddspf.dwRGBBitCount / 8;
	pCompBytes = new BYTE[ddsd.lPitch*ddsd.dwHeight];
	if (pCompBytes == NULL )
	{
	return( false );
	}

	nCompSize = ddsd.lPitch * ddsd.dwHeight;
	nCompLineSz = dwBytesPerRow;

	BYTE* pDest = pCompBytes;
	for( DWORD yp = 0; yp < ddsd.dwHeight; yp++ )
	{
	file.Read( pDest, dwBytesPerRow );
	pDest += ddsd.lPitch;
	}
	}
	*/

	outImage.Allocate( ddsd.dwWidth*nHorizontalFaces,ddsd.dwHeight*nVerticalFaces);
	pDecompBytes = (BYTE*)outImage.GetData();
	if (!pDecompBytes)
	{
		Warning( "Cannot allocate image %dx%d, Out of memory",ddsd.dwWidth,ddsd.dwHeight );
		delete []pCompBytes;
		return false;
	}

	if(pQualityLoss)
	{
		if(format==eTF_A4R4G4B4
			|| format==eTF_R5G6B5
			|| format==eTF_R5G5B5
			|| format==eTF_DXT1
			|| format==eTF_DXT3
			|| format==eTF_DXT5
			|| format==eTF_3DC)
			*pQualityLoss=true;
	}

	bool bOk = true;

	int nCurrentFace(0);
	int nCurrentHorizontalFace(0);
	int nCurrentVerticalFace(0);
	unsigned char *dest(NULL);
	unsigned char *src(NULL);

	unsigned char *basedest(NULL);
	unsigned char *basesrc(NULL);

	for (nCurrentHorizontalFace=0;nCurrentHorizontalFace<nHorizontalFaces;++nCurrentHorizontalFace)
	{
		basedest = &pDecompBytes[nTargetPitch*nCurrentHorizontalFace]; // Horizontal offset.
		for (nCurrentVerticalFace=0;nCurrentVerticalFace<nVerticalFaces;++nCurrentVerticalFace,++nCurrentFace)
		{
			basesrc =    &pCompBytes
				[
					nSourcePageSize*nCurrentFace+ //Face offset
					nMipMapsSize*nCurrentFace // Mipmap offset.
				];

			basedest+= nVerticalPageOffset*nCurrentVerticalFace; // Vertical offset.

			if (format == eTF_A8R8G8B8)
			{
				if (ddsd.ddspf.dwRGBBitCount == 24)
				{
					for (int y = 0; y < ddsd.dwHeight; y++)
					{
						src = basesrc  + nSourcePitch*y; //Scanline position.
						dest= basedest + nHorizontalPageOffset*y; // Pixel position.

						for (int x = 0; x < ddsd.dwWidth; x++)
						{
							dest[0] = src[2];
							dest[1] = src[1];
							dest[2] = src[0];
							dest[3] = 255;
							dest+=4;
							src+=3;
						}
					}
				}
				else if (ddsd.ddspf.dwRGBBitCount == 32)
				{
					for (int y = 0; y < ddsd.dwHeight; y++)
					{
						src = basesrc  + nSourcePitch*y; //Scanline position.
						dest= basedest + nHorizontalPageOffset*y; // Pixel position.

						for (int x = 0; x < ddsd.dwWidth; x++)
						{
							dest[0] = src[2];
							dest[1] = src[1];
							dest[2] = src[0];
							dest[3] = src[3];
							dest+=nInputBytesPerPixel;
							src+=nInputBytesPerPixel;
						}
					}
				}
			}
			else if (format == eTF_X8R8G8B8)
			{
				for (int y = 0; y < ddsd.dwHeight; y++)
				{
					src = basesrc  + nSourcePitch*y; //Scanline position.
					dest= basedest + nHorizontalPageOffset*y; // Pixel position.

					for (int x = 0; x < ddsd.dwWidth; x++)
					{
						dest[0] = src[2];
						dest[1] = src[1];
						dest[2] = src[0];
						dest[3] = 255;
						dest+=4;
						src+=4;
					}
				}
			}
			else if (format == eTF_R8G8B8)
			{
				for (int y = 0; y < ddsd.dwHeight; y++)
				{
					src = basesrc  + nSourcePitch*y; //Scanline position.
					dest= basedest + nHorizontalPageOffset*y; // Pixel position.

					for (int x = 0; x < ddsd.dwWidth; x++)
					{
						dest[0] = src[2];
						dest[1] = src[1];
						dest[2] = src[0];
						dest[3] = 255;
						dest+=4;
						src+=3;
					}
				}
			}
			else if (format == eTF_L8)
			{
				for (int y = 0; y < ddsd.dwHeight; y++)
				{
					src = basesrc  + nSourcePitch*y; //Scanline position.
					dest= basedest + nHorizontalPageOffset*y; // Pixel position.

					for (int x = 0; x < ddsd.dwWidth; x++)
					{
						dest[0] = *src;
						dest[1] = *src;
						dest[2] = *src;
						dest[3] = 255;
						dest+=4;
						++src;
					}
				}
			}
			else if (format == eTF_A8)
			{
				for (int y = 0; y < ddsd.dwHeight; y++)
				{
					src = basesrc  + nSourcePitch*y; //Scanline position.
					dest= basedest + nHorizontalPageOffset*y; // Pixel position.

					for (int x = 0; x < ddsd.dwWidth; x++)
					{				
						dest[0] = *src;
						dest[1] = *src;
						dest[2] = *src;
						dest[3] = *src;
						dest+=4;
						++src;
					}
				}
			}
			else if (format == eTF_3DC)
			{
				const byte* const srcData = pCompBytes;
				const int width = ddsd.dwWidth;
				const int height = ddsd.dwHeight;
				void* outputBuffer = pDecompBytes;
				const int pixelCount = width * height;
				const int outputBufferSize = pixelCount * 4;
				const int mipCount = ddsd.dwMipMapCount;
											 
				const COMPRESSOR_ERROR err = DecompressTextureATI2N(width, height,FORMAT_COMP_ATI2N,FORMAT_ARGB_8888,srcData,outputBuffer,outputBufferSize);
				if (err != COMPRESSOR_ERROR_NONE)
				{
					delete []pCompBytes;
					return false;
				}

				// Convert BGRA to RGBA
				{
					uint8* p = (uint8*) outputBuffer;
					for (int n=0; n < pixelCount; ++n)
					{
						const uint8 t = p[n*4+0];
						p[n*4+0] = p[n*4+2];
						p[n*4+2] = t;
					}
				}				

				// alpha channel might be attached (currently only in 3DC)
				{
					size_t nSizeDDS = CImage_DXTC::TextureDataSize(width,height,1,mipCount,format);

					const uint8 *pMem = 0;

					if (file.GetLength()-sizeof(CImageExtensionHelper::DDS_HEADER)-4 > nSizeDDS)
					{
						pMem = srcData+nSizeDDS;
					}

					CImageExtensionHelper::DDS_HEADER *pDDSHeader = (CImageExtensionHelper::DDS_HEADER *)CImageExtensionHelper::GetAttachedImage(pMem);

					if(pDDSHeader)
					{
						const uint8 *pL8Data = (uint8 *)(pDDSHeader)+sizeof(CImageExtensionHelper::DDS_HEADER);		// assuming it's A8 format (ensured with assets when loading)

						// assuming attached image can have lower res and difference is power of two
						const uint32 reducex = IntegerLog2((uint32)(width/pDDSHeader->dwWidth));
						const uint32 reducey = IntegerLog2((uint32)(height/pDDSHeader->dwHeight));

						uint8* pDst = ((uint8*)outputBuffer) + 3;	// point to alpha channel of the resulting image

						for(int y=0;y<height;++y)
						{
							for(int x=0;x<width;++x)
							{
								pDst[0] = pL8Data[(x>>reducex)+(y>>reducey)*pDDSHeader->dwWidth];
								pDst += 4;
							}
						}
					}
				}
			}
			else /*if (format == eTF_DXT1 || format == eTF_DXT3 || format == eTF_DXT5)*/
			{
				const byte* const srcData = pCompBytes;
				const int width = ddsd.dwWidth;
				const int height = ddsd.dwHeight;
				void* outputBuffer = pDecompBytes;
				const int pixelCount = width * height;
				const int outputBufferSize = pixelCount * 4;
				const int mipCount = ddsd.dwMipMapCount;
											 
				const COMPRESSOR_ERROR err = DecompressTextureDXT(width, height,format,FORMAT_ARGB_8888,srcData,outputBuffer,outputBufferSize);
				if (err != COMPRESSOR_ERROR_NONE)
				{
					delete []pCompBytes;
					return false;
				}

				// Convert BGRA to RGBA
				{
					uint8* p = (uint8*) outputBuffer;
					for (int n=0; n < pixelCount; ++n)
					{
						const uint8 t = p[n*4+0];
						p[n*4+0] = p[n*4+2];
						p[n*4+2] = t;
					}
				}				
			}
		}
	}

	delete []pCompBytes;

	//////////////////////////////////////////////////////////////////////////
	CString strFormat = NameForTextureFormat(format);
	CString mips;
	mips.Format( " Mips:%d",ddsd.dwMipMapCount );
	strFormat += mips;

	outImage.SetFormatDescription( strFormat );
	outImage.SetNumberOfMipMaps(ddsd.dwMipMapCount);
	outImage.SetIsCubemap(boIsCubemap);
	outImage.SetFormat(format);

	// done reading file
	return bOk;
}
//////////////////////////////////////////////////////////////////////////
int CImage_DXTC::TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF)
{
	if (eTF == eTF_Unknown)
		return 0;

	if (nMips <= 0)
		nMips = 0;
	int nSize = 0;
	int nM = 0;
	while (nWidth || nHeight || nDepth)
	{
		if (!nWidth)
			nWidth = 1;
		if (!nHeight)
			nHeight = 1;
		if (!nDepth)
			nDepth = 1;
		nM++;

		int nSingleMipSize;
		if (IsDXTCompressed(eTF))
		{
			int blockSize = (eTF == eTF_DXT1) ? 8 : 16;
			nSingleMipSize = ((nWidth+3)/4)*((nHeight+3)/4)*nDepth*blockSize;
		}
		else
			nSingleMipSize = nWidth * nHeight * nDepth * BitsPerPixel(eTF)/8;
		nSize += nSingleMipSize;

		nWidth >>= 1;
		nHeight >>= 1;
		nDepth >>= 1;
		if (nMips == nM)
			break;
	}
	//assert (nM == nMips);

	return nSize;
}
//////////////////////////////////////////////////////////////////////////
int CImage_DXTC::BitsPerPixel(ETEX_Format eTF)
{
	switch (eTF)
	{
	case eTF_A8R8G8B8:
	case eTF_X8R8G8B8:
	case eTF_X8L8V8U8:
	case eTF_A2R10G10B10:




		return 32;
	case eTF_A4R4G4B4:
		return 16;
	case eTF_R8G8B8:
		return 24;
	case eTF_L8V8U8:
		return 24;
	case eTF_V8U8:
		return 16;
	case eTF_R5G6B5:
	case eTF_R5G5B5:
		return 16;
	case eTF_A16B16G16R16:
	case eTF_A16B16G16R16F:
		return 64;
	case eTF_A32B32G32R32F:
		return 128;
	case eTF_G16R16:
	case eTF_G16R16F:
	case eTF_V16U16:
		return 32;
	case eTF_A8:



	case eTF_L8:
		return 8;
	case eTF_A8L8:
		return 16;
	case eTF_R32F:
		return 32;
	case eTF_R16F:
		return 16;
		//case eTF_DXT1:
		//  return 16;
		//case eTF_DXT3:
		//case eTF_DXT5:
	case eTF_3DC:
		  return 16;
	case 	eTF_DEPTH24:
		return 32;

	case 	eTF_DF16:
		return 16;
	case 	eTF_DF24:
		return 24;
	case 	eTF_D16:
		return 16;
	case 	eTF_D24S8:
		return 32;
	case 	eTF_D32F:
		return 32;

	case 	eTF_NULL:
		return 8;

	default:
		assert(0);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
const char* CImage_DXTC::NameForTextureFormat(ETEX_Format ETF)
{
	char *sETF;
	switch (ETF)
	{
	case eTF_R8G8B8:
		sETF = "R8G8B8";
		break;
	case eTF_A8R8G8B8:
		sETF = "A8R8G8B8";
		break;
	case eTF_X8R8G8B8:
		sETF = "X8R8G8B8";
		break;
	case eTF_A8:
		sETF = "A8";
		break;
	case eTF_A8L8:
		sETF = "A8L8";
		break;
	case eTF_L8:
		sETF = "L8";
		break;
	case eTF_A4R4G4B4:
		sETF = "A4R4G4B4";
		break;
	case eTF_R5G6B5:
		sETF = "R5G6B5";
		break;
	case eTF_R5G5B5:
		sETF = "R5G5B5";
		break;
	case eTF_DXT1:
		sETF = "DXT1";
		break;
	case eTF_DXT3:
		sETF = "DXT3";
		break;
	case eTF_DXT5:
		sETF = "DXT5";
		break;
	case eTF_3DC:
		sETF = "3DC";
		break;
	case eTF_V16U16:
		sETF = "V16U16";
		break;
	case eTF_X8L8V8U8:
		sETF = "X8L8V8U8";
		break;
	case eTF_V8U8:
		sETF = "V8U8";
		break;
	case eTF_L8V8U8:
		sETF = "L8V8U8";
		break;
	case eTF_A16B16G16R16F:
		sETF = "A16B16G16R16F";
		break;
	case eTF_A16B16G16R16:
		sETF = "A16B16G16R16";
		break;
	case eTF_A32B32G32R32F:
		sETF = "A32B32G32R32F";
		break;
	case eTF_G16R16F:
		sETF = "G16R16F";
		break;
	case eTF_R16F:
		sETF = "R16F";
		break;
	case eTF_R32F:
		sETF = "R32F";
		break;
	default:
		sETF = "Unknown";		// for better behaviour in non debug
		break;
	}
	return sETF;
}
//////////////////////////////////////////////////////////////////////////
CImage_DXTC::COMPRESSOR_ERROR CImage_DXTC::CheckParameters(
	int width,
	int height,
	CImage_DXTC::UNCOMPRESSED_FORMAT destinationFormat,
	const void* sourceData,
	void* destinationData,
	int destinationDataSize)
{
	const int blockWidth = 4;
	const int blockHeight = 4;

	const int bgraPixelSize = sizeof(ColorB8G8R8A8);
	const int bgraRowSize = bgraPixelSize * width;

	if((width<=0)||(height<=0)||(!sourceData))
	{
		return COMPRESSOR_ERROR_NO_INPUT_DATA;
	}

	if((width%blockWidth)||(height%blockHeight))
	{
		return COMPRESSOR_ERROR_GENERIC;
	}
		
	if((destinationData == 0) || (destinationDataSize <= 0))
	{
		return COMPRESSOR_ERROR_NO_OUTPUT_POINTER;
	}
		
	if(destinationFormat != FORMAT_ARGB_8888)
	{
		return COMPRESSOR_ERROR_UNSUPPORTED_DESTINATION_FORMAT;
	}

	if((height*bgraRowSize<=0)||(height*bgraRowSize>destinationDataSize))
	{
		return COMPRESSOR_ERROR_GENERIC;
	}

	return COMPRESSOR_ERROR_NONE;
}
//////////////////////////////////////////////////////////////////////////
CImage_DXTC::COMPRESSOR_ERROR CImage_DXTC::DecompressTextureDXT(
	int width,
	int height,
	ETEX_Format sourceFormat,
	CImage_DXTC::UNCOMPRESSED_FORMAT destinationFormat,
	const void* sourceData,
	void* destinationData,
	int destinationDataSize)
{
	if((sourceFormat!=eTF_DXT1) && (sourceFormat!=eTF_DXT3) && (sourceFormat!=eTF_DXT5))
	{
		return COMPRESSOR_ERROR_UNSUPPORTED_SOURCE_FORMAT;
	}

	{
		const COMPRESSOR_ERROR result = CheckParameters(
			width,
			height,
			destinationFormat,
			sourceData,
			destinationData,
			destinationDataSize);

		if(result != COMPRESSOR_ERROR_NONE)
		{
			return result;
		}
	}

	const int blockWidth = 4;
	const int blockHeight = 4;

	const int bgraPixelSize = sizeof(ColorB8G8R8A8);
	const int bgraRowSize = bgraPixelSize * width;

	ColorB8G8R8A8 pal[4];

	if(sourceFormat == eTF_DXT1) 
	{
		const BlockDXT1* src = (const BlockDXT1*) sourceData;

		for(int y=0; y<height; y+=blockHeight)
		{
			uint8* dst = ((uint8*)destinationData) + (y * bgraRowSize);

			for(int x=0; x<width; x+=blockWidth)
			{
				src[0].evaluatePalette3or4(pal);
				src[0].decodeB8G8R8A8(pal, &dst[0], bgraRowSize);

				dst += blockWidth * bgraPixelSize;
				++src;
			}
		}
	}
	else if(sourceFormat == eTF_DXT3) 
	{
		const BlockDXT3* src = (const BlockDXT3*) sourceData;

		for(int y=0; y<height; y+=blockHeight)
		{
			uint8* dst = ((uint8*)destinationData) + (y * bgraRowSize);

			for(int x=0; x<width; x+=blockWidth)
			{
				src[0].alpha.decode<bgraPixelSize>(&dst[3], bgraRowSize);
				src[0].color.evaluatePalette4KeepAlpha(pal);
				src[0].color.decodeB8G8R8<bgraPixelSize>(pal, &dst[0], bgraRowSize);

				dst += blockWidth * bgraPixelSize;
				++src;
			}
		}
	}
	else /* if(sourceFormat == eTF_DXT5) */
	{
		const BlockDXT5* src = (const BlockDXT5*) sourceData;

		for(int y=0; y<height; y+=blockHeight)
		{
			uint8* dst = ((uint8*)destinationData) + (y * bgraRowSize);

			for(int x=0; x<width; x+=blockWidth)
			{
				src[0].alpha.decode<bgraPixelSize>(&dst[3], bgraRowSize);
				src[0].color.evaluatePalette4KeepAlpha(pal);
				src[0].color.decodeB8G8R8<bgraPixelSize>(pal, &dst[0], bgraRowSize);

				dst += blockWidth * bgraPixelSize;
				++src;
			}
		}
	}

	return COMPRESSOR_ERROR_NONE;
}
//////////////////////////////////////////////////////////////////////////
CImage_DXTC::COMPRESSOR_ERROR CImage_DXTC::DecompressTextureATI2N(
	int width,
	int height,
	CImage_DXTC::COMPRESSED_FORMAT sourceFormat,
	CImage_DXTC::UNCOMPRESSED_FORMAT destinationFormat,
	const void* sourceData,
	void* destinationData,
	int destinationDataSize)
{
	bool swizzle;
	if(sourceFormat==FORMAT_COMP_ATI2N)
	{
		swizzle = false;
	}
	else if(sourceFormat==FORMAT_COMP_ATI2N_DXT5)
	{
		swizzle = true;
	}
	else
	{
		return COMPRESSOR_ERROR_UNSUPPORTED_SOURCE_FORMAT;
	}

	{
		const COMPRESSOR_ERROR result = CheckParameters(
			width,
			height,
			destinationFormat,
			sourceData,
			destinationData,
			destinationDataSize);

		if(result != COMPRESSOR_ERROR_NONE)
		{
			return result;
		}
	}

	const int blockWidth = 4;
	const int blockHeight = 4;

	const int bgraPixelSize = sizeof(ColorB8G8R8A8);
	const int bgraRowSize = bgraPixelSize * width;

	// Decode source data and write them to first and second channels of destination buffer. 
	{
		const AlphaBlockDXT5* src = (const AlphaBlockDXT5*) sourceData;

		for(int y=0; y<height; y+=blockHeight)
		{
			uint8* dst = ((uint8*)destinationData) + (y * bgraRowSize);

			for(int x=0; x<width; x+=blockWidth)
			{
				if(swizzle)
				{
					src[0].decode<bgraPixelSize>(&dst[1], bgraRowSize);
					src[1].decode<bgraPixelSize>(&dst[0], bgraRowSize);
				}
				else
				{
					src[0].decode<bgraPixelSize>(&dst[0], bgraRowSize);
					src[1].decode<bgraPixelSize>(&dst[1], bgraRowSize);
				}

				dst += blockWidth * bgraPixelSize;
				src += 2;		// note: each 3DC (ATI2N) block consists of two DXT5 alpha blocks.
			}
		}
	}

	// Reconstruct "usual" normalmap by using first two channels of the buffer:
	// first input channel goes to green, second input channel goes to red, 
	// blue is autocomputed, alpha is 255.
	{
		uint8* dst = (uint8*)destinationData;

		for(int pixelCount = width*height; pixelCount>0; --pixelCount)
		{
			const uint8 colorX = dst[0];
			const uint8 colorY = dst[1];
			uint8 colorZ;

			const float x = (colorX * (2/255.0f)) - 1.0f;
			const float y = (colorY * (2/255.0f)) - 1.0f;

			float lenXYSquared = x*x + y*y;

			if(lenXYSquared > 1)
			{
				//const float scale = 1/sqrt(lenXYSquared);
				//x *= scale;
				//y *= scale;
				colorZ = 0;
			}
			else
			{
				const float z = sqrt(1 - lenXYSquared);
				const int intZ = (int)(((z + 1) * 0.5f) * 255);
				colorZ = (intZ > 255) ? 255 : (uint8)intZ;
			}

			dst[0] = colorZ;    // blue
			dst[1] = colorX;    // green
			dst[2] = colorY;    // red
			dst[3] = 255;       // alpha

			dst += bgraPixelSize;
		}
	}

	return COMPRESSOR_ERROR_NONE;
}
//////////////////////////////////////////////////////////////////////////
