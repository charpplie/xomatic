////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   resourcecompiler.h
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __resourcecompiler_h__
#define __resourcecompiler_h__
#pragma once

#include "IRCLog.h"
#include "IResCompiler.h"
#include "Config.h"
#include "ExtensionManager.h"
#include "PakSystem.h"

#include <map>										// stl multimap<>


/** Implementation of IResCompiler interface.
*/
class ResourceCompiler :
	public IResourceCompiler,
	public IRCLog,
	public IConfigKeyRegistry
{
	friend class CTextureCompressor;
public:
	// constructor
	ResourceCompiler();
	// destructor
	~ResourceCompiler();

	//! e.g. print dependencies
	void PostBuild();

	int GetNumWarnings() const {return m_numWarnings;}
	int GetNumErrors() const {return m_numErrors;}

	// interface IConfigKeyRegistry --------------------------------------------

	virtual void VerifyKeyRegistration( const char *szKey );
	virtual bool VerifyKeyRegistration2( const char *szKey );

	// interface IResourceCompiler ---------------------------------------------

	virtual void RegisterKey( const char *key, const char *helptxt );
	virtual void EnsureDirectoriesPresent( const char *path );
	virtual void RegisterConvertor( IConvertor *conv );
	virtual FILE*	OpenFile( const char *filename,const char *mode );

	bool CompileFile(EPlatform platform, IConfig* config, const char* sourceFullFileName, const char* targetLeftPath, const char* sourceInnerPath, ICompiler* compiler, IConvertor* convertor, std::vector<CFileStats>* fileStats);

	//! Load and parse the Crytek Chunked File into the universal (very big) structure
	//! The caller should then call Release on the structure to free the mem
	//! @param filename Full filename including path to the file
//	virtual CryChunkedFile* LoadCryChunkedFile (const char* szFileName);
	
	virtual IRCLog *GetIRCLog();
	virtual IPakSystem* GetPakSystem();

	virtual void AddDependencyMaterial( const char *inszSrcFilename, const char *inszMatName, const char *inszScriptName );
	virtual void AddDependencyFile( const char *inszSrcFilename, const char *inszPathFileName );
	//virtual void AddFileStats( const char *inszPathFileName, const size_t SrcFileSize, const size_t DstFileSize, const char *inszPreset, const char *inszInfo );

	virtual void Init( IConfig* config );
	virtual void DeInit();
		
	virtual ICfgFile *CreateCfgFile();

	// interface IRCLog ---------------------------------------------

	virtual void AddCallback( ILogCallback *pCallback ) { assert(0); } // no implemented
	virtual void RemoveCallback( ILogCallback *pCallback ) { assert(0); } // no implemented
	virtual void Update();	// empty.
	virtual void LogV( const ELogType ineType, const char* szFormat, va_list args );
	void LogLine( const ELogType ineType, const char* szText );

	//////////////////////////////////////////////////////////////////////////
	// Resource compiler implementation.
	//////////////////////////////////////////////////////////////////////////
	bool Compile( EPlatform platform,IConfig* config,const char *filespec );
	bool ProcessJobFile( EPlatform platform,IConfig* config );

	const char* GetSectionName( EPlatform platform ) const;

	//! call this if user asks for help
	void show_help( const bool bDetailed );

	//! Returns the main application window
	HWND GetHWnd();
	HWND GetEmptyWindow();

	//////////////////////////////////////////////////////////////////////////
	void QueryVersionInfo();
	SFileVersion& GetFileVersion() { return m_fileVersion; }

	class ICryCompressorRC* GetCompressor() { return m_pCompressorRoutine; }

	void LogFailedFileInfo( IConfig* config,int index, const char* fileName, bool bLogSourceControlInfo, const char* sourceControlClientName );

	void InitLogs(IConfig *config);
	void SortLogFiles();
	void AddFileStats(void* fileStatsHandle, const char *inszPathFileName, const size_t SrcFileSize, const size_t DstFileSize, const char *inszPreset, const char *inszInfo);

	bool													m_bQuiet;							//!< true= don't log anything to console, otherwise false

	//!< number of supported threads 
	virtual int GetMaxNumThreads() const;

private:	
	void CreateSourceControl();
	void StartLogRecording();
	void StopLogRecording();
	void CopyFilesToTargetFolder( IConfig *config,const std::vector<string> &files,const string &targetFolder );
	bool CreatePakFile( IConfig *config,std::vector<string> files,const string &sourceFolder,const string &pakFilename,bool bUpdate );

	class XmlNodeRef LoadXml( const char *filename );
	void RunJobXmlNode( class CPropertyVars &properties,EPlatform platform,IConfig* config,XmlNodeRef &jobNode );

private:

	// private structures

	typedef std::multimap<string,string>    CFileDepMap;
	typedef std::pair<string,string>        CFileDepPair;

	class CMatDep
	{
	public:
		string        m_sMatName;       //!<
		string        m_sScriptName;    //!<

		bool operator==( const CMatDep &inRhS ) const
		{
			return(inRhS.m_sMatName==m_sMatName && inRhS.m_sScriptName==m_sScriptName);
		}
	};


	// helper to get order for CMatDep
	struct CMatDepOrder: public std::binary_function< CMatDep, CMatDep, bool>
	{
		bool operator() ( const CMatDep &a, const CMatDep &b ) const
		{
			// first sort by m_sScriptName (neccessary for printout)
			if(a.m_sScriptName<b.m_sScriptName)return(true);
			if(a.m_sScriptName>b.m_sScriptName)return(false);

			// then by m_sMatName
			if(a.m_sMatName<b.m_sMatName)return(true);
			if(a.m_sMatName>b.m_sMatName)return(false);

			return(false);
		}
	};

	// helper to get uint32 sorted with bigger values first
	struct CInvOrder: public std::binary_function< uint32, uint32, bool>
	{
		bool operator() ( const uint32 &a, const uint32 &b ) const
		{
			if(a>b)return true;
			if(a<b)return false;

			return false;
		}
	};

	typedef std::multimap<CMatDep,string,CMatDepOrder>	CMatDepMap;
	typedef std::pair<CMatDep,string>										CMatDepPair;

	//typedef std::multimap<string,uint32>								CPresetMap;
	// ------------------------------------------------



	ExtensionManager				m_extensionManager;			//!<

	// pointer to the compressing tool
	class ICryCompressorRC*	m_pCompressorRoutine;

	// pointer to the source control tool. note: it's allowed to be null.
	class ICrySourceControl* m_pSourceControl;
	bool m_bSourceControlCreated;
	FnCreateSourceControl m_fnCreateSourceControl;
	FnDestroySourceControl m_fnDestroySourceControl;

	CFileDepMap             m_FileDependencies;     //!< key=dependency e.g. nm.dds   value=file that is using it e.g. ball.cgf
	CMatDepMap              m_MaterialDependencies; //!< key=materialname+scriptmaterialname  value=file that is using it
	//CPresetMap            m_PresetUsage;          //!< key=presetname, value= file statistics index
	std::vector<CFileStats> m_Files;                //!< [index] = file statistics
	volatile long           m_numFilesProcessed;

	// log files
	string                  m_logFileName;			//!< for all messages, might be empty (no file logging)
	string                  m_warningLogFileName;   //!< for warnings only, might be empty (no file logging)
	string                  m_errorLogFileName;     //!< for errors only, might be empty (no file logging)

	ICfgFile *              m_presets;

	//
	bool                    m_bWarningHeaderLine;   //!< true= header was already printed, false= header needs to be printed
	bool                    m_bErrorHeaderLine;     //!< true= header was already printed, false= header needs to be printed
	bool                    m_bStatistics;          //!< true= show statistics.
	bool                    m_bWarningsAsErrors;    //!< true= Treat any warning as error.
	bool                    m_bLogRecordingEnabled;
	int                     m_maxThreads;           //!< max number of threads 
	string                  m_sHeaderLine;          //!<
	SFileVersion            m_fileVersion;
	SFileVersion            m_productVersion;
	string                  m_exePath;
	std::map<string,string> m_KeyHelp;              // [lower key] = help, created from RegisterKey
	
	// Log recording for compiled file.
	std::vector<string>     m_RecordedLogLines;
	typedef std::map<string,std::vector<string> > LogToFileMap;
	LogToFileMap m_LogForFileMap;

	HWND m_hEmptyWindow;

	PakSystem               m_pakSystem;

	int m_numWarnings;
	int m_numErrors;

	unsigned long m_threadIdTLSIndex;

	//
	void ShowFileDependencies();
	//
//	void ShowMaterialDependencies();
	//
	void ShowPresetUsage();
	// in format that can be easily read from Excel
	void ShowXLSFileSizes();
	// to remove old files for less confusion
	void RemoveOutputFiles();
	//
	void SetHeaderLine( const char *inszLine );

	void InitializeThreadIds();
	int GetThreadId();

	void SortLogFileByThread(FILE* file);

	// to log multiple lines (\n separated) with padding before
	void LogMultiLine( const char *szText );

	friend void RegisterCompressor( ResourceCompiler *rc );		
};


class CryCriticalSectionRC
{
public:
	CryCriticalSectionRC() { 
		memset(&m_cs, 0, sizeof(m_cs));
		InitializeCriticalSection(&m_cs); 
	}

	~CryCriticalSectionRC() { DeleteCriticalSection(&m_cs); }

	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }
	bool TryLock() { return TryEnterCriticalSection(&m_cs) != FALSE; }

#ifndef NDEBUG
	bool IsLocked() { return m_cs.RecursionCount > 0 && (DWORD)m_cs.OwningThread == GetCurrentThreadId(); }
#endif

private:
	CRITICAL_SECTION m_cs;
};

class CryAutoLockRC
{
private:
	CryCriticalSectionRC &m_Lock;

	CryAutoLockRC();
	CryAutoLockRC(const CryAutoLockRC&);
	CryAutoLockRC& operator = (const CryAutoLockRC&);

public:
	CryAutoLockRC(CryCriticalSectionRC &Lock) : m_Lock(Lock) { m_Lock.Lock(); }
	~CryAutoLockRC() { m_Lock.Unlock(); }
};

#endif // __resourcecompiler_h__

