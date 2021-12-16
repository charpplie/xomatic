#include "StdAfx.h"
#include "ConsoleHotUpdate.h"
#include "ConsoleConnection.h"

#include "Settings.h"
#include "ResourceCompilerHelper.h"

// file types { fileExtension, compileOptions, Ignorable, needsCompiling }
CConsoleHotUpdate::CThread::HotSyncFileTypes CConsoleHotUpdate::CThread::fileTypes[] = 
{
	{"dds",			NULL,						false,true},
	{"tif",			NULL,						false,true},
	{"mtl",			"/overwriteextension=xml",	false,true},
	{"xml",			NULL,						false,true},
	{"ent",			"/overwriteextension=xml",	false,true},
	{"dlg",			"/overwriteextension=xml",	false,true},
	{"cgf",			NULL,						false,true},
	{"cga",			NULL,						false,true},
	{"chr",			NULL,						false,true},
	{"cba",			"/cleanupfast=1",			false,true},
	{"cba",			"/cleanupfast=1",			false,true},
	{"animevents",	"/overwriteextension=xml",	false,true},
	{"pak",			NULL,						false,false},
	{"max",			NULL,						true,false},
	{"psd",			NULL,						true,false},		
	{"bak",			NULL,						true,false},
	{"lyr",			NULL,						true,false},		
	{"cry",			NULL,						true,false},
	{"tmp",			NULL,						true,false},
	{"pak$",		NULL,						true,false},
};

ILINE void replace_chars(string &s, const char replaceThis, const char withThis)
{
	size_t length = s.size();
	for (size_t i=0; i<length; i++)
	{
		if (s[i] == replaceThis)
		{
			s.replace(i, 1, 1, withThis);
		}
	}
}

ILINE void append_if_not_last(string &s, char c, const char *ifNot)
{
	size_t length = s.length();
	if (!length)
		return;

	const char &last = s[s.length()-1];
	while (*ifNot)
	{
		if (last == *ifNot)
			return;

		++ifNot;
	}

	s += c;
}

//


/*

  CConsoleHotUpdate::CThread

*/

CConsoleHotUpdate::CThread::CThread()
{
	m_bRun = true;
}

CConsoleHotUpdate::CThread::~CThread()
{
}

//

void CConsoleHotUpdate::CThread::Begin()
{
	Start(-1, "ConsoleHotUpdate");
}

void CConsoleHotUpdate::CThread::End()
{
	m_bRun = false;
	//WaitForThread();
	Stop();
}

void CConsoleHotUpdate::CThread::Resume()
{
	m_processCondition.Notify();
	m_processMutex.Unlock();
}

void CConsoleHotUpdate::CThread::FileProcessingPause()
{
	m_pauseMutex.Lock();
}

void CConsoleHotUpdate::CThread::FileProcessingResume()
{
	m_pauseMutex.Unlock();
}

void CConsoleHotUpdate::CThread::AddFile(const char *file)
{
	m_files.insert(file);
	Resume();
}

void CConsoleHotUpdate::CThread::ProcessFile(const char *file)
{
	string extension = Path::GetExt(file);

	if (IgnoreFile(extension))
		return;

	bool bNeedsCompiling = NeedsCompiling(extension);

	std::vector<string> output;
	for (uint32 i=CHotUpdateSystem::eFirstPlatform;	i<CHotUpdateSystem::eNumberOfPlatforms; ++i)
	{
		CHotUpdateSystem::eConsolePlatforms ePlatform =	(CHotUpdateSystem::eConsolePlatforms)i;

		if (!gSettings.oHotUpdateSystemSettings.boPlatformEnabled[i])
			continue;

		output.clear();
		if (bNeedsCompiling)
		{
			if (!ResourceCompiler(ePlatform, file, output, extension))
				continue;
		}
		else
		{
			output.push_back(file);
		}

		if ( gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled )
			AssetCopyAndNotify(ePlatform, file, output);

		if (bNeedsCompiling)
		{
			for (size_t j=0; j<output.size(); ++j)
			{
				CFileUtil::DeleteFile(output[j].c_str());
			}
		}
	}
}

bool CConsoleHotUpdate::CThread::IgnoreFile(const char * ext)
{
	//if (CResourceCompilerHelper().IsSourceFormat(ext))
	//	return true;

	for (uint32 i=0; i<ARRAYSIZE(fileTypes); ++i)
	{
		if (stricmp(ext, fileTypes[i].extension) == 0 )
		{
			return fileTypes[i].ignore;
		}
	}

	return true;
}

bool CConsoleHotUpdate::CThread::NeedsCompiling(const char * ext)
{
	for (uint32 i=0; i<ARRAYSIZE(fileTypes); ++i)
	{
		if (stricmp(ext, fileTypes[i].extension) == 0)
		{
			return fileTypes[i].compile;
		}
	}

	return false;
}

bool CConsoleHotUpdate::CThread::GetCompileOptions(const char * ext, CString & options)
{
	for (uint32 i=0; i<ARRAYSIZE(fileTypes); ++i)
	{
		if (stricmp(ext, fileTypes[i].extension) == 0)
		{
			if ( fileTypes[i].compileparams != NULL )
			{
				options = fileTypes[i].compileparams;
			}
			return true;
		}
	}
	return false;
}

bool CConsoleHotUpdate::CThread::ResourceCompiler(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *file, std::vector<string> &output, const char * ext)
{
	CString settings("");
	CString compileOptions("");
	
	switch (ePlatform)
	{
		case CHotUpdateSystem::eXBOX360 : settings.Append("/p=X360 ");
			break;
		case CHotUpdateSystem::ePLAYSTATION3 : settings.Append("/p=PS3 ");
			break;
		default : return false;
	}

	settings.Append("/refresh ");
	
	if ( GetCompileOptions(ext,compileOptions) )
	{
		settings.Append(compileOptions);
	}

	string workingPath = Path::GetExecutableParentDirectory();
	workingPath = Path::AddBackslash(workingPath.c_str());
	workingPath += gSettings.strStandardTempDirectory;
	workingPath = Path::AddBackslash(workingPath.c_str());
	CFileUtil::CreatePath(workingPath.c_str());

	string fileTemp = workingPath;
	fileTemp += Path::GetFile(file);
	if (!::CopyFile(file, fileTemp.c_str(), false))
		return false;

	if (CResourceCompilerHelper().CallResourceCompiler(
		fileTemp.c_str(), settings.GetBuffer(), NULL, false, false, CResourceCompilerHelper::eRcExePath_currentFolder, true, true, NULL)
		!= CResourceCompilerHelper::eRcCallResult_success)
	{
		CFileUtil::DeleteFile(fileTemp.c_str());
		return false;
	}

	string fileName;
	output.clear();
	for (uint32 i=0;; ++i)
	{
		fileName.Format("%s.%d", fileTemp.c_str(), i);
		if (!CFileUtil::FileExists(fileName.c_str()))
			break;

		output.push_back(fileName);
	}
	if (output.empty())
		output.push_back(fileTemp);
	else
		CFileUtil::DeleteFile(fileTemp.c_str());
	
	return true;
}

bool CConsoleHotUpdate::CThread::AssetCopyAndNotify(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *fileOriginal, std::vector<string> &files)
{
	string destination =
		gSettings.oHotUpdateSystemSettings.GetPlatformConsoleRootDirectory(ePlatform, CString());
	if (destination.empty())
		return false;
	destination = Path::AddBackslash(destination.c_str());

	string fileRelative = Path::GetRelativePath(fileOriginal);
	fileRelative.MakeLower();
	destination += Path::GetPath(fileRelative.c_str());

	string fileDestination;
	for (size_t i=0; i<files.size(); ++i)
	{
		fileDestination = destination;
		fileDestination += Path::GetFile(files[i].c_str());
		if (ePlatform == CHotUpdateSystem::ePLAYSTATION3)
			::replace_chars(fileDestination, '\\', '/');

		if (!CConsoleConnection::Instance().FileCopyTo(
			ePlatform, files[i].c_str(), fileDestination))
		{
			return false;
		}
	}

	INotificationNetworkClient *pClient =
		CConsoleConnection::Instance().NotificationBegin(ePlatform);
	if (!pClient)
		return false;

	fileRelative = Path::GetRelativePath(fileOriginal, true);
	::replace_chars(fileRelative, '\\', '/');
	fileRelative.MakeLower();
	bool bSent = pClient->Send("HotUpdate", fileRelative.c_str(), fileRelative.length() + 1);

	CConsoleConnection::Instance().NotificationEnd(ePlatform);

	return bSent;
}

// CryThread

void CConsoleHotUpdate::CThread::Run()
{
	CryThreadSetName(-1, "ConsoleHotUpdate");

	string file;
	while (m_bRun)
	{
		while (m_bRun && m_files.empty())
			m_processCondition.Wait(m_processMutex);

		if (!m_bRun)
			break;

		if (!m_files.pop_front(file))
			continue;

		m_pauseMutex.Lock();
		ProcessFile(file);
		m_pauseMutex.Unlock();
	}
}

/*

  CConsoleHotUpdate

*/

CConsoleHotUpdate::CConsoleHotUpdate()
{
	m_thread.Begin();
}

CConsoleHotUpdate::~CConsoleHotUpdate()
{
	m_thread.End();
}

//

void CConsoleHotUpdate::FileProcessingPause()
{
	m_thread.FileProcessingPause();
}

void CConsoleHotUpdate::FileProcessingResume()
{
	m_thread.FileProcessingResume();
}

void CConsoleHotUpdate::NotifyFilesChanged(std::set<CString> &files)
{
	for (std::set<CString>::iterator f = files.begin(); f != files.end(); ++f)
	{
		m_thread.AddFile(*f);
	}
}
