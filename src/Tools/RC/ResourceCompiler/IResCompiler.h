////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   irescompiler.h
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: IResourceCompiler interface.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __irescompiler_h__
#define __irescompiler_h__
#pragma once

#include <stdio.h>

struct IConvertor;
class  ICryCompressorRC;
class  ICrySourceControl;
struct CryChunkedFile;
struct IRCLog;
struct IPhysicalWorld;
struct ICfgFile;
struct ICompiler;
struct ConvertContext;
class IPakSystem;

/** Main interface of resource compiler.
*/
struct IResourceCompiler
{
	//! Register new convertor.
	virtual void RegisterConvertor( IConvertor *conv ) = 0;

	//! Use this instead of fopen.
	virtual FILE*	OpenFile( const char *filename,const char *mode ) = 0;

	//! Returns the main application window
	virtual HWND GetHWnd() = 0;

	// returns a handle to an empty window to be used with DirectX
	virtual HWND GetEmptyWindow() = 0;
	
	//! \return is always a valid pointer
	virtual IRCLog *GetIRCLog()=0;

	// Get the interface for opening files - handles files stored in ZIP archives.
	virtual IPakSystem* GetPakSystem() = 0;

	//! \inszName full name like in 3dsmax
	virtual void AddDependencyMaterial( const char *inszSrcFilename, const char *inszMatName, const char *inszScriptName )=0;

	//! \inszPathFileName absolute path names
	virtual void AddDependencyFile( const char *inszSrcFilename, const char *inszPathFileName )=0;

	virtual void AddFileStats(void* fileStatsHandle, const char *inszPathFileName, const size_t SrcFileSize, const size_t DstFileSize, const char *inszPreset, const char *inszInfo) = 0;

	virtual struct SFileVersion& GetFileVersion() = 0;

	// initialize rc configuration
	virtual void Init( struct IConfig* config ) = 0;

	// initiate cleanup process - call DeInit() for registered compilers
	virtual void DeInit() = 0;

	virtual ICfgFile *CreateCfgFile() = 0;

	// Arguments:
	//   key - must not be 0
	//   helptxt - must not be 0
	virtual void RegisterKey( const char *key, const char *helptxt )=0;

	// Get compressor routine
	virtual ICryCompressorRC* GetCompressor() = 0;

	// number of supported threads 
	virtual int GetMaxNumThreads() const = 0;

	// Argumenst:
	//   path - must not be 0, path e.g. "c:\temp\test"
	virtual void EnsureDirectoriesPresent( const char *path )=0;
};


// this is the plugin function that's exported by plugins
// Registers all convertors residing in this DLL
extern "C" {
typedef void  (__stdcall* FnRegisterConvertors )(IResourceCompiler*pRC);
}

// this is the compressor function that's exported by compressor
// Registers compressor residing in this DLL
extern "C" {
	typedef ICryCompressorRC*  (__stdcall* FnCreateComressor )(IResourceCompiler*pRC);
}

// these are the source control functions which are exported by source control module
// Creates/destroys source control object residing in DLL
extern "C" {
	typedef ICrySourceControl*  (__stdcall* FnCreateSourceControl  )();
	typedef void                (__stdcall* FnDestroySourceControl )(ICrySourceControl*pSC);
}

#endif // __irescompiler_h__
