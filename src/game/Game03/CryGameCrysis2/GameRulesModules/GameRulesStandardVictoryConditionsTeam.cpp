/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Standard game rules module to handle victory conditions
	-------------------------------------------------------------------------
	History:
	- 08:09:2009  : Created by Ben Johnson

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardVictoryConditionsTeam.h"
#include "IXml.h"
#include "GameRules.h"
#include "IGameRulesPlayerStatsModule.h"
#include "IGameRulesStateModule.h"
#include "HUD/HUD.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "Audio/Announcer.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#define OTHER_TEAM(x) 3 - x
#define GAMERULES_VICTORY_TEAM_MSG_DELAY_TIMER_LENGTH	3.f

//-------------------------------------------------------------------------
CGameRulesStandardVictoryConditionsTeam::CGameRulesStandardVictoryConditionsTeam()
{
	m_pGameRules = NULL;
	m_currentSound = eSVC_1min;
	m_checkScore = true;
	m_checkTime = true;
	m_checkScoreAsTime = false;
	m_winOnPrimaryTeamDead = false;
	m_winOnSecondaryTeamDead = false;
	m_decideRoundWinnerUsingRoundPoints = false;
}

//-------------------------------------------------------------------------
CGameRulesStandardVictoryConditionsTeam::~CGameRulesStandardVictoryConditionsTeam()
{
	if (g_pGame->GetGameRules() && gEnv->bServer && (m_winOnPrimaryTeamDead || m_winOnSecondaryTeamDead))
	{
		g_pGame->GetGameRules()->UnRegisterSurvivorCountListener(this);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::Init( XmlNodeRef xml )
{
	m_pGameRules = g_pGame->GetGameRules();

	if (!xml->getAttr("checkScore", m_checkScore))
	{
		m_checkScore = true;
	}
	if (!xml->getAttr("checkTime", m_checkTime))
	{
		m_checkTime = true;
	}
	if (!xml->getAttr("checkScoreAsTime", m_checkScoreAsTime))
	{
		m_checkScoreAsTime = false;
	}
	
	// (temporary until we have a frontend that can set this)
	float  tmpTimeLimit = 0;
	if (xml->getAttr("tmpTimeLimit", tmpTimeLimit))
	{
		g_pGameCVars->g_timelimit = tmpTimeLimit;
	}

	if (!xml->getAttr("winOnPrimaryTeamDead", m_winOnPrimaryTeamDead))
	{
		m_winOnPrimaryTeamDead = false;
	}
	if (!xml->getAttr("winOnSecondaryTeamDead", m_winOnSecondaryTeamDead))
	{
		m_winOnSecondaryTeamDead = false;
	}

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "Strings"))
		{
			const char *pString = 0;
			if (xmlChild->getAttr("win", &pString))
			{
				m_winString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("lose", &pString))
			{
				m_loseString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("draw", &pString))
			{
				m_drawString.Format("@%s", pString);
			}
		}
	}
	
	if (!xml->getAttr("decideRoundWinnerUsingJustPointsScoredInRound", m_decideRoundWinnerUsingRoundPoints))
	{
		m_decideRoundWinnerUsingRoundPoints = false;
	}

	if (gEnv->bServer && (m_winOnPrimaryTeamDead || m_winOnSecondaryTeamDead))
	{
		m_pGameRules->RegisterSurvivorCountListener(this);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::Update( float frameTime )
{
	IGameRulesStateModule *pStateModule = m_pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)
		return;

	UpdateTimeLimitSounds();

	if (!gEnv->bServer)
		return;

	CheckTimeLimit();
	CheckScoreLimit();

	CheckScoreAsTimeLimit();
	CRY_TODO(09, 09, 2009, "Check Score limit could be done when the score changes as well as here.")
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::GetMaxTeamScore(STeamScoreResult &result)
{
	int numTeams = m_pGameRules->GetTeamCount();
	for (int i=1; i <= numTeams; ++i)
	{
		int score = (m_decideRoundWinnerUsingRoundPoints ? m_pGameRules->SvGetTeamsScoreScoredThisRound(i) : m_pGameRules->GetTeamsScore(i));
		if (score >= result.score)
		{
			if (result.score==score)
			{
				result.scoreTeamId = 0;
			}
			else
			{
				result.scoreTeamId = i;
				result.score = score;
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::GetMinTeamScore(STeamScoreResult &result)
{
	result.score = -1;

	int numTeams = m_pGameRules->GetTeamCount();
	for (int i=1; i <= numTeams; ++i)
	{
		int score = (m_decideRoundWinnerUsingRoundPoints ? m_pGameRules->SvGetTeamsScoreScoredThisRound(i) : m_pGameRules->GetTeamsScore(i));
		if ((result.score==-1) || (score <= result.score))
		{
			if (result.score==score)
			{
				result.scoreTeamId = 0;
			}
			else
			{
				result.scoreTeamId = i;
				result.score = score;
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::CheckTimeLimit()
{
	if (m_checkTime && m_pGameRules->IsTimeLimited())
	{
		float timeRemaining = m_pGameRules->GetRemainingGameTime();

		if(timeRemaining <= 0.f)
		{
			TimeLimitExpired();
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::TimeLimitExpired()
{
	CRY_ASSERT(gEnv->bServer);

	if (IGameRulesRoundsModule* pRoundsModule=m_pGameRules->GetRoundsModule())
	{
		if (pRoundsModule->IsInProgress())
		{
			m_pGameRules->SvOnTimeLimitExpired_NotifyListeners();  // nb. must come before the call to GetMaxTeamScore(), because listeners of this notification might want to award scores which then need to be taken into account by GetMaxTeamScore()
		}
	}

	STeamScoreResult result;
	GetMaxTeamScore(result);

	CCCPOINT(VictoryConditionsTeam_SvTimeLimitExpired);
	OnEndGame(result.scoreTeamId, EGOR_TimeLimitReached);
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::UpdateTimeLimitSounds()
{
	if(gEnv->bClient && m_checkTime && m_pGameRules->IsTimeLimited())
	{
		float timeRemaining = m_pGameRules->GetRemainingGameTime();

		if(timeRemaining <= 61.0f && m_currentSound == eSVC_1min)
		{
			CAnnouncer::GetInstance()->Announce("1MinuteLeft");
			m_currentSound = eSVC_15secs;
		}
		else if(timeRemaining <= 16.0f && m_currentSound == eSVC_15secs)
		{
			CAnnouncer::GetInstance()->Announce("15SecondsLeft");
			m_currentSound = eSVC_5secs;
		}
		else if(timeRemaining <= 6.0f && m_currentSound == eSVC_5secs)
		{
			CAnnouncer::GetInstance()->Announce("5SecondsLeft");
			m_currentSound = eSVC_3secs;
		}
		else if(timeRemaining <= 4.0f && m_currentSound == eSVC_3secs)
		{
			CAnnouncer::GetInstance()->Announce("3SecondsLeft");
			m_currentSound = eSVC_end;
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::CheckScoreLimit()
{
	int scoreLimit = (m_checkScore ? m_pGameRules->GetScoreLimit() : 0);
	if (scoreLimit > 0)
	{
		STeamScoreResult result;
		GetMaxTeamScore(result);

		if (result.score >= scoreLimit)
		{
			CCCPOINT(VictoryConditionsTeam_SvScoreLimitReached);
			OnEndGame(result.scoreTeamId, EGOR_ScoreLimitReached);
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::CheckScoreAsTimeLimit()
{
	if (m_checkScoreAsTime)
	{
		STeamScoreResult result;
		GetMinTeamScore(result);

		int gameTime = (int)ceil(m_pGameRules->GetCurrentGameTime());

		if (result.score - gameTime  <= 0)
		{
			CCCPOINT(VictoryConditionsTeam_SvTimeLimitReached);
			OnEndGame(result.scoreTeamId ? OTHER_TEAM(result.scoreTeamId) : 0, EGOR_TimeLimitReached); // TODO: Another win type - OtherTeamEliminated?
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::ClVictoryTeam(int teamId, EGameOverReason reason)
{
	if (gEnv->bClient)
	{
		OnEndGame(teamId, reason);
	}
}


//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::OnStartGame()
{
	m_currentSound = eSVC_1min;
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::OnEndGame(int teamId, EGameOverReason reason)
{
	if (gEnv->bServer)
	{
		IGameRulesRoundsModule *pRoundsModule = m_pGameRules->GetRoundsModule();
		if (pRoundsModule)
		{
			pRoundsModule->OnEndGame(teamId, 0, reason);
			if (pRoundsModule->GetRoundsRemaining() != 0)
			{
				// Game isn't actually over yet!
				return;
			}
		}

		IGameRulesStateModule *pStateModule = m_pGameRules->GetStateModule();
		if (pStateModule)
			pStateModule->OnGameEnd();

		CGameRules::VictoryTeamParams params(teamId, reason);
		m_pGameRules->GetGameObject()->InvokeRMI(CGameRules::ClVictoryTeam(), params, eRMI_ToRemoteClients);
	}
	
	m_pGameRules->OnEndGame();

	if (gEnv->bClient)
	{
		if (teamId)
		{
			//string teamNameString = "@mp_team_";
			//const char* teamName = m_pGameRules->GetTeamName(teamId);
			//teamNameString.append(teamName);

			//m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_GameOverWinner", teamNameString.c_str());
			
			CRY_TODO(07, 12, 2009, "Add support for putting the team name into the victory message i.e. 'Team US Won the game'");

			int clientTeam = m_pGameRules->GetTeam(gEnv->pGame->GetIGameFramework()->GetClientActorId());
			if(clientTeam == teamId)
			{
				CAudioSignalPlayer::JustPlay("MatchWon");
				
				if (!m_winString.empty())
					CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_winString.c_str(), NULL, GAMERULES_VICTORY_TEAM_MSG_DELAY_TIMER_LENGTH);
			}
			else
			{
				CAudioSignalPlayer::JustPlay("MatchLost");
				
				if (!m_loseString.empty())
					CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_loseString.c_str(), NULL, GAMERULES_VICTORY_TEAM_MSG_DELAY_TIMER_LENGTH);
			}
		}
		else
		{
			CAudioSignalPlayer::JustPlay("MatchDraw");

			//m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_GameOverNoWinner");
			if (!m_drawString.empty())
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_drawString.c_str(), NULL, GAMERULES_VICTORY_TEAM_MSG_DELAY_TIMER_LENGTH);
		}
	
		EGameOverType gameOverType = EGOT_Unknown;
		if(teamId == 0)
		{
			gameOverType = EGOT_Draw;
		}
		else
		{
			int clientTeam = m_pGameRules->GetTeam(gEnv->pGame->GetIGameFramework()->GetClientActorId());
			gameOverType = (clientTeam==teamId) ? EGOT_Win : EGOT_Lose;
		}
		CRY_ASSERT(gameOverType != EGOT_Unknown);
		
		m_pGameRules->GameOver( gameOverType );
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardVictoryConditionsTeam::SvSurvivorCountRefresh( int count, const EntityId survivors[], int numKills )
{
	// Don't end the round due to people dying if there is no-one on the teams!
	int team1Members = m_pGameRules->GetTeamPlayerCount(1);
	int team2Members = m_pGameRules->GetTeamPlayerCount(2);

	if (team1Members && team2Members)
	{
		int primaryTeamId = (m_pGameRules->GetRoundsModule()) ? m_pGameRules->GetRoundsModule()->GetPrimaryTeam() : 1;

		int primaryCount = 0;
		int secondaryCount = 0;

		for (int i = 0; i < count; ++ i)
		{
			if (m_pGameRules->GetTeam(survivors[i]) == primaryTeamId)
			{
				++ primaryCount;
			}
			else
			{
				++ secondaryCount;
			}
		}

		if (m_winOnPrimaryTeamDead && m_winOnSecondaryTeamDead && !primaryCount && !secondaryCount)
		{
			// Draw
			OnEndGame(0, EGOR_NoLivesRemaining);
		}
		else if (m_winOnPrimaryTeamDead && !primaryCount)
		{
			// Primary team is dead, secondary team wins
			OnEndGame(3 - primaryTeamId, EGOR_NoLivesRemaining);
		}
		else if (m_winOnSecondaryTeamDead && !secondaryCount)
		{
			// Secondary team is dead, primary team wins
			OnEndGame(primaryTeamId, EGOR_NoLivesRemaining);
		}
	}
}

#undef OTHER_TEAM
