#include "stdafx.h"

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/Impl/ICryFactoryRegistryImpl.h>
#include <CryExtension/CryCreateClassInstance.h>

#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "PoseModifierHelper.h"
#include "FeetLock.h"

namespace Util {

SPU_NO_INLINE void AdjustFeetHelper(QuatT &rAbsoluteCalf, QuatT &rRelativeCalf, QuatT &rAbsoluteTight, QuatT &rAbsoluteFoot,QuatT &rRelativeFoot, const Vec3 &rCross, const Vec3 &rKneeDir, uint32 iteration, float knee_factor)
{
	for (uint32 i=0; i<iteration; i++)
	{
		Vec3 ulleg = rAbsoluteCalf.t - rAbsoluteTight.t;
		Vec3 llleg = rAbsoluteFoot.t - rAbsoluteCalf.t;

		Vec3 lcross = ulleg%llleg;

		if ( (lcross|lcross)>0.0001f && (rCross|lcross)>0)
			break;

		rAbsoluteCalf.t += rKneeDir*knee_factor;
	}

	rRelativeCalf = rAbsoluteTight.GetInverted() 	* rAbsoluteCalf;
	rRelativeFoot = rAbsoluteCalf.GetInverted() 	* rAbsoluteFoot;
}

}

/*

  CFeetPoseStore

*/

CRYREGISTER_CLASS(CFeetPoseStore)

//

CFeetPoseStore::CFeetPoseStore()
{
}

CFeetPoseStore::~CFeetPoseStore()
{
}

//

SPU_NO_INLINE void CFeetPoseStore::ComputeFootPositions(QuatT &rRelativeCalf, QuatT &rRelativeTight, QuatT &rFoot, QuatT &plevis, Vec3 &rCross, Vec3 &rKneeDir, QuatT &rAnkle)
{	
	QuatT rAbsoluteTight	= plevis* rRelativeTight;
	QuatT rAbsoluteCalf		= rAbsoluteTight * rRelativeCalf;

	rAnkle = rAbsoluteCalf	* rFoot;	

	Vec3 ulleg = rAbsoluteCalf.t - rAbsoluteTight.t;	
	Vec3 llleg = rAnkle.t - rAbsoluteCalf.t;

	llleg.Normalize();	
	ulleg.Normalize();

	rCross = ulleg%llleg;	
	f32 ldot=rCross|rCross;

	//	assert( ldot>0.0001f ); 

#if !defined(__SPU__)
	/*	if (ldot<0.0001f)
	{
	const char* pAnimName=0;
	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	const ModelAnimationHeader* anim;

	if (m_arrLayer_AFIFO[0][0].m_LMG.m_nLMGID>=0)
	anim	=	&pAnimationSet->GetModelAnimationHeaderRef(m_arrLayer_AFIFO[0][0].m_LMG.m_nLMGID);
	else
	anim	=	&pAnimationSet->GetModelAnimationHeaderRef(m_arrLayer_AFIFO[0][0].m_LMG.m_nAnimID[0]);
	pAnimName=anim->GetAnimName();
	const char* pModelFilepath = m_pInstance->m_pModel->GetFilePath();
	AnimFileWarning(0,"Warning: hyper extension on leg. IK will fail: %s %s", pModelFilepath,pAnimName );
	}*/
#endif // !__SPU__

	Vec3 lkneeaxis	= ulleg%llleg;	
	rKneeDir = (ulleg%lkneeaxis).GetNormalized();	
}

// IAnimationPoseModifier

SPU_INDIRECT(CommandBufferExecute(ML))
bool CFeetPoseStore::Execute(const SAnimationPoseModiferParams& params)
{
	ICharacterModelSkeleton* pCharModelSkeleton = SPU_MAIN_PTR( params.GetICharacterModelSkeleton() );
	QuatT *pRelativePose = SPU_LOCAL_PTR( params.pPoseRelative );

	uint32 lFootIdx = pCharModelSkeleton->GetJointIDByType(eIM_LFootIdx);
	uint32 rFootIdx = pCharModelSkeleton->GetJointIDByType(eIM_RFootIdx);
	uint32 mPelvisIdx = pCharModelSkeleton->GetJointIDByType(eIM_PelvisIdx);

	uint32 lThighIdx = pCharModelSkeleton->GetJointIDByType(eIM_LThighIdx);
	uint32 lCalfIdx = pCharModelSkeleton->GetJointIDByType(eIM_LCalfIdx);

	uint32 rThighIdx = pCharModelSkeleton->GetJointIDByType(eIM_RThighIdx);
	uint32 rCalfIdx = pCharModelSkeleton->GetJointIDByType(eIM_RCalfIdx);

	QuatT & RESTRICT_REFERENCE m_RelativePoseLeftTight = pRelativePose[lThighIdx];
	QuatT & RESTRICT_REFERENCE m_RelativePoseLeftCalf = pRelativePose[lCalfIdx];
	QuatT & RESTRICT_REFERENCE m_RelativePoseLeftFoot = pRelativePose[lFootIdx];

	QuatT & RESTRICT_REFERENCE m_RelativePoseRightTight = pRelativePose[rThighIdx];
	QuatT & RESTRICT_REFERENCE m_RelativePoseRightCalf = pRelativePose[rCalfIdx];
	QuatT & RESTRICT_REFERENCE m_RelativePoseRightFoot = pRelativePose[rFootIdx];

	QuatT m_AbsolutePosePelvis;

	m_AbsolutePosePelvis = pRelativePose[0] * pRelativePose[mPelvisIdx]; 

	ComputeFootPositions(
		m_RelativePoseLeftCalf, m_RelativePoseLeftTight,
		m_RelativePoseLeftFoot, m_AbsolutePosePelvis,
		m_pFeetData->m_lCross, m_pFeetData->m_lKneeDir, m_pFeetData->m_lAnkle);
	assert(m_pFeetData->m_lAnkle.IsValid());
	ComputeFootPositions(
		m_RelativePoseRightCalf, m_RelativePoseRightTight,
		m_RelativePoseRightFoot, m_AbsolutePosePelvis,
		m_pFeetData->m_rCross, m_pFeetData->m_rKneeDir, m_pFeetData->m_rAnkle);
	assert(m_pFeetData->m_rAnkle.IsValid());

	m_pFeetData->m_IsAnkleValid = 1;

	return false;
}

/*

  CFeetPoseRestore

*/

CRYREGISTER_CLASS(CFeetPoseRestore)

//

CFeetPoseRestore::CFeetPoseRestore()
{
}

CFeetPoseRestore::~CFeetPoseRestore()
{
}

// IAnimationPoseModifier

SPU_INDIRECT(CommandBufferExecute(ML))
bool CFeetPoseRestore::Execute(const SAnimationPoseModiferParams& params)
{
	if (!m_pFeetData->m_IsAnkleValid)
		return false;

	ICharacterModelSkeleton* pCharModelSkeleton = SPU_MAIN_PTR( params.GetICharacterModelSkeleton() );
	CSkeletonPose* pSkeletonPose = SPU_MAIN_PTR( PoseModifierHelper::GetSkeletonPose(params) );

//	assert(ac.m_TargetBuffer<=AC::TargetBuffer);
	QuatT*		parrRelPoseDst	= params.pPoseRelative;
//	Status4*	parrStatusDst		= SPU_LOCAL_PTR((Status4*)  CBTemp[ac.m_TargetBuffer+1]);
//	Locator*	pLocatorDst			= SPU_LOCAL_PTR((Locator*)CBTemp[ac.m_TargetBuffer+2]);

	//	float fColor[4] = {0,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.3f, fColor, false,"Anchoring");	
	//	g_YLine+=16.0f;

	QuatT*	const	__restrict	pSkeletonAbsolutPose	= SPU_LOCAL_PTR(params.pPoseAbsolute);
	QuatT*	const	__restrict	pSkeletonRelativePose	= SPU_LOCAL_PTR(params.pPoseRelative);

	// shortcuts to often used memory refences
	uint32 mPelvisIdx	=	pCharModelSkeleton->GetJointIDByType(eIM_PelvisIdx);

	uint32 lThighIdx	=	pCharModelSkeleton->GetJointIDByType(eIM_LThighIdx);
	uint32 lCalfIdx		=	pCharModelSkeleton->GetJointIDByType(eIM_LCalfIdx);
	uint32 lFootIdx		=	pCharModelSkeleton->GetJointIDByType(eIM_LFootIdx);

	uint32 rThighIdx	=	pCharModelSkeleton->GetJointIDByType(eIM_RThighIdx);
	uint32 rCalfIdx		=	pCharModelSkeleton->GetJointIDByType(eIM_RCalfIdx);
	uint32 rFootIdx		=	pCharModelSkeleton->GetJointIDByType(eIM_RFootIdx);

	QuatT & RESTRICT_REFERENCE m_AbsolutePosePelvis = pSkeletonAbsolutPose[mPelvisIdx];

	QuatT & RESTRICT_REFERENCE m_AbsolutePoseLeftTight = pSkeletonAbsolutPose[lThighIdx];		
	QuatT & RESTRICT_REFERENCE m_AbsolutePoseLeftCalf = pSkeletonAbsolutPose[lCalfIdx];	
	QuatT & RESTRICT_REFERENCE m_AbsolutePoseLeftFoot = pSkeletonAbsolutPose[lFootIdx];

	QuatT & RESTRICT_REFERENCE m_AbsolutePoseRightTight = pSkeletonAbsolutPose[rThighIdx];
	QuatT & RESTRICT_REFERENCE m_AbsolutePoseRightCalf = pSkeletonAbsolutPose[rCalfIdx];
	QuatT & RESTRICT_REFERENCE m_AbsolutePoseRightFoot = pSkeletonAbsolutPose[rFootIdx];


	QuatT & RESTRICT_REFERENCE m_RelativePoseLeftTight = pSkeletonRelativePose[lThighIdx];		
	QuatT & RESTRICT_REFERENCE m_RelativePoseLeftCalf = pSkeletonRelativePose[lCalfIdx];	
	QuatT & RESTRICT_REFERENCE m_RelativePoseLeftFoot = pSkeletonRelativePose[lFootIdx];

	QuatT & RESTRICT_REFERENCE m_RelativePoseRightTight = pSkeletonRelativePose[rThighIdx];
	QuatT & RESTRICT_REFERENCE m_RelativePoseRightCalf = pSkeletonRelativePose[rCalfIdx] ;
	QuatT & RESTRICT_REFERENCE m_RelativePoseRightFoot = pSkeletonRelativePose[rFootIdx];

	pSkeletonAbsolutPose[0] =	pSkeletonRelativePose[0];
	m_AbsolutePosePelvis = pSkeletonAbsolutPose[0] * pSkeletonRelativePose[mPelvisIdx]; 


	m_AbsolutePoseLeftTight = m_AbsolutePosePelvis * m_RelativePoseLeftTight;
	m_AbsolutePoseLeftCalf = m_AbsolutePoseLeftTight * m_RelativePoseLeftCalf;
	m_AbsolutePoseLeftFoot = m_AbsolutePoseLeftCalf * m_RelativePoseLeftFoot;
	Util::AdjustFeetHelper(
		m_AbsolutePoseLeftCalf,
		m_RelativePoseLeftCalf,
		m_AbsolutePoseLeftTight,
		m_AbsolutePoseLeftFoot,
		m_RelativePoseLeftFoot,
		m_pFeetData->m_lCross,
		m_pFeetData->m_lKneeDir, 10, 0.01f);

	m_AbsolutePoseRightTight = m_AbsolutePosePelvis * m_RelativePoseRightTight;
	m_AbsolutePoseRightCalf = m_AbsolutePoseRightTight * m_RelativePoseRightCalf;
	m_AbsolutePoseRightFoot = m_AbsolutePoseRightCalf * m_RelativePoseRightFoot;
	Util::AdjustFeetHelper(
		m_AbsolutePoseRightCalf,
		m_RelativePoseRightCalf,
		m_AbsolutePoseRightTight,
		m_AbsolutePoseRightFoot,
		m_RelativePoseRightFoot,
		m_pFeetData->m_rCross,
		m_pFeetData->m_rKneeDir, 10, 0.01f);

	//g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
	//static Ang3 angle(0,0,0); 
	//angle += Ang3(0.01f,0.02f,0.03f);
	//AABB aabb = AABB(Vec3(-0.03f,-0.03f,-0.03f),Vec3(+0.03f,+0.03f,+0.03f));
	//OBB obb=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle),aabb );
	//g_pAuxGeom->DrawOBB(obb,rAnimLocationCurr.q*lAnkle.t,0,RGBA8(0xff,0xff,0xff,0xff), eBBD_Extremes_Color_Encoded);
	//g_pAuxGeom->DrawOBB(obb,rAnimLocationCurr.q*rAnkle.t,0,RGBA8(0xff,0xff,0xff,0xff), eBBD_Extremes_Color_Encoded);
	//g_pAuxGeom->DrawLine(rAnimLocationCurr.q*gSkeletonAbsolutPose[lCalfIdx].t,RGBA8(0xff,0xff,0xff,0xff),rAnimLocationCurr.q*(gSkeletonAbsolutePose[lCalfIdx].t+lKneeDir),RGBA8(0xff,0xff,0xff,0xff));
	//g_pAuxGeom->DrawLine(rAnimLocationCurr.q*gSkeletonAbsolutPose[rCalfIdx].t,RGBA8(0xff,0xff,0xff,0xff),rAnimLocationCurr.q*(gSkeletonAbsolutePose[lCalfIdx].t+rKneeDir),RGBA8(0xff,0xff,0xff,0xff));

	assert(m_pFeetData->m_lAnkle.IsValid());
	assert(m_pFeetData->m_lAnkle.IsValid());

	QuatT lFoot = m_pFeetData->m_lAnkle;
	pSkeletonPose->SetHumanLimbIK_Local(lFoot.t, "LftLeg01");
	m_AbsolutePoseLeftFoot=lFoot;
	Util::AdjustFeetHelper(
		m_AbsolutePoseLeftCalf,
		m_RelativePoseLeftCalf,
		m_AbsolutePoseLeftTight,
		m_AbsolutePoseLeftFoot,
		m_RelativePoseLeftFoot,
		m_pFeetData->m_lCross,
		m_pFeetData->m_lKneeDir, 20, 0.002f );

	QuatT rFoot = m_pFeetData->m_rAnkle;
	pSkeletonPose->SetHumanLimbIK_Local(rFoot.t, "RgtLeg01");
	m_AbsolutePoseRightFoot=rFoot;
	Util::AdjustFeetHelper(
		m_AbsolutePoseRightCalf,
		m_RelativePoseRightCalf,
		m_AbsolutePoseRightTight,
		m_AbsolutePoseRightFoot,
		m_RelativePoseRightFoot,
		m_pFeetData->m_rCross,
		m_pFeetData->m_rKneeDir, 20, 0.002f);

	return true;
}

/*

  CFeetLock

*/

CFeetLock::CFeetLock()
{
	::CryCreateClassInstance<IAnimationPoseModifier>(
		"AnimationPoseModifier_FeetPoseStore", m_store);
	assert(m_store.get());

	CFeetPoseStore* pStore = static_cast<CFeetPoseStore*>(m_store.get());
	pStore->m_pFeetData = &m_feetData;

	::CryCreateClassInstance<IAnimationPoseModifier>(
		"AnimationPoseModifier_FeetPoseRestore", m_restore);
	assert(m_restore.get());

	CFeetPoseRestore* pRestore = static_cast<CFeetPoseRestore*>(m_restore.get());
	pRestore->m_pFeetData = &m_feetData;
}
