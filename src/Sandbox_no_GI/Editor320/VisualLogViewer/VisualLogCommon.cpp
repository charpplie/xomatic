// VisualLogCommon.cpp : implementation of the common classes
//

#include "stdafx.h"
#include "VisualLogCommon.h"




const float		SVLogCommonData::fFONT_MUL	= 60.f;
const CString	SVLogCommonData::strFONT		= "Arial";

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFixedToolTip class
IMPLEMENT_DYNAMIC(CFixedToolTip, CToolTipCtrl)

BEGIN_MESSAGE_MAP(CFixedToolTip, CToolTipCtrl)
END_MESSAGE_MAP()



// CFixedToolTip construction & destruction
CFixedToolTip::CFixedToolTip()
{
}

CFixedToolTip::~CFixedToolTip()
{
}



// CFixedToolTip operations
bool CFixedToolTip::AddWindowTool(CWnd *pWnd, LPCTSTR szText)
{
	TOOLINFO ti;

	::ZeroMemory(&ti, sizeof(TOOLINFO));
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = pWnd->GetParent()->GetSafeHwnd();
	ti.uId = (UINT_PTR)pWnd->GetSafeHwnd();
	ti.hinst = AfxGetInstanceHandle();
	ti.lpszText = (LPTSTR)szText;

	return (SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti))  ?  true : false;
}

bool CFixedToolTip::AddRectTool(CWnd *pWnd, LPCTSTR szText, LPRECT pRect, UINT nIDTool)
{
	TOOLINFO ti;

	::ZeroMemory(&ti, sizeof(TOOLINFO));
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = pWnd->GetSafeHwnd();
	ti.uId = nIDTool;
	ti.hinst = AfxGetInstanceHandle();
	ti.lpszText = (LPTSTR)szText;
	::CopyRect(&ti.rect, pRect);

	return (SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti))  ?  true : false;
}



// CFixedToolTip message handlers
////////////////////////////////////////////////////////////////////////////////////////////////////////////
