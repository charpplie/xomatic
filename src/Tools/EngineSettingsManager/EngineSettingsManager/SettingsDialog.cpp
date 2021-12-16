#include "stdafx.h"
#include "EngineSettingsManager.h"
#include "SettingsDialog.h"


#include "Utils.h"
#include "ResourceCompilerHelper.h"


IMPLEMENT_DYNAMIC(CSettingsDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CSettingsDialog::CSettingsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDialog::IDD, pParent)
	, m_bRCFound(false)
{
}


//////////////////////////////////////////////////////////////////////////
void CSettingsDialog::Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager)
{
	m_settingsManager = settingsManager;
	CDialog::Create(temp, parent);
}


//////////////////////////////////////////////////////////////////////////
void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ROOT_PATH, m_eRootPath);
	DDX_Control(pDX, IDC_RC_STATE, m_imgRCState);
	DDX_Control(pDX, IDC_RC_STATE_LABEL, m_lRCState);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CSettingsDialog, CDialog)
	ON_BN_CLICKED(IDC_BROWSE_BUTTON,&OnBrowseButtonClicked)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
BOOL CSettingsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_eRootPath.SetWindowText(m_settingsManager->GetRootPath().c_str());	
	UpdateEngineStateView();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
void CSettingsDialog::OnBrowseButtonClicked()
{
	LPTSTR szPath[MAX_PATH];

	if (CUtils::BrowseForFolder(m_hWnd, "C:\\",(LPTSTR)&szPath, "Please select the installation directory of CryENGINE."))
	{
		CString enginePath = (LPCTSTR)&szPath;
		int len = enginePath.GetLength();
		if (len>0 && (enginePath.GetBuffer()[len-1]=='\\' || enginePath.GetBuffer()[len-1]=='/'))
			enginePath.Truncate(len-1);

		m_eRootPath.SetWindowText(enginePath);
		
		UpdateEngineStateView();
	}
}


//////////////////////////////////////////////////////////////////////////
void CSettingsDialog::UpdateEngineStateView()
{
	CResourceCompilerHelper rch;

	CString text;
	m_eRootPath.GetWindowText(text);
	rch.GetSettingsManager()->SetKey(_T("ENG_RootPath"), text.GetBuffer());
	m_bRCFound = 
		(rch.CallResourceCompiler(NULL, NULL, NULL, false, false, CResourceCompilerHelper::eRcExePath_settingsManager, true, true, NULL) 
		== rch.eRcCallResult_success);

	if (m_bRCFound)
	{
		HICON icon = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_GREEN_ICON), IMAGE_ICON, 0, 0, 0);
		m_imgRCState.SetIcon(icon);
		m_lRCState.SetWindowText("rc.exe found!");
	}
	else
	{
		HICON icon = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_RED_ICON), IMAGE_ICON, 0, 0, 0);
		m_imgRCState.SetIcon(icon);
		m_lRCState.SetWindowText("missing rc.exe!");
	}
}


//////////////////////////////////////////////////////////////////////////
CString CSettingsDialog::GetRootPath() const
{
	CString s;
	m_eRootPath.GetWindowText(s);
	return s;
}

bool CSettingsDialog::IsRootPathGood() const
{
	return m_bRCFound;
}
