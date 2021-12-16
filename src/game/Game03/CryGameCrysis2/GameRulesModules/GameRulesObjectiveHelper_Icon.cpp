/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Helper objective, manages placing the correct icon over an entity

	-------------------------------------------------------------------------
	History:
	- 27:11:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesObjectiveHelper_Icon.h"
#include "IXml.h"
#include "GameRules.h"

#include "HUD/HUD.h"

#define GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_IN_BASE			(1 << 0)
#define GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_NOT_IN_BASE	(1 << 1)

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Icon::CGameRulesObjectiveHelper_Icon()
{
	m_pEntityClass = NULL;
	m_neutralIcon = int(EGRMO_Unknown);
	m_friendlyIcon = int(EGRMO_Unknown);
	m_hostileIcon = int(EGRMO_Unknown);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Icon::Init( XmlNodeRef xml )
{
	const char *pEntityClass = 0;
	if (xml->getAttr("class", &pEntityClass))
	{
		m_pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pEntityClass);
	}

	xml->getAttr("neutralIcon", m_neutralIcon);
	xml->getAttr("friendlyIcon", m_friendlyIcon);
	xml->getAttr("hostileIcon", m_hostileIcon);

	CGameRules *pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterClientConnectionListener(this);
	pGameRules->RegisterTeamChangedListener(this);
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Icon::~CGameRulesObjectiveHelper_Icon()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterClientConnectionListener(this);
		pGameRules->UnRegisterTeamChangedListener(this);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Icon::OnStartGame()
{
	if (gEnv->bClient)
	{
		m_entities.clear();

		if (m_pEntityClass)
		{
			IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
			IEntity *pEntity = 0;

			pIt->MoveFirst();

			while(pEntity = pIt->Next())
			{
				if (pEntity->GetClass() == m_pEntityClass)
				{
					m_entities.push_back(SEntityDetails());

					SEntityDetails &pEntityDetails = m_entities.back();
					pEntityDetails.m_id = pEntity->GetId();
					pEntityDetails.m_currentIcon = -1;
				}
			}
		}

		UpdateAllIcons();
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Icon::OnOwnClientEnteredGame()
{
	UpdateAllIcons();
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Icon::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if (entityId == g_pGame->GetIGameFramework()->GetClientActorId())
	{
		UpdateAllIcons();
	}
	else
	{
		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SEntityDetails *pSpawnEntity = &m_entities[i];
			if (pSpawnEntity->m_id == entityId)
			{
				UpdateIcon(pSpawnEntity);
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Icon::UpdateAllIcons()
{
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SEntityDetails *pSpawnEntity = &m_entities[i];

		UpdateIcon(pSpawnEntity);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Icon::UpdateIcon( SEntityDetails *pSpawnEntity )
{
	if (pSpawnEntity->m_currentIcon != int(EGRMO_Unknown))
	{
		SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
		newRemoveObjective.ReserveData(1);
		newRemoveObjective.AddData( static_cast<int>(pSpawnEntity->m_id) ); /*(EntityId)*/
		CHUD::CallEvent(newRemoveObjective);

		pSpawnEntity->m_currentIcon = int(EGRMO_Unknown);
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	int localTeamId = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	int entityTeamId = pGameRules->GetTeam(pSpawnEntity->m_id);

	int requestedIconId = int(EGRMO_Unknown);
	if (entityTeamId == 0)
	{
		requestedIconId = m_neutralIcon;
	}
	else if (entityTeamId == localTeamId)
	{
		requestedIconId = m_friendlyIcon;
	}
	else
	{
		requestedIconId = m_hostileIcon;
	}

	if (requestedIconId != int(EGRMO_Unknown))
	{
		SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
		newMissionObjective.ReserveData(2);
		newMissionObjective.AddData( static_cast<int>(pSpawnEntity->m_id) ); /*(EntityId)*/
		newMissionObjective.AddData( requestedIconId ); /*(EGameRulesMissionObjectives)*/ 
		CHUD::CallEvent(newMissionObjective);

		pSpawnEntity->m_currentIcon = requestedIconId;
	}
}
