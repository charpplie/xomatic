#include "StdAfx.h"
#include "PlayerInput.h"
#include "Player.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"
#include "Weapon.h"
#include "WeaponSystem.h"
#include "IVehicleSystem.h"
#include "VehicleClient.h"

#include "HUD/HUDSilhouettes.h"
#include "GameRules.h"
#include "ScreenEffects.h"
#include "PlayerMovementController.h"
#include "GunTurret.h"
#include "GameRulesModules/IGameRulesActorActionModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"

#include "Utility/CryWatch.h"
#include "HUD/HUD.h"

#include <IWorldQuery.h>
#include <IInteractor.h>

#include "RecordingSystem.h"
#include "GodMode.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "IAIActor.h"

TActionHandler<CPlayerInput>	CPlayerInput::s_actionHandler;

CPlayerInput::CPlayerInput( CPlayer * pPlayer ) : 
	m_pPlayer(pPlayer), 
	m_actions(0), 
	m_actionFlags(CPlayer::eAF_NONE), 
	m_deltaRotation(0,0,0), 
	m_deltaMovement(0,0,0), 
	m_xi_deltaMovement(0,0,0),
	m_xi_deltaRotation(0,0,0),
	m_xi_deltaRotationRaw(0.0f, 0.0f, 0.0f),
	m_flyCamDeltaMovement(0,0,0),
	m_flyCamDeltaRotation(0,0,0),
	m_flyCamTurbo(false),
	m_filteredDeltaMovement(0,0,0),
	m_speedLean(0.0f),
	m_jumpPressTime(0.0f),
	m_moveButtonState(0),
	m_lastSerializeFrameID(0),
	m_requestSuperJump(false),
	m_suitMenuOpen(false),
	m_bDisabledXIRot(false),
	m_bUseXIInput(false),
	m_lastSensitivityFactor(1.0f),
	m_lastRegisteredInputTime(0.0f),
	m_suitMenuOpenButtonPressedTime(0.0f)
{
	m_pPlayer->GetGameObject()->CaptureActions(this);

	ClearSuitSelectionInputs();

#if FREE_CAM_SPLINE_ENABLED
	m_freeCamPlaying = false;
	m_freeCamCurrentIndex = 0;
	m_freeCamPlayTimer = 0.f;
	m_freeCamTotalPlayTime = 0.f;
#endif

	// set up the handlers
	if (s_actionHandler.GetNumHandlers() == 0)
	{
	#define ADD_HANDLER(action, func) s_actionHandler.AddHandler(actions.action, &CPlayerInput::func)
		const CGameActions& actions = g_pGame->Actions();

		ADD_HANDLER(moveforward, OnActionMoveForward);
		ADD_HANDLER(moveback, OnActionMoveBack);
		ADD_HANDLER(moveleft, OnActionMoveLeft);
		ADD_HANDLER(moveright, OnActionMoveRight);
		ADD_HANDLER(rotateyaw, OnActionRotateYaw);
		ADD_HANDLER(rotatepitch, OnActionRotatePitch);
		ADD_HANDLER(jump, OnActionJumpOrGlide);
		ADD_HANDLER(deployA, OnActionDeployAir);
		ADD_HANDLER(deployG, OnActionDeployGround);
		ADD_HANDLER(crouch, OnActionCrouch);
		ADD_HANDLER(sprint, OnActionSprint);
		ADD_HANDLER(leanleft, OnActionLeanLeft);
		ADD_HANDLER(leanright, OnActionLeanRight);
		ADD_HANDLER(use, OnActionUse);
		ADD_HANDLER(attack1_xi, OnActionAttackRightTrigger);

		ADD_HANDLER(suitmode_menu_open, OnActionSuitMenuOpen);
		ADD_HANDLER(suitmode_menu_close, OnActionSuitMenuClose);
		ADD_HANDLER(suitmode_menu_closenoselect, OnActionSuitMenuCloseNoSelect);
		ADD_HANDLER(suitmode_menu_manouver, OnActionSwitchSuitMode);
		ADD_HANDLER(suitmode_menu_energy, OnActionSwitchSuitMode);
		ADD_HANDLER(suitmode_menu_combat, OnActionSwitchSuitMode);
		ADD_HANDLER(suitmode_menu_infiltrate, OnActionSwitchSuitMode);
		ADD_HANDLER(suitselectxaxis, OnActionSuitSelectXAxis);
		ADD_HANDLER(suitselectyaxis, OnActionSuitSelectYAxis);

		ADD_HANDLER(suitPower, OnActionSuitPower);
		ADD_HANDLER(special, OnActionStealthKill);

		ADD_HANDLER(thirdperson, OnActionThirdPerson);
		ADD_HANDLER(flymode, OnActionFlyMode);
		ADD_HANDLER(godmode, OnActionGodMode);
		ADD_HANDLER(toggleaidebugdraw, OnActionAIDebugDraw);
		ADD_HANDLER(togglepdrawhelpers, OnActionPDrawHelpers);
		ADD_HANDLER(toggledmode, OnActionDMode);

		ADD_HANDLER(v_rotateyaw, OnActionVRotateYaw); // needed so player can shake unfreeze while in a vehicle
		ADD_HANDLER(v_rotatepitch, OnActionVRotatePitch);

		ADD_HANDLER(xi_v_rotateyaw, OnActionXIRotateYaw);
		ADD_HANDLER(xi_rotateyaw, OnActionXIRotateYaw);
		ADD_HANDLER(xi_rotatepitch, OnActionXIRotatePitch);

		ADD_HANDLER(xi_v_rotatepitch, OnActionXIRotatePitch);
		ADD_HANDLER(xi_movex, OnActionXIMoveX);
		ADD_HANDLER(xi_movey, OnActionXIMoveY);
		ADD_HANDLER(xi_disconnect, OnActionXIDisconnect);
		ADD_HANDLER(xi_usereload, OnActionUse);

		ADD_HANDLER(hud_show_pda_map, OnActionMap);

		ADD_HANDLER(invert_mouse, OnActionInvertMouse);

		ADD_HANDLER(flycam_movex, OnActionFlyCamMoveX);
		ADD_HANDLER(flycam_movey, OnActionFlyCamMoveY);
		ADD_HANDLER(flycam_moveup, OnActionFlyCamMoveUp);
		ADD_HANDLER(flycam_movedown, OnActionFlyCamMoveDown);
		ADD_HANDLER(flycam_speedup, OnActionFlyCamSpeedUp);
		ADD_HANDLER(flycam_speeddown, OnActionFlyCamSpeedDown);
		ADD_HANDLER(flycam_turbo, OnActionFlyCamTurbo);
		ADD_HANDLER(flycam_rotateyaw, OnActionFlyCamRotateYaw);
		ADD_HANDLER(flycam_rotatepitch, OnActionFlyCamRotatePitch);
		ADD_HANDLER(flycam_setpoint, OnActionFlyCamSetPoint);
		ADD_HANDLER(flycam_play, OnActionFlyCamPlay);
		ADD_HANDLER(flycam_clear, OnActionFlyCamClear);
		
	#undef ADD_HANDLER
	}

	CCCPOINT(PlayerState_SetInputCallbacks);
}

CPlayerInput::~CPlayerInput()
{
	m_pPlayer->GetGameObject()->ReleaseActions(this);
}

void CPlayerInput::Reset()
{
	m_actions = 0;
	m_lastActions = m_actions;
	m_actionFlags = CPlayer::eAF_NONE;

	m_deltaMovement.zero();
	m_xi_deltaMovement.zero();
	m_filteredDeltaMovement.zero();
	m_jumpPressTime = 0.f;
	m_deltaRotation.Set(0,0,0);
	m_xi_deltaRotation.Set(0,0,0);
	m_xi_deltaRotationRaw.Set(0.0f, 0.0f, 0.0f);
	m_flyCamDeltaMovement.Set(0,0,0);
	m_flyCamDeltaRotation.Set(0,0,0);
	m_flyCamTurbo=false;
	m_bDisabledXIRot = false;
	m_moveButtonState = 0;
	m_lastSerializeFrameID = 0;
	m_requestSuperJump = false;
	m_lastSensitivityFactor = 1.0f;
	m_lastRegisteredInputTime = gEnv->pTimer->GetAsyncCurTime();
	m_filteredSuitSelectInputX = 0.0f;
	m_filteredSuitSelectInputY = 0.0f;
	m_suitSelectInputX = 0.0f;
	m_suitSelectInputY = 0.0f;
	m_suitMenuOpenButtonPressedTime = 0.0f;
}

void CPlayerInput::DisableXI(bool disabled)
{
	m_bDisabledXIRot = disabled;
}

void CPlayerInput::ApplyMovement(Vec3 delta)
{
	//m_deltaMovement += delta;
	m_deltaMovement.x = clamp_tpl(m_deltaMovement.x+delta.x,-1.0f,1.0f);
	m_deltaMovement.y = clamp_tpl(m_deltaMovement.y+delta.y,-1.0f,1.0f);
	m_deltaMovement.z = 0;

	//static float color[] = {1,1,1,1};
	//gEnv->pRenderer->Draw2dLabel(100,50,1.5,color,false,"m_deltaMovement:%f,%f (requested:%f,%f", m_deltaMovement.x, m_deltaMovement.y,delta.x,delta.y);
}

void CPlayerInput::OnAction( const ActionId& actionId, int activationMode, float value )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

#if defined(USER_timf)
	CryLogAlways ("$7PLAYER INPUT:$o <FRAME %05d> $6%s $%c[MODE=0x%x] $4value=%.3f$o", gEnv->pRenderer->GetFrameID(false), actionId.c_str(), (char) ((activationMode > 7) ? '8' : (activationMode + '1')), activationMode, value);
#endif

	CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );

	m_lastActions = m_actions;
	m_lastRegisteredInputTime = gEnv->pTimer->GetAsyncCurTime();

	//this tell if OnAction have to be forwarded to scripts, now its true by default, only high framerate actions are ignored
	bool filterOut = true;
	const CGameActions& actions = g_pGame->Actions();
	IVehicle* pVehicle = m_pPlayer->GetLinkedVehicle();

	bool canMove = CanMove();

	// disable movement while standing up
	if (!canMove)
		m_deltaMovement.zero();

	// try to dispatch action to OnActionHandlers
	bool handled = false;

	if(OnActionPerks(m_pPlayer->GetEntityId(), actionId, activationMode, value))
	{
		return;
	}

	if(!SAFE_HUD_FUNC_RET(IsPDAActive()))
	{
		FRAME_PROFILER("New Action Processing", GetISystem(), PROFILE_GAME);
		handled = s_actionHandler.Dispatch(this, m_pPlayer->GetEntityId(), actionId, activationMode, value, filterOut);
	}

	//------------------------------------

	{
		FRAME_PROFILER("Regular Action Processing", GetISystem(), PROFILE_GAME);
		bool inKillCam = g_pGame->GetRecordingSystem() && (g_pGame->GetRecordingSystem()->IsPlayingBack() || g_pGame->GetRecordingSystem()->IsPlaybackQueued());
		if (!handled)
		{
			filterOut = true;			
			if (!m_pPlayer->GetSpectatorMode() && !inKillCam)
			{
				if (actions.ulammo==actionId && m_pPlayer->m_pGameFramework->CanCheat() && gEnv->pSystem->IsDevMode())
				{
					g_pGameCVars->g_infiniteAmmo = 1;
				}
				else if (actions.debug_ag_step == actionId)
				{
					gEnv->pConsole->ExecuteString("ag_step");
				}
				else if(actions.voice_chat_talk == actionId)
				{
					//CAUTION:	After the latest integrate from frankfurt, voice is crashing with a floating
					//					point exception
					/*
					if(gEnv->bMultiplayer)
					{
						if(activationMode == eAAM_OnPress)
							g_pGame->GetIGameFramework()->EnableVoiceRecording(true);
						else if(activationMode == eAAM_OnRelease)
							g_pGame->GetIGameFramework()->EnableVoiceRecording(false);
					}
					*/
				}
			}
		}

		if (!m_pPlayer->GetSpectatorMode() && !inKillCam )
		{
			IInventory* pInventory = m_pPlayer->GetInventory();
			if (!pInventory)
				return;

			bool binoculars = false;
			bool scope = false;
			bool allowSwitch = true;
			EntityId itemId = pInventory->GetCurrentItem();
			CWeapon *pWeapon = 0;
			if (itemId)
			{
				pWeapon = m_pPlayer->GetWeapon(itemId);
				if (pWeapon)
				{
					binoculars = (pWeapon->GetEntity()->GetClass() == CItem::sBinocularsClass);
					scope = (pWeapon->IsZoomed() && pWeapon->GetMaxZoomSteps()>1);
					allowSwitch = pWeapon->CanDeselect();
				}
			}

			if (pVehicle)
			{
				float vehicleValue = value;

				if (actionId == actions.xi_v_movex)	
				{
						vehicleValue = MapControllerValue(value, g_pGameCVars->vehicle_steering_curve_scale, g_pGameCVars->vehicle_steering_curve, false);
				}
				else if (actionId == actions.xi_v_accelerate)
				{
						vehicleValue = MapControllerValue(value, g_pGameCVars->vehicle_acceleration_curve_scale, g_pGameCVars->vehicle_acceleration_curve, false);
				}
				else if (actionId == actions.xi_v_deccelerate)
				{
						vehicleValue = MapControllerValue(value, g_pGameCVars->vehicle_deceleration_curve_scale, g_pGameCVars->vehicle_deceleration_curve, false);
				}
					
				if (m_pPlayer->m_pVehicleClient)
				{
					m_pPlayer->m_pVehicleClient->OnAction(pVehicle, m_pPlayer->GetEntityId(), actionId, activationMode, vehicleValue);
				}

				//FIXME:not really good
				m_actions = 0;
				m_actionFlags = CPlayer::eAF_NONE;
//			m_deltaRotation.Set(0,0,0);
				m_deltaMovement.Set(0,0,0);
			}
			else if (m_pPlayer->GetHealth() > 0 && !m_pPlayer->m_stats.isStandingUp && m_pPlayer->GetGameObject()->GetAspectProfile(eEA_Physics)!=eAP_Sleep && (m_pPlayer->IsOnLedge() == false))
			{
				m_pPlayer->CActor::OnAction(actionId, activationMode, value);

				if (actionId == actions.use)
				{
					const CPlayer::SInteractionInfo& interactionInfo = m_pPlayer->GetCurrentInteractionInfo();
					if (interactionInfo.interactionType == CPlayer::eInteraction_Grab || interactionInfo.interactionType == CPlayer::eInteraction_GrabEnemy)
					{
							m_pPlayer->EnterPickAndThrow( interactionInfo.interactiveEntityId );
					}
				}

				if (!binoculars && allowSwitch)
				{
					if(!scope)
					{
						if ((!pWeapon || !pWeapon->IsMounted()))
						{
							bool bMultiplayer = gEnv->bMultiplayer;

							if (actions.nextitem==actionId)
								m_pPlayer->SelectNextItem(1, true, 0);
							else if (actions.previtem==actionId)
								m_pPlayer->SelectNextItem(-1, true, 0);
							else if (!bMultiplayer && (actions.toggle_grenade == actionId))
								m_pPlayer->SelectNextItem(1, true, "grenade");
							else if (actions.toggle_explosive == actionId)
								m_pPlayer->SelectNextItem(1, true, bMultiplayer ? "explosive|grenade" : "explosive");
							else if (actions.toggle_weapon==actionId)
								m_pPlayer->SelectNextItem(1, true, "primary|secondary");


							else if (actions.debug==actionId)
							{
								if (g_pGame)
								{							
									if (!m_pPlayer->GetInventory()->GetItemByClass(CItem::sDebugGunClass)&& CItem::sDebugGunClass != 0)
										g_pGame->GetWeaponSystem()->DebugGun(0);				
									if (!m_pPlayer->GetInventory()->GetItemByClass(CItem::sRefWeaponClass) && CItem::sRefWeaponClass != 0)
										g_pGame->GetWeaponSystem()->RefGun(0);
								}

								m_pPlayer->SelectNextItem(1, true, actionId.c_str());
							}
						}
					}
					else
					{
						if (actions.toggle_weapon==actionId)
							m_pPlayer->SelectNextItem(1, true, "primary|secondary");
						else if (actions.toggle_explosive==actionId)
							m_pPlayer->SelectNextItem(1, true, "explosive");
					}
				}
			}
		}
	}

	g_pGame->GetHUD()->OnAction(actionId, activationMode, value);

	//send the onAction to scripts, after filter the range of actions. for now just use and hold
	if (filterOut)
	{
		FRAME_PROFILER("Script Processing", GetISystem(), PROFILE_GAME);
		HSCRIPTFUNCTION scriptOnAction(NULL);

		IScriptTable *scriptTbl = m_pPlayer->GetEntity()->GetScriptTable();

		if (scriptTbl)
		{
			scriptTbl->GetValue("OnAction", scriptOnAction);

			if (scriptOnAction)
			{
				char *activation = 0;

				switch(activationMode)
				{
				case eAAM_OnHold:
					activation = "hold";
					break;
				case eAAM_OnPress:
					activation = "press";
					break;
				case eAAM_OnRelease:
					activation = "release";
					break;
				default:
					activation = "";
					break;
				}

				Script::Call(gEnv->pScriptSystem,scriptOnAction,scriptTbl,actionId.c_str(),activation, value);

				if (g_pGame->GetGameRules()->GetActorActionModule())
				{
					g_pGame->GetGameRules()->GetActorActionModule()->OnActorAction(m_pPlayer, actionId, activationMode, value);
				}
			}
		}

		gEnv->pScriptSystem->ReleaseFunc(scriptOnAction);
	}

	{
		FRAME_PROFILER("Final Action Processing", GetISystem(), PROFILE_GAME);

		if(IsDemoPlayback() && actionId == g_pGame->Actions().hud_show_multiplayer_scoreboard && activationMode == eAAM_OnPress)
			g_pGame->GetIGameFramework()->GetIActorSystem()->SwitchDemoSpectator();
	}
}

//this function basically returns a smoothed movement vector, for better movement responsivness in small spaces
const Vec3 &CPlayerInput::FilterMovement(const Vec3 &desired)
{
	float frameTimeCap(min(gEnv->pTimer->GetFrameTime(),0.033f));
	float inputAccel(g_pGameCVars->pl_inputAccel);

	Vec3 oldFilteredMovement = m_filteredDeltaMovement;

	if (desired.len2()<0.01f)
	{
		m_filteredDeltaMovement.zero();
	}
	else if (inputAccel<=0.0f)
	{
		m_filteredDeltaMovement = desired;
	}
	else
	{
		Vec3 delta(desired - m_filteredDeltaMovement);

		float len(delta.len());
		if (len<=1.0f)
			delta = delta * (1.0f - len*0.55f);

		m_filteredDeltaMovement += delta * min(frameTimeCap * inputAccel,1.0f);
	}

	if (oldFilteredMovement.GetDistance(m_filteredDeltaMovement) > 0.001f)
		CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );

	return m_filteredDeltaMovement;
}

bool CPlayerInput::CanMove() const
{
	bool canMove = !m_pPlayer->GetSpectatorMode();
	canMove &=!m_pPlayer->m_stats.isStandingUp;
	return canMove;
}

void CPlayerInput::NormalizeInput(float& fX, float& fY, float fCoeff, float fCurve)
{
	float fMag = MapControllerValue(min(cry_sqrtf(fX*fX + fY*fY), 1.0f), fCoeff, fCurve, false);
	if (fMag > 0.0f)
	{
		float fAbsX = cry_fabsf(fX);
		float fAbsY = cry_fabsf(fY);
		float fFactor = fMag / max(fAbsX, fAbsY);

		fX *= fFactor;
		fY *= fFactor;
	}
}

void CPlayerInput::DrawDebugInfo()
{
	const float fRadius = 60.0f;
	const float fX = 120.0f;
	const float fY = 600.0f;
	const float fTimeOut = 0.5f;
	const float fSize = 12.f;

	// process the input as in PreProcess, but without scaling
	Ang3 processedDeltaRot(UpdateXIInputs(m_xi_deltaRotationRaw, false));

	IUIDraw* pUIDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();
	pUIDraw->PreRender();

	// Draw enclosing circle
	ColorF whiteColor(0.7f, 1.0f, 1.0f, 1.0f);
	// pUIDraw->DrawCircleHollow(fX, fY, fRadius, 1.0f, whiteColor.pack_argb8888());

	// Print explanatory text
	IFFont* pFont = gEnv->pCryFont->GetFont("default");

	string sMsg;
	sMsg.Format("Raw input: (%f, %f)", m_xi_deltaRotationRaw.z, m_xi_deltaRotationRaw.x);
	pUIDraw->DrawTextSimple(pFont, fX - fRadius, fY + fRadius + fSize, fSize, fSize, sMsg.c_str(), Col_Green, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP);

	sMsg.Format("Processed input: (%f, %f)", processedDeltaRot.z, processedDeltaRot.x);
	pUIDraw->DrawTextSimple(pFont, fX - fRadius, fY + fRadius + (fSize * 2.f), fSize, fSize, sMsg.c_str(), Col_Orange, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP);

	pUIDraw->PostRender();

	// to improve following the movement
	IPersistantDebug* pPersistantDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
	pPersistantDebug->Begin("CPlayerInput::DrawDebugInfo", false);

	float fTraceRawXStart = fX + (m_debugDrawStats.lastRaw.z * fRadius);
	float fTraceRawYStart = fY + (-m_debugDrawStats.lastRaw.x * fRadius);

	float fTraceProcessedXStart = fX + (-m_debugDrawStats.lastProcessed.z * fRadius);
	float fTraceProcessedYStart = fY + (-m_debugDrawStats.lastProcessed.x * fRadius);

	float fRawXEnd = fX + (m_xi_deltaRotationRaw.z * fRadius);
	float fRawYEnd = fY + (-m_xi_deltaRotationRaw.x * fRadius);
	float fProcessedXEnd = fX + (-processedDeltaRot.z * fRadius);
	float fProcessedYEnd = fY + (-processedDeltaRot.x * fRadius);

	if ((fProcessedXEnd != fX) && (fProcessedYEnd != fY))
		pPersistantDebug->Add2DLine(fTraceProcessedXStart, fTraceProcessedYStart, fProcessedXEnd, fProcessedYEnd, Col_Orange, fTimeOut);
	if ((fRawXEnd != fX) && (fRawYEnd != fY))
		pPersistantDebug->Add2DLine(fTraceRawXStart, fTraceRawYStart, fRawXEnd, fRawYEnd, Col_Green, fTimeOut);

	// Display our aiming displacement
	const CCamera& camera = gEnv->pRenderer->GetCamera();
	float fDepth = camera.GetNearPlane() + 0.15f;
	Vec3 vNewAimPos = camera.GetPosition() + (camera.GetViewdir() * fDepth);

	pPersistantDebug->AddLine(m_debugDrawStats.vOldAimPos, vNewAimPos, Col_White, fTimeOut);

	// Draw input lines
	pPersistantDebug->Begin("CPlayerInput::DrawDebugInfo::InputLines", true);
	pPersistantDebug->Add2DLine(fX, fY, fProcessedXEnd, fProcessedYEnd, Col_Orange, 0.1f);
	pPersistantDebug->Add2DLine(fX, fY, fRawXEnd, fRawYEnd, Col_Green, 0.1f);

	// store values for next call
	m_debugDrawStats.lastRaw = m_xi_deltaRotationRaw;
	m_debugDrawStats.lastProcessed = processedDeltaRot;
	m_debugDrawStats.vOldAimPos = vNewAimPos;
}

Ang3 CPlayerInput::UpdateXIInputs(const Ang3& inputAngles, bool bScaling/* = true*/)
{
	const SCVars* const __restrict pGameCVars = g_pGameCVars;

	if (pGameCVars->aim_altNormalization.enable == 0)
	{
		Ang3 xiDeltaRot(m_xi_deltaRotationRaw.x, 0.0f, m_xi_deltaRotationRaw.z);

		// Calculate the parameters for the input mapping
		const float fCurve = g_pGameCVars->controller_power_curve;

		// NormalizeInput maps the magnitude internally
		NormalizeInput(xiDeltaRot.z, xiDeltaRot.x, 1.0f, fCurve);
		xiDeltaRot.z = -xiDeltaRot.z;

		return Ang3(xiDeltaRot.x, 0.0f, xiDeltaRot.z);	
	}
	else
	{
		Ang3 xiDeltaRot(m_xi_deltaRotationRaw.x, 0.0f, m_xi_deltaRotationRaw.z);

		// Calculate the parameters for the input mapping
		const float fCoeff = g_pGameCVars->aim_altNormalization.hud_ctrl_Coeff_Unified;
		const float fCurve = g_pGameCVars->aim_altNormalization.hud_ctrl_Curve_Unified;

		// NormalizeInput maps the magnitude internally
		NormalizeInput(xiDeltaRot.z, xiDeltaRot.x, bScaling ? fCoeff : 1.0f, fCurve);
		xiDeltaRot.z = -xiDeltaRot.z;

		return Ang3(xiDeltaRot.x, 0.0f, xiDeltaRot.z);
	}
}

void CPlayerInput::PreUpdate()
{
	CMovementRequest request;

	float generalSensitivity = 1.0f;	//sensitivity adjustment regardless of control type
	float mouseSensitivity;						//sensitivity adjustment specific to mouse control

	float dt = gEnv->pTimer->GetFrameTime();

	mouseSensitivity = 0.00333f*MAX(0.01f, g_pGameCVars->cl_sensitivity);

	mouseSensitivity *= gf_PI / 180.0f;//doesnt make much sense, but after all helps to keep reasonable values for the sensitivity cvars
	
	//Move this to player rotation instead?
	generalSensitivity *= (m_pPlayer->GetWeaponRotationFactor() * m_pPlayer->GetActorSuitGameParameters().GetProps().rotationSpeedScale);

	Interpolate(m_lastSensitivityFactor, generalSensitivity, 4.0f, dt, 1.0f);
	generalSensitivity = m_lastSensitivityFactor;

	Ang3 deltaRotation = m_deltaRotation * mouseSensitivity * generalSensitivity;

	UpdateSuitSelectionInputs(dt);

	// apply rotation from xinput controller
	if(!m_bDisabledXIRot)
	{
		m_xi_deltaRotation = UpdateXIInputs(m_xi_deltaRotationRaw);

		Ang3 xiDeltaRot = m_xi_deltaRotation;

		//Apply turning acceleration to the input values. This is here because the acceleration
		//	should be application-specific, e.g. different for the player and vehicles
		m_pPlayer->m_pMovementController->ApplyControllerAcceleration(xiDeltaRot.x, xiDeltaRot.z, gEnv->pTimer->GetFrameTime());

		xiDeltaRot *=  generalSensitivity;

		// [tlh] slow down rotation speed when using CCTV spectator cam to make it feel more like a CCTV camera's movement
		if (m_pPlayer->GetSpectatorMode() == CActor::eASM_CCTV)
		{
			xiDeltaRot *= g_pGameCVars->g_spectate_cctv_RotScale;
		}

		// Applying aspect modifiers
		if (g_pGameCVars->hud_aspectCorrection > 0)
		{
			int vx, vy, vw, vh;
			gEnv->pRenderer->GetViewport(&vx, &vy, &vw, &vh);
			float med=((float)vw+vh)/2.0f;
			float crW=((float)vw)/med;
			float crH=((float)vh)/med;
			xiDeltaRot.x*=g_pGameCVars->hud_aspectCorrection == 2 ? crW : crH;
			xiDeltaRot.z*=g_pGameCVars->hud_aspectCorrection == 2 ? crH : crW;
		}

		if(g_pGameCVars->cl_invertController)
			xiDeltaRot.x*=-1;
		
		// Controller framerate compensation needs frame time! 
		// The constant is to counter for small frame time values.
		// adjust some too small values, should be handled differently later on
		if(g_pGameCVars->pl_aim_acceleration_enabled == 0)
		{
			deltaRotation += (xiDeltaRot * dt * 50.0f * generalSensitivity * mouseSensitivity);
		}
		else
		{
			float controllerSensitivity = clamp(g_pGameCVars->cl_sensitivityController * 0.5f, 0.0f, 1.0f);
			float fractionIncrease = 1.0f + ((controllerSensitivity - 0.5f) * 0.5f); //result is between 0.75f to 1.25f
			xiDeltaRot.z *= g_pGameCVars->controller_multiplier_z * fractionIncrease;
			xiDeltaRot.x *= g_pGameCVars->controller_multiplier_x * fractionIncrease;

			//Output debug information
			if(g_pGameCVars->ctrlr_OUTPUTDEBUGINFO > 0)
			{
				const float dbg_my_white[4] = {1,1,1,1};
				gEnv->pRenderer->Draw2dLabel( 20, 400, 1.3f, dbg_my_white, false, "PRE-DT MULTIPLY:\n  xRot: %.9f\n  zRot: %.9f\n", xiDeltaRot.x, xiDeltaRot.z);
			}

			deltaRotation += (xiDeltaRot * dt * generalSensitivity);
		}

		IVehicle *pVehicle = m_pPlayer->GetLinkedVehicle();
		if (pVehicle)
		{
			if (m_pPlayer->m_pVehicleClient)
			{
				m_pPlayer->m_pVehicleClient->PreUpdate(pVehicle, m_pPlayer->GetEntityId());
			}

			//FIXME:not really good
			m_actions = 0;
			m_actionFlags = CPlayer::eAF_NONE;
			m_deltaMovement.Set(0,0,0);
			m_deltaRotation.Set(0,0,0);
		}
	}

	if(m_bUseXIInput)
	{
		m_deltaMovement.x = m_xi_deltaMovement.x;
		m_deltaMovement.y = m_xi_deltaMovement.y;
		m_deltaMovement.z = 0;

		if (m_xi_deltaMovement.len2()>0.0f)
			m_actions |= ACTION_MOVE;
		else
			m_actions &= ~ACTION_MOVE;
	}

	bool animControlled(m_pPlayer->m_stats.animationControlled);

	// If there was a recent serialization, ignore the delta rotation, since it's accumulated over several frames.
	if ((m_lastSerializeFrameID + 2) > gEnv->pRenderer->GetFrameID())
		deltaRotation.Set(0,0,0);


	if (!animControlled)
	{
		request.AddDeltaRotation( deltaRotation );
		request.AddDeltaMovement( FilterMovement(m_deltaMovement) );
	}

	// handle actions
	if ((m_actions & ACTION_JUMP) || m_requestSuperJump)
	{
		request.SetJump();		

		m_requestSuperJump = false;
	}

	request.SetStance(FigureOutStance());

	float pseudoSpeed = 0.0f;
	if (m_deltaMovement.len2() > 0.0f)
	{
		pseudoSpeed = m_pPlayer->CalculatePseudoSpeed(m_pPlayer->m_stats.bSprinting);
	}

	request.SetPseudoSpeed(pseudoSpeed);

	if (m_deltaMovement.GetLength() > 0.1f)
	{
		float moveAngle = (float)RAD2DEG(fabs_tpl(cry_atan2f(-m_deltaMovement.x, fabsf(m_deltaMovement.y)<0.01f?0.01f:m_deltaMovement.y)));
		request.SetAllowStrafing(moveAngle > 20.0f);
	}
	else
	{
		request.SetAllowStrafing(true);
	}

	// send the movement request to the appropriate spot!
	m_pPlayer->m_pMovementController->RequestMovement( request );
	m_pPlayer->m_actions = m_actions;
	m_pPlayer->m_actionFlags = m_actionFlags;

	if ((m_pPlayer->GetSpectatorMode() == CActor::eASM_CCTV) && ((deltaRotation.x != 0.f) || (deltaRotation.z != 0.f)))
	{
		// TODO might not need to do the RequestMovement() calls (and maybe other things) if coming through here?
		CCTVSpectatorPreUpdate(&deltaRotation);
	}

	if (g_pGameCVars->g_detachCamera && g_pGameCVars->g_moveDetachedCamera)
	{
		HandleMovingDetachedCamera(m_flyCamDeltaRotation, m_flyCamDeltaMovement);
	}

	// reset things for next frame that need to be
	m_deltaRotation = Ang3(0,0,0);

	if (m_pPlayer->GetFlyMode() == 0)
	{
		m_actions &= ~ACTION_JUMP;
		m_actionFlags &= ~(CPlayer::eAF_JUMP_QUICK);
	}

	//static float color[] = {1,1,1,1};    
  //gEnv->pRenderer->Draw2dLabel(100,50,1.5,color,false,"deltaMovement:%f,%f", m_deltaMovement.x,m_deltaMovement.y);
}

void CPlayerInput::UpdateSuitSelectionInputs(float dt)
{
	CNanoSuit* pNanoSuit = m_pPlayer->GetNanoSuit();

	ENanoSuitMode currentlyHighlightedMode = eNanoSuitMode_Invalid;

	if(pNanoSuit && m_suitMenuOpen)
	{
		if(fabsf(m_suitSelectInputX) > fabsf(m_filteredSuitSelectInputX))
		{
			m_filteredSuitSelectInputX = m_suitSelectInputX;
		}
		else
		{
			m_filteredSuitSelectInputX = LERP(m_filteredSuitSelectInputX, m_suitSelectInputX, dt);
		}

		if(fabsf(m_suitSelectInputY) > fabsf(m_filteredSuitSelectInputY))
		{
			m_filteredSuitSelectInputY = m_suitSelectInputY;
		}
		else
		{
			m_filteredSuitSelectInputY = LERP(m_filteredSuitSelectInputY, m_suitSelectInputY, dt);
		}

		currentlyHighlightedMode = GetSuitModeFromAxes();	
	}
	else
	{
		ClearSuitSelectionInputs();
	}

	//TODO: send to nanosuit
	if(m_lastHighlightedMode != currentlyHighlightedMode)
	{
		pNanoSuit->ActivateMode(currentlyHighlightedMode);
		m_lastHighlightedMode = currentlyHighlightedMode;
	}	
}

void CPlayerInput::ClearSuitSelectionInputs()
{
	m_filteredSuitSelectInputX = 0.f;
	m_filteredSuitSelectInputY = 0.f;
	m_suitSelectInputX = 0.f;
	m_suitSelectInputY = 0.f;
	m_lastHighlightedMode = eNanoSuitMode_Invalid;
}

void CPlayerInput::CCTVSpectatorPreUpdate(const Ang3* deltaRotation)
{
	assert(deltaRotation);
	assert((deltaRotation->x != 0.f) || (deltaRotation->z != 0.f));
	assert(m_pPlayer->GetSpectatorMode() == CActor::eASM_CCTV);

	//CryLogAlways("delta rot %f %f %f", deltaRotation->x, deltaRotation->y, deltaRotation->z);

	EntityId  cctvEid = m_pPlayer->GetSpectatorCCTVCam();
	assert(cctvEid);
	IEntity*  cctvEnt = gEnv->pEntitySystem->GetEntity(cctvEid);
	assert(cctvEnt);

	const Vec3  cctvPos = cctvEnt->GetWorldPos();
	const Ang3  cctvAng = cctvEnt->GetWorldAngles();

	IView*  pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
	assert(pView);
	const SViewParams*  pViewParams = pView->GetCurrentParams();

	USpectatorModeData::SCctv*  cctvData = &m_pPlayer->m_stats.spectatorInfo.dataU.cctv;

	cctvData->rotOffset.x += (0.1f * deltaRotation->x);
	cctvData->rotOffset.z += (0.1f * deltaRotation->z);

	float  limitV, limitH;
	GetCCTVEntityRotationLimits(cctvEid, &limitV, &limitH);  // these values could probably be cached (calculated in the "changed camera" block above perhaps) if wanted
	if (fabsf(cctvData->rotOffset.x) > limitV)
		cctvData->rotOffset.x = ((cctvData->rotOffset.x >= 0.f) ? limitV : -limitV);
	if (fabsf(cctvData->rotOffset.z) > limitH)
		cctvData->rotOffset.z = ((cctvData->rotOffset.z >= 0.f) ? limitH : -limitH);

	Quat  rot = Quat::CreateIdentity();
	rot = (rot * Quat::CreateRotationZ(cctvAng.z + cctvData->rotOffset.z));
	rot = (rot * Quat::CreateRotationX(cctvAng.x + cctvData->rotOffset.x));

	SViewParams  params = (*pViewParams);
	params.rotation = rot;
	pView->SetCurrentParams(params);
}

// TODO - tidy up
// TODO - add up down movement on analogue shoulder buttons
// TODO - perhaps move to somewhere else more suitable?
void CPlayerInput::HandleMovingDetachedCamera(const Ang3 &deltaRotation, const Vec3 &deltaMovement)
{
	//const Vec3  cctvPos = cctvEnt->GetWorldPos();
	//const Ang3  cctvAng = cctvEnt->GetWorldAngles();

	//CryWatch("deltaRot=%f,%f,%f; deltaMove=%f,%f,%f", deltaRotation.x, deltaRotation.y, deltaRotation.z, deltaMovement.x, deltaMovement.y, deltaMovement.z);
	IView*  pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
	assert(pView);

	if (!pView)
	{
		return;
	}

#if FREE_CAM_SPLINE_ENABLED
	if (g_pGameCVars->g_detachedCameraDebug)
	{
		ColorB col(255,255,255);
		ColorB col2(255,255,130);

		SFreeCamPointData *camData = NULL;
		SFreeCamPointData *lastCamData = NULL;
		float totalDistance = 0.f;
		Vec3 diff;

		for (int num=0; num < MAX_FREE_CAM_DATA_POINTS; ++num)
		{
			camData = &m_freeCamData[num];
			if (camData->valid)
			{
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawPoint(camData->position, col, 3);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawPoint(camData->lookAtPosition, col2, 3);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(camData->position, col, camData->lookAtPosition, col);

				if (lastCamData)
				{
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(camData->position, col, lastCamData->position, col, 3.f);
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(camData->lookAtPosition, col2, lastCamData->lookAtPosition, col2, 3.f);
				}
			}
			else
			{
				break;
			}

			lastCamData = camData;
		}
	}

	if (m_freeCamPlaying)
	{
		if (m_freeCamSpline.num_keys() > 0)
		{
			const SViewParams*  pViewParams = pView->GetCurrentParams();

			SViewParams  params = (*pViewParams);

			float frameTime=gEnv->pTimer->GetFrameTime();
			m_freeCamPlayTimer += frameTime;

			Vec3 point, lookAtPoint;

			float time = CLAMP((m_freeCamPlayTimer / m_freeCamTotalPlayTime), 0.f, 1.f);

			m_freeCamSpline.interpolate( time, point);
			m_freeCamLookAtSpline.interpolate( time, lookAtPoint);

			Vec3 diff = lookAtPoint - point;
			diff.Normalize();
			Quat lookAtQuat = Quat::CreateRotationVDir( diff );

			params.rotation = lookAtQuat;
			params.position = point;
			pView->SetCurrentParams(params);

			if (m_freeCamPlayTimer > m_freeCamTotalPlayTime)
			{
				if (g_pGameCVars->g_flyCamLoop)
				{
					m_freeCamPlayTimer = 0;
				}
				else
				{
					m_freeCamPlaying = false;
					m_freeCamPlayTimer = m_freeCamTotalPlayTime;
				}
			}
		}
		else
		{
			m_freeCamPlaying = false;
		}
	}
	else
#endif
	{
		float moveSpeed = g_pGameCVars->g_detachedCameraMoveSpeed;
		if (m_flyCamTurbo)
		{
			moveSpeed *= g_pGameCVars->g_detachedCameraTurboBoost;
		}

		const SViewParams*  pViewParams = pView->GetCurrentParams();

		Quat curRot = pViewParams->rotation;
		Quat newRot;
		Vec3 upDir = curRot.GetColumn2();
		Vec3 rightDir = curRot.GetColumn0();
		//Quat::CreateRotationXYZ()
		newRot = curRot;

		float frameTime=gEnv->pTimer->GetFrameTime();

		Ang3 useRot;
		useRot.x = MapControllerValue(deltaRotation.x, g_pGameCVars->g_detachedCameraRotateSpeed, 2.5f, false);
		useRot.y = MapControllerValue(deltaRotation.y, g_pGameCVars->g_detachedCameraRotateSpeed, 2.5f, false);
		useRot.z = MapControllerValue(deltaRotation.z, g_pGameCVars->g_detachedCameraRotateSpeed, 2.5f, true);

		// space of transformation done left to right
		// rotz in world space (or no space)
		// newRot is applied to worldspaced rotz
		// rotx in current view space already worldspaced rotz (as held in newRot)
		newRot = Quat::CreateRotationZ(frameTime*useRot.z) * newRot * (Quat::CreateRotationX(frameTime*useRot.x) ) ;
		newRot.Normalize();

		Vec3 movement; // don't call FilterMovement() its actually doing state things as well
		movement.x = MapControllerValue(deltaMovement.x, moveSpeed, 2.5f, false);
		movement.y = MapControllerValue(deltaMovement.y, moveSpeed, 2.5f, false);
		movement.z = MapControllerValue(deltaMovement.z, moveSpeed, 2.5f, false);

		rightDir = newRot.GetColumn0();
		upDir = newRot.GetColumn2();
		Vec3 lookDir = newRot.GetColumn1();

		lookDir *= movement.y * frameTime;
		rightDir *= movement.x * frameTime;
		upDir *= movement.z * frameTime;

		movement = lookDir + rightDir + upDir;

		SViewParams  params = (*pViewParams);
		params.position += movement;
		params.rotation = newRot;
		pView->SetCurrentParams(params);
	}
}

void CPlayerInput::GetCCTVEntityRotationLimits(EntityId cctvEid, float *outLimitV, float *outLimitH)  // [static]
{
	IEntity*  cctv = gEnv->pEntitySystem->GetEntity(cctvEid);

	if (cctv)
	{
		if (outLimitV) (*outLimitV) = DEG2RAD(25.f);  // default values incase properties cannot be found...
		if (outLimitH) (*outLimitH) = DEG2RAD(25.f);  // ...

		IScriptTable  *pScriptTable = cctv->GetScriptTable();
		SmartScriptTable  props;
		if (pScriptTable && pScriptTable->GetValue("Properties", props))
		{
			float  rotLimitUD, rotLimitLR;
			if (outLimitV && props->GetValue("fRotLimitUD", rotLimitUD))
			{
				(*outLimitV) = DEG2RAD(rotLimitUD/2.f);
			}
			if (outLimitH && props->GetValue("fRotLimitLR", rotLimitLR))
			{
				(*outLimitH) = DEG2RAD(rotLimitLR/2.f);
			}
		}
	}
}

EStance CPlayerInput::FigureOutStance()
{
	if (m_actions & ACTION_CROUCH)
		return STANCE_CROUCH;
	else if (m_actions & ACTION_RELAXED)
		return STANCE_RELAXED;
	else if (m_actions & ACTION_STEALTH)
		return STANCE_STEALTH;
	else if (m_pPlayer->GetStance() == STANCE_NULL)
		return STANCE_STAND;
	return STANCE_STAND;
}

void CPlayerInput::Update()
{
	if (m_suitMenuOpenButtonPressedTime > 0.01f)
	{
		if(gEnv->pTimer->GetAsyncCurTime() - m_suitMenuOpenButtonPressedTime > g_pGameCVars->g_nanoSuitMenuToggleTime)
		{
			OpenSuitMenu();
		}
	}

	if (SPlayerStats* pStats = static_cast<SPlayerStats*>(m_pPlayer->GetActorStats()))
	{
		if(pStats->bSprinting)
		{
			if (m_actions & ACTION_CROUCH)
			{
				m_actions &= ~ACTION_SPRINT;
			}
		}
	}

	if (m_jumpPressTime > 0.f)
	{
		float  pressDelta = (gEnv->pTimer->GetAsyncCurTime() - m_jumpPressTime);
		if (pressDelta > g_pGameCVars->pl_jump_quickPressThresh)
		{
			//CryLog("[tlh] calling PerformJump(false) from PlayerInput::Update()");
			m_jumpPressTime = 0.f;
			PerformJump(false);
		}
	}
}

// [tlh] TODO? copy-pasted from HUDTagNames.cpp (where it's implicitly static - ie. not in any header files) ... shouldn't this be in the engine alongside the ProjectToScreen funcs?
static bool CoordsOnScreen(const Vec3& vScreenSpace)
{
	bool bResult = true;

	if(vScreenSpace.z < 0.0f || vScreenSpace.z > 1.0f)
	{
		bResult = false;
	}

	if(vScreenSpace.y < 0.0f || vScreenSpace.y > 100.0f)
	{
		bResult = false;
	}

	if(vScreenSpace.x < 0.0f || vScreenSpace.x > 100.0f)
	{
		bResult = false;
	}

	return bResult;
}

void CPlayerInput::PostUpdate()
{
	if (m_actions!=m_lastActions)
		CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );

	m_actions &= ~(ACTION_LEANLEFT | ACTION_LEANRIGHT);

	// Debug Drawing
	if (g_pGameCVars->pl_debug_aiming_input && !m_bDisabledXIRot)
		DrawDebugInfo();

	if (CGameRules* pGameRules=static_cast< CGameRules* >( gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules() ))
	{
		if (IGameRulesSpectatorModule* specmod=pGameRules->GetSpectatorModule())
		{
			specmod->PostUpdateSpectatorCCTVTagging();  // TODO it'd be nice to be able to call this from elsewhere, but for now this is the only place i know of in the loop where the 2D screen projection functions will work from (which this function needs)
		}
	}
}

void CPlayerInput::GetState( SSerializedPlayerInput& input )
{
	SMovementState movementState;
	m_pPlayer->GetMovementController()->GetMovementState( movementState );

	Quat worldRot = m_pPlayer->GetBaseQuat();
	input.stance = FigureOutStance();
	input.deltaMovement = worldRot.GetNormalized() * m_filteredDeltaMovement;
	// ensure deltaMovement has the right length
	input.deltaMovement = input.deltaMovement.GetNormalizedSafe(ZERO) * m_filteredDeltaMovement.GetLength();
	input.sprint = ((m_actions & ACTION_SPRINT) != 0);

	input.leanl = (m_actions & ACTION_LEANLEFT) != 0;
	input.leanr = (m_actions & ACTION_LEANRIGHT) != 0;
	input.lookDirection = movementState.eyeDirection;
	input.bodyDirection = movementState.bodyDirection;
	input.aiming = true;
	input.usinglookik = true;
	input.allowStrafing = true;

	float pseudoSpeed=0.0f;
	if (input.deltaMovement.len2() > 0.0f)
		pseudoSpeed = m_pPlayer->CalculatePseudoSpeed(input.sprint);
	input.pseudoSpeed=pseudoSpeed;
}

void CPlayerInput::SetState( const SSerializedPlayerInput& input )
{
	GameWarning("CPlayerInput::SetState called: should never happen");
}

void CPlayerInput::SerializeSaveGame( TSerialize ser )
{
	if(ser.GetSerializationTarget() != eST_Network)
	{
		// Store the frame we serialize, to avoid accumulated input during serialization.
		m_lastSerializeFrameID = gEnv->pRenderer->GetFrameID();

		if(ser.IsReading())
		{
			Reset();
		}
	}
}

bool CPlayerInput::OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove())
	{
		if(activationMode == 2)
		{
			if(!(m_moveButtonState&eMBM_Left) && !(m_moveButtonState&eMBM_Back) && !(m_moveButtonState&eMBM_Right))
			{
				m_actions &= ~ACTION_MOVE;
			}
		}
		else 
		{
			m_actions |= ACTION_MOVE;
		}

		if(CheckMoveButtonStateChanged(eMBM_Forward, activationMode))
		{
			ApplyMovement(Vec3(0,value*2.0f - 1.0f,0));
			AdjustMoveButtonState(eMBM_Forward, activationMode);
		}

	}

	return false;
}

bool CPlayerInput::OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove())
	{
		if(activationMode == eAAM_OnRelease)
		{
			if(!(m_moveButtonState&eMBM_Left) && !(m_moveButtonState&eMBM_Forward) && !(m_moveButtonState&eMBM_Right))
			{
				m_actions &= ~ACTION_MOVE;
			}
		}
		else
		{
			m_actions |= ACTION_MOVE;
		}

		if(CheckMoveButtonStateChanged(eMBM_Back, activationMode))
		{
			ApplyMovement(Vec3(0,-(value*2.0f - 1.0f),0));
			AdjustMoveButtonState(eMBM_Back, activationMode);
		}
	}

	return false;
}

bool CPlayerInput::OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove())
	{
		if(activationMode == eAAM_OnRelease)
		{
			if(!(m_moveButtonState&eMBM_Forward) && !(m_moveButtonState&eMBM_Back) && !(m_moveButtonState&eMBM_Right))
			{
				m_actions &= ~ACTION_MOVE;
		}
		}
		else
		{
			m_actions |= ACTION_MOVE;
		}

		if(CheckMoveButtonStateChanged(eMBM_Left, activationMode))
		{
			ApplyMovement(Vec3(-(value*2.0f - 1.0f),0,0));
			AdjustMoveButtonState(eMBM_Left, activationMode);
		}
	}

	return false;
}

bool CPlayerInput::OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove())
	{
		if(activationMode == eAAM_OnRelease)
		{
			if(!(m_moveButtonState&eMBM_Left) && !(m_moveButtonState&eMBM_Back) && !(m_moveButtonState&eMBM_Forward))
			{
				m_actions &= ~ACTION_MOVE;
		}
		}
		else
		{
			m_actions |= ACTION_MOVE;
		}

		if(CheckMoveButtonStateChanged(eMBM_Right, activationMode))
		{
			ApplyMovement(Vec3(value*2.0f - 1.0f,0,0));
			AdjustMoveButtonState(eMBM_Right, activationMode);
		}
	}

	return false;
}

bool CPlayerInput::OnActionRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_deltaRotation.z -= value;

	if((m_actions & ACTION_SPRINT) && g_pGameCVars->g_enableSpeedLean)
	{
		if(value < 0.0f && m_speedLean > 0.0f)
		{
			m_speedLean = 0.0f;
		}
		else if(value > 0.0f && m_speedLean < 0.0f)
		{
			m_speedLean = 0.0f;
		}

		m_pPlayer->SetSpeedLean(m_speedLean);
	}

	return false;
}

bool CPlayerInput::OnActionRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(g_pGameCVars->cl_invertMouse)
	{
		value *= -1.0f;
	}

	m_deltaRotation.x -= value;

	return false;
}

bool CPlayerInput::OnActionVRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_deltaRotation.z -= value;

	return false;
}

bool CPlayerInput::OnActionVRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_deltaRotation.x -= value;
	if(g_pGameCVars->cl_invertMouse)
		m_deltaRotation.x*=-1.0f;

	return false;
}

//===============================================================
bool CPlayerInput::OnActionSuitPower(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(gEnv->bMultiplayer && g_pGameCVars->g_mpCoreGameplayTesting)	//Core gameplay testing
		return false;

	if (activationMode != eAAM_OnPress)
		return false;

	SNanoSuitEvent suitEvent;
	suitEvent.event = eNanoSuitEvent_SUITPOWER;
	m_pPlayer->SendActorSuitEvent(suitEvent);

	return false;
}

bool CPlayerInput::OnActionDeployGround(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	int team = g_pGame->GetGameRules()->GetTeam(entityId);
	IEntity* pPlayerEnt = m_pPlayer->GetEntity();

	SEntitySpawnParams params;
	params.sName = "DeployableTurret";
	params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DeployableTurret");
	params.nFlags |= (ENTITY_FLAG_NO_PROXIMITY|ENTITY_FLAG_NEVER_NETWORK_STATIC);
	params.vPosition = pPlayerEnt->GetWorldPos();
	params.qRotation = pPlayerEnt->GetRotation();

	Matrix34 mtx(params.qRotation);
	Vec3 forward(0.f,2.f,0.f);

	params.vPosition += mtx * forward;

	params.vPosition.z += 0.5f;

	IEntity* pItemEnt = gEnv->pEntitySystem->SpawnEntity(params);

	CGunTurret* pGunTurret = static_cast<CGunTurret*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pItemEnt->GetId()));

	if(pGunTurret)
	{
		pGunTurret->SetTeamNum(team);
	}

	return true;
}

bool CPlayerInput::OnActionDeployAir(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	int team = g_pGame->GetGameRules()->GetTeam(entityId);
	IEntity* pPlayerEnt = m_pPlayer->GetEntity();

	Vec3 position = pPlayerEnt->GetWorldPos();
	Quat rotation = pPlayerEnt->GetRotation();

	Matrix34 mtx(rotation);
	Vec3 forward(0.f,2.f,0.f);

	forward = mtx * forward;

	position += forward;
	position.z += 0.5f;

	Vec3 spawnPos;
	
	if (CGunTurret::QueryDeployment(position, forward.normalized(), 0.5f, spawnPos))
	{
		SEntitySpawnParams params;

		params.sName = "DeployableHoverTurret";
		params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DeployableHoverTurret");
		params.nFlags |= (ENTITY_FLAG_NO_PROXIMITY|ENTITY_FLAG_NEVER_NETWORK_STATIC);
		params.vPosition = spawnPos;
		params.qRotation = rotation;
		IEntity* pItemEnt = gEnv->pEntitySystem->SpawnEntity(params);    

		CGunTurret* pGunTurret = static_cast<CGunTurret*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pItemEnt->GetId()));

		if(pGunTurret)
		{
			pGunTurret->SetTeamNum(team);
		}
	}

	return true;
}

bool CPlayerInput::OnActionJumpOrGlide(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	bool canJump = ((m_pPlayer->GetStance() == STANCE_SWIM) ||
									 m_pPlayer->TrySetStance(STANCE_STAND)) && 
									 (m_pPlayer->m_stats.onGround > 0.01f);

	bool  performJump = false;
	bool  performQuickJump = false;

	if (CanMove() && canJump && m_pPlayer->GetSpectatorMode() != CActor::eASM_Free && m_pPlayer->GetSpectatorMode() != CActor::eASM_Fixed)
	{
		if (value > 0.0f)
		{
			if(m_actions & ACTION_CROUCH)
			{
				CCCPOINT(PlayerMovement_PressJumpWhileCrouchedToStandUp);
				m_actions &= ~ACTION_CROUCH;
				return false;
			}

			if (m_pPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power)
			{
				CRY_ASSERT(m_jumpPressTime <= 0.f);
				m_jumpPressTime = gEnv->pTimer->GetAsyncCurTime();
				return true;
			}

			performJump = true;
		}
		else
		{
			if (m_jumpPressTime > 0.f)
			{
				float  pressDelta = (gEnv->pTimer->GetAsyncCurTime() - m_jumpPressTime);
				//CryLog("[tlh] @ CPlayerInput::OnActionJump(), pressDelta = %f", pressDelta);
				m_jumpPressTime = 0.f;
				performQuickJump = true;
			}
		}
	}
	else
	{
		CCCPOINT_IF(value > 0.0f, PlayerMovement_PressJumpInputIgnored);
		m_jumpPressTime = 0.f;
	}

	if (performJump || performQuickJump)
	{
		PerformJump(performQuickJump);
	}
	else
	{
		// Moved this outside the (CanMove() && ...) condition, since if it's false the JUMP flag might not be cleared, 
		// and the player continues jumping as if the jump key was held.
		if ((m_pPlayer->GetFlyMode() == 0) || (activationMode == eAAM_OnRelease))
		{
			m_actions &= ~ACTION_JUMP;
			m_actionFlags &= ~CPlayer::eAF_JUMP_QUICK;
		}
	}

	CHANGED_NETWORK_STATE(m_pPlayer, CPlayer::ASPECT_INPUT_CLIENT);

	return false;
}

bool CPlayerInput::PerformJump(bool quickPress)
{
	//CryLog("[tlh] @ CPlayerInput::PerformJump(quickPress=%d)", quickPress);

	bool canJump = ((m_pPlayer->GetStance() == STANCE_SWIM) ||
		m_pPlayer->TrySetStance(STANCE_STAND)) && 
		(m_pPlayer->m_stats.onGround > 0.01f);

	if (!CanMove() || !canJump || (m_pPlayer->GetSpectatorMode() != CActor::eASM_None))
	{
		CryLog("			> either can't move or can't jump or is spectating, aborting.");
		return false;
	}

	if(m_actions & ACTION_CROUCH)
	{
		CryLog("			> is crouching, un-crouching rather than jumping.");
		CCCPOINT(PlayerMovement_PressJumpWhileCrouchedToStandUp);
		m_actions &= ~ACTION_CROUCH;
		return false;
	}

	m_actions |= ACTION_JUMP;
	if (quickPress)
		m_actionFlags |= CPlayer::eAF_JUMP_QUICK;
	else
		m_actionFlags &= ~CPlayer::eAF_JUMP_QUICK;
	m_speedLean = 0.0f;

	if (CPlayerMovementController *pPMC = static_cast<CPlayerMovementController *>(m_pPlayer->GetMovementController()))
	{
		bool bigWeaponEquipped = m_pPlayer->HasHeavyWeaponEquipped();
		if(!bigWeaponEquipped)
		{
			const SNanoSuitGameParameters& suitParams = m_pPlayer->GetActorSuitGameParameters();
			//m_requestSuperJump = (suitParams.GetMode() == eNanoSuitMode_Power) && (suitParams.IsSuitPowerActive());
			m_requestSuperJump = (suitParams.GetProps().superJumpScale > 1.0f) && (!quickPress);
			pPMC->StrengthJump(m_requestSuperJump);
		}
	}

	return true;
}

bool CPlayerInput::OnActionMap(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		if(g_pGameCVars->hud_mapEnable)
		{
			SHUDEvent event;
			event.eventType = eHUDEvent_ToggleMap;
			CHUD::CallEvent(event);

			return false;
		}
	}

	return true;
}

bool CPlayerInput::OnActionPerks(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	const CGameActions& actions = g_pGame->Actions();

	//Map deployment instead of pointer deployment
#if 1
	if(actionId == actions.binoculars && activationMode == eAAM_OnPress)
	{
		if(m_pPlayer->IsPerkActive(ePerk_EMPStrike) || m_pPlayer->IsPerkActive(ePerk_SatelliteStrike))
		{
			SHUDEvent event;
			event.eventType = eHUDEvent_ToggleMap;
			CHUD::CallEvent(event);
		}
		else if(m_pPlayer->IsPerkActive(ePerk_TeamSuitBoost) || m_pPlayer->IsPerkActive(ePerk_TeamSuperRadar))
		{
			m_pPlayer->SendPerkEvent(EPE_ActivateTeamPerk);
		}
	}
#else
	if(actionId == actions.binoculars)
		SOnActionData onAction = SOnActionData(activationMode);
		m_pPlayer->SendPerkEvent(EPE_Pointer, &onAction);
		return onAction.handled;
	}


	if(actionId == actions.deployStrike)
	{
		SOnActionData onAction = SOnActionData(activationMode);
		m_pPlayer->SendPerkEvent(EPE_OrbitalStrike, &onAction);
		return onAction.handled;
	}
#endif
	if(actionId == actions.special)
	{
		SOnActionData onAction = SOnActionData(activationMode);
		m_pPlayer->SendPerkEvent(EPE_StampMelee, &onAction);
		return onAction.handled;
	}

	return false;
}

bool CPlayerInput::OnActionCrouch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
#if ENABLE_GAME_CODE_COVERAGE
	uint32 oldCrouchAction = (m_actions & ACTION_CROUCH);
#endif

	if (CanMove() && m_pPlayer->GetSpectatorMode() != CActor::eASM_Free && m_pPlayer->GetSpectatorMode() != CActor::eASM_Fixed)
	{
		if (g_pGameCVars->cl_crouchToggle)
		{
			if (value > 0.0f)
			{
				if (!(m_actions & ACTION_CROUCH))
					m_actions |= ACTION_CROUCH;
				else
					m_actions &= ~ACTION_CROUCH;
			}
		}
		else
		{
			if (value > 0.0f)
			{
				m_actions |= ACTION_CROUCH;
			}
			else
			{
				m_actions &= ~ACTION_CROUCH;
			}
		}
	}

	SetSliding((m_actions&ACTION_CROUCH) ? true : false );

#if ENABLE_GAME_CODE_COVERAGE
	if (oldCrouchAction == (m_actions & ACTION_CROUCH))
	{
		CCCPOINT(PlayerMovement_IgnoreCrouchInput);
	}
	else if (m_actions & ACTION_CROUCH)
	{
		CCCPOINT(PlayerMovement_ToggleCrouchActionOn);
	}
	else
	{
		CCCPOINT(PlayerMovement_ToggleCrouchActionOff);
	}
#endif

	return false;
}

//------------------------------------------------------------
void CPlayerInput::SetSliding(bool set)
{
	SPlayerStats* pPlayerStats = static_cast<SPlayerStats*>(m_pPlayer->GetActorStats());
	assert(pPlayerStats);


	if(set)
	{
		//Before setting we have to check if the player has the suit button pressed and enough speed
		if(m_pPlayer->CanSlide())
		{
			m_pPlayer->SetSlideState(SPlayerStats::eSS_Sliding);
		}

	}
	else if (m_pPlayer->IsSliding())
	{
		m_pPlayer->SetSlideState(SPlayerStats::eSS_ExitingSlide);
	}
}

bool CPlayerInput::OnActionSprint(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove() == false)
		return false;
	
	if (activationMode == eAAM_OnPress)
	{
		m_actions |= ACTION_SPRINT;
		
		CCCPOINT_IF(m_actions & ACTION_CROUCH, PlayerMovement_SprintActionDisablesCrouchAction);
		m_actions &= ~ACTION_CROUCH;

		if(gEnv->bMultiplayer)
		{
			CWeapon* pWeapon = m_pPlayer->GetWeapon(m_pPlayer->GetCurrentItemId());

			if(pWeapon)
			{
				if(pWeapon->IsReloading())
				{
					pWeapon->RequestCancelReload();
					pWeapon->AddPendingReload();
				}
			}
		}
	}
	else
	{
		m_actions &= ~ACTION_SPRINT;
		m_speedLean = 0.0f;
		m_pPlayer->SetSpeedLean(0.0f);

	}

	return false;
}

bool CPlayerInput::OnActionLeanLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!m_pPlayer->GetSpectatorMode())
	{
		m_actions |= ACTION_LEANLEFT;
		//not sure about this, its for zeroG
		m_deltaRotation.y -= 30.0f;
	}
	return false;
}

bool CPlayerInput::OnActionLeanRight(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!m_pPlayer->GetSpectatorMode())
	{
		m_actions |= ACTION_LEANRIGHT;
		//not sure about this, its for zeroG
		m_deltaRotation.y += 30.0f;
	}
	return false;
}


bool CPlayerInput::OnActionAttackRightTrigger(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (m_pPlayer->GetHealth() > 0.0f)
	{
		if (activationMode & (eAAM_OnPress|eAAM_OnHold))
		{
			m_actions |= ACTION_FIRE;
		}
		else
		{
			m_actions &= ~ACTION_FIRE;
		}
	
		return false;
	}
	else
	{
		return true;
	}
}


bool CPlayerInput::OnActionUse(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	bool filterOut = true;
	IVehicle* pVehicle = m_pPlayer->GetLinkedVehicle();

	//FIXME:on vehicles use cannot be used
	if (pVehicle)
	{
		filterOut = false;
	}
	
	if (activationMode==eAAM_OnPress)
	{
		//--------------------------FINAL COUNTDOWN PERK-----------------------------------------------		
		if(m_pPlayer->IsPerkActive(ePerk_SelfDestruct))
		{
			m_pPlayer->SendPerkEvent(EPE_SelfDestructExplode);
		}

		if(m_pPlayer->GetPerkData<bool>(EPD_SelfDestructMinigame))
		{
			return false;	//don't set us action when in self destruct
		}

		m_actions |= ACTION_USE;
	}
	else
	{
		m_actions &= ~ACTION_USE;
	}
	
	return filterOut;
}

bool CPlayerInput::OnActionSuitMenuOpen(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	m_suitMenuOpenButtonPressedTime = gEnv->pTimer->GetAsyncCurTime();
	CNanoSuit* pNanoSuit = m_pPlayer->GetNanoSuit();
	if(pNanoSuit)
		pNanoSuit->EnterSlowMotion();

	return false;
}

void CPlayerInput::OpenSuitMenu()
{
	if(gEnv->bMultiplayer && g_pGameCVars->g_mpCoreGameplayTesting)	//Core gameplay testing
		return;

	if (m_pPlayer->GetSpectatorMode())
		return;

	if (!m_pPlayer->GetNanoSuit())
		return;

	const SPlayerStats* pPlayerStats = static_cast<SPlayerStats*>(m_pPlayer->GetActorStats());
	bool isAnimationControlled = pPlayerStats ? pPlayerStats->animationControlled : false;
	if (isAnimationControlled)
		return;

	if (!g_pGame->GetGameRules()->UseNanoSuit(m_pPlayer->GetEntityId()))
		return;

	if (g_pGame->GetGameRules()->IsDead(m_pPlayer->GetEntityId()))
		return;

	ClearSuitSelectionInputs();
	const CGameActions& actions = g_pGame->Actions();
	IActionFilter* suitMenuFilter = actions.FilterSuitMenu();
	suitMenuFilter->Enable(true);
	
	if(!m_suitMenuOpen)
	{
		SHUDEvent event;
		event.eventType = eHUDEvent_OnSuitMenuOpened;
		CHUD::CallEvent(event);
		m_suitMenuOpen = true;
	}
}

ENanoSuitMode CPlayerInput::GetSuitModeFromAxes()
{	
	const float suitSelectStickMovement = 0.7f;

	ENanoSuitMode currentMode = eNanoSuitMode_Invalid;

	if(m_filteredSuitSelectInputY > suitSelectStickMovement && fabsf(m_filteredSuitSelectInputX) < m_filteredSuitSelectInputY)
	{
		currentMode = eNanoSuitMode_Tactical;		
	}
	else if(m_filteredSuitSelectInputY < -suitSelectStickMovement && -fabsf(m_filteredSuitSelectInputX) > m_filteredSuitSelectInputY)
	{
		currentMode = eNanoSuitMode_Power;		
	}
	else if(m_filteredSuitSelectInputX > suitSelectStickMovement)
	{
		currentMode = eNanoSuitMode_Armor;
	}
	else if(m_filteredSuitSelectInputX < -suitSelectStickMovement)
	{
		currentMode = eNanoSuitMode_Stealth;		
	}

	return currentMode;
}

void CPlayerInput::ChooseSelectedSuitMode()
{
	CNanoSuit* pNanoSuit = m_pPlayer->GetNanoSuit();

	if(pNanoSuit)
	{
		ENanoSuitMode currentMode = GetSuitModeFromAxes();
		pNanoSuit->ActivateMode(currentMode);
		ClearSuitSelectionInputs();
	}
}


bool CPlayerInput::OnActionSuitMenuClose(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(m_suitMenuOpen)
	{
		const CGameActions& actions = g_pGame->Actions();
		IActionFilter* suitMenuFilter = actions.FilterSuitMenu();
		suitMenuFilter->Enable(false);
		m_suitMenuOpen = false;

		ChooseSelectedSuitMode();
		SHUDEvent event;
		event.eventType = eHUDEvent_OnSuitMenuClosed;
		CHUD::CallEvent(event);
		m_pPlayer->GetNanoSuit()->ExitSlowMotion();
	}
	else if (m_suitMenuOpenButtonPressedTime > 0.0f)
	{
		m_pPlayer->GetNanoSuit()->ActivateLastMode();
	}

	m_suitMenuOpenButtonPressedTime = 0.0f;

	return false;
}

bool CPlayerInput::OnActionSuitMenuCloseNoSelect(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (m_suitMenuOpen)
	{
		const CGameActions& actions = g_pGame->Actions();
		IActionFilter* suitMenuFilter = actions.FilterSuitMenu();
		suitMenuFilter->Enable(false);
		m_suitMenuOpen = false;

		SHUDEvent event;
		event.eventType = eHUDEvent_OnSuitMenuClosed;
		CHUD::CallEvent(event);
		m_pPlayer->GetNanoSuit()->ExitSlowMotion();
	}
	else if (m_suitMenuOpenButtonPressedTime > 0.0f)
	{
		m_pPlayer->GetNanoSuit()->ActivateLastMode();
	}

	m_suitMenuOpenButtonPressedTime = 0.0f;

	return false;
}


bool CPlayerInput::OnActionSuitSelectXAxis(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	m_suitSelectInputX = value;
	return true;
}


bool CPlayerInput::OnActionSuitSelectYAxis(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	m_suitSelectInputY = value;
	return true;
}


bool CPlayerInput::OnActionSwitchSuitMode(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	CNanoSuit* pNanoSuit = m_pPlayer->GetNanoSuit();
	if (!m_suitMenuOpen || !pNanoSuit || (m_pPlayer->GetHealth() <= 0))
		return false;

	const CGameActions& actions = g_pGame->Actions();

	ENanoSuitMode new_suit_mode = eNanoSuitMode_Invalid;
	if (actionId == actions.suitmode_menu_combat)
		new_suit_mode = eNanoSuitMode_Armor;
	else if (actionId == actions.suitmode_menu_manouver)
		new_suit_mode = eNanoSuitMode_Power;
	else if (actionId == actions.suitmode_menu_infiltrate)
		new_suit_mode = eNanoSuitMode_Stealth;
	else if (actionId == actions.suitmode_menu_energy)
		new_suit_mode = eNanoSuitMode_Tactical;

	if(new_suit_mode != eNanoSuitMode_Invalid)
	{
		pNanoSuit->ActivateMode(new_suit_mode);
		ClearSuitSelectionInputs();
	}

	return false;
}

bool CPlayerInput::OnActionThirdPerson(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!gEnv->pSystem->IsDevMode())
		return false;

	if (!m_pPlayer->GetSpectatorMode() && m_pPlayer->m_pGameFramework->CanCheat())
	{
		if (!m_pPlayer->GetLinkedVehicle())
			m_pPlayer->ToggleThirdPerson();
	}
	return false;
}

bool CPlayerInput::OnActionFlyMode(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
 	if (!gEnv->pSystem->IsDevMode())
		return false;

	if (!m_pPlayer->GetSpectatorMode() && m_pPlayer->m_pGameFramework->CanCheat())
	{
		uint8 flyMode=m_pPlayer->GetFlyMode()+1;
		if (flyMode>2)
			flyMode=0;
		m_pPlayer->SetFlyMode(flyMode);

		switch(m_pPlayer->m_stats.flyMode)
		{
		case 0:m_pPlayer->CreateScriptEvent("printhud",0,"FlyMode/NoClip OFF");break;
		case 1:m_pPlayer->CreateScriptEvent("printhud",0,"FlyMode ON");break;
		case 2:m_pPlayer->CreateScriptEvent("printhud",0,"NoClip ON");break;
		}
	}
	return false;
}

bool CPlayerInput::OnActionGodMode(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!gEnv->pSystem->IsDevMode())
		return false;

	if (!m_pPlayer->GetSpectatorMode() && m_pPlayer->m_pGameFramework->CanCheat())
	{
		CGodMode& godMode = CGodMode::GetInstance();

		godMode.MoveToNextState();
		godMode.RespawnIfDead(m_pPlayer);
	}

	return false;
}

bool CPlayerInput::OnActionAIDebugDraw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!gEnv->pSystem->IsDevMode())
		return false;

	if(ICVar* pCVar = gEnv->pConsole->GetCVar("ai_DebugDraw"))
	{
		pCVar->Set(pCVar->GetIVal()==0 ? 1 : 0);
		return true;
	}
	return false;
}

bool CPlayerInput::OnActionPDrawHelpers(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!gEnv->pSystem->IsDevMode())
		return false;

	if(ICVar* pCVar = gEnv->pConsole->GetCVar("p_draw_helpers"))
	{
		pCVar->Set(pCVar->GetIVal()==0 ? 1 : 0);
		return true;
	}
	return false;
}

bool CPlayerInput::OnActionDMode(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (!gEnv->pSystem->IsDevMode())
		return false;

	if(ICVar* pCVar = gEnv->pConsole->GetCVar("hud_DMode"))
	{
		pCVar->Set(pCVar->GetIVal()==0 ? 1 : 0);
		return true;
	}
	return false;
}

bool CPlayerInput::OnActionXIRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaRotationRaw.z = value;

	//NOTE: Controller mapping no longer carried out here. Speak to Richard Semmens

#if defined(PS3)
	if(fabs(m_xi_deltaRotation.z) < 0.02f)
#else
	if(fabs(m_xi_deltaRotation.z) < 0.003f)
#endif	
		m_xi_deltaRotation.z = 0.f;//some dead point
		
	return false;
}

bool CPlayerInput::OnActionXIRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaRotationRaw.x = value;

	//NOTE: Controller mapping no longer carried out here. Speak to Richard Semmens

#if defined(PS3)
	if(fabs(m_xi_deltaRotation.x) < 0.02f)
#else
	if(fabs(m_xi_deltaRotation.x) < 0.003f)
#endif
		m_xi_deltaRotation.x = 0.f;//some dead point

	return false;
}

bool CPlayerInput::OnActionXIMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove())
	{
		m_xi_deltaMovement.x = value;
		if(fabsf(value)>0.001f && !m_bUseXIInput)
		{
			m_bUseXIInput = true;
		}
		else if(fabsf(value)<=0.001f && m_bUseXIInput && fabsf(m_xi_deltaMovement.y)<=0.001f)
		{
			m_bUseXIInput = false;
			if (!GetMoveButtonsState())
				m_actions &= ~ACTION_MOVE;

			m_deltaMovement.zero();
		}
	}
	return false;
}

bool CPlayerInput::OnActionXIMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (CanMove())
	{
		m_xi_deltaMovement.y = value;
		if(fabsf(value)>0.001f && !m_bUseXIInput)
		{
			m_bUseXIInput = true;
		}
		else if(fabsf(value)<=0.001f && m_bUseXIInput && fabsf(m_xi_deltaMovement.x)<=0.001f)
		{
			m_bUseXIInput = false;
			if (!GetMoveButtonsState())
				m_actions &= ~ACTION_MOVE;

			m_deltaMovement.zero();
		}
	}

	return false;
}


void CPlayerInput::ClearXIMovement()
{
	m_xi_deltaRotationRaw.Set(0.0f, 0.0f, 0.0f);
	m_xi_deltaRotation.Set(0.f,0.f,0.f);
	m_xi_deltaMovement.zero();
}

bool CPlayerInput::OnActionXIDisconnect(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_xi_deltaRotation.Set(0,0,0);
	m_xi_deltaMovement.zero();
	m_bUseXIInput = false;
	if (!GetMoveButtonsState())
		m_actions &= ~ACTION_MOVE;
	m_deltaMovement.zero();

	return false;
}

bool CPlayerInput::OnActionInvertMouse(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	g_pGameCVars->cl_invertMouse = !g_pGameCVars->cl_invertMouse;

	return false;
}

bool CPlayerInput::OnActionFlyCamMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_flyCamDeltaMovement.x = value;

#if defined(PS3)
	if(fabs(m_flyCamDeltaMovement.x) < 0.02f)
#else
	if(fabs(m_flyCamDeltaMovement.x) < 0.003f)
#endif	
		m_flyCamDeltaMovement.x = 0.f;//some dead point
		
	return false;
}

bool CPlayerInput::OnActionFlyCamMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_flyCamDeltaMovement.y = value;

#if defined(PS3)
	if(fabs(m_flyCamDeltaMovement.y) < 0.02f)
#else
	if(fabs(m_flyCamDeltaMovement.y) < 0.003f)
#endif	
		m_flyCamDeltaMovement.y = 0.f;//some dead point
		
	return false;
}

bool CPlayerInput::OnActionFlyCamMoveUp(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_flyCamDeltaMovement.z = value;

#if defined(PS3)
	if(fabs(m_flyCamDeltaMovement.z) < 0.02f)
#else
	if(fabs(m_flyCamDeltaMovement.z) < 0.003f)
#endif	
		m_flyCamDeltaMovement.z = 0.f;//some dead point
		
	return false;
}

bool CPlayerInput::OnActionFlyCamMoveDown(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_flyCamDeltaMovement.z = -value;

#if defined(PS3)
	if(fabs(m_flyCamDeltaMovement.z) < 0.02f)
#else
	if(fabs(m_flyCamDeltaMovement.z) < 0.003f)
#endif	
		m_flyCamDeltaMovement.z = 0.f;//some dead point
		
	return false;
}

bool CPlayerInput::OnActionFlyCamSpeedUp(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		g_pGameCVars->g_detachedCameraMoveSpeed = CLAMP(g_pGameCVars->g_detachedCameraMoveSpeed + 0.5f, 0.5f, 30.f);
	}

	return false;
}

bool CPlayerInput::OnActionFlyCamSpeedDown(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		g_pGameCVars->g_detachedCameraMoveSpeed = CLAMP(g_pGameCVars->g_detachedCameraMoveSpeed - 0.5f, 0.5f, 30.f);
	}

	return false;
}

bool CPlayerInput::OnActionFlyCamTurbo(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		m_flyCamTurbo=true;
	}
	else if (activationMode == eAAM_OnRelease)
	{
		m_flyCamTurbo=false;
	}
	
	return false;
}

bool CPlayerInput::OnActionFlyCamRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_flyCamDeltaRotation.z = value;

#if defined(PS3)
	if(fabs(m_flyCamDeltaRotation.z) < 0.02f)
#else
	if(fabs(m_flyCamDeltaRotation.z) < 0.003f)
#endif	
		m_flyCamDeltaRotation.z = 0.f;//some dead point
		
	return false;
}

bool CPlayerInput::OnActionFlyCamRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	m_flyCamDeltaRotation.x = value;

#if defined(PS3)
	if(fabs(m_flyCamDeltaRotation.x) < 0.02f)
#else
	if(fabs(m_flyCamDeltaRotation.x) < 0.003f)
#endif
		m_flyCamDeltaRotation.x = 0.f;//some dead point

	return false;
}

bool CPlayerInput::OnActionFlyCamSetPoint(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
#if FREE_CAM_SPLINE_ENABLED
	if(activationMode == eAAM_OnPress)
	{
		AddFlyCamPoint();
	}
#endif

	return false;
}

void CPlayerInput::AddFlyCamPoint()
{
	IView*  pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
	assert(pView);
	const SViewParams*  pViewParams = pView->GetCurrentParams();

	Vec3 curPos = pViewParams->position;
	Vec3 lookAtPos = pViewParams->position;

	Matrix34 mtx(pViewParams->rotation);
	Vec3 forward(0.f,3.f,0.f);
	forward = mtx * forward;
	lookAtPos += forward;

	AddFlyCamPoint(curPos, lookAtPos);
}

void CPlayerInput::AddFlyCamPoint(Vec3 pos, Vec3 lookAtPos)
{
#if FREE_CAM_SPLINE_ENABLED
	if (m_freeCamCurrentIndex < MAX_FREE_CAM_DATA_POINTS)
	{
		SFreeCamPointData &camData = m_freeCamData[m_freeCamCurrentIndex];
		camData.valid = true;
		camData.position = pos;
		camData.lookAtPosition = lookAtPos;

		CryLog("FREECAM Added new FreeCamPointData - index:%d pos:%f %f %f lookAt:%f %f %f", m_freeCamCurrentIndex, pos.x, pos.y, pos.z, lookAtPos.x, lookAtPos.y, lookAtPos.z);

		++m_freeCamCurrentIndex;
	}
	else
	{
		CryLog("FREECAM Have reached the maximum (%d) num of FreeCamPointData, no more can be added!", MAX_FREE_CAM_DATA_POINTS);
	}
#endif
}

void CPlayerInput::FlyCamPlay()
{
#if FREE_CAM_SPLINE_ENABLED
	m_freeCamPlaying = !m_freeCamPlaying;

	if (m_freeCamPlaying)
	{
		m_freeCamPlayTimer = 0.f;

		int num=0;

		SFreeCamPointData *camData = NULL;
		SFreeCamPointData *lastCamData = NULL;
		float totalDistance = 0.f;
		Vec3 diff;

		for (num=0; num < MAX_FREE_CAM_DATA_POINTS; ++num)
		{
			camData = &m_freeCamData[num];
			if (camData->valid)
			{
				if (lastCamData)
				{
					diff = camData->position - lastCamData->position;
					camData->distanceFromLast = diff.len();
					totalDistance += camData->distanceFromLast;
				}
			}
			else
			{
				break;
			}

			lastCamData = camData;
		}

		m_freeCamSpline.resize(num);
		m_freeCamLookAtSpline.resize(num);

		float curDistance = 0.f;

		for (int i=0; i<num; ++i)
		{
			camData = &m_freeCamData[i];

			curDistance += camData->distanceFromLast;
			float time = curDistance / totalDistance;

			m_freeCamSpline.key(i).flags = 0;
			m_freeCamSpline.time(i) = time;
			m_freeCamSpline.value(i) = camData->position;

			m_freeCamLookAtSpline.key(i).flags = 0;
			m_freeCamLookAtSpline.time(i) = time;
			m_freeCamLookAtSpline.value(i) = camData->lookAtPosition;
		}

		m_freeCamTotalPlayTime = totalDistance / g_pGameCVars->g_detachedCameraMoveSpeed;

		if (num > 0)
			CryLog("FREECAM Starting to play free cam spline numpoints:%d distance:%f totaltime:%f", num, totalDistance, m_freeCamTotalPlayTime);
		else
			CryLog("FREECAM Cannot start playing free cam spline - no points set");
	}
	else
	{
		CryLog("FREECAM Stop playing free cam spline");
	}
#endif
}

bool CPlayerInput::OnActionFlyCamPlay(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		FlyCamPlay();
	}

	return false;
}

bool CPlayerInput::OnActionFlyCamClear(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
#if FREE_CAM_SPLINE_ENABLED
	if(activationMode == eAAM_OnPress)
	{
		for (int i=0; i<MAX_FREE_CAM_DATA_POINTS; ++i)
		{
			SFreeCamPointData &camData = m_freeCamData[i];
			camData.valid = false;	
		}

		m_freeCamCurrentIndex = 0;

		m_freeCamPlayTimer = 0.f;
		m_freeCamTotalPlayTime = 0.f;

		m_freeCamSpline.empty();
		m_freeCamLookAtSpline.empty();
		
		CryLog("FREECAM Cleared data");
	}
#endif

	return false;
}

bool CPlayerInput::OnActionStealthKill(EntityId entityId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		const CPlayer::SInteractionInfo& interactionInfo = m_pPlayer->GetCurrentInteractionInfo();
		if (interactionInfo.interactionType == CPlayer::eInteraction_Stealthkill)
		{
			m_pPlayer->EnterStealthKill(interactionInfo.interactiveEntityId);
		}
	}

	return false;
}

void CPlayerInput::AdjustMoveButtonState(EMoveButtonMask buttonMask, int activationMode )
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

bool CPlayerInput::CheckMoveButtonStateChanged(EMoveButtonMask buttonMask, int activationMode)
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

float CPlayerInput::MapControllerValue(float value, float scale, float curve, bool inverse)
{
	// Any attempts to create an advanced analog stick value mapping function could be put here

	// After several experiments a simple pow(x, n) function seemed to work best.
	float res=scale * powf(fabs(value), curve);
	return (value >= 0.0f ? (inverse ? -1.0f : 1.0f) : (inverse ? 1.0f : -1.0f))*res;
}

void CPlayerInput::ClearAllExceptAction( uint32 actionFlags )
{
	uint32 actionsToKeep = m_actions & actionFlags;
	Reset();
	m_actions |= actionsToKeep;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CAIInput::CAIInput( CPlayer * pPlayer ) : 
m_pPlayer(pPlayer), 
m_pStats(&pPlayer->m_stats)
{
}

CAIInput::~CAIInput()
{
}

void CAIInput::GetState( SSerializedPlayerInput& input )
{
	SMovementState movementState;
	m_pPlayer->GetMovementController()->GetMovementState( movementState );

	Quat worldRot = m_pPlayer->GetBaseQuat();
	input.stance = movementState.stance;
	input.bodystate = 0;

	IAIActor* pAIActor = CastToIAIActorSafe(m_pPlayer->GetEntity()->GetAI());
	if (pAIActor)
	{
		input.bodystate=pAIActor->GetState()->bodystate;
		input.allowStrafing = pAIActor->GetState()->allowStrafing;
	}

	input.deltaMovement = movementState.movementDirection.GetNormalizedSafe()*movementState.desiredSpeed;
	input.lookDirection = movementState.eyeDirection;
	input.bodyDirection = movementState.bodyDirection;
	input.sprint = false;
	input.leanl = false;
	input.leanr = false;
	

	IAnimationGraphState *pState=0;
	if (m_pPlayer->GetAnimatedCharacter())
		pState=m_pPlayer->GetAnimatedCharacter()->GetAnimationGraphState();

	if (pState)
	{
		input.aiming = pState->GetInputAsFloat(m_pPlayer->m_inputAiming)!=0.0f;
		input.usinglookik = pState->GetInputAsFloat(m_pPlayer->m_inputUsingLookIK)!=0.0f;
		input.pseudoSpeed=pState->GetInputAsFloat(pState->GetInputId("PseudoSpeed"));
	}
}

void CAIInput::SetState( const SSerializedPlayerInput& input )
{
	GameWarning("CAIInput::SetState called: should never happen");
}
