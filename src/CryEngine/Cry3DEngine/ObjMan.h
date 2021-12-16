////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjman.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef CObjManager_H
#define CObjManager_H

#include "StatObj.h"
#include "../RenderDll/Common/Shadow_Renderer.h"
#include "terrain_sector.h"
#include "StlUtils.h"
#include "CullBuffer.h"
#include <SpuUtils.h>

#include <map>
#include <vector>

#include "ObjManCullQueue.h"

#define   ENTITY_MAX_DIST_FACTOR 100
#define MAX_VALID_OBJECT_VOLUME (10000000000.f)

struct CStatObj;
struct IIndoorBase;
struct IRenderNode;
struct ISystem;
struct IDecalRenderNode;

class CVegetation;

class C3DEngine;
struct IMaterial;

#define SMC_EXTEND_FRUSTUM 8
#define SMC_SHADOW_FRUSTUM_TEST 16

#define OCCL_TEST_HEIGHT_MAP	1
#define OCCL_TEST_CBUFFER			2
#define OCCL_TEST_INDOOR_OCCLUDERS_ONLY		4

//! contains stat obj instance group properties (vegetation object properties)
struct StatInstGroup : public IStatInstGroup
{
	StatInstGroup() 
  { 
    pStatObj = 0; 
    ZeroStruct( m_arrSSpriteLightInfo );
  }
	
	CStatObj * GetStatObj()
	{ 
		IStatObj * p = pStatObj;
		return (CStatObj*)p;
	}

	void Update(struct CVars * pCVars, int nGeomDetailScreenRes);

  SVegetationSpriteLightInfo m_arrSSpriteLightInfo[FAR_TEX_COUNT];

	float m_fSpriteSwitchDist;
};

struct SExportedBrushMaterial
{
  int size;
  char material[64];
};

struct SRenderMeshInfoInput
{
	SRenderMeshInfoInput() { memset(this,0,sizeof(*this)); nChunkId=-1; }
	IRenderMesh * pMesh;
	IMaterial * pMat;
	Matrix34	mat;
	int nChunkId;
	IRenderNode * pSrcRndNode;
};

struct SRenderMeshInfoOutput
{
	SRenderMeshInfoOutput() { memset(this,0,sizeof(*this)); }
	IRenderMesh * pMesh;
};

// Inplace object for IStreamable* to cache StreamableMemoryContentSize
struct SStreamAbleObject
{
	explicit SStreamAbleObject( IStreamable *pObj ) : m_pObj(pObj), fCurImportance(-1000.f)
	{
		if( pObj )
			m_nStreamableContentMemoryUsage = pObj->GetStreamableContentMemoryUsage();
		else
			m_nStreamableContentMemoryUsage = 0;
	}

	bool operator==( const SStreamAbleObject &rOther ) const
	{
		return m_pObj == rOther.m_pObj;
	}

	int GetStreamableContentMemoryUsage() const { return m_nStreamableContentMemoryUsage; }
	IStreamable* GetStreamAbleObject() const { return SPU_MAIN_PTR(m_pObj); }
	uint32 GetLastDrawMainFrameId() const
	{



		return m_pObj->GetLastDrawMainFrameId();

	}
	float													fCurImportance;
private:
	SPU_DOMAIN_MAIN IStreamable*	m_pObj;
	int														m_nStreamableContentMemoryUsage;

};

class CObjManager : public Cry3DEngineBase
{
public:
  CObjManager();
  ~CObjManager();

  void PreloadLevelObjects();
  void UnloadObjects(bool bDeleteAll);
  void UnloadVegetationModels();

	void DrawFarObjects(float fMaxViewDist);
  void GenerateFarObjects(float fMaxViewDist);
  void RenderFarObjects();
	void CheckTextureReadyFlag();

  template <class T>
	static int GetItemId(std::vector<T*> * pArray, T * pItem, bool bAssertIfNotFound = true)
	{
    for (uint32 i =0, end = pArray->size() ; i < end; ++i)
      if ((*pArray)[i] == pItem)
        return i;

//    if(bAssertIfNotFound)
  //    assert(!"Item not found");

    return -1;
	}

  template <class T>
  static T * GetItemPtr(std::vector<T*> * pArray, int nId)
  {
    if(nId<0)
      return NULL;

    assert(nId < (int)pArray->size());

    if(nId < (int)pArray->size())
      return (*pArray)[nId];
    else
      return NULL;
  }

  CStatObj *LoadStatObj( const char *szFileName,const char *szGeomName=NULL,IStatObj::SSubObject **ppSubObject=NULL,bool bUseStreaming=true,unsigned long nLoadingFlags=0, 
    const void * m_pData=0, int m_nDataSize=0);
	void GetLoadedStatObjArray( IStatObj** pObjectsArray,int &nCount );
	
	// Deletes object.
	// Only should be called by Release function of CStatObj.
	bool InternalDeleteObject( CStatObj *pObject );

	std::vector<StatInstGroup> m_lstStaticTypes;

  PodArray<ShadowMapFrustum*> * GetShadowFrustumsList(PodArray<CDLight*> * pAffectingLights, const AABB & aabbReceiver, 
		float fObjDistance, uint32 nDLightMask, bool bIncludeNearFrustums);

  PodArray<SVegetationSpriteInfo> m_arrVegetationSprites[MAX_RECURSION_LEVELS][nThreadsNum];

	void MakeShadowCastersList(CVisArea*pReceiverArea, const AABB & aabbReceiver, 
		int dwAllowedTypes, Vec3 vLightPos, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<struct SPlaneObject> * pShadowHull);

	// decal pre-caching
	typedef std::vector< IDecalRenderNode* > DecalsToPrecreate;
	DecalsToPrecreate m_decalsToPrecreate;

	void PrecacheStatObjMaterial(IMaterial* pMaterial, const float fEntDistance, IStatObj *pStatObj );
	void PrecacheMaterial(IMaterial* pMaterial, const float fEntDistance, IRenderMesh *pRenderMesh );
	void PrecacheCharacter(IRenderNode * pObj, const float fImportance, ICharacterInstance * pCharacter, IMaterial* pSlotMat, 
													const Matrix34& matParent, const float fEntDistance, const float fScale, int nMaxDepth);

	NCullQueue::SCullQueue& CullQueue(){return m_cullQueue;}

  typedef std::map<string,CStatObj*,stl::less_stricmp<string> > ObjectsMap;
  ObjectsMap m_nameToObjectMap;

	typedef std::set<CStatObj*> LoadedObjects;
	LoadedObjects m_lstLoadedObjects;

protected:
  CREFarTreeSprites * m_REFarTreeSprites;

#ifdef WIN64
#pragma warning( push )									//AMD Port
#pragma warning( disable : 4267 )
#endif

public:
	int GetLoadedObjectCount() { return m_lstLoadedObjects.size(); }

#ifdef WIN64
#pragma warning( pop )									//AMD Port
#endif

  void RenderObject( IRenderNode * o,
          PodArray<CDLight*> * pAffectingLights, 
          const Vec3 & vAmbColor,
          const AABB & objBox, 
          float fEntDistance, 
          const CCamera & rCam, 
          bool bSunOnly, 
          int8 nThreadId = 0);

  void RenderVegetation( class CVegetation * pEnt, PodArray<CDLight*> * pAffectingLights,
    const AABB & objBox, float fEntDistance, const CCamera & rCam,
    bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo);
  void RenderBrush( class CBrush * pEnt, PodArray<CDLight*> * pAffectingLights,
    const AABB & objBox, float fEntDistance, 
    bool bSunOnly, int8 nThreadId, CVisArea * pVisArea, bool nCheckOcclusion);
  void RenderDecalAndRoad(IRenderNode * pEnt, PodArray<CDLight*> * pAffectingLights,
    const Vec3 & vAmbColor, const AABB & objBox, 
    float fEntDistance, const CCamera & rCam,
    bool bSunOnly, int8 nThreadId);

	void RenderOccluderIntoZBuffer(IRenderNode * pEnt, float fEntDistance, CCullBuffer & rCB, bool bCompletellyInFrustum);
	void RenderObjectDebugInfo(IRenderNode * pEnt, float fEntDistance, const SRendParams & DrawParams);
  void RenderObjectDebugInfo(IRenderNode * pEnt, const CRenderObject * pRendObj);

  float GetXYRadius(int nType);
  bool GetStaticObjectBBox(int nType, Vec3 & vBoxMin, Vec3 & vBoxMax);
  
  Vec3					m_vSkyColor;				// 
  Vec3					m_vSunColor;				// 
	float					m_fSunSkyRel;				//relation factor of sun sky, 1->sun has full part of brightness, 0->sky has full part
	float					m_fILMul;
	float					m_fSkyBrightMul;
	float					m_fSSAOAmount;
  float					m_fGIAmount;

  IStatObj * GetStaticObjectByTypeID(int nTypeID);
  IStatObj * FindStaticObjectByFilename(const char * filename);
  
  float GetBendingRandomFactor();

  bool IsBoxOccluded(	const AABB & objBox, 
											float fDistance, 
											OcclusionTestClient * const __restrict pOcclTestVars, 
											bool bIndoorOccludersOnly, 
											EOcclusionObjectType eOcclusionObjectType);

	void AddDecalToRenderer( float fDistance,
													 IMaterial* pMat,
													 const int nDynLMask,
													 const uint8 sortPrio,
													 Vec3 right,
													 Vec3 up,
													 const UCol& ucResCol,
													 const EParticleBlendType eBlendType,
													 const Vec3& vAmbientColor,
													 Vec3 vPos,
													 const int nAfterWater,
													 CVegetation* pVegetation = 0 );

  // tmp containers (replacement for local static vars)

//  void DrawObjSpritesSorted(PodArray<CVegetation*> *pList, float fMaxViewDist, int useBending);
//	void ProcessActiveShadowReceiving(IRenderNode * pEnt, float fEntDistance, CDLight * pLight, bool bFocusOnHead);

//	void SetupEntityShadowMapping( IRenderNode * pEnt, SRendParams * pDrawParams, float fEntDistance, CDLight * pLight );
	float m_fMaxViewDistanceScale;
	float m_fGSMMaxDistance;
	IShader * m_pShaderOcclusionQuery;
	
//	bool LoadStaticObjectsFromXML(XmlNodeRef xmlVegetation);
	_smart_ptr<CStatObj> m_pDefaultCGF;
	IRenderMesh * m_pRMBox;

	//////////////////////////////////////////////////////////////////////////
	std::vector<_smart_ptr<IStatObj> > m_lockedObjects;
	int m_bLockCGFResources;

	//////////////////////////////////////////////////////////////////////////
	std::vector<CStatObj*> m_checkForGarbage;

  PodArray<SStreamAbleObject> m_arrStreamableObjects;

	//////////////////////////////////////////////////////////////////////////

  void RegisterForStreaming(IStreamable*pObj);
  void UnregisterForStreaming(IStreamable*pObj);
  void UpdateRenderNodeStreamingPrioriry(IRenderNode * pObj, float fEntDistance);

	void GetMemoryUsage(class ICrySizer * pSizer);

//  PodArray<class CBrush*> m_lstBrushContainer;
//  PodArray<class CVegetation*> m_lstVegetContainer;
	void LoadBrushes();
//  void MergeBrushes();
	void ReregisterEntitiesInArea(Vec3 vBoxMin, Vec3 vBoxMax);
//	void ProcessEntityParticles(IRenderNode * pEnt, float fEntDistance);
  void UpdateObjectsStreamingPriority(bool bSyncLoad);
	void ProcessObjectsStreaming();
	
	// implementation parts of ProcessObjectsStreaming
	void ProcessObjectsStreaming_Impl(bool bSyncLoad);
	void ProcessObjectsStreaming_Sort();
	void ProcessObjectsStreaming_Release();
	void ProcessObjectsStreaming_InitLoad(bool bSyncLoad);	
	void ProcessObjectsStreaming_Finish();
	
	// time counters

	static bool IsAfterWater(const Vec3 & vPos, const Vec3 & vCamPos, float fUserWaterLevel = WATER_LEVEL_UNKNOWN);

  void SetupWindBending( IRenderNode *pEnt, SRendParams &pDrawParams, const AABB & objBox );

	void GetObjectsStreamingStatus(I3DEngine::SObjectsStreamingStatus &outStatus);
//	bool ProcessShadowMapCasting(IRenderNode * pEnt, CDLight * pLight);
	void CheckObjectLeaks(bool bDeleteAll = false);
//	bool IsSphereAffectedByShadow(IRenderNode * pCaster, IRenderNode * pReceiver, CDLight * pLight);
//	void MakeShadowCastersListInArea(CBasicArea * pArea, const AABB & boxReceiver, 
//		int dwAllowedTypes, Vec3 vLightPos, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<struct SPlaneObject> * pShadowHull );
	void PrefechObjects();
//	void DrawEntityShadowFrustums(IRenderNode * pEnt);
	
	void FreeNotUsedCGFs();
//	void RenderObjectVegetationNonCastersNoFogVolume( IRenderNode * pEnt,uint32 nDLightMask,
	//	const CCamera & EntViewCamera, 
		//bool bAllInside, float fMaxViewDist, IRenderNodeInfo * pEntInfo);
//	void InitEntityShadowMapInfoStructure(IRenderNode * pEnt);
//	float CalculateEntityShadowVolumeExtent(IRenderNode * pEntity, CDLight * pLight);
//	void MakeShadowBBox(Vec3 & vBoxMin, Vec3 & vBoxMax, const Vec3 & vLightPos, float fLightRadius, float fShadowVolumeExtent);
	void MakeUnitCube();
#ifdef SUPP_HWOBJ_OCCL
	bool IsBoxOccluded_HWOcclQuery( const AABB & objBox, float fDistance, OcclusionTestClient * pOcclTestVars );
#endif
	void BoxCastingShadow_HWOcclQuery( const AABB & objBox,const Vec3& rSunDir,OcclusionTestClient * const pOcclTestVars)
	{
#ifdef USE_CULL_QUEUE
		if(GetCVars()->e_CoverageBuffer)
		{
			const uint32 mainFrameID = GetMainFrameID();
			CullQueue().AddItem(objBox, rSunDir,pOcclTestVars,mainFrameID);
		}
#endif
	}
	bool IsBoxOccluded_HeightMap(const AABB & objBox, float fDistance, EOcclusionObjectType eOcclusionObjectType, OcclusionTestClient * pOcclTestVars );
//	void UnregisterCGFFromTypeTables(CStatObj * pStatObj);	
	bool RenderStatObjIntoZBuffer(IStatObj * pStatObj, Matrix34A & objMatrix, 
		unsigned int nRenderFlags, bool bCompletellyInFrustum,
		CCullBuffer & rCB, IMaterial * pMat);

	//////////////////////////////////////////////////////////////////////////
	// Garbage collection for parent stat objects.
	//////////////////////////////////////////////////////////////////////////
	void CheckForGarbage( CStatObj *pStatObj,bool bAdd );
	void ClearStatObjGarbage();
	
	NCullQueue::SCullQueue m_cullQueue;
	
	float m_fOcclTimeRatio;
  PodArray<COctreeNode*> m_arrUpdateStreamingPrioriryStack;
  static int m_nUpdateStreamingPrioriryRoundId;
  static int s_nLastStreamingMemoryUsage;					//For streaming tools in editor

	static int GetObjectLOD(float fDistance, float fLodRatioNorm, float fRadius);
	static bool RayStatObjIntersection(IStatObj * pStatObj, const Matrix34 & objMat, IMaterial * pMat,
		Vec3 vStart, Vec3 vEnd, Vec3 & vClosestHitPoint, float & fClosestHitDistance, bool bFastTest);
	static bool RayRenderMeshIntersection(IRenderMesh * pRenderMesh, const Vec3 & vInPos, const Vec3 & vInDir, Vec3 & vOutPos, Vec3 & vOutNormal,bool bFastTest,IMaterial * pMat);
  static bool SphereRenderMeshIntersection(IRenderMesh * pRenderMesh, const Vec3 & vInPos, const float fRadius);
  static void CullLightsPerTriangle(IRenderMesh * pRenderMesh, uint32 & nDLightMask, CRNTmpData * pRNTmpData);
  static void FillTerrainTexInfo(IOctreeNode * pOcNode, float fEntDistance, struct SSectorTextureSet * & pTerrainTexInfo, const AABB & objBox);
  PodArray<CVisArea *> m_tmpAreas0, m_tmpAreas1;

  uint8 GetDissolveRef(float fDistNorm, SDissolveTransitionState * pState);
	
private:
	SpuDataStack<IStreamable*>	m_arrStreamableToRelease;
	SpuDataStack<IStreamable*>	m_arrStreamableToLoad;
	SpuDataStack<IStreamable*>	m_arrStreamableToDelete;
	bool m_bNeedProcessObjectsStreaming_Finish;
};


#endif // CObjManager_H
