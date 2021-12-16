/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
Special IPlayerInput that handles 'mind controlling', the act of temporarily
taking over the control of an otherwise AI-controlled CPlayer.

It tries to mimic the way the AI controls a CPlayer as much as possible.
-------------------------------------------------------------------------
History:
- 22:10:2009: Created by Sven Van Soom

*************************************************************************/
#include "StdAfx.h"
#include "MindControlInput.h"

#if ENABLE_MINDCONTROL

#include "Player.h"
#include "GameCVars.h"
#include "GameActions.h"
#include "HUD/HUD.h"
#include "IAIActor.h"

#if defined(PS3)
const float DEAD_ZONE_ROTATE_YAW = 0.02f;
const float DEAD_ZONE_ROTATE_PITCH = 0.02f;
#else
const float DEAD_ZONE_ROTATE_YAW = 0.003f;
const float DEAD_ZONE_ROTATE_PITCH = 0.003f;
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
TActionHandler<CMindControlInput>	CMindControlInput::s_actionHandler;


// ---------------------------------------------------------------------------
CMindControlInput::CMindControlInput( CPlayer * pPlayer, CPlayer* pMaster ) : 
m_pPlayer(pPlayer),
m_pMaster(pMaster),
m_moveButtonState(0),
m_deltaMovement(ZERO),
m_bUseXIInput(false),
m_xi_deltaMovement(ZERO),
m_xi_deltaRotation(0,0,0),
m_xi_deltaRotationRaw(0.0f, 0.0f, 0.0f),
m_moveMode(eMM_MoveTarget),
m_moveTarget(ZERO),
m_aimMode(eAM_None),
m_aimTarget(ZERO),
m_suppressAiming(false),
m_aimDeltaAngles(ZERO),
m_lookMode(eLM_None),
m_lookTarget(ZERO),
m_lookDeltaAngles(ZERO),
m_cameraMode(eCM_FollowFront),
m_cameraDeltaAngles(ZERO),
m_pressedSprintingButton(false),
m_isSprinting(false),
m_isFiring(false),
m_allowStrafing(true),
m_menuOpen(false),
m_menuOpenness(0.0f),
m_stance(STANCE_STAND),
m_agentMovementUrgency(AgentMovementSpeeds::AMU_WALK),
m_agentMovementStance(AgentMovementSpeeds::AMS_COMBAT)
{
	m_pMaster->GetGameObject()->CaptureActions(this);

	// set up the handlers
	if (s_actionHandler.GetNumHandlers() == 0)
	{
#define ADD_HANDLER(action, func) s_actionHandler.AddHandler(actions.action, &CMindControlInput::func)
		const CGameActions& actions = g_pGame->Actions();

		// TODO: Create a new input scheme for Mind Control

		ADD_HANDLER(moveforward, OnActionMoveForward);
		ADD_HANDLER(moveback, OnActionMoveBack);
		ADD_HANDLER(moveleft, OnActionMoveLeft);
		ADD_HANDLER(moveright, OnActionMoveRight);

		ADD_HANDLER(sprint, OnActionSprint);
		ADD_HANDLER(crouch, OnActionCrouch);

		ADD_HANDLER(xi_movex, OnActionXIMoveX);
		ADD_HANDLER(xi_movey, OnActionXIMoveY);
		ADD_HANDLER(xi_disconnect, OnActionXIDisconnect);
		ADD_HANDLER(xi_rotateyaw, OnActionXIRotateYaw);
		ADD_HANDLER(xi_rotatepitch, OnActionXIRotatePitch);

		ADD_HANDLER(suitmode_menu_open, OnActionMenuOpen);
		ADD_HANDLER(suitmode_menu_close, OnActionMenuClose);
		ADD_HANDLER(suitmode_menu_manouver, OnActionMenuClick);
		ADD_HANDLER(suitmode_menu_energy, OnActionMenuClick);
		ADD_HANDLER(suitmode_menu_combat, OnActionMenuClick);
		ADD_HANDLER(suitmode_menu_infiltrate, OnActionMenuClick);
		ADD_HANDLER(suitPower, OnActionAllowStrafingToggle);

		ADD_HANDLER(attack1_xi, OnActionAttackRightTrigger);
		ADD_HANDLER(attack2_xi, OnActionAttackLeftTrigger);

		ADD_HANDLER(toggle_weapon, OnActionToggleMediumWeapon);

#undef ADD_HANDLER
	}
}


// ---------------------------------------------------------------------------
CMindControlInput::~CMindControlInput()
{
	m_pMaster->GetGameObject()->ReleaseActions(this);
}


// ---------------------------------------------------------------------------
void CMindControlInput::Reset()
{
	m_deltaMovement.zero();
	m_xi_deltaMovement.zero();
	m_xi_deltaRotation.Set(0,0,0);
	m_xi_deltaRotationRaw.Set(0.0f, 0.0f, 0.0f);
	m_moveButtonState = 0;
	m_agentMovementStance = AgentMovementSpeeds::AMS_COMBAT;
	m_isSprinting = false;
	m_pressedSprintingButton = false;
	m_suppressAiming = false;
	m_isFiring = false;
	m_aimTarget.zero();
	m_moveTarget.zero();
}


// ---------------------------------------------------------------------------
void CMindControlInput::DisableXI(bool disabled)
{
	// Not supported
}


// ---------------------------------------------------------------------------
void CMindControlInput::ClearXIMovement()
{
	m_xi_deltaRotationRaw.Set(0.0f, 0.0f, 0.0f);
	m_xi_deltaRotation.Set(0.f,0.f,0.f);
	m_xi_deltaMovement.zero();
}


// ---------------------------------------------------------------------------
void CMindControlInput::OnAction( const ActionId& actionId, int activationMode, float value )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	bool filterOut = true;
	bool handled = s_actionHandler.Dispatch(this, m_pPlayer->GetEntityId(), actionId, activationMode, value, filterOut);

	// Pass the actions along to the actor, its items, in particular the weapon, which can then choose to fire or not:
	m_pPlayer->CActor::OnAction(actionId, activationMode, value);

	// Pass the actions along to the HUD for the Tweak Menu
	g_pGame->GetHUD()->OnAction(actionId, activationMode, value);

	// I do not pass the actions along to script because it does not seem necessary
}


// ---------------------------------------------------------------------------
void CMindControlInput::PreUpdateAiming(CMovementRequest& mr)
{
	bool isAiming = false;

	if (m_suppressAiming || (m_aimMode == eAM_None))
	{
		isAiming = false;
		// don't zero the aim target, we want to remember it for when we enable the aiming again
	}
	else
	{
		switch(m_aimMode)
		{
		case eAM_AimAtCamera:
			{
				isAiming = true;
				m_aimTarget = gEnv->pRenderer->GetCamera().GetPosition();
				break;
			}
		case eAM_AimAtPlayer:
			{
				isAiming = true;
				m_aimTarget = m_pMaster->GetWBodyCenter();
				break;
			}
		case eAM_AimForward:
			{
				isAiming = true;
				m_aimTarget = m_pPlayer->GetWBodyCenter() + 8.0f*m_pPlayer->GetEntity()->GetForwardDir();
				break;
			}
		case eAM_AimDirection:
			{
				isAiming = true;

				const float degreesPerSecond = 90.0f;

				m_aimDeltaAngles.x = 0.0f;
				m_aimDeltaAngles.y -= m_xi_deltaRotationRaw.x * DEG2RAD(degreesPerSecond) * gEnv->pTimer->GetFrameTime();
				m_aimDeltaAngles.z -= m_xi_deltaRotationRaw.z * DEG2RAD(degreesPerSecond) * gEnv->pTimer->GetFrameTime();

				m_aimTarget = m_pPlayer->GetWBodyCenter() + (Quat::CreateRotationXYZ(m_aimDeltaAngles) * Vec3(8,0,0));
				break;
			}
		default:
			CRY_ASSERT(false);
		}
	}


	if (isAiming)
		mr.SetAimTarget(m_aimTarget);
	else
		mr.ClearAimTarget();

	if (m_isFiring)
		mr.SetFireTarget( m_aimTarget );
	else
		mr.ClearFireTarget();
}


// ---------------------------------------------------------------------------
void CMindControlInput::PreUpdateLooking(CMovementRequest& mr)
{
	bool isLooking = false;

	switch(m_lookMode)
	{
	case eLM_LookAtCamera:
		{
			isLooking = true;
			m_lookTarget = gEnv->pRenderer->GetCamera().GetPosition();
			break;
		}
	case eLM_LookAtPlayer:
		{
			isLooking = true;
			m_lookTarget = m_pMaster->GetWBodyCenter();
			break;
		}
	case eLM_LookForward:
		{
			isLooking = true;
			m_lookTarget = m_pPlayer->GetWBodyCenter() + 8.0f*m_pPlayer->GetEntity()->GetForwardDir();
			break;
		}
	case eLM_LookDirection:
		{
			isLooking = true;

			const float degreesPerSecond = 90.0f;

			m_lookDeltaAngles.x = 0.0f;
			m_lookDeltaAngles.y -= m_xi_deltaRotationRaw.x * DEG2RAD(degreesPerSecond) * gEnv->pTimer->GetFrameTime();
			m_lookDeltaAngles.z -= m_xi_deltaRotationRaw.z * DEG2RAD(degreesPerSecond) * gEnv->pTimer->GetFrameTime();

			m_lookTarget = m_pPlayer->GetWBodyCenter() + (Quat::CreateRotationXYZ(m_lookDeltaAngles) * Vec3(8,0,0));
			break;
		}
	case eLM_None:
		{
			isLooking = false;
			// don't zero the look target, we want to remember it for when we enable the looking again
			break;
		}
	default:
		CRY_ASSERT(false);
	}

	if (isLooking)
		mr.SetLookTarget(m_lookTarget);
	else
		mr.ClearLookTarget();
}


// ---------------------------------------------------------------------------
void CMindControlInput::PreUpdateMovementAndRotation(CMovementRequest& mr)
{
	// ----------------
	// Process XI Input
	// ----------------

	if(m_bUseXIInput)
	{
		m_deltaMovement.x = m_xi_deltaMovement.x;
		m_deltaMovement.y = m_xi_deltaMovement.y;
		m_deltaMovement.z = 0;

		if (m_moveMode == eMM_MoveTarget)
		{
			// Experimental code that can probably better be replaced by a variation on CPlayerInput's 
			// UpdateXIRotation. Look into this when implementing skids/turns.

			// When using the MoveTarget mode make the X movement less sensitive
			// but keep the original 'length' so running remains running, walking remains walking.
			float originalLength = m_deltaMovement.GetLength2D();
			m_deltaMovement.x = MapControllerValue(m_deltaMovement.x, 1.0f, 4.0f, false);
			m_deltaMovement.y = MapControllerValue(m_deltaMovement.y, 1.0f, 2.0f, false);
			m_deltaMovement *= originalLength/m_deltaMovement.GetLength2D();

// 			// When using the MoveTarget mode snap the movement to the major axes or diagonals
// 			float angle = atan2f(m_deltaMovement.y, m_deltaMovement.x);
// 
// 			// snap to closest multiple of PI/4
// 			float snapAngle = gf_PI/4.f;
// 			angle = snapAngle * int( (angle + 2*gf_PI + snapAngle/2.f) / snapAngle );
// 
// 			float len = m_deltaMovement.GetLength2D();
// 			m_deltaMovement.x = len * cos(angle);
// 			m_deltaMovement.y = len * sin(angle);
		}
	}

	UpdateXIRotation();

	// ---------
	// Sprinting
	// ---------

	const float thresholdSinAngle = sin_tpl((3.141592f * 0.5f) - DEG2RAD(g_pGameCVars->pl_power_sprint.foward_angle));
	const float currentSinAngle = m_deltaMovement.y;
	bool isMovingForward = (currentSinAngle > thresholdSinAngle);

	if (isMovingForward && (m_isSprinting || m_pressedSprintingButton))
	{
		m_isSprinting = true;
	}
	else
	{
		m_isSprinting = false;
	}


	// --------------------------------
	// AI Stance & Urgency, PseudoSpeed
	// --------------------------------

	// NOTE: The sprinting code in PlayerMovement does NOT work for the AIs. This is because the CAIInput does NOT set the ACTION_SPRINT
	// action flag as far as I can see. Because I want to emulate an AI I will set the sprinting by myself too:
	if (m_deltaMovement.len2() > 0.0f)
	{
		if (m_isSprinting)
		{
			m_agentMovementUrgency = AgentMovementSpeeds::AMU_SPRINT;
			mr.SetPseudoSpeed(AISPEED_SPRINT);
		}
		else
		{
			if (m_deltaMovement.len2() > 0.6f)
			{
				m_agentMovementUrgency = AgentMovementSpeeds::AMU_RUN;
				mr.SetPseudoSpeed(AISPEED_RUN);
			}
			else
			{
				m_agentMovementUrgency = AgentMovementSpeeds::AMU_WALK;
				mr.SetPseudoSpeed(AISPEED_WALK);
			}
		}
	}
	else
	{
		CRY_ASSERT(m_isSprinting == false);
		m_agentMovementUrgency = AgentMovementSpeeds::AMU_WALK;
		mr.SetPseudoSpeed(AISPEED_ZERO);
	}

	// ----------------------------------------------------------------------------
	// MoveTarget, DesiredSpeed, DistanceToPathEnd (or DeltaMovement/DeltaRotation)
	// ----------------------------------------------------------------------------

	IAIActor* pAIActor = CastToIAIActorSafe(m_pPlayer->GetEntity()->GetAI());
	CRY_ASSERT(pAIActor);
	float maxSpeed = pAIActor->GetMovementAbility().movementSpeeds.GetRange(m_agentMovementStance, m_agentMovementUrgency).def;

	switch(m_moveMode)
	{
		case eMM_MoveTarget:
		{
			// This is the method the AI is using
			Vec3 currentPos = m_pPlayer->GetEntity()->GetWorldPos();

			if (m_deltaMovement.len2() > 0.0f)
			{
				Vec3 movetargetdir = m_pPlayer->GetEntity()->GetWorldRotation() * m_deltaMovement.GetNormalizedSafe();
				m_moveTarget = currentPos + movetargetdir;	

				mr.SetMoveTarget(m_moveTarget);
				mr.SetDesiredSpeed(maxSpeed);
				mr.SetDistanceToPathEnd(1000.0f);
			}
			else
			{
				mr.ClearMoveTarget(); // do NOT call SetMoveTarget(currentPos) as the currentPos here isn't the same as the pos in the movementcontroller so he will still try to move!
				mr.SetDesiredSpeed(0.0f);
				mr.ClearDistanceToPathEnd();
			}

			mr.ClearActorTarget();
			break;
		}
		case eMM_MoveRelative:
		{
			// This is the method the player is using, so not accurate to simulate an AI

			float speed = m_deltaMovement.GetLength2D();
			Vec3 deltaMovement = m_deltaMovement;

			if (speed > 1.0f) // make sure you cannot move faster than 1 (normalized speed), this happens when strafing
			{
				deltaMovement.x /= speed;
				deltaMovement.y /= speed;
			}

			deltaMovement *= maxSpeed / m_pPlayer->GetStanceMaxSpeed(m_stance);

			mr.AddDeltaMovement(deltaMovement); // local space
			mr.ClearMoveTarget();
			mr.ClearDesiredSpeed();
			mr.ClearDistanceToPathEnd();
			mr.ClearActorTarget();

			mr.AddDeltaRotation(m_xi_deltaRotation * 6.0f * gEnv->pTimer->GetFrameTime() ); // 6 is a random number that replaces many lines of dubious mouse sensitivity code
			break;
		}
		default:
		{
			CRY_ASSERT(false);
		}
	}

	// Note: AI calculates prediction, a predicted path, but we don't. We have no way of predicting what a user will do next.
	mr.ClearPrediction();

	// -------------
	// AllowStrafing
	// -------------

	mr.SetAllowStrafing(m_allowStrafing);

	// Note: AI doesn't call SetBodyTarget, so we don't do it either
}

// ---------------------------------------------------------------------------
void CMindControlInput::PreUpdateBodyTarget(CMovementRequest& mr)
{
	switch(m_agentMovementStance)
	{
	case AgentMovementSpeeds::AMS_COMBAT:
	case AgentMovementSpeeds::AMS_CROUCH:
	case AgentMovementSpeeds::AMS_PRONE:
	case AgentMovementSpeeds::AMS_RELAXED:
	case AgentMovementSpeeds::AMS_STEALTH:
	case AgentMovementSpeeds::AMS_SWIM:
		{
			mr.ClearBodyTarget();
			break;
		}
	case AgentMovementSpeeds::AMS_LOW_COVER:
	case AgentMovementSpeeds::AMS_HIGH_COVER: 
		{
			mr.SetBodyTarget(m_pPlayer->GetWBodyCenter() + Vec3(1.0f, 0.0f, 0.0f)); 
			break;
		}
	default: 
		{
			CRY_ASSERT(false); 
			mr.ClearBodyTarget();
		}
	}
}

// ---------------------------------------------------------------------------
void CMindControlInput::PreUpdateStance(CMovementRequest& mr)
{
	switch(m_agentMovementStance)
	{
		case AgentMovementSpeeds::AMS_COMBAT: m_stance = STANCE_STAND; break;
		case AgentMovementSpeeds::AMS_LOW_COVER: m_stance = STANCE_LOW_COVER; break;
		case AgentMovementSpeeds::AMS_HIGH_COVER: m_stance = STANCE_HIGH_COVER; break;
		case AgentMovementSpeeds::AMS_CROUCH: m_stance = STANCE_CROUCH; break;
		case AgentMovementSpeeds::AMS_PRONE: m_stance = STANCE_PRONE; break;
		case AgentMovementSpeeds::AMS_RELAXED: m_stance = STANCE_RELAXED; break;
		case AgentMovementSpeeds::AMS_STEALTH: m_stance = STANCE_STEALTH; break;
		case AgentMovementSpeeds::AMS_SWIM: m_stance = STANCE_SWIM; break;
		default: CRY_ASSERT(false); m_stance = STANCE_STAND; break;
	}

	mr.SetStance(m_stance);
}


// ---------------------------------------------------------------------------
void CMindControlInput::PreUpdate()
{
	CMovementRequest mr;

	// Not implemented: SetAlertness
	// Not implemented: SetFacialAlertnessLevel

	// Not implemented: SetLookStyle

	// Not implemented: SetLean
	// Not implemented: SetPrediction

	// Not implemented: ag->SetTargetPointVerifier

	// Not implemented: Exact positioning and SetActorTarget

	PreUpdateStance(mr); // must be called before updating movement and rotation because stance influences those

	PreUpdateBodyTarget(mr);

	// Not implemented: SetJump

	PreUpdateAiming(mr);
	PreUpdateLooking(mr);

	PreUpdateMovementAndRotation(mr);

	// Not implemented: SetForcedNavigation

	//+ AI DOESN'T CALL SetNoAiming

	m_pPlayer->GetMovementController()->RequestMovement(mr);
}


// ---------------------------------------------------------------------------
void CMindControlInput::Update()
{
}


// ---------------------------------------------------------------------------
void CMindControlInput::PostUpdate()
{
	const char* AimModeNames[] =
	{
		"None",
		"Aim at Player",
		"Aim at Camera",
		"Aim Forward",
		"Aim in Specific Direction",
	};

	const char* LookModeNames[] =
	{
		"None",
		"Look at Player",
		"Look at Camera",
		"Look Forward",
		"Look in Specific Direction",
	};

	const char* MoveModeNames[] =
	{
		"Follow the Ball (AI)",
		"2-Stick (player)",
	};

	const char* CameraModeNames[] =
	{
		"Player",
		"FollowBehind",
		"FollowFront",
		"FollowSide",
		"FollowTop",
		"LookAtSlaveFromPlayer",
	};

	const char* StrafingNames[] =
	{
		"Not Allowed",
		"Allowed",
	};

	// Note: this should be defined somewhere in the AI project, near the enum
	const char* AgentMovementStanceNames[] =
	{
		"RELAXED", 
		"COMBAT",
		"STEALTH", 
		"LOW COVER",
		"HIGH COVER",
		"CROUCH", 
		"PRONE", 
		"SWIM",
	};

	// Note: this should be defined somewhere in the AI project, near the enum
	const char* AgentMovementUrgencyNames[] =
	{
		"SLOW",
		"WALK",
		"RUN",
		"SPRINT",
	};

	ColorF mainColor(1.0f, 1.0f, 1.0f);
	ColorF yellowColor(1.0f, 0.9f, 0.0f);
	ColorF aimColor(1.0f, 0.0f, 0.0f);
	ColorF moveColor(0.1f, 1.0f, 0.0f);
	ColorF lookColor(0.1f,0.4f,0.9f);
	ColorF strafingColor(0.7f,0.7f,0.9f);
	ColorF semiTransparent(1.0f,1.0f,1.0f,0.4f);

	// ---------------
	// Draw debug text
	// ---------------
	{
		const float openDuration = 0.1f;

		const float closedSize = 1.3f;
		const float openSize = 4.0f;

		const float closedX = 10.0f;
		const float openX = 15.0f;

		const float closedY = 10.0f;
		const float openY = 15.0f;

		float textSize = closedSize;
		float textX = closedX;
		float textY = closedY;

		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*00, textSize, &mainColor[0], false, "MIND CONTROL Entity = %s", m_pPlayer->GetEntity()->GetName());

		const char* szItemName = m_pPlayer->GetCurrentItem() ? m_pPlayer->GetCurrentItem()->GetEntity()->GetClass()->GetName() : "<none>";
		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*10, textSize, &mainColor[0], false, "AI Urgency = %s, AI Stance = %s, Stance = %s, Item = %s", AgentMovementUrgencyNames[m_agentMovementUrgency], AgentMovementStanceNames[m_agentMovementStance], ::GetStanceName(m_stance), szItemName);

		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*20, textSize, &strafingColor[0], false, "Strafing = %s", StrafingNames[(int)m_allowStrafing]);

		m_menuOpenness = clamp_tpl<float>(m_menuOpenness + (m_menuOpen ? +1.0f : -1.0f) * gEnv->pTimer->GetFrameTime()/openDuration, 0.0f, 1.0f);
		textSize = LERP(closedSize, openSize, m_menuOpenness);
		textX = LERP(closedX, openX, m_menuOpenness);
		textY = LERP(closedY, openY, m_menuOpenness);

		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*35, textSize, &yellowColor[0], false, "camera = %s", CameraModeNames[m_cameraMode]);
		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*45, textSize, &lookColor[0], false, "look = %s", LookModeNames[m_lookMode]);
		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*55, textSize, &aimColor[0], false, "aim = %s%s", m_suppressAiming ? "[SUPPRESSED]" : "", AimModeNames[m_aimMode]);
		gEnv->pRenderer->Draw2dLabel(textX, textY+textSize*65, textSize, &moveColor[0], false, "move = %s", MoveModeNames[m_moveMode]);
	}

	// ---------------------------------------------
	// Draw Aiming/Looking/Moving lines and spheres
	// ---------------------------------------------
	{
		IRenderAuxGeom  *pRenderAuxGeom ( gEnv->pRenderer->GetIRenderAuxGeom() );

		SAuxGeomRenderFlags oldFlags = pRenderAuxGeom->GetRenderFlags();
		pRenderAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags|e_AlphaBlended);

		if (m_moveMode == eMM_MoveTarget)
		{
			pRenderAuxGeom->DrawSphere(m_moveTarget, 0.1f, moveColor*semiTransparent, false);
			pRenderAuxGeom->DrawLine(m_pPlayer->GetWBodyCenter(), moveColor*semiTransparent, m_moveTarget,  moveColor*semiTransparent, 0.1f);
		}

		if (m_aimMode != eAM_None)
		{
			pRenderAuxGeom->DrawSphere(m_aimTarget, 0.1f, aimColor*semiTransparent, false);
			pRenderAuxGeom->DrawLine(m_pPlayer->GetWBodyCenter(), aimColor*semiTransparent, m_aimTarget, aimColor*semiTransparent, 0.1f);
		}

		if (m_lookMode != eLM_None)
		{
			pRenderAuxGeom->DrawSphere(m_lookTarget, 0.1f, lookColor*semiTransparent, false);
			pRenderAuxGeom->DrawLine(m_pPlayer->GetWBodyCenter(), lookColor*semiTransparent, m_lookTarget, lookColor*semiTransparent, 0.1f);
		}

		pRenderAuxGeom->SetRenderFlags(oldFlags);
	}
}


// ---------------------------------------------------------------------------
void CMindControlInput::GetState( SSerializedPlayerInput& input )
{
	GameWarning("CMindControlInput::GetState called: should never happen");
}


// ---------------------------------------------------------------------------
void CMindControlInput::SetState( const SSerializedPlayerInput& input )
{
	GameWarning("CMindControlInput::SetState called: should never happen");
}


// ---------------------------------------------------------------------------
void CMindControlInput::ApplyMovement(Vec3 delta)
{
	m_deltaMovement.x = clamp_tpl(m_deltaMovement.x+delta.x,-1.0f,1.0f);
	m_deltaMovement.y = clamp_tpl(m_deltaMovement.y+delta.y,-1.0f,1.0f);
	m_deltaMovement.z = 0;
}


// ---------------------------------------------------------------------------
void CMindControlInput::AdjustMoveButtonState(EMoveButtonMask buttonMask, int activationMode )
{
	if (activationMode == eAAM_OnPress)
	{
		m_moveButtonState |= buttonMask;
	}
	else if (activationMode == eAAM_OnRelease)
	{
		m_moveButtonState &= ~buttonMask;
	}
}


// ---------------------------------------------------------------------------
bool CMindControlInput::CheckMoveButtonStateChanged(EMoveButtonMask buttonMask, int activationMode)
{
	bool current = (m_moveButtonState & buttonMask) != 0;

	if(activationMode == eAAM_OnRelease)
	{
		return current;
	}
	else if(activationMode == eAAM_OnPress)
	{
		return !current;
	}
	return true;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(CheckMoveButtonStateChanged(eMBM_Forward, activationMode))
	{
		ApplyMovement(Vec3(0,value*2.0f - 1.0f,0));
		AdjustMoveButtonState(eMBM_Forward, activationMode);
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(CheckMoveButtonStateChanged(eMBM_Back, activationMode))
	{
		ApplyMovement(Vec3(0,-(value*2.0f - 1.0f),0));
		AdjustMoveButtonState(eMBM_Back, activationMode);
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(CheckMoveButtonStateChanged(eMBM_Left, activationMode))
	{
		ApplyMovement(Vec3(-(value*2.0f - 1.0f),0,0));
		AdjustMoveButtonState(eMBM_Left, activationMode);
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(CheckMoveButtonStateChanged(eMBM_Right, activationMode))
	{
		ApplyMovement(Vec3(value*2.0f - 1.0f,0,0));
		AdjustMoveButtonState(eMBM_Right, activationMode);
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionXIMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaMovement.x = value;
	if(fabsf(value)>0.001f && !m_bUseXIInput)
	{
		m_bUseXIInput = true;
	}
	else if(fabsf(value)<=0.001f && m_bUseXIInput && fabsf(m_xi_deltaMovement.y)<=0.001f)
	{
		m_bUseXIInput = false;
		m_deltaMovement.zero();
	}
	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionXIMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaMovement.y = value;
	if(fabsf(value)>0.001f && !m_bUseXIInput)
	{
		m_bUseXIInput = true;
	}
	else if(fabsf(value)<=0.001f && m_bUseXIInput && fabsf(m_xi_deltaMovement.x)<=0.001f)
	{
		m_bUseXIInput = false;
		m_deltaMovement.zero();
	}

	return false;
}


// ---------------------------------------------------------------------------
float CMindControlInput::MapControllerValue(float value, float scale, float curve, bool inverse)
{
	// Just a copy of CPlayerInput's method

	float res=scale * powf(fabs(value), curve);
	return (value >= 0.0f ? (inverse ? -1.0f : 1.0f) : (inverse ? 1.0f : -1.0f))*res;
}


// ---------------------------------------------------------------------------
void CMindControlInput::UpdateXIRotation()
{
	// The following simple mapping suffices for now.
	// If necessary look into a variation of CPlayerInput::UpdateXIRotation.

	m_xi_deltaRotation.x = m_xi_deltaRotationRaw.x;
	m_xi_deltaRotation.y = m_xi_deltaRotationRaw.y;
	m_xi_deltaRotation.z = -m_xi_deltaRotationRaw.z;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionXIRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaRotationRaw.z = value;

	if(fabs(m_xi_deltaRotation.z) < DEAD_ZONE_ROTATE_YAW)
		m_xi_deltaRotation.z = 0.f;

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionXIRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaRotationRaw.x = value;

	if(fabs(m_xi_deltaRotation.x) < DEAD_ZONE_ROTATE_PITCH)
		m_xi_deltaRotation.x = 0.f;

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionXIDisconnect(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaRotation.Set(0,0,0);
	m_xi_deltaMovement.zero();
	m_bUseXIInput = false;
	m_deltaMovement.zero();

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMenuOpen(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		CRY_ASSERT(!m_menuOpen);
		if (m_menuOpen)
			return false;

		const CGameActions& actions = g_pGame->Actions();
		IActionFilter* suitMenuFilter = actions.FilterSuitMenu();
		suitMenuFilter->Enable(true);

		m_menuOpen = true;
	}
	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMenuClose(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	CRY_ASSERT(m_menuOpen);
	if (!m_menuOpen)
		return false;

	const CGameActions& actions = g_pGame->Actions();
	IActionFilter* suitMenuFilter = actions.FilterSuitMenu();
	suitMenuFilter->Enable(false);

	m_menuOpen = false;
	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionMenuClick(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (!m_menuOpen)
		return false;

	const CGameActions& actions = g_pGame->Actions();

	if (actionId == actions.suitmode_menu_combat)
	{
		m_aimMode = static_cast<EAimMode>( (m_aimMode + 1) % eAM_COUNT );
	}
	else if (actionId == actions.suitmode_menu_manouver)
	{
		m_moveMode = static_cast<EMoveMode>( (m_moveMode + 1) % eMM_COUNT );
	}
	else if (actionId == actions.suitmode_menu_infiltrate)
	{
		m_lookMode = static_cast<ELookMode>( (m_lookMode + 1) % eLM_COUNT );
	}
	else if (actionId == actions.suitmode_menu_energy)
	{
		m_cameraMode = static_cast<ECameraMode>( (m_cameraMode + 1) % eCM_COUNT );
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionCrouch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		bool stanceIsAllowed;
		do
		{
			m_agentMovementStance = static_cast<AgentMovementSpeeds::EAgentMovementStance>((m_agentMovementStance + 1) % AgentMovementSpeeds::AMS_NUM_VALUES);
			
			if (m_agentMovementStance == AgentMovementSpeeds::AMS_SWIM)
				stanceIsAllowed = false;
			else if (m_agentMovementStance == AgentMovementSpeeds::AMS_PRONE)
				stanceIsAllowed = false;
			else
				stanceIsAllowed = true;
		}
		while(!stanceIsAllowed);
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionSprint(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		m_pressedSprintingButton = true;
	}
	else
	{
		m_pressedSprintingButton = false;
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionAttackRightTrigger(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_isFiring = (activationMode & (eAAM_OnPress|eAAM_OnHold)) != 0;

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionAttackLeftTrigger(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_suppressAiming = (activationMode & (eAAM_OnPress|eAAM_OnHold)) != 0;

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionAllowStrafingToggle(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		m_allowStrafing = !m_allowStrafing;
	}

	return false;
}


// ---------------------------------------------------------------------------
bool CMindControlInput::OnActionToggleMediumWeapon(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		// select next item or holster when going beyond the last item
		IInventory* pInventory = m_pPlayer->GetInventory();
		if (!pInventory)
			return false;

		EntityId lastItemId;
		lastItemId = pInventory->GetItem(pInventory->GetCount() - 1);
		if (lastItemId && (m_pPlayer->GetCurrentItemId() == lastItemId))
		{
			m_pPlayer->HolsterItem(true);
		}
		else
		{
			m_pPlayer->SelectNextItem(1, false /*don't keep history*/, NULL);
		}
	}

	return false;
}

#endif
