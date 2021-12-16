/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  AI grenade weapon
-------------------------------------------------------------------------
History:
- 17:01:2008: Created by Benito G.R.
							Split from OffHand to a separate, smaller and simpler class

*************************************************************************/

#ifndef __AIGRENADE_H__
#define __AIGRENADE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Weapon.h"

class CAIGrenade : public CWeapon
{
	struct FinishGrenadeAction;

public:
	virtual ~CAIGrenade() {};

	//IWeapon
	virtual void PerformThrow(const SProjectileThrowParams& throwParams, IActor* pActor = NULL);
	virtual int  GetAmmoCount(IEntityClass* pAmmoType) const { return 1; } //Always has ammo

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
};


#endif // __AIGRENADE_H__