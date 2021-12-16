#pragma once

////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include <ILiveCreateCommon.h>
#include <ILiveCreatePlatform.h>
#include <ILiveCreateManager.h>

#include "IBackgroundTaskManager.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

//-----------------------------------------------------------------------------

class CBGTask_ScanBuilds;
class CBGTask_ResolveAddress;
class CBGTask_LaunchGame;
class CBGTask_Connect;
class CBGTask_SearchForHosts;

//-----------------------------------------------------------------------------

#define PROCESS_CANCEL_REQUEST() if ( IsCanceled() ) { return eTaskResult_Canceled; }

//-----------------------------------------------------------------------------

/// Background task for resolving the console actual address and creating the platform handler for it
class CBGTask_ResolveAddress : public IBackgroundTask
{
protected:
	IPlatformHandlerFactory* m_pFactory;
	CString m_targetName;
	CString m_resolvedAddress;

public:
	CBGTask_ResolveAddress(IPlatformHandlerFactory* pFactory, const CString& targetName);

	ILINE const CString& GetTargetName() const
	{
		return m_targetName;
	}

	ILINE const CString& GetResolvedAddress() const
	{
		return m_resolvedAddress;
	}

protected:
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

/// Background task for launching an executable on console
class CBGTask_LaunchExecutable : public IBackgroundTask
{
protected:
	IPlatformHandler* m_pPlatform;
	CString m_args;
	CString m_levelFolder;
	CString m_directory;
	CString m_executable;

public:
	CBGTask_LaunchExecutable(IPlatformHandler* pPlatform, const CString& directory, const CString& executable, const CString& args);
	virtual ~CBGTask_LaunchExecutable();

protected:
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

/// Background task that will try to connect to LiveCreate host
class CBGTask_Connect : public IBackgroundTask
{
protected:
	IHostInfo* m_pHost;

public:
	CBGTask_Connect(IHostInfo* pHost);
	virtual ~CBGTask_Connect();

protected:
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

/// Background task for searching for new hosts
class CBGTask_SearchForHosts : public IBackgroundTask
{
private:
	// factories to check
	std::vector<IPlatformHandlerFactory*> m_pFactories;

	// found targets
	static const uint32 kMaxTargets = 64;
	LiveCreate::IPlatformHandlerFactory::TargetInfo m_targets[kMaxTargets];
	uint32 m_numTargets;

public:
	// Get number of discovered targets
	ILINE const uint32 GetNumTargets() const
	{
		return m_numTargets;
	}

	// Get the target info
	ILINE const IPlatformHandlerFactory::TargetInfo& GetTarget(const uint32 index) const
	{
		return m_targets[index];
	}

public:
	CBGTask_SearchForHosts(CEditorManager* pManager);

protected:
	// IBackgroundTask interface
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

/// Background task for searching for new hosts on given platform factory
class CBGTask_SearchForHostsForPlatform : public IBackgroundTask
{
private:
	// factory to check
	IPlatformHandlerFactory* m_pFactory;

	// found targets
	static const uint32 kMaxTargets = 64;
	LiveCreate::IPlatformHandlerFactory::TargetInfo m_targets[kMaxTargets];
	uint32 m_numTargets;

public:
	// Get number of discovered targets
	ILINE const uint32 GetNumTargets() const
	{
		return m_numTargets;
	}

	// Get the target info
	ILINE const IPlatformHandlerFactory::TargetInfo& GetTarget(const uint32 index) const
	{
		return m_targets[index];
	}

public:
	CBGTask_SearchForHostsForPlatform(IPlatformHandlerFactory* pFactory);

protected:
	// IBackgroundTask interface
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

/// Background task for scanning directory content
class CBGTask_ScanDirectory : public IBackgroundTask, public IPlatformHandlerFolderScan
{
private:
	IPlatformHandler* m_pPlatform;
	CString m_path;

public:
	std::vector<CString> m_directories;
	std::vector<CString> m_executables;
	std::vector<CString> m_files;

public:
	// Get scanned path
	ILINE const CString& GetPath() const
	{
		return m_path;
	}

public:
	CBGTask_ScanDirectory(IPlatformHandler* pPlatform, const CString& remotePath);
	virtual ~CBGTask_ScanDirectory();

protected:
	// IBackgroundTask interface
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;

private:
	// IPlatformHandlerFolderScan interface implementation
	virtual void OnFolder(const char* szBasePath, const char* szEntryName);
	virtual void OnFile(const char* szBasePath, const char* szEntryName, bool bIsExecutable);
};

//-----------------------------------------------------------------------------

/// Background task for copying file to target console
class CBGTask_CopyFileToTarget : public IBackgroundTask
{
private:
	CString m_address;
	CString m_srcPath;
	CString m_destPath;
	bool m_bIsSourceAbsolute;
	bool m_bAutoDeleteSource;

public:
	CBGTask_CopyFileToTarget(const CString& targetAddress, const CString& srcPath, const CString& destPath, bool bIsSourceAbsolute=false, bool bAutoDeletesource=false);
	virtual ~CBGTask_CopyFileToTarget();

	const CString& GetDestPath() const { return m_destPath; }

	const CString& GetAddress() const { return m_address; }

protected:
	// IBackgroundTask interface
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

/// Execute console command on target - with ACK
class CBGTask_ExecuteCommand : public IBackgroundTask
{
private:
	CString m_address;
	CString m_command;
	CTimeValue m_startTime;
	float m_waitTime;
	uint32 m_retryCount;

public:
	CBGTask_ExecuteCommand(const CString& address, const CString& command, const float maxWaitTime=3.0f, const uint32 maxRetryCount=10);
	virtual ~CBGTask_ExecuteCommand();

protected:
	// IBackgroundTask interface
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};


//-----------------------------------------------------------------------------

/// Retrieve game related host information
class CBGTask_GetHostInfoPacket : public IBackgroundTask
{
protected:
	CString m_address;
	CHostInfoPacket m_packet;

public:
	CBGTask_GetHostInfoPacket(const CString& address);
	virtual ~CBGTask_GetHostInfoPacket();

	ILINE const CHostInfoPacket& GetInfoPacket() const
	{
		return m_packet;
	}

protected:
	virtual void Delete() override
	{
		delete this;
	}

	virtual ETaskResult Work() override;
};

//-----------------------------------------------------------------------------

class CUDPConnection
{
public:
	const static uint32 kMaxPacketSize = 512;

	CString m_addrStr;
	sockaddr_in m_addr;
	int m_socket;

public:
	CUDPConnection(const CString& addr);
	~CUDPConnection();

	IDataReadStream* SendWithResponse(IDataWriteStream& writer);
	uint8 SendWithShortResponse(IDataWriteStream& writer);
	void SendNoResponse(IDataWriteStream& writer);

	IDataReadStream* Receive();
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

}

#endif