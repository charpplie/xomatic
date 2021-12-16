/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Impulse Beam Projectile implementation.

-------------------------------------------------------------------------
History:
- 23:02:2009   18:49 : Created by Filipe Amim

*************************************************************************/

#ifndef __IMPULSEBEAMPROJECTILE_H__
#define __IMPULSEBEAMPROJECTILE_H__

#if _MSC_VER > 1000
# pragma once
#endif

class CImpulseBeamProjectile : public CProjectile
{
public:

private:
	struct SImpulseBoxState
	{
		float m_energyRemaining;
	};

public:
	CImpulseBeamProjectile();
	virtual ~CImpulseBeamProjectile();

	// CProjectile
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);
	// ~CProjectile

	void SetImpulseBeamParams(const SImpulseConeParams* pConeData, float energy);

private:
	void Contact(IPhysicalEntity *pPhysicalEntity, float impulse, int partId=-1, Vec3 point=ZERO);

	static const unsigned int numBoxes = 5;

	Vec3 m_position;
	Vec3 m_direction;
	float m_velocity;
	float m_radius;
	SImpulseBoxState m_impulseBoxes[numBoxes];
	VectorSet<IEntity*> m_affectedEntities;

	const SImpulseConeParams* m_pConeData;
};

#endif
