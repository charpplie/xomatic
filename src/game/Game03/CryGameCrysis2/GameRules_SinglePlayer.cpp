/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
		- 18:6:2009 : Created by Colin Gulliver

*************************************************************************/
#include "StdAfx.h"
#include "GameRules_SinglePlayer.h"
#include "Player.h"
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"

CGameRules_SinglePlayer::CGameRules_SinglePlayer(void)
{
	CryLogAlways("SP constructor");
}

CGameRules_SinglePlayer::~CGameRules_SinglePlayer(void)
{
}

bool CGameRules_SinglePlayer::Init( IGameObject * pGameObject )
{
	if (!CGameRules::Init(pGameObject))
		return false;

	RegisterHitMaterial("mat_helmet");
	RegisterHitMaterial("mat_kevlar");
	RegisterHitMaterial("mat_head");
	RegisterHitMaterial("mat_torso");
	RegisterHitMaterial("mat_arm_left");
	RegisterHitMaterial("mat_arm_right");
	RegisterHitMaterial("mat_hand_left");
	RegisterHitMaterial("mat_hand_right");
	RegisterHitMaterial("mat_leg_left");
	RegisterHitMaterial("mat_leg_right");
	RegisterHitMaterial("mat_foot_left");
	RegisterHitMaterial("mat_foot_right");

	//aliens		
	RegisterHitMaterial("mat_alien_vulnerable");
	RegisterHitMaterial("mat_alien_hunter_leg");
	RegisterHitMaterial("mat_alien_hunter_torso");
	RegisterHitMaterial("mat_alien_hunter_head");
	RegisterHitMaterial("mat_alien_hunter_vulnerable");
	RegisterHitMaterial("mat_alien_forcefield");

	RegisterHitMaterial("mat_alien_hunter_topFace");
	RegisterHitMaterial("mat_alien_hunter_bottomFace");
	RegisterHitMaterial("mat_alien_hunter_leftFace");
	RegisterHitMaterial("mat_alien_hunter_rightFace");


	RegisterHitType("normal");
	RegisterHitType("repair");
	RegisterHitType("lockpick");
	RegisterHitType("bullet");
	RegisterHitType("gaussbullet");
	RegisterHitType("frost");
	RegisterHitType("fire");
	RegisterHitType("radiation");
	RegisterHitType("melee");
	RegisterHitType("silentMelee");
	RegisterHitType("tac");
	RegisterHitType("frag");
	RegisterHitType("fall");
	RegisterHitType("collision");
	RegisterHitType("event");
	RegisterHitType("punish");
	RegisterHitType("avmine");
	RegisterHitType("moacbullet");
	RegisterHitType("trooper_melee");
	RegisterHitType("scout_moac");
	RegisterHitType("aacannon");
	RegisterHitType("emp");
	RegisterHitType("pingerPing");
	RegisterHitType("vspike");
	RegisterHitType("kvolt");
	RegisterHitType("impulse_hit");
	RegisterHitType("mike_burn");
	RegisterHitType("gib");

	return true;
}

void CGameRules_SinglePlayer::SvOnClientConnect( int channelId, bool reset, const char *name )
{
	Vec3 pos(0.f, 0.f, 0.f);
	Ang3 angles(0.f, 0.f, 0.f);

	EntityId locationId = GetFirstSpawnLocation();
	IEntity *location(0);
	if (locationId)
	{
		location = gEnv->pEntitySystem->GetEntity(locationId);
		if (location)
		{
			pos = location->GetWorldPos();
			angles = location->GetWorldAngles();
		}
	}

	IActor *player = SpawnPlayer(channelId, "Dude", "Player", pos, angles);

	if (location)
	{
		IScriptTable *pScript = location->GetScriptTable();
		if (pScript && pScript->GetValueType("Spawned") == svtFunction)
		{
			IScriptTable *pPlayerScript = player->GetEntity()->GetScriptTable();

			m_pScriptSystem->BeginCall(pScript, name);
			m_pScriptSystem->PushFuncParam(pScript);
			m_pScriptSystem->PushFuncParam(pPlayerScript);
			m_pScriptSystem->EndCall();	
		}
	}
}