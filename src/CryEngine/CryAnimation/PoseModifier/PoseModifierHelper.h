#ifndef PoseModifierHelper_h
#define PoseModifierHelper_h

#include "../CharacterInstance.h"


namespace PoseModifierHelper {

ILINE ISkeletonPose* GetISkeletonPose(const SAnimationPoseModiferParams& params)
{
	return static_cast<CCharInstance*>(params.pCharacterInstance)->CCharInstance::GetISkeletonPose();
}

// NOTE: Get() helpers that don't return an interface should never be used!
// Only here for backward-compatibility.

ILINE CCharInstance* GetCharInstance(const SAnimationPoseModiferParams& params)
{
	return static_cast<CCharInstance*>(params.pCharacterInstance);
}

ILINE CSkeletonPose* GetSkeletonPose(const SAnimationPoseModiferParams& params)
{
	return &(static_cast<CCharInstance*>(params.pCharacterInstance)->m_SkeletonPose);
}

ILINE CSkeletonAnim* GetSkeletonAnim(const SAnimationPoseModiferParams& params)
{
	return &(static_cast<CCharInstance*>(params.pCharacterInstance)->m_SkeletonAnim);
}

ILINE CModelSkeleton* GetModelSkeleton(const SAnimationPoseModiferParams& params)
{
	ISkeletonPose *pSkeletonPose = static_cast<CCharInstance*>(params.pCharacterInstance)->CCharInstance::GetISkeletonPose();	
	CModelSkeleton* pModelSkeleton = static_cast<CSkeletonPose*>( pSkeletonPose )->m_pModelSkeleton;	
	return pModelSkeleton;
}

	void IK_Solver2Bones( const Vec3r& goal, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict pAbsolutePose);
	void IK_Solver3Bones( const Vec3r& goal, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict pAbsolutePose);
	void IK_Solver3Bones_Ivo( const Vec3r& goal, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict pAbsolutePose);
	void IK_SolverCCD(    const Vec3&  goal, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict pAbsolutePose);

} // namespace PoseModifierHelper

#endif // PoseModifierHelper_h
