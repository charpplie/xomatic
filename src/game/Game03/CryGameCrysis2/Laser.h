/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Laser accessory (re-factored from Crysis L.A.M.)

-------------------------------------------------------------------------
History:
- 11-6-2008   Created by Benito Gangoso Rodriguez

*************************************************************************/

#ifndef LASER_H
#define LASER_H

#include <IItemSystem.h>
#include "Accessory.h"

class CLaser : public CAccessory
{
public:

	CLaser();
	virtual			~CLaser();

	//IItem
	virtual bool Init(IGameObject * pGameObject );
	virtual void Reset();

	virtual void OnAttach(bool attach);
	virtual void OnParentSelect(bool select);
	virtual void Update(SEntityUpdateContext& ctx, int slot);
	//~IItem

	//Item Events
	virtual void OnEnterFirstPerson();
	virtual void OnEnterThirdPerson();

	//For AI control (or outside)
	ILINE bool IsLaserActivated() const { return m_laserOn; }
	void	ActivateLaser(bool activate);

private:

	void TurnOnLaser(bool manual = false);
	void TurnOffLaser(bool manual = false);

	void UpdateFirstPerson(CItem* pParentWeapon);
	void UpdateThirdPerson(CItem* pParentWeapon);

	void ReadLaserParams(const IItemParamsNode *params);

	void GetLaserPositionAndDirectionFP(CItem* pParentWeapon, Vec3& pos, Vec3& dir);
	void GetLaserPositionAndDirectionTP(CItem* pParentWeapon, Vec3& pos, Vec3& dir);

	void CreateLaserEntity();
	void DestroyLaserEntity();

	void SetLaserEntitySlots(bool freeSlots, bool firstPerson = true);
	void SwitchFirstPersonGeometry(bool laserOn, CItem* pParentWeapon);

	bool m_laserOn;
	bool m_laserWasOn;

	EntityId m_laserEntityId;
	int			 m_laserDotSlot;
	int			 m_laserGeometrySlot;
	string m_laserHelperFP;

	float m_laserUpdateTimer;
	float m_lastLaserLength;
};

#endif