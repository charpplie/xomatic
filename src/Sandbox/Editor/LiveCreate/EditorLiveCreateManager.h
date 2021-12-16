#pragma once

////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include <ILiveCreateCommon.h>
#include <ILiveCreatePlatform.h>
#include <ILiveCreateManager.h>
#include <ILiveCreateHost.h>
#include "ConfigGroup.h"

#ifndef NO_LIVECREATE

struct IBackgroundSchedule;

namespace LiveCreate
{

class CBGTask_SearchForHosts;
class CPowerStatusThread;
struct ILiveCreateCommand;
class CFileSyncManager;
class CObjectSync;
class CEditorHostInfo;

enum ELiveCreateObjectType
{
	eLiveCreateObjectType_Terrain,
	eLiveCreateObjectType_Brushes,
	eLiveCreateObjectType_Vegetation,
	eLiveCreateObjectType_Decals,
	eLiveCreateObjectType_Roads,

	eLiveCreateObjectType_MAX,
};

struct STerrainTextureInfo
{
	uint8* pData;
	uint32 aDataSize;
	int32 posx;
	int32 posy;
	uint32 width;
	uint32 height;
	ETEX_Format eTFSrc;
};

// LiveCreate general settings
class CGeneralSettings : public Config::CConfigGroup
{
public:
	CGeneralSettings();

	bool bResetBeforeStart;
	bool bExportLevelOnStart;
	bool bExportTerrainOnStart;
	bool bFastTargetReset;
	bool bShowSelectionBoxes;
	bool bShowSelectionNames;
	bool bSyncCameraFromGame;
	bool bAlwaysReloadLevel;
	bool bSyncFileUpdates;
};

// LiveCreate advanced settings
class CAdvancedSettings : public Config::CConfigGroup
{
public:
	CAdvancedSettings();

	bool bEnableLog;
	bool bGodMode;
	bool bDisableAI;
	bool bDisableScripts;
	bool bSkipCinematics;
	bool bMemReplay;
	bool bAutoConnect;
	bool bResumePhysicsOnDisconnect;
	bool bAutoWakeUpPhysicalObjects;
	bool bAlwaysSyncFullEntities;
	bool bUseCRCWithEntitySync;
	float fArchetypeSyncTime;
	float fParticleSyncTime;
	float fObjectsSyncTime;
	float fSlowResetWaitTime;
	float fFastResetWaitTime;
	int iReconnectionCount;
	float fReconnectionDelay;
};

/// Settings and profile manager
class CEditorManager : public IEditorNotifyListener, IConsoleVarSink
{
public:
	struct IListener
	{
		virtual ~IListener() {};
		virtual void OnHostAdded(CEditorHostInfo* pHost) {};
		virtual void OnHostRemoved(CEditorHostInfo* pHost) {};
		virtual void OnLiveCreateStarting(const float progress) {};
		virtual void OnLiveCreateStarted() {};
		virtual void OnLiveCreateStopped() {};
		virtual void OnLiveCreateError() {};
	};

protected:
	// The low-level manager (from CryLiveCreate)
	IManager* m_pManager;

	// Editor-side host informations (wrappers around IHostInfo from CryLiveCreate)
	typedef std::vector< CEditorHostInfo* > THostList;
	THostList m_pHosts;

	// LiveCreate settings profiles
	CGeneralSettings* m_pGeneralSettings;
	CAdvancedSettings* m_pAdvancedSettings;

	// General listeners for LiveCreate related events
	typedef std::vector< IListener* > TListenerList;
	TListenerList m_pListeners;

	// Thread checking the power on status of hosts in the background
	CPowerStatusThread* m_pPowerStatusThread;
	CryMutex m_accessLock;

	// Object sync manager (for syncing portions of terrain and objects)
	CObjectSync* m_pObjectSync;

	// Manager class for file sync operations
	CFileSyncManager* m_pFileSyncManager;

	// Archetype syncing 
	CTimeValue m_lastArchetypeSyncTime;
	bool m_bArchetypesDirty;

	// Particles syncing 
	CTimeValue m_lastParticleSyncTime;
	bool m_bParticlesDirty;

	// Level flags
	bool m_bIsLoadingObjects;

	// Camera sync state is handled outside profile settings (it's to important)
	bool m_bIsCameraSyncEnabled;
	Vec3 m_lastCameraSyncPosition;
	Quat m_lastCameraSyncRotation;
	float m_lastCameraSyncFOV;
	Vec3 m_lastCameraKnownPosition;
	Quat m_lastCameraKnownRotation;
	float m_lastCameraKnownFOV;

	// LiveCreate startup schedule
	bool m_bRequestedToStartLiveCreate;
	THostList m_pStartingHosts;
	IBackgroundSchedule* m_pStartingSchedule;

public:
	ILINE const CGeneralSettings& GetGeneralSettings() const
	{
		return *m_pGeneralSettings;
	}

	ILINE CGeneralSettings& GetGeneralSettings()
	{
		return *m_pGeneralSettings;
	}

	ILINE const CAdvancedSettings& GetAdvancedSettings() const
	{
		return *m_pAdvancedSettings;
	}

	ILINE CAdvancedSettings& GetAdvancedSettings()
	{
		return *m_pAdvancedSettings;
	}

	ILINE const uint GetNumPlatforms() const
	{
		return m_pManager->GetNumPlatforms();
	}

	ILINE IPlatformHandlerFactory* GetPlatform(const uint index) const
	{
		return m_pManager->GetPlatformFactory(index);
	}

	ILINE bool IsEnabled() const
	{
		return m_pManager->IsEnabled();
	}

	ILINE bool IsCameraSyncEnabled() const
	{
		return m_bIsCameraSyncEnabled;
	}

	ILINE bool CanSend() const
	{
		return !m_bIsLoadingObjects && m_pManager && m_pManager->CanSend();
	}

public:
	CEditorManager();
	~CEditorManager();

	bool Initialize(LiveCreate::IManager* pManager);
	void Shutdown();
	void SetEnabled(bool bIsEnabled);
	void SetCameraSync(bool bIsEnabled);

	// LiveCreate on/off
	bool StartLiveCreate(CString& outReasonToFail, bool bQuick);
	void StopLiveCreate();
	bool IsStarting() const;

	// General listener
	void RegisterListener(IListener* pListener);
	void UnregisterListener(IListener* pListener);

	// Host related functions, for thread safety all the Get*() functions increment the refcount for the objects
	CEditorHostInfo* AddHostEntry(const char* szPlatformName, const char* szTargetName, const char* szKnownAddress);
	bool RemoveEntry(CEditorHostInfo* pHostEntry);
	void GetEnabledHosts(std::vector<CEditorHostInfo*>& outHosts) const;
	void GetHosts(std::vector<CEditorHostInfo*>& outHosts) const;

	// Check the power on status of given platform handler
	bool IsPlatformOn(IPlatformHandler* pHandler) const;

	// Send live create command to connected hosts
	bool SendCommand(const ILiveCreateCommand& command);

	// Log LiveCreate message (passes it to the low-level manager)
	void LogMessagef(ELogMessageType aType, const char* szMessage, ... );

	// Profile related functions
	void LoadSettings();
	void SaveSettings();

	// Level loading phase
	void BeginObjectsLoading();
	void EndObjectsLoading();

	// Editor->Consoles synchronisation interface
	bool SyncEnableLiveCreate();
	bool SyncDisableLiveCreate();
	bool SyncCameraTransform(const Matrix34& matrix);
	bool SyncCameraFOV(const float fov);
	bool SyncObjectTransform(const CBaseObject& object);
	bool SyncObjectCreated(const CBaseObject& object);
	bool SyncObjectDeleted(const CBaseObject& object);
	bool SyncObjectFull(const CBaseObject& object);
	bool SyncObjectProperty(const CBaseObject& object, IVariable* var);
	bool SyncEntityProperty(const CEntityObject& entity, IVariable* var);
	bool SyncCVarChange(const ICVar* pCVar);
	bool SyncTimeOfDay(const float time);
	bool SyncTimeOfDayVariable(const int varIndex, const char* displayName, ISplineInterpolator* pSpline);
	bool SyncTimeOfDayFull();
	bool SyncEnvironmentFull();
	bool Sync3DEngine(const AABB& area, ELiveCreateObjectType type);
	bool Sync3DObject(const CBaseObject& object);
	bool SyncArchetypesFull();
	bool SyncParticlesFull();
	bool SyncTerrainTexture(const STerrainTextureInfo& texInfo);
	bool SyncFile(const char* szPath);
	bool SyncSelection();
	bool SyncCameraFlag();	
	bool SyncObjectMaterial(const CBaseObject& entity, CMaterial* pNewMaterial);
	bool SyncLayerVisibility(const CObjectLayer& layer, bool bIsVisible);

private:
	// Sync with current background work and check if there's a new background work to be done for this host
	void Update();

	bool BuildFullSyncFile(string& outPath);
	bool SendArchetypeData();
	bool SendParticlesData();

	void FinishLiveCreateStart();

private:
	// IEditorNotifyListener interface
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

	// IConsoleVarSink interface implementation
	virtual bool OnBeforeVarChange(ICVar *pVar,const char *sNewValue);
	virtual void OnAfterVarChange(ICVar *pVar);
};

//-----------------------------------------------------------------------------

} 

#endif