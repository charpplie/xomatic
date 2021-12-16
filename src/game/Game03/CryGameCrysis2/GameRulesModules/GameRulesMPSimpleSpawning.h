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

#ifndef __GAMERULES_MP_SIMPLE_SPAWNING_H__
#define __GAMERULES_MP_SIMPLE_SPAWNING_H__

#include "GameRulesMPSpawning.h"

class CGameRulesMPSimpleSpawning : public CGameRulesMPSpawningBase
{
private:
	typedef CGameRulesMPSpawningBase inherited;

protected:

public:

	CGameRulesMPSimpleSpawning();
	virtual ~CGameRulesMPSimpleSpawning();

protected:
	EntityId GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos);
};

#endif // __GAMERULES_MP_SIMPLE_SPAWNING_H__