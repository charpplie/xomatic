// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "MemReplay.h"
#include "MainFrm.h"
#include "MemReplayView.h"
#include "MemReplayDoc.h"

#include "ITabPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static UINT WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);

static void DockBarNextTo(CControlBar* pBarToDock, CControlBar* pDockedBar, UINT nDockBarID)
{
	CFrameWnd *pFrameWnd = pDockedBar->GetDockingFrame ();
	pFrameWnd->RecalcLayout();

	CRect rect;
	pDockedBar->GetWindowRect(&rect);

	rect.OffsetRect (1, 1);
	pFrameWnd->DockControlBar(pBarToDock, nDockBarID, &rect);
}

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindReplace)
	ON_UPDATE_COMMAND_UI(ID_MAIN, OnMainUI)
	ON_UPDATE_COMMAND_UI(ID_RSX, OnRSXUI)
	ON_UPDATE_COMMAND_UI(ID_COMBINED, OnCombinedUI)
	ON_UPDATE_COMMAND_UI(ID_COUNT, OnCountUI)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
	: m_findDlg(NULL)
	, m_doTypeLookup(false)
	, m_activeToolBar(-1)
{
	// TODO: add member initialization code here
}

void CMainFrame::SetActiveToolBar(CMainFrame::ToolBar tb)
{
	if (m_activeToolBar != -1)
		m_wndToolBars[m_activeToolBar].ShowWindow(SW_HIDE);

	if (tb != -1)
	{
		ShowControlBar(&m_wndToolBars[tb], TRUE, FALSE);
	//	DockBarNextTo (&m_wndToolBars[tb], &m_wndToolBarCommon, AFX_IDW_DOCKBAR_TOP);
	}

	m_activeToolBar = tb;
}

void CMainFrame::StartFind()
{
	if (m_findDlg == NULL)
	{
		m_findDlg = new CFindReplaceDialog();
		m_findDlg->Create(TRUE, "");
	}

	m_findDlg->ShowWindow(SW_SHOW);
	m_doTypeLookup=false;
}

void CMainFrame::StartTypeInfoLookup()
{
	if (m_findDlg == NULL)
	{
		m_findDlg = new CFindReplaceDialog();
		m_findDlg->Create(TRUE, "");
	}

	m_findDlg->ShowWindow(SW_SHOW);
	m_doTypeLookup=true;
}

void CMainFrame::SetStatus(const TCHAR* statusText)
{
	if (m_wndStatusBar.m_hWnd)
	{
		int numTabs = 0;
		for (const TCHAR* st = statusText; *st; ++ st)
		{
			if (*st == _T('\t'))
				++ numTabs;
		}

		CStatusBarCtrl& sb = m_wndStatusBar.GetStatusBarCtrl();

		int numParts = sb.GetParts(0, NULL);
		if (numParts != numTabs)
		{
			std::vector<int> widths;
			widths.reserve(numTabs + 1);
			for (int i = 0; i < numTabs; ++ i)
				widths.push_back(150 * (i + 1));
			widths.push_back(-1);

			for (int i = numTabs; i < numParts; ++ i)
				sb.SetText(_T(""), i, 0);

			sb.SetParts(numTabs + 1, &widths[0]);
		}

		int pane = 0;

		for (const TCHAR* st = statusText; *st; ++ pane)
		{
			int length = 0;
			
			while (st[length] && st[length] != _T('\t'))
			{
				++ length;
			}

			TCHAR buf[256];
			_tcsncpy(buf, st, std::min(255, length));
			buf[std::min(255, length)] = 0;
			st += length + (st[length] != 0);

			sb.SetText(buf, pane, 0);
		}
	}
}

void CMainFrame::UpdateToolbar()
{
	m_wndToolBarCommon.OnUpdateCmdUI(this, FALSE);
	m_wndToolBarCommon.UpdateWindow();

	if (m_activeToolBar >= 0)
	{
		m_wndToolBars[m_activeToolBar].OnUpdateCmdUI(this, FALSE);
		m_wndToolBars[m_activeToolBar].UpdateWindow();
	}
}

CMainFrame::~CMainFrame()
{
	delete m_findDlg;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (
		!m_wndToolBarCommon.Create(this) || !m_wndToolBarCommon.LoadToolBar(IDR_TOOLBAR_COMMON) ||
		!m_wndToolBars[TB_Bucket].Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, 1000) || !m_wndToolBars[TB_Bucket].LoadToolBar(IDR_TOOLBAR_BUCKET) ||
		!m_wndToolBars[TB_Code].Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, 1001) || !m_wndToolBars[TB_Code].LoadToolBar(IDR_TOOLBAR_CODE) ||
		!m_wndToolBars[TB_Context].Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, 1002) || !m_wndToolBars[TB_Context].LoadToolBar(IDR_TOOLBAR_CONTEXT) ||
		!m_wndToolBars[TB_Usage].Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, 1003) || !m_wndToolBars[TB_Usage].LoadToolBar(IDR_TOOLBAR_USAGE))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndToolBarCommon.SetBarStyle(m_wndToolBarCommon.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBarCommon.EnableDocking(CBRS_ALIGN_ANY);
	for (int tb = 0; tb != TB_Count; ++ tb)
	{
		m_wndToolBars[tb].SetBarStyle(m_wndToolBars[tb].GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
		m_wndToolBars[tb].EnableDocking(CBRS_ALIGN_ANY);
	}

	EnableDocking(CBRS_ALIGN_ANY);

	DockControlBar(&m_wndToolBarCommon, AFX_IDW_DOCKBAR_TOP);

	for (int tb = 0; tb != TB_Count; ++ tb)
		DockBarNextTo (&m_wndToolBars[tb], &m_wndToolBarCommon, AFX_IDW_DOCKBAR_TOP);

	m_wndToolBars[TB_Code].SetButtonStyle(0, m_wndToolBars[TB_Code].GetButtonStyle(0) | TBBS_CHECKGROUP);
	m_wndToolBars[TB_Code].SetButtonStyle(1, m_wndToolBars[TB_Code].GetButtonStyle(1) | TBBS_CHECKGROUP);
	m_wndToolBars[TB_Code].SetButtonStyle(2, m_wndToolBars[TB_Code].GetButtonStyle(2) | TBBS_CHECKGROUP);
	m_wndToolBars[TB_Code].SetButtonStyle(4, m_wndToolBars[TB_Code].GetButtonStyle(4) | TBBS_CHECKBOX);

	m_wndToolBars[TB_Context].SetButtonStyle(0, m_wndToolBars[TB_Context].GetButtonStyle(0) | TBBS_CHECKGROUP);
	m_wndToolBars[TB_Context].SetButtonStyle(1, m_wndToolBars[TB_Context].GetButtonStyle(1) | TBBS_CHECKGROUP);
	m_wndToolBars[TB_Context].SetButtonStyle(2, m_wndToolBars[TB_Context].GetButtonStyle(2) | TBBS_CHECKGROUP);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.SetPaneInfo(0, 0, SBPS_STRETCH, 100);

	for (int tb = 0; tb != TB_Count; ++ tb)
		ShowControlBar(&m_wndToolBars[tb], FALSE, FALSE);

	return 0;
}

void CMainFrame::OnClose()
{
	if (m_findDlg)
	{
		m_findDlg->DestroyWindow();
		m_findDlg = NULL;
	}

	CFrameWnd::OnClose();
}

LRESULT CMainFrame::OnFindReplace(WPARAM wParam, LPARAM lParam)
{
	LPFINDREPLACE fr = reinterpret_cast<LPFINDREPLACE>(lParam);

	if (fr->Flags & FR_DIALOGTERM)
	{
		m_findDlg = NULL;
	}
	else if (fr->Flags & FR_FINDNEXT)
	{
		CMemReplayView* view = static_cast<CMemReplayView*>(GetActiveView());
		if (view)
		{
			if (m_doTypeLookup)
			{
				view->OpenTypeInfoViewFor(fr->lpstrFindWhat);
			}
			else
			{
				ITabPage* page = view->GetActiveTab();
				if (page)
				{
					page->OnFindNext(fr->lpstrFindWhat, 
						((fr->Flags & FR_DOWN) ? ITabPage::FF_Forward : 0) | 
						((fr->Flags & FR_MATCHCASE) ? 0 : ITabPage::FF_CaseInsensitive));
				}
			}
		}
	}

	return 0;
}

afx_msg void CMainFrame::OnMainUI(CCmdUI* pCmdUI)
{
	CMemReplayView* view = static_cast<CMemReplayView*>(GetActiveView());
	ITabPage* page = view ? view->GetActiveTab() : NULL;
	if (page)
		pCmdUI->SetCheck(page->GetMemoryType() == MEMTYPE_MAIN);
}

afx_msg void CMainFrame::OnRSXUI(CCmdUI* pCmdUI)
{
	CMemReplayView* view = static_cast<CMemReplayView*>(GetActiveView());
	ITabPage* page = view ? view->GetActiveTab() : NULL;
	if (page)
		pCmdUI->SetCheck(page->GetMemoryType() == MEMTYPE_RSX);
}

afx_msg void CMainFrame::OnCombinedUI(CCmdUI* pCmdUI)
{
	CMemReplayView* view = static_cast<CMemReplayView*>(GetActiveView());
	ITabPage* page = view ? view->GetActiveTab() : NULL;
	if (page)
		pCmdUI->SetCheck(page->GetMemoryType() == MEMTYPE_COMBINED);
}

afx_msg void CMainFrame::OnCountUI(CCmdUI* pCmdUI)
{
	CMemReplayView* view = static_cast<CMemReplayView*>(GetActiveView());
	ITabPage* page = view ? view->GetActiveTab() : NULL;
	if (page)
		pCmdUI->SetCheck(page->GetCountScaling());
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style |= WS_CLIPCHILDREN;
	//cs.dwExStyle |= WS_EX_COMPOSITED; 
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG
