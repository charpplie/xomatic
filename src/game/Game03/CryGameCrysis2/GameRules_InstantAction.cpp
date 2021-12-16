/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
		- 18:6:2009 : Created by Colin Gulliver

*************************************************************************/
#include "StdAfx.h"
#include "GameRules_InstantAction.h"
#include "Player.h"
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "HUD/UIBattleLog.h"
#include "GameRulesTypes.h"

#define ASSERT_IS_CLIENT if (!gEnv->bClient) { assert(gEnv->bClient); return; }
#define ASSERT_IS_SERVER if (!gEnv->bServer) { assert(gEnv->bServer); return; }
#define ASSERT_IS_SERVER_RETURN_FALSE if (!gEnv->bServer) { assert(gEnv->bServer); return false; }

#define START_TIMER_LENGTH			15.f
#define NEXT_LEVEL_TIMER_LENGTH		12000.f

CGameRules_InstantAction::CGameRules_InstantAction(void)
{
	CryLogAlways("IA constructor");
}

CGameRules_InstantAction::~CGameRules_InstantAction(void)
{
	if (gEnv->bServer)
	{
		TPlayers players;
		GetPlayers(players);
		for (TPlayers::iterator it = players.begin(); it != players.end(); ++it)
		{
			SvCleanUpPlayer(*it);
		}
	}
}

bool CGameRules_InstantAction::Init( IGameObject * pGameObject )
{
	if (!CGameRules_SinglePlayer::Init(pGameObject))
		return false;

	m_state = EGR_PreGame;
	m_starting = false;
	m_forceInGame = false;

	m_gameRulesType = EGR_INSTANTACTION;

	return true;
}

void CGameRules_InstantAction::SvOnClientConnect( int channelId, bool reset, const char *name )
{
	CryLogAlways("CGameRules_InstantAction::SvOnClientConnect");
	ASSERT_IS_SERVER;

	Vec3 pos(0.f, 0.f, 0.f);
	Ang3 angles(0.f, 0.f, 0.f);

	EntityId locationId = (m_spectatorModule ? m_spectatorModule->GetInterestingSpectatorLocation() : 0);
	if (locationId)
	{
		IEntity *location = gEnv->pEntitySystem->GetEntity(locationId);
		if (location)
		{
			pos = location->GetWorldPos();
			angles = location->GetWorldAngles();
		}
	}

	// Check if there's a migrating player for this channel
	GetMigratedPlayerLocation(channelId, pos, angles);

	IActor *player(0);
	if (name && name[0] != 0)
	{
		player = SpawnPlayer(channelId, name, "Player", pos, angles);
	}
	else
	{
		player = SpawnPlayer(channelId, "Nomad", "Player", pos, angles);
	}

	if (!player)
	{
		CryLogAlways("Failed to spawn player");
		return;
	}

	CryLogAlways("Spawned player (%s) at {%f,%f,%f}", name, pos.x, pos.y, pos.z);

	SvSetupPlayer(player->GetEntityId());

	// Check if there's a migrating player for this channel
	SetupMigratedPlayer(channelId);

	if (!reset)
	{
		if (m_spectatorModule)
			m_spectatorModule->ChangeSpectatorMode(player, 2, 0, false);

		if (!m_pGameFramework->IsChannelOnHold(channelId))
		{
			SvResetScore(player->GetEntityId());
			//TODO: self.otherClients:ClClientConnect(channelId, player:GetName(), false);
		}
		else
		{
			//TODO: self.otherClients:ClClientConnect(channelId, player:GetName(), true);
		}
	}
	else
	{
		if (!m_pGameFramework->IsChannelOnHold(channelId))
		{
			SvResetScore(player->GetEntityId());
		}

		int  specMode;
	 	if (m_spectatorModule && m_spectatorModule->GetModeFromChannelSpectatorMap(channelId, &specMode))
		{
			int teamId = GetChannelTeam(channelId);

			if (specMode == 0 || teamId != 0)
			{
				SetTeam(teamId, player->GetEntityId()); // make sure the player's got a team before reviving
				SvRevivePlayer(player->GetEntityId());
			}
			else
			{
				m_spectatorModule->SvOnChangeSpectatorMode(player->GetEntityId(), specMode, 0, true);
			}
		}
	}

	UI8Params params((uint8)m_state);
	GetGameObject()->InvokeRMI(ClStateChanged(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, channelId);
}

void CGameRules_InstantAction::SvRevivePlayer(EntityId playerId)
{
#if 0
	Vec3 deathPos(ZERO);
	TPlayerDataMap::iterator it = m_playerValues.find(playerId);
	if (it != m_playerValues.end())
	{
		deathPos = it->second.deathPos;
	}
#endif

	float zOffset=0.0f;
	EntityId locationId = GetSpawnLocationNew(playerId, zOffset);
	if (locationId)
	{
		IEntity *location = gEnv->pEntitySystem->GetEntity(locationId);
		if (location)
		{
			Vec3 pos = location->GetWorldPos();
			Ang3 angles = location->GetWorldAngles();

			pos.z += zOffset;

			CryLogAlways("Reviving player %i", playerId);
			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
			if (pActor)
			{
				CryLogAlways("Revived player (%s) at {%f,%f,%f}", pActor->GetEntity()->GetName(), pos.x, pos.y, pos.z);
				RevivePlayer(pActor, pos, angles, 0, true);
			}
		}
		else
		{
			CryLogAlways("Failed to find spawn location");
		}
	}
	else
	{
		CryLogAlways("Failed to find spawn location");
	}
}

void CGameRules_InstantAction::SvOnClientDisconnect( int channelId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnClientDisconnect");
	ASSERT_IS_SERVER;

	if (IActor *pActor = GetActorByChannelId(channelId))
	{
		SvCleanUpPlayer(pActor->GetEntityId());
	}

	if (m_spectatorModule)
	{
		m_spectatorModule->FindAndRemoveFromChannelSpectatorMap(channelId);
	}
}

void CGameRules_InstantAction::SvOnClientEnteredGame( int channelId, IEntity *player, bool reset, bool loadingSaveGame )
{
	CryLogAlways("CGameRules_InstantAction::SvOnClientEnteredGame");
	ASSERT_IS_SERVER;

	bool onHold = gEnv->pGame->GetIGameFramework()->IsChannelOnHold(channelId);
	if (!onHold && !reset)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(player->GetId());
		if (pActor)
		{
			if (m_spectatorModule)
			{
				//TODO: Make player spectate until they join (once we have a join button)
				m_spectatorModule->ChangeSpectatorMode(static_cast<CActor*>(pActor), 0, 0, false);
			}
		}
	}
}

void CGameRules_InstantAction::SvOnStartLevel()
{
	CryLogAlways("CGameRules_InstantAction::SvOnStartLevel");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnStartGame()
{
	CryLogAlways("CGameRules_InstantAction::SvOnStartGame");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnKill( EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type )
{
	CryLogAlways("CGameRules_InstantAction::SvOnKill");
	ASSERT_IS_SERVER;

	if (CActor *pTarget = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetId)))
	{
		int deaths = 0;
		GetSynchedEntityValue(targetId, EIA_KEY_DEATHS, deaths);
		++ deaths;
		SetSynchedEntityValue(targetId, EIA_KEY_DEATHS, deaths);

		TPlayerDataMap::iterator it = m_playerValues.find(targetId);
		if (it != m_playerValues.end())
		{
			it->second.deathTime = GetServerTime();
			it->second.deathPos = pTarget->GetEntity()->GetWorldPos();
		}

		CryLogAlways("Player is dead, death count=%i", deaths);

		if (CActor *pShooter = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId)))
		{
			if (targetId == shooterId)
			{
				CryLogAlways("Suicide");
			}
			else
			{
				int kills = 0;
				GetSynchedEntityValue(shooterId, EIA_KEY_KILLS, kills);
				++ kills;
				SetSynchedEntityValue(shooterId, EIA_KEY_KILLS, kills);

				CryLogAlways("%s has killed %s, kill count=%i", pShooter->GetEntity()->GetName(), pTarget->GetEntity()->GetName(), kills);

				if (m_state == EGR_InGame)
				{
					SvCheckPlayerScoreLimit(shooterId, kills);
				}
			}
		}
	}
}

void CGameRules_InstantAction::SvOnHit( const HitInfo *hit )
{
	CryLogAlways("CGameRules_InstantAction::SvOnHit");
	ASSERT_IS_SERVER;

}

bool CGameRules_InstantAction::SvOnFreeze( EntityId targetId, EntityId shooterId, EntityId weaponId, float value )
{
	CryLogAlways("CGameRules_InstantAction::SvOnFreeze");
	ASSERT_IS_SERVER_RETURN_FALSE;

	return true;
}

void CGameRules_InstantAction::SvOnExplosion( const ExplosionInfo *explosion )
{
	CryLogAlways("CGameRules_InstantAction::OnExplosion");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnChangeTeam( EntityId playerId, int teamId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnChangeTeam");
	ASSERT_IS_SERVER;

}


void CGameRules_InstantAction::SvOnSetTeam( EntityId playerId, int teamId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnSetTeam");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnItemPickedUp( EntityId itemId, EntityId actorId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnItemPickedUp");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnItemDropped( EntityId itemId, EntityId actorId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnItemDropped");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnAddTaggedEntity( EntityId shooterId, EntityId targetId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnAddTaggedEntity");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnVehicleDestroyed( EntityId vehicleId )
{
	CryLogAlways("CGameRules_InstantAction::SvOnVehicleDestroyed");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnVehicleSubmerged( EntityId vehicleId, float ratio )
{
	CryLogAlways("CGameRules_InstantAction::SvOnVehicleSubmerged");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnTimer( int timerId, int64 msec )
{
	CryLogAlways("CGameRules_InstantAction::SvOnTimer");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvOnUpdate( float frameTime )
{
	ASSERT_IS_SERVER;

	switch (m_state)
	{
	case EGR_PreGame:
		if (SvPlayerCountOk())
		{
			if (!m_starting)
			{
				m_starting = true;
				ResetGameStartTimer(START_TIMER_LENGTH);
				CryLogAlways("CGameRules_InstantAction::SvOnUpdate: Player count is now ok, starting restart timer");
			}
		}
		else if (m_starting)
		{
			m_starting = false;
			ResetGameStartTimer(-1);
			CryLogAlways("CGameRules_InstantAction::SvOnUpdate: Not enough players, stopping restart timer");
		}

		if (m_starting)
		{
			if (GetRemainingStartTimer() <= 0.f)
			{
				CryLogAlways("CGameRules_InstantAction::SvOnUpdate: Restart timer finished");
				TPlayers players;
				GetPlayers(players);
				for (TPlayers::iterator it = players.begin(); it != players.end(); ++it)
				{
					SvCleanUpPlayer(*it);
				}
				SvGotoState(EGR_Reset);
				m_forceInGame = true;
				ResetEntities();
			}
		}
		break;
	case EGR_InGame:
		SvCheckTimeLimit();
		break;
	case EGR_PostGame:
		if (m_stateChangeTime + NEXT_LEVEL_TIMER_LENGTH < GetServerTime())
		{
			CryLogAlways("CGameRules_InstantAction::SvOnUpdate: Starting next level");
			SvGotoState(EGR_Reset);
			NextLevel();
		}
		break;
	case EGR_Reset:
		break;
	}
}

void CGameRules_InstantAction::SvOnReset()
{
	CryLogAlways("CGameRules_InstantAction::SvOnReset()");
	if (m_forceInGame || SvPlayerCountOk())
	{
		CryLogAlways("Switching to InGame state");
		SvGotoState(EGR_InGame);
		m_forceInGame = false;
	}
	else
	{
		CryLogAlways("Switching to PreGame state");
		SvGotoState(EGR_PreGame);
	}
}

bool CGameRules_InstantAction::SvPlayerCountOk()
{
	bool preGame = (m_state == EGR_PreGame);
	int totalPlayers = GetPlayerCount(preGame);
	int spectators = GetSpectatorCount(preGame);
	if (totalPlayers - spectators < g_pGameCVars->g_minplayerlimit)
	{
		return false;
	}
	return true;
}

void CGameRules_InstantAction::SvRestartGame( bool forceInGame )
{
	CryLogAlways("CGameRules_InstantAction::SvRestartGame");
	ASSERT_IS_SERVER;

}

void CGameRules_InstantAction::SvCheckPlayerScoreLimit( EntityId playerId, int kills )
{
	CryLogAlways("CGameRules_InstantAction::SvCheckPlayerScoreLimit");
	ASSERT_IS_SERVER;

	int fragLimit = g_pGame->GetCVars()->g_fraglimit;
	int fragLead = g_pGame->GetCVars()->g_fraglead;

	if (fragLimit > 0 && kills >= fragLimit)
	{
		if (fragLead > 1)
		{
			CGameRules::TPlayers players;
			GetPlayers(players);
			for(CGameRules::TPlayers::iterator it=players.begin();it!=players.end();++it)
			{
				if (*it == playerId)
					continue;
				int otherKills = 0;
				GetSynchedEntityValue(*it, EIA_KEY_KILLS, otherKills);
				if (otherKills + fragLead > kills)
					return;
			}
		}

		CryLogAlways("player has won");
		SvOnGameEnd(playerId, EGER_ScoreLimitReached);
	}
}

void CGameRules_InstantAction::SvResetScore(EntityId playerId)
{
	SetSynchedEntityValue(playerId, EIA_KEY_KILLS, 0);
	SetSynchedEntityValue(playerId, EIA_KEY_DEATHS, 0);
	SetSynchedEntityValue(playerId, EIA_KEY_HEADSHOTS, 0);
}

void CGameRules_InstantAction::SvCheckTimeLimit()
{
	ASSERT_IS_SERVER;

	if (IsTimeLimited() && (GetRemainingGameTime() <= 0.f))
	{
		// Game has ended
		TPlayers players;
		GetPlayers(players);

		int maxScore = -1;
		int minDeaths = 0;
		EntityId maxId = 0;
		bool draw = false;

		for (TPlayers::const_iterator it = players.begin(); it != players.end(); ++it)
		{
			int kills = 0;
			int deaths = 0;
			GetSynchedEntityValue(*it, EIA_KEY_KILLS, kills);
			GetSynchedEntityValue(*it, EIA_KEY_DEATHS, deaths);

			// Game is won by the player with the highest score, if two or more players share this score then the 
			// one with the least deaths wins, if this is also shared then the game is a draw
			if (kills > maxScore)
			{
				maxScore = kills;
				minDeaths = deaths;
				maxId = *it;
				draw = false;
			}
			else if (kills == maxScore)
			{
				if (minDeaths > deaths)
				{
					minDeaths = deaths;
					maxId = *it;
					draw = false;
				}
				else if (minDeaths == maxScore)
				{
					draw = true;
				}
			}
		}

		if (!maxId)
		{
			draw = true;
		}

		if (draw)
		{
			// No winner
			SvOnGameEnd(0, EGER_TimeLimitReached);
		}
		else
		{
			// Winner = maxId
			SvOnGameEnd(maxId, EGER_TimeLimitReached);
		}
	}
}

void CGameRules_InstantAction::SvOnGameEnd( unsigned int winnerId, EGameEndReason reason )
{
	CryLogAlways("CGameRules_InstantAction::SvOnGameEnd, winner=%i, reason=%i", winnerId, reason);

	if (winnerId)
	{
		EntityParams params(winnerId);
		GetGameObject()->InvokeRMI(ClVictory(), params, eRMI_ToAllClients);
	}
	else
	{
		NoParams params;
		GetGameObject()->InvokeRMI(ClNoWinner(), params, eRMI_ToAllClients);
	}

	SvGotoState(EGR_PostGame);

	OnEndGame();
}



void CGameRules_InstantAction::ClOnConnect()
{
	CryLogAlways("CGameRules_InstantAction::ClOnConnect");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnDisconnect( EDisconnectionCause cause, const char *desc )
{
	CryLogAlways("CGameRules_InstantAction::ClOnDisconnect");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnActorAction( IEntity *player, const ActionId& action, int activationMode, float value )
{
	ASSERT_IS_CLIENT;

	if (CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(player->GetId())))
	{
		//TODO: Use actual key!
		if (pActor->GetHealth() <= 0 && !stricmp(action.c_str(), "jump") && (activationMode == 1))
		{
			EntityParams params;
			params.entityId = player->GetId();

			CryLogAlways("Calling request revive");
			GetGameObject()->InvokeRMI(SvRequestRevive(), params, eRMI_ToServer);
		}
	}
}

void CGameRules_InstantAction::ClOnStartLevel()
{
	CryLogAlways("CGameRules_InstantAction::ClOnStartLevel");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnStartGame()
{
	CryLogAlways("CGameRules_InstantAction::ClOnStartGame");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnRevive( EntityId playerId, const Vec3 &pos, const Quat &rot, int teamId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnRevive");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnReviveInVehicle( EntityId playerId, EntityId vehicleId, int seatId, int teamId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnReviveInVehicle");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnKill( EntityId targetId, EntityId shooterId, int weaponClassId, float damage, int material, int hit_type )
{
	CryLogAlways("CGameRules_InstantAction::ClOnKill");
	ASSERT_IS_CLIENT;

	static char weaponClassName[129]={0};
	m_pGameFramework->GetNetworkSafeClassName(weaponClassName, 128, weaponClassId);
	OnKillMessage(targetId, shooterId, weaponClassName, damage, material, hit_type);

#if 0
	if (CUIBattleLog::Exists())
	{
		SBattleLogMessageInfo info;
		info.hitType = hit_type;
		info.shooterId = shooterId;
		info.targetId = targetId;

		CUIBattleLog::GetInstance()->NewBattleLogMessage(info);
	}
#endif
}

void CGameRules_InstantAction::ClOnHit( const HitInfo *hit )
{
	CryLogAlways("CGameRules_InstantAction::ClOnHit");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnFreeze( EntityId targetId, EntityId shooterId, EntityId weaponId, float value )
{
	CryLogAlways("CGameRules_InstantAction::ClOnFreeze");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnExplosion( const ExplosionInfo *explosion )
{
	CryLogAlways("CGameRules_InstantAction::ClOnExplosion");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnChangeTeam( EntityId playerId, int teamId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnChangeTeam");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnSetTeam( EntityId playerId, int teamId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnSetTeam");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnItemPickedUp( EntityId itemId, EntityId actorId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnItemPickedUp");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnItemDropped( EntityId itemId, EntityId actorId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnItemDropped");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnAddTaggedEntity( EntityId shooterId, EntityId targetId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnAddTaggedEntity");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnVehicleDestroyed( EntityId vehicleId )
{
	CryLogAlways("CGameRules_InstantAction::ClOnVehicleDestroyed");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnVehicleSubmerged( EntityId vehicleId, float ratio )
{
	CryLogAlways("CGameRules_InstantAction::ClOnVehicleSubmerged");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnTimer( int timerId, int64 msec )
{
	CryLogAlways("CGameRules_InstantAction::ClOnTimer");
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClOnUpdate( float frameTime )
{
	ASSERT_IS_CLIENT;

}

void CGameRules_InstantAction::ClRequestSpawnGroup( EntityId spawnGroupId )
{
	CryLogAlways("CGameRules_InstantAction::ClRequestSpawnGroup");
	ASSERT_IS_CLIENT;

}

IMPLEMENT_RMI(CGameRules_InstantAction, ClKill)
{
	CryLogAlways("CGameRules_InstantAction:ClKill");
	ClOnKill(params.targetId, params.shooterId, params.weaponClassId, params.damage, params.material, params.hit_type);
	return true;
}

IMPLEMENT_RMI(CGameRules_InstantAction, ClVictory)
{
	CryLogAlways("CGameRules_InstantAction:ClVictory");
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(params.entityId);
	if (pEntity)
	{
		CryLogAlways("%s has won", pEntity->GetName());
		bool isLocalActor = (m_pGameFramework->GetClientActorId() == params.entityId);
		//TODO: display end game message on hud
		if (isLocalActor)
		{
			CryLogAlways("You won");
		}
		else
		{
			CryLogAlways("You lost");
		}
	}
	return true;
}

IMPLEMENT_RMI(CGameRules_InstantAction, ClNoWinner)
{
	CryLogAlways("CGameRules_InstantAction:ClNoWinner");
	//TODO: display "draw game" message on hud
	return true;
}

void CGameRules_InstantAction::SvGotoState(EGR_GameState newState)
{
	CryLogAlways("CGameRules_InstantAction:SvGotoState(), newState=%i", newState);
	m_state = newState;
	m_stateChangeTime = GetServerTime();

	switch (m_state)
	{
	case EGR_PreGame:
		break;
	case EGR_InGame:
		ResetGameTime();
		SvResetPlayers();
		break;
	case EGR_PostGame:
		break;
	}

	UI8Params params((uint8) newState);
	GetGameObject()->InvokeRMI(ClStateChanged(), params, eRMI_ToAllClients|eRMI_NoLocalCalls);
}

IMPLEMENT_RMI(CGameRules_InstantAction, ClStateChanged)
{
	CryLogAlways("CGameRules_InstantAction:ClStateChanged, state changed to %i", params.data);
	m_state = (EGR_GameState) params.data;
	m_stateChangeTime = GetServerTime();
	return true;
}

void CGameRules_InstantAction::SvReviveAllPlayers()
{
	TPlayers players;
	GetPlayers(players);
	for (TPlayers::iterator it = players.begin(); it != players.end(); ++it)
	{
		CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*it));
		if (pActor)
		{
			if (pActor->GetSpectatorMode() == 0)
			{
				SvRevivePlayer(*it);
			}
		}
	}
}

void CGameRules_InstantAction::SvResetPlayers()
{
	SvReviveAllPlayers();

	TPlayers players;
	GetPlayers(players);
	for (TPlayers::iterator it = players.begin(); it != players.end(); ++it)
	{
		SvResetScore(*it);
	}
}

void CGameRules_InstantAction::SvSetupPlayer( EntityId playerId )
{
	m_playerValues.insert(TPlayerDataMap::value_type(playerId, SPlayerData()));
}

void CGameRules_InstantAction::SvCleanUpPlayer( EntityId playerId )
{
	TPlayerDataMap::iterator it = m_playerValues.find(playerId);
	if (it != m_playerValues.end())
	{
		m_playerValues.erase(it);
	}
}
