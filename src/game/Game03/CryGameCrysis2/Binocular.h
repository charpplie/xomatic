/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Binocular Implementation

-------------------------------------------------------------------------
History:
- 18:12:2005   14:00 : Created by Márcio Martins

*************************************************************************/
#ifndef __BINOCULAR_H__
#define __BINOCULAR_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IItemSystem.h>
#include "Weapon.h"


class CBinocular :	public CWeapon
{
public:

	CBinocular();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	
	virtual void Select(bool select);
	virtual void UpdateFPView(float frameTime);

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
	virtual bool CanModify() const;
	virtual bool CanFire() const;
	virtual void StartFire(const Vec3 *fireTarget);
	virtual void StopFire();
	virtual void OnZoomIn();
	virtual void OnZoomOut();

private:

	~CBinocular();

	bool OnActionChangeZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSprint(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	static TActionHandler<CBinocular> s_actionHandler;
	tSoundID m_ambienceLoopSoundId;
};

#endif // __BINOCULAR_H__