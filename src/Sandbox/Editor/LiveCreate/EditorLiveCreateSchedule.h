#pragma once

////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include <ILiveCreateCommon.h>
#include <ILiveCreatePlatform.h>
#include <ILiveCreateManager.h>

#include "IBackgroundTaskManager.h"
#include "IBackgroundScheduleManager.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

//-----------------------------------------------------------------------------

/// Reference counted background schedule item
class CBGScheduleWork_Base : public IBackgroundScheduleItemWork
{
private:
	volatile int m_refCount;

public:
	CBGScheduleWork_Base();
	virtual ~CBGScheduleWork_Base();

	// Partial IBackgroundScheduleItemWork implementation
	virtual void AddRef();
	virtual void Release();
	virtual bool OnStart();
	virtual bool OnStop();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_ResetTarget : public CBGScheduleWork_Base
{
private:
	string m_name;
	CEditorHostInfo* m_pHost;
	IPlatformHandler* m_pTarget;
	CTimeValue m_resetEndTime;

public:
	CBGScheduleWork_ResetTarget(CEditorHostInfo* pHost);
	virtual ~CBGScheduleWork_ResetTarget();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_ExportLevel : public CBGScheduleWork_Base
{
private:
	bool m_bExportLevel;
	bool m_bExportTerrain;

public:
	CBGScheduleWork_ExportLevel(bool bExportLevel, bool bExportTerrain);

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_CopyFileToTarget : public CBGScheduleWork_Base
{
private:
	CString m_address;
	CString m_sourcePath;
	CString m_destPath;
	IBackgroundTask* m_pCopyTask;

public:
	CBGScheduleWork_CopyFileToTarget(CEditorHostInfo* pHost, const CString& srcPath, const CString& destPath);
	virtual ~CBGScheduleWork_CopyFileToTarget();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual bool OnStop();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_LaunchExecutable : public CBGScheduleWork_Base
{
private:
	IPlatformHandler* m_pTarget;
	CString m_directory;
	CString m_executable;
	CString m_args;
	CString m_name;
	IBackgroundTask* m_pLaunchTask;

public:
	CBGScheduleWork_LaunchExecutable(CEditorHostInfo* pHost, const CString& directory, const CString& executable, const CString& args);
	virtual ~CBGScheduleWork_LaunchExecutable();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual bool OnStop();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_Connect : public CBGScheduleWork_Base
{
private:
	CEditorHostInfo* m_pHost;
	CString m_name;
	uint32 m_currentRetry;
	const uint32 m_retryCount;
	const float m_retryDelay;
	CTimeValue m_nextRetry;
	IBackgroundTask* m_pConnectTask;

public:
	CBGScheduleWork_Connect(CEditorHostInfo* pHost, const uint32 retryCount, const float reconnectionDelay);
	virtual ~CBGScheduleWork_Connect();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual bool OnStop();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_WaitForTarget : public CBGScheduleWork_Base
{
private:
	CEditorHostInfo* m_pHost;
	const CTimeValue m_startTime;
	const float m_maxTime;

public:
	CBGScheduleWork_WaitForTarget(CEditorHostInfo* pHost);
	virtual ~CBGScheduleWork_WaitForTarget();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual bool OnStop();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_ExecuteRemoteConsoleCommand : public CBGScheduleWork_Base
{
private:
	CString m_address;
	CString m_command;
	CBGTask_ExecuteCommand* m_pTask;
	CString m_name;
	uint m_retryCount;
	float m_waitTime;

public:
	CBGScheduleWork_ExecuteRemoteConsoleCommand(const CString& address, const CString& command, const uint retryCount=10, const float waitTime=1.0f);
	virtual ~CBGScheduleWork_ExecuteRemoteConsoleCommand();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual bool OnStop();
	virtual EScheduleWorkItemStatus OnUpdate();
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_ToggleLiveCreateCommands : public CBGScheduleWork_ExecuteRemoteConsoleCommand
{
private:
	bool m_bSuppress;

public:
	CBGScheduleWork_ToggleLiveCreateCommands(const CString& address, bool bSuppress);
	virtual ~CBGScheduleWork_ToggleLiveCreateCommands();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
};

//-----------------------------------------------------------------------------

class CBGScheduleWork_EnableLiveCreate : public CBGScheduleWork_Base
{
private:
	float m_launchDelayTime;
	float m_totalDelayTime;
	bool m_bEnabled;
	CTimeValue m_startTime;

public:
	CBGScheduleWork_EnableLiveCreate();
	virtual ~CBGScheduleWork_EnableLiveCreate();

	// IBackgroundScheduleItemWork interface
	virtual const char* GetDescription() const;
	virtual float GetProgress() const;
	virtual bool OnStart();
	virtual EScheduleWorkItemStatus OnUpdate();
};


//-----------------------------------------------------------------------------

}

#endif