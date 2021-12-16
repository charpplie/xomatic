#include "StdAfx.h"
#include "GameAIInstanceBase.h"
#include "IAgent.h"
#include "IAIObject.h"
#include "IAIActor.h"

CGameAIInstanceBase::CGameAIInstanceBase(const EntityId entityID)
: m_entityID(entityID)
, m_debugEntityName(gEnv->pEntitySystem->GetEntity(entityID)->GetName())
{

}

void CGameAIInstanceBase::SendSignal(const char* signal)
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(GetEntityID());
	assert(entity);
	if (entity)
	{
		IAIObject* aiObject = entity->GetAI();
		assert(aiObject);
		if (aiObject)
			aiObject->CastToIAIActor()->SetSignal(1, signal, NULL, NULL, 0);
	}
}
