/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements character parts dialog.

-------------------------------------------------------------------------
History:
- 11:01:2008: Created by Maxim Dyachenko

*************************************************************************/

#include "stdafx.h"
#include "TaskPanelDlg.h"

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CTaskPanelDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

//-------------------------------------------------------------------------
CTaskPanelDlg::CTaskPanelDlg(CWnd* pParent /* =NULL */) : CDialog(UINT(0), pParent), m_pItem(NULL)
{
}

//-------------------------------------------------------------------------
void CTaskPanelDlg::UpdateColors()
{
	if (!m_pItem)
		return;

	COLORREF clrBack = m_pItem->GetBackColor();

	if (!m_brushBack.GetSafeHandle() || clrBack != m_clrBack)
	{
		m_brushBack.DeleteObject();
		m_brushBack.CreateSolidBrush(clrBack);
		m_clrBack = clrBack;
	}
}

//-------------------------------------------------------------------------
void CTaskPanelDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	Invalidate(FALSE);
}

//-------------------------------------------------------------------------
BOOL CTaskPanelDlg::OnEraseBkgnd(CDC* pDC)
{
	UpdateColors();
	pDC->FillSolidRect(CXTPClientRect(this), m_clrBack);

	return TRUE;
}

//-------------------------------------------------------------------------
HBRUSH CTaskPanelDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor != CTLCOLOR_BTN && nCtlColor != CTLCOLOR_STATIC)
		return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	UpdateColors();
	pDC->SetBkMode(TRANSPARENT);

	return m_brushBack;
}