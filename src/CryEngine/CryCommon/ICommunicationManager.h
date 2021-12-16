#ifndef __ICommunicationManager_h__
#define __ICommunicationManager_h__

#pragma  once


typedef uint32 CommunicationConfigID;
typedef uint32 CommunicationChannelID;
typedef uint32 CommunicationID;
typedef uint32 CommunicationPlayID;


struct SCommunicationRequest;


struct ICommunicationManager
{
	enum ECommunicationEvent
	{
		CommunicationQueued = 0,
		CommunicationExpired,
		CommunicationStarted,
		CommunicationCancelled,
		CommunicationFinished,
	};

	struct IEventListener
	{
		virtual void OnCommunicationEvent(ECommunicationEvent event, EntityId actorID, const CommunicationPlayID& playID) = 0;
	};

	virtual uint32 GetConfigCount() const = 0;
	virtual const char* GetConfigName(uint32 index) const = 0;
};


struct SCommunicationRequest
{
	SCommunicationRequest()
		: configID(0)
		, channelID(0)
		, commID(0)
		, actorID(0)
		, targetID(0)
		, target(ZERO)
		, ordering(Unordered)
		, contextExpirity(0.0f)
		, eventListener(0)
	{
	}

	enum EOrdering
	{
		Ordered = 0,
		Unordered = 1,
	};

	CommunicationConfigID configID;
	CommunicationChannelID channelID;
	CommunicationID commID;

	EntityId actorID;

	EntityId targetID;
	Vec3 target;

	EOrdering ordering;
	float contextExpirity;

	ICommunicationManager::IEventListener* eventListener;
};

#endif