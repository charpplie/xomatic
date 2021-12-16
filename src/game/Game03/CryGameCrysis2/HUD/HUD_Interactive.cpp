#include "StdAfx.h"
#include "HUD_Interactive.h"
#include "HUD/HUD.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUD_UnifiedAsset.h"

#include "Game.h"
#include "IItemSystem.h"
#include "Utility/CryWatch.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// TODO: Make data driven! Read these from data file!
#if defined(PS3)
static const char * s_weaponInputIcon = "PS3_Square";
static const char * s_useInputIcon = "PS3_Triangle";
#else
static const char * s_weaponInputIcon = "XBox360_X";
static const char * s_useInputIcon = "XBox360_Y";
#endif


CHUD_Interactive::CHUD_Interactive()
: m_forceDraw( 0 )
, m_forceHide (eHide_None)
, m_objectRoot(NULL)
, m_objectWeapons(NULL)
, m_objectRoot2D(NULL)
, m_objectButton(NULL)
, m_showing(false)
{
	gEnv->pConsole->Register("hud_forceInteractive", &m_forceDraw, m_forceDraw, 0, "Draw safe areas.");
}



CHUD_Interactive::~CHUD_Interactive()
{
	gEnv->pConsole->UnregisterVariable("hud_forceInteractive", true);
}



void CHUD_Interactive::Init()
{
	IHUDAsset* pAsset = GetAsset();
	HUD_FLASVAROBJ_REG( pAsset, "Root_Interaction_Text", m_objectRoot);
	HUD_FLASVAROBJ_REG( pAsset, "Root_Interaction_Text.Weapons", m_objectWeapons);

	pAsset = GetAsset2D();
	HUD_FLASVAROBJ_REG( pAsset, "Root_Interaction_Text", m_objectRoot2D);
	HUD_FLASVAROBJ_REG( pAsset, "Root_Interaction_Text.Button", m_objectButton);
}



void CHUD_Interactive::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot);
	HUD_FLASHOBJ_SAFERELEASE(m_objectWeapons);
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot2D);
	HUD_FLASHOBJ_SAFERELEASE(m_objectButton);
}



void CHUD_Interactive::Update(float frameTime)
{
	m_usable.Update(frameTime);
	m_interaction.Update(frameTime);

	CHUD_Interactive::InteractionDisplay* pCurrent;

	if( m_forceDraw )
	{
		if( m_interaction.IsActive() )
		{
			m_interaction.m_timer = 0.0f;
			m_interaction.m_duration = 0.0f;
			m_usable.m_timer = 1.0f;
			m_usable.m_duration = 1.0f;
		}
		else
		{
			m_interaction.m_timer = 1.0f;
			m_interaction.m_duration = 1.0f;
			m_usable.m_timer = 0.0f;
			m_usable.m_duration = 0.0f;
		}
	}

	bool show = m_forceHide==eHide_None;

	if(m_interaction.IsActive())
	{
		pCurrent = &m_interaction;
	}
	else if(m_usable.IsActive())
	{
		pCurrent = &m_usable;
	}
	else
	{
		show = false;
	}

	if(m_showing != show)
	{
		m_showing = show;
		m_objectRoot->SetVisible(show);
		m_objectRoot2D->SetVisible(show);
		SHUDEvent event(eHUDEvent_HUDElementVisibility);
		event.AddData(SHUDEventData(show));
		event.AddData(SHUDEventData((int)eHUDElement_interactions));
		CHUD::CallEvent(event);
	}

	if(!m_showing)
		return;


	if(pCurrent->m_dirty)
	{
		m_objectRoot->SetMember("WeaponIconClass",pCurrent->m_class.c_str());
		m_objectWeapons->Invoke0("Update");

		m_objectRoot2D->SetMember("InteractionText", SFlashVarValue(pCurrent->m_message.c_str()));
		m_objectButton->GotoAndStop(pCurrent->m_button.c_str());

		pCurrent->m_dirty = false;
	}
}



void CHUD_Interactive::OnHUDEvent(const SHUDEvent& event)
{
	switch( event.eventType )
	{
	case eHUDEvent_OnUsableChanged :
		UpdateUsable((EntityId)event.eventIntData, (CPlayer::EInteractionType)event.eventIntData2);
		break;
	case eHUDEvent_OnInteractionRequest :
		InteractionRequest(event.GetData(0).GetBool(), static_cast<char*>(event.GetData(1).GetPtr()), static_cast<char*>(event.GetData(2).GetPtr()), event.GetData(3).GetFloat());
		break;
	case eHUDEvent_LeavingBattleArea :
		m_forceHide |= eHide_BattleAreaWarning;
		break;
	case eHUDEvent_ReturningToBattleArea :
		m_forceHide &= ~eHide_BattleAreaWarning;
		break;
	case eHUDEvent_OnSuitMenuOpened :
		m_forceHide |= eHide_SuitMenu;
		break;
	case eHUDEvent_OnSuitMenuClosed :
		m_forceHide &= ~eHide_SuitMenu;
		break;
	}
}

void CHUD_Interactive::UpdateUsable(EntityId usable, CPlayer::EInteractionType interactionType)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayer)
		return;

	m_usable.m_duration = -1.0f;
	m_usable.m_button = s_useInputIcon;

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(usable);
	if(pEntity)
	{
		IEntityClass* pClass = pEntity->GetClass();
		if(pClass)
		m_usable.m_class = pClass->GetName();
	}
	else
	{
		m_usable.m_class = "None";
	}

	switch (interactionType)
	{
	case CPlayer::eInteraction_PickupItem:
		{
			m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_pickup", GetItemName(usable));
			m_usable.m_button = s_weaponInputIcon;
		}
		break;

	case CPlayer::eInteraction_ExchangeItem:
		{
			m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_exchange", GetItemName(pPlayer->GetCurrentItemId()), GetItemName(usable));
			m_usable.m_button = s_weaponInputIcon;
		}
		break;
	case CPlayer::eInteraction_Grab:
	case CPlayer::eInteraction_GrabEnemy:
		m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_grab");
		break;
	case CPlayer::eInteraction_Use:
		m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_use", GetItemName(usable));
		break;
	case CPlayer::eInteraction_LetGo:
		m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_letgo", GetItemName(usable));
		break;
	case CPlayer::eInteraction_StopUsing:
		m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_stopusing", GetItemName(usable));
		break;
	case CPlayer::eInteraction_Stealthkill:
		m_usable.m_message = g_pGame->GetHUD()->LocalizeStringW("@ui_interaction_stealthkill");
		CRY_FIXME(26, 2, 2009, "Make it work properly for PC keyboard and ps3");
#if defined(PS3)
		m_usable.m_button = "PS3_R3";
#else
		m_usable.m_button = "XBox360_RS";
#endif
		break;
	default:
		m_usable.m_duration = 0.0f;
		break;
	}

	m_usable.m_dirty = true;
}



const char * CHUD_Interactive::GetItemName(const EntityId item) const
{
	const char * reply = "??";
	IItem* pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(item);

	if (pItem)
	{
		reply = pItem->GetDisplayName();

		assert (reply);
		if (reply[0] == '\0')
		{
			if(IEntity* pEntity = pItem->GetEntity())
			{
				if(IEntityClass* pClass = pEntity->GetClass())
				{
					reply = pClass->GetName();
				}
			}
		}
	}

	return reply;
}



void CHUD_Interactive::InteractionRequest(const bool activate, const char* message, const char* button, const float duration)
{
	if(!activate)
	{
		m_interaction.m_timer = 0.0f;
		m_interaction.m_duration = 0.0f;
		m_usable.m_dirty = true;
		return;
	}

	m_interaction.m_button = button;
	m_interaction.m_message = g_pGame->GetHUD()->LocalizeStringW(message);
	m_interaction.m_duration = duration;
	m_interaction.m_timer = duration;
	m_interaction.m_dirty = true;
}


//////////////////////////////////////////////////////////////////////////
