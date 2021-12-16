#ifndef __CoordinationTaskCoordination_h__
#define __CoordinationTaskCoordination_h__

#pragma once


#include "CoordinationTask.h"
#include "CoordinationTaskSync.h"



class CoordinationTaskCoordination
	: public CoordinationTask
{
public:
	CoordinationTaskCoordination(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor);

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
	typedef struct RunningCoordination
	{
		RunningCoordination()
			: state(CoordinationRunning)
			, timeout(0.0f)
			, finished(false)
			, coordID(0)
		{
		}

		ECoordinationState state;

		float timeout;
		bool finished;
		CoordinationID coordID;
	};

	float m_timeout;
	CoordinationSetupID m_coordSetupID;
	
	typedef std::map<CoordinationID, RunningCoordination> RunningCoordinations;
	RunningCoordinations m_running;

	CoordinationTaskSync m_sync;

	const CoordinationSetup* m_setup;
	const CoordinationTaskMonitor* m_taskMonitor;
};


#endif