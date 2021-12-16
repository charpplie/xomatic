#if !defined(AFX_LICENSELOGINDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_)
#define AFX_LICENSELOGINDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LicenseKeyDialog.h : header file
//

#if defined(IS_PROSDK)

/////////////////////////////////////////////////////////////////////////////
// CLicenseLoginDialog dialog
#include "resource.h"
#include "afxwin.h"

struct IProtectionManager;

class CLicenseLoginDialog : public CDialog
{
	DECLARE_DYNAMIC(CLicenseLoginDialog)

	// Construction
public:
	CLicenseLoginDialog(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CLicenseLoginDialog)
	enum { IDD = IDD_LICENSE_LOGIN };

	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLicenseLoginDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL
	virtual void OnOK();

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLicenseLoginDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonLogin();
	afx_msg void OnBnClickedButtonCreateaccount();
	afx_msg void OnBnClickedButtonPasswordRemind();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	bool IsValidEditText();
	void RunLoginProcess();
	void LoginImpl();
	void MakeEnableInput();
	void MakeDisableInput();
	bool IsAcceptLicenseAgreement(bool agreeLicenseFlag);

private:
	CEdit m_eAccount;
	CEdit m_ePassword;
	CButton m_buttonLogin;
	CButton m_buttonCreateAccount;
	CButton m_buttonPasswordRemind;
	bool m_doingLoginProcess;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif 

#endif // !defined(AFX_LICENSEKEYDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_)
