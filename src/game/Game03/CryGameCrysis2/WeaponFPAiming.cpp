#include "StdAfx.h"

#include "IEntitySystem.h"
#include "ICryAnimation.h"
#include "WeaponFPAiming.h"
#include "GameCVars.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SSTAP_CVars::InitCvars()
{
	if (m_initialized)
		return;

	m_initialized = true;

	REGISTER_CVAR( STAP_MF_All,			1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Scope,		1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_ScopeVert,	0.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_HeavyWeapon,	1.5f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Up,			1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Down,			1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Left,			1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Right,		1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Front,		1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_Back,			1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_StrafeLeft,	1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_StrafeRight,	1.0f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_VertMotion,	0.4f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_VelFactorVert, 0.4f, VF_NULL, "" );
	REGISTER_CVAR( STAP_MF_VelFactorHoriz, 0.3f, VF_NULL, "" );
}

void SSTAP_CVars::ReleaseCVars()
{
	/*
	if (gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("STAP_MF_All", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Scope", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_ScopeVert", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_HeavyWeapon", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Up", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Down", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Left", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Right", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Front", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_Back", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_StrafeLeft", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_StrafeRight", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_VertMotion", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_VelFactorVert", true);
		gEnv->pConsole->UnregisterVariable("STAP_MF_VelFactorHoriz", true);
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static float Sign(float v)
{
	return v < 0 ? -1.0f : v > 0 ? 1.0f : 0;
}

static float SignedPow(float linearVal, float curvePower)
{
	return cry_powf(cry_fabsf(linearVal), curvePower) * Sign(linearVal);
}

void SParams_WeaponFPAiming::SetAnimation(int layerId, int animationId)
{
	animID[layerId] = animationId;
}

uint32 SParams_WeaponFPAiming::UpdateStatus(const SCachedItemAnimation &newAnimCache)
{
	uint32 ret = animCache.GetChangeFlags(newAnimCache);

	if (ret)
	{
		animCache = newAnimCache;
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SSTAP_CVars CWeaponFPAiming::s_stapCVars;

CWeaponFPAiming::CWeaponFPAiming()
	:	m_smoothedVelocity(ZERO)
	,	m_lastVelocity(ZERO)
	,	m_interpFront(0.0f)
	,	m_interpSide(0.0f)
	,	m_sprintFactor(0.0f)
{
	m_interpVert  = 0.0f;
	m_interpHoriz = 0.0f;
	m_initialTime = 0.0f;
	m_fallFromHeight = 0.0f;
	m_runFactor = 0.0f;
	m_falling = false;
	m_enabled = false;
	m_firstUpdate = true;
	memset(m_currentAnimID, -1, sizeof(m_currentAnimID));

	s_stapCVars.InitCvars();
}

void CWeaponFPAiming::SetActive(bool active)
{
	m_pendingEnabled = active;
}

void CWeaponFPAiming::Start(ISkeletonAnim *skelAnim)
{
	m_enabled = true;
}

void CWeaponFPAiming::Stop(ISkeletonAnim *skelAnim)
{
	if (m_data.stopOnExit && m_enabled)
	{
		
		for (int i=0; i<WeaponAnimLayerID::Total; i++)
		{
			if (m_data.layer+i > 0)
			{
				skelAnim->StopAnimationInLayer(m_data.layer+i, m_data.exitTime);
			}
			m_currentAnimID[i] = WeaponAimAnim::Total;
		}
	}
	m_enabled = false;
}

void CWeaponFPAiming::SnapSwitch(bool snap)
{
	m_snapSwitch = true;
}

void CWeaponFPAiming::Update(const SParams_WeaponFPAiming &params)
{
	ASSERT_IS_NOT_NULL( params.characterInst );
	ASSERT_IS_NOT_NULL( params.skelAnim );

	const bool hasValidParams = params.characterInst != 0 && params.skelAnim != 0;
	if (!hasValidParams)
	{
		return;
	}

	if (m_pendingEnabled != m_enabled)
	{
		if (m_pendingEnabled)
			Start(params.skelAnim);
		else
			Stop(params.skelAnim);
	}

	if (!m_enabled)
	{
		return;
	}

	const float STAP_MF_All					= s_stapCVars.STAP_MF_All;
	const float STAP_MF_Scope				= s_stapCVars.STAP_MF_Scope;
	const float STAP_MF_ScopeVert		= s_stapCVars.STAP_MF_ScopeVert;
	const float STAP_MF_HeavyWeapon	= s_stapCVars.STAP_MF_HeavyWeapon;
	const float STAP_MF_Up					= s_stapCVars.STAP_MF_Up;
	const float STAP_MF_Down				= s_stapCVars.STAP_MF_Down;
	const float STAP_MF_Left				= s_stapCVars.STAP_MF_Left;
	const float STAP_MF_Right				= s_stapCVars.STAP_MF_Right;
	const float STAP_MF_Front				= s_stapCVars.STAP_MF_Front;
	const float STAP_MF_Back				= s_stapCVars.STAP_MF_Back;
	const float STAP_MF_StrafeLeft	= s_stapCVars.STAP_MF_StrafeLeft;
	const float STAP_MF_StrafeRight	= s_stapCVars.STAP_MF_StrafeRight;
	const float STAP_MF_VertMotion	= s_stapCVars.STAP_MF_VertMotion;
	const float STAP_MF_VelFactorVert		= s_stapCVars.STAP_MF_VelFactorVert;
	const float STAP_MF_VelFactorHoriz	= s_stapCVars.STAP_MF_VelFactorHoriz;

	static const float SCOPE_LAND_FACTOR = 0.1f;

	static const float MIN_VERT_DIR = 0.2f;
	static const float MAX_VERT_DIR = 1.0f;
	static const float MIN_HORIZ_DIR  = 0.2f;
	static const float MAX_HORIZ_DIR  = 3.0f;
	static const float POW_VERT   = 1.0f;
	static const float POW_HORIZ  = 2.0f;
	static const float VERT_VEL_SCALE = -0.3f;
	static const float HORIZ_VEL_SCALE = 0.2f;
	static const float runEaseFactor = 0.2f;

	static const float MIN_SPEED = 0.2f;
	static const float MAX_SPEED = 4.0f;

	static const float MAX_FALL_HEIGHT = 5.0f;
	static const float MIN_FALL_HEIGHT = 0.2f;

	float sprintTransitionTime = params.isSprinting ? params.runToSprintBlendTime : params.sprintToRunBlendTime;
	float strafeFactor = STAP_MF_All;
	float rotationFactor = STAP_MF_All;
	float vertFactor = 1.0f;
	float horizFactor = 1.0f;
	if (params.isZoomed)
	{
		strafeFactor *= STAP_MF_Scope * m_data.strafeScopeFactor;
		vertFactor *= STAP_MF_ScopeVert;
		rotationFactor *= STAP_MF_Scope * m_data.rotateScopeFactor;
		horizFactor = -1.0f;
	}
	if (params.isHeavyWeapon)
	{
		rotationFactor *= STAP_MF_HeavyWeapon;
	}

	strafeFactor *= params.overlayFactor;
	rotationFactor *= params.overlayFactor;

	const float frameTime = gEnv->pTimer->GetFrameTime();
	if (frameTime > 0.0f)
	{
		Ang3 inputRot = params.inputRot;

		float horizInterp = 0.0f;
		float vertInterp  = 0.0f;
		//--- Generate our horizontal & vertical target additive factors
		float absAimDirVert = cry_fabsf(params.aimDirection.z) * rotationFactor;
		if (absAimDirVert > MIN_VERT_DIR)
		{
			float factor = (float)__fsel(params.aimDirection.z, STAP_MF_Up, -STAP_MF_Down);
			vertInterp = vertFactor * factor * cry_powf(min((absAimDirVert - MIN_VERT_DIR) / (MAX_VERT_DIR - MIN_VERT_DIR), 1.0f), POW_VERT);
		}
		vertInterp  += rotationFactor * STAP_MF_VelFactorVert * clamp(params.velocity.z * VERT_VEL_SCALE, -1.0f, 1.0f);
		vertInterp  += (inputRot.x * STAP_MF_VertMotion * rotationFactor);
		vertInterp = clamp(vertInterp, -1.0f, 1.0f);

		float absInputRotHoriz = cry_fabsf(inputRot.z) * rotationFactor;
		if (absInputRotHoriz > MIN_HORIZ_DIR)
		{
			float factor = (float)__fsel(inputRot.z, STAP_MF_Left, -STAP_MF_Right);
			horizInterp = horizFactor* rotationFactor * factor * cry_powf(min((absInputRotHoriz - MIN_HORIZ_DIR) / (MAX_HORIZ_DIR - MIN_HORIZ_DIR), 1.0f), POW_HORIZ);
		}
		Vec3 up(0.0f, 0.0f, 1.0f);
		Vec3 idealRight = up.Cross(params.aimDirection);
		idealRight.NormalizeSafe();
		float rightVel = idealRight.Dot(params.velocity);
		horizInterp  += rotationFactor * STAP_MF_VelFactorHoriz * clamp(rightVel * HORIZ_VEL_SCALE, -1.0f, 1.0f); 
		horizInterp = clamp(horizInterp, -1.0f, 1.0f);

		//--- Interpolate from our previous factors
		if ((m_interpVert < -1.0f) && (m_interpHoriz < -1.0f))
		{
			m_interpVert = vertInterp;
			m_interpHoriz = horizInterp;
		}
		else
		{
			const float decStep = (m_data.easeFactorDec * frameTime);
			const float incStep = (m_data.easeFactorInc * frameTime);
			float easeFactor = (float)__fsel(cry_fabsf(m_interpVert) - cry_fabsf(vertInterp), decStep, incStep);
			easeFactor = clamp(easeFactor, 0.0f, 1.0f);
			vertInterp  = m_interpVert = (vertInterp * easeFactor) + (m_interpVert * (1.0f - easeFactor));

			easeFactor = (float)__fsel(cry_fabsf(m_interpHoriz) - cry_fabsf(horizInterp), decStep, incStep);
			easeFactor = clamp(easeFactor, 0.0f, 1.0f);
			horizInterp = m_interpHoriz = (horizInterp * easeFactor) + (m_interpHoriz * (1.0f - easeFactor));
		}

		//--- Calculate run anim's additive factor
		float horizSpeed = params.velocity.GetLength2D();
		float runFactor	= !params.isOnGround || params.isZoomed ? 0.0f : (horizSpeed - MIN_SPEED) / (MAX_SPEED - MIN_SPEED);
		runFactor = clamp(runFactor, 0.0f, 1.0f);
		m_runFactor = ((runFactor * runEaseFactor) + (m_runFactor * (1.0f - runEaseFactor))) * params.overlayFactor;
		m_sprintFactor -= frameTime * (1.0f/sprintTransitionTime);
		m_sprintFactor = params.isSprinting ? 1.0f : max(m_sprintFactor, 0.0f);

		//--- Calculate movement acceleration
		static const float interpMultiplier = 1.2f;
		static const float velocityLowPassFilter = 6.0f;
		static const float accellerationSmoothing = 0.8f;
		static const float frontAugmentation = 3.0f;

		const Vec3 inputMoveAugmented = Vec3(
			params.inputMove.x * (1.0f / frontAugmentation),
			params.inputMove.y * frontAugmentation,
			0.0f);
		Interpolate(m_smoothedVelocity, inputMoveAugmented, velocityLowPassFilter, frameTime);

		const Vec3 velocityDerivative = (m_smoothedVelocity - m_lastVelocity) / frameTime;
		m_lastVelocity = m_smoothedVelocity;

		float interpFront = SignedPow(clamp(velocityDerivative.y * interpMultiplier, -1, 1), accellerationSmoothing);
		interpFront *= (float)__fsel(interpFront, STAP_MF_Front, STAP_MF_Back) * strafeFactor;
		Interpolate(m_interpFront, interpFront, velocityLowPassFilter, frameTime);
		float interpSide = SignedPow(clamp(velocityDerivative.x * interpMultiplier, -1, 1), accellerationSmoothing);
		interpSide *= (float)__fsel(interpFront, STAP_MF_StrafeLeft, STAP_MF_StrafeRight) * strafeFactor;
		Interpolate(m_interpSide, interpSide, velocityLowPassFilter, frameTime);
	}

	float transitionTime = (float)__fsel(params.transitionTime, params.transitionTime, m_firstUpdate ? m_data.entryTime : m_data.transitionTime);

	AnimInstall animInstalls[WeaponAnimLayerID::Total];
	const bool LAYER_SHOULD_LOOP[WeaponAnimLayerID::Total] = {true, true, true, true, true, true, true, false};
	const bool LAYER_SHOULD_BLEND[WeaponAnimLayerID::Total] = {params.blendBasePose, false, false, false, false, true, true, true};
	const float LAYER_BLEND_TIME[WeaponAnimLayerID::Total] = {transitionTime, 0.0f, 0.0f, 0.0f, 0.0f, sprintTransitionTime, transitionTime, transitionTime};

	bool haveIdleAnimation = true;
	if (params.isZoomed && !params.isHeavyWeapon)
		haveIdleAnimation = false;

	animInstalls[WeaponAnimLayerID::Base].anim		= WeaponAimAnim::Base;
	animInstalls[WeaponAnimLayerID::Vert].anim		= (m_interpVert >= 0.0f) ? WeaponAimAnim::Up : WeaponAimAnim::Down;
	animInstalls[WeaponAnimLayerID::Vert].factor	= cry_fabsf(m_interpVert);
	animInstalls[WeaponAnimLayerID::Horiz].anim		= (m_interpHoriz >= 0.0f) ? WeaponAimAnim::Left : WeaponAimAnim::Right;
	animInstalls[WeaponAnimLayerID::Horiz].factor	= cry_fabsf(m_interpHoriz);
	animInstalls[WeaponAnimLayerID::Front].anim		= m_interpFront > 0 ? WeaponAimAnim::Front : WeaponAimAnim::Back;
	animInstalls[WeaponAnimLayerID::Front].factor	= cry_fabsf(m_interpFront);
	animInstalls[WeaponAnimLayerID::Side].anim		= m_interpSide > 0 ? WeaponAimAnim::StrafeRight : WeaponAimAnim::StrafeLeft;
	animInstalls[WeaponAnimLayerID::Side].factor	= cry_fabsf(m_interpSide);
	animInstalls[WeaponAnimLayerID::Run].anim		= params.isSprinting ? WeaponAimAnim::Sprint : WeaponAimAnim::Run;
	animInstalls[WeaponAnimLayerID::Run].factor		= STAP_MF_All * max(m_runFactor, m_sprintFactor);
	animInstalls[WeaponAnimLayerID::Idle].anim		= WeaponAimAnim::Idle;
	animInstalls[WeaponAnimLayerID::Idle].factor	= haveIdleAnimation ? STAP_MF_All * (1.0f - m_runFactor) * params.overlayFactor : 0.0f;

	//--- Check for pushing a bump animation
	float currentHeight = params.position.z;
	bool aboutToLand = (params.groundDistance < (-params.velocity.z * frameTime * 2.0f));
	if (!params.isOnGround && !aboutToLand)
	{
		m_fallFromHeight = m_falling ? max(m_fallFromHeight, currentHeight) : currentHeight;
	}
	else if (m_falling)
	{
		float fallHeight = (m_fallFromHeight - currentHeight);
		if (fallHeight > MIN_FALL_HEIGHT)
		{
			const float superJumpLandFactor = params.superJump ? 1.5f : 1.0f;
			float fallFactor = (fallHeight - MIN_FALL_HEIGHT) / (MAX_FALL_HEIGHT - MIN_FALL_HEIGHT);
			fallFactor = min(fallFactor * superJumpLandFactor, 1.0f);
			if (params.isZoomed)
			{
				fallFactor *= SCOPE_LAND_FACTOR;
			}

			animInstalls[WeaponAnimLayerID::Bump].anim		= WeaponAimAnim::Bump;
			animInstalls[WeaponAnimLayerID::Bump].factor	= STAP_MF_All * fallFactor;
		}
	}
	m_falling = !params.isOnGround && !aboutToLand;

	//Set super jump idle if we super jumped (idle one is temporary, we might need different assets)
	if (params.superJump && m_falling)
	{
		animInstalls[WeaponAnimLayerID::Idle].anim		= WeaponAimAnim::SuperJump;
		animInstalls[WeaponAnimLayerID::Idle].factor	= 1.0f;
	}
 
	//--- Run through installing animations or updating their blend factors
	CryCharAnimationParams animParams;
	m_firstUpdate = false;

	bool newBase = false;
	for (int i=0; i<WeaponAnimLayerID::Total; i++)
	{
		if (animInstalls[i].anim != WeaponAimAnim::Total)
		{
			float animFactor = m_data.blendFactors[animInstalls[i].anim];
			int animID = params.animID[animInstalls[i].anim];
			
			if(animID < 0)
			{
				if (m_currentAnimID[i] != WeaponAimAnim::Total)
				{
					params.skelAnim->StopAnimationInLayer(m_data.layer + i, 0.1f);
					m_currentAnimID[i] = WeaponAimAnim::Total;
				}
				continue;
			}
			
			bool noAnimsInLayer = (params.characterInst->GetISkeletonAnim()->GetNumAnimsInFIFO(m_data.layer + i) == 0);

			if (!LAYER_SHOULD_LOOP[i] || (animID != m_currentAnimID[i]) || noAnimsInLayer)
			{
				if (m_snapSwitch)
				{
					params.skelAnim->ClearFIFOLayer(m_data.layer+i);
				}

				animParams.m_nLayerID	  = m_data.layer+i;
				animParams.m_fTransTime   = !m_snapSwitch && (newBase || LAYER_SHOULD_BLEND[i]) ? LAYER_BLEND_TIME[i] : 0.0f;
				animParams.m_nFlags		  = LAYER_SHOULD_LOOP[i] ? CA_LOOP_ANIMATION : CA_ALLOW_ANIM_RESTART;
				params.skelAnim->StartAnimationById(animID, animParams);
				m_currentAnimID[i]	  = animID;

				if (i == WeaponAnimLayerID::Base)
				{
					newBase = true;
				}
			}

			params.skelAnim->SetAdditiveWeight(m_data.layer+i, animInstalls[i].factor * animFactor);
		}
	}	

	m_snapSwitch = false;
}
