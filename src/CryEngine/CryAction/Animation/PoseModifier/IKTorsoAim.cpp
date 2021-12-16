#include "StdAfx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>

#include "IKTorsoAim.h"

CRYREGISTER_CLASS(CIKTorsoAim)

CIKTorsoAim::STorsoAim_CVars CIKTorsoAim::s_CVars;

//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CIKTorsoAim::STorsoAim_CVars::InitCvars()
{
	if (m_initialized)
		return;

	m_initialized = true;

	REGISTER_CVAR( STAP_DEBUG,									0, VF_NULL, "Enable STAP Debug Rendering" );
	REGISTER_CVAR( STAP_DISABLE,								0, VF_NULL, "Disable torso orientation" );
	REGISTER_CVAR( STAP_TRANSLATION_FUDGE,			1, VF_NULL, "Enable STAP Translation Fudge" );
	REGISTER_CVAR( STAP_TRANSLATION_FEATHER,		1, VF_NULL, "Enable STAP Translation Feathering" );
	REGISTER_CVAR( STAP_LOCK_EFFECTOR,					0, VF_NULL, "Lock the STAP Effector Joint" );
	REGISTER_CVAR( STAP_OVERRIDE_TRACK_FACTOR, -1.0f, VF_NULL, "Override the base anim tracking factor" );
}

void CIKTorsoAim::STorsoAim_CVars::ReleaseCVars()
{
	if (gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("STAP_DEBUG", true);
		gEnv->pConsole->UnregisterVariable("STAP_DISABLE", true);
		gEnv->pConsole->UnregisterVariable("STAP_TRANSLATION_FUDGE", true);
		gEnv->pConsole->UnregisterVariable("STAP_TRANSLATION_FEATHER", true);
		gEnv->pConsole->UnregisterVariable("STAP_LOCK_EFFECTOR", true);
		gEnv->pConsole->UnregisterVariable("STAP_OVERRIDE_TRACK_FACTOR", true);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CIKTorsoAim::CIKTorsoAim() 
{	
	Init();	
};

CIKTorsoAim::~CIKTorsoAim() 
{
}

//

void CIKTorsoAim::Init()
{
	m_blend		= 0.0f;

	m_targetDirection = Vec3(ZERO);
	m_viewOffset = Vec3(ZERO);
	m_absoluteTargetPosition = Vec3(ZERO);
	m_aimJoint = -1;
	m_effectorJoint = -1;
	m_numParents = 0;
	m_weights[0] = 1.0f;
	m_baseAnimTrackFactor = 1.0f;
	m_aimToEffector = Vec3(ZERO);
	m_effectorDirQuat.SetIdentity();

	s_CVars.InitCvars();
}

void CIKTorsoAim::SetBlendWeight(float blend)
{
	m_blend = blend;
}

void CIKTorsoAim::SetTargetDirection(const Vec3& direction)
{
	m_targetDirection = direction;
}

void CIKTorsoAim::SetViewOffset(const Vec3& offset)
{
	m_viewOffset = offset;
}


void CIKTorsoAim::SetAbsoluteTargetPosition( const Vec3& targetPosition )
{
	m_absoluteTargetPosition = targetPosition;
}

void CIKTorsoAim::SetFeatherWeights(uint32 weights, const f32 *customBlends )
{
	memcpy(m_weights, customBlends, sizeof(f32) * weights);
	m_numParents = weights-1;
}

void CIKTorsoAim::SetJoints(uint32 jntEffector, uint32 jntAdjustment)
{
	m_aimJoint		= jntAdjustment;
	m_effectorJoint = jntEffector;
}

void CIKTorsoAim::SetBaseAnimTrackFactor(float factor)
{
	m_baseAnimTrackFactor = factor;
}

// IAnimationPoseModifier

bool CIKTorsoAim::Execute(const SAnimationPoseModiferParams& params)
{
	ISkeletonPose* pSkeletonPose = params.GetISkeletonPose();

	if ((m_effectorJoint < 0) || (m_aimJoint < 0))
		return false;

	bool ShowDebug									= s_CVars.STAP_DEBUG != 0;
	bool StapDisable								= s_CVars.STAP_DISABLE != 0;
	bool DoCameraTranslationFix			= s_CVars.STAP_TRANSLATION_FUDGE != 0;
	bool DoCameraTranslationFeather = s_CVars.STAP_TRANSLATION_FEATHER != 0;
	bool StapLockEffector						= s_CVars.STAP_LOCK_EFFECTOR != 0;
	float OverrideTrackFactor				= s_CVars.STAP_OVERRIDE_TRACK_FACTOR;

	if (StapDisable)
	{
		return false;
	}

	const float TRANSLATION_PIN_SPEED = 1.0f;
	f32 timeStep = params.timeDelta * TRANSLATION_PIN_SPEED;

	QuatT &effJointTrans = params.pPoseAbsolute[m_effectorJoint];
	QuatT &aimJointTrans = params.pPoseAbsolute[m_aimJoint];
	QuatT inverseInitialAim	= aimJointTrans.GetInverted();

	const float XPOS = 20.0f;
	const float YPOS = 40.0f;
	const float FONT_SIZE = 4.0f;
	const float FONT_COLOUR[4] = {1,1,1,1};
	if (ShowDebug)
	{
		gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, FONT_SIZE, FONT_COLOUR, false, "Torso Aim Pose: %f (%f, %f, %f)", m_blend, m_targetDirection.x, m_targetDirection.y, m_targetDirection.z);
	}
	const float SPHERE_SIZE = 0.1f;
	IRenderAuxGeom * pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	const ColorF red( 1.0f, 0.0f, 0.0f, 1.0f );
	const ColorF green( 0.0f, 1.0f, 0.0f, 1.0f );
	const ColorF yellow( 1.0f, 1.0f, 0.0f, 1.0f );
	const ColorF blue( 0.0f, 0.0f, 1.0f, 1.0f );

	const bool RemoveInitialRoll = true;

	Vec3 localTgtDir = !params.locationNextPhysics.q * m_targetDirection;

	Quat relHeadQuat;
	Vec3 effFwds  = effJointTrans.GetColumn1();

	Vec3 initialEffectorMS = effFwds;
	Vec3 targetEffectorMS = localTgtDir;

	//--- Calculate initial roll, ready for removal
	Vec3 idealRight = effJointTrans.GetColumn1().Cross(Vec3Constants<float>::fVec3_OneZ).GetNormalizedSafe();
	float upDP = effJointTrans.GetColumn2().Dot(idealRight);
	float initialRoll = (f32)(g_PI * 0.5f) - cry_acosf(upDP);

	Quat effectorDirQuat = m_effectorDirQuat;
	Vec3 aimToEffector = m_aimToEffector;
	float baseFactor = (float) __fsel(OverrideTrackFactor, OverrideTrackFactor, m_baseAnimTrackFactor);
	if (!StapLockEffector)
	{
		if (baseFactor > 0.0f)
		{
			effectorDirQuat = RemoveInitialRoll ? Quat::CreateRotationVDir(initialEffectorMS, initialRoll) : Quat::CreateRotationVDir(initialEffectorMS);
			aimToEffector		= inverseInitialAim * effJointTrans.t;

			if (baseFactor == 1.0f)
			{
				m_effectorDirQuat = effectorDirQuat;
				m_aimToEffector		= aimToEffector;
			}
			else
			{
				effectorDirQuat = Quat::CreateSlerp(m_effectorDirQuat, effectorDirQuat, baseFactor);
				aimToEffector.SetLerp(m_aimToEffector, aimToEffector, baseFactor);
			}
		}
	}
	Quat rebase			 = !effectorDirQuat * aimJointTrans.q;
	Quat targetDirQuat = Quat::CreateRotationVDir(targetEffectorMS);
	Quat absHeadQuat = targetDirQuat * rebase;
	relHeadQuat = inverseInitialAim.q * absHeadQuat;
	relHeadQuat.NormalizeSafe();

	if (ShowDebug)
	{
		//--- Debug draw
		Vec3 effTarget = effJointTrans.t + (effFwds * 100.0f);
		Vec3 localTgtPos = effJointTrans.t + (localTgtDir * 100.0f);

		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		pAux->SetRenderFlags( renderFlags );
		pAux->DrawSphere(params.locationNextPhysics * effTarget, SPHERE_SIZE, red);
		pAux->DrawSphere(params.locationNextPhysics * localTgtPos, SPHERE_SIZE, green);
		pAux->DrawLine(params.locationNextPhysics*aimJointTrans.t, yellow, params.locationNextPhysics * effTarget, red);
		pAux->DrawLine(params.locationNextPhysics*aimJointTrans.t, yellow, params.locationNextPhysics * localTgtPos, green);
		//--- Debug draw
	}


	bool useAbsoluteTarget = (m_absoluteTargetPosition.IsZero() == false);

	const bool useAbsoluteCameraPos = true;

	const Vec3 absoluteOffset = useAbsoluteTarget ? params.locationNextPhysics.q.GetInverted() * (m_absoluteTargetPosition - params.locationNextPhysics.t): ZERO;
	Vec3 relativeOffset = absoluteOffset - aimJointTrans.t;

	int16 idx = m_aimJoint;
	int16 childidx = -1;
	for (int16 i=m_numParents; i>=0; i--)
	{
		int16 pidx = pSkeletonPose->GetParentIDByID(idx);
		float weight = m_weights[i] * m_blend;

		if (childidx < 0)
		{
			Quat lerpSpine	= Quat::CreateNlerp( Quat(IDENTITY), relHeadQuat, weight);
			params.pPoseAbsolute[idx].q *= lerpSpine;

			if (useAbsoluteCameraPos)
			{
				Vec3 newEffectorT = aimJointTrans * aimToEffector;
				relativeOffset = absoluteOffset - newEffectorT;
			}
		}
		else
		{
			Quat targetPose = params.pPoseAbsolute[childidx].q * !params.pPoseRelative[childidx].q;
			Quat Diff = !params.pPoseAbsolute[idx].q * targetPose;
			Quat lerpSpine	= Quat::CreateNlerp( Quat(IDENTITY), Diff, weight);
			params.pPoseAbsolute[idx].q *= lerpSpine;
		}

		if (useAbsoluteTarget)
		{
			params.pPoseAbsolute[idx].t += relativeOffset * weight;
		}

		params.pPoseAbsolute[idx].t += m_viewOffset * weight;

		childidx = idx;
		assert(params.pPoseAbsolute[idx].IsValid());

		idx = pidx;
	}	
	idx = m_aimJoint;
	for (int16 i=m_numParents; i>=0; i--)
	{
		int16 pidx = pSkeletonPose->GetParentIDByID(idx);
		params.pPoseRelative[idx] = params.pPoseAbsolute[pidx].GetInverted() * params.pPoseAbsolute[idx];

		assert(params.pPoseRelative[idx].IsValid());

		idx = pidx;
	}	
	for (int i=1; i<params.jointCount; i++)
	{
		int32 p = pSkeletonPose->GetParentIDByID(i);
		params.pPoseAbsolute[i]	= params.pPoseAbsolute[p] * params.pPoseRelative[i];
		params.pPoseAbsolute[i].q.Normalize();
	}


	if (ShowDebug)
	{
		const float CAMERA_EFFECTOR_LEN = 5.0f;
		//--- Debug draw
		QuatT wsAim = params.locationNextPhysics * params.pPoseAbsolute[m_aimJoint];
		QuatT wsEffector = params.locationNextPhysics * params.pPoseAbsolute[m_effectorJoint];

		pAux->DrawLine(wsEffector.t, red, m_targetDirection, green);
		pAux->DrawSphere(wsEffector.t, SPHERE_SIZE, red);
		pAux->DrawSphere(wsAim.t, SPHERE_SIZE, green);

		pAux->DrawLine(wsEffector.t, red, wsEffector.t + (wsEffector.GetColumn1() * CAMERA_EFFECTOR_LEN), blue);


		//--- Draw effector AXIS
		const float AXIS_LENGTH = 0.5f;
		Vec3 DEBUG_init_right = effJointTrans.GetColumn0();
		Vec3 DEBUG_init_fwd = effJointTrans.GetColumn1();
		Vec3 DEBUG_init_up = effJointTrans.GetColumn2();
		pAux->DrawLine(params.locationNextPhysics * effJointTrans.t, red, params.locationNextPhysics * (effJointTrans.t + (DEBUG_init_right * AXIS_LENGTH)), red);
		pAux->DrawLine(params.locationNextPhysics * effJointTrans.t, green, params.locationNextPhysics * (effJointTrans.t + (DEBUG_init_fwd * AXIS_LENGTH)), green);
		pAux->DrawLine(params.locationNextPhysics * effJointTrans.t, blue, params.locationNextPhysics * (effJointTrans.t + (DEBUG_init_up * AXIS_LENGTH)), blue);
		//--- Debug draw
	}

	return true;
}
