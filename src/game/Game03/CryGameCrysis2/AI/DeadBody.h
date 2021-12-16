#pragma once

#ifndef DeadBody_h
#define DeadBody_h

#include <IEntity.h>
#include <IVisionMap.h>

class Agent;

// Contains information about a dead body that can be spotted by
// other team members.  It is registered as an observable in the
// vision map.
class DeadBody
{
public:
	DeadBody();
	~DeadBody();

	void Init(const Agent& agent);

	operator const VisionID& () const
	{
		return m_visionID;
	}

	int GetGroupID() const
	{
		return m_groupID;
	}

	EntityId GetEntityID() const
	{
		return m_entityID;
	}

	bool IsOfSameGroupAs(const Agent& agent) const;
	bool IsOfGroup(int otherGroupID) const { return m_groupID == otherGroupID; }

private:
	int m_groupID;
	EntityId m_entityID;
	VisionID m_visionID;
};

#endif // DeadBody_h
