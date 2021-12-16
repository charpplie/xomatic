#ifndef __CoordinationTaskCommunication_h__
#define __CoordinationTaskCommunication_h__

#pragma once


#include "CoordinationTask.h"
#include "CoordinationTaskSync.h"
#include "Communication/Communication.h"


class CoordinationTaskCommunication
	: public CoordinationTask
{
public:
	CoordinationTaskCommunication(const CoordinationSetup* setup, const CoordinationTaskMonitor* taskMonitor);
	~CoordinationTaskCommunication();

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
		CommunicationPlayID playID;
		ECoordinationState state;
	};

	typedef std::map<CoordinationActor, ActorState> ActorStates;

	typedef struct RunningCommunication
		: public ICommunicationManager::IEventListener
	{
		RunningCommunication()
			: state(CoordinationRunning)
			, activeCount(0)
		{
		}

		virtual void OnCommunicationEvent(ICommunicationManager::ECommunicationEvent event, EntityId actorID, 
			const CommunicationPlayID& playID);

		ActorStates actors;
		uint32 activeCount;
		ECoordinationState state;
	};

	typedef std::map<CoordinationID, RunningCommunication> RunningCommunications;
	RunningCommunications m_running;

	float m_timeout;

	CommunicationID m_commID;
	CommunicationChannelID m_channelID;
	SCommunicationRequest::EOrdering m_ordering;
	float m_expirity;
		
	CoordinationTaskSync m_sync;

	const CoordinationSetup* m_setup;
	const CoordinationTaskMonitor* m_taskMonitor;
};


#endif //__CoordinationTaskCommunication_h__