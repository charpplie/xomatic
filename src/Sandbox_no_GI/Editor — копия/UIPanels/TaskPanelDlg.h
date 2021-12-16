/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements base dialog for task panel embedded dialogs.

-------------------------------------------------------------------------
History:
- 11:01:2008: Created by Maxim Dyachenko

*************************************************************************/
#ifndef __TASKPANELDLG_H__
#define __TASKPANELDLG_H__

#if _MSC_VER > 1000
#pragma once
#endif

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
class SANDBOX_API CTaskPanelDlg : public CDialog
{
public:
  //-------------------------------------------------------------------------
	CTaskPanelDlg(CWnd* pParent = NULL);

  //-------------------------------------------------------------------------
	void SetItem(CXTPTaskPanelGroupItem* pItem)
  {
    m_pItem = pItem;
  }

  //-------------------------------------------------------------------------
	virtual void OnOK() { }
	virtual void OnCancel() { }
	virtual BOOL PreTranslateMessage(MSG* pMsg) { return __super::PreTranslateMessage(pMsg); }

  virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam)
  {
    // Notify task panel's parent
    CWnd* pParent = GetParent();
    if (pParent)
    {
      pParent = pParent->GetParent();
      if (pParent)
      {
        pParent->SendMessage(WM_COMMAND, wParam, lParam);
        return TRUE;
      }
    }
    return FALSE;
  }

protected:
  //-------------------------------------------------------------------------
	void UpdateColors();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

protected:
	CXTPTaskPanelGroupItem* m_pItem;
	CBrush      m_brushBack;
	COLORREF    m_clrBack;

	DECLARE_MESSAGE_MAP()
};

#endif // __TASKPANELDLG_H__