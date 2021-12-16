/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle player scores and stats
	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Ben Johnson

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardPlayerStats.h"
#include "IXml.h"
#include "GameRules.h"
#include "IGameRulesStateModule.h"
#include "HUD/HUD.h"
#include "Utility/CryWatch.h"
#include "Player.h"

#if CRY_WATCH_ENABLED
#define WATCH_LV1				m_dbgWatchLvl<1  ? (NULL) : CryWatch
#define WATCH_LV1_ONLY	m_dbgWatchLvl!=1 ? (NULL) : CryWatch
#define WATCH_LV2				m_dbgWatchLvl<2  ? (NULL) : CryWatch
#else
#define WATCH_LV1
#define WATCH_LV1_ONLY
#define WATCH_LV2
#endif

const float CGameRulesStandardPlayerStats::GR_PLAYER_STATS_PING_UPDATE_INTERVAL = 1000.f;

CRY_FIXME(16,9,2009,"Need a way of displaying points earned on screen!");
#define DISPLAY_POINTS_ON_HUD			0

//-------------------------------------------------------------------------
CGameRulesStandardPlayerStats::CGameRulesStandardPlayerStats()
{
	m_lastUpdatedPings = 0.f;
	m_playerStats.reserve(24);
	m_dbgWatchLvl = 0;
}

//-------------------------------------------------------------------------
CGameRulesStandardPlayerStats::~CGameRulesStandardPlayerStats()
{
	CGameRules  *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterRevivedListener(this);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::Init( XmlNodeRef xml )
{
	if (!xml->getAttr("dbgWatchLvl", m_dbgWatchLvl))
		m_dbgWatchLvl = 0;

	CGameRules  *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->RegisterRevivedListener(this);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::Reset()
{
	m_lastUpdatedPings = 0.f;
	m_playerStats.clear();
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::Update(float frameTime)
{
#if CRY_WATCH_ENABLED
	if (m_dbgWatchLvl >= 1)
	{
		EntityId localPlayerId = g_pGame->GetIGameFramework()->GetClientActorId();
		SGameRulesPlayerStat *playerStat = GetPlayerStatsInternal(localPlayerId);
		if (playerStat)
		{
			WATCH_LV1("Score=%d; Team=%d", playerStat->points, g_pGame->GetGameRules()->GetTeam(localPlayerId));
		}
		WATCH_LV1_ONLY(" ");
	}

	if (m_dbgWatchLvl >= 2)
	{
		WATCH_LV2("Player stats:");
		for (TPlayerStats::iterator it=m_playerStats.begin(); it!=m_playerStats.end(); ++it)
		{
			if (IEntity* e=gEnv->pEntitySystem->GetEntity(it->playerId))
			{
				WATCH_LV2("%s: deaths %d, deathsThisRound %d, HASSPAWNEDTHISROUND %d", e->GetName(), it->deaths, it->deathsThisRound, ((it->flags&SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND)?1:0));
			}
		}
	}
#endif

	if (!gEnv->bServer)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		float currTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
		if (currTime - m_lastUpdatedPings > CGameRulesStandardPlayerStats::GR_PLAYER_STATS_PING_UPDATE_INTERVAL)
		{
			m_lastUpdatedPings = currTime;

			for (TPlayerStats::iterator it=m_playerStats.begin(); it!=m_playerStats.end(); ++it)
			{
				IActor *actor =  g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(it->playerId);
				if (actor)
				{
					INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(actor->GetChannelId());
					if (pNetChannel)
						it->ping = (int)floor((pNetChannel->GetPing(true) * 1000.f) + 0.5f);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::OnStartNewRound()
{
	CryLog("[tlh] @ CGameRulesStandardPlayerStats::OnStartNewRound()");
	CRY_ASSERT(gEnv->bServer);
	for (TPlayerStats::iterator it=m_playerStats.begin(); it!=m_playerStats.end(); ++it)
	{
		it->deathsThisRound = 0;
		it->flags &= ~(SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND | SGameRulesPlayerStat::PLYSTATFL_DIEDINEXTRATIMETHISROUND);
	}
	CHANGED_NETWORK_STATE(g_pGame->GetGameRules(), CPlayer::ASPECT_PLAYERSTATS_SERVER);
}

//-------------------------------------------------------------------------
bool CGameRulesStandardPlayerStats::NetSerialize( EntityId playerId, TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == CPlayer::ASPECT_PLAYERSTATS_SERVER)
	{
		SGameRulesPlayerStat *playerStat = GetPlayerStatsInternal(playerId);
		if (playerStat)
		{
			int pointsBefore = playerStat->points;
			uint16 deathsThisRoundBefore = playerStat->deathsThisRound;
			uint8 explosionsBefore = playerStat->successfulExplosions;
			uint8 flagsBefore = playerStat->flags;

			playerStat->NetSerialize(ser);

			if (gEnv->bClient)
			{
				if (playerStat->deathsThisRound > deathsThisRoundBefore)
				{
					CRY_ASSERT(ser.IsReading());
					if (CGameRules* pGameRules=g_pGame->GetGameRules())
					{
						pGameRules->ClPlayerStatsNetSerializeReadDeath_NotifyListeners(playerStat, deathsThisRoundBefore, flagsBefore);
					}
				}

				if(playerStat->successfulExplosions != explosionsBefore && playerStat->playerId == g_pGame->GetIGameFramework()->GetClientActorId())  //!= allows for wrapping back to 0
				{
					SendHUDExplosionEvent();
				}
			}

#if 0
			if (pointsBefore != playerStat->points)
			{
				assert (ser.IsReading());

#if DISPLAY_POINTS_ON_HUD
				if (gEnv->bClient && (playerId == gEnv->pGame->GetIGameFramework()->GetClientActorId()))
				{
					if (CHUD *pHUD = g_pGame->GetHUD())
					{
						string message;
						message.Format("%d", (playerStat->points - pointsBefore));
						pHUD->DisplayFunMessage(message.c_str(), NULL);
					}
				}
#endif
			}
#endif
		}
		else
		{
			SGameRulesPlayerStat dummyStat(0);
			dummyStat.NetSerialize(ser);
		}
	}

	return true;
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::CreatePlayerStats(EntityId playerId)
{
	if (!GetPlayerStats(playerId))
	{
		m_playerStats.push_back(SGameRulesPlayerStat(playerId));
		return;
	}
	else
	{
		GameWarning("CreatePlayerStats Stats already exist for player '%d'", playerId);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::RemovePlayerStats(EntityId playerId)
{
	for (TPlayerStats::iterator it=m_playerStats.begin(); it!=m_playerStats.end(); ++it)
	{
		if (it->playerId == playerId)
		{
			m_playerStats.erase(it);
			return;
		}
	}
	
	GameWarning("RemovePlayerStats No stats found for player '%d'", playerId);
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::ClearAllPlayerStats()
{
	for (TPlayerStats::iterator it=m_playerStats.begin(); it!=m_playerStats.end(); ++it)
	{
		// [tlh] this is extremely dodgy, but the player spawning happens before this clear function is called and we need the spawn count to be correct.
		//       also, is this clear function actually needed at all, because between pre-game and ingame the playerstats are recreated anyway...?
		bool  hackSpawns = false;
		if (it->flags & SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND)
		{
#if 0				// It is perfectly valid to spawn and die in the same round!
			if (!GetISystem()->IsDedicated())
			{
				CRY_ASSERT(it->deathsThisRound == 0);
			}
#endif
			hackSpawns = true;
		}

		it->Clear();

		if (hackSpawns)
		{
			it->flags |= SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND;
		}

		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(it->playerId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
	}
}

//-------------------------------------------------------------------------
SGameRulesPlayerStat * CGameRulesStandardPlayerStats::GetPlayerStatsInternal(EntityId playerId)
{
	int num = m_playerStats.size();
	for (int i=0; i < num; ++i)
	{
		if (m_playerStats[i].playerId == playerId)
		{
			return &m_playerStats[i];
		}
	}

	return 0;
}

//-------------------------------------------------------------------------
int CGameRulesStandardPlayerStats::GetNumPlayerStats()
{
	return m_playerStats.size();
}

//-------------------------------------------------------------------------
const SGameRulesPlayerStat* CGameRulesStandardPlayerStats::GetNthPlayerStats(int n)
{
	CRY_ASSERT(n>=0 && n<m_playerStats.size());
	return &m_playerStats[n];
}

//-------------------------------------------------------------------------
const SGameRulesPlayerStat * CGameRulesStandardPlayerStats::GetPlayerStats(EntityId playerId)
{
	return GetPlayerStatsInternal(playerId);
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::OnPlayerKilled(const HitInfo &info)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (!pGameRules)
		return;

	// No scoring at game end
	IGameRulesStateModule *stateModule = pGameRules->GetStateModule();
	if (stateModule && stateModule->GetGameState() == IGameRulesStateModule::EGRS_PostGame)
		return;

	bool  isSuddenDeath = ((g_pGameCVars->g_timelimitextratime > 0.f) && (pGameRules->GetRemainingGameTimeNotZeroCapped() < 0.f));

	IActorSystem* pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
	IActor *targetActor =  pActorSystem->GetActor(info.targetId);
	IActor *shooterActor =  pActorSystem->GetActor(info.shooterId);

	if (targetActor)
	{
		SGameRulesPlayerStat *playerStat = GetPlayerStatsInternal(info.targetId);
		if (playerStat)
		{
			playerStat->deaths++;
			playerStat->deathsThisRound++;
			if (isSuddenDeath)
				playerStat->flags |= SGameRulesPlayerStat::PLYSTATFL_DIEDINEXTRATIMETHISROUND;

			if (gEnv->bServer)
			{
				g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(info.targetId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
			}
		}
	}

	if (shooterActor && (shooterActor != targetActor))
	{
		SGameRulesPlayerStat *playerStats = GetPlayerStatsInternal(info.shooterId);
		if (playerStats)
		{
			int targetTeam = pGameRules->GetTeam(info.targetId);
			int shooterTeam = pGameRules->GetTeam(info.shooterId);

			if ((pGameRules->GetTeamCount() > 1) && targetTeam == shooterTeam)	// Team Kill
			{
				playerStats->teamKills++;
			}
			else
			{
				playerStats->kills++;

				if (targetActor->IsPlayer() && static_cast<CPlayer*>(targetActor)->IsHeadShot(info))
					playerStats->headshots++;
			}

			if (gEnv->bServer)
			{
				g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(info.shooterId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::IncreasePoints(EntityId playerId, int amount)
{
	SGameRulesPlayerStat *playerStat = GetPlayerStatsInternal(playerId);
	if (playerStat)
	{
		playerStat->points += amount;

		if (gEnv->bServer)
		{
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(playerId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
		}

#if DISPLAY_POINTS_ON_HUD
		if (gEnv->bServer && gEnv->bClient && (playerId == gEnv->pGame->GetIGameFramework()->GetClientActorId()))
		{
			if (CHUD *pHUD = g_pGame->GetHUD())
			{
				string message;
				message.Format("%d", amount);	// TODO: Handle float amounts
				pHUD->DisplayFunMessage(message.c_str(), NULL);
			}
		}
#endif
	}
	else
	{
		CryLogAlways ("CGameRulesStandardPlayerStats::IncreasePoints failed to find SGameRulesPlayerStat for %s", g_pGame->GetGameRules()->GetEntityName(playerId));
	}
}

void CGameRulesStandardPlayerStats::EntityRevived(EntityId entityId)
{
	if (gEnv->bServer)
	{
		if (SGameRulesPlayerStat* s=GetPlayerStatsInternal(entityId))
		{
			s->flags |= SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND;
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(entityId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
		}
	}
}

void CGameRulesStandardPlayerStats::ProcessSuccessfulExplosion(EntityId playerId)
{
	CRY_ASSERT_MESSAGE(gEnv->bServer, "ProcessSuccessfulExplosion should only be called from the server");

	if (SGameRulesPlayerStat* stats = GetPlayerStatsInternal(playerId))
	{
		stats->successfulExplosions++;

		if(gEnv->bClient && playerId == g_pGame->GetIGameFramework()->GetClientActorId())
		{
			SendHUDExplosionEvent();
		}
		else
		{
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(playerId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
		}
	}
}

void CGameRulesStandardPlayerStats::IncrementAssistKills(EntityId playerId)
{
	CRY_ASSERT_MESSAGE(gEnv->bServer, "IncrementAssistKills should only be called from the server");

	if (SGameRulesPlayerStat* stats = GetPlayerStatsInternal(playerId))
	{
		stats->assists++;

		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(playerId, CPlayer::ASPECT_PLAYERSTATS_SERVER);
	}
}

void CGameRulesStandardPlayerStats::SendHUDExplosionEvent()
{
	SHUDEvent hudEvent;
	hudEvent.eventType = eHUDEvent_OnHitTarget;
	hudEvent.eventIntData = EGRTT_Hostile;
	hudEvent.eventIntData2 = eHUDEventHT_Explosive;
				
	CHUD::CallEvent(hudEvent);
}

//-------------------------------------------------------------------------
void CGameRulesStandardPlayerStats::IncreaseKillCount( EntityId playerId, EntityId victimId )
{
	SGameRulesPlayerStat *playerStat = GetPlayerStatsInternal(playerId);
	playerStat->IncrementTimesPlayerKilled( victimId );
}

#undef WATCH_LV1
#undef WATCH_LV1_ONLY
#undef WATCH_LV2
