#include "StdAfx.h"
#include "ILiveCreatePlatform.h"
#include "LiveCreateEditConnectionDlg.h"
#include "LiveCreateFilePickerDlg.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "LiveCreate/LiveCreateTaskWaitDlg.h"
#include "EditorLiveCreateTasks.h"

#ifndef NO_LIVECREATE

IMPLEMENT_DYNAMIC(CLiveCreateEditConnectionDlg, CDialog)

CLiveCreateEditConnectionDlg::CLiveCreateEditConnectionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLiveCreateEditConnectionDlg::IDD, pParent)
	, m_pFactory(NULL)
	, m_pPlatform(NULL)
{
	m_params.bAdding = false;
}

CLiveCreateEditConnectionDlg::~CLiveCreateEditConnectionDlg()
{
	if (NULL != m_pPlatform)
	{
		m_pPlatform->Release();
		m_pPlatform = NULL;
	}
}

void CLiveCreateEditConnectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_TARGET_NAME, m_edTargetName);
	DDX_Control(pDX, IDC_TARGET_IPADDRESS, m_edIpAddress);
	DDX_Control(pDX, IDC_COMBO_PLATFORM, m_cbPlatform);
	DDX_Control(pDX, IDC_EDIT_BUILD_EXECUTABLE, m_edBuildExecutable);
	DDX_Control(pDX, IDC_EDIT_BUILD_ROOT_PATH, m_edBuildPath);
	DDX_Control(pDX, IDC_BUTTON_REFRESH_IP, m_btnRefreshIP);
	DDX_Control(pDX, IDC_BUTTON_PICK_GAME_DIRECTORY, m_btnPickDirectory);
	DDX_Control(pDX, IDC_CHECK_ENABLED, m_chkEnabled);
}

BEGIN_MESSAGE_MAP(CLiveCreateEditConnectionDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CLiveCreateEditConnectionDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_TEST_CONNECTION, &OnBnClickedButtonTestConnection)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH_IP, &OnBnClickedButtonRefreshIp)
	ON_BN_CLICKED(IDC_BUTTON_PICK_GAME_DIRECTORY, &OnBnBrowseGameFolder)
	ON_CBN_SELCHANGE(IDC_COMBO_PLATFORM, &OnCbnSelchangeComboPlatform)
	ON_BN_CLICKED(IDC_CHECK_ENABLED, &OnBnClickedCheckEnableHost)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CLiveCreateEditConnectionDlg::SetParameters(const Parameters& rParams)
{
	m_params = rParams;
}

const CLiveCreateEditConnectionDlg::Parameters& CLiveCreateEditConnectionDlg::GetParameters()
{
	return m_params;
}

BOOL CLiveCreateEditConnectionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// use existing platform interface if given
	m_pPlatform = m_params.pPlatform;
	if (NULL != m_pPlatform)
	{
		m_pPlatform->AddRef();
	}

	// copy settings to dialog
	m_edTargetName.SetWindowText(m_params.targetName);
	m_edIpAddress.SetWindowText(m_params.address);
	m_chkEnabled.SetCheck(m_params.bIsEnabled || m_params.bAdding);

	// we don't need some settings when editing existing host
	if (m_params.bAdding)
	{
		SetWindowText("Add new host");
	}
	else
	{
		SetWindowText("Edit host");
		m_cbPlatform.EnableWindow(FALSE);
		m_edTargetName.EnableWindow(FALSE);
	}

	// list possible platforms, select current one
	const uint numPlatforms = gEnv->pLiveCreateManager->GetNumPlatforms();
	for (int i = 0; i < numPlatforms; ++i)
	{
		// add to list of platforms
		const char* platformName = gEnv->pLiveCreateManager->GetPlatformFactory(i)->GetPlatformName();

		// do not add the "Any" platform here - it's reserved for the "Add by IP option"
		if (0 == stricmp(platformName, "Any"))
		{
			continue;
		}

		m_cbPlatform.AddString(platformName);

		if (!m_params.bAdding && m_params.platformName == platformName)
		{
			m_cbPlatform.SetCurSel(i);
			m_pFactory = gEnv->pLiveCreateManager->GetPlatformFactory(i);
		}
	}

	// Special case for "Any" platform
	if (0 == stricmp(m_params.platformName, "Any"))
	{
		m_cbPlatform.ResetContent();
		m_cbPlatform.AddString("(Unknown)");
		m_cbPlatform.SetCurSel(0);
		m_cbPlatform.EnableWindow(false);
	}

	// no platform selected
	if (NULL == m_pFactory)
	{
		m_pFactory = gEnv->pLiveCreateManager->GetPlatformFactory(0);
		m_cbPlatform.SetCurSel(0);
	}

	// create the platform handler
	if (!m_params.bAdding)
	{
		m_pPlatform = m_pFactory->CreatePlatformHandlerInstance(m_params.targetName);
	}

	UpdateBuildControls();

	 // return TRUE unless you set the focus to a control
	return TRUE; 
}

void CLiveCreateEditConnectionDlg::UpdateBuildControls()
{
	if (m_pPlatform == NULL)
	{
		if (m_params.bAdding)
		{
			m_edBuildExecutable.SetWindowText("(Please specify valid target name and resovle IP first)");
			m_edBuildPath.SetWindowText("(Please specify valid target name and resovle IP first)");
		}
		else
		{
			m_edBuildExecutable.SetWindowText(m_params.buildExecutable);
			m_edBuildPath.SetWindowText(m_params.buildDirectory);
		}

		m_edBuildExecutable.EnableWindow(false);
		m_edBuildPath.EnableWindow(false);
		m_btnPickDirectory.EnableWindow(false);
	}
	else
	{
		m_edBuildExecutable.SetWindowText(m_params.buildExecutable);
		m_edBuildPath.SetWindowText(m_params.buildDirectory);
		m_edBuildExecutable.EnableWindow(true);
		m_edBuildPath.EnableWindow(true);
		m_btnPickDirectory.EnableWindow(true);
	}
}

void CLiveCreateEditConnectionDlg::OnBnClickedOk()
{
	CString fullExePath;
	
	m_cbPlatform.GetWindowText(m_params.platformName);
	m_edTargetName.GetWindowText(m_params.targetName);
	m_edIpAddress.GetWindowText(m_params.address);
	m_cbPlatform.GetWindowText(m_params.platformName);
	m_edBuildExecutable.GetWindowText(m_params.buildExecutable);
	m_edBuildPath.GetWindowText(m_params.buildDirectory);

	CDialog::OnOK();
}

void CLiveCreateEditConnectionDlg::OnBnClickedButtonTestConnection()
{
	CString targetName, ipAddress, platformName;

	m_edTargetName.GetWindowText(targetName);
	m_edIpAddress.GetWindowText(ipAddress);
	m_cbPlatform.GetWindowText(platformName);

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


void CLiveCreateEditConnectionDlg::OnBnClickedButtonRefreshIp()
{
	CString strName;

	m_edTargetName.GetWindowText(strName);

	if (strName.IsEmpty())
	{
		MessageBoxA("Please provide a Name for the target.", "Error", MB_ICONEXCLAMATION);
		return;
	}

	// start address resolving task
	LiveCreate::CBGTask_ResolveAddress* pTask = new LiveCreate::CBGTask_ResolveAddress(m_pFactory, strName);
	pTask->AddRef();
	GetIEditor()->GetBackgroundTaskManager()->AddTask(pTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);

	CString messageText;
	messageText.Format("Resolving address of '%s'", (const char*)strName);

	// wait for the task to finish
	CLiveCreateTaskWaitDlg::ShowDialog(this, pTask, "Resolving target address", messageText);
	if (pTask->HasFinishedWithoutError())
	{
		m_edIpAddress.SetWindowText(pTask->GetResolvedAddress());
		m_edIpAddress.Invalidate(TRUE);

		// recreate the platform handler
		SAFE_RELEASE(m_pPlatform);

		m_pPlatform = m_pFactory->CreatePlatformHandlerInstance(strName);
		UpdateBuildControls();
	}
	else
	{
		MessageBoxA("Unable to resolve target IP address. Make sure console is ON and connected to the network.", "Error", MB_ICONEXCLAMATION);
	}

	pTask->Release();
}

void CLiveCreateEditConnectionDlg::OnCbnSelchangeComboPlatform()
{
	// update platform name
	m_cbPlatform.GetWindowText(m_params.platformName);

	// search factory
	const uint numPlatforms = gEnv->pLiveCreateManager->GetNumPlatforms();
	for (uint i = 0; i < numPlatforms; ++i)
	{
		const char* platformName = gEnv->pLiveCreateManager->GetPlatformFactory(i)->GetPlatformName();
		if (0 == m_params.platformName.Compare(platformName))
		{
			m_pFactory = gEnv->pLiveCreateManager->GetPlatformFactory(i);
			break;
		}
	}
	
}

void CLiveCreateEditConnectionDlg::OnBnClickedCheckEnableHost()
{
	m_params.bIsEnabled = (m_chkEnabled.GetCheck() != 0);
}


void CLiveCreateEditConnectionDlg::OnBnBrowseGameFolder()
{
	if (NULL == m_pPlatform)
	{
		MessageBoxA("Unable to explore target directories. Please make sure target is available.", "Error", MB_ICONEXCLAMATION);
		return;
	}

	CLiveCreateFilePickerDlg dlg(this);
	dlg.Setup(m_pPlatform, m_params.buildExecutable);

	if (dlg.DoModal() == IDCANCEL)
		return;

	// update build settings
	m_params.buildDirectory = dlg.GetSelectedDirectory();
	m_params.buildExecutable = dlg.GetSelectedDirectory() + dlg.GetSelectedExecutable();

	// update dialog windows
	m_edBuildPath.SetWindowText(m_params.buildDirectory);
	m_edBuildExecutable.SetWindowText(m_params.buildExecutable);	
}

#endif