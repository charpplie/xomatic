/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: CHUDMissionObjectives manages the status of a mission objective.
Also the description of the objective is saved here.
CHUDMissionObjectiveSystem encapsulates all objectives of a level.

-------------------------------------------------------------------------
History:
- 01/02:2006: Created by Jan Müller

*************************************************************************/
#ifndef __HUD_MISSION_OBJECTIVE_SYSTEM_H__
#define __HUD_MISSION_OBJECTIVE_SYSTEM_H__

#pragma once

class CHUDMissionObjectiveSystem;
class CHUDMissionObjective
{
	friend class CHUDMissionObjectiveSystem;

public:
	enum HUDMissionStatus
	{
		FIRST = 0,
		DEACTIVATED = FIRST,
		COMPLETED,
		FAILED,
		ACTIVATED,
		LAST = ACTIVATED
	};

	struct HUDMissionStatusChange
	{
		HUDMissionStatus	m_status;
		EntityId					m_following;

		HUDMissionStatusChange() : m_status(FIRST), m_following(0)
		{}
		HUDMissionStatusChange(HUDMissionStatus status, EntityId following) : m_status(status), m_following(following)
		{}

		void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing*/ }
	};

	CHUDMissionObjective() : m_pMOS(0), m_trackedEntity(0), m_eStatus(DEACTIVATED), m_lastTimeChanged(0), m_secondary(false), m_tactical(false)
	{
	}

	CHUDMissionObjective(CHUDMissionObjectiveSystem* pMOS, const char* id, const char* shortMsg, const char* msg = 0, const char* mapLabel = 0, const char* uiTexture = 0, bool secondaryObjective = false, bool tactical = false)
											: m_pMOS(pMOS), m_shortMessage(shortMsg), m_screenMessage(msg), m_id(id), m_silent(false), m_mapLabel(mapLabel), m_uiTexture(uiTexture), m_tactical(tactical)
	{
		m_eStatus = DEACTIVATED;
		m_trackedEntity = 0;
		m_lastTimeChanged = 0;
		m_secondary = secondaryObjective;
	}

	HUDMissionStatus GetStatus() const
	{
		return m_eStatus;
	}

	void SetStatus(HUDMissionStatus);

	ILINE void SetSilent(bool silent) { m_silent = silent; }
	ILINE bool IsSilent() const { return m_silent; }

	ILINE bool IsSecondary() const { return m_secondary; }

	ILINE void SaveMPChange(int team, HUDMissionStatus status, EntityId attachedTo)
	{
		m_mpChangeMap[team] = HUDMissionStatusChange(status, attachedTo);
	}

	ILINE const std::map<int, HUDMissionStatusChange>* GetMPChangeMap() const
	{
		return &m_mpChangeMap;
	}

	ILINE EntityId GetTrackedEntity() const
	{
		return m_trackedEntity;
	}

	void SetTrackedEntity(EntityId entityID);
	
	ILINE bool GetRadar() const
	{
		return m_trackedEntity != 0;
	}

	ILINE bool IsActivated() const
	{
		return (m_eStatus == ACTIVATED);
	}

	ILINE bool IsTactical() const
	{
		return m_tactical;
	}

	ILINE const char* GetShortDescription() const
	{
		return m_shortMessage.c_str();
	}

	ILINE const char* GetID() const
	{
		return m_id.c_str();
	}

	ILINE const char* GetMessage() const
	{
		return m_screenMessage.c_str();
	}

	ILINE const char* GetMapLabel() const
	{
		if(m_mapLabel.size() == 0)
			return m_shortMessage.c_str();
		return m_mapLabel.c_str();
	}

	ILINE const char* GetUITexture() const
	{
		return m_uiTexture.c_str();
	}

	void Serialize(TSerialize ser)
	{
		ser.Value("m_id", m_id);
		ser.Value("m_shortDescription", m_shortMessage);
		ser.Value("m_screenMessage", m_screenMessage);
		ser.Value("m_mapLabel", m_mapLabel);
		ser.Value("m_uiTexture", m_uiTexture);
		ser.Value("m_trackedEntity", m_trackedEntity, 'eid');
		ser.Value("m_silent", m_silent);
		ser.Value("m_secondary", m_secondary);
		ser.EnumValue("m_eStatus", m_eStatus, FIRST, LAST);
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(m_shortMessage);
		pSizer->AddObject(m_screenMessage);
		pSizer->AddObject(m_id);		
		pSizer->AddObject(m_mapLabel);
		pSizer->AddObject(m_uiTexture);
		pSizer->AddObject(m_mpChangeMap);
	}

protected:

	ILINE void SetMOS(CHUDMissionObjectiveSystem* pMOS)
	{
		m_pMOS = pMOS;
	}

private:
	CHUDMissionObjectiveSystem* m_pMOS;
	string						m_shortMessage;
	string						m_screenMessage;
	string						m_id;
	string						m_mapLabel;
	string						m_uiTexture;

	EntityId          m_trackedEntity;
	HUDMissionStatus	m_eStatus;
	float							m_lastTimeChanged;
	bool							m_silent;
	bool							m_secondary;
	bool							m_tactical;

	std::map<int, HUDMissionStatusChange> m_mpChangeMap;
};

class CHUDMissionObjectiveSystem
{
	//this is a list of the current level's mission objectives ...
	std::vector<CHUDMissionObjective> m_currentMissionObjectives;
	bool m_bLoadedObjectives;

public:

	CHUDMissionObjectiveSystem();

	const std::vector<CHUDMissionObjective>& GetObjectives() const
	{
		return m_currentMissionObjectives;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const;

	//get the description of the objective
	const char* GetMissionObjectiveLongDescription(const char* objectiveShort);

	//get the name of the objective
	const char* GetMissionObjectiveShortDescription(const char* id);

	//loads the level's mission objectives from a XML file
	void LoadLevelObjectives(bool forceReloading = false);

	//deactivate all loaded objectives
	void DeactivateObjectives(bool leaveOutRecentlyChanged = false);

	//get a pointer to the objective (NULL if not available)
	//TODO: don't return ptr into a vector! If vector changes, ptr is trash!
	CHUDMissionObjective* GetMissionObjective(const char* id);

	CHUDMissionObjective* GetMissionObjectiveByEntityId(EntityId id);

	void Serialize(TSerialize ser);	//not tested!!
};

#endif