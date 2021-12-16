/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Standard game rules module to handle victory conditions for all vs 
		all modes
	-------------------------------------------------------------------------
	History:
	- 19:10:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardVictoryConditionsPlayer.h"
#include "IXml.h"
#include "GameRules.h"
#include "IGameRulesPlayerStatsModule.h"
#include "IGameRulesStateModule.h"
#include "HUD/HUD.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "Audio/Announcer.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "Utility/CryWatch.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#if CRY_WATCH_ENABLED
#define WATCH_SURVONE_LV1  g_pGameCVars->g_SurvivorOneVictoryConditions_watchLvl<1 ? (NULL) : CryWatch
#define WATCH_SURVONE_LV2  g_pGameCVars->g_SurvivorOneVictoryConditions_watchLvl<2 ? (NULL) : CryWatch
#else
#define WATCH_SURVONE_LV1
#define WATCH_SURVONE_LV2
#endif

#define GAMERULES_VICTORY_PLAYER_MSG_DELAY_TIMER_LENGTH	3.f

//-------------------------------------------------------------------------
CGameRulesStandardVictoryConditionsPlayer::CGameRulesStandardVictoryConditionsPlayer()
{

}

//-------------------------------------------------------------------------
CGameRulesStandardVictoryConditionsPlayer::~CGameRulesStandardVictoryConditionsPlayer()
{
	if (gEnv->bServer)
	{
		CallLuaFunc(&m_luaFuncEndSuddenDeathSv);  // calling here on destruction for sanity's sake
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::Init( XmlNodeRef xml )
{
	inherited::Init(xml);

	m_luaFuncStartSuddenDeathSv.Format("%s", xml->getAttr("luaFuncStartSuddenDeathSv"));
	m_luaFuncEndSuddenDeathSv.Format("%s", xml->getAttr("luaFuncEndSuddenDeathSv"));

	m_tmpSuddenDeathMsg.Format("%s", xml->getAttr("tmpSuddenDeathMsg"));

	// (temporary until we have a frontend that can set this)
	float  tmpTimeLimitExtraTime = 0;
	if (xml->getAttr("tmpExtraTime", tmpTimeLimitExtraTime))
	{
		g_pGameCVars->g_timelimitextratime = tmpTimeLimitExtraTime;
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::Update( float frameTime )
{
	inherited::Update(frameTime);

	IGameRulesStateModule *pStateModule = m_pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)
		return;

	if (m_checkTime && m_pGameRules->IsTimeLimited())
	{
		if (g_pGameCVars->g_timelimitextratime > 0.f)
		{
			float  timeRemaining = m_pGameRules->GetRemainingGameTimeNotZeroCapped();
			if (timeRemaining >= 0.f)
			{
				WATCH_SURVONE_LV1("TIME: %.2f", timeRemaining);
			}
			else
			{
				if ((frameTime > 0.f) && (-timeRemaining <= frameTime))
				{
					if (gEnv->bServer)
					{
						CCCPOINT(VictoryConditionsPlayer_SvStartSuddenDeath);
						CallLuaFunc(&m_luaFuncStartSuddenDeathSv);
					}
					if (gEnv->bClient)
					{
						if (!m_tmpSuddenDeathMsg.empty())
						{
							CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_tmpSuddenDeathMsg.c_str(), NULL, 4.0f);
						}
					}
				}
				float  rem = MAX(0.f, ((g_pGameCVars->g_timelimitextratime * 60.f) + timeRemaining));  // remember, timeRemaining will be negative here
				WATCH_SURVONE_LV1("TIME: %.2f *** SUDDEN DEATH!! ***", rem);
			}
		}
		else
		{
			float  timeRemaining = m_pGameRules->GetRemainingGameTime();
			WATCH_SURVONE_LV1("TIME: %.2f", timeRemaining);
		}
		WATCH_SURVONE_LV1(" ");
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::ClVictoryPlayer( int playerId, EGameOverReason reason )
{
	if (gEnv->bClient)
	{
		OnEndGamePlayer(playerId, reason);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::CheckScoreLimit()
{
	int scoreLimit = (m_checkScore ? m_pGameRules->GetScoreLimit() : 0);
	if (scoreLimit)
	{
		SPlayerScoreResult result;
		GetMaxPlayerScore(result);

		if (result.m_maxScore >= scoreLimit)
		{
			if (result.m_maxScorePlayerId)
			{
				OnEndGamePlayer(result.m_maxScorePlayerId, EGOR_ScoreLimitReached);
			}
			else
			{
				OnEndGamePlayer(0, EGOR_ScoreLimitReached);
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::CheckTimeLimit()
{
	assert(gEnv->bServer);
	if (g_pGameCVars->g_timelimitextratime > 0.f)
	{
		if (m_checkTime && m_pGameRules->IsTimeLimited())
		{
			float  timeRemaining = m_pGameRules->GetRemainingGameTimeNotZeroCapped();
			if (timeRemaining <= -(g_pGameCVars->g_timelimitextratime * 60.f))
			{
				CCCPOINT(VictoryConditionsPlayer_SvSuddenDeathTimeExpired);
				TimeLimitExpired();
			}
		}
	}
	else
	{
		inherited::CheckTimeLimit();
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::TimeLimitExpired()
{
	SPlayerScoreResult result;
	GetMaxPlayerScore(result);

	if (result.m_maxScorePlayerId)
	{
		OnEndGamePlayer(result.m_maxScorePlayerId, EGOR_TimeLimitReached);
	}
	else
	{
		OnEndGamePlayer(0, EGOR_TimeLimitReached);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::GetMaxPlayerScore(SPlayerScoreResult &result)
{
	IGameRulesPlayerStatsModule *pStatsModule = m_pGameRules->GetPlayerStatsModule();
	if (pStatsModule)
	{
		CGameRules::TPlayers players;
		m_pGameRules->GetPlayers(players);

		int numPlayers = players.size();
		for (int i = 0; i < numPlayers; ++ i)
		{
			EntityId playerId = players[i];

			const SGameRulesPlayerStat *pPlayerStats = pStatsModule->GetPlayerStats(playerId);
			if (pPlayerStats)
			{
				if (pPlayerStats->points > result.m_maxScore)
				{
					result.m_maxScorePlayerId = playerId;
					result.m_maxScore = pPlayerStats->points;
				}
				else if (pPlayerStats->points == result.m_maxScore)
				{
					result.m_maxScorePlayerId = 0;
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsPlayer::OnEndGamePlayer( EntityId playerId, EGameOverReason reason )
{
	if (gEnv->bServer)
	{
		IGameRulesRoundsModule *pRoundsModule = m_pGameRules->GetRoundsModule();
		if (pRoundsModule)
		{
				float  timeRemaining = m_pGameRules->GetRemainingGameTimeNotZeroCapped();
				if (timeRemaining < 0.f)
				{
					CCCPOINT(VictoryConditionsPlayer_SvEndSuddenDeath);
					CallLuaFunc(&m_luaFuncEndSuddenDeathSv);
				}
			pRoundsModule->OnEndGame(0, playerId, reason);
			if (pRoundsModule->GetRoundsRemaining() != 0)
			{
				// Game isn't actually over yet!
				return;
			}
		}

		IGameRulesStateModule *pStateModule = m_pGameRules->GetStateModule();
		if (pStateModule)
			pStateModule->OnGameEnd();

		CGameRules::VictoryPlayerParams params(playerId, reason);
		m_pGameRules->GetGameObject()->InvokeRMI(CGameRules::ClVictoryPlayer(), params, eRMI_ToRemoteClients);
	}
	
	m_pGameRules->OnEndGame();

	if (gEnv->bClient)
	{
		bool localPlayerWon = (playerId == g_pGame->GetIGameFramework()->GetClientActorId());
		CHUD* pHUD = g_pGame->GetHUD();
		const char* localizedText = "";

		if (playerId)
		{
			IEntity *pPlayer = gEnv->pEntitySystem->GetEntity(playerId);
			//if (pPlayer)
			//{
			//	m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_GameOverWinner", pPlayer->GetName());
			//}

			if (localPlayerWon)
			{
				CAudioSignalPlayer::JustPlay("MatchWon");

				if (!m_winString.empty())
				{
					localizedText = pHUD->LocalizeString( m_winString.c_str(), pPlayer ? pPlayer->GetName() : "" );
					CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", localizedText, NULL, GAMERULES_VICTORY_PLAYER_MSG_DELAY_TIMER_LENGTH);
				}
			}
			else
			{
				CAudioSignalPlayer::JustPlay("MatchLost");

				if (!m_loseString.empty())
				{
					localizedText = pHUD->LocalizeString( m_loseString.c_str(), pPlayer ? pPlayer->GetName() : "" );
					CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", localizedText, NULL, GAMERULES_VICTORY_PLAYER_MSG_DELAY_TIMER_LENGTH);
				}
			}
		}
		else
		{
			CAudioSignalPlayer::JustPlay("MatchDraw");

			//m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_GameOverNoWinner");
			if (!m_drawString.empty())
			{
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_drawString.c_str(), NULL, GAMERULES_VICTORY_PLAYER_MSG_DELAY_TIMER_LENGTH);
			}
		}

		EGameOverType gameOverType = EGOT_Unknown;
		if(playerId == 0)
		{
			gameOverType = EGOT_Draw;
		}
		else
		{
			gameOverType = localPlayerWon ? EGOT_Win : EGOT_Lose;
		}
		CRY_ASSERT(gameOverType != EGOT_Unknown);
	
		m_pGameRules->GameOver( gameOverType );
	}
}

void CGameRulesStandardVictoryConditionsPlayer::CallLuaFunc(TFixedString* funcName)
{
	if (funcName && !funcName->empty())
	{
		IScriptTable*  pTable = m_pGameRules->GetEntity()->GetScriptTable();
		HSCRIPTFUNCTION  func;
		if (pTable && pTable->GetValue(funcName->c_str(), func))
		{
			IScriptSystem*  pScriptSystem = gEnv->pScriptSystem;
			Script::Call(pScriptSystem, func, pTable);
		}
	}
}


#undef WATCH_SURVONE_LV1
#undef WATCH_SURVONE_LV2