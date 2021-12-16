#ifndef __CoordinationTaskSync_h__
#define __CoordinationTaskSync_h__

#pragma once


#include "Coordination.h"


class CoordinationTaskSync
{
public:
	CoordinationTaskSync(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor);

	bool LoadFromXML(int currentRoleID, const XmlNodeRef& rootNode);

	void WaitForRole(int roleId);
	void WaitForTask(int roleId, int taskId);

	ECoordinationState GetState(const CoordinationID& coordinationID) const;
	void Start(const CoordinationID& coordinationID);
	void Stop(const CoordinationID& coordinationID);
	void Update();

private:
	struct SRunningSync
	{
		SRunningSync()
			: waitFinished(0)
			, state(CoordinationRunning)
		{
		}

		uint32 waitFinished;
		ECoordinationState state;
	};

	typedef std::map<CoordinationID, SRunningSync> RunningSyncs;
	RunningSyncs m_running;

	struct SWaitInfo
	{
		SWaitInfo()
			: roleId(-1)
			, taskId(-1)
		{
		}
		SWaitInfo(int _roleId, int _taskId = -1)
			: roleId(_roleId)
			, taskId(_taskId)
		{
		}

		int roleId;
		int taskId;
	};

	typedef std::vector<SWaitInfo> WaitInfos;
	WaitInfos m_waitInfos;

	const CoordinationSetup* m_setup;
	const CoordinationTaskMonitor* m_taskMonitor;
};


#endif //__CoordinationTaskSync_h__
