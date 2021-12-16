/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  Base-class functionality for downloading files
-------------------------------------------------------------------------
History:
- 16/02/2007   : Steve Humphreys, Created
*************************************************************************/

#ifndef __DOWNLOADTASK_H__
#define __DOWNLOADTASK_H__

#pragma once

#include "INetworkService.h"

// the current task
enum EDownloadTask
{
	eDT_None,									// not downloading
	eDT_GettingIndexFile,			// downloading a filelist.xml, which will be parsed to get the download list
	eDT_DownloadingFiles,			// downloading files in the queue
};

// the status of the current task
enum EDownloadState
{
	eDS_None,									// not doing anything
	eDS_Downloading,					// downloading a file
	eDS_Done,									// finished
};

class CDownloadTask 
{
public:
	CDownloadTask();
	virtual ~CDownloadTask();

	// check client and server machines both have a particular level
	bool Sv_StartMapDownload(const char* levelname);
	bool Cl_StartMapDownload(SFileDownloadParameters& dl);
	bool ProcessMapIndexFile();

	// download a patch executable
	bool StartPatchDownload(SFileDownloadParameters& dl);

	void Update();
	void StopDownloadTask();

	bool IsDownloadTaskInProgress() const;
	int GetNumberOfFilesRemaining() const;
	float GetCurrentFileProgress() const;

private:

	void DownloadNextFile();
	bool ValidateDownload();								// returns true if the last file completed ok. Retries it if not.
	bool FileExists(string file, int expectedSize);

	bool GetUserDataFolder(string& path);
	bool CreateDestinationFolder(string& folder);

	void FillMD5FromString(SFileDownloadParameters& dl, string md5);


	EDownloadState m_downloadState;					// status of the current download task
	EDownloadTask m_currentTask;						// what are we currently doing

	std::list<SFileDownloadParameters> m_downloadList;		// download queue
	SFileDownloadParameters m_currentDownload;						// file currently downloading

	int m_downloadAttempt;									// fail after three unsuccessful attempts

	string m_downloadFolderPath;						// where to save the downloaded files
};

#endif // __DOWNLOADTASK_H__