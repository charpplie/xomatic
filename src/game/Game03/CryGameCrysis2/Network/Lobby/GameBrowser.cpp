#include "StdAfx.h"

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


//-------------------------------------------------------------------------
CGameBrowser::CGameBrowser( void ) : REGISTER_GAME_MECHANISM(CGameBrowser)
{
	m_NatType = eNT_Unknown;

	m_searchingTask = CryLobbyInvalidTaskID;

	Init(); // init and trigger a request to register data - needs to be done early.
}

//-------------------------------------------------------------------------
void CGameBrowser::Init( void )
{
	gEnv->pNetwork->GetLobby()->RegisterEventInterest(eCLSE_NatType, CGameBrowser::GetNatTypeCallback, this);

	gEnv->pNetwork->GetLobby()->Initialise(eCLS_LAN, CGameBrowser::ConfigurationCallback, CGameBrowser::InitialiseCallback, this);
	gEnv->pNetwork->GetLobby()->Initialise(eCLS_Online, CGameBrowser::ConfigurationCallback, CGameBrowser::InitialiseCallback, this);

#if defined(XENON) || defined(PS3)
	gEnv->pNetwork->GetLobby()->SetLobbyService(eCLS_Online);	//default to online for stats support
#else
	gEnv->pNetwork->GetLobby()->SetLobbyService(eCLS_LAN);
#endif
}

//-------------------------------------------------------------------------
void CGameBrowser::Update(float dt)
{
	//remove me
}

//-------------------------------------------------------------------------
void CGameBrowser::StartSearchingForServers()
{
	ICryLobby *lobby = gEnv->pNetwork->GetLobby();

	if (lobby)
	{
		CCCPOINT (GameLobby_StartSearchingForServers);

		SCrySessionSearchParam param;
		SCrySessionSearchData data[2];

		param.m_type = SESSION_MATCH_QUERY_TEST;
		param.m_data = data;
		param.m_numData = 1;
		param.m_numFreeSlots = 1;
		param.m_maxNumReturn = 32;
		param.m_ranked = false;

		data[0].m_operator = eCSSO_Equal;
		data[0].m_data.m_id = REQUIRED_SESSIONS_SEARCH_PARAM;
		data[0].m_data.m_type = eCSUDT_Int32;
		data[0].m_data.m_int32 = USERDATA_GAMEMODE_CTF; // TODO : Is this filtering results?
		/*
		data[1].m_operator = eCSSO_Equal;
		data[1].m_data.m_id = LID_UNUSED;
		data[1].m_data.m_type = eCSUDT_Int32;
		data[1].m_data.m_int32 = UNUSED_DATA;
		*/
		if (lobby->GetLobbyService())
		{
			CRY_ASSERT_MESSAGE(m_searchingTask==CryLobbyInvalidTaskID,"CGameBrowser Trying to search for sessions when you think you are already searching.");

			ECryLobbyError error = lobby->GetLobbyService()->GetMatchMaking()->SessionSearch(0, &param, &m_searchingTask, CGameBrowser::MatchmakingSessionSearchCallback, this);
			CRY_ASSERT_MESSAGE(error==eCLE_Success,"CGameBrowser searching for sessions failed.");

			if (error == eCLE_Success)
			{
				CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
				CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
				if (mpMenuHub)
					mpMenuHub->StartSearching();
			}
			CryLogAlways("CCGameBrowser::StartSearchingForServers %d", m_searchingTask);
		}
		else
		{
			CRY_ASSERT_MESSAGE(0,"CGameBrowser Cannot search for servers : no lobby service available.");
		}
	}
}

//-------------------------------------------------------------------------
void CGameBrowser::CancelSearching()
{
	CryLogAlways("CGameBrowser::CancelSearching");

	m_searchingTask = CryLobbyInvalidTaskID;

	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;
	if (mpMenuHub)
		mpMenuHub->SearchComplete();
}

//------------
// CALLBACKS
//------------

// This is a new callback since adding PSN support, Gamespy will also likely fire this callback.
//Basically whenever this callback is fired, you are required to fill in the requestedParams that are asked for.
//At present specifications are that the callback can fire multiple times.
//
// 'PCom'		void*		ptr to static sceNpCommunitcationId					(not copied - DO NOT PLACE ON STACK!)
// 'PPas'		void*		ptr to static sceNpCommunicationPassphrase	(not copied - DO NOT PLACE ON STACK!)
// 'XSvc'		uint32	Service Id for XLSP servers
// 'XPor'		uint16	Port for XLSP server to communicate with Telemetry
//

#if defined(XENON)

#define XLSP_SERVICE_ID (0x454108E3)							// This is currently our TITLEID (but it will change when we fix the servers properly)
#define XLSP_SERVICE_PORT	(1000)									// 1000 is the correct ID for nottelemetry01

#endif//XENON

#if defined(PS3)

#include <np.h>

// For security reasons these values must NOT be placed outside of the executable (e.g. in xml data for instance!)

static const SceNpCommunicationId s_communication_id = {
	{'N','P','W','R','0','0','9','6','5'},
	'\0',
	0,
	0
};

static const SceNpCommunicationPassphrase s_communication_passphrase = {
	{
		0x19,0xcd,0xd2,0x25,0xd5,0x12,0x00,0xaf,
			0x0e,0x47,0xaa,0x58,0xca,0x9a,0x0c,0x1c,
			0x17,0x54,0x26,0x7c,0xa7,0xd3,0x2f,0x26,
			0x03,0x01,0x01,0x03,0xe4,0xf0,0x6a,0x52,
			0x22,0x66,0x4c,0xc6,0xd5,0x20,0xe1,0xf2,
			0xa6,0x0f,0x3b,0x46,0x3f,0x53,0x10,0x95,
			0x56,0x5d,0x1d,0xbb,0x4a,0x35,0xab,0x1e,
			0xc4,0xe6,0x8b,0x4a,0x21,0xb8,0x56,0xd5,
			0xc6,0x28,0x68,0x6d,0x96,0x12,0x36,0x3a,
			0x0b,0xe7,0xc9,0x19,0x73,0x6b,0x2e,0x65,
			0x57,0xae,0x72,0x1c,0x70,0xcc,0xee,0xa4,
			0x90,0x95,0xa7,0x1a,0xb2,0x2c,0x25,0x8b,
			0xd8,0x99,0x72,0xc1,0xef,0x7a,0x4a,0x16,
			0x35,0x13,0xee,0xc6,0x0f,0xec,0x80,0xf3,
			0x29,0x66,0xa1,0x55,0x81,0x29,0xbd,0x6e,
			0x51,0x5e,0xa9,0x10,0x10,0x4a,0x41,0xb5
	}
};

#endif//PS3

void CGameBrowser::ConfigurationCallback(ECryLobbyService service, SConfigurationParams *requestedParams, uint32 paramCount)
{
	uint32 a;
	for (a=0;a<paramCount;a++)
	{
		switch (requestedParams[a].m_fourCCID)
		{
		case 'xxxx':				// default (currently not used id) - avoids compiler issue 'no case labels'
			break;
#if defined(XENON)
		case 'XSvc':
			requestedParams[a].m_32 = XLSP_SERVICE_ID;
			break;
		case 'XPor':
			requestedParams[a].m_16 = XLSP_SERVICE_PORT;
			break;
#endif//XENON
#if defined(PS3)
		case 'PCom':
			requestedParams[a].m_pData = (void*)&s_communication_id;
			break;
		case 'PPas':
			requestedParams[a].m_pData = (void*)&s_communication_passphrase;
			break;
#endif//PS3
		default:
			CRY_ASSERT_MESSAGE(0,"Unknown Configuration Parameter Requested!");
			break;
		}
	}
}

//-------------------------------------------------------------------------
// Returns the NAT type when set-up.
void CGameBrowser::GetNatTypeCallback(UCryLobbyEventData eventData, void *arg)
{
	SCryLobbyNatTypeData *natTypeData = eventData.pNatTypeData;
	if (natTypeData)
	{
		CGameBrowser* pGameBrowser = (CGameBrowser*) arg;
		pGameBrowser->SetNatType(natTypeData->m_curState);

		const char* natString = pGameBrowser->GetNatTypeString();

#if !defined(_RELEASE)
		g_pGameCVars->net_nat_type->ForceSet(natString);
#endif

		if (CFlashFrontEnd *flashMenu = g_pGame->GetFlashMenu())
		{
			if (flashMenu->GetMPMenu())
				flashMenu->GetMPMenu()->UpdateNatType(natString);
		}
	}
}

//-------------------------------------------------------------------------
const char * CGameBrowser::GetNatTypeString() const
{
	switch(m_NatType)
	{
	case eNT_Open:
		return "NAT Type: Open";
	case eNT_Moderate:
		return "NAT Type: Moderate";
	case eNT_Strict:
		return "NAT Type: Strict";
	default:
		return "NAT Type: Unknown";
	};
	return "";
}

//-------------------------------------------------------------------------
// Register the data in CryLobby.
void CGameBrowser::InitialiseCallback(ECryLobbyService service, ECryLobbyError error, void* arg)
{
	assert( error == eCLE_Success );

	if (error == eCLE_Success)
	{
		SCrySessionUserData userData[10];

		userData[0].m_id = LID_USERDATA_GAMEMODE;
		userData[0].m_type = eCSUDT_Int32;
		userData[0].m_int32 = USERDATA_GAMEMODE_CTF;

		// TODO : Remove
		userData[1].m_id = LID_UNUSED;
		userData[1].m_type = eCSUDT_Int32;
		userData[1].m_int32 = UNUSED_DATA;

		userData[2].m_id = LID_USERDATA_MAP_NAME_1;
		userData[2].m_type = eCSUDT_Int64;
		userData[2].m_int64 = 0;

		userData[3].m_id = LID_USERDATA_MAP_NAME_2;
		userData[3].m_type = eCSUDT_Int64;
		userData[3].m_int64 = 0;

		userData[4].m_id = LID_USERDATA_MAP_NAME_3;
		userData[4].m_type = eCSUDT_Int64;
		userData[4].m_int64 = 0;

		userData[5].m_id = LID_USERDATA_MAP_NAME_4;
		userData[5].m_type = eCSUDT_Int64;
		userData[5].m_int64 = 0;

		userData[6].m_id = LID_USERDATA_GAMERULES_1;
		userData[6].m_type = eCSUDT_Int64;
		userData[6].m_int64 = 0;

		userData[7].m_id = LID_USERDATA_GAMERULES_2;
		userData[7].m_type = eCSUDT_Int64;
		userData[7].m_int64 = 0;

		userData[8].m_id = LID_USERDATA_GAMERULES_3;
		userData[8].m_type = eCSUDT_Int64;
		userData[8].m_int64 = 0;

		userData[9].m_id = LID_USERDATA_GAMERULES_4;
		userData[9].m_type = eCSUDT_Int64;
		userData[9].m_int64 = 0;

		gEnv->pNetwork->GetLobby()->SetLobbyService(service);

		ECryLobbyError error = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetMatchMaking()->SessionRegisterUserData(userData, 10, 0, NULL, NULL);
	}
}

//-------------------------------------------------------------------------
// Process a search result.
void CGameBrowser::MatchmakingSessionSearchCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg)
{
	CFlashFrontEnd *menu = g_pGame->GetFlashMenu();
	CMPMenuHub *mpMenuHub = menu ? menu->GetMPMenu() : NULL;

	if( session && mpMenuHub )
	{
		// get the map name from the session data.
		char16toint644convert in_mapname;
		char16toint644convert in_gamemode;
		COMPILE_TIME_ASSERT( ( sizeof(in_mapname.str32s) == sizeof(in_mapname.int64s) ) );
		char sz_mapname[33];
		char sz_gamemode[33];
		memset( sz_mapname, 0, 33 ); // guaranteed null term
		memset( sz_gamemode, 0, 33 ); // guaranteed null term

		SCrySessionData& sessionData = session->m_data;
		SCrySessionUserData* userData = sessionData.m_data;

		int a=0;
		for (a=0;a<sessionData.m_numData;a++)		// This is the correct way to pull session data out (using userData[LID_USERDATA_MAP_NAME_1] won't work!!!)
		{
			switch (userData[a].m_id)
			{
			case LID_USERDATA_MAP_NAME_1:
				in_mapname.int64s[0] = userData[a].m_int64;
				break;
			case LID_USERDATA_MAP_NAME_2:
				in_mapname.int64s[1] = userData[a].m_int64;
				break;
			case LID_USERDATA_MAP_NAME_3:
				in_mapname.int64s[2] = userData[a].m_int64;
				break;
			case LID_USERDATA_MAP_NAME_4:
				in_mapname.int64s[3] = userData[a].m_int64;
				break;
			case LID_USERDATA_GAMERULES_1:
				in_gamemode.int64s[0] = userData[a].m_int64;
				break;
			case LID_USERDATA_GAMERULES_2:
				in_gamemode.int64s[1] = userData[a].m_int64;
				break;
			case LID_USERDATA_GAMERULES_3:
				in_gamemode.int64s[2] = userData[a].m_int64;
				break;
			case LID_USERDATA_GAMERULES_4:
				in_gamemode.int64s[3] = userData[a].m_int64;
				break;
			}
		}

		if( eLittleEndian )
		{
			SwapEndianBase( in_mapname.int64s, 4, true );
			SwapEndianBase( in_gamemode.int64s, 4, true );
		}//eLittleEndian

		memcpy( (void*)sz_mapname, (void*)in_mapname.str32s, 32 );
		memcpy( (void*)sz_gamemode, (void*)in_gamemode.str32s, 32 );

		CUIServerList::SServerInfo si;
		si.m_numPlayers   = session->m_numFilledSlots;
		si.m_maxPlayers   = 16;//session->m_data->m_numPrivateSlots + session->m_data->m_numPublicSlots;
		si.m_hostName     = session->m_data.m_name;
		si.m_mapName      = gEnv->pGame->GetMappedLevelName(sz_mapname);
		CRY_TODO(10, 02, 2010, "Needs to correctly get game version.");
		si.m_gameVersion  = "TODO";
		si.m_gameTypeName = sz_gamemode;
		// FIXME :
		//   Make server id unique in some other way....
		si.m_serverId     = (int)session->m_id.get(); // for lack of unique ids deref the pointer location of the server. This is the id that gets sent to flash...
		si.m_sessionId    = session->m_id;
		CRY_TODO(10, 02, 2010, "Need to get the server latency");
		si.m_ping         = 9999;

		mpMenuHub->AddServer(si);
	}

	if (error != eCLE_SuccessContinue && error != eCLE_Success)
	{
		CryLogAlways("CGameBrowser search for sessions error %d", (int)error);
		CHUD* pHUD = g_pGame->GetHUD();
		if (mpMenuHub && pHUD)
		{
			CryFixedStringT<32> errorMsg;
			errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
			errorMsg = pHUD->LocalizeString(errorMsg.c_str());
			mpMenuHub->ShowErrorDialog("LobbyError", errorMsg.c_str());
		}
	}

	if (error != eCLE_SuccessContinue)
	{
		CryLogAlways("CCGameBrowser::MatchmakingSessionSearchCallback DONE");

		CGameBrowser* pGameBrowser = (CGameBrowser*) arg;
		pGameBrowser->CancelSearching();
	}
}
