/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 02:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardTwoTeams.h"
#include "IXml.h"
#include "GameRules.h"
#include "Player.h"

#define STANDARDTWOTEAMS_AUTO_BALANCE_TIME				10.f
#define STANDARDTWOTEAMS_AUTO_BALANCE_WARNING_TIME		5.f

#if NUM_ASPECTS > 8
	#define STANDARD_TEAMS_ASPECT			eEA_GameServerB
#else
	#define STANDARD_TEAMS_ASPECT			eEA_GameServerStatic
#endif

//-------------------------------------------------------------------------
CGameRulesStandardTwoTeams::CGameRulesStandardTwoTeams()
{
	CryLog("CGameRulesStandardTwoTeams::CGameRulesStandardTwoTeams()");

	m_pGameRules = NULL;
}

//-------------------------------------------------------------------------
CGameRulesStandardTwoTeams::~CGameRulesStandardTwoTeams()
{
	CryLog("CGameRulesStandardTwoTeams::~CGameRulesStandardTwoTeams()");
}

//-------------------------------------------------------------------------
void CGameRulesStandardTwoTeams::Init( XmlNodeRef xml )
{
	CryLog("CGameRulesStandardTwoTeams::Init()");
	m_pGameRules = g_pGame->GetGameRules();

	m_pGameRules->CreateTeam("tan");
	m_pGameRules->CreateTeam("black");

	m_sleepTime = 0.f;
	m_autoBalanceState = EABS_OFF;

	int timeRemaining = int(STANDARDTWOTEAMS_AUTO_BALANCE_WARNING_TIME);
	m_timeToBalance.Format("%i", timeRemaining);
}

//-------------------------------------------------------------------------
void CGameRulesStandardTwoTeams::PostInit()
{
	CryLog("CGameRulesStandardTwoTeams::PostInit()");
}

//-------------------------------------------------------------------------
void CGameRulesStandardTwoTeams::RequestChangeTeam(EntityId playerId, int teamId)
{
	CryLog("CGameRulesStandardTwoTeams::RequestChangeTeam()");
	int currentTeam = m_pGameRules->GetTeam(playerId);

	if (g_pGameCVars->g_autoteambalance)
	{
		// Check if teams would be balanced if the player switched teams
		if (currentTeam)
		{
			int oldTeamPlayerCount = m_pGameRules->GetTeamPlayerCount(currentTeam) - 1;		// Remove player who is switching team
			int newTeamPlayerCount = m_pGameRules->GetTeamPlayerCount(teamId) + 1;			// Include player switching team

			if (newTeamPlayerCount > oldTeamPlayerCount + g_pGameCVars->g_autoTeamBalanceAmount)
			{
				CryLog("Refusing team change, would leave old team with %i players and new team with %i players", oldTeamPlayerCount, newTeamPlayerCount);
				return;
			}
		}
		else
		{
			int otherTeamId = m_pGameRules->GetEnemyTeamId(teamId);

			int enemyTeamPlayerCount = m_pGameRules->GetTeamPlayerCount(otherTeamId);
			int newTeamPlayerCount = m_pGameRules->GetTeamPlayerCount(teamId) + 1;			// Include player switching team

			if (newTeamPlayerCount > enemyTeamPlayerCount + g_pGameCVars->g_autoTeamBalanceAmount)
			{
				CryLog("Refusing team change, would leave old team with %i players and new team with %i players", enemyTeamPlayerCount, newTeamPlayerCount);
				return;
			}
		}
	}
	DoTeamChange(playerId, teamId);
}

//-------------------------------------------------------------------------
void CGameRulesStandardTwoTeams::DoTeamChange(EntityId playerId, int teamId)
{
	// Change team
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	if (pActor && pActor->IsPlayer())
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(pActor);
		if (pPlayer && pPlayer->GetHealth() > 0)
		{
			m_pGameRules->KillPlayer(pActor, true, true, playerId, playerId, 0.f, -1, 0, Vec3(ZERO), 0);
		}
	}
	m_pGameRules->SetTeam(teamId, playerId);
}

//-------------------------------------------------------------------------
int CGameRulesStandardTwoTeams::GetAutoAssignTeamId(EntityId playerId)
{
	int team1Players = m_pGameRules->GetTeamPlayerCount(1);
	int team2Players = m_pGameRules->GetTeamPlayerCount(2);

	if (team1Players > team2Players)
	{
		return 2;
	}
	return 1;
}

//-------------------------------------------------------------------------
void CGameRulesStandardTwoTeams::Update( float frameTime )
{
	if (gEnv->bServer)
	{
		bool aspectChanged = false;

		// Check for CVar changes
		if (m_autoBalanceState == EABS_OFF && g_pGameCVars->g_autoteambalance)
		{
			m_sleepTime = 0.f;
			m_autoBalanceState = EABS_CHECKING;
			aspectChanged = true;
		}
		else if (m_autoBalanceState != EABS_OFF && !g_pGameCVars->g_autoteambalance)
		{
			m_autoBalanceState = EABS_OFF;
			aspectChanged = true;
		}

		if (m_autoBalanceState == EABS_CHECKING)
		{
			m_sleepTime -= frameTime;
			if (m_sleepTime < 0.f)
			{
				int team1Count = m_pGameRules->GetTeamPlayerCount(1);
				int team2Count = m_pGameRules->GetTeamPlayerCount(2);
				int difference = abs(team1Count - team2Count);
				if (difference > g_pGameCVars->g_autoTeamBalanceAmount)
				{
					m_autoBalanceState = EABS_BALANCING;
					m_sleepTime = STANDARDTWOTEAMS_AUTO_BALANCE_WARNING_TIME;
					aspectChanged = true;

					if (gEnv->bClient)
					{
						m_pGameRules->OnTextMessage(eTextMessageAnnouncement, "@ui_msg_auto_balance", m_timeToBalance.c_str());
					}
				}
				else
				{
					m_sleepTime = STANDARDTWOTEAMS_AUTO_BALANCE_TIME;
				}
			}
		}
		else if (m_autoBalanceState == EABS_BALANCING)
		{
			m_sleepTime -= frameTime;
			if (m_sleepTime < 0.f)
			{
				CryLog("CGameRulesStandardTwoTeams::Update(), doing team auto balance");
				// Do auto balancing
				int team1Count = m_pGameRules->GetTeamPlayerCount(1);
				int team2Count = m_pGameRules->GetTeamPlayerCount(2);

				int difference = abs(team1Count - team2Count);
				if (difference > g_pGameCVars->g_autoTeamBalanceAmount)
				{
					int fromTeamId = (team1Count > team2Count) ? 1 : 2;
					int toTeamId = 3 - fromTeamId;
					int playersToMove = difference / 2;

					CryLog("CGameRulesStandardTwoTeams::Update(), team %i has %i more players than team %i, changing %i of them", fromTeamId, difference, toTeamId, playersToMove);

					CGameRules::TPlayers players;
					m_pGameRules->GetTeamPlayers(fromTeamId, players);

					for (int i = 0; i < playersToMove; ++ i)
					{
						CRY_TODO(07, 10, 2009, "[CG] Choose which player to switch a little better, i.e. last to join / attempt to balance player scores on each team");
						DoTeamChange(players[i], toTeamId);
					}
				}

				m_autoBalanceState = EABS_CHECKING;
				m_sleepTime = STANDARDTWOTEAMS_AUTO_BALANCE_TIME;
				aspectChanged = true;
			}
		}

		if (aspectChanged)
		{
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(m_pGameRules->GetEntityId(), STANDARD_TEAMS_ASPECT);
		}
	}
}

//-------------------------------------------------------------------------
bool CGameRulesStandardTwoTeams::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == STANDARD_TEAMS_ASPECT)
	{
		int state = int(m_autoBalanceState);

		ser.Value("state", state, 'ui2');

		if (ser.IsReading())
		{
			EAutoBalanceState newState = (EAutoBalanceState) state;
			if (newState != m_autoBalanceState)
			{
				m_autoBalanceState = newState;
				if (m_autoBalanceState == EABS_BALANCING)
				{
					m_pGameRules->OnTextMessage(eTextMessageAnnouncement, "@ui_msg_auto_balance", m_timeToBalance.c_str());
				}
			}
		}
	}
	return true;
}
