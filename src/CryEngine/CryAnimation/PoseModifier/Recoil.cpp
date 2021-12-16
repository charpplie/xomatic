#include "StdAfx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "PoseModifierHelper.h"
#include "Recoil.h"

namespace PoseModifier {


/*
CRecoil
*/

CRYREGISTER_CLASS(CRecoil)

//

CRecoil::CRecoil()
{
	m_state.Reset();
	m_stateExecute.Reset();
	m_bStateUpdate = false;
}

CRecoil::~CRecoil()
{
}


// IAnimationPoseModifier

bool CRecoil::Prepare(const SAnimationPoseModiferParams& params)
{
	if (!m_bStateUpdate)
		return true;

	m_stateExecute = m_state;
	m_bStateUpdate = false;
	return true;
}

SPU_INDIRECT(CommandBufferExecute(ML))
bool CRecoil::Execute(const SAnimationPoseModiferParams& params)
{
	if (m_stateExecute.time >= m_stateExecute.duration * 2.0f)
		return false;

	CModelSkeleton* const __restrict pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);
	int16* const __restrict pIdxArray = pModelSkeleton->m_IdxArray;
	QuatT* const __restrict pRelPose = SPU_PTR_SELECT(params.pPoseRelative, gSkeletonRelativePose);
	QuatT* const __restrict	pAbsPose = SPU_PTR_SELECT(params.pPoseAbsolute, gSkeletonAbsolutePose);

	f32 tn = m_stateExecute.time / m_stateExecute.duration;
	m_stateExecute.time += params.timeDelta;

	//-------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------

	int32 RWeaponBoneIdx		= pModelSkeleton->m_RecoilRWeaponBoneIdx;
#if !defined(__SPU__)
	if (RWeaponBoneIdx<0)
	{	
	//	AnimFileWarning(PoseModifierHelper::GetCharInstance(params)->m_pModel->GetModelFilePath(),"CryAnimation: Invalid Bone Index");
		return false;
	}
#endif
//	QuatT WeaponWorld			= QuatT(params.locationNextAnimation)*pAbsPose[WeaponBoneIdx];
	QuatT RWeaponWorld			= pAbsPose[RWeaponBoneIdx];

	f32 fImpact = RecoilEffect(tn);
	//	float fColor[4] = {0,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rRecoil.m_fAnimTime: %f   fImpact: %f",m_state.time,fImpact);	g_YLine+=16.0f;
	//	g_YLine+=16.0f;

	Vec3 vWWeaponX			= RWeaponWorld.GetColumn0();
	Vec3 vWWeaponY			= RWeaponWorld.GetColumn1();
	Vec3 vWWeaponZ			= RWeaponWorld.GetColumn2();
	Vec3 vWRecoilTrans	= (-vWWeaponY*fImpact*m_stateExecute.strengh)+(vWWeaponZ*fImpact*m_stateExecute.strengh*0.4f); 

//	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
//	g_pAuxGeom->DrawLine(WeaponWorld.t,RGBA8(0x3f,0x3f,0x3f,0x00), WeaponWorld.t+vWWeaponX,RGBA8(0xff,0x00,0x00,0x00) );
//	g_pAuxGeom->DrawLine(WeaponWorld.t,RGBA8(0x3f,0x3f,0x3f,0x00), WeaponWorld.t+vWWeaponY,RGBA8(0x00,0xff,0x00,0x00) );
//	g_pAuxGeom->DrawLine(WeaponWorld.t,RGBA8(0x3f,0x3f,0x3f,0x00), WeaponWorld.t+vWWeaponZ,RGBA8(0x00,0x00,0xff,0x00) );


	const char* strRIKSolver=pModelSkeleton->m_strRecoilRIKHandle;
	Quat qRRealHandRot;
	uint32 nREndEffector=0;
	if (m_stateExecute.arms&1)
	{
		uint64 nHandle = *(uint64*)strRIKSolver;
		int32 idxDefinition = pModelSkeleton->GetLimbDefinitionIdx(nHandle);
		if (idxDefinition < 0)
			return 0;
		const IKLimbType& rIKLimbType = pModelSkeleton->m_IKLimbTypes[idxDefinition];
		uint32 numLinks = rIKLimbType.m_arrJointChain.size();
		nREndEffector = rIKLimbType.m_arrJointChain[numLinks-1].m_idxJoint;
		Vec3 vRealHandPos = pAbsPose[nREndEffector].t;
		qRRealHandRot = pAbsPose[nREndEffector].q;
		QuatT LocalGoal		=	QuatT(vRealHandPos+vWRecoilTrans, Quat(IDENTITY));
		PoseModifierHelper::IK_Solver( pModelSkeleton, strRIKSolver, LocalGoal, pRelPose, pAbsPose);
	}


	const char* strLIKSolver=pModelSkeleton->m_strRecoilLIKHandle;
	Quat qLRealHandRot;
	uint32 nLEndEffector=0;
	if (m_stateExecute.arms&2)
	{
		if (m_stateExecute.arms==2)
		{
			int32 LWeaponBoneIdx		= pModelSkeleton->m_RecoilLWeaponBoneIdx;
			if (LWeaponBoneIdx<0)
				return false;
			QuatT LWeaponWorld			= pAbsPose[LWeaponBoneIdx];
			vWWeaponX			= LWeaponWorld.GetColumn0();
			vWWeaponY			= LWeaponWorld.GetColumn1();
			vWWeaponZ			= LWeaponWorld.GetColumn2();
			vWRecoilTrans	= (-vWWeaponY*fImpact*m_stateExecute.strengh)+(vWWeaponZ*fImpact*m_stateExecute.strengh*0.4f); 
		}

		uint64 nHandle = *(uint64*)strLIKSolver;
		int32 idxDefinition = pModelSkeleton->GetLimbDefinitionIdx(nHandle);
		if (idxDefinition < 0)
			return 0;
		const IKLimbType& rIKLimbType = pModelSkeleton->m_IKLimbTypes[idxDefinition];
		uint32 numLinks = rIKLimbType.m_arrJointChain.size();
		nLEndEffector = rIKLimbType.m_arrJointChain[numLinks-1].m_idxJoint;
		Vec3 vRealHandPos = pAbsPose[nLEndEffector].t;
		qLRealHandRot = pAbsPose[nLEndEffector].q;
		QuatT LocalGoal		=	QuatT(vRealHandPos+vWRecoilTrans, Quat(IDENTITY));
		PoseModifierHelper::IK_Solver( pModelSkeleton, strLIKSolver, LocalGoal, pRelPose, pAbsPose);
	}

	uint32 numJoints=pModelSkeleton->m_RecoilJoints.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		int32 arm = pModelSkeleton->m_RecoilJoints[i].m_nArm;
		if (m_stateExecute.arms&arm)
		{
			int32 id		=pModelSkeleton->m_RecoilJoints[i].m_nIdx;
			f32 delay		=pModelSkeleton->m_RecoilJoints[i].m_fDelay;
			f32 weight	=pModelSkeleton->m_RecoilJoints[i].m_fWeight;
			int32 _p0 = pModelSkeleton->GetParent(id)->m_idx;
			pAbsPose[id].t += -vWWeaponY*RecoilEffect(tn-delay)*m_stateExecute.strengh*weight;  //pelvis
			pRelPose[id] = pAbsPose[_p0].GetInverted() * pAbsPose[id];
		}
	}

	if (m_stateExecute.arms&1)
	{
		pAbsPose[nREndEffector].q = qRRealHandRot;
		int32 pr = pModelSkeleton->m_arrModelJoints[nREndEffector].m_idxParent;
		pRelPose[nREndEffector].q = !pAbsPose[pr].q * pAbsPose[nREndEffector].q;
	}

	if (m_stateExecute.arms&2)
	{
		pAbsPose[nLEndEffector].q = qLRealHandRot;
		int32 pr = pModelSkeleton->m_arrModelJoints[nLEndEffector].m_idxParent;
		pRelPose[nLEndEffector].q = !pAbsPose[pr].q * pAbsPose[nLEndEffector].q;
	}

	for (uint32 i=1; i<params.jointCount; i++)
	{
		ANIM_ASSET_ASSERT( pRelPose[i].q.IsUnit() );
		ANIM_ASSET_ASSERT( pRelPose[i].IsValid() );
		int32 p=pModelSkeleton->m_arrModelJoints[i].m_idxParent;
		pRelPose[p].q.NormalizeSafe();
		pAbsPose[i] = pAbsPose[p] * pRelPose[i];
		pAbsPose[i].q.NormalizeSafe();
	}
#ifdef _DEBUG
	for (uint32 j=0; j<params.jointCount; j++)
	{
		ANIM_ASSET_ASSERT( pRelPose[j].q.IsUnit() );
		ANIM_ASSET_ASSERT( pAbsPose[j].q.IsUnit() );
		ANIM_ASSET_ASSERT( pRelPose[j].IsValid() );
		ANIM_ASSET_ASSERT( pAbsPose[j].IsValid() );
	}
#endif			

	return true;
}

void CRecoil::Synchronize()
{
}



f32 CRecoil::RecoilEffect(f32 t) 
{
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	f32 sq2 = sqrtf(m_stateExecute.kickin);
	f32 scale = sq2 + gf_PI;

	f32 x = t*scale - sq2;
	if (x < 0.0f)
		return (-(x*x) + 2.0f) * 0.5f;
	return (cosf(x) + 1.0f) * 0.5f;
}

} // namespace PoseModifier
