////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobj.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef STAT_OBJ_H
#define STAT_OBJ_H

// disabled the tracing of leaks for CGFs and Materials for Consoles
#if !defined(PS3) && !defined(XENON)
	#define TRACE_CGF_LEAKS
#endif

class CIndexedMesh;
class CRenderObject;
class CContentCGF;
struct CNodeCGF;
struct CMaterialCGF;
struct phys_geometry;
struct IIndexedMesh;
struct IParticleEffect;

#include "../Cry3DEngine/Cry3DEngineBase.h"
#include "CryArray.h"

#include <IStatObj.h>
#include <IStreamEngine.h>
#include "RenderMeshUtils.h"
#include "GeomQuery.h"

#define MAX_PHYS_GEOMS_TYPES 4

struct SDeformableMeshData 
{
	IGeometry *pInternalGeom;
	int *pVtxMap;
	unsigned int *pUsedVtx;
	int *pVtxTri;
	int *pVtxTriBuf;
	float *prVtxValency;
	Vec3 *pPrevVtx;
	float kViscosity;
};

struct SSpine 
{
	~SSpine() { if (pVtx) delete[] pVtx; if (pVtxCur) delete[] pVtxCur; if (pSegDim) delete[] pSegDim; }
  SSpine() { pVtx=0; pVtxCur=0; pSegDim=0; }

	bool bActive;
	Vec3 *pVtx;
	Vec3 *pVtxCur;
	Vec4 *pSegDim;
	int nVtx;
	float len;
	Vec3 navg;
	int idmat;
	int iAttachSpine;
	int iAttachSeg;
};

class CStatObjFoliage : public IFoliage, public Cry3DEngineBase
{
public:
	CStatObjFoliage() { 
		m_pStatObj=0; m_pRopes=0; m_pRopesActiveTime=0; m_nRopes=0; m_nRefCount=1; m_timeIdle=0; m_pVegInst=0;
		m_pSkinQuats=0; m_pPrevSkinQuats=0; m_nFrameID=0; m_iActivationSource=0; m_flags=0; m_bGeomRemoved=0;
		m_bWasVisible = 1; m_timeInvisible = 0;
		m_bDelete = 0;
	}
	~CStatObjFoliage();
	virtual void AddRef() { m_nRefCount++; }
	virtual void Release() { if (--m_nRefCount<=0) m_bDelete=2; }
	virtual uint32 GetSkeletonPose(int nLod, int nList, const Matrix34& RenderMat34, QuatTS*& pBoneQuatsL, QuatTS*& pBoneQuatsS, QuatTS*& pMBBoneQuatsL, QuatTS*& pMBBoneQuatsS, Vec4 shapeDeformationData[], uint32 &DoWeNeedMorphtargets, uint8*& pRemapTable );

	VIRTUAL int Serialize(TSerialize ser);
	VIRTUAL void SetFlags(int flags);
	VIRTUAL int GetFlags() { return m_flags; }
	VIRTUAL IRenderNode* GetIRenderNode()	{	return m_pVegInst; }
	VIRTUAL int GetBranchCount() { return m_nRopes; }
	VIRTUAL IPhysicalEntity *GetBranchPhysics(int iBranch) { return (unsigned int)iBranch<(unsigned int)m_nRopes ? m_pRopes[iBranch]:0; }

	void OnHit(struct EventPhysCollision *pHit);
	void Update(float dt);
	void BreakBranch(int idx);

	CStatObjFoliage *m_next,*m_prev;
	int m_nRefCount;
	int m_flags;
	CStatObj *m_pStatObj;
	IPhysicalEntity **m_pRopes;
	float *m_pRopesActiveTime;
	int m_nRopes;
	float m_timeIdle,m_lifeTime;
	IFoliage **m_ppThis;
	QuatTS *m_pSkinQuats;
  QuatTS *m_pPrevSkinQuats;
  uint32 m_nFrameID;
	int m_iActivationSource;
	int m_bGeomRemoved;
	IRenderNode *m_pVegInst;
	int m_bWasVisible;
	float m_timeInvisible;
	int m_bDelete;
};

struct SClothTangentVtx {
	int ivtxT; // for each vertex, specifies the iThisVtx->ivtxT edge, which is the closest to the vertex's tangent vector
	Vec3 edge; // that edge's projection on the vertex's normal basis
	int sgnNorm; // sign of phys normal * normal from the basis
};

struct SSkinVtx {
	int bVolumetric;
	int idx[4];
	float w[4];
	Matrix33 M;
};

struct SDelayedSkinParams {
	Matrix34 mtxSkelToMesh;
	IGeometry *pPhysSkel;
	float r;
};

struct SPhysGeomThunk {
	phys_geometry *pgeom;
	int type;
};

struct SPhysGeomArray {
	phys_geometry *operator[](int idx) const 
	{
		if (idx<PHYS_GEOM_TYPE_DEFAULT)
			return idx<(int)m_array.size() ? m_array[idx].pgeom : 0;
		else 
		{
			int i; for(i=m_array.size()-1; i>=0 && m_array[i].type!=idx; i--);
			return i>=0 ? m_array[i].pgeom : 0;
		}
	}
	void SetPhysGeom(phys_geometry *pgeom, int idx=PHYS_GEOM_TYPE_DEFAULT, int type=PHYS_GEOM_TYPE_DEFAULT) 
	{
		int i;
		if (idx<PHYS_GEOM_TYPE_DEFAULT)
			i=idx, idx=type;
		else
			for(i=0; i<(int)m_array.size() && m_array[i].type!=idx; i++);
		if (pgeom)
		{
			if (i>=(int)m_array.size())
				m_array.resize(i+1);
			m_array[i].pgeom = pgeom;
			m_array[i].type = idx;
		} else if (i<(int)m_array.size())
			m_array.erase(m_array.begin()+i);
	}
	int GetGeomCount() { return m_array.size(); }
	int GetGeomType(int idx) { return idx>=PHYS_GEOM_TYPE_DEFAULT ? idx : m_array[idx].type; }
	std::vector<SPhysGeomThunk> m_array;
};

struct CStatObj : public IStatObj, public IStreamCallback, public Cry3DEngineBase
{
  void * m_pData;
  int m_nDataSize;
  CStatObj();
  ~CStatObj();

  static PodArray<CStatObj*> * GetCounter()
  {
    static PodArray<CStatObj*> arrAll;
    return &arrAll;
  }

public:
 	//////////////////////////////////////////////////////////////////////////
	// Variables.
	//////////////////////////////////////////////////////////////////////////
	int m_nUsers; // reference counter
	int m_nChildUsers; // reference counter of all child users.
  uint32 m_nLastDrawMainFrameId;

	IRenderMesh *m_pRenderMesh;
	// Render mesh for occlusion only.
	_smart_ptr<IRenderMesh> m_pRenderMeshOcclusion;
	CIndexedMesh *m_pIndexedMesh;

	string m_szFileName;
	string m_szGeomName;
	string m_szProperties;

	int m_nLoadedTrisCount;
	int m_nLoadedVertexCount;
	int m_nRenderTrisCount;
	int m_nRenderMatIds;

	// Default material.
	_smart_ptr<IMaterial> m_pMaterial;
	uint32 m_nMaterialLayersMask;

  float m_fObjectRadius;
	float m_fRadiusHors;
	float m_fRadiusVert;

	Vec3 m_vBoxMin, m_vBoxMax, m_vVegCenter;
	
	SPhysGeomArray m_arrPhysGeomInfo;
	ITetrLattice *m_pLattice;
	IGeometry *m_pSrcPhysMesh;
	IStatObj *m_pLastBooleanOp;
	float m_lastBooleanOpScale;

	_smart_ptr<CStatObj> m_arrpLowLODs[MAX_STATOBJ_LODS_NUM];
	CStatObj *m_pLod0; // Level 0 stat object. (Pointer to the original object of the LOD)
	unsigned int m_nMinUsableLod0 : 8;  // What is the minimal LOD that can be used as LOD0.
	unsigned int m_nMaxUsableLod  : 8;  // What is the maximal LOD that can be used.
	unsigned int m_nLoadedLodsNum : 8;  // How many lods loaded.


	int m_nFileNodeId;
	char m_FileNodeName[8];

	//////////////////////////////////////////////////////////////////////////
	// Externally set flags from enum EStaticObjectFlags.
	//////////////////////////////////////////////////////////////////////////
	int m_nFlags;

	//////////////////////////////////////////////////////////////////////////
	// Internal Flags.
	//////////////////////////////////////////////////////////////////////////
	unsigned int m_bCheckGarbage : 1;
	unsigned int m_bUseStreaming : 1;
  unsigned int m_bLodsLoaded : 1;
	unsigned int m_bDefaultObject : 1;
	unsigned int m_bOpenEdgesTested : 1;
	unsigned int m_bSubObject : 1;       // This is sub object.
	unsigned int m_bVehicleOnlyPhysics : 1;  // Object can be used for collisions with vehicles only
	unsigned int m_bBreakableByGame : 1; // material is marked as breakable by game
	unsigned int m_bSharesChildren : 1; // means its subobjects belong to another parent statobj
	unsigned int m_bHasDeformationMorphs : 1;
	unsigned int m_bTmpIndexedMesh : 1; // indexed mesh is temporary and can be deleted after MakeRenderMesh
	unsigned int m_bUnmergable : 1; // Set if sub objects cannot be merged together to the single render merge.
	unsigned int m_bMerged : 1; // Set if sub objects merged together to the single render merge.
	unsigned int m_bForInternalUse : 1; // Set if sub objects merged together to the single render merge.
	unsigned int m_bLowSpecLod0Set : 1;
	unsigned int m_bHaveOcclusionProxy : 1; // If this stat object or its childs have occlusion proxy.
  unsigned int m_bLodsAreLoadedFromSeparateFile : 1;
	unsigned int m_bNoHitRefinement : 1; // doesn't refine bullet hits against rendermesh
	unsigned int m_bDontOccludeExplosions : 1; // don't act as an explosion occluder in physics
	int m_idmatBreakable;	// breakable id for the physics
	//////////////////////////////////////////////////////////////////////////
	
  // streaming
  int  m_nStreamingFinishedMainFrameId;
  int m_nRenderMeshMemoryUsage;
  int m_nMergedMemoryUsage;
  int m_arrRenderMeshesPotentialMemoryUsage[2];
  IReadStreamPtr m_pReadStream;

	//////////////////////////////////////////////////////////////////////////

	SDeformableMeshData *m_pDeformableMeshData;
	uint16 *m_pMapFaceToFace0;
	SClothTangentVtx *m_pClothTangentsData;
	SSkinVtx *m_pSkinInfo;
	SDelayedSkinParams *m_pDelayedSkinParams;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Bendable Foliage.
	//////////////////////////////////////////////////////////////////////////
	SSpine *m_pSpines;
	int m_nSpines;
	struct SMeshBoneMapping *m_pBoneMapping;
	DynArray<uint16> m_chunkBoneIds;
	//IRenderMesh *m_pRenderMeshSkinned;
	int m_nPhysFoliages;
	//////////////////////////////////////////////////////////////////////////

  static PodArray<CStatObj*> m_arrStatObjForRenderMeshDelete;

	//////////////////////////////////////////////////////////////////////////
	// for debug purposes
	//////////////////////////////////////////////////////////////////////////
#ifdef TRACE_CGF_LEAKS
	string	m_sLoadingCallstack;
#endif

private:
	typedef std::set<std::pair<CStatObj*, IRenderMesh*> > ObjMeshPairs;

	//////////////////////////////////////////////////////////////////////////
	// Sub objects.
	//////////////////////////////////////////////////////////////////////////
	DynArray<SSubObject> m_subObjects;
  PodArray<int> * m_pSubObjectsForRendering;
	CStatObj *m_pParentObject;  // Parent object (Must not be smart pointer). 
	CStatObj *m_pClonedSourceObject;  // If this is cloned object, pointer to original source object (Must not be smart pointer).
	_smart_ptr<CStatObj> m_pMergedObject;
	int m_nSubObjectMeshCount;

	//////////////////////////////////////////////////////////////////////////
	// Special AI/Physics parameters.
	//////////////////////////////////////////////////////////////////////////
  float m_aiVegetationRadius;
	float m_phys_mass;
	float m_phys_density;

  //////////////////////////////////////////////////////////////////////////
  // used only in the editor
  //////////////////////////////////////////////////////////////////////////
  float * m_pHeightmap;
  int m_nHeightmapSize;
  float m_fOcclusionAmount;

	//////////////////////////////////////////////////////////////////////////
	// METHODS.
	//////////////////////////////////////////////////////////////////////////
public:
	//////////////////////////////////////////////////////////////////////////
	// Fast non virtual access functions.
	ILINE IStatObj::SSubObject& SubObject( int nIndex ) { return m_subObjects[nIndex]; };
	ILINE int SubObjectCount() const { return m_subObjects.size(); };
	//////////////////////////////////////////////////////////////////////////

	IIndexedMesh *GetIndexedMesh(bool bCreatefNone=false);
	void ReleaseIndexedMesh(bool bRenderMeshUpdated=false);
	ILINE const Vec3 GetVegCenter() { return m_vVegCenter; }
	ILINE float GetRadius() { return m_fObjectRadius; }

	VIRTUAL void SetFlags( int nFlags ) { m_nFlags = nFlags; };
	VIRTUAL int  GetFlags() { return m_nFlags; };

	VIRTUAL unsigned int  GetVehicleOnlyPhysics()		{return m_bVehicleOnlyPhysics;};
	VIRTUAL int						GetIDMatBreakable()				{return m_idmatBreakable;};
	VIRTUAL unsigned int	GetBreakableByGame()			{return m_bBreakableByGame;};

  // Loader
  bool LoadCGF( const char *filename,bool bLod=false,unsigned long nLoadingFlags=0 );
	bool LoadCGF_FromMemBlock( ObjMeshPairs* ppOutputMeshes, const void * pData, int nDataSize, bool bLod, bool bPhysicalize );

	//////////////////////////////////////////////////////////////////////////
	void SetMaterial( IMaterial *pMaterial );
	IMaterial* GetMaterial() { return m_pMaterial; };
	//////////////////////////////////////////////////////////////////////////

  void RenderInternal(CRenderObject * pRenderObject, const SRenderObjectModifier * pROM, int nThreadId, int nLod);
	void RenderSubObject(CRenderObject * pRenderObject, const SRenderObjectModifier * pROM, int nThreadId, int nLod, 
											 int nSubObjId, const Matrix34A & renderTM, const Matrix34A & renderPrevTM);
  void RenderSubObjectInternal(CRenderObject * pRenderObject, const SRenderObjectModifier * pROM, int nThreadId, int nLod);
  VIRTUAL void Render(const SRendParams & rParams);
	void RenderRenderMesh(CRenderObject * pObj, struct SInstancingInfo * pInstInfo, int nThreadId, const SRenderObjectModifier * pROII);
  static void RenderRenderMeshReal(CRenderObject * pObj, IRenderMesh * pRenderMesh, SInstancingInfo * pInstInfo, const SRenderObjectModifier * pROII);
	phys_geometry* GetPhysGeom( int nGeomType = PHYS_GEOM_TYPE_DEFAULT ) { return m_arrPhysGeomInfo[nGeomType]; }
	void SetPhysGeom( phys_geometry *pPhysGeom, int nGeomType=PHYS_GEOM_TYPE_DEFAULT ) 
	{ 
		if(m_arrPhysGeomInfo[nGeomType])
			GetPhysicalWorld()->GetGeomManager()->UnregisterGeometry(m_arrPhysGeomInfo[nGeomType]);
		m_arrPhysGeomInfo.SetPhysGeom(pPhysGeom, nGeomType); 
	}
	ITetrLattice *GetTetrLattice() { return m_pLattice; }

  float GetAIVegetationRadius() const {return m_aiVegetationRadius;}
  void SetAIVegetationRadius(float radius) {m_aiVegetationRadius = radius;}

  //! Refresh object ( reload shaders or/and object geometry )
	VIRTUAL void Refresh(int nFlags);

  IRenderMesh * GetRenderMesh() { return m_pRenderMesh; };
  void SetRenderMesh( IRenderMesh *pRM );

  const char *GetFilePath()    { return (m_szFileName); }
	void SetFilePath(const char * szFileName) { m_szFileName = szFileName; }
  const char *GetGeoName()    { return (m_szGeomName); }
  bool IsSameObject(const char * szFileName, const char * szGeomName);

  //set object's min/max bbox 
  void  SetBBoxMin(const Vec3 &vBBoxMin) { m_vBoxMin=vBBoxMin; }  
  void  SetBBoxMax(const Vec3 &vBBoxMax) { m_vBoxMax=vBBoxMax; }
  Vec3 GetBoxMin() { return m_vBoxMin; }
  Vec3 GetBoxMax() { return m_vBoxMax; }
  AABB GetAABB() {  return AABB(m_vBoxMin,m_vBoxMax);  }

	float ComputeExtent(GeomQuery& geo, EGeomForm eForm);
	void GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm)
	{
		geo.GetExtent(this, eForm);
		int i = geo.GetRandomPart();
		if (i == 0)
		{
			if (m_pRenderMesh)
			{
				m_pRenderMesh->GetRandomPos(ran, geo.GetPart(0), eForm);
				return;
			}
		}
		else
		{
			IStatObj::SSubObject* pSub = GetSubObject(i-1);
			if (pSub && pSub->nType==STATIC_SUB_OBJECT_MESH && pSub->pStatObj)
			{
				pSub->pStatObj->GetRandomPos(ran, geo.GetPart(i), eForm);
				return;
			}
		}
		ran.vPos.zero();
		ran.vNorm.zero();
	}

  VIRTUAL Vec3 GetHelperPos(const char * szHelperName);
  VIRTUAL const Matrix34& GetHelperTM(const char * szHelperName);

  float & GetRadiusVert() { return m_fRadiusVert; }
  float & GetRadiusHors() { return m_fRadiusHors; }
  
	virtual int AddRef();	

	virtual int Release();
	int GetNumRefs() const { return m_nUsers; }

  VIRTUAL bool IsDefaultObject() { return (m_bDefaultObject); }

  int GetLoadedTrisCount() { return m_nLoadedTrisCount; }
	int GetRenderTrisCount() { return m_nRenderTrisCount; }

	// Load LODs
	void SetLodObject( int nLod,CStatObj *pLod );
	void LoadLowLODs(bool bUseStreaming,unsigned long nLoadingFlags);
	// Free render resources for unused upper LODs.
	void CleanUnusedLods();

  VIRTUAL void FreeIndexedMesh();
  bool RenderDebugInfo( CRenderObject *pObj, const SRenderObjectModifier *pROM );
  void DrawMatrix(const Matrix34 & pMat);

  //! Release method.
  void GetMemoryUsage(class ICrySizer* pSizer);
  
  void ShutDown();
  void Init();

//  void CheckLoaded();
  VIRTUAL IStatObj* GetLodObject( int nLodLevel,bool bReturnNearest=false );
  VIRTUAL IStatObj* GetLowestLod();

  virtual int FindNearesLoadedLOD(int nLodIn, const SRendParams & rParams);

	// interface IStreamCallback -----------------------------------------------------
  
	virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
	virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);

	// -------------------------------------------------------------------------------

	virtual void StartStreaming(bool bFinishNow, IReadStream_AutoPtr* ppStream);
	void UpdateStreamingPrioriryInternal(const Matrix34A & objMatrix, float fDistance);

  void MakeCompiledFileName(char * szCompiledFileName, int nMaxLen);

  bool IsPhysicsExist();
  static void SetupBending(CRenderObject * pObj, Vec2 const& vBending, float fRadiusVert, float fRadiusHor);
  bool IsSphereOverlap(const Sphere& sSphere);
	void Invalidate( bool bPhysics=false, float tolerance=0.05f );

	void AnalizeFoliage(IRenderMesh * pRenderMesh, CContentCGF *pCGF=0);
  void FreeFoliageData();
	void CopyFoliageData(IStatObj *pObjDst, bool bMove=false, IFoliage *pSrcFoliage=0, int *pVtxMap=0, primitives::box *pMoveBoxes=0,int nMovedBoxes=-1);
	int PhysicalizeFoliage(IPhysicalEntity *pTrunk, const Matrix34 &mtxWorld, IFoliage *&pRes, float lifeTime=0.0f, int iSource=0);
	int SerializeFoliage(TSerialize ser, ISkinnable *pFoliage);

	IStatObj *UpdateVertices(strided_pointer<Vec3> pVtx, strided_pointer<Vec3> pNormals, int iVtx0,int nVtx, int *pVtxMap=0);
	bool HasSkinInfo(float skinRadius=-1.0f) { return m_pSkinInfo && (skinRadius<0.0f || m_pSkinInfo[m_nLoadedVertexCount].w[0]==skinRadius); }
	void PrepareSkinData(const Matrix34 &mtxSkelToMesh, IGeometry *pPhysSkel, float r=0.0f);
	IStatObj *SkinVertices(strided_pointer<Vec3> pSkelVtx, const Matrix34 &mtxSkelToMesh);

	//////////////////////////////////////////////////////////////////////////
	// Sub objects.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL int GetSubObjectCount() const { return m_subObjects.size(); }
	VIRTUAL void SetSubObjectCount( int nCount );
	VIRTUAL IStatObj::SSubObject* FindSubObject( const char *sNodeName );
  VIRTUAL IStatObj::SSubObject* FindSubObject_StrStr( const char *sNodeName );
	VIRTUAL IStatObj::SSubObject* FindSubObject_CGA( const char *sNodeName );
	VIRTUAL IStatObj::SSubObject* GetSubObject( int nIndex )
	{
		if ( nIndex >= 0 && nIndex < (int)m_subObjects.size() )
			return &m_subObjects[nIndex];
		else
			return 0;
	}
	VIRTUAL bool RemoveSubObject( int nIndex );
	VIRTUAL IStatObj* GetParentObject() const { return m_pParentObject; }
	VIRTUAL IStatObj* GetCloneSourceObject() const { return m_pClonedSourceObject; }
	VIRTUAL bool IsSubObject() const { return m_bSubObject; };
	VIRTUAL bool CopySubObject( int nToIndex,IStatObj *pFromObj,int nFromIndex );
	VIRTUAL int PhysicalizeSubobjects(IPhysicalEntity *pent, const Matrix34 *pMtx, float mass,float density=0.0f, int id0=0, 
		strided_pointer<int> pJointsIdMap=0, const char *szPropsOverride=0);
	virtual IStatObj::SSubObject& AddSubObject( IStatObj *pStatObj );
	VIRTUAL int Physicalize(IPhysicalEntity *pent, pe_geomparams *pgp, int id=-1, const char *szPropsOverride=0);
	//////////////////////////////////////////////////////////////////////////	

	VIRTUAL bool SaveToCGF( const char *sFilename,IChunkFile** pOutChunkFile=NULL );

	VIRTUAL IStatObj* Clone(bool bCloneChildren=true, bool nDynamic=false);
	VIRTUAL IStatObj* Clone(bool bCloneGeometry,bool bCloneChildren,bool bMeshesOnly);

	VIRTUAL void MarkAsDeformable(IGeometry *pInternalGeom = 0);

	VIRTUAL int SetDeformationMorphTarget(IStatObj *pDeformed);
	VIRTUAL int SubobjHasDeformMorph(int iSubObj);
	VIRTUAL IStatObj *DeformMorph(const Vec3 &pt,float r,float strength, IRenderMesh *pWeights=0);

	VIRTUAL IStatObj *HideFoliage();

	VIRTUAL int Serialize(TSerialize ser);

	// Get object properties as loaded from CGF.
	VIRTUAL const char* GetProperties() { return m_szProperties.c_str(); };

	VIRTUAL bool GetPhysicalProperties( float &mass,float &density );

	VIRTUAL IStatObj *GetLastBooleanOp(float &scale) { scale=m_lastBooleanOpScale; return m_pLastBooleanOp; }

	// Intersect ray with static object.
	// Ray must be in object local space.
	VIRTUAL bool RayIntersection( SRayHitInfo &hitInfo,IMaterial *pCustomMtl=0 );

	VIRTUAL void DebugDraw( const SGeometryDebugDrawInfo &info );
	VIRTUAL void GetStatistics( SStatistics &stats );
	//////////////////////////////////////////////////////////////////////////

	float GetOcclusionAmount();

	IParticleEffect* GetSurfaceBreakageEffect( const char *sType );

  void CheckUpdateObjectHeightmap();
  float GetObjectHeight(float x, float y);
  int GetMinUsableLod();

	int CountChildReferences();
  void ReleaseStreamableContent();
  int GetStreamableContentMemoryUsage();
  int GetLod(Matrix34 & mat, float fEntDistance, IRenderNode * pObj);
	int GetLodFromScale(float fScale, float fLodRatioNormalized, float fEntDistance, bool bFoliage);
  bool UpdateStreamableComponents(float fImportance, Matrix34A & objMatrix, IRenderNode * pObj, float fDistance);
  void UpdateSubObjectsMerging();
  void GetStreamableName(string & sName)
  {
    sName = m_szFileName;
    if(m_szGeomName.length())
    {
      const char * pGeomName = m_szGeomName.c_str();
      sName += " - ";
      sName += m_szGeomName;
    }
  };
  static void FillRenderObject(const SRendParams & rParams, IRenderNode * pRenderNode, IMaterial * pMaterial, 
    float fRadiusVert, float fRadiusHors, SInstancingInfo * pInstInfo, CRenderObject * pObj, SRenderObjectModifier * pROM);
	virtual uint32 GetLastDrawMainFrameId() { return m_nLastDrawMainFrameId; }

protected:
	void MakeRenderMesh();
	bool MergeSubObjectsRenderMeshes();
	bool MergeSubObjectsRenderMeshes( int nLod );
	void UnMergeSubObjectsRenderMeshes();

//	bool LoadCGF_Info( const char *filename );
	CStatObj* MakeStatObjFromCgfNode( CContentCGF *pCGF,CNodeCGF *pNode,int nLoadingFlags,AABB &commonBBox );
	void ParseProperties();

	void CalcRadiuses();
	void GetStatisticsNonRecursive( SStatistics &stats );

	void SavePhysicalizeData( CNodeCGF *pNode );
	void PhysicalizeCompiled( CNodeCGF *pNode, int bAppend=0 );
	bool PhysicalizeGeomType( int nGeomType, CMesh &mesh, float tolerance=0.05f, int bAppend=0);
	bool RegisterPhysicGeom( int nGeomType,phys_geometry *pPhysGeom );
	void AssignPhysGeom( int nGeomType,phys_geometry *pPhysGeom,int bAppend=0 );

	// Creates static object contents from mesh.
	// Return true if successful.
	bool SetFromMesh( IRenderMesh** ppOutputMesh, CMesh *pMesh, bool bDoRenderMesh );

  const char* stristr(const char* szString, const char* szSubstring)
  {
    int nSuperstringLength = (int)strlen(szString);
    int nSubstringLength = (int)strlen(szSubstring);

    for (int nSubstringPos = 0; nSubstringPos <= nSuperstringLength - nSubstringLength; ++nSubstringPos)
    {
      if (strnicmp(szString+nSubstringPos, szSubstring, nSubstringLength) == 0)
        return szString+nSubstringPos;
    }
    return NULL;
  }
};


//////////////////////////////////////////////////////////////////////////
// Wrapper around CStatObj that allow rendering of static object with specified parameters.
//////////////////////////////////////////////////////////////////////////
class CStatObjWrapper : public CStatObj
{
	virtual void Render(const SRendParams & rParams);
private:
	// Reference Static Object this wrapper was created for.
	CStatObj *m_pReference;
};

//////////////////////////////////////////////////////////////////////////
inline void InitializeSubObject( IStatObj::SSubObject &so )
{
	so.localTM.SetIdentity();
	so.name = "";
	so.properties = "";
	so.nType = STATIC_SUB_OBJECT_MESH;
	so.pWeights = 0;
	so.nParent = -1;
	so.tm.SetIdentity();
	so.bIdentityMatrix = true;
	so.bHidden = false;
	so.helperSize = Vec3(0,0,0);
	so.pStatObj = 0;
}

#endif // STAT_OBJ_H
