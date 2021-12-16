/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: TextureSplitter.h,v 1.0 2008/01/17 15:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for creation of streaming pak
-------------------------------------------------------------------------
History:
- 17:1:2008   10:31 : Created by Anton Kaplanyan
*************************************************************************/

#pragma once

#ifndef __TEXTURE_SPLITTER_H__
#define __TEXTURE_SPLITTER_H__

#ifndef MAKEFOURCC
#define NEED_UNDEF_MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
				((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |       \
				((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

#include "IConvertor.h"
#include "../../ResourceCompiler/ConvertContext.h"
#include <list>
#include <set>
#include <ImageExtensionHelper.h>

// creates pak file from file with list of resources
class CTextureSplitter : public IConvertor, public ICompiler
{
protected:
	struct SStreamableResource;
	
	friend string MakeFileName( const string& filePath, const uint32 nChunk, const uint32 nFlags );
protected:
	// textures work
	void ParseDDSTexture(SStreamableResource& resource, std::vector<uint8>& fileContent);
	bool BuildDDSChunks(const CImageExtensionHelper::DDS_HEADER* const pDDSHeader, SStreamableResource* pResource);
	void AssembleDDSTextureChunk( const std::vector<uint8>& fileContent, SStreamableResource& resource, const uint32 nChunkNumber, std::vector<uint8>* pOutChunk );

	void AddResourceToAdditionalList( SStreamableResource& resource, const std::vector<uint8>& fileContent );

	// load texture content
	bool LoadTexture( SStreamableResource& resource, std::vector<uint8>& fileContent );

	// postprocessing after loading(clamping mips etc.)
	void PostLoadProcessTexture(SStreamableResource& resource, std::vector<uint8>& fileContent);

	// Arguments:
	//   fileContent - input image, must not be NULL
	// applied to attached images recursively after processing
	// necessary to correctly swap all endians according to the current platform 
	void ProcessPlatformSpecificConversions( byte* fileContent, const size_t fileSize );

	// process single texture
	void ProcessResource( SStreamableResource& resource, const std::vector<uint8>& fileContent, const uint32 nChunk );

	// file IO framework
	bool SaveFile(const string& sFileName, const void* pBuffer, const size_t nSize, const FILETIME& fileTime);

	// show compression statistics
	void PrintStatistics();

public:
	CTextureSplitter();
	~CTextureSplitter();

	virtual void Release();

	virtual bool Process(ConvertContext &cc);
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	virtual void Init(IConfig* config, const char* exePath);
	virtual void DeInit();
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual int GetNumPlatforms() const;
	virtual EPlatform GetPlatform( int index ) const;
	virtual int GetNumExt() const;
	virtual const char* GetExt( int index ) const;
	virtual DWORD GetTimestamp() const;
	virtual IConvertContext* CreateConvertContext() const { return new ConvertContext; }

protected:

	// settings to store textures for streaming
	enum EHeaderInfo
	{
		ehiNumLastMips = 3,
	};

	// flags for different resource purposes
	enum EStreamableResourceFlags
	{
		esrfDDSHasAdditionalAlpha = 1<<0,			// indicates DDS texture has attached alpha A8 texture
		esrfDDSIsAttachedAlpha = 1<<1,				// indicates DDS texture is attached alpha A8 texture
	};

	// structure to store mapping information about interleaved resource 
	struct SStreamableResource
	{
		// description of single resource chunk
		struct SChunkDesc
		{
			uint8			m_nChunkNumber;														// ordered number of chunk
			struct SFace
			{
				uint32		m_nOriginalOffset;												// offset in file
				uint32		m_nChunkSize;															// size of chunk
				SFace() :	m_nOriginalOffset(0), m_nChunkSize(0) { }
			};
			typedef std::vector<SFace>	TFaces;
			TFaces		m_Faces;												// there are 6 of them in case of cubemap, otherwise it's always one
			uint8			m_nFlags;																	// flags for different chunk purposes
			bool operator <(const SChunkDesc& cd) const { return m_Faces.begin()->m_nOriginalOffset > cd.m_Faces.begin()->m_nOriginalOffset; }	// decreasing by offset
			bool operator ==(const SChunkDesc& cd) const { return m_Faces.begin()->m_nOriginalOffset == cd.m_Faces.begin()->m_nOriginalOffset; }	// decreasing by offset

			SChunkDesc() : m_nChunkNumber(0), m_nFlags(0) { }
		};

		string										m_sFileName;			// full path to the resource
		uint32										m_nFileSize;			// size of source resource(in bytes)
		uint8											m_nFileFlags;			// flags for different resource purposes
		std::list<SChunkDesc>			m_chunks;					// all file chunks

		SStreamableResource() : m_nFileSize(0), m_nFileFlags(0) {}

		// ordering operators
		bool operator<(const SStreamableResource& anotherResource) const { return m_sFileName.compare(anotherResource.m_sFileName) < 0; }
		bool operator==(const SStreamableResource& anotherResource) const { return m_sFileName.compare(anotherResource.m_sFileName) == 0; }
	};

protected:
	static const string s_sInputExtensions[];
	volatile long m_refCount;

	SStreamableResource* m_pAttachedResource;                   // resource may have one attached resource

	EEndian m_currentEndian;

	struct ConvertContext* m_pCC;                               // for different reasons
	struct IResourceCompiler* m_pResourceCompiler;              // for different reasons
};

#ifdef NEED_UNDEF_MAKEFOURCC
	#undef MAKEFOURCC
#endif // NEED_UNDEF_MAKEFOURCC

#endif // __TEXTURE_SPLITTER_H__