////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   PakManagerDlg.cpp
//  Version:     v1.00
//  Description: Implementation file for the pack manager user interface.
//  Author: Nicusor Nedelcu
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "PakManagerDlg.h"
#include "GameExporter.h"
#include "IEditor.h"
#include "GameEngine.h"
#include "CryEditDoc.h"

enum EFileTypeIconIndex
{
	eFileTypeIconIndex_None = -1,
	eFileTypeIconIndex_Folder = 0,
	eFileTypeIconIndex_Generic = 2
};

namespace PakManager
{
	const char* kNoPakMessage = "There is no PAK archive opened or created, please create or open one first.";
	const char* kPaksNotAllowedToAddFiles = "gamecrysis2/Animations.pak gamecrysis2/GameData.pak gamecrysis2/Music.pak gamecrysis2/Objects.pak gamecrysis2/ObjectsLS.pak gamecrysis2/patch.pak Patch/patch1.pak Patch/DX11.pak Patch/TexturesHighRes.pak gamecrysis2/Scripts.pak gamecrysis2/Sounds.pak gamecrysis2/Textures.pak gamecrysis2/Videos.pak Engine/Engine.pak Engine/ShaderCache.pak Engine/ShaderCacheStartup.pak Engine/Shaders.pak Engine/ShadersBin.pak";
	const char* kAddFilesToOfficialPaksMessage = "IMPORTANT WARNING!\n\nYou are trying to modify an official game PAK archive.\nThis could make the game unplayable!\nBy continuing this operation you agree with the risks of damaging your game installation.\n\nContinue?";
	const char* kPakCannotBeOpenedMessage = "The PAK archive cannot be opened, it is locked by the engine or other process(es).\n\nIf the pak is level.pak or other pak used by the current loaded level, please load another level or close the editor to have access to that PAK archive.";
	const char* kUnpackFullPathMessage = "Create full path for the extracted files?";
	const char* kAddFilesWithFullPathMessage = "Add the files using the full path?\n\nNote: If a MOD is set, the files must reside in the MOD's folder, else they must reside in the \\GameCrysis2 folder.";
	const char* kCannotAddFilesWithFullPathMessage = "Cannot add files with full relative path because they are residing outside the current game data folder (Mods\\<mod name>\\GameCrysis2 or \\GameCrysis2)\n\nContinue adding the files with no full path info?";
}

// CPakManagerDlg dialog

IMPLEMENT_DYNAMIC(CPakManagerDlg, CDialog)

CPakManagerDlg::CPakManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPakManagerDlg::IDD, pParent)
{
	m_hCurrentFolder = NULL;
	m_bReloadLevelAfterPakClose = false;
}

CPakManagerDlg::~CPakManagerDlg()
{
}

void CPakManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILES, m_pakEntriesList);
	DDX_Control(pDX, IDC_PROGRESS, m_pakProgress);
	DDX_Control(pDX, IDC_STATIC_PAK_STATUS, m_stStatus);
}


BEGIN_MESSAGE_MAP(CPakManagerDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_OPEN_PAK, &CPakManagerDlg::OnBnClickedButtonOpenPak)
	ON_BN_CLICKED(IDC_BUTTON_CREATE_PAK, &CPakManagerDlg::OnBnClickedButtonCreatePak)
	ON_BN_CLICKED(IDC_BUTTON_ADD_FILES_TO_PAK, &CPakManagerDlg::OnBnClickedButtonAddFilesToPak)
	ON_BN_CLICKED(IDC_BUTTON_EXTRACT_FILES_FROM_PAK, &CPakManagerDlg::OnBnClickedButtonExtractFilesFromPak)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_FILES_FROM_PAK, &CPakManagerDlg::OnBnClickedButtonDeleteFilesFromPak)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CPakManagerDlg::OnBnClickedButtonClose)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILES, &CPakManagerDlg::OnNMDblclkListFiles)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_FILES, &CPakManagerDlg::OnLvnKeydownListFiles)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_ADD_FOLDERS_TO_PAK, &CPakManagerDlg::OnBnClickedButtonAddFoldersToPak)
END_MESSAGE_MAP()


// CPakManagerDlg message handlers

void CPakManagerDlg::SetDlgTitle()
{
	if( m_pak.GetArchive() )
	{
		CString str;

		str.Format( "PAK Manager - [%s]", m_pak.GetArchive()->GetFullPath() );
		SetWindowText( str );
	}
	else
	{
		SetWindowText( "PAK Manager" );
	}
}

void CPakManagerDlg::SetPathText()
{
	CString strPath;

	GetCurrentPakPath( strPath );
	GetDlgItem(IDC_STATIC_PATH)->SetWindowText( strPath );
}

void CPakManagerDlg::FillFiles()
{
	m_pakEntriesList.DeleteAllItems();

	if( !m_pak.GetArchive() )
		return;

	if( m_hCurrentFolder != m_hRootFolder )
	{
		m_pakEntriesList.InsertItem( 0, ".." );
	}

	m_currentVisibleFolders.clear();
	m_currentVisibleFiles.clear();
	m_pak.GetArchive()->EnumEntries( m_hCurrentFolder, this );
	SetPathText();
}

void CPakManagerDlg::GetCurrentPakPath( CString& rPath )
{
	rPath = "";

	for( size_t i = 0, iCount = m_folderNamesStack.size(); i < iCount; ++i )
	{
		rPath += m_folderNamesStack[i];
		rPath += "/";
	}
}

bool CPakManagerDlg::OnEnumArchiveEntry( const char* pFilename, ICryArchive::Handle hEntry, bool bIsFolder, int aSize, __int64 aModifiedTime )
{
	int index = m_pakEntriesList.InsertItem( m_pakEntriesList.GetItemCount(), pFilename, bIsFolder ? eFileTypeIconIndex_Folder : eFileTypeIconIndex_Generic );

	m_pakEntriesList.SetItemData( index, (DWORD_PTR)hEntry );

	if( !bIsFolder )
	{
		CString strTmp;
		SYSTEMTIME time;
		int sizeKb = aSize / 1024;
		bool bIsOver1k = aSize >= 1024;

		strTmp.Format( "%d %s", bIsOver1k ? sizeKb : aSize, bIsOver1k ? "KB" : "B" );
		m_pakEntriesList.SetItemText( index, 1, strTmp );

		FileTimeToSystemTime( (FILETIME*) &aModifiedTime, &time );
		strTmp.Format( "%d/%d/%d %d:%d", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute );
		m_pakEntriesList.SetItemText( index, 2, strTmp );
	}
	else
	{
		m_pakEntriesList.SetItemText( index, 1, "<DIR>" );
	}

	if( bIsFolder )
	{
		m_currentVisibleFolders[hEntry] = index;
	}
	else
	{
		m_currentVisibleFiles[hEntry] = index;
	}

	return true;
}

struct SSaveFileFromPak : ICryArchive::IEnumerateArchiveEntries
{
	bool OnEnumArchiveEntry( const char* pFilename, ICryArchive::Handle hEntry, bool bIsFolder, int aSize, __int64 aModifiedTime )
	{
		if( bIsFolder )
		{
			m_folders[pFilename] = hEntry;
		}
		else
		{
			m_files[pFilename] = hEntry;
		}

		return true;
	}

	void Clear()
	{
		m_folders.clear();
		m_files.clear();
	}

	typedef std::map<CString,ICryArchive::Handle> TFilenameHandleMap;

	TFilenameHandleMap m_folders, m_files;
};

void CPakManagerDlg::UnpackFilesRecursive( ICryArchive::Handle hFolder, const CString& path, bool bFirstLevel )
{
	SSaveFileFromPak  saveFilesEnumerator;
	CString newFile, newPath, strTmp;

	m_pak.GetArchive()->EnumEntries( hFolder, &saveFilesEnumerator );
	::CreateDirectory( path, NULL );
	
	// unpack folders
	for( SSaveFileFromPak::TFilenameHandleMap::iterator iter = saveFilesEnumerator.m_folders.begin(),
				iterEnd = saveFilesEnumerator.m_folders.end(); iter != iterEnd; ++iter )
	{
		if( bFirstLevel )
		{
			// check if folder selected
			std::map<ICryArchive::Handle,int>::iterator iterSearch = m_currentVisibleFolders.find( iter->second );

			if(	iterSearch == m_currentVisibleFolders.end() ||
					!(m_pakEntriesList.GetItemState( iterSearch->second, LVIS_SELECTED ) & LVIS_SELECTED) )
			{
				continue;
			}
		}

		newPath = path;
		newPath = Path::AddBackslash( newPath );
		newPath += iter->first;
		::CreateDirectory( newPath, NULL );
		UnpackFilesRecursive( iter->second, newPath, false );
	}

	// unpack files
	for( SSaveFileFromPak::TFilenameHandleMap::iterator iter = saveFilesEnumerator.m_files.begin(),
				iterEnd = saveFilesEnumerator.m_files.end(); iter != iterEnd; ++iter )
	{
		if( bFirstLevel )
		{
			// check if file selected
			std::map<ICryArchive::Handle,int>::iterator iterSearch = m_currentVisibleFiles.find( iter->second );

			if( iterSearch == m_currentVisibleFiles.end() )
			{
				continue;
			}
			else
			{
				if( !(m_pakEntriesList.GetItemState( iterSearch->second, LVIS_SELECTED ) & LVIS_SELECTED) )
				{
					continue;
				}
			}
		}

		newFile = path;
		newFile = Path::AddBackslash( newFile );
		newFile += iter->first;
		FILE* pFile = fopen( newFile, "wb" );

		strTmp.Format( "Unpacking: %s...", newFile.GetBuffer() );
		m_stStatus.SetWindowText( strTmp );
		
		if( pFile )
		{
			char* pData = new char[m_pak.GetArchive()->GetFileSize( iter->second )];
			ASSERT(pData);
			
			if( pData )
			{
				m_pak.GetArchive()->ReadFile( iter->second, pData );
				fwrite( pData, m_pak.GetArchive()->GetFileSize( iter->second ), 1, pFile );
				fclose( pFile );
				delete [] pData;
			}
			else
			{
				Warning( "[PAK Manager] Out of memory, while unpacking file: %s", newFile.GetBuffer() );
				fclose( pFile );
				return;
			}
		}
	}
}

void CPakManagerDlg::ReloadLevelCheck()
{
	if( m_bReloadLevelAfterPakClose )
	{
		AfxMessageBox( "The level will be reloaded now, because a level specific PAK was modified (level.pak).", MB_ICONINFORMATION|MB_OK );
		AfxGetApp()->OpenDocumentFile(GetIEditor()->GetDocument()->GetPathName());
		m_bReloadLevelAfterPakClose = false;
	}
}

BOOL CPakManagerDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_pakEntriesList.InsertColumn( 0, "Filename", LVCFMT_LEFT, 330 );
	m_pakEntriesList.InsertColumn( 1, "Size", LVCFMT_LEFT, 120 );
	m_pakEntriesList.InsertColumn( 2, "Modified", LVCFMT_LEFT, 120 );
	m_pakEntriesList.SetExtendedStyle( LVS_EX_DOUBLEBUFFER|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT );
	m_hCurrentFolder = NULL;

	static CImageList s_imageList;

	s_imageList.DeleteImageList();
	CMFCUtils::LoadTrueColorImageList( s_imageList, IDB_FILES_IMAGE, 16, RGB( 255, 0, 255 ) );
	m_pakEntriesList.SetImageList( &s_imageList, LVSIL_SMALL );

	m_startBrowseFolder = Path::GetExecutableParentDirectory();

	return TRUE;
}

void CPakManagerDlg::OnBnClickedButtonOpenPak()
{
	CFileDialog dlg( TRUE, "*.pak", "", OFN_NOCHANGEDIR, "CryPAK files (*.pak)|*.pak", this );
	
	dlg.m_ofn.lpstrInitialDir = m_startBrowseFolder;

	INT_PTR nResult(IDCANCEL);

	// If your Visual Studio crashes here, just press continue.
	// This is due to an internal exception in KernelBase.dll, exception code 6BA
	// This exception can be ignored, and it's acceptable to disable it from the debuggers
	// exception catching.
	// Steps to disable the exception:
	// 1 – Goto:  Debug->Select Exceptions->Expand the Win32 Exceptions.
	// 2 – Add exception (type: Win32 Exceptions, Name: “The RPC server is unavailable:”, Number: 0x000006BA).
	// 3 – Uncheck the exception.
  nResult = dlg.DoModal();

	if( IDOK ==  nResult)
	{
		CString strFilename =	dlg.GetPathName();

		m_startBrowseFolder = Path::GetPath( dlg.GetPathName() );
		strFilename = Path::MakeGamePath( strFilename );

		ReloadLevelCheck();
		
		if( strstri( PakManager::kPaksNotAllowedToAddFiles, strFilename ) )
		{
			if( IDNO == AfxMessageBox( CString(PakManager::kAddFilesToOfficialPaksMessage) + "\n\nFile: " + strFilename, MB_ICONWARNING|MB_YESNO ) )
				return;
		}

		// try to close the pak if its loaded
		gEnv->pCryPak->ClosePack( strFilename );
		// try to close pak as absolute path
		gEnv->pCryPak->ClosePack( Path::AddSlash(Path::GetExecutableParentDirectory()) + strFilename );

		if( m_pak.GetArchive() )
		{
			// make sure pack is closed in the pak system
			CString strTmp = m_pak.GetArchive()->GetFullPath();
			m_pak.Close();
			// Disabled, for now. (inside EncryptPakFile)
			CGameExporter::EncryptPakFile( strTmp );
		}

		m_pak.Open( dlg.GetPathName(), false );

		if( !m_pak.GetArchive() )
		{
			if( IDNO == AfxMessageBox( PakManager::kPakCannotBeOpenedMessage, MB_ICONEXCLAMATION ) )
				return;
		}

		CString fullPath = Path::ToUnixPath(m_pak.GetArchive()->GetFullPath());
		CString levelPath = Path::ToUnixPath(Path::AddSlash(GetIEditor()->GetLevelFolder()) + "level.pak" );

		if( strstri( fullPath, levelPath ) )
		{
			m_bReloadLevelAfterPakClose = true;
		}

		m_hCurrentFolder = m_hRootFolder = m_pak.GetArchive()->GetRootFolderHandle();
		m_folderStack.clear();
		m_folderNamesStack.clear();
		FillFiles();
	}

	SetDlgTitle();
}

void CPakManagerDlg::OnBnClickedButtonCreatePak()
{
	static int s_archiveCount = 1;

	CString newArchiveName;

	newArchiveName.Format( "newarchive%02d.pak", s_archiveCount++ );
	CFileDialog dlg( FALSE, "*.pak", newArchiveName, OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, "CryPAK files (*.pak)|*.pak", this );

	dlg.m_ofn.lpstrInitialDir = m_startBrowseFolder;

	if( IDOK == dlg.DoModal() )
	{
		m_startBrowseFolder = Path::GetPath( dlg.GetPathName() );

		ReloadLevelCheck();

		if( m_pak.GetArchive() )
		{
			CString strTmp = m_pak.GetArchive()->GetFullPath();
			m_pak.Close();
			// Disabled, for now. (inside EncryptPakFile)
			CGameExporter::EncryptPakFile( strTmp );
		}

		m_pak.Open( dlg.GetPathName(), false );

		if( !m_pak.GetArchive() )
			return;

		m_hCurrentFolder = m_hRootFolder = m_pak.GetArchive()->GetRootFolderHandle();
		m_folderStack.clear();
		m_folderNamesStack.clear();
		FillFiles();
	}
	
	SetDlgTitle();
}

void CPakManagerDlg::OnBnClickedButtonAddFilesToPak()
{
	if( !m_pak.GetArchive() )
	{
		AfxMessageBox( PakManager::kNoPakMessage, MB_ICONERROR );
		return;
	}

	CFileDialog dlg( TRUE, "", "", OFN_ALLOWMULTISELECT|OFN_NOCHANGEDIR, "Any files (*.*)|*.*", this );

	dlg.m_ofn.lpstrInitialDir = m_startBrowseFolder;

	if( IDOK == dlg.DoModal() )
	{
		m_startBrowseFolder = Path::AddSlash( Path::ToUnixPath( Path::GetPath( dlg.GetPathName() ) ) );
		m_startBrowseFolder.MakeLower();

		POSITION pos = dlg.GetStartPosition();
		CString pakPath, pakFile, absFilename;
		CString fullPathToGameDataFolder = gSettings.modName.IsEmpty() ? (Path::GetExecutableParentDirectory() + "/" + CString(PathUtil::GetGameFolder())) : (Path::GetExecutableParentDirectory() + "/Mods/" + gSettings.modName + "/" + CString(PathUtil::GetGameFolder()));
		bool bAddWithFullPath = false;

		fullPathToGameDataFolder = Path::AddSlash(Path::ToUnixPath(fullPathToGameDataFolder));
		fullPathToGameDataFolder.MakeLower();

		if( strstr( m_startBrowseFolder.GetBuffer(), fullPathToGameDataFolder.GetBuffer() ) )
		{
			bAddWithFullPath = ( IDYES == AfxMessageBox( PakManager::kAddFilesWithFullPathMessage, MB_ICONQUESTION|MB_YESNO ) );
		}
		else
		{
			if( IDNO == AfxMessageBox( PakManager::kCannotAddFilesWithFullPathMessage, MB_ICONQUESTION|MB_YESNO ) )
				return;
		}

		GetCurrentPakPath( pakPath );

		while( pos )
		{
			CString filename = dlg.GetNextPathName(pos);

			CFile file(filename,CFile::modeRead);
			char* pData = new char[file.GetLength()];
			
			ASSERT(pData);

			if( !pData && file.GetLength() )
			{
				Warning( "[PAK Manager] Out of memory, while packing file: %s", filename.GetBuffer() );
				file.Close();
				return;
			}

			if( pData )
				file.Read( pData, file.GetLength() );

			absFilename = filename;
			filename = Path::GetFile( filename );

			if( !bAddWithFullPath )
			{
				pakFile = pakPath;
				pakFile += filename;
			}
			else
			{
				CString relPathInsideGameDataFolder = Path::ToUnixPath( absFilename );

				relPathInsideGameDataFolder.MakeLower();
				relPathInsideGameDataFolder.Replace( fullPathToGameDataFolder.GetBuffer(), "" );
				pakFile = pakPath;
				pakFile = Path::AddSlash( pakFile );
				pakFile += relPathInsideGameDataFolder;
			}

			if( !m_pak.UpdateFile( pakFile.GetBuffer(), pData, file.GetLength() ) )
			{
				Warning( "[PAK Manager] Error while packing file: %s", pakFile.GetBuffer() );
			}

			file.Close();
			delete [] pData;
		}
		
		_flushall();
		FillFiles();
	}
}

void CPakManagerDlg::OnBnClickedButtonExtractFilesFromPak()
{
	if( !m_pak.GetArchive() )
	{
		AfxMessageBox( PakManager::kNoPakMessage, MB_ICONERROR );
		return;
	}

	BROWSEINFO bi;

	ZeroMemory( &bi, sizeof(bi) );

	bi.hwndOwner = m_hWnd;
	bi.lpszTitle = "Choose a destination folder where to unpack the files";
	bi.ulFlags = BIF_NEWDIALOGSTYLE;

	PCIDLIST_ABSOLUTE ret = SHBrowseForFolder( &bi );

	if( !ret )
		return;

	char path[MAX_PATH];

	if( SHGetPathFromIDList( ret, path ) )
	{
		bool bFullPath = false;
		CString thePath = path;

		if( IDYES == AfxMessageBox( PakManager::kUnpackFullPathMessage, MB_YESNO ) )
		{
			bFullPath = true;
			CString pakPath;

			GetCurrentPakPath( pakPath );
			thePath = Path::AddSlash( thePath );
			thePath += Path::AddSlash( pakPath );
			CFileUtil::CreateDirectory( thePath );
		}

		UnpackFilesRecursive( m_hCurrentFolder, thePath, true );
	}

	m_stStatus.SetWindowText( "Ready" );
	m_pakProgress.SetPos( 0 );
}

void CPakManagerDlg::OnBnClickedButtonDeleteFilesFromPak()
{
	if( !m_pak.GetArchive() )
	{
		AfxMessageBox( PakManager::kNoPakMessage, MB_ICONERROR );
		return;
	}

	CString deleteMsg = "Delete the selected entries?";

	if( IDNO == AfxMessageBox( deleteMsg, MB_ICONWARNING|MB_YESNO ) )
	{
		return;
	}

	std::vector<CString> filesToDelete;
	CString pakPath;

	GetCurrentPakPath( pakPath );

	for( size_t i = 0, iCount = m_pakEntriesList.GetItemCount(); i < iCount; ++i )
	{
		if( LVIS_SELECTED & m_pakEntriesList.GetItemState( i, LVIS_SELECTED ) )
		{
			LVITEM li;

			ZeroMemory(&li,sizeof(li));
			li.iItem = i;
			li.stateMask = LVIS_SELECTED;
			li.mask = LVIF_IMAGE|LVIF_PARAM;
			m_pakEntriesList.GetItem( &li );

			CString itemText = m_pakEntriesList.GetItemText( i, 0 );

			if( itemText == ".." )
				continue;

			CString fullPath = pakPath;
			
			fullPath += itemText;

			if( li.iImage == eFileTypeIconIndex_Folder )
			{
				m_pak.RemoveDir( fullPath );
			}
			else
			{
				m_pak.RemoveFile( fullPath );
			}
		}
	}

	_flushall();
	FillFiles();
}

void CPakManagerDlg::OnBnClickedButtonClose()
{
	OnClose();
	EndDialog(0);
}

void CPakManagerDlg::HandleEnterKeyOnList( int index )
{
	if( index < 0 )
		return;

	if( index >= m_pakEntriesList.GetItemCount() )
		return;

	LVITEM li;

	ZeroMemory(&li,sizeof(li));
	li.iItem = index;
	li.mask = LVIF_IMAGE|LVIF_PARAM;
	m_pakEntriesList.GetItem( &li );
	CString itemText = m_pakEntriesList.GetItemText( index, 0 );

	if( itemText == ".." )
	{
		if( !m_folderStack.empty() )
		{
			m_hCurrentFolder = m_folderStack.back();
			m_folderStack.pop_back();
			m_folderNamesStack.pop_back();
			FillFiles();
		}
	}
	else
	if( li.iImage == eFileTypeIconIndex_Folder )
	{
		m_folderStack.push_back( m_hCurrentFolder );
		m_folderNamesStack.push_back( itemText );
		m_hCurrentFolder = (ICryArchive::Handle)li.lParam;
		FillFiles();
	}
}

void CPakManagerDlg::OnNMDblclkListFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	if( !m_pak.GetArchive() )
	{
		AfxMessageBox( PakManager::kNoPakMessage, MB_ICONERROR );
		return;
	}

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	HandleEnterKeyOnList( pNMItemActivate->iItem );

	*pResult = 0;
}

void CPakManagerDlg::OnLvnKeydownListFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if( pLVKeyDown->wVKey == VK_BACK )
	{
		if( !m_folderStack.empty() )
		{
			m_hCurrentFolder = m_folderStack.back();
			m_folderStack.pop_back();
			m_folderNamesStack.pop_back();
			FillFiles();
		}
	}

	if( pLVKeyDown->wVKey == VK_DELETE )
	{
		OnBnClickedButtonDeleteFilesFromPak();
	}

	if( pLVKeyDown->wVKey == VK_RETURN )
	{
		HandleEnterKeyOnList( m_pakEntriesList.GetSelectionMark() );
	}

	*pResult = 0;
}

void CPakManagerDlg::OnClose()
{
	ReloadLevelCheck();

	if( m_pak.GetArchive() )
	{
		CString strTmp = m_pak.GetArchive()->GetFullPath();
		m_pak.Close();
		CGameExporter::EncryptPakFile( strTmp );
	}

	__super::OnClose();
}

void CPakManagerDlg::OnBnClickedButtonAddFoldersToPak()
{
	if( !m_pak.GetArchive() )
	{
		AfxMessageBox( PakManager::kNoPakMessage, MB_ICONERROR );
		return;
	}

	BROWSEINFO bi;

	ZeroMemory( &bi, sizeof(bi) );

	bi.hwndOwner = m_hWnd;
	bi.lpszTitle = "Choose folder to add to pak file";
	bi.ulFlags = BIF_NEWDIALOGSTYLE;

	PCIDLIST_ABSOLUTE ret = SHBrowseForFolder( &bi );

	if( !ret )
		return;

	char path[MAX_PATH];

	if( SHGetPathFromIDList( ret, path ) )
	{
		CFileUtil::FileArray files;
		CString pakPath, strProgress, filename, pakFilename, absFilename;

		m_startBrowseFolder = Path::AddSlash(Path::ToUnixPath( Path::GetPath( path ) ));
		m_startBrowseFolder.MakeLower();

		CString fullPathToGameDataFolder = gSettings.modName.IsEmpty() ? (Path::GetExecutableParentDirectory() + "/" + CString(PathUtil::GetGameFolder())) : (Path::GetExecutableParentDirectory() + "/Mods/" + gSettings.modName + "/" + CString(PathUtil::GetGameFolder()));
		bool bAddWithFullPath = false;

		fullPathToGameDataFolder = Path::AddSlash(Path::ToUnixPath(fullPathToGameDataFolder));
		fullPathToGameDataFolder.MakeLower();

		if( strstr( m_startBrowseFolder.GetBuffer(), fullPathToGameDataFolder.GetBuffer() ) )
		{
			bAddWithFullPath = ( IDYES == AfxMessageBox( PakManager::kAddFilesWithFullPathMessage, MB_ICONQUESTION|MB_YESNO ) );
		}
		else
		{
			if( IDNO == AfxMessageBox( PakManager::kCannotAddFilesWithFullPathMessage, MB_ICONQUESTION|MB_YESNO ) )
				return;
		}

		// get our current pak relative path
		GetCurrentPakPath( pakPath );

		// scan given folder to be added to pak
		if( CFileUtil::ScanDirectory( path, "*.*", files ) )
		{
			m_pakProgress.SetRange( 0, files.size() );

			// lets ask user if he adds more than N files, just to make sure it wasnt a mistake

			const int kFileCountPackWarningThreshold = 50;

			if( files.size() >= kFileCountPackWarningThreshold )
			{
				CString str;

				str.Format( "Are you sure you want to add this folder? (%d files total)", files.size() );
				
				if( AfxMessageBox( str, MB_ICONQUESTION|MB_YESNO ) == IDNO )
					return;
			}

			for( size_t i = 0, iCount = files.size(); i < iCount; ++i )
			{
				CFileUtil::FileDesc& desc = files[i];
				
				filename = path;
				
				// make filename for real file on disk
				filename = Path::AddBackslash( filename );
				filename += desc.filename;
				absFilename = filename;

				if( !bAddWithFullPath )
				{
					// make filename for relative in pak
					pakFilename = pakPath;
					pakFilename += desc.filename;
				}
				else
				{
					CString relPathInsideGameDataFolder = Path::ToUnixPath( absFilename );

					relPathInsideGameDataFolder.MakeLower();
					relPathInsideGameDataFolder.Replace( fullPathToGameDataFolder.GetBuffer(), "" );
					pakFilename = pakPath;
					pakFilename = Path::AddSlash( pakFilename );
					pakFilename += relPathInsideGameDataFolder;
				}

				// show progress
				strProgress.Format( "Packing: %s...", pakFilename.GetBuffer() );
				m_stStatus.SetWindowText( strProgress );
				m_pakProgress.SetPos( i );
				m_stStatus.UpdateWindow();
				m_pakProgress.UpdateWindow();

				// open file on disk, read it, and pak it
				CFile file(filename,CFile::modeRead);
				char* pData = new char[file.GetLength()];

				ASSERT(pData);

				if( !pData && file.GetLength() )
				{
					Warning( "[PAK Manager] Out of memory or file is empty, while packing file: %s", pakFilename.GetBuffer() );
					file.Close();
					return;
				}

				if( pData )
					file.Read( pData, file.GetLength() );

				if( !m_pak.UpdateFile( pakFilename.GetBuffer(), pData, file.GetLength() ) )
				{
					Warning( "[PAK Manager] Error while packing file: %s", pakFilename.GetBuffer() );
				}

				// cleanup
				file.Close();
				delete [] pData;
			}

			// force flush file caches
			_flushall();
			m_stStatus.SetWindowText( "Reloading entries..." );
			m_stStatus.UpdateWindow();
			FillFiles();

			// idle again
			m_stStatus.SetWindowText( "Ready" );
			m_pakProgress.SetPos( 0 );
		}
	}
}

void CPakManagerDlg::OnOK()
{
	// no handling, keep dialog opened
}

BOOL CPakManagerDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// lets handle here the ENTER on pak entries list
	if( nID == IDOK && &m_pakEntriesList == GetFocus() )
	{
		HandleEnterKeyOnList( m_pakEntriesList.GetSelectionMark() );
		return TRUE;
	}

	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
