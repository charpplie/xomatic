#include "StdAfx.h"
#include "Dialogs/FeedbackFormDlg.h"
#include "Util/Mailer.h"
#define SECURITY_WIN32
#include <security.h>
LINK_SYSTEM_LIBRARY(Secur32.lib)


#define FEEDBACK_EMAIL_ADDRESS "nicusor@crytek.com"

IMPLEMENT_DYNAMIC(CFeedbackFormDlg, CDialog)

CFeedbackFormDlg::CFeedbackFormDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFeedbackFormDlg::IDD, pParent)
{
}

CFeedbackFormDlg::~CFeedbackFormDlg()
{
}

void CFeedbackFormDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFeedbackFormDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CFeedbackFormDlg::OnBnClickedOk)
END_MESSAGE_MAP()

void CFeedbackFormDlg::SetFeatureName(const char* pFeatureName)
{
	m_featureName = pFeatureName;
}

BOOL CFeedbackFormDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	const int kMaxUserNameSize = 512;
	ULONG maxUserNameSize = kMaxUserNameSize;
	char userName[kMaxUserNameSize];

	if (GetUserNameEx(NameDisplay, userName, &maxUserNameSize))
	{
		GetDlgItem(IDC_EDIT_FEEDBACK_SENDER)->SetWindowText(userName);
	}

	GetDlgItem(IDC_EDIT_FEEDBACK_FEATURE)->SetWindowText(m_featureName);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFeedbackFormDlg::OnBnClickedOk()
{
	std::vector<const char*> recps;
	std::vector<const char*> atts;
	CString body, sender, email, message;

	recps.push_back(FEEDBACK_EMAIL_ADDRESS);
	GetDlgItem(IDC_EDIT_FEEDBACK_SENDER)->GetWindowText(sender);
	GetDlgItem(IDC_EDIT_FEEDBACK_EMAIL)->GetWindowText(email);
	GetDlgItem(IDC_EDIT_FEEDBACK_DETAILS)->GetWindowText(message);
	body = "Sender: " + sender;
	body += "\r\nEmail: " + email;
	body += "\r\nDetails:\r\n\r\n" + message;
	CMailer::SendMail(
		(LPCTSTR)("Sandbox Feedback: " + m_featureName),
		body, recps, atts, IsDlgButtonChecked(IDC_CHECK_ALLOW_FURTHER_EDITING));
	CDialog::OnOK();
}
