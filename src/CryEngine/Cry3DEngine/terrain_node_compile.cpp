////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_node.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: terrain node
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "terrain_sector.h"
#include "terrain.h"
#include "ObjMan.h"
#include "VisAreas.h"

#define TERRAIN_NODE_CHUNK_VERSION 6

int CTerrainNode::Load(uint8 * & f, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo) { return Load_T(f, nDataSize, eEndian, pExportInfo); }
int CTerrainNode::Load(FILE * & f, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo) { return Load_T(f, nDataSize, eEndian, pExportInfo); }

int CTerrainNode::FTell(uint8 * & f) { return -1; }
int CTerrainNode::FTell(FILE * & f) { return GetPak()->FTell(f); }

template<class T> 
int CTerrainNode::Load_T(T * & f, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
  const AABB * pAreaBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  if(pAreaBox && !Overlap::AABB_AABB(GetBBox(), *pAreaBox))
    return 0;	

  // set node data
  STerrainNodeChunk chunk;
  if(!CTerrain::LoadDataFromFile(&chunk, 1, f, nDataSize, eEndian))
    return 0;
  assert(chunk.nChunkVersion == TERRAIN_NODE_CHUNK_VERSION || chunk.nChunkVersion == 5);
  if(chunk.nChunkVersion != TERRAIN_NODE_CHUNK_VERSION && chunk.nChunkVersion != 5)
    return 0;

  // set error levels, bounding boxes and some flags
  m_boxHeigtmapLocal = chunk.boxHeightmap;
  m_boxHeigtmapLocal.max.z = max(m_boxHeigtmapLocal.max.z, GetTerrain()->GetWaterLevel());
  m_bHasHoles = chunk.bHasHoles!=0;
  m_rangeInfo.fOffset = chunk.fOffset;
  m_rangeInfo.fRange = chunk.fRange;
  m_rangeInfo.nSize = chunk.nSize;
  SAFE_DELETE_ARRAY(m_rangeInfo.pHMData);
  m_rangeInfo.UpdateBitShift(GetTerrain()->m_nUnitsToSectorBitShift);
  m_rangeInfo.nModified = false;

  m_nNodeHMDataOffset = -1;
  if(m_rangeInfo.nSize)
  {
    m_rangeInfo.pHMData = new uint16[m_rangeInfo.nSize*m_rangeInfo.nSize];
    m_nNodeHMDataOffset = FTell(f);
    if(!CTerrain::LoadDataFromFile(m_rangeInfo.pHMData, m_rangeInfo.nSize*m_rangeInfo.nSize, f, nDataSize, eEndian))
      return 0;
    if(chunk.nChunkVersion != 5)
      CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);
  }

  // load LOD errors
  delete [] m_pGeomErrors;
  m_pGeomErrors = new float[GetTerrain()->m_nUnitsToSectorBitShift];
  if(!CTerrain::LoadDataFromFile(m_pGeomErrors, GetTerrain()->m_nUnitsToSectorBitShift, f, nDataSize, eEndian))
    return 0;
  assert(m_pGeomErrors[0] == 0);

  // load used surf types
  for(int i=0; i<m_lstSurfaceTypeInfo.Count(); i++)
    m_lstSurfaceTypeInfo[i].DeleteRenderMeshes(GetRenderer());
  m_lstSurfaceTypeInfo.Clear();
  m_lstSurfaceTypeInfo.PreAllocate(chunk.nSurfaceTypesNum,chunk.nSurfaceTypesNum);
  uint8 * pTypes = new uint8[chunk.nSurfaceTypesNum];
  if(!CTerrain::LoadDataFromFile(pTypes, chunk.nSurfaceTypesNum, f, nDataSize, eEndian))
    return 0;

  if(chunk.nChunkVersion != 5)
    CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);

  for(int i=0; i<m_lstSurfaceTypeInfo.Count() && i<MAX_SURFACE_TYPES_COUNT; i++)
    m_lstSurfaceTypeInfo[i].pSurfaceType = &GetTerrain()->m_SSurfaceType[m_nSID][min(pTypes[i],MAX_SURFACE_TYPES_COUNT-1)];

  delete [] pTypes;

  // count number of nodes saved
  int nNodesNum = 1;

  // process childs
  for(int i=0; m_arrChilds[0] && i<4; i++)
    nNodesNum += m_arrChilds[i]->Load_T(f, nDataSize, eEndian, pExportInfo);

  return nNodesNum;
}

int CTerrainNode::ReloadModifiedHMData(FILE * f)
{
  if(m_rangeInfo.nSize && m_nNodeHMDataOffset>=0 && m_rangeInfo.nModified)
  {
    m_rangeInfo.nModified = false;

    GetPak()->FSeek(f, m_nNodeHMDataOffset, SEEK_SET);
    int nDataSize = m_rangeInfo.nSize*m_rangeInfo.nSize*sizeof(m_rangeInfo.pHMData[0]);
    if(!CTerrain::LoadDataFromFile(m_rangeInfo.pHMData, m_rangeInfo.nSize*m_rangeInfo.nSize, f, nDataSize, m_pTerrain->m_eEndianOfTexture))
      return 0;
  }

  // process childs
  for(int i=0; m_arrChilds[0] && i<4; i++)
    m_arrChilds[i]->ReloadModifiedHMData(f);

  return 1;
}

int CTerrainNode::GetData(byte * & pData, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
  const AABB * pAreaBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  AABB boxWS = GetBBox();

  if(pAreaBox && !Overlap::AABB_AABB(boxWS, *pAreaBox))
    return 0;	

	if(pData)
	{
		// get node data
		STerrainNodeChunk * pCunk = (STerrainNodeChunk *)pData;
		pCunk->nChunkVersion = TERRAIN_NODE_CHUNK_VERSION;
		pCunk->boxHeightmap = boxWS;
		pCunk->bHasHoles = m_bHasHoles;	
		pCunk->fOffset = m_rangeInfo.fOffset;
		pCunk->fRange = m_rangeInfo.fRange;
		pCunk->nSize = m_rangeInfo.nSize;
		pCunk->nSurfaceTypesNum = m_lstSurfaceTypeInfo.Count();

    SwapEndian(*pCunk, eEndian);
		UPDATE_PTR_AND_SIZE(pData,nDataSize,sizeof(STerrainNodeChunk));

		// get heightmap data
    AddToPtr(pData,nDataSize,m_rangeInfo.pHMData,m_rangeInfo.nSize*m_rangeInfo.nSize,eEndian,true);
//		memcpy(pData, m_rangeInfo.pHMData, nBlockSize);
	//	UPDATE_PTR_AND_SIZE(pData,nDataSize,nBlockSize);

		// get heightmap error data
    AddToPtr(pData,nDataSize,m_pGeomErrors,GetTerrain()->m_nUnitsToSectorBitShift,eEndian);
//		memcpy(pData, m_pGeomErrors, GetTerrain()->m_nUnitsToSectorBitShift*sizeof(m_pGeomErrors[0]));
	//	UPDATE_PTR_AND_SIZE(pData,nDataSize,GetTerrain()->m_nUnitsToSectorBitShift*sizeof(m_pGeomErrors[0]));

		// get used surf types
		uint8 arrTmp[MAX_TERRAIN_SURFACE_TYPES];
		for(int i=0; i<MAX_TERRAIN_SURFACE_TYPES && i<m_lstSurfaceTypeInfo.Count(); i++)
			arrTmp[i] = m_lstSurfaceTypeInfo[i].pSurfaceType->ucThisSurfaceTypeId;

    AddToPtr(pData,nDataSize,arrTmp,m_lstSurfaceTypeInfo.Count(),eEndian,true);
//		memcpy(pData, arrTmp, m_lstSurfaceTypeInfo.Count()*sizeof(uint8));
	//	UPDATE_PTR_AND_SIZE(pData,nDataSize,m_lstSurfaceTypeInfo.Count()*sizeof(uint8));
  }
	else // just count size
	{
		nDataSize += sizeof(STerrainNodeChunk);
		nDataSize += m_rangeInfo.nSize*m_rangeInfo.nSize*sizeof(m_rangeInfo.pHMData[0]);
    while(nDataSize&3)
      nDataSize++;
		nDataSize += GetTerrain()->m_nUnitsToSectorBitShift*sizeof(m_pGeomErrors[0]);
		nDataSize += m_lstSurfaceTypeInfo.Count()*sizeof(uint8);
    while(nDataSize&3)
      nDataSize++;
	}

	// count number of nodes saved
	int nNodesNum = 1;

	// process childs
	for(int i=0; m_arrChilds[0] && i<4; i++)
		nNodesNum += m_arrChilds[i]->GetData(pData, nDataSize, eEndian, pExportInfo);

	return nNodesNum;
}

