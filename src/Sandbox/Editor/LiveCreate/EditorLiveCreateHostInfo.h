#pragma once

////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include <ILiveCreateCommon.h>
#include <ILiveCreatePlatform.h>
#include <ILiveCreateManager.h>
#include <ILiveCreateHost.h>

#ifndef NO_LIVECREATE

namespace LiveCreate
{

//-----------------------------------------------------------------------------

class CBGTask_ScanBuilds;
class CBGTask_ResolveAddress;
class CBGTask_GetHostInfoPacket;

//-----------------------------------------------------------------------------

/// LiveCreate host status (for editor)
enum EHostStatus
{
	// We don't have a platform handler for this host - it's probably something that is no longer there
	eHostStatus_Dead=0,

	// We do have a platform handler for the host and it is off (so usually no IP address can be established and we can't launch the game)
	eHostStatus_Offline,

	// The host is online, we have a platform handler - we can upload the build and launch the game
	eHostStatus_Online,

	// The host is online, the game is running and we have the connection to the LiveCreate host
	eHostStatus_Connected,

	// The host is online, we have connection and it was confirmed by the remote side.
	eHostStatus_Ready,
};

//-----------------------------------------------------------------------------

/// Editor side only LiveCreate host wrapper
class CEditorHostInfo : public CMultiThreadRefCount
{
	// how often should we try to resolve the address (ms)
	static const int64 kAddressResolveTimer = 1000;

protected:
	class CEditorManager* m_pManager;

	// LiveCreate platform factory interface
	IPlatformHandlerFactory* m_pPlatformFactory;

	// Is the LiveCreate enabled for this host?
	bool m_bIsEnabled;

	// Name of the target (recognized by related platform factory), resolvable to network address
	CString m_targetName;

	// Selected build directory
	CString m_buildDirectory;

	// Selected executable that should be used to launch game on the target
	CString m_buildExecutable;

protected:
	// LiveCreate platform interface (valid once the address was resolved)
	IPlatformHandler* m_pPlatform;

	// LiveCreate host interface (valid once the platform handler was created)
	IHostInfo* m_pHost;

	// Resolved physical IP address
	CString m_validAddress;

	// Last valid host info data
	CHostInfoPacket m_hostInfoPacket;
	CTimeValue m_hostInfoPacketUpdateTime;
	bool m_bHostInfoPacketValid;

private:
	// Background editor task related to resolving the address of this console
	CBGTask_ResolveAddress* m_pResolveAddressTask;

	// Background task to query the host info packet
	CBGTask_GetHostInfoPacket* m_pHostInfoPacketTask;

	// Last time an address resolve task was scheduled
	int64 m_lastAddressResolveTaskTime;

	// Critical section for accessing the tasks results
	CryMutex m_lock;

public:
	ILINE bool IsEnabled() const
	{
		return m_bIsEnabled;
	}

	ILINE IPlatformHandlerFactory* GetPlatformFactory() const
	{
		return m_pPlatformFactory;
	}

	ILINE IPlatformHandler* GetPlatform() const
	{
		return m_pPlatform;
	}

	ILINE IHostInfo* GetHostInfo() const
	{
		return m_pHost;
	}

	ILINE class CEditorManager* GetManager() const
	{
		return m_pManager;
	}

	ILINE const CString& GetTargetName() const
	{
		return m_targetName;
	}

	ILINE const CString& GetAddres() const
	{
		return m_validAddress;
	}

	ILINE const CString& GetBuildExecutable() const
	{
		return m_buildExecutable;
	}

	ILINE const CString& GetBuildDirectory() const
	{
		return m_buildDirectory;
	}

	ILINE const bool HasValidAddress() const
	{
		return !m_validAddress.IsEmpty();
	}

	ILINE const bool HasValidHostInfoPacket() const
	{
		return m_bHostInfoPacketValid;
	}

	ILINE const bool IsUpdatingHostInfo() const
	{
		return (NULL != m_pHostInfoPacketTask);
	}

	ILINE const CHostInfoPacket& GetHostInfoPacket() const
	{
		return m_hostInfoPacket;
	}

public:
	CEditorHostInfo(CEditorManager* pManager, IPlatformHandlerFactory* pPlatformFactory, const CString& targetName, const CString& lastKnownAddress);

	// Evaluate current host status (mostly for UI updates)
	EHostStatus EvaluateStatus() const;

	// Enable/Disable the host
	void Enable(bool bFlag);

	// Check if the host hardware is on
	bool IsOn() const;

	// Do we have a connection to the host (confirmed or not)?
	bool IsConnected() const;

	// Do we have a confirmed and not suppressed connection to the host ?
	bool IsReady() const;

	// Cancel all background tasks (game launching, build scanning, etc) currently performed by this host
	void CancelAllTasks();

	// Take a screenshot (synchronous)
	bool Screenshot(CString& outErrorString);

	// Request build list to be retrieved from target
	void ScanForBuilds();

	// Sync with current background work and check if there's a new background work to be done for this host
	void UpdateBackgroundTasks();

	// Select build executable and directory
	void SetBuild(const CString& buildDirectory, const CString& buildExecutable);
	
	// Update stored address
	void SetAddress(const CString& address);

	// Request updating host information
	void RequestHostInfoUpdate();

public:
	// Create and load the object from XML node
	static CEditorHostInfo* LoadFromXML(CEditorManager* pManager, XmlNodeRef node);

	// Save host settings to an XML node
	void SaveToXML(XmlNodeRef node) const;

private:
	~CEditorHostInfo();
};

}

#endif