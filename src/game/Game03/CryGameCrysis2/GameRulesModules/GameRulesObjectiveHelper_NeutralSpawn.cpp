/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Helper objective, spawns an entity at the position of another entity
		i.e. Spawns a gun at a base

	-------------------------------------------------------------------------
	History:
	- 27:11:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesObjectiveHelper_NeutralSpawn.h"
#include "IXml.h"
#include "GameRules.h"
#include "Utility/CryWatch.h"
#include "Item.h"
#include "Player.h"

#include "HUD/HUD.h"

#define GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_IN_BASE			(1 << 0)
#define GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_NOT_IN_BASE	(1 << 1)

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_NeutralSpawn::CGameRulesObjectiveHelper_NeutralSpawn()
{
	m_teamEnabled[0] = false;
	m_teamEnabled[1] = false;
	m_resetOnRemoved = false;
	m_pPositionEntityClass = NULL;
	m_pSpawnEntityClass = NULL;
	m_totalEntitiesSpawned = 0;
	m_moduleRMIIndex = -1;
	m_iconPriority = 0;
	m_baseIcon = int(EGRMO_Unknown);
	m_resetOnRemovedTime = 0.f;
	m_spawnOffset.zero();
	m_spawnRotation.zero();
	m_maxAtOnce = 0;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::Init( XmlNodeRef xml )
{
	const char *pEntityName = 0;
	if (xml->getAttr("spawnAt", &pEntityName))
	{
		CryLog("CGameRulesObjectiveHelper_NeutralSpawn::Init, 'spawn at' entity type=%s", pEntityName);
		m_pPositionEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pEntityName);
	}
	CRY_ASSERT_MESSAGE(m_pPositionEntityClass, "Failed to find spawnAt entity class");

	if (xml->getAttr("spawn", &pEntityName))
	{
		CryLog("CGameRulesObjectiveHelper_NeutralSpawn::Init, 'spawn' entity type=%s", pEntityName);
		m_pSpawnEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pEntityName);
	}
	CRY_ASSERT_MESSAGE(m_pSpawnEntityClass, "Failed to find spawn entity class");

	xml->getAttr("maxAtOnce", m_maxAtOnce);

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);

		if (!stricmp(xmlChild->getTag(), "Reset"))
		{
			int iValue = 0;
			if (xmlChild->getAttr("onRemoved", iValue))
			{
				m_resetOnRemoved = (iValue != 0);
				xmlChild->getAttr("onRemovedTime", m_resetOnRemovedTime);
			}
		}
		else if (!stricmp(xmlChild->getTag(), "Offset"))
		{
			xmlChild->getAttr("x", m_spawnOffset.x);
			xmlChild->getAttr("y", m_spawnOffset.y);
			xmlChild->getAttr("z", m_spawnOffset.z);
		}
		else if (!stricmp(xmlChild->getTag(), "Rotation"))
		{
			xmlChild->getAttr("x", m_spawnRotation.x);
			xmlChild->getAttr("y", m_spawnRotation.y);
			xmlChild->getAttr("z", m_spawnRotation.z);
		}
		else if (!stricmp(xmlChild->getTag(), "Icons"))
		{
			xmlChild->getAttr("base", m_baseIcon);
			xmlChild->getAttr("priority", m_iconPriority);
		}
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	m_moduleRMIIndex = pGameRules->RegisterModuleRMIListener(this);
	pGameRules->RegisterClientConnectionListener(this);
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_NeutralSpawn::~CGameRulesObjectiveHelper_NeutralSpawn()
{
	CleanupEntities();
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterModuleRMIListener(m_moduleRMIIndex);
		pGameRules->UnRegisterClientConnectionListener(this);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::OnStartGame()
{
	if (gEnv->bServer)
	{
		// Could be a restart, remove all entities previously spawned by this class
		CleanupEntities();
		m_entities.clear();
		m_entities.reserve(8);

		if (m_pPositionEntityClass)
		{
			IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
			IEntity *pEntity = 0;

			pIt->MoveFirst();

			while(pEntity = pIt->Next())
			{
				if (pEntity->GetClass() == m_pPositionEntityClass)
				{
					m_entities.push_back(SSpawnEntity());

					SSpawnEntity *pSpawnEntity = &m_entities[m_entities.size() - 1];
					pSpawnEntity->m_positionEntityId = pEntity->GetId();
					pSpawnEntity->m_spawnedEntityId = 0;
					pSpawnEntity->m_timeToSpawn = 0.f;
					pSpawnEntity->m_state = eSES_Waiting;

					CryLog("CGameRulesObjectiveHelper_Spawn::LevelLoaded(), found 'spawn at' entity, idx=%i, id=%i, name=%s", (m_entities.size() - 1), pEntity->GetId(), pEntity->GetName());
				}
			}
			m_availableSpawnPoints.reserve(m_entities.size());
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::Update( float frameTime )
{
	if (IsEnabled())
	{
		int numSpawnedEntities = 0;
		m_availableSpawnPoints.clear();

		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SSpawnEntity *pSpawnEntity = &m_entities[i];
			if (pSpawnEntity->m_state == eSES_Spawned)
			{
				++ numSpawnedEntities;
				if (pSpawnEntity->m_inBase)
				{
					IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pSpawnEntity->m_spawnedEntityId);
					if (pEntity)
					{
						if (!pEntity->GetWorldPos().IsEquivalent(pSpawnEntity->m_startingPosition))
						{
							pSpawnEntity->m_inBase = false;
							UpdateIcon(pSpawnEntity);
						}
					}
				}
			}
			else if (pSpawnEntity->m_state == eSES_Spawning)
			{
				++ numSpawnedEntities;
				pSpawnEntity->m_timeToSpawn -= frameTime;
				if (pSpawnEntity->m_timeToSpawn < 0.f)
				{
					SpawnEntity(pSpawnEntity);
				}
			}
			else	// (pSpawnEntity->m_state == eSES_Waiting)
			{
				m_availableSpawnPoints.push_back(i);
			}
		}

		if (gEnv->bServer)
		{
			int numAvailable = m_availableSpawnPoints.size();
			if (m_maxAtOnce)
			{
				int numAllowed = m_maxAtOnce - numSpawnedEntities;
				while (numAvailable > numAllowed)
				{
					int index = cry_rand() % numAvailable;
					TIntVec::iterator it = m_availableSpawnPoints.begin() + index;
					m_availableSpawnPoints.erase(it);
					-- numAvailable;
				}
			}

			for (int i = 0; i < numAvailable; ++ i)
			{
				int index = m_availableSpawnPoints[i];
				SSpawnEntity *pSpawnEntity = &m_entities[index];
				pSpawnEntity->m_state = eSES_Spawning;
				pSpawnEntity->m_timeToSpawn = m_resetOnRemovedTime;
			}
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_NeutralSpawn::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return true;
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_NeutralSpawn::IsComplete( int teamId )
{
	return true;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::Enable( int teamId, bool enable )
{
	bool enabledBefore = IsEnabled();

	CRY_ASSERT(teamId == 1 || teamId == 2);
	m_teamEnabled[teamId] = enable;

	bool enabledAfter = IsEnabled();

	if (gEnv->bServer)
	{
		if (enabledBefore && !enabledAfter)
		{
			CleanupEntities();
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::SpawnEntity( SSpawnEntity *pSpawnEntity )
{
	CRY_ASSERT(gEnv->bServer);
	IEntity *pSpawnAtEntity = gEnv->pEntitySystem->GetEntity(pSpawnEntity->m_positionEntityId);
	if (pSpawnAtEntity)
	{
		++ m_totalEntitiesSpawned;

		// Spawn entity
		CryFixedStringT<16> sName;
		sName.Format("neutralSpawnItem_%i", m_totalEntitiesSpawned);

		SEntitySpawnParams params;
		params.pClass = m_pSpawnEntityClass;
		params.sName = sName.c_str();
		params.vPosition = pSpawnAtEntity->GetWorldPos();
		params.vPosition.x += m_spawnOffset.x;
		params.vPosition.y += m_spawnOffset.y;
		params.vPosition.z += m_spawnOffset.z;
		Ang3 rotation = pSpawnAtEntity->GetWorldAngles();
		rotation.x += m_spawnRotation.x;
		rotation.y += m_spawnRotation.y;
		rotation.z += m_spawnRotation.z;
		params.qRotation = Quat(rotation);
		params.nFlags = ENTITY_FLAG_NEVER_NETWORK_STATIC;

		IEntity *pSpawnedEntity = gEnv->pEntitySystem->SpawnEntity(params, true);
		if (pSpawnedEntity)
		{
			pSpawnEntity->m_spawnedEntityId = pSpawnedEntity->GetId();
			pSpawnEntity->m_inBase = true;
			pSpawnEntity->m_startingPosition = pSpawnedEntity->GetWorldPos();
			pSpawnEntity->m_state = eSES_Spawned;

			CryLog("CGameRulesObjectiveHelper_NeutralSpawn::Update(), spawned entity '%s' id=%i", pSpawnedEntity->GetName(), pSpawnedEntity->GetId());

			gEnv->pEntitySystem->AddEntityEventListener(pSpawnedEntity->GetId(), ENTITY_EVENT_DONE, this);

			CGameRules::SModuleRMITwoEntityParams params;
			params.m_listenerIndex = m_moduleRMIIndex;
			params.m_entityId1 = pSpawnedEntity->GetId();
			params.m_entityId2 = pSpawnAtEntity->GetId();
			params.m_data = GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_IN_BASE;

			g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToRemoteClients, pSpawnedEntity->GetId());
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::OnEntityEvent( IEntity *pEntity,SEntityEvent &event )
{
	if (event.event == ENTITY_EVENT_DONE)
	{
		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SSpawnEntity *pSpawnEntity = &m_entities[i];
			if (pSpawnEntity->m_spawnedEntityId == pEntity->GetId())
			{
				if (pSpawnEntity->m_inBase && gEnv->bClient)
				{
					pSpawnEntity->m_inBase = false;
					UpdateIcon(pSpawnEntity);
				}

				if (gEnv->bServer)
				{
					pSpawnEntity->m_spawnedEntityId = 0;
					if (m_resetOnRemoved)
					{
						pSpawnEntity->m_state = eSES_Waiting;
					}
				}
				else
				{
					TSpawnEntityVec::iterator it = (m_entities.begin() + i);
					m_entities.erase(it);
				}
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::CleanupEntities()
{
	int numExistingEntities = m_entities.size();
	for (int i = 0; i < numExistingEntities; ++ i)
	{
		SSpawnEntity *pSpawnEntity = &m_entities[i];
		if (pSpawnEntity->m_spawnedEntityId)
		{
			gEnv->pEntitySystem->RemoveEntityEventListener(pSpawnEntity->m_spawnedEntityId, ENTITY_EVENT_DONE, this);
			if (gEnv->bServer)
			{
				gEnv->pEntitySystem->RemoveEntity(pSpawnEntity->m_spawnedEntityId);
				pSpawnEntity->m_spawnedEntityId = 0;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::OnDoubleEntityRMI( CGameRules::SModuleRMITwoEntityParams params )
{
	int numEntities = m_entities.size();
	m_entities.resize(numEntities + 1);
	SSpawnEntity *pSpawnEntity = &m_entities[numEntities];
	pSpawnEntity->m_spawnedEntityId = params.m_entityId1;
	pSpawnEntity->m_positionEntityId = params.m_entityId2;
	pSpawnEntity->m_inBase = ((params.m_data & GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_IN_BASE) != 0);
	pSpawnEntity->m_timeToSpawn = 0.f;
	pSpawnEntity->m_state = eSES_Spawned;

	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pSpawnEntity->m_spawnedEntityId);
	if (pSpawnEntity->m_inBase && pEntity)
	{
		pSpawnEntity->m_startingPosition = pEntity->GetWorldPos();
		UpdateIcon(pSpawnEntity);
	}
	else
	{
		pSpawnEntity->m_startingPosition.zero();
	}
	gEnv->pEntitySystem->AddEntityEventListener(pSpawnEntity->m_spawnedEntityId, ENTITY_EVENT_DONE, this);
	CryLog("CGameRulesObjectiveHelper_NeutralSpawn::OnDoubleEntityRMI(), spawned entity '%s' id=%i, atBase=%i", (pEntity ? pEntity->GetName() : "<NULL>"), pSpawnEntity->m_spawnedEntityId, (pSpawnEntity->m_inBase ? 1 : 0));
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::OnClientEnteredGame( int channelId, bool isReset, EntityId playerId )
{
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SSpawnEntity *pSpawnEntity = &m_entities[i];
		if (pSpawnEntity->m_spawnedEntityId)
		{
			CGameRules::SModuleRMITwoEntityParams params;
			params.m_listenerIndex = m_moduleRMIIndex;
			params.m_entityId1 = pSpawnEntity->m_spawnedEntityId;
			params.m_entityId2 = pSpawnEntity->m_positionEntityId;
			params.m_data = pSpawnEntity->m_inBase ? GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_IN_BASE : GAMERULES_HELPER_NEUTRAL_SPAWN_FLAGS_NOT_IN_BASE;

			g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, pSpawnEntity->m_spawnedEntityId, channelId);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::OnOwnClientEnteredGame()
{
	UpdateAllIcons();
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::UpdateAllIcons()
{
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SSpawnEntity *pSpawnEntity = &m_entities[i];

		UpdateIcon(pSpawnEntity);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::UpdateIcon( SSpawnEntity *pSpawnEntity )
{
	if (m_baseIcon != int(EGRMO_Unknown))
	{
		if (pSpawnEntity->m_inBase)
		{
			SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
			newMissionObjective.ReserveData(4);
			newMissionObjective.AddData( static_cast<int>(pSpawnEntity->m_positionEntityId) ); /*(EntityId)*/
			newMissionObjective.AddData( m_baseIcon ); /*(EGameRulesMissionObjectives)*/ 
			newMissionObjective.AddData( 0.0f ); /*(float)*/ 
			newMissionObjective.AddData( m_iconPriority ); /*(int)*/ 
			CHUD::CallEvent(newMissionObjective);
		}
		else
		{
			SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
			newRemoveObjective.ReserveData(1);
			newRemoveObjective.AddData( static_cast<int>(pSpawnEntity->m_positionEntityId) ); /*(EntityId)*/
			newRemoveObjective.AddData( m_iconPriority );
			CHUD::CallEvent(newRemoveObjective);
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_NeutralSpawn::IsEnabled()
{
	return (m_teamEnabled[0] || m_teamEnabled[1]);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_NeutralSpawn::OnHostMigration( bool becomeServer )
{
	if (becomeServer && m_pPositionEntityClass)
	{
		// Need to setup entities (server maintains list of all spawn points, clients only know about the ones that have associated spawned entities)
		IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
		IEntity *pEntity = 0;

		pIt->MoveFirst();

		while(pEntity = pIt->Next())
		{
			if (pEntity->GetClass() == m_pPositionEntityClass)
			{
				SSpawnEntity *pSpawnEntity = NULL;

				int numExistingEntities = m_entities.size();
				for (int i = 0; i < numExistingEntities; ++ i)
				{
					if (m_entities[i].m_positionEntityId == pEntity->GetId())
					{
						pSpawnEntity = &m_entities[i];
						break;
					}
				}
				if (!pSpawnEntity)
				{
					m_entities.push_back(SSpawnEntity());
					pSpawnEntity = &m_entities[m_entities.size() - 1];
					pSpawnEntity->m_positionEntityId = pEntity->GetId();
					pSpawnEntity->m_spawnedEntityId = 0;
					pSpawnEntity->m_timeToSpawn = 0.f;
					pSpawnEntity->m_state = eSES_Waiting;
				}

				CryLog("CGameRulesObjectiveHelper_Spawn::OnHostMigration(), found 'spawn at' entity, idx=%i, id=%i, name=%s", (m_entities.size() - 1), pEntity->GetId(), pEntity->GetName());
			}
		}
		m_availableSpawnPoints.reserve(m_entities.size());
	}
}
