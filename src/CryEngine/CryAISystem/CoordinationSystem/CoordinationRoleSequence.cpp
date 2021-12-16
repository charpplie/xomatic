#include "StdAfx.h"
#include "CoordinationRoleSequence.h"
#include "CoordinationSetup.h"
#include "CoordinationTaskAction.h"
#include "CoordinationTaskSignal.h"
#include "CoordinationTaskCommunication.h"
#include "CoordinationTaskCoordination.h"

#include "DebugDrawContext.h"


CoordinationRoleSequence::CoordinationRoleSequence(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor)
: m_syncMode(Task)
, m_roleID(-1)
, m_setup(setup)
, m_taskMonitor(taskMonitor)
{
}

bool CoordinationRoleSequence::LoadFromXML(uint32 sequenceID, int roleID, const XmlNodeRef& rootNode)
{
	m_roleID = roleID;

	int childCount = rootNode->getChildCount();

	if (rootNode->haveAttr("syncMode"))
	{
		const char* syncMode;
		rootNode->getAttr("syncMode", &syncMode);

		if (!stricmp(syncMode, "end"))
			m_syncMode = End;
		else if (!stricmp(syncMode, "task"))
			m_syncMode = Task;
		else
		{
			AIWarning("Invalid value for attribute 'syncMode' tag '%s' at line %d...", 
				rootNode->getTag(), rootNode->getLine());

			return false;
		}
	}

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef childNode = rootNode->getChild(i);

		const char* tagName = childNode->getTag();

		if (!stricmp(tagName, "Action"))
		{
			CoordinationTaskAction action(m_setup, m_taskMonitor);

			if (!action.LoadFromXML(roleID, childNode))
				return false;

			m_tasks.push_back(new CoordinationTaskAction(action));
		}
		else if (!stricmp(tagName, "Signal"))
		{
			CoordinationTaskSignal signal(m_setup, m_taskMonitor);

			if (!signal.LoadFromXML(roleID, childNode))
				return false;

			m_tasks.push_back(new CoordinationTaskSignal(signal));
		}
		else if (!stricmp(tagName, "Communication"))
		{
			CoordinationTaskCommunication communication(m_setup, m_taskMonitor);

			if (!communication.LoadFromXML(roleID, childNode))
				return false;

			m_tasks.push_back(new CoordinationTaskCommunication(communication));
		}
		else if (!stricmp(tagName, "Coordination"))
		{
			CoordinationTaskCoordination coordination(m_setup, m_taskMonitor);

			if (!coordination.LoadFromXML(roleID, childNode))
				return false;

			m_tasks.push_back(new CoordinationTaskCoordination(coordination));
		}
		else
		{
			AIWarning("Unexpected tag '%s' at line %d...", tagName, childNode->getLine());

			return false;
		}
	}

	return true;
}

int CoordinationRoleSequence::GetRoleID() const
{
	return m_roleID;
}

void CoordinationRoleSequence::DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const
{
	const CoordinationRoles& roles = m_setup->GetRoles();
	const char* roleName = roles[m_roleID].GetName();

	stack_string text(roleName);

	RunningSequences::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningSequence& running = it->second;

		ECoordinationState state = running.state;
		context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize * 1.15f,
			GetStateColor(state, Col_CornflowerBlue, Col_DarkGreen, Col_Gold, Col_VioletRed),
			false, "%s", text.Left(20).c_str());
		context.y += context.lineHeight * 1.15f;
		context.y += context.lineHeight * 0.1f;

		uint32 taskCount = m_tasks.size();

		for (uint32 taskID = 0; taskID < taskCount; ++taskID)
		{
			const CoordinationTask::Ptr& task = m_tasks[taskID];
			ECoordinationState taskState = task->GetState(coordinationID);
			
			if (taskState == CoordinationNotRunning)
				continue;

			text.FormatFast("%d:%s:%s", taskID, task->GetName(), task->GetDescription());

			context.y += context.lineHeight * 0.2f;

			context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize,
				GetStateColor(taskState, Col_CornflowerBlue, Col_DarkGreen, Col_Gold, Col_VioletRed),
				false, "%s", text.Left(22).c_str());
			context.y += context.lineHeight;

			const float fontSize = context.fontSize * 0.9f;
			const float lineHeight = context.lineHeight * 0.9f;

			ActorStates::const_iterator it = running.actors.begin();
			ActorStates::const_iterator end = running.actors.end();

			for ( ; it != end; ++it)
			{
				const CoordinationActor& actor = it->first;
				const ActorState& actorState = it->second;

				if (taskID != actorState.taskID)
					continue;
				
				ECoordinationState actorTaskState = task->GetActorState(coordinationID, actor);

				text = actor.GetName();

				context->Draw2dLabel(context.x + context.ident + 3.5f, context.y, fontSize, 
					GetStateColor(actorTaskState, Col_CadetBlue, Col_LimeGreen, Col_Gold, Col_Red),
					false, "%s", text.Left(26).c_str());
				context.y += lineHeight;
			}
		}
	}
}

ECoordinationState CoordinationRoleSequence::GetTaskState(const CoordinationID& coordinationID, int taskID) const
{
	RunningSequences::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningSequence& running = it->second;
	if (taskID < running.taskID)
		return CoordinationFinished;
	else if (taskID > running.taskID)
		return CoordinationNotRunning;
	// equal after this point
	else if (running.taskID < m_tasks.size())
		return m_tasks[running.taskID]->GetState(coordinationID);

	return CoordinationNotRunning;
}

int CoordinationRoleSequence::GetCurrentTask(const CoordinationID& coordinationID) const
{
	RunningSequences::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningSequence& running = it->second;
	return running.taskID;
}


int CoordinationRoleSequence::GetCurrentTask(const CoordinationID& coordinationID, const CoordinationActor& actor) const
{
	RunningSequences::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningSequence& running = it->second;

	ActorStates::const_iterator ait = running.actors.find(actor);
	assert(ait != running.actors.end());

	const ActorState& actorState = ait->second;
	return actorState.taskID;
}

const char* CoordinationRoleSequence::GetTaskName(int taskID) const
{
	assert(taskID >= 0 && taskID < m_tasks.size());
	return m_tasks[taskID]->GetName();
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

ECoordinationState CoordinationRoleSequence::GetState(const CoordinationID& coordinationID) const
{
	RunningSequences::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningSequence& running = it->second;
	return running.state;
}

bool CoordinationRoleSequence::CanStart(const CoordinationInternalStartParams& params,
																				const CoordinationActors& assignees) const
{
	return true;
}

void CoordinationRoleSequence::Start(const CoordinationID& coordinationID, 
																		 const CoordinationActors& assignees)
{
	RunningSequences::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningSequences::iterator, bool> result = m_running.insert(RunningSequences::value_type(coordinationID, RunningSequence()));
	RunningSequence& running = result.first->second;

	running.taskID = 0;
	running.state = CoordinationRunning;

	const CoordinationRoles& roles = m_setup->GetRoles();
	const CoordinationInternalStartParams& params = m_setup->GetParams(coordinationID);

	CoordinationActors::const_iterator ait = assignees.begin();
	CoordinationActors::const_iterator aend = assignees.end();

	const char* roleName = roles[GetRoleID()].GetName();

	for ( ; ait != aend; ++ait)
	{
		const CoordinationActor& actor = *ait;
		ActorState& actorState = stl::map_insert_or_get(running.actors, actor, ActorState());
		actorState.taskID = 0;

		actor.NotifyPerforming(roleName);
	}

	CoordinationTask::Ptr& task = m_tasks[running.taskID];
	task->Start(coordinationID, assignees);
}

void CoordinationRoleSequence::Stop(const CoordinationID& coordinationID)
{
	RunningSequences::iterator it = m_running.find(coordinationID);

	if (it != m_running.end())
	{
		RunningSequence& running = it->second;

		uint32 taskCount = m_tasks.size();
		for (int taskID = running.taskID; taskID < taskCount; ++taskID)
		{
			ECoordinationState state = m_tasks[taskID]->GetState(coordinationID);
			if (state != CoordinationNotRunning)
				m_tasks[taskID]->Stop(coordinationID);
		}

		if (running.state == CoordinationWaiting)
		{
			ActorStates::iterator ait = running.actors.begin();
			ActorStates::iterator aend = running.actors.end();

			const CoordinationRoles& roles = m_setup->GetRoles();

			const char* roleName = roles[GetRoleID()].GetName();

			for ( ; ait != aend; ++ait)
				ait->first.NotifyFinished(roleName);
		}

		m_running.erase(it);
	}
}

void CoordinationRoleSequence::UpdateActor(const CoordinationID& coordinationID, RunningSequence& running,
																					 const CoordinationActor& actor, ActorState& actorState)
{
	CoordinationTask::Ptr task = m_tasks[actorState.taskID];
	ECoordinationState state = task->GetActorState(coordinationID, actor);
	
	assert(state != CoordinationNotRunning);

	if ((state == CoordinationWaiting) || (state == CoordinationFinished))
	{
		if (actorState.taskID < (m_tasks.size() - 1))
		{
			CoordinationTask::Ptr nextTask = m_tasks[actorState.taskID + 1];

			task->Stop(coordinationID, actor);
			++actorState.taskID;

			m_assignmentBuf.clear();
			m_assignmentBuf.push_back(actor);

			nextTask->Start(coordinationID, m_assignmentBuf);
		}
	}
}

void CoordinationRoleSequence::UpdateEndSyncMode(const CoordinationID& coordinationID, RunningSequence& running)
{
	ActorStates::iterator it = running.actors.begin();
	ActorStates::iterator end = running.actors.end();

	int earliestTaskID = INT_MAX;
	for ( ; it != end; ++it)
	{
		const CoordinationActor& actor = it->first;
		ActorState& actorState = it->second;

		UpdateActor(coordinationID, running, actor, actorState);
		if (actorState.taskID < earliestTaskID)
			earliestTaskID = actorState.taskID;
	}

	if (earliestTaskID < m_tasks.size())
	{
		for (int i = 0; i < earliestTaskID; ++i)
		{
			CoordinationTask::Ptr task = m_tasks[i];
			if (task->GetState(coordinationID) != CoordinationNotRunning)
				m_tasks[i]->Stop(coordinationID);
		}

		running.taskID = earliestTaskID;
	}

	if (earliestTaskID < m_tasks.size())
	{
		ECoordinationState state = m_tasks[earliestTaskID]->GetState(coordinationID);
		if (state != CoordinationRunning)
			running.state = CoordinationWaiting;
	}
	else
		running.state = CoordinationWaiting;
}

void CoordinationRoleSequence::UpdateTaskSyncMode(const CoordinationID& coordinationID, RunningSequence& running)
{
	ECoordinationState state = m_tasks[running.taskID]->GetState(coordinationID);
	if ((state == CoordinationRunning) || (state == CoordinationWaiting))
		return;

	while (state == CoordinationFinished)
	{
		if (running.taskID == (m_tasks.size() - 1))
		{
			running.state = CoordinationWaiting;
			break;
		}

		m_tasks[running.taskID]->Stop(coordinationID);
		++running.taskID;

		m_assignmentBuf.clear();

		ActorStates::iterator it = running.actors.begin();
		ActorStates::iterator end = running.actors.end();

		for ( ; it != end; ++it)
		{
			ActorState& actorState = it->second;
			actorState.taskID = running.taskID;

			m_assignmentBuf.push_back(it->first);
		}

		if (m_tasks[running.taskID]->CanStart(coordinationID, m_assignmentBuf))
		{
			m_tasks[running.taskID]->Start(coordinationID, m_assignmentBuf);
			state = m_tasks[running.taskID]->GetState(coordinationID);
		}
		else
		{
			state = CoordinationFailed;
			running.state = CoordinationFailed;
		}
	}

	if (running.state == CoordinationFailed)
	{
		const CoordinationRoles& roles = m_setup->GetRoles();

		const char* roleName = roles[GetRoleID()].GetName();

		ActorStates::iterator ait = running.actors.begin();
		ActorStates::iterator aend = running.actors.end();

		for ( ; ait != aend; ++ait)
			ait->first.NotifyFailed(roleName);
	}
}

void CoordinationRoleSequence::Update(float updateTime)
{
	if (!m_running.empty())
	{
		CoordinationTasks::iterator it = m_tasks.begin();
		CoordinationTasks::iterator end = m_tasks.end();

		for ( ; it != end; ++it)
			(*it)->Update(updateTime);

		RunningSequences::iterator sit = m_running.begin();
		RunningSequences::iterator send = m_running.end();

		for ( ; sit != send; ++sit)
		{
			const CoordinationID& coordinationID = sit->first;
			RunningSequence& running = sit->second;

			if (running.state != CoordinationRunning)
				continue;

			if (m_syncMode == End)
				UpdateEndSyncMode(coordinationID, running);
			else
				UpdateTaskSyncMode(coordinationID, running);
		}
	}
}

void CoordinationRoleSequence::RemoveActor(const CoordinationID& coordinationID, const CoordinationActor& actor)
{
	RunningSequences::iterator it = m_running.find(coordinationID);
	if (it == m_running.end())
	{
		RunningSequence& running = it->second;
		running.actors.erase(actor);
	}
}
