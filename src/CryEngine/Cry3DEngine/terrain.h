////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef TERRAIN_H
#define TERRAIN_H

#include "TerrainModifications.h"						// CTerrainModifications

// lowest level of the outdoor world
#define TERRAIN_BOTTOM_LEVEL 0

// hightmap contain some additional info in last 4 bits
#define STYPE_BIT_MASK     (1|2|4|8)    // surface type id

#define STYPE_HOLE STYPE_BIT_MASK  // this surface type is reserved for hole in terrain
#define MAX_SURFACE_TYPES_COUNT 32  // max number of surfaces excluding hole type
#define TERRAIN_BASE_TEXTURES_NUM 2

// max view distance for objects shadow is size of object multiplied by this number
#define OBJ_MAX_SHADOW_VIEW_DISTANCE_RATIO 4

#define TERRAIN_NODE_TREE_DEPTH 16

#define OCEAN_IS_VERY_FAR_AWAY 1000000.f

#define ARR_TEX_OFFSETS_SIZE_DET_MAT 16

const int nHMCacheSize = 32;

// Heightmap data
class CHeightMap : public Cry3DEngineBase
{
public:
  // Access to heightmap data
  float GetZSafe(int x, int y, int nSID);
	float GetZ(int x, int y, int nSID);
  void SetZ(const int x, const int y, float fHeight, int nSID);
  float GetZfromUnits(uint32 nX_units, uint32 nY_units, int nSID);
  void SetZfromUnits(uint32 nX_units, uint32 nY_units, float fHeight, int nSID);
	float GetZMaxFromUnits(uint32 nX0_units, uint32 nY0_units, uint32 nX1_units, uint32 nY1_units, int nSID);
	uint8 GetSurfTypefromUnits(uint32 nX_units, uint32 nY_units, int nSID);
	static float GetHeightFromUnits_Callback(int ix, int iy);
	static unsigned char GetSurfaceTypeFromUnits_Callback(int ix, int iy);

  uint8 GetSurfaceTypeID(int x, int y, int nSID);
  float GetZApr(float x1, float y1, bool bIncludeVoxels, int nSID);
	float GetZMax(float x0, float y0, float x1, float y1, int nSID);
	bool GetHole(int x, int y, int nSID);
  bool IntersectWithHeightMap(Vec3 vStartPoint, Vec3 vStopPoint, float fDist, int nMaxTestsToScip, int nSID);
#ifdef SUPP_HMAP_OCCL
  bool Intersect(Vec3 vStartPoint, Vec3 vStopPoint, float fDist, int nMaxTestsToScip, Vec3 & vLastVisPoint, int nSID);
  bool IsBoxOccluded
	( 
		const AABB & objBox, 
		float fDistance, 
		bool bTerrainNode, 
		OcclusionTestClient * const __restrict pOcclTestVars,
    int nSID
	);
#endif
	// Exact test.
	struct SRayTrace
	{
		float	fInterp;
		Vec3	vHit;
		Vec3	vNorm;
		IMaterial*	pMaterial;

		SRayTrace() {}
		SRayTrace(float fT, Vec3 const& vH, Vec3 const& vN, IMaterial* pM)
			: fInterp(fT), vHit(vH), vNorm(vN), pMaterial(pM)
		{}
	};
  bool RayTrace(Vec3 const& vStart, Vec3 const& vEnd, SRayTrace* prt, int nSID);

	CHeightMap()
	{
		m_nUnitsToSectorBitShift = m_nBitShift = 0;
		m_fHeightmapZRatio = 0;
		m_bHeightMapModified = 0;
    ResetHeightMapCache();
	}

  void ResetHeightMapCache()
  {
    memset(m_arrCacheHeight,0,sizeof(m_arrCacheHeight));
    assert(sizeof(m_arrCacheHeight[0][0]) == 8);
    memset(m_arrCacheSurfType,0,sizeof(m_arrCacheSurfType));
    assert(sizeof(m_arrCacheSurfType[0][0]) == 4);
  }

	int				m_nBitShift;
	int				m_nUnitsToSectorBitShift;
	int				m_bHeightMapModified;
	float			m_fHeightmapZRatio;

protected:
	
	//protected: passes some internal data to avoid repetitive member loading
	bool IsPointUnderGround(CTerrain* const __restrict pTerrain, 
													int nUnitsToSectorBitShift, 
													uint32 nX_units, 
													uint32 nY_units, 
													float fTestZ, int nSID);

	inline bool IsPointUnderGround(uint32 nX_units, uint32 nY_units, float fTestZ, int nSID)
	{
		CTerrain* const __restrict pTerrain = Cry3DEngineBase::GetTerrain();
		return IsPointUnderGround(pTerrain, m_nUnitsToSectorBitShift, nX_units, nY_units, fTestZ, nSID);
	}

  struct SCachedHeight
  {
    SCachedHeight() { x=y=0; fHeight=0; } 
    uint16 x,y;
    float fHeight;
  };

  struct SCachedSurfType
  {
    SCachedSurfType() { x = y = surfType = 0; } 
    uint32 x : 14;
    uint32 y : 14;
    uint32 surfType : 4;
  };

  static SCachedHeight m_arrCacheHeight[nHMCacheSize][nHMCacheSize];
  static SCachedSurfType m_arrCacheSurfType[nHMCacheSize][nHMCacheSize];
};

struct SSurfaceType
{
  SSurfaceType() 
  { memset(this, 0,sizeof(SSurfaceType)); ucThisSurfaceTypeId=255; }

	bool IsMaterial3D() { return pLayerMat && pLayerMat->GetSubMtlCount()==3; }

	bool HasMaterial() { return pLayerMat != NULL; }

	IMaterial * GetMaterialOfProjection(uint8 ucProjAxis)
	{
		if (pLayerMat)
		{
			if(pLayerMat->GetSubMtlCount() == 3)
			{
				for(int i=0; i<pLayerMat->GetSubMtlCount(); i++)
					if(pLayerMat->GetSubMtl(i)->m_ucDefautMappingAxis == ucProjAxis)
						return pLayerMat->GetSubMtl(i);
			}
			else if(ucProjAxis == ucDefProjAxis)
				return pLayerMat;

			//assert(!"SSurfaceType::GetMaterialOfProjection: Material not found");
		}

		return NULL;
	}

	float GetMaxMaterialDistanceOfProjection(uint8 ucProjAxis)
	{
		if(ucProjAxis == 'X' || ucProjAxis == 'Y')
			return fMaxMatDistanceXY;

		return fMaxMatDistanceZ;
	}

	char szName[64];
	_smart_ptr<IMaterial> pLayerMat;
  float fScale;
	PodArray<int> lstnVegetationGroups;
	float fMaxMatDistanceXY;
	float fMaxMatDistanceZ;
	float arrRECustomData[4][ARR_TEX_OFFSETS_SIZE_DET_MAT];
	uint8 ucDefProjAxis;
	uint8 ucThisSurfaceTypeId;
  float fCustomMaxDistance;
};

struct CTerrainNode;

struct CTextureCache : public Cry3DEngineBase
{
	PodArray<uint32> m_FreeTextures;
	PodArray<uint32> m_UsedTextures;
	PodArray<uint32> m_Quarantine;
	ETEX_Format m_eTexFormat;
	int m_nDim;

	CTextureCache();
	~CTextureCache();
	uint32 GetTexture(byte * pData);
	void ReleaseTexture(uint32 nTexId);
	bool Update();
	void InitPool(byte * pData, int nDim, ETEX_Format eTexFormat);
	void ResetTexturePool();
	int GetPoolSize();
};

// The Terrain Class
class CTerrain : public ITerrain, public CHeightMap
{
	friend struct CTerrainNode;

public:

  CTerrain( const STerrainInfo & TerrainInfo );
  ~CTerrain();

	void InitHeightfieldPhysics(int nSID);
	inline static const int GetTerrainSize()					{ return m_nTerrainSize; }
	inline static const int GetSectorSize()					{ return m_nSectorSize; }
	inline static const int GetHeightMapUnitSize()		{ return m_nUnitSize; }
	inline static const int GetSectorsTableSize()		{ return m_nSectorsTableSize; }
	inline static const float GetInvUnitSize()				{ return m_fInvUnitSize; }

	ILINE const int GetTerrainUnits()								{ return m_nTerrainSizeDiv; }
	ILINE void ClampUnits(uint32& xu, uint32& yu)
	{
    xu = (int)xu < 0 ? 0 : (int)xu < GetTerrainUnits() ? xu : GetTerrainUnits();//min(max((int)xu, 0), GetTerrainUnits());
    yu = (int)yu < 0 ? 0 : (int)yu < GetTerrainUnits() ? yu : GetTerrainUnits();//min(max((int)xu, 0), GetTerrainUnits());
	}
	ILINE CTerrainNode* GetSecInfoUnits(int xu, int yu, int nSID)
	{
		return m_arrSecInfoPyramid[nSID][0][xu>>m_nUnitsToSectorBitShift][yu>>m_nUnitsToSectorBitShift];
	}
	ILINE CTerrainNode* GetSecInfoUnits(int xu, int yu, int nUnitsToSectorBitShift, int nSID)
	{
		return m_arrSecInfoPyramid[nSID][0][xu>>nUnitsToSectorBitShift][yu>>nUnitsToSectorBitShift];
	}
	ILINE CTerrainNode* GetSecInfo(int x, int y, int nSID)
	{
		if (x < 0 || y < 0 || x >= m_nTerrainSize || y >= m_nTerrainSize || !m_pParentNodes[nSID])
			return 0;
		return GetSecInfoUnits(x >> m_nBitShift, y >> m_nBitShift, nSID);
	}
  CTerrainNode * GetSecInfo(const Vec3 & pos)
  {
    for(int nSID=0; nSID<m_arrSecInfoPyramid.Count() && nSID<m_pParentNodes.Count(); nSID++)
    {
      if(m_pParentNodes[nSID])
        if(Overlap::Point_AABB2D( pos, m_pParentNodes[nSID]->GetBBoxVirtual()))
          return GetSecInfo((int)pos.x, (int)pos.y, nSID);
    }
    assert(!"Requested segment is not loaded");
    return NULL;
  }
  CTerrainNode * GetSecInfo(int x, int y)
  {
    Vec3 vPos((float)x, (float)y, 0);    
    return GetSecInfo(vPos);
  }	
  ILINE CTerrainNode * GetSecInfo(const Vec3 & pos, int nSID)
	{
		return GetSecInfo((int)pos.x, (int)pos.y, nSID);
	}
	ILINE float GetWaterLevel() { return m_fOceanWaterLevel;/* ? m_fOceanWaterLevel : WATER_LEVEL_UNKNOWN;*/ }
	ILINE void SetWaterLevel(float fOceanWaterLevel) { m_fOceanWaterLevel = fOceanWaterLevel; }

	//////////////////////////////////////////////////////////////////////////
	// ITerrain Implementation.
	//////////////////////////////////////////////////////////////////////////

  template <class T>
  bool Load_T(T * & f, int & nDataSize, STerrainChunkHeader * pTerrainChunkHeader, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo, int nSID, Vec3 vSegmentOrigin);

  VIRTUAL bool GetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo, int nSID);
	VIRTUAL int  GetCompiledDataSize(SHotUpdateInfo * pExportInfo, int nSID);

	VIRTUAL IRenderNode* AddVegetationInstance( int nStaticGroupID,const Vec3 &vPos,const float fScale,uint8 ucBright,uint8 angle,int nSID );
	VIRTUAL void SetOceanWaterLevel( float fOceanWaterLevel );

  VIRTUAL int CreateSegment();
  VIRTUAL bool SetSegmentOrigin(int nSID, Vec3 vSegmentOrigin);
  VIRTUAL bool DeleteSegment(int nSID);

	//////////////////////////////////////////////////////////////////////////

	void RemoveAllStaticObjects(int nSID);
	void ApplyForceToEnvironment(Vec3 vPos, float fRadius, float fAmountOfForce);
//	void CheckUnload();
	void CloseTerrainTextureFile(int nSID);
	void DrawVisibleSectors();
	VIRTUAL int GetDetailTextureMaterials(IMaterial* materials[],int nSID);
  void RenderAOSectors(int nSID);
	void MakeCrater(Vec3 vExploPos, float fExploRadius);
	bool RemoveObjectsInArea(Vec3 vExploPos, float fExploRadius);
	float GetDistanceToSectorWithWater() { return m_fDistanceToSectorWithWater; }
	void GetMemoryUsage(class ICrySizer*pSizer);
	void GetObjects(PodArray<struct SRNInfo> * pLstObjects);
	void GetObjectsAround(Vec3 vPos, float fRadius, PodArray<struct SRNInfo> * pEntList, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects);
	class COcean * GetOcean() { return m_pOcean; }
	Vec3 GetTerrainSurfaceNormal(Vec3 vPos, float fRange, bool bIncludeVoxels = false);
	int GetActiveTextureNodesCount() { return m_lstActiveTextureNodes.Count(); }
	int GetActiveProcObjNodesCount() { return m_lstActiveProcObjNodes.Count(); }
	int GetNotReadyTextureNodesCount();
	void GetTextureCachesStatus(int & nCount0, int & nCount1) 
	{ nCount0 = m_texCache[0].GetPoolSize(); nCount1 = m_texCache[1].GetPoolSize(); }

	bool PreloadResources();
	void CheckVis();
	int RenderTerrainWater();
	void UpdateNodesIncrementaly();
  void CheckNodesGeomUnload(int nSID);
	void GetStreamingStatus(int & nLoadedSectors, int & nTotalSectors);
	void InitTerrainWater(IMaterial * pTerrainWaterMat, int nWaterBottomTexId);
	void ResetTerrainVertBuffers(const AABB * pBox, int nSID);
	void SetTerrainSectorTexture( const int nTexSectorX, const int nTexSectorY, unsigned int textureId, int nSID );
	void SetDetailLayerProperties(int nId, float fScaleX, float fScaleY, uint8 ucProjAxis, const char * szSurfName, const PodArray<int> & lstnVegetationGroups, IMaterial * pMat, int nSID);
	bool IsOceanVisible() { return m_bOceanIsVisible != 0; }
	void SetTerrainElevation(int x1, int y1, int nSizeX, int nSizeY, float * pTerrainBlock, unsigned char * pSurfaceData, uint32 * pResolMap, int nResolMapSizeX, int nResolMapSizeY, int nSID);
	void GetVisibleSectorsInAABB(PodArray<struct CTerrainNode*> & lstBoxSectors, const AABB & boxBox);
	void RegisterLightMaskInSectors(CDLight * pLight, int nSID);
	void LoadSurfaceTypesFromXML(XmlNodeRef pDoc, int nSID);
	void UpdateSurfaceTypes(int nSID);
	bool RenderArea(Vec3 vPos, float fRadius, 
		IRenderMesh ** arrLightRenderMeshs, CRenderObject * pObj, IMaterial * pMaterial, 
		const char * szComment, float * pCustomData, Plane * planes);
	void IntersectWithShadowFrustum(PodArray<CTerrainNode*> * plstResult, ShadowMapFrustum * pFrustum, int nSID);
	void IntersectWithBox(const AABB & aabbBox, PodArray<CTerrainNode*> * plstResult, int nSID);
	void MarkAllSectorsAsUncompiled( int nSID );
	void BuildErrorsTableForArea(float * pLodErrors, int nMaxLods, int X1, int Y1, int X2, int Y2, float * pTerrainBlock,
		uint8 * pSurfaceData, int nSurfOffsetX, int nSurfOffsetY, int nSurfSizeX, int nSurfSizeY);
  int GenerateMeshFromHeightmap(  int nMinX, int nMinY, int nMaxX, int nMaxY, PodArray<Vec3> & arrVerts, PodArray<uint16> & arrIndices,int nSID );

	void GetResourceMemoryUsage(ICrySizer*	pSizer,const AABB&	crstAABB,int nSID);
protected:

  void AddVisSector(CTerrainNode * newsec);

  PodArray<CTerrainNode*> m_pParentNodes;

  PodArray<Vec3> m_arrSegmentOrigns;

	void BuildSectorsTree(bool bBuildErrorsTable, int nSID);
	int GetTerrainNodesAmount();
	bool OpenTerrainTextureFile(SCommonFileHeader &hdrDiffTexHdr, STerrainTextureFileHeader & hdrDiffTexInfo, const char * szFileName, uint8 *  & ucpDiffTexTmpBuffer, int & nDiffTexIndexTableSize, int nSID);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////////////////////////

public:

	void DoVoxelShape(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<class CVoxelObject*> * pAffectedVoxAreas);
	bool Voxel_Recompile_Modified_Incrementaly_Objects();
	bool Voxel_FindNeighboursForObject(class CVoxelObject * pThisObject, class CVoxelObject ** arrNeighbours);
	void Voxel_SetFlags(bool bPhysics, bool bSimplify, bool bShadows, bool bMaterials);
	void BuildVoxelSpace();

  CTerrainNode * GetParentNode(int nId) { return ( nId>=0 && nId<m_pParentNodes.Count()) ? m_pParentNodes.GetAt(nId) : NULL; }

  bool Recompile_Modified_Incrementaly_RoadRenderNodes();

	virtual void SerializeTerrainState( TSerialize ser );

	void SetHeightMapMaxHeight(float fMaxHeight);

	SSurfaceType * GetSurfaceTypePtr(int x, int y, int nSID)
	{
		int nType = GetSurfaceTypeID(x,y,nSID);
		assert(nType>=0 && nType<MAX_SURFACE_TYPES_COUNT);
		return (nType!=STYPE_HOLE) ? &m_SSurfaceType[nSID][nType] : NULL;
	}

	SSurfaceType * GetSurfaceTypes(int nSID) { return &m_SSurfaceType[nSID][0]; }

  int GetTerrainTextureNodeSizeMeters() { return CTerrain::GetSectorSize(); }

	int GetTerrainTextureNodeSizePixels(int nLayer, int nSID)
	{
		assert(nLayer>=0 && nLayer<2);
		return m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[nLayer].nSectorSizePixels;
	}

	void GetMaterials(IMaterial * & pTerrainEf)
	{
		pTerrainEf = m_pTerrainEf;
	}

	void SetSunLightMask(uint32 nSunLightMask) { m_nSunLightMask = nSunLightMask; }
	uint32 GetSunLightMask() { return m_nSunLightMask; }

	int m_nWhiteTexId;
	int m_nBlackTexId;

	PodArray < PodArray < Array2d < struct CTerrainNode* > > > m_arrSecInfoPyramid;//[TERRAIN_NODE_TREE_DEPTH];

	float GetTerrainTextureMultiplier(int nSID) const { return m_arrBaseTexInfos[nSID].m_hdrDiffTexInfo.fBrMultiplier; }
//	void ProcessPanorama();
//	void ProcessPanoramaCluster(CREPanoramaCluster * pImp, const Vec3 & vCamDir, float fFOV, bool bCheckUpdate);
	
	void ActivateNodeTexture( CTerrainNode * pNode );
	void ActivateNodeProcObj( CTerrainNode * pNode );
	CTerrainNode * FindMinNodeContainingBox(const AABB & someBox, int nSID);
	int GetTerrainLightmapTexId( Vec4 & vTexGenInfo, int nSID );

	IRenderMesh * MakeAreaRenderMesh(	const Vec3 & vPos, float fRadius, 
		IMaterial * pMat, const char * szLSourceName, Plane * planes);

	bool IsAmbientOcclusionEnabled();

  template <class T> static bool LoadDataFromFile(T *data, size_t elems, FILE * & f, int & nDataSize, EEndian eEndian, int * pSeek = 0)
  {
    if(pSeek)
      *pSeek = GetPak()->FTell(f);

    if(GetPak()->FRead(data, elems, f, eEndian) != elems)
    {
      assert(0);
      return false;
    }
    nDataSize -= sizeof(T)*elems;
    assert(nDataSize >= 0);
    return true;
  }

  static bool LoadDataFromFile_Seek(size_t elems, FILE * & f, int & nDataSize, EEndian eEndian)
  {
    GetPak()->FSeek(f, elems, SEEK_CUR);
    nDataSize -= elems;
    assert(nDataSize >= 0);
    return (nDataSize >= 0);
  }

  template <class T> static bool LoadDataFromFile(T *data, size_t elems, uint8 * & f, int & nDataSize, EEndian eEndian, int * pSeek = 0)
  {
    StepDataCopy(data, f, elems, eEndian);
		nDataSize -= elems*sizeof(T);
		assert(nDataSize >= 0);
    return (nDataSize >= 0);
  }

  static bool LoadDataFromFile_Seek(size_t elems, uint8 * & f, int & nDataSize, EEndian eEndian)
  {
    nDataSize -= elems;
    f += elems;
    assert(nDataSize >= 0);
    return true;
  }

  static void LoadDataFromFile_FixAllignemt(FILE * & f, int & nDataSize)
  {
    while(nDataSize&3)
    {
      int nRes = GetPak()->FSeek(f,1,SEEK_CUR);
      assert(nRes==0);
      assert(nDataSize);
      nDataSize--;
    }
    assert(nDataSize >= 0);
  }

  static void LoadDataFromFile_FixAllignemt(uint8 * & f, int & nDataSize)
  {
    while(nDataSize&3)
    {
      assert(*f == 222);
      f++;
      assert(nDataSize);
      nDataSize--;
    }
    assert(nDataSize >= 0);
  }

  int ReloadModifiedHMData(FILE * f, int nSID);

protected: // ------------------------------------------------------------------------

	CTerrainModifications				m_StoredModifications;					// to serialize (load/save) terrain heighmap changes and limit the modification
	int													m_nLoadedSectors;								//
	int													m_bOceanIsVisible;								//

	float												m_fDistanceToSectorWithWater;		//
	
	struct SBaseTexInfo {
		int m_nDiffTexIndexTableSize;
		SCommonFileHeader m_hdrDiffTexHdr;
		STerrainTextureFileHeader m_hdrDiffTexInfo;
		STerrainTextureLayerFileHeader m_TerrainTextureLayer[2];
		uint8 * m_ucpDiffTexTmpBuffer;
	};

  PodArray < SBaseTexInfo > m_arrBaseTexInfos;
	
	IMaterial * m_pTerrainEf, *m_pImposterEf;

	float												m_fOceanWaterLevel;
	uint32												m_nSunLightMask;

	PodArray<struct CTerrainNode*> m_lstVisSectors;

	PodArray < PodArray < SSurfaceType > >  m_SSurfaceType;

	static int						m_nUnitSize;							// in meters
	static float					m_fInvUnitSize;						// in 1/meters
	static int						m_nTerrainSize; 					// in meters
  int                   m_nTerrainSizeDiv;
	static int						m_nSectorSize;						// in meters
	static int						m_nSectorsTableSize; 			// sector width/height of the finest LOD level (sector count is the square of this value)

	class COcean *				m_pOcean;

	PodArray<CTerrainNode*> m_lstActiveTextureNodes;
	PodArray<CTerrainNode*> m_lstActiveProcObjNodes;

	CTextureCache m_texCache[2];

  EEndian m_eEndianOfTexture;

public:
  bool SetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo, int nSID, Vec3 vSegmentOrigin);
  bool Load(FILE * f, int nDataSize, struct STerrainChunkHeader * pTerrainChunkHeader, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, int nSID, Vec3 vSegmentOrigin);
  EEndian GetEndianOfTexture() { return m_eEndianOfTexture; }
};

#endif // TERRAIN_H

