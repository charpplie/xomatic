////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   ReadOnlyChunkFile.cpp
//  Version:     v1.00
//  Created:     15/11/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _READONLY_CHUNK_FILE_READER_HDR_
#define _READONLY_CHUNK_FILE_READER_HDR_

#include "CryHeaders.h"
#include <smartptr.h>
#include <IChunkFile.h>

////////////////////////////////////////////////////////////////////////
// Chunk file reader. 
// Accesses a chunked file structure through file mapping object.
// Opens a chunk file and checks for its validity.
// If it's invalid, closes it as if there was no open operation.
// Error handling is performed through the return value of Read: it must
// be true for successfully open files
////////////////////////////////////////////////////////////////////////

class CReadOnlyChunkFile : public IChunkFile
{
public:
	//////////////////////////////////////////////////////////////////////////
	CReadOnlyChunkFile( bool bCopyFileData, bool bNoWarningMode = false );
	virtual ~CReadOnlyChunkFile();

	// interface IChunkFile --------------------------------------------------

	virtual void Release() { delete this; };

	virtual bool IsReadOnly() const { return true; };
	virtual bool IsLoaded() const { return m_bLoaded; };

	virtual bool Read( const char *filename );
	virtual bool ReadFromMemBlock( const void * pData, int nDataSize );
	
	// Write chunks to file.
	virtual bool Write( const char *filename ) { return false; };
	virtual void WriteToMemory( void **pData,int *nSize ) {};

	//! Add chunk to file.
	//! @return ChunkID of added chunk.
	virtual int AddChunk( const CHUNK_HEADER &hdr,void *chunkData,int chunkSize ) { return -1; };
	virtual void SetChunkData( int nChunkId,void *chunkData,int chunkSize ) {};
	virtual void DeleteChunkId( int nChunkId ) {};

	virtual ChunkDesc* FindChunkByType( ChunkTypes nChunkType );
	virtual ChunkDesc* FindChunkById( int nChunkId );

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
	virtual int NumChunksOfType(ChunkTypes nChunkType) const;
	virtual const char* GetLastError() const { return m_LastError; }


//312
	virtual ChunkDesc* FindChunkByType( int type );

	// -----------------------------------------------------------------------

	//virtual void GetMemoryUsage(ICrySizer *pSizer) const
	//{
	//	pSizer->AddObject(this, sizeof(*this));
	//	pSizer->AddObject(m_LastError);
	//	pSizer->AddObject(m_chunks);
	//}
private:
	bool ReadChunkTableFromBuffer();
	void FreeBuffer();

private:
	// this variable contains the last error occurred in this class
	string m_LastError;

	FILE_HEADER m_fileHeader;
	std::vector<ChunkDesc> m_chunks;
	
	char *m_pFileBuffer;
	int m_nBufferSize;
	bool m_bOwnFileBuffer;
	bool m_bNoWarningMode;
	bool m_bLoaded;
	bool m_bCopyFileData;

	FILE *m_hFile;
};

TYPEDEF_AUTOPTR(CReadOnlyChunkFile);

#endif //_READONLY_CHUNK_FILE_READER_HDR_