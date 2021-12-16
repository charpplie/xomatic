//////////////////////////////////////////////////////////////////////
//
// CryEngine Source code
//
// File:Skeleton.cpp
// Implementation of Skeleton class
//
// History:
// January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _SKELETON_POSE_H
#define _SKELETON_POSE_H

#include "Skeleton.h"

#include "ModelAnimationSet.h"

class CCharacterModel;
class CSkinInstance;
class CCharInstance;
class CSkeletonAnim;
class CSkeletonPose;
class CAttachment;

struct SAimDirection;
struct Locator;
 
#define RIGHT_ARM_AIM (10) //the amount of bones we need for the right arm



//#define MAX_MOTIONBLUR_FRAMES (2)
#define CA_FADEOUT (0x40000000) 





#define LHEEL (0x01)
#define RHEEL (0x02)
#define LTOE0 (0x04)
#define RTOE0 (0x08)
#define LNUB0 (0x10)
#define RNUB0 (0x20)



struct CPhysicsJoint
{
	CPhysicsJoint() : m_DefaultRelativeQuat(IDENTITY), m_qRelFallPlay(IDENTITY)
	{
		m_qRelPhysParent[0].SetIdentity();
		m_qRelPhysParent[1].SetIdentity();
	}
	void GetMemoryUsage(ICrySizer *pSizer ) const{}

	QuatT m_DefaultRelativeQuat;		//default relative joint (can be different for every instance)
	Quat m_qRelPhysParent[2];				// default orientation relative to the physicalized parent
	Quat m_qRelFallPlay;
};

struct CCGAJoint
{ 
	CCGAJoint() : m_CGAObjectInstance(0), m_pRNTmpData(0),m_qqqhasPhysics(~0),m_pMaterial(0) {}
	~CCGAJoint() 
	{
		if(m_pRNTmpData)
			GetISystem()->GetI3DEngine()->FreeRNTmpData(&m_pRNTmpData);
		assert(!m_pRNTmpData);
	};
	void GetMemoryUsage(ICrySizer *pSizer ) const{}

	_smart_ptr<IStatObj> m_CGAObjectInstance;	//Static object controlled by this joint (this can be different for every instance).
	struct CRNTmpData * m_pRNTmpData; // pointer to LOD transition state (allocated in 3dngine for visible objects)
	int m_qqqhasPhysics;						//>=0 if have physics (don't make it int16!!)
	_smart_ptr<IMaterial> m_pMaterial; // custom material override
};


struct ColorRGB{ uint8 r,g,b; };


struct SPostProcess
{
	QuatT m_RelativePose;
	uint32 m_JointIdx;

	SPostProcess(){};

	SPostProcess(int idx,const QuatT& qp)
	{
		m_JointIdx=idx;
		m_RelativePose=qp;
	}
	void GetMemoryUsage(ICrySizer *pSizer ) const{}
};


struct aux_bone_info {
	quaternionf quat0;
	Vec3 dir0;
	int iBone;
	f32 rlen0;
};

struct aux_phys_data {
	IPhysicalEntity *pPhysEnt;
	Vec3 *pVtx;
	Vec3 *pSubVtx;
	const char *strName;
	aux_bone_info *pauxBoneInfo;
	int nChars;
	int nBones;
	int iBoneTiedTo[2];
	int nSubVtxAlloc;
	bool bPhysical;
	bool bTied0,bTied1;

	void GetMemoryUsage(ICrySizer * pSizer) const
	{
//		pSizer->AddObject(pPhysEnt);
	}
};

struct SRecoil 
{
	f32 m_fAnimTime;
	f32 m_fDuration;
	f32 m_fStrengh;
	uint32 m_nArms;
	SRecoil()	{	void Init(); }

	void Init()
	{
		m_fAnimTime	=100.0f;
		m_fDuration	=0.0f;
		m_fStrengh	=0.0f;
		m_nArms			=3;
	};
};

struct SAimInfo 
{
#if defined(__CRYCG__)	
	SAimInfo(){ /*dummy for crycg*/}
#endif
	
	int16	m_numAimPoses;
	int16	m_pad;
	int16	m_nGlobalAimID0;
	int16	m_nGlobalAimID1;
	f32		m_fWeight;
	f32		m_fAnimTime;
};


struct IPol
{
#if defined(__CRYCG__)		
	IPol(){ /*dummy for crycg*/}
#endif
	
	Vec3 iepv;
	Vec3 ieoutput;
	Vec3 ipv;
	Vec3 output;
	uint32 k0,k1,k2;
	f32 distance;
	uint32 inside;
};


#include "PoseModifier/PoseBlenderAim.h"
#include "PoseModifier/LookAt.h"
#include "PoseModifier/FeetLock.h"

struct SFootAnchor 
{
	uint32 m_LastLegUpdate;
	uint32 m_UseFootAnchoring;
	Vec3	m_absSkeletonInWorld;

	QuatT m_AbsoluteRoot;
	QuatT m_AbsolutePelvis;

	QuatT m_AbsoluteLThigh;
	QuatT m_AbsoluteLCalf;
	QuatT m_AbsoluteLFoot;
	QuatT m_AbsoluteLHeel;
	QuatT m_AbsoluteLToe0;
	Vec3 m_LLastHeel;
	Vec3 m_LLastToe;
	Vec3 m_LHeelSlide;

	QuatT m_AbsoluteRThigh;
	QuatT m_AbsoluteRCalf;
	QuatT m_AbsoluteRFoot;
	QuatT m_AbsoluteRHeel;
	QuatT m_AbsoluteRToe0;
	Vec3 m_RLastHeel;
	Vec3 m_RLastToe;
	Vec3 m_RHeelSlide;
	
	f32 m_LAnkleIntensity;
	f32 m_fLAnkle;

	f32 m_RAnkleIntensity;
	f32 m_fRAnkle;

	SFootAnchor() {	Init();	};
	void Init() 
	{
		m_UseFootAnchoring=0;

		m_LastLegUpdate =(uint32)-1;
		m_absSkeletonInWorld=Vec3(ZERO);

		m_AbsoluteRoot.SetIdentity();
		m_AbsolutePelvis.SetIdentity();
		m_AbsoluteLThigh.SetIdentity();
		m_AbsoluteLCalf.SetIdentity();
		m_AbsoluteLFoot.SetIdentity();
		m_AbsoluteLHeel.SetIdentity();
		m_AbsoluteLToe0.SetIdentity();
		m_AbsoluteRThigh.SetIdentity();
		m_AbsoluteRCalf.SetIdentity();
		m_AbsoluteRFoot.SetIdentity();
		m_AbsoluteRHeel.SetIdentity();
		m_AbsoluteRToe0.SetIdentity();

		m_LAnkleIntensity = 0;
		m_fLAnkle=0;
		m_LLastHeel= Vec3(ZERO);
		m_LLastToe = Vec3(ZERO);
		m_LHeelSlide=Vec3(ZERO);

		m_RAnkleIntensity = 0;
		m_fLAnkle=0;
		m_RLastHeel= Vec3(ZERO);
		m_RLastToe = Vec3(ZERO);
		m_RHeelSlide=Vec3(ZERO);
	};
	
} _ALIGN(32);


//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
class CSkeletonPose: public ISkeletonPose
{
	friend class CSkinInstance;

public:
	CSkeletonPose();

	//-----------------------------------------------------------------------------------
	//---- interface functions to access bones ----
	//-----------------------------------------------------------------------------------
	size_t SizeOfThis (ICrySizer * pSizer);
	void GetMemoryUsage(ICrySizer * pSizer) const;

	//! Returns the number of bones; all bone ids are in the range from 0 to this number exclusive; 0th bone is the root
	uint32 GetJointCount() const 
	{ 
#if defined(__SPU__)
		// this expects that g_jointCount is set at the beginning of the job		
		return g_jointCount;	
#else
		return m_pModelSkeleton->m_poseData.GetJointCount();
#endif
	}

	// finds the index by its name; returns the index of the bone
	int16 GetJointIDByName (const char* szJointName) const { return m_pModelSkeleton->GetJointIDByName(szJointName); };
	// finds the parent-index by the child-index and returns the parent-index
	int16 GetParentIDByID (int32 ChildID) const { return m_pModelSkeleton->GetJointParentIDByID(ChildID); };
	// returns the name of bone from bone table, return zero id nId is out of range
	const char* GetJointNameByID(int32 nId) const { return m_pModelSkeleton->GetJointNameByID(nId); };

	VIRTUAL uint32 GetJointCRC32 (int32 nJointID) const;
	uint32 GetChildrenCount(int32 parentID) const;
	int16 GetChildID(int32 parentID, int32 childIndex) const;

	const QuatT& GetAbsJointByID(int32 id) 
	{ 
		int32 numJoints=(int)CSkeletonPose::GetJointCount();
		if(id>=0 && id<numJoints)
			return GetPoseData().m_jointsAbsolute[id];
		assert(0);
		return g_IdentityQuatT; 
	}

	void SetAbsJointByID(int32 id, const QuatT& pose)
	{
		int32 numJoints=(int)CSkeletonPose::GetJointCount();
		if(id>=0 && id<numJoints)
		{
			GetPoseDataWriteable()->m_jointsAbsolute[id] = pose;
			if (m_parrModelJoints[id].m_idxParent>=0 && m_parrModelJoints[id].m_idxParent<numJoints)
			{
				GetPoseDataWriteable()->m_jointsRelative[id].t = (pose.t - GetPoseDataWriteable()->m_jointsAbsolute[m_parrModelJoints[id].m_idxParent].t) * GetPoseDataWriteable()->m_jointsAbsolute[m_parrModelJoints[id].m_idxParent].q;
				GetPoseDataWriteable()->m_jointsRelative[id].q = GetPoseDataWriteable()->m_jointsAbsolute[m_parrModelJoints[id].m_idxParent].q.GetInverted() * pose.q;
			}

			for (int32 i=1; i<numJoints; i++)
			{
				int32 p=m_parrModelJoints[i].m_idxParent;
				GetPoseDataWriteable()->m_jointsAbsolute[i]	= GetPoseDataWriteable()->m_jointsAbsolute[p] * GetPoseDataWriteable()->m_jointsRelative[i];
			}
		} 
	}
  void SetDefaultAbsInvJointByID(int32 nJointID, const QuatT& pose)
  {
		//sorry, not enough mememory on consoles. Need to do it at runtime
  //  m_parrModelJoints[nJointID].m_DefaultAbsPoseInverse = pose;
  }
	const QuatT& GetRelJointByID(int32 id) 
	{ 
		int32 numJoints=(int)CSkeletonPose::GetJointCount();
		if(id>=0 && id<numJoints)
			return GetPoseData().m_jointsRelative[id];
		assert(!"GetRelJointByID() - Index out of range!");
		return g_IdentityQuatT;
	}

	const QuatT& GetDefaultAbsJointByID(int32 id) 
	{ 
		const QuatT *ret = &g_IdentityQuatT;
		int32 numJoints=(int)CSkeletonPose::GetJointCount();
		if(id>=0 && id<numJoints)
			ret = &GetPoseDataDefault().m_jointsAbsolute[id]; 
		else
		{
			assert(!"GetDefaultAbsJointByID() - Index out of range!");
		}

		return *ret;
	}
	const QuatT& GetDefaultRelJointByID(int32 id) 
	{ 
		int32 numJoints=(int)GetPoseDataDefault().GetJointCount();
		if(id>=0 && id<numJoints)
			return m_pModelSkeleton->m_poseData.m_jointsRelative[id];
		assert(!"GetDefaultRelJointByID() - Index out of range!");
		return g_IdentityQuatT; 
	}

	void InitSkeletonPose( CCharInstance* pInstance, CSkeletonAnim* pSkeletonAnim);
	void InitPhysicsSkeleton();
	void InitCGASkeleton();

	void SetDefaultPose();
	void SetDefaultPosePerInstance();
	
	//------------------------------------------------------------------------------------	

	void SkeletonPostProcess( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender=0 );
	void SkeletonPostProcess2( const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, uint32 OnRender=0  );
	void KinematicPostProcess(const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext);

	SFootPlant BlendFootPlants(const SFootPlant& fp0, const SFootPlant& fp1, f32 t);
	void AdjustFeetHelper( QuatT &rAbsoluteCalf, QuatT &rRelativeCalf, QuatT &rAbsoluteTight, QuatT &rAbsoluteFoot,QuatT &rRelativeFoot, const Vec3 &rCross, const Vec3 &rKneeDir, uint32 iteration, float knee_factor );



	int32 m_nForceSkeletonUpdate;
	void SetForceSkeletonUpdate(int32 i) { m_nForceSkeletonUpdate=i; };

	void UpdateBBox(uint32 update=0);
	void SetSkinningSkeletonMaster(f32 fScaling, int nList);

	//-----------------------------------------------------------------------------------


	//------------------------------------------------------------------------
	//----> LookIK
	//------------------------------------------------------------------------
	void SetLookIK(uint32 ik, f32 FOR, const Vec3& LookAtTarget,const f32 *customBlends, bool allowAdditionalTransforms, bool ignoreAimBlend);
	void LookIK( const QuatT& rPhysLocationNext, uint32 LIKFlag );
	void SetLookIKHunter(uint32 ik, f32 FOR, const Vec3& LookAtTarget);
	void LookIKHunter( uint32 LIKFlag, const QuatT& rPhysLocationNew, const QuatTS& rAnimLocationNew  );
	uint8 IsLookIKEnabled() { return m_LookIK().m_UseLookIK; }
	uint8 GetLookIKInterpolatedTarget( Vec3& vTarget ); // Returns false if look IK is disabled

	IAnimationPoseModifierPtr m_LookAt;
	CLookAt& m_LookIK() { return *static_cast<CLookAt*>(m_LookAt.get()); }

	//------------------------------------------------------------------------
	//----> AimIK
	//------------------------------------------------------------------------
	void SetAimIK(bool bUseAimIK, const Vec3& AimAtTarget, uint32 arms=3, float blendTime = -1.0f ); 
	void SetAimIKLayer(uint32 nLayer);
	void SetAimIKFadeOut(uint32 a) { m_AimIK().SetFadeOut(a); }
	void SetAimIKFadeOutSpeed(f32 time) { m_AimIK().SetFadeOutSpeed(time); }
	void SetAimIKFadeInSpeed(f32 time) { m_AimIK().SetFadeInSpeed(time); }
	void SetAimIKTargetSmoothTime(f32 fSmoothTime) { m_AimIK().SetTargetSmoothTime(fSmoothTime); }
	void SetAimIKPolarCoordinates(const Vec2& pc) 
	{
		m_AimIK().SetPolarCoordinates(pc);	
	};
	uint32 GetAimIKStatus() { return m_AimIK().GetStatus(); }
	f32 GetAimIKBlend() { return m_AimIK().GetBlend(); }

	IAnimationPoseBlenderAimPtr m_PoseBlenderAim;
	CPoseBlenderAim& m_AimIK() { return *static_cast<CPoseBlenderAim*>(m_PoseBlenderAim.get()); }

	void SetPoseBlenderAimState(bool bEnable);
	void SetPoseBlenderAimLayer(uint32 layer);
	IAnimationPoseBlenderAim* GetPoseBlenderAim() { return m_PoseBlenderAim.get(); }

	CFeetLock m_feetLock;

public:

	//------------------------------------------------------------------------
	//----> recoil implementation
	//------------------------------------------------------------------------
	void ApplyRecoilAnimation(f32 fDuration, f32 fImpact, uint32 arms=3 );
	void PlayRecoilAnimation( const QuatT& rPhysEntityLocation );
	SRecoil m_Recoil;

	//------------------------------------------------------------------------
	//----> weapon raising
	//------------------------------------------------------------------------
	void SetWeaponRaisedPose(EWeaponRaisedPose pose) {}

	//------------------------------------------------------------------------
	//----> Foot Anchoring
	//------------------------------------------------------------------------
	void SetFootAnchoring(uint32 at){ m_FootAnchor.m_UseFootAnchoring=at > 0; };
	void FootAnchoring( const SFootPlant& BFPlant, uint8 Foot, const QuatTS& rAnimLocationCurr );
	void AbsoluteLegPosition(const QuatTS& rAnimLocationCurr);
	void EaseOut_LAnkle();
	void EaseOut_RAnkle();
	SFootAnchor m_FootAnchor;



	//------------------------------------------------------------------------
	//----> human limb-IK
	//------------------------------------------------------------------------
	uint32 SetCustomArmIK(const Vec3& goal,int32 idx0,int32 idx1,int32 idx2);
	uint32 SetHumanLimbIK(const Vec3& vWorldPos, const char* strLimb);
	uint32 SetHumanLimbIK_Local(const Vec3& vLocalPos, const char* strLimb);

	void MoveSkeletonVertical( f32 vertical );
	uint32 SetFootGroundAlignmentCCD( const char* strLEG, const Plane& GroundPlane);
	uint32 SetFootGroundAlignment(const Vec3& normal2, int32 CalfIdx,int32 FootIdx,int32 HeelIdx,int32 Toe0Idx  );
	void EnableFootGroundAlignment(bool enable);


	//------------------------------------------------------------------------
	//----> CCD-IK 
	//------------------------------------------------------------------------
	DynArray<int32> m_arrCCDChain;
	void CCDInitIKBuffer(QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK);
	int32* CCDInitIKChain(int32 sCCDJoint,int32 eCCDJoint);
	void CCDRotationSolver(const Vec3& PositionCCD,f32 fThreshold,f32 StepSize,uint32 iTry,const Vec3& normal,QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK,Quat* pQuat=0);
	void CCDTranslationSolver(const Vec3& PositionCCD,QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK);
	void CCDDirConstraint(const Vec3& dir,QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK);
	void CCDQuatConstraint(const Quat* pQuats,QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK);
	void CCDUpdateSkeleton(QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK);
	void CCDLockHunterFoot(int32 idx,QuatT* pRelativeQuatIK,QuatT* pAbsoluteQuatIK);





	void SetPostProcessQuat(int32 idx, const QuatT& qt )
	{
		m_arrPostProcess.push_back( SPostProcess(idx,qt) );
	}
	DynArray<SPostProcess> m_arrPostProcess;


	int (*m_pPostProcessCallback)(ICharacterInstance*,void*);
	void* m_pPostProcessCallbackData;
	void SetPostProcessCallback(CallBackFuncType func, void *pdata)
	{
		m_pPostProcessCallback = func;
		m_pPostProcessCallbackData = pdata;
	}

	int (*m_pPostPhysicsCallback)(ICharacterInstance*,void*);
	void* m_pPostPhysicsCallbackData;
	void SetPostPhysicsCallback(CallBackFuncType func, void *pdata)
	{
		m_pPostPhysicsCallback = func;
		m_pPostPhysicsCallbackData = pdata;
	}



	//-------------------------------------------------------------------------------
	//---         Physics from CSkinInstance                                  ---
	//-------------------------------------------------------------------------------
	IStatObj* GetStatObjOnJoint(int32 nId);
	void SetStatObjOnJoint(int32 nId, IStatObj* pStatObj);
	IPhysicalEntity *GetPhysEntOnJoint(int32 nId) { return m_ppBonePhysics ? m_ppBonePhysics[nId]:0; }
	void SetPhysEntOnJoint(int32 nId, IPhysicalEntity *pPhysEnt);
	int GetPhysIdOnJoint(int32 nId);
	void SetMaterialOnJoint(int32 nId, IMaterial* pMaterial) { assert(nId>=0 && nId<(int32)m_arrCGAJoints.size()); m_arrCGAJoints[nId].m_pMaterial = pMaterial; }
	IMaterial* GetMaterialOnJoint(int32 nId) { assert(nId>=0 && nId<(int32)m_arrCGAJoints.size()); return m_arrCGAJoints[nId].m_pMaterial; }

	void SetRagdollDefaultPose();
	int getBonePhysChildIndex (int nBoneIndex, int nLod=0);
	int getBonePhysParentIndex (int nBoneIndex, int nLod=0); // same, but finds the first ancestor that has physical geometry
	int getBonePhysParentOrSelfIndex (int nBoneIndex, int nLod=0);
	int GetBoneSurfaceTypeId(int nBoneIndex, int nLod=0);

	int TranslatePartIdToDeadBody(int partid);
	float AssessPoseSimilarity(const CSkeletonPose &skelAnim, int iLod);
	void ProcessPhysics(const QuatT& rPhysEntityLocation, f32 fDeltaTimePhys, int nNeff, SCharUpdateFeedback *pCharFeedback=0);

	void Fall();
	void GoLimp();
	void StandUp(const Matrix34 &mtx, bool b3DOF, IPhysicalEntity *&pNewPhysicalEntity, Matrix34 &mtxDelta);

	float Falling() const;
	float Lying() const;
	float StandingUp() const
	{
		return m_timeStandingUp;
	}

	int GetFallingDir();

	bool SetFnPAnimGroup(const char *name);
	bool SetFnPAnimGroup(int idx);

	void FindSpineBones();

	void BuildPhysicalEntity(IPhysicalEntity *pent,f32 mass,int surface_idx,f32 stiffness_scale, int nLod=0,int partid0=0, 
		const Matrix34 &mtxloc=Matrix34(QuatT(IDENTITY)));
	IPhysicalEntity *GetCharacterPhysics(const char *pRootBoneName);
	IPhysicalEntity *GetCharacterPhysics(int iAuxPhys);
	int GetAuxPhysicsBoneId(int iAuxPhys, int iBone=0) { 
		PREFAST_SUPPRESS_WARNING(6385) 
		return (iAuxPhys<m_nAuxPhys && iBone<m_auxPhys[iAuxPhys].nBones) ? m_auxPhys[iAuxPhys].pauxBoneInfo[iBone].iBone : -1;
	}
	void DestroyCharacterPhysics(int iMode=0);
	IPhysicalEntity* CreateCharacterPhysics(IPhysicalEntity *pHost, f32 mass,int surface_idx,f32 stiffness_scale, int nLod=0, 
		const Matrix34 &mtxloc=Matrix34(QuatT(IDENTITY)));
	int CreateAuxilaryPhysics(IPhysicalEntity *pHost, const Matrix34 &mtx, int nLod=0);
	int CreateAuxilaryPhysics(IPhysicalEntity *pHost, const Matrix34 &mtx, f32 scale,Vec3 offset, int nLod);
	int FillRopeLenArray(float *arr,int i0,int sz);
	void SynchronizeWithPhysicalEntity(IPhysicalEntity *pent,const Vec3& posMaster,const Quat& qMaster);
	void SynchronizeWithPhysicalEntity(IPhysicalEntity *pent, const Vec3& posMaster,const Quat& qMaster, QuatT offset, int iDir=-1 );
	IPhysicalEntity *RelinquishCharacterPhysics(const Matrix34 &mtx, float stiffness=0.0f, bool bCopyJointVelocities=false);
	bool AddImpact(int partid, Vec3 point,Vec3 impact);
	void ResetNonphysicalBoneRotations (int nLOD, f32 fBlend);
	void UnconvertBoneGlobalFromRelativeForm(bool bNonphysicalOnly, int nLod=0, bool bRopeTipsOnly=false);
	void ConvertBoneGlobalToRelativeMatrices();
	//! marks all LODs as needed to be reskinned
	void ForceReskin ();

	IPhysicalEntity* GetCharacterPhysics() const { return m_pCharPhysics; }
	void SetCharacterPhysics(IPhysicalEntity *pent) { 
		m_pCharPhysics=pent; m_bAliveRagdoll = 0;	m_timeStandingUp = -1;
	}

	void DestroyPhysics();
	void SetAuxParams(pe_params* pf);

	void SetOffset(Vec3 offset) {	m_vOffset = offset; }
	Vec3 GetOffset() { return m_vOffset; }

	CModelJoint* GetModelJointPointer(int nBone) { return &m_parrModelJoints[nBone]; }
	//returns the j-th child of i-th child of the given bone
	int GetModelJointChildIndex (int nBone, int i)
	{
		assert (i >= 0 && i<(int)GetModelJointPointer(nBone)->numChildren());
		return nBone + GetModelJointPointer(nBone)->getFirstChildIndexOffset() + i;
	}
	// given the bone index, (INDEX, NOT ID), returns this bone's parent index
	uint32 getBoneParentIndex (uint32 nBoneIndex);
		
	
	void SetFootPlants(int val) { m_FootPlants = (val > 0); };
	int GetFootPlants() const { return m_FootPlants; };

	int GetInstanceVisible() const { return m_bFullSkeletonUpdate; };


	//just for debugging
	void DrawBBox( const Matrix34& rRenderMat34 );
	void DrawSkeleton( const Matrix34& rRenderMat34, uint32 shift=0 );
	void DrawDefaultSkeleton( const Matrix34& rRenderMat34 );
	void DrawArrow( const QuatT& location, const Vec3& vTravelDir,f32 length, ColorB col );
	void DrawArrowArc( const QuatT& location, const f32 turnAngle, const f32 length, const f32 slope, const ColorB& col );
	f32 SecurityCheck();
	uint32 IsSkeletonValid();
	int32 m_Superimposed;
	void SetSuperimposed(uint32 i){ m_Superimposed=i; };
	void DrawThinSkeletons( const QuatT& m34, const std::vector< std::vector<DebugJoint> >& arrSkeletons, int32 nAnimID,int32 nGlobalAnimID );
	void DrawThinSkeleton( const QuatT& m34, const std::vector<DebugJoint>& arrSkeleton, uint8 FootPlant, ColorB col );
	AABB GetAABB(){ return m_AABB; }
	void ExportSkeleton();

	void SetGroundAlignmentData(const bool bAlignSkeletonVertical, const f32 rootHeight, const Plane& planeLeft, const Plane& planeRight) 
	{
		m_bGroundAlignDataReady = true; 
		m_alignSkeletonVertical = bAlignSkeletonVertical; 
		m_rootHeight = rootHeight; 
		m_planeLeft = planeLeft; 
		m_planeRight = planeRight;
	}

public:
	IPhysicalEntity *m_pCharPhysics;
	IPhysicalEntity *m_pPrevCharHost;
	IPhysicalEntity **m_ppBonePhysics;
	CCharInstance* m_pInstance;
	CModelSkeleton* m_pModelSkeleton;

	CModelJoint* m_parrModelJoints;

public:
	ILINE const Skeleton::CPoseData& GetPoseData() const { return m_pPoseData ? *m_pPoseData : *m_pPoseDataDefault; }
	ILINE Skeleton::CPoseData* GetPoseDataWriteable() { return m_pPoseData; }
	ILINE const Skeleton::CPoseData& GetPoseDataDefault() const { return *m_pPoseDataDefault; }

	ILINE const Skeleton::CLocator& GetLocator() const { return *m_pLocator; }
	ILINE Skeleton::CLocator* GetLocatorWriteable() { return m_pLocator; }

private:
	Skeleton::CPoseData m_poseData;
	Skeleton::CLocator m_locator;

	Skeleton::CPoseData* m_pPoseDataDefault;
	Skeleton::CPoseData* m_pPoseData;
	Skeleton::CLocator* m_pLocator;

public:
	DynArray<Vec3>		    m_FaceAnimPosSmooth; 
	DynArray<Vec3>		    m_FaceAnimPosSmoothRate; 
	DynArray<CCGAJoint>     m_arrCGAJoints;
	DynArray<CPhysicsJoint> m_arrPhysicsJoints;
  //Pavlo: this array is used only for character editor
	DynArray<IAnimationPoseModifier*> m_poseModifiers;

	Vec3 m_vRenderOffset;
	AABB m_AABB;

	Vec3 m_vOffset;
	int m_iSpineBone[3];
	mutable int m_nSpineBones;
	int m_nAuxPhys;
	int m_iSurfaceIdx;
	int m_iFlyDir;
	aux_phys_data m_auxPhys[12];
	int m_b3DOFStandup;
	f32 m_fPhysBlendTime,m_fPhysBlendMaxTime,m_frPhysBlendMaxTime;
	float m_stiffnessScale;
	float m_timeRagdolled,m_timeLying,m_timeNoColl,m_timeStandingUp;
	int m_iFnPSet;
	f32 m_fScale;
	f32 m_fMass;
	Vec3 m_prevPosPivot,m_velPivot;

	f32 m_LFootGroundAlign;
	f32 m_RFootGroundAlign;
	bool m_enableFootGroundAlignment;

	bool m_bInstanceVisible : 1;
	bool m_bFullSkeletonUpdate : 1;
	uint32 m_bAllNodesValid : 1; //True if this animation was already played once.
	bool m_bAllAbsoluteJointsValid : 1;
	bool m_bPhysicsRelinquished : 1;
	bool m_FootPlants : 1;
	bool m_bHasPhysics  : 1, m_bPhysicsAwake : 1, m_bPhysicsWasAwake : 1;
	bool m_bAliveRagdoll : 1;
	bool m_bLimpRagdoll : 1;

	CSkeletonAnim* m_pSkeletonAnim;


private:
	bool m_bGroundAlignDataReady;
	bool m_alignSkeletonVertical;
	f32 m_rootHeight;
	Plane m_planeLeft;
	Plane m_planeRight;

	void PoseModifiersExecute(const QuatT& locationNextPhysics, const QuatTS& locationNextAnimation);

}_ALIGN(128);

#endif // _SKELETON_POSE_H
