////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_load.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: terrain loading
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "terrain.h"
#include "terrain_sector.h"
#include "ObjMan.h"
#include "terrain_water.h"
#include "3dEngine.h"
#include "Vegetation.h"

CTerrain::CTerrain( const STerrainInfo & TerrainInfo )
{
	m_nLoadedSectors = 0;
	m_bOceanIsVisible = 0;
	m_fDistanceToSectorWithWater = 0;
//	m_nDiffTexIndexTableSize = 0;
//	m_nDiffTexTreeLevelOffset = 0;
//	ZeroStruct(m_hdrDiffTexInfo);
//	ZeroStruct(m_TerrainTextureLayer);
//	m_ucpDiffTexTmpBuffer = 0;
	m_pTerrainEf = 0;
	m_nSunLightMask = 0;
//	ZeroStruct(m_SSurfaceType); // staic
	m_pOcean = 0;
  m_eEndianOfTexture = eLittleEndian;

	// load default textures
	m_nWhiteTexId = GetRenderer()->EF_LoadTexture("Textures\\Defaults\\white.dds",FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D)->GetTextureID();
	m_nBlackTexId = GetRenderer()->EF_LoadTexture("Textures\\Defaults\\black.dds",FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D)->GetTextureID();

	// set params
	m_nUnitSize					= TerrainInfo.nUnitSize_InMeters;
	m_fInvUnitSize			= 1.f/TerrainInfo.nUnitSize_InMeters;
	m_nTerrainSize			= TerrainInfo.nHeightMapSize_InUnits*TerrainInfo.nUnitSize_InMeters;
	m_nSectorSize				= TerrainInfo.nSectorSize_InMeters;
	m_nSectorsTableSize = TerrainInfo.nSectorsTableSize_InSectors;
	m_fHeightmapZRatio	=	TerrainInfo.fHeightmapZRatio;
	m_fOceanWaterLevel  = TerrainInfo.fOceanWaterLevel;

	m_nUnitsToSectorBitShift = 0;
	while(m_nSectorSize>>m_nUnitsToSectorBitShift > m_nUnitSize)
		m_nUnitsToSectorBitShift++;

	// make flat 0 level heightmap
//	m_arrusHightMapData.Allocate(TerrainInfo.nHeightMapSize_InUnits+1);
//	m_arrHightMapRangeInfo.Allocate((TerrainInfo.nHeightMapSize_InUnits>>m_nRangeBitShift)+1);

	assert(m_nSectorsTableSize == m_nTerrainSize/m_nSectorSize);

	m_nBitShift=0;
	while((128>>m_nBitShift) != 128/CTerrain::GetHeightMapUnitSize())
		m_nBitShift++;

	m_nTerrainSizeDiv =(m_nTerrainSize>> m_nBitShift) - 1;

  // create default segment
  CreateSegment();

	InitHeightfieldPhysics(0);

	m_pTerrainEf           = MakeSystemMaterialFromShader("Terrain");
	m_pImposterEf					 = MakeSystemMaterialFromShader("Common.Imposter");

//	memset(m_arrImposters,0,sizeof(m_arrImposters));
//	memset(m_arrImpostersTopBottom,0,sizeof(m_arrImpostersTopBottom));

	m_StoredModifications.SetTerrain(*this);

	assert(m_SSurfaceType[0].GetDataSize()<16000);
}

void CTerrain::CloseTerrainTextureFile(int nSID)
{
  m_arrBaseTexInfos[nSID].m_nDiffTexIndexTableSize = 0;

  while(m_lstActiveTextureNodes.Count())
  {
    m_lstActiveTextureNodes.Last()->UnloadNodeTexture(false);
    m_lstActiveTextureNodes.DeleteLast();
  }

  while(m_lstActiveProcObjNodes.Count())
  {
    m_lstActiveProcObjNodes.Last()->RemoveProcObjects(false);
    m_lstActiveProcObjNodes.DeleteLast();
  }

	if(GetParentNode(nSID))
		GetParentNode(nSID)->UnloadNodeTexture(true);
}

CTerrain::~CTerrain()
{
  delete m_pOcean;

  for(int nSID=0; nSID<m_SSurfaceType.Count(); nSID++)
    DeleteSegment(nSID);
}

void CTerrain::InitHeightfieldPhysics(int nSID)
{
  if(GetCVars()->e_VoxTer && (!GetCVars()->e_VoxTerHeightmapEditing || !m_bEditor))
    return;

	// for phys engine
	primitives::heightfield hf;
	hf.Basis.SetIdentity();
	hf.origin.zero();
	hf.step.x = hf.step.y = (float)CTerrain::GetHeightMapUnitSize();
	hf.size.x = hf.size.y = CTerrain::GetTerrainSize()/CTerrain::GetHeightMapUnitSize();
	hf.stride.set(hf.size.y+1,1);
	hf.heightscale = 1.0f;//m_fHeightmapZRatio;
	hf.typemask = hf.typehole = STYPE_BIT_MASK;
	hf.heightmask = ~STYPE_BIT_MASK;
	
	hf.fpGetHeightCallback = GetHeightFromUnits_Callback;
	hf.fpGetSurfTypeCallback = GetSurfaceTypeFromUnits_Callback;

	int arrMatMapping[MAX_SURFACE_TYPES_COUNT];
	memset(arrMatMapping,0,sizeof(arrMatMapping));
	for(int i=0; i<MAX_SURFACE_TYPES_COUNT; i++)
		if(IMaterial * pMat = m_SSurfaceType[nSID][i].pLayerMat)
    {
      if(pMat->GetSubMtlCount()>2)
        pMat = pMat->GetSubMtl(2);
      arrMatMapping[i] = pMat->GetSurfaceTypeId();
    }

	IPhysicalEntity *pPhysTerrain = GetPhysicalWorld()->SetHeightfieldData(&hf, arrMatMapping, MAX_SURFACE_TYPES_COUNT);
	pe_params_foreign_data pfd;
	pfd.iForeignData = PHYS_FOREIGN_ID_TERRAIN;
	pPhysTerrain->SetParams(&pfd);
}
/*
int __cdecl CTerrain__Cmp_CVegetationForLoading_Size(const void* v1, const void* v2)
{
  CVegetationForLoading * p1 = ((CVegetationForLoading*)v1);
  CVegetationForLoading * p2 = ((CVegetationForLoading*)v2);

	PodArray<StatInstGroup> & lstStaticTypes = CStatObj::GetObjManager()->m_lstStaticTypes;

  CStatObj * pStatObj1 = (p1->GetID()<lstStaticTypes.Count()) ? lstStaticTypes[p1->GetID()].GetStatObj() : 0;
  CStatObj * pStatObj2 = (p2->GetID()<lstStaticTypes.Count()) ? lstStaticTypes[p2->GetID()].GetStatObj() : 0;

  if(!pStatObj1)
    return 1;
  if(!pStatObj2)
    return -1;

  int nSecId1 = 0;
  {  // get pos
    Vec3 vCenter = Vec3(p1->GetX(),p1->GetY(),p1->GetZ()) + (pStatObj1->GetBoxMin()+pStatObj1->GetBoxMax())*0.5f*p1->GetScale();
    // get sector ids
    int x = (int)(((vCenter.x)/CTerrain::GetSectorSize()));
    int y = (int)(((vCenter.y)/CTerrain::GetSectorSize()));
    // get obj bbox
    Vec3 vBMin = pStatObj1->GetBoxMin()*p1->GetScale();
    Vec3 vBMax = pStatObj1->GetBoxMax()*p1->GetScale();
    // if outside of the map, or too big - register in sector (0,0)
//    if( x<0 || x>=CTerrain::GetSectorsTableSize() || y<0 || y>=CTerrain::GetSectorsTableSize() ||
  //    (vBMax.x - vBMin.x)>TERRAIN_SECTORS_MAX_OVERLAPPING*2 || (vBMax.y - vBMin.y)>TERRAIN_SECTORS_MAX_OVERLAPPING*2)
    //  x = y = 0;

    // get sector id
    nSecId1 = (x)*CTerrain::GetSectorsTableSize() + (y);
  }

  int nSecId2 = 0;
  {  // get pos
    Vec3 vCenter = Vec3(p2->GetX(),p2->GetY(),p2->GetZ()) + (pStatObj2->GetBoxMin()+pStatObj2->GetBoxMax())*0.5f*p2->GetScale();
    // get sector ids
    int x = (int)(((vCenter.x)/CTerrain::GetSectorSize()));
    int y = (int)(((vCenter.y)/CTerrain::GetSectorSize()));
    // get obj bbox
    Vec3 vBMin = pStatObj2->GetBoxMin()*p2->GetScale();
    Vec3 vBMax = pStatObj2->GetBoxMax()*p2->GetScale();
    // if outside of the map, or too big - register in sector (0,0)
//    if( x<0 || x>=CTerrain::GetSectorsTableSize() || y<0 || y>=CTerrain::GetSectorsTableSize() ||
  //    (vBMax.x - vBMin.x)>TERRAIN_SECTORS_MAX_OVERLAPPING*2 || (vBMax.y - vBMin.y)>TERRAIN_SECTORS_MAX_OVERLAPPING*2)
    //  x = y = 0;

    // get sector id
    nSecId2 = (x)*CTerrain::GetSectorsTableSize() + (y);
  }

  if(nSecId1 > nSecId2)
    return -1;
  if(nSecId1 < nSecId2)
    return 1;

  if(p1->GetScale()*pStatObj1->GetRadius() > p2->GetScale()*pStatObj2->GetRadius())
    return 1;
  if(p1->GetScale()*pStatObj1->GetRadius() < p2->GetScale()*pStatObj2->GetRadius())
    return -1;

  return 0;
}

int __cdecl CTerrain__Cmp_Int(const void* v1, const void* v2)
{
  if(*(uint32*)v1 > *(uint32*)v2)
    return 1;
  if(*(uint32*)v1 < *(uint32*)v2)
    return -1;

  return 0;
}

void CTerrain::LoadVegetationances()
{
  assert(this); if(!this) return;

  PrintMessage("Loading static object positions ...");


  for( int x=0; x<CTerrain::GetSectorsTableSize(); x++)
  for( int y=0; y<CTerrain::GetSectorsTableSize(); y++)
    assert(!m_arrSecInfoPyramid[nSID][0][x][y]->m_lstEntities[STATIC_OBJECTS].Count());


  // load static object positions list
  PodArray<CVegetationForLoading> static_objects;
  static_objects.Load(Get3DEngine()->GetLevelFilePath("objects.lst"), gEnv->pCryPak);

	// todo: sorting in not correct for hierarchical system
  qsort(static_objects.GetElements(), static_objects.Count(), 
    sizeof(static_objects[0]), CTerrain__Cmp_CVegetationForLoading_Size);

  // put objects into sectors depending on object position and fill lstUsedCGFs
  PodArray<CStatObj*> lstUsedCGFs;
  for(int i=0; i<static_objects.Count(); i++)
  {
    float x       = static_objects[i].GetX();
    float y       = static_objects[i].GetY();
    float z       = static_objects[i].GetZ()>0 ? static_objects[i].GetZ() : GetZApr(x,y);
    int  nId      = static_objects[i].GetID();
    uint8 ucBr    = static_objects[i].GetBrightness();
		uint8 ucAngle = static_objects[i].GetAngle();
    float fScale  = static_objects[i].GetScale();

    if( nId>=0 && nId<GetObjManager()->m_lstStaticTypes.Count() && 
        fScale>0 && 
        x>=0 && x<CTerrain::GetTerrainSize() && y>=0 && y<CTerrain::GetTerrainSize() &&
        GetObjManager()->m_lstStaticTypes[nId].GetStatObj() )
    {
      if(GetObjManager()->m_lstStaticTypes[nId].GetStatObj()->GetRadius()*fScale < GetCVars()->e_VegetationMinSize)
        continue; // skip creation of very small objects

      if(lstUsedCGFs.Find(GetObjManager()->m_lstStaticTypes[nId].GetStatObj())<0)
        lstUsedCGFs.Add(GetObjManager()->m_lstStaticTypes[nId].GetStatObj());

      CVegetation * pEnt = (CVegetation*)Get3DEngine()->CreateRenderNode( eERType_Vegetation );
      pEnt->m_fScale = fScale;
      pEnt->m_vPos = Vec3(x,y,z);
      pEnt->SetStatObjGroupId(nId);
      pEnt->m_ucBright = ucBr;
			pEnt->CalcBBox();
      pEnt->Physicalize( );
    }
  }

  // release not used CGF's
  int nGroupsReleased=0;
  for(int i=0; i<GetObjManager()->m_lstStaticTypes.Count(); i++)
  {
    CStatObj * pStatObj = GetObjManager()->m_lstStaticTypes[i].GetStatObj();
    if(pStatObj && lstUsedCGFs.Find(pStatObj)<0)
    {
      Get3DEngine()->ReleaseObject(pStatObj);
      GetObjManager()->m_lstStaticTypes[i].pStatObj = NULL;
      nGroupsReleased++;
    }
  }

	PrintMessagePlus(" %d objects created", static_objects.Count());
}

void CTerrain::CompileObjects()
{
  // set max view distance and sort by size
	for(int nTreeLevel=0; nTreeLevel<TERRAIN_NODE_TREE_DEPTH; nTreeLevel++)
	for( int x=0; x<m_arrSecInfoPyramid[nSID][nTreeLevel].GetSize(); x++)
	for( int y=0; y<m_arrSecInfoPyramid[nSID][nTreeLevel].GetSize(); y++)
    m_arrSecInfoPyramid[nSID][nTreeLevel][x][y]->CompileObjects(STATIC_OBJECTS);
}*/

void CTerrain::GetStreamingStatus(int & nLoadedSectors, int & nTotalSectors)
{
  nLoadedSectors = m_nLoadedSectors;
  nTotalSectors = CTerrain::GetSectorsTableSize()*CTerrain::GetSectorsTableSize();
}

bool CTerrain::OpenTerrainTextureFile(SCommonFileHeader &hdrDiffTexHdr, STerrainTextureFileHeader & hdrDiffTexInfo, const char * szFileName, uint8 *  & ucpDiffTexTmpBuffer, int & nDiffTexIndexTableSize, int nSID)
{
  FUNCTION_PROFILER_3DENGINE;

  if(GetParentNode(nSID))
    GetParentNode(nSID)->UnloadNodeTexture(true);

	static char bNoLog = ~0; bNoLog++; // prevent printing error messages every frame

	if(!bNoLog)
		PrintMessage("Opening %s ...", szFileName);

	// rbx open flags, x is a hint to not cache whole file in memory.
	FILE * fpDiffTexFile = gEnv->pCryPak->FOpen(Get3DEngine()->GetLevelFilePath(szFileName), "rbx");

	if(!fpDiffTexFile) 
	{
		if(!bNoLog)
			PrintMessage("Error opening terrain texture file: file not found (you might need to regenerate the surface texture)");
		return false;
	}

	if(!gEnv->pCryPak->FRead(&hdrDiffTexHdr, 1, fpDiffTexFile, false))
	{
		gEnv->pCryPak->FClose(fpDiffTexFile);
		fpDiffTexFile=0;
		CloseTerrainTextureFile(nSID);
		if(!bNoLog)
			PrintMessage("Error opening terrain texture file: header not found (file is broken)");
		return false;
	}

  m_eEndianOfTexture = (hdrDiffTexHdr.flags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;

  SwapEndian(hdrDiffTexHdr, m_eEndianOfTexture);

	if(strcmp(hdrDiffTexHdr.signature,"CRY") || hdrDiffTexHdr.file_type != eTerrainTextureFile)
	{
		gEnv->pCryPak->FClose(fpDiffTexFile); fpDiffTexFile=0;
		CloseTerrainTextureFile(nSID);
		PrintMessage("Error opening terrain texture file: invalid signature");
		return false;
	}

	if(hdrDiffTexHdr.version != FILEVERSION_TERRAIN_TEXTURE_FILE)
	{
		gEnv->pCryPak->FClose(fpDiffTexFile);
		fpDiffTexFile=0;
		CloseTerrainTextureFile(nSID);
		if(!bNoLog)
			Error("Error opening terrain texture file: version error (you might need to regenerate the surface texture)");
		return false;
	}

	gEnv->pCryPak->FRead(&hdrDiffTexInfo, 1, fpDiffTexFile, m_eEndianOfTexture != GetPlatformEndian());

	memset(m_arrBaseTexInfos[nSID].m_TerrainTextureLayer,0,sizeof(m_arrBaseTexInfos[nSID].m_TerrainTextureLayer));

	// layers
	for(uint32 dwI=0;dwI<hdrDiffTexInfo.nLayerCount;++dwI)
	{
		assert(dwI<=2);		if(dwI>2)break;				// too many layers

		gEnv->pCryPak->FRead(&m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[dwI], 1, fpDiffTexFile, m_eEndianOfTexture != GetPlatformEndian());

		PrintMessage("  TerrainLayer TexFormat = %s", GetRenderer()->GetTextureFormatName(m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[dwI].eTexFormat));
		PrintMessage("    TerrainSectorTextureSize %dx%d", m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[dwI].nSectorSizePixels, m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[dwI].nSectorSizePixels);
		PrintMessage("    SectorTextureDataSizeBytes = %d", m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[dwI].nSectorSizeBytes);
	}

  // detect invalid hdrDiffTexInfo.fBrMultiplier, this allows to avoid re-export of all Crysis1 levels
  bool bBrMultiplierIsValid = false;
  for(int i=1; i<16; i++)
    if(1.f/hdrDiffTexInfo.fBrMultiplier == (float)i)
      bBrMultiplierIsValid = true;

  if(!bBrMultiplierIsValid)
    hdrDiffTexInfo.fBrMultiplier = 1.f;

  PrintMessage("  RGB multiplier = %.f", 1.f/hdrDiffTexInfo.fBrMultiplier);

  // unlock all nodes
  for(int nTreeLevel=0; nTreeLevel<TERRAIN_NODE_TREE_DEPTH; nTreeLevel++)
  for( int x=0; x<m_arrSecInfoPyramid[nSID][nTreeLevel].GetSize(); x++)
  for( int y=0; y<m_arrSecInfoPyramid[nSID][nTreeLevel].GetSize(); y++)
  {
    m_arrSecInfoPyramid[nSID][nTreeLevel][x][y]->m_nEditorDiffuseTex = 0;
    m_arrSecInfoPyramid[nSID][nTreeLevel][x][y]->m_nNodeTextureOffset = -1;
  }

	// index block
	{
		std::vector<int16> IndexBlock;		// for explanation - see saving code

		uint16 wSize;
		gEnv->pCryPak->FRead(&wSize, 1, fpDiffTexFile, m_eEndianOfTexture != GetPlatformEndian());

		IndexBlock.resize(wSize);

		for(uint16 wI=0;wI<wSize;++wI)
			gEnv->pCryPak->FRead(&IndexBlock[wI], 1, fpDiffTexFile, m_eEndianOfTexture != GetPlatformEndian());

    PrintMessage("  Texture indices = %d", wSize);

		int16 * pIndices = &IndexBlock[0];
		int16 nElementsLeft = wSize;

    if(wSize>0 && pIndices[0]>=0)
    {
      GetParentNode(nSID)->AssignTextureFileOffset(pIndices, nElementsLeft);
		  assert(nElementsLeft == 0);
    }

		nDiffTexIndexTableSize = wSize*sizeof(uint16) + sizeof(uint16);
	}

	delete [] ucpDiffTexTmpBuffer;
	ucpDiffTexTmpBuffer = new uint8[m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[0].nSectorSizeBytes+m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[1].nSectorSizeBytes];

	gEnv->pCryPak->FClose(fpDiffTexFile);
	fpDiffTexFile=0;

	// init texture pools
	m_texCache[0].InitPool(ucpDiffTexTmpBuffer, m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[0].nSectorSizePixels, m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[0].eTexFormat);
	
	if(m_arrBaseTexInfos[nSID].m_hdrDiffTexInfo.dwFlags == TTFHF_AO_DATA_IS_VALID && GetCVars()->e_TerrainAo)
		m_texCache[1].InitPool(ucpDiffTexTmpBuffer, m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[1].nSectorSizePixels, m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[1].eTexFormat);

	return true;
}
