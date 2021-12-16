/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins
-  18:07:2008		Slightly Refactored (cleaned-up): Benito G.R.

*************************************************************************/
#include "StdAfx.h"
#include "Throw.h"
#include "Actor.h"
#include "Player.h"
#include "Game.h"
#include "Projectile.h"
#include "WeaponSystem.h"
#include "GameRules.h"
#include "Battlechatter.h"

#include "WeaponSharedParams.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//------------------------------------------------------------------------
CThrow::CThrow()
: m_primed(false),
	m_throwing(false),
	m_primeTime(0.0f),
	m_projectileLifeTime(0.0f),
	m_startFireTime(0.0f),
	m_primedLoopSound(INVALID_SOUNDID)
{
}

//------------------------------------------------------------------------
CThrow::~CThrow()
{
}

//------------------------------------------------------------------
void CThrow::SetProjectileThrowParams(const SProjectileThrowParams &throwParams)
{
	SetProjectileSpeedScale(throwParams.fSpeedScale);
}

//------------------------------------------------------------------------
void CThrow::Activate(bool activate)
{
	m_primed = false;
	m_primeTime = m_startFireTime = 0.0f;
	m_firing = false;
	m_throwing = false;
	
	if(m_primedLoopSound != INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_primedLoopSound);
		m_primedLoopSound = INVALID_SOUNDID;
	}

	if(activate)
	{
		CheckAmmo();

		if(const SAmmoParams* pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(GetAmmoType()))
		{
			m_projectileLifeTime = pAmmoParams->lifetime;
		}
	}

	m_pWeapon->GetScheduler()->Reset();

	CSingle::Activate(activate);
}

//------------------------------------------------------------------------
void CThrow::Update(float frameTime, uint32 frameId)
{
	CSingle::Update(frameTime, frameId);

	CActor* pActor = m_pWeapon->GetOwnerActor();

	if(pActor && pActor->IsClient() && m_primed && IsReadyToThrow() && m_fireParams->throwparams.prime_timer && (m_projectileLifeTime > 0.0f))
	{
		float primedTime = gEnv->pTimer->GetAsyncCurTime() - m_primeTime;
		if(primedTime >= m_projectileLifeTime)
		{
			CCCPOINT(Throw_AutomaticThrow);
			Shoot(true);
			m_firing = false;
		}
		
		if(g_pGameCVars->i_debug_projectiles > 0)
		{
			const Vec3 helperPos = m_pWeapon->GetSlotHelperPos(m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson, "grenade_term"/*m_fireParams->fireparams.helper->c_str()*/, true);
			gEnv->pRenderer->DrawLabel(helperPos, 3.0f, "%.2f", (m_projectileLifeTime - primedTime));
		}

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}

	//TODO - Finish this when prediction is working again
	/*
	if(m_startFireTime > 0.0f)
	{
		if(gEnv->pTimer->GetAsyncCurTime() - m_startFireTime > 2.0f)
		{
			DisplayTrajectory();
		}
		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}*/
}

//----------------------------------------------------------------------
struct CThrow::PrimeAction
{
	PrimeAction(CThrow* _pThrow) { pThrow = _pThrow; }
	CThrow* pThrow;

	void execute(CItem* _this)
	{
		if(!pThrow->m_primed && pThrow->m_firing)
		{
			pThrow->m_primed = true;
			pThrow->m_primeTime = gEnv->pTimer->GetAsyncCurTime();
			pThrow->m_primedLoopSound = _this->PlayAction(pThrow->m_fireParams->actions.primed_loop.c_str());
		}
	}
};

//----------------------------------------------------------------------
struct CThrow::HoldAction
{
	HoldAction(CThrow* _pThrow) { pThrow = _pThrow; }
	CThrow* pThrow;

	void execute(CItem* _this)
	{
		if(!pThrow->m_throwing && pThrow->m_firing)
		{
			_this->PlayAction(pThrow->m_fireParams->actions.hold, 0, true);
		}
	}
};

void CThrow::Prime()
{
	if(!m_fireParams->throwparams.prime_enabled)
		return;

	if(!IsReadyToFire())
		return;

	if(!m_primed)
	{
		if(!IsReadyToThrow())
		{
			CCCPOINT(Throw_Prime);
			StartFire();
		}
	}
}

//------------------------------------------------------------------------
bool CThrow::CanReload() const
{
	return CSingle::CanReload();
}

//------------------------------------------------------------------------
bool CThrow::IsReadyToFire() const
{
	return CanFire(true) && !m_firing && !m_throwing;
}


//------------------------------------------------------------------------
void CThrow::StartFire()
{
	if (IsReadyToFire())
	{
		CCCPOINT(Throw_Start);

		m_firing = true;
		m_throwing = false;
		m_primed = false;
		m_startFireTime = gEnv->pTimer->GetAsyncCurTime();

		if (m_fireParams->throwparams.prime_enabled)
		{
			m_pWeapon->PlayAction(m_fireParams->actions.prime);
			m_pWeapon->GetScheduler()->TimerAction((uint32)(m_fireParams->throwparams.prime_delay * 1000.0f), CSchedulerAction<PrimeAction>::Create(this), false);
			m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_Owner) - 100, CSchedulerAction<HoldAction>::Create(this), false);
		}
		else
		{
			m_pWeapon->PlayAction(m_fireParams->actions.pre_fire);
			m_pWeapon->PlayAction(m_fireParams->actions.hold);
			m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_fireParams->actions.hold);
		}		
		
		m_pWeapon->RequestStartFire();

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
	else if (!CanFire(true))
	{ 
		// reload if possible.
		if( CanReload() )
		{
			//Auto reload
			m_pWeapon->Reload();
		}
		return;
	}
}

//------------------------------------------------------------------------
void CThrow::StopFire()
{
	if (IsReadyToThrow())
	{
		CCCPOINT(Throw_Stop);
		DoThrow();

		m_pWeapon->RequestStopFire();

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::NetStartFire()
{
	CCCPOINT(Throw_NetStart);
	StartFire();
}

//------------------------------------------------------------------------
void CThrow::NetStopFire()
{
	if (IsReadyToThrow())
	{
		CCCPOINT(Throw_NetStop);
		DoThrow();
		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
bool CThrow::CheckAmmo()
{
	bool hide = false;

	if(m_fireParams->fireparams.clip_size >= 0)
	{
		hide = m_fireParams->fireparams.clip_size == 0	? !m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class) 
																										: !m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class);
	}

	m_pWeapon->HideItem(hide);

	return hide;
}

//------------------------------------------------------------------------
struct CThrow::ThrowAction
{
	ThrowAction(CThrow *_throw): pThrow(_throw) {};
	CThrow *pThrow;

	void execute(CItem *_this)
	{
		CActor* pOwner = pThrow->m_pWeapon->GetOwnerActor();

		bool shoot = gEnv->bMultiplayer ? pOwner && pOwner->IsClient() : true;
		
		if(shoot)
		{
			pThrow->Shoot(true);
		}
		
		if(pThrow->m_primedLoopSound != INVALID_SOUNDID)
		{
			pThrow->m_pWeapon->StopSound(pThrow->m_primedLoopSound);
			pThrow->m_primedLoopSound = INVALID_SOUNDID;
		}
	}
};

struct CThrow::FinishAction
{
	FinishAction(CThrow *_throw): pThrow(_throw) {};
	CThrow *pThrow;

	void execute(CItem *_this)
	{
		pThrow->m_throwing = false;
		pThrow->m_primed = false;
		bool outOfAmmo = pThrow->CheckAmmo();

		CActor* pOwner = pThrow->m_pWeapon->GetOwnerActor();

		if(pOwner && pOwner->IsClient() && outOfAmmo)
		{
			CCCPOINT(Throw_SwitchBack);
			pOwner->SelectLastItem(true, true);
		}
		else
		{
			_this->PlayAction(_this->GetParams().select);
		}
	}
};

struct CThrow::ShowItemAction
{
	ShowItemAction(CThrow *_throw): pThrow(_throw) {};
	CThrow *pThrow;

	void execute(CItem *_this)
	{
		pThrow->m_pWeapon->HideItem(false);
	}
};

//------------------------------------------------------
void CThrow::DoThrow()
{
	if (CActor *pActor = m_pWeapon->GetOwnerActor())
	{
		BATTLECHATTER(BC_GrenadeThrow, pActor->GetEntityId());
		if(pActor->HasNanoSuit())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_THROW;
			pActor->SendActorSuitEvent(event);
		}
	}

	m_startFireTime = 0.0f;
	m_throwing = true;
	m_firing = false;

	m_pWeapon->PlayAction(m_fireParams->actions.throwit, 0, false, CItem::eIPAF_Default|CItem::eIPAF_EndCurrentWeaponAnim);
	m_pWeapon->GetScheduler()->TimerAction((uint32)(m_fireParams->throwparams.throw_delay * 1000.0f), CSchedulerAction<ThrowAction>::Create(this), false);
	m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<FinishAction>::Create(this), false);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_Owner, m_pWeapon->GetParams().idle);
}

//-----------------------------------------------------
bool CThrow::Shoot(bool resetAnimation, bool autoreload, bool isRemote)
{
	CCCPOINT(Throw_Shoot);
	//Grenade speed scale is always one (for player)
	if(CActor *pOwner = static_cast<CActor*>(m_pWeapon->GetOwnerActor()))
	{
		if(pOwner->GetHealth()<=0 || pOwner->GetGameObject()->GetAspectProfile(eEA_Physics)==eAP_Sleep)
			return false; //Do not throw grenade is player is death (AI "ghost grenades")
	}

	float newLifeTime = m_primed && m_fireParams->throwparams.prime_timer ? m_projectileLifeTime - (gEnv->pTimer->GetAsyncCurTime() - m_primeTime) : m_projectileLifeTime;
	newLifeTime = max(newLifeTime, 0.001f);
	
	m_pWeapon->HideItem(true);

	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);

	CActor *pActor = m_pWeapon->GetOwnerActor();
	bool clientIsShooter = pActor?pActor->IsClient():false;

	if (m_fireParams->fireparams.clip_size==0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	bool bHit = false;
	ray_hit rayhit;	 
	rayhit.pCollider = 0;

	Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE, &bHit, &rayhit);
	Vec3 pos = (pActor && pActor->IsPlayer()) ? m_pWeapon->GetEntity()->GetPos() : GetFiringPos(hit);
	Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(dir);

	CProjectile *pAmmo = m_throwing || gEnv->bServer ? m_pWeapon->SpawnAmmo(ammo, false) : NULL;
	if (pAmmo)
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();

		pAmmo->SetParams(m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), (int)m_fireParams->fireparams.damage, 0.f, 0.f, 0.f, pGameRules->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str()), m_fireParams->fireparams.bullet_pierceability_modifier);
		// this must be done after owner is set
		pAmmo->InitWithAI();

		if (m_bLocked)
			pAmmo->SetDestination(m_lockedTarget);
		else
			pAmmo->SetDestination(m_pWeapon->GetDestination());

		pAmmo->Launch(pos, dir, vel, m_speed_scale);

		pAmmo->SetLifeTime(newLifeTime);
		
		m_projectileId = pAmmo->GetEntity()->GetId();
	}

	if (pAmmo && pAmmo->IsPredicted() && gEnv->bClient && gEnv->bServer && clientIsShooter)
	{
		pAmmo->GetGameObject()->BindToNetwork();
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = ammo != NULL ? ammo->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, 1, (void *)m_pWeapon->GetEntityId()));
	}

	m_fired = true;
	m_next_shot += m_next_shot_dt;

	if (m_fireParams->fireparams.clip_size != -1)
	{
		if (m_fireParams->fireparams.clip_size!=0)
			m_pWeapon->SetAmmoCount(ammo, ammoCount-1);
		else
			m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount-1);
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	m_pWeapon->RequestShoot(ammo, pos, dir, vel, hit, newLifeTime, pAmmo? pAmmo->GetGameObject()->GetPredictionHandle() : 0, false);

	return true;

}

//------------------------------------------------------------------------
void CThrow::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle)
{
	CCCPOINT(Throw_NetShoot);
	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);

	CActor* pActor = m_pWeapon->GetOwnerActor();
	bool playerIsShooter = pActor?pActor->IsPlayer():false;

	extra = max(extra, 0.001f);

	int clipSize = GetClipSize();
	if (clipSize == 0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, true);
	if (pAmmo)
	{
		int hitTypeId = g_pGame->GetGameRules()->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str());			
		pAmmo->SetParams(m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), m_fireParams->fireparams.damage, 0.f, 0.f, 0.f, hitTypeId, m_fireParams->fireparams.bullet_pierceability_modifier);

		if (m_bLocked)
			pAmmo->SetDestination(m_lockedTarget);
		else
			pAmmo->SetDestination(m_pWeapon->GetDestination());

		pAmmo->SetRemote(true);

		pAmmo->Launch(pos, dir, vel, m_speed_scale);
		pAmmo->SetLifeTime(extra);

		m_projectileId = pAmmo->GetEntity()->GetId();
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = ammo != NULL ? ammo->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, 1, (void *)m_pWeapon->GetEntityId()));
	}

	m_fired = true;
	m_next_shot = 0.0f;

	ammoCount--;

	if(ammoCount<0)
		ammoCount = 0;

	if (m_pWeapon->IsServer())
	{
		if (clipSize != -1)
		{
			if (clipSize != 0)
				m_pWeapon->SetAmmoCount(ammo, ammoCount);
			else
				m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
		}
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	if (OutOfAmmo())
		m_pWeapon->OnOutOfAmmo(ammo);

	if (pAmmo && predictionHandle && pActor)
	{
		pAmmo->GetGameObject()->RegisterAsValidated(pActor->GetGameObject(), predictionHandle);
		pAmmo->GetGameObject()->BindToNetwork();
	}
	else if (pAmmo && pAmmo->IsPredicted() && gEnv->bClient && gEnv->bServer && pActor && pActor->IsClient())
	{
		pAmmo->GetGameObject()->BindToNetwork();
	}

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CThrow::ReplayShoot()
{
	CCCPOINT(CThrow_ReplayShoot);
	// Don't need to actually spawn a projectile because the entity recording system will recreate that
	// Animations will also be recorded and played back through the animation recording system
	
	// Hide the item
	m_pWeapon->HideItem(true);
	// Then show it again after the animation is finished
	m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<ShowItemAction>::Create(this), false);
}

//---------------------------------------------------
void CThrow::DisplayTrajectory()
{
	// TODO - Finish implementation when projectile prediction works again
	return;
/*
	CActor* pOwner = m_pWeapon->GetOwnerActor();
	IMovementController* pMC = pOwner ? pOwner->GetMovementController() : NULL;

	if(pMC == NULL)
		return;

	const SAmmoParams* pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(GetAmmoType());
	if(pAmmoParams == NULL)
		return;

	SMovementState moveState;
	pMC->GetMovementState(moveState);

	Vec3 velocity(0.0f, 0.0f, 0.0f);
	IPhysicalEntity* pOwnerPhysics = pOwner->GetEntity()->GetPhysics();
	if(pOwnerPhysics != NULL)
	{
		pe_status_dynamics dyn;
		pOwnerPhysics->GetStatus(&dyn);
		velocity = dyn.v;
	}

	Vec3 predictedPosOut;
	float predictedSpeedOut;
	unsigned int sampleCount = 15;
	Vec3 trajectory[15];

	if(m_pWeapon->PredictProjectileHit(pOwnerPhysics, moveState.weaponPosition, moveState.fireDirection, velocity, pAmmoParams->speed,
																	predictedPosOut, predictedSpeedOut, trajectory, &sampleCount))
	{

	}
*/
}
//-----------------------------------------------------
void CThrow::CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius)
{

}

//-----------------------------------------------------
void CThrow::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));	
	CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
}

