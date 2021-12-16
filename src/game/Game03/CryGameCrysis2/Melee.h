/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Beam Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 23:3:2006   13:02 : Created by Márcio Martins

*************************************************************************/

#pragma once

#ifndef __MELEE_H__
#define __MELEE_H__

#include "Weapon.h"
#include "ItemParamReader.h"
#include "IGameRulesSystem.h"
#include "GameParameters.h"

#include "FireMode.h"

#include "MeleeCollisionHelper.h"

class CMelee :
	public CFireMode,
	public IMeleeCollisionHelperListener
{
	struct StopAttackingAction;
	struct DelayedImpulse;

private:
	typedef CFireMode BaseClass;

public:
	CMelee();
	virtual ~CMelee();

	//IFireMode
	virtual void InitFireMode(IWeapon *pWeapon, const SParentFireModeParams* pParams, uint32 id);

	virtual void PostInit() { }
	virtual void Update(float frameTime, uint32 frameId);
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime) {};
	virtual void Release();
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const { return 0; };
	virtual int GetClipSize() const { return 0; };

	virtual bool OutOfAmmo() const { return false; };
	virtual bool LowAmmo(float thresholdPerCent) const { return false; };
	virtual bool CanReload() const { return false; };
	virtual void Reload(int zoomed) {};
	virtual bool IsReloading() { return false; };
	virtual void CancelReload() {};
	virtual bool CanCancelReload() { return false; };

	virtual bool AllowZoom() const { return true; };
	virtual void Cancel() {};

	virtual float GetRecoil() const { return 0.0f; };
	virtual float GetSpread() const { return 0.0f; };
	virtual float GetSpreadForHUD() const { return 0.0f; };
	virtual float GetMinSpread() const { return 0.0f; }
	virtual float GetMaxSpread() const { return 0.0f; }
	virtual const char *GetCrosshair() const { return ""; };

	virtual bool CanFire(bool considerAmmo=true) const;
	virtual void StartFire();
	virtual void StopFire();
	virtual bool IsFiring() const { return m_attacking || m_netAttacking; };
	virtual bool IsSilenced() const { return false; }
	virtual float GetHeat() const { return 0.0f; };
	virtual bool	CanOverheat() const {return false;};

	virtual void NetShoot(const Vec3 &hit, int ph);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph);
	virtual void NetEndReload() {};

	virtual void NetStartFire();
	virtual void NetStopFire();

	virtual EntityId GetProjectileId() const { return 0; };
	virtual EntityId RemoveProjectileId() { return 0; };
	virtual void SetProjectileId(EntityId id) {};

	virtual const char *GetType() const;
	virtual IEntityClass* GetAmmoType() const { return 0; };
	virtual int GetDamage() const;

	virtual float GetSpinUpTime() const { return 0.0f; };
	virtual float GetSpinDownTime() const { return 0.0f; };
  virtual float GetNextShotTime() const { return 0.0f; };
	virtual void SetNextShotTime(float time) {};
  virtual float GetFireRate() const { return 0.0f; };

	virtual void Enable(bool enable) { m_enabled = enable; };
	virtual bool IsEnabled() const { return m_enabled; };

	virtual void SetSecondary(bool secondary) { m_secondary = secondary; }
	virtual bool IsSecondary() const { return m_secondary; }

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const {return ZERO;}
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const {return ZERO;}
	virtual void SetName(const char *name) { m_name = name; };
	virtual const char *GetName() { return m_name.empty()?0:m_name.c_str();};

  virtual bool HasFireHelper() const { return false; }
  virtual Vec3 GetFireHelperPos() const { return Vec3(ZERO); }
  virtual Vec3 GetFireHelperDir() const { return FORWARD_DIRECTION; }

  virtual int GetCurrentBarrel() const { return 0; }
	virtual void Serialize(TSerialize ser) {};
	virtual void PostSerialize(){};

	virtual void SetRecoilMultiplier(float recoilMult) { }
	virtual float GetRecoilMultiplier() const { return 1.0f; }

	virtual void ResetLock() {};
	virtual void StartLocking(EntityId targetId, int partId) {};
	virtual void Lock(EntityId targetId, int partId) {};
	virtual void Unlock() {};

	virtual void OnZoomStateChanged();
	//~IFireMode

	//IMeleeCollisionHelperListener
	virtual void OnSuccesfulHit(ray_hit& hitResult);
	virtual void OnFailedHit(); 
	//~IMeleeCollisionHelperListener

private:
	EntityId GetNearestTarget();
	void RequestAlignmentToNearestTarget();
	void PerformMelee(const Vec3 &pos, const Vec3 &dir, bool remote);
	bool PerformCylinderTest(const Vec3 &pos, const Vec3 &dir, bool remote);
	void Hit(ray_hit *hit, const Vec3 &dir, bool remote);
	void Hit(geom_contact *contact, const Vec3 &dir, bool remote);
	void Hit(const Vec3 &pt, const Vec3 &dir, const Vec3 &normal, IPhysicalEntity *pCollider, EntityId collidedEntityId, int partId, int ipart, int surfaceIdx, bool remote);
	void Impulse(const Vec3 &pt, const Vec3 &dir, const Vec3 &normal, IPhysicalEntity *pCollider, EntityId collidedEntityId, int partId, int ipart, int surfaceIdx);

	void ApplyMeleeEffects(bool hit);
	bool IsFriendlyHit(IEntity* pShooter, IEntity* pTarget);

	bool HasAutoTarget() const;

protected:
	bool		m_enabled;
	bool		m_secondary;

	bool		m_attacking;

	float		m_delayTimer;
	ItemString	m_name;
	bool		m_attacked;
	bool		m_netAttacking;

	static EntityId s_meleeSnapTargetId;

	CMeleeCollisionHelper m_collisionHelper;

};

#endif //__MELEE_H__
