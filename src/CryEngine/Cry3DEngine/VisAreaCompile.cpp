////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   VisAreaCompile.cpp
//  Version:     v1.00
//  Created:     28/4/2005 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: visarea node loading/saving
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ObjMan.h"
#include "VisAreas.h"

#define VISAREA_NODE_CHUNK_VERSION 2

#define VISAREA_FLAG_OCEAN_VISIBLE 1
#define VISAREA_FLAG_IGNORE_SKY_COLOR 2
#define VISAREA_FLAG_AFFECTEDBYOUTLIGHTS 4
#define VISAREA_FLAG_SKYONLY 8
#define VISAREA_FLAG_DOUBLESIDE 16
#define VISAREA_FLAG_USEININDOORS 32

#define MAX_VIS_AREA_CONNECTIONS_NUM 30

struct SVisAreaChunk
{
	int		nChunkVersion;
	AABB	boxArea,boxStatics;
	char	sName[32];
	int		nObjectsBlockSize;
	int		arrConnectionsId[MAX_VIS_AREA_CONNECTIONS_NUM];
  uint32  dwFlags;
  uint32  dwReserved;
	Vec3	vConnNormals[2];
	float fHeight;
	Vec3  vAmbColor;
	float fViewDistRatio;

	AUTO_STRUCT_INFO_LOCAL
};

int CVisArea::GetData(byte * & pData, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
	if(m_pObjectsTree)
		m_pObjectsTree->CleanUpTree();

	if(pData)
	{
		// save node info
		SVisAreaChunk * pCunk = (SVisAreaChunk *)pData;
		pCunk->nChunkVersion = VISAREA_NODE_CHUNK_VERSION;
		pCunk->boxArea = m_boxArea;
		pCunk->boxStatics = m_boxStatics;
		strncpy(pCunk->sName, m_sName, sizeof(pCunk->sName));
		memcpy(pCunk->vConnNormals, m_vConnNormals, sizeof(pCunk->vConnNormals));
		pCunk->fHeight = m_fHeight;
		pCunk->vAmbColor = m_vAmbientColor;
		pCunk->fViewDistRatio = m_fViewDistRatio;
    
    pCunk->dwFlags = 0;
    if(m_bOceanVisible)
      pCunk->dwFlags |= VISAREA_FLAG_OCEAN_VISIBLE;   
    if(m_bIgnoreSky)
      pCunk->dwFlags |= VISAREA_FLAG_IGNORE_SKY_COLOR;
    if(m_bAffectedByOutLights)
      pCunk->dwFlags |= VISAREA_FLAG_AFFECTEDBYOUTLIGHTS;
    if(m_bSkyOnly)
      pCunk->dwFlags |= VISAREA_FLAG_SKYONLY;
    if(m_bDoubleSide)
      pCunk->dwFlags |= VISAREA_FLAG_DOUBLESIDE;
    if(m_bUseInIndoors)
      pCunk->dwFlags |= VISAREA_FLAG_USEININDOORS;

    pCunk->dwReserved = 0;

		// transform connections id into pointers
		PodArray<CVisArea*> & rAreas = IsPortal() ? GetVisAreaManager()->m_lstVisAreas : GetVisAreaManager()->m_lstPortals;
		for(int i=0; i<MAX_VIS_AREA_CONNECTIONS_NUM; i++)
			pCunk->arrConnectionsId[i] = -1;
		for(int i=0; i<m_lstConnections.Count() && i<MAX_VIS_AREA_CONNECTIONS_NUM; i++)
		{
			IVisArea * pArea = m_lstConnections[i];
			int nId;
			for(nId=0; nId<rAreas.Count(); nId++)
			{
				if(pArea == rAreas[nId])
					break;
			}

			if(nId<rAreas.Count())
				pCunk->arrConnectionsId[i] = nId;
			else
			{
				pCunk->arrConnectionsId[i] = -1;
				assert(!"Undefined connction");
			}
		}

		UPDATE_PTR_AND_SIZE(pData,nDataSize,sizeof(SVisAreaChunk));

		// save shape points num
		int nPointsCount = m_lstShapePoints.Count();
    SwapEndian(nPointsCount, eEndian);
		memcpy(pData, &nPointsCount, sizeof(nPointsCount)); 
		UPDATE_PTR_AND_SIZE(pData,nDataSize,sizeof(nPointsCount));

		// save shape points
		memcpy(pData, m_lstShapePoints.GetElements(), m_lstShapePoints.GetDataSize()); 
    SwapEndian((Vec3*)pData, m_lstShapePoints.Count(), eEndian);
		UPDATE_PTR_AND_SIZE(pData,nDataSize,m_lstShapePoints.GetDataSize());

		// save objects
		pCunk->nObjectsBlockSize = 0;

		// get data from objects tree
		if(m_pObjectsTree)
		{
			byte * pTmp = NULL;
			m_pObjectsTree->GetData(pTmp, pCunk->nObjectsBlockSize, NULL, NULL, eEndian, pExportInfo);
			m_pObjectsTree->GetData(pData, nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo); // UPDATE_PTR_AND_SIZE is inside
		}

    SwapEndian(*pCunk, eEndian);
	}
	else // just count size
	{
		nDataSize += sizeof(SVisAreaChunk);

		nDataSize += sizeof(int);
		nDataSize += m_lstShapePoints.GetDataSize();

		if(m_pObjectsTree)
		{
			int nObjectsBlockSize = 0;
			m_pObjectsTree->GetData(pData, nObjectsBlockSize, NULL, NULL, eEndian, pExportInfo);
			nDataSize += nObjectsBlockSize;
		}
	}

	return true;
}

int CVisArea::Load(byte * & f, int & nDataSizeLeft, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
  return Load_T(f, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pExportInfo);
}

int CVisArea::Load(FILE * & f, int & nDataSizeLeft, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
  return Load_T(f, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pExportInfo);
}

template <class T>
int CVisArea::Load_T(T * & f, int & nDataSizeLeft, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
  SVisAreaChunk chunk;
  if(!CTerrain::LoadDataFromFile(&chunk, 1, f, nDataSizeLeft, eEndian))
    return 0;

	assert(chunk.nChunkVersion == VISAREA_NODE_CHUNK_VERSION);
	if(chunk.nChunkVersion != VISAREA_NODE_CHUNK_VERSION)
		return 0;

	// get area info
	m_boxArea = chunk.boxArea;
	m_boxStatics = chunk.boxStatics;
  chunk.sName[sizeof(chunk.sName)-1]=0;
	strncpy_s(m_sName, chunk.sName, sizeof(m_sName));		
	m_bThisIsPortal = strstr(m_sName,"portal") != 0;
  m_bIgnoreSky = (strstr(m_sName,"ignoresky") != 0) || ((chunk.dwFlags & VISAREA_FLAG_IGNORE_SKY_COLOR) != 0);
	memcpy(m_vConnNormals, chunk.vConnNormals, sizeof(m_vConnNormals));
	m_fHeight = chunk.fHeight;
	m_vAmbientColor = chunk.vAmbColor;
	m_fViewDistRatio = chunk.fViewDistRatio;
  
  if(chunk.dwFlags == uint32(-1))
    chunk.dwFlags = 0;
  m_bOceanVisible = (chunk.dwFlags & VISAREA_FLAG_OCEAN_VISIBLE) != 0;

  m_bAffectedByOutLights = (chunk.dwFlags & VISAREA_FLAG_AFFECTEDBYOUTLIGHTS) != 0; 
  m_bSkyOnly = (chunk.dwFlags & VISAREA_FLAG_SKYONLY) != 0; 
  m_bDoubleSide = (chunk.dwFlags & VISAREA_FLAG_DOUBLESIDE) != 0;
  m_bUseInIndoors = (chunk.dwFlags & VISAREA_FLAG_USEININDOORS) != 0;

	// convert connections id into pointers
	PodArray<CVisArea*> & rAreas = IsPortal() ? GetVisAreaManager()->m_lstVisAreas : GetVisAreaManager()->m_lstPortals;
	for(int i=0; i<MAX_VIS_AREA_CONNECTIONS_NUM && rAreas.Count(); i++)
	{
    assert(chunk.arrConnectionsId[i] < rAreas.Count());
		if(chunk.arrConnectionsId[i] >= 0)
			m_lstConnections.Add(rAreas[chunk.arrConnectionsId[i]]);
	}

  { // get shape points
    int nPointsCount = 0;
    if(!CTerrain::LoadDataFromFile(&nPointsCount, 1, f, nDataSizeLeft, eEndian))
      return 0;

    // get shape points
    m_lstShapePoints.PreAllocate(nPointsCount,nPointsCount);
    if(!CTerrain::LoadDataFromFile(m_lstShapePoints.GetElements(), nPointsCount, f, nDataSizeLeft, eEndian))
      return 0;
  }

  // mark tree as invalid since new visarea was just added
  SAFE_DELETE(GetVisAreaManager()->m_pAABBTree);

  AABB * pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  // load content of objects tree
	if(!m_bEditor && chunk.nObjectsBlockSize>4)
	{
		int nCurDataSize = nDataSizeLeft;
    if (nCurDataSize > 0)
    {
      if(!m_pObjectsTree)
      {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
        m_pObjectsTree = new COctreeNode(-1, m_boxArea, this);
      }
      m_pObjectsTree->Load(f, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pBox);
      assert(nDataSizeLeft == (nCurDataSize-chunk.nObjectsBlockSize));
    }
	}

  UpdateOcclusionFlagInTerrain();

	return true;
}

const AABB* CVisArea::GetAABBox() const
{
	return &m_boxArea;
}


#include "TypeInfo_impl.h"
#include "VisAreaCompile_info.h"


