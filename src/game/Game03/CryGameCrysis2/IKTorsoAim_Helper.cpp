/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Helper class to setup / update ik torso aim for first person 

-------------------------------------------------------------------------
History:
- 20-8-2009		Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"

#include <ICryAnimation.h>
#include "IKTorsoAim_Helper.h"
#include <CryExtension/CryCreateClassInstance.h>

CIKTorsoAim_Helper::CIKTorsoAim_Helper()
: m_initialized(false)
, m_enabled(false)
, m_blendTime(0.0f)
, m_cameraLockFactor(1.0f)
, m_actualTorsoOffset(ZERO)
{
	CryCreateClassInstance<CIKTorsoAim>("AnimationPoseModifier_IKTorsoAim", m_ikTorsoAim); 
	CryCreateClassInstance<ITransformationPinning>("AnimationPoseModifier_TransformationPin", m_transformationPin); 
}

void CIKTorsoAim_Helper::Update( CIKTorsoAim_Helper::SIKTorsoParams& ikTorsoParams )
{
	if (!m_initialized)
	{
		Init(ikTorsoParams);
	}

	const float frameTime = gEnv->pTimer->GetFrameTime();
	m_blendTime = m_enabled ? clamp(m_blendTime + (frameTime * ikTorsoParams.blendRate), 0.0f, 1.0f) : clamp(m_blendTime - (frameTime * ikTorsoParams.blendRate), 0.0f, 1.0f);

	//const float XPOS = 200.0f;
	//const float YPOS = 110.0f;
	//const float FONT_SIZE = 4.0f;
	//const float FONT_COLOUR[4] = {1,1,1,1};
	//gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, FONT_SIZE, FONT_COLOUR, false, "CIKTorsoAim_Helper::Update: %s", m_blendTime > 0.0f ? "update" : "dont update");

	if (m_blendTime <= 0.0f)
		return;

	float newFactor = m_cameraLockFactor;
	if (m_cameraBoneLocked)
	{
		newFactor = clamp(m_cameraLockFactor + (frameTime * ikTorsoParams.cameraLockBlendRate), 0.0f, 1.0f);
	}
	else
	{
		newFactor = 0.0f;
	}

	m_cameraLockFactor = newFactor;

	Interpolate(m_actualTorsoOffset, ikTorsoParams.viewOffset, 32.0f, gEnv->pTimer->GetFrameTime());

	m_ikTorsoAim->SetBlendWeight(m_blendTime);
	m_ikTorsoAim->SetTargetDirection(ikTorsoParams.aimDirection);
	m_ikTorsoAim->SetViewOffset(m_actualTorsoOffset);
	m_ikTorsoAim->SetAbsoluteTargetPosition(ikTorsoParams.targetPosition);
	m_ikTorsoAim->SetBaseAnimTrackFactor(newFactor);

	CIKTorsoAim *torsoAim = m_ikTorsoAim.get();
	ikTorsoParams.character->GetISkeletonAnim()->PushLayer(torsoAim);


	if (ikTorsoParams.shadowCharacter)
	{
		m_transformationPin->SetBlendWeight(ikTorsoParams.updateTranslationPinning ? m_blendTime : 0.0f);
		ikTorsoParams.character->GetISkeletonAnim()->PushLayer(m_transformationPin.get());
	}
}


void CIKTorsoAim_Helper::Init( CIKTorsoAim_Helper::SIKTorsoParams& ikTorsoParams )
{
	m_initialized = true;

	int effectorJoint = ikTorsoParams.character->GetISkeletonPose()->GetJointIDByName("Bip01 Camera");
	int aimJoint = ikTorsoParams.character->GetISkeletonPose()->GetJointIDByName("Bip01 Spine2");
	int pinJoint = ikTorsoParams.character->GetISkeletonPose()->GetJointIDByName("Bip01 Spine1");

	assert(effectorJoint != -1);
	assert(aimJoint != -1);
	assert(pinJoint != -1);

	m_ikTorsoAim->SetJoints(effectorJoint, aimJoint);

	const uint32 numWeights = 3;
	f32 weights[numWeights] = {0.4f, 0.75f, 1.0f};
	
	m_ikTorsoAim->SetFeatherWeights(numWeights, weights);

	if (ikTorsoParams.shadowCharacter)
	{
		m_transformationPin->SetSource(ikTorsoParams.shadowCharacter->GetISkeletonPose());
		m_transformationPin->SetJoint(pinJoint);
	}
}


void CIKTorsoAim_Helper::Enable(bool snap)
{
	m_enabled = true;
	if (snap)
	{
		m_blendTime = 1.0f;
	}
}

void CIKTorsoAim_Helper::Disable(bool snap)
{
	m_enabled = false;
	if (snap)
	{
		m_blendTime = 0.0f;
	}
}
