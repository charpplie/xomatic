#include "StdAfx.h"
#include "FlashMainMenu.h"

#include "GameCVars.h"
#include "IGameFramework.h"
#include "ILevelSystem.h"
#include "FlashHelper.h"
#include "Game.h"
#include "HUD/BitmapUI.h"

const char* FLASH_MAINMENU_FILENAME = "Libs\\UI\\FlashMainMenu.gfx";

#define ADD_LIST_GROUP(list, name) { SFlashVarValue args[6] = {"default" #name, name, "ListGroup", "default", 0, 1}; \
									 m_pFlashPlayer->Invoke(#list ".CreateItem", args, 6); }

#define ADD_LIST_ITEM(list, name, command) { SFlashVarValue args[6] = {name, name, "ListItem", command, 0, 1}; \
	m_pFlashPlayer->Invoke(#list ".CreateItem", args, 6); }

#define ADD_LIST_CHECKBOX(list, name, cvar) { SFlashVarValue args[6] = {cvar, name, "ListCheckbox", "cvar_change_b", 0, 1}; \
	m_pFlashPlayer->Invoke(#list ".CreateItem", args, 6); m_RegisteredCVars.push_back(cvar); }

#define ADD_LIST_SLIDER(list, name, cvar, min, max) { SFlashVarValue args[6] = {cvar, name, "ListSlider", "cvar_change_f", min, max}; \
	m_pFlashPlayer->Invoke(#list ".CreateItem", args, 6); m_RegisteredCVars.push_back(cvar); }

CFlashMainMenu::CFlashMainMenu() :
	m_bCursor(false),
	m_iWidth(1),
	m_iHeight(1)
{
	m_pFlashPlayer = gEnv->pSystem->CreateFlashPlayerInstance();

	if (m_pFlashPlayer)
	{
		bool bRes = m_pFlashPlayer->Load(FLASH_MAINMENU_FILENAME);
		m_pFlashPlayer->SetFSCommandHandler(this);
		m_pFlashPlayer->SetBackgroundAlpha(0);
		CryLogAlways("Loading %s: %s",FLASH_MAINMENU_FILENAME, (bRes)?"Done":"Error");
		if (!bRes)
		{
			SAFE_RELEASE(m_pFlashPlayer);
			return;
		}
	}
	else
	{
		CryLogAlways("No Scaleform support");
		return;
	}

	g_pGameCVars->g_flash_mainmenu_state = eSM_Menu;

	ILevelSystem* pLevelSystem = gEnv->pGame->GetIGameFramework()->GetILevelSystem();
	pLevelSystem->AddListener(this);
	gEnv->pHardwareMouse->AddListener(this);


	ADD_LIST_GROUP(LevelList, "levels")

	for (int i = 0; i < pLevelSystem->GetLevelCount(); ++i)
	{
		ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(i);
		ADD_LIST_ITEM(LevelList,  pLevelInfo->GetName(), "levelSelect")
	}

	ADD_LIST_GROUP(LevelList, "")
	ADD_LIST_ITEM(LevelList,  "exit", "act_exit")


	ADD_LIST_GROUP(ScreenSettings, "screen")
	ADD_LIST_CHECKBOX(ScreenSettings, "fullscreen", "r_Fullscreen")
	ADD_LIST_SLIDER(ScreenSettings, "width", "r_Width", 1024, 1920);
	ADD_LIST_SLIDER(ScreenSettings, "height", "r_Height", 768, 1080);

	ADD_LIST_GROUP(SoundSettings, "sound")
	ADD_LIST_SLIDER(SoundSettings, "sfx volume", "s_SFXVolume", 0, 1);
	ADD_LIST_SLIDER(SoundSettings, "music volume", "s_MusicVolume", 0, 1);
}

CFlashMainMenu::~CFlashMainMenu()
{
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
	gEnv->pHardwareMouse->RemoveListener(this);
	SAFE_RELEASE(m_pFlashPlayer);
}

void CFlashMainMenu::Draw(float fDeltaTime)
{
	if (!m_pFlashPlayer || gEnv->bEditor) return;

	updateState();

	if (g_pGameCVars->g_flash_mainmenu_state == eSM_None) return;

	int x, y, dx, dy;
	gEnv->pRenderer->GetViewport(&x, &y, &dx, &dy);

	if (dx != m_iWidth || dy != m_iHeight)
	{
		m_iWidth = dx;
		m_iHeight = dy;
		SFlashHelper::updateViewPort(m_pFlashPlayer, true, SFlashHelper::eFPT_Mid, SFlashHelper::eFPT_Mid);
	}

	for (std::list<string>::iterator iter = m_RegisteredCVars.begin(); iter != m_RegisteredCVars.end(); ++iter)
	{
		ICVar* pCVar = gEnv->pConsole->GetCVar(iter->c_str());
		if (pCVar) 
		{
			SFlashVarValue args[2] = {iter->c_str(), pCVar->GetFVal()};
			m_pFlashPlayer->Invoke("SetValue", args, 2);
		}
	}
	if (gEnv->pConsole->GetCVar("r_Fullscreen") && gEnv->pConsole->GetCVar("r_Fullscreen")->GetIVal() == 1)
	{
		SFlashVarValue wargs[2] = {"r_Width", false};
		SFlashVarValue hargs[2] = {"r_Height", false};
		m_pFlashPlayer->Invoke("SetEnabled", wargs, 2);
		m_pFlashPlayer->Invoke("SetEnabled", hargs, 2);
	}
	else
	{
		SFlashVarValue wargs[2] = {"r_Width", true};
		SFlashVarValue hargs[2] = {"r_Height", true};
		m_pFlashPlayer->Invoke("SetEnabled", wargs, 2);
		m_pFlashPlayer->Invoke("SetEnabled", hargs, 2);
	}

	m_pFlashPlayer->Advance(fDeltaTime);
	m_pFlashPlayer->Render();
	m_pFlashPlayer->Invoke0("Update");
}

void CFlashMainMenu::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta )
{
	if (!m_pFlashPlayer) return;

	m_pFlashPlayer->ScreenToClient(iX, iY);

	switch(eHardwareMouseEvent)
	{
	case HARDWAREMOUSEEVENT_LBUTTONDOWN:
		m_pFlashPlayer->SendCursorEvent(SFlashCursorEvent(SFlashCursorEvent::eCursorPressed,iX, iY));
		break;
	case HARDWAREMOUSEEVENT_LBUTTONUP:
		m_pFlashPlayer->SendCursorEvent(SFlashCursorEvent(SFlashCursorEvent::eCursorReleased,iX, iY));
		break;
	case HARDWAREMOUSEEVENT_MOVE:
		m_pFlashPlayer->SendCursorEvent(SFlashCursorEvent(SFlashCursorEvent::eCursorMoved,iX, iY));
		break;
	}
	m_pFlashPlayer->Advance(0);
}

void CFlashMainMenu::HandleFSCommand( const char *szCommand,const char *strArgs, void* pUserData )
{
	if (g_pGameCVars->g_flash_mainmenu_state != eSM_Menu) return;

	std::vector<string> args = SFlashHelper::SliceArguments(strArgs);
	if (!strcmp(szCommand, "levelSelect") && args.size() > 0)
	{
		gEnv->pGame->GetIGameFramework()->PauseGame(false,true);
		char map_exec_str[255];
		sprintf(map_exec_str,"map %s", args[0].c_str());
		gEnv->pConsole->ExecuteString(map_exec_str);
	}
	else if (!strcmp(szCommand, "cvar_change_b") && args.size() > 0)
	{
		ICVar* pCVar = gEnv->pConsole->GetCVar(args[0]);
		if (pCVar)
			pCVar->Set(pCVar->GetIVal() ? 0 : 1);
	}
	else if (!strcmp(szCommand, "cvar_change_f") && args.size() > 1)
	{
		ICVar* pCVar = gEnv->pConsole->GetCVar(args[0]);
		if (pCVar)
			pCVar->Set(args[1]);
	}
	else if (!strcmp(szCommand, "act_exit"))
	{
		gEnv->pSystem->Quit();
	}
}

void CFlashMainMenu::OnLoadingStart(ILevelInfo* pLevel)
{
	g_pGameCVars->g_flash_mainmenu_state = eSM_Loading;
}

void CFlashMainMenu::OnLoadingComplete(ILevel* pLevel)
{
	g_pGameCVars->g_flash_mainmenu_state = eSM_None;
}

void CFlashMainMenu::OnLoadingProgress(ILevelInfo* pLevel, int progressAmount)
{
	bool isStandAlone = ( gEnv->pRenderer->EF_Query(EFQ_RecurseLevel) <= 0 );
	if (isStandAlone)
	{
		gEnv->pSystem->RenderBegin();
	}

	CGame* pGame = static_cast<CGame*>(gEnv->pGame);
	if (pGame)
		pGame->GetBitmapUI()->Draw();

	if (m_pFlashPlayer)
		m_pFlashPlayer->Invoke1("SetLoadingProgress", progressAmount);
	Draw(gEnv->pTimer->GetFrameRate());


	if (isStandAlone)
	{
		gEnv->pSystem->RenderEnd();
	}
}

void CFlashMainMenu::updateState()
{
	switch (g_pGameCVars->g_flash_mainmenu_state)
	{
	case eSM_Menu:
		m_pFlashPlayer->Invoke0("SetMenu");
		if (!m_bCursor)
		{
			m_bCursor = true;
			gEnv->pHardwareMouse->IncrementCounter();
		}
		break;
	case eSM_Loading:
	case eSM_None:
		m_pFlashPlayer->Invoke0("SetLoading");
		if (m_bCursor)
		{
			m_bCursor = false;
			gEnv->pHardwareMouse->DecrementCounter();
		}
		break;
	}
}
