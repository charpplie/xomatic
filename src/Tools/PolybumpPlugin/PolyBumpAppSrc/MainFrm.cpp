// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PolyBumpApplication.h"
#include "MainFrm.h"
#include "..\PolyBumpWorkerThread.h"
//#include "PolyBumpApplicationDoc.h"				// CPolyBumpApplicationDoc

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_MESSAGE(WM_POLYBUMPTHREADFINISHED,OnUserMsg_UpdateWindow)
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
{
}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}



afx_msg LRESULT CMainFrame::OnUserMsg_UpdateWindow( WPARAM, LPARAM )
{
	Sleep(200);		// don't update right after call as the thread is not fully finished yet

	POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();

	while(pos)
	{
		CDocTemplate *pDocTempl = AfxGetApp()->GetNextDocTemplate(pos);
		ASSERT_VALID(pDocTempl);
	
		POSITION pos2 = pDocTempl->GetFirstDocPosition();

		while(pos2)
		{
			CDocument *pDoc = pDocTempl->GetNextDoc(pos2);

			pDoc->UpdateAllViews(0);
/*
			POSITION pos3 = pDoc->GetFirstViewPosition();

			while(pos3)
			{
				CView *pView = pDoc->GetNextView(pos3);

				pView->Invalidate(true);
				pView->UpdateWindow();
			}
*/
		}
	}


	return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		 | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE | WS_SYSMENU;

	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

