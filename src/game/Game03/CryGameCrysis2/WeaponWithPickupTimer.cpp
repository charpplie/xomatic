/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Weapon that requires the user to hold the 'use' button for
             a while to pick it up

-------------------------------------------------------------------------
History:
- 18:11:2009 : Created by Colin Gulliver

*************************************************************************/
#include "StdAfx.h"
#include "WeaponWithPickupTimer.h"

#include "ItemParamReader.h"
#include "Player.h"
#include "IPlayerInput.h"
#include "GameRules.h"
#include "IInteractor.h"

#include "HUD/UI/UISimpleBar.h"
#include "HUD/UI/UIButtonPromptRegion.h"

//------------------------------------------------------------------------
void CWeaponWithPickupTimer::ResetVariables()
{
	m_pickupProgress = 0.f;
	m_pickerId = 0;
}

//------------------------------------------------------------------------
CWeaponWithPickupTimer::CWeaponWithPickupTimer()
{
	ResetVariables();

	// Initialise to something small since 0 could cause a divide by 0 error
	m_pickupTimerLength = 0.0001f;
}

//------------------------------------------------------------------------
void CWeaponWithPickupTimer::OnReset()
{
	CWeapon::OnReset();

	ResetVariables();
}

//------------------------------------------------------------------------
void CWeaponWithPickupTimer::PostInit( IGameObject * pGameObject )
{
	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	CWeapon::PostInit(pGameObject);
}

//-------------------------------------------------------------------------
CWeaponWithPickupTimer::~CWeaponWithPickupTimer()
{
}

//------------------------------------------------------------------------
bool CWeaponWithPickupTimer::ReadItemParams(const IItemParamsNode *root)
{
	if (!CWeapon::ReadItemParams(root))
		return false;

	const IItemParamsNode *timingNode = root->GetChild("timing");
	if (timingNode)
	{
		CItemParamReader reader(timingNode);
		reader.Read("pickupTime", m_pickupTimerLength);

		if (m_pickupTimerLength == 0.0f)
		{
			m_pickupTimerLength = 0.0001f;
		}
	}

	return true;
}

//-------------------------------------------------------------------------
bool CWeaponWithPickupTimer::IsPickable() const
{
	return false;
}

//-------------------------------------------------------------------------
bool CWeaponWithPickupTimer::CanPickUp(EntityId userId) const
{
	return false;
}

//-------------------------------------------------------------------------
bool CWeaponWithPickupTimer::CanUse(EntityId userId) const
{
	int weaponTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());
	int userTeam = g_pGame->GetGameRules()->GetTeam(userId);

	return ((weaponTeam && (weaponTeam == userTeam)) || !weaponTeam) && !m_owner.GetId();
}

//-------------------------------------------------------------------------
void CWeaponWithPickupTimer::Use(EntityId userId)
{
	if (!m_pickerId)
	{
		StartUse(userId);
	}
}

//------------------------------------------------------------------------
void CWeaponWithPickupTimer::StartUse( EntityId userId )
{
	if (gEnv->bServer)
	{
		CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(userId));
		if (pActor)
		{
			if (!pActor->IsMigrating())
			{
				pActor->GetGameObject()->InvokeRMI(CActor::ClStartUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);
			}
			else
			{
				PickUp(userId, false);
				return;
			}
		}
	}
	m_pickerId = userId;
	m_pickupProgress = 0.f;
}

//------------------------------------------------------------------------
void CWeaponWithPickupTimer::Update( SEntityUpdateContext& ctx, int slot)
{
	if (eIUS_General == slot && m_pickerId)
	{
		CPlayer* pPickerPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_pickerId));
		if (pPickerPlayer && pPickerPlayer->GetPlayerInput())
		{
			if (pPickerPlayer->IsClient())
			{
				bool canUse = false;

				uint32 actions = pPickerPlayer->GetPlayerInput()->GetActions();
				if((actions & ACTION_USE) == ACTION_USE)
				{
					IInteractor *pInteractor = pPickerPlayer->GetInteractor();
					if (pInteractor)
					{
						if (GetEntityId() == (EntityId) pInteractor->GetOverEntityId())
						{
							canUse = true;
						}
					}
				}

				if (canUse)
				{
					CRY_TODO(18, 11, 2009, "Text should be localised and should use own hud element");
					CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", "Picking up...", NULL, 0.5f);

					CUISimpleBar * deployBar = CUISimpleBar::GetInstanceWithName("JavelinBar");
					if (deployBar)
					{
						deployBar->Set(m_pickupProgress);
					}
				}
				else
				{
					m_pickerId = 0;
					m_pickupProgress = 0.f;
					if (!gEnv->bServer)
					{
						GetGameObject()->InvokeRMI(SvStopPickingUp(), NoParams(), eRMI_ToServer);
					}
				}
			}

			m_pickupProgress += ctx.fFrameTime / m_pickupTimerLength;

			if (gEnv->bServer && (m_pickupProgress > 1.f))
			{
				PickUp(m_pickerId, true);
				m_pickerId = 0;
			}
		}
	}

	CWeapon::Update(ctx, slot);
}

//------------------------------------------------------------------------
void CWeaponWithPickupTimer::Drop( float impulseScale/*=1.0f*/, bool selectNext/*=true*/, bool byDeath/*=false*/ )
{
	CWeapon::Drop(impulseScale, selectNext, byDeath);

	m_pickerId = 0;
	m_pickupProgress = 0.f;

	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeaponWithPickupTimer, SvStopPickingUp)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(g_pGame->GetIGameFramework()->GetGameChannelId(pNetChannel));
	if (pActor && pActor->GetEntityId() == m_pickerId)
	{
		m_pickerId = 0;
		m_pickupProgress = 0.f;
	}

	return true;
}
