#include "StdAfx.h"
#include "DashboardDlg.h"
#include "LevelFileDialog.h"
#include "Controls/DynamicPopupMenu.h"

//TODO: change to settings/settable in the future
const char* kFeedbackEmailAddress = "";

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CDashboardDlg, CDialogEx)

CDashboardDlg::CDashboardDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDashboardDlg::IDD, pParent)
{
	m_pRecentList = nullptr;
}

CDashboardDlg::~CDashboardDlg()
{
	m_listFont.DeleteObject();
}

void CDashboardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_RECENT_LEVELS, m_lstRecentLevels);
	DDX_Control(pDX, IDC_CMD_CREATE_NEW_LEVEL, m_newLevelButton);
	DDX_Control(pDX, IDC_CMD_DOCUMENTATION, m_openDocumentationButton);
	DDX_Control(pDX, IDC_CMD_OPEN_LEVEL, m_openLevelButton);
	DDX_Control(pDX, IDOK, m_closeButton);
}

BEGIN_MESSAGE_MAP(CDashboardDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CMD_OPEN_LEVEL, &CDashboardDlg::OnBnClickedCmdOpenLevel)
	ON_BN_CLICKED(IDC_CMD_CREATE_NEW_LEVEL, &CDashboardDlg::OnBnClickedCmdCreateNewLevel)
	ON_BN_CLICKED(IDC_CMD_DOCUMENTATION, &CDashboardDlg::OnBnClickedCmdDocumentation)
	ON_BN_CLICKED(IDC_CHECK_DONT_SHOW_AGAIN, &CDashboardDlg::OnBnClickedCheckDontShowAgain)
	ON_NOTIFY(NM_CLICK, IDC_LIST_RECENT_LEVELS, &CDashboardDlg::OnNMClickListRecentLevels)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_RECENT_LEVELS, &CDashboardDlg::OnLvnRecentLevelsGetInfoTip)
	ON_BN_CLICKED(IDC_CHECK_AUTOLOAD_LAST_LEVEL, &CDashboardDlg::OnBnClickedCheckAutoloadLastLevel)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_RECENT_LEVELS, &CDashboardDlg::OnNMRClickListRecentLevels)
END_MESSAGE_MAP()


// CDashboardDlg message handlers


BOOL CDashboardDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_lstRecentLevels.SetExtendedStyle(
		LVS_EX_FLATSB
		|LVS_EX_INFOTIP
		|LVS_EX_LABELTIP
		|LVS_EX_SINGLEROW
		|LVS_EX_GRIDLINES
		|LVS_EX_UNDERLINEHOT
		|LVS_EX_ONECLICKACTIVATE);
	
	CRect rc;
	
	m_lstRecentLevels.GetClientRect(rc);
	m_lstRecentLevels.InsertColumn(0, "", LVCFMT_LEFT, rc.Width());
	m_listFont.CreatePointFont(100, "Arial");
	m_lstRecentLevels.SetFont(&m_listFont);
	CheckDlgButton(IDC_CHECK_AUTOLOAD_LAST_LEVEL, gSettings.bAutoloadLastLevelAtStartup);
	BringWindowToTop();
	FillRecentLevels();

	if (CStatic* pPictureCtrl = (CStatic*)GetDlgItem(IDC_STATIC))
	{
		LPCTSTR bitmapResource = MAKEINTRESOURCE(gSettings.gui.bDarkSkin ? IDB_WELCOME_BANNER_DARK : IDB_WELCOME_BANNER_LIGHT);
		HBITMAP bitmap = ::LoadBitmap(AfxGetInstanceHandle(), bitmapResource);
		pPictureCtrl->SetBitmap(bitmap);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDashboardDlg::FillRecentLevels()
{
	if (m_pRecentList)
	{
		CString gamePath;

		gamePath = Path::GetExecutableParentDirectory() + "\\" + gEnv->pConsole->GetCVar("sys_game_folder")->GetString();
		Path::ConvertSlashToBackSlash(gamePath);
		gamePath.MakeLower();
		gamePath = Path::ToUnixPath(gamePath);
		gamePath = Path::AddSlash(gamePath);

		for (int i = 0; i < m_pRecentList->GetSize(); ++i)
		{
			if (!CFileUtil::Exists(m_pRecentList->m_arrNames[i], false))
			{
				m_pRecentList->Remove(i);
				continue;
			}

			CString fullPath = m_pRecentList->m_arrNames[i];
			CString name = Path::GetFileName(fullPath);

			Path::ConvertSlashToBackSlash(fullPath);
			fullPath.MakeLower();
			fullPath = Path::ToUnixPath(fullPath);
			fullPath = Path::AddSlash(fullPath);

			if (-1 != fullPath.Find(gamePath, 0))
			{
				m_lstRecentLevels.InsertItem(i, name);
				m_levels.push_back(std::make_pair(name, m_pRecentList->m_arrNames[i]));
			}
		}
	}
}

const CString& CDashboardDlg::GetLevelPath()
{
	return m_levelPath;
}

void CDashboardDlg::SetRecentFileList(CRecentFileList* pList)
{
	m_pRecentList = pList;
}

void CDashboardDlg::OnBnClickedCmdOpenLevel()
{
	CLevelFileDialog dlg(true);

	if (dlg.DoModal() == IDOK)
	{
		m_levelPath = dlg.GetFileName();
		EndDialog(IDOK);
	}
}

void CDashboardDlg::OnBnClickedCmdCreateNewLevel()
{
	m_levelPath = "new";
	EndDialog(IDOK);
}

void CDashboardDlg::OnBnClickedCmdDocumentation()
{
	string documentation;
	documentation = "http://docs.cryengine.com";
	ShellExecute(0, 0, documentation.c_str(), 0 , 0, SW_SHOWNORMAL);
}

void CDashboardDlg::OnBnClickedCheckDontShowAgain()
{
	gSettings.bShowDashboardAtStartup = !IsDlgButtonChecked(IDC_CHECK_DONT_SHOW_AGAIN);
	gSettings.Save();
}

void CDashboardDlg::OnNMClickListRecentLevels(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	int index = pNMItemActivate->iItem;

	if (index >= 0 && index < m_levels.size())
	{
		m_levelPath = m_levels[index].second;
		EndDialog(IDOK);
	}
}

void CDashboardDlg::OnLvnRecentLevelsGetInfoTip(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVGETINFOTIP* pInfoTipNotify = (NMLVGETINFOTIP*)pNMHDR;
	int index = pInfoTipNotify->iItem;
	*pResult = 0;

	if (index >= 0 && index < m_levels.size())
	{
		const CString& fullPath = m_levels[index].second;
		strcpy_s(pInfoTipNotify->pszText, pInfoTipNotify->cchTextMax, fullPath);
	}
}

void CDashboardDlg::OnBnClickedCheckAutoloadLastLevel()
{
	gSettings.bAutoloadLastLevelAtStartup = IsDlgButtonChecked(IDC_CHECK_AUTOLOAD_LAST_LEVEL);
	gSettings.Save();
}

void CDashboardDlg::RemoveLevelEntry(int index)
{
	TNamePathPair levelPath = m_levels[index];

	m_levels.erase(m_levels.begin() + index);
	m_lstRecentLevels.DeleteItem(index);

	if (!m_pRecentList)
		return;

	for (int i = 0; i < m_pRecentList->GetSize(); ++i)
	{
		CString fullPath = m_pRecentList->m_arrNames[i];
		CString fullPath2 = levelPath.second;
		
		// path from recent list
		Path::ConvertSlashToBackSlash(fullPath);
		fullPath.MakeLower();
		fullPath = Path::ToUnixPath(fullPath);
		fullPath = Path::AddBackslash(fullPath);
		
		// path from our dashboard list
		Path::ConvertSlashToBackSlash(fullPath2);
		fullPath2.MakeLower();
		fullPath2 = Path::ToUnixPath(fullPath2);
		fullPath2 = Path::AddBackslash(fullPath2);

		if (fullPath == fullPath2)
		{
			m_pRecentList->Remove(index);
			break;
		}
	}

	m_pRecentList->WriteList();
}

void CDashboardDlg::OnNMRClickListRecentLevels(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	int index = pNMItemActivate->iItem;

	if (index >= 0 && index < m_levels.size())
	{
		CDynamicPopupMenu menu;
		CPopupMenuItem& root = menu.GetRoot();

		root.Add<int>( CString("Remove '") + m_levels[index].first + "' from list", functor(*this, &CDashboardDlg::RemoveLevelEntry), index);
		menu.Spawn(GetSafeHwnd());
	}
}
