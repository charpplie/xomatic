#ifndef __CoordinationStep_h__
#define __CoordinationStep_h__

#pragma once


#include "CoordinationElement.h"
#include "CoordinationRoleSequence.h"


class CoordinationPlan;

class CoordinationStep :
	public CoordinationElement,
	public CoordinationTaskMonitor
{
public:
	explicit CoordinationStep(const CoordinationSetup* setup);

	uint32 GetRunningCount() const;
	const CoordinationID& GetRunningCoordinationID(uint32 index) const;
	uint32 GetSequenceCount() const;
	const CoordinationRoleSequence& GetSequence(uint32 index) const;
	
	// CoordinationElement
	virtual bool LoadFromXML(uint32 elementId, const XmlNodeRef& rootNode);

	virtual void DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const;

	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const;
	virtual void Start(const CoordinationID& coordinationID);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Update(float updateTime);
	// ~CoordinationElement

	// CoordinationTaskMonitor
	virtual ECoordinationState GetTaskState(const CoordinationID& coordinationID, int roleID, int taskID) const;
	virtual ECoordinationState GetRoleState(const CoordinationID& coordinationID, int roleID) const;
	virtual int GetCurrentTask(const CoordinationID& coordinationID, int roleID) const;
	virtual const char* GetTaskName(int roleID, int taskID) const;
	// ~CoordinationTaskMonitor

private:
	typedef struct RunningStep
	{
		RunningStep()
			: state(CoordinationRunning)
		{
		}

		ECoordinationState state;
	};

	typedef std::map<CoordinationID, RunningStep> RunningSteps;
	RunningSteps m_running;

	RoleSequences m_roleSequences;
	string m_binderName;

	const CoordinationSetup* m_setup;
};


#endif //__CoordinationStep_h__