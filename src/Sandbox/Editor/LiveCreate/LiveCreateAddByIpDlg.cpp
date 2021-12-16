#include "StdAfx.h"
#include "ILiveCreatePlatform.h"
#include "LiveCreateAddByIpDlg.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "EditorLiveCreateTasks.h"

#ifndef NO_LIVECREATE

IMPLEMENT_DYNAMIC(CLiveCreateAddByIpDlg, CDialog)

CLiveCreateAddByIpDlg::CLiveCreateAddByIpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLiveCreateAddByIpDlg::IDD, pParent)
{
}

CLiveCreateAddByIpDlg::~CLiveCreateAddByIpDlg()
{
}

void CLiveCreateAddByIpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TARGET_IPADDRESS, m_edIpAddress);
}

BEGIN_MESSAGE_MAP(CLiveCreateAddByIpDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CLiveCreateAddByIpDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_TEST_CONNECTION, &OnBnClickedButtonTestConnection)
END_MESSAGE_MAP()

BOOL CLiveCreateAddByIpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// return TRUE unless you set the focus to a control
	m_edIpAddress.SetFocus();
	return FALSE;
}

bool CLiveCreateAddByIpDlg::CheckValidIP() const
{
	CString str;
	m_edIpAddress.GetWindowText(str);

	int a=-1,b=-1,c=-1,d=-1;
	if (4 != sscanf((const char*)str, "%d.%d.%d.%d", &a, &b, &c, &d))
	{
		return false;
	}

	if (a < 0 || a > 255)
	{
		return false;
	}

	if (b < 0 || b > 255)
	{
		return false;
	}

	if (c < 0 || c > 255)
	{
		return false;
	}

	if (d < 0 || d > 255)
	{
		return false;
	}

	return true;
}

void CLiveCreateAddByIpDlg::OnBnClickedOk()
{
	if (!CheckValidIP())
	{
		MessageBox("Invalid IP entered", "Error", MB_ICONERROR | MB_OK);
		return;
	}

	m_edIpAddress.GetWindowText(m_ip);

	CDialog::OnOK();
}

void CLiveCreateAddByIpDlg::OnBnClickedButtonTestConnection()
{
	if (!CheckValidIP())
	{
		MessageBox("Invalid IP entered", "Error", MB_ICONERROR | MB_OK);
		return;
	}

	CString ipAddress;
	m_edIpAddress.GetWindowText(ipAddress);

	if (ipAddress == "0.0.0.0")
	{
		AfxMessageBox("Please provide a valid IP address.");
		return;
	}

	bool bIpValid = true;

	// check valid connection to IP
	{
		CString cmd;

		// use ping 2 times, 500ms apart
		cmd.Format("ping -n 2 -w 500 %s", ipAddress.GetBuffer());

		PROCESS_INFORMATION pi;
		STARTUPINFO si;

		ZeroMemory(&pi, sizeof(pi));
		ZeroMemory(&si, sizeof(si));

		si.cb = sizeof(STARTUPINFO);
		si.wShowWindow = SW_SHOW;

		if (CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			DWORD result = 0;

			WaitForSingleObject(pi.hProcess, INFINITE);

			if (!GetExitCodeProcess(pi.hProcess, &result))
			{
				result = 0;
			}

			if (result)
			{
				bIpValid = false;
			}

			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
		else
		{
			AfxMessageBox("Cannot run ping process!", MB_ICONERROR);
		}
	}

	if (!bIpValid)
	{
		AfxMessageBox("Could not reach the machine, please turn it ON, check the IP address or machine name and try again.", MB_ICONERROR);
	}
	else
	{
		AfxMessageBox("Success! The machine is present on the network.", MB_ICONINFORMATION);
	}
}

#endif