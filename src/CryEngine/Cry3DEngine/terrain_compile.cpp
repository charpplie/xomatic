////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_compile.cpp
//  Version:     v1.00
//  Created:     15/04/2005 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: check vis
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "ObjectsTree.h"

#define SIGC_ALIGNTOTERRAIN		    (1<<0)
#define SIGC_USETERRAINCOLOR	    (1<<1)
#define SIGC_AFFECTED_BY_VOXELS   (1<<2)
#define SIGC_HIDEABILITY          (1<<3)
#define SIGC_HIDEABILITYSECONDARY (1<<4)
#define SIGC_PICKABLE             (1<<5)
#define SIGC_COMPLEXBENDING       (1<<6)
#define SIGC_CASTSHADOW           (1<<7)
#define SIGC_RECVSHADOW           (1<<8)
#define SIGC_PRECSHADOW           (1<<9)
#define SIGC_USEALPHABLENDING     (1<<10)
#define SIGC_USESPRITES           (1<<11)
#define SIGC_RANDOMROTATION       (1<<12)

// Bits 13-14 reserved for player hideability
#define SIGC_PLAYERHIDEABLE_LOWBIT (13)
#define SIGC_PLAYERHIDEABLE_MASK   (1<<13 | 1<<14)

#pragma pack(push,4)

//! structure to vegetation group properties loading/saving
struct StatInstGroupChunk
{
	StatInstGroupChunk() 
	{ 
    ZeroStruct(*this);
	}

	char  szFileName[256];
	float fBending;
	float fSpriteDistRatio;
	float fShadowDistRatio;
	float fMaxViewDistRatio;
	float	fBrightness;
	uint32  nMaterialLayers;

	float fDensity;
	float fElevationMax;
	float fElevationMin;
	float fSize;
	float fSizeVar;
	float fSlopeMax;
	float fSlopeMin;

  float fStatObjRadius_NotUsed;
  float fStatObjRadiusVert_NotUsed;
  
  float fLodDistRatio;
  uint32  nReserved;

  int nFlags;
  int nMaterialId;

	//! flags similar to entity render flags
	int m_dwRndFlags;

	AUTO_STRUCT_INFO_LOCAL
};

struct SNameChunk
{
	SNameChunk() { memset(this,0,sizeof(SNameChunk)); }

	char szFileName[256];

	AUTO_STRUCT_INFO_LOCAL
};

#pragma pack(pop)

int CTerrain::GetCompiledDataSize(SHotUpdateInfo * pExportInfo, int nSID)
{
	int nDataSize = 0;
	byte * pData = NULL;

  bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
  bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);

  if(bObjs)
  {
    assert(nSID < Get3DEngine()->m_pObjectsTree.Count() && Get3DEngine()->m_pObjectsTree[nSID]);
	  Get3DEngine()->m_pObjectsTree[nSID]->CleanUpTree();
    Get3DEngine()->m_pObjectsTree[nSID]->GetData(pData, nDataSize, NULL, NULL, eLittleEndian, pExportInfo);
  }

  if(bHMap)
  	GetParentNode(nSID)->GetData(pData, nDataSize, GetPlatformEndian(), pExportInfo);

	// get header size
	nDataSize += sizeof(STerrainChunkHeader);

	// get vegetation objects table size
  if(bObjs)
  {
	  nDataSize += sizeof(int);
	  nDataSize += GetObjManager()->m_lstStaticTypes.size()*sizeof(StatInstGroupChunk);

	  // get brush objects table size
	  nDataSize += sizeof(int);
    std::vector<IStatObj*> brushTypes;
    std::vector<IMaterial*> usedMats;

    { // get vegetation objects materials
      std::vector<IMaterial*> * pMatTable = &usedMats;
      std::vector<StatInstGroup> & rTable = GetObjManager()->m_lstStaticTypes;
      int nObjectsCount = rTable.size();

      // init struct values and load cgf's
      for(uint32 i=0; i<rTable.size(); i++)
      {
        int nMaterialId = -1;
        if(rTable[i].pMaterial)
        {
          nMaterialId = CObjManager::GetItemId(pMatTable, (IMaterial*)rTable[i].pMaterial, false);
          if(nMaterialId<0)
          {
            nMaterialId = pMatTable->size();
            pMatTable->push_back(rTable[i].pMaterial);
          }
        }
      }
    }

    Get3DEngine()->m_pObjectsTree[nSID]->GenerateStatObjAndMatTables(&brushTypes, &usedMats, pExportInfo);
    GetVisAreaManager()->GenerateStatObjAndMatTables(&brushTypes, &usedMats, pExportInfo);
	  nDataSize += brushTypes.size()*sizeof(SNameChunk);
    brushTypes.clear();

    // get custom materials table size
	  nDataSize += sizeof(int);
	  nDataSize += usedMats.size()*sizeof(SNameChunk);
  }

	return nDataSize;
}

bool CTerrain::GetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo, int nSID )
{
  float fStartTime = GetCurAsyncTimeSec();

  bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
  bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);

  //PrintMessage("Exporting terrain data (%s, %.2f MB) ...",
  //  (bHMap && bObjs) ? "Objects and heightmap" : (bHMap ? "Heightmap" : (bObjs ? "Objects" : "Nothing")), ((float)nDataSize)/1024.f/1024.f);

	// write header
	STerrainChunkHeader * pTerrainChunkHeader = (STerrainChunkHeader *)pData;
	pTerrainChunkHeader->nVersion = TERRAIN_CHUNK_VERSION;
  pTerrainChunkHeader->nDummy = 0;
  
  pTerrainChunkHeader->nFlags = (eEndian == eBigEndian) ? SERIALIZATION_FLAG_BIG_ENDIAN : 0;

  pTerrainChunkHeader->nFlags2 = 0;
	pTerrainChunkHeader->nChunkSize = nDataSize;
	pTerrainChunkHeader->TerrainInfo.nHeightMapSize_InUnits			= m_nSectorSize*m_nSectorsTableSize/m_nUnitSize;
	pTerrainChunkHeader->TerrainInfo.nUnitSize_InMeters					= m_nUnitSize;
	pTerrainChunkHeader->TerrainInfo.nSectorSize_InMeters				= m_nSectorSize;
	pTerrainChunkHeader->TerrainInfo.nSectorsTableSize_InSectors= m_nSectorsTableSize;
	pTerrainChunkHeader->TerrainInfo.fHeightmapZRatio						=	m_fHeightmapZRatio;
	pTerrainChunkHeader->TerrainInfo.fOceanWaterLevel						= (m_fOceanWaterLevel>WATER_LEVEL_UNKNOWN) ? m_fOceanWaterLevel : 0;

  SwapEndian(*pTerrainChunkHeader, eEndian);
	UPDATE_PTR_AND_SIZE(pData,nDataSize,sizeof(STerrainChunkHeader));

  std::vector<struct IStatObj*> * pStatObjTable = NULL;
  std::vector<struct IMaterial*> * pMatTable = NULL;

  if(bObjs)
  {
    pMatTable = new std::vector<struct IMaterial*>;

    { // get vegetation objects count
	    std::vector<StatInstGroup> & rTable = GetObjManager()->m_lstStaticTypes;
	    int nObjectsCount = rTable.size();

	    // prepare temporary chunks array for saving
	    PodArray<StatInstGroupChunk> lstFileChunks;
	    lstFileChunks.PreAllocate(nObjectsCount,nObjectsCount);

	    // init struct values and load cgf's
	    for(uint32 i=0; i<rTable.size(); i++)
	    {
		    strncpy(lstFileChunks[i].szFileName,rTable[i].szFileName,sizeof(lstFileChunks[i].szFileName));
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fBending);
        COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fSpriteDistRatio);
        COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fLodDistRatio);
        if(lstFileChunks[i].fLodDistRatio < 0.001f) // not allow to export value of 0 because it would mean that variable is not set
          lstFileChunks[i].fLodDistRatio = 0.001f;
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fShadowDistRatio);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fMaxViewDistRatio);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fBrightness);

        lstFileChunks[i].nFlags = 0;
        if(rTable[i].bAlignToTerrain)
          lstFileChunks[i].nFlags |= SIGC_ALIGNTOTERRAIN;
        if(rTable[i].bUseTerrainColor)
          lstFileChunks[i].nFlags |= SIGC_USETERRAINCOLOR;
        if(rTable[i].bAffectedByVoxels)
          lstFileChunks[i].nFlags |= SIGC_AFFECTED_BY_VOXELS;
        if(rTable[i].bHideability)
          lstFileChunks[i].nFlags |= SIGC_HIDEABILITY;
        if(rTable[i].bHideabilitySecondary)
          lstFileChunks[i].nFlags |= SIGC_HIDEABILITYSECONDARY;
        if(rTable[i].bPickable)
          lstFileChunks[i].nFlags |= SIGC_PICKABLE;
        if(rTable[i].bComplexBending)
          lstFileChunks[i].nFlags |= SIGC_COMPLEXBENDING;
        if(rTable[i].bCastShadow)
          lstFileChunks[i].nFlags |= SIGC_CASTSHADOW;
        if(rTable[i].bRecvShadow)
          lstFileChunks[i].nFlags |= SIGC_RECVSHADOW;
        if(rTable[i].bPrecShadow)
          lstFileChunks[i].nFlags |= SIGC_PRECSHADOW;
        if(rTable[i].bUseAlphaBlending)
          lstFileChunks[i].nFlags |= SIGC_USEALPHABLENDING;
        if(rTable[i].bUseSprites)
          lstFileChunks[i].nFlags |= SIGC_USESPRITES;
        if(rTable[i].bRandomRotation)
          lstFileChunks[i].nFlags |= SIGC_RANDOMROTATION;

		lstFileChunks[i].nFlags |= ((rTable[i].nPlayerHideable << SIGC_PLAYERHIDEABLE_LOWBIT) & SIGC_PLAYERHIDEABLE_MASK);

        lstFileChunks[i].nMaterialId = -1;
        if(rTable[i].pMaterial)
        {
          lstFileChunks[i].nMaterialId = CObjManager::GetItemId(pMatTable, (IMaterial*)rTable[i].pMaterial, false);
          if(lstFileChunks[i].nMaterialId<0)
          {
            lstFileChunks[i].nMaterialId = pMatTable->size();
            pMatTable->push_back(rTable[i].pMaterial);
          }
        }

		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],nMaterialLayers);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fDensity);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fElevationMax);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fElevationMin);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fSize);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fSizeVar);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fSlopeMax);
		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fSlopeMin);

		    COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],m_dwRndFlags);

    /*      // this values will be used if cgf is not streamed in yet
        rTable[i].fStatObjRadius = rTable[i].pStatObj ? rTable[i].pStatObj->GetRadius() : 0;
        rTable[i].fStatObjRadiusVert = rTable[i].pStatObj ? rTable[i].pStatObj->GetRadiusVert() : 0;
        COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fStatObjRadius);
        COPY_MEMBER_SAVE(&lstFileChunks[i],&rTable[i],fStatObjRadiusVert);*/
      }

      // get count
      AddToPtr(pData,nDataSize,nObjectsCount,eEndian);

      // get table content
      for(int i=0; i<lstFileChunks.Count(); i++)
        AddToPtr(pData,nDataSize,lstFileChunks[i],eEndian);

    /*      SwapEndian(lstFileChunks[i], eEndian);
	    memcpy(pData, lstFileChunks.GetElements(), lstFileChunks.Count()*sizeof(StatInstGroupChunk));	
	    UPDATE_PTR_AND_SIZE(pData,nDataSize,lstFileChunks.Count()*sizeof(StatInstGroupChunk));*/
    }

    pStatObjTable = new std::vector<struct IStatObj*>;
    Get3DEngine()->m_pObjectsTree[nSID]->GenerateStatObjAndMatTables(pStatObjTable,pMatTable, pExportInfo);
    GetVisAreaManager()->GenerateStatObjAndMatTables(pStatObjTable,pMatTable, pExportInfo);

    { // get brush objects count
	    int nObjectsCount = pStatObjTable->size();
      AddToPtr(pData,nDataSize,nObjectsCount,eEndian);

	    PodArray<SNameChunk> lstFileChunks;
	    lstFileChunks.PreAllocate(pStatObjTable->size(),pStatObjTable->size());

	    for(uint32 i=0; i<pStatObjTable->size(); i++)
	    {
		    strncpy(lstFileChunks[i].szFileName, (*pStatObjTable)[i]->GetFilePath(), sizeof(lstFileChunks[i].szFileName));
        AddToPtr(pData,nDataSize,lstFileChunks[i],eEndian);
	    }
    }

    { // get brush materials count
	    std::vector<IMaterial*> & rTable = *pMatTable;
	    int nObjectsCount = rTable.size();

	    // count
      AddToPtr(pData,nDataSize,nObjectsCount,eEndian);

	    // get table content
	    for(int dwI=0;dwI<nObjectsCount;++dwI)
	    {
		    SNameChunk tmp;
		    assert(strlen(rTable[dwI] ? rTable[dwI]->GetName() : "") < sizeof(tmp.szFileName));
        strcpy(tmp.szFileName, rTable[dwI] ? rTable[dwI]->GetName() : "");
        AddToPtr(pData,nDataSize,tmp,eEndian);
	    }
    }
  }

	// get nodes data
  int nNodesLoaded = bHMap ? GetParentNode( nSID )->GetData(pData, nDataSize, eEndian, pExportInfo) : 1;

  if(bObjs)
  {
	  Get3DEngine()->m_pObjectsTree[nSID]->CleanUpTree();

    int nOcNodesLoaded = Get3DEngine()->m_pObjectsTree[nSID]->GetData(pData, nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);

    if(ppStatObjTable)
      *ppStatObjTable = pStatObjTable;
    else
      SAFE_DELETE(pStatObjTable);

    if(ppMatTable)
      *ppMatTable = pMatTable;
    else
      SAFE_DELETE(pMatTable);
  }

  //PrintMessagePlus(" done in %.2f sec", GetCurAsyncTimeSec()-fStartTime );

	assert(nNodesLoaded && nDataSize==0);
  return (nNodesLoaded && nDataSize==0);
}

template <class T>
bool CTerrain::Load_T(T * & f, int & nDataSize, STerrainChunkHeader * pTerrainChunkHeader, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo, int nSID, Vec3 vSegmentOrigin)
{
  assert(pTerrainChunkHeader->nVersion == TERRAIN_CHUNK_VERSION);
  if(pTerrainChunkHeader->nVersion != TERRAIN_CHUNK_VERSION)
  {
    Error("CTerrain::SetCompiledData: version of file is %d, expected version is %d", pTerrainChunkHeader->nVersion, (int)TERRAIN_CHUNK_VERSION);
    return 0;
  }

  EEndian eEndian = (pTerrainChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;

  bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
  if(GetCVars()->e_VoxTer && !m_bEditor)
    bHMap = false;
  bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);
  AABB * pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  if(bHotUpdate)
    PrintMessage("Importing outdoor data (%s, %.2f MB) ...",
      (bHMap && bObjs) ? "Objects and heightmap" : (bHMap ? "Heightmap" : (bObjs ? "Objects" : "Nothing")), ((float)nDataSize)/1024.f/1024.f);

  if(pTerrainChunkHeader->nChunkSize != nDataSize+sizeof(STerrainChunkHeader))
    return 0;

  // get terrain settings
  m_nUnitSize					= pTerrainChunkHeader->TerrainInfo.nUnitSize_InMeters;
  m_fInvUnitSize			= 1.f/m_nUnitSize;
  m_nTerrainSize			= pTerrainChunkHeader->TerrainInfo.nHeightMapSize_InUnits*pTerrainChunkHeader->TerrainInfo.nUnitSize_InMeters;

  m_nTerrainSizeDiv =   (m_nTerrainSize>> m_nBitShift) - 1;
  m_nSectorSize				= pTerrainChunkHeader->TerrainInfo.nSectorSize_InMeters;
  m_nSectorsTableSize = pTerrainChunkHeader->TerrainInfo.nSectorsTableSize_InSectors;
  m_fHeightmapZRatio	=	pTerrainChunkHeader->TerrainInfo.fHeightmapZRatio;
  m_fOceanWaterLevel  = pTerrainChunkHeader->TerrainInfo.fOceanWaterLevel ? pTerrainChunkHeader->TerrainInfo.fOceanWaterLevel : WATER_LEVEL_UNKNOWN;

  m_nUnitsToSectorBitShift = 0;
  while(m_nSectorSize>>m_nUnitsToSectorBitShift > m_nUnitSize)
    m_nUnitsToSectorBitShift++;

  if(bHMap)
  {
    m_arrSegmentOrigns[nSID] = vSegmentOrigin;

    // build nodes tree in fast way
    BuildSectorsTree(false,nSID);

    // pass heightmap to the physics
    InitHeightfieldPhysics(nSID);
  }

  // setup physics grid
  if(!m_bEditor && !bHotUpdate)
  {
		int nCellSize = CTerrain::GetTerrainSize()>2048 ? CTerrain::GetTerrainSize()>>10 : 2;
    nCellSize = max(nCellSize,GetCVars()->e_PhysMinCellSize);
		int log2PODGridSize = 0;
		if (nCellSize==2)
			log2PODGridSize = 2;
		else if (nCellSize==4)
			log2PODGridSize = 1;
    GetPhysicalWorld()->SetupEntityGrid(2,Vec3(0,0,0), // this call will destroy all physicalized stuff
      CTerrain::GetTerrainSize()/nCellSize,CTerrain::GetTerrainSize()/nCellSize,(float)nCellSize,(float)nCellSize,log2PODGridSize);
  }

  std::vector<IMaterial*> * pMatTable = NULL;
  std::vector<IStatObj*> * pStatObjTable = NULL;

  if(bObjs)
  {
    PodArray<StatInstGroupChunk> lstStatInstGroupChunkFileChunks;

    { // get vegetation objects count
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Vegetation");

      int nObjectsCount = 0;
      if(!LoadDataFromFile(&nObjectsCount, 1, f, nDataSize, eEndian))
        return 0;

      // get vegetation objects table
      if(!m_bEditor || bHotUpdate)
      {
        if(!bHotUpdate)
          PrintMessage("===== Loading %d vegetation models =====", nObjectsCount);

        // load chunks into temporary array
        PodArray<StatInstGroupChunk> & lstFileChunks = lstStatInstGroupChunkFileChunks;
        lstFileChunks.PreAllocate(nObjectsCount,nObjectsCount);
        if(!LoadDataFromFile(lstFileChunks.GetElements(), lstFileChunks.Count(), f, nDataSize, eEndian))
          return 0;

        // preallocate real array
        std::vector<StatInstGroup> & rTable = GetObjManager()->m_lstStaticTypes;
        rTable.resize(nObjectsCount);//,nObjectsCount);

        // init struct values and load cgf's
        for(uint32 i=0; i<rTable.size(); i++)
        {
          strncpy(rTable[i].szFileName,lstFileChunks[i].szFileName,sizeof(rTable[i].szFileName));
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fBending);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fSpriteDistRatio);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fLodDistRatio);
          if(rTable[i].fLodDistRatio == 0) 
            rTable[i].fLodDistRatio = 1.f; // set default value if it was not exported
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fShadowDistRatio);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fMaxViewDistRatio);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fBrightness);

          rTable[i].bAlignToTerrain =  (lstFileChunks[i].nFlags & SIGC_ALIGNTOTERRAIN)!=0;
          rTable[i].bUseTerrainColor =  (lstFileChunks[i].nFlags & SIGC_USETERRAINCOLOR)!=0;
          rTable[i].bAffectedByVoxels =  (lstFileChunks[i].nFlags & SIGC_AFFECTED_BY_VOXELS)!=0;
          rTable[i].bHideability =  (lstFileChunks[i].nFlags & SIGC_HIDEABILITY)!=0;
          rTable[i].bHideabilitySecondary =  (lstFileChunks[i].nFlags & SIGC_HIDEABILITYSECONDARY)!=0;
          rTable[i].bPickable =  (lstFileChunks[i].nFlags & SIGC_PICKABLE)!=0;
          rTable[i].bComplexBending =  (lstFileChunks[i].nFlags & SIGC_COMPLEXBENDING)!=0;
          rTable[i].bCastShadow =  (lstFileChunks[i].nFlags & SIGC_CASTSHADOW)!=0;
          rTable[i].bRecvShadow =  (lstFileChunks[i].nFlags & SIGC_RECVSHADOW)!=0;
          rTable[i].bPrecShadow =  (lstFileChunks[i].nFlags & SIGC_PRECSHADOW)!=0;
          rTable[i].bUseAlphaBlending =  (lstFileChunks[i].nFlags & SIGC_USEALPHABLENDING)!=0;
          rTable[i].bUseSprites =  (lstFileChunks[i].nFlags & SIGC_USESPRITES)!=0;
          rTable[i].bRandomRotation =  (lstFileChunks[i].nFlags & SIGC_RANDOMROTATION)!=0;

		  rTable[i].nPlayerHideable = ((lstFileChunks[i].nFlags & SIGC_PLAYERHIDEABLE_MASK) >> SIGC_PLAYERHIDEABLE_LOWBIT);

          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],nMaterialLayers);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fDensity);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fElevationMax);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fElevationMin);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fSize);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fSizeVar);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fSlopeMax);
          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fSlopeMin);

          COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],m_dwRndFlags);

          // this values will be used if cgf is not streamed in yet
    //        COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fStatObjRadius);
    //      COPY_MEMBER_LOAD(&rTable[i],&lstFileChunks[i],fStatObjRadiusVert);


          int nMinSpec = (rTable[i].m_dwRndFlags&ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
          rTable[i].minConfigSpec = (ESystemConfigSpec)nMinSpec;

          if(rTable[i].szFileName[0])
          {
            ReleaseOwnership(rTable[i].pStatObj);
            rTable[i].pStatObj = GetObjManager()->LoadStatObj(rTable[i].szFileName);//,NULL,NULL,false);
          }

          rTable[i].Update(GetCVars(), Get3DEngine()->GetGeomDetailScreenRes());
        }
      }
      else
      { // skip data
        PodArray<StatInstGroupChunk> & lstFileChunks = lstStatInstGroupChunkFileChunks;
        lstFileChunks.PreAllocate(nObjectsCount,nObjectsCount);
        if(!LoadDataFromFile(lstFileChunks.GetElements(), lstFileChunks.Count(), f, nDataSize, eEndian))
          return 0;
        lstFileChunks.Clear();
      }
    }

    pStatObjTable = new std::vector<IStatObj*>;

    { // get brush objects count
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Brushes");

      int nObjectsCount = 0;
      if(!LoadDataFromFile(&nObjectsCount, 1, f, nDataSize, eEndian))
        return 0;

      // get brush objects table
      if(!m_bEditor || bHotUpdate)
      {
        if(!bHotUpdate)
          PrintMessage("===== Loading %d brush models ===== ", nObjectsCount);

        PodArray<SNameChunk> lstFileChunks;
        lstFileChunks.PreAllocate(nObjectsCount,nObjectsCount);
        if(!LoadDataFromFile(lstFileChunks.GetElements(), lstFileChunks.Count(), f, nDataSize, eEndian))
          return 0;

        pStatObjTable->resize(nObjectsCount);//PreAllocate(nObjectsCount,nObjectsCount);

        // load cgf's
        for(uint32 i=0; i<pStatObjTable->size(); i++)
        {
          if(lstFileChunks[i].szFileName[0])
            (*pStatObjTable)[i] = GetObjManager()->LoadStatObj(lstFileChunks[i].szFileName);
        }
      }
      else
      { // skip data
        PodArray<SNameChunk> lstFileChunks;
        lstFileChunks.PreAllocate(nObjectsCount,nObjectsCount);
        if(!LoadDataFromFile(lstFileChunks.GetElements(), lstFileChunks.Count(), f, nDataSize, eEndian))
          return 0;
      }
    }

    pMatTable = new std::vector<IMaterial*>;

    { // get brush materials count
      int nObjectsCount = 0;
      if(!LoadDataFromFile(&nObjectsCount, 1, f, nDataSize, eEndian))
        return 0;

      // get vegetation objects table
      if(!m_bEditor || bHotUpdate)
      {
        if(!bHotUpdate)
          PrintMessage("===== Loading %d brush materials ===== ", nObjectsCount);

        std::vector<IMaterial*> & rTable = *pMatTable;
        rTable.clear();
        rTable.resize(nObjectsCount);//PreAllocate(nObjectsCount,nObjectsCount);

        const uint32 cTableCount = rTable.size();
        for (uint32 tableIndex = 0; tableIndex < cTableCount; ++tableIndex)
        {
          SNameChunk matName;
          if(!LoadDataFromFile(&matName, 1, f, nDataSize, eEndian))
            return 0;
          rTable[tableIndex] = matName.szFileName[0] ? GetMatMan()->LoadMaterial(matName.szFileName) : NULL;
        }

        // assign real material to vegetation group
        std::vector<StatInstGroup> & rStaticTypes = GetObjManager()->m_lstStaticTypes;
        for(uint32 i=0; i<rStaticTypes.size(); i++)
        {
          if(lstStatInstGroupChunkFileChunks[i].nMaterialId >= 0)
            rStaticTypes[i].pMaterial = rTable[lstStatInstGroupChunkFileChunks[i].nMaterialId];
          else
            rStaticTypes[i].pMaterial = NULL;
        }
      }
      else
      { // skip data
        SNameChunk matInfoTmp;
        for (int tableIndex = 0; tableIndex < nObjectsCount; ++tableIndex)
        {
          if(!LoadDataFromFile(&matInfoTmp, 1, f, nDataSize, eEndian))
            return 0;
        }
      }
    }
  }

  // set nodes data
  if(!bHotUpdate)
    PrintMessage("===== Initializing terrain nodes ===== ");
  int nNodesLoaded = bHMap ? GetParentNode( nSID )->Load(f, nDataSize, eEndian, pExportInfo) : 1;

  if(bObjs)
  {
    PodArray<IRenderNode *> arrUnregisteredObjects;
    Get3DEngine()->m_pObjectsTree[nSID]->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, true);

    // load object instances (in case of editor just check input data do no create object instances)
    int nOcNodesLoaded = Get3DEngine()->m_pObjectsTree[nSID]->Load(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox);

    for(int i=0; i<arrUnregisteredObjects.Count(); i++)
      arrUnregisteredObjects[i]->ReleaseNode();
    arrUnregisteredObjects.Reset();
  }

  if(m_bEditor && !bHotUpdate)
  { // editor will re-insert all objects
    delete Get3DEngine()->m_pObjectsTree[nSID];
    Get3DEngine()->m_pObjectsTree[nSID] = NULL;
  }

  if(bObjs)
  {
    if(ppStatObjTable)
      *ppStatObjTable = pStatObjTable;
    else
      SAFE_DELETE(pStatObjTable);

    if(ppMatTable)
      *ppMatTable = pMatTable;
    else
      SAFE_DELETE(pMatTable);
  }

  if(bHMap)
  {
    GetParentNode(nSID)->UpdateRangeInfoShift();
    ResetTerrainVertBuffers(NULL, nSID);
  }

  assert(nNodesLoaded && nDataSize==0);
  return (nNodesLoaded && nDataSize==0);
}

bool CTerrain::SetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo, int nSID, Vec3 vSegmentOrigin)
{
  STerrainChunkHeader * pTerrainChunkHeader = (STerrainChunkHeader *)pData;
  pData += sizeof(STerrainChunkHeader);
  nDataSize -= sizeof(STerrainChunkHeader);

  SwapEndian(*pExportInfo, (pTerrainChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian);

  return Load_T(pData, nDataSize, pTerrainChunkHeader, ppStatObjTable, ppMatTable, bHotUpdate, pExportInfo, nSID, vSegmentOrigin);
}

bool CTerrain::Load(FILE * f, int nDataSize, STerrainChunkHeader * pTerrainChunkHeader, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable,int nSID, Vec3 vSegmentOrigin)
{
  return Load_T(f, nDataSize, pTerrainChunkHeader, ppStatObjTable, ppMatTable, 0, 0, nSID, vSegmentOrigin);
}

#include "TypeInfo_impl.h"
#include "terrain_compile_info.h"
