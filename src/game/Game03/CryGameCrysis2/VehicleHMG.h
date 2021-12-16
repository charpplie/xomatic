/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Mounted machine gun that can be ripped off by the player
and move around with it - vehicle mounted version
-------------------------------------------------------------------------
History:
- 16:02:2010: Created by SNH

*************************************************************************/

#pragma once

#ifndef _VEHICLEHMG_H_
#define _VEHICLEHMG_H_

#include <IItemSystem.h>
#include <CryCharAnimationParams.h>
#include "HMG.h"

struct IVehicle;
struct IVehiclePart;
struct IVehicleSeat;

class CVehicleHMG : public CHMG
{
public:

	CVehicleHMG();

	// CWeapon
	virtual void StartUse(EntityId userId);
	virtual void ApplyViewLimit(EntityId userId, bool apply) {}; // should not be done for vehicle weapons

	virtual void StartFire(const Vec3 *fireTarget=NULL);

	virtual void Update(SEntityUpdateContext& ctx, int update);

	virtual void SetAmmoCount(IEntityClass* pAmmoType, int count);
	virtual void SetInventoryAmmoCount(IEntityClass* pAmmoType, int count);

	virtual bool CanZoom() const;

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CHMG::GetInternalMemoryUsage(s); // collect memory of parent class
	}	

	virtual bool ApplyActorRecoil() const { return (m_pOwnerSeat == m_pSeatUser); }  
	// ~CWeapon

protected:

	bool CheckWaterLevel() const;
	virtual void PerformRipOff(CActor* pOwner);
	virtual void FinishRipOff();

	IVehicle* m_pVehicle;
	IVehiclePart* m_pPart;
	IVehicleSeat* m_pOwnerSeat; // owner seat of the weapon
	IVehicleSeat* m_pSeatUser; // seat of the weapons user

private:
	float   m_dtWaterLevelCheck;
};

#endif // _VEHICLEHMG_H_