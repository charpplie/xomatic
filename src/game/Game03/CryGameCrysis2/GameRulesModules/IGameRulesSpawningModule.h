/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 04:09:2009  : Created by James Bamford

*************************************************************************/

#ifndef __IGAMERULESSPAWNINGMODULE_H__
#define __IGAMERULESSPAWNINGMODULE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesSystem.h"

class IGameRulesSpawningModule
{
public:
	struct SPlayerData
	{
		SPlayerData() : 
			deathTime(-10000000.f), // negative time is given so first spawn definitely occurs. When autotesting server time was not ticking much in frontend and stopping spawning happening as it was not long enough since death!
			deathPos(ZERO), 
			reviveOptOutTime(0.f),
			lastRevivedTime(0.f),
			lastSpawnLocationId(0),
			lastKillerId(0)
		{};

		float deathTime;
		Vec3 deathPos;
		float reviveOptOutTime;
		float lastRevivedTime;
		EntityId lastSpawnLocationId;
		EntityId lastKillerId;
	};

	typedef std::map<EntityId, SPlayerData>	TPlayerDataMap;

public:
	virtual ~IGameRulesSpawningModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;
	virtual void Update(float frameTime) = 0;

	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) = 0;

	virtual void AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest) = 0;
	virtual void RemoveSpawnLocation(EntityId id, bool isBaseSpawn) = 0;
	
	virtual EntityId GetSpawnLocation(EntityId playerId, float &zOffset) = 0;
	virtual EntityId GetFirstSpawnLocation(int teamId) const = 0;
	virtual int GetSpawnLocationCount() const = 0;
	virtual EntityId GetNthSpawnLocation(int idx) const = 0;

	virtual void AddAvoidPOI(EntityId entityId, float avoidDistance, bool enabled) = 0;
	virtual void RemovePOI(EntityId entityId) = 0;
	virtual void EnablePOI(EntityId entityId) = 0;
	virtual void DisablePOI(EntityId entityId) = 0;

	virtual void PlayerJoined(EntityId playerId) = 0;
	virtual void PlayerLeft(EntityId playerId) = 0;
	virtual void SvOnPlayerKilled(const HitInfo &hitInfo) = 0;

	virtual void ClRequestRevive(EntityId playerId) = 0;
	virtual void SvRequestRevive(EntityId playerId) = 0;
	virtual void PerformRevive(EntityId playerId, int teamId) = 0;

	virtual const TPlayerDataMap* GetPlayerValuesMap(void) const = 0;

	virtual void HandleOptOutOfSpawn(EntityId entityId) = 0;
	virtual void SvRecordReviveOptOut(EntityId entityId) = 0;

	virtual void ReviveAllPlayers(bool isReset) = 0;

	virtual int  GetRemainingLives(EntityId playerId) = 0;
	virtual int  GetNumLives() = 0;

	virtual bool HandleAutoRevive() const = 0;
	virtual float GetTimeFromDeathTillAutoRevive() const = 0;
	virtual bool HasOptedOut() const = 0;

	virtual void HostMigrationInsertIntoReviveQueue(EntityId playerId, float timeTillRevive) = 0;

protected:
	virtual void ClAutoReviveOptOut() = 0;
};

#endif  // __IGAMERULESSPAWNINGMODULE_H__
