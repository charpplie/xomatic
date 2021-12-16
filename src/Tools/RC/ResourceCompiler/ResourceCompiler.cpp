// ResourceCompiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <time.h>
#include <DbgHelp.h>
#include <io.h>
#include "ResourceCompiler.h"
#include "CmdLine.h"
#include "Config.h"
#include "CfgFile.h"
#include "FileUtil.h"
#include "IConvertor.h"
#include "ICrySourceControl.h"
#include "CrashHandler.h"
#include "CpuInfo.h"
#include "Mailer.h"
#include "StringHelpers.h"
#include "ListFile.h"
#include "ICryXML.h"
#include "IXmlSerializer.h"
#include "crc32.h"
#include "PropertyVars.h"

#pragma comment( lib, "Version.lib" )

static const char* const RC_FILENAME_LOG           = "rc_log.log";
static const char* const RC_FILENAME_WARNINGS      = "rc_log_warnings.log";
static const char* const RC_FILENAME_ERRORS        = "rc_log_errors.log";
static const char* const RC_FILENAME_CRASH_DUMP    = "rc_crash.dmp";
static const char* const RC_FILENAME_FILEDEP       = "rc_stats_filedependencies.log";
//static const char* RC_FILENAME_MATDEP = "rc_stats_materialdependencies.log";
static const char* const RC_FILENAME_PRESETUSAGE   = "rc_stats_presetusage.log";
static const char* const RC_FILENAME_XLS_FILESIZES = "rc_stats_filesizes_xls.log";     // in format that can be easily read from Excel



//////////////////////////////////////////////////////////////////////////
// Globals.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// If not in static library.
//#ifdef CRY_STRING
//int sEmptyStringBuffer[] = { -1, 0, 0, 0 };
//string::StrHeader* string::m_emptyStringData = (string::StrHeader*)&sEmptyStringBuffer;
//wstring::StrHeader* wstring::m_emptyStringData = (wstring::StrHeader*)&sEmptyStringBuffer;
//#endif //CRY_STRING

// Determines whether a path to a file system object such as a file or directory is valid

BOOL RCPathFileExists (const char* szPath)
{
	DWORD dwAttr = GetFileAttributes (szPath);
	return (dwAttr != 0xFFFFFFFF);
}


//////////////////////////////////////////////////////////////////////////
// ResourceCompiler implementation.
//////////////////////////////////////////////////////////////////////////
ResourceCompiler::ResourceCompiler()
{
	m_bWarningHeaderLine=false;
	m_bErrorHeaderLine=false;
	m_bStatistics = false;
	m_bQuiet = false;
	m_maxThreads = 0;
	m_numWarnings = 0;
	m_numErrors = 0;
	m_bSourceControlCreated = false;
	m_bLogRecordingEnabled = false;
	InitializeThreadIds();
}

ResourceCompiler::~ResourceCompiler()
{
	if(m_bSourceControlCreated)
	{
		if(m_pSourceControl)
		{
			m_fnDestroySourceControl(m_pSourceControl);
			m_pSourceControl = 0;
		}
		m_bSourceControlCreated = false;
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RegisterConvertor( IConvertor *conv )
{
	m_extensionManager.RegisterConvertor( conv, this );
}

//////////////////////////////////////////////////////////////////////////
FILE*	ResourceCompiler::OpenFile( const char *filename,const char *mode )
{
	FILE *file = fopen(filename,mode);
	// check if read only.
	return file;
}

IRCLog *ResourceCompiler::GetIRCLog()
{
	return(this);
}

//////////////////////////////////////////////////////////////////////////
IPakSystem* ResourceCompiler::GetPakSystem()
{
	return &m_pakSystem;
}

//////////////////////////////////////////////////////////////////////////
const char* ResourceCompiler::GetSectionName( EPlatform platform ) const
{
	switch (platform)
	{
	case ePlatform_PC:				return "PC";
//	case ePlatform_XBOX:			return "XBOX";
//	case ePlatform_PS2:			return "PS2";
//	case ePlatform_GAMECUBE:	return "GAMECUBE";
//	case ePlatform_WII:			return "WII";
	case ePlatform_PS3:			return "PS3";
	case ePlatform_X360:			return "X360";
	default:
		// unknown platform.
		MessageBoxError( _T("Section name requested for unknown platform") );
		assert(0);
	}
	return "";
}

void ResourceCompiler::RemoveOutputFiles()
{
	DeleteFile(m_exePath+RC_FILENAME_FILEDEP);
//	DeleteFile(RC_FILENAME_MATDEP);
	DeleteFile(m_exePath+RC_FILENAME_PRESETUSAGE);
	DeleteFile(m_exePath+RC_FILENAME_XLS_FILESIZES);
}



class FilesToConvert
{
public:
	std::vector<string> m_allFiles;
	std::vector<string> m_inputFiles_innerPathAndName;
	std::vector<string> m_outOfMemoryFiles;
	std::vector<string> m_failedFiles;
	std::vector<string> m_convertedFiles;
private:
	CryCriticalSectionRC m_lock;

public:
	void lock()
	{
		m_lock.Lock();
	}

	void unlock()
	{
		m_lock.Unlock();
	}
};


struct ThreadData
{
	ResourceCompiler* rc;
	FilesToConvert *pFilesToConvert;
	unsigned long threadIdTLSIndex;
	int threadId;
	string sourceLeftPath;
	string targetLeftPath;
	EPlatform platform;
	Config config;
	IConvertor* convertor;
	ICompiler* compiler;
	std::vector<CFileStats> fileStats;
};


unsigned int WINAPI ThreadFunc(void* threadDataMemory);
	

static void CompileFilesMultiThreaded(
	ResourceCompiler* pRC,
	unsigned long a_threadIdTLSIndex,
	FilesToConvert& a_files,
	int threadCount,
	const string& a_sourceLeftPath,
	const string& a_targetLeftPath,
	EPlatform platform,
	IConfig* config,
	IConvertor* convertor,
	std::vector<CFileStats>& a_filesStats)
{
	if (threadCount <= 0)
	{
		return;
	}

	while(!a_files.m_inputFiles_innerPathAndName.empty())
	{
		// Never create more threads than needed
		if (threadCount > a_files.m_inputFiles_innerPathAndName.size())
		{
			threadCount = a_files.m_inputFiles_innerPathAndName.size();
		}

		RCLog("Spawning %d threads.", threadCount);

		char szRCPath[1000];
		GetModuleFileName(NULL, szRCPath, sizeof(szRCPath));
		string exePath = PathHelpers::GetDirectory(szRCPath);

		// Initialize the convertor
		convertor->Init(config, exePath.c_str());

		// Initialize the thread data for each thread.
		std::vector<ThreadData> threadData(threadCount);
		for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			threadData[threadIndex].rc = pRC;
			threadData[threadIndex].sourceLeftPath = a_sourceLeftPath;
			threadData[threadIndex].targetLeftPath = a_targetLeftPath;
			threadData[threadIndex].platform = platform;
			threadData[threadIndex].config.SetConfigKeyRegistry(pRC);
			threadData[threadIndex].config.Merge(config);
			threadData[threadIndex].convertor = convertor;
			threadData[threadIndex].compiler = convertor->CreateCompiler();
			threadData[threadIndex].threadIdTLSIndex = a_threadIdTLSIndex;
			threadData[threadIndex].threadId = threadIndex + 1;
			threadData[threadIndex].pFilesToConvert = &a_files;
		}

		// Spawn the threads.
		std::vector<HANDLE> threads(threadCount);
		for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			threads[threadIndex] = (HANDLE)_beginthreadex( 
				0,                               //void *security,
				0,                               //unsigned stack_size,
				ThreadFunc,                      //unsigned ( *start_address )( void * ),
				&threadData[threadIndex],        //void *arglist,
				0,                               //unsigned initflag,
				0);                              //unsigned *thrdaddr 
		}

		// Wait until all the threads have exited
		int lastCountShown = -1;
		while(WaitForMultipleObjects(threads.size(), &threads[0], TRUE, 500) == WAIT_TIMEOUT)
		{
			// Show progress

			a_files.lock();

			if(!a_files.m_inputFiles_innerPathAndName.empty())
			{
				const int processedFileCount = a_files.m_outOfMemoryFiles.size() + a_files.m_failedFiles.size() + a_files.m_convertedFiles.size();

				if(processedFileCount != lastCountShown)
				{
					lastCountShown = processedFileCount;
					const float fPercentage = (100.f * processedFileCount) / a_files.m_allFiles.size();
					char str[1024];
					_snprintf(str, sizeof(str),"Progress: %.1f%% %s", fPercentage, a_files.m_inputFiles_innerPathAndName.back().c_str());
					SetConsoleTitle(str);
				}
			}

			a_files.unlock();
		}

		assert(a_files.m_inputFiles_innerPathAndName.empty());

		// Release all the compiler objects.
		for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			threadData[threadIndex].compiler->Release();
		}

		// Compile all statistics from all threads.
		for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			a_filesStats.insert(a_filesStats.end(), threadData[threadIndex].fileStats.begin(), threadData[threadIndex].fileStats.end());
		}

		// Clean up the converter.
		convertor->DeInit();

		if(!a_files.m_outOfMemoryFiles.empty())
		{
			if(threadCount > 1)
			{
				// If we ran out of memory when processing files, we will process the files again in one thread (since we may
				// have run out of memory just because we had multiple threads).
				a_files.m_inputFiles_innerPathAndName.insert( a_files.m_inputFiles_innerPathAndName.end(), a_files.m_outOfMemoryFiles.begin(), a_files.m_outOfMemoryFiles.end() );
				threadCount = 1; 
			}
			else
			{
				a_files.m_failedFiles.insert( a_files.m_failedFiles.end(), a_files.m_outOfMemoryFiles.begin(), a_files.m_outOfMemoryFiles.end() );
			}

			a_files.m_outOfMemoryFiles.resize(0);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// Returns true if successfully converted at least one file
bool ResourceCompiler::Compile( EPlatform platform,IConfig *config,const char *filespec )
{
	RemoveOutputFiles();		// to remove old files for less confusion

	m_numFilesProcessed = 0;	// init progress

	m_bStatistics = config->HasKey("statistics");

	FilesToConvert filesToConvert;	// paths in filesToConvert.m_allFiles are relative

	const bool bRecursive = config->GetAs<bool>("recursive", true);
	
	m_presets = new CfgFile();
	string presetcfg;

	if(!config->Get("presetcfg", presetcfg))
	{
		char str[512];

		sprintf(str,"No preset configuration defined (e.g. presetcfg=rc_presets_pc.ini)");

		RCLog("%s",str);

		MessageBox(0,str,"ResourceCompiler Error",MB_OK|MB_ICONERROR);
	
		// it's better to have resource not working without that info
		return false;
	}
	else if(!m_presets->Load( PathHelpers::Join(m_exePath,presetcfg)) )
	{
		char str[512];

		sprintf(str,"Failed to read preset configuration %s, exiting...", presetcfg.c_str());

		RCLog("%s",str);

		MessageBox(0,str,"ResourceCompiler Error",MB_OK|MB_ICONERROR);

		// it's better to have resource not working without that info
		return false;
	}

	RCLog("used preset configuration: presetcfg=%s",presetcfg.c_str());

	string dirToIgnore;
	const char* pDirToIgnore = 0;
	if (config->Get( "targetroot", dirToIgnore ))
	{
		pDirToIgnore = dirToIgnore.c_str();
	}


	bool bUseListFile = false;
	string listFile;
	config->Get( "listfile",listFile );
	if (!listFile.empty())
	{
		bUseListFile = true;
		CListFile lf(this);
		lf.Process( listFile,filespec,filesToConvert.m_allFiles );

		if (filesToConvert.m_allFiles.empty())
		{
			RCLogError("No Files to Convert found from List File.");
			return false;
		}
	}

	// RC expects source filenames in command line in form 
	// "<source left path><mask for recursion>" or "<source left path><non-masked name>".
	// After recursive subdirectory scan we will have a list with source filenames in 
	// form "<source left path><source inner path><name>".
	// The target filename can be written as "<target left path><source inner path><name>".

	string sourceLeftPath = PathHelpers::GetDirectory(filespec);

	if (!bUseListFile)
	{
		const DWORD dwFileSpecAttr = GetFileAttributes(filespec);

		if (dwFileSpecAttr == 0xFFFFFFFF)
		{
			// There's no such file

			if (PathHelpers::GetFilename(filespec).find_first_of("*?") == string::npos)
			{
				// It's not a mask (path\*.mask). Allow later part of the code to open it from the .pak (see PakSystem::Open()).
				RCLog("");
				RCLog("RC can't open file %s. Trying to find the file in .pak files...", filespec);
				filesToConvert.m_allFiles.push_back(PathHelpers::GetFilename(filespec));
			}
			else
			{
				// It's a mask (path\*.mask). Scan directory and accumulate matching filenames in the list.
				RCLog("");
				RCLog("Scanning directory '%s' for '%s'...", sourceLeftPath.c_str(), PathHelpers::GetFilename(filespec));
				FileUtil::ScanDirectory(sourceLeftPath, PathHelpers::GetFilename(filespec), filesToConvert.m_allFiles, bRecursive, pDirToIgnore);
				RCLog("");

				if (filesToConvert.m_allFiles.empty())
				{
					// Failed to find any file matching the mask specified by user.
					// Using mask (say, *.cgf) usually means that user doesn't know if
					// the file exists or not, so it's better to return "success" code.
					RCLog("RC can't find files matching %s, 0 files converted", filespec);
					return true;
				}
			}
		}
		else
		{
			// The file exists

			if (dwFileSpecAttr & FILE_ATTRIBUTE_DIRECTORY)
			{
				// We found a file, but it's a directory, not a regular file.
				// Let's assume that the user wants to export every file in the 
				// directory (with subdirectories if bRecursive == true) or
				// that he wants to export a file specified in /file option.
				sourceLeftPath = PathHelpers::AddSeparator(filespec);
				string const filename = config->GetAs<string>("file", "*.*");
				RCLog("");
				RCLog("Scanning directory '%s' for '%s'...", sourceLeftPath.c_str(), filename.c_str());
				FileUtil::ScanDirectory(sourceLeftPath, filename, filesToConvert.m_allFiles, bRecursive, pDirToIgnore);

				if (filesToConvert.m_allFiles.empty())
				{
					if (filename.find_first_of("*?") == string::npos)
					{
						// Failed to find the file specified by user.
						// Using a filename without mask (say, abcde.cgf) usually means that user
						// expects that the file exists, so it's better to return "failure" code.
						RCLog("RC can't find file %s, 0 files converted", filespec);
						return false;
					}
					else
					{
						// Failed to find any file matching the mask specified by user.
						// Using mask (say, *.cgf) usually means that user doesn't know if
						// the file exists or not, so it's better to return "success" code.
						RCLog("RC can't find files matching %s, 0 files converted", filespec);
						return true;
					}
				}
			}
			else
			{	
				// we found a regular file
				filesToConvert.m_allFiles.push_back(PathHelpers::GetFilename(filespec));
			}
		}

		if (filesToConvert.m_allFiles.empty())
		{
			RCLogError("No Files to Convert.");
			return false;
		}
	}

	// determine the target output path (may be a different directory structure)
	// if none is specified, the target is the same as the source, as before.
	string targetLeftPath;
	if (!config->Get( "targetroot", targetLeftPath ))
	{
		targetLeftPath = sourceLeftPath;
	}
	else
	{
		bool bCopyOnly = false;
		config->Get( "copyonly", bCopyOnly );
		if (bCopyOnly)
		{
			FileUtil::CreateDirectoryRecursive(targetLeftPath);
			CopyFilesToTargetFolder( config,filesToConvert.m_allFiles,targetLeftPath );
			return true;
		}
	}
	targetLeftPath = PathHelpers::AddSeparator(targetLeftPath);

	{
		string pakFilename;
		if (config->Get( "CreatePakFile", pakFilename ))
		{
			return CreatePakFile( config,filesToConvert.m_allFiles,sourceLeftPath,pakFilename,true );
		}
	}

	// these are the files that couldn't be converted
	std::vector<string> arrNonConvertedFiles;

	int nTimer = GetTickCount();

	// Split up the files based on the convertor they are to use.
	typedef std::map<IConvertor*, std::vector<string> > FileConvertorMap;
	FileConvertorMap fileConvertorMap;
	for (size_t i = 0; i < filesToConvert.m_allFiles.size(); ++i)
	{
		string filenameForConvertorSearch;
		{
			string sOverWriteExtension;
			if(config->Get("overwriteextension",sOverWriteExtension))
			{
				filenameForConvertorSearch = string("filename.") + sOverWriteExtension;
			}
			else
			{
				filenameForConvertorSearch = filesToConvert.m_allFiles[i];
			}
		}

		IConvertor* convertor = m_extensionManager.FindConvertor(platform, filenameForConvertorSearch.c_str());

		FileConvertorMap::iterator convertorIt = fileConvertorMap.find(convertor);
		if (convertorIt == fileConvertorMap.end())
		{
			convertorIt = fileConvertorMap.insert(std::make_pair(convertor, std::vector<string>())).first;
		}
		(*convertorIt).second.push_back(filesToConvert.m_allFiles[i]);
	}

	if (config->HasKey("verbose"))
	{
		RCLog("%i file%s to convert:", filesToConvert.m_allFiles.size(), ((filesToConvert.m_allFiles.size()!=1) ? "s" : ""));
		for (size_t i = 0; i < filesToConvert.m_allFiles.size(); ++i)
		{
			RCLog("  %s", filesToConvert.m_allFiles[i].c_str());
		}
		RCLog("");
	}

	// Loop through all the convertors that we need to invoke.
	for (FileConvertorMap::iterator convertorIt = fileConvertorMap.begin(); convertorIt != fileConvertorMap.end(); ++convertorIt)
	{
		assert(filesToConvert.m_inputFiles_innerPathAndName.empty());
		assert(filesToConvert.m_outOfMemoryFiles.empty());

		IConvertor* convertor = (*convertorIt).first;
		if (!convertor)
		{
			continue;
		}

		// Check whether this convertor is thread-safe.
		assert(m_maxThreads>=1);
		int threadCount = m_maxThreads;
		if ((threadCount > 1) && (!convertor->SupportsMultithreading()))
		{
			RCLog("/threads specified, but convertor does not support multi-threading. Falling back to single-threading.");
			threadCount = 1;
		}

		const std::vector<string>& convertorFiles = (*convertorIt).second;
		assert(convertorFiles.size()>0);

		// implementation note: we insert filenames starting from last, because converting function will take filenames one by one from the end(!) of the array
		for(int i=convertorFiles.size()-1; i>=0; --i)
		{
			filesToConvert.m_inputFiles_innerPathAndName.push_back( convertorFiles[i] );
		}

		CompileFilesMultiThreaded( this, m_threadIdTLSIndex, filesToConvert, threadCount, sourceLeftPath, targetLeftPath, platform, config, convertor, m_Files );

		assert(filesToConvert.m_inputFiles_innerPathAndName.empty());
		assert(filesToConvert.m_outOfMemoryFiles.empty());
	}

	const int numFilesConverted = filesToConvert.m_convertedFiles.size();
	const int numFilesFailed = filesToConvert.m_failedFiles.size();
	assert(numFilesConverted+numFilesFailed==filesToConvert.m_allFiles.size());

	nTimer = GetTickCount() - nTimer;
	char szTimeMsg[128] ;
	szTimeMsg[0] = '\0';
	if (nTimer > 500)
		sprintf (szTimeMsg, " in %.1f sec", nTimer/1000.0f);

	RCLog("");

	if (numFilesFailed <= 0)
	{
		RCLog("%d file%s processed%s.", numFilesConverted, (numFilesConverted>1?"s":""), szTimeMsg);
	}
	else
	{
		const bool bLogSourceControlInfo = config->HasKey("sourcecontrol");
		string sourceControlClientName;

		if(bLogSourceControlInfo)
		{
			// Lazy registering of source control
			if(!m_bSourceControlCreated)
			{
				CreateSourceControl();
			}
			if(!config->Get("sourcecontrol", sourceControlClientName))
			{
				sourceControlClientName = "";
			}
		}

		RCLog("");
		RCLog(
			"%d of %d file%s were converted%s. Couldn't convert the following file%s:", 
			numFilesConverted, numFilesConverted+numFilesFailed, (numFilesConverted+numFilesFailed > 1 ? "s":""), szTimeMsg, (numFilesFailed>1?"s":""));
		RCLog("");
		for(size_t i = 0; i < numFilesFailed; ++i)
		{
			LogFailedFileInfo( config,i, filesToConvert.m_failedFiles[i], bLogSourceControlInfo, sourceControlClientName.c_str());
		}
		RCLog("");
	}

	delete m_presets;

	return numFilesConverted > 0;
}


static void EnableFloatingPointExceptions()
{
	_clearfp();
	unsigned int old;
	_controlfp_s( &old, _EM_INEXACT, _MCW_EM );
}


unsigned int WINAPI ThreadFunc(void* threadDataMemory)
{
//	EnableFloatingPointExceptions();

	ThreadData* data = static_cast<ThreadData*>(threadDataMemory);

	// Initialize the thread local storage, so the log can prepend the thread id to each line.
	TlsSetValue(data->threadIdTLSIndex, &data->threadId);

	// Create a copy of the main configuration.
	Config localConfig;
	localConfig.SetConfigKeyRegistry(data->rc);
	localConfig.Merge(&data->config);	// merge main config into local config

	for(;;)
	{
		data->pFilesToConvert->lock();

		if (data->pFilesToConvert->m_inputFiles_innerPathAndName.empty())
		{
			data->pFilesToConvert->unlock();
			break;
		}

		const string sourceInnerPathAndName = data->pFilesToConvert->m_inputFiles_innerPathAndName.back();
		data->pFilesToConvert->m_inputFiles_innerPathAndName.pop_back();

		data->pFilesToConvert->unlock();

		const string sourceInnerPath = PathHelpers::GetDirectory(sourceInnerPathAndName);
		const string sourceFullFileName = PathHelpers::Join(data->sourceLeftPath, sourceInnerPathAndName);

		int result;

		try
		{
			if (data->rc->CompileFile(data->platform, &localConfig, sourceFullFileName.c_str(), data->targetLeftPath.c_str(), sourceInnerPath.c_str(), data->compiler, data->convertor, &data->fileStats))
			{
				result = 1;
			}
			else
			{
				result = 2;
			}
		}
		catch (std::bad_alloc&)
		{
			result = 3;
		}
		catch (...)
		{
			result = 2;
		}

		data->pFilesToConvert->lock();
		switch (result)
		{
		case 1:
			data->pFilesToConvert->m_convertedFiles.push_back(sourceInnerPathAndName);
			break;
		case 2:
			data->pFilesToConvert->m_failedFiles.push_back(sourceFullFileName);
			break;
		case 3:
			data->pFilesToConvert->m_outOfMemoryFiles.push_back(sourceInnerPathAndName);
			break;
		default:
			assert(0);
			break;
		}
		data->pFilesToConvert->unlock();
	}

	return 0;
}

void ResourceCompiler::EnsureDirectoriesPresent(const char *path)
{
	DWORD dwFileSpecAttr = GetFileAttributes (path);
	if (dwFileSpecAttr == 0xFFFFFFFF && *path)
	{
		EnsureDirectoriesPresent(PathHelpers::GetDirectory(PathHelpers::RemoveSeparator(path)).c_str());
		if(_mkdir(path))
			RCLog("Creating directory failed: %s", path);

//		RCLog("Creating directory %s (%s)", path, _mkdir(path) ? "failed" : "ok");
	}
}
	

// makes the relative path out of any
string NormalizePath(const char* szPath)
{
	char szCurDir[0x800]="";
	GetCurrentDirectory(sizeof(szCurDir),szCurDir);
	strcat(szCurDir, "/");

	char szFullPath[0x800];
	if (!_fullpath(szFullPath, szPath, sizeof(szFullPath)))
		strcpy (szFullPath, szPath);


	char* p, *q;
	string sRes = szPath;
	for (p = szCurDir, q = szFullPath; *p && *q; ++p, ++q)
	{
		if (tolower(*p)==tolower(*q))
			continue;

    if ((*p=='/'||*p=='\\')&&(*q=='/'||*q=='\\'))
			continue;

		return sRes;
	}

	if (*p)
		return szPath;

	return q; // return whatever has left after truncating the leading path
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::CompileFile(EPlatform platform, IConfig* config, const char* const sourceFullFileName, const char* const targetLeftPath, const char* const sourceInnerPath, ICompiler* compiler, IConvertor* convertor, std::vector<CFileStats>* fileStats)
{
	CmdLine cmdLine;

//	if (!RCPathFileExists(sourceFullFileName) && strstr(sourceFullFileName,".cba")==0)
//		return false;
// don't check for whether the file exists
//	bool fileExists = (0 != RCPathFileExists(sourceFullFileName));
//	if (!fileExists) // [MichaelS 28/4/2008] If files don't exist, look for the same path with a '.zip' extension.
//		fileExists = (0 != RCPathFileExists((string(sourceFullFileName) + ".zip").GetString()));
//	if (!fileExists)
//		return false;

	const string targetPath = PathHelpers::Join(targetLeftPath, sourceInnerPath);

	// get file extension.
	string ext = PathHelpers::FindExtension(sourceFullFileName);

	{
		string sOverwriteExtension;

		if(config->Get("overwriteextension",sOverwriteExtension))
		{
			ext = sOverwriteExtension;
		}
	}

	ext.MakeLower();

	// get key for special copy/ignore options to certain extensions
	{
		string extkey = "ext_";
		extkey += ext;
		string extcommand;
		if(config->Get(extkey.c_str(), extcommand))
		{
			if(extcommand=="ignore")
			{
				RCLog("Ignoring %s", sourceFullFileName);
				return false;
			}
				
			if(extcommand=="copy")
			{
				string targetFullFileName = targetPath;
				targetFullFileName += PathHelpers::GetFilename(sourceFullFileName);
				if(targetFullFileName != sourceFullFileName)
				{
					// TODO: can compare filestamps of source and destination to avoid copy, but maybe overkill
					RCLog("Copying %s to %s", sourceFullFileName, targetFullFileName.c_str());
					CopyFile(sourceFullFileName, targetFullFileName.c_str(), false); // overwrites any existing file, same as all converters
					FileUtil::SetFileTimes(targetFullFileName.c_str(), FileUtil::GetLastWriteFileTime(sourceFullFileName));
				}
				return true;
			}
		}
	}

	CfgFile CfgFile;				// file specific config file

	Config localConfig;

	localConfig.SetConfigKeyRegistry(this);

	localConfig.Merge(config);	// merge main config into local config

	// Setup conversion context.
	IConvertContext* pCC = convertor->CreateConvertContext();

	pCC->SetConfig(&localConfig);
	pCC->SetPlatform(platform);
	pCC->SetPlattformName(GetSectionName(platform));
	pCC->SetRC(this);
	pCC->SetThreads(m_maxThreads);
	pCC->SetFileStatsHandle(fileStats);

	pCC->SetSourceFileFinal(PathHelpers::GetFilename(sourceFullFileName));
	pCC->SetSourceFileFinalExtension(ext);
	pCC->SetSourceFolder(NormalizePath(PathHelpers::GetDirectory(sourceFullFileName)));

	const string outputFolder = NormalizePath(targetPath);
	pCC->SetOutputFolder(outputFolder);
	pCC->SetPresets(m_presets);
	pCC->SetQuiet(m_bQuiet);
	compiler->ConstructAndSetOutputFile(*(ConvertContext*)pCC);

	EnsureDirectoriesPresent(outputFolder.c_str());
	
	const bool bVerbose = config->HasKey("verbose");

	string filenameForUpToDateCheck;
	{
		char buff[MAX_PATH];
		buff[0] = 0;
		compiler->GetFilenameForUpToDateCheck(*(ConvertContext*)pCC, buff, sizeof(buff));
		filenameForUpToDateCheck = buff;
	}

	const FILETIME fileTimeSource = FileUtil::GetLastWriteFileTime(sourceFullFileName);

	if ((!filenameForUpToDateCheck.empty()) && FileUtil::FileTimeIsValid(fileTimeSource) && (!config->HasKey("refresh")))
	{
		const FILETIME fileTime = FileUtil::GetLastWriteFileTime(filenameForUpToDateCheck);

		if (FileUtil::FileTimesAreEqual(fileTimeSource, fileTime))
		{
			if (bVerbose)
			{
				RCLog("Skipping %s: file %s is up to date", sourceFullFileName, filenameForUpToDateCheck.c_str());
			}
			pCC->Release();
			return true;
		}												
	}

	RCLog("");
	RCLog("-------------------------------------------------------");
	if (bVerbose)
	{
		RCLog("Path='%s'", PathHelpers::RemoveSeparator(sourceInnerPath).c_str());
		RCLog("File='%s'", PathHelpers::GetFilename(sourceFullFileName).c_str());
	}
	else
	{
		const string sourceInnerPathAndName = PathHelpers::AddSeparator(sourceInnerPath) + PathHelpers::GetFilename(sourceFullFileName);
		RCLog("File='%s'", sourceInnerPathAndName.c_str());
	}

	OutputDebugString("Current file: '");
	OutputDebugString(sourceFullFileName);
	OutputDebugString("' ... ");

	// file name changed - print new header for warnings and errors
	SetHeaderLine(sourceFullFileName);

	// Start Recording Log messages from convertor.
	StartLogRecording();

	// compile file
	bool const bRet = compiler->Process( *(ConvertContext*)pCC );

	if (bRet && FileUtil::FileTimeIsValid(fileTimeSource) && !filenameForUpToDateCheck.empty())
	{
		FileUtil::SetFileTimes(filenameForUpToDateCheck, fileTimeSource);
	}

	// Stop Recording Log messages from convertor.
	StopLogRecording();

	// Only do if no MT
	if (GetMaxNumThreads() == 1)
	{
		m_LogForFileMap[sourceFullFileName] = m_RecordedLogLines;
	}

	OutputDebugString("processed\n");

	if (!bRet)
	{
		LogError("failed to convert file %s",sourceFullFileName);
	}

	// Release cloned config.
//	if (config != m_config)
//		config->Release();

	pCC->Release();

	return bRet;
}


void ResourceCompiler::SetHeaderLine( const char *inszLine )
{
	m_bWarningHeaderLine=false;
	m_bErrorHeaderLine=false;
	m_sHeaderLine=inszLine;
}

void ResourceCompiler::InitializeThreadIds()
{
	m_threadIdTLSIndex = TlsAlloc();
	if (m_threadIdTLSIndex == TLS_OUT_OF_INDEXES)
	{
		printf("RC Initialization error");
		exit( 100 );
	}
	TlsSetValue(m_threadIdTLSIndex, 0);
}

int ResourceCompiler::GetThreadId()
{
	const int* pThreadId = (const int*) TlsGetValue(m_threadIdTLSIndex);
	const int threadId = (pThreadId ? (*pThreadId) : -1);
	return threadId;
}

void ResourceCompiler::LogLine( const ELogType ineType, const char* szText )
{
	// Get the index of the thread.
	const int threadIndex = GetThreadId();

	static volatile int g_LogLock;
	WriteLock lock(g_LogLock);

	FILE* fLog = 0;
	if (!m_logFileName.empty())
	{
		fopen_s(&fLog, m_logFileName.c_str(), "a+t");
	}

	if (m_bQuiet)
	{
		if (fLog)
		{
			fprintf(fLog,"%s\n",szText);
			fflush(fLog);
			fclose(fLog);
			fLog = 0;
		}
		return;
	}

	char threadString[10];
	threadString[0] = 0;
	if (threadIndex > 0)
		sprintf(threadString, "%d> ", threadIndex);

	switch(ineType)
	{
	case eMessage:
		printf ("%s  ", threadString);							// to make it aligned with E: and W:
		break;
	case eWarning:
		printf ("W:%s ", threadString);							// for Warning
		if (!m_warningLogFileName.empty())
		{
			FILE* fWarningLog = 0;
			fopen_s(&fWarningLog, m_warningLogFileName.c_str(), "a+t");
			if (!m_bWarningHeaderLine)
			{
				fprintf(fWarningLog, "%s\r\n-----------------------------------------------------------------\r\n\r\n", threadString);
				fprintf(fWarningLog, "W: %s%s\r\n", threadString, m_sHeaderLine.c_str());
				m_bWarningHeaderLine = true;
			}
			fprintf(fWarningLog,"%s  %s\r\n", threadString, szText);
			fflush(fWarningLog);
			fclose(fWarningLog);
			fWarningLog = 0;
		}
		break;
	case eError:
		printf ("E:%s ", threadString);							// for Error
		if (!m_errorLogFileName.empty())
		{
			FILE* fErrorLog = 0;
			fopen_s(&fErrorLog, m_errorLogFileName.c_str(), "a+t");
			if(!m_bErrorHeaderLine)
			{
				fprintf(fErrorLog, "%s\r\n-----------------------------------------------------------------\r\n\r\n", threadString);
				fprintf(fErrorLog, "E: %s%s\r\n",threadString,m_sHeaderLine.c_str());
				m_bErrorHeaderLine = true;
			}
			fprintf(fErrorLog,"%s  %s\r\n", threadString, szText);
			fflush(fErrorLog);
			fclose(fErrorLog);
			fErrorLog = 0;
		}
		break;
	default:
		assert(0);
		break;
	}

	if (fLog)
	{
		const char* prefix = "  ";
		switch (ineType)
		{
		case eWarning: prefix = "W:"; break;
		case eError: prefix = "E:"; break;
		}
		fprintf(fLog, "%s %s%s\r\n", prefix, threadString, szText);
		fflush(fLog);
		fclose(fLog);
		fLog = 0;
	}

	if (m_bWarningsAsErrors && (ineType == eWarning || ineType == eError))
	{
		MessageBox( NULL,szText,"RC Compilation Error",MB_OK|MB_ICONERROR );
		exit( EXIT_FAILURE );
	}

	if (m_bLogRecordingEnabled)
	{
		m_RecordedLogLines.push_back( szText );
	}

	printf("%s\n",szText);
}


/*
//! Load and parse the Crytek Chunked File into the universal (very big) structure
//! The caller should then call Release on the structure to free the mem
//! @param filename Full filename including path to the file
CryChunkedFile* ResourceCompiler::LoadCryChunkedFile (const char* szFileName)
{
	CChunkFile_AutoPtr pReader = new CChunkFile ();
	if (!pReader->open (szFileName))
		return NULL;

	try
	{
		return new CryChunkedFile(pReader);
	}
	catch (CryChunkedFile::Error& e)
	{
		LogError("%s", e.strDesc.c_str());
		return NULL;
	}
	catch (...)
	{
		LogError("UNEXPECTED ERROR while trying to load Cry Chunked File \"%s\"", szFileName);
		return NULL;
	}
}
*/

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Print error message.
void MessageBoxError( const char *format,... )
{
	va_list	ArgList;
	char		szBuffer[1024];

	va_start(ArgList, format);
	vsprintf(szBuffer, format, ArgList);
	va_end(ArgList);

	string str = "####-ERROR-####: ";
	str += szBuffer;

//	printf( "%s\n",str );
	MessageBox( NULL,szBuffer,_T("Error"),MB_OK|MB_ICONERROR );
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogMultiLine( const char *szText )
{
	const char *p=szText;
	char szLine[80],*pLine=szLine;

	for(;;)
	{
		if(*p=='\n' || *p==0 || (pLine-szLine)>=sizeof(szLine)-(5+2+1)) // 5 spaces +2 (W: or E:) +1 to avoid nextline jump
		{
			*pLine=0;                                                     // zero termination
			RCLog("     %s",szLine);                                      // 5 spaces
			pLine=szLine;
			
			if(*p=='\n')
			{
				++p;
				continue;
			}
		}

		if(*p==0)
			return;

		*pLine++=*p++;

	} while(*p);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::show_help( const bool bDetailed )
{
	RCLog( "" );
	RCLog( "Usage: RC filespec /p=<platform> [/Key1=Value1] [/Key2=Value2] etc..." );

	if(bDetailed)
	{
		RCLog( "" );

		std::map<string,string>::const_iterator it, end=m_KeyHelp.end();

		for(it=m_KeyHelp.begin();it!=end;++it)
		{
			const string &rKey = it->first;
			const string &rHelp = it->second;

			RCLog("/%s",rKey.c_str());
			LogMultiLine(rHelp.c_str());
			RCLog( "" );
			RCLog( "" );
		}
	}
	else
	{
		RCLog( "       RC /help             // will list all usable keys with description" );
		RCLog( "       RC /help >file.txt   // help to file.txt" );
	}

	RCLog( "" );
}

//////////////////////////////////////////////////////////////////////////
static EPlatform GetPlatformFromName( const char *sPlatform )
{
	// Platform name to enum mapping.
	struct {
		const char *name;
		EPlatform platform;
	} platformNames[] =
	{
		{ "PC", ePlatform_PC },
//		{ "XBOX", ePlatform_XBOX },
//		{ "PS2", ePlatform_PS2 },
//		{ "GameCube", ePlatform_GAMECUBE },
//		{ "WII", ePlatform_WII },
		{ "PS3", ePlatform_PS3 },
		{ "X360", ePlatform_X360 }, 
	};
	for (int i = 0; i < sizeof(platformNames)/sizeof(platformNames[0]); i++)
	{
		if (stricmp(platformNames[i].name,sPlatform) == 0)
			return platformNames[i].platform;
	}
	return ePlatform_UNKNOWN;
}




//////////////////////////////////////////////////////////////////////////
void RegisterConvertors( IResourceCompiler *rc )
{
	string strDir;
	{
		char szRCPath[1000];
		if (GetModuleFileName (NULL, szRCPath, sizeof(szRCPath)))
			strDir = PathUtil::GetParentDirectory(szRCPath) + "\\";
	}
	__finddata64_t fd;
	int hSearch = _findfirst64 ((strDir +  "ResourceCompiler*.dll").c_str(), &fd);
	if (hSearch != -1)

	do {
		HMODULE hPlugin = LoadLibrary ((strDir+fd.name).c_str());
		if (!hPlugin)
		{
			RCLog("Error: Couldn't load plug-in module %s", fd.name);
			continue;
		}
		
		FnRegisterConvertors fnRegister = hPlugin?(FnRegisterConvertors)GetProcAddress(hPlugin, "RegisterConvertors"):NULL;
		if (!fnRegister)
		{
			RCLog("Error: plug-in module %s doesn't have RegisterConvertors function", fd.name);
			continue;
		}

		time_t nTime = GetTimestampForLoadedLibrary (hPlugin);
		char* szTime = "unknown";
		if (nTime)
		{
			szTime = asctime(localtime(&nTime));
			szTime[strlen(szTime)-1] = '\0';
		}
//		Info ("timestamp %s", szTime);
		RCLog("");
		RCLog("  Loading \"%s\"", fd.name);

		fnRegister (rc);
	}
	while(_findnext64(hSearch, &fd) != -1);

	_findclose(hSearch);
	RCLog("");
}

void RegisterCompressor( ResourceCompiler *rc )
{
	string strDir;
	{
		char szRCPath[1000];
		if (GetModuleFileName (NULL, szRCPath, sizeof(szRCPath)))
			strDir = PathUtil::GetParentDirectory(szRCPath) + "\\";
	}

	string compressorName = "CryCompressorRC.dll";

	HMODULE hPlugin = LoadLibrary (PathUtil::Make(strDir, compressorName).c_str());
	if (!hPlugin)
	{
		RCLogError("Couldn't load compressor plug-in module %s", compressorName.c_str());
		return;
	}

	FnCreateComressor fnRegister = hPlugin?(FnCreateComressor)GetProcAddress(hPlugin, "RegisterCompressor"):NULL;
	if (!fnRegister)
	{
		RCLogError("Compressor module %s doesn't have RegisterCompressor function", compressorName.c_str());
		return;
	}

	RCLog("CryCompressorRC dll loaded");

	rc->m_pCompressorRoutine = fnRegister (rc);
}

void ResourceCompiler::CreateSourceControl()
{
	assert(!m_bSourceControlCreated);

	m_bSourceControlCreated = true;
	m_pSourceControl = 0;

	string strDir;
	{
		char szRCPath[1000];
		if (GetModuleFileName (NULL, szRCPath, sizeof(szRCPath)))
		{
			strDir = PathUtil::GetParentDirectory(szRCPath) + "\\";
		}
	}

	const string dllName = "CryPerforce.dll";

	HMODULE hPlugin = LoadLibrary(PathUtil::Make(strDir, dllName).c_str());
	if (!hPlugin)
	{
		RCLogError("Couldn't load source control plug-in module %s", dllName.c_str());
		return;
	}

	m_fnCreateSourceControl =
		(hPlugin)
		? (FnCreateSourceControl)GetProcAddress(hPlugin, "CreateSourceControl")
		: NULL;

	m_fnDestroySourceControl =
		(hPlugin)
		? (FnDestroySourceControl)GetProcAddress(hPlugin, "DestroySourceControl")
		: NULL;

	if(!m_fnCreateSourceControl)
	{
		RCLogError("Source control module %s doesn't have CreateSourceControl function", dllName.c_str());
		return;
	}

	if(!m_fnDestroySourceControl)
	{
		RCLogError("Source control module %s doesn't have DestroySourceControl function", dllName.c_str());
		return;
	}

	RCLog("%s loaded", dllName.c_str());

	m_pSourceControl = m_fnCreateSourceControl();

	if(m_pSourceControl == 0)
	{
		RCLogWarning("Source control module %s failed to start (cannot connect to the server?)", dllName.c_str());
	}
}


void ResourceCompiler::LogFailedFileInfo( IConfig* config,int index, const char* fileName, bool bLogSourceControlInfo, const char* sourceControlClientName)
{
	assert(fileName);
	assert(fileName[0]);

	if(m_bSourceControlCreated && m_pSourceControl)
	{
		RCLog("  ConvertError #%i", index);

		RCLog("    FileName:       %s", fileName);

		CSourceControlFileInfo fi;
		CSourceControlUserInfo ui;
		CSourceControlTimeInfo ti;

		if(!m_pSourceControl->GetLastCheckOutInfo(fileName, sourceControlClientName, fi, ui, ti))
		{
			RCLog("    SourceControl:  Failed to obtain file and/or user info!");
		}
		else
		{
			char strTime[100];
			ti.GetAsStr(strTime, sizeof(strTime));

			// Removing trailing '\r' and '\n' characters from time string
			unsigned int len = strlen(strTime);
			while((len > 0) && ((strTime[len-1]=='\r')||(strTime[len-1]=='\n')))
			{
				strTime[--len] = 0;
			}

			RCLog("    ClientFileName: %s", fi.m_clientFileName);
			RCLog("    DepotFileName:  %s", fi.m_depotFileName);
			RCLog("    User:           %s", ui.m_userName);
			RCLog("    Client:         %s", ui.m_clientName);
			RCLog("    UserFullName:   %s", ui.m_userFullName);
			RCLog("    UserMail:       %s", ui.m_email);
			RCLog("    CheckInTime:    %s", strTime);
			RCLog("    Error:          %s", "UNKNOWN");	// FIXME: use real error message


			bool bMailErrors = config->GetAs<bool>("MailErrors", false);
			if (bMailErrors)
			{
				string mailBody;

#if defined(_MSC_VER)
				{
					char compName[256];
					DWORD size = ARRAYSIZE(compName);

					typedef BOOL (WINAPI *FP_GetComputerNameExA)(COMPUTER_NAME_FORMAT, LPSTR, LPDWORD);
					FP_GetComputerNameExA pGetComputerNameExA = (FP_GetComputerNameExA) GetProcAddress(LoadLibrary("kernel32.dll"), "GetComputerNameExA");

					if (pGetComputerNameExA)
						pGetComputerNameExA(ComputerNamePhysicalDnsHostname, compName, &size);
					else
						GetComputerName(compName, &size);

					mailBody += string("Report sent from ") + compName + "...\n\n";
				}
#endif

				mailBody += string("FileName: ") + fileName + "\r\n";

				mailBody += string("ClientFileName: ") + fi.m_clientFileName + "\r\n";
				mailBody += string("DepotFileName:  ") + fi.m_depotFileName+ "\r\n";
				mailBody += string("User:           ") + ui.m_userName+ "\r\n";
				mailBody += string("Client:         ") + ui.m_clientName+ "\r\n";
				mailBody += string("UserFullName:   ") + ui.m_userFullName+ "\r\n";
				mailBody += string("UserMail:       ") + ui.m_email+ "\r\n";
				mailBody += string("CheckInTime:    ") + strTime+ "\r\n";
				mailBody += string("Compile Errors: ") + "\r\n";

				// Add log output from this file.
				LogToFileMap::const_iterator it = m_LogForFileMap.find( fileName );
				if (it != m_LogForFileMap.end())
				{
					const std::vector<string> &logs = it->second;
					for (int i = 0; i < (int)logs.size(); i++)
					{
						mailBody += string("                ") + logs[i] + "\n\n";
					}
				}

				//Attachment.resize(0);

				CSMTPMailer::tstrcol Rcpt;
				CSMTPMailer::tstrcol cc;
				CSMTPMailer::tstrcol bcc;
				CSMTPMailer::tstrcol attachments;

				Rcpt.push_back( ui.m_email );

				string cc_email;
				if (config->Get("cc_email", cc_email))
				{
					int curPos= 0;
					string resToken= cc_email.Tokenize(";,",curPos);
					while (!resToken.empty())
					{
						cc.push_back( resToken );
						resToken= cc_email.Tokenize(";,",curPos);
					};
				}

				string smtp_server = "mail.intern.crytek.de";
				if (config->Get("MailServer", smtp_server))
				{
					CSMTPMailer mail("", "", smtp_server);
					string mailSubject = string("[RC Validator] Asset Compile Failed") + ": " + fi.m_depotFileName;
					bool res = mail.Send("RC-noreply@crytek.de", Rcpt, cc, bcc, mailSubject, mailBody, attachments);
					if (res)
					{
						RCLog("  Mail to %s sent", ui.m_email);
					}
					else
					{
						RCLogWarning("  Failed to send Mail to %s", ui.m_email);
					}
				}
			}
		}

		RCLog("  EndConvertError #%i", index);
	}
	else
	{
		RCLog("  %s", fileName);
	}
}


static CrashHandler s_crashHandler(RC_FILENAME_LOG, RC_FILENAME_ERRORS, RC_FILENAME_CRASH_DUMP);

//////////////////////////////////////////////////////////////////////////
int __cdecl main(int argc, char **argv, char **envp)
{
	/*
	int tmpDbgFlag;
	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	// Clear the upper 16 bits and OR in the desired freqency
	tmpDbgFlag = (tmpDbgFlag & 0x0000FFFF) | (32768 << 16);
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpDbgFlag);

	// Check heap every 
	//_CrtSetBreakAlloc(2031);
	*/

//	EnableFloatingPointExceptions();

	ResourceCompiler rc;
	Config mainConfig;
	mainConfig.SetConfigKeyRegistry(&rc);
	CmdLine cmdLine;

	rc.QueryVersionInfo();
	SFileVersion fv = rc.GetFileVersion();

	rc.RegisterKey("wait","wait for key after running the application");
	rc.RegisterKey("WX","pause and display message box in case of warning or error");
	rc.RegisterKey("recursive","traverse input directory with sub folders");
	rc.RegisterKey("refresh","force recompilation of resources with up to date timestamp");
	rc.RegisterKey("p","to specify platform (PC,XBOX,PS2,GC,X360,PS3)");
	rc.RegisterKey("statistics","log statistics to rc_stats_* files");
	rc.RegisterKey("verbose","to produce detailed printouts");
	rc.RegisterKey("quiet","to suppress all printouts");
	rc.RegisterKey("logfiles","to suppress generating log file rc_log.log");
	rc.RegisterKey("presetcfg","to define the path to the presets e.g. \"rc_presets_pc.ini\"");
	rc.RegisterKey("targetroot","to define the destination folder");
	rc.RegisterKey("threads","=N to use N threads (only supported for TIFF compilation)");
	rc.RegisterKey("failonwarnings","return error code if warnings are encountered");
	rc.RegisterKey("sourcecontrol","output source control information for failed files");
	rc.RegisterKey("help","lists all usable keys of the ResourceCompiler with description");
	
	rc.RegisterKey("listfile","Specify List file, List file can contain file lists from zip files like: @Levels\\Test\\level.pak|resourcelist.txt");
	rc.RegisterKey("copyonly","Only copy source files to target root without processing");
	rc.RegisterKey("CopySourceFolder","Source folder for the copy operation");
	rc.RegisterKey("name_as_crc32","When creating Pak File outputs target filename as the CRC32 code without the extension");
	rc.RegisterKey("CreatePakFile","Pak source files into the zip file specified with this parameter");

	rc.RegisterKey("validate","When specified RC is running in a resource validation mode");
	rc.RegisterKey("MailServer","SMTP Mail server used when RC needs to send an e-mail");
	rc.RegisterKey("MailErrors","0=off 1=on When enabled sends an email to the user who checked in asset that failed validation");
	rc.RegisterKey("cc_email","When sending mail this address will be added to CC, semicolon separates multiple addresses");
	rc.RegisterKey("job","Process a job xml file");

	char moduleName[_MAX_PATH];
	GetModuleFileName( NULL, moduleName, _MAX_PATH );//retrieves the PATH for the current module

	// Load main config.
	CfgFile cfgFile;
	if (!cfgFile.Load( string(PathUtil::Make(PathUtil::GetPath(moduleName),RC_INI_FILE))) )
	{
		char str[512];

		sprintf(str,"Resource compiler ini file ('%s') is missing - check current working folder",RC_INI_FILE);

		RCLog("%s",str);

		MessageBox(0,str,"ResourceCompiler Error",MB_OK|MB_ICONERROR);
		return 1;
	}

	cfgFile.SetConfig( eCP_PriorityPlatform,COMMON_SECTION,&mainConfig );

	// Parse command line.
	cmdLine.Parse( argc,argv,&mainConfig );

	bool bWork=true;

	string platformStr;
	if (!((IConfig &)mainConfig).Get( "p", platformStr ))
	{
		// Platform switch not specified.
		RCLog("Platform (/p) not specified, defaulting to PC.");
		RCLog("");
		platformStr = "PC";
		mainConfig.Set(eCP_PriorityCmdline,"p",platformStr.c_str());
	}
	
	// Detect platform.
	EPlatform platform = GetPlatformFromName(platformStr.c_str());
	if (platform == ePlatform_UNKNOWN)
	{
		char str[512];

		sprintf(str,"Unknown platform %s specified",platformStr.c_str());

		RCLog("%s",str);

		MessageBox(0,str,"ResourceCompiler Error",MB_OK|MB_ICONERROR);
		return 1;
	}

	rc.GetHWnd();

	// Load configuration from per platform section. 
//	RCLog("Using platform settings '%s'",rc.GetSectionName(platform));
	cfgFile.SetConfig( eCP_PriorityPlatform, rc.GetSectionName(platform),&mainConfig );

	// initialize rc
	rc.Init( &mainConfig );

	RCLog("ResourceCompiler Version %d.%d.%d %s %s",fv.v[2],fv.v[1],fv.v[0], __DATE__, __TIME__ );
	RCLog("================");

	RCLog("Copyright(c) Crytek 2001-2010, All Rights Reserved." );
#ifdef _WIN64
	RCLog("64-bit edition");
#endif

	RCLog("");
	if(argc>1)
	{
		RCLog("CommandLine:");
		for(int i=1;i<argc;++i)
		{
			RCLog("  '%s'",argv[i]);
		}
		RCLog("");
	}

	RCLog("Registering sub compilers (ResourceCompiler*.dll)");

	RegisterConvertors( &rc );

	RegisterCompressor( &rc );

	if(!mainConfig.CheckForUnknownKeys())
	{
		RCLogWarning("Unknown command-line option(s) (see above). Use \"RC /help\".");
		if(mainConfig.GetAs<bool>("failonwarnings", false))
		{
			return 1;
		}
	}

	if(cmdLine.m_fileSpec.empty())				// e.g. "path\rc.exe /help" or "path\rc.exe"
		bWork=false;

	if(bWork)
	{
		rc.Compile(platform, &mainConfig, cmdLine.m_fileSpec.c_str());

		rc.PostBuild();		// e.g. print material dependencies
	}

	if (mainConfig.HasKey("job"))
	{
		bWork = true;
		rc.ProcessJobFile(platform, &mainConfig);
	}

	rc.DeInit();		// to clean up before waiting for user

	//rc.SortLogFiles();

	if(!rc.m_bQuiet)
	if(!bWork)
		rc.show_help(false);

	if(mainConfig.HasKey("help"))
		rc.show_help(true);

	if (rc.GetNumErrors() || rc.GetNumWarnings())
		RCLog("%d errors, %d warnings.", rc.GetNumErrors(), rc.GetNumWarnings());

	if(mainConfig.HasKey("wait"))
	{
		RCLog("");     
		RCLog("                                              <RETURN>  (/wait was specified)");            // right aligned on 80 char screen
		getchar();
	};

	bool successful = true;
	if (rc.GetNumErrors())
		successful = false;
	if (rc.GetNumWarnings() && mainConfig.GetAs<bool>("failonwarnings", false))
		successful = false;

	return (successful ? 0 : 1);
}

//! Returns the main application window
HWND ResourceCompiler::GetHWnd()
{
	HMODULE hKernel32 = LoadLibrary ("kernel32.dll");
	HWND hResult = GetDesktopWindow();
	if (hKernel32)
	{
		//typedef WINBASEAPI  HWND APIENTRY (*FnGetConsoleWindow )(VOID);
		typedef HWND (APIENTRY*FnGetConsoleWindow )(VOID);
		FnGetConsoleWindow GetConsoleWindow = (FnGetConsoleWindow)GetProcAddress (hKernel32, "GetConsoleWindow");
		if (GetConsoleWindow)
		{
			hResult = GetConsoleWindow();
		}
		FreeLibrary (hKernel32);
	}
	return hResult;
}

HWND ResourceCompiler::GetEmptyWindow()
{
	if (!m_hEmptyWindow)
	{
		const char szClassName[] = "DirectXWnd";
		WNDCLASS wc;
		memset (&wc, 0, sizeof(wc));
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = DefWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance  = (HINSTANCE)GetModuleHandle (NULL);
		wc.lpszClassName = szClassName;

		ATOM atomWndClass = RegisterClass (&wc);
		m_hEmptyWindow = CreateWindow (szClassName, "DirectXEmpty", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,CW_USEDEFAULT,256,256,NULL, NULL, wc.hInstance, 0);
	}
	return m_hEmptyWindow;
}

// ------------------------------------------------------
void ResourceCompiler::AddDependencyMaterial( const char *inszSrcFilename, const char *inszMatName, const char *inszScriptName )
{
	if (!m_bStatistics)
		return;
	CMatDep dep;

	dep.m_sMatName=inszMatName;
	dep.m_sScriptName=inszScriptName;

	m_MaterialDependencies.insert( CMatDepPair(dep,inszSrcFilename) );

//	RCLog("  DepMat: <%s> <%s>",inszMatName,inszScriptName);
}




// ------------------------------------------------------
void ResourceCompiler::AddDependencyFile( const char *inszSrcFilename, const char *inszPathFileName )
{
	if (!m_bStatistics)
		return;
	m_FileDependencies.insert( CFileDepPair(inszPathFileName,inszSrcFilename) );

//	RCLog("  DepFile: <%s>",inszPathFileName);
}

// ------------------------------------------------------
void ResourceCompiler::ShowFileDependencies()
{
	const char *szFileName=RC_FILENAME_FILEDEP;

	FILE *out=fopen(m_exePath+szFileName,"wb");

	if(!out)
	{
		LogError("unable to open %s - file it not updated",szFileName);
		return;
	}

	CFileDepMap::iterator it;

	string sLastDep="";		// for a nice printout
	bool bFirst=true;

	for(it=m_FileDependencies.begin(); it!=m_FileDependencies.end(); ++it)
	{
		const string &rsDepFile = it->first;
		const string &rsSrcFile = it->second;

		if(bFirst || rsDepFile!=sLastDep)
		{
			fprintf(out,"\r\n");
			fprintf(out,"'%s'\r\n",rsDepFile.c_str());
			sLastDep=rsDepFile;
			bFirst=false;
		}

		fprintf(out,"    used by: '%s'\r\n",rsSrcFile.c_str());
	}

	fprintf(out,"\r\n");
	fprintf(out,"------------------------------------------------------------------------------\r\n");
	fprintf(out,"\r\n");
	fprintf(out,"all used files:\r\n");
	fprintf(out,"\r\n");

	bFirst=true;
	for(it=m_FileDependencies.begin(); it!=m_FileDependencies.end(); ++it)
	{
		const string &rsDepFile = it->first;
		const string &rsSrcFile = it->second;

		if(bFirst || rsDepFile!=sLastDep)
		{
			fprintf(out,"  '%s'\r\n",rsDepFile.c_str());
			sLastDep=rsDepFile;
			bFirst=false;
		}
	}

	fclose(out);
}


// ------------------------------------------------------
void ResourceCompiler::ShowPresetUsage()
{
	const char *szFileName=RC_FILENAME_PRESETUSAGE;

	FILE *out=fopen(m_exePath+szFileName,"wb");

	if(!out)
	{
		LogError("unable to open %s - file it not updated",szFileName);
		return;
	}

	std::vector<std::pair<string, int> > presetSortedMappingTable(m_Files.size());
	for (int fileIndex = 0, fileCount = int(m_Files.size()); fileIndex < fileCount; ++fileIndex)
		presetSortedMappingTable[fileIndex] = std::make_pair(m_Files[fileIndex].m_sPreset, fileIndex);
	std::sort(presetSortedMappingTable.begin(), presetSortedMappingTable.end());

	string sLastDep="";		// for a nice printout
	bool bFirst=true;

	fprintf(out,"preset usage:\r\n");
	fprintf(out,"\r\n");

	//for(it=m_PresetUsage.begin(); it!=m_PresetUsage.end(); ++it)
	for (int mappingTableIndex = 0, mappingTableSize = int(presetSortedMappingTable.size()); mappingTableIndex < mappingTableSize; ++mappingTableIndex)
	{
		const uint32 dwFileStatsIndex = presetSortedMappingTable[mappingTableIndex].second;

		CFileStats &stats = m_Files[dwFileStatsIndex];

		if(bFirst || stats.m_sPreset!=sLastDep)
		{
			fprintf(out,"\r\n  PRESET %s:\r\n",stats.m_sPreset.c_str());
			sLastDep=stats.m_sPreset;
			bFirst=false;
		}

		fprintf(out,"    %3d%% '%s' %d KB -> %d KB \r\n",(stats.m_DstFileSizeKB*100)/stats.m_SrcFileSizeKB,stats.m_sDestFilename.c_str(),
			stats.m_SrcFileSizeKB,stats.m_DstFileSizeKB);
	}

	fclose(out);
}






// ------------------------------------------------------
void ResourceCompiler::ShowXLSFileSizes()
{
	const char *szFileName=RC_FILENAME_XLS_FILESIZES;

	FILE *out=fopen(m_exePath+szFileName,"wb");

	if(!out)
	{
		LogError("unable to open %s - file it not updated",szFileName);
		return;
	}

	fprintf(out,"DestFileSizeInKB"
							"\tDestFileName"
							"\tPreset"
							"\tWidth"
							"\tHeight"
							"\tAlpha"
							"\tMips"
							"\tMemInKB(in Memory)"
							"\tFormat"
							"\tReduce"
							"\tAttachedMemInKB(e.g. Alpha of 3dC)"
							"\tDestFilePath"
							"\r\n");
	fprintf(out,"----------------------------------------------------------------------------------------------<in Excel friendly format>\r\n");

	// Produce a mapping table into the file stats array that is sorted by file size.
	std::vector<std::pair<uint32, uint32> > fileSizeSortedMappingTable(m_Files.size());
	for (int fileIndex = 0, fileCount = int(m_Files.size()); fileIndex < fileCount; ++fileIndex)
		fileSizeSortedMappingTable[fileIndex] = std::make_pair(m_Files[fileIndex].m_DstFileSizeKB, fileIndex);
	std::sort(fileSizeSortedMappingTable.begin(), fileSizeSortedMappingTable.end());

	for (int mappingTableIndex = 0, mappingTableSize = fileSizeSortedMappingTable.size(); mappingTableIndex < mappingTableSize; ++mappingTableIndex)
	{
		const uint32 dwFileStatsIndex = fileSizeSortedMappingTable[mappingTableIndex].second;

		CFileStats &stats = m_Files[dwFileStatsIndex];

		fprintf(out,"%d"					// DestFileSizeInKB
								"\t%s"				// DestFileName
								"\t%s"				// preset
								"\t%s"				// info
								"\t%s\r\n",		// DestPath
			stats.m_DstFileSizeKB,
			PathHelpers::GetFilename(stats.m_sDestFilename).c_str(),
			stats.m_sPreset,
			stats.m_sInfo,
			PathHelpers::GetDirectory(stats.m_sDestFilename).c_str());
	}

	fclose(out);
}

/*
// ------------------------------------------------------
void ResourceCompiler::ShowMaterialDependencies()
{
	const char *szFileName=RC_FILENAME_MATDEP;

	FILE *out=fopen(m_exePath+szFileName,"wb");

	if(!out)
	{
		LogError("unable to open %s - file it not updated",szFileName);
		return;
	}

	CMatDepMap::iterator it;

	CMatDep LastDep;		// for a nice printout
	bool bFirst=true;

	// max info
	for(it=m_MaterialDependencies.begin(); it!=m_MaterialDependencies.end(); ++it)
	{
		const CMatDep &rsMatDep = it->first;
		const string &rsSrcFile = it->second;

		if(bFirst || !(rsMatDep==LastDep))
		{
			fprintf(out,"\r\n");
			fprintf(out,"scriptmaterial='%s' materialname='%s'\r\n",rsMatDep.m_sScriptName.c_str(),rsMatDep.m_sMatName.c_str());
			LastDep=rsMatDep; 
			bFirst=false;
		}

		fprintf(out,"    used by: '%s'\r\n",rsSrcFile.c_str());
	}

	fprintf(out,"\r\n");
	fprintf(out,"------------------------------------------------------------------------------\r\n");
	fprintf(out,"\r\n");
	fprintf(out,"all used scriptmaterials:\r\n");
	fprintf(out,"\r\n");

	// only the used scripsmaterials
	bFirst=true;
	for(it=m_MaterialDependencies.begin(); it!=m_MaterialDependencies.end(); ++it)
	{
		const CMatDep &rsMatDep = it->first;
		const string &rsSrcFile = it->second;

		if(bFirst || !(rsMatDep.m_sScriptName==LastDep.m_sScriptName))
		{
			fprintf(out,"  '%s'\r\n",rsMatDep.m_sScriptName.c_str());
			LastDep=rsMatDep; 
			bFirst=false;
		}
	}

	fclose(out);
}
*/

// ------------------------------------------------------
void ResourceCompiler::PostBuild()
{
	if (m_bStatistics)
	{
		RCLog("writing statistics files (rc_stats_...) ");
		RCLog("");

		ShowFileDependencies();
//		ShowMaterialDependencies();
		ShowPresetUsage();
		ShowXLSFileSizes();
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::QueryVersionInfo()
{
	char moduleName[_MAX_PATH];
	DWORD dwHandle;
	UINT len;

	char ver[1024*8];

	GetModuleFileName( NULL, moduleName, _MAX_PATH );//retrieves the PATH for the current module
	m_exePath = PathHelpers::AddSeparator( PathHelpers::GetDirectory(moduleName) );

	int verSize = GetFileVersionInfoSize( moduleName,&dwHandle );
	if (verSize > 0)
	{
		GetFileVersionInfo( moduleName,dwHandle,1024*8,ver );
		VS_FIXEDFILEINFO *vinfo;
		VerQueryValue( ver,"\\",(void**)&vinfo,&len );

		m_fileVersion.v[0] = vinfo->dwFileVersionLS & 0xFFFF;
		m_fileVersion.v[1] = vinfo->dwFileVersionLS >> 16;
		m_fileVersion.v[2] = vinfo->dwFileVersionMS & 0xFFFF;
		m_fileVersion.v[3] = vinfo->dwFileVersionMS >> 16;

		m_productVersion.v[0] = vinfo->dwProductVersionLS & 0xFFFF;
		m_productVersion.v[1] = vinfo->dwProductVersionLS >> 16;
		m_productVersion.v[2] = vinfo->dwProductVersionMS & 0xFFFF;
		m_productVersion.v[3] = vinfo->dwProductVersionMS >> 16;
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::DeInit()
{
	m_extensionManager.UnregisterAll();
}
//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::Init( IConfig* config )
{
	{
		unsigned int numThreadsAvailableToSystem = 0;
		unsigned int numThreadsAvailableToProcess = 0;
		GetNumCPUCores(numThreadsAvailableToSystem, numThreadsAvailableToProcess);
		m_maxThreads = numThreadsAvailableToProcess;
		if(m_maxThreads < 1)
		{
			m_maxThreads = 1;
		}
	}
	
	bool bValidateMode = config->GetAs<bool>("validate", false);

	string threadString;
	if (!config->Get("threads", threadString) || bValidateMode)
	{
		m_maxThreads = 1;
	}
	else
	{
		char* endptr;
		const char* str = threadString.c_str();
		int threadCount = strtol(str, &endptr, 10);

		if (endptr == str) 
		{
			RCLog("/threads specified, but number of threads not given. Using %d thread%s.", m_maxThreads, ((m_maxThreads>1)?"s":""));
			threadCount = m_maxThreads;
		}
		else if (threadCount < 1)
		{
			RCLog("[Warning]: %d threads specified. Using single thread.", threadCount);
			threadCount = 1;
		}
		else if (threadCount > m_maxThreads)
		{
			RCLog("[Warning]: %d threads specified. Clamping to %d thread%s.", threadCount, m_maxThreads, ((m_maxThreads>1)?"s":""));
			threadCount = m_maxThreads;
		}

		m_maxThreads = threadCount;
	}

	m_bQuiet = config->HasKey("quiet");

	m_bWarningsAsErrors = config->HasKey("WX");

	InitLogs(config);
	SetRCLog(this);
}
//////////////////////////////////////////////////////////////////////////
ICfgFile *ResourceCompiler::CreateCfgFile()
{
	return new CfgFile;
}


//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::VerifyKeyRegistration( const char *szKey )
{
	assert(szKey);
	string sKey = szKey;
	sKey.MakeLower();

	const bool ok = (m_KeyHelp.count(sKey) != 0);

	if(!ok)
	{
		RCLogWarning("Key '%s' was not registered, call RegisterKey() before using the key",szKey);
	}
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::VerifyKeyRegistration2( const char *szKey )
{
	assert(szKey);
	string sKey = szKey;
	sKey.MakeLower();

	const bool ok = (m_KeyHelp.count(sKey) != 0);

	if(!ok)
	{
		RCLogWarning("Key '%s' is unknown.",szKey);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RegisterKey( const char *key, const char *helptxt )
{
	string sKey = key;

	sKey.MakeLower();

	assert(m_KeyHelp.count(sKey)==0);		// registered twice

	m_KeyHelp[sKey] = helptxt;
}


//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::SortLogFileByThread(FILE* file)
{
	fseek(file, 0, SEEK_SET);

	// Sort all the lines into a vector for each thread.
	typedef std::vector<std::vector<string> > StringsByThread;
	StringsByThread stringsByThread;
	while (!feof(file))
	{
		char lineBuffer[1024];
		fgets(lineBuffer, sizeof(lineBuffer), file);

		// Check the start of the line to see whether it is prefixed with the thread index.
		// Threads are written in this form: 3> (message text).
		int threadIndex = 0, threadIndexLength = 0;
		{
			int pos;
			const int MAX_THREAD_LENGTH = 5;
			for (pos = 0; lineBuffer[pos] >= '0' && lineBuffer[pos] <= '9' && pos < MAX_THREAD_LENGTH; ++pos);
			threadIndexLength = (lineBuffer[pos] == '>' ? pos : 0);
			if (threadIndexLength > 0)
			{
				threadIndex = strtol(lineBuffer, 0, 10);
				threadIndexLength += 2; // Skip the '> '
			}
		}

		// Sanity check for thread index.
		const int THREAD_COUNT_SANITY_CHECK = 128;
		if (threadIndex > THREAD_COUNT_SANITY_CHECK)
			threadIndex = THREAD_COUNT_SANITY_CHECK;

		if (int(stringsByThread.size()) < threadIndex + 1)
			stringsByThread.resize(threadIndex + 1);

		stringsByThread[threadIndex].push_back(lineBuffer + threadIndexLength);
	}

	// Output all the lines.
	fseek(file, 0, SEEK_SET);
	for (int threadIndex = 0, threadCount = int(stringsByThread.size()); threadIndex < threadCount; ++threadIndex)
	{
		std::vector<string>& lines = stringsByThread[threadIndex];
		for (int lineIndex = 0, lineCount = int(lines.size()); lineIndex < lineCount; ++lineIndex)
			fprintf(file, "%s", lines[lineIndex].c_str());
	}
}


void ResourceCompiler::InitLogs(IConfig *config)
{
	DeleteFile(m_exePath+RC_FILENAME_LOG);
	DeleteFile(m_exePath+RC_FILENAME_WARNINGS);
	DeleteFile(m_exePath+RC_FILENAME_ERRORS);

	m_logFileName = "";
	if(!config->HasKey("logfiles"))
	{
		m_logFileName = m_exePath+RC_FILENAME_LOG;
	}

	m_warningLogFileName = m_exePath+RC_FILENAME_WARNINGS;

	m_errorLogFileName = m_exePath+RC_FILENAME_ERRORS;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::SortLogFiles()
{
	{
		FILE* f = fopen(m_exePath+RC_FILENAME_LOG, "wb+");
		if (f)
		{
			SortLogFileByThread(f);
			fclose(f);
		}
	}
	{
		FILE* f = fopen(m_exePath+RC_FILENAME_WARNINGS, "wb+");
		if (f)
		{
			SortLogFileByThread(f);
			fclose(f);
		}
	}
	{
		FILE* f = fopen(m_exePath+RC_FILENAME_ERRORS, "wb+");
		if (f)
		{
			SortLogFileByThread(f);
			fclose(f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::AddFileStats(void* fileStatsHandle, const char *inszPathFileName, const size_t SrcFileSize, const size_t DstFileSize, const char *inszPreset, const char *inszInfo)
{
	std::vector<CFileStats>* fileStats = static_cast<std::vector<CFileStats>*>(fileStatsHandle);

	CFileStats value;

	value.m_SrcFileSizeKB=(SrcFileSize+1023)/1024;
	value.m_DstFileSizeKB=(DstFileSize+1023)/1024;
	value.m_sDestFilename=inszPathFileName;

	if(inszPreset)
		value.m_sPreset=inszPreset;

	if(inszInfo)
		value.m_sInfo=inszInfo;

	uint32 dwFileStatsIndex = (uint32)fileStats->size();

	fileStats->push_back(value);
}

int ResourceCompiler::GetMaxNumThreads() const
{
	return m_maxThreads;
}

void ResourceCompiler::Update()
{
}

void ResourceCompiler::LogV( const ELogType ineType, const char* szFormat, va_list args )
{
	if(ineType == eWarning)
	{
		++m_numWarnings;
	}
	else if(ineType == eError)
	{
		++m_numErrors;
	}

	char str[16*1024],*p=str;

	vsnprintf(str, sizeof(str), szFormat, args);

	bool bRun=true;

	while(bRun)
	{
		char *start=p;

		// search for end marker
		while(*p!=0)
		{
			// remove nonprintable characters except newlines and tabs
			if( (*p<' ') && (*p!='\n') && (*p!='\t') ) 
				*p=' ';  

			p++;
		}

		if(*p==0)
			bRun=false;

		*p=0;

		LogLine(ineType,start);

		p++;	// jump over end marker
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::StartLogRecording()
{
	if (GetMaxNumThreads() == 1)
	{
		m_bLogRecordingEnabled = true;
		m_RecordedLogLines.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::StopLogRecording()
{
	m_bLogRecordingEnabled = false;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::CopyFilesToTargetFolder( IConfig *config,const std::vector<string> &files,const string &inTargetFolder )
{
	const bool bVerbose = config->HasKey("verbose");

	bool bOverwriteExtension = false;
	string sOverwriteExtension;
	if(config->Get("overwriteextension",sOverwriteExtension))
	{
		bOverwriteExtension = true;
	}

	string sCopySourceFolder;
	if(config->Get("CopySourceFolder",sCopySourceFolder))
	{
		sCopySourceFolder = PathHelpers::AddSeparator(sCopySourceFolder);
	}

	size_t numFiles = files.size();
	RCLog("Starting to copy %d files to %s", numFiles,inTargetFolder.c_str() );
	string targetFolder = PathHelpers::AddSeparator(inTargetFolder);

	for (int i = 0; i < numFiles; i++)
	{
		string srcFilename = files[i];
		string trgFilename = targetFolder + srcFilename;

		if (bOverwriteExtension)
		{
			srcFilename = PathHelpers::ReplaceExtension(srcFilename,sOverwriteExtension);
			trgFilename = PathHelpers::ReplaceExtension(trgFilename,sOverwriteExtension);
		}

		if (!sCopySourceFolder.empty())
		{
			srcFilename = sCopySourceFolder + srcFilename;
		}

		if (bVerbose)
			RCLog("Copying %s to %s", srcFilename.c_str(), trgFilename.c_str());
		
		// check if source file exist.
		if (-1 != GetFileAttributes( srcFilename.c_str() ))
		{
			FileUtil::CreateDirectoryRecursive( PathHelpers::GetDirectory(trgFilename) );
		}
		if (0 == ::CopyFile( srcFilename,trgFilename,FALSE ))
		{
			if (bVerbose)
			{
				RCLog("Failed to Copy %s to %s", srcFilename.c_str(), trgFilename.c_str());
			}
		}
	}
	RCLog("Finished copying %d files to %s", numFiles,inTargetFolder.c_str() );
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::CreatePakFile( IConfig *config,std::vector<string> files,const string &inSourceFolder,const string &pakFilename,bool bUpdate )
{
	bool bResult = true;
	RCLog("Packing folder %s to zip file %s", inSourceFolder.c_str(),pakFilename.c_str() );
	
	string sourceFolder = PathHelpers::AddSeparator(inSourceFolder);
	string pakFolder = PathHelpers::AddSeparator(PathHelpers::GetDirectory(pakFilename));

	Crc32Gen crc32generator;
	std::set<int> crc32set;
	bool name_as_crc32 = false;
	config->Get("name_as_crc32",name_as_crc32);

	// Make all filenames lowcase.
	for (size_t i = 0; i < files.size(); i++)
	{
		files[i].MakeLower();
	}
	// Soft all files alphabetically
	std::sort( files.begin(),files.end() );
	
	if (!bUpdate)
	{
		// Delete old pak file.
		::SetFileAttributes( pakFilename.c_str(),FILE_ATTRIBUTE_ARCHIVE );
		::DeleteFile( pakFilename.c_str() );
	}

	std::vector<char> buffer;

	int nNumAddedFiles = 0;
	// Add them to pak file.
	PakSystemArchive *pPakFile = GetPakSystem()->OpenArchive( pakFilename.c_str() );

	// Add files to Pak
	for (size_t i = 0; i < files.size(); i++)
	{
		int nFileSize = 0;
		string sRealFilename = sourceFolder + files[i];
		FILE *f = fopen(sRealFilename.c_str(),"rb");
		if (f)
		{
			fseek(f,0,SEEK_END);
			nFileSize = ftell(f);
			buffer.resize(nFileSize+1);
			fseek(f,0,SEEK_SET);
			fread( &buffer[0],1,nFileSize,f );
			fclose(f);

			string fileNameInPak = files[i];
			if (name_as_crc32)
			{
				string outFilename = files[i];
				outFilename.MakeLower();
				unsigned int crc32 = crc32generator.GetCRC32Lowercase(outFilename.c_str());
				fileNameInPak.Format( "%X",crc32 );
				if (crc32set.find(crc32) != crc32set.end())
				{
					RCLogError( "Duplicate CRC32 code for file: %s when creating Pak File: %s",outFilename.c_str(),pakFilename.c_str() );
					bResult = false;
					break;
				}
				crc32set.insert(crc32);
			}

			GetPakSystem()->AddToArchive( pPakFile,fileNameInPak.c_str(),& buffer[0],nFileSize );
			nNumAddedFiles++;
		}
	}
	GetPakSystem()->CloseArchive(pPakFile);

	RCLog("%d Files added to the to zip file %s", nNumAddedFiles,pakFilename.c_str() );

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
static ICryXML* LoadICryXML()
{
	HMODULE hXMLLibrary = LoadLibrary("CryXML.dll");
	if (NULL == hXMLLibrary)
	{
		RCLogError("Unable to load xml library (CryXML.dll)");
		return 0;
	}
	FnGetICryXML pfnGetICryXML = (FnGetICryXML)GetProcAddress(hXMLLibrary, "GetICryXML");
	if (pfnGetICryXML == 0)
	{
		RCLogError("Unable to load xml library (CryXML.dll) - cannot find exported function GetICryXML().");
		return 0;
	}
	return pfnGetICryXML();
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef ResourceCompiler::LoadXml( const char *filename )
{
	ICryXML *pCryXML = LoadICryXML();
	if (!pCryXML)
		return false;

	// Get the xml serializer.
	IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

	// Read in the input file.
	XmlNodeRef root;
	{
		const bool bRemoveNonessentialSpacesFromContent = false;
		char szErrorBuffer[1024];
		root = pSerializer->Read(FileXmlBufferSource(filename), false, sizeof(szErrorBuffer), szErrorBuffer);
		if (!root)
		{
			RCLogError("Cannot open XML file '%s': %s\n", filename, szErrorBuffer);
			return 0;
		}
	}
	return root;
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::ProcessJobFile( EPlatform platform,IConfig* config )
{
	// Job file is an XML with multiple jobs for the RC
	string jobFile;
	if (!config->Get( "job", jobFile ))
	{
		RCLogError( "No job file specified" );
		return false;
	}

	CPropertyVars properties(this);

	XmlNodeRef root = LoadXml( jobFile.c_str() );
	if (!root)
	{
		RCLogError( "Failed to load job XML file %s",jobFile.c_str() );
		return false;
	}
	for (int i = 0; i < root->getChildCount(); i++)
	{
		XmlNodeRef jobNode = root->getChild(i);
		RunJobXmlNode( properties,platform,config,jobNode );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RunJobXmlNode( CPropertyVars &properties,EPlatform platform,IConfig* config,XmlNodeRef &jobNode )
{
	if (jobNode->isTag("Properties"))
	{
		// Attributes are config modifiers.
		for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
		{
			const char *key = "";
			const char *value = "";
			jobNode->getAttributeByIndex(attr,&key,&value);
			string strValue = value;
			properties.ExpandProperties(strValue);
			properties.SetProperty( key,strValue );
		}
		return;
	}

	if (jobNode->isTag("Run"))
	{
		const char *jobListName = jobNode->getAttr("Job");
		if (strlen(jobListName) == 0)
			return;

		// Attributes are config modifiers.
		for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
		{
			const char *key = "";
			const char *value = "";
			jobNode->getAttributeByIndex(attr,&key,&value);
			string strValue = value;
			properties.ExpandProperties(strValue);
			properties.SetProperty( key,strValue );
		}
		
		XmlNodeRef root = jobNode;
		while (root->getParent()) root = root->getParent();
		// Find JobList.
		XmlNodeRef jobListNode = root->findChild(jobListName);
		if (jobListNode)
		{
			// Execute Job sub nodes.
			for (int i = 0; i < jobListNode->getChildCount(); i++)
			{
				XmlNodeRef subJobNode = jobListNode->getChild(i);
				RunJobXmlNode( properties,platform,config,subJobNode );
			}
		}
		return;
	}

	if (jobNode->isTag("Include"))
	{
		const char *includeFile = jobNode->getAttr("file");
		if (strlen(includeFile) == 0)
			return;
		
		string jobFile;
		config->Get( "job", jobFile );
		string includePath = PathHelpers::AddSeparator(PathHelpers::GetDirectory(jobFile)) + includeFile;

		XmlNodeRef root = LoadXml( includePath );
		if (!root)
		{
			RCLogError( "Cannot open Job include file %s",includePath.c_str() );
			return;
		}

		// Add include sub-nodes
		XmlNodeRef parent = jobNode->getParent();
		for (int i = 0; i < root->getChildCount(); i++)
		{
			XmlNodeRef subJobNode = root->getChild(i);
			parent->addChild(subJobNode);
		}
		return;
	}

	if (jobNode->isTag("Job"))
	{
		RCLog( "-------------------------------------------------------------------" );
		string jobLog = "Job: ";
		// Delete all config entries from previous job.
		config->ClearPriorityUsage(eCP_PriorityJob);

		// Attributes are config modifiers.
		for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
		{
			const char *key = "";
			const char *value = "";
			jobNode->getAttributeByIndex(attr,&key,&value);

			jobLog += string("/") + key + "=" + value + " ";

			if (stricmp(key,"input") == 0)
				continue;
			string valueStr = value;
			properties.ExpandProperties(valueStr);
			config->Set( eCP_PriorityJob,key,valueStr );
		}

		string fileSpec = jobNode->getAttr( "input" );
		properties.ExpandProperties(fileSpec);
		if (!fileSpec.empty())
		{
			RCLog(jobLog);
			Compile( platform,config,fileSpec );
		}
	}
}
