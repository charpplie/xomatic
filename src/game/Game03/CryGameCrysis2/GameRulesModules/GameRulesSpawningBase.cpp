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

#include "StdAfx.h"
#include "GameRulesSpawningBase.h"
#include "Game.h"
#include "GameRules.h"

struct compare_spawns
{
	bool operator() (EntityId lhs, EntityId rhs ) const
	{
		int lhsT=g_pGame->GetGameRules()->GetTeam(lhs);
		int rhsT=g_pGame->GetGameRules()->GetTeam(rhs);
		if (lhsT == rhsT)
		{
			EntityId lhsG=g_pGame->GetGameRules()->GetSpawnLocationGroup(lhs);
			EntityId rhsG=g_pGame->GetGameRules()->GetSpawnLocationGroup(rhs);
			if (lhsG==rhsG)
			{
				IEntity *pLhs=gEnv->pEntitySystem->GetEntity(lhs);
				IEntity *pRhs=gEnv->pEntitySystem->GetEntity(rhs);

				return strcmp(pLhs->GetName(), pRhs->GetName())<0;
			}
			return lhsG<rhsG;
		}
		return lhsT<rhsT;
	}
};

CGameRulesSpawningBase::CGameRulesSpawningBase()
{
	m_pGameRules=NULL;
}

CGameRulesSpawningBase::~CGameRulesSpawningBase()
{

}

void CGameRulesSpawningBase::Init(XmlNodeRef xml)
{
	m_pGameRules = g_pGame->GetGameRules();
}

void CGameRulesSpawningBase::PostInit()
{

}

void CGameRulesSpawningBase::Update(float frameTime)
{

}

void CGameRulesSpawningBase::AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest)
{
	stl::push_back_unique(m_spawnLocations, location);
	//std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns());	// perhaps this isn't required anymore now spawngroups dont rely on this ordering?
}

void CGameRulesSpawningBase::RemoveSpawnLocation(EntityId id, bool isBaseSpawn)
{
	stl::find_and_erase(m_spawnLocations, id);
	//std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns());	// perhaps this isn't required anymore now spawngroups dont rely on this ordering?
}

EntityId CGameRulesSpawningBase::GetSpawnLocation(EntityId playerId, float &zOffset)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::GetSpawnLocation() is NOT implemented and shouldn't be called until it has been"));
	return 0;
}

EntityId CGameRulesSpawningBase::GetFirstSpawnLocation(int teamId) const
{
	if (!m_spawnLocations.empty())
	{
		for (TSpawnLocations::const_iterator it=m_spawnLocations.begin(); it!=m_spawnLocations.end(); ++it)
		{
			if (teamId==m_pGameRules->GetTeam(*it)) // && (!groupId || groupId==GetSpawnLocationGroup(*it)))	// no groups yet
				return *it;
		}
	}

	return 0;
}

int CGameRulesSpawningBase::GetSpawnLocationCount() const
{
	return (int)m_spawnLocations.size();
}

EntityId CGameRulesSpawningBase::GetNthSpawnLocation(int idx) const
{
	if (idx>=0 && idx<(int)m_spawnLocations.size())
		return m_spawnLocations[idx];
	return 0;
}

void CGameRulesSpawningBase::AddAvoidPOI(EntityId entityId, float avoidDistance, bool enabled)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::AddAvoidPOI() is NOT implemented and shouldn't be called until it has been"));
}

void CGameRulesSpawningBase::RemovePOI(EntityId entityId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::RemovePOI() is NOT implemented and shouldn't be called until it has been"));
}
	
void CGameRulesSpawningBase::EnablePOI(EntityId entityId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::EnablePOI() is NOT implemented and shouldn't be called until it has been"));

}

void CGameRulesSpawningBase::DisablePOI(EntityId entityId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::DisablePOI() is NOT implemented and shouldn't be called until it has been"));
}

void CGameRulesSpawningBase::PlayerJoined(EntityId playerId)
{
	m_playerValues.insert(TPlayerDataMap::value_type(playerId, SPlayerData()));
}

void CGameRulesSpawningBase::PlayerLeft(EntityId playerId)
{
	TPlayerDataMap::iterator it = m_playerValues.find(playerId);
	if (it != m_playerValues.end())
	{
		m_playerValues.erase(it);
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesSpawningBase::PlayerLeft() failed to find a playervalue for player. This should not happen");
	}
}

void CGameRulesSpawningBase::SvOnPlayerKilled(const HitInfo &hitInfo)
{
	CryLog("CGameRulesSpawningBase::SvOnPlayerKilled()");
	
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesSpawningBase::SvOnPlayerKilled() yet NOT a server");

	IEntity *targetEntity = gEnv->pEntitySystem->GetEntity(hitInfo.targetId);

	if (targetEntity)
	{
		TPlayerDataMap::iterator it = m_playerValues.find(hitInfo.targetId);
		if (it != m_playerValues.end())
		{
			it->second.deathTime = GetTime();
			it->second.deathPos = targetEntity->GetWorldPos();
			it->second.lastKillerId = hitInfo.shooterId;
		}
		else
		{
			CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesSpawningBase::SvOnPlayerKilled() failed to find a playervalue for player %s. This should not happen", targetEntity->GetName()));
		}
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesSpawningBase::SvOnPlayerKilled() failed to find a targetEntity for player that died");
	}
}

void CGameRulesSpawningBase::ClRequestRevive(EntityId playerId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::ClRequestRevive() is NOT implemented and shouldn't be called until it has been"));
}

void CGameRulesSpawningBase::SvRequestRevive(EntityId playerId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::SvRequestRevive() is NOT implemented and shouldn't be called until it has been"));
}

void CGameRulesSpawningBase::PerformRevive(EntityId playerId, int teamId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::PerformRevive() is NOT implemented and shouldn't be called until it has been"));
}
	
void CGameRulesSpawningBase::HandleOptOutOfSpawn(EntityId entityId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::HandleOptOutOfSpawn() is NOT implemented and shouldn't be called until it has been. Shouldn't be needed in singleplayer"));
}

void CGameRulesSpawningBase::SvRecordReviveOptOut(EntityId entityId)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::SvRecordReviveOptOut() is NOT implemented and shouldn't be called until it has been. Shouldn't be needed in singleplayer"));
}

void CGameRulesSpawningBase::ClAutoReviveOptOut()
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::ClAutoReviveOptOut() is NOT implemented and shouldn't be called until it has been. Shouldn't be needed in singleplayer"));
}

void CGameRulesSpawningBase::ReviveAllPlayers(bool isReset)
{
	CRY_ASSERT_TRACE(0, ("CGameRulesSpawningBase::ReviveAllPlayers() is NOT implemented and shouldn't be called until it has been. Shouldn't be needed in singleplayer"));
}
