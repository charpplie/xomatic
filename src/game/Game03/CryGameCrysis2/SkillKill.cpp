/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
static skill kill checks 
- 02:02:2010		Created by Ben Parbury
*************************************************************************/

#include "StdAfx.h"
#include "SkillKill.h"

#include "Player.h"
#include "GameRules.h"
#include "PersistantStats.h"

static bool s_firstBlood = true;

bool SkillKill::IsFirstBlood()
{
	if(s_firstBlood)
	{
		s_firstBlood = false;
		return true;
	}
	return false;
}

void SkillKill::Reset()
{
	s_firstBlood = true;
}

bool SkillKill::IsHeadshot(CPlayer* pTargetPlayer, int hit_bodyPart)
{
	HitInfo hitInfo;
	hitInfo.partId = hit_bodyPart;
	return pTargetPlayer->IsHeadShot(hitInfo);
}

bool SkillKill::IsAirDeath(CPlayer* pTargetPlayer)
{
	return pTargetPlayer->GetActorStats()->inAir > 0.5f;
}

bool SkillKill::IsMeleeTakedown(CGameRules* pGameRules, int hit_type)
{
	static const int hitMelee = pGameRules->GetHitTypeId("melee");
	return hit_type == hitMelee;
}

bool SkillKill::IsImpale(CGameRules* pGameRules, int hit_type)
{
	static const int hitVS = pGameRules->GetHitTypeId("vspike");
	return hit_type == hitVS;
}

bool SkillKill::IsPistolKill(const char* weaponClassName)
{
	return strcmp(weaponClassName, "Revolver") == 0 || strcmp(weaponClassName, "RazorFive") == 0;
}

bool SkillKill::IsBlindKill(CPlayer* pShooterPlayer)
{
	const SPlayerStats * pPlayerStats = static_cast<const SPlayerStats*>(pShooterPlayer->GetActorStats());
	return pPlayerStats->flashBangStunMult != 1.0f;
}

bool SkillKill::IsRumbled(CPlayer* pTargetPlayer)
{
	return pTargetPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Stealth;
}

bool SkillKill::IsUnderwaterKill(CPlayer* pShooterPlayer)
{
	SActorStats* pActorStats = pShooterPlayer->GetActorStats();
	return pActorStats->inWaterTimer > 0.0f;
}

bool SkillKill::IsRoadKill(CGameRules* pGameRules, int hit_type)
{
	static const int hitCollision = pGameRules->GetHitTypeId("collision");
	return hit_type == hitCollision;
}

bool SkillKill::IsNearDeathExperience(CPlayer* pShooterPlayer)
{
	const static int k_healthFractionForNearDeathExperience = 5; //20% or less health
	return pShooterPlayer->GetHealth() < (pShooterPlayer->GetMaxHealth() / k_healthFractionForNearDeathExperience);
}

bool SkillKill::IsKillJoy(CPlayer* pTargetPlayer)
{
	return g_pGame->GetPersistantStats()->GetCurrentStat(pTargetPlayer->GetEntityId(), ESIPS_Kills) > 2;
}