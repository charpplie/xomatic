#include "StdAfx.h"
#include "HUD_PSLScoreElement.h"

#include "HUD/HUD.h" // convert to time / TODO REMVOE!
#include "HUD/HUD_Impl.h" // convert to time / TODO REMVOE!

#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"

static const float k_maskYStart = 10.2f;
static const float k_maskEnd = -11.0f;

CHUD_PSLScoreElement::CHUD_PSLScoreElement()
: m_pPSLScoreElement(NULL),
  m_ownTeamScore(0),
  m_enemyTeamScore(0),
  m_ownScore(0),
  m_ownTeam(0),
  m_roundTime(0),
  m_scoreLimit(0)
{
	// ...
}

void CHUD_PSLScoreElement::Init( void )
{
	m_pPSLScoreElement = GetAsset("PSLScoreElement");
	m_pPSLScoreElement->Invoke( "_root.Root.Base01.gotoAndStop", "GreyToRed" );
	m_pPSLScoreElement->Invoke( "_root.Root.Base02.gotoAndStop", "GreyToRed" );
	m_pPSLScoreElement->Invoke( "_root.Root.Base03.gotoAndStop", "GreyToRed" );
	m_pPSLScoreElement->SetVariable( "_root.Root.Base01.GreyToRed.mask01._y", k_maskYStart);
	m_pPSLScoreElement->SetVariable( "_root.Root.Base02.GreyToRed.mask01._y", k_maskYStart);
	m_pPSLScoreElement->SetVariable( "_root.Root.Base03.GreyToRed.mask01._y", k_maskYStart);
	UpdateFlash();
}

void CHUD_PSLScoreElement::Draw( void )
{
	m_pPSLScoreElement->Draw();
}

void CHUD_PSLScoreElement::Update( float frametime )
{
	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if(!pClientActor)
	{
		return;
	}

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(!pGameRules)
	{
		return;
	}

	IScriptTable *pGameRulesScript=pGameRules->GetEntity()->GetScriptTable();
	if(!pGameRulesScript)
	{
		return;
	}

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
	scoreLimit = 100;//percent - g_pGameCVars->g_fraglimit;

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
		ownTeam!=m_ownTeam ||
		m_bForceUpdate )
	{
		m_ownTeamScore = ownTeamScore;
		m_enemyTeamScore = enemyTeamScore;
		m_ownScore = ownScore;
		m_roundTime = roundTime;
		m_scoreLimit = scoreLimit;
		m_ownTeam = ownTeam;
		UpdateFlash();
		m_pPSLScoreElement->Update(frametime);
		m_bForceUpdate = false;
	}
}

void CHUD_PSLScoreElement::UpdateFlash( void )
{
	string stmp("");
	g_pGame->GetHUD()->GetHUDImpl()->ConvertSecondsToTimerString(m_roundTime,&stmp,true);
	m_pPSLScoreElement->SetVariable("_root.Root.Time.ColorsetTime.Time.text", stmp.c_str());

	m_pPSLScoreElement->SetVariable("_root.Root.OwnScore.ColorsetScore.Score.text", stmp.Format("%3d%",m_ownTeamScore).c_str());
	m_pPSLScoreElement->SetVariable("_root.Root.EnemyScore.ColorsetScore.Score.text", stmp.Format("%3d%",m_enemyTeamScore).c_str());

	int percentOwn = (int)(100.0f*(float)m_ownTeamScore/(float)m_scoreLimit);
	int percentEnemy = (int)(100.0f*(float)m_enemyTeamScore/(float)m_scoreLimit);
	m_pPSLScoreElement->SetVariable("_root.Root.own_prog.progress1._xscale", percentOwn);
	m_pPSLScoreElement->SetVariable("_root.Root.oponent_prog.progress1._xscale", percentEnemy);
}

void CHUD_PSLScoreElement::OnHUDEvent( const SHUDEvent& event )
{
	switch( event.eventType )
	{
	case eHUDEvent_OnSiteBeingCaptured :
		{
			const int& baseOwnerId = event.eventIntData2;
			const int& capturerId = event.eventIntData3;
			const int& baseId = 1 + event.eventIntData;
			const float& progressFraction = event.eventFloatData;

			string frameName("");
			if( baseOwnerId == 0 )
			{
				// neutral
				if( capturerId == m_ownTeam )
				{
					frameName = "GreyToBlue";
				}
				else
				{
					frameName = "GreyToRed";
				}
			}
			else if( baseOwnerId == m_ownTeam )
			{
				// friendly
				frameName = "BlueToRed";
			}
			else
			{
				//enemy
				frameName = "RedToBlue";
			}

			m_pPSLScoreElement->Invoke( string().Format("_root.Root.Base0%d.gotoAndStop", baseId).c_str(), frameName.c_str() );

			float newMaskPos = (k_maskYStart-k_maskEnd)*progressFraction + k_maskYStart;
			m_pPSLScoreElement->SetVariable( string().Format("_root.Root.Base0%d.%s.mask01._y", baseId, frameName.c_str()).c_str(), newMaskPos );

			m_bForceUpdate = true;
			break;
		}
	case eHUDEvent_OnSiteCaptured :
		{
			// Get Team
			IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
			if(!pClientActor)
			{
				return;
			}

			CGameRules *pGameRules=g_pGame->GetGameRules();
			if(!pGameRules)
			{
				return;
			}

			m_ownTeam = pGameRules->GetTeam(pClientActor->GetEntityId());
			if( !m_ownTeam )
			{
				m_ownTeam = 1;// courtesy of colG.
			}

			const int baseOwnerId = event.eventIntData2;
			const int baseId = 1 + event.eventIntData;

			string frameName("");
			if( baseOwnerId == 0 )
			{
				// neutral
				frameName = "GreyToRed";
			}
			else if( baseOwnerId == m_ownTeam )
			{
				// friendly
				frameName = "BlueToRed";
			}
			else
			{
				//enemy
				frameName = "RedToBlue";
			}

			m_pPSLScoreElement->Invoke( string().Format("_root.Root.Base0%d.gotoAndStop", baseId).c_str(), frameName.c_str() );
			m_pPSLScoreElement->SetVariable( string().Format("_root.Root.Base0%d.%s.mask01._y", baseId, frameName.c_str()).c_str(), k_maskYStart);
			m_bForceUpdate = true;
			break;
		}
	}
}