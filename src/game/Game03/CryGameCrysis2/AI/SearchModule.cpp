#include "StdAfx.h"
#include "SearchModule.h"
#include <IAgent.h>
#include <IAISystem.h>
#include <IAIObject.h>
#include <IRenderAuxGeom.h>
#include <HidespotQueryContext.h>
#include "Agent.h"
#include "GameCVars.h"

// Question: What happens if a member locks a search spot and then dies?
// Proposal: We could register the search module as an entity listener... but that might be too extreme.
// Solution: TBD

// TODO: Query for search spots through the TPS

SearchSpot::SearchSpot()
: m_pos(ZERO)
, m_status(NotSearchedYet)
, m_assigneeID(0)
{

}

SearchSpot::~SearchSpot()
{
	if (m_visionID)
	{
		assert(gEnv->pAISystem);
		IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();
		visionMap.UnregisterObservable(m_visionID);
		m_visionID = VisionID();
	}
}

void SearchSpot::Init(const Vec3& pos)
{
	assert(pos.IsValid());

	m_pos = pos;
	m_status = NotSearchedYet;

	assert(!m_visionID);
	if (!m_visionID)
	{
		assert(gEnv->pAISystem);
		IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();

		static unsigned int searchSpotCounter = 0;
		string visionName;
		visionName.Format("SearchSpot%d", searchSpotCounter++);
		m_visionID = visionMap.CreateVisionID(visionName);

		ObservableParams observableParams;
		observableParams.posCount = 1;
		observableParams.pos[0] = pos;
		visionMap.RegisterObservable(m_visionID, observableParams);
	}
}

void SearchSpot::DebugDraw()
{
	ColorB spotColor;

	switch (m_status)
	{
	case NotSearchedYet:
		spotColor = ColorB(0, 0, 255);
		break;
	case BeingSearchedRightAboutNow:
		spotColor = ColorB(255, 255, 0);
		break;
	case Searched:
		spotColor = ColorB(0, 255, 0);
		break;
	case Unreachable:
		spotColor = ColorB(255, 0, 0);
		break;
	}

	IRenderAuxGeom* pDebugRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
	pDebugRenderer->DrawSphere(m_pos, 0.3f, spotColor);

	if (m_assigneeID)
	{
		Agent agent(m_assigneeID);
		if (agent)
			pDebugRenderer->DrawLine(agent.GetPos(), ColorB(255, 255, 0), m_pos, ColorB(255, 255, 0), 2.0f);
	}
}

bool SearchSpot::IsAssignedTo(EntityId entityID) const
{
	return m_assigneeID == entityID;
}

void SearchSpot::AssignTo(EntityId entityID)
{
	m_assigneeID = entityID;
}

void SearchSpot::MarkAsSearchedBy(SearchActor& participant)
{
	if (IsAssignedTo(participant.entityID))
	{
		Agent agent(participant.entityID);
		agent.SetSignal(0, "OnAssignedSearchSpotSeen");
		m_assigneeID = 0;
	}

	assert(m_assigneeID == 0);

	m_status = Searched;
}

void SearchSpot::MarkAsUnreachable()
{
	m_status = Unreachable;
	m_assigneeID = 0;
}

void SearchGroup::Init(int groupID, const Vec3& targetPos)
{
	m_targetPos = targetPos;

	StoreActors(groupID);
	GenerateSearchSpots();
}

void SearchGroup::Destroy()
{
	IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();

	SearchActorIter it = m_actors.begin();
	SearchActorIter end = m_actors.end();

	for ( ; it != end; ++it)
	{
		SearchActor& actor = *it;
		visionMap.UnregisterObserver(actor.visionID);
	}

	m_actors.clear();
}

void SearchGroup::StoreActors(int groupID)
{
	IAISystem& aiSystem = *gEnv->pAISystem;
	IVisionMap& visionMap = aiSystem.GetVisionMap();

	assert(m_actors.empty());
	m_actors.clear(); // TODO: Remove when implementation is done!

	int groupSize = aiSystem.GetGroupCount(groupID);
	m_actors.reserve(groupSize);
	for (int i = 0; i < groupSize; ++i)
	{
		IAIObject* pAI = aiSystem.GetGroupMember(groupID, i);
		if (pAI)
		{
			EntityId entityID = pAI->GetEntityID();
			VisionID visionID = visionMap.CreateVisionID(pAI->GetName());

			ObserverParams observerParams;
			observerParams.entityID = entityID;
			observerParams.factionMask = 0xFFFFFFFF;
			observerParams.typeMask = 0xFFFFFFFF;
			observerParams.eyePos = pAI->GetPos();
			observerParams.eyeDir = pAI->GetViewDir();

			observerParams.primaryFoVCos = cosf(60.0f);
			observerParams.peripheralFoVCos = cosf(120.0f);
			observerParams.sightRange = 8.0f;

			visionMap.RegisterObserver(visionID, observerParams);

			m_actors.push_back(SearchActor(entityID, visionID));
		}
	}
}

void SearchGroup::GenerateSearchSpots()
{
	assert(m_searchSpots.empty());
	m_searchSpots.clear(); // TODO: Remove when implementation is done!

	const unsigned int maxHideSpots = 128;
	Vec3 coverPos[maxHideSpots];
	Vec3 coverObjPos[maxHideSpots];
	Vec3 coverObjDir[maxHideSpots];

	HidespotQueryContext query;
	query.centerOfQuery = m_targetPos;
	query.maxRange = 40.0f;
	query.onlyThoseThatGiveCover = false;
	query.maxPoints = maxHideSpots;
	query.pCoverPos = coverPos;
	query.pCoverObjPos = coverObjPos;
	query.pCoverObjDir = coverObjDir;

	IAISystem& aiSystem = *gEnv->pAISystem;
	unsigned int hideSpotCount = aiSystem.GetHideSpots(query);

	m_searchSpots.resize(hideSpotCount);
	for (unsigned int i = 0; i < hideSpotCount; ++i)
	{
		SearchSpot& spot = m_searchSpots[i];
		spot.Init(coverPos[i]);
	}
}

float SearchGroup::CalculateScore(SearchSpot& searchSpot, EntityId entityID, SearchSpotQuery* query) const
{
	assert(query);

	Agent agent(entityID);

	const float closenessToAgentScore  = 1.0f - clamp(searchSpot.GetPos().GetDistance(agent.GetPos()), 1.0f, 50.0f) / 50.0f;
	const float closenessToTargetScore = 1.0f - clamp(searchSpot.GetPos().GetDistance(m_targetPos),    1.0f, 50.0f) / 50.0f;

	return
		closenessToAgentScore  * query->closenessToAgentWeight +
		closenessToTargetScore * query->closenessToTargetWeight;
}

void SearchGroup::Update()
{
	IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();

	// Update vision
	{
		std::vector<SearchActor>::iterator actorIt = m_actors.begin();
		std::vector<SearchActor>::iterator actorEnd = m_actors.end();

		for ( ; actorIt != actorEnd; ++actorIt)
		{
			SearchActor& actor = (*actorIt);

			Agent agent(actor.entityID);

			ObserverParams observerParams;
			observerParams.eyePos = agent.GetPos();
			observerParams.eyeDir = agent.GetViewDir();

			visionMap.ObserverChanged(actor.visionID, observerParams, eChangedPosition | eChangedOrientation);
		}
	}

	// Debug draw target pos
	if (g_pGameCVars->ai_DebugSearch)
	{
		IRenderAuxGeom* pDebugRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
		pDebugRenderer->DrawSphere(m_targetPos, 0.6f, ColorB(255, 255, 255));
	}

	std::vector<SearchSpot>::iterator spotIt = m_searchSpots.begin();
	std::vector<SearchSpot>::iterator spotEnd = m_searchSpots.end();

	for ( ; spotIt != spotEnd; ++spotIt)
	{
		SearchSpot& searchSpot = (*spotIt);

		if (g_pGameCVars->ai_DebugSearch)
			searchSpot.DebugDraw();

		if (searchSpot.HasBeenSearched())
			continue;

		// Naive Implementation!
		// Go through all the actors and see
		// if they see any of the search spots.
		// Later on, use a callback for this!

		SearchActorIter actorIt = m_actors.begin();
		std::vector<SearchActor>::iterator actorEnd = m_actors.end();

		for ( ; actorIt != actorEnd; ++actorIt)
		{
			SearchActor& actor = *actorIt;

			if (searchSpot.IsBeingSearched())
			{
				if (searchSpot.IsAssignedTo(actor.entityID) && visionMap.IsVisible(actor.visionID, searchSpot))
				{
					searchSpot.MarkAsSearchedBy(actor);
					break;
				}
			}
			else if (visionMap.IsVisible(actor.visionID, searchSpot))
			{
				searchSpot.MarkAsSearchedBy(actor);
				break;
			}
		}
	}
}

bool SearchGroup::GetNextSearchPoint(EntityId entityID, SearchSpotQuery* query)
{
	if (SearchSpot* best = FindBestSearchSpot(entityID, query))
	{
		assert(query);
		query->result = best->GetPos();
		best->m_status = BeingSearchedRightAboutNow;
		best->m_assigneeID = entityID;
		return true;
	}

	return false;
}

void SearchGroup::MarkAssignedSearchSpotAsUnreachable(EntityId entityID)
{
	SearchSpot* assignedSpot = GetAssignedSearchSpot(entityID);

	if (assignedSpot)
	{
		//assignedSpot->MarkAsUnreachable();
		assignedSpot->m_status = Unreachable;
		assignedSpot->m_assigneeID = 0;
	}
}

SearchSpot* SearchGroup::FindBestSearchSpot(EntityId entityID, SearchSpotQuery* query)
{
	SearchSpotIter it = m_searchSpots.begin();
	SearchSpotIter end = m_searchSpots.end();

	SearchSpot* best = NULL;
	float bestScore = FLT_MIN;

	for ( ; it != end; ++it)
	{
		SearchSpot& searchSpot = (*it);

		if (searchSpot.GetStatus() == NotSearchedYet)
		{
			float score = CalculateScore(searchSpot, entityID, query);
			if (score > bestScore)
			{
				best = &searchSpot;
				bestScore = score;
			}
		}
	}

	return best;
}

SearchSpot* SearchGroup::GetAssignedSearchSpot(EntityId entityID)
{
	// TODO: Fix this! It is implemented in a suuuuuper naive way.
	// It's going through all search spots and sees if it
	// has the agent as an assignee.  We should have an internal
	// representation of the agents so we can store this reference back.

	SearchSpotIter it = m_searchSpots.begin();
	SearchSpotIter end = m_searchSpots.end();

	for ( ; it != end; ++it)
	{
		SearchSpot& spot = (*it);

		//if (searchSpot.IsAssignedTo(entityID))
		if (spot.m_assigneeID == entityID)
			return &spot;
	}

	return NULL;
}

SearchModule* SearchModule::GetInstance()
{
	static SearchModule instance;
	return &instance;
}

void SearchModule::Enter(EntityId entityID)
{

}

void SearchModule::Leave(EntityId entityID)
{

}

void SearchModule::Reset()
{
	SearchGroupIter it = m_groups.begin();
	SearchGroupIter end = m_groups.end();

	for ( ; it != end; ++it)
	{
		SearchGroup& group = it->second;
		group.Destroy();
	}

	m_groups.clear();
}

void SearchModule::Update(float dt)
{
	SearchGroupIter it = m_groups.begin();
	SearchGroupIter end = m_groups.end();

	for ( ; it != end; ++it)
	{
		SearchGroup& group = it->second;
		group.Update();
	}
}

void SearchModule::GroupEnter(int groupID, const Vec3& targetPos)
{
	assert(!GroupExist(groupID));

	SearchGroup& group = m_groups[groupID];
	group.Init(groupID, targetPos);
}

void SearchModule::GroupLeave(int groupID)
{
	SearchGroupIter it = m_groups.find(groupID);

	if (it != m_groups.end())
	{
		SearchGroup& group = it->second;
		group.Destroy();
		m_groups.erase(it);
	}
}

bool SearchModule::GetNextSearchPoint(EntityId entityID, SearchSpotQuery* query)
{
	Agent agent(entityID);
	if (agent)
	{
		SearchGroup* group = GetGroup(agent.GetGroupID());
		if (group)
			return group->GetNextSearchPoint(entityID, query);
	}

	return false;
}

void SearchModule::MarkAssignedSearchSpotAsUnreachable(EntityId entityID)
{
	Agent agent(entityID);
	if (agent)
	{
		SearchGroup* group = GetGroup(agent.GetGroupID());
		if (group)
			return group->MarkAssignedSearchSpotAsUnreachable(entityID);
	}
}

bool SearchModule::GroupExist(int groupID) const
{
	return m_groups.find(groupID) != m_groups.end();
}

SearchGroup* SearchModule::GetGroup(int groupID)
{
	SearchGroupIter it = m_groups.find(groupID);

	if (it != m_groups.end())
		return &it->second;

	return NULL;
}
