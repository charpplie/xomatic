/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description:
Handles player interaction...
* Sometimes prompts and responds to inputs
* Sometimes automatic
**************************************************************************/

#include "StdAfx.h"
#include "PlayerPlugin_Interaction.h"
#include "Player.h"
#include "GameCVars.h"
#include "Utility/CryWatch.h"

#if defined(_DEBUG)
static AUTOENUM_BUILDNAMEARRAY(s_playerInteractionTypeNames, PlayerInteractionTypeList);
#endif

void CPlayerPlugin_Interaction::Update(float dt)
{
	assert (IsEntered());

	const CPlayer::SInteractionInfo& interactionInfo = m_ownerPlayer->GetCurrentInteractionInfo();
	IItem * pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(interactionInfo.interactiveEntityId);

#if defined(_DEBUG)
	PlayerPluginWatch ("Can use %s (%s)", pItem ? pItem->GetEntity()->GetName() : "nothing", s_playerInteractionTypeNames[interactionInfo.interactionType]);

	if (pItem)
	{
		EntityId newNearestEntityId = pItem->GetEntityId();
		if (m_lastNearestEntityId != newNearestEntityId)
		{
			CryLog("[INTERACTION] %s '%s' is now near interactive %s '%s' (%s)", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), pItem->GetEntity()->GetClass()->GetName(), pItem->GetEntity()->GetName(), s_playerInteractionTypeNames[interactionInfo.interactionType]);
			m_lastNearestEntityId = newNearestEntityId;
		}
	}
#endif

	switch (interactionInfo.interactionType)
	{
		case CPlayer::eInteraction_PickupAmmo:
//		CryLog("[INTERACTION] %s '%s' is automatically collecting %s '%s'", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), pItem->GetEntity()->GetClass()->GetName(), pItem->GetEntity()->GetName());
		//m_ownerPlayer->PickUpItem(interactionInfo.interactiveEntityId, true);
		m_ownerPlayer->PickUpItemAmmo(interactionInfo.interactiveEntityId);
		break;
	}
}

// This class may want to respond to events in the future... if not, this function should be removed [TF]
void CPlayerPlugin_Interaction::HandleEvent(EPlayerPlugInEvent theEvent, void * data)
{
	CPlayerPlugin::HandleEvent(theEvent, data);

	switch (theEvent)
	{
		case EPE_Reset:
#if defined(_DEBUG)
		m_lastNearestEntityId = 0;
#endif
		break;
	}
}
