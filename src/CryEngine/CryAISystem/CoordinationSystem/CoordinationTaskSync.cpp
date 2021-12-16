#include "StdAfx.h"
#include "CoordinationTaskSync.h"
#include "CoordinationSetup.h"


CoordinationTaskSync::CoordinationTaskSync(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor)
: m_setup(setup)
, m_taskMonitor(taskMonitor)
{
}

bool CoordinationTaskSync::LoadFromXML(int currentRoleID, const XmlNodeRef& rootNode)
{
	if (rootNode->haveAttr("waitFor"))
	{
		const char* waitFor;
		rootNode->getAttr("waitFor", &waitFor);

		stack_string waitList(waitFor);

		int start = 0;
		stack_string roleTaskName = waitList.Tokenize(",", start);
		
		while (!roleTaskName.empty())
		{
			int roleID = -1;
			int taskID = -1;

			stack_string roleName;
			stack_string taskId;

			roleTaskName.Trim();
			
			int colon = roleTaskName.find_first_of(':');
			if (colon > -1)
			{
				roleName = roleTaskName.Left(colon);
				taskId = roleTaskName.Right(roleTaskName.size() - colon - 1);

				roleID = m_setup->GetRoleID(roleName.c_str());
				if (!taskId.empty() && (taskId.SpanIncluding("0123456789") == taskId))
					taskID = atoi(taskId.c_str());
				else
				{
					AIWarning("Invalid 'waitFor' attribute for tag '%s' at line %d... %s[:taskID] - taskID must be a number!",
						rootNode->getTag(), rootNode->getLine(), roleName.c_str());

					return false;
				}
			}
			else
				roleID = m_setup->GetRoleID(roleTaskName.c_str());

			if (roleID >= 0)
			{
				if (roleID != currentRoleID)
					m_waitInfos.push_back(SWaitInfo(roleID, taskID));
				else
				{
					AIWarning("Role '%s' waiting for itself in 'waitFor' attribute for tag '%s' at line %d...", 
						roleName.c_str(), rootNode->getTag(), rootNode->getLine());

					return false;
				}
			}
			else
			{
				AIWarning("Unknown role '%s' in 'waitFor' attribute for tag '%s' at line %d...", 
					roleName.c_str(), rootNode->getTag(), rootNode->getLine());

				return false;
			}

			roleTaskName = waitList.Tokenize(",", start);
		}
	}

	return true;
}

void CoordinationTaskSync::WaitForRole(int roleId)
{
	m_waitInfos.push_back(SWaitInfo(roleId));
}

void CoordinationTaskSync::WaitForTask(int roleId, int taskId)
{
	m_waitInfos.push_back(SWaitInfo(roleId, taskId));
}

ECoordinationState CoordinationTaskSync::GetState(const CoordinationID& coordinationID) const
{
	RunningSyncs::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const SRunningSync& sync = it->second;
	return sync.state;
}

void CoordinationTaskSync::Start(const CoordinationID& coordinationID)
{
	RunningSyncs::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningSyncs::iterator, bool> result = m_running.insert(RunningSyncs::value_type(coordinationID, SRunningSync()));
	SRunningSync& running = result.first->second;
}

void CoordinationTaskSync::Stop(const CoordinationID& coordinationID)
{
	RunningSyncs::iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	m_running.erase(it);
}

void CoordinationTaskSync::Update()
{
	if (!m_running.empty())
	{
		RunningSyncs::iterator it = m_running.begin();
		RunningSyncs::iterator end = m_running.end();

		for ( ; it != end; ++it)
		{
			const CoordinationID& coordinationID = it->first;
			SRunningSync& sync = it->second;

			uint32 waitCount = m_waitInfos.size();
			
			for (uint32 i = 0; i < waitCount; ++i)
			{
				if ((sync.waitFinished & (1 << i)) == 0)
				{
					SWaitInfo& waitInfo = m_waitInfos[i];
					
					if (waitInfo.taskId >= 0)
					{
						ECoordinationState taskState = m_taskMonitor->GetTaskState(coordinationID, waitInfo.roleId, waitInfo.taskId);
						if ((taskState == CoordinationFinished) || (taskState == CoordinationWaiting))
							sync.waitFinished |= (1 << i);
					}
					else
					{
						ECoordinationState roleState = m_taskMonitor->GetRoleState(coordinationID, waitInfo.roleId);
						if ((roleState == CoordinationFinished) || (roleState == CoordinationWaiting))
							sync.waitFinished |= (1 << i);
					}
				}
			}

			bool waitFinished = sync.waitFinished == (1 << waitCount)-1;
			if (waitFinished)
				sync.state = CoordinationFinished;
		}
	}
}