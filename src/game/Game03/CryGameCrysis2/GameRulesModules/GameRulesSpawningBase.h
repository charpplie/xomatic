/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 30:09:2009 : Created by James Bamford

*************************************************************************/

#ifndef __GAMERULES_SPAWNING_BASE_H__
#define __GAMERULES_SPAWNING_BASE_H__

#include "GameRulesModules/IGameRulesSpawningModule.h"

class CGameRules;


class CGameRulesSpawningBase : public IGameRulesSpawningModule
{
private:
	typedef IGameRulesSpawningModule inherited;

protected:
/*
	struct SPlayerData
	{
		SPlayerData() : deathTime(-10000000.f), deathPos(ZERO)	// negative time is given so first spawn definitely occurs. When autotesting server time was not ticking much in frontend and stopping spawning happening as it was not long enough since death!
		{};

		float deathTime;
		Vec3 deathPos;
	};
*/
	typedef std::vector<EntityId>								TSpawnLocations;
	typedef std::map<EntityId, SPlayerData>	TPlayerDataMap;

	TSpawnLocations			m_spawnLocations;
	TPlayerDataMap m_playerValues;
	CGameRules *m_pGameRules;

public:
	CGameRulesSpawningBase();
	virtual ~CGameRulesSpawningBase();

	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();
	virtual void Update(float frameTime);

	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return true; }

	virtual void AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest);
	virtual void RemoveSpawnLocation(EntityId id, bool isBaseSpawn);
	
	virtual EntityId GetSpawnLocation(EntityId playerId, float &zOffset);
	virtual EntityId GetFirstSpawnLocation(int teamId) const;
	virtual int GetSpawnLocationCount() const;
	virtual EntityId GetNthSpawnLocation(int idx) const;

	virtual void AddAvoidPOI(EntityId entityId, float avoidDistance, bool enabled);
	virtual void RemovePOI(EntityId entityId);
	virtual void EnablePOI(EntityId entityId);
	virtual void DisablePOI(EntityId entityId);

	virtual void PlayerJoined(EntityId playerId);
	virtual void PlayerLeft(EntityId playerId);
	virtual void SvOnPlayerKilled(const HitInfo &hitInfo);

	virtual void ClRequestRevive(EntityId playerId);
	virtual void SvRequestRevive(EntityId playerId);
	virtual void PerformRevive(EntityId playerId, int teamId);

	virtual const TPlayerDataMap* GetPlayerValuesMap(void) const { return &m_playerValues; }

	virtual void HandleOptOutOfSpawn(EntityId entityId);
	virtual void SvRecordReviveOptOut(EntityId entityId);

	virtual void ReviveAllPlayers(bool isReset);
	
	virtual int  GetRemainingLives(EntityId playerId) { return 0; }
	virtual int  GetNumLives()  { return 0; }

	virtual ILINE bool HandleAutoRevive() const { return false; }
	virtual ILINE float GetTimeFromDeathTillAutoRevive() const { return -1.f; }
	virtual ILINE bool HasOptedOut() const { return false; }

	virtual void HostMigrationInsertIntoReviveQueue(EntityId playerId, float timeTillRevive) {}

protected:
	virtual void ClAutoReviveOptOut();

	ILINE float GetTime() { return gEnv->pTimer->GetFrameStartTime().GetMilliSeconds(); }
};



#endif // __GAMERULES_SPAWNING_BASE_H__
