/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: CryCompressorRC.h,v 1.0 2008/02/14 15:18:23 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  source file that defines the entry point for the compressor DLL application
							purposes: compression of any kind of data for loading and streaming purposes
-------------------------------------------------------------------------
History:
- 14:2:2008 15:18 : Created by Anton Kaplanyan
*************************************************************************/

#include "stdafx.h"
#include "Endian.h"
#include "CryCompressorRC.h"
#include "ResourceCompiler.h"
#include "ICfgFile.h"
#include <ImageExtensionHelper.h>
#include "ijl.h"
#include <d3d9.h>
#include <d3dx9.h>

#define PTC_RATIO_THRESHOLD (60.f)
#define MCT_RATIO_THRESHOLD (90.f)
#define JPEG_RATIO_THRESHOLD (80.f)

namespace
{
	static const D3DFORMAT GetXBoxFormat(const ETEX_Format fmt)
	{
		switch (fmt)
		{
		case eTF_A8R8G8B8:
			return D3DFMT_LIN_A8R8G8B8;
		//case eTF_A8B8G8R8:
			//return D3DFMT_LIN_A8B8G8R8;
		case eTF_X8R8G8B8:
			return D3DFMT_LIN_X8R8G8B8;
		case eTF_G16R16F:
			return D3DFMT_LIN_G16R16F;
		case eTF_R32F:
			return D3DFMT_LIN_R32F;
		case eTF_A4R4G4B4:
			return D3DFMT_LIN_A4R4G4B4;
		case eTF_DXT3:
			return D3DFMT_LIN_DXT3;
		case eTF_DXT5:
			return D3DFMT_LIN_DXT5;
		case eTF_3DC:
			return D3DFMT_LIN_DXN;
		case eTF_CTX1:
			return D3DFMT_LIN_CTX1;
		case eTF_DXT1:
			return D3DFMT_LIN_DXT1;
		case eTF_A8:
			return D3DFMT_LIN_A8;
		case eTF_L8:
			return D3DFMT_LIN_L8;
		default:
			assert(0);
			return D3DFMT_UNKNOWN;
		}
	}
	static const D3DFORMAT GetXBoxTiledFormat(const ETEX_Format fmt)
	{
		switch (fmt)
		{
		case eTF_A8R8G8B8:
			return D3DFMT_A8R8G8B8;
			//case eTF_A8B8G8R8:
			//return D3DFMT_A8B8G8R8;
		case eTF_X8R8G8B8:
			return D3DFMT_X8R8G8B8;
		case eTF_G16R16F:
			return D3DFMT_G16R16F;
		case eTF_R32F:
			return D3DFMT_R32F;
		case eTF_A4R4G4B4:
			return D3DFMT_A4R4G4B4;
		case eTF_DXT3:
			return D3DFMT_DXT3;
		case eTF_DXT5:
			return D3DFMT_DXT5;
		case eTF_3DC:
			return D3DFMT_DXN;
		case eTF_CTX1:
			return D3DFMT_CTX1;
		case eTF_DXT1:
			return D3DFMT_DXT1;
		case eTF_A8:
			return D3DFMT_A8;
		case eTF_L8:
			return D3DFMT_L8;
		default:
			assert(0);
			return D3DFMT_UNKNOWN;
		}
	}
	static ETEX_Format GetDXTFormat(const CImageExtensionHelper::DDS_HEADER* const pHeader)
	{
		if(pHeader->ddspf.dwFourCC)
		{
			if(pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_DXT1.dwFourCC) 			return eTF_DXT1;
			if(pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_DXT3.dwFourCC) 			return eTF_DXT3;
			if(pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_DXT5.dwFourCC) 			return eTF_DXT5;
			if (pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_3DC.dwFourCC)			return eTF_3DC;
			if (pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_CTX1.dwFourCC)			return eTF_CTX1;
			if( pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_R32F.dwFourCC)			return eTF_R32F;
			if( pHeader->ddspf.dwFourCC == DDSFormats::DDSPF_G16R16F.dwFourCC)	return eTF_G16R16F;
		}
		else
		{
			if (pHeader->ddspf.dwFlags == DDS_RGBA && pHeader->ddspf.dwRGBBitCount == 32 && pHeader->ddspf.dwABitMask == 0xff000000)
				return eTF_A8R8G8B8;
			else if (pHeader->ddspf.dwFlags == DDS_RGBA && pHeader->ddspf.dwRGBBitCount == 16)
				return eTF_A4R4G4B4;
			//else if (pHeader->ddspf.dwFlags == DDS_RGB  && pHeader->ddspf.dwRGBBitCount == 24)
			//	return eTF_R8G8B8;
			else if (pHeader->ddspf.dwFlags == DDS_RGB  && pHeader->ddspf.dwRGBBitCount == 32)
				return eTF_X8R8G8B8;
			else if (pHeader->ddspf.dwFlags == DDS_LUMINANCE  && pHeader->ddspf.dwRGBBitCount == 8)
				return eTF_L8;
			else if ((pHeader->ddspf.dwFlags == DDS_A || pHeader->ddspf.dwFlags == DDS_A_ONLY) && pHeader->ddspf.dwRGBBitCount == 8)
				return eTF_A8;
		}

		//assert(0);
		return eTF_Unknown;
	}

	static bool IsDXTCompressed(const ETEX_Format eTF)
	{
		if (eTF == eTF_DXT1 || eTF == eTF_DXT3 || eTF == eTF_DXT5 || eTF == eTF_3DC || eTF == eTF_CTX1)
			return true;
		return false;
	}

	static uint32 GetBytesPerBlock(const ETEX_Format format)
	{
		switch (format)
		{
		case eTF_A8R8G8B8:
		//case eTF_A8B8G8R8:
		case eTF_X8R8G8B8:
		case eTF_G16R16F:
		case eTF_R32F:
			return 4;
		case eTF_R8G8B8:
			return 3;
		case eTF_A4R4G4B4:
			return 2;
		case eTF_DXT3:
		case eTF_DXT5:
		case eTF_3DC:
			return 16;
		case eTF_DXT1:
		case eTF_CTX1:
			return 8;
		case eTF_A8:
		case eTF_L8:
			return 1;
		default:
			assert(0);
		}
		return 0;
	}
	// desc of PTC-compressed chunk of data
	struct PTCDesc
	{
		enum EPTCDescVersion
		{
			eptcdvVersion2		= MAKEFOURCC('P','T','C','3'),
			eptcdvDefault			= eptcdvVersion2,
		};
		enum EPTCDescCompressioParameters
		{
			numPTCMIPs				= 5,	
		};

		uint32			version;
		uint32			format;
		uint32			compressedSize;
		uint32			decompressedSize;
		uint32			numMips;
	};
	// desc of MCT-compressed chunk of data
	struct MCTDesc
	{
		enum EMCTDescVersion
		{
			emctdvVersion1		= MAKEFOURCC('M','C','T','2'),
			emctdvDefault			= emctdvVersion1,
		};
		uint32			version;
		uint32			format;
		uint32			numMips;
		uint32			compressedSize;
		uint32			decompressedSize;
	};
	// desc of JPEG-compressed chunk of data
	struct JPEGDesc
	{
		enum EJPEGDescVersion
		{
			ejpegdvVersion0		= MAKEFOURCC('J','P','G','0'),
			ejpegdvDefault		= ejpegdvVersion0,
		};
		uint32			version;
		uint32			srcFormat;
		uint32			dataFormat;
		uint32			width;
		uint32			height;
		uint32			compressedSize;
		uint32			decompressedSize;
	};
}

CTextureCompressor::CTextureCompressor(IResourceCompiler *pRC) : m_pRC(pRC), m_platform(ePlatform_PC), m_eEndian(eLittleEndian)
{
}

CTextureCompressor::~CTextureCompressor()
{
}

ICryCompressorRC* CTextureCompressor::Create( IResourceCompiler *pRC )
{
	return new CTextureCompressor(pRC);
}

bool CTextureCompressor::CompressMipsWithMCT( const CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, 
																									const uint32 nNumMips, const uint8* pBuffer, const uint32 nBufferSize,
																									std::vector<uint8>& output )
{
	//if(m_platform != ePlatform_X360)
		return false;

	const ETEX_Format fmt = GetDXTFormat(pHeader);

	if(fmt == eTF_Unknown)
		return false;

	if(pHeader->dwDepth > 1)
		return false;

	if(!IsDXTCompressed(fmt))
		return false;

	assert(output.size() >= nBufferSize + sizeof(MCTDesc));

	XenonTexture* pTexData = LoadDataIntoTexture(pHeader->dwWidth, pHeader->dwHeight, nStartMip, nNumMips, pBuffer, nBufferSize, fmt );

	// Compress the surface.
	UINT  nDstSize = nBufferSize;
	UINT  nMipDstSize = 0;

	XGMCT_COMPRESSION_PARAMS params;
	params.Level = 5;
	params.TrainingSampleSpread.x = 1;
	params.TrainingSampleSpread.y = 1;
	params.TrainingSampleSpread.z = 1;
	params.LinearThreshold = 64;//max(64, min(pHeader->dwWidth, pHeader->dwHeight) >> nStartMip);

	// compression: lossy: 1..99, (lossless - 100)
	params.Quality = 100.f;

	// it's only lossless compression for NMs
	if(fmt == eTF_3DC || fmt == eTF_CTX1)
		params.Quality = 100.f;

	float ratio = 100.f;

	std::vector<byte> vecMips(nBufferSize);

	// Compress the texture.
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = XGMCTCompressTexture( NULL, &output[sizeof(MCTDesc)], &nDstSize, &vecMips[0], &nMipDstSize, D3DFMT_UNKNOWN, 
																					pTexData->pTexture, NULL, XGCOMPRESS_MCT_CONTIGUOUS_MIP_LEVELS, &params )))
	{
		assert(vecMips.size() >= nMipDstSize);
		vecMips.resize(nMipDstSize);
		if(nDstSize + nMipDstSize + sizeof(MCTDesc) < nBufferSize)
		{
			MCTDesc* pDesc = (MCTDesc*)&output[0];
			pDesc->version = MCTDesc::emctdvDefault;
			SwapEndian(pDesc->version, m_eEndian);
			pDesc->format = fmt;
			SwapEndian(pDesc->format, m_eEndian);
			pDesc->numMips = nNumMips;
			SwapEndian(pDesc->numMips, m_eEndian);
			pDesc->compressedSize = nDstSize + nMipDstSize;
			SwapEndian(pDesc->compressedSize, m_eEndian);
			pDesc->decompressedSize = nBufferSize;
			SwapEndian(pDesc->decompressedSize, m_eEndian);
			output.resize(nDstSize + nMipDstSize + sizeof(MCTDesc));
			memcpy(&output[nDstSize + sizeof(MCTDesc)], &vecMips[0], vecMips.size());
			ratio = (float)output.size() / nBufferSize * 100.f;
			if(ratio < MCT_RATIO_THRESHOLD)
			{
				if(nNumMips > 1)
					printf("Compressed MIPs %d-%d using MCT, ratio: %.1f%%\n", nStartMip, nStartMip + nNumMips - 1, ratio);
				else
					printf("Compressed MIP %d using MCT, ratio: %.1f%%\n", nStartMip, ratio);
				// collect statistics
				m_statistics.AddRatio<ectMCT>(ratio);
			}
		}
		else
			hr = E_FAIL;
	}
	else
		assert(hr == D3DERR_MOREDATA);

	delete pTexData;

	return SUCCEEDED(hr) && ratio < MCT_RATIO_THRESHOLD;
}

bool CTextureCompressor::CompressMipsWithPTC( const CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, 
																									const uint32 nNumMips, const uint8* pBuffer, const uint32 nBufferSize, 
																									std::vector<uint8>& output )
{
	//if(m_platform != ePlatform_X360)
		return false;

	ETEX_Format srcFormat = GetDXTFormat(pHeader);

	if(srcFormat == eTF_Unknown)
		return false;

	if(pHeader->dwDepth > 1)
		return false;

	// PTC has a bug with L8 decompression
	if(srcFormat == eTF_L8 || srcFormat == eTF_A8 || srcFormat == eTF_G16R16F)
		return false;

	assert(output.size() >= nBufferSize);

	const uint8* pBufferPointer = pBuffer;

	XenonTexture* pTexData = LoadDataIntoTexture(pHeader->dwWidth, pHeader->dwHeight, nStartMip, nNumMips, pBuffer, nBufferSize, srcFormat );

	HRESULT hr = E_FAIL;
	UINT nCompressedSize;
	uint32 nTotalCompressedSize = 0;

	D3DLOCKED_RECT rect;
	D3DSURFACE_DESC desc;

	float ratio = 100.f;

	XGPTC_COMPRESSION_PARAMS params;

	params.Qs = 100;            			// Quantization step, 4..10000000 [default = 100]
																	// Compression ratio increases for larger Qs
																	// Compression is lossless for Qs = 0
	params.Qa = 100;            			// Quantization step for alpha, same as Qs
	params.ColorSpace = 2;    			// Color space of input pixels; options are:
																	// 0 - undefined; channels coded independently
																	// 1 - RGB     2 - CMY     3 - CMYK
	params.OneBitAlpha = FALSE;   	// Treat alpha channel as a one-bit alpha channel
	params.Ec = 0;            			// Entropy coding mode [default = 1]
																	// 0 = bit-plane coder, scalable
																	// 1 = adaptive run-length/Rice, non scalable, faster
	params.Eca = 0;           			// Entropy coding mode for the alpha channel
	params.ChunkWidth = 256; 				// Encoding chunk width, in # of pixels, 32..5184 [default = 256]
	params.Bd = 8;            			// # of bits to preserve for lossless compression
																	// when the intput is 16-bits, from 8 to 16
	params.Npp = FALSE;           	// Use non-overlapping transforms [not recommended]; improves
																	// compression and speed in lossless mode, but precludes mipmap
																	// decoding.
	params.pMeta = NULL;	        	// Inserts specified string into global metadata area
																	// of the PTC image (may be NULL)
	params.pFrameMeta = NULL;				// Inserts specified string into frame metadata area
																	// of the PTC image (may be NULL)

	// it's only lossless compression for NMs
	if(srcFormat == eTF_3DC || srcFormat == eTF_CTX1)
	{
		params.Qs = 0;
		params.Qa = 0;
		params.ColorSpace = 0;
		params.Ec = 1;
		params.Eca = 1;
		params.ChunkWidth = 256;
		params.Npp = TRUE;
		return false;
	}

	std::vector<std::pair<int, int> > vecMips;
	int lastMip = (int)nNumMips;
	do 
	{
		int curMip = max((int)0, lastMip - PTCDesc::numPTCMIPs);
		assert(lastMip > curMip);
		vecMips.push_back(std::make_pair(curMip, lastMip - curMip));
		lastMip -= PTCDesc::numPTCMIPs;
	} while (lastMip > 0);

	uint32 numCompressedMips = 0;

	for(std::vector<std::pair<int, int> >::reverse_iterator it = vecMips.rbegin();it != vecMips.rend();++it)
	{
		const int iMip = it->first;
		const int numMips = it->second;
		pTexData->pTexture->GetLevelDesc(iMip, &desc);

		uint32 nBlockWidth = desc.Width;
		uint32 nBlockHeight = desc.Height;

		if(IsDXTCompressed(srcFormat))
		{
			nBlockWidth  = (nBlockWidth  + 3) / 4;
			nBlockHeight = (nBlockHeight + 3) / 4;
		}

		//params.ChunkWidth = max(32, min(2048, desc.Width * desc.Height));

		const uint32 nBlockRowPitch = nBlockWidth * GetBytesPerBlock(srcFormat);

		// avoid last mip
		if(desc.Height == 1 && desc.Width == 1)
		{
			memcpy(&output[nTotalCompressedSize], pBufferPointer, nBlockRowPitch * nBlockHeight);
			nTotalCompressedSize += nBlockRowPitch * nBlockHeight;
			pBufferPointer += nBlockRowPitch * nBlockHeight;
			assert(pBufferPointer == pBuffer + nBufferSize);
			break;
		}

		// calculate size of chunk of mips
		uint32 nMipsChunkSize = 0;
		for(uint32 iCurMip = 0;iCurMip<numMips;++iCurMip)
		{
			uint32 nBlockWidth = max(1u, desc.Width>>iCurMip);
			uint32 nBlockHeight = max(1u, desc.Height>>iCurMip);

			if(IsDXTCompressed(srcFormat))
			{
				nBlockWidth  = (nBlockWidth  + 3) / 4;
				nBlockHeight = (nBlockHeight + 3) / 4;
			}

			const uint32 nBlockRowPitch = nBlockWidth * GetBytesPerBlock(srcFormat);

			nMipsChunkSize += nBlockRowPitch * nBlockHeight;
		}

		pTexData->pTexture->LockRect(iMip, &rect, NULL, D3DLOCK_READONLY);
		void* pCompressedBuffer = NULL;
		hr = XGPTCCompressSurfaceEx(&pCompressedBuffer, &nCompressedSize,
																rect.pBits, rect.Pitch, desc.Width,
																desc.Height, desc.Format, NULL, &params);
		pTexData->pTexture->UnlockRect(iMip);

		if(SUCCEEDED(hr))
		{
			if(nCompressedSize && nTotalCompressedSize + nCompressedSize + sizeof(PTCDesc) <= nBufferSize)
			{
				output.resize(nTotalCompressedSize + sizeof(PTCDesc) + nCompressedSize);
				PTCDesc* desk = (PTCDesc*)&output[nTotalCompressedSize];
				desk->version = PTCDesc::eptcdvDefault;
				SwapEndian(desk->version, m_eEndian);
				desk->format = srcFormat;
				SwapEndian((uint32&)desk->format, m_eEndian);
				desk->compressedSize = nCompressedSize;
				SwapEndian(desk->compressedSize, m_eEndian);
				desk->decompressedSize = nMipsChunkSize;
				SwapEndian(desk->decompressedSize, m_eEndian);
				desk->numMips = numMips;
				assert(desk->numMips > 0);
				numCompressedMips += desk->numMips;
				SwapEndian(desk->numMips, m_eEndian);
				memcpy(&output[nTotalCompressedSize + sizeof(PTCDesc)], pCompressedBuffer, nCompressedSize);
				nTotalCompressedSize += nCompressedSize + sizeof(PTCDesc);
				pBufferPointer += nMipsChunkSize;
			}
			else
				hr = E_FAIL;
			XGPTCFreeMemory(pCompressedBuffer);
		}
		else
			assert(hr == D3DERR_MOREDATA);

		if(FAILED(hr))
			break;
	}

	delete pTexData;

	if(SUCCEEDED(hr))
	{
		assert(numCompressedMips == nNumMips);
		ratio = (float)output.size() / nBufferSize * 100.f;
		if(ratio < PTC_RATIO_THRESHOLD)
		{
			if(nNumMips > 1)
				printf("Compressed MIPs %d-%d using PTC, ratio: %.1f%%\n", nStartMip, nStartMip + nNumMips - 1, ratio);
			else
				printf("Compressed MIP %d using PTC, ratio: %.1f%%\n", nStartMip, ratio);
			// collect statistics
			m_statistics.AddRatio<ectPTC>(ratio);
		}
	}

	return SUCCEEDED(hr) && ratio < PTC_RATIO_THRESHOLD;
}

bool CTextureCompressor::CompressMipsWithJPEG( const CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, 
																						 const uint32 nNumMips, const uint8* pBuffer, const uint32 nBufferSize,
																						 std::vector<uint8>& output )
{
	if(m_platform == ePlatform_X360)	// JPEG dec is unsupported on XBox now(not needed as we have MCT and PTC)
		return false;

	//if(m_platform == ePlatform_PC)	// JPEG dec is temporary disabled for PC due to it's uselessness
		return false;

	if(pHeader->dwDepth > 1)	// no volume textures
		return false;

	ETEX_Format srcFormat = GetDXTFormat(pHeader);
	ETEX_Format destFormat = srcFormat;

	if(srcFormat == eTF_Unknown)
		return false;

	if(srcFormat == eTF_CTX1 || srcFormat == eTF_3DC ||
		srcFormat == eTF_G16R16F || srcFormat == eTF_R32F	||
		srcFormat == eTF_A4R4G4B4 || srcFormat == eTF_DXT3)	// don't support normal map JPEG compression yet
		return false;

	JPEG_CORE_PROPERTIES image;
	ZeroMemory( &image, sizeof( JPEG_CORE_PROPERTIES ) );

	if( ijlInit( &image ) != IJL_OK )	// init codec
		return false;

	// decompression back to raw RGBA8 format
	std::vector<byte> convertedBuffer;
	if(srcFormat == eTF_DXT1 || srcFormat == eTF_DXT5/* || srcFormat == eTF_DXT3 || srcFormat == eTF_A4R4G4B4*/) 
	{
		XenonTexture* pTexData = LoadDataIntoTexture(pHeader->dwWidth, pHeader->dwHeight, 
																								nStartMip, nNumMips, pBuffer, nBufferSize, srcFormat);	// load compressed texture
		if(!ConvertToARGB8(pTexData, convertedBuffer))	// decompress by means of standard DX routines
		{
			assert(0);
			ijlFree( &image );
			return false;
		}
		(uint8*&)pBuffer = &convertedBuffer[0];
		(uint32&)nBufferSize = (int)convertedBuffer.size();
		if(srcFormat == eTF_DXT1)
			destFormat = eTF_X8R8G8B8;
		else if(srcFormat == eTF_DXT5)
			destFormat = eTF_A8R8G8B8;
		else
			assert(0);
	}

	assert(!IsDXTCompressed(destFormat));	// raw data only!

	// setup quality
	image.jquality = 100;

	// it's only lossless compression for NMs(in the future?)
	if(srcFormat == eTF_3DC || srcFormat == eTF_CTX1)
		image.jquality = 100;

	// it's only lossless compression for NMs(in the future?)
	if(IsDXTCompressed(srcFormat))
		image.jquality = 33;

	float ratio = 100.f;

	output.resize(0);	// shrink array

	size_t nMipOffset = 0;

	bool hasAlpha = false;
	bool removeAlpha = false;

	// compress each mip separately
	for(uint32 iMip = nStartMip;iMip<nStartMip + nNumMips;++iMip)
	{
		uint32 nBlockWidth = max(1ul, pHeader->dwWidth>>iMip);
		uint32 nBlockHeight = max(1ul, pHeader->dwHeight>>iMip);
		const size_t nMipSize = nBlockHeight * nBlockWidth * GetBytesPerBlock(destFormat);

		// set up JPEG compressor parameters
		switch(destFormat)
		{
		case eTF_A8R8G8B8:
			hasAlpha = true;
			image.DIBChannels = 3;
			image.DIBColor = IJL_RGB;
			image.JPGChannels = 3;
			image.JPGColor = IJL_YCBCR;
			destFormat = eTF_R8G8B8;	// change dest format
			image.JPGSubsampling = IJL_411;
			break;
		case eTF_X8R8G8B8:
			removeAlpha = true;
			image.DIBChannels = 3;
			image.DIBColor = IJL_RGB;
			image.JPGChannels = 3;
			image.JPGColor = IJL_YCBCR;
			image.JPGSubsampling = IJL_411;
			destFormat = eTF_R8G8B8;	// change dest format
			break;
		case eTF_R8G8B8:
			image.DIBChannels = 3;
			image.DIBColor = IJL_RGB;
			image.JPGChannels = 3;
			image.JPGColor = IJL_YCBCR;
			image.JPGSubsampling = IJL_411;
			break;
		case eTF_A8:
			image.DIBChannels = 1;
			image.DIBColor = IJL_G;
			image.JPGChannels = 1;
			image.JPGColor = IJL_G;
			image.JPGSubsampling = IJL_NONE;
			break;
		case eTF_L8:
			image.DIBChannels = 1;
			image.DIBColor = IJL_G;
			image.JPGChannels = 1;
			image.JPGColor = IJL_G;
			image.JPGSubsampling = IJL_NONE;
			break;
		default:
			assert(0);
			return false;
		}

		size_t nColorsSize = nMipSize;

		std::vector<byte> compressedData(nMipSize);

		image.JPGBytes = &compressedData[0];
		image.JPGSizeBytes = (int)nMipSize;

		// set up header data
		image.JPGWidth  = nBlockWidth;
		image.JPGHeight = nBlockHeight;
		image.DIBWidth  = nBlockWidth;
		image.DIBHeight = nBlockHeight;

		size_t oldOffset = output.size();

		std::vector<byte> colorsData;	// for separated alpha and colors
		std::vector<byte> alphaData;
		if(!hasAlpha && !removeAlpha)
			image.DIBBytes = (unsigned char*)pBuffer + nMipOffset;
		else
		{
			const uint8* pMip = pBuffer + nMipOffset;

			colorsData.reserve(nBlockHeight * nBlockWidth * 3);
			alphaData.reserve(nBlockHeight * nBlockWidth);

			// separate colors and alpha data
			for(uint32 iTexel = 0;iTexel < nBlockHeight * nBlockWidth;++iTexel)
			{
				colorsData.push_back(pMip[iTexel*4+2]);
				colorsData.push_back(pMip[iTexel*4+1]);
				colorsData.push_back(pMip[iTexel*4+0]);
				alphaData.push_back (pMip[iTexel*4+3]);
			}
			nColorsSize = nBlockHeight * nBlockWidth * 3;
			image.DIBBytes = &colorsData[0];
			image.JPGSizeBytes = (int)nColorsSize;
		}

		// Compress the texture.
		IJLERR err = ijlWrite( &image, IJL_JBUFF_WRITEWHOLEIMAGE );

//////
		//image.JPGFile = "Test.jpg";
		//err = ijlWrite( &image, IJL_JFILE_WRITEWHOLEIMAGE );
//////

		if( err == IJL_OK && oldOffset + image.JPGSizeBytes + sizeof(JPEGDesc) < nBufferSize )
		{
			output.resize(oldOffset + image.JPGSizeBytes + sizeof(JPEGDesc));
			JPEGDesc* pDesc = (JPEGDesc*)&output[oldOffset];
			pDesc->version = JPEGDesc::ejpegdvDefault;
			SwapEndian(pDesc->version, m_eEndian);
			pDesc->srcFormat = srcFormat;
			SwapEndian(pDesc->srcFormat, m_eEndian);
			pDesc->dataFormat = destFormat;
			SwapEndian(pDesc->dataFormat, m_eEndian);
			pDesc->width = nBlockWidth;
			SwapEndian(pDesc->width, m_eEndian);
			pDesc->height = nBlockHeight;
			SwapEndian(pDesc->height, m_eEndian);
			pDesc->compressedSize = image.JPGSizeBytes;
			SwapEndian(pDesc->compressedSize, m_eEndian);
			pDesc->decompressedSize = (uint32)nColorsSize;
			SwapEndian(pDesc->decompressedSize, m_eEndian);
			memcpy(&output[oldOffset + sizeof(JPEGDesc)], &compressedData[0], image.JPGSizeBytes);

			if(hasAlpha)	// add alpha chunk
			{
				assert(destFormat == eTF_R8G8B8);
				assert(!alphaData.empty());

				const size_t nAlphaSize = nBlockHeight * nBlockWidth;

				// set up new codec parameters
				image.DIBChannels = 1;
				image.DIBColor = IJL_G;
				image.JPGChannels = 1;
				image.JPGColor = IJL_G;
				image.JPGSubsampling = IJL_NONE;
				image.DIBBytes = &alphaData[0];
				image.JPGSizeBytes = (int)nAlphaSize;

				oldOffset = output.size();

				err = ijlWrite( &image, IJL_JBUFF_WRITEWHOLEIMAGE );

//////
				//image.JPGFile = "TestAlpha.jpg";
				//err = ijlWrite( &image, IJL_JFILE_WRITEWHOLEIMAGE );
//////

				if( err == IJL_OK && oldOffset + image.JPGSizeBytes + sizeof(JPEGDesc) < nBufferSize )
				{
					output.resize(oldOffset + image.JPGSizeBytes + sizeof(JPEGDesc));
					JPEGDesc* pDesc = (JPEGDesc*)&output[oldOffset];
					pDesc->version = JPEGDesc::ejpegdvDefault;
					SwapEndian(pDesc->version, m_eEndian);
					pDesc->srcFormat = srcFormat;
					SwapEndian(pDesc->srcFormat, m_eEndian);
					pDesc->dataFormat = eTF_A8;					// special case for synthetic attached alpha channel in JPEG compression
					SwapEndian(pDesc->dataFormat, m_eEndian);
					pDesc->width = nBlockWidth;
					SwapEndian(pDesc->width, m_eEndian);
					pDesc->height = nBlockHeight;
					SwapEndian(pDesc->height, m_eEndian);
					pDesc->compressedSize = image.JPGSizeBytes;
					SwapEndian(pDesc->compressedSize, m_eEndian);
					pDesc->decompressedSize = (uint32)nAlphaSize;
					SwapEndian(pDesc->decompressedSize, m_eEndian);
					memcpy(&output[oldOffset + sizeof(JPEGDesc)], &compressedData[0], image.JPGSizeBytes);
				}
				else
				{
					assert(err == IJL_OK || err == IJL_BUFFER_TOO_SMALL);
					ijlFree( &image );
					return false;
				}
			}

			nMipOffset += nMipSize;
			assert(nMipOffset <= nBufferSize);
		}
		else
		{
			assert(err == IJL_OK || err == IJL_BUFFER_TOO_SMALL);
			ijlFree( &image );
			return false;
		}
	}

	assert(nMipOffset == nBufferSize);

	if( ijlFree( &image ) != IJL_OK )
	{
		assert(0);
		return false;
	}

	ratio = (float)output.size() / nBufferSize * 100.f;
	if(ratio < JPEG_RATIO_THRESHOLD)
	{
		if(nNumMips > 1)
			printf("Compressed MIPs %d-%d using JPEG, ratio: %.1f%%\n", nStartMip, nStartMip + nNumMips - 1, ratio);
		else
			printf("Compressed MIP %d using JPEG, ratio: %.1f%%\n", nStartMip, ratio);
		// collect statistics
		m_statistics.AddRatio<ectJPEG>(ratio);
	}

	return ratio < JPEG_RATIO_THRESHOLD;
}

bool CTextureCompressor::QuantizeWithMCT( const uint32 nWidth, const uint32 nHeight, const ETEX_Format format, uint8* pBuffer, const uint32 nBufferSize, const float fQuality )
{
	if(format == eTF_Unknown)
		return false;

	if(!IsDXTCompressed(format))
		return false;

	XenonTexture* pSrcData = LoadDataIntoTexture(nWidth, nHeight, 0, 1, pBuffer, nBufferSize, format );

	XenonTexture* pDestData = CreateTexture(nWidth, nHeight, format, 0, 1);

	// Compress the surface.
	UINT  nDstSize = nBufferSize;

	XGMCT_COMPRESSION_PARAMS params;
	params.Level = 5;
	params.TrainingSampleSpread.x = 1;
	params.TrainingSampleSpread.y = 1;
	params.TrainingSampleSpread.z = 1;
	params.LinearThreshold = 64;//max(64, min(pHeader->dwWidth, pHeader->dwHeight) >> nStartMip);

	// compression: lossy: 1..99, (lossless - 100)
	assert(fQuality >= 1.f && fQuality <= 100.f);
	params.Quality = fQuality;

	// it's only lossless compression for NMs
	if(format == eTF_3DC || format == eTF_CTX1)
		return false;//params.Quality = 100.f;

	float ratio = 100.f;

	HRESULT hr = XGMCTQuantizeTexture( NULL, pDestData->pTexture, NULL, pSrcData->pTexture, NULL, XGCOMPRESS_MCT_CONTIGUOUS_MIP_LEVELS, &params );

	delete pSrcData;

	LoadDataFromTexture(pDestData, pBuffer, nBufferSize, format);

	delete pDestData;

	return SUCCEEDED(hr);
}

CTextureCompressor::XenonTexture* CTextureCompressor::LoadDataIntoTexture( const uint32 nWidth, const uint32 nHeight, 
																																					const uint32 nStartMip, const uint32 nNumMips, const uint8* pBuffer, 
																																					const uint32 nBufferSize, ETEX_Format destFormat )
{
	assert(destFormat != eTF_Unknown);

	XenonTexture* pTexDesc = CreateTexture(nWidth, nHeight, destFormat, nStartMip, nNumMips);

	const uint8* pData = pBuffer;
	for(uint32 iMip=0;iMip<nNumMips;++iMip)
	{
		uint32 nBlockWidth = max(1ul, nWidth>>(nStartMip+iMip));
		uint32 nBlockHeight = max(1ul, nHeight>>(nStartMip+iMip));

		if(IsDXTCompressed(destFormat))
		{
			nBlockWidth  = (nBlockWidth  + 3) / 4;
			nBlockHeight = (nBlockHeight + 3) / 4;
		}

		const uint32 nBlockRowPitch = nBlockWidth * GetBytesPerBlock(destFormat);

		D3DLOCKED_RECT rect;
		pTexDesc->pTexture->LockRect(iMip, &rect, NULL, 0);
		uint8* pRow = (uint8*)rect.pBits;
		for(uint32 i = 0;i < nBlockHeight;++i)
		{
			memcpy(pRow, pData, nBlockRowPitch);
			pData += nBlockRowPitch;
			pRow += rect.Pitch;
			assert(pData <= pBuffer + nBufferSize);
		}
		pTexDesc->pTexture->UnlockRect(iMip);
		if(iMip == nNumMips-1)
			assert(pData == pBuffer + nBufferSize);
	}

	return pTexDesc;
}

void SaveDDS(const char* name, const byte* pData, const ETEX_Format format, const uint32 width, const uint32 height)
{
	size_t nMipPitch = width * GetBytesPerBlock(format);
	size_t nMipSize = height * nMipPitch;

	if(IsDXTCompressed(format))
	{
		nMipPitch = (width+3)/4 * GetBytesPerBlock(format);
		nMipSize = (height+3)/4 * nMipPitch;
	}

	FILE* file = fopen(name, "wb");
	std::vector<byte> dds(nMipSize+sizeof(CImageExtensionHelper::DDS_FILE_DESC));
	CImageExtensionHelper::DDS_FILE_DESC& ddsdesc = (CImageExtensionHelper::DDS_FILE_DESC&)dds[0];
	ddsdesc.dwMagic = MAKEFOURCC('D','D','S',' ');
	ddsdesc.header.dwSize = sizeof(ddsdesc.header);
	ddsdesc.header.dwWidth = width;
	ddsdesc.header.dwHeight = height;
	ddsdesc.header.dwMipMapCount = 1;
	ddsdesc.header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE;
	ddsdesc.header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	ddsdesc.header.dwTextureStage = 'CRYF';
	ddsdesc.header.dwReserved1[0] = 0;
	ddsdesc.header.ddspf = DDSFormats::GetDescByFormat(format);
	ddsdesc.header.dwPitchOrLinearSize = (DWORD)nMipPitch;
	memcpy(&dds[sizeof(CImageExtensionHelper::DDS_FILE_DESC)], pData, nMipSize);
	fwrite(&dds[0], dds.size(), 1, file);
	fclose(file);
}

bool CTextureCompressor::ConvertToARGB8( XenonTexture* pInData, std::vector<byte>& outData )
{
	HRESULT hr;
	D3DSURFACE_DESC desc;
	assert(pInData && pInData->pTexture && pInData->pAllocatedData);
	assert(pInData->pTexture->GetLevelCount());

	outData.clear();

	for(uint32 iLevel = 0;iLevel < pInData->pTexture->GetLevelCount();++iLevel)
	{
		const size_t oldSize = outData.size();
		pInData->pTexture->GetLevelDesc(iLevel, &desc);
		IDirect3DSurface9* pSrcSurface = NULL;
		pInData->pTexture->GetSurfaceLevel(iLevel, &pSrcSurface);

		IDirect3DTexture9* pDestTexture = new IDirect3DTexture9;

		UINT dwTextureSize;
		XGSetTextureHeaderEx( desc.Width, desc.Height, 1,
													/*D3DUSAGE_CPU_CACHED_MEMORY*/0, D3DFMT_LIN_A8R8G8B8, 0, XGHEADEREX_NONPACKED, 
													0, 0, 0, pDestTexture, &dwTextureSize, NULL );
		byte* pAllocatedData = new uint8[4097 + dwTextureSize];
		memset(pAllocatedData, 0, 4097 + dwTextureSize);
		uint8* pAlignedBaseBuffer = (uint8*)((uint64)((uint8*)pAllocatedData + 4096) & (~uint64(0xFFF)));

		XGOffsetResourceAddress( pDestTexture, pAlignedBaseBuffer ); 

		IDirect3DSurface9* pDestSurface = NULL;
		pDestTexture->GetSurfaceLevel(0, &pDestSurface);

		hr = D3DXLoadSurfaceFromSurface(pDestSurface, NULL, NULL, pSrcSurface, NULL, NULL, D3DX_DEFAULT, 0);

		pSrcSurface->Release();
		pDestSurface->Release();

		if(FAILED(hr))
		{
			delete[] pAllocatedData;
			delete pDestTexture;
			break;
		}
	
		size_t nMipPitch = desc.Width * GetBytesPerBlock(eTF_A8R8G8B8);
		size_t nMipSize = desc.Height * nMipPitch;

		outData.resize(oldSize + nMipSize);

		D3DLOCKED_RECT rect;
		pDestTexture->LockRect(0, &rect, NULL, 0/*D3DLOCK_READONLY*/);
		for(uint32 iLine = 0;iLine < desc.Height;++iLine)
			memcpy(&outData[oldSize + iLine * nMipPitch], pAlignedBaseBuffer + rect.Pitch * iLine, nMipPitch);
		pDestTexture->UnlockRect(0);

//////
		//SaveDDS("Test.dds", &outData[oldSize], eTF_A8R8G8B8, desc.Width, desc.Height);
//////

		delete[] pAllocatedData;
		delete pDestTexture;
	}

	return SUCCEEDED(hr);
}

bool CTextureCompressor::CompressMips( const struct CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, const uint8* pBuffer, const uint32 nBufferSize, std::vector<uint8>& output )
{
	const ETEX_Format fmt = GetDXTFormat(pHeader);
	if(fmt == eTF_Unknown)
		return false;

	bool res = false;
	const size_t originalBufferSize = output.size();
	float ratio = 100.f;
	float MCTratio = 100.f;
	if(IsDXTCompressed(fmt))
	{
		res = CompressMipsWithMCT(pHeader, nStartMip, nNumMips, pBuffer, nBufferSize, output);
		MCTratio = (float)output.size() / nBufferSize * 100.f;
		ratio = MCTratio;
	}
	float PTCratio = 100.f;
	if(!res || MCTratio > 30.f)
	{
		std::vector<byte> tempPTC(originalBufferSize);
		const bool PTCRes = CompressMipsWithPTC(pHeader, nStartMip, nNumMips, pBuffer, nBufferSize, tempPTC);
		PTCratio = (float)tempPTC.size() / nBufferSize * 100.f;
		if(PTCRes && PTCratio < MCTratio)
		{
			output = tempPTC;
			ratio = PTCratio;
			res = true;
		}
	}

	float JPEGratio = 100.f;
	if(!res || (MCTratio > 30.f && PTCratio > 10.f))
	{
		std::vector<byte> tempJPEG(originalBufferSize);
		const bool JPEGRes = CompressMipsWithJPEG(pHeader, nStartMip, nNumMips, pBuffer, nBufferSize, tempJPEG);
		JPEGratio = (float)tempJPEG.size() / nBufferSize * 100.f;
		if(JPEGRes && JPEGratio < MCTratio && JPEGratio < PTCratio)
		{
			output = tempJPEG;
			ratio = JPEGratio;
			res = true;
		}
	}

	return res;
}

const ICryCompressorRC::CompressionStatistics& CTextureCompressor::GetStatistics() const
{
	return m_statistics;
}

void CTextureCompressor::SetPlatform( const uint32 platform )
{
	m_platform = platform;
	if(m_platform == ePlatform_X360 || m_platform == ePlatform_PS3)
		m_eEndian = eBigEndian;
	else
		m_eEndian = eLittleEndian;
}

uint32 CTextureCompressor::GetPlatform() const
{
	return m_platform;
}

bool CTextureCompressor::TileTextureLevelForXBox( const byte* pInData, byte* pOutData, const uint32 nWidthInBlocks, 
																								 const uint32 nHeightInBlocks, const ETEX_Format format, const uint32 nPitch, const uint32 nBytesPerBlock )
{
	// check if we can tile it
	const uint32 texSize = nWidthInBlocks * nHeightInBlocks * nBytesPerBlock;
	const uint32 nTexelsPerBlock = IsDXTCompressed(format) ? 4 : 1;
	const UINT nDXTexSize = XGSetTextureHeaderEx(nWidthInBlocks * nTexelsPerBlock, nHeightInBlocks * nTexelsPerBlock,
																							1, 0, GetXBoxTiledFormat(format), 0, XGHEADEREX_NONPACKED, 0, 
																							XGHEADER_CONTIGUOUS_MIP_OFFSET, nPitch, NULL, NULL, NULL);
	if(texSize != nDXTexSize)
		return false;

	std::vector<byte> tempBuffer;
	if(pInData == pOutData) // copy source data
	{

		tempBuffer.resize(texSize);
		memcpy(&tempBuffer[0], pInData, texSize);
		pInData = &tempBuffer[0];
	}

	XGTileSurface(pOutData, nWidthInBlocks, nHeightInBlocks, NULL, pInData, nPitch, NULL, nBytesPerBlock);

	return true;
}

CTextureCompressor::XenonTexture* CTextureCompressor::CreateTexture( const uint32 nWidth, const uint32 nHeight, ETEX_Format destFormat,
																																		const uint32 nStartMip, const uint32 nNumMips )
{
	assert(destFormat != eTF_Unknown);

	XenonTexture* pTexDesc = new XenonTexture;

	// create texture from memory chunk
	pTexDesc->pTexture = new IDirect3DTexture9();
	uint32 nBaseWidth = max(1ul, nWidth>>nStartMip);
	uint32 nBaseHeight = max(1ul, nHeight>>nStartMip);

	UINT nBaseSize;
	UINT nMipSize;
	DWORD dwTextureSize = XGSetTextureHeaderEx( nBaseWidth, nBaseHeight, nNumMips,
																							/*D3DUSAGE_CPU_CACHED_MEMORY*/0, GetXBoxFormat(destFormat), 
																							0, XGHEADEREX_NONPACKED, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0,
																							pTexDesc->pTexture, &nBaseSize, &nMipSize );
	pTexDesc->pAllocatedData = new uint8[nBaseSize + 4097 + nMipSize];
	assert(pTexDesc->pAllocatedData);
	memset(pTexDesc->pAllocatedData, 0, nBaseSize + 4097 + nMipSize);
	uint8* pAlignedBaseBuffer = (uint8*)((uint64)((uint8*)pTexDesc->pAllocatedData + 4096) & (~uint64(0xFFF)));

	XGOffsetResourceAddress( pTexDesc->pTexture, pAlignedBaseBuffer ); 

	return pTexDesc;
}

bool CTextureCompressor::LoadDataFromTexture(XenonTexture* pTexture, uint8* pBuffer, const uint32 nBufferSize, const ETEX_Format format)
{
	assert(pTexture);
	assert(pTexture->pTexture);
	assert(pBuffer);

	uint8* pData = pBuffer;
	for(uint32 iMip=0;iMip<pTexture->pTexture->GetLevelCount();++iMip)
	{
		D3DSURFACE_DESC desc;
		pTexture->pTexture->GetLevelDesc(iMip, &desc);

		uint32 nBlockWidth = desc.Width;
		uint32 nBlockHeight = desc.Height;

		if(IsDXTCompressed(format))
		{
			nBlockWidth  = (nBlockWidth  + 3) / 4;
			nBlockHeight = (nBlockHeight + 3) / 4;
		}

		const uint32 nBlockRowPitch = nBlockWidth * GetBytesPerBlock(format);

		D3DLOCKED_RECT rect;
		pTexture->pTexture->LockRect(iMip, &rect, NULL, D3DLOCK_READONLY);
		const uint8* pRow = (const uint8*)rect.pBits;
		for(uint32 i = 0;i < nBlockHeight;++i)
		{
			memcpy(pData, pRow, nBlockRowPitch);
			pData += nBlockRowPitch;
			pRow += rect.Pitch;
			assert(pData <= pBuffer + nBufferSize);
		}
		pTexture->pTexture->UnlockRect(iMip);
	}
	assert(pData == pBuffer + nBufferSize);

	return true;
}

// Convert to Xbox 360 specific CXT1 normal map format
bool CTextureCompressor::CompressARGB8ToCTX1( const char* pInData, char* pOutData, const uint32 nWidth, const uint32 nHeight )
{
	const POINT point = { 0, 0 };
	const RECT rect = { 0, 0, nWidth, nHeight };
	const UINT DstRowPitch = (nWidth+3)/4 * GetBytesPerBlock(eTF_CTX1);
	const UINT SrcRowPitch = nWidth * GetBytesPerBlock(eTF_A8R8G8B8);
	const HRESULT hr = XGCompressSurface(pOutData, DstRowPitch, nWidth, nHeight, D3DFMT_LIN_CTX1, &point, pInData, SrcRowPitch, D3DFMT_LIN_A8R8G8B8, &rect, 0,0);
	assert(SUCCEEDED(hr));
	return SUCCEEDED(hr);
}

CTextureCompressor::XenonTexture::~XenonTexture()
{
	SAFE_DELETE_ARRAY(pAllocatedData);
	SAFE_DELETE(pTexture);
}