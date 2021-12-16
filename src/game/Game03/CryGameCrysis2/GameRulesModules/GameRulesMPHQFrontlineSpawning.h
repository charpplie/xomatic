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

#ifndef __GAMERULES_MP_HQ_FRONTLINE_SPAWNING_H__
#define __GAMERULES_MP_HQ_FRONTLINE_SPAWNING_H__

#include "GameRulesMPSpawning.h"
#include "RecursiveDimensionalClustering.h"

class CGameRulesMPHQFrontlineSpawning : public CGameRulesMPSpawningBase
{
private:
	typedef CGameRulesMPSpawningBase inherited;

protected:
	TSpawnLocations			m_baseSpawnLocations;
	TSpawnLocations			m_nonBaseSpawnLocations;

	bool m_isHQSpawningCompatible;

public:
	CGameRulesMPHQFrontlineSpawning();
	virtual ~CGameRulesMPHQFrontlineSpawning();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest);
	virtual void RemoveSpawnLocation(EntityId id, bool isBaseSpawn);

protected:

	int GetNumberOfBaseSpawnLocations(void) const;
	int GetNumberOfNonBaseSpawnLocations(void) const;
	EntityId GetSpawnLocationHQMode(EntityId playerId, const Vec3 &deathPos);
	int CalculateTeamFromEntity(int entityId) const;
	int CalculateConflicts(float entityRDCRadius, std::vector<EntityRDCGroup> *outResultGroups) const;
	EntityId GetSpawnLocationClosestToConflict(const TSpawnLocations &spawnLocations, const std::vector<EntityRDCGroup> &conflicts, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr);
	EntityId GetSpawnLocationFrontlineMode( EntityId playerId, const Vec3 &deathPos );

	EntityId GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos);

	void VisuallyTestFrontlineSpawnMode(void);

};

#endif // __GAMERULES_MP_HQ_FRONTLINE_SPAWNING_H__