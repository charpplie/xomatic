#include "StdAfx.h"
#include "HUD_TacticalInfo.h"

#include "HUD/HUDDefines.h"
#include "HUD/HUD_UnifiedAsset.h"


//////////////////////////////////////////////////////////////////////////


CHUD_TacticalInfo::CHUD_TacticalInfo()
: m_dirty(CHUD_TacticalInfo::eDF_All)
, m_primary("")
, m_secondary("")
, m_tertiary("")
, m_health(100)
, m_alertness(0)
, m_suitMode(eNanoSuitMode_Tactical)
, m_object(NULL)
, m_objectPrimary(NULL)
, m_objectSecondary(NULL)
, m_objectTertiary(NULL)
, m_objectHealth(NULL)
, m_objectAlertness(NULL)
, m_show(true)
{
	m_audioInfo.SetSignal( "HUD_TacticalInfo");
	m_audioHeartbeat.SetSignal( "HUD_Heartbeat");
}



CHUD_TacticalInfo::~CHUD_TacticalInfo()
{
	m_audioHeartbeat.Stop();  // TODO: this should not be needed. the signal should stop automatically when the object is destroyed
}



void CHUD_TacticalInfo::Init()
{
	IHUDAsset* pAsset = GetAsset();
	
	HUD_FLASVAROBJ_REG( pAsset, "TacticalInformations", m_object);
	HUD_FLASVAROBJ_REG( pAsset, "TacticalInformations.Primary",m_objectPrimary);
	HUD_FLASVAROBJ_REG( pAsset, "TacticalInformations.Secondary",m_objectSecondary);
	HUD_FLASVAROBJ_REG( pAsset, "TacticalInformations.Tertiary",m_objectTertiary);
	HUD_FLASVAROBJ_REG( pAsset, "TacticalInformations.Health",m_objectHealth);
	HUD_FLASVAROBJ_REG( pAsset, "TacticalInformations.Alertness",m_objectAlertness);
	Show(false);
}



void CHUD_TacticalInfo::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_object);
	HUD_FLASHOBJ_SAFERELEASE(m_objectPrimary);
	HUD_FLASHOBJ_SAFERELEASE(m_objectSecondary);
	HUD_FLASHOBJ_SAFERELEASE(m_objectTertiary);
	HUD_FLASHOBJ_SAFERELEASE(m_objectHealth);
	HUD_FLASHOBJ_SAFERELEASE(m_objectAlertness);
}



void CHUD_TacticalInfo::Update(float frameTime)
{
	if(m_dirty & CHUD_TacticalInfo::eDF_Info)
	{
		const bool show = !m_primary.empty();
		Show(show);
		if(show)
		{
			m_objectPrimary->SetText(m_primary.c_str());
			m_objectSecondary->SetText(m_secondary.c_str());
			m_objectTertiary->SetText(m_tertiary.c_str());
			m_object->Invoke0("setPositions");
		}
	}

	if(m_dirty & CHUD_TacticalInfo::eDF_Health)
	{
		bool shouldShow = (m_suitMode==eNanoSuitMode_Power) && (m_health > 0);
		m_objectHealth->SetVisible(shouldShow);
		if(shouldShow)
		{
			m_objectHealth->GotoAndStop(m_health);
		}
	}

	if(m_dirty & CHUD_TacticalInfo::eDF_Alertness)
	{
		bool shouldShow = (m_suitMode==eNanoSuitMode_Stealth) && (m_alertness != -1);
		m_objectAlertness->SetVisible(shouldShow);
		if(shouldShow)
		{
			m_objectAlertness->GotoAndStop(m_alertness + 1);
			if (!m_audioHeartbeat.IsPlaying())
				m_audioHeartbeat.Play();
		}
	}

	m_dirty = CHUD_TacticalInfo::eDF_None;
}



void CHUD_TacticalInfo::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_OnTacticalInfoChanged)
	{
		SetTacticalDescription(static_cast<char*>(event.GetData(0).GetPtr()), static_cast<char*>(event.GetData(1).GetPtr()), static_cast<char*>(event.GetData(2).GetPtr()));
		SetTacticalHealth(event.GetData(3).GetInt());
		SetTacticalAlertness(event.GetData(4).GetInt());
	}
	else if(event.eventType == eHUDEvent_OnSuitModeChanged)
	{
		SetSuitMode(static_cast<ENanoSuitMode>(event.eventIntData));
	}
}



void CHUD_TacticalInfo::SetTacticalDescription(const char* primary, const char* secondary, const char* tertiary)
{
	if(primary)
	{
		m_primary = primary;
		m_secondary = secondary;
		m_tertiary = tertiary;
		m_dirty |= CHUD_TacticalInfo::eDF_Info;
		
		if (m_primary.empty())
		{
			if (m_audioHeartbeat.IsPlaying())
				m_audioHeartbeat.Stop();
		}
		else
			m_audioInfo.Play();
	}
}



void CHUD_TacticalInfo::SetTacticalHealth(const int health)
{
	if(m_health != health)
	{
		m_health = health;
		m_dirty |= CHUD_TacticalInfo::eDF_Health;
	}
}



void CHUD_TacticalInfo::SetTacticalAlertness(const int alertness)
{
	if(m_alertness != alertness)
	{
		m_alertness = alertness;
		m_dirty |= CHUD_TacticalInfo::eDF_Alertness;
	}
}



void CHUD_TacticalInfo::SetSuitMode(const ENanoSuitMode mode)
{
	if(m_suitMode != mode)
	{
		m_suitMode = mode;
		m_dirty |= CHUD_TacticalInfo::eDF_Health;
		m_dirty |= CHUD_TacticalInfo::eDF_Alertness;
		if (m_audioHeartbeat.IsPlaying())
			m_audioHeartbeat.Stop();
	}
}



void CHUD_TacticalInfo::Show(const bool show)
{
	if(m_show != show)
	{
		SHUDEvent event(eHUDEvent_HUDElementVisibility);
		event.AddData(SHUDEventData(show));
		event.AddData(SHUDEventData((int)eHUDElement_tacticalinfo));
		CHUD::CallEvent(event);
		m_object->SetVisible(show);
		m_show = show;
	}
}


//////////////////////////////////////////////////////////////////////////