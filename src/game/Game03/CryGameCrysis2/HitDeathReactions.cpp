#include "StdAfx.h"

#include "HitDeathReactions.h"
#include "HitDeathReactionsSystem.h"
#include "ScriptBind_HitDeathReactions.h"

#include "Actor.h"
#include "Item.h"
#include "IAIActor.h"
#include "INavigation.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "StatsRecordingMgr.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

// As defined in SkeletonAnim.h (no other way to know the number of animation layers)
#define numVIRTUALLAYERS (0x10)

// Unnamed namespace for constants
namespace
{
	const char HIT_DEATH_REACTIONS_SCRIPT_TABLE[] = "HitDeathReactions";

	const char DEFAULT_KILL_REACTION_FUNCTION[] = "DefaultKillReaction";
	const char DEFAULT_HIT_REACTION_FUNCTION[] = "DefaultHitReaction";
	const char DEFAULT_VALIDATION_FUNCTION[] = "DefaultValidation";

	const char DEATH_REACTION_END_ANIMATION_EVENT[] = "DeathReactionEnd";

	const char AI_HIT_REACTION_GOALPIPE[] = "_SETINCODE_HIT_REACT_";
	const char AI_SIGNAL_FINISHED_REACTION[] = "OnFinishedHitDeathReaction";
	const char AI_SIGNAL_INTERRUPTED_REACTION[] = "OnHitDeathReactionInterrupted";

	const uint32 ANIM_USER_TOKEN = 'HdRA';

	const float MAXIMUM_REACTION_TIME = 4.0f;

	const float COLLISION_REACTION_DISTANCE_OFFSET = 0.4f;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct CHitDeathReactions::SPredFindValidReaction : public std::unary_function<bool, const SReactionParams&>
{
	SPredFindValidReaction(const CHitDeathReactions& owner, const HitInfo& hitInfo) : m_owner(owner), m_hitInfo(hitInfo) {}

	bool operator ()(const SReactionParams& reactionParams) const
	{
		bool bResult = false;

		HSCRIPTFUNCTION validationFunc = NULL;
		if (!reactionParams.sCustomValidationFunc.empty() && !m_owner.m_pSelfTable->GetValue(reactionParams.sCustomValidationFunc.c_str(), validationFunc))
			CHitDeathReactionsSystem::Warning("Couldn't find custom LUA method (method HitDeathReactions:%s). Executing default code", reactionParams.sCustomValidationFunc.c_str());

		if (!validationFunc && g_pGameCVars->g_hitDeathReactions_useLuaDefaultFunctions && !m_owner.m_pSelfTable->GetValue(DEFAULT_VALIDATION_FUNCTION, validationFunc))
			CHitDeathReactionsSystem::Warning("Couldn't find default LUA method (method HitDeathReactions:%s)", DEFAULT_VALIDATION_FUNCTION);

		bool bSuccess = false;
		if (validationFunc)
		{
			ScriptTablePtr scriptHitInfo(m_owner.m_pScriptSystem);
			g_pGame->GetGameRules()->CreateScriptHitInfo(scriptHitInfo, m_hitInfo);

			bSuccess = Script::CallReturn(m_owner.m_pScriptSystem, validationFunc, m_owner.m_pSelfTable, reactionParams.reactionScriptTable, scriptHitInfo, bResult);
			m_owner.m_pScriptSystem->ReleaseFunc(validationFunc);
		}

		if (!bSuccess)
		{
			// Default C++ validation
			bResult = m_owner.IsValidReaction(m_hitInfo, reactionParams);
		}

		return bResult;
	}

private:
	const CHitDeathReactions& m_owner;
	const HitInfo& m_hitInfo;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
inline void AnimIDError_Dup(const char *animName)
{
	if (g_pGameCVars->g_animatorDebug)
	{
		static const ColorF col (1.0f, 0.0f, 0.0f, 1.0f);
		g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText(string().Format("Missing %s", animName).c_str(), 1.0f, col, 10.0f);
	}

	CHitDeathReactionsSystem::Warning("Missing anim: %s", animName);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CHitDeathReactions::CHitDeathReactions(CActor& actor) : m_pScriptSystem(gEnv->pScriptSystem), m_actor(actor), 
m_fReactionCounter(-1.0f), m_iGoalPipeId(0), m_AIPausedState(eAIP_NotPaused), m_currentReaction(eRT_None), m_fReactionEndTime(-1.0f), 
m_pHitInfo(NULL), m_reactionOnCollision(NO_COLLISION_REACTION), m_currentExecutionType(eET_None), m_pDeathReactions(NULL), m_pHitReactions(NULL),
m_lastEventCRC32(0)
{
	m_pSelfTable.Create(m_pScriptSystem);

	LoadData(m_pSelfTable, false);

	// Add the methods from the scriptbind file to a "binds" table
	SmartScriptTable binds(m_pScriptSystem);
	binds->Delegate(g_pGame->GetHitDeathReactionsScriptBind()->GetMethodsTable());
	binds->SetValue("__actor", ScriptHandle(m_actor.GetEntityId()));
	m_pSelfTable->SetValue("binds", binds);

	// adding a hitDeathReactions subtable to the actor script so it can access it if needed 
	m_actor.GetEntity()->GetScriptTable()->SetValue("hitDeathReactions", m_pSelfTable);

	// Subscribe to deferred raycast events
	m_raycastHelper.SetReceiver(this);

	if (gEnv->bServer)
	{
		m_iGoalPipeId = gEnv->pAISystem->AllocGoalPipeId();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::Update(float fFrameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (IsInReaction())
	{
#ifdef _DEBUG
		// Debug check: If marked to be disabled, the AI SHOULD be disabled while playing a reaction
		if (eAIP_PipeUserPaused == m_AIPausedState)
		{
			IPipeUser* pPipeUser = GetAIPipeUser();
			CRY_ASSERT_TRACE(!pPipeUser || pPipeUser->IsPaused(), ("The actor %s pipe user has been resumed before the reaction has ended! Investigate", m_actor.GetEntity()->GetName()));
		}
#endif

		// Protection against bad animation graph setups or other errors
		m_fReactionCounter += fFrameTime;
		if ((m_fReactionEndTime >= 0.0f) && (m_fReactionCounter > m_fReactionEndTime))
		{
			EndCurrentReaction(true);
		}
		else if (m_fReactionCounter > MAXIMUM_REACTION_TIME)
		{
			CHitDeathReactionsSystem::Warning("Current %s reaction for entity %s was" 
				" taking too long (more than %f seconds) so its end was forced. Talk with David Ramos if its long duration was intended", 
				IsInDeathReaction() ? "Death" : "Hit", m_actor.GetEntity()->GetName(), MAXIMUM_REACTION_TIME);

			EndCurrentReaction();
		}
		else if (m_reactionOnCollision != NO_COLLISION_REACTION)
		{
			// [*DavidR | 26/Mar/2010] FIX: HACK: temporary CVar for disabling animated reactions for milestone 3 if necessary
			// we leave reactions on collisions greater than 1 (so Fall n Play reactions to collisions is allowed, for heavy-caliber
			// reactions not going into geometry)
			bool bForbidCollisions = g_pGameCVars->g_hitDeathReactions_disableHitAnimatedCollisions && 
				IsInHitReaction() && (m_reactionOnCollision > 1);
		
			if (!bForbidCollisions)
				DoCollisionCheck();
		}
	}

	if (CustomAnimHasFinished())
	{
		// If no animation graph is working on that layer stop the animation, otherwise it will be played there forever
		OnCustomAnimFinished();
	}

#ifndef _RELEASE
	if (g_pGameCVars->g_hitDeathReactions_debug)
	{
		DrawDebugInfo();
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::OnKill(const HitInfo& hitInfo)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	CRY_ASSERT(m_actor.IsDead());

	bool bSuccess = false;

	if (CanPlayDeathReaction())
	{
		// Generate and set seed for this event
		uint32 uSeed = GetSynchedSeed(true);
		m_pseudoRandom.seed(uSeed);

		// Choose proper death reaction
		ReactionsContainer::const_iterator itBestFit = std::find_if(m_pDeathReactions->begin(), m_pDeathReactions->end(), SPredFindValidReaction(*this, hitInfo));

		// If found a fit, execute it
		if (itBestFit != m_pDeathReactions->end())
		{
			bSuccess = StartDeathReaction(hitInfo, *itBestFit);
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::OnHit(const HitInfo& hitInfo)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	bool bSuccess = false;

	if (CanPlayHitReaction())
	{
		// Generate and set seed for this event
		uint32 uSeed = GetSynchedSeed(false);
		m_pseudoRandom.seed(uSeed);

		// Choose proper hit reaction
		ReactionsContainer::const_iterator itBestFit = std::find_if(m_pHitReactions->begin(), m_pHitReactions->end(), SPredFindValidReaction(*this, hitInfo));

		// If found a fit, execute it
		if (itBestFit != m_pHitReactions->end())
		{
			bSuccess = StartHitReaction(hitInfo, *itBestFit);
		}	
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
// Only for explosion-caused HIT reactions
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::OnExplosion(const ExplosionInfo& explosionInfo)
{
	if (CanPlayHitReaction())
	{
		// Convert Explosion info to HitInfo
		HitInfo explHit;
		explHit.pos = explosionInfo.pos;
		explHit.dir = (m_actor.GetEntity()->GetWorldPos() - explosionInfo.pos).GetNormalized();
		explHit.radius = explosionInfo.radius;
		explHit.partId = -1;
		explHit.targetId = explosionInfo.impact_targetId;
		explHit.weaponId = explosionInfo.weaponId;
		explHit.shooterId = explosionInfo.shooterId;
		explHit.projectileId = explosionInfo.projectileId;
		explHit.damage = explosionInfo.damage;
		explHit.type = explosionInfo.type;

		return OnHit(explHit);
	}
	

	return false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::OnCollision(const EventPhysCollision& collision)
{
	// [*DavidR | 19/Feb/2010] This method is faster than the PWI one and greatly simplifies things, 
	// but requires that the character silhouette fits nicely the living physic entity collision volume
	// Could it be used for simple knockabouts? Anyway, we'll probably need to enable this method as an 
	// option on the reaction params
	/* 
	if (IsInReaction() && (collision.pEntity[1]->GetType() == PE_STATIC))
	{
	// if we are playing a death reaction, we may want to enable the ragdoll
	IMovementController* pMovementController = m_actor.GetMovementController();
	if (pMovementController)
	{
	// Movement opposite to collision normal, otherwise we will slide
	Vec3 vMoveDir(m_actor.GetActorStats()->velocity.x, m_actor.GetActorStats()->velocity.y, 0.0f);
	vMoveDir.NormalizeFast();

	if (collision.n.Dot(vMoveDir) < -0.8f)
	EndCurrentReaction();
	}
	}*/

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::OnRevive()
{
	ClearState();
}

//////////////////////////////////////////////////////////////////////////
// returns true if processed this event
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::OnAnimationEvent(const AnimEventInstance &event, const uint32 eventNameCRC)
{
	bool bSuccessfullyProcessed = false;

	if (IsInReaction())
	{
		if (m_actor.GetAnimationEventsTable().m_deathReactionEndId == eventNameCRC)
		{
			bSuccessfullyProcessed = EndCurrentReaction(true);
		}
		else if (m_actor.GetAnimationEventsTable().m_ragdollStartId == eventNameCRC)
		{
			//--- Initiate rag-doll between now & the end of the animation
			CAnimationPlayerProxy *animPlayerProxy = m_actor.GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
			const CAnimation *animPtr = animPlayerProxy ? animPlayerProxy->GetTopAnimation(m_actor.GetEntity(), 0) : NULL;
			if (animPtr)
			{
				m_fReactionEndTime = m_fReactionCounter + (GetRandomProbability() * (animPtr->m_fCurrentDuration * (1.0f - animPtr->m_fAnimTime)));

				bSuccessfullyProcessed = true;
			}
		}
		else if (m_actor.GetAnimationEventsTable().m_reactionOnCollision == eventNameCRC)
		{
			// Change the current state of the reactionOnCollision flag
			if (event.m_CustomParameter && *event.m_CustomParameter)
			{
				m_reactionOnCollision = atoi(event.m_CustomParameter);

				bSuccessfullyProcessed = true;
			}
		}
	}

	return bSuccessfullyProcessed;
}

/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::HandleEvent(const SGameObjectEvent& event)
{
	bool bProcessed = true;

	switch(event.event)
	{
	case eCGE_ReactionEnd:
		{
			bProcessed = EndCurrentReaction();
		}
		break;
	default:
		// [*DavidR | 24/Nov/2009] ToDo: Invoke an event handler function within HitDeathReactions.lua?
		bProcessed = false;
		break;
	}

	return bProcessed;
}

/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::Reload()
{
	// Reload the data structure and scripts
	LoadData(m_pSelfTable, true);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::IsValidReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams) const
{
	// Check speed
	const SActorStats* pActorStats = m_actor.GetActorStats();
	CRY_ASSERT(pActorStats);

	if (reactionParams.fMinimumSpeedAllowed > pActorStats->speedFlat)
		return false;

	if (pActorStats->speedFlat > reactionParams.fMaximumSpeedAllowed)
		return false;

	// Check damage
	if (reactionParams.fMinimumDamageAllowed > hitInfo.damage)
		return false;

	if (hitInfo.damage > reactionParams.fMaximumDamageAllowed)
		return false;

	// Check movement direction
	if (reactionParams.movementDir != eCD_Invalid)
	{
		IMovementController* pMovementController = m_actor.GetMovementController();
		if (pMovementController)
		{
			// Get move direction of entity
			SMovementState state;
			pMovementController->GetMovementState(state);
			Vec2 vMoveDir(state.movementDirection);
			if (vMoveDir.IsValid())
			{
				if (!CheckCardinalDirection2D(reactionParams.movementDir, Vec2(m_actor.GetEntity()->GetForwardDir()), vMoveDir))
					return false;
			}
			else
				return false;
		}
	}

	// Check hit joint (if the allowedPartIds set is empty, any part will be valid)
	const SReactionParams::IdContainer& allowedPartIds = reactionParams.allowedPartIds;
	if (!allowedPartIds.empty() && (allowedPartIds.find(hitInfo.partId) == allowedPartIds.end()))
		return false;

	// Check shot origin
	if (reactionParams.shotOrigin != eCD_Invalid)
	{
		Vec2 v2DShotDir(hitInfo.dir);
		if (v2DShotDir.IsValid())
		{
			if (!CheckCardinalDirection2D(reactionParams.shotOrigin, Vec2(m_actor.GetEntity()->GetForwardDir()), -v2DShotDir))
				return false;
		}
		else
			return false;
	}

	// Check probability
	if ((reactionParams.fProbability != 1.0f) && (GetRandomProbability() > reactionParams.fProbability))
		return false;

	// Check stance
	const SReactionParams::StanceContainer& allowedStances = reactionParams.allowedStances;
	if (!allowedStances.empty() && (allowedStances.find(m_actor.GetStance()) == allowedStances.end()))
		return false;

	// Check hit type
	const SReactionParams::IdContainer& allowedHitTypes = reactionParams.allowedHitTypes;
	if (!allowedHitTypes.empty() && (allowedHitTypes.find(hitInfo.type) == allowedHitTypes.end()))
		return false;

	// Check projectile class
	const SReactionParams::IdContainer& allowedProjectiles = reactionParams.allowedProjectiles;
	if (!allowedProjectiles.empty() && (allowedProjectiles.find(hitInfo.projectileClassId) == allowedProjectiles.end()))
		return false;

	// Check weapon class
	const SReactionParams::IdContainer& allowedWeapons = reactionParams.allowedWeapons;
	if (!allowedWeapons.empty())
	{
		uint16 uWeaponClassId = 0;

		const IEntity* pWeaponEntity = gEnv->pEntitySystem->GetEntity(hitInfo.weaponId);
		if (pWeaponEntity && 
			g_pGame->GetIGameFramework()->GetNetworkSafeClassId(uWeaponClassId, pWeaponEntity->GetClass()->GetName()) && 
			(allowedWeapons.find(uWeaponClassId) == allowedWeapons.end()))
			return false;
	}

	// Check using mounted item
	if (reactionParams.bAllowOnlyWhenUsingMountedItems && (pActorStats->mountedWeaponID == 0))
		return false;

	// Check destructible parts event
	if ((reactionParams.destructibleEvent != 0) && (reactionParams.destructibleEvent != m_lastEventCRC32))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::StartHitReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams)
{
	bool bSuccess = false;

	EndCurrentReaction();
	SetInReaction(eRT_Hit);
	m_pHitInfo = &hitInfo;
	m_reactionOnCollision = reactionParams.reactionOnCollision;

	if (!reactionParams.sCustomExecutionFunc.empty())
	{
		bSuccess = Script::CallMethod(m_pSelfTable, reactionParams.sCustomExecutionFunc.c_str(), reactionParams.reactionScriptTable);
		if (bSuccess)
			SetExecutionType(eET_Custom);
	}
	else
	{
		bSuccess = g_pGameCVars->g_hitDeathReactions_useLuaDefaultFunctions ? Script::CallMethod(m_pSelfTable, DEFAULT_HIT_REACTION_FUNCTION, reactionParams.reactionScriptTable) : false;
		if (!bSuccess)
		{
			CRY_ASSERT_MESSAGE(!g_pGameCVars->g_hitDeathReactions_useLuaDefaultFunctions, "Can't run default hit reaction lua method. Check HitDeathReactions.lua");

			// Default execution
			bSuccess = ExecuteHitReaction(reactionParams);
		}
	}


	if (bSuccess && 
		!IsExecutionType(eET_None) && IsInReaction()) // Custom reactions can invoke EndCurrentReaction inside them if they fail, so we need to react accordingly
	{
		m_fReactionCounter = 0.0f;
		m_fReactionEndTime = -1.0f;

		if (IsAI())
		{
			HandleAIStartHitReaction(hitInfo, reactionParams);
		}
	}
	else
	{
		SetInReaction(eRT_None);
		SetExecutionType(eET_None);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::StartDeathReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams)
{
	bool bSuccess = false;

	EndCurrentReaction();
	SetInReaction(eRT_Death);
	m_pHitInfo = &hitInfo;
	m_reactionOnCollision = reactionParams.reactionOnCollision;

	if (!reactionParams.sCustomExecutionFunc.empty())
	{
		bSuccess = Script::CallMethod(m_pSelfTable, reactionParams.sCustomExecutionFunc.c_str(), reactionParams.reactionScriptTable);
		if (bSuccess)
			SetExecutionType(eET_Custom);
	}
	else
	{
		bSuccess = g_pGameCVars->g_hitDeathReactions_useLuaDefaultFunctions ? Script::CallMethod(m_pSelfTable, DEFAULT_KILL_REACTION_FUNCTION, reactionParams.reactionScriptTable) : false;
		if (!bSuccess)
		{
			CRY_ASSERT_MESSAGE(!g_pGameCVars->g_hitDeathReactions_useLuaDefaultFunctions, "Can't run default kill reaction lua method. Check HitDeathReactions.lua");

			// Default C++ execution
			bSuccess = ExecuteDeathReaction(reactionParams);
		}
	}

	if (bSuccess && !IsExecutionType(eET_None))
	{
		m_fReactionCounter = 0.0f;
		m_fReactionEndTime = -1.0f;
	}
	else
	{
		SetInReaction(eRT_None);
		SetExecutionType(eET_None);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::HandleAIStartHitReaction(const HitInfo& hitInfo, const SReactionParams& reactionParams)
{
	CRY_ASSERT(IsAI());

	if (!reactionParams.sCustomAISignal.empty())
	{
		IAISignalExtraData *pData = gEnv->pAISystem->CreateSignalExtraData();
		if (pData)
		{
			pData->iValue = m_iGoalPipeId;
			pData->point = hitInfo.pos;
			pData->point2 = hitInfo.dir;
		}

		SendAISignal(reactionParams.sCustomAISignal.c_str(), pData);

		m_AIPausedState = eAIP_ExecutingCustomSignal;
	}
	else if (reactionParams.bPauseAI)
	{
		// If the AI is using a Mounted Item, force stop using it
		CItem* pCurrentItem = static_cast<CItem*>(m_actor.GetCurrentItem());
		if(pCurrentItem && pCurrentItem->IsMounted() && pCurrentItem->IsUsed())
			pCurrentItem->StopUse(m_actor.GetEntityId());
		
		PausePipeUser(true);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::HandleAIEndHitReaction()
{
	CRY_ASSERT(IsAI());

	if (IsInReaction())
	{
		IAISignalExtraData *pData = gEnv->pAISystem->CreateSignalExtraData();
		if (pData)
		{
			pData->iValue = m_iGoalPipeId;
		}

		SendAISignal(AI_SIGNAL_FINISHED_REACTION, pData);
	}

	if (eAIP_ExecutingCustomSignal != m_AIPausedState)
	{
		PausePipeUser(false);
	}

	m_AIPausedState = eAIP_NotPaused;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::HandleAIHitReactionInterrupted()
{
	CRY_ASSERT(IsAI());

	if (IsInReaction())
	{
		IAISignalExtraData *pData = gEnv->pAISystem->CreateSignalExtraData();
		if (pData)
		{
			pData->iValue = m_iGoalPipeId;
		}

		SendAISignal(AI_SIGNAL_INTERRUPTED_REACTION, pData);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::ExecuteHitReaction(const SReactionParams& reactionParams)
{
	bool bSuccess = false;

	CCCPOINT(HitDeath_ExecuteHitReaction);

	if (IsInHitReaction())
	{
		bSuccess = ExecuteCommonReaction(reactionParams);
	}
	else
	{  
		CHitDeathReactionsSystem::Warning("[CHitDeathReactions::ExecuteHitReaction] Can't execute a hit reaction when "
			"not in Hit Reaction mode. If you invoked this function from lua, make sure you're doing it from a reactionFunc method inside HitDeathReactions.lua");
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::ExecuteDeathReaction(const SReactionParams& reactionParams)
{
	bool bSuccess = false;

	CCCPOINT(HitDeath_ExecuteDeathReaction);

	if (IsInDeathReaction())
	{
		bSuccess = ExecuteCommonReaction(reactionParams);
	}
	else
	{
		CHitDeathReactionsSystem::Warning("Can't execute a death reaction when "
			"not in Death Reaction mode. If you invoked this function from lua, make sure you're doing it from a reactionFunc method inside HitDeathReactions.lua");
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::IsValidReaction(const HitInfo& hitInfo, const ScriptTablePtr pScriptTable) const
{
	bool bIsValidReaction = false;

	ReactionId reactionId = INVALID_REACTION_ID;
	if (pScriptTable->GetValue(REACTION_ID, reactionId) && IsValidReactionId(reactionId))
		bIsValidReaction = IsValidReaction(hitInfo, GetReactionParamsById(reactionId));

	return bIsValidReaction;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::ExecuteHitReaction(const ScriptTablePtr pScriptTable)
{
	bool bRet = false;

	ReactionId reactionId = INVALID_REACTION_ID;
	if (pScriptTable->GetValue(REACTION_ID, reactionId) && IsValidReactionId(reactionId))
		bRet = ExecuteHitReaction(GetReactionParamsById(reactionId));

	return bRet;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::ExecuteDeathReaction(const ScriptTablePtr pScriptTable)
{
	bool bRet = false;

	ReactionId reactionId = INVALID_REACTION_ID;
	if (pScriptTable->GetValue(REACTION_ID, reactionId) && IsValidReactionId(reactionId))
		bRet = ExecuteDeathReaction(GetReactionParamsById(reactionId));

	return bRet;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::EndCurrentReaction(bool bForceRagdollOnHit/* = false*/)
{
	bool bSuccess = true;
	bool bEndingDeathAnimation = IsInDeathReaction();
	bool bEndingHitAnimation = IsInHitReaction();
	
	if (IsAI())
	{
		HandleAIEndHitReaction();
	}

	if (IsPlayingReactionAnim())
	{
		EndReactionAnim();
	}
	else if (IsExecutionType(eET_AnimationGraphAnim))
	{
		// Leave reaction state
		ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(0);
		if (pCharacter)
		{
			// [*DavidR | 16/Feb/2010] HACK: Only known way to force leave state of an anim graph state that forces
			// to be in there until time is 1.0 without breaking the blend-in of the next state :´(
			ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
			int iNumAnims = pISkeletonAnim->GetNumAnimsInFIFOAndDeferredQueue(0);
			for (int i = 0; i < iNumAnims; ++i)
				pISkeletonAnim->GetAnimFromFIFOAndDeferredQueue(0, i).m_fAnimTime = 1.0f;
		}	
	}

	if (gEnv->bServer)
	{
		if (bEndingDeathAnimation && !g_pGameCVars->g_hitDeathReactions_disableRagdoll)
		{
			// Death reactions always end with a ragdoll
			bSuccess = m_actor.GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
		}
		else if (bEndingHitAnimation && bForceRagdollOnHit)
		{
			// We can force fall and play in the end of some hit reactions
			m_actor.Fall();
			bSuccess = true;
		}
	}

	ClearState();

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::StartReactionAnim(const string& sAnimName, bool bLoop/* = false*/, float fBlendTime/* = 0.2f*/, int iSlot/* = 0*/, int iLayer/* = 0*/, uint32 animFlags /*= (DEFAULT_REACTION_ANIM_FLAGS)*/, float fAdditiveWeight/* = 0.0f*/, float fAniSpeed/* = 1.0f*/)
{
	bool bSuccess = false;

	ICharacterInstance *pMainChar = m_actor.GetEntity()->GetCharacter(0);
	CRY_ASSERT(pMainChar);
	IAnimationSet *pAnimSet = pMainChar ? pMainChar->GetIAnimationSet() : NULL;
	if (pAnimSet)
	{
		int animID = pAnimSet->GetAnimIDByName(sAnimName.c_str());
		if (animID >= 0)
		{
			bSuccess = StartReactionAnimByID(animID, bLoop, fBlendTime, iSlot, iLayer, animFlags, fAdditiveWeight, fAniSpeed);
		}
		else
			AnimIDError_Dup(sAnimName.c_str());
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::StartReactionAnimByID(int animID, bool bLoop/* = false*/, float fBlendTime/* = 0.2f*/, int iSlot/* = 0*/, int iLayer/* = 0*/, uint32 animFlags /*= (DEFAULT_REACTION_ANIM_FLAGS)*/, float fAdditiveWeight/* = 0.0f*/, float fAniSpeed/* = 1.0f*/)
{
	CRY_ASSERT(fAniSpeed > 0.0f);

	CRY_ASSERT_MESSAGE(IsInReaction(), "This method should be called only during a hit or death reaction");
	if (!IsInReaction())
	{
		CHitDeathReactionsSystem::Warning("Can't start an animation through HitDeathReactions system "
			"outside of a reaction. If you invoked this function from lua, make sure you're doing it from a reactionFunc method inside HitDeathReactions.lua");

		return false;
	}

	bool bResult = false;

	ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(iSlot);
	if (pCharacter)
	{
		CryCharAnimationParams animParams;
		animParams.m_nLayerID = iLayer;
		animParams.m_fTransTime = fBlendTime;
		animParams.m_nFlags = animFlags;
		animParams.m_nFlags |= bLoop ? CA_LOOP_ANIMATION : 0;
		animParams.m_nUserToken = ANIM_USER_TOKEN;

		// [*DavidR | 19/Feb/2010] ToDo: !canMix states on the AG or lower layers animations with CA_DISABLE_MULTILAYER
		// can negate the effects of partial/additive anims on higher layers. Find a solution to this
		MakePreviousAnimInterruptible(iSlot, animParams.m_nLayerID);

		ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
		CAnimationPlayerProxy* pPlayerProxy = m_actor.GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
		if (pPlayerProxy)
		{
			bResult = pPlayerProxy->StartAnimationById(m_actor.GetEntity(), animID, animParams, fAniSpeed);
		}
		else
		{
			bResult = pISkeletonAnim->StartAnimationById(animID, animParams);
		}

		if (bResult)
		{
			bool bAdditive = fAdditiveWeight > 0.0f;
			if (bAdditive)
			{
				pISkeletonAnim->SetAdditiveWeight(animParams.m_nLayerID, fAdditiveWeight);
			}
			else
			{
				// If not additive, stop higher layers
				StopHigherLayers(iSlot, animParams.m_nLayerID, animParams.m_fTransTime);

				// Disable lookIk
				// [*DavidR | 18/Feb/2010] ToDo: Optionally?
				m_actor.GetAnimatedCharacter()->AllowLookIk(false);
			}

			if (!pPlayerProxy)
			{
				pISkeletonAnim->SetLayerUpdateMultiplier(animParams.m_nLayerID, fAniSpeed);
			}

			// Pause animation graph when playing animations on layer 0 (otherwise they will be overwritten by state changes)
			if (animParams.m_nLayerID == eAnimationGraphLayer_FullBody)
			{
				m_actor.GetAnimationGraphState()->Pause(true, eAGP_PlayAnimationNode);

				// Set default movement control method and collider mode for fullbody animations
				m_actor.GetAnimatedCharacter()->SetMovementControlMethods(eMCM_AnimationHCollision, eMCM_SmoothedEntity);
				m_actor.GetAnimatedCharacter()->RequestPhysicalColliderMode(eColliderMode_Pushable, eColliderModeLayer_Game);
			}

			m_currentCustomAnim.animParams = animParams;
			m_currentCustomAnim.sAnimName = pCharacter->GetIAnimationSet()->GetNameByAnimID(animID);
			m_currentCustomAnim.iLayer = iLayer;
		}
		else
			CHitDeathReactionsSystem::Warning("Failed to start reaction anim %s on character %s", pCharacter->GetIAnimationSet()->GetNameByAnimID(animID), m_actor.GetEntity()->GetName());
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::EndReactionAnim()
{
	if (!IsInDeathReaction())
	{
		if (IsPlayingReactionAnim())
		{
			OnCustomAnimFinished();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::IsPlayingReactionAnim() const
{
	bool bPlayingAnim = false;

	if (IsExecutionType(eET_ReactionAnim))
	{
		CAnimationPlayerProxy* pPlayerProxy = m_actor.GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
		if (pPlayerProxy)
		{
			const CAnimation *anim = pPlayerProxy->GetTopAnimation(m_actor.GetEntity(), m_currentCustomAnim.animParams.m_nLayerID);

			bPlayingAnim = anim && (anim->m_AnimParams.m_nUserToken == ANIM_USER_TOKEN) && (anim->m_fAnimTime < 1.0f);
		}
		else
		{
			ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(0);
			ISkeletonAnim* pSkel = pCharacter ? pCharacter->GetISkeletonAnim() : NULL;
			const int iNumAnims = pSkel ? pSkel->GetNumAnimsInFIFOAndDeferredQueue(m_currentCustomAnim.animParams.m_nLayerID) : 0;
			if (iNumAnims > 0)
			{
				const CAnimation& anim = pSkel->GetAnimFromFIFOAndDeferredQueue(m_currentCustomAnim.animParams.m_nLayerID, iNumAnims - 1);

				// If the latest anim on the Anim Queue of this layer is a Reaction anim and hasn't reached its end, is
				// being played
				bPlayingAnim = (anim.m_AnimParams.m_nUserToken == ANIM_USER_TOKEN) && (anim.m_fAnimTime < 1.0f);
			}
		}
	}

	return bPlayingAnim;
}

//////////////////////////////////////////////////////////////////////////
//async raycasts results callback
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::OnDataReceived(const EventPhysRWIResult *pRWIResult)
{

}

//////////////////////////////////////////////////////////////////////////
//async primitive casts results callback
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::OnDataReceived(const EventPhysPWIResult *pPWIResult)
{
	// [*DavidR | 25/Feb/2010] ToDo: All the magic numbers in this method should be converted to constants or parameters

	if (!pPWIResult || (pPWIResult->dist <= 0.0f)	||			// Why is this check necessary? if dist == 0.0f physic event data is bogus. WHY?
		(m_reactionOnCollision == NO_COLLISION_REACTION))		// No Collision if we are not supposed to
		return;

	ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(0);
	CRY_ASSERT(pCharacter);

	bool bValidCollision = IsInDeathReaction() || (m_reactionOnCollision == 1); // [*DavidR | 26/Mar/2010] ToDo: Tidy-up this assumption of reaction on collision number 1 being the FnP/ragdoll
	if (!bValidCollision && pCharacter)
	{
		// In hit reactions we need to be more accurate with the checks, check:
		// - collision normal horizontality (reaction collisions are designed for perfect vertical obstacles)
		// - collision normal reasonably opposing movement direction (should we take velocity into account?)
		// - height?
		const float fMaxZ = sin(0.34f); // 0.34 rads = 20º
		const bool bHorizontalCheck = cry_fabsf(pPWIResult->n.z) < fMaxZ;

		const Vec3& vRelDir = pCharacter->GetISkeletonAnim()->GetRelMovement().t.GetNormalized();
		const Vec3& vAbsDir = m_actor.GetEntity()->GetWorldTM().TransformVector(vRelDir);

		const float fMinDot = 0.7071f; // a cone of 90º (so this is cos(DEG2RAD(45)))
		const bool bDirectCollisionCheck = pPWIResult->n.Dot(vAbsDir) > fMinDot;

		bValidCollision = bHorizontalCheck && bDirectCollisionCheck;
	}

#ifndef _RELEASE
	if (g_pGameCVars->g_hitDeathReactions_debug && pCharacter)
	{
		// Let's draw the normal vector and the movement vector for every collision detected, if the collision
		// is valid we draw it for more time (so it's easy to see which one was the validated)
		const Vec3& vConeDrawPos = pPWIResult->pt + Vec3(0.0f, 0.0f, 2.0f); 
		float fTime = bValidCollision ? 4.0f : 2.0f;
		const Vec3& vRelDir = pCharacter->GetISkeletonAnim()->GetRelMovement().t.GetNormalized();
		const Vec3& vAbsDir = m_actor.GetEntity()->GetWorldTM().TransformVector(vRelDir);

		IPersistantDebug* pPersistantDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
		pPersistantDebug->Begin("CHitDeathReactions::OnDataReceived", false);
		pPersistantDebug->AddCone(vConeDrawPos, pPWIResult->n, 0.1f, 0.8f, Col_Red, fTime);
		pPersistantDebug->AddCone(vConeDrawPos, vAbsDir, 0.1f, 0.8f, Col_Blue, fTime);
	}
#endif // _RELEASE

	if (bValidCollision)
	{
		StartCollisionReaction(pPWIResult->n, pPWIResult->pt);
	}
}

//////////////////////////////////////////////////////////////////////////
//reset data callback
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::OnDRWReset()
{

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::ClearState()
{
	SetInReaction(eRT_None);
	SetExecutionType(eET_None);
	m_fReactionCounter = -1.0f;	// we give it a -1.0f value to be more readable when debugging. Could be 0.0f 
	m_fReactionEndTime = -1.0f;
	m_pHitInfo = NULL;
	m_reactionOnCollision = NO_COLLISION_REACTION;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::CanPlayHitReaction() const
{
	return (g_pGameCVars->g_hitDeathReactions_enable != 0) && 
		!m_actor.IsDead() && 
		!m_actor.GetLinkedVehicle() && // [*DavidR | 17/Feb/2010] Temporary: No reactions in vehicles
		!IsInReaction() && // For now, don't allow hit reactions to interrupt other hit reactions
		!m_actor.IsFallen() && // Don't try reactions when the actor is in Fall and Play fall stage
		(!m_actor.GetActorStats() || !m_actor.GetActorStats()->isGrabbed ) && // Not grabbed by pick and throw
		!IsAIPlayingTargetPhase(); // Block hit reactions based on the actor's target phase (playing exact positioning)
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::CanPlayDeathReaction() const
{
	return (g_pGameCVars->g_hitDeathReactions_enable != 0) && 
		m_actor.IsDead() && 
		!m_actor.GetLinkedVehicle() && // [*DavidR | 17/Feb/2010] Temporary: No reactions in vehicles
		!IsInDeathReaction() &&
		(!m_actor.GetActorStats() || !m_actor.GetActorStats()->isGrabbed ) && // Not grabbed by pick and throw
		!IsAIPlayingTargetPhase(); // Block hit reactions based on the actor's target phase (playing exact positioning)
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::IsAIPlayingTargetPhase() const
{
	bool bResult = false;

	IAIObject *pAI = m_actor.GetEntity()->GetAI();
	IAIActorProxy *pAIProxy = pAI ? pAI->GetProxy() : NULL;
	if (pAIProxy)
	{
		EActorTargetPhase ePhase = pAIProxy->GetActorTargetPhase();
		bResult = (ePhase == eATP_Playing);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::SetInReaction(EReactionType reactionType)
{
	m_currentReaction = reactionType;
}

/*
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::SetHitReactGoalPipe(IPipeUser *pPipeUser) const
{
	CRY_ASSERT(gEnv->bServer);

	if (pPipeUser)
	{
		pPipeUser->InsertSubPipe(AIGOALPIPE_HIGHPRIORITY | AIGOALPIPE_KEEP_ON_TOP | AIGOALPIPE_NOTDUPLICATE, AI_HIT_REACTION_GOALPIPE, NULL, m_iGoalPipeId);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::ClearHitReactGoalPipe(IPipeUser *pPipeUser) const
{
	CRY_ASSERT(gEnv->bServer);

	if (pPipeUser && pPipeUser->IsUsingPipe(m_iGoalPipeId))
	{
		pPipeUser->CancelSubPipe(m_iGoalPipeId);
	}
}
*/

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::IsAI() const
{
	return (gEnv->bServer && !m_actor.IsPlayer());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
IPipeUser* CHitDeathReactions::GetAIPipeUser() const
{
	IPipeUser *pPipeUser = NULL;

	if (IsAI())
	{
		IAIObject* pAI = m_actor.GetEntity()->GetAI();
		pPipeUser = CastToIPipeUserSafe(pAI);
	}

	return pPipeUser;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::SendAISignal(const char* szSignal, IAISignalExtraData *pData, bool bWaitOpOnly) const
{
	CRY_ASSERT(szSignal && szSignal[0]);

	bool bResult = false;

	IAIObject *pAI = (IsAI() ? m_actor.GetEntity()->GetAI() : NULL);
	if (pAI)
	{
		if (!bWaitOpOnly)
		{
			gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER, 0, szSignal, pAI, pData);
			bResult = true;
		}
		else if (IAIActor *pAIActor = CastToIAIActorSafe(pAI))
		{
			pAIActor->SetSignal(AISIGNAL_NOTIFY_ONLY, szSignal, m_actor.GetEntity(), pData);
			bResult = true;
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::ExecuteCommonReaction(const SReactionParams& reactionParams)
{
	CRY_ASSERT(m_pHitInfo);
	bool bSuccess = false;

	if (!reactionParams.agReaction.sAGInputValue.empty())
	{
		SetVariations(reactionParams.agReaction.variations);
		bSuccess = m_actor.GetAnimationGraphState()->SetInput("Signal", reactionParams.agReaction.sAGInputValue.c_str());
		if (bSuccess)
		{
			m_actor.GetAnimationGraphState()->ForceTeleportToQueriedState();

			SetExecutionType(eET_AnimationGraphAnim);
		}
	}
	else if (!reactionParams.reactionAnim.animIDs.empty())
	{
		const SReactionParams::SReactionAnim& reactionAnim = reactionParams.reactionAnim;
		int animNum = m_pseudoRandom.Generate() % reactionParams.reactionAnim.animIDs.size();
		bSuccess = StartReactionAnimByID(reactionAnim.animIDs[animNum], false, 0.1f, 0, reactionAnim.iLayer, reactionAnim.animFlags, reactionAnim.bAdditive ? 1.0f : 0.0f);
		if (bSuccess)
			SetExecutionType(eET_ReactionAnim);
	}

	if (bSuccess && (reactionParams.flags & SReactionParams::OrientateToHitDir))
	{
		Ang3 angFacing;
		angFacing.Set(0.0f, 0.0f, cry_atan2f(m_pHitInfo->dir.x, -m_pHitInfo->dir.y) + reactionParams.orientationSnapAngle);
		angFacing.RangePI();

		IEntity *pEntity = m_actor.GetEntity();
		pEntity->SetRotation(Quat::CreateRotationXYZ( angFacing ));

		m_actor.GetAnimatedCharacter()->ForceAnimLocationToEntity();

#ifndef _RELEASE
		if (g_pGameCVars->g_hitDeathReactions_debug)
			CryLogAlways("Oriented to (%f, %f, %f)", angFacing.x, angFacing.y, angFacing.z);
#endif
	}

#ifndef _RELEASE
	// Send telemetry event
	if (bSuccess)
	{
		CStatsRecordingMgr* pRecordingMgr = g_pGame->GetStatsRecorder();
		IStatsTracker* pTracker = pRecordingMgr ? pRecordingMgr->GetStatsTracker(&m_actor) : NULL;
		if (pTracker)
		{
			EGameStatisticEvent eventType = IsInHitReaction() ? eGSE_HitReactionAnim : eGSE_DeathReactionAnim;

			if(pRecordingMgr->ShouldRecordEvent(eventType, &m_actor))
			{
				switch (m_currentExecutionType)
				{
					case eET_ReactionAnim: 
						pTracker->Event(eventType, m_currentCustomAnim.sAnimName.c_str()); 
						break;
					case eET_AnimationGraphAnim: 
						{
							// Obtain anim name. We are assuming the animation graph already transitioned to the new hit reaction state and
							// put its animation already on the queue, which could be a wild assumption
							ICharacterInstance* pMainChar = m_actor.GetEntity()->GetCharacter(0);
							CRY_ASSERT(pMainChar);
							if (pMainChar)
							{
								const CAnimation* pAnim = NULL;
								CAnimationPlayerProxy* pPlayerProxy = m_actor.GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
								if (pPlayerProxy)
								{
									pAnim = pPlayerProxy->GetTopAnimation(m_actor.GetEntity(), eAnimationGraphLayer_FullBody);
								}
								else
								{
									int iNumAnims = pMainChar->GetISkeletonAnim()->GetNumAnimsInFIFOAndDeferredQueue(eAnimationGraphLayer_FullBody);
									if (iNumAnims > 0)
										pAnim = &pMainChar->GetISkeletonAnim()->GetAnimFromFIFOAndDeferredQueue(eAnimationGraphLayer_FullBody, iNumAnims - 1);
								}
								CRY_ASSERT(pAnim);
								if (pAnim)
								{
									const CAnimation& anim = *pAnim;
									int32 topAnimId = (anim.m_Parametric.m_nParametricID >= 0) ? anim.m_Parametric.m_nParametricID : anim.m_Parametric.m_nAnimID[0];

									IAnimationSet* pAnimSet = pMainChar->GetIAnimationSet();
									const char* szAnimName = pAnimSet->GetNameByAnimID(topAnimId);
									pTracker->Event(eventType, string().Format("AG: %s", szAnimName).c_str()); 
								}
							}
						}
						break;
					case eET_Custom: 
						ReactionId reactionId = INVALID_REACTION_ID;
						reactionParams.reactionScriptTable->GetValue(REACTION_ID, reactionId);
						CRY_ASSERT(IsValidReactionId(reactionId));

						pTracker->Event(eventType, string().Format("Custom reaction (%d)", reactionId).c_str()); 
						break;
				}
			}
		}
	}
#endif

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::PausePipeUser(bool bPause)
{
	CRY_ASSERT(IsAI());

	// This function should not be called if our current paused state is currently executing a custom signal.
	//	The reason for this is the AI are right now creating custom behavior, and pausing the pipeuser will
	//	disable that custom behavior from executing properly. (Kevin)
	CRY_ASSERT_MESSAGE(eAIP_ExecutingCustomSignal != m_AIPausedState, "CHitDeathReactions::PausePipeUser Attempting to pause while the current pause state is 'eAIP_ExecutingCustomSignal'. This must not happen!");

	// Special handling for AI on server
	if (bPause != (eAIP_PipeUserPaused == m_AIPausedState))
	{
		IPipeUser* pPipeUser = GetAIPipeUser();
		if (pPipeUser && (!bPause || g_pGameCVars->g_hitDeathReactions_disable_ai))
		{
			pPipeUser->Pause(bPause);

			m_AIPausedState = bPause ? eAIP_PipeUserPaused : eAIP_NotPaused;
		}

/*
		if (pPipeUser)
		{
			if (bPause)
			{
				if (g_pGameCVars->g_hitDeathReactions_disable_ai)
					SetHitReactGoalPipe(pPipeUser);
			}
			else 
			{
				ClearHitReactGoalPipe(pPipeUser);
			}
		}
*/
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::CanAIEnterPoint(const Vec3 &vDest) const
{
	bool bResult = m_actor.IsPlayer();

	IAIObject *pAI = m_actor.GetEntity()->GetAI();
	IAIActor *pAIActor = CastToIAIActorSafe(pAI);
	if (!bResult && pAIActor)
	{
		INavigation *pNavigation = gEnv->pAISystem->GetINavigation();
		CRY_ASSERT(pNavigation);

		const AgentParameters &parameters = pAIActor->GetParameters();
		IPuppet *pPuppet = CastToIPuppetSafe(pAI);
		if (!pPuppet || pPuppet->IsPointInsideTerritoryShape(vDest, true))
		{
			// Check if too close to forbidden area
			if (!pNavigation->IsPointForbidden(vDest, parameters.m_fPassRadius))
			{
				// Check against forbidden boundaries
				const Vec3 &vPos = m_actor.GetEntity()->GetWorldPos();
				bResult = !pNavigation->IsPathForbidden(vPos, vDest);
			}
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::SetVariations(const SReactionParams::SAnimGraphReaction::VariationsContainer& variations) const
{
	IAnimationGraphState* pAnimationGraphState = m_actor.GetAnimationGraphState();
	if (!variations.empty() && pAnimationGraphState)
	{
		SReactionParams::SAnimGraphReaction::VariationsContainer::const_iterator itEnd = variations.end();
		for (SReactionParams::SAnimGraphReaction::VariationsContainer::const_iterator it = variations.begin(); it != itEnd; ++it)
		{
			pAnimationGraphState->SetVariationInput(it->sName, it->sValue);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::LoadData(SmartScriptTable pSelfTable, bool bForceReload)
{
	SmartScriptTable aux;
	if (m_pScriptSystem->GetGlobalValue(HIT_DEATH_REACTIONS_SCRIPT_TABLE, aux))
	{
		// This way we will have different instances of the HitDeathReactions global table sharing the same functions and
		// current elements (but in LUA we can do self.variable = 0; to create a "variable" field only in that instance)
		pSelfTable->Delegate(aux);

		// Set an entity field in the self table with the associated entity so we can easily access it from LUA
		SmartScriptTable pActorScriptTable = m_actor.GetEntity()->GetScriptTable();
		CRY_ASSERT(pActorScriptTable);
		pSelfTable->SetValue("entity", pActorScriptTable);

		g_pGame->GetHitDeathReactionsSystem().GetReactionParamsForActor(m_actor, m_pHitReactions, m_pDeathReactions, m_pCollisionReactions);
	}
	else
		CHitDeathReactionsSystem::Warning("Can't find HitDeathReactions global script table!");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::OnCustomAnimFinished()
{
	// Make sure it gets removed from the animation FIFO. Since we play the anims with repeat last key flag enabled, they
	// will remain there unless other system overwrites them (Animation Graph) or we clear them
	if ((m_currentCustomAnim.iLayer != eAnimationGraphLayer_FullBody) && (m_currentCustomAnim.iLayer != eAnimationGraphLayer_UpperBody))
	{
		CAnimationPlayerProxy* pPlayerProxy = m_actor.GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
		if (pPlayerProxy)
		{
			pPlayerProxy->StopAnimationInLayer(m_actor.GetEntity(), m_currentCustomAnim.iLayer, 0.2f);
		}
		else
		{
			ICharacterInstance* pMainChar = m_actor.GetEntity()->GetCharacter(0);
			CRY_ASSERT(pMainChar);
			if (pMainChar)
				pMainChar->GetISkeletonAnim()->StopAnimationInLayer(m_currentCustomAnim.iLayer, 0.2f);
		}
	}

	// If anim is finished, resume animation graph (if is unpaused it won't do anything)
	IAnimationGraphState* pGraph = m_actor.GetAnimationGraphState();
	if (pGraph)
	{
		pGraph->Pause(false, eAGP_PlayAnimationNode);
	}

	// Re-enable lookIk
	m_actor.GetAnimatedCharacter()->AllowLookIk(true);

	CRY_ASSERT(IsExecutionType(eET_ReactionAnim));
	SetExecutionType(eET_None);

	// [*DavidR | 17/Feb/2010] ToDo: Is this really what we want? we could allow
	// Custom executions to trigger several animations inside the same reactions. This will
	// forbid it
	// End current reaction
	EndCurrentReaction();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::DoCollisionCheck()
{
	// [*DavidR | 25/Feb/2010] ToDo: All the magic numbers in this method should be converted to constants or parameters
	// [*DavidR | 25/Feb/2010] ToDo: These need to be specified in the data file. These values make sense for humans, but
	// not necessarily for different types of aliens
	const float RAGDOLL_TEST_RADIUS = 0.6f;
	const float RAGDOLL_TEST_GROUND_OFFSET = 0.5f;

	ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(0);
	CRY_ASSERT(pCharacter);
	if (!pCharacter)
		return;

	// [*DavidR | 25/Feb/2010] WIP/ToDo: The Bone used as reference needs to be specified in the data file too, it changes
	// per character (although is currently shared between humans, grunts and stalkers)
	int spineID = m_actor.GetBoneID(BONE_SPINE);
	if (spineID == -1)
	{
		spineID = pCharacter->GetISkeletonPose()->GetJointIDByName("Spine01");
	}

	Vec3 vRootBone = pCharacter->GetISkeletonPose()->GetAbsJointByID(spineID).t; 
	vRootBone.z=0;

	primitives::sphere sphPrim;
	sphPrim.center = m_actor.GetEntity()->GetWorldTM() * vRootBone;
	sphPrim.center.z += RAGDOLL_TEST_RADIUS + RAGDOLL_TEST_GROUND_OFFSET;
	sphPrim.r = RAGDOLL_TEST_RADIUS;

	int collisionEntityTypes = ent_static | ent_terrain | ent_ignore_noncolliding;
	m_raycastHelper->CastPrimitive(sphPrim.type, &sphPrim, Vec3(ZERO), collisionEntityTypes, geom_colltype0);

#ifndef _RELEASE
	if (g_pGameCVars->g_hitDeathReactions_debug)
	{
		ColorB col(0xff, 0, 0, 0x44);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(sphPrim.center, sphPrim.r, col);
	}
#endif 
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::StartCollisionReaction(const Vec3& vNormal, const Vec3& vPoint)
{
	if (IsInHitReaction())
	{
		float fAdditionalAngle = 0.0f;

		// Position the entity at the right distance
		IEntity* pEntity = m_actor.GetEntity();
		const Vec3& vPos = pEntity->GetWorldPos();
		Vec3 vPlainNormal(vNormal.x, vNormal.y, 0.0f);
		vPlainNormal.NormalizeSafe(Vec3Constants<float>::fVec3_OneY);
		Vec3 vStartPos = vPoint - (vPlainNormal * COLLISION_REACTION_DISTANCE_OFFSET);
		vStartPos.z = vPos.z;
		pEntity->SetPos(vStartPos);

		m_actor.GetAnimatedCharacter()->ForceAnimLocationToEntity();

		// Play a collision reaction, this will interrupt the previous one. 
		CRY_ASSERT(m_pHitInfo);
		CRY_ASSERT(m_reactionOnCollision != NO_COLLISION_REACTION);

		if (m_reactionOnCollision <= m_pCollisionReactions->size())
		{
			if (IsAI())
			{
				HandleAIHitReactionInterrupted();
			}

			// Use a fake hitInfo for the initial execution
			const HitInfo* pRealHitInfo = m_pHitInfo;

			HitInfo fakeHitInfo(*m_pHitInfo);
			fakeHitInfo.pos = vPoint;
			fakeHitInfo.dir = -vNormal;
			StartHitReaction(fakeHitInfo, m_pCollisionReactions->at(m_reactionOnCollision - 1));

			// restore the real hit info (shouldn't be accessed within a collision reaction, but just in case)
			m_pHitInfo = pRealHitInfo;
		}
		else
		{
			// Just end the reaction (if no collision reaction and "1" is specified we allow it as "end reaction when collision happens")
			if (m_reactionOnCollision != 1)
				CHitDeathReactionsSystem::Warning("Invalid collision reaction index %d. Check HitDeathReactions data file", m_reactionOnCollision);

			EndCurrentReaction();
		}
	}
	else
		EndCurrentReaction();
}

//////////////////////////////////////////////////////////////////////////
/// GetRelativeCardinalDirection returns which 90º cone (for forward, back,
/// left and right) the direction vector vDir2 is pointing to compared to
/// direction vector vDir1
//////////////////////////////////////////////////////////////////////////
ECardinalDirection CHitDeathReactions::GetRelativeCardinalDirection2D(const Vec2& vDir1, const Vec2& vDir2) const
{
	float fDotForward = vDir1.Dot(vDir2);
	float fDotLeft = vDir1.Cross(vDir2); // the same as: vDir1.Perp().Dot(vDir2);

	if (cry_fabsf(fDotForward) > cry_fabsf(fDotLeft))
	{
		if (fDotForward > 0.0f)
			return eCD_Forward;
		else
			return eCD_Back;
	}
	else
	{
		if (fDotLeft > 0.0f)
			return eCD_Left;
		else
			return eCD_Right;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::CheckCardinalDirection2D(ECardinalDirection direction, const Vec2& vDir1, const Vec2& vDir2) const
{
	bool bCheck = false;

	switch(direction)
	{
	case eCD_Ahead:
		{
			float fDot = vDir1.Dot(vDir2);
			bCheck = (fDot >= 0);
		} break;
	case eCD_Behind:
		{
			float fDot = vDir1.Dot(vDir2);
			bCheck = (fDot < 0);
		} break;

	case eCD_RightSide:
		{
			float fCross = vDir1.Cross(vDir2);
			bCheck = (fCross < 0);
		} break;
	case eCD_LeftSide:
		{
			float fCross = vDir1.Cross(vDir2);
			bCheck = (fCross >= 0);
		} break;

	default:
		bCheck = (direction == GetRelativeCardinalDirection2D(vDir1, vDir2));
	}

	return bCheck;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
uint32 CHitDeathReactions::GetSynchedSeed(bool bKillReaction) const
{
	// [*DavidR | 9/Dec/2009] This method returns a seed that must be the same
	// between calls for the same OnHit/OnKill event between side and server, 
	// but different between successive calls

	// Calculate it using the name of the target + current health
	// [*DavidR | 9/Dec/2009] ToDo: Try to find a better way to have a synced seed
	// [*DavidR | 9/Dec/2009] FixMe: This means the exact same actor is always going to use the same seed (which is a problem
	// if the actor respawns/revive with the same entity name)
	return bKillReaction ? 
		gEnv->pSystem->GetCrc32Gen()->GetCRC32(m_actor.GetEntity()->GetName()) :
	gEnv->pSystem->GetCrc32Gen()->GetCRC32(m_actor.GetEntity()->GetName()) + static_cast<uint32>(m_actor.GetHealth() * 100.f);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
float CHitDeathReactions::GetRandomProbability() const
{
	return m_pseudoRandom.GenerateFloat();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::CustomAnimHasFinished() const
{
	return IsExecutionType(eET_ReactionAnim) && !IsPlayingReactionAnim();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::MakePreviousAnimInterruptible(int iSlot, int iLayer)
{
	ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(iSlot);
	ISkeletonAnim* pISkeletonAnim = pCharacter ? pCharacter->GetISkeletonAnim() : NULL;
	const int iNumAnims = pISkeletonAnim ? pISkeletonAnim->GetNumAnimsInFIFOAndDeferredQueue(iLayer) : 0;
	if (iNumAnims > 0)
	{
		// These flags can delay the transition, remove them before invoking the new animation. 
		// Should be in synch with CSkeletonAnim::EvaluateTransitionFlags logic
		CAnimation& anim = pISkeletonAnim->GetAnimFromFIFOAndDeferredQueue(iLayer, iNumAnims - 1);
		anim.m_AnimParams.m_nFlags &= ~(CA_START_AT_KEYTIME | CA_START_AFTER | CA_IDLE2MOVE | CA_MOVE2IDLE);
	}

	// [*DavidR | 25/Feb/2010] ToDo: If the anim is partial / additive CA_DISABLE_MULTILAYER in previous 
	// layers is stopping the additive/partial anims to play. Try to fix it
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::StopHigherLayers(int iSlot, int iLayer, float fBlendOut)
{
	if (iLayer < (numVIRTUALLAYERS - 1))
	{
		ICharacterInstance* pCharacter = m_actor.GetEntity()->GetCharacter(iSlot);
		ISkeletonAnim* pISkeletonAnim = pCharacter ? pCharacter->GetISkeletonAnim() : NULL;
		if (pISkeletonAnim)
		{
			for (int i = iLayer + 1; i < numVIRTUALLAYERS; ++i)
			{
				pISkeletonAnim->StopAnimationInLayer(i, fBlendOut);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactions::DrawDebugInfo()
{
	float fFontSize = 1.2f;
	float standbyColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float reactingColor[4] = {Col_GreenYellow.r, Col_GreenYellow.g, Col_GreenYellow.b, Col_GreenYellow.a};
	float* drawColor = standbyColor;

	string sMsg(string().Format("Hit/Death reactions system is [%s]\nNumber of Hit reactions: %u. Number of Death reactions %u",
		g_pGameCVars->g_hitDeathReactions_enable ? " enabled" : "disabled", m_pHitReactions->size(), m_pDeathReactions->size()));

	Vec3 vDrawPos = m_actor.GetEntity()->GetWorldPos() + m_actor.GetLocalEyePos();
	if (g_pGameCVars->g_hitDeathReactions_enable)
	{
		string sMsg2("\n\n\n\n");

		sMsg += string().Format("\nCan trigger Hit reactions? [%s]\nCan trigger Death reactions? [%s]", CanPlayHitReaction() ? "YES" : " NO", CanPlayDeathReaction() ? "YES" : " NO");		
		if (IsInReaction())
		{
			drawColor = reactingColor;

			sMsg += string().Format("\nExecuting %s reaction (%f)", IsInHitReaction() ? "Hit" : "Death", m_fReactionCounter);

			// The following lines will most probably make the sMsg grow bigger than 256, the current hardcoded limit for DrawLabel,
			// so let's use a different string that will be printed on a different DrawLabel call
			if (IsExecutionType(eET_ReactionAnim))
			{
				sMsg2 += string().Format("\nExecuting Reaction anim: %s", m_currentCustomAnim.sAnimName.c_str());
			}
			else if (IsExecutionType(eET_AnimationGraphAnim))
			{
				sMsg2 += string().Format("\nExecuting Animation graph-based reaction");
			}
			else if (IsExecutionType(eET_Custom))
			{
				sMsg2 += string().Format("\nExecuting custom LUA reaction code");
			}
			else
			{
				sMsg2 += string().Format("\nTHIS SHOULDN'T HAPPEN. WHAT ARE WE EXECUTING?");
			}
		}

		sMsg2 += string().Format("\n Pos (%.2f, %.2f, %.2f) RotZ (%f) %s", m_actor.GetEntity()->GetPos().x, m_actor.GetEntity()->GetPos().y, m_actor.GetEntity()->GetPos().z, m_actor.GetEntity()->GetRotation().GetRotZ(), m_actor.GetActorStats()->isRagDoll ? "RAGDOLL" : "");		
		gEnv->pRenderer->DrawLabelEx(vDrawPos, fFontSize, drawColor, true, false, sMsg2.c_str());
	}

	gEnv->pRenderer->DrawLabelEx(vDrawPos, fFontSize, drawColor, true, false, sMsg.c_str());

	if ((g_pGameCVars->g_hitDeathReactions_debug > 1) && IsInDeathReaction())
	{
		//--- Draw a ragdoll indicator to aid animators to visualise the transitions
		IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		ColorB colour = (m_fReactionEndTime >= 0.0f) ? ColorB(0,0,255,255) : ColorB(255, 0, 0, 255);
		pAuxGeom->DrawSphere(m_actor.GetEntity()->GetWorldPos() + m_actor.GetLocalEyePos()+Vec3(0.0f, 0.0f, 0.5f), 0.25f, colour);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactions::IsValidReactionId(ReactionId reactionId) const
{
	// A reaction id is invalid if any of the following is true:
	//* reactionId is the invalid reaction id constant
	//* reactionId is greater than the size of the reaction params container is associated to
	//* The associated reaction params container is empty
	if (reactionId == INVALID_REACTION_ID)
		return false;

	ReactionId absReactionId = abs(reactionId);

	ReactionsContainerConstPtr pReactionList;
	if (reactionId < 0)
	{
		int deathReactionsSize = static_cast<int>(m_pDeathReactions->size());
		if (absReactionId > deathReactionsSize)
		{
			pReactionList = m_pCollisionReactions;
			absReactionId -= deathReactionsSize;
		}
		else
			pReactionList = m_pDeathReactions;
	}
	else
		pReactionList = m_pHitReactions;

	return ((absReactionId <= pReactionList->size()) && 
					(!pReactionList->empty()));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
const SReactionParams& CHitDeathReactions::GetReactionParamsById(ReactionId reactionId) const
{
	CRY_ASSERT_MESSAGE(IsValidReactionId(reactionId), "this method shouldn't be called if IsValidReactionId is false");

	ReactionId absReactionId = abs(reactionId);

	ReactionsContainerConstPtr pReactionList;
	if (reactionId < 0)
	{
		int deathReactionsSize = static_cast<int>(m_pDeathReactions->size());
		if (absReactionId > deathReactionsSize)
		{
			pReactionList = m_pCollisionReactions;
			absReactionId -= deathReactionsSize;
		}
		else
			pReactionList = m_pDeathReactions;
	}
	else
		pReactionList = m_pHitReactions;

	return pReactionList->at(absReactionId - 1);
}

void CHitDeathReactions::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
//	pSizer->AddObject( m_pHitInfo ); //Diesel cut
	pSizer->AddObject( m_pDeathReactions );
	pSizer->AddObject( m_pHitReactions );
	pSizer->AddObject( m_pCollisionReactions );
}