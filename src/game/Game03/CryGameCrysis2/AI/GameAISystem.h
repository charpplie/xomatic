#pragma once

#ifndef GameAISystem_h
#define GameAISystem_h

#include "AdvantagePointOccupancyControl.h"
#include "DeathManager.h"

struct SConsistencyFailEntry
{
	EntityId entityID;
	string name;
};

enum ConsistencyStatus
{
	ConsistencyStatus_OK,
	ConsistencyStatus_MissingEntities
};

struct IGameAIModule
{
	virtual ~IGameAIModule() {}
	virtual void Enter(EntityId entityID) = 0;
	virtual void Leave(EntityId entityID) = 0;
	virtual void Pause(EntityId entityID) = 0;
	virtual void Resume(EntityId entityID) = 0;
	virtual void Reset() {}
	virtual void Update(float dt) {}
	virtual const char* GetName() const = 0;
	virtual ConsistencyStatus CheckConsistency(std::vector<SConsistencyFailEntry>* missingEntities)
	{
		return ConsistencyStatus_OK;
	}
};

class CGameAISystem
{
public:
	IGameAIModule* FindModule(const char* moduleName) const;
	void EnterModule(EntityId entityID, const char* moduleName);
	void LeaveModule(EntityId entityID, const char* moduleName);
	void LeaveAllModules(EntityId entityID);
	void PauseAllModules(EntityId entityID);
	void ResumeAllModules(EntityId entityID);
	void Update(float frameTime);
	void ResetAllModules();
	void Reset();

	CAdvantagePointOccupancyControl* GetAdvantagePointOccupancyControl() { return &m_advantagePointOccupancyControl; }

	static CGameAISystem* GetInstance();

private:
	CGameAISystem();
	void UpdateModules(float frameTime);

	std::vector<IGameAIModule*> m_modules;
	bool m_checkConsistency;

	CAdvantagePointOccupancyControl m_advantagePointOccupancyControl;
	DeathManager m_deathManager;
};

#endif // GameAISystem_h
