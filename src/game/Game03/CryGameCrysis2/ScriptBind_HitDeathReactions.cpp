#include "StdAfx.h"

#include "ScriptBind_HitDeathReactions.h"
#include "HitDeathReactions.h"

#include "Player.h"

#include <IGameRulesSystem.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CScriptBind_HitDeathReactions::CScriptBind_HitDeathReactions(ISystem* pSystem, IGameFramework* pGameFramework) : m_pSystem(pSystem), m_pGameFW(pGameFramework)
{
	Init(pSystem->GetIScriptSystem(), pSystem, 1);

	//////////////////////////////////////////////////////////////////////////
	// Init tables.
	//////////////////////////////////////////////////////////////////////////
	m_pParams.Create(m_pSS);

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_HitDeathReactions::

	SCRIPT_REG_TEMPLFUNC(ExecuteHitReaction, "reactionParams");
	SCRIPT_REG_TEMPLFUNC(ExecuteDeathReaction, "reactionParams");
	SCRIPT_REG_FUNC(EndCurrentReaction);
	SCRIPT_REG_FUNC(StartReactionAnim);
	SCRIPT_REG_FUNC(EndReactionAnim);
	SCRIPT_REG_TEMPLFUNC(IsValidReaction, "reactionParams, scriptHitInfo");
	SCRIPT_REG_TEMPLFUNC(SetDestructiblesLastEvent, "szLastEventName");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CScriptBind_HitDeathReactions::~CScriptBind_HitDeathReactions()
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CScriptBind_HitDeathReactions::ExecuteDeathReaction (IFunctionHandler *pH, SmartScriptTable reactionParams)
{
	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
		pHitDeathReactions->ExecuteDeathReaction(reactionParams);

	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CScriptBind_HitDeathReactions::ExecuteHitReaction (IFunctionHandler *pH, SmartScriptTable reactionParams)
{
	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
		pHitDeathReactions->ExecuteHitReaction(reactionParams);

	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CScriptBind_HitDeathReactions::EndCurrentReaction(IFunctionHandler *pH)
{
	bool bSuccess = false;

	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
		bSuccess = pHitDeathReactions->EndCurrentReaction();

	return pH->EndFunction(bSuccess);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int	CScriptBind_HitDeathReactions::IsValidReaction(IFunctionHandler *pH, SmartScriptTable reactionParams, SmartScriptTable scriptHitInfo)
{
	bool bResult = false;

	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
	{
		HitInfo hitInfo;
		CreateHitInfoFromScript(scriptHitInfo, hitInfo);

		bResult = pHitDeathReactions->IsValidReaction(hitInfo, reactionParams);
	}

	return pH->EndFunction(bResult);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CScriptBind_HitDeathReactions::StartReactionAnim(IFunctionHandler *pH)
{
	bool bResult = false;

	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
	{
		const char* szAnimName = NULL;
		bool bLoop = false;
		float fBlendTime = 0.2f;
		int iSlot = 0;
		int iLayer = 0;
		float fAniSpeed = 1.0f;
		uint32 animFlags = 0;

		if (!pH->GetParam(1, szAnimName))
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CScriptBind_HitDeathReactions::StartReactionAnim, animation name not specified");
		}
		else if (pH->GetParamCount() > 1)
		{
			pH->GetParam(2, bLoop);
			if (pH->GetParamCount() > 2)
			{
				pH->GetParam(3, fBlendTime);
				if (pH->GetParamCount() > 3)
				{
					pH->GetParam(4, iSlot);
					if (pH->GetParamCount() > 4)
					{
						pH->GetParam(5, iLayer);
						if (pH->GetParamCount() > 5)
						{
							pH->GetParam(5, fAniSpeed);
							if (pH->GetParamCount() > 6)
							{
								pH->GetParam(6, animFlags);
							}
						}
					}
				}
			}
		}
		
		bResult = pHitDeathReactions->StartReactionAnim(szAnimName, bLoop, fBlendTime, iSlot, iLayer, animFlags, fAniSpeed);
	}

	return pH->EndFunction(bResult);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CScriptBind_HitDeathReactions::EndReactionAnim(IFunctionHandler *pH)
{
	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
	{
		pHitDeathReactions->EndReactionAnim();
	}

	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int	CScriptBind_HitDeathReactions::SetDestructiblesLastEvent(IFunctionHandler *pH, const char* szLastEventName)
{	
	CHitDeathReactionsPtr pHitDeathReactions = GetHitDeathReactions(pH);
	if (pHitDeathReactions)
	{
		const Crc32Gen* pCRC32 = gEnv->pSystem->GetCrc32Gen();
		unsigned int lastEventCRC32 = (!szLastEventName || szLastEventName[0] == '\0') ? 0 : pCRC32->GetCRC32Lowercase(szLastEventName);
		pHitDeathReactions->SetDestructiblesLastEvent(lastEventCRC32);
	}

	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CPlayer* CScriptBind_HitDeathReactions::GetAssociatedActor(IFunctionHandler *pH) const
{
	SmartScriptTable selfTable;
	pH->GetSelf(selfTable);

	CRY_ASSERT(selfTable->HaveValue("__actor"));

	ScriptHandle actorEntityId;
	selfTable->GetValue("__actor", actorEntityId);

	IActor* pActor = m_pGameFW->GetIActorSystem()->GetActor(static_cast<EntityId>(actorEntityId.n));

	// [*DavidR | 13/Nov/2009] WARNING: This downcast could be dangerous if CHitDeathReactions is moved to 
	// CActor classes
	CRY_ASSERT(pActor && (pActor->GetActorClass() == CPlayer::GetActorClassType()));
	return static_cast<CPlayer*>(pActor);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CHitDeathReactionsPtr CScriptBind_HitDeathReactions::GetHitDeathReactions(IFunctionHandler *pH) const
{
	CPlayer* pActor = GetAssociatedActor(pH);
	CRY_ASSERT(pActor);

	return pActor ? pActor->GetHitDeathReactions() : CHitDeathReactionsPtr();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CScriptBind_HitDeathReactions::CreateHitInfoFromScript(SmartScriptTable scriptHitInfo, HitInfo &hitInfo) const
{
	CRY_ASSERT(scriptHitInfo);

	CScriptSetGetChain hit(scriptHitInfo);
	{
		hit.GetValue("normal", hitInfo.normal);
		hit.GetValue("pos", hitInfo.pos);
		hit.GetValue("dir", hitInfo.dir);
		hit.GetValue("partId", hitInfo.partId);

		ScriptHandle entId;
		const IEntitySystem* pEntitySystem = gEnv->pEntitySystem;

		hit.GetValue("targetId", entId);		
		hitInfo.targetId = static_cast<EntityId>(entId.n);

		hit.GetValue("shooterId", entId);
		hitInfo.shooterId = static_cast<EntityId>(entId.n);

		hit.GetValue("weaponId", entId);
		hitInfo.weaponId = static_cast<EntityId>(entId.n);

		hit.GetValue("projectileId", entId);
		hitInfo.projectileId = static_cast<EntityId>(entId.n);

		unsigned int uProjectileClassId = 0;
		hit.GetValue("projectileClassId", uProjectileClassId);
		hitInfo.projectileClassId = uProjectileClassId;

		hit.GetValue("materialId", hitInfo.material);

		hit.GetValue("damage", hitInfo.damage);
		hit.GetValue("radius", hitInfo.radius);

		hit.GetValue("typeId", hitInfo.type);
		hit.GetValue("remote", hitInfo.remote);
		hit.GetValue("bulletType", hitInfo.bulletType);

	}
}
