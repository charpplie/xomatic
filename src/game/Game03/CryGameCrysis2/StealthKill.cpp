/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
	Part of Player that controls stealth killing.

-------------------------------------------------------------------------
History:
- 15:10:2009: Created by Sven Van Soom

*************************************************************************/
#include "StdAfx.h"
#include "Player.h"
#include "StealthKill.h"
#include "GameRules.h"
#include "ICooperativeAnimationManager.h"
#include "Game.h"
#include "AutoAimManager.h"

CStealthKill::TStealthKillParams CStealthKill::s_stealthKillParams;
bool CStealthKill::s_dataLoaded = false;

void CStealthKill::Init(CPlayer* pPlayer)
{
	CRY_ASSERT(m_pPlayer == NULL);
	if (m_pPlayer != NULL)
		return;

	m_pPlayer = pPlayer;
}

//-----------------------------------------------------------------------
bool CStealthKill::CanExecuteOn(CActor* pTarget) const
{
	IEntity* pTargetEntity = pTarget->GetEntity();
	IEntity* pPlayerEntity = m_pPlayer->GetEntity();

	const SAutoaimTarget* pTargetInfo = g_pGame->GetAutoAimManager().GetTargetInfo(pTargetEntity->GetId());

	bool isValidTarget = pTargetInfo && pTargetInfo->HasFlagsSet((int8)(eAATF_AIHostile|eAATF_StealthKillable)) && CanExecuteOnEnemyClass(pTargetEntity->GetClass());
	if (isValidTarget)
	{
		// this player needs to be within a certain angle and distance behind the target in 2D
		const float behindTargetAngleDegrees = 70.0f;
		const float maxDistance = 1.4f;

		Vec2 vPlayerToTarget = Vec2(pPlayerEntity->GetWorldPos()) - Vec2(pTargetEntity->GetWorldPos());
		float distSquared = vPlayerToTarget.GetLength2();
		if (distSquared >= maxDistance*maxDistance)
			return false;

		float cosineBehindTargetHalfAngleRadians = cos(DEG2RAD(behindTargetAngleDegrees/2.0f));
		Vec2 vBehindTargetDir = -Vec2(pTarget->GetAnimatedCharacter()->GetAnimLocation().GetColumn1()).GetNormalizedSafe(); // In decoupled catchup mode we need the animated character's orientation
		if (vPlayerToTarget.GetNormalizedSafe() * vBehindTargetDir <= cosineBehindTargetHalfAngleRadians)
			return false;
		
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------
void CStealthKill::Update()
{
	if (!m_isBusy)
		return;

	ICooperativeAnimationManager* pCooperativeAnimationManager = gEnv->pGame->GetIGameFramework()->GetICooperativeAnimationManager();
	if (!pCooperativeAnimationManager->IsActorBusy(m_pPlayer->GetEntityId()) &&
		!pCooperativeAnimationManager->IsActorBusy(m_pTarget->GetEntityId()))
	{
		Leave();
	}
}



//-----------------------------------------------------------------------
void CStealthKill::DeathBlow()
{
	CRY_ASSERT_MESSAGE(m_isBusy, "Stealth kill should be in progress when triggering the death blow");
	if (!m_isBusy)
		return;

	if (m_isDeathBlowDone)
		return;

	ICooperativeAnimationManager* pCooperativeAnimationManager = gEnv->pGame->GetIGameFramework()->GetICooperativeAnimationManager();
	pCooperativeAnimationManager->StopCooperativeAnimationOnActor(m_pTarget->GetAnimatedCharacter());

	CGameRules *pGameRules = g_pGame->GetGameRules();

	HitInfo hitInfo;
	hitInfo.shooterId = m_pPlayer->GetEntityId();
	hitInfo.targetId = m_pTarget->GetEntityId();
	hitInfo.damage = 999999999.0f; // CERTAIN_DEATH
	hitInfo.dir = m_pTarget->GetEntity()->GetForwardDir();
	hitInfo.normal = -hitInfo.dir; // this has to be in an opposite direction from the hitInfo.dir or the hit is ignored as a 'backface' hit
	hitInfo.type = g_pGame->GetGameRules()->GetHitTypeId("silentMelee"); // assuming 'silentMelee' is a good type for a stealth kill
	g_pGame->GetGameRules()->ClientHit(hitInfo);

	// WARNING: RagDollize resets the entity's rotation!
	m_pTarget->RagDollize(false);

	// Give a small nudge in the hit direction to make the target fall over
	const SParams* pStealthKillParams = GetParamsForClass(m_pTarget->GetEntity()->GetClass());
	
	if (pStealthKillParams)
	{
		ICharacterInstance* pCharacter = m_pTarget->GetEntity()->GetCharacter(0);
		int boneId = pCharacter ? pCharacter->GetISkeletonPose()->GetJointIDByName("Spine04") : -1;
		if (boneId != -1)
		{
			const float stealthKillDeathBlowVelocity = pStealthKillParams->impulseScale; // desired velocity after impulse in meters per second

			IPhysicalEntity* pTargetPhysics = m_pTarget->GetEntity()->GetPhysics();
			if (pTargetPhysics)
			{
				pe_simulation_params simulationParams;
				pTargetPhysics->GetParams(&simulationParams);

				pe_action_impulse impulse;
				impulse.partid = boneId;
				impulse.impulse = hitInfo.dir*stealthKillDeathBlowVelocity*simulationParams.mass; // RagDollize reset the entity's rotation so I have to use the value I cached earlier
				pTargetPhysics->Action(&impulse);
			}
		}
	}
#ifdef STEALTH_KILL_DEBUG
	else
	{
		GameWarning("StealthKill (DeathBlow): params for class '%s' not found! This code shouldn't have executed.", m_pTarget->GetEntity()->GetClass()->GetName());
	}
#endif

	m_isDeathBlowDone = true;
}

//-----------------------------------------------------------------------
void CStealthKill::Leave()
{
	CRY_ASSERT_MESSAGE(m_isBusy, "Stealth kill cannot be stopped if it is not in progress");
	if (!m_isBusy)
		return;

	ICooperativeAnimationManager* pCooperativeAnimationManager = gEnv->pGame->GetIGameFramework()->GetICooperativeAnimationManager();

	m_pPlayer->HolsterItem(false);

	pCooperativeAnimationManager->StopCooperativeAnimationOnActor(m_pPlayer->GetAnimatedCharacter(), m_pTarget->GetAnimatedCharacter());

	// Enable AI on the target again (for what it's worth - this helps editor)
	if (!m_pTarget->IsPlayer() && m_pTarget->GetEntity()->GetAI())
		m_pTarget->GetEntity()->GetAI()->Event(AIEVENT_ENABLE, 0);

	DeathBlow(); // Call this in case the notification from the animation system got skipped or missed for some reason

	// Reset State
	m_isBusy = false;
	m_pTarget = NULL;
	m_isDeathBlowDone = false;

	// Update visibility to change render mode of 1st person character
	m_pPlayer->UpdateVisibility();

	SetStatsViewMode(false);

	if (CNanoSuit* pNanoSuit = m_pPlayer->GetNanoSuit())
	{
		pNanoSuit->ShowKnife(false);
	}

}


//-----------------------------------------------------------------------
void CStealthKill::Enter(int targetEntityId)
{
	CRY_ASSERT_MESSAGE(!m_isBusy, "Stealth kill should not be initiated while a stealth kill is already in progress");
	if (m_isBusy)
		return;

	CActor *pTargetActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetEntityId));
	CRY_ASSERT_MESSAGE(pTargetActor != NULL, "Stealth kill should act upon an actor");
	if (pTargetActor == NULL)
		return;

	ICooperativeAnimationManager* pCooperativeAnimationManager = gEnv->pGame->GetIGameFramework()->GetICooperativeAnimationManager();
	if (pCooperativeAnimationManager->IsActorBusy(m_pPlayer->GetEntityId()) ||
		pCooperativeAnimationManager->IsActorBusy(targetEntityId))
		return;

	const SParams* pStealthKillParams = GetParamsForClass(pTargetActor->GetEntity()->GetClass());
	if (pStealthKillParams)
	{
		// Disable AI on the target
		if (!pTargetActor->IsPlayer() && pTargetActor->GetEntity()->GetAI())
			pTargetActor->GetEntity()->GetAI()->Event(AIEVENT_DISABLE, 0);

		m_pPlayer->HolsterItem(true);

		const float slideTime = 0.2f;
		SCharacterParams killerParams(m_pPlayer->GetAnimatedCharacter(), pStealthKillParams->playerAnimation.c_str(), /*allowHPhysics*/false, slideTime);
		SCharacterParams targetParams(pTargetActor->GetAnimatedCharacter(), pStealthKillParams->enemyAnimation.c_str(), /*allowHPhysics*/false, slideTime);
		SCooperativeAnimParams animParams(/*forceStart*/true, /*looping*/ false, /*alignment*/ /*eAF_FirstActorNoRot*/eAF_FirstActor);
		animParams.bIgnoreCharacterDeath = true;
		animParams.bPreventFallingThroughTerrain = false;

		CRY_FIXME(8, 2, 2010, "Enable 'new' co-op animation manager by default, but not this way!");
		// Enable the new experimental part of the coop animation system that doesn't use the animation graph anymore
		gEnv->pConsole->GetCVar( "co_usenewcoopanimsystem" )->Set(1);

		bool bStarted = pCooperativeAnimationManager->StartNewCooperativeAnimation(targetParams, killerParams, animParams);
		CRY_ASSERT_MESSAGE(bStarted, "Could not start cooperative animation");
		if (!bStarted)
			return;

		m_pTarget = pTargetActor;
		m_isBusy = true;

		// Update visibility to change render mode of 1st person character
		m_pPlayer->UpdateVisibility();

		// Inform suit about the action
		SNanoSuitEvent suitEvent;
		suitEvent.event = eNanoSuitEvent_STEALTH_KILL;
		m_pPlayer->SendActorSuitEvent(suitEvent);

		SetStatsViewMode(true);

		if (CNanoSuit* pNanoSuit = m_pPlayer->GetNanoSuit())
		{
			pNanoSuit->ShowKnife(true);
		}
	}
#ifdef STEALTH_KILL_DEBUG
	else
	{
		GameWarning("StealthKill (Enter): params for class '%s' not found! This code shouldn't have executed.", pTargetActor->GetEntity()->GetClass()->GetName());
	}
#endif
}


bool CStealthKill::AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC)
{
	if (!m_isBusy)
		return false;

	if (pCharacter == m_pPlayer->GetEntity()->GetCharacter(0))
	{
		if (m_pPlayer->GetAnimationEventsTable().m_stealthKillId == eventNameCRC)
		{
			DeathBlow();
			return true;
		}
	}

	return false;
}

const CStealthKill::SParams* CStealthKill::GetParamsForClass( IEntityClass* pTargetClass )
{
	CRY_ASSERT(pTargetClass);

	const int paramsCount = s_stealthKillParams.size();
	for (int i = 0; i < paramsCount; ++i)
	{
		if (pTargetClass == s_stealthKillParams[i].pEnemyClass)
		{
			return &(s_stealthKillParams[i]);
		}
	}

	return NULL;
}

bool CStealthKill::CanExecuteOnEnemyClass( IEntityClass* pTargetClass )
{
	CRY_ASSERT(pTargetClass);

	const int paramsCount = s_stealthKillParams.size();
	for (int i = 0; i < paramsCount; ++i)
	{
		if (pTargetClass == s_stealthKillParams[i].pEnemyClass)
		{
			return true;
		}
	}

	return false;
}

void CStealthKill::ReadXmlData( const IItemParamsNode* pRootNode, bool reloading /*= false*/ )
{
	if (!reloading && s_dataLoaded)
	{
		return;
	}

	const IItemParamsNode* pParams = pRootNode->GetChild("StealthKill");
	if (!pParams)
		return;

	s_dataLoaded = true;

	const int childCount = pParams->GetChildCount();
	s_stealthKillParams.clear();
	s_stealthKillParams.reserve(childCount);

	for (int i = 0; i < childCount; ++i)
	{
		const IItemParamsNode* pTargetParams = pParams->GetChild(i);
		CRY_ASSERT(pTargetParams);

		IEntityClass* pTargetClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pTargetParams->GetName());
		if (pTargetClass)
		{
			SParams targetParams;
			targetParams.pEnemyClass = pTargetClass;
			targetParams.playerAnimation = pTargetParams->GetAttributeSafe("playerAnimation");
			targetParams.enemyAnimation = pTargetParams->GetAttributeSafe("enemyAnimation");
			targetParams.impulseBone = pTargetParams->GetAttributeSafe("impulseBone");
			pTargetParams->GetAttribute("impulseScale", targetParams.impulseScale);

			s_stealthKillParams.push_back(targetParams);
		}
#ifdef STEALTH_KILL_DEBUG
		else
		{
			GameWarning("Stealth Kill: Couldn't find entity of class '%s', skipping", pTargetParams->GetName());
		}
#endif
	}
}

void CStealthKill::SetStatsViewMode( bool followCameraBone )
{
	SPlayerStats* pStats = static_cast<SPlayerStats*>(m_pPlayer->GetActorStats());
	if (pStats)
	{
		pStats->followCharacterHead = followCameraBone;
	}
}
