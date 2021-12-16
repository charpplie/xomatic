// Demo.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MemReplay.h"
#include "MainFrm.h"
#include "treemap.h"
#include "MemReplayDoc.h"
#include "MemReplayView.h"
#include "IIdleHandler.h"
#include "ITabPage.h"
#include "AboutDlg.h"
#include "ReplayProgressDlg.h"
#include "CodeTreePage.h"
#include "ValidationPage.h"
#include "WaitForFutureReporter.h"
#include "SizerTreePage.h"

#include "ReplaySDK/DistributionMap.h"
#include "ReplaySDK/ContextTree.h"

#include <WS2tcpip.h>
#include <afxole.h>

namespace
{
	struct InvokeParams 
	{
		HANDLE ev;
		CMemReplayApp::InvokeHandler func;
		void* user;
	};

	void Invoke_Proxy(LPARAM lparam)
	{
		InvokeParams* params = reinterpret_cast<InvokeParams*>(lparam);
		(*params->func)(params->user);
		SetEvent(params->ev);
	}

	CMemReplayView* GetView()
	{
		CFrameWnd * pFrame = (CFrameWnd *)(AfxGetApp()->m_pMainWnd);
		CView * pView = pFrame->GetActiveView();
		if ( !pView )
			return NULL;
		if ( ! pView->IsKindOf( RUNTIME_CLASS(CMemReplayView) ) )
			return NULL;
		return (CMemReplayView*)pView;
	}
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const UINT CMemReplayApp::MSG_INVOKE = WM_USER + 2843;

// CMemReplayApp

BEGIN_MESSAGE_MAP(CMemReplayApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_APP_DISTRIBUTION, OnAppDistribution)
	ON_COMMAND(ID_APP_REVERSE, OnAppReverse)
	ON_COMMAND(ID_APP_GATHERTD, OnAppGatherTD)
	ON_COMMAND(ID_APP_GATHERBU, OnAppGatherBU)
	ON_COMMAND(ID_APP_EXCLUDE, OnAppExclude)
	ON_COMMAND(ID_APP_PLOT, OnAppPlot)
	ON_COMMAND(ID_COMBINED, OnAppCombined)
	ON_COMMAND(ID_MAIN, OnAppMainMem)
	ON_COMMAND(ID_RSX, OnAppRSXMem)
	ON_COMMAND(ID_COUNT, OnAppCountMem)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_EXPORTTOXLS, OnFileExportToXLS)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_TYPEINFO_LOOKUP, OnTypeInfoLookup)
	ON_COMMAND(ID_TOOLS_VALIDATEALLOCATIONS, OnToolsValidateAllocations)
	ON_COMMAND(ID_TOOLS_FINDSIZERS, OnToolsFindSizers)
	ON_COMMAND(ID_TOOLS_FINDOVERRUNSIZERS, OnToolsFindOverrunSizers)
	ON_COMMAND(ID_TOOLS_ANALYSEBUCKETS, OnToolsAnalyseBuckets)
END_MESSAGE_MAP()


// CMemReplayApp construction

CMemReplayApp::CMemReplayApp()
	: m_hAccel(NULL)
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	SetHooks(this);
}

void CMemReplayApp::AddIdleHandler(IIdleHandler* handler)
{
	m_idleList.push_back(handler);
}

void CMemReplayApp::RemoveIdleHandler(IIdleHandler* handler)
{
	m_idleList.erase(std::remove(m_idleList.begin(), m_idleList.end(), handler));
}

UINT CMemReplayApp::PopupMenu(UINT id)
{
	CMenu popup;
	popup.LoadMenu(id);
	CMenu* subMenu = popup.GetSubMenu(0);

	POINT cursor;
	GetCursorPos(&cursor);

	TPMPARAMS params = {0};
	params.cbSize = sizeof(params);

	return subMenu->TrackPopupMenuEx(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, cursor.x, cursor.y, m_pMainWnd, &params);
}

void CMemReplayApp::ClipboardCopy(const char* text)
{
	HANDLE hData = ::GlobalAlloc(GMEM_MOVEABLE, strlen(text) + 1);
	char* pData = (char*) ::GlobalLock(hData);
	strcpy(pData, text);
	::GlobalUnlock(hData);

	COleDataSource* clip = new COleDataSource;

	STGMEDIUM data = {0};
	data.tymed = TYMED_HGLOBAL;
	data.hGlobal = hData;

	clip->CacheData(CF_TEXT, &data);
	clip->SetClipboard();
}

// The one and only CMemReplayApp object

CMemReplayApp theApp;

// CMemReplayApp initialization

BOOL CMemReplayApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Crytek - memReplay"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMemReplayDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMemReplayView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (cmdInfo.m_nShellCommand==CCommandLineInfo::FileOpen)
	{
		if (!AfxGetApp()->OnCmdMsg(ID_FILE_NEW, 0, NULL, NULL))
			OnFileNew();
	}
	else
	{
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;
	}
	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	if (cmdInfo.m_nShellCommand==CCommandLineInfo::FileOpen)
	{
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;
	}

	/*
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	*/

	m_hAccel = LoadAccelerators(NULL, MAKEINTRESOURCE(IDR_MAINFRAME));

	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}

int CMemReplayApp::ExitInstance()
{
	if (m_hAccel)
		DestroyAcceleratorTable(m_hAccel);

	//WSACleanup();
	return 0;
}

void CMemReplayApp::OnFileOpen()
{
	ASSERT(m_pDocManager != NULL);

	CString OpenFilter = "Log File (*.mrl;*.zmrl)|*.mrl;*.zmrl|Compressed Log File (*.zmrl)|*.zmrl|";
	OpenFilter += "All Files (*.*)|*.*||";

	CFileDialog FileOpenDialog(
		TRUE,
		NULL,
		NULL,
		OFN_FILEMUSTEXIST |	OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
		OpenFilter,	 // filter
		AfxGetMainWnd());	
	if(IDOK == FileOpenDialog.DoModal())
	{ 
		m_currentDoc=AfxGetApp()->OpenDocumentFile(FileOpenDialog.GetPathName());
	}
}

void CMemReplayApp::OnFileExportToXLS()
{
	CMemReplayView* view = GetView();
	if (view)
	{
		view->ExportToXLS();
	}
}

void CMemReplayApp::OnEditCopy()
{
	CMemReplayView* view = GetView();
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
			page->OnEditCopy();
	}
}

void CMemReplayApp::OnEditFind()
{
	CMainFrame* frame = static_cast<CMainFrame*>(m_pMainWnd);

	frame->StartFind();
}

void CMemReplayApp::OnTypeInfoLookup()
{
	CMainFrame* frame = static_cast<CMainFrame*>(m_pMainWnd);

	frame->StartTypeInfoLookup();
}

void CMemReplayApp::OnToolsValidateAllocations()
{
	CMemReplayView* view = GetView();

	if (view)
	{
		IReplayDatabaseConnection* db = view->GetConnection();

		if (db)
		{
			ReplayQueryFuture<SharedPtr<ValidateResult> > future = db->BeginQueryValidation();

			CProgressReportDlg dlg;
			dlg.Run(MakeWaitForFutureReporter(future));

			CValidationPage* page = new CValidationPage(future->GetResult(), db->GetSymbolTable());
			view->AddTab(page, true);
			view->SelectTab(page);
		}
	}
}

void CMemReplayApp::OnToolsFindSizers()
{
	CMemReplayView* view = GetView();

	if (view)
	{
		IReplayDatabaseConnection* db = view->GetConnection();

		if (db)
		{
			ReplayQueryFuture<SharedPtr<GenericTree> > future = db->BeginQuerySizerTree();

			CProgressReportDlg dlg;
			dlg.Run(MakeWaitForFutureReporter(future));

			CSizerTreePage* page = new CSizerTreePage(future->GetResult(), "Sizers");
			view->AddTab(page, true);
			view->SelectTab(page);
		}
	}
}

void CMemReplayApp::OnToolsFindOverrunSizers()
{
	CMemReplayView* view = GetView();

	if (view)
	{
		IReplayDatabaseConnection* db = view->GetConnection();

		if (db)
		{
			ReplayQueryFuture<SharedPtr<GenericTree> > future = db->BeginQuerySizerAddObjectOverrunTree();

			CProgressReportDlg dlg;
			dlg.Run(MakeWaitForFutureReporter(future));

			CSizerTreePage* page = new CSizerTreePage(future->GetResult(), "Sizer Overrun");
			view->AddTab(page, true);
			view->SelectTab(page);
		}
	}
}

void CMemReplayApp::OnToolsAnalyseBuckets()
{
	CMemReplayView* view = GetView();

	if (view)
	{
		view->OpenBucketAnalysis();
	}
}

BOOL CMemReplayApp::OnIdle(LONG lCount)
{
	for (IdleList::iterator it = m_idleList.begin(), itEnd = m_idleList.end();
		it != itEnd;
		++ it)
	{
		(*it)->OnIdle();
	}

	Sleep(1);

	return TRUE;
}

BOOL CMemReplayApp::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == MSG_INVOKE)
	{
		reinterpret_cast<void (*)(LPARAM)>(pMsg->wParam)(pMsg->lParam);
		return TRUE;
	}

	return CWinApp::PreTranslateMessage(pMsg);
}

BOOL CMemReplayApp::ProcessMessageFilter(int code, LPMSG lpMsg)
{
	if (m_hAccel)
	{
		if (::TranslateAccelerator(m_pMainWnd->m_hWnd, m_hAccel, lpMsg))
		{
			return TRUE;
		}
	}

	return CWinApp::ProcessMessageFilter(code, lpMsg);
}

CMainFrame* CMemReplayApp::GetMainFrame()
{
	return static_cast<CMainFrame*>(AfxGetApp()->m_pMainWnd);
}

void CMemReplayApp::InvokeOnMainImpl(InvokeHandler func, void* user)
{
	if (GetCurrentThreadId() == m_nThreadID)
	{
		(*func)(user);
	}
	else
	{
		InvokeParams params;
		params.ev = CreateEvent(NULL, TRUE, FALSE, NULL);
		params.func = func;
		params.user = user;

		::PostThreadMessage(m_nThreadID, MSG_INVOKE, reinterpret_cast<WPARAM>(Invoke_Proxy), reinterpret_cast<LPARAM>(&params));
		WaitForSingleObject(params.ev, INFINITE);

		CloseHandle(params.ev);
	}
}

void CMemReplayApp::ShowMessageImpl(const TCHAR* msg, const TCHAR* title)
{
	MessageBox(GetMainWnd()->m_hWnd, msg, title, MB_OK);
}

std::string CMemReplayApp::OpenFileImpl(const char* filter, const TCHAR* title)
{
	CString OpenFilter = filter;

	CFileDialog FileOpenDialog(
		TRUE,
		NULL,
		NULL,
		OFN_FILEMUSTEXIST |	OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
		OpenFilter,	 // filter
		AfxGetMainWnd());	

	FileOpenDialog.m_ofn.lpstrTitle = title;

	if(IDOK == FileOpenDialog.DoModal())
		return FileOpenDialog.GetPathName().GetString();

	return std::string();
}

std::string CMemReplayApp::OpenFolderImpl(const TCHAR* title)
{
	TCHAR displayName[MAX_PATH] = {0};

	BROWSEINFO bi = {0};
	bi.hwndOwner = GetMainWnd()->m_hWnd;
	bi.pszDisplayName = displayName;

	bi.lpszTitle = title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	PIDLIST_ABSOLUTE pid = SHBrowseForFolder(&bi);

	char path[MAX_PATH];

	if (!SHGetPathFromIDList(pid, path))
	{
		path[0] = '\0';
	}

	CoTaskMemFree(pid);

	return path;
}

void CMemReplayApp::RunProgressTaskImpl(IProgressTask& task)
{
	CProgressTaskDlg dlg;
	dlg.Run(task);
}

// CMemReplayApp message handlers

// App command to run the dialog
void CMemReplayApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CMemReplayApp::OnAppDistribution()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			page->OnInspectClicked();
		}
	}
}

void CMemReplayApp::OnAppReverse()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			switch (PopupMenu(IDR_GATHER))
			{
			case ID_BYADDRESS: page->OnReverseClicked(ITabPage::GT_Address); break;
			case ID_BYFUNCTION: page->OnReverseClicked(ITabPage::GT_Function); break;
			case ID_BYFILELINE: page->OnReverseClicked(ITabPage::GT_Line); break;
			case ID_BYFILE: page->OnReverseClicked(ITabPage::GT_File); break;
			}
		}
	}
}

void CMemReplayApp::OnAppGatherTD()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			switch (PopupMenu(IDR_GATHER))
			{
			case ID_BYADDRESS: page->OnGatherTDClicked(ITabPage::GT_Address); break;
			case ID_BYFUNCTION: page->OnGatherTDClicked(ITabPage::GT_Function); break;
			case ID_BYFILELINE: page->OnGatherTDClicked(ITabPage::GT_Line); break;
			case ID_BYFILE: page->OnGatherTDClicked(ITabPage::GT_File); break;
			}
		}
	}
}

void CMemReplayApp::OnAppGatherBU()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			switch (PopupMenu(IDR_GATHER))
			{
			case ID_BYADDRESS: page->OnGatherBUClicked(ITabPage::GT_Address); break;
			case ID_BYFUNCTION: page->OnGatherBUClicked(ITabPage::GT_Function); break;
			case ID_BYFILELINE: page->OnGatherBUClicked(ITabPage::GT_Line); break;
			case ID_BYFILE: page->OnGatherBUClicked(ITabPage::GT_File); break;
			}
		}
	}
}

void CMemReplayApp::OnAppExclude()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			switch (PopupMenu(IDR_GATHER))
			{
			case ID_BYADDRESS: page->OnExcludeClicked(ITabPage::GT_Address); break;
			case ID_BYFUNCTION: page->OnExcludeClicked(ITabPage::GT_Function); break;
			case ID_BYFILELINE: page->OnExcludeClicked(ITabPage::GT_Line); break;
			case ID_BYFILE: page->OnExcludeClicked(ITabPage::GT_File); break;
			}
		}
	}
}

void CMemReplayApp::OnAppPlot()
{
	CMemReplayView* view = GetView();

	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			switch (PopupMenu(IDR_GATHER))
			{
			case ID_BYADDRESS: page->OnPlotClicked(ITabPage::GT_Address); break;
			case ID_BYFUNCTION: page->OnPlotClicked(ITabPage::GT_Function); break;
			case ID_BYFILELINE: page->OnPlotClicked(ITabPage::GT_Line); break;
			case ID_BYFILE: page->OnPlotClicked(ITabPage::GT_File); break;
			}
		}
	}
}

void CMemReplayApp::OnAppCombined()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			page->SetMemoryType(MEMTYPE_COMBINED);
		}
	}

	GetMainFrame()->UpdateToolbar();
}

void CMemReplayApp::OnAppMainMem()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			page->SetMemoryType(MEMTYPE_MAIN);
		}
	}

	GetMainFrame()->UpdateToolbar();
}

void CMemReplayApp::OnAppRSXMem()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			page->SetMemoryType(MEMTYPE_RSX);
		}
	}

	GetMainFrame()->UpdateToolbar();
}

void CMemReplayApp::OnAppCountMem()
{
	CMemReplayView* view = GetView();
 
	if (view)
	{
		ITabPage* page = view->GetActiveTab();
		if (page)
		{
			page->ToggleCountScaling();
		}
	}
	
	GetMainFrame()->UpdateToolbar();
}