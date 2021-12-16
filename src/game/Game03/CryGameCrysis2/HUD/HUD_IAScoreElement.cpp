#include "StdAfx.h"

#include "HUD_IAScoreElement.h"

#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"

#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"

CHUD_IAScoreElement::CHUD_IAScoreElement()
{
	m_pPlayer = NULL;
}

CHUD_IAScoreElement::~CHUD_IAScoreElement()
{
}

// Ripped from PreCXP 'HUDTeamInstantAction.cpp' (apparently)
void CHUD_IAScoreElement::UpdateData( void )
{
	IActor*  pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	EntityId  clientId = g_pGame->GetIGameFramework()->GetClientActorId();
	assert(clientId);

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules)
		return;

	IGameRulesPlayerStatsModule*  pPlayStatsMo = pGameRules->GetPlayerStatsModule();
	IGameRulesSpawningModule*  pSpawnMo = pGameRules->GetSpawningModule();
	IGameRulesStateModule*  pStateMo = pGameRules->GetStateModule();

	int roundTime = m_roundTime;
	int ownScore = m_ownScore;
	int ownRank = m_ownRank;

	if (!pStateMo || (pStateMo->GetGameState() != IGameRulesStateModule::EGRS_PreGame))
	{
		if (pGameRules->IsTimeLimited())
		{
			if (g_pGameCVars->g_timelimitextratime > 0.f)
			{
				float  rem = pGameRules->GetRemainingGameTimeNotZeroCapped();
				if (rem >= 0.f)
				{
					roundTime = (int)floor(rem);
				}
				else
				{
					roundTime = MAX(0, (int)floor((g_pGameCVars->g_timelimitextratime * 60.f) + rem));  // remember, rem will be negative here
				}
			}
			else
			{
				roundTime = (int)floor(pGameRules->GetRemainingGameTime());
			}
		}
		else
		{
			roundTime = (int)floor(pGameRules->GetCurrentGameTime());
		}
	}
	
	if (pPlayStatsMo)
	{
		if (const SGameRulesPlayerStat* s=pPlayStatsMo->GetPlayerStats(clientId))
		{
			ownScore = s->points;
		}
	}

	if (pSpawnMo && pPlayStatsMo)
	{
		ownRank = 1;
		int  numStats = pPlayStatsMo->GetNumPlayerStats();
		for (int i=0; i<numStats; i++)
		{
			const SGameRulesPlayerStat*  s = pPlayStatsMo->GetNthPlayerStats(i);
			if (s->flags & SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND)
			{
				if (s->points > ownScore)
				{
					ownRank++;
				}
			}
		}
	}

	/*
	if (m_firstUpdate)
	{
	}
	*/

	if (roundTime!=m_roundTime ||
			ownScore!=m_ownScore ||
			ownRank!=m_ownRank)
	{
		UpdateFlash(roundTime, ownScore, ownRank);

		m_roundTime = roundTime;
		m_ownScore = ownScore;
		m_ownRank = ownRank;

		m_firstUpdate = false;
	}
}

void CHUD_IAScoreElement::UpdateFlash(int roundTime, int ownScore, int ownRank)
{
	EntityId  clientId = g_pGame->GetIGameFramework()->GetClientActorId();
	if (!clientId)
		return;

	string  stmp;

	// IA is currently using the AON Flash assets, so we need to make some of the elements invisible...
	m_pPlayer->SetVariable("_root.Root.GameState._visible", false);  // _root.Root.GameState.GameStateText.Text.Prompt.text
	m_pPlayer->SetVariable("_root.Root.PlayerLives._visible", false);
	m_pPlayer->SetVariable("_root.Root.PlayerLivesCount._visible", false);
	m_pPlayer->SetVariable("_root.Root.EnemyList._visible", false);  // _root.Root.EnemyList.EnemyCount[1-5].EnemyText.text

	// Score
	if (ownScore != m_ownScore)
	{
		stmp.Format("%d",ownScore);
		m_pPlayer->SetVariable("_root.Root.OwnScore.ColorsetScore.Score.text", stmp.c_str());
		m_pPlayer->SetVariable("_root.Root.OwnScore.ColorsetScore.ScoreShadow.text", stmp.c_str());
	}

	// Rank
	if (ownRank != m_ownRank)
	{
		if (ownRank == 1)
			stmp.Format("%dst",ownRank);
		else if (ownRank == 2)
			stmp.Format("%dnd",ownRank);
		else if (ownRank == 3)
			stmp.Format("%drd",ownRank);
		else
			stmp.Format("%dth",ownRank);
		m_pPlayer->SetVariable("_root.Root.PositionScore.ColorsetScore.Score.text", stmp.c_str());
		m_pPlayer->SetVariable("_root.Root.PositionScore.ColorsetScore.ScoreShadow.text", stmp.c_str());
	}

	// Timers
	if (roundTime != m_roundTime)
	{
		g_pGame->GetHUD()->GetHUDImpl()->ConvertSecondsToTimerString(roundTime, &stmp, true);
		m_pPlayer->SetVariable("_root.Root.Time.ColorsetTime.Time.text", stmp.c_str());
	}
}

void CHUD_IAScoreElement::Update( float frameTime )
{
	CRY_ASSERT_MESSAGE(m_pPlayer, "ia scoreelement not found but ticking!");

	UpdateData();
	m_pPlayer->Update(frameTime);
}

void CHUD_IAScoreElement::Draw( void )
{
	if (!m_pPlayer)
		return;

	m_pPlayer->Draw();
}

void CHUD_IAScoreElement::Init()
{
	CHUDObject::Init();
	m_pPlayer = GetAsset("iaScoreElement");

	m_firstUpdate = true;

	m_roundTime = -1;
	m_ownScore = -1;
	m_ownRank = -1;
}

void CHUD_IAScoreElement::OnHUDEvent( const SHUDEvent& event )
{
#if 0  // (example)
	if(event.eventType == eHUDEvent_OnStartCarryingFlag)
	{
		m_carryingEnemyFlag = (event.eventIntData == 1);
	}
#endif
}
