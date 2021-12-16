/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 14:08:2006   11:29 : Created by AlexL

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_Game.h"
#include "Game.h"
#include "Audio/GameAudio.h"
#include "IAISystem.h"
#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "Perk.h"
#include "Player.h" // for perk setting
#include "TacticalManager.h"
#include "ICheckPointSystem.h"
#include "NanoSuitDefs.h"
#include "IGameFramework.h"

#ifdef WIN64
	#pragma warning(disable: 4244)
#endif


//------------------------------------------------------------------------
CScriptBind_Game::CScriptBind_Game(ISystem *pSystem, IGameFramework *pGameFramework)
: m_pSystem(pSystem),
	m_pSS(pSystem->GetIScriptSystem()),
	m_pGameFW(pGameFramework)
{
	Init(m_pSS, m_pSystem);
	SetGlobalName("Game");

	RegisterMethods();
	RegisterGlobals();
}

//------------------------------------------------------------------------
CScriptBind_Game::~CScriptBind_Game()
{
}

//------------------------------------------------------------------------
void CScriptBind_Game::RegisterGlobals()
{
	m_pSS->SetGlobalValue("eTacticalEntity_Story", CTacticalManager::eTacticalEntity_Story);
	m_pSS->SetGlobalValue("eTacticalEntity_Item", CTacticalManager::eTacticalEntity_Item);
}

//------------------------------------------------------------------------
void CScriptBind_Game::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Game::

	SCRIPT_REG_TEMPLFUNC(ShowMainMenu, "");
	SCRIPT_REG_TEMPLFUNC(ShowInGameMenu, "");
	SCRIPT_REG_TEMPLFUNC(PauseGame, "pause");
	SCRIPT_REG_TEMPLFUNC(PlayFlashAnim, "");
	SCRIPT_REG_TEMPLFUNC(PlayVideo, "");
//	SCRIPT_REG_TEMPLFUNC(QueryBattleStatus, "");
	SCRIPT_REG_TEMPLFUNC(SetPerksAllowed,"allowed");
	SCRIPT_REG_TEMPLFUNC(GetPerkIndex,"name");
	SCRIPT_REG_TEMPLFUNC(GivePlayerPerk,"playerId, perkName, enabled");
	SCRIPT_REG_TEMPLFUNC(ClearPlayersPerks,"playerId");
	SCRIPT_REG_TEMPLFUNC(CountPlayersInVehicleWithPerk,"vehicleId, perkIndex");

	SCRIPT_REG_FUNC(IsMountedWeaponUsableWithTarget);

	SCRIPT_REG_TEMPLFUNC(IsPlayer, "entityId");

	SCRIPT_REG_TEMPLFUNC(AddTacticalEntity, "entityId, type");
	SCRIPT_REG_TEMPLFUNC(RemoveTacticalEntity, "entityId, type");

	SCRIPT_REG_TEMPLFUNC(SaveCheckpoint, "checkpointId, fileName");
	SCRIPT_REG_TEMPLFUNC(LoadCheckpoint, "checkpointId");
	SCRIPT_REG_TEMPLFUNC(QuickLoad, "");

	SCRIPT_REG_TEMPLFUNC(DebugDrawCylinder, "x, y, z, radius, height, r, g, b, a");

#undef SCRIPT_REG_CLASSNAME
}

//------------------------------------------------------------------------
int CScriptBind_Game::AddTacticalEntity(IFunctionHandler *pH, ScriptHandle id, int type)
{
	g_pGame->GetTacticalManager()->AddEntity((EntityId)id.n, (CTacticalManager::ETacticalEntityType)type);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Game::RemoveTacticalEntity(IFunctionHandler *pH, ScriptHandle id, int type)
{
	g_pGame->GetTacticalManager()->RemoveEntity((EntityId)id.n, (CTacticalManager::ETacticalEntityType)type);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Game::ShowMainMenu(IFunctionHandler *pH)
{
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Game::ShowInGameMenu(IFunctionHandler *pH)
{
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Game::PauseGame( IFunctionHandler *pH, bool pause )
{
	bool forced = false;

	if (pH->GetParamCount() > 1)
	{
		pH->GetParam(2, forced);
	}
	m_pGameFW->PauseGame(pause, forced);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Game::PlayFlashAnim(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* pFlashAnim(0);
	pH->GetParam(1, pFlashAnim);
/*	CFlashMenuObject* pFMO(CFlashMenuObject::GetFlashMenuObject());
	if (pFlashAnim && pFMO)
		pFMO->PlayFlashAnim(pFlashAnim);
*/
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Game::PlayVideo(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS_MIN(1);

	const char* pVideo(0);
	pH->GetParam(1, pVideo);

	int audioCh(0);
	if (pH->GetParamCount() > 1)
		pH->GetParam(2, audioCh);

	int voiceCh(-1);
	if (pH->GetParamCount() > 2)
		pH->GetParam(3, voiceCh);

	bool useSubtitles(false);
	if (pH->GetParamCount() > 3)
		pH->GetParam(4, useSubtitles);

	bool exclusiveVideo(false);
	if (pH->GetParamCount() > 4)
		pH->GetParam(5, exclusiveVideo);

/*	CFlashMenuObject* pFMO(CFlashMenuObject::GetFlashMenuObject());
	if (pVideo && pFMO)
		pFMO->PlayVideo(pVideo, true, 0, audioCh, voiceCh, useSubtitles, exclusiveVideo);
*/
	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
//int CScriptBind_Game::QueryBattleStatus(IFunctionHandler *pH)
//{		
//	float fStatus = SAFE_GAMEAUDIO_BATTLESTATUS_FUNC_RET(QueryBattleStatus());
	
//	return pH->EndFunction(fStatus);
//}

//////////////////////////////////////////////////////////////////////////

int CScriptBind_Game::IsPlayer(IFunctionHandler *pH, ScriptHandle entityId)
{
	EntityId eId = entityId.n;
	if(eId == LOCAL_PLAYER_ENTITY_ID)
		return pH->EndFunction(true);

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(eId);
	if(pActor && pActor->IsPlayer())
		return true;

	return pH->EndFunction(false);
}

//////////////////////////////////////////////////////////////////////////

#define GET_ENTITY(i) \
	ScriptHandle hdl;\
	pH->GetParam(i,hdl);\
	int nID = hdl.n;\
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(nID);


//////////////////////////////////////////////////////////////////////////
// IsMountedWeaponUsableWithTarget
// A piece of game-code moved from CryAction when scriptbind_AI moved to the AI system
//////////////////////////////////////////////////////////////////////////
int CScriptBind_Game::IsMountedWeaponUsableWithTarget(IFunctionHandler *pH)
{
	int paramCount = pH->GetParamCount();
	if(paramCount<2)
	{
		GameWarning("%s: too few parameters.", __FUNCTION__);
		return pH->EndFunction();
	}

	GET_ENTITY(1);

	if(!pEntity)
	{
		GameWarning("%s: wrong entity id in parameter 1.", __FUNCTION__);
		return pH->EndFunction();
	}

	IAIObject* pAI = pEntity->GetAI();
	if (!pAI)
	{
		GameWarning("%s: Entity '%s' does not have AI.",__FUNCTION__,  pEntity->GetName());
		return pH->EndFunction();
	}

	EntityId itemEntityId;
	ScriptHandle hdl2;

	if(!pH->GetParam(2,hdl2))
	{
		GameWarning("%s: wrong parameter 2 format.", __FUNCTION__);
		return pH->EndFunction();
	}

	itemEntityId = hdl2.n;

	if (!itemEntityId)
	{
		GameWarning("%s: wrong entity id in parameter 2.", __FUNCTION__);
		return pH->EndFunction();
	}
	
	IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
	IItem* pItem = pGameFramework->GetIItemSystem()->GetItem(itemEntityId);
	if (!pItem)
	{
		//gEnv->pAISystem->Warning("<CScriptBind> ", "entity in parameter 2 is not an item/weapon");
		GameWarning("%s: entity in parameter 2 is not an item/weapon.", __FUNCTION__);
		return pH->EndFunction();
	}

	float minDist = 7;
	bool bSkipTargetCheck = false;
	Vec3 targetPos(ZERO);

	if(paramCount > 2)
	{
		for(int i=3;i <= paramCount ; i++)
		{
			if(pH->GetParamType(i) == svtBool)
				pH->GetParam(i,bSkipTargetCheck);
			else if(pH->GetParamType(i) == svtNumber)
				pH->GetParam(i,minDist);
			else if(pH->GetParamType(i) == svtObject)
				pH->GetParam(i,targetPos);
		}
	}

	IPipeUser* pPiper = CastToIPipeUserSafe(pAI);
	if(!pPiper)
	{
		//gEnv->pAISystem->Warning("<CScriptBind> ", "entity '%s' in parameter 1 is not a puppet", pEntity->GetName());
		GameWarning("%s: entity '%s' in parameter 1 is not a puppet.", __FUNCTION__, pEntity->GetName());
		return pH->EndFunction();
	}


	IEntity* pItemEntity = pItem->GetEntity();
	if(!pItemEntity)
		return pH->EndFunction();


	if(!pItem->GetOwnerId())
	{
		// weapon is not used, check if it is on a vehicle
		IEntity* pParentEntity = pItemEntity->GetParent();
		if(pParentEntity)
		{
			IAIObject* pParentAI = pParentEntity->GetAI();
			if(pParentAI && pParentAI->GetAIType()==AIOBJECT_VEHICLE)
			{
				// (MATT) Feature was cut and code was tricky, hence ignore weapons in vehicles  {2008/02/15:11:08:51}
				return pH->EndFunction();
			}
		}
	}
	else if( pItem->GetOwnerId()!= pEntity->GetId()) // item is used by someone else?
		return pH->EndFunction(false);

	// check target
	if(bSkipTargetCheck)
		return pH->EndFunction(true);

	IAIObject* pTarget = pPiper->GetAttentionTarget();
	if(targetPos.IsZero())
	{
		if(!pTarget)
			return pH->EndFunction();
		targetPos = pTarget->GetPos();
	}

	Vec3 targetDir(targetPos - pItemEntity->GetWorldPos());
	Vec3 targetDirXY(targetDir.x, targetDir.y, 0);

	float length2D = targetDirXY.GetLength();
	if(length2D < minDist || length2D<=0)
		return pH->EndFunction();

	targetDirXY /= length2D;//normalize

	Vec3 mountedAngleLimits(pItem->GetMountedAngleLimits());

	float yawRange = DEG2RAD(mountedAngleLimits.z);
	if(yawRange > 0 && yawRange < gf_PI)
	{
		float deltaYaw = pItem->GetMountedDir().Dot(targetDirXY);
		if(deltaYaw < cosf(yawRange))
			return pH->EndFunction(false);
	}

	float minPitch = DEG2RAD(mountedAngleLimits.x);
	float maxPitch = DEG2RAD(mountedAngleLimits.y);

	//maxPitch = (maxPitch - minPitch)/2;
	//minPitch = -maxPitch;

	float pitch = atanf(targetDir.z / length2D);

	if ( pitch < minPitch || pitch > maxPitch )
		return pH->EndFunction(false);

	if(pTarget)
	{
		IEntity* pTargetEntity = pTarget->GetEntity();
		if(pTargetEntity)
		{
			// check target distance and where he's going
			IPhysicalEntity *phys = pTargetEntity->GetPhysics();
			if(phys)
			{
				pe_status_dynamics	dyn;
				phys->GetStatus(&dyn);
				Vec3 velocity ( dyn.v);
				velocity.z = 0;

				float speed = velocity.GetLength2D();
				if(speed>0)
				{
					//velocity /= speed;
					if(length2D< minDist * 0.75f && velocity.Dot(targetDirXY)<=0)
						return pH->EndFunction(false);
				}
			}
		}
	}
	return pH->EndFunction(true);

}

////////////////////
int CScriptBind_Game::SetPerksAllowed(IFunctionHandler *pH, bool allowed)
{
	CPlayer*  pPlayer = static_cast< CPlayer* >( gEnv->pGame->GetIGameFramework()->GetClientActor() );
	if (pPlayer)
	{
		pPlayer->SetPerksAllowed(allowed);
	}
	return pH->EndFunction();
}

int CScriptBind_Game::GetPerkIndex(IFunctionHandler *pH, const char * name)
{
	EPerks perkNumber = CPerk::GetInstance()->FindPerkNumberByName(name);
	CRY_ASSERT_MESSAGE(perkNumber >= 0, string().Format("A lua function called GetPerkIndex(\"%s\") but there's no such perk", name));
	return pH->EndFunction(perkNumber);
}

int CScriptBind_Game::GivePlayerPerk(IFunctionHandler *pH, ScriptHandle playerId, const char * perkName, bool enabled)
{
	EntityId actorId = static_cast<EntityId>(playerId.n);
	CPlayer *pPlayer = 0;
	pPlayer = static_cast<CPlayer*>(m_pGameFW->GetIActorSystem()->GetActor(actorId));
	assert(pPlayer);
	pPlayer->SetPerkActive( CPerk::GetInstance()->FindPerkNumberByName(perkName), enabled );

	return pH->EndFunction();
}

int CScriptBind_Game::ClearPlayersPerks(IFunctionHandler *pH, ScriptHandle playerId )
{
	EntityId actorId = (EntityId)playerId.n;
	CPlayer *pPlayer = 0;
	pPlayer = static_cast<CPlayer*>(m_pGameFW->GetIActorSystem()->GetActor(actorId));
	assert(pPlayer);
	CPerk * perkInstance = CPerk::GetInstance();
	for( EPerks id_flag = perkInstance->Begin( ePerkValid_Players ); 
		id_flag != perkInstance->End(); 
		id_flag=perkInstance->GetNextId(id_flag, ePerkValid_Players) )
	{
		pPlayer->SetPerkActive( id_flag, false );
	}

	return pH->EndFunction();
}

int CScriptBind_Game::CountPlayersInVehicleWithPerk(IFunctionHandler * pH, ScriptHandle vehicleId, int perkIndex)
{
	int counter = 0;
	IVehicle * vehicle = m_pGameFW->GetIVehicleSystem()->GetVehicle((EntityId)vehicleId.n);

	if (vehicle)
	{
		int numSeats = vehicle->GetSeatCount();
		for (TVehicleSeatId id = 1; id <= numSeats; ++ id)
		{
			EntityId passengerEntityID = vehicle->GetSeatById(id)->GetPassenger();
			IActor * passengerActor = m_pGameFW->GetIActorSystem()->GetActor(passengerEntityID);
			
			if (passengerActor && passengerActor->IsPlayer())
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(passengerActor);
				bool got = pPlayer->IsPerkActive((EPerks)perkIndex);
				counter += (got ? 1 : 0);
			}
		}
	}
	return pH->EndFunction(counter);
}

//====================================================================
// Checkpoint Trigger 
//====================================================================
int CScriptBind_Game::SaveCheckpoint(IFunctionHandler *pH, ScriptHandle checkpointId, const char *fileName)
{
	assert(pH);
	assert(fileName);
	bool success = m_pGameFW->GetICheckpointSystem()->SaveGame((EntityId)checkpointId.n, fileName);
	return pH->EndFunction(success);
}

//====================================================================
// Checkpoint Loading 
//====================================================================
int CScriptBind_Game::LoadCheckpoint(IFunctionHandler *pH, const char *fileName)
{
	assert(pH);
	assert(fileName);
	bool success = m_pGameFW->GetICheckpointSystem()->LoadGame(fileName);
	return pH->EndFunction(success);
}

int CScriptBind_Game::QuickLoad( IFunctionHandler *pH )
{
	assert(pH);
	g_pGame->GetIGameFramework()->ExecuteCommandNextFrame("loadLastSave");
	return pH->EndFunction();
}

//====================================================================
// Debug
//====================================================================
int CScriptBind_Game::DebugDrawCylinder( IFunctionHandler *pH, float x, float y, float z, float radius, float height, int r, int g, int b, int a )
{
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawCylinder(Vec3(x, y, z), Vec3(0.f, 0.f, 1.f), radius, height, ColorB(r, g, b, a));
	return pH->EndFunction();
}
