////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LiveCreate/EditorLiveCreateHostInfo.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "LiveCreate/EditorLiveCreateAreaSync.h"
#include "LiveCreate/EditorLiveCreateFileSync.h"
#include "EditorLiveCreateTasks.h"
#include "EditorLiveCreateSchedule.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

CGeneralSettings::CGeneralSettings()
{
	AddVar("bResetBeforeStart", "Reset before starting LC", bResetBeforeStart, true);
	AddVar("bFastTargetReset", "Fast reset (title)", bFastTargetReset, true);
	AddVar("bExportLevelOnStart", "Export level to targets", bExportLevelOnStart, true);
	AddVar("bExportTerrainOnStart", "Export terrain to targets", bExportTerrainOnStart, false);
	AddVar("bShowSelectionBoxes", "Show selection boxes", bShowSelectionBoxes, true);
	AddVar("bShowSelectionNames", "Show selection names", bShowSelectionNames, false);
	AddVar("bSyncCameraFromGame", "Sync camera in game", bSyncCameraFromGame, false);
	AddVar("bAlwaysReloadLevel", "Always reload level", bAlwaysReloadLevel, false);	
	AddVar("bSyncFileUpdates", "Sync changed files", bSyncFileUpdates, true);	
}

CAdvancedSettings::CAdvancedSettings()
{
	AddVar("bEnableLog", "Enable log output", bEnableLog, false);
	AddVar("bGodMode", "God mode", bGodMode, true);
	AddVar("bDisableAI", "Disable AI", bDisableAI, true);
	AddVar("bDisableScripts", "Disable Scripts", bDisableScripts, true);
	AddVar("bSkipCinematics", "Skip in-game cinematics", bSkipCinematics, true);
	AddVar("bMemReplay", "Run with MemReplay", bMemReplay, false);
	AddVar("bAutoConnect", "Auto connect after start", bAutoConnect, true);
	AddVar("bResumePhysicsOnDisconnect", "Resume physics when disconnected", bResumePhysicsOnDisconnect, true);
	AddVar("bAutoWakeUpPhysicalObjects", "Wakeup physical objects when disconnected", bAutoWakeUpPhysicalObjects, false);
	AddVar("bAlwaysSyncFullEntities", "Always use full entity sync", bAlwaysSyncFullEntities, false);
	AddVar("bUseCRCWithEntitySync", "Send entity CRC", bUseCRCWithEntitySync, true);
	AddVar("fArchetypeSyncTime", "Archetype sync time (seconds)", fArchetypeSyncTime, 1.0f);
	AddVar("fParticleSyncTime", "Particle sync time (seconds)", fParticleSyncTime, 1.0f);
	AddVar("fSlowResetWaitTime", "Slow reset wait time (seconds)", fSlowResetWaitTime, 30.0f);
	AddVar("fFastResetWaitTime", "Fast reset wait time (seconds)", fFastResetWaitTime, 3.0f);
	AddVar("fObjectsSyncTime", "Object sync time (seconds)", fObjectsSyncTime, 0.05f);
	AddVar("iReconnectionCount", "Connection retry count", iReconnectionCount, 30);
	AddVar("fReconnectionDelay", "Connection retry delay (seconds)", fReconnectionDelay, 1.0f);
}

//-----------------------------------------------------------------------------

/// Thread that checks the power on status of platforms in the background
class CPowerStatusThread : public CryThread<CPowerStatusThread>
{
private:
	CEditorManager* m_pManager;

public:
	CPowerStatusThread(CEditorManager* pManager)
		: m_pManager(pManager)
	{
	}

	virtual void Run()
	{
		CryThreadSetName(-1, "LiveCreatePowerStatusThread");

		while (IsStarted())
		{
			std::vector<CEditorHostInfo*> hosts;
			m_pManager->GetHosts(hosts);

			const uint numHosts = hosts.size();
			for (uint i=0; i<numHosts; ++i)
			{
				CEditorHostInfo* pHost = hosts[i];

				LiveCreate::IPlatformHandler* pHandler = pHost->GetPlatform();
				if (NULL != pHandler && pHandler->IsFlagSet(LiveCreate::IPlatformHandler::eFlag_HasSlowPowerStateCheck))
				{
					const bool bOn = pHandler->IsOn();

					{
						CryAutoLock<CryMutex> lock(m_lock);
						m_powerStatus[pHandler] = bOn;
					}
				}

				pHost->Release();
			}

			Sleep(500);
		}
	}

	bool IsPoweredOn(IPlatformHandler* pHandler) const
	{
		CryAutoLock<CryMutex> lock(m_lock);
		auto iter = m_powerStatus.find(pHandler);

		if (iter == m_powerStatus.end())
		{
			return false;
		}

		return iter->second;
	}

protected:
	CryMutex m_lock;
	std::map<IPlatformHandler*, bool> m_powerStatus;
};

//-----------------------------------------------------------------------------

CEditorManager* g_EditorManager = NULL;

//-----------------------------------------------------------------------------

CEditorManager::CEditorManager()
	: m_pManager(NULL)
	, m_pPowerStatusThread(NULL)
	, m_pObjectSync(NULL)
	, m_pFileSyncManager(NULL)
	, m_bParticlesDirty(false)
	, m_bArchetypesDirty(false)
	, m_bIsLoadingObjects(false)
	, m_lastCameraSyncPosition(0,0,0)
	, m_lastCameraSyncRotation(Quat::CreateIdentity())
	, m_lastCameraSyncFOV(50.0f)
	, m_lastCameraKnownPosition(0,0,0)
	, m_lastCameraKnownRotation(Quat::CreateIdentity())
	, m_lastCameraKnownFOV(50.0f)
	, m_bRequestedToStartLiveCreate(false)
	, m_pStartingSchedule(NULL)
{
	GetIEditor()->RegisterNotifyListener(this);

	m_pGeneralSettings = new CGeneralSettings();
	m_pAdvancedSettings = new CAdvancedSettings();
}

CEditorManager::~CEditorManager()
{
	SAFE_DELETE(m_pGeneralSettings);
	SAFE_DELETE(m_pAdvancedSettings);
}

bool CEditorManager::Initialize(LiveCreate::IManager* pManager)
{
	if (NULL == m_pManager)
	{
		m_pManager = pManager;

		// create the power check thread
		m_pPowerStatusThread = new CPowerStatusThread(this);
		m_pPowerStatusThread->Start();

		// add as a console var sink
		gEnv->pConsole->AddConsoleVarSink(this);

		// sub systems
		m_pObjectSync = new CObjectSync(this);
		m_pFileSyncManager = new CFileSyncManager(this);

		// load initial settings
		LoadSettings();
	}

	return true;
}

void CEditorManager::Shutdown()
{
	gEnv->pConsole->RemoveConsoleVarSink(this);

	m_bRequestedToStartLiveCreate = false;

	// stop the launching schedule
	if (NULL != m_pStartingSchedule)
	{
		m_pStartingSchedule->Cancel();
		m_pStartingSchedule->Release();
		m_pStartingSchedule = NULL;
	}

	// close the power check thread
	if (NULL != m_pPowerStatusThread)
	{
		m_pPowerStatusThread->Stop();
		m_pPowerStatusThread->WaitForThread();
		delete m_pPowerStatusThread;
		m_pPowerStatusThread = NULL;
	}

	// delete all of the hosts
	for (THostList::const_iterator it = m_pHosts.begin();
		it != m_pHosts.end(); ++it )
	{
		(*it)->Release();
	}
	m_pHosts.clear();

	SAFE_DELETE(m_pObjectSync);
	SAFE_DELETE(m_pFileSyncManager);
}

void CEditorManager::StopLiveCreate()
{
	// clean the start flag
	m_bRequestedToStartLiveCreate = false;

	// cancel the start procedure
	if (m_pStartingSchedule != NULL)
	{
		m_pStartingSchedule->Cancel();
		m_pStartingSchedule->Release();
		m_pStartingSchedule = NULL;
	}

	// send the "disconnect" message
	SyncDisableLiveCreate();
	Sleep(500);

	// disconnect all of the hosts
	for (THostList::const_iterator it = m_pHosts.begin();
		it != m_pHosts.end(); ++it )
	{
		if (NULL != (*it)->GetHostInfo())
		{
			(*it)->GetHostInfo()->Disconnect();
		}
	}

	// disable the low-level LiveCreate manager
	m_pManager->SetEnabled(false);

	// propagate to editor based listeners
	for (TListenerList::const_iterator it=m_pListeners.begin();
		it != m_pListeners.end(); ++it)
	{
		(*it)->OnLiveCreateStopped();
	}
}

bool CEditorManager::IsStarting() const
{
	return (NULL != m_pStartingSchedule);
}

void CEditorManager::SetEnabled(bool bIsEnabled)
{
	m_pManager->SetEnabled(bIsEnabled);
}

void CEditorManager::SetCameraSync(bool bIsEnabled)
{
	if (m_bIsCameraSyncEnabled != bIsEnabled)
	{
		m_bIsCameraSyncEnabled = bIsEnabled;
		SyncCameraFlag();
	}
}

CEditorHostInfo* CEditorManager::AddHostEntry(const char* szPlatformName, const char* szTargetName, const char* szKnownAddress)
{
	// Find matching platform name
	IPlatformHandlerFactory* pFactory = NULL;
	for (uint i=0; i<GetNumPlatforms(); ++i)
	{
		IPlatformHandlerFactory* pTestFactory = GetPlatform(i);
		if (0 == stricmp(pTestFactory->GetPlatformName(), szPlatformName))
		{
			pFactory = pTestFactory;
			break;
		}
	}

	// Unknown platform factory
	if (NULL == pFactory)
	{
		return NULL;
	}

	// Make sure we don't have a duplicate
	for (THostList::const_iterator it=m_pHosts.begin();
		it != m_pHosts.end(); ++it)
	{
		CEditorHostInfo* pTestHost = (*it);

		if ((0 == stricmp(pTestHost->GetPlatformFactory()->GetPlatformName(), szPlatformName)) &&
			(0 == stricmp(pTestHost->GetTargetName(), szTargetName)))
		{
			if (szKnownAddress && szKnownAddress[0])
			{
				pTestHost->SetAddress(szKnownAddress);
			}

			return NULL;
		}
	}

	// Create new host info
	CEditorHostInfo* pHostInfo = new CEditorHostInfo(this, pFactory, szTargetName, szKnownAddress);
	pHostInfo->AddRef();
	m_pHosts.push_back(pHostInfo);

	// Signal listeners
	for (TListenerList::const_iterator it=m_pListeners.begin();
		it != m_pListeners.end(); ++it)
	{
		(*it)->OnHostAdded(pHostInfo);
	}

	return pHostInfo;
}

bool CEditorManager::RemoveEntry(CEditorHostInfo* pHostEntry)
{
	for (THostList::iterator it = m_pHosts.begin(); 
		it != m_pHosts.end(); ++it)
	{
		if ((*it) == pHostEntry)
		{
			m_pHosts.erase(it);

			// Signal listeners
			for (TListenerList::const_iterator jt=m_pListeners.begin();
				jt != m_pListeners.end(); ++jt)
			{
				(*jt)->OnHostRemoved(pHostEntry);
			}

			// remove the host info from low-level LiveCreate manager
			IHostInfo* pHostInfo = pHostEntry->GetHostInfo();
			if (NULL != pHostInfo)
			{
				m_pManager->RemoveHost(pHostInfo);
			}

			// cleanup
			if (NULL != pHostEntry->GetHostInfo())
			{
				pHostEntry->GetHostInfo()->Disconnect();
			}
			pHostEntry->Release();
			return true;
		}
	}

	// not found
	return false;
}

bool CEditorManager::IsPlatformOn(IPlatformHandler* pHandler) const
{
	if (NULL != pHandler)
	{
		if (pHandler->IsFlagSet(IPlatformHandler::eFlag_HasSlowPowerStateCheck))
		{
			return m_pPowerStatusThread->IsPoweredOn(pHandler);
		}
		else
		{
			return pHandler->IsOn();
		}
	}

	// invalid platform, assume it's not on
	return false;
}

void CEditorManager::RegisterListener(IListener* pListener)
{
	if (NULL != pListener)
	{
		// make sure we register each listener only once
		TListenerList::const_iterator it = std::find(m_pListeners.begin(), m_pListeners.end(), pListener);
		if (it == m_pListeners.end())
		{
			m_pListeners.push_back(pListener);
		}
	}
}

void CEditorManager::UnregisterListener(IListener* pListener)
{
	// remove the listener from list
	TListenerList::const_iterator it = std::find(m_pListeners.begin(), m_pListeners.end(), pListener);
	if (it != m_pListeners.end())
	{
		m_pListeners.erase(it);
	}
}

void CEditorManager::GetEnabledHosts(std::vector<CEditorHostInfo*>& outHosts) const
{
	CryAutoLock<CryMutex> lock(m_accessLock);

	for (THostList::const_iterator it=m_pHosts.begin();
		it != m_pHosts.end(); ++it )
	{
		if ( (*it)->IsEnabled() )
		{
			(*it)->AddRef();
			outHosts.push_back(*it);
		}
	}
}

void CEditorManager::GetHosts(std::vector<CEditorHostInfo*>& outHosts) const
{
	CryAutoLock<CryMutex> lock(m_accessLock);

	for (THostList::const_iterator it=m_pHosts.begin();
		it != m_pHosts.end(); ++it )
	{
		if ((*it) != NULL)
		{
			(*it)->AddRef();
			outHosts.push_back(*it);
		}
	}
}

void CEditorManager::BeginObjectsLoading()
{
	CRY_ASSERT(m_bIsLoadingObjects == false);
	m_bIsLoadingObjects = true;
	LogMessagef(eLogType_Normal, "Object loading started");
}

void CEditorManager::EndObjectsLoading()
{
	CRY_ASSERT(m_bIsLoadingObjects == true);
	m_bIsLoadingObjects = false;
	LogMessagef(eLogType_Normal, "Object loading ended");
}

void CEditorManager::LoadSettings()
{
	CString strLiveCreateSettingsFile(Path::GetUserSandboxFolder());
	strLiveCreateSettingsFile += "LiveCreateSettings.xml";

	if (!CFileUtil::FileExists(strLiveCreateSettingsFile))
		return;

	XmlNodeRef rootElem = XmlHelpers::LoadXmlFromFile(strLiveCreateSettingsFile);
	if (rootElem)
	{
		// Camera sync flag
		{
			bool bEnabled = false;
			rootElem->getAttr("bEnableCameraSync", bEnabled);
			SetCameraSync(bEnabled);
		}

		// Load general settings
		{
			XmlNodeRef profilesElem = rootElem->findChild("general");
			if (profilesElem)
			{
				m_pGeneralSettings->LoadFromXML(profilesElem);
			}
		}

		// Load advanced settings
		{
			XmlNodeRef profilesElem = rootElem->findChild("advanced");
			if (profilesElem)
			{
				m_pAdvancedSettings->LoadFromXML(profilesElem);
			}
		}
	}

	// Load peers
	if (rootElem)
	{
		XmlNodeRef peersElem = rootElem->findChild("peers");
		if (peersElem)
		{
			for (int i = 0; i < peersElem->getChildCount(); ++i)
			{
				XmlNodeRef peerElem = peersElem->getChild(i);
				if (peerElem)
				{
					CEditorHostInfo* pHostInfo = CEditorHostInfo::LoadFromXML(this, peerElem);
					if (NULL != pHostInfo)
					{
						pHostInfo->AddRef();
						m_pHosts.push_back(pHostInfo);
					}
				}
			}
		}
	}

	// sync the config value with the internal manager value
	m_pManager->SetLogEnabled(GetAdvancedSettings().bEnableLog);
}

void CEditorManager::SaveSettings()
{
	XmlNodeRef rootElem = XmlHelpers::CreateXmlNode("settings");
	if (NULL != rootElem)
	{
		// general settings
		rootElem->setAttr("bEnableCameraSync", IsCameraSyncEnabled());

		// save other general settings
		{
			XmlNodeRef profilesElem = rootElem->createNode("general");
			{
				rootElem->addChild(profilesElem);
				m_pGeneralSettings->SaveToXML(profilesElem);
			}
		}

		// save advanced settings
		{
			XmlNodeRef profilesElem = rootElem->createNode("advanced");
			{
				rootElem->addChild(profilesElem);
				m_pAdvancedSettings->SaveToXML(profilesElem);
			}
		}

		// save hosts
		XmlNodeRef peersElem = rootElem->createNode("peers");
		if (peersElem)
		{
			rootElem->addChild(peersElem);

			for (THostList::const_iterator it=m_pHosts.begin();
				it != m_pHosts.end(); ++it )
			{
				CEditorHostInfo* pHost = (*it);

				XmlNodeRef peerElem = peersElem->createNode("peer");
				if (peerElem)
				{
					peersElem->addChild(peerElem);
					pHost->SaveToXML(peerElem);
				}
			}
		}

		CString strLiveCreateSettingsFile(Path::GetUserSandboxFolder());
		strLiveCreateSettingsFile+="LiveCreateSettings.xml";

		XmlHelpers::SaveXmlNode(rootElem,strLiveCreateSettingsFile);
	}
}

void CEditorManager::Update()
{
	// Propagate to existing hosts
	for (THostList::const_iterator it=m_pHosts.begin();
		it != m_pHosts.end(); ++it)
	{
		(*it)->UpdateBackgroundTasks();
	}

	// Propagate to low-level manager
	if (NULL != m_pManager)
	{
		m_pManager->Update();
	}

	// Flush changes from area sync
	if (NULL != m_pObjectSync)
	{
		m_pObjectSync->FlushChanges();
	}

	// Archetypes syncing
	if (m_bArchetypesDirty)
	{
		const float diff = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(m_lastArchetypeSyncTime);
		if (diff > GetAdvancedSettings().fArchetypeSyncTime)
		{
			if (SendArchetypeData())
			{
				m_lastArchetypeSyncTime = gEnv->pTimer->GetAsyncTime();
				m_bArchetypesDirty = false;
			}
		}
	}

	// Particles syncing
	if (m_bParticlesDirty)
	{
		const float diff = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(m_lastParticleSyncTime);
		if (diff > GetAdvancedSettings().fParticleSyncTime)
		{
			if (SendParticlesData())
			{
				m_lastParticleSyncTime = gEnv->pTimer->GetAsyncTime();
				m_bParticlesDirty = false;
			}
		}
	}

	// Start request but waiting for the host info
	if (m_bRequestedToStartLiveCreate)
	{
		bool bHasAnyPendingUpdate = false;
		for (THostList::const_iterator it = m_pStartingHosts.begin();
			it != m_pStartingHosts.end(); ++it)
		{
			CEditorHostInfo* pHost = (*it);
			if (pHost->IsUpdatingHostInfo())
			{
				bHasAnyPendingUpdate = true;
				break;
			}
		}

		if (!bHasAnyPendingUpdate)
		{
			gEnv->pLog->Log("Finished getting host information. Starting LiveCreate.");
			FinishLiveCreateStart();
		}
	}

	// Update starting schedule
	if (NULL != m_pStartingSchedule)
	{
		const EScheduleState state = m_pStartingSchedule->GetState();
		if (state == eScheduleState_Failed || state == eScheduleState_Completed)
		{
			SAFE_RELEASE(m_pStartingSchedule);

			if (state == eScheduleState_Failed)
			{
				for (TListenerList::const_iterator it=m_pListeners.begin();
					it != m_pListeners.end(); ++it)
				{
					(*it)->OnLiveCreateError();
				}
			}
			else
			{
				for (TListenerList::const_iterator it=m_pListeners.begin();
					it != m_pListeners.end(); ++it)
				{
					(*it)->OnLiveCreateStarted();
				}
			}
		}
		else
		{
			const float fProggress = m_pStartingSchedule->GetProgress();

			for (TListenerList::const_iterator it=m_pListeners.begin();
				it != m_pListeners.end(); ++it)
			{
				(*it)->OnLiveCreateStarting(fProggress);
			}
		}
	}

	// Pass the update file sync  manager (will create new syncing jobs)
	m_pFileSyncManager->Update();
}

bool CEditorManager::SendCommand(const ILiveCreateCommand& command)
{
	if (CanSend())
	{
		return m_pManager->SendCommand((const IRemoteCommand&)command);
	}
	else
	{
		return false;
	}
}

void CEditorManager::LogMessagef(ELogMessageType aType, const char* szMessage, ... )
{
	if (NULL != m_pManager)
	{
		char szBuffer[2048];
		va_list args;
		va_start(args, szMessage);
		vsnprintf_s(szBuffer, sizeof(szBuffer), szMessage, args);
		va_end(args);

		m_pManager->LogMessage(aType, szBuffer);
	}
}

void CEditorManager::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
		case eNotify_OnIdleUpdate:
		{
			Update();
			break;
		}

		case eNotify_OnSelectionChange:
		{
			SyncSelection();
			break;
		}
		case eNotify_OnQuit:
			{
					GetIEditor()->UnregisterNotifyListener(this);
			}
			break;
	}
}

bool CEditorManager::OnBeforeVarChange(ICVar *pVar, const char *sNewValue)
{
	return true;
}

void CEditorManager::OnAfterVarChange(ICVar *pVar)
{
	SyncCVarChange(pVar);
}

} // namespace LiveCreate

#endif