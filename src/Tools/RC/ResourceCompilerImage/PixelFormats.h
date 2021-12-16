#ifndef PIXELFORMATS_H
#define PIXELFORMATS_H

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>

#include "platform.h"							// uint32
#include <ddraw.h>								// D3DFORMAT
#include <ITexture.h>							// ETEX_Format

struct SPixelFormats;
typedef int TPixelFormatID;
struct ImageObject;


class CPixelFormats
{
public:

	static TPixelFormatID FindFinalTextureFormat( const TPixelFormatID iPixelFormat, const bool bAlphaChannelUsed );

	//! \return pixelformat no which has same properties but is uncompressed
	static TPixelFormatID GetPixelFormatUncompressed( const TPixelFormatID iniNo );

	//! not case sensitive
	//! /return -1 if the name was not recognized, otherwise no
	static TPixelFormatID GetNoFromName( const char *inszName );

	//! /return -1 if the format was not recognized, otherwise no
	static TPixelFormatID GetNoFromD3DFormat( D3DFORMAT inFormat );

	// Arguments:
	//   iniNo [0..GetPixelFormatCount()-1], -1 if not used
	static const char *GetPixelFormatName( const TPixelFormatID iniNo );

	// Return:
	//   -1 if not found
	static TPixelFormatID GetPixelFormatNoFromDestNo( const TPixelFormatID iniDestNo );

	// Return:
	//  ARGB8 if not found
	static TPixelFormatID GetFormatForMips( const TPixelFormatID iniDestNo );

	// Return:
	//   -1 if not found
	static TPixelFormatID GetDestNoFromPixelFormatNo( const TPixelFormatID iniPixelFmtNo );

	//!
	static char *GetPixelFormatDesc( const TPixelFormatID iniNo );

	//!
	static int GetPixelFormatCount();

	// Returns:
	//   0 is not valid
	static SPixelFormats *GetPixelFormatInfo( const TPixelFormatID iniNo );

	static void BuildSurfaceDesc( ImageObject *pImage, DDSURFACEDESC2 &surf );

	// returns texture format by pixel format
	static const ETEX_Format GetTextureFormat(const TPixelFormatID iPixelFormat);
};


struct SPixelFormats
{
	int iBitsPerPixel;
	char *szAlpha;
	D3DFORMAT DxtNo;
	char *szName;
	char *szUncompressedAltName;
	char *szDescription;
	bool bCompressed;
	bool bDestinationFormat;			// shows up in the list of usable destination pixel formats
};


#define D3DFMT_3DC ((D3DFORMAT)(MAKEFOURCC('A', 'T', 'I', '2')))				// two channel compressed normal maps 8bit -> 8 bits per pixel
#define D3DFMT_CTX1 ((D3DFORMAT)(MAKEFOURCC('C', 'T', 'X', '1')))			// two channel compressed normal maps 4bit -> 4 bits per pixel

static const char *g_szInvalidPixelFormatName = "*";				// string version of (TPixelFormatID)-1


extern SPixelFormats g_pixelformats[];

#endif // PIXELFORMATS_H 