#include "StdAfx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "PoseModifierHelper.h"
#include "GroundAlignment.h"

CRYREGISTER_CLASS(CGroundAlignment)

CGroundAlignment::CGroundAlignment()
{
	m_LFootGroundAlign = 0.0f;
	m_RFootGroundAlign = 0.0f;
}

CGroundAlignment::~CGroundAlignment()
{
}

//

uint32 CGroundAlignment::SetFootGroundAlignment(const SAnimationPoseModiferParams& params, const Vec3& normal2, int32 CalfIdx, int32 FootIdx, int32 HeelIdx, int32 Toe0Idx)
{
	CModelSkeleton* pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* const __restrict pRelPose = SPU_PTR_SELECT(params.pPoseRelative, gSkeletonRelativePose);
	QuatT* const __restrict	pAbsPose = SPU_PTR_SELECT(params.pPoseAbsolute, gSkeletonAbsolutePose);

	Vec3 normal = Vec3(normal2.x,normal2.y,normal2.z).GetNormalized();
	assert((fabs_tpl(1-(normal|normal)))<0.001); //check if unit-vector

	int32 pb1 = pModelSkeleton->m_arrModelJoints[FootIdx].m_idxParent;
	int32 pb2 = pModelSkeleton->m_arrModelJoints[HeelIdx].m_idxParent;
	int32 pb3 = pModelSkeleton->m_arrModelJoints[Toe0Idx].m_idxParent;

	Vec3 AxisX = -pAbsPose[FootIdx].GetColumn0();

	pAbsPose[FootIdx].q = Quat::CreateRotationV0V1(AxisX,normal)*pAbsPose[FootIdx].q;
	pAbsPose[FootIdx].q.NormalizeSafe(); // avoids a nasty release-build-only divide by zero
	pRelPose[FootIdx].q	= !pAbsPose[pb1].q * pAbsPose[FootIdx].q;
	pRelPose[FootIdx].q.Normalize();

	Vec3 v0	= (pAbsPose[CalfIdx].t-pAbsPose[FootIdx].t).GetNormalized();
	Vec3 v1 = pAbsPose[FootIdx].GetColumn1();
	f32 angle = acos_tpl( v0|v1 ); 

	f32 up_limit=0.90f;
	f32 down_limit=1.70f;
	f32 rad=0;

	if (angle<up_limit)
		rad = angle-up_limit;
	if (angle>down_limit)
		rad = angle-down_limit;

	Vec3 AxisZ = pAbsPose[FootIdx].GetColumn2();
	Quat qrad = Quat::CreateRotationAA(rad,AxisZ);

	pAbsPose[FootIdx].q	= qrad*pAbsPose[FootIdx].q;
	pRelPose[FootIdx].q = !pAbsPose[pb1].q * pAbsPose[FootIdx].q;
	pAbsPose[HeelIdx] = pAbsPose[pb2] * pRelPose[HeelIdx];
	pAbsPose[Toe0Idx] = pAbsPose[pb3] * pRelPose[Toe0Idx];

	return 1;
}

uint32 CGroundAlignment::PerformGroundAlignmentCCD(const SAnimationPoseModiferParams& params, const char* strLEG, const Plane& GroundPlane)
{
	ICharacterModel* pModel = params.GetICharacterModel();
	ICharacterModelSkeleton* pModelSkeleton = pModel->GetICharacterModelSkeleton();

	QuatT* const __restrict pRelPose = SPU_PTR_SELECT(params.pPoseRelative, gSkeletonRelativePose);
	QuatT* const __restrict	pAbsPose = SPU_PTR_SELECT(params.pPoseAbsolute, gSkeletonAbsolutePose);

	uint64 nHandle = *(uint64*)strLEG;

	f32* pFootGroundAlign = &m_LFootGroundAlign;
	int32 CalfIdx = pModelSkeleton->GetJointIDByType(eIM_LCalfIdx);	
	int32 FootIdx = pModelSkeleton->GetJointIDByType(eIM_LFootIdx);
	int32 HeelIdx = pModelSkeleton->GetJointIDByType(eIM_LHeelIdx);
	int32 Toe0Idx = pModelSkeleton->GetJointIDByType(eIM_LToe0Idx);

	if (nHandle==*(uint64*)SPU_MAIN_PTR("RgtLeg01"))
	{
		pFootGroundAlign = &m_RFootGroundAlign;
		CalfIdx = pModelSkeleton->GetJointIDByType(eIM_RCalfIdx);
		FootIdx = pModelSkeleton->GetJointIDByType(eIM_RFootIdx);
		HeelIdx = pModelSkeleton->GetJointIDByType(eIM_RHeelIdx);
		Toe0Idx = pModelSkeleton->GetJointIDByType(eIM_RToe0Idx);
	}

	if (CalfIdx < 0 || FootIdx < 0 || HeelIdx < 0 || Toe0Idx < 0)
		return 0;

	uint32 IkProcess=0;
	for (uint32 i=0; i<8; i++)
	{
		Quat rel_b0 = pRelPose[FootIdx].q;
		QuatT abs_b0 = pAbsPose[FootIdx];
		QuatT abs_b1 = pAbsPose[HeelIdx];
		QuatT abs_b2 = pAbsPose[Toe0Idx];

		SetFootGroundAlignment(params, GroundPlane.n, CalfIdx, FootIdx, HeelIdx, Toe0Idx);

		Vec3 FootPos = pAbsPose[FootIdx].t;
		Vec3 HeelPos = pAbsPose[HeelIdx].t;
		Vec3 Toe0Pos = pAbsPose[Toe0Idx].t;

		pRelPose[FootIdx].q = rel_b0;
		pAbsPose[FootIdx] = abs_b0;
		pAbsPose[HeelIdx] = abs_b1;
		pAbsPose[Toe0Idx] = abs_b2;

		//--------------------------------------------------------------------

		Ray ray_heel( HeelPos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );

		f32 heel_dist=0;
		Vec3 output_heel(0,0,0);
		bool _heel = Intersect::Ray_Plane( ray_heel, GroundPlane, output_heel );
		if (_heel)
		{
			heel_dist = output_heel.z-HeelPos.z;
			if (heel_dist<0.001f)
				heel_dist=0.0f;
		}

		//----------------------------------------------------------------

		Ray ray_toe0( Toe0Pos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );

		f32 toe0_dist=0;
		Vec3 output_toe0(0,0,0);
		bool _toe0 = Intersect::Ray_Plane( ray_toe0, GroundPlane, output_toe0 );
		if (_toe0)
		{
			toe0_dist = output_toe0.z-Toe0Pos.z;
			if (toe0_dist<0.001f)
				toe0_dist=0.0f;
		}

		f32 distance = max(toe0_dist,heel_dist);
		if (distance==0)
			break;

		FootPos.z += distance;

		PoseModifierHelper::IK_Solver(
			PoseModifierHelper::GetModelSkeleton(params), strLEG, QuatT(FootPos, Quat(IDENTITY)),
			pRelPose, pAbsPose);

		IkProcess++;
	}

	//--------------------------------------------------------------

	Quat fk_foot = pRelPose[FootIdx].q;
	SetFootGroundAlignment(params, GroundPlane.n, CalfIdx, FootIdx, HeelIdx, Toe0Idx);

	if (IkProcess)
	{
		// TODO: Should not access layer speed multiplier information!
		*pFootGroundAlign += 3.0f*fabsf(params.timeDelta * PoseModifierHelper::GetSkeletonAnim(params)->m_layers[0].m_transitionQueue.m_speedMultiplier);
	}
	else
	{
		Vec3	FootPos	= pAbsPose[FootIdx].t;
		Vec3	HeelPos	= pAbsPose[HeelIdx].t;
		Vec3	Toe0Pos	= pAbsPose[Toe0Idx].t;

		Ray ray_heel(HeelPos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1));
		f32 heel_dist=0;
		Vec3 output_heel(0,0,0);
		bool _heel = Intersect::Ray_Plane( ray_heel, GroundPlane, output_heel );
		if (_heel)
		{
			heel_dist = output_heel.z-HeelPos.z;
			if (heel_dist < 0.00f)
				heel_dist = 0.0f;
		}

		//----------------------------------------------------------------

		Ray ray_toe0( Toe0Pos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );
		f32 toe0_dist=0;
		Vec3 output_toe0(0,0,0);
		bool _toe0 = Intersect::Ray_Plane( ray_toe0, GroundPlane, output_toe0 );
		if (_toe0)
		{
			toe0_dist = output_toe0.z-Toe0Pos.z;
			if (toe0_dist < 0.00f)
				toe0_dist = 0.0f;
		}

		//do just alignment
		f32 distance = max(toe0_dist, heel_dist);
		if (distance>0)
		{	
			*pFootGroundAlign += 3.0f*fabsf(params.timeDelta * PoseModifierHelper::GetSkeletonAnim(params)->m_layers[0].m_transitionQueue.m_speedMultiplier);
			SetFootGroundAlignment(params, GroundPlane.n, CalfIdx, FootIdx, HeelIdx, Toe0Idx);
		}
		else
		{
			*pFootGroundAlign -= 3.0f*fabsf(params.timeDelta * PoseModifierHelper::GetSkeletonAnim(params)->m_layers[0].m_transitionQueue.m_speedMultiplier);
		}
	}

	if (*pFootGroundAlign > 1.0f)
		*pFootGroundAlign = 1.0f;
	if (*pFootGroundAlign < 0.0f)
		*pFootGroundAlign = 0.0f;

	f32 a = *pFootGroundAlign;
	f32 t = -(2*a*a*a - 3*a*a);
	f32 t0 = 1.0f-t;
	f32 t1 = t;
	pRelPose[FootIdx].q = fk_foot * t0 + pRelPose[FootIdx].q * t1;
	pRelPose[FootIdx].q.Normalize();

	int32 pb1 = pModelSkeleton->GetJointParentIDByID(FootIdx);
	int32 pb2 = pModelSkeleton->GetJointParentIDByID(HeelIdx);
	int32 pb3 = pModelSkeleton->GetJointParentIDByID(Toe0Idx);
	pAbsPose[FootIdx] = pAbsPose[pb1] * pRelPose[FootIdx];
	pAbsPose[HeelIdx] = pAbsPose[pb2] * pRelPose[HeelIdx];
	pAbsPose[Toe0Idx] = pAbsPose[pb3] * pRelPose[Toe0Idx];

	return 1;
};

void CGroundAlignment::MoveSkeletonVertical(const SAnimationPoseModiferParams& params, f32 vertical)
{
	CSkeletonAnim* pSkeletonAnim = PoseModifierHelper::GetSkeletonAnim(params);

	if ((pSkeletonAnim->m_IsAnimPlaying & 1) && pSkeletonAnim->m_AnimationDrivenMotion)
	{
		uint32 numJoints = params.jointCount;
		for (uint32 i=1; i<numJoints; i++)
			params.pPoseAbsolute[i].t.z += vertical;
		if (numJoints > 2)
			params.pPoseRelative[m_state.jointIndexPelvis] = params.pPoseAbsolute[0].GetInverted() * params.pPoseAbsolute[m_state.jointIndexPelvis];
	}
}

bool AlignJointToPlane(const SAnimationPoseModiferParams& params, uint32 jointIndex, const Vec3& jointVector, const Vec3& planeNormal)
{
	QuatT& jointAbsolute = params.pPoseAbsolute[jointIndex];
	Quat orientation = Quat::CreateRotationV0V1(jointVector, planeNormal) * jointAbsolute.q;
	PoseModifierHelper::SetJointAbsolute(params, jointIndex, &orientation, NULL);
	return true;
}

// IAnimationPoseModifier

bool CGroundAlignment::LimbAlign(const SAnimationPoseModiferParams& params, uint32 index, const char* ik, const Plane& plane)
{
	static const uint32 ITERATION_COUNT = 8;

	uint64 ikHandle = *(uint64*)ik;
	CModelSkeleton* pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);
	int32 limbDefinitionIndex = pModelSkeleton->GetLimbDefinitionIdx(ikHandle);
	if (limbDefinitionIndex < 0)
		return false;

	const IKLimbType* pIkLimbType = pModelSkeleton->GetLimbDefinition(limbDefinitionIndex);
	uint32 endEffectorIndex = pIkLimbType->m_arrJointChain.back().m_idxJoint;

	Vec3 jointVector;
	Vec3 target;
	for (uint32 i=0; i<ITERATION_COUNT; ++i)
	{
		jointVector = -(params.pPoseAbsolute[index].t - params.pPoseAbsolute[endEffectorIndex].t).GetNormalized();
		AlignJointToPlane(params, endEffectorIndex, jointVector, plane.n);

		Vec3 position = params.pPoseAbsolute[index].t;

		Ray ray(position + Vec3(0.0f, 0.0f, 0.6f), Vec3(0.0f, 0.0f, -1.0f));
		f32 distance = 0.0f;

		target = position;
		bool bHit = Intersect::Ray_Plane(ray, plane, target);
		if (bHit)
			distance = target.z - position.z;
		if (distance < 0.001f)
			break;
		
		target = params.pPoseAbsolute[endEffectorIndex].t;
		target.z += distance;

		PoseModifierHelper::IK_Solver(
			PoseModifierHelper::GetModelSkeleton(params), ik, QuatT(target, Quat(IDENTITY)),
			params.pPoseRelative, params.pPoseAbsolute);
	}
/*
	g_pAuxGeom->DrawCone(
		params.locationNextPhysics * params.pPoseAbsolute[index].t,
		params.locationNextPhysics.q * plane.n, 0.1f, 0.8f, RGBA8(0xff, 0xff, 0xff, 0xff));

	jointVector = -(params.pPoseAbsolute[index].t - params.pPoseAbsolute[endEffectorIndex].t).GetNormalized();
	g_pAuxGeom->DrawCone(
		params.locationNextPhysics * params.pPoseAbsolute[index].t,
		params.locationNextPhysics.q * jointVector, 0.1f, 0.8f, RGBA8(0xff, 0x80, 0x80, 0xff));
*/
	return true;
}

bool CGroundAlignment::Prepare(const SAnimationPoseModiferParams& params)
{
	CSkeletonPose* pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);

	m_state.jointIndexLeft = pSkeletonPose->GetJointIDByName("L Heel");
	m_state.jointIndexRight = pSkeletonPose->GetJointIDByName("R Heel");

	m_stateExecute = m_state;

	if (m_state.jointIndexLeft < 0 || m_state.jointIndexRight < 0)
		return false;

	return true;
}

const char* g_leftLeg = "LftLeg01";
const char* g_rightLeg = "RgtLeg01";

bool CGroundAlignment::ExecuteNew(const SAnimationPoseModiferParams& params)
{
	if (m_stateExecute.jointIndexLeft < 0 || m_stateExecute.jointIndexRight < 0)
		return false;

	LimbAlign(params, m_stateExecute.jointIndexLeft, "LftLeg01", m_stateExecute.planeLeft);
	LimbAlign(params, m_stateExecute.jointIndexRight, "RgtLeg01", m_stateExecute.planeRight);

	return true;
}

SPU_INDIRECT(CommandBufferExecute(ML))
bool CGroundAlignment::Execute(const SAnimationPoseModiferParams& params)
{
	if (m_stateExecute.alignSkeletonVertical)
		MoveSkeletonVertical(params, m_stateExecute.rootHeight);

	if (ExecuteNew(params))
		return true;

	PerformGroundAlignmentCCD(params, g_leftLeg, m_stateExecute.planeLeft);
	PerformGroundAlignmentCCD(params, g_rightLeg, m_stateExecute.planeRight);


#ifdef _DEBUG
	for (uint32 j=0; j<params.jointCount; j++)
	{
		ANIM_ASSET_ASSERT( params.pPoseRelative[j].q.IsUnit() );
		ANIM_ASSET_ASSERT( params.pPoseAbsolute[j].q.IsUnit() );
		ANIM_ASSET_ASSERT( params.pPoseRelative[j].IsValid() );
		ANIM_ASSET_ASSERT( params.pPoseAbsolute[j].IsValid() );
	}
#endif			

	return true;
}

void CGroundAlignment::Synchronize()
{
}
