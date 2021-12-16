#ifndef __CoordinationActor_h__
#define __CoordinationActor_h__

#pragma once


#include "Communication/Communication.h"


class CAIObject;
class CoordinationActor
{
public:
	CoordinationActor()
	{
	}

	CoordinationActor(IAIObject* object)
	{
		m_object.Assign(static_cast<CAIObject*>(object)->GetSelfReference());
	}

	CoordinationActor(EntityId entityID)
	{
		IEntity* entity = gEnv->pEntitySystem->GetEntity(entityID);
		if (entity)
			m_object.Assign(static_cast<CAIObject*>(entity->GetAI())->GetSelfReference());
	}

	EntityId GetEntityID() const;
	CommunicationConfigID GetCommunicationConfigID() const;

	const char* GetName() const;
	IScriptTable* GetScript() const;

	void Signal(const char* signal) const;
	void Action(const char* name) const;

	void NotifyPerforming(const char* roleName) const;
	void NotifyFinished(const char* roleName) const;
	void NotifyFailed(const char* roleName) const;

	void RegisterBehaviorListener(IActorBehaviorListener* listener) const;
	void UnregisterBehaviorListener(IActorBehaviorListener* listener) const;

public:

	bool operator ==(const CoordinationActor& other) const
	{
		return m_object == other.m_object;
	}

	bool operator !=(const CoordinationActor& other) const
	{
		return m_object != other.m_object;
	}

	bool operator <(const CoordinationActor& other) const
	{
		return m_object < other.m_object;
	}

private:
	CWeakRef<CAIObject> m_object; // TODO(márcio): change this to something quicker
};


class CoordinationActors :
	public std::vector<CoordinationActor>
{
};


#endif //__CoordinationActor_h__