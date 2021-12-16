//---------------------------
// Hand grenades weapon
//
// 18:07:2008 - Benito G.R.
//------------------------------------

#ifndef _HAND_GRENADES_H_
#define _HAND_GRENADES_H_

#include "Weapon.h"
#include "IActionMapManager.h"

class CHandGrenades : public CWeapon
{

public:
	CHandGrenades();
	virtual ~CHandGrenades();

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}

private:

	typedef CWeapon inherited;

	//------------ CWeapon -----------------------
	
	bool CanSelect() const;
	bool CanDeselect() const;
	bool OnActionAttack(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionAttackPrimary(EntityId actorId, const ActionId& actionId, int activationMode, float value) { return OnActionAttack(actorId, actionId, activationMode, value); }
	void UpdateMovementIdles(bool sprinting, float runFactor);
	void StartSprint(CActor* pOwnerActor);
	bool ShouldSendOnShootHUDEvent() const;
	//------------ ~CWeapon -----------------------
};

#endif