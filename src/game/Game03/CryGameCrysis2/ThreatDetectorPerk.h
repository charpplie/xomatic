#ifndef __THREATDETECTORPERK_H__
#define __THREATDETECTORPERK_H__

#include "IPerk.h"

class CProjectile;

class ThreatDetectorPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(ThreatDetectorPerk);

	ThreatDetectorPerk();
	virtual ~ThreatDetectorPerk() {}

	virtual void Update(const float dt);

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual const void* GetData(EPlayerPlugInData dataType);

	static void CreateBulletTrail(const Vec3 &initialPos, const Vec3 &currentPos);
protected:
	bool ShouldTrackGrenade(const CProjectile* pProj);
	bool ShouldCreateBulletTrail(const CProjectile* pProj, const Vec3 currentPos);

	void TryUpdateGrenadeTrail(CProjectile* pProj, Vec3 newPos);
	void UpdateGrenadeTrail(CProjectile* pProj, CPlayer * pPlayer, const Vec3 &newPos, const char * particleEffect, float gap, float moveSpeed, float visibilityRange, float sizeIn, float appearDistance);

	void RemoveGrenade(int index);

	const static int k_maxGrenadeTrails = 8;
	CProjectile *m_pGrenades[k_maxGrenadeTrails];
	int m_grenadesTracked;
	bool m_threatIcons;
};

#endif
