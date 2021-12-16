#include "StdAfx.h"
#include "HUDEventTranslator.h"

#include "HUD/HUD.h"
#include "HUD/HUDEventDispatcher.h"
#include "IGameFramework.h"
#include "Player.h"
#include "IItemSystem.h"
#include "Game.h"
#include "GameRules.h"

//////////////////////////////////////////////////////////////////////////


CHUDEventTranslator::CHUDEventTranslator()
{
	// Init() is controlled by CHUD::Load
}


CHUDEventTranslator::~CHUDEventTranslator()
{
	// Clear() is controlled by CHUD::Load
}

void CHUDEventTranslator::Init( void )
{
	m_currentWeapon = 0;
	m_select_menu_shown = false;
	m_map_menu_shown = false;

	CHUD::AddHUDEventListener(this,"OnInitPlayer");
	CHUD::AddHUDEventListener(this,"OnInitGameRules");
	CHUD::AddHUDEventListener(this,"OnHUDReload");
	CHUD::AddHUDEventListener(this,"OnSpawn");

	CHUD::AddHUDEventListener(this, "ShowSelectMenu");
	CHUD::AddHUDEventListener(this, "HideSelectMenu" );
	CHUD::AddHUDEventListener(this, "ToggleMap");
	CHUD::AddHUDEventListener(this, "ShowSelectMenuMap");
	CHUD::AddHUDEventListener(this, "HideSelectMenuMap");
	CHUD::AddHUDEventListener(this, "GameEnded");
	CHUD::AddHUDEventListener(this, "ShowHostMigrationScreen");
	CHUD::AddHUDEventListener(this, "HideHostMigrationScreen");
	CHUD::AddHUDEventListener(this, "OpenedIngameMenu");
}

void CHUDEventTranslator::Clear( void )
{
	CHUD::RemoveHUDEventListener(this);

	gEnv->pGame->GetIGameFramework()->GetIItemSystem()->UnregisterListener(this);

	if(g_pGame->GetGameRules())
		g_pGame->GetGameRules()->RemoveHitListener(this);

	UnsubscribeWeapon(m_currentWeapon);

	CPlayer* pPlayer = (CPlayer*)gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pPlayer)
	{
		pPlayer->UnregisterPlayerEventListener(this);
		pPlayer->UnregisterPlayerSuitEventListener(this);
	}
}

void CHUDEventTranslator::ModeChanged(ENanoSuitMode mode)
{
	SHUDEvent suitModeEvent;
	suitModeEvent.eventType = eHUDEvent_OnSuitModeChanged;

	if(mode == eNanoSuitMode_Power)
		suitModeEvent.eventPtrData = const_cast<char*>("Power");
	else if(mode == eNanoSuitMode_Armor)
		suitModeEvent.eventPtrData = const_cast<char*>("Armor");
	else if(mode == eNanoSuitMode_Stealth)
		suitModeEvent.eventPtrData = const_cast<char*>("Stealth");
	else if(mode == eNanoSuitMode_Tactical)
		suitModeEvent.eventPtrData = const_cast<char*>("Tactical");

	suitModeEvent.eventIntData = (int)mode;

	CHUD::CallEvent(suitModeEvent);
}

void CHUDEventTranslator::OnModeSelected(ENanoSuitMode mode)
{
	ModeChanged(mode);
}


void CHUDEventTranslator::EnergyChanged(float energy)
{
	SHUDEvent event(eHUDEvent_OnEnergyChanged);
	event.AddData(SHUDEventData(energy));

	CHUD::CallEvent(event);
}



void CHUDEventTranslator::OnSuitStateChanged(ENanoSuitState state)
{
	SHUDEvent event;
	event.eventType = eHUDEvent_OnSuitStateChanged;
	event.eventIntData = (int)state;

	CHUD::CallEvent(event);
}



void CHUDEventTranslator::OnSuitPowerActivated(const bool activated)
{
	SHUDEvent event;
	event.eventType = eHUDEvent_OnSuitPowerActivated;
	event.AddData(SHUDEventData(activated));
	CHUD::CallEvent(event);
}



void CHUDEventTranslator::OnSetActorItem(IActor *pActor, IItem *pItem)
{
	if(!pActor || pActor->GetEntityId() != gEnv->pGame->GetIGameFramework()->GetClientActorId())
		return;

	if(m_currentWeapon)
	{
		UnsubscribeWeapon(m_currentWeapon);
	}

	if(pItem)
	{
		SubscribeWeapon(pItem->GetEntityId());
	}


	SHUDEvent event;
	event.eventType = eHUDEvent_OnItemSelected;
	event.eventIntData = pItem ? pItem->GetEntityId() : 0;

	CHUD::CallEvent(event);

}



void CHUDEventTranslator::OnHit(const HitInfo& hitInfo)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(clientId != hitInfo.shooterId && clientId != hitInfo.targetId)
		return;

	HitInfo info = hitInfo;

	SHUDEvent event;
	event.eventType = eHUDEvent_OnHit;
	event.eventPtrData = &info;

	CHUD::CallEvent(event);
}



void CHUDEventTranslator::OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(clientId != shooterId)
		return;

	SHUDEvent startReloadEvent( eHUDEvent_OnStartReload );
	CHUD::CallEvent(startReloadEvent);
}



void CHUDEventTranslator::OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(clientId != shooterId)
		return;

	SHUDEvent event;
	event.eventType = eHUDEvent_OnReloaded;
	event.eventPtrData = pWeapon;

	CHUD::CallEvent(event);
}

void CHUDEventTranslator::OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(clientId != shooterId)
		return;

	SHUDEvent event;
	event.eventType = eHUDEvent_OnSetAmmoCount;
	event.eventPtrData = pWeapon;

	CHUD::CallEvent(event);
}



void CHUDEventTranslator::OnFireModeChanged(IWeapon *pWeapon, int currentFireMode)
{
	SHUDEvent event;
	event.eventType = eHUDEvent_OnFireModeChanged;
	event.eventPtrData = pWeapon;
	event.eventIntData = currentFireMode;

	CHUD::CallEvent(event);
}



void CHUDEventTranslator::OnDeath(IActor* pActor, bool bIsGod)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(clientId != pActor->GetEntityId() )
		return;

	CHUD::CallEvent(SHUDEvent( eHUDEvent_OnPlayerDeath ));
}



void CHUDEventTranslator::OnHealthChanged(IActor* pActor, float newHealth)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(clientId != pActor->GetEntityId() )
		return;

	SHUDEvent event(eHUDEvent_OnHealthChanged);
	event.AddData(SHUDEventData(newHealth));

	CHUD::CallEvent(event);
}



void CHUDEventTranslator::SubscribeWeapon(EntityId weapon)
{
	if(weapon)
	{
		IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weapon);
		if(pItem)
		{
			IWeapon* pWeapon = pItem->GetIWeapon();
			if(pWeapon)
			{
				pWeapon->AddEventListener(this, "hud");
				OnFireModeChanged(pWeapon, pWeapon->GetCurrentFireMode());
				m_currentWeapon = weapon;
			}
		}
	}
}



void CHUDEventTranslator::UnsubscribeWeapon(EntityId weapon)
{
	if(weapon)
	{
		IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weapon);
		if(pItem)
		{
			IWeapon* pWeapon = pItem->GetIWeapon();
			if(pWeapon)
			{
				pWeapon->RemoveEventListener(this);
			}
		}
	}
	m_currentWeapon = 0;
}


void CHUDEventTranslator::OnHUDEvent( const SHUDEvent& event )
{
	switch(event.eventType)
	{
	case eHUDEvent_OnInitPlayer:
	case eHUDEvent_OnSpawn:
		{
			OnInitPlayer();
		}
		break;
	case eHUDEvent_OnInitGameRules:
		{
			OnInitGameRules();
		}
		break;
	case eHUDEvent_OnHUDReload:
		{
			OnInitGameRules();
			OnInitPlayer();
		}
		break;
	case eHUDEvent_ToggleMap :
		{
			m_map_menu_shown = !m_map_menu_shown;
			SHUDEvent hudEvent;
			if(!m_map_menu_shown)
			{
				hudEvent = eHUDEvent_ShowSelectMenuMap;
			}
			else
			{
				hudEvent = eHUDEvent_HideSelectMenuMap;
			}
			CHUD::CallEvent(hudEvent);
		}
		break;
	case eHUDEvent_ShowSelectMenuMap :
		m_map_menu_shown = true;
		break;
	case eHUDEvent_HideSelectMenuMap :
		m_map_menu_shown = false;
		break;
	case eHUDEvent_ShowSelectMenu :	// Deliberate fall-through
	case eHUDEvent_GameEnded :			// Deliberate fall-through
	case eHUDEvent_ShowHostMigrationScreen :
		m_select_menu_shown = true;
		break;
	case eHUDEvent_HideSelectMenu :	// Deliberate fall-through
	case eHUDEvent_HideHostMigrationScreen :
		{
			SHUDEvent hudEvent(eHUDEvent_HideSelectMenuMap);
			CHUD::CallEvent(hudEvent);
			m_select_menu_shown = false;
		}
		break;
	case eHUDEvent_OpenedIngameMenu :
		{
			SHUDEvent hudEvent(eHUDEvent_HideSelectMenu);
			CHUD::CallEvent(hudEvent);
			hudEvent = eHUDEvent_HideSelectMenuMap;
			CHUD::CallEvent(hudEvent);
		}
		break;
	}
}

void CHUDEventTranslator::OnInitGameRules( void )
{
	gEnv->pGame->GetIGameFramework()->GetIItemSystem()->RegisterListener(this);

	if(g_pGame->GetGameRules())
		g_pGame->GetGameRules()->AddHitListener(this);
}

void CHUDEventTranslator::OnInitPlayer( void )
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	// May be called from HUD reload, before player is ready; in 
	// which case this will be called when the player _is_ ready.
	if(!pActor) 
		return;

	CPlayer* pPlayer = (CPlayer*)(pActor);
	if(pPlayer)
	{
		pPlayer->RegisterPlayerEventListener(this);
		pPlayer->RegisterPlayerSuitEventListener(this);

		CNanoSuit* pNanoSuit = pPlayer->GetNanoSuit();
		if(pNanoSuit)
		{
			ModeChanged(pNanoSuit->GetGameParams().GetMode());
			EnergyChanged(pNanoSuit->GetGameParams().GetEnergy().Get());
			OnHealthChanged	(pActor, (float)pPlayer->GetHealth());
		}
	}

	IItem* pItem = pActor->GetCurrentItem();
	if(pItem)
	{
		OnSetActorItem(pActor, pItem);
	}
}

//////////////////////////////////////////////////////////////////////////
