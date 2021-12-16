/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------

GunTurret which can be activated by a player and then aims where that
player is aiming (until the player gets too far from the turret, a timer
elapses or the player dies)

-------------------------------------------------------------------------
History:
- 9:11:2009   Created by Tim Furnish

*************************************************************************/

#ifndef __GUNTURRET_AIMWHERECONTROLLINGPLAYERISAIMING_H__
#define __GUNTURRET_AIMWHERECONTROLLINGPLAYERISAIMING_H__

#include "GunTurret.h"

class CGunTurret_AimWhereControllingPlayerIsAiming : public CGunTurret
{
	static const NetworkAspectType ASPECT_CONTROLLINGACTOR = eEA_GameServerStatic;

	public:
	CGunTurret_AimWhereControllingPlayerIsAiming();
	~CGunTurret_AimWhereControllingPlayerIsAiming();

	protected:
	EntityId m_beingUsedByEntity;
	bool m_aimedOk;

	void CheckControllerCanStillUseTurret();
	void ActivateFeedback(EntityId who, bool activate);

	// Overload some functions defined in CGunTurret class...
	virtual bool CanUse(EntityId userId) const;
	virtual IEntity * GetClosestTarget() const;
	virtual void Use(EntityId userId);
	virtual void Update(SEntityUpdateContext& ctx, int update);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void UpdateAiming(IEntity * pCurrentTarget, SEntityUpdateContext& ctx);
	virtual void OverrideShouldShoot(IEntity* pTarget, bool & shouldShoot) const;

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CGunTurret::GetInternalMemoryUsage(s); // collect memory of parent class
	}
};

#endif // __GUNTURRET_AIMWHERECONTROLLINGPLAYERISAIMING_H__

