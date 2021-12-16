/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010
-------------------------------------------------------------------------

Helper class to record telemetry for actors

-------------------------------------------------------------------------
History:
- 02-3-2010		Steve Humphreys

*************************************************************************/

#pragma once

#ifndef __ACTORTELEMETRY_H__
#define __ACTORTELEMETRY_H__

#include <IWeapon.h>
#include <IItemSystem.h>
#include <IGameStatistics.h>		// need EStatisticsEvent
#include "ActorRef.h"
#include "NanoSuit_v2.h"

struct IStatsTracker;

class CActorTelemetry : public IItemSystemListener, public IWeaponEventListener, public CNanoSuit::INanoSuitListener
{
public:
	CActorTelemetry();
	~CActorTelemetry();

	//IItemSystemListener
	virtual void OnSetActorItem(IActor *pActor, IItem *pItem );
	virtual void OnDropActorItem(IActor *pActor, IItem *pItem ) {}
	virtual void OnSetActorAccessory(IActor *pActor, IItem *pItem ) {}
	virtual void OnDropActorAccessory(IActor *pActor, IItem *pItem ){}
	//~IItemSystemListener

	// IWeaponEventListener
	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnFireModeChanged(IWeapon *pWeapon, int currentFireMode);
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {};
	virtual void OnReadyToFire(IWeapon *pWeapon) {};
	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed) {};
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId);
	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId);
	virtual void OnStartTargetting(IWeapon *pWeapon) {};
	virtual void OnStopTargetting(IWeapon *pWeapon) {};
	virtual void OnSelected(IWeapon *pWeapon, bool selected) {};
	// ~IWeaponEventListener

	// INanoSuitListener
	virtual void ModeChanged(ENanoSuitMode mode);			
	virtual void EnergyChanged(float energy);					
	virtual void OnSuitStateChanged(ENanoSuitState state) {};
	virtual void OnModeSelected(ENanoSuitMode mode) {};
	virtual void OnSuitPowerActivated(const bool activated);
	// ~INanoSuitListener

	void SubscribeToWeapon(EntityId weaponId);
	void UnsubscribeFromWeapon();
	void SetOwner(const CActor* pActor);
	IStatsTracker* GetStatsTracker();
	void Update();

	//Save/Load
	virtual void Serialize(TSerialize ser);
	virtual void PostSerialize();

	void GetMemoryUsage( ICrySizer *pSizer ) const {}
private:
	SActorWeakRef m_ownerRef;

	// previous values and timestamps
	//	(to prevent saving unchanged data)
	CTimeValue m_lastPositionTime;
	Vec3 m_lastPosition;
	CTimeValue m_lastHealthTime;
	int32 m_lastHealth;
	CTimeValue m_lastLookDirTime;
	Vec3 m_lastLookDir;
	float m_lastSuitEnergy;

	EntityId m_currentWeaponId;
	bool m_isNanoSuitListener;
};

#endif	// __ACTORTELEMETRY_H__
