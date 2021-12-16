#include "StdAfx.h"
#include "AIPressure.h"

#include "GameCVars.h"
#include "IDebugHistory.h"



CAIPressureInstance::CAIPressureInstance(EntityId _entityID)
: CGameAIInstanceBase(_entityID)
, current(0.0f)
, decreaseRate(0.1f)
, threshold(0.8f)
, sustain(6.0f)
, activeTime(0.0f)
, active(false)
{

}

void CAIPressureInstance::ReadParams()
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(GetEntityID());
	assert(entity);
	if (!entity)
		return;

	SmartScriptTable pressureParams;
	IScriptTable* scriptTable = entity->GetScriptTable();
	if(!scriptTable || !scriptTable->GetValue("PressureParams", pressureParams))
		return;

	pressureParams->GetValue("decreaseRate", decreaseRate);
	pressureParams->GetValue("threshold", threshold);
	pressureParams->GetValue("sustain", sustain);
}

void CAIPressureInstance::Update(float dt)
{
	if (active)
	{
		activeTime += dt;
		if (activeTime > sustain)
		{
			OnNoSustainedPressure();
		}
	}
	else if (current > 0.0f)
	{
		current = max(0.0f, current - decreaseRate * dt);
		if (current == 0.0f)
		{
			OnNoPressure();
		}
	}
}

void CAIPressureInstance::DebugDraw()
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(GetEntityID());
	if(pEntity)
	{
		Vec3 pos = pEntity->GetPos();
		pos.z += 2.0f;

		float underPressureColor[4] = {1,1,1,1};	
		if(active)
		{
			underPressureColor[1] = 0.0f;
			underPressureColor[2] = 0.0f;
		}

		float ratio = max(1.0f -(current/threshold), 0);
		float pressureValueColor[4] = {1,ratio,ratio,1};

		gEnv->pRenderer->DrawLabelEx(pos, 1,underPressureColor, true, false, "Pressure");
		gEnv->pRenderer->DrawLabelEx(pos, 1,pressureValueColor, true, false, "        - %.1f", current);
	}
}

void CAIPressureInstance::IncreasePressure(float amount)
{
	current = min(1.0f, current + amount);

	if (current >= threshold)
	{
		OnPressure();
	}
}

void CAIPressureInstance::OnPressure()
{
	if (!active)
	{
		SendSignal("OnPressure");
	}

	active = true;
	activeTime = 0.0f;
}

void CAIPressureInstance::OnNoPressure()
{
	SendSignal("OnNoPressure");
}

void CAIPressureInstance::OnNoSustainedPressure()
{
	SendSignal("OnNoSustainedPressure");
	active = false;
}

CAIPressureModule::CAIPressureModule()
: m_statsTargetVar(0)
{
	m_debugHistoryManager = g_pGame->GetIGameFramework()->CreateDebugHistoryManager();
	memset(m_pressureGraphOccupied, 0, sizeof(m_pressureGraphOccupied));
}

CAIPressureModule::~CAIPressureModule()
{
	SAFE_RELEASE(m_debugHistoryManager);
}

void CAIPressureModule::Enter(EntityId entityID)
{
	Leave(entityID);
	CAIPressureInstance* instance = CreatePressureInstance(entityID);
//Diesel cut	
	//m_runningInstances.insert( std::make_pair<EntityId, CAIPressureInstance*>(entityID, instance) );
//Diesel new
	m_runningInstances.insert(std::make_pair(entityID, instance));


}

void CAIPressureModule::Leave(EntityId entityID)
{
	LeaveFromMap(m_runningInstances, entityID);
	LeaveFromMap(m_pausedInstances, entityID);
}

void CAIPressureModule::Pause(EntityId entityID)
{
	CAIPressureInstance* instance = GetRunningInstance(entityID);
	if (instance)
	{
		DetachDebugGraph(instance);
		MoveFromOneMapToAnother(entityID, m_runningInstances, m_pausedInstances);
	}
}

void CAIPressureModule::Resume(EntityId entityID)
{
	MoveFromOneMapToAnother(entityID, m_pausedInstances, m_runningInstances);
	CAIPressureInstance* instance = GetRunningInstance(entityID);
	if(instance)
		AttachDebugGraph(instance);
}

void CAIPressureModule::Update(float dt)
{
	if (!m_statsTargetVar)
		m_statsTargetVar = gEnv->pConsole->GetCVar("ai_StatsTarget");
	
	const char* statsTarget = m_statsTargetVar->GetString();
	bool viewAllGraphs = (strcmp(statsTarget, "none") == 0);
	std::map<EntityId, CAIPressureInstance*>::iterator it = m_runningInstances.begin();

	const bool debugPressure = (g_pGameCVars->ai_DebugPressureSystem != 0);

	while (it != m_runningInstances.end())
	{
		CAIPressureInstance* instance = it->second;
		instance->Update(dt);

		if (debugPressure)
			instance->DebugDraw();

		// TODO: Move the following section into DebugDraw!
		if (instance->graphIndex != UINT_MAX)
		{
			IDebugHistory* debugHistory = m_debugHistoryManager->GetHistory(instance->GetDebugEntityName().c_str());
			if (debugHistory)
			{
				debugHistory->AddValue(instance->current);

				if (debugPressure && (viewAllGraphs || strcmp(instance->GetDebugEntityName().c_str(), statsTarget) == 0))
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

const char* CAIPressureModule::GetName() const
{
	return "Pressure";
}

ConsistencyStatus CAIPressureModule::CheckConsistency(std::vector<SConsistencyFailEntry>* missingEntities)
{
	assert(missingEntities);
	missingEntities->clear();

	for (std::map<EntityId, CAIPressureInstance*>::iterator it = m_runningInstances.begin(); it != m_runningInstances.end(); ++it)
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

CAIPressureInstance* CAIPressureModule::GetRunningInstance(EntityId entityID)
{
	std::map<EntityId, CAIPressureInstance*>::iterator it = m_runningInstances.find(entityID);

	if (it == m_runningInstances.end())
		return NULL;

	return it->second;
}

CAIPressureModule* CAIPressureModule::GetInstance()
{
	static CAIPressureModule instance;
	return &instance;
}

bool CAIPressureModule::FindFreeGraphSlot(unsigned int* graphSlotIndex)
{
	assert(graphSlotIndex);

	for (unsigned int i = 0; i < 16; ++i)
	{
		if (!m_pressureGraphOccupied[i])
		{
			*graphSlotIndex = i;
			return true;
		}
	}

	return false;
}

CAIPressureInstance* CAIPressureModule::CreatePressureInstance(EntityId entityID)
{
	CAIPressureInstance* instance = new CAIPressureInstance(entityID); // TODO: Get from pool
	instance->ReadParams();
	AttachDebugGraph(instance);
	return instance;
}

void CAIPressureModule::DestroyPressureInstance(CAIPressureInstance* instance)
{
	DetachDebugGraph(instance);
	delete instance; // TODO: Remove from pool
}

void CAIPressureModule::LeaveFromMap(std::map<EntityId, CAIPressureInstance*>& instancesMap, EntityId entityID)
{
	std::map<EntityId, CAIPressureInstance*>::iterator it = instancesMap.find(entityID);
	if (it != instancesMap.end())
	{
		CAIPressureInstance* instance = it->second;
		DestroyPressureInstance(instance);
		instancesMap.erase(it);
	}
}

void CAIPressureModule::AttachDebugGraph(CAIPressureInstance* instance)
{
	unsigned int graphSlotIndex;
	if (FindFreeGraphSlot(&graphSlotIndex))
	{
		m_pressureGraphOccupied[graphSlotIndex] = true;
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

void CAIPressureModule::DetachDebugGraph(CAIPressureInstance* instance)
{
	if (instance->graphIndex != UINT_MAX)
	{
		m_debugHistoryManager->RemoveHistory(instance->GetDebugEntityName().c_str());
		m_pressureGraphOccupied[instance->graphIndex] = false;
	}
}

void CAIPressureModule::MoveFromOneMapToAnother(EntityId entityID, std::map<EntityId, CAIPressureInstance*>& oldMap, std::map<EntityId, CAIPressureInstance*>& newMap)
{
	std::map<EntityId, CAIPressureInstance*>::iterator it = oldMap.find(entityID);
	if (it != oldMap.end())
	{
		newMap.insert(*it);
		oldMap.erase(it);
	}
}
