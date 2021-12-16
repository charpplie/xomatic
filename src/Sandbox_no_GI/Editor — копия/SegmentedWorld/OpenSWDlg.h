////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   OpenSWDlg.h
//  Version:     v1.00
//  Created:     20/05/2010 by Veli.
//  Compilers:   Visual Studio.NET
//  Description: Utility to view and open available segmented world
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __OPENSWDLG_H__
#define __OPENSWDLG_H__
#pragma once

// OpenSWDlg.h : header file
//

#include "resource.h"
#include "afxwin.h"
#include "SegmentedWorldManager.h"
/////////////////////////////////////////////////////////////////////////////
// OpenSWDlg dialog

class COpenSWDlg : public CDialog
{
// Construction
public:
	COpenSWDlg( CWnd* pParent = NULL, const char *title = NULL);   // standard constructor
	virtual ~COpenSWDlg();

	// Dialog Data
	enum { IDD = IDD_DIALOG_OPEN_SW };
	int m_nLevelListType;
// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
// Implementation
protected:
	
	CString m_title;
	std::vector<sw::TWorldName> m_lstWorldsNames;
	CListBox m_listBox;

	// Generated message map functions
	afx_msg void OnOK();
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWorldBrowse();
	afx_msg void OnLbnDblclkListSw();
};

#endif // __OPENSWDLG_H__
