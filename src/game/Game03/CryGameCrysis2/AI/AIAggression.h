#pragma once

#ifndef AIAggression_h
#define AIAggression_h

#include "GameAISystem.h"
#include "GameAIInstanceBase.h"

class CAIAggressionInstance : public CGameAIInstanceBase
{
public:
	CAIAggressionInstance(EntityId _entityID);
	void ReadParams();
	void Update(float frameTime);
	void DebugDraw();
	void IncreaseAggression(float amount);
	float GetAggressionLevel() const { return m_current; }
	float m_current;
	float m_increaseRate;
	unsigned int graphIndex;
};

class CAIAggressionModule : public IGameAIModule
{
public:
	CAIAggressionModule();
	virtual ~CAIAggressionModule();
	virtual void Enter(EntityId entityID);
	virtual void Leave(EntityId entityID);
	virtual void Pause(EntityId entityID);

	virtual void Resume(EntityId entityID);
	virtual void Update(float frameTime);
	virtual const char* GetName() const;
	virtual ConsistencyStatus CheckConsistency(std::vector<SConsistencyFailEntry>* missingEntities);
	CAIAggressionInstance* GetRunningInstance(EntityId entityID);

	static CAIAggressionModule* GetInstance();

private:
	bool FindFreeGraphSlot(unsigned int* graphSlotIndex);
	CAIAggressionInstance* CreateAggressionInstance(EntityId entityID);
	void DestroyAggressionInstance(CAIAggressionInstance* instance);
	void LeaveFromMap(std::map<EntityId, CAIAggressionInstance*>& instanceMap, EntityId entityID);
	void AttachDebugGraph( CAIAggressionInstance* instance );
	void DetachDebugGraph(CAIAggressionInstance* instance);
	void MoveFromOneMapToAnother(EntityId entityID, std::map<EntityId, CAIAggressionInstance*>& oldMap, std::map<EntityId, CAIAggressionInstance*>& newMap);
	std::map<EntityId, CAIAggressionInstance*> m_runningInstances;
	std::map<EntityId, CAIAggressionInstance*> m_pausedInstances;
	IDebugHistoryManager* m_debugHistoryManager;
	bool m_AggressionGraphOccupied[16];

	ICVar* m_statsTargetVar;
};

#endif // AIAggression_h
