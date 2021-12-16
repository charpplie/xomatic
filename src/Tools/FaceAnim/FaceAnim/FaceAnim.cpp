// FaceAnim.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "FaceAnim.h"
#include "MainFrm.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"

#include <direct.h> // getcwd

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFaceAnimApp

BEGIN_MESSAGE_MAP(CFaceAnimApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


// CFaceAnimApp construction

CFaceAnimApp::CFaceAnimApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CFaceAnimApp object

CFaceAnimApp theApp;

// CFaceAnimApp initialization

BOOL CFaceAnimApp::InitInstance()
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
		RUNTIME_CLASS(CFaceAnimDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CFaceAnimView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	//////////////////////////////////////////////////////////////////////////
	// load config and calibration file
	char szText[256];
	FILE *fp=fopen("config.ini","rt");
	if (!fp)
	{
		// create default config file
		fp=fopen("config.ini","wt");
		strcpy(m_szCalibrationFile,DEFAULT_CALIBRATION_FILE);
		sprintf(szText,"CalibrationFile %s\n",m_szCalibrationFile);
		fputs(szText,fp);		
	}
	else
	{
		fgets(szText,256,fp);
		sscanf(szText,"CalibrationFile %s\n",m_szCalibrationFile);
	}
	fclose(fp);
	
	sprintf(szText,"FaceAnim Video Tool - calibration file \"%s\"",m_szCalibrationFile);
	CWnd *wnd=AfxGetMainWnd();
	// Change the window's title to the saved file's title.	
	SetWindowText(wnd->GetSafeHwnd(),szText);		

	//GetModuleFileName(AfxGetInstanceHandle(), szText, 250);
	_getcwd(m_szWorkingFolder,250);


	//////////////////////////////////////////////////////////////////////////

	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
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
void CFaceAnimApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CFaceAnimApp message handlers

