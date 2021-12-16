#include "stdafx.h"
#include "AboutDlg.h"

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_VERSIONLABEL, m_versionLabel);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	TCHAR vers[64];
	LoadString(NULL, IDS_VERSION, vers, 64);

	TCHAR label[128];
	_stprintf_s(label, 128, "Version %s", vers);
	m_versionLabel.SetWindowText(label);

	return result;
}
