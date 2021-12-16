#if !defined(AFX_LICENSECREATEDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_)
#define AFX_LICENSECREATEDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LicenseKeyDialog.h : header file
//

#if defined(IS_PROSDK)

/////////////////////////////////////////////////////////////////////////////
// CLicenseCreateDialog dialog
#include "resource.h"
#include "afxwin.h"

class CLicenseCreateDialog : public CDialog
{
	DECLARE_DYNAMIC(CLicenseCreateDialog)

	// Construction
public:
	CLicenseCreateDialog(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CLicenseCreateDialog)
	enum { IDD = IDD_LICENSE_CREATE };
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLicenseCreateDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLicenseCreateDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CEdit m_eFirstname;
	CEdit m_eLastname;
	CEdit m_eEmail;
	CEdit m_eAddress;
	CEdit m_eCity;
	CComboBox m_cCountry;
	CEdit m_eState;
	CEdit m_eZipcode;
	CEdit m_ePassword;
	CEdit m_eUsername;
	CButton m_buttonSubmit;
	CButton m_buttonCancel;

protected:
	afx_msg void OnBnClickedButtonCreatesubmit();
	afx_msg void OnBnClickedButtonCreatecancel();
	void RegisterCountryCombo();
	bool IsValidEditText();
	bool CheckEachEdit(CEdit& editObj, CString editTitle);
	void CreateAccountImpl();
	void MakeEnableInput();
	void MakeDisableInput();
	void GetAccountParamText(struct SCreateAccountParam& param);
	void ConvertEditText(CEdit& editObj, string& text);
	bool CheckUsernameEdit();
	bool CheckPasswordEdit();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif 

#endif // !defined(AFX_LICENSEKEYDIALOG_H__C8E98A11_BBE9_499F_AB62_6EA12BDBCF84__INCLUDED_)
