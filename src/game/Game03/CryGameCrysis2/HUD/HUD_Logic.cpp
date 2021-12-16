#include "StdAfx.h"
#include "HUD_Logic.h"

#include "HUD.h"
#include "HUDState.h"

//////////////////////////////////////////////////////////////////////////

IEntityClass* CHUD_Logic::s_binocularsClass = NULL;

CHUD_Logic::CHUD_Logic()
: m_mode("")
, m_binoculars(false)
{
	s_binocularsClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Binoculars" );
}



CHUD_Logic::~CHUD_Logic()
{
}



void CHUD_Logic::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_OnSuitModeChanged)
	{
		OnSuitModeChanged(static_cast<ENanoSuitMode>(event.eventIntData));
	}
	else if(event.eventType == eHUDEvent_OnItemSelected)
	{
		OnItemSelected((EntityId)event.eventIntData);
	}
}



void CHUD_Logic::OnSuitModeChanged(ENanoSuitMode mode)
{
	// Suit mode doesn't effect HUDStates in multiplayer
	if( gEnv->bMultiplayer )
	{
		return;
	}

	// SInglePlayer Only
	if(mode==eNanoSuitMode_Tactical)
	{
		g_pGame->GetHUD()->ActivateState("sp_tactical");
	}
	else
	{
		g_pGame->GetHUD()->ActivateDefaultState();
	}

}



void CHUD_Logic::OnItemSelected(EntityId item)
{
	IEntity* pItem = gEnv->pEntitySystem->GetEntity(item);
	if (pItem)
	{
		IEntityClass* pClass = pItem->GetClass();
		if(pClass == s_binocularsClass)
		{
			if(!m_binoculars)
			{
				SHUDEvent binocsEvent;
				binocsEvent.eventType = eHUDEvent_OnVisorChanged;
				binocsEvent.eventIntData = 1;
				CHUD::CallEvent(binocsEvent);
				m_binoculars = true;
			}
			return;
		}
	}

	if(m_binoculars)
	{
		SHUDEvent binocsEvent;
		binocsEvent.eventType = eHUDEvent_OnVisorChanged;
		binocsEvent.eventIntData = 0;
		CHUD::CallEvent(binocsEvent);
		m_binoculars = false;
	}
}


//////////////////////////////////////////////////////////////////////////