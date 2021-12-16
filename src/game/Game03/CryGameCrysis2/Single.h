/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Single-shot Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 11:9:2004   15:00 : Created by Márcio Martins

*************************************************************************/
#ifndef __SINGLE_H__
#define __SINGLE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Weapon.h"
#include "ItemParamReader.h"
#include "TracerManager.h"
#include "GameParameters.h"
#include "Recoil.h"
#include "IDeferredRaycastManager.h"
#include "WeaponSharedParams.h"
#include "FireMode.h"

#define WEAPON_HIT_RANGE				(250.0f)
#define WEAPON_HIT_MIN_DISTANCE	(1.5f)

#define AUTOAIM_TIME_OUT				(10.0f)

#define MAX_PROBABLE_HITS				(16)

struct SAmmoParams;

class CSingle :
	public CFireMode
{
	struct FillAmmoAction;
	struct StartReload_SliderBack;
	struct RezoomAction;
	struct Shoot_SliderBack;
	struct CockAction;
	struct EndReloadAction;
	class ScheduleAutoReload;

public:

	enum EProbableHitDeferredState
	{
		eProbableHitDeferredState_none,
		eProbableHitDeferredState_dispatched,
		eProbableHitDeferredState_done,
	};

	struct SProbableHitInfo : public IDeferredRaycastReceiver
	{
		CDeferredRaycastHelper		m_raycastHelper;
		Vec3											m_hit;
		EProbableHitDeferredState	m_state;

		SProbableHitInfo();

		virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);		// Raycasts
		virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult) {}	// Primitives
		virtual void OnDRWReset() {}
	};

private:
	typedef CFireMode BaseClass;

public:
	CSingle();
	virtual ~CSingle();

	//IFireMode
	virtual void PostInit();
	virtual void Update(float frameTime, uint32 frameId);
	virtual void UpdateFPView(float frameTime);
	virtual void Release();
	virtual void GetMemoryUsage(ICrySizer * s) const;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void ResetParams(const struct IItemParamsNode *params);

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const;
	virtual int GetClipSize() const;
	
	virtual bool OutOfAmmo() const;
	virtual bool LowAmmo(float thresholdPerCent) const;
	virtual bool CanReload() const;
	virtual void Reload(int zoomed);
	virtual void CancelReload();
	virtual bool CanCancelReload() { return true;};

	virtual const char *GetSuffix() const;
	virtual const char *GetSuffixAG() const;

	virtual bool CanFire(bool considerAmmo = true) const;
	virtual void StartFire();
	virtual void StopFire();
	virtual bool IsFiring() const { return m_firing; };
	virtual bool IsSilenced() const;
	virtual void SetProjectileSpeedScale(float fSpeedScale) { m_speed_scale = fSpeedScale; }
	
	virtual bool AllowZoom() const;
	virtual void Cancel();

	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootDeferred(const Vec3 &inHit);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle);
	virtual void NetEndReload() { m_reloadPending=false; };
	virtual void ReplayShoot();

	virtual void NetStartFire() {};
	virtual void NetStopFire() {};

	virtual bool IsReadyToFire() const { return CanFire(true); };

	virtual EntityId GetProjectileId() const { return m_projectileId; };
	virtual EntityId RemoveProjectileId();
	virtual void SetProjectileId(EntityId id) { m_projectileId = id; };

	virtual const char* GetType() const;
	virtual IEntityClass* GetAmmoType() const;

	virtual float GetSpinUpTime() const;
	virtual float GetSpinDownTime() const;
	virtual float GetNextShotTime() const;
	virtual void SetNextShotTime(float time);
	virtual float GetFireRate() const;

	virtual void Enable(bool enable);
	virtual bool IsEnabled() const;

	virtual void SetSecondary(bool secondary) { m_secondary = secondary; }
	virtual bool IsSecondary() const { return m_secondary; }
	
  virtual bool HasFireHelper() const;
  virtual Vec3 GetFireHelperPos() const;
  virtual Vec3 GetFireHelperDir() const;

  virtual int GetCurrentBarrel() const { return m_barrelId; }  
	virtual void Serialize(TSerialize ser) 
	{ 
		if(ser.GetSerializationTarget() != eST_Network)
		{
			ser.BeginGroup("firemode");
			ser.Value("enabled", m_enabled);
			ser.Value("nextShot", m_next_shot);
			ser.EndGroup();
			if(ser.IsReading())
				m_saved_next_shot = m_next_shot;
		}
	};

	virtual void PostSerialize() 
	{
		SetNextShotTime(m_saved_next_shot);
	};

	void PatchSpreadMod(const SSpreadModParams &sSMP);
	void ResetSpreadMod();
	void PatchRecoilMod(const SRecoilModParams &sRMP);
	void ResetRecoilMod();


	virtual void ResetLock();
	virtual void StartLocking(EntityId targetId, int partId = 0);
	virtual void Lock(EntityId targetId, int partId = 0);
	virtual void Unlock();

	virtual void OnZoomStateChanged();
	virtual const char* GetChangeFiremodeAction(const char* newFiremode);
  //~IFireMode

	//CFireMode
	virtual void InitFireMode(IWeapon* pWeapon, const SParentFireModeParams* pParams, uint32 id);
	//~CFireMode

	virtual bool IsValidAutoAimTarget(IEntity* pEntity, int partId = 0);

	virtual void StartReload(int zoomed);
	virtual void EndReload(int zoomed);
	virtual bool IsReloading();
	virtual bool Shoot(bool resetAnimation, bool autoreload=true, bool isRemote=false);
	virtual void FillAmmo();

	virtual bool ShootFromHelper(const Vec3 &eyepos, const Vec3 &probableHit) const;
	virtual Vec3 GetProbableHit(float maxRayLength, bool *pbHit=0, ray_hit *hit=0) const;
	virtual	void DeferGetProbableHit(float maxRayLength);
	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const;
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	virtual Vec3 GetFiringVelocity(const Vec3& dir) const;
	virtual Vec3 ApplySpread(const Vec3 &dir, float spread, int quadrant = -1) const;

	virtual Vec3 GetTracerPos(const Vec3 &firingPos, const STracerParams* useTracerParams);

	virtual int GetDamage() const;

	float GetRecoil() const {return m_recoil.GetRecoil();}
	virtual float GetSpread() const {return m_recoil.GetSpread();}
	virtual float GetSpreadForHUD() const { return GetSpread(); }
	float GetMinSpread() const {return m_recoil.GetMinSpread();}
	float GetMaxSpread() const {return m_recoil.GetMaxSpread();}
	virtual float GetHeat() const;
	virtual bool	CanOverheat() const;
	virtual void MuzzleFlashEffect(bool attach);
	virtual void SmokeEffect(bool effect=true);
	virtual void SpinUpEffect(bool attach);
	virtual void RejectEffect();
  virtual void RecoilImpulse(const Vec3& firingPos, const Vec3& firingDir);

	virtual void UpdateHeat(float frameTime);

	// recoil/spread
	virtual void ResetRecoil(bool spread=true) {m_recoil.Reset(spread);}

	virtual void SetRecoilMultiplier(float recoilMult) { m_recoil.SetRecoilMultiplier(recoilMult); }
	virtual float GetRecoilMultiplier() const { return m_recoil.GetRecoilmultiplier(); }

	virtual void UpdateAutoAim(float frameTime);
	virtual void PostUpdate(float frameTime);
	virtual void SetName(const char *name);
	virtual const char *GetName() { return m_name.empty()?0:m_name.c_str(); };


	virtual float GetProjectileFiringAngle(float v, float g, float x, float y);

	virtual void SetupEmitters(bool attach);
	// sound and animation flags
	virtual int PlayActionSAFlags(int flags) { return flags | CItem::eIPAF_Sound | CItem::eIPAF_Animation; };

	static int GetSkipEntities(CWeapon* pWeapon, IPhysicalEntity** pSkipEnts, int nMaxSkip);

	virtual void InitSharedParams(const SFireModeParams* pParams);

protected:

	bool DoesFireRayIntersectFrustum(const Vec3& vPos, bool& firePosInFrustum);
	bool DoesFireLineSegmentIntersectFrustum(const Vec3& start, const Vec3& end);
	
	virtual void CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius);
	void CacheTracerForParams(const STracerParams & params);
	void CacheTracer();
	void CacheAmmoGeometry();
	void ClearTracerCache();
	bool CheckAutoAimTolerance(const Vec3& aimPos, const Vec3& aimDir);
	const STracerParams * PickBestTracerParams();

	const SAmmoParams* GetAmmoParams() const;

	void SetAutoAimHelperTimer(float time) { m_autoAimHelperTimer = time;}

  bool CrosshairAssistAiming(const Vec3& firingPos, Vec3& firingDir, ray_hit* pRayhit=NULL);

	void EmitTracer(const Vec3& pos,const Vec3& destination, const STracerParams * useTracerParams);

	void UpdateBulletBelt();

	Vec3 GetOwnerVelocity() const;

	void UpdateFireAnimationWeight(float frameTime);
	bool DampRecoilEffects() const;
	float GetFireAnimationWeight() const;
	float GetFireFFeedbackWeight() const;

	virtual const char* GetBestReloadAction(int ammoCount);

	std::vector<IStatObj *> m_tracerCache;

	bool			m_fired;
	bool			m_firstFire;
	bool			m_firing;
	bool			m_reloading;
	bool			m_emptyclip;

	float			m_next_shot_dt;
	float			m_next_shot;
	float     m_saved_next_shot; //For serialization
  short     m_barrelId;

	EntityId	m_projectileId;

  struct SMuzzleEffectInfo
  {
    uint32 mfId[2];

    SMuzzleEffectInfo()
    {
      mfId[0] = mfId[1] = 0;
    }
		void GetMemoryUsage(ICrySizer *pSizer) const{}
  };
    
  std::vector<SMuzzleEffectInfo> m_mfIds;

	uint32			m_spinUpEffectId;
	float			m_spinUpTimer;		

	float			m_speed_scale;

	bool			m_enabled;
	bool			m_secondary;

	uint32						m_ammoid;

	float						m_spinUpTime;

	CRecoil			m_recoil;

	float						m_heat;
	float						m_overheat;
	float           m_nextHeatTime;
	int							m_heatEffectId;
  tSoundID        m_heatSoundId;

	int             m_smokeEffectId;

	EntityId				m_lastNearMissId;
	bool						m_firstShot;

	EntityId				m_lockedTarget;
	bool						m_bLocked;
	float						m_fStareTime;
	bool						m_bLocking;

	ItemString			m_name;	
	bool						m_targetSpotSelected;
	Vec3						m_targetSpot;
	Vec3						m_lastAimSpot;
	float						m_autoAimHelperTimer;

	float						m_autoaimTimeOut;
	
	float						m_soundVariationParam; //Should be 1.0, 2.0f, 3.0f (or something in between 0 and 5)	

	float						m_fireAnimationWeight;

	bool						m_reloadCancelled;
	int							m_reloadStartFrame;
	bool						m_reloadPending;
	bool						m_firePending;

	bool						m_lastModeStrength;

	bool						m_cocking;

	SProbableHitInfo m_probableHits[MAX_PROBABLE_HITS];

	typedef std::queue<SProbableHitInfo*> TProbableHitQueue;
  TProbableHitQueue m_queuedProbableHits;	

private:

	void RestoreOverHeating(bool activate);
};

#endif //__SINGLE_H__
