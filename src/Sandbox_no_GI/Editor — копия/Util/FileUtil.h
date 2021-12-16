////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   fileutil.h
//  Version:     v1.00
//  Created:     13/9/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: 
//	HINT - Add a file traversal function with a callback.
////////////////////////////////////////////////////////////////////////////

#ifndef __fileutil_h__
#define __fileutil_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "CryThread.h"

/* File types used for File Open dialogs.
 *	
 */
enum ECustomFileType
{
	EFILE_TYPE_ANY,
	EFILE_TYPE_GEOMETRY,
	EFILE_TYPE_TEXTURE,
	EFILE_TYPE_SOUND,
	EFILE_TYPE_DIALOG,
	EFILE_TYPE_LAST,
};

//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CFileUtil
{
public:
	struct FileDesc
	{
		CString filename;
		unsigned int attrib;
		time_t  time_create;    //! -1 for FAT file systems
		time_t  time_access;    //! -1 for FAT file systems
		time_t  time_write;
		int64 size;
	};
	enum ETextFileType
	{
		FILE_TYPE_SCRIPT,
		FILE_TYPE_SHADER
	};

	enum ECopyTreeResult
	{
		ETREECOPYOK,
		ETREECOPYFAIL,
		ETREECOPYUSERCANCELED,
		ETREECOPYUSERDIDNTCOPYSOMEITEMS,
	};

	typedef std::vector<FileDesc> FileArray;
	typedef bool (*ScanDirectoryUpdateCallBack)(const CString& msg);

	static bool ScanDirectory( const CString &path,const CString &fileSpec,FileArray &files, bool recursive=true, bool addDirAlso=false, ScanDirectoryUpdateCallBack updateCB=NULL );
	//static bool ScanDirectory( const CString &startDirectory,const CString &searchPath,const CString &fileSpecZ,FileArray &files, bool recursive=true );

	static bool CompileLuaFile( const char *luaFilename );
	static void EditTextFile( const char *txtFile,int line=0,ETextFileType fileType=FILE_TYPE_SCRIPT, bool bUseGameFolder=true );

	//! Open file selection dialog.
	static bool SelectFile( const CString &fileSpec,const CString &searchFolder,CString &fullFileName );
	//! Open file selection dialog.
	static bool SelectFiles( const CString &fileSpec,const CString &searchFolder,std::vector<CString> &files );
	
	//! Display OpenFile dialog and allow to select multiple files.
	//! @return true if selected, false if canceled.
	//! @outputFile Inputs and Outputs filename.
	static bool SelectSingleFile( ECustomFileType fileType,CString &outputFile,const CString &filter="",const CString &initialDir="" );

	//! Display OpenFile dialog and allow to select multiple files.
	//! @return true if selected, false if canceled.
	static bool SelectMultipleFiles( ECustomFileType fileType,std::vector<CString> &files,const CString &filter="",const CString &initialDir="" );

	static bool SelectSaveFile( const CString &fileFilter,const CString &defaulExtension,const CString &startFolder,CString &fileName );
	
	//! If file is read-only ask user if he wants to overwrite it.
	//! If yes file is deleted.
	//! @return True if file was deleted.
	static bool OverwriteFile( const char *filename );

	//////////////////////////////////////////////////////////////////////////
	// Interface to Source safe.
	//////////////////////////////////////////////////////////////////////////
	//! Checks out the file from source safe.
	static bool CheckoutFile( const char *filename );

	//! Checks in the file to source safe.
	static bool CheckinFile( const char *filename );

	//! Creates this directory.
	static void CreateDirectory( const char *dir );

	//! Makes a backup file.
	static void BackupFile( const char *filename );

	//! Makes a backup file, marked with a datestamp, e.g. myfile.20071014.093320.xml 
	//! If bUseBackupSubDirectory is true, moves backup file into a relative subdirectory "backups"
	static void BackupFileDated( const char *filename, bool bUseBackupSubDirectory=false );

	// ! Added deltree as a copy from the function found in Crypak.
	static bool Deltree(const char *szFolder, bool bRecurse);

	// Checks if a file or directory exist.
	// We are using 3 functions here in order to make the names more instructive for the programmers.
	// Those functions only work for OS files and directories.
	static bool   Exists(const CString& strPath,bool boDirectory);
	static bool   FileExists(const CString& strFilePath);
	static bool   PathExists(const CString& strPath);

	// This function should be used only with physical files.
	static bool   IsFileExclusivelyAccessable(const CString& strFilePath);

	// Creates the entire path, if needed.
	static bool   CreatePath(const CString& strPath);

	// Attempts to delete a file (if read only it will set its attributes to normal first).
	static bool   DeleteFile(const CString& strPath);

	// Attempts to remove a directory (if read only it will set its attributes to normal first).
	static bool		RemoveDirectory(const CString& strPath);

	// Copies all the elements from the source directory to the target directory.
	// It doesn't copy the source folder to the target folder, only it's contents.
	// THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
	static ECopyTreeResult   CopyTree(const CString& strSourceDirectory,const CString& strTargetDirectory,bool boRecurse=true,bool boConfirmOverwrite=false);

	//////////////////////////////////////////////////////////////////////////
	static ECopyTreeResult   CopyFile(const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite=false);

	// As we don't have a FileUtil interface here, we have to duplicate some code :-( in order to keep
	// function calls clean.
	// Moves all the elements from the source directory to the target directory.
	// It doesn't move the source folder to the target folder, only it's contents.
	// THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
	static ECopyTreeResult   MoveTree(const CString& strSourceDirectory,const CString& strTargetDirectory,bool boRecurse=true,bool boConfirmOverwrite=false);

	//
	static ECopyTreeResult   MoveFile(const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite=false);

	//! Display a 'Smart' OpenFile dialog and allow to select a file.
	//! @return true if selected, false if canceled.
	//! @fileType ECustomFileType enum value used in case of switching to the custom file dialog.
	//! @outputFile Inputs and Outputs filename.
	//! @initialSearchTerm Initial search terms which will be inputted by default.
	//! @initialDir An initial directory to begin.
	//! @filter A filter string used in case of switching to the custom file dialog.
	static bool SmartSelectSingleFile(ECustomFileType fileType, CString &outputFile, 
		const CString& initialSearchTerm, const CString& initialDir="", const CString& filter="");

	struct ExtraMenuItems
	{
		std::vector<CString> names;
		int selectedIndexIfAny;

		ExtraMenuItems() : selectedIndexIfAny(-1) {}
		
		int AddItem(const CString& name)
		{
			names.push_back(name);
			return names.size() - 1;
		}
	};

	// Show Popup Menu with file commands include Source Control commands
	// filename: a name of file without path
	// fullGamePath: a game path to folder like "/Game/Objects" without filename
	// wnd: pointer to window class, can be NULL
	// isSelected: output value indicated if Select menu item was chosen, if pointer is 0 - no Select menu item.
	// pItems: you can specify additional menu items and get the result of selection using this parameter.
	// return false if source control operation failed
	static bool PopupMenu(const char* filename, const char* fullGamePath, CWnd* wnd, bool* pIsSelected=0,
												ExtraMenuItems *pItems=NULL);

private:
	// True means to use the custom file dialog, false means to use the smart file open dialog.
	static bool s_singleFileDlgPref[EFILE_TYPE_LAST];

	static bool CustomSelectSingleFile( ECustomFileType fileType, CString &outputFile, const CString & filter, const CString & initialDir );
};

class CAutoRestoreMasterCDRoot
{
public:
	~CAutoRestoreMasterCDRoot();
};

class CAutoDirectoryRestoreFileDialog : public CAutoRestoreMasterCDRoot, public CFileDialog
{
public:
	explicit CAutoDirectoryRestoreFileDialog(
		BOOL bOpenFileDialog,
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0
		) : CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize) {}
};

#endif // __fileutil_h__
