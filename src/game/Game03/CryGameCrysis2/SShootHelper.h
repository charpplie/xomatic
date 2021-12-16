#ifndef __SSHOOT_HELPER_H__
#define __SSHOOT_HELPER_H__

class CProjectile;
struct SAmmoParams;

class SShootHelper
{
public:
	static CProjectile* Shoot(EntityId ownerID, const char* ammo, const char* hitType, const Vec3& firePos, const Vec3& fireDir, int damage);
	static void Explode(EntityId ownerID, const char* ammo, const Vec3& firePos, const Vec3& fireDir, int damage, float desiredRadius = -1.0f);
private:
	static CProjectile* ShootInternal(EntityId ownerID, const char* ammo, const char* hitType, const Vec3& firePos, const Vec3& fireDir, int damage);
	static const SAmmoParams* GetAmmoParams(const char* ammo);
};

#endif