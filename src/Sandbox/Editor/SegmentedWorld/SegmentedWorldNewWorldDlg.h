////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SegmentedWorldNewWorldDlg.h
//  Version:     v1.00
//  Created:     08/03/2011 by Veli.
//  Compilers:   Visual Studio 2008
//  Description: Utility to select new segmented world name
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SW_NEW_WORLD_DLG_H__
#define __SW_NEW_WORLD_DLG_H__
#pragma once

// SegmentedWorldNewWorldDlg.h : header file
//

#include "resource.h"
#include "afxwin.h"
#include "SegmentedWorldManager.h"



using namespace sw;
/////////////////////////////////////////////////////////////////////////////
// CSWNewWorldDlg dialog

class CSWNewWorldDlg : public CDialog
{
// Construction
public:
	CSWNewWorldDlg( CWnd* pParent = NULL, const char *title = NULL);   // standard constructor
	virtual ~CSWNewWorldDlg();

	// Dialog Data
	enum { IDD = IDD_SW_SAVE_NAME };
	int m_nLevelListType;
// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
// Implementation
protected:
	CString m_title;
	string m_strSelectedWorld;
	CEdit m_worldNameEditBox;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	//virtual void OnCancel();
	//virtual void OnOK();
	
	DECLARE_MESSAGE_MAP()
public:
	void GetSelectedWorld(string &strWorldName) { strWorldName.Format("%s", m_strSelectedWorld.c_str()); }
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	
	afx_msg void OnEnUpdateEditSwNewName();
};

#endif // __SW_NEW_WORLD_DLG_H__
