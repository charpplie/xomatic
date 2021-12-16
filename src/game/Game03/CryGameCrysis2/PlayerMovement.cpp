#include "StdAfx.h"
#include "PlayerMovement.h"
#include "GameUtils.h"
#include "Game.h"
#include "GameCVars.h"
#include "PlayerInput.h"
#include "GameActions.h"
#include "NetInputChainDebug.h"
#include "GameRules.h"
#include "Perk.h"
#include "Weapon.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#undef CALL_PLAYER_EVENT_LISTENERS
#define CALL_PLAYER_EVENT_LISTENERS(func) \
{ \
	if (m_player.m_playerEventListeners.empty() == false) \
	{ \
	  CPlayer::TPlayerEventListeners::const_iterator iter = m_player.m_playerEventListeners.begin(); \
	  CPlayer::TPlayerEventListeners::const_iterator cur; \
	  while (iter != m_player.m_playerEventListeners.end()) \
	  { \
	  	cur = iter; \
	  	++iter; \
	  	(*cur)->func; \
	  } \
	} \
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
CPlayerMovement::CPlayerMovement(CPlayer& player, const SActorFrameMovementParams& movement, float m_frameTime ) : 
	m_frameTime(m_frameTime),
	m_params(player.m_params),
	m_stats(player.m_stats),
	m_viewQuat(player.m_viewQuat),
	m_baseQuat(player.m_baseQuat),
	m_movement(movement),
	m_player(player),
	m_velocity(player.m_velocity),
	m_upVector(player.m_upVector),
	m_jumped(player.m_stats.jumped),
	m_actions(player.m_actions),
	m_actionFlags(player.m_actionFlags),
	m_hasJumped(false),
	m_swimJumping(player.m_stats.swimJumping),
	m_stickySurfaceTimer(player.m_stickySurfaceTimer)
{
	// derive some values that will be useful later
	m_worldPos = player.GetEntity()->GetWorldPos();
}

//-----------------------------------------------------------------------------------------------
void CPlayerMovement::Process(CPlayer& player)
{
	//FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);
	
	if (m_stats.isStandingUp)
		return;

	if (m_stats.spectatorInfo.mode || m_stats.flyMode)
		ProcessFlyMode();
	else if (player.IsOnLedge())
		ProcessMovementOnLedge();
	else if (player.IsSliding())
		ProcessSliding();
	else if (player.ShouldSwim())
		ProcessSwimming();
	else
		ProcessOnGroundOrJumping(player);

	// if (!m_player.GetLinkedEntity() && !m_player.GetEntity()->GetParent()) // Leipzig hotfix, these can get out of sync
	if (ShouldProcessTurning())
		ProcessTurning();
}

//-----------------------------------------------------------------------------------------------
void CPlayerMovement::Commit( CPlayer& player )
{
	if (player.m_pAnimatedCharacter)
	{
		m_request.allowStrafe = m_movement.allowStrafe;
		m_request.prediction = m_movement.prediction;

		CRY_ASSERT(m_request.velocity.IsValid());

		NETINPUT_TRACE(m_player.GetEntityId(), m_request.rotation * FORWARD_DIRECTION);
		NETINPUT_TRACE(m_player.GetEntityId(), m_request.velocity);

		m_request.jumping = m_stats.jumped;

		m_player.DebugGraph_AddValue("ReqVelo", m_request.velocity.GetLength());
		m_player.DebugGraph_AddValue("ReqVeloX", m_request.velocity.x);
		m_player.DebugGraph_AddValue("ReqVeloY", m_request.velocity.y);
		m_player.DebugGraph_AddValue("ReqVeloZ", m_request.velocity.z);
		m_player.DebugGraph_AddValue("ReqRotZ", RAD2DEG(m_request.rotation.GetRotZ()));

		player.m_pAnimatedCharacter->AddMovement( m_request );
	}

	if (m_hasJumped && !m_player.GetPerkData<bool>(EPD_MuteJumping))
		player.CreateScriptEvent("jumped",0);

	NETINPUT_TRACE(m_player.GetEntityId(), m_velocity);
	NETINPUT_TRACE(m_player.GetEntityId(), m_jumped);

	// Reset ground timer to prevent ground time before the jump to be inherited
	// and incorrectly/prematurely used to identify landing in mid air in MP.
	if (m_jumped && !player.m_stats.jumped)
		player.m_stats.onGround = 0.0f;

	player.m_velocity = m_velocity;
	player.m_stats.jumped = m_jumped;
	player.m_lastRequestedVelocity = m_request.velocity; 
	player.m_stats.swimJumping = m_swimJumping;

	player.m_stickySurfaceTimer = m_stickySurfaceTimer;
	
	player.m_stats.bSprinting = ShouldSprint();
}

bool CPlayerMovement::ShouldSprint() const
{
	bool movingForward = (m_stats.inMovement > 0.1f) && IsMovingForward();
	bool restrictSprint = (m_stats.inAir > g_pGameCVars->pl_movement.sprint_timeInAirToStopSprinting) || m_stats.jumped || m_stats.bIgnoreSprinting || m_player.HasHeavyWeaponEquipped() || m_player.IsSliding();

	const SNanoSuitGameParameters& suitParams = m_player.GetActorSuitGameParameters();
	bool suitVisor = suitParams.IsSuitPowerActive() && (suitParams.GetMode() == eNanoSuitMode_Tactical);

	CWeapon* pWeapon = m_player.GetWeapon(m_player.GetCurrentItemId());
	bool isZooming = (pWeapon && !suitVisor) ? pWeapon->IsZoomed() && !pWeapon->IsZoomingInOrOut() : false;
	bool isReloading = gEnv->bMultiplayer && pWeapon ? pWeapon->IsReloading() : false;
	if (pWeapon && pWeapon->IsBusy())
		restrictSprint = true;

	bool shouldSprint = false;

	if (m_player.m_stats.bSprinting == false)
	{
		shouldSprint = movingForward && !restrictSprint && !isZooming && !isReloading && (m_actions & ACTION_SPRINT);
		CCCPOINT_IF(shouldSprint, PlayerMovement_SprintOn);
		CCCPOINT_IF((! shouldSprint) && (m_actions & ACTION_SPRINT), PlayerMovement_SprintRequestIgnored);
	}
	else
	{
		shouldSprint = movingForward && !restrictSprint && !isZooming && !isReloading;

		shouldSprint = shouldSprint && (!(m_actions & ACTION_CROUCH) || (m_actions & ACTION_SPRINT));

		CCCPOINT_IF(! shouldSprint, PlayerMovement_SprintOff);

		if(!shouldSprint && pWeapon)
		{
			pWeapon->ForcePendingActions();
		}
	}

	return shouldSprint;
}

//-----------------------------------------------------------------------------------------------
// utility functions
//-----------------------------------------------------------------------------------------------
static Vec3 ProjectPointToLine(const Vec3 &point,const Vec3 &lineStart,const Vec3 &lineEnd)
{
	Lineseg seg(lineStart, lineEnd);
	float t;
	Distance::Point_Lineseg( point, seg, t );
	return seg.GetPoint(t);
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
// NOTE: also used for Spectator Mode
void CPlayerMovement::ProcessFlyMode()
{
	Vec3 move = m_viewQuat * m_movement.desiredVelocity;

	float zMove(0.0f);
	if (m_actions & ACTION_JUMP)
		zMove += 1.0f;
	if (m_actions & ACTION_CROUCH)
		zMove -= 1.0f;

	move += m_viewQuat.GetColumn2() * zMove;

	//cap the movement vector to max 1
	float moveModule(move.len());

	if (moveModule > 1.0f)
 		move /= moveModule;

	move *= m_params.speedMultiplier * m_player.GetWeaponMovementFactor();
	move *= 30.0f;

	if (m_actions & ACTION_SPRINT)
		move *= 10.0f;

	m_request.type = eCMT_Fly;
	m_request.velocity = move;
}

//-----------------------------------------------------------------------------------------------
bool CPlayerMovement::ShouldGetOutBoost(Vec3& entityPos)
{
	//	We've run into something
	return m_stats.hasRunIntoSomething;

	//TODO:
	//	See if we can get on top of it
	//	Linetest forward at our height + a bit
	//		If clear, linetest up & down
	//			If enough room up & down
	//				return true
}

//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessSwimming()
{
 	bool debug = (g_pGameCVars->cl_debugSwimming != 0);
	const bool previousSwimJumping = m_swimJumping;
	bool bNewSwimJumping = m_swimJumping;
	float fNewSwimJumping = bNewSwimJumping ? 1.0f : -1.0f;
	bool bOutOfWaterPoint2 = m_stats.relativeWaterLevel > 0.2f;
	Vec3 entityPos = m_player.GetEntity()->GetWorldPos();
	Vec3 vRight(m_baseQuat.GetColumn0());

	// Don't enable sticky surface immediately when entering water.
	m_stickySurfaceTimer = (float)__fsel(m_stats.inWaterTimer - 0.5f, m_stickySurfaceTimer, 0.0f);
	
	bool shouldGetOutBoost = false;

	const SNanoSuitGameParameters& suitParams = m_player.GetActorSuitGameParameters();

	IPhysicalEntity* pPhysEnt = m_player.GetEntity()->GetPhysics();
	if (pPhysEnt != NULL) // entity might have been shattered on be unphysicalized
	{
		pe_status_dynamics sd;
		if (pPhysEnt->GetStatus(&sd) != 0)
			m_velocity = sd.v;

		pe_player_dynamics pd;
		pd.kAirControl = 1.0f;
		pd.kAirResistance = 0.0f;
		pPhysEnt->SetParams(&pd);
	}

	{
		// Apply water flow velocity to the player
		Vec3 gravity;
		pe_params_buoyancy buoyancy;
		if (gEnv->pPhysicalWorld->CheckAreas(entityPos, gravity, &buoyancy))
			m_velocity += buoyancy.waterFlow*m_frameTime;
	}

	Vec3 acceleration(ZERO);

	//--------------------

	// Apply gravity when above the surface.
 	if (bNewSwimJumping || bOutOfWaterPoint2)
	{
		float gravityScaling = 0.5f;
		if (!m_stats.gravity.IsZeroFast())
			acceleration += m_stats.gravity * gravityScaling;
		else
			acceleration.z += -9.8f * gravityScaling;
	}

	

	//--------------------

	if ((m_velocity.z < -0.5f) && (m_stats.relativeWaterLevel < -0.2f))
	{
		bNewSwimJumping = false;
	}

	// Apply jump impulse when below but close to the surface (if in water for long enough).
	if (!bNewSwimJumping && (m_actions & ACTION_JUMP) && (m_velocity.z >= -0.2f) && (m_stats.relativeWaterLevel > -0.1f) && (m_stats.relativeWaterLevel < 0.1f))
	{
		const float jumpMul = suitParams.GetProps().superJumpScale;

		m_velocity.z = max(m_velocity.z, 6.0f + 2.0f * jumpMul);
		bNewSwimJumping = true;
	}

	if (bOutOfWaterPoint2 && (m_velocity.z > 0.5f))
	{
		bNewSwimJumping = true;
	}

	if(m_stats.relativeWaterLevel > -0.8f && m_stats.relativeWaterLevel < 0.4f)
	{
		shouldGetOutBoost = ShouldGetOutBoost(entityPos);
		bNewSwimJumping |= shouldGetOutBoost;		
	}

	//--------------------

	// Apply desired movement
	Vec3 desiredLocalNormalizedVelocity(ZERO);
	Vec3 desiredLocalVelocity(ZERO);
	Vec3 desiredWorldVelocity(ZERO);

	// Less control when jumping or otherwise above surface
	// (where bottom ray miss geometry that still keeps the collider up).
	float userControlFraction = (float)__fsel(fNewSwimJumping, 0.2f, 1.0f);

	// Calculate desired acceleration (user input)		
	{
		// Apply up/down control when well below surface (if in water for long enough).
		if ((m_stats.inWaterTimer > 0.5f) && !bNewSwimJumping)
		{
			if (m_actions & ACTION_JUMP)
				desiredLocalNormalizedVelocity.z += 1.0f;
			else if (m_actions & ACTION_CROUCH)
				desiredLocalNormalizedVelocity.z -= 1.0f;
		}

		float backwardMultiplier = (m_movement.desiredVelocity.y < 0.0f) ? g_pGameCVars->pl_swimBackSpeedMul : 1.0f;
		desiredLocalNormalizedVelocity.x = m_movement.desiredVelocity.x * g_pGameCVars->pl_swimSideSpeedMul;
		desiredLocalNormalizedVelocity.y = m_movement.desiredVelocity.y * backwardMultiplier;

		// AI can set a custom sprint value, so don't cap the movement vector
		float sprintMultiplier = 1.0f;
		if ((m_actions & ACTION_SPRINT) && !bOutOfWaterPoint2)
		{
			sprintMultiplier = g_pGameCVars->pl_swimNormalSprintSpeedMul;

			if ((suitParams.GetMode() == eNanoSuitMode_Power) && (suitParams.GetState()&~eNanoSuitState_Disabled))
				sprintMultiplier = g_pGameCVars->pl_swimSpeedSprintSpeedMul;

			// Higher speed multiplier when sprinting while looking up, to get higher dolphin jumps.
			float upFraction = clamp(m_viewQuat.GetFwdZ(), 0.0f, 1.0f);
			sprintMultiplier *= LERP(1.0f, g_pGameCVars->pl_swimUpSprintSpeedMul, upFraction);
		}
		
		float baseSpeed = g_pGameCVars->pl_swimBaseSpeed;

		if (m_player.IsPerkActive(ePerk_Dolphin))
		{
			desiredLocalVelocity = desiredLocalNormalizedVelocity * CPerk::GetInstance()->GetVars()->perk_dolphinSpeedMult * sprintMultiplier * baseSpeed;
		}
		else
		{
			desiredLocalVelocity.x = desiredLocalNormalizedVelocity.x * sprintMultiplier * baseSpeed;
			desiredLocalVelocity.y = desiredLocalNormalizedVelocity.y * sprintMultiplier * baseSpeed;
			desiredLocalVelocity.z = desiredLocalNormalizedVelocity.z * g_pGameCVars->pl_swimVertSpeedMul * baseSpeed;
		}

		// The desired movement is applied in viewspace, not in entityspace, since entity does not necessarily pitch while swimming.
		desiredWorldVelocity += m_viewQuat.GetColumn0() * desiredLocalVelocity.x;
		desiredWorldVelocity += m_viewQuat.GetColumn1() * desiredLocalVelocity.y;
		
		// though, apply up/down in world space.
		desiredWorldVelocity.z += desiredLocalVelocity.z;

#if !defined(_RELEASE)
		if (debug)
		{
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.8f), 1.5f, "BaseSpeed %1.3f", baseSpeed);
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,1.0f), 1.5f, "SprintMul %1.2f", sprintMultiplier);
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.6f), 1.5f, "MoveN[%1.3f, %1.3f, %1.3f]", desiredLocalNormalizedVelocity.x, desiredLocalNormalizedVelocity.y, desiredLocalNormalizedVelocity.z);
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.5f), 1.5f, "VeloL[%1.3f, %1.3f, %1.3f]", desiredLocalVelocity.x, desiredLocalVelocity.y, desiredLocalVelocity.z);
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.4f), 1.5f, "VeloW[%1.3f, %1.3f, %1.3f]", desiredWorldVelocity.x, desiredWorldVelocity.y, desiredWorldVelocity.z);
		}
#endif

		acceleration += desiredWorldVelocity * userControlFraction;
	}

	//--------------------

	float surfaceDistanceFraction = clamp(fabsf(m_stats.relativeWaterLevel), 0.0f, 1.0f);
	float surfaceProximityInfluence = 1.0f - surfaceDistanceFraction;
	float verticalVelocityFraction = clamp((fabsf(desiredWorldVelocity.z) - 0.3f) / 0.4f, 0.0f, 1.0f);
	surfaceProximityInfluence = (float)__fsel(fNewSwimJumping, 0.0f, surfaceProximityInfluence * (1.0f - verticalVelocityFraction));

	//--------------------

	// Apply acceleration (framerate independent)
	const float accelerateDelay = 0.3f;
	m_velocity += acceleration * (m_frameTime / accelerateDelay);

	// Apply velocity dampening (framerate independent)
	Vec3 damping(ZERO);
	{
		damping.x = (float)__fsel(fNewSwimJumping, 0.0f, fabsf(m_velocity.x));
		damping.y = (float)__fsel(fNewSwimJumping, 0.0f, fabsf(m_velocity.y));

		// Vertical damping is special, to allow jumping out of water with higher speed, 
		// and also not sink too deep when falling down ito the water after jump or such.
		float zDamp = 1.0f + (6.0f * clamp((-m_velocity.z - 1.0f) / 3.0f, 0.0f, 1.0f));
		zDamp *= 1.0f - surfaceProximityInfluence;
		
		zDamp = (float)__fsel(fNewSwimJumping, 0.0f, zDamp);

		damping.z = fabsf(m_velocity.z) * zDamp;

		const float stopDelay = 0.3f;
		damping *= (m_frameTime / stopDelay);
		m_velocity.x = (float)__fsel((fabsf(m_velocity.x) - damping.x), (m_velocity.x - fsgnf(m_velocity.x) * damping.x), 0.0f);
		m_velocity.y = (float)__fsel((fabsf(m_velocity.y) - damping.y), (m_velocity.y - fsgnf(m_velocity.y) * damping.y), 0.0f);
		m_velocity.z = (float)__fsel((fabsf(m_velocity.z) - damping.z), (m_velocity.z - fsgnf(m_velocity.z) * damping.z), 0.0f);
	}

	// ---- Dolphin Perk Effects ---

	if (m_player.IsPerkActive(ePerk_Dolphin))
	{
		if(bNewSwimJumping)
		{
			Vec3 vFwd(m_baseQuat.GetColumn1());
			m_player.SpawnParticleEffect("perk_fx.dolphin_jump.water_trail", entityPos, vFwd);

			if(!previousSwimJumping)
			{
				Vec3 vUp(0.0f, 0.0f, 1.0f);
				m_player.SpawnParticleEffect("perk_fx.dolphin_jump.water_exit", entityPos, vUp);
			}
		}
	}

	//--------------------

	if (surfaceProximityInfluence > 0.0f)
	{
		float bNewStickySurfaceTimer = m_stickySurfaceTimer + (m_frameTime * surfaceProximityInfluence);
		float stickyFraction = clamp(bNewStickySurfaceTimer, 0.0f, 1.0f);
		m_stickySurfaceTimer = bNewStickySurfaceTimer;

		float desiredVeloZ = m_frameTime > 0.0f ? (m_stats.worldWaterLevelDelta / m_frameTime * surfaceProximityInfluence) : 0.0f;
		m_velocity.z = LERP(m_velocity.z, desiredVeloZ, stickyFraction);

		m_velocity.z += stickyFraction * 1.0f * -fsgnf(m_stats.relativeWaterLevel) * powf(surfaceDistanceFraction, 1.0f);
	}
	else
	{
		m_stickySurfaceTimer = 0.0f;
	}

	//--------------------

	if(shouldGetOutBoost)
	{
		float velocityBoost = max(-(g_pGameCVars->pl_swimGetOutBoost * ((m_stats.relativeWaterLevel * 0.5f) + g_pGameCVars->pl_swimGetOutOffset)), 1.4f);
		m_velocity.z = (float)__fsel(m_velocity.z - g_pGameCVars->pl_swimGetOutBoost, m_velocity.z, m_velocity.z + velocityBoost );
	}

	// Set request type and velocity
	m_request.type = eCMT_Fly;
	m_request.velocity = m_velocity;
	
	// DEBUG VELOCITY
	if (debug)
	{
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.0f), 1.5f, "Velo[%1.3f, %1.3f, %1.3f]", m_velocity.x, m_velocity.y, m_velocity.z);
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.2f), 1.5f, " Axx[%1.3f, %1.3f, %1.3f]", acceleration.x, acceleration.y, acceleration.z);
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.4f), 1.5f, "Damp[%1.3f, %1.3f, %1.3f]", damping.x, damping.y, damping.z);
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.6f), 1.5f, "FrameTime %1.4f", m_frameTime);
		if (bNewSwimJumping)
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 0.15f + Vec3(0,0,0.6f), 2.0f, "JUMP");
	}

	m_swimJumping = bNewSwimJumping;

	return;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
Vec3 CPlayerMovement::CalculateVelocityWithAirFrictionPerkActivated(const Vec3& desiredVelocity)
{
	const SPlayerAirFrictionPerkControl& glideControl = g_pGameCVars->pl_airFrictionPerk_control;

	const float playerHeight = m_player.GetEntity()->GetWorldPos().z;
	const float fallDistanceWithGlider = (m_stats.startFallingHeight - glideControl.fall_activation_height) - playerHeight;
	const float glideDistanceBase = glideControl.fall_maxSeepd_height - glideControl.fall_activation_height;
	const float fallingHeightFactor = clamp((float)__fsel(-glideDistanceBase, 1.0f, fallDistanceWithGlider * (float)__fres(glideDistanceBase)), 0.0f, 1.0f);

	//Clamp forward velocity if required
	float desiredVelocityLength = desiredVelocity.len();
	desiredVelocityLength = (float)__fsel(-(desiredVelocityLength - glideControl.forward_speed), desiredVelocityLength, glideControl.forward_speed);

	Vec3 velocity = (!desiredVelocity.IsZero()) ? desiredVelocity.GetNormalized() * desiredVelocityLength : desiredVelocity;
	velocity.z = -LERP(glideControl.min_falling_speed, glideControl.max_falling_speed, powf(fallingHeightFactor, 2.0f));

	return velocity;
}

//------------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CPlayerMovement::ProcessSliding()
{
	const SPlayerSlideControl& slideCvars = g_pGameCVars->pl_sliding_control;

	const float frameTime = gEnv->pTimer->GetFrameTime();
	const float decceleration = (m_player.GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power) ?
																	-slideCvars.deceleration_speed_powerMode : -slideCvars.deceleration_speed; // In m/s

	const Vec3  fowardDir = m_baseQuat.GetColumn1();
	
	float groundSlopeDeg = 0.0f;
	float  downHillAccel = 0.0f;

	if(m_stats.groundNormal.IsUnit())
	{
		const Vec3  normalXY(m_stats.groundNormal.x, m_stats.groundNormal.y, 0.0f);
		const float cosine	= fowardDir.Dot(normalXY);

		groundSlopeDeg = RAD2DEG(acos_tpl( clamp(cosine, -1.0f, 1.0f ))) - 90.0f;

		// groundSlopeDeg tells us if going down hill (negative value) or up hill (positive value)
		if(groundSlopeDeg < -g_pGameCVars->pl_sliding_control.min_downhill_threshold)
		{ 
			const float maxDiff = slideCvars.max_downhill_threshold - slideCvars.min_downhill_threshold;
			const float currentDiff = fabs_tpl(groundSlopeDeg) - slideCvars.min_downhill_threshold; 
			downHillAccel = (float)__fsel(-maxDiff, slideCvars.max_downhill_acceleration, clamp(currentDiff/maxDiff, 0.0f, 1.0f) * slideCvars.max_downhill_acceleration);
		}
	}

	Vec3 velocity = ((fowardDir) * max(m_stats.speedFlat + ((decceleration + downHillAccel)*frameTime), 0.0f));
	velocity += Vec3(0.0f, 0.0f, m_stats.velocity.z) + (m_stats.gravity * frameTime);

	m_request.type = eCMT_Fly;
	m_request.velocity = velocity;

	if (m_stats.slideStats.slidingState == SPlayerStats::eSS_Sliding)
	{
		m_player.GetAnimationGraphState()->SetInput("Action", "slide");	

		if (m_player.IsClient() && !ShouldContinueSliding())
		{
			m_player.SetSlideState(SPlayerStats::eSS_ExitingSlide);
		}
	}
	else
	{
		const float exitSlideThreshold = 0.52f;

		if ((gEnv->pTimer->GetAsyncCurTime() - m_stats.slideStats.slideExitTime) > exitSlideThreshold)
		{
			m_player.SetSlideState(SPlayerStats::eSS_None);
		}
	}

}

//-----------------------------------------------------------------------------------------------

#if defined(XENON) && defined(XENON_INTRINSICS)

XMVECTOR vecRotateNoTrans(const Matrix34A& m, const XMVECTOR& v)
{
	const XMVECTOR combineMask1 = XMVectorSelectControl(0, 1, 1, 1);
	const XMVECTOR combineMask2 = XMVectorSelectControl(0, 0, 1, 1);
	XMVECTOR out;
	XMVECTOR xmv = v;
	XMVECTOR xm0 = m.m0;
	XMVECTOR xm1 = m.m1;
	XMVECTOR xm2 = m.m2;


	XMVECTOR c1 = XMVectorSplatX(XMVector3Dot(xmv, xm0));
	XMVECTOR c2 = XMVectorSplatX(XMVector3Dot(xmv, xm1));
	XMVECTOR combine = XMVectorSelect(c1, c2, combineMask1);
	XMVECTOR c3 = XMVectorSplatX(XMVector3Dot(xmv, xm2));

	
	out = XMVectorSelect(combine, c3, combineMask2);
	return out;
}

#endif

//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessOnGroundOrJumping(CPlayer& player)
{
	//process movement
	Vec3 move(0,0,0);

	const float totalMovement = fabsf(m_movement.desiredVelocity.x) + fabsf(m_movement.desiredVelocity.y);
	const bool isPlayer = m_player.IsPlayer();
	const bool isPlayerInAir = (m_stats.inAir > 0.0f) && (m_stats.onGround < 0.01f) && isPlayer;

	const bool moving = (totalMovement > 0.0f);	

	//TODO: On state change update this, not every frame
	IPhysicalEntity* pPhysEnt = m_player.GetEntity()->GetPhysics();
	if (pPhysEnt != NULL)
	{
		SAnimatedCharacterParams params = m_player.m_pAnimatedCharacter->GetParams();
		pe_player_dynamics pd;
		if (!isPlayerInAir)
		{	
			pd.kAirControl = m_player.GetAirControl();
			pd.kAirResistance = m_player.GetAirResistance();

			params.inertia = m_player.GetInertia();
		}
		else
		{
			pd.kAirControl = m_player.GetAirControl()* g_pGameCVars->pl_jump_control.air_control_scale;
			pd.kAirResistance = m_player.GetAirResistance() * g_pGameCVars->pl_jump_control.air_resistance_scale;

			params.inertia = m_player.GetInertia() * g_pGameCVars->pl_jump_control.air_inertia_scale;
		}

		if (m_player.m_pAnimatedCharacter->GetMCMV() != eMCM_Animation)
		{
			pd.gravity = m_stats.gravity; // NOTE: this might be problematic if UpdateStats is called while under eMCM_Animation.
		}
		else
		{
			pd.gravity.zero();
		}

		pPhysEnt->SetParams(&pd);

		// Let Animated character handle the inertia
		m_player.m_pAnimatedCharacter->SetParams(params);
	}

	if (moving)
	{	
		Vec3 desiredVelocityClamped = m_movement.desiredVelocity;
		const float desiredVelocityMag = desiredVelocityClamped.GetLength();

		const float invDesiredVelocityMag = 1.0f / desiredVelocityMag;
		
		const float strafeMul = m_params.strafeMultiplier;
		float backwardMul = 1.0f;

		desiredVelocityClamped = desiredVelocityClamped * (float)__fsel(-(desiredVelocityMag - 1.0f), 1.0f, invDesiredVelocityMag);

		//going back?
		if (isPlayer)	//[Mikko] Do not limit backwards movement when controlling AI.
    {
			backwardMul = (float)__fsel(desiredVelocityClamped.y, 1.0f, LERP(backwardMul, m_params.backwardMultiplier, -desiredVelocityClamped.y));
		}

		NETINPUT_TRACE(m_player.GetEntityId(), backwardMul);
		NETINPUT_TRACE(m_player.GetEntityId(), strafeMul);

		move += m_baseQuat.GetColumn0() * desiredVelocityClamped.x * strafeMul * backwardMul;
		move += m_baseQuat.GetColumn1() * desiredVelocityClamped.y * backwardMul;
	}

	//ai can set a custom sprint value, so dont cap the movement vector
	if (m_movement.sprint<=0.0f)
	{
		//cap the movement vector to max 1
		float moveModule(move.len());

		//[Mikko] Do not limit backwards movement when controlling AI, otherwise it will disable sprinting.
		if (isPlayer)
		{
			//^^[Stas] Added this hack, other clients are not AIs
			//if ( moveModule > 1.0f) move /= moveModule
			move /= (float)__fsel(-(moveModule - 1.0f), 1.0f, moveModule);
		}

		NETINPUT_TRACE(m_player.GetEntityId(), moveModule);
	}

	//player movement don't need the m_frameTime, its handled already in the physics
	float scale = m_player.GetStanceMaxSpeed(m_player.GetStance());
	move *= scale;
	NETINPUT_TRACE(m_player.GetEntityId(), scale);

  if (isPlayer)
	{
    AdjustMovementForEnvironment( move, (m_actions&ACTION_SPRINT)!=0 );
	}

	//only the Z component of the basematrix, handy with flat speeds,jump and gravity
	Matrix34A baseMtxZ(Matrix34A(m_baseQuat) * Matrix33::CreateScale(Vec3(0,0,1)));
	baseMtxZ.SetTranslation(Vec3(0,0,0));

	m_request.type = eCMT_Normal;

	Vec3 jumpVec(0,0,0);

	//jump?
	
	const bool allowJump = !m_jumped;
	bool isRemoteClient = !gEnv->bServer && !m_player.IsClient();

	if (m_movement.jump)
	{
		bool didJump = allowJump && StartJump(jumpVec, move, baseMtxZ);
		CCCPOINT_IF(!didJump, PlayerMovement_PressJumpWhileNotAllowedToJump);
	}
	
	//apply movement
	Vec3 desiredVel(0,0,0);
	Vec3 entityPos = m_player.GetEntity()->GetWorldPos();
	Vec3 entityRight(m_baseQuat.GetColumn0());

#if defined(XENON)
	XMVECTOR xmDesiredVel = XMVectorZero();
	XMVECTOR xmMove				= XMLoadVector3(&move);	

	if (m_stats.onGround)
	{
		xmDesiredVel = xmMove;

		{ // Shallow water speed slowdown
			float shallowWaterMultiplier = 1.0f;

			if (isPlayer)
				shallowWaterMultiplier = g_pGameCVars->cl_shallowWaterSpeedMulPlayer;
			else
				shallowWaterMultiplier = g_pGameCVars->cl_shallowWaterSpeedMulAI;

			shallowWaterMultiplier = max(shallowWaterMultiplier, 0.1f);
			assert(shallowWaterMultiplier <= 1.0f);


			float shallowWaterDepthSpan = (g_pGameCVars->cl_shallowWaterDepthHi - g_pGameCVars->cl_shallowWaterDepthLo);
			shallowWaterDepthSpan = max(0.1f, shallowWaterDepthSpan);
			float slowdownFraction = (m_stats.relativeBottomDepth - g_pGameCVars->cl_shallowWaterDepthLo) / shallowWaterDepthSpan;
			slowdownFraction = clamp(slowdownFraction, 0.0f, 1.0f);
			shallowWaterMultiplier = LERP(1.0f, shallowWaterMultiplier, slowdownFraction);

			//avoid branch if m_stats.relativeBottomDepth <= 0.0f;
			shallowWaterMultiplier = (float)__fsel(-m_stats.relativeBottomDepth, 1.0f, shallowWaterMultiplier);

			XMVECTOR vfShallowWaterMultiplier = XMVectorReplicate(shallowWaterMultiplier);

			xmDesiredVel = XMVectorMultiply(vfShallowWaterMultiplier, xmDesiredVel);

		}
	}
	else if (move.len2()>0.01f)//"passive" air control, the player can air control as long as it is to decelerate
	{	
		XMVECTOR xmVelocity = XMLoadVector3(&m_stats.velocity);
		const XMVECTOR xmfDiffMultiplier = XMVectorReplicate(0.3f);
		const XMVECTOR xmfMaxDiff = XMVectorReplicate(0.1f);
		const XMVECTOR xmfMaxMove = XMVectorReplicate(1.0f);
		const XMVECTOR xmfMinMove = XMVectorReplicate(0.5f);
		const XMVECTOR xmfOnePointFive = XMVectorReplicate(1.5f);

		XMVECTOR xmMoveFlat		= xmMove - vecRotateNoTrans(baseMtxZ, xmMove);
		XMVECTOR xmCurrVelFlat = xmVelocity - vecRotateNoTrans(baseMtxZ, xmVelocity);

		XMVECTOR xmMoveFlatNormalized		= XMVector3Normalize(xmMoveFlat);
		XMVECTOR xmCurDirFlatNormalized	= XMVector3Normalize(xmCurrVelFlat);

		XMVECTOR xmfDot = XMVector3Dot(xmMoveFlatNormalized, xmCurDirFlatNormalized);

 		XMVECTOR xmScaledMoveFlat = XMVectorMultiply(xmMoveFlat, XMVectorClamp(xmfDot, xmfMinMove, xmfMaxMove));
		XMVECTOR xmReducedMove = XMVectorSubtract(xmMoveFlat, xmCurrVelFlat) * XMVectorMax(XMVectorAbs(xmfDot) * xmfDiffMultiplier, xmfMaxDiff);
 		xmDesiredVel = XMVectorSelect(	xmScaledMoveFlat,
 																		xmReducedMove,
 																		XMVectorLess( xmfDot, XMVectorZero()));

		XMVECTOR xmfCurrVelSizeSq = XMVector3LengthSq(xmCurrVelFlat);
		XMVECTOR xmfDesiredVelSizeSq = XMVector3LengthSq(xmDesiredVel);

		xmDesiredVel = XMVectorSelect( XMVector3Normalize(xmDesiredVel) * XMVectorMax( xmfOnePointFive, XMVectorSqrt(xmfCurrVelSizeSq)), xmDesiredVel, XMVectorLess( xmfDesiredVelSizeSq, xmfCurrVelSizeSq));
	}

	XMVec4::StoreVec3(&desiredVel, xmDesiredVel);
#else
	if (m_stats.onGround)
	{
		desiredVel = move;

		{ // Shallow water speed slowdown
			float shallowWaterMultiplier = 1.0f;
			
			if (isPlayer)
			{
				shallowWaterMultiplier = g_pGameCVars->cl_shallowWaterSpeedMulPlayer;
			}
			else
			{
				shallowWaterMultiplier = g_pGameCVars->cl_shallowWaterSpeedMulAI;
			}

			shallowWaterMultiplier = max(shallowWaterMultiplier, 0.1f);
			assert(shallowWaterMultiplier <= 1.0f);

			float shallowWaterDepthSpan = (g_pGameCVars->cl_shallowWaterDepthHi - g_pGameCVars->cl_shallowWaterDepthLo);
			shallowWaterDepthSpan = max(0.1f, shallowWaterDepthSpan);
			float slowdownFraction = (m_stats.relativeBottomDepth - g_pGameCVars->cl_shallowWaterDepthLo) / shallowWaterDepthSpan;
			slowdownFraction = clamp(slowdownFraction, 0.0f, 1.0f);
			shallowWaterMultiplier = LERP(1.0f, shallowWaterMultiplier, slowdownFraction);

			//avoid branch if m_stats.relativeBottomDepth <= 0.0f;
			shallowWaterMultiplier = (float)__fsel(-m_stats.relativeBottomDepth, 1.0f, shallowWaterMultiplier);

			desiredVel *= shallowWaterMultiplier;
		}
	}
	else if (move.len2()>0.01f)//"passive" air control, the player can air control as long as it is to decelerate
	{	
		Vec3 currVelFlat(m_stats.velocity - (baseMtxZ * m_stats.velocity));
		Vec3 moveFlat(move - ( baseMtxZ * move ));

		float dot(currVelFlat.GetNormalizedSafe(ZERO) * moveFlat.GetNormalizedSafe(ZERO));

		float curVelFlatMultiplier	= (float)__fsel(dot, 0.0f, -1.0f);
		float multiplier						=	(float)__fsel(dot, clamp(dot, 0.5f, 1.0f), max(fabsf(dot)*0.3f, 0.1f));

		desiredVel = (moveFlat + (curVelFlatMultiplier * currVelFlat)) * multiplier;

		float currVelModSq(currVelFlat.len2());
		float desiredVelModSq(desiredVel.len2());

		if (desiredVelModSq>currVelModSq)
		{
			desiredVel.Normalize();
			desiredVel *= max(1.5f,sqrtf(currVelModSq));
		}

	}
#endif

	// Slow down on sloped terrain, simply proportional to the slope. 
	desiredVel *= (float)__fsel(-(m_stats.onGround), 1.0f, m_stats.groundNormal.z);

	//be sure desired velocity is flat to the ground
	desiredVel -= baseMtxZ * desiredVel;

#if !defined(PS3) && !defined(XENON) && !defined(_RELEASE)
	bool debugJumping = (g_pGameCVars->pl_debug_jumping != 0);
#endif

	Vec3 modifiedSlopeNormal = m_stats.groundNormal;
	if (isPlayer)
	{
		float h = Vec2(modifiedSlopeNormal.x, modifiedSlopeNormal.y).GetLength(); // TODO: OPT: sqrt(x*x+y*y)
		float v = modifiedSlopeNormal.z;
		float slopeAngleCur = RAD2DEG(cry_atan2f(h, v));

		const float divisorH = (float)__fsel(-h, 1.0f, h);
		const float divisorV = (float)__fsel(-v, 1.0f, v);
		
		const float invV = __fres(divisorV);
		const float invH = __fres(divisorH);
		
		const float slopeAngleHor = 10.0f;
		const float slopeAngleVer = 50.0f;
		float slopeAngleFraction = clamp((slopeAngleCur - slopeAngleHor) * __fres(slopeAngleVer - slopeAngleHor), 0.0f, 1.0f);
		
		slopeAngleFraction = slopeAngleFraction * slopeAngleFraction * slopeAngleFraction;

		float slopeAngleMod = LERP(0.0f, 90.0f, slopeAngleFraction);

		float s, c;

		cry_sincosf(DEG2RAD(slopeAngleMod), &s, &c);

		const float hMultiplier = (float)__fsel(-h, 1.0f, s * invH);
		const float vMultiplier = (float)__fsel(-v, 1.0f, c * invV);

		modifiedSlopeNormal.x *= hMultiplier;
		modifiedSlopeNormal.y *= hMultiplier;
		modifiedSlopeNormal.z *= vMultiplier;

		//Normalize the slope normal if possible
		const float fSlopeNormalLength = modifiedSlopeNormal.len();
		const float fSlopeNormalLengthSafe = (float)__fsel(fSlopeNormalLength - 0.000001f, fSlopeNormalLength, 1.0f);
		modifiedSlopeNormal = modifiedSlopeNormal * __fres(fSlopeNormalLengthSafe);
		
		float alignment = min(modifiedSlopeNormal * desiredVel, 0.0f);

		// Also affect air control (but not as much), to prevent jumping up against steep slopes.

		alignment *= (float)__fsel(-fabsf(m_stats.onGround), LERP(0.7f, 1.0f, 1.0f - clamp(modifiedSlopeNormal.z * 100.0f, 0.0f, 1.0f)), 1.0f);
		
		//While jumping ensure we don't add Z velocity
		modifiedSlopeNormal.z = m_stats.jumped ? 0.0f : modifiedSlopeNormal.z;

		desiredVel -= modifiedSlopeNormal * alignment;


#if !defined(PS3) && !defined(XENON) && !defined(_RELEASE)
		if (debugJumping)
		{
			m_player.DebugGraph_AddValue("GroundSlope", slopeAngleCur);
			m_player.DebugGraph_AddValue("GroundSlopeMod", slopeAngleMod);
		}
#endif
	}


	Vec3 newVelocity = m_stats.velocity;

	NETINPUT_TRACE(m_player.GetEntityId(), jumpVec);

	bool airFrictionPerkActivated = m_player.IsAirFrictionActive();

	Vec3 jumpExtraForce(0,0,0);
	
	if (airFrictionPerkActivated)
	{
		desiredVel = CalculateVelocityWithAirFrictionPerkActivated(desiredVel);
	}
	else if (isPlayerInAir && m_stats.jumped)
	{
		jumpExtraForce = CalculateInAirJumpExtraVelocity(desiredVel);
		if(newVelocity.z < 0.0f)
		{
			m_player.SendPerkEvent(EPE_OverrideFallVelocity, &jumpExtraForce);
		}
	}

	if (m_movement.jump && allowJump)
		newVelocity = jumpVec;
	else
		newVelocity = desiredVel + jumpExtraForce;
	
	const float fNewSpeed = newVelocity.len();

	const float fVelocityMultiplier = (float)__fsel(fNewSpeed - 22.0f, __fres(fNewSpeed+FLT_EPSILON) * 22.0f, 1.0f);

	m_request.velocity = newVelocity * m_stats.flashBangStunMult * fVelocityMultiplier;
	
#if !defined(PS3) && !defined(XENON) && !defined(_RELEASE)
	if(g_pGameCVars->pl_debug_movement > 0)
	{
		float white[] = {1.0f,1.0f,1.0f,1.0f};
		gEnv->pRenderer->Draw2dLabel(20, 450, 2.0f, white, false, "Speed: %.3f m/s", m_request.velocity.len());

		if(g_pGameCVars->pl_debug_movement > 1)
		{
			const SNanoSuitGameParameters& suitParams = m_player.GetActorSuitGameParameters();
			gEnv->pRenderer->Draw2dLabel(35, 470, 1.8f, white, false, "Stance Speed: %.3f m/s\nSuit Mode speedScale: %.3f\nSuit Mode sprintSpeedScale: %.3f (%sSprinting)", scale, suitParams.GetProps().speedScale, suitParams.GetProps().sprintSpeedScale, m_stats.bSprinting ? "" : "Not ");
		}		
	}

	if (debugJumping)
	{
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos, ColorB(255,255,255,255), entityPos + modifiedSlopeNormal, ColorB(255,255,0,255), 2.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos+Vec3(0,0,2), ColorB(255,255,255,255), entityPos+Vec3(0,0,2) + desiredVel, ColorB(0,255,0,255), 2.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos, ColorB(255,255,255,255), entityPos + jumpVec, ColorB(0,255,255,255), 2.0f);
		gEnv->pRenderer->DrawLabel(entityPos - entityRight * 1.0f + Vec3(0,0,3.0f), 1.5f, "Velo[%2.3f = %2.3f, %2.3f, %2.3f]", m_request.velocity.len(), m_request.velocity.x, m_request.velocity.y, m_request.velocity.z);
	}
#endif

	m_velocity.Set(0,0,0);
}

bool CPlayerMovement::StartJump(Vec3& jumpVec, Vec3& move, const Matrix34& baseMtxZ )
{
	const float onGroundTime = 0.2f;
	bool allow = (m_stats.onGround > onGroundTime);

	if (allow)
	{
		const SNanoSuitGameParameters& suitParams = m_player.GetActorSuitGameParameters();
		const SNanoSuitProperties& suitProps = suitParams.GetProps();

		float g = m_stats.gravity.len();

		const float jumpHeightScale = (m_movement.strengthJump) ? suitProps.superJumpScale : 1.0f;
		const float jumpHeight = m_params.jumpHeight * jumpHeightScale;
		
		float jumpSpeed = 0.0f;

		if (g > 0.0f)
		{
			jumpSpeed = (cry_sqrtf(2.0f*jumpHeight*(1.0f/g))  - (m_stats.inAir*0.5f)) * g;
		}

		//this is used to easily find steep ground
		float slopeDelta = (m_stats.upVector - m_stats.groundNormal).len();

		m_player.GetAnimationGraphState()->SetInput("Action","jumpMP");

		bool bNormalJump = true;
		if(m_movement.strengthJump)
		{
			CALL_PLAYER_EVENT_LISTENERS(OnSpecialMove(&m_player, IPlayerEventListener::eSM_StrengthJump));
			m_player.GetAnimationGraphState()->SetInput("Action","jumpMPStrength");

			//Inform AI system
			if (IAIObject* pIAObj = m_player.GetEntity()->GetAI())
				pIAObj->Event(AIEVENT_PLAYER_STUNT_JUMP, 0);

			SNanoSuitEvent suitEvent;
			suitEvent.event = eNanoSuitEvent_POWERJUMP;
			suitEvent.bParam = m_player.IsPerkActive(ePerk_ManeuverMKII) ? true : false;
			m_player.SendActorSuitEvent(suitEvent);
			m_player.SetSuperJumping(true);

			CCCPOINT_IF(m_player.IsClient(),   PlayerMovement_LocalPlayerStrengthJump);
			CCCPOINT_IF(! m_player.IsClient(), PlayerMovement_NonLocalPlayerStrengthJump);
		}
		else
		{
			m_player.PlaySound(CPlayer::ESound_Jump);
			m_player.PlayBreathingSound( CPlayer::EActionSoundParam_Jump );
			CALL_PLAYER_EVENT_LISTENERS(OnSpecialMove(&m_player, IPlayerEventListener::eSM_Jump));

			CCCPOINT_IF(m_player.IsClient(),   PlayerMovement_LocalPlayerNormalJump);
			CCCPOINT_IF(! m_player.IsClient(), PlayerMovement_NonLocalPlayerNormalJump);
		}

		m_player.SendPerkEvent(EPE_Jump);

		{
			// This was causing the vertical jumping speed to be much slower.
			float verticalMult = max(1.0f - m_stats.jumpLock, 0.3f);

			m_request.type = eCMT_JumpInstant;
			jumpVec += m_baseQuat.GetColumn2() * jumpSpeed * verticalMult;

			if (m_stats.groundNormal.len2() > 0.0f)
			{
				float vertical = clamp((m_stats.groundNormal.z - 0.25f) / 0.5f, 0.0f, 1.0f);
				Vec3 modifiedJumpDirection = LERP(m_stats.groundNormal, Vec3(0,0,1), vertical);
				jumpVec = modifiedJumpDirection * jumpVec.len();
			}

		}

		// Don't speed up...
		move -= baseMtxZ * move;

		m_jumped = true;
		m_hasJumped = true;

		if (m_player.IsClient())
		{
			m_player.HasJumped();
		}
	}

	return allow;
}

//////////////////////////////////////////////////////////////////////////
const Vec3 CPlayerMovement::CalculateInAirJumpExtraVelocity(const Vec3& desiredVelocity) const
{
	const float speedUpFactor = 0.175f;

	Vec3 jumpExtraVelocity(0.0f, 0.0f, 0.0f);

	if (m_stats.velocity.z > 0.0f)
	{
		//Note: Desired velocity is flat (not 'z' component), so jumpHeight should not be altered
		jumpExtraVelocity = desiredVelocity * speedUpFactor;
	}
	else
	{
		//Note: this makes the jump feel less 'floaty', by accelerating the player slightly down
		//      and compensates the extra traveled distance when going up 
		const float g = m_stats.gravity.len();
		if (g > 0.0f)
		{
			const SNanoSuitGameParameters& suitParams = m_player.GetActorSuitGameParameters();
			const SNanoSuitProperties& suitProps = suitParams.GetProps();

			const float jumpHeightScale = (m_player.IsSuperJumping()) ? suitProps.superJumpScale : 1.0f;
			const float jumpHeight = m_params.jumpHeight * jumpHeightScale;

			const float estimatedLandTime = cry_sqrtf(2.0f*jumpHeight*(1.0f/g)) * (1.0f - speedUpFactor);
			assert(estimatedLandTime > 0.0f);
			if (estimatedLandTime > 0.0f)
			{
				const float requiredGravity = (2.0f*jumpHeight)/(estimatedLandTime * estimatedLandTime);
				const float initialAccelerationScale = clamp((-m_stats.velocity.z * 0.6f), 0.0f, 1.0f);
				jumpExtraVelocity = (requiredGravity - g) * m_stats.gravity.GetNormalized() * initialAccelerationScale;
			}
		}
	}

	return jumpExtraVelocity;
}

//-----------------------------------------------------------------------------------------------
void CPlayerMovement::AdjustMovementForEnvironment( Vec3& move, bool sprinting )
{
	//nanoSuit
	const SNanoSuitGameParameters& suitParams = m_player.GetActorSuitGameParameters();
	CItem * pItem = static_cast<CItem*>(m_player.GetCurrentItem());
	bool bigWeaponRestrict = m_player.HasHeavyWeaponEquipped(pItem) && (suitParams.GetMode() != eNanoSuitMode_Power);

	float mult = (bigWeaponRestrict) ? g_pGameCVars->pl_movement.nonCombat_heavy_weapon_speed_scale : suitParams.GetProps().speedScale;

	if(m_stats.bSprinting)
	{
		mult *= (!bigWeaponRestrict) ? suitParams.GetProps().sprintSpeedScale : g_pGameCVars->pl_movement.nonCombat_heavy_weapon_sprint_scale;
	}

	move *= mult;

	// adjust for cover and lean mechanic
	m_player.GetCoverAndLean().AdjustMovement(&move);
}

//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessTurning()
{
	if (m_stats.isRagDoll)
		return;

	Quat entityRot = m_player.GetEntity()->GetWorldRotation().GetNormalized();
	Quat inverseEntityRot = entityRot.GetInverted();

	m_request.rotation = inverseEntityRot * m_baseQuat;
	m_request.rotation.Normalize();

	bool doProceduralLean = (g_pGameCVars->ac_enableProceduralLeaning > 0.0f) && m_player.IsThirdPerson();

	if (m_player.IsPlayer() && (g_pGameCVars->ca_GameControlledStrafingPtr->GetIVal() != 0) && !doProceduralLean)
	{
		float turningSpeed = m_frameTime > 0.0f ? (fabs(RAD2DEG(m_request.rotation.GetRotZ())) / m_frameTime) : 0.0f;
		float turningSpeedMin = 30.0f;
		float turningSpeedMax = 180.0f;
		float turningSpeedFraction = CLAMP((turningSpeed - turningSpeedMin) / (turningSpeedMax - turningSpeedMin), 0.0f, 1.0f);
		float travelSpeedScale = LERP(1.0f, CLAMP(g_pGameCVars->pl_curvingSlowdownSpeedScale, 0.0f, 1.0f), turningSpeedFraction);
		m_request.velocity *= travelSpeedScale;
	}

	m_request.proceduralLeaning = (doProceduralLean ? m_params.proceduralLeaningFactor  : 0.0f);

/*
	Vec3 pos = m_player.GetEntity()->GetWorldPos();
	Vec3 curDir = entityRot.GetColumn1();
	Vec3 wantDir = m_baseQuat.GetColumn1();
	Vec3 lftDir = entityRot.GetColumn0();
	float rot = m_request.rotation.GetRotZ();
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags( e_Def3DPublicRenderflags );
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, ColorB(255,255,0,255), pos+curDir, ColorB(255,255,0,255), 2.0f);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, ColorB(255,0,255,255), pos+wantDir, ColorB(255,0,255,255), 2.0f);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos+curDir, ColorB(0,255,255,255), pos+curDir+lftDir*rot, ColorB(0,255,255,255), 2.0f);
*/
}


//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
// ledge functions
//-----------------------------------------------------------------------------------------------

void CPlayerMovement::ProcessMovementOnLedge()
{
	SPlayerStats::SLedgeBlending& ledgeBlending = m_player.m_stats.ledgeBlending;
	IEntity* pEntity = m_player.GetEntity();

	ledgeBlending.m_runningTimer -= m_frameTime;
	ledgeBlending.m_correctionTimer = clamp(ledgeBlending.m_correctionTimer - m_frameTime, 0.0f, ledgeBlending.m_correctionTimer);

	if (ledgeBlending.m_runningTimer > 0.0f)
	{
		IEntity* pEntity = m_player.GetEntity();

		QuatT qtCurLoc(pEntity->GetWorldPos(), pEntity->GetWorldRotation());

		QuatT qtCorrection(IDENTITY, ZERO);
		if (ledgeBlending.m_correctionTimer > 0.0f)
		{
			//Correct while playing the pull up animation (no transition for smoothness)
			const float fScale = clamp((m_frameTime/ledgeBlending.m_correctionTimer), 0.0f, 1.0f);
			const QuatT qtIdentity(IDENTITY, ZERO);
			qtCorrection.SetNLerp(qtIdentity, ledgeBlending.m_qtCorrection, fScale);
			ledgeBlending.m_qtCorrection.SetNLerp(ledgeBlending.m_qtCorrection, qtIdentity, fScale);
		}

		const QuatT& relativeAnimMovement = m_player.GetAnimationRelativeMovement();
		m_request.rotation = relativeAnimMovement.q * qtCorrection.q;
		m_request.velocity = ((qtCurLoc.q * relativeAnimMovement.t) + qtCorrection.t) / m_frameTime;
		m_request.rotation.Normalize();
		m_request.type = eCMT_Fly;

	}
	else
	{
		ledgeBlending.m_runningTimer = 0.0f;
		m_player.GrabOntoLedge(false, SPlayerStats::eOLT_None);
	}
}



//----------------------------------------------------------
//----------------------------------------------------------
bool CPlayerMovement::IsMovingForward() const
{
	const float thresholdSinAngle = sin_tpl((3.141592f * 0.5f) - DEG2RAD(g_pGameCVars->pl_power_sprint.foward_angle));
	const float currentSinAngle = m_movement.desiredVelocity.y;
	bool movingForward = (currentSinAngle > thresholdSinAngle);

	return movingForward;
}

bool CPlayerMovement::ShouldContinueSliding() const
{
	bool shouldContinueSliding = (m_stats.speedFlat > g_pGameCVars->pl_sliding_control.min_speed) && (m_stats.onGround > 0.0f) ;

	return (shouldContinueSliding && IsMovingForward());
}

bool CPlayerMovement::ShouldProcessTurning() const
{
	return (m_player.m_linkStats.CanRotate()) && (m_player.IsOnLedge() == false); 
}

void CPlayerMovement::AddVelocity( const Vec3 &velocityDelta )
{
	m_velocity += velocityDelta;
	m_request.velocity += velocityDelta;
}
