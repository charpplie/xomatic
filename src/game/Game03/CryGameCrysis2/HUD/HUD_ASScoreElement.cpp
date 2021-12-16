#include "StdAfx.h"

#include "HUD_ASScoreElement.h"

#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"

#include "HUD/HUD_Impl.h"
#include "HUD/HUD.h"

#include "Utility/CryWatch.h"

static string  s_tmpGameStateWatch;  // temp! (see CRY_TODO in UpdateGameStatePrompt())
static bool  s_tmpDownloadingCogWatch;  // temp! (see CRY_TODO in BooleanFlashUpdates())

CHUD_AsScoreElement::CHUD_AsScoreElement()
{
	m_pPlayer = NULL;
	m_numTerminalIdxs = 0;

	Reset();
}

CHUD_AsScoreElement::~CHUD_AsScoreElement()
{
}

void CHUD_AsScoreElement::Reset()
{
	m_isResetUpdate = true;

	m_ownTeamScore = 0;
	m_enemyTeamScore = 0;
	m_ownScore = 0;
	m_ownTeam = 0;
	m_roundTime = 0;
	m_scoreLimit = 0;
	m_isAttacking = true;

	m_overallProgress = 0.f;

	m_gameStatePromptNeedsUpdate = true;
	m_overallProgressUpdated = true;
	m_localDownloading = false;
	m_localDownloadedLastFrame = false;


	s_tmpGameStateWatch = "";
	s_tmpDownloadingCogWatch = false;
}

void CHUD_AsScoreElement::UpdateData( void )
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

	IGameRulesRoundsModule *pRoundsModule = pGameRules->GetRoundsModule();
	CRY_ASSERT(pRoundsModule);
	if (!pRoundsModule)
		return;

	int ownTeamScore = 0;
	int enemyTeamScore = 0;
	int ownScore = 0;
	int roundTime = 0;
	int ownTeam = 0;
	bool isAttacking = true;

	ownTeam = pGameRules->GetTeam(pClientActor->GetEntityId());
	if (!ownTeam)		// If we haven't got a team (just joined or in spectator mode), default to 1 (since it's only used to determine which way round to show the scores)
	{
		ownTeam = 1;
	}

	CRY_ASSERT(ownTeam == 1 || ownTeam == 2);

	int  attackingTeamId = pRoundsModule->GetPrimaryTeam();
	CRY_ASSERT(attackingTeamId > 0);

	isAttacking = (ownTeam == attackingTeamId);

	ownTeamScore = pGameRules->GetTeamsScore(ownTeam);
	enemyTeamScore = pGameRules->GetTeamsScore(1 + 2 - ownTeam);

	IGameRulesStateModule *stateModule = pGameRules->GetStateModule();
	if (!stateModule || stateModule->GetGameState() != IGameRulesStateModule::EGRS_PreGame)
	{
		roundTime = pGameRules->IsTimeLimited() ? (int)floor(pGameRules->GetRemainingGameTime()) : (int)floor(pGameRules->GetCurrentGameTime());
	}

	if (isAttacking != m_isAttacking)
	{
		m_isAttacking = isAttacking;
		m_gameStatePromptNeedsUpdate = true;;
	}

	if(	m_isResetUpdate ||
		ownTeamScore!=m_ownTeamScore ||
		enemyTeamScore!=m_enemyTeamScore ||
		ownScore!=m_ownScore ||
		roundTime!=m_roundTime ||
		ownTeam!=m_ownTeam)
	{
		m_ownTeamScore = ownTeamScore;
		m_enemyTeamScore = enemyTeamScore;
		m_ownScore = ownScore;
		m_roundTime = roundTime;
		m_ownTeam = ownTeam;
		UpdateFlash();
	}

	if (m_isResetUpdate || m_gameStatePromptNeedsUpdate)
	{
		UpdateGameStatePrompt();
	}

	BooleanFlashUpdates();
}

void CHUD_AsScoreElement::UpdateFlash( void )
{
	// Team
	SFlashVarValue teamID( m_ownTeam );
	m_pPlayer->Invoke( "setTeam", teamID );	

	// Scores
	SFlashVarValue ownScore( m_ownTeamScore );
	SFlashVarValue opponentScore( m_enemyTeamScore );
	m_pPlayer->Invoke( "setOwnScore", ownScore );
	m_pPlayer->Invoke( "setEnemyScore", opponentScore );

	// Timers
	SFlashVarValue timearg(m_roundTime);
	m_pPlayer->Invoke("setValues", &timearg, 1);
}

void CHUD_AsScoreElement::BooleanFlashUpdates( void )
{
	// Progress
	if (m_overallProgressUpdated)
	{
		CRY_TODO(24,02,2010, "[AS HUD] here's where the overall progress indicator should be updated - the overall progress watch in Update() can be removed once this is working");
	}

	// Terminal statuses
	CRY_TODO(24,02,2010, "[AS HUD] here's where the terminal statuses (above the game timer) should be turned on and off - the loop of watches in Update() can be removed once this is working");
	if (m_overallProgressUpdated)
	{
		CRY_ASSERT(m_numTerminalIdxs <= CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES);
		for (int i=0; i<m_numTerminalIdxs; i++)
		{
			STerminalInfo*  tinf = &m_terminalInfos[i];
			STerminalInfo*  lasttinf = &m_terminalInfosLastFrame[i];
			if (tinf->beingCaptured && !lasttinf->beingCaptured)
			{
				// turn on for this terminal idx
			}
		}
	}
	else
	{
		for (int i=0; i<m_numTerminalIdxs; i++)
		{
			// turn off for this terminal idx
		}
	}

	// Local downloading
	CRY_TODO(24,02,2010, "[AS HUD] here's where the 'Downloading... [cog anim]' element should be turned on and off - 's_tmpDownloadingCogWatch' and everything that references it can be removed once this is working");
	if (m_localDownloading)
	{
		s_tmpDownloadingCogWatch = true;
	}
	else if (m_localDownloadedLastFrame)
	{
		s_tmpDownloadingCogWatch = false;
	}

	// Terminal statuses
	if (m_overallProgressUpdated)
	{
		CRY_TODO(24,02,2010, "[AS HUD] here's where the overall progress indicator should be updated - the overall progress watch in Update() can be removed once this is working");
	}
}

void CHUD_AsScoreElement::UpdateGameStatePrompt()
{
	const wchar_t* localizedText = L"";
	ColorF col = Col_White;
	int state = 0;
	CHUD* pHud = g_pGame->GetHUD();

	if (m_isAttacking)
	{
		s_tmpGameStateWatch = pHud->LocalizeString( "@ui_msg_as_capture_state", NULL, NULL );
		localizedText =      pHud->LocalizeStringW( "@ui_msg_as_capture_state", NULL, NULL );
		state = ECTFHud_Enemy;
	}
	else
	{
		if (m_overallProgressUpdated && (m_overallProgress > 0.f))
		{
			s_tmpGameStateWatch = pHud->LocalizeString( "@ui_msg_as_protect_state", NULL, NULL );
			localizedText =      pHud->LocalizeStringW( "@ui_msg_as_protect_state", NULL, NULL );
			state = ECTFHud_Friendly;
		}
		else
		{
			s_tmpGameStateWatch = pHud->LocalizeString( "@ui_msg_as_defend_state", NULL, NULL );
			localizedText =      pHud->LocalizeStringW( "@ui_msg_as_defend_state", NULL, NULL );
			state = ECTFHud_Friendly;
		}
	}

	CRY_TODO(23,02,2010, "[AS HUD] here's where the game state prompt is invoked - 's_tmpGameStateWatch' and everything that references it can be removed once this is working");
	SFlashVarValue args[3] = {localizedText, state, col.pack_rgb888()};
	m_pPlayer->Invoke("setGameStatePrompt", args, 3);
}

void CHUD_AsScoreElement::Update( float frameTime )
{
	CRY_ASSERT_MESSAGE( m_pPlayer, "asScoreElement not found but ticking!" );

	UpdateData();
	m_pPlayer->Update(frameTime);

#if CRY_WATCH_ENABLED
	{
		CryWatch("***** Game State Prompt: %s", s_tmpGameStateWatch.c_str());  // temp! (see CRY_TODO in UpdateGameStatePrompt())

		// temp! (see CRY_TODO in BooleanFlashUpdates())
		CRY_ASSERT(m_numTerminalIdxs <= CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES);
		for (int i=0; i<m_numTerminalIdxs; i++)
		{
			IEntity*  tent = gEnv->pEntitySystem->GetEntity(m_terminalEids[i]);
			CRY_ASSERT_MESSAGE(tent, "Assault mode doesn't expect teminals to be removed!");
			if (tent)
			{
				STerminalInfo*  tinf = &m_terminalInfos[i];
				CryWatch("***** Terminal %d '%s' Status: %s", i, tent->GetName(), (tinf->beingCaptured ? "BEING CAPTURED!!!!!" : "ok"));
			}
		}

		CryWatch("***** Overall Download Progress: %.0f%%", (m_overallProgress * 100.f));  // temp! (see CRY_TODO in BooleanFlashUpdates())

		// temp! (see CRY_TODO in BooleanFlashUpdates())
		if (s_tmpDownloadingCogWatch)
		{
			CryWatch("***** You Are DOWNLOADING!!!");
		}
	}
#endif

	// end of update...
	memcpy(m_terminalInfosLastFrame, m_terminalInfos, sizeof(m_terminalInfosLastFrame));
	memset(m_terminalInfos, 0, sizeof(m_terminalInfos));
	m_gameStatePromptNeedsUpdate = m_overallProgressUpdated;  // ie. the game state prompt also needs to be updated the frame after the overall progress has stopped updating
	m_overallProgressUpdated = false;
	m_localDownloadedLastFrame = m_localDownloading;
	m_localDownloading = false;
	m_isResetUpdate = false;
}

void CHUD_AsScoreElement::Draw( void )
{
	if( !m_pPlayer )
		return;

	m_pPlayer->Draw();
}

void CHUD_AsScoreElement::Initialize( const IItemParamsNode * object )
{
	CHUDObject::Initialize( object );
	m_pPlayer = GetAsset("asScoreElement");
}

void CHUD_AsScoreElement::OnHUDEvent(const SHUDEvent& event )
{
	switch (event.eventType)
	{
	case eHUDEvent_OnRoundStart:
		{
			Reset();
			break;
		}
	case eHUDEvent_OnCaptureObjectiveNumChanged:
		{
			int  numActiveEntities = event.GetData(0).GetInt();
			int  lastActiveCount = event.GetData(1).GetInt();

			if (numActiveEntities > lastActiveCount)
			{
				int  newTerminalIdx = event.GetData(2).GetInt();
				int  newTerminalEid = event.GetData(3).GetInt();

				CRY_ASSERT(newTerminalIdx >= 0);
				CRY_ASSERT(newTerminalIdx < CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES);

				if (newTerminalIdx >= m_numTerminalIdxs)
					m_numTerminalIdxs = (newTerminalIdx + 1);

				m_terminalEids[newTerminalIdx] = newTerminalEid;
			}

			break;
		}
	case eHUDEvent_OnSiteBeingCaptured:
		{
			int  terminalIdx = event.eventIntData;
			EntityId  terminalEntId = (EntityId) event.eventIntData2;
			bool  localDownloading = (event.eventIntData3 == 1);
			//float  terminalProgress = pCaptureEntity->m_captureProgress.m_progress = event.eventFloatData;
			float  overallProgress = event.eventFloat2Data;

			CRY_ASSERT(terminalIdx >= 0);
			CRY_ASSERT(terminalIdx < CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES);

			CRY_ASSERT(terminalEntId == m_terminalEids[terminalIdx]);

			m_terminalInfos[terminalIdx].beingCaptured = 1;
			m_overallProgress = overallProgress;
			m_localDownloading = localDownloading;

			m_overallProgressUpdated = true;
			m_gameStatePromptNeedsUpdate = true;

			break;
		}
	case eHUDEvent_OnOverallCaptureProgressUpdate:
		{
			float  overallProgress = event.GetData(0).GetFloat();
			m_overallProgress = overallProgress;
			m_overallProgressUpdated = true;
			break;
		}
	}
}
