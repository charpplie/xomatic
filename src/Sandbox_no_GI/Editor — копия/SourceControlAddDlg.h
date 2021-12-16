////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SourceControlAddDlg.h
//  Version:     v1.00
//  Created:     25/10/2010 by Francesco Roccucci.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#if !defined(_SOURCECONTROLADDDLG_H_)
#define _SOURCECONTROLADDDLG_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SourceControlDescDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSourceControlAddDlg dialog

class CSourceControlAddDlg : public CDialog
{
public:

	enum ESCDialogResult
	{
		ADD_AND_SUBMIT,
		ADD_ONLY,
		NO_OP,
	};

	// Construction
	CSourceControlAddDlg(const CString& sFilename, CWnd* pParent = NULL);   // standard constructor
	// Return the state of the dialog
	ESCDialogResult GetResult() const { return m_result; };

// Dialog Data
	//{{AFX_DATA(CSourceControlAddDlg)
	enum { IDD = IDD_SOURCECONTROL_ADD };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSourceControlAddDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSourceControlAddDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedAddDefault();
	virtual BOOL OnInitDialog();

public:

	CString m_sDesc;
	CString m_sFilename;

private:
	ESCDialogResult m_result;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_SOURCECONTROLADDDLG_H_)
