#include "stdafx.h"

#include "PolyBumpApplication.h"
#include "MainFrm.h"
#include "ResourceCompilerHelper.h"		// CResourceCompilerHelper
#include "ChildFrm.h"
#include "PolyBumpApplicationDoc.h"
#include "PolyBumpApplicationView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPolyBumpApplicationApp

BEGIN_MESSAGE_MAP(CPolyBumpApplicationApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND(ID_FILE_APPLICATIONSETTINGS, OnFileApplicationsettings)
END_MESSAGE_MAP()


// CPolyBumpApplicationApp construction

CPolyBumpApplicationApp::CPolyBumpApplicationApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CPolyBumpApplicationApp object

CPolyBumpApplicationApp theApp;

// CPolyBumpApplicationApp initialization

BOOL CPolyBumpApplicationApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(12);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_CrySurfaceFileTYPE,
		RUNTIME_CLASS(CPolyBumpApplicationDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CPolyBumpApplicationView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();
	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
	pMainFrame->UpdateWindow();
	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CPolyBumpApplicationApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CPolyBumpApplicationApp message handlers


void CPolyBumpApplicationApp::OnFileApplicationsettings()
{
	CResourceCompilerHelper rc;
	
	rc.ResourceCompilerUI(AfxGetMainWnd()->GetSafeHwnd());
}


/*
BOOL CPolyBumpApplicationApp::OnIdle(LONG lCount)
{
	POSITION pos = GetFirstDocTemplatePosition();

	while(pos)
	{
		CDocTemplate *pDoc = GetNextDocTemplate(pos);
		ASSERT_VALID(pDoc);
	
		((CPolyBumpApplicationDoc *)pDoc)->OnUserIdle();
	}

//	return CWinApp::OnIdle(lCount);
	CWinApp::OnIdle(lCount);

	Sleep(100);

	return true;
}
*/