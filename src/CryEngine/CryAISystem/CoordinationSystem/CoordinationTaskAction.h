#ifndef __CoordinationTaskAction_h__
#define __CoordinationTaskAction_h__

#pragma once


#include "CoordinationTask.h"
#include "CoordinationTaskSync.h"


class CoordinationTaskAction
	: public CoordinationTask
{
public:
	CoordinationTaskAction(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor);

	bool LoadFromXML(int roleID, const XmlNodeRef& rootNode);

	virtual const char* GetName() const;
	virtual const char* GetDescription() const;
	virtual ECoordinationState GetActorState(const CoordinationID& coordinationID, const CoordinationActor& actor) const;
	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationActors& assignees) const;
	virtual void Start(const CoordinationID& coordinationID, const CoordinationActors& assignees);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Stop(const CoordinationID& coordinationID, const CoordinationActor& actor);
	virtual void Update(float updateTime);
private:
	typedef struct ActorState
	{
		float timeout;
		ECoordinationState state;
	};

	typedef std::map<CoordinationActor, ActorState> ActorStates;

	typedef struct RunningAction :
		public IActorBehaviorListener
	{
		RunningAction()
			: state(CoordinationRunning)
			, activeCount(0)
		{
		}
		~RunningAction();

		ActorStates actors;
		uint32 activeCount;
		ECoordinationState state;

		virtual void BehaviorEvent(IAIObject* actor, EBehaviorEvent event);
		virtual void BehaviorChanged(IAIObject* actor, const char* current, const char* previous);
	};

	typedef std::map<CoordinationID, RunningAction> RunningActions;
	RunningActions m_running;

	float m_timeout;
	string m_actionName;
	string m_stopName;

	CoordinationTaskSync m_sync;

	const CoordinationSetup* m_setup;
	const CoordinationTaskMonitor* m_taskMonitor;
};


#endif //__CoordinationTaskAction_h__