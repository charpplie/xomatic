/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
static skill kill checks 
- 02:02:2010		Created by Ben Parbury
*************************************************************************/

#ifndef __SKILLKILL_H__
#define __SKILLKILL_H__

class CPlayer;
class CGameRules;

class SkillKill
{
public:
	static bool IsHeadshot(CPlayer* pTargetPlayer, int hit_joint);
	static bool IsAirDeath(CPlayer* pTargetPlayer);
	static bool IsMeleeTakedown(CGameRules* pGameRules, int hit_type);
	static bool IsImpale(CGameRules* pGameRules, int hit_type);
	static bool IsPistolKill(const char* weaponClassName);
	static bool IsBlindKill(CPlayer* pShooterPlayer);
	static bool IsFirstBlood();
	static bool IsRumbled(CPlayer* pTargetPlayer);
	static bool IsUnderwaterKill(CPlayer* pShooterPlayer);
	static bool IsRoadKill(CGameRules* pGameRules, int hit_type);
	static bool IsNearDeathExperience(CPlayer* pShooterPlayer);
	static bool IsKillJoy(CPlayer* pTargetPlayer);

	static void Reset();
};

#endif
