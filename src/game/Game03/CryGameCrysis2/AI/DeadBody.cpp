#include "StdAfx.h"
#include "DeadBody.h"
#include "Agent.h"
#include <IAISystem.h>

DeadBody::DeadBody()
: m_entityID(0)
, m_groupID(0)
{

}

void DeadBody::Init(const Agent& agent)
{
	IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();

	m_groupID = agent.GetGroupID();
	m_entityID = agent.GetEntityID();
	m_visionID = visionMap.CreateVisionID(agent.GetName());

	ObservableParams observableParams;
	observableParams.faction = agent.GetSpecies();
	observableParams.type = 0;
	observableParams.posCount = 1;
	observableParams.pos[0] = agent.GetPos();
	visionMap.RegisterObservable(m_visionID, observableParams);
}

DeadBody::~DeadBody()
{
	if (m_visionID)
	{
		assert(gEnv->pAISystem);
		IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();
		visionMap.UnregisterObservable(m_visionID);
	}
}

bool DeadBody::IsOfSameGroupAs(const Agent& agent) const
{
	return m_groupID == agent.GetGroupID();
}
