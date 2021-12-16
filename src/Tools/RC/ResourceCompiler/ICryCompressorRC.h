/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: ICryCompressorRC.h,v 1.0 2008/02/14 15:18:23 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  interface for compressor routine
-------------------------------------------------------------------------
History:
- 14:2:2008 15:18 : Created by Anton Kaplanyan
*************************************************************************/

#pragma once

#ifndef __RC_ICOMPRESSOR_H__
#define __RC_ICOMPRESSOR_H__

#include <ITexture.h>

namespace CImageExtensionHelper
{
	struct DDS_HEADER;
};

enum ECompressorType
{
	ectPTC = 0,
	ectMCT,
	ectJPEG,

	ectTotalCompressors,
};

class ICryCompressorRC
{
public:
	class CompressionStatistics
	{
	private:
		class Ratio
		{
		protected:
			float	sumRatio;
			uint32 nCompressed;
		public:
			Ratio() : sumRatio(0.f), nCompressed(0) { }
			void AddRatio(const float ratio) { sumRatio += ratio; nCompressed++; }
			const float GetRatio() const { return sumRatio / max(1u, nCompressed); }
		};

		Ratio m_ratios[ectTotalCompressors];
	public:
		CompressionStatistics() { }

		template<ECompressorType type>
		const float GetAvgRatio() const { return m_ratios[type].GetRatio(); }

		template<ECompressorType type>
		void AddRatio(const float ratio) { m_ratios[type].AddRatio(ratio); }
	};
protected:
	ICryCompressorRC() {}
	virtual ~ICryCompressorRC() {}
public:
	virtual bool CompressMipsWithMCT(const CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
																	const uint8* pBuffer, const uint32 nBufferSize,
																	std::vector<uint8>& output) = 0;
	virtual bool CompressMipsWithPTC(const CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
																	const uint8* pBuffer, const uint32 nBufferSize,
																	std::vector<uint8>& output) = 0;
	virtual bool CompressMipsWithJPEG(const struct CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
																	const uint8* pBuffer, const uint32 nBufferSize,
																	std::vector<uint8>& output) = 0;

	virtual bool QuantizeWithMCT(const uint32 nWidth, const uint32 nHeight, const ETEX_Format format, uint8* pBuffer, const uint32 nBufferSize, const float fQuality/*1..100*/) = 0;

	virtual bool CompressMips(const CImageExtensionHelper::DDS_HEADER* const pHeader, const uint32 nStartMip, const uint32 nNumMips, 
														const uint8* pBuffer, const uint32 nBufferSize,
														std::vector<uint8>& output) = 0;

	virtual void SetPlatform(const uint32 platform) = 0;
	virtual uint32 GetPlatform() const = 0;

	virtual const CompressionStatistics& GetStatistics() const = 0;

	// Convert to Xbox 360 specific CXT1 normal map format
	// It is placed here and not in ImageCompiler because of conflict of DirectX libraries for PC / Xbox 360
	virtual bool CompressARGB8ToCTX1(const char* pInData, char* pOutData, const uint32 nWidth, const uint32 nHeight) = 0;

	// XBox specific helper
	virtual bool TileTextureLevelForXBox(const byte* pInData, byte* pOutData, const uint32 nWidthInBlocks, const uint32 nHeightInBlocks, 
																			const ETEX_Format format, const uint32 nPitch, const uint32 nBytesPerBlock) = 0;
};

#endif // __RC_ICOMPRESSOR_H__
