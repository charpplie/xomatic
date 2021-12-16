/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Mounted machine gun that can be ripped off by the player
and move around with it
-------------------------------------------------------------------------
History:
- 20:01:2009: Created by Benito G.R.
  30:09:2009: Ported from Rippable turret gun

*************************************************************************/

#pragma once

#ifndef _HMG_H_
#define _HMG_H_

#include "Weapon.h"

class CHMG : public CWeapon
{
private:
	struct StopUseAction;
	struct EndRippingOff;

	typedef CWeapon BaseClass;

	static const EEntityAspects ASPECT_RIPOFF = eEA_GameServerStatic;

public:

	CHMG();
	virtual ~CHMG(){};

	//Common functions
	virtual void OnReset();
	virtual void UpdateFPView(float frameTime);
	virtual void Update(SEntityUpdateContext& ctx, int slot);
	virtual void FadeCrosshair(float from, float to, float time);
	
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void InitClient(int channelId);

	//Input handling
	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	//Use functions
	virtual bool CanUse(EntityId userId) const;
	virtual void Use(EntityId userId);
	virtual void StartUse(EntityId userId);
	virtual void StopUse(EntityId userId);

	//Mount/rip-off functions
	virtual bool IsRippedOff() const { return m_rippedOff; }
	virtual bool IsRippingOrRippedOff() const { return (m_rippingOff || m_rippedOff); }
	virtual void SetRippingOff(bool ripOff);

	virtual bool UpdateAimAnims(SParams_WeaponFPAiming &aimAnimParams, bool &releaseCameraBone);
	virtual void UpdateIKMounted(IActor* pActor, const Vec3& vGunXAxis);

	virtual void ProcessEvent(SEntityEvent& event);

	virtual void Select(bool select);

	virtual void FinishRipOff();
	void UnlinkMountedGun();

	struct SRipOffParams
	{
		SRipOffParams(): ownerId(0) {};
		SRipOffParams(EntityId owner): ownerId(owner) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("ownerId", ownerId, 'eid');
		}
		EntityId ownerId;
	};

	DECLARE_SERVER_RMI_NOATTACH(SvRequestRipOff, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRipOff, SRipOffParams, eNRT_ReliableUnordered);
	
	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		GetInternalMemoryUsage(s);
	}
	void GetInternalMemoryUsage(ICrySizer * s) const
	{
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}

protected:

	virtual void PerformRipOff(CActor* pOwner);
	bool IsRippingOff() const {return m_rippingOff;}

private:

	void TryRipOffGun();
	void SetUnMountedConfiguration();

	//Input handling
	void RegisterActions();
	bool OnActionRipOff(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	static TActionHandler<CHMG>	s_actionHandler;

	EntityId m_linkedParentId;		//To remember linked entity

	bool		m_rippedOff;
	bool		m_rippingOff;
	ENanoSuitMode m_currentSuitMode;
	tSoundID m_rotatingSoundID;
	int			m_framesToStopRotationSound;
	float		m_lastZAngle;
};

#endif
