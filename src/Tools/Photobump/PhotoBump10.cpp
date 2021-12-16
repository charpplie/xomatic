// PhotoBump10.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PhotoBump10.h"
#include "MainFrm.h"

#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPhotoBump10App

BEGIN_MESSAGE_MAP(CPhotoBump10App, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CPhotoBump10App construction

CPhotoBump10App::CPhotoBump10App()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_pView=NULL;
	m_bCmdLineParsed=false;
	m_bStandalone=false;
}


// The one and only CPhotoBump10App object

CPhotoBump10App theApp;

// CPhotoBump10App initialization

BOOL CPhotoBump10App::InitInstance()
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
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CPhotoBump10Doc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CPhotoBump10View));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	//ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;	

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
BOOL CPhotoBump10App::OnIdle(LONG lCount)
{			
	//if (m_pView)
	//	m_pView->OnPaint();	
	
	if (m_pView && !m_bCmdLineParsed)
	{	
		for (int i = 1; i < __argc; i++)
		{
			LPCTSTR pszParam = __targv[i];
			BOOL bFlag = FALSE;
			BOOL bLast = ((i + 1) == __argc);
			if (pszParam[0] == '-' || pszParam[0] == '/')
			{
				// remove flag specifier
				bFlag = TRUE;
				++pszParam;
			}

			m_bCmdLineParsed=true;
			if (stricmp(pszParam,"standalone")==0)			
			{
				m_bStandalone=true;
				CPhotoBump10Doc *pDoc=m_pView->GetDocument();				
			}
			else
			{			
				m_pView->SetTimer(1,15,NULL); 
				// dump error at least once to set all camera matrices
				// NOTE: must be called from view otherwise GL context etc. is not 
				// valid, thus no gl matrix is returned - the function just sets a bool
				//m_pGLView->DumpError();
				//m_pGLView->SetTimer(1,500,NULL); 			
				
				CPhotoBump10Doc *pDoc=m_pView->GetDocument();
				strcpy(pDoc->m_szDataLoad,pszParam);				
				pDoc->m_bLoadData=true;
			}
			//rCmdInfo.ParseParam(pszParam, bFlag, bLast);
		} //i
	}

	return(CWinApp::OnIdle(lCount));
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
void CPhotoBump10App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CPhotoBump10App message handlers

