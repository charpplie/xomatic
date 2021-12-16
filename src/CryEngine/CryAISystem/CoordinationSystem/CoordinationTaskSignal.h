#ifndef __CoordinationTaskSignal_h__
#define __CoordinationTaskSignal_h__

#pragma once


#include "CoordinationTask.h"
#include "CoordinationTaskSync.h"


class CoordinationTaskSignal
	: public CoordinationTask
{
public:
	CoordinationTaskSignal(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor);

	bool LoadFromXML(int roleID, const XmlNodeRef& rootNode);

	virtual const char* GetName() const;
	virtual const char* GetDescription() const;
	virtual ECoordinationState GetActorState(const CoordinationID& coordinationID, const CoordinationActor& actor) const;
	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationActors& assignees) const;
	virtual void Start(const CoordinationID& coordinationID, const CoordinationActors& assignees);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Stop(const CoordinationID& coordinationID, const CoordinationActor& actor);
	virtual void Update(float frameTime);

private:
	typedef struct RunningSignal
	{
		ECoordinationState state;
	};

	typedef std::map<CoordinationID, RunningSignal> RunningSignals;
	RunningSignals m_running;

	string m_signalName;

	CoordinationTaskSync m_sync;

	const CoordinationSetup* m_setup;
	const CoordinationTaskMonitor* m_taskMonitor;
};


#endif //__CoordinationTaskSignal_h__