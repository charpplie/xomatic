////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ANMSaver.cpp
//  Version:     v1.00
//  Created:     27/9/2007 by Norbert
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ANMSaver.h"

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

void CSaverANM::Save(CContentCGF *pCGF,CInternalSkinningInfo* pSkinningInfo)
{
	assert(pCGF);
	assert(pSkinningInfo);

	SaveExportFlags(pCGF);
	SaveTiming(pSkinningInfo);
	
	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		int nextChunk = m_pChunkFile->NumChunks();

		int positionIndex = -1;
		int rotationIndex = -1;
		int scaleIndex = -1;

		if (pSkinningInfo->m_arrControllers[i*3].type == 0x55)
		{
			positionIndex = nextChunk+1;
			nextChunk++;
		}

		if (pSkinningInfo->m_arrControllers[i*3+1].type == 0xaa)
		{
			rotationIndex = nextChunk+1;
			nextChunk++;
		}

		if (pSkinningInfo->m_arrControllers[i*3+2].type == 0x55)
		{
			scaleIndex = nextChunk+1;
			nextChunk++;
		}

		if (positionIndex != -1 || rotationIndex != -1 || scaleIndex != -1)
		{
			SaveNode(pCGF->GetNode(i),positionIndex,rotationIndex,scaleIndex);

			if (positionIndex != -1)
			{
				SaveTCB3Track(pSkinningInfo,pSkinningInfo->m_arrControllers[i*3].index);
			}
			if (rotationIndex != -1)
			{
				SaveTCBQTrack(pSkinningInfo,pSkinningInfo->m_arrControllers[i*3+1].index);
			}
			if (scaleIndex != -1)
			{
				SaveTCB3Track(pSkinningInfo,pSkinningInfo->m_arrControllers[i*3+2].index);
			}
		}
	}
}

#define SCALE_TO_CGF 100.0f

int CSaverANM::SaveNode(CNodeCGF *pNode,int pos_cont_id,int rot_cont_id,int scl_cont_id)
{
	NODE_CHUNK_DESC chunk;
	CHUNK_HEADER chunk_hdr;

	ZeroStruct(chunk);

	chunk_hdr.ChunkType = ChunkType_Node;
	chunk_hdr.ChunkVersion = NODE_CHUNK_DESC::VERSION;

	strcpy(chunk.name,pNode->name);

	chunk.ObjectID = -1;
	chunk.ParentID = -1;
	chunk.nChildren = 0;
	chunk.MatID = -1;
	chunk.IsGroupHead = false;
	chunk.IsGroupMember = false;

	float *pMat = (float*)&chunk.tm;
	pMat[0] = pNode->localTM(0,0);
	pMat[1] = pNode->localTM(1,0);
	pMat[2] = pNode->localTM(2,0);
	pMat[4] = pNode->localTM(0,1);
	pMat[5] = pNode->localTM(1,1);
	pMat[6] = pNode->localTM(2,1);
	pMat[8] = pNode->localTM(0,2);
	pMat[9] = pNode->localTM(1,2);
	pMat[10]= pNode->localTM(2,2);
	pMat[12] = pNode->localTM.GetTranslation().x * SCALE_TO_CGF;
	pMat[13] = pNode->localTM.GetTranslation().y * SCALE_TO_CGF;
	pMat[14] = pNode->localTM.GetTranslation().z * SCALE_TO_CGF;
	
	chunk.pos = pNode->pos;
	chunk.rot = pNode->rot;
	chunk.scl = pNode->scl;

	chunk.pos_cont_id = pos_cont_id;
	chunk.rot_cont_id = rot_cont_id;
	chunk.scl_cont_id = scl_cont_id;

	chunk.PropStrLen = 0;

	CChunkData chunkData;
	chunkData.Add(chunk);

	return m_pChunkFile->AddChunk(chunk_hdr,chunkData.data,chunkData.size);
}

int CSaverANM::SaveTCB3Track(CInternalSkinningInfo* pSkinningInfo,int trackIndex)
{
	return CSaverAnim::SaveTCB3Track(m_pChunkFile, pSkinningInfo, trackIndex);
}

int CSaverANM::SaveTCBQTrack(CInternalSkinningInfo* pSkinningInfo,int trackIndex)
{
	return CSaverAnim::SaveTCBQTrack(m_pChunkFile, pSkinningInfo, trackIndex);
}