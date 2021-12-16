/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Bullet

-------------------------------------------------------------------------
History:
- 12:10:2005   11:15 : Created by Márcio Martins

*************************************************************************/
#pragma once

#ifndef __BULLET_H__
#define __BULLET_H__

#include "Projectile.h"

struct ISkeletonPose;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//Debug Bullet penetration

#if !defined(XENON) && !defined(PS3) && !defined(_RELEASE)
	#define DEBUG_BULLET_PENETRATION
#endif

#ifdef DEBUG_BULLET_PENETRATION

#define MAX_DEBUG_BULLET_HITS 64
#define DEFAULT_DEBUG_BULLET_HIT_LIFETIME 5.0f

struct SDebugBulletPenetration
{
private:

	struct SDebugBulletHit
	{
		SDebugBulletHit()
			: lifeTime(0.0f)
		{

		}

		Vec3 hitPosition;
		Vec3 bulletDirection;

		float damage;
		float lifeTime;

		int8 surfacePierceability;
		bool isBackFaceHit;
		bool stoppedBullet;
		bool tooThick;
	};

	typedef std::vector<SDebugBulletHit> TDebugBulletHitsList;

public:

	SDebugBulletPenetration()
		: m_nextHit(0)
	{
		m_hitsList.reserve(MAX_DEBUG_BULLET_HITS);
	}

	void AddBulletHit(const Vec3& hitPosition, const Vec3& hitDirection, float currentDamage, int8 surfacePierceability, bool isBackFace, bool stoppedBullet, bool tooThick);
	void Update(float frameTime);

private:

	const char* GetPenetrationLevelByPierceability(int8 surfacePierceability) const;

	TDebugBulletHitsList m_hitsList;

	uint32 m_nextHit;
};

#endif //DEBUG_BULLET_PENETRATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CBullet : public CProjectile
{
private: 
	typedef CProjectile BaseClass;

	struct SBackHitInfo
	{
		Vec3 pt;	
	};

public:
	CBullet();
	virtual ~CBullet();

	// CProjectile
	virtual void HandleEvent(const SGameObjectEvent &);
	virtual bool Init(IGameObject *pGameObject);
	virtual void SetParams(EntityId ownerId, EntityId hostId, EntityId weaponId, int damage, float damageFallOffStart, float damageFallOffAmount, float damageFallOffMin, int hitTypeId, int8 bulletPierceabilityModifier);
	virtual void ReInitFromPool();
	virtual bool IsAlive() const;
	// ~CProjectile

#ifdef DEBUG_BULLET_PENETRATION
	static void UpdateBulletPenetrationDebug(float frameTime)
	{
		s_debugBulletPenetration.Update(frameTime);
	}
#endif

	static int  GetWaterMaterialId() { return s_waterMaterialId; }

	static IEntityClass*	EntityClass;

protected:

	virtual void SetUpParticleParams(IEntity* pOwnerEntity, uint8 pierceabilityModifier);

private:

	void EmitUnderwaterTracer(const Vec3& pos, const Vec3& destination);
	bool FilterFriendlyAIHit(IEntity* pHitTarget);
	float GetFinalDamage(const Vec3& hitPos) const;
	ILINE float GetDamageAfterPenetrationFallOff() const { return ((float)m_damage - m_accumulatedDamageFallOffAfterPenetration); };

	void HandlePierceableSurface(const EventPhysCollision* pCollision, IEntity* pHitTarget, const Vec3& hitDirection);
	bool ShouldDestroyBullet() const;
	ILINE int16 GetBulletPierceability() const { return m_bulletPierceability; };

	bool RayTraceGeometry(const EventPhysCollision* pCollision, const Vec3& pos, const Vec3& hitDirection, SBackHitInfo* pBackHitInfo);
	void ProcessHit(CGameRules& gameRules, const EventPhysCollision& collision, IEntity& target, float damage, int hitMatId, const Vec3& hitDir);
	int GetRopeBoneId(const EventPhysCollision& collision, IEntity& target, IPhysicalEntity* pRopePhysicalEntity) const;

	std::vector <EntityId> m_hitActors;

	float m_damageFallOffStart;
	float m_damageFallOffAmount;
	float m_damageFalloffMin;

	float m_accumulatedDamageFallOffAfterPenetration;
	int16		m_bulletPierceability;
	int16		m_penetrationCount;

	bool m_alive;

	static int  s_waterMaterialId;

#ifdef DEBUG_BULLET_PENETRATION
	//Bullet penetration debug
	static SDebugBulletPenetration s_debugBulletPenetration;
#endif

};


#endif // __BULLET_H__