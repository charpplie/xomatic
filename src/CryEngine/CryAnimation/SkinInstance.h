
//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:CSkinInstance.h
//  Declaration of CSkinInstance class
//
//	History:
//	August 16, 2004: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#ifndef _SKIN_INSTANCE_HEADER_
#define _SKIN_INSTANCE_HEADER_

struct CryEngineDecalInfo;
struct CryParticleSpawnInfo;

extern f32 g_YLine;
extern uint32 g_nCharInstanceLoaded;
extern uint32 g_nSkinInstanceLoaded;
extern uint32 g_nCharInstanceDeleted;
extern uint32 g_nSkinInstanceDeleted;

#include <ICryAnimation.h>
#include <CGFContent.h>

#include "Model.h"							//embedded
#include "CryModEffMorph.h"			//embedded

#include "Morphing.h"						//embedded
#include "AttachmentManager.h"	//embedded
#include "DecalManager.h"				//embedded


//////////////////////////////////////////////////////////////////////
struct AnimData;
class CryCharManager;
class CFacialInstance;
class CModelMesh;
class CCamera;
class CSkeletonAnim;
class CSkeletonPose;




enum
{
	FLAG_VISIBLE       = 1,
	FLAG_ENABLE_DECALS = 1 << 1,
	FLAG_DEFAULT_FLAGS = FLAG_ENABLE_DECALS
};




struct CryCharInstanceRenderParams
{
	ColorF m_Color;
	int m_nFlags; 
	CDLight m_ambientLight;
};


///////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of ICharacterInstance interface, the main interface in the Animation module
///////////////////////////////////////////////////////////////////////////////////////////////
class CSkinInstance : public ICharacterInstance,public ISkinnable
{
	friend class CAttachmentManager;
	friend class CAttachment;
	friend class CModelMesh;

protected:
	~CSkinInstance(); // dtor is private, delete using DeleteThis
public:

	CSkinInstance (const string &strFileName, CCharacterModel* pBody, uint32 IsSkinAtt, CAttachment* pMasterAttachment );

	void AddRef() {	++m_nRefCounter; }
	void Release();
	virtual void DeleteThis();
	
	virtual void Init();
	virtual void ShutDown();

	//////////////////////////////////////////////////////////////////////////
	// ISkinnable implementation
	//////////////////////////////////////////////////////////////////////////
	// Compute all effectors and remove those whose life time has ended, then apply effectors to the bones
	uint32 GetSkeletonPose( int nLod, int nList, const Matrix34& RenderMat34, QuatTS*& pBoneQuatsL, QuatTS*& pBoneQuatsS, QuatTS*& pMBBoneQuatsL, QuatTS*& pMBBoneQuatsS, Vec4 shapeDeformationData[], uint32 &DoWeNeedMorphtargets, uint8*& pRemapTable  );
	//////////////////////////////////////////////////////////////////////////

  virtual void ReplaceModel(ICharacterModel* pNewModel);
	uint32 IsSkinAttachment() { return m_IsSkinAtt==0xDeadBeef; }
	// Returns the interface for animations applicable to this model
	virtual IAttachmentManager* GetIAttachmentManager()	{	return &m_AttachmentManager;	}
	virtual ISkeletonAnim* GetISkeletonAnim() 
	{	
		assert(!"Invalid access of ISkeletonAnim (not implemented)!");
		return 0;	
	}
	virtual ISkeletonPose* GetISkeletonPose() 
	{	
//		assert(!"Invalid access of ISkeletonPose (not implemented)!");
		return 0; 
	}
	virtual IMorphing* GetIMorphing() {	return &m_Morphing; }
	virtual IAnimationSet* GetIAnimationSet() {	return m_pModel->GetAnimationSet();	}	
	virtual ICharacterModel* GetICharacterModel();
	virtual void Serialize(TSerialize ser) { assert(!"not supported for skin-instances"); };


	//-------------------------------------------------------------------------------
	//-----   decals  ---------------------------------------------------------------
	//-------------------------------------------------------------------------------
	uint32 m_useDecals : 1;
	CAnimDecalManager m_DecalManager;
	void CreateDecal(CryEngineDecalInfo& Decal) { assert(!"not supported for skin-instances"); };
	void EnableDecals(uint32 d) { m_useDecals = d>0; };
	void AddDecalsToRenderer( CCharInstance* pMaster, CAttachment* pAttachment, const Matrix34& RotTransMatrix34 );
	void FillUpDecalBuffer( CCharInstance* pMaster, QuatTS* parrNewSkinQuat, SVF_P3F_C4B_T2F* pVertices, SPipTangents* pTangents, uint16* pIndices, size_t decalIdx );
	void DeleteDecals();


	const char* GetModelAnimEventDatabase() { return m_pModel->GetModelAnimEventDatabaseCStr(); }

	//this is either the model-path or the CDF-path
	const char* GetFilePath() { return m_strFilePath.c_str(); }



	//-----------------------------------------------------------------------------------------------
	//---    bounding box calculation      ----------------------------------------------------------
	//-----------------------------------------------------------------------------------------------
	AABB GetAABB(); 
	virtual float ComputeExtent(GeomQuery& geo, EGeomForm eForm) { assert(!"not supported for skin-instances"); return 0; };
	virtual void GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm)  { assert(!"not supported for skin-instances"); };

	// calculates the mem usage
	virtual void GetMemoryUsage(ICrySizer* pSizer) const;
	virtual void GetMemoryUsage2(ICrySizer* pSizer) const;
	virtual void SetAnimationSpeed(f32 speed) { assert(!"not supported for skin-instances"); }
	virtual f32 GetAnimationSpeed() {  assert(!"not supported for skin-instances"); return 1.0f; }


	////////////////////////////////////////////////////////////////////////
	// update functions																		
	////////////////////////////////////////////////////////////////////////


	void UpdateAttachedObjects(const QuatT& rPhysEntityLocation, IAttachmentObject::EType *pOnlyThisType, float fZoomAdjustedDistanceFromCamera, uint32 OnRender );
	void UpdateAttachedObjectsFast(const QuatT& rPhysEntityLocation, float fZoomAdjustedDistanceFromCamera, uint32 OnRender );
	f32 GetAverageFrameTime() { return g_AverageFrameTime; };



	//--------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------
	void DrawWireframeStatic( const Matrix34& m34, int nLOD, uint32 color)
	{
		CModelMesh* pModelMesh = m_pModel->GetModelMesh(nLOD);
		if (pModelMesh)
			pModelMesh->DrawWireframeStatic(&m_Morphing,m34,nLOD,color);
	}


	//-----------------------------------------------------------------------------------


	// Returns true if this character was created from the file the path refers to.
	// If this is true, then there's no need to reload the character if you need to change its model to this one.
	bool IsModelFileEqual (const char* szFileName);
	void SetFlags(int nFlags) { assert(!"not supported for skin-instances"); }
	int  GetFlags() {  assert(!"not supported for skin-instances"); return 0; }


	//-----------------------------------------------------------------------------------------------
	//------render and shader stuff  ----------------------------------------------------------------
	//-----------------------------------------------------------------------------------------------

	//! Render object ( register render elements into renderer )
	void Render(const struct SRendParams &rParams, const QuatTS& Offset, Matrix34 *pFinalPhysLocation=0, bool* updated=0 );
	void ProcessSkinAttachment(const QuatT &rPhysLocationNext,const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender );

	//! Sets color parameter
	void SetColor(f32 fR, f32 fG, f32 fB, f32 fA) { assert(!"not supported for skin-instances"); };
	void SetMaterial( IMaterial* pMaterial );
	IMaterial* GetMaterial();
	IMaterial* GetMaterialOverride();

	void PreloadResources ( f32 fDistance, f32 fTime, int nFlags );
	// notifies the renderer that the character will soon be rendered
	void PreloadResources2 ( f32 fDistance, f32 fTime, int nFlags, IMaterial*pMat);
	// this is the array that's returned from the RenderMesh
	const PodArray<struct CRenderChunk>* getRenderMeshMaterials();

	IRenderMesh* GetRenderMesh ();
	// copies the given RenderMeshs to this instance RenderMeshs
	void CopyRenderMeshs (IRenderMesh** pRenderMeshs);
	void AddCurrentRenderData(CRenderObject *pObj, const SRendParams &pParams);  

	virtual int GetOjectType() { return m_pModel->m_ObjectType; }

	virtual bool IsAnimationGraphValid()
	{
		assert(!"not supported for skin-instances"); return 0;
	}

	//////////////////////////////////////////////////////////////////////////

	virtual IFacialInstance* GetFacialInstance();
	virtual void EnableProceduralFacialAnimation( bool bEnable );
	virtual void EnableFacialAnimation( bool bEnable );
	virtual void LipSyncWithSound( uint32 nSoundId, bool bStop=false );
	virtual void ReleaseTemporaryResources();

	virtual int	FindAffectedVertices(const Vec3 &vPos,float fRadius,std::vector<int> &lstIDs);

	//////////////////////////////////////////////////////////////////////////

  virtual void SetUniformScale(const float& fUniformScale) { m_fUniformScale = fUniformScale; }
  virtual float GetUniformScale() const { return m_fUniformScale; }

  virtual bool GetIgnoreScaleForJoint(uint32 jointIndex) const
  {
    return false;
  }
  virtual bool GetIsCustomScaleForJoint(uint32 jointIndex) const
  {
    return false;
  };
  virtual float GetCustomScaleForJoint(uint32 jointIndex) const { return 1.0f; };
  virtual void SetCustomScaleForJoint(uint32 jointIndex,bool bValue,float fValue){};

public:
	virtual size_t SizeOfCharInstance(ICrySizer * pSizer);

	virtual uint32 GetResetMode() { assert(!"not supported for skin-instances"); return 0; };
	virtual void SetResetMode(uint32 rm) { assert(!"not supported for skin-instances"); };

	uint32 GetWasVisible() { return m_bWasVisible; };

	virtual void EnableStartAnimation (bool bEnable) {	assert(!"not supported for skin-instances");	}
	virtual void SkeletonPreProcess(const QuatT &rPhysLocationCurr, const QuatTS &rAnimLocationCurr, const CCamera& rCamera, uint32 OnRender ) { assert(!"not supported for skin-instances"); };
	virtual void SkeletonPostProcess( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender ) { assert(!"not supported for skin-instances"); } 	
	virtual void SetPostProcessParameter( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment*	pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 nOnRender ) { assert(!"not supported for skin-instances"); }
	virtual void FinishAnimationComputations() { assert(!"not supported for skin-instances"); } ;


	virtual void OnDetach() {	m_pSkinAttachment=0; };
	virtual uint32 IsCharacterVisible(){ assert(!"not supported for skin-instances"); return 0; };


	virtual void HideMaster(uint32 h) { assert(!"not supported for skin-instances"); };
	void SetFPWeapon(f32 fp) { assert(!"not supported for skin-instances"); }
	void DrawDecalsBBoxes( CCharInstance* pMaster, CAttachment* pAttachment, const Matrix34& rRenderMat34);
	f32* GetShapeDeformArray() { assert(!"not supported for skin-instances"); return 0; }
	void AddMorphTargetsToVertexBuffer( int nLOD, uint32 nCloseEnough, uint32 &HWSkinningFlags );

	void SpawnSkeletonEffect(int animID, const char* animName, const char* effectName, const char* boneName, const Vec3& offset, const Vec3& dir, const Matrix34& entityTM)
	{
		 assert(!"not supported for skin-instances");
	}

	void KillAllSkeletonEffects()
	{
		 assert(!"not supported for skin-instances");
	}

	bool IsFastUpdateType(IAttachmentObject::EType eAttachmentType) const
	{
		if(eAttachmentType == IAttachmentObject::eAttachment_Entity)
			return true;
		if(eAttachmentType == IAttachmentObject::eAttachment_Effect)
			return true;
		return false;
	}

	bool FacialAnimationEnabled() const
	{
		return m_bFacialAnimationEnabled;
	}

	void SetFilePath(const string &filePath)
	{
		m_strFilePath = filePath;
	}

	bool WasVisible() const
	{
		return m_bWasVisible;
	}

	void SetWasVisible(bool wasVisible)
	{
		m_bWasVisible = wasVisible;
	}

	void SetHaveEntityAttachments(bool haveEntityAttachments)
	{
		m_bHaveEntityAttachments = haveEntityAttachments;
	}

	uint32 GetAnimationLOD()
	{
		return m_nAnimationLOD;
	}

	void SetForceLOD(const bool bSet) {m_bForceLOD = bSet;}
	void SetForceLODNum(const int32 level) {m_ForcedLODNum = level;}

	CMorphing m_Morphing;
	CAttachmentManager m_AttachmentManager;

	CAttachment* m_pSkinAttachment;

	CCharacterModel_AutoPtr m_pModel;

	CFacialInstance* m_pFacialInstance;

protected:
	int m_nRefCounter;	// Reference count.

	_smart_ptr<IRenderMesh> m_pRenderMeshs[g_nMaxGeomLodLevels];
	_smart_ptr<IMaterial> m_pInstanceMaterial;

	string m_strFilePath;
	uint32 m_uInstanceFlags;
	uint32 m_nInstanceUpdateCounter;
	uint32 m_nLastRenderedFrameID;
	uint32 m_LastRenderedFrameID;     // Can be accessed from main thread only!
	uint32 m_RenderPass;              // Can be accessed from render thread only!

	int32 m_nAnimationLOD;
	int32 m_nRenderLOD;
	short int m_nStillNeedsMorph; // Some counter so that last morph will continue for some time.
	short int m_nMorphTargetLod; // Which LOD should we use for morph targets? Necessary since shadow rendering sometimes uses different lod.
															// See AddMorphTargetsToVertexBuffer() for more information.

	bool m_bFacialAnimationEnabled : 1;
	uint32 m_bHaveEntityAttachments : 1;
	uint32 m_bWasVisible : 1;
	uint32 m_bPlayedPhysAnim : 1;
	uint32 m_UpdateAttachmentMesh : 1;
	uint32 m_UpdatedMorphTargetVBThisFrame : 1;
  //
  float m_fUniformScale;
  uint32 m_IsSkinAtt;

	static bool m_bForceLOD;
	static int32 m_ForcedLODNum;

protected:
  CSkinInstance ()
  {   
    m_fUniformScale = 1.0f;
		m_bForceLOD = false;
		m_ForcedLODNum = 0;
  }

  void Init(const string& strFileName, CCharacterModel* pModel, CCharInstance* pIMasterInstance);
};


#endif
