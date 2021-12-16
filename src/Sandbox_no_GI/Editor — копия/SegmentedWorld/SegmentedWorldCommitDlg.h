////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SegmentedWorldNewWorldDlg.h
//  Version:     v1.00
//  Created:     08/04/2011 by Veli.
//  Compilers:   Visual Studio 2008
//  Description: writing "commit" description for segmented world
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SW_COMMIT_DLG_H__
#define __SW_COMMIT_DLG_H__
#pragma once

// SegmentedWorldCommitDlg.h : header file
//

#include "resource.h"
#include "afxwin.h"
/////////////////////////////////////////////////////////////////////////////
// CSWCommitDlg dialog

class CSWCommitDlg : public CDialog
{
// Construction
public:
	CSWCommitDlg( CWnd* pParent = NULL, const char *title = NULL);   // standard constructor
	virtual ~CSWCommitDlg();

	// Dialog Data
	enum { IDD = IDD_DIALOG_SW_COMMIT };
	
// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
// Implementation
protected:
	CString m_title;
	CEdit m_EditDescription;
	string m_strDescription;

	CButton m_cbEnableDescr;
	
	// Generated message map functions
	virtual BOOL OnInitDialog();
	//virtual void OnCancel();
	//virtual void OnOK();
	
	DECLARE_MESSAGE_MAP()
public:
	const char* GetDescription();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	
	afx_msg void OnBnClickedCheckSwCommit();
};

#endif // __SW_COMMIT_DLG_H__
