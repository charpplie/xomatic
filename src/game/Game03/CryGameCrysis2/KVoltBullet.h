/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: K-Volt bullet

-------------------------------------------------------------------------
History:
- 13:05:2009   15:00 : Created by Claire Allan

*************************************************************************/
  
#include "Projectile.h"

struct SKVoltParams
{
	SKVoltParams()
	{
		damageRadius							= 0.5f;
		directHitDamageMultiplier	= 1.5f;
		pHitEffect								= NULL;
		pEnemyHitEffect						= NULL;
		isInitialised							= false;
	}

	float							damageRadius;
	float							directHitDamageMultiplier;
	IParticleEffect*	pHitEffect;
	IParticleEffect*	pEnemyHitEffect;
	bool							isInitialised;
};

class CKVoltBullet: public CProjectile
{
public:
	CKVoltBullet();
	virtual ~CKVoltBullet();

	// CProjectile
	virtual void HandleEvent(const SGameObjectEvent& event);
	virtual bool Init(IGameObject *pGameObject);
	// ~CProjectile

protected:

	static SKVoltParams	s_kvoltParams;

	void	Trigger(Vec3 collisionPos);
	void	DamageEnemiesInRange(float range, Vec3 pos, EntityId ignoreId);
	void	ProcessDamage(EntityId targetId, float damage, int hitPartId, Vec3 pos, Vec3 dir);
	int		GetActorsInArea(float range, Vec3 pos, IPhysicalEntity**& pPhysicalEntities);

private:
	typedef CProjectile inherited;

};
