#include "StdAfx.h"
#include "TacticalPointLanguageExtender.h"
#include "IAgent.h"
#include "AI/GameAISystem.h"
#include "AIBattleFront.h"
#include "IAIObjectManager.h"

namespace FlankCalculator
{
	bool IsPointOnMyFlank(EntityId actorEntityID, const Vec3& actorPos, const Vec3& targetPos, const Vec3& pointPos)
	{
		CAIBattleFrontGroup* group = CAIBattleFrontModule::GetInstance()->GetGroupForEntity(actorEntityID);
		ASSERT_IS_NOT_NULL(group);
		if (!group || group->GetMemberCount() == 1)
			return true; // I'm alone so this is my flank

		const Vec3& groupAveragePos = group->GetAveragePosition();

		// Set up a coordinate system where Y corresponds to the direction
		// from the battle group's center position towards the target
		Vec3 Y = (targetPos - groupAveragePos);
		Vec3 X = Vec3(0,0,-1).Cross(Y);

		// Calculate flank side sign
		Vec3 groupAveragePosToPuppet = actorPos - groupAveragePos;
		float flankSideSign = (groupAveragePosToPuppet.Dot(X) > 0.0f) ? 1.0f : -1.0f; // Flank side sign (-1 for left, +1 for right)

		// Calculate how far the point is on the X-axis
		Vec3 groupAveragePosToPoint = pointPos - groupAveragePos;
		float pointDistOnXAxis = groupAveragePosToPoint.Dot(X);

		bool pointIsOnMyFlank = pointDistOnXAxis * flankSideSign > 0.0f;
		return pointIsOnMyFlank;
	}
}

CTacticalPointLanguageExtender::CTacticalPointLanguageExtender()
: m_battlefrontAIObject(0)
{
}

void CTacticalPointLanguageExtender::Initialize()
{
	CryLogAlways("Registering TPS Extensions...");

	RegisterWithTacticalPointSystem();
	RegisterQueries();
	StoreBattleFrontModulePointer();

	Script::Call(gEnv->pScriptSystem, "ReloadTPSExtensions");
}

void CTacticalPointLanguageExtender::Deinitialize()
{
	UnregisterQueries();
	UnregisterFromTacticalPointSystem();
	ReleaseBattleFrontObject();
}

bool CTacticalPointLanguageExtender::GeneratePoints(TGenerateParameters& parameters, SGenerateDetails& details, TObjectType object, const Vec3& objectPos, TObjectType auxObject, const Vec3& auxObjectPos) const
{
	bool generationHandled = false;

	if (stricmp(parameters.query, "advantagePoints") == 0)
	{
		generationHandled = true;

		CRY_ASSERT_MESSAGE(gEnv->pAISystem != NULL, "Expected AI System to exist, but it didn't.");
		IAIObjectManager* pAIObjMgr = gEnv->pAISystem->GetAIObjectManager();

		IAIObjectIter* advPointObjIter = pAIObjMgr->GetFirstAIObjectInRange(IAISystem::OBJFILTER_TYPE, AIANCHOR_ADVANTAGE_POINT, objectPos, details.fSearchDist, true);
		if (advPointObjIter != NULL)
		{
			while (IAIObject* advPointObj = advPointObjIter->GetObject())
			{
				const Vec3& advPointPos = advPointObj->GetPos();
				const Vec3& advPointDir = advPointObj->GetBodyDir();

				if(!CGameAISystem::GetInstance()->GetAdvantagePointOccupancyControl()->IsAdvantagePointOccupied(advPointPos))
				{
					Vec3 advPointToObjDir = (objectPos - advPointPos).GetNormalizedSafe();
					float dotProduct = advPointToObjDir.Dot(advPointDir);

					bool objIsVisibleFromAdvPoint = (dotProduct > 0.5f);
					if (objIsVisibleFromAdvPoint)
					{
						parameters.result->AddPoint(advPointPos);
					}
				}

				advPointObjIter->Next();
			}

			advPointObjIter->Release();
		}
	}

	return generationHandled;
}

bool CTacticalPointLanguageExtender::GetObject(TObjectParameters& parameters) const
{
	// Find the battlefront for this user
	CAIBattleFrontGroup* battleFrontGroup = m_battleFrontModule->GetGroupForEntity(parameters.pOwner.actorEntityId);
	
	CRY_ASSERT(battleFrontGroup);
	
	if (!battleFrontGroup)
	{
		gEnv->pLog->LogError("CTacticalPointLanguageExtender::GetObject: Couldn't get battlefront group for entity %d", parameters.pOwner.actorEntityId);
		return false;
	}

	IAIObject * pBattleFrontAIObject = GetBattleFrontObject();
	assert(pBattleFrontAIObject);
	if (!pBattleFrontAIObject)
			return false;

	pBattleFrontAIObject->SetPos(battleFrontGroup->GetBattleFrontPosition());

	// Return the result
	// (MATT) The interface is poor for this - it shouldn't need a pointer {2009/12/01}
	parameters.result = pBattleFrontAIObject;
	return true;
}

bool CTacticalPointLanguageExtender::BoolTest(TBoolParameters& params, TObjectType pObject, const Vec3& objPos, TPointType point) const
{
	if (stricmp(params.query, "myflank") == 0)
	{
		params.result = FlankCalculator::IsPointOnMyFlank(params.pOwner.actorEntityId, params.pOwner.actorPos, objPos, point.GetPos());
		return true;
	}

	return false;
}

void CTacticalPointLanguageExtender::RegisterWithTacticalPointSystem()
{
	CRY_ASSERT_MESSAGE(gEnv->pAISystem->GetTacticalPointSystem() != NULL, "Expecting tactical point system to exist, but it doesn't.");
	ITacticalPointSystem& tacticalPointSystem = *gEnv->pAISystem->GetTacticalPointSystem();

	bool successfullyAddedLanguageExtender = tacticalPointSystem.AddLanguageExtender(this);
	CRY_ASSERT_MESSAGE(successfullyAddedLanguageExtender, "Failed to add tactical point language extender.");
}

void CTacticalPointLanguageExtender::RegisterQueries()
{
	ITacticalPointSystem& tacticalPointSystem = *gEnv->pAISystem->GetTacticalPointSystem();

	tacticalPointSystem.ExtendQueryLanguage("advantagePoints", eTPQT_GENERATOR_O, eTPQC_IGNORE);
	tacticalPointSystem.ExtendQueryLanguage("battlefront", eTPQT_PRIMARY_OBJECT, eTPQC_IGNORE);
	tacticalPointSystem.ExtendQueryLanguage("myflank",  eTPQT_TEST, eTPQC_CHEAP);
}

void CTacticalPointLanguageExtender::UnregisterFromTacticalPointSystem()
{
	CRY_ASSERT_MESSAGE(gEnv->pAISystem->GetTacticalPointSystem() != NULL, "Expecting tactical point system to exist, but it doesn't.");
	ITacticalPointSystem& tacticalPointSystem = *gEnv->pAISystem->GetTacticalPointSystem();

	bool successfullyRemovedLanguageExtender = tacticalPointSystem.RemoveLanguageExtender(this);
	CRY_ASSERT_MESSAGE(successfullyRemovedLanguageExtender, "Failed to remove tactical point language extender.");
}

void CTacticalPointLanguageExtender::UnregisterQueries()
{
	// At the moment there is no functionality in the Tactical Point System
	// to remove extensions in a proper way :(

	// Pseudo code:
	//   ITacticalPointSystem& tacticalPointSystem = *gEnv->pAISystem->GetTacticalPointSystem();
	//   tacticalPointSystem.DeExtendQueryLanguage("advantagePoints");
}

void CTacticalPointLanguageExtender::StoreBattleFrontModulePointer()
{
	m_battleFrontModule = static_cast<CAIBattleFrontModule*>(CGameAISystem::GetInstance()->FindModule("BattleFront"));
	CRY_ASSERT(m_battleFrontModule);
	if (!m_battleFrontModule)
	{
		gEnv->pLog->LogError("CTacticalPointLanguageExtender: Trying to access the battlefront module but it doesn't exist");
	}
}

IAIObject* CTacticalPointLanguageExtender::GetBattleFrontObject() const
{
	IAIObjectManager* pAIObjMgr = gEnv->pAISystem->GetAIObjectManager();
	IAIObject *pObj = pAIObjMgr->GetAIObject(m_battlefrontAIObject);

	//AIObjectParameters params; //Diesel new

	if (!pObj)
	{
		// Not created or has been removed (by quickload, reset, etc)
		// Create one

//Diesel cut
//		pObj = pAIObjMgr->CreateAIObject(AIObjectParams(AIOBJECT_DUMMY));

//Diesel new
	
	pObj = pAIObjMgr->CreateAIObject(AIOBJECT_DUMMY, 0, 0);
		
//Diesel new

		assert(pObj);
		pObj->SetName("Game_BattleFront");
		m_battlefrontAIObject = pObj->GetAIObjectID();
	}
	return pObj;
}

void CTacticalPointLanguageExtender::ReleaseBattleFrontObject()
{
	IAIObjectManager* pAIObjMgr = gEnv->pAISystem->GetAIObjectManager();

	if (pAIObjMgr->GetAIObject(m_battlefrontAIObject))
		pAIObjMgr->RemoveObject(m_battlefrontAIObject);
	m_battlefrontAIObject = 0;
}
