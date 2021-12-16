////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   ChunkFile.h
//  Version:     v1.00
//  Created:     15/11/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ChunkFile.h"

#define MAX_CHUNKS_NUM 10000

#if !defined(FUNCTION_PROFILER_3DENGINE)
  #define FUNCTION_PROFILER_3DENGINE
#endif

#if !defined(LOADING_TIME_PROFILE_SECTION)
  #define LOADING_TIME_PROFILE_SECTION
#endif

inline bool ChunkLess( CChunkFile::ChunkDesc *d1,CChunkFile::ChunkDesc *d2 )
{
	return d1->hdr.FileOffset < d2->hdr.FileOffset;
}


CChunkFile::CChunkFile()
{
	m_fileHeader.FileType				= FileType_Geom;
	m_fileHeader.ChunkTableOffset		= -1;
	m_fileHeader.Version				= ChunkFileVersion;
	strcpy(m_fileHeader.Signature,FILE_SIGNATURE);

	m_nLastChunkId = 0;
	m_pInternalData = NULL;
	m_bLoaded = false;
}

CChunkFile::~CChunkFile()
{
	ReleaseChunks();
	if (m_pInternalData)
	{
		free(m_pInternalData);
		m_pInternalData = 0;
	}
}

// retrieves the raw chunk header, as it appears in the file
const CChunkFile::ChunkHeader& CChunkFile::GetChunkHeader(int nChunkIdx) const
{
	return m_chunks[nChunkIdx]->hdr;
}

//////////////////////////////////////////////////////////////////////////
CChunkFile::ChunkDesc* CChunkFile::GetChunk(int nChunkIdx)
{
	assert( nChunkIdx >= 0 && nChunkIdx < (int)m_chunks.size() );
	return m_chunks[nChunkIdx];
}

// returns the raw data of the i-th chunk
const void* CChunkFile::GetChunkData(int nChunkIdx) const
{
	assert (nChunkIdx >= 0 && nChunkIdx < NumChunks());
	if (nChunkIdx>= 0 && nChunkIdx < NumChunks())
	{
		return m_chunks[nChunkIdx]->data;
	}
	return 0;
}

// number of chunks
int CChunkFile::NumChunks() const
{
	return (int)m_chunks.size();
}

// number of chunks of the specified type
int CChunkFile::NumChunksOfType(ChunkTypes nChunkType) const
{
	int nResult = 0;
	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		if (m_chunks[i]->hdr.ChunkType == nChunkType)
		{
			++nResult;
		}
	}
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
int CChunkFile::GetChunkSize(int nChunkIdx) const
{
	assert (nChunkIdx >= 0 && nChunkIdx < NumChunks());
	return m_chunks[nChunkIdx]->size;
}

//////////////////////////////////////////////////////////////////////////
int CChunkFile::AddChunk( const CHUNK_HEADER &hdr,void *chunkData,int chunkSize )
{
	ChunkDesc* const pChunk = new ChunkDesc;
	pChunk->hdr = hdr;
	pChunk->data = new char[chunkSize];
	pChunk->size = chunkSize;
	memcpy( pChunk->data,chunkData,chunkSize );

	const int chunkID = ++m_nLastChunkId;
	pChunk->hdr.ChunkID = chunkID;
	m_chunks.push_back( pChunk );
	m_chunkIdMap[chunkID] = pChunk;
	return chunkID;
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::SetChunkData( int nChunkId,void *chunkData,int chunkSize )
{
	ChunkDesc* const pChunk = FindChunkById(nChunkId);
	if (pChunk->data)
	{
		delete [] (char *)pChunk->data;
	}
	pChunk->data = new char[chunkSize];
	pChunk->size = chunkSize;
	memcpy( pChunk->data,chunkData,chunkSize );
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::DeleteChunkId(int nChunkId)
{
	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		if (m_chunks[i]->hdr.ChunkID == nChunkId) 
		{
			if (m_chunks[i]->data)
			{
				delete [] (char *)m_chunks[i]->data;
			}
			delete m_chunks[i];
			m_chunks.erase(m_chunks.begin() + i);
			m_chunkIdMap.erase(nChunkId);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::ReleaseChunks()
{
	m_bLoaded = false;
	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		if (m_chunks[i]->data)
		{
			delete [](char *) m_chunks[i]->data;
		}
		delete m_chunks[i];
	}
	m_chunks.clear();
	m_chunkIdMap.clear();
}

//////////////////////////////////////////////////////////////////////////
CChunkFile::ChunkDesc* CChunkFile::FindChunkByType( ChunkTypes nChunkType )
{
	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		if (m_chunks[i]->hdr.ChunkType == nChunkType) 
		{
			return m_chunks[i];
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
CChunkFile::ChunkDesc* CChunkFile::FindChunkById( int nChunkId )
{
	ChunkIdMap::iterator it = m_chunkIdMap.find(nChunkId);
	if (it != m_chunkIdMap.end())
	{
		return it->second;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
static bool writeZeroes( FILE* file, unsigned int nByteCount )
{
	const char zeroes[32] = { 0 };

	while (nByteCount > 0)
	{
		const unsigned int n = (nByteCount <= sizeof(zeroes)) ? nByteCount : sizeof(zeroes);
		nByteCount -= n;
		if (fwrite(zeroes, n, 1, file) != 1)
		{
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CChunkFile::Write( const char *filename )
{
	static const int chunkAlignment = 4;

	FILE* const file = fopen( filename,"wb" );
	if (!file)
	{
		m_LastError.Format( "File %s failed to open for writing",filename );
		return false;
	}

	typedef CHUNK_TABLE_ENTRY_0745 ChunkTableEntry;

	const unsigned int numChunks = m_chunks.size();
	const int nChunkTableOffset = sizeof(m_fileHeader);
	const int nChunksOffset = nChunkTableOffset +  sizeof(int) + numChunks * sizeof(ChunkTableEntry);

	unsigned int nCurrOffset;

	//////////////////////////////////////////////////////////////////////////
	// Update offsets of chunks.
	//////////////////////////////////////////////////////////////////////////
	nCurrOffset = nChunksOffset;
	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		ChunkDesc& ch = *m_chunks[i];
		const unsigned int nAlignedOffset = (nCurrOffset + (chunkAlignment - 1)) & ~(chunkAlignment - 1);
		ch.hdr.FileOffset = nAlignedOffset;
		nCurrOffset = nAlignedOffset + ch.size;
	}

	//=======================
	//Write File Header.
	//=======================
	m_fileHeader.Version = ChunkFileVersion_Align;
	m_fileHeader.ChunkTableOffset = nChunkTableOffset;
	if (fwrite(&m_fileHeader,sizeof(m_fileHeader),1,file) != 1)
	{
		fclose(file);
		return false;
	}

	//=======================
	//Write Number of Chunks.
	//=======================
	assert( ftell(file) == nChunkTableOffset );
	if (fwrite(&numChunks,sizeof(numChunks),1,file) != 1)
	{
		fclose(file);
		return false;
	}

	//=======================
	//Write Chunk List.
	//=======================
	for(size_t i = 0; i < numChunks; ++i)
	{
		const ChunkDesc& ch = *m_chunks[i];

		ChunkTableEntry elem;
		elem.chdr = ch.hdr;
		elem.ChunkSize = ch.size;

		if (fwrite(&elem,sizeof(elem),1,file)!=1)
		{
			fclose(file);
			return false;
		}
	}

	//=======================
	// Write Chunks' Data.
	//=======================
	assert( ftell(file) == nChunksOffset );
	nCurrOffset = nChunksOffset;

	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		const ChunkDesc& ch = *m_chunks[i];

		const unsigned int nAlignedOffset = (nCurrOffset + (chunkAlignment - 1)) & ~(chunkAlignment - 1);
		assert( ch.hdr.FileOffset == nAlignedOffset );

		// Write zeroes into the space created by chunk alignment.
		if (!writeZeroes(file, nAlignedOffset - nCurrOffset))
		{
			fclose(file);
			return false;
		}

		// Copy chunk header.
		{
			bool bCopyHeader = true;

			// Hack for incompatible chunk types
			switch (ch.hdr.ChunkType)
			{
			case ChunkType_SourceInfo:
				bCopyHeader = false;
				break;
			case ChunkType_Controller:
				if (ch.hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION ||
					ch.hdr.ChunkVersion == CONTROLLER_BSPLINE_DATA_0826::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			case ChunkType_BoneNameList:
				if (ch.hdr.ChunkVersion == BONENAMELIST_CHUNK_DESC_0745::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			case ChunkType_MeshMorphTarget:
				if (ch.hdr.ChunkVersion == MESHMORPHTARGET_CHUNK_DESC_0001::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			case ChunkType_BoneInitialPos:
				if (ch.hdr.ChunkVersion == BONEINITIALPOS_CHUNK_DESC_0001::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			default:
				break;
			}

			if (bCopyHeader)
			{
				memcpy( ch.data,&ch.hdr,sizeof(ch.hdr) );
			}
		}

		// Write data.
		if (fwrite(ch.data,ch.size,1,file) != 1)
		{
			fclose(file);
			return false;
		}
	
		nCurrOffset = nAlignedOffset + ch.size;
	}

	fclose(file);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::WriteToMemory( void **pData,int *nSize )
{
	static const int chunkAlignment = 4;

	typedef CHUNK_TABLE_ENTRY_0745 ChunkTableEntry;

	const unsigned int numChunks = m_chunks.size();
	const int nChunkTableOffset = sizeof(m_fileHeader);
	const int nChunksOffset = nChunkTableOffset +  sizeof(int) + numChunks * sizeof(ChunkTableEntry);

	unsigned int nCurrOffset;

	//////////////////////////////////////////////////////////////////////////
	// Update offsets of chunks.
	//////////////////////////////////////////////////////////////////////////
	nCurrOffset = nChunksOffset;
	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		ChunkDesc& ch = *m_chunks[i];
		const unsigned int nAlignedOffset = (nCurrOffset + (chunkAlignment - 1)) & ~(chunkAlignment - 1);
		ch.hdr.FileOffset = nAlignedOffset;
		nCurrOffset = nAlignedOffset + ch.size;
	}

	const unsigned int nTotalSize = nCurrOffset;
	
	if (m_pInternalData)
	{
		free(m_pInternalData);
	}
	m_pInternalData = (char*)malloc( nTotalSize );

	char *pBuf = m_pInternalData;
	assert(pBuf);

	nCurrOffset = 0;

	//////////////////////////////////////////////////////////////////////////
	// Write File Header.
	//////////////////////////////////////////////////////////////////////////
	m_fileHeader.Version = ChunkFileVersion_Align;
	m_fileHeader.ChunkTableOffset = nChunkTableOffset;
	memcpy( &pBuf[nCurrOffset],&m_fileHeader,sizeof(m_fileHeader) );
	nCurrOffset += sizeof(m_fileHeader);

	//////////////////////////////////////////////////////////////////////////
	// Write Number of Chunks
	//////////////////////////////////////////////////////////////////////////
	assert( nCurrOffset == nChunkTableOffset );
	memcpy( &pBuf[nCurrOffset],&numChunks,sizeof(numChunks) );
	nCurrOffset += sizeof(numChunks);

	//////////////////////////////////////////////////////////////////////////
	// Write Chunk List
	//////////////////////////////////////////////////////////////////////////
	for(size_t i = 0; i < numChunks; ++i)
	{
		const ChunkDesc& ch = *m_chunks[i];

		ChunkTableEntry elem;
		elem.chdr = ch.hdr;
		elem.ChunkSize = ch.size;

		memcpy( &pBuf[nCurrOffset],&elem,sizeof(elem) );
		nCurrOffset += sizeof(elem);
	}

	//////////////////////////////////////////////////////////////////////////
	// Write Chunks' Data.
	//////////////////////////////////////////////////////////////////////////
	assert( nCurrOffset == nChunksOffset );

	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		const ChunkDesc& ch = *m_chunks[i];

		const unsigned int nAlignedOffset = (nCurrOffset + (chunkAlignment - 1)) & ~(chunkAlignment - 1);
		assert( ch.hdr.FileOffset == nAlignedOffset );

		// Write zeroes into the space created by chunk alignment.
		memset( &pBuf[nCurrOffset],0,nAlignedOffset - nCurrOffset);
		nCurrOffset += nAlignedOffset - nCurrOffset;

		// Copy chunk header.
		{
			bool bCopyHeader = true;

			// Hack for incompatible chunk types
			switch (ch.hdr.ChunkType)
			{
			case ChunkType_SourceInfo:
				bCopyHeader = false;
				break;
			case ChunkType_Controller:
				if (ch.hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION ||
					ch.hdr.ChunkVersion == CONTROLLER_BSPLINE_DATA_0826::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			case ChunkType_BoneNameList:
				if (ch.hdr.ChunkVersion == BONENAMELIST_CHUNK_DESC_0745::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			case ChunkType_MeshMorphTarget:
				if (ch.hdr.ChunkVersion == MESHMORPHTARGET_CHUNK_DESC_0001::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			case ChunkType_BoneInitialPos:
				if (ch.hdr.ChunkVersion == BONEINITIALPOS_CHUNK_DESC_0001::VERSION)
				{
					bCopyHeader = false;
				}
				break;
			default:
				break;
			}

			if (bCopyHeader)
			{
				memcpy( ch.data,&ch.hdr,sizeof(ch.hdr) );
			}
		}

		// Write data.
		memcpy( &pBuf[nCurrOffset],ch.data,ch.size );
		nCurrOffset += ch.size;
	}

	*pData = m_pInternalData;

	assert(nTotalSize > 0);
	*nSize = (int)nTotalSize;
}

//////////////////////////////////////////////////////////////////////////
bool CChunkFile::ReadChunkTable( CCryFile &file )
{
	FUNCTION_PROFILER_3DENGINE;
//  LOADING_TIME_PROFILE_SECTION;

	int res;

	ReleaseChunks();

	if (file.Seek(m_fileHeader.ChunkTableOffset,SEEK_SET) != 0)
	{
		m_LastError.Format( 
			"Chunk table offset (%d (0x%08x)) is bad in file %s", 
			m_fileHeader.ChunkTableOffset, m_fileHeader.ChunkTableOffset, file.GetFilename());
		return false;
	}

	int n_chunks;
	res = file.ReadType(&n_chunks);
	if (res != sizeof(n_chunks))
	{
		m_LastError.Format( 
			"Failed to read chunk count from file %s (%d bytes were read)", 
			file.GetFilename(), res);
		return false;
	}
	if ((n_chunks < 0) || (n_chunks > MAX_CHUNKS_NUM))
	{
		m_LastError.Format( 
			"Chunk count (%d (0x%08x)) is bad in file %s", 
			n_chunks, n_chunks, file.GetFilename());
		return false;
	}

	if (m_fileHeader.Version == ChunkFileVersion_Align)
	{
		typedef CHUNK_TABLE_ENTRY_0745 ChunkTableEntry;

		ChunkTableEntry *chunks = new ChunkTableEntry[n_chunks];
		assert( chunks );
		res = file.ReadType( chunks,n_chunks );
		if(res != sizeof(ChunkTableEntry)*n_chunks)
		{
			m_LastError.Format( "Failed to read chunk list from file %s", file.GetFilename() );
			delete []chunks;
			return false;
		}

		m_chunks.clear();
		m_chunks.resize(n_chunks);

		for (size_t i = 0; i < m_chunks.size(); ++i)
		{
			m_chunks[i] = new ChunkDesc;
			m_chunks[i]->hdr = chunks[i].chdr;
			m_chunks[i]->data = 0;
			m_chunks[i]->size = chunks[i].ChunkSize;

			m_chunks[i]->bSwapEndian = (m_chunks[i]->hdr.ChunkVersion & CONSOLE_VERSION_MASK) ? eBigEndian : eLittleEndian;
			m_chunks[i]->hdr.ChunkVersion &= ~CONSOLE_VERSION_MASK;
		}

		delete []chunks;

		for (size_t i = 0; i < m_chunks.size(); ++i)
		{
			ChunkDesc &cd = *m_chunks[i];

			cd.data = new char[cd.size];

			file.Seek( cd.hdr.FileOffset,SEEK_SET );

			res = file.ReadRaw( cd.data,cd.size );
			if (res != cd.size)
			{
				m_LastError.Format(
					"Failed to read chunk data (offset:%d, size:%d) from file %s",
					cd.hdr.FileOffset, cd.size, file.GetFilename() );
				return false;
			}
		}
	}
	else
	{
		CHUNK_HEADER *chunks = new CHUNK_HEADER[n_chunks];
		assert( chunks );
		res = file.ReadType( chunks,n_chunks );
		if(res != sizeof(CHUNK_HEADER)*n_chunks)
		{
			m_LastError.Format( "Failed to read chunk list from file %s", file.GetFilename() );
			delete []chunks;
			return false;
		}

		m_chunks.clear();
		m_chunks.resize(n_chunks);
		std::vector<ChunkDesc*> sortedChunks;
		sortedChunks.resize(n_chunks);

		for (size_t i = 0; i < m_chunks.size(); ++i)
		{
			m_chunks[i] = new ChunkDesc;
			m_chunks[i]->hdr = chunks[i];
			m_chunks[i]->data = 0;
			m_chunks[i]->size = -1;

			m_chunks[i]->bSwapEndian = (m_chunks[i]->hdr.ChunkVersion & CONSOLE_VERSION_MASK) ? eBigEndian : eLittleEndian;
			m_chunks[i]->hdr.ChunkVersion &= ~CONSOLE_VERSION_MASK;

			sortedChunks[i] = m_chunks[i];
		}

		delete [] chunks;

		std::sort( sortedChunks.begin(),sortedChunks.end(),ChunkLess );
			
		const unsigned int nEndOfChunks = (m_fileHeader.ChunkTableOffset == sizeof(m_fileHeader))
			? file.GetLength()
			: m_fileHeader.ChunkTableOffset;

		for (size_t i = 0; i < sortedChunks.size(); ++i)
		{
			// calculate the chunk size, based on the very next chunk with greater offset
			// or the end of the raw data portion of the file

			const int nextFileOffset = (i+1 < sortedChunks.size())
				? sortedChunks[i+1]->hdr.FileOffset
				: nEndOfChunks;

			ChunkDesc &cd = *sortedChunks[i];

			cd.size = nextFileOffset - cd.hdr.FileOffset;
			cd.data = new char[cd.size];

			file.Seek( cd.hdr.FileOffset,SEEK_SET );

			res = file.ReadRaw( cd.data,cd.size );
			if (res != cd.size)
			{
				m_LastError.Format(
					"Failed to read chunk data (offset:%d, size:%d) from file %s", 
					cd.hdr.FileOffset, cd.size, file.GetFilename());
				return false;
			}
		}
	}

	m_nLastChunkId = 0;

	for (size_t i = 0; i < m_chunks.size(); ++i)
	{
		// Add chunk to chunk map.
		m_chunkIdMap[m_chunks[i]->hdr.ChunkID] = m_chunks[i];

		// Find last chunk ID.
		if (m_chunks[i]->hdr.ChunkID > m_nLastChunkId)
		{
			m_nLastChunkId = m_chunks[i]->hdr.ChunkID;
		}
	}

	if (m_chunks.size() != m_chunkIdMap.size())
	{
		const int duplicateCount = (int)(m_chunks.size() - m_chunkIdMap.size());
		m_LastError.Format( 
			"%d duplicate chunk ID%s found in file %s", 
			duplicateCount, ((duplicateCount > 1) ? "s" : ""), file.GetFilename());
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CChunkFile::Read( const char *filename )
{
	FUNCTION_PROFILER_3DENGINE;
//  LOADING_TIME_PROFILE_SECTION;

	CCryFile file;
	if (!file.Open(filename, "rb"))
	{
		m_LastError.Format("File %s failed to open for reading", filename);
		return false;
	}

	if (!file.ReadType(&m_fileHeader))
	{
		m_LastError.Format(
			"Failed to read file header (%lu bytes) from file %s (file size is %lu bytes)",
			(unsigned long)sizeof(m_fileHeader), filename, (unsigned long)file.GetLength());
		return false;
	}

	if (!ReadChunkTable(file))
	{
		return false;
	}

	m_bLoaded = true;

	return true;
}




//////////////////////////////////312////////////////////////////////////////
CChunkFile::ChunkDesc* CChunkFile::FindChunkByType( int type )
{
	for (unsigned int i = 0; i < m_chunks.size(); i++)
	{
		if (m_chunks[i]->hdr.ChunkType == type) 
		{
			return m_chunks[i];
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////