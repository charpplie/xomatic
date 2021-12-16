/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a class which handle client actions on vehicles.

-------------------------------------------------------------------------
History:
- 17:10:2006: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "IGame.h"
#include "IActorSystem.h"
#include "IVehicleSystem.h"
#include "VehicleClient.h"
#include "GameCVars.h"
#include "Game.h"
#include "GameActions.h"
#include "Weapon.h"
#include "Player.h"
#include "NanoSuitDefs.h"
#include "VehicleMovementStdWheeled.h"
#include "VehicleMovementBase.h"

#include <ISerialize.h>

//------------------------------------------------------------------------
bool CVehicleClient::Init()
{
  m_actionNameIds.clear();	
 
  m_actionNameIds.insert(TActionNameIdMap::value_type("use", eVAI_Exit));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_exit", eVAI_Exit));
	m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeseat", eVAI_ChangeSeat));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeseat1", eVAI_ChangeSeat1));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeseat2", eVAI_ChangeSeat2));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeseat3", eVAI_ChangeSeat3));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeseat4", eVAI_ChangeSeat4));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeseat5", eVAI_ChangeSeat5));

  m_actionNameIds.insert(TActionNameIdMap::value_type("v_changeview", eVAI_ChangeView));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_viewoption", eVAI_ViewOption));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_zoom_in", eVAI_ZoomIn));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_zoom_out", eVAI_ZoomOut));

  m_actionNameIds.insert(TActionNameIdMap::value_type("attack1", eVAI_Attack1));
  m_actionNameIds.insert(TActionNameIdMap::value_type("zoom", eVAI_Attack2));
	m_actionNameIds.insert(TActionNameIdMap::value_type("v_attack2", eVAI_Attack2));
	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_zoom", eVAI_Attack2));
  m_actionNameIds.insert(TActionNameIdMap::value_type("firemode", eVAI_FireMode));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_lights", eVAI_ToggleLights));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_horn", eVAI_Horn));

	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_attack1", eVAI_Attack1));
	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_attack2", eVAI_Attack2));

  m_actionNameIds.insert(TActionNameIdMap::value_type("v_rotateyaw", eVAI_RotateYaw));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_rotatepitch", eVAI_RotatePitch));

  m_actionNameIds.insert(TActionNameIdMap::value_type("v_moveforward", eVAI_MoveForward));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_moveback", eVAI_MoveBack));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_moveup", eVAI_MoveUp));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_movedown", eVAI_MoveDown));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_turnleft", eVAI_TurnLeft));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_turnright", eVAI_TurnRight));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_strafeleft", eVAI_StrafeLeft));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_straferight", eVAI_StrafeRight));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_rollleft", eVAI_RollLeft));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_rollright", eVAI_RollRight));

	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_rotateyaw", eVAI_XIRotateYaw));
	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_rotatepitch", eVAI_XIRotatePitch));
	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_movey", eVAI_XIMoveY));
	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_movex", eVAI_XIMoveX));

	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_accelerate", eVAI_XIAccelerate));
	m_actionNameIds.insert(TActionNameIdMap::value_type("xi_v_deccelerate", eVAI_XIDeccelerate));

  m_actionNameIds.insert(TActionNameIdMap::value_type("v_pitchup", eVAI_PitchUp));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_pitchdown", eVAI_PitchDown));

  m_actionNameIds.insert(TActionNameIdMap::value_type("v_brake", eVAI_Brake));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_afterburner", eVAI_AfterBurner));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_boost", eVAI_Boost));

  m_actionNameIds.insert(TActionNameIdMap::value_type("v_debug_1", eVAI_Debug_1));
  m_actionNameIds.insert(TActionNameIdMap::value_type("v_debug_2", eVAI_Debug_2));

	m_actionNameIds.insert(TActionNameIdMap::value_type("v_ripoff_weapon", eVAI_RipoffWeapon));
  
#if PJH_MERGE_HACK
	m_actionNameIds.insert(TActionNameIdMap::value_type("cloak", eVAI_Cloak));
	m_actionNameIds.insert(TActionNameIdMap::value_type("uncloak", eVAI_Uncloak));
#endif

	m_xiRotation.Set(0,0,0);
	m_bMovementFlagForward = false;
	m_bMovementFlagBack = false;
	m_bMovementFlagRight = false;
	m_bMovementFlagLeft = false;
  m_fLeftRight = 0.f;
  m_fForwardBackward = 0.f;
	m_tp = true; // We always want to choose third person as the camera when first getting into a vehicle
	
	m_cloaked = false;
	m_playerId = 0;
	m_vehicleId = 0;

  return true;
}

//------------------------------------------------------------------------
void CVehicleClient::Reset()
{
	m_tp = false;
}

//------------------------------------------------------------------------
void CVehicleClient::OnAction(IVehicle* pVehicle, EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	assert(pVehicle);
 	if (!pVehicle)
		return;
	
  TActionNameIdMap::const_iterator ite = m_actionNameIds.find(actionId);
	if (ite == m_actionNameIds.end())
    return;

	switch (ite->second)
  {
  case (eVAI_XIMoveX):	
    {

			IVehicleMovement *pMovement = pVehicle->GetMovement();
			if(pMovement && pMovement->GetMovementType() == IVehicleMovement::eVMT_Air)
			{
				//strafe instead of turning for air vehicles
				if(value>0.f)
				{
					pVehicle->OnAction(eVAI_StrafeRight, eAAM_OnPress, value, actorId);
					m_bMovementFlagRight = true;
				}
				else if(value==0.f)
				{
					if(m_bMovementFlagRight)
					{
						pVehicle->OnAction(eVAI_StrafeRight, eAAM_OnRelease, 0.f, actorId);
						m_bMovementFlagRight = false;
					}
					else if(m_bMovementFlagLeft)
					{
						pVehicle->OnAction(eVAI_StrafeLeft, eAAM_OnRelease, 0.f, actorId);
						m_bMovementFlagLeft = false;
					}
				}
				else//value<0
				{
					pVehicle->OnAction(eVAI_StrafeLeft, eAAM_OnPress, -value, actorId);
					m_bMovementFlagLeft = true;
				}
			}
			else
			{
				if(value>0.f)
				{
					pVehicle->OnAction(eVAI_TurnRight, eAAM_OnPress, value, actorId);
					m_bMovementFlagRight = true;
				}
				else if(value==0.f)
				{
					if(m_bMovementFlagRight)
					{
						pVehicle->OnAction(eVAI_TurnRight, eAAM_OnRelease, 0.f, actorId);
						m_bMovementFlagRight = false;
					}
					else if(m_bMovementFlagLeft)
					{
						pVehicle->OnAction(eVAI_TurnLeft, eAAM_OnRelease, 0.f, actorId);
						m_bMovementFlagLeft = false;
					}
				}
				else//value<0
				{
					pVehicle->OnAction(eVAI_TurnLeft, eAAM_OnPress, -value, actorId);
					m_bMovementFlagLeft = true;
				}
			}
			break;
		}
  case (eVAI_XIMoveY):
		{
			EVehicleActionIds eForward = eVAI_MoveForward;
			EVehicleActionIds eBack = eVAI_MoveBack;
			if(!strcmp("Asian_helicopter",pVehicle->GetEntity()->GetClass()->GetName()))
			{
				eForward = eVAI_MoveUp;
				eBack = eVAI_MoveDown;
			}

			if(value>0.f)
			{
				pVehicle->OnAction(eForward, eAAM_OnPress, value, actorId);
				m_bMovementFlagForward = true;
			}
			else if(value==0.f)
			{
				if(m_bMovementFlagForward)
				{
					pVehicle->OnAction(eForward, eAAM_OnRelease, 0.f, actorId);
					m_bMovementFlagForward = false;
				}
				else if(m_bMovementFlagBack)
				{
					pVehicle->OnAction(eBack, eAAM_OnRelease, 0.f, actorId);
					m_bMovementFlagBack = false;
				}
			}			
			else//value<0.f
			{
				pVehicle->OnAction(eBack, eAAM_OnPress, -value, actorId);
				m_bMovementFlagBack = true;
			}
      break;
		}
	case (eVAI_XIAccelerate):
		{
			if(value>0.f)
			{
				pVehicle->OnAction(eVAI_MoveForward, eAAM_OnPress, value, actorId);
				m_bMovementFlagForward = true;
			}
			else if(value==0.f)
			{
				if(m_bMovementFlagForward)
				{
					pVehicle->OnAction(eVAI_MoveForward, eAAM_OnRelease, 0.f, actorId);
					m_bMovementFlagForward = false;
				}
			}			
		}
		break;
	case (eVAI_XIDeccelerate):
		{
			if(value>0.f)
			{
				pVehicle->OnAction(eVAI_MoveBack, eAAM_OnPress, value, actorId);
				m_bMovementFlagBack = true;
			}
			else if(value==0.f)
			{
				if(m_bMovementFlagBack)
				{
					pVehicle->OnAction(eVAI_MoveBack, eAAM_OnRelease, 0.f, actorId);
					m_bMovementFlagBack = false;
				}
			}	
		}
		break;
  case (eVAI_XIRotateYaw):
		{
			IVehicleMovement *pMovement = pVehicle->GetMovement();
			if(pMovement && pMovement->GetMovementType() == IVehicleMovement::eVMT_Air)
			{
				pVehicle->OnAction(eVAI_RotateYaw, eAAM_OnPress, value, actorId);
			}
			else
			{
				m_xiRotation.x = (5.0f*value)*(5.0f*value)*value;
			}
      break;
		}
  case (eVAI_XIRotatePitch):
		{
			IVehicleMovement *pMovement = pVehicle->GetMovement();
			if(pMovement && pMovement->GetMovementType() == IVehicleMovement::eVMT_Air)
			{
				pVehicle->OnAction(eVAI_RotatePitch, eAAM_OnPress, g_pGameCVars->cl_invertController ? -value : value, actorId);
			}
			else
			{
				m_xiRotation.y = (3.5f*value)*(3.5f*value)*(-value);
				if(g_pGameCVars->cl_invertController)
					m_xiRotation.y*=-1;
			}
      break;
		}
  case (eVAI_RotatePitch):
    {
      if (g_pGameCVars->cl_invertMouse)
        value *= -1.f;
      pVehicle->OnAction(ite->second, activationMode, value, actorId);
      break;
    }
  case (eVAI_TurnLeft):
    {
      if (activationMode == eAAM_OnPress || activationMode == eAAM_OnRelease)
        m_fLeftRight -= value*2.f - 1.f;
			m_fLeftRight = CLAMP(m_fLeftRight, -1.0f, 1.0f);
      pVehicle->OnAction(ite->second, activationMode, -m_fLeftRight, actorId);
      break;
    }
  case (eVAI_TurnRight):
    {
      if (activationMode == eAAM_OnPress || activationMode == eAAM_OnRelease)
        m_fLeftRight += value*2.f - 1.f;
			m_fLeftRight = CLAMP(m_fLeftRight, -1.0f, 1.0f);
      pVehicle->OnAction(ite->second, activationMode, m_fLeftRight, actorId);
      break;
    }  
  case (eVAI_MoveForward):
    {
      if (activationMode == eAAM_OnPress || activationMode == eAAM_OnRelease)
        m_fForwardBackward += value*2.f - 1.f;

			if(activationMode == eAAM_OnRelease)
				m_fForwardBackward = CLAMP(m_fForwardBackward, 0.0f, 1.0f);
			else
				m_fForwardBackward = CLAMP(m_fForwardBackward, -1.0f, 1.0f);
      pVehicle->OnAction(ite->second, activationMode, m_fForwardBackward, actorId);
      break;
    }
  case (eVAI_MoveBack):
    {
      if (activationMode == eAAM_OnPress || activationMode == eAAM_OnRelease)
        m_fForwardBackward -= value*2.f - 1.f;

			if(activationMode == eAAM_OnRelease)
				m_fForwardBackward = CLAMP(m_fForwardBackward, -1.0f, 0.0f);
			else
				m_fForwardBackward = CLAMP(m_fForwardBackward, -1.0f, 1.0f);
      pVehicle->OnAction(ite->second, activationMode, -m_fForwardBackward, actorId);
      break;
    }  
	case (eVAI_RipoffWeapon):
		{
			if(activationMode == eAAM_OnPress)
			{
				// pass this action to the weapon itself, which handles ripping off if weapon is of the right class
				EntityId weaponId = pVehicle->GetCurrentWeaponId(actorId);
				if(weaponId != 0)
				{
					// don't allow ripping off unless the actor is actually using the correct seat
					//	 (eg not for driver-controlled guns)
					IVehicleSeat* pWeaponSeat = pVehicle->GetWeaponParentSeat(weaponId);
					IVehicleSeat* pOwnerSeat = pVehicle->GetSeatForPassenger(actorId);
					if(pWeaponSeat == pOwnerSeat)
					{
						IItem* pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId);
						if(pItem)
						{
							const CGameActions& actions = g_pGame->Actions();
							pItem->OnAction(actorId, actions.special, activationMode, value);			// 'special' is the action for ripping off guns
						}
					}
				}
			}
		}
		break;
	case (eVAI_ZoomIn):
	case (eVAI_ZoomOut):
		if(SAFE_HUD_FUNC_RET(GetModalHUD()))
			break;
#if PJH_MERGE_HACK
	case (eVAI_Cloak):
	case (eVAI_Uncloak):
		{
			IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
			CActor *pCActor = static_cast<CActor*>(pActor);
			if(pCActor)
			{
				SNanoSuitEvent event;
				event.event = (ite->second == eVAI_Cloak)? eNanoSuitEvent_CLOAK : eNanoSuitEvent_UNCLOAK;
				pCActor->GetNanoSuit()->ProcessEvent(event);
			}
		}
		break;
#endif
  default:
		pVehicle->OnAction(ite->second, activationMode, value, actorId);
    break;		
	}
}

//------------------------------------------------------------------------

void CVehicleClient::PreUpdate(IVehicle* pVehicle, EntityId actorId)
{
	// Controller framerate compensation needs frame time! 
	// The constant is to compensate for small frame time values.
	if(fabsf(m_xiRotation.x) > 0.001)
	{
		pVehicle->OnAction(eVAI_RotateYaw, eAAM_OnPress, m_xiRotation.x*gEnv->pTimer->GetFrameTime()*30.0f, actorId);
	}
	if(fabsf(m_xiRotation.y) > 0.001)
	{
		pVehicle->OnAction(eVAI_RotatePitch, eAAM_OnPress, m_xiRotation.y*gEnv->pTimer->GetFrameTime()*30.0f, actorId);
	}

#if 0
	// This code is commented out until dirty static_cast is sorted
	int blownTires = 0;
	IVehicleMovement* pVehicleMovement = pVehicle->GetMovement();
	if(pVehicleMovement->GetMovementType() == IVehicleMovement::eVMT_Land)
	{
		CVehicleMovementStdWheeled *pVehicleMovementStdWheeled = static_cast<CVehicleMovementStdWheeled *>(pVehicleMovement);
		if(pVehicleMovementStdWheeled)
		{
			blownTires = pVehicleMovementStdWheeled->GetBlownTires();
		}
	}

	if(blownTires > 0)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
		CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
		if(pPlayer && pPlayer->IsPerkActive(ePerk_ReinflatingTires))
		{
			if(pPlayer->AreReinflatingTiresCharged())
			{
				SVehicleEventParams params;
				pVehicle->BroadcastVehicleEvent(eVE_RepairTires, params);
				pPlayer->ResetReinflatingTires();
			}
			else
			{
				pPlayer->SetReinflatingTiresCharging();
			}
		}
	}
#endif
}

//------------------------------------------------------------------------
void CVehicleClient::OnEnterVehicleSeat(IVehicleSeat* pSeat)
{
	m_bMovementFlagRight=m_bMovementFlagLeft=m_bMovementFlagForward=m_bMovementFlagBack=false;
  m_fLeftRight = m_fForwardBackward = 0.f;

	bool isThirdPerson = m_tp;

	TVehicleViewId viewId = InvalidVehicleViewId;
	TVehicleViewId firstViewId = InvalidVehicleViewId;

	while (viewId = pSeat->GetNextView(viewId))
	{
		if (viewId == firstViewId)
			break;

		if (firstViewId == InvalidVehicleViewId)
			firstViewId = viewId;

		if (IVehicleView* pView = pSeat->GetView(viewId))
		{
			if (pView->IsThirdPerson() == isThirdPerson)
				break;
		}
	}

	if (viewId != InvalidVehicleViewId)
		pSeat->SetView(viewId);

	IActionMapManager* pMapManager = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	assert(pMapManager);

	pMapManager->EnableActionMap("landvehicle", false);
	pMapManager->EnableActionMap("seavehicle", false);
	pMapManager->EnableActionMap("helicopter", false);
	pMapManager->EnableActionMap("vtol", false);
}

//------------------------------------------------------------------------
void CVehicleClient::OnExitVehicleSeat(IVehicleSeat* pSeat)
{
	if(pSeat && pSeat->IsDriver())
	{
		m_bMovementFlagRight=m_bMovementFlagLeft=m_bMovementFlagForward=m_bMovementFlagBack=false;
	  m_fLeftRight = m_fForwardBackward = 0.f;
	}
  
  TVehicleViewId viewId = pSeat->GetCurrentView();
	
  if (viewId != InvalidVehicleViewId)  
  {
    if (IVehicleView* pView = pSeat->GetView(viewId))    
      m_tp = pView->IsThirdPerson();
    
    pSeat->SetView(InvalidVehicleViewId);
  }
}

void CVehicleClient::OnEnterVehicle(IVehicle* pVehicle, const EntityId passengerId)
{
	m_playerId = passengerId;
	m_vehicleId = pVehicle->GetEntityId();
}

void CVehicleClient::OnExitVehicle(IVehicle* pVehicle, const EntityId passengerId)
{
	m_playerId = 0;
	m_vehicleId = 0;
}

//------------------------------------------------------------------------
CVehicleClient::SVehicleClientInfo& CVehicleClient::GetVehicleClientInfo(IVehicle* pVehicle)
{
	IEntityClass* pClass = pVehicle->GetEntity()->GetClass();

	TVehicleClientInfoMap::iterator ite = m_vehiclesInfo.find(pClass);

	if (ite == m_vehiclesInfo.end())
	{
		// we need to add this class in our list
		SVehicleClientInfo clientInfo;
		clientInfo.seats.resize(pVehicle->GetSeatCount());

		TVehicleSeatClientInfoVector::iterator seatInfoIte = clientInfo.seats.begin();
		TVehicleSeatClientInfoVector::iterator seatInfoEnd = clientInfo.seats.end();
		TVehicleSeatId seatId = InvalidVehicleSeatId;

		for (; seatInfoIte != seatInfoEnd; ++seatInfoIte)
		{
			seatId++;

			SVehicleSeatClientInfo& seatInfo = *seatInfoIte;
			seatInfo.seatId = seatId;
			seatInfo.viewId = InvalidVehicleViewId;
		}


		m_vehiclesInfo.insert(TVehicleClientInfoMap::value_type(pClass, clientInfo));

		ite = m_vehiclesInfo.find(pClass);
	}

	// this will never happen
	assert(ite != m_vehiclesInfo.end());
	
	return ite->second;
}

//------------------------------------------------------------------------
CVehicleClient::SVehicleSeatClientInfo& 
	CVehicleClient::GetVehicleSeatClientInfo(SVehicleClientInfo& vehicleClientInfo, TVehicleSeatId seatId)
{
	TVehicleSeatClientInfoVector::iterator seatInfoIte = vehicleClientInfo.seats.begin();
	TVehicleSeatClientInfoVector::iterator seatInfoEnd = vehicleClientInfo.seats.end();

	for (; seatInfoIte != seatInfoEnd; ++seatInfoIte)
	{
		SVehicleSeatClientInfo& seatClientInfo = *seatInfoIte;
		if (seatClientInfo.seatId == seatId)
			return *seatInfoIte;
	}

	// will never happen, unless the vehicle has new seat created after the 
	// game started
	assert(0);
	return vehicleClientInfo.seats[0];
}
