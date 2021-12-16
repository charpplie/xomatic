#ifndef __CoordinationRoleSequence_h__
#define __CoordinationRoleSequence_h__

#pragma once


#include "CoordinationElement.h"
#include "CoordinationTask.h"


class CoordinationRoleSequence
{
	struct RunningSequence;
	struct ActorState;
public:
	CoordinationRoleSequence(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor);

	bool LoadFromXML(uint32 sequenceID, int roleID, const XmlNodeRef& rootNode);
	int GetRoleID() const;

	void DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const;

	ECoordinationState GetTaskState(const CoordinationID& coordinationID, int taskID) const;
	int GetCurrentTask(const CoordinationID& coordinationID) const;
	int GetCurrentTask(const CoordinationID& coordinationID, const CoordinationActor& actor) const;
	const char* GetTaskName(int taskID) const;

	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationInternalStartParams& params, const CoordinationActors& assignees) const;
	virtual void Start(const CoordinationID& coordinationID, const CoordinationActors& assignees);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Update(float frameTime);

	void UpdateEndSyncMode(const CoordinationID& coordinationID, RunningSequence& running);
	void UpdateTaskSyncMode(const CoordinationID& coordinationID, RunningSequence& running);

	void UpdateActor(const CoordinationID& coordinationID, RunningSequence& running, const CoordinationActor& actor, 
		ActorState& state);
	void RemoveActor(const CoordinationID& coordinationID, const CoordinationActor& actor);

private:
	enum ESyncMode
	{
		End = 0,
		Task,
	};

	typedef struct ActorState
	{
		int taskID;
	};
	typedef std::map<CoordinationActor, ActorState> ActorStates;

	typedef struct RunningSequence
	{
		RunningSequence()
			: state(CoordinationRunning)
		{
		}

		ActorStates actors;
		ECoordinationState state;
		int taskID;
	};

	typedef std::map<CoordinationID, RunningSequence> RunningSequences;
	RunningSequences m_running;

	CoordinationTasks m_tasks;

	ESyncMode m_syncMode;
	int m_roleID;
	const CoordinationSetup* m_setup;
	const CoordinationTaskMonitor* m_taskMonitor;

	CoordinationActors m_assignmentBuf;
};


class RoleSequences :
	public std::vector<CoordinationRoleSequence>
{
};


#endif //__CoordinationRoleSequence_h__