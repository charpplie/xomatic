#include "stdafx.h"
#include "DashboardDlg.h"
#include "afxdialogex.h"
#include "LevelFileDialog.h"
#include <IGameFramework.h>

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
}

BEGIN_MESSAGE_MAP(CDashboardDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CMD_OPEN_LEVEL, &CDashboardDlg::OnBnClickedCmdOpenLevel)
	ON_BN_CLICKED(IDC_CMD_CREATE_NEW_LEVEL, &CDashboardDlg::OnBnClickedCmdCreateNewLevel)
	ON_BN_CLICKED(IDC_CMD_DOCUMENTATION, &CDashboardDlg::OnBnClickedCmdDocumentation)
	ON_BN_CLICKED(IDC_CHECK_DONT_SHOW_AGAIN, &CDashboardDlg::OnBnClickedCheckDontShowAgain)
	ON_NOTIFY(NM_CLICK, IDC_LIST_RECENT_LEVELS, &CDashboardDlg::OnNMClickListRecentLevels)
	ON_NOTIFY(LVN_GETINFOTIP, IDC_LIST_RECENT_LEVELS, &CDashboardDlg::OnLvnRecentLevelsGetInfoTip)
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

	if (m_pRecentList)
	{
		CString gamePath;

		gamePath = Path::GetExecutableParentDirectory();
		Path::ConvertSlashToBackSlash(gamePath);
		gamePath.MakeLower();
		gamePath = Path::ToUnixPath(gamePath);
		gamePath = Path::AddSlash(gamePath);

		for (int i = 0; i < m_pRecentList->GetSize(); ++i)
		{
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

	m_listFont.CreatePointFont(100, "Arial");
	m_lstRecentLevels.SetFont(&m_listFont);

	if (CStatic* pPictureCtrl = (CStatic*)GetDlgItem(IDC_STATIC))
	{
		LPCTSTR bitmapResource = MAKEINTRESOURCE(gSettings.gui.bDarkSkin ? IDB_WELCOME_BANNER_DARK : IDB_WELCOME_BANNER_LIGHT);
		HBITMAP bitmap = ::LoadBitmap(AfxGetInstanceHandle(), bitmapResource);
		pPictureCtrl->SetBitmap(bitmap);
	}
	
	BringWindowToTop();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
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
	



	documentation = "http://doc.mycryengine.com";


	ShellExecute(0, 0, documentation.c_str(), 0 , 0, SW_SHOWNORMAL);
}

void CDashboardDlg::OnBnClickedCheckDontShowAgain()
{
	gSettings.bShowDashboardAtStartup = !IsDlgButtonChecked(IDC_CHECK_DONT_SHOW_AGAIN);
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
