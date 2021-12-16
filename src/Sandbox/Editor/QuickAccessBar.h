//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : QuickAccessBar.h
//  Author           : Jaewon Jung
//  Time of creation : 8/30/2011   14:20
//  Compilers        : VS2008
//  Description      : A dialog bar for quickly accessing menu items and cvars
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __QUICK_ACCESS_BAR_H__
#define __QUICK_ACCESS_BAR_H__

#pragma once

#include "Controls/ACEdit.h"

class CQuickAccessBar : public CDialog
{
	DECLARE_DYNCREATE(CQuickAccessBar)

public:
	CQuickAccessBar(CWnd* pParent = NULL);

	enum { IDD = IDD_QUICK_ACCESS_BAR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	CACEdit m_inputEdit;
	std::map<CString, int> m_menuIdTable;

	void CollectMenuItems(const CXTPCommandBar *pCmdBar, const CString& menuPath);
	void CollectDynamicOpenViewPaneItems(const CString& newMenuPath);
	void AddMRUFileItems();
};

#endif // __QUICK_ACCESS_BAR_H__