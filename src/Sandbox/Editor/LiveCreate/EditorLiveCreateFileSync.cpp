////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "LiveCreate/EditorLiveCreateFileSync.h"
#include "LiveCreate/EditorLiveCreateTasks.h"
#include "ResourceCompilerHelper.h"

#ifndef NO_LIVECREATE

#define NO_LIVECREATE_COMMAND_IMPLEMENTATION
#include "../../CryEngine/CryLiveCreate/LiveCreateCommands.h"
#include "../../CryEngine/CryLiveCreate/LiveCreate_System.h"
#undef NO_LIVECREATE_COMMAND_IMPLEMENTATION

namespace LiveCreate
{

CFileSyncManager::CFileSyncManager(CEditorManager* pManager)
	: m_pManager(pManager)
{
	m_ignoreFileMasks.push_back("*.lyr");
	m_ignoreFileMasks.push_back("*.cry");
	m_ignoreFileMasks.push_back("*.tmp");
	m_ignoreFileMasks.push_back("*.tmp$");
	m_ignoreFileMasks.push_back("*$tmp*");
	m_ignoreFileMasks.push_back("*.bak");
	m_ignoreFileMasks.push_back("*.bai");
	m_ignoreFileMasks.push_back("*.bmp");
	m_ignoreFileMasks.push_back("*.tif");
	m_ignoreFileMasks.push_back("*.max");
	m_ignoreFileMasks.push_back("*.psd");
	m_ignoreFileMasks.push_back("Editor/*");
	m_ignoreFileMasks.push_back("Editor\\*");

	m_fileTypes["dds"] = SSyncFileType(NULL, false, 1, "dds");
	m_fileTypes["tif"] = SSyncFileType(NULL, false, 2, "dds");
	m_fileTypes["mtl"] = SSyncFileType("/overwriteextension=xml", false, 1, "mtl,binxml,binmtl");
	m_fileTypes["xml"] = SSyncFileType(NULL, false, 1, "xml,binxml");
	m_fileTypes["ent"] = SSyncFileType("/overwriteextension=xml", false, 1, NULL);
	m_fileTypes["dlg"] = SSyncFileType("/overwriteextension=xml", false, 1, NULL);
	m_fileTypes["cgf"] = SSyncFileType(NULL, false, 1, NULL);
	m_fileTypes["cga"] = SSyncFileType(NULL, false, 1, NULL);
	m_fileTypes["chr"] = SSyncFileType(NULL, false, 1, NULL);
	m_fileTypes["cba"] = SSyncFileType("/cleanupfast=1", false, 1, NULL);
	m_fileTypes["cdf"] = SSyncFileType(NULL, false, 0, NULL);
	m_fileTypes["animevents"] = SSyncFileType("/overwriteextension=xml", false, 1, NULL);
	m_fileTypes["pak"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["max"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["psd"] = SSyncFileType(NULL, true, 0, "tif,dds");
	m_fileTypes["bak"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["lyr"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["cry"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["cry$"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["tmp"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["tmp$"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["pak$"] = SSyncFileType(NULL, true, 0, NULL);
	m_fileTypes["bai"] = SSyncFileType(NULL, true, 0, NULL);
}

CFileSyncManager::~CFileSyncManager()
{
	while (!m_filesToSync.empty())
	{
		m_filesToSync.pop();
	}

	for (TCopyJobs::iterator it = m_copyJobs.begin();
		it != m_copyJobs.end(); ++it)
	{
		(*it)->Cancel();
		(*it)->Release();
	}

	m_copyJobs.clear();
}

bool CFileSyncManager::AddFile(const char* pOriginalFilename, const char* pTempFilename, int aGroupId, const char* pOnlyForPlatformName)
{
	// delete the temp filename, this happens only on PC
	if (IsFileIgnored(pOriginalFilename))
	{
		m_pManager->LogMessagef(eLogType_Normal, "Deleting ignored temp file: '%s'", pTempFilename);
		::DeleteFile(pTempFilename);
		return false;
	}

	SFileItem* pFileInfo = new SFileItem(
		pOriginalFilename,
		pTempFilename,
		aGroupId,
		pOnlyForPlatformName);

	m_filesToSync.push(pFileInfo);
	return true;
}

bool CFileSyncManager::IsFileIgnored(const char* pFilename)
{
	// check if the file is ignorable
	for (size_t i = 0; i < m_ignoreFileMasks.size(); ++i)
	{
		if (PathUtil::MatchWildcard(pFilename, m_ignoreFileMasks[i].c_str()))
		{
			return true;
		}
	}

	return false;
}

void CFileSyncManager::Update()
{
	// process current copy jobs
	for (TCopyJobs::iterator it = m_copyJobs.begin(); 
		it != m_copyJobs.end(); )
	{
		CBGTask_CopyFileToTarget* pJob = (*it);

		if (pJob->HasFinished())
		{
			if (!pJob->HasFinishedWithoutError())
			{
				m_pManager->LogMessagef(eLogType_Error, "Failed to copy file '%s' to '%s'",
					(const char*)pJob->GetDestPath(), (const char*)pJob->GetAddress());
			}

			pJob->Release();

			it = m_copyJobs.erase(it);
		}
		else
		{
			++it;
		}
	}

	// still copying current file
	if (!m_copyJobs.empty())
	{
		return;		
	}

	// get next file to send
	SFileItem* pNextFile = m_filesToSync.pop();
	if (NULL == pNextFile)
	{
		return;
	}

	// get hosts (adds references)
	std::vector<CEditorHostInfo*> pHosts;
	m_pManager->GetEnabledHosts(pHosts);
	const uint numHosts = pHosts.size();
	for (uint i = 0; i < numHosts; ++i )
	{
		CEditorHostInfo* pHost = pHosts[i];

		// invalid platform
		IPlatformHandler* pPlatformHandler = pHost->GetPlatform();
		if (NULL == pPlatformHandler)
		{
			continue;
		}

		// do not sync files for platforms with shared data directory (saves time)
		if (pPlatformHandler->IsFlagSet(IPlatformHandler::eFlag_SharedDataDirectory))
		{
			continue;
		}

		// if we choose to send file to specific platforms
		if (!pNextFile->onlyForPlatformName.empty())
		{
			if (pNextFile->onlyForPlatformName != pPlatformHandler->GetPlatformName())
			{
				continue;
			}
		}

		string destFilename = pHost->GetBuildDirectory();
		destFilename = PathUtil::AddSlash(destFilename);
		string lower = pNextFile->originalFilename;
		lower.MakeLower();
		destFilename += lower;

		m_pManager->LogMessagef(eLogType_Normal, "Copy (target: %s [%s]): '%s' to '%s'",
			pPlatformHandler->GetTargetName(),
			pPlatformHandler->GetPlatformName(),
			pNextFile->tempFilename.c_str(), destFilename.c_str());

		// create a file sync job
		CBGTask_CopyFileToTarget* job = new CBGTask_CopyFileToTarget(pHost->GetAddres(), pNextFile->tempFilename.c_str(), destFilename.c_str());
		GetIEditor()->GetBackgroundTaskManager()->AddTask(job, eTaskPriority_FileUpdate, eTaskThreadMask_IO);
	}

	// cleanup host references
	for (uint i = 0; i < numHosts; ++i )
	{
		CEditorHostInfo* pHost = pHosts[i];
		pHost->Release();
	}

	// after the wile was sent, delete it
	m_pManager->LogMessagef(eLogType_Normal, "Deleting temp file: '%s'", pNextFile->tempFilename.c_str());
	if ( !::DeleteFile(pNextFile->tempFilename.c_str()))
	{
		m_pManager->LogMessagef(eLogType_Warning, "Failed to delete temp file: '%s'", pNextFile->tempFilename.c_str());
	}

	// send a notification to hosts
	{
		CLiveCreateCmd_FileSynced command;
		command.m_fileName = pNextFile->originalFilename;
		m_pManager->SendCommand(command);
	}

	delete pNextFile;

}

const CFileSyncManager::SSyncFileType* CFileSyncManager::FindSyncFileType(const char* pFileExtension) const
{
	TFileTypesMap::const_iterator it = m_fileTypes.find(pFileExtension);
	if (it == m_fileTypes.end())
	{
		return NULL;
	}
	else
	{
		return &((*it).second);
	}
}

bool CFileSyncManager::GetCompileOptions(const char* pFileExtension, CString& rOutOptions) const
{
	TFileTypesMap::const_iterator it = m_fileTypes.find(pFileExtension);
	if (it == m_fileTypes.end())
	{
		rOutOptions = "";
		return false;
	}
	else
	{
		rOutOptions = (*it).second.pCompileParams;
		return true;
	}
}

bool CFileSyncManager::IgnoreFileExtension(const char* pFileExtension) const
{
	TFileTypesMap::const_iterator it = m_fileTypes.find(pFileExtension);
	if (it == m_fileTypes.end())
	{
		return false;
	}
	else
	{
		return (*it).second.bIgnore;
	}
}

bool CFileSyncManager::NeedsCompiling(const char* pFileExtension) const
{
	TFileTypesMap::const_iterator it = m_fileTypes.find(pFileExtension);
	if (it == m_fileTypes.end())
	{
		return false;
	}
	else
	{
		return (*it).second.compileStageCount != 0;
	}
}

bool CFileSyncManager::NeedsMultipassCompiling(const char* pFileExtension) const
{
	TFileTypesMap::const_iterator it = m_fileTypes.find(pFileExtension);
	if (it == m_fileTypes.end())
	{
		return false;
	}
	else
	{
		return (*it).second.compileStageCount > 0;
	}
}

bool CFileSyncManager::ExecuteResourceCompiler(const char* pPlatformName, const char* pFilename, std::vector<CString>& rOutputFilenames)
{
	CString settings, compileOptions, fileExt;

	fileExt = Path::GetExt(pFilename);
	fileExt.MakeLower();

	const SSyncFileType* pFileType = FindSyncFileType(fileExt);
	if (NULL == pFileType)
	{
		return false;
	}

	if (pFileType->bIgnore)
	{
		return false;
	}

	if (!stricmp(pPlatformName, "X360"))
	{
		settings.Append( "/p=x360 " );
	}
	else if (!stricmp(pPlatformName, "PS3"))
	{
		settings.Append("/p=ps3 ");
	}

	settings.Append("/refresh ");

	if (GetCompileOptions(fileExt, compileOptions))
	{
		settings.Append(compileOptions);
	}

	CString workingPath = Path::GetExecutableParentDirectory();

	workingPath = Path::AddBackslash( workingPath );
	workingPath += gSettings.strStandardTempDirectory;
	workingPath = Path::AddBackslash( workingPath );
	CFileUtil::CreatePath( workingPath );

	CString fileTemp = workingPath;
	fileTemp += Path::GetFile( pFilename );

	if( !::CopyFile( pFilename, fileTemp, false ) )
	{
		return false;
	}

	CString compiledTemp(workingPath);

	compiledTemp += "compiled\\";
	CString targetroot(" /targetroot=");
	targetroot += compiledTemp;
	settings.Append(targetroot);

	if (CResourceCompilerHelper().CallResourceCompiler(
		fileTemp.GetBuffer(),
		settings.GetBuffer(),
		NULL,
		false,
		CResourceCompilerHelper::eRcExePath_currentFolder,
		true,
		true,
		NULL) != CResourceCompilerHelper::eRcCallResult_success)
	{
		CFileUtil::DeleteFile(fileTemp);
		return false;
	}

	rOutputFilenames.clear();
	CString fileNameTemp(compiledTemp);
	CString fileName;
	fileNameTemp += Path::GetFile(pFilename);

	if (CFileUtil::FileExists(fileNameTemp))
	{
		rOutputFilenames.push_back(fileNameTemp);
	}

	size_t i = 0;
	bool bHasDDSMips = false;

	// search for XXX.n split files (like DDS.0)
	while (true)
	{
		fileName.Format("%s.%d", fileNameTemp.GetBuffer(), i);

		if (!CFileUtil::FileExists(fileName))
		{
			break;
		}

		rOutputFilenames.push_back(fileName);
		bHasDDSMips = true;
		++i;
	}

	if (bHasDDSMips)
	{
		// add the original filename at the end so it will sync
		rOutputFilenames.push_back(pFilename);
	}

	// search for files which can possibly be output of RC compilation
	const SSyncFileType* pFileTypeInfo = FindSyncFileType(fileExt);
	if (pFileTypeInfo && pFileTypeInfo->pOutputFileExtensions)
	{
		std::vector<CString> extensions;

		SplitString(CString(pFileTypeInfo->pOutputFileExtensions), extensions, ',');

		for (size_t i = 0; i < extensions.size(); ++i)
		{
			fileName.Format("%s.%s", fileNameTemp.GetBuffer(), extensions[i].GetBuffer());

			if (!CFileUtil::FileExists(fileName))
			{
				continue;
			}

			// add the compiled temp file
			rOutputFilenames.push_back(fileName);
		}
	}

	// delete the original non-compiled temp file
	CFileUtil::DeleteFile(fileTemp);
	return true;
}

bool CFileSyncManager::SyncFile(const char* szFilePath)
{
	// do not accept new files if disabled
	if (!m_pManager->GetGeneralSettings().bSyncFileUpdates)
	{
		return false;
	}

	// file is not allowed
	if (IsFileIgnored(szFilePath))
	{
		return false;
	}

	// get host that support FileSync
	std::vector<CEditorHostInfo*> pHosts;
	m_pManager->GetEnabledHosts(pHosts);
	if (pHosts.empty())
	{
		return false;
	}

	// execute RC for given file for every platform type this file will be send to
	{
		CWaitCursor wait;

		// extract actual platform types
		std::set<string> platformTypes;
		for (size_t i = 0; i < pHosts.size(); ++i)
		{
			IPlatformHandler* pPlatformHandler = pHosts[i]->GetPlatform();
			pHosts[i]->Release();

			if (NULL != pPlatformHandler)
			{
				platformTypes.insert(pPlatformHandler->GetPlatformName());
			}
		}

		for (auto it = platformTypes.begin(); it != platformTypes.end(); ++it)
		{
			const string& platformType = (*it);

			// process the modified PC files by the ResourceCompiler
			std::vector<CString> outFilenames;
			if (!ExecuteResourceCompiler(platformType, szFilePath, outFilenames))
			{
				continue;
			}

			for (size_t i = 0; i < outFilenames.size(); ++i)
			{
				static volatile int s_tempFileUidCounter = 0;
				const char* kFileSyncTempFolder = "Temp/FileSync";

				const CString& theFile = outFilenames[i];
				CString filename = Path::GetFile(theFile);
				CString strTmp = kFileSyncTempFolder;
				strTmp = PathUtil::ToDosPath((LPCTSTR)strTmp).c_str();

				CreateDirectory(strTmp, 0);
				CString tempFilename = kFileSyncTempFolder;
				tempFilename = PathUtil::AddSlash(tempFilename);
				strTmp.Format("uid%d_", s_tempFileUidCounter);
				CryInterlockedIncrement(&s_tempFileUidCounter);
				tempFilename += strTmp;
				tempFilename += filename;
				tempFilename = PathUtil::ToDosPath((LPCTSTR)tempFilename).c_str();
				CopyFile((LPCSTR)theFile, tempFilename, FALSE);

				tempFilename = Path::AddBackslash(Path::GetExecutableParentDirectory()) + tempFilename;

				// now make the path to original file
				CString origModifiedFile = PathUtil::GetPath(szFilePath).c_str();

				origModifiedFile = PathUtil::AddSlash(origModifiedFile);
				origModifiedFile += filename;
				origModifiedFile = PathUtil::ToDosPath((LPCTSTR)origModifiedFile);

				AddFile((const char*)origModifiedFile, tempFilename, 0, platformType.c_str());
			}
		}
	}

	// added
	return true;
}

} // namespace LiveCreate

#endif

