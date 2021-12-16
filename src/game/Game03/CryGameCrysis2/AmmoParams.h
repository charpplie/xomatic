#ifndef __AMMOPARAMS_H__
#define __AMMOPARAMS_H__

#pragma once

#include <IItemSystem.h>
#include <IWeapon.h>
#include "ItemString.h"
#include <IGameRulesSystem.h>

struct SScaledEffectParams
{
	const char* ppname;
	float radius;
	float delay;
	float fadeInTime;
	float fadeOutTime;
	float maxValue;
	float aiObstructionRadius;

	SScaledEffectParams(const IItemParamsNode* scaledEffect);
};

struct SCollisionParams
{
	IParticleEffect*	pParticleEffect;
	const char*				sound;
	float							scale;

	SCollisionParams(const IItemParamsNode* collision);
	~SCollisionParams();
};

struct SExplosionParams
{
	float minRadius;
	float maxRadius;
	float soundRadius;
	float minPhysRadius;
	float maxPhysRadius;
	float pressure;
	float holeSize;
	float terrainHoleSize;
	IParticleEffect* pParticleEffect;
	const char *effectName;
	float effectScale;
	string type;
	uint8	hitTypeId;
	float maxblurdist;
	EFriendyFireType friendlyFire;

	SExplosionParams(const IItemParamsNode* explosion);
	~SExplosionParams();
};

struct SFlashbangParams
{
	float maxRadius;
	float blindAmount;
	float flashbangBaseTime;

	SFlashbangParams(const IItemParamsNode* flashbang);
};

struct SGravityParams
{
	float areaRadius;
	float areaHeight;
	float fallOff;
	Vec3	gravity;
	Vec3	fallOffSize;
	float damping;
	float lifeTime;
	bool	uniform;

	SGravityParams(const IItemParamsNode* gravity);
};

struct SBeaconParams
{
	float totalActiveLifeTime;

	ItemString	spawnProjectile;
	float		spawnProjectile_Amount;
	float   spawnProjectile_Damage;
	float		spawnProjectile_FirstDelay;
	float		spawnProjectile_Delay;
	float		spawnDistance;
	float		spawnAngleRange;

	Vec3		activeLightColor;

	ItemString	activeFX;
	ItemString	activeSound;

	SBeaconParams(const IItemParamsNode* beacon);
};

struct SImplosiveParams
{
	float		lifeTime;
	float		radius;
	float		update_tick;
	float		effectScale;

	float		maxForce;
	float		spinImpulseScale;
	float		attractImpulseScale;
	float		upImpulseScale;

	string effect;
	string sound;

	SImplosiveParams(const IItemParamsNode* implosive);
};

struct STrailParams
{
	const char* sound;
	const char*	effect;
	const char* effect_fp;
	float				scale;
	bool				prime;

	STrailParams(const IItemParamsNode* trail);
};

struct SWhizParams
{
	const char* sound;
	const char* armourPiercingSound;
	float				speed;
	float				probability;
	float				distanceSq;

	SWhizParams(const IItemParamsNode* whiz);
};

struct SLTagGrenadeParams
{
	struct SCommonParams
	{
		SCommonParams();

		void ReadParams(const IItemParamsNode* pParams);

		float		m_ltagLifetime;
		float		m_safeExplosion;
		float		m_boxDimension;
		float		m_armTime;

		string		m_armSound;
		string		m_disarmSound;
		string		m_armedSound;
		string		m_bounceSound;

		string      m_fireMaterial;
		string      m_armMaterial;
		string      m_disarmMaterial;
		string      m_armedMaterial;
		string      m_bounceMaterial;

		string		m_armedSafeMaterial;
		string		m_armedSafeTrail;

		string      m_fireTrail;
		string      m_armTrail;
		string      m_disarmTrail;
		string      m_armedTrail;
		string      m_bounceTrail;

		string			m_explosionEffect;
	};

	SCommonParams m_sticky;
	SCommonParams m_ricochet;

	SLTagGrenadeParams(const IItemParamsNode* pParams);
};

struct SGrenadeParams
{
	SGrenadeParams(const IItemParamsNode* pParams);

	float onImpactLifetime;
	float collisionDamage;
	bool	detonateOnActorImpact;
	bool	detonateOnImpact;
};

struct SVolatileSpikeParams
{
	SVolatileSpikeParams(const IItemParamsNode* pParams);

	DynArray<IMaterial*>	decalMaterials;
	IParticleEffect*			steamEffect;
	IParticleEffect*			explosionEffect;
	int										actualExplosionTime;
	float									visualExplosionTime;
	float									projectileLifeAfterExplosion;
};

struct SPierceabilityParams
{
	enum EPierceabilityLevels
	{
		ePL_NonPierceable = 0,		//Pierceability 0
		ePL_Level1,								//Pierceability 1, 2, 3
		ePL_Level2,								//Pierceability 4, 5, 6
		ePL_Level3,								//Pierceability 7, 8, 9
		ePL_MaxLevel
	};

	SPierceabilityParams()
		: maxPenetrationThickness (0.5f)
		, pierceability(0)
	{
		for (int i = 0; i < ePL_MaxLevel; i++)
		{
			damageFallOffLevels[i] = 100.0f;
		}
	}

	float GetDamageFallOffForPierceability(uint32 pierceability) const
	{
		assert(pierceability >= 0);

		static const int	kPierceabilityToPenetrationLevel[s_kMaxPierceability] = {
			ePL_NonPierceable,
			ePL_Level1, ePL_Level1, ePL_Level1,
			ePL_Level2, ePL_Level2, ePL_Level2,
			ePL_Level3, ePL_Level3, ePL_Level3,
		};
		
		return (pierceability < s_kMaxPierceability) ? damageFallOffLevels[kPierceabilityToPenetrationLevel[pierceability]] : 0.0f;
	}

	bool SurfaceRequiresBackFaceCheck(uint32 pierceability) const
	{
		return ((pierceability > 0) && (pierceability <s_kMaxPierceability));
	}

	float damageFallOffLevels[ePL_MaxLevel];
	float maxPenetrationThickness;
	int		pierceability;			//Temporary value, while bullet penetration is not enabled by default

private:

	static const uint32 s_kMaxPierceability = 10;
};

// this structure holds cached XML attributes for fast acccess
struct SAmmoParams
{
	//flags
	uint32	flags;
	bool	serverSpawn;
	bool	predictSpawn;
	bool	reusable;

	// common parameters
	float	lifetime;
	float	showtime;
	float armTime;
	float sleepTime;
	int		bulletType;
	int		hitPoints;
	int		m_hitRecoilId;
	int		m_hitRecoilIdArmorMode;
	uint16 aiType;
	bool  noBulletHits;
	bool	quietRemoval;
	
	// physics parameters
	EPhysicalizationType	physicalizationType;
	float mass;
	float speed;
	int		maxLoggedCollisions;
	bool	traceable;
	Vec3	spin;
	Vec3	spinRandom;

	ISurfaceType*							pSurfaceType;
	pe_params_particle*				pParticleParams;
	SPierceabilityParams			pierceabilityParams;

	// firstperson geometry
	string		fpGeometryName;
	mutable	bool			fpGeometryIsCached; //Need to modify in a const function ;/
	Matrix34	fpLocalTM;

	SScaledEffectParams*	pScaledEffect;
	SCollisionParams*			pCollision;
	SExplosionParams*			pExplosion;
	SFlashbangParams*			pFlashbang;
	SWhizParams*					pWhiz;
	SWhizParams*					pRicochet;
	STrailParams*					pTrail;
	STrailParams*					pTrailUnderWater;
	SGravityParams*				pGravityParams;
	SBeaconParams*				pBeaconParams;
	SImplosiveParams*			pImplosiveParams;
	SLTagGrenadeParams*		pLTagParams;
	SGrenadeParams*				pGrenadeParams;
	SVolatileSpikeParams*	pVSpikeParams;

	const IEntityClass*			pEntityClass;
	const IItemParamsNode*	pItemParams;

	SAmmoParams(const IItemParamsNode* pItemParams_ = 0, const IEntityClass* pEntityClass_=0);
	~SAmmoParams();

	void Init(const IItemParamsNode* pItemParams_, const IEntityClass* pEntityClass_);

	void CacheGeometry() const;
	void ResetCacheGeometry() const { fpGeometryIsCached = false; };

	void GetMemoryUsage(ICrySizer *pSizer) const;

	IParticleEffect* GetCachedEffect(const char* effectReferenceName) const;
	IMaterial* GetCachedMaterial(const char* materialReferenceName) const;

private:
	void LoadFlagsAndParams();
	void LoadPhysics();
	void LoadGeometry();
	void LoadScaledEffect();
	void LoadCollision();
	void LoadExplosion();
	void LoadFlashbang();
	void LoadTrailsAndWhizzes();
	void LoadGravityParams();
	void LoadBeaconParams();
	void LoadImplosiveParams();
	void LoadLTagGrenadeParams();
	void LoadGrenadeParams();
	void LoadVolatileSpikeParams();
	void LoadEffects();
	void LoadMaterials();

	typedef std::map<string,_smart_ptr<IParticleEffect>,stl::less_stricmp<string> > EffectsMap;
	EffectsMap  effectsMap;

	typedef std::map<string,_smart_ptr<IMaterial>,stl::less_stricmp<string> > MaterialsMap;
	MaterialsMap  materialsMap;
};

#endif//__AMMOPARAMS_H__
