/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2010.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Base implementation for a take and hold objective
			- Handles keeping track of who is inside the objective area

	-------------------------------------------------------------------------
	History:
	- 10:02:2010  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesHoldObjectiveBase.h"
#include "IXml.h"
#include "GameRules.h"
#include "Player.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#if NUM_ASPECTS > 8
	#define HOLD_OBJECTIVE_STATE_ASPECT		eEA_GameServerA
#else
	#define HOLD_OBJECTIVE_STATE_ASPECT		eEA_GameServerStatic
#endif

//------------------------------------------------------------------------
CGameRulesHoldObjectiveBase::CGameRulesHoldObjectiveBase()
{
	m_spawnPOIType = eSPT_None;
	m_spawnPOIDistance = 0.f;

	g_pGame->GetGameRules()->RegisterClientConnectionListener(this);
	g_pGame->GetGameRules()->RegisterKillListener(this);
	g_pGame->GetGameRules()->RegisterTeamChangedListener(this);

	g_pGame->GetIGameFramework()->RegisterListener(this, "holdObjective", FRAMEWORKLISTENERPRIORITY_GAME);
}

//------------------------------------------------------------------------
CGameRulesHoldObjectiveBase::~CGameRulesHoldObjectiveBase()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterClientConnectionListener(this);
		pGameRules->UnRegisterKillListener(this);
		pGameRules->UnRegisterTeamChangedListener(this);
	}

	g_pGame->GetIGameFramework()->UnregisterListener(this);
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::Init( XmlNodeRef xml )
{
	const int numChildren = xml->getChildCount();
	for (int childIdx = 0; childIdx < numChildren; ++ childIdx)
	{
		XmlNodeRef xmlChild = xml->getChild(childIdx);
		if (!stricmp(xmlChild->getTag(), "SpawnParams"))
		{
			const char *pType = 0;
			if (xmlChild->getAttr("type", &pType))
			{
				if (!stricmp(pType, "avoid"))
				{
					m_spawnPOIType = eSPT_Avoid;
				}
				else
				{
					CryLog("CGameRulesHoldObjectiveBase::Init: ERROR: Unknown spawn point of interest type ('%s')", pType);
				}
				xmlChild->getAttr("distance", m_spawnPOIDistance);
			}
		}
	}

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		m_entities[i].Reset();
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::AddEntityId(int type, EntityId entityId, int index, bool isNewEntity)
{
	CRY_ASSERT(index < HOLD_OBJECTIVE_MAX_ENTITIES);
	CryLog("CGameRulesHoldObjectiveBase::AddEntity() received objective, eid=%i, index=%i", entityId, index);

	SHoldEntityDetails *pDetails = &m_entities[index];
	if (pDetails->m_id)
	{
		// We're overwriting another entity, disable the old one
		CleanUpEntity(pDetails);
	}

	pDetails->m_id = entityId;
	OnNewHoldEntity(pDetails, index);
	CCCPOINT(HoldObjective_AddEntity);

	gEnv->pEntitySystem->AddEntityEventListener(entityId, ENTITY_EVENT_ENTERAREA, this);
	gEnv->pEntitySystem->AddEntityEventListener(entityId, ENTITY_EVENT_LEAVEAREA, this);

	if (gEnv->bServer)
	{
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), HOLD_OBJECTIVE_STATE_ASPECT);

		if (m_spawnPOIType == eSPT_Avoid)
		{
			IGameRulesSpawningModule *pSpawningModule = g_pGame->GetGameRules()->GetSpawningModule();
			if (pSpawningModule)
			{
				pSpawningModule->AddAvoidPOI(entityId, m_spawnPOIDistance, true);
			}
		}
	}

	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	if (pEntity)
	{
		IScriptTable *pScript = pEntity->GetScriptTable();
		CRY_TODO(11, 02, 2009, "function name from xml?");
		if (pScript)
		{
			if (pScript->GetValueType("ActivateCapturePoint") == svtFunction)
			{
				IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
				pScriptSystem->BeginCall(pScript, "ActivateCapturePoint");
				pScriptSystem->PushFuncParam(pScript);
				pScriptSystem->PushFuncParam(isNewEntity);
				pScriptSystem->EndCall();
			}
			SmartScriptTable propertiesTable;
			if (pScript->GetValue("Properties", propertiesTable))
			{
				pDetails->m_controlRadius = 5.f;
				propertiesTable->GetValue("ControlRadius", pDetails->m_controlRadius);
				pDetails->m_controlRadiusSqr = (pDetails->m_controlRadius * pDetails->m_controlRadius);
				pDetails->m_controlHeight = 5.f;
				propertiesTable->GetValue("ControlHeight", pDetails->m_controlHeight);
				pDetails->m_controlOffsetZ = 0.f;
				propertiesTable->GetValue("ControlOffsetZ", pDetails->m_controlOffsetZ);
			}
		}

		const IActor *pLocalPlayer = g_pGame->GetIGameFramework()->GetClientActor();
		if (pLocalPlayer)
		{
			CheckLocalPlayerInside(pDetails, pEntity, pLocalPlayer->GetEntity());
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::RemoveEntityId( int type, EntityId entityId )
{
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id == entityId)
		{
			CleanUpEntity(pDetails);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::ClearEntities(int type)
{
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id)
		{
			CleanUpEntity(pDetails);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::CleanUpEntity(SHoldEntityDetails *pDetails)
{
	CCCPOINT(HoldObjective_CleanUpActiveCaptureEntity);

	OnRemoveHoldEntity(pDetails);

	gEnv->pEntitySystem->RemoveEntityEventListener(pDetails->m_id, ENTITY_EVENT_ENTERAREA, this);
	gEnv->pEntitySystem->RemoveEntityEventListener(pDetails->m_id, ENTITY_EVENT_LEAVEAREA, this);

	if (gEnv->bServer)
	{
		if (m_spawnPOIType == eSPT_Avoid)
		{
			IGameRulesSpawningModule *pSpawningModule = g_pGame->GetGameRules()->GetSpawningModule();
			if (pSpawningModule)
			{
				pSpawningModule->RemovePOI(pDetails->m_id);
			}
		}
	}
	
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
	if (pEntity)
	{
		IScriptTable *pScript = pEntity->GetScriptTable();
		if (pScript && pScript->GetValueType("DeactivateCapturePoint") == svtFunction)
		{
			IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
			pScriptSystem->BeginCall(pScript, "DeactivateCapturePoint");
			pScriptSystem->PushFuncParam(pScript);
			pScriptSystem->PushFuncParam(true);
			pScriptSystem->EndCall();
		}
	}

	pDetails->Reset();
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::OnEntityEvent( IEntity *pEntity,SEntityEvent &event )
{
	EntityId insideId = (EntityId) event.nParam[0];
	int teamId = g_pGame->GetGameRules()->GetTeam(insideId);

	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(insideId);
	if (pActor && pActor->IsPlayer() && teamId)
	{
		SHoldEntityDetails *pDetails = NULL;

		for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			if (m_entities[i].m_id == pEntity->GetId())
			{
				pDetails = &m_entities[i];
				break;
			}
		}
		if (pDetails)
		{
			if (event.event == ENTITY_EVENT_ENTERAREA)
			{
				if (pActor->GetSpectatorMode() == CPlayer::eASM_None)
				{
					stl::push_back_unique(pDetails->m_insideBoxEntities[teamId - 1], insideId);
					CryLog("CGameRulesHoldObjectiveBase::OnEntityEvent(), entity '%s' entered capture entity", pActor->GetEntity()->GetName());
				}
			}
			else if (event.event == ENTITY_EVENT_LEAVEAREA)
			{
				stl::find_and_erase(pDetails->m_insideBoxEntities[teamId - 1], insideId);
				CryLog("CGameRulesHoldObjectiveBase::OnEntityEvent(), entity '%s' left capture entity", pActor->GetEntity()->GetName());
			}
			CheckCylinder(pDetails, g_pGame->GetIGameFramework()->GetClientActorId());
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::InsideStateChanged(SHoldEntityDetails *pDetails)
{
	pDetails->m_insideCount[0] = pDetails->m_insideEntities[0].size();
	pDetails->m_insideCount[1] = pDetails->m_insideEntities[1].size();

	int team1Count = pDetails->m_insideCount[0];
	int team2Count = pDetails->m_insideCount[1];

	int oldControllingTeamId = pDetails->m_controllingTeamId;

	DetermineControllingTeamId(pDetails, team1Count, team2Count);

	if (g_pGame->GetIGameFramework()->GetNetContext())
	{
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), HOLD_OBJECTIVE_STATE_ASPECT);
	}
	OnInsideStateChanged(pDetails);
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::DetermineControllingTeamId(SHoldEntityDetails *pDetails, const int team1Count, const int team2Count)
{
	if (team1Count && !team2Count)
	{
		pDetails->m_controllingTeamId = 1;
		CryLog("CGameRulesHoldObjectiveBase::InsideStateChanged: entity controlled by team 1");
		CCCPOINT(HoldObjective_TeamTanNowInProximity);
	}
	else if (!team1Count && team2Count)
	{
		pDetails->m_controllingTeamId = 2;
		CryLog("CGameRulesHoldObjectiveBase::InsideStateChanged: entity controlled by team 2");
		CCCPOINT(HoldObjective_TeamBlackNowInProximity);
	}
	else if (!team1Count && !team2Count)
	{
		pDetails->m_controllingTeamId = 0;
		CryLog("CGameRulesHoldObjectiveBase::InsideStateChanged: entity controlled by neither team");
		CCCPOINT(HoldObjective_NeitherTeamNowInProximity);
	}
	else
	{
		pDetails->m_controllingTeamId = CONTESTED_TEAM_ID;
		CryLog("CGameRulesHoldObjectiveBase::InsideStateChanged: entity is contested");
		CCCPOINT(HoldObjective_BothTeamsNowInProximity);
	}
}

//------------------------------------------------------------------------
bool CGameRulesHoldObjectiveBase::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == HOLD_OBJECTIVE_STATE_ASPECT)
	{
		for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SHoldEntityDetails *pDetails = &m_entities[i];

			int oldTeam1Count = pDetails->m_insideCount[0];
			int oldTeam2Count = pDetails->m_insideCount[1];

			int team1Count = pDetails->m_insideEntities[0].size();
			int team2Count = pDetails->m_insideEntities[1].size();

			ser.Value("team1Players", team1Count, 'ui5');
			ser.Value("team2Players", team2Count, 'ui5');

			OnNetSerializeHoldEntity(ser, aspect, profile, flags, pDetails, i);

			if (ser.IsReading())
			{
				DetermineControllingTeamId(pDetails, team1Count, team2Count);

				if ((oldTeam1Count != team1Count) || (oldTeam2Count != team2Count))
				{
					pDetails->m_insideCount[0] = team1Count;
					pDetails->m_insideCount[1] = team2Count;

					if (pDetails->m_id)
					{
						OnInsideStateChanged(pDetails);
					}
				}
			}
		}
	}
	
	return true;
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::OnClientDisconnect( int channelId, EntityId playerId )
{
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (!pDetails->m_id)
		{
			continue;
		}

		for (int teamIdx = 0; teamIdx < 2; ++ teamIdx)
		{
			if (stl::find_and_erase(pDetails->m_insideBoxEntities[teamIdx], playerId))
			{
				CheckCylinder(pDetails, g_pGame->GetIGameFramework()->GetClientActorId());
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::OnEntityKilled( const HitInfo &hitInfo )
{
	int teamId = g_pGame->GetGameRules()->GetTeam(hitInfo.targetId);
	if (teamId == 1 || teamId == 2)
	{
		for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SHoldEntityDetails *pDetails = &m_entities[i];
			if (pDetails->m_id && stl::find_and_erase(pDetails->m_insideBoxEntities[teamId - 1], hitInfo.targetId))
			{
				CCCPOINT(HoldObjective_PlayerWithinRangeKilled);
				CheckCylinder(pDetails, g_pGame->GetIGameFramework()->GetClientActorId());
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::OnActionEvent( const SActionEvent& event )
{
	switch(event.m_event)
	{
	case eAE_resetBegin:
		ClearEntities(0);
		break;
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::AwardPlayerPoints( TEntityIdVec *pEntityVec, EGRST scoreType )
{
	IGameRulesScoringModule *pScoringModule = g_pGame->GetGameRules()->GetScoringModule();
	if (pScoringModule)
	{
		int numEntities = pEntityVec->size();
		for (int i = 0; i < numEntities; ++ i)
		{
			pScoringModule->OnPlayerScoringEvent((*pEntityVec)[i], scoreType);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::Update( float frameTime )
{
	EntityId localPlayerId = g_pGame->GetIGameFramework()->GetClientActorId();

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id && pDetails->m_totalInsideBoxCount)
		{
			CheckCylinder(pDetails, localPlayerId);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::CheckCylinder( SHoldEntityDetails *pDetails, EntityId localPlayerId )
{
	IEntity *pHoldEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
	if (pHoldEntity)
	{
		pDetails->m_totalInsideBoxCount = 0;

		const Vec3 &holdPos = pHoldEntity->GetPos();
		bool hasChanged = false;
		bool localPlayerWithinRange = false;

		for (int teamIdx = 0; teamIdx < NUM_TEAMS; ++ teamIdx)
		{
			const int previousNumEntities = pDetails->m_insideEntities[teamIdx].size();
			pDetails->m_insideEntities[teamIdx].clear();

			const TEntityIdVec &boxEntities = pDetails->m_insideBoxEntities[teamIdx];
			const int numBoxEntities = boxEntities.size();
			pDetails->m_totalInsideBoxCount += numBoxEntities;
			for (int entIdx = 0; entIdx < numBoxEntities; ++ entIdx)
			{
				EntityId playerId = boxEntities[entIdx];
				IEntity *pPlayer = gEnv->pEntitySystem->GetEntity(playerId);
				if (pPlayer)
				{
					const Vec3 &playerPos = pPlayer->GetPos();
					
					const float xDist = playerPos.x - holdPos.x;
					const float yDist = playerPos.y - holdPos.y;

					const float flatDistSqr = (xDist * xDist) + (yDist * yDist);
					if (flatDistSqr < pDetails->m_controlRadiusSqr)
					{
						pDetails->m_insideEntities[teamIdx].push_back(playerId);
						if (playerId == localPlayerId)
						{
							localPlayerWithinRange = true;
						}
					}
				}
			}
			if (pDetails->m_insideEntities[teamIdx].size() != previousNumEntities)
			{
				hasChanged = true;
			}
		}
		if (hasChanged)
		{
			if (localPlayerWithinRange != pDetails->m_localPlayerIsWithinRange)
			{
				if (localPlayerWithinRange)
				{
					pDetails->m_localPlayerIsWithinRange = true;
					CCCPOINT(HoldObjective_LocalPlayerEnterArea);
				}
				else
				{
					pDetails->m_localPlayerIsWithinRange = false;
					CCCPOINT(HoldObjective_LocalPlayerLeaveArea);
				}
			}
			if (gEnv->bServer)
			{
				InsideStateChanged(pDetails);
			}
			else
			{
				OnInsideStateChanged(pDetails);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::OnOwnClientEnteredGame()
{
	CheckLocalPlayerInsideAllEntities();
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if (!oldTeamId)
	{
		CheckLocalPlayerInsideAllEntities();
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::CheckLocalPlayerInsideAllEntities()
{
	const IActor *pLocalPlayer = g_pGame->GetIGameFramework()->GetClientActor();
	if (pLocalPlayer)
	{
		const IEntity *pLocalEntity = pLocalPlayer->GetEntity();
		for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SHoldEntityDetails *pDetails = &m_entities[i];
			if (pDetails->m_id)
			{
				IEntity *pHoldEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
				if (pHoldEntity)
				{
					CheckLocalPlayerInside(pDetails, pHoldEntity, pLocalEntity);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesHoldObjectiveBase::CheckLocalPlayerInside(SHoldEntityDetails *pDetails, const IEntity *pHoldEntity, const IEntity *pLocalPlayer)
{
	const int teamId = g_pGame->GetGameRules()->GetTeam(pLocalPlayer->GetId());
	const int teamIdx = teamId - 1;

	if ((teamId == 1) || (teamId == 2))
	{
		const Vec3 holdEntPos = pHoldEntity->GetWorldPos();
		const Vec3 playerPos = pLocalPlayer->GetWorldPos();

		const Vec3 diff = (playerPos - holdEntPos);
		// Check z first
		if ((diff.z >= pDetails->m_controlOffsetZ) && (diff.z <= (pDetails->m_controlOffsetZ + pDetails->m_controlHeight)))
		{
			// Now check x & y (box test)
			if ((fabs(diff.x) <= pDetails->m_controlRadius) && (fabs(diff.y) <= pDetails->m_controlRadius))
			{
				EntityId localPlayerId = pLocalPlayer->GetId();
				stl::push_back_unique(pDetails->m_insideBoxEntities[teamIdx], localPlayerId);

				// Now check cylinder
				const float flatDistSqr = (diff.x * diff.x) + (diff.y * diff.y);
				if (flatDistSqr < pDetails->m_controlRadiusSqr)
				{
					// Player is inside
					pDetails->m_localPlayerIsWithinRange = true;
					const int teamId = g_pGame->GetGameRules()->GetTeam(localPlayerId);
					stl::push_back_unique(pDetails->m_insideEntities[teamIdx], localPlayerId);

					// Update actual objective implementation
					OnInsideStateChanged(pDetails);
				}
			}
		}
	}
}
