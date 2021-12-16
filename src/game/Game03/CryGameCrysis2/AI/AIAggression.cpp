#include "StdAfx.h"
#include "AIAggression.h"

#include "GameCVars.h"
#include "IDebugHistory.h"



CAIAggressionInstance::CAIAggressionInstance(EntityId _entityID)
: CGameAIInstanceBase(_entityID)
, m_current(0.0f)
, m_increaseRate(0.05f)
{

}

void CAIAggressionInstance::ReadParams()
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(GetEntityID());
	assert(entity);
	if (!entity)
		return;

	SmartScriptTable aggressionParams;
	IScriptTable* scriptTable = entity->GetScriptTable();
	if(!scriptTable || !scriptTable->GetValue("AggressionParams", aggressionParams))
		return;

	aggressionParams->GetValue("increaseRate", m_increaseRate);
}

void CAIAggressionInstance::Update(float frameTime)
{
	IncreaseAggression(m_increaseRate * frameTime);
}

void CAIAggressionInstance::DebugDraw()
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(GetEntityID());
	if (entity)
	{
		Vec3 pos = entity->GetPos();
		pos.z += 2.0f;

		float underAggressionColor[4] = {1,1,1,1};	

		float ratio = max(1.0f -(m_current/*/threshold*/), 0);
		float aggressionValueColor[4] = {1,ratio,ratio,1};

		gEnv->pRenderer->DrawLabelEx(pos, 1, underAggressionColor, true, false, "Aggression");
		gEnv->pRenderer->DrawLabelEx(pos, 1, aggressionValueColor, true, false, "        - %.1f", m_current);
	}
}

void CAIAggressionInstance::IncreaseAggression(float amount)
{
	if (m_current < 1.0f)
	{
		m_current += amount;

		if (m_current >= 1.0f)
		{
			m_current = 1.0f;
			SendSignal("OnAggressionOutburst");
		}
	}
}

CAIAggressionModule::CAIAggressionModule()
: m_statsTargetVar(0)
{
	m_debugHistoryManager = g_pGame->GetIGameFramework()->CreateDebugHistoryManager();
	memset(m_AggressionGraphOccupied, 0, sizeof(m_AggressionGraphOccupied));
}

CAIAggressionModule::~CAIAggressionModule()
{
	SAFE_RELEASE(m_debugHistoryManager);
}

void CAIAggressionModule::Enter(EntityId entityID)
{
	Leave(entityID);
	CAIAggressionInstance* instance = CreateAggressionInstance(entityID);
//Diesel cut	
	//m_runningInstances.insert( std::make_pair<EntityId, CAIAggressionInstance*>(entityID, instance) );
//Diesel new
	m_runningInstances.insert(std::make_pair(entityID, instance));

}

void CAIAggressionModule::Leave(EntityId entityID)
{
	LeaveFromMap(m_runningInstances, entityID);
	LeaveFromMap(m_pausedInstances, entityID);
}

void CAIAggressionModule::Pause(EntityId entityID)
{
	CAIAggressionInstance* instance = GetRunningInstance(entityID);
	if (instance)
	{
		DetachDebugGraph(instance);
		MoveFromOneMapToAnother(entityID, m_runningInstances, m_pausedInstances);
	}
}

void CAIAggressionModule::Resume(EntityId entityID)
{
	MoveFromOneMapToAnother(entityID, m_pausedInstances, m_runningInstances);
	CAIAggressionInstance* instance = GetRunningInstance(entityID);
	if (instance)
	{
		AttachDebugGraph(instance);
	}
}

void CAIAggressionModule::Update(float frameTime)
{
	if (!m_statsTargetVar)
		m_statsTargetVar = gEnv->pConsole->GetCVar("ai_StatsTarget");
	
	const char* statsTarget = m_statsTargetVar->GetString();
	bool viewAllGraphs = (strcmp(statsTarget, "none") == 0);
	std::map<EntityId, CAIAggressionInstance*>::iterator it = m_runningInstances.begin();

	const bool debugAggression = (g_pGameCVars->ai_DebugAggressionSystem != 0);

	while (it != m_runningInstances.end())
	{
		CAIAggressionInstance* instance = it->second;
		instance->Update(frameTime);

		if (debugAggression)
		{
			instance->DebugDraw();
		}

		// TODO: Move the following section into DebugDraw!
		if (instance->graphIndex != UINT_MAX)
		{
			IDebugHistory* debugHistory = m_debugHistoryManager->GetHistory(instance->GetDebugEntityName().c_str());
			if (debugHistory)
			{
				debugHistory->AddValue(instance->m_current);

				if (debugAggression && (viewAllGraphs || strcmp(instance->GetDebugEntityName().c_str(), statsTarget) == 0))
				{
					debugHistory->SetVisibility(true);
				}
				else
				{
					debugHistory->SetVisibility(false);
				}
			}
		}

		++it;
	}
}

const char* CAIAggressionModule::GetName() const
{
	return "Aggression";
}

ConsistencyStatus CAIAggressionModule::CheckConsistency(std::vector<SConsistencyFailEntry>* missingEntities)
{
	assert(missingEntities);
	missingEntities->clear();

	for (std::map<EntityId, CAIAggressionInstance*>::iterator it = m_runningInstances.begin(); it != m_runningInstances.end(); ++it)
	{
		IEntity* entity = gEnv->pEntitySystem->GetEntity(it->first);
		if (!entity)
		{
			SConsistencyFailEntry entry;
			entry.entityID = it->first;
			entry.name = it->second->GetDebugEntityName();
			missingEntities->push_back(entry);
		}
	}

	if (missingEntities->empty())
	{
		return ConsistencyStatus_OK;
	}
	else
	{
		return ConsistencyStatus_MissingEntities;
	}
}

CAIAggressionInstance* CAIAggressionModule::GetRunningInstance(EntityId entityID)
{
	std::map<EntityId, CAIAggressionInstance*>::iterator it = m_runningInstances.find(entityID);

	if (it == m_runningInstances.end())
		return NULL;

	return it->second;
}

CAIAggressionModule* CAIAggressionModule::GetInstance()
{
	static CAIAggressionModule instance;
	return &instance;
}

bool CAIAggressionModule::FindFreeGraphSlot(unsigned int* graphSlotIndex)
{
	assert(graphSlotIndex);

	for (unsigned int i = 0; i < 16; ++i)
	{
		if (!m_AggressionGraphOccupied[i])
		{
			*graphSlotIndex = i;
			return true;
		}
	}

	return false;
}

CAIAggressionInstance* CAIAggressionModule::CreateAggressionInstance(EntityId entityID)
{
	CAIAggressionInstance* instance = new CAIAggressionInstance(entityID); // TODO: Get from pool
	instance->ReadParams();
	AttachDebugGraph(instance);
	return instance;
}

void CAIAggressionModule::DestroyAggressionInstance(CAIAggressionInstance* instance)
{
	DetachDebugGraph(instance);
	delete instance; // TODO: Remove from pool
}

void CAIAggressionModule::LeaveFromMap(std::map<EntityId, CAIAggressionInstance*>& instancesMap, EntityId entityID)
{
	std::map<EntityId, CAIAggressionInstance*>::iterator it = instancesMap.find(entityID);
	if (it != instancesMap.end())
	{
		CAIAggressionInstance* instance = it->second;
		DestroyAggressionInstance(instance);
		instancesMap.erase(it);
	}
}

void CAIAggressionModule::AttachDebugGraph(CAIAggressionInstance* instance)
{
	unsigned int graphSlotIndex;
	if (FindFreeGraphSlot(&graphSlotIndex))
	{
		m_AggressionGraphOccupied[graphSlotIndex] = true;
		instance->graphIndex = graphSlotIndex;

		unsigned int x = graphSlotIndex % 4;
		unsigned int y = graphSlotIndex / 4;

		m_debugHistoryManager->LayoutHelper(instance->GetDebugEntityName().c_str(), NULL, true, -20.0f, 20.0f, 0.0f, 1.0f, 1.0f+float(x)*1.0f, float(y)*1.0f);
		m_debugHistoryManager->GetHistory(instance->GetDebugEntityName().c_str())->SetVisibility(false);
	}
	else
	{
		instance->graphIndex = UINT_MAX;
	}
}

void CAIAggressionModule::DetachDebugGraph(CAIAggressionInstance* instance)
{
	if (instance->graphIndex != UINT_MAX)
	{
		m_debugHistoryManager->RemoveHistory(instance->GetDebugEntityName().c_str());
		m_AggressionGraphOccupied[instance->graphIndex] = false;
	}
}

void CAIAggressionModule::MoveFromOneMapToAnother(EntityId entityID, std::map<EntityId, CAIAggressionInstance*>& oldMap, std::map<EntityId, CAIAggressionInstance*>& newMap)
{
	std::map<EntityId, CAIAggressionInstance*>::iterator it = oldMap.find(entityID);
	if (it != oldMap.end())
	{
		newMap.insert(*it);
		oldMap.erase(it);
	}
}
