/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: JavelinMissile
	- Used for PowerStruggleLite (C2MP game mode)
	- Missile climbs to an altitude
	- Then descends into an enemy power struggle base
-------------------------------------------------------------------------
History:
- 06:11:2009   Created by Ben Parbury

*************************************************************************/

#ifndef __JavelinMissile_H__
#define __JavelinMissile_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "HomingMissile.h"

struct IParticleEffect;

class CJavelinMissile : public CHomingMissile
{
public:
	CJavelinMissile();
	virtual ~CJavelinMissile();

	virtual bool Init(IGameObject *pGameObject);
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);

	virtual void SetDestination(EntityId targetId);
	virtual void SetDestination(const Vec3& pos);

protected:
	IParticleEffect* m_launchEffect;
};

#endif // __JavelinMissile_H__