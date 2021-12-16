/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Class for specific JAW rocket launcher functionality
-------------------------------------------------------------------------
History:
- 22:06:2007: Created by Benito G.R.
- 30:10:2009: Ported from RocketLauncher

*************************************************************************/

#pragma once

#ifndef _JAW_H_
#define _JAW_H_

#include "Weapon.h"



struct IAttachment;



class CJaw : public CWeapon, public IWeaponFiringLocator
{
private:

	typedef CWeapon BaseClass;

	struct DropAction;

public:
	CJaw();
	virtual ~CJaw() {};

	virtual void Update(SEntityUpdateContext& ctx, int slot);
	virtual void OnReset();
	virtual bool SetAspectProfile(EEntityAspects aspect, uint8 profile);
	virtual void Select(bool select);
	virtual void PickUp(EntityId pickerId, bool sound, bool select/* =true */, bool keepHistory/* =true */, const char* setup);
	virtual void Drop(float impulseScale, bool selectNext/* =true */, bool byDeath/* =false */);
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	virtual void StartFire(const Vec3 *fireTarget=NULL);
	virtual void StopFire();

	virtual void GetAttachmentsAtHelper(const char *helper, CCryFixedStringListT<5, 30> &attachments);

	virtual bool CanPickUp(EntityId userId) const;

	virtual void FullSerialize( TSerialize ser );
	virtual void PostSerialize();

	virtual void AutoDrop();
	virtual void AddFiredRocket() { m_firedRockets++; }
	virtual bool CanFire() const;
	virtual bool CanDeselect() const;


	virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit);
	virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos);
	virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
	virtual bool GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
	virtual bool GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir);
	virtual void WeaponReleased();

	virtual float GetMovementModifier() const;
	virtual float GetRotationModifier() const;

	virtual ColorF GetSilhouetteColor() const;

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
private:
	void SvActivateMissileCountermeasures(EntityId shooterId, const Vec3 &pos, const Vec3 &dir);
	bool CanAutoDrop();
	void DoAutoDrop();

	void HideRocket();
	void CreateSmokeEffect();
	void ReleaseSmokeEffect();
	IAttachment* GetSmokeHelper() const;
	void UpdatePendingShot();
	void AutoZoomOut();

	bool OnActionJawZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	int		m_firedRockets;		//Number of rockets still flying

	float	m_autoDropPendingTimer;
	float	m_dropTime;

	bool	m_auxSlotUsed;
	bool	m_auxSlotUsedBQS;
	bool	m_autoDropping;
	bool	m_controllingRocket;
	bool	m_zoomTriggerDown;
	bool	m_fireTriggerDown;
	bool	m_firePending;
	bool	m_smokeActive;
	bool	m_dropped;
};


#endif // _JAW_H_