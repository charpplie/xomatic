////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIViewport.cpp
//  Version:     v1.00
//  Created:     06/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIViewport.h"

#include <IFlashUI.h>
#include <ITimer.h>
#include <IGameFramework.h>
#include <IActionMapManager.h>

IMPLEMENT_DYNCREATE(CUIViewport, CRenderViewport)

BEGIN_MESSAGE_MAP(CUIViewport, CRenderViewport)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETCURSOR()

	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUIViewport::CUIViewport()
: m_bPaused(false)
, m_bGameMode(false)
, m_bUpdateFG(false)
{
	m_lastFrameTimeUI = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);
	gEnv->pFlashUI->SetEditorScreenSizeCallback(functor(*this,&CUIViewport::OnGetScreenSize));
	SetRegularUpdateFg(false);
}

////////////////////////////////////////////////////////////////////
CUIViewport::~CUIViewport()
{
	gEnv->pFlashUI->RemoveEditorScreenSizeCallback();
}

////////////////////////////////////////////////////////////////////
int CUIViewport::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	if (CRenderViewport::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnDestroy()
{
	CRenderViewport::OnDestroy();
}

////////////////////////////////////////////////////////////////////
BOOL CUIViewport::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{

	HCURSOR hCursor = AfxGetApp()->LoadCursor(IDC_ARRWHITE);

	if(hCursor)
	{
		SetCursor(hCursor);
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIViewport::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{

}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnMouseMove(UINT nFlags, CPoint point)
{
	CRenderViewport::OnMouseMove(nFlags,point);
	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode)
		gEnv->pFlashUI->SendFlashMouseEvent(SFlashCursorEvent::eCursorMoved, point.x, point.y, 0);
}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRenderViewport::OnLButtonDown(nFlags,point);
	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode)
		gEnv->pFlashUI->SendFlashMouseEvent(SFlashCursorEvent::eCursorPressed, point.x, point.y, 0);
}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnLButtonUp(UINT nFlags, CPoint point)
{
	CRenderViewport::OnLButtonUp(nFlags,point);
	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode)
		gEnv->pFlashUI->SendFlashMouseEvent(SFlashCursorEvent::eCursorReleased, point.x, point.y, 0);
}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnRButtonDown(UINT nFlags, CPoint point)
{
	CRenderViewport::OnRButtonDown(nFlags,point);
	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode)
		gEnv->pFlashUI->SendFlashMouseEvent(SFlashCursorEvent::eCursorPressed, point.x, point.y, 1);
}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnRButtonUp(UINT nFlags, CPoint point)
{
	CRenderViewport::OnRButtonUp(nFlags,point);
	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode)
		gEnv->pFlashUI->SendFlashMouseEvent(SFlashCursorEvent::eCursorReleased, point.x, point.y, 1);
}

////////////////////////////////////////////////////////////////////
BOOL CUIViewport::OnMouseWheel(UINT nFlags, short wheelData, CPoint point)
{
	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode)
		gEnv->pFlashUI->SendFlashMouseEvent(SFlashCursorEvent::eWheel, point.x, point.y, 0, wheelData / 120.0f);
	return CRenderViewport::OnMouseWheel(nFlags,wheelData, point);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIViewport::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if ( event != eNotify_OnBeginGameMode )
		CRenderViewport::OnEditorNotifyEvent( event );

	if (event == eNotify_OnBeginGameMode)
	{
		if (gEnv->pFlashUI) 
		{
			gEnv->pFlashUI->RemoveEditorScreenSizeCallback();
			const int count = gEnv->pFlashUI->GetUIElementCount();
			for (int i = 0; i < count; ++i)
			{
				IUIElement* pElement = gEnv->pFlashUI->GetUIElement(i);
				pElement->Unload(true);
			}
			gEnv->pFlashUI->EnableEventStack(true);
		}

		m_bGameMode = true;
	}
	else if (event == eNotify_OnEndGameMode)
	{
		if (gEnv->pFlashUI) 
		{
			gEnv->pFlashUI->SetEditorScreenSizeCallback(functor(*this,&CUIViewport::OnGetScreenSize));
			gEnv->pFlashUI->EnableEventStack(m_bUpdateFG);
			gEnv->pFlashUI->Reload();
		}

		m_bGameMode = false;
	}
}

////////////////////////////////////////////////////////////////////
void CUIViewport::OnRender()
{
	FUNCTION_PROFILER( GetIEditor()->GetSystem(),PROFILE_EDITOR );

	const float now = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);
	float deltaUI =  now - m_lastFrameTimeUI;
	m_lastFrameTimeUI = now;

	ColorF black(Col_Black);
	gEnv->pRenderer->ClearBuffer( FRT_CLEAR | FRT_CLEAR_IMMEDIATE, &black );

	if (gEnv->pFlashUI && !m_bGameMode)
	{
		static ICVar* pRXVar = gEnv->pConsole->GetCVar("r_Width");
		static ICVar* pRYVar = gEnv->pConsole->GetCVar("r_Height");
		int width, height;
		OnGetScreenSize(width, height);
		if (pRXVar) pRXVar->Set(width);
		if (pRYVar) pRYVar->Set(height);
		gEnv->pFlashUI->Update(deltaUI);
	}

	ProcessKeys();
}

////////////////////////////////////////////////////////////////////
void CUIViewport::Update() 
{	
	FUNCTION_PROFILER( GetIEditor()->GetSystem(),PROFILE_EDITOR );

	if (gEnv->pFlashUI && !m_bPaused && !m_bGameMode && m_bUpdateFG)
	{
		IActionMapManager* pAmMgr = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
		if (pAmMgr)
		{
			pAmMgr->Enable(true);
			pAmMgr->EnableFilter("only_ui", true);
		}
		if (gEnv->pInput) gEnv->pInput->Update(true);
		gEnv->pFlashUI->UpdateFG();
		if (pAmMgr)
		{
			pAmMgr->EnableFilter("only_ui", false);
		}
	}

	CRenderViewport::Update();
}

////////////////////////////////////////////////////////////////////
void CUIViewport::SetRegularUpdateFg(bool bUpdate) 
{
	m_bUpdateFG = bUpdate;
	if (gEnv->pFlashUI)
		gEnv->pFlashUI->EnableEventStack(m_bUpdateFG);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIViewport::OnGetScreenSize(int& width, int&height)
{
	CRect rc;
	GetWindowRect(&rc);
	width = rc.Width();
	height = rc.Height();
}
