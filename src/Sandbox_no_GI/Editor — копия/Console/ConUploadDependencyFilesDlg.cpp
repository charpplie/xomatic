// ConUploadDependencyFilesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConUploadDependencyFilesDlg.h"


// CConUploadDependencyFilesDlg dialog

const int kConUploadWindowCollapsedHeight = 145;

IMPLEMENT_DYNAMIC(CConUploadDependencyFilesDlg, CDialog)

CConUploadDependencyFilesDlg::CConUploadDependencyFilesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConUploadDependencyFilesDlg::IDD, pParent)
{
	m_bCollapsed = true;
	m_objectCount = 0;
	m_hUploadJob = 0;
}

CConUploadDependencyFilesDlg::~CConUploadDependencyFilesDlg()
{
}

void CConUploadDependencyFilesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_UPLOAD, m_progressUpload);
	DDX_Control(pDX, IDC_RICHEDIT_LOG, m_richedLog);
}


BEGIN_MESSAGE_MAP(CConUploadDependencyFilesDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_MORE, &CConUploadDependencyFilesDlg::OnBnClickedButtonMore)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_LOG, &CConUploadDependencyFilesDlg::OnBnClickedButtonClearLog)
	ON_BN_CLICKED(IDCANCEL, &CConUploadDependencyFilesDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_ABORT_UPLOAD, &CConUploadDependencyFilesDlg::OnBnClickedButtonAbortUpload)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CConUploadDependencyFilesDlg message handlers

bool CConUploadDependencyFilesDlg::StartUpload()
{
	CSelectionGroup* pSelGrp = GetIEditor()->GetSelection();
	std::set<CString> files;
	
	m_objectCount = 0;

	if( pSelGrp )
	{
		m_objectCount = pSelGrp->GetCount();

		for( size_t i = 0, iCount = pSelGrp->GetCount(); i < iCount; ++i )
		{
			CBaseObject* pObj = pSelGrp->GetObject( i );
			
			if( pObj )
			{
				CUsedResources res;

				pObj->GatherUsedResources( res );

				for( CUsedResources::ResourceFiles::iterator iter = res.files.begin(), iterEnd = res.files.end(); iter != iterEnd; ++iter )
				{
					files.insert( *iter );
				}
			}
		}
	}

	if( !pSelGrp || !pSelGrp->GetCount() )
	{
		AfxMessageBox( "Nothing to upload, no objects are selected!", MB_ICONEXCLAMATION );
		WriteToLog( "*** Nothing to upload, no selected objects\r\n" );
		return false;
	}

	if( files.empty() )
	{
		AfxMessageBox( "Nothing to upload, the selected objects did not report any used resource files", MB_ICONEXCLAMATION );
		WriteToLog( "*** Nothing to upload, the selected objects did not report any used resource files\r\n" );
		return false;
	}

	m_hUploadJob = CConsoleHotUpdate::Instance().AddFiles( files, m_hUploadJob, this );
	m_progressUpload.SetRange32( 0, files.size() );
	OnUploadStart();

	return true;
}

void CConUploadDependencyFilesDlg::OnUploadStart()
{
	WriteToLog( "*** Upload started\r\n" );
	GetDlgItem( IDC_BUTTON_ABORT_UPLOAD )->SetWindowText( "Abort upload" );
}

void CConUploadDependencyFilesDlg::OnUploadEnd( bool bAborted )
{
	CString tmpStr;

	tmpStr.Format( "*** Upload %s\r\n", bAborted ? "aborted" : "ended" );
	WriteToLog( tmpStr );
	SetDlgItemText( IDC_STATIC_INFO, "Current file: <stopped>" );
	SetDlgItemText( IDC_STATIC_TOTAL_FILES_INFO, "Total: <stopped>" );
	SetWindowText( "Console Upload Object(s) Dependency Files" );
	m_progressUpload.SetPos( 0 );
	GetDlgItem( IDC_BUTTON_ABORT_UPLOAD )->SetWindowText( "Upload" );
}

void CConUploadDependencyFilesDlg::OnUploadFileProgress( CHotUpdateSystem::EConsolePlatform aPlatform, const char* pCurrentFilename, UINT aUploadedFileCount, UINT aTotalFileCount, EResultCode aResult, const char* pAdditionalResultMsg )
{
	COLORREF msgColor = 0;
	CString str, strPlatform;

	if( !aTotalFileCount )
		return;

	int progress = (int)( 100 * (float)aUploadedFileCount/aTotalFileCount );

	str.Format( "Current file: %s", pCurrentFilename );
	SetDlgItemText( IDC_STATIC_INFO, str );
	m_progressUpload.SetPos( aUploadedFileCount );
	str.Format( "Total progress: %d%% (uploaded %d from %d total files, gathered from %d objects)", progress, aUploadedFileCount, aTotalFileCount, m_objectCount );
	SetDlgItemText( IDC_STATIC_TOTAL_FILES_INFO, str );
	str.Format( "%d%% (%d/%d) Console Upload Object(s) Dependency Files", progress, aUploadedFileCount, aTotalFileCount );
	SetWindowText( str );

	switch( aPlatform )
	{
		case CHotUpdateSystem::eXBOX360: strPlatform = "X360"; break;
		case CHotUpdateSystem::ePLAYSTATION3: strPlatform = "PS3"; break;
	};

	str = "";

	switch( aResult )
	{
			case eResultCode_UploadSuccessful:
			{
				str.Format( "[%s] Upload file '%s' successfuly\r\n", strPlatform.GetBuffer(), pCurrentFilename );
				msgColor = RGB( 0, 170, 0 );
				break;
			}

			case eResultCode_NeedsCompiling:
			{
				str.Format( "[%s] File needs compiling '%s', doing it..\r\n", strPlatform.GetBuffer(), pCurrentFilename );
				msgColor = RGB( 150, 100, 0 );
				break;
			}

			case eResultCode_UploadError:
			{
				str.Format( "[%s] Error while uploading file '%s'! (%s)\r\n", strPlatform.GetBuffer(), pCurrentFilename, pAdditionalResultMsg );
				msgColor = RGB( 200, 0, 0 );
				break;
			}

			case eResultCode_ExtensionNotSupported:
			{
				str.Format( "[%s] Extension not supported for '%s'\r\n", strPlatform.GetBuffer(), pCurrentFilename );
				msgColor = RGB( 120, 0, 120 );
				break;
			}

			case eResultCode_OperationAborted:
			{
				str.Format( "Operation aborted for '%s'\r\n", pCurrentFilename );
				msgColor = RGB( 20, 120, 20 );
				break;
			}

			case eResultCode_SkippedPlatform:
			{
				str.Format( "Platform '%s' is not enabled!\r\n", pAdditionalResultMsg );
				msgColor = RGB( 20, 120, 120 );
				break;
			}
	};

	WriteToLog( str.GetBuffer(), msgColor );
}

void CConUploadDependencyFilesDlg::OnRcMessage( CHotUpdateSystem::EConsolePlatform aPlatform, IResourceCompilerListener::MessageSeverity aSeverity, const char* pMsg )
{
	// skip info lines
	if( aSeverity == IResourceCompilerListener::MessageSeverity_Info )
		return;

	CString str, strSeverity, strPlatform;

	switch( aSeverity )
	{
		case IResourceCompilerListener::MessageSeverity_Debug: strSeverity = "[DEBUG]"; break;
		case IResourceCompilerListener::MessageSeverity_Error: strSeverity = "[ERROR]"; break;
		case IResourceCompilerListener::MessageSeverity_Warning: strSeverity = "[WARNING]"; break;
	}

	switch( aPlatform )
	{
		case CHotUpdateSystem::eXBOX360: strPlatform = "X360"; break;
		case CHotUpdateSystem::ePLAYSTATION3: strPlatform = "PS3"; break;
	};

	str.Format( "[%s][%s] %s\r\n", strPlatform.GetBuffer(), strSeverity.GetBuffer(), pMsg );
	WriteToLog( str.GetBuffer(), RGB( 200, 120, 0 ) );
}

void CConUploadDependencyFilesDlg::WriteToLog( const char* pText, COLORREF aTextColor )
{
	CHARFORMAT cf;

	ZeroMemory( &cf, sizeof( cf ) );
	cf.cbSize = sizeof(cf);
	cf.crTextColor = aTextColor;
	cf.dwEffects = CFE_BOLD;
	cf.dwMask = CFM_BOLD | CFM_COLOR;

	m_richedLog.SetSel( -1, -1 );
	m_richedLog.SetSelectionCharFormat( cf );
	m_richedLog.ReplaceSel( pText );
}

BOOL CConUploadDependencyFilesDlg::OnInitDialog()
{
	CRect rc;

	CDialog::OnInitDialog();
	GetWindowRect( &rc );
	m_expandedDialogHeight = rc.Height();
	// window collapsed by default
	SetWindowPos( 0, 0, 0, rc.Width(), kConUploadWindowCollapsedHeight, SWP_NOMOVE );
	SetDlgItemText( IDC_STATIC_INFO, "Current file: <stopped>" );
	SetDlgItemText( IDC_STATIC_TOTAL_FILES_INFO, "Total: <stopped>" );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConUploadDependencyFilesDlg::OnBnClickedButtonMore()
{
	CRect rc;

	GetWindowRect( &rc );

	if( m_bCollapsed )
	{
		SetWindowPos( 0, 0, 0, rc.Width(), m_expandedDialogHeight, SWP_NOMOVE );
		GetDlgItem( IDC_BUTTON_MORE )->SetWindowText( "Less" );
	}
	else
	{
		SetWindowPos( 0, 0, 0, rc.Width(), kConUploadWindowCollapsedHeight, SWP_NOMOVE );
		GetDlgItem( IDC_BUTTON_MORE )->SetWindowText( "More..." );
	}

	m_bCollapsed = !m_bCollapsed;
}

void CConUploadDependencyFilesDlg::OnBnClickedButtonClearLog()
{
	m_richedLog.SetWindowText( "" );
}

void CConUploadDependencyFilesDlg::OnBnClickedCancel()
{
	// just hide it, it can go on in the background
	ShowWindow( SW_HIDE );
}

void CConUploadDependencyFilesDlg::OnOK()
{
	// empty, not accepting ENTER
}

void CConUploadDependencyFilesDlg::OnBnClickedButtonAbortUpload()
{
	if( CConsoleHotUpdate::Instance().IsJobDone( m_hUploadJob ) )
	{
		StartUpload();
		return;
	}

	CConsoleHotUpdate::Instance().AbortJob( m_hUploadJob );
	WriteToLog( "Aborting transfer...\r\n", RGB( 0, 100, 200 ) );
}

void CConUploadDependencyFilesDlg::OnClose()
{
	if( CConsoleHotUpdate::Instance().IsJobDone( m_hUploadJob ) )
	{
		if( IDNO == AfxMessageBox( "Closing will abort upload, continue ?", MB_ICONWARNING|MB_YESNO ) )
		{
			return;
		}
	}

	CConsoleHotUpdate::Instance().AbortJob( m_hUploadJob );

	__super::OnClose();
}
