//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Skeleton.cpp
//  Implementation of Skeleton class (Inverse Kinematics)
//
//	History:
//	March 18, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"

#include "FacialAnimation/FacialInstance.h"

#include "PoseModifier/PoseModifierHelper.h"

//--------------------------------------------------------------------------------------
//--------                     set Look-IK parameters                           --------
//--------------------------------------------------------------------------------------
void CSkeletonPose::SetLookIK(uint32 ik, f32 FOR, const Vec3& vLookAtTarget,const f32 *customBlends, bool allowAdditionalTransforms, bool ignoreAimBlend) 
{
	uint32 IsValid = vLookAtTarget.IsValid();

	if (ik)
	{
		if (IsValid==0)
		{
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,m_pInstance->GetFilePath(),	"CryAnimation: Look Target Invalid (%f %f %f)",vLookAtTarget.x,vLookAtTarget.y,vLookAtTarget.z );
#ifdef _DEBUG
			g_pISystem->debug_LogCallStack();
#endif
		}
	}

	//	float fColor[4] = {1,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"LookIK: %d   %12.8f    Target:(%12.8f %12.8f %12.8f) ",ik,FOR, LookAtTarget.x, LookAtTarget.y, LookAtTarget.z );	g_YLine+=16.0f;

	m_LookIK().m_UseLookIK=ik;	
	m_LookIK().m_AllowAdditionalTransforms=allowAdditionalTransforms;
	m_LookIK().m_LookIKFOR=FOR;
	if (IsValid && ik)
		m_LookIK().m_LookIKTarget=vLookAtTarget;



	f32 t0=1.0f-m_AimIK().m_AimIKBlend;
	f32 t1=m_AimIK().m_AimIKBlend;

	if (ignoreAimBlend)
	{
		t0=1.0f;
		t1=0.0f;
	}

	//default values
	m_LookIK().m_lookIKBlends[0] = 0.04f*t0;
	m_LookIK().m_lookIKBlends[1] = 0.06f*t0;
	m_LookIK().m_lookIKBlends[2] = 0.08f*t0;
	m_LookIK().m_lookIKBlends[3] = 0.15f*t0;
	m_LookIK().m_lookIKBlends[4] = 0.60f*t0+0.25f*t1;

	if (customBlends)
	{
		cryMemcpy(m_LookIK().m_lookIKBlends,customBlends,sizeof(f32)*LOOKIK_BLEND_RATIOS);
		m_LookIK().m_lookIKBlends[0] = m_LookIK().m_lookIKBlends[0]*t0;
		m_LookIK().m_lookIKBlends[1] = m_LookIK().m_lookIKBlends[1]*t0;
		m_LookIK().m_lookIKBlends[2] = m_LookIK().m_lookIKBlends[2]*t0;
		m_LookIK().m_lookIKBlends[3] = m_LookIK().m_lookIKBlends[3]*t0;
		m_LookIK().m_lookIKBlends[4] = m_LookIK().m_lookIKBlends[4]*t0+0.25f*t1;
	}
}

//--------------------------------------------------------------------------------------
//--------        get the position of the interpolated look IK target           --------
//--------------------------------------------------------------------------------------
uint8 CSkeletonPose::GetLookIKInterpolatedTarget( Vec3& vTarget )
{
	uint8 bRet = IsLookIKEnabled();
	if ( bRet )
	{
		vTarget = m_LookIK().m_LookIKTargetSmooth;
	}

	return bRet;
}

//--------------------------------------------------------------------------------------
//--------                procedural recoil animation                           --------
//--------------------------------------------------------------------------------------

void CSkeletonPose::ApplyRecoilAnimation(f32 fDuration, f32 fKinematicImpact, uint32 nArms ) 
{
	m_Recoil.m_fAnimTime				= 0.0f;
	m_Recoil.m_fDuration				= fDuration;
	m_Recoil.m_fStrengh					= fKinematicImpact; //recoil in cm
	m_Recoil.m_nArms						= nArms;   //1-right arm  2-left arm   3-both
};


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
f32 RecoilEffect( f32 t ) 
{
	if (t<0.0f)	t=0.0f;
	if (t>1.0f)	t=1.0f;

	f32 sq2		= sqrtf(2.0f);
	f32 scale	=	sq2+gf_PI;

	f32 x=t*scale-sq2;
	if (x<0.0f)
		return (-(x*x)+2.0f)*0.5f;
	else
		return (cosf(x)+1.0f)*0.5f;
}

static const float SPINE_MULTIPLIER = 1.5f;


void CSkeletonPose::PlayRecoilAnimation( const QuatT& rPhysEntityLocation ) 
{
	//return;

	if (m_Recoil.m_fAnimTime>=(m_Recoil.m_fDuration*2))
		return;

	// use local pointer to spare dereferencing
	CModelSkeleton * const __restrict pModelSkeleton = m_pModelSkeleton;
	int16* 			 const __restrict pIdxArray 			= pModelSkeleton->m_IdxArray;
	
	QuatT* 		const	__restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	QuatT* 		const	__restrict	pAbsolutePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsAbsolute[0], gSkeletonAbsolutePose );
	
	SpuStackValue<SRecoil> spuRecoil(m_Recoil);
	SRecoil &rRecoil = spuRecoil;

//	rRecoil.m_nArms=2;

	f32 tn = rRecoil.m_fAnimTime/rRecoil.m_fDuration;
	f32 fImpact = RecoilEffect(tn);

	rRecoil.m_fAnimTime += m_pInstance->m_fOriginalDeltaTime;

	//float fColor[4] = {0,1,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rRecoil.m_fAnimTime: %f   fImpact: %f",rRecoil.m_fAnimTime,fImpact);	g_YLine+=16.0f;
	//g_YLine+=16.0f;

	//-------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------

	int32 WeaponBoneIdx		= pIdxArray[eIM_RWeaponBoneIdx];
#if !defined(__SPU__)
	if (WeaponBoneIdx<0)
	{	
		AnimFileWarning(m_pInstance->m_pModel->GetModelFilePath(),"CryAnimation: Invalid Bone Index");
		return; //don't do anything
	}
#endif
	QuatT WeaponWorld			= rPhysEntityLocation*pAbsolutePose[WeaponBoneIdx];

	Vec3 vWWeaponX			= WeaponWorld.GetColumn0();
	Vec3 vWWeaponY			= WeaponWorld.GetColumn1();
	Vec3 vWWeaponZ			= WeaponWorld.GetColumn2();
	Vec3 vWRecoilTrans	= (-vWWeaponY*fImpact*rRecoil.m_fStrengh)+(vWWeaponZ*fImpact*rRecoil.m_fStrengh*0.4f); 
	
	Vec3 vLWeaponY			= pAbsolutePose[WeaponBoneIdx].q.GetColumn1();
	Vec3 vLWeaponZ			= pAbsolutePose[WeaponBoneIdx].q.GetColumn2();
	Vec3 vLRecoilTrans	= (-vLWeaponY*fImpact*rRecoil.m_fStrengh)+(vLWeaponZ*fImpact*rRecoil.m_fStrengh*0.4f); 

	//g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
	//g_pAuxGeom->DrawLine(WeaponWorld.t,RGBA8(0x3f,0x3f,0x3f,0x00), WeaponWorld.t+vWWeaponX,RGBA8(0xff,0x00,0x00,0x00) );
	//g_pAuxGeom->DrawLine(WeaponWorld.t,RGBA8(0x3f,0x3f,0x3f,0x00), WeaponWorld.t+vWWeaponY,RGBA8(0x00,0xff,0x00,0x00) );
	//g_pAuxGeom->DrawLine(WeaponWorld.t,RGBA8(0x3f,0x3f,0x3f,0x00), WeaponWorld.t+vWWeaponZ,RGBA8(0x00,0x00,0xff,0x00) );


	int32 l0 = pIdxArray[eIM_LClavicleIdx];
	int32 l1 = pIdxArray[eIM_LUpperArmIdx];
	int32 l2 = pIdxArray[eIM_LForeArmIdx];
	int32 l3 = pIdxArray[eIM_LHandIdx];
	int32 _l0 = pModelSkeleton->GetParent(l0)->m_idx;
	int32 _l1 = pModelSkeleton->GetParent(l1)->m_idx;
	int32 _l2 = pModelSkeleton->GetParent(l2)->m_idx;

	if (rRecoil.m_nArms&2)
	{
		int32 LHand2Idx				= pIdxArray[eIM_LHandIdx];
		Vec3 vRealLHandPos		= rPhysEntityLocation*pAbsolutePose[LHand2Idx].t;
		Quat qRealLHandRot		= pAbsolutePose[LHand2Idx].q;
		Vec3 LocalGoalL=rPhysEntityLocation.GetInverted()*(vRealLHandPos+vWRecoilTrans*0.7f);
		SetHumanLimbIK_Local(LocalGoalL,"LftArm01");
		pAbsolutePose[LHand2Idx].q	=	qRealLHandRot;
		int32 pl=m_parrModelJoints[LHand2Idx].m_idxParent;
		pRelativePose[LHand2Idx].q = !pAbsolutePose[pl].q * pAbsolutePose[LHand2Idx].q;
	}

	int32 r0 = pIdxArray[eIM_RClavicleIdx];
	int32 r1 = pIdxArray[eIM_RUpperArmIdx];
	int32 r2 = pIdxArray[eIM_RForeArmIdx];
	int32 r3 = pIdxArray[eIM_RHandIdx];
	int32 _r0 = pModelSkeleton->GetParent(r0)->m_idx;
	int32 _r1 = pModelSkeleton->GetParent(r1)->m_idx;
	int32 _r2 = pModelSkeleton->GetParent(r2)->m_idx;
	if (rRecoil.m_nArms&1)
	{
		int32 RHand2Idx				= pIdxArray[eIM_RHandIdx];
		Vec3 vRealRHandPos		= rPhysEntityLocation*pAbsolutePose[RHand2Idx].t;
		Quat qRealRHandRot		= pAbsolutePose[RHand2Idx].q;
		Vec3 LocalGoalR=rPhysEntityLocation.GetInverted()*(vRealRHandPos+vWRecoilTrans);

		SetHumanLimbIK_Local(LocalGoalR,"RgtArm01");

		pAbsolutePose[RHand2Idx].q	=	qRealRHandRot;
		int32 pr=m_parrModelJoints[RHand2Idx].m_idxParent;
		pRelativePose[RHand2Idx].q = !pAbsolutePose[pr].q * pAbsolutePose[RHand2Idx].q;
	}


//	int32 bp = pIdxArray[eIM_BipIdx];

	int32 tl = pIdxArray[eIM_LThighIdx];
	int32 kl = pIdxArray[eIM_LCalfIdx];

	int32 tr = pIdxArray[eIM_RThighIdx];
	int32 kr = pIdxArray[eIM_RCalfIdx];

	int32 p0 = pIdxArray[eIM_PelvisIdx];
	int32 s0 = pIdxArray[eIM_Spine0Idx];
	int32 s1 = pIdxArray[eIM_Spine1Idx];
	int32 s2 = pIdxArray[eIM_Spine2Idx];
	int32 s3 = pIdxArray[eIM_Spine3Idx];
	int32 n0 = pIdxArray[eIM_NeckIdx];

	int32 _tl = pModelSkeleton->GetParent(tl)->m_idx;
	int32 _kl = pModelSkeleton->GetParent(kl)->m_idx;

	int32 _tr = pModelSkeleton->GetParent(tr)->m_idx;
	int32 _kr = pModelSkeleton->GetParent(kr)->m_idx;

	int32 _p0 = pModelSkeleton->GetParent(p0)->m_idx;
	int32 _s0 = pModelSkeleton->GetParent(s0)->m_idx;
	int32 _s1 = pModelSkeleton->GetParent(s1)->m_idx;
	int32 _s2 = pModelSkeleton->GetParent(s2)->m_idx;
	int32 _s3 = pModelSkeleton->GetParent(s3)->m_idx;
	int32 _n0 = pModelSkeleton->GetParent(n0)->m_idx;

	Vec3 vLRecoilTrans1	= -vLWeaponY*RecoilEffect(tn-0.20f)*m_Recoil.m_fStrengh; 
	Vec3 vLRecoilTrans2	= -vLWeaponY*RecoilEffect(tn-0.30f)*m_Recoil.m_fStrengh; 
	Vec3 vLRecoilTrans3	= -vLWeaponY*RecoilEffect(tn-0.40f)*m_Recoil.m_fStrengh; 
	Vec3 vLRecoilTrans4	= -vLWeaponY*RecoilEffect(tn-0.50f)*m_Recoil.m_fStrengh; 
	Vec3 vLRecoilTrans5	= -vLWeaponY*RecoilEffect(tn-0.60f)*m_Recoil.m_fStrengh; 

	pAbsolutePose[tl].t += vLRecoilTrans5*0.05f;
	pAbsolutePose[tr].t += vLRecoilTrans5*0.05f;

	pAbsolutePose[p0].t += vLRecoilTrans5*0.15f;
	pAbsolutePose[s0].t += vLRecoilTrans5*0.20f*SPINE_MULTIPLIER;
	pAbsolutePose[s1].t += vLRecoilTrans4*0.25f*SPINE_MULTIPLIER;
	pAbsolutePose[s2].t += vLRecoilTrans3*0.30f*SPINE_MULTIPLIER;
	pAbsolutePose[s3].t += vLRecoilTrans2*0.35f*SPINE_MULTIPLIER;
	pAbsolutePose[n0].t += vLRecoilTrans1*0.40f*SPINE_MULTIPLIER;

	if (rRecoil.m_nArms&2)
	{
		pAbsolutePose[l0].t += vLRecoilTrans1*0.45f; //clavicle
		pAbsolutePose[l1].t += vLRecoilTrans*0.50f;
	}

	if (rRecoil.m_nArms&1)
	{
		pAbsolutePose[r0].t += vLRecoilTrans1*0.50f; //clavicle
		pAbsolutePose[r1].t += vLRecoilTrans*0.60f;
	}



	pRelativePose[kl] = pAbsolutePose[_kl].GetInverted() * pAbsolutePose[kl];
	pRelativePose[tl] = pAbsolutePose[_tl].GetInverted() * pAbsolutePose[tl];

	pRelativePose[kr] = pAbsolutePose[_kr].GetInverted() * pAbsolutePose[kr];
	pRelativePose[tr] = pAbsolutePose[_tr].GetInverted() * pAbsolutePose[tr];

	pRelativePose[p0] = pAbsolutePose[_p0].GetInverted() * pAbsolutePose[p0];

	pRelativePose[s0] = pAbsolutePose[_s0].GetInverted() * pAbsolutePose[s0];
	pRelativePose[s1] = pAbsolutePose[_s1].GetInverted() * pAbsolutePose[s1];
	pRelativePose[s2] = pAbsolutePose[_s2].GetInverted() * pAbsolutePose[s2];
	pRelativePose[s3] = pAbsolutePose[_s3].GetInverted() * pAbsolutePose[s3];
	pRelativePose[n0] = pAbsolutePose[_n0].GetInverted() * pAbsolutePose[n0];

	if (rRecoil.m_nArms&2)
	{
		pRelativePose[l0] = pAbsolutePose[_l0].GetInverted() * pAbsolutePose[l0];
		pRelativePose[l1] = pAbsolutePose[_l1].GetInverted() * pAbsolutePose[l1];
		pRelativePose[l2] = pAbsolutePose[_l2].GetInverted() * pAbsolutePose[l2];
	}

	if (rRecoil.m_nArms&1)
	{
		pRelativePose[r0] = pAbsolutePose[_r0].GetInverted() * pAbsolutePose[r0];
		pRelativePose[r1] = pAbsolutePose[_r1].GetInverted() * pAbsolutePose[r1];
		pRelativePose[r2] = pAbsolutePose[_r2].GetInverted() * pAbsolutePose[r2];
	}

	uint32 numJoints = GetPoseDataWriteable()->m_jointsAbsolute.size();
	for (uint32 i=1; i<numJoints; i++)
		pAbsolutePose[i]	= pAbsolutePose[m_parrModelJoints[i].m_idxParent] * pRelativePose[i];
};
