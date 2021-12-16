/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 23:3:2006   13:05 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Melee.h"
#include "Game.h"
#include "Item.h"
#include "Weapon.h"
#include "GameRules.h"
#include "Player.h"
#include "IVehicleSystem.h"
#include <IEntitySystem.h>
#include "IMaterialEffects.h"
#include "GameCVars.h"
#include "WeaponSharedParams.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "IRenderer.h"
#include "IRenderAuxGeom.h"	
#include "ScreenEffects.h"
#include "AutoAimManager.h"
#include "ITargetTrackManager.h"

#if 0 && (defined(USER_claire) || defined(USER_tombe) || defined(USER_johnmichael)) 
#define MeleeDebugLog(...)				CryLogAlways("[MELEE DEBUG] " __VA_ARGS__)
#else
#define MeleeDebugLog(...)				(void)(0)
#endif

EntityId CMelee::s_meleeSnapTargetId = 0;

//------------------------------------------------------------------------
CMelee::CMelee()
{
	m_collisionHelper.SetUser(this);
}

//------------------------------------------------------------------------
CMelee::~CMelee()
{
	m_fireParams = 0;
}

void CMelee::InitFireMode(IWeapon *pWeapon, const SParentFireModeParams* pParams, uint32 id)
{
	BaseClass::InitFireMode(pWeapon, pParams, id);

	m_attacking = false;
	m_attacked = false;
	m_netAttacking = false;
	m_delayTimer=0.0f;
}

//------------------------------------------------------------------------
void CMelee::Update(float frameTime, uint32 frameId)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool remote = false;
	bool doMelee = false;
	bool requireUpdate = false;

	if (m_attacking)
	{
		MeleeDebugLog ("CMelee<%p> Update while attacking: m_attacked=%d, delay=%f", this, m_attacked, m_delayTimer);

		requireUpdate = true;
		if (m_delayTimer>0.0f)
		{
			RequestAlignmentToNearestTarget();
			m_delayTimer-=frameTime;
			if (m_delayTimer<=0.0f)
				m_delayTimer=0.0f;
		}
		else if (m_netAttacking)
		{
			remote = true;
			doMelee = true;
			m_attacking = false;
			m_netAttacking = false;

			m_pWeapon->SetBusy(false);
		}	
		else if (!m_attacked)
		{
			doMelee = true;
			m_attacked = true;
		}


		if ( !m_collisionHelper.IsBlocked() && doMelee)
		{
			if (CActor *pActor = m_pWeapon->GetOwnerActor())
			{
				if (IMovementController * pMC = pActor->GetMovementController())
				{
					SMovementState info;
					pMC->GetMovementState(info);

					Vec3  pos (info.eyePosition);
					if ((pos.z - info.pos.z) > g_pGameCVars->pl_melee.raycast_max_height)
					{
						pos.z = (info.pos.z + g_pGameCVars->pl_melee.raycast_max_height);
					}

					PerformMelee(pos, info.eyeDirection, remote);

					if (pActor->IsClient())
					{
						s_meleeSnapTargetId = 0;
					}
				}
			}
		}
	}

	if (requireUpdate)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CMelee::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CMelee::Activate(bool activate)
{
	MeleeDebugLog ("CMelee<%p> Activate(%s)", this, activate ? "true" : "false");

	m_attacking = false;
	m_delayTimer = 0.0f;
}

//------------------------------------------------------------------------
bool CMelee::CanFire(bool considerAmmo) const
{
	CActor* pOwnerActor = m_pWeapon->GetOwnerActor();
	return !m_attacking && (!pOwnerActor || !pOwnerActor->IsStandingUp());
}

//------------------------------------------------------------------------
struct CMelee::StopAttackingAction
{
	CMelee *_this;
	StopAttackingAction(CMelee *melee): _this(melee) {};
	void execute(CItem *pItem)
	{
		_this->m_attacking = false;

		_this->m_delayTimer = 0.0f;
		pItem->SetBusy(false);
		pItem->ForcePendingActions();
		MeleeDebugLog ("CMelee<%p> StopAttackingAction is being executed!", _this);
	}
};

void CMelee::StartFire()
{
	bool canFire = CanFire();

	MeleeDebugLog ("CMelee<%p> StartFire canFire=%s", this, canFire ? "true" : "false");

	if (!canFire)
		return;

	CActor* pOwner = m_pWeapon->GetOwnerActor();

	m_attacking = true;
	m_attacked = false;
	m_pWeapon->RequireUpdate(eIUS_FireMode);
	m_pWeapon->ExitZoom();

	bool isClient = pOwner ? pOwner->IsClient() : false;
	bool superJumpAttack = pOwner ? (pOwner->IsSuperJumping() && !m_fireParams->actions.attack_superJump.empty()) : false;
	bool superPowerAttack = pOwner ? pOwner->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power && pOwner->GetActorSuitGameParameters().IsSuitPowerActive() : false;
	const char* meleeAction = m_fireParams->actions.attack.c_str();
	if (superJumpAttack)
	{
		meleeAction = m_fireParams->actions.attack_superJump.c_str();
	}
	else if (superPowerAttack && !m_fireParams->actions.attack_superPower.empty())
	{
		meleeAction = m_fireParams->actions.attack_superPower.c_str();
	}

	m_pWeapon->PlayAction(meleeAction, 
		0, false, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending|CItem::eIPAF_RestartAnimation|CItem::eIPAF_EndCurrentWeaponAnim);
	m_pWeapon->SetBusy(true);
	m_delayTimer = m_fireParams->meleeparams.delay;

	// Set up a timer to disable the melee attack at the end of the first-person animation...
	uint32 durationInMilliseconds = static_cast<uint32>(m_fireParams->meleeparams.duration * 1000.f);
	MeleeDebugLog ("CMelee<%p> Setting a timer to trigger StopAttackingAction (duration=%u)", this, durationInMilliseconds);

	// How much longer we need the animation to be than the damage delay, in seconds...
	const float k_requireAdditionalTime = 0.1f;

	if (durationInMilliseconds < (m_delayTimer + k_requireAdditionalTime) * 1000.0f)
	{
		if (!GetISystem()->IsDedicated())
		{
			CRY_ASSERT_MESSAGE(false, string().Format("CMelee<%p> Warning! Melee attack timeout (%f seconds) needs to be substantially longer than the damage delay (%f seconds)! Increasing...", this, durationInMilliseconds / 1000.f, m_delayTimer));
		}
		durationInMilliseconds = (uint32) ((m_delayTimer + k_requireAdditionalTime) * 1000.0f + 0.5f);	// Add 0.5f to round up when turning into a uint32
	}
	m_pWeapon->GetScheduler()->TimerAction(durationInMilliseconds, CSchedulerAction<StopAttackingAction>::Create(this), true);

	m_pWeapon->OnMelee(m_pWeapon->GetOwnerId());

	m_pWeapon->RequestStartMeleeAttack(m_pWeapon->GetMeleeFireMode()==this);

	if (isClient)
	{
		s_meleeSnapTargetId = GetNearestTarget();

		SNanoSuitEvent event;
		event.event = eNanoSuitEvent_MELEE;
		pOwner->SendActorSuitEvent(event);

		CCCPOINT(Melee_LocalActorMelee);
	}
	else
	{
		CCCPOINT(Melee_NonLocalActorMelee);
	}
}

//------------------------------------------------------------------------
void CMelee::StopFire()
{
	MeleeDebugLog ("CMelee<%p> StopFire", this);
}

//------------------------------------------------------------------------
void CMelee::NetStartFire()
{
	MeleeDebugLog ("CMelee<%p> NetStartFire", this);

	m_delayTimer =  m_fireParams->meleeparams.delay;
	m_attacking = true;
	m_netAttacking = true;
	
	m_pWeapon->OnMelee(m_pWeapon->GetOwnerId());
	m_pWeapon->PlayAction(m_fireParams->actions.attack, 0, false, CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_EndCurrentWeaponAnim);
	m_pWeapon->SetBusy(true);
	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CMelee::NetStopFire()
{
	MeleeDebugLog ("CMelee<%p> NetStopFire", this);
}

//------------------------------------------------------------------------
void CMelee::NetShoot(const Vec3 &hit, int ph)
{
	MeleeDebugLog ("CMelee<%p> NetShoot", this);
}

//------------------------------------------------------------------------
void CMelee::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph)
{
	MeleeDebugLog ("CMelee<%p> NetShootEx", this);
}

//------------------------------------------------------------------------
const char *CMelee::GetType() const
{
	return "Melee";
}

//------------------------------------------------------------------------
int CMelee::GetDamage() const
{
	return m_fireParams->meleeparams.damage;
}

//-----------------------------------------------------------------------
void CMelee::PerformMelee(const Vec3 &pos, const Vec3 &dir, bool remote)
{
	MeleeDebugLog ("CMelee<%p> PerformMelee(remote=%s)", this, remote ? "true" : "false");

#if !defined(_RELEASE)
	if(g_pGameCVars->pl_melee.debug_gfx)
	{
		IPersistantDebug  *pDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
		pDebug->Begin("CMelee::PerformMelee", false);
		pDebug->AddLine(pos, (pos + dir), ColorF(1.f,0.f,0.f,1.f), 15.f);
	}
#endif

	m_collisionHelper.DoCollisionTest(SCollisionTestParams(pos, dir, m_fireParams->meleeparams.range, m_pWeapon->GetOwnerId(), 0, remote));
}

//------------------------------------------------------------------------
bool CMelee::PerformCylinderTest(const Vec3 &pos, const Vec3 &dir, bool remote)
{
	MeleeDebugLog ("CMelee<%p> PerformCylinderTest(remote=%s)", this, remote ? "true" : "false");

	CRY_FIXME(4, 21, 2009, "filipe - simplify and move to engine facade. cylinder testing is common.");

	IEntity *pOwner = m_pWeapon->GetOwner();
	IPhysicalEntity *pIgnore = pOwner?pOwner->GetPhysics():0;
	IEntity *pHeldObject = NULL;

	primitives::cylinder cyl;
	cyl.r = 0.25f;
	cyl.axis = dir;
	cyl.hh = m_fireParams->meleeparams.range/2.0f;
	cyl.center = pos + dir.normalized()*cyl.hh;

	float n = 0.0f;
	geom_contact *contacts;
	intersection_params params;
	params.bStopAtFirstTri = false;
	params.bNoBorder = true;
	params.bNoAreaContacts = true;
	n = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(primitives::cylinder::type, &cyl, Vec3(ZERO), 
		ent_rigid|ent_sleeping_rigid|ent_independent|ent_static|ent_terrain|ent_water, &contacts, 0,
		geom_colltype0|geom_colltype_foliage|geom_colltype_player, &params, 0, 0, &pIgnore, pIgnore?1:0);

	int ret = (int)n;

	float closestdSq = 9999.0f;
	geom_contact *closestc = 0;
	geom_contact *currentc = contacts;

	for (int i=0; i<ret; i++)
	{
		geom_contact *contact = currentc;
		if (contact)
		{
			IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(contact->iPrim[0]);
			if (pCollider)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pCollider);
				if (pEntity)
				{
					if ((pEntity == pOwner)||(pHeldObject && (pEntity == pHeldObject)))
					{
						++currentc;
						continue;
					}
				}

				float distSq = (pos-currentc->pt).len2();
				if (distSq < closestdSq)
				{
					closestdSq = distSq;
					closestc = contact;
				}
			}
		}
		++currentc;
	}

	if (ret)
	{
		WriteLockCond lockColl(*params.plock, 0);
		lockColl.SetActive(1);
	}


	if (closestc)
	{
		bool isSilentMelee = m_pWeapon->GetOwnerActor()? (m_pWeapon->GetOwnerActor()->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Stealth) : false;

		IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(closestc->iPrim[0]);
		IEntity* pCollidedEntity = pCollider ? gEnv->pEntitySystem->GetEntityFromPhysics(pCollider) : NULL;
		EntityId collidedEntityId = pCollidedEntity ? pCollidedEntity->GetId() : 0;

		Hit(closestc, dir, remote);
		Impulse(closestc->pt, dir, closestc->n, pCollider, collidedEntityId, closestc->iPrim[1], 0, closestc->id[1]);
	}

	return closestc!=0;
}

//------------------------------------------------------------------------
void CMelee::Hit(const Vec3 &pt, const Vec3 &dir, const Vec3 &normal, IPhysicalEntity *pCollider, EntityId collidedEntityId, int partId, int ipart, int surfaceIdx, bool remote)
{
	MeleeDebugLog ("CMelee<%p> HitPointDirNormal(remote=%s)", this, remote ? "true" : "false");

	CActor *pOwnerActor = m_pWeapon->GetOwnerActor();

	if (pOwnerActor)
	{	
		IEntity *pTarget = gEnv->pEntitySystem->GetEntity(collidedEntityId);
		IEntity *pOwnerEntity = pOwnerActor->GetEntity();
		IAIObject *pOwnerAI = pOwnerEntity->GetAI();
		
		float damageScale = 1.0f;
		bool silentHit = false;

		// AI events/stimulus
		if (pOwnerActor->HasNanoSuit())
		{
			damageScale = pOwnerActor->GetActorSuitGameParameters().GetProps().meleeDamageScale;
			silentHit = pOwnerActor->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Stealth;

			// Report punch to AI system.
			// The AI notification must come before the game rules are 
			// called so that the death handler in AIsystem understands that the hit
			// came from the player.
			if(pOwnerAI)
			{
				SAIEVENT AIevent;
				AIevent.targetId = pTarget ? pTarget->GetId() : 0;
				pOwnerAI->Event(AIEVENT_PLAYER_STUNT_PUNCH, &AIevent);
			}
		}

		// Send target stimuli
		if (!gEnv->bMultiplayer)
		{
			IAISystem *pAISystem = gEnv->pAISystem;
			ITargetTrackManager *pTargetTrackManager = pAISystem ? pAISystem->GetTargetTrackManager() : NULL;
			if (pTargetTrackManager && pOwnerAI)
			{
				IAIObject *pTargetAI = pTarget ? pTarget->GetAI() : NULL;
				if (pTargetAI)
				{
					const tAIObjectID aiOwnerId = pOwnerAI->GetAIObjectID();
					const tAIObjectID aiTargetId = pTargetAI->GetAIObjectID();

					pTargetTrackManager->HandleStimulusEvent(aiTargetId, aiOwnerId, "MeleeHit", pt);
					pTargetTrackManager->HandleStimulusEvent(aiOwnerId, "MeleeHitNear", pt, 5.0f);
				}
			}
		}

		//Check if is a friendly hit, in that case FX and Hit will be skipped
		bool isFriendlyHit = (pOwnerEntity && pTarget) ? IsFriendlyHit(pOwnerEntity, pTarget) : false;

		if(!isFriendlyHit)
		{
			CPlayer * pAttackerPlayer = pOwnerActor->IsPlayer() ? static_cast<CPlayer*>(pOwnerActor) : NULL;
			const float damage = pAttackerPlayer ? m_fireParams->meleeparams.damage : m_fireParams->meleeparams.damage_ai;

			if (pAttackerPlayer && pAttackerPlayer->IsPerkActive(ePerk_SuperStrength))
			{
				float scale = CPerk::GetInstance()->GetVars()->perk_superStrength_scaleMeleeDamage;
				CryLog ("Melee attacker %s has the 'Super Strength' perk enabled! Doing damage x %f to %s!", pOwnerEntity ? pOwnerEntity->GetName() : "NULL owner", scale, pTarget ? pTarget->GetName() : "NULL target");
				damageScale *= scale;
			}

			//Generate Hit
			if(pTarget)
			{
				CGameRules *pGameRules = g_pGame->GetGameRules();

				HitInfo info(m_pWeapon->GetOwnerId(), pTarget->GetId(), m_pWeapon->GetEntityId(),
					damage * damageScale, 0.0f, pGameRules->GetHitMaterialIdFromSurfaceId(surfaceIdx), partId,
					pGameRules->GetHitTypeId(silentHit ? "silentMelee" : m_fireParams->meleeparams.hit_type.c_str()), pt, dir, normal);

				if (m_fireParams->meleeparams.knockdown_chance>0 && Random(100)<m_fireParams->meleeparams.knockdown_chance)
					info.knocksDown = true;

				info.remote = remote;

				pGameRules->ClientHit(info);

			}

			//Play Material FX
			bool combatMode = pOwnerActor->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power;
			const char* meleeFXType = combatMode ? "melee_combat" : "melee";  //Benito: Check with fx guys to update names

			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();

			TMFXEffectId effectId = pMaterialEffects->GetEffectId(meleeFXType, surfaceIdx);
			if (effectId != InvalidEffectId)
			{
				SMFXRunTimeEffectParams params;
				params.pos = pt;
				params.playflags = MFX_PLAY_ALL | MFX_DISABLE_DELAY;
				params.soundSemantic = eSoundSemantic_Player_Foley;
				pMaterialEffects->ExecuteEffect(effectId, params);
			}
		}

		bool shouldPlayMeleeHitSound = true;
		if (pTarget)
		{
			CActor *pTargetActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTarget->GetId()));
			CPlayer* pTargetPlayer = (pTargetActor && pTargetActor->IsPlayer()) ? static_cast<CPlayer*>(pTargetActor) : NULL;
			if(pTargetPlayer && pTargetPlayer->IsPerkActive(ePerk_MeleeDefense))
			{
				shouldPlayMeleeHitSound = false;
			}

			if(pTargetPlayer && pTargetPlayer->IsClient())
			{
				if(m_fireParams->meleeparams.trigger_client_reaction)
				{
					pTargetPlayer->TriggerMeleeReaction();
				}
			}
			
		}

		uint32 flags = shouldPlayMeleeHitSound ? CItem::eIPAF_Default : CItem::eIPAF_Default &~ CItem::eIPAF_Sound;
		m_pWeapon->PlayAction(m_fireParams->actions.hit, 0, false, flags);
	}
}

//------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
struct CMelee::DelayedImpulse
{
	DelayedImpulse(CMelee& melee, EntityId collidedEntityId, const Vec3& impulsePos, const Vec3& impulse, int partId, int ipart)
		: m_melee(melee)
		, m_collidedEntityId(collidedEntityId)
		, m_impulsePos(impulsePos)
		, m_impulse(impulse)
		, m_partId(partId)
		, m_ipart(ipart)
	{

	};

	void execute(CItem *pItem)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_collidedEntityId);
		IPhysicalEntity* pPhysicalEntity = pEntity ? pEntity->GetPhysics() : NULL;

		if (pPhysicalEntity)
		{
			m_melee.m_collisionHelper.Impulse(pPhysicalEntity, m_impulsePos, m_impulse, m_partId, m_ipart);
		}
	}

private:
	CMelee& m_melee;
	EntityId m_collidedEntityId;
	Vec3 m_impulsePos;
	Vec3 m_impulse;
	int m_partId;
	int m_ipart;
};
//////////////////////////////////////////////////////////////////////////

void CMelee::Impulse(const Vec3 &pt, const Vec3 &dir, const Vec3 &normal, IPhysicalEntity *pCollider, EntityId collidedEntityId, int partId, int ipart, int surfaceIdx)
{
	if (pCollider && m_fireParams->meleeparams.impulse>0.001f)
	{
		//[kirill] add impulse to phys proxy - to make sure it's applied to cylinder as well (not only skeleton) - so that entity gets pushed
		// if no pEntity - do it old way
		IEntity * pEntity = gEnv->pEntitySystem->GetEntity(collidedEntityId);

		CActor* pWeaponOwner = m_pWeapon->GetOwnerActor();
		const SMeleeParams& meleeParams = m_fireParams->meleeparams;
		SNanoSuitGameParameters suitDefaultParams;
		const SNanoSuitGameParameters& suitParams = pWeaponOwner ? pWeaponOwner->GetActorSuitGameParameters() : suitDefaultParams;

		float impulse = meleeParams.impulse;
		float impulseScale = suitParams.GetProps().meleeStrengthScale;
		bool delayImpulse = false;

		if (pEntity)
		{
			IGameFramework* pGameFramework = g_pGame->GetIGameFramework();

			//If it's an entity, use the specific impulses if needed, and apply to physics proxy
			CActor* pTargetActor = static_cast<CActor*>(pGameFramework->GetIActorSystem()->GetActor(collidedEntityId));
			if (pTargetActor)
			{
				//Delay a bit on death actors, when swithching from alive to death, impulses don't apply
				//I schedule an impulse here, to get rid off the ugly .lua code which was calculating impulses on its own
				if (pTargetActor->IsDead())
				{
					delayImpulse = true;
					const SActorParams* pActorParams = pTargetActor->GetActorParams();
					const float actorCustomScale = pActorParams ? pActorParams->meeleHitRagdollImpulseScale : 1.0f;

					impulseScale *= actorCustomScale;
				}
			}
			else if (pGameFramework->GetIVehicleSystem()->GetVehicle(collidedEntityId))
			{
				impulse = m_fireParams->meleeparams.impulse_vehicle;
				impulseScale = suitParams.GetProps().meleeStrengthVehicleScale;
			}
		}

		if (!delayImpulse)
		{
			m_collisionHelper.Impulse(pCollider, pt, dir * impulse* impulseScale, partId, ipart);
		}
		else
		{
			//Force up impulse, to make the enemy fly a bit
			Vec3 newDir = (dir.z < 0.0f) ? Vec3(dir.x, dir.y, 0.1f) : dir;
			newDir.Normalize();

			m_pWeapon->GetScheduler()->TimerAction(75, CSchedulerAction<DelayedImpulse>::Create(DelayedImpulse(*this, collidedEntityId, pt, newDir * impulse * impulseScale, partId, ipart)), true);
		}

		// scar bullet
		// m = 0.0125
		// v = 800
		// energy: 4000
		// in this case the mass of the active collider is a player part
		// so we must solve for v given the same energy as a scar bullet
		float speed = cry_sqrtf(4000.0f/(80.0f*0.5f)); // 80.0f is the mass of the player

		// Check if an object. Should take lots of time to break stuff if not in nanosuit strength mode;
		// and still creates a very low impulse for stuff that might depend on receiving an impulse.
		if( IRenderNode *pBrush = (IRenderNode*)pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC) )
		{
			if (pWeaponOwner)
			{
				if (pWeaponOwner->GetActorSuitGameParameters().GetMode() != eNanoSuitMode_Power)
				{
					speed = 0.003f;
				}
			}
		}

		m_collisionHelper.GenerateArtificialCollision(m_pWeapon->GetOwner(), pCollider, pt, normal, dir * speed, partId, ipart, surfaceIdx);
	}
}

//-----------------------------------------------------------
bool CMelee::IsFriendlyHit(IEntity* pShooter, IEntity* pTarget)
{
	if(gEnv->bMultiplayer)
	{
		// Only count entity as friendly if friendly fire damage is off
		// and the player is on the same team. This will prevent blood effects.
		if (g_pGameCVars->g_friendlyfireratio == 0)
		{
			CActor* pTargetActor = (CActor*)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTarget->GetId());
			if (pTargetActor)
			{
				return pTargetActor->IsFriendlyEntity(pShooter->GetId()) != 0;
			}
		}
	}
	else
	{
		IActor* pAITarget = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTarget->GetId());
		if(pAITarget && pTarget->GetAI() && !pTarget->GetAI()->IsHostile(pShooter->GetAI(),false))
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
void CMelee::Hit(geom_contact *contact, const Vec3 &dir, bool remote)
{
	MeleeDebugLog ("CMelee<%p> HitGeomContact(remote=%s)", this, remote ? "true" : "false");

	CActor *pOwner = m_pWeapon->GetOwnerActor();
	if (!pOwner)
		return;

	Vec3 view(0.0f, 1.0f, 0.0f);

	if (IMovementController *pMC = pOwner->GetMovementController())
	{
		SMovementState state;
		pMC->GetMovementState(state);
		view = state.eyeDirection;
	}

	// some corrections to make sure the impulse is always away from the camera, and is not a backface collision
	bool backface = dir.Dot(contact->n)>0;
	bool away = dir.Dot(view.normalized())>0; // away from cam?

	Vec3 normal=contact->n;
	Vec3 ndir=dir;

	if (backface)
	{
		if (away)
			normal = -normal;
		else
			ndir = -dir;
	}
	else
	{
		if (!away)
		{
			ndir = -dir;
			normal = -normal;
		}
	}

	IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(contact->iPrim[0]);
	IEntity* pCollidedEntity = pCollider ? gEnv->pEntitySystem->GetEntityFromPhysics(pCollider) : NULL;
	EntityId collidedEntityId = pCollidedEntity ? pCollidedEntity->GetId() : 0;

	Hit(contact->pt, ndir, normal, pCollider, collidedEntityId, contact->iPrim[1], 0, contact->id[1], remote);
}

//------------------------------------------------------------------------
void CMelee::Hit(ray_hit *hit, const Vec3 &dir, bool remote)
{
	MeleeDebugLog ("CMelee<%p> HitRay(remote=%s)", this, remote ? "true" : "false");

	IEntity* pCollidedEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit->pCollider);
	EntityId collidedEntityId = pCollidedEntity ? pCollidedEntity->GetId() : 0;

	Hit(hit->pt, dir, hit->n, hit->pCollider, collidedEntityId, hit->partid, hit->ipart, hit->surface_idx, remote);
}

//-----------------------------------------------------------------------
void CMelee::ApplyMeleeEffects(bool hit)
{
	// apply suit actions
	if ( m_pWeapon )
	{
		CActor* pOwner = m_pWeapon->GetOwnerActor();
		if ( pOwner )
		{
			MeleeDebugLog ("CMelee<%p> Actor=%s (%s) ApplyMeleeEffects() hit=%s", this, pOwner ? pOwner->GetEntity()->GetName() : "NULL", pOwner->IsPlayer() ? "player" : "non-player", hit ? "true" : "false");

			if (pOwner->IsClient())
			{
				SNanoSuitEvent event;
				event.event = eNanoSuitEvent_MELEE_HIT;
				pOwner->SendActorSuitEvent(event);
			}
		}
	}
}

//---------------------------------------------------------------------
void CMelee::RequestAlignmentToNearestTarget()
{
	if(!s_meleeSnapTargetId)
		return;

	g_pGame->GetAutoAimManager().SetExternalSnapTarget(s_meleeSnapTargetId);
}

//--------------------------------------------------------
EntityId CMelee::GetNearestTarget()
{
	CActor* pOwnerActor = m_pWeapon->GetOwnerActor();
	if(pOwnerActor == NULL)
		return 0;

	CRY_ASSERT(pOwnerActor->IsClient());

	IMovementController* pMovementController = pOwnerActor->GetMovementController();
	if (!pMovementController)
		return 0;

	SMovementState moveState;
	pMovementController->GetMovementState(moveState);

	const Vec3 playerDir = moveState.aimDirection;
	const Vec3 playerPos = moveState.eyePosition;
	const float range = m_fireParams->meleeparams.range * 1.4f;
	const float angleLimit = cos_tpl(DEG2RAD(g_pGameCVars->pl_melee.melee_snap_angle_limit));

	return m_collisionHelper.GetBestAutoAimTargetForUser(pOwnerActor->GetEntityId(), playerPos, playerDir, range, angleLimit);
}

//-----------------------------------------------------------------------
void CMelee::GetMemoryUsage( ICrySizer * s ) const
{
	s->AddObject(this, sizeof(*this));
	s->AddObject(m_name);
	CFireMode::GetInternalMemoryUsage(s);	// collect memory of parent class
}

//-----------------------------------------------------------------------
void CMelee::OnZoomStateChanged()
{

}

bool CMelee::HasAutoTarget() const
{
	if (CActor *pOwner = m_pWeapon->GetOwnerActor())
	{
		return ((pOwner->IsClient()) && (s_meleeSnapTargetId != 0));
	}
	
	return false;
}

void CMelee::OnSuccesfulHit( ray_hit& hitResult )
{
	const SCollisionTestParams& collisionParams = m_collisionHelper.GetCollisionTestParams();

	IEntity* pCollidedEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hitResult.pCollider);
	EntityId collidedEntityId = pCollidedEntity ? pCollidedEntity->GetId() : 0;

	const float blend = SATURATE(m_fireParams->meleeparams.impulse_up_percentage);
	Vec3 impulseDir = Vec3(0,0,1) * blend + collisionParams.m_dir * (1.0f - blend);

	Hit(hitResult.pt, collisionParams.m_dir, hitResult.n, hitResult.pCollider, collidedEntityId, hitResult.partid, hitResult.ipart, hitResult.surface_idx, collisionParams.m_remote);
	Impulse(hitResult.pt, impulseDir, hitResult.n, hitResult.pCollider, collidedEntityId, hitResult.partid, hitResult.ipart, hitResult.surface_idx);
}

void CMelee::OnFailedHit()
{
	const SCollisionTestParams& collisionParams = m_collisionHelper.GetCollisionTestParams();

	bool collided = PerformCylinderTest(collisionParams.m_pos, collisionParams.m_dir, collisionParams.m_remote);

	bool hitAutoTarget = !collided && HasAutoTarget();
	if (hitAutoTarget)
	{
		collided = m_collisionHelper.PerformMeleeOnAutoTarget(s_meleeSnapTargetId);
	}

	ApplyMeleeEffects(collided);
}

