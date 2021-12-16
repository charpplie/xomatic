////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   convertcontext.h
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __convertcontext_h__
#define __convertcontext_h__
#pragma once

#include "PathHelpers.h"

struct IResourceCompiler;
struct IConfig;
struct IRCLog;
struct ICfgFile;

//! Section in rc.ini file for common settings.
#define COMMON_SECTION "Common"

// [MichaelS] Moved this into convert context from ResourceCompiler so that it can be threadsafe.
// Stats are gathered in a vector per thread and then collated at the end.
class CFileStats
{
public:
	uint32        m_SrcFileSizeKB;  //!< in KB
	uint32        m_DstFileSizeKB;  //!< in KB
	string        m_sInfo;          //!< \t separated list of properties (used for excel export)
	string        m_sDestFilename;  //!<
	string        m_sPreset;        //!<
};

/** Enumeration of supported platforms.
*/
enum EPlatform
{
	ePlatform_UNKNOWN,	//!< Unknown platform.
	ePlatform_PC,
//	ePlatform_XBOX,
//	ePlatform_PS2,
//	ePlatform_GAMECUBE,
//	ePlatform_WII,
	ePlatform_PS3,
	ePlatform_X360,

	// This entry must be last in EPlatform enum.
	ePlatform_LAST,
};

struct IConvertContext
{
	virtual void Release() { delete this; }

	virtual void SetSourceFileFinal(const char* sourceFileFinal) = 0;
	virtual void SetSourceFileFinalExtension(const char* sourceFileFinalExtension) = 0;
	virtual void SetSourceFolder(const char* sourceFolder) = 0;
	virtual void SetOutputFile(const char* outputFile) = 0;
	virtual void SetOutputFolder(const char* sOutputFolder) = 0;
	virtual void SetRC(IResourceCompiler* pRC) = 0;
	virtual void SetConfig(IConfig *config) = 0;
	virtual void SetQuiet(bool bQuiet) = 0;
	virtual void SetPresets(ICfgFile *presets) = 0;
	virtual void SetPlatform(EPlatform platform) = 0;
	virtual void SetPlattformName(const char* plattformName) = 0;
	virtual void SetThreads(int threads) = 0;
	virtual void SetFileStatsHandle(void* fileStatsHandle) = 0;
};

struct IConfig;

/** ConvertContext is a description of what should be processed by convertor.
*/
struct ConvertContext : public IConvertContext
{
	virtual void SetSourceFileFinal(const char* sourceFileFinal) {this->sourceFileFinal = sourceFileFinal;}
	virtual void SetSourceFileFinalExtension(const char* sourceFileFinalExtension) {this->sourceFileFinalExtension = sourceFileFinalExtension;}
	virtual void SetSourceFolder(const char* sourceFolder) {this->sourceFolder = sourceFolder;}
	virtual void SetOutputFile(const char* outputFile) {this->outputFile = outputFile;}
	virtual void SetOutputFolder(const char* sOutputFolder) {this->outputFolder = sOutputFolder;}
	virtual void SetRC(IResourceCompiler* pRC) {this->pRC = pRC;}
	virtual void SetConfig(IConfig *config) {this->config = config;}
	virtual void SetQuiet(bool bQuiet) {this->bQuiet = bQuiet;}
	virtual void SetPresets(ICfgFile *presets) {this->presets = presets;}
	virtual void SetPlatform(EPlatform platform) {this->platform = platform;}
	virtual void SetPlattformName(const char* plattformName) {this->plattformName = plattformName;}
	virtual void SetThreads(int threads) {this->threads = threads;}
	virtual void SetFileStatsHandle(void* fileStatsHandle) {this->fileStatsHandle = fileStatsHandle;}

	string getSourcePath() const { return PathHelpers::Join(sourceFolder,sourceFileFinal); }
	string getOutputPath() const { return PathHelpers::Join(outputFolder,outputFile); }

	// Returns
	//   path to the resource compiler exe including path e.g.  "c:\temp\bin32\rc\"
	string getCompilerFolderPath() const
	{
		char szRCPath[1000];

		if (GetModuleFileName (NULL, szRCPath, sizeof(szRCPath)))
			return PathHelpers::GetDirectory(szRCPath);

		assert(0);
		return "";
	}

	//! Source file's folder.
	string sourceFolder;
	//! Source file that needs to be converted (final name of file - e.g. "dirt.tif").
	//! Contains filename only, the folder stored in sourceFolder.
	string sourceFileFinal;
	// Source file extension (use this if you need the extension, the sourceFile might not have the right extension e.g. .tmp)
	// is always lower case (e.g. ".tif")
	string sourceFileFinalExtension;

	//! Pointer to resource compiler interface.
	IResourceCompiler *pRC;
	//! Configuration settings for this file.
	IConfig *config;

	//! Don't log much info into log.
	bool bQuiet;

	//! file specific config file, must not be 0 (if there was no file, a empty instance with the right filename is created)
//	ICfgFile *pFileSpecificConfig;

	//! presets config (for use with the image compiler)
	ICfgFile *presets;

	//! Platform to which file must be processed.
	EPlatform platform;

	//! e.g. "PC", "XBOX"
	string plattformName;

	int threads;                                 // Number of threads 
	void* fileStatsHandle;

private:
	// Output folder.
	string outputFolder;
	//! Output file that would be created from sourceFile (relative or absolute)
	string outputFile;
};

#endif // __convertcontext_h__