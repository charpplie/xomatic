#pragma once

#ifndef AIPressure_h
#define AIPressure_h

#include "GameAISystem.h"
#include "GameAIInstanceBase.h"

class CAIPressureInstance : public CGameAIInstanceBase
{
public:
	CAIPressureInstance(EntityId _entityID);
	void ReadParams();
	void Update(float dt);
	void DebugDraw();
	void IncreasePressure(float amount);
	void OnPressure();
	void OnNoPressure();
	void OnNoSustainedPressure();
	float current;
	float decreaseRate;
	float threshold;
	float sustain;
	float activeTime;
	bool active;
	unsigned int graphIndex;
};

class CAIPressureModule : public IGameAIModule
{
public:
	CAIPressureModule();
	virtual ~CAIPressureModule();
	virtual void Enter(EntityId entityID);
	virtual void Leave(EntityId entityID);
	virtual void Pause(EntityId entityID);

	virtual void Resume(EntityId entityID);
	virtual void Update(float dt);
	virtual const char* GetName() const;
	virtual ConsistencyStatus CheckConsistency(std::vector<SConsistencyFailEntry>* missingEntities);
	CAIPressureInstance* GetRunningInstance(EntityId entityID);

	static CAIPressureModule* GetInstance();

private:
	bool FindFreeGraphSlot(unsigned int* graphSlotIndex);
	CAIPressureInstance* CreatePressureInstance(EntityId entityID);
	void DestroyPressureInstance(CAIPressureInstance* instance);
	void LeaveFromMap(std::map<EntityId, CAIPressureInstance*>& instanceMap, EntityId entityID);
	void AttachDebugGraph( CAIPressureInstance* instance );
	void DetachDebugGraph(CAIPressureInstance* instance);
	void MoveFromOneMapToAnother(EntityId entityID, std::map<EntityId, CAIPressureInstance*>& oldMap, std::map<EntityId, CAIPressureInstance*>& newMap);
	std::map<EntityId, CAIPressureInstance*> m_runningInstances;
	std::map<EntityId, CAIPressureInstance*> m_pausedInstances;
	IDebugHistoryManager* m_debugHistoryManager;
	bool m_pressureGraphOccupied[16];
	ICVar* m_statsTargetVar;
};

#endif // AIPressure_h
