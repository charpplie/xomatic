/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Description: LTAG Implementation

-------------------------------------------------------------------------
History:
- 16:09:09	: Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "LTAG.h"

#include "Game.h"
#include "GameActions.h"
#include "LTagSingle.h"
#include "Actor.h"

TActionHandler<CLTag> CLTag::s_actionHandler;

#define LTAG_GRENADES 2

CLTag::CLTag()
: m_visibleGrenades(LTAG_GRENADES)
{
	if(s_actionHandler.GetNumHandlers() == 0)
	{
#define ADD_HANDLER(action, func) s_actionHandler.AddHandler(actions.action, &CLTag::func)
		const CGameActions& actions = g_pGame->Actions();

		ADD_HANDLER(weapon_change_firemode, OnActionSwitchFireMode);
#undef ADD_HANDLER
	}
}


CLTag::~CLTag()
{

}

void CLTag::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(!s_actionHandler.Dispatch(this,actorId,actionId,activationMode,value))
		CWeapon::OnAction(actorId, actionId, activationMode, value);
}

bool CLTag::OnActionSwitchFireMode(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if (m_fm == NULL)
		return true;

	if (activationMode != eAAM_OnPress)
		return true;

	assert(strcmp(m_fm->GetType(), "LTagSingle") == 0);

	CLTagSingle* pLTagFireMode = static_cast<CLTagSingle*>(m_fm);
	pLTagFireMode->NextGrenadeType();

	return true;
}

bool CLTag::CanModify() const
{
	return false;
}

void CLTag::ProcessEvent(SEntityEvent& event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	if (event.event == ENTITY_EVENT_ANIM_EVENT)
	{
		const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
		ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
		if (pAnimEvent && pCharacter)
		{
			AnimationEvent(pCharacter, *pAnimEvent);
		}
	}
	else
	{
		inherited::ProcessEvent(event);
	}
}

void CLTag::AnimationEvent( ICharacterInstance *pCharacter, const AnimEventInstance &event )
{
	if (event.m_EventName)
	{
		if(stricmp(event.m_EventName, "HideGrenade") == 0)
		{
			HideGrenade(pCharacter);
		}
		else if (stricmp(event.m_EventName, "ShowGrenade") == 0)
		{
			ShowGrenades(pCharacter);
		}
	}
}

void CLTag::HideGrenade(ICharacterInstance* pWeaponCharacter)
{
	if (m_fm)
	{
		const int ammoCount = GetAmmoCount(m_fm->GetAmmoType());
		bool shouldHideGrenade = ((ammoCount > 0 ) && (ammoCount <= LTAG_GRENADES)) && (m_visibleGrenades == ammoCount);
		if (shouldHideGrenade)
		{
			const char* grenadeAttachment = (m_visibleGrenades == 2) ? "newShell" : "currentShell";

			HideGrenadeAttachment(pWeaponCharacter, grenadeAttachment, true);

			m_visibleGrenades--;

			if (m_visibleGrenades == 1)
			{
				//This will reset to the default pose, with the grenades in place
				PlayAction(g_pItemStrings->idle_lastGrenade, 0, true, eIPAF_Default|eIPAF_CleanBlending|eIPAF_NoBlend);
			}
		}
	}
}

void CLTag::ShowGrenades(ICharacterInstance* pWeaponCharacter)
{
	CRY_ASSERT(pWeaponCharacter);

	int inventoryCount = GetInventoryAmmoCount(m_fm->GetAmmoType());
	bool shouldShowNewShell = inventoryCount > 1;

	if (m_visibleGrenades == 1)
	{
		HideGrenadeAttachment(pWeaponCharacter, "newShell", !shouldShowNewShell);
	}
	else if (m_visibleGrenades == 0)
	{
		HideGrenadeAttachment(pWeaponCharacter, "newShell", !shouldShowNewShell);
		HideGrenadeAttachment(pWeaponCharacter, "currentShell", false);
	}

	m_visibleGrenades = 2;
	if (inventoryCount <= 1)
		m_visibleGrenades = 1;
}

void CLTag::HideGrenadeAttachment( ICharacterInstance* pWeaponCharacter, const char* attachmentName, bool hide )
{
	CRY_ASSERT(pWeaponCharacter);

	IAttachment* pAttachment = pWeaponCharacter->GetIAttachmentManager()->GetInterfaceByName(attachmentName);
	if (pAttachment)
	{
		pAttachment->HideAttachment(hide ? 1 : 0);
	}
}

void CLTag::Reset()
{
	inherited::Reset();

	SetupGrenadeAttachments(LTAG_GRENADES);
}

void CLTag::FullSerialize( TSerialize ser )
{
	inherited::FullSerialize(ser);

	ser.Value("visibleGrenades", m_visibleGrenades);

	if (ser.IsReading())
	{
		SetupGrenadeAttachments(m_visibleGrenades);
	}

}

void CLTag::SetupGrenadeAttachments(int numberOfVisibleGrenades)
{
	m_visibleGrenades = numberOfVisibleGrenades;

	if (ICharacterInstance* pWeaponCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson))
	{
		HideGrenadeAttachment(pWeaponCharacter, "newShell", (m_visibleGrenades < LTAG_GRENADES));
		HideGrenadeAttachment(pWeaponCharacter, "currentShell", (m_visibleGrenades == 0));
	}
}

void CLTag::OnSelected( bool selected )
{
	inherited::OnSelected(selected);

	if (selected)
	{
		SetupGrenadeAttachments(m_visibleGrenades);
	}
}
//CA: MP design have chosen to have only one grenade type so this is no longer necessary, and there were problems with delegate authority
//Leaving here for now in case the fickle designers change their mind again
/*
void CLTag::Select(bool select)
{
	inherited::Select(select);

	CActor* pOwner = GetOwnerActor();

	if (select && gEnv->bServer && pOwner && pOwner->IsPlayer())
	{
		INetContext *pNetContext = g_pGame->GetIGameFramework()->GetNetContext();	

		if (pNetContext)        
			pNetContext->DelegateAuthority(GetEntityId(), pOwner->GetGameObject()->GetNetChannel());     
	}   
}

bool CLTag::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	//TODO: FIX ME! Yuck, optional group is bad, but firemode can be null. Need a proper solution to client firemode serialisation
	bool optional = (m_fm && (strcmp(m_fm->GetType(), "LTagSingle") == 0));
	
	if(ser.BeginOptionalGroup("firemode", optional))
	{
		CLTagSingle* pLTagFireMode = static_cast<CLTagSingle*>(m_fm);
		pLTagFireMode->NetSerialize(ser, aspect, profile, flags);
	}
	
	return inherited::NetSerialize(ser, aspect, profile, flags);
}
*/