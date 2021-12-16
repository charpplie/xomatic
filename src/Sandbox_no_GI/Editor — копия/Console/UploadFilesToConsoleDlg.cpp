////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	UploadFilesToConsoleDlg.cpp
//  Version:	v1.00
//  Created:	29/07/2010 by Nicusor Nedelcu
//  Description:	Implements UploadFilesToConsoleDlg.h
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "UploadFilesToConsoleDlg.h"
#include "Util/IndexedFiles.h"
#include "Console/ConsoleHotUpdate.h"

enum ETreeItemKind
{
	eTreeItemKind_File,
	eTreeItemKind_Folder
};

enum EFileTypeIconIndex
{
	eFileTypeIconIndex_None = 0,
	eFileTypeIconIndex_Generic = 2
};

// CUploadFilesToConsoleDlg dialog

IMPLEMENT_DYNAMIC(CUploadFilesToConsoleDlg, CDialog)

CUploadFilesToConsoleDlg::CUploadFilesToConsoleDlg(CWnd* pParent /*=NULL*/)
	: CXTResizeDialog(CUploadFilesToConsoleDlg::IDD, pParent)
{
	m_totalFileCount = m_totalFolderCount = m_totalBytesToSend = 0;
	m_hUploadJob = 0;
}

CUploadFilesToConsoleDlg::~CUploadFilesToConsoleDlg()
{
}

void CUploadFilesToConsoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SOURCE, m_lstSource);
	DDX_Control(pDX, IDC_PROGRESS_TOTAL, m_progressTotal);
	DDX_Control(pDX, IDC_RICHEDIT_LOG, m_reditLog);
}


BEGIN_MESSAGE_MAP(CUploadFilesToConsoleDlg, CXTResizeDialog)
	ON_BN_CLICKED(IDC_UPLOAD_FILES, &CUploadFilesToConsoleDlg::OnBnClickedUploadFiles)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SOURCE, &CUploadFilesToConsoleDlg::OnNMDblclkListSource)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_SOURCE, &CUploadFilesToConsoleDlg::OnLvnKeydownListSource)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST_SOURCE, &CUploadFilesToConsoleDlg::OnLvnBegindragListSource)
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_CLEAR_ALL, &CUploadFilesToConsoleDlg::OnBnClickedClearAll)
	ON_BN_CLICKED(IDC_REMOVE_SELECTED, &CUploadFilesToConsoleDlg::OnBnClickedRemoveSelected)
	ON_BN_CLICKED(IDC_CLOSE, &CUploadFilesToConsoleDlg::OnBnClickedClose)
	ON_BN_CLICKED(IDC_BUTTON_HIDE, &CUploadFilesToConsoleDlg::OnBnClickedButtonHide)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ITEMS, &CUploadFilesToConsoleDlg::OnBnClickedAddItems)
	ON_BN_CLICKED(IDC_BUTTON_ABORT_UPLOAD, &CUploadFilesToConsoleDlg::OnBnClickedButtonAbortUpload)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_LOG, &CUploadFilesToConsoleDlg::OnBnClickedButtonClearLog)
END_MESSAGE_MAP()


// CUploadFilesToConsoleDlg message handlers

BOOL CUploadFilesToConsoleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pIPak = GetIEditor()->GetSystem()->GetIPak();
	assert( m_pIPak );
	m_bDragItems = false;
	m_rootFolder = gEnv->pSystem->GetRootFolder();
	m_currentPath = m_rootFolder;

	CRect rc;

	GetDlgItem(IDC_TREE_TARGET)->GetWindowRect( &rc );
	GetDlgItem(IDC_TREE_TARGET)->DestroyWindow();
	ScreenToClient( &rc );
	m_treeTarget.Create( WS_VISIBLE|WS_CHILD, rc, this, IDC_TREE_TARGET );
	m_treeTarget.AddTreeColumn( "Files" );
	m_treeTarget.EnableAutoNameGrouping( true, 0 );
	m_treeTarget.SetPathSeparators( "\\/" );

	static CImageList m_imageList;

	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_FILES_IMAGE, 16, RGB( 255, 0, 255 ) );
	m_lstSource.SetImageList( &m_imageList, LVSIL_SMALL );
	m_treeTarget.SetImageList( &m_imageList );

	m_mapExtToIconIndex["cgf"] = 3;
	m_mapExtToIconIndex["chr"] = 4;
	m_mapExtToIconIndex["cga"] = 4;
	m_mapExtToIconIndex["jpg"] = 5;
	m_mapExtToIconIndex["dds"] = 6;
	m_mapExtToIconIndex["tga"] = 7;
	m_mapExtToIconIndex["bmp"] = 8;
	m_mapExtToIconIndex["wav"] = 9;
	m_mapExtToIconIndex["ogg"] = 9;
	m_mapExtToIconIndex["mp2"] = 9;
	m_mapExtToIconIndex["mp3"] = 9;

	FillSourceTree();

	CFont* font = GetDlgItem( IDC_UPLOAD_FILES )->GetFont();
	LOGFONT logFont;

	font->GetLogFont(&logFont);
	logFont.lfWeight = FW_BOLD;
	m_fntBoldButton.CreateFontIndirect( &logFont );
	GetDlgItem( IDC_UPLOAD_FILES )->SetFont( &m_fntBoldButton );
	GetDlgItem( IDC_BUTTON_ABORT_UPLOAD )->SetFont( &m_fntBoldButton );
	GetDlgItem( IDC_BUTTON_ADD_ITEMS )->SetFont( &m_fntBoldButton );
	GetDlgItem( IDC_BUTTON_REMOVE_SELECTED )->SetFont( &m_fntBoldButton );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CUploadFilesToConsoleDlg::FileInfoCompare( const FileInfo &f1, const FileInfo &f2 )
{
	if( ( f1.attrib & _A_SUBDIR ) && !( f2.attrib & _A_SUBDIR ) )
		return true;

	if(!( f1.attrib & _A_SUBDIR ) && ( f2.attrib & _A_SUBDIR ) )
		return false;

	return stricmp( f1.filename, f2.filename ) < 0;
}

bool CUploadFilesToConsoleDlg::ScanDirectory( const CString &dir, std::vector<FileInfo>& files, bool bAddFoldersAlso, bool bRecursive, bool bScanPaksAlso )
{
	files.clear();

	ScanDirectoryRecursive( dir, files, bAddFoldersAlso, bRecursive, bScanPaksAlso );
	std::sort( files.begin(), files.end(), FileInfoCompare );

	if( dir != m_rootFolder && !bRecursive )
	{
		FileInfo file;

		file.attrib = _A_SUBDIR;
		file.filename = "..";
		files.insert( files.begin(), file );
	}

	return !files.empty();
}

bool CUploadFilesToConsoleDlg::ScanDirectoryRecursive( const CString &dir, std::vector<FileInfo>& files, bool bAddFoldersAlso, bool bRecursive, bool bScanPaksAlso )
{
	CString parentFolder = Path::AddBackslash(dir);
	CString searchPath = parentFolder + "*.*";

	// Add all directories.
	_finddata_t	fd;
	intptr_t		fhandle;
	CString			ext;

	fhandle = _findfirst( searchPath, &fd );

	if( fhandle != -1 )
	{
		do 
		{
			// skip current folder
			if( fd.name[0] == '.' )
			{
				continue;
			}

			ext = PathUtil::GetExt( fd.name );

			// skip paks
			if( ext == "pak" )
			{
				continue;
			}

			CString strFullPath = parentFolder;
			
			strFullPath += fd.name;

			// add file description to current directory listing
			FileInfo file;
			
			file.attrib = fd.attrib;
			file.filename = bRecursive ? strFullPath : fd.name;
			file.size = fd.size;
			file.time_access = fd.time_access;
			file.time_create = fd.time_create;
			file.time_write = fd.time_write;

			if( fd.attrib & _A_SUBDIR )
			{
				// if we also do want folders
				if( bAddFoldersAlso )
				{
					files.push_back( file );
				}
			}
			else
			{
				// add simple file
				files.push_back( file );
			}

			// if recursive, go inside subfolders and search for more files and folders
			if( bRecursive && ( fd.attrib & _A_SUBDIR ) )
			{
				ScanDirectoryRecursive( strFullPath, files, bAddFoldersAlso, bRecursive, bScanPaksAlso );
			}

		} while( _findnext( fhandle, &fd ) == 0 );

		_findclose( fhandle );
	}

	return !files.empty();
}

void CUploadFilesToConsoleDlg::FillSourceTree()
{
	CString								strExtension;
	CString								strFilename;
	int										nTotalFiles = 0;
	int										nCurrentFile = 0;
	CString								strIntermediateFilename;
	CCryFile							file;
	CFileUtil::FileArray	cFiles;

	ScanDirectory( m_currentPath, m_files, true, false, false );
	nTotalFiles = m_files.size();
	m_lstSource.DeleteAllItems();

	for( nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile )
	{
		FileInfo& rstFileDescriptor = m_files[nCurrentFile];
		strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();

		// this is a folder
		if( rstFileDescriptor.attrib & _A_SUBDIR )
		{
			int item = m_lstSource.InsertItem( nCurrentFile, strIntermediateFilename.GetBuffer(), 0 );
			m_lstSource.SetItemData( item, eTreeItemKind_Folder );
		}
		else
		{
			string ext = PathUtil::GetExt( strIntermediateFilename.GetBuffer() );
			int imageIndex = m_mapExtToIconIndex[ext];

			// not found ?
			if( imageIndex == eFileTypeIconIndex_None )
			{
				// unknown file type
				imageIndex = eFileTypeIconIndex_Generic;
			}

			int item = m_lstSource.InsertItem( nCurrentFile, strIntermediateFilename.GetBuffer(), imageIndex );
			m_lstSource.SetItemData( item, eTreeItemKind_File );
		}
	}

	m_lstSource.SetItemState( 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
	m_lstSource.SetSelectionMark( 0 );
	GetDlgItem(IDC_STATIC_CRT_PATH)->SetWindowText( m_currentPath.GetBuffer() );
	GetDlgItem(IDC_STATIC_CRT_PATH)->RedrawWindow();
}

void CUploadFilesToConsoleDlg::OnBnClickedUploadFiles()
{
	if( m_filesToUpload.empty() )
	{
		WriteToLog( "*** Nothing to upload, please add some files to the right side tree panel\r\n" );
		return;
	}

	if( !CConsoleHotUpdate::Instance().IsJobDone( m_hUploadJob ) )
	{
		AfxMessageBox( "Uploading in progress, please abort it first", MB_ICONEXCLAMATION );
		return;
	}

	GetDlgItem(IDC_UPLOAD_FILES)->EnableWindow( FALSE );
	GetDlgItem(IDC_BUTTON_ABORT_UPLOAD)->EnableWindow( TRUE );
	m_progressTotal.SetRange32( 0, m_totalFileCount );
	m_hUploadJob = CConsoleHotUpdate::Instance().AddFiles( m_filesToUpload, m_hUploadJob, this );
	OnUploadStart();
}

void CUploadFilesToConsoleDlg::OnUploadFileProgress( CHotUpdateSystem::EConsolePlatform aPlatform, const char* pCurrentFilename, UINT aUploadedFileCount, UINT aTotalFileCount, EResultCode aResult, const char* pAdditionalResultMsg )
{
	COLORREF msgColor = 0;
	CString str, strPlatform;

	if( !aTotalFileCount )
		return;

	int progress = (int)( 100 * (float)aUploadedFileCount/aTotalFileCount );

	str.Format( "Current file: %s", pCurrentFilename );
	SetDlgItemText( IDC_STATIC_FILE_INFO, str );
	m_progressTotal.SetPos( aUploadedFileCount );
	str.Format( "Total progress: %d%% (uploaded %d from %d total files)", progress, aUploadedFileCount, m_filesToUpload.size() );
	SetDlgItemText( IDC_STATIC_TOTAL_FILES_INFO, str );
	str.Format( "%d%% (%d/%d) Upload Files To Console", progress, aUploadedFileCount, m_filesToUpload.size() );
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

void CUploadFilesToConsoleDlg::OnRcMessage( CHotUpdateSystem::EConsolePlatform aPlatform, IResourceCompilerListener::MessageSeverity aSeverity, const char* pMsg )
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

void CUploadFilesToConsoleDlg::OnUploadStart()
{
	WriteToLog( "*** Upload started/resumed ***\r\n", RGB( 150, 80, 10 ) );
}

void CUploadFilesToConsoleDlg::OnUploadEnd( bool bAborted )
{
	CString tmpStr;

	tmpStr.Format( "*** Upload %s\r\n", bAborted ? "aborted" : "ended" );
	WriteToLog( tmpStr );
	SetDlgItemText( IDC_STATIC_FILE_INFO, "Current file: <ended>" );
	m_progressTotal.SetPos( 0 );
	SetDlgItemText( IDC_STATIC_TOTAL_FILES_INFO, "Total progress: <ended>" );
	GetDlgItem(IDC_UPLOAD_FILES)->EnableWindow( TRUE );
	GetDlgItem(IDC_BUTTON_ABORT_UPLOAD)->EnableWindow( FALSE );
	SetWindowText( "Upload Files To Console" );
}

void CUploadFilesToConsoleDlg::ActivateListItem( int aIndex )
{
	CString strName;

	strName = m_lstSource.GetItemText( aIndex, 0 );

	if( strName == ".." )
	{
		m_currentPath = PathUtil::GetParentDirectory( m_currentPath.GetBuffer() ).c_str();
		FillSourceTree();
	}
	else
	{
		if( eTreeItemKind_Folder == m_lstSource.GetItemData( aIndex ) )
		{
			m_currentPath = Path::AddBackslash( m_currentPath );
			m_currentPath += strName.GetBuffer();
			FillSourceTree();
		}
		else
		{
			//TODO: add file on double click
		}
	}
}
void CUploadFilesToConsoleDlg::OnNMDblclkListSource(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	ActivateListItem( pNMItemActivate->iItem );
}

void CUploadFilesToConsoleDlg::OnLvnKeydownListSource(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	*pResult = 0;

	if( pLVKeyDow->wVKey == VK_RETURN )
	{
		ActivateListItem( m_lstSource.GetSelectionMark() );
	}
	else
	if( pLVKeyDow->wVKey == VK_BACK && m_currentPath != m_rootFolder )
	{
		m_currentPath = PathUtil::GetParentDirectory( m_currentPath.GetBuffer() ).c_str();
		FillSourceTree();
	}
}

void CUploadFilesToConsoleDlg::OnOK()
{
	// nothing, does not accept ENTER key to close window
}

BOOL CUploadFilesToConsoleDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && &m_lstSource == GetFocus() )
	{
		ActivateListItem( m_lstSource.GetSelectionMark() );
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CUploadFilesToConsoleDlg::OnLvnBegindragListSource(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	m_bDragItems = true;
	SetCapture();
	::SetCursor( AfxGetApp()->LoadCursor( MAKEINTRESOURCE( IDC_POINTER_DRAG_ITEM ) ) );
}

void CUploadFilesToConsoleDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if( m_bDragItems )
	{
		ReleaseCapture();
		
		POINT pt;

		GetCursorPos( &pt );
		HWND hOverWnd = ::WindowFromPoint( pt );

		if( hOverWnd == m_treeTarget.GetSafeHwnd() )
		{
			AddSelectedFilesAndFolders();
		}
		
		m_bDragItems = false;
	}

	::SetCursor( AfxGetApp()->LoadCursor( MAKEINTRESOURCE( IDC_ARROW ) ) );

	CDialog::OnLButtonUp(nFlags, point);
}

void CUploadFilesToConsoleDlg::AddSelectedFilesAndFolders()
{
	m_treeTarget.BeginUpdate();

	for( size_t i = 0; i < m_lstSource.GetItemCount(); ++i )
	{
		if( LVIS_SELECTED == m_lstSource.GetItemState( i, LVIS_SELECTED ) )
		{
			CString strName = m_lstSource.GetItemText( i, 0 );
			bool bIsFolder = ( eTreeItemKind_Folder == m_lstSource.GetItemData( i ) );

			if( strName == ".." )
			{
				continue;
			}

			// create full path to file
			CString path = CString( ( Path::AddBackslash( m_currentPath ) + strName));

			// strip engine root folder, we dont need it, aka "Main" for example
			path.Replace( Path::AddBackslash( m_rootFolder ), "" );

			//
			// if it is a folder, then we should get all files inside it, recursive
			//
			if( bIsFolder )
			{
				std::vector<FileInfo> dirFiles;
				
				ScanDirectory( path, dirFiles, false, true, false );

				// add files to tree
				for( size_t j = 0, jCount = dirFiles.size(); j < jCount; ++j )
				{
					FileInfo& dirFile = dirFiles[j];
					CString filepath = dirFile.filename;

					// strip engine root folder, we dont need it, aka "Main" for example
					filepath.Replace( Path::AddBackslash( m_rootFolder ), "" );

					// if we already have it, skip it
					if( m_filesToUpload.end() != m_filesToUpload.find( filepath ) )
					{
						continue;
					}

					++m_totalFileCount;
					uint64 fileSize = 0;
					CFileUtil::GetDiskFileSize( filepath, fileSize );
					m_totalBytesToSend += fileSize;
					m_filesToUpload.insert( filepath );
					CTreeItemRecord* pRec = new CTreeItemRecord( false, filepath );

					if( !pRec )
						continue;

					string ext = PathUtil::GetExt( dirFile.filename.GetBuffer() );
					int imageIndex = m_mapExtToIconIndex[ext];

					if( imageIndex == eFileTypeIconIndex_None )
					{
						imageIndex = eFileTypeIconIndex_Generic;
					}

					pRec->SetIcon( imageIndex );
					pRec->SetUserString( filepath );
					m_treeTarget.AddTreeRecord( pRec, NULL );
					m_treeTarget.EnsureItemVisible( pRec );
				}
			}
			else // this is a file
			{
				// if we already have it, skip it
				if( m_filesToUpload.end() != m_filesToUpload.find( path ) )
				{
					continue;
				}

				m_totalFileCount += bIsFolder ? 0 : 1;
				m_totalFolderCount += bIsFolder ? 1 : 0;
				uint64 fileSize = 0;
				CFileUtil::GetDiskFileSize( path, fileSize );
				m_totalBytesToSend += fileSize;
				m_filesToUpload.insert( path );
				CTreeItemRecord* pRec = new CTreeItemRecord( false, path );

				if( !pRec )
					continue;

				string ext = PathUtil::GetExt( strName.GetBuffer() );
				int imageIndex = m_mapExtToIconIndex[ext];

				if( imageIndex == eFileTypeIconIndex_None )
				{
					imageIndex = eFileTypeIconIndex_Generic;
				}

				pRec->SetIcon( bIsFolder ? 0 : imageIndex );
				pRec->SetUserString( path );
				m_treeTarget.AddTreeRecord( pRec, NULL );
				m_treeTarget.EnsureItemVisible( pRec );
			}
		}
	}

	m_treeTarget.EndUpdate();
	m_treeTarget.Populate();
	UpdateUploadInfoLabel();
}

void CUploadFilesToConsoleDlg::UpdateUploadInfoLabel()
{
	// 
	CString str;

	str.Format( "%d files, %d folders, %d kB", m_totalFileCount, m_totalFolderCount, m_totalBytesToSend / 1024 );
	SetDlgItemText( IDC_STATIC_UPLOAD_INFO, str );
}

void CUploadFilesToConsoleDlg::WriteToLog( const char* pText, COLORREF aTextColor )
{
	CHARFORMAT cf;

	ZeroMemory( &cf, sizeof( cf ) );
	cf.cbSize = sizeof(cf);
	cf.crTextColor = aTextColor;
	cf.dwEffects = CFE_BOLD;
	cf.dwMask = CFM_BOLD | CFM_COLOR;

	m_reditLog.SetSel( -1, -1 );
	m_reditLog.SetSelectionCharFormat( cf );
	m_reditLog.ReplaceSel( pText );
}

BOOL CUploadFilesToConsoleDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CUploadFilesToConsoleDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if( m_bDragItems )
	{
		POINT pt;

		GetCursorPos( &pt );
		HWND hOverWnd = ::WindowFromPoint( pt );

		if( hOverWnd == m_treeTarget.GetSafeHwnd() || hOverWnd == m_lstSource.GetSafeHwnd() )
		{
			::SetCursor( AfxGetApp()->LoadCursor( MAKEINTRESOURCE( IDC_POINTER_DRAG_ITEM ) ) );
		}
		else
		{
			::SetCursor( AfxGetApp()->LoadCursor( MAKEINTRESOURCE( IDC_NODROP ) ) );
		}
	}

	CDialog::OnMouseMove(nFlags, point);
}

void CUploadFilesToConsoleDlg::OnBnClickedClearAll()
{
	m_filesToUpload.clear();
	m_treeTarget.DeleteAllItems();
	m_treeTarget.Populate();
	m_totalFileCount = 0;
	m_totalFolderCount = 0;
	m_totalBytesToSend = 0;
	UpdateUploadInfoLabel();
}

void CUploadFilesToConsoleDlg::OnBnClickedRemoveSelected()
{
	CTreeCtrlReport::Records recs;

	m_treeTarget.GetSelectedRecords( recs );

	for( size_t i = 0; i < recs.size(); ++i  )
	{
		CTreeItemRecord* pRec = recs[i];
		std::set<CString>::iterator iter = std::find( m_filesToUpload.begin(), m_filesToUpload.end(), pRec->GetUserString().GetBuffer() );

		if( iter != m_filesToUpload.end() )
		{
			uint64 fileSize = 0;
			CFileUtil::GetDiskFileSize( *iter, fileSize );
			m_totalBytesToSend -= fileSize;
			--m_totalFileCount;
			m_filesToUpload.erase( iter );
			m_treeTarget.DeleteRecordItem( pRec );
		}
		else
		// we are trying to delete a folder, delete all children folders and files
		{
			std::set<CString>::iterator iterFile = m_filesToUpload.begin();
			CString folderPath;
			
			m_treeTarget.CalculateItemPath( pRec, folderPath, false );
			folderPath = Path::AddBackslash( folderPath);

			while( iterFile != m_filesToUpload.end() )
			{
				if( 0 == iterFile->Find( folderPath ) )
				{
					uint64 fileSize = 0;
					CFileUtil::GetDiskFileSize( *iterFile, fileSize );
					m_totalBytesToSend -= fileSize;
					--m_totalFileCount;
					m_filesToUpload.erase( iterFile++ );
				}
				else
				{
					++iterFile;
				}
			}

			m_treeTarget.DeleteRecordItem( pRec );
		}
	}

	// delete empty folders
	std::vector<CXTPReportRecord*> roots;
	
	for( size_t i = 0, iCount = m_treeTarget.GetRecords()->GetCount(); i < iCount; ++i )
	{
		// if record is a root
		if( !m_treeTarget.GetRecords()->GetAt( i )->GetParentRecord() )
		{
			roots.push_back( m_treeTarget.GetRecords()->GetAt( i ) );
		}
	}

	m_treeTarget.BeginUpdate();
	
	for( size_t i = 0, iCount = roots.size(); i < iCount; ++i )
	{
		DeleteEmptyFoldersRecursive( (CTreeItemRecord*)roots[i] );
	}

	m_treeTarget.EndUpdate();
	m_treeTarget.Populate();
	UpdateUploadInfoLabel();
}

bool CUploadFilesToConsoleDlg::DeleteEmptyFoldersRecursive( CTreeItemRecord* pRec )
{
	std::set<CString>::iterator iter = std::find( m_filesToUpload.begin(), m_filesToUpload.end(), pRec->GetUserString().GetBuffer() );

	if( iter != m_filesToUpload.end() )
	{
		return false;
	}

	for( size_t i = 0, iCount = pRec->GetChildCount(); i < iCount; ++i )
	{
		if( DeleteEmptyFoldersRecursive( pRec->GetChild( i ) ) )
		{
			iCount--;
			i--;
		}
	}

	if( !pRec->HasChildren() )
	{
		m_treeTarget.DeleteRecordItem( pRec );
		return true;
	}

	return false;
}

void CUploadFilesToConsoleDlg::OnBnClickedClose()
{
	EndDialog(0);
}

void CUploadFilesToConsoleDlg::OnBnClickedButtonHide()
{
	ShowWindow( SW_HIDE );
}

void CUploadFilesToConsoleDlg::OnBnClickedAddItems()
{
	AddSelectedFilesAndFolders();
}

void CUploadFilesToConsoleDlg::OnBnClickedButtonAbortUpload()
{
	if( !CConsoleHotUpdate::Instance().IsJobDone( m_hUploadJob ) )
	{
		if( IDYES == AfxMessageBox( "Do you want to abort the file transfer ?", MB_YESNO|MB_ICONQUESTION ) )
		{
			CConsoleHotUpdate::Instance().AbortJob( m_hUploadJob );
			WriteToLog( "Aborting transfer...\r\n", RGB( 0, 100, 200 ) );
		}
	}
}

void CUploadFilesToConsoleDlg::OnBnClickedButtonClearLog()
{
	m_reditLog.SetWindowText( "" );
}
