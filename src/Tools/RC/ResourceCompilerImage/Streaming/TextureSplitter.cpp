/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: TextureSplitter.cpp,v 1.0 2008/01/17 15:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for creation of streaming pak
							file from file with list of resources
-------------------------------------------------------------------------
History:
- 17:1:2008   10:31 : Created by Anton Kaplanyan

*************************************************************************/

#include "StdAfx.h"
#include <IConfig.h>
#include <ICryCompressorRC.h>
#include "TextureSplitter.h"
#include "..\..\ResourceCompiler\IResCompiler.h"
#include <BitFiddling.h>
#include <ImageExtensionHelper.h>
#include <IConsole.h>
#include "DbgHelp.h"
#include "FileUtil.h"
#include "IRCLog.h"
#include "IResCompiler.h"

//#define DEBUG_DISABLE_TEXTURE_SPLITTING
//#define DEBUG_FILE_SIZE_LIMIT 140000

#ifndef COMPILE_TIME_ASSERT
	#define COMPILE_TIME_ASSERT(x) { switch(false) { case(false):case(x):; } }
#endif

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
				((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |       \
				((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))

const string CTextureSplitter::s_sInputExtensions[] = { "dds" };

// global convention for naming files and directories inside pak. 
namespace TextureHelper
{
	static ETEX_Format GetTextureFormat( const CImageExtensionHelper::DDS_HEADER *ddsh )
	{
		assert(ddsh);

		if (ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_DXT1.dwFourCC)
			return eTF_DXT1;
		else if (ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_DXT3.dwFourCC)
			return eTF_DXT3;
		else if (ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_DXT5.dwFourCC)
			return eTF_DXT5;
		else if (ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_3DC.dwFourCC)
			return eTF_3DC;
		else if (ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_CTX1.dwFourCC)
			return eTF_CTX1;
		else if( ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_R32F.dwFourCC)
			return eTF_R32F;
		else if( ddsh->ddspf == DDSFormats::DDSPF_V16U16)
			return eTF_V16U16;  
		else if( ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_G16R16F.dwFourCC)
			return eTF_G16R16F;  
		else if (ddsh->ddspf.dwFlags == DDS_RGBA && ddsh->ddspf.dwRGBBitCount == 32 && ddsh->ddspf.dwABitMask == 0xff000000)
			return eTF_A8R8G8B8;
		else if (ddsh->ddspf.dwFourCC == DDSFormats::DDSPF_A16B16G16R16F.dwFourCC)
			return eTF_A16B16G16R16F;
		else if (ddsh->ddspf.dwFlags == DDS_RGBA && ddsh->ddspf.dwRGBBitCount == 16)
			return eTF_A4R4G4B4;
		else if (ddsh->ddspf.dwFlags == DDS_RGB  && ddsh->ddspf.dwRGBBitCount == 24)
			return eTF_R8G8B8;
		else if (ddsh->ddspf.dwFlags == DDS_RGB  && ddsh->ddspf.dwRGBBitCount == 32)
			return eTF_X8R8G8B8;
		else if (ddsh->ddspf.dwFlags == DDS_LUMINANCE  && ddsh->ddspf.dwRGBBitCount == 8)
			return eTF_L8;
		else if ((ddsh->ddspf.dwFlags == DDS_A || ddsh->ddspf.dwFlags == DDS_A_ONLY) && ddsh->ddspf.dwRGBBitCount == 8)
			return eTF_A8;
		else if (ddsh->ddspf.dwRGBBitCount == 8)
			return eTF_A8;
		else
		{
			assert(0);
			return eTF_Unknown;
		}
	}

	static int BytesPerBlock(const ETEX_Format eTF)
	{
		switch (eTF)
		{
			case eTF_A8R8G8B8:
			case eTF_X8R8G8B8:
			case eTF_X8L8V8U8:
				return 4;
			case eTF_A4R4G4B4:
				return 2;
			case eTF_R8G8B8:
			case eTF_L8V8U8:
				return 3;
			case eTF_V8U8:
				return 2;
			case eTF_R5G6B5:
			case eTF_R5G5B5:
				return 2;
			case eTF_A16B16G16R16:
			case eTF_A16B16G16R16F:
				return 8;
			case eTF_A32B32G32R32F:
				return 16;
			case eTF_G16R16:
			case eTF_G16R16F:
			case eTF_V16U16:
				return 4;
			case eTF_A8:



			case eTF_L8:
				return 1;
			case eTF_A8L8:
				return 2;
			case eTF_R32F:
				return 4;
			case eTF_DXT3:
			case eTF_DXT5:
			case eTF_3DC:
				return 16;
			case eTF_DXT1:
			case eTF_CTX1:
				return 8;
			case eTF_R16F:
			case eTF_DF16:
				return 2;
			case eTF_DF24:
				return 3;
			case eTF_D16:
				return 2;
			case eTF_D24S8:
			case eTF_D32F:
				return 4;
			case eTF_NULL:
			case eTF_Unknown:
			default:
				assert(0);
				return 0;
		}
		return 0;
	}

	static bool IsDXTCompressed(const ETEX_Format eTF)
	{
		if (eTF == eTF_DXT1 || eTF == eTF_DXT3 || eTF == eTF_DXT5 || eTF == eTF_3DC || eTF == eTF_CTX1)
			return true;
		return false;
	}
	static int TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, const ETEX_Format eTF)
	{
		if (eTF == eTF_Unknown)
			return 0;

		if (nMips <= 0)
			nMips = 1;
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
				int blockSize = (eTF == eTF_DXT1 || eTF == eTF_CTX1) ? 8 : 16;
				nSingleMipSize = ((nWidth+3)/4)*((nHeight+3)/4)*nDepth*blockSize;
			}
			else
				nSingleMipSize = nWidth * nHeight * nDepth * BytesPerBlock(eTF);
			nSize += nSingleMipSize;

			nWidth >>= 1;
			nHeight >>= 1;
			nDepth >>= 1;
			if (nMips == nM)
				break;
		}

		return nSize;
	}
};

static string MakeFileName( const string& filePath, const uint32 nChunk, const uint32 nFlags )
{
	string suffix = "";

	if(nFlags & CTextureSplitter::esrfDDSIsAttachedAlpha)
		suffix = "a";

	string sFullDestFileName;
	sFullDestFileName.Format("%s.%d%s", filePath, nChunk, suffix.c_str());

	return sFullDestFileName;
}

bool CTextureSplitter::SaveFile( const string& sFileName, const void* pBuffer, const size_t nSize, const FILETIME& fileTime )
{
	// create all subdirs
	if(m_pResourceCompiler)
	{
		const string sDir = PathHelpers::AddSeparator(PathHelpers::GetDirectory(sFileName));
		m_pResourceCompiler->EnsureDirectoriesPresent(sDir);
	}

	// create file
	FILE* file = fopen(sFileName, "wb");
	if(!file)
	{
		RCLogError("Error '%s': Failed to create file '%s'\n", strerror(errno), sFileName.c_str());
		return false;
	}

	// save content
	bool bResult = true;
	if(fwrite(pBuffer, nSize, 1, file) != 1)
	{
		bResult = false;
		RCLogError("Error '%s': Failed to write content of file '%s'\n", strerror(errno), sFileName.c_str());
	}

	fclose(file);
	
	if (bResult)
	{
		FileUtil::SetFileTimes(sFileName.c_str(), fileTime);
	}

	return bResult;
}

CTextureSplitter::CTextureSplitter( ) : 
		m_refCount(1),
		m_currentEndian(eLittleEndian),
		m_pCC(NULL)
{
}

CTextureSplitter::~CTextureSplitter( )
{
}

bool CTextureSplitter::BuildDDSChunks(const CImageExtensionHelper::DDS_HEADER* const pDDSHeader, SStreamableResource* pResource )
{
	if (pDDSHeader->dwSize != sizeof(CImageExtensionHelper::DDS_HEADER))
	{
		RCLogError("Error: Unknown DDS file header: '%s'\n", pResource->m_sFileName.c_str());
		return false;
	}

	// get number of real MIPs
	const uint32 nNumMips = pDDSHeader->GetMipCount();

	// get number of sides
	const bool bIsCubemap = (pDDSHeader->dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) != 0 && (pDDSHeader->dwCubemapFlags & DDS_CUBEMAP_ALLFACES) != 0;
	const uint32 nSides = bIsCubemap ? 6 : 1;

	int dataHeaderOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC);

	const size_t nSideSize = TextureHelper::TextureDataSize(pDDSHeader->dwWidth, pDDSHeader->dwHeight, pDDSHeader->dwDepth, nNumMips, TextureHelper::GetTextureFormat(pDDSHeader));

	// if we load alpha channel we need take into account main mips offset
	if((pResource->m_nFileFlags & esrfDDSIsAttachedAlpha) != 0)
		dataHeaderOffset = sizeof(CImageExtensionHelper::DDS_HEADER);		// we don't have DWORD header for attached alpha channel

	// parse chunks
	int nEstimatedMips = (int)nNumMips;

	pResource->m_nFileSize = dataHeaderOffset;

	// header file
	for(int currentPhase = 0;nEstimatedMips > 0;++currentPhase)
	{
		SStreamableResource::SChunkDesc chunkDesc;
		chunkDesc.m_nChunkNumber = currentPhase;

		// calculate mips to save
		uint32 nStartMip;
		uint32 nEndMip;
		if(currentPhase == 0)
		{
			nStartMip = max(0, (int)nNumMips - ehiNumLastMips);
			nEndMip = nNumMips;
		}
		else
		{
			nStartMip = max(0, ((int)nNumMips - ehiNumLastMips) - currentPhase);
			nEndMip = min(nNumMips, nStartMip + 1);
		}

		const uint32 nNumMipsToStore = nEndMip - nStartMip;

		for(int iSide = 0;iSide < nSides;++iSide)
		{
			// add face chunk
			chunkDesc.m_Faces.push_back(SStreamableResource::SChunkDesc::SFace());
			SStreamableResource::SChunkDesc::SFace& face = chunkDesc.m_Faces.back();

			// initialize offset for side
			// add header to the first chunk of the first side
			face.m_nOriginalOffset = iSide * nSideSize + dataHeaderOffset; 

			assert(nNumMipsToStore > 0);

			// whole texture mips to store
			face.m_nOriginalOffset += nStartMip == 0 ? 0 : TextureHelper::TextureDataSize(pDDSHeader->dwWidth, 
																																									pDDSHeader->dwHeight, pDDSHeader->dwDepth, nStartMip, 
																																									TextureHelper::GetTextureFormat(pDDSHeader));

			// calculating mip chunk size for each side
			face.m_nChunkSize = TextureHelper::TextureDataSize(max(1, pDDSHeader->dwWidth >> nStartMip), 
																												max(1, pDDSHeader->dwHeight >> nStartMip), max(1, pDDSHeader->dwDepth >> nStartMip), nNumMipsToStore,
																												TextureHelper::GetTextureFormat(pDDSHeader));

			assert(face.m_nOriginalOffset + face.m_nChunkSize <= nSideSize * nSides + dataHeaderOffset);

			// calculate file size
			pResource->m_nFileSize += face.m_nChunkSize;
		}

		nEstimatedMips -= nNumMipsToStore;
		assert(nNumMips >= 0);

		// add chunk
		pResource->m_chunks.push_back(chunkDesc);
	}

	assert(nSideSize * nSides + dataHeaderOffset == pResource->m_nFileSize);

	// sort all chunks in offset ascending order
	pResource->m_chunks.sort();

#ifdef DEBUG_DISABLE_TEXTURE_SPLITTING
	pResource->m_chunks.clear();
#endif

	return true;
}


void CTextureSplitter::PostLoadProcessTexture( SStreamableResource& resource, std::vector<uint8>& fileContent )
{
	CImageExtensionHelper::DDS_FILE_DESC* pFileDesc = (CImageExtensionHelper::DDS_FILE_DESC*)&fileContent[0];
	const ETEX_Format format = DDSFormats::GetFormatByDesc(pFileDesc->header.ddspf);

	const int nOldMips = pFileDesc->header.GetMipCount();
	int nFaces = 1;
	if ((pFileDesc->header.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) != 0 && (pFileDesc->header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES) != 0)
		nFaces = 6;

	// if we have already texture with proper mips, just return
	if( (pFileDesc->header.dwWidth >> nOldMips) >= 4 && (pFileDesc->header.dwHeight >> nOldMips) >= 4
		|| pFileDesc->header.GetMipCount() == 1)
		return;

	// else calculate # mips to drop and drop it
	int nNumMipsToDrop = 0;
	while(pFileDesc->header.GetMipCount() - nNumMipsToDrop > 1)
	{
		int nNewMips = pFileDesc->header.GetMipCount() - nNumMipsToDrop - 1;
		if((pFileDesc->header.dwWidth >> nNewMips) >= 4 && 
			(pFileDesc->header.dwHeight >> nNewMips) >= 4)
			break;
		nNumMipsToDrop++;
	}

	if(!nNumMipsToDrop)
		return;

	const int nTexDataSize = TextureHelper::TextureDataSize(pFileDesc->header.dwWidth, pFileDesc->header.dwHeight, 
																													max(1ul, pFileDesc->header.dwDepth), 
																													pFileDesc->header.GetMipCount(), format);
	const int nNewTexDataSize = TextureHelper::TextureDataSize(pFileDesc->header.dwWidth, pFileDesc->header.dwHeight, 
																															max(1ul, pFileDesc->header.dwDepth), 
																															pFileDesc->header.GetMipCount() - nNumMipsToDrop, format);
	const int nDiffOffset = nTexDataSize - nNewTexDataSize;
	uint8* pData = &fileContent[sizeof(CImageExtensionHelper::DDS_FILE_DESC)];
	int nEndOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC) + nTexDataSize;
	int nNewEndOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC) + nNewTexDataSize;
	for(int iSide = 1;iSide < nFaces;++iSide)
	{
		uint8* pOldSideData = pData + nTexDataSize * iSide;
		uint8* pNewSideData = pData + nNewTexDataSize * iSide;
		assert(sizeof(CImageExtensionHelper::DDS_FILE_DESC) + nTexDataSize + nDiffOffset + nNewTexDataSize <= fileContent.size());
		memmove(pNewSideData, pOldSideData, nNewTexDataSize);
		nEndOffset += nTexDataSize;
		nNewEndOffset += nNewTexDataSize;
	}

	// copy the rest of the file
	int nRest = fileContent.size() - nEndOffset;
	if(nRest)
	{
		assert(nRest > 0);
		memmove(&fileContent[nNewEndOffset], &fileContent[nEndOffset], nRest);
	}

	fileContent.resize(fileContent.size() - nDiffOffset * nFaces);

	// write to the header
	pFileDesc->header.dwMipMapCount -= nNumMipsToDrop;
}

bool CTextureSplitter::LoadTexture( SStreamableResource& resource, std::vector<uint8>& fileContent )
{
	// init file size
	resource.m_nFileSize = 0;

	// try to open file
	FILE* file = fopen(resource.m_sFileName.c_str(), "rb");
	if(file == NULL)
	{
		RCLogError("Error: Cannot open texture file: '%s'\n", resource.m_sFileName.c_str());
		return false;
	}

	// get file size
	fseek( file,0,SEEK_END );
	const uint32 nOriginalSize = ftell(file);
	fseek( file,0,SEEK_SET );

	// debug helper
#ifdef DEBUG_FILE_SIZE_LIMIT
	if(nOriginalSize > DEBUG_FILE_SIZE_LIMIT)
	{
		fclose(file);
		return false;
	}
#endif

	if(nOriginalSize < sizeof(CImageExtensionHelper::DDS_FILE_DESC))
	{
		RCLogError("Error: Cannot load texture, file too small: '%s'\n", resource.m_sFileName.c_str());
		fclose(file);
		return false;
	}

	// allocate memory for file content
	fileContent.resize(nOriginalSize);

	if(fread(&fileContent[0], nOriginalSize, 1, file) != 1)
	{
		fclose(file);
		return false;
	}

	CImageExtensionHelper::DDS_FILE_DESC* pFileDesc = (CImageExtensionHelper::DDS_FILE_DESC*)&fileContent[0];

	// check for valid CryEngine DDS file
	if (pFileDesc->dwMagic != MAKEFOURCC('D','D','S',' ') || 
		pFileDesc->header.dwSize != sizeof(pFileDesc->header) ||
		pFileDesc->header.ddspf.dwSize != sizeof(pFileDesc->header.ddspf) || 
		pFileDesc->header.dwDepth > 8192 || 
		pFileDesc->header.dwHeight > 8192 ||
		pFileDesc->header.dwWidth > 8192)
	{
		RCLogError("Error: Cannot load texture header: '%s'\n", resource.m_sFileName.c_str());
		fclose(file);
		return false;
	}

	const ETEX_Format format = DDSFormats::GetFormatByDesc(pFileDesc->header.ddspf);
	if(format == eTF_Unknown)
	{
		RCLogError("Error: Cannot load texture(unknown format): '%s'\n", resource.m_sFileName.c_str());
		fclose(file);
		return false;
	}
	int nFaces = 1;
	if ((pFileDesc->header.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) != 0 && (pFileDesc->header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES) != 0)
		nFaces = 6;
	int nTexDataSize = TextureHelper::TextureDataSize(pFileDesc->header.dwWidth, pFileDesc->header.dwHeight, max(1ul, pFileDesc->header.dwDepth), 
																											pFileDesc->header.GetMipCount(), format) * nFaces;
	if(sizeof(CImageExtensionHelper::DDS_FILE_DESC) + nTexDataSize > nOriginalSize)
	{
		RCLogError("Error: Cannot load texture, file content too small: '%s'\n", resource.m_sFileName.c_str());
		fclose(file);
		return false;
	}

	fclose(file);

	PostLoadProcessTexture(resource, fileContent);

	return true;
}

void CTextureSplitter::ParseDDSTexture( SStreamableResource& resource, std::vector<uint8>& fileContent )
{
	if(!LoadTexture(resource, fileContent))
	{
		RCLogError("Error: Cannot load texture: '%s'\n", resource.m_sFileName.c_str());
		fileContent.clear();
		return;
	}

	// process texture platform-specific conversions
	ProcessPlatformSpecificConversions(&fileContent[sizeof(DWORD)], fileContent.size() - sizeof(DWORD));

	static const int dataHeaderOffset = sizeof(DWORD) + sizeof(CImageExtensionHelper::DDS_HEADER);

	// load header
	CImageExtensionHelper::DDS_HEADER* pDDSHeader = (CImageExtensionHelper::DDS_HEADER*)(&fileContent[0] + sizeof(DWORD));

	//COMPILE_TIME_ASSERT(sizeof(CImageExtensionHelper::CImageExtensionHelper::DDS_HEADER) == sizeof(CImageExtensionHelper::DDS_HEADER));
	const uint32 imageFlags = CImageExtensionHelper::GetImageFlags((CImageExtensionHelper::DDS_HEADER *)pDDSHeader);

	// we need to skip 3D textures 
	if(pDDSHeader->dwDepth > 1)
		return;

	// skip splitting if we have an explicit hint for that
	if(m_pCC->config->HasKey("dont_split"))
		return;

	// build main mips
	if(!BuildDDSChunks(pDDSHeader, &resource))
		return;

	// load attached alpha channel
	if(resource.m_nFileSize < fileContent.size())
	{
		// load last portion of file
		// gets file size
		uint8* pAttachedData = &fileContent[resource.m_nFileSize];

		// try to read header of attached alpha channel
		CImageExtensionHelper::DDS_HEADER* pAlphaDDSHeader = (CImageExtensionHelper::DDS_HEADER*)CImageExtensionHelper::GetAttachedImage(&pAttachedData[0]);
		if (pAlphaDDSHeader)
		{
			// force set A8 FORMAT
			pAlphaDDSHeader->ddspf = DDSFormats::DDSPF_A8;

			// set flag
			resource.m_nFileFlags |= esrfDDSHasAdditionalAlpha;

			assert(!resource.m_sFileName.empty());
			assert(m_pAttachedResource);
			assert(m_pAttachedResource->m_sFileName.empty());
			m_pAttachedResource->m_sFileName = resource.m_sFileName;
			m_pAttachedResource->m_nFileSize = 0;
			m_pAttachedResource->m_nFileFlags |= esrfDDSIsAttachedAlpha;

			// build alpha mips
			if(!BuildDDSChunks(pAlphaDDSHeader, m_pAttachedResource))
			{
				RCLogError("Error: Cannot load attached alpha channel of texture header: '%s'\n", resource.m_sFileName.c_str());
				return;
			}
		}
	}

	// validate file size
	//assert(fabsf(nOriginalSize - resource.m_nFileSize) <= 512);
}

void CTextureSplitter::AssembleDDSTextureChunk( const std::vector<uint8>& fileContent, SStreamableResource& resource, 
																							 const uint32 nChunkNumber, std::vector<uint8>* pOutChunk )
{
	int dataHeaderOffset = sizeof(CImageExtensionHelper::DDS_FILE_DESC);

	assert(resource.m_chunks.size() > nChunkNumber);
	assert(pOutChunk);

	// offset and size of chunk
	std::list<SStreamableResource::SChunkDesc>::iterator it = resource.m_chunks.begin();
	for(uint32 i=0;i<resource.m_chunks.size();++i)
	{
		if(it->m_nChunkNumber == nChunkNumber)
		{
			break;
		}
		++it;
	}
	assert(it!= resource.m_chunks.end());
	SStreamableResource::SChunkDesc& currentChunk = *it;

	// allocate space for it
	size_t nTotalSize = 0;
	for(SStreamableResource::SChunkDesc::TFaces::const_iterator it = currentChunk.m_Faces.begin();it != currentChunk.m_Faces.end();++it)
		nTotalSize += it->m_nChunkSize;
	pOutChunk->resize(nTotalSize);

	// check for attached resource
	SStreamableResource::SChunkDesc const* attachedChunk = NULL;
	if( (resource.m_nFileFlags & esrfDDSHasAdditionalAlpha) != 0)
	{
		if(m_pAttachedResource->m_chunks.size() > nChunkNumber)
		{
			std::list<SStreamableResource::SChunkDesc>::const_iterator it = m_pAttachedResource->m_chunks.begin();
			std::advance(it, nChunkNumber);
			attachedChunk = &(*it);
		}
	}

	// get DDS header for compression
	CImageExtensionHelper::DDS_HEADER* pDDSHeader;
	if(!(resource.m_nFileFlags & esrfDDSIsAttachedAlpha))
		pDDSHeader = (CImageExtensionHelper::DDS_HEADER*)(&fileContent[0] + sizeof(DWORD));
	else
	{
		pDDSHeader = CImageExtensionHelper::GetAttachedImage(&fileContent[0]);
		dataHeaderOffset = sizeof(CImageExtensionHelper::DDS_HEADER);
	}

	// header file
	if(nChunkNumber == 0)
	{
		// allocate place for header
		pOutChunk->resize(pOutChunk->size() + dataHeaderOffset);

		// copy header to head of array
		if(!(resource.m_nFileFlags & esrfDDSIsAttachedAlpha))
		{
			memcpy(&(*pOutChunk)[0], &fileContent[0], dataHeaderOffset);
			pDDSHeader = (CImageExtensionHelper::DDS_HEADER*)(&(*pOutChunk)[0] + sizeof(DWORD));
		}
		else
		{
			memcpy(&(*pOutChunk)[0], pDDSHeader, dataHeaderOffset);
			pDDSHeader = (CImageExtensionHelper::DDS_HEADER*)(&(*pOutChunk)[0]);
		}
		// copy mips to array
		size_t destOffset = 0;
		for(SStreamableResource::SChunkDesc::TFaces::const_iterator it = currentChunk.m_Faces.begin();it != currentChunk.m_Faces.end();++it)
		{
			memcpy(&(*pOutChunk)[dataHeaderOffset + destOffset], &fileContent[it->m_nOriginalOffset], it->m_nChunkSize);
			destOffset += it->m_nChunkSize;
		}
		// compress chunk
		//if(0)//m_bSupportsXenonCompression && (resource.m_nFileFlags & (esrfDDSHasAdditionalAlpha|esrfDDSIsAttachedAlpha)) == 0)
		//{
		//	// create storage
		//	uint32 nStartMip = max(0, (int32)pDDSHeader->GetMipCount() - ehiNumLastMips);
		//	uint32 nNumMips = min(pDDSHeader->GetMipCount(), (uint32)ehiNumLastMips);
		//	assert(m_pCC && m_pCC->pRC);
		//	assert(m_pCC->pRC->GetCompressor());
		//	uint32 nSize = currentChunk.m_nChunkSize - dataHeaderOffset;
		//	std::vector<uint8> vecContent(nSize * 2 + 2048);
		//	if(m_pCC->pRC->GetCompressor()->CompressMips(pDDSHeader, nStartMip, 
		//																													nNumMips, &(*pOutChunk)[dataHeaderOffset], nSize,
		//																													vecContent)
		//																													&& vecContent.size() + dataHeaderOffset < pOutChunk->size())
		//	{
		//		pOutChunk->resize(vecContent.size() + dataHeaderOffset);
		//		memcpy(&((*pOutChunk)[dataHeaderOffset]), &vecContent[0], vecContent.size());
		//	}
		//}

		// check for alpha
		//if( (resource.m_nFileFlags & esrfDDSHasAdditionalAlpha) != 0)
		//{
		//	assert(attachedChunk);
		//	const uint32 alphaHeaderOffset = sizeof(CImageExtensionHelper::DDS_HEADER);
		//	// allocate space for it
		//	const uint32 oldOffset = pOutChunk->size();
		//	pOutChunk->resize(pOutChunk->size() + attachedChunk->m_nChunkSize);

		//	const CImageExtensionHelper::DDS_HEADER* pAlphaHeader = (const CImageExtensionHelper::DDS_HEADER*)CImageExtensionHelper::GetAttachedImage(&fileContent[0] + resource.m_nFileSize);
		//	assert(pAlphaHeader);
		//	const char* AttachedFileStart = (const char*)pAlphaHeader + alphaHeaderOffset;

		//	// copy header to the end of array
		//	memcpy(&(*pOutChunk)[oldOffset], pAlphaHeader, alphaHeaderOffset);
		//	// copy mips and to array
		//	memcpy(&(*pOutChunk)[oldOffset + alphaHeaderOffset], 
		//		AttachedFileStart + attachedChunk->m_nOriginalOffset - alphaHeaderOffset, 
		//		attachedChunk->m_nChunkSize - alphaHeaderOffset);

		//	currentChunk.m_nChunkSize += attachedChunk->m_nChunkSize;

		//	// correct file size once!
		//	resource.m_nFileSize += attachedAlpha->m_nFileSize;
		//}
	}
	else	// bigger mips
	{
		// copy mips to head of array
		size_t destOffset = 0;
		for(SStreamableResource::SChunkDesc::TFaces::const_iterator it = currentChunk.m_Faces.begin();it != currentChunk.m_Faces.end();++it)
		{
			memcpy(&(*pOutChunk)[destOffset], &fileContent[it->m_nOriginalOffset], it->m_nChunkSize);
			destOffset += it->m_nChunkSize;
		}

		// compress chunk
		//if(!(resource.m_nFileFlags & (esrfDDSHasAdditionalAlpha|esrfDDSIsAttachedAlpha)))
		//{
		//	// create storage
		//	assert(pDDSHeader->GetMipCount() >= ehiNumLastMips + currentChunk.m_nChunkNumber); 
		//	uint32 nStartMip = pDDSHeader->GetMipCount() - ehiNumLastMips - currentChunk.m_nChunkNumber;
		//	uint32 nNumMips = 1;
		//	assert(m_pCC &&m_pCC->pRC);
		//	assert(m_pCC->pRC->GetCompressor());
		//	uint32 nSize = currentChunk.m_nChunkSize;
		//	std::vector<uint8> vecContent(nSize * 2 + 512);	// just in case
		//	vecContent.resize(nSize * 2 + 512);

		//	if(m_pCC->pRC->GetCompressor()->CompressMips(pDDSHeader, nStartMip, 
		//																														nNumMips, &(*pOutChunk)[0], nSize,
		//																														vecContent) && vecContent.size() < pOutChunk->size())
		//	{
		//		pOutChunk->resize(vecContent.size());
		//		memcpy(&(*pOutChunk)[0], &vecContent[0], vecContent.size());
		//	}
		//}
	}

	// check for alpha
	if(attachedChunk)
	{
		std::vector<byte> vecAttachedAlpha(m_pAttachedResource->m_nFileSize);
		memcpy(&vecAttachedAlpha[0], &fileContent[resource.m_nFileSize], m_pAttachedResource->m_nFileSize);
		ProcessResource(*m_pAttachedResource, vecAttachedAlpha, nChunkNumber);
	}
}

void CTextureSplitter::ProcessResource( SStreamableResource& resource, const std::vector<uint8>& fileContent, const uint32 nChunk )
{
	const string sDestFileName = PathHelpers::GetFilename( MakeFileName(resource.m_sFileName, nChunk, resource.m_nFileFlags) );
	const string sFullDestFileName = PathHelpers::Join( PathHelpers::GetDirectory(m_pCC->getOutputPath()), sDestFileName );

	const FILETIME fileTimeSource = FileUtil::GetLastWriteFileTime( resource.m_sFileName );

	// Compare time stamp of output file.
	if (FileUtil::FileTimeIsValid(fileTimeSource) && (!m_pCC->config->HasKey("refresh")))
	{
		const FILETIME fileTimeTarget = FileUtil::GetLastWriteFileTime( sFullDestFileName );

		if (FileUtil::FileTimesAreEqual(fileTimeSource, fileTimeTarget))
		{
			if (m_pCC->config->HasKey("verbose"))
			{
				RCLog("Skipping %s: destination file %s is up to date", resource.m_sFileName.c_str(), sFullDestFileName.c_str());
			}
			return;
		}
	}

	// get file chunk
	std::vector<uint8> vecChunkToWrite;
	AssembleDDSTextureChunk(fileContent, resource, nChunk, &vecChunkToWrite);

	assert(!vecChunkToWrite.empty());

	// adds file to pak
	if(!SaveFile(sFullDestFileName, &vecChunkToWrite[0], vecChunkToWrite.size(), fileTimeSource ) )
		RCLogError("Error: Cannot save file '%s'\n", resource.m_sFileName.c_str());
}

void CTextureSplitter::PrintStatistics()
{
	if(m_pResourceCompiler)
	{
		RCLog("Compression statistics:\n");
		const float MCTRatio = m_pResourceCompiler->GetCompressor()->GetStatistics().GetAvgRatio<ectMCT>();
		const float PTCRatio = m_pResourceCompiler->GetCompressor()->GetStatistics().GetAvgRatio<ectPTC>();
		const float JPGRatio = m_pResourceCompiler->GetCompressor()->GetStatistics().GetAvgRatio<ectJPEG>();
		RCLog("    Average PTC compression ratio: %.2f\n", PTCRatio);
		RCLog("    Average MCT compression ratio: %.2f\n", MCTRatio);
		RCLog("    Average JPG compression ratio: %.2f\n", JPGRatio);
	}
}

void CTextureSplitter::AddResourceToAdditionalList( SStreamableResource& resource, const std::vector<uint8>& fileContent )
{
	const string sDestFileName = PathHelpers::GetFilename( resource.m_sFileName );
	const string sFullDestFileName = PathHelpers::Join( PathHelpers::GetDirectory(m_pCC->getOutputPath()), sDestFileName );

	const FILETIME fileTimeSource = FileUtil::GetLastWriteFileTime( resource.m_sFileName );

	// Compare time stamp of output file.
	if (FileUtil::FileTimeIsValid(fileTimeSource) && (!m_pCC->config->HasKey("refresh")))
	{
		const FILETIME fileTimeTarget = FileUtil::GetLastWriteFileTime( sFullDestFileName );

		if (FileUtil::FileTimesAreEqual(fileTimeSource, fileTimeTarget))
		{
			if (m_pCC->config->HasKey("verbose"))
			{
				RCLog("Skipping %s: destination file %s is up to date", resource.m_sFileName.c_str(), sFullDestFileName.c_str());
			}
			return;
		}
	}

	const uint32 nDDSHeaderSize = sizeof(DWORD) + sizeof(CImageExtensionHelper::DDS_HEADER);

	byte const* pFileHeader = &fileContent[0];
	CImageExtensionHelper::DDS_HEADER* pDDSHeader = (CImageExtensionHelper::DDS_HEADER*)(&fileContent[0] + sizeof(DWORD));
	byte const* pData = &fileContent[0] + nDDSHeaderSize;
	const uint32 nTextureSize = TextureHelper::TextureDataSize(
		pDDSHeader->dwWidth, 
		pDDSHeader->dwHeight, pDDSHeader->dwDepth, pDDSHeader->GetMipCount(),
		TextureHelper::GetTextureFormat(pDDSHeader));

	assert(nTextureSize <= fileContent.size() - nDDSHeaderSize);

	// add resource to list of non-streamable resources
	std::vector<byte> finalContent;
	{
		finalContent.resize(nTextureSize * 2 + nDDSHeaderSize);

		bool ableToBeCompressed = false;//true;
		if ((pDDSHeader->dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) && (pDDSHeader->dwCubemapFlags & DDS_CUBEMAP_ALLFACES))
			ableToBeCompressed = false;
		if (pDDSHeader->dwDepth > 1)
			ableToBeCompressed = false;

		// check if we have attached alpha channel
		CImageExtensionHelper::DDS_HEADER* pAlphaDDSHeader = NULL;
		if(fileContent.size() > nDDSHeaderSize+nTextureSize)
			pAlphaDDSHeader = (CImageExtensionHelper::DDS_HEADER*)CImageExtensionHelper::GetAttachedImage(&fileContent[nDDSHeaderSize+nTextureSize]);
		if(pAlphaDDSHeader)
			ableToBeCompressed = false;

		if(ableToBeCompressed 
			&& m_pCC->pRC->GetCompressor()->CompressMips(pDDSHeader, 0, pDDSHeader->GetMipCount(), pData, nTextureSize, finalContent)
			&& finalContent.size() + nDDSHeaderSize < fileContent.size())
		{
			pDDSHeader->dwReserved1[0] |= CImageExtensionHelper::EIF_Compressed;
			size_t oldSize = finalContent.size();
			finalContent.resize(oldSize + nDDSHeaderSize);
			memmove(&finalContent[nDDSHeaderSize], &finalContent[0], oldSize);
			memcpy(&finalContent[0], pFileHeader, nDDSHeaderSize);
		}
		else
		{
			finalContent.resize(fileContent.size());
			memcpy(&finalContent[0], &fileContent[0], fileContent.size());
		}
		const bool res = SaveFile(sFullDestFileName, &finalContent[0], finalContent.size(), fileTimeSource);
		assert(res);
		RCLog("Resource added to additional pak: '%s'\n", resource.m_sFileName.c_str());
	}
}

void CTextureSplitter::Init( IConfig* config, const char* exePath )
{
}

void CTextureSplitter::DeInit( )
{
	// output statistics
	PrintStatistics();

	m_pResourceCompiler = NULL;
}

void CTextureSplitter::Release( )
{
	if (InterlockedDecrement(&m_refCount) == 0)
		delete this;
}

bool CTextureSplitter::Process( ConvertContext &cc )
{
	cc.pRC->GetCompressor()->SetPlatform(cc.platform);

	m_pCC = &cc;

	m_pResourceCompiler = cc.pRC;

	if(cc.platform == ePlatform_X360 || cc.platform == ePlatform_PS3)
		m_currentEndian = eBigEndian;
	else
		m_currentEndian = eLittleEndian;

	// Get the files to process.
	const string sInputFile = cc.getSourcePath();

	{
		SStreamableResource attachedResource;
		m_pAttachedResource = &attachedResource;

		std::vector<uint8> fileContent;

		SStreamableResource resource;
		resource.m_sFileName = sInputFile;

		// load texture parse dds chunks
		ParseDDSTexture(resource, fileContent);

		if (!fileContent.empty())
		{
			if(resource.m_chunks.size())
			{
				// write out dds chunks
				for(int32 nChunk = 0;nChunk < resource.m_chunks.size();++nChunk)
				{
					// split texture's MIPs regarding to current chunk
					// save it to separate file
					ProcessResource(resource, fileContent, nChunk);
				}

				RCLog("Added file '%s'\n", resource.m_sFileName.c_str());
			}
			else
			{
				// if failed then this resource is not for streaming
				// add it to additional pak
				AddResourceToAdditionalList(resource, fileContent);
			}
		}
		m_pAttachedResource = 0;
	}

	RCLog("Processing file '%s' finished.\n", sInputFile.c_str());

	m_pCC = NULL;

	return true;
}

void CTextureSplitter::ConstructAndSetOutputFile( ConvertContext &cc )
{
	string sExtension = PathHelpers::FindExtension(cc.sourceFileFinal);
	sExtension = "bin" + sExtension;
	cc.SetOutputFile( PathHelpers::ReplaceExtension(cc.sourceFileFinal, sExtension) );
}

void CTextureSplitter::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const filename(cc.getOutputPath());

	if (filenameBuffer && (filename.length() < bufferSize))
	{
		strcpy(filenameBuffer, filename.c_str());		
	}
}

int CTextureSplitter::GetNumPlatforms() const
{
	return 3;
}

EPlatform CTextureSplitter::GetPlatform( int index ) const
{
	switch (index)
	{
	case 0:
		return ePlatform_PC;
	case 1:
		return ePlatform_X360;
	case 2:
		return ePlatform_PS3;
	default:
		assert(0);
		return ePlatform_UNKNOWN;
	}
}

int CTextureSplitter::GetNumExt( ) const
{
	return sizeof(s_sInputExtensions) / sizeof(s_sInputExtensions[0]);
}

const char* CTextureSplitter::GetExt( int index ) const
{
	return s_sInputExtensions[index].c_str();
}

DWORD CTextureSplitter::GetTimestamp( ) const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}

ICompiler* CTextureSplitter::CreateCompiler( )
{
	if(m_refCount == 1)
	{
		++m_refCount;
		return this;
	}
	CTextureSplitter* pNewSplitter = new CTextureSplitter();
	return pNewSplitter;
}

bool CTextureSplitter::SupportsMultithreading( ) const
{
	return true;
}

void CTextureSplitter::ProcessPlatformSpecificConversions( byte* fileContent, const size_t fileSize )
{
	assert(fileContent);

	CImageExtensionHelper::DDS_HEADER& header = (CImageExtensionHelper::DDS_HEADER&)*fileContent;
	if(header.dwTextureStage != 'CRYF')
	{
		header.dwReserved1[0] = 0;
		header.dwTextureStage = 'CRYF';
	}

	byte* pDataHead = fileContent + sizeof(CImageExtensionHelper::DDS_HEADER);

	DWORD& imageFlags = header.dwReserved1[0];

	if(m_pCC->platform == ePlatform_PC)		// there is no conversions for PC platform
		return;

	// check if this texture already native-converted
	switch (m_pCC->platform)
	{
	case ePlatform_X360:
		if(imageFlags & CImageExtensionHelper::EIF_XBox360Native) return;
		break;
	case ePlatform_PS3:
		if(imageFlags & CImageExtensionHelper::EIF_PS3Native) return;
		break;
	}

	// we need to process endians only if it's necessary
	EEndian currentEndian = eLittleEndian;
	if(m_pCC->platform == ePlatform_X360)
		currentEndian = eBigEndian;

	// mark this texture as native for current platform
	switch (m_pCC->platform)
	{
	case ePlatform_X360:
		imageFlags |= CImageExtensionHelper::EIF_XBox360Native;
		break;
	case ePlatform_PS3:
		imageFlags |= CImageExtensionHelper::EIF_PS3Native;
		break;
	}
	
	// get pixel format of texture
	const CImageExtensionHelper::DDS_PIXELFORMAT& pixelFormat = header.ddspf;

	// get dimensions
	uint32 dwWidth,dwHeight,dwDepth,dwSides,dwMips;
	dwWidth = header.dwWidth;
	dwHeight = header.dwHeight;
	dwDepth = max(1ul, header.dwDepth);
	dwMips = header.GetMipCount();
	const bool cubemap = (header.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) != 0 && (header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES) != 0;
	dwSides = cubemap ? 6 : 1;
	const ETEX_Format format = DDSFormats::GetFormatByDesc(header.ddspf);
	const bool bDXTCompressed = TextureHelper::IsDXTCompressed(format);
	const uint32 nBitsPerPixel = bDXTCompressed ? (TextureHelper::BytesPerBlock(format) / 2) : (TextureHelper::BytesPerBlock(format) * 8);
	// all formats that don't need endian swapping/byte rearrangement should be here
	const bool bNeedEndianSwapping = (format != eTF_CTX1);

	// process mips
	uint32 nDataOffset = 0;
	for(uint32 dwSide=0;dwSide<dwSides;++dwSide)
	{
		for(uint32 dwMip=0;dwMip<dwMips;++dwMip)
		{
			// calc mip's size
			const uint32 dwLocalWidth = max(1u, dwWidth >> dwMip);
			const uint32 dwLocalHeight = max(1u, dwHeight >> dwMip);
			const uint32 dwLocalDepth = max(1u, dwDepth >> dwMip);
			const uint32 dwBlockWidth = bDXTCompressed ? (dwLocalWidth+3)/4 : dwLocalWidth;
			const uint32 dwBlockHeight = bDXTCompressed ? (dwLocalHeight+3)/4 : dwLocalHeight;
			const uint32 dwPitch = TextureHelper::TextureDataSize(dwLocalWidth, 1, 1, 1, format);
			const uint32 dwMipSize = TextureHelper::TextureDataSize(dwLocalWidth, dwLocalHeight, dwLocalDepth, 1, format);

			byte* pMem=0;

			// get offsetted mip chunk
			byte* pStartMem = pDataHead + nDataOffset;

			pMem = pStartMem;

			for(uint32 d=0;d<dwDepth;++d)
			{
				for(uint32 v=0;v<dwBlockHeight;++v)	// for each line of blocks
				{
					if(bNeedEndianSwapping)
					{
						// swizzling and endians swapping for specific platforms
						if(bDXTCompressed)
						{
							SwapEndian((uint16*)pMem, dwPitch / sizeof(uint16), currentEndian);
						}
						else if(format == eTF_G16R16F)
						{
							if(m_pCC->platform == ePlatform_X360)
							{
								for(uint32 i=0;i<dwLocalWidth;++i)
								{
									std::swap(pMem[i*4+0], pMem[i*4+3]);
									std::swap(pMem[i*4+1], pMem[i*4+2]);
								}
							}
							else if(m_pCC->platform == ePlatform_PS3)
								SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), eBigEndian);
							else
							{
								SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), currentEndian);
							}
						}
						else if(format == eTF_A8R8G8B8)
						{
							SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), currentEndian);
						}
						else if(format == eTF_A4R4G4B4)
						{
							SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), currentEndian);
						}
						else if(format == eTF_X8R8G8B8)
						{
							SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), currentEndian);
						}
						else if(format == eTF_R8G8B8)
						{
							assert(nBitsPerPixel == 24);
							if((imageFlags & DDS_RESF1_DSDT) == 0)
							{
								if(m_pCC->platform == ePlatform_X360 || m_pCC->platform == ePlatform_PS3)
								{
									for(uint32 i=0;i<dwLocalWidth;++i)
										std::swap(pMem[i*3], pMem[i*3+2]);
								}
								SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), currentEndian);
							}
							else	// DUDV(L8V8U8)
							{
								for(uint32 i=0;i<dwLocalWidth;++i)
									std::swap(pMem[i*3], pMem[i*3+2]);
							}
						}
						else
						{
							if(format == eTF_DXT1)
								SwapEndian((uint16*)pMem, dwPitch / sizeof(uint16), currentEndian);
							else 
							{
								if(nBitsPerPixel == 32 || bDXTCompressed)
									SwapEndian((uint32*)pMem, dwPitch / sizeof(uint32), currentEndian);
								else if(nBitsPerPixel == 16)
									SwapEndian((uint16*)pMem, dwPitch / sizeof(uint16), currentEndian);
							}
						}
					}

					pMem += dwPitch;
				}

				// tile texture level for XBox360
				if(m_pCC->platform == ePlatform_X360 && !(imageFlags & CImageExtensionHelper::EIF_X360NotPretiled))
				{
					const uint32 nBlockSize = bDXTCompressed ? (4*4*nBitsPerPixel/8) : nBitsPerPixel / 8;
					const bool isBlockSizePowOfTwo = (nBlockSize & (nBlockSize - 1)) == 0;
					const bool isPitchGPUFriendly = (dwPitch % (32 * nBlockSize)) == 0;
					// tile only levels that more that 64 in both width and height
					// otherwise we will have huge disk space waste due to GPU alignment
					if(dwLocalWidth > 64 && dwLocalHeight > 64 && isBlockSizePowOfTwo && isPitchGPUFriendly)	
					{
						const uint32 nBlockWidth = bDXTCompressed ? (dwLocalWidth + 3) / 4 : dwLocalWidth;
						const uint32 nBlockHeight = bDXTCompressed ? (dwLocalHeight + 3) / 4 : dwLocalHeight;

						if(!m_pCC->pRC->GetCompressor()->TileTextureLevelForXBox((byte*)pStartMem, (byte*)pStartMem, 
																																			nBlockWidth, nBlockHeight, format, dwPitch, nBlockSize ))
							imageFlags |= CImageExtensionHelper::EIF_X360NotPretiled;
					}
				}
			}
			nDataOffset += dwMipSize;
		}
	}

	// apply to attached images recursively
	if(nDataOffset + sizeof(CImageExtensionHelper::DDS_HEADER) < fileSize)
	{
		uint32 size;
		byte* pAttachedImage = (byte*)CImageExtensionHelper::GetAttachedImage(pDataHead + nDataOffset, &size);
		if(pAttachedImage)
			ProcessPlatformSpecificConversions(pAttachedImage, size);
	}
}
