/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: CryCompressorRC.h,v 1.0 2008/02/14 15:18:23 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  header file that defines the compressor class
							purposes: compression of any kind of data for loading and streaming purposes
-------------------------------------------------------------------------
History:
- 14:2:2008 15:18 : Created by Anton Kaplanyan
*************************************************************************/

#pragma once

#ifndef __RC_XENON_COMPRESSOR_H__
#define __RC_XENON_COMPRESSOR_H__

#include "ICryCompressorRC.h"
#include <queue>
#include <Endian.h>

class CTextureCompressor : public ICryCompressorRC
{
private:
	IResourceCompiler *											m_pRC;

	CompressionStatistics										m_statistics;

	uint32																	m_platform;
	EEndian																	m_eEndian;

	struct XenonTexture
	{
		void*	pAllocatedData;
		IDirect3DTexture9* pTexture;
		~XenonTexture();
	};

	bool ConvertToARGB8(XenonTexture* pInData, std::vector<byte>& outData);
public:
	static ICryCompressorRC* Create( IResourceCompiler *pRC );
	virtual ~CTextureCompressor();
	virtual bool CompressMipsWithMCT(const struct CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
																	const uint8* pBuffer, const uint32 nBufferSize,
																	std::vector<uint8>& output);
	virtual bool CompressMipsWithPTC(const struct CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
																	const uint8* pBuffer, const uint32 nBufferSize,
																	std::vector<uint8>& output);
	virtual bool CompressMipsWithJPEG(const struct CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
																	const uint8* pBuffer, const uint32 nBufferSize,
																	std::vector<uint8>& output);

	virtual bool QuantizeWithMCT(const uint32 nWidth, const uint32 nHeight, const ETEX_Format format, uint8* pBuffer, const uint32 nBufferSize, const float fQuality/*1..100*/);

	virtual bool CompressMips(const struct CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
															const uint8* pBuffer, const uint32 nBufferSize,
															std::vector<uint8>& output);

	// Convert to Xbox 360 specific CXT1 normal map format
	virtual bool CompressARGB8ToCTX1(const char* pInData, char* pOutData, const uint32 nWidth, const uint32 nHeight);

	virtual void SetPlatform(const uint32 platform);
	virtual uint32 GetPlatform() const;

	virtual const CompressionStatistics& GetStatistics() const;

	virtual bool TileTextureLevelForXBox(const byte* pInData, byte* pOutData, const uint32 nWidthInBlocks, const uint32 nHeightInBlocks, 
																				const ETEX_Format format, const uint32 nPitch, const uint32 nBytesPerBlock);
private:
	CTextureCompressor( IResourceCompiler *pRC );

	XenonTexture* LoadDataIntoTexture(const uint32 nWidth, const uint32 nHeight, 
																		const uint32 nStartMip, const uint32 nNumMips, 
																		const uint8* pBuffer, const uint32 nBufferSize, ETEX_Format destFormat);

	bool LoadDataFromTexture(XenonTexture* pTexture, uint8* pBuffer, const uint32 nBufferSize, const ETEX_Format format);

	XenonTexture* CreateTexture(const uint32 nWidth, const uint32 nHeight, ETEX_Format destFormat,
															const uint32 nStartMip, const uint32 nNumMips);
};

#endif // __RC_XENON_COMPRESSOR_H__
