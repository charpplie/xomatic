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

#include "StdAfx.h"

#include "Network/DownloadTask.h"

#include "CryAction.h"
#include "GameContext.h"
#include "INetworkService.h"
#include "Network/GameClientChannel.h"
#include "Network/GameServerChannel.h"
#include "Network/GameServerNub.h"

static bool s_enableDLLogging = false;

CDownloadTask::CDownloadTask()
{
	m_currentDownload.destFilename.clear();
	m_currentDownload.md5[0] = 0;
	m_currentDownload.sourceFilename.clear();

	m_downloadAttempt = 0;
	m_downloadState = eDS_None;
	m_currentTask = eDT_None;

	m_downloadFolderPath.clear();
}

CDownloadTask::~CDownloadTask()
{
	StopDownloadTask();
}

bool CDownloadTask::Sv_StartMapDownload(const char* levelname)
{
	if(IsDownloadTaskInProgress())
		return false;

	if(s_enableDLLogging)
		CryLog("Server next map: %s", levelname);

	// SERVER:
	// open file <map dir>\levelname\filelist.xml
	// Process xml file and redownload filelist.xml from server.
	//	Send download cmd to clients so they download filelist.xml too.
	// When filelist.xml has been downloaded, process it and attempt to download all
	//	listed files (unless already present)

	// Note that if the next map is one supplied with the game, the following will fail
	// to find the map in the user data folder, so no download will occur (and nothing
	//	will be sent to clients)

	if(gEnv->bServer)
	{
		GetUserDataFolder(m_downloadFolderPath);
		m_downloadFolderPath = m_downloadFolderPath + "Levels/" + levelname;
		string xmlfilename = m_downloadFolderPath + "/filelist.xml";

		XmlNodeRef node = GetISystem()->LoadXmlFile(xmlfilename);
		if(!node)
		{
			if(s_enableDLLogging)
				CryLog("Can't load map filelist.xml. Next map cancelled.");
			return false;
		}

		XmlString name;
		XmlString type;
		node->getAttr("name", name);
		CRY_ASSERT(!stricmp(name.c_str(), levelname));
		node->getAttr("type", type);

		XmlNodeRef indexNode = node->findChild( "Index" );
		if(indexNode)
		{
			XmlString src;
			XmlString dest;
			XmlString md5;
			int size;
			indexNode->getAttr("src", src);
			indexNode->getAttr("dest", dest);
			indexNode->getAttr("md5", md5);
			indexNode->getAttr("size", size);

			SFileDownloadParameters dl;
			dl.sourceFilename = src;
			dl.destFilename = dest;
			dl.destPath = levelname;
			dl.fileSize = 0;
			FillMD5FromString(dl, md5);

			m_currentDownload = dl;
			m_downloadList.push_back(dl);
			m_downloadAttempt = 0;

			// send download cmd to clients.
			if (CGameServerNub * pGSN = CCryAction::GetCryAction()->GetGameServerNub())
				if (TServerChannelMap * m = pGSN->GetServerChannelMap())
					for (TServerChannelMap::iterator iter = m->begin(); iter != m->end(); ++iter)
					{
						CGameClientChannel::SendStartFileDownloadWith(dl, iter->second->GetNetChannel() );
					}

			// state is now 'getting file list'
			m_currentTask = eDT_GettingIndexFile;
			return true;
		}
	}

	return false;
}

bool CDownloadTask::Cl_StartMapDownload(SFileDownloadParameters& dl)
{
	if(IsDownloadTaskInProgress())
		return false;

	if(s_enableDLLogging)
		CryLog("Client next map: %s", dl.destPath.c_str());

	// CLIENT:
	// receive a cmd to download an index.xml file. When downloaded, process in the 
	//	same way as the server.
	GetUserDataFolder(m_downloadFolderPath);
	m_downloadFolderPath += "Levels/" + dl.destPath;
	CreateDestinationFolder(m_downloadFolderPath);
	m_currentDownload = dl;
	m_downloadList.push_back(dl);
	m_downloadAttempt = 0;
	m_currentTask = eDT_GettingIndexFile;

	return true;
}

bool CDownloadTask::ProcessMapIndexFile()
{
	if(s_enableDLLogging)
		CryLog("Processing map filelist.xml");
	string file = m_downloadFolderPath + "/filelist.xml";
	XmlNodeRef node = GetISystem()->LoadXmlFile(file);
	if(!node)
		return false;

	// tODO: check this is the correct file...

	// now loop through file list and add them to the download queue.
	XmlNodeRef filesNode = node->findChild( "Files" );
	if (filesNode)
	{
		for (int i = 0; i < filesNode->getChildCount(); i++)
		{
			XmlString src;
			XmlString dest;
			XmlString md5;
			int size;

			XmlNodeRef fileNode = filesNode->getChild(i);
			fileNode->getAttr( "src", src );
			fileNode->getAttr( "dest", dest);
			fileNode->getAttr( "md5", md5);
			fileNode->getAttr( "size", size);

			SFileDownloadParameters params;			
			params.sourceFilename = src;
			params.destFilename = dest;
			params.destPath = m_downloadFolderPath;
			params.fileSize = size;
			FillMD5FromString(params, md5);

			m_downloadList.push_back(params);	
			m_downloadAttempt = 0;
		}

		m_currentTask = eDT_DownloadingFiles;
		m_downloadState = eDS_Downloading;
		return true;
	}

	return false;
}

bool CDownloadTask::StartPatchDownload(SFileDownloadParameters& dl)
{
	if(IsDownloadTaskInProgress())
		return false;

	if(s_enableDLLogging)
		CryLog("Downloading patch");

	GetUserDataFolder(m_downloadFolderPath);
	CreateDestinationFolder(m_downloadFolderPath);
	dl.destPath = m_downloadFolderPath;
	m_currentDownload = dl;
	m_downloadList.push_back(dl);
	m_downloadAttempt = 0;
	m_currentTask = eDT_DownloadingFiles;

	// store the filename in the patch system so it can be run later if required.
	INetworkService* pserv = gEnv->pNetwork->GetService("GameSpy");
	if(pserv)
	{
		IPatchCheck* pPC = pserv->GetPatchCheck();
		if(pPC && pPC->IsAvailable())
		{
			string filename;
			filename = m_downloadFolderPath;
			filename += dl.destFilename;
			pPC->SetPatchFileName(filename.c_str());
		}
	}

	return true;
}

void CDownloadTask::Update()
{
	if(m_currentTask == eDT_None)
		return;

	INetworkService* pserv = gEnv->pNetwork->GetService("GameSpy");
	if(pserv)
	{
		IFileDownloader* pfd = pserv->GetFileDownloader();
		if(pfd && pfd->IsAvailable())
		{
			// TODO: update throttling based on game state.
			//	Eg if in MP net game, throttle to not disrupt gameplay.
			//	Otherwise (for patches, etc) DL at max speed
			pfd->SetThrottleParameters(2048, 200);		// eg 10kb/sec

			switch(m_downloadState)
			{
				default:
				case eDS_None:
					// no file downloading. Start one now.
					DownloadNextFile();
					break;

				case eDS_Downloading:
					// wait for download to finish
					if(!pfd->IsDownloading())
					{
						m_downloadState = eDS_Done;
					}
					break;

				case eDS_Done:
				{
					// validate will restart the download if it failed
					if(ValidateDownload())
					{
						if(m_currentTask == eDT_GettingIndexFile)
						{
							ProcessMapIndexFile();
						}
						else
						{
							DownloadNextFile();
						}
					}
					break;
				}
			}
		}
	}
}

void CDownloadTask::StopDownloadTask()
{
	if(s_enableDLLogging)
		CryLog("Stopping all downloads");

	// cancel pending downloads
	INetworkService* pserv = gEnv->pNetwork->GetService("GameSpy");
	if(pserv)
	{
		IFileDownloader* pfd = pserv->GetFileDownloader();
		if(pfd && pfd->IsAvailable())
		{
			pfd->Stop();
			m_downloadState = eDS_None;
			m_currentTask = eDT_None;
			m_downloadList.clear();
			m_currentDownload.sourceFilename.clear();
			m_downloadAttempt = 0;
		}
	}
}

bool CDownloadTask::IsDownloadTaskInProgress() const
{
	return ((m_currentTask != eDT_None) && (m_downloadState != eDS_None));
}

int CDownloadTask::GetNumberOfFilesRemaining() const
{
	return m_downloadList.size();
}

float CDownloadTask::GetCurrentFileProgress() const
{
 	INetworkService* pserv = gEnv->pNetwork->GetService("GameSpy");
 	if(pserv)
 	{
 		IFileDownloader* pfd = pserv->GetFileDownloader();
 		if(pfd && pfd->IsAvailable())
 		{
 			return pfd->GetDownloadProgress();
 		}
 	}

	return 0.0f;
}

void CDownloadTask::DownloadNextFile()
{
	INetworkService* pserv = gEnv->pNetwork->GetService("GameSpy");
	if(pserv)
	{
		IFileDownloader* pfd = pserv->GetFileDownloader();
		if(pfd && pfd->IsAvailable())
		{
			if(!m_downloadList.empty())
			{
				// first check to see if the file exists already (except the index file - always DL this)
				m_currentDownload = m_downloadList.front();
				m_downloadList.pop_front();
				m_currentDownload.destPath = m_downloadFolderPath;
				if(!FileExists(m_currentDownload.destPath + "/" + m_currentDownload.destFilename, m_currentDownload.fileSize) || m_currentTask == eDT_GettingIndexFile)
				{
					if(s_enableDLLogging)
						CryLog("Downloading file: %s", m_currentDownload.destFilename.c_str());
					m_downloadAttempt = 0;
					pfd->DownloadFile(m_currentDownload);
					m_downloadState = eDS_Downloading;
				}
				else
				{
					if(s_enableDLLogging)
						CryLog("File exists (skipping): %s", m_currentDownload.destFilename.c_str());
					m_downloadState = eDS_Done;	
				}
			}
			else
			{
				// no more files. 
				StopDownloadTask();
				m_downloadState = eDS_None;
				m_currentTask = eDT_None;
			}
		}
	}
}

void CDownloadTask::FillMD5FromString(SFileDownloadParameters& dl, string md5)
{
	const char* md5str = md5.c_str();
	for(int i=0; i<16; ++i)
	{
		char thisbyte[3];
		thisbyte[0] = md5str[2 * i];
		thisbyte[1] = md5str[2 * i + 1];
		thisbyte[2] = 0;

		int result = 0;
		if (sscanf(thisbyte, "%x", &result) != 1)
		{
			CRY_ASSERT(0);
		}
		dl.md5[i] = static_cast<unsigned char>(result);
	}
}

bool CDownloadTask::ValidateDownload()
{
	bool fileOK = true;

	// first check we have a filename to validate...
	if(m_currentDownload.destFilename.empty())
		fileOK = false;
	
	// check the file exists and is of the expected size
	if(!FileExists(m_downloadFolderPath + "/" + m_currentDownload.destFilename, m_currentDownload.fileSize))
		fileOK = false;

	// get the checksum from the downloader
	const unsigned char* md5Checksum = NULL;
	INetworkService* pserv = gEnv->pNetwork->GetService("GameSpy");
	if(pserv)
	{
		IFileDownloader* pfd = pserv->GetFileDownloader();
		if(pfd && pfd->IsAvailable())
		{
			md5Checksum = pfd->GetFileMD5();
		}
	}
	// NB if file already existed locally, it wasn't downloaded. Probably need to md5 at load-time.
	if(md5Checksum)
	{
		for(int i=0; i<16; ++i)
		{
			if((m_currentDownload.md5[i] != 0) && (md5Checksum[i] != m_currentDownload.md5[i]))
			{
				fileOK = false;
				break;
			}
		}
	}

	if(!fileOK)
	{
		++m_downloadAttempt;
		if(m_downloadAttempt < 3)
		{
			if(s_enableDLLogging)
				CryLog("Download failed, retrying: %s", m_currentDownload.destFilename.c_str());
			m_downloadList.push_front(m_currentDownload);
			m_downloadState = eDS_Downloading;
		}
		else
		{
			if(s_enableDLLogging)
				CryLog("Download failed, giving up: %s", m_currentDownload.destFilename.c_str());
			StopDownloadTask();
		}
	}

	if(fileOK && s_enableDLLogging)
		CryLog("File downloaded ok");

	return fileOK;
}

bool CDownloadTask::FileExists(string file, int expectedSize)
{
	bool ok = true;

	ICryPak* pPak = gEnv->pCryPak;
	if(pPak)
	{
		FILE* pFile = pPak->FOpen(file, "r", ICryPak::FLAGS_NO_FULL_PATH);
		if(!pFile)
			ok = false;
		else
		{
			if(expectedSize > 0 && pPak->FGetSize(pFile) != expectedSize)
				ok = false;

			pPak->FClose(pFile);
		}
	}

	return ok;
}

bool CDownloadTask::GetUserDataFolder(string& path)
{
	// eventually this will be something like 
	// 'c:\documents and settings\<username>\application data\Crytek\Crysis\'
	//	but since we can't yet load levels from there, just use this:
	path = PathUtil::GetGameFolder() + "/Levels/Multiplayer/DL/";
	return true;
}

bool CDownloadTask::CreateDestinationFolder(string& folder)
{
	ICryPak* pPak = gEnv->pCryPak;
	if(pPak)
		pPak->MakeDir(folder.c_str());

	return true;
}