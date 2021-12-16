////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   ReadOnlyChunkFile.h
//  Version:     v1.00
//  Created:     15/11/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ReadOnlyChunkFile.h"

#define MAX_CHUNKS_NUM 10000

// enable loading profiler if compiled as part of engine
#if defined(FUNCTION_PROFILER_3DENGINE)
  #define LOADING_TIME_PROFILE_SECTION_CGF LOADING_TIME_PROFILE_SECTION(gEnv->pSystem)
#else
  #define LOADING_TIME_PROFILE_SECTION_CGF
#endif

inline bool ChunkCompareFileOffset( const CReadOnlyChunkFile::ChunkDesc &d1,const CReadOnlyChunkFile::ChunkDesc &d2 )
{
	return d1.hdr.FileOffset < d2.hdr.FileOffset;
}
inline bool ChunkCompareId( const CReadOnlyChunkFile::ChunkDesc &d1,const CReadOnlyChunkFile::ChunkDesc &d2 )
{
	return d1.hdr.ChunkID < d2.hdr.ChunkID;
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::CReadOnlyChunkFile( bool bCopyFileData,bool bNoWarningMode )
{
	m_pFileBuffer = 0;
	m_nBufferSize = 0;

	m_fileHeader.FileType				= FileType_Geom;
	m_fileHeader.ChunkTableOffset		= -1;
	m_fileHeader.Version				= ChunkFileVersion;
	strcpy(m_fileHeader.Signature,FILE_SIGNATURE);

	m_bNoWarningMode = bNoWarningMode;

	m_bOwnFileBuffer = false;
	m_bLoaded = false;
	m_bCopyFileData = bCopyFileData;

	m_hFile = 0;
}

CReadOnlyChunkFile::~CReadOnlyChunkFile()
{
	FreeBuffer();

	if (m_hFile)
		gEnv->pCryPak->FClose(m_hFile);
	m_hFile = 0;
}

//////////////////////////////////////////////////////////////////////////
void CReadOnlyChunkFile::FreeBuffer()
{
	if (m_pFileBuffer && m_bOwnFileBuffer)
		delete []m_pFileBuffer;
	m_pFileBuffer = 0;
	m_nBufferSize = 0;
	m_bOwnFileBuffer = false;
}

// retrieves the raw chunk header, as it appears in the file
const CReadOnlyChunkFile::ChunkHeader& CReadOnlyChunkFile::GetChunkHeader(int nChunkIdx)const
{
	return m_chunks[nChunkIdx].hdr;
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::GetChunk( int nIndex )
{
	assert( nIndex >= 0 && nIndex < (int)m_chunks.size() );
	return &m_chunks[nIndex];
}

// returns the raw data of the i-th chunk
const void* CReadOnlyChunkFile::GetChunkData(int nChunkIdx)const
{
	assert (nChunkIdx >= 0 && nChunkIdx < NumChunks());
	if (nChunkIdx>= 0 && nChunkIdx < NumChunks())
	{
		return m_chunks[nChunkIdx].data;
	}
	else
		return 0;
}

// number of chunks
int CReadOnlyChunkFile::NumChunks()const
{
	return (int)m_chunks.size();
}

// number of chunks of the specified type
int CReadOnlyChunkFile::NumChunksOfType (ChunkTypes nChunkType)const
{
	int nResult = 0;
	for (int i = 0; i < NumChunks(); ++i)
	{
		if (m_chunks[i].hdr.ChunkType == nChunkType)
			++nResult;
	}
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// calculates the chunk size, based on the very next chunk with greater offset
// or the end of the raw data portion of the file
int CReadOnlyChunkFile::GetChunkSize(int nChunkIdx) const
{
	assert (nChunkIdx >= 0 && nChunkIdx < NumChunks());
	return m_chunks[nChunkIdx].size;
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::FindChunkByType( int type )
{
	int nchunks = m_chunks.size();
	if (nchunks > 0)
	{
		ChunkDesc *chunks = &m_chunks[0];
		for (int i = 0; i < nchunks; i++)
		{
			if (chunks[i].hdr.ChunkType == type)
			{
				return &chunks[i];
			}
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
CReadOnlyChunkFile::ChunkDesc* CReadOnlyChunkFile::FindChunkById( int id )
{
	ChunkDesc chunkToFind;
	chunkToFind.hdr.ChunkID = id;
	std::vector<ChunkDesc>::iterator it = stl::binary_find( m_chunks.begin(),m_chunks.end(),chunkToFind );
	if (it != m_chunks.end())
	{
		ChunkDesc* pChunk = &*it;
		return pChunk;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CReadOnlyChunkFile::ReadChunkTableFromBuffer()
{
	LOADING_TIME_PROFILE_SECTION_CGF;

	if (m_pFileBuffer == 0 || m_nBufferSize <= 0)
	{
		return false;
	}

	int i;

	memcpy(&m_fileHeader, m_pFileBuffer, sizeof(m_fileHeader));
	SwapEndian( m_fileHeader );

	int n_chunks = *(int*)(m_pFileBuffer + m_fileHeader.ChunkTableOffset);
	SwapEndian(n_chunks);
	if(n_chunks < 0 || n_chunks > MAX_CHUNKS_NUM)
	{
		return false;
	}

	m_chunks.clear();
	m_chunks.resize(n_chunks);

	if (m_fileHeader.Version == ChunkFileVersion_Align)
	{
		CHUNK_TABLE_ENTRY_0745 *chunks = (CHUNK_TABLE_ENTRY_0745*)(m_pFileBuffer + m_fileHeader.ChunkTableOffset + 4);
		SwapEndian( chunks,n_chunks );

		for (i = 0; i < n_chunks; ++i)
		{
			ChunkDesc &cd = m_chunks[i];
			cd.hdr = chunks[i].chdr;
			cd.size = chunks[i].ChunkSize;
			cd.data = m_pFileBuffer + cd.hdr.FileOffset;
			cd.bSwapEndian = (cd.hdr.ChunkVersion & CONSOLE_VERSION_MASK) ? eBigEndian : eLittleEndian;
			cd.hdr.ChunkVersion &= ~CONSOLE_VERSION_MASK;
		}
	}
	else
	{
		CHUNK_HEADER *chunks = (CHUNK_HEADER*)(m_pFileBuffer + m_fileHeader.ChunkTableOffset + 4);
		SwapEndian( chunks,n_chunks );

		for (i = 0; i < n_chunks; ++i)
		{
			ChunkDesc &cd = m_chunks[i];
			cd.hdr = chunks[i];
			cd.bSwapEndian = (cd.hdr.ChunkVersion & CONSOLE_VERSION_MASK) ? eBigEndian : eLittleEndian;
			cd.hdr.ChunkVersion &= ~CONSOLE_VERSION_MASK;
		}

		std::sort( m_chunks.begin(),m_chunks.end(),ChunkCompareFileOffset );

		for (i = 0; i < n_chunks; ++i)
		{
			const int nextFileOffset = (i+1 < n_chunks)
				? m_chunks[i+1].hdr.FileOffset
				: m_nBufferSize;

			ChunkDesc &cd = m_chunks[i];

			cd.size = nextFileOffset - cd.hdr.FileOffset;
			cd.data = m_pFileBuffer + cd.hdr.FileOffset;
		}
	}

	// Sort chunks by Id, for faster queries later (see FindChunkById()).
	std::sort( m_chunks.begin(),m_chunks.end(),ChunkCompareId );

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CReadOnlyChunkFile::Read( const char *filename )
{
	LOADING_TIME_PROFILE_SECTION_CGF;

	if (m_hFile)
		gEnv->pCryPak->FClose(m_hFile);
	m_hFile = 0;
	FreeBuffer();

	int nOpenFlags = 0;
	if (m_bNoWarningMode)
		nOpenFlags |= ICryPak::FOPEN_HINT_QUIET;

	m_hFile = gEnv->pCryPak->FOpen(filename,"rb",nOpenFlags );
	if (!m_hFile)
		return false;

	size_t nFileSize = 0;

	if (m_bCopyFileData)
	{
		nFileSize = gEnv->pCryPak->FGetSize(m_hFile);
		m_pFileBuffer = new char[nFileSize];
		m_bOwnFileBuffer = true;
		if (gEnv->pCryPak->FReadRawAll(m_pFileBuffer,nFileSize,m_hFile) != nFileSize)
		{
			return false;
		}
	}
	else
	{
		m_pFileBuffer = (char*)gEnv->pCryPak->FGetCachedFileData(m_hFile,nFileSize);
		m_bOwnFileBuffer = false;
	}

	if (!m_pFileBuffer)
		return false;

	m_nBufferSize = nFileSize;

	if (!ReadChunkTableFromBuffer())
		return false;

	m_bLoaded = true;

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CReadOnlyChunkFile::ReadFromMemBlock( const void * pData, int nDataSize )
{
	if (m_hFile)
		gEnv->pCryPak->FClose(m_hFile);
	m_hFile = 0;
	FreeBuffer();

	m_pFileBuffer = (char*)pData;
	m_bOwnFileBuffer = false;
	m_nBufferSize = nDataSize;

	if (!ReadChunkTableFromBuffer())
	{
		return false;
	}
	m_bLoaded = true;
	return true;
}
