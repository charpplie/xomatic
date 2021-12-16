/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Weapon that requires the user to hold the 'use' button for
             a while to pick it up

-------------------------------------------------------------------------
History:
- 18:11:2009 : Created by Colin Gulliver

*************************************************************************/
#ifndef __WEAPON_WITH_PICKUP_TIMER_H__
#define __WEAPON_WITH_PICKUP_TIMER_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Weapon.h"

class CWeaponWithPickupTimer : public CWeapon
{
public:
	CWeaponWithPickupTimer();
	virtual ~CWeaponWithPickupTimer();

	void PostInit( IGameObject * pGameObject );

	virtual bool ReadItemParams(const IItemParamsNode *root);

	virtual void OnReset();

	virtual bool IsPickable() const;
	virtual bool CanPickUp(EntityId userId) const;

	virtual bool CanUse(EntityId userId) const;
	virtual void Use(EntityId userId);
	virtual void StartUse(EntityId userId);

	virtual void Drop(float impulseScale=1.0f, bool selectNext=true, bool byDeath=false);

	virtual void Update( SEntityUpdateContext& ctx, int );

	struct NoParams
	{
		void SerializeWith(const TSerialize& ser) {};
	};
	DECLARE_SERVER_RMI_NOATTACH(SvStopPickingUp, NoParams, eNRT_ReliableUnordered);

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
protected:
	void ResetVariables();

	EntityId m_pickerId;

	float m_pickupTimerLength;
	float m_pickupProgress;
};


#endif //__WEAPON_WITH_PICKUP_TIMER_H__
