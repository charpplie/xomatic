/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Multiplayer UI Handling

-------------------------------------------------------------------------
History:
- 02/2010: Created by Ben Johnson

*************************************************************************/
#include "StdAfx.h"
#include "MPMenuHub.h"
#include "Flash/Flash.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/ProfileOptions.h"
#include "Network/Lobby/GameLobby.h"
#include "Network/Lobby/GameBrowser.h"
#include "IPlayerProfiles.h"

#include "HUD/HUD.h"
#include "Menus/OptionsManager.h"
#include "PersistantStats.h"
#include "PlayerProgression.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"
#include "UILeaderboards.h"

//------------------------------------------------------------------------
CMPMenuHub::CMPMenuHub() : m_pServerList(0), m_pLeaderboards(0)
{
	m_currentScreen = eFFES_unknown;

	m_bLoadingDialog = false;

	RegisterCommand( eMPFEC_set_lobby_type,					"mp_set_lobby_type"				);
	RegisterCommand( eMPFEC_mp_load,								"mp_load"									);
	RegisterCommand( eMPFEC_serverlist_scrollUp,		"mp_serverlist_up"				);
	RegisterCommand( eMPFEC_serverlist_scrollDown,	"mp_serverlist_down"			);
	RegisterCommand( eMPFEC_serverlist_join,				"mp_serverlist_join"			);
	RegisterCommand( eMPFEC_set_serverlist_size,		"mp_set_serverlist_size"	);
	RegisterCommand( eMPFEC_leaderboard_move,				"mp_leaderboard_move"			);
	RegisterCommand( eMPFEC_leaderboard_change,			"mp_leaderboard_change"		);
	RegisterCommand( eMPFEC_leaderboard_set_size,		"mp_leaderboard_set_size"	);
	RegisterCommand( eMPFEC_leaderboard_move_to_user,			"mp_leaderboard_move_to_user"	);
	RegisterCommand( eMPFEC_leaderboard_move_to_top,			"mp_leaderboard_move_to_top"	);
}

//------------------------------------------------------------------------
CMPMenuHub::~CMPMenuHub()
{
	SAFE_DELETE(m_pServerList);
	SAFE_DELETE(m_pLeaderboards);
}

//------------------------------------------------------------------------
void CMPMenuHub::RegisterCommand(EMPMenuCommand command, const char *name)
{
	m_commands.insert(TMPCommandMap::value_type(CryStringUtils::CalculateHash(name), command));
}

//------------------------------------------------------------------------
const CMPMenuHub::EMPMenuCommand CMPMenuHub::GetCommand(const char *name) const
{
	uint32 hash = CryStringUtils::CalculateHash(name);
	TMPCommandMap::const_iterator it = m_commands.find(hash);
	if (it != m_commands.end())
		return it->second;
	else
		return eMPFEC_none;
}

void CMPMenuHub::InitIngameMenu()
{
	CRY_TODO(15, 02, 2010, "Add creation of any ingame only classes e.g. scoreboard");
}

void CMPMenuHub::DestroyIngameMenu()
{
	CRY_TODO(15, 02, 2010, "Add destroying of any ingame only classes e.g. scoreboard");
}

void CMPMenuHub::InitFrontEndMenu()
{
	CRY_ASSERT(!m_pServerList);
	m_pServerList = new CUIServerList();

#if defined(XENON)
	m_pLeaderboards = new CUILeaderboards();
#endif
}

void CMPMenuHub::DestroyFrontEndMenu()
{
	SAFE_DELETE(m_pServerList);
	SAFE_DELETE(m_pLeaderboards);
}

//------------------------------------------------------------------------
void CMPMenuHub::EnterScreen(const CFlashFrontEnd::EFlashMenus menu, const EFlashFrontEndScreens screen)
{
	m_currentScreen = screen;

	switch(screen)
	{
	case eFFES_play_online:		// deliberate fall though
	case eFFES_play_lan:
		{
			CFlashFrontEnd *flashMenu = g_pGame->GetFlashMenu();
			if (flashMenu && !flashMenu->IsGameActive())
			{
				if (g_pGame->GetGameBrowser())
					UpdateNatType(g_pGame->GetGameBrowser()->GetNatTypeString());

				char strProductVersion[256];
				gEnv->pSystem->GetProductVersion().ToString(strProductVersion);
				m_pFlashUI->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("setGameVersion"), strProductVersion);
			}

#if defined(XENON)
			//Need to register before sign-in to live and allow time for callback
			CPersistantStats *pStats = CPersistantStats::GetInstance();
			if(!pStats->AreLeaderboardsRegistered())
			{
				pStats->RegisterLeaderboards();
			}
#endif
		}
		break;
	case eFFES_leaderboards:
		{
			if (m_pLeaderboards)
			{
				m_pLeaderboards->EnterLeaderboard();
			}
		}
		break;
	case eFFES_lobby:
		{
			CryLog("Entered Lobby Screen");

			if (m_pServerList)
			{
				m_pServerList->ClearServerList(m_pFlashUI);
			}

			g_pGame->GetGameBrowser()->StartSearchingForServers();
		}
		break;
	case eFFES_reputation:
		{
			if (g_pGame->GetPersistantStats())
				g_pGame->GetPersistantStats()->SendStatsToFlash(m_pFlashUI);
			
			if (CPlayerProgression *pPlayerProgression = CPlayerProgression::GetInstance())
			{
				const int argSize = 7;
				SFlashVarValue ppArgs[argSize] = { pPlayerProgression->GetData(EPP_Rank),
					pPlayerProgression->GetRankName(pPlayerProgression->GetData(EPP_Rank)),
					pPlayerProgression->GetData(EPP_NextRank),
					pPlayerProgression->GetRankName(pPlayerProgression->GetData(EPP_NextRank)),
					pPlayerProgression->GetData(EPP_MaxRank),
					pPlayerProgression->GetData(EPP_XP),
					pPlayerProgression->GetData(EPP_XPToNextRank) };
				m_pFlashUI->Invoke(FRONTEND_SUBSCREEN_PATH_SET("UpdateProgressionStats"), ppArgs, argSize);
			}
		}
		break;
	};
}

//------------------------------------------------------------------------
void CMPMenuHub::LeaveScreen(const CFlashFrontEnd::EFlashMenus menu, const EFlashFrontEndScreens screen)
{
	switch(screen)
	{
	case eFFES_lobby:
		{
			CryLog("Leaving Lobby Screen");
			g_pGame->GetGameLobby()->CancelAllLobbyTasks();
		}
		break;
	case eFFES_leaderboards:
		{
			if (m_pLeaderboards)
			{
				m_pLeaderboards->LeaveLeaderboard();
			}
		}
		break;
	};

	m_currentScreen = eFFES_unknown;
}

//------------------------------------------------------------------------
void CMPMenuHub::Update(float frameTime)
{
#ifndef _RELEASE
	if (m_pLeaderboards)
		m_pLeaderboards->Update(frameTime);
#endif
}

//------------------------------------------------------------------------
bool CMPMenuHub::HandleCommand(const char* pCommand, const char* pArgs, void* pUserData)
{
	CMPMenuHub::EMPMenuCommand command = GetCommand(pCommand);
	if (command == eMPFEC_none)
		return false;

	return HandleCommand(command, pArgs, pUserData);
}

//------------------------------------------------------------------------
bool CMPMenuHub::HandleCommand(EMPMenuCommand command, const char* pArgs, void* pUserData)
{
	if (command == eMPFEC_none)
		return false;

	switch (command)
	{
	case eMPFEC_mp_load:
		{
			if(pArgs && pArgs[0])
			{
				CRY_TODO(02, 02, 2010, "Server name needs setting elsewhere. Temporarily done here for dev purposes.");
				if(ICVar* v = gEnv->pConsole->GetCVar("sv_serverName"))
				{
					const char *svServerName = v->GetString();
					if (svServerName && svServerName[0] == '\0') // If none already manually set
					{
						const char* playerName = '\0';
						string serverName("sv_serverName ");
						serverName.append(gEnv->pNetwork->GetHostName());
						serverName.append(" ");

						IPlayerProfileManager *pProfileMan = g_pGame->GetOptions()->GetProfileManager();
						IPlayerProfile *pProfile = pProfileMan ? pProfileMan->GetCurrentProfile(pProfileMan->GetCurrentUser()) : NULL;
						if ( pProfileMan  && pProfile)
							playerName = pProfile->GetName();
						else
							playerName = gEnv->pSystem->GetUserName();

						if ( playerName[0] == '\0' )
						{
							playerName = "default";
						}
						serverName.append( playerName );
						gEnv->pConsole->ExecuteString(serverName.c_str());
					}
				}

				gEnv->pConsole->ExecuteString("sv_maxplayers 16");

				gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame(string().Format("map %s s", pArgs).c_str());
			}
			else
			{
				CryLog("eFECMD_mp_load no map param provided.");
			}
		}
		break;
	case eMPFEC_set_lobby_type:
		{
			CryLog("eFECMD_mp_set_lobby_type %s", pArgs);
			if (pArgs && pArgs[0])
			{
				if (!stricmp(pArgs, "lan"))
					gEnv->pNetwork->GetLobby()->SetLobbyService(eCLS_LAN);
				else if (!stricmp(pArgs, "online"))
					gEnv->pNetwork->GetLobby()->SetLobbyService(eCLS_Online);
				else
					CryLog("eFECMD_mp_set_lobby_type unknown lobby type %s", pArgs);
			}
		}
		break;
	// Server List -------------------------------
	case eMPFEC_serverlist_scrollDown:
		{
			if (m_pServerList)
			{
				int startIndex = m_pServerList->GetStartIndex();
				if (startIndex < MAX(m_pServerList->GetTotalServers() - m_pServerList->GetNumEntries(),0))
				{
					startIndex++;
					CryLog("startIndex %d",startIndex);
					m_pServerList->SetStartIndex(startIndex);
					m_pServerList->RefreshServerList(m_pFlashUI);
				}
			}
		}
		break;
	case eMPFEC_serverlist_scrollUp:
		{
			if (m_pServerList)
			{
				int startIndex = m_pServerList->GetStartIndex();
				if (startIndex > 0)
				{
					startIndex--;
					CryLog("startIndex %d",startIndex);
					m_pServerList->SetStartIndex(startIndex);
					m_pServerList->RefreshServerList(m_pFlashUI);
				}
			}
		}
		break;
	case eMPFEC_serverlist_join:
		{
			if (pArgs && pArgs[0])
			{
				JoinServer(atoi(pArgs));
			}
		}
		break;
	case eMPFEC_set_serverlist_size:
		{
			if (m_pServerList)
			{
				int numEntries = atoi(pArgs);
				CRY_ASSERT(numEntries > 0);
				m_pServerList->SetNumEntries(numEntries);
			}
		}
		break;
	// Leaderboards -------------------------------
	case eMPFEC_leaderboard_move:
		{
			if (m_pLeaderboards)
			{
				int direction = atoi(pArgs);
				if (direction>0)
					m_pLeaderboards->MoveLeaderboard(true);
				else
					m_pLeaderboards->MoveLeaderboard(false);
			}
		}
		break;
	case eMPFEC_leaderboard_change:
		{
			if (m_pLeaderboards)
			{
				int direction = atoi(pArgs);
				if (direction>0)
					m_pLeaderboards->GetNextLeaderboardType(false);
				else
					m_pLeaderboards->GetNextLeaderboardType(true);
			}
		}
		break;
	case eMPFEC_leaderboard_set_size:
		{
			if (m_pLeaderboards)
			{
				int entires = atoi(pArgs);
				m_pLeaderboards->SetEntriesSize(entires);
			}
		}
		break;
	case eMPFEC_leaderboard_move_to_user:
		{
			if (m_pLeaderboards)
			{
				m_pLeaderboards->GetLocalUserLeaderboard();
			}
		}
		break;
	case eMPFEC_leaderboard_move_to_top:
		{
			if (m_pLeaderboards)
			{
				m_pLeaderboards->MoveTopLeaderboard();
			}
		}
		break;
	};

	return true;
}

//------------------------------------------------------------------------
bool CMPMenuHub::OnWarningReturn(THUDWarningId id, const char* returnValue)
{
	if(id == g_pGame->GetWarnings()->GetWarningId("JoinSession"))
	{
		// Only 1 return value - cancel
		//if(!stricmp(returnValue, "cancel"))
		{
			gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");

			if (g_pGame->GetGameLobby())
			{
				g_pGame->GetGameLobby()->CancelLobbyTask(eLTT_Creating);
				g_pGame->GetGameLobby()->CancelLobbyTask(eLTT_Joining);
			}
		}
	}
	else if (id == g_pGame->GetWarnings()->GetWarningId("CreateSession"))
	{
		// Only 1 return value - cancel
		gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");

		if (g_pGame->GetGameLobby())
		{
			g_pGame->GetGameLobby()->CancelLobbyTask(eLTT_Creating);
			g_pGame->GetGameLobby()->CancelLobbyTask(eLTT_Joining);
		}
	}

	return true;
}

//------------------------------------------------------------------------
void CMPMenuHub::OnWarningRemoved(THUDWarningId id)
{
	if((id == g_pGame->GetWarnings()->GetWarningId("JoinSession")) || (id == g_pGame->GetWarnings()->GetWarningId("CreateSession")))
	{
		m_bLoadingDialog = false;
	}
}

//------------------------------------------------------------------------
void CMPMenuHub::UpdateNatType(const char* natType)
{
	if ((m_currentScreen == eFFES_play_online) || (m_currentScreen == eFFES_play_lan))
	{
		CFlashFrontEnd *flashMenu = g_pGame->GetFlashMenu();
		if (flashMenu && !flashMenu->IsGameActive())
		{
			m_pFlashUI->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("setNatType"),natType);
		}
	}
}

//------------------------------------------------------------------------
// Trigger a join search.
void CMPMenuHub::JoinServer(int serverIdx)
{
	if (m_pServerList)
	{
		const CUIServerList::SServerInfo *serverInfo = m_pServerList->GetServerInfo(serverIdx);
		
		CRY_ASSERT(serverInfo);

		if (serverInfo && g_pGame->GetGameLobby())
		{
			g_pGame->GetGameLobby()->JoinServer(serverInfo->m_sessionId, serverInfo->m_hostName.c_str());
		}
	}
}

//------------------------------------------------------------------------
void CMPMenuHub::AddServer(const CUIServerList::SServerInfo &serverInfo)
{
	if (m_pServerList)
	{
		m_pServerList->AddServer(serverInfo, m_pFlashUI);	
	}
}

//------------------------------------------------------------------------
void CMPMenuHub::StartSearching()
{
	if (m_pFlashUI)
	{
		m_pFlashUI->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("StartSearching"));
	}
}

//------------------------------------------------------------------------
void CMPMenuHub::SearchComplete()
{
	if (m_pFlashUI)
	{
		m_pFlashUI->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("SearchComplete"));
	}
	CCCPOINT(GameLobby_UpdateComplete);
}

//------------------------------------------------------------------------
void CMPMenuHub::StopServerListUpdate()
{
}

//------------------------------------------------------------------------
void CMPMenuHub::ShowLoadingDialog(const char *dialogName, const char *param/* = NULL*/)
{
	g_pGame->GetWarnings()->AddWarning(dialogName, param, this);
	m_bLoadingDialog = true;
}

//------------------------------------------------------------------------
void CMPMenuHub::HideLoadingDialog(const char *dialogName)
{
	g_pGame->GetWarnings()->RemoveWarning(dialogName);
	m_bLoadingDialog = false;
}

//------------------------------------------------------------------------
void CMPMenuHub::ShowErrorDialog(const char *dialogName, const char *param/* = NULL*/)
{
	if (m_bLoadingDialog)
	{
		g_pGame->GetWarnings()->RemoveWarning("JoinSession");
		g_pGame->GetWarnings()->RemoveWarning("CreateSession");
		m_bLoadingDialog = false;
	}
	
	g_pGame->GetWarnings()->AddWarning(dialogName, param, this);
}

//------------------------------------------------------------------------
void CMPMenuHub::Disconnected(int value)
{
	if (gEnv->bHostMigrating && (int(eDC_NubDestroyed) == value)) // ignore during migration
	{
		return;
	}
	
	if((int(eDC_UserRequested) != value) && (gEnv->bServer==false || (int(eDC_NubDestroyed) != value)))
	{
		CryFixedStringT<8> param;
		param.Format("%d", value);
		ShowErrorDialog("Disconnect", param.c_str());	
	}
}


//------------------------------------------------------------------------
void CMPMenuHub::ConnectionFailed()
{
	ShowErrorDialog("ConnectionFailed");
}