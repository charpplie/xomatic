/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a kill objective (destroy an entity)

	-------------------------------------------------------------------------
	History:
	- 23:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesObjectiveHelper_Spawn.h"
#include "IXml.h"
#include "GameRules.h"
#include "Utility/CryWatch.h"
#include "Item.h"
#include "Player.h"

#include "HUD/HUD.h"

#define OBJECTIVEHELPER_SPAWN_REPEAT_DELAY	15000.f

#define OBJECTIVEHELPER_SPAWN_FLAGS_DROPPED		(1 << 0)

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::Init( XmlNodeRef xml )
{
	m_pPositionEntityClass = NULL;
	m_pSpawnEntityClass = NULL;
	m_friendlyIconDropped = int(EGRMO_Unknown);
	m_hostileIconDropped = int(EGRMO_Unknown);
	m_friendlyIconCarried = int(EGRMO_Unknown);
	m_hostileIconCarried = int(EGRMO_Unknown);
	m_friendlyIconAtBase = int(EGRMO_Unknown);
	m_hostileIconAtBase = int(EGRMO_Unknown);
	m_moduleRMIIndex = -1;
	m_invertTeams = false;
	m_resetOnDropped = true;
	m_resetOnRemoved = true;
	m_prefixSpawnAtName = false;
	m_spawnOffset.zero();
	m_spawnRotation.zero();
	m_carrierSpeedMult = 1.f;

	const char *pEntityName = 0;
	if (xml->getAttr("spawnAt", &pEntityName))
	{
		CryLog("CGameRulesObjectiveHelper_Spawn::Init, 'spawn at' entity type=%s", pEntityName);
		m_pPositionEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pEntityName);
	}
	CRY_ASSERT_MESSAGE(m_pPositionEntityClass, "Failed to find spawnAt entity class");

	pEntityName = 0;
	if (xml->getAttr("spawn", &pEntityName))
	{
		CryLog("CGameRulesObjectiveHelper_Spawn::Init, 'spawn' entity type=%s", pEntityName);
		m_pSpawnEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pEntityName);
	}
	CRY_ASSERT_MESSAGE(m_pSpawnEntityClass, "Failed to find spawn entity class");

	int  iVal = 0;

	if (xml->getAttr("invertTeams", iVal))
	{
		m_invertTeams = (iVal != 0);
	}

	if (xml->getAttr("prefixSpawnAtName", iVal))
	{
		m_prefixSpawnAtName = (iVal != 0);
	}

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "DroppedIcons"))
		{
			xmlChild->getAttr("friendly", m_friendlyIconDropped);
			xmlChild->getAttr("hostile", m_hostileIconDropped);
		}
		else if (!stricmp(xmlChild->getTag(), "CarriedIcons"))
		{
			xmlChild->getAttr("friendly", m_friendlyIconCarried);
			xmlChild->getAttr("hostile", m_hostileIconCarried);
		}
		else if (!stricmp(xmlChild->getTag(), "AtBaseIcons"))
		{
			xmlChild->getAttr("friendly", m_friendlyIconAtBase);
			xmlChild->getAttr("hostile", m_hostileIconAtBase);
		}
		else if (!stricmp(xmlChild->getTag(), "Strings"))
		{
			const char *pString = 0;
			if (xmlChild->getAttr("friendlyPickUp", &pString))
			{
				m_textMessagePickUpFriendly.Format("@%s", pString);
			}
			if (xmlChild->getAttr("hostilePickUp", &pString))
			{
				m_textMessagePickUpHostile.Format("@%s", pString);
			}
			if (xmlChild->getAttr("friendlyDropped", &pString))
			{
				m_textMessageDroppedFriendly.Format("@%s", pString);
			}
			if (xmlChild->getAttr("hostileDropped", &pString))
			{
				m_textMessageDroppedHostile.Format("@%s", pString);
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
		else if (!stricmp(xmlChild->getTag(), "Reset"))
		{
			int reset = 0;
			if (xmlChild->getAttr("onDropped", reset))
			{
				m_resetOnDropped = (reset != 0);
			}
			if (xmlChild->getAttr("onRemoved", reset))
			{
				m_resetOnRemoved = (reset != 0);
			}
		}
		else if (!stricmp(xmlChild->getTag(), "Carrier"))
		{
			xmlChild->getAttr("speedMultiplier", m_carrierSpeedMult);
		}
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	m_moduleRMIIndex = pGameRules->RegisterModuleRMIListener(this);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::Update( float frameTime )
{
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnStartGame()
{
	if (gEnv->bServer)
	{
		// Could be a restart, remove all entities previously spawned by this class
		int numExistingEntities = m_entities.size();
		for (int i = 0; i < numExistingEntities; ++ i)
		{
			SSpawnEntity *pSpawnEntity = &m_entities[i];
			for (int teamIdx = 0; teamIdx < SSpawnEntity::NUM_TEAMS; ++ teamIdx)
			{
				if (pSpawnEntity->m_spawnedEntityId[teamIdx])
				{
					// Need to remove the listener before the entity, otherwise we'll get a callback saying the entity has gone
					gEnv->pEntitySystem->RemoveEntityEventListener(pSpawnEntity->m_spawnedEntityId[teamIdx], ENTITY_EVENT_DONE, this);
					gEnv->pEntitySystem->RemoveEntity(pSpawnEntity->m_spawnedEntityId[teamIdx]);
				}
			}
		}
		m_entities.clear();
		if (gEnv->bClient)
		{
			ClClearEntities();
		}

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
					pSpawnEntity->m_spawnedEntityId[0] = 0;
					pSpawnEntity->m_spawnedEntityId[1] = 0;
					pSpawnEntity->m_carrierEntityId[0] = 0;
					pSpawnEntity->m_carrierEntityId[1] = 0;

					CryLog("CGameRulesObjectiveHelper_Spawn::LevelLoaded(), found 'spawn at' entity, idx=%i, id=%i, name=%s, team=%d", (m_entities.size() - 1), pEntity->GetId(), pEntity->GetName(), g_pGame->GetGameRules()->GetTeam(pEntity->GetId()));
				}
			}
		}
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterPickupListener(this);
	pGameRules->RegisterTeamChangedListener(this);
	pGameRules->RegisterClientConnectionListener(this);
	pGameRules->RegisterKillListener(this);
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Spawn::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return true;
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Spawn::CGameRulesObjectiveHelper_Spawn()
{
	m_teamEnabled[0] = false;
	m_teamEnabled[1] = false;
	g_pGame->GetIGameFramework()->RegisterListener(this, "objectiveHelper_Spawn", FRAMEWORKLISTENERPRIORITY_GAME);
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Spawn::~CGameRulesObjectiveHelper_Spawn()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterPickupListener(this);
		pGameRules->UnRegisterTeamChangedListener(this);
		pGameRules->UnRegisterClientConnectionListener(this);
		pGameRules->UnRegisterKillListener(this);
		pGameRules->UnRegisterModuleRMIListener(m_moduleRMIIndex);
	}
	g_pGame->GetIGameFramework()->UnregisterListener(this);

	if (gEnv->bServer)
	{
		TSpawnEntityVec::iterator it = m_entities.begin();
		for (; it != m_entities.end(); ++ it)
		{
			for (int teamIndex = 0; teamIndex < SSpawnEntity::NUM_TEAMS; ++ teamIndex)
			{
				if (it->m_spawnedEntityId[teamIndex])
				{
					gEnv->pEntitySystem->RemoveEntityEventListener(it->m_spawnedEntityId[teamIndex], ENTITY_EVENT_DONE, this);
				}
			}
		}
	}
	else
	{
		TClientEntityVec::iterator it = m_clientEntities.begin();
		for (; it != m_clientEntities.end(); ++ it)
		{
			gEnv->pEntitySystem->RemoveEntityEventListener(it->m_id, ENTITY_EVENT_DONE, this);
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Spawn::IsComplete( int teamId )
{
	return true;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::Enable( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_teamEnabled[teamId - 1] = enable;

		if (gEnv->bServer)
		{
			if (enable)
			{
				int numEntities = m_entities.size();
				for (int i = 0; i < numEntities; ++ i)
				{
					SSpawnEntity *pSpawnEntity = &m_entities[i];

					if (IsCorrectTeam(g_pGame->GetGameRules()->GetTeam(pSpawnEntity->m_positionEntityId), teamId))
					{
						SpawnEntity(pSpawnEntity);
					}
				}
			}
			else
			{
				int teamIndex = teamId - 1;

				int numEntities = m_entities.size();
				for (int i = 0; i < numEntities; ++ i)
				{
					SSpawnEntity *pSpawnEntity = &m_entities[i];
					if (pSpawnEntity->m_spawnedEntityId[teamIndex])
					{
						CItem::DeselectItem(pSpawnEntity->m_spawnedEntityId[teamIndex]);
						gEnv->pEntitySystem->RemoveEntity(pSpawnEntity->m_spawnedEntityId[teamIndex]);
						pSpawnEntity->m_spawnedEntityId[teamIndex] = 0;
						pSpawnEntity->m_carrierEntityId[teamIndex] = 0;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Spawn::SClientEntity * CGameRulesObjectiveHelper_Spawn::FindClientEntity( EntityId itemId )
{
	SClientEntity *pEntity = NULL;

	int numEntities = m_clientEntities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SClientEntity *pClientEntity = &m_clientEntities[i];
		if (pClientEntity->m_id == itemId)
		{
			pEntity = pClientEntity;
			break;
		}
	}

	return pEntity;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnItemPickedUp( EntityId itemId, EntityId actorId )
{
	OnItemOrEntityPickup(itemId, actorId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnPickupEntityAttached( EntityId entityId, EntityId actorId )
{
	OnItemOrEntityPickup(entityId, actorId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnItemOrEntityPickup( EntityId entityId, EntityId actorId )
{
	if (gEnv->bServer)
	{
		int numSpawnPoints = m_entities.size();
		for (int spawnIdx = 0; spawnIdx < numSpawnPoints; ++ spawnIdx)
		{
			SSpawnEntity *pDetails = &m_entities[spawnIdx];
			for (int teamIndex = 0; teamIndex < SSpawnEntity::NUM_TEAMS; ++ teamIndex)
			{
				if (pDetails->m_spawnedEntityId[teamIndex] == entityId)
				{
					pDetails->m_carrierEntityId[teamIndex] = actorId;
					pDetails->m_state[teamIndex] = ESES_Carried;
				}
			}
		}
	}
	if (gEnv->bClient)
	{
		SClientEntity *pClientEntity = FindClientEntity(entityId);
		if (pClientEntity)
		{
			EntityScriptCall("Close", pClientEntity->m_spawnerId);

			pClientEntity->m_carrierId = actorId;
			pClientEntity->m_state = ESES_Carried;

			ApplyCarrierSpeedMultiplier(actorId, m_carrierSpeedMult);

			SetIcon(pClientEntity, false);

			CGameRules *pGameRules = g_pGame->GetGameRules();

			int teamId = pGameRules->GetTeam(actorId);
			EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
			int localTeamId = pGameRules->GetTeam(localActorId);

			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(actorId);
			if (pEntity)
			{
				if ((localTeamId == teamId) && !m_textMessagePickUpFriendly.empty())
				{
					pGameRules->OnTextMessage(eTextMessageAnnouncement, m_textMessagePickUpFriendly.c_str(), pEntity->GetName());
				}
				else if ((localTeamId != teamId) && !m_textMessagePickUpHostile.empty())
				{
					pGameRules->OnTextMessage(eTextMessageAnnouncement, m_textMessagePickUpHostile.c_str(), pEntity->GetName());
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::SpawnEntity( SSpawnEntity *pDetails )
{
	int teamId = g_pGame->GetGameRules()->GetTeam(pDetails->m_positionEntityId);
	if (teamId == 1 || teamId == 2)
	{
		int teamIndex = teamId - 1;
		IEntity *pSpawnAtEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_positionEntityId);
		if (pSpawnAtEntity)
		{
			// Spawn entity

			CryFixedStringT<32>  sName;
			if (m_prefixSpawnAtName)
				sName.Format("%s__spawnedItem_%i", pSpawnAtEntity->GetName(), teamId);
			else
				sName.Format("spawnedItem_%i", teamId);

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
				int itemTeamId = GetItemTeamId(teamId);
				int itemTeamIndex = itemTeamId - 1;
				g_pGame->GetGameRules()->SetTeam(itemTeamId, pSpawnedEntity->GetId());
				
				if (pDetails->m_spawnedEntityId[itemTeamIndex])
				{
					// If we already have an entity for this slot, remove it
					gEnv->pEntitySystem->RemoveEntity(pDetails->m_spawnedEntityId[itemTeamIndex]);
				}

				pDetails->m_spawnedEntityId[itemTeamIndex] = pSpawnedEntity->GetId();
				pDetails->m_carrierEntityId[itemTeamIndex] = 0;
				pDetails->m_state[itemTeamIndex] = ESES_AtBase;
				CryLog("CGameRulesObjectiveHelper_Spawn::Update(), spawned entity '%s' id=%i for team %i", pSpawnedEntity->GetName(), pSpawnedEntity->GetId(), itemTeamId);

				gEnv->pEntitySystem->AddEntityEventListener(pSpawnedEntity->GetId(), ENTITY_EVENT_DONE, this);
				if (gEnv->bClient)
				{
					AddEntity(pSpawnedEntity->GetId(), pDetails->m_positionEntityId, pDetails->m_state[itemTeamIndex]);
				}
				CGameRules::SModuleRMITwoEntityParams params(m_moduleRMIIndex, pSpawnedEntity->GetId(), pDetails->m_positionEntityId, int(pDetails->m_state[itemTeamIndex]));
				g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToRemoteClients, params.m_entityId1);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if (gEnv->bServer)
	{
		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SSpawnEntity *pSpawnEntity = &m_entities[i];
			if (pSpawnEntity->m_positionEntityId == entityId)
			{
				// If the team that used to own the site has a spawned item at the base, remove it
				if (oldTeamId == 1 || oldTeamId == 2)
				{
					int oldTeamIndex = GetItemTeamId(oldTeamId) - 1;
					if (pSpawnEntity->m_spawnedEntityId[oldTeamIndex] && ! pSpawnEntity->m_carrierEntityId[oldTeamIndex])
					{
						gEnv->pEntitySystem->RemoveEntity(pSpawnEntity->m_spawnedEntityId[oldTeamIndex]);
						pSpawnEntity->m_spawnedEntityId[oldTeamIndex] = 0;
					}
				}

				// If the capturing team is enabled and currently has no spawned item here, create one
				if (newTeamId == 1 || newTeamId == 2)
				{
					int newTeamIndex = GetItemTeamId(newTeamId) - 1;
					if (m_teamEnabled[newTeamIndex])
					{
						if (!pSpawnEntity->m_spawnedEntityId[newTeamIndex])
						{
							SpawnEntity(pSpawnEntity);
						}
					}
				}
			}
		}
	}
	if (gEnv->bClient)
	{
		if (entityId == g_pGame->GetIGameFramework()->GetClientActorId())
		{
			// Local actor has changed team, need to reset all icons
			int numEntities = m_clientEntities.size();
			for (int i = 0; i < numEntities; ++ i)
			{
				SClientEntity *pClientEntity = &m_clientEntities[i];
				SetIcon(pClientEntity, false);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnEntityKilled( const HitInfo &hitInfo )
{
	if (!gEnv->bServer)
		return;

	CheckForItemCarrier(hitInfo.targetId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnClientDisconnect( int channelId, EntityId playerId )
{
	CheckForItemCarrier(playerId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::CheckForItemCarrier( EntityId playerId )
{
	CryLog("CGameRulesObjectiveHelper_Spawn::CheckForItemCarrier(), entity id=%i has died", playerId);
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SSpawnEntity *pSpawnEntity = &m_entities[i];
		for (int teamIndex = 0; teamIndex < SSpawnEntity::NUM_TEAMS; ++ teamIndex)
		{
			int teamId = teamIndex + 1;
			if ((pSpawnEntity->m_state[teamIndex] == ESES_Carried) && (pSpawnEntity->m_carrierEntityId[teamIndex] == playerId))
			{
				IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pSpawnEntity->m_spawnedEntityId[teamIndex]);
				if (pItem && g_pGame->GetIGameFramework()->GetNetContext())		// Don't bother dropping it if we're the one disconnecting
				{
					pItem->Drop();
				}
				else
				{
					SvItemDropped(pSpawnEntity, teamIndex);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::SvItemDropped(SSpawnEntity *pSpawnEntity, int teamIndex)
{
	pSpawnEntity->m_carrierEntityId[teamIndex] = 0;
	if (m_resetOnDropped)
	{
		gEnv->pEntitySystem->RemoveEntity(pSpawnEntity->m_spawnedEntityId[teamIndex]);
	}
	else
	{
		pSpawnEntity->m_state[teamIndex] = ESES_Dropped;
		if (gEnv->bClient)
		{
			ClItemDropped(pSpawnEntity->m_spawnedEntityId[teamIndex]);
		}
		CGameRules::SModuleRMIEntityParams params(m_moduleRMIIndex, pSpawnEntity->m_spawnedEntityId[teamIndex], OBJECTIVEHELPER_SPAWN_FLAGS_DROPPED);
		g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::ClModuleRMISingleEntity(), params, eRMI_ToRemoteClients);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::SetIcon(SClientEntity *pClientEntity, bool force)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
	int localTeamId = pGameRules->GetTeam(localActorId);
	int itemTeamId = pGameRules->GetTeam(pClientEntity->m_id);

	if (itemTeamId)
	{
		int requestedIcon = int(EGRMO_Unknown);
		EntityId requestedEntityId = 0;
		int requestedPriority = 0;

		if (localTeamId == itemTeamId)
		{
			if ((pClientEntity->m_state == ESES_Carried) && (m_friendlyIconCarried != int(EGRMO_Unknown)))
			{
				CRY_ASSERT(pClientEntity->m_carrierId);
				if (pClientEntity->m_carrierId != localActorId)
				{
					requestedIcon = m_friendlyIconCarried;
					requestedEntityId = pClientEntity->m_carrierId;
					requestedPriority = 0; // TODO: should come from xml
				}
			}
			else if ((pClientEntity->m_state == ESES_AtBase) && (m_friendlyIconAtBase != int(EGRMO_Unknown)))
			{
				requestedIcon = m_friendlyIconAtBase;
				requestedEntityId = pClientEntity->m_spawnerId;
				requestedPriority = 1; // TODO: should come from xml
			}
			else if ((pClientEntity->m_state == ESES_Dropped) && (m_friendlyIconDropped != int(EGRMO_Unknown)))
			{
				requestedIcon = m_friendlyIconDropped;
				requestedEntityId = pClientEntity->m_id;
				requestedPriority = 0; // TODO: should come from xml
			}
		}
		else
		{
			if ((pClientEntity->m_state == ESES_Carried) && (m_hostileIconCarried != int(EGRMO_Unknown)))
			{
				CRY_ASSERT(pClientEntity->m_carrierId);
				requestedIcon = m_hostileIconCarried;
				requestedEntityId = pClientEntity->m_carrierId;
				requestedPriority = 0; // TODO: should come from xml
			}
			else if ((pClientEntity->m_state == ESES_AtBase) && (m_hostileIconAtBase != int(EGRMO_Unknown)))
			{
				requestedIcon = m_hostileIconAtBase;
				requestedEntityId = pClientEntity->m_spawnerId;
				requestedPriority = 1; // TODO: should come from xml
			}
			else if ((pClientEntity->m_state == ESES_Dropped) && (m_hostileIconDropped != int(EGRMO_Unknown)))
			{
				requestedIcon = m_hostileIconDropped;
				requestedEntityId = pClientEntity->m_id;
				requestedPriority = 0; // TODO: should come from xml
			}
		}

		if ((requestedIcon != pClientEntity->m_currentIcon) || 
			(requestedPriority != pClientEntity->m_currentIconPriority) || 
			(requestedEntityId != pClientEntity->m_currentIconEntityId) ||
			force)
		{
			if (pClientEntity->m_currentIcon != int(EGRMO_Unknown))
			{
				SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
				newRemoveObjective.ReserveData(2);
				newRemoveObjective.AddData( static_cast<int>(pClientEntity->m_currentIconEntityId) ); /*(EntityId)*/
				newRemoveObjective.AddData( pClientEntity->m_currentIconPriority );
				CHUD::CallEvent(newRemoveObjective);
			}
			if (requestedIcon != int(EGRMO_Unknown))
			{
				SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
				newMissionObjective.ReserveData(4);
				newMissionObjective.AddData( static_cast<int>(requestedEntityId) ); /*(EntityId)*/
				newMissionObjective.AddData( requestedIcon ); /*(EGameRulesMissionObjectives)*/ 
				newMissionObjective.AddData( 0.0f ); 
				newMissionObjective.AddData( requestedPriority ); /*(int)*/ 
				CHUD::CallEvent(newMissionObjective);
			}
			pClientEntity->m_currentIcon = requestedIcon;
			pClientEntity->m_currentIconPriority = requestedPriority;
			pClientEntity->m_currentIconEntityId = requestedEntityId;
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::AddEntity( EntityId itemId, EntityId spawnerId, ESpawnEntityState state )
{
	CryLog("CGameRulesObjectiveHelper_Spawn::AddEntity(), adding item id=%i", itemId);

	CRY_ASSERT(itemId);
	if (!itemId)
	{
		return;
	}

	int teamId = g_pGame->GetGameRules()->GetTeam(itemId);
	EntityId carrierId = 0;

	// Item may already be picked up when we're told about it (if we're joining the game part way through)
	IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
	if (pItem)
	{
		carrierId = pItem->GetOwnerId();
		if (carrierId)
		{
			ApplyCarrierSpeedMultiplier(carrierId, m_carrierSpeedMult);
		}
	}

	m_clientEntities.push_back(SClientEntity(itemId, carrierId, teamId, spawnerId, state));

	SClientEntity *pClientEntity = &m_clientEntities[m_clientEntities.size() - 1];
	SetIcon(pClientEntity, false);

	if (!gEnv->bServer)
	{
		gEnv->pEntitySystem->AddEntityEventListener(itemId, ENTITY_EVENT_DONE, this);
	}

	if(gEnv->bClient)
	{
		EntityScriptCall("Open", spawnerId);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnClientEnteredGame( int channelId, bool isReset, EntityId playerId )
{
	// New client, tell them about the items
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SSpawnEntity *pSpawnEntity = &m_entities[i];
		for (int teamIndex = 0; teamIndex < SSpawnEntity::NUM_TEAMS; ++ teamIndex)
		{
			if (pSpawnEntity->m_spawnedEntityId[teamIndex])
			{
				CGameRules::SModuleRMITwoEntityParams params(m_moduleRMIIndex, pSpawnEntity->m_spawnedEntityId[teamIndex], pSpawnEntity->m_positionEntityId, pSpawnEntity->m_state[teamIndex]);
				g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, params.m_entityId1, channelId);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnActionEvent( const SActionEvent& event )
{
	switch(event.m_event)
	{
	case eAE_resetBegin:
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();
			pGameRules->UnRegisterPickupListener(this);
			pGameRules->UnRegisterTeamChangedListener(this);
			pGameRules->UnRegisterClientConnectionListener(this);
			pGameRules->UnRegisterKillListener(this);

			if (gEnv->bClient)
			{
				ClClearEntities();
			}
		}
		break;
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::ClClearEntities()
{
	if (m_clientEntities.size())
	{
		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		CryLog("CGameRulesObjectiveHelper_Spawn::OnActionEvent(): game is resetting, clear objective");

		TClientEntityVec::iterator it = m_clientEntities.begin();
		for (; it != m_clientEntities.end(); ++ it)
		{
			if (it->m_currentIcon != int(EGRMO_Unknown))
			{
				SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
				newRemoveObjective.ReserveData(2);
				newRemoveObjective.AddData( static_cast<int>(it->m_currentIconEntityId) ); /*(EntityId)*/
				newRemoveObjective.AddData( it->m_currentIconPriority );
				CHUD::CallEvent(newRemoveObjective);
			}
			gEnv->pEntitySystem->RemoveEntityEventListener(it->m_id, ENTITY_EVENT_DONE, this);
			if (it->m_state == ESES_Carried && it->m_carrierId)
			{
				ApplyCarrierSpeedMultiplier(it->m_carrierId, 1.f / m_carrierSpeedMult);
			}
		}
	}
	m_clientEntities.clear();
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnDoubleEntityRMI( CGameRules::SModuleRMITwoEntityParams params )
{
	AddEntity(params.m_entityId1, params.m_entityId2, ESpawnEntityState(params.m_data));
}

//------------------------------------------------------------------------
int CGameRulesObjectiveHelper_Spawn::GetItemTeamId( int spawnerTeamId )
{
	if (m_invertTeams)
	{
		return (3 - spawnerTeamId);
	}
	else
	{
		return (spawnerTeamId);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnEntityEvent(IEntity *pEntity, SEntityEvent &event)
{
	if (event.event == ENTITY_EVENT_DONE)
	{
		EntityId entId = pEntity->GetId();

		if (gEnv->bServer)
		{
			int numEntities = m_entities.size();
			for (int i = 0; i < numEntities; ++ i)
			{
				SSpawnEntity *pSpawnEntity = &m_entities[i];
				for (int teamIndex = 0; teamIndex < SSpawnEntity::NUM_TEAMS; ++ teamIndex)
				{
					int teamId = teamIndex + 1;
					if (pSpawnEntity->m_spawnedEntityId[teamIndex] == entId)
					{
						pSpawnEntity->m_spawnedEntityId[teamIndex] = 0;
						if (m_resetOnRemoved)
						{
							if (g_pGame->GetIGameFramework()->GetNetContext())  // this makes sure the game isn't shutting down - don't want to spawn anything if it is
							{
								CryLog("CGameRulesObjectiveHelper_Spawn::OnEntityKilled, carrier for team %i (spawner index %i) has been killed", teamId, i);
								// Weapon has been destroyed, if this helper is still enabled and the spawner is still owned by the correct team, spawn a new one
								if (m_teamEnabled[teamIndex] && IsCorrectTeam(g_pGame->GetGameRules()->GetTeam(pSpawnEntity->m_positionEntityId), teamId))
								{
									SpawnEntity(pSpawnEntity);
								}
							}
						}
					}
				}
			}
		}

		// Entity has been removed, find it
		TClientEntityVec::iterator it = m_clientEntities.begin();
		for (; it != m_clientEntities.end(); ++ it)
		{
			if (it->m_id == entId)
			{
				if ( it->m_currentIcon != int(EGRMO_Unknown) )
				{
					SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
					newRemoveObjective.ReserveData(2);
					newRemoveObjective.AddData( static_cast<int>(it->m_currentIconEntityId) ); /*(EntityId)*/
					newRemoveObjective.AddData( it->m_currentIconPriority );
					CHUD::CallEvent(newRemoveObjective);
				}
				if (it->m_state == ESES_Carried && it->m_carrierId)
				{
					ApplyCarrierSpeedMultiplier(it->m_carrierId, 1.0f / m_carrierSpeedMult);
				}
				CItem::DeselectItem(it->m_id);
				m_clientEntities.erase(it);
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Spawn::IsCorrectTeam(int teamId1, int teamId2)
{
	if (m_invertTeams)
	{
		return (teamId1 == (3 - teamId2));
	}
	else
	{
		return (teamId1 == teamId2);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::ClItemDropped( EntityId itemId )
{
	SClientEntity *pClientEntity = FindClientEntity(itemId);
	if (pClientEntity)
	{
		pClientEntity->m_state = ESES_Dropped;
		ApplyCarrierSpeedMultiplier(pClientEntity->m_carrierId, 1.0f / m_carrierSpeedMult);
		SetIcon(pClientEntity, false);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnSingleEntityRMI( CGameRules::SModuleRMIEntityParams params )
{
	if (params.m_data & OBJECTIVEHELPER_SPAWN_FLAGS_DROPPED)
	{
		ClItemDropped(params.m_entityId);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnOwnClientEnteredGame()
{
	int numEntities = m_clientEntities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SetIcon(&m_clientEntities[i], true);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::ApplyCarrierSpeedMultiplier( EntityId playerId, float mult )
{
	CPlayer *pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
	if (pPlayer)
	{
		pPlayer->ApplyGameRulesSpeedMultiplier(mult);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnItemDropped( EntityId itemId, EntityId actorId )
{
	OnItemOrEntityDrop(itemId, actorId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnPickupEntityDetached( EntityId entityId, EntityId actorId, bool isOnRemove )
{
	OnItemOrEntityDrop(entityId, actorId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnItemOrEntityDrop( EntityId entityId, EntityId actorId )
{
	if (gEnv->bServer)
	{
		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SSpawnEntity *pSpawnEntity = &m_entities[i];
			for (int teamIndex = 0; teamIndex < SSpawnEntity::NUM_TEAMS; ++ teamIndex)
			{
				int teamId = teamIndex + 1;
				if (pSpawnEntity->m_spawnedEntityId[teamIndex] == entityId)
				{
					SvItemDropped(pSpawnEntity, teamIndex);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::EntityScriptCall(const char* scriptFuncName, EntityId entityId)
{
	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(entityId);
	if(pTarget)
	{
		IScriptTable *pEntityScript = pTarget->GetScriptTable();
		HSCRIPTFUNCTION pFn = 0;
		if (pEntityScript && pEntityScript->GetValue(scriptFuncName, pFn))
		{
			Script::Call(gEnv->pScriptSystem, pFn, pEntityScript);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Spawn::OnHostMigration( bool becomeServer )
{
	if (becomeServer)
	{
		// Need to construct the server side data
		if (m_pPositionEntityClass)
		{
			// Find all the position entities
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
					pSpawnEntity->m_spawnedEntityId[0] = 0;
					pSpawnEntity->m_spawnedEntityId[1] = 0;
					pSpawnEntity->m_carrierEntityId[0] = 0;
					pSpawnEntity->m_carrierEntityId[1] = 0;

					CryLog("CGameRulesObjectiveHelper_Spawn::OnHostMigration(), found 'spawn at' entity, idx=%i, id=%i, name=%s, team=%d", (m_entities.size() - 1), pEntity->GetId(), pEntity->GetName(), g_pGame->GetGameRules()->GetTeam(pEntity->GetId()));
				}
			}

			// Now check our existing client data (some entities may already be spawned)
			const int numClientEntities = m_clientEntities.size();
			const int numSpawnerEntities = m_entities.size();
			for (int clientIdx = 0; clientIdx < numClientEntities; ++ clientIdx)
			{
				SClientEntity *pClientEntity = &m_clientEntities[clientIdx];
				// Need to find the corresponding server entry
				for (int spawnerIdx = 0; spawnerIdx < numSpawnerEntities; ++ spawnerIdx)
				{
					SSpawnEntity *pSpawnEntity = &m_entities[spawnerIdx];
					if (pSpawnEntity->m_positionEntityId == pClientEntity->m_spawnerId)
					{
						// Now populate the server entry
						CRY_ASSERT(pClientEntity->m_teamId);
						int teamIdx = pClientEntity->m_teamId - 1;
						pSpawnEntity->m_spawnedEntityId[teamIdx] = pClientEntity->m_id;
						pSpawnEntity->m_state[teamIdx] = pClientEntity->m_state;
						pSpawnEntity->m_carrierEntityId[teamIdx] = pClientEntity->m_carrierId;
#ifdef _DEBUG
						IEntity *pSEntity = gEnv->pEntitySystem->GetEntity(pSpawnEntity->m_positionEntityId);
						IEntity *pCEntity = gEnv->pEntitySystem->GetEntity(pClientEntity->m_id);

						CryLog("    Associated client entry %i (%s) with server entry %i (%s)", clientIdx, pCEntity ? pCEntity->GetName() : "<NULL>", spawnerIdx, pSEntity ? pSEntity->GetName() : "<NULL>");
#endif
					}
				}
			}
		}
	}
}
