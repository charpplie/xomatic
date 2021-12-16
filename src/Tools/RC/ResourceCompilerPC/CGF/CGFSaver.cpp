////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CGFSaver.cpp
//  Version:     v1.00
//  Created:     7/11/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CGFSaver.h"
#include "AnimSaver.h"

using namespace mesh_compiler;

#define SCALE_TO_CGF 100.0f

struct CChunkData
{
	char *data;
	int size;

	//////////////////////////////////////////////////////////////////////////
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

template<class T>
void SwapEndianChunkWrite(T& chunk)
{
  SwapEndian(chunk, true);
  SwapEndian(chunk.chdr, true);
	assert(chunk.chdr.GetEndian() == eLittleEndian);
}

//////////////////////////////////////////////////////////////////////////
CSaverCGF::CSaverCGF( const char *filename,CChunkFile &chunkFile )
{
	assert(m_pChunkFile);
	m_filename = filename;
	m_pChunkFile = &chunkFile;
	m_bDoNotSaveMeshData = false;
}

//////////////////////////////////////////////////////////////////////////
void CSaverCGF::SetMeshDataSaving( bool bEnable )
{
	m_bDoNotSaveMeshData = !bEnable;
}

//////////////////////////////////////////////////////////////////////////
void CSaverCGF::SaveContent( CContentCGF *pCGF, bool bSwapEndian, bool bNeedCompressVertices, bool bUseQtangents )
{
	SetContent( pCGF );

	SaveExportFlags();
	SaveMaterials(bSwapEndian);
	SaveNodes(bSwapEndian, bNeedCompressVertices, bUseQtangents, NULL);
	SaveBreakablePhysics(bSwapEndian);
	SaveFoliage();
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveSpeedInfo( void *pData,int nSize )
{
	CHUNK_HEADER chunk;

	ZeroStruct(chunk);
	chunk.ChunkType = ChunkType_SpeedInfo;
	chunk.ChunkVersion = SPEED_CHUNK_DESC::VERSION;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveSpeedInfo1( void *pData,int nSize )
{
	CHUNK_HEADER chunk;

	ZeroStruct(chunk);
	chunk.ChunkType = ChunkType_SpeedInfo;
	chunk.ChunkVersion = SPEED_CHUNK_DESC_1::VERSION;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveSpeedInfo2( void *pData,int nSize )
{
	CHUNK_HEADER chunk;

	ZeroStruct(chunk);
	chunk.ChunkType = ChunkType_SpeedInfo;
	chunk.ChunkVersion = SPEED_CHUNK_DESC_2::VERSION;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk,chunkData.data,chunkData.size );
}


//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveFootPlantInfo(void *pData,int nSize )
{
	CHUNK_HEADER chunk;

	ZeroStruct(chunk);
	chunk.ChunkType = ChunkType_FootPlantInfo;
	chunk.ChunkVersion = FOOTPLANT_INFO::VERSION;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveTimeStamp(DWORD timeStamp)
{
		CHUNK_HEADER chunk;

		ZeroStruct(chunk);
		chunk.ChunkType = ChunkType_Timestamp;
		chunk.ChunkVersion = TIMESTAMP_INFO_VERSION;

		CChunkData chunkData;
		chunkData.Add(chunk);
		chunkData.AddData( &timeStamp, sizeof(DWORD) );

		return m_pChunkFile->AddChunk( chunk,chunkData.data,chunkData.size );

}


//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledBones( void *pData,int nSize, int version )
{
	COMPILED_BONE_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_CompiledBones;
	chunk.chdr.ChunkVersion = version;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledPhysicalBones( void *pData,int nSize, int version )
{
	COMPILED_PHYSICALBONE_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_CompiledPhysicalBones;
	chunk.chdr.ChunkVersion = version;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledPhysicalProxis( void *pData,int nSize, uint32 numPhysicalProxies, int version  )
{
	COMPILED_PHYSICALPROXY_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType		= ChunkType_CompiledPhysicalProxies;
	chunk.chdr.ChunkVersion = version;
	chunk.numPhysicalProxies	=	numPhysicalProxies;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledMorphTargets( void *pData,int nSize, uint32 numMorphTargets, int version )
{
	COMPILED_MORPHTARGETS_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType		= ChunkType_CompiledMorphTargets;
	chunk.chdr.ChunkVersion = version;
	chunk.numMorphTargets		=	numMorphTargets;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}



//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledIntSkinVertices( void *pData,int nSize, int version )
{
	COMPILED_INTSKINVERTICES_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_CompiledIntSkinVertices;
	chunk.chdr.ChunkVersion = version;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledIntFaces( void *pData,int nSize, int version )
{
	COMPILED_INTFACES_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_CompiledIntFaces;
	chunk.chdr.ChunkVersion = version;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledBoneBox( void *pData,int nSize, int version )
{
	COMPILED_INTFACES_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_BonesBoxes;
	chunk.chdr.ChunkVersion = version;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
	
}


//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveCompiledExt2IntMap( void *pData,int nSize, int version )
{
	COMPILED_EXT2INTMAP_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_CompiledExt2IntMap;
	chunk.chdr.ChunkVersion = version;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveBones(void* pData, int numBones, int nSize)
{
	BONEANIM_CHUNK_DESC chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_BoneAnim;
	chunk.chdr.ChunkVersion = BONEANIM_CHUNK_DESC::VERSION;
	chunk.nBones = numBones;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveBoneNames(char* boneList, int numBones, int listSize)
{
	CHUNK_HEADER hdr;
	hdr.ChunkType = ChunkType_BoneNameList;
	hdr.ChunkVersion = BONENAMELIST_CHUNK_DESC_0745::VERSION;

	BONENAMELIST_CHUNK_DESC_0745 chunk;
	ZeroStruct(chunk);
	chunk.numEntities = numBones;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData(boneList, listSize);

	return m_pChunkFile->AddChunk( hdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveBoneInitialMatrices(SBoneInitPosMatrix* matrices, int numBones, int nSize)
{
	CHUNK_HEADER hdr;
	hdr.ChunkType  = ChunkType_BoneInitialPos;
	hdr.ChunkVersion = BONEINITIALPOS_CHUNK_DESC_0001::VERSION;

	BONEINITIALPOS_CHUNK_DESC_0001 chunk;
	ZeroStruct(chunk);
	chunk.numBones = numBones;

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData(matrices, nSize);

	return m_pChunkFile->AddChunk( hdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveBoneMesh(PhysicalProxy& proxy)
{
	//TODO: are these flags okay?
	bool HasBoneInfo = false;
	bool HasVertexColors = false;
	bool HasVertexAlpha = false;
	bool WriteVCol = false;

	// uncompiled mesh chunk
	MESH_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_BoneMesh;
	chunk.chdr.ChunkVersion = MESH_CHUNK_DESC::VERSION;
	
	int numVertices = proxy.m_arrPoints.size();
	int numFaces = proxy.m_arrMaterials.size();

	chunk.flags1 = 0;
	chunk.flags2 = 0;
	chunk.flags1 |= HasBoneInfo ? MESH_CHUNK_DESC::FLAG1_BONE_INFO : 0;
	chunk.flags2 |= (WriteVCol && HasVertexColors) ? MESH_CHUNK_DESC::FLAG2_HAS_VERTEX_COLOR : 0;
	chunk.flags2 |= (WriteVCol && HasVertexAlpha) ? MESH_CHUNK_DESC::FLAG2_HAS_VERTEX_ALPHA : 0;
	chunk.nFaces = numFaces;					// pMesh->FaceCount();
	chunk.nTVerts = 0;						// pMesh->UVCount();
	chunk.nVerts = numVertices;					// pMesh->VertexCount();
	chunk.VertAnimID = -1;						// nVertexAnimChunkID;

	// add chunk members
	CChunkData chunkData;
	chunkData.Add(chunk);

	// copy positions and vertices to a CryVertex array -----------------------------------
	CryVertex* vertices = new CryVertex[numVertices];
	for (int i=0; i<numVertices; i++)
	{
		vertices[i].p = proxy.m_arrPoints[i] * 100.0f;
		vertices[i].n = Vec3(ZERO);
	}
	chunkData.AddData(vertices, numVertices*sizeof(CryVertex));
	SAFE_DELETE(vertices);

	// copy faces to a CryFace array ------------------------------------------------------
	CryFace* faces = new CryFace[numFaces];
	for (int i=0; i<numFaces; i++)
	{
		faces[i].v0 = proxy.m_arrIndices[i * 3 + 0];
		faces[i].v1 = proxy.m_arrIndices[i * 3 + 1];
		faces[i].v2 = proxy.m_arrIndices[i * 3 + 2];
		faces[i].MatID = proxy.m_arrMaterials[i];
		faces[i].SmGroup = 0;
	}
	chunkData.AddData(faces, numFaces*sizeof(CryFace));
	SAFE_DELETE(faces);

	int nMeshChunkId = m_pChunkFile->AddChunk(chunk.chdr,chunkData.data,chunkData.size);

	return nMeshChunkId;
}

//////////////////////////////////////////////////////////////////////////
void CSaverCGF::SaveNodes(bool bSwapEndian, bool bNeedCompressVertices, bool bUseQtangents, CInternalSkinningInfo* pSkinningInfo)
{
	m_savedNodes.clear();
	uint32 numNodes = m_pCGF->GetNodeCount();
	for (int i=0; i<numNodes; i++)
	{
		CNodeCGF *pNode = m_pCGF->GetNode(i);
		const char* pNodeName = pNode->name;
		// Check if not yet saved.
		uint32 numSavedNodes = m_savedNodes.size();
		//	if (m_savedNodes.find(pNode) == m_savedNodes.end())
		SaveNode( pNode, bSwapEndian, bNeedCompressVertices, bUseQtangents,	pSkinningInfo );

		if (pNode->bHasFaceMap)
		{
			FACEMAP_CHUNK_DESC chunkMap;
			ZeroStruct(chunkMap);
			chunkMap.chdr.ChunkType = ChunkType_FaceMap;
			chunkMap.chdr.ChunkVersion = FACEMAP_CHUNK_DESC::VERSION;
			strcpy(chunkMap.nodeName,pNodeName);
			chunkMap.StreamID = SaveStreamDataChunk(&pNode->mapFaceToFace0[0], CGF_STREAM_FACEMAP, 
				pNode->pMesh->GetIndexCount()/3, sizeof(pNode->mapFaceToFace0[0]), bSwapEndian);
			m_pChunkFile->AddChunk( chunkMap.chdr,&chunkMap,sizeof(chunkMap) );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSaverCGF::SaveUncompiledNodes()
{
	m_savedNodes.clear();
	uint32 numNodes = m_pCGF->GetNodeCount();
	for (int i=0; i<numNodes; i++)
	{
		CNodeCGF *pNode = m_pCGF->GetNode(i);
		SaveUncompiledNode(pNode);

		// FACEMAP_CHUNK_DESC is a compiled chunk
		/*
		if (pNode->bHasFaceMap)
		{
			FACEMAP_CHUNK_DESC chunkMap;
			ZeroStruct(chunkMap);
			chunkMap.chdr.ChunkType = ChunkType_FaceMap;
			chunkMap.chdr.ChunkVersion = FACEMAP_CHUNK_DESC::VERSION;
			strcpy(chunkMap.nodeName,pNodeName);
			chunkMap.StreamID = SaveStreamDataChunk(&pNode->mapFaceToFace0[0], CGF_STREAM_FACEMAP, 
				pNode->pMesh->GetIndexCount()/3, sizeof(pNode->mapFaceToFace0[0]));
			m_pChunkFile->AddChunk( chunkMap.chdr,&chunkMap,sizeof(chunkMap) );
		}
		*/
	}
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveNode( CNodeCGF *pNode,bool bSwapEndian, bool bNeedCompressVertices, bool bUseQtangents,
													CInternalSkinningInfo* pSkinningInfo )
{

	const char* pNodeName = pNode->name;

	typedef std::set<CNodeCGF*> MySet;

	for( MySet::iterator it( m_savedNodes.begin() ); it != m_savedNodes.end(); ++it )
	{
		if( pNode == *it )
		{
			return pNode->nChunkId;
		}
	}

	m_savedNodes.insert(pNode);

	NODE_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Node;
 	chunk.chdr.ChunkVersion = NODE_CHUNK_DESC::VERSION;
  if (bSwapEndian)
    chunk.chdr.ChunkVersion |= CONSOLE_VERSION_MASK;

	for (int i = 0; i < m_pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pOtherNode = m_pCGF->GetNode(i);
		if (pOtherNode->pParent == pNode)
			chunk.nChildren++;
	}

	strcpy_s( chunk.name,pNode->name );
	chunk.pos = pNode->pos * SCALE_TO_CGF;
	chunk.rot = pNode->rot;
	chunk.scl = pNode->scl;

	// Set matrix to node chunk.
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

	if (pNode->pMaterial)
	{
		chunk.MatID = pNode->pMaterial->nChunkId;
	}

	chunk.ObjectID = -1;
	chunk.ParentID = -1;
	if (pNode->pParent)
	{
		pNode->nParentChunkId = SaveNode( pNode->pParent, bSwapEndian, bNeedCompressVertices, bUseQtangents, pSkinningInfo );
		chunk.ParentID = pNode->nParentChunkId;
	}
	if (pNode->type == CNodeCGF::NODE_MESH)
	{
		pNode->nObjectChunkId = SaveNodeMesh( pNode, bSwapEndian, bNeedCompressVertices, bUseQtangents );
	}
	if (pNode->type == CNodeCGF::NODE_HELPER && pNode->helperType == HP_GEOMETRY)
	{
		pNode->nObjectChunkId = SaveNodeMesh( pNode, bSwapEndian, bNeedCompressVertices, bUseQtangents );
	}
	else if (pNode->type == CNodeCGF::NODE_HELPER)
	{
		pNode->nObjectChunkId = SaveHelperChunk( pNode, bSwapEndian );
	}
	else if (pNode->type == CNodeCGF::NODE_LIGHT)
	{
		// Save Light chunk
	}

	int nextChunk = m_pChunkFile->NumChunks();

	int positionIndex = pNode->pos_cont_id;
	int rotationIndex = pNode->rot_cont_id;
	int scaleIndex = pNode->scl_cont_id;

	int controllerIndex = -1;
	if (pSkinningInfo)
	{
		// Get the node index.
		for (int i = 0; i < m_pCGF->GetNodeCount(); i++)
		{
			CNodeCGF *pOtherNode = m_pCGF->GetNode(i);
			if (pOtherNode == pNode)
			{
				controllerIndex = 3*i;
				break;
			}
		}

		assert(controllerIndex > -1);

		if (pSkinningInfo->m_arrControllers[controllerIndex].type == 0x55)
		{
			positionIndex = nextChunk+1;
			nextChunk++;
		}

		if (pSkinningInfo->m_arrControllers[controllerIndex+1].type == 0xaa)
		{
			rotationIndex = nextChunk+1;
			nextChunk++;
		}

		if (pSkinningInfo->m_arrControllers[controllerIndex+2].type == 0x55)
		{
			scaleIndex = nextChunk+1;
			nextChunk++;
		}
	}

	chunk.pos_cont_id = positionIndex;
	chunk.rot_cont_id = rotationIndex;
	chunk.scl_cont_id = scaleIndex;

	chunk.ObjectID = pNode->nObjectChunkId;
	chunk.PropStrLen = pNode->properties.length();

  int PropLen = chunk.PropStrLen;
	if (bSwapEndian)
		SwapEndianChunkWrite(chunk);

	CChunkData chunkData;
	chunkData.Add(chunk);
	// Copy property string
	chunkData.AddData( pNode->properties.c_str(), PropLen );

	pNode->nChunkId = m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );

	if (pSkinningInfo)
	{
		if (positionIndex != -1)
		{
			SaveTCB3Track(pSkinningInfo,pSkinningInfo->m_arrControllers[controllerIndex].index);
		}
		if (rotationIndex != -1)
		{
			SaveTCBQTrack(pSkinningInfo,pSkinningInfo->m_arrControllers[controllerIndex+1].index);
		}
		if (scaleIndex != -1)
		{
			SaveTCB3Track(pSkinningInfo,pSkinningInfo->m_arrControllers[controllerIndex+2].index);
		}
	}

	return pNode->nChunkId;
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveUncompiledNode( CNodeCGF *pNode )
{
	const char* pNodeName = pNode->name;

	typedef std::set<CNodeCGF*> MySet;

	for ( MySet::iterator it( m_savedNodes.begin() ); it != m_savedNodes.end(); ++it )
		if (pNode == *it)
			return pNode->nChunkId;

	m_savedNodes.insert(pNode);

	NODE_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Node;
	chunk.chdr.ChunkVersion = NODE_CHUNK_DESC::VERSION;

	for (int i = 0; i < m_pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pOtherNode = m_pCGF->GetNode(i);
		if (pOtherNode->pParent == pNode)
			chunk.nChildren++;
	}

	strcpy_s( chunk.name,pNode->name );
	chunk.pos = pNode->pos*SCALE_TO_CGF;
	chunk.rot = pNode->rot;
	chunk.scl = pNode->scl;

	chunk.pos_cont_id = pNode->pos_cont_id;
	chunk.rot_cont_id = pNode->rot_cont_id;
	chunk.scl_cont_id = pNode->scl_cont_id;

	// Set matrix to node chunk.
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
	pMat[12] = pNode->localTM.GetTranslation().x*SCALE_TO_CGF;
	pMat[13] = pNode->localTM.GetTranslation().y*SCALE_TO_CGF;
	pMat[14] = pNode->localTM.GetTranslation().z*SCALE_TO_CGF;

	if (pNode->pMaterial)
	{
		chunk.MatID = pNode->pMaterial->nChunkId;
	}

	chunk.ObjectID = -1;
	chunk.ParentID = -1;
	if (pNode->pParent)
	{
		pNode->nParentChunkId = SaveUncompiledNode( pNode->pParent );
		chunk.ParentID = pNode->nParentChunkId;
	}
	if (pNode->type == CNodeCGF::NODE_MESH && pNode->pMesh && pNode->pMesh->GetFacesCount() > 0)
	{
		pNode->nObjectChunkId = SaveUncompiledNodeMesh( pNode );
	}
	if (pNode->type == CNodeCGF::NODE_HELPER && pNode->helperType == HP_GEOMETRY && pNode->pMesh)
	{
		pNode->nObjectChunkId = SaveUncompiledNodeMesh( pNode );
	}
	else if (pNode->type == CNodeCGF::NODE_HELPER)
	{
		pNode->nObjectChunkId = SaveUncompiledHelperChunk( pNode );
	}
	else if (pNode->type == CNodeCGF::NODE_LIGHT)
	{
		// Save Light chunk
	}

	chunk.ObjectID = pNode->nObjectChunkId;
	chunk.PropStrLen = pNode->properties.length();

	CChunkData chunkData;
	chunkData.Add(chunk);
	// Copy property string
	chunkData.AddData( pNode->properties.c_str(),chunk.PropStrLen );

	pNode->nChunkId = m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
	return pNode->nChunkId;
}


void CSaverCGF::SaveUncompiledMorphTargets()
{
	CSkinningInfo* skinningInfo = (m_pCGF ? m_pCGF->GetSkinningInfo() : 0);
	for (int morphIndex = 0, morphCount = int(skinningInfo ? skinningInfo->m_arrMorphTargets.size() : 0); morphIndex < morphCount; ++morphIndex)
	{
		MorphTargets* morph = skinningInfo->m_arrMorphTargets[morphIndex];

		CHUNK_HEADER hdr;
		hdr.ChunkType = ChunkType_MeshMorphTarget;
		hdr.ChunkVersion = MESHMORPHTARGET_CHUNK_DESC_0001::VERSION;

		MESHMORPHTARGET_CHUNK_DESC_0001 chunk;
		ZeroStruct(chunk);
		chunk.nChunkIdMesh = -1; // TODO: Save this properly!
		chunk.numMorphVertices = (skinningInfo ? int(morph->m_arrIntMorph.size()) : 0);

		CChunkData chunkData;
		chunkData.Add(chunk);
		if (morph->m_arrIntMorph.size())
			chunkData.AddData(&morph->m_arrIntMorph[0], int(morph->m_arrIntMorph.size() * sizeof(SMeshMorphTargetVertex)));
		chunkData.AddData(morph->m_strName.c_str(), int(morph->m_strName.size() + 1));

		m_pChunkFile->AddChunk( hdr,chunkData.data,chunkData.size );
	}
}

void ConvertToQTangents(SMeshTangents* pTangents, SMeshQTangents* pResult, size_t count )
{
  for (int x = 0; x < count ; ++x) 
  {
    Vec3 tangent  = Vec3( tPackB2F(pTangents[x].Tangent.x), tPackB2F(pTangents[x].Tangent.y), tPackB2F(pTangents[x].Tangent.z) );
    Vec3 binormal	= Vec3( tPackB2F(pTangents[x].Binormal.x), tPackB2F(pTangents[x].Binormal.y),	tPackB2F(pTangents[x].Binormal.z) );
    if (tangent.GetLength()==0) tangent=Vec3(1,0,0);
    if (binormal.GetLength()==0) binormal=Vec3(0,1,0);
    tangent.Normalize();
    binormal.Normalize();

    //create a tangent-matrix with a perfect right-handed orthonormal base 
    Vec3 normal = (tangent%binormal).GetNormalized();
    Vec3 middle = (tangent+binormal).GetNormalized();
    Vec3 ortho_tangent  =	(middle+(middle%normal)).GetNormalized();
    Vec3 ortho_binormal = (middle+(normal%middle)).GetNormalized();
    //store it into a matrix33 and convert it into quaternion  
    Matrix33 m33;
    m33.SetRow(0,ortho_tangent );
    m33.SetRow(1,ortho_binormal);
    m33.SetRow(2,normal );
    assert(m33.IsOrthonormalRH());
    Quat quat=Quat(m33);

    if (quat.w==0)
    {
      Vec3 axis = quat.v.GetNormalized();
      quat=Quat::CreateRotationAA(0.001f,axis)*quat;
    };
    assert(quat.w); 

    assert(quat.w);
    //make sure the w-value is always positive  
    if (quat.w<0.0f) quat=-quat;
    //flip the quaternion, if we have a reflection  
    if (pTangents[x].Tangent.w<0)	quat=-quat;
    //store it
    pResult[x].TangentBinormal.x	=  tPackF2B(quat.v.x);
    pResult[x].TangentBinormal.y	=  tPackF2B(quat.v.y);
    pResult[x].TangentBinormal.z	=  tPackF2B(quat.v.z);
    pResult[x].TangentBinormal.w	=  tPackF2B(quat.w);		
  }
}
/*

*/

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveNodeMesh( CNodeCGF *pNode, bool bSwapEndian, bool bNeedCompressVertices, bool bUseQTangents )
{
	if (pNode->pMesh)
	{
		if (m_mapMeshToChunk.find(pNode->pMesh) != m_mapMeshToChunk.end())
		{
			// This mesh already saved.
			return m_mapMeshToChunk.find(pNode->pMesh)->second;
		}
	}

	MESH_CHUNK_DESC_0800 chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Mesh;
  chunk.chdr.ChunkVersion = MESH_CHUNK_DESC_0800::VERSION;
  if (bSwapEndian)
    chunk.chdr.ChunkVersion |= CONSOLE_VERSION_MASK;

	if (pNode->pMesh)
	{
		CMesh &mesh = *pNode->pMesh;

		int nVerts = mesh.GetVertexCount();
		int nIndices = mesh.GetIndexCount();

		chunk.nVerts = nVerts;
		chunk.nIndices = nIndices;
		chunk.nSubsets = mesh.GetSubSetCount();
		chunk.bboxMin = mesh.m_bbox.min;
		chunk.bboxMax = mesh.m_bbox.max;
	}
	else
	{
		chunk.nVerts = pNode->meshInfo.nVerts;
		chunk.nIndices = pNode->meshInfo.nIndices;
		chunk.nSubsets = pNode->meshInfo.nSubsets;
		chunk.bboxMax = pNode->meshInfo.bboxMin;
		chunk.bboxMax = pNode->meshInfo.bboxMax;
	}

	bool bEmptyMesh = m_bDoNotSaveMeshData || pNode->bPhysicsProxy || !pNode->pMesh;

	if (bEmptyMesh)
	{
		chunk.nFlags |= MESH_CHUNK_DESC_0800::MESH_EMPTY;
	}
	chunk.nFlags2 = pNode->nPhysicalizeFlags;

	for (int i = 0; i < 4; i++)
	{
		if (!pNode->physicalGeomData[i].empty()) {
			chunk.nPhysicsDataChunkId[i] = SavePhysicalDataChunk( &pNode->physicalGeomData[i][0],pNode->physicalGeomData[i].size(), bSwapEndian );
		}
	}

	if (!bEmptyMesh)
	{
		CMesh &mesh = *pNode->pMesh;
		chunk.nSubsetsChunkId = SaveMeshSubsetsChunk( mesh, bSwapEndian, bNeedCompressVertices );

		if (mesh.m_pPositions) {
			if (bNeedCompressVertices) {

				Vec3 * pRealData = (Vec3*)mesh.m_pPositions;
				std::vector<Vec3f16> tmpVec(mesh.GetVertexCount());
				for (size_t i = 0, end = tmpVec.size(); i < end; ++i, ++pRealData) {
					tmpVec[i] = Vec3f16(pRealData->x, pRealData->y, pRealData->z);
					SwapEndian(tmpVec[i], bSwapEndian);
				}

				chunk.nStreamChunkID[CGF_STREAM_POSITIONS] = SaveStreamDataChunk( &tmpVec[0],CGF_STREAM_POSITIONS,mesh.GetVertexCount(),sizeof(Vec3f16), bSwapEndian);
			}
			else {
				SwapEndian(mesh.m_pPositions, mesh.GetVertexCount(), bSwapEndian);
				chunk.nStreamChunkID[CGF_STREAM_POSITIONS] = SaveStreamDataChunk( mesh.m_pPositions,CGF_STREAM_POSITIONS,mesh.GetVertexCount(),sizeof(Vec3), bSwapEndian);
			}
		}

		if (mesh.m_pNorms) {
			if (bNeedCompressVertices) {
				Vec3 * pRealData = (Vec3*)mesh.m_pNorms;
				std::vector<Vec3f16> tmpVec(mesh.GetVertexCount());
				for (size_t i = 0, end = tmpVec.size(); i < end; ++i, ++pRealData) {
					tmpVec[i] = Vec3f16(pRealData->x, pRealData->y, pRealData->z);
					SwapEndian(tmpVec[i], bSwapEndian);
				}

				chunk.nStreamChunkID[CGF_STREAM_NORMALS] = SaveStreamDataChunk( &tmpVec[0],CGF_STREAM_NORMALS,mesh.GetVertexCount(),sizeof(Vec3f16), bSwapEndian );
			} else {
				SwapEndian(mesh.m_pNorms, mesh.GetVertexCount(), bSwapEndian);
				chunk.nStreamChunkID[CGF_STREAM_NORMALS] = SaveStreamDataChunk( mesh.m_pNorms,CGF_STREAM_NORMALS,mesh.GetVertexCount(),sizeof(Vec3), bSwapEndian );
			}
		}

		if (mesh.m_pTexCoord) {
			SwapEndian(mesh.m_pTexCoord, mesh.GetTexCoordsCount(), bSwapEndian);
			chunk.nStreamChunkID[CGF_STREAM_TEXCOORDS] = SaveStreamDataChunk( mesh.m_pTexCoord,CGF_STREAM_TEXCOORDS,mesh.GetTexCoordsCount(),sizeof(CryUV), bSwapEndian  );
		}

		if (mesh.m_pColor0)
		{
			SwapEndian(mesh.m_pColor0, mesh.GetVertexCount(), bSwapEndian); 
			chunk.nStreamChunkID[CGF_STREAM_COLORS] = SaveStreamDataChunk( mesh.m_pColor0,CGF_STREAM_COLORS,mesh.GetVertexCount(),sizeof(SMeshColor), bSwapEndian  );
		}
		if (mesh.m_pColor1)
		{
			SwapEndian(mesh.m_pColor1, mesh.GetVertexCount(), bSwapEndian); 
			chunk.nStreamChunkID[CGF_STREAM_COLORS2] = SaveStreamDataChunk( mesh.m_pColor1,CGF_STREAM_COLORS2,mesh.GetVertexCount(),sizeof(SMeshColor), bSwapEndian  );
		}
		if (mesh.m_pIndices) {
			SwapEndian(mesh.m_pIndices, mesh.GetIndexCount(), bSwapEndian);
			chunk.nStreamChunkID[CGF_STREAM_INDICES] = SaveStreamDataChunk( mesh.m_pIndices,CGF_STREAM_INDICES,mesh.GetIndexCount(),sizeof(uint16), bSwapEndian  );
		}

		if (mesh.m_pTangents) {

			if (bUseQTangents) {
				std::vector<SMeshQTangents> qTangents(mesh.GetVertexCount());
				ConvertToQTangents(mesh.m_pTangents, &qTangents[0], mesh.GetVertexCount());
				SwapEndian(&qTangents[0], mesh.GetVertexCount(), bSwapEndian); 
				chunk.nStreamChunkID[CGF_STREAM_TANGENTS] = SaveStreamDataChunk( &qTangents[0],CGF_STREAM_QTANGENTS,mesh.GetVertexCount(),sizeof(SMeshQTangents), bSwapEndian  );
			} else {
				SwapEndian(mesh.m_pTangents, mesh.GetVertexCount(), bSwapEndian); 
				chunk.nStreamChunkID[CGF_STREAM_TANGENTS] = SaveStreamDataChunk( mesh.m_pTangents,CGF_STREAM_TANGENTS,mesh.GetVertexCount(),sizeof(SMeshTangents), bSwapEndian  );
			}
		}

		if (mesh.m_pShapeDeformation) {
			SwapEndian(mesh.m_pShapeDeformation, mesh.GetVertexCount(), bSwapEndian);
			chunk.nStreamChunkID[CGF_STREAM_SHAPEDEFORMATION] = SaveStreamDataChunk( mesh.m_pShapeDeformation,CGF_STREAM_SHAPEDEFORMATION,mesh.GetVertexCount(),sizeof(SMeshShapeDeformation), bSwapEndian  );
		}

		if (mesh.m_pBoneMapping) {
			SwapEndian(mesh.m_pBoneMapping, mesh.GetVertexCount(), bSwapEndian);
			chunk.nStreamChunkID[CGF_STREAM_BONEMAPPING]			= SaveStreamDataChunk( mesh.m_pBoneMapping,CGF_STREAM_BONEMAPPING,mesh.GetVertexCount(),sizeof(SMeshBoneMapping), bSwapEndian  );
		}

		if (mesh.m_pSHInfo && mesh.m_pSHInfo->pSHCoeffs) {
			/*SwapMeshData(mesh.m_pSHInfo->pSHCoeffs, mesh.GetVertexCount(), bSwapEndian);*/
			chunk.nStreamChunkID[CGF_STREAM_SHCOEFFS] = SaveStreamDataChunk( mesh.m_pSHInfo->pSHCoeffs,CGF_STREAM_SHCOEFFS,mesh.GetVertexCount(),sizeof(SMeshSHCoeffs), bSwapEndian  );
		}

		if (pNode->pSkinInfo) {
			SwapEndian(pNode->pSkinInfo, mesh.GetVertexCount()+1, bSwapEndian);
			chunk.nStreamChunkID[CGF_STREAM_SKINDATA] = SaveStreamDataChunk( pNode->pSkinInfo, CGF_STREAM_SKINDATA, mesh.GetVertexCount()+1, sizeof(pNode->pSkinInfo[0]), bSwapEndian );
		}
	}

	if (bSwapEndian)
		SwapEndianChunkWrite(chunk);

	int nMeshChunkId = m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
	m_mapMeshToChunk[pNode->pMesh] = nMeshChunkId;
	return nMeshChunkId;
} 

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveUncompiledNodeMesh( CNodeCGF *pNode )
{
	if (m_mapMeshToChunk.find(pNode->pMesh) != m_mapMeshToChunk.end())
	{
		// This mesh already saved.
		return m_mapMeshToChunk.find(pNode->pMesh)->second;
	}

	CMesh* mesh = pNode->pMesh;

	//TODO: are these flags okay?
	bool HasBoneInfo = mesh->m_pBoneMapping ? true : false;
	bool HasVertexColors = mesh->m_pColor0 ? true : false;		// pMesh->HasVertexColours()
	bool HasVertexAlpha = HasVertexColors;						// pMesh->HasVertexAlpha()
	bool WriteVCol = true;										// pExportFlags->ShouldWriteVertexColours();

	// uncompiled mesh chunk
	MESH_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Mesh;
	chunk.chdr.ChunkVersion = MESH_CHUNK_DESC::VERSION;
	
	int numVertices = mesh->GetVertexCount();
	int numFaces = mesh->GetFacesCount();
	int numUVs = mesh->GetTexCoordsCount();

	assert(mesh->m_pPositions);
	assert(mesh->m_pNorms);
	assert(mesh->m_pFaces);

	chunk.flags1 = 0;
	chunk.flags2 = 0;
	chunk.flags1 |= HasBoneInfo ? MESH_CHUNK_DESC::FLAG1_BONE_INFO : 0;
	chunk.flags2 |= (WriteVCol && HasVertexColors) ? MESH_CHUNK_DESC::FLAG2_HAS_VERTEX_COLOR : 0;
	chunk.flags2 |= (WriteVCol && HasVertexAlpha) ? MESH_CHUNK_DESC::FLAG2_HAS_VERTEX_ALPHA : 0;
	chunk.nFaces = numFaces;					// pMesh->FaceCount();
	chunk.nTVerts = numUVs;						// pMesh->UVCount();
	chunk.nVerts = numVertices;					// pMesh->VertexCount();
	chunk.VertAnimID = -1;						// nVertexAnimChunkID;

	// add chunk members
	CChunkData chunkData;
	chunkData.Add(chunk);

	// copy positions and vertices to a CryVertex array -----------------------------------
	CryVertex* vertices = new CryVertex[numVertices];
	for (int i=0; i<numVertices; i++)
	{
		vertices[i].p = mesh->m_pPositions[i];
		vertices[i].n = mesh->m_pNorms[i];
	}
	chunkData.AddData(vertices, numVertices*sizeof(CryVertex));
	SAFE_DELETE(vertices);

	// copy faces to a CryFace array ------------------------------------------------------
	CryFace* faces = new CryFace[numFaces];
	for (int i=0; i<numFaces; i++)
	{
		faces[i].v0 = mesh->m_pFaces[i].v[0];
		faces[i].v1 = mesh->m_pFaces[i].v[1];
		faces[i].v2 = mesh->m_pFaces[i].v[2];
		faces[i].MatID = mesh->m_subsets[mesh->m_pFaces[i].nSubset].nMatID;
		faces[i].SmGroup = 0;		//TODO: convert normal vectors to smoothing groups! vazze !#@^&**@^
	}
	chunkData.AddData(faces, numFaces*sizeof(CryFace));
	SAFE_DELETE(faces);

	// copy texture coordinates to a CryUV array ------------------------------------------
	if (numUVs)
	{
		CryUV* uvs = new CryUV[numUVs];
		for (int i=0; i<numUVs; i++)
		{
			uvs[i].u = mesh->m_pTexCoord[i].s;
			uvs[i].v = mesh->m_pTexCoord[i].t;
		}
		chunkData.AddData(uvs, numUVs*sizeof(CryUV));
		SAFE_DELETE(uvs);

		CryTexFace* textfaces = new CryTexFace[numFaces];
		for (int i=0; i<numFaces; i++)
		{
			textfaces[i].t0 = mesh->m_pFaces[i].t[0];
			textfaces[i].t1 = mesh->m_pFaces[i].t[1];
			textfaces[i].t2 = mesh->m_pFaces[i].t[2];
		}
		chunkData.AddData(textfaces, numFaces*sizeof(CryTexFace));
		SAFE_DELETE(textfaces);
	}

	if (HasBoneInfo)
	{
		CSkinningInfo* skinningInfo = m_pCGF->GetSkinningInfo();
		Matrix34 objectTransform = pNode->worldTM;

		for (int k=0; k<numVertices; k++)
		{
			std::vector<CryLink> arrLinks;

			Vec3 point = mesh->m_pPositions[k];
			Vec3 worldVertex = objectTransform.TransformPoint(point);
			
			float totalweight = 0.0f;
			for (int j=0; j<4; j++)
			{
				int boneID;
				float blending;
				
				GetBoneLinkInfo(mesh->m_pBoneMapping[k],j,boneID,blending);
				if (blending < 0.01f)
					continue;

				totalweight += blending;

				const Matrix34& boneTransform = skinningInfo->m_arrBonesDesc[boneID].m_DefaultW2B;
				Vec3 offset = boneTransform.TransformPoint(worldVertex);

				static float const metersToCentimeters = 100.0f;

				CryLink link;
				link.BoneID = boneID;
				link.Blending = blending;
				link.offset = offset * metersToCentimeters;
				
				arrLinks.push_back(link);
			}
			int nLinks = arrLinks.size();

			for (int j=0; j<nLinks; j++)
				arrLinks[j].Blending /= totalweight;

			chunkData.AddData(&nLinks, sizeof(int));
			chunkData.AddData(&arrLinks[0], nLinks*sizeof(CryLink));
		}
	}

	if (WriteVCol)
	{
		if (HasVertexColors)
		{
			CryIRGB* vertexcolors = new CryIRGB[numVertices];
			for (int i=0; i<numVertices; i++)
			{
				vertexcolors[i].r = mesh->m_pColor0[i].r;
				vertexcolors[i].g = mesh->m_pColor0[i].g;
				vertexcolors[i].b = mesh->m_pColor0[i].b;
			}
			chunkData.AddData(vertexcolors, numVertices*sizeof(CryIRGB));
			SAFE_DELETE(vertexcolors);
		}

		if (HasVertexAlpha)
		{
			unsigned char* vertexalphas = new unsigned char[numVertices];
			for (int i=0; i<numVertices; i++)
			{
				vertexalphas[i] = mesh->m_pColor0[i].a;
			}
			chunkData.AddData(vertexalphas, numVertices*sizeof(unsigned char));
			SAFE_DELETE(vertexalphas);
		}
	}

	if (0)		// save morph targets - the loader load this?
	{

	}

	if (0)		// save bone initial pose - the loader load this?
	{

	}

	int nMeshChunkId = m_pChunkFile->AddChunk(chunk.chdr,chunkData.data,chunkData.size);
	m_mapMeshToChunk[pNode->pMesh] = nMeshChunkId;

	return nMeshChunkId;
}

void CSaverCGF::GetBoneLinkInfo(SMeshBoneMapping& boneMapping, int link, int& boneID, float& weight)
{
	switch (link)
	{
		case 0:
			{
				boneID = boneMapping.boneIDs.r;
				weight = (float)boneMapping.weights.r / 255.0f;
			}
			break;

		case 1:
			{
				boneID = boneMapping.boneIDs.g;
				weight = (float)boneMapping.weights.g / 255.0f;
			}
			break;

		case 2:
			{
				boneID = boneMapping.boneIDs.b;
				weight = (float)boneMapping.weights.b / 255.0f;
			}
			break;

		case 3:
			{
				boneID = boneMapping.boneIDs.a;
				weight = (float)boneMapping.weights.a / 255.0f;
			}
			break;

		default:
			assert(false);
	}
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveHelperChunk( CNodeCGF *pNode, bool bSwapEndian )
{
	HELPER_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Helper;

	chunk.type = pNode->helperType;
	chunk.size = pNode->helperSize;
  chunk.chdr.ChunkVersion = HELPER_CHUNK_DESC::VERSION;

  if (bSwapEndian) {
    chunk.chdr.ChunkVersion |= CONSOLE_VERSION_MASK;
    SwapEndianChunkWrite(chunk);
  }

	return m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveUncompiledHelperChunk( CNodeCGF *pNode )
{
	HELPER_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_Helper;
	chunk.chdr.ChunkVersion = HELPER_CHUNK_DESC::VERSION;

	chunk.type = pNode->helperType;
	chunk.size = pNode->helperSize;

	return m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
}

//////////////////////////////////////////////////////////////////////////
// TODO:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

int CSaverCGF::SaveBreakablePhysics(bool bNeedEndianSwap )
{
	CPhysicalizeInfoCGF * pPi = m_pCGF->GetPhysiclizeInfo();

	if(!pPi || pPi->nGranularity==-1)
		return 0;

	BREAKABLE_PHYSICS_CHUNK_DESC chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_BreakablePhysics;
	chunk.chdr.ChunkVersion = BREAKABLE_PHYSICS_CHUNK_DESC::VERSION;

	chunk.granularity = pPi->nGranularity;
	chunk.nMode = pPi->nMode;
	chunk.nRetVtx = pPi->nRetVtx;
	chunk.nRetTets = pPi->nRetTets;

	CChunkData chunkData;
	chunkData.Add(chunk);
	if (pPi->pRetVtx)
		chunkData.AddData( pPi->pRetVtx , pPi->nRetVtx * sizeof(Vec3) );
	if (pPi->pRetTets)
		chunkData.AddData( pPi->pRetTets , pPi->nRetTets * sizeof(int)*4 );

	if (bNeedEndianSwap)
		SwapEndianChunkWrite(chunk);

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}


//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveMeshSubsetsChunk( CMesh &mesh,  bool bSwapEndian, bool bNeedCompressVertices  )
{
	MESH_SUBSETS_CHUNK_DESC_0800 chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_MeshSubsets;
  chunk.chdr.ChunkVersion = MESH_SUBSETS_CHUNK_DESC_0800::VERSION;
  if (bSwapEndian)
    chunk.chdr.ChunkVersion |= CONSOLE_VERSION_MASK;

	chunk.nCount = mesh.GetSubSetCount();
	uint32 cbHasDecompressionMatrix = (mesh.m_pSHInfo && mesh.m_pSHInfo->nDecompressionCount == mesh.GetSubSetCount());	//if we have a decompression matrix available per subset, store it;
	chunk.nFlags |= (cbHasDecompressionMatrix ? MESH_SUBSETS_CHUNK_DESC_0800::SH_HAS_DECOMPR_MAT : 0);//flag it


	uint32 cbHasBoneIDs=0;
	for (int i = 0; i < mesh.GetSubSetCount(); i++)
	{
		SMeshSubset &srcSubset = mesh.m_subsets[i];
		cbHasBoneIDs|=srcSubset.m_arrGlobalBonesPerSubset.size();
	}
	if (cbHasBoneIDs)
		chunk.nFlags |=  MESH_SUBSETS_CHUNK_DESC_0800::BONEINDICES;

	if (bSwapEndian)
		SwapEndianChunkWrite(chunk);

	CChunkData chunkData;
	chunkData.Add(chunk);


	for (int i = 0; i < mesh.GetSubSetCount(); i++)
	{
		SMeshSubset &srcSubset = mesh.m_subsets[i];

		MESH_SUBSETS_CHUNK_DESC_0800::MeshSubset subset;
		subset.nFirstIndexId = srcSubset.nFirstIndexId;
		subset.nNumIndices = srcSubset.nNumIndices;
		subset.nFirstVertId = srcSubset.nFirstVertId;
		subset.nNumVerts = srcSubset.nNumVerts;
		subset.nMatID = srcSubset.nMatID;
		subset.fRadius = srcSubset.fRadius;
		subset.vCenter = srcSubset.vCenter;
		cbHasBoneIDs|=srcSubset.m_arrGlobalBonesPerSubset.size();
	  SwapEndian(subset, bSwapEndian);
		chunkData.Add(subset);
	}

	//add decompression matrices
	if(cbHasDecompressionMatrix)
	{
		for (int i = 0; i < mesh.GetSubSetCount(); i++)
			chunkData.AddData(&(mesh.m_pSHInfo->pDecompressions[i]), sizeof(SSHDecompressionMat));
	}

	//add boneindices
	if(cbHasBoneIDs)
	{
		for (int i = 0; i < mesh.GetSubSetCount(); i++)
		{
			SMeshSubset &srcSubset = mesh.m_subsets[i];
			MESH_SUBSETS_CHUNK_DESC_0800::MeshBoneIDs subset;
			subset.numBoneIDs = srcSubset.m_arrGlobalBonesPerSubset.size();
			for (uint32 b=0; b<subset.numBoneIDs; b++)
				subset.arrBoneIDs[b]=srcSubset.m_arrGlobalBonesPerSubset[b];
		  SwapEndian(subset, bSwapEndian);
			chunkData.AddData(&subset, sizeof(MESH_SUBSETS_CHUNK_DESC_0800::MeshBoneIDs));
		}
	}



	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveStreamDataChunk( void *pStreamData,int nStreamType,int nCount,int nElemSize, bool bSwapEndian)
{
	STREAM_DATA_CHUNK_DESC_0800 chunk;
	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_DataStream;

  chunk.chdr.ChunkVersion = STREAM_DATA_CHUNK_DESC_0800::VERSION;
  if (bSwapEndian)
    chunk.chdr.ChunkVersion |= CONSOLE_VERSION_MASK;

	chunk.nStreamType = nStreamType;
	chunk.nCount = nCount;
	chunk.nElementSize = nElemSize;


	if (bSwapEndian)
		SwapEndianChunkWrite(chunk);

	CChunkData chunkData;
	chunkData.Add(chunk);
  chunkData.AddData( pStreamData,nCount*nElemSize );

  
	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SavePhysicalDataChunk( void *pData,int nSize, bool bSwapEndian )
{
	MESH_PHYSICS_DATA_CHUNK_DESC_0800 chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_MeshPhysicsData;
  chunk.chdr.ChunkVersion = MESH_PHYSICS_DATA_CHUNK_DESC_0800::VERSION;
  if (bSwapEndian)
    chunk.chdr.ChunkVersion |= CONSOLE_VERSION_MASK;

	chunk.nDataSize = nSize;

	if (bSwapEndian)
		SwapEndianChunkWrite(chunk);

	CChunkData chunkData;
	chunkData.Add(chunk);
	chunkData.AddData( pData,nSize );

	return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveExportFlags()
{
	EXPORT_FLAGS_CHUNK_DESC chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_ExportFlags;
	chunk.chdr.ChunkVersion = EXPORT_FLAGS_CHUNK_DESC::VERSION;

	CExportInfoCGF *pExpInfo = m_pCGF->GetExportInfo();
	if (pExpInfo->bMergeAllNodes)
		chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::MERGE_ALL_NODES;
	if (pExpInfo->bHaveAutoLods)
		chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::HAVE_AUTO_LODS;
	
	if (pExpInfo->bFromColladaXSI)
		chunk.reserved[0] |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_XSI;
	if (pExpInfo->bFromColladaMAX)
		chunk.reserved[0] |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_MAX;

	enum {NUM_RC_VERSION_ELEMENTS = sizeof(pExpInfo->rc_version) / sizeof(pExpInfo->rc_version[0])};
	std::copy(pExpInfo->rc_version, pExpInfo->rc_version + NUM_RC_VERSION_ELEMENTS, chunk.rc_version);
	strncpy( chunk.rc_version_string,pExpInfo->rc_version_string,sizeof(chunk.rc_version_string) );

	return m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
}

//////////////////////////////////////////////////////////////////////////
void CSaverCGF::SaveMaterials(bool bSwapEndian)
{
	int i;
	for (i = 0; i < m_pCGF->GetNodeCount(); i++)
	{
		CMaterialCGF *pMaterialCGF = m_pCGF->GetNode(i)->pMaterial;
		if (pMaterialCGF)
		{
			SaveMaterial( pMaterialCGF, bSwapEndian );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveMaterial( CMaterialCGF *pMtl, bool bNeedSwap )
{
	// Check whether the material has already been saved.
	if (m_savedMaterials.find(pMtl) == m_savedMaterials.end())
	{
		m_savedMaterials.insert(pMtl);

		MTL_NAME_CHUNK_DESC_0800 chunk;
		ZeroStruct(chunk);
		chunk.chdr.ChunkType = ChunkType_MtlName;
    if (bNeedSwap)
		  chunk.chdr.ChunkVersion = MTL_NAME_CHUNK_DESC_0800::VERSION;
    else 
      chunk.chdr.ChunkVersion = MTL_NAME_CHUNK_DESC_0800::VERSION;

		strncpy( chunk.name,pMtl->name,sizeof(chunk.name) );
		chunk.name[sizeof(chunk.name)-1] = 0;

		chunk.nFlags = pMtl->nFlags;//(pMtl->subMaterials.empty() ? MTL_NAME_CHUNK_DESC_0800::FLAG_SUB_MATERIAL : MTL_NAME_CHUNK_DESC_0800::FLAG_MULTI_MATERIAL);
		chunk.sh_opacity = pMtl->shOpacity;
		chunk.nPhysicalizeType = pMtl->nPhysicalizeType;

		// Add the material now - it is important that the parent comes before the children,
		// since the loading code considers the first material encountered to be the name of
		// the .mtl file. However we have not finished setting up the chunk yet, so we will
		// have to update the data later.
		pMtl->nChunkId = m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );

		// Recurse to the child materials.
		chunk.nSubMaterials = int(pMtl->subMaterials.size());
		if (chunk.nSubMaterials > MAX_SUB_MATERIALS)
			chunk.nSubMaterials = MAX_SUB_MATERIALS;
		for (int childIndex = 0; childIndex < chunk.nSubMaterials; ++childIndex)
		{
			int chunkID = 0;
			CMaterialCGF* childMaterial = pMtl->subMaterials[childIndex];
			if (childMaterial)
				chunkID = this->SaveMaterial(childMaterial, bNeedSwap);
			chunk.nSubMatChunkId[childIndex] = chunkID;
		}

		if (bNeedSwap)
			SwapEndianChunkWrite(chunk);

		// Update the data for the parent chunk.
		m_pChunkFile->SetChunkData(pMtl->nChunkId, &chunk, sizeof(chunk));
	}

	return pMtl->nChunkId;
}


int CSaverCGF::SaveController(int nKeys, int nControllerId, CryKeyPQLog* pData,int nSize )
{

	CONTROLLER_CHUNK_DESC_0828 pCtrlChunk;

	
	ZeroStruct(pCtrlChunk);
	pCtrlChunk.chdr.ChunkType = ChunkType_Controller;
	pCtrlChunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0828::VERSION;

	pCtrlChunk.numKeys = nKeys;
	pCtrlChunk.nControllerId = nControllerId;

	
	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize * sizeof(CryKeyPQLog) );

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );



	return 1;
}

int CSaverCGF::SaveController829(CONTROLLER_CHUNK_DESC_0829& pCtrlChunk, void* pData,int nSize )
{

	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );

}

int CSaverCGF::SaveController830(CONTROLLER_CHUNK_DESC_0830& pCtrlChunk, void* pData,int nSize )
{

	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );

}

int CSaverCGF::SaveControllerDB900(CONTROLLER_CHUNK_DESC_0900& pCtrlChunk,void* pData,int nSize )
{

	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );

}

int CSaverCGF::SaveControllerDB903(CONTROLLER_CHUNK_DESC_0903& pCtrlChunk,void* pData,int nSize )
{

	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );

}

int CSaverCGF::SaveControllerDB904(CONTROLLER_CHUNK_DESC_0904& pCtrlChunk,void* pData,int nSize )
{

	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );

}

//STUPID. I know. Good place to some refactoring
int CSaverCGF::SaveController901(CONTROLLER_CHUNK_DESC_0901& pCtrlChunk,void* pData,int nSize )
{

	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );

}

int CSaverCGF::SaveController902(CONTROLLER_CHUNK_DESC_0902& pCtrlChunk,void* pData,int nSize )
{
	CChunkData chunkData;
	chunkData.Add(pCtrlChunk);
	chunkData.AddData( pData,nSize);

	return m_pChunkFile->AddChunk( pCtrlChunk.chdr,chunkData.data,chunkData.size );
}


//////////////////////////////////////////////////////////////////////////
int CSaverCGF::SaveFoliage()
{
	SFoliageInfoCGF &fi = *m_pCGF->GetFoliageInfo();
	FOLIAGE_INFO_CHUNK_DESC chunk;

	ZeroStruct(chunk);
	chunk.chdr.ChunkType = ChunkType_FoliageInfo;
	chunk.chdr.ChunkVersion = FOLIAGE_INFO_CHUNK_DESC::VERSION;

	if (fi.nSpines>0)
	{
		int i,j;
		chunk.nSpines = fi.nSpines;
		chunk.nSkinnedVtx = fi.nSkinnedVtx;
		chunk.nBoneIds = fi.chunkBoneIds.size();
		chunk.nSpineVtx = 0;
		for(i=0; i<fi.nSpines; chunk.nSpineVtx+=fi.pSpines[i++].nVtx);
		FOLIAGE_SPINE_SUB_CHUNK *pSpineBuf = new FOLIAGE_SPINE_SUB_CHUNK[fi.nSpines];
		Vec3 *pSpineVtx = new Vec3[chunk.nSpineVtx];
		Vec4 *pSpineSegDim = new Vec4[chunk.nSpineVtx];

		for(i=j=0; i<fi.nSpines; j+=fi.pSpines[i++].nVtx)
		{
			pSpineBuf[i].nVtx = fi.pSpines[i].nVtx;
			pSpineBuf[i].len = fi.pSpines[i].len;
			pSpineBuf[i].navg = fi.pSpines[i].navg;
			pSpineBuf[i].iAttachSpine = fi.pSpines[i].iAttachSpine+1;
			pSpineBuf[i].iAttachSeg = fi.pSpines[i].iAttachSeg+1;
			memcpy(pSpineVtx+j, fi.pSpines[i].pVtx, fi.pSpines[i].nVtx*sizeof(Vec3));
			memcpy(pSpineSegDim+j, fi.pSpines[i].pSegDim, fi.pSpines[i].nVtx*sizeof(Vec4));
		}

		CChunkData chunkData;
		chunkData.Add(chunk);
		chunkData.AddData( pSpineBuf, sizeof(pSpineBuf[0])*fi.nSpines );
		chunkData.AddData( pSpineVtx, sizeof(pSpineVtx[0])*chunk.nSpineVtx );
		chunkData.AddData( pSpineSegDim, sizeof(pSpineSegDim[0])*chunk.nSpineVtx );
		chunkData.AddData( fi.pBoneMapping, sizeof(fi.pBoneMapping[0])*fi.nSkinnedVtx );
		chunkData.AddData( &fi.chunkBoneIds[0], sizeof(fi.chunkBoneIds[0])*fi.chunkBoneIds.size() );

		delete[] pSpineSegDim; delete[] pSpineVtx; delete[] pSpineBuf;

		return m_pChunkFile->AddChunk( chunk.chdr,chunkData.data,chunkData.size );
	}	else
		return m_pChunkFile->AddChunk( chunk.chdr,&chunk,sizeof(chunk) );
}

int CSaverCGF::SaveTCB3Track(CInternalSkinningInfo* pSkinningInfo,int trackIndex)
{
	return CSaverAnim::SaveTCB3Track(m_pChunkFile, pSkinningInfo, trackIndex);
}

int CSaverCGF::SaveTCBQTrack(CInternalSkinningInfo* pSkinningInfo,int trackIndex)
{
	return CSaverAnim::SaveTCBQTrack(m_pChunkFile, pSkinningInfo, trackIndex);
}

int CSaverCGF::SaveTiming(CInternalSkinningInfo* pSkinningInfo)
{
	return CSaverAnim::SaveTiming(m_pChunkFile, pSkinningInfo);
}