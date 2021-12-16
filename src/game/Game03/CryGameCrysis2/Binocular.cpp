/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:12:2005   14:01 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Binocular.h"
#include "GameActions.h"

#include <IActorSystem.h>
#include <IMovementController.h>

#include "Game.h"
#include "Audio/GameAudio.h"
#include "Player.h"
#include "HUD/HUD.h"
#include "GameCVars.h"



TActionHandler<CBinocular> CBinocular::s_actionHandler;



CBinocular::CBinocular()
: m_ambienceLoopSoundId(INVALID_SOUNDID)
{
	if(s_actionHandler.GetNumHandlers() == 0)
	{

#define ADD_HANDLER(action, func) s_actionHandler.AddHandler(actions.action, &CBinocular::func)
		const CGameActions& actions = g_pGame->Actions();
		ADD_HANDLER(attack2_xi, OnActionChangeZoom);
		ADD_HANDLER(sprint, OnActionSprint);
#undef ADD_HANDLER

	}
}



CBinocular::~CBinocular()
{
}



void CBinocular::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(!s_actionHandler.Dispatch(this,actorId,actionId,activationMode,value))
		CWeapon::OnAction(actorId, actionId, activationMode, value);
}



void CBinocular::Select(bool select)
{
	CWeapon::Select(select);

	if (!GetOwnerActor() || !GetOwnerActor()->IsClient())
		return;

	if (GetOwnerActor()->GetActorSuitGameParameters().GetMode()!=eNanoSuitMode_Tactical)
	{
		GetOwnerActor()->SelectLastItem(false, true);
		return;
	}

	//turn sound-zooming on / off
	gEnv->pSoundSystem->CalcDirectionalAttenuation(GetOwnerActor()->GetEntity()->GetWorldPos(), GetOwnerActor()->GetViewRotation().GetColumn1(), select?0.15f:0.0f);

	if (select)
	{
		SetBusy(false);

		if (m_zm)
			m_zm->StartZoom();

		m_ambienceLoopSoundId = PlayAction(g_pItemStrings->AmbienceSound);
	}
	else
	{
		StopSound(m_ambienceLoopSoundId);
		m_ambienceLoopSoundId = INVALID_SOUNDID;
	}
}



void CBinocular::UpdateFPView(float frameTime)
{
	CWeapon::UpdateFPView(frameTime);

	CActor *pOwner = GetOwnerActor();
	if(pOwner && pOwner->IsClient())
	{
		if(m_zm && IsZoomed())
			gEnv->pSoundSystem->CalcDirectionalAttenuation(pOwner->GetEntity()->GetWorldPos(), pOwner->GetViewRotation().GetColumn1(), 0.35f - m_zm->GetCurrentStep() * 0.05f);
	}
}



bool CBinocular::CanModify() const
{
	return false;
}



bool CBinocular::CanFire() const
{
	return true;
}



void CBinocular::StartFire(const Vec3 *fireTarget)
{
	CWeapon::StartFire(fireTarget);
	if(GetOwnerActor() && GetOwnerActor()->IsClient())
	{
		SHUDEvent startScanningEvent;
		startScanningEvent.eventType = eHUDEvent_OnScanningStart;
		startScanningEvent.eventFloatData = g_pGameCVars->pl_TacticalScanDuration;
		CHUD::CallEvent(startScanningEvent);
	}
}



void CBinocular::StopFire()
{
	CWeapon::StopFire();
	if(GetOwnerActor() && GetOwnerActor()->IsClient())
	{
		SHUDEvent stopScanningEvent;
		stopScanningEvent.eventType = eHUDEvent_OnScanningStop;
		CHUD::CallEvent(stopScanningEvent);
	}
}

bool CBinocular::OnActionChangeZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (m_zm == NULL)
		return true;

	if (activationMode == eAAM_OnPress)
	{
		PlayAction( "zoom_in_binocular" );
		m_zm->StartZoom(false, false);
	}
	else if (activationMode == eAAM_OnRelease)
	{
		PlayAction( "zoom_out_binocular" );
		m_zm->ZoomOut();
	}
	return true;
}

bool CBinocular::OnActionSprint(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (activationMode == eAAM_OnPress)
	{
		// do nothing
		return true;
	}
	
	return false;
}

void CBinocular::OnZoomIn() {}
void CBinocular::OnZoomOut() {}

