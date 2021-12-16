/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Beam Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 19:12:2005   12:16 : Created by Márcio Martins

*************************************************************************/
#ifndef __IMPULSEBEAM_H__
#define __IMPULSEBEAM_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CImpulseCone : public CFireMode
{
private:
	typedef CFireMode BaseClass;
public:
	CImpulseCone();
	virtual ~CImpulseCone();

	virtual void PostInit() {}
	virtual void Update(float frameTime, uint32 frameId);
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime) {};
	virtual void Release() { delete this;};
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const { return 0; };
	virtual int GetClipSize() const { return 0; };

	virtual bool OutOfAmmo() const {return false; };
	virtual bool LowAmmo(float thresholdPerCent) const {return false; };
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
	virtual float GetMinSpread() const { return 0.0f; };
	virtual float GetMaxSpread() const { return 0.0f; };
	virtual const char *GetCrosshair() const { return ""; };
	virtual float GetHeat() const { return 0.0f; };
	virtual bool	CanOverheat() const {return false;};

	virtual bool CanFire(bool considerAmmo=true) const;
	virtual void StartFire();
	virtual void StopFire();
	virtual bool IsFiring() const { return m_firing; };
	virtual bool IsSilenced() const { return false; }

	virtual void NetShoot(const Vec3 &hit, int ph) {};
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph) {};
	virtual void NetEndReload() {};

	virtual void NetStartFire() {};
	virtual void NetStopFire() {};

	virtual EntityId GetProjectileId() const {return 0;};
	virtual void SetProjectileId(EntityId id) {};
	virtual EntityId RemoveProjectileId() {return 0;};

	virtual const char *GetType() const {return "ImpulseCone" ;};
	virtual IEntityClass* GetAmmoType() const {return 0;};
	virtual int GetDamage() const{return 0;};

	virtual float GetSpinUpTime() const { return 0.0f; };
	virtual float GetSpinDownTime() const { return 0.0f; };
	virtual float GetNextShotTime() const { return 0.0f; };
	virtual void SetNextShotTime(float time) {};
	virtual float GetFireRate() const { return 0.0f; };

	virtual void Enable(bool enable) { m_enabled = enable; };
	virtual bool IsEnabled() const { return m_enabled; };


	virtual void SetSecondary(bool secondary) { m_secondary = secondary; }
	virtual bool IsSecondary() const { return m_secondary; }

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const { return ZERO;}
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const { return ZERO;}
	virtual void SetName(const char *name) {  m_name = name; };
	virtual const char *GetName() { return m_name.empty()?0:m_name.c_str();};

	virtual bool HasFireHelper() const { return false; }
	virtual Vec3 GetFireHelperPos() const { return Vec3(ZERO); }
	virtual Vec3 GetFireHelperDir() const { return FORWARD_DIRECTION; }

	virtual int GetCurrentBarrel() const { return 0; }
	virtual void PostSerialize() {};

	virtual void SetRecoilMultiplier(float recoilMult) { }
	virtual float GetRecoilMultiplier() const { return 1.0f; }

	virtual void Time() { ; };
	virtual void SetTime(float time) {  };
	virtual float GetTime() const { return 0; };

	virtual void ResetLock() {};
	virtual void StartLocking(EntityId targetId, int partId) {};
	virtual void Lock(EntityId targetId, int partId) {};
	virtual void Unlock() {};

	virtual void OnZoomStateChanged();

	virtual void Serialize(TSerialize ser) {};
protected:

	virtual void Blast(float imp);
	virtual void Contact(IPhysicalEntity *pPhysicalEntity, const Vec3 &dir, float impulse, int partId=-1, Vec3 point=ZERO);

	virtual void SetupEmitters(bool attach);
	virtual void MuzzleFlash(bool attach);

	tSoundID					m_hitSoundId;
	int								m_spinUpFXId;

	ItemString				m_name;
	bool              m_enabled;
	bool							m_secondary;

	float							m_chargeTime;
	float							m_cooldownTime;
	bool              m_charging;
	bool              m_charged;
	bool              m_firing;

	int								m_mfIds[2];

};

#endif //__IMPULSEBEAM_H__
