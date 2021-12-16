#include "pch.h"
#include "ActorEntityObject.h"

using namespace CryGame;

IMPLEMENT_DYNCREATE(CActorEntityObject, CEntityObject)

bool CActorEntityObject::Init(IEditor* ie, CBaseObject* prev, const CString& file)
{
	bool bRes = false;
	if (file.IsEmpty())
	{
		bRes = CEntityObject::Init(ie, prev, "");
		SetClass("ActorEntity");
	}
	else
	{
		bRes = CEntityObject::Init(ie, prev, "ActorEntity");
		SetClass("ActorEntity");

		IVariable* pDescVar = m_pProperties->FindVariable("DescName");
		CRY_ASSERT_MESSAGE(pDescVar, "Can't find ActorEntity desc var, not gonna work");

		if (pDescVar != NULL)
		{
			string descName = PathUtil::GetFile(file);
			PathUtil::RemoveExtension(descName);

			pDescVar->Set(descName);

			// Degenericify the entity name
			static int index = 0;
			descName = string().Format("%s%d", descName, index++);
			SetName(descName.c_str());
		}
	}

	return bRes;
}

void CActorEntityObject::OnEvent(ObjectEvent event)
{
	__super::OnEvent(event);

	switch (event)
	{
	case EVENT_INGAME:
	case EVENT_OUTOFGAME:
		if (m_pEntity)
		{
			// Make entity sleep on in/out game events.
			IPhysicalEntity* pe = m_pEntity->GetPhysics();
			if (pe)
			{
				pe_action_awake aa;
				aa.bAwake = 0;
				pe->Action(&aa);
			}
		}
	}
}
