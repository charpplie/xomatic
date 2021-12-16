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

// enable loading profiler if compiled as part of engine
#if defined(FUNCTION_PROFILER_3DENGINE)
  #define LOADING_TIME_PROFILE_SECTION_CGF LOADING_TIME_PROFILE_SECTION(gEnv->pSystem)
#else
  #define LOADING_TIME_PROFILE_SECTION_CGF
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
		free(m_pInternalData);
}

// retrieves the raw chunk header, as it appears in the file
const CChunkFile::ChunkHeader& CChunkFile::GetChunkHeader(int nChunkIdx)const
{
	return m_chunks[nChunkIdx]->hdr;
}

//////////////////////////////////////////////////////////////////////////
CChunkFile::ChunkDesc* CChunkFile::GetChunk( int nIndex )
{
	assert( nIndex >= 0 && nIndex < (int)m_chunks.size() );
	return m_chunks[nIndex];
}

// returns the raw data of the i-th chunk
const void* CChunkFile::GetChunkData(int nChunkIdx)const
{
	assert (nChunkIdx >= 0 && nChunkIdx < NumChunks());
	if (nChunkIdx>= 0 && nChunkIdx < NumChunks())
	{
		return m_chunks[nChunkIdx]->data;
	}
	else
		return 0;
}

// number of chunks
int CChunkFile::NumChunks()const
{
	return (int)m_chunks.size();
}

// number of chunks of the specified type
int CChunkFile::NumChunksOfType (ChunkTypes nChunkType)const
{
	int nResult = 0;
	for (int i = 0; i < NumChunks(); ++i)
	{
		if (m_chunks[i]->hdr.ChunkType == nChunkType)
			++nResult;
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
	ChunkDesc *chunk = new ChunkDesc;
	chunk->hdr = hdr;
	chunk->data = new char[chunkSize];
	chunk->size = chunkSize;
	memcpy( chunk->data,chunkData,chunkSize );

	int chunkID = ++m_nLastChunkId;
	chunk->hdr.ChunkID = chunkID;
	m_chunks.push_back( chunk );
	m_chunkIdMap[chunkID] = chunk;
	return chunkID;
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::SetChunkData( int nChunkId,void *chunkData,int chunkSize )
{
	ChunkDesc *pChunk = FindChunkById(nChunkId);
	delete [](char *)pChunk->data;
	pChunk->data = new char[chunkSize];
	pChunk->size = chunkSize;
	memcpy( pChunk->data,chunkData,chunkSize );
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::DeleteChunkId( int nChunkId )
{
	for (unsigned int i = 0; i < m_chunks.size(); i++)
	{
		if (m_chunks[i]->hdr.ChunkID == nChunkId) 
		{
			if (m_chunks[i]->data)
				delete [] (char *)m_chunks[i]->data;
			m_chunks.erase( m_chunks.begin()+i );
			return;
		}
	}
	m_chunkIdMap.erase(nChunkId);
}

//////////////////////////////////////////////////////////////////////////
void CChunkFile::ReleaseChunks()
{
	m_bLoaded = false;
	for (unsigned int i = 0; i < m_chunks.size(); i++)
	{
		if (m_chunks[i]->data)
			delete [](char *) m_chunks[i]->data;
		delete m_chunks[i];
	}
	m_chunks.clear();
	m_chunkIdMap.clear();
}

//////////////////////////////////////////////////////////////////////////
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
CChunkFile::ChunkDesc* CChunkFile::FindChunkById( int id )
{
	ChunkIdMap::iterator it = m_chunkIdMap.find(id);
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
		if (fwrite(zeroes,n,1,file) != 1)
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

	FILE *file = fopen( filename,"wb" );
	if (!file)
	{
		m_LastError.Format( "File %s failed to open for writing",filename );
		return false;
	}

	typedef CHUNK_TABLE_ENTRY_0745 ChunkTableEntry;

	const unsigned int numChunks = m_chunks.size();
	const int nChunkTableOffset = sizeof(m_fileHeader);
	const int nChunksOffset = nChunkTableOffset +  sizeof(int) + numChunks * sizeof(ChunkTableEntry);

	unsigned int i;
	unsigned int nCurrOffset;

	//////////////////////////////////////////////////////////////////////////
	// Update offsets of chunks.
	//////////////////////////////////////////////////////////////////////////
	nCurrOffset = nChunksOffset;
	for (i = 0; i < m_chunks.size(); ++i)
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
	for(i = 0; i < numChunks; ++i)
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

	for (i = 0; i < m_chunks.size(); ++i)
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

		// Copy header.
		switch (ch.hdr.ChunkType)
		{
			// workaround for incompatible chunk types
		case ChunkType_Controller:
			if (ch.hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION ||
				ch.hdr.ChunkVersion == CONTROLLER_BSPLINE_DATA_0826::VERSION)
				break;
		case ChunkType_BoneNameList:
			if (ch.hdr.ChunkVersion == BONENAMELIST_CHUNK_DESC_0745::VERSION)
				break;
		case ChunkType_MeshMorphTarget:
			if (ch.hdr.ChunkVersion == MESHMORPHTARGET_CHUNK_DESC_0001::VERSION)
				break;
		case ChunkType_BoneInitialPos:
			if (ch.hdr.ChunkVersion == BONEINITIALPOS_CHUNK_DESC_0001::VERSION)
				break;
		default:
			memcpy( ch.data,&ch.hdr,sizeof(ch.hdr) );
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

	unsigned int i;

	unsigned int size;
	unsigned int nCurrOffset;

	//////////////////////////////////////////////////////////////////////////
	// Update offsets of chunks.
	//////////////////////////////////////////////////////////////////////////
	nCurrOffset = nChunksOffset;
	for (i = 0; i < m_chunks.size(); ++i)
	{
		ChunkDesc& ch = *m_chunks[i];
		const unsigned int nAlignedOffset = (nCurrOffset + (chunkAlignment - 1)) & ~(chunkAlignment - 1);
		ch.hdr.FileOffset = nAlignedOffset;
		nCurrOffset = nAlignedOffset + ch.size;
	}

	size = nCurrOffset;
	
	if (m_pInternalData)
		free(m_pInternalData);
	m_pInternalData = (char*)malloc( size );

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
	for(i = 0; i < numChunks; ++i)
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

	for (i = 0; i < m_chunks.size(); ++i)
	{
		const ChunkDesc& ch = *m_chunks[i];

		const unsigned int nAlignedOffset = (nCurrOffset + (chunkAlignment - 1)) & ~(chunkAlignment - 1);
		assert( ch.hdr.FileOffset == nAlignedOffset );

		// Write zeroes into the space created by chunk alignment.
		memset( &pBuf[nCurrOffset],0,nAlignedOffset - nCurrOffset);
		nCurrOffset += nAlignedOffset - nCurrOffset;

		// Copy header.
		switch (ch.hdr.ChunkType)
		{
			// workaround for incompatible chunk types
		case ChunkType_Controller:
			if (ch.hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION ||
				ch.hdr.ChunkVersion == CONTROLLER_BSPLINE_DATA_0826::VERSION)
				break;
		case ChunkType_BoneNameList:
			if (ch.hdr.ChunkVersion == BONENAMELIST_CHUNK_DESC_0745::VERSION)
				break;
		case ChunkType_MeshMorphTarget:
			if (ch.hdr.ChunkVersion == MESHMORPHTARGET_CHUNK_DESC_0001::VERSION)
				break;
		case ChunkType_BoneInitialPos:
			if (ch.hdr.ChunkVersion == BONEINITIALPOS_CHUNK_DESC_0001::VERSION)
				break;
		default:
			memcpy( ch.data,&ch.hdr,sizeof(ch.hdr) );
		}

		// Write data.
		memcpy( &pBuf[nCurrOffset],ch.data,ch.size );
		nCurrOffset += ch.size;
	}

	*pData = m_pInternalData;
	*nSize = size;
}

//////////////////////////////////////////////////////////////////////////
bool CChunkFile::ReadChunkTable( CCryFile &file )
{
	LOADING_TIME_PROFILE_SECTION_CGF;

	int res;
	unsigned i;

	ReleaseChunks();

	file.Seek(m_fileHeader.ChunkTableOffset,SEEK_SET);

	int n_chunks;
	res = file.ReadType(&n_chunks);
	if(res!=sizeof(n_chunks) || n_chunks < 0 || n_chunks > MAX_CHUNKS_NUM)
	{
		m_LastError.Format( "Failed to read chunk count from file %s", file.GetFilename() );
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

		for (i = 0; i < (int)m_chunks.size(); ++i)
		{
			m_chunks[i] = new ChunkDesc;
			m_chunks[i]->hdr = chunks[i].chdr;
			m_chunks[i]->data = 0;
			m_chunks[i]->size = chunks[i].ChunkSize;

			m_chunks[i]->bSwapEndian = (m_chunks[i]->hdr.ChunkVersion & CONSOLE_VERSION_MASK) ? eBigEndian : eLittleEndian;
			m_chunks[i]->hdr.ChunkVersion &= ~CONSOLE_VERSION_MASK;
		}

		delete []chunks;

		for (i = 0; i < (int)m_chunks.size(); ++i)
		{
			ChunkDesc &cd = *m_chunks[i];

			cd.data = new char[cd.size];

			file.Seek( cd.hdr.FileOffset,SEEK_SET );

			res = file.ReadRaw( cd.data,cd.size );
			if (res != cd.size)
			{
				m_LastError.Format( "Failed to read chunk data (offset:%d, size:%d) from file %s", cd.hdr.FileOffset, cd.size, file.GetFilename() );
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

		for (i = 0; i < (int)m_chunks.size(); ++i)
		{
			m_chunks[i] = new ChunkDesc;
			m_chunks[i]->hdr = chunks[i];
			m_chunks[i]->data = 0;
			m_chunks[i]->size = -1;

			m_chunks[i]->bSwapEndian = (m_chunks[i]->hdr.ChunkVersion & CONSOLE_VERSION_MASK) ? eBigEndian : eLittleEndian;
			m_chunks[i]->hdr.ChunkVersion &= ~CONSOLE_VERSION_MASK;

			sortedChunks[i] = m_chunks[i];
		}

		delete []chunks;

		std::sort( sortedChunks.begin(),sortedChunks.end(),ChunkLess );
			
		const unsigned int nEndOfChunks = (m_fileHeader.ChunkTableOffset == sizeof(m_fileHeader))
			? file.GetLength()
			: m_fileHeader.ChunkTableOffset;

		for (i = 0; i < sortedChunks.size(); ++i)
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
				m_LastError.Format( "Failed to read chunk data (offset:%d, size:%d) from file %s", cd.hdr.FileOffset, cd.size, file.GetFilename() );
				return false;
			}
		}
	}

	m_nLastChunkId = 0;

	for (i = 0; i < (int)m_chunks.size(); ++i)
	{
		// Add chunk to chunkid map.
		m_chunkIdMap[m_chunks[i]->hdr.ChunkID] = m_chunks[i];

		// Find last chunk id.
		if (m_chunks[i]->hdr.ChunkID > m_nLastChunkId)
		{
			m_nLastChunkId = m_chunks[i]->hdr.ChunkID;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CChunkFile::Read( const char *filename )
{
  LOADING_TIME_PROFILE_SECTION_CGF;

  CCryFile file;
  if (!file.Open(filename,"rb"))
  {
    m_LastError.Format( "File %s failed to open for reading",filename );
    return false;
  }

	if (!file.ReadType( &m_fileHeader))
	{
		m_LastError.Format( "Failed to read file header from file %s",filename );
		return false;
	}

	if (!ReadChunkTable( file ))
	{
		return false;
	}

	m_bLoaded = true;

	return true;
}

