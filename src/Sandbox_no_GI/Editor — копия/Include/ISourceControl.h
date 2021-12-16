////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   ISourceControl.h
//  Version:     v1.00
//  Created:     1/9/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ISourceControl_h__
#define __ISourceControl_h__
#pragma once

// Source control status of item.
enum ESccFileAttributes
{
	SCC_FILE_ATTRIBUTE_INVALID       = 0x0000, // File is not found.
	SCC_FILE_ATTRIBUTE_NORMAL        = 0x0001, // Normal file on disk.
	SCC_FILE_ATTRIBUTE_READONLY     = 0x0002, // Read only files cannot be modified at all, either read only file not under source control or file in packfile.
	SCC_FILE_ATTRIBUTE_INPAK        = 0x0004, // File is inside pack file.
	SCC_FILE_ATTRIBUTE_MANAGED       = 0x0008, // File is managed under source control.
	SCC_FILE_ATTRIBUTE_CHECKEDOUT   = 0x0010, // File is under source control and is checked out.
	SCC_FILE_ATTRIBUTE_BYANOTHER    = 0x0020, // File is under source control and is checked out by another user.
};

// GetLatestVersion flags
enum ESccGetLatest
{
  GETLATEST_NO_REVERT       = 0x0001, // don't revert when perform GetLatestVersion() on opened files
  GETLATEST_ONLY_CHECK      = 0x0002, // don't actually get latest version of the file, check if scc has more recent version
};

// To match versions of editor and DLL
#define PERFORCE_PLUGIN_CURRENT_VERSION 3
#define PERFORCE_PLUGIN_NAME "Perforce plug-in"

//////////////////////////////////////////////////////////////////////////
// Description
//    This interface provide access to the source control functionality.
//////////////////////////////////////////////////////////////////////////
struct __declspec( uuid("{1D391E8C-A124-46bb-808D-9BCA155BCAFD}") ) ISourceControl : public IUnknown
{
	// Description:
	//    Returns attributes of the file.
	// Return:
	//    Combination of flags from ESccFileAttributes enumeration.
	virtual uint32 GetFileAttributes( const char *filename ) = 0;

	virtual bool Add( const char *filename, const char * desc = 0, int nFlags = 0) = 0;
	virtual bool CheckIn( const char *filename, const char * desc=0, int nFlags = 0) = 0;
	virtual bool CheckOut( const char *filename, int nFlags = 0) = 0;
	virtual bool UndoCheckOut( const char *filename, int nFlags = 0) = 0;
	virtual bool Rename( const char *filename, const char *newfilename, const char * desc = 0, int nFlags = 0) = 0;
	virtual bool Delete( const char *filename, const char * desc = 0, int nFlags = 0) = 0;
	virtual bool GetLatestVersion( const char *filename, int nFlags = 0) = 0;

	// GetInternalPath - Get internal Source Control path for file filename
	// outPath: output char buffer, provided by user, MAX_PATH - recommended size of this buffer
	// nOutPathSize: size of outPath buffer, MAX_PATH is recommended
	// if a smaller size will be used, it may returned a truncated path.
	virtual bool GetInternalPath( const char *filename, char* outPath, int nOutPathSize) = 0;


	//////////////////////////////////////////////////////////////////////////
	// IUnknown
	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,void** ppvObject ) { return E_NOINTERFACE; };
	virtual ULONG STDMETHODCALLTYPE AddRef() { return 0; };
	virtual ULONG STDMETHODCALLTYPE Release() { return 0; };
	//////////////////////////////////////////////////////////////////////////
};

#endif // __ISourceControl_h__

