#include "StdAfx.h"
#include "Game.h"
#include "PlayerMovementController.h"
#include "GameUtils.h"
#include "ITimer.h"
#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "GameCVars.h"
#include "NetInputChainDebug.h"
#include "Item.h"
#include "IPlayerInput.h"

#define ENABLE_NAN_CHECK

#ifdef ENABLE_NAN_CHECK
#define CHECKQNAN_FLT(x) \
	assert(((*(unsigned*)&(x))&0xff000000) != 0xff000000u && (*(unsigned*)&(x) != 0x7fc00000))
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#define CHECKQNAN_VEC(v) \
	CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)

// minimum desired speed accepted
static const float MIN_DESIRED_SPEED = 0.00002f;

// anticipation look ik
static const float ANTICIPATION_COSINE_ANGLE = DEG2RAD(45);
// amount of time to look for
static const CTimeValue LOOK_TIME = 2.0f;

// IDLE Checking stuff from here on
#define DEBUG_IDLE_CHECK
#undef  DEBUG_IDLE_CHECK

static const float IDLE_CHECK_TIME_TO_IDLE = 5.0f;
static const float IDLE_CHECK_MOVEMENT_TIMEOUT = 3.0f;
// ~IDLE Checking stuff

#define lerpf(a, b, l) (a + (l * (b - a)))
static float s_dbg_my_white[4] = {1,1,1,1};

CPlayerMovementController::CPlayerMovementController( CPlayer * pPlayer ) : m_pPlayer(pPlayer), m_animTargetSpeed(-1.0f), m_animTargetSpeedCounter(0)
{
	
	m_lookTarget=Vec3(ZERO);
	m_aimTarget=Vec3(ZERO);
	m_fireTarget=Vec3(ZERO);
	Reset();
	m_lastRotX = 0.0f;
	m_lastRotZ = 0.0f;
	m_timeTurningInSameDirX = 0.0f;
	m_timeTurningInSameDirZ = 0.0f;
}

CPlayerMovementController::~CPlayerMovementController()
{
}

void CPlayerMovementController::Reset()
{
	Construct(m_state);
	m_atTarget = false;
	m_desiredSpeed = 0.0f;
	m_usingAimIK = m_usingLookIK = false;
	m_aimClamped = false;

	m_aimInterpolator.Reset();
	m_lookInterpolator.Reset();
	m_updateFunc = &CPlayerMovementController::UpdateNormal;
	m_targetStance = STANCE_NULL;
	m_strengthJump = false;
	m_idleChecker.Reset(this);

	if(!GetISystem()->IsSerializingFile() == 1)
		UpdateMovementState( m_currentMovementState );

	m_lookTarget=Vec3(ZERO);
	m_aimTarget=Vec3(ZERO);
	m_fireTarget=Vec3(ZERO);

}

inline float filterFloat(float value, float target, float maxChange)
{
	float absChange = fabsf(maxChange);

	if(target > value)
	{
		return min(target, value + absChange);
	}
	else
	{
		return max(target, value - absChange);
	}
}

#if 1
void CPlayerMovementController::ApplyControllerAcceleration( float& xRot, float& zRot, float dt )
{
	float initialRotX = xRot;
	float initialRotZ = zRot;

	if(g_pGameCVars->pl_aim_acceleration_enabled)
	{
		//If we're using the Crytek UK acceleration values

		//This now switches between two response curves as well as scaling turn rates etc
		float controllerSensitivity = clamp(g_pGameCVars->cl_sensitivityController * 0.5f, 0.0f, 1.0f);

		float fractionIncrease = 0.5f + controllerSensitivity; //0.5 to 1.5f

		float newXRot = xRot;
		float newZRot = zRot;

		const float kTimeToFullMultiplier = 0.5f;
		const float kInvTimeToFullMultiplier = 1.0f / kTimeToFullMultiplier;

		float timeAtHighDeflectionX = min((float)__fsel(fabsf(xRot) - 0.95f, m_timeTurningInSameDirX + dt, 0.0f), kTimeToFullMultiplier);
		float timeAtHighDeflectionZ = min((float)__fsel(fabsf(zRot) - 0.95f, m_timeTurningInSameDirZ + dt, 0.0f), kTimeToFullMultiplier);

		float kFullXMultiplier = (g_pGameCVars->controller_full_turn_multiplier_x - 1.0f) * fractionIncrease;
		float kFullZMultiplier = (g_pGameCVars->controller_full_turn_multiplier_z - 1.0f) * fractionIncrease;

		newXRot = newXRot + (newXRot * kFullXMultiplier * timeAtHighDeflectionX * kInvTimeToFullMultiplier);
		newZRot = newZRot + (newZRot * kFullZMultiplier * timeAtHighDeflectionZ * kInvTimeToFullMultiplier);		

		m_timeTurningInSameDirX = timeAtHighDeflectionX;
		m_timeTurningInSameDirZ = timeAtHighDeflectionZ;
		
		xRot = newXRot;
		zRot = newZRot;
	}

	m_lastRotX = xRot;
	m_lastRotZ = zRot;
}
#else
void CPlayerMovementController::ApplyControllerAcceleration( float& xRot, float& zRot, float dt )
{
	float initialRotX = xRot;
	float initialRotZ = zRot;

	if(g_pGameCVars->pl_aim_acceleration_enabled)
	{
		//If we're using the Crytek UK acceleration values

		//This now switches between two response curves as well as scaling turn rates etc
		float controllerSensitivity = clamp(g_pGameCVars->cl_sensitivityController * 0.5f, 0.0f, 1.0f);

		float fractionIncrease = 1.0f + ((controllerSensitivity - 0.5f) * 1.5f); //0.25f to 1.75f
		float invControllerSensitivity = (1.0f - controllerSensitivity);

		float absRotX = fabsf(initialRotX);
		float absLastRotX = fabsf(m_lastRotX);
		float absRotZ = fabsf(initialRotZ);
		float absLastRotZ = fabsf(m_lastRotZ);

		//Rot X handling (Pitch)
		float sign			= 0.0f;
		if(absRotX != 0.0f)
		{
			sign = initialRotX / absRotX;
		}

		float lastSign	= 0.0f;
		if(absLastRotX != 0.0f)
		{
			lastSign = m_lastRotX / absLastRotX;
		}		

		float absTargetRotX = cry_powf(absRotX, g_pGameCVars->ctrlr_exponent);
		float targetRotX = min(absTargetRotX, g_pGameCVars->ctrlr_maxspeed_vertical * fractionIncrease) * sign;

		float linearX = 0.0f;
		float exponentialX = 0.0f;

		float timeInDirX = m_timeTurningInSameDirX;
		float timeInDirZ = m_timeTurningInSameDirZ;

		//Curve 1
		if(absTargetRotX > fabsf(m_lastRotX) && sign == lastSign)
		{
			linearX = filterFloat( m_lastRotX, targetRotX, g_pGameCVars->ctrlr_acceleration_vertical_up * dt * fractionIncrease );
		}
		else
		{
			if(sign != lastSign && absLastRotX != 0.0f)
			{
				linearX = 0.0f;
			}
			else
			{
				linearX = filterFloat( m_lastRotX, targetRotX, g_pGameCVars->ctrlr_acceleration_vertical_down * dt * fractionIncrease );
			}
		}

		//Curve 2
		if(sign == lastSign && absTargetRotX >= fabsf(m_lastRotX) && absTargetRotX > 0.0f)
		{
			//using initialRotX instead of targetRotX so it hasn't been multiplied up/down
			exponentialX = lerpf( m_lastRotX, max(fabsf(initialRotX), absTargetRotX) * sign, min((((1.0f - timeInDirX) * g_pGameCVars->ctrlr_timeaccel_multiplier_vertical) + g_pGameCVars->ctrlr_timeaccel_base_multiplier_vertical) * dt * fractionIncrease, 1.0f));
			timeInDirX = min(timeInDirX + (dt * fractionIncrease / g_pGameCVars->ctrlr_timeaccel_boost_time), 1.0f);
		}
		else
		{
			if(sign != lastSign && absLastRotX != 0.0f)
			{
				exponentialX = 0.0f;
				timeInDirX = 0.0f;
			}
			else
			{
				exponentialX = lerpf(m_lastRotX, targetRotX, min(1.0f, 30.0f * dt));
				timeInDirX = timeInDirX * 0.5f * (1.0f - dt);
			}
		}

		//Blend between Curve 1 and 2
		xRot = (linearX * controllerSensitivity) + (exponentialX * invControllerSensitivity);

		
		//Rot Z handling (Yaw)
		if(absRotZ != 0.0f)
		{
			sign	= initialRotZ / absRotZ;
		}
		else
		{
			sign = 0.0f;
		}

		if(absLastRotZ != 0.0f)
		{
			lastSign	= m_lastRotZ / absLastRotZ;
		}
		else
		{
			lastSign = 0.0f;
		}

		//Rot Z handling (Yaw)
		float absTargetRotZ = cry_powf(absRotZ, g_pGameCVars->ctrlr_exponent);
		float targetRotZ = min(absTargetRotZ, g_pGameCVars->ctrlr_maxspeed_horizontal * fractionIncrease) * sign;


		float linearZ = 0.0f;
		float exponentialZ = 0.0f;

		//Curve 1
		//  Blends to the target value linearly.
		if(absTargetRotZ > fabsf(m_lastRotZ) && sign == lastSign)
		{
			linearZ = filterFloat( m_lastRotZ, targetRotZ, g_pGameCVars->ctrlr_acceleration_horizontal_up * dt * fractionIncrease);
		}
		else
		{
			if(sign != lastSign && absLastRotZ != 0.0f)
			{
				linearZ = 0.0f;
			}
			else
			{
				linearZ = filterFloat( m_lastRotZ, targetRotZ, g_pGameCVars->ctrlr_acceleration_horizontal_down * dt * fractionIncrease);
			}
		}

		//Curve 2
		//  Blends to the target value with a lerp that is boosted significantly towards the start of the turn.
		//  m_timeInDirZ tracks how long the player has been rotating in a given direction
		if(sign == lastSign && absTargetRotZ >= fabsf(m_lastRotZ) && absTargetRotZ > 0.0f)
		{
			//using initialRotZ instead of targetRotZ so it hasn't been multiplied up/down
			exponentialZ = lerpf( m_lastRotZ, max(fabsf(initialRotZ), absTargetRotZ) * sign, min((((1.0f - timeInDirZ) * g_pGameCVars->ctrlr_timeaccel_multiplier_horizontal) + g_pGameCVars->ctrlr_timeaccel_base_multiplier_horizontal) * dt * fractionIncrease, 1.0f));
			timeInDirZ = min(timeInDirZ + (dt * fractionIncrease / g_pGameCVars->ctrlr_timeaccel_boost_time), 1.0f);
		}
		else
		{
			if(sign != lastSign && absLastRotZ != 0.0f)
			{
				exponentialZ = 0.0f;
				timeInDirZ = 0.0f;
			}
			else
			{
				exponentialZ = lerpf(m_lastRotZ, targetRotZ, min(1.0f, 30.0f * dt));
				timeInDirZ = timeInDirZ * 0.75f * (1.0f - dt);
			}
		}

		m_timeTurningInSameDirX = timeInDirX;
		m_timeTurningInSameDirZ = timeInDirZ;

		//blend between Curve 1 and Curve 2
		zRot = (linearZ * controllerSensitivity) + (exponentialZ * invControllerSensitivity);
	}
	
	m_lastRotX = xRot;
	m_lastRotZ = zRot;
}
#endif

bool CPlayerMovementController::RequestMovement( CMovementRequest& request )
{
	if (!m_pPlayer->IsPlayer())
	{
		if (IVehicle* pVehicle = m_pPlayer->GetLinkedVehicle())
		{
			if (IMovementController* pController = pVehicle->GetPassengerMovementController(m_pPlayer->GetEntityId()))
			{
        IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(m_pPlayer->GetEntityId());
        if (!pSeat->IsDriver())
				  pController->RequestMovement(request);
			}
		}

		if (gEnv->bServer)
			CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );
	}	

	// because we're not allowed to "commit" values here, we make a backup,
	// perform modifications, and then copy the modifications make if everything
	// was successful
	CMovementRequest state = m_state;
	// we have to process right through and not early out, because otherwise we
	// won't modify request to a "close but correct" state
	bool ok = true;

	if (request.HasMoveTarget())
	{
		// TODO: check validity of getting to that target
		state.SetMoveTarget( request.GetMoveTarget() );
		m_atTarget = false;

		float distanceToEnd(request.GetDistanceToPathEnd());
		if (distanceToEnd>0.001f)
			state.SetDistanceToPathEnd(distanceToEnd);
	}
	else if (request.RemoveMoveTarget())
	{
		state.ClearMoveTarget();
		state.ClearDesiredSpeed();
		state.ClearDistanceToPathEnd();
	}

	if (request.HasInflectionPoint())
	{
		// NOTE: Having an inflection point indicates the move target will be absolute data.
		// Previously the move target was just (position + velocity.GetNormalized()).

		// This is the estimated position of the next move target after reaching the current move target
		state.SetInflectionPoint(request.GetInflectionPoint());
	}
	else if (request.RemoveInflectionPoint())
	{
		state.ClearInflectionPoint();
	}

	if (request.HasStance())
	{
		state.SetStance( request.GetStance() );
	}
	else if (request.RemoveStance())
	{
		state.ClearStance();
	}

	if (request.HasDesiredSpeed())
	{
		if (!state.HasMoveTarget() && request.GetDesiredSpeed() > 0.0f)
		{
			request.SetDesiredSpeed(0.0f);
			ok = false;
		}
		else
		{
			state.SetDesiredSpeed( request.GetDesiredSpeed() );
		}
	}
	else if (request.RemoveDesiredSpeed())
	{
		state.RemoveDesiredSpeed();
	}

	if (request.HasAimTarget())
	{
		state.SetAimTarget( request.GetAimTarget() );
	}
	else if (request.RemoveAimTarget())
	{
		state.ClearAimTarget();
		//state.SetNoAiming();
	}

	if (request.HasBodyTarget())
	{
		state.SetBodyTarget( request.GetBodyTarget() );
	}
	else if (request.RemoveBodyTarget())
	{
		state.ClearBodyTarget();
		//state.SetNoBodying();
	}

	if (request.HasFireTarget())
	{
		state.SetFireTarget( request.GetFireTarget() );
		//forcing fire target here, can not wait for it to be set in the dateNormalate.
		//if don't do it - first shot of the weapon might be in some undefined direction (ProsessShooting is done right after this 
		//call in AIProxy). This is particularly problem for RPG shooting
		m_currentMovementState.fireTarget = request.GetFireTarget();
		// the weaponPosition is from last update - might be different at this moment, but should not be too much
		m_currentMovementState.fireDirection = (m_currentMovementState.fireTarget - m_currentMovementState.weaponPosition).GetNormalizedSafe();
	}
	else if (request.RemoveFireTarget())
	{
		state.ClearFireTarget();
		//state.SetNoAiming();
	}

	if (request.HasLookTarget())
	{
		// TODO: check against move direction to validate request
		state.SetLookTarget( request.GetLookTarget(), request.GetLookImportance() );
	}
	else if (request.RemoveLookTarget())
	{
		state.ClearLookTarget();
	}

	if (request.HasStance())
	{
		state.SetStance( request.GetStance() );
	}
	else if (request.RemoveStance())
	{
		state.ClearStance();
	}

	if (request.HasLean())
	{
		state.SetLean( request.GetLean() );
	}
	else if (request.RemoveLean())
	{
		state.ClearLean();
	}

	if (request.HasPeekOver())
	{
		state.SetPeekOver( request.GetPeekOver() );
	}
	else if (request.RemovePeekOver())
	{
		state.ClearPeekOver();
	}

	if (request.ShouldJump())
		state.SetJump();

	state.SetAllowStrafing(request.AllowStrafing());

	if (request.HasNoAiming())
		state.SetNoAiming();

	if (request.HasDeltaMovement())
		state.AddDeltaMovement( request.GetDeltaMovement() );
	if (request.HasDeltaRotation())
		state.AddDeltaRotation( request.GetDeltaRotation() );

	if (request.HasPseudoSpeed())
		state.SetPseudoSpeed(request.GetPseudoSpeed());
	else if (request.RemovePseudoSpeed())
		state.ClearPseudoSpeed();

	if (request.HasPrediction())
		state.SetPrediction( request.GetPrediction() );
	else if (request.RemovePrediction())
		state.ClearPrediction();

	state.SetAlertness(request.GetAlertness());

	// commit modifications
	if (ok)
	{
		m_state = state;
	}

//	if (ICharacterInstance * pChar = m_pPlayer->GetEntity()->GetCharacter(0))
//		pChar->GetISkeleton()->SetFuturePathAnalyser( (m_state.HasPathTarget() || m_state.HasMoveTarget())? 1 : 0 );

/*
	if (m_state.HasPathTarget())
	{
		SAnimationTargetRequest req;
		if (m_state.HasDesiredBodyDirectionAtTarget())
		{
			req.direction = m_state.GetDesiredBodyDirectionAtTarget();
			req.directionRadius = DEG2RAD(1.0f);
		}
		else
		{
			req.direction = FORWARD_DIRECTION;
			req.directionRadius = DEG2RAD(180.0f);
		}
		req.position = m_state.GetPathTarget();

		IAnimationSpacialTrigger * pTrigger = m_pPlayer->GetAnimationGraphState()->SetTrigger( req );
		if (m_state.HasDesiredSpeedAtTarget())
			pTrigger->SetInput("DesiredSpeed", m_state.GetDesiredSpeedAtTarget());
		else
			pTrigger->SetInput("DesiredSpeed", 0.0f); // TODO: hack to test
	}
*/

	if (request.HasActorTarget())
	{
		const SActorTargetParams& p = request.GetActorTarget();

		SAnimationTargetRequest req;
		req.position = p.location;
		req.startArcAngle = p.startArcAngle;
		req.startWidth = p.startWidth;
		req.direction = p.direction;
		req.directionTolerance = p.directionTolerance;
		req.prepareRadius = 3.0f;
		req.projectEnd = p.projectEnd;
		req.navSO = p.navSO;

		IAnimationSpacialTrigger * pTrigger = m_pPlayer->GetAnimationGraphState()->SetTrigger(req, p.triggerUser, p.pQueryStart, p.pQueryEnd);
		if (pTrigger)
		{
			if (!p.vehicleName.empty())
			{
				pTrigger->SetInput( "Vehicle", p.vehicleName.c_str() );
				pTrigger->SetInput( "VehicleSeat", p.vehicleSeat );
			}
			if (p.speed >= 0.0f)
			{
				pTrigger->SetInput( m_inputDesiredSpeed, p.speed );
			}
			m_animTargetSpeed = p.speed;
			pTrigger->SetInput( m_inputDesiredTurnAngleZ, 0 );
			if (!p.animation.empty())
			{
				pTrigger->SetInput( p.signalAnimation? "Signal" : "Action", p.animation.c_str() );
			}
			if (p.stance != STANCE_NULL)
			{
				m_targetStance = p.stance;
				pTrigger->SetInput( m_inputStance, m_pPlayer->GetStanceInfo(p.stance)->name );
			}
		}
	}
	else if (request.RemoveActorTarget())
	{
		if(m_pPlayer->GetAnimationGraphState())
			m_pPlayer->GetAnimationGraphState()->ClearTrigger(eAGTU_AI);
	}

	return ok;
}

ILINE static f32 GetYaw( const Vec3& v0, const Vec3& v1 )
{
  float a0 = atan2f(v0.y, v0.x);
  float a1 = atan2f(v1.y, v1.x);
  float a = a1 - a0;
  if (a > gf_PI) a -= gf_PI2;
  else if (a < -gf_PI) a += gf_PI2;
  return a;
}

static float yellow[4] = {1,1,0,1};
static CTimeValue lastTime;
static int y = 100;

void CPlayerMovementController::BindInputs( IAnimationGraphState * pAGState )
{
	if ( pAGState )
	{
		m_inputDesiredSpeed = pAGState->GetInputId("DesiredSpeed");
		m_inputDesiredTurnAngleZ = pAGState->GetInputId("DesiredTurnAngleZ");
		m_inputStance = pAGState->GetInputId("Stance");
		m_inputPseudoSpeed = pAGState->GetInputId("PseudoSpeed");
	}
}

bool CPlayerMovementController::Update( float frameTime, SActorFrameMovementParams& params )
{
	bool ok = false;

	params.lookTarget = Vec3(-1,-1,-1);
	params.aimTarget = Vec3(-1,-1,-1);

/*
	if (m_pPlayer->IsFrozen())
	{
		params.lookTarget = Vec3(-2,-2,-2);
		params.aimTarget = Vec3(-2,-2,-2);

		if (m_state.HasAimTarget())
		{
			Vec3 aimTarget = m_state.GetAimTarget();
			if (!aimTarget.IsValid())
				CryFatalError("");
			m_state.ClearAimTarget();
		}

		if (m_state.HasLookTarget())
		{
			Vec3 lookTarget = m_state.GetLookTarget();
			if (!lookTarget.IsValid())
				CryFatalError("");
			m_state.ClearLookTarget();
		}
	}
*/

	if (m_updateFunc)
	{
		ok = (this->*m_updateFunc)(frameTime, params);
	}


//	m_state.RemoveDeltaMovement();
//	m_state.RemoveDeltaRotation();

	return ok;
}

void CPlayerMovementController::CTargetInterpolator::TargetValue(
	const Vec3& value, 
	const CTimeValue& now,  
	bool updateLastValue )
{
	m_target = value;
	if (updateLastValue)
		m_lastValue = now;
}

void CPlayerMovementController::CTargetInterpolator::Update(float frameTime)
{
	m_pain += frameTime * (m_painDelta - DEG2RAD(10.0f));
	m_pain = CLAMP(m_pain, 0, 2);
}

bool CPlayerMovementController::CTargetInterpolator::GetTarget(
	Vec3& target, 
	Vec3& rotateTarget, 
	const Vec3& playerPos, 
	const Vec3& moveDirection, 
	const Vec3& bodyDirection,
	const Vec3& entityDirection,
	float maxAngle, 
	float distToEnd, 
	float viewFollowMovement, 
	ColorB * clr,
	const char ** bodyTargetType )
{
  bool retVal = true;
	Vec3 desiredTarget = GetTarget();
	Vec3 desiredDirection = desiredTarget - playerPos;
	float	targetDist = desiredDirection.GetLength();

	// if the target gets very close, then move it to the current body direction (failsafe)
	float desDirLenSq = desiredDirection.GetLengthSquared2D();
	if (desDirLenSq < sqr(0.2f))
	{
		float blend = min(1.0f, 5.0f * (0.2f - sqrtf(desDirLenSq)));
		desiredDirection = LERP(desiredDirection, entityDirection, blend);
	}
	desiredDirection.NormalizeSafe(entityDirection);

	Vec3 clampedDirection = desiredDirection;

	// clamp yaw rotation into acceptable bounds
	float yawFromBodyToDesired = GetYaw( bodyDirection, desiredDirection );
	if (fabsf(yawFromBodyToDesired) > maxAngle)
	{
		float clampAngle = (yawFromBodyToDesired < 0) ? -maxAngle : maxAngle;
		clampedDirection = Quat::CreateRotationZ(clampAngle) * bodyDirection;
		clampedDirection.z = desiredDirection.z;
		clampedDirection.Normalize();

		if (viewFollowMovement > 0.1f)
		{
			clampedDirection = Quat::CreateSlerp( 
			Quat::CreateIdentity(), 
			Quat::CreateRotationV0V1(clampedDirection, moveDirection), 
			min(viewFollowMovement, 1.0f)) * clampedDirection;
      // return false to indicate that the direction is out of range. This stops the gun
      // being held up in the aiming pose, but not pointing at the target, whilst running
      retVal = false;
		}
		// Clamped, rotate the body until it is facing towards the target.
	}

	if(viewFollowMovement > 0.5f)
	{
		// Rotate the body towards the movement direction while moving and strafing is not specified.
		rotateTarget = playerPos + 5.0f * moveDirection.GetNormalizedSafe();
		*bodyTargetType = "ik-movedir";
	}
	else
	{
		rotateTarget = desiredTarget;
		*bodyTargetType = "ik-target";
	}

	// Return the safe aim/look direction.
	target = clampedDirection * targetDist + playerPos;

	return retVal;
}

static void DebugDrawWireFOVCone(IRenderer* pRend, const Vec3& pos, const Vec3& dir, float rad, float fov, const ColorB& col)
{
	const unsigned npts = 32;
	const unsigned npts2 = 16;
	Vec3	points[npts];
	Vec3	pointsx[npts2];
	Vec3	pointsy[npts2];

	Matrix33	base;
	base.SetRotationVDir(dir);

	fov *= 0.5f;

	float coneRadius = sinf(fov) * rad;
	float coneHeight = cosf(fov) * rad;

	for(unsigned i = 0; i < npts; i++)
	{
		float	a = ((float)i / (float)npts) * gf_PI2;
		float rx = cosf(a) * coneRadius;
		float ry = sinf(a) * coneRadius;
		points[i] = pos + base.TransformVector(Vec3(rx, coneHeight, ry));
	}

	for(unsigned i = 0; i < npts2; i++)
	{
		float	a = -fov + ((float)i / (float)(npts2-1)) * (fov*2);
		float rx = sinf(a) * rad;
		float ry = cosf(a) * rad;
		pointsx[i] = pos + base.TransformVector(Vec3(rx, ry, 0));
		pointsy[i] = pos + base.TransformVector(Vec3(0, ry, rx));
	}

	pRend->GetIRenderAuxGeom()->DrawPolyline(points, npts, true, col);
	pRend->GetIRenderAuxGeom()->DrawPolyline(pointsx, npts2, false, col);
	pRend->GetIRenderAuxGeom()->DrawPolyline(pointsy, npts2, false, col);

	pRend->GetIRenderAuxGeom()->DrawLine(points[0], col, pos, col);
	pRend->GetIRenderAuxGeom()->DrawLine(points[npts/4], col, pos, col);
	pRend->GetIRenderAuxGeom()->DrawLine(points[npts/2], col, pos, col);
	pRend->GetIRenderAuxGeom()->DrawLine(points[npts/2+npts/4], col, pos, col);
}


static bool ClampTargetPointToCone(Vec3& target, const Vec3& pos, const Vec3& dir, float coneAngle)
{
	Vec3	reqDir = target - pos;
	float dist = reqDir.NormalizeSafe();
	if (dist < 0.01f)
		return false;

	// Slerp
	float cosAngle = dir.Dot(reqDir);

	coneAngle *= 0.5f;

	const float thr = cosf(coneAngle);

	if (cosAngle >= thr)
		return false;

	// Target is outside the cone
	float targetAngle = acos_tpl(cosAngle);
	if (targetAngle < FLT_EPSILON)
	{
		// The angle between the requested direction and the current direction
		// is so small we don't even have precision to calculate with it.
		// Therefore, we can assume that we are close enough, and we don't need to clamp.
		return false;
	}

	float t = coneAngle / targetAngle;

	Quat	cur;
	cur.SetRotationVDir(dir);
	Quat	req;
	req.SetRotationVDir(reqDir);

	Quat	view;
	view.SetSlerp(cur, req, t);

	target = pos + view.GetColumn1() * dist;

	return true;
}


static bool ClampBodySoTargetIsClampedToCone(Vec3& bodyTarget, Vec3& target, const Vec3& pos, const Vec3& dir, float coneAngle)
{
	Vec3	reqDir = target - pos;
	float dist = reqDir.NormalizeSafe();
	if (dist < 0.01f)
		return false;

	// Slerp
	float cosAngle = dir.Dot(reqDir);

	coneAngle *= 0.5f;

	const float thr = cosf(coneAngle);

	if (cosAngle >= thr)
		return false;

	// Target is outside the cone, clamp it to the cone (we cannot assume we reach our new body target this frame, so just clamp to current direction)
	float targetAngle = acos_tpl(cosAngle);
	if (targetAngle < FLT_EPSILON)
	{
		// The angle between the requested direction and the current direction
		// is so small we don't even have precision to calculate with it.
		// Therefore, we can assume that we are close enough, and we don't need to clamp.
		return false;
	}

	float t = coneAngle / targetAngle;

	Quat	cur;
	cur.SetRotationVDir(dir);
	Quat	req;
	req.SetRotationVDir(reqDir);

	Quat	view;
	view.SetSlerp(cur, req, t);

	target = pos + view.GetColumn1() * dist;

	// Rotate body so the target is reached
	t = (targetAngle - coneAngle)/targetAngle;
	t = min(t, 1.0f);
	view.SetSlerp(cur, req, t);
	bodyTarget = pos + view.GetColumn1() * dist;

	return true;
}



bool CPlayerMovementController::UpdateNormal( float frameTime, SActorFrameMovementParams& params )
{
	// TODO: Don't update all this mess when a character is dead.

	ITimer * pTimer = gEnv->pTimer;

#if !defined(_RELEASE)
	IRenderer * pRend = gEnv->pRenderer;
	if (g_pGame->GetCVars()->g_debugaimlook)
	{
		if (pTimer->GetFrameStartTime()!=lastTime)
		{
			y = 100;
			lastTime = pTimer->GetFrameStartTime();
		}
	}
#endif

	IEntity * pEntity = m_pPlayer->GetEntity();
	Vec3 playerPos = pEntity->GetWorldPos();

	params.desiredVelocity.Set(0.0f, 0.0f, 0.0f);
	params.lookTarget = m_currentMovementState.eyePosition + m_currentMovementState.eyeDirection * 10.0f;
	
	if(m_state.HasAimTarget())
		params.aimTarget = m_state.GetAimTarget();
	else
		params.aimTarget = m_currentMovementState.weaponPosition + m_currentMovementState.aimDirection * 10.0f;

	params.lookIK = false;
	params.aimIK = false;
	params.deltaAngles.Set(0.0f, 0.0f, 0.0f);

	static bool forceStrafe = false;	// [mikko] For testing, should be 'false'.
	params.allowStrafe = m_state.AllowStrafing() || forceStrafe;

	if (m_state.HasStance() && m_state.GetStance() == STANCE_CROUCH)
	{
		if ((m_pPlayer->GetStance() != STANCE_CROUCH) && (m_pPlayer->GetActorStats()->inAir > 0.01f))
		{
			m_state.SetStance(m_pPlayer->GetStance());
		}
	}

	if (m_state.AlertnessChanged())
	{
		if (m_state.GetAlertness() == 2)
		{
			if (!m_state.HasStance() || m_state.GetStance() == STANCE_RELAXED)
				m_state.SetStance(STANCE_STAND);
		}
	}

	CTimeValue now = pTimer->GetFrameStartTime();

	const Vec3 currentBodyDirection(pEntity->GetRotation().GetColumn1());
	Vec3 moveDirection(currentBodyDirection);
	if (m_pPlayer->GetLastRequestedVelocity().len2() > 0.01f)
		moveDirection = m_pPlayer->GetLastRequestedVelocity().GetNormalized();

	// potentially override our normal targets due to a targeted animation request
	bool hasMoveTarget = false;
	bool hasBodyTarget = false;
	Vec3 moveTarget(ZERO);
	Vec3 bodyTarget(ZERO);

	const char * bodyTargetType = "none";
	const char * moveTargetType = "none";
	if (m_state.HasMoveTarget() && m_pPlayer->CanMove())
	{
		hasMoveTarget = true;
		moveTarget = m_state.GetMoveTarget();
		moveTargetType = "steering";
	}
	const SAnimationTarget * pAnimTarget = m_pPlayer->GetAnimationGraphState()->GetAnimationTarget();
	
	if (pAnimTarget != NULL)		
	{
		if( pAnimTarget->preparing)
		{

			float distanceToTarget = (pAnimTarget->position - playerPos).GetLength2D();

			Vec3 animTargetFwdDir = pAnimTarget->orientation.GetColumn1();

			Vec3 moveDirection2d = hasMoveTarget ? moveDirection : (pAnimTarget->position - playerPos);
			moveDirection2d.z = 0.0f;
			moveDirection2d.NormalizeSafe(animTargetFwdDir);

			float fraction = (distanceToTarget - 0.2f) / 1.8f;
			float blend = clamp(fraction, 0.0f, 1.0f);
			animTargetFwdDir.SetSlerp(animTargetFwdDir, moveDirection2d, blend );

			float animTargetFwdDepthFraction = max(0.0f, animTargetFwdDir.Dot(moveDirection2d));
			float animTargetFwdDepth = LERP(1.0f, 3.0f, animTargetFwdDepthFraction);

			bodyTarget = pAnimTarget->position + animTargetFwdDepth * animTargetFwdDir;
			bodyTargetType = "animation";
			hasBodyTarget = true;
			params.allowStrafe = true;

			if (!hasMoveTarget || pAnimTarget->notAiControlledAnymore || (distanceToTarget < 1.0f))
			{
				moveTarget = pAnimTarget->position;
				moveTargetType = "animation";
				hasMoveTarget = true;
				pAnimTarget->notAiControlledAnymore = true;
			}
		}

		const float kCriticalDistanceSqr = sqr(5.0f);
		float distance = (pAnimTarget->position - playerPos).GetLengthSquared();
		
		// HACK: DistanceToPathEnd is bogus values. Using true distance to anim target pos instead.
		pAnimTarget->allowActivation = sqr(m_state.GetDistanceToPathEnd()) < kCriticalDistanceSqr && distance < kCriticalDistanceSqr;
	}
	

	m_animTargetSpeedCounter -= m_animTargetSpeedCounter!=0;

 	// speed control
	if (hasMoveTarget)
	{
		Vec3 desiredMovement = moveTarget - playerPos;

		// should do the rotation thing?
		desiredMovement.z = 0;

		float distance = desiredMovement.len();

		if (distance > 0.01f) // need to have somewhere to move, to actually move :)
		{
			desiredMovement /= distance;
			float desiredSpeed = 1.0f;
			
			if (m_state.HasDesiredSpeed())
				desiredSpeed = m_state.GetDesiredSpeed();

			if (pAnimTarget && pAnimTarget->preparing)  
			{
				desiredSpeed = std::max(desiredSpeed, m_desiredSpeed);
				desiredSpeed = std::max(1.0f, desiredSpeed);

				float approachSpeed = frameTime == 0.0f ? 0.0f : (distance * __fres(frameTime)) * 0.4f;
				desiredSpeed = std::min(desiredSpeed, approachSpeed);
			}

			if (pAnimTarget && (pAnimTarget->activated || m_animTargetSpeedCounter) && m_animTargetSpeed >= 0.0f)
			{
				desiredSpeed = m_animTargetSpeed;
        if (pAnimTarget->activated)
  				m_animTargetSpeedCounter = 4;
			}

			float stanceSpeed=m_pPlayer->GetStanceMaxSpeed(m_pPlayer->GetStance());
			NETINPUT_TRACE(m_pPlayer->GetEntityId(), desiredSpeed);
			NETINPUT_TRACE(m_pPlayer->GetEntityId(), stanceSpeed);
			NETINPUT_TRACE(m_pPlayer->GetEntityId(), desiredMovement);
			if ((desiredSpeed > MIN_DESIRED_SPEED) && stanceSpeed>0.001f)
			{
				//calculate the desired speed amount (0-1 length) in world space
				params.desiredVelocity = desiredMovement * desiredSpeed / stanceSpeed;
				
				//and now, after used it for the viewFollowMovement convert it to the Actor local space
				moveDirection = params.desiredVelocity.GetNormalizedSafe(ZERO);
				params.desiredVelocity = m_pPlayer->GetEntity()->GetWorldRotation().GetInverted() * params.desiredVelocity;
			}
		}
	}

	Vec3 vForcedLookDir(ZERO);
	const bool bHasForcedLookDir = (m_pPlayer && m_pPlayer->GetForcedLookDir(vForcedLookDir));

	if (!hasBodyTarget)
	{
		if (bHasForcedLookDir)
		{
			bodyTarget = playerPos + vForcedLookDir;
			bodyTargetType = "forcedLook";
		}
		else if (m_state.HasBodyTarget())
		{
			bodyTarget = m_state.GetBodyTarget();
			bodyTargetType = "requested";
		}
		else if (hasMoveTarget)
		{
			bodyTarget = playerPos + moveDirection;
			bodyTargetType = "movement";
		}
		else
		{
			bodyTarget = playerPos + currentBodyDirection;
			bodyTargetType = "current";
		}
	}

	// look and aim direction
	Vec3 tgt(ZERO);

	Vec3 eyePosition(playerPos.x, playerPos.y, m_currentMovementState.eyePosition.z);

	bool hasLookTarget = false;
	tgt = eyePosition + 5.0f * currentBodyDirection;
	const char * lookType = "current";
	if (bHasForcedLookDir)
	{
		hasLookTarget = true;
		tgt = eyePosition + 5.0f * vForcedLookDir;
		params.allowStrafe = true;
		lookType = "forcedLook";
	}
	else if (pAnimTarget && pAnimTarget->preparing)
	{
		hasLookTarget = false;
		tgt = pAnimTarget->position + 3.0f * (pAnimTarget->orientation * FORWARD_DIRECTION) + Vec3(0, 0, 1.5);
		lookType = "exactpositioning";
	}
	else if (pAnimTarget && pAnimTarget->activated)
	{
		hasLookTarget = false;
		tgt = eyePosition + 5.0f * currentBodyDirection;
		lookType = "animation";
	}
	else if (m_state.HasLookTarget())
	{
		hasLookTarget = true;
		tgt = m_state.GetLookTarget();
		lookType = "look";
	}
	else if (m_state.HasAimTarget())
	{
		hasLookTarget = true;
		tgt = m_state.GetAimTarget();
		lookType = "aim";

		/*		if (m_state.HasFireTarget() && m_state.GetFireTarget().Dot(m_state.GetAimTarget()) < cry_cosf(MAX_FIRE_TARGET_DELTA_ANGLE))
		{
		tgt = m_state.GetFireTarget();
		lookType = "fire";
		}*/
	}
	else if (hasMoveTarget)
	{
		Vec3 desiredMoveDir = (moveTarget - playerPos).GetNormalizedSafe(ZERO);
		if (moveDirection.Dot(desiredMoveDir) < ANTICIPATION_COSINE_ANGLE)
		{
			hasLookTarget = true;
			tgt = moveTarget;
			lookType = "anticipate";
		}
	}
	else if (m_lookInterpolator.HasTarget(now, LOOK_TIME))
	{
		tgt = m_lookInterpolator.GetTarget();
		lookType = "lastLook";
	}
//	AdjustForMovement( tgt, moveTarget, playerPos, viewFollowMovement );

	Vec3 lookTarget = tgt;

	if (!bHasForcedLookDir)
		m_lookInterpolator.TargetValue( tgt, now, hasLookTarget );

	bool hasAimTarget = false;
	const char * aimType = lookType;
	if (pAnimTarget && pAnimTarget->preparing)
	{
		hasAimTarget = true;
		tgt = pAnimTarget->position + 3.0f * (pAnimTarget->orientation * FORWARD_DIRECTION) + Vec3(0, 0, 1.5);
		aimType = "animation";
	}
	else if (m_state.HasAimTarget())
	{
		hasAimTarget = true;
		tgt = m_state.GetAimTarget();
		aimType = "aim";
//		AdjustForMovement( tgt, moveTarget, playerPos, viewFollowMovement );
	}

	Vec3 aimTarget = tgt;

	//if ((pAnimTarget == NULL) || (!pAnimTarget->activated)) // (Dejan was not sure about this yet, for leaning and such stuff.)
		m_aimInterpolator.TargetValue( tgt, now, hasAimTarget );

	const char * ikType = "none";
	//if (!m_state.HasNoAiming() && m_aimInterpolator.HasTarget( now, AIM_TIME ))	// AIM IK

	bool hasControl = !pAnimTarget || !pAnimTarget->activated && !pAnimTarget->preparing;

#if ENABLE_MINDCONTROL
	hasControl = hasControl && (gEnv->bMultiplayer || !pEntity->GetAI() || pEntity->GetAI()->IsEnabled() || !m_pPlayer->GetMindControlSlave() || m_pPlayer->GetMindControlSlave()->IsBusy());
#else
	hasControl = hasControl && (gEnv->bMultiplayer || !pEntity->GetAI() || pEntity->GetAI()->IsEnabled());
#endif

	m_aimClamped = false;

	if (!m_pPlayer->IsClient() && hasControl)
	{
		const SActorParams * pActorParams = m_pPlayer->GetActorParams();
		
		if (((SPlayerStats*)m_pPlayer->GetActorStats())->mountedWeaponID)
		{
			if (hasAimTarget)
				params.aimTarget = aimTarget;
			else
				params.aimTarget = lookTarget;

			// Luciano - prevent snapping of aim direction out of existing horizontal angle limits
			float limitH = pActorParams->vLimitRangeH;
			IEntity *pWeaponEntity = gEnv->pEntitySystem->GetEntity(m_pPlayer->GetActorStats()->mountedWeaponID);

			// m_currentMovementState.weaponPosition is offset to the real weapon position
			Vec3 weaponPosition (pWeaponEntity ? pWeaponEntity->GetWorldPos() : m_currentMovementState.weaponPosition); 
			
			if(limitH > 0 && limitH < gf_PI)
			{
				Vec3 aimDirection(params.aimTarget - weaponPosition);//m_currentMovementState.weaponPosition);
				Vec3 limitAxisY(pActorParams->vLimitDir);
				limitAxisY.z = 0;
				Vec3 limitAxisX(limitAxisY.y, -limitAxisY.x, 0);
				limitAxisX.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);
				limitAxisY.NormalizeSafe(Vec3Constants<float>::fVec3_OneY);
				float x = limitAxisX.Dot(aimDirection);
				float y = limitAxisY.Dot(aimDirection);
				float len = cry_sqrtf(sqr(x) + sqr(y));
				len = max(len,2.f);// bring the aimTarget away for degenerated cases when it's between firepos and weaponpos
				float angle = cry_atan2f(x, y);
				if (angle < -limitH || angle > limitH)
				{
					Limit(angle, -limitH, limitH);
					x = sinf(angle) * len;
					y = cosf(angle) * len;
					params.aimTarget = weaponPosition + x* limitAxisX + y * limitAxisY + Vec3(0,0, aimDirection.z);
					m_aimClamped = true;
				}

			}

			float limitUp = pActorParams->vLimitRangeVUp;
			float limitDown = pActorParams->vLimitRangeVDown;
			if(limitUp!=0 || limitDown != 0)
			{
				Vec3 aimDirection(params.aimTarget - weaponPosition);//m_currentMovementState.weaponPosition);
				Vec3 limitAxisXY(aimDirection);
				limitAxisXY.z = 0;
				Vec3 limitAxisZ(Vec3Constants<float>::fVec3_OneZ);

				limitAxisXY.NormalizeSafe(Vec3Constants<float>::fVec3_OneY);
				float z = limitAxisZ.Dot(aimDirection);
				float x = limitAxisXY.Dot(aimDirection);
				float len = cry_sqrtf(sqr(z) + sqr(x));
				len = max(len,2.f); // bring the aimTarget away for degenerated cases when it's between firepos and weaponpos
				float angle = cry_atan2f(z, x);
				if (angle < limitDown && limitDown!=0 || angle > limitUp && limitUp !=0)
				{
					Limit(angle, limitDown, limitUp);
					z = sinf(angle) * len;
					x = cosf(angle) * len;
					params.aimTarget = weaponPosition + z* limitAxisZ + x * limitAxisXY;
					m_aimClamped = true;
				}
			}
			params.aimIK = true;
			params.lookIK = true;

			lookType = aimType = bodyTargetType = "mountedweapon";
		}
		else
		{
			Vec3 limitDirection = currentBodyDirection;
			Vec3 weaponPos(playerPos.x, playerPos.y, m_currentMovementState.weaponPosition.z);

			float lookConeFOV = m_pPlayer->GetLookFOV(pActorParams);

			float aimConeFOV = pActorParams->aimFOVRadians;

			CRY_ASSERT(!m_pPlayer->IsPlayer() || (aimConeFOV == gf_PI));

			if (g_pGame->GetCVars()->g_debugaimlook)
			{
				DebugDrawWireFOVCone(gEnv->pRenderer, weaponPos, limitDirection, 2.0f, aimConeFOV, ColorB(255,255,255));
				DebugDrawWireFOVCone(gEnv->pRenderer, eyePosition, limitDirection, 3.0f, lookConeFOV, ColorB(0,196,255));
			}

			if (hasLookTarget)
			{
				params.lookTarget = lookTarget;

				// If we have both a look target and an aim target and they are too far apart,
				// clamp the look target. Aiming is always more important.
				if (hasAimTarget)
				{
					float maxlookAimAngle = pActorParams->maxLookAimAngleRadians;
					Vec3 dir = (aimTarget - playerPos).GetNormalized();
					ClampTargetPointToCone(params.lookTarget, weaponPos, dir, maxlookAimAngle * 2.0f);
				}

				// When we have a look target, always use look IK. Also when the look direction had to be clamped.
				ikType = "look";
				params.lookIK = true;

				if (!hasMoveTarget)
				{
					bool lookClamped = ClampTargetPointToCone(params.lookTarget, eyePosition, limitDirection, lookConeFOV);

					if (lookClamped)
					{
						bodyTarget = params.lookTarget;
						bodyTargetType = "lookIdleTurn";
					}
				}
				else
				{
					Vec3 newBodyTarget(ZERO);
					bool lookClamped = ClampBodySoTargetIsClampedToCone(newBodyTarget, params.lookTarget, eyePosition, limitDirection, lookConeFOV);

					if (lookClamped)
					{
						// We can't move our head further, but we can turn our body.
						// Turn only when:
						// * Moving, but strafing is allowed and we don't have a fixed body target
						bool bAllowLookTurn = !m_state.HasBodyTarget() && params.allowStrafe;
						if (bAllowLookTurn)
						{
							bodyTarget = newBodyTarget;
							bodyTargetType = "lookStrafe";
						}
					}
				}

				// As a further refinement we can turn our whole body towards the look target
				// when we are staring to the side for a while
				// if (some fancy time-based condition && bAllowLookTurn)
				// {
				//	bodyTarget = params.lookTarget;
				//	bodyTargetType = "look-fullturn";
				// }
			}


			bool swimming = (m_pPlayer->GetStance() == STANCE_SWIM);

			if (hasAimTarget && !swimming)
			{
				params.aimTarget = aimTarget;

				// If we have both a body target and aim target and they are too far apart,
				// disable the body target. Aiming is always more important.
				if (m_state.HasBodyTarget())
				{
					Vec3 dir = (bodyTarget - playerPos).GetNormalized();
					Vec3 aimTarget = params.aimTarget;
					bool clamped = ClampTargetPointToCone(aimTarget, weaponPos, dir, aimConeFOV);
					if (clamped)
					{
							bodyTarget = playerPos + currentBodyDirection;
							bodyTargetType = "current";
					}
				}

				// When we have an aim target, always use aim IK. Also when the aim direction had to be clamped.
				ikType = "aim";
				params.aimIK = true;

				if (!hasMoveTarget)
				{
					m_aimClamped = ClampTargetPointToCone(params.aimTarget, weaponPos, limitDirection, aimConeFOV);

					if (m_aimClamped)
					{
						bodyTarget = params.aimTarget;
						bodyTargetType = "aimIdleTurn";
					}
				}
				else
				{
					Vec3 newBodyTarget(ZERO);
					m_aimClamped = ClampBodySoTargetIsClampedToCone(newBodyTarget, params.aimTarget, weaponPos, limitDirection, aimConeFOV);

					if (m_aimClamped)
					{
						// We can't move our aim further, but we can turn our body.
						// Turn only when strafing is allowed.
						bool bAllowAimTurn = params.allowStrafe;
						if (bAllowAimTurn)
						{
							bodyTarget = newBodyTarget;
							bodyTargetType = "aimStrafe";
						}
					}
				}

				// As a further refinement we can turn our whole body towards the aim target
				// when we are aiming to the side for a while
				// if (some fancy time-based condition && bAllowAimTurn)
				// {
				// 	bodyTarget = params.aimTarget;
				// 	bodyTargetType = "aim-fullturn";
				// }
			}
		
/*
		if (!m_state.HasNoAiming() && m_aimInterpolator.HasTarget( now, AIM_TIME ) && !swimming)
		{
			params.aimIK = m_aimInterpolator.GetTarget( 
				params.aimTarget, 
				bodyTarget, 
				Vec3( playerPos.x, playerPos.y, m_currentMovementState.weaponPosition.z ),
				moveDirection, 
				animBodyDirection, 
				entDirection,
				MAX_AIM_TURN_ANGLE, 
				m_state.GetDistanceToPathEnd(), 
				viewFollowMovement, 
				pDbgClr,
				&bodyTargetType );
			ikType = "aim";
		}
		else if (m_lookInterpolator.HasTarget( now, LOOK_TIME ))	// Look IK
		{
			params.lookIK = m_lookInterpolator.GetTarget( 
				params.lookTarget, 
				bodyTarget, 
				Vec3( playerPos.x, playerPos.y, m_currentMovementState.eyePosition.z ),
				moveDirection, 
				animBodyDirection, 
				entDirection,
				MAX_HEAD_TURN_ANGLE, 
				m_state.GetDistanceToPathEnd(), 
				viewFollowMovement, 
				pDbgClr,
				&bodyTargetType );
			ikType = "look";
		}
*/
		}	
	}


	Vec3 viewDir = ((bodyTarget - playerPos).GetNormalizedSafe(ZERO));
	if (!m_pPlayer->IsClient() && viewDir.len2() > 0.01f)
	{
		//Vec3 localVDir(m_pPlayer->GetEntity()->GetWorldRotation().GetInverted() * viewDir);
		Vec3 localVDir(m_pPlayer->GetViewQuat().GetInverted() * viewDir);
		
		CHECKQNAN_VEC(localVDir);

		if ((pAnimTarget == NULL) || (!pAnimTarget->activated))
		{
			params.deltaAngles.x += asinf(clamp(localVDir.z,-1,1));
			params.deltaAngles.z += cry_atan2f(-localVDir.x,localVDir.y);
		}

		CHECKQNAN_VEC(params.deltaAngles);

		const float maxDeltaAngleRateNormal = DEG2RAD(180.0f);
		const float maxDeltaAngleRateAnimTarget = DEG2RAD(360.0f);
		const float maxDeltaAngleMultiplayer = DEG2RAD(3600.0f);

		float maxDeltaAngleRate = maxDeltaAngleRateNormal;

		if (gEnv->bMultiplayer)
		{
			maxDeltaAngleRate = maxDeltaAngleMultiplayer;
		}
		else
		{
			// Less clamping when approaching animation target.
			if (pAnimTarget && pAnimTarget->preparing)
			{
				float	distanceFraction = clamp(pAnimTarget->position.GetDistance(playerPos) * 2.0f, 0.0f, 1.0f);
				float	animTargetTurnRateFraction = 1.0f - distanceFraction;
				animTargetTurnRateFraction = 0.0f;
				maxDeltaAngleRate = LERP(maxDeltaAngleRate, maxDeltaAngleRateAnimTarget, animTargetTurnRateFraction);
			}
		}

		for (int i=0; i<3; i++)
			Limit(params.deltaAngles[i], -maxDeltaAngleRate * frameTime, maxDeltaAngleRate * frameTime);

		CHECKQNAN_VEC(params.deltaAngles);
	}

	if (m_state.HasDeltaRotation())
	{
		params.deltaAngles += m_state.GetDeltaRotation();
		CHECKQNAN_VEC(params.deltaAngles);
		ikType = "mouse";
	}

#if !defined(_RELEASE)
	if (g_pGame->GetCVars()->g_debugaimlook)
	{
		pRend->Draw2dLabel( 10.f, (float)y, 1.5f, s_dbg_my_white, false, 
			"%s:  body=%s   look=%s   aim=%s   rotik=%s   move=%s   delta ang=(%3.2f, %3.2f, %3.2f)", 
			pEntity->GetName(), bodyTargetType, aimType, lookType, ikType, moveTargetType, 
			params.deltaAngles.x, params.deltaAngles.y, params.deltaAngles.z );
		y += 15;
		if (m_state.GetDistanceToPathEnd() >= 0.0f)
		{
			pRend->Draw2dLabel( 10.f, (float)y, 1.5f, yellow, false, "distanceToEnd: %f (%f)", m_state.GetDistanceToPathEnd(), moveTarget.GetDistance(playerPos) );
			y += 15;
		}

		if (m_state.HasAimTarget())
		{
			pRend->GetIRenderAuxGeom()->DrawLine( m_currentMovementState.weaponPosition, ColorF(1,1,1,1), params.aimTarget+Vec3(0,0,0.05f), ColorF(1,1,1,1), 3.0f);
			pRend->GetIRenderAuxGeom()->DrawLine( m_currentMovementState.weaponPosition, ColorF(1,1,1,0.3f), m_state.GetAimTarget(), ColorF(1,1,1,0.3f));
		}
	}
#endif

	// process incremental movement
	if (m_state.HasDeltaMovement())
	{
		params.desiredVelocity += m_state.GetDeltaMovement();// NOTE: desiredVelocity is normalized to the maximumspeed for the specifiedstance, so DeltaMovement should be between 0 and 1!
	}

	// stance control
	if (pAnimTarget && pAnimTarget->activated && m_targetStance != STANCE_NULL)
	{
		m_state.SetStance( m_targetStance );
	}

	if (m_state.HasStance())
	{
		params.stance = m_state.GetStance();
	}

	// leaning
	if (m_state.HasLean())
		params.desiredLean = m_state.GetLean();
	else
		params.desiredLean = 0.0f;

	// peeking
	if (m_state.HasPeekOver())
		params.desiredPeekOver = m_state.GetPeekOver();
	else
		params.desiredPeekOver = 0.0f;

	params.jump = m_state.ShouldJump();
	m_state.ClearJump();

	params.strengthJump=ShouldStrengthJump();
	ClearStrengthJump();

	// TODO: This should probably be calculate BEFORE it is used (above), or the previous frame's value is used.
	m_desiredSpeed = params.desiredVelocity.GetLength() * m_pPlayer->GetStanceMaxSpeed( m_pPlayer->GetStance() );

	m_state.RemoveDeltaRotation();
	m_state.RemoveDeltaMovement();

	if (params.aimIK)
	{
		m_aimTarget = params.aimTarget;
		if (!params.lookIK)
		{
			// if aiming force looking as well
			// In spite of what it may look like with eye/look IK, the look/aim target tends
			// to be right on/in the target's head so doesn't need any extra offset
			params.lookTarget = params.aimTarget;
			params.lookIK = true;
		}
		m_lookTarget = params.lookTarget;
	}
	else
	{
		m_aimTarget = m_lookTarget;
		if (params.lookIK)
			m_lookTarget = params.lookTarget;
		else
			m_lookTarget = m_currentMovementState.eyePosition + m_pPlayer->GetEntity()->GetRotation() * FORWARD_DIRECTION * 10.0f;
	}

	m_usingAimIK = params.aimIK;
	m_usingLookIK = params.lookIK;

	if (m_state.HasFireTarget())
	{
		m_fireTarget = m_state.GetFireTarget();
	}

	m_aimInterpolator.Update(frameTime);
	m_lookInterpolator.Update(frameTime);

	float pseudoSpeed = 0.0f;
	
	if (m_state.HasPseudoSpeed())
		pseudoSpeed = m_state.GetPseudoSpeed();
	
	if (pAnimTarget && pAnimTarget->preparing && pseudoSpeed < 0.4f)
		pseudoSpeed = 0.4f;
	
	if (params.stance == STANCE_CROUCH && pseudoSpeed > 0.4f && m_pPlayer->IsPlayer())
		pseudoSpeed = 0.4f;

	m_pPlayer->GetAnimationGraphState()->SetInput(m_inputPseudoSpeed, pseudoSpeed);

	bool hasPrediction = m_state.HasPrediction() && (m_state.GetPrediction().nStates > 0);
	bool hasAnimTarget = (pAnimTarget != NULL) && (pAnimTarget->activated || pAnimTarget->preparing);
	if (hasPrediction && !hasAnimTarget)
	{
		params.prediction = m_state.GetPrediction();
	}
	else
	{
    params.prediction.nStates = 0;
	 }

/*
#ifdef USER_dejan
	// Debug Render & Text
	{
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(moveTarget, 0.2f, ColorB(128,255,0, 255), true);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(playerPos, ColorB(255,255,255, 128), moveTarget, ColorB(128,255,0, 128), 0.4f);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(bodyTarget, 0.2f, ColorB(255,128,0, 128), true);
		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(moveTarget, ColorB(255,255,255, 128), bodyTarget, ColorB(255,128,0, 128), 0.2f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(playerPos, ColorB(255,255,255, 128), bodyTarget, ColorB(255,128,0, 128), 0.4f);
	}
#endif
*/

	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.desiredVelocity);
	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.desiredLean);
	NETINPUT_TRACE(m_pPlayer->GetEntityId(), Vec3(params.deltaAngles));
	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.sprint);

	return true;
}

void CPlayerMovementController::UpdateMovementState( SMovementState& state )
{
	IEntity * pEntity = m_pPlayer->GetEntity();
	ICharacterInstance * pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;
	ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
	//FIXME:
	if (!pSkeletonAnim)
		return;

	Vec3	forward(1,0,0);
	Vec3  vEntityPos = pEntity->GetWorldPos();
	bool isCharacterVisible = pCharacter->IsCharacterVisible() != 0;
	EStance playerStance = m_pPlayer->GetStance();

	const SActorStats * pActorStats = m_pPlayer->GetActorStats();

	if (m_pPlayer->IsPlayer())
	{
		// PLAYER CHARACTER
		Vec3 vNewEyePosition, vNewEyeDirection;

		if (m_pPlayer->IsClient() && !m_pPlayer->IsThirdPerson())
		{
			//Beni - New FP aiming
			vNewEyePosition = GetISystem()->GetViewCamera().GetPosition();
		}
		else
		{
			Vec3 viewOfs(m_pPlayer->GetStanceViewOffset(playerStance));
			vNewEyePosition = vEntityPos + m_pPlayer->GetBaseQuat() * viewOfs;
		}

		if (!m_pPlayer->IsClient()) // marcio: fixes the eye direction for remote players
			vNewEyeDirection = (m_lookTarget - vNewEyePosition).GetNormalizedSafe(state.eyeDirection);
		else
			vNewEyeDirection = m_pPlayer->GetViewQuatFinal().GetColumn1();

		state.animationEyeDirection = vNewEyeDirection;
		state.weaponPosition = vNewEyePosition;
		state.fireDirection = state.aimDirection = vNewEyeDirection;
		state.eyeDirection = vNewEyeDirection;
		state.fireTarget = m_fireTarget;
		state.eyePosition = vNewEyePosition;

		if (pActorStats && pActorStats->mountedWeaponID)
		{
			state.isAiming = true;
		}
		else if (isCharacterVisible)
		{
			state.isAiming = pCharacter->GetISkeletonPose()->GetAimIKBlend() > 0.99f;
		}
		else
		{
			state.isAiming = true;
		}
	}
	else
	{
		// AI CHARACTER

		Quat	orientation = pEntity->GetWorldRotation();
		forward = orientation.GetColumn1();
		Vec3	entityPos = vEntityPos;
		assert( entityPos.IsValid() );
		Vec3	constrainedLookDir = m_lookTarget - entityPos;
		Vec3	constrainedAimDir = m_aimTarget - entityPos;
		constrainedLookDir.z = 0.0f;
		constrainedAimDir.z = 0.0f;

		constrainedAimDir = constrainedAimDir.GetNormalizedSafe(constrainedLookDir.GetNormalizedSafe(forward));
		constrainedLookDir = constrainedLookDir.GetNormalizedSafe(forward);

		Matrix33	lookRot;
		lookRot.SetRotationVDir(constrainedLookDir);
		state.eyePosition = entityPos + lookRot.TransformVector(m_pPlayer->GetEyeOffset());

		Matrix33	aimRot;
		aimRot.SetRotationVDir(constrainedAimDir);
		state.weaponPosition = entityPos + aimRot.TransformVector(m_pPlayer->GetWeaponOffset());
			
		state.upDirection = orientation.GetColumn2();

		state.eyeDirection = (m_lookTarget - state.eyePosition).GetNormalizedSafe(forward); //(lEyePos - posHead).GetNormalizedSafe(FORWARD_DIRECTION);
		state.animationEyeDirection = state.eyeDirection;
		state.aimDirection = (m_aimTarget - state.weaponPosition).GetNormalizedSafe((m_lookTarget - state.weaponPosition).GetNormalizedSafe(forward)); //pEntity->GetRotation() * dirWeapon;
		state.fireTarget = m_fireTarget;
		state.fireDirection = (state.fireTarget - state.weaponPosition).GetNormalizedSafe(forward);
		state.bodyDirection = pEntity->GetWorldRotation().GetColumn1();

		//changed by ivo: most likely this doesn't work any more
		//state.bodyDirection = pEntity->GetRotation() * pSkeleton->GetCurrentBodyDirection();
		// [kirill] when AI at MG need to update body/movementDirection coz entity is not moved/rotated AND set AIPos/weaponPs to weapon
		if (pActorStats && pActorStats->mountedWeaponID)
		{		
			IEntity *pWeaponEntity = gEnv->pEntitySystem->GetEntity(pActorStats->mountedWeaponID);
			if(pWeaponEntity)
			{
				state.eyePosition.x = pWeaponEntity->GetWorldPos().x;
				state.eyePosition.y = pWeaponEntity->GetWorldPos().y;
				state.weaponPosition = state.eyePosition;
				if(	m_pPlayer->GetEntity()->GetAI() &&
					m_pPlayer->GetEntity()->GetAI()->GetProxy() &&
					m_pPlayer->GetEntity()->GetAI()->GetProxy()->GetCurrentWeapon() )
				{
					IWeapon * pMG = m_pPlayer->GetEntity()->GetAI()->GetProxy()->GetCurrentWeapon();
					state.weaponPosition = pMG->GetFiringPos( m_fireTarget );
				}
				// need to recalculate aimDirection, since weaponPos is changed
				state.aimDirection = (m_aimTarget - state.weaponPosition).GetNormalizedSafe((m_lookTarget - state.weaponPosition).GetNormalizedSafe(forward)); //pEntity->GetRotation() * dirWeapon;
			}

			state.isAiming = !m_aimClamped;
			state.bodyDirection = state.aimDirection;
			state.movementDirection = state.aimDirection;
		}
		else
		{
			state.bodyDirection = pEntity->GetWorldRotation().GetColumn1();

			if (isCharacterVisible)
				state.isAiming = pCharacter->GetISkeletonPose()->GetAimIKBlend() > 0.99f;
			else
				state.isAiming = true;
		}
	}

	//changed by ivo: most likely this doesn't work any more
	//state.movementDirection = pEntity->GetRotation() * pSkeleton->GetCurrentBodyDirection();
	state.movementDirection = pEntity->GetRotation().GetColumn1();

	if (m_pPlayer->GetLastRequestedVelocity().len2() > 0.01f)
		state.movementDirection = m_pPlayer->GetLastRequestedVelocity().GetNormalized();	

	state.lean = pActorStats ? ((SPlayerStats*)pActorStats)->leanAmount : 0.0f;

	state.atMoveTarget = m_atTarget;
	state.desiredSpeed = m_desiredSpeed;
	state.stance = playerStance;
	state.upDirection = pEntity->GetWorldRotation().GetColumn2();

	state.minSpeed = -1.0f;
	state.maxSpeed = -1.0f;
	state.normalSpeed = -1.0f;

	state.m_StanceSize = m_pPlayer->GetStanceInfo(playerStance)->GetStanceBounds();

	state.pos = vEntityPos;

	state.isFiring = (pActorStats->inFiring>=10.f);

	state.isAlive = (m_pPlayer->GetHealth()>0);

	IVehicle *pVehicle = m_pPlayer->GetLinkedVehicle();
	if (pVehicle)
	{
		IMovementController *pVehicleMovementController = pVehicle->GetPassengerMovementController(m_pPlayer->GetEntityId());
		if (pVehicleMovementController)
			pVehicleMovementController->GetMovementState(state);
	}	

	const IAnimatedCharacter* pAC = m_pPlayer->GetAnimatedCharacter();
	state.slopeAngle = (pAC != NULL) ? pAC->GetSlopeDegreeMoveDir() : 0.0f;

	m_pPlayer->SendPerkEvent(EPE_OverrideMovementStateData, & state);
}

bool CPlayerMovementController::GetStanceState( const SStanceStateQuery& query, SStanceState& state )
{
	IEntity * pEntity = m_pPlayer->GetEntity();
	const SStanceInfo*	stanceInfo = m_pPlayer->GetStanceInfo(query.stance);
	if(!stanceInfo)
		return false;

	Quat orientation = pEntity->GetWorldRotation();
	Vec3 forward = orientation.GetColumn1();
	Vec3 pos = query.position.IsZero() ? pEntity->GetWorldPos() : query.position;

	Vec3 aimTarget = m_aimTarget;
	Vec3 lookTarget = m_lookTarget;

	if (!query.target.IsZero())
	{
		aimTarget = query.target;
		lookTarget = query.target;
		forward = (query.target - pos).GetNormalizedSafe(forward);
	}

	state.pos = pos;
	state.m_StanceSize = stanceInfo->GetStanceBounds();
	state.m_ColliderSize = stanceInfo->GetColliderBounds();
	state.lean = query.lean;	// pass through
	state.peekOver = query.peekOver;

	if(query.defaultPose)
	{
		state.eyePosition = pos + stanceInfo->GetViewOffsetWithLean(query.lean, query.peekOver);
		state.weaponPosition = pos + m_pPlayer->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pPlayer->GetEyeOffset());
		state.upDirection.Set(0,0,1);
		state.eyeDirection = FORWARD_DIRECTION;
		state.aimDirection = FORWARD_DIRECTION;
		state.fireDirection = FORWARD_DIRECTION;
		state.bodyDirection = FORWARD_DIRECTION;
	}
	else
	{
		Vec3 constrainedLookDir = lookTarget - pos;
		Vec3 constrainedAimDir = aimTarget - pos;
		constrainedLookDir.z = 0.0f;
		constrainedAimDir.z = 0.0f;

		constrainedAimDir = constrainedAimDir.GetNormalizedSafe(constrainedLookDir.GetNormalizedSafe(forward));
		constrainedLookDir = constrainedLookDir.GetNormalizedSafe(forward);

		Matrix33 lookRot;
		lookRot.SetRotationVDir(constrainedLookDir);
		state.eyePosition = pos + lookRot.TransformVector(stanceInfo->GetViewOffsetWithLean(query.lean, query.peekOver));

		Matrix33 aimRot;
		aimRot.SetRotationVDir(constrainedAimDir);
		state.weaponPosition = pos + aimRot.TransformVector(m_pPlayer->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pPlayer->GetEyeOffset()));

		state.upDirection = orientation.GetColumn2();

		state.eyeDirection = (lookTarget - state.eyePosition).GetNormalizedSafe(forward);
		state.aimDirection = (aimTarget - state.weaponPosition).GetNormalizedSafe((lookTarget - state.weaponPosition).GetNormalizedSafe(forward));
		state.fireDirection = state.aimDirection;
		state.bodyDirection = forward;
	}

	return true;
}

void CPlayerMovementController::Release()
{
	delete this;
}

void CPlayerMovementController::IdleUpdate(float frameTime)
{
	m_idleChecker.Update(frameTime);
}

bool CPlayerMovementController::GetStats(IActorMovementController::SStats& stats)
{
	stats.idle = m_idleChecker.IsIdle();
	return true;
}

CPlayerMovementController::CIdleChecker::CIdleChecker() : m_pMC(0)
{
}


void CPlayerMovementController::CIdleChecker::Reset(CPlayerMovementController* pMC)
{
	m_pMC = pMC;
	m_timeToIdle = IDLE_CHECK_TIME_TO_IDLE;
	m_movementTimeOut = IDLE_CHECK_MOVEMENT_TIMEOUT;
	m_bInIdle = false;
	m_bHaveValidMove = false;
	m_bHaveInteresting = false;
	m_frameID = -1;
}

void CPlayerMovementController::CIdleChecker::Update(float frameTime)
{
	if (m_pMC->m_pPlayer->IsPlayer())
		return;

	bool goToIdle = false;
	bool wakeUp = false;

	if (m_bHaveValidMove)
	{
		m_movementTimeOut = IDLE_CHECK_MOVEMENT_TIMEOUT;
		m_bHaveValidMove = false;
		wakeUp = true;
	}
	else
	{
		m_movementTimeOut-= frameTime;
		if (m_movementTimeOut <= 0.0f)
		{
			goToIdle = true;
		}
	}

#if 0
	if (m_bHaveInteresting)
	{
		m_timeToIdle = IDLE_CHECK_TIME_TO_IDLE;
		m_bHaveInteresting = false;
		wakeUp = true;
	}
	else
	{
		m_timeToIdle -= frameTime;
		if (m_timeToIdle <= 0.0f)
		{
			goToIdle = true;
		}
	}
#endif

	if (goToIdle && !wakeUp && m_bInIdle == false)
	{
		// turn idle on
#ifdef DEBUG_IDLE_CHECK
		CryLogAlways("Turn Idle ON 0x%p %s", this, m_pMC->m_pPlayer->GetEntity()->GetName());
#endif
		m_bInIdle = true;
	}
	else if (wakeUp && m_bInIdle)
	{
		// turn idle off
#ifdef DEBUG_IDLE_CHECK
		CryLogAlways("Turn Idle OFF 0x%p %s", this, m_pMC->m_pPlayer->GetEntity()->GetName());
#endif
		m_bInIdle = false;
	}

	static int sCurFrame = -1;
	static int sIdles = 0;
	static int sAwake = 0;
	
	if (g_pGameCVars->g_showIdleStats != 0)
	{
		int frameId = gEnv->pRenderer->GetFrameID(false);
		if (frameId != sCurFrame)
		{
			float fColor[4] = {1,0,0,1};
			gEnv->pRenderer->Draw2dLabel( 1,22, 1.3f, fColor, false,"Idles=%d Awake=%d", sIdles, sAwake );
			sCurFrame = frameId;
			sIdles = sAwake = 0;
		}
		if (m_bInIdle)
			++sIdles;
		else
			++sAwake;
	}
}

bool CPlayerMovementController::CIdleChecker::Process(SMovementState& movementState, 
																										  CMovementRequest& currentReq,
																										  CMovementRequest& newReq)
{
	if (m_pMC->m_pPlayer->IsPlayer())
		return false;

	IRenderer* pRend = gEnv->pRenderer;
	int nCurrentFrameID = pRend->GetFrameID(false);

	// check if we have been called already in this frame
	// if so, and we already have a validMove request -> early return
	if (nCurrentFrameID != m_frameID)
	{
		m_frameID = nCurrentFrameID;
	}
	else
	{
		if (m_bHaveValidMove)
			return true;
	}

	// no valid move request up to now (not in this frame)
	m_bHaveValidMove = false;

	// an empty moverequest 
	if (currentReq.IsEmpty())
	{
		// CryLogAlways("IdleCheck: Req empty 0x%p %s", this, m_pMC->m_pPlayer->GetEntity()->GetName());
		;
	}
	else if (newReq.HasDeltaMovement() && newReq.HasDeltaRotation())
	{
		m_bHaveValidMove = true;
	}
	else if (newReq.HasMoveTarget() && 
		( (newReq.HasDesiredSpeed() && newReq.GetDesiredSpeed() > 0.001f)
		||(newReq.HasPseudoSpeed() && newReq.GetPseudoSpeed() > 0.001f) ))
	{
		m_bHaveValidMove = true;
	}

	return m_bHaveValidMove;
}

void CPlayerMovementController::CIdleChecker::Serialize(TSerialize &ser)
{
	ser.Value("m_timeToIdle", m_timeToIdle);
	ser.Value("m_movementTimeOut", m_movementTimeOut);
	ser.Value("m_frameID", m_frameID);
	ser.Value("m_bHaveInterest", m_bHaveInteresting);
	ser.Value("m_bInIdle", m_bInIdle);
	ser.Value("m_bHaveValidMove", m_bHaveValidMove);
}

void CPlayerMovementController::Serialize(TSerialize &ser)
{
	if(ser.GetSerializationTarget() != eST_Network)	//basic serialization
	{
		ser.Value("DesiredSpeed", m_desiredSpeed);
		ser.Value("atTarget", m_atTarget);
		ser.Value("m_usingLookIK", m_usingLookIK);
		ser.Value("m_usingAimIK", m_usingAimIK);
		ser.Value("m_aimClamped", m_aimClamped);
		ser.Value("m_lookTarget", m_lookTarget);
		ser.Value("m_aimTarget", m_aimTarget);
		ser.Value("m_animTargetSpeed", m_animTargetSpeed);
		ser.Value("m_animTargetSpeedCounter", m_animTargetSpeedCounter);
		ser.Value("m_fireTarget", m_fireTarget);
		ser.EnumValue("targetStance", m_targetStance, STANCE_NULL, STANCE_LAST);

		SMovementState m_currentMovementState;

		ser.BeginGroup("m_currentMovementState");

		ser.Value("bodyDir", m_currentMovementState.bodyDirection);
		ser.Value("aimDir", m_currentMovementState.aimDirection);
		ser.Value("fireDir", m_currentMovementState.fireDirection);
		ser.Value("fireTarget", m_currentMovementState.fireTarget);
		ser.Value("weaponPos", m_currentMovementState.weaponPosition);
		ser.Value("desiredSpeed", m_currentMovementState.desiredSpeed);
		ser.Value("moveDir", m_currentMovementState.movementDirection);
		ser.Value("upDir", m_currentMovementState.upDirection);
		ser.EnumValue("stance", m_currentMovementState.stance, STANCE_NULL, STANCE_LAST);
		ser.Value("Pos", m_currentMovementState.pos);
		ser.Value("eyePos", m_currentMovementState.eyePosition);
		ser.Value("eyeDir", m_currentMovementState.eyeDirection);
		ser.Value("animationEyeDirection", m_currentMovementState.animationEyeDirection);
		ser.Value("minSpeed", m_currentMovementState.minSpeed);
		ser.Value("normalSpeed", m_currentMovementState.normalSpeed);
		ser.Value("maxSpeed", m_currentMovementState.maxSpeed);
		ser.Value("stanceSize.Min", m_currentMovementState.m_StanceSize.min);
		ser.Value("stanceSize.Max", m_currentMovementState.m_StanceSize.max);
		ser.Value("colliderSize.Min", m_currentMovementState.m_ColliderSize.min);
		ser.Value("colliderSize.Max", m_currentMovementState.m_ColliderSize.max);
		ser.Value("atMoveTarget", m_currentMovementState.atMoveTarget);
		ser.Value("isAlive", m_currentMovementState.isAlive);
		ser.Value("isAiming", m_currentMovementState.isAiming);
		ser.Value("isFiring", m_currentMovementState.isFiring);
		ser.Value("isVis", m_currentMovementState.isVisible);

		ser.EndGroup();

		ser.BeginGroup("AimInterpolator");
		m_aimInterpolator.Serialize(ser);
		ser.EndGroup();

		ser.BeginGroup("LookInterpolator");
		m_lookInterpolator.Serialize(ser);
		ser.EndGroup();

		ser.Value("inputDesiredSpeed", m_inputDesiredSpeed);
		ser.Value("inputDersiredAngleZ", m_inputDesiredTurnAngleZ);
		ser.Value("inputStance", m_inputStance);
		ser.Value("inputPseudoSpeed", m_inputPseudoSpeed);

		if(ser.IsReading())
			m_idleChecker.Reset(this);
		m_idleChecker.Serialize(ser);

		if(ser.IsReading())
			UpdateMovementState(m_currentMovementState);
	}
}

void CPlayerMovementController::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
}