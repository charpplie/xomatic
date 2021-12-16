//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : QuickAccessBar.cpp
//  Author           : Jaewon Jung
//  Time of creation : 8/30/2011   14:20
//  Compilers        : VS2008
//  Description      : A dialog bar for quickly accessing menu items and cvars
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "QuickAccessBar.h"
#include "ToolBox.h"

IMPLEMENT_DYNCREATE(CQuickAccessBar, CDialog)

BEGIN_MESSAGE_MAP(CQuickAccessBar, CDialog)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

CQuickAccessBar::CQuickAccessBar(CWnd* pParent)
: CDialog(CQuickAccessBar::IDD, pParent)
{
}

void CQuickAccessBar::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_QUICK_ACCESS_BAR_EDIT, m_inputEdit);
}

BOOL CQuickAccessBar::OnInitDialog()
{
	__super::OnInitDialog();

	m_inputEdit.Init();
	m_inputEdit.SetMode(_MODE_STANDARD_|_MODE_HISTORY_|_MODE_FIND_ALL_);

	// Set WS_EX_LAYERED on this window .
	SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE,
		GetWindowLong(GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);
	// Make this window 50% alpha.
	SetLayeredWindowAttributes(0, 128, LWA_ALPHA);

	CXTPMenuBar *pMenuBar
		= static_cast<CMainFrame*>(AfxGetMainWnd())->GetCommandBars()->GetMenuBar();
	CollectMenuItems(pMenuBar, "");

	AddMRUFileItems();

	// Add console variables & commands.
	IConsole *console = GetIEditor()->GetSystem()->GetIConsole();
	std::vector<const char*> cmds;
	cmds.resize(console->GetNumVars());
	size_t cmdCount = console->GetSortedVars(&cmds[0], cmds.size());
	for(int i=0; i<cmdCount; ++i)
	{
		m_inputEdit.AddSearchString(cmds[i]);
	}

	return TRUE;
}

void CQuickAccessBar::OnOK()
{
	CString menuCmd;
	m_inputEdit.GetWindowText(menuCmd);
	if(menuCmd.IsEmpty() == false)
	{
		std::map<CString, int>::const_iterator itr = m_menuIdTable.find(menuCmd);
		if(itr != m_menuIdTable.end())
		{
			AfxGetMainWnd()->SendMessage(WM_COMMAND, MAKEWPARAM(itr->second,0), 0);
		}
		else
		{
			GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(menuCmd);
		}

		m_inputEdit.AddHistoryString(menuCmd);
		m_inputEdit.SetWindowText("");
	}

	ShowWindow(SW_HIDE);
}

void CQuickAccessBar::OnCancel()
{
	ShowWindow(SW_HIDE);
}

void CQuickAccessBar::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	if(nState == WA_INACTIVE)
		OnCancel();
}

void CQuickAccessBar::CollectMenuItems(const CXTPCommandBar *pCmdBar, const CString& menuPath)
{
	if (!pCmdBar)
	{
		ASSERT(pCmdBar);
		return;
	}

	for(int i=0; i<pCmdBar->GetControlCount(); ++i)
	{
		CXTPControl *pControl = pCmdBar->GetControl(i);
		if (!pControl)
			continue;

		CString menuString = pControl->GetCaption();
		menuString.Replace("&", "");
		menuString.Replace(' ', '_');
		CString newMenuPath = menuPath;
		if(newMenuPath.IsEmpty() == false)
			newMenuPath += ".";
		newMenuPath += menuString;

		if(pControl->GetType() == xtpControlPopup)					// A submenu
		{
			if (pControl->GetCommandBar())
				CollectMenuItems(pControl->GetCommandBar(), newMenuPath);

			if(pControl->GetID() == ID_VIEW_OPENVIEWPANE)	// A special handling for the dynamic 'Open View Pane' menu
			{
				CollectDynamicOpenViewPaneItems(newMenuPath);
			}
		}
		else if(pControl->GetType() == xtpControlButton)		// A menu item
		{
			int itemID = pControl->GetID();
			if(itemID > 0)
			{
				m_inputEdit.AddSearchString(newMenuPath);
				m_menuIdTable[newMenuPath] = itemID;			
			}
		}
	}
}

void CQuickAccessBar::CollectDynamicOpenViewPaneItems(const CString& newMenuPath)
{
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_VIEWPANE, classes);

	for(size_t k=0; k<classes.size(); ++k)
	{
		IClassDesc *pClass = classes[k];
		IViewPaneClass *pViewClass = NULL;
		if(SUCCEEDED(pClass->QueryInterface(__uuidof(IViewPaneClass), (void**)&pViewClass)))
		{
			CString paneTitle = pViewClass->GetPaneTitle();
			paneTitle.Replace(' ', '_');
			CString finalItemText = newMenuPath;
			finalItemText += ".";
			finalItemText += paneTitle;
			m_inputEdit.AddSearchString(finalItemText);
			m_menuIdTable[finalItemText] = ID_VIEW_OPENPANE_FIRST + k;
		}
	}
}

void CQuickAccessBar::AddMRUFileItems()
{
	CRecentFileList *pMRUList = static_cast<CCryEditApp*>(AfxGetApp())->GetRecentFileList();

	// someone may set HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\NoRecentDocsHistory to 1, thus pMRUList is a nullptr
	if (!pMRUList)
		return;

	for(int i=0; i<pMRUList->GetSize(); ++i)
	{
		CString mruText;
		pMRUList->GetDisplayName(mruText, i, "", 0);
		if(mruText.IsEmpty())
			continue;
		mruText.Replace(' ', '_');
		m_inputEdit.AddSearchString(mruText);
		m_menuIdTable[mruText] = ID_FILE_MRU_FILE1 + i;
	}
}