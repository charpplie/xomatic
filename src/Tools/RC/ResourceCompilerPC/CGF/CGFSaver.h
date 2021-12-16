////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CGFSaver.h
//  Version:     v1.00
//  Created:     7/11/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CGFSaver_h__
#define __CGFSaver_h__
#pragma once
 
#include "../MeshCompiler/MeshCompiler.h"
#include "ChunkFile.h"
#include "CGFContent.h"
#include "..\CGA\Controller.h"
#include "..\CGA\ControllerPQ.h"
#include "..\CGA\ControllerPQLog.h"
#include "LoaderCAF.h"


//////////////////////////////////////////////////////////////////////////
class CSaverCGF
{
public:
	CSaverCGF( const char *filename,CChunkFile &chunkFile );

	void SaveContent( CContentCGF *pCGF, bool bSwapEndian, bool bNeedCompressVertices, bool bUseQtangents );
	void SetContent( CContentCGF *pCGF ) { m_pCGF = pCGF; };

	// Enable/Disable saving of the node mesh.
	void SetMeshDataSaving( bool bEnable );

	// Store nodes in chunk file.
	void SaveNodes(bool bSwapEndian, bool bNeedCompressVertices, bool bUseQtangents, CInternalSkinningInfo* pSkinningInfo = NULL);
	void SaveMaterials(bool bSwapEndian);
	int SaveExportFlags();

	// Return node chunk id.
	int SaveNode( CNodeCGF *pNode, bool bSwapEndian, bool bNeedCompressVertices, bool bUseQtangents, CInternalSkinningInfo* pSkinningInfo );
	int SaveMaterial( CMaterialCGF *pMtl, bool bNeedSwap );

	// compiled chunks for characters
	int SaveCompiledBones( void *pData,int nSize, int version );
	int SaveCompiledPhysicalBones( void *pData,int nSize, int version );
	int SaveCompiledPhysicalProxis( void *pData,int nSize, uint32 numIntMorphTargets, int version );
	int SaveCompiledMorphTargets( void *pData,int nSize, uint32 numIntMorphTargets, int version );
	int SaveCompiledIntFaces( void *pData,int nSize, int version );
	int SaveCompiledIntSkinVertices( void *pData,int nSize, int version );
	int SaveCompiledExt2IntMap( void *pData,int nSize, int version );
	int SaveCompiledBoneBox( void *pData,int nSize, int version );

	// chunks for characters (for collada to cgf export) -----------------
	int SaveBones(void* pData, int numBones, int nSize);
	int SaveBoneNames(char* boneList, int numBones, int listSize);
	int SaveBoneInitialMatrices(SBoneInitPosMatrix* matrices, int numBones, int nSize);
	int SaveBoneMesh(PhysicalProxy& proxy);

	void SaveUncompiledNodes();
	int SaveUncompiledNode( CNodeCGF *pNode );
	void SaveUncompiledMorphTargets();
	// -------------------------------------------------------------------

	int SaveBreakablePhysics( bool bNeedEndianSwap);

	int SaveSpeedInfo( void *pData,int nSize );
	int SaveSpeedInfo1( void *pData,int nSize );
	int SaveSpeedInfo2( void *pData,int nSize );
	int SaveFootPlantInfo(void *pData,int nSize );
	int SaveTimeStamp(DWORD timeStamp);
	int SaveController(int nKeys, int nControllerId, CryKeyPQLog* pData,int nSize );
	int SaveController829(CONTROLLER_CHUNK_DESC_0829& pCtrlChunk,void* pData,int nSize );
	int SaveController830(CONTROLLER_CHUNK_DESC_0830& pCtrlChunk,void* pData,int nSize );
	int SaveControllerDB900(CONTROLLER_CHUNK_DESC_0900& pCtrlChunk,void* pData,int nSize );
	int SaveController901(CONTROLLER_CHUNK_DESC_0901& pCtrlChunk,void* pData,int nSize );
	int SaveController902(CONTROLLER_CHUNK_DESC_0902& pCtrlChunk,void* pData,int nSize );
	int SaveControllerDB903(CONTROLLER_CHUNK_DESC_0903& pCtrlChunk,void* pData,int nSize );
	int SaveControllerDB904(CONTROLLER_CHUNK_DESC_0904& pCtrlChunk,void* pData,int nSize );

	int SaveFoliage();

	int SaveTiming(CInternalSkinningInfo* pSkinningInfo);

private:
	// Return mesh chunk id.
	
	int SaveNodeMesh( CNodeCGF *pNode, bool bSwapEndian, bool bNeedCompressVertices, bool bUseQTangents );
	int SaveHelperChunk( CNodeCGF *pNode, bool bSwapEndian );
	int SaveMeshSubsetsChunk( CMesh &mesh, bool bSwapEndian, bool bNeedCompressVertices   );
	int SaveStreamDataChunk( void *pStreamData,int nStreamType,int nCount,int nElemSize, bool bSwapEndian);
	int SavePhysicalDataChunk( void *pData,int nSize, bool bSwapEndian );

	// uncompiled nodes ---------------------------------
	int SaveUncompiledNodeMesh( CNodeCGF *pNode );
	int SaveUncompiledHelperChunk( CNodeCGF *pNode );

	void GetBoneLinkInfo(SMeshBoneMapping& boneMapping, int link, int& boneID, float& weight);
	// --------------------------------------------------

	int SaveTCB3Track(CInternalSkinningInfo* pSkinningInfo,int trackIndex);
	int SaveTCBQTrack(CInternalSkinningInfo* pSkinningInfo,int trackIndex);

private:
	string m_filename;
	
	CChunkFile *m_pChunkFile;
	CContentCGF *m_pCGF;
	std::set<CNodeCGF*> m_savedNodes;
	std::set<CMaterialCGF*> m_savedMaterials;
	std::map<CMesh*,int> m_mapMeshToChunk;
	bool m_bDoNotSaveMeshData;
};

#endif //__CGFSaver_h__
