#include "StdAfx.h"

#include "HUD_CDScoreElement.h"

#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "Utility/CryWatch.h"
#include "HUD/HUD.h"

CHUD_CDScoreElement::CHUD_CDScoreElement()
{
	m_pCDScoreBoard = NULL;
	m_ownTeamScore = -1.f;
	m_enemyTeamScore = -1.f;

	m_team1RoundScore = -1;
	m_team2RoundScore = -1;
	m_roundScoreLimit = -1;
	m_maxTeamScore = -1;

	m_ownTeam = -1;
	m_currentWaveNum = -1;
	m_currentWaveCount = -1;
	m_currentActiveWaveCount = -1;
}

void CHUD_CDScoreElement::Init( void )
{
	m_pCDScoreBoard = GetAsset("cdScoreElement");
	m_currentWaveNum = 0;
	m_currentWaveCount = 0;
	m_currentActiveWaveCount = 0;
}

void CHUD_CDScoreElement::UpdateData( void )
{
	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if(!pClientActor)
		return;

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(!pGameRules)
		return;

	IScriptTable *pGameRulesScript=pGameRules->GetEntity()->GetScriptTable();
	if(!pGameRulesScript)
		return;

	int ownTeam = pGameRules->GetTeam(pClientActor->GetEntityId());
	if( !ownTeam )
	{
		ownTeam = 1;
	}
	assert(ownTeam == 1 || ownTeam == 2);

	// Get Scores
	float ownTeamScore = (float)pGameRules->GetTeamsScore(ownTeam);
	float enemyTeamScore = (float)pGameRules->GetTeamsScore(1 + 2 - ownTeam);

	int team1RoundScore = pGameRules->GetTeamRoundScore(1);
	int team2RoundScore = pGameRules->GetTeamRoundScore(2);
	int roundScoreLimit = 0; //pGameRules->GetRoundLimit(); // TODO: Countdown rounds are first to not best of.

	IGameRulesScoringModule *scoringModule = pGameRules->GetScoringModule();
	int maxTeamScore = scoringModule ? scoringModule->GetMaxTeamScore() : 0;
	if (maxTeamScore <= 0)
	{
		maxTeamScore = 60;
	}

	float gameTime = pGameRules->GetCurrentGameTime();
	ownTeamScore = CLAMP(ownTeamScore - gameTime, 0.f, 999.f);
	enemyTeamScore = CLAMP(enemyTeamScore - gameTime, 0.f, 999.f);

	CRY_TODO(03, 12, 2009, "Temporary Watches, remove when hud implentation is available.");
	int team1score = pGameRules->GetTeamsScore(1);
	int team2score = pGameRules->GetTeamsScore(2);
	CryWatch("Wave #%d, count=%d active=%d", m_currentWaveNum, m_currentWaveCount, m_currentActiveWaveCount);
	CryWatch("Team 1 - Time Left: %d RoundWins: %d", team1score - (int)ceil(gameTime), team1RoundScore);
	CryWatch("Team 2 - Time Left: %d RoundWins: %d", team2score - (int)ceil(gameTime), team2RoundScore);

	// Team
	if (ownTeam!=m_ownTeam)
	{
		SFlashVarValue teamID( ownTeam );
		m_pCDScoreBoard->Invoke( "setOwnTeam", teamID );	
		m_ownTeam = ownTeam;
	}

	// Scores - Team Time
	if ((maxTeamScore != m_maxTeamScore) || (ownTeamScore!=m_ownTeamScore))
	{
		SFlashVarValue ownTeamScoreValues[3] = { ownTeam, ownTeamScore, ownTeamScore * 100.f / maxTeamScore };
		m_pCDScoreBoard->Invoke( "setTimer", ownTeamScoreValues, 3 );
		m_ownTeamScore = ownTeamScore;
	}

	if ((maxTeamScore != m_maxTeamScore) || (enemyTeamScore!=m_enemyTeamScore))
	{
		SFlashVarValue enemyTeamScoreValues[3] = { 1 + 2 - ownTeam, enemyTeamScore, enemyTeamScore * 100.f / maxTeamScore };
		m_pCDScoreBoard->Invoke( "setTimer", enemyTeamScoreValues, 3 );
		m_enemyTeamScore = enemyTeamScore;
	}

	if (maxTeamScore != m_maxTeamScore)
	{
		m_maxTeamScore = maxTeamScore;
	}

	// Scores - Round
	if ((roundScoreLimit != m_roundScoreLimit) || (team1RoundScore!=m_team1RoundScore))
	{
		SFlashVarValue team1RoundScoreValues[3] = { 1, team1RoundScore, roundScoreLimit };
		m_pCDScoreBoard->Invoke( "setScore", team1RoundScoreValues, 3 );
		m_team1RoundScore = team1RoundScore;
	}

	if ((roundScoreLimit != m_roundScoreLimit) || (team2RoundScore!=m_team2RoundScore))
	{
		SFlashVarValue team2RoundScoreValues[3] = { 2, team2RoundScore, roundScoreLimit };
		m_pCDScoreBoard->Invoke( "setScore", team2RoundScoreValues, 3 );
		m_team2RoundScore = team2RoundScore;
	}

	if (roundScoreLimit != m_roundScoreLimit)
	{
		m_roundScoreLimit = roundScoreLimit;
	}
}

void CHUD_CDScoreElement::Update( float frameTime )
{
	CRY_ASSERT_MESSAGE( m_pCDScoreBoard, "countdown scoreboard not found!" );

	UpdateData( );
	m_pCDScoreBoard->Update( frameTime );
}

void CHUD_CDScoreElement::Draw( void )
{
	assert( m_pCDScoreBoard );
	m_pCDScoreBoard->Draw();
}

void CHUD_CDScoreElement::OnHUDEvent( const SHUDEvent& event )
{
	switch( event.eventType )
	{
	case eHUDEvent_OnNewCaptureObjectiveWave :
		{
			m_currentWaveNum = event.GetData(0).GetInt();
			m_currentWaveCount = event.GetData(1).GetInt();
			m_currentActiveWaveCount = event.GetData(2).GetInt();
			UpdateBonusText();

			break;
		}
	case eHUDEvent_OnCaptureObjectiveNumChanged :
		{
			m_currentActiveWaveCount = event.GetData(0).GetInt();
			UpdateBonusText();

			break;
		}
	}
}

void CHUD_CDScoreElement::UpdateBonusText()
{
	CHUD* pHUD = g_pGame->GetHUD();
	const wchar_t* localizedText = L"";
	if (pHUD)
	{
		if (m_currentWaveCount > 0)
		{
			static char szCount[4];
			itoa(m_currentActiveWaveCount, szCount, 10);
			szCount[3]='\0';

			if (m_currentActiveWaveCount == 1)
			{
				localizedText = pHUD->LocalizeStringW( "@ui_msg_cnd_bonus_available", szCount );
			}
			else if (m_currentActiveWaveCount == 0)
			{
				// Nothing
			}
			else
			{
				localizedText = pHUD->LocalizeStringW( "@ui_msg_cnd_bonuses_available", szCount );
			}
		}
		else
		{
			localizedText = pHUD->LocalizeStringW( "@ui_msg_cnd_sudden_death" );
		}

		SFlashVarValue statusBarText(localizedText);
		m_pCDScoreBoard->Invoke("setStatusBarText", &statusBarText, 1);
	}
}