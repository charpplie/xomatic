/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 30:09:2009 : Created by James Bamford

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesMPHQFrontlineSpawning.h"
#include "GameRules.h"
#include "Game.h"
#include "HUD/HUD.h"
#include "GameCVars.h"
#include "Actor.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "Player.h"
#include "Utility/CryWatch.h"

CGameRulesMPHQFrontlineSpawning::CGameRulesMPHQFrontlineSpawning()
{
	m_isHQSpawningCompatible = false;
}

CGameRulesMPHQFrontlineSpawning::~CGameRulesMPHQFrontlineSpawning()
{

}

void CGameRulesMPHQFrontlineSpawning::Init(XmlNodeRef xml)
{
	inherited::Init(xml);

	if (!xml->getAttr("isHQSpawningCompatible", m_isHQSpawningCompatible))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPHQFrontlineSpawning::Init() failed to find valid isHQSpawningCompatible param");
	}

	//CryLog("CGameRulesMPHQFrontlineSpawning::Init() set params to isHQSpawningCompatible=%d", m_isHQSpawningCompatible);
}

void CGameRulesMPHQFrontlineSpawning::Update(float frameTime)
{
	inherited::Update(frameTime);

#if DEBUG_NEW_SPAWNING
	if (g_pGameCVars->g_debug_spawning_visually)
	{
		VisuallyTestFrontlineSpawnMode();
	}
#endif // DEBUG_NEW_SPAWNING
}

void CGameRulesMPHQFrontlineSpawning::AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest)
{
	inherited::AddSpawnLocation(location, isBaseSpawn, doVisTest);

	if (isBaseSpawn)
	{
		stl::push_back_unique(m_baseSpawnLocations, location);
		//CryLog("num baseSpawnLocations now =%d", m_baseSpawnLocations.size());
	}
	else
	{
		stl::push_back_unique(m_nonBaseSpawnLocations, location);
		//CryLog("num nonBaseSpawnLocations now =%d", m_nonBaseSpawnLocations.size());
	}
}

void CGameRulesMPHQFrontlineSpawning::RemoveSpawnLocation(EntityId id, bool isBaseSpawn)
{
	inherited::RemoveSpawnLocation(id, isBaseSpawn);

	if (isBaseSpawn)
	{
		stl::find_and_erase(m_baseSpawnLocations, id);
		//CryLog("num baseSpawnLocations now =%d", m_baseSpawnLocations.size());
	}
	else
	{
		stl::find_and_erase(m_nonBaseSpawnLocations, id);
		//CryLog("num nonBaseSpawnLocations now =%d", m_nonBaseSpawnLocations.size());
	}
}



int CGameRulesMPHQFrontlineSpawning::GetNumberOfBaseSpawnLocations(void) const
{
	return m_baseSpawnLocations.size();
}

int CGameRulesMPHQFrontlineSpawning::GetNumberOfNonBaseSpawnLocations(void) const
{
	return m_nonBaseSpawnLocations.size();
}

//------------------------------------------------------------------------
EntityId CGameRulesMPHQFrontlineSpawning::GetSpawnLocationHQMode(EntityId playerId, const Vec3 &deathPos)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);
	EntityId bestPointId(0);
	int playerTeamId(m_pGameRules->GetTeam(playerId));
	int playerTeamSize(m_pGameRules->GetTeamPlayerCount(playerTeamId, false));
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)
	float minDistToDeathSqr(g_pGameCVars->g_spawndeathdist*g_pGameCVars->g_spawndeathdist);
	int enemyTeamId(m_pGameRules->GetEnemyTeamId(playerTeamId));
	
	// TODO find a better alternative (or location) for CallScriptFunction() for general entities

	// the only player of the team - maximize enemy dist
	if(playerTeamSize<2)	
	{
		//CryLog("Only one player on our team - maximising enemy distance");

		bestPointId = GetSpawnLocationFurthestAwayFromEnemy(m_baseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		return bestPointId;
	}

	// have teammates in the game already
	//CryLog("Have players already on our team");

	bestPointId = GetSpawnLocationClosestToTeamMate(m_baseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
	return bestPointId;
}


int CGameRulesMPHQFrontlineSpawning::CalculateTeamFromEntity(int entityId) const
{
	int teamId=0;

#if DEBUG_NEW_SPAWNING
	if (g_pGameCVars->g_debug_spawning_visually)
	{
		string friendlyTeamClassName = g_pGameCVars->g_debug_spawning_visually_teamA->GetString();
		string enemyTeamClassName = g_pGameCVars->g_debug_spawning_visually_teamB->GetString();

		const IEntity *entity = gEnv->pEntitySystem->GetEntity(entityId);
		const char *entityClassName = entity->GetClass()->GetName();


		if (entityClassName == friendlyTeamClassName)
		{
			teamId = 1;	
		}
		else if (entityClassName == enemyTeamClassName)
		{
			teamId = 2;
		}
		else
		{
		//CryLog("CalculateTeamFromEntity() debugging spawning visually - unhandled entityClassName=%s", entityClassName);
		}
	}
	else
#endif // DEBUG_NEW_SPAWNING
	{
		teamId = m_pGameRules->GetTeam(entityId);
	}

	return teamId;
}

// outResultsGroup contains sorted conflict groups
// returns number of conflicts (groups with num entities > 1)
int CGameRulesMPHQFrontlineSpawning::CalculateConflicts(float entityRDCRadius, std::vector<EntityRDCGroup> *outResultGroups) const
{
	std::vector<EntityRDCElement> entityRDCElements;
	
	// Populate entityRDCElements
	IActorIteratorPtr actorIt = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	IActor *actor;
	
	while (actor=actorIt->Next())
	{
		if (m_pGameRules->IsDead(actor->GetEntityId()))
		{
			IEntity *entity = actor->GetEntity();

			if (entity)
			{
			//CryLog("CGameRulesMPHQFrontlineSpawning::CalculateConflicts() - skipping actor %s as it is dead!", entity->GetName());
			}
			else
			{
			//CryLog("CGameRulesMPHQFrontlineSpawning::CalculateConflicts() - skipping actor who is dead but has no entity!");
			}
		}
		else
		{
			int actorTeam = CalculateTeamFromEntity(actor->GetEntityId());
			if (actorTeam > 0)
			{
				EntityRDCElement ele;

				Vec3 worldPos = actor->GetEntity()->GetWorldPos();
				ele.Set(actor->GetEntityId(), worldPos, entityRDCRadius);
				entityRDCElements.push_back(ele);
			}
		}
	}

	PerformRDCOnElements(&entityRDCElements, outResultGroups);

	std::sort(outResultGroups->begin(), outResultGroups->end(), EntityRDCGroup::Compare);


	bool onlyValidGroups=true;
#if DEBUG_NEW_SPAWNING
	if (g_pGameCVars->g_debug_spawning_visually)
	{
		onlyValidGroups=false;	// calculate AABBs for all groups when visually debugging
	}
#endif // DEBUG_NEW_SPAWNING

	CalculateBoundsOfGroups(&entityRDCElements, outResultGroups, onlyValidGroups);

	int numConflicts=0;
	std::vector<EntityRDCGroup>::size_type sz = outResultGroups->size();
	for (int i=sz-1; i>=0; i--)
	{
		EntityRDCGroup &group = outResultGroups->at(i);
		group.entitiesContainsMixedTeams = false;

		if (group.entities.size() > 1)
		{
			std::vector<int>::size_type groupSz = group.entities.size();
			int groupTeam=-1;

			for (int j=0; j<groupSz; j++)
			{
				int entityIndex = group.entities[j];
				EntityRDCElement element = entityRDCElements.at(entityIndex);
				
				if (groupTeam >= 0)
				{
					int newGroupTeam = CalculateTeamFromEntity(element.entityId);

					// ignore any actors that we don't want to consider (player, gamerules, camera etc) they'll return a negative teamId
					if (newGroupTeam >= 0 && groupTeam != newGroupTeam)
					{
						group.entitiesContainsMixedTeams=true;
						numConflicts++;
						break;
					}
				}
				else
				{
					groupTeam = CalculateTeamFromEntity(element.entityId);
				}
			}
		}
		else
		{

#if DEBUG_NEW_SPAWNING
			if (!g_pGameCVars->g_debug_spawning_visually)	// don't break out if we're debugging visually
#endif // DEBUG_NEW_SPAWNING
			{
				break;	// we're a sorted list so no need to continue iterating
			}
		}
	}

	return numConflicts;
}

// playerTeamId < 0 means ignore spawnpoint teams
EntityId CGameRulesMPHQFrontlineSpawning::GetSpawnLocationClosestToConflict(const TSpawnLocations &spawnLocations, const std::vector<EntityRDCGroup> &conflicts, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr)
{
	EntityId bestPointId(0);
	float	bestConflictDist(std::numeric_limits<float>::max());
	const EntityRDCGroup *bestConflict = NULL; 
	float bestClosestEnemyDist=0;
	
	int absPlayerTeamId = abs(playerTeamId);
	float enemyRejectDistSqr = GetMinEnemyDist(absPlayerTeamId);
	enemyRejectDistSqr *= enemyRejectDistSqr;
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)
	int enemyTeamId(m_pGameRules->GetEnemyTeamId(absPlayerTeamId));
	std::vector<EntityRDCGroup>::size_type sz = conflicts.size();

	for (int i=sz-1; i>= 0; i--)
	{
		const EntityRDCGroup &conflict = conflicts.at(i);
		if (conflict.entities.size() > 1)
		{
			if (conflict.entitiesContainsMixedTeams)
			{
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
					if(!IsSpawnLocationSafe(playerId, spawnId, safeDistance, false, 0.f))
					{
#if DEBUG_NEW_SPAWNING
						//CryLog("Found spawn point %s is NOT safe", pSpawn->GetName());
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

					float	conflictDistSqr( (spawnPos-conflict.centre).GetLengthSquared() );
					if( bestConflictDist < conflictDistSqr )
					{
#if DEBUG_NEW_SPAWNING
						//CryLog("Found spawn point %s is not as close (%fm) as current best conflict distance (%fm)", pSpawn->GetName(), sqrtf(conflictDistSqr), sqrtf(bestConflictDist));
#endif
						continue;
					}

					bestPointId = spawnId;
					bestConflictDist = conflictDistSqr;
					bestConflict = &conflict;
					bestClosestEnemyDist = closestEnemyDistSqr;
				}
			}
		}
		else
		{
			break;
		}
	}
	
#if DEBUG_NEW_SPAWNING
	IEntity *playerEntity = gEnv->pEntitySystem->GetEntity(playerId);
	if (bestPointId > 0)
	{
		IEntity *bestSpawnEntity = gEnv->pEntitySystem->GetEntity(bestPointId);
		//CryLog("CGameRulesMPHQFrontlineSpawning::GetSpawnLocationClosestToConflict() player %s succeeded to find spawnpoint %s which was %fm away from the nearest conflict; and %fm away from the nearest enemy - which was less than %fm the enemyRejectDistance for this level", playerEntity->GetName(), bestSpawnEntity->GetName(), sqrt(bestConflictDist), sqrt(bestClosestEnemyDist), sqrt(enemyRejectDistSqr));
	}
	else
	{
		//CryLog("CGameRulesMPHQFrontlineSpawning::GetSpawnLocationClosestToConflict() player %s failed to find a suitable spawnpoint close to any conflict", playerEntity->GetName());
	}
#endif // DEBUG_NEW_SPAWNING
	
	return bestPointId;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPHQFrontlineSpawning::GetSpawnLocationFrontlineMode( EntityId playerId, const Vec3 &deathPos )
{
	EntityId bestPointId(0);

	int playerTeamId;
	int playerTeamSize; 
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)
	float minDistToDeathSqr(g_pGameCVars->g_spawndeathdist*g_pGameCVars->g_spawndeathdist);
	int enemyTeamId;

	if (m_teamSpawnsType != eTST_None)
	{
		playerTeamId=m_pGameRules->GetTeam(playerId);
	}
	else
	{
		playerTeamId=-m_pGameRules->GetTeam(playerId);
	}

	int absPlayerTeamId = abs(playerTeamId);

	playerTeamSize = m_pGameRules->GetTeamPlayerCount(absPlayerTeamId, false);
	enemyTeamId = m_pGameRules->GetEnemyTeamId(absPlayerTeamId);

//  Early unit test of sorts - superseded now by the visual testing
//	TestEntityClustering();
//	assert(0);

	// the only player of the team - maximize enemy dist
	if(playerTeamSize<2)	
	{
		//CryLog("Only one player on our team - maximising enemy distance");

		bestPointId = GetSpawnLocationFurthestAwayFromEnemy(m_baseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);

		if (!bestPointId)
		{
			//CryLog("failed to find a base spawn location furthest away from enemy, trying non-base spawn locations");
			bestPointId = GetSpawnLocationFurthestAwayFromEnemy(m_nonBaseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		}

		return bestPointId;
	}

	// Locate any conflicts happening
	std::vector<EntityRDCGroup> resultGroups;
	int numConflicts = CalculateConflicts(g_pGameCVars->g_spawn_conflict_clustering_radius, &resultGroups);

	if (numConflicts > 0)
	{
		// choose a random valid SP closest to a random conflict - considering ALL spawn locations including base spawns (any conflict is preferred to spawn near)
		//CryLog("GetSpawnLocationFrontlineMode() managed to find %d conflicts to consider", numConflicts);
		bestPointId = GetSpawnLocationClosestToConflict(m_spawnLocations, resultGroups, playerId, playerTeamId, deathPos, minDistToDeathSqr);
	}
	else
	{
		// choose any valid spawn closest to any team mate
		//CryLog("GetSpawnLocationFrontlineMode() did NOT find any conflicts - finding closest non-base spawn to any team mates");

		// consider non-base spawns first
		bestPointId = GetSpawnLocationClosestToTeamMate(m_nonBaseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		if (!bestPointId)
		{
			//CryLog("GetSpawnLocationFrontlineMode() did not manage to find a valid non-base spawn point, now considering base spawn points");
			bestPointId = GetSpawnLocationClosestToTeamMate(m_baseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		}
	}

	return bestPointId;
}

// cannot test against the right teams of spawn props as CGameRulesSystem::CreateGameRules() isn't called when in the editor
// to setup the teams
void CGameRulesMPHQFrontlineSpawning::VisuallyTestFrontlineSpawnMode(void)
{
#if DEBUG_NEW_SPAWNING
	EntityId bestPointId(0);

	int playerId = 0; // TODO shouldn't really need this we're using our own actors for each team in this mode
	int playerTeamId(0); //m_pGameRules->GetTeam(playerId)); // TODO shouldn't really need this we're using our own actors for each team in this mode
	int playerTeamSize(m_pGameRules->GetTeamPlayerCount(playerTeamId, false));
	float safeDistance=1.f;//0.82f; // this is 2x the radius of a player collider (capsule/cylinder)
	float minDistToDeathSqr(g_pGameCVars->g_spawndeathdist*g_pGameCVars->g_spawndeathdist);
	int enemyTeamId(m_pGameRules->GetEnemyTeamId(playerTeamId));
	Vec3 deathPos(0);

	// the only player of the team - maximize enemy dist
	// during the editor there are no teams as such. always do the conflict analysis
//	if(playerTeamSize<2)	
//	{
//	//CryLog("Only one player on our team - maximising enemy distance");
//
//		bestPointId = GetSpawnLocationFurthestAwayFromEnemy(m_baseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
//	}
//	else
//	{
		// Locate any conflicts happening
		std::vector<EntityRDCGroup> resultGroups;
		int numConflicts = CalculateConflicts(g_pGameCVars->g_spawn_conflict_clustering_radius, &resultGroups);

		if (numConflicts > 0)
		{
			// choose a random valid SP closest to a random conflict - considering ALL spawn locations including base spawns (any conflict is preferred to spawn near)
			//CryLog("VisuallyTestFrontlineSpawnMode() managed to find %d conflicts to consider", numConflicts);
			bestPointId = GetSpawnLocationClosestToConflict(m_spawnLocations, resultGroups, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		}
		else
		{
			// choose any valid spawn closest to any team mate
			//CryLog("VisuallyTestFrontlineSpawnMode() did NOT find any conflicts - finding closest non-base spawn to any team mates");

			// consider non-base spawns first
			bestPointId = GetSpawnLocationClosestToTeamMate(m_nonBaseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
			if (!bestPointId)
			{
				//CryLog("VisuallyTestFrontlineSpawnMode() did not manage to find a valid non-base spawn point, now considering base spawn points");
				bestPointId = GetSpawnLocationClosestToTeamMate(m_baseSpawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
			}
		}

#if ENABLE_GAMERULES_DEBUG_TEXT
		float color[4] = {1,1,1,1};
		gEnv->pRenderer->Draw2dLabel(50,GetDebugTextYPos(),2.0f,color,false,"Num Conflicts=%d", numConflicts);
#endif

//	}


	// setup renderflag
	SAuxGeomRenderFlags renderFlags;
	renderFlags.SetDepthWriteFlag(e_DepthWriteOff);
	renderFlags.SetDepthTestFlag(e_DepthTestOn);
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(renderFlags);

	// draw conflicts (and clusters)
	std::vector<EntityRDCGroup>::size_type sz = resultGroups.size();
	for (int i=sz-1; i>=0; i--)
	{
		EntityRDCGroup &group = resultGroups.at(i);
		ColorB color = ColorB(0, 64, 0, 1);

		if (group.entities.size() > 1)
		{
			if (group.entitiesContainsMixedTeams)
			{
				// is a conflict - draw in red
				color.r = 64;
				color.g = 0;
				color.b = 0;
			}
			else
			{
				// is NOT a true conflict draw in green (or not at all?)
			}

		}
		else
		{
			// render them all as it helps visualise - colour different again with a size of 1
			//break;	// we're a sorted list so no need to continue iterating
			color.r = 0; 
			color.g = 0;
			color.b = 64;
		}

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(group.myAABB, false, color, eBBD_Faceted);
	}

	// draw selected spawn point 
	const IEntity *pSpawn( gEnv->pEntitySystem->GetEntity(bestPointId));

	if (pSpawn)
	{
		// transparent
		renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
		gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(renderFlags);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(pSpawn->GetPos(), 2.0f, ColorF(0.0f, 0.5f, 0.0f, 0.4f), true);
	}
	else
	{
		//CryLog("VisuallyTestFrontlineSpawnMode() failed to find a best spawn entity");
	}

#endif // DEBUG_NEW_SPAWNING
}

EntityId CGameRulesMPHQFrontlineSpawning::GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos)
{
	EntityId id=0;
	int spawnMode=g_pGameCVars->g_debug_spawn_mode;

	const IActor *playerActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	if (playerActor)
	{
#if ENABLE_MP_SPAWNMODES
		const CPlayer* pPlayer = static_cast<const CPlayer*>(playerActor);
		spawnMode=pPlayer->GetSpawnModeType();

		//CryLog("CGameRulesMPHQFrontlineSpawning::GetSpawnLocationTeamGame() Player %s found - using their spawn mode type=%d", pPlayer->GetEntity()->GetName(), spawnMode);
#else
		assert(0);
#endif
	}
	else
	{
		//CryLog("CGameRulesMPHQFrontlineSpawning::GetSpawnLocationTeamGame() failed to find a valid playerActor, using cvar debugspawn mode=%d", spawnMode);
	}

	float zOffset=0.0f;

	//CryLog("CGameRulesMPHQFrontlineSpawning::GetSpawnLocationTeamGame() spawnMode=%d", spawnMode);
	switch (spawnMode)
	{
//		case 0:
//		//CryLog("debug spawn mode is 0 normal behaviour");
			//id = pGameRules->GetSpawnLocationTeam((EntityId)playerId.n, deathPos);	// actually doesn't respect teams on the spawn points themselves
//			id=pGameRules->GetSpawnLocation((EntityId)playerId.n, false, false, 0, 0.0f, deathPos, &zOffset);
//			break;
		case 1:
		{
			//CryLog("debug spawn mode is 1: HQ spawn mode");
			int numBaseSpawns = GetNumberOfBaseSpawnLocations();
			int numNonBaseSpawns = GetNumberOfNonBaseSpawnLocations();

			//CryLog("Num Base Spawns=%d; Num nonBaseSpawns=%d", numBaseSpawns, numNonBaseSpawns);

			// TODO - find a better way of getting the game rules than asking the HUD


#if 0
			bool gameModeIsHQCompatible=false;
			EHUDGAMERULES rules=pHUD->GetGameRules();
			if (rules==EHUD_POWERSTRUGGLE || rules==EHUD_POWERSTRUGGLELITE || rules==EHUD_CAPTURETHEFLAG || rules==EHUD_BOMBTHEBASE || rules==EHUD_EXTRACTION)	// probably doesn't need extraction here, only single spawns in bases per round anyway 
			{
				if (numBaseSpawns > 0)	// really need enough for each team but we're trying to detect a level that's had some base spawns setup
				{
					gameModeIsHQCompatible=true;
				}
				else
				{
					//CryLog("Valid game mode but there are no basespawns so aren't able to do HQ spawning");
				}
			}
#endif

			if (m_isHQSpawningCompatible)
			{
				id = GetSpawnLocationHQMode(playerId, deathPos);
			}
			else
			{
				//CryLog("spawn mode is HQ yet gamerules is not HQ compatible. Using Frontline spawning mode");
				id = GetSpawnLocationFrontlineMode(playerId, deathPos);
			}
			break;
		}
		case 2:
		{
			//CryLog("debug spawn mode is 2: Frontline spawn mode");
			int numBaseSpawns=GetNumberOfBaseSpawnLocations();
			int numNonBaseSpawns=GetNumberOfNonBaseSpawnLocations();

			//CryLog("Num Base Spawns=%d; Num nonBaseSpawns=%d", numBaseSpawns, numNonBaseSpawns);

			id = GetSpawnLocationFrontlineMode(playerId, deathPos);
			break;
		}
		default:
			//CryLog("unhandled spawn mode of %d", g_pGameCVars->g_debug_spawn_mode);
			break;
	}

	return id;
}





