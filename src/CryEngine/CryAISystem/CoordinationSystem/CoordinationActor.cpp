#include "StdAfx.h"
#include "CoordinationActor.h"
#include "Communication/CommunicationManager.h"


EntityId CoordinationActor::GetEntityID() const
{
	if (CAIObject* object = m_object.GetAIObject())
		return object->GetEntityID();

	return 0;
}

CommunicationConfigID CoordinationActor::GetCommunicationConfigID() const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return CommunicationConfigID(0);

	if (IAIActorProxy* proxy = object->GetProxy())
		return gAIEnv.pCommunicationManager->GetConfigID(proxy->GetCommunicationConfigName());
	
	return CommunicationConfigID(0);
};

const char* CoordinationActor::GetName() const
{
	if (CAIObject* object = m_object.GetAIObject())
		return object->GetName();

	return "<null>";
}

IScriptTable* CoordinationActor::GetScript() const
{
	if (CAIObject* object = m_object.GetAIObject())
		return object->GetEntity()->GetScriptTable();

	return 0;
}

void CoordinationActor::Signal(const char* signal) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->SetSignal(AISIGNAL_ALLOW_DUPLICATES, signal);
}

void CoordinationActor::Action(const char* name) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->SetSignal(AISIGNAL_ALLOW_DUPLICATES, name);
}

void CoordinationActor::RegisterBehaviorListener(IActorBehaviorListener* listener) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->RegisterBehaviorListener(listener);
}

void CoordinationActor::UnregisterBehaviorListener(IActorBehaviorListener* listener) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->UnregisterBehaviorListener(listener);
}

void CoordinationActor::NotifyPerforming(const char* roleName) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->SetSignal(AISIGNAL_ALLOW_DUPLICATES, "PerformingRole");
}

void CoordinationActor::NotifyFinished(const char* roleName) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->SetSignal(AISIGNAL_ALLOW_DUPLICATES, "RoleFinished");
}

void CoordinationActor::NotifyFailed(const char* roleName) const
{
	CAIObject* object = m_object.GetAIObject();
	if (!object)
		return;

	if (IAIActor* pAIActor = object->CastToIAIActor())
		pAIActor->SetSignal(AISIGNAL_ALLOW_DUPLICATES, "RoleFailed");
}