// LicenseLoginDialog.cpp : implementation file
//

#include "stdafx.h"
#include "LicenseLoginDialog.h"

#if defined(IS_PROSDK)

#include "LicenseCreateDialog.h"
#include "LicenseAgreementDialog.h"
#include "LibSDKEvaluationWrapper.h"

IMPLEMENT_DYNAMIC(CLicenseLoginDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CLicenseLoginDialog::CLicenseLoginDialog(CWnd* pParent /*=NULL*/) :
CDialog(CLicenseLoginDialog::IDD, pParent), m_doingLoginProcess(false)
{
	//{{AFX_DATA_INIT(CLicenseLoginDialog)
	//}}AFX_DATA_INIT
}


//////////////////////////////////////////////////////////////////////////
BOOL CLicenseLoginDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetFocus();
	m_eAccount.SetFocus();
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
void CLicenseLoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLicenseLoginDialog)
	DDX_Control(pDX, IDC_EDIT_ACCOUNT, m_eAccount);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_ePassword);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BUTTON_LOGIN, m_buttonLogin);
	DDX_Control(pDX, IDC_BUTTON_CREATEACCOUNT, m_buttonCreateAccount);
	DDX_Control(pDX, IDC_BUTTON_PASSWORD_REMIND, m_buttonPasswordRemind);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CLicenseLoginDialog, CDialog)
	//{{AFX_MSG_MAP(CLicenseLoginDialog)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CLicenseLoginDialog::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_CREATEACCOUNT, &CLicenseLoginDialog::OnBnClickedButtonCreateaccount)
	ON_BN_CLICKED(IDC_BUTTON_PASSWORD_REMIND, &CLicenseLoginDialog::OnBnClickedButtonPasswordRemind)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
void CLicenseLoginDialog::OnBnClickedButtonLogin()
{
	// TODO: Add your control notification handler code here
	RunLoginProcess();
}

void CLicenseLoginDialog::OnBnClickedButtonCreateaccount()
{
	// TODO: Add your control notification handler code here
	CLicenseCreateDialog createDlg;
	createDlg.DoModal();
}

bool CLicenseLoginDialog::IsValidEditText()
{
	CString accountText;
	m_eAccount.GetWindowText(accountText);
	if (accountText.IsEmpty())
	{
		MessageBox("Please fill account.", "Login Error", MB_ICONERROR);
		m_eAccount.SetFocus();
		return false;
	}
	if (accountText.GetLength() < 4)
	{
		MessageBox("Account must be longer than 4", "Login Error", MB_ICONERROR);
		m_eAccount.SetFocus();
		return false;
	}

	CString passwordText;
	m_ePassword.GetWindowText(passwordText);
	if (passwordText.IsEmpty())
	{
		MessageBox("Please fill password.", "Login Error", MB_ICONERROR);
		m_ePassword.SetFocus();
		return false;
	}
	if (passwordText.GetLength() < 4)
	{
		MessageBox("Password must be longer than 4", "Login Error", MB_ICONERROR);
		m_ePassword.SetFocus();
		return false;
	}
	return true;
}

BOOL CLicenseLoginDialog::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->wParam == VK_RETURN)
	{
		CString passwordText;
		m_ePassword.GetWindowText(passwordText);
		bool passwordEditHasFocus = (pMsg->hwnd == m_ePassword.GetSafeHwnd());
		bool accountEditHasFocus = (pMsg->hwnd == m_eAccount.GetSafeHwnd());
		if (passwordEditHasFocus && false == passwordText.IsEmpty())
		{
			RunLoginProcess();
			return TRUE;
		}
		else if (accountEditHasFocus)
		{
			m_ePassword.SetFocus();
			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CLicenseLoginDialog::LoginImpl()
{
	if (false == IsValidEditText())
		return;

	CString accountText;
	m_eAccount.GetWindowText(accountText);
	CString passwordText;
	m_ePassword.GetWindowText(passwordText);
	string usernameParam = accountText.GetString();
	string passwordParam = passwordText.GetString();

	//string encodedPassword = Base64Encode((const uint8*)passwordParam.c_str(), passwordParam.length());
	bool agreeLicenseFlag = false;
	EPMErrorCode subErrorCode = PMEC_NO_ERROR;
	ELoginAccountResult resultCode = eLA_Success;
	IEvaluationManager* pEM = GetISystem()->GetEvaluationManager();
	resultCode = pEM->RequestLoginAccount(usernameParam, passwordParam, agreeLicenseFlag, subErrorCode);

	// if it takes long that user inputs userdata to dialog, socket could be timeout.
	// Retry send account information to LicenseServer, 
	if (resultCode == eCA_Undefined && (subErrorCode == PMEC_CANT_WRITE_TO_SOCKET || subErrorCode == PMEC_CANT_READ_FROM_SOCKET))
		resultCode = pEM->RequestLoginAccount(usernameParam, passwordParam, agreeLicenseFlag, subErrorCode);

	if (eLA_Success == resultCode)
	{
		if (IsAcceptLicenseAgreement(agreeLicenseFlag))
			EndDialog(IDOK);
		else
			EndDialog(IDCANCEL);
		return;
	}
	else if (eLA_InvalidLicense == resultCode)
	{
		MessageBox("Your License could not be validated.\n\nPlease contact Crytek to resolve this issue.", "Login Error", MB_ICONERROR);
	}
	else if (eLA_InvalidAccount == resultCode)
	{
		MessageBox("Invalid Username or Password.\n\nPlease check Username or Password.", "Login Error", MB_ICONERROR);
	}
	else if (eLA_InactiveAccount == resultCode)
	{
		MessageBox("Your account is not activated.\n\nPlease check your e-mail for authentication.", "Login Error", MB_ICONERROR);
	}
	else if (eLA_Undefined == resultCode)
	{
		if (PMEC_NO_ERROR == subErrorCode)
		{
			MessageBox("Undefined error.\n\nPlease try again later.", "Login Error", MB_ICONERROR);
		}
		else
		{
			pEM->AlertServerErrorMessage(subErrorCode, GetSafeHwnd());
		}
	}
}

void CLicenseLoginDialog::MakeEnableInput()
{
	m_buttonLogin.EnableWindow(TRUE);
	m_buttonCreateAccount.EnableWindow(TRUE);
	m_buttonPasswordRemind.EnableWindow(TRUE);
}

void CLicenseLoginDialog::MakeDisableInput()
{
	m_buttonLogin.EnableWindow(FALSE);
	m_buttonCreateAccount.EnableWindow(FALSE);
	m_buttonPasswordRemind.EnableWindow(FALSE);
}

void CLicenseLoginDialog::RunLoginProcess()
{
	if (m_doingLoginProcess)
		return;
	m_doingLoginProcess = true;
	MakeDisableInput();
	LoginImpl();
	MakeEnableInput();
	m_doingLoginProcess = false;
}

bool CLicenseLoginDialog::IsAcceptLicenseAgreement(bool agreeLicenseFlag)
{
	if (agreeLicenseFlag)
		return true;
	CLicenseAgreementDialog agreementDlg;
	INT_PTR iResult = agreementDlg.DoModal();
	if (iResult == IDOK)
	{
		CString accountText;
		m_eAccount.GetWindowText(accountText);
		string usernameParam = accountText.GetString();
		IEvaluationManager* pEM = GetIEditor()->GetSystem()->GetEvaluationManager();
		pEM->SendLicenseAgree(usernameParam);
		return true;
	}
	else
	{
		return false;
	}
}

void CLicenseLoginDialog::OnOK()
{
}

void CLicenseLoginDialog::OnBnClickedButtonPasswordRemind()
{
	CString accountText;
	m_eAccount.GetWindowText(accountText);
	if (accountText.IsEmpty())
	{
		MessageBox("Please enter your e-mail address or username in the \"Username\" field.", 
			"Send Password", MB_ICONERROR);
		m_eAccount.SetFocus();
		return;
	}

	string username, email;
	if (-1 == accountText.Find('@'))
		username = accountText.GetString();
	else
		email = accountText.GetString();

	IEvaluationManager* pEM = GetIEditor()->GetSystem()->GetEvaluationManager();
	bool result = pEM->RequestPasswordRemind(username, email);
	if (result)
		MessageBox("Your password has been sent to your e-mail address.", "Send Password", MB_OK);
	else
		MessageBox("The username or e-mail address entered does not exist.\n", "Send Password", MB_ICONERROR);
}
#endif