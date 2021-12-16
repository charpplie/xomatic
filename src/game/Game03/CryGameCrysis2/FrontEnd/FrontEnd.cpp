#include "StdAfx.h"
#include <StlUtils.h>

#include <time.h>

#include "FrontEndElement.h"
#include "FrontEndScreen.h"

#include "IGameFramework.h"
#include "IMusicSystem.h"
#include "ISound.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"
#include <ISaveGame.h>
#include <ILoadGame.h>
#include <StringUtils.h>
#include "IViewSystem.h"
#include "IActionMapManager.h"
#include "ProfileOptions.h"
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "Menus/FlashMenuObject.h"

#include "FrontEnd.h"

#include "Graphics/2DRenderUtils.h"
#include "FrontEnd/WarningsManager.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//////////////////////////////////////////////////////////////////////////


//first element of array is default values
#define KEY_FROM_VALUE(v,g) GetKeyByValue(v,g,sizeof(g)/sizeof(g[0]))

CFrontEnd *CFrontEnd::m_pInst = NULL;

static TFrontEndKeyValuePair<EFrontEndCommand,string>
g_commandList[] = {
	{eFECMD_none,""},
	{eFECMD_goto,"Goto"},
	{eFECMD_goto_options,"GotoOptions"},
	{eFECMD_goto_exit,"GotoExit"},
	{eFECMD_release_and_goto_main,"ReleaseAndGotoMain"},
	{eFECMD_new_game,"NewGame"},
	{eFECMD_continue,"Continue"},
	{eFECMD_restart,"Restart"},
	{eFECMD_quit,"Quit"},
	{eFECMD_returnToMain,"ReturnToMain"},
	{eFECMD_exit,"Exit"},
	{eFECMD_screen_back,"ScreenBack"},
	{eFECMD_load,"Load"}
};


//////////////////////////////////////////////////////////////////////////


CFrontEnd::CFrontEnd() :
m_currentScreenIndex(0),
m_currentXML(""),
m_isActive(false),
m_maxLevelProgress(500),
m_curLevelProgress(0),
m_clearOnUpdate(false),
m_initializeOnUpdate(false),
m_reloadOnUpdate(false),
m_soundListener(LISTENERID_INVALID),
m_debugFrontEnd(false),
m_nextLoadLevel(""),
m_postLoadingProgress(false),
m_reloading(false)
{

//	SAFE_HARDWARE_MOUSE_FUNC(AddListener(this));
//	if (gEnv->pInput) gEnv->pInput->AddEventListener(this);
//	gEnv->pGame->GetIGameFramework()->RegisterListener(this, "frontend", FRAMEWORKLISTENERPRIORITY_MENU);
//	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);
//	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

	m_screens.resize(0);

	m_options = new CProfileOptions();

	// this maybe should come from an .xml
	const char* audioSignalNames[ eFES_Last ] =
	{
		"FrontEnd_Start",
		"FrontEnd_Confirm",
		"FrontEnd_Cancel",
		"FrontEnd_Move",
		"FrontEnd_Ambient"
	};

	for(int i=0; i<eFES_Last; ++i)
		m_soundPlayers[i].SetSignal( audioSignalNames[i] );
}



CFrontEnd::~CFrontEnd()
{
//	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
//	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
//	gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
//	if (gEnv->pInput) gEnv->pInput->RemoveEventListener(this);
//	SAFE_HARDWARE_MOUSE_FUNC(RemoveListener(this));

	SAFE_DELETE(m_options);

	Clear();
}


void CFrontEnd::Initialize(void)
{
	InitializeFromFile(NULL);
}

void CFrontEnd::InitializeAsset(const char* const path, const char* const asset )
{
	CryFixedStringT<128> fullName( path );
	fullName += "/";
	if (g_pGameCVars->g_multiplayerDefault)
	{
		fullName += "CW2";
	}
	fullName += asset;
	InitializeFromFile( fullName );
}

void CFrontEnd::InitializeFromFile(const char* path)
{
	if(gEnv->bEditor)
		return;

	if(IsActive())
		Clear();

	if(!path)
	{
		if(!m_currentXML.empty())
			path = m_currentXML.c_str();
		else
		{
			CryLog("[FrontEnd]Error: FrontEnd initialization without XML!");
			return;
		}
	}
	XmlNodeRef frontEnd = GetISystem()->LoadXmlFile(path);
	if(!frontEnd)
	{
		CryLog("[FrontEnd]Error: '%s' could not be loaded for initialization!",path);
		return;
	}

	m_currentXML = path;
	m_currentScreenIndex = 0;

	for(int i = 0; i < frontEnd->getChildCount(); ++i)
	{

		XmlNodeRef screen = frontEnd->getChild(i);

		if(!stricmp(screen->getTag(),"Screen"))
		{
			CFrontEndScreen* pScreen = new CFrontEndScreen();
			pScreen->Initialize(screen);
			m_screens.push_back(pScreen);
		}
		else if(!stricmp(screen->getTag(),"ScreenStart"))
		{
			CFrontEndScreenStart* pScreen = new CFrontEndScreenStart();
			pScreen->Initialize(screen);
			m_screens.push_back(pScreen);
		}
		else if(!stricmp(screen->getTag(),"ScreenLoading"))
		{
			CFrontEndScreenLoading* pScreen = new CFrontEndScreenLoading();
			pScreen->Initialize(screen);
			m_screens.push_back(pScreen);
		}
	}

	CCCPOINT(FrontEnd_ReadFromFile);

	if(CFrontEndScreen* pCurrent = GetCurrentScreen())
	{
		ScreenForward(pCurrent->GetName());
	}
	SetFont("default");

	if(!IsGameActive() && m_soundListener==LISTENERID_INVALID)
	{
		m_soundListener = gEnv->pSoundSystem->CreateListener();
		IListener *pListener = gEnv->pSoundSystem->GetListener(m_soundListener);
		pListener->SetRecordLevel(1.0f);
		pListener->SetActive(true);
	}

	PlaySound(eFES_Ambient,true);

	IMusicSystem* pMusicSystem = gEnv->pMusicSystem;
	if(pMusicSystem)
	{
		pMusicSystem->LoadFromXML("music/cxp.xml",true, false);
		pMusicSystem->SerializeInternal(true);
		pMusicSystem->EndTheme(EThemeFade_FadeOut, 0, true);
		pMusicSystem->SetTheme("menu", true, false);

		if(IsGameActive())
			pMusicSystem->SetDefaultMood("pause_menu");
		else
			pMusicSystem->SetDefaultMood("main_menu");
	}

	m_options->InitializeFromCVar();
	SetActive(true);
}



void CFrontEnd::Clear()
{
	if(!IsActive())
		return;

	SetActive(false);

	m_postLoadingProgress = false;

	IMusicSystem* pMusicSystem = gEnv->pMusicSystem;
	if(pMusicSystem)
	{
		pMusicSystem->EndTheme(EThemeFade_FadeOut, 0, true);
		pMusicSystem->SerializeInternal(false);
	}

	PlaySound(eFES_Ambient,false);

	m_screenStack.resize(0);
	while(m_screens.size()>0)
	{
		CFrontEndScreen* pScreen = m_screens.back(); m_screens.pop_back();
		SAFE_DELETE(pScreen);
	}
	m_options->SaveProfile();
}



bool CFrontEnd::IsGameActive()
{
	CGameRules* pGameRules = g_pGame->GetGameRules();
	return pGameRules!=NULL;
}



void CFrontEnd::FromGame()
{
	IActionMapManager *pActionMapManager = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	if(!pActionMapManager)
		return;

	g_pGameActions->FilterIngameMenu()->Enable(true);

	pActionMapManager->ReleaseFilteredActions();

	if(!gEnv->bMultiplayer)
		g_pGame->GetIGameFramework()->PauseGame(true, false);

	g_pGame->GetHUD()->ActivateState("no_hud");
}



void CFrontEnd::ToGame()
{
	IActionMapManager *pActionMapManager = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	if(!pActionMapManager)
		return;

	g_pGameActions->FilterIngameMenu()->Enable(false);

	if(!gEnv->bMultiplayer)
		g_pGame->GetIGameFramework()->PauseGame(false, false);

	g_pGame->GetHUD()->ActivateDefaultState();
}



CFrontEndScreen* CFrontEnd::GetCurrentScreen()
{
	int size = m_screens.size();
	if(!size)
		return NULL;

	if(m_currentScreenIndex<0 || m_currentScreenIndex>=size)
		return NULL;

	return m_screens[m_currentScreenIndex];
}



void CFrontEnd::SetCurrentScreen(int index, float updateFrameTime)
{
	m_currentScreenIndex = index;
	CFrontEndScreen* pCurrent = GetCurrentScreen();
	if(pCurrent)
		pCurrent->Update(updateFrameTime);
}



void CFrontEnd::ScreenForward(const char* name, bool releaseOld)
{
	CCCPOINT (FrontEnd_ScreenForward);

	assert(name);

	if(releaseOld)
	{
		m_screenStack.pop_back();
	}

	std::vector<CFrontEndScreen*>::const_iterator it = m_screens.begin();
	std::vector<CFrontEndScreen*>::const_iterator end = m_screens.end();
	int index = 0;
	for(; it!=end; ++it, ++index)
	{
		if(!stricmp((*it)->GetName(),name))
		{
			m_screenStack.push_back(string(name));
			CFrontEndScreen* pCurrent = GetCurrentScreen();
			if(pCurrent)
			{
				pCurrent->Fade(true, index);
			}
			else
			{
				SetCurrentScreen(index);
			}
			(*it)->Fade(false);
			return;
		}
	}
}



void CFrontEnd::ScreenBack()
{
	if(m_screenStack.size()<=1)
		return;

	m_screenStack.pop_back();

	const char* last = m_screenStack[m_screenStack.size()-1];

	std::vector<CFrontEndScreen*>::const_iterator it = m_screens.begin();
	std::vector<CFrontEndScreen*>::const_iterator end = m_screens.end();
	int index = 0;
	for(; it!=end; ++it, ++index)
	{
		if(!stricmp((*it)->GetName(),last))
		{
			PlaySound(eFES_Cancel,true);
			CFrontEndScreen* pCurrent = GetCurrentScreen();
			if(pCurrent)
			{
				pCurrent->Fade(true, index);
			}
			else
			{
				SetCurrentScreen(index);
			}
			(*it)->Fade(false);
			return;
		}
	}
}



void CFrontEnd::DrawTexture(ITexture* texture, float posX, float posY, float width, float height, float opacity, float u0, float w0, float u1, float w1)
{
	assert(texture);

	if(width==-1)
		width = (float)texture->GetWidth();

	if(height==-1)
		height = (float)texture->GetHeight();

	float renderW = (float)gEnv->pRenderer->GetWidth();
	float renderH = (float)gEnv->pRenderer->GetHeight();

	float xRatio = 800.0f / renderW;
	float yRatio = 600.0f / renderH;

	posX *= xRatio;
	posY *= yRatio;
	width *= xRatio;
	height *= yRatio;

	float supposedRenderW = renderW*xRatio;

	if(supposedRenderW!=800.0f)
	{
		posX += (800.0f - supposedRenderW) * 0.5f;
	}

	gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
	gEnv->pRenderer->Draw2dImage(posX, posY, width, height, texture->GetTextureID(), u0, w0, u1, w1, 0.0f, 1.0f, 1.0f, 1.0f, opacity);
}



void CFrontEnd::DrawLine(float x1, float y1, float x2, float y2)
{
	float renderW = (float)gEnv->pRenderer->GetWidth();
	float renderH = (float)gEnv->pRenderer->GetHeight();

	float xRatio = 800.0f / renderW;
	float yRatio = 600.0f / renderH;

	g_pGame->GetHUD()->Get2DRenderUtils()->DrawLine(x1*xRatio, y1*yRatio, x2*xRatio, y2*yRatio, ColorF(1.0f));
}



void CFrontEnd::SetFont(const char* font)
{
	if(!font || !font[0])
		return;

	m_font = gEnv->pCryFont->GetFont(font);
	if(!m_font)
	{
		m_font = gEnv->pCryFont->NewFont(font);
		string load("Fonts\\");
		load.append(font);
		load.append(".xml");
		m_font->Load(load.c_str());
	}

	if(!m_font)
	{
		CryLog("[FrontEnd]Warning: font '%s' not found!",font);
		m_font = gEnv->pCryFont->GetFont("default");
	}
}



void CFrontEnd::DrawFont(const char* text, float posX, float posY, float widthspan, float heightspan, int alignment, float fontSize, ColorF color)
{
	posY += (heightspan*0.5f);
	EUIDRAWHORIZONTAL align = UIDRAWHORIZONTAL_CENTER;

	if(alignment==eFEA_Left)
	{
		align = UIDRAWHORIZONTAL_LEFT;
	}
	else if(alignment==eFEA_Right)
	{
		align = UIDRAWHORIZONTAL_RIGHT;
		posX += widthspan;
	}
	else if(alignment==eFEA_Center)
	{
		align = UIDRAWHORIZONTAL_CENTER;
		posX += (widthspan*0.5f);
	}

	float renderW = (float)gEnv->pRenderer->GetWidth();
	float renderH = (float)gEnv->pRenderer->GetHeight();

	float xRatio = 800.0f / renderW;
	float yRatio = 600.0f / renderH;


	posX *= xRatio;
	posY *= yRatio;
	widthspan *= xRatio;
	heightspan *= yRatio;

	g_pGame->GetHUD()->Get2DRenderUtils()->SetFont(m_font);
	g_pGame->GetHUD()->Get2DRenderUtils()->DrawText(posX, posY, widthspan, heightspan, text, color, align, UIDRAWVERTICAL_CENTER);
}



bool CFrontEnd::Execute(string command, const char* param)
{
	EFrontEndCommand c = KEY_FROM_VALUE(command,g_commandList);
	if(c!=eFECMD_none)
	{
		return Execute(c, param);
	}
	else if(m_options->IsOption(command.c_str()))
	{
		m_options->SetOptionValue(command.c_str(), param);
		return true;
	}
	else
	{
		CryLog("Unknown UI command: '%s'",command.c_str());
		return false;
	}

}



bool CFrontEnd::Execute(EFrontEndCommand command, const char* param)
{
	bool reset = false;
	switch(command)
	{
	case eFECMD_exit:
		{
			m_options->SaveProfile();
			gEnv->pSystem->Quit();
		}
		break;
	case eFECMD_screen_back:
		{
			ScreenBack();
			reset = true;
		}
		break;
	case eFECMD_goto:
		{
			if(param && param[0])
				ScreenForward(param);
			reset = true;
		}
		break;
	case eFECMD_goto_options:
		{
			ScreenForward("Options");
			reset = true;
		}
		break;
	case eFECMD_goto_exit:
		{
			ScreenForward("Exit");
			reset = true;
		}
		break;
	case eFECMD_release_and_goto_main:
		{
			ScreenForward("Main", true);
			reset = true;
		}
		break;
	case eFECMD_new_game:
		{
			gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame(string().Format("map %s", m_nextLoadLevel.c_str()).c_str());
			reset = true;
		}
		break;
	case eFECMD_load:
		{
			m_nextLoadLevel = param;
			if(!m_nextLoadLevel.empty())
				ScreenForward("Loading");
			reset = true;
		}
		break;
	case eFECMD_continue:
		{
			m_clearOnUpdate = true;
			ToGame();
			reset = true;
		}
		break;
	case eFECMD_restart:
		{
			m_reloadOnUpdate = true;
			ToGame();
			reset = true;
		}
		break;
	case eFECMD_quit:
		{
			m_clearOnUpdate = true;
			m_initializeOnUpdate = true;
			ToGame();
			gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
			reset = true;
		}
		break;
	case eFECMD_returnToMain:
		{
			CCCPOINT(FrontEnd_ReturnToMain);
			m_initializeOnUpdate = true;
			reset = true;
		}
		break;
	default:
		break;
	}

	return reset;
}



string CFrontEnd::GetCurrentValue(string command)
{
	if(m_options->IsOption(command.c_str()))
	{
		return m_options->GetOptionValue(command.c_str());
	}
	else
	{
		CryLog("[FrontEnd]Warning: Invalid option value request '%s'!",command.c_str());
		return "0";
	}
}



void CFrontEnd::PlaySound(EFrontEndSound eSound,bool bPlay)
{
	assert( eSound<eFES_Last );

	m_soundPlayers[eSound].Stop();

	if (bPlay)
		m_soundPlayers[eSound].Play();
}


// IGameFrameworkListener
void CFrontEnd::OnPostUpdate(float frameTime)
{
	if(gEnv->bEditor)
		return;

#if ENABLE_HUD_EXTRA_DEBUG
	if( g_pGame->GetHUD()->GetCVars()->hud_hideMenus > 0 )
		return;
#endif

	if(m_clearOnUpdate)
	{
		Clear();
		m_clearOnUpdate = false;
		return;
	}

	if(m_initializeOnUpdate)
	{
		InitializeAsset("Scripts/FrontEnd", "FrontEnd.xml");
		m_initializeOnUpdate = false;
		return;
	}

	if(m_reloadOnUpdate)
	{
		m_reloadOnUpdate = false;
		m_reloading = true;
		g_pGame->InitMapReloading();
		return;
	}

	if(!g_pGame->GetIGameFramework()->StartedGameContext() && !g_pGame->GetIGameFramework()->GetClientChannel())
	{
		const char * frontend;
		if (g_pGameCVars->g_multiplayerDefault)
		{
			frontend = "Scripts/FrontEnd/CW2FrontEnd.xml";
		}
		else
		{
			frontend = "Scripts/FrontEnd/FrontEnd.xml";
		}

		if (m_currentXML.compare(frontend))
		{
			g_pGame->GetHUD()->Unload(); // Unload the Hud (if it isn't already)

			m_initializeOnUpdate = true;
			return;
		}
	}

	CFrontEndScreen* pCurrent = GetCurrentScreen();
	if(!pCurrent)
		return;

	pCurrent->Update(m_postLoadingProgress ? 1.0f : frameTime);

	pCurrent = GetCurrentScreen();
	if(!pCurrent)
		return;

	g_pGame->GetHUD()->Get2DRenderUtils()->PreRender();
	pCurrent->Render();
	g_pGame->GetHUD()->Get2DRenderUtils()->PostRender();
	g_pGame->GetWarnings()->Update(frameTime);
}



void CFrontEnd::OnSaveGame(ISaveGame* pSaveGame)
{
}



void CFrontEnd::OnLoadGame(ILoadGame* pLoadGame)
{
}



void CFrontEnd::OnLevelEnd(const char *pNextLevel)
{
	if(!gEnv->bEditor)
	{
		if(!pNextLevel || !pNextLevel[0])
			Execute(eFECMD_quit,"");
	}
}



void CFrontEnd::OnActionEvent(const SActionEvent& event)
{
}
// ~IGameFrameworkListener



// IHardwareMouseEventListener
void CFrontEnd::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta)
{
}
// ~IHardwareMouseEventListener



// IInputEventListener
bool CFrontEnd::OnInputEvent(const SInputEvent &rInputEvent)
{
	if (g_pGameCVars->g_multiplayerDefault)
	{
		return false;
	}

	if(gEnv->bEditor)
		return false;

	EFrontEndInputState state;
	if(rInputEvent.state==eIS_Pressed)
		state = eFEIS_Press;
	else if(rInputEvent.state==eIS_Released)
		state = eFEIS_Release;
	else
		return false;

	bool handled = false;


	if(IsActive())
	{
		CFrontEndScreen* pCurrent = GetCurrentScreen();
		if(pCurrent)
		{
			if(rInputEvent.deviceId == eDI_XI)
			{
				if(rInputEvent.keyId == eKI_XI_DPadUp || rInputEvent.keyId == eKI_XI_ThumbLUp || rInputEvent.keyId == eKI_XI_ThumbRUp || rInputEvent.keyId == eKI_PS3_Up)
					handled = pCurrent->Input(eFEIT_Up, state);
				else if(rInputEvent.keyId == eKI_XI_DPadDown || rInputEvent.keyId == eKI_XI_ThumbLDown || rInputEvent.keyId == eKI_XI_ThumbRDown || rInputEvent.keyId == eKI_PS3_Down)
					handled = pCurrent->Input(eFEIT_Down, state);
				else if(rInputEvent.keyId == eKI_XI_DPadLeft || rInputEvent.keyId == eKI_XI_ThumbLLeft || rInputEvent.keyId == eKI_XI_ThumbRLeft || rInputEvent.keyId == eKI_PS3_Left)
					handled = pCurrent->Input(eFEIT_Left, state);
				else if(rInputEvent.keyId == eKI_XI_DPadRight || rInputEvent.keyId == eKI_XI_ThumbLRight || rInputEvent.keyId == eKI_XI_ThumbRRight || rInputEvent.keyId == eKI_PS3_Right)
					handled = pCurrent->Input(eFEIT_Right, state);
				else if(rInputEvent.keyId == eKI_XI_A || rInputEvent.keyId == eKI_PS3_Cross)
					handled = pCurrent->Input(eFEIT_Ok, state);
				else if(rInputEvent.keyId == eKI_XI_B || rInputEvent.keyId == eKI_PS3_Circle)
				{
					ScreenBack();
					handled = true;
				}
				else if((rInputEvent.keyId == eKI_XI_Start || rInputEvent.keyId == eKI_PS3_Start) && rInputEvent.state==eIS_Pressed)
				{
					if(IsGameActive())
					{
						ToGame();
						Clear();
						handled = true;
					}
					else
					{
						handled = pCurrent->Input(eFEIT_Start, state);
					}
				}

			}
			else if(rInputEvent.deviceId == eDI_Keyboard)
			{
				if(rInputEvent.keyId == eKI_Up)
					handled = pCurrent->Input(eFEIT_Up, state);
				else if(rInputEvent.keyId == eKI_Down)
					handled = pCurrent->Input(eFEIT_Down, state);
				else if(rInputEvent.keyId == eKI_Left)
					handled = pCurrent->Input(eFEIT_Left, state);
				else if(rInputEvent.keyId == eKI_Right)
					handled = pCurrent->Input(eFEIT_Right, state);
				else if(rInputEvent.keyId == eKI_Enter || rInputEvent.keyId == eKI_NP_Enter)
					handled = pCurrent->Input(eFEIT_Ok, state);
				else if(rInputEvent.keyId == eKI_Space)
					handled = pCurrent->Input(eFEIT_Start, state);
				else if(rInputEvent.keyId == eKI_Escape && rInputEvent.state==eIS_Pressed)
				{
					if(IsGameActive() && m_screenStack.size()<=1)
					{
						ToGame();
						Clear();
					}
					else
					{
						ScreenBack();
					}
					handled = true;
				}
			}
		}
	}
	else
	{
		if(IsGameActive())
		{
			if((rInputEvent.deviceId == eDI_XI && (rInputEvent.keyId == eKI_XI_Start || rInputEvent.keyId == eKI_PS3_Start) && rInputEvent.state==eIS_Pressed)
				|| (rInputEvent.deviceId == eDI_Keyboard && rInputEvent.keyId == eKI_Escape && rInputEvent.state==eIS_Pressed))
			{
				FromGame();
				InitializeAsset("Scripts/FrontEnd", "IngameFrontEnd.xml");
			}			
		}
	}
	return handled;
}



bool CFrontEnd::OnInputEventUI( const SInputEvent &rInputEvent )
{
	return false;
}
// ~IInputEventListener



// ILevelSystemListener
void CFrontEnd::OnLevelNotFound(const char *levelName)
{
	if(!gEnv->bEditor)
		Initialize();
}



void CFrontEnd::OnLoadingStart(ILevelInfo *pLevel)
{
	CCCPOINT (FrontEnd_OnLoadingStart);

	if(!gEnv->bEditor)
	{
		Clear();

		if(g_pGameCVars->hud_UseFlashFrontEndLoading==0)
		{
			InitializeAsset("Scripts/FrontEnd", "Loading.xml");
		}

		if(pLevel)
		{
			string ret("");
			string option(pLevel->GetName());
			option.append(".maxLevelProgress");

			string result = m_options->GetOptionValue(option.c_str());
			if(result.empty())
				m_maxLevelProgress = 2000;
			else
				m_maxLevelProgress = atoi(result.c_str());
		}

		if(m_soundListener!=LISTENERID_INVALID)
		{
			gEnv->pSoundSystem->RemoveListener(m_soundListener);
			m_soundListener = LISTENERID_INVALID;
		}
	}
}



void CFrontEnd::OnLoadingComplete(ILevel *pLevel)
{
	CCCPOINT (FrontEnd_OnLoadingComplete);

	if(!gEnv->bEditor)
	{
		if(pLevel && pLevel->GetLevelInfo())
		{
			string ret("");
			string option(pLevel->GetLevelInfo()->GetName());
			option.append(".maxLevelProgress");
			m_options->SetOptionValue(option.c_str(),m_curLevelProgress);

			m_postLoadingProgress = true;
			if ( gEnv->bMultiplayer == true )
			{
				Clear();
			}
			else
			{
				//MS1 HACK
				if(m_reloading)
				{
					Clear();
					m_reloading = false;
				}
				else
				{
#if defined(XENON) || defined(PS3)
					ICVar* const sysSpec = gEnv->pConsole->GetCVar("sys_spec");
					if( sysSpec && sysSpec->GetIVal() == 5 )
					{
						ICVar* const autoPrecacheCgf = gEnv->pConsole->GetCVar("e_LevelAutoPrecacheCgf");
#if !defined(PS3)
						if(autoPrecacheCgf->GetIVal()==0)
						{
							Clear();
						}
#endif
					}
					else 
					{
						Clear();
					}
#else
					Clear();
#endif
				}
			}
		}
	}
}



void CFrontEnd::OnLoadingError(ILevelInfo *pLevel, const char *error)
{
	if(!gEnv->bEditor)
		Initialize();

}



void CFrontEnd::OnLoadingProgress(ILevelInfo *pLevel, int progressAmount)
{
	if(!gEnv->bEditor)
	{

		m_curLevelProgress = progressAmount;

		if(g_pGameCVars->hud_UseFlashFrontEndLoading)
			return;

		if(IsActive())
		{
			const bool bStandAlone = reinterpret_cast<int>(gEnv->pRenderer->EF_Query(EFQ_RecurseLevel)) <= 0;
			if (bStandAlone)
				gEnv->pSystem->RenderBegin();
			OnPostUpdate(m_maxLevelProgress ? (float)progressAmount / (float)m_maxLevelProgress : 0.f);
			if (bStandAlone)
				gEnv->pSystem->RenderEnd();
		}
	}
}
// ~ILevelSystemListener



// ISystemEventListener
//MS1 HACK
void CFrontEnd::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	if(event == ESYSTEM_EVENT_LEVEL_PRECACHE_START)
	{
		if(!gEnv->bEditor)
		{
			Clear();
		}
	}
}
// ~ISystemEventListener


void CFrontEnd::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}


#undef KEY_FROM_VALUE

