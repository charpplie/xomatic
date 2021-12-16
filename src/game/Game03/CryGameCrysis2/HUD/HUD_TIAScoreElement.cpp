#include "StdAfx.h"

#include "HUD_TIAScoreElement.h"

#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"

CHUD_TiaScoreElement::CHUD_TiaScoreElement()
{
	m_pTiaScoreBoard = NULL;
	m_ownTeamScore = 0;
	m_enemyTeamScore = 0;
	m_ownScore = 0;
	m_ownTeam = 0;
	m_roundTime = 0;
	m_scoreLimit = 0;
}

void CHUD_TiaScoreElement::Init( void )
{
	m_pTiaScoreBoard = GetAsset("tiaScoreElement");
}

// Ripped from PreCXP 'HUDTeamInstantAction.cpp'
void CHUD_TiaScoreElement::UpdateData( void )
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

	int ownTeamScore = 0;
	int enemyTeamScore = 0;
	int ownScore = 0;
	int roundTime = 0;
	int scoreLimit = 100;
	int ownTeam = 0;
	
	ownTeam = pGameRules->GetTeam(pClientActor->GetEntityId());
	if( !ownTeam )
	{
		ownTeam = 1;// courtesy of colG.
	}
	assert(ownTeam == 1 || ownTeam == 2);

	// Get Scores
	ownTeamScore = pGameRules->GetTeamsScore(ownTeam);
	enemyTeamScore = pGameRules->GetTeamsScore(1 + 2 - ownTeam);

	IGameRulesStateModule *stateModule = pGameRules->GetStateModule();
	if (!stateModule || stateModule->GetGameState() != IGameRulesStateModule::EGRS_PreGame)
	{
		roundTime = pGameRules->IsTimeLimited() ? (int)floor(pGameRules->GetRemainingGameTime()) : (int)floor(pGameRules->GetCurrentGameTime());
	}
	scoreLimit = g_pGameCVars->g_fraglimit;

	IGameRulesPlayerStatsModule *pPlayerStats = pGameRules->GetPlayerStatsModule();
	const SGameRulesPlayerStat *playerStats = pPlayerStats ? pPlayerStats->GetPlayerStats(pClientActor->GetEntityId()) : NULL;
	if (playerStats)
	{
		ownScore = playerStats->kills;
	}

	if(	ownTeamScore!=m_ownTeamScore ||
		enemyTeamScore!=m_enemyTeamScore ||
		ownScore!=m_ownScore ||
		roundTime!=m_roundTime ||
		scoreLimit!=m_scoreLimit ||
		ownTeam!=m_ownTeam)
	{
		m_ownTeamScore = ownTeamScore;
		m_enemyTeamScore = enemyTeamScore;
		m_ownScore = ownScore;
		m_roundTime = roundTime;
		m_scoreLimit = scoreLimit;
		m_ownTeam = ownTeam;
		UpdateFlash();
	}
}

void CHUD_TiaScoreElement::UpdateFlash( void )
{
	// Team
	SFlashVarValue teamID( m_ownTeam );
	m_pTiaScoreBoard->Invoke( "setTeam", teamID );	

	// Scores
	SFlashVarValue ownScore( m_ownTeamScore );
	SFlashVarValue opponentScore( m_enemyTeamScore );
	m_pTiaScoreBoard->Invoke( "setOwnScore", ownScore );
	m_pTiaScoreBoard->Invoke( "setEnemyScore", opponentScore );

	// Timers
	SFlashVarValue timearg(m_roundTime);
	m_pTiaScoreBoard->Invoke("setValues", &timearg, 1);
}


void CHUD_TiaScoreElement::Update( float frameTime )
{
	CRY_ASSERT_MESSAGE( m_pTiaScoreBoard, "tiascoreboard not found!" );

	UpdateData( );
	m_pTiaScoreBoard->Update( frameTime );
}

void CHUD_TiaScoreElement::Draw( void )
{
	assert( m_pTiaScoreBoard );
	m_pTiaScoreBoard->Draw();
}

