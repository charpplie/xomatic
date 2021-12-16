/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ThrowEx.h"
#include "Actor.h"
#include "Player.h"
#include "Game.h"
#include "Projectile.h"
#include "WeaponSystem.h"
#include "GameCVars.h"

#include "WeaponSharedParams.h"
#include "ScreenEffects.h"


//------------------------------------------------------------------------
CThrowEx::CThrowEx():
m_throwing(false),
m_throwableAction(NULL),
m_dropRequested(true)
{
}

//------------------------------------------------------------------------
CThrowEx::~CThrowEx()
{
}


//------------------------------------------------------------------------
void CThrowEx::Update(float frameTime, uint32 frameId)
{
	CSingle::Update(frameTime, frameId);
}

//-----------------------------------------------------------------------
void CThrowEx::Activate(bool activate)
{
	CSingle::Activate(activate);

	m_firing = false;
	m_throwing = false;
	m_dropRequested = false;
}

//------------------------------------------------------------------------
bool CThrowEx::CanFire(bool considerAmmo) const
{
	return (!m_firing && !m_throwing);// cannot be changed. it's used in CSingle::Shoot()
}

//------------------------------------------------------------------------
bool CThrowEx::CanReload() const
{
	return false;
}

//------------------------------------------------------------------------
bool CThrowEx::IsReadyToFire() const
{
	return CanFire(true);
}

//------------------------------------------------------------------------
void CThrowEx::StartFire()
{
	if (CanFire(true))
	{
		m_firing = true;
	}
}

//------------------------------------------------------------------------
void CThrowEx::StopFire()
{
	if (m_firing && !m_throwing)
	{
		if(m_dropRequested)
			DoDrop();
		else
			DoThrow();
	}
}

//------------------------------------------------------------------------
struct CThrowEx::ThrowAction
{
	ThrowAction(CThrowEx *_throw): pThrow(_throw) {};
	CThrowEx *pThrow;

	void execute(CItem *_this)
	{
		pThrow->Shoot(true);
	}
};

void CThrowEx::DoThrow()
{
	if (CActor *pActor = m_pWeapon->GetOwnerActor())
	{
		if(pActor->HasNanoSuit())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_SHOT;
			pActor->SendActorSuitEvent(event);
		}
	}

	m_pWeapon->PlayAction(m_fireParams->actions.throwit);
	m_pWeapon->GetScheduler()->TimerAction((uint32)(m_fireParams->throwExParams.delay * 1000.0f), CSchedulerAction<ThrowAction>::Create(this), false);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_pWeapon->GetParams().idle);
}

void CThrowEx::DoDrop()
{
	if (CActor *pActor = m_pWeapon->GetOwnerActor())
	{
		if(pActor->HasNanoSuit())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_SHOT;
			pActor->SendActorSuitEvent(event);
		}
	}

	m_pWeapon->PlayAction(m_fireParams->actions.dropit);
	m_pWeapon->GetScheduler()->TimerAction((uint32)(m_fireParams->throwExParams.drop_delay * 1000.0f), CSchedulerAction<ThrowAction>::Create(this), false);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_pWeapon->GetParams().idle);
}

//----------------------------------------
bool CThrowEx::Shoot(bool resetAnimation, bool autoreload /* = true  */, bool isRemote)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_projectileId);
	IPhysicalEntity* pPhysics = pEntity ? pEntity->GetPhysics() : NULL;

	if(pEntity && pPhysics)
	{
		if(pPhysics->GetType() != PE_LIVING)
			ThrowObject(pEntity, pPhysics, m_dropRequested);
		//else
			//ThrowLivingEntity(pEntity, pPhysics, m_dropRequested);		//Called from offhand directly to workaround a bug
	}

	if(m_throwableAction)
		m_throwableAction->execute(m_pWeapon);

	m_throwableAction = NULL;
	m_projectileId = 0;
	m_dropRequested = false;

	return true;
}

//------------------------------------------------------------------------
void CThrowEx::SetThrowable(EntityId entityId, ISchedulerAction *action)
{
	m_projectileId = entityId;
	m_throwableAction = action;
}

//-----------------------------------------------------
void CThrowEx::ThrowObject(IEntity* pEntity, IPhysicalEntity* pPE, bool isDrop)
{
	CPlayer *pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());
	if(!pPlayer)
		return;

	Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
	Vec3 pos = GetFiringPos(hit);
	Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(dir);

	float strength = pPlayer->GetActorSuitGameParameters().GetProps().throwStrengthScale;

	float speed = 12.0f;
	speed *= strength;

	if(isDrop)
		speed = 2.0f;

	speed = max(2.0f, speed);

	pe_params_pos ppos;
	ppos.pos = pEntity->GetWorldPos();
	pPE->SetParams(&ppos);

	if(CheckForIntersections(pPE,dir))
	{
		Matrix34 newTM = pEntity->GetWorldTM();
		newTM.SetTranslation(newTM.GetTranslation()-(dir*0.4f));
		pEntity->SetWorldTM(newTM,ENTITY_XFORM_POS);
	}
	else
	{
		pe_action_set_velocity asv;
		asv.v = (dir*speed)+vel;
		AABB box;
		pEntity->GetWorldBounds(box);
		Vec3 finalW = -gEnv->pSystem->GetViewCamera().GetMatrix().GetColumn0()*(8.0f/max(0.1f,box.GetRadius()));
		finalW.x *= Random(0.5f,1.3f);
		finalW.y *= Random(0.5f,1.3f);
		finalW.z *= Random(0.5f,1.3f);
		asv.w = finalW;
		//asv.w = Vec3(Random(-4.5f,3.5f),Random(-1.75f,2.5f),Random(-1.5f,2.2f));
		pPE->Action(&asv);
	}		

	SEntityEvent entityEvent;
	entityEvent.event = ENTITY_EVENT_PICKUP;
	entityEvent.nParam[0] = 0;
	if (pPlayer)
		entityEvent.nParam[1] = pPlayer->GetEntityId();
	entityEvent.fParam[0] = speed;
	pEntity->SendEvent( entityEvent );

	// apply effects
	ApplyThrowEffects();
}

//-----------------------------------------------------
void CThrowEx::ApplyThrowEffects()
{
	// apply suit actions
	CActor* pOwner = m_pWeapon->GetOwnerActor();
	CPlayer* pPlayer = NULL;
	if (pOwner && pOwner->IsClient() && pOwner->IsPlayer())
		pPlayer = (CPlayer*)pOwner;
	CNanoSuit* pNanoSuit = NULL;
	if (pPlayer && pPlayer->HasNanoSuit())
		pNanoSuit = pPlayer->GetNanoSuit();

	if(pNanoSuit)
	{
		SNanoSuitEvent event;
		event.event = eNanoSuitEvent_THROW;
		pPlayer->SendActorSuitEvent(event);
	}
}

//-----------------------------------------------------
void CThrowEx::ThrowLivingEntity(IEntity* pEntity, IPhysicalEntity* pPE, bool isDrop)
{
	CPlayer *pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());
	if(pPlayer)
	{
		Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
		Vec3 pos = GetFiringPos(hit);
		Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
		Vec3 vel = GetFiringVelocity(dir);

		float speed = 8.0f;
		dir.Normalize();

		float strength = pPlayer->GetActorSuitGameParameters().GetProps().throwStrengthScale;

		if (strength>1.0f)
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_THROW;
			pPlayer->SendActorSuitEvent(event);
		}

		speed*=strength;
		if(isDrop)
			speed = 2.0f;

		if(CheckForIntersections(pPE,dir))
		{
			Matrix34 newTM = pEntity->GetWorldTM();
			newTM.SetTranslation(newTM.GetTranslation()-(dir*0.6f));
			pEntity->SetWorldTM(newTM,ENTITY_XFORM_POS);
		}

		{
			pe_action_set_velocity asv;
			asv.v = (dir*speed)+vel;
			pPE->Action(&asv); 
			// [anton] use thread safe=1 (immediate) if the character is still a living entity at this stage, 
			//   but will be ragdollized during the same frame

			pe_params_articulated_body pab;
			pab.bCheckCollisions = 1;	// was set to 0 while carrying
			pPE->SetParams(&pab);
		}		

		// Report throw to AI system.
		if (pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
		{
			SAIEVENT AIevent;
			AIevent.targetId = pEntity->GetId();
			pPlayer->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_THROW_NPC, &AIevent);
		}
	}
}

//----------------------------------------------------
bool CThrowEx::CheckForIntersections(IPhysicalEntity* heldEntity, Vec3& dir)
{

	Vec3 pos = m_pWeapon->GetSlotHelperPos(eIGS_FirstPerson, "item_attachment", true);
	ray_hit hit;

	if(gEnv->pPhysicalWorld->RayWorldIntersection(pos-dir*0.4f, dir*0.8f, ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid,
		rwi_stop_at_pierceable|14,&hit, 1, heldEntity))
		return true;

	return false;

}

//-----------------------------------------------------
void CThrowEx::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));	
	CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
}

