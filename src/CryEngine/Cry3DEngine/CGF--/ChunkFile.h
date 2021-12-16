//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:ChunkFile.h
//  Declaration of class CChunkFile
//
//	History:
//	06/26/2002 :Created by Sergiy Migdalskiy <sergiy@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#ifndef _CHUNK_FILE_READER_HDR_
#define _CHUNK_FILE_READER_HDR_

#include "CryHeaders.h"
#include <smartptr.h>
#include <IChunkFile.h>

////////////////////////////////////////////////////////////////////////
// Chunk file reader. 
// Accesses a chunked file structure through file mapping object.
// Opens a chunk file and checks for its validity.
// If it's invalid, closes it as if there was no open operation.
// Error handling is performed through the return value of open: it must
// be true for successfully open files
////////////////////////////////////////////////////////////////////////

class CChunkFile : public IChunkFile
{
public:
	//////////////////////////////////////////////////////////////////////////
	CChunkFile();
	virtual ~CChunkFile();

	// interface IChunkFile --------------------------------------------------

	virtual void Release() { delete this; };

	virtual bool IsReadOnly() const { return false; };
	virtual bool IsLoaded() const { return m_bLoaded; };

	virtual bool Read( const char *filename );
	virtual bool ReadFromMemBlock( const void *pData,int nDataSize ) { return false; }
	
	// Write chunks to file.
	virtual bool Write( const char *filename );
	virtual void WriteToMemory( void **pData,int *nSize );

	//! Add chunk to file.
	//! @return ChunkID of added chunk.
	virtual int AddChunk( const CHUNK_HEADER &hdr,void *chunkData,int chunkSize );
	virtual void SetChunkData( int nChunkId,void *chunkData,int chunkSize );
	virtual void DeleteChunkId( int nChunkId );

	virtual ChunkDesc* FindChunkByType( int type );
	virtual ChunkDesc* FindChunkById( int id );

	// returns the file header
	virtual const FileHeader& GetFileHeader() const { return m_fileHeader; };

	// returns the raw data of the i-th chunk
	virtual const void* GetChunkData(int nIndex ) const;
	// retrieves the raw chunk header, as it appears in the file
	virtual const ChunkHeader& GetChunkHeader( int nIndex ) const;
	// Get chunk description at i-th index.
	virtual ChunkDesc* GetChunk( int nIndex );
	// calculates the chunk size, based on the very next chunk with greater offset
	// or the end of the raw data portion of the file
	virtual int GetChunkSize( int nIndex ) const;
	// number of chunks
	virtual int NumChunks() const;
	// number of chunks of the specified type
	virtual int NumChunksOfType (ChunkTypes nChunkType) const;
	virtual const char* GetLastError() const { return m_LastError; }

	// -----------------------------------------------------------------------

private:
	void ReleaseChunks();
	bool ReadChunkTable( CCryFile &file );

private:
	// this variable contains the last error occured in this class
	string m_LastError;

	int m_nLastChunkId;
	FILE_HEADER m_fileHeader;
	std::vector<ChunkDesc*> m_chunks;
	typedef std::map<int,ChunkDesc*> ChunkIdMap;
	ChunkIdMap m_chunkIdMap;
	char *m_pInternalData;
	bool m_bLoaded;
};

TYPEDEF_AUTOPTR(CChunkFile);

#endif