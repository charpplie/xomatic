#if !defined(AFX_ABOUT_H__D8E04BE2_B88E_11D1_98AD_0040051EDCE7__INCLUDED_)
#define AFX_ABOUT_H__D8E04BE2_B88E_11D1_98AD_0040051EDCE7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// About.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog

class CAbout : public CDialog
{
// Construction
public:
	CAbout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAbout)
	enum { IDD = IDD_ABOUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAbout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAbout)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_sSerialNo;

protected:
	virtual void OnOK();
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedGetserialfromcrytek();
	afx_msg void OnBnClickedCredits();
	afx_msg void OnBnClickedSetuprc();
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.




#endif // !defined(AFX_ABOUT_H__D8E04BE2_B88E_11D1_98AD_0040051EDCE7__INCLUDED_)
