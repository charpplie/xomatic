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
#include "GameRulesMPSimpleSpawning.h"
#include "GameRules.h"

CGameRulesMPSimpleSpawning::CGameRulesMPSimpleSpawning()
{
}

CGameRulesMPSimpleSpawning::~CGameRulesMPSimpleSpawning()
{

}

EntityId CGameRulesMPSimpleSpawning::GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos)
{
	EntityId bestPointId=0;

	int playerTeamId;
	int absPlayerTeamId;
	int playerTeamSize;
	float minDistToDeathSqr(g_pGameCVars->g_spawndeathdist*g_pGameCVars->g_spawndeathdist);
//	int enemyTeamId(m_pGameRules->GetEnemyTeamId(playerTeamId));

	if (m_teamSpawnsType != eTST_None)
	{
		playerTeamId=m_pGameRules->GetTeam(playerId);
	}
	else
	{
		playerTeamId=-m_pGameRules->GetTeam(playerId);
	}
	if (m_pGameRules->GetLivingPlayerCount() > 0)
	{
		absPlayerTeamId=abs(playerTeamId);
		playerTeamSize=(m_pGameRules->GetTeamPlayerCount(absPlayerTeamId, false));

		CRY_FIXME( 24, 09, 2009, "[SPAWNING|GAMEMODES] Why is this getting called before PlayerIdSet()?" );

		if (playerTeamSize > 1)
		{
			CryLog("CGameRulesMPSimpleSpawning::GetSpawnLocationTeamGame() - Have players already on our team");
			bestPointId = GetSpawnLocationClosestToTeamMate(m_spawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		}
		else
		{
			CryLog("CGameRulesMPSimpleSpawning::GetSpawnLocationTeamGame() - Only one player on our team - maximising enemy distance");
			bestPointId = GetSpawnLocationFurthestAwayFromEnemy(m_spawnLocations, playerId, playerTeamId, deathPos, minDistToDeathSqr);
		}
	}
	else
	{
		CryLog("CGameRulesMPSimpleSpawning::GetSpawnLocationTeamGame() - First player in game - finding random edge spawn");
		bestPointId = GetRandomSpawnLocation(m_spawnLocations, playerTeamId);
	}

	return bestPointId;
}

