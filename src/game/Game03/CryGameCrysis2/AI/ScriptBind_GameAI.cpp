#include "StdAfx.h"
#include "ScriptBind_GameAI.h"
#include "GameAISystem.h"
#include "AIPressure.h"
#include "AIAggression.h"
#include "SearchModule.h"


CScriptBind_GameAI::CScriptBind_GameAI(ISystem* system, IGameFramework* gameFramework)
: m_system(system)
, m_gameFramework(gameFramework)
, m_scriptSystem(system->GetIScriptSystem())
{
	Init(m_scriptSystem, m_system);
	SetGlobalName("GameAI");

	RegisterMethods();
	RegisterGlobals();
}



void CScriptBind_GameAI::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_GameAI::

	SCRIPT_REG_TEMPLFUNC(RegisterWithModule, "moduleName, entityId");
	SCRIPT_REG_TEMPLFUNC(UnregisterWithModule, "moduleName, entityId");
	SCRIPT_REG_TEMPLFUNC(UnregisterWithAllModules, "entityId");
	SCRIPT_REG_TEMPLFUNC(PauseAllModules, "entityId");
	SCRIPT_REG_TEMPLFUNC(ResumeAllModules, "entityId");
	SCRIPT_REG_TEMPLFUNC(IncreasePressure, "entityId, amount");
	SCRIPT_REG_TEMPLFUNC(GetPressureLevel, "entityId");
	SCRIPT_REG_TEMPLFUNC(IncreaseAggression, "entityId, amount");
	SCRIPT_REG_TEMPLFUNC(GetAggressionLevel, "entityId");
	SCRIPT_REG_TEMPLFUNC(GetClosestEntityToTarget, "attackerPos, targetPos, radius, maxAngle");
 	SCRIPT_REG_FUNC(ResetAdvantagePointOccupancyControl);
 	SCRIPT_REG_TEMPLFUNC(OccupyAdvantagePoint, "entityId, point");
 	SCRIPT_REG_TEMPLFUNC(ReleaseAdvantagePointFor, "entityId");
	SCRIPT_REG_TEMPLFUNC(IsAdvantagePointOccupied, "point");
	SCRIPT_REG_TEMPLFUNC(StartSearchModuleFor, "groupID, point");
	SCRIPT_REG_TEMPLFUNC(StopSearchModuleFor, "groupID");
	SCRIPT_REG_TEMPLFUNC(GetNextSearchSpot, "entityId, closenessToAgentWeight, closenessToTargetWeight");
	SCRIPT_REG_TEMPLFUNC(MarkAssignedSearchSpotAsUnreachable, "entityId");

#undef SCRIPT_REG_CLASSNAME
}



void CScriptBind_GameAI::RegisterGlobals()
{
}



#define GET_ENTITY(i) \
	ScriptHandle hdl;\
	funcHandler->GetParam(i,hdl);\
	int nID = hdl.n;\
	IEntity* entity = gEnv->pEntitySystem->GetEntity(nID);



int CScriptBind_GameAI::RegisterWithModule(IFunctionHandler* functionHandler, const char* moduleName, ScriptHandle entityID)
{
	CGameAISystem::GetInstance()->EnterModule(static_cast<EntityId>(entityID.n), moduleName);
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::UnregisterWithModule(IFunctionHandler* functionHandler, const char* moduleName, ScriptHandle entityID)
{
	CGameAISystem::GetInstance()->LeaveModule(static_cast<EntityId>(entityID.n), moduleName);
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::UnregisterWithAllModules(IFunctionHandler* functionHandler, ScriptHandle entityID)
{
	CGameAISystem::GetInstance()->LeaveAllModules(static_cast<EntityId>(entityID.n));
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::PauseAllModules(IFunctionHandler* functionHandler, ScriptHandle entityID)
{
	CGameAISystem::GetInstance()->PauseAllModules(static_cast<EntityId>(entityID.n));
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::ResumeAllModules(IFunctionHandler* functionHandler, ScriptHandle entityID)
{
	CGameAISystem::GetInstance()->ResumeAllModules(static_cast<EntityId>(entityID.n));
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::IncreasePressure(IFunctionHandler* functionHandler, ScriptHandle entityID, float amount)
{
	CAIPressureModule* pressureModule = static_cast<CAIPressureModule*>(CGameAISystem::GetInstance()->FindModule("Pressure"));
	assert(pressureModule);
	if (!pressureModule)
		return functionHandler->EndFunction();

	CAIPressureInstance* pressureInstance = pressureModule->GetRunningInstance(static_cast<EntityId>(entityID.n));
	assert(pressureInstance);
	if (!pressureInstance)
		return functionHandler->EndFunction();

	pressureInstance->IncreasePressure(amount);

	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::GetPressureLevel(IFunctionHandler* functionHandler, ScriptHandle entityID)
{
	CAIPressureModule* pressureModule = static_cast<CAIPressureModule*>(CGameAISystem::GetInstance()->FindModule("Pressure"));
	assert(pressureModule);
	if (!pressureModule)
		return functionHandler->EndFunction();

	CAIPressureInstance* pressureInstance = pressureModule->GetRunningInstance(static_cast<EntityId>(entityID.n));
	assert(pressureInstance);
	if (!pressureInstance)
		return functionHandler->EndFunction();

	return functionHandler->EndFunction(pressureInstance->current);
}

int CScriptBind_GameAI::IncreaseAggression(IFunctionHandler* funcHandler, ScriptHandle entityID, float amount)
{
	CAIAggressionInstance* aggrInst = CAIAggressionModule::GetInstance()->GetRunningInstance(static_cast<EntityId>(entityID.n));
	assert(aggrInst);
	if (aggrInst)
	{
		aggrInst->IncreaseAggression(amount);
	}

	return funcHandler->EndFunction();
}

int CScriptBind_GameAI::GetAggressionLevel(IFunctionHandler* funcHandler, ScriptHandle entityID)
{
	CAIAggressionInstance* aggrInst = CAIAggressionModule::GetInstance()->GetRunningInstance(static_cast<EntityId>(entityID.n));
	assert(aggrInst);
	if (aggrInst)
	{
		return funcHandler->EndFunction(aggrInst->GetAggressionLevel());
	}

	return funcHandler->EndFunction();
}


bool CScriptBind_GameAI::GetEntitiesInRange(const Vec3& center, float radius, const char* pClassName, SEntityProximityQuery* pQuery) const
{
	CRY_ASSERT(pQuery);

	IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
	IEntityClass* pClass = pEntitySystem->GetClassRegistry()->FindClass(pClassName);
	if (!pClass)
		return false;

	pQuery->box.min = center - Vec3(radius, radius, radius);
	pQuery->box.max = center + Vec3(radius, radius, radius);
	pQuery->pEntityClass = pClass;
	pEntitySystem->QueryProximity(*pQuery);

	return true;
}

int CScriptBind_GameAI::GetClosestEntityToTarget(IFunctionHandler* funcHandler, Vec3 attackerPos, Vec3 targetPos, const char* pClassName, float radius, float maxAngle)
{
	SEntityProximityQuery query;

	if (GetEntitiesInRange(targetPos, radius, pClassName, &query))
	{
		int closestIndex = -1;
		float closestDistSq = FLT_MAX;
		float cosMaxAngle = cosf(maxAngle);
		Vec3 attackerToTargetDir = (targetPos - attackerPos).GetNormalized();

		for (int i = 0; i < query.nCount; ++i)
		{
			IEntity* pEntity = query.pEntities[i];
			if (pEntity)
			{
				float targetToEntityDistSq = (pEntity->GetWorldPos() - targetPos).GetLengthSquared();
				if (targetToEntityDistSq < closestDistSq)
				{
					// Closer than the current, but is the angle valid?
					Vec3 attackerToEntityDir = (pEntity->GetWorldPos() - attackerPos).GetNormalized();
					if (attackerToTargetDir.Dot(attackerToEntityDir) > cosMaxAngle)
					{
						closestIndex = i;
						closestDistSq = targetToEntityDistSq;
					}
				}
			}
		}

		if (closestIndex != -1)
		{
			return funcHandler->EndFunction(query.pEntities[closestIndex]->GetScriptTable());
		}
	}

	return funcHandler->EndFunction();
}

int CScriptBind_GameAI::ResetAdvantagePointOccupancyControl(IFunctionHandler* functionHandler)
{
	CGameAISystem::GetInstance()->GetAdvantagePointOccupancyControl()->ResetAdvantagePointOccupancyControl();
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::OccupyAdvantagePoint(IFunctionHandler* functionHandler, ScriptHandle entityID, Vec3 point)
{
	CGameAISystem::GetInstance()->GetAdvantagePointOccupancyControl()->OccupyAdvantagePoint(static_cast<EntityId>(entityID.n), point);
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::ReleaseAdvantagePointFor(IFunctionHandler* functionHandler, ScriptHandle entityID)
{
	CGameAISystem::GetInstance()->GetAdvantagePointOccupancyControl()->ReleaseAdvantagePoint(static_cast<EntityId>(entityID.n));
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::IsAdvantagePointOccupied(IFunctionHandler* functionHandler, Vec3 point)
{
	CGameAISystem::GetInstance()->GetAdvantagePointOccupancyControl()->IsAdvantagePointOccupied(point);
	return functionHandler->EndFunction();
}

int CScriptBind_GameAI::StartSearchModuleFor(IFunctionHandler* pH, int groupID, Vec3 targetPos)
{
	SearchModule::GetInstance()->GroupEnter(groupID, targetPos);
	return pH->EndFunction();
}

int CScriptBind_GameAI::StopSearchModuleFor(IFunctionHandler* pH, int groupID)
{
	SearchModule::GetInstance()->GroupLeave(groupID);
	return pH->EndFunction();
}

int CScriptBind_GameAI::GetNextSearchSpot(IFunctionHandler* pH, ScriptHandle entityID, float closenessToAgentWeight, float closenessToTargetWeight)
{
	SearchSpotQuery query;
	query.closenessToAgentWeight = closenessToAgentWeight;
	query.closenessToTargetWeight = closenessToTargetWeight;

	if (SearchModule::GetInstance()->GetNextSearchPoint(static_cast<EntityId>(entityID.n), &query))
	{
		return pH->EndFunction(Script::SetCachedVector(query.result, pH, 1));
	}

	return pH->EndFunction();
}

int CScriptBind_GameAI::MarkAssignedSearchSpotAsUnreachable(IFunctionHandler* pH, ScriptHandle entityID)
{
	SearchModule::GetInstance()->MarkAssignedSearchSpotAsUnreachable(static_cast<EntityId>(entityID.n));
	return pH->EndFunction();
}
