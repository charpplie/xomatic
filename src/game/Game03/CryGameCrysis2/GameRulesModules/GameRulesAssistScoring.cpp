/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
Game rules module to handle scoring points values
-------------------------------------------------------------------------
History:
- 14:09:2009  : Created by James Bamford

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesAssistScoring.h"
#include "GameRules.h"
#include "GameRulesTypes.h"
#include "IGameRulesScoringModule.h"
#include "IGameRulesPlayerStatsModule.h"
#include "Player.h"

CGameRulesAssistScoring::CGameRulesAssistScoring()
{
	m_pGameRules = NULL;
	m_maxTimeBetweenAttackAndKillForAssist = 0.0f;
}

CGameRulesAssistScoring::~CGameRulesAssistScoring()
{

}

void CGameRulesAssistScoring::Init(XmlNodeRef xml)
{
	m_pGameRules = g_pGame->GetGameRules();

	if (!xml->getAttr("maxTimeBetweenAttackAndKillForAssist", m_maxTimeBetweenAttackAndKillForAssist))
	{
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpawning failed to find valid maxTimeBetweenAttackAndKillForAssist param");
	}
}

void CGameRulesAssistScoring::SvPlayerJoined(EntityId playerId)
{
	CryLog("CGameRulesAssistScoring::SvPlayerJoined()");
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesAssistScoring::SvPlayerJoined() but NOT the server");
	m_playerAttackers.insert(TPlayerAttackers::value_type(playerId, TAttackers()));
}

void CGameRulesAssistScoring::SvPlayerLeft(EntityId playerId)
{
	CryLog("CGameRulesAssistScoring::SvPlayerLeft()");
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesAssistScoring::SvPlayerLeft() but NOT the server");
	stl::member_find_and_erase(m_playerAttackers, playerId);
}

void CGameRulesAssistScoring::SvOnPlayerHit(const HitInfo &info)
{
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesAssistScoring::SvOnPlayerHit() but NOT the server");

	// Cheap early checks for being injured by self or injured anonymously... do this before looking up team
	if (info.shooterId && info.shooterId != info.targetId)
	{
		int shooterTeamId = m_pGameRules->GetTeam(info.shooterId);

		// Only need to look up target's team if shooter is on a team
		if (shooterTeamId == 0 || shooterTeamId != m_pGameRules->GetTeam(info.targetId))
		{
			CryLog("CGameRulesAssistScoring::SvOnPlayerHit() - Entity %d '%s' on team %d injured enemy entity %d '%s'", info.shooterId, m_pGameRules->GetEntityName(info.shooterId), shooterTeamId, info.targetId, m_pGameRules->GetEntityName(info.targetId));

			TPlayerAttackers::iterator it=m_playerAttackers.find(info.targetId);
			if (it==m_playerAttackers.end())
			{
				CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesAssistScoring::SvOnPlayerHit() failed to find a playerAttacker element for target %s", m_pGameRules->GetEntityName(info.targetId)));
			}
			else
			{
				bool existingAttacker=false;

				for (TAttackers::iterator vit=it->second.begin(); vit!=it->second.end(); ++vit)
				{
					SAttackerData &data = *vit;
					if (data.m_entityId == info.shooterId)
					{
						data.m_time = m_pGameRules->GetServerTime();
						existingAttacker = true;
						CryLog("CGameRulesAssistScoring::SvOnPlayerHit() has found an existing attacker %s who's hit this player %s. Updating to new time %f", m_pGameRules->GetEntityName(info.shooterId), m_pGameRules->GetEntityName(info.targetId), data.m_time);
						break;
					}
				}

				if (!existingAttacker)
				{
					SAttackerData newAttacker(info.shooterId, m_pGameRules->GetServerTime());
					it->second.push_back(newAttacker);
					CryLog("CGameRulesAssistScoring::SvOnPlayerHit() has found a new attacker %s who's hit this player %s. Saving time %f",  m_pGameRules->GetEntityName(info.shooterId), m_pGameRules->GetEntityName(info.targetId), newAttacker.m_time);
				}
			}
		}
	}
}

//void CGameRulesAssistScoring::ClAwardAssistKillPoints(EntityId victimId)
//{
//	IGameRulesScoringModule *scoringModule = m_pGameRules->GetScoringModule();
//	int playerPoints=scoringModule->GetPlayerPointsByName("assistKill");
//	CryLog("CGameRulesAssistScoring::ClAwardAssistKillPoints()");
//	m_pGameRules->IncreasePoints(g_pGame->GetIGameFramework()->GetClientActorId(), SGameRulesScoreInfo(EGRST_PlayerKillAssist, playerPoints).AttachVictim(victimId));
//}

void CGameRulesAssistScoring::SvDoScoringForDeath(IActor *pActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type)
{
	CryLog("CGameRulesAssistScoring::SvDoScoringForDeath()");
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesAssistScoring::SvDoScoringForDeath() should only be called on the server");

	TPlayerAttackers::iterator it=m_playerAttackers.find(pActor->GetEntityId());

	if (it==m_playerAttackers.end())
	{
		CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesAssistScoring::SvDoScoringForDeath() failed to find a playerAttacker element for target %s", m_pGameRules->GetEntityName(pActor->GetEntityId())));
	}
	else
	{
		float time=m_pGameRules->GetServerTime();

		for (TAttackers::iterator vit=it->second.begin(); vit!=it->second.end(); ++vit)
		{
			SAttackerData &data = *vit;
			CRY_ASSERT_MESSAGE(data.m_entityId, string().Format("List of %s's recent attackers includes entity ID 0! Why has a NULL entity been allocated a slot in the list?", pActor->GetEntity()->GetName()));
			if (time - data.m_time < m_maxTimeBetweenAttackAndKillForAssist)
			{
				CryLog("CGameRulesAssistScoring::SvDoScoringForDeath() found entity %d %s has attacked the dead player %s recently enough to be considered for assist", data.m_entityId, m_pGameRules->GetEntityName(data.m_entityId), m_pGameRules->GetEntityName(pActor->GetEntityId()));
				if (shooterId == data.m_entityId)
				{
					CryLog("CGameRulesAssistScoring::SvDoScoringForDeath() found the attacking player %s actually killed the player %s, NOT considering them for assist bonus", m_pGameRules->GetEntityName(data.m_entityId), m_pGameRules->GetEntityName(pActor->GetEntityId()));
				}
				else
				{
					CryLog("CGameRulesAssistScoring::SvDoScoringForDeath() has found player %s has assisted in killing player %s, awarding assist points", m_pGameRules->GetEntityName(data.m_entityId), m_pGameRules->GetEntityName(pActor->GetEntityId()));
					IGameRulesScoringModule *scoringModule = m_pGameRules->GetScoringModule();
					CRY_ASSERT_MESSAGE(scoringModule, "CGameRulesAssistScoring::SvDoScoringForDeath() failed to find a scoring module, this should NOT happen");
					if (scoringModule)
					{
						TGameRulesScoreInt playerPoints = scoringModule->GetPlayerPointsByType(EGRST_PlayerKillAssist);
						EGameRulesScoreType type = EGRST_PlayerKillAssist;
						if(pActor->IsPlayer())
						{
							CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
							if(pPlayer->GetPerkData<bool>(EPD_SelfDestructMinigame))
							{
								type = EGRST_SelfDestructPrevention;
							}
						}

						IGameRulesPlayerStatsModule *pPlayerStatsModule = m_pGameRules->GetPlayerStatsModule();
						if (pPlayerStatsModule)
						{
							pPlayerStatsModule->IncrementAssistKills(data.m_entityId);
						}

						m_pGameRules->IncreasePoints(data.m_entityId, SGameRulesScoreInfo(type, playerPoints).AttachVictim(pActor->GetEntityId()));
					}
				}
			}
		}

		it->second.clear();
	}
}

EntityId CGameRulesAssistScoring::SvGetMostRecentAttacker(EntityId targetId)
{
	CryLog("CGameRulesAssistScoring::SvGetMostRecentAttacker");
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CGameRulesAssistScoring::SvGetMostRecentAttacker should only be called on the server");

	TPlayerAttackers::iterator it=m_playerAttackers.find(targetId);

	if (it==m_playerAttackers.end())
	{
		CRY_ASSERT_MESSAGE(0, string().Format("CGameRulesAssistScoring::SvGetMostRecentAttacker failed to find a playerAttacker element for target %s", m_pGameRules->GetEntityName(targetId)));
	}
	else
	{
		float time=m_pGameRules->GetServerTime();

		for (TAttackers::iterator vit=it->second.begin(); vit!=it->second.end(); ++vit)
		{
			SAttackerData &data = *vit;
			CRY_ASSERT_MESSAGE(data.m_entityId, string().Format("List of %s's recent attackers includes entity ID 0! Why has a NULL entity been allocated a slot in the list?", m_pGameRules->GetEntityName(targetId)));
			if (time - data.m_time < m_maxTimeBetweenAttackAndKillForAssist)
			{
				CryLog("CGameRulesAssistScoring::SvGetMostRecentAttacker found entity %d %s has attacked the dead player %s recently enough to be considered for assist", data.m_entityId, m_pGameRules->GetEntityName(data.m_entityId), m_pGameRules->GetEntityName(targetId));
				if (targetId == data.m_entityId)
				{
					CryLog("CGameRulesAssistScoring::SvGetMostRecentAttacker found the attacking player %s actually killed the player %s, NOT considering them for Recent attacker", m_pGameRules->GetEntityName(data.m_entityId), m_pGameRules->GetEntityName(targetId));
				}
				else
				{
					CryLog("CGameRulesAssistScoring::SvGetMostRecentAttacker has found player %s has assisted in killing player %s, awarding assist points", m_pGameRules->GetEntityName(data.m_entityId), m_pGameRules->GetEntityName(targetId));
					return data.m_entityId;
				}
			}
		}
	}
	return 0;
}

void CGameRulesAssistScoring::SvBecomeServer()
{
	m_playerAttackers.clear();

	IEntityClass *pPlayerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	IEntityIt *pEntityIt = gEnv->pEntitySystem->GetEntityIterator();
	pEntityIt->MoveFirst();

	// This is somewhat slow however it occurs at a point where CGameRules::GetPlayers() won't work (due to not having any channels!)
	while (IEntity *pEntity = pEntityIt->Next())
	{
		if (pEntity->GetClass() == pPlayerClass)
		{
			SvPlayerJoined(pEntity->GetId());
		}
	}
}
