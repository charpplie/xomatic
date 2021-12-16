/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$   All input weapon stuff here

-------------------------------------------------------------------------
History:
- 30.07.07   12:50 : Created by Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "GameActions.h"
#include "Game.h"
#include "GameCVars.h"

#include "IPlayerInput.h"
#include "Player.h"


 
//===========AUX FUNCTIONS====================
namespace
{
	bool CheckPickupInteraction(CWeapon* pThis)
	{
		CActor* pActor = pThis->GetOwnerActor();
		if(!pActor || !pActor->IsClient())
			return false;

		CPlayer* pClientPlayer = static_cast<CPlayer*>(pActor);
		CPlayer::EInteractionType interactionType = pClientPlayer->GetCurrentInteractionInfo().interactionType;
		
		bool itemInteraction =	(interactionType == CPlayer::eInteraction_ExchangeItem) ||
														(interactionType == CPlayer::eInteraction_PickupAmmo) ||
														(interactionType == CPlayer::eInteraction_PickupItem);

		return itemInteraction;
	}
	void GetDualWieldInfo(CWeapon* pThis, bool &isDualWield, CWeapon** pSlave)
	{
		if (pThis->IsDualWieldMaster())
		{
			IItem *slave = pThis->GetDualWieldSlave();
			if (slave && slave->GetIWeapon())
			{
				(*pSlave) = static_cast<CWeapon *>(slave);
				isDualWield = true;
			}
		}
	}
}

//=================================================================
TActionHandler<CWeapon>	CWeapon::s_actionHandler;

void CWeapon::RegisterActions()
{
	if (s_actionHandler.GetNumHandlers() == 0)
	{
		#define ADD_HANDLER(action, func) s_actionHandler.AddHandler(actions.action, &CWeapon::func)
		const CGameActions& actions = g_pGame->Actions();

		ADD_HANDLER(attack1, OnActionAttackPrimary);
		ADD_HANDLER(attack1_xi, OnActionAttackPrimary);
		ADD_HANDLER(attack2_xi,OnActionAttackSecondary);
		ADD_HANDLER(reload,OnActionReload);
		ADD_HANDLER(special,OnActionSpecial);
		ADD_HANDLER(modify,OnActionModify);
		ADD_HANDLER(weapon_change_firemode,OnActionFiremode);
		ADD_HANDLER(zoom_in,OnActionZoomIn);
		ADD_HANDLER(zoom_out,OnActionZoomOut);
		ADD_HANDLER(zoom,OnActionZoom);
		ADD_HANDLER(xi_zoom,OnActionZoom);
		ADD_HANDLER(sprint,OnActionSprint);
		//ADD_HANDLER(xi_zoom,OnActionZoomXI);

		#undef ADD_HANDLER
	}
}
//-----------------------------------------------------
void CWeapon::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	CItem::OnAction(actorId, actionId, activationMode, value);

	s_actionHandler.Dispatch(this,actorId,actionId,activationMode,value);
}

//------------------------------------------------------
void CWeapon::ForcePendingActions(uint8 blockedActions)
{
	CItem::ForcePendingActions(blockedActions);

	CActor* pOwner = GetOwnerActor();
	if(!pOwner || !pOwner->IsClient())
		return;

	//Force start firing, if needed and possible
	if(IsInputFlagSet(eWeaponAction_Fire) && !(blockedActions&eWeaponAction_Fire))
	{
		if(!IsDualWield() && !IsWeaponRaised())
		{
			ClearInputFlag(eWeaponAction_Fire);
			if(IsTargetOn() || (m_fm && !m_fm->AllowZoom()))
				return;
			
			OnAction(GetOwnerId(),"attack1",eAAM_OnPress,0.0f);
		}
		else if(IsDualWield() && IsDualWieldMaster())
		{
			IItem *slave = GetDualWieldSlave();
			if(!IsWeaponRaised())
			{
				ClearInputFlag(eWeaponAction_Fire);
				OnAction(GetOwnerId(),"attack1",eAAM_OnPress,0.0f);
			}
			else if(slave && slave->GetIWeapon())
			{
				CWeapon* dualwield = static_cast<CWeapon*>(slave);
				if(!dualwield->IsWeaponRaised())
				{
					ClearInputFlag(eWeaponAction_Fire);
					OnAction(GetOwnerId(),"attack1",eAAM_OnPress,0.0f);
				}
			}
		}
	}

	//Force zoom in if needed
	if(IsInputFlagSet(eWeaponAction_Zoom) && !(blockedActions&eWeaponAction_Zoom))
	{
		if(!IsZoomed() && !IsZoomingInOrOut())
			OnActionZoom(GetOwnerId(), "zoom", eAAM_OnPress, 0.0f);

		ClearInputFlag(eWeaponAction_Zoom);
	}

	if(IsInputFlagSet(eWeaponAction_Reload) && !(blockedActions&eWeaponAction_Reload))
	{
		Reload();
		ClearInputFlag(eWeaponAction_Reload);
	}
}

//--------------------------------------------------------------------
bool CWeapon::PreActionAttack(bool startFire)
{
	if(startFire)
	{
		SetInputFlag(eWeaponAction_Fire);
	}
	else if(!startFire)
	{
		ClearInputFlag(eWeaponAction_Fire);
	}

	CPlayer *pPlayer = static_cast<CPlayer*>(GetOwnerActor());
	if(!pPlayer)
		return false;

	if(IsModifying())
		return true;

	SPlayerStats *pStats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
	assert(pStats);

	if (startFire && pStats->bSprinting)
	{
		pStats->bIgnoreSprinting = true;
		m_delayedFireActionTimeOut = GetParams().sprintToFireDelay;
		return true;
	}

	return false;
}

bool CWeapon::OnActionSprint(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	//If zoomed, you can not sprint (in SP only)
	if ((gEnv->bMultiplayer == false) && IsZoomed())
		return false;

	CActor* pOwnerActor = GetOwnerActor();
	CPlayer *pPlayer = 0;
	SPlayerStats *pStats = 0;
	if (pOwnerActor && pOwnerActor->IsPlayer())
	{
		pPlayer = static_cast<CPlayer*>(pOwnerActor);
		pStats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
	}

	if (activationMode == eAAM_OnPress)
	{
		if (pPlayer)
		{
			bool canMove = !pPlayer->IsStandingUp();

			if (pStats && pStats->bIgnoreSprinting && !m_isFiring)
				pStats->bIgnoreSprinting = false;

			if (canMove)
			{
				StartSprint(pOwnerActor);
				return true;
			}
		}
	}

	return false;
}

//--------------------------------------------------------------------
void CWeapon::StartSprint(CActor* pOwnerActor)
{
	StopFire();
	ExitZoom();
}

//---------------------------------------------------------
//Controller input (primary fire)
bool CWeapon::OnActionAttackPrimary(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		if(PreActionAttack(true))
			return true;

		StartFire();
	}
	else if(activationMode == eAAM_OnHold)
	{
		if(m_delayedFireActionTimeOut<=0.0f && m_fm && m_fm->CanFire() && !m_fm->IsFiring())
		{
			StartFire();
		}		
	}
	else
	{
		PreActionAttack(false);
		StopFire();
	}

	return true;
}
//---------------------------------------------------------
//Controller input (secondary fire)
bool CWeapon::OnActionAttackSecondary(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{

	//If not, must be dual wield
	bool isDualWield = false;
	CWeapon *dualWield = NULL;
	GetDualWieldInfo(this,isDualWield,&dualWield);

	if(dualWield)
	{
		if (activationMode == eAAM_OnPress)
		{
			if(PreActionAttack(true))
				return true;

			dualWield->StartFire();
		}
		else
		{
			PreActionAttack(false);

			dualWield->StopFire();
		}
	}
	else
	{
		//Zoom in/out
		OnActionZoom(actorId, actionId, activationMode, value);

	}

	return true;
}

//---------------------------------------------------------
bool CWeapon::OnActionReload(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	bool playerCanStartReload = false;
	if (activationMode == eAAM_OnPress && (IsOwnerSuperJumpingOrUsingAirFriction() == false) && (CheckPickupInteraction(this) == false))
		playerCanStartReload = true;
	// this condition results from the interaction between reload and item pickups. If on front on an item and press reload/pick button but not for long enought,
	// then the player should still reload the weapon instead of picking up the item.
	else if(activationMode == eAAM_OnRelease && CheckPickupInteraction(this) == true)
		playerCanStartReload = true;

	if (!playerCanStartReload)
		return true;

	if (!IsBusy() && !m_modifying)
	{
		bool isDualWield = false;
		CWeapon *dualWield = NULL;
		GetDualWieldInfo(this,isDualWield,&dualWield);

		if(IsWeaponRaised() && m_fm && m_fm->CanReload())
			RaiseWeapon(false);

		Reload();

		if (isDualWield)
		{
			if(dualWield->IsWeaponRaised() && dualWield->CanReload())
				dualWield->RaiseWeapon(false);
			dualWield->Reload();
		}
	}
	else if (!IsReloading())
	{
		SetInputFlag(eWeaponAction_Reload);
	}

	return true;
}

//---------------------------------------------------------------------------------
bool CWeapon::OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	//if (activationMode==eAAM_OnPress)
	if(activationMode == eAAM_OnRelease)
	{
		bool isDualWield = false;
		CWeapon *dualWield = NULL;
		GetDualWieldInfo(this,isDualWield,&dualWield);

		if (isDualWield)
		{
			if(IsWeaponRaised())
				RaiseWeapon(false,true);

			if(dualWield->IsWeaponRaised())
				dualWield->RaiseWeapon(false,true);

			StartChangeFireMode();
		}
		else
		{
			if(m_weaponRaised)
				RaiseWeapon(false,true);

			StartChangeFireMode();
		}
	}

	return true;
}

//---------------------------------------------------------------------
bool CWeapon::OnActionSpecial(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		if(m_weaponRaised)
			RaiseWeapon(false,true);

		CActor* pOwnerActor = GetOwnerActor();

		if (CanMeleeAttack())
			MeleeAttack();
	}

	return true;
}

//------------------------------------------------------------------------
class CWeapon::ScheduleLayer_Leave
{
public:
	ScheduleLayer_Leave(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) {
		_pWeapon->m_transitioning = false;
		gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0.0f);
		_pWeapon->s_dofSpeed=0.0f;
	}
private:
	CWeapon *_pWeapon;
};

class CWeapon::ScheduleLayer_Enter
{
public:
	ScheduleLayer_Enter(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) {
		SAFE_HUD_FUNC(WeaponAccessoriesInterface(true));
		_pWeapon->PlayLayer(g_pItemStrings->modify_layer, eIPAF_Default|eIPAF_NoBlend, false);
		_pWeapon->m_transitioning = false;

		gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 1.0f);
		_pWeapon->s_dofSpeed=0.0f;
	}
private:
	CWeapon *_pWeapon;
};

//-------------------------------------------------------------------------
bool CWeapon::OnActionModify(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (!CanModify())
		return true;

	if (!IsBusy())
	{
		if (m_fm)
			m_fm->StopFire();

		if(IsZoomed())
			StopZoom(actorId);

		if(m_weaponRaised)
			RaiseWeapon(false,true);

		if (m_modifying)// && !m_transitioning)
		{
			GetScheduler()->Reset();

			StopLayer(g_pItemStrings->modify_layer, eIPAF_Default, false);
			PlayAction(g_pItemStrings->leave_modify, 0);
			float  div = ((float)GetCurrentAnimationTime(eIGS_Owner)/1000.0f);
			s_dofSpeed = (div!=0.f ? -1.0f/div : 0.f);
			s_dofValue = 1.0f;
			s_focusValue = -1.0f;

			GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<ScheduleLayer_Leave>::Create(this), false);
			m_transitioning = true;

			SAFE_HUD_FUNC(WeaponAccessoriesInterface(false));
			m_modifying = false;

			GetGameObject()->InvokeRMI(CItem::SvRequestLeaveModify(), CItem::EmptyParams(), eRMI_ToServer);
		}
		else if (!m_modifying)// && !m_transitioning)
		{
			GetScheduler()->Reset();

			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMin", 0.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMax", 5.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", 20.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 0.0f);

			PlayAction(g_pItemStrings->enter_modify, 0, false, eIPAF_Default | eIPAF_RepeatLastFrame, 1.5f);
			float  div = ((float)GetCurrentAnimationTime(eIGS_Owner)/1000.0f);
			s_dofSpeed = (div!=0.f ? 1.0f/div : 0.f);

			s_dofValue = 0.0f;
			m_transitioning = true;

			GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<ScheduleLayer_Enter>::Create(this), false);
			m_modifying = true;

			GetGameObject()->InvokeRMI(CItem::SvRequestEnterModify(), CItem::EmptyParams(), eRMI_ToServer);
		}
	}

	return true;
}

//---------------------------------------------------------
bool CWeapon::OnActionZoomIn(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(m_zm && m_zm->IsZoomed())
	{
		int numSteps = m_zm->GetMaxZoomSteps();
		if((numSteps>1) && (m_zm->GetCurrentStep()<numSteps))
			StartZoom(actorId,1);	
	}

	return true;
}

//----------------------------------------------------------
bool CWeapon::OnActionZoomOut(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(m_zm && m_zm->IsZoomed())
	{
		int numSteps = m_zm->GetMaxZoomSteps();
		if((numSteps>1) && (m_zm->GetCurrentStep()>1))
			m_zm->ZoomOut();
	}

	return true;
}

//----------------------------------------------------------
bool CWeapon::OnActionZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress || activationMode == eAAM_OnHold)
	{
		SetInputFlag(eWeaponAction_Zoom);
	}
	else
	{
		ClearInputFlag(eWeaponAction_Zoom);
	}
	
	if (!m_modifying && (gEnv->bMultiplayer || !IsOwnerSuperJumpingOrUsingAirFriction()))	//can ironsight while super jumping in MP
	{
		bool isDualWield = false;
		CWeapon *dualWield = NULL;
		GetDualWieldInfo(this,isDualWield,&dualWield);

		if (!isDualWield)
		{
			if (m_fm && !m_fm->IsReloading())
			{
				if (activationMode == eAAM_OnPress)
				{
					if(!m_fm->AllowZoom())
					{
						if(IsTargetOn())
						{
							m_fm->Cancel();
						}
						else
						{
							return false;
						}
					}

					if(m_weaponRaised)
					{
						RaiseWeapon(false,true);
					}
				}
				else if (activationMode == eAAM_OnRelease)
				{
					StopZoom(actorId);
				}

				bool zoomButtonPressed = activationMode == eAAM_OnPress || activationMode == eAAM_OnHold;
				if (m_zm && zoomButtonPressed)
				{
					bool zoomingOut = m_zm->IsZoomingInOrOut() && !m_zm->IsZoomingIn();
					bool zoomingIn = m_zm->IsZoomingIn();
					bool zoomed = m_zm->IsZoomed();
					if ((!zoomed && !zoomingIn) || zoomingOut)
					{
						StartZoom(actorId,1);
					}
				}
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------------
bool CWeapon::OnActionZoomXI(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (!m_modifying && !IsWeaponRaised())
	{
		bool isDualWield = false;
		CWeapon *dualWield = NULL;
		GetDualWieldInfo(this,isDualWield,&dualWield);

		if (g_pGameCVars->hud_ctrlZoomMode)
		{
			if (activationMode == eAAM_OnPress)
			{
				if (!isDualWield)
				{
					if(m_fm && !m_fm->IsReloading())
					{
						// The zoom code includes the aim assistance
						if (m_fm->AllowZoom())
							StartZoom(actorId,1);
						else
							m_fm->Cancel();
					}
				}
			}
			else if (activationMode == eAAM_OnRelease)
			{
				if (!isDualWield)
				{
					if(m_fm && !m_fm->IsReloading())
						StopZoom(actorId);
				}
			}
		}
		else
		{
			if (activationMode == eAAM_OnPress && m_fm && !m_fm->IsReloading())
			{
				if (!isDualWield)
				{
					if (m_fm->AllowZoom())
						StartZoom(actorId,1);		
					else
						m_fm->Cancel();
				}
			}
		}
	}

	return true;
}
