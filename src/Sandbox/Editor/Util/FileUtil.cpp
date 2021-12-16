////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   fileutil.cpp
//  Version:     v1.00
//  Created:     13/9/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "STLPoolAllocator.h"
#include "FileUtil.h"
#include "IndexedFiles.h"
#include "..\Settings.h"
#include "..\CheckOutDialog.h"
#include "..\SrcSafeSettingsDialog.h"
#include "ISourceControl.h"
#include "..\SoundBrowserDialog.h"
#include "..\DialogBrowserDialog.h"
#include "..\Dialogs/Generic/GenericOverwriteDialog.h"
#include "..\Dialogs/Generic/UserOptions.h"
#include "..\SourceControlDescDlg.h"
#include "..\Clipboard.h"


#include "..\CustomFileDialog.h"
#include "..\SmartFileOpenDialog.h"
#include <CryFile.h>
#include <io.h>
#include <time.h>

bool CFileUtil::s_singleFileDlgPref[EFILE_TYPE_LAST] = { true, true, true, true, true };

CAutoRestoreMasterCDRoot::~CAutoRestoreMasterCDRoot()
{
	SetCurrentDirectoryW(GetIEditor()->GetMasterCDFolder());
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::CompileLuaFile( const char *luaFilename )
{
	CString luaFile = luaFilename;
	// Check if this file is in Archive.
	{
		CCryFile file;
		if (file.Open( luaFilename,"rb" ))
		{
			// Check if in pack.
			if (file.IsInPak())
				return true;
			luaFile = file.GetAdjustedFilename();
		}
	}
	// First try compiling script and see if it have any errors.
	CString LuaCompiler;
	CString CompilerOutput;

	// Create the filepath of the lua compiler
	 char szExeFileName[_MAX_PATH];
	// Get the path of the executable
	GetModuleFileName( GetModuleHandle(NULL), szExeFileName, sizeof(szExeFileName));
	CString exePath = Path::GetPath( szExeFileName );

	LuaCompiler = Path::AddBackslash(exePath) + "LuaCompiler.exe ";

	luaFile.Replace( '/','\\' );
	// Add the name of the Lua file
	CString cmdLine = LuaCompiler + luaFile;

	// Execute the compiler and capture the output
	if (!GetIEditor()->ExecuteConsoleApp( cmdLine,CompilerOutput ))
	{
		AfxMessageBox("Error while executing 'LuaCompiler.exe', make sure the file is in" \
			" your Master CD folder !");
		return false;
	}

	// Check return string
	if (strlen(CompilerOutput) > 0)
	{
		// Errors while compiling file.

		// Show output from Lua compiler
		if (MessageBox( NULL,(CString("Error output from Lua compiler:\r\n") + CompilerOutput +
			CString("\r\nDo you want to edit the file ?")),"Lua Compiler", MB_ICONERROR | MB_YESNO) == IDYES)
		{
			int line = 0;
			CString cmdLine = luaFile;
			int index = CompilerOutput.Find( "at line" );
			if (index >= 0)
			{
				const char *str = CompilerOutput;
				sscanf( &str[index],"at line %d",&line );
			}
			// Open the Lua file for editing
			EditTextFile( luaFile,line );
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::EditTextFile( const char *txtFile,int line,ETextFileType fileType, bool bUseGameFolder )
{
	CString file = txtFile;

	if (bUseGameFolder)
	{
		string sGameFolder=PathUtil::GetGameFolder();
		int nLen=sGameFolder.length();
		if (strnicmp(txtFile,sGameFolder.c_str(),nLen)!=0)
		{
			file=sGameFolder+string("\\")+string(txtFile);
		}
	}

	file.Replace( '/','\\' );

	// Check if this file is in Archive.
	{
		CCryFile cryfile;
		if (cryfile.Open( file,"rb" ))
		{
			// Check if in pack.
			if (cryfile.IsInPak())
			{
				const char *sPakName = cryfile.GetPakPath();
				// Cannot edit file in pack, suggest to extract it for editing.
				CString msg;
				msg.Format( _T("File %s is inside Pak file %s\r\nDo you want it to be extracted for Editing?"),(const char*)file,sPakName );
				if (AfxMessageBox( msg,MB_YESNO|MB_ICONEXCLAMATION) == IDNO)
				{
					return;
				}
				CFileUtil::CreateDirectory( Path::GetPath(file) );
				// Extract it from Pak file.
				CFile diskFile;
				if (diskFile.Open( file,CFile::modeCreate|CFile::modeWrite))
				{
					// Copy data from packed file to disk file.
					char *data = new char[cryfile.GetLength()];
					cryfile.ReadRaw( data,cryfile.GetLength() );
					diskFile.Write( data,cryfile.GetLength() );
					delete []data;
				}
				else
				{
					Warning( "Failed to create file %s on disk",(const char*)file );
				}
			}
			else
			{
				file = cryfile.GetAdjustedFilename();
			}
		}
	}

	file.Replace( '/','\\' );
	CString cmd;
	if (line != 0)
	{
		cmd.Format( "%s/%d/0",(const char*)file,line );
	}
	else
	{
		cmd = file;
	}

	CString TextEditor = gSettings.textEditorForScript;
	if (fileType == FILE_TYPE_SHADER)
	{
		TextEditor = gSettings.textEditorForShaders;
	}

	HINSTANCE hInst = ShellExecute( NULL, "open", TextEditor, cmd,	NULL, SW_SHOWNORMAL );
	if ((DWORD_PTR)hInst <= 32)
	{
		// Failed.
		file = file.SpanExcluding( "/" );
		// Try standart open.
		hInst = ShellExecute( NULL, "open", file, NULL, NULL, SW_SHOWNORMAL );
		if ((DWORD_PTR)hInst <= 32)
		{
			if (AfxMessageBox( "Can't open the file. You can specify a source editor in Sandbox Preferences or create an association in Windows. Do you want to open the file in the Notepad?", MB_YESNO ) == IDYES)
			{
				ShellExecute( NULL, "open", "notepad", cmd,	NULL, SW_SHOWNORMAL );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::SelectFile( const CString &fileSpec,const CString &searchFolder,CString &fullFileName )
{
	CString filter;
	filter.Format( "%s|%s||",(const char*)fileSpec,(const char*)fileSpec );

	CAutoDirectoryRestoreFileDialog dlg(TRUE, NULL,NULL, OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, filter );
	dlg.m_ofn.lpstrInitialDir = searchFolder;

	if (dlg.DoModal() == IDOK)
	{
		fullFileName = dlg.GetPathName();
		return true;
		/*
		if (!fileName.IsEmpty())
		{
			relativeFileName = Path::GetRelativePath( fileName );
			if (!relativeFileName.IsEmpty())
			{
				return true;
			}
			else
			{
				Warning( "You must select files from %s folder",(const char*)GetIEditor()->GetMasterCDFolder(); );
			}
		}
		*/
	}

//	CSelectFileDlg cDialog;
//	bool res = cDialog.SelectFileName( &fileName,&relativeFileName,fileSpec,searchFolder );
	return false;
}

bool CFileUtil::SelectFiles( const CString &fileSpec,const CString &searchFolder,std::vector<CString> &files )
{
	CString filter;
	filter.Format( "%s|%s||",(const char*)fileSpec,(const char*)fileSpec );

	char filesStr[16768];
	memset( filesStr,0,sizeof(filesStr) );

	CAutoDirectoryRestoreFileDialog dlg(TRUE, NULL,NULL, OFN_ALLOWMULTISELECT|OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, filter );
	dlg.m_ofn.lpstrInitialDir = searchFolder;
	dlg.m_ofn.lpstrFile = filesStr;
	dlg.m_ofn.nMaxFile = sizeof(filesStr);

	files.clear();
	if (dlg.DoModal())
	{
		POSITION pos = dlg.GetStartPosition();
		while (pos != NULL)
		{
			CString fileName = dlg.GetNextPathName(pos);
			if (fileName.IsEmpty())
				continue;
			files.push_back( fileName );
		}
	}

	if (!files.empty())
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::SelectSaveFile( const CString &fileFilter,const CString &defaulExtension,const CString &startFolder,CString &fileName )
{
	CAutoDirectoryRestoreFileDialog dlg(FALSE, defaulExtension, fileName, OFN_ENABLESIZING|OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT, fileFilter );
	CString initialPath = Path::MakeFullPath(startFolder);
	dlg.m_ofn.lpstrInitialDir = initialPath;

	if (dlg.DoModal() == IDOK)
	{
		fileName = dlg.GetPathName();
		if (!fileName.IsEmpty())
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::SelectSingleFile( ECustomFileType fileType,CString &outputFile,const CString &filter,const CString &initialDir )
{
	CAutoRestoreMasterCDRoot adr;

	if(fileType==EFILE_TYPE_SOUND)
	{
		CSoundBrowserDialog dlg;
		dlg.Init(outputFile);
		if (dlg.DoModal() == IDOK)
		{
			if(!dlg.IsBrowse())
			{
				outputFile = dlg.GetString();
				if(strlen(outputFile)==0)
					return false;
				return true;
			}
		}
		else
			return false;
	}

	if(fileType==EFILE_TYPE_DIALOG)
	{
		CDialogBrowserDialog dlg;
		dlg.Init(outputFile);
		if (dlg.DoModal() == IDOK)
		{
			if(!dlg.IsBrowse())
			{
				outputFile = dlg.GetString();
				if(strlen(outputFile)==0)
					return false;
				return true;
			}
		}
		else
			return false;
	}

	if(s_singleFileDlgPref[fileType])
		return CustomSelectSingleFile(fileType, outputFile, filter, initialDir);
	else
		return SmartSelectSingleFile(fileType, outputFile, "", initialDir, filter);
}

//! Display OpenFile dialog and allow to select multiple files.
bool CFileUtil::SelectMultipleFiles( ECustomFileType fileType,std::vector<CString> &files,const CString &filter,const CString &initialDir )
{
	CAutoRestoreMasterCDRoot adr;

	CCustomFileDialog::OpenParams op;
	op.bMultiSelection = true;
	op.filetype = fileType;
	op.filter = filter;
	op.initialDir = initialDir;
	CCustomFileDialog dlg( op );

	files.clear();
	if (dlg.DoModal() == IDOK)
	{
		for (int i = 0; i < dlg.GetSelectedCount(); i++)
		{
			files.push_back( dlg.GetSelectedFile(i) );
		}
	}
	return !files.empty();
}

//////////////////////////////////////////////////////////////////////////
// Get directory contents.
//////////////////////////////////////////////////////////////////////////
inline bool ScanDirectoryFiles( const CString &root,const CString &path,const CString &fileSpec,std::vector<CFileUtil::FileDesc> &files )
{
	bool anyFound = false;
	CString dir = Path::AddBackslash(root + path);

	CString findFilter = Path::Make(dir,fileSpec);
	ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();
	
	// Add all directories.
	_finddata_t fd;
	intptr_t fhandle;

	fhandle = pIPak->FindFirst( findFilter,&fd );
	if (fhandle != -1)
	{
		do {
			// Skip back folders.
			if (fd.name[0] == '.')
				continue;
			if (fd.attrib & _A_SUBDIR) // skip sub directories.
				continue;

			anyFound = true;

			CFileUtil::FileDesc file;
			file.filename = path + fd.name;
			file.attrib = fd.attrib;
			file.size = fd.size;
			file.time_access = fd.time_access;
			file.time_create = fd.time_create;
			file.time_write = fd.time_write;

			files.push_back( file );
		} while (pIPak->FindNext( fhandle,&fd ) == 0);
		pIPak->FindClose(fhandle);
	}

	/*
	CFileFind finder;
	BOOL bWorking = finder.FindFile( Path::Make(dir,fileSpec) );
	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		if (finder.IsDots())
			continue;

		if (!finder.IsDirectory())
		{
			anyFound = true;

			CFileUtil::FileDesc fd;
			fd.filename = dir + finder.GetFileName();
			fd.nFileSize = finder.GetLength();

			finder.GetCreationTime( &fd.ftCreationTime );
			finder.GetLastAccessTime( &fd.ftLastAccessTime );
			finder.GetLastWriteTime( &fd.ftLastWriteTime );

			fd.dwFileAttributes = 0;
			if (finder.IsArchived())
				fd.dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
			if (finder.IsCompressed())
				fd.dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED;
			if (finder.IsNormal())
				fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			if (finder.IsHidden())
				fd.dwFileAttributes = FILE_ATTRIBUTE_HIDDEN;
			if (finder.IsReadOnly())
				fd.dwFileAttributes = FILE_ATTRIBUTE_READONLY;
			if (finder.IsSystem())
				fd.dwFileAttributes = FILE_ATTRIBUTE_SYSTEM;
			if (finder.IsTemporary())
				fd.dwFileAttributes = FILE_ATTRIBUTE_TEMPORARY;

			files.push_back(fd);
		}
	}
	*/

	return anyFound;
}

//////////////////////////////////////////////////////////////////////////
// Get directory contents.
//////////////////////////////////////////////////////////////////////////
inline int ScanDirectoryRecursive( const CString &root,const CString &path,const CString &fileSpec,std::vector<CFileUtil::FileDesc> &files, bool recursive, bool addDirAlso, CFileUtil::ScanDirectoryUpdateCallBack updateCB )
{
	bool anyFound = false;
	CString dir = Path::AddBackslash(root + path);

	if(updateCB)
	{
		CString msg;
		msg.Format("Scanning %s...", dir);
		if(updateCB(msg) == false)
			return -1;
	}

	if (ScanDirectoryFiles( root,Path::AddBackslash(path),fileSpec,files ))
		anyFound = true;

	if (recursive)
	{
		/*
		CFileFind finder;
		BOOL bWorking = finder.FindFile( Path::Make(dir,"*.*") );
		while (bWorking)
		{
			bWorking = finder.FindNextFile();

			if (finder.IsDots())
				continue;

			if (finder.IsDirectory())
			{
				// Scan directory.
				if (ScanDirectoryRecursive( root,Path::AddBackslash(path+finder.GetFileName()),fileSpec,files,recursive ))
					anyFound = true;
			}
		}
		*/

		ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();

		// Add all directories.
		_finddata_t fd;
		intptr_t fhandle;

		fhandle = pIPak->FindFirst( Path::Make(dir,"*.*"),&fd );
		if (fhandle != -1)
		{
			do {
				// Skip back folders.
				if (fd.name[0] == '.')
					continue;
				if (!(fd.attrib & _A_SUBDIR)) // skip not directories.
					continue;

				if(addDirAlso)
				{
					CFileUtil::FileDesc Dir;
					Dir.filename = path + fd.name;
					Dir.attrib = fd.attrib;
					Dir.size = fd.size;
					Dir.time_access = fd.time_access;
					Dir.time_create = fd.time_create;
					Dir.time_write = fd.time_write;
					files.push_back(Dir);
				}

				// Scan directory.
				int result = ScanDirectoryRecursive( root,Path::AddBackslash(path + fd.name),fileSpec,files,recursive,addDirAlso,updateCB );
				if(result == -1)
				// Cancel the scan immediately.
				{
					pIPak->FindClose(fhandle);
					return -1;
				}
				else if(result == 1)
					anyFound = true;

			} while (pIPak->FindNext( fhandle,&fd ) == 0);
			pIPak->FindClose(fhandle);
		}
	}

	return anyFound?1:0;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::ScanDirectory( const CString &path,const CString &file,std::vector<FileDesc> &files, bool recursive, bool addDirAlso, CFileUtil::ScanDirectoryUpdateCallBack updateCB )
{
	CString fileSpec = Path::GetFile(file);
	CString localPath = Path::GetPath(file);
	return ScanDirectoryRecursive(Path::AddBackslash(path),localPath,fileSpec,files,recursive,addDirAlso,updateCB ) > 0;
}

/*
bool CFileUtil::ScanDirectory( const CString &startDirectory,const CString &searchPath,const CString &fileSpec,FileArray &files, bool recursive=true )
{
	return ScanDirectoryRecursive(startDirectory,SearchPath,file,files,recursive );
}
*/

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::OverwriteFile( const char *filename )
{
	// check if file exist.
	FILE *file = fopen( filename,"rb" );
	if (!file)
		return true;
	fclose(file);
	
	int res = (GetFileAttributes(filename)&FILE_ATTRIBUTE_READONLY);
	if (res == INVALID_FILE_ATTRIBUTES)
	{
		Warning( "Cannot Save File %s",filename );
		return false;
	}
	if (res != 0)
	{
		CCheckOutDialog dlg( filename,AfxGetMainWnd() );
		if (dlg.DoModal() == IDCANCEL)
		{
			return false;
		}
		if (dlg.GetResult() == CCheckOutDialog::CHECKOUT)
		{
			return CheckoutFile( filename );
		}
		SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );
	}
	return true;
}

/*
static bool CheckOutFile( const char *filename )
{
	CString ssafeExe = "C:\\Program Files\\Microsoft Visual Studio\\VSS\\win32\\ss.exe";
	SetEnvironmentVariable( "ssuser","timur" );
	SetEnvironmentVariable( "ssdir","\\\\Server2\\XISLE\\ArtworkVss" );
	//CString SSafeArtwork = "\\\\Server2\\XISLE\\ArtworkVss\\win32\\ss.exe";
	//CString SSafeArtworkProject = "$/MASTERCD";

	CString cmd = ssafeExe + " " + " checkout cg.dll";

	char currDirectory[MAX_PATH];
	GetCurrentDirectory( sizeof(currDirectory),currDirectory  );
	char cmdLine[MAX_PATH];
	strcpy( cmdLine,cmd );

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset( &si,0,sizeof(si) );
	si.cb = sizeof(si);
	memset( &pi,0,sizeof(pi) );
	if (CreateProcess( NULL,cmdLine,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,currDirectory,&si,&pi ))
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
}
*/

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::CheckoutFile( const char *filename )
{
	/*
	if (gSettings.ssafeParams.user.IsEmpty())
	{
		AfxMessageBox( _T("Source Safe login user name must be configured."),MB_OK|MB_ICONEXCLAMATION );

		// Source safe not configured.
		CSrcSafeSettingsDialog dlg;
		if (dlg.DoModal() != IDOK)
		{
			AfxMessageBox( _T("Checkout canceled"),MB_OK|MB_ICONEXCLAMATION );
			return false;
		}
	}
	SetEnvironmentVariable( "ssuser",gSettings.ssafeParams.user );
	SetEnvironmentVariable( "ssdir",gSettings.ssafeParams.databasePath );

	CString relFile = Path::GetRelativePath(filename);
	if (relFile.IsEmpty())
		relFile = filename;

	CString cmd = gSettings.ssafeParams.exeFile + " checkout " + relFile;

	char currDirectory[MAX_PATH];
	GetCurrentDirectory( sizeof(currDirectory),currDirectory  );
	char cmdLine[MAX_PATH];
	strcpy( cmdLine,cmd );

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset( &si,0,sizeof(si) );
	si.cb = sizeof(si);
	memset( &pi,0,sizeof(pi) );
	if (CreateProcess( NULL,cmdLine,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,currDirectory,&si,&pi ))
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return true;
	}
	*/

	uint32 attr = GetIEditor()->GetSourceControl()->GetFileAttributes( filename );
	if (attr & SCC_FILE_ATTRIBUTE_MANAGED)
	{
		if (attr & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
		{
			return true;
		}
		else
		{
			return GetIEditor()->GetSourceControl()->CheckOut( filename );
		}
	}
	else
	{
		// Files from pak should be easily openable.
		if (attr & SCC_FILE_ATTRIBUTE_INPAK)
			return true;

		if (attr & SCC_FILE_ATTRIBUTE_READONLY)
		{
			CString str;
			str.Format( "File %s is Read-Only, Overrite?", (const char*)filename );
			if (AfxMessageBox( str,MB_YESNO ) == IDYES)
			{
				SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );
				return false;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::CheckinFile( const char *filename )
{
	/*
	SetEnvironmentVariable( "ssuser",gSettings.ssafeParams.user );
	SetEnvironmentVariable( "ssdir",gSettings.ssafeParams.databasePath );

	CString relFile = Path::GetRelativePath(filename);
	if (relFile.IsEmpty())
		relFile = filename;

	CString cmd = gSettings.ssafeParams.exeFile + " checkout " + relFile;

	char currDirectory[MAX_PATH];
	GetCurrentDirectory( sizeof(currDirectory),currDirectory  );
	char cmdLine[MAX_PATH];
	strcpy( cmdLine,cmd );

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset( &si,0,sizeof(si) );
	si.cb = sizeof(si);
	memset( &pi,0,sizeof(pi) );
	if (CreateProcess( NULL,cmdLine,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,currDirectory,&si,&pi ))
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return true;
	}
	*/

	uint32 attr = GetIEditor()->GetSourceControl()->GetFileAttributes( filename );
	if (attr & SCC_FILE_ATTRIBUTE_MANAGED)
	{
		if (attr & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
		{
			return GetIEditor()->GetSourceControl()->CheckIn( filename );
		}
	}

	return true;
}

// Create new directory, check if directory already exist.
static bool CheckAndCreateDirectory( const char *path )
{
	WIN32_FIND_DATA FindFileData;

	HANDLE hFind = FindFirstFile( path,&FindFileData );
	if (hFind == INVALID_HANDLE_VALUE) {
		return ::CreateDirectory( path,NULL ) == TRUE;
	} else {
		DWORD attr = FindFileData.dwFileAttributes;
		FindClose(hFind);
		if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::CreateDirectory( const char *directory )
{
	CString path = directory;
	CString dir;
	bool res = CheckAndCreateDirectory( path );
	if (!res)
	{
		int iStart = 0;
		CString token = TokenizeString(path,"\\/",iStart );
		dir = token;
		while (token != "")
		{
			CheckAndCreateDirectory( dir );
			token = TokenizeString(path,"\\/",iStart );
			dir += CString("\\") + token;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::BackupFile( const char *filename )
{
	// Make a backup of previous file.
	bool makeBackup = true;

	CString bakFilename = Path::ReplaceExtension( filename,"bak" );

	{
		// Check if backup needed.
		CFile bak;
		if (bak.Open( filename,CFile::modeRead ))
		{
			if (bak.GetLength() <= 0)
				makeBackup = false;
		}
		else
			makeBackup = false;
	}
	// Backup the backup..
	if (makeBackup)
	{
		MoveFileEx( bakFilename,Path::ReplaceExtension( bakFilename,"bak2" ),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH );
		SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );
		MoveFileEx( filename,bakFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH );
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::BackupFileDated( const char *filename, bool bUseBackupSubDirectory /*=false*/ )
{
	bool makeBackup = true;
	{
		// Check if backup needed.
		CFile bak;
		if (bak.Open( filename,CFile::modeRead ))
		{
			if (bak.GetLength() <= 0)
				makeBackup = false;
		}
		else
			makeBackup = false;
	}

	if ( makeBackup )
	{
		// Generate new filename
		time_t ltime;
		time( &ltime );
		tm *today = localtime( &ltime );

		char sTemp[128];
		strftime( sTemp, sizeof(sTemp), ".%Y%m%d.%H%M%S.", today);		
		CString bakFilename = Path::RemoveExtension(filename) + sTemp + Path::GetExt(filename);

		if ( bUseBackupSubDirectory )
		{
			CString sBackupPath = Path::ToUnixPath( Path::GetPath( filename ) ) + CString( "/backups" );
			CFileUtil::CreateDirectory( sBackupPath );
			bakFilename = sBackupPath + CString("/") + Path::GetFile( bakFilename );
		}

		// Do the backup
		SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );
		MoveFileEx( filename,bakFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH );	
	}
}
//////////////////////////////////////////////////////////////////////////
bool CFileUtil::Deltree(const char *szFolder,bool bRecurse)
{
	bool							boRemoveResult(false);

	__finddata64_t		fd;
	string						filespec = szFolder;

	filespec += "*.*";

	intptr_t hfil = 0;
	if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
	{
		return false;
	}

	do
	{
		if (fd.attrib & _A_SUBDIR)
		{
			CString name = fd.name;

			if ((name != ".") && (name != ".."))
			{
				if (bRecurse)
				{
					name = szFolder;
					name += fd.name;
					name += "/";

					Deltree(name.GetBuffer(), bRecurse);
				}
			}
		}
		else
		{
			CString name = szFolder;

			name += fd.name;

			CFileUtil::DeleteFile(name);
		}

	} while(!_findnext64(hfil, &fd));

	_findclose(hfil);

	boRemoveResult=CFileUtil::RemoveDirectory(szFolder);

	return boRemoveResult;
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::Exists(const CString& strPath,bool boDirectory)
{
	ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();
	intptr_t				nFindHandle(NULL);
	_finddata_t			stFoundData;
	bool						boIsDirectory(false);

	memset(&stFoundData,0,sizeof(_finddata_t));
	nFindHandle=pIPak->FindFirst(strPath,&stFoundData);
	// If it found nothing, no matter if it is a file or directory, it was not found.
	if (nFindHandle==-1)
	{
		return false;
	}
	pIPak->FindClose(nFindHandle);

	if (stFoundData.attrib&_A_SUBDIR)
	{
		boIsDirectory=true;
	}

	// If we are seeking directories...
	if (boDirectory)
	{
		// The return value will tell us if the found element is a directory.
		return boIsDirectory;
	}
	else
	{
		// If we are not seeking directories...
		// We return true if the found element is not a directory.
		return !boIsDirectory;
	}
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::FileExists(const CString& strFilePath)
{
	return Exists(strFilePath,false);
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::PathExists(const CString& strPath)
{
	return Exists(strPath,true);
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::IsFileExclusivelyAccessable(const CString& strFilePath)
{
	HANDLE hTesteFileHandle(NULL);
		
	// We should use instead CreateFileTransacted, but this would mean
	// requiring Vista as the Minimum OS to run.
	hTesteFileHandle=CreateFile(
		strFilePath,	// The filename
		0,	//	We don't really want to access the file...
		0,	//	This is the hot spot:  we don't want to share this file!
		NULL,   // We don't care about the security attributes.
		OPEN_EXISTING,	// The file must exist, or this will be of no use.
		FILE_ATTRIBUTE_NORMAL, // We don't care about attributes.
		NULL // We don't care about template files.
		);

	// We couldn't actually open the file in exclusive mode for whatever
	// reason: for example some other app is using it or the file doesn't
	// exist.
	if (hTesteFileHandle==INVALID_HANDLE_VALUE)
	{
		return false;
	}

	CloseHandle(hTesteFileHandle);

	return true;
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::CreatePath(const CString& strPath)
{
	CString									strDriveLetter;
	CString									strDirectory;
	CString									strFilename;
	CString									strExtension;
	CString									strCurrentDirectoryPath;
	std::vector<CString>		cstrDirectoryQueue;
	size_t									nCurrentPathQueue(0);
	size_t									nTotalPathQueueElements(0);
	BOOL										bnLastDirectoryWasCreated(FALSE);

	if (PathExists(strPath))
	{
		return true;
	}

	Path::SplitPath(strPath,strDriveLetter,strDirectory,strFilename,strExtension);
	Path::GetDirectoryQueue(strDirectory,cstrDirectoryQueue);

	if (strDriveLetter.GetLength()>0)
	{
		strCurrentDirectoryPath=strDriveLetter;
		strCurrentDirectoryPath+="\\";
	}


	nTotalPathQueueElements=cstrDirectoryQueue.size();
	for (nCurrentPathQueue=0;nCurrentPathQueue<nTotalPathQueueElements;++nCurrentPathQueue)
	{
		strCurrentDirectoryPath+=cstrDirectoryQueue[nCurrentPathQueue];
		strCurrentDirectoryPath+="\\";
		// The value which will go out of this loop is the result of the attempt to create the
		// last directory, only.
		bnLastDirectoryWasCreated=::CreateDirectory(strCurrentDirectoryPath,NULL);
	}

	if (!bnLastDirectoryWasCreated)
	{
		if (ERROR_ALREADY_EXISTS!=GetLastError())
		{
			return false;
		}
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::DeleteFile(const CString& strPath)
{
	BOOL bnAttributeChange(FALSE);
	BOOL bnFileDeletion(FALSE);

	bnAttributeChange=SetFileAttributes(strPath,FILE_ATTRIBUTE_NORMAL);
	bnFileDeletion=::DeleteFile(strPath);

	return (bnFileDeletion!=FALSE);
}
//////////////////////////////////////////////////////////////////////////
bool	CFileUtil::RemoveDirectory(const CString& strPath)
{
	BOOL bnRemoveResult(FALSE);
	BOOL bnAttributeChange(FALSE);

	bnAttributeChange=SetFileAttributes(strPath,FILE_ATTRIBUTE_NORMAL);
	bnRemoveResult=::RemoveDirectory(strPath);
	
	return (bnRemoveResult!=FALSE);
}
//////////////////////////////////////////////////////////////////////////
CFileUtil::ECopyTreeResult CFileUtil::CopyTree(const CString& strSourceDirectory,const CString& strTargetDirectory,bool boRecurse,bool boConfirmOverwrite)
{
	static CUserOptions				oFileOptions;
	static CUserOptions				oDirectoryOptions;

	CUserOptions::CUserOptionsReferenceCountHelper	oFileOptionsHelper(oFileOptions);
	CUserOptions::CUserOptionsReferenceCountHelper	oDirectoryOptionsHelper(oDirectoryOptions);

	ECopyTreeResult						eCopyResult(ETREECOPYOK);

	__finddata64_t						fd;
	string										filespec(strSourceDirectory);

	intptr_t									hfil(0);

	std::vector<CString>			cFiles;
	std::vector<CString>			cDirectories;

	size_t										nCurrent(0);
	size_t										nTotal(0);

	// For this function to work properly, it has to first process all files in the directory AND JUST AFTER IT
	// work on the sub-folders...this is NOT OBVIOUS, but imagine the case where you have a hierarchy of folders,
	// all with the same names and all with the same files names inside. If you would make a depth-first search
	// you could end up with the files from the deepest folder in ALL your folders.

	filespec += "*.*";

	if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
	{
		return ETREECOPYOK;
	}

	do
	{
		if (fd.attrib & _A_SUBDIR)
		{
			CString name(fd.name);

			if ((name != ".") && (name != ".."))
			{
				if (boRecurse)
				{
					cDirectories.push_back(name);
				}
			}
		}
		else
		{
			cFiles.push_back(fd.name);
		}
	} while(!_findnext64(hfil, &fd));

	_findclose(hfil);

	// First we copy all files (maybe not all, depending on the user options...)
	nTotal=cFiles.size();
	for (nCurrent=0;nCurrent<nTotal;++nCurrent)
	{
		BOOL		bnLastFileWasCopied(FALSE);
		CString	name(strSourceDirectory);
		CString	strTargetName(strTargetDirectory);

		if (eCopyResult==ETREECOPYUSERCANCELED)
		{
			return eCopyResult;
		}

		name+=cFiles[nCurrent];
		strTargetName+=cFiles[nCurrent];

		if (boConfirmOverwrite)
		{
			if (PathFileExists(strTargetName))
			{

				// If the directory already exists...
				// we must warn our user about the possible actions.
				int				nUserOption(0);

				if (boConfirmOverwrite)
				{
					// If the option is not valid to all folder, we must ask anyway again the user option.
					if (!oFileOptions.IsOptionToAll())
					{
						CString		strRewriteText;

						strRewriteText.Format("There is already a file named \"%s\" in the target folder. Do you want to move this file anyway replacing the old one?",fd.name);
						CGenericOverwriteDialog	oOverwriteDialog("Confirm file overwrite?",strRewriteText);
						nUserOption=oOverwriteDialog.DoModal();
						oFileOptions.SetOption(nUserOption,oOverwriteDialog.IsToAllToggled());
					}
					else
					{
						nUserOption=oFileOptions.GetOption();
					}
				}

				switch (nUserOption)
				{
					case IDYES:
					{
						// Actually, we need to do nothing in this case.
					}
					break;

					case IDNO:
					{
						eCopyResult=ETREECOPYUSERDIDNTCOPYSOMEITEMS;
						continue;
					}
					break;

					// This IS ALWAYS for all... so it's easy to deal with.
					case IDCANCEL:
					{
						return ETREECOPYUSERCANCELED;
					}
					break;
				}
			}
		}

		bnLastFileWasCopied=::CopyFile(name,strTargetName,FALSE);
		if (!bnLastFileWasCopied)
		{
			eCopyResult=ETREECOPYFAIL;
		}
	}

	// Now we can recurse into the directories, if needed.
	nTotal=cDirectories.size();
	for (nCurrent=0;nCurrent<nTotal;++nCurrent)
	{
		CString name(strSourceDirectory);
		CString	strTargetName(strTargetDirectory);
		BOOL		bnLastDirectoryWasCreated(FALSE);

		if (eCopyResult==ETREECOPYUSERCANCELED)
		{
			return eCopyResult;
		}

		name += cDirectories[nCurrent];
		name += "/";

		strTargetName+=cDirectories[nCurrent];
		strTargetName+="/";

		bnLastDirectoryWasCreated=::CreateDirectory(strTargetName,NULL);
		if (!bnLastDirectoryWasCreated)
		{
			if (ERROR_ALREADY_EXISTS!=GetLastError())
			{
				return ETREECOPYFAIL;
			}
			else 
			{		
				// If the directory already exists...
				// we must warn our user about the possible actions.
				int				nUserOption(0);

				if (boConfirmOverwrite)
				{
					// If the option is not valid to all folder, we must ask anyway again the user option.
					if (!oDirectoryOptions.IsOptionToAll())
					{
						CString		strRewriteText;

						strRewriteText.Format("There is already a folder named \"%s\" in the target folder. Do you want to move this folder anyway?",fd.name);
						CGenericOverwriteDialog	oOverwriteDialog("Confirm directory overwrite?",strRewriteText);
						nUserOption=oOverwriteDialog.DoModal();
						oDirectoryOptions.SetOption(nUserOption,oOverwriteDialog.IsToAllToggled());
					}
					else
					{
						nUserOption=oDirectoryOptions.GetOption();
					}
				}

				switch (nUserOption)
				{
					case IDYES:
					{
						// Actually, we need to do nothing in this case.
					}
					break;

					case IDNO:
					{
						// If no, we just need to go to the next item.
						eCopyResult=ETREECOPYUSERDIDNTCOPYSOMEITEMS;
						continue;
					}
					break;

					// This IS ALWAYS for all... so it's easy to deal with.
					case IDCANCEL:
					{
						return ETREECOPYUSERCANCELED;
					}
					break;
				}
			}
		}

		eCopyResult=CopyTree(name,strTargetName,boRecurse,boConfirmOverwrite);
	}
	
	return eCopyResult;
}
//////////////////////////////////////////////////////////////////////////
CFileUtil::ECopyTreeResult   CFileUtil::CopyFile(const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite)
{
	CUserOptions							oFileOptions;

	ECopyTreeResult						eCopyResult(ETREECOPYOK);

	intptr_t									hfil(0);

	std::vector<CString>			cFiles;
	std::vector<CString>			cDirectories;

	size_t										nCurrent(0);
	size_t										nTotal(0);

	// First we copy all files (maybe not all, depending on the user options...)
	BOOL											bnLastFileWasCopied(FALSE);
	CString										name(strSourceFile);
	CString										strTargetName(strTargetFile);
	CString										strQueryFilename;
	CString										strFullStargetName;

	CString										strDriveLetter;
	CString										strDirectory;
	CString										strFilename;
	CString										strExtension;

	Path::SplitPath(strTargetFile,strDriveLetter,strDirectory,strFilename,strExtension);
	strFullStargetName=strDriveLetter;
	strFullStargetName+=strDirectory;

	if (strFilename.GetLength()==0)
	{
		strFullStargetName+=Path::GetFileName(strSourceFile);
		strFullStargetName+=".";
		strFullStargetName+=Path::GetExt(strSourceFile);
	}
	else
	{
		strFullStargetName+=strFilename;
		strFullStargetName+=strExtension;
	}


	if (boConfirmOverwrite)
	{
		if (PathFileExists(strFullStargetName))
		{
			strQueryFilename=strFilename;
			if (strFilename.GetLength()==0)
			{
				strQueryFilename=Path::GetFileName(strSourceFile);
				strQueryFilename+=".";
				strQueryFilename+=Path::GetExt(strSourceFile);
			}
			else
			{
				strQueryFilename+=strExtension;
			}

			// If the directory already exists...
			// we must warn our user about the possible actions.
			int				nUserOption(0);

			if (boConfirmOverwrite)
			{
				// If the option is not valid to all folder, we must ask anyway again the user option.
				if (!oFileOptions.IsOptionToAll())
				{
					CString		strRewriteText;

					strRewriteText.Format("There is already a file named \"%s\" in the target folder. Do you want to move this file anyway replacing the old one?",strQueryFilename.GetBuffer());
					CGenericOverwriteDialog	oOverwriteDialog("Confirm file overwrite?",strRewriteText,false);
					nUserOption=oOverwriteDialog.DoModal();
					oFileOptions.SetOption(nUserOption,oOverwriteDialog.IsToAllToggled());
				}
				else
				{
					nUserOption=oFileOptions.GetOption();
				}
			}

			switch (nUserOption)
			{
			case IDYES:
				{
					// Actually, we need to do nothing in this case.
				}
				break;

			case IDNO:
				{
					return eCopyResult=ETREECOPYUSERCANCELED;
				}
				break;

				// This IS ALWAYS for all... so it's easy to deal with.
			case IDCANCEL:
				{
					return ETREECOPYUSERCANCELED;
				}
				break;
			}
		}
	}

	bnLastFileWasCopied=::CopyFile(name,strFullStargetName,FALSE);
	if (!bnLastFileWasCopied)
	{
		eCopyResult=ETREECOPYFAIL;
	}

	return eCopyResult;
}
//////////////////////////////////////////////////////////////////////////
CFileUtil::ECopyTreeResult   CFileUtil::MoveTree(const CString& strSourceDirectory,const CString& strTargetDirectory,bool boRecurse,bool boConfirmOverwrite)
{
	static CUserOptions				oFileOptions;
	static CUserOptions				oDirectoryOptions;

	CUserOptions::CUserOptionsReferenceCountHelper	oFileOptionsHelper(oFileOptions);
	CUserOptions::CUserOptionsReferenceCountHelper	oDirectoryOptionsHelper(oDirectoryOptions);

	ECopyTreeResult						eCopyResult(ETREECOPYOK);

	__finddata64_t						fd;
	string										filespec(strSourceDirectory);

	intptr_t									hfil(0);

	std::vector<CString>			cFiles;
	std::vector<CString>			cDirectories;

	size_t										nCurrent(0);
	size_t										nTotal(0);

	// For this function to work properly, it has to first process all files in the directory AND JUST AFTER IT
	// work on the sub-folders...this is NOT OBVIOUS, but imagine the case where you have a hierarchy of folders,
	// all with the same names and all with the same files names inside. If you would make a depth-first search
	// you could end up with the files from the deepest folder in ALL your folders.

	filespec += "*.*";

	if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
	{
		return ETREECOPYOK;
	}

	do
	{
		if (fd.attrib & _A_SUBDIR)
		{
			CString name(fd.name);

			if ((name != ".") && (name != ".."))
			{
				if (boRecurse)
				{
					cDirectories.push_back(name);
				}
			}
		}
		else
		{
			cFiles.push_back(fd.name);
		}
	} while(!_findnext64(hfil, &fd));

	_findclose(hfil);

	// First we copy all files (maybe not all, depending on the user options...)
	nTotal=cFiles.size();
	for (nCurrent=0;nCurrent<nTotal;++nCurrent)
	{
		BOOL		bnLastFileWasCopied(FALSE);
		CString	name(strSourceDirectory);
		CString	strTargetName(strTargetDirectory);

		if (eCopyResult==ETREECOPYUSERCANCELED)
		{
			return eCopyResult;
		}

		name+=cFiles[nCurrent];
		strTargetName+=cFiles[nCurrent];

		if (boConfirmOverwrite)
		{
			if (PathFileExists(strTargetName))
			{

				// If the directory already exists...
				// we must warn our user about the possible actions.
				int				nUserOption(0);

				if (boConfirmOverwrite)
				{
					// If the option is not valid to all folder, we must ask anyway again the user option.
					if (!oFileOptions.IsOptionToAll())
					{
						CString		strRewriteText;

						strRewriteText.Format("There is already a file named \"%s\" in the target folder. Do you want to move this file anyway replacing the old one?",fd.name);
						CGenericOverwriteDialog	oOverwriteDialog("Confirm file overwrite?",strRewriteText);
						nUserOption=oOverwriteDialog.DoModal();
						oFileOptions.SetOption(nUserOption,oOverwriteDialog.IsToAllToggled());
					}
					else
					{
						nUserOption=oFileOptions.GetOption();
					}
				}

				switch (nUserOption)
				{
				case IDYES:
					{
						// Actually, we need to do nothing in this case.
					}
					break;

				case IDNO:
					{
						eCopyResult=ETREECOPYUSERDIDNTCOPYSOMEITEMS;
						continue;
					}
					break;

					// This IS ALWAYS for all... so it's easy to deal with.
				case IDCANCEL:
					{
						return ETREECOPYUSERCANCELED;
					}
					break;
				}
			}
		}

		bnLastFileWasCopied=::MoveFileEx(name,strTargetName,MOVEFILE_REPLACE_EXISTING);
		if (!bnLastFileWasCopied)
		{
			eCopyResult=ETREECOPYFAIL;
		}
	}

	// Now we can recurse into the directories, if needed.
	nTotal=cDirectories.size();
	for (nCurrent=0;nCurrent<nTotal;++nCurrent)
	{
		CString name(strSourceDirectory);
		CString	strTargetName(strTargetDirectory);
		BOOL		bnLastDirectoryWasCreated(FALSE);

		if (eCopyResult==ETREECOPYUSERCANCELED)
		{
			return eCopyResult;
		}

		name += cDirectories[nCurrent];
		name += "/";

		strTargetName+=cDirectories[nCurrent];
		strTargetName+="/";

		bnLastDirectoryWasCreated=::CreateDirectory(strTargetName,NULL);
		if (!bnLastDirectoryWasCreated)
		{
			if (ERROR_ALREADY_EXISTS!=GetLastError())
			{
				return ETREECOPYFAIL;
			}
			else 
			{		
				// If the directory already exists...
				// we must warn our user about the possible actions.
				int				nUserOption(0);

				if (boConfirmOverwrite)
				{
					// If the option is not valid to all folder, we must ask anyway again the user option.
					if (!oDirectoryOptions.IsOptionToAll())
					{
						CString		strRewriteText;

						strRewriteText.Format("There is already a folder named \"%s\" in the target folder. Do you want to move this folder anyway?",fd.name);
						CGenericOverwriteDialog	oOverwriteDialog("Confirm directory overwrite?",strRewriteText);
						nUserOption=oOverwriteDialog.DoModal();
						oDirectoryOptions.SetOption(nUserOption,oOverwriteDialog.IsToAllToggled());
					}
					else
					{
						nUserOption=oDirectoryOptions.GetOption();
					}
				}

				switch (nUserOption)
				{
				case IDYES:
					{
						// Actually, we need to do nothing in this case.
					}
					break;

				case IDNO:
					{
						// If no, we just need to go to the next item.
						eCopyResult=ETREECOPYUSERDIDNTCOPYSOMEITEMS;
						continue;
					}
					break;

					// This IS ALWAYS for all... so it's easy to deal with.
				case IDCANCEL:
					{
						return ETREECOPYUSERCANCELED;
					}
					break;
				}
			}
		}

		eCopyResult=MoveTree(name,strTargetName,boRecurse,boConfirmOverwrite);
	}

	CFileUtil::RemoveDirectory(strSourceDirectory);

	return eCopyResult;
}
//////////////////////////////////////////////////////////////////////////
CFileUtil::ECopyTreeResult   CFileUtil::MoveFile(const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite)
{
	CUserOptions							oFileOptions;

	ECopyTreeResult						eCopyResult(ETREECOPYOK);

	intptr_t									hfil(0);

	std::vector<CString>			cFiles;
	std::vector<CString>			cDirectories;

	size_t										nCurrent(0);
	size_t										nTotal(0);

	// First we copy all files (maybe not all, depending on the user options...)
	BOOL											bnLastFileWasCopied(FALSE);
	CString										name(strSourceFile);
	CString										strTargetName(strTargetFile);
	CString										strQueryFilename;
	CString										strFullStargetName;

	CString										strDriveLetter;
	CString										strDirectory;
	CString										strFilename;
	CString										strExtension;

	Path::SplitPath(strTargetFile,strDriveLetter,strDirectory,strFilename,strExtension);
	strFullStargetName=strDriveLetter;
	strFullStargetName+=strDirectory;

	if (strFilename.GetLength()==0)
	{
		strFullStargetName+=Path::GetFileName(strSourceFile);
		strFullStargetName+=".";
		strFullStargetName+=Path::GetExt(strSourceFile);
	}
	else
	{
		strFullStargetName+=strFilename;
		strFullStargetName+=strExtension;
	}


	if (boConfirmOverwrite)
	{
		if (PathFileExists(strFullStargetName))
		{
			strQueryFilename=strFilename;
			if (strFilename.GetLength()==0)
			{
				strQueryFilename=Path::GetFileName(strSourceFile);
				strQueryFilename+=".";
				strQueryFilename+=Path::GetExt(strSourceFile);
			}
			else
			{
				strQueryFilename+=strExtension;
			}

			// If the directory already exists...
			// we must warn our user about the possible actions.
			int				nUserOption(0);

			if (boConfirmOverwrite)
			{
				// If the option is not valid to all folder, we must ask anyway again the user option.
				if (!oFileOptions.IsOptionToAll())
				{
					CString		strRewriteText;

					strRewriteText.Format("There is already a file named \"%s\" in the target folder. Do you want to move this file anyway replacing the old one?",strQueryFilename.GetBuffer());
					CGenericOverwriteDialog	oOverwriteDialog("Confirm file overwrite?",strRewriteText,false);
					nUserOption=oOverwriteDialog.DoModal();
					oFileOptions.SetOption(nUserOption,oOverwriteDialog.IsToAllToggled());
				}
				else
				{
					nUserOption=oFileOptions.GetOption();
				}
			}

			switch (nUserOption)
			{
				case IDYES:
				{
					// Actually, we need to do nothing in this case.
				}
				break;

				case IDNO:
				{
					return eCopyResult=ETREECOPYUSERCANCELED;
				}
				break;

				// This IS ALWAYS for all... so it's easy to deal with.
				case IDCANCEL:
				{
					return ETREECOPYUSERCANCELED;
				}
				break;
			}
		}
	}

	bnLastFileWasCopied=::MoveFileEx(name,strFullStargetName,MOVEFILE_REPLACE_EXISTING);
	if (!bnLastFileWasCopied)
	{
		eCopyResult=ETREECOPYFAIL;
	}

	return eCopyResult;
}
//////////////////////////////////////////////////////////////////////////
bool CFileUtil::SmartSelectSingleFile(ECustomFileType fileType, CString &outputFile, 
																			const CString& initialSearchTerm, const CString& initialDir, 
																			const CString& filter)
{
	CString tags;
	if(initialDir.GetLength() > 0)
	{
		tags = PathUtil::MakeGamePath(initialDir.GetString());
		tags.Replace('/', ' ');
		tags.Replace('\\', ' ');
		tags =  initialSearchTerm + " " + tags;
	}
	else
		tags = initialSearchTerm;
	CSmartFileOpenDialog dlg(tags);

	INT_PTR result;
	
	if (CIndexedFiles::HasFileIndexingDone() == false)
		result = IDRETRY;
	else
		result = dlg.DoModal();

	if (result == IDOK)
	{
		outputFile = dlg.GetFilePath();
		return true;
	}
	else if (result == IDRETRY)
	{
		s_singleFileDlgPref[fileType] = true;
		return CustomSelectSingleFile(fileType, outputFile, filter, initialDir);
	}
	return false;
}

bool CFileUtil::CustomSelectSingleFile( ECustomFileType fileType, CString &outputFile, const CString & filter, const CString & initialDir )
{
	CCustomFileDialog::OpenParams op;
	op.bMultiSelection = false;
	op.filetype = fileType;
	op.filter = filter;
	op.initialDir = initialDir;
	op.initialFile = outputFile;
	CCustomFileDialog dlg( op );

	INT_PTR result = dlg.DoModal();
	if (result == IDOK)
	{
		outputFile = dlg.GetFilePath();
		return true;
	}
	else if (result == IDRETRY)
	{
		s_singleFileDlgPref[fileType] = false;
		return SmartSelectSingleFile(fileType, outputFile, "", initialDir, filter);
	}
	return false;
}

bool CFileUtil::PopupMenu(const char* filename, const char* fullGamePath, CWnd* wnd, 
													bool* pIsSelected, CFileUtil::ExtraMenuItems *pItems)
{
	bool bRes = true;

	CString path = fullGamePath;
	if(filename && *filename)
		 path = Path::AddSlash(path) + filename;

	char rootpath[_MAX_PATH];
	GetCurrentDirectory(sizeof(rootpath),rootpath);
	CString fullPath = CString(rootpath) + "\\" + Path::GamePathToFullPath( path );

	uint32 nFileAttr = SCC_FILE_ATTRIBUTE_INVALID;

	// Pop up menu
	enum
	{
		MENU_SELECT = 1,
		MENU_OPEN,
		MENU_EXPLORE,

		MENU_COPY_NAME,
		MENU_COPY_PATH,

		MENU_SCM_GETPATH,
		MENU_SCM_ADD,
		MENU_SCM_CHECK_OUT,
		MENU_SCM_CHECK_IN,
		MENU_SCM_UNDO_CHECK_OUT,
		MENU_SCM_GET_LATEST,

		MENU_EXTRA,
	};

	if(gSettings.enableSourceControl && GetIEditor()->GetSourceControl())
	{
		nFileAttr = GetIEditor()->GetSourceControl()->GetFileAttributes( path );
	}

	// Create pop up menu.
	CMenu menu;
	menu.CreatePopupMenu();

	if(pIsSelected)
		menu.AppendMenu( MF_STRING, MENU_SELECT, "Select" );
	menu.AppendMenu( MF_STRING | ((nFileAttr & SCC_FILE_ATTRIBUTE_INPAK) ? MF_GRAYED : 0), MENU_OPEN, "Open" );
	menu.AppendMenu( MF_STRING | ((nFileAttr & SCC_FILE_ATTRIBUTE_INPAK) ? MF_GRAYED : 0), MENU_EXPLORE, "Explore" );
	menu.AppendMenu( MF_STRING, MENU_COPY_NAME, "Copy Name To Clipboard" );
	menu.AppendMenu( MF_STRING, MENU_COPY_PATH, "Copy Path To Clipboard" );

	if(gSettings.enableSourceControl && GetIEditor()->GetSourceControl() && nFileAttr!=SCC_FILE_ATTRIBUTE_INVALID)
	{
		bool isEnableSC = nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED;
		bool isInPak = nFileAttr & SCC_FILE_ATTRIBUTE_INPAK;
		menu.AppendMenu( MF_SEPARATOR, 0, "");
		if ( isInPak && !isEnableSC)
		{
			menu.AppendMenu( MF_STRING|MF_GRAYED, 0, "File In Pak (Read Only)" );
		}
		else
		{
			if(isEnableSC)
				menu.AppendMenu( MF_STRING, MENU_SCM_GETPATH, "Copy Source Control Path To Clipboard" );
			menu.AppendMenu( MF_STRING | (isEnableSC && !isInPak && (nFileAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT) ? 0: MF_GRAYED), MENU_SCM_CHECK_IN, "Check In" );
			menu.AppendMenu( MF_STRING | (isEnableSC && !isInPak && (nFileAttr & SCC_FILE_ATTRIBUTE_READONLY) ? 0: MF_GRAYED), MENU_SCM_CHECK_OUT,"Check Out" );
			menu.AppendMenu( MF_STRING | (isEnableSC && !isInPak && (nFileAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT) ? 0 : MF_GRAYED), MENU_SCM_UNDO_CHECK_OUT,"Undo Check Out" );
			menu.AppendMenu( MF_STRING | (isEnableSC ? 0 : MF_GRAYED), MENU_SCM_GET_LATEST,"Get Latest Version" );
			menu.AppendMenu( MF_STRING | (isEnableSC ? MF_GRAYED : 0), MENU_SCM_ADD, "Add To Source Control" );
		}
	}

	bool bThereAreSomeExtraItems = pItems && pItems->names.size() > 0;
	if (bThereAreSomeExtraItems)
	{
		menu.AppendMenu( MF_SEPARATOR, 0, "");
		for (size_t i=0; i<pItems->names.size(); ++i)
		{
			if (pItems->names[i].GetLength() > 0)
				menu.AppendMenu( MF_STRING, MENU_EXTRA + i, pItems->names[i] );
			else
				menu.AppendMenu( MF_SEPARATOR, 0, "");
		}
	}

	if(pIsSelected)
		*pIsSelected = false;
	CPoint point;
	GetCursorPos( &point );
	int cmd = menu.TrackPopupMenu( TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON, point.x, point.y, wnd);
	switch(cmd)
	{
		case MENU_SELECT:
			if(pIsSelected)
				*pIsSelected = true;
			break;
		case MENU_OPEN:
			if(!(nFileAttr & SCC_FILE_ATTRIBUTE_INPAK))
			{
				HINSTANCE hInst = ShellExecute( NULL, "open", fullPath, NULL, NULL, SW_SHOWNORMAL );
				if ((DWORD_PTR)hInst <= 32)
				{
					if (AfxMessageBox( "Can't open the file. Do you want to open the file in the Notepad?", MB_YESNO ) == IDYES)
					{
						ShellExecute( NULL, "open", "notepad", fullPath,	NULL, SW_SHOWNORMAL );
					}
				}
			}
			break;
		case MENU_EXPLORE:
		{
			if(nFileAttr & SCC_FILE_ATTRIBUTE_INPAK)
				ShellExecute(0, _T("explore"), Path::GetPath(fullPath), 0, 0, SW_SHOWNORMAL);
			else
			{
				if(int(ShellExecute(0, _T("open"), _T("explorer"), CString("/select, ")+filename, Path::GetPath(fullPath), SW_SHOWNORMAL))<=32)
					ShellExecute(0, _T("explore"), Path::GetPath(fullPath), 0, 0, SW_SHOWNORMAL);
			}
			break;
		}
		case MENU_COPY_NAME:
		{
			CClipboard clipboard;
			clipboard.PutString( filename );
			break;
		}
		case MENU_COPY_PATH:
		{
			CClipboard clipboard;
			clipboard.PutString( fullPath );
			break;
		}
		case MENU_SCM_GETPATH:
		{
			char outPath[MAX_PATH];
			bool bRes = GetIEditor()->GetSourceControl()->GetInternalPath( path, outPath, MAX_PATH);
			if(bRes && *outPath)
			{
				CClipboard clipboard;
				clipboard.PutString( outPath );
			}
			break;
		}
		case MENU_SCM_ADD:
		{
			CSourceControlDescDlg dlg;
			if(dlg.DoModal()==IDOK)
				bRes = GetIEditor()->GetSourceControl()->Add( path, dlg.m_sDesc );
			break;
		}
		case MENU_SCM_CHECK_OUT:
			bRes = GetIEditor()->GetSourceControl()->CheckOut( path );
			break;
		case MENU_SCM_CHECK_IN:
		{
			CSourceControlDescDlg dlg;
			if(dlg.DoModal()==IDOK)
				bRes = GetIEditor()->GetSourceControl()->CheckIn( path, dlg.m_sDesc );
			break;
		}
		case MENU_SCM_UNDO_CHECK_OUT:
			bRes = GetIEditor()->GetSourceControl()->UndoCheckOut( path );
			break;
		case MENU_SCM_GET_LATEST:
			bRes = GetIEditor()->GetSourceControl()->GetLatestVersion( path );
			break;
	}

	if (bThereAreSomeExtraItems)
	{
			if (cmd >= MENU_EXTRA)
				pItems->selectedIndexIfAny = cmd - MENU_EXTRA;
			else
				pItems->selectedIndexIfAny = -1;
	}

	if(!bRes)
		MessageBox(wnd ? wnd->GetSafeHwnd() : 0, "Source Control Operation Failed.\r\nCheck if Source Control Provider correctly setup and working directory is correct.", "Error", MB_OK | MB_ICONERROR);
	return bRes;
}