#ifndef __CoordinationTask_h__
#define __CoordinationTask_h__

#pragma once


#include "Coordination.h"


class CoordinationTask
	: public _reference_target_t
{
public:
	typedef _smart_ptr<CoordinationTask> Ptr;

public:
	virtual ~CoordinationTask()
	{
	}

	virtual const char* GetName() const = 0;
	virtual const char* GetDescription() const = 0;
	virtual ECoordinationState GetActorState(const CoordinationID& coordinationID, const CoordinationActor& actor) const = 0;
	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const = 0;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationActors& assignees) const = 0;
	virtual void Start(const CoordinationID& coordinationID, const CoordinationActors& assignees) = 0;
	virtual void Stop(const CoordinationID& coordinationID) = 0;
	virtual void Stop(const CoordinationID& coordinationID, const CoordinationActor& actor) = 0;
	virtual void Update(float frameTime) = 0;
};


class CoordinationTasks :
	public std::vector<CoordinationTask::Ptr>
{
};


#endif //__CoordinationTask_h__
