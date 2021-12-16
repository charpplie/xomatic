////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   image_dxtc.h
//  Version:     v1.00
//  Created:     5/9/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __image_dxtc_h__
#define __image_dxtc_h__
#pragma once

class CImage_DXTC  
{
	// Typedefs
	public:
	protected:
		//////////////////////////////////////////////////////////////////////////
		// Extracted from Compressorlib.h on SDKs\CompressATI directory.
		// Added here because we are not really using the elements inside
		// this header file apart from those definitions as we are currently
		// loading the DLL CompressATI2.dll manually as recommended by the rendering 
		// team.
		typedef enum
		{
			FORMAT_COMP_ATI2N,
			FORMAT_COMP_ATI2N_DXT5,
			FORMAT_COMP_TOOBIG
		} COMPRESSED_FORMAT;

		typedef enum
		{
			FORMAT_ARGB_8888,
			FORMAT_ARGB_TOOBIG
		} UNCOMPRESSED_FORMAT;

		typedef enum
		{
			COMPRESSOR_ERROR_NONE,
			COMPRESSOR_ERROR_NO_INPUT_DATA,
			COMPRESSOR_ERROR_NO_OUTPUT_POINTER,
			COMPRESSOR_ERROR_UNSUPPORTED_SOURCE_FORMAT,
			COMPRESSOR_ERROR_UNSUPPORTED_DESTINATION_FORMAT,
			COMPRESSOR_ERROR_UNABLE_TO_INIT_CODEC,
			COMPRESSOR_ERROR_GENERIC
		} COMPRESSOR_ERROR;
		//////////////////////////////////////////////////////////////////////////

	// Methods
	public:
		CImage_DXTC();
		~CImage_DXTC();

		// Arguments:
		//   pQualityLoss - 0 if info is not needed, pointer to the result otherwise - not need to preinitialize
		bool Load( const char *filename,CImage &outImage, bool *pQualityLoss=0 );		// true if success

		static bool IsDXTCompressed(ETEX_Format eTF)
		{
			return (eTF == eTF_DXT1) || (eTF == eTF_DXT3) || (eTF == eTF_DXT5) || (eTF == eTF_3DC);
		}

		int TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF);
		int BitsPerPixel(ETEX_Format eTF);

	protected:
		inline const char* NameForTextureFormat(ETEX_Format ETF);

	private:
		static COMPRESSOR_ERROR CheckParameters(
			int width,
			int height,
			UNCOMPRESSED_FORMAT destinationFormat,
			const void* sourceData,
			void* destinationData,
			int destinationDataSize);

		static COMPRESSOR_ERROR DecompressTextureDXT(
			int width,
			int height,
			ETEX_Format sourceFormat,
			UNCOMPRESSED_FORMAT destinationFormat,
			const void* sourceData,
			void* destinationData,
			int destinationDataSize);

		static COMPRESSOR_ERROR DecompressTextureATI2N(
			int width,
			int height,
			COMPRESSED_FORMAT sourceFormat,
			UNCOMPRESSED_FORMAT destinationFormat,
			const void* sourceData,
			void* destinationData,
			int destinationDataSize);
};

#endif // __image_dxtc_h__
