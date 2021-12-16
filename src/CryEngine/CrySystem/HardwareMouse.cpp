/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

System "Hardware mouse" cursor with reference counter.
This is needed because Menus / HUD / Profiler / or whatever
can use the cursor not at the same time be successively
=> We need to know when to enable/disable the cursor.

-------------------------------------------------------------------------
History:
- 18:12:2006   Created by Julien Darré

*************************************************************************/

#include "StdAfx.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "IConsole.h"
#include "IInput.h"
#include "ISystem.h"
#include "ITimer.h"
#include "HardwareMouse.h"

#ifdef WIN32
void ReleaseCursor()
{
	::ClipCursor(NULL);
}
#endif
//-----------------------------------------------------------------------------------------------------

CHardwareMouse::CHardwareMouse(bool bVisibleByDefault)
: m_debugHardwareMouse(0)
{
#ifdef WIN32
	atexit(ReleaseCursor);
#else
	if(gEnv->pRenderer)
	{
		m_pCursorTexture = gEnv->pRenderer->EF_LoadTexture("Textures/Gui/Cursor_Green.tif",FT_DONT_RELEASE|FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
		m_pCursorTexture->SetClamp(true);		
		SetHardwareMousePosition(gEnv->pRenderer->GetWidth()*0.5f,gEnv->pRenderer->GetHeight()*0.5f);
	}
	else
	{
		m_pCursorTexture = NULL;
		SetHardwareMousePosition(0.0f,0.0f);
	}
#endif
	Reset(bVisibleByDefault);

	if (gEnv->pSystem)
		gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

#if !defined(_RELEASE)
	if(gEnv->pConsole)
		gEnv->pConsole->Register("g_debugHardwareMouse", &m_debugHardwareMouse, 0, VF_CHEAT, CVARHELP("Enables debug mode for the hardware mouse."));
#endif //!defined(_RELEASE)


	m_hide = false;
	m_calledShowHWMouse = false;
}

//-----------------------------------------------------------------------------------------------------

CHardwareMouse::~CHardwareMouse()
{
	if(gEnv)
	{
		if(gEnv->pRenderer)
		{
			gEnv->pRenderer->RemoveListener(this);
#ifndef WIN32
			SAFE_RELEASE(m_pCursorTexture);
#endif
		}
		if(gEnv->pInput)
			gEnv->pInput->RemoveEventListener(this);
		
		if (gEnv->pSystem)
			gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

#if !defined(_RELEASE)
		if(gEnv->pConsole)
			gEnv->pConsole->UnregisterVariable("g_debugHardwareMouse", true);
#endif //!defined(_RELEASE)
	}
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::ShowHardwareMouse(bool bShow)
{
	if (m_debugHardwareMouse)
		CryLogAlways("HM: ShowHardwareMouse = %d", bShow);

	if(bShow)
	{
		SetHardwareMousePosition(m_fCursorX,m_fCursorY);
	}
	else
	{
		GetHardwareMousePosition(&m_fCursorX,&m_fCursorY);
	}

#ifdef WIN32
	::ShowCursor(bShow && !m_hide);
#endif
	bool bConfine = !bShow;

	if (IsFullscreen())
		bConfine = true;

	ConfineCursor(bConfine);

	if (gEnv->pInput) gEnv->pInput->SetExclusiveMode(eDI_Mouse,false);

	m_calledShowHWMouse = true;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::ConfineCursor(bool confine)
{
	if (m_debugHardwareMouse)
		CryLogAlways("HM: ConfineCursor = %d", confine);
#ifdef WIN32
	if (gEnv == NULL || gEnv->pRenderer == NULL)
		return;

	HWND hWnd = 0;
	
	if (gEnv->IsEditor())
		hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
	else
		hWnd = (HWND) gEnv->pRenderer->GetHWND();

	if (hWnd)
	{
		// It's necessary to call ClipCursor AFTER the calls to
		// CreateDevice/ResetDevice otherwise the clip area is reseted.
		if(confine && !gEnv->pSystem->IsEditorMode())
		{
			if (m_debugHardwareMouse)
				gEnv->pLog->Log("HM:   Confining cursor");
			RECT rcClient;
			::GetClientRect(hWnd, &rcClient);
			::ClientToScreen(hWnd, (LPPOINT)&rcClient.left);
			::ClientToScreen(hWnd, (LPPOINT)&rcClient.right);
			::ClipCursor(&rcClient);
		}
		else
		{
			if (m_debugHardwareMouse)
				gEnv->pLog->Log("HM:   Releasing cursor");
			::ClipCursor(NULL);
		}
	}
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::OnPostCreateDevice()
{
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::OnPostResetDevice()
{
}

//-----------------------------------------------------------------------------------------------------

bool CHardwareMouse::OnInputEvent(const SInputEvent &rInputEvent)
{
	static float s_fAcceleration = 1.0f;






	const EKeyId thumbX = eKI_XI_ThumbLX;
	const EKeyId thumbY = eKI_XI_ThumbLY;
	const EKeyId XButton = eKI_XI_A; //A on 360


	if(0 == m_iReferenceCounter)
	{
		// Do not emulate if mouse is not present on screen
	}
	else if(eDI_XI == rInputEvent.deviceId)
	{
		if(rInputEvent.keyId == thumbX)
		{
			m_fIncX = rInputEvent.value;
		}
		else if(rInputEvent.keyId == thumbY)
		{
			m_fIncY = -rInputEvent.value;
		}
		else if(rInputEvent.keyId == XButton)
		{
			// This emulation was just not right, A-s meaning is context sensitive
			/*if(eIS_Pressed == rInputEvent.state)
			{
				Event((int)m_fCursorX,(int)m_fCursorY,HARDWAREMOUSEEVENT_LBUTTONDOWN);
			}
			else if(eIS_Released == rInputEvent.state)
			{
				Event((int)m_fCursorX,(int)m_fCursorY,HARDWAREMOUSEEVENT_LBUTTONUP);
			}*/
			// TODO: do we simulate double-click?
		}
	}
	else if(rInputEvent.keyId == eKI_SYS_Commit)
	{
		const float fSensitivity = 100.0f;
		const float fDeadZone = 0.3f;

		if(	m_fIncX < -fDeadZone || m_fIncX > +fDeadZone ||
				m_fIncY < -fDeadZone || m_fIncY > +fDeadZone)
		{
			float fFrameTime = gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI);
			if(s_fAcceleration < 10.0f)
			{
				s_fAcceleration += fFrameTime * 5.0f;
			}
			m_fCursorX += m_fIncX * fSensitivity * s_fAcceleration * fFrameTime;
			m_fCursorY += m_fIncY * fSensitivity * s_fAcceleration * fFrameTime;
			SetHardwareMousePosition(m_fCursorX,m_fCursorY);
		}
		else
		{
			GetHardwareMousePosition(&m_fCursorX,&m_fCursorY);
			s_fAcceleration = 1.0f;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------
void CHardwareMouse::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	if (event == ESYSTEM_EVENT_CHANGE_FOCUS)
	{
		//gEnv->pLog->Log("Change focus - %d", wparam);
		m_bFocus = wparam != 0;

		if (m_bFocus)
		{
			if (!gEnv->pSystem->IsEditorMode() && m_recapture)
			{
				m_recapture = false;
				DecrementCounter();
			}

			if (IsFullscreen())
				ConfineCursor(true);
		}
		else
		{
			if (!gEnv->pSystem->IsEditorMode() && m_iReferenceCounter == 0)
			{
				m_recapture = true;
				IncrementCounter();
			}
		}
	}
	else if (event == ESYSTEM_EVENT_MOVE)
	{
		if (IsFullscreen() || m_iReferenceCounter==0)
			ConfineCursor(true);
	}
	else if (event == ESYSTEM_EVENT_RESIZE)
	{
		if (IsFullscreen() || m_iReferenceCounter==0)
			ConfineCursor(true);
	}
	else if (event == ESYSTEM_EVENT_TOGGLE_FULLSCREEN)
	{
		if (wparam || m_iReferenceCounter==0)
			ConfineCursor(true);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Release()
{
	delete this;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::OnPreInitRenderer()
{
	CRY_ASSERT(gEnv->pRenderer);

	if (gEnv->pRenderer)
		gEnv->pRenderer->AddListener(this);
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::OnPostInitInput()
{
	CRY_ASSERT(gEnv->pInput);

	if (gEnv->pInput) 
		gEnv->pInput->AddEventListener(this);
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Event(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta)
{
	// Ignore events while console is active
	if(gEnv->pConsole->GetStatus())
	{
		return;
	}

	for(TListHardwareMouseEventListeners::iterator iter=m_listHardwareMouseEventListeners.begin(); iter!=m_listHardwareMouseEventListeners.end(); ++iter)
	{
		(*iter)->OnHardwareMouseEvent(iX,iY,eHardwareMouseEvent, wheelDelta);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::AddListener(IHardwareMouseEventListener *pHardwareMouseEventListener)
{
	stl::push_back_unique(m_listHardwareMouseEventListeners,pHardwareMouseEventListener);
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::RemoveListener(IHardwareMouseEventListener *pHardwareMouseEventListener)
{
	stl::find_and_erase(m_listHardwareMouseEventListeners,pHardwareMouseEventListener);
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::SetGameMode(bool bGameMode)
{
	if(bGameMode)
	{
		DecrementCounter();
	}
	else
	{
		IncrementCounter();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::IncrementCounter()
{
	m_iReferenceCounter++;

	if (m_debugHardwareMouse)
		CryLogAlways("HM: IncrementCounter = %d", m_iReferenceCounter);
	CRY_ASSERT(m_iReferenceCounter >= 0);

	if(1 == m_iReferenceCounter)
	{
		ShowHardwareMouse(true);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::DecrementCounter()
{
	m_iReferenceCounter--;

	if (m_debugHardwareMouse)
		CryLogAlways("HM: DecrementCounter = %d", m_iReferenceCounter);
	CRY_ASSERT(m_iReferenceCounter >= 0);

	if(0 == m_iReferenceCounter)
	{
		ShowHardwareMouse(false);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::GetHardwareMousePosition(float *pfX,float *pfY)
{
#ifdef WIN32
	POINT pointCursor;
	GetCursorPos(&pointCursor);
	*pfX = (float) pointCursor.x;
	*pfY = (float) pointCursor.y;
#else
	*pfX = m_fVirtualX;
	*pfY = m_fVirtualY;
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::SetHardwareMousePosition(float fX,float fY)
{
#ifdef WIN32
	SetCursorPos((int) fX,(int) fY);
#else
	m_fVirtualX = fX;
	m_fVirtualY = fY;
	if(gEnv && gEnv->pRenderer)
	{
		float fWidth	= float(gEnv->pRenderer->GetWidth());
		float fHeight	= float(gEnv->pRenderer->GetHeight());

		if(m_fVirtualX < 0.0f)
		{
			m_fVirtualX = 0.0f;
		}
		else if(m_fVirtualX >= fWidth)
		{
			m_fVirtualX = fWidth - 1.0f;
		}

		if(m_fVirtualY < 0.0f)
		{
			m_fVirtualY = 0.0f;
		}
		else if(m_fVirtualY >= fHeight)
		{
			m_fVirtualY = fHeight - 1.0f;
		}
	}
	Event(int(m_fVirtualX),int(m_fVirtualY),HARDWAREMOUSEEVENT_MOVE);
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::GetHardwareMouseClientPosition(float *pfX,float *pfY)
{
#ifdef WIN32
	if (gEnv == NULL || gEnv->pRenderer == NULL)
		return;

	HWND hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
	CRY_ASSERT_MESSAGE(hWnd,"Impossible to get client coordinates from a non existing window!");

	if(hWnd)
	{
		POINT pointCursor;
		GetCursorPos(&pointCursor);
		ScreenToClient(hWnd,&pointCursor);
		*pfX = (float) pointCursor.x;
		*pfY = (float) pointCursor.y;
	}
	else
	{
		*pfX = 0.0f;
		*pfY = 0.0f;
	}
#else
	*pfX = m_fVirtualX;
	*pfY = m_fVirtualY;
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::SetHardwareMouseClientPosition(float fX,float fY)
{
#ifdef WIN32
	HWND hWnd = (HWND) gEnv->pRenderer->GetCurrentContextHWND();
	CRY_ASSERT_MESSAGE(hWnd,"Impossible to set position of the mouse relative to client coordinates from a non existing window!");

	if(hWnd)
	{
		POINT pointCursor;
		pointCursor.x = (int)fX;
		pointCursor.y = (int)fY;
		ClientToScreen(hWnd,&pointCursor);
		SetCursorPos(pointCursor.x,pointCursor.y);
	}
#else
	SetHardwareMousePosition(fX,fY);
#endif
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Reset(bool bVisibleByDefault)
{
	m_iReferenceCounter = bVisibleByDefault ? 1 : 0;
	//ShowHardwareMouse(bVisibleByDefault);
	GetHardwareMousePosition(&m_fCursorX,&m_fCursorY);

	if (!bVisibleByDefault)
		ConfineCursor(true);

	m_fIncX = 0.0f;
	m_fIncY = 0.0f;
	m_bFocus = true;
	m_recapture = false;
}

//-----------------------------------------------------------------------------------------------------

#ifndef WIN32
void CHardwareMouse::Render()
{
	if(gEnv && gEnv->pRenderer && m_iReferenceCounter && m_pCursorTexture && !m_hide)
	{
		float fScalerX = gEnv->pRenderer->ScaleCoordX(1.f);
		float fScalerY = gEnv->pRenderer->ScaleCoordY(1.f);
		const float fSizeX = float(m_pCursorTexture->GetWidth());
		const float fSizeY = float(m_pCursorTexture->GetHeight());
		gEnv->pRenderer->Draw2dImage(m_fVirtualX / fScalerX,m_fVirtualY / fScalerY,fSizeX / fScalerX,fSizeY / fScalerY,m_pCursorTexture->GetTextureID(),0,1,1,0,0,1,1,1,1,0);
	}
}
#endif

//-----------------------------------------------------------------------------------------------------

bool CHardwareMouse::IsFullscreen()
{
	assert(gEnv);
	assert(gEnv->pRenderer);

	return gEnv->pRenderer->EF_Query(EFQ_Fullscreen)!=0;
}

//-----------------------------------------------------------------------------------------------------

void CHardwareMouse::Hide(bool hide)
{
	m_hide = hide;
#ifdef WIN32
	if (m_calledShowHWMouse)
		::ShowCursor(m_iReferenceCounter > 0 && !m_hide);
#endif
}
