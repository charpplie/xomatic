#include "StdAfx.h"

#include "HUD_AONScoreElement.h"

#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"

#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"

#define AONSE_GFX_20091120				1
#define AONSE_GFX_20091118_1402		0

CHUD_AONScoreElement::CHUD_AONScoreElement()
{
	m_pPlayer = NULL;
}

CHUD_AONScoreElement::~CHUD_AONScoreElement()
{
}

// Ripped from PreCXP 'HUDTeamInstantAction.cpp' (apparently)
void CHUD_AONScoreElement::UpdateData( void )
{
	IActor*  pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	EntityId  clientId = g_pGame->GetIGameFramework()->GetClientActorId();
	assert(clientId);

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(!pGameRules)
		return;

	IGameRulesPlayerStatsModule*  pPlayStatsMo = pGameRules->GetPlayerStatsModule();
	IGameRulesSpawningModule*  pSpawnMo = pGameRules->GetSpawningModule();
	IGameRulesStateModule*  pStateMo = pGameRules->GetStateModule();

	int roundTime = m_roundTime;
	int ownScore = m_ownScore;
	int ownRank = m_ownRank;
	bool suddenDeath = m_suddenDeath;
	int numStats = m_numStats;
	int numAlive = m_numAlive;
	int numEliminated = m_numEliminated;
	int lives = m_lives;

	suddenDeath = ((g_pGameCVars->g_timelimitextratime > 0.f) && (pGameRules->GetRemainingGameTimeNotZeroCapped() < 0.f));

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
		numStats = pPlayStatsMo->GetNumPlayerStats();

		if (const SGameRulesPlayerStat* s=pPlayStatsMo->GetPlayerStats(clientId))
		{
			ownScore = s->points;
		}
	}

	if (pSpawnMo && pPlayStatsMo)
	{
		int  spawnNumLives = pSpawnMo->GetNumLives();
		ownRank = 1;
		for (int i=0; i<numStats; i++)
		{
			const SGameRulesPlayerStat*  s = pPlayStatsMo->GetNthPlayerStats(i);
			if (s->flags & SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND)
			{
				if (s->points > ownScore)
				{
					ownRank++;
				}

				if ((s->deathsThisRound < spawnNumLives))
				{
					if (!suddenDeath || !(s->flags & SGameRulesPlayerStat::PLYSTATFL_DIEDINEXTRATIMETHISROUND))
					{
						numAlive++;
					}
					else
					{
						numEliminated++;
					}
				}
			}
		}
	}

	if (pSpawnMo)
	{
		lives = pSpawnMo->GetRemainingLives(clientId);
	}

	if (m_firstUpdate)
	{
		m_suddenDeath = !suddenDeath;
	}

	if (roundTime!=m_roundTime ||
			ownScore!=m_ownScore ||
			ownRank!=m_ownRank ||
			suddenDeath!=m_suddenDeath ||
			numStats!=m_numStats ||
			numAlive!=m_numAlive ||
			numEliminated!=m_numEliminated ||
			lives!=m_lives)
	{
		UpdateFlash(roundTime, ownScore, ownRank, suddenDeath, numStats, numAlive, numEliminated, lives);

		m_roundTime = roundTime;
		m_ownScore = ownScore;
		m_ownRank = ownRank;
		m_suddenDeath = suddenDeath;
		m_numStats = numStats;
		m_numAlive = numAlive;
		m_numEliminated = numEliminated;
		m_lives = lives;

		m_firstUpdate = false;
	}
}

void CHUD_AONScoreElement::UpdateFlash(int roundTime, int ownScore, int ownRank, bool suddenDeath, int numStats, int numAlive, int numEliminated, int lives)
{
	EntityId  clientId = g_pGame->GetIGameFramework()->GetClientActorId();
	if (!clientId)
		return;

	string  stmp;

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

	if (suddenDeath != m_suddenDeath)
	{
		CRY_TODO(20,11,09, "Messages need localising and adding to string tables.");
		if (!suddenDeath)
			stmp.Format("Eliminate All Enemies");
		else
			stmp.Format("Eliminate Remaining Enemies!");
		m_pPlayer->SetVariable("_root.Root.GameState.GameStateText.Text.Prompt.text", stmp.c_str());
	}

	if (lives != m_lives)
	{
#if AONSE_GFX_20091120
		{
			if (lives > 3)
			{
				m_pPlayer->SetVariable("_root.Root.PlayerLives._visible", false);
				m_pPlayer->SetVariable("_root.Root.PlayerLivesCount._visible", true);
				if (lives < m_lives)
					m_pPlayer->Invoke("_root.Root.PlayerLivesCount.gotoAndPlay", "LifeLost");
				else
					m_pPlayer->Invoke("_root.Root.PlayerLivesCount.gotoAndPlay", "Life");
				m_pPlayer->SetVariable("_root.Root.PlayerLivesCount.CountText.Count.text", stmp.Format("%d X",lives).c_str());
			}
			else
			{
				m_pPlayer->SetVariable("_root.Root.PlayerLives._visible", true);
				m_pPlayer->SetVariable("_root.Root.PlayerLivesCount._visible", false);
				if (lives == 0)
					m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "Dead");
				else if (lives == 1)
					m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "1Life");
				else if (lives == 2)
					m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "2Lives");
				else
				{
					m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "3Lives");
				}
			}
		}
#elif AONSE_GFX_20091118_1402
		{
			if (lives == 0)
				m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "Dead");
			else if (lives == 1)
				m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "1Life");
			else if (lives == 2)
				m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "2Lives");
			else
			{
				m_pPlayer->Invoke("_root.Root.PlayerLives.gotoAndPlay", "3Lives");
			}
		}
#endif
	}

	if ((numStats != m_numStats) || (numAlive != m_numAlive) || (numEliminated != m_numEliminated))
	{
		if (CGameRules* pGameRules=g_pGame->GetGameRules())
		{
			IGameRulesSpawningModule*  pSpawnMo = pGameRules->GetSpawningModule();
			IGameRulesPlayerStatsModule*  pPlayStatsMo = pGameRules->GetPlayerStatsModule();
			if (pSpawnMo && pPlayStatsMo)
			{
				int  numLives = pSpawnMo->GetNumLives();
				const int  shortSz = 4;
				EntityId  shortlist[shortSz];
				int  count = 0;
				for (int i=0; i<numStats; i++)
				{
					const SGameRulesPlayerStat*  s = pPlayStatsMo->GetNthPlayerStats(i);
					if ((s->flags & SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND) && (s->deathsThisRound < numLives))
					{
						if (!suddenDeath || !(s->flags & SGameRulesPlayerStat::PLYSTATFL_DIEDINEXTRATIMETHISROUND))
						{
							if (count < shortSz)
								shortlist[count] = s->playerId;
							count++;
						}
					}
				}
				if ((count > shortSz) || (count == 0))
				{
					m_pPlayer->SetVariable("_root.Root.EnemyList.EnemyCount1.EnemyText.text", stmp.Format("%d players",count).c_str());
					m_pPlayer->SetVariable("_root.Root.EnemyList.EnemyCount2.EnemyText.text", stmp.Format("").c_str());
					m_pPlayer->SetVariable("_root.Root.EnemyList.EnemyCount3.EnemyText.text", stmp.Format("").c_str());
					m_pPlayer->SetVariable("_root.Root.EnemyList.EnemyCount4.EnemyText.text", stmp.Format("").c_str());
					m_pPlayer->SetVariable("_root.Root.EnemyList.EnemyCount5.EnemyText.text", stmp.Format("").c_str());
				}
				else
				{
					SetRemainingText(1, "_root.Root.EnemyList.EnemyCount1.EnemyText.text", shortlist, count);
					SetRemainingText(2, "_root.Root.EnemyList.EnemyCount2.EnemyText.text", shortlist, count);
					SetRemainingText(3, "_root.Root.EnemyList.EnemyCount3.EnemyText.text", shortlist, count);
					SetRemainingText(4, "_root.Root.EnemyList.EnemyCount4.EnemyText.text", shortlist, count);
					SetRemainingText(5, "_root.Root.EnemyList.EnemyCount5.EnemyText.text", shortlist, count);
				}
			}
		}
	}
}

void CHUD_AONScoreElement::SetRemainingText(const int slot, const char func[], const EntityId list[], const int count)
{
	string  stmp;
	if (slot <= count)
	{
		IEntity*  e = gEnv->pEntitySystem->GetEntity(list[slot - 1]);
		stmp.Format("%s",(e?e->GetName():"?"));
	}
	else
		stmp.Format("");
	m_pPlayer->SetVariable(func, stmp.c_str());
}

void CHUD_AONScoreElement::Update( float frameTime )
{
	CRY_ASSERT_MESSAGE( m_pPlayer, "aon scoreelement not found but ticking!" );

	UpdateData();
	m_pPlayer->Update(frameTime);
}

void CHUD_AONScoreElement::Draw( void )
{
	if( !m_pPlayer )
		return;

	m_pPlayer->Draw();
}

void CHUD_AONScoreElement::Init()
{
	CHUDObject::Init();
	m_pPlayer = GetAsset("aonScoreElement");

	m_firstUpdate = true;

	m_roundTime = -1;
	m_ownScore = -1;
	m_ownRank = -1;
	m_numStats = -1;
	m_numAlive = -1;
	m_numEliminated = -1;
	m_lives = -1;
}

void CHUD_AONScoreElement::OnHUDEvent( const SHUDEvent& event )
{
#if 0  // (example)
	if(event.eventType == eHUDEvent_OnStartCarryingFlag)
	{
		m_carryingEnemyFlag = (event.eventIntData == 1);
	}
#endif
}
