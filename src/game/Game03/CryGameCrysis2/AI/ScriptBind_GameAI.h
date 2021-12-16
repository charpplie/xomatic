#pragma once

#ifndef ScriptBind_GameAI_h
#define ScriptBind_GameAI_h

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IGameFramework;
struct ISystem;

class CScriptBind_GameAI : public CScriptableBase
{
public:
	CScriptBind_GameAI(ISystem* system, IGameFramework* gameFramework);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

private:
	void RegisterMethods();
	void RegisterGlobals();

	int RegisterWithModule(IFunctionHandler* functionHandler, const char* moduleName, ScriptHandle entityID);
	int UnregisterWithModule(IFunctionHandler* functionHandler, const char* moduleName, ScriptHandle entityID);
	int UnregisterWithAllModules(IFunctionHandler* functionHandler, ScriptHandle entityID);
	int PauseAllModules(IFunctionHandler* functionHandler, ScriptHandle entityID);
	int ResumeAllModules(IFunctionHandler* functionHandler, ScriptHandle entityID);
	int IncreasePressure(IFunctionHandler* functionHandler, ScriptHandle entityID, float amount);
	int GetPressureLevel(IFunctionHandler* functionHandler, ScriptHandle entityID);
	int IncreaseAggression(IFunctionHandler* funcHandler, ScriptHandle entityID, float amount);
	int GetAggressionLevel(IFunctionHandler* funcHandler, ScriptHandle entityID);
	bool GetEntitiesInRange(const Vec3& center, float radius, const char* pClassName, SEntityProximityQuery* pQuery) const;
	int GetClosestEntityToTarget(IFunctionHandler* funcHandler, Vec3 attackerPos, Vec3 targetPos, const char* pClassName, float radius, float maxAngle);

	int ResetAdvantagePointOccupancyControl(IFunctionHandler* functionHandler);
	int OccupyAdvantagePoint(IFunctionHandler* functionHandler, ScriptHandle entityID, Vec3 point);
	int ReleaseAdvantagePointFor(IFunctionHandler* functionHandler, ScriptHandle entityID);
	int IsAdvantagePointOccupied(IFunctionHandler* functionHandler, Vec3 point);

	int StartSearchModuleFor(IFunctionHandler* pH, int groupID, Vec3 targetPos);
	int StopSearchModuleFor(IFunctionHandler* pH, int groupID);
	int GetNextSearchSpot(IFunctionHandler* pH, ScriptHandle entityID, float closenessToAgentWeight, float closenessToTargetWeight);
	int MarkAssignedSearchSpotAsUnreachable(IFunctionHandler* pH, ScriptHandle entityID);

	ISystem* m_system;
	IGameFramework* m_gameFramework;
	IScriptSystem* m_scriptSystem;
};

#endif // ScriptBind_GameAI_h
