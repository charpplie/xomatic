#include "StdAfx.h"

#include "HUD_CTFScoreElement.h"

#include "HUD/HUD.h"
#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "Utility/CryWatch.h"

CHUD_CtfScoreElement::CHUD_CtfScoreElement()
{
	m_pPlayer = NULL;

	m_ownTeamScore = 0;
	m_enemyTeamScore = 0;
	m_ownScore = 0;
	m_ownTeam = 0;
	m_roundTime = 0;
	m_scoreLimit = 0;
	m_ownFlagState = m_lastOwnFlagState = 0;
	m_opponentFlagState = m_lastopponentFlagState = 0;
	m_carryingEnemyFlag = false;
	m_lastCarryingEnemyFlag = false;

	m_friendCarryingEnemyFlag = false;
	m_enemyCarryingFriendlyFlag = false;
}

CHUD_CtfScoreElement::~CHUD_CtfScoreElement()
{

}

// Ripped from PreCXP 'HUDTeamInstantAction.cpp'
void CHUD_CtfScoreElement::UpdateData( void )
{
	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if(!pClientActor)
		return;

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(!pGameRules)
		return;

	int ownTeamScore = 0;
	int enemyTeamScore = 0;
	int ownScore = 0;
	int roundTime = 0;
	int scoreLimit = 100;
	int ownTeam = 0;

	ownTeam = pGameRules->GetTeam(pClientActor->GetEntityId());
	if( !ownTeam ) // team is sometime zero when [re-]joining game.
	{
		ownTeam = 1;// courtesy of colG.
	}

	CRY_ASSERT(ownTeam == 1 || ownTeam == 2);

	// Get Scores
	ownTeamScore = pGameRules->GetTeamsScore(ownTeam);
	enemyTeamScore = pGameRules->GetTeamsScore(1 + 2 - ownTeam);

	IGameRulesStateModule *stateModule = pGameRules->GetStateModule();
	if (!stateModule || stateModule->GetGameState() != IGameRulesStateModule::EGRS_PreGame)
	{
		roundTime = pGameRules->IsTimeLimited() ? (int)floor(pGameRules->GetRemainingGameTime()) : (int)floor(pGameRules->GetCurrentGameTime());
	}
	scoreLimit = g_pGameCVars->g_fraglimit;

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

	if( m_carryingEnemyFlag != m_lastCarryingEnemyFlag )
	{
		m_lastCarryingEnemyFlag = m_carryingEnemyFlag ;

		// Update game state message.
		UpdateGameStatePrompt( g_pGame->GetGameRules(), m_ownTeam, m_carryingEnemyFlag );
	}
}

void CHUD_CtfScoreElement::UpdateGameStatePrompt(CGameRules* pGameRules, int iTeam, bool bFlagCarrier)
{
	const wchar_t* localizedText = L"";
	ColorF col = Col_White;
	int state = 0;

	if (iTeam == 1 || iTeam == 2)
	{
		CHUD* pHud = g_pGame->GetHUD();
		if (m_carryingEnemyFlag)
		{
			localizedText = pHud->LocalizeStringW( "@ui_msg_ctf_status_2", NULL, NULL );
			state = ECTFHud_Neutral;
		}
		else if (m_enemyCarryingFriendlyFlag)
		{
			localizedText = pHud->LocalizeStringW("@ui_msg_ctf_status_3", NULL, NULL );
			state = ECTFHud_Enemy;
		}
		else if (m_friendCarryingEnemyFlag)
		{
			localizedText = pHud->LocalizeStringW("@ui_msg_ctf_status_1", NULL, NULL );
			state = ECTFHud_Neutral;
		}
		else	// Their flag dropped or in base
		{
			localizedText = pHud->LocalizeStringW("@ui_msg_ctf_status_0", NULL, NULL );
			state = ECTFHud_Friendly;
		}
	}
	else
	{
		state = ECTFHud_Off;
	}

	unsigned int packedColor = col.pack_rgb888();

	SFlashVarValue args[3] = {localizedText, state, packedColor};
	m_pPlayer->Invoke("setGameStatePrompt", args, 3);
}

void CHUD_CtfScoreElement::UpdateFlash( void )
{
	// Team
	SFlashVarValue teamID( m_ownTeam );
	m_pPlayer->Invoke( "setTeam", teamID );	

	// Scores
	SFlashVarValue ownScore( m_ownTeamScore );
	SFlashVarValue opponentScore( m_enemyTeamScore );
	m_pPlayer->Invoke( "setOwnScore", ownScore );
	m_pPlayer->Invoke( "setEnemyScore", opponentScore );

	// Update game state.
	UpdateGameStatePrompt( g_pGame->GetGameRules(), m_ownTeam, m_carryingEnemyFlag );

	// Timers
	SFlashVarValue timearg(m_roundTime);
	m_pPlayer->Invoke("setValues", &timearg, 1);
}

void CHUD_CtfScoreElement::Update( float frameTime )
{
#if CRY_WATCH_ENABLED
	if (g_pGameCVars->g_CTFScoreElement_watchLvl > 0)
	{
		CryWatch("[CHUD_CtfScoreElement::Update()]");
		CryWatch(" carryingEnemyFlag %d, friendCarryingEnemyFlag %d, enemyCarryingFriendlyFlag %d", m_carryingEnemyFlag, m_friendCarryingEnemyFlag, m_enemyCarryingFriendlyFlag);
	}
#endif

	m_pPlayer = GetAsset("ctfScoreElement");
	CRY_ASSERT_MESSAGE( m_pPlayer, "ctf scoreelement not found but ticking!" );

	UpdateData();
	m_pPlayer->Update(frameTime);
}

void CHUD_CtfScoreElement::Draw( void )
{
	if( !m_pPlayer )
		return;

	m_pPlayer->Draw();
}

void CHUD_CtfScoreElement::Initialize( const IItemParamsNode * object )
{
	CHUDObject::Initialize( object );
}

void CHUD_CtfScoreElement::OnHUDEvent( const SHUDEvent& event )
{
	if(event.eventType == eHUDEvent_OnStartCarryingFlag)
	{
		int  flagTeam = event.eventIntData;
		EntityId  carrierEid = event.eventIntData2;
		if (flagTeam)
		{
			EntityId  localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
			int  localTeam = g_pGame->GetGameRules()->GetTeam(localActorId);
			bool  sameTeam_flag_local = (flagTeam == localTeam);
			if (carrierEid == 0)
			{
				if (!sameTeam_flag_local)
				{
					// no-one carrying enemy flag
					m_carryingEnemyFlag = false;
					m_friendCarryingEnemyFlag = false;
				}
				else
				{
					// no-one carrying our flag
					m_enemyCarryingFriendlyFlag = false;
				}
			}
			else if (carrierEid == localActorId)
			{
				if (!sameTeam_flag_local)
				{
					// local carrying enemy flag
					CRY_ASSERT(m_friendCarryingEnemyFlag == false);
					m_carryingEnemyFlag = true;
				}
				else
				{
					// local carrying our flag
					CRY_ASSERT(m_enemyCarryingFriendlyFlag == false);
				}
			}
			else
			{
				int  carrTeam = g_pGame->GetGameRules()->GetTeam(carrierEid);
				bool  sameTeam_flag_carr = (flagTeam == carrTeam);
				if (!sameTeam_flag_carr)
				{
					if (!sameTeam_flag_local)
					{
						// friend carrying enemy flag
						CRY_ASSERT(m_carryingEnemyFlag == false);
						m_friendCarryingEnemyFlag = true;
					}
					else
					{
						// enemy carrying our flag
						m_enemyCarryingFriendlyFlag = true;
					}
				}
				else
				{
					if (!sameTeam_flag_local)
					{
						// enemy carrying enemy flag
						CRY_ASSERT(m_carryingEnemyFlag == false);
						CRY_ASSERT(m_friendCarryingEnemyFlag == false);
					}
					else
					{
						// friend carrying our flag
						CRY_ASSERT(m_enemyCarryingFriendlyFlag == false);
					}
				}
			}
		}
	}
}
