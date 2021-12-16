////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <IServiceNetwork.h>
#include <IRemoteCommand.h>
#include "LiveCreate/EditorLiveCreateHostInfo.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "EditorLiveCreateTasks.h"
#include "EditorLiveCreateSchedule.h"
#include "CrySocks.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

//-----------------------------------------------------------------------------

CBGScheduleWork_Base::CBGScheduleWork_Base()
	: m_refCount(1)
{
}

CBGScheduleWork_Base::~CBGScheduleWork_Base()
{
	CRY_ASSERT(m_refCount == 0);
}

void CBGScheduleWork_Base::AddRef()
{
	CryInterlockedIncrement(&m_refCount);
}

void CBGScheduleWork_Base::Release()
{
	if (0 == CryInterlockedDecrement(&m_refCount))
	{
		delete this;
	}
}

bool CBGScheduleWork_Base::OnStart()
{
	return true;
}

bool CBGScheduleWork_Base::OnStop()
{
	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_Base::OnUpdate()
{
	return eScheduleWorkItemStatus_Finished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_ResetTarget::CBGScheduleWork_ResetTarget(CEditorHostInfo* pHost)
	: m_pHost(pHost)
	, m_pTarget(pHost->GetPlatform())
{
	m_pTarget->AddRef();
	m_pHost->AddRef();

	m_name = "Resetting '";
	m_name += pHost->GetTargetName();
	m_name += "'";
}

CBGScheduleWork_ResetTarget::~CBGScheduleWork_ResetTarget()
{
	m_pTarget->Release();
	m_pHost->Release();
}

const char* CBGScheduleWork_ResetTarget::GetDescription() const
{
	return m_name.c_str();
}

float CBGScheduleWork_ResetTarget::GetProgress() const
{
	return 0.0f;
}

bool CBGScheduleWork_ResetTarget::OnStart()
{
	// select reset mode
	LiveCreate::EResetMode resetMode = LiveCreate::eResetMode_Soft;
	if (GetIEditor()->GetLiveCreate()->GetGeneralSettings().bFastTargetReset)
	{
		resetMode = LiveCreate::eResetMode_Soft;

		const float resetTime = GetIEditor()->GetLiveCreate()->GetAdvancedSettings().fFastResetWaitTime;
		m_resetEndTime = gEnv->pTimer->GetAsyncTime() + resetTime;
	}
	else
	{
		resetMode = LiveCreate::eResetMode_Soft;

		const float resetTime = GetIEditor()->GetLiveCreate()->GetAdvancedSettings().fSlowResetWaitTime;
		m_resetEndTime = gEnv->pTimer->GetAsyncTime() + resetTime;
	}

	if (!m_pTarget->Reset(resetMode))
	{
		return false;
	}

	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_ResetTarget::OnUpdate()
{
	const CTimeValue time = gEnv->pTimer->GetAsyncTime();
	if (time > m_resetEndTime)
	{
		return eScheduleWorkItemStatus_Finished;
	}

	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_ExportLevel::CBGScheduleWork_ExportLevel(bool bExportLevel, bool bExportTerrain)
	: m_bExportLevel(bExportLevel)
	, m_bExportTerrain(bExportTerrain)
{
}

const char* CBGScheduleWork_ExportLevel::GetDescription() const
{
	if (m_bExportLevel && m_bExportTerrain)
	{
		return "Exporting level & terrain";
	}
	else if (m_bExportTerrain)
	{
		return "Exporting terrain";
	}
	else
	{
		return "Exporting level data";
	}
}

float CBGScheduleWork_ExportLevel::GetProgress() const
{
	return 0.0f;
}

bool CBGScheduleWork_ExportLevel::OnStart()
{
	((CCryEditApp*)AfxGetApp())->ExportLevel(m_bExportLevel, m_bExportTerrain, false);
	return true;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_CopyFileToTarget::CBGScheduleWork_CopyFileToTarget(CEditorHostInfo* pHost, const CString& srcPath, const CString& destPath) 
	: m_address(pHost->GetAddres())
	, m_sourcePath(srcPath)
	, m_destPath(destPath)
	, m_pCopyTask(NULL)
{
}

CBGScheduleWork_CopyFileToTarget::~CBGScheduleWork_CopyFileToTarget()
{
	SAFE_RELEASE(m_pCopyTask);
}

const char* CBGScheduleWork_CopyFileToTarget::GetDescription() const
{
	return "Copy file";
}

float CBGScheduleWork_CopyFileToTarget::GetProgress() const
{
	return 0.0f;
}

bool CBGScheduleWork_CopyFileToTarget::OnStart()
{
	// create and issue actual file copy task
	m_pCopyTask = new CBGTask_CopyFileToTarget(m_address, m_sourcePath, m_destPath);
	m_pCopyTask->AddRef(); // local reference
	GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pCopyTask, eTaskPriority_FileUpdate, eTaskThreadMask_Any);
	return true;
}

bool CBGScheduleWork_CopyFileToTarget::OnStop()
{
	if (NULL != m_pCopyTask)
	{
		m_pCopyTask->Cancel();
		m_pCopyTask->Release();
		m_pCopyTask = NULL;
	}

	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_CopyFileToTarget::OnUpdate()
{
	if (m_pCopyTask->HasFinished())
	{
		if (m_pCopyTask->HasFinishedWithoutError())
		{
			SAFE_RELEASE(m_pCopyTask);
			return eScheduleWorkItemStatus_Finished;
		}
		else
		{
			SAFE_RELEASE(m_pCopyTask);
			return eScheduleWorkItemStatus_Failed;
		}
	}

	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_LaunchExecutable::CBGScheduleWork_LaunchExecutable(CEditorHostInfo* pHost, const CString& directory, const CString& executable, const CString& args)
	: m_pTarget(pHost->GetPlatform())
	, m_args(args)
	, m_directory(directory)
	, m_executable(executable)
{
	char szShortFileName[_MAX_FNAME];
	_splitpath(executable, NULL, NULL, szShortFileName, NULL);

	m_name = "Launching '";
	m_name += szShortFileName;
	m_name += "' on '";
	m_name += m_pTarget->GetTargetName();
	m_name += "'";

	m_pTarget->AddRef();
}

CBGScheduleWork_LaunchExecutable::~CBGScheduleWork_LaunchExecutable()
{
	m_pTarget->Release();
}

const char* CBGScheduleWork_LaunchExecutable::GetDescription() const
{
	return m_name;
}

float CBGScheduleWork_LaunchExecutable::GetProgress() const
{
	return 0.0f;
}

bool CBGScheduleWork_LaunchExecutable::OnStart()
{
	m_pLaunchTask = new CBGTask_LaunchExecutable(m_pTarget, m_directory, m_executable, m_args);
	GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pLaunchTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
	m_pLaunchTask->AddRef();
	return true;
}

bool CBGScheduleWork_LaunchExecutable::OnStop()
{
	if (NULL != m_pLaunchTask)
	{
		m_pLaunchTask->Cancel();
		m_pLaunchTask->Release();
		m_pLaunchTask = NULL;
	}

	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_LaunchExecutable::OnUpdate()
{
	if (m_pLaunchTask->HasFinished())
	{
		if (m_pLaunchTask->HasFinishedWithoutError())
		{
			SAFE_RELEASE(m_pLaunchTask);
			return eScheduleWorkItemStatus_Finished;
		}
		else
		{
			SAFE_RELEASE(m_pLaunchTask);
			return eScheduleWorkItemStatus_Failed;
		}
	}

	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_Connect::CBGScheduleWork_Connect(CEditorHostInfo* pHost, const uint32 retryCount, const float reconnectionDelay)
	: m_pHost(pHost)
	, m_retryCount(retryCount)
	, m_retryDelay(reconnectionDelay)
	, m_pConnectTask(NULL)
	, m_currentRetry(0)
{
	m_pHost->AddRef();

	m_name = "Connecting to '";
	m_name += m_pHost->GetTargetName();
	m_name += "'";
}

CBGScheduleWork_Connect::~CBGScheduleWork_Connect()
{
	SAFE_RELEASE(m_pHost);
	
	if (NULL != m_pConnectTask)
	{
		m_pConnectTask->Cancel();
		m_pConnectTask->Release();
		m_pConnectTask = NULL;
	}
}

const char* CBGScheduleWork_Connect::GetDescription() const
{
	return m_name;
}

float CBGScheduleWork_Connect::GetProgress() const
{
	return m_currentRetry / (float)m_retryCount;
}

bool CBGScheduleWork_Connect::OnStart()
{
	// always reconnect (helps to preserve connection health)
	m_pHost->GetHostInfo()->Disconnect();

	// if already connected our job is done
	m_pConnectTask = new CBGTask_Connect(m_pHost->GetHostInfo());
	m_pConnectTask->AddRef();
	GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pConnectTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
	return true;
}

bool CBGScheduleWork_Connect::OnStop()
{
	if (NULL != m_pConnectTask)
	{
		m_pConnectTask->Cancel();
		m_pConnectTask->Release();
		m_pConnectTask = NULL;
	}

	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_Connect::OnUpdate()
{
	if (m_pConnectTask != NULL)
	{
		if (m_pConnectTask->HasFinished())
		{
			if (m_pConnectTask->HasFinishedWithoutError())
			{
				SAFE_RELEASE(m_pConnectTask);
				return eScheduleWorkItemStatus_Finished;
			}
			else
			{
				SAFE_RELEASE(m_pConnectTask);

				if (m_currentRetry == m_retryCount)
				{
					// we failed to connect and there are no more reconnection attempts left
					return eScheduleWorkItemStatus_Failed;
				}
				else
				{
					// schedule next retry
					m_nextRetry = gEnv->pTimer->GetAsyncTime() + m_retryDelay;
					m_currentRetry += 1;
				}
			}
		}
	}
	else
	{
		// create new connection task
		if (gEnv->pTimer->GetAsyncTime() >= m_nextRetry)
		{
			m_pConnectTask = new CBGTask_Connect(m_pHost->GetHostInfo());
			m_pConnectTask->AddRef();
			GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pConnectTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
		}
	}

	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_WaitForTarget::CBGScheduleWork_WaitForTarget(CEditorHostInfo* pHost)
	: m_pHost(pHost)
	, m_maxTime(60.0f)
	, m_startTime(gEnv->pTimer->GetAsyncTime())
{
	m_pHost->AddRef();
}

CBGScheduleWork_WaitForTarget::~CBGScheduleWork_WaitForTarget()
{
	SAFE_RELEASE(m_pHost);
}

const char* CBGScheduleWork_WaitForTarget::GetDescription() const
{
	return "Waiting for target";
}

float CBGScheduleWork_WaitForTarget::GetProgress() const
{
	const float timePassed = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(m_startTime);
	return timePassed / m_maxTime;
}

bool CBGScheduleWork_WaitForTarget::OnStart()
{
	return true;
}

bool CBGScheduleWork_WaitForTarget::OnStop()
{
	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_WaitForTarget::OnUpdate()
{
	// timeout
	const float timePassed = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(m_startTime);
	if (timePassed > m_maxTime)
	{
		return eScheduleWorkItemStatus_Failed;
	}

	// wait for valid connection and target that is not loading a level
	if (m_pHost->IsReady())
	{
		return eScheduleWorkItemStatus_Finished;
	}

	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_ExecuteRemoteConsoleCommand::CBGScheduleWork_ExecuteRemoteConsoleCommand(const CString& address, const CString& command, const uint retryCount/*=10*/, const float waitTime/*=1.0f*/)
	: m_address(address)
	, m_command(command)
	, m_pTask(NULL)
	, m_waitTime(waitTime)
	, m_retryCount(retryCount)
{
	m_name = "Execute '";
	m_name += m_command;
	m_name += "'";
}

CBGScheduleWork_ExecuteRemoteConsoleCommand::~CBGScheduleWork_ExecuteRemoteConsoleCommand()
{
}

const char* CBGScheduleWork_ExecuteRemoteConsoleCommand::GetDescription() const
{
	return m_name;
}

float CBGScheduleWork_ExecuteRemoteConsoleCommand::GetProgress() const
{
	return 0.0f;
}

bool CBGScheduleWork_ExecuteRemoteConsoleCommand::OnStart()
{
	// if already connected our job is done
	m_pTask = new CBGTask_ExecuteCommand(m_address, m_command, m_waitTime, m_retryCount);
	m_pTask->AddRef();
	GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
	return true;
}

bool CBGScheduleWork_ExecuteRemoteConsoleCommand::OnStop()
{
	if (NULL != m_pTask)
	{
		m_pTask->Cancel();
		m_pTask->Release();
		m_pTask = NULL;
	}

	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_ExecuteRemoteConsoleCommand::OnUpdate()
{
	if (m_pTask != NULL)
	{
		if (m_pTask->HasFinished())
		{
			if (m_pTask->HasFinishedWithoutError())
			{
				SAFE_RELEASE(m_pTask);
				return eScheduleWorkItemStatus_Finished;
			}
			else
			{
				SAFE_RELEASE(m_pTask);
				return eScheduleWorkItemStatus_Failed;
			}
		}
	}

	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

CBGScheduleWork_ToggleLiveCreateCommands::CBGScheduleWork_ToggleLiveCreateCommands(const CString& address, bool bSuppress)
	: CBGScheduleWork_ExecuteRemoteConsoleCommand(address, bSuppress ? "SuppressCommands" : "ResumeCommands", 10, 2.0f)
	, m_bSuppress(bSuppress)
{
}

CBGScheduleWork_ToggleLiveCreateCommands::~CBGScheduleWork_ToggleLiveCreateCommands()
{
}

const char* CBGScheduleWork_ToggleLiveCreateCommands::GetDescription() const
{
	return m_bSuppress ? "Disabling LiveCreate commands" : "Enabling LiveCreate commands";
}

//-----------------------------------------------------------------------------

CBGScheduleWork_EnableLiveCreate::CBGScheduleWork_EnableLiveCreate()
	: m_launchDelayTime(1.0f)
	, m_totalDelayTime(2.0f)
	, m_bEnabled(false)
{
}

CBGScheduleWork_EnableLiveCreate::~CBGScheduleWork_EnableLiveCreate()
{
}

const char* CBGScheduleWork_EnableLiveCreate::GetDescription() const
{
	return "Enable LiveCreate";
}

float CBGScheduleWork_EnableLiveCreate::GetProgress() const
{
	return 0.0f;
}

bool CBGScheduleWork_EnableLiveCreate::OnStart()
{
	GetIEditor()->GetLiveCreate()->SetEnabled(true);

	m_startTime = gEnv->pTimer->GetAsyncTime();

	return true;
}

EScheduleWorkItemStatus CBGScheduleWork_EnableLiveCreate::OnUpdate()
{
	const float timePassed = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(m_startTime);
	if (timePassed >= m_launchDelayTime && !m_bEnabled)
	{
		m_bEnabled = true;

		if (!GetIEditor()->GetLiveCreate()->SyncEnableLiveCreate())
		{
			return eScheduleWorkItemStatus_Failed;
		}
	}

	if (timePassed > m_totalDelayTime)
	{
		return eScheduleWorkItemStatus_Finished;
	}
		
	return eScheduleWorkItemStatus_NotFinished;
}

//-----------------------------------------------------------------------------

bool CEditorManager::StartLiveCreate(CString& outReasonToFail, bool bQuick)
{
	// already requested a start
	if (m_bRequestedToStartLiveCreate || NULL != m_pStartingSchedule)
	{
		outReasonToFail = "There is already one LiveCreate process starting";
		return false;
	}

	// no level loaded
	if (GetIEditor()->GetLevelFolder().IsEmpty())
	{
		outReasonToFail = "There is no level loaded. LiveCreate requires loaded level to start.";
		return false;
	}

	// get level name
	const string strLevelFolder(GetIEditor()->GetLevelFolder());
	gEnv->pLog->LogAlways("Current level: '%s'", strLevelFolder.c_str());

	// Filter hosts to keep only the enabled ones with valid IP
	THostList validHosts;
	for (THostList::const_iterator it = m_pHosts.begin();
		it != m_pHosts.end(); ++it)
	{
		CEditorHostInfo* pHost = (*it);
		if (pHost->IsEnabled() && pHost->HasValidAddress() && pHost->IsOn())
		{
			validHosts.push_back(pHost);
		}
	}

	// No hosts to work on
	if (validHosts.empty())
	{
		outReasonToFail = "There are no selected console targets capable of running LiveCreate right now.";
		return false;
	}

	// Request status update from all of the valid hosts so we know about the game running
	m_pStartingHosts.clear();
	for (THostList::const_iterator it = validHosts.begin();
		it != validHosts.end(); ++it)
	{
		CEditorHostInfo* pHost = (*it);
		if (!pHost->IsUpdatingHostInfo())
		{
			pHost->RequestHostInfoUpdate();
		}

		m_pStartingHosts.push_back(pHost);
	}

	// Signal that we requested LiveCreate start - it can happen once we know the status of all the hosts
	m_bRequestedToStartLiveCreate = true;
	return true;
}

void CEditorManager::FinishLiveCreateStart()
{
	// clear flag
	m_bRequestedToStartLiveCreate = false;

	// LiveCreate startup work order:
	//
	//  For consoles that are not running the game yet:
	//  - Reset targets
	//  - Export Level&Terrain (depends on the settings)
	//  - Copy exported level to consoles
	//  - Launch game
	//
	// Common steps:
	//  - (Re)connect with consoles
	//  - Build level sync packet
	//  - Send level sync packet data
	//  - Enable LiveCreate on targets (triggers consuming of the file sync data)	

	// use the initial host list
	THostList validHosts;
	std::swap(validHosts, m_pStartingHosts);

	// create the launch schedule
	IBackgroundSchedule* pSchedule = GetIEditor()->GetBackgroundScheduleManager()->CreateSchedule("Starting LiveCreate");

	// Do we have any "fresh" console - without a game running ?
	THostList freshConsoles, levelLoadingConsoles;
	bool bWillBeLoadingLevel = false;
	for (THostList::const_iterator it = validHosts.begin();
		it != validHosts.end(); ++it)
	{
		CEditorHostInfo* pHost = (*it);
		if (pHost->HasValidAddress())
		{
			// if the game running ?
			if (!pHost->HasValidHostInfoPacket())
			{	
				// console has no build running and no build selected to launch - that's an error
				if (pHost->GetBuildExecutable().IsEmpty())
				{
					gEnv->pLog->LogWarning("Target '%s' (platform '%s') has no build running and no build selected for auto start. Skipping.", pHost->GetTargetName(), pHost->GetPlatform()->GetPlatformName());
					continue;
				}

				// add to list of fresh consoles that require booting
				freshConsoles.push_back(pHost);
				levelLoadingConsoles.push_back(pHost);
				bWillBeLoadingLevel = true;
			}
			else
			{
				// if the level on the console is not the same as our current level than we need to reload it
				const string strCurrentLevel(Path::GetFileName(GetIEditor()->GetLevelFolder()));
				const string strHostLevel(Path::GetFileName(pHost->GetHostInfoPacket().currentLevel.c_str()));
				if (0 != stricmp(strCurrentLevel.c_str(), strHostLevel.c_str()))
				{
					if (pHost->GetHostInfoPacket().currentLevel.empty())
					{
						gEnv->pLog->LogWarning("Target '%s' (platform '%s') has no level loaded. Current level '%s' will be loaded.", 
							pHost->GetTargetName(), pHost->GetPlatform()->GetPlatformName(), strCurrentLevel.c_str());
					}
					else
					{
						gEnv->pLog->LogWarning("Target '%s' (platform '%s') has level '%s' loaded. Current level '%s' will be loaded instead.", 
							pHost->GetTargetName(), pHost->GetPlatform()->GetPlatformName(), 
							pHost->GetHostInfoPacket().currentLevel.c_str(), strCurrentLevel.c_str());
					}

					levelLoadingConsoles.push_back(pHost);
					bWillBeLoadingLevel = true;
				}
				else
				{
					if (GetGeneralSettings().bAlwaysReloadLevel)
					{
						gEnv->pLog->LogWarning("Target '%s' (platform '%s') has level '%s' loaded. Level will be reloaded anyway.", 
							pHost->GetTargetName(), pHost->GetPlatform()->GetPlatformName(), 
							pHost->GetHostInfoPacket().currentLevel.c_str());

						levelLoadingConsoles.push_back(pHost);
						bWillBeLoadingLevel = true;
					}
				}
			}
		}
	}

	// consoles without running builds are reset
	if (GetGeneralSettings().bResetBeforeStart)
	{
		IBackgroundScheduleItem* pResetTask = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Resetting targets");
		for (THostList::const_iterator it = freshConsoles.begin();
			it != freshConsoles.end(); ++it)
		{
			CEditorHostInfo* pHost = (*it);
			if (pHost->GetPlatform()->IsFlagSet(IPlatformHandler::eFlag_RequiresResetBeforeLaunch))
			{
				CBGScheduleWork_ResetTarget* pResetTarget = new CBGScheduleWork_ResetTarget(pHost);
				pResetTask->AddWorkItem(pResetTarget);
			}
		}

		pSchedule->AddItem(pResetTask);
		pResetTask->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// level & terrain export
	if (bWillBeLoadingLevel)
	{
		// one task to export the level
		if (GetGeneralSettings().bExportLevelOnStart || GetGeneralSettings().bExportTerrainOnStart)
		{
			IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Export level");
			CBGScheduleWork_ExportLevel* pWork = new CBGScheduleWork_ExportLevel(GetGeneralSettings().bExportLevelOnStart, GetGeneralSettings().bExportTerrainOnStart);
			pSchedule->AddItem(pScheduleItem);
			pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
		}
	}

	// launch game on fresh targets (the ones that were reset)
	{
		// extract options
		const bool bDisableAI = GetIEditor()->GetLiveCreate()->GetAdvancedSettings().bDisableAI;
		const bool bGodMode = GetIEditor()->GetLiveCreate()->GetAdvancedSettings().bGodMode;
		const bool bMemReplay = GetIEditor()->GetLiveCreate()->GetAdvancedSettings().bMemReplay;

		// Format launching arguments
		CString args;
		args.Format("+e_objectlayersactivation 0 "
			"+es_enablepooluse 0 "
			"+ai_ignoreplayer %i "
			"+ai_noupdate %i "
			"+con_restricted 0 "
			"+sys_pakpriority 0 "
			"+g_godmode %i "
			"+sys_paklogmissingfiles 0 "
			"+sys_pakloginvalidfileaccess 0 "
			"+sys_pakmessageinvalidfileaccess 0 "
			"+sys_ai %i ",
			bDisableAI ? 1 : 0,
			bDisableAI ? 1 : 0,
			bGodMode ? 1 : 0,
			bDisableAI ? 0 : 1,
			bMemReplay ? " -memreplay" : "");

		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Launching game");
		for (THostList::const_iterator it = freshConsoles.begin();
			it != freshConsoles.end(); ++it)
		{
			CEditorHostInfo* pHost = (*it);

			const CString& buildDirectory = pHost->GetBuildDirectory();
			const CString& buildExecutable = pHost->GetBuildExecutable();
			CBGScheduleWork_LaunchExecutable* pWork = new CBGScheduleWork_LaunchExecutable(pHost, buildDirectory, buildExecutable, args);
			pScheduleItem->AddWorkItem(pWork);
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// connect/reconnect
	{
		const float retryDelay = GetAdvancedSettings().fReconnectionDelay;
		const int retryCount = GetAdvancedSettings().iReconnectionCount;

		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Connecting to targets");
		for (THostList::const_iterator it = validHosts.begin();
			it != validHosts.end(); ++it)
		{
			CBGScheduleWork_Connect* pWork = new CBGScheduleWork_Connect(*it, retryCount, retryDelay);
			pScheduleItem->AddWorkItem(pWork);
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// disable live create on all targets
	{
		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Resetting LiveCreate");
		for (THostList::const_iterator it = validHosts.begin();
			it != validHosts.end(); ++it)
		{
			CBGScheduleWork_ToggleLiveCreateCommands* pWork = new CBGScheduleWork_ToggleLiveCreateCommands((*it)->GetAddres(), true);
			pScheduleItem->AddWorkItem(pWork);
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// copy level files to the target consoles
	{
		const CString strLevelFolder(CString(GetIEditor()->GetLevelFolder()).MakeLower());

		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Copying level data to targets");
		for (THostList::const_iterator it = levelLoadingConsoles.begin();
			it != levelLoadingConsoles.end(); ++it)
		{
			// we do not need to copy local data
			if (!(*it)->GetPlatform()->IsFlagSet(IPlatformHandler::eFlag_SharedDataDirectory))
			{
				const CString srcFilePath = strLevelFolder + "/level.pak";
				const CString destFilePath = strLevelFolder + "/_level.pak";

				CBGScheduleWork_CopyFileToTarget* pWork = new CBGScheduleWork_CopyFileToTarget((*it), srcFilePath, destFilePath);
				pScheduleItem->AddWorkItem(pWork);
			}
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// copy the terrain file
	{
		const CString strLevelFolder(CString(GetIEditor()->GetLevelFolder()).MakeLower());

		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Copying terrain data to targets");
		for (THostList::const_iterator it = levelLoadingConsoles.begin();
			it != levelLoadingConsoles.end(); ++it)
		{
			// we do not need to copy local data
			if (!(*it)->GetPlatform()->IsFlagSet(IPlatformHandler::eFlag_SharedDataDirectory))
			{
				const CString srcFilePath = strLevelFolder + "/terraintexture.pak";
				const CString destFilePath = strLevelFolder + "/_terraintexture.pak";
			
				CBGScheduleWork_CopyFileToTarget* pWork = new CBGScheduleWork_CopyFileToTarget((*it), srcFilePath, destFilePath);
				pScheduleItem->AddWorkItem(pWork);
			}
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// load level
	{
		const CString strLevelFolder(CString(GetIEditor()->GetLevelFolder()).MakeLower());
		const CString strMap(Path::GetFileName(strLevelFolder));

		const CString cmd = CString("map ") + strMap;

		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Wait for consoles");
		for (THostList::const_iterator it = levelLoadingConsoles.begin();
			it != levelLoadingConsoles.end(); ++it)
		{
			CBGScheduleWork_ExecuteRemoteConsoleCommand* pWork = new CBGScheduleWork_ExecuteRemoteConsoleCommand((*it)->GetAddres(), cmd);
			pScheduleItem->AddWorkItem(pWork);
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// wait for targets that were loading level
	{
		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Wait for consoles");
		for (THostList::const_iterator it = levelLoadingConsoles.begin();
			it != levelLoadingConsoles.end(); ++it)
		{
			CBGScheduleWork_WaitForTarget* pWork = new CBGScheduleWork_WaitForTarget(*it);
			pScheduleItem->AddWorkItem(pWork);
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// resume command system
	{
		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Resuming LiveCreate interface");
		for (THostList::const_iterator it = validHosts.begin();
			it != validHosts.end(); ++it)
		{
			CBGScheduleWork_ToggleLiveCreateCommands* pWork = new CBGScheduleWork_ToggleLiveCreateCommands((*it)->GetAddres(), false);
			pScheduleItem->AddWorkItem(pWork);
		}

		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// finished, enable live create
	{
		IBackgroundScheduleItem* pScheduleItem = GetIEditor()->GetBackgroundScheduleManager()->CreateScheduleItem("Enabling LiveCreate");
		CBGScheduleWork_EnableLiveCreate* pWork = new CBGScheduleWork_EnableLiveCreate();
		pScheduleItem->AddWorkItem(pWork);
		pSchedule->AddItem(pScheduleItem);
		pScheduleItem->Release(); // do not keep local reference (it's all in the schedule now)
	}

	// initial listeners update
	for (TListenerList::const_iterator it=m_pListeners.begin();
		it != m_pListeners.end(); ++it)
	{
		(*it)->OnLiveCreateStarting(0.0f);
	}

	// submit the work schedule
	m_pStartingSchedule = pSchedule;
	GetIEditor()->GetBackgroundScheduleManager()->SubmitSchedule(pSchedule);
}

//-----------------------------------------------------------------------------

} // namespace

#endif
