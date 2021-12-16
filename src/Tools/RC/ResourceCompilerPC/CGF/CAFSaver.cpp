////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CAFSaver.cpp
//  Version:     v1.00
//  Created:     27/9/2007 by Norbert
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CAFSaver.h"

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

void CSaverCAF::Save(CContentCGF *pCGF,CInternalSkinningInfo* pSkinningInfo)
{
	assert(pCGF);
	assert(pSkinningInfo);

	SaveExportFlags(pCGF);
	SaveTiming(pSkinningInfo);
	
	for (int i=0; i<pSkinningInfo->m_pControllers.size(); i++)
		SaveController(pSkinningInfo,i);

	SaveBoneNameList(pCGF);
}

int CSaverCAF::SaveController(CInternalSkinningInfo* pSkinningInfo,int ctrlIndex)
{
	CControllerPQLog *pController  = dynamic_cast<CControllerPQLog *>(pSkinningInfo->m_pControllers[ctrlIndex].get());
	CONTROLLER_CHUNK_DESC_0827 chunk;
	CHUNK_HEADER chunk_hdr;

	ZeroStruct(chunk);

	chunk_hdr.ChunkType = ChunkType_Controller;
	chunk_hdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0827::VERSION;

	assert(pController->m_arrTimes.size() == pController->m_arrKeys.size());

	int numKeys = pController->m_arrTimes.size();
	int nControllerId = pController->m_nControllerId;
	CryKeyPQLog* pData = new CryKeyPQLog[numKeys];

	chunk.numKeys = numKeys;
	chunk.nControllerId = nControllerId;

	for (int i=0; i<numKeys; i++)
	{
		PQLog& q = pController->m_arrKeys[i];

		pData[i].nTime = pController->m_arrTimes[i];		// what's the measure??  (at load: /TICKS_CONVERT)
		pData[i].vPos = q.vPos;					// at load: /100.0f (SCALE_TO_CGF ?)
		pData[i].vRotLog = q.vRotLog;
	}

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData(pData, numKeys*sizeof(CryKeyPQLog));

	return m_pChunkFile->AddChunk( chunk_hdr,chunkData.data,chunkData.size );
}

int CSaverCAF::SaveBoneNameList(CContentCGF *pCGF)
{
	CHUNK_HEADER chunk_hdr;

	ZeroStruct(chunk_hdr);
	chunk_hdr.ChunkType = ChunkType_BoneNameList;
	chunk_hdr.ChunkVersion = BONENAMELIST_CHUNK_DESC_0745::VERSION;

	BONENAMELIST_CHUNK_DESC_0745 hdr;
	ZeroStruct(hdr);

	int numBones = pCGF->GetSkinningInfo()->m_arrBonesDesc.size();
	hdr.numEntities = numBones;
	int dataSize = 0;
	for (int i=0; i<numBones; i++)
	{
		int len = strlen(pCGF->GetSkinningInfo()->m_arrBonesDesc[i].m_arrBoneName);
		dataSize += (len + 1);
	}
	dataSize++;			// the last string terminated with double 0

	char *pData = new char[dataSize];
	char *p = pData;

	for (int i=0; i<numBones; i++)
	{
		strcpy(p,pCGF->GetSkinningInfo()->m_arrBonesDesc[i].m_arrBoneName);
		int len = strlen(pCGF->GetSkinningInfo()->m_arrBonesDesc[i].m_arrBoneName);
		p += (len + 1);
	}
	*p = 0;
	p++;

	CChunkData chunkData;
	chunkData.Add(hdr);
	chunkData.AddData(pData, dataSize);

	return m_pChunkFile->AddChunk( chunk_hdr,chunkData.data,chunkData.size );
}