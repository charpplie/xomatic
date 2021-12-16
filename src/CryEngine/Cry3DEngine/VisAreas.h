////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   visareas.h
//  Version:     v1.00
//  Created:     18/12/2002 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: visibility areas header
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef VisArea_H
#define VisArea_H

#include "BasicArea.h"
#include "CRAMSolution.h"





	#define bool_type bool



struct CVisArea : public IVisArea, public CBasicArea
{
	// editor interface
	virtual void Update(const Vec3 * pPoints, int nCount, const char * szName, const SVisAreaInfo & info);

	CVisArea();
	~CVisArea();
	bool IsSphereInsideVisArea(const Vec3 & vPos, const f32 fRadius);
	bool IsPointInsideVisArea(const Vec3 & vPos);
  bool IsBoxOverlapVisArea(const AABB & objBox);
	bool ClipToVisArea(bool bInside, Sphere& sphere, Vec3 const& vNormal);
	bool FindVisArea(IVisArea * pAnotherArea, int nMaxRecursion, bool bSkipDisabledPortals);
	bool FindVisAreaReqursive(IVisArea * pAnotherArea, int nMaxReqursion, bool bSkipDisabledPortals, PodArray<CVisArea*> & arrVisitedParents);
  bool GetDistanceThruVisAreas(AABB vCurBoxIn, IVisArea * pTargetArea, const AABB & targetBox, int nMaxReqursion, float & fResDist);
  bool GetDistanceThruVisAreasReq(AABB vCurBoxIn, float fCurDistIn, IVisArea * pTargetArea, const AABB & targetBox, int nMaxReqursion, PodArray<CVisArea*> & arrVisitedParents, float & fResDist);
	void FindSurroundingVisArea( int nMaxReqursion, bool bSkipDisabledPortals, PodArray<IVisArea*> * pVisitedAreas = NULL, int nMaxVisitedAreas = 0, int nDeepness = 0);
	void FindSurroundingVisAreaReqursive( int nMaxReqursion, bool bSkipDisabledPortals, PodArray<IVisArea*> * pVisitedAreas, int nMaxVisitedAreas, int nDeepness, PodArray<CVisArea*> * pUnavailableAreas );
	int GetVisFrameId();
	Vec3 GetConnectionNormal(CVisArea * pPortal);
	void PreRender(int nReqursionLevel, CCamera CurCamera, CVisArea * pParent, CVisArea * pCurPortal, bool_type * pbOutdoorVisible, PodArray<CCamera> * plstOutPortCameras, bool_type * pbSkyVisible, bool_type * pbOceanVisible, PodArray<CVisArea*> & lstVisibleAreas);
	void UpdatePortalCameraPlanes(CCamera & cam, Vec3 * pVerts, bool bMergeFrustums);
	int GetVisAreaConnections(IVisArea ** pAreas, int nMaxConnNum, bool bSkipDisabledPortals = false);
  int GetRealConnections(IVisArea ** pAreas, int nMaxConnNum, bool bSkipDisabledPortals = false);
	bool IsPortalValid();
	bool IsPortalIntersectAreaInValidWay(CVisArea * pPortal);
	bool IsPortal();
	bool IsShapeClockwise();
	bool IsAffectedByOutLights() { return m_bAffectedByOutLights; }
	bool IsActive() { return m_bActive || (GetCVars()->e_Portals==4); }
	void UpdateGeometryBBox();
  void DrawAreaBoundsIntoCBuffer(class CCullBuffer * pCBuffer);
  void ClipPortalVerticesByCameraFrustum(PodArray<Vec3> * pPolygon, const CCamera & cam);
  void GetMemoryUsage(ICrySizer*pSizer);
	bool IsConnectedToOutdoor();
	const char * GetName() { return m_sName; }
	int GetData(byte * & pData, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo);
  template <class T>
  int Load_T(T * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo);
  int Load(byte * & f, int & nDataSizeLeft, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo);
  int Load(FILE * & f, int & nDataSizeLeft, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo);
	const AABB* GetAABBox() const;
  void UpdateOcclusionFlagInTerrain();
  void AddConnectedAreas(PodArray<CVisArea *> & lstAreas, int nMaxRecursion);

	//RAM
	void		CalcAmbientCube(f32* pAmbientCube,const Vec3& rAABBMin,const Vec3& rAABBMax);
	void		CalcHQAmbientCube(f32* pAmbientCube,const Vec3& rAABBMin,const Vec3& rAABBMax);
	void		RAMAssign(CDLight* pLight);
	void		RAMRelease(CDLight* pLight);

  const Vec3 GetFinalAmbientColor();

	char m_sName[64];
	PodArray<CVisArea*> m_lstConnections;
	Vec3 m_vConnNormals[2];
	int m_nRndFrameId;

#ifdef SUPPORT_LM
	CRAMSolution	m_RAMSolution[2];	//double buffered
	int m_RAMFrame;
#endif // SUPPORT_LM

	PodArray<Vec3> m_lstShapePoints;
	float m_fHeight;
	Vec3 m_vAmbientColor;
	float m_fDistance;
	float m_fViewDistRatio;
	CCamera * m_arrOcclCamera[MAX_RECURSION_LEVELS];
	OcclusionTestClient m_OcclState;
	Vec3 m_arrvActiveVerts[4];
	PodArray<CCamera> m_lstCurCameras;
	bool_type m_bAffectedByOutLights;
	bool_type m_bSkyOnly;
	bool_type m_bOceanVisible;
	bool_type m_bDoubleSide;
	bool_type m_bUseDeepness;
	bool_type m_bUseInIndoors;
	bool_type m_bThisIsPortal;
	bool_type m_bIgnoreSky;
	bool_type m_bActive;
};

struct SAABBTreeNode
{
	SAABBTreeNode( PodArray<CVisArea*> & lstAreas, AABB box, int nMaxRecursion = 0);
	~SAABBTreeNode( ) ;
	CVisArea * FindVisarea(const Vec3 & vPos);
	SAABBTreeNode* GetTopNode(const AABB& box, void** pNodeCache);
	bool IntersectsVisAreas(const AABB& box);
	int ClipOutsideVisAreas(Sphere& sphere, Vec3 const& vNormal);

	AABB nodeBox;
	PodArray<CVisArea*> nodeAreas;
	SAABBTreeNode * arrChilds[2];
};

struct CVisAreaManager : public IVisAreaManager, Cry3DEngineBase
{
	CVisArea * m_pCurArea, * m_pCurPortal;
	PodArray<CVisArea * > m_lstActiveEntransePortals;

	PodArray<CVisArea*> m_lstVisAreas;
  PodArray<CVisArea*> m_lstPortals;
  PodArray<CVisArea*> m_lstOcclAreas;
	PodArray<CVisArea*> m_lstActiveOcclVolumes;
	PodArray<CVisArea*> m_lstIndoorActiveOcclVolumes;
	PodArray<CVisArea*> m_lstVisibleAreas;
  bool_type m_bOutdoorVisible;
  bool_type m_bSkyVisible;
  bool_type m_bOceanVisible;
	bool_type m_bSunIsNeeded;
  PodArray<CCamera> m_lstOutdoorPortalCameras;
	PodArray<IVisAreaCallback*> m_lstCallbacks;
	SAABBTreeNode * m_pAABBTree;

	CVisAreaManager();
	~CVisAreaManager();
	void UpdateAABBTree();
	void SetCurAreas();
	PodArray<CVisArea*> * GetActiveEntransePortals() { return &m_lstActiveEntransePortals; }
	void PortalsDrawDebug();
	bool IsEntityVisible(IRenderNode * pEnt);
	bool IsOutdoorAreasVisible();
  bool IsSkyVisible();
  bool IsOceanVisible();
	CVisArea * CreateVisArea();
	bool DeleteVisArea(CVisArea * pVisArea);
	bool SetEntityArea(IRenderNode* pEnt, const AABB & objBox, const float fObjRadiusSqr);
	void CheckVis();
	void DrawVisibleSectors();
	void ActivatePortal(const Vec3 &vPos, bool bActivate, const char * szEntityName);
	void UpdateVisArea(CVisArea * pArea, const Vec3 * pPoints, int nCount, const char * szName, const SVisAreaInfo & info);
	void UpdateConnections();
	void MoveObjectsIntoList(PodArray<SRNInfo> * plstVisAreasEntities, const AABB & boxArea, bool bRemoveObjects = false);
	IVisArea * GetVisAreaFromPos(const Vec3 &vPos);
	bool IntersectsVisAreas(const AABB& box, void** pNodeCache = 0);
	bool ClipOutsideVisAreas(Sphere& sphere, Vec3 const& vNormal, void* pNodeCache = 0);
	bool IsEntityVisAreaVisible(IRenderNode * pEnt, int nMaxReqursion, const CDLight * pLight );
	void MakeActiveEntransePortalsList(const CCamera * pCamera, PodArray<CVisArea *> & lstActiveEntransePortals, CVisArea * pThisPortal);
  void MergeCameras(CCamera & cam, const CCamera & camPlus);
  void DrawOcclusionAreasIntoCBuffer(CCullBuffer * pCBuffer);
  bool IsValidVisAreaPointer(CVisArea * pVisArea);
  void GetStreamingStatus(int & nLoadedSectors, int & nTotalSectors);
  void GetMemoryUsage(ICrySizer*pSizer);
	bool IsOccludedByOcclVolumes(const AABB& objBox, bool bCheckOnlyIndoorVolumes = false);
	void GetObjectsAround(Vec3 vExploPos, float fExploRadius, PodArray<SRNInfo> * pEntList, bool bSkip_ERF_NO_DECALNODE_DECALS = false, bool bSkipDynamicObjects = false);
	void DoVoxelShape(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<CVoxelObject*> * pAffectedVoxAreas);
	void IntersectWithBox(const AABB & aabbBox, PodArray<CVisArea*> * plstResult, bool bOnlyIfVisible);
  template <class T>
  bool Load_T(T * & f, int & nDataSize, struct SVisAreaManChunkHeader * pVisAreaManagerChunkHeader, std::vector<struct IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo);
  virtual bool Load(FILE * & f, int & nDataSize, struct SVisAreaManChunkHeader * pVisAreaManagerChunkHeader, std::vector<struct IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable);
  VIRTUAL bool SetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, bool bHotUpdate, SHotUpdateInfo * pExportInfo);
	VIRTUAL bool GetCompiledData(byte * pData, int nDataSize, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo);
	VIRTUAL int GetCompiledDataSize(SHotUpdateInfo * pExportInfo);
  void UnregisterEngineObjectsInArea(const SHotUpdateInfo * pExportInfo, PodArray<IRenderNode *> & arrUnregisteredObjects, bool bOnlyEngineObjects);
	void PrecacheLevel(bool bPrecacheAllVisAreas, Vec3 * pPrecachePoints, int nPrecachePointsNum);
	void AddLightSource(CDLight * pLight);
	void AddLightSourceReqursive(CDLight * pLight, CVisArea* pArea, const int32 nDeepness);
	bool IsEntityVisAreaVisibleReqursive(CVisArea * pVisArea, int nMaxReqursion, PodArray<CVisArea*> * pUnavailableAreas, const CDLight * pLight );
  bool IsAABBVisibleFromPoint(AABB & aabb, Vec3 vPos);
  bool FindShortestPathToVisArea(CVisArea * pThisArea, CVisArea * pTargetArea, PodArray<CVisArea*> & arrVisitedAreas, int & nRecursion, const struct Shadowvolume & sv);

	int				GetNumberOfVisArea() const;																				// the function give back the accumlated number of visareas and portals
	IVisArea *GetVisAreaById( int nID ) const;																	// give back the visarea interface based on the id (0..GetNumberOfVisArea()) it can be a visarea or a portal

	VIRTUAL void AddListener( IVisAreaCallback *pListener );
	VIRTUAL void RemoveListener( IVisAreaCallback *pListener );
	
	void MarkAllSectorsAsUncompiled();

	// -------------------------------------

	void GetObjectsByType( PodArray<IRenderNode*> & lstObjects, EERType objType, const AABB * pBBox, float fViewDist = -1);
	void GetObjects( PodArray<IRenderNode*> & lstObjects, const AABB * pBBox, float fViewDist = -1);
  CVisArea * GetCurVisArea() { return m_pCurArea ? m_pCurArea : m_pCurPortal; }
  void GenerateStatObjAndMatTables(std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, SHotUpdateInfo * pExportInfo);
  void OnVisAreaDeleted(IVisArea * pArea);
};

#endif // VisArea_H
