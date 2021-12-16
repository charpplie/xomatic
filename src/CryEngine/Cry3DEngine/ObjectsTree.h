#ifndef	__INCLUDE_CRY3DENGINE_OBJECTTREE_H
#define __INCLUDE_CRY3DENGINE_OBJECTTREE_H

#pragma once

#define OCTREENODE_RENDER_FLAG_OBJECTS		1
#define OCTREENODE_RENDER_FLAG_OCCLUDERS	2
#define OCTREENODE_RENDER_FLAG_CASTERS		4
#define OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES 8

#define OCTREENODE_CHUNK_VERSION 3

enum EOcTeeNodeListType
{
  eMain,
  eCasters,
  eOccluders,
  eSprites,
  eLights,
};

template <class T> struct TDoublyLinkedList
{
  T * m_pFirstNode, * m_pLastNode;

  TDoublyLinkedList() 
  { 
    m_pFirstNode = m_pLastNode = NULL; 
  }

  ~TDoublyLinkedList() 
  { 
    assert(!m_pFirstNode && !m_pLastNode); 
  }

  void insertAfter(T * pAfter, T * pObj)
  {
    pObj->m_pPrev = pAfter;
    pObj->m_pNext = pAfter->m_pNext;
    if (pAfter->m_pNext == NULL)
      m_pLastNode = pObj;
    else
      pAfter->m_pNext->m_pPrev = pObj;
    pAfter->m_pNext = pObj;
  }

  void insertBefore(T * pBefore, T * pObj)
  {
    pObj->m_pPrev = pBefore->m_pPrev;
    pObj->m_pNext = pBefore;
    if (pBefore->m_pPrev == NULL)
      m_pFirstNode = pObj;
    else
      pBefore->m_pPrev->m_pNext = pObj;
    pBefore->m_pPrev    = pObj;
  }

  void insertBeginning(T * pObj)
  {
    if (m_pFirstNode == NULL)
    {
      m_pFirstNode = pObj;
      m_pLastNode  = pObj;
      pObj->m_pPrev = NULL;
      pObj->m_pNext = NULL;
    }
    else
      insertBefore(m_pFirstNode, pObj);
  }

  void insertEnd(T * pObj)
  {
    if (m_pLastNode == NULL)
      insertBeginning(pObj);
    else
      insertAfter(m_pLastNode, pObj);
  }

  void remove(T * pObj)
  {
    if (pObj->m_pPrev == NULL)
      m_pFirstNode = pObj->m_pNext;
    else
      pObj->m_pPrev->m_pNext = pObj->m_pNext;

    if (pObj->m_pNext == NULL)
      m_pLastNode = pObj->m_pPrev;
    else
      pObj->m_pNext->m_pPrev = pObj->m_pPrev;

    pObj->m_pPrev = pObj->m_pNext = NULL;
  }
};

struct SInstancingInfo
{
  SInstancingInfo() { pStatInstGroup=0; aabb.Reset(); fMinSpriteDistance = 10000.f; bInstancingInUse=0; }
  StatInstGroup * pStatInstGroup;
  DynArray<CVegetation*> arrInstances;
  DynArray16<SInstanceInfo> arrMats;
  DynArray16<SVegetationSpriteInfo> arrSprites;
  AABB aabb;
  float fMinSpriteDistance;
  bool bInstancingInUse;
};

class COctreeNode : public IOctreeNode, Cry3DEngineBase
{
public:
	COctreeNode(int nSID, const AABB & box, struct CVisArea * pVisArea, COctreeNode * pParent = NULL);
	~COctreeNode();
	
//  void UpdateSceneMerging();
  bool HasChildNodes();
	void InsertObject(IRenderNode * pObj, const AABB & objBox, const float fObjRadiusSqr, const Vec3 & vObjCenter);
	bool DeleteObject(IRenderNode * pObj);
  void Render_Object_Nodes(bool bNodeCompletelyInFrustum, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, int8 nThreadId);
  bool Render_Occl_Nodes(const CCamera & rCam, CCullBuffer & rCB, int nRenderMask, int8 nThreadId);
  void RenderMT(const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor);
  void RenderNR_Object_Nodes(const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, PodArray<COctreeNode*> * pArrNodes, int8 nThreadId);
  void PreRender_Object_Nodes(bool bNodeCompletelyInFrustum, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, PodArray<COctreeNode*> & rVisNodes, int8 nThreadId);
  void RenderContent(const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, int8 nThreadId);
  void RenderVegetations(TDoublyLinkedList<IRenderNode> & arrObjects, const CCamera & rCam, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo);
  void RenderCommonObjects(TDoublyLinkedList<IRenderNode> & lstObjects, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo);
  void RenderDecalsAndRoads(TDoublyLinkedList<IRenderNode> & lstObjects, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo);
  void RenderBrushes(TDoublyLinkedList<IRenderNode> & lstObjects, const CCamera & rCam, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo);
  PodArray<CDLight*> * GetAffectingLights();
  void AddLightSource(CDLight * pSource);
  void CheckInitAffectingLights();
	void FillShadowCastersList(bool bNodeCompletellyInFrustum , CDLight * pLight, struct ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest);
	void FillSubSurfCastersList(struct ShadowMapFrustum * pFr);
	void FillIndirectLightingCastersList(const CDLight * pLight, struct ShadowMapFrustum * pFr);

  void MarkAsUncompiled();
	COctreeNode * FindNodeContainingBox(const AABB & objBox);
	void MoveObjectsIntoList(PodArray<SRNInfo> * plstResultEntities, const AABB * pAreaBox, bool bRemoveObjects = false, bool bSkipDecals = false, bool bSkip_ERF_NO_DECALNODE_DECALS = false, bool bSkipDynamicObjects = false, EERType eRNType = eERType_TypesNum);
	int PhysicalizeVegetationInBox(const AABB &bbox);
	int DephysicalizeVegetationInBox(const AABB &bbox);
  int GetData(byte * & pData, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo);

	ILINE const AABB & GetBBox() { return m_nodeBox; }
	const AABB & GetObjectsBBox() { return m_objectsBox; }
	void DeleteObjectsByFlag(int nRndFlag);
  void UnregisterEngineObjectsInArea(const SHotUpdateInfo * pExportInfo, PodArray<IRenderNode *> & arrUnregisteredObjects, bool bOnlyEngineObjects);
	uint32 GetLastVisFrameId() { return m_nLastVisFrameId; }
	void GetObjectsByType(PodArray<IRenderNode*> & lstObjects, EERType objType, const AABB * pBBox, float fViewDist = -1);
	void GetObjects(PodArray<IRenderNode*> & lstObjects, const AABB * pBBox, float fViewDist = -1);
  bool IsObjectTypeInTheBox(EERType objType, const AABB & WSBBox);
  void GetObjectsForIntegrationIntoTexture(PodArray<SBuildTerrainTextureParams::SBuildItem> & arrItems, const AABB * pBBox, float fViewDist);
	bool CleanUpTree();
	int GetObjectsCount(EOcTeeNodeListType eListType);
  int	 SaveObjects(class CMemoryBlock * pMemBlock, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, const SHotUpdateInfo * pExportInfo);
  int	 LoadObjects(byte * pPtr, byte * pEndPtr, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian);
	bool IsRightNode(const AABB & objBox, const float fObjRadius, float fObjMaxViewDist);
	void GetMemoryUsage(ICrySizer * pSizer);
	IRenderNode * FindTerrainSectorVoxObject(const AABB & objBox);
	CTerrainNode * GetTerrainNode() { return m_pTerrainNode; }
	void UpdateTerrainNodes();
	bool RayObjectsIntersection2D( Vec3 vStart, Vec3 vEnd, Vec3 & vClosestHitPoint, float & fClosestHitDistance, EERType eERType );
  bool RayVoxelIntersection2D( Vec3 vStart, Vec3 vEnd, Vec3 & vClosestHitPoint, float & fClosestHitDistance );

  template <class T>
  int Load_T(T * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, AABB * pBox);
  int Load(FILE * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, AABB * pBox);
  int Load(uint8 * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, AABB * pBox);

  static void FreeLoadingCache();
  void GenerateStatObjAndMatTables(std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, SHotUpdateInfo * pExportInfo);
  static void ReleaseEmptyNodes();
  bool IsEmpty();
  bool HasObjects();
  bool UpdateStreamingPrioriry(PodArray<COctreeNode*> & arrRecursion);
  void CheckManageVegetationSprites(float fNodeDistance, int nMaxFrames);

protected:
	AABB GetChildBBox(int nChildId);
	void CompileObjects();
  void CompileCharacter( ICharacterInstance * pChar, unsigned char &nInternalFlags);
	void CompileObjectsBrightness();
	float GetNodeObjectsMaxViewDistance();
	
	// Check if min spec specified in render node passes current server config spec.
	bool CheckRenderFlagsMinSpec( uint32 dwRndFlags );
  
  void LinkObject(IRenderNode * pObj, EERType eERType, bool bPushFront = true);
  void UnlinkObject(IRenderNode * pObj);

  static int Cmp_OctreeNodeSize(const void* v1, const void* v2);

  uint32 m_nOccludedFrameId;
	uint32 m_renderFlags;
	uint32 m_errTypesBitField;
  AABB m_objectsBox;
  float m_fObjectsMaxViewDist;
  uint32 m_nLastVisFrameId;
  OcclusionTestClient m_occlTestState;

	COctreeNode * m_arrChilds[8];
  TDoublyLinkedList<IRenderNode> m_arrObjects[eRNListType_ListsNum];
  PodArray<SVegetationSpriteInfo> m_arrVegetationSprites;
	PodArray<IRenderNode *> m_lstOccluders;
  PodArray<SCasterInfo> m_lstCasters;
  PodArray<SCasterInfo> m_lstSubSurfCasters;
	AABB m_nodeBox;
  Vec3 m_vNodeCenter;
  float m_fNodeRadius;
  PodArray<CDLight*> m_lstAffectingLights; uint32 m_nLightMaskFrameId;
	uint32 m_bHasLights : 1, m_bHasVoxels : 1;
  uint32 m_bHasRoads : 1;
	Vec3 m_vSunDirReady;
  static CMemoryBlock m_mbLoadingCache;

  COctreeNode * m_pParent;
  uint32 nFillShadowCastersSkipFrameId;
  float m_fNodeDistance;
  int m_bNodeCompletelyInFrustum;
  int m_nManageVegetationsFrameId;
  int m_nSID;

public:
  static PodArray<COctreeNode*> m_arrEmptyNodes;
};

/*class CSceneTree : public Cry3DEngineBase
{
public:
  CSceneTree(const AABB & box, CSceneTree * pParent) :
    m_plstAreaBrush(0),
    m_bAreaBrushesUpdateRequested(0),
    m_bHasShadowCasters(0),
    m_nLightMaskFrameId(0),
    m_nGSMFrameId(0)
  {
    ZeroStruct(m_arrChilds);

    m_nodeBox = box;
    m_pParent = pParent;
  }

  ~CSceneTree()
  {
    FreeAreaBrushes(false);
    for(int i=0; i<8; i++) 
      SAFE_DELETE(m_arrChilds[i]);
  }

  void Render(const CCamera & rCam, bool bCompletelyInFrustum);
  void RequestUpdate(const AABB & areaBox);
  void CheckUpdateAreaBrushes();
  void FreeAreaBrushes(bool bRecursive);
  AABB GetChildBBox(int nChildId);
  bool HasChildNodes();
  bool FillShadowCastersList(bool bAllIn, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest);

  void CheckInitAffectingLights();
  PodArray<CDLight*> * GetAffectingLights();
  void AddLightSource(CDLight * pSource);

  PodArray<CBrush*> * m_plstAreaBrush;
  bool m_bAreaBrushesUpdateRequested;
  AABB m_nodeBox;
  CSceneTree * m_pParent;
  CSceneTree * m_arrChilds[8];
  bool m_bHasShadowCasters;
  PodArray<CDLight*> m_lstAffectingLights; 
  uint32 m_nLightMaskFrameId;
  uint32 m_nGSMFrameId;
  OcclusionTestClient m_occlTestState;
};*/

#endif
