#include "StdAfx.h"
#include <StlUtils.h>

#include "FlashFrontEnd.h"

#include <IPlatformOS.h>
#include "IPlayerProfiles.h"
#include "Flash/Flash.h"
#include "Menus/OptionsManager.h"
#include "ProfileOptions.h"
#include "Game.h"
#include "GameCVars.h"
#include "StringUtils.h"
#include "FrontEnd/MenuData.h"
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "HUD/ScreenLayoutManager.h"
#include "GameActions.h"
#include "Graphics/2DRenderUtils.h"
#include "Multiplayer/MPMenuHub.h"
#include "Utility/CryWatch.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"


static const float MIN_ASPECT_RATIO = (16.0f / 9.0f);

#define KEY_REPEAT_DELAY 400.f // In ms
#define KEY_REPEAT_SPEED 40.f // In ms

#define GAME_SELECT_SCREEN "game_select"

//////////////////////////////////////////////////////////////////////////



CFlashFrontEnd::CFlashFrontEnd()
: m_pFlashUI(NULL)
, m_currentScreenIndex(-1)
, m_nextLoadLevel("")
, m_maxLevelProgress(500)
, m_scheduleClear(false)
, m_scheduleInitialize(false)
, m_scheduleReload(false)
, m_scheduleQuit(false)
, m_scheduleSwitchGame(false)
, m_switchGameMultiplayer(false)
, m_isFirstRun(true)
, m_currentMenu(eFlM_Menu)
, m_currentScreen(eFFES_unknown)
, m_backgroundTexture(NULL)
, m_currentResolution(0, 0)
, m_mpMenuHub(0)
, m_frontEndInited(false)
, m_ingameInited(false)
, m_repeatTimer(0.f)
, m_hudHidePS3HackErmWorkaround(false)
{
	SAFE_HARDWARE_MOUSE_FUNC(AddListener(this));
	gEnv->pGame->GetIGameFramework()->RegisterListener(this, "frontend", FRAMEWORKLISTENERPRIORITY_MENU);
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);
	if (gEnv->pInput)
		gEnv->pInput->AddEventListener(this);
#if defined(XENON) || defined(PS3)
	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
#endif //defined(XENON) || defined(PS3)

	RegisterCommand(eFECMD_command, "cmd");
	RegisterCommand(eFECMD_goto, "goto");
	RegisterCommand(eFECMD_exit, "exit");
	RegisterCommand(eFECMD_back, "back");
	RegisterCommand(eFECMD_load, "load");
	RegisterCommand(eFECMD_resume, "resume");
	RegisterCommand(eFECMD_restart, "restart");
	RegisterCommand(eFECMD_back_to_mainmenu, "back_to_mainmenu");
	RegisterCommand(eFECMD_suicide, "suicide");
	RegisterCommand(eFECMD_switch_game, "switch_game");
	RegisterCommand(eFECMD_set_highlight, "menu_set_highlight");
	RegisterCommand(eFECMD_verify_profile, "verify_profile");

	m_lastUpdateTime = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);
	g_pGame->GetProfileOptions()->InitializeFromProfile();
}



CFlashFrontEnd::~CFlashFrontEnd()
{
#if defined(XENON) || defined(PS3)
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
#endif //defined(XENON) || defined(PS3)
	if (gEnv->pInput)
		gEnv->pInput->RemoveEventListener(this);
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
	gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
	SAFE_HARDWARE_MOUSE_FUNC(RemoveListener(this));
	Clear();

	SAFE_DELETE(m_mpMenuHub);
}



void CFlashFrontEnd::ScheduleInitialize(EFlashMenus menu)
{
	m_currentMenu = menu;
	m_scheduleInitialize = true;
}



void CFlashFrontEnd::Initialize(EFlashMenus menu /*= eFlM_NotSpecified*/)
{
	if(gEnv->bEditor)
		return;

	if(menu==eFlM_NotSpecified)
	{
		menu = m_currentMenu;
	}
	
	bool gameSelect = (menu == eFlM_GameSelect);

	const bool gameRunning = IsGameActive();
	if(menu!=eFlM_LoadingScreen)
	{
		m_parser.Read(m_menuData, gameRunning, gameSelect);
	}

	const char* path = NULL;
	bool background = true;


	if(menu==eFlM_GameSelect)
	{
		path = "Libs/UI/Menus_StartMenu.gfx";
		m_audio.EnterMenu(gameRunning);
	}
	else if(menu==eFlM_Menu)
	{
		path = "Libs/UI/Menus_StartMenu.gfx";
		m_audio.EnterMenu(gameRunning);

		if (m_frontEndInited==false)
		{
			InitFrontEndMenu();
		}
	}
	else if(menu==eFlM_IngameMenu)
	{
		path = "Libs/UI/Menus_StartMenu.gfx"; // Currently ingame menu uses the same flash file as the frontend
		m_audio.EnterMenu(gameRunning);

		if (m_ingameInited==false)
		{
			InitIngameMenu();
		}
	}
	else if(menu==eFlM_LoadingScreen)
	{
		path = "Libs/UI/IdleAnimation.gfx";
	}
	else if(menu==eFlM_ResetScreen)
	{
		path = "Libs/UI/cw2_Menus_RestartScreenProgress.gfx";
	}

	if (menu!=eFlM_Menu && m_frontEndInited==true)
	{
		DestroyFrontEndMenu();
	}

	if(!IsGameActive() || menu==eFlM_LoadingScreen)
		LoadBackground();

	m_pFlashUI = CFlash::Get()->CreateSafeFlashPlayerInstance();

	//m_pFlashUI->SetFSCommandHandler(this); //Diesel cut
	//m_pFlashUI->Load(path); //Diesel cut
	//m_pFlashUI->SetBackgroundAlpha(0.0f); //Diesel cut

	if (m_mpMenuHub)
	{
		m_mpMenuHub->SetFlashPlayer(m_pFlashUI);
	}

// #if !defined(PS3) && !defined(XENON)
// 	m_pFlashUI->Invoke1("loadFooter", "PC");
// #else
// #if defined(PS3)
// 	m_pFlashUI->Invoke1("loadFooter", "PS3");
// #else
// 	m_pFlashUI->Invoke1("loadFooter", "XBox360");
// #endif
// #endif

	m_currentMenu = menu;

// #if !defined(PS3) && !defined(XENON)
// 	SAFE_HARDWARE_MOUSE_FUNC(IncrementCounter());
// #endif

	ScreenForward("main");

	m_repeatEvent.keyId = eKI_Unknown;

	g_pGame->GetProfileOptions()->InitializeFromCVar();
}


void CFlashFrontEnd::InitIngameMenu()
{
	CRY_ASSERT(m_ingameInited==false);

	if (m_mpMenuHub)
		m_mpMenuHub->InitIngameMenu();
	
	m_ingameInited = true;
}

void CFlashFrontEnd::DestroyIngameMenu()
{
	if (m_ingameInited==true)
	{
		if (m_mpMenuHub)
			m_mpMenuHub->DestroyIngameMenu();

		m_ingameInited = false;
	}
}

void CFlashFrontEnd::InitFrontEndMenu()
{
	CRY_ASSERT(m_frontEndInited==false);

	if (g_pGameCVars->g_multiplayerDefault!=0)
	{
		m_mpMenuHub = new CMPMenuHub();
		m_mpMenuHub->InitFrontEndMenu();
	}
	
	m_frontEndInited = true;
}

void CFlashFrontEnd::DestroyFrontEndMenu()
{
	CRY_ASSERT(m_frontEndInited==true);

	SAFE_DELETE(m_mpMenuHub);

	if (m_mpMenuHub)
		m_mpMenuHub->DestroyFrontEndMenu();
	
	m_frontEndInited = false;
}

void CFlashFrontEnd::Clear()
{
	if (m_currentMenu==eFlM_LoadingScreen)
	{
		if( gEnv->bMultiplayer )
		{
			g_pGame->GetHUD()->ActivateState("mp_welcome_screen");
		}
	}

	if (m_currentScreenIndex>0)
	{
		LeaveScreen(m_menuData.GetScreen(m_currentScreenIndex)->GetScreenType());
		m_currentScreenIndex = -1;
	}

	m_menuData.Clear();

	m_currentResolution = Vec2i(0,0);
	SAFE_RELEASE(m_pFlashUI);

	if (m_mpMenuHub)
		m_mpMenuHub->SetFlashPlayer(NULL);

	g_pGame->GetProfileOptions()->SaveProfile();

	m_audio.LeaveMenu();

/*
	if(m_backgroundTexture)
	{
		gEnv->pRenderer->RemoveTexture(m_backgroundTexture->GetTextureID());
		m_backgroundTexture = NULL;
	}
*/

	m_screenStack.clear();

	if (g_pGame->GetWarnings())
	{
		g_pGame->GetWarnings()->ClearCurrentWarnings();
	}

	if(m_hudHidePS3HackErmWorkaround)
	{
		if(ICVar* pCVar = gEnv->pConsole->GetCVar("hud_hide"))
		{
			pCVar->Set(0);
		}
		m_hudHidePS3HackErmWorkaround = false;
	}

	// #if !defined(PS3) && !defined(XENON)
// 	SAFE_HARDWARE_MOUSE_FUNC(DecrementCounter());
// #endif
}



void CFlashFrontEnd::OnPostUpdate(float frameTime)
{
	CorrectFrameTime(frameTime);

	WorkSchedules();

#if !defined(_RELEASE)
	DebugWatches();
#endif

	if(!IsGameActive())
	{
		const char* path = m_parser.AssembleCurrentPath(IsGameActive(), m_currentMenu==eFlM_GameSelect);
		if(stricmp(path, m_menuData.GetPath()))
		{
			g_pGame->GetHUD()->Unload(); // Unload the Hud (if it isn't already)

			if (m_ingameInited)
			{
				DestroyIngameMenu();
			}
			
			if(IsMenuLoaded())
				m_scheduleClear = true;

			m_scheduleInitialize = true;
			m_currentMenu = eFlM_Menu;
			return;
		}
	}

	if(!IsMenuLoaded())
		return;

	if (m_mpMenuHub)
		m_mpMenuHub->Update(frameTime);

#if defined(XENON) || defined(PS3)
	if (m_currentMenu == eFlM_GameSelect)
	{
		if (m_currentScreen == eFFES_profile_login)
		{
			IPlatformOS* os = GetISystem()->GetPlatformOS();
			unsigned int userIndex = g_pGame->GetExclusiveControllerDeviceIndex();
			const bool signedIn = os->UserIsSignedIn(userIndex);
			if (signedIn)
			{
				Execute(eFECMD_goto, GAME_SELECT_SCREEN);
			}
		}
	}
#endif


	// Handle key repeat
	if (m_repeatEvent.keyId != eKI_Unknown)
	{
		float nextTimer = KEY_REPEAT_SPEED;
		float now = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();

		if (now - m_repeatTimer > nextTimer)
		{
			OnInputEvent(m_repeatEvent);
			m_repeatTimer = now + nextTimer;
		}
	}

	int width = gEnv->pRenderer->GetWidth();
	int height = gEnv->pRenderer->GetHeight();
	
	if(m_currentResolution.x != width || m_currentResolution.y != height)
	{
		UpdateResolution(width, height);
	}

	if(m_backgroundTexture && m_currentMenu != eFlM_IngameMenu)
	{
		gEnv->pRenderer->Draw2dImage(0.0f, 0.0f, 800.0f, 600.0f, m_backgroundTexture->GetTextureID(), 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
	}
	else
	{
		ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
		C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
		const float fWidth  = pLayoutManager->GetVirtualWidth();
		const float fHeight = pLayoutManager->GetVirtualHeight();
		pRenderUtils->PreRender();
		pLayoutManager->SetState(eSLO_FullScreen);
		pRenderUtils->DrawQuad(0.0f, 0.0f, fWidth, fHeight, ColorF(0.0f, 0.0f, 0.0f, 0.8f));
		pRenderUtils->PostRender();
	}

	m_pFlashUI->Advance(frameTime);
	m_pFlashUI->Render();

	g_pGame->GetWarnings()->Update(frameTime);
}



void CFlashFrontEnd::CorrectFrameTime(float &frameTime)
{
	float now = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);

	if(m_currentMenu == eFlM_LoadingScreen)
	{
		now = gEnv->pTimer->GetAsyncCurTime() ;
	}

	if(frameTime <= 0.0f)
	{
		frameTime = now - m_lastUpdateTime;
	}
	m_lastUpdateTime = now;
}



void CFlashFrontEnd::WorkSchedules()
{
	if(m_scheduleClear)
	{
		Clear();
		m_scheduleClear = false;
	}

	if(m_scheduleInitialize)
	{
		Initialize();
		m_scheduleInitialize = false;
	}

	if(m_scheduleReload)
	{
		g_pGame->InitMapReloading();
		m_scheduleReload = false;
		m_isReloading = true;
	}

	if(m_scheduleQuit)
	{
		g_pGame->GetProfileOptions()->SaveProfile();
		gEnv->pSystem->Quit();
	}

	if (m_scheduleSwitchGame)
	{
		m_scheduleSwitchGame = false;
		g_pGame->InitGameType(m_switchGameMultiplayer);
	}
}



void CFlashFrontEnd::UpdateResolution(const int width, const int height)
{
	m_currentResolution.x = width;
	m_currentResolution.y = height;
	
	if(!IsMenuLoaded())
		return;

	const float assetWidth = (float)m_pFlashUI->GetWidth();
	const float assetHeight = (float)m_pFlashUI->GetHeight();

	const float renderWidth = (float)width;
	const float renderHeight = (float)height;
	
	if( (renderWidth / renderHeight) > MIN_ASPECT_RATIO )
	{
		const int proposedWidth = int_round( (renderHeight / assetHeight) * assetWidth);
		const int offset = int_round(0.5f * (float)(width - proposedWidth));
		m_pFlashUI->SetViewport(offset, 0, proposedWidth, height);
	}
	else
	{
		const int proposedHeight = int_round( (renderWidth / assetWidth) * assetHeight);
		const int offset = int_round(0.5f * (float)(height - proposedHeight));
		m_pFlashUI->SetViewport(0, offset, width, proposedHeight);
	}
}



void CFlashFrontEnd::LoadBackground()
{
	if(!m_backgroundTexture)
	{
		m_backgroundTexture = gEnv->pRenderer->EF_LoadTexture("Textures/FrontEnd/background.tif", FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
	}
}



void CFlashFrontEnd::HandleFSCommand(const char* pCommand, const char* pArgs, void* pUserData)
{
	if( m_audio.HandleCommand(pCommand) )
	{
		return;
	}
	else if( g_pGame->GetProfileOptions()->IsOption(pCommand) )
	{
		g_pGame->GetProfileOptions()->SetOptionValue(pCommand, pArgs);
	}
	else if( m_mpMenuHub && m_mpMenuHub->HandleCommand(pCommand, pArgs, pUserData) )
	{
		// Handled by MP Menu
	}
	else if( EFlashFrontEndCommand command = GetCommand(pCommand) )
	{
		Execute(command, pArgs);
	}
}



void CFlashFrontEnd::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta)
{
	if(!IsMenuLoaded())
		return;

	if(HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK == eHardwareMouseEvent)
	{
		//Handle double clicks
	}
	else
	{
		SFlashCursorEvent::ECursorState eCursorState = SFlashCursorEvent::eCursorMoved;
		if(HARDWAREMOUSEEVENT_LBUTTONDOWN == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorPressed;
		}
		else if(HARDWAREMOUSEEVENT_LBUTTONUP == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorReleased;
		}

		int x(iX), y(iY);
		m_pFlashUI->ScreenToClient(x,y);
		m_pFlashUI->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
	}
}



bool CFlashFrontEnd::OnInputEvent(const SInputEvent &rInputEvent)
{
	if(gEnv->IsEditor() || gEnv->pSystem->IsDedicated() || rInputEvent.keyId == eKI_SYS_Commit || gEnv->pConsole->GetStatus())
		return false;

	bool bMenuLoaded = IsMenuLoaded();

	if(bMenuLoaded && m_currentMenu==eFlM_ResetScreen)
		return false;

	if (bMenuLoaded)
	{
		// Key repeat handling
		if((rInputEvent.state == eIS_Pressed) || (rInputEvent.state == eIS_Released))
		{
			switch (rInputEvent.keyId)
			{
			case eKI_Up:			// Supported inputs for key repeat
			case eKI_Down:
			case eKI_Left:
			case eKI_Right:
			case eKI_PS3_Up:
			case eKI_PS3_Down:
			case eKI_PS3_Left:
			case eKI_PS3_Right:
			case eKI_XI_DPadUp:
			case eKI_XI_DPadDown:
			case eKI_XI_DPadLeft:
			case eKI_XI_DPadRight:
			case eKI_XI_ThumbLUp:
			case eKI_XI_ThumbLDown:
			case eKI_XI_ThumbLLeft:
			case eKI_XI_ThumbLRight:
			case eKI_XI_ThumbRUp:
			case eKI_XI_ThumbRDown:
			case eKI_XI_ThumbRLeft:
			case eKI_XI_ThumbRRight:
				{
					if (rInputEvent.state == eIS_Released)
					{
						m_repeatEvent.keyId = eKI_Unknown;
					}
					else
					{
						float repeatDelay = KEY_REPEAT_DELAY;
						float now = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();

						m_repeatTimer = now+repeatDelay;
						m_repeatEvent = rInputEvent;
					}
				}
				break;
			default:
				break;
			};
		}
	}

	if(g_pGame->GetWarnings()->IsActive())
	{
		g_pGame->GetWarnings()->OnInputEvent(rInputEvent);
		return true;
	}

	if(!bMenuLoaded)
	{
		if(rInputEvent.state==eIS_Pressed)
		{
			if(IsGameActive())
			{
				if(rInputEvent.keyId == eKI_XI_Start || rInputEvent.keyId == eKI_Escape || rInputEvent.keyId == eKI_PS3_Start)
				{
					FromGame();
					Initialize(eFlM_IngameMenu);
				}
			}
		}
	}
	else
	{
		if(rInputEvent.state==eIS_Pressed)
		{
			if(rInputEvent.deviceId == eDI_XI)
			{
				const char* forwardCmd = "confirm";
				const char* backCmd = "back";
				bool forwardBackSwapped = (gEnv->pInput->GetPlatformFlags() & eIPF_SwapFrontEndForwardAndBack);
				if(forwardBackSwapped)
				{
					forwardCmd = "back";
					backCmd = "confirm";
				}

				if(rInputEvent.keyId == eKI_XI_DPadUp || rInputEvent.keyId == eKI_XI_ThumbLUp || rInputEvent.keyId == eKI_XI_ThumbRUp || rInputEvent.keyId == eKI_PS3_Up)
					m_pFlashUI->Invoke1("navigate", "up");
				else if(rInputEvent.keyId == eKI_XI_DPadDown || rInputEvent.keyId == eKI_XI_ThumbLDown || rInputEvent.keyId == eKI_XI_ThumbRDown || rInputEvent.keyId == eKI_PS3_Down)
					m_pFlashUI->Invoke1("navigate", "down");
				else if(rInputEvent.keyId == eKI_XI_DPadLeft || rInputEvent.keyId == eKI_XI_ThumbLLeft || rInputEvent.keyId == eKI_XI_ThumbRLeft || rInputEvent.keyId == eKI_PS3_Left)
					m_pFlashUI->Invoke1("navigate", "left");
				else if(rInputEvent.keyId == eKI_XI_DPadRight || rInputEvent.keyId == eKI_XI_ThumbLRight || rInputEvent.keyId == eKI_XI_ThumbRRight || rInputEvent.keyId == eKI_PS3_Right)
					m_pFlashUI->Invoke1("navigate", "right");
				else if(rInputEvent.keyId == eKI_XI_A || rInputEvent.keyId == eKI_PS3_Cross)
					m_pFlashUI->Invoke1("navigate", forwardCmd);
				else if(rInputEvent.keyId == eKI_XI_X || rInputEvent.keyId == eKI_PS3_Square)
					m_pFlashUI->Invoke1("navigate", "x");
				else if(rInputEvent.keyId == eKI_XI_Y || rInputEvent.keyId == eKI_PS3_Triangle)
					m_pFlashUI->Invoke1("navigate", "y");
				else if(rInputEvent.keyId == eKI_XI_B || rInputEvent.keyId == eKI_PS3_Circle)
				{
					ScreenBack();
				}
				else if(rInputEvent.keyId == eKI_XI_Start || rInputEvent.keyId == eKI_PS3_Start)
				{
					if(IsGameActive())
					{
						m_scheduleClear = true;
						ToGame();
					}
				}
				else
				{
					return false;
				}
				return true;
			}
			else if(rInputEvent.deviceId == eDI_Keyboard)
			{
				if(rInputEvent.keyId == eKI_Up)
					m_pFlashUI->Invoke1("navigate", "up");
				else if(rInputEvent.keyId == eKI_Down)
					m_pFlashUI->Invoke1("navigate", "down");
				else if(rInputEvent.keyId == eKI_Left)
					m_pFlashUI->Invoke1("navigate", "left");
				else if(rInputEvent.keyId == eKI_Right)
					m_pFlashUI->Invoke1("navigate", "right");
				else if(rInputEvent.keyId == eKI_Enter || rInputEvent.keyId == eKI_NP_Enter)
					m_pFlashUI->Invoke1("navigate", "confirm");
				else if((rInputEvent.keyId == eKI_Escape)||(rInputEvent.keyId == eKI_Backspace))
				{
					ScreenBack();
				}
				else
				{
					return false;
				}
				return true;
			}
		}
	}

	return false;
}



void CFlashFrontEnd::OnLoadingStart(ILevelInfo *pLevel)
{
	if(gEnv->bEditor)
		return;

	if(pLevel)
	{
		string ret("");
		string option(pLevel->GetName());
		option.append(".maxLevelProgress");

		string result = g_pGame->GetProfileOptions()->GetOptionValue(option.c_str());
		if(result.empty())
			m_maxLevelProgress = 500;
		else
			m_maxLevelProgress = atoi(result.c_str());
	}
}



void CFlashFrontEnd::OnLoadingComplete(ILevel *pLevel)
{
	if(gEnv->bEditor)
		return;

	bool keepLoadingScreen = false;

#if defined(XENON) || defined(PS3)
	keepLoadingScreen = !gEnv->bMultiplayer && !m_isReloading;
	//if(ICVar* const sysSpec = gEnv->pConsole->GetCVar("sys_spec"))
	{
		//keepLoadingScreen &= (sysSpec->GetIVal() == 5);
#if !defined(PS3)
		if(ICVar* const autoPrecacheCgf = gEnv->pConsole->GetCVar("e_LevelAutoPrecacheCgf"))
		{
			keepLoadingScreen &= (autoPrecacheCgf->GetIVal()!=0);
		}
#endif; //!PS3
	}
#endif; //XENON || PS3

	if(pLevel)
	{
		ILevelInfo* pLevelInfo = pLevel->GetLevelInfo();
		if(pLevelInfo)
		{
			string ret("");
			string option(pLevelInfo->GetName());
			option.append(".maxLevelProgress");

			g_pGame->GetProfileOptions()->SetOptionValue(option.c_str(), m_maxLevelProgress);
		}
	}

	m_scheduleClear = !keepLoadingScreen;
	m_isReloading = false;
}



void CFlashFrontEnd::OnLoadingError(ILevelInfo *pLevel, const char *error)
{
	if(gEnv->bEditor)
		return;

	Clear();
}



void CFlashFrontEnd::OnLoadingProgress(ILevelInfo *pLevel, int progressAmount)
{
	if(!IsMenuLoaded())
		return; 

	if(m_scheduleClear)
		return;

	bool bStandAlone = (gEnv->pRenderer->EF_Query(EFQ_RecurseLevel) <= 0);
	if (bStandAlone)
		gEnv->pSystem->RenderBegin();

	if(m_maxLevelProgress)
	{
		int progress = int_round(((float)progressAmount / (float)m_maxLevelProgress) * 100.0f);
		m_pFlashUI->Invoke1("setLoadingProgress", progress);
		if(progressAmount>m_maxLevelProgress)
		{
			m_maxLevelProgress = progressAmount;
		}
	}

	OnPostUpdate(0.0f);

	if (bStandAlone)
		gEnv->pSystem->RenderEnd();
}



bool CFlashFrontEnd::OnWarningReturn(THUDWarningId id, const char* returnValue)
{
	if(id == g_pGame->GetWarnings()->GetWarningId("Exit"))
	{
		if(!stricmp(returnValue, "exit"))
		{
			m_scheduleQuit = true;
		}
	}
	else if(id == g_pGame->GetWarnings()->GetWarningId("ExitSession"))
	{
		if(!stricmp(returnValue, "exit"))
		{
			Execute(eFECMD_quit, "");
		}
	}
	else if(id == g_pGame->GetWarnings()->GetWarningId("Suicide"))
	{
		if(!stricmp(returnValue, "suicide"))
		{
			m_scheduleClear = true;
			ToGame();
			gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("kill");
		}
	}
	else if (id == g_pGame->GetWarnings()->GetWarningId("LogIn"))
	{
		if (stricmp(returnValue, "yes")==0)
		{
			// Try again
			Execute(eFECMD_verify_profile, GAME_SELECT_SCREEN);
			return false; // Don't close dialog
		}
		else // No..
		{
			Execute(eFECMD_goto, GAME_SELECT_SCREEN);
		}
	}

	return true;
}



void CFlashFrontEnd::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	if(event == ESYSTEM_EVENT_LEVEL_PRECACHE_START)
	{
		if(!gEnv->bEditor)
		{
			m_scheduleClear = true;
		}
	}
}



void CFlashFrontEnd::Execute(EFlashFrontEndCommand command, const char* param)
{
	switch(command)
	{
	case eFECMD_exit:
		{
			g_pGame->GetWarnings()->AddWarning(g_pGame->GetWarnings()->GetWarningId("Exit"), this);
		}
		break;
	case eFECMD_goto:
		{
			if(param && param[0])
				ScreenForward(param);
		}
		break;
	case eFECMD_back:
		{
			ScreenBack();
		}
		break;
	case eFECMD_load:
		{
			if(param && param[0])
				gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame(string().Format("map %s", param).c_str());
		}
		break;
	case eFECMD_resume:
		{
			m_scheduleClear = true;
			ToGame();
		}
		break;
	case eFECMD_restart:
		{
			m_scheduleClear = true;
			m_scheduleReload = true;
			ToGame();
		}
		break;
	case eFECMD_back_to_mainmenu:
		{
			g_pGame->GetWarnings()->AddWarning(g_pGame->GetWarnings()->GetWarningId("ExitSession"), this);
		}
		break;
	case eFECMD_quit:
		{
			m_scheduleClear = true;
			ToGame();
			gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
		}
		break;
	case eFECMD_suicide:
		{
			g_pGame->GetWarnings()->AddWarning(g_pGame->GetWarnings()->GetWarningId("Suicide"), this);
		}
		break;
	case eFECMD_switch_game:
		{
			if(param && param[0])
			{
				m_scheduleSwitchGame = true;
				m_switchGameMultiplayer = !stricmp(param, "multiplayer");
			}
		}
		break;
	case eFECMD_set_highlight:
		{
			if (param && param[0] && (m_currentScreenIndex >= 0))
				m_menuData.GetScreen(m_currentScreenIndex)->SetCurrentHighlight(atoi(param));
		}
		break;
	case eFECMD_verify_profile:
		{
			// Checks for a profile log-in. On consoles this wants to prompt xmb to log-in.
			// PC - just use default for now..
			CCCPOINT(PlayerProfile_SignIn);
			bool signedIn = false;

#if defined(XENON) || defined(PS3)
			IPlayerProfileManager *pPlayerProfileManager = g_pGame->GetOptions()->GetProfileManager();
			if (pPlayerProfileManager)
			{
				const char* userName = pPlayerProfileManager->GetCurrentUser();
				unsigned int userIndex = g_pGame->GetExclusiveControllerDeviceIndex();
				bool bIsFirstTime = false;

				pPlayerProfileManager->LogoutUser(userName);
				pPlayerProfileManager->LoginUser(userName, bIsFirstTime, userIndex); // Triggers console sign-in box

				IPlatformOS* os = GetISystem()->GetPlatformOS();
				signedIn = os->UserIsSignedIn(userIndex);

				if (signedIn)
				{
					// If profile has changed, update profile options
					if (g_pGame->GetProfileOptions())
					{
						g_pGame->GetProfileOptions()->UpdatePlayerProfile(pPlayerProfileManager->GetCurrentProfile(userName));
					}
				}
			}
#else
			// PC should be signed into a default profile
			signedIn = false;
#endif

			if (signedIn)
			{
				CCCPOINT(PlayerProfile_Verified);
				Execute(CFlashFrontEnd::eFECMD_goto, GAME_SELECT_SCREEN);
			}
		}
		break;
	case eFECMD_command:
		{
			gEnv->pConsole->ExecuteString(param);
		}
		break;
	}
}



void CFlashFrontEnd::ScreenForward(const char* name)
{
	CCCPOINT(FrontEnd_ScreenForward);
	assert(name);

	const int index = m_menuData.GetScreen(name);
	if(index != -1)
	{
		if(m_screenStack.size()>=1)
		{
			const int last = m_screenStack[m_screenStack.size()-1];
			if(last != -1)
			{
				const CMenuScreen *screen = m_menuData.GetScreen(last);
				if (screen && !screen->ShouldKeepInStack())
				{
					m_screenStack.pop_back(); // Don't remember screen to go back to
				}
			}
		}

		m_screenStack.push_back(index);
		SetScreen(index);
	}
}



void CFlashFrontEnd::ScreenBack()
{
	if(m_screenStack.size()<=1)
	{
		if(IsGameActive())
		{
			m_scheduleClear = true;
			ToGame();
		}
		return;
	}

	m_screenStack.pop_back();

	const int last = m_screenStack[m_screenStack.size()-1];
	if(last != -1)
	{
		SetScreen(last);
	}
}



void CFlashFrontEnd::SetScreen(const int index)
{
	if(!IsMenuLoaded())
		return;

	ClearScreen();
	
	m_currentScreenIndex = index;

	CMenuScreen *screen = m_menuData.GetScreen(index);
	screen->Announce(m_pFlashUI);

// #if defined(PS3) || defined(XENON)
	SFlashVarValue args[2] = { -1, screen->GetCurrentHighlight() };
	m_pFlashUI->Invoke("highlightButton", args, 2);
// #endif

	m_pFlashUI->Advance(0.f); // To load the subscreen

	EnterScreen(screen->GetScreenType());
}



void CFlashFrontEnd::ClearScreen()
{
	if(!IsMenuLoaded())
		return;

	if (m_currentScreenIndex>0)
	{
		LeaveScreen(m_menuData.GetScreen(m_currentScreenIndex)->GetScreenType());
		m_currentScreenIndex = -1;
	}
	
	m_pFlashUI->Invoke0("clearButtons");
}

void CFlashFrontEnd::EnterScreen(const EFlashFrontEndScreens screen)
{
	m_currentScreen = screen;

	switch(screen)
	{
		case eFFES_main:
			{
				if (m_currentMenu==eFlM_GameSelect)
				{
					// Any controller should be able to press start on this screen and become the exclusive controller
					g_pGame->RemoveExclusiveController();
				}
			}
		break;
		case eFFES_profile_login:
			{
				if (m_currentMenu==eFlM_GameSelect)
				{
					// Set which ever controller pressed start to be the exclusive controller
					g_pGame->SetExclusiveControllerFromPreviousInput();

#if defined(XENON) || defined(PS3)
					IPlatformOS* os = GetISystem()->GetPlatformOS();
					unsigned int userIndex = g_pGame->GetExclusiveControllerDeviceIndex();
					if (!os->UserIsSignedIn(userIndex))
					{
						g_pGame->GetWarnings()->AddWarning("LogIn", NULL, this);
						break;
					}
#endif
					Execute(eFECMD_goto, GAME_SELECT_SCREEN);
				}
			}
			break;
	};

	if (m_mpMenuHub)
		m_mpMenuHub->EnterScreen(m_currentMenu, screen);
}

void CFlashFrontEnd::LeaveScreen(const EFlashFrontEndScreens screen)
{
	switch(screen)
	{
	case eFFES_profile_login:
		{
			if (m_currentMenu==eFlM_GameSelect)
			{
				g_pGame->GetWarnings()->RemoveWarning("LogIn");
			}
		}
		break;
	};

	if (m_mpMenuHub)
		m_mpMenuHub->LeaveScreen(m_currentMenu, screen);

	m_currentScreen = eFFES_unknown;
}


bool CFlashFrontEnd::IsGameActive() const
{
	return g_pGame->GetIGameFramework()->StartedGameContext() && (gEnv->bHostMigrating || g_pGame->GetIGameFramework()->GetClientChannel());
}



void CFlashFrontEnd::FromGame()
{
	IActionMapManager *pActionMapManager = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	if(!pActionMapManager)
		return;

	g_pGameActions->FilterIngameMenu()->Enable(true);

	pActionMapManager->ReleaseFilteredActions();

	if(!gEnv->bMultiplayer)
		g_pGame->GetIGameFramework()->PauseGame(true, false);

	SHUDEvent hudEvent(eHUDEvent_OpenedIngameMenu);
	CHUD::CallEvent(hudEvent);
	
	g_pGame->GetHUD()->ActivateState("no_hud");
}



void CFlashFrontEnd::ToGame()
{
	IActionMapManager *pActionMapManager = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	if(!pActionMapManager)
		return;

	g_pGameActions->FilterIngameMenu()->Enable(false);

	if(!gEnv->bMultiplayer)
		g_pGame->GetIGameFramework()->PauseGame(false, false);

	g_pGame->GetHUD()->ActivateDefaultState();

	SHUDEvent hudEvent(eHUDEvent_ClosedIngameMenu);
	CHUD::CallEvent(hudEvent);
}



void CFlashFrontEnd::RegisterCommand(EFlashFrontEndCommand command, const char *name)
{
	m_commandMap.insert(TCommandMap::value_type(CryStringUtils::CalculateHash(name), command));
}



CFlashFrontEnd::EFlashFrontEndCommand CFlashFrontEnd::GetCommand(const char *name)
{
	uint32 hash = CryStringUtils::CalculateHash(name);
	TCommandMap::iterator it = m_commandMap.find(hash);
	if (it != m_commandMap.end())
		return it->second;
	else
		return eFECMD_none;
}


void CFlashFrontEnd::OnActionEvent(const SActionEvent& event)
{
	switch(event.m_event)
	{
	case eAE_connectFailed:
		if (m_mpMenuHub)
		{
			m_mpMenuHub->ConnectionFailed();
		}
		break;
	case eAE_disconnected:
		if (m_mpMenuHub)
		{
			m_mpMenuHub->Disconnected(event.m_value);
		}
		break;
	case eAE_channelCreated:
		break;
	case eAE_resetBegin:
		ResetBegin();
		break;
	case eAE_resetEnd:
		ResetEnd();
		break;
	case eAE_resetProgress:
		ResetProgress(event.m_value);
		break;
	case eAE_inGame:
		break;
	case eAE_earlyPreUpdate:
		break;
	case eAE_mapCmdIssued:
		ShowLoadingScreen();
		break;
	}
}



void CFlashFrontEnd::OnLevelEnd(const char* pNextLevel)
{
	if(!gEnv->bEditor)
	{
		if(!pNextLevel || !pNextLevel[0])
			Execute(eFECMD_quit,"");
	}
}



void CFlashFrontEnd::ShowLoadingScreen()
{
	if(gEnv->bEditor)
		return;

	Clear();
	Initialize(eFlM_LoadingScreen);
	//OnLoadingProgress(NULL, 0); //Diesel cut

	if(ICVar* pCVar = gEnv->pConsole->GetCVar("hud_hide"))
	{
		if(pCVar->GetIVal()!=1)
		{
			pCVar->Set(1);
			m_hudHidePS3HackErmWorkaround = true;
		}
	}
}



void CFlashFrontEnd::ResetBegin()
{
	Clear();
	Initialize(eFlM_ResetScreen);
	g_pGame->GetHUD()->ActivateState("no_hud"); // hide the hud
}

void CFlashFrontEnd::ResetEnd()
{
	Clear();
	g_pGame->GetHUD()->ActivateDefaultState(); // show the hud
}

void CFlashFrontEnd::ResetProgress(int progress)
{
	if (IsMenuActive(eFlM_ResetScreen) && m_pFlashUI)
	{
		m_pFlashUI->Invoke1("SetProgress", float(progress));
	}
}

#if !defined(_RELEASE)
void CFlashFrontEnd::DebugWatches()
{
	if(g_pGame->GetHUD()->GetCVars()->menu_debugScreenStack)
	{
		TScreenStack::const_iterator it = m_screenStack.begin();
		TScreenStack::const_iterator itEnd = m_screenStack.end();
		
		CryWatch("Menu Screen Stack size %d", m_screenStack.size());
		for(int i=0;it!=itEnd;++it)
		{
			const CMenuScreen *screen = (*it >= 0) ? m_menuData.GetScreen(*it) : NULL;
			CryWatch("  %d. %d - %s", i, *it, screen?screen->GetName():"null");

			i++;
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
