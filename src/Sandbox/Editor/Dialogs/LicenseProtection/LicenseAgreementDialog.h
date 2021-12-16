#if !defined(AFX_LICENSEAGREEMENTDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_)
#define AFX_LICENSEAGREEMENTDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LicenseKeyDialog.h : header file
//

#if defined(IS_PROSDK)

/////////////////////////////////////////////////////////////////////////////
// CLicenseAgreementDialog dialog
#include "resource.h"
#include "afxwin.h"

class CLicenseAgreementDialog : public CDialog
{
	DECLARE_DYNAMIC(CLicenseAgreementDialog)

	// Construction
public:
	CLicenseAgreementDialog(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CLicenseAgreementDialog)
	enum { IDD = IDD_LICENSE_AGREEMENT };

	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLicenseAgreementDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ConfigureAgreement();

	// Generated message map functions
	//{{AFX_MSG(CLicenseAgreementDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CEdit m_eAgreement;
public:
	afx_msg void OnBnClickedButtonAccept();
	afx_msg void OnBnClickedButtonDeny();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif 

#endif // !defined(AFX_LICENSEKEYDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_)
