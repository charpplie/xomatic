#include "StdAfx.h"
#include "HUD_ObjectiveDisplay.h"

#include "HUD/HUDMissionObjectiveSystem.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUD_UnifiedAsset.h"


//////////////////////////////////////////////////////////////////////////


CHUD_ObjectiveDisplay::CHUD_ObjectiveDisplay()
: m_dirty(CHUD_ObjectiveDisplay::eDF_None)
, m_show(true)
, m_rootObject(NULL)
{
}



CHUD_ObjectiveDisplay::~CHUD_ObjectiveDisplay()
{
}



void CHUD_ObjectiveDisplay::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_OnObjectiveChanged)
	{
		ObjectiveChanged(static_cast<CHUDMissionObjective*>(event.GetData(0).GetPtr()));
	}
}



void CHUD_ObjectiveDisplay::Init()
{
	IHUDAsset* pAsset = GetAsset();
	HUD_FLASVAROBJ_REG(pAsset, "Root_ObjectivesDisplay", m_rootObject);
	Show(false);
}



void CHUD_ObjectiveDisplay::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_rootObject);
}



void CHUD_ObjectiveDisplay::Update(float frameTime)
{
	if(m_dirty & CHUD_ObjectiveDisplay::eDF_Objective)
	{
		CRY_TODO(11, 3, 2010, "Show all active objectives, not only the first!");
		if(m_objectives.size()>0)
		{
			Show(true);
			CHUDMissionObjective* pObjective = g_pGame->GetMOSystem()->GetMissionObjectiveByEntityId(m_objectives[0]);
			if(pObjective)
			{
				m_rootObject->SetMember("Objective", g_pGame->GetHUD()->LocalizeStringW(pObjective->GetMessage()));
				m_rootObject->Invoke0("setObjectivesDisplayPositions");
			}
		}
		else
		{
			Show(false);
		}
	}

	m_dirty = CHUD_ObjectiveDisplay::eDF_None;
}



void CHUD_ObjectiveDisplay::ObjectiveChanged(const CHUDMissionObjective* pObjective)
{
	const EntityId id = pObjective->GetTrackedEntity();

	int size = m_objectives.size();
	if(pObjective->GetStatus() == CHUDMissionObjective::ACTIVATED)
	{
		if(size<k_maxActiveObjectivesSupported)
		{
			m_objectives.push_back(id);
			m_dirty |= CHUD_ObjectiveDisplay::eDF_Objective;
		}
	}
	else
	{
		for(int i=0; i<size; ++i)
		{
			if(m_objectives[i] == id)
			{
				m_objectives.removeAt(i);
				--i; --size;
				m_dirty |= CHUD_ObjectiveDisplay::eDF_Objective;
			}
		}
	}
}



void CHUD_ObjectiveDisplay::Show(const bool show)
{
	if(m_show != show)
	{
		SHUDEvent event(eHUDEvent_HUDElementVisibility);
		event.AddData(SHUDEventData(show));
		event.AddData(SHUDEventData((int)eHUDElement_objectives));
		CHUD::CallEvent(event);
		m_rootObject->SetVisible(show);
		m_show = show;
	}
}



//////////////////////////////////////////////////////////////////////////