/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2006.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 14:02:2006   11:29 : Created by AlexL
	- 04:04:2006	 17:30 : Extended by Jan Müller

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_HUD.h"
#include "IGameObject.h"
#include "Game.h"
#include "GameRules.h"
#include "IWorldQuery.h"

#include "HUD/HUD.h"
#include "HUD/HUDMissionObjectiveSystem.h"
#include "HUD/UI/UISimpleBar.h" // TODO : remove in favor of sending a HUDEvent

//------------------------------------------------------------------------
CScriptBind_HUD::CScriptBind_HUD(ISystem *pSystem, IGameFramework *pGameFramework)
: m_pSystem(pSystem),
	m_pSS(pSystem->GetIScriptSystem()),
	m_pGameFW(pGameFramework)
{
	Init(m_pSS, m_pSystem);
	SetGlobalName("HUD");

	RegisterMethods();
	RegisterGlobals();
}

//------------------------------------------------------------------------
CScriptBind_HUD::~CScriptBind_HUD()
{
}

//------------------------------------------------------------------------
void CScriptBind_HUD::RegisterGlobals()
{
	m_pSS->SetGlobalValue("MO_DEACTIVATED", CHUDMissionObjective::DEACTIVATED);
	m_pSS->SetGlobalValue("MO_COMPLETED", CHUDMissionObjective::COMPLETED);
	m_pSS->SetGlobalValue("MO_FAILED", CHUDMissionObjective::FAILED);
	m_pSS->SetGlobalValue("MO_ACTIVATED", CHUDMissionObjective::ACTIVATED);

	m_pSS->SetGlobalValue("eBLE_Information", 2);
	m_pSS->SetGlobalValue("eBLE_Currency", 1);
	m_pSS->SetGlobalValue("eBLE_Warning", 3);
	m_pSS->SetGlobalValue("eBLE_System", 0);

}

//------------------------------------------------------------------------
void CScriptBind_HUD::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_HUD::

	SCRIPT_REG_TEMPLFUNC(SetObjectiveStatus,"objective,status,silent");
	SCRIPT_REG_TEMPLFUNC(GetObjectiveStatus,"");
	SCRIPT_REG_TEMPLFUNC(SetMainObjective, "objective");
	SCRIPT_REG_FUNC(GetMainObjective);
	SCRIPT_REG_TEMPLFUNC(SetObjectiveEntity,"objective,entity");
	SCRIPT_REG_TEMPLFUNC(DrawStatusText, "text");
	SCRIPT_REG_TEMPLFUNC(SetUsability, "objId, message");
	SCRIPT_REG_FUNC(ReloadLevel);
	SCRIPT_REG_FUNC(ReloadLevelSavegame);
	SCRIPT_REG_FUNC(TacWarning);
	SCRIPT_REG_FUNC(HitIndicator);
	SCRIPT_REG_TEMPLFUNC(DamageIndicator, "weaponId, shooterId, direction, onVehicle");
	SCRIPT_REG_TEMPLFUNC(EnteredBuyZone, "zoneId, entered");
	SCRIPT_REG_TEMPLFUNC(EnteredServiceZone, "zoneId, entered");
	SCRIPT_REG_TEMPLFUNC(UpdateBuyList, "");
	SCRIPT_REG_TEMPLFUNC(RadarShowVehicleReady, "vehicleId");
	SCRIPT_REG_TEMPLFUNC(AddEntityToRadarAsBinocularTag, "shooterId, targetId");
	SCRIPT_REG_TEMPLFUNC(AddEntityToRadar, "entityId");
	SCRIPT_REG_TEMPLFUNC(RemoveEntityFromRadar, "entityId");
	SCRIPT_REG_TEMPLFUNC(ShowKillZoneTime, "active, seconds");
	SCRIPT_REG_TEMPLFUNC(OnPlayerVehicleBuilt, "vehicleId, playerId");
	SCRIPT_REG_FUNC(StartPlayerFallAndPlay);
	SCRIPT_REG_TEMPLFUNC(ShowDeathFX, "type");
	SCRIPT_REG_TEMPLFUNC(BattleLogEvent, "type, msg, [p1], [p2], [p3], [p4]");
	SCRIPT_REG_TEMPLFUNC(OnItemBought, "success, itemName");
	SCRIPT_REG_FUNC(FakeDeath);
	SCRIPT_REG_TEMPLFUNC(ShowWarningMessage, "warning, text");
	SCRIPT_REG_TEMPLFUNC(GetMapGridCoord, "x, y");
	SCRIPT_REG_TEMPLFUNC(OpenPDA, "show, buymenu");

	SCRIPT_REG_TEMPLFUNC(SetCurrentCaptureSite, "site");
	SCRIPT_REG_TEMPLFUNC(ShowCaptureProgress, "show");
	SCRIPT_REG_TEMPLFUNC(SetCaptureProgress, "progress");
	SCRIPT_REG_TEMPLFUNC(SetCaptureContested, "contested");
	SCRIPT_REG_TEMPLFUNC(ShowConstructionProgress, "show, queued, constructionTime");
	SCRIPT_REG_TEMPLFUNC(ShowReviveCycle, "show");
	SCRIPT_REG_TEMPLFUNC(SpawnGroupInvalid, "");
	
	SCRIPT_REG_TEMPLFUNC(SetProgressBar, "show, percent, text");
	SCRIPT_REG_TEMPLFUNC(DisplayBigOverlayFlashMessage, "msg, duration, posX, posY, color");
	SCRIPT_REG_TEMPLFUNC(DisplayFunMessagePostfix, "msg, postfix");
	SCRIPT_REG_TEMPLFUNC(DisplayBonusMessage, "msg, teamId");
	SCRIPT_REG_TEMPLFUNC(FadeOutBigOverlayFlashMessage, "");
	SCRIPT_REG_TEMPLFUNC(GetLastInGameSave, "");
	SCRIPT_REG_TEMPLFUNC(TextMessage, "type, lifetime, text, [r], [g], [b]");

	SCRIPT_REG_TEMPLFUNC(AddEntitySilhouette, "entityId, r, g, b, a");
	SCRIPT_REG_TEMPLFUNC(RemoveEntitySilhouette, "entityId");
	//SCRIPT_REG_TEMPLFUNC(UpdateFlagState, "flag1_id, base1_id, state1, pos1, alpha1, carrierId1, flag2_id, base2_id, state2, pos2, alpha2, carrierId2");
	SCRIPT_REG_TEMPLFUNC(UpdateFlagState, "flag1_id, base1_id, state1, carrierId1, flag2_id, base2_id, state2, carrierId2");
	SCRIPT_REG_TEMPLFUNC(RequestLoadout, "");
	SCRIPT_REG_TEMPLFUNC(DisplayFunMessage, "msg");

//SCRIPT_REG_TEMPLFUNC(UpdateInventoryList, "");

	SCRIPT_REG_TEMPLFUNC(ClearPrototypeWeapons, "");
	SCRIPT_REG_TEMPLFUNC(AddPrototypeWeapon, "entityId");
	SCRIPT_REG_TEMPLFUNC(RemovePrototypeWeapon, "entityId");
	
	SCRIPT_REG_TEMPLFUNC(ClearActivePayloads, "");
	SCRIPT_REG_TEMPLFUNC(AddActivePayload, "entityId");
	SCRIPT_REG_TEMPLFUNC(RemoveActivePayload, "entityId");
	SCRIPT_REG_TEMPLFUNC(SetPayloadBank, "entityId");
	SCRIPT_REG_TEMPLFUNC(NewMissionObjective, "targetId, objectiveType, progress, priority");

	SCRIPT_REG_TEMPLFUNC(GetLookAtEntity, "");
	SCRIPT_REG_TEMPLFUNC(SetCaptureBar, "progress");
	SCRIPT_REG_TEMPLFUNC(TriggerCaptureBarWarning, "");

#undef SCRIPT_REG_CLASSNAME
}

//------------------------------------------------------------------------
int CScriptBind_HUD::SetObjectiveStatus(IFunctionHandler *pH,const char* pObjectiveID, int status, bool silent)
{
	CHUDMissionObjective*	pObj = g_pGame->GetMOSystem()->GetMissionObjective(pObjectiveID);
	if (pObj)
	{
		pObj->SetSilent(silent);
		pObj->SetStatus((CHUDMissionObjective::HUDMissionStatus)status);
	}
	else
		GameWarning("CScriptBind_HUD::Tried to access non existing MissionObjective '%s'", pObjectiveID);
	return pH->EndFunction();  
}

//------------------------------------------------------------------------
int CScriptBind_HUD::GetObjectiveStatus(IFunctionHandler* pH, const char* pObjectiveID)
{	
	CHUDMissionObjective*	pObj = g_pGame->GetMOSystem()->GetMissionObjective(pObjectiveID);
	if (pObj)
	{
		return pH->EndFunction(pObj->GetStatus());
	}
	else
		GameWarning("CScriptBind_HUD::Tried to access non existing MissionObjective '%s'", pObjectiveID);
	return pH->EndFunction(); 
}

//------------------------------------------------------------------------
int CScriptBind_HUD::SetMainObjective(IFunctionHandler* pH, const char* pObjectiveID)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (!pHUD || !pObjectiveID)
		return pH->EndFunction();
	pHUD->SetMainObjective(pObjectiveID, true);
*/
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_HUD::GetMainObjective(IFunctionHandler* pH)
{
//	CHUD* pHUD = g_pGame->GetHUD();
//	if(!pHUD)
		return pH->EndFunction();

//	return pH->EndFunction(pHUD->GetMainObjective());
}

//------------------------------------------------------------------------
int CScriptBind_HUD::SetObjectiveEntity(IFunctionHandler *pH,const char* pObjectiveID, ScriptHandle entityID)
{
	CHUDMissionObjective*	pObj = g_pGame->GetMOSystem()->GetMissionObjective(pObjectiveID);
	if (pObj)
		pObj->SetTrackedEntity((EntityId)entityID.n);
	else
		GameWarning("CScriptBind_HUD::Tried to access non existing MissionObjective '%s'", pObjectiveID);
	return pH->EndFunction();  
}

int CScriptBind_HUD::SetUsability(IFunctionHandler *pH, int objId, const char *message)
{
//	CHUD *pHUD = g_pGame->GetHUD();
//	if (!pHUD)
		return pH->EndFunction();

/*
	int usable = (gEnv->pEntitySystem->GetEntity(objId))?1:0;
	string textLabel;
	bool gotMessage = (message && strlen(message) != 0);
	if(gotMessage)
		textLabel = message;
	string param;
	if(usable == 1)
	{
		if(IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(objId))
		{
			textLabel = "@use_vehicle";
			if(IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor())
			{
				int pteamId=g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId());
				int vteamId=g_pGame->GetGameRules()->GetTeam(objId);
			
				if (vteamId && vteamId!=pteamId)
				{
					usable = 2;
					textLabel = "@use_vehicle_locked";
				}
			}
		}
		else if(IItem *pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(objId))
		{
			CActor *pActor = (CActor*)(gEnv->pGame->GetIGameFramework()->GetClientActor());
			if(pActor)
			{
				if(!gotMessage)
					return pH->EndFunction(); 
				//Offhand controls pick up messages
			}
		}
		else
		{
			if(gEnv->bMultiplayer && !gotMessage)
				usable = 0; 
			else if(!gotMessage)
				textLabel = "@pick_object";
		}
	}
	else
		textLabel = ""; //turn off text

	pHUD->GetCrosshair()->SetUsability(usable, textLabel.c_str(), (param.empty())?NULL:param.c_str());
*/

	return pH->EndFunction();  
}

int CScriptBind_HUD::DrawStatusText(IFunctionHandler *pH, const char* pText)
{
	return pH->EndFunction();
}

//-------------------------------------------------------------------------------------------------
int CScriptBind_HUD::ReloadLevel(IFunctionHandler *pH)
{
	string command("map ");
	command.append(gEnv->pGame->GetIGameFramework()->GetLevelName());
	gEnv->pConsole->ExecuteString(command.c_str());
	return pH->EndFunction();
}

int CScriptBind_HUD::ReloadLevelSavegame(IFunctionHandler *pH)
{
	g_pGame->InitMapReloading();
	return pH->EndFunction();
}

int CScriptBind_HUD::TacWarning(IFunctionHandler *pH)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (!pHUD)
		return pH->EndFunction();
	std::vector<CHUDRadar::RadarEntity> buildings = *(pHUD->GetRadar()->GetBuildings());
	for(int i = 0; i < buildings.size(); ++i)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(buildings[i].m_id);
		if(!stricmp(pEntity->GetClass()->GetName(), "HQ"))
		{
			Vec3 pos = pEntity->GetWorldPos();
			pos.z += 10;
			ISound *pSound = gEnv->pSoundSystem->CreateSound("sounds/interface:multiplayer_interface:mp_tac_alarm_ambience", FLAG_SOUND_3D);
			if(pSound)
			{
				pSound->SetPosition(pos);
				pSound->Play();
			}
		}
	}
	*/
	return pH->EndFunction();
}

int CScriptBind_HUD::EnteredBuyZone(IFunctionHandler *pH, ScriptHandle zoneId, bool entered)
{
	// call the hud, tell him we entered a buy zone
	// the hud itself needs to keep track of which team owns the zone and enable/disable buying accordingly
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetPowerStruggleHUD())
		pHUD->GetPowerStruggleHUD()->UpdateBuyZone(entered, (EntityId)zoneId.n);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::EnteredServiceZone(IFunctionHandler *pH, ScriptHandle zoneId, bool entered)
{
	// call the hud, tell him we entered a service zone
	// the hud itself needs to keep track of which team owns the zone and enable/disable buying accordingly
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetPowerStruggleHUD())
		pHUD->GetPowerStruggleHUD()->UpdateServiceZone(entered, (EntityId)zoneId.n);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::UpdateBuyList(IFunctionHandler *pH)
{
	// something that might have changed item availability happened, so we better update our list
/*	CHUD *pHUD = g_pGame->GetHUD();

	if (pHUD && pHUD->GetPowerStruggleHUD())
	{
		pHUD->GetPowerStruggleHUD()->UpdateBuyList();
		pHUD->GetPowerStruggleHUD()->UpdateBuyZone(true, 0);
		pHUD->GetPowerStruggleHUD()->UpdateServiceZone(true, 0);
	}
*/
	return pH->EndFunction();
}


int CScriptBind_HUD::DamageIndicator(IFunctionHandler *pH, ScriptHandle weaponId, ScriptHandle shooterId, Vec3 direction, bool onVehicle)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		Vec3 normDirection = direction.GetNormalized();
		pHUD->IndicateDamage((EntityId)weaponId.n, normDirection, onVehicle);
		pHUD->ShowTargettingAI((EntityId)shooterId.n);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::HitIndicator(IFunctionHandler *pH)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
		pHUD->IndicateHit();
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::RadarShowVehicleReady(IFunctionHandler *pH, ScriptHandle vehicleId)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
		pHUD->GetRadar()->ShowEntityTemporarily(EWayPoint, (EntityId)vehicleId.n);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::AddEntityToRadarAsBinocularTag(IFunctionHandler *pH, ScriptHandle shooterId, ScriptHandle targetId)
{
/*
	if (pHUD)
	{
		EHUDGAMERULES hudGameRules = pHUD->GetGameRules();
		CGameRules *gameRules = g_pGame->GetGameRules();
		if (hudGameRules == EHUD_ASSAULT)
		{
			if (pHUD->GetRadar()->CheckAssaultBinoculars((EntityId)targetId.n))
			{
				gameRules->AddTaggedEntity((EntityId)shooterId.n, (EntityId)targetId.n, true, 15.f, CGameRules::eRTR_AssaultBinoculars);
			}
		}
	}*/

	return pH->EndFunction();
}

int CScriptBind_HUD::AddEntityToRadar(IFunctionHandler *pH, ScriptHandle entityId)
{
	SHUDEvent hudevent(eHUDEvent_AddEntity);
	hudevent.AddData(SHUDEventData((int)entityId.n));
	CHUD::CallEvent(hudevent);

	return pH->EndFunction();
}

int CScriptBind_HUD::UpdateFlagState(IFunctionHandler *pH, ScriptHandle flag1_id, ScriptHandle base1_id, int state1/*, Vec3 pos1, float alpha1*/, ScriptHandle carrierId1
                                                         , ScriptHandle flag2_id, ScriptHandle base2_id, int state2/*, Vec3 pos2, float alpha2*/, ScriptHandle carrierId2)
{
	CRY_TODO(23, 09, 09, "[HUD/Gamerules] Refactor/Remove to CTF Lua & use Non-gamemode specific objective specific script binds.");
#if 0

	CHUD *pHUD = g_pGame->GetHUD();
	if (!pHUD)
	{
		return pH->EndFunction();
	}
		
	CUIMissionObjectives* pObjectives = CUIMissionObjectives::GetInstance();

	EntityId own_flag = (EntityId)flag1_id.n;
	EntityId enemy_flag = (EntityId)flag2_id.n;
	assert( own_flag );
	assert( enemy_flag );

	EntityId own_base = (EntityId)base1_id.n;
	EntityId enemy_base = (EntityId)base2_id.n;
	assert( own_base );
	assert( enemy_base );

	EntityId own_flag_carrier = (EntityId)carrierId1.n;
	EntityId enemy_flag_carrier = (EntityId)carrierId2.n;

	int own_flag_state = state1;
	int enemy_flag_state = state2;

	//don't care about pos or alpha ... just the now.

	EntityId localClientId = g_pGame->GetIGameFramework()->GetClientActorId();
	int  friendlyTeam = g_pGame->GetGameRules()->GetTeam(localClientId);

	if( friendlyTeam == 2 )
	{
		EntityId tmp_flag_id = own_flag;
		own_flag = enemy_flag;
		enemy_flag= tmp_flag_id;

		EntityId tmp_base_id = own_base;
		own_base = enemy_base;
		enemy_base = tmp_base_id;

		EntityId tmp_flag_carrier = own_flag_carrier;
		own_flag_carrier = enemy_flag_carrier;
		enemy_flag_carrier = tmp_flag_carrier;

		int tmp_flag_state = own_flag_state;
		own_flag_state = enemy_flag_state;
		enemy_flag_state = tmp_flag_state;
	}

	static EntityId last_own_flag_carrier   = 0;
	static EntityId last_enemy_flag_carrier = 0;
	CUITagNames* pTagNames = CUITagNames::GetInstance();
	pTagNames->IgnoreEntity( last_own_flag_carrier, false);
	pTagNames->IgnoreEntity( last_enemy_flag_carrier, false);
	pTagNames->IgnoreEntity( own_flag_carrier, true);
	pTagNames->IgnoreEntity( enemy_flag_carrier, true);
	pObjectives->RemoveMissionObjective(last_enemy_flag_carrier);
	pObjectives->RemoveMissionObjective(last_own_flag_carrier);
	last_own_flag_carrier   = own_flag_carrier;
	last_enemy_flag_carrier = enemy_flag_carrier;

	EGameRulesMissionObjectives own_flag_status = EGRMO_Unknown;
	EGameRulesMissionObjectives own_flag_base_status = EGRMO_Unknown;
	// Flag1 shit
	switch( own_flag_state )
	{
	case 0 : // FLAG_STATE_INBASE	= 0;
		own_flag_status = EGRMO_Unknown; // hopeful this won't error or span warnings.
		own_flag_base_status = EGRMO_CTF_Base_with_flag_blue;
		break;
	case 1 : // FLAG_STATE_CARRIED	= 1;
		own_flag_status = EGRMO_CTF_Flag_carried_red; // hopeful this won't error or span warnings.
		own_flag_base_status = EGRMO_CTF_Base_no_flag_blue;
		own_flag = own_flag_carrier;
		break;
	case 2 : // FLAG_STATE_DROPPED	= 2;
		own_flag_status = EGRMO_CTF_Flag_dropped_blue; // hopeful this won't error or span warnings.
		own_flag_base_status = EGRMO_CTF_Base_no_flag_blue;
		break;
	}

	EGameRulesMissionObjectives enemy_flag_status = EGRMO_Unknown;
	EGameRulesMissionObjectives enemy_flag_base_status = EGRMO_Unknown;
	// Flag1 shit
	switch( enemy_flag_state )
	{
	case 0 : // FLAG_STATE_INBASE	= 0;
		enemy_flag_status = EGRMO_Unknown; // hopeful this won't error or span warnings.
		enemy_flag_base_status = EGRMO_CTF_Base_with_flag_red;
		break;
	case 1 : // FLAG_STATE_CARRIED	= 1;
		enemy_flag_status = EGRMO_CTF_Flag_carried_blue; // hopeful this won't error or span warnings.
		enemy_flag_base_status = EGRMO_CTF_Base_no_flag_red;
		enemy_flag = enemy_flag_carrier;
		break;
	case 2 : // FLAG_STATE_DROPPED	= 2;
		enemy_flag_status = EGRMO_CTF_Flag_dropped_red; // hopeful this won't error or span warnings.
		enemy_flag_base_status = EGRMO_CTF_Base_no_flag_red;
		break;
	}

	// Bases are always drawn
	pObjectives->NewMissionObjective( own_base, own_flag_base_status );
	pObjectives->NewMissionObjective( enemy_base, enemy_flag_base_status );

	// Update ScoreElement in JanHUD.
	SHUDEvent e;
	e.eventType = eHUDEvent_OnOwnFlagStatusChanged;
	e.eventIntData = (int)own_flag_base_status;
	CHUD::CallEvent(e);

	// Update ScoreElement in JanHUD.
	e.eventType = eHUDEvent_OnEnemyFlagStatusChanged;
	e.eventIntData = (int)enemy_flag_base_status;
	CHUD::CallEvent(e);

	if( own_flag_status != EGRMO_Unknown ) 
	{
		pObjectives->NewMissionObjective( own_flag, own_flag_status );
	}
	else
	{
		pObjectives->RemoveMissionObjective(own_flag);
	}

	if( !( ( enemy_flag_status == EGRMO_Unknown ) || 
		( enemy_flag_status == EGRMO_CTF_Flag_carried_blue && enemy_flag_carrier == localClientId) ) )
	{
		pObjectives->NewMissionObjective( enemy_flag, enemy_flag_status );
	}
	else
	{
		pObjectives->RemoveMissionObjective(enemy_flag);
	}

	// Update carrying flag bool on ctf.
	e.eventType = eHUDEvent_OnStartCarryingFlag;
	int carrying_flag = 0;
	if( enemy_flag_carrier == localClientId )
	{
		carrying_flag = 1;			
	}

	e.eventIntData = carrying_flag;
	CHUD::CallEvent(e);
	//pHUD->GetRadar()->UpdateFlagState(state1, pos1, alpha1, (EntityId)carrierId1.n, state2, pos2, alpha2, (EntityId)carrierId2.n);

#endif
	return pH->EndFunction();
}

int CScriptBind_HUD::RemoveEntityFromRadar(IFunctionHandler *pH, ScriptHandle entityId)
{
	SHUDEvent hudevent(eHUDEvent_RemoveEntity);
	hudevent.AddData(SHUDEventData((int)entityId.n));
	CHUD::CallEvent(hudevent);

	return pH->EndFunction();
}

int CScriptBind_HUD::ShowKillZoneTime(IFunctionHandler *pH, bool active, int seconds)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
		pHUD->ShowKillAreaWarning(active, seconds);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::StartPlayerFallAndPlay(IFunctionHandler *pH)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
		pHUD->StartPlayerFallAndPlay();
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::OnPlayerVehicleBuilt(IFunctionHandler *pH, ScriptHandle playerId, ScriptHandle vehicleId)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
		pHUD->OnPlayerVehicleBuilt((EntityId)playerId.n, (EntityId)vehicleId.n);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::ShowDeathFX(IFunctionHandler *pH, int type)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
		pHUD->ShowDeathFX(type);*/
/*
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::OnItemBought(IFunctionHandler *pH, bool success, const char* itemName)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if(pHUD)
		pHUD->PlaySound(success?ESound_BuyBeep:ESound_BuyError);

	if(success && itemName && pHUD->GetPowerStruggleHUD())
	{
		pHUD->GetPowerStruggleHUD()->SetLastPurchase(itemName);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::BattleLogEvent(IFunctionHandler *pH, int type, const char *msg)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if(pHUD)
	{
		string p[4];
		for (int i=0;i<pH->GetParamCount()-2;i++)
		{
			switch(pH->GetParamType(3+i))
			{
			case svtPointer:
				{
					ScriptHandle sh;
					pH->GetParam(3+i, sh);

					if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity((EntityId)sh.n))
						p[i]=pEntity->GetName();
				}
				break;
			default:
				{
					ScriptAnyValue value;
					pH->GetParamAny(3+i, value);
					switch(value.GetVarType())
					{
					case svtNumber:
						p[i].Format("%g", value.number);
						break;
					case svtString:
						p[i]=value.str;
						break;
					case svtBool:
						p[i]=value.b?"true":"false";
						break;
					default:
						break;
					}			
				}
				break;
			}
		}

		pHUD->BattleLogEvent(type, msg, p[0].c_str(), p[1].c_str(), p[2].c_str(), p[3].c_str());
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::ShowWarningMessage(IFunctionHandler *pH, int message, const char* text)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if(pHUD)
		pHUD->ShowWarningMessage(EWarningMessages(message), text);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::GetMapGridCoord(IFunctionHandler *pH, float x, float y)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if(pHUD)
	{
		Vec2i grid=pHUD->GetRadar()->GetMapGridPosition(x, y);
		return pH->EndFunction(grid.x, grid.y);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::OpenPDA(IFunctionHandler *pH, bool show, bool buyMenu)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if(!pHUD)
		return pH->EndFunction();

	pHUD->ShowPDA(show, buyMenu);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::SetCurrentCaptureSite(IFunctionHandler *pH, ScriptHandle site)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetPowerStruggleLiteHUD())
		pHUD->GetPowerStruggleLiteHUD()->SetCurrentCaptureSite((EntityId)site.n);
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::ShowCaptureProgress(IFunctionHandler *pH, bool show)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		EHUDGAMERULES gameRules = pHUD->GetGameRules();

		if (gameRules==EHUD_POWERSTRUGGLE)
		{
			if(pHUD->GetPowerStruggleHUD())
				pHUD->GetPowerStruggleHUD()->ShowCaptureProgress(show);
		}
		else if (gameRules==EHUD_POWERSTRUGGLELITE)
		{
			if(pHUD->GetPowerStruggleLiteHUD())
				pHUD->GetPowerStruggleLiteHUD()->ShowCaptureProgress(show);
		}
	}

*/
	return pH->EndFunction();
}

int CScriptBind_HUD::SetCaptureProgress(IFunctionHandler *pH, float progress)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		EHUDGAMERULES gameRules = pHUD->GetGameRules();

		if (gameRules==EHUD_POWERSTRUGGLE)
		{
	if(pHUD && pHUD->GetPowerStruggleHUD())
		pHUD->GetPowerStruggleHUD()->SetCaptureProgress(progress);
		}
		else if (gameRules==EHUD_POWERSTRUGGLELITE)
		{
			if(pHUD->GetPowerStruggleLiteHUD())
				pHUD->GetPowerStruggleLiteHUD()->SetCaptureProgress(progress);
		}
	}
*/
	return pH->EndFunction();
}


int CScriptBind_HUD::SetCaptureContested(IFunctionHandler *pH, bool contested)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		EHUDGAMERULES gameRules = pHUD->GetGameRules();
		if (gameRules==EHUD_POWERSTRUGGLE)
		{
			if(pHUD->GetPowerStruggleHUD())
				pHUD->GetPowerStruggleHUD()->SetCaptureContested(contested);
		}
		else if (gameRules==EHUD_POWERSTRUGGLELITE)
		{
			if(pHUD->GetPowerStruggleLiteHUD())
				pHUD->GetPowerStruggleLiteHUD()->SetCaptureContested(contested);
		}
	}	
*/
	return pH->EndFunction();
}


int CScriptBind_HUD::ShowConstructionProgress(IFunctionHandler *pH, bool show, bool queued, float constructionTime)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		EHUDGAMERULES gameRules = pHUD->GetGameRules();
		if (gameRules==EHUD_POWERSTRUGGLE)
		{
			if(pHUD->GetPowerStruggleHUD())
				pHUD->GetPowerStruggleHUD()->ShowConstructionProgress(show, queued, constructionTime);
		}
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::ShowReviveCycle(IFunctionHandler *pH, bool show)
{
	SHUDEvent hudEvent;
	hudEvent.eventType = eHUDEvent_ShowReviveCycle;
	hudEvent.eventIntData = (int)show;
	CHUD::CallEvent(hudEvent);

	return pH->EndFunction();
}

int CScriptBind_HUD::SpawnGroupInvalid(IFunctionHandler *pH)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if(pHUD && pHUD->GetPowerStruggleHUD())
		pHUD->SpawnPointInvalid();
*/
	return pH->EndFunction();
}


int CScriptBind_HUD::FakeDeath(IFunctionHandler *pH)
{
/*	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && !pHUD->IsFakeDead())
		pHUD->FakeDeath();
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::SetProgressBar(IFunctionHandler *pH, bool show, int progress, const char *text)
{
/*	if (CHUD *pHUD = g_pGame->GetHUD())
	{
		if (show)
			pHUD->ShowProgress(CLAMP(progress, 0, 100), true, 400, 200, text);
		else
			pHUD->ShowProgress();
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::DisplayBigOverlayFlashMessage(IFunctionHandler *pH, const char *msg, float duration, int posX, int posY, Vec3 color)
{
/*	if (CHUD *pHUD = g_pGame->GetHUD())
	{
		pHUD->DisplayBigOverlayFlashMessage(msg, duration, posX, posY, color);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::DisplayFunMessagePostfix(IFunctionHandler *pH, const char* msg, const char* postfix)
{
/*	if (CHUD *pHUD = g_pGame->GetHUD())
	{
		pHUD->DisplayFunMessage(msg, postfix);
	}*/
	return pH->EndFunction();
}

int CScriptBind_HUD::DisplayBonusMessage(IFunctionHandler *pH, const char* msg, int teamId)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetCountdownHUD())
	{
		const wchar_t* localizedText = L"";
		localizedText = pHUD->LocalizeWithParams(msg, false);

		pHUD->GetCountdownHUD()->AddBonusMessage(localizedText, teamId);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::FadeOutBigOverlayFlashMessage(IFunctionHandler *pH)
{
/*	if (CHUD *pHUD = g_pGame->GetHUD())
	{
		pHUD->FadeOutBigOverlayFlashMessage();
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::GetLastInGameSave(IFunctionHandler *pH)
{
/*	string *lastSave = g_pGame->GetMenu()->GetLastInGameSave();

	if (lastSave)
	{
		return pH->EndFunction(lastSave->c_str());
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::TextMessage(IFunctionHandler *pH, int type, float lifetime, const char *text)
{
/*	if (CHUD *pHUD = g_pGame->GetHUD())
	{
		ColorF col(1.0f, 1.0f, 1.0f);

		if (pH->GetParamCount()>3)
		{
			pH->GetParam(4, col.r);
			pH->GetParam(5, col.g);
			pH->GetParam(6, col.b);
		}
		pHUD->DisplayTextMessage(text, lifetime, type, col);
	}

*/	return pH->EndFunction();
}

int CScriptBind_HUD::AddEntitySilhouette(IFunctionHandler *pH, ScriptHandle entityId, float r, float g, float b, float a)
{
	CRY_TODO(23, 09, 09, "Refactor/Remove");

	/*CHUD *pHUD = g_pGame->GetHUD();

	if (pHUD)
	{
		pHUD->GetSilhouettes()->ResetFlowGraphSilhouette((EntityId)entityId.n);
		pHUD->GetSilhouettes()->SetFlowGraphSilhouette(gEnv->pEntitySystem->GetEntity((EntityId)entityId.n), r, g, b, a, -1);
	}*/

	return pH->EndFunction();
}

int CScriptBind_HUD::RemoveEntitySilhouette(IFunctionHandler *pH, ScriptHandle entityId)
{
	CRY_TODO(23, 09, 09, "Refactor/Remove");

	/*CHUD *pHUD = g_pGame->GetHUD();

	if (pHUD)
	{
		pHUD->GetSilhouettes()->ResetFlowGraphSilhouette((EntityId)entityId.n);
	}*/

	return pH->EndFunction();
}

int CScriptBind_HUD::RequestLoadout(IFunctionHandler *pH)
{
	CRY_TODO(23, 09, 09, "Refactor/Remove");

/*
	CHUD *pHUD = g_pGame->GetHUD();
	CryLog("CScriptBind_HUD::RequestLoadout");
	if(pHUD && pHUD->GetInventoryLoadoutHUD())
	{
		pHUD->GetInventoryLoadoutHUD()->RequestCurrentPackage();
	}
*/

	return pH->EndFunction();
}

int CScriptBind_HUD::DisplayFunMessage(IFunctionHandler *pH, const char* msg)
{
	CRY_TODO(23, 09, 09, "Refactor/Remove");

	/*if (CHUD *pHUD = g_pGame->GetHUD())
	{
		pHUD->DisplayFunMessage(msg, NULL);
	}*/
	return pH->EndFunction();
}

// Extraction
int CScriptBind_HUD::ClearPrototypeWeapons(IFunctionHandler *pH)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_EXTRACTION && pHUD->GetExtractionHUD())
	{
		pHUD->GetExtractionHUD()->ClearPrototypeWeapons();
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::AddPrototypeWeapon(IFunctionHandler *pH, ScriptHandle entityId)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_EXTRACTION && pHUD->GetExtractionHUD())
	{
		pHUD->GetExtractionHUD()->AddPrototypeWeapon((EntityId) entityId.n);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::RemovePrototypeWeapon( IFunctionHandler *pH, ScriptHandle entityId )
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_EXTRACTION && pHUD->GetExtractionHUD())
	{
		pHUD->GetExtractionHUD()->RemovePrototypeWeapon((EntityId) entityId.n);
	}
*/
	return pH->EndFunction();
}

// Payload
int CScriptBind_HUD::ClearActivePayloads(IFunctionHandler *pH)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_PAYLOAD && pHUD->GetPayloadHUD())
	{
		pHUD->GetPayloadHUD()->ClearActivePayloads();
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::AddActivePayload(IFunctionHandler *pH, ScriptHandle entityId)
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_PAYLOAD && pHUD->GetPayloadHUD())
	{
		pHUD->GetPayloadHUD()->AddActivePayload((EntityId) entityId.n);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::RemoveActivePayload( IFunctionHandler *pH, ScriptHandle entityId )
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_PAYLOAD && pHUD->GetPayloadHUD())
	{
		pHUD->GetPayloadHUD()->RemoveActivePayload((EntityId) entityId.n);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::SetPayloadBank( IFunctionHandler *pH, ScriptHandle entityId )
{
/*
	CHUD *pHUD = g_pGame->GetHUD();
	if (pHUD && pHUD->GetGameRules() == EHUD_PAYLOAD && pHUD->GetPayloadHUD())
	{
		pHUD->GetPayloadHUD()->SetActiveBank((EntityId) entityId.n);
	}
*/
	return pH->EndFunction();
}

int CScriptBind_HUD::NewMissionObjective( IFunctionHandler *pH, ScriptHandle targetId, int objectiveType, float progress, int priority )
{
	SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
	newMissionObjective.ReserveData(4);
	newMissionObjective.AddData( static_cast<int>(targetId.n) ); /*(EntityId)*/
	newMissionObjective.AddData( objectiveType ); /*(EGameRulesMissionObjectives)*/ 
	newMissionObjective.AddData( progress ); /*float*/
	newMissionObjective.AddData( priority ); /*int*/
	CHUD::CallEvent(newMissionObjective);

	return pH->EndFunction();
}

int CScriptBind_HUD::GetLookAtEntity(IFunctionHandler *pH)
{
	if (IActor* pActor=gEnv->pGame->GetIGameFramework()->GetClientActor())
	{
		if (IGameObject* pGameObject=pActor->GetGameObject())
		{
			if (EntityId lookAtId=pGameObject->GetWorldQuery()->GetLookAtEntityId())
			{
				if (IEntity* pLookAt=gEnv->pEntitySystem->GetEntity(lookAtId))
				{
					return pH->EndFunction(pLookAt->GetScriptTable());
				}
			}
		}
	}
	return pH->EndFunction();
}

int CScriptBind_HUD::SetCaptureBar(IFunctionHandler *pH, float progress)
{
	CUISimpleBar * captureBar = CUISimpleBar::GetInstanceWithName("CaptureProgressBar");
	if (captureBar)
	{
		captureBar->Set(clamp(progress, 0.f, 1.f));
	}
	return pH->EndFunction();
}

int CScriptBind_HUD::TriggerCaptureBarWarning(IFunctionHandler *pH)
{
	CUISimpleBar * captureBar = CUISimpleBar::GetInstanceWithName("CaptureProgressBar");
	if (captureBar)
	{
		captureBar->TriggerWarningFlash();
	}
	return pH->EndFunction();
}
