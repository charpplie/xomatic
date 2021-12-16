////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LiveCreate/EditorLiveCreateHostInfo.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "EditorLiveCreateTasks.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

//-----------------------------------------------------------------------------

CEditorHostInfo::CEditorHostInfo(CEditorManager* pManager, IPlatformHandlerFactory* pPlatformFactory, const CString& targetName, const CString& lastKnownAddress)
	: m_pManager(pManager)
	, m_pPlatformFactory(pPlatformFactory)
	, m_pResolveAddressTask(NULL)
	, m_pHost(NULL)
	, m_targetName(targetName)
	, m_validAddress(lastKnownAddress)
	, m_lastAddressResolveTaskTime(0)
	, m_bIsEnabled(true)
	, m_hostInfoPacketUpdateTime((int64)0)
	, m_pHostInfoPacketTask(NULL)
{
	// Create a generic platform handler for the host, this should be always possible
	m_pPlatform = m_pPlatformFactory->CreatePlatformHandlerInstance(targetName);

	// Valid address was already provided
	if (!m_validAddress.IsEmpty())
	{
		m_pHost = gEnv->pLiveCreateManager->CreateHost(m_pPlatform, (const char*)m_validAddress);
		RequestHostInfoUpdate();
	}

	// Start a background task to resolve console target name to some sensible address as soon as possible
	UpdateBackgroundTasks();
}

CEditorHostInfo::~CEditorHostInfo()
{
	CancelAllTasks();	

	// release host info
	if (NULL != m_pHost)
	{
		m_pHost->Disconnect();
		m_pHost->Release();
		m_pHost = NULL;
	}

	// release the platform handler
	if (NULL != m_pPlatform)
	{
		m_pPlatform->Release();
		m_pPlatform = NULL;
	}
}

bool CEditorHostInfo::IsOn() const
{
	if (NULL != m_pPlatform)
	{
		// the power-
		if (m_pPlatform->IsFlagSet(IPlatformHandler::eFlag_HasSlowPowerStateCheck))
		{
			return m_pManager->IsPlatformOn(m_pPlatform);
		}
		else
		{
			return m_pPlatform->IsOn();
		}
	}

	// invalid platform, assume it's not on
	return false;
}

EHostStatus CEditorHostInfo::EvaluateStatus() const
{
	EHostStatus status = eHostStatus_Dead;

	// see the description of EHostStatus enum to understand the evaluation method.
	if (NULL != m_pPlatform && IsEnabled())
	{
		if (IsOn())
		{
			if ((NULL != m_pHost) && m_pHost->IsConnected())
			{
				if (m_pHost->IsReady())
				{
					status = eHostStatus_Ready;
				}
				else
				{
					status = eHostStatus_Connected;
				}
			}
			else
			{
				status = eHostStatus_Online;
			}
		}
		else
		{
			status = eHostStatus_Offline;
		}
	}

	return status;
}

void CEditorHostInfo::Enable(bool bFlag)
{
	if (m_bIsEnabled != bFlag)
	{
		m_bIsEnabled = bFlag;
		GetIEditor()->GetLiveCreate()->SaveSettings();

		// we do not need background tasks when we are disabled
		if (!m_bIsEnabled)
		{
			if (NULL != m_pHost)
			{
				m_pHost->Disconnect();
			}

			CancelAllTasks();
		}
	}
}

bool CEditorHostInfo::IsConnected() const
{
	return (NULL!=m_pHost) && m_pHost->IsConnected();
}

bool CEditorHostInfo::IsReady() const
{
	return (NULL!=m_pHost) && m_pHost->IsConnected() && m_pHost->IsReady();
}

void CEditorHostInfo::CancelAllTasks()
{
	if (NULL != m_pResolveAddressTask)
	{
		m_pResolveAddressTask->Cancel();
		m_pResolveAddressTask->Release();
		m_pResolveAddressTask = NULL;
	}

	if (NULL != m_pHostInfoPacketTask)
	{
		m_pHostInfoPacketTask->Cancel();
		m_pHostInfoPacketTask->Release();
		m_pHostInfoPacketTask = NULL;
	}
}

bool CEditorHostInfo::Screenshot(CString& outErrorString)
{

	return true;
}

void CEditorHostInfo::SetAddress(const CString& address)
{
	m_validAddress = address;
}

void CEditorHostInfo::SetBuild(const CString& buildDirectory, const CString& buildExecutable)
{
	m_buildDirectory = buildDirectory;
	m_buildExecutable = buildExecutable;
}

void CEditorHostInfo::RequestHostInfoUpdate()
{
	CryAutoLock<CryMutex> lock(m_lock);

	if (!m_validAddress.IsEmpty() && NULL == m_pHostInfoPacketTask)
	{
		m_pHostInfoPacketTask = new CBGTask_GetHostInfoPacket(m_validAddress);
		m_pHostInfoPacketTask->AddRef();
		GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pHostInfoPacketTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
	}
}

void CEditorHostInfo::UpdateBackgroundTasks()
{
	CryAutoLock<CryMutex> lock(m_lock);

	// Get current time
	const int64 currentTime = gEnv->pTimer->GetAsyncTime().GetMilliSecondsAsInt64();

	// Background address resolving
	if (m_validAddress.IsEmpty())
	{
		if (NULL == m_pResolveAddressTask)
		{
			// schedule new address resolving task, only if we are enabled
			if ( m_bIsEnabled && ((currentTime - m_lastAddressResolveTaskTime) > kAddressResolveTimer) )
			{
				m_pResolveAddressTask = new CBGTask_ResolveAddress(m_pPlatformFactory, m_targetName);
				m_pResolveAddressTask->AddRef();// bullshit - needed because the refcounted object initial reference count is 0...
				GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pResolveAddressTask, eTaskPriority_BackgroundScan, eTaskThreadMask_Any);
			}
		}
		else if (m_pResolveAddressTask->HasFinished())
		{
			// get the resolved address
			if (!m_pResolveAddressTask->GetResolvedAddress().IsEmpty())
			{
				// remember the resolved address
				m_validAddress = m_pResolveAddressTask->GetResolvedAddress();

				// create host object
				m_pHost = gEnv->pLiveCreateManager->CreateHost(m_pPlatform, (const char*)m_validAddress);

				// try to get the host info
				RequestHostInfoUpdate();
			}

			// release
			m_pResolveAddressTask->Release();
			m_pResolveAddressTask = NULL;

			// update time limit
			m_lastAddressResolveTaskTime = currentTime;
		}
	}

	// Update the host info packet (checks if the game is running)
	if (NULL != m_pHostInfoPacketTask)
	{
		if (m_pHostInfoPacketTask->HasFinished())
		{
			// valid data received
			if (m_pHostInfoPacketTask->HasFinishedWithoutError())
			{
				m_bHostInfoPacketValid = true;
				m_hostInfoPacket = m_pHostInfoPacketTask->GetInfoPacket();

				m_pManager->LogMessagef(eLogType_Normal, "Host info packet from '%s' (%s) received:", (const char*)m_targetName, (const char*)m_validAddress);
				m_pManager->LogMessagef(eLogType_Normal, "True host name: '%s'", m_hostInfoPacket.hostName.c_str());
				m_pManager->LogMessagef(eLogType_Normal, "Host game dir: '%s'", m_hostInfoPacket.gameFolder.c_str());
				m_pManager->LogMessagef(eLogType_Normal, "Build executable: '%s'", m_hostInfoPacket.buildExecutable.c_str());
				m_pManager->LogMessagef(eLogType_Normal, "Build directory: '%s'", m_hostInfoPacket.buildDirectory.c_str());				
				m_pManager->LogMessagef(eLogType_Normal, "Host root dir: '%s'", m_hostInfoPacket.rootFolder.c_str());
				m_pManager->LogMessagef(eLogType_Normal, "Current level: '%s'", m_hostInfoPacket.currentLevel.c_str());
				m_pManager->LogMessagef(eLogType_Normal, "Host platform name: '%s'", m_hostInfoPacket.platformName.c_str());
				m_pManager->LogMessagef(eLogType_Normal, "Host resolution: %dx%d", m_hostInfoPacket.screenWidth, m_hostInfoPacket.screenHeight);				
				m_pManager->LogMessagef(eLogType_Normal, "Has LiveCreate: %d", m_hostInfoPacket.bHasLiveCreateConnection);
				m_pManager->LogMessagef(eLogType_Normal, "Is LiveCreate allowed: %d", m_hostInfoPacket.bAllowsLiveCreate);
			}
			else
			{
				m_bHostInfoPacketValid = false;
			}

			m_hostInfoPacketUpdateTime = gEnv->pTimer->GetAsyncTime();
			SAFE_RELEASE(m_pHostInfoPacketTask);
		}
	}
	else if (!m_validAddress.IsEmpty() && NULL == m_pHostInfoPacketTask)
	{
		const float kHostInfoPacketRequestTime = 10.0f;
		const uint32 timePassed = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(m_hostInfoPacketUpdateTime);
		if (timePassed > kHostInfoPacketRequestTime)
		{
			m_pHostInfoPacketTask = new CBGTask_GetHostInfoPacket(m_validAddress);
			m_pHostInfoPacketTask->AddRef();
			GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pHostInfoPacketTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
		}
	}

	// If the host crashed or got turned off than close all connections
	if (!IsOn() && IsConnected())
	{
		m_pHost->Disconnect();
	}
}

CEditorHostInfo* CEditorHostInfo::LoadFromXML(CEditorManager* pManager, XmlNodeRef node)
{
	// load config
	bool bIsEnabled = false;
	CString targetName = "";
	CString platformName = "";
	CString address = "";
	CString buildExecutable = "";
	CString buildDirectory = "";
	CString lastKnownAddress = "";
	node->getAttr("bEnabled", bIsEnabled);
	node->getAttr("targetName", targetName);
	node->getAttr("address", address);
	node->getAttr("platformName", platformName);
	node->getAttr("lastKnownAddress", lastKnownAddress);
	node->getAttr("buildExecutable", buildExecutable);
	node->getAttr("buildDirectory", buildDirectory);

	// invalid target name
	if (targetName.IsEmpty())
	{
		return NULL;
	}

	// find matching platform
	IPlatformHandlerFactory* pFactory = NULL;
	for (uint i=0; i<pManager->GetNumPlatforms(); ++i)
	{
		IPlatformHandlerFactory* pTestFactory = pManager->GetPlatform(i);
		if (0 == stricmp(pTestFactory->GetPlatformName(), (const char*)platformName))
		{
			pFactory = pTestFactory;
			break;
		}
	}

	// no matching platform factory found, we can't restore this host configuration
	if (NULL == pFactory)
	{
		return NULL;
	}

	// create an editor side wrapping object for the provided host settings
	CEditorHostInfo* pObject = new CEditorHostInfo(pManager, pFactory, targetName, lastKnownAddress);
	pObject->m_bIsEnabled = bIsEnabled;
	pObject->m_buildExecutable = buildExecutable;

	return pObject;
}

void CEditorHostInfo::SaveToXML(XmlNodeRef node) const
{
	node->setAttr("bEnabled", m_bIsEnabled);
	node->setAttr("targetName", (const char*)m_targetName);
	node->setAttr("platformName", m_pPlatformFactory->GetPlatformName());
	node->setAttr("buildExecutable", (const char*)m_buildExecutable);
	node->setAttr("buildDirectory", (const char*)m_buildDirectory);
	node->setAttr("lastKnownAddress", (const char*)m_validAddress);
}

} // namespace LiveCreate

#endif