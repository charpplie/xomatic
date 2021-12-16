#include "StdAfx.h"
#include "FlashIngameMenu.h"

#include "IGameFramework.h"
#include "ILevelSystem.h"
#include "GameCVars.h"
#include "FlashHelper.h"

const char* FLASH_INGAMEMENU_FILENAME = "Libs\\UI\\FlashIngameMenu.gfx";

#define NO_RETURN_VALUE
#define ASSERT_AND_RETURN_PTR(ptr, ret) assert((ptr)); if (!(ptr)) return ret;

CFlashIngameMenu::CFlashIngameMenu() :
	m_bShow(false),
	m_iWidth(1),
	m_iHeight(1)
{
	m_pFlashPlayer = gEnv->pSystem->CreateFlashPlayerInstance();

	if (m_pFlashPlayer)
	{
		bool bRes = m_pFlashPlayer->Load(FLASH_INGAMEMENU_FILENAME);
		m_pFlashPlayer->SetFSCommandHandler(this);
		m_pFlashPlayer->SetBackgroundAlpha(0);
		CryLogAlways("Loading %s: %s",FLASH_INGAMEMENU_FILENAME, (bRes)?"Done":"Error");
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

	gEnv->pHardwareMouse->AddListener(this);
	gEnv->pInput->AddEventListener(this);
}

CFlashIngameMenu::~CFlashIngameMenu()
{
	gEnv->pHardwareMouse->RemoveListener(this);
	gEnv->pInput->RemoveEventListener(this);
	SAFE_RELEASE(m_pFlashPlayer);
}

void CFlashIngameMenu::Draw(float fDeltaTime)
{
	if (!m_pFlashPlayer || !m_bShow) return;

	int x, y, dx, dy;
	gEnv->pRenderer->GetViewport(&x, &y, &dx, &dy);

	if (dx != m_iWidth || dy != m_iHeight)
	{
		m_iWidth = dx;
		m_iHeight = dy;
		SFlashHelper::updateViewPort(m_pFlashPlayer, true, SFlashHelper::eFPT_Mid, SFlashHelper::eFPT_Mid);
	}

	m_pFlashPlayer->Advance(fDeltaTime);
	m_pFlashPlayer->Render();
}


void CFlashIngameMenu::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta )
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

void CFlashIngameMenu::HandleFSCommand( const char *szCommand,const char *strArgs, void* pUserData )
{
	if (!m_bShow) return;

	if (!strcmp(szCommand, "action_restart")) 
	{
		gEnv->pGame->GetIGameFramework()->Reset(false);
		ShowMenu(false);

		/*
		ASSERT_AND_RETURN_PTR(gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel(), NO_RETURN_VALUE);
		char map_exec_str[255];
		sprintf(map_exec_str,"map %s", gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->GetName());
		gEnv->pConsole->ExecuteString(map_exec_str);
		*/
	}
	else if (!strcmp(szCommand, "action_save"))
	{
		ASSERT_AND_RETURN_PTR(gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel(), NO_RETURN_VALUE);
		gEnv->pGame->GetIGameFramework()->SaveGame(gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->GetPath(), true);
		ShowMenu(false);
	}
	else if (!strcmp(szCommand, "action_load"))
	{
		ASSERT_AND_RETURN_PTR(gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel(), NO_RETURN_VALUE);
		gEnv->pGame->GetIGameFramework()->LoadGame(gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->GetPath(), true);
		ShowMenu(false);
	}
	else if (!strcmp(szCommand, "action_quit"))
	{
		g_pGameCVars->g_flash_mainmenu_state = 1;
		ShowMenu(false, false);
	}
}

bool CFlashIngameMenu::OnInputEvent( const SInputEvent &event )
{
	if (!gEnv->bEditor 
		&& g_pGameCVars->g_flash_mainmenu_state == 0
		&& event.state == eIS_Released 
		&& event.keyId == eKI_Escape
		&& gEnv->pGame->GetIGameFramework()->IsGameStarted())
	{
		ShowMenu(!m_bShow);
		return true;
	}
	return false;
}

void CFlashIngameMenu::ShowMenu(bool bShow, bool bSetGamePause)
{
	if (bShow == m_bShow) return;

	m_bShow = bShow;

	if (m_bShow)
		gEnv->pHardwareMouse->IncrementCounter();
	else
		gEnv->pHardwareMouse->DecrementCounter();

	if (bSetGamePause)
		gEnv->pGame->GetIGameFramework()->PauseGame(m_bShow , true);
}

