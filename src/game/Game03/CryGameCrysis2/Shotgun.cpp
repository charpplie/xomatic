#include "StdAfx.h"
#include "Shotgun.h"
#include "Game.h"
#include "WeaponSystem.h"
#include "Actor.h"
#include "Player.h"
#include "Projectile.h"
#include "GameRules.h"

#include "WeaponSharedParams.h"
#include "GameForceFeedback.h"

#include  "GameCodeCoverage/GameCodeCoverageTracker.h"

void CShotgun::Activate(bool activate)
{
	CSingle::Activate(activate);

	m_next_shot = 0.1f;
	m_reload_pump = true;
	m_load_shell_on_end = false;
	m_break_reload = false;
	m_reload_was_broken = false;
	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

void CShotgun::Reload(int zoomed)
{
	StartReload(zoomed);
}

// Move shotgun into reloading pose
struct CShotgun::BeginReloadLoop
{
	BeginReloadLoop(CShotgun *_shotty, int _zoomed): shotty(_shotty), zoomed(_zoomed) {};
	CShotgun *shotty;
	int zoomed;

	void execute(CItem *_this)
	{
		shotty->ReloadShell(zoomed);
	}
};

struct CShotgun::SliderBack
{
	SliderBack(CShotgun *_shotgun): shotgun(_shotgun) {};
	CShotgun *shotgun;

	void execute(CItem *_this)
	{
		_this->StopLayer(shotgun->m_fireParams->fireparams.slider_layer);
	}
};

void CShotgun::StartReload(int zoomed)
{
	//If reload was broken to shoot, do not start a reload before shooting
	if(m_break_reload)
		return;

	int clipSize = GetClipSize();
	int ammoCount = m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class);
	if ((ammoCount >= clipSize) || m_reloading)
		return;

	CActor* pActor =  m_pWeapon->GetOwnerActor();

	m_max_shells = clipSize - ammoCount;

	if(ammoCount <= 0 && m_fireParams->shotgunparams.loadShellOnEndModes && pActor && pActor->HasNanoSuit())
	{
		ENanoSuitMode mode = pActor->GetActorSuitGameParameters().GetMode();

		if(m_fireParams->shotgunparams.loadShellOnEndModes & 1 << mode)
		{
			m_load_shell_on_end = true;
			m_max_shells--;
		}
	}

	m_reload_was_broken = false;
	m_reloading = true;
	if (zoomed)
		m_pWeapon->ExitZoom();

	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	m_pWeapon->OnStartReload(m_pWeapon->GetOwnerId(), ammo);

	m_pWeapon->PlayAction(g_pItemStrings->begin_reload, 0, false, CItem::eIPAF_Default|CItem::eIPAF_RepeatLastFrame, -1);

	m_reload_pump = ammoCount > 0 ? false : true;
	uint32 animTime = m_pWeapon->GetCurrentAnimationTime(eIGS_Owner);
	if(animTime==0)
		animTime = 500; //For DS
	m_pWeapon->GetScheduler()->TimerAction(animTime, CSchedulerAction<BeginReloadLoop>::Create(BeginReloadLoop(this, zoomed)), false);

	m_pWeapon->GetScheduler()->TimerAction((uint32)((animTime-0.125f)*1000), CSchedulerAction<SliderBack>::Create(this), false);

}

// Reload shells
class CShotgun::PartialReloadAction
{
public:
	PartialReloadAction(CWeapon *_wep, int _zoomed)
		: pWep(_wep)
		, rzoomed(_zoomed)
	{
	}
	void execute(CItem *_this)
	{
		CShotgun *fm = (CShotgun *)pWep->GetFireMode(pWep->GetCurrentFireMode());

		if(fm->m_reload_was_broken)
			return;

		IEntityClass* pAmmoType = fm->GetAmmoType();

		if (pWep->IsServer())
		{
			const int ammoCount = pWep->GetAmmoCount(pAmmoType);
			const int inventoryCount = pWep->GetInventoryAmmoCount(pAmmoType);

			const int refill = fm->m_fireParams->shotgunparams.partial_reload ? 1 : min(inventoryCount, fm->GetClipSize() - ammoCount);

			pWep->SetAmmoCount(pAmmoType, ammoCount + refill);
			pWep->SetInventoryAmmoCount(pAmmoType, pWep->GetInventoryAmmoCount(pAmmoType) - refill);
		}
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(pWep->GetOwner(), GameplayEvent(eGE_WeaponReload, pAmmoType->GetName(), 1, (void *)(pWep->GetEntityId())));

		if (!fm->m_break_reload)
			fm->ReloadShell(rzoomed);
		else
			fm->EndReload(rzoomed);
	}
private:
	CWeapon *pWep;
	int rzoomed;
};

void CShotgun::ReloadShell(int zoomed)
{
	if(m_reload_was_broken)
		return;

	CActor* pOwner = m_pWeapon->GetOwnerActor();
	bool isAI = pOwner && (pOwner->IsPlayer() == false);
	int ammoCount = m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class);
	if ((ammoCount < GetClipSize()) && (m_max_shells>0) &&
		(isAI || (m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class) > (m_load_shell_on_end ? 1 : 0))) ) // AI has unlimited ammo
	{
		CCCPOINT(Shotgun_ReloadShell);

		m_max_shells --;
		
		float speedOverride = -1.0f;
		if (pOwner)
		{
			speedOverride = pOwner->GetReloadSpeedScale();
		}

		// reload a shell
		m_pWeapon->PlayAction(g_pItemStrings->reload_shell, 0, false, CItem::eIPAF_Default|CItem::eIPAF_RepeatLastFrame|CItem::eIPAF_RestartAnimation,speedOverride);		
		uint32 animTime = m_pWeapon->GetCurrentAnimationTime(eIGS_Owner);
		if(animTime==0)
			animTime = 530; //For DS
		m_pWeapon->GetScheduler()->TimerAction(animTime, CSchedulerAction<PartialReloadAction>::Create(PartialReloadAction(m_pWeapon, zoomed)), false);
		// call this again
	}
	else
	{
		EndReload(zoomed);
	}
}

// Move shotgun out of reloading pose
class CShotgun::ReloadEndAction
{
public:
	ReloadEndAction(CWeapon *_wep, int _zoomed)
	{
		pWep = _wep;
		rzoomed = _zoomed;
	}
	void execute(CItem *_this)
	{
		CShotgun *fm = (CShotgun *)pWep->GetFireMode(pWep->GetCurrentFireMode());
		IEntityClass* ammo = fm->GetAmmoType();
		pWep->OnEndReload(pWep->GetOwnerId(), ammo);

		pWep->SetBusy(false);

		fm->m_reloading = false;
		fm->m_emptyclip = false;
		fm->m_spinUpTime = fm->m_firing ? fm->m_fireParams->fireparams.spin_up_time:0.0f;

		if (fm->m_break_reload)
		{
			fm->Shoot(true, true);
			fm->m_break_reload=false;
		}
		else if(fm->m_load_shell_on_end && pWep->IsServer())
		{
			IEntityClass* pAmmoType = fm->GetAmmoType();
			const int ammoCount = pWep->GetAmmoCount(pAmmoType);
			const int inventoryCount = pWep->GetInventoryAmmoCount(pAmmoType);

			pWep->SetAmmoCount(pAmmoType, ammoCount + 1);
			pWep->SetInventoryAmmoCount(pAmmoType, inventoryCount - 1);

			fm->m_load_shell_on_end = false;
		}
	}
private:
	CWeapon *pWep;
	int		rzoomed;
};

void CShotgun::EndReload(int zoomed)
{
	CActor *pActor = m_pWeapon->GetOwnerActor();

	float speedOverride = 1.0f;
	if (m_reload_was_broken)
	{
		speedOverride = 1.75f;
	}
	else if (pActor)
	{
		speedOverride = pActor->GetReloadSpeedScale();
	}


	uint32 animTime = 100;

	if (m_reload_pump && !m_reload_was_broken)
		m_pWeapon->PlayAction(g_pItemStrings->exit_reload_pump,0,false,CItem::eIPAF_Default,speedOverride);
	else
		m_pWeapon->PlayAction(g_pItemStrings->exit_reload_nopump,0,false,CItem::eIPAF_Default,speedOverride);

	animTime = m_pWeapon->GetCurrentAnimationTime(eIGS_Owner);

	if(!m_reload_was_broken)
		m_pWeapon->GetScheduler()->TimerAction(animTime, CSchedulerAction<ReloadEndAction>::Create(ReloadEndAction(m_pWeapon, zoomed)), false);
	else
		m_pWeapon->GetScheduler()->TimerAction(100, CSchedulerAction<ReloadEndAction>::Create(ReloadEndAction(m_pWeapon, zoomed)), false);

	m_pWeapon->SendEndReload();
}

bool CShotgun::CanFire(bool considerAmmo) const
{
	return (m_next_shot<=0.0f) && (m_spinUpTime<=0.0f) &&
		!m_pWeapon->IsBusy() && !m_pWeapon->IsSwitchingFireMode() && (!considerAmmo || !OutOfAmmo() || !m_fireParams->fireparams.ammo_type_class || GetClipSize() == -1)
		&& (!m_reloading || m_fireParams->shotgunparams.partial_reload);
}

class CShotgun::ScheduleReload
{
public:
	ScheduleReload(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) 
	{
		_pWeapon->Reload();
	}
private:
	CWeapon *_pWeapon;
};

bool CShotgun::Shoot(bool resetAnimation, bool autoreload/* =true */, bool isRemote)
{
	CCCPOINT(Shotgun_TryShoot);

	m_firePending = false;

	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);

	int clipSize = GetClipSize();
	if (clipSize == 0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	CActor *pActor = m_pWeapon->GetOwnerActor();
	bool playerIsShooter = pActor?pActor->IsPlayer():false;

	if (!CanFire(true))
	{
		if ((ammoCount <= 0) && (!m_reloading))
		{
			m_pWeapon->PlayAction(m_fireParams->actions.empty_clip);
			//Auto reload
			m_pWeapon->Reload();
			CCCPOINT(Shotgun_TryShootWhileOutOfAmmo);
		}
		else
		{
			CCCPOINT(Shotgun_TryShootWhileCannotBeFired);
		}
		return false;
	}
	else if(m_pWeapon->IsWeaponLowered())
	{
		m_pWeapon->PlayAction(m_fireParams->actions.null_fire);
		CCCPOINT(Shotgun_TryShootWhileLowered);
		return false;
	}

	if (m_reloading)
	{
		if(m_pWeapon->IsBusy())
			m_pWeapon->SetBusy(false);
		
		if(CanFire(true) && !m_break_reload)
		{
			m_break_reload = true;
			m_pWeapon->RequestCancelReload();
		}
		CCCPOINT(Shotgun_TryShootWhileReloading);
		return false;
	}

	const char *action = m_fireParams->actions.fire_cock.c_str();
	if (ammoCount == 1 || (m_fireParams->fireparams.no_cock && m_pWeapon->IsZoomed()))
		action = m_fireParams->actions.fire.c_str();

	m_pWeapon->PlayAction(action, 0, false, CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_CleanBlending|CItem::eIPAF_Shoot);

	Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
	Vec3 pos = GetFiringPos(hit);
	Vec3 fdir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(fdir);
	Vec3 dir;
	const float hitDist = hit.GetDistance(pos);

	CheckNearMisses(hit, pos, fdir, WEAPON_HIT_RANGE, m_fireParams->shotgunparams.spread);
	
	bool serverSpawn = m_pWeapon->IsServerSpawn(ammo);

	int quad = Random(4);
	// SHOT HERE
	for (int i = 0; i < m_fireParams->shotgunparams.pellets; i++)
	{
		CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, false);
		if (pAmmo)
		{
			dir = ApplySpread(fdir, m_fireParams->shotgunparams.spread, quad);  
			quad = (quad+1)%4;
			int hitTypeId = g_pGame->GetGameRules()->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str());	

			int pelletDamage = m_fireParams->shotgunparams.pelletdamage;
			if (m_fireParams->shotgunparams.secondary_damage && !playerIsShooter)
				pelletDamage = m_fireParams->shotgunparams.ai_vs_player_damage;

			pAmmo->SetParams(
				m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), pelletDamage, m_fireParams->fireparams.damage_drop_min_distance,
				m_fireParams->fireparams.damage_drop_per_meter, m_fireParams->fireparams.damage_drop_min_damage, hitTypeId, m_fireParams->fireparams.bullet_pierceability_modifier);
			pAmmo->SetDestination(m_pWeapon->GetDestination());
			pAmmo->Launch(pos, dir, vel);
			pAmmo->SetKnocksTargetInfo( GetShared() );

			if ((!m_fireParams->tracerparams.geometry.empty() || !m_fireParams->tracerparams.effect.empty()) && ((ammoCount == clipSize) || (ammoCount%m_fireParams->tracerparams.frequency==0)))
			{
				EmitTracer(pos,pos+dir*hitDist, & m_fireParams->tracerparams);
			}

			m_projectileId = pAmmo->GetEntity()->GetId();

		}
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = ammo != NULL ? ammo->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, m_fireParams->shotgunparams.pellets, (void *)m_pWeapon->GetEntityId()));
	}

	MuzzleFlashEffect(true);
	RejectEffect();

	m_fired = true;
	m_next_shot += m_next_shot_dt;
	ammoCount--;

	if (playerIsShooter)
	{
		GameForceFeedback_ForEntity(m_pWeapon->GetOwnerId(), SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.15f, 0.0f, fabsf(m_recoil.GetBackImpulse())*3.0f) );
	}
	if (clipSize != -1)
	{
		if (clipSize != 0)
			m_pWeapon->SetAmmoCount(ammo, ammoCount);
		else
			m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
	}

	if ((ammoCount<1) && !m_fireParams->fireparams.slider_layer.empty())
	{
		const char *slider_back_layer = m_fireParams->fireparams.slider_layer.c_str();
		m_pWeapon->PlayLayer(slider_back_layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), 0, ammo, pos, dir, vel);

	if (OutOfAmmo())
	{
		m_pWeapon->OnOutOfAmmo(ammo);
		if (autoreload)
		{
			m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<ScheduleReload>::Create(m_pWeapon), false);
		}
	}

	m_pWeapon->RequestShoot(ammo, pos, dir, vel, hit, 1.0f, 0, false);

	CCCPOINT(Shotgun_Fired);

	return true;
}

//------------------------------------------------------------------------
void CShotgun::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph)
{
	CCCPOINT(Shotgun_NetShoot);

	assert(0 == ph);
	
	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	const char *action = m_fireParams->actions.fire_cock.c_str();

	CActor *pActor = m_pWeapon->GetOwnerActor();
	bool playerIsShooter = pActor?pActor->IsPlayer():false;

	int ammoCount = m_pWeapon->GetAmmoCount(ammo);
	int clipSize = GetClipSize();
	if (clipSize == 0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	if (ammoCount == 1)
		action = m_fireParams->actions.fire.c_str();

	m_pWeapon->ResetAllAnimations();
	m_pWeapon->PlayAction(action, 0, false, CItem::eIPAF_Default|CItem::eIPAF_NoBlend|CItem::eIPAF_RestartAnimation);

	Vec3 pdir;

	int quad = Random(4);

	// SHOT HERE
	for (int i = 0; i < m_fireParams->shotgunparams.pellets; i++)
	{
		CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, true);
		if (pAmmo)
		{
			pdir = ApplySpread(dir, m_fireParams->shotgunparams.spread, quad);
			quad = (quad+1)%4;
			int hitTypeId = g_pGame->GetGameRules()->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str());			

			pAmmo->SetParams(
				m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), m_fireParams->shotgunparams.pelletdamage, m_fireParams->fireparams.damage_drop_min_distance,
				m_fireParams->fireparams.damage_drop_min_damage, m_fireParams->fireparams.damage_drop_per_meter, hitTypeId, m_fireParams->fireparams.bullet_pierceability_modifier);
			pAmmo->SetDestination(m_pWeapon->GetDestination());
			pAmmo->SetRemote(true);
			pAmmo->Launch(pos, pdir, vel);

			bool emit = false;
			if(m_pWeapon->GetStats().fp)
				emit = (!m_fireParams->tracerparams.geometryFP.empty() || !m_fireParams->tracerparams.effectFP.empty()) && ((ammoCount == clipSize) || (ammoCount%m_fireParams->tracerparams.frequency==0));
			else
				emit = (!m_fireParams->tracerparams.geometry.empty() || !m_fireParams->tracerparams.effect.empty()) && ((ammoCount == clipSize) || (ammoCount%m_fireParams->tracerparams.frequency==0));

			if (emit)
				EmitTracer(pos,hit,&m_fireParams->tracerparams);

			m_projectileId = pAmmo->GetEntity()->GetId();
		}
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = ammo != NULL ? ammo->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, m_fireParams->shotgunparams.pellets, (void *)m_pWeapon->GetEntityId()));
	}

	MuzzleFlashEffect(true);
	RejectEffect();

	m_fired = true;
	m_next_shot = 0.0f;

	ammoCount--;
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

	if ((ammoCount<1) && !m_fireParams->fireparams.slider_layer.empty())
	{
		const char *slider_back_layer = m_fireParams->fireparams.slider_layer.c_str();
		m_pWeapon->PlayLayer(slider_back_layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), 0, ammo, pos, dir, vel);

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//---------------------------------------------------------------------
void CShotgun::CancelReload()
{
	if(!m_reload_was_broken)
	{
		m_reload_was_broken = true;
		m_pWeapon->GetScheduler()->Reset();
		EndReload(0);
	}
}

//------------------------------------------------------------------------
const char *CShotgun::GetType() const
{
	return "Shotgun";
}

//------------------------------------------------
void CShotgun::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));	
	GetInternalMemoryUsage(s);
}
void CShotgun::GetInternalMemoryUsage(ICrySizer * s) const
{
	CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
}

//------------------------------------------------
float CShotgun::GetSpreadForHUD() const
{
	return m_fireParams->shotgunparams.spread;
}
