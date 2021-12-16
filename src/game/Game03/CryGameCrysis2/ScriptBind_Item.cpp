/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 27:10:2004   11:29 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_Item.h"
#include "Item.h"
#include "IGameObject.h"
#include "Player.h"

#include "ItemSharedParams.h"


#define REUSE_VECTOR(table, name, value)	\
	{ if (table->GetValueType(name) != svtObject) \
	{ \
	table->SetValue(name, (value)); \
	} \
		else \
	{ \
	SmartScriptTable v; \
	table->GetValue(name, v); \
	v->SetValue("x", (value).x); \
	v->SetValue("y", (value).y); \
	v->SetValue("z", (value).z); \
	} \
	}


//------------------------------------------------------------------------
CScriptBind_Item::CScriptBind_Item(ISystem *pSystem, IGameFramework *pGameFramework)
: m_pSystem(pSystem),
	m_pSS(pSystem->GetIScriptSystem()),
	m_pGameFW(pGameFramework)
{
	Init(m_pSS, m_pSystem, 1);

	RegisterMethods();
	RegisterGlobals();

	m_stats.Create(m_pSystem->GetIScriptSystem());
	m_params.Create(m_pSystem->GetIScriptSystem());
}

//------------------------------------------------------------------------
CScriptBind_Item::~CScriptBind_Item()
{
}

//------------------------------------------------------------------------
void CScriptBind_Item::AttachTo(CItem *pItem)
{
	IScriptTable *pScriptTable = pItem->GetEntity()->GetScriptTable();

	if (pScriptTable)
	{
		SmartScriptTable thisTable(m_pSS);

		thisTable->SetValue("__this", ScriptHandle(pItem->GetEntityId()));
		thisTable->Delegate(GetMethodsTable());

		pScriptTable->SetValue("item", thisTable);
	}
}

//------------------------------------------------------------------------
void CScriptBind_Item::RegisterGlobals()
{
}

//------------------------------------------------------------------------
void CScriptBind_Item::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Item::
	SCRIPT_REG_TEMPLFUNC(Reset, "");
	SCRIPT_REG_TEMPLFUNC(Quiet, "");

	SCRIPT_REG_TEMPLFUNC(CanPickUp, "userId");
	SCRIPT_REG_TEMPLFUNC(CanUse, "userId");
	SCRIPT_REG_TEMPLFUNC(IsPickable, "");
	SCRIPT_REG_TEMPLFUNC(IsMounted, "");
	SCRIPT_REG_TEMPLFUNC(IsRippedOff, "");

	SCRIPT_REG_TEMPLFUNC(GetOwnerId, "");
	SCRIPT_REG_TEMPLFUNC(StartUse, "userId");
	SCRIPT_REG_TEMPLFUNC(StopUse, "userId");
	SCRIPT_REG_TEMPLFUNC(Use, "userId");
	SCRIPT_REG_TEMPLFUNC(IsUsed, "");
 	SCRIPT_REG_TEMPLFUNC(GetMountedDir, "");
	SCRIPT_REG_TEMPLFUNC(GetMountedAngleLimits, "");
	SCRIPT_REG_TEMPLFUNC(SetMountedAngleLimits,"min_pitch, max_pitch, yaw_range");

   SCRIPT_REG_TEMPLFUNC(OnHit, "hit");
  SCRIPT_REG_TEMPLFUNC(IsDestroyed, "");
	SCRIPT_REG_TEMPLFUNC(OnUsed, "userId");

	SCRIPT_REG_TEMPLFUNC(GetHealth, "");
	SCRIPT_REG_TEMPLFUNC(GetMaxHealth, "");
}

//------------------------------------------------------------------------
CItem *CScriptBind_Item::GetItem(IFunctionHandler *pH)
{
	void *pThis = pH->GetThis();

	if (pThis)
	{
		IItem *pItem = m_pGameFW->GetIItemSystem()->GetItem((EntityId)(UINT_PTR)pThis);
		if (pItem)
			return static_cast<CItem *>(pItem);
	}

	return 0;
}

//------------------------------------------------------------------------
CActor *CScriptBind_Item::GetActor(EntityId actorId)
{
	return static_cast<CActor *>(m_pGameFW->GetIActorSystem()->GetActor(actorId));
}

//------------------------------------------------------------------------
int CScriptBind_Item::Reset(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	pItem->Reset();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::Quiet(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	pItem->Quiet();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::CanPickUp(IFunctionHandler *pH, ScriptHandle userId)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(pItem->CanPickUp((EntityId)userId.n));
}

//------------------------------------------------------------------------
int CScriptBind_Item::CanUse(IFunctionHandler *pH, ScriptHandle userId)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(pItem->CanUse((EntityId)userId.n));
}

//------------------------------------------------------------------------
int CScriptBind_Item::IsPickable(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(pItem->IsPickable());
}

//------------------------------------------------------------------------
int CScriptBind_Item::IsMounted(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(pItem->IsMounted());
}

//------------------------------------------------------------------------
int CScriptBind_Item::IsRippedOff(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(pItem->IsRippedOff());
}

//------------------------------------------------------------------------
int CScriptBind_Item::GetOwnerId(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(ScriptHandle(pItem->GetOwnerId()));
}

//------------------------------------------------------------------------
int CScriptBind_Item::StartUse(IFunctionHandler *pH, ScriptHandle userId)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	pItem->StartUse((EntityId)userId.n);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::StopUse(IFunctionHandler *pH, ScriptHandle userId)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	pItem->StopUse((EntityId)userId.n);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::Use(IFunctionHandler *pH, ScriptHandle userId)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	pItem->Use((EntityId)userId.n);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::IsUsed(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(pItem->IsUsed());
}

//------------------------------------------------------------------------
int CScriptBind_Item::GetMountedDir(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(Script::SetCachedVector(pItem->GetStats().mount_dir, pH, 1));
}


//------------------------------------------------------------------------
int CScriptBind_Item::GetMountedAngleLimits(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	return pH->EndFunction(Script::SetCachedVector(pItem->GetMountedAngleLimits(), pH, 1));
}

//------------------------------------------------------------------------
int CScriptBind_Item::SetMountedAngleLimits(IFunctionHandler *pH, float min_pitch, float max_pitch, float yaw_range)
{
	CItem *pItem = GetItem(pH);
	if (pItem)
		pItem->SetMountedAngleLimits(min_pitch, max_pitch, yaw_range);
	return pH->EndFunction();
}


//------------------------------------------------------------------------
int CScriptBind_Item::OnHit(IFunctionHandler *pH, SmartScriptTable hitTable)
{
  CItem *pItem = GetItem(pH);
  if (!pItem)
    return pH->EndFunction();

  float damage = 0.f;
  hitTable->GetValue("damage", damage);
  char* damageType = 0;
  hitTable->GetValue("type",damageType);
  
  pItem->OnHit(damage,damageType);

  return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::IsDestroyed(IFunctionHandler *pH)
{
  CItem *pItem = GetItem(pH);
  if (!pItem)
    return pH->EndFunction(false);

  return pH->EndFunction(pItem->IsDestroyed());
}

//------------------------------------------------------------------------
int CScriptBind_Item::OnUsed(IFunctionHandler *pH, ScriptHandle userId)
{
	CItem *pItem = GetItem(pH);
	if (!pItem)
		return pH->EndFunction();

	CActor *pActor = GetActor((EntityId)userId.n);
	if (!pActor)
		return pH->EndFunction();

	if (pItem->CanUse((EntityId)userId.n))
	{
		pActor->UseItem(pItem->GetEntityId());
		return pH->EndFunction(true);
	}
	else if (pItem->CanPickUp((EntityId)userId.n))
	{
		//Should be always the client...
		if (pActor->IsClient())
		{
			CPlayer* pClientPlayer = static_cast<CPlayer*>(pActor);
			const CPlayer::SInteractionInfo& interactionInfo = pClientPlayer->GetCurrentInteractionInfo();
			bool expectedItem = (interactionInfo.interactiveEntityId == pItem->GetEntityId());
			bool expectedInteraction =	(interactionInfo.interactionType == CPlayer::eInteraction_PickupItem) || 
																	(interactionInfo.interactionType == CPlayer::eInteraction_ExchangeItem);
			if (!expectedItem || !expectedInteraction)
			{
				return pH->EndFunction();
			}

			if (interactionInfo.interactionType == CPlayer::eInteraction_ExchangeItem)
			{
				pActor->DropItem(pActor->GetCurrentItemId());
			}
		}
		pActor->PickUpItem(pItem->GetEntityId(), true);
		return pH->EndFunction(true);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::GetHealth(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (pItem)
		return pH->EndFunction(pItem->GetStats().health);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Item::GetMaxHealth(IFunctionHandler *pH)
{
	CItem *pItem = GetItem(pH);
	if (pItem)
		return pH->EndFunction(pItem->GetProperties().hitpoints);

	return pH->EndFunction();
}

//------------------------------------------------------------------------

#undef GVALUE
#undef SVALUE
#undef REUSE_VECTOR
