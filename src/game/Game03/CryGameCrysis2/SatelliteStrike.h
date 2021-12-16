/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description:
- SatelliteStrike projectile that is used for team perks

-------------------------------------------------------------------------
History:
- 30:4:2009   11:15 : Created by Ben Parbury

*************************************************************************/
#ifndef __SATELLITESTRIKE_H__
#define __SATELLITESTRIKE_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Projectile.h"

struct SStrikeParams
{
	SStrikeParams()
	{
		effect				= NULL;
		isInitialised = false;
	}

	IParticleEffect*	effect;
	bool							isInitialised;
};

class CSatelliteStrike : public CProjectile
{
public:
	CSatelliteStrike();
	virtual ~CSatelliteStrike();

	virtual bool Init(IGameObject *pGameObject);

	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);
	virtual void Update( SEntityUpdateContext &ctx, int updateSlot);

	static void GetHitPosition(const Vec3 &pos, Vec3 &hitPos, float &dist);
	static void StrikeTargetPosition(const int currentPoint, const int numberOfPoints, Vec3& targetPos);

protected:
	virtual void LaunchEffect();
	virtual void UpdateEffect();

	Vec3 m_targetPos;
	float m_distanceToTravel;
	float m_explodeTimer;
	float	m_explodeTime;
	IParticleEffect*	m_effect;
	const char* m_audioSignal;

	static SStrikeParams s_satelliteStrikeParams;
};

#endif // __SATELLITESTRIKE_H__
