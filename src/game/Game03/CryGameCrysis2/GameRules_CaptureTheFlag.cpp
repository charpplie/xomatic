/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
		- 25:6:2009 : Created by Colin Gulliver

*************************************************************************/
#include "StdAfx.h"
#include "GameRules_CaptureTheFlag.h"
#include "IEntity.h"
#include "ISound.h"
#include "Player.h"
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"

#define ASSERT_IS_CLIENT if (!gEnv->bClient) { assert(gEnv->bClient); return; }
#define ASSERT_IS_SERVER if (!gEnv->bServer) { assert(gEnv->bServer); return; }
#define ASSERT_IS_SERVER_RETURN_FALSE if (!gEnv->bServer) { assert(gEnv->bServer); return false; }

#define FLAG_RESET_TIMER_LENGTH				30000.f

#define NUM_POINTS_KILL						5
#define NUM_POINTS_KILL_ASSIST				2
#define NUM_POINTS_FLAG_CAPTURED			15
#define NUM_POINTS_FLAG_CAPTURED_ALONE		5
#define NUM_POINTS_FLAG_RETRIEVED			5
#define NUM_POINTS_FLAG_DEFENDED			2
#define NUM_POINTS_FLAG_TAKEN				2
#define NUM_POINTS_FLAG_CARRIER_PROTECTED	2
#define NUM_POINTS_FLAG_CARRIER_RETRIEVED	5
#define NUM_POINTS_FLAG_CARRIER_KILLED		2
#define NUM_POINTS_FLAG_ASSISTED_CAPTURE	5

#define DEFEND_FLAG_RADIUS_SQR				100.f
#define MAX_TIME_BETWEEN_FLAG_HIT_AND_KILL_FOR_PROTECT		10000.f

CGameRules_CaptureTheFlag::CGameRules_CaptureTheFlag(void)
{
	CryLogAlways("CTF constructor");
}

CGameRules_CaptureTheFlag::~CGameRules_CaptureTheFlag(void)
{
}

bool CGameRules_CaptureTheFlag::Init( IGameObject * pGameObject )
{
	if (!CGameRules_TeamInstantAction::Init(pGameObject))
		return false;

	m_clientStateSet = false;

	m_gameRulesType = EGR_CAPTURETHEFLAG;

	return true;
}

void CGameRules_CaptureTheFlag::SvRevivePlayer(EntityId playerId)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	if (pActor)
	{
		//TODO: This is temp code, automatically select a team since we can't do it through the non-existent HUD!
		int teamId = GetTeam(playerId);
		if (teamId == 0)
		{
			if (GetTeamPlayerCount(1, false) > GetTeamPlayerCount(2, false))
			{
				teamId = 2;
			}
			else
			{
				teamId = 1;
			}
			SetTeam(teamId, playerId);
			CryLogAlways("Auto selected team %i for player (%s)", teamId, pActor->GetEntity()->GetName());
		}

#if 0
		Vec3 deathPos(ZERO);
		TPlayerDataMap::iterator it = m_playerValues.find(playerId);
		if (it != m_playerValues.end())
		{
			deathPos = it->second.deathPos;
		}
#endif
		float zOffset=0.0f;

		//TODO: Select a spawn location using Jim's new spawning code
		EntityId locationId = GetSpawnLocationNew(playerId, zOffset);
		if (locationId)
		{
			IEntity *location = gEnv->pEntitySystem->GetEntity(locationId);
			if (location)
			{
				Vec3 pos = location->GetWorldPos();
				Ang3 angles = location->GetWorldAngles();

				pos.z += zOffset;

				CryLogAlways("Reviving player %i", playerId);
				RevivePlayer(pActor, pos, angles, 0, true);
			}
			else
			{
				CryLogAlways("Failed to find spawn location");
			}
		}
		else
		{
			CryLogAlways("Failed to find spawn location");
		}
	}
}

void CGameRules_CaptureTheFlag::SvOnKill( EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type )
{
	CryLogAlways("CGameRules_InstantAction::SvOnKill");
	ASSERT_IS_SERVER;

	CGameRules_TeamInstantAction::SvOnKill(targetId, shooterId, weaponClassName, damage, material, hit_type);

	SvCheckForDeadFlagCarrier(targetId);
}

void CGameRules_CaptureTheFlag::SvOnStartGame()
{
	CGameRules_TeamInstantAction::SvOnStartGame();

	InitEntities();
	GatherEntities();
}

void CGameRules_CaptureTheFlag::SvRestartGame( bool forceInGame )
{
	CGameRules_InstantAction::SvRestartGame(forceInGame);

	SvReset();
}

void CGameRules_CaptureTheFlag::SvGotoState( EGR_GameState newState )
{
	CGameRules_TeamInstantAction::SvGotoState(newState);

	if (newState == EGR_InGame)
	{
		SvReset();
	}
}

void CGameRules_CaptureTheFlag::SvOnChangeTeam( EntityId playerId, int teamId )
{
	ASSERT_IS_SERVER;

	if (teamId != GetTeam(playerId))
	{
		CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
		if (pActor)
		{
			SvCheckForDeadFlagCarrier(playerId);

			CGameRules_TeamInstantAction::SvOnChangeTeam(playerId, teamId);
		}
	}
}

void CGameRules_CaptureTheFlag::SvOnClientDisconnect( int channelId )
{
	if (IActor *pActor = GetActorByChannelId(channelId))
	{
		SvCheckForDeadFlagCarrier(pActor->GetEntityId());
	}
	CGameRules_TeamInstantAction::SvOnClientDisconnect(channelId);
}

void CGameRules_CaptureTheFlag::SvCheckForDeadFlagCarrier(EntityId playerId)
{
	CryLogAlways("CGameRules_CaptureTheFlag::SvCheckForDeadFlagCarrier(EntityId playerId=%i)", playerId);
	for (int teamId = 1; teamId < 3; ++ teamId)
	{
		STeamInfo *teamInfo = GetTeamInfo(teamId);
		if (teamInfo->flagState == ECTF_FLAG_STATE_CARRIED && teamInfo->carrierId == playerId)
		{
			CryLogAlways("Player is flag carrier for team %i", teamId);
			SvFlagDropped(teamId, playerId);
		}
	}
}

void CGameRules_CaptureTheFlag::SvResetScore( EntityId playerId )
{
	CGameRules_TeamInstantAction::SvResetScore(playerId);

	SetSynchedEntityValue(playerId, ECTF_KEY_FLAGS, 0);
}

void CGameRules_CaptureTheFlag::SvOnHit( const HitInfo *hit )
{
	CGameRules_TeamInstantAction::SvOnHit(hit);

	if (hit->targetId != hit->shooterId)
	{
		CActor *pTarget = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hit->targetId));
		CActor *pShooter = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hit->shooterId));

		// If both target and shooter are players, record the hit time for use calculating assist points
		if (pTarget && pShooter)
		{
			int targetTeamId = GetTeam(hit->targetId);
			int shooterTeamId = GetTeam(hit->shooterId);
			if (targetTeamId != shooterTeamId)
			{
				STeamInfo *teamInfo = GetTeamInfo(shooterTeamId);
				if ((teamInfo->flagState == ECTF_FLAG_STATE_CARRIED) && (teamInfo->carrierId == hit->targetId))
				{
					TCTFPlayerDataMap::iterator it = m_ctfPlayerValues.find(hit->shooterId);
					if (it != m_ctfPlayerValues.end())
					{
						it->second.timeLastHitFlagCarrier = GetServerTime();
					}
				}
			}
		}
	}
}

void CGameRules_CaptureTheFlag::SvSetupPlayer( EntityId playerId )
{
	CGameRules_TeamInstantAction::SvSetupPlayer(playerId);

	m_ctfPlayerValues.insert(TCTFPlayerDataMap::value_type(playerId, SCTFPlayerData()));
}

void CGameRules_CaptureTheFlag::SvCleanUpPlayer( EntityId playerId )
{
	CGameRules_TeamInstantAction::SvCleanUpPlayer(playerId);

	TCTFPlayerDataMap::iterator it = m_ctfPlayerValues.find(playerId);
	if (it != m_ctfPlayerValues.end())
	{
		m_ctfPlayerValues.erase(it);
	}
}

void CGameRules_CaptureTheFlag::ClOnSetTeam( EntityId entityId, int teamId )
{
	CGameRules_TeamInstantAction::ClOnSetTeam(entityId, teamId);

	if (g_pGame->GetIGameFramework()->GetClientActorId() == entityId)
	{
		ClRethinkMusic();
	}
	else
	{
		if (!gEnv->bServer && teamId)
		{
			IEntityClass *pFlagClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFFlag");
			IEntityClass *pBaseClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFBase");
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
			if (pEntity)
			{
				if (pEntity->GetClass() == pFlagClass)
				{
					CryLogAlways("CTFFlag (%s) set to team %i", pEntity->GetName(), teamId);
					GetTeamInfo(teamId)->flagId = entityId;
					CallEntityScriptFunction(entityId, "LoadCorrectGeometry");
				}
				else if (pEntity->GetClass() == pBaseClass)
				{
					CryLogAlways("CTFBase (%s) set to team %i", pEntity->GetName(), teamId);
					GetTeamInfo(teamId)->baseId = entityId;
				}
			}
			ClSetEntities();
		}
	}
}

void CGameRules_CaptureTheFlag::ClOnStartGame()
{
	CGameRules_TeamInstantAction::ClOnStartGame();

	if (!gEnv->bServer)
	{
		GatherEntities();

		if (m_clientStateSet)
		{
			ClSetEntities();
		}
	}
}

void CGameRules_CaptureTheFlag::InitEntities()
{
	CryLogAlways("CGameRules_CaptureTheFlag::InitEntities()");

	IEntityClass *pBaseClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFBase");
	IEntityClass *pFlagClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFFlag");

	if (pBaseClass && pFlagClass)
	{
		IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
		IEntity *pEntity = 0;

		pIt->MoveFirst();

		while(pEntity = pIt->Next())
		{
			if (pEntity->GetClass() == pBaseClass)
			{
				IScriptTable *pEntityScript = pEntity->GetScriptTable();
				if (pEntityScript)
				{
					if (pEntityScript->GetValueType("Properties") == svtObject)
					{
						SmartScriptTable pPropertiesScript;
						// IScriptTable *pPropertiesScript;
						if (pEntityScript->GetValue("Properties", pPropertiesScript))
						{
							const char *teamName;
							if (pPropertiesScript->GetValue("teamName", teamName))
							{
								int teamId = GetTeamId(teamName);
								if (teamId == 1 || teamId == 2)
								{
									CryLogAlways("Found base (%s on team %i), spawning flag", pEntity->GetName(), teamId);

									SetTeam(teamId, pEntity->GetId());

									char sName[10];
									sprintf(sName, "flag%i", teamId);

									SEntitySpawnParams params;
									params.pClass = pFlagClass;
									params.sName = sName;
									params.vPosition = pEntity->GetWorldPos();

									IEntity *pFlag = gEnv->pEntitySystem->SpawnEntity(params, true);
									if (pFlag)
									{
										SetTeam(teamId, pFlag->GetId());

										gEnv->pEntitySystem->AddEntityEventListener(pFlag->GetId(), ENTITY_EVENT_ENTERAREA, this);
										gEnv->pEntitySystem->AddEntityEventListener(pFlag->GetId(), ENTITY_EVENT_LEAVEAREA, this);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void CGameRules_CaptureTheFlag::GatherEntities()
{
	CryLogAlways("CGameRules_CaptureTheFlag::GatherEntities()");

	IEntityClass *pBaseClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFBase");
	IEntityClass *pFlagClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFFlag");

	if (pBaseClass && pFlagClass)
	{
		IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
		IEntity *pEntity = 0;

		pIt->MoveFirst();

		while(pEntity = pIt->Next())
		{
			if (pEntity->GetClass() == pBaseClass)
			{
				int teamId = GetTeam(pEntity->GetId());
				if (teamId == 1 || teamId == 2)
				{
					GetTeamInfo(teamId)->baseId = pEntity->GetId();
					CryLogAlways("Found base (%s) for team %i", pEntity->GetName(), teamId);
				}
			}
			else if (pEntity->GetClass() == pFlagClass)
			{
				int teamId = GetTeam(pEntity->GetId());
				if (teamId == 1 || teamId == 2)
				{
					GetTeamInfo(teamId)->flagId = pEntity->GetId();
					CallEntityScriptFunction(pEntity->GetId(), "LoadCorrectGeometry");
					CryLogAlways("Found flag (%s) for team %i", pEntity->GetName(), teamId);
				}
			}
		}
	}
}

CGameRules_CaptureTheFlag::STeamInfo * CGameRules_CaptureTheFlag::GetTeamInfo( int teamId )
{
	if (teamId == 1 || teamId == 2)
	{
		return &m_teamInfo[teamId - 1];
	}
	return 0;
}

void CGameRules_CaptureTheFlag::OnEntityEvent( IEntity *pEntity,SEntityEvent &event )
{
	EntityId insideId = (EntityId) event.nParam[0];
	int flagTeamId = GetTeam(pEntity->GetId());

	if (event.event == ENTITY_EVENT_ENTERAREA)
	{
		SvEntityEnterFlagArea(flagTeamId, insideId);
	}
	else if (event.event == ENTITY_EVENT_LEAVEAREA)
	{
		SvEntityLeaveFlagArea(flagTeamId, insideId);
	}
}

void CGameRules_CaptureTheFlag::ClFlagRetrieved( int teamId, EntityId playerId )
{
	CryLogAlways("CGameRules_CaptureTheFlag::ClFlagRetrieved( int teamId=%i, EntityId playerId=%i )", teamId, playerId);
	if (m_pGameFramework->GetClientActorId() == playerId)
	{
		// Local player retrieved the flag
		//TODO: Display message and points
	}

	ClFlagReset(teamId);
}

void CGameRules_CaptureTheFlag::ResetFlag( int teamId )
{
	STeamInfo *teamInfo = GetTeamInfo(teamId);
	IEntity *pFlag = gEnv->pEntitySystem->GetEntity(teamInfo->flagId);

	if (teamInfo && pFlag)
	{
		if (teamInfo->carrierId)
		{
			CallEntityScriptFunction(teamInfo->flagId, "AttachTo");
			teamInfo->carrierId	= 0;
		}
		SetFlagState(teamId, ECTF_FLAG_STATE_INBASE);
		teamInfo->previousCarriers.clear();

		IEntity *pBase = gEnv->pEntitySystem->GetEntity(teamInfo->baseId);
		if (pBase)
		{
			pFlag->SetPos(pBase->GetPos());
			CallEntityScriptFunction(teamInfo->baseId, "FlagReturned");
		}

		//TODO: Recolour the flag (if we're a client)
	}
}

void CGameRules_CaptureTheFlag::ClFlagReset( int teamId )
{
	CryLogAlways("CGameRules_CaptureTheFlag::ClFlagReset( int teamId=%i )", teamId);

	STeamInfo *teamInfo = GetTeamInfo(teamId);
	IEntity *pFlag = gEnv->pEntitySystem->GetEntity(teamInfo->flagId);
	if (!pFlag)
		return;

	ClPlaySound("Sounds/ctf:ctf:flag_teleport_out", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);

	if (!gEnv->bServer)
	{
		ResetFlag(teamId);
	}

	ClPlaySound("Sounds/ctf:ctf:flag_teleport_in", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
	ClRethinkMusic();

	if (GetTeam(m_pGameFramework->GetClientActorId()) == teamId)
	{
		//TODO: Display message - our flag has been reset
	}
	else
	{
		//TODO: Display message - enemy flag has been reset
	}
}

void CGameRules_CaptureTheFlag::SvFlagCompleted( int teamId, EntityId playerId )
{
	ResetFlag(3 - teamId);

	STeamInfo *teamInfo = GetTeamInfo(teamId);
	CallEntityScriptFunction(teamInfo->baseId, "TeamScored");
}

void CGameRules_CaptureTheFlag::ClFlagCompleted( int teamId, EntityId playerId )
{
	STeamInfo *teamInfo = GetTeamInfo(teamId);
	IEntity *pFlag = gEnv->pEntitySystem->GetEntity(teamInfo->flagId);
	if (!pFlag)
		return;

	if (m_pGameFramework->GetClientActorId() == playerId)
	{
		//TODO: Display message - local player scored
		ClPlaySound("Sounds/ctf:ctf:player_team_score", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
	}
	else if (GetTeam(m_pGameFramework->GetClientActorId()) == teamId)
	{
		//TODO: Display message - teammate scored
		ClPlaySound("Sounds/ctf:ctf:player_team_score", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
	}
	else
	{
		//TODO: Display message - enemy scored
		ClPlaySound("Sounds/ctf:ctf:enemy_team_score", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
	}

	ClPlaySound("Sounds/ctf:ctf:flag_teleport_out", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
	if (!gEnv->bServer)
	{
		ResetFlag(3 - teamId);
		CallEntityScriptFunction(teamInfo->baseId, "TeamScored");
	}

	ClRethinkMusic();
	ClPlaySound("Sounds/ctf:ctf:flag_teleport_in", pFlag->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
}

void CGameRules_CaptureTheFlag::SvFlagDropped( int teamId, EntityId playerId )
{
	CryLogAlways("CGameRules_CaptureTheFlag::SvFlagDropped( int teamId=%i, EntityId playerId=%i )", teamId, playerId);
	STeamInfo *teamInfo = GetTeamInfo(teamId);
	IEntity *pFlag = gEnv->pEntitySystem->GetEntity(teamInfo->flagId);
	if (!pFlag)
		return;
	Vec3 flagPos = pFlag->GetWorldPos();

	CallEntityScriptFunction(teamInfo->flagId, "AttachTo");

	pFlag->SetPos(flagPos);
	SetFlagState(teamId, ECTF_FLAG_STATE_DROPPED);
	
	EntityTeamPosParams params(playerId, teamId, flagPos);
	GetGameObject()->InvokeRMI(ClOnFlagDropped(), params, eRMI_ToAllClients);

	teamInfo->flagDropTime = GetServerTime();

	EntityId closestId = 0;
	float closestDist = 1000000.f;

	const Vec3 &droppedFlagPos = pFlag->GetPos();

	std::vector<EntityId>::iterator it = teamInfo->insideIds.begin();
	for (; it != teamInfo->insideIds.end(); ++it)
	{
		CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*it));
		if (pActor && (pActor->GetHealth() > 0) && (pActor->GetSpectatorMode() == CActor::eASM_None))
		{
			float distSqr = droppedFlagPos.GetSquaredDistance(pActor->GetEntity()->GetPos());
			if (distSqr < closestDist)
			{
				closestDist = distSqr;
				closestId = *it;
			}
		}
	}
	if (closestId)
	{
		SvEntityEnterFlagArea(teamId, closestId);
	}
}

void CGameRules_CaptureTheFlag::ClFlagDropped( int teamId, EntityId playerId, const Vec3 &flagPos )
{
	STeamInfo *teamInfo = GetTeamInfo(teamId);
	IEntity *pFlag = gEnv->pEntitySystem->GetEntity(teamInfo->flagId);
	if (!pFlag || !teamInfo)
		return;

	if (!gEnv->bServer)
	{
		CallEntityScriptFunction(teamInfo->flagId, "AttachTo");

		pFlag->SetPos(flagPos);
		SetFlagState(teamId, ECTF_FLAG_STATE_DROPPED);
	}
	ClRethinkMusic();
	//TODO: Recolour flag

	if (GetTeam(m_pGameFramework->GetClientActorId()) == teamId)
	{
		//TODO: Display message - our teammate has dropped the flag
	}
	else
	{
		//TODO: Display message - enemy has dropped the flag
	}
}

void CGameRules_CaptureTheFlag::FlagPickedUp( int teamId, EntityId playerId )
{
	STeamInfo *teamInfo = GetTeamInfo(teamId);
	IEntity *pFlag = gEnv->pEntitySystem->GetEntity(teamInfo->flagId);
	IEntity *pCarrier = gEnv->pEntitySystem->GetEntity(playerId);
	if (pFlag && pCarrier)
	{
		IScriptTable *playerTable = pCarrier->GetScriptTable();

		IScriptTable *pScript = pFlag->GetScriptTable();
		if (pScript && pScript->GetValueType("AttachTo") == svtFunction)
		{
			m_pScriptSystem->BeginCall(pScript, "AttachTo");
			m_pScriptSystem->PushFuncParam(pScript);
			m_pScriptSystem->PushFuncParam(playerTable);
			m_pScriptSystem->EndCall();
		}
	}
	bool found = false;
	for (std::vector<EntityId>::const_iterator it = teamInfo->previousCarriers.begin(); it != teamInfo->previousCarriers.end(); ++it)
	{
		if (*it == playerId)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		teamInfo->previousCarriers.push_back(playerId);
	}
	SetFlagState(teamId, ECTF_FLAG_STATE_CARRIED);
	teamInfo->carrierId = playerId;
}

void CGameRules_CaptureTheFlag::ClFlagPickedUp( int teamId, EntityId playerId )
{
	STeamInfo *teamInfo = GetTeamInfo(teamId);
	if (!gEnv->bServer)
	{
		FlagPickedUp(teamId, playerId);
	}
	ClRethinkMusic();
	//TODO: display message
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagRetrieved)
{
	ClFlagRetrieved(params.teamId, params.entityId);
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagReset)
{
	ClFlagReset(params.data);
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagCompleted)
{
	ClFlagCompleted(params.teamId, params.entityId);
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagPickedUp)
{
	ClFlagPickedUp(params.teamId, params.entityId);
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagDropped)
{
	ClFlagDropped(params.teamId, params.entityId, params.pos);
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagAssistCapture)
{
	//TODO: Display a nice HUD message!
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClOnFlagCapture)
{
	//TODO: Display a nice HUD message with points from the params object
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClSetState)
{
	CryLogAlways("ClSetState( carrier %i, state %i, carrier %i, state %i )", params.flag1CarrierId, params.flag1State, params.flag2CarrierId, params.flag2State);
	STeamInfo *info1 = GetTeamInfo(1);
	info1->carrierId = params.flag1CarrierId;
	info1->flagState = params.flag1State;
	STeamInfo *info2 = GetTeamInfo(2);
	info2->carrierId = params.flag2CarrierId;
	info2->flagState = params.flag2State;

	if (info1->flagId && info2->flagId)
	{
		ClSetEntities();
	}

	m_clientStateSet = true;

	return true;
}

void CGameRules_CaptureTheFlag::SvOnClientEnteredGame( int channelId, IEntity *player, bool reset, bool loadingSaveGame )
{
	CGameRules_TeamInstantAction::SvOnClientEnteredGame(channelId, player, reset, loadingSaveGame);

	STeamInfo *info1 = GetTeamInfo(1);
	STeamInfo *info2 = GetTeamInfo(2);

	CTFStateParams params(info1->flagState, info1->carrierId, info2->flagState, info2->carrierId);
	GetGameObject()->InvokeRMI(ClSetState(), params, eRMI_ToClientChannel, channelId);
}

void CGameRules_CaptureTheFlag::SvReset()
{
	ResetFlag(1);
	ResetFlag(2);

	CTFStateParams params(ECTF_FLAG_STATE_INBASE, 0, ECTF_FLAG_STATE_INBASE, 0);
	GetGameObject()->InvokeRMI(ClSetState(), params, eRMI_ToAllClients);
}

void CGameRules_CaptureTheFlag::CmdDumpEntities( IConsoleCmdArgs *pArgs )
{
	CGameRules_CaptureTheFlag *pGameRules = static_cast<CGameRules_CaptureTheFlag*>(g_pGame->GetGameRules());

	for (int teamId = 1; teamId < 3; ++ teamId)
	{
		STeamInfo *teamInfo = pGameRules->GetTeamInfo(teamId);
		CryLogAlways("Team %i", teamId);
		CryLogAlways("  Flag id = %i", teamInfo->flagId);
		CryLogAlways("  Base id = %i", teamInfo->baseId);
		CryLogAlways("  Flag state = %i", teamInfo->flagState);
		CryLogAlways("  Carrier id = %i", teamInfo->carrierId);
	}
}

void CGameRules_CaptureTheFlag::PostInit( IGameObject * pGameObject )
{
	CGameRules_TeamInstantAction::PostInit(pGameObject);

	REGISTER_COMMAND("g_ctfDumpEntities", CmdDumpEntities, VF_NULL, "");
}

void CGameRules_CaptureTheFlag::Release()
{
	gEnv->pConsole->RemoveCommand("g_ctfDumpEntities");

	CGameRules_TeamInstantAction::Release();
}

void CGameRules_CaptureTheFlag::ClSetEntities()
{
	for (int teamId = 1; teamId < 3; ++ teamId)
	{
		STeamInfo *teamInfo = GetTeamInfo(teamId);

		CryLogAlways("CGameRules_CaptureTheFlag::ClSetEntities(), team %i, flag state=%i, carrier=%i", teamId, teamInfo->flagState, teamInfo->carrierId);

		if (teamInfo->flagState == ECTF_FLAG_STATE_CARRIED)
		{
			ClFlagPickedUp(teamId, teamInfo->carrierId);
		}
		else if (teamInfo->flagState == ECTF_FLAG_STATE_INBASE)
		{
			ResetFlag(teamId);
		}

		if (teamInfo->flagState != ECTF_FLAG_STATE_INBASE)
		{
			teamInfo->previousCarriers.push_back(0);
			CallEntityScriptFunction(teamInfo->baseId, "FlagTaken");
		}
	}
	ClRethinkMusic();
}

void CGameRules_CaptureTheFlag::SvCheckTeamScore(int teamId)
{
	int score = GetTeamScore(teamId);

	if (m_state == EGR_InGame)
	{
		int scoreLimit = g_pGame->GetCVars()->g_scoreLimit;

		if (scoreLimit > 0 && score >= scoreLimit)
		{
			CryLogAlways("team %i has won", teamId);
			SvOnGameEnd(teamId, EGER_ScoreLimitReached);
		}
	}
}

void CGameRules_CaptureTheFlag::SvOnUpdate( float frameTime )
{
	CGameRules_TeamInstantAction::SvOnUpdate(frameTime);

	for (int teamId = 1; teamId < 3; ++ teamId)
	{
		STeamInfo *teamInfo = GetTeamInfo(teamId);
		if (teamInfo->flagState == ECTF_FLAG_STATE_DROPPED)
		{
			if (teamInfo->flagDropTime + FLAG_RESET_TIMER_LENGTH < GetServerTime())
			{
				ResetFlag(teamId);
				UI8Params params((uint8) teamId);
				GetGameObject()->InvokeRMI(ClOnFlagReset(), params, eRMI_ToAllClients);
			}
		}
	}
}

void CGameRules_CaptureTheFlag::SvEntityEnterFlagArea( int flagTeamId, EntityId insideId )
{
	STeamInfo *flagTeamInfo = GetTeamInfo(flagTeamId);
	std::vector<EntityId>::iterator it = flagTeamInfo->insideIds.begin();
	bool found = false;
	for (; it != flagTeamInfo->insideIds.end(); ++it)
	{
		if (*it == insideId)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		flagTeamInfo->insideIds.push_back(insideId);
	}

	int teamId = GetTeam(insideId);
	int otherTeam = 3 - teamId;

	CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(insideId));
	if (pActor && pActor->GetHealth() > 0 && pActor->GetSpectatorMode() == CActor::eASM_None && teamId)
	{
		STeamInfo *teamInfo = GetTeamInfo(teamId);
		STeamInfo *otherInfo = GetTeamInfo(3 - teamId);

		if (flagTeamId == teamId)
		{
			CryLogAlways("Same team");
			if (teamInfo->flagState == ECTF_FLAG_STATE_DROPPED)
			{
				CryLogAlways("Flag is dropped return it");
				ResetFlag(teamId);
				EntityTeamParams params(insideId, teamId);
				GetGameObject()->InvokeRMI(ClOnFlagRetrieved(), params, eRMI_ToAllClients);

				int points = 5;		// Retrieval points
				if (otherInfo->flagState == ECTF_FLAG_STATE_CARRIED && otherInfo->carrierId == insideId)
				{
					points += 5;	// Extra retrieval points for retrieving a flag whilst carrying the other flag
				}
				SvIncreasePlayerScore(insideId, points);
			}
			else if (teamInfo->flagState == ECTF_FLAG_STATE_INBASE)
			{
				CryLogAlways("Flag is in base");
				if (otherInfo->flagState == ECTF_FLAG_STATE_CARRIED && otherInfo->carrierId == insideId)
				{
					CryLogAlways("Carrying other flag, increase score and reset");

					SvIncreaseSynchedEntityValue(insideId, ECTF_KEY_FLAGS, 1);

					if (otherInfo->previousCarriers.size() == 1)
					{
						SvIncreasePlayerScore(insideId, 20);	// Capture points + extra captured alone points
						UI8Params params(20);
						GetGameObject()->InvokeRMI(ClOnFlagCapture(), params, eRMI_ToClientChannel, pActor->GetChannelId());
					}
					else if (otherInfo->previousCarriers.size() > 1)
					{
						std::vector<EntityId>::const_iterator it = otherInfo->previousCarriers.begin();
						for (; it != otherInfo->previousCarriers.end(); ++it)
						{
							CActor *pCarrierActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*it));
							if (pCarrierActor)
							{
								int points = 0;
								if (*it == insideId)
								{
									points = 15;		// Capture points
									UI8Params params(15);
									GetGameObject()->InvokeRMI(ClOnFlagCapture(), params, eRMI_ToClientChannel, pActor->GetChannelId());
								}
								else
								{
									points = 5;			// Assisted capture
									NoParams params;
									GetGameObject()->InvokeRMI(ClOnFlagAssistCapture(), params, eRMI_ToClientChannel, pActor->GetChannelId());
								}
								SvIncreasePlayerScore(*it, points);
							}
						}
					}

					SvFlagCompleted(teamId, insideId);
					EntityTeamParams params(insideId, teamId);
					GetGameObject()->InvokeRMI(ClOnFlagCompleted(), params, eRMI_ToAllClients);
					SvIncreaseTeamScore(teamId, 1);
				}
			}
		}
		else
		{
			CryLogAlways("Different team");

			if (otherInfo->flagState == ECTF_FLAG_STATE_INBASE || otherInfo->flagState == ECTF_FLAG_STATE_DROPPED)
			{
				if (otherInfo->flagState == ECTF_FLAG_STATE_INBASE)
				{
					SvIncreasePlayerScore(insideId, 2);		// Flag taken from base
				}

				CryLogAlways("Flag is available for pickup");
				FlagPickedUp(otherTeam, insideId);
				EntityTeamParams params(insideId, otherTeam);
				GetGameObject()->InvokeRMI(ClOnFlagPickedUp(), params, eRMI_ToAllClients);
			}
		}
	}
}

void CGameRules_CaptureTheFlag::SvEntityLeaveFlagArea( int flagTeamId, EntityId insideId )
{
	STeamInfo *teamInfo = GetTeamInfo(flagTeamId);

	std::vector<EntityId>::iterator it = teamInfo->insideIds.begin();
	bool found = false;
	for (; it != teamInfo->insideIds.end(); ++it)
	{
		if (*it == insideId)
		{
			found = true;
			break;
		}
	}
	if (found)
	{
		teamInfo->insideIds.erase(it);
	}
}

void CGameRules_CaptureTheFlag::ClRethinkMusic()
{
	ASSERT_IS_CLIENT;

	int localTeam = GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());

	int actionAmount = 0;
	for (int teamId = 1; teamId < 3; ++ teamId)
	{
		STeamInfo *teamInfo = GetTeamInfo(teamId);
		if (teamInfo->flagState == ECTF_FLAG_STATE_CARRIED)
		{
			if (localTeam && (localTeam != teamId))
			{
				actionAmount += 3;
			}
			else
			{
				actionAmount += 2;
			}
		}
		else if ((teamInfo->flagState == ECTF_FLAG_STATE_DROPPED) && localTeam && (localTeam == teamId))
		{
			actionAmount += 1;
		}
	}

	char musicMood[16];

	switch (actionAmount)
	{
	case 1:
		sprintf(musicMood, "incidental");
		break;
	case 2:
		sprintf(musicMood, "middle");
		break;
	case 3:
		sprintf(musicMood, "action");
		break;
	case 4:
		sprintf(musicMood, "motivation1");
		break;
	case 5:
		sprintf(musicMood, "resolution1");
		break;
	default:
		sprintf(musicMood, "ambient");
		break;
	}

	CryLogAlways("CGameRules_CaptureTheFlag::RethinkMusic() - actionAmount = %i, setting music mood to \"%s\"", actionAmount, musicMood);
	gEnv->pMusicSystem->SetMood(musicMood);
}

tSoundID CGameRules_CaptureTheFlag::ClPlaySound( const char *soundName, const Vec3 &pos, uint32 flags, uint32 semantic )
{
	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(soundName, flags);

	if (pSound)
	{
		pSound->SetPosition(pos);
		pSound->SetSemantic((ESoundSemantic)semantic);
		pSound->Play();

		return pSound->GetId();
	}
	return 0;
}

void CGameRules_CaptureTheFlag::SetFlagState(int teamId, ECTFFlagState state)
{
	STeamInfo *teamInfo = GetTeamInfo(teamId);

	CryLogAlways("CGameRules_CaptureTheFlag::SetFlagState, flag %i changing from state %i to %i", teamId, teamInfo->flagState, state);
	teamInfo->flagState = state;

	if (gEnv->bClient)
	{
		if (teamInfo->flagState != ECTF_FLAG_STATE_INBASE)
		{
			if (!teamInfo->alarmSoundId)
			{
				IEntity *pBase = gEnv->pEntitySystem->GetEntity(teamInfo->baseId);
				if (pBase)
				{
					teamInfo->alarmSoundId = ClPlaySound("Sounds/ctf:ctf:base_alarm_loop", pBase->GetWorldPos(), FLAG_SOUND_3D, eSoundSemantic_Mechanic_Entity);
				}
			}
		}
		else if (teamInfo->alarmSoundId)
		{
			ISound *pSound = gEnv->pSoundSystem->GetSound(teamInfo->alarmSoundId);
			if (pSound)
			{
				pSound->Stop();
			}
			teamInfo->alarmSoundId = 0;
		}
	}
}

void CGameRules_CaptureTheFlag::SvAssignKillPoints( EntityId targetId, EntityId shooterId )
{
	if (targetId == shooterId)
	{
		return;
	}
	else
	{
		int shooterTeamId = GetTeam(shooterId);
		int targetTeamId = GetTeam(targetId);
		if (shooterTeamId == targetTeamId)
		{
			SvIncreaseSynchedEntityValue(shooterId, ETIA_KEY_TEAM_KILLS, 1);
			SvIncreasePlayerScore(shooterId, -5);
		}
		else
		{
			SvIncreaseSynchedEntityValue(shooterId, EIA_KEY_KILLS, 1);

			int points = NUM_POINTS_KILL;

			STeamInfo *shooterTeamInfo = GetTeamInfo(shooterTeamId);
			if ((shooterTeamInfo->flagState == ECTF_FLAG_STATE_CARRIED) && (shooterTeamInfo->carrierId == targetId))
			{
				points += NUM_POINTS_FLAG_CARRIER_KILLED;
			}
			else if (shooterTeamInfo->flagState == ECTF_FLAG_STATE_INBASE)
			{
				IEntity *pTarget = gEnv->pEntitySystem->GetEntity(targetId);
				IEntity *pFlag = gEnv->pEntitySystem->GetEntity(shooterTeamInfo->flagId);
				if (pTarget && pFlag)
				{
					float distSqr = pTarget->GetWorldPos().GetSquaredDistance(pFlag->GetWorldPos());
					if (distSqr < DEFEND_FLAG_RADIUS_SQR)
					{
						points += NUM_POINTS_FLAG_DEFENDED;
						GetGameObject()->InvokeRMI(ClFlagDefended(), NoParams(), eRMI_ToClientChannel, GetChannelId(shooterId));
					}
				}
			}

			STeamInfo *targetTeamInfo = GetTeamInfo(targetTeamId);
			if ((targetTeamInfo->flagState == ECTF_FLAG_STATE_CARRIED) && (targetTeamInfo->carrierId != shooterId))
			{
				TCTFPlayerDataMap::iterator it = m_ctfPlayerValues.find(targetId);
				if (it != m_ctfPlayerValues.end())
				{
					float timeLastHitCarrier = GetServerTime() - it->second.timeLastHitFlagCarrier;
					if (timeLastHitCarrier < MAX_TIME_BETWEEN_FLAG_HIT_AND_KILL_FOR_PROTECT)
					{
						points += NUM_POINTS_FLAG_CARRIER_PROTECTED;
						GetGameObject()->InvokeRMI(ClFlagCarrierProtected(), NoParams(), eRMI_ToClientChannel, GetChannelId(shooterId));
					}
				}
			}

			SvIncreasePlayerScore(shooterId, points);
		}
	}
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClFlagDefended)
{
	//TODO: Display message
	return true;
}

IMPLEMENT_RMI(CGameRules_CaptureTheFlag, ClFlagCarrierProtected)
{
	//TODO: Display message
	return true;
}

void CGameRules_CaptureTheFlag::ClOnKill( EntityId targetId, EntityId shooterId, int weaponClassId, float damage, int material, int hit_type )
{
	CGameRules_TeamInstantAction::ClOnKill(targetId, shooterId, weaponClassId, damage, material, hit_type);

	if (g_pGame->GetIGameFramework()->GetClientActorId() == shooterId)
	{
		int targetTeamId = GetTeam(targetId);
		int shooterTeamId = GetTeam(shooterId);

		int points = NUM_POINTS_KILL;

		if ((targetTeamId != shooterTeamId) && shooterTeamId)
		{
			STeamInfo *teamInfo = GetTeamInfo(shooterTeamId);
			if ((teamInfo->flagState == ECTF_FLAG_STATE_CARRIED) && (teamInfo->carrierId == targetId))
			{
				points += NUM_POINTS_FLAG_CARRIER_KILLED;
			}
		}

		//TODO: Display kill message using "points"
	}
}