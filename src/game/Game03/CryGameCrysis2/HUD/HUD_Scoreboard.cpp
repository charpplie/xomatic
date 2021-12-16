#include "StdAfx.h"
#include "HUD_Scoreboard.h"
#include "Flash/Flash.h"
#include "HUD.h"
#include "HUD/HUD_Impl.h"
#include "IActorSystem.h"
#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/GameRulesStandardPlayerStats.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "FlashAnimation.h"
#include "Player.h"
#include "IPlayerInput.h"

#define SCOREBOARD_UPDATE_FREQUENCY 0.1f	// Frequency in seconds at which the scoreboard updates.

//------------------------------------------------------------------------
bool CHUD_Scoreboard::CompareScores(const SPlayerScoreEntry &elem1, const SPlayerScoreEntry &elem2)
{
	if (elem1.score > elem2.score)
		return true;
	else if (elem1.score < elem2.score)
		return false;

	if (elem1.kills > elem2.kills)
		return true;
	else if (elem1.kills < elem2.kills)
		return false;

	if (elem1.deaths > elem2.deaths)
		return false;
	
	return true;
}


//------------------------------------------------------------------------
CHUD_Scoreboard::CHUD_Scoreboard()
{
	m_gamemode = eHSG_InstantAction;
	m_bVisible = false;
	m_lastUpdateTime = 0.f;

	m_teamScore[0] = -1;
	m_teamScore[1] = -1;
	m_teamRoundScore[0] = -1;
	m_teamRoundScore[1] = -1;

	m_roundNumber = -1;
	m_scoreLimit = -1;
	m_roundLimit = -1;
	m_timeLimit = 0.f;
	m_isSuddenDeath = false;
}

//------------------------------------------------------------------------
CHUD_Scoreboard::~CHUD_Scoreboard()
{
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::Reload()
{
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::OnHUDEvent(const SHUDEvent& event)
{
	IHUDAsset *scoreboard = GetAsset("scoreboard");
	if(event.eventType == eHUDEvent_ShowScoreboard)
	{
		m_bVisible = true;

		//const char* data = (const char*) event.eventPtrData;
		//if (data)
		//	scoreboard->Invoke("SetScoreboardType", SFlashVarValue(data));

		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer && pPlayer->GetPlayerInput())
			pPlayer->GetPlayerInput()->ClearXIMovement();
	}
	else if(event.eventType == eHUDEvent_HideScoreboard)
	{
		m_bVisible = false;
	}
	else if(event.eventType == eHUDEvent_PlayerSwitchTeams)
	{
		EntityId eventData = (EntityId)event.GetData(0).GetInt();
		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		if (localActorId == eventData)
		{
			int teamId = g_pGame->GetGameRules()->GetTeam(localActorId);
			scoreboard->Invoke("SetLocalTeam", SFlashVarValue(teamId));
		}
	}
	else if (event.eventType == eHUDEvent_OnInitPlayer) // Local player
	{
		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		if (localActorId)
		{
			int teamId = g_pGame->GetGameRules()->GetTeam(localActorId);
			scoreboard->Invoke("SetLocalTeam", SFlashVarValue(teamId));
		}
	}
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::Init()
{
	IHUDAsset *scoreboard = GetAsset("scoreboard");
	if (scoreboard == CHUD::s_pHUDAssetNULL)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		// TODO: Not the nicest way of looking up gamemode to know which specific data to look-up, possibly try to think of a better way.
		string gamemode = pGameRules->GetEntity()->GetClass()->GetName();
		if (!stricmp(gamemode.c_str(),"InstantAction"))
		{
			m_gamemode = eHSG_InstantAction;
		}
		else if (!stricmp(gamemode.c_str(),"TeamInstantAction"))
		{
			m_gamemode = eHSG_TeamInstantAction;
		}
		else if (!stricmp(gamemode.c_str(),"CaptureTheFlag"))
		{
			m_gamemode = eHSG_CaptureTheFlag;
		}
		else if (!stricmp(gamemode.c_str(),"CrashSite"))
		{
			m_gamemode = eHSG_CrashSite;
		}
		else if (!stricmp(gamemode.c_str(),"PowerStruggleLite"))
		{
			m_gamemode = eHSG_PowerStruggleLite;
		}
		else if (!stricmp(gamemode.c_str(),"Extraction"))
		{
			m_gamemode = eHSG_Extraction;
		}
		else if (!stricmp(gamemode.c_str(),"AllOrNothing"))
		{
			m_gamemode = eHSG_AllOrNothing;
		}
		else if (!stricmp(gamemode.c_str(),"Assault"))
		{
			m_gamemode = eHSG_Assault;
		}
		else if (!stricmp(gamemode.c_str(),"Countdown"))
		{
			m_gamemode = eHSG_Countdown;
		}
		else
		{
				if (pGameRules->GetTeamCount() > 1)
				{
					m_gamemode = eHSG_TeamInstantAction;
					gamemode = "TeamInstantAction";
				}
				else
				{
					m_gamemode = eHSG_InstantAction;
					gamemode = "InstantAction";
				}
		}

		scoreboard->Invoke("Init", SFlashVarValue(gamemode.c_str()));
		scoreboard->Invoke("ClearPlayers");

		int scoreLimit = pGameRules->GetScoreLimit();
		int roundLimit = pGameRules->GetRoundLimit();
		float timeLimit = pGameRules->GetTimeLimit();

		m_scoreLimit = scoreLimit;
		m_roundLimit = roundLimit;
		m_timeLimit = timeLimit;
		m_roundNumber = 0;
		m_isSuddenDeath = false;

		SFlashVarValue gameInfoArgs[5] = { timeLimit, scoreLimit, 0, roundLimit, false };
		scoreboard->Invoke("SetGameInfo", gameInfoArgs, 5);

		scoreboard->Update(0.f); // Zero time update to make sure correct gamemode hud elements have been loaded.
	}
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::Update(float frameTime)
{
	if (!m_bVisible)
		return;

	float currFrameTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
	if (m_lastUpdateTime + SCOREBOARD_UPDATE_FREQUENCY > currFrameTime)
		return;

	frameTime = currFrameTime - m_lastUpdateTime;
	m_lastUpdateTime = currFrameTime;

	IHUDAsset *scoreboard = GetAsset("scoreboard");
	if (scoreboard == CHUD::s_pHUDAssetNULL)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
	if (pGameRules && localActorId)
	{
		// TODO: Only update stuff that's actually changed! Could do this by caching all the player entries and checking them for updates.

		// Update time remaining
		bool isSuddenDeath = false;

		string timeString = "";

		if (pGameRules->IsTimeLimited())
		{
			if (IGameRulesStateModule* pStateModule=pGameRules->GetStateModule())
			{
				if ((pStateModule->GetGameState() == IGameRulesStateModule::EGRS_InGame) || (pStateModule->GetGameState() == IGameRulesStateModule::EGRS_PostGame))
				{
					float  timeRemaining = (g_pGameCVars->g_timelimitextratime > 0.f) ? pGameRules->GetRemainingGameTimeNotZeroCapped() : pGameRules->GetRemainingGameTime();
					if (timeRemaining >= 0.f)
					{
						CHUD_Impl::ConvertSecondsToTimerString((int)timeRemaining, &timeString);
					}
					else
					{
						isSuddenDeath = true;
						timeRemaining = CLAMP((g_pGameCVars->g_timelimitextratime*60.f) + timeRemaining, 0.f, (g_pGameCVars->g_timelimitextratime*60.f));
						CHUD_Impl::ConvertSecondsToTimerString((int)timeRemaining, &timeString);
					}
				}
				else
				{
					CHUD_Impl::ConvertSecondsToTimerString((int)(g_pGameCVars->g_timelimit * 60.f), &timeString);
				}
			}
		}

		IGameRulesRoundsModule *pRoundsModule = pGameRules->GetRoundsModule();
		int roundNumber = pRoundsModule ? pRoundsModule->GetRoundNumber() : 0;
		int scoreLimit = pGameRules->GetScoreLimit();
		int roundLimit = pGameRules->GetRoundLimit();
		float timeLimit = pGameRules->GetTimeLimit();

		if (m_scoreLimit != scoreLimit
			|| m_roundLimit != roundLimit
			|| m_timeLimit != timeLimit
			|| m_roundNumber != roundNumber
			|| m_isSuddenDeath != isSuddenDeath)
		{
			SFlashVarValue gameInfoArgs[5] = { timeLimit, scoreLimit, (roundLimit>0)?CLAMP(roundNumber, 0, roundLimit):roundNumber, roundLimit, isSuddenDeath };
			scoreboard->Invoke("SetGameInfo", gameInfoArgs, 5);

			m_scoreLimit = scoreLimit;
			m_roundLimit = roundLimit;
			m_timeLimit = timeLimit;
			m_roundNumber = roundNumber;
			m_isSuddenDeath = isSuddenDeath;
		}
		
		SFlashVarValue timeArg(timeString.c_str());
		scoreboard->Invoke("SetTimeRemaining", &timeArg, 1);

		bool isTeamGame = pGameRules->GetTeamCount() != 0;

		if (isTeamGame)
		{
			UpdateTeamGame(scoreboard, localActorId);
		}
		else
		{
			UpdateNonTeamGame(scoreboard, localActorId);
		}
	}

	scoreboard->Update(frameTime);
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::UpdateNonTeamGame(IHUDAsset *scoreboard, EntityId localActorId)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	IGameRulesPlayerStatsModule *pPlayerStatsModule = pGameRules->GetPlayerStatsModule();

	std::vector<SPlayerScoreEntry> playerScores;

	// Update players

	if (pPlayerStatsModule)
	{
		int  numStats = pPlayerStatsModule->GetNumPlayerStats();
		for (int i=0; i<numStats; i++)
		{
			const SGameRulesPlayerStat *playerStats = pPlayerStatsModule->GetNthPlayerStats(i);

			if (playerStats)
			{
				SPlayerScoreEntry scoreEntry;

				scoreEntry.playerId = playerStats->playerId;

				IActor	*pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerStats->playerId);
				if(pActor && pActor->IsPlayer())
				{
					CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
					scoreEntry.rank = pPlayer->GetRank();
					scoreEntry.playerName = pActor->GetEntity()->GetName();
				}

				scoreEntry.score = playerStats->points;
				scoreEntry.kills = playerStats->kills;
				scoreEntry.assists = playerStats->assists;
				scoreEntry.deaths = playerStats->deaths;
				scoreEntry.isDead = pGameRules->IsDead(playerStats->playerId);

				GetGamemodeSpecificPlayerData(&scoreEntry);

				playerScores.push_back(scoreEntry);
			}
		}
	}
	// else - No lua equivalent as of yet.

	std::sort(playerScores.begin(), playerScores.end(), CompareScores);

	const int numPlayerScores = playerScores.size();
	for (int playerIndex = 0; playerIndex < numPlayerScores; ++ playerIndex)
	{
		SPlayerScoreEntry *scoreEntry = &(playerScores[playerIndex]);

		SFlashVarValue args[11] = {	0, 
			playerIndex, 
			scoreEntry->playerName, 
			scoreEntry->rank,
			scoreEntry->score, 
			scoreEntry->kills,
			scoreEntry->assists,
			scoreEntry->deaths, 
			scoreEntry->isDead,
			scoreEntry->gamemodeData1,
			(localActorId == scoreEntry->playerId) };
		scoreboard->Invoke("SetPlayerEntry", args, 11);
	}
	scoreboard->Invoke("ClearPlayers"); // Clear all other rows
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::UpdateTeamGame(IHUDAsset *scoreboard, EntityId localActorId)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	// Update team scores
	int team1Score=0, team2Score=0;
	IGameRulesPlayerStatsModule *pPlayerStatsModule = pGameRules->GetPlayerStatsModule();
	if (pPlayerStatsModule)
	{
		GetGamemodeSpecificTeamScores(pGameRules, team1Score, team2Score);
	}
	else
	{
		IScriptTable *pGameRulesScript = pGameRules->GetEntity()->GetScriptTable();
		if (pGameRulesScript)
		{
			int teamScoreKey;
			if (pGameRulesScript->GetValue("TEAMSCORE_TEAM1_KEY", teamScoreKey))
			{
				pGameRules->GetSynchedGlobalValue(teamScoreKey, team1Score);
			}
			if (pGameRulesScript->GetValue("TEAMSCORE_TEAM2_KEY", teamScoreKey))
			{
				pGameRules->GetSynchedGlobalValue(teamScoreKey, team2Score);
			}
		}
	}

	if ((m_teamScore[0] != team1Score) || (m_teamScore[1] != team2Score))
	{
		SFlashVarValue scoreArgs[2] = { team1Score, team2Score };
		scoreboard->Invoke("SetTeamScores", scoreArgs, 2);
		m_teamScore[0] = team1Score;
		m_teamScore[1] = team2Score;
	}

	// No gamemode requires this at the moment.
	//int team1RoundScore = pGameRules->GetTeamRoundScore(1);
	//int team2RoundScore = pGameRules->GetTeamRoundScore(2);
	//if ((m_teamRoundScore[0] != team1RoundScore) || (m_teamRoundScore[1] != team2RoundScore))
	//{
	//	SFlashVarValue roundScoreArgs[2] = { team1RoundScore, team2RoundScore };
	//	scoreboard->Invoke("SetTeamRoundsWon", roundScoreArgs, 2);
	//	m_teamRoundScore[0] = team1RoundScore;
	//	m_teamRoundScore[1] = team2RoundScore;
	//}

	// Update players

	for (int teamId = 1; teamId < 3; ++ teamId)
	{
		std::vector<SPlayerScoreEntry> playerScores;

		CGameRules::TPlayers players;
		pGameRules->GetTeamPlayers(teamId, players);
		const int numPlayersOnTeam = players.size();
		for (int playerIndex = 0; playerIndex < numPlayersOnTeam; ++ playerIndex)
		{
			EntityId playerId = players[playerIndex];

			if (pPlayerStatsModule)
			{
				const SGameRulesPlayerStat *playerStats = pPlayerStatsModule->GetPlayerStats(playerId);
				if (playerStats)
				{
					SPlayerScoreEntry scoreEntry;

					IActor	*pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerStats->playerId);
					if(pActor && pActor->IsPlayer())
					{
						CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
						scoreEntry.rank = pPlayer->GetRank();
						scoreEntry.playerName = pActor->GetEntity()->GetName();
					}

					scoreEntry.playerId = playerId;
					scoreEntry.score = playerStats->points;
					scoreEntry.kills = playerStats->kills;
					scoreEntry.assists = playerStats->assists;
					scoreEntry.deaths = playerStats->deaths;
					scoreEntry.isDead = pGameRules->IsDead(playerStats->playerId);

					GetGamemodeSpecificPlayerData(&scoreEntry);

					playerScores.push_back(scoreEntry);
				}
			}
			else // old lua gamerules
			{
				IEntity *pPlayer = gEnv->pEntitySystem->GetEntity(playerId);
				if (pPlayer)
				{
					IScriptTable *pGameRulesScript = pGameRules->GetEntity()->GetScriptTable();
					if(pGameRulesScript)
					{
						SPlayerScoreEntry scoreEntry;
						scoreEntry.playerId = playerId;
						scoreEntry.playerName = pPlayer->GetName();
						scoreEntry.isDead = pGameRules->IsDead(playerId);

						IActor	*pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
						if(pActor && pActor->IsPlayer())
						{
							CPlayer* pActorPlayer = static_cast<CPlayer*>(pActor);
							scoreEntry.rank = pActorPlayer->GetRank();
						}

						HSCRIPTFUNCTION pfnFuHelper = 0;
						if (pGameRulesScript->GetValue("GetPlayerPoints", pfnFuHelper) && pfnFuHelper)
						{
							Script::CallReturn(gEnv->pScriptSystem, pfnFuHelper, pGameRulesScript, ScriptHandle(playerId), scoreEntry.score);
							gEnv->pScriptSystem->ReleaseFunc(pfnFuHelper);
						}

						int killsKey;
						if (pGameRulesScript->GetValue("SCORE_KILLS_KEY", killsKey))
						{
							pGameRules->GetSynchedEntityValue(playerId, killsKey, scoreEntry.kills);
						}
						
						// TODO: Assists

						int deathsKey;
						if (pGameRulesScript->GetValue("SCORE_DEATHS_KEY", deathsKey))
						{
							pGameRules->GetSynchedEntityValue(playerId, deathsKey, scoreEntry.deaths);
						}

						GetGamemodeSpecificPlayerData(&scoreEntry);

						playerScores.push_back(scoreEntry);
					}
				}
			}
		}

		std::sort(playerScores.begin(), playerScores.end(), CompareScores);

		const int numPlayerScores = playerScores.size();
		for (int playerIndex = 0; playerIndex < numPlayerScores; ++ playerIndex)
		{
			SPlayerScoreEntry *scoreEntry = &(playerScores[playerIndex]);

			SFlashVarValue args[11] = {	teamId, 
				playerIndex, 
				scoreEntry->playerName,
				scoreEntry->rank,
				scoreEntry->score, 
				scoreEntry->kills,
				scoreEntry->assists,
				scoreEntry->deaths, 
				scoreEntry->isDead,
				scoreEntry->gamemodeData1,
				(localActorId == scoreEntry->playerId) };
			scoreboard->Invoke("SetPlayerEntry", args, 11);
		}
	}
	scoreboard->Invoke("ClearPlayers"); // Clear all other rows
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::Draw()
{
	if (m_bVisible)
	{
		GetAsset("scoreboard")->Draw();
	}
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::GetGamemodeSpecificPlayerData(SPlayerScoreEntry *scoreEntry)
{
	if (m_gamemode == eHSG_AllOrNothing)
	{
		scoreEntry->gamemodeData1 = 0;

		CGameRules *pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			if (IGameRulesSpawningModule *pSpawningModule = pGameRules->GetSpawningModule())
			{
				scoreEntry->gamemodeData1 = pSpawningModule->GetRemainingLives(scoreEntry->playerId);
			}
		}
	}
	// Other gamemode specific look-ups
}

//------------------------------------------------------------------------
void CHUD_Scoreboard::GetGamemodeSpecificTeamScores(CGameRules *pGameRules, int &team1Score, int &team2Score)
{
	if (m_gamemode == eHSG_Countdown)
	{
		int team1ScoreInSecs = pGameRules->GetTeamsScore(1) * 100;
		int team2ScoreInSecs = pGameRules->GetTeamsScore(2) * 100;
		int gameTime = (int)ceil(pGameRules->GetCurrentGameTime() * 100.f); // To get 2dp as int.
		team1Score = CLAMP(team1ScoreInSecs - gameTime, 0, 99999);
		team2Score = CLAMP(team2ScoreInSecs - gameTime, 0, 99999);
	}
	else
	{
		team1Score = pGameRules->GetTeamsScore(1);
		team2Score = pGameRules->GetTeamsScore(2);
	}
}