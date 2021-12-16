#include "StdAfx.h"

#include "LookAtSimple.h"

using namespace AnimPoseModifier;

/*
CLookAtSimple
*/

CRYREGISTER_CLASS(CLookAtSimple)

//

CLookAtSimple::CLookAtSimple() 
{
	m_state.jointId = -1;
	m_state.jointOffsetRelative.zero();
	m_state.targetGlobal.zero();
	m_state.weight = 1.0f;
};

CLookAtSimple::~CLookAtSimple() 
{
}

//

bool CLookAtSimple::ValidateJointId(ICharacterModelSkeleton* pModelSkeleton)
{
	if (m_stateExecute.jointId < 0)
		return false;

	return m_stateExecute.jointId < pModelSkeleton->GetJointCount();
}

// IAnimationPoseModifier

bool CLookAtSimple::ExecutePreProcess()
{
	m_stateExecute = m_state;

	return true;
}

bool CLookAtSimple::Execute(const SAnimationPoseModiferParams& params)
{
	// TODO: Once IAnimationPoseModifier supports this call remove this.
	ExecutePreProcess();

	ICharacterModelSkeleton* pModelSkeleton = params.GetICharacterModelSkeleton();

	if (!ValidateJointId(pModelSkeleton))
		return false;

	QuatT& transformation = params.pPoseAbsolute[m_stateExecute.jointId];

	Vec3 offsetAbsolute = m_stateExecute.jointOffsetRelative * transformation.q;
	Vec3 targetAbsolute = m_stateExecute.targetGlobal - params.locationNextAnimation.t;
	Vec3 directionAbsolute = (targetAbsolute - (transformation.t + offsetAbsolute)) * params.locationNextAnimation.q;
	directionAbsolute.Normalize();

	transformation.q = Quat::CreateNlerp(transformation.q, Quat::CreateRotationVDir(directionAbsolute, -gf_PI * 0.5f), m_stateExecute.weight);

	return true;
}
