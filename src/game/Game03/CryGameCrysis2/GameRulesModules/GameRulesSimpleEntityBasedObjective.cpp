/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Class for managing a series of SimpleEntityObjectives based on xml
		parameterisation

	-------------------------------------------------------------------------
	History:
	- 20:10:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesSimpleEntityBasedObjective.h"
#include "IXml.h"
#include "GameRules.h"
#include "Utility/CryWatch.h"

#include "GameRulesModules/GameRulesCaptureObjective.h"
#include "GameRulesModules/GameRulesCombiCaptureObjective.h"
#include "GameRulesModules/GameRulesKillObjective.h"
#include "GameRulesModules/GameRulesCarryObjective.h"
#include "GameRulesModules/GameRulesLuaEntityObjective.h"
#include "GameRulesModules/GameRulesKingOfTheHillObjective.h"

#define SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_REMOVE	0x40
#define SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_NEW		0x20

#define SIMPLE_ENTITY_BASED_OBJECTIVE_CONNECTION_GRACE_TIME		2.f

//------------------------------------------------------------------------
CGameRulesSimpleEntityBasedObjective::CGameRulesSimpleEntityBasedObjective()
{
	m_pObjective = NULL;
	m_moduleRMIIndex = 0;
	g_pGame->GetIGameFramework()->RegisterListener(this, "simpleentitybasedobjective", FRAMEWORKLISTENERPRIORITY_GAME);
}

//------------------------------------------------------------------------
CGameRulesSimpleEntityBasedObjective::~CGameRulesSimpleEntityBasedObjective()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterModuleRMIListener(m_moduleRMIIndex);
		pGameRules->UnRegisterClientConnectionListener(this);
	}
	gEnv->pEntitySystem->RemoveSink(this);
	SAFE_DELETE(m_pObjective);
	g_pGame->GetIGameFramework()->UnregisterListener(this);
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::Init(XmlNodeRef xml)
{
	int numChildren = xml->getChildCount();
	for (int childIndex = 0; childIndex < numChildren; ++ childIndex)
	{
		XmlNodeRef xmlChild = xml->getChild(childIndex);

		const char *pChildTag = xmlChild->getTag();
		if (!stricmp(pChildTag, "Entity"))
		{
			SEntityDetails entityDetails;

			const char *pEntityClass = 0;
			const char *pSelectType = 0;
			if (xmlChild->getAttr("class", &pEntityClass))
			{
				entityDetails.m_pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pEntityClass);
				CryLog("CGameRulesSimpleEntityBasedObjective::Init, Entity class='%s'", pEntityClass);
			}
			else
			{
				CryLog("CGameRulesSimpleEntityBasedObjective::Init, expected 'class' attribute within 'Entity' node");
				CRY_ASSERT_MESSAGE(false, "We require 'class' attribute within 'Entity' node");
			}

			if (xmlChild->getAttr("select", &pSelectType))
			{
				if (!stricmp(pSelectType, "All"))
				{
					entityDetails.m_entitySelectType = EEST_All;
					CryLog("CGameRulesSimpleEntityBasedObjective::Init, Selection type='All'");
				}
				else if (!stricmp(pSelectType, "Random"))
				{
					entityDetails.m_entitySelectType = EEST_Random;

					if (xmlChild->getAttr("time", entityDetails.m_randomChangeTimeLength))
					{
						entityDetails.m_useRandomChangeTimer = true;
						entityDetails.m_timeToRandomChange = entityDetails.m_randomChangeTimeLength;

						xmlChild->getAttr("timeBetweenLocations", entityDetails.m_timeBetweenSites);
					}

					entityDetails.m_entitySelectCount = 0;

					int entCount;
					int numEntityChildren = xmlChild->getChildCount();
					for (int entityChildIndex = 0; entityChildIndex < numEntityChildren; ++ entityChildIndex)
					{
						XmlNodeRef xmlEntityChild = xmlChild->getChild(entityChildIndex);
						const char *pEntityChildTag = xmlEntityChild->getTag();
						if (!stricmp(pEntityChildTag, "NthWave"))
						{
							if (xmlEntityChild->getAttr("count", entCount))
							{
								CryLog("CGameRulesSimpleEntityBasedObjective::Init NthWave %d has count %d", entityDetails.m_entitySelectCount, entCount);
								entityDetails.m_entitySelectNthWaveCount[entityDetails.m_entitySelectCount] = entCount;
								entityDetails.m_entitySelectCount++;
							}
						}
					}

					CryLog("CGameRulesSimpleEntityBasedObjective::Init, Selection type='Random', count='%i'", entityDetails.m_entitySelectCount);
				}
				else
				{
					CryLog("CGameRulesSimpleEntityBasedObjective::Init, unknown 'select' attribute within 'Entity' node");
					CRY_ASSERT_MESSAGE(false, "Unknown entity select type");
				}
			}

			int scan = 0;
			if (xmlChild->getAttr("scan", scan))
			{
				entityDetails.m_scan = (scan != 0);
			}

			const char *pUpdateType = 0;
			if (xmlChild->getAttr("update", &pUpdateType))
			{
				if (!stricmp(pUpdateType, "all"))
				{
					entityDetails.m_updateType = SEntityDetails::eUT_All;
				}
				else if (!stricmp(pUpdateType, "active"))
				{
					entityDetails.m_updateType = SEntityDetails::eUT_Active;
				}
			}

			// Only add the entity if it is a new class
			bool addEntity = true;
			int numEntityTypes = m_entityDetails.size();
			for (int i = 0; i < numEntityTypes; ++ i)
			{
				if (m_entityDetails[i].m_pEntityClass == entityDetails.m_pEntityClass)
				{
					CRY_ASSERT_MESSAGE(false, "Duplicate entity class detected!");
					CryLog("CGameRulesSimpleEntityBasedObjective::Init, ERROR: duplicate entity class detected, entity definitions must be unique");
					addEntity = false;
				}
			}

			if (addEntity)
			{
				m_entityDetails.push_back(entityDetails);
			}
		}
		else if (!stricmp(pChildTag, "Implementation"))
		{
			const char *pImplementationType = 0;
			if (xmlChild->getAttr("type", &pImplementationType))
			{
				if (!stricmp(pImplementationType, "Capture"))
				{
					m_pObjective = new CGameRulesCaptureObjective();
				}
				else if (!stricmp(pImplementationType, "CombiCapture"))
				{
					m_pObjective = new CGameRulesCombiCaptureObjective();
				}
				else if (!stricmp(pImplementationType, "Kill"))
				{
					m_pObjective = new CGameRulesKillObjective();
				}
				else if (!stricmp(pImplementationType, "Carry"))
				{
					m_pObjective = new CGameRulesCarryObjective();
				}
				else if (!stricmp(pImplementationType, "Lua"))
				{
					m_pObjective = new CGameRulesLuaEntityObjective();
				}
				else if (!stricmp(pImplementationType, "KingOfTheHill"))
				{
					m_pObjective = new CGameRulesKingOfTheHillObjective();
				}
				else
				{
					CryLog("CGameRulesSimpleEntityBasedObjective::Init, unknown implementation type given ('%s')", pImplementationType);
					CRY_ASSERT_MESSAGE(false, "Unknown implementation type given");
				}
				if (m_pObjective)
				{
					m_pObjective->Init(xmlChild);
				}
			}
			else
			{
				CryLog("CGameRulesSimpleEntityBasedObjective::Init, implementation type not given");
				CRY_ASSERT_MESSAGE(false, "Implementation type not given");
			}
		}
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	m_moduleRMIIndex = pGameRules->RegisterModuleRMIListener(this);
	pGameRules->RegisterClientConnectionListener(this);

	gEnv->pEntitySystem->AddSink(this);

	CRY_ASSERT_MESSAGE(m_pObjective, "Sub-objective not created, this will crash!");
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::Update(float frameTime)
{
#if CRY_WATCH_ENABLED
	if (g_pGameCVars->g_SimpleEntityBasedObjective_watchLvl > 0)
	{
		CryWatch("[CGameRulesSimpleEntityBasedObjective::Update()]");
		CryWatch(" Carry entity details:");
		SEntityDetails*  pEntityDetails = &m_entityDetails[0];
		int  numSlots = pEntityDetails->m_allEntities.size();
		for (int i=0; i<numSlots; i++)
		{
			IEntity*  e = gEnv->pEntitySystem->GetEntity(pEntityDetails->m_allEntities[i]);
			CryWatch("  %d: ent %d '%s'", i, pEntityDetails->m_allEntities[i], (e?e->GetName():"?"));
		}
	}
#endif

	int numEntityTypes = m_entityDetails.size();
	if (gEnv->bServer)
	{
		for (int entityType = 0; entityType < numEntityTypes; ++ entityType)
		{
			SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

			int remainingEntities = 0;

			int numEntities = pEntityDetails->m_currentEntities.size();
			for (int i = 0; i < numEntities; ++ i)
			{
				if (pEntityDetails->m_currentEntities[i])
				{
					if (m_pObjective->IsEntityFinished(entityType, i))
					{
						SvRemoveEntity(entityType, i, true);
					}
					else
					{
						++ remainingEntities;
					}
				}
			}

			if (!remainingEntities)
			{
				// No remaining entities, jump straight to 'between sites' state
				if (pEntityDetails->m_timeToRandomChange > pEntityDetails->m_timeBetweenSites)
				{
					pEntityDetails->m_timeToRandomChange = pEntityDetails->m_timeBetweenSites;
				}
			}

			bool finalCountZero = false;
			if ((pEntityDetails->m_entitySelectCount > 0 ) && (pEntityDetails->m_nthSpawn >= pEntityDetails->m_entitySelectCount))
			{
				finalCountZero = (pEntityDetails->m_entitySelectNthWaveCount[pEntityDetails->m_entitySelectCount-1] <= 0);
			}

			if (pEntityDetails->m_useRandomChangeTimer && finalCountZero==false)
			{
				float oldValue = pEntityDetails->m_timeToRandomChange;
				pEntityDetails->m_timeToRandomChange -= frameTime;

				if (oldValue >= pEntityDetails->m_timeBetweenSites && pEntityDetails->m_timeToRandomChange < pEntityDetails->m_timeBetweenSites)
				{
					bool canRemoveEntities = true;

					// Check if we can remove current entities
					int numEntities = pEntityDetails->m_currentEntities.size();
					for (int i = 0; i < numEntities; ++ i)
					{
						if (pEntityDetails->m_currentEntities[i])
						{
							canRemoveEntities &= m_pObjective->CanRemoveEntity(entityType, i);
						}
					}

					if (canRemoveEntities)
					{
						for (int i = 0; i < numEntities; ++ i)
						{
							if (pEntityDetails->m_currentEntities[i])
							{
								SvRemoveEntity(entityType, i, true);
							}
						}
					}
					else
					{
						pEntityDetails->m_timeToRandomChange = oldValue;
					}
				}

				if (pEntityDetails->m_timeToRandomChange < 0.f)
				{
					// Add new site
					SvDoRandomSelection(entityType);
				}
			}
		}
	}
	else
	{
		for (int entityType = 0; entityType < numEntityTypes; ++ entityType)
		{
			// Keep random site change timer vaguely up to date for host migration purposes
			SEntityDetails *pEntityDetails = &m_entityDetails[entityType];
			if (pEntityDetails->m_useRandomChangeTimer)
			{
				pEntityDetails->m_timeToRandomChange -= frameTime;
			}
		}
	}

	for (int entityType = 0; entityType < numEntityTypes; ++ entityType)
	{
		SEntityDetails *pEntityDetails = &m_entityDetails[entityType];
		if (pEntityDetails->m_updateType == SEntityDetails::eUT_All)
		{
			CallScriptUpdateFunction(pEntityDetails->m_allEntities, frameTime);
		}
		else if (pEntityDetails->m_updateType == SEntityDetails::eUT_Active)
		{
			CallScriptUpdateFunction(pEntityDetails->m_currentEntities, frameTime);
		}
	}

	m_pObjective->Update(frameTime);
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::CallScriptUpdateFunction( TEntityIdVec &entitiesVec, float frameTime )
{
	IScriptSystem *pScriptSystem = gEnv->pScriptSystem;

	int numEntities = entitiesVec.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entitiesVec[i]);
		if (pEntity)
		{
			IScriptTable *pScript = pEntity->GetScriptTable();
			if (pScript && pScript->GetValueType("Update") == svtFunction)
			{
				pScriptSystem->BeginCall(pScript, "Update");
				pScriptSystem->PushFuncParam(pScript);
				pScriptSystem->PushFuncParam(frameTime);
				pScriptSystem->EndCall();
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::OnStartGame()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	int numEntityTypes = m_entityDetails.size();
	for (int entityType = 0; entityType < numEntityTypes; ++ entityType)
	{
		SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

		pEntityDetails->m_allEntities.clear();
		pEntityDetails->m_currentEntities.clear();
		pEntityDetails->m_nthSpawn = -1;

		IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
		IEntity *pEntity = 0;

		pIt->MoveFirst();

		while(pEntity = pIt->Next())
		{
			if (pEntity->GetClass() == pEntityDetails->m_pEntityClass)
			{
				pEntityDetails->m_allEntities.push_back(pEntity->GetId());
			}
		}
		pEntityDetails->m_currentEntities.reserve(pEntityDetails->m_allEntities.size());

		if (gEnv->bServer)
		{
			m_pObjective->ClearEntities(entityType);
			switch (pEntityDetails->m_entitySelectType)
			{
			case EEST_All:
				{
					int numEntities = pEntityDetails->m_allEntities.size();
					for (int i = 0; i < numEntities; ++ i)
					{
						EntityId entId = pEntityDetails->m_allEntities[i];
						SvAddEntity(entityType, entId, i);
					}
				}
				break;
			case EEST_Random:
				{
					// All existing entities are available for selection
					pEntityDetails->m_availableEntities = pEntityDetails->m_allEntities;
					SvDoRandomSelection(entityType);
				}
				break;
			}
		}
	}

	m_pObjective->OnStartGame();
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::SvDoRandomSelection(int entityType)
{
	CRY_ASSERT(entityType < m_entityDetails.size());
	SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

	pEntityDetails->m_nthSpawn++;

	int numEntitiesToChoose = 1;
	if ((pEntityDetails->m_nthSpawn < pEntityDetails->m_entitySelectCount))
	{
		numEntitiesToChoose = pEntityDetails->m_entitySelectNthWaveCount[pEntityDetails->m_nthSpawn];
	}
	else if (pEntityDetails->m_entitySelectCount > 0)
	{
		numEntitiesToChoose = pEntityDetails->m_entitySelectNthWaveCount[pEntityDetails->m_entitySelectCount-1];
	}

	int numAvailable = pEntityDetails->m_availableEntities.size();
	for (int i = 0; i < numEntitiesToChoose; ++ i)
	{
		if (!numAvailable)
		{
			numAvailable += SvResetAvailableEntityList(entityType);
		}
		if (numAvailable)
		{
			int rndIndex = cry_rand() % numAvailable;
			EntityId entId = pEntityDetails->m_availableEntities[rndIndex];
			SvAddEntity(entityType, entId, i);
			stl::find_and_erase(pEntityDetails->m_availableEntities, entId);
			-- numAvailable;
		}
	}
	if (!numAvailable)
	{
		SvResetAvailableEntityList(entityType);
	}
	m_pObjective->SetWaveNumber(pEntityDetails->m_nthSpawn, numEntitiesToChoose);
	pEntityDetails->m_timeToRandomChange = pEntityDetails->m_randomChangeTimeLength;
}

//------------------------------------------------------------------------
int CGameRulesSimpleEntityBasedObjective::SvResetAvailableEntityList(int entityType)
{
	CRY_ASSERT(entityType < m_entityDetails.size());
	SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

	int entitiesMadeAvailable = 0;
	int numEntities = pEntityDetails->m_allEntities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		// Copy all entities that aren't currently active into the available entities list
		if (!stl::find(pEntityDetails->m_currentEntities, pEntityDetails->m_allEntities[i]))
		{
			pEntityDetails->m_availableEntities.push_back(pEntityDetails->m_allEntities[i]);
			++ entitiesMadeAvailable;
		}
	}
	return entitiesMadeAvailable;
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::SvAddEntity( int entityType, EntityId id, int index )
{
	CRY_ASSERT(entityType < m_entityDetails.size());
	SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

	if (pEntityDetails->m_currentEntities.size() <= index)
	{
		pEntityDetails->m_currentEntities.resize(index + 1);
	}
	pEntityDetails->m_currentEntities[index] = id;

	int flags = 0;
	bool countAsNewEntity = false;
	if (pEntityDetails->m_useRandomChangeTimer)
	{
		flags = SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_NEW;
		countAsNewEntity = true;
	}
	CGameRules::SModuleRMIEntityParams params(m_moduleRMIIndex, id, index | flags);
	g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMISingleEntity(), params, eRMI_ToRemoteClients, id);

	m_pObjective->AddEntityId(entityType, id, index, countAsNewEntity);
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::SvRemoveEntity(int entityType, int index, bool sendRMI)
{
	CRY_ASSERT(entityType < m_entityDetails.size());
	SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

	EntityId id = pEntityDetails->m_currentEntities[index];
	pEntityDetails->m_currentEntities[index] = 0;

	if (sendRMI)
	{
		CGameRules::SModuleRMIEntityParams params(m_moduleRMIIndex, id, SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_REMOVE);
		g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMISingleEntity(), params, eRMI_ToRemoteClients, id);
	}

	m_pObjective->RemoveEntityId(entityType, id);
}

//------------------------------------------------------------------------
bool CGameRulesSimpleEntityBasedObjective::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return m_pObjective->NetSerialize(ser, aspect, profile, flags);
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::Enable(int teamId, bool enable)
{
	m_pObjective->EnableScoring(teamId, enable);
	m_pObjective->EnableCompletion(teamId, enable);
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::OnClientEnteredGame( int channelId, bool isReset, EntityId playerId )
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	int numEntityTypes = m_entityDetails.size();
	for (int entityType = 0; entityType < numEntityTypes; ++ entityType)
	{
		SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

		int flags = 0;
		if (pEntityDetails->m_useRandomChangeTimer)
		{
			float timeSinceSiteChange = pEntityDetails->m_randomChangeTimeLength - pEntityDetails->m_timeToRandomChange;
			if (timeSinceSiteChange < SIMPLE_ENTITY_BASED_OBJECTIVE_CONNECTION_GRACE_TIME)
			{
				flags |= SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_NEW;
			}
		}

		int numEntities = pEntityDetails->m_currentEntities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			EntityId entId = pEntityDetails->m_currentEntities[i];

			if (entId)
			{
				CGameRules::SModuleRMIEntityParams params(m_moduleRMIIndex, entId, i | flags);
				pGameRules->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMISingleEntity(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, entId, channelId);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::OnSingleEntityRMI( CGameRules::SModuleRMIEntityParams params )
{
	// Determine which entity type we're dealing with
	int entityType = -1;
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(params.m_entityId);
	if (pEntity)
	{
		SEntityDetails *pEntityDetails = 0;
		int numEntityTypes = m_entityDetails.size();
		for (int i = 0; i < numEntityTypes; ++ i)
		{
			pEntityDetails = &m_entityDetails[i];

			if (pEntityDetails->m_pEntityClass == pEntity->GetClass())
			{
				entityType = i;
				break;
			}
		}
		if (entityType == -1)
		{
			CRY_ASSERT_MESSAGE(false, "Entity type not found");
			return;
		}

		// Work out if we're adding or removing
		int data = params.m_data;
		if (data & SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_REMOVE)
		{
			m_pObjective->RemoveEntityId(entityType, params.m_entityId);
			stl::find_and_erase(pEntityDetails->m_currentEntities, params.m_entityId);
		}
		else
		{
			bool isNewEntity = (0 != (data & SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_NEW));
			data &= ~SIMPLE_ENTITY_BASED_OBJECTIVE_FLAGS_NEW;
			if (pEntityDetails->m_currentEntities.size() <= data)
			{
				pEntityDetails->m_currentEntities.resize(data + 1);
			}
			pEntityDetails->m_currentEntities[data] = params.m_entityId;
			pEntityDetails->m_timeToRandomChange = pEntityDetails->m_randomChangeTimeLength;

			m_pObjective->AddEntityId(entityType, params.m_entityId, data, isNewEntity);
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesSimpleEntityBasedObjective::IsComplete(int teamId)
{
	return m_pObjective->IsComplete(teamId);
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::OnSpawn( IEntity *pEntity,SEntitySpawnParams &params )
{
	int numTypes = m_entityDetails.size();
	for (int entityType = 0; entityType < numTypes; ++ entityType)
	{
		SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

		if (pEntityDetails->m_scan && pEntity->GetClass() == pEntityDetails->m_pEntityClass)
		{
			EntityId entId = pEntity->GetId();

			int index = -1;
			int numSlots = pEntityDetails->m_allEntities.size();
			for (int i = 0; i < numSlots; ++ i)
			{
				if (pEntityDetails->m_allEntities[i] == 0)
				{
					pEntityDetails->m_allEntities[i] = entId;
					index = i;
					break;
				}
			}

			if (index == -1)
			{
				index = pEntityDetails->m_allEntities.size();
				pEntityDetails->m_allEntities.push_back(entId);
			}

			CryLog("CGameRulesSimpleEntityBasedObjective::OnSpawn, new entity '%s' detected", pEntity->GetName());
			if (gEnv->bServer && pEntityDetails->m_entitySelectType == EEST_All)
			{
				SvAddEntity(entityType, entId, index);
			}
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesSimpleEntityBasedObjective::OnRemove( IEntity *pEntity )
{
	int numTypes = m_entityDetails.size();
	for (int entityType = 0; entityType < numTypes; ++ entityType)
	{
		SEntityDetails *pEntityDetails = &m_entityDetails[entityType];

		if (pEntity->GetClass() == pEntityDetails->m_pEntityClass)
		{
			EntityId entId = pEntity->GetId();

#if 0
			CryLog("[tlh][6] entity type %d, size %d, ERASING!",entityType,pEntityDetails->m_allEntities.size());
			stl::find_and_erase(pEntityDetails->m_allEntities, entId);
#endif
#if 1
			CryLog("[tlh][6] entity type %d, size %d, ZEROING!",entityType,pEntityDetails->m_allEntities.size());
			int  numAll = pEntityDetails->m_allEntities.size();
			for (int i=0; i<numAll; i++)
			{
				if (pEntityDetails->m_allEntities[i] == entId)
				{
					pEntityDetails->m_allEntities[i] = 0;
				}
			}
#endif

			if (gEnv->bServer)
			{
				int numCurrentEntities = pEntityDetails->m_currentEntities.size();
				for (int index = 0; index < numCurrentEntities; ++ index)
				{
					if (pEntityDetails->m_currentEntities[index] == entId)
					{
						SvRemoveEntity(entityType, index, false);		// Don't need to tell clients, they will know because the entity has just been deleted
					}
				}
			}
			else
			{
				// Find and remove the entity
				int numEntities = pEntityDetails->m_currentEntities.size();
				for (int entIdx = 0; entIdx < numEntities; ++ entIdx)
				{
					EntityId id = pEntityDetails->m_currentEntities[entIdx];
					if (id == pEntity->GetId())
					{
						pEntityDetails->m_currentEntities[entIdx] = 0;
						m_pObjective->RemoveEntityId(entityType, id);
					}
				}
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::OnActionEvent( const SActionEvent& event )
{
	switch(event.m_event)
	{
	case eAE_resetBegin:
		{
			int numDetails = m_entityDetails.size();
			for (int i = 0; i < numDetails; ++ i)
			{
				SEntityDetails *pEntityDetails = &m_entityDetails[i];
				pEntityDetails->m_allEntities.clear();
				pEntityDetails->m_availableEntities.clear();
				pEntityDetails->m_currentEntities.clear();
			}
		}
		break;
	}
}

//------------------------------------------------------------------------
void CGameRulesSimpleEntityBasedObjective::OnHostMigration( bool becomeServer )
{
	m_pObjective->OnHostMigration(becomeServer);
}
