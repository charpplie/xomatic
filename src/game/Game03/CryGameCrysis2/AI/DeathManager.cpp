#include "StdAfx.h"
#include "DeathManager.h"
#include "Agent.h"
#include <IAISystem.h>

DeathManager::DeathManager()
{
	assert(gEnv->pAISystem);
	gEnv->pAISystem->RegisterListener(this);
}

DeathManager::~DeathManager()
{
	// ISystem already released here.
	//if (gEnv->pAISystem)
		//gEnv->pAISystem->UnregisterListener(this);
}

void DeathManager::AddPendingDeathReactionFor(const Agent& deadAgent)
{
	IAISystem& aiSystem = *gEnv->pAISystem;

	EntityId closestMemberID = 0;
	EntityId closestWitnessMemberID = 0;
	float closestDistSq = FLT_MAX;
	float closestWitnessDistSq = FLT_MAX;

	int groupID = deadAgent.GetGroupID();
	int memberCount = aiSystem.GetGroupCount(groupID);
	if (memberCount < 1)
		return;

	for (int memberIndex = 0; memberIndex < memberCount; ++memberIndex)
	{
		Agent member(aiSystem.GetGroupMember(groupID, memberIndex));

		if (member)
		{
			const bool witnessedDeath = member.CanSee(deadAgent);
			float distSq = SquaredDistance(member, deadAgent);

			if (witnessedDeath)
			{
				if (distSq < closestWitnessDistSq)
				{
					closestWitnessDistSq = distSq;
					closestWitnessMemberID = member.GetEntityID();
				}
			}

			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestMemberID = member.GetEntityID();
			}
		}
	}

	PendingDeathReaction pendingDeathReaction;
	pendingDeathReaction.groupID = groupID;
	pendingDeathReaction.framesLeftBeforeDispatch = 15;
	pendingDeathReaction.victimID = deadAgent.GetEntityID();
	pendingDeathReaction.deathPos = deadAgent.GetPos();
	pendingDeathReaction.closestMemberID = closestMemberID;
	pendingDeathReaction.closestWitnessMemberID = closestWitnessMemberID;

	m_pendingDeathReactions.push_back(pendingDeathReaction);
}

void DeathManager::ProcessPendingDeathReactions()
{
	std::vector<PendingDeathReaction>::iterator it = m_pendingDeathReactions.begin();
	std::vector<PendingDeathReaction>::iterator end = m_pendingDeathReactions.end();

	for ( ; it != end; ++it)
	{
		PendingDeathReaction& pdr = (*it);

		// Note: Maybe we should have a time value here too, in case the
		// player paused within 15 frames (500 ms) after killing someone.
		if ((--pdr.framesLeftBeforeDispatch) == 0)
		{
			GatherDataAndTriggerDeathReaction(pdr);

			std::swap(*it, m_pendingDeathReactions.back());
			m_pendingDeathReactions.pop_back();
			return;
		}
	}
}

void DeathManager::GatherDataAndTriggerDeathReaction(const PendingDeathReaction& pdr)
{
	//
	// Gather data
	//

	IAISystem& aiSystem = *gEnv->pAISystem;

	// Check if anyone is aware of threat (has threatening stimulus)
	bool awareOfThreat = false;
	Vec3 threatPos(ZERO);
	int memberCount = aiSystem.GetGroupCount(pdr.groupID);
	for (int memberIndex = 0; memberIndex < memberCount; ++memberIndex)
	{
		Agent member(aiSystem.GetGroupMember(pdr.groupID, memberIndex));

		if (member && (member.GetTargetThreat() >= AITHREAT_THREATENING))
		{
			IAIObject* pAttentionTarget = member.GetAttentionTarget();
			assert(pAttentionTarget);
			if (pAttentionTarget)
			{
				awareOfThreat = true;
				threatPos = pAttentionTarget->GetPos();
				break;
			}
		}
	}

	//
	// Trigger Death Reaction
	//

	// In the future I want to pass this information straight to
	// a sort of group behavior script table so that different group
	// behaviors can treat the information differently.
	// For now, however, I fill a global Lua table and send
	// the signal to an arbitrary group member. /Jonas

	Agent arbitraryMember(pdr.closestMemberID);

	if (arbitraryMember)
	{
		IScriptTable* pDeathContext = gEnv->pScriptSystem->CreateTable();
		pDeathContext->AddRef();
		if (pdr.closestWitnessMemberID)
			pDeathContext->SetValue("witnessID", ScriptHandle(pdr.closestWitnessMemberID));
		pDeathContext->SetValue("awareOfThreat", awareOfThreat);
		pDeathContext->SetValue("threatPos", threatPos);
		pDeathContext->SetValue("victimID", ScriptHandle(pdr.victimID));
		pDeathContext->SetValue("deathPos", pdr.deathPos);
		pDeathContext->SetValue("groupID", pdr.groupID);
		gEnv->pScriptSystem->SetGlobalValue("deathContext", pDeathContext);
		pDeathContext->Release();

		arbitraryMember.SetSignal(0, "OnMemberDeath");
	}



	Agent victim(pdr.victimID);

	if (victim)
	{
		AddDeadBodyFor(victim);
	}
}

void DeathManager::AddDeadBodyFor(const Agent& deadMember)
{
	m_unseenDeadBodies.push_back(DeadBody());
	m_unseenDeadBodies.back().Init(deadMember);
}

void DeathManager::CheckDeadBodyVisibilityFor(Agent& agent)
{
	std::vector<DeadBody>::iterator it = m_unseenDeadBodies.begin();
	std::vector<DeadBody>::iterator end = m_unseenDeadBodies.end();

	for ( ; it != end; ++it)
	{
		const DeadBody& deadBody = (*it);

		if (agent.CanSee(deadBody) && deadBody.IsOfSameGroupAs(agent))
		{
			agent.SetSignal(0, "OnDeadMemberSpotted");

			std::swap(*it, m_unseenDeadBodies.back());
			m_unseenDeadBodies.pop_back();
			return;
		}
	}
}

void DeathManager::ClearDeadBodiesForGroup(int groupID)
{
	for (unsigned int i = 0; i < m_unseenDeadBodies.size(); ++i)
	{
		const DeadBody& deadBody = m_unseenDeadBodies[i];

		if (deadBody.IsOfGroup(groupID))
		{
			std::swap(m_unseenDeadBodies[i], m_unseenDeadBodies.back());
			m_unseenDeadBodies.pop_back();
			--i;
		}
	}
}

void DeathManager::OnAgentDeath(EntityId deadEntityID)
{
	Agent deadAgent(deadEntityID);

	if (deadAgent)
	{
		const bool lastOneInGroupJustDied = (gEnv->pAISystem->GetGroupCount(deadAgent.GetGroupID()) == 0);
		if (lastOneInGroupJustDied)
			ClearDeadBodiesForGroup(deadAgent.GetGroupID());
		else
			AddPendingDeathReactionFor(deadAgent);
	}
}

void DeathManager::OnAgentUpdate(EntityId entityID)
{
	Agent agent(entityID);
	if (agent)
	{
		CheckDeadBodyVisibilityFor(agent);
	}
}
