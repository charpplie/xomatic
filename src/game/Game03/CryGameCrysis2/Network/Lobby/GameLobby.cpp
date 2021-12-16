#include "StdAfx.h"

#include "GameLobby.h"
#include "GameBrowser.h"

#include "ICryLobby.h"
#include "ICryMatchMaking.h"
#include "IGameFramework.h"

#include "Game.h"
#include "HUD/HUD.h"
#include "Menus/MPLobbyUI.h"
#include "Menus/MultiplayerMenu.h"
#include "Menus/FlashMenuObject.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/Multiplayer/MPMenuHub.h"
#include "FrontEnd/Multiplayer/UIServerList.h"

#include "Endian.h"

#include "GameCVars.h"
#include "Utility/StringUtils.h"
#include "Utility/CryWatch.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//------------
// Local Structs
//------------

union char16toint644convert
{
	char str32s[32];
	uint64 int64s[4];
};

//------------
// LOBBY DATA
//------------

#if defined(XENON)
#include "Network/Lobby/Xenon_LobbyDefinitions.h"

enum ELOBBYIDS{
	LID_USERDATA_GAMEMODE = PROPERTY_SESSION_DATA_I32_0,
	LID_UNUSED = PROPERTY_SESSION_DATA_I32_1, // TODO REMOVE
	LID_USERDATA_MAP_NAME_1 = PROPERTY_SESSION_DATA_I64_0,
	LID_USERDATA_MAP_NAME_2 = PROPERTY_SESSION_DATA_I64_1,
	LID_USERDATA_MAP_NAME_3 = PROPERTY_SESSION_DATA_I64_2,
	LID_USERDATA_MAP_NAME_4 = PROPERTY_SESSION_DATA_I64_3,
	LID_USERDATA_GAMERULES_1= PROPERTY_SESSION_DATA_I64_4,
	LID_USERDATA_GAMERULES_2= PROPERTY_SESSION_DATA_I64_5,
	LID_USERDATA_GAMERULES_3= PROPERTY_SESSION_DATA_I64_6,
	LID_USERDATA_GAMERULES_4= PROPERTY_SESSION_DATA_I64_7,
};

#define REQUIRED_SESSIONS_SEARCH_PARAM	X_CONTEXT_GAME_MODE

#else

// PS3 Note : There are only 8 uint32 attribute fields on PSN, everything else must be packed into 1 of 2 binary attribute fields which
//						are 256 bytes each. If filtering is to be required make sure items you want to filter by are in the 8 uint32 fields.
//						To put it another way, the first 8 eCSUDT_Int32 values registered will be allocated to the 8 searchable uint32 fields,
//						all other data will be placed into binary data and not be able to be filtered!


// Ids
enum ELOBBYIDS{
	LID_USERDATA_GAMEMODE = 0,
	LID_UNUSED, // TODO REMOVE
	LID_USERDATA_MAP_NAME_1,
	LID_USERDATA_MAP_NAME_2,
	LID_USERDATA_MAP_NAME_3,
	LID_USERDATA_MAP_NAME_4,
	LID_USERDATA_GAMERULES_1,
	LID_USERDATA_GAMERULES_2,
	LID_USERDATA_GAMERULES_3,
	LID_USERDATA_GAMERULES_4,
};

#define SESSION_MATCH_QUERY_TEST	0

#define REQUIRED_SESSIONS_SEARCH_PARAM	0

#endif

// default & Search Data. // ?
//#define TEST_USERDATA_MAP           1
#define TEST_SEARCH1                0
#define USERDATA_GAMEMODE_CTF       0
#define UNUSED_DATA                 0 // TODO REMOVE
//#endif


//------------
// CGameLobby
//------------

//-------------------------------------------------------------------------
CGameLobby::CGameLobby( void ) : REGISTER_GAME_MECHANISM(CGameLobby)
{
	m_creatingGame = false;
	m_gameStartParams = NULL;
	m_currentSession = CrySessionInvalidHandle;

	for (int i=0; i<eLTT_Num; i++)
	{
		m_lobbyTasks[i] = CryLobbyInvalidTaskID;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::Update(float dt)
{
	if (m_creatingGame)
	{
		if (m_gameStartParams)
		{
			CCCPOINT(GameLobby_StartGame);

			SGameContextParams gameContextParams;
			gameContextParams.levelName = m_gameStartParams->levelName.c_str();
			gameContextParams.gameRules = m_gameStartParams->gameModeName.c_str();
			gameContextParams.demoPlaybackFilename = m_gameStartParams->demoPlaybackFilename.c_str();
			gameContextParams.demoRecorderFilename = m_gameStartParams->demoRecorderFilename.c_str();

			SGameStartParams gameStartParams;
			gameStartParams.flags = m_gameStartParams->flags;
			gameStartParams.port = m_gameStartParams->port;
			gameStartParams.maxPlayers = m_gameStartParams->maxPlayers;
			gameStartParams.hostname = m_gameStartParams->hostname.c_str();
			gameStartParams.connectionString = m_gameStartParams->connectionString.c_str();
			gameStartParams.pContextParams = &gameContextParams;
			gameStartParams.session = m_currentSession;

			g_pGame->GetIGameFramework()->StartGameContext(&gameStartParams);

			SAFE_DELETE(m_gameStartParams);
		}

		m_creatingGame = false;
	}
}

//-------------------------------------------------------------------------
void CGameLobby::CreateSession( const SGameStartParams* pGameStartParams )
{
	CCCPOINT(GameLobby_CreateSession);

	SAFE_DELETE(m_gameStartParams);
	m_gameStartParams = new SLobbyGameStartParams();

	// Get the map name in a CryLobby format.
	// Can't use strings so pack the first 32 chars into a
	// a array of int64s.
	m_gameStartParams->levelName = pGameStartParams->pContextParams->levelName;
	m_gameStartParams->gameModeName = pGameStartParams->pContextParams->gameRules;

	char16toint644convert sz_mapname;
	char16toint644convert sz_gamemode;
	COMPILE_TIME_ASSERT( ( sizeof(sz_mapname.str32s) == sizeof(sz_mapname.int64s) ) );

	memcpy( (void*)sz_mapname.str32s, (void*)m_gameStartParams->levelName.c_str(), 32);
	memcpy( (void*)sz_gamemode.str32s, (void*)m_gameStartParams->gameModeName.c_str(), 32);

	if( eLittleEndian )
	{
		SwapEndianBase( sz_mapname.int64s, 4, true );
		SwapEndianBase( sz_gamemode.int64s, 4, true );
	}//eLittleEndian

	SCrySessionUserData userData[10];
	SCrySessionData session;
	uint32 users = 0;

	userData[0].m_id = LID_USERDATA_GAMEMODE;
	userData[0].m_type = eCSUDT_Int32;
	userData[0].m_int32 = USERDATA_GAMEMODE_CTF; // TODO : Set this to a game mode id...

	userData[1].m_id = LID_UNUSED;
	userData[1].m_type = eCSUDT_Int32;
	userData[1].m_int32 = UNUSED_DATA;

	userData[2].m_id = LID_USERDATA_MAP_NAME_1;
	userData[2].m_type = eCSUDT_Int64;
	userData[2].m_int64 = sz_mapname.int64s[0];

	userData[3].m_id = LID_USERDATA_MAP_NAME_2;
	userData[3].m_type = eCSUDT_Int64;
	userData[3].m_int64 = sz_mapname.int64s[1];

	userData[4].m_id = LID_USERDATA_MAP_NAME_3;
	userData[4].m_type = eCSUDT_Int64;
	userData[4].m_int64 = sz_mapname.int64s[2];

	userData[5].m_id = LID_USERDATA_MAP_NAME_4;
	userData[5].m_type = eCSUDT_Int64;
	userData[5].m_int64 = sz_mapname.int64s[3];

	userData[6].m_id = LID_USERDATA_GAMERULES_1;
	userData[6].m_type = eCSUDT_Int64;
	userData[6].m_int64 = sz_gamemode.int64s[0];

	userData[7].m_id = LID_USERDATA_GAMERULES_2;
	userData[7].m_type = eCSUDT_Int64;
	userData[7].m_int64 = sz_gamemode.int64s[1];

	userData[8].m_id = LID_USERDATA_GAMERULES_3;
	userData[8].m_type = eCSUDT_Int64;
	userData[8].m_int64 = sz_gamemode.int64s[2];

	userData[9].m_id = LID_USERDATA_GAMERULES_4;
	userData[9].m_type = eCSUDT_Int64;
	userData[9].m_int64 = sz_gamemode.int64s[3];

	session.m_data = userData;
	session.m_numData = 10;
	session.m_numPublicSlots = 16;
	session.m_numPrivateSlots = 0;
	memset( session.m_name, 0, MAX_SESSION_NAME_LENGTH );
	memcpy( (void*)session.m_name, (void*)gEnv->pConsole->GetCVar("sv_servername")->GetString(), MAX_SESSION_NAME_LENGTH-1 );	
	session.m_ranked = false;

	// Store the game start params, needs to be done else strings are lost from stack before callback comes back.
	m_gameStartParams->flags = pGameStartParams->flags;
	m_gameStartParams->port = pGameStartParams->port;
	m_gameStartParams->maxPlayers = pGameStartParams->maxPlayers;
	m_gameStartParams->hostname = pGameStartParams->hostname;
	m_gameStartParams->connectionString = pGameStartParams->connectionString;
	if (pGameStartParams->pContextParams)
	{
		m_gameStartParams->levelName = pGameStartParams->pContextParams->levelName;
		m_gameStartParams->gameModeName = pGameStartParams->pContextParams->gameRules;
		m_gameStartParams->demoPlaybackFilename = pGameStartParams->pContextParams->demoPlaybackFilename;
		m_gameStartParams->demoRecorderFilename = pGameStartParams->pContextParams->demoRecorderFilename;
	}

	if (gEnv->pNetwork && gEnv->pNetwork->GetLobby() && gEnv->pNetwork->GetLobby()->GetLobbyService())
	{
		ECryLobbyError error = eCLE_Success;
		ICryMatchMaking *pMatchmaking = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking();

		if (pMatchmaking)
		{
			if (m_gameStartParams->flags & eGSF_HostMigrated)
			{
				error = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking()->SessionMigrate(m_currentSession,&users, 1, CRYSESSION_CREATE_FLAG_SEARCHABLE, &session, 0, CGameLobby::MatchmakingSessionMigrateCallback, this);
			}
			else
			{
				error = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking()->SessionCreate(&users, 1, CRYSESSION_CREATE_FLAG_SEARCHABLE, &session, 0, CGameLobby::MatchmakingSessionCreateCallback, this);
				CRY_ASSERT_MESSAGE(error==eCLE_Success,"GameLobby creating session failed.");

				if (error==eCLE_Success)
				{
					CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
					CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
					if (mpMenuHub)
						mpMenuHub->ShowLoadingDialog("CreateSession");
				}
			}
		}
		else
		{
			error = eCLE_ServiceNotSupported;
		}

		if (error == eCLE_Success)
		{
			//m_processing = true;
		}
	}
	else
	{
			CRY_ASSERT_MESSAGE(0,"GameLobby Unable to create session : no lobby service available.");
	}
}

//-------------------------------------------------------------------------
void CGameLobby::CancelLobbyTask(ELobbyTaskType taskId)
{
	CryLogAlways("CGameLobby::CancelLobbyTask %d", (int)taskId);

	ICryLobby *lobby = gEnv->pNetwork->GetLobby();

	if (lobby && lobby->GetLobbyService())
	{
		if (m_lobbyTasks[taskId] != CryLobbyInvalidTaskID)
		{
			lobby->GetLobbyService()->GetMatchMaking()->CancelTask(m_lobbyTasks[taskId]);
			m_lobbyTasks[taskId] = CryLobbyInvalidTaskID;
		}
	}
}

//-------------------------------------------------------------------------
void CGameLobby::CancelAllLobbyTasks()
{
	CryLogAlways("CGameLobby::CancelAllLobbyTasks");

	if (ICryLobby *lobby = gEnv->pNetwork->GetLobby())
	{
		if (ICryLobbyService *lobbyService = lobby->GetLobbyService())
		{
			for (int i=0; i<eLTT_Num; i++)
			{
				if (m_lobbyTasks[i] != CryLobbyInvalidTaskID)
				{
					lobby->GetLobbyService()->GetMatchMaking()->CancelTask(m_lobbyTasks[i]);
					m_lobbyTasks[i] = CryLobbyInvalidTaskID;

					CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
					CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
					if (mpMenuHub)
						mpMenuHub->SearchComplete();
				}
			}
		}
	}

	CGameBrowser *gameBrowser = g_pGame->GetGameBrowser();
	gameBrowser->CancelSearching();
}

//-------------------------------------------------------------------------
// Connect to a server
void CGameLobby::JoinServer(CrySessionID sessionId, const char *sessionName)
{
	CryLogAlways("CGameLobby::JoinServer %d", (int)sessionId);

	// Request a server join on the lobby system.
	ICryLobby *lobby = gEnv->pNetwork->GetLobby();
	if( lobby )
	{
		CCCPOINT (GameLobby_JoinServer);

		uint32 users[2] = {0, 1};

		if (lobby->GetLobbyService())
		{
			CGameBrowser *gameBrowser = g_pGame->GetGameBrowser();
			gameBrowser->CancelSearching();

			CRY_ASSERT_MESSAGE(m_lobbyTasks[eLTT_Joining]==CryLobbyInvalidTaskID,"GameLobby Trying to join a session when you think you are already joining one.");

			ECryLobbyError error = lobby->GetLobbyService()->GetMatchMaking()->SessionJoin(users, 1, sessionId, &m_lobbyTasks[eLTT_Joining], CGameLobby::MatchmakingSessionJoinCallback, this);
			CRY_ASSERT_MESSAGE(error==eCLE_Success,"GameLobby joining session failed.");

			if (error == eCLE_Success)
			{
				bool useBlank = sessionName && sessionName[0]=='\0';
				CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
				CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
				if (mpMenuHub)
					mpMenuHub->ShowLoadingDialog("JoinSession", useBlank ? " " : sessionName);
			}
		}
		else
		{
			CRY_ASSERT_MESSAGE(0,"GameLobby Unable to join server : no lobby service available.");
		}
	}
}

//-------------------------------------------------------------------------
// Delete the current session.
void CGameLobby::DeleteSession()
{
	// Request a server delete in the lobby system.
	ICryLobby *lobby = gEnv->pNetwork->GetLobby();
	if (lobby && lobby->GetLobbyService())
	{
		if (m_currentSession != CrySessionInvalidHandle)
		{
			CRY_ASSERT_MESSAGE(m_lobbyTasks[eLTT_Deleting]==CryLobbyInvalidTaskID,"GameLobby Trying to delete a session when you think you are already deleting one.");

			ECryLobbyError error = lobby->GetLobbyService()->GetMatchMaking()->SessionDelete(m_currentSession, &m_lobbyTasks[eLTT_Deleting], CGameLobby::MatchmakingSessionDeleteCallback, this);
			CRY_ASSERT_MESSAGE(error==eCLE_Success,"GameLobby failed to delete a session.");
			
			m_currentSession = CrySessionInvalidHandle;
		}
	}
}

//-------------------------------------------------------------------------
// Migrate callback.
void CGameLobby::MatchmakingSessionMigrateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg)
{
	if ( error == eCLE_Success )
	{
		CGameLobby *lobby = static_cast<CGameLobby *>(arg);
		if (lobby)
		{
			lobby->SetCurrentSession(h);
		}
	}
	else
	{
		CryLogAlways("GameLobby session migrate error %d", (int)error);
		CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
		CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
		CHUD* pHUD = g_pGame->GetHUD();
		if (mpMenuHub && pHUD)
		{
			CryFixedStringT<32> errorMsg;
			errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
			errorMsg = pHUD->LocalizeString(errorMsg.c_str());
			mpMenuHub->ShowErrorDialog("LobbyError", errorMsg.c_str());
		}
	}
}



//-------------------------------------------------------------------------
// Create callback.
// Starts the server game.
void CGameLobby::MatchmakingSessionCreateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg)
{
	CGameLobby *lobby = static_cast<CGameLobby *>(arg);
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;

	if ( error == eCLE_Success )
	{
		if (lobby)
		{
			lobby->SetCurrentSession(h);
			if (!(lobby->GetLobbyGameStartParams()->flags & eGSF_HostMigrated))
			{
				// Only create the game if we're not migrating the server (game already exists)
				lobby->CreateGameNextFrame(); // Create next frame
			}

			CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
			CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
			if (mpMenuHub)
				mpMenuHub->HideLoadingDialog("CreateSession");
		}
	}
	else
	{
		CryLogAlways("GameLobby session create error %d", (int)error);
		CHUD* pHUD = g_pGame->GetHUD();
		if (mpMenuHub && pHUD)
		{
			CryFixedStringT<32> errorMsg;
			errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
			errorMsg = pHUD->LocalizeString(errorMsg.c_str());
			mpMenuHub->ShowErrorDialog("LobbyError", errorMsg.c_str());
		}
	}

	if (lobby)
	{
		lobby->ClearLobbyTaskID(eLTT_Creating);
	}
}

//-------------------------------------------------------------------------
// Do the actual game-connect on successful find.
// arg is CGameLobby*
void CGameLobby::MatchmakingSessionJoinCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 ip, uint16 port, void* arg)
{
	CGameLobby *lobby = static_cast<CGameLobby *>(arg);

	if (error == eCLE_Success)
	{
		if (lobby)
		{
			lobby->SetCurrentSession(h);
		}

		char command[128];
		sprintf(command, "connect <session>%d,%d.%d.%d.%d:%d", h, ((uint8*)&ip)[0], ((uint8*)&ip)[1], ((uint8*)&ip)[2], ((uint8*)&ip)[3], port);
		
		g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(command);

		CryLogAlways("CGameLobby::MatchmakingSessionJoinCallback");
	}
	else // an error
	{
		CryLogAlways("GameLobby session join error %d", (int)error);
		CHUD* pHUD = g_pGame->GetHUD();
		CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
		CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
		if (mpMenuHub && pHUD)
		{
			CryFixedStringT<32> errorMsg;
			errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
			errorMsg = pHUD->LocalizeString(errorMsg.c_str());
			mpMenuHub->ShowErrorDialog("LobbyError", errorMsg.c_str());
		}
	}

	if (lobby)
	{
		lobby->ClearLobbyTaskID(eLTT_Joining);
	}
}

//-------------------------------------------------------------------------
// Callback from deleting session
// arg is CGameLobby*
void CGameLobby::MatchmakingSessionDeleteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg)
{
	assert(error == eCLE_Success);

	CGameLobby *lobby = static_cast<CGameLobby *>(arg);
	if (lobby)
	{
		lobby->SetCurrentSession(CrySessionInvalidHandle);
		lobby->ClearLobbyTaskID(eLTT_Deleting);
	}
}
