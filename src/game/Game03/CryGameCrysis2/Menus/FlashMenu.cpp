#include "StdAfx.h"
#include "FlashMenu.h"

#include "FlashMainMenu.h"
#include "FlashIngameMenu.h"
#include "IHardwareMouse.h"

CFlashMenu::CFlashMenu() :
	m_pMainMenu(0),
	m_pIngameMenu(0)
{
	m_pMainMenu = new CFlashMainMenu();
	m_pIngameMenu = new CFlashIngameMenu();
	if ( gEnv->pInput )
	{
		gEnv->pInput->AddEventListener(this);
	}
}

CFlashMenu::~CFlashMenu()
{
	if ( gEnv->pInput )
	{
		gEnv->pInput->RemoveEventListener(this);
	}
	SAFE_DELETE(m_pMainMenu)
	SAFE_DELETE(m_pIngameMenu)
}

void CFlashMenu::Draw(float fDeltaTime)
{
	m_pMainMenu->Draw(fDeltaTime);
	m_pIngameMenu->Draw(fDeltaTime);
}

bool CFlashMenu::OnInputEvent( const SInputEvent &event )
{
	// hack to use A button as mouse clicks
	EHARDWAREMOUSEEVENT evt;

	switch (event.state)
	{
	case eIS_Pressed: evt = HARDWAREMOUSEEVENT_LBUTTONDOWN;
		break;
	case eIS_Released: evt = HARDWAREMOUSEEVENT_LBUTTONUP;
		break;
	default: return false;
	}

	float fX, fY;
	gEnv->pHardwareMouse->GetHardwareMousePosition(&fX, &fY);

	switch (event.keyId)
	{
	case eKI_XI_A: // todo add keys for all plattforms
		m_pMainMenu->OnHardwareMouseEvent((int) fX, (int) fY, evt, 0);
		m_pIngameMenu->OnHardwareMouseEvent((int) fX, (int) fY, evt, 0);
	}
	return false;

}
