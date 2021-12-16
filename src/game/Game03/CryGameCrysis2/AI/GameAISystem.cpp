/*

GameAISystem Tasks:
- Pool instances
- Create helpers
- Add ability to pass in parameterization data from lua
- Remove GameAISystem singleton

*/

#include "StdAfx.h"
#include "GameAISystem.h"
#include "AIPressure.h"
#include "AIAggression.h"
#include "AIBattleFront.h"
#include "SearchModule.h"
#include <IAISystem.h>


CGameAISystem::CGameAISystem()
: m_checkConsistency(false)
{
	m_modules.push_back(CAIPressureModule::GetInstance());
	m_modules.push_back(CAIAggressionModule::GetInstance());
	m_modules.push_back(CAIBattleFrontModule::GetInstance());
	m_modules.push_back(SearchModule::GetInstance());
}

IGameAIModule* CGameAISystem::FindModule(const char* moduleName) const
{
	for (unsigned int i = 0; i < m_modules.size(); ++i)
	{
		if (strcmp(m_modules[i]->GetName(), moduleName) == 0)
		{
			return m_modules[i];
		}
	}

	return NULL;
}

void CGameAISystem::EnterModule(EntityId entityID, const char* moduleName)
{
	IGameAIModule* module = FindModule(moduleName);
	if (!module)
	{
		gEnv->pLog->LogError("GameAISystem: Could not register entity [%d] with module '%s' - the module doesn't exist.", entityID, moduleName);
		return;
	}

	module->Enter(entityID);
}

void CGameAISystem::LeaveModule(EntityId entityID, const char* moduleName)
{
	IGameAIModule* module = FindModule(moduleName);
	if (!module)
	{
		gEnv->pLog->LogError("GameAISystem: Could not unregister entity [%d] with module '%s' - the module doesn't exist.", entityID, moduleName);
		return;
	}

	module->Leave(entityID);
}

void CGameAISystem::LeaveAllModules(EntityId entityID)
{
	for (unsigned int i = 0; i < m_modules.size(); ++i)
	{
		IGameAIModule* module = m_modules[i];
		module->Leave(entityID);
	}
}

void CGameAISystem::PauseAllModules(EntityId entityID)
{
	for (unsigned int i = 0; i < m_modules.size(); ++i)
	{
		IGameAIModule* module = m_modules[i];
		module->Pause(entityID);
	}
}

void CGameAISystem::ResumeAllModules(EntityId entityID)
{
	for (unsigned int i = 0; i < m_modules.size(); ++i)
	{
		IGameAIModule* module = m_modules[i];
		module->Resume(entityID);
	}
}

void CGameAISystem::Update(float frameTime)
{
	UpdateModules(frameTime);
	m_deathManager.ProcessPendingDeathReactions();
}

CGameAISystem* CGameAISystem::GetInstance()
{
	static CGameAISystem instance;
	return &instance;
}

void CGameAISystem::ResetAllModules()
{
	for (unsigned int i = 0; i < m_modules.size(); ++i)
	{
		IGameAIModule* module = m_modules[i];
		module->Reset();
	}
}

void CGameAISystem::Reset()
{
	ResetAllModules();
	m_advantagePointOccupancyControl.ResetAdvantagePointOccupancyControl();
}

void CGameAISystem::UpdateModules(float frameTime)
{
	for (unsigned int i = 0; i < m_modules.size(); ++i)
	{
		IGameAIModule* module = m_modules[i];

		module->Update(frameTime);

		if (m_checkConsistency)
		{
			std::vector<SConsistencyFailEntry> missingEntities;
			if (module->CheckConsistency(&missingEntities) == ConsistencyStatus_MissingEntities)
			{
				for (unsigned int i = 0; i < missingEntities.size(); ++i)
				{
					gEnv->pLog->LogError("Entity id=%d name=%s have been removed without unregistering with module. Removing from list...",
						missingEntities[i].entityID, missingEntities[i].name.c_str()
						);

					module->Leave(missingEntities[i].entityID);
				}
			}
		}
	}
}
