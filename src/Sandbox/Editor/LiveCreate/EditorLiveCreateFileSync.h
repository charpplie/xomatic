#pragma once

//////////////////////////////////////////////////////////////////////////
//  CryENGINE Source File
//  Copyright (C) 2000-2012, Crytek GmbH, All rights reserved
//////////////////////////////////////////////////////////////////////////

#include "ILiveCreateCommon.h"
#include "ILiveCreatePlatform.h"
#include "ILiveCreateManager.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

class CBGTask_CopyFileToTarget;

class CFileSyncManager
{
public:
	// Used to specify settings for the file types
	struct SSyncFileType
	{
		SSyncFileType()
			: pCompileParams("")
			, bIgnore(false)
			, compileStageCount(0)
			, pOutputFileExtensions(NULL)
		{}

		SSyncFileType(
			const char* pCompileParams,
			bool bIgnore,
			uint32 aCompileStageCount = 0,
			const char* pRcOutputExtensions = NULL)
			: pCompileParams(pCompileParams)
			, bIgnore(bIgnore)
			, compileStageCount(aCompileStageCount)
			, pOutputFileExtensions(pRcOutputExtensions)
		{}

		// compile params given to RC
		const char* pCompileParams;

		// ignore changed files of this type
		bool bIgnore;

		// number of RC compile steps for this file type, 0 is no compilation needed
		uint32 compileStageCount;

		// possible file extensions to look for after RC run
		const char* pOutputFileExtensions;
	};

	// File item to sync
	struct SFileItem
	{
		SFileItem()
			: groupId(0)
		{
		}

		SFileItem(const char* pOriginalFilename, const char* pTempFilename, int aGroupId = 0, const char* pOnlyForPlatformName = NULL)
		{
			originalFilename = pOriginalFilename;
			tempFilename = pTempFilename;
			onlyForPlatformName = pOnlyForPlatformName;
			groupId = aGroupId;
		}

		string originalFilename;
		string tempFilename;
		string onlyForPlatformName;
		int groupId;
	};

	CFileSyncManager(class CEditorManager* pManager);
	virtual ~CFileSyncManager();

	// request a local data file to be send to all consoles, will call RC if needed
	bool SyncFile(const char* pPath);

	// is given file ignored by the file sync
	bool IsFileIgnored(const char* pFilename);

	// remove all files from the send queue
	void AbortSync();

	// process file queue
	void Update();

private:
	bool AddFile(const char* pOriginalFilename, const char* pTempFilename, int aGroupId = 0, const char* pOnlyForPlatformName = NULL);
	const SSyncFileType* FindSyncFileType(const char* pFileExtension) const;
	bool GetCompileOptions(const char* pFileExtension, CString& rOutOptions) const;
	bool IgnoreFileExtension(const char* pFileExtension) const;
	bool NeedsCompiling(const char* pFileExtension) const;
	bool NeedsMultipassCompiling(const char* pFileExtension) const;
	bool ExecuteResourceCompiler(const char* pPlatformName, const char* pFilename, std::vector<CString>& rOutputFilenames);

private:
	class CEditorManager* m_pManager;
	string m_fileSyncTempFolder;
	CryMutex m_lockFilesToSync;

	// RC settings
	typedef std::map<CString, SSyncFileType> TFileTypesMap;
	TFileTypesMap m_fileTypes;

	// file processing queue
	typedef CryMT::CLocklessPointerQueue< SFileItem > TFileQueue;
	TFileQueue m_filesToSync;

	// selective file ignore
	typedef std::vector<string> TFileMaskList;
	TFileMaskList m_ignoreFileMasks;

	// file copy jobs (one for each platform)
	typedef std::vector<CBGTask_CopyFileToTarget*> TCopyJobs;
	TCopyJobs m_copyJobs;
};

}

#endif