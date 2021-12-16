#include "StdAfx.h"
#include "HUD_SuitMenu.h"
#include "HUD.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUD_UnifiedAsset.h"

//////////////////////////////////////////////////////////////////////////


CHUD_SuitMenu::CHUD_SuitMenu()
 : m_mode(eNanoSuitMode_Invalid)
 , m_menuObject(NULL)
 , m_powerObject(NULL)
 , m_tacticalObject(NULL)
 , m_stealthObject(NULL)
 , m_armorObject(NULL)
{
}



CHUD_SuitMenu::~CHUD_SuitMenu()
{
}



void CHUD_SuitMenu::Init()
{
	IHUDAsset* pAsset = GetAsset();
/*
	HUD_FLASVAROBJ_REG(pAsset, "Root_SuitMenu", m_menuObject);
	HUD_FLASVAROBJ_REG(pAsset, "Root_SuitMenu.Power", m_powerObject);
	HUD_FLASVAROBJ_REG(pAsset, "Root_SuitMenu.Stealth", m_stealthObject);
	HUD_FLASVAROBJ_REG(pAsset, "Root_SuitMenu.Armor", m_armorObject);
	HUD_FLASVAROBJ_REG(pAsset, "Root_SuitMenu.Tactical", m_tacticalObject);
*/
	SuitMenuChanged(false);
}



void CHUD_SuitMenu::PreDelete()
{
/*
	HUD_FLASHOBJ_SAFERELEASE(m_menuObject);
	HUD_FLASHOBJ_SAFERELEASE(m_powerObject);
	HUD_FLASHOBJ_SAFERELEASE(m_stealthObject);
	HUD_FLASHOBJ_SAFERELEASE(m_armorObject);
	HUD_FLASHOBJ_SAFERELEASE(m_tacticalObject);
*/
}



void CHUD_SuitMenu::OnHUDEvent(const SHUDEvent& event)
{
/*
	switch (event.eventType)
	{
	case eHUDEvent_OnSuitModeChanged:
		{
			OnSuitModeChanged((ENanoSuitMode)event.eventIntData);
		}
		break;
	case eHUDEvent_OnSuitMenuOpened:
		{
			SuitMenuChanged(true);
		}
		break;
	default:
		{
			CRY_ASSERT(event.eventType == eHUDEvent_OnSuitMenuClosed);
			SuitMenuChanged(false);
		}
		break;
	}
*/
}
 


void CHUD_SuitMenu::OnSuitModeChanged(const ENanoSuitMode mode)
{
	IHUDAsset* pMenu = GetAsset();

	if(m_mode!=eNanoSuitMode_Invalid)
	{
		SetFrame(m_mode, 1);
	}

	m_mode = mode;
	SetFrame(mode, 2);
}



void CHUD_SuitMenu::SetFrame(const ENanoSuitMode mode, const int frame)
{
	switch(mode)
	{
	case eNanoSuitMode_Power:
//		m_powerObject->GotoAndStop(frame);
		break;
	case eNanoSuitMode_Stealth:
//		m_stealthObject->GotoAndStop(frame);
		break;
	case eNanoSuitMode_Armor:
//		m_armorObject->GotoAndStop(frame);
		break;
	case eNanoSuitMode_Tactical:
//		m_tacticalObject->GotoAndStop(frame);
		break;
	}
}



void CHUD_SuitMenu::SuitMenuChanged(const bool show)
{
//	m_menuObject->SetVisible(show);
}


//////////////////////////////////////////////////////////////////////////