////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002-2012.
// -------------------------------------------------------------------------
//  File name:   LevelFileDialog.cpp
//  Version:     v1.00
//  Created:     25/01/2012 by Axel.
//  Compilers:   Visual Studio 2010
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LevelFileDialog.h"
#include "Dialogs/Generic/StringInputDialog.h"
#include "Dialogs/Generic/UserOptions.h"

#ifdef SEG_WORLD
#include "SegmentedWorld/SegmentedWorldManager.h"
#endif // SEG_WORLD

static const char lastLoadPathFilename[] = "lastLoadPath.preset";

// File name extension for the main level file
static const char kLevelExtension[] = "cry";

#ifdef SEG_WORLD
// File name extension for the sw level file, while scry means Segemented-Cry file.
static const char kSWLevelExtension[] = "scry";
#endif // SEG_WORLD

// Folder in which levels are stored
static const char kLevelsFolder[] = "Levels";

// List of folder names that are used to detect a level folder
static const char *kLevelFolderNames[] = 
{
	"Layers",
	"Minimap",
	"LevelData"
};

// List of files that are used to detect a level folder
static const char *kLevelFileNames[] = 
{
	"level.pak", 
	"terraintexture.pak", 
	"filelist.xml", 
	"levelshadercache.pak"
};

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CLevelFileDialog, CXTResizeDialog)
	CLevelFileDialog::CLevelFileDialog( bool bOpenFileDialog ) 
	: CXTResizeDialog( IDD_LEVEL_FILE_DIALOG, NULL ), m_bOpenDialog( bOpenFileDialog )
{
}

BEGIN_MESSAGE_MAP(CLevelFileDialog, CXTResizeDialog)
	ON_BN_CLICKED(IDC_NEW_FOLDER, OnNewFolder)
	ON_NOTIFY(TVN_SELCHANGING, IDC_TREE, OnTreeSelectionChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE, OnTreeDoubleClicked)
	ON_EN_CHANGE(IDC_FILTER, OnFilterChanged)
END_MESSAGE_MAP()

void CLevelFileDialog::DoDataExchange( CDataExchange* pDX )
{
	CXTResizeDialog::DoDataExchange(pDX);	
	DDX_Control( pDX, IDC_TREE, m_tree );
	DDX_Control( pDX, IDC_NAME, m_nameEdit );
	DDX_Control( pDX, IDC_FILTER, m_filterEdit );
}

enum ETreeImage
{
	eTreeImage_Folder = 0,
	eTreeImage_Level = 2
};

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::OnOK()
{
	CString enteredPath;
	m_nameEdit.GetWindowText( enteredPath );	
	
	enteredPath = enteredPath.Trim();
	enteredPath = Path::RemoveBackslash( enteredPath );
	CString levelPath = Path::GetGameFolder() + "/" + kLevelsFolder + "/" + enteredPath;	

	if ( m_bOpenDialog )
	{
		std::vector<CString> levelFiles;
		if ( CheckLevelFolder( levelPath, &levelFiles ) && levelFiles.size() >= 1)	
		{
			// A level folder was entered. Prefer the .cry file with the 
			// folder name, otherwise pick the first one in the list
			CString needle = Path::GetFileName( levelPath ) + "." + kLevelExtension;
			auto iter = std::find( levelFiles.begin(), levelFiles.end(),  needle );

			if ( iter != levelFiles.end() )
				m_fileName = levelPath + "/" + *iter;
			else
			{
#ifdef SEG_WORLD
				needle = Path::GetFileName( levelPath ) + "." + kSWLevelExtension;
				iter = std::find( levelFiles.begin(), levelFiles.end(),  needle );

				if (iter != levelFiles.end())
					m_fileName = levelPath + "/" + *iter;
				else
#endif // SEG_WORLD
					m_fileName = levelPath + "/" + levelFiles[0]; 
			}
		}
		else
		{
			// Otherwise try to directly load the specified file (backward compatibility)
			m_fileName = levelPath;
		}

		// Extension must be .cry/.scry at this point and the file must exist
		bool isInvalidFileExtension = Path::GetExt(m_fileName) != kLevelExtension 
#ifdef SEG_WORLD
			&& Path::GetExt(m_fileName) != kSWLevelExtension
#endif // SEG_WORLD
			;

		if (isInvalidFileExtension || !CFileUtil::FileExists(m_fileName))
		{
			MessageBox ("Please enter a valid level name", "Error", MB_OK | MB_ICONERROR );
			return;
		}
	}
	else
	{
		// Make sure that this folder can be used as a level folder:
		// - It is a valid level path
		// - It isn't already a file
		// - There are no other level folders under it
		if ( !CryStringUtils::IsValidFileName( Path::GetFileName(levelPath) ) )
		{
			MessageBox("Please enter a valid level name (standard English alphanumeric characters only)", "Error", MB_OK | MB_ICONERROR);
			return;
		}

		if ( !ValidateLevelPath( enteredPath ) )
		{
			MessageBox( "Please enter a valid level location. "
				"You cannot save levels inside levels.", "Error", MB_OK | MB_ICONERROR );
			return;
		}

		if ( CFileUtil::FileExists( levelPath ) )
		{
			MessageBox( "A file with that name already exists", "Error", MB_OK | MB_ICONERROR );
			return;
		}
		
		if ( CheckSubFoldersForLevelsRec( levelPath ) )
		{
			MessageBox( "You cannot save a level in a folder with sub "
				"folders that contain levels", "Error", MB_OK | MB_ICONERROR );
			return;
		}

		// Check if there is already a level folder at that 
		// location, if so ask before for overwriting it
		if ( CheckLevelFolder( levelPath ) )
		{
			CString message = "Do you really want to overwrite '" + enteredPath + "'?";
			if ( MessageBox( message, "Warning", MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2 ) != IDYES )
				return;
		}

		auto fileExtension =
#ifdef SEG_WORLD
			GetIEditor()->IsValidSegmentedWorldDoc() ? kSWLevelExtension : 
#endif //SEG_WORLD
			kLevelExtension;
		m_fileName = levelPath + "/" + Path::GetFileName(levelPath) + "." + fileExtension;
	}

	SaveLastUsedLevelPath( m_fileName );
	CXTResizeDialog::OnOK();
}

//////////////////////////////////////////////////////////////////////////
BOOL CLevelFileDialog::OnInitDialog()
{
	const int kNewFolderButtonSpacing = 25;

	CXTResizeDialog::OnInitDialog();

	if ( m_bOpenDialog )
	{
		SetWindowText( "Open Level" );

		GetDlgItem( IDC_NEW_FOLDER )->ShowWindow( FALSE );	

		CRect rect;
		m_tree.GetWindowRect( rect );
		ScreenToClient( rect );
		rect.bottom += kNewFolderButtonSpacing;
		m_tree.MoveWindow( rect );

		GetDlgItem( IDOK )->SetWindowText( "Open" );
	}
	else
	{
		SetWindowText( "Save Level As ");
		GetDlgItem( IDOK )->SetWindowText( "Save" );
	}

	CMFCUtils::LoadTrueColorImageList( m_imageList, IDB_ENTITY_TREE, 16, RGB(255, 0, 255) );

	m_tree.SetImageList( &m_imageList, TVSIL_NORMAL );
	m_tree.SetIndent( 0 );
	ReloadTree();

	SetResize( IDC_TREE, SZ_RESIZE( 1.0f ));
	SetResize( IDC_NEW_FOLDER, SZ_REPOS( 1.0f ) );	
	SetResize( IDC_LEVEL_NAME_STATIC, SZ_VERREPOS( 1.0f ) );
	SetResize( IDC_NAME, CXTResizeRect( 0.0f, 1.0f, 1.0f, 1.0f ) );
	SetResize( IDOK, SZ_REPOS( 1.0f ) );
	SetResize( IDCANCEL, SZ_REPOS( 1.0f ) );
	
	m_tree.ModifyStyle( 0, CS_DBLCLKS );

	LoadLastUsedLevelPath();

	m_filterEdit.SetFocus();

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::OnTreeSelectionChanged( NMHDR* pNMHDR, LRESULT* pResult )
{
	LPNMTREEVIEW pNMTreeView = (LPNMTREEVIEW)pNMHDR;

	if(pNMHDR)
	{
		const CString itemFolder = m_itemFolders[ pNMTreeView->itemNew.hItem ];
		m_nameEdit.SetWindowText( itemFolder );
	}
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::OnTreeDoubleClicked( NMHDR * pNotifyStruct, LRESULT* result )
{
	HTREEITEM selectedItem = m_tree.GetSelectedItem();

	if(selectedItem == NULL)
		return;

	int itemImage;
	int selectedImage;
	m_tree.GetItemImage( selectedItem, itemImage, selectedImage );

	if ( itemImage != eTreeImage_Folder )
		OnOK();
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::OnNewFolder()
{
	HTREEITEM selectedItem = m_tree.GetSelectedItem();

	if(selectedItem == NULL)
	{
		MessageBox( "Please select a folder first", "Error", MB_OK | MB_ICONERROR );
		return;
	}

	int itemImage;
	int selectedImage;
	m_tree.GetItemImage( selectedItem, itemImage, selectedImage );

	// Creating folders is not allowed in level folders
	if ( itemImage == eTreeImage_Folder ) 
	{
		const CString itemFolder = m_itemFolders[selectedItem];
		
		CStringInputDialog inputDialog;
		inputDialog.SetTitle( "Please enter a folder name" );
		if ( inputDialog.DoModal() == IDOK )
		{
			const CString newFolderName = inputDialog.GetResultingText();			
			const CString newFolderPath = Path::GetGameFolder() + "/" + kLevelsFolder + "/" + itemFolder + newFolderName;

			if ( !CryStringUtils::IsValidFileName( newFolderName ) )
			{
				MessageBox("Please enter a single, valid folder name (standard English alphanumeric characters only)", "Error", MB_OK | MB_ICONERROR);
				return;
			}

			if ( CFileUtil::PathExists( newFolderPath ) )
			{				
				MessageBox("Folder already exists", "Warning", MB_OK | MB_ICONWARNING);				
				return;
			}

			// The trailing / is important, otherwise CreatePath doesn't work
			if ( !CFileUtil::CreatePath( newFolderPath + "/") )
			{		
				MessageBox("Could not create folder", "Error", MB_OK | MB_ICONERROR);				
				return;
			}

			HTREEITEM newTreeItem = m_tree.InsertItem( newFolderName, eTreeImage_Folder, eTreeImage_Folder, selectedItem );

			const CString newItemFolder = m_itemFolders[ selectedItem ] + newFolderName + "/";
			m_itemFolders[ newTreeItem ] = newItemFolder;
			m_nameEdit.SetWindowText( newItemFolder );

			m_tree.SortChildren( selectedItem );
			m_tree.SelectSetFirstVisible( newTreeItem );
		}
	}
	else
	{
		MessageBox( "Please select a folder first", "Error", MB_OK | MB_ICONERROR );
		return;
	}
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::OnFilterChanged()
{
	m_filterEdit.GetWindowText( m_filter );
	m_filter.MakeLower();
	ReloadTree();
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::ReloadTree()
{	
	CString levelsFolder = Path::GetGameFolder() + "/" + kLevelsFolder;

	m_tree.DeleteAllItems();
	m_itemFolders.clear();
	
	bool bSelectNextMatchingFilter = !m_filter.IsEmpty();
	HTREEITEM firstMatching = TVI_ROOT;
	ReloadTreeRec( levelsFolder, kLevelsFolder, "", TVI_ROOT, bSelectNextMatchingFilter, firstMatching );	

	if (firstMatching != TVI_ROOT)
	{
		m_tree.Expand( firstMatching, TVE_EXPAND );
		m_tree.Select( firstMatching, TVGN_CARET );
	}
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::ReloadTreeRec( const CString currentFolder, const CString currentFolderName, 
	const CString currentItemPath, const HTREEITEM currentTreeItem, bool &bSelectNextMatchingFilter, HTREEITEM &firstMatching )
{
	CFileEnum fileEnum;
	__finddata64_t fileData;	
	
	std::vector<CString> levelFiles;
	bool bIsLevelFolder = CheckLevelFolder( currentFolder, &levelFiles );

	// Recurse to sub folders if this is not a level folder
	// Also recurse to sub folders if we detected a level folder 
	// but there are no level files in it in open mode
	if ( !bIsLevelFolder || (m_bOpenDialog && levelFiles.size() == 0) )
	{
		std::vector<CString> subFolders;

		for ( bool bFoundFile = fileEnum.StartEnumeration( currentFolder, "*", &fileData );
			bFoundFile; bFoundFile = fileEnum.GetNextFile( &fileData ) )
		{
			const CString fileName = fileData.name;

			// Have we found a folder?
			if ( fileData.attrib & _A_SUBDIR )
			{
				// Skip the parent folder entries
				if ( fileName == "." || fileName == ".." )
					continue;	
				
				subFolders.push_back( fileName );
			}
		}

		HTREEITEM subTreeItem = m_tree.InsertItem( currentFolderName, eTreeImage_Folder, eTreeImage_Folder, currentTreeItem );
		m_itemFolders[subTreeItem] = currentFolder;

		const CString currentFolderSlash = Path::AddSlash( currentFolder );
		const CString currentItemPathSlash = Path::AddSlash( currentItemPath );

		m_itemFolders[ subTreeItem ] = currentItemPathSlash;

		if ( currentTreeItem == TVI_ROOT )
		{
			firstMatching = subTreeItem;
		}

		for (auto iter = subFolders.begin(); iter != subFolders.end(); ++iter )
		{
			ReloadTreeRec( currentFolderSlash + *iter, *iter, currentItemPathSlash + *iter,
				subTreeItem, bSelectNextMatchingFilter, firstMatching );	
		}
	}
	else
	{
		CString currentFolderNameLowerCase = currentFolderName;
		currentFolderNameLowerCase.MakeLower();

		// Filter items if a filter string was entered
		if (!m_filter.IsEmpty() && currentFolderNameLowerCase.Find(m_filter) == -1)
		{
			return;
		}

		HTREEITEM subTreeItem = m_tree.InsertItem( currentFolderName, eTreeImage_Level, eTreeImage_Level, currentTreeItem );
		m_itemFolders[ subTreeItem ] = currentItemPath;

		if ( currentTreeItem == TVI_ROOT )
		{
			firstMatching = subTreeItem;
		}
		else if ( bSelectNextMatchingFilter && bIsLevelFolder)
		{
			firstMatching = subTreeItem;
			bSelectNextMatchingFilter = false;
		}

		// Support for legacy folder structure: Multiple cry files in level folder
		if ( m_bOpenDialog && levelFiles.size() > 1 )
		{
			for (auto iter = levelFiles.begin(); iter != levelFiles.end(); ++iter)
			{
				HTREEITEM subLevelTreeItem = m_tree.InsertItem( *iter, eTreeImage_Level, eTreeImage_Level, subTreeItem );
				m_itemFolders[ subLevelTreeItem ] = currentItemPath + "/" + *iter;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Heuristic to detect a level folder, also returns all .cry files in it
//////////////////////////////////////////////////////////////////////////
bool CLevelFileDialog::CheckLevelFolder( const CString folder, std::vector<CString> *levelFiles )
{
	CFileEnum fileEnum;
	__finddata64_t fileData;
	bool bIsLevelFolder = false;

	for ( bool bFoundFile = fileEnum.StartEnumeration( folder, "*", &fileData );
		bFoundFile; bFoundFile = fileEnum.GetNextFile( &fileData ) )
	{
		const CString fileName = fileData.name;

		// Have we found a folder?
		if ( fileData.attrib & _A_SUBDIR )
		{
			// Skip the parent folder entries
			if ( fileName == "." || fileName == ".." )
				continue;	

			for ( unsigned int i = 0; i < sizeof( kLevelFolderNames ) / sizeof( char* ); ++i ) 
			{
				if ( fileName == kLevelFolderNames[i] )
					bIsLevelFolder = true;
			}
		}
		else
		{
			CString ext = Path::GetExt( fileName );

			if (ext == kLevelExtension 
#ifdef SEG_WORLD
				|| ext == kSWLevelExtension
#endif //SEG_WORLD
				)
			{
				bIsLevelFolder = true;

				if ( levelFiles)
					levelFiles->push_back( fileName );
			}
			
			for ( unsigned int i = 0; i < sizeof( kLevelFileNames ) / sizeof( char* ); ++i )
			{
				if ( fileName == kLevelFileNames[i] )
					bIsLevelFolder = true;
			}
		}
	}

	return bIsLevelFolder;
}

//////////////////////////////////////////////////////////////////////////
// Checks if there are levels in the sub folders of a folder
//////////////////////////////////////////////////////////////////////////
bool CLevelFileDialog::CheckSubFoldersForLevelsRec( const CString folder, bool bRoot )
{
	if ( !bRoot && CheckLevelFolder( folder ) )
		return true;		

	CFileEnum fileEnum;
	__finddata64_t fileData;
	bool bIsLevelFolder = false;	

	for ( bool bFoundFile = fileEnum.StartEnumeration( folder, "*", &fileData );
		bFoundFile; bFoundFile = fileEnum.GetNextFile( &fileData ) )
	{
		const CString fileName = fileData.name;

		// Have we found a folder?
		if ( fileData.attrib & _A_SUBDIR )
		{
			// Skip the parent folder entries
			if ( fileName == "." || fileName == ".." )
				continue;	

			// Check if this sub folder contains a level
			if ( CheckSubFoldersForLevelsRec( folder + "/" + fileName, false ) )
				return true; 
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// Checks if a given path is a valid level path
//////////////////////////////////////////////////////////////////////////
bool CLevelFileDialog::ValidateLevelPath( const CString levelPath )
{
	if ( levelPath.IsEmpty() || Path::GetExt( levelPath ) != "" )
		return false;

	// Check for invalid characters
	for ( int i = 0; i < levelPath.GetLength(); ++i )
	{
		char c = levelPath[i];
		if (c == '<' || c == '>' || c == '.' || c == ':' 
			|| c == '|' || c == '?' || c == '*')
			return false;
	}

	// Split path
	std::vector<CString> splittedPath;
	int currentPos = 0;
	for ( CString token = levelPath.Tokenize( "/\\", currentPos ); !token.IsEmpty(); 
		token = levelPath.Tokenize( "/\\", currentPos ) ) 
	{
		splittedPath.push_back(token);
	}

	// This shouldn't happen, but be careful
	if ( splittedPath.empty() )
		return false;

	// Make sure that no folder before the last in the name contains a level
	if ( splittedPath.size() > 1 )
	{
		CString currentPath = Path::GetGameFolder() + "/" + kLevelsFolder;
		for ( size_t i = 0; i < splittedPath.size() - 1; ++i )
		{
			currentPath += "/" + splittedPath[i];

			if ( CheckLevelFolder ( currentPath ) )
				return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::SaveLastUsedLevelPath( const CString path )
{
	const CString settingPath = CString( GetIEditor()->GetUserFolder() ) + lastLoadPathFilename;

	CString lastLoadedFileName;
	m_nameEdit.GetWindowText( lastLoadedFileName );

	XmlNodeRef lastUsedLevelPathNode = XmlHelpers::CreateXmlNode( "lastusedlevelpath" );
	lastUsedLevelPathNode->setAttr( "path", lastLoadedFileName );

	XmlHelpers::SaveXmlNode( lastUsedLevelPathNode, settingPath );
}

//////////////////////////////////////////////////////////////////////////
void CLevelFileDialog::LoadLastUsedLevelPath()
{
	const CString settingPath = CString( GetIEditor()->GetUserFolder() ) + lastLoadPathFilename;
	if( !CFileUtil::FileExists( settingPath ) )
		return;

	XmlNodeRef lastUsedLevelPathNode = XmlHelpers::LoadXmlFromFile( settingPath );
	if( lastUsedLevelPathNode == NULL )
		return;

	CString lastLoadedFileName;
	lastUsedLevelPathNode->getAttr( "path", lastLoadedFileName );

	std::vector<CString> segments = Path::SplitIntoSegments( lastLoadedFileName );
	
	// Select path as far down as possible in tree
	HTREEITEM currentItem = m_tree.GetRootItem();
	for( auto iter = segments.begin(); iter != segments.end() && m_tree.ItemHasChildren( currentItem ); ++iter )
	{
		HTREEITEM nextItem = NULL;
		for( HTREEITEM childItem = m_tree.GetChildItem( currentItem ); childItem != NULL; childItem = m_tree.GetNextItem( childItem, TVGN_NEXT ) )
		{
			if( *iter == m_tree.GetItemText( childItem ) )
			{
				if( iter != segments.end() - 1 )
					m_tree.Expand( childItem, TVE_EXPAND );
				m_tree.Select( childItem, TVGN_CARET );
				nextItem = childItem;
			}
		}
		currentItem = nextItem;
	}

	m_nameEdit.SetWindowText( lastLoadedFileName );
}