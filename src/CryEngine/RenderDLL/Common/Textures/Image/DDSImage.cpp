/*=============================================================================
  DDSImage.cpp : DDS image file format implementation.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Khonich Andrey
	4/4/8	Refactored by Kaplanyan Anton

=============================================================================*/

#include "StdAfx.h"
#include "CImage.h"
#include "DDSImage.h"
#include "CompressedImage.h"
#include "ImageExtensionHelper.h"				// CImageExtensionHelper
#include "TypeInfo_impl.h"
#include "ImageExtensionHelper_info.h"	
#include "StringUtils.h"								// stristr()
#include "ILog.h"

CImageDDSFile::CImageDDSFile (const string& filename, uint32 nFlags) : CImageFile (filename)
{
	m_pFileMemory = 0;
	if(!Load(filename, nFlags) || NULL == m_pFileMemory)	// load data from file
	{
		if(mfGet_error() == eIFE_OK)
			mfSet_error(eIFE_IOerror, "Texture does not exist");
		return;
	}

	const size_t contentSize = m_pFileMemory->GetSize();
	const byte* ptrBuffer = (byte*)m_pFileMemory->GetData();

	const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&m_DDSHeader);

	// set up flags
	if (!(nFlags & FIM_ALPHA))
	{
		if ((imageFlags & DDS_RESF1_NORMALMAP) || (m_FileName.size() > 4 && (CryStringUtils::stristr(m_FileName, "_ddn"))))
			mfSet_Flags(FIM_NORMALMAP);
	}
	if (imageFlags & CImageExtensionHelper::EIF_Decal)
		m_Flags |= FIM_DECAL;
	if (imageFlags & CImageExtensionHelper::EIF_SRGBRead)
		m_Flags |= FIM_SRGB_READ;
	if (imageFlags & CImageExtensionHelper::EIF_Greyscale)
		m_Flags |= FIM_GREYSCALE;
	if (imageFlags & CImageExtensionHelper::EIF_FileSingle)
		m_Flags |= FIM_FILESINGLE;
	if (imageFlags & CImageExtensionHelper::EIF_X360NotPretiled)
		m_Flags |= FIM_X360_NOT_PRETILED;
	if(imageFlags & CImageExtensionHelper::EIF_AttachedAlpha)
	{
		m_Flags |= FIM_ALPHA;
	}

	// formats fix
	if (m_eSrcFormat == eTF_R8G8B8)
		m_eFormat = eTF_X8R8G8B8;
	else if (m_eSrcFormat == eTF_L8V8U8)
		m_eFormat = eTF_X8L8V8U8;
		
#if defined(DIRECT3D10) && !defined(PS3)
	if (m_eSrcFormat == eTF_L8)
		m_eFormat = eTF_X8R8G8B8;
#endif

	// calculate texture dimensions
  int nWdt = m_Width;
	int nHgt = m_Height;
	int nDpth = m_Depth;
	int nMipsDst = m_NumMips;
  if (nFlags & FIM_STREAM_PREPARE)
  {
		nMipsDst = min((int)DDSSplitted::etexNumLastMips, nMipsDst);
    nWdt = max(1, m_Width >> (m_NumMips-nMipsDst));
		nHgt = max(1, m_Height >> (m_NumMips-nMipsDst));
		nDpth = max(1, nDpth >> (m_NumMips-nMipsDst));
  }

	// calculate texture mips' sizes
	const int sizeSrc = CTexture::TextureDataSize(nWdt, nHgt, nDpth, nMipsDst, m_eSrcFormat);
	const int sizeSrcSlice = CTexture::TextureDataSize(nWdt, nHgt, 1, nMipsDst, m_eSrcFormat);
	const int sizeImg = CTexture::TextureDataSize(nWdt, nHgt, nDpth, nMipsDst, m_eFormat);
	const int sizeDst = CTexture::TextureDataSize(nWdt, nHgt, nDpth, nMipsDst, m_eFormat);
	mfSet_ImageSize(sizeImg);

	// offsets
	int nOffsSrc = 0;
	int nOffsDst = 0;

	// loading mips from file
	for (int nS=0; nS<m_Sides; nS++)
  {
    nOffsDst = 0;

		mfFree_image(nS);
    mfGet_image(nS);

		// stop of an allocation failed
		if( m_pByteImage[nS] == NULL )
		{
			// free already allocated data
			for( int i = 0 ; i < nS ; ++i )
			{
				mfFree_image(i);
			}
			mfSet_ImageSize(0);
			mfSet_error(eIFE_OutOfMemory, "Failed to allocate Memory");
			return;
		}
		assert(nOffsDst + sizeDst <= sizeImg);
		assert(nOffsSrc + sizeSrc <= contentSize);

		CTexture::ExpandMipFromFile(m_pByteImage[nS] + nOffsDst, sizeDst, ptrBuffer + nOffsSrc, sizeSrc, m_eSrcFormat);
		nOffsDst += sizeDst;
		nOffsSrc += sizeSrcSlice;
  }

	// We don't need file memory anymore, free it.
	m_pFileMemory = 0;
}

CImageDDSFile::~CImageDDSFile ()
{
}

//////////////////////////////////////////////////////////////////////
bool CImageDDSFile::Load( const string& filename, uint32 nFlags )
{
	// load file content
	if(DDSSplitted::IsSplitted(filename))
	{
		if(!LoadFromSplittedFile(filename, nFlags)) return false;
	}
	else
	{
		if(!LoadFromFile(filename, nFlags)) return false;
	}

	return true;
}

bool CImageDDSFile::LoadFromFile( const string& filename, uint32 nFlags )
{
	// trying to open splitted file
	CDebugAllowFileAccess dafa;
	CCryFile file(filename, "rb");
	dafa.End();

	if(file.GetHandle() == NULL)
	{
		//assert(0);
		return false;
	}

	const size_t fileSize = file.GetLength();

	_smart_ptr<IMemoryBlock> pImageMemory;

	// alloc space for header
	CImageExtensionHelper::DDS_FILE_DESC ddsHeader;
	file.ReadRaw(&ddsHeader, sizeof(CImageExtensionHelper::DDS_FILE_DESC));

	m_nStartSeek = sizeof(CImageExtensionHelper::DDS_FILE_DESC);

	// set image header to this object
	if(!SetHeaderFromMemory((byte*)&ddsHeader, nFlags & ~FIM_ALPHA)) return false;

	const size_t nDDSSize = sizeof(CImageExtensionHelper::DDS_FILE_DESC) + CTexture::TextureDataSize(m_Width, m_Height, m_Depth, m_NumMips, m_eFormat) * m_Sides;

	// load file(take into account attached alpha channel)
	if(!(nFlags & FIM_ALPHA))
	{
		size_t readSize = 0;
		size_t nSizeToRead = 0;
		if(!(nFlags & FIM_STREAM_PREPARE))
		{
			nSizeToRead = CTexture::TextureDataSize(m_Width, m_Height, m_Depth, m_NumMips, m_eFormat) * m_Sides;
			pImageMemory = gEnv->pCryPak->PoolAllocMemoryBlock(nSizeToRead,"CImageDDSFile::LoadFromFile");
			readSize = file.ReadRaw(pImageMemory->GetData(), nSizeToRead);
		}
		else
		{
			if(m_Depth > 1)		//  we don't support streaming and preload for volume textures
			{
				mfSet_error(eIFE_BadFormat, "Unable stream volume texture");
				return false;
			}

			if(!(nFlags & FIM_SPLITTED) && m_Sides > 1)
			{
				mfSet_error(eIFE_BadFormat/*, "Unable stream volume texture"*/);
				return false;
			}

			// calc size of mips
			int nNumMips = min(m_NumMips, DDSSplitted::GetNumLastMips(m_Width, m_Height, m_NumMips, m_Sides, m_eFormat, nFlags));
			int nDeltaMips = m_NumMips - nNumMips;
			int nHeight = max(1, m_Height >> nDeltaMips);
			int nWidth = max(1, m_Width >> nDeltaMips);
			size_t nSideSize = CTexture::TextureDataSize(nWidth, nHeight, m_Depth, m_NumMips, m_eFormat);
			size_t nSideSizeToRead = CTexture::TextureDataSize(nWidth, nHeight, m_Depth, nNumMips, m_eFormat);
			nSizeToRead = nSideSizeToRead * m_Sides;

			pImageMemory = gEnv->pCryPak->PoolAllocMemoryBlock(nSizeToRead,"CImageDDSFile::LoadFromFile");

			// set to beginning of low MIPs, read all sides at once
			size_t fileOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC);
			if(nDeltaMips > 0)
				fileOffset += CTexture::TextureDataSize(m_Width, m_Height, m_Depth, nDeltaMips, m_eFormat);
			for(int iSide = 0;iSide < m_Sides;++iSide)
			{
				assert(nSideSizeToRead + fileOffset <= fileSize);
				file.Seek(fileOffset, SEEK_SET);
				size_t SideReadSize = file.ReadRaw(pImageMemory->GetData(), nSideSizeToRead);
				readSize += SideReadSize;
				assert(SideReadSize == nSideSizeToRead);
				fileOffset += nSideSize;
			}
		}

		// decompress PTC or MCT data
		if(readSize < nSizeToRead)
		{
			const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&m_DDSHeader);













			assert(0);
			assert(!(CImageExtensionHelper::GetImageFlags(&m_DDSHeader) & CImageExtensionHelper::EIF_Compressed));

		}
	}
	else	// if we load attached alpha then we need to read whole the rest of file since we don't know where is starts exactly
	{
		if(fileSize > nDDSSize + sizeof(CImageExtensionHelper::DDS_HEADER))	// check if we have enough size for attached alpha
		{
			_smart_ptr<IMemoryBlock> pTempBlock = gEnv->pCryPak->PoolAllocMemoryBlock(fileSize - nDDSSize,"CImageDDSFile::LoadFromFile");
			byte *pTempBuf = (byte*)pTempBlock->GetData();
			file.Seek(nDDSSize, SEEK_SET);
			file.ReadRaw(pTempBuf, fileSize - nDDSSize);
			if(byte* attachedImage = (byte*)CImageExtensionHelper::GetAttachedImage(pTempBuf))
			{
				const size_t offset = (attachedImage - pTempBuf) + sizeof(CImageExtensionHelper::DDS_HEADER);
				size_t attachedImageSize = pTempBlock->GetSize() - offset;
				m_nStartSeek = nDDSSize + offset;
				if(!SetHeaderFromMemory(attachedImage, nFlags)) return false;
				const size_t nAlphaDDSSize = CTexture::TextureDataSize(m_Width, m_Height, m_Depth, m_NumMips, m_eFormat) * m_Sides;
				if(attachedImageSize < nAlphaDDSSize) return false;	// if we have not enough data

				pImageMemory = gEnv->pCryPak->PoolAllocMemoryBlock(attachedImageSize,"CImageDDSFile::LoadFromFile");
				memcpy(pImageMemory->GetData(), attachedImage + sizeof(CImageExtensionHelper::DDS_HEADER), attachedImageSize);

				m_eFormat = eTF_A8;
				m_eSrcFormat = eTF_A8;
				mfSet_Flags(FIM_ALPHA);
			}
			else
			{
				mfSet_error(eIFE_ChunkNotFound/*, "Attached alpha not found"*/);
				return false;
			}
		}
		else
		{
			mfSet_error(eIFE_ChunkNotFound/*, "Attached alpha not found"*/);
			return false;
		}
	}

	m_pFileMemory = pImageMemory;
	return true;
}

bool CImageDDSFile::LoadFromSplittedFile( const string& filename, uint32 nFlags )
{
	// set splitted flag
	m_Flags |= FIM_SPLITTED;

	string firstChunkName = DDSSplitted::MakeName(filename, 0, nFlags);

	CCryFile firstChunk(firstChunkName, "rb");
	if(firstChunk.GetHandle() == NULL)
	{
		assert(0);
		return false;
	}

	const size_t firstChunkSize = firstChunk.GetLength();

	int headerOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC);
	if(nFlags & FIM_ALPHA)
	{
		headerOffset = sizeof(CImageExtensionHelper::DDS_HEADER);
		CImageExtensionHelper::DDS_HEADER ddsHeader;
		firstChunk.ReadRaw(&ddsHeader, headerOffset);
		if(!SetHeaderFromMemory((byte*)&ddsHeader, nFlags)) return false;
	}
	else
	{
		CImageExtensionHelper::DDS_FILE_DESC ddsHeader;
		headerOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC);
		firstChunk.ReadRaw(&ddsHeader, headerOffset);
		if(!SetHeaderFromMemory((byte*)&ddsHeader, nFlags)) return false;
	}

	m_nStartSeek = headerOffset;

	const size_t nDataSize = CTexture::TextureDataSize(m_Width, m_Height, m_Depth, m_NumMips, m_eFormat) * m_Sides;
	const size_t nDDSSize = headerOffset + nDataSize;

	firstChunk.Seek(headerOffset, SEEK_SET);

	int nNumMips = min(m_NumMips, DDSSplitted::GetNumLastMips(m_Width, m_Height, m_NumMips, m_Sides, m_eFormat, nFlags));
	int nDeltaMips = m_NumMips - nNumMips;
	int nHeight = max(1, m_Height >> nDeltaMips);
	int nWidth = max(1, m_Width >> nDeltaMips);
	int nDepth = max(1, m_Depth >> nDeltaMips);
	const size_t nFirstDataSize = CTexture::TextureDataSize(nWidth, nHeight, nDepth, nNumMips, m_eFormat) * m_Sides;
	
	_smart_ptr<IMemoryBlock> pLowMipsBlock = gEnv->pCryPak->PoolAllocMemoryBlock(nFirstDataSize,"CImageDDSFile::LoadFromSplittedFile");
	m_pFileMemory = pLowMipsBlock;
	
	const size_t readSize = firstChunk.ReadRaw(pLowMipsBlock->GetData(), nFirstDataSize);
	// decompress PTC or MCT data
	/*if(readSize < nFirstDataSize)
	{
		const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&m_DDSHeader);
#ifdef XENON
		if(!CCompressedImageDDSFile::Decompress(&vecContent[0], readSize, &vecContent[0], nFirstDataSize))				
		{
			assert(0);
			mfSet_error (eIFE_BadFormat, "Failed to decompress compressed image!");
			return false;
		}
#else
		assert(!(CImageExtensionHelper::GetImageFlags(&m_DDSHeader) & CImageExtensionHelper::EIF_Compressed));
		assert(0);
#endif
	}*/

	if(nFlags & FIM_STREAM_PREPARE || nDeltaMips == 0)
		return true;

	if(m_Sides > 1)
	{
		mfSet_error(eIFE_BadFormat, "Don't support cubemap preloading");
		return false;
	}

	_smart_ptr<IMemoryBlock> pAllMipsBlock = gEnv->pCryPak->PoolAllocMemoryBlock(nDDSSize,"CImageDDSFile::LoadFromSplittedFile");
	m_pFileMemory = pAllMipsBlock;

	// move last mips data to back of array
	const size_t nLastMipsOffset = CTexture::TextureDataSize(m_Width, m_Height, m_Depth, nDeltaMips, m_eFormat) * m_Sides;
	const size_t nSideSize = CTexture::TextureDataSize(m_Width, m_Height, m_Depth, m_NumMips, m_eFormat);

	assert(nLastMipsOffset + nFirstDataSize == nDataSize);

	// Copy low mips at the end of image data
	memcpy( (byte*)pAllMipsBlock->GetData() + nLastMipsOffset, pLowMipsBlock->GetData(), nFirstDataSize);

	const size_t nHighterMipsSize = nLastMipsOffset;

	// load the rest chunks
	const size_t bytesRead = DDSSplitted::LoadMips(filename, (byte*)pAllMipsBlock->GetData(), sizeof(CImageExtensionHelper::DDS_FILE_DESC), nHighterMipsSize,
																									m_Width, m_Height, m_Depth, m_Sides, 
																									m_NumMips, 0, nDeltaMips - 1, DDSSplitted::GetNumLastMips(m_Width, m_Height, m_NumMips, m_Sides, m_eFormat, nFlags), m_eSrcFormat, nFlags | FIM_SPLITTED);
	assert(bytesRead == nHighterMipsSize);

	if(bytesRead == 0)
		return false;

	return true;
}

bool CImageDDSFile::SetHeaderFromMemory( byte* pFileStart, uint32 nFlags )
{
	// load DDS header
	if(!(nFlags & FIM_ALPHA))
	{
		CImageExtensionHelper::DDS_FILE_DESC& dds = *(CImageExtensionHelper::DDS_FILE_DESC*)pFileStart;

		SwapEndian(dds);

		if(!dds.IsValid())
		{
			mfSet_error (eIFE_BadFormat, "Bad DDS header");
			return false;
		}

		m_DDSHeader = dds.header;
	}
	else
	{
		CImageExtensionHelper::DDS_HEADER& ddsh = *(CImageExtensionHelper::DDS_HEADER*)pFileStart;

		SwapEndian(ddsh);

		if(!ddsh.IsValid())
		{
			mfSet_error (eIFE_BadFormat, "Bad DDS header");
			return false;
		}

		m_DDSHeader = ddsh;
	}
	
	// check for nativeness of texture
	const uint32 imageFlags = CImageExtensionHelper::GetImageFlags(&m_DDSHeader);
	if(!CImageExtensionHelper::IsImageNative(imageFlags))
	{
		mfSet_error(eIFE_BadFormat, "Not converted for this platform");
		return false;
	}

	// setup texture properties
	m_Width = m_DDSHeader.dwWidth;
	m_Height = m_DDSHeader.dwHeight;

	m_eFormat = DDSFormats::GetFormatByDesc(m_DDSHeader.ddspf);
	if ( eTF_Unknown == m_eFormat )
	{
		mfSet_error (eIFE_BadFormat, "Unknown DDS pixel format!");
		return false;
	}

	m_eSrcFormat = m_eFormat;
	mfSet_numMips(m_DDSHeader.GetMipCount());
	m_Depth = max((uint32)1ul, (uint32)m_DDSHeader.dwDepth);

	m_Sides = 1;
	if ((m_DDSHeader.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) && (m_DDSHeader.dwCubemapFlags & DDS_CUBEMAP_ALLFACES))
		m_Sides = 6;
 //Cut Diesel bug dds (only dx10 norma)
/*
	if(DDSFormats::IsNormalMap(m_eFormat))
	{
		const int nLastMipWidth = m_Width >> (m_NumMips-1);
		const int nLastMipHeight = m_Height >> (m_NumMips-1);
		if(nLastMipWidth < 4 || nLastMipHeight < 4 )
			mfSet_error (eIFE_BadFormat, "3DC/CTX1 texture has wrong number of mips");
		
	}
*/

//Diesel  patch (dx11 skoree budet bug, chem huy ego znaet)
#if defined (DIRECT3D9)
 //Cut Diesel bug dds 
/*
	if(DDSFormats::IsNormalMap(m_eFormat))
	{
		const int nLastMipWidth = m_Width >> (m_NumMips-1);
		const int nLastMipHeight = m_Height >> (m_NumMips-1);
		if(nLastMipWidth < 4 || nLastMipHeight < 4 )
			mfSet_error (eIFE_BadFormat, "3DC/CTX1 texture has wrong number of mips");
	}
*/

#elif defined (DIRECT3D10)
return true;

#else
	if(DDSFormats::IsNormalMap(m_eFormat))
	{
		const int nLastMipWidth = m_Width >> (m_NumMips-1);
		const int nLastMipHeight = m_Height >> (m_NumMips-1);
		if(nLastMipWidth < 4 || nLastMipHeight < 4 )
			mfSet_error (eIFE_BadFormat, "3DC/CTX1 texture has wrong number of mips");
	} 

#endif
//Diesel  patch end


	return true;
}

//////////////////////////////////////////////////////////////////////
byte *WriteDDS(byte*dat, int wdt, int hgt, int dpth, const char*name, ETEX_Format eTF, int nMips, ETEX_Type eTT, bool bToMemory, int *pSize)
{
  CImageExtensionHelper::DDS_FILE_DESC fileDesc;
  memset(&fileDesc, 0, sizeof(fileDesc));
  byte *pData = NULL;
  CCryFile file;
  int nOffs = 0;
  int nSize = CTexture::TextureDataSize(wdt, hgt, dpth, nMips, eTF);

  fileDesc.dwMagic = MAKEFOURCC('D','D','S',' ');

  if (!bToMemory)
  {
    if (!file.Open(name, "wb"))
      return false;

    file.Write(&fileDesc.dwMagic, sizeof(fileDesc.dwMagic));
  }
  else
  {
    pData = new byte[sizeof(fileDesc)+nSize];
    *(DWORD *)pData = fileDesc.dwMagic;
    nOffs += sizeof(fileDesc.dwMagic);
  }

  fileDesc.header.dwSize = sizeof(fileDesc.header);
  fileDesc.header.dwWidth = wdt;
  fileDesc.header.dwHeight = hgt;
  fileDesc.header.dwMipMapCount = max(1, nMips);
  fileDesc.header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
  fileDesc.header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
	fileDesc.header.dwTextureStage = 'CRYF';
	fileDesc.header.dwReserved1[0] = 0;
  int nSides = 1;
  if (eTT == eTT_Cube)
  {
    fileDesc.header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_CUBEMAP;
    fileDesc.header.dwCubemapFlags |= DDS_CUBEMAP_ALLFACES;
    nSides = 6;
  }
  else if (eTT == eTT_3D)
  {
    fileDesc.header.dwHeaderFlags |= DDS_HEADER_FLAGS_VOLUME;
  }
  if (eTT != eTT_3D)
    dpth = 1;
  fileDesc.header.dwDepth = dpth;
  if (name)
  {
    size_t len = strlen(name);
	  if (len > 4)
	  {
		  if (!stricmp(&name[len-4], ".ddn"))
			  fileDesc.header.dwReserved1[0] = DDS_RESF1_NORMALMAP;
	  }
  }
	fileDesc.header.ddspf = DDSFormats::GetDescByFormat(eTF);
  fileDesc.header.dwPitchOrLinearSize = CTexture::TextureDataSize(wdt, 1, 1, 1, eTF);
  if (!bToMemory)
  {
    file.Write(&fileDesc.header, sizeof(fileDesc.header));

    nOffs = 0;
    int nSide;
    for (nSide=0; nSide<nSides; nSide++)
    {
      file.Write(&dat[nOffs], nSize);
      nOffs += nSize;
    }
  }
  else
  {
    memcpy(&pData[nOffs], &fileDesc.header, sizeof(fileDesc.header));
    nOffs += sizeof(fileDesc.header);

    int nSide;
    int nSrcOffs = 0;
    for (nSide=0; nSide<nSides; nSide++)
    {
      memcpy(&pData[nOffs], &dat[nSrcOffs], nSize);
      nSrcOffs += nSize;
      nOffs += nSize;
    }

    if (pSize)
      *pSize = nOffs;
    return pData;
  }

  return NULL;
}

//////////////////////////////////////////////////////////////////////
namespace DDSSplitted
{
	bool IsSplitted( const string& sOriginalName )
	{
		// there is no need in this on consoles(since all textures are preprocessed for streaming)
		// and there is no preprocessed textures on PC
		//if(gEnv->pSystem->IsDevMode() && gEnv->pCryPak->IsFileExist(sOriginalName))
		//	return false;		// priority to non-splitted files in devmode
		return gEnv->pCryPak->IsFileExist(MakeName(sOriginalName, 0, 0),ICryPak::eFileLocation_Any);
	}

	string MakeName( const string& sOriginalName, const uint32 nChunk, const uint32 nFlags )
	{
		string sFullDestFileName(sOriginalName);

		char buffer[10];
		sprintf(buffer, ".%d", nChunk);

		sFullDestFileName += buffer;
		if(nFlags & FIM_ALPHA)
			sFullDestFileName += 'a';	// additional suffix for attached alpha channel

		return sFullDestFileName;
	}

	bool GetFilesToRead(Chunks& files, const string& sOriginalName, const size_t offset, const size_t size, 
											int nWidth, int nHeight, int nDepth,int nSides,
											int nNumMips, int nStartMip, int nEndMip, int nMipsPersistent, const ETEX_Format format, 
											const uint32 nFlags)
	{
		assert(nStartMip <= nEndMip);
		assert(nEndMip < nNumMips);

		const bool bSplitted = (nFlags & FIM_SPLITTED) != 0;

		size_t initialOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC);		// starting DDS m_nOffsetInFile
		if(nFlags & FIM_ALPHA)
		{
			if(bSplitted)
				initialOffset = sizeof(CImageExtensionHelper::DDS_HEADER);
			else	// respect the original offset inside the solid file
				initialOffset = offset;
		}

		const size_t DDSDataSize = CTexture::TextureDataSize(nWidth, nHeight, nDepth, nNumMips, format) * nSides;	// size of raw dds mips

		size_t lastMipsDataSize = 0;
		if(nNumMips > nMipsPersistent)
			lastMipsDataSize = CTexture::TextureDataSize(nWidth, nHeight, nDepth, nNumMips - nMipsPersistent, format) * nSides;	// size of last few mips to preload

		size_t sizeToRead = 0;

		files.reserve(max(0, nEndMip - nStartMip + 1));
		int lastChunk = -1;
		for(int mip = nStartMip;mip <= nEndMip;++mip)
		{
			const int chunkNumber = mip >= (nNumMips - nMipsPersistent) ? 0 : nNumMips - mip - nMipsPersistent;	// number of file chunk number

			const int nMipWidth = max(1, nWidth >> mip);
			const int nMipHeight = max(1, nHeight >> mip);
			const int nMipDepth = max(1, nDepth >> mip);

			const int chunkSize = CTexture::TextureDataSize(nMipWidth, nMipHeight, nMipDepth, 1, format) * nSides;		// size of mip

			if(lastChunk != chunkNumber)	// new chunk
			{
				ChunkInfo newChunk;
				if(bSplitted)
					newChunk.fileName = MakeName(sOriginalName, chunkNumber, nFlags);
				else
					newChunk.fileName = sOriginalName;
				// calc m_nOffsetInFile in non-splitted original file(sort order for read)
				newChunk.m_nOriginalOffset = initialOffset + DDSDataSize - CTexture::TextureDataSize(nMipWidth, nMipHeight, nDepth, nNumMips - mip, format) * nSides;	
				newChunk.size = chunkSize;
				if(bSplitted)
				{
					if(chunkNumber > 0)
						newChunk.m_nOffsetInFile = 0;
					else	// we need to separate it
						newChunk.m_nOffsetInFile = newChunk.m_nOriginalOffset - lastMipsDataSize;		// if we have last chunk, we have few last mips together, we need m_nOffsetInFile it
				}
				else
					newChunk.m_nOffsetInFile = newChunk.m_nOriginalOffset;

				newChunk.m_nMipLevel = mip;

				files.push_back(newChunk);
			}
			else	// add to the last one
			{
				assert(!files.empty());
				files.back().size += chunkSize;
			}

			sizeToRead += chunkSize;
			lastChunk = chunkNumber;
		}

		std::sort(files.begin(), files.end());		// sort chunks in sequential order

		assert(sizeToRead == size);

		return true;
	}

	size_t LoadMips( const string& sOriginalName, byte* pBuffer, const size_t offset, const size_t size, 
										int nWidth, int nHeight, int nDepth, int nSides,
										const int nNumMips, const int nStartMip, const int nEndMip, int nMipsPersistent, const ETEX_Format format, const uint32 nFlags )
	{
		if((nFlags & FIM_SPLITTED) == 0)	// just load the raw file part
		{
			CCryFile file(sOriginalName, "rb");
			if(!file.GetHandle())
			{
				assert(0);
				return 0;
			}
			if(file.GetLength() < offset + size)
			{
				assert(0);
				return 0;
			}
			size_t bytesRead = 0;
			
			const size_t nSideSize = CTexture::TextureDataSize(nWidth, nHeight, nDepth, nNumMips, format);	// size of one cubemap side
			const size_t nSideSizeToRead = CTexture::TextureDataSize(max(1, nWidth >> nStartMip), max(1, nHeight >> nStartMip), max(1, nDepth >> nStartMip), nEndMip - nStartMip + 1, format);
			assert(nSideSizeToRead * nSides == size);
			for(int iSide = 0;iSide < nSides;++iSide)
			{
				assert(file.GetLength() >= offset + nSideSize * iSide + nSideSizeToRead);
				file.Seek(offset + nSideSize * iSide, SEEK_SET);
				bytesRead += file.ReadRaw(pBuffer + nSideSizeToRead * iSide, nSideSizeToRead);
			}
			return bytesRead;
		}

		// else loading parts of splitted file
		Chunks names;	// chunk-files to load
		if(!GetFilesToRead(names, sOriginalName, offset, size, 
												nWidth, nHeight, nDepth, nSides,
												nNumMips, nStartMip, nEndMip, nMipsPersistent, format, nFlags) || names.empty())
			return 0;

		CCryFile file;

		// load files
		int nOffset = 0;
		const Chunks::const_iterator end = names.end();
		for(Chunks::const_iterator it = names.begin();it != end;++it)
		{
			const ChunkInfo& chunk = *it;
      if(!file.Open(chunk.fileName, "rb"))
      {
        assert(0);
        return 0;
      }

      if(nOffset + chunk.size > size)
      {
        assert(0);
        return 0;
      }

      if(chunk.m_nOffsetInFile + chunk.size > file.GetLength())
      {
        assert(0);
        return 0;
      }

			file.Seek(chunk.m_nOffsetInFile, SEEK_SET);
			const size_t readBytes = file.ReadRaw(pBuffer + nOffset, chunk.size);

			if(readBytes == 0)
			{
				assert(0);
				return 0;
			}

			// inplace decompression for PTC & MCT(if needed)
			if(readBytes < chunk.size)
			{
				const bool res = CCompressedImageDDSFile::Decompress(pBuffer + nOffset, readBytes, pBuffer + nOffset, chunk.size);
				assert(res);
			}

			nOffset += chunk.size;
		}

		return size;
	}
	int GetNumLastMips(const int nWidth, const int nHeight, const int nNumMips, const int nSides, ETEX_Format eTF, const uint32 nFlags)
	{
		return (int)DDSSplitted::etexNumLastMips;
	}
}
