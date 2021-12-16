#pragma once

#ifndef DeathManager_h
#define DeathManager_h

#include <IEntity.h>
#include <AISystemListener.h>
#include "DeadBody.h"

class Agent;

// At the very moment a group member gets killed we do not yet
// have all the data we need to pick the best reaction to it.
// We must wait for stimulus to propagate through the systems
// out to the agents.  Therefore we wait a few frames before
// we gather data and react.
struct PendingDeathReaction
{
	int groupID;
	int framesLeftBeforeDispatch;
	Vec3 deathPos;
	EntityId victimID;
	EntityId closestMemberID;
	EntityId closestWitnessMemberID;
};

class DeathManager : public IAISystemListener
{
public:
	DeathManager();
	virtual ~DeathManager();
	void ProcessPendingDeathReactions();

private:
	void AddPendingDeathReactionFor(const Agent& deadAgent);
	void GatherDataAndTriggerDeathReaction(const PendingDeathReaction& pdr);
	void AddDeadBodyFor(const Agent& deadMember);
	void CheckDeadBodyVisibilityFor(Agent& agent);
	void ClearDeadBodiesForGroup(int groupID);

	// IAISystemListener
	virtual void OnAgentDeath(EntityId deadEntityID);
	virtual void OnAgentUpdate(EntityId entityID);
	// ~IAISystemListener

private:
	std::vector<PendingDeathReaction> m_pendingDeathReactions;
	std::vector<DeadBody> m_unseenDeadBodies;
};

#endif // DeathManager_h
