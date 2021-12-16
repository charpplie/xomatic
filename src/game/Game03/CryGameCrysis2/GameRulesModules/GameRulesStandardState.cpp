/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardState.h"
#include "IXml.h"
#include "GameRules.h"
#include "GameCVars.h"
#include "IGameObject.h"
#include "ISerialize.h"
#include "GameRulesModules/IGameRulesStatsRecording.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "HUD/HUD.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "PlayerProgression.h"
#include "Actor.h"

#if NUM_ASPECTS > 8
	#define STANDARD_STATE_ASPECT			eEA_GameServerC
#else
	#define STANDARD_STATE_ASPECT			eEA_GameServerStatic
#endif

//-------------------------------------------------------------------------
CGameRulesStandardState::CGameRulesStandardState()
{
	CryLog("CGameRulesStandardState::CGameRulesStandardState()");

	m_pGameRules = NULL;
}

//-------------------------------------------------------------------------
CGameRulesStandardState::~CGameRulesStandardState()
{
	IConsole *pConsole = gEnv->pConsole;
	pConsole->RemoveCommand("g_setgamestate");

	if (m_scoreboardShown)
	{
		SAFE_HUD_FUNC(ForceScoreBoard(false));
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::Init( XmlNodeRef xml )
{
	m_pGameRules = g_pGame->GetGameRules();

	m_state = EGRS_PreGame;
	m_scoreboardTime = 0.f;
	m_nextLevelTime = 0.f;
	m_minPlayerWarningTime = 0.f;
	m_forceInGame = false;
	m_isStarting = false;
	m_scoreboardShown = false;
	m_introMessageShown = false;
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::PostInit()
{
	REGISTER_COMMAND("g_setgamestate", CmdSetState, VF_NULL, "Force game state (0=Reset, 1=PreGame, 2=InGame, 3=PostGame");
}

//-------------------------------------------------------------------------
CGameRulesStandardState::EGR_GameState CGameRulesStandardState::GetGameState()
{
	return m_state;
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::Update( float frameTime )
{
	if (gEnv->bServer)
	{
		if (m_state == EGRS_PreGame)
		{
			bool hasRequiredPlayers = HasRequiredPlayers();

			if (hasRequiredPlayers && !m_isStarting)
			{
				CryLog("CGameRulesStandardState::Update(), now have required players");
				m_isStarting = true;
				m_pGameRules->ResetGameStartTimer(g_pGame->GetCVars()->g_gameRules_startTimerLength);
				g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(m_pGameRules->GetEntityId(), STANDARD_STATE_ASPECT);
			}
			else if (!hasRequiredPlayers && m_isStarting)
			{
				CryLog("CGameRulesStandardState::Update(), no longer have required players");
				m_isStarting = false;
				m_pGameRules->ResetGameStartTimer(-1.f);
				g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(m_pGameRules->GetEntityId(), STANDARD_STATE_ASPECT);
			}

			if (m_isStarting && m_pGameRules->GetRemainingStartTimer() <= 0.f)
			{
				CryLog("CGameRulesStandardState::Update(), restarting game");

				m_pGameRules->Restart();

#if 0			// go through CGameRules::Restart() to ensure other restart hooks are called!
				m_forceInGame = true;

				ChangeState(EGRS_Reset);
				m_pGameRules->ResetEntities();
#endif
			}
		}
		else if (m_state == EGRS_PostGame)
		{
			if (m_pGameRules->GetServerTime() > m_nextLevelTime)
			{
				m_pGameRules->NextLevel();

				ChangeState(EGRS_Reset);
			}
		}
	}
	if (gEnv->bClient)
	{
		if (m_state == EGRS_PreGame)
		{
#if 0	// TODO: Re-enable when there is better HUD text support - 1 message region for a single message
			if (!m_isStarting && (m_pGameRules->GetServerTime() > m_minPlayerWarningTime))
			{
				if (m_pGameRules->GetTeamCount())
				{
					CryFixedStringT<8> requiredPlayers;
					requiredPlayers.Format("%i", g_pGame->GetCVars()->g_minteamlimit);
					m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_MinTeamWarning", requiredPlayers.c_str());
				}
				else
				{
					CryFixedStringT<8> requiredPlayers;
					requiredPlayers.Format("%i", g_pGame->GetCVars()->g_minplayerlimit);
					m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_MinPlayerWarning", requiredPlayers.c_str());
				}

				m_minPlayerWarningTime = m_pGameRules->GetServerTime() + GR_STATE_MIN_PLAYER_WARNING_TIMER_LENGTH;
			}
			else if (m_isStarting)
			{
				int time = int(floor(m_pGameRules->GetRemainingStartTimer() + 0.5f));
				if (time > 0)
				{
					CryFixedStringT<8> startTime;
					startTime.Format("%i", time);
					m_pGameRules->OnTextMessage(eTextMessageCenter, "@mp_GameStartingCountdown", startTime.c_str());
				}
			}
#else
			if( !gEnv->pSystem->IsDedicated() )
			{
				SHUDEvent prematchEvent;
				if (!m_isStarting)
				{
					prematchEvent.eventType = eHUDEvent_OnWaitingForPlayers;
				}
				else
				{
					int time = int(floor(m_pGameRules->GetRemainingStartTimer() + 0.5f));
					prematchEvent.eventType = eHUDEvent_OnUpdateGameStartMessage;
					prematchEvent.AddData(time);
				}
				CHUD::CallEvent(prematchEvent);
			}
#endif
		}
		else if (m_state == EGRS_InGame && !gEnv->pSystem->IsDedicated() )
		{
			if (m_introMessageShown == false)	// Show only once
			{
				CGameRules *pGameRules = g_pGame->GetGameRules();
				if (pGameRules)
				{
					if (EntityId localPlayerId = g_pGame->GetIGameFramework()->GetClientActorId())
					{
						int teamId = g_pGame->GetGameRules()->GetTeam(localPlayerId);
						bool bTeamGame = (pGameRules->GetTeamCount() > 1);

						IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor();
						if (pActor->GetSpectatorMode()==CActor::eASM_None && pActor->GetHealth()>0.f && (!bTeamGame || teamId!=0))
						{
							if (IGameRulesPlayerStatsModule *statsModule = pGameRules->GetPlayerStatsModule())
							{
								const SGameRulesPlayerStat *stats = statsModule->GetPlayerStats(localPlayerId);
								if (stats->deaths <= 0) // Not died ever
								{
									CryFixedStringT<32> strGameMode;
									if (bTeamGame)
									{
										strGameMode.Format("@ui_rules_%s", g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName());
										strGameMode = g_pGame->GetHUD()->LocalizeString(strGameMode.c_str(), NULL, NULL);

										CryFixedStringT<16> strTeamName;
										strTeamName.Format("@ui_hud_team_%d", teamId);
										strTeamName = g_pGame->GetHUD()->LocalizeString(strTeamName.c_str(), NULL, NULL);

										CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", "@ui_enter_gamemode_team_message", strGameMode.c_str(), strTeamName.c_str(), NULL, 2.f);
									}
									else
									{
										strGameMode.Format("@ui_rules_%s", g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName());
										CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", strGameMode.c_str(), NULL, NULL, NULL, 2.f);
									}
								}
							}

							m_introMessageShown = true; // Or not if has already died, but don't check again anyway.
						}
					}
				}
			}
		}
		else if (m_state == EGRS_PostGame)
		{
			if (!m_scoreboardShown && (m_pGameRules->GetServerTime() > m_scoreboardTime))
			{
				
				//SAFE_HUD_FUNC(ForceScoreBoard(true));

				SHUDEvent hudEventSelectMenu;
				hudEventSelectMenu.eventType = eHUDEvent_GameEnded;
				CHUD::CallEvent(hudEventSelectMenu);

				//SHUDEvent hudEventScoreboard;
				//hudEventScoreboard.eventType = eHUDEvent_ShowScoreboard;
				//hudEventScoreboard.eventPtrData = (void *)"Normal";
				//CHUD::CallEvent(hudEventScoreboard);

				m_scoreboardShown = true;

				g_pGame->GetHUD()->ActivateState("mp_endgame");
			}
		}
	}
}

//-------------------------------------------------------------------------
bool CGameRulesStandardState::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == STANDARD_STATE_ASPECT)
	{
		uint8 state = (uint8) m_state;
		bool isStarting = m_isStarting;
		ser.Value("state", state, 'ui2');
		ser.Value("isStarting", isStarting, 'bool');

		if (ser.IsReading())
		{
			EGR_GameState newState = (EGR_GameState) state;
			if (m_state != newState)
			{
				CryLog("CGameRulesStandardState::NetSerialize(), state changed from %i to %i", (int)m_state, (int)newState);
				ChangeState(newState);
			}

			if (m_isStarting != isStarting)
			{
				m_isStarting = isStarting;
				if (m_isStarting)
				{
					m_minPlayerWarningTime = m_pGameRules->GetServerTime() + g_pGame->GetCVars()->g_gameRules_minPlayerWarningTimerLength;
				}
			}
		}
	}

	return true;
}

// called when the CGameRules is Restart()-ing
void CGameRulesStandardState::OnGameRestart()
{
	m_forceInGame=true;
	ChangeState(EGRS_Reset);
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::OnGameReset()
{
	if (gEnv->bServer)
	{
		EGR_GameState newState = EGRS_PreGame;

		if (m_forceInGame)
		{
			m_forceInGame=false;
			newState = EGRS_InGame;
			CryLog("CGameRulesStandardState::OnGameReset(), setting state to InGame (forced)");
		}
		else
		{
			if (HasRequiredPlayers())
			{
				newState = EGRS_InGame;
				CryLog("CGameRulesStandardState::OnGameReset(), setting state to InGame (not forced)");
			}
			else
			{
				m_isStarting = false;
				CryLog("CGameRulesStandardState::OnGameReset(), setting state to PreGame");
			}
		}

		ChangeState(newState);
	}
	else
	{
		// Predict that we're going to end up in InGame
		if (m_state != EGRS_InGame)
		{
			ChangeState(EGRS_InGame);
			m_pGameRules->ResetGameTime();
		}
	}
}

//-------------------------------------------------------------------------
bool CGameRulesStandardState::HasRequiredPlayers() const
{
	bool hasRequiredPlayers = true;

	int numTeams = m_pGameRules->GetTeamCount();
	if (numTeams)
	{
		int requiredPlayers = g_pGame->GetCVars()->g_minteamlimit;
		for (int i = 1; i <= numTeams; ++ i)
		{
			if (m_pGameRules->GetTeamPlayerCount(i) < requiredPlayers)
			{
				hasRequiredPlayers = false;
				break;
			}
		}
	}
	else
	{
		int requiredPlayers = g_pGame->GetCVars()->g_minplayerlimit;
		if (m_pGameRules->GetPlayerCount() < requiredPlayers)
		{
			hasRequiredPlayers = false;
		}
	}
	return hasRequiredPlayers;
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::OnGameEnd()
{
	if (gEnv->bServer)
	{
		ChangeState(EGRS_PostGame);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::ChangeState( EGR_GameState newState )
{
	if (gEnv->bServer)
	{
		if (newState == EGRS_PreGame)
		{
			m_isStarting = false;
		}
		else if (newState == EGRS_InGame)
		{
			m_pGameRules->ResetGameTime();
			IGameRulesStatsRecording	*st=m_pGameRules->GetStatsRecordingModule();
			if (st)
			{
				st->OnInGameBegin();
			}
		}
		else if (newState == EGRS_PostGame)
		{
			m_nextLevelTime = m_pGameRules->GetServerTime() + g_pGame->GetCVars()->g_gameRules_nextLevelTimerLength;
			IGameRulesStatsRecording	*st=m_pGameRules->GetStatsRecordingModule();
			if (st)
			{
				st->OnPostGameBegin();
			}

			m_pGameRules->FreezeInput(true);
		}
	}

	if (gEnv->bClient)
	{
		if (newState == EGRS_PostGame)
		{
			m_scoreboardTime = m_pGameRules->GetServerTime() + g_pGame->GetCVars()->g_gameRules_showScoreboardTimerLength;
			m_scoreboardShown = false;
			
			m_pGameRules->FreezeInput(true);
		}
		else if (newState == EGRS_InGame)
		{
			CHUD::CallEvent( SHUDEvent(eHUDEvent_OnGameStart) );
		}
		else if (newState == EGRS_Reset)
		{
			m_introMessageShown = false;
		}
		//SAFE_HUD_FUNC(ForceScoreBoard(false));
	}

	m_state = newState;
	if (gEnv->bServer)
	{
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(m_pGameRules->GetEntityId(), STANDARD_STATE_ASPECT);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardState::CmdSetState( IConsoleCmdArgs *pArgs )
{
	if (gEnv->bServer)
	{
		const char *pState = pArgs->GetArg(1);
		if (pState)
		{
			int state = atoi(pState);
			if (state >= 0 && state < (int) EGRS_MAX)
			{
				EGR_GameState requestedState = (EGR_GameState) state;

				CGameRulesStandardState *pStateModule = static_cast<CGameRulesStandardState *>(g_pGame->GetGameRules()->GetStateModule());
				if (pStateModule)
				{
					pStateModule->ChangeState(requestedState);
				}
			}
		}
	}
}
