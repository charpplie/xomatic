#include "StdAfx.h"
#include "SShootHelper.h"

#include "Game.h"
#include "WeaponSystem.h"
#include "Projectile.h"
#include "GameRules.h"

CProjectile* SShootHelper::Shoot(EntityId ownerID, const char* ammo, const char* hitType, const Vec3& firePos, const Vec3& fireDir, int damage)
{
	return ShootInternal(ownerID, ammo, hitType, firePos, fireDir, damage);
}

void SShootHelper::Explode(EntityId ownerID, const char* ammo, const Vec3& firePos, const Vec3& fireDir, int damage, float desiredRadius)
{
	if (gEnv->bServer)
	{
		const SAmmoParams* pAmmoParams = GetAmmoParams(ammo);
		if(!pAmmoParams)
		{
			CRY_ASSERT_MESSAGE(NULL, ("unable to GetAmmoParams for %s", ammo));
			return;
		}

		const SExplosionParams* pExplosionParams = pAmmoParams->pExplosion;
		if(!pExplosionParams)
		{
			CRY_ASSERT_MESSAGE(NULL, ("unable to SExplosionParams for %s", ammo));
			return;
		}

		CGameRules *pGameRules = g_pGame->GetGameRules();
		CRY_ASSERT_MESSAGE(pGameRules, "Unable to find game rules, gonna crash");

		float scaleRadius = desiredRadius > 0 ? desiredRadius/pExplosionParams->maxRadius : 1.0f;

		int hitTypeNum = pGameRules->GetHitTypeId(pExplosionParams->type.c_str());

		CRY_ASSERT_MESSAGE(hitTypeNum, string().Format("Invalid hit type '%s' in explosion params for '%s'", pExplosionParams->type.c_str(), ammo));

		ExplosionInfo explosionInfo(ownerID, 0, 0, (float) damage, firePos, fireDir,
			pExplosionParams->minRadius * scaleRadius, pExplosionParams->maxRadius * scaleRadius,
			pExplosionParams->minPhysRadius * scaleRadius, pExplosionParams->maxPhysRadius * scaleRadius,
			0.0f, pExplosionParams->pressure, pExplosionParams->holeSize, hitTypeNum);

		explosionInfo.SetEffect(pExplosionParams->effectName, pExplosionParams->effectScale * scaleRadius, pExplosionParams->maxblurdist);
		explosionInfo.SetEffectClass(pAmmoParams->pEntityClass->GetName());
		explosionInfo.SetFriendlyFire(pExplosionParams->friendlyFire);

		pGameRules->QueueExplosion(explosionInfo);
	}
}

CProjectile* SShootHelper::ShootInternal(EntityId ownerID, const char* ammo, const char* hitType, const Vec3& firePos, const Vec3& fireDir, int damage)
{
	IEntityClass* pAmmoClass = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo) : NULL;
	if(!pAmmoClass)
	{
		CRY_ASSERT_MESSAGE(NULL, string().Format("Unable to find ammo class - %s", ammo));
		return NULL;
	}

	CProjectile* pAmmo = g_pGame->GetWeaponSystem()->SpawnAmmo(pAmmoClass, false);
	if(!pAmmo)
	{
		CRY_ASSERT_MESSAGE(NULL, string().Format("Unable to Spawn ammo - %s", ammo));
		return NULL;
	}

	int hitTypeId = g_pGame ? g_pGame->GetGameRules()->GetHitTypeId(hitType): 0;

	pAmmo->SetParams(ownerID, 0, 0, damage, 0.f, 0.f, 0.f, hitTypeId, 0);
	pAmmo->Launch(firePos, fireDir, Vec3(0.0f, 0.0f, 0.0f));

	return pAmmo;
}

const SAmmoParams* SShootHelper::GetAmmoParams(const char* ammo)
{
	IEntityClass* pAmmoClass = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo) : NULL;
	if(!pAmmoClass)
	{
		CRY_ASSERT_MESSAGE(NULL, string().Format("Unable to find ammo class - %s", ammo));
		return NULL;
	}

	return g_pGame->GetWeaponSystem()->GetAmmoParams(pAmmoClass);
}