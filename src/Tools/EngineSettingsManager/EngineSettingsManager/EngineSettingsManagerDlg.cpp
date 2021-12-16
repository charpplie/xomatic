#include "stdafx.h"
#include "EngineSettingsManagerApp.h"
#include "EngineSettingsManagerDlg.h"


#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////////
CEngineSettingsManagerDlg::CEngineSettingsManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEngineSettingsManagerDlg::IDD, pParent)
{
	m_hIcon = NULL;//AfxGetApp()->LoadIcon(IDI_CE2_ICON);

	char* moduleName = NULL;
	char* moduleFileName = NULL;

	if (__argc>2)
	{
		moduleName = __argv[1];
		moduleFileName = __argv[2];
	}

	m_settingsManager = new CEngineSettingsManager(moduleName, moduleFileName);
}

//////////////////////////////////////////////////////////////////////////
CEngineSettingsManagerDlg::~CEngineSettingsManagerDlg()
{
	if (m_settingsManager)
	{
		delete m_settingsManager;
		m_settingsManager = 0;
	}
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGS_TAB, m_tcSettings);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CEngineSettingsManagerDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGING, IDC_SETTINGS_TAB, &OnSelchangingTab)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SETTINGS_TAB, &OnSelchangeTab)
	ON_COMMAND(IDOK, OnOkButtonClicked)
	ON_WM_CREATE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManagerDlg::LoadCrySkin()
{

	CXTPPaintManager::SetTheme(xtpThemeNativeWinXP);
	BOOL bLoaded = XTPSkinManager()->LoadSkin( "..\\Editor\\Styles\\CryDark.cjstyles" );

	// Apply skin, including metrics, coloring and frame
	XTPSkinManager()->SetApplyOptions(xtpSkinApplyMetrics|xtpSkinApplyColors|xtpSkinApplyFrame|xtpSkinApplyMenus);

	// Apply skin to all new windows and threads

	XTPSkinManager()->SetAutoApplyNewWindows(TRUE);

	XTPSkinManager()->SetAutoApplyNewThreads(TRUE);

	// Apply skin to current thread and window

	XTPSkinManager()->EnableCurrentThread();

	XTPSkinManager()->ApplyWindow(this->GetSafeHwnd());

	bool test = XTPSkinManager()->IsColorFilterExists();
	// Redraw everything

	XTPSkinManager()->RedrawAllControls();

	OnSkinChanged();
}

//////////////////////////////////////////////////////////////////////////
int CEngineSettingsManagerDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LoadCrySkin();
	return 0;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManagerDlg::OnSkinChanged()
{
	OnSysColorChange();
	RedrawWindow(0, 0, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN);		

	XTPPaintManager()->RefreshMetrics();

}
//////////////////////////////////////////////////////////////////////////
BOOL CEngineSettingsManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_settingsManager->SetParentDialog((unsigned long)m_hWnd);


	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_tcSettings.Create(m_settingsManager);
	
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManagerDlg::OnSelchangingTab(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_tcSettings.DeactivateTabDialog(m_tcSettings.GetCurSel());
	*pResult = 0;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManagerDlg::OnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_tcSettings.ActivateTabDialog(m_tcSettings.GetCurSel());
	*pResult = 0;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManagerDlg::OnOkButtonClicked()
{
	if (m_tcSettings.PollSettings(m_settingsManager))
	{
		m_tcSettings.SaveSettingsToSystemCfg();
		m_settingsManager->StoreData();
	}

	PostQuitMessage(0);
}
