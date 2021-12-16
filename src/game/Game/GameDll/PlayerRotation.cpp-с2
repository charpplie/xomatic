#include "StdAfx.h"
#include "PlayerRotation.h"
#include "GameUtils.h"
#include "GameCVars.h"
#include "Item.h"
#include "Weapon.h"
#include "GameRules.h"
#include <IPlayerInput.h>
#include "PlayerInput.h"

#if !defined(_RELEASE) && !defined(PS3)
	#define ENABLE_NAN_CHECK
#endif

#ifdef ENABLE_NAN_CHECK
#define CHECKQNAN_FLT(x) \
	assert(((*(unsigned*)&(x))&0xff000000) != 0xff000000u && (*(unsigned*)&(x) != 0x7fc00000))
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#define CHECKQNAN_VEC(v) \
	CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)

/*#define CHECKQNAN_MAT33(v) \
	CHECKQNAN_VEC(v.GetColumn(0)); \
	CHECKQNAN_VEC(v.GetColumn(1)); \
	CHECKQNAN_VEC(v.GetColumn(2))
*/

#define CHECKQNAN_QUAT(q) \
	CHECKQNAN_VEC(q.v); \
	CHECKQNAN_FLT(q.w)

int8 CPlayerRotation::s_lastAimSnapId = 0;
bool CPlayerRotation::s_lastTimeRequestedSnapping = false;

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
CPlayerRotation::CPlayerRotation(
		const CPlayer& player, const SActorFrameMovementParams& movement, const SAimAccelerationParams& horizontalAcceleration,
		const SAimAccelerationParams& verticalAcceleration, float m_frameTime ) : 
	m_frameTime(m_frameTime),
	m_params(player.m_params),
	m_stats(player.m_stats),
	m_viewAngles(player.m_viewAngles),
	m_viewQuat(player.m_viewQuat),
	m_baseQuat(player.m_baseQuat),
	m_viewQuatFinal(player.m_viewQuatFinal),
	m_baseQuatLinked(player.m_linkStats.baseQuatLinked),
	m_viewQuatLinked(player.m_linkStats.viewQuatLinked),
	m_player(player),
	m_viewRoll(player.m_viewRoll),
	m_upVector(player.m_upVector),
	m_viewAnglesOffset(player.GetViewAngleOffset()),
	m_leanAmount(player.m_stats.leanAmount),
	m_peekOverAmount(player.m_stats.peekOverAmount),
	m_actions(player.m_actions),	  
	m_angularImpulseTime(m_stats.angularImpulseTime),
	m_angularImpulse(m_stats.angularImpulse),
	m_angularImpulseDelta(m_stats.angularImpulse),
	m_angularVel(m_stats.angularVel),
	m_desiredLeanAmount(movement.desiredLean),
	m_desiredPeekOverAmount(movement.desiredPeekOver),
	m_follow_target_id(player.m_stats.follow_target_id),
	m_follow_target_dir(player.m_stats.follow_target_dir),
	m_snap_target_dir(player.m_stats.snap_target_dir),
	m_horizontalAcceleration(horizontalAcceleration),
	m_verticalAcceleration(verticalAcceleration),
	m_desiredAimSnapId(0)
{
	m_deltaAngles = movement.deltaAngles;
	m_desiredVelocity = movement.desiredVelocity;
	CHECKQNAN_FLT(m_deltaAngles);
}

void CPlayerRotation::Process()
{
	//FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	//
	float forceLookLen2(m_player.m_stats.forceLookVector.len2());
	if (forceLookLen2>0.001f)
	{
		float forceLookLen(cry_sqrtf(forceLookLen2));
		Vec3 forceLook(m_player.m_stats.forceLookVector);
		forceLook *= 1.0f/forceLookLen;
		forceLook = m_player.m_viewQuatFinal.GetInverted() * forceLook;

		float smoothSpeed(6.6f * forceLookLen);

		const float blendAmount = min(1.0f,m_frameTime*smoothSpeed);
		m_deltaAngles.x += asinf(forceLook.z) * blendAmount;
		m_deltaAngles.z += cry_atan2f(-forceLook.x,forceLook.y) * blendAmount;

		CHECKQNAN_VEC(m_deltaAngles);
	}

	ProcessAngularImpulses();

	ProcessLeanAndPeek();
	
	if (m_stats.slideStats.slidingState == SPlayerStats::eSS_Sliding)
	{
		ProcessSliding();
	}
	else
	{
		ProcessNormalRoll();
		ClampAngles();
		ProcessNormal();
	}

	//CHECKQNAN_MAT33(m_viewMtx);

	//update freelook when linked to an entity
	SLinkStats *pLinkStats = &m_player.m_linkStats;
	if (pLinkStats->linkID)
	{
		IEntity *pLinked = pLinkStats->GetLinked();
		if (pLinked)
		{
			//at this point m_baseQuat and m_viewQuat contain the previous frame rotation, I'm using them to calculate the delta rotation.
			m_baseQuatLinked *= m_player.m_baseQuat.GetInverted() * m_baseQuat;
			m_viewQuatLinked *= m_player.m_viewQuat.GetInverted() * m_viewQuat;

			const Quat qLinkedRotation = pLinked->GetRotation();

			m_baseQuat = qLinkedRotation * m_baseQuatLinked;
			m_viewQuat = qLinkedRotation * m_viewQuatLinked;
		}
	}
	//

	m_viewQuatFinal = m_viewQuat * Quat::CreateRotationXYZ(m_player.GetViewAngleOffset());
}

void CPlayerRotation::Commit( CPlayer& player )
{
	player.m_baseQuat = m_baseQuat.GetNormalized();
	player.m_viewQuat = m_viewQuat.GetNormalized();
	player.m_viewAngles = m_viewAngles;

	CHECKQNAN_QUAT(m_baseQuat);
	CHECKQNAN_QUAT(m_viewQuat);
	if (!player.IsTimeDemo())
	{
		//CHECKQNAN_MAT33(m_baseMtx);
		//CHECKQNAN_MAT33(m_viewMtx);
		player.m_viewQuatFinal = m_viewQuatFinal.GetNormalized();
	}

	player.m_linkStats.baseQuatLinked = m_baseQuatLinked.GetNormalized();
	player.m_linkStats.viewQuatLinked = m_viewQuatLinked.GetNormalized();
	
	player.m_viewRoll = m_viewRoll;
	player.m_upVector = m_upVector;
	//player.m_viewAnglesOffset = m_viewAnglesOffset;
	player.m_stats.leanAmount = m_leanAmount;	
	player.m_stats.peekOverAmount = m_peekOverAmount;
	player.m_stats.angularImpulseTime = m_angularImpulseTime;
	player.m_stats.angularImpulse = m_angularImpulse;
	player.m_stats.angularVel = m_angularVel;
	player.m_stats.follow_target_id = m_follow_target_id;
	player.m_stats.follow_target_dir = m_follow_target_dir;
	player.m_stats.snap_target_dir = m_snap_target_dir;


	player.m_stats.forceLookVector.zero();

	player.ResetFrameAngleViewOffset();
}

void CPlayerRotation::GetStanceAngleLimits(float & minAngle,float & maxAngle)
{
	float fNewMinAngle = m_verticalAcceleration.angle_min;
	float fNewMaxAngle = m_verticalAcceleration.angle_max;

	if(m_stats.pickAndThrowEntity !=0)
	{
		fNewMinAngle = -35.0f;  //Limit angle to prevent clipping, throw objects at feet, etc...
	}
	
	minAngle = DEG2RAD(fNewMinAngle);
	maxAngle = DEG2RAD(fNewMaxAngle);
}


//---------------------------------------------------------------
void CPlayerRotation::ProcessNormalRoll()
{
	//apply lean/roll
	float rollAngleGoal(0);
	float speed2(m_stats.velocity.len2());


	if (speed2 > 0.01f && m_stats.inAir)
	{
		Vec3 velVec(m_stats.velocity);
		float maxSpeed = m_player.GetStanceMaxSpeed(STANCE_STAND);
		if (maxSpeed > 0.01f)
			velVec /= maxSpeed;

		float dotSide(m_viewQuat.GetColumn0() * velVec);

		rollAngleGoal -= dotSide * 1.5f * gf_PI/180.0f;
	}

	float tempLean = m_leanAmount;
	if(m_player.IsPlayer())
	{	
		if(m_player.IsThirdPerson())
		{
			// Scale down player leaning by 50% when moving in 3rd person.
			float movementFraction = min(1.0f, speed2 / sqr(2.0f));
			tempLean *= 1.0f - movementFraction * 0.5f;
		}
		else
		{
			tempLean *= 0.3f;
		}		
	}
	
	const float leanAmoutMultiplier = 3.0f;
	float leanAmount = clamp(tempLean * leanAmoutMultiplier, -1.0f, 1.0f);
	rollAngleGoal += leanAmount * m_params.leanAngle * gf_PI/180.0f;
	Interpolate(m_viewRoll,rollAngleGoal,9.9f,m_frameTime);

	Interpolate(m_angularVel,Ang3(0,0,0),6.6f,m_frameTime);
	m_deltaAngles += m_stats.angularVel + m_angularImpulseDelta;

	//Restore leanAmount if reseted
}

void CPlayerRotation::ProcessAngularImpulses()
{
	//update angular impulse
	if (m_angularImpulseTime>0.001f)
	{
		m_angularImpulse *= min(m_angularImpulseTime / m_stats.angularImpulseTimeMax,1.0f);
		m_angularImpulseTime -= m_frameTime;
	}
	else if (m_stats.angularImpulseDeceleration>0.001f)
	{
		Interpolate(m_angularImpulse,ZERO,m_stats.angularImpulseDeceleration, m_frameTime);
	}
	m_angularImpulseDelta -= m_angularImpulse;
}

void CPlayerRotation::ClampAngles()
{
	{
		//cap up/down looking
		float minAngle,maxAngle;
		GetStanceAngleLimits(minAngle,maxAngle);

		float currentViewPitch=GetLocalPitch();
		float newPitch = clamp(currentViewPitch + m_deltaAngles.x, minAngle, maxAngle);
		m_deltaAngles.x = newPitch - currentViewPitch;
	}

	{
		//further limit the view if necessary
		float limitV = m_params.vLimitRangeV;
		float limitH = m_params.vLimitRangeH;
		Vec3  limitDir = m_params.vLimitDir;
		float limitVUp = m_params.vLimitRangeVUp;
		float limitVDown = m_params.vLimitRangeVDown;

		if ((limitH+limitV+fabsf(limitVUp)+fabsf(limitVDown)) && limitDir.len2()>0.1f)
		{
			//A matrix is built around the view limit, and then the player view angles are checked with it.
			//Later, if necessary the upVector could be made customizable.
			Vec3 forward(limitDir);
			Vec3 up(m_baseQuat.GetColumn2());
			Vec3 right(-(up % forward));
			right.Normalize();

			Matrix33 limitMtx;
			limitMtx.SetFromVectors(right,forward,right%forward);
			limitMtx.Invert();

			Vec3 localDir(limitMtx * m_viewQuat.GetColumn1());

			Ang3 limit;

			if (limitV)
			{
				limit.x = asinf(localDir.z) + m_deltaAngles.x;

				float deltaX(limitV - fabs(limit.x));

				m_deltaAngles.x = m_deltaAngles.x + (float)__fsel(deltaX, 0.0f, deltaX * (float)__fsel(limit.x, 1.0f, -1.0f));
			}

			if (limitVUp || limitVDown)
			{
				limit.x = asinf(localDir.z) + m_deltaAngles.x;

				float deltaXUp(limitVUp - limit.x);
				float fNewDeltaX = m_deltaAngles.x;

				const float fDeltaXUpIncrement = (float)__fsel( deltaXUp, 0.0f, deltaXUp);
				fNewDeltaX = fNewDeltaX + (float)__fsel(-fabsf(limitVUp), 0.0f, fDeltaXUpIncrement);

				float deltaXDown(limitVDown - limit.x);

				const float fDeltaXDownIncrement = (float)__fsel( deltaXDown, deltaXDown, 0.0f);
				fNewDeltaX = fNewDeltaX + (float)__fsel(-fabsf(limitVDown), 0.0f, fDeltaXDownIncrement);

				m_deltaAngles.x = fNewDeltaX;
			}

			if (limitH)
			{
				limit.z = cry_atan2f(-localDir.x,localDir.y) + m_deltaAngles.z;

				float deltaZ(limitH - fabs(limit.z));

				m_deltaAngles.z = m_deltaAngles.z + (float)__fsel(deltaZ, 0.0f, deltaZ * (float)__fsel(limit.z, 1.0f, -1.0f));
			}
		}
	}
}

static void DrawDisc(const Vec3& center, Vec3 axis, float innerRadius, float outerRadius, const ColorB& innerColor, const ColorB& outerColor)
{
	axis.NormalizeSafe(Vec3Constants<float>::fVec3_OneZ);
	Vec3 u = ((axis * Vec3Constants<float>::fVec3_OneZ) > 0.5f) ? Vec3Constants<float>::fVec3_OneY : Vec3Constants<float>::fVec3_OneZ;
	Vec3 v = u.cross(axis);
	u = v.cross(axis);

	float sides = cry_ceilf(3.0f * (float)g_PI * outerRadius);
	//sides = 20.0f;
	float step = 1.0f / sides;
	for (float i = 0.0f; i < 0.99f; i += step)
	{
		float a0 = i * 2.0f * (float)g_PI;
		float a1 = (i+step) * 2.0f * (float)g_PI;
		Vec3 i0 = center + innerRadius * (u*cry_cosf(a0) + v*cry_sinf(a0));
		Vec3 i1 = center + innerRadius * (u*cry_cosf(a1) + v*cry_sinf(a1));
		Vec3 o0 = center + outerRadius * (u*cry_cosf(a0) + v*cry_sinf(a0));
		Vec3 o1 = center + outerRadius * (u*cry_cosf(a1) + v*cry_sinf(a1));
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, innerColor, i1, innerColor, o1, outerColor);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, innerColor, o1, outerColor, o0, outerColor);
	}
}

#define DBG_AUTO_AIM 0

void CPlayerRotation::TargetAimAssistance(float& followH, float& followV, float& scale)
{
	CRY_ASSERT(m_player.IsClient());

	followH = 0.0f;
	followV = 0.0f;
	scale = 1.0f;
	float bestScale = 1.0f;

	IMovementController* pMovementController = m_player.GetMovementController();
	CRY_ASSERT(pMovementController);

	SMovementState moveState;
	pMovementController->GetMovementState(moveState);

	const Vec3 playerFwd = m_viewQuat.GetColumn1();
	const Vec3 playerRgt = m_viewQuat.GetColumn0();
	const Vec3 playerUp = m_viewQuat.GetColumn2();
	const Vec3 playerPos = moveState.eyePosition;

	Vec3 follow_target_pos(ZERO);
	float follow_vote_leader = 0.0f;
	float snap_vote_leader = 0.0f;
	Vec3 follow_target_dir(ZERO);
	Vec3 snap_target_dir(ZERO);
	EntityId follow_target_id = 0;

	CGameRules * pGameRules = g_pGame->GetGameRules();
	float distance_follow_threshold_near	= max(0.0f, g_pGameCVars->aim_assistMinDistance);
	float distance_follow_threshold_far		= max(20.0f, g_pGameCVars->aim_assistMaxDistance);
	int playerTeam = pGameRules->GetTeam(m_player.GetEntity()->GetId());
	float cloakedPlayerMultiplier = g_pGameCVars->pl_aim_cloaked_multiplier;
	const bool multipleTeams = pGameRules->GetTeamCount() > 0;

	const float falloffStartDistance = g_pGameCVars->aim_assistSlowFalloffStartDistance;
	const float falloffPerMeter = 1.0f / (g_pGameCVars->aim_assistSlowDisableDistance - falloffStartDistance);

	const TAutoaimTargets& aaTargets = g_pGame->GetAutoAimManager().GetAutoAimTargets();
	const int targetCount = aaTargets.size();

#if DBG_AUTO_AIM
	SAuxGeomRenderFlags oldFlags = gEnv->pRenderer->GetIRenderAuxGeom()->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone); 
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(newFlags);
#endif

	for (int i = 0; i < targetCount; ++i)
	{
		const SAutoaimTarget& target = aaTargets[i];

		CRY_ASSERT(target.entityId != m_player.GetEntityId());

		//Skip friendly ai
		if(gEnv->bMultiplayer)
		{
			if(multipleTeams &&  (pGameRules->GetTeam(target.entityId) == playerTeam))
			{
				continue;
			}

		}
		else if ((target.HasFlagSet(eAATF_AIHostile) == false))
		{			
			continue;
		}
		
		Vec3 targetPos = target.primaryAimPosition;
		Vec3 targetDistVec = (targetPos - playerPos);
		float distance = targetDistVec.GetLength();
		
		if (distance <= 0.1f)
			continue;

		Vec3 dirToTarget = targetDistVec / distance;

		// fast reject everything behind player, too far away or too near from line of view
		// sort rest by angle to crosshair and distance from player
		float alignment = playerFwd * dirToTarget;
		if (alignment <= 0.0f)
			continue;

		if ((distance < distance_follow_threshold_near) || (distance > distance_follow_threshold_far))
			continue;

		const int kAutoaimVisibilityLatency = 2;
		if (!g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(target.entityId, kAutoaimVisibilityLatency))
		{
			// Since both player and target entities are ignored, and the ray still intersected something, there's something in the way.
			// Need to profile this and see if it's faster to do the below checks before doing the linetest. It's fairly expensive but
			// linetests generally are as well... - Richard
			continue;
		}

#if DBG_AUTO_AIM
		const ColorB green(0,255,0,255);
		const ColorB darkgreen(0,155,0,225);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( playerPos, darkgreen, targetPos, green);
#endif

		const float angle = -(RAD2DEG(cry_acosf(playerRgt.dot(dirToTarget))) - 90.f);
		const float absAngle = fabsf(angle);

		const float angleToTargetV = (RAD2DEG(cry_acosf(playerUp.dot(dirToTarget))) - 90.f);
		const float absAngleV = fabsf(angleToTargetV);

		const float slowModifiedDistance = distance * g_pGameCVars->aim_assistSlowDistanceModifier;
		const float radius_slow_threshold_inner = 0.5f;
		const float radius_slow_threshold_outer = g_pGameCVars->aim_assistSlowThresholdOuter;
		const float angle_slow_threshold_inner = RAD2DEG(cry_atanf(radius_slow_threshold_inner / slowModifiedDistance));
		const float angle_slow_threshold_outer = RAD2DEG(cry_atanf(radius_slow_threshold_outer / slowModifiedDistance));
		float angle_slow_fraction = clamp((absAngle - angle_slow_threshold_inner) / (angle_slow_threshold_outer - angle_slow_threshold_inner), 0.0f, 1.0f);

		const float distance_follow_fraction = clamp((distance - distance_follow_threshold_near) / (distance_follow_threshold_far - distance_follow_threshold_near), 0.0f, 1.0f);
		const float radius_follow_threshold_inner = target.innerRadius;
		const float radius_follow_threshold_outer = target.outerRadius;
		const float radius_snap = target.snapRadius;
		const float angle_follow_threshold_inner = RAD2DEG(cry_atanf(radius_follow_threshold_inner / distance));
		const float angle_follow_threshold_outer = RAD2DEG(cry_atanf(radius_follow_threshold_outer / distance));
		const float angle_follow_fraction = clamp((absAngle - angle_follow_threshold_inner) / (angle_follow_threshold_outer - angle_follow_threshold_inner), 0.0f, 1.0f);
		const float angle_follow_fractionV = clamp((absAngleV - angle_follow_threshold_inner) / (angle_follow_threshold_outer - angle_follow_threshold_inner), 0.0f, 1.0f);
		const float worst_follow_fraction = (float)__fsel(angle_follow_fraction - angle_follow_fractionV, angle_follow_fraction, angle_follow_fractionV);
		float follow_fraction = ((1.0f - worst_follow_fraction) * (1.0f - distance_follow_fraction));
		float follow_vote = follow_fraction;

		IActor* pTargetActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(target.entityId);
		if((pTargetActor != NULL) && (pTargetActor->IsPlayer()))
		{
			CPlayer * pPlayer = static_cast<CPlayer*>(pTargetActor);

			if(pPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Stealth)
			{
				follow_vote *= cloakedPlayerMultiplier;
				follow_fraction *= cloakedPlayerMultiplier;

				const float newSlowAmount = (1.0f - angle_slow_fraction) * cloakedPlayerMultiplier;
				angle_slow_fraction = (1.0f - newSlowAmount);
			}
		}

		//clamp the lower bound of the distance_slow_modifier so it can't be lower than the angle slow fraction
		//  which prevents close but off-centre targets slowing us too much
		const float distance_slow_modifier = clamp( 1.0f - ((distance - falloffStartDistance) * falloffPerMeter), angle_slow_fraction, 1.0f);		

		bestScale = min(angle_slow_fraction * distance_slow_modifier, bestScale);

		if (follow_vote > follow_vote_leader)
		{
			follow_vote_leader = follow_vote;

			//m_follow_target_id only gets set after the loop -> this won't get hit when a target is selected
			// as a follow target for the first time. This doesn't need to be in the loop.
			if (m_follow_target_id == target.entityId)
			{
				Vec3 target_rgt = playerRgt;
				Vec3 target_up = target_rgt.cross(m_follow_target_dir);
				target_rgt = m_follow_target_dir.cross(target_up);
				target_rgt.Normalize();
				target_up.Normalize();
				float alignH = dirToTarget * -target_rgt;
				float alignV = dirToTarget.z - m_follow_target_dir.z;
				float angleH = alignH * 0.75f;
				float angleV = alignV * 0.75f;

				if(fabsf((follow_fraction * angleH) + angle) < absAngle)
				{
					followH = follow_fraction * angleH;
				}
				else
				{
					followH = 0.0f;
				}

				if(fabsf((follow_fraction * angleV) + angleToTargetV) < absAngleV)
				{
					followV = follow_fraction * angleV;
				}
				else
				{
					followV = 0.0f;
				}
				
				follow_vote_leader += 0.05f; // anti oscillation between different targets
				follow_target_pos = targetPos;
			}

			follow_target_id = target.entityId;
			follow_target_dir = dirToTarget;
			snap_target_dir = PickBestSnapDirection(playerPos, playerFwd, dirToTarget, target, (1.0f - worst_follow_fraction));

		}
		else if (!follow_target_id)
		{
			const float angle_snap_threshold = RAD2DEG(cry_atanf(radius_snap / distance));
			const float angle_snap_fraction = clamp((absAngle - angle_follow_threshold_inner) / (angle_snap_threshold - angle_follow_threshold_inner), 0.0f, 1.0f);
			const float angle_snap_fractionV = clamp((absAngleV - angle_follow_threshold_inner) / (angle_snap_threshold - angle_follow_threshold_inner), 0.0f, 1.0f);
			const float worst_snap_fraction = (float)__fsel(angle_snap_fraction - angle_snap_fractionV, angle_snap_fraction, angle_snap_fractionV);
			const float snap_fraction = ((1.0f - worst_snap_fraction) * (1.0f - distance_follow_fraction));

			if (snap_fraction > snap_vote_leader)
			{
				snap_vote_leader = snap_fraction;
				snap_target_dir = PickBestSnapDirection(playerPos, playerFwd, dirToTarget, target, 0.0f);
			}
		}
	}

#if DBG_AUTO_AIM
	if ((!follow_target_pos.IsZeroFast()) && (g_pGameCVars->pl_targeting_debug != 0))
	{
		float radius_inner = 0.30f;
		float radius_outer = 0.33f;
		ColorB colorInner(255,255,0,0x40);
		ColorB colorOuter(255,255,0,0x40);
		DrawDisc(follow_target_pos, follow_target_dir, radius_inner, radius_outer, colorInner, colorOuter);
	}

	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(oldFlags);
#endif

	m_follow_target_id = follow_target_id;
	m_follow_target_dir = follow_target_dir;
	m_snap_target_dir = snap_target_dir;

	const float minTurnScale = g_pGameCVars->aim_assistMinTurnScale;
	scale = minTurnScale + ((1.0f - minTurnScale) * bestScale);

}


void CPlayerRotation::ProcessTargetAssistance()
{
	////////////////////////////////////////////////////////
	//Crytek Nottingham aiming acceleration and autoaim code
	////////////////////////////////////////////////////////

	// aim assistance
	float targetAimAssistAngleFollowH;
	float targetAimAssistAngleFollowV;
	float targetAimAssistAngleScale;

	IPlayerInput * pIPlayerInput = m_player.GetPlayerInput();
	float absInput = 0.0f;

	float aimAssistPowerFactor = 1.0f;
	bool isUsingDedicatedInput=gEnv->pSystem->IsDedicated();

#if !defined(_RELEASE)
	isUsingDedicatedInput |= g_pGameCVars->g_playerUsesDedicatedInput ? true : false;
#endif

	if(pIPlayerInput && !isUsingDedicatedInput)
	{
		assert(pIPlayerInput->GetType() == IPlayerInput::PLAYER_INPUT);
		CPlayerInput * pPlayerInput = static_cast<CPlayerInput*>(pIPlayerInput);

		Ang3 rawDeltas = pPlayerInput->GetRawControllerInput();

		absInput = min((fabsf(rawDeltas.x) + fabsf(rawDeltas.z)) * 1.4f, 1.0f);

		aimAssistPowerFactor = 2.0f - absInput;
	}

	//Gather weapon info before processing target aim assistance
	bool playerWeaponIsRequestingSnap = false;
	CWeapon* pWeapon = m_player.GetWeapon(m_player.GetCurrentItemId());
	if (pWeapon)
	{
		playerWeaponIsRequestingSnap = pWeapon->ShouldSnapToTarget();
	}

	TargetAimAssistance(targetAimAssistAngleFollowH, targetAimAssistAngleFollowV, targetAimAssistAngleScale);

	targetAimAssistAngleScale = powf(targetAimAssistAngleScale, aimAssistPowerFactor);

	//TODO: Fix so it's not using auto aim unless selected
	m_deltaAngles.z = (m_deltaAngles.z * targetAimAssistAngleScale) + (absInput * targetAimAssistAngleFollowH);
	m_deltaAngles.x = (m_deltaAngles.x * targetAimAssistAngleScale) + (absInput * targetAimAssistAngleFollowV);

	if (gEnv->bMultiplayer == false)
	{
		bool snapToTarget = (playerWeaponIsRequestingSnap && !m_snap_target_dir.IsZero());
		EntityId forcedSnapTargetId = g_pGame->GetAutoAimManager().GetExternalSnapTarget();

		if (forcedSnapTargetId)
		{
			Vec3 snapDirection = GetSnapTargetDirection(forcedSnapTargetId);
			if (!snapDirection.IsZero())
			{
				const float blendSpeed = clamp( g_pGameCVars->pl_melee.melee_snap_blend_speed , 0.0f, 1.0f);
				m_deltaAngles.z = -cry_asinf(snapDirection * m_viewQuat.GetColumn0()) * blendSpeed;
				m_deltaAngles.x = cry_asinf(snapDirection * m_viewQuat.GetColumn2()) * blendSpeed;
			}
			//Clear after used
			g_pGame->GetAutoAimManager().SetExternalSnapTarget(0);
		}
		else if (snapToTarget)
		{
			const float zoomInTime = pWeapon->GetZoomInTime();
			const float frameTime = gEnv->pTimer->GetFrameTime();
			const float blendFactor = clamp(cry_powf((1.0f-zoomInTime),2.0f), 0.0f, 1.0f);
			m_deltaAngles.z = -cry_asinf(m_snap_target_dir * m_viewQuat.GetColumn0()) * blendFactor;
			m_deltaAngles.x = cry_asinf(m_snap_target_dir * m_viewQuat.GetColumn2()) * blendFactor;		
		}
		s_lastAimSnapId = snapToTarget ? m_desiredAimSnapId : 0;
		s_lastTimeRequestedSnapping = playerWeaponIsRequestingSnap;
	}

	if(g_pGameCVars->ctrlr_OUTPUTDEBUGINFO)
	{
		float white[] = {1,1,1,1};
		gEnv->pRenderer->Draw2dLabel( 20, 100, 1.4f, white, false, "Crytek Nottingham Aim Acceleration & Assist\n  absInput: %.6f", absInput );
	}
}


bool CPlayerRotation::ShouldProcessTargetAssistance() const
{
	if (!m_player.IsClient())
		return false;

	if (!g_pGameCVars->pl_aim_acceleration_enabled)
		return false;

#if ENABLE_MINDCONTROL
	if (m_player.IsMindControlling())
		return false;
#endif

	return true;
}


void CPlayerRotation::ProcessNormal()
{
	Vec3 vUpVector = Vec3::CreateSlerp(m_upVector,m_stats.upVector,min(5.0f*m_frameTime, 1.0f));
	
	m_upVector = vUpVector;

	//create a matrix perpendicular to the ground
	Vec3 up(vUpVector);
	//..or perpendicular to the linked object Z
	SLinkStats *pLinkStats = &m_player.m_linkStats;
	if (pLinkStats->linkID && pLinkStats->flags & LINKED_FREELOOK)
	{
		IEntity *pLinked = pLinkStats->GetLinked();
		if (pLinked)
			up = pLinked->GetRotation().GetColumn2();
	}
	
	Vec3 right(m_baseQuat.GetColumn0());
	Vec3 forward((up % right).GetNormalized());

	CHECKQNAN_VEC(up);
	CHECKQNAN_VEC(right);

	if (ShouldProcessTargetAssistance())
	{
		ProcessTargetAssistance();
	}

	Ang3 vNewDeltaAngles = m_deltaAngles * m_stats.flashBangStunMult;

	m_player.DebugGraph_AddValue("AimDeltaH", vNewDeltaAngles.z);
	m_player.DebugGraph_AddValue("AimDeltaV", vNewDeltaAngles.x);

	Ang3 newViewAngles;
	newViewAngles.Set(m_viewAngles.x + vNewDeltaAngles.x, m_viewAngles.y, m_viewAngles.z + vNewDeltaAngles.z);

	//These values need to be used because the player rotation is a quat and quaternions wrap on 720 degrees
	newViewAngles.z = (float)__fsel(  newViewAngles.z - (2.0f * gf_PI2),  newViewAngles.z - (4.0f * gf_PI2), newViewAngles.z);
	newViewAngles.z = (float)__fsel(-(newViewAngles.z + (2.0f * gf_PI2)), newViewAngles.z + (4.0f * gf_PI2), newViewAngles.z);

	m_viewAngles = newViewAngles;
		
	m_baseQuat = Quat::CreateRotationZ(newViewAngles.z);
	
	newViewAngles.y += m_viewRoll;
	m_viewQuat.SetRotationXYZ(newViewAngles);
	
	m_deltaAngles = vNewDeltaAngles;

	CHANGED_NETWORK_STATE_REF(m_player, CPlayer::ASPECT_INPUT_CLIENT);
}

void CPlayerRotation::ProcessLeanAndPeek()
{
	float leanAmt(0.0f);

	if(float sLean = m_player.GetSpeedLean())
		leanAmt = std::min(1.0f, sLean * 0.05f);
	else if((m_actions & (ACTION_LEANLEFT|ACTION_LEANRIGHT)) != 0)
		leanAmt = ((m_actions & ACTION_LEANLEFT)?-1.0f:0.0f) + ((m_actions & ACTION_LEANRIGHT)?1.0f:0.0f);
	else if(fabsf(m_desiredLeanAmount) > 0.01f)
		leanAmt = m_desiredLeanAmount;
	

	EStance stance = m_player.GetStance();

	m_leanAmount = leanAmt + m_player.GetCoverAndLean().GetLeanAmount();
	
	//check if its possible
	if (m_leanAmount*m_leanAmount > 0.01f)
	{
		float noLean(0.0f);
		Vec3 headPos(m_player.GetEntity()->GetWorldPos() + m_baseQuat * m_player.GetStanceViewOffset(stance,&noLean));
		Vec3 newPos(m_player.GetEntity()->GetWorldPos() + m_baseQuat * m_player.GetStanceViewOffset(stance,&m_leanAmount));

		/*gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(headPos, 0.05f, ColorB(0,0,255,255) );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(newPos, 0.05f, ColorB(0,0,255,255) );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(headPos, ColorB(0,0,255,255), newPos, ColorB(0,0,255,255));*/

		ray_hit hit;
		int rayFlags(rwi_stop_at_pierceable|rwi_colltype_any);//COLLISION_RAY_PIERCABILITY & rwi_pierceability_mask);
		IPhysicalEntity *pSkip(m_player.GetEntity()->GetPhysics());

		float distMult(3.0f);

		if (gEnv->pPhysicalWorld->RayWorldIntersection(headPos + m_viewQuat.GetColumn1() * 0.25f, (newPos - headPos)*distMult, ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid, rayFlags, &hit, 1, &pSkip, 1))
		{
			float dist((headPos - newPos).len2() * distMult);
			m_leanAmount *= ((hit.pt - headPos).len2() / dist) / distMult;

			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(hit.pt, 0.05f, ColorB(0,255,0,255) );
		}
	}

	// TODO(Márcio): Maybe do some checks here!
	m_peekOverAmount = m_desiredPeekOverAmount;
}


void CPlayerRotation::ProcessSliding()
{
	ClampAngles();

	m_viewRoll = 0;

	Ang3 viewBaseAngles(m_viewQuat);
	viewBaseAngles.z += m_deltaAngles.z;
	viewBaseAngles.x = 0.0f;
	
	m_viewQuat = Quat::CreateRotationXYZ(viewBaseAngles) * Quat::CreateRotationX(GetLocalPitch() + m_deltaAngles.x);

	CHANGED_NETWORK_STATE_REF(m_player, CPlayer::ASPECT_INPUT_CLIENT);
}


Vec3 CPlayerRotation::GetSnapTargetDirection(EntityId snapTargetId) const
{
	Vec3 snapDirection = ZERO;

	const SAutoaimTarget* pAATarget = g_pGame->GetAutoAimManager().GetTargetInfo(snapTargetId);
	if(pAATarget == NULL)
		return snapDirection;

	Vec3 playerPos = m_player.GetEntity()->GetWorldPos() + m_player.GetEyeOffset();

	snapDirection = pAATarget->primaryAimPosition - playerPos;
	
	return (snapDirection.GetNormalized());
}

Vec3 CPlayerRotation::PickBestSnapDirection( const Vec3& aimPos, const Vec3& aimDirection, const Vec3& targetDir, const SAutoaimTarget& aaTarget, const float onTargetWeight )
{
	const Vec3 secondaryTargetDir = ((aaTarget.secondaryAimPosition - aimPos).GetNormalizedSafe());
	m_desiredAimSnapId = s_lastAimSnapId;

	//If a target was previously selected, keep it
	switch(m_desiredAimSnapId)
	{
	case eSnapTarget_None:
		{
			if (s_lastTimeRequestedSnapping)
				return ZERO;
		}
		break;

	case eSnapTarget_Primary:
		return targetDir;

	case eSnapTarget_Secondary:
		return secondaryTargetDir;
	}

	//Inside inner sphere, we assume, we are aiming at the target already
	if (onTargetWeight >= 0.999f)
	{
		m_desiredAimSnapId = eSnapTarget_OnTarget;
		return ZERO;
	}

	//Somewhere in the outer sphere, pick best target
	if (onTargetWeight >= 0.01)
	{
		Lineseg segment(aimPos, aimPos + (aimDirection * g_pGameCVars->aim_assistMaxDistance));
		Sphere secondaryTarget;
		secondaryTarget.center = aaTarget.secondaryAimPosition;
		secondaryTarget.radius = aaTarget.secondaryRadius;
		Vec3 intPoint1, intPoint2;

		if (Intersect::Lineseg_Sphere(segment, secondaryTarget, intPoint1, intPoint2) == 0)
		{
			m_desiredAimSnapId = eSnapTarget_Primary;
			return targetDir;
		}
		else
		{
			m_desiredAimSnapId = eSnapTarget_Secondary;
			return secondaryTargetDir;
		}
	}

	//By default pick the primary (snap sphere)
	m_desiredAimSnapId = eSnapTarget_Primary;
	return targetDir;
}
