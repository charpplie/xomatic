/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a carry objective (take something to a place)

	-------------------------------------------------------------------------
	History:
	- 27:10:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesCarryObjective.h"
#include "IGameRulesObjective.h"
#include "IXml.h"
#include "GameRules.h"
#include "Player.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "GameRulesModules/GameRulesObjectiveHelper_Carry.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "HUD/HUD.h"
#include "HUD/UI/UISimpleBar.h" // TODO : remove in favor of sending a HUDEvent
#include "HUD/UI/UIButtonPromptRegion.h"
#include "Item.h"

#define CARRY_OBJECTIVE_ENTITY_TYPE_CARRY  0
#define CARRY_OBJECTIVE_ENTITY_TYPE_TARGET 1

#if NUM_ASPECTS > 8
	#define CARRY_OBJECTIVE_NET_ASPECT		eEA_GameServerA
#else
	#define CARRY_OBJECTIVE_NET_ASPECT		eEA_GameServerStatic
#endif

//------------------------------------------------------------------------
void CGameRulesCarryObjective::Init( XmlNodeRef xml )
{
	m_friendlyBaseIcon = int(EGRMO_Unknown);
	m_hostileBaseIcon = int(EGRMO_Unknown);
	m_iconPriority = 0;
	m_entitiesCompleted[0] = 0;
	m_entitiesCompleted[1] = 0;
	m_requireOwnAtBase = false;

	int iValue = 0;
	const char* pCharTmp = NULL;

	if (xml->getAttr("removeOnCompletion", iValue))
	{
		m_removeOnCompletion = (iValue != 0);
	}
	if (xml->getAttr("invertTargets", iValue))
	{
		m_invertTargets = (iValue != 0);
	}
	if (xml->getAttr("deployTime", m_deployTime))
	{
		m_useDeployTime = true;
	}
	if (xml->getAttr("requireOwnAtBaseToComplete", iValue))
	{
		m_requireOwnAtBase = (iValue != 0);
	}

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "BaseIcons"))
		{
			xmlChild->getAttr("friendly", m_friendlyBaseIcon);
			xmlChild->getAttr("hostile", m_hostileBaseIcon);

			xmlChild->getAttr("priority", m_iconPriority);
		}
		else if (!stricmp(xmlChild->getTag(), "CarryClassPrefs"))
		{
			if (xmlChild->getAttr("pickupType", &pCharTmp))
			{
				if (!stricmp(pCharTmp, "Proximity"))
				{
					m_carryPickupType = ePT_Proximity;
				}
				else if (!stricmp(pCharTmp, "Item"))
				{
					m_carryPickupType = ePT_Item;
				}
			}
		}
	}

	memset(m_carryEntities, 0, sizeof(SCarryEntity) * CARRY_OBJECTIVE_MAX_CARRY_ENTITIES);
	memset(m_targetEntities, 0, sizeof(STargetEntity) * CARRY_OBJECTIVE_MAX_TARGET_ENTITIES);
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::AddEntityId(int entityType, EntityId entityId, int index, bool isNewEntity)
{
	IEntity*  pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	CRY_ASSERT(pEntity);

	CryLog("@ [tlh] CGameRulesCarryObjective::AddEntityId(entityType=%d, entityId=[name=%s], index=%d, isNewEntity=%d)", entityType, (pEntity?pEntity->GetName():"?"), index, isNewEntity);

	CGameRules *pGameRules = g_pGame->GetGameRules();

	if (entityType == CARRY_OBJECTIVE_ENTITY_TYPE_CARRY)		// Carry entity
	{
		CRY_ASSERT(index < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES);
		CryLog("CGameRulesCarryObjective::AddEntity() received carry entity, eid=%i, index=%i", entityId, index);

		SCarryEntity *pCarryEntity = &m_carryEntities[index];
		if (pCarryEntity->m_isActive)
		{
			// We're overwriting another entity, disable the old one
			CleanUpCarryEntity(pCarryEntity, index);
		}
		if (gEnv->bServer)
		{
			memset(pCarryEntity, 0, sizeof(*pCarryEntity));	// Clients data will be set by net serialise, can't reset here because the serialise will probably arrive before this RMI
		}
		pCarryEntity->m_entityId = entityId;
		pCarryEntity->m_isActive = true;
		pCarryEntity->m_isItem = (g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(entityId) != NULL);

		pCarryEntity->m_entityTeamId = pGameRules->GetTeam(entityId);

		pCarryEntity->m_baseId = 0;  // will get lazily evaluated by calls to GetBaseEntityIdForCarryEnt()

		if (gEnv->bServer)
		{
			// force script "onsetteam" functions to be called
			int  tmpTeam = pCarryEntity->m_entityTeamId;
			g_pGame->GetGameRules()->SetTeam(0, pCarryEntity->m_entityId);
			g_pGame->GetGameRules()->SetTeam(tmpTeam, pCarryEntity->m_entityId);
		}
	}
	else if (entityType == CARRY_OBJECTIVE_ENTITY_TYPE_TARGET)		// Target entity
	{
		CRY_ASSERT(index < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES);
		CryLog("CGameRulesCarryObjective::AddEntity() received target entity, eid=%i, index=%i", entityId, index);

		STargetEntity *pTargetEntity = &m_targetEntities[index];
		if (pTargetEntity->m_isActive)
		{
			// We're overwriting another entity, disable the old one
			CleanUpTargetEntity(pTargetEntity);
		}
		if (gEnv->bServer)
		{
			memset(pTargetEntity, 0, sizeof(*pTargetEntity));	// Clients data will be set by net serialise, can't reset here because the serialise will probably arrive before this RMI
		}
		pTargetEntity->m_entityId = entityId;
		pTargetEntity->m_isActive = true;

		pTargetEntity->m_entityTeamId = pGameRules->GetTeam(entityId);
		if (m_invertTargets)
		{
			pTargetEntity->m_objectiveTeamId = 3 - pTargetEntity->m_entityTeamId;
		}
		else
		{
			pTargetEntity->m_objectiveTeamId = pTargetEntity->m_entityTeamId;
		}

		if (gEnv->bServer)
		{
			gEnv->pEntitySystem->AddEntityEventListener(entityId, ENTITY_EVENT_ENTERAREA, this);
			gEnv->pEntitySystem->AddEntityEventListener(entityId, ENTITY_EVENT_LEAVEAREA, this);
		}

		if (gEnv->bClient)
		{
			ClSetIcon(pTargetEntity);
		}

		if (gEnv->bServer)
		{
			// force script "onsetteam" functions to be called
			int  tmpTeam = pTargetEntity->m_entityTeamId;
			g_pGame->GetGameRules()->SetTeam(0, pTargetEntity->m_entityId);
			g_pGame->GetGameRules()->SetTeam(tmpTeam, pTargetEntity->m_entityId);
		}
	}
	if (gEnv->bServer)
	{
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), eEA_GameServerStatic);
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if ((g_pGame->GetIGameFramework()->GetClientActorId() == entityId) && newTeamId)
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
		{
			STargetEntity *pTargetEntity = &m_targetEntities[i];
			if (pTargetEntity->m_isActive)
			{
				ClSetIcon(pTargetEntity);
			}
		}
	}
	else
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ i)
		{
			if (m_carryEntities[i].m_entityId == entityId)
			{
				m_carryEntities[i].m_entityTeamId = newTeamId;
				return;
			}
		}
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
		{
			if (m_targetEntities[i].m_entityId == entityId)
			{
				m_targetEntities[i].m_entityTeamId = newTeamId;
				if (m_invertTargets)
				{
					m_targetEntities[i].m_objectiveTeamId = 3 - newTeamId;
				}
				else
				{
					m_targetEntities[i].m_objectiveTeamId = newTeamId;
				}
				if (gEnv->bClient)
				{
					ClSetIcon(&m_targetEntities[i]);
				}
				return;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnStartGame()
{
	for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
	{
		STargetEntity *pTargetEntity = &m_targetEntities[i];
		if (pTargetEntity->m_isActive)
		{
			ClSetIcon(pTargetEntity);
		}
	}
	m_entitiesCompleted[0] = 0;
	m_entitiesCompleted[1] = 0;
}

//------------------------------------------------------------------------
CGameRulesCarryObjective::SCarryEntity *CGameRulesCarryObjective::GetCarryEntity(EntityId actorId, int &resultIndex)
{
	SCarryEntity  *pCarryEntity = NULL;
	resultIndex = -1;
	EntityId  attachedEid = GetAttachedEntity(actorId, 0, "left_weapon");
	for (int i = 0; i < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ i)
	{
		SCarryEntity *pEnt = &m_carryEntities[i];
		if (pEnt->m_isActive)
		{
			bool isEnt = false;
			if (!pEnt->m_isItem)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pEnt->m_entityId);
				if (pEntity)
				{
					// Check if the entity entering the area has the objective entity attached to it
					IEntity *pParent = pEntity->GetParent();
					if (pParent && pParent->GetId() == actorId)
					{
						isEnt = true;
					}
					else if (pEnt->m_entityId == attachedEid)
					{
						CRY_ASSERT(attachedEid);
						//IEntity*  pActorEnt = gEnv->pEntitySystem->GetEntity(actorId);
						//CryLog("CGameRulesCarryObjective::GetCarryEntity: entity '%s' has carry objective '%s' attached", (pActorEnt?pActorEnt->GetName():"!"), pEntity->GetName());
						isEnt = true;
					}
				}
			}
			else
			{
				IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pEnt->m_entityId);
				if (pItem && pItem->GetOwnerId() == actorId)
				{
					isEnt = true;
				}
			}
			if (isEnt)
			{
				pCarryEntity = pEnt;
				resultIndex = i;
				break;
			}
		}
	}
	
	return pCarryEntity;
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnEntityEvent( IEntity *pEntity, SEntityEvent &event )
{
	EntityId insideId = (EntityId) event.nParam[0];

	// Determine which target entity we're dealing with
	STargetEntity *pTargetEntity = 0;
	for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
	{
		STargetEntity *pEnt = &m_targetEntities[i];
		if (pEnt->m_isActive && pEnt->m_entityId == pEntity->GetId())
		{
			pTargetEntity = pEnt;
			break;
		}
	}

	CRY_ASSERT(pTargetEntity);

	if (event.event == ENTITY_EVENT_ENTERAREA)
	{
		CActor *pActor = static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(insideId));
		if (pActor && pActor->IsPlayer() && pActor->GetHealth() > 0 && pActor->GetSpectatorMode() == CActor::eASM_None && g_pGame->GetGameRules()->GetTeam(insideId) == pTargetEntity->m_objectiveTeamId)
		{
			pTargetEntity->m_insideEntities.push_back(insideId);
		}
	}
	else if (event.event == ENTITY_EVENT_LEAVEAREA)
	{
		int numEntities = pTargetEntity->m_insideEntities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			if (pTargetEntity->m_insideEntities[i] == insideId)
			{
				pTargetEntity->m_insideEntities.removeAt(i);
				// If the player leaving is currently deploying an objective, stop them!
				for (int j = 0; j < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ j)
				{
					SCarryEntity *pCarryEntity = &m_carryEntities[j];
					if (pCarryEntity->m_isActive && pCarryEntity->m_deployerId == insideId)
					{
						SvSetDeployerId(j, 0);
						break;
					}
				}
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::Update( float frameTime )
{
	#if CRY_WATCH_ENABLED
	if (g_pGameCVars->g_CarryObjective_watchLvl > 0)
	{
		CryWatch("[CGameRulesCarryObjective::Update()]");
		CryWatch(" Carry ents:");
		for (int i=0; i<CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; i++)
		{
			SCarryEntity*  ce = &m_carryEntities[i];
			if (ce->m_entityId)
			{
				IEntity*  e = gEnv->pEntitySystem->GetEntity(ce->m_entityId);
				CryWatch("  %d: ent %d '%s' ent team %d active %d finished %d", i, ce->m_entityId, (e?e->GetName():"?"), ce->m_entityTeamId, ce->m_isActive, ce->m_isFinished);
			}
		}
		CryWatch(" Target ents:");
		for (int i=0; i<CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; i++)
		{
			STargetEntity*  te = &m_targetEntities[i];
			if (te->m_entityId)
			{
				IEntity*  e = gEnv->pEntitySystem->GetEntity(te->m_entityId);
				CryWatch("  %d: ent %d '%s' ent team %d obj team %d active %d", i, te->m_entityId, (e?e->GetName():"?"), te->m_entityTeamId, te->m_objectiveTeamId, te->m_isActive);
			}
		}
		EntityId  attachedEid = GetAttachedEntity(g_pGame->GetIGameFramework()->GetClientActorId(), 0, "left_weapon");
		IEntity*  pAttachedEnt = gEnv->pEntitySystem->GetEntity(attachedEid);
		CryWatch(" Local Attached ent: %d '%s'", attachedEid, (pAttachedEnt?pAttachedEnt->GetName():"?"));
	}
#endif

	if (gEnv->bServer)
	{
		for (int targetIdx = 0; targetIdx < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ targetIdx)
		{
			STargetEntity *pTargetEntity = &m_targetEntities[targetIdx];
			if (pTargetEntity->m_isActive)
			{
				int numInsideEntities = pTargetEntity->m_insideEntities.size();
				for (int insideIdx = 0; insideIdx < numInsideEntities; ++ insideIdx)
				{
					EntityId playerId = pTargetEntity->m_insideEntities[insideIdx];
					CActor *pActor = static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
					if (pActor && pActor->GetHealth() > 0 && pActor->GetSpectatorMode() == CActor::eASM_None)
					{
						// First make sure own carry objective is at base (if required)
						if (m_requireOwnAtBase)
						{
							int  plyrTeam = g_pGame->GetGameRules()->GetTeam(playerId);
							if (plyrTeam)
							{
								bool  atBase = true;
								for (int i=0; i<CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; i++)
								{
									SCarryEntity*  pCarryEntity = &m_carryEntities[i];
									if (pCarryEntity->m_isActive && (pCarryEntity->m_entityTeamId == plyrTeam))
									{
										if (IEntity* pEntity=gEnv->pEntitySystem->GetEntity(pCarryEntity->m_entityId))
										{
											CRY_TODO(15,3,2009,"Write a GetBaseStatusForCarryEnt() func for this instead, because this current method won't work if a spawn offset is used");
											if (IEntity* pBase=gEnv->pEntitySystem->GetEntity(GetBaseEntityIdForCarryEnt(pCarryEntity)))
											{
												if (!pEntity->GetWorldPos().IsEquivalent(pBase->GetWorldPos()))
												{
													atBase = false;
													break;
												}
											}
										}
									}
								}
								if (!atBase)
								{
									continue;
								}
							}
						}

						// We only care about this if it is carrying a carry objective
						int carryIndex = -1;
						SCarryEntity *pCarryEntity = GetCarryEntity(playerId, carryIndex);
						if (pCarryEntity)
						{
							if (pCarryEntity->m_deployerId == playerId)
							{
								pCarryEntity->m_deployingTime += frameTime;
								if (pCarryEntity->m_deployingTime >= m_deployTime)
								{
									CryLog("CGameRulesCarryObjective::Update(), Score!");
									// Points scored!
									if (m_scoringEnabled[pTargetEntity->m_objectiveTeamId - 1])
									{
										IGameRulesScoringModule *pScoringModule = g_pGame->GetGameRules()->GetScoringModule();
										if (pScoringModule)
										{
											pScoringModule->OnTeamScoringEvent(pTargetEntity->m_objectiveTeamId, EGRST_CarryObjectiveCompleted);
											pScoringModule->OnPlayerScoringEvent(playerId, EGRST_CarryObjectiveCompleted);
										}
									}

									/*
									// NOTE this isn't needed here providing the carryentity is dropped/detached upon completion of the objective...
									if (m_useSpawnPOIs)
									{
										if (IGameRulesSpawningModule* spawningModule=pGameRules->GetSpawningModule())
										{
											spawningModule->DisablePOI(pCarryEntity->m_entityId);
										}
									}
									*/

									if (gEnv->bClient)
									{
										ClEntityCompleted(pCarryEntity);
									}

									SvSetDeployerId(carryIndex, 0);
									++ m_entitiesCompleted[pTargetEntity->m_objectiveTeamId - 1];
									CleanUpCarryEntity(pCarryEntity, carryIndex);

									if (m_removeOnCompletion)
									{
										gEnv->pEntitySystem->RemoveEntity(pCarryEntity->m_entityId);
									}
								}
							}
							else
							{
								SvSetDeployerId(carryIndex, playerId);
							}
						}
					}
				}
			}
		}
	}
	if (gEnv->bClient)
	{
		if (m_useDeployTime && m_clientDeployingIndex != -1 && m_carryEntities[m_clientDeployingIndex].m_isActive)
		{
			m_clientDeployingTime += frameTime;
			CRY_TODO(1, 12, 2009, "[CG]: Localise and get own hud element");
			CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", "Deploying ...", NULL, 0.5f);

			CUISimpleBar * deployBar = CUISimpleBar::GetInstanceWithName("JavelinBar");
			if (deployBar)
			{
				float perc = m_clientDeployingTime / m_deployTime;
				deployBar->Set(clamp(perc, 0.f, 1.f));
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnEntityKilled( const HitInfo &hitInfo )
{
	// Only care if the entity is a player
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);
	if (pActor && pActor->IsPlayer())
	{
		int resultIndex = -1;
		SCarryEntity *pCarryEntity = GetCarryEntity(hitInfo.targetId, resultIndex);
		if (pCarryEntity)
		{
			SvSetDeployerId(resultIndex, 0);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::SvSetDeployerId( int index, EntityId newId )
{
	SCarryEntity *pCarryEntity = &m_carryEntities[index];
	if (gEnv->bClient)
	{
		ClDeployerIdChanged(index, newId);
	}
	pCarryEntity->m_deployerId = newId;
	pCarryEntity->m_deployingTime = 0.f;

	if (m_useDeployTime && g_pGame->GetIGameFramework()->GetNetContext())
	{
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), CARRY_OBJECTIVE_NET_ASPECT);
	}
}

//------------------------------------------------------------------------
bool CGameRulesCarryObjective::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == CARRY_OBJECTIVE_NET_ASPECT)
	{
		if (m_useDeployTime)
		{
			for (int i = 0; i < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ i)
			{
				EntityId deployerId = m_carryEntities[i].m_deployerId;
				ser.Value("deployerId", deployerId, 'eid');
				if (ser.IsReading())
				{
					ClDeployerIdChanged(i, deployerId);
				}
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
CGameRulesCarryObjective::CGameRulesCarryObjective()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterClientConnectionListener(this);
	pGameRules->RegisterTeamChangedListener(this);

	if (gEnv->bServer)
	{
		pGameRules->RegisterKillListener(this);
	}

	m_scoringEnabled[0] = false;
	m_scoringEnabled[1] = false;
	m_invertTargets = false;
	m_useDeployTime = false;
	m_carryPickupType = ePT_Item;
	m_deployTime = 0.f;
	m_clientDeployingIndex = -1;
	m_clientDeployingTime = 0.f;

	g_pGame->GetIGameFramework()->RegisterListener(this, "carryObjective", FRAMEWORKLISTENERPRIORITY_GAME);
}

//------------------------------------------------------------------------
CGameRulesCarryObjective::~CGameRulesCarryObjective()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterClientConnectionListener(this);
		pGameRules->UnRegisterTeamChangedListener(this);
		if (gEnv->bServer)
		{
			pGameRules->UnRegisterKillListener(this);
		}
	}

	g_pGame->GetIGameFramework()->UnregisterListener(this);
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::EnableCompletion( int teamId, bool enable )
{
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::EnableScoring( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_scoringEnabled[teamId - 1] = enable;
	}
}

//------------------------------------------------------------------------
bool CGameRulesCarryObjective::IsComplete( int teamId )
{
	// TODO: If we're removing entities on completion, the objective is complete when there are none left

	bool completed = false;
	if (m_removeOnCompletion && teamId && m_entitiesCompleted[teamId - 1])
	{
		completed = true;
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ i)
		{
			SCarryEntity *pCarryEntity = &m_carryEntities[i];
			if (pCarryEntity->m_isActive && pCarryEntity->m_entityTeamId == teamId)
			{
				completed = false;
				break;
			}
		}
	}

	return completed;
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnActionEvent( const SActionEvent& event )
{
	switch(event.m_event)
	{
	case eAE_resetBegin:
		ClearEntities(CARRY_OBJECTIVE_ENTITY_TYPE_CARRY);
		ClearEntities(CARRY_OBJECTIVE_ENTITY_TYPE_TARGET);
		m_entitiesCompleted[0] = 0;
		m_entitiesCompleted[1] = 0;
		break;
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::RemoveEntityId( int entityType, EntityId entityId )
{
	if (entityType == CARRY_OBJECTIVE_ENTITY_TYPE_CARRY)		// Carry entity
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ i)
		{
			SCarryEntity *pCarryEntity = &m_carryEntities[i];
			if (pCarryEntity->m_isActive && pCarryEntity->m_entityId == entityId)
			{
				CleanUpCarryEntity(pCarryEntity, i);
			}
		}
	}
	else if (entityType == CARRY_OBJECTIVE_ENTITY_TYPE_TARGET)		// Target entity
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
		{
			STargetEntity *pTargetEntity = &m_targetEntities[i];
			if (pTargetEntity->m_isActive && pTargetEntity->m_entityId == entityId)
			{
				CleanUpTargetEntity(pTargetEntity);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::ClearEntities( int entityType )
{
	if (entityType == CARRY_OBJECTIVE_ENTITY_TYPE_CARRY)		// Carry entity
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_CARRY_ENTITIES; ++ i)
		{
			SCarryEntity *pCarryEntity = &m_carryEntities[i];
			if (pCarryEntity->m_isActive)
			{
				CleanUpCarryEntity(pCarryEntity, i);
			}
		}
	}
	else if (entityType == CARRY_OBJECTIVE_ENTITY_TYPE_TARGET)	// Target entity
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
		{
			STargetEntity *pTargetEntity = &m_targetEntities[i];
			if (pTargetEntity->m_isActive)
			{
				CleanUpTargetEntity(pTargetEntity);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnOwnClientEnteredGame()
{
	for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
	{
		STargetEntity *pTargetEntity = &m_targetEntities[i];
		if (pTargetEntity->m_isActive)
		{
			ClSetIcon(pTargetEntity);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::CleanUpCarryEntity( SCarryEntity *pCarryEntity, int index )
{
	pCarryEntity->m_isActive = false;

	if (gEnv->bClient && m_clientDeployingIndex == index)
	{
		m_clientDeployingIndex = -1;
	}
}


//------------------------------------------------------------------------
void CGameRulesCarryObjective::CleanUpTargetEntity( STargetEntity *pTargetEntity )
{
	pTargetEntity->m_isActive = false;

	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pTargetEntity->m_entityId);
	if (pEntity)
	{
		if (gEnv->bServer)
		{
			gEnv->pEntitySystem->RemoveEntityEventListener(pTargetEntity->m_entityId, ENTITY_EVENT_ENTERAREA, this);
			gEnv->pEntitySystem->RemoveEntityEventListener(pTargetEntity->m_entityId, ENTITY_EVENT_LEAVEAREA, this);
		}
		if (gEnv->bClient)
		{
			SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
			newRemoveObjective.ReserveData(2);
			newRemoveObjective.AddData( static_cast<int>(pTargetEntity->m_entityId) ); /*(EntityId)*/
			newRemoveObjective.AddData( m_iconPriority );
			CHUD::CallEvent(newRemoveObjective);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::ClEntityCompleted( SCarryEntity *pCarryEntity )
{
	// TODO: Display message
}

//------------------------------------------------------------------------
bool CGameRulesCarryObjective::IsEntityFinished(int type, int index)
{
	return false;
}

//------------------------------------------------------------------------
bool CGameRulesCarryObjective::CanRemoveEntity(int type, int index)
{
	return true;
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::ClSetIcon( STargetEntity *pTargetEntity )
{
	int localTeamId = g_pGame->GetGameRules()->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	if (pTargetEntity->m_entityTeamId)
	{
		int iconToUse = -1;
		if ((localTeamId == pTargetEntity->m_entityTeamId) && (m_friendlyBaseIcon != int(EGRMO_Unknown)))
		{
			iconToUse = m_friendlyBaseIcon;
		}
		else if ((localTeamId != pTargetEntity->m_entityTeamId) && (m_hostileBaseIcon != int(EGRMO_Unknown)))
		{
			iconToUse = m_hostileBaseIcon;
		}

		if( iconToUse>=0 )
		{
			SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
			newMissionObjective.ReserveData(4);
			newMissionObjective.AddData( static_cast<int>(pTargetEntity->m_entityId) ); /*(EntityId)*/
			newMissionObjective.AddData( iconToUse ); /*(EGameRulesMissionObjectives)*/ 
			newMissionObjective.AddData( 0.0f );
			newMissionObjective.AddData( m_iconPriority ); /*(int)*/ 
			CHUD::CallEvent(newMissionObjective);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::ClDeployerIdChanged( int index, EntityId newId )
{
	EntityId oldId = m_carryEntities[index].m_deployerId;
	if (newId != oldId)
	{
		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		if (localActorId == oldId)
		{
			m_clientDeployingIndex = -1;
		}
		else if (localActorId == newId)
		{
			m_clientDeployingIndex = index;
			m_clientDeployingTime = 0.f;
		}
	}
	m_carryEntities[index].m_deployerId = newId;
}

//------------------------------------------------------------------------
EntityId CGameRulesCarryObjective::GetAttachedEntity(EntityId parentEid, int chrSlot, char attachmentInterfaceName[])
{
	EntityId  attachedEid = 0;
	if (IEntity* pParentEnt=gEnv->pEntitySystem->GetEntity(parentEid))
	{
		if (ICharacterInstance* pCharacter=pParentEnt->GetCharacter(chrSlot))  // for CTF, slot 0 is specified in its lua call to SetAttachmentObject() in CTFFlag::AttachTo()
		{
			IAttachmentManager*  pAttMgr = pCharacter->GetIAttachmentManager();
			CRY_ASSERT(pAttMgr);
			IAttachment*  pAtt = pAttMgr->GetInterfaceByName(attachmentInterfaceName);  // for CTF, "left_weapon" is specified in its lua call to SetAttachmentObject() in CTFFlag::AttachTo()
			CRY_ASSERT_TRACE(pAtt, ("Failed to get attachment interface with name '%s'", attachmentInterfaceName));  // not sure it's correct that this should ever be null?
			if (pAtt)
			{
				if (IAttachmentObject* pAO=pAtt->GetIAttachmentObject())
				{
					if (pAO->GetAttachmentType() == IAttachmentObject::eAttachment_Entity)
					{
						CEntityAttachment*  pEA = static_cast< CEntityAttachment* >( pAO );
						attachedEid = pEA->GetEntityId();
					}
				}
			}
		}
	}
	return attachedEid;
}

//------------------------------------------------------------------------
EntityId CGameRulesCarryObjective::GetBaseEntityIdForCarryEnt(SCarryEntity* pCarryEntity)
{
	return CGameRulesObjectiveHelper_Carry::S_GetBaseEntityIdForCarryEnt(pCarryEntity->m_entityId, &pCarryEntity->m_baseId);
}

//------------------------------------------------------------------------
void CGameRulesCarryObjective::OnHostMigration( bool becomeServer )
{
	if (becomeServer)
	{
		for (int i = 0; i < CARRY_OBJECTIVE_MAX_TARGET_ENTITIES; ++ i)
		{
			STargetEntity *pTargetEntity = &m_targetEntities[i];
			if (pTargetEntity->m_isActive)
			{
				gEnv->pEntitySystem->AddEntityEventListener(pTargetEntity->m_entityId, ENTITY_EVENT_ENTERAREA, this);
				gEnv->pEntitySystem->AddEntityEventListener(pTargetEntity->m_entityId, ENTITY_EVENT_LEAVEAREA, this);
			}
		}
	}
}
