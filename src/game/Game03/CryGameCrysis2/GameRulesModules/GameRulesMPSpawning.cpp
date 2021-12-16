/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 04:09:2009 : Created by James Bamford

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesMPSpawning.h"
#include "GameRules.h"
#include "Game.h"
#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h" // TODO: Remove once HUD is sorted
#include "GameCVars.h"
#include "Actor.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "Player.h"
#include "Utility/CryWatch.h"
#include "IUIDraw.h"
#include "ILocalizationManager.h"
#include "WeaponSystem.h"
#include "Bullet.h"
#include "Utility/DesignerWarning.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#define VERBOSE_DEBUG_SPAWNING_VISTABLE 0

static AUTOENUM_BUILDNAMEARRAY(s_teamSpawnsTypeStr, ETSTList);

const float CSvSpawningVisTable::k_spawnLocationZOffset=1.5f;
const float CSvSpawningVisTable::k_playerZOffset=1.5f;

#if VISTABLE_USE_DEFERRED_LINETESTS
void CSvSpawningVisTable::STableElement::OnDataReceived(const EventPhysRWIResult *pRWIResult)
{
	if(pRWIResult->nHits)
	{
		m_visibleState = EVFL_NOT_VISIBLE;
	}
	else
	{
		m_visibleState = EVFL_VISIBLE;
	}
}
#endif


CSvSpawningVisTable::CSvSpawningVisTable()
{
	SetTableIndex(0);
	SetElementsIndex(0);

	m_pGameRules=NULL;

	m_tableVec.reserve(150);	// more spawns than we'd ever hope to need

#if DEBUG_SPAWNING_VISTABLE
	m_timeLastStartedNewUpdate=0.0f;
	m_timeLastFullTableUpdateTook=0.0f;
	m_numUpdatesSinceLastFullTableUpdate=0;
	m_numUpdatesForLastFullUpdate=0;
	m_lastTimeFullUpdateLogged=0.0f;
	m_numSpawnsVisTestedLastFullUpdate=0;
	m_numSpawnsVisTested=0;
#endif
}

CSvSpawningVisTable::~CSvSpawningVisTable()
{
	CryLog("CSvSpawningVisTable::~CSvSpawningVisTable()");
}

void CSvSpawningVisTable::Init(ETeamSpawnsType teamSpawnsType)
{
	CryLog("CSvSpawningVisTable::Init() setting teamSpawnsType=%d", teamSpawnsType);
	m_teamSpawnsType = teamSpawnsType;

	m_pGameRules = g_pGame->GetGameRules();
}

void CSvSpawningVisTable::AddSpawnLocation(EntityId location, bool doVisibilityTest)
{
	if (!doVisibilityTest)
	{
		CryLog("CSvSpawningVisTable::AddSpawnLocation() adding a spawn %s that does NOT want to doVisibilityTest", m_pGameRules->GetEntityName(location));
	}
	m_tableVec.push_back(SSpawnTableVectorElement(location, m_players, doVisibilityTest));
	int newIndex=m_tableVec.size() - 1;
	
	std::pair<TSpawnVisTableMap::iterator, bool> ret = m_tableMap.insert(TSpawnVisTableMap::value_type(location, SSpawnTableMapElement(newIndex)));

#if VISTABLE_USE_DEFERRED_LINETESTS
	TPlayerElements &playerElements = m_tableVec.at(newIndex).playerElements;
	for (int i=0; i<playerElements.size(); i++)
	{
		STableElement &element = playerElements[i];
		element.UpdateRaycastHelper();
	}
#endif
}

// requires elements to correctly implement operator=
template <class VectorType, class IteratorType>
void FastVectorErase(VectorType &vector, IteratorType &iteratorToErase)
{
	IteratorType vectorBack = vector.begin() + (vector.size()-1); // why is this so hard :)

	if (iteratorToErase != vectorBack)
	{
		*iteratorToErase = *vectorBack;
	}

	vector.erase(vectorBack);
}

void CSvSpawningVisTable::RemoveSpawnLocation(EntityId location)
{
	TSpawnVisTableMap::iterator it=m_tableMap.find(location);
	if (it != m_tableMap.end())
	{
		int indexToRemove=it->second.index;
		SSpawnTableVectorElement &vElement = m_tableVec.at(indexToRemove);
		CRY_ASSERT_MESSAGE(vElement.spawnEntityId == location, string().Format("CSvSpawningVisTable::RemoveSpawnLocation() has found that the vector element at the map's index %d does not have the same spawnEntityId (%d) as the map (%d). This shouldn't happen", indexToRemove, vElement.spawnEntityId, location));
		CryLogAlways("CSvSpawningVisTable::RemoveSpawnLocation() vElement=%p", &vElement);
		m_tableMap.erase(it);
#if 0
		// slow and "safe" erase
		m_tableVec.erase(m_tableVec.begin()+indexToRemove);

		// patch up any indices > indexToRemove
		it = m_tableMap.begin();
		for (; it != m_tableMap.end(); ++it)
		{	
			if (it->second.index > indexToRemove)
			{
				CryLog("CSvSpawningVisTable::RemoveSpawnLocation() have removed spawn location and found another location in the map who's index needs adjusting");
				it->second.index--;
			}
		}
		// TODO - do we need to do UpdateRaycastHelper(); on all the vector elements that will have shifted down from this remove?
#else
		int lastIndex=m_tableVec.size()-1;

		TSpawnVisTableVec::iterator iteratorToRemove = m_tableVec.begin()+indexToRemove;
		FastVectorErase(m_tableVec, iteratorToRemove);

		if (indexToRemove == lastIndex)
		{
			CryLog("CSvSpawningVisTable::RemoveSpawnLocation() has removed the element at the end of vector, no need to patch up any index's in the map");
		}
		else
		{
			// fix up the last element that's been swapped
			
			// fix up the raycast helper receivers in the swapped element
			SSpawnTableVectorElement &tableVecElement = m_tableVec[indexToRemove];
			int playerElementsSize = tableVecElement.playerElements.size();
			for (int i=0; i<playerElementsSize; i++)
			{
				STableElement &playerElement = tableVecElement.playerElements[i];
				playerElement.UpdateRaycastHelper();
			}

			bool hasPatchedIndex=false;

			it = m_tableMap.begin();
			for (; it != m_tableMap.end(); ++it)
			{	
				if (it->second.index == lastIndex)
				{
					CRY_ASSERT_MESSAGE(!hasPatchedIndex, "CSvSpawningVisTable::RemoveSpawnLocation() is patching index's in the map, has already patched one index and has found another index to patch in the map. This should not happen");
					CryLog("CSvSpawningVisTable::RemoveSpawnLocation() have removed spawn location and found another location in the map who's index needs adjusting");
					it->second.index=indexToRemove;

					hasPatchedIndex=true;
#if !_DEBUG
					// in debug carry on looking for duplicate elements to patch
					break;
#endif
				}
			}
			CRY_ASSERT_MESSAGE(hasPatchedIndex, "CSvSpawningVisTable::RemoveSpawnLocation() has failed to find any element in the map who's index requires patching. This should not happen");
		}
#endif
	}
}

void CSvSpawningVisTable::PlayerJoined(EntityId playerId)
{
	IEntity * playerEntity = gEnv->pEntitySystem->GetEntity(playerId);

	if (playerEntity && strcmp(playerEntity->GetClass()->GetName(), "Player") == 0)
	{
		STableElement newPlayerElement(playerId);
		m_players.push_back(newPlayerElement);

		int tableSize=m_tableVec.size();
		for (int i=0; i<tableSize; i++)
		{
			SSpawnTableVectorElement &element = m_tableVec.at(i);
		
			STableElement newElement(playerId);
			element.playerElements.push_back(newElement);
#if VISTABLE_USE_DEFERRED_LINETESTS
			element.playerElements.back().UpdateRaycastHelper();
#endif		
		}
	}
	else
	{
		CryLog("CSvSpawningVisTable::PlayerJoined() NOT adding player %s className %s as they are not a player", playerEntity ? playerEntity->GetName() : "NULL", playerEntity ? playerEntity->GetClass()->GetName() : "NULL");
	}
}

bool CSvSpawningVisTable::RemoveElementForPlayer(EntityId playerId, TPlayerElements &elements)
{
	bool found=false;

	int num=0;

	for (int i=0; i<elements.size(); i++)
	{
		STableElement &element=elements[i];
		if (element.m_playerEntityId == playerId)
		{
			bool swapped = elements.removeAt(i);			

#if VISTABLE_USE_DEFERRED_LINETESTS
			if (swapped)
			{
				// new ith element has been swapped, update its raycast helper
				elements[i].UpdateRaycastHelper();
			}
#endif

			//RemoveElement(it, elements);
			found=true;
			break;
		}
		num++;
	}

	return found;
}

void CSvSpawningVisTable::PlayerLeft(EntityId playerId)
{
	IEntity * playerEntity = gEnv->pEntitySystem->GetEntity(playerId);

	if (playerEntity && strcmp(playerEntity->GetClass()->GetName(), "Player") == 0)
	{
		bool removed;
		removed=RemoveElementForPlayer(playerId, m_players);
		assert(removed);

		CryLog("CSvSpawningVisTable::PlayerLeft player=%s, numRemainingPlayers=%i", playerEntity->GetName(), m_players.size());

		int tableSize=m_tableVec.size();
		for (int i=0; i<tableSize; i++)
		{
			SSpawnTableVectorElement &element = m_tableVec.at(i);
			removed=RemoveElementForPlayer(playerId, element.playerElements);
			assert(removed);
		}
	}
	else
	{
		CryLog("CSvSpawningVisTable::PlayerLeft() NOT removing player %s className %s as they are not a player", playerEntity ? playerEntity->GetName() : "NULL", playerEntity ? playerEntity->GetClass()->GetName() : "NULL");
	}
}

void CSvSpawningVisTable::SetTableIndex(unsigned int newTableIndex)
{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
	CryLog("CSvSpawningVisTable::SetTableIndex() %d -> %d", m_tableIndex, newTableIndex);
#endif
	m_tableIndex=newTableIndex;
}

void CSvSpawningVisTable::SetElementsIndex(unsigned int newElementsIndex)
{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
	CryLog("CSvSpawningVisTable::SetElementsIndex() %d -> %d", m_elementsIndex, newElementsIndex);
#endif
	m_elementsIndex=newElementsIndex;
}

void CSvSpawningVisTable::IncrementTableIndex()
{
	int tableSize=m_tableMap.size();
	if (tableSize > 0)
	{
		SetTableIndex(m_tableIndex+1);
		SetElementsIndex(0);
		
		if (m_tableIndex >= tableSize)
		{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			CryLog("CSvSpawningVisTable::IncrementTableIndex() found tableIndex (%d) is larger than m_tableMap size (%d). Setting back to 0", m_tableIndex, tableSize);
#endif
			SetTableIndex(0);

#if DEBUG_SPAWNING_VISTABLE
			float serverTime=m_pGameRules->GetServerTime();
			m_timeLastFullTableUpdateTook = serverTime - m_timeLastStartedNewUpdate;
			m_timeLastStartedNewUpdate = serverTime;
			m_numUpdatesForLastFullUpdate = m_numUpdatesSinceLastFullTableUpdate;
			m_numUpdatesSinceLastFullTableUpdate=0;
			m_numSpawnsVisTestedLastFullUpdate=m_numSpawnsVisTested;
			m_numSpawnsVisTested=0;

			if (serverTime - m_lastTimeFullUpdateLogged > 20000.0f)
			{
				CryLog("CSvSpawningVisTable: numLocations=%d; numPlayers=%d; numSpawnsVisTested=%d; testsPerFrame=%d; tableLatency=%d frames (%fs)\n", tableSize, m_players.size(), m_numSpawnsVisTestedLastFullUpdate, g_pGameCVars->g_spawn_vistable_numLineTestsPerFrame, m_numUpdatesForLastFullUpdate, m_timeLastFullTableUpdateTook / 1000.0f);
				m_lastTimeFullUpdateLogged = serverTime;
			}
#endif
		}
	}
	else
	{
		CryLog("CSvSpawningVisTable::IncrementTableIndex() found that m_tableMap is empty - doing nothing");
		SetTableIndex(0);
		SetElementsIndex(0);
	}
}

// Hopefully not slow anymore!
bool CSvSpawningVisTable::GetNthTableElement(unsigned int n, const EntityId **outSpawnLocation, CSvSpawningVisTable::SSpawnTableVectorElement **outElement)
{
	bool found=false;

	*outSpawnLocation = NULL;
	*outElement = NULL;

	CRY_ASSERT_MESSAGE(m_tableVec.size() == m_tableMap.size(), "tablevec and table map are different sizes, this should NEVER happen they have to be in sync");

	if (n >= 0 && n < m_tableVec.size())
	{
		found=true;
		
		SSpawnTableVectorElement &element = m_tableVec.at(n);
		*outElement = &element;
		*outSpawnLocation = &element.spawnEntityId;	// not strictly needed now this is in the actual struct we're returning
	}

	return found;
}

bool CSvSpawningVisTable::HandleFindingAltSpawnLocation(SSpawnTableVectorElement *element, IEntity *locationEntity)
{
	bool getNewElement=false;
	
	assert(locationEntity);

	if (locationEntity)
	{
		if (!element->altSpawnEntityId)
		{
			const Vec3 &pos = locationEntity->GetPos();
			//CryLog("CSvSpawningVisTable::HandleFindingAltSpawnLocation() found a SpawnTableElement %s that is NOT doing visibility tests but has not yet got an altSpawnEntityId set. Locating close enough spawn to use as altSpawn", locationEntity->GetName());	
			float maxDistSq=g_pGameCVars->g_spawn_vistable_maxDistToAltSpawnEntity*g_pGameCVars->g_spawn_vistable_maxDistToAltSpawnEntity;

			int tableSize=m_tableVec.size();
			for (int i=0; i<tableSize; i++)
			{
				SSpawnTableVectorElement &ele = m_tableVec.at(i);

				EntityId altLocation = ele.spawnEntityId;

				if (ele.doVisibilityTest)
				{
					IEntity *altLocationEntity=gEnv->pEntitySystem->GetEntity(altLocation);

					assert(altLocationEntity);
					if (altLocationEntity)
					{
						const Vec3 &altPos = altLocationEntity->GetPos();
						Vec3 diff = pos - altPos;
						float distSq = diff.GetLengthSquared();

						//CryLog("CSvSpawningVisTable::HandleFindingAltSpawnLocation() found potential altSpawnEntity %s that IS doing visibilityTests", altLocationEntity->GetName());

						if (distSq < maxDistSq)
						{
							//CryLog("CSvSpawningVisTable::HandleFindingAltSpawnLocation() found potential altSpawnEntity %s that is close enough to be used as the altSpawn for spawn %s", altLocationEntity->GetName(), locationEntity->GetName());

							CRY_ASSERT_MESSAGE(!element->altSpawnEntityId, string().Format("SETUP ERROR: CONFLICT: Trying to find altSpawn for spawn %s and has found spawn %s which is close enough (< %fm) but spawn %s has already been found within range. This should not happen", locationEntity->GetName(), altLocationEntity->GetName(), g_pGameCVars->g_spawn_vistable_maxDistToAltSpawnEntity, m_pGameRules->GetEntityName(element->altSpawnEntityId)));
#if DESIGNER_WARNING_ENABLED
							if (!gEnv->pSystem->IsEditor())		// Since the editor uses all layers at the same time, we end up with conflicts that would never occur in the main game
							{
								DesignerWarning(!element->altSpawnEntityId, string().Format("SETUP ERROR: CONFLICT: Trying to find altSpawn for spawn %s and has found spawn %s which is close enough (< %fm) but spawn %s has already been found within range. This should not happen", locationEntity->GetName(), altLocationEntity->GetName(), g_pGameCVars->g_spawn_vistable_maxDistToAltSpawnEntity, m_pGameRules->GetEntityName(element->altSpawnEntityId)));
							}
#endif

							if (!element->altSpawnEntityId)
							{
								element->altSpawnEntityId = altLocation;
							}
							else
							{
								//CryLogAlways("SETUP ERROR: CONFLICT: Trying to find altSpawn for spawn %s and has found spawn %s which is close enough (< %fm) but spawn %s has already been found within range. This should not happen", locationEntity->GetName(), altLocationEntity->GetName(), g_pGameCVars->g_spawn_vistable_maxDistToAltSpawnEntity, m_pGameRules->GetEntityName(element->altSpawnEntityId));
							}
						}
					}
				}
			}
		}

		CRY_ASSERT_MESSAGE(element->altSpawnEntityId, string().Format("SETUP ERROR: CSvSpawningVisTable::HandleFindingAltSpawnLocation() failed to find an alternative spawn to use for spawn %s", locationEntity->GetName()));
#if DESIGNER_WARNING_ENABLED
		if (!gEnv->pSystem->IsEditor())		// don't want to continuously pester the designers (and artists) in the editor with designer warnings!
		{
			DesignerWarning(element->altSpawnEntityId, string().Format("SETUP ERROR: CSvSpawningVisTable::HandleFindingAltSpawnLocation() failed to find an alternative spawn to use for spawn %s", locationEntity->GetName()));
		}
#endif

		if (element->altSpawnEntityId)
		{
			const char *spawnEntityName = locationEntity->GetName();
			const char *altSpawnEntityName = m_pGameRules->GetEntityName(element->altSpawnEntityId);

#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			//CryLog("CSvSpawningVisTable::HandleFindingAltSpawnLocation() found a SpawnTableElement for spawn %s has an altSpawnEntityId set to %s. Moving onto next spawn's table elements", spawnEntityName ? spawnEntityName : "NULL", altSpawnEntityName ? altSpawnEntityName : "NULL");
#endif			
			IncrementTableIndex();
			getNewElement=true;
		}
		else
		{
			//CryLogAlways("CSvSpawningVisTable::HandleFindingAltSpawnLocation() failed to find an alternative spawn to use for spawn %s", locationEntity->GetName());
		}
	}

	return getNewElement;
};


bool CSvSpawningVisTable::GetNextElementToTest(const EntityId **outSpawnLocation, STableElement **outTableElement, int &outUsedTableIndex, int &outUsedElementsIndex)
{
	bool found=false;
	*outSpawnLocation=NULL;
	*outTableElement=NULL;
	outUsedTableIndex=-1;
	outUsedElementsIndex=-1;

	int tableSize=m_tableMap.size();	// for debugging

	if (m_tableMap.size() > 0)
	{
		assert(m_tableIndex < m_tableMap.size());
		if (m_tableIndex >= m_tableMap.size())
		{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			CryLog("CSvSpawningVisTable::GetNextElementToTest() found tableIndex (%d) is larger than m_tableMap size (%d). This can only happen if a spawn location has been removed. Setting back to 0", m_tableIndex, m_tableMap.size());
#endif
			IncrementTableIndex();	// will reset counts and wrap back to 0
		}

		bool getNewElement=false;
		SSpawnTableVectorElement *element=NULL;
		const EntityId *spawnlocation = NULL;
		bool noPlayers=false;

		do 
		{
			getNewElement=false;
			element=NULL;
			spawnlocation=NULL;
			bool success = GetNthTableElement(m_tableIndex, &spawnlocation, &element);
			assert(success);
			if (element && element->playerElements.size() > 0 && spawnlocation)
			{
				int elementsSize=element->playerElements.size(); 

#if VERBOSE_DEBUG_SPAWNING_VISTABLE
				{
					IEntity *locationEntity=gEnv->pEntitySystem->GetEntity(*spawnlocation);
					CryLog("CSvSpawningVisTable::GetNextElementToTest() considering spawnlocation %s - m_tableIndex=%d; m_elementsIndex=%d", locationEntity ? locationEntity->GetName() : "NULL", m_tableIndex, m_elementsIndex);
				}
#endif
				//assert(m_elementsIndex < elements->size());
				if (m_elementsIndex >= elementsSize)
				{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
					CryLog("CSvSpawningVisTable::GetNextElementToTest() found elementsIndex (%d) is larger than current elements size (%d). This can only have happened if a player has logged off. Moving onto next table elements", m_elementsIndex, elementsSize);
#endif
					IncrementTableIndex();
					m_numSpawnsVisTested++;
					getNewElement=true;
				}
				else if (!element->doVisibilityTest)
				{
					IEntity *locationEntity=gEnv->pEntitySystem->GetEntity(*spawnlocation);
					assert(locationEntity);
					if (locationEntity)
					{
						getNewElement = HandleFindingAltSpawnLocation(element, locationEntity);
					}
				}
			}
			else
			{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
				CryLog("CSvSpawningVisTable::GetNextElementToTest() found that a table element is itself empty - no players - doing nothing");
#endif
				SetElementsIndex(0);
				noPlayers=true;
			}
		} while (getNewElement);

		if (!noPlayers)
		{
			assert(element);
			if (element)
			{
				STableElement &playerElement=element->playerElements[m_elementsIndex];
				*outSpawnLocation = spawnlocation;
				*outTableElement = &playerElement;
				outUsedTableIndex = m_tableIndex;
				outUsedElementsIndex = m_elementsIndex;
				found=true;

				SetElementsIndex(m_elementsIndex+1);
				if (m_elementsIndex >= element->playerElements.size())
				{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
					CryLog("CSvSpawningVisTable::GetNextElementToTest() found elementsIndex (%d) is larger than current elements size (%d). Moving onto next table elements", m_elementsIndex, element->playerElements.size());
#endif
					IncrementTableIndex();
					m_numSpawnsVisTested++;
				}
			}
			else
			{
				CryLogAlways("CSvSpawningVisTable::GetNextElementToTest() after iterating failed to find an element to return. This should not happen. TableIndex %d", m_tableIndex);
			}
		}
	}
	else
	{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
		CryLog("CSvSpawningVisTable::GetNextElementToTest() found that m_tableMap is empty - no spawn props - doing nothing");
#endif
		SetTableIndex(0);
	}

	return found;
}

bool CSvSpawningVisTable::ShouldTestElement(EntityId spawnEntityId, const STableElement *tableElement, const IEntity *spawnEntity, const IEntity *playerEntity)
{
	bool shouldTest=true;

	if (m_pGameRules->IsDead(tableElement->m_playerEntityId))
	{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
		if (tableElement->m_visibleState != EVFL_UNTESTED)
		{
			CryLog("CSvSpawningVisTable::ShouldTestElement() - spawn=%s; player=%s; Found that player %s is dead, setting visible flags to untested", spawnEntity->GetName(), playerEntity->GetName(), playerEntity->GetName());
		}
#endif
		shouldTest = false;
	}
	else if (m_teamSpawnsType != eTST_None) 
	{
		int spawnTeam;

		CGameRulesMPSpawningBase*  spawnmod = (CGameRulesMPSpawningBase*) m_pGameRules->GetSpawningModule();
		CRY_ASSERT(spawnmod);
		if (spawnmod)
		{
			spawnTeam=spawnmod->GetSpawnLocationTeam(spawnEntityId);
		}
		else
		{
			spawnTeam=m_pGameRules->GetTeam(spawnEntityId);
		}

		int playerTeam=m_pGameRules->GetTeam(tableElement->m_playerEntityId);

		if (spawnTeam == playerTeam)
		{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			CryLog("CSvSpawningVisTable::ShouldTestElement() found that player %s is on the same team (%d) as the spawnpoint %s. Not testing element", playerEntity->GetName(), playerTeam, spawnEntity->GetName());
#endif
			shouldTest=false;
		}
	}

	return shouldTest;
}

// copied from tunneling grenade
static uint32 PROBE_TYPES = ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid;
static uint32 PROBE_FLAGS = (geom_colltype_ray/*|geom_colltype13*/)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces|8;

// TODO allow it to factor in a useTeamSpawns gamemode to reduce the line tests to calculate
// change boolean to an enum to allow a new untested value to isVisible in cases where its not a complete map
void CSvSpawningVisTable::Update()
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_GAME);

	int num=0;
	bool success=false;

	const EntityId *spawnEntityId=NULL;
	STableElement *tableElement=NULL;
	IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;

	int startingTableIndex=-1;    //m_tableIndex;
	int startingElementsIndex=-1; //m_elementsIndex;

	int usedTableIndex;
	int usedElementsIndex;

	// if there are fewer elements to test than g_spawn_vistable_numLineTestsPerFrame it wastes time retesting the same elements and 
	// makes the latency calculations be wrong. Not usually a problem in a typical game
	while (num<g_pGameCVars->g_spawn_vistable_numLineTestsPerFrame && (success=GetNextElementToTest(&spawnEntityId, &tableElement, usedTableIndex, usedElementsIndex)))
	{
		assert(spawnEntityId);
		assert(tableElement);
		assert(usedTableIndex >=0);
		assert(usedElementsIndex >=0);

		IEntity *spawnEntity = gEnv->pEntitySystem->GetEntity(*spawnEntityId);
		IEntity *playerEntity = gEnv->pEntitySystem->GetEntity(tableElement->m_playerEntityId);
		bool firstTest=false;

		if (startingTableIndex < 0 && startingElementsIndex < 0)
		{
			startingTableIndex = usedTableIndex;
			startingElementsIndex = usedElementsIndex;
			firstTest=true;

#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			CryLog("CSvSpawningVisTable::Update() setting startingTableIndex=%d; startingElementIndex=%d\n", startingTableIndex, startingElementsIndex);
#endif
		}

		if (ShouldTestElement(*spawnEntityId, tableElement, spawnEntity, playerEntity))
		{
			Vec3 source=playerEntity->GetPos();
			source.z +=k_playerZOffset;
			Vec3 dest=spawnEntity->GetPos();
			dest.z += k_spawnLocationZOffset;
			Vec3 dir=dest - source;

			ray_hit hit;

			//CryLog("Testing spawnEntity=%s (@ %f, %f, %f) vs playerEntity=%s (@ %f, %f, %f); tableElement=%p\n", spawnEntity->GetName(), dest.x, dest.y, dest.z, playerEntity->GetName(), source.x, source.y, source.z, tableElement);
			IPhysicalEntity *skipEntities[1];
			skipEntities[0]=playerEntity->GetPhysics();
			num++;

#if VISTABLE_USE_DEFERRED_LINETESTS
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			CryLog("CSvSpawningVisTable::Update() performing deferred raycast for spawn=%s vs player=%s; tableElement=%p", spawnEntity->GetName(), playerEntity->GetName(), tableElement);
#endif
			tableElement->m_raycastHelper->CastRay(source, dir, PROBE_TYPES, PROBE_FLAGS, skipEntities, 1);
#else
			if ( pWorld->RayWorldIntersection(source, dir, PROBE_TYPES, PROBE_FLAGS, &hit, 1, skipEntities, 1))
			{
				tableElement->m_visibleState = EVFL_NOT_VISIBLE;
			}
			else
			{
				tableElement->m_visibleState = EVFL_VISIBLE;
			}
#endif

#if DEBUG_SPAWNING_VISTABLE
			tableElement->m_timeLastTested = m_pGameRules->GetServerTime();
#endif
		}
		else
		{
			tableElement->m_visibleState = EVFL_UNTESTED;
		}

		spawnEntityId=NULL;
		tableElement=NULL;

#if VERBOSE_DEBUG_SPAWNING_VISTABLE
		CryLog("CSvSpawningVisTable::Update() startingTableIndex=%d; startingElementsIndex=%d; usedTableIndex=%d; usedElementsIndex=%d", startingTableIndex, startingElementsIndex, usedTableIndex, usedElementsIndex);
#endif

		if (!firstTest && startingTableIndex==usedTableIndex && startingElementsIndex==usedElementsIndex)
		{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
			CryLog("CSvSpawningVisTable::Update() has managed to wrap round all of the vistable in one update - breaking out of loop now");
#endif
			break;
		}
	}

//	assert(success);	// happens on restart with no players left

#if VERBOSE_DEBUG_SPAWNING_VISTABLE
	Debug();
#endif

#if DEBUG_SPAWNING_VISTABLE
	if (g_pGameCVars->g_spawn_vistable_show)
	{
		VisuallyDebug();
	}
	m_numUpdatesSinceLastFullTableUpdate++;
#endif
}

#if DEBUG_SPAWNING_VISTABLE
void CSvSpawningVisTable::Debug()
{
	TSpawnVisTableMap::const_iterator it = m_tableMap.begin();
	for (; it != m_tableMap.end(); it++)
	{
		EntityId locationFromMap = it->first;
		int index = it->second.index;

		SSpawnTableVectorElement &element = m_tableVec.at(index);

		EntityId location = element.spawnEntityId;
		EntityId altLocation = element.altSpawnEntityId; //it->second.altSpawnEntityId;
		
		const TPlayerElements &elements=element.playerElements;
		
		IEntity *locationFromMapEntity=gEnv->pEntitySystem->GetEntity(locationFromMap);
		IEntity *locationEntity=gEnv->pEntitySystem->GetEntity(location);
		IEntity *altLocationEntity=gEnv->pEntitySystem->GetEntity(altLocation);
		CryLog("indexFromMap=%d; locationFromMap=%s (%d); locationFromVec=%s (%d)", index, locationFromMapEntity ? locationFromMapEntity->GetName() : "NULL", locationFromMap, locationEntity->GetName(), location);

		for (int i=0; i<elements.size(); i++)
		{
			const STableElement &element = elements[i];

			IEntity *playerEntity=gEnv->pEntitySystem->GetEntity(element.m_playerEntityId);
			CryLog("location=%s; altLocationEntity=%s; player=%s; visibleflags=%x; time=%f", locationEntity->GetName(), altLocationEntity ? altLocationEntity->GetName() : "NULL", playerEntity->GetName(), element.m_visibleState, element.m_timeLastTested);
		}
	}
}

void CSvSpawningVisTable::VisuallyDebugVisTests()
{
	TSpawnVisTableMap::const_iterator it = m_tableMap.begin();
	for (; it != m_tableMap.end(); it++)
	{
		EntityId locationFromMap = it->first;
		int index = it->second.index;
		SSpawnTableVectorElement &element = m_tableVec.at(index);

		assert(element.spawnEntityId == locationFromMap);

		EntityId location = element.spawnEntityId;
		const TPlayerElements *elements=&element.playerElements;
		bool usingAltSpawn=false;

		if (!element.doVisibilityTest && element.altSpawnEntityId)
		{
			TSpawnVisTableMap::const_iterator altIt = m_tableMap.find(element.altSpawnEntityId);
			if (altIt != m_tableMap.end())
			{
				usingAltSpawn=true;
	
				int altIndex = altIt->second.index;
				SSpawnTableVectorElement &altElement = m_tableVec.at(altIndex);

				//const TPlayerElements &elements2 = altIt->second.playerElements;
				elements = &altElement.playerElements; 
			}
		}



		IEntity *locationEntity=gEnv->pEntitySystem->GetEntity(location);
		Vec3 locationPos = locationEntity->GetPos();
		locationPos.z += k_spawnLocationZOffset;
		ColorF sphereColor;

		if (usingAltSpawn)
		{
			sphereColor.set(0.1f, 0.1f, 0.1f, 0.6f);
		}
		else
		{
			sphereColor.set(0.7f, 0.7f, 0.7f, 0.6f);
		}

		g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(locationPos, 0.5f, sphereColor, 0.05f);

		for (int i=0; i<elements->size(); i++)
		{
			const STableElement &element=elements->operator[](i);

			IEntity *playerEntity=gEnv->pEntitySystem->GetEntity(element.m_playerEntityId);
			
			ColorF color;
			switch(element.m_visibleState)
			{
				case EVFL_UNTESTED:
					if (usingAltSpawn)
					{
						color.Set(0.1f, 0.1f, 0.1f, 0.6f);
					}
					else
					{
						color.Set(0.7f, 0.7f, 0.7f, 0.6f);
					}
					break;
				case EVFL_NOT_VISIBLE:
					if (usingAltSpawn)
					{
						color.Set(0.2f, 0.f, 0.f, 1.f);
					}
					else
					{
						color.Set(1.f, 0.f, 0.f, 1.f);
					}
					break;
				case EVFL_VISIBLE:
					if (usingAltSpawn)
					{
						color.Set(0.f, 0.2f, 0.f, 1.f);
					}
					else
					{
						color.Set(0.f, 1.f, 0.f, 1.f);
					}
					break;
			}

			Vec3 playerPos=playerEntity->GetPos();
			playerPos.z +=k_playerZOffset;

			g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddLine(locationPos, playerPos, color, 0.05f);
			//DEBUG_OUTPUT("location=%s; player=%s; visible=%d; time=%f", locationEntity->GetName(), playerEntity->GetName(), element.m_isVisible, element.m_timeLastTested);
		}
	}
}

void CSvSpawningVisTable::VisuallyDebugAltSpawns()
{
	float sphereRadius=0.5f;

	TSpawnVisTableMap::const_iterator it = m_tableMap.begin();
	for (; it != m_tableMap.end(); it++)
	{
		EntityId locationFromMap = it->first;
		int index = it->second.index;
		SSpawnTableVectorElement &element = m_tableVec.at(index);
		assert(element.spawnEntityId == locationFromMap);

		EntityId location = element.spawnEntityId; 
		EntityId altLocation=0;
		bool usingAltSpawn=false;
		
		if (!element.doVisibilityTest && element.altSpawnEntityId)
		{
			TSpawnVisTableMap::const_iterator altIt = m_tableMap.find(element.altSpawnEntityId);
			if (altIt != m_tableMap.end())
			{
				usingAltSpawn=true;
				int altIndex = altIt->second.index;
				SSpawnTableVectorElement &altElement = m_tableVec.at(altIndex);

				//const TPlayerElements &elements2 = altIt->second.playerElements;
				altLocation = altElement.spawnEntityId; 
			}
		}

		IEntity *locationEntity=gEnv->pEntitySystem->GetEntity(location);
		assert(locationEntity);
		Vec3 locationPos = locationEntity->GetPos();
		locationPos.z += k_spawnLocationZOffset;
		ColorF sphereColor;

		if (usingAltSpawn)
		{
			sphereColor.set(0.1f, 0.1f, 0.1f, 0.6f);
		}
		else
		{
			sphereColor.set(0.7f, 0.7f, 0.7f, 0.6f);
		}

		g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(locationPos, sphereRadius, sphereColor, 0.05f);

		if (altLocation)
		{
			IEntity *altLocationEntity=gEnv->pEntitySystem->GetEntity(altLocation);
			assert(altLocationEntity);
			Vec3 altLocationPos = altLocationEntity->GetPos();
			altLocationPos.z += k_spawnLocationZOffset;

			Vec3 dir = locationPos - altLocationPos;
			Vec3 normalisedDir = dir;
			normalisedDir.Normalize();
			float len = (dir.len() - 2*sphereRadius) / 2.0f;
			len /= 1.15f;	// cone is drawn on the end with 0.3 of the radius - 
			Vec3 centrePos = altLocationPos + (normalisedDir*sphereRadius) + (normalisedDir * len);
			//float len = dir.len() - sphereRadius;		// the spheres overwrite the cones of the arrows so end on the edge of the sphere so you can see the start of the arrowhead
//			g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddDirection(altLocationPos, len, dir, ColorF(0.7f, 0.7f, 0.7f, 0.6f), 0.05f);
			g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddDirection(centrePos, len, dir, ColorF(0.7f, 0.7f, 0.7f, 0.6f), 0.05f);
		}
	}
}

// TODO debug altPlayerElements as well on different values of the debug cvar
void CSvSpawningVisTable::VisuallyDebug()
{
	CryWatch("numLocations=%d; numPlayers=%d; tableIndex=%d; elementsIndex=%d; numSpawnsVisTested=%d; testsPerFrame=%d; tableLatency=%d frames (%fs)\n", m_tableMap.size(), m_players.size(), m_tableIndex, m_elementsIndex, m_numSpawnsVisTestedLastFullUpdate, g_pGameCVars->g_spawn_vistable_numLineTestsPerFrame, m_numUpdatesForLastFullUpdate, m_timeLastFullTableUpdateTook / 1000.0f);
	
	g_pGame->GetIGameFramework()->GetIPersistantDebug()->Begin("spawn_vistable", false);

	switch (g_pGameCVars->g_spawn_vistable_show)
	{
		case 1:
			VisuallyDebugVisTests();
			break;
		case 2:
			VisuallyDebugAltSpawns();
			break;
	}
}

#endif

bool CSvSpawningVisTable::IsSpawnLocationVisibleByTeam(EntityId location, int teamId) const
{
	bool visible=false;
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
	IEntity *locationEntity=gEnv->pEntitySystem->GetEntity(location);
#endif
	TSpawnVisTableMap::const_iterator it = m_tableMap.find(location);
	assert(it != m_tableMap.end());
	if (it != m_tableMap.end())
	{
		int index = it->second.index;
		const SSpawnTableVectorElement &element = m_tableVec.at(index);
		const TPlayerElements &elements=element.playerElements;
		
		for (int i=0; i<elements.size(); i++)
		{
			const STableElement &element = elements[i];

			int playerTeamId = m_pGameRules->GetTeam(element.m_playerEntityId);
			if (playerTeamId != teamId)
			{
				continue;
			}

			IEntity *playerEntity=gEnv->pEntitySystem->GetEntity(element.m_playerEntityId);

			if (element.m_visibleState == EVFL_VISIBLE)
			{
#if VERBOSE_DEBUG_SPAWNING_VISTABLE
				CryLog("CSvSpawningVisTable::IsSpawnLocationVisible() found that player %s can see spawn location %s", playerEntity ? playerEntity->GetName() : "NULL", locationEntity ? locationEntity->GetName() : "NULL");
#endif
				visible=true;
				break;
			}
		}
	}

	return visible;
}

bool CSvSpawningVisTable::CanPlayerSeeSpawnLocation(EntityId playerId, EntityId location) const
{
	assert(0);
	// TODO - implement me if needed
	return false;
}


CGameRulesMPSpawningBase::CGameRulesMPSpawningBase()
{
	m_teamGame = false;
	m_teamSpawnsType = eTST_None;
	m_handleAutoReviving = false;
	m_localOptedOut = false;
	m_minTimeFromDeathTillRevive = 0.f;
	m_timeFromDeathTillAutoRevive = 0.f;
	m_usingLua=false;
	m_allowMidRoundJoining = true;
}

CGameRulesMPSpawningBase::~CGameRulesMPSpawningBase()
{
}

void CGameRulesMPSpawningBase::Init(XmlNodeRef xml)
{
	inherited::Init(xml);

	int  ival;
	bool  bval;
	const char*  cpval;

	if (!xml->getAttr("teamGame", m_teamGame))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase failed to find valid teamGame param");
	}

	bool  typeOk = false;
	if (xml->getAttr("teamSpawnsType", &cpval))
	{
		if (AutoEnum_GetEnumValFromString(cpval, s_teamSpawnsTypeStr, eTST_NUM, &ival))
		{
			m_teamSpawnsType = (ETeamSpawnsType) ival;
			typeOk = true;
		}
	}
	if (!typeOk)
	{
		if (xml->getAttr("useTeamSpawns", bval))  // legacy
		{
			m_teamSpawnsType = (bval ? eTST_Standard : eTST_None);
			typeOk = true;
		}
	}
	CRY_ASSERT_MESSAGE(typeOk, "CGameRulesMPSpawningBase failed to find valid teamSpawnsType OR useTeamSpawns param");

	if (!xml->getAttr("handleAutoReviving", m_handleAutoReviving))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase failed to find valid handleAutoReviving param");
	}
	if (!xml->getAttr("minTimeFromDeathTillRevive", m_minTimeFromDeathTillRevive))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase failed to find valid minTimeFromDeathTillRevive param");
	}
	if (!xml->getAttr("timeFromDeathTillAutoRevive", m_timeFromDeathTillAutoRevive))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase failed to find valid timeFromDeathTillAutoRevive param");
	}
	if (!xml->getAttr("usingLua", m_usingLua))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase failed to find valid usingLua param");
	}

	if (!xml->getAttr("midRoundJoining", m_allowMidRoundJoining))
	{
		m_allowMidRoundJoining = true;
	}

	CryLog("CGameRulesMPSpawningBase::Init() set params to teamGame=%d; teamSpawnsType=%d; timeFromDeathTillAutoRevive=%f; usingLua=%d", m_teamGame, m_teamSpawnsType, m_timeFromDeathTillAutoRevive, m_usingLua);

	m_vistable.Init(m_teamSpawnsType);
}

// FIXME a nasty temporary hack function (copy-pasted from elsewhere) until we have a version of Draw2dLabel that works with wstrings
static inline string& WStrToStrHACK(const wstring& str, string& dstr)
{
	dstr.resize(str.length());
	char* dst = dstr.begin();
	const wchar_t* src = str.c_str();
	while (const char c = (char) (*src++))
	{
		*dst++ = c;
	}
	return dstr;
}


void CGameRulesMPSpawningBase::Update(float frameTime)
{
	inherited::Update(frameTime);

	bool  forceInstantRevive = (g_pGameCVars->g_spectate_DisableDead != 0);

	if (gEnv->bServer)
	{
		if (m_handleAutoReviving || forceInstantRevive)
		{
			IGameRulesStateModule*  stateModule = m_pGameRules->GetStateModule();
			if (!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame))
			{
				float gameTime = GetTime();

				TPlayerDataMap::iterator it = m_playerValues.begin();
				for (; it != m_playerValues.end(); ++it)
				{
					EntityId playerId = it->first;
					IActor *pActor=(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
					CActor *pActorImpl = static_cast<CActor *>(pActor);

					if (pActorImpl)
					{
						IGameRulesSpectatorModule*  specmod = m_pGameRules->GetSpectatorModule();
						float  autoSpectateTime = (specmod ? specmod->GetTimeFromDeathTillAutoSpectate() : 0.f);

						if ((((autoSpectateTime <= 0.f) || forceInstantRevive) && (pActorImpl->GetSpectatorMode() <= 0)) || ((autoSpectateTime > 0.f) && (autoSpectateTime < m_timeFromDeathTillAutoRevive)))
						{
							// Not in spectator mode - or is in spectator mode because auto spectate is set

							if (pActorImpl->GetHealth() <= 0)
							{
								if ((it->second.reviveOptOutTime <= it->second.deathTime) || forceInstantRevive)
								{
									// Haven't opted-out of auto-revive for this death

									if ((pActorImpl->GetSpectatorState()==CActor::eASS_Ingame) && (((gameTime-it->second.deathTime) > m_timeFromDeathTillAutoRevive) || forceInstantRevive))
									{
										//Only call PerformRevive once after deathTime
										if(it->second.lastRevivedTime < it->second.deathTime)
										{
											CryLog("CGameRulesMPSpawning::Update() found a player %s who's been dead long enough - auto reviving", pActor->GetEntity()->GetName());
											PerformRevive(playerId, m_pGameRules->GetTeam(playerId));
											it->second.lastRevivedTime = gameTime;
										}
									}
								}
							}
						}
					}
					else
					{
						CryLog("CGameRulesMPSpawningBase::Update() failed to find an actor for a player in m_playerValues, this should NOT happen");
					}
				}
			}
		}

		m_vistable.Update();

		if (g_pGameCVars->g_spawn_vistable_show)	// show enemy reject dist to aid designers setting up levels
		{
			for (int i=1; i<3; i++)
			{
				float rejectDistSqr = GetMinEnemyDist(i, false);
				CryWatch("Team %d: MinEnemyDist=%f", i, rejectDistSqr);
			}
		}

		//DebugPOIs();	
	}


	if( m_localOptedOut && m_handleAutoReviving )
	{
		bool  forceInstantRevive = (g_pGameCVars->g_spectate_DisableDead != 0);

		if( !forceInstantRevive )
		{
			IGameRulesSpectatorModule*  specmod = m_pGameRules->GetSpectatorModule();
			float  autoSpectateTime = (specmod ? specmod->GetTimeFromDeathTillAutoSpectate() : 0.f);

			IActor* pClient=g_pGame->GetIGameFramework()->GetClientActor();
			if( pClient )
			{
				CActor* pActorImpl = static_cast< CActor* >( pClient );

				if ( !( (pActorImpl->GetHealth() <= 0) && (autoSpectateTime > 0.f) && (pActorImpl->GetSpectatorMode() > 0) && (pActorImpl->GetSpectatorState() == CActor::eASS_Ingame) ) )
				{
					m_localOptedOut = false;
				}
			}
		}
	}
}

void CGameRulesMPSpawningBase::AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest)
{
	inherited::AddSpawnLocation(location, isBaseSpawn, doVisTest);

	m_vistable.AddSpawnLocation(location, doVisTest); 
}

void CGameRulesMPSpawningBase::RemoveSpawnLocation(EntityId id, bool isBaseSpawn)
{
	inherited::RemoveSpawnLocation(id, isBaseSpawn);

	m_vistable.RemoveSpawnLocation(id);
}

void CGameRulesMPSpawningBase::PlayerJoined(EntityId playerId)
{
	inherited::PlayerJoined(playerId);

	m_vistable.PlayerJoined(playerId);
}

void CGameRulesMPSpawningBase::PlayerLeft(EntityId playerId)
{
	inherited::PlayerLeft(playerId);

	m_vistable.PlayerLeft(playerId);
}

void CGameRulesMPSpawningBase::SvOnPlayerKilled(const HitInfo &hitInfo)
{
	inherited::SvOnPlayerKilled(hitInfo);

	CryLog("CGameRulesMPSpawningBase::SvOnPlayerKilled()");
	
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesMPSpawningBase::SvOnPlayerKilled() yet NOT a server");

	TPlayerDataMap::iterator it = m_playerValues.find(hitInfo.targetId);
	if (it != m_playerValues.end())
	{
		it->second.reviveOptOutTime = 0.f;
	}
	else
	{
		IEntity *targetEntity = gEnv->pEntitySystem->GetEntity(hitInfo.targetId);
		CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesMPSpawningBase::SvOnPlayerKilled() failed to find a playervalue for player %s. This should not happen", targetEntity ? targetEntity->GetName() : "NULL"));
	}
}

void CGameRulesMPSpawningBase::AddAvoidPOI(EntityId entityId, float avoidDistance, bool startActive)
{
#if DEBUG_NEW_SPAWNING
	IEntity *poiEntity = gEnv->pEntitySystem->GetEntity(entityId);
	CryLog("CGameRulesMPSpawningBase::AddAvoidPOI() entity=%s (id=%d); avoidDistance=%f; startActive=%d", poiEntity ? poiEntity->GetName() : "NULL", entityId, avoidDistance, startActive);
#endif

	SPointOfInterest newPOI;
	newPOI.SetToAvoidEntity(entityId, avoidDistance*avoidDistance, startActive);
	m_pointsOfInterest.push_back(newPOI);
}

void CGameRulesMPSpawningBase::RemovePOI(EntityId entityId)
{
	TPointsOfInterest::iterator it;

#if DEBUG_NEW_SPAWNING
	IEntity *poiEntity = gEnv->pEntitySystem->GetEntity(entityId);
	CryLog("CGameRulesMPSpawningBase::RemovePOI() entity=%s; (id=%d)", poiEntity ? poiEntity->GetName() : "NULL", entityId);
#endif

	for (it=m_pointsOfInterest.begin(); it!=m_pointsOfInterest.end(); ++it)
	{
		SPointOfInterest &poi = *it;
		if (poi.m_entityId == entityId)
		{
			m_pointsOfInterest.erase(it);
			break;
		}
	}

	if (it == m_pointsOfInterest.end())
	{
		CryLog("CGameRulesMPSpawningBase::RemovePOI() failed to find POI for entity");
	}


}

void CGameRulesMPSpawningBase::EnablePOI(EntityId entityId)
{
	TPointsOfInterest::iterator it;

#if DEBUG_NEW_SPAWNING
	IEntity *poiEntity = gEnv->pEntitySystem->GetEntity(entityId);
	CryLog("CGameRulesMPSpawningBase::EnablePOI() entity=%s; (id=%d)", poiEntity ? poiEntity->GetName() : "NULL", entityId);
#endif

	for (it=m_pointsOfInterest.begin(); it!=m_pointsOfInterest.end(); ++it)
	{
		SPointOfInterest &poi = *it;
		if (poi.m_entityId == entityId)
		{
			poi.Enable();
			break;
		}
	}


	if (it == m_pointsOfInterest.end())
	{
		CryLog("CGameRulesMPSpawningBase::EnablePOI() failed to find POI for entity");
	}
}

void CGameRulesMPSpawningBase::DisablePOI(EntityId entityId)
{
	TPointsOfInterest::iterator it;

#if DEBUG_NEW_SPAWNING
	IEntity *poiEntity = gEnv->pEntitySystem->GetEntity(entityId);
	CryLog("CGameRulesMPSpawningBase::DisablePOI() entity=%s; (id=%d)", poiEntity ? poiEntity->GetName() : "NULL", entityId);
#endif

	for (it=m_pointsOfInterest.begin(); it!=m_pointsOfInterest.end(); ++it)
	{
		SPointOfInterest &poi = *it;
		if (poi.m_entityId == entityId)
		{
			poi.Disable();
			break;
		}
	}

	if (it == m_pointsOfInterest.end())
	{
		CryLog("CGameRulesMPSpawningBase::DisablePOI() failed to find POI for entity");
	}
}

void CGameRulesMPSpawningBase::ClRequestRevive(EntityId playerId)
{
	// no inherited call - not implemented in CGameRulesSpawningBase
	CryLog("CGameRulesMPSpawningBase::ClRequestRevive()");

	CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));

	if (pActor)
	{
		CryLogAlways("CGameRulesMPSpawningBase::RequestRevive() player (%s)", pActor->GetEntity()->GetName());

		// Check if the player can revive
		if ((pActor->GetHealth() <= 0) || (pActor->GetSpectatorState()==CActor::eASS_None))
		{
			CGameRules::EntityParams params;
			params.entityId = playerId;

			CryLog("CGameRulesMPSpawningBase::ClRequestRevive() Requesting revive via RMI ");
			if (gEnv->bServer)
				SvRequestRevive(playerId);
			else
				m_pGameRules->GetGameObject()->InvokeRMI(CGameRules::SvRequestRevive(), params, eRMI_ToServer);
		}
		else
		{
			CryLogAlways("CGameRulesMPSpawningBase::ClRequestRevive() Player %s is not dead", pActor->GetEntity()->GetName());
			CCCPOINT(PlayerState_CannotSpawnBecauseIsAlive);
		}
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase::ClRequestRevive() failed to find a playerEntity for player that died");
	}
}

void CGameRulesMPSpawningBase::SvRequestRevive(EntityId playerId)
{
	// no inherited call - not implemented in CGameRulesSpawningBase
	if (m_pGameRules->GetHostMigrationState() == CGameRules::eHMS_NotMigrating)
	{
		TPlayerDataMap::iterator it = m_playerValues.find(playerId);
		if (it != m_playerValues.end())
		{
			CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
			if (pActor)
			{
				if ((m_teamGame==(m_pGameRules->GetTeam(playerId)!=0)))
				{
					if ( (pActor->GetSpectatorState()==CActor::eASS_None) || ((pActor->GetSpectatorState()==CActor::eASS_Ingame) && (GetTime() - it->second.deathTime) > m_minTimeFromDeathTillRevive) )
					{
						PerformRevive(playerId, m_pGameRules->GetTeam(playerId));
						CCCPOINT(PlayerState_SvPerformRevive);
					}
					else
					{
						CryLog("CGameRulesMPSpawningBase::SvRequestRevive() not enough time has elapsed since death, NOT ok to revive");
						CCCPOINT(PlayerState_SvDoNotReviveTooSoonAfterDeath);
					}
				}
				else
				{
					CryLog("CGameRulesMPSpawningBase::SvRequestRevive() %s", m_teamGame?"team game trying to revive a player who doesn't have team":"non-team game trying to revive a player who has a team");
				}
			}
		}
	}
#ifndef _RELEASE
	else
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(playerId);
		CryLog("CGameRulesMPSpawningBase::SvRequestRevive() request received from %s but we're mid hostmigration, refusing", pEntity ? pEntity->GetName() : "<NULL>");
	}
#endif
}

void CGameRulesMPSpawningBase::PerformRevive(EntityId playerId, int teamId)
{
	// no inherited call - not implemented in CGameRulesSpawningBase
	CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));

	if (gEnv->bServer)
	{
		if (pActor)
		{
			CryLogAlways("CGameRulesMPSpawningBase::PerformRevive() player (%s) teamId=%d", pActor->GetEntity()->GetName(), teamId);
			// Check if the player can revive
			if (pActor->GetHealth() <= 0 || pActor->IsMigrating())
			{
				TPlayerDataMap::iterator it = m_playerValues.find(playerId);
				if (it != m_playerValues.end())
				{
					if (CheckMidRoundJoining(playerId))
					{
						float zOffset = 0.f;
						EntityId spawnId = GetSpawnLocation(pActor->GetEntityId(), zOffset);
						// If this is a migrating player, ignore the spawn location and use the actor's current location
						IEntity *pSpawnPoint = gEnv->pEntitySystem->GetEntity(pActor->IsMigrating() ? playerId : spawnId);
						if (pSpawnPoint)
						{
							CryLog("CGameRulesMPSpawningBase::PerformRevive, reviving player '%s'", pActor->GetEntity()->GetName());

							IGameRulesSpectatorModule*  specmod = m_pGameRules->GetSpectatorModule();

							if (specmod)
							{
								CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
								if (pPlayer)
								{
									pPlayer->SetSpectatorState(CActor::eASS_Ingame);

									specmod->ChangeSpectatorMode(pPlayer, CActor::eASM_None, 0, false);
								}
							}

							m_pGameRules->RevivePlayerMP(pActor, pSpawnPoint, teamId);
						}
						else
						{
							CryLogAlways("CGameRulesMPSpawningBase::PerformRevive() failed to find spawn point");
						}
					}
					else
					{
						CryLogAlways("CGameRulesMPSpawningBase::PerformRevive() failed because mid-round joins are disabled and we're currently mid-round (ingame) and the player to be spawned is not the server");
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesMPSpawningBase::PerformRevive() failed to find a playervalue for player %s. This should not happen", pActor->GetEntity()->GetName()));
				}
			}
			else
			{
				CryLogAlways("CGameRulesMPSpawningBase::PerformRevive() Player %s is not dead", pActor->GetEntity()->GetName());
			}
		}
		else
		{
			CryLogAlways("CGameRulesMPSpawningBase::PerformRevive() failed to find the actor to revive");
		}
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase::PerformRevive() being called when NOT a server. This is wrong!");
	}
}

void CGameRulesMPSpawningBase::HandleOptOutOfSpawn(EntityId entityId)
{
	m_pGameRules->SendRMI_SvAutoReviveOptOut(entityId, eRMI_ToServer);
	ClAutoReviveOptOut();
}

void CGameRulesMPSpawningBase::SvRecordReviveOptOut(EntityId entityId)
{
	assert(gEnv->bServer);
	TPlayerDataMap::iterator  it = m_playerValues.find(entityId);
	if (it != m_playerValues.end())
	{
		it->second.reviveOptOutTime = GetTime();
	}
	else
	{
		IEntity*  e = gEnv->pEntitySystem->GetEntity(entityId);
		CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesMPSpawningBase::RecordReviveOptOutTime() failed to find a playervalue for player %s, couldn't record opt-out time", (e?e->GetName():"!")));
	}
}

void CGameRulesMPSpawningBase::ClAutoReviveOptOut()
{
	m_localOptedOut = true;
}

EntityId CGameRulesMPSpawningBase::GetSpawnLocation(EntityId playerId, float &zOffset)
{
	EntityId id=0;
	
	zOffset = 0.0f;	// not being passed through to new spawning algorithms at the moment

	IEntity *playerEntity = gEnv->pEntitySystem->GetEntity(playerId);
	if (!playerEntity)
	{
		CryLog("ERROR: GetSpawnLocation() failed to find a playerEntity to work with. Returning.");
		return 0;
	}

	Vec3 deathPos=ZERO;

	TPlayerDataMap::iterator it = m_playerValues.find(playerId);
	if (it != m_playerValues.end())
	{
		deathPos = it->second.deathPos;
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesMPSpawningBase::GetSpawnLocation() failed to find a playervalue for player %s. This should not happen", playerEntity->GetName()));
	}

	CryLog("CGameRulesMPSpawningBase::GetSpawnLocation() player=%s; deathPos=%f, %f, %f; spawnModule params: teamGame=%d; teamSpawnsType=%d;", playerEntity->GetName(), deathPos.x, deathPos.y, deathPos.z, m_teamGame, m_teamSpawnsType);

	if (m_teamGame)
	{
		id = GetSpawnLocationTeamGame(playerId, deathPos);
	}
	else
	{
		id = GetSpawnLocationNonTeamGame(playerId, deathPos);
	}

	if (!id)
	{
#if DEBUG_SPAWNING_VISTABLE
		m_pGameRules->SendTextMessage(eTextMessageServer, "FAILED TO FIND A VALID SPAWN", eRMI_ToClientChannel, m_pGameRules->GetChannelId(playerId));
#endif

		CryLog("GetSpawnLocation() player=%s; has failed to find a valid spawn, so now calling old gamerules GetSpawnLocation() to find a spawn point", playerEntity->GetName());
		id=GetSpawnLocationOld(playerId, !m_teamGame, (m_teamSpawnsType==eTST_None), 0, g_pGameCVars->g_spawndeathdist, deathPos, &zOffset);
	}

	if (id)
	{
		if(m_pGameRules->IsSpawnUsed(id))
			zOffset += .5f;

		if (it != m_playerValues.end())
		{
			CryLog("CGameRulesMPSpawningBase::GetSpawnLocation() setting lastSpawnLocationId to %d\n", id);
			it->second.lastSpawnLocationId=id;
		}

		return id;
	}
	else
	{
		CryLog("GetSpawnLocation() player=%s; has failed to find a valid spawn itself and within old gamerules GetSpawnLocation(). Something is seriously broken", playerEntity->GetName());
	}

	return 0;
}

int CGameRulesMPSpawningBase::GetSpawnLocationTeam(EntityId spawnLocEid) const
{
	int  team;
	switch (m_teamSpawnsType)
	{
	case eTST_None:
	case eTST_Standard:
		{
			team = m_pGameRules->GetTeam(spawnLocEid);
			break;
		}
	case eTST_RoundSwap:
		{
			team = m_pGameRules->GetTeam(spawnLocEid);
			if (team > 0)
			{
				IGameRulesRoundsModule*  pRoundsModule = m_pGameRules->GetRoundsModule();
				CRY_ASSERT(pRoundsModule);
				if (pRoundsModule)
				{
					int  prim = pRoundsModule->GetPrimaryTeam();
					if (prim != 2)
					{
						CRY_ASSERT(team < 3);
						team = (3 - team);
					}
				}
			}
			break;
		}
	}
	return team;
}

//	TIA spawn behavior - ideal min distance is approximately a 6th of the distance between the 2 furthest apart spawn points
//	Note: Won't always find the 2 furthest apart spawn points but this is only an indication so it doesn't matter
//------------------------------------------------------------------------
float CGameRulesMPSpawningBase::GetMinEnemyDist(int teamId, bool log)
{
	TTeamMinEnemyDistMap::iterator it = m_minEnemyDistanceMap.find(teamId);
	float minEnemyDist=0.0f;

	if (it == m_minEnemyDistanceMap.end())
	{
		Vec3 minv(9999999999.f, 9999999999.f, 9999999999.f), maxv(-9999999999.f, -9999999999.f, -9999999999.f);
		
		int count=GetSpawnLocationCount()-1;
		for(; count>=0; --count)
		{
			EntityId entId = GetNthSpawnLocation(count);
			const IEntity *pEntity( gEnv->pEntitySystem->GetEntity(entId) );
			if(!pEntity)
				continue;

			if ((m_teamSpawnsType != eTST_None) && (GetSpawnLocationTeam(entId) != teamId))
				continue;

			const Vec3 pos(pEntity->GetWorldPos());
		
			minv.CheckMin(pos);
			maxv.CheckMax(pos);
		}
		
		Vec3 diff = maxv-minv;
		float dist=diff.GetLength();
		float oldDist=max(max(diff.x, diff.y), diff.z) / g_pGameCVars->g_spawn_enemyDistLevelSizeDivider;
		minEnemyDist=dist / g_pGameCVars->g_spawn_enemyDistLevelSizeDivider;
		// TODO have a different divider for uses spawns teams
		CryLogAlways("CGameRulesMPSpawningBase::GetMinEnemyDist(team=%d) calculated result as %f (oldDist=%f)\n", teamId, minEnemyDist, oldDist);
		
		m_minEnemyDistanceMap.insert(TTeamMinEnemyDistMap::value_type(teamId, minEnemyDist));
	}
	else
	{
		minEnemyDist = it->second;
		if (log)
		{
			CryLog("CGameRulesMPSpawningBase::GetMinEnemyDist(team=%d) returned already calculated result as %f", teamId, minEnemyDist);
		}
	}

	return minEnemyDist;
}

//------------------------------------------------------------------------
bool CGameRulesMPSpawningBase::TestSpawnLocationWithEnvironment(EntityId spawnLocationId, EntityId playerId, float offset, float height) const
{
	if (!spawnLocationId)
		return false;

	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	IPhysicalEntity *pPlayerPhysics=0;

	IEntity *pPlayer=gEnv->pEntitySystem->GetEntity(playerId);
	if (pPlayer)
		pPlayerPhysics=pPlayer->GetPhysics();

	static float r = 0.3f;
	primitives::sphere sphere;
	sphere.center = pSpawn->GetWorldPos();
	sphere.r = r;
	sphere.center.z+=offset+r;

	Vec3 end = sphere.center;
	end.z += height-2.0f*r;

	geom_contact *pContact = 0;
	float dst = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(sphere.type, &sphere, end-sphere.center, ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid|ent_living,
		&pContact, 0, (geom_colltype_player<<rwi_colltype_bit)|rwi_stop_at_pierceable, 0, 0, 0, &pPlayerPhysics, pPlayerPhysics?1:0);

	if(dst>0.001f)
		return false;
	else
		return true;
}

bool CGameRulesMPSpawningBase::IsSpawnLocationSafeFromExplosives(EntityId spawnLocationId) const
{
	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	const Vec3& centre = pSpawn->GetPos();
	const Vec3& radiusVec = Vec3(g_pGameCVars->g_spawn_explosiveSafeDist, g_pGameCVars->g_spawn_explosiveSafeDist, g_pGameCVars->g_spawn_explosiveSafeDist);
	const Vec3 boxMin = centre - radiusVec;
	const Vec3 boxMax = centre + radiusVec;
	bool safe=true;

	SProjectileQuery projQuery;
	projQuery.box = AABB(boxMin, boxMax);
	g_pGame->GetWeaponSystem()->QueryProjectiles(projQuery);

	for (int i=0; i<projQuery.nCount; i++)
	{
		IEntity* pEntity = projQuery.pResults[i];

		if(pEntity->GetClass() != CBullet::EntityClass)
		{
#if DEBUG_NEW_SPAWNING
			CryLog("CGameRulesMPSpawningBase::IsSpawnLocationSafeFromExplosives() has found a non-bullet entity %s within range %f", pEntity->GetName(), g_pGameCVars->g_spawn_explosiveSafeDist);
#endif
			safe=false;
			break;
		}
	}
	
	return safe;
}



//------------------------------------------------------------------------
bool CGameRulesMPSpawningBase::IsSpawnLocationSafe(EntityId playerId, EntityId spawnLocationId, float safeDistance, bool ignoreTeam, float zoffset) const
{
	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	if (safeDistance<=0.01f)
		return true;

	int playerTeamId = m_pGameRules->GetTeam(playerId);

	SEntityProximityQuery query;
	Vec3	c(pSpawn->GetWorldPos());
	float l(safeDistance*1.5f);
	float safeDistanceSq=safeDistance*safeDistance;

	query.box = AABB(Vec3(c.x-l,c.y-l,c.z-0.15f), Vec3(c.x+l,c.y+l,c.z+2.0f));
	query.nEntityFlags = 0;
	query.pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	gEnv->pEntitySystem->QueryProximity(query);

	bool result=true;

	if (zoffset<=0.0001f)
	{
		for (int i=0; i<query.nCount; i++)
		{
			EntityId entityId=query.pEntities[i]->GetId();
			if (playerId==entityId) // ignore self
				continue;

			IActor *pActor=(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId));

#if 0
			IActorComponent_OldActor* pOldActorCmp = pActor ? pActor->GetComponent<IActorComponent_OldActor>() : NULL;
			if (pOldActorCmp && pOldActorCmp->GetSpectatorMode()!=0) // ignore spectators
				continue;
#endif

			if (playerTeamId && playerTeamId==m_pGameRules->GetTeam(entityId)) // ignore team players on team games
			{
				if (pActor && (pActor->GetEntity()->GetWorldPos()-c).len2()<=safeDistanceSq) // only if they are not too close
				{
					result=false;
					break;
				}

				continue;
			}

			result=false;
			break;
		}
	}
	else
		result=TestSpawnLocationWithEnvironment(spawnLocationId, playerId, zoffset, 2.0f);

	return result;
}

//------------------------------------------------------------------------
bool CGameRulesMPSpawningBase::IsSpawnLocationFarEnough(EntityId spawnLocationId, float minDistance, const Vec3 &testPosition) const
{
	if (minDistance<=0.1f)
		return true;

	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	if ((pSpawn->GetWorldPos()-testPosition).len2()<minDistance*minDistance)
		return false;

	return true;
}

bool CGameRulesMPSpawningBase::DoesSpawnLocationRespectPOIs(EntityId spawnLocationId) const
{
	bool ok=true;

	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	TPointsOfInterest::const_iterator it;

	for (it=m_pointsOfInterest.begin(); it!=m_pointsOfInterest.end(); ++it)
	{
		const SPointOfInterest &poi = *it;
		
		if (poi.m_flags & EPOIFL_ENABLED)
		{
			bool done=false;

			IEntity *pPOI=gEnv->pEntitySystem->GetEntity(poi.m_entityId);
			if (pPOI)
			{
				Vec3 diff;
				float dist;
				
				diff = pPOI->GetPos() - pSpawn->GetPos();
				dist = diff.GetLengthSquared();

				switch (poi.m_state)
				{
					case EPOIS_AVOID:
						if (dist < poi.m_stateData.avoid.avoidDistance)
						{
#if DEBUG_NEW_SPAWNING
							CryLog("CGameRulesMPSpawningBase::DoesSpawnLocationRespectPOIs() found spawn %s is too close (%f) to avoid POI %s", pSpawn->GetName(), sqrtf(dist), pPOI->GetName());
#endif
							ok=false;
							done=true;
						}
						break;
					case EPOIS_ATTRACT:
						if (dist > poi.m_stateData.attract.attractDistance)
						{
#if DEBUG_NEW_SPAWNING
							CryLog("CGameRulesMPSpawningBase::DoesSpawnLocationRespectPOIs() found spawn %s is too far away (%f) from attract POI %s", pSpawn->GetName(), sqrtf(dist), pPOI->GetName());
#endif
							ok=false;
							done=true;
						}
						break;
					default:
						CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::DoesSpawnLocationRespectPOIs() found a POI (entity=%s) with unhandled state %d", pPOI->GetName(), poi.m_state));
						break;
				}
			}
			if (done)
			{
				break;
			}
		}
	}

	return ok;
}

void CGameRulesMPSpawningBase::DebugPOIs() const
{
	TPointsOfInterest::const_iterator it;

	for (it=m_pointsOfInterest.begin(); it!=m_pointsOfInterest.end(); ++it)
	{
		const SPointOfInterest &poi = *it;
		IEntity *pPOI=gEnv->pEntitySystem->GetEntity(poi.m_entityId);

		CryWatch("POI: entity=%s (%d); state=%d; enabled=%d", pPOI ? pPOI->GetName() : "NULL", poi.m_entityId, poi.m_state, poi.m_flags & EPOIFL_ENABLED ? 1 : 0);
	}	
}

// Considers ALL spawn locations - not suitable for base/non-base etc at the moment
EntityId CGameRulesMPSpawningBase::GetSpawnLocationFurthestAwayFromAnyPlayer(EntityId playerId, const Vec3 &deathPos, float minDistToDeathSqr) const
{
	EntityId bestPointId(0);
	float	bestEnemyDist(0);
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)

	for (TSpawnLocations::const_iterator it=m_spawnLocations.begin(); it!=m_spawnLocations.end(); ++it)
	{
		EntityId spawnId(*it);
		const IEntity *pSpawn( gEnv->pEntitySystem->GetEntity(spawnId));

		const Vec3 spawnPos(pSpawn->GetWorldPos());
		float	deathDistSqr( (spawnPos-deathPos).GetLengthSquared() );
		// Warning - atm the death position isn't being passed through, hardcoded to the origin
		if( deathDistSqr < minDistToDeathSqr )	// too close to death position 
			continue;
		if(!IsSpawnLocationSafe(playerId, spawnId, safeDistance, false, 0.f))
			continue;
		if (!IsSpawnLocationSafeFromExplosives(spawnId))
			continue;
		if (!DoesSpawnLocationRespectPOIs(spawnId))
			continue;
		float closeDist = m_pGameRules->GetClosestPlayerDistSqr(spawnId, playerId);
		if( bestEnemyDist > closeDist )
			continue;
		bestPointId = spawnId;
		bestEnemyDist = closeDist;
	}
	return bestPointId;
}

// playerTeamId < 0 means ignore spawnpoint teams
EntityId CGameRulesMPSpawningBase::GetSpawnLocationFurthestAwayFromEnemy(const TSpawnLocations &spawnLocations, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr)
{
	EntityId bestPointId(0);
	float	bestEnemyDist(0);
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)
	int absPlayerTeamId = abs(playerTeamId);
	int enemyTeamId(m_pGameRules->GetEnemyTeamId(absPlayerTeamId));
	float enemyRejectDistSqr = GetMinEnemyDist(absPlayerTeamId);
	enemyRejectDistSqr *= enemyRejectDistSqr;

	for (TSpawnLocations::const_iterator it=spawnLocations.begin(); it!=spawnLocations.end(); ++it)
	{
		EntityId spawnId(*it);
		const IEntity *pSpawn( gEnv->pEntitySystem->GetEntity(spawnId));
		int spawnTeam=GetSpawnLocationTeam(spawnId);
		if (playerTeamId > 0 && spawnTeam != absPlayerTeamId)
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s is NOT on player's side. playerTeamId=%d; spawnTeamId=%d; (teamSpawnsType=%d)", pSpawn->GetName(), playerTeamId, spawnTeam, m_teamSpawnsType);
#endif
			continue;
		}

		if (m_vistable.IsSpawnLocationVisibleByTeam(spawnId, enemyTeamId))
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s is visible by the enemy team %d", pSpawn->GetName(), enemyTeamId);
#endif
			continue;
		}

		const Vec3 spawnPos(pSpawn->GetWorldPos());
		float	deathDistSqr( (spawnPos-deathPos).GetLengthSquared() );
		// Warning - atm the death position isn't being passed through, hardcoded to the origin
		if( deathDistSqr < minDistToDeathSqr )	// too close to death position 
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s is too close to deathpos", pSpawn->GetName());
#endif
			continue;
		}
		if(!IsSpawnLocationSafe(playerId, spawnId, safeDistance, false, 0.f))
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s is NOT safe", pSpawn->GetName());
#endif
			continue;
		}

		if (!IsSpawnLocationSafeFromExplosives(spawnId))
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s is NOT safe from explosives", pSpawn->GetName());
#endif
			continue;
		}

		if (!DoesSpawnLocationRespectPOIs(spawnId))
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s does NOT respect POIs", pSpawn->GetName());
#endif
			continue;
		}

		float closeDist = m_pGameRules->GetClosestEnemyDistSqrToPlayer(absPlayerTeamId, spawnPos);
		if( bestEnemyDist > closeDist )
		{
#if DEBUG_NEW_SPAWNING
			CryLog("Found spawn point %s is closer (%fm) to an enemy than current bestEnemyDist (%fm) continuing", pSpawn->GetName(), sqrtf(closeDist), sqrtf(bestEnemyDist));
#endif
			continue;
		}
		bestPointId = spawnId;
		bestEnemyDist = closeDist;
	}

	if (bestEnemyDist < enemyRejectDistSqr)
	{
		CryLogAlways("CGameRulesMPSpawningBase::GetSpawnLocationFurthestAwayFromEnemy() failed to find a furthest away enemy (bestEnemyDist=%f) that is further away than minEnemyDist=%f", bestEnemyDist, enemyRejectDistSqr);
	}

#if DEBUG_NEW_SPAWNING
	IEntity *playerEntity = gEnv->pEntitySystem->GetEntity(playerId);
	if (bestPointId > 0)
	{
		IEntity *bestSpawnEntity = gEnv->pEntitySystem->GetEntity(bestPointId);
		CryLog("CGameRulesMPSpawningBase::GetSpawnLocationFurthestAwayFromEnemy() player %s succeeded to find spawnpoint %s which was %fm away from the nearest enemy. minEnemyDist=%f", playerEntity->GetName(), bestSpawnEntity->GetName(), sqrtf(bestEnemyDist), enemyRejectDistSqr);
	}
	else
	{
		CryLog("CGameRulesMPSpawningBase::GetSpawnLocationFurthestAwayFromEnemy() player %s failed to find a suitable spawnpoint close to any teammate", playerEntity->GetName());
	}
#endif // DEBUG_NEW_SPAWNING

	return bestPointId;
}

// playerTeamId < 0 means ignore spawnpoint teams
EntityId CGameRulesMPSpawningBase::GetSpawnLocationClosestToTeamMate(const TSpawnLocations &spawnLocations, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr)
{
	EntityId bestPointId(0);
	float	bestFriendDist(std::numeric_limits<float>::max());
	float bestClosestEnemyDist=0;
	int absPlayerTeamId = abs(playerTeamId);
	float enemyRejectDistSqr = GetMinEnemyDist(absPlayerTeamId);
	enemyRejectDistSqr *= enemyRejectDistSqr;
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)
	int enemyTeamId(m_pGameRules->GetEnemyTeamId(absPlayerTeamId));
	float lastKillerMinDist=g_pGameCVars->g_spawn_lastKillerDist * g_pGameCVars->g_spawn_lastKillerDist;
	float lastSpawnMinDist=g_pGameCVars->g_spawn_lastSpawnDist * g_pGameCVars->g_spawn_lastSpawnDist;

	TPlayerDataMap::iterator jit = m_playerValues.find(playerId);
	CRY_ASSERT_TRACE(jit != m_playerValues.end(), ("failed to find a playervalue for player - this should NOT happen!"));

	for (TSpawnLocations::const_iterator it=spawnLocations.begin(); it!=spawnLocations.end(); ++it)
	{
		EntityId spawnId(*it);
		const IEntity *pSpawn( gEnv->pEntitySystem->GetEntity(spawnId));
		int spawnTeam=GetSpawnLocationTeam(spawnId);
		if (playerTeamId>0 && spawnTeam != playerTeamId)
		{
			//CryLog("Found spawn point %s is NOT on player's side", pSpawn->GetName());
			continue;
		}

		if (m_vistable.IsSpawnLocationVisibleByTeam(spawnId, enemyTeamId))
		{
			//CryLog("Found spawn point %s is visible by the enemy team %d", pSpawn->GetName(), enemyTeamId);
			continue;
		}

		const Vec3 spawnPos( pSpawn->GetWorldPos() );
		float	deathDistSqr( (spawnPos-deathPos).GetLengthSquared() );
		if( deathDistSqr < minDistToDeathSqr )	// too close to death position
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s is too close to deathpos", pSpawn->GetName());
#endif
			continue;
		}

		if (jit != m_playerValues.end() && jit->second.lastKillerId)
		{
			const IEntity *pLastKiller( gEnv->pEntitySystem->GetEntity(jit->second.lastKillerId));

			if (pLastKiller)
			{
				Vec3 diff;
				diff=spawnPos-pLastKiller->GetPos();

				if (diff.GetLengthSquared() < lastKillerMinDist)
				{
#if DEBUG_NEW_SPAWNING
					//CryLog("Found spawn point %s is too close to lastKiller %s", pSpawn->GetName(), pLastKiller->GetName());
#endif
					continue;
				}
			}

			const IEntity *pLastSpawnLocation( gEnv->pEntitySystem->GetEntity(jit->second.lastSpawnLocationId));
			if (pLastSpawnLocation)
			{
				Vec3 diff;
				diff=spawnPos - pLastSpawnLocation->GetPos();
				if (diff.GetLengthSquared() < lastSpawnMinDist)
				{
#if DEBUG_NEW_SPAWNING
					//CryLog("Found spawn point %s is too close to last spawn location %s", pSpawn->GetName(), pLastSpawnLocation->GetName());
#endif
					continue;		
				}
			}
		}

		if(!IsSpawnLocationSafe(playerId, spawnId, safeDistance, false, 0.f))
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s is NOT safe", pSpawn->GetName());
#endif
			continue;
		}

		if (!IsSpawnLocationSafeFromExplosives(spawnId))
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s is NOT safe from explosives", pSpawn->GetName());
#endif
			continue;
		}

		if (!DoesSpawnLocationRespectPOIs(spawnId))
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s does NOT respect POIs", pSpawn->GetName());
#endif
			continue;
		}
		
		float closestEnemyDistSqr = m_pGameRules->GetClosestEnemyDistSqrToPlayer(absPlayerTeamId, spawnPos);
		if( closestEnemyDistSqr < enemyRejectDistSqr )
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s is too close to enemy (%fm)", pSpawn->GetName(), sqrtf(closestEnemyDistSqr));
#endif
			continue;
		}
		float closeDist = m_pGameRules->GetClosestTeamMateDistSqrToPlayer(absPlayerTeamId, spawnPos);
		if( closeDist < .3f )	// too close - skip this point
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s is too close to a teammate (%fm)", pSpawn->GetName(), sqrtf(closeDist));
#endif
			continue;
		}
		if( bestFriendDist < closeDist )
		{
#if DEBUG_NEW_SPAWNING
			//CryLog("Found spawn point %s is not as close (%fm) as current bestFriendDist (%fm)", pSpawn->GetName(), sqrtf(closeDist), sqrtf(bestFriendDist));
#endif
			continue;
		}
		bestPointId = spawnId;
		bestFriendDist = closeDist;
		bestClosestEnemyDist = closestEnemyDistSqr;
	}

#if DEBUG_NEW_SPAWNING
	IEntity *playerEntity = gEnv->pEntitySystem->GetEntity(playerId);
	if (bestPointId > 0)
	{
		IEntity *bestSpawnEntity = gEnv->pEntitySystem->GetEntity(bestPointId);
		//CryLog("CGameRulesMPSpawningBase::GetSpawnLocationClosestToTeamMate() player %s succeeded to find spawnpoint %s which was %fm away from the nearest friend; and %fm away from the nearest enemy - which was less than %fm the enemyRejectDistance for this level", playerEntity->GetName(), bestSpawnEntity->GetName(), sqrtf(bestFriendDist), sqrtf(bestClosestEnemyDist), sqrtf(enemyRejectDistSqr));
	}
	else
	{
		//CryLog("CGameRulesMPSpawningBase::GetSpawnLocationClosestToTeamMate() player %s failed to find a suitable spawnpoint close to any teammate", playerEntity->GetName());
	}
#endif // DEBUG_NEW_SPAWNING

	return bestPointId;
}

EntityId CGameRulesMPSpawningBase::GetSpawnLocationNonTeamGame(EntityId playerId, const Vec3 &deathPos)
{
	EntityId id=0;
	float minDistToDeathSqr(g_pGameCVars->g_spawndeathdist*g_pGameCVars->g_spawndeathdist);

	if (m_pGameRules->GetLivingPlayerCount() > 0)
	{
		id = GetSpawnLocationFurthestAwayFromAnyPlayer(playerId, deathPos, minDistToDeathSqr);
	}
	else
	{
		id = GetRandomSpawnLocation(m_spawnLocations, 0);
	}
	return id;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpawningBase::GetSpawnLocationOld(EntityId playerId, bool ignoreTeam, bool includeNeutral, EntityId groupId, float minDistToDeath, const Vec3 &deathPos, float *pZOffset) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	const TSpawnLocations *locations=0;

#if 0
	if (groupId)
	{
		TSpawnGroupMap::const_iterator it=m_spawnGroups.find(groupId);
		if (it==m_spawnGroups.end())
			return 0;

		locations=&it->second;
	}
	else
		locations=&m_spawnLocations;
#endif

	if (groupId)
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawningBase::GetSpawnLocationOld() - doesn't handle groups at the moment");
	}

	locations=&m_spawnLocations;

	if (locations->empty())
		return 0;

	static TSpawnLocations candidates;
	candidates.resize(0);

	int playerTeamId=m_pGameRules->GetTeam(playerId);
	for (TSpawnLocations::const_iterator it=locations->begin(); it!=locations->end(); ++it)
	{
		int  spawnTeam = GetSpawnLocationTeam(*it);

		if ((ignoreTeam || playerTeamId==spawnTeam) || (!spawnTeam && includeNeutral))
			candidates.push_back(*it);
	}

	int n=candidates.size();
	if (!n)
		return 0;

	int s=Random(n);
	int i=s;

	float mdtd=minDistToDeath;
	float zoffset=0.0f;
	float safeDistance=0.82f; // this is 2x the radius of a player collider (capsule/cylinder)

	while (!IsSpawnLocationSafe(playerId, candidates[i], safeDistance, ignoreTeam, zoffset) ||
		!IsSpawnLocationFarEnough(candidates[i], mdtd, deathPos))
	{
		++i;

		if (i==n)
			i=0;

		if (i==s)
		{
			if (mdtd>0.0f && mdtd==minDistToDeath)// if we have a min distance to death point
				mdtd*=0.5f;													// half it and see if it helps
			else if (mdtd>0.0f)										// if that didn't help
				mdtd=0.0f;													// ignore death point
			else if (zoffset==0.0f)								// nothing worked, so we'll have to resort to height offset
				zoffset=2.0f;
			else
				return 0;														// can't do anything else, just don't spawn and wait for the situation to clear up

			s=Random(n);													// select a random starting point again
			i=s;
		}
	}

	if (pZOffset)
		*pZOffset=zoffset;

	return candidates[i];
}

//------------------------------------------------------------------------
void CGameRulesMPSpawningBase::ReviveAllPlayers(bool isReset)
{
	if (isReset)
		return;

	if (m_teamGame && (m_teamSpawnsType != eTST_None))
	{
		// We're reviving everyone using team spawners so there is no need to check for visibility etc
		for (int teamId = 1; teamId < 3; ++ teamId)
		{
			CGameRules::TPlayers players;
			m_pGameRules->GetTeamPlayers(teamId, players);

			TSpawnLocations spawnPoints;
			GetTeamSpawnLocations(teamId, spawnPoints);

			int numPlayers = players.size();
			for (int i = 0; i < numPlayers; ++ i)
			{
				EntityId playerId = players[i];
				IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
				EntityId spawnId = 0;

				if (pActor)
				{
					int numPoints = spawnPoints.size();
					if (!numPoints)
					{
						numPoints = GetTeamSpawnLocations(teamId, spawnPoints);
						if (!numPoints)
						{
							CRY_ASSERT_MESSAGE(false, "Designer ASSERT: No team spawn points");
							return;
						}
					}

					int index = cry_rand() % numPoints;
					spawnId = spawnPoints[index];
					
					TSpawnLocations::iterator it = spawnPoints.begin() + index;
					spawnPoints.erase(it);

					IEntity *pSpawnPoint = gEnv->pEntitySystem->GetEntity(spawnId);
					if (pSpawnPoint)
					{
						IGameRulesSpectatorModule*  specmod = m_pGameRules->GetSpectatorModule();
						if (specmod)
						{
							CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
							if (pPlayer)
							{
								pPlayer->SetSpectatorState(CActor::eASS_Ingame);
								specmod->ChangeSpectatorMode(pPlayer, CActor::eASM_None, 0, false);
							}
						}

						m_pGameRules->RevivePlayerMP(pActor, pSpawnPoint, teamId);
					}
				}
			}
		}
	}
	else
	{
		CGameRules::TPlayers players;
		m_pGameRules->GetPlayers(players);

		int numPlayers = players.size();
		for (int i = 0; i < numPlayers; ++ i)
		{
			EntityId playerId = players[i];
			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
			if (pActor)
			{
				float zOffset = 0.f;
				EntityId spawnId = GetSpawnLocation(playerId, zOffset);

				IEntity *pSpawnPoint = gEnv->pEntitySystem->GetEntity(spawnId);
				if (pSpawnPoint)
				{
					IGameRulesSpectatorModule*  specmod = m_pGameRules->GetSpectatorModule();
					if (specmod)
					{
						CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
						if (pPlayer)
						{
							pPlayer->SetSpectatorState(CActor::eASS_Ingame);
							specmod->ChangeSpectatorMode(pPlayer, CActor::eASM_None, 0, false);
						}
					}

					m_pGameRules->RevivePlayerMP(pActor, pSpawnPoint, m_pGameRules->GetTeam(playerId));
				}
			}
		}
	}
}

//------------------------------------------------------------------------
int CGameRulesMPSpawningBase::GetTeamSpawnLocations( int teamId, TSpawnLocations &results )
{
	int numSpawnLocations = m_spawnLocations.size();
	results.reserve(numSpawnLocations);
	for (int i = 0; i < numSpawnLocations; ++ i)
	{
		EntityId spawnId = m_spawnLocations[i];
		if (GetSpawnLocationTeam(spawnId) == teamId)
		{
			results.push_back(spawnId);
		}
	}

	return results.size();
}

//------------------------------------------------------------------------
void CGameRulesMPSpawningBase::HostMigrationInsertIntoReviveQueue( EntityId playerId, float timeTillRevive )
{
	TPlayerDataMap::iterator it = m_playerValues.begin();
	for (; it != m_playerValues.end(); ++it)
	{
		if (it->first == playerId)
		{
			// Fake death time so that the player is auto revived at the correct time
			float gameTime = GetTime();
			it->second.deathTime = (gameTime + timeTillRevive) - m_timeFromDeathTillAutoRevive;
			// Ensure opt out time is lower than death time
			it->second.reviveOptOutTime = it->second.deathTime - 1000.f;
			break;
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesMPSpawningBase::CheckMidRoundJoining(EntityId playerId)
{
	bool  ok = false;
	IGameRulesStateModule*  stateModule = g_pGame->GetGameRules()->GetStateModule();
	if (m_allowMidRoundJoining ||
		  (gEnv->bServer && (playerId == g_pGame->GetIGameFramework()->GetClientActorId())) ||
			(!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)))
	{
		ok = true;
	}
	return ok;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpawningBase::GetRandomSpawnLocation(const TSpawnLocations &spawnLocations, int playerTeamId) const
{
	const int numSpawnPoints = spawnLocations.size();

	std::vector<EntityId> elegibleSpawnPoints;
	elegibleSpawnPoints.reserve(numSpawnPoints);

	for (int i = 0; i < numSpawnPoints; ++ i)
	{
		EntityId spawnId = spawnLocations[i];

		if (playerTeamId > 0)
		{
			int spawnTeam = GetSpawnLocationTeam(spawnId);
			if (spawnTeam != playerTeamId)
			{
				continue;
			}
		}
		elegibleSpawnPoints.push_back(spawnId);
	}

	EntityId spawnId = 0;
	const int numElegibleSpawnPoints = elegibleSpawnPoints.size();
	if (numElegibleSpawnPoints)
	{
		// Should really use cry_rand() but unfortunately that isn't particularly random! (it is never given a random seed)
		int index = int(gEnv->pTimer->GetFrameStartTime().GetValue() % numElegibleSpawnPoints);
		spawnId = elegibleSpawnPoints[index];
		CryLog("CGameRulesMPSpawningBase::GetRandomSpawnLocation() picked spawn point eid=%i (%i of %i)", spawnId, index, numElegibleSpawnPoints);
	}
	else
	{
		CryLog("CGameRulesMPSpawningBase::GetRandomSpawnLocation() failed to find elegible spawn point");
	}

	return spawnId;
}
