/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:2:2010		10:00 : Rewritten by Christian Helmich	
- 2:8:2004		15:20 : Created by Márcio Martins

*************************************************************************/

#include "StdAfx.h"
#include "GameStartup.h"
#include "Game.h"
#include "../LiveCreate/GameRealtimeRemoteUpdate.h"

#ifndef USE_EXTERNAL_FRAMEWORK_DLL
#include "../GameFramework/MiniGameFramework.h"
#endif //not USE_EXTERNAL_FRAMEWORK_DLL

#include <StringUtils.h>
#include <CryFixedString.h>
#include <CryLibrary.h>
#include <platform_impl.h>
#include <INetworkService.h>

#include <LoadSeq.h>

#include <IHardwareMouse.h>
#include <ICryPak.h>
#include <ILocalizationManager.h>

//#include "Editor/GameRealtimeRemoteUpdate.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GCOV

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
#endif	//__LINK_GCOV__

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// DLL entry function
#ifdef USE_EXTERNAL_FRAMEWORK_DLL
#if defined(_LIB) || defined(LINUX) || defined(PS3)
extern "C"
{
	IGameFramework *CreateGameFramework();
}
#endif	//defined(_LIB) || defined(LINUX) || defined(PS3)

#ifndef XENON
#define DLL_INITFUNC_CREATEGAME "CreateGameFramework"


#endif	//XENON
#endif //USE_EXTERNAL_FRAMEWORK_DLL

extern "C"
{
	IGame*	CreateGame(IGameFramework* pGameFramework);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Windows system keys

#ifdef WIN32
bool		g_StickyKeysStatusSaved = false;
STICKYKEYS	g_StartupStickyKeys = {sizeof(STICKYKEYS), 0};
TOGGLEKEYS	g_StartupToggleKeys = {sizeof(TOGGLEKEYS), 0};
FILTERKEYS	g_StartupFilterKeys = {sizeof(FILTERKEYS), 0};

const static bool g_debugWindowsMessages = false;

void RestoreStickyKeys()
{
	CGameStartup::AllowAccessibilityShortcutKeys(true);
}
#endif	//WIN32

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// static globals
static	CSystemEventListener_Game	g_system_event_listener_game;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CGameStartup static members

IGame*			CGameStartup::m_pGame = NULL;
IGameRef		CGameStartup::m_gameRef;
HMODULE			CGameStartup::m_hGameDll = 0;
HMODULE			CGameStartup::m_hSystemDll = 0;
HMODULE			CGameStartup::m_hFrameworkDll = 0;
IGameFramework* CGameStartup::m_pFramework = NULL;

string			CGameStartup::m_rootDir;
string			CGameStartup::m_binDir;
string			CGameStartup::m_reqModName;

bool			CGameStartup::m_initWindow = false;

// CGameStartup static members/
//////////////////////////////////////////////////////////////////////////
// CGameStartup ctor/dtor
 
CGameStartup::CGameStartup()
{
	LOG_CODE_COVERAGE();
	m_gameRef = &m_pGame;
}


CGameStartup::~CGameStartup()
{
	LOG_CODE_COVERAGE();
	if (m_pGame)
	{
		m_pGame->Shutdown();
		m_pGame = 0;
	}

	if (m_hGameDll)
	{
		CryFreeLibrary(m_hGameDll);
		m_hGameDll = 0;
	}

	ShutdownFramework();
}


IGameStartup* CGameStartup::CreateInstance()
{
	LOG_CODE_COVERAGE();

	// at this point we have no dynamic memory allocation, and we cannot
	// rely on atexit() doing the right thing; the only recourse is to
	// have a static buffer that we use for this object
	static char gameStartup_buffer[sizeof(CGameStartup)];
	return new ((void*)gameStartup_buffer) CGameStartup();
}

// CGameStartup ctor/dtor/
//////////////////////////////////////////////////////////////////////////
// IGameStartup implementation
//- instance lifecycle

IGameRef CGameStartup::Init(SSystemInitParams& startupParams)
{
	LOG_CODE_COVERAGE();
	LOADING("game_startup");

	//TODO: replace with direct loading of CrySystem.dll

	bool bFrameworkInit = InitFramework(startupParams);
	CRY_ASSERT(bFrameworkInit);
	CRY_SAFE_RETURN(!bFrameworkInit, 0);

	CRY_ASSERT(m_pFramework);
	CRY_SAFE_RETURN(!m_pFramework, 0);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, 0);	

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, 0);

	// Configuration for this game
	ICVar* pCVar = gEnv->pConsole->GetCVar("ai_CompatibilityMode");
	CRY_ASSERT(pCVar);
	if (pCVar)
		pCVar->Set("crysis");	//what is this actually used for? plus, using cfg for this is way safer

	// get system	
	ISystem*	pSystem	= gEnv->pSystem;
	CRY_ASSERT(pSystem);
	CRY_SAFE_RETURN(!pSystem, 0);
	LOADING_TIME_PROFILE_SECTION(pSystem);
	startupParams.pSystem	= pSystem;

	// get console
	IConsole*	pConsole	= gEnv->pConsole;
	CRY_ASSERT(pConsole);
	CRY_SAFE_RETURN(!pConsole, 0);	

	// load the appropriate game/mod
	IGameRef	pOut = LoadGameMod();

	// Load all localized strings.
	LoadLocalizationData();

	bFrameworkInit = m_pFramework->CompleteInit();
	if (!bFrameworkInit)
	{
		pOut->Shutdown();
		return 0;
	}
	LOADING_DONE;

	
	// execute remaining command line (should be done even if there is no game loaded)
	if(startupParams.bExecuteCommandLine)
		pSystem->ExecuteCommandLine();

	//register event listener
	ISystemEventDispatcher* pSystemEventDispatcher = pSystem->GetISystemEventDispatcher();
	CRY_ASSERT(pSystemEventDispatcher);
	CRY_SAFE_RETURN(!pSystemEventDispatcher, 0);
	pSystemEventDispatcher->RegisterListener( &g_system_event_listener_game );

	// initialize LiveCreate in DevMode
	if (pSystem->IsDevMode())
	{
		CGameRealtimeRemoteUpdateListener::GetGameRealtimeRemoteUpdateListener().Enable(true);
	}

	GCOV_FLUSH;

	return pOut;
}

//-----------------------------------

IGameRef CGameStartup::Reset(const char* pModName)
{
	LOG_CODE_COVERAGE();

	//shutdown currently loaded mod, if there is any
	if (m_pGame)
	{
		m_pGame->Shutdown();	//calls destructor, so next is fine
		m_pGame = 0;
	}

	//create new game
	ModuleInitISystem(m_pFramework->GetISystem(), "CryGame");

	CRY_SAFE_CREATE(m_pGame, CreateGame(m_pFramework));
	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, 0);

	//finally initialize game
	bool bGameInit = 
		m_pGame->Init(m_pFramework);
	CRY_ASSERT(bGameInit);
	CRY_SAFE_RETURN(!bGameInit, 0);
	
	//return m_modRef;	//IGameRef holds a ** pointing to &m_pMod, so this is valid
	return IGameRef(&m_pGame);
}

//-----------------------------------

void CGameStartup::Shutdown()
{
	LOG_CODE_COVERAGE();
#ifdef WIN32
	AllowAccessibilityShortcutKeys(true);
#endif
	//dtor called manually since allocated on static memory
	this->~CGameStartup();
}

//-----------------------------------

int CGameStartup::Update(bool haveFocus, unsigned int updateFlags)
{
	LOG_CODE_COVERAGE();
	int returnCode = 0;

	//note: if these assertions happen because the game is not loaded, replace with if();
	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, 0);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, 0);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, 0);

	CRY_ASSERT(gEnv->pRenderer);
	CRY_SAFE_RETURN(!gEnv->pRenderer, 0);

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, 0);

#ifdef WIN32 //WIN64 as well?
	// check if app has focus (on windows)
	WIN_HWND hWnd = gEnv->pRenderer->GetHWND();
	if(hWnd)
	{		
		bool focus = (::GetFocus() == hWnd);
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

	// fullscreen detection, because renderer does not provide any kind of listener
	// windows only, consoles don't offer windowed mode AFAIK
	ICVar* pVar = gEnv->pConsole->GetCVar("r_Fullscreen");
	CRY_ASSERT(pVar);
	CRY_SAFE_RETURN(!pVar, 0);

	static int fullscreen = pVar->GetIVal();
	if (fullscreen != pVar->GetIVal())
	{
		if(gEnv->pSystem->GetISystemEventDispatcher())
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_TOGGLE_FULLSCREEN, pVar->GetIVal(), 0);
		}
		fullscreen = pVar->GetIVal();		
	}
#endif //WIN32	

	// update the game
	returnCode = m_pGame->Update(haveFocus, updateFlags);
	
	GCOV_FLUSH_UPDATE;

	return returnCode;
}

//- instance lifecycle/
//////////////////////////////////////////////////////////////////////////
//- level

bool CGameStartup::GetRestartLevel(char** pszLevelName)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pszLevelName);
	CRY_SAFE_RETURN(!pszLevelName, false);

	ISystem* pSystem = GetISystem();
	CRY_ASSERT(pSystem);
	CRY_SAFE_RETURN(!pSystem, false);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	IGame* pGame = gEnv->pGame;
	CRY_ASSERT(pGame);
	CRY_SAFE_RETURN(!pGame, false);

	IGameFramework* pGameFwrk = pGame->GetIGameFramework();
	CRY_ASSERT(pGameFwrk);
	CRY_SAFE_RETURN(!pGameFwrk, false);

	bool bIsRelaunch = pSystem->IsRelaunch();
	if(bIsRelaunch)
	{
		*pszLevelName = (char*)(pGameFwrk->GetLevelName());
	}
	return bIsRelaunch;
}

//-----------------------------------

int CGameStartup::Run(const char* autoStartLevelName)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, 0);
	
	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, 0);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, 0);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, 0);

	IGameFramework* pGameFwrk = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFwrk);
	CRY_SAFE_RETURN(!pGameFwrk, 0);

	//run autoexec.cfg
	gEnv->pConsole->ExecuteString( "exec autoexec.cfg" );

	//load start level (if given)
	if (autoStartLevelName)
	{
		//autoload savegame
		if(CryStringUtils::stristr(autoStartLevelName, GAME_SAVE_EXT) != 0 )
		{
			CryFixedStringT<256> fileName (autoStartLevelName);
			// NOTE! two step trimming is intended!
			fileName.Trim(" ");  // first:  remove enclosing spaces (outside ")
			fileName.Trim("\""); // second: remove potential enclosing "
			pGameFwrk->LoadGame(fileName.c_str());
		}
		else	//start specified level
		{
			CryFixedStringT<256> mapCmd ("map ");
			mapCmd.append(autoStartLevelName);
			gEnv->pConsole->ExecuteString(mapCmd.c_str());
		}
	}

#ifdef WIN32
	if (	!(gEnv && gEnv->pSystem) 
		||	(!gEnv->IsEditor() && !gEnv->pSystem->IsDedicated())	)
	{
		::ShowCursor(TRUE);
		if (	gEnv 
			&&	gEnv->pSystem 
			&&	gEnv->pSystem->GetIHardwareMouse()	)
		{
			gEnv->pSystem->GetIHardwareMouse()->DecrementCounter();
		}
	}

	AllowAccessibilityShortcutKeys(false);

	//windows message pump
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
			if (! Update(true, 0))
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


//- level/
//////////////////////////////////////////////////////////////////////////
//- patch & mod

const char* CGameStartup::GetPatch() const
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, NULL);
	
	CRY_ASSERT(gEnv->pNetwork);
	CRY_SAFE_RETURN(!gEnv->pNetwork, NULL);

#ifdef USE_GAMESPY
	INetworkService* pService = gEnv->pNetwork->GetService("GameSpy");
	CRY_ASSERT(pService);
	CRY_SAFE_RETURN(!pService, NULL);

	IPatchCheck* pPC = pService->GetPatchCheck();
	CRY_ASSERT(pPC);
	CRY_SAFE_RETURN(!pPC, NULL);

	if(pPC->GetInstallOnExit())
	{
		return pPC->GetPatchFileName();
	}
#endif //USE_GAMESPY
	return NULL;	
}

//-----------------------------------

bool CGameStartup::GetRestartMod(char* pModName, int nameLenMax)
{
	LOG_CODE_COVERAGE();

	if (m_reqModName.empty())
		return false;

	strncpy(pModName, m_reqModName.c_str(), nameLenMax);
	pModName[nameLenMax-1] = 0;
	return true;
}

//- patch & mod
// IGameStartup implementation/
//////////////////////////////////////////////////////////////////////////
//per-instance utilities

//- localization
void CGameStartup::LoadLocalizationData()
{
	ISystem* pSystem = GetISystem();
	CRY_ASSERT(pSystem);
	CRY_SAFE_RETURN(!pSystem, CRY_NO_RETURN_VALUE);

	ILocalizationManager* pLocMan = pSystem->GetLocalizationManager();
	CRY_ASSERT(pLocMan);
	CRY_SAFE_RETURN(!pLocMan, CRY_NO_RETURN_VALUE);

	// Load localization xml.
	//TODO: iterate over all xml files in /languages to load them
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/dialog_recording_list.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/ai_dialog_recording_list.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/ui_dialog_recording_list.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/ui_text_messages.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/mp_text_messages.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/game_text_messages.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/game_controls.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/ps_basic_tutorial_subtitles.xml" );
// 	pLocMan->LoadExcelXmlSpreadsheet( "Languages/ui_credit_list.xml" );
}

//-----------------------------------
//- load mod or game, according to cmd line
IGameRef CGameStartup::LoadGameMod()
{
	LOG_CODE_COVERAGE();

	return Reset(NULL);
}

//per-instance utilities/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//public static utilities

void CGameStartup::ForceCursorUpdate()
{
	LOG_CODE_COVERAGE();
#ifdef WIN32
	if (	gEnv 
		&&	gEnv->pRenderer 
		&&	gEnv->pRenderer->GetHWND()	)
	{
		SendMessage(HWND(gEnv->pRenderer->GetHWND()),WM_SETCURSOR,0,0);
	}
#endif
}

//-----------------------------------

void CGameStartup::AllowAccessibilityShortcutKeys(bool bAllowKeys)
{
	LOG_CODE_COVERAGE();
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
#endif //WIN32
}

//public static utilities/
//////////////////////////////////////////////////////////////////////////
//protected static utilities

bool CGameStartup::InitWindow(SSystemInitParams& startupParams)
{
	LOG_CODE_COVERAGE();
#ifdef WIN32
	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));

	wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = (WNDPROC)CGameStartup::WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(0);
	// FIXME: Very bad way of getting the Icon and Cursor from the Launcher project
	wc.hIcon         = LoadIcon((HINSTANCE)startupParams.hInstance, MAKEINTRESOURCE(101));
	wc.hCursor       = LoadCursor((HINSTANCE)startupParams.hInstance, MAKEINTRESOURCE(DEFAULT_CURSOR_RESOURCE_ID));
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

//-----------------------------------

void CGameStartup::ShutdownWindow()
{
	LOG_CODE_COVERAGE();
#ifdef WIN32
	if (m_initWindow)
	{
		UnregisterClass(GAME_WINDOW_CLASSNAME, GetModuleHandle(0));
	}
#endif
}

//-----------------------------------
#ifdef WIN32
LRESULT CALLBACK CGameStartup::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LOG_CODE_COVERAGE();
	switch(msg)
	{
	case WM_CLOSE:
		if (gEnv && gEnv->pSystem)
			gEnv->pSystem->Quit();
		return 0;
	case WM_MOUSEACTIVATE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_MOUSEACTIVATE (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		return MA_ACTIVATEANDEAT;
	case WM_ENTERSIZEMOVE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_ENTERSIZEMOVE (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
		{
			gEnv->pSystem->GetIHardwareMouse()->IncrementCounter();
		}
		return  0;
	case WM_EXITSIZEMOVE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_EXITSIZEMOVE (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
		{
			gEnv->pSystem->GetIHardwareMouse()->DecrementCounter();
		}
		return  0;
	case WM_ENTERMENULOOP:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_ENTERMENULOOP (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
		{
			gEnv->pSystem->GetIHardwareMouse()->IncrementCounter();
		}
		return  0;
	case WM_EXITMENULOOP:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_EXITMENULOOP (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
		{
			gEnv->pSystem->GetIHardwareMouse()->DecrementCounter();
		}
		return  0;
	case WM_HOTKEY:
	case WM_SYSCHAR:	// prevent ALT + key combinations from creating 'ding' sounds
		return  0;
	case WM_CHAR:
		{
			if (gEnv && gEnv->pInput)
			{
				SInputEvent event;
				event.modifiers = gEnv->pInput->GetModifiers();
				event.deviceId = eDI_Keyboard;
				event.state = eIS_UI;
				event.value = 1.0f;
				event.pSymbol = 0;//m_rawKeyboard->GetSymbol((lParam>>16)&0xff);
				if (event.pSymbol)
					event.keyId = event.pSymbol->keyId;

				wchar_t tmp[2] = { 0 };
				MultiByteToWideChar(CP_ACP, 0, (char*)&wParam, 1, tmp, 2);
				event.timestamp = tmp[0];

				char szKeyName[4] = {0};
				if (wctomb(szKeyName, (WCHAR)wParam) != -1)
				{
					if (szKeyName[1]==0 && ((unsigned char)szKeyName[0])>=32)
					{
						event.keyName = szKeyName;
						gEnv->pInput->PostInputEvent(event);
					}
				}
			}
		}
		break;
	case WM_SYSKEYDOWN:	// prevent ALT-key entering menu loop
		if (wParam != VK_RETURN && wParam != VK_F4)
		{
			return 0;
		}
		else
		{
			if (wParam == VK_RETURN)	// toggle fullscreen
			{
				if (gEnv && gEnv->pRenderer && gEnv->pRenderer->GetRenderType() != eRT_DX11)
				{
					ICVar *pVar = gEnv->pConsole->GetCVar("r_Fullscreen");
					if (pVar)
					{
						int fullscreen = pVar->GetIVal();
						pVar->Set((int)(fullscreen == 0));
					}
				}
			}
			// let the F4 pass through to default handler (it will send an WM_CLOSE)
		}
		break;
	case WM_SETCURSOR:
		if(g_pGame)
		{
			HCURSOR hCursor = LoadCursor(GetModuleHandle(0),MAKEINTRESOURCE(DEFAULT_CURSOR_RESOURCE_ID));
			::SetCursor(hCursor);
		}
		return 0;
	case WM_MOUSEMOVE:
		if(gEnv && gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_MOVE);
		}
		return 0;
	case WM_LBUTTONDOWN:
		if(gEnv && gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_LBUTTONDOWN);
		}
		return 0;
	case WM_LBUTTONUP:
		if(gEnv && gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_LBUTTONUP);
		}
		return 0;
	case WM_LBUTTONDBLCLK:
		if(gEnv && gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK);
		}
		return 0;
	case WM_MOVE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_MOVE %d %d (%s %s)", LOWORD(lParam), HIWORD(lParam), (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE,LOWORD(lParam), HIWORD(lParam));
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_SIZE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_SIZE %d %d (%s %s)", LOWORD(lParam), HIWORD(lParam), (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE,LOWORD(lParam), HIWORD(lParam));
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_ACTIVATE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_ACTIVATE %d (%s %s)", LOWORD(wParam), (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");

		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			//gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, LOWORD(wParam) != WA_INACTIVE, 0);
		}
		break;
	case WM_SETFOCUS:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_SETFOCUS (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			//gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 1, 0);
		}
		break;
	case WM_KILLFOCUS:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_KILLFOCUS (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			//gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 0, 0);
		}
		break;
	case WM_WINDOWPOSCHANGED:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_WINDOWPOSCHANGED (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			//gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 1, 0);
		}
		break;
	case WM_STYLECHANGED:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_STYLECHANGED (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 1, 0);
		}
		break;
	case WM_INPUTLANGCHANGE:
		if (g_debugWindowsMessages && gEnv && gEnv->pLog)
			gEnv->pLog->Log("MSG: WM_INPUTLANGCHANGE");
		if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LANGUAGE_CHANGE, wParam, lParam);
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif //WIN32
//////////////////////////////////////////////////////////////////////////

bool CGameStartup::InitFramework(SSystemInitParams& startupParams)
{
	LOG_CODE_COVERAGE();
#ifdef USE_EXTERNAL_FRAMEWORK_DLL
#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)
	//load 
	m_hFrameworkDll = GetFrameworkDLL();
	CRY_ASSERT(m_hFrameworkDll);
	if (!m_hFrameworkDll)
	{
		// failed to open the framework dll
		CryFatalError("Failed to open the GameFramework DLL!");
		return false;
	}

	IGameFramework::TEntryFunction CreateGameFramework = (IGameFramework::TEntryFunction)CryGetProcAddress(m_hFrameworkDll, DLL_INITFUNC_CREATEGAME );
	CRY_ASSERT(CreateGameFramework);
	if (!CreateGameFramework)
	{
		// the dll is not a framework dll
		CryFatalError("Specified GameFramework DLL is not valid!");
		return false;
	}
#endif //!_LIB && !LINUX && !PS3
	// create game framework
	CRY_SAFE_CREATE(m_pFramework, (CreateGameFramework()) );
#else	
	CRY_SAFE_CREATE(m_pFramework, (simpleframework::CMiniGameFramework::CreateInstance()) );
#endif //USE_EXTERNAL_FRAMEWORK_DLL
	
	CRY_ASSERT(m_pFramework);
	if (!m_pFramework)
	{
		// failed to create the framework
		CryFatalError("Failed to create the GameFramework Interface!");
		return false;
	}

	// initialize window
	if (!startupParams.hWnd)
	{
		m_initWindow = true;
		bool bInitWndSuccess = InitWindow(startupParams);
		CRY_ASSERT(bInitWndSuccess);
		if (!bInitWndSuccess)
		{
			// failed to register window class
			CryFatalError("Failed to register CryENGINE window class!");
			return false;
		}
	}

	// initialize the engine
	bool bFrameworkInit = m_pFramework->Init(startupParams);
	CRY_ASSERT(bFrameworkInit);
	if (!bFrameworkInit)
	{
		CryFatalError("Failed to initialize CryENGINE!");
		return false;
	}

	ModuleInitISystem(m_pFramework->GetISystem(), "CryGame");

	return true;
}

//-----------------------------------

void CGameStartup::ShutdownFramework()
{
	LOG_CODE_COVERAGE();
	if (m_pFramework)
	{
		m_pFramework->Shutdown();	//calls dtor, so next is safe
		m_pFramework = 0;
	}

	ShutdownWindow();
}

//protected static utilities/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(IGameStartup)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CSystemEventListener_Game::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	LOG_CODE_COVERAGE();
	switch (event)
	{
	case ESYSTEM_EVENT_RANDOM_SEED:
		g_random_generator.seed((uint32)wparam);
		break;

	case ESYSTEM_EVENT_CHANGE_FOCUS:
		CGameStartup::AllowAccessibilityShortcutKeys(wparam==0);		
		break;

	case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
		STLALLOCATOR_CLEANUP;
		break;
	}
}

//#include UNIQUE_VIRTUAL_WRAPPER(ISystemEventListener)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
