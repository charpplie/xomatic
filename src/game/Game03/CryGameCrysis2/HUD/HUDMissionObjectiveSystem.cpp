#include "StdAfx.h"
#include "HUDMissionObjectiveSystem.h"

#include "ISerialize.h"
#include "Game.h"
#include "HUD/HUD.h"

#include "GameRules.h"

void CHUDMissionObjective::SetStatus(HUDMissionStatus status)
{
	if (status == m_eStatus)
		return;

	m_eStatus = status;

	assert (m_pMOS != 0);

	SHUDEvent objectiveEvent(eHUDEvent_OnObjectiveChanged);
	objectiveEvent.AddData(SHUDEventData((void*)this));
	objectiveEvent.AddData(SHUDEventData(IsSilent()));
	CHUD::CallEvent(objectiveEvent);

	SHUDEvent radarEvent;
	if(m_eStatus == ACTIVATED)
		radarEvent.eventType = eHUDEvent_AddEntity;
	else
		radarEvent.eventType = eHUDEvent_RemoveEntity;
	radarEvent.AddData(SHUDEventData((int)GetTrackedEntity()));
	CHUD::CallEvent(radarEvent);

	m_lastTimeChanged = gEnv->pTimer->GetFrameStartTime().GetSeconds();
}


CHUDMissionObjectiveSystem::CHUDMissionObjectiveSystem()
: m_bLoadedObjectives(false)
{
	LoadLevelObjectives(true);
}



const char* CHUDMissionObjectiveSystem::GetMissionObjectiveLongDescription(const char* id)
{
	const char* message = 0;
	CHUDMissionObjective* pObj = GetMissionObjective(id);
	if (pObj)
		message = pObj->GetMessage();
	return message;
}

const char* CHUDMissionObjectiveSystem::GetMissionObjectiveShortDescription(const char* id)
{
	const char* message = 0;
	CHUDMissionObjective* pObj = GetMissionObjective(id);
	if (pObj)
		message = pObj->GetShortDescription();
	return message;
}

void CHUDMissionObjectiveSystem::LoadLevelObjectives(bool forceReloading)
{
	if(!m_bLoadedObjectives || forceReloading)
	{
		m_currentMissionObjectives.clear();

		const char* filename = "Libs/UI/Objectives_new.xml";
		/*if(gEnv->bMultiplayer)
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();
			if(stricmp (pGameRules->GetEntity()->GetClass()->GetName(), "Coop"))
				filename = "Libs/UI/MP_Objectives.xml";
		}*/

		XmlNodeRef missionObjectives = GetISystem()->LoadXmlFile(filename);
		if (missionObjectives == 0)
			return;

		for(int tag = 0; tag < missionObjectives->getChildCount(); ++tag)
		{
			XmlNodeRef mission = missionObjectives->getChild(tag);
			const char* attrib;
			const char* objective;
			const char* text;
			const char* optional;
			for(int obj = 0; obj < mission->getChildCount(); ++obj)
			{
				XmlNodeRef objectiveNode = mission->getChild(obj);
				string id(mission->getTag());
				id.append(".");
				id.append(objectiveNode->getTag());
				if(objectiveNode->getAttributeByIndex(0, &attrib, &objective) && objectiveNode->getAttributeByIndex(1, &attrib, &text))
				{
					bool secondaryObjective = false;
					bool tactical = false;
					const char* mapLabel = NULL;
					const char* uiTexture = NULL;
					int attribs = objectiveNode->getNumAttributes();
					for(int attribIndex = 2; attribIndex < attribs; ++attribIndex)
					{
						if(objectiveNode->getAttributeByIndex(attribIndex, &attrib, &optional))
						{
							if(attrib)
							{
								if(!stricmp(attrib, "Secondary"))
								{
									if(!stricmp(optional, "true"))
										secondaryObjective = true;
								}
								else if(!stricmp(attrib, "Tactical"))
								{
									if(!stricmp(optional, "true"))
										tactical = true;
								}
								else if(!stricmp(attrib, "MapLabel"))
									mapLabel = optional;
								else if(!stricmp(attrib, "UITexture"))
									uiTexture = optional;
							}
						}
					}
					m_currentMissionObjectives.push_back(CHUDMissionObjective(this, id.c_str(), objective, text, mapLabel, uiTexture, secondaryObjective, tactical));
				}
				else
					GameWarning("Error reading mission objectives.");
			}
		}

		m_bLoadedObjectives = true;
	}
	//else
	//	DeactivateObjectives(true);		//deactive old objectives on level change
}

void CHUDMissionObjectiveSystem::DeactivateObjectives(bool leaveOutRecentlyChanged)
{
	for(int i = 0; i < int(m_currentMissionObjectives.size()); ++i)
	{
		if(!leaveOutRecentlyChanged ||
			(m_currentMissionObjectives[i].m_lastTimeChanged - gEnv->pTimer->GetFrameStartTime().GetSeconds() > 0.2f))
		{
			bool isSilent = m_currentMissionObjectives[i].IsSilent();
			m_currentMissionObjectives[i].SetSilent(true);
			m_currentMissionObjectives[i].SetStatus(CHUDMissionObjective::DEACTIVATED);
			m_currentMissionObjectives[i].SetSilent(isSilent);
		}
	}
}

CHUDMissionObjective* CHUDMissionObjectiveSystem::GetMissionObjective(const char* id)
{
	std::vector<CHUDMissionObjective>::iterator it;
	for(it = m_currentMissionObjectives.begin(); it != m_currentMissionObjectives.end(); ++it)
	{
		if(!strcmp( (*it).GetID(), id))
			return &(*it);
	}

	return NULL;
}


CHUDMissionObjective* CHUDMissionObjectiveSystem::GetMissionObjectiveByEntityId(EntityId id)
{
	std::vector<CHUDMissionObjective>::iterator it;
	for(it = m_currentMissionObjectives.begin(); it != m_currentMissionObjectives.end(); ++it)
	{
		if((*it).GetTrackedEntity() == id)
			return &(*it);
	}

	return NULL;
}

void CHUDMissionObjectiveSystem::Serialize(TSerialize ser)	//not tested!!
{
	if (ser.IsReading())
		LoadLevelObjectives(true);

	float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();

	for(std::vector<CHUDMissionObjective>::iterator it = m_currentMissionObjectives.begin(); it != m_currentMissionObjectives.end(); ++it)
	{
		ser.BeginGroup("HUD_Objective");
		ser.EnumValue("ObjectiveStatus", (*it).m_eStatus, CHUDMissionObjective::FIRST, CHUDMissionObjective::LAST);
		ser.Value("trackedEntity", (*it).m_trackedEntity);
		ser.EndGroup();

		if(ser.IsReading() && (*it).m_eStatus != CHUDMissionObjective::DEACTIVATED)
		{				
			CHUDMissionObjective obj = *it;
			obj.m_lastTimeChanged = now;
			bool isSilent = obj.IsSilent();
			obj.SetSilent(true);
			SAFE_HUD_FUNC(UpdateObjective(&obj));
			obj.SetSilent(isSilent);
		}
	}
}

void CHUDMissionObjectiveSystem::GetMemoryUsage(ICrySizer * s) const
{
	s->Add(*this);
	s->AddObject(m_currentMissionObjectives);	
}

void CHUDMissionObjective::SetTrackedEntity(EntityId entityID)
{	

	m_trackedEntity = entityID;
}
