/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Projectile

-------------------------------------------------------------------------
History:
- 12:10:2005   11:15 : Created by Márcio Martins

*************************************************************************/
#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IGameObject.h>
#include "IGameRulesSystem.h"
#include "IItemSystem.h"
#include "ItemParamReader.h"
#include "TracerManager.h"
#include "Weapon.h"
#include "AmmoParams.h"
#include "PlayerPlugin.h"
#include "WeaponSharedParams.h"


#define MIN_DAMAGE								5

class CPlayer;

class CProjectile :
	public CGameObjectExtensionHelper<CProjectile, IGameObjectExtension>,
	public IHitListener,	public IGameObjectProfileManager
{
public:
	enum ProjectileTimer
	{
		ePTIMER_LIFETIME		= 0x110,
		ePTIMER_SHOWTIME		= 0x111,
		ePTIMER_STICKY			= 0x112,
		ePTIMER_ACTIVATION	= 0x113, // used by EMPField
		ePTIMER_BEACONDELAY = 0x114,
	};

	enum ProjectileFlags
	{
		ePFlag_none									= 0,
		ePFlag_collided							= (1<<0),
		ePFlag_hitListener					= (1<<1),
		ePFlag_remote								= (1<<2),
		ePFlag_scaledEffectSignaled	= (1<<3),
		ePFlag_destroying						= (1<<4),
		ePFlag_noBulletHits					= (1<<5),
		ePFlag_knocksTarget					= (1<<6),
		ePFlag_launched							= (1<<7),
		ePFlag_isOnHUD							= (1<<8),
		ePFlag_threatTrailEmitted		= (1<<9)
	};

	CProjectile();
	virtual ~CProjectile();
	
	// IGameObjectExtension
	virtual bool Init(IGameObject *pGameObject);
	virtual void InitClient(int channelId) {};
	virtual void PostInit(IGameObject *pGameObject);
	virtual void PostInitClient(int channelId) {};
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	virtual void SerializeSpawnInfo( TSerialize ser );
	virtual ISerializableInfoPtr GetSpawnInfo();
	virtual void Update( SEntityUpdateContext &ctx, int updateSlot);	
	virtual void PostUpdate(float frameTime ) {};
	virtual void PostRemoteSpawn();
	virtual void HandleEvent( const SGameObjectEvent &);
	virtual void ProcessEvent(SEntityEvent &);
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority(bool auth);
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;
	virtual int  GetMemorySize() { return sizeof(*this); };
	//~IGameObjectExtension

	// IGameObjectProfileManager
	virtual bool SetAspectProfile( EEntityAspects aspect, uint8 profile );
	virtual uint8 GetDefaultProfile( EEntityAspects aspect );
	// ~IGameObjectProfileManager

	virtual void ReInitFromPool();
	virtual void SetLifeTime(float lifeTime);

	virtual void LoadGeometry();
	virtual void Physicalize();
	virtual void SetVelocity(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale=1.0f);
	virtual void SetParams(EntityId ownerId, EntityId hostId, EntityId weaponId, int damage, float damageFallOffStart, float damageFallOffAmount, float damageFallOffMin, int hitTypeId, int8 bulletPierceabilityModifier);
  virtual void SetDestination(const Vec3& pos){}
  virtual void SetDestination(EntityId targetId){}
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale=1.0f);
	virtual void Destroy();
	virtual bool IsRemote() const;
	virtual void SetRemote(bool remote);
	void SetKnocksTargetInfo(const SFireModeParams* pParams);
	
	virtual bool Detonate() { return true; }
	virtual void Explode(bool destroy, bool impact=false, const Vec3 &pos=ZERO, const Vec3 &normal=FORWARD_DIRECTION, const Vec3 &vel=ZERO, EntityId targetId=0, float explosionScale = 1.f);
	virtual void TrailSound(bool enable, const Vec3 &dir=Vec3(0.0f,1.0f,0.0f));
	virtual void UpdateWhiz();
	virtual void WhizSound(bool enable, const Vec3 &pos, const Vec3 &dir);
	virtual void TrailEffect(bool enable);
	virtual void FlashbangEffect(const SFlashbangParams* flashbang);
	virtual void ScaledEffect(const SScaledEffectParams* scaledEffect);
	virtual void EndScaledEffect(const SScaledEffectParams* scaledEffect);
	virtual void Activate() {};
	virtual bool IsAlive() const;

	virtual void Ricochet(EventPhysCollision *pCollision);
	virtual void RicochetSound(const Vec3 &pos, const Vec3 &dir);

	virtual int AttachEffect(bool attach, int id, const char *name=0, const Vec3 &offset=Vec3(0.0f,0.0f,0.0f), const Vec3 &dir=Vec3(0.0f,1.0f,0.0f), float scale=1.0f, bool bParticlePrime = true);

  EntityId GetOwnerId()const;

	float GetSpeed() const;
	inline float GetLifeTime() const { return m_pAmmoParams? m_pAmmoParams->lifetime : 0.0f; }
	bool IsPredicted() const { return m_pAmmoParams? m_pAmmoParams->predictSpawn != 0 : false; }

	//IHitListener
	virtual void OnHit(const HitInfo&);
	virtual void OnExplosion(const ExplosionInfo&);
	virtual void OnServerExplosion(const ExplosionInfo&);

	//Helper function to initialize particle params in exceptional cases
	void SetDefaultParticleParams(pe_params_particle *pParams);

	virtual void InitWithAI( );

	void ResolveTarget(EventPhysCollision* pCollision, int& targetId, int& sourceId, IEntity*& pTargetEntity) const;

	const SAmmoParams& GetAmmoParams() const {return *m_pAmmoParams;}
	
	inline void SetProjectileFlags(uint32 flags, bool on)		{ m_projectileFlags = on ? (m_projectileFlags | flags) : (m_projectileFlags & ~flags); }
	inline void SetProjectileFlags(uint32 flags)						{ m_projectileFlags |= flags; }
	inline void ClearProjectileFlags(uint32 flags)					{ m_projectileFlags &= ~flags; }
	inline bool CheckAnyProjectileFlags(uint32 flags) const { return (m_projectileFlags & flags) ? true : false; }
	inline bool CheckAllProjectileFlags(uint32 flags) const	{ return (m_projectileFlags & flags) == flags; }

	const Vec3 GetInitialPos() const { return m_initial_pos; }
	const Vec3 GetLastPos() const { return m_last; }

protected:
	CWeapon *GetWeapon();
	void DestroyObstructObject();

	void SetShouldBeOnHud							(bool onOff);
	void SendClientPerkEvent(EPlayerPlugInEvent event, Vec3 pos);
	bool IsGrenade() const;

	void OnLaunch() const;

	virtual bool ShouldKnockTarget() const;
	void KnockTarget(EventPhysCollision* pCollision);

	virtual void SetUpParticleParams(IEntity* pOwnerEntity, uint8 pierceabilityModifier);

	IEntitySoundProxy *GetSoundProxy();
	template<typename T> T GetParam(const char *name, T &def)
	{
		T v(def);
		if (m_pAmmoParams)
		{
			const IItemParamsNode *params = m_pAmmoParams->pItemParams->GetChild("params");
			if (params)
			{
				CItemParamReader reader(params);
				reader.Read(name, v);
			}
		}
		return v;
	}
	
	const SAmmoParams			*m_pAmmoParams;
	IPhysicalEntity *m_pPhysicalEntity;

	uint32		m_projectileFlags;

	Vec3			m_last;
	Vec3			m_initial_pos;
	Vec3			m_initial_dir;	
	Vec3			m_initial_vel;
	
	float			m_minDamageForKnockDown;
	float			m_minDamageForKnockDownLeg;
	float			m_totalLifetime;
	float			m_scaledEffectval;
	
	int				m_whizSoundId;
	int				m_trailSoundId;
	int				m_trailEffectId;
	int				m_damage;
	int				m_hitTypeId;
	int				m_hitPoints;
	int				m_chanceToKnockDownLeg; // 0..100

	EntityId	m_ownerId;
	EntityId	m_hostId;
	EntityId	m_weaponId;
	
	IPhysicalEntity *m_obstructObject;
//	IParticleEmitter *m_sonarVisionTrail;
	int8			m_bullet_pierceability_modifier;
	uint8			m_currentPhysProfile;

	bool			m_trailSoundEnable;
};

#endif
