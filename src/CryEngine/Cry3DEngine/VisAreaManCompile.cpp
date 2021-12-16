////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   visareamancompile.cpp
//  Version:     v1.00
//  Created:     15/04/2005 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: check vis
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ObjMan.h"
#include "VisAreas.h"

bool CVisAreaManager::GetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
  float fStartTime = GetCurAsyncTimeSec();

  bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
  bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);

//  PrintMessage("Exporting indoor data (%s, %.2f MB) ...",
  //  (bHMap && bObjs) ? "Objects and heightmap" : (bHMap ? "Heightmap" : (bObjs ? "Objects" : "Nothing")), ((float)nDataSize)/1024.f/1024.f);

	// write header
	SVisAreaManChunkHeader * pVisAreaManagerChunkHeader = (SVisAreaManChunkHeader *)pData;
	pVisAreaManagerChunkHeader->nVersion = VISAREAMANAGER_CHUNK_VERSION;
  pVisAreaManagerChunkHeader->nDummy = 0;
  pVisAreaManagerChunkHeader->nFlags = (eEndian == eBigEndian)? SERIALIZATION_FLAG_BIG_ENDIAN : 0;
  pVisAreaManagerChunkHeader->nFlags2 = 0;
	pVisAreaManagerChunkHeader->nChunkSize = nDataSize;
	
	pVisAreaManagerChunkHeader->nVisAreasNum = m_lstVisAreas.Count();
	pVisAreaManagerChunkHeader->nPortalsNum = m_lstPortals.Count();
	pVisAreaManagerChunkHeader->nOcclAreasNum = m_lstOcclAreas.Count();

  SwapEndian(*pVisAreaManagerChunkHeader,eEndian);

	UPDATE_PTR_AND_SIZE(pData,nDataSize,sizeof(SVisAreaManChunkHeader));

	for(int i=0; i<m_lstVisAreas.Count(); i++)
		m_lstVisAreas[i]->GetData(pData, nDataSize, *ppStatObjTable, *ppMatTable, eEndian, pExportInfo);

	for(int i=0; i<m_lstPortals.Count(); i++)
		m_lstPortals[i]->GetData(pData, nDataSize, *ppStatObjTable, *ppMatTable, eEndian, pExportInfo);

	for(int i=0; i<m_lstOcclAreas.Count(); i++)
		m_lstOcclAreas[i]->GetData(pData, nDataSize, *ppStatObjTable, *ppMatTable, eEndian, pExportInfo);

  SAFE_DELETE(*ppStatObjTable);
  SAFE_DELETE(*ppMatTable);

  PrintMessagePlus(" done in %.2f sec", GetCurAsyncTimeSec()-fStartTime );

	assert(nDataSize==0);
	return nDataSize==0;
}

int CVisAreaManager::GetCompiledDataSize(SHotUpdateInfo * pExportInfo)
{
	int nDataSize = 0;
	byte * pData = NULL;

	// get header size
	nDataSize += sizeof(SVisAreaManChunkHeader);

	for(int i=0; i<m_lstVisAreas.Count(); i++)
		m_lstVisAreas[i]->GetData(pData, nDataSize, NULL, NULL, eLittleEndian, pExportInfo);

	for(int i=0; i<m_lstPortals.Count(); i++)
		m_lstPortals[i]->GetData(pData, nDataSize, NULL, NULL, eLittleEndian, pExportInfo);

	for(int i=0; i<m_lstOcclAreas.Count(); i++)
		m_lstOcclAreas[i]->GetData(pData, nDataSize, NULL, NULL, eLittleEndian, pExportInfo);

	return nDataSize;
}

bool CVisAreaManager::Load(FILE * & f, int & nDataSize, struct SVisAreaManChunkHeader * pVisAreaManagerChunkHeader, std::vector<struct IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable)
{
  return Load_T(f, nDataSize, pVisAreaManagerChunkHeader, pStatObjTable, pMatTable, false, NULL);
}

bool CVisAreaManager::SetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo)
{
  SVisAreaManChunkHeader * pChunkHeader = (SVisAreaManChunkHeader *)pData;
  pData += sizeof(SVisAreaManChunkHeader);
//  nDataSize -= sizeof(SVisAreaManChunkHeader);

  SwapEndian(*pExportInfo, (pChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian);

  bool bRes = Load_T(pData, nDataSize, pChunkHeader, *ppStatObjTable, *ppMatTable, bHotUpdate, pExportInfo);

  if(ppStatObjTable)
    SAFE_DELETE(*ppStatObjTable);
  if(ppMatTable)
    SAFE_DELETE(*ppMatTable);

  return bRes;
}

void CVisAreaManager::UnregisterEngineObjectsInArea(const SHotUpdateInfo * pExportInfo, PodArray<IRenderNode *> & arrUnregisteredObjects, bool bOnlyEngineObjects)
{
  for(int i=0; i<m_lstVisAreas.Count(); i++)
    if(m_lstVisAreas[i]->m_pObjectsTree)
      m_lstVisAreas[i]->m_pObjectsTree->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);

  for(int i=0; i<m_lstPortals.Count(); i++)
    if(m_lstPortals[i]->m_pObjectsTree)
      m_lstPortals[i]->m_pObjectsTree->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);

  for(int i=0; i<m_lstOcclAreas.Count(); i++)
    if(m_lstOcclAreas[i]->m_pObjectsTree)
      m_lstOcclAreas[i]->m_pObjectsTree->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);
}

void CVisAreaManager::OnVisAreaDeleted(IVisArea * pArea)
{
  for (int i = 0,num = m_lstCallbacks.size(); i < num; i++)
    m_lstCallbacks[i]->OnVisAreaDeleted(pArea);

  m_lstActiveOcclVolumes.Delete((CVisArea*)pArea);
  m_lstIndoorActiveOcclVolumes.Delete((CVisArea*)pArea);
  m_lstActiveEntransePortals.Delete((CVisArea*)pArea);
}

template <class T>
bool CVisAreaManager::Load_T(T * & f, int & nDataSize, SVisAreaManChunkHeader * pVisAreaManagerChunkHeader, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo)
{
	if( pVisAreaManagerChunkHeader->nVersion != VISAREAMANAGER_CHUNK_VERSION )
	{ Error("CVisAreaManager::SetCompiledData: verison of file is %d, expected version is %d", pVisAreaManagerChunkHeader->nVersion, (int)VISAREAMANAGER_CHUNK_VERSION); return 0; }

  if( pVisAreaManagerChunkHeader->nChunkSize != nDataSize )
  { Error("CVisAreaManager::SetCompiledData: data size mismuch (%d != %d)", pVisAreaManagerChunkHeader->nChunkSize, nDataSize); return 0; }

  bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
  bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);
  AABB * pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  EEndian eEndian = (pVisAreaManagerChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;

  PodArray<IRenderNode *> arrUnregisteredObjects;
  UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, true);

  PodArray<IRenderNode *> arrUnregisteredEntities;
  UnregisterEngineObjectsInArea(NULL, arrUnregisteredEntities, false);

  for(int i=0; i<m_lstVisAreas.Count(); i++)
    delete m_lstVisAreas[i];

  for(int i=0; i<m_lstPortals.Count(); i++)
    delete m_lstPortals[i];

  for(int i=0; i<m_lstOcclAreas.Count(); i++)
    delete m_lstOcclAreas[i];

  SAFE_DELETE(m_pAABBTree);
  m_pCurArea = m_pCurPortal = 0;

  { // construct areas
	  m_lstVisAreas.PreAllocate(pVisAreaManagerChunkHeader->nVisAreasNum,pVisAreaManagerChunkHeader->nVisAreasNum);
	  m_lstPortals.PreAllocate(pVisAreaManagerChunkHeader->nPortalsNum,pVisAreaManagerChunkHeader->nPortalsNum);
	  m_lstOcclAreas.PreAllocate(pVisAreaManagerChunkHeader->nOcclAreasNum,pVisAreaManagerChunkHeader->nOcclAreasNum);

    nDataSize -= sizeof(SVisAreaManChunkHeader);

//    if(bHotUpdate)
  //    PrintMessage("Importing indoor data (%s, %.2f MB) ...",
    //  (bHMap && bObjs) ? "Objects and heightmap" : (bHMap ? "Heightmap" : (bObjs ? "Objects" : "Nothing")), ((float)nDataSize)/1024.f/1024.f);

	  for(int i=0; i<m_lstVisAreas.Count(); i++)
		  m_lstVisAreas[i] = new CVisArea();
	  for(int i=0; i<m_lstPortals.Count(); i++)
		  m_lstPortals[i] = new CVisArea();
	  for(int i=0; i<m_lstOcclAreas.Count(); i++)
		  m_lstOcclAreas[i] = new CVisArea();
  }

  { // load areas content
	  for(int i=0; i<m_lstVisAreas.Count(); i++)
		  m_lstVisAreas[i]->Load(f,nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);

	  for(int i=0; i<m_lstPortals.Count(); i++)
		  m_lstPortals[i]->Load(f,nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);

	  for(int i=0; i<m_lstOcclAreas.Count(); i++)
		  m_lstOcclAreas[i]->Load(f,nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);
  }

  for(int i=0; i<arrUnregisteredObjects.Count(); i++)
    arrUnregisteredObjects[i]->ReleaseNode();
  arrUnregisteredObjects.Reset();

  for(int i=0; i<arrUnregisteredEntities.Count(); i++)
    Get3DEngine()->RegisterEntity(arrUnregisteredEntities[i]);
  arrUnregisteredEntities.Reset();

  SAFE_DELETE(m_pAABBTree);
  m_pCurArea = m_pCurPortal = 0;
  UpdateConnections();

	return nDataSize==0;
}

