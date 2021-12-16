/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Flashlight accessory (re-factored from Crysis L.A.M.)

-------------------------------------------------------------------------
History:
- 11-6-2008   Created by Benito Gangoso Rodriguez

*************************************************************************/

#ifndef FLASHLIGHT_H
#define FLASHLIGHT_H

#include <IItemSystem.h>
#include "Accessory.h"

class CFlashLight : public CAccessory
{
public:

	CFlashLight();
	virtual			~CFlashLight();

	//IItem
	virtual void Reset();

	virtual void OnAttach(bool attach);
	virtual void OnParentSelect(bool select);
	virtual void Update(SEntityUpdateContext& ctx, int slot);
	//~IItem

	//Item Events
	virtual void OnEnterFirstPerson();
	virtual void OnEnterThirdPerson();

	//For AI control (or outside)
	ILINE bool IsLightActivated() const{ return m_flashlightOn; }
	void	ActivateLight(bool activate);

private:

	void TurnOnFlashlight(bool force = false);
	void TurnOffFlashlight();

	void ReadFlashlightParams(const IItemParamsNode *params);

	void GetFlashlightPositionAndDirection(Vec3& pos, Vec3& dir);
	void UpdateAIFlashlight(CItem* pParentWeapon, const Vec3& pos, const Vec3& dir, float lightRange, float fov);

	bool m_flashlightOn;
	bool m_flashlightWasOn;

	float m_aiDynamicSpotLightUpdateTime;
	float m_lastLightRadius;

	int	 m_lightID[2];
};

#endif