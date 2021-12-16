/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
		- 24:6:2009 : Created by Colin Gulliver

*************************************************************************/
#include "StdAfx.h"
#include "GameRules_TeamInstantAction.h"
#include "Player.h"
#include "Actor.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "Game.h"
#include "GameCVars.h"

#define ASSERT_IS_CLIENT if (!gEnv->bClient) { assert(gEnv->bClient); return; }
#define ASSERT_IS_SERVER if (!gEnv->bServer) { assert(gEnv->bServer); return; }
#define ASSERT_IS_SERVER_RETURN_FALSE if (!gEnv->bServer) { assert(gEnv->bServer); return false; }

CGameRules_TeamInstantAction::CGameRules_TeamInstantAction(void)
{
	CryLogAlways("TIA constructor");
}

CGameRules_TeamInstantAction::~CGameRules_TeamInstantAction(void)
{
}

bool CGameRules_TeamInstantAction::Init( IGameObject * pGameObject )
{
	if (!CGameRules_InstantAction::Init(pGameObject))
		return false;

	CreateTeam("tan");
	CreateTeam("black");

	m_gameRulesType = EGR_TEAMINSTANTACTION;

	return true;
}

void CGameRules_TeamInstantAction::SvRevivePlayer(EntityId playerId)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	if (pActor)
	{
		//TODO: This is temp code, automatically select a team since we can't do it through the non-existent HUD!
		int teamId = GetTeam(playerId);
		if (teamId == 0)
		{
			if (GetTeamPlayerCount(1, false) > GetTeamPlayerCount(2, false))
			{
				teamId = 2;
			}
			else
			{
				teamId = 1;
			}
			SetTeam(teamId, playerId);
			CryLogAlways("Auto selected team %i for player (%s)", teamId, pActor->GetEntity()->GetName());
		}

#if 0
		Vec3 deathPos(ZERO);
		TPlayerDataMap::iterator it = m_playerValues.find(playerId);
		if (it != m_playerValues.end())
		{
			deathPos = it->second.deathPos;
		}
#endif

		float zOffset=0.0f;
		IEntity* pLocation = NULL;
		uint32 teamID = 0;

		if (pActor->IsMigrating())
		{
			pLocation = gEnv->pEntitySystem->GetEntity(playerId);
			teamID = GetTeam(playerId);
		}
		else
		{
			//TODO: Select a spawn location using Jim's new spawning code
			EntityId locationId = GetSpawnLocationNew(playerId, zOffset);
			if (locationId)
			{
				pLocation = gEnv->pEntitySystem->GetEntity(locationId);
			}
		}

		if (pLocation)
		{
			Vec3 pos = pLocation->GetWorldPos();
			Ang3 angles = pLocation->GetWorldAngles();
			pos.z += zOffset;

			CryLogAlways("Reviving player %i", playerId);
			RevivePlayer(pActor, pos, angles, teamID, true);

			pActor->SetMigrating(false);
		}
		else
		{
			CryLogAlways("Failed to find spawn location");
		}
	}
}

void CGameRules_TeamInstantAction::SvOnKill( EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type )
{
	CryLogAlways("CGameRules_TeamInstantAction::SvOnKill");
	ASSERT_IS_SERVER;

	int deaths = 0;
	GetSynchedEntityValue(targetId, EIA_KEY_DEATHS, deaths);
	++ deaths;
	SetSynchedEntityValue(targetId, EIA_KEY_DEATHS, deaths);

	SvIncreasePlayerScore(targetId, -1);

	TPlayerDataMap::iterator it = m_playerValues.find(targetId);
	if (it != m_playerValues.end())
	{
		it->second.deathTime = GetServerTime();
	}

	CryLogAlways("Player is dead, death count=%i", deaths);

	CActor *pTarget = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetId));
	CActor *pShooter = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId));
	if (pTarget && pShooter)
	{
		TTIAPlayerDataMap::iterator it = m_tiaPlayerValues.find(targetId);
		if (it != m_tiaPlayerValues.end())
		{
			float earliestTimeAllowed = GetServerTime() - 10000.f;

			std::vector<SAttackerStats>::const_iterator stats = it->second.attackerStats.begin();
			for(; stats != it->second.attackerStats.end(); ++ stats)
			{
				if ((stats->hitTime > earliestTimeAllowed) && (stats->shooterId != shooterId))
				{
					CActor *assistShooter = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(stats->shooterId));
					if (assistShooter)
					{
						SvIncreasePlayerScore(stats->shooterId, 1);
						EntityParams params(targetId);
						GetGameObject()->InvokeRMI(ClAssistKill(), params, eRMI_ToClientChannel, assistShooter->GetChannelId());
					}
				}
			}

			it->second.attackerStats.clear();
		}

		SvAssignKillPoints(targetId, shooterId);
	}
}

int CGameRules_TeamInstantAction::GetTeamScore(int teamId)
{
	int score = 0;
	int key = (teamId == 1 ? ETIA_KEY_TEAM_SCORE_1 : ETIA_KEY_TEAM_SCORE_2);

	GetSynchedGlobalValue(key, score);

	return score;
}

void CGameRules_TeamInstantAction::SvIncreaseTeamScore(int teamId, int increase)
{
	int newScore = GetTeamScore(teamId) + increase;
	int key = (teamId == 1 ? ETIA_KEY_TEAM_SCORE_1 : ETIA_KEY_TEAM_SCORE_2);

	SetSynchedGlobalValue(key, newScore);

	CryLogAlways("CGameRules_TeamInstantAction::SvIncreaseTeamScore, team %i now has %i points", teamId, newScore);

	SvCheckTeamScore(teamId);
}

void CGameRules_TeamInstantAction::SvCheckTeamScore(int teamId)
{
	int newScore = GetTeamScore(teamId);

	if (m_state == EGR_InGame)
	{
		int fragLimit = g_pGame->GetCVars()->g_fraglimit;
		int fragLead = g_pGame->GetCVars()->g_fraglead;

		if (fragLimit > 0 && newScore >= fragLimit)
		{
			if (fragLead > 1)
			{
				int otherScore = GetTeamScore(3 - teamId);
				if (otherScore + fragLead > newScore)
				{
					return;
				}
			}

			CryLogAlways("team %i has won", teamId);
			SvOnGameEnd(teamId, EGER_ScoreLimitReached);
		}
	}
}

void CGameRules_TeamInstantAction::SvOnGameEnd( unsigned int winnerId, EGameEndReason reason )
{
	CryLogAlways("CGameRules_TeamInstantAction::SvOnGameEnd, winner=%i, reason=%i", winnerId, reason);

	if (winnerId)
	{
		UI8Params params(winnerId);
		GetGameObject()->InvokeRMI(ClTeamVictory(), params, eRMI_ToAllClients);
	}
	else
	{
		NoParams params;
		GetGameObject()->InvokeRMI(ClNoWinner(), params, eRMI_ToAllClients);
	}

	SvGotoState(EGR_PostGame);

	OnEndGame();
}

IMPLEMENT_RMI(CGameRules_TeamInstantAction, ClTeamVictory)
{
	bool isLocalTeam = (GetTeam(m_pGameFramework->GetClientActorId()) == params.data);
	
	//TODO: display end game message on hud
	if (isLocalTeam)
	{
		CryLogAlways("You won");
	}
	else
	{
		CryLogAlways("You lost");
	}

	return true;
}

void CGameRules_TeamInstantAction::SvResetTeamScore( int teamId )
{
	int key = (teamId == 1 ? ETIA_KEY_TEAM_SCORE_1 : ETIA_KEY_TEAM_SCORE_2);

	SetSynchedGlobalValue(key, 0);
}

void CGameRules_TeamInstantAction::SvOnStartGame()
{
	CGameRules_InstantAction::SvOnStartGame();

	SvResetTeamScore(1);
	SvResetTeamScore(2);
}

void CGameRules_TeamInstantAction::SvRestartGame( bool forceInGame )
{
	CGameRules_InstantAction::SvRestartGame(forceInGame);

	SvResetTeamScore(1);
	SvResetTeamScore(2);
}

void CGameRules_TeamInstantAction::SvGotoState( EGR_GameState newState )
{
	CGameRules_InstantAction::SvGotoState(newState);

	if (newState == EGR_InGame)
	{
		SvResetTeamScore(1);
		SvResetTeamScore(2);
	}
}

void CGameRules_TeamInstantAction::SvCheckTimeLimit()
{
	ASSERT_IS_SERVER;

	if (IsTimeLimited() && (GetRemainingGameTime() <= 0.f))
	{
		int team1Score = GetTeamScore(1);
		int team2Score = GetTeamScore(2);

		if (team1Score > team2Score)
		{
			// Team 1 wins
			SvOnGameEnd(1, EGER_TimeLimitReached);
		}
		else if (team1Score < team2Score)
		{
			// Team 2 wins
			SvOnGameEnd(2, EGER_TimeLimitReached);
		}
		else
		{
			// No winner
			SvOnGameEnd(0, EGER_TimeLimitReached);
		}
	}
}

bool CGameRules_TeamInstantAction::SvPlayerCountOk()
{
	for (int i = 1; i < 3; ++ i)
	{
		int teamPlayers = GetTeamChannelCount(i, true);
		if (teamPlayers < g_pGameCVars->g_minteamlimit)
		{
			return false;
		}
	}
	return true;
}

void CGameRules_TeamInstantAction::SvOnChangeTeam( EntityId playerId, int teamId )
{
	ASSERT_IS_SERVER;

	if (teamId != GetTeam(playerId))
	{
		CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
		if (pActor)
		{
			if (pActor->GetHealth() > 0 && pActor->GetSpectatorMode() == CActor::eASM_None)
			{
				KillPlayer(pActor, true, true, playerId, 0, 0, 0, 0, Vec3(ZERO));
			}

			SetTeam(teamId, playerId);

			CGameRules*  pGameRules = g_pGame->GetGameRules();
			IGameRulesSpectatorModule*  specmod = (pGameRules ? pGameRules->GetSpectatorModule() : NULL);

			if (specmod)
				specmod->ChangeSpectatorMode(pActor, CActor::eASM_None, 0, false);

			SvRevivePlayer(playerId);
		}
	}
}

void CGameRules_TeamInstantAction::SvResetScore( EntityId playerId )
{
	CGameRules_InstantAction::SvResetScore(playerId);

	SetSynchedEntityValue(playerId, ETIA_KEY_TEAM_KILLS, 0);
	SetSynchedEntityValue(playerId, ETIA_KEY_PLAYER_SCORE, 0);
}

void CGameRules_TeamInstantAction::SvIncreasePlayerScore( EntityId playerId, int increase )
{
	if (m_state == EGR_InGame)
	{
		int score = 0;
		GetSynchedEntityValue(playerId, ETIA_KEY_PLAYER_SCORE, score);
		score += increase;
		SetSynchedEntityValue(playerId, ETIA_KEY_PLAYER_SCORE, score);
		CryLogAlways("CGameRules_TeamInstantAction::SvIncreasePlayerScore, playerId(%i) now has %i points", playerId, score);
	}
}

void CGameRules_TeamInstantAction::SvOnHit( const HitInfo *hit )
{
	CGameRules_InstantAction::SvOnHit(hit);

	if (hit->targetId != hit->shooterId)
	{
		CActor *pTarget = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hit->targetId));
		CActor *pShooter = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hit->shooterId));

		// If both target and shooter are players, record the hit time for use calculating assist points
		if (pTarget && pShooter)
		{
			TTIAPlayerDataMap::iterator valuesIt = m_tiaPlayerValues.find(hit->targetId);
			if (valuesIt != m_tiaPlayerValues.end())
			{
				bool found = false;
				std::vector<SAttackerStats>::iterator statsIt = valuesIt->second.attackerStats.begin();
				for (; statsIt != valuesIt->second.attackerStats.end(); ++ statsIt)
				{
					if (statsIt->shooterId == hit->shooterId)
					{
						statsIt->hitTime = GetServerTime();
						found = true;
						break;
					}
				}
				if (!found)
				{
					valuesIt->second.attackerStats.push_back(SAttackerStats(hit->shooterId, GetServerTime()));
				}
			}
		}
	}
}

void CGameRules_TeamInstantAction::SvSetupPlayer( EntityId playerId )
{
	CGameRules_InstantAction::SvSetupPlayer(playerId);

	m_tiaPlayerValues.insert(TTIAPlayerDataMap::value_type(playerId, STIAPlayerData()));
}

void CGameRules_TeamInstantAction::SvCleanUpPlayer( EntityId playerId )
{
	CGameRules_InstantAction::SvCleanUpPlayer(playerId);

	TTIAPlayerDataMap::iterator it = m_tiaPlayerValues.find(playerId);
	if (it != m_tiaPlayerValues.end())
	{
		m_tiaPlayerValues.erase(it);
	}
}

void CGameRules_TeamInstantAction::ClOnSetTeam( EntityId playerId, int teamId )
{
	//TODO: Sort out hud silhouettes (if we're still using them and once we have a hud)
}

IMPLEMENT_RMI(CGameRules_TeamInstantAction, ClAssistKill)
{
	//TODO: Display hud message + points for the assist kill
	CryLogAlways("CGameRules_TeamInstantAction::ClAssistKill()");
	return true;
}

void CGameRules_TeamInstantAction::SvAssignKillPoints( EntityId targetId, EntityId shooterId )
{
	if (targetId != shooterId)
	{
		int shooterTeamId = GetTeam(shooterId);
		if (shooterTeamId != GetTeam(targetId))
		{
			SvIncreaseSynchedEntityValue(shooterId, EIA_KEY_KILLS, 1);
			SvIncreasePlayerScore(shooterId, 3);

			SvIncreaseTeamScore(GetTeam(shooterId), 1);
		}
		else
		{
			SvIncreaseSynchedEntityValue(shooterId, ETIA_KEY_TEAM_KILLS, 1);
			SvIncreasePlayerScore(shooterId, -3);
		}
	}
}
