////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   AnimSaver.cpp
//  Version:     v1.00
//  Created:     27/9/2007 by Norbert
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimSaver.h"

struct CChunkData
{
	char *data;
	int size;

	CChunkData() { data = 0; size = 0; }
	~CChunkData() { free(data); }

	template <class T>
	void Add( const T& object )
	{
		AddData( &object,sizeof(object) );
	}
	void AddData( const void *pSrcData,int nSrcDataSize )
	{
		data = (char*)realloc(data,size+nSrcDataSize);
		memcpy( data+size,pSrcData,nSrcDataSize );
		size += nSrcDataSize;
	}
};

CSaverAnim::CSaverAnim(const char *filename, CChunkFile &chunkFile)
{
	m_filename = filename;
	m_pChunkFile = &chunkFile;
}

int CSaverAnim::SaveExportFlags(CContentCGF *pCGF)
{
	EXPORT_FLAGS_CHUNK_DESC chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_ExportFlags;
	chunk.chdr.ChunkVersion = EXPORT_FLAGS_CHUNK_DESC::VERSION;

	CExportInfoCGF *pExpInfo = pCGF->GetExportInfo();
	if (pExpInfo->bMergeAllNodes)
		chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::MERGE_ALL_NODES;

	if (pExpInfo->bFromColladaXSI)
		chunk.reserved[0] |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_XSI;
	if (pExpInfo->bFromColladaMAX)
		chunk.reserved[0] |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_MAX;

	enum {NUM_RC_VERSION_ELEMENTS = sizeof(pExpInfo->rc_version) / sizeof(pExpInfo->rc_version[0])};
	std::copy(pExpInfo->rc_version, pExpInfo->rc_version + NUM_RC_VERSION_ELEMENTS, chunk.rc_version);
	strncpy( chunk.rc_version_string,pExpInfo->rc_version_string,sizeof(chunk.rc_version_string) );

	return m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
}

int CSaverAnim::SaveTiming(CInternalSkinningInfo* pSkinningInfo)
{
	return SaveTiming(m_pChunkFile, pSkinningInfo);
}

int CSaverAnim::SaveTCB3Track(CChunkFile *pChunkFile, CInternalSkinningInfo* pSkinningInfo, int trackIndex)
{
	CONTROLLER_CHUNK_DESC_0826 chunk;
	CHUNK_HEADER chunk_hdr;

	ZeroStruct(chunk);

	chunk.chdr.ChunkID = pChunkFile->NumChunks();
	chunk_hdr.ChunkType = ChunkType_Controller;
	chunk_hdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0826::VERSION;

	chunk.type = CTRL_TCB3;
	chunk.nKeys = pSkinningInfo->m_TrackVec3[trackIndex]->size();

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData(&(*pSkinningInfo->m_TrackVec3[trackIndex])[0],chunk.nKeys*sizeof(CryTCB3Key));

	return pChunkFile->AddChunk(chunk_hdr,chunkData.data,chunkData.size);
}

int CSaverAnim::SaveTCBQTrack(CChunkFile *pChunkFile, CInternalSkinningInfo* pSkinningInfo, int trackIndex)
{
	CONTROLLER_CHUNK_DESC_0826 chunk;
	CHUNK_HEADER chunk_hdr;

	ZeroStruct(chunk);

	chunk.chdr.ChunkID = pChunkFile->NumChunks();
	chunk_hdr.ChunkType = ChunkType_Controller;
	chunk_hdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0826::VERSION;

	chunk.type = CTRL_TCBQ;
	chunk.nKeys = pSkinningInfo->m_TrackQuat[trackIndex]->size();

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData(&(*pSkinningInfo->m_TrackQuat[trackIndex])[0],chunk.nKeys*sizeof(CryTCBQKey));

	return pChunkFile->AddChunk(chunk_hdr,chunkData.data,chunkData.size);
}

int CSaverAnim::SaveTiming(CChunkFile *pChunkFile, CInternalSkinningInfo* pSkinningInfo)
{
	TIMING_CHUNK_DESC chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Timing;
	chunk.chdr.ChunkVersion = TIMING_CHUNK_DESC::VERSION;

	chunk.TicksPerFrame = pSkinningInfo->m_nTicksPerFrame;
	chunk.SecsPerTick = pSkinningInfo->m_secsPerTick;
	chunk.global_range.start = pSkinningInfo->m_nStart;
	chunk.global_range.end = pSkinningInfo->m_nEnd;
	strcpy(chunk.global_range.name,"GlobalRange");		//TODO: for what? (copied from max exporter)

	return pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
}