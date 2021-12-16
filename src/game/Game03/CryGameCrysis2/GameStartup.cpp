/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 2:8:2004   15:20 : Created by Márcio Martins

*************************************************************************/

#include "StdAfx.h"
#include "GameStartup.h"
#include "Game.h"
#include "GameCVars.h"

#include <StringUtils.h>
#include <CryFixedString.h>
#include <CryLibrary.h>
#include <platform_impl.h>
#include <INetworkService.h>

#include <LoadSeq.h>

#include "IHardwareMouse.h"
#include "ICryPak.h"
#include <ILocalizationManager.h>
#include "IBasicEventListener.h"
#include "Editor/GameRealtimeRemoteUpdate.h"
#include "Utility/StringUtils.h"
#include "Audio/Announcer.h"

#include "Testing/AutoTester.h"

static CAutoTester s_autoTesterSingleton;

#if defined(ENABLE_STATS_AGENT)
#include "StatsAgent.h"
#endif

#ifdef __LINK_GCOV__
extern "C" void __gcov_flush(void);
#define GCOV_FLUSH __gcov_flush()
namespace
{
	static void gcovFlushUpdate()
	{
		static unsigned sCounter = 0;
		static const sInterval = 1000;

		if (++sCounter == sInterval)
		{
			__gcov_flush();
			sCounter = 0;
		}
	}
}
#define GCOV_FLUSH_UPDATE gcovFlushUpdate()
#else
#define GCOV_FLUSH ((void)0)
#define GCOV_FLUSH_UPDATE ((void)0)
#endif

#if defined(PS3) && !defined(_LIB) 
#if defined PS3_PRX_CryAction
extern "C" IGameFramework *CreateGameFramework();
#endif
namespace
{
	static IGameFramework *PS3CreateGameFramework()
	{
#if defined PS3_PRX_CryAction
		return CreateGameFramework();
#else
		return gPS3Env->pInitFnTable->pInitAction();
#endif
	}
}
#define CreateGameFramework PS3CreateGameFramework
#elif defined(_LIB) || defined(LINUX) || defined(PS3)
extern "C" IGameFramework *CreateGameFramework();
#endif

#ifndef XENON
#define DLL_INITFUNC_CREATEGAME "CreateGameFramework"
#else
#define DLL_INITFUNC_CREATEGAME (LPCSTR)1
#endif

#ifdef WIN32
bool g_StickyKeysStatusSaved = false;
STICKYKEYS g_StartupStickyKeys = {sizeof(STICKYKEYS), 0};
TOGGLEKEYS g_StartupToggleKeys = {sizeof(TOGGLEKEYS), 0};
FILTERKEYS g_StartupFilterKeys = {sizeof(FILTERKEYS), 0};

#endif

static void RestoreStickyKeys();

static void AllowAccessibilityShortcutKeys(bool bAllowKeys)
{
#if defined(WIN32)
	if(!g_StickyKeysStatusSaved)
	{
		SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
		SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
		SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
		g_StickyKeysStatusSaved = true;
		atexit(RestoreStickyKeys);
	}

	if(bAllowKeys)
	{
		// Restore StickyKeys/etc to original state and enable Windows key      
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
	}
	else
	{
		STICKYKEYS skOff = g_StartupStickyKeys;
		skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
		skOff.dwFlags &= ~SKF_CONFIRMHOTKEY; 
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);

		TOGGLEKEYS tkOff = g_StartupToggleKeys;
		tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
		tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);

		FILTERKEYS fkOff = g_StartupFilterKeys;
		fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
		fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
	}
#endif
}




static void RestoreStickyKeys()
{
	AllowAccessibilityShortcutKeys(true);
}

#define EYEADAPTIONBASEDEFAULT		0.25f					// only needed for Crysis





//Diesel new
#define DEFAULT_CURSOR_RESOURCE_ID 105

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Game : public ISystemEventListener
{
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			g_random_generator.seed((uint32)wparam);
			break;
		case ESYSTEM_EVENT_CHANGE_FOCUS:
			{
				CGameStartup::AllowAccessibilityShortcutKeys(wparam==0); //Diesel cut
			}
			break;
		case ESYSTEM_EVENT_LEVEL_LOAD_START:
			{
				// workaround for needed for Crysis - to reset cvar set in level.cfg
				ICVar *pCVar = gEnv->pConsole->GetCVar("r_EyeAdaptationBase");		assert(pCVar);

				float fOldVal = pCVar->GetFVal();

				if(fOldVal!=EYEADAPTIONBASEDEFAULT)
				{
					CryLog("r_EyeAdaptationBase was reset to default");
					pCVar->Set(EYEADAPTIONBASEDEFAULT);		// set to default value
				}
			}
			break;

		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
			STLALLOCATOR_CLEANUP;
			break;
		}
	}
};
static CSystemEventListner_Game g_system_event_listener_game;

//IGame* CGameStartup::m_pMod = NULL;
//IGameRef CGameStartup::m_modRef;
//IGameFramework* CGameStartup::m_pFramework = NULL;

//HMODULE CGameStartup::m_modDll = 0;
//HMODULE CGameStartup::m_frameworkDll = 0;
//HMODULE CGameStartup::m_systemDll = 0;

//string CGameStartup::m_rootDir;
//string CGameStartup::m_binDir;
//string CGameStartup::m_reqModName;

//bool CGameStartup::m_initWindow = false;

//Diesel new







#if defined(WIN32) || defined (WIN64)
void debugLogCallStack()
{
	// Print call stack for each find.
	const char *funcs[32];
	int nCount = 32;

	CryLogAlways( "    ----- CallStack () -----");
	gEnv->pSystem->debug_GetCallStack( funcs, nCount);
	for (int i = 1; i < nCount; i++) // start from 1 to skip this function.
	{
		CryLogAlways( "    %02d) %s",i,funcs[i] );
	}
}
#endif

void GameStartupErrorObserver::OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber)
{
	if(!g_pGameCVars)
		return;

	if (g_pGameCVars->cl_logAsserts != 0)
		CryLogAlways("---ASSERT: condition:%s; message:%s; fileName:%s; line %d", condition, message, fileName, fileLineNumber);

#if defined(WIN32) || defined (WIN64)
	if (g_pGameCVars->cl_logAsserts > 1)
	{
		debugLogCallStack();
		CryLogAlways("----------------------------------------");
	}
#endif
}

void GameStartupErrorObserver::OnFatalError(const char* message)
{
	CryLogAlways("---FATAL ERROR: message:%s", message);

#if defined(WIN32) || defined (WIN64)
	gEnv->pSystem->debug_LogCallStack();
	CryLogAlways("----------------------------------------");
#endif
}

//////////////////////////////////////////////////////////////////////////

CGameStartup* CGameStartup::Create(IBasicEventListener& basicEventListener)
{
	static char buff[sizeof(CGameStartup)];
	return new (buff) CGameStartup(basicEventListener);
}

CGameStartup::CGameStartup(IBasicEventListener& basicEventListener)
	:	m_basicEventListener(basicEventListener), 
		m_pMod(NULL),
		m_modRef(&m_pMod),
		m_initWindow(false), 
		m_quit(false),
		m_modDll(0), 
		m_frameworkDll(NULL),
		m_pFramework(NULL)
{
	CGameStartupStatic::g_pGameStartup = this;
}

CGameStartup::~CGameStartup()
{
	if (m_pMod)
	{
		m_pMod->Shutdown();
		m_pMod = 0;
	}

	if (m_modDll)
	{
		CryFreeLibrary(m_modDll);
		m_modDll = 0;
	}

	ShutdownFramework();

	CGameStartupStatic::g_pGameStartup = NULL;
}






/*
IGameRef CGameStartup::Init(SSystemInitParams &startupParams)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Game startup initialisation");

	LOADING("game_startup");
	if (!InitFramework(startupParams))
	{
		return 0;
	}

#if defined(ENABLE_STATS_AGENT)
	CStatsAgent::CreatePipe();
#endif

  LOADING_TIME_PROFILE_SECTION(m_pFramework->GetISystem());

	ISystem* pSystem = m_pFramework->GetISystem();
	startupParams.pSystem = pSystem;

	REGISTER_COMMAND("g_loadMod", CGameStartupStatic::RequestLoadMod,VF_NULL,"");

	// load the appropriate game/mod
	const ICmdLineArg *pModArg = pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"MOD");

	// Load all localized strings.	- Moved above Reset so that any loaded menus (flash) have localized strings available.
	LoadLocalizationData();

	IGameRef pOut;
	if (pModArg && (*pModArg->GetValue() != 0) && (pSystem->IsMODValid(pModArg->GetValue())))
	{
		const char* pModName = pModArg->GetValue();
		assert(pModName);

		pOut = Reset(pModName);
	}
	else
	{
		pOut = Reset(GAME_NAME);
	}

	// Creating static classes (rather than using new) means they automagically get freed up properly on exit.
	// These are created before CompleteInit() because the lua scripts called during CompleteInit have been known
	// to use these classes!
	
	
//Diesel cut	
	static CAnnouncer s_announcerSingleton;



	if (!m_pFramework->CompleteInit())
	{
		pOut->Shutdown();
		return 0;
	}

	// Read in this data as late as possible (but before executing command-line params) so that all the required
	// systems (texture loading, audio, localization etc.) are functional.



//Diesel cut
	s_announcerSingleton.LoadAnnouncements("Scripts/Sounds/cw2_Annoucements.xml"); 

	LOADING_DONE;

	// should be after init game (should be executed even if there is no game)
	if(startupParams.bExecuteCommandLine)
		pSystem->ExecuteCommandLine();

	pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	// Creates and starts the realtime update system listener.
	if (pSystem->IsDevMode())
	{
		CGameRealtimeRemoteUpdateListener::GetGameRealtimeRemoteUpdateListener().Enable(true);
	}


	GCOV_FLUSH;

	if (gEnv && GetISystem())
	{
//Diesel cut
		//CryLogAlways("CGameStartup::Init() - Registering error observer");
		//GetISystem()->RegisterErrorObserver(&m_errorObsever);
//Diesel cut
	}
	else
	{
		CryLogAlways("failed to find ISystem to register error observer");
		assert(0);
	}

	return pOut;
}
*/



IGameRef CGameStartup::Init(SSystemInitParams &startupParams)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Game startup initialisation");

	LOADING("game_startup");

	if (!InitFramework(startupParams))
	{
		return 0;
	}

#if defined(ENABLE_STATS_AGENT)
	CStatsAgent::CreatePipe();
#endif

	// Configuration for this game
	ICVar *pCVar = gEnv->pConsole->GetCVar("ai_CompatibilityMode");
	if (pCVar)
		pCVar->Set("crysis");

  LOADING_TIME_PROFILE_SECTION(m_pFramework->GetISystem());

	ISystem* pSystem = m_pFramework->GetISystem();
	IConsole* pConsole = gEnv->pConsole;
	startupParams.pSystem = pSystem;

	//REGISTER_COMMAND("g_loadMod", RequestLoadMod,VF_NULL,""); //Diesel cut
	REGISTER_COMMAND("g_loadMod", CGameStartupStatic::RequestLoadMod,VF_NULL,""); //Diesel new

	// load the appropriate game/mod
	const ICmdLineArg *pModArg = pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"MOD");

	IGameRef pOut;
	if (pModArg && (*pModArg->GetValue() != 0) && (pSystem->IsMODValid(pModArg->GetValue())))
	{
		const char* pModName = pModArg->GetValue();
		assert(pModName);

		pOut = Reset(pModName);
	}
	else
	{
		pOut = Reset(GAME_NAME);
	}

	// Load all localized strings.
	LoadLocalizationData();

	if (!m_pFramework->CompleteInit())
	{
		pOut->Shutdown();
		return 0;
	}

	LOADING_DONE;

	// should be after init game (should be executed even if there is no game)
	if(startupParams.bExecuteCommandLine)
		pSystem->ExecuteCommandLine();

	pSystem->GetISystemEventDispatcher()->RegisterListener( &g_system_event_listener_game );
	//pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	// Creates and starts the realtime update system listener.
	if (pSystem->IsDevMode())
	{
		CGameRealtimeRemoteUpdateListener::GetGameRealtimeRemoteUpdateListener().Enable(true);
	}

	GCOV_FLUSH;

	return pOut;
}











//////////////////////////////////////////////////////////////////////////
void CGameStartup::LoadLocalizationData()
{
	// Loads any XML files in Languages directory

	ILocalizationManager *pLocMan = GetISystem()->GetLocalizationManager();

	string folderName = "Languages/";
	string search = folderName + "*.xml";

	ICryPak *pPak = gEnv->pCryPak;

	_finddata_t fd;
	intptr_t handle = pPak->FindFirst(search.c_str(), &fd);

	if (handle > -1)
	{
		do
		{
			CRY_ASSERT_MESSAGE(stricmp(PathUtil::GetExt(fd.name), "xml") == 0, "expected xml files only");

			string filename = folderName + fd.name;
			pLocMan->LoadExcelXmlSpreadsheet(filename.c_str());
		}
		while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Unable to find any Localization Data!");
	}
}

IGameRef CGameStartup::Reset(const char *pModName)
{
	if (m_pMod)
	{
		m_pMod->Shutdown();

		if (m_modDll)
		{
			CryFreeLibrary(m_modDll);
			m_modDll = 0;
		}
	}

	m_modDll = 0;
	string modPath;
	
#ifndef SP_DEMO
	if (stricmp(pModName, GAME_NAME) != 0)
	{
		modPath.append("Mods\\");
		modPath.append(pModName);
		modPath.append("\\");

		string filename;
		filename.append("..\\");
		filename.append(modPath);
		
#ifdef WIN64
		filename.append("Bin64\\");
#else
		filename.append("Bin32\\");
#endif
		
		filename.append(pModName);
		filename.append(".dll");

#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)
		m_modDll = CryLoadLibrary(filename.c_str());
#endif
	}
#endif

	if (!m_modDll)
	{
		ModuleInitISystem(m_pFramework->GetISystem(),"CryGame");
		static char pGameBuffer[sizeof(CGame)];
		m_pMod = new ((void*)pGameBuffer) CGame();
	}
	else
	{
		IGame::TEntryFunction CreateGame = (IGame::TEntryFunction)CryGetProcAddress(m_modDll, "CreateGame");
		if (!CreateGame)
			return 0;

		m_pMod = CreateGame(m_pFramework);
	}

	if (m_pMod && m_pMod->Init(m_pFramework))
	{
		return m_modRef;
	}

	return 0;
}

void CGameStartup::Shutdown()
{
#ifdef WIN32
	AllowAccessibilityShortcutKeys(true);
#endif

#if defined(ENABLE_STATS_AGENT)
	CStatsAgent::ClosePipe();
#endif

	/*delete this;*/
	this->~CGameStartup();
}

int CGameStartup::Update(bool haveFocus, unsigned int updateFlags)
{
	int returnCode = 0;

	if (gEnv && gEnv->pSystem && gEnv->pConsole)
	{
#ifdef WIN32
		if(gEnv && gEnv->pRenderer && gEnv->pRenderer->GetHWND())
		{
			bool focus = (::GetFocus() == gEnv->pRenderer->GetHWND());
			static bool focused = focus;
			if (focus != focused)
			{
				if(gEnv->pSystem->GetISystemEventDispatcher())
				{
					gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, focus, 0);
				}
				focused = focus;
			}
		}
#endif
	}

	// update the game
	if (m_pMod)
	{
		returnCode = m_pMod->Update(haveFocus, updateFlags);
	}

#if defined(ENABLE_STATS_AGENT)
	CStatsAgent::Update();
#endif

	// ghetto fullscreen detection, because renderer does not provide any kind of listener
	if (gEnv && gEnv->pSystem && gEnv->pConsole)
	{
		ICVar *pVar = gEnv->pConsole->GetCVar("r_Fullscreen");
		if (pVar)
		{
			static int fullscreen = pVar->GetIVal();
			if (fullscreen != pVar->GetIVal())
			{
				if(gEnv->pSystem->GetISystemEventDispatcher())
				{
					gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_TOGGLE_FULLSCREEN, pVar->GetIVal(), 0);
				}
				fullscreen = pVar->GetIVal();
			}
		}
	}

	s_autoTesterSingleton.Update();

	GCOV_FLUSH_UPDATE;

	return returnCode;
}

bool CGameStartup::GetRestartLevel(char** levelName)
{
	if(GetISystem()->IsRelaunch())
		*levelName = (char*)(gEnv->pGame->GetIGameFramework()->GetLevelName());
	return GetISystem()->IsRelaunch();
}

bool CGameStartup::GetRestartMod(char* pModName, int nameLenMax)
{
	if (m_reqModName.empty())
		return false;

	cry_strncpy(pModName, m_reqModName.c_str(), nameLenMax);
	return true;
}

const char* CGameStartup::GetPatch() const
{
	INetworkService* pService = gEnv->pNetwork->GetService("GameSpy");
	if(pService)
	{
		IPatchCheck* pPC = pService->GetPatchCheck();
		if(pPC && pPC->GetInstallOnExit())
		{
			return pPC->GetPatchFileName();
		}
	}
	return NULL;	
}




//Diesel new
void CGameStartup::AllowAccessibilityShortcutKeys(bool bAllowKeys)
{
#if defined(WIN32)
	if(!g_StickyKeysStatusSaved)
	{
		SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
		SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
		SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
		g_StickyKeysStatusSaved = true;
		atexit(RestoreStickyKeys);
	}

	if(bAllowKeys)
	{
		// Restore StickyKeys/etc to original state and enable Windows key      
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
	}
	else
	{
		STICKYKEYS skOff = g_StartupStickyKeys;
		skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
		skOff.dwFlags &= ~SKF_CONFIRMHOTKEY; 
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);

		TOGGLEKEYS tkOff = g_StartupToggleKeys;
		tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
		tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);

		FILTERKEYS fkOff = g_StartupFilterKeys;
		fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
		fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
	}
#endif
}
//Diesel new







int CGameStartup::Run( const char * autoStartLevelName )
{
	gEnv->pConsole->ExecuteString( "exec autoexec.cfg" );
	if (autoStartLevelName)
	{
		//load savegame
		if(CryStringUtils::stristr(autoStartLevelName, ".CRYSISJMSF") != 0 )
		{
			CryFixedStringT<256> fileName (autoStartLevelName);
			// NOTE! two step trimming is intended!
			fileName.Trim(" ");  // first:  remove enclosing spaces (outside ")
			fileName.Trim("\""); // second: remove potential enclosing "
			gEnv->pGame->GetIGameFramework()->LoadGame(fileName.c_str());
		}
		else	//start specified level
		{
			CryFixedStringT<256> mapCmd ("map ");
			mapCmd+=autoStartLevelName;
			gEnv->pConsole->ExecuteString(mapCmd.c_str());
		}
	}

#ifdef WIN32
	if (!(gEnv && gEnv->pSystem) || (!gEnv->IsEditor() && !gEnv->pSystem->IsDedicated()))
	{
		::ShowCursor(TRUE);
		if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
			gEnv->pSystem->GetIHardwareMouse()->DecrementCounter();
	}

	AllowAccessibilityShortcutKeys(false);

	for(;;)
	{
		MSG msg;

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message != WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
 				break;
			}
		}
		else
		{
			if (!Update(true, 0))
			{
				// need to clean the message loop (WM_QUIT might cause problems in the case of a restart)
				// another message loop might have WM_QUIT already so we cannot rely only on this 
				while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				break;
			}
		}
	}
#else
	// We should use bVisibleByDefault=false then...
	if (gEnv && gEnv->pHardwareMouse)
		gEnv->pHardwareMouse->DecrementCounter();

	for(;;)
	{
		if (!Update(true, 0))
		{
			break;
		}
	}
#endif //WIN32

	return 0;
}

void CGameStartup::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_RANDOM_SEED:
		g_random_generator.seed((uint32)wparam);
		break;
	case ESYSTEM_EVENT_CHANGE_FOCUS:
		{
			#ifdef WIN32

			AllowAccessibilityShortcutKeys(wparam==0);
			if (!wparam && !gEnv->pSystem->IsDevMode() && !gEnv->IsEditor() && g_pGame && g_pGame->GetIGameFramework() && g_pGame->GetIGameFramework()->IsGameStarted())
			{
				SAFE_MENU_FUNC(ShowInGameMenu(true));
			}
			#endif
		}
		break;
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			// hack for needed for Crysis - to reset cvar set in level.cfg
			ICVar *pCVar = gEnv->pConsole->GetCVar("r_EyeAdaptationBase");		assert(pCVar);

			float fOldVal = pCVar->GetFVal();

			if(fOldVal!=EYEADAPTIONBASEDEFAULT)
			{
				CryLog("r_EyeAdaptationBase was reset to default");
				pCVar->Set(EYEADAPTIONBASEDEFAULT);		// set to default value
			}
			// For MP gamemodes set the correct sound parameter
			// Default to SP
			float sp_coop_mp = 0.0f;
			if ( gEnv->bMultiplayer )
			{
				sp_coop_mp = 2.0f;
			}
			gEnv->pSoundSystem->SetGlobalParameter( "sp_coop_mp", sp_coop_mp );
			CryLog("sp_coop_mp set to %f", sp_coop_mp);
		}
		break;

	case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
		STLALLOCATOR_CLEANUP;
		break;

	case ESYSTEM_EVENT_SHUTDOWN:
		m_quit = true;
		break;
	}
}

bool CGameStartup::InitFramework(SSystemInitParams &startupParams)
{
#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)
	m_frameworkDll = GetFrameworkDLL(startupParams.szBinariesDir);

	if (!m_frameworkDll)
	{
		// failed to open the framework dll
		CryFatalError("Failed to open the GameFramework DLL!");
		
		return false;
	}

	IGameFramework::TEntryFunction CreateGameFramework = (IGameFramework::TEntryFunction)CryGetProcAddress(m_frameworkDll, DLL_INITFUNC_CREATEGAME );

	if (!CreateGameFramework)
	{
		// the dll is not a framework dll
		CryFatalError("Specified GameFramework DLL is not valid!");

		return false;
	}
#endif //_LIB

	m_pFramework = CreateGameFramework();

	if (!m_pFramework)
	{
		CryFatalError("Failed to create the GameFramework Interface!");
		// failed to create the framework

		return false;
	}

	if (!startupParams.hWnd)
	{
		m_initWindow = true;

		if (!InitWindow(startupParams))
		{
			// failed to register window class
			CryFatalError("Failed to register CryENGINE window class!");

			return false;
		}
	}

	// initialize the engine
	if (!m_pFramework->Init(startupParams))
	{
		CryFatalError("Failed to initialize CryENGINE!");

		return false;
	}
	ModuleInitISystem(m_pFramework->GetISystem(),"CryGame");

#ifdef WIN32
	SetWindowLongPtr(reinterpret_cast<HWND>(gEnv->pRenderer->GetHWND()), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#endif
	return true;
}

void CGameStartup::ShutdownFramework()
{
	if (m_pFramework)
	{
		m_pFramework->Shutdown();
		m_pFramework = 0;
	}

	ShutdownWindow();
}

bool CGameStartup::InitWindow(SSystemInitParams &startupParams)
{
#ifdef WIN32
	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));

	wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = (WNDPROC)CGameStartup::WndProcHndl;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(0);
#ifdef SP_DEMO
	wc.hIcon         = LoadIcon((HINSTANCE)startupParams.hInstance, MAKEINTRESOURCE(102));
	wc.hCursor       = LoadCursor((HINSTANCE)startupParams.hInstance, MAKEINTRESOURCE(101));
#else
	// FIXME: Very bad way of getting the Icon and Cursor from the Launcher project
	wc.hIcon         = LoadIcon((HINSTANCE)startupParams.hInstance, MAKEINTRESOURCE(101));
	wc.hCursor       = LoadCursor((HINSTANCE)startupParams.hInstance, MAKEINTRESOURCE(105));
#endif
	wc.hbrBackground =(HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = GAME_WINDOW_CLASSNAME;

	if (!RegisterClass(&wc))
	{
		return false;
	}

	if (startupParams.pSystem == NULL || (!startupParams.pSystem->IsEditor() && !startupParams.pSystem->IsDedicated()))
		::ShowCursor(FALSE);

#endif WIN32
	return true;

}

void CGameStartup::ShutdownWindow()
{
#ifdef WIN32
	if (m_initWindow)
	{
		UnregisterClass(GAME_WINDOW_CLASSNAME, GetModuleHandle(0));
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
#ifdef WIN32

LRESULT CALLBACK CGameStartup::WndProcHndl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CGameStartup* self = reinterpret_cast<CGameStartup*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return self 
		? self->WndProc(hWnd, msg, wParam, lParam)
		: DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CGameStartup::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	IBasicEventListener::EAction action = m_quit ? IBasicEventListener::eA_Default : ProcessMessage(hWnd, msg, wParam, lParam);
	switch (action)
	{
	case IBasicEventListener::eA_None:
		result = 0;
		break;
	case IBasicEventListener::eA_Default:
		result = DefWindowProc(hWnd, msg, wParam, lParam);;
		break;
	case IBasicEventListener::eA_ActivateAndEat:
		result = MA_ACTIVATEANDEAT;
		break;
	}

	return result;
}

IBasicEventListener::EAction CGameStartup::ProcessMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CRY_ASSERT(gEnv && gEnv->pSystem && !m_quit);

	IBasicEventListener::EAction result = IBasicEventListener::eA_Default;
	switch(msg)
	{
	case WM_CLOSE:
		result = m_basicEventListener.OnClose(hWnd);
		break;
	case WM_MOUSEACTIVATE:
		result = m_basicEventListener.OnMouseActivate(hWnd);
		break;
	case WM_ENTERSIZEMOVE:
		result = m_basicEventListener.OnEnterSizeMove(hWnd);
		break;
	case WM_EXITSIZEMOVE:
		result = m_basicEventListener.OnExitSizeMove(hWnd);
		break;
	case WM_ENTERMENULOOP:
		result = m_basicEventListener.OnEnterMenuLoop(hWnd);
		break;
	case WM_EXITMENULOOP:
		result = m_basicEventListener.OnExitMenuLoop(hWnd);
		break;
	case WM_HOTKEY:
		result = m_basicEventListener.OnHotKey(hWnd);
		break;
	case WM_SYSCHAR:	// prevent ALT + key combinations from creating 'ding' sounds
		result = m_basicEventListener.OnSycChar(hWnd);
		break;
	case WM_CHAR:
		result = m_basicEventListener.OnChar(hWnd, wParam);
		break;
	case WM_SYSKEYDOWN:	// prevent ALT-key entering menu loop
		result = m_basicEventListener.OnSysKeyDown(hWnd, wParam);
		break;
	case WM_SETCURSOR:
		result = m_basicEventListener.OnSetCursor(hWnd);
		break;
	case WM_MOUSEMOVE:
		result = m_basicEventListener.OnMouseMove(hWnd, lParam);
		break;
	case WM_LBUTTONDOWN:
		result = m_basicEventListener.OnLeftButtonDown(hWnd, lParam);
		break;
	case WM_LBUTTONUP:
		result = m_basicEventListener.OnLeftButtonUp(hWnd, lParam);
		break;
	case WM_LBUTTONDBLCLK:
		result = m_basicEventListener.OnLeftButtonDoubleClick(hWnd, lParam);
		break;
	case WM_MOVE:
		result = m_basicEventListener.OnMove(hWnd, lParam);
		break;
	case WM_SIZE:
		result = m_basicEventListener.OnSize(hWnd, lParam);
		break;
	case WM_ACTIVATE:
		result = m_basicEventListener.OnActivate(hWnd, wParam);
		break;
	case WM_SETFOCUS:
		result = m_basicEventListener.OnSetFocus(hWnd);
		if (g_pGameCVars)
		{
		g_pGameCVars->g_hasWindowFocus = true;
		}
		break;
	case WM_KILLFOCUS:
		result = m_basicEventListener.OnKillFocus(hWnd);
		if (g_pGameCVars)
		{
		g_pGameCVars->g_hasWindowFocus = false;
		}
		break;
	case WM_WINDOWPOSCHANGED:
		result = m_basicEventListener.OnWindowPositionChanged(hWnd);
		break;
	case WM_STYLECHANGED:
		result = m_basicEventListener.OnWindowStyleChanged(hWnd);
		break;
	case WM_INPUTLANGCHANGE:
		result = m_basicEventListener.OnInputLanguageChanged(hWnd, wParam, lParam);
		break;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////
#endif //WIN32

CGameStartup* CGameStartupStatic::g_pGameStartup = NULL;

void CGameStartupStatic::RequestLoadMod(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArgCount() == 2)
	{
		if (g_pGameStartup) 
		{	
			g_pGameStartup->m_reqModName = pCmdArgs->GetArg(1);
			ISystem* pSystem = g_pGameStartup->m_pFramework->GetISystem();
			pSystem->Quit();
		}
	}
	else
	{
		CryLog("Error, correct syntax is: g_loadMod modname");
	}
}

void CGameStartupStatic::ForceCursorUpdate()
{
#ifdef WIN32
	if(gEnv && gEnv->pRenderer && gEnv->pRenderer->GetHWND())
	{
		SendMessage(HWND(gEnv->pRenderer->GetHWND()),WM_SETCURSOR,0,0);
	}
#endif
}

//--------------------------------------------------------------------------



