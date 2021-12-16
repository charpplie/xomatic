/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 7:10:2004   14:19 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_Actor.h"
#include "IMovementController.h"
#include "Item.h"
#include "Game.h"
#include "Player.h"
#include "Alien.h"
#include "GameCVars.h"
#include "Utility/StringUtils.h"
#include "WeaponSystem.h"
#include "Projectile.h"

#include <IGameFramework.h>
#include <IVehicleSystem.h>
#include <IGameObject.h>
#include <Cry_Geo.h>
#include <Cry_GeoDistance.h>
#include <IEntitySystem.h>
#include <IViewSystem.h>
#include "IAIActor.h"

#ifdef WIN64
	#pragma warning(disable: 4244)
#endif

#include "IPersonalBehaviorTree.h"

//------------------------------------------------------------------------
CScriptBind_Actor::CScriptBind_Actor(ISystem *pSystem)
: m_pSystem(pSystem),
	m_pGameFW(pSystem->GetIGame()->GetIGameFramework())
{
	Init(pSystem->GetIScriptSystem(), pSystem, 1);

	//////////////////////////////////////////////////////////////////////////
	// Init tables.
	//////////////////////////////////////////////////////////////////////////
	m_pParams.Create(m_pSS);

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Actor::

  SCRIPT_REG_FUNC(DumpActorInfo);
	SCRIPT_REG_FUNC(Revive);
	SCRIPT_REG_FUNC(Kill);
	SCRIPT_REG_FUNC(RagDollize);
	SCRIPT_REG_FUNC(SetStats);
	SCRIPT_REG_FUNC(SetParams);
	SCRIPT_REG_FUNC(GetParams);
	SCRIPT_REG_FUNC(GetHeadDir);
	SCRIPT_REG_FUNC(PostPhysicalize);
	SCRIPT_REG_FUNC(GetChannel);
	SCRIPT_REG_FUNC(IsPlayer);
	SCRIPT_REG_FUNC(IsLocalClient);
	SCRIPT_REG_FUNC(LinkToEntity);
	SCRIPT_REG_TEMPLFUNC(GetLinkedVehicleId, "");
  SCRIPT_REG_FUNC(IsGhostPit);
	SCRIPT_REG_FUNC(IsFlying);
	SCRIPT_REG_TEMPLFUNC(SetAngles,"vAngles");
	SCRIPT_REG_FUNC(GetAngles);
	SCRIPT_REG_TEMPLFUNC(AddAngularImpulse,"vAngular,deceleration,duration");
	SCRIPT_REG_TEMPLFUNC(SimulateOnAction,"action,mode,value");
	SCRIPT_REG_TEMPLFUNC(SetMovementTarget,"pos,target,up,speed");
	SCRIPT_REG_TEMPLFUNC(CameraShake,"amount,duration,frequency,pos");
	SCRIPT_REG_TEMPLFUNC(SetViewShake,"shakeAngle, shakeShift, duration, frequency, randomness");
	SCRIPT_REG_FUNC(VectorToLocal);

	SCRIPT_REG_TEMPLFUNC(SetExtensionActivation,"extension,bActivate");
	SCRIPT_REG_TEMPLFUNC(SetExtensionParams,"extension,params");
	SCRIPT_REG_TEMPLFUNC(GetExtensionParams,"extension,params");

	SCRIPT_REG_TEMPLFUNC(SetInventoryAmmo, "ammo, amount");
	SCRIPT_REG_TEMPLFUNC(AddInventoryAmmo, "ammo, amount");
	SCRIPT_REG_TEMPLFUNC(GetInventoryAmmo, "ammo");
	SCRIPT_REG_FUNC(RefillAllAmmo);

	SCRIPT_REG_TEMPLFUNC(SetHealth,"health");
	SCRIPT_REG_TEMPLFUNC(DamageInfo,"shooterID, targetID, weaponID, projectileID, damage, damageType, hitDirection");
	SCRIPT_REG_TEMPLFUNC(GetLowHealthThreshold, "")
	SCRIPT_REG_TEMPLFUNC(SetMaxHealth,"health");
	SCRIPT_REG_FUNC(GetHealth);
	SCRIPT_REG_FUNC(GetMaxHealth);
	SCRIPT_REG_FUNC(GetArmor);
	SCRIPT_REG_FUNC(GetMaxArmor);

	SCRIPT_REG_TEMPLFUNC(SetPhysicalizationProfile, "profile");
	SCRIPT_REG_TEMPLFUNC(GetPhysicalizationProfile, "");

	SCRIPT_REG_TEMPLFUNC(GetClosestAttachment, "characterSlot, testPos, maxDistance, suffix");
  SCRIPT_REG_TEMPLFUNC(AttachVulnerabilityEffect, "characterSlot, partid, hitPos, radius, effect, attachmentIdentifier");
  SCRIPT_REG_TEMPLFUNC(ResetVulnerabilityEffects, "characterSlot");
  SCRIPT_REG_TEMPLFUNC(GetCloseColliderParts, "characterSlot, hitPos, radius");
	SCRIPT_REG_TEMPLFUNC(QueueAnimationState,"animationState");
	SCRIPT_REG_TEMPLFUNC(CreateCodeEvent,"params");
	SCRIPT_REG_FUNC(GetCurrentAnimationState);
	SCRIPT_REG_TEMPLFUNC(SetAnimationInput,"name,value");
	SCRIPT_REG_FUNC(PauseAnimationGraph);
	SCRIPT_REG_FUNC(ResumeAnimationGraph);
	SCRIPT_REG_TEMPLFUNC(TrackViewControlled,"characterSlot");
	SCRIPT_REG_TEMPLFUNC(SetSpectatorMode,"mode, target");
	SCRIPT_REG_TEMPLFUNC(GetSpectatorMode,"");
	SCRIPT_REG_TEMPLFUNC(GetSpectatorState,"");
	SCRIPT_REG_TEMPLFUNC(GetSpectatorTarget, "");
	
	SCRIPT_REG_TEMPLFUNC(Fall,"hitPosX, hitPosY, hitPosZ");
	SCRIPT_REG_TEMPLFUNC(LooseHelmet,"hitDir, hitPos");
	
	SCRIPT_REG_TEMPLFUNC(GetExtraHitLocationInfo, "slot, partId");
	SCRIPT_REG_TEMPLFUNC(StandUp,"");

	SCRIPT_REG_TEMPLFUNC(SetForcedLookDir, "dir");
	SCRIPT_REG_TEMPLFUNC(ClearForcedLookDir, "");

	SCRIPT_REG_TEMPLFUNC(IsPerkActive,"perkIndex");
	SCRIPT_REG_TEMPLFUNC(ActivateNanoSuit,"on");
	SCRIPT_REG_TEMPLFUNC(SetNanoSuitMode,"mode");
	SCRIPT_REG_FUNC(GetNanoSuitMode);
	SCRIPT_REG_FUNC(GetNanoSuitEnergy);
	SCRIPT_REG_TEMPLFUNC(SetNanoSuitEnergy,"energy");
	SCRIPT_REG_TEMPLFUNC(NanoSuitHit,"damage");
	SCRIPT_REG_TEMPLFUNC(SendPlayerSuitEvent,"eventIndex");

	SCRIPT_REG_TEMPLFUNC(SetCloakMode, "mode");

	SCRIPT_REG_TEMPLFUNC(PlayAction, "action");
	SCRIPT_REG_TEMPLFUNC(RequestMechanic, "mechanic");
	SCRIPT_REG_TEMPLFUNC(PlayerSetViewAngles,"vAngles");

	//------------------------------------------------------------------------
	// NETWORK
	//------------------------------------------------------------------------
	SCRIPT_REG_TEMPLFUNC(CheckInventoryRestrictions, "itemClassName");
	SCRIPT_REG_TEMPLFUNC(HolsterItem, "holster");
	SCRIPT_REG_TEMPLFUNC(DropItem, "itemId");
	SCRIPT_REG_TEMPLFUNC(PickUpItem, "itemId");
	SCRIPT_REG_TEMPLFUNC(SelectNextItem, "direction, keepHistory, category");

	SCRIPT_REG_TEMPLFUNC(SelectItemByName, "");
	SCRIPT_REG_TEMPLFUNC(SelectItem, "");
	SCRIPT_REG_TEMPLFUNC(SelectLastItem, "");

	SCRIPT_REG_TEMPLFUNC(EnableSwitchingItems, "enable");
	SCRIPT_REG_TEMPLFUNC(EnableIronSights, "enable");
	SCRIPT_REG_TEMPLFUNC(EnablePickingUpItems, "enable");
  	
	SCRIPT_REG_TEMPLFUNC(ForceItemSuffix, "suffix");
	SCRIPT_REG_TEMPLFUNC(ResetItemSuffix, "");

	//------------------------------------------------------------------------
	
	SCRIPT_REG_TEMPLFUNC(CreateIKLimb,"slot,limbName,rootBone,midBone,endBone,flags");
	SCRIPT_REG_TEMPLFUNC(CreateFXBoneAttachment, "pH, iCharacterSlot, szBoneName,*szAttachmentName");

	SCRIPT_REG_TEMPLFUNC(ResetScores, "");
	SCRIPT_REG_TEMPLFUNC(RenderScore, "");

  SCRIPT_REG_TEMPLFUNC(SetSearchBeam, "dir");
	
	m_pSS->SetGlobalValue("ZEROG_AREA_ID", ZEROG_AREA_ID);

	m_pSS->SetGlobalValue("IKLIMB_LEFTHAND", IKLIMB_LEFTHAND);
	m_pSS->SetGlobalValue("IKLIMB_RIGHTHAND", IKLIMB_RIGHTHAND);

	m_pSS->SetGlobalValue("NANOMODE_SPEED", 0);
	m_pSS->SetGlobalValue("NANOMODE_STRENGTH", 0);
	m_pSS->SetGlobalValue("NANOMODE_CLOAK", 1);
	m_pSS->SetGlobalValue("NANOMODE_DEFENSE", 2);

	m_pSS->SetGlobalValue("eCloakMode_Disabled", eCloakMode_Disabled);
	m_pSS->SetGlobalValue("eCloakMode_Enabled", eCloakMode_Enabled);

	m_pSS->SetGlobalValue("BONE_SPINE", BONE_SPINE);
	m_pSS->SetGlobalValue("BONE_HEAD", BONE_HEAD);
	m_pSS->SetGlobalValue("BONE_ALIEN_HEAD", BONE_ALIEN_HEAD);
	m_pSS->SetGlobalValue("BONE_AUTOAIMTARGET", BONE_AUTOAIMTARGET);
	m_pSS->SetGlobalValue("BONE_AUTOAIMTARGET_ALIEN", BONE_AUTOAIMTARGET_ALIEN);

	SCRIPT_REG_GLOBAL(eNanoSuitEvent_SUITPOWER);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_SHOT);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_HIT);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_EXPLOSION);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_EMP_DISCHARGE);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_DISABLE_SUIT);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_REBOOT_SUIT);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_THROW);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_MELEE);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_MELEE_HIT);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_ACTION);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_POWERJUMP);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_SLIDE_ACTIVE);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_ENEMY_RAMMING_RUN);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_ENEMY_RAMMING_JUMP);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_ENEMY_RAMMING_SLIDE);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_COMBO_LINK);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_KVOLT_CHARGE);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_ENERGY_DRAIN);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_CLOAK);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_UNCLOAK);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_ONAFTER_MODE_ACTIVATION);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_CLOAK_INTERFERENCE);
	SCRIPT_REG_GLOBAL(eNanoSuitEvent_SUIT_CLIENTSTARTUP);

}

//------------------------------------------------------------------------
CScriptBind_Actor::~CScriptBind_Actor()
{
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Actor::PlayerSetViewAngles(IFunctionHandler *pH,Ang3 vAngles)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor->IsPlayer())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
		pPlayer->SetViewRotation( Quat( vAngles ) );
	}

	return pH->EndFunction();
}


//------------------------------------------------------------------------
void CScriptBind_Actor::AttachTo(IActor *pActor)
{
	IScriptTable *pScriptTable = pActor->GetEntity()->GetScriptTable();

	if (pScriptTable)
	{
		SmartScriptTable thisTable(m_pSS);

		thisTable->SetValue("__this", ScriptHandle(pActor->GetEntityId()));
		thisTable->Delegate(GetMethodsTable());

		pScriptTable->SetValue("actor", thisTable);
	}
}

//------------------------------------------------------------------------
CActor *CScriptBind_Actor::GetActor(IFunctionHandler *pH)
{
	void *pThis = pH->GetThis();

	if (pThis)
	{
		IActor *pActor = m_pGameFW->GetIActorSystem()->GetActor((EntityId)(UINT_PTR)pThis);
		if (pActor)
			return static_cast<CActor *>(pActor);
	}

	return 0;
}


//------------------------------------------------------------------------
int CScriptBind_Actor::DumpActorInfo(IFunctionHandler *pH)
{
  CActor *pActor = GetActor(pH);
  if (!pActor)
    return pH->EndFunction();

  pActor->DumpActorInfo();
  
  return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::Revive(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->Revive(CActor::kRFR_ScriptBind);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::Kill(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->Kill();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::RagDollize(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
	//pActor->RagDollize();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
//set some status of the actor
int CScriptBind_Actor::SetStats(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
	{
		SmartScriptTable params;

		if (pH->GetParamType(1) != svtNull && pH->GetParam(1, params))
			pActor->SetStats(params);
	}
	
	return pH->EndFunction();
}

//------------------------------------------------------------------------
//set the actor params, pass the params table to the actor
int CScriptBind_Actor::SetParams(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
	{
		SmartScriptTable params;

		if (pH->GetParamType(1) != svtNull && pH->GetParam(1, params))
			pActor->SetParams(params);
	}
	
	return pH->EndFunction();
}

//------------------------------------------------------------------------
//get some infos from the actor
int CScriptBind_Actor::GetParams(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor && pActor->GetParams(m_pParams))
	{
		return pH->EndFunction(m_pParams);
	}
	else
	{
		return pH->EndFunction();
	}
}

// has to be changed! (maybe bone position)
//------------------------------------------------------------------------
int CScriptBind_Actor::GetHeadDir(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	Vec3 headDir = FORWARD_DIRECTION;

	if (IMovementController * pMC = pActor->GetMovementController())
	{
		SMovementState ms;
		pMC->GetMovementState( ms );
		headDir = ms.eyeDirection;
	}

	return pH->EndFunction(Script::SetCachedVector( headDir, pH, 1 ));
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetChannel(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return pH->EndFunction( (int)pActor->GetChannelId() );
}

//------------------------------------------------------------------------
int CScriptBind_Actor::IsPlayer(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor && pActor->IsPlayer())
		return pH->EndFunction(1);
	else
		return pH->EndFunction();
} 

//------------------------------------------------------------------------
int CScriptBind_Actor::IsLocalClient(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor && pActor->IsClient())
		return pH->EndFunction(1);
	else
		return pH->EndFunction();
} 

//------------------------------------------------------------------------
int CScriptBind_Actor::PostPhysicalize(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
		pActor->PostPhysicalize();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetLinkedVehicleId(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
	{
		ScriptHandle vehicleHandle;

		if (IVehicle* pVehicle = pActor->GetLinkedVehicle())
		{
			vehicleHandle.n = pVehicle->GetEntityId();
			return pH->EndFunction(vehicleHandle);
		}
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::LinkToEntity(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
	{
		IEntity *pEntity(0);
		ScriptHandle entityId;

		entityId.n = 0;
		if (pH->GetParamType(1) != svtNull)
			pH->GetParam(1, entityId);

		pActor->LinkToEntity(entityId.n);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
//TOFIX:rendundant with CScriptBind_Entity::SetAngles
int CScriptBind_Actor::SetAngles(IFunctionHandler *pH,Ang3 vAngles)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
		pActor->SetAngles(vAngles);

	return pH->EndFunction();
}

int CScriptBind_Actor::GetAngles(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	Ang3 angles(0,0,0);

	if (pActor)
		angles = pActor->GetAngles();

	return pH->EndFunction( Script::SetCachedVector( (Vec3)angles, pH, 1 ) );
}

int CScriptBind_Actor::AddAngularImpulse(IFunctionHandler *pH,Ang3 vAngular,float deceleration,float duration)
{
	CActor *pActor = GetActor(pH);
	if (pActor)
		pActor->AddAngularImpulse(vAngular,deceleration,duration);

	return pH->EndFunction();
}

int CScriptBind_Actor::SimulateOnAction(IFunctionHandler *pH,const char *action,int mode,float value)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
		pActor->OnAction(action,mode,value);
		
	return pH->EndFunction();
}

int CScriptBind_Actor::SetMovementTarget(IFunctionHandler *pH, Vec3 pos, Vec3 target, Vec3 up, float speed )
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
		pActor->SetMovementTarget(pos,target,up,speed);
		
	return pH->EndFunction();
}

int CScriptBind_Actor::CameraShake(IFunctionHandler *pH,float amount,float duration,float frequency,Vec3 pos)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

  const char* source = "";
  if (pH->GetParamType(5) != svtNull)
    pH->GetParam(5, source);
    
	pActor->CameraShake(amount,0,duration,frequency,pos,0,source);
		
	return pH->EndFunction();
}

int CScriptBind_Actor::SetViewShake(IFunctionHandler *pH, Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	EntityId actorId = pActor->GetEntityId();
	IView* pView = m_pGameFW->GetIViewSystem()->GetViewByEntityId(actorId);
	if (pView)
	{
		const int SCRIPT_SHAKE_ID = 42;
		pView->SetViewShake(shakeAngle, shakeShift, duration, frequency, randomness, SCRIPT_SHAKE_ID);
	}

	return pH->EndFunction();
}

int CScriptBind_Actor::VectorToLocal(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	Vec3 vector;
	if (!pH->GetParam(1, vector))
		return pH->EndFunction();

	pActor->VectorToLocal(vector);
		
	return pH->EndFunction(Script::SetCachedVector( vector, pH, 2 ));
}


//------------------------------------------------------------------------
int CScriptBind_Actor::SetExtensionActivation(IFunctionHandler *pH, const char *extension, bool activation)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	bool ok = false;
	if (pActor)
	{
		if (activation)
			ok = pActor->GetGameObject()->ActivateExtension(extension);
		else
		{
			pActor->GetGameObject()->DeactivateExtension(extension);
			ok = true;
		}
	}
	if (!ok)
		pH->GetIScriptSystem()->RaiseError("Failed to %s extension %s", activation? "enable" : "disable", extension);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetExtensionParams(IFunctionHandler* pH, const char *extension, SmartScriptTable params)
{
	CActor * pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	bool ok = false;
	if (pActor)
		ok = pActor->GetGameObject()->SetExtensionParams(extension, params);
	if (!ok)
		pH->GetIScriptSystem()->RaiseError("Failed to set params for extension %s", extension);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetExtensionParams(IFunctionHandler* pH, const char *extension, SmartScriptTable params)
{
	CActor * pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	bool ok = false;
	if (pActor)
		ok = pActor->GetGameObject()->GetExtensionParams(extension, params);
	if (!ok)
		pH->GetIScriptSystem()->RaiseError("Failed to set params for extension %s", extension);
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetInventoryAmmo(IFunctionHandler *pH, const char *ammo, int amount)
{
	CActor * pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	IInventory *pInventory=pActor->GetInventory();
	if (!pInventory)
		return pH->EndFunction();

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo);
	assert(pClass);

	int capacity = pInventory->GetAmmoCapacity(pClass);
	int current = pInventory->GetAmmoCount(pClass);
	if((!gEnv->IsEditor()) && (amount > capacity))
	{
		if(pActor->IsClient())
			SAFE_HUD_FUNC(DisplayFlashMessage("@ammo_maxed_out", 2, ColorF(1.0f, 0,0), true, (string("@")+pClass->GetName()).c_str()));

		//If still there's some place, full inventory to maximum...
		if(current<capacity)
		{
			pInventory->SetAmmoCount(pClass,capacity);
		}
	}
	else
	{
		pInventory->SetAmmoCount(pClass, amount);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::AddInventoryAmmo(IFunctionHandler *pH, const char *ammo, int amount)
{
	CActor * pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	IInventory *pInventory=pActor->GetInventory();
	if (!pInventory)
		return pH->EndFunction();

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo);
	assert(pClass);

	int capacity = pInventory->GetAmmoCapacity(pClass);
	int current = pInventory->GetAmmoCount(pClass);
	if((!gEnv->IsEditor()) && (amount > capacity))
	{
		if(pActor->IsClient())
			SAFE_HUD_FUNC(DisplayFlashMessage("@ammo_maxed_out", 2, ColorF(1.0f, 0,0), true, (string("@")+pClass->GetName()).c_str()));

		//If still there's some place, full inventory to maximum...

		if(current<capacity)
		{
			pInventory->SetAmmoCount(pClass,capacity);
			if (gEnv->bServer)
				pActor->GetGameObject()->InvokeRMI(CActor::ClAddAmmo(), CActor::AmmoParams(ammo, amount), eRMI_ToRemoteClients);
		}
	}
	else
	{
		pInventory->SetAmmoCount(pClass, amount);
		if (gEnv->bServer)
			pActor->GetGameObject()->InvokeRMI(CActor::ClAddAmmo(), CActor::AmmoParams(ammo, amount), eRMI_ToRemoteClients);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetInventoryAmmo(IFunctionHandler *pH, const char *ammo)
{
	CActor * pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	IInventory *pInventory=pActor->GetInventory();
	if (!pInventory)
		return pH->EndFunction();

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo);
	assert(pClass);
	return pH->EndFunction(pInventory->GetAmmoCount(pClass));
}

//------------------------------------------------------------------------
int CScriptBind_Actor::RefillAllAmmo(IFunctionHandler* pH)
{
	CActor * pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	IInventory *pInventory=pActor->GetInventory();
	if (!pInventory)
		return pH->EndFunction();

	bool isLocalClient = pActor->IsClient();
	IItemSystem* pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
	IPersistantDebug* pPersistantDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
	CRY_ASSERT(pItemSystem);
	CRY_ASSERT(pPersistantDebug);

	CryFixedStringT<64> hudMessage = "";
	const ColorF hudMessageColor = (0.0f, 1.0f, 1.0f, 1.0f);

	const int itemCount = pInventory->GetCount();
	for (int i = 0; i < itemCount; ++i)
	{
		IItem* pItem = pItemSystem->GetItem(pInventory->GetItem(i));

		CWeapon* pWeapon = (pItem && pItem->GetIWeapon()) ? static_cast<CWeapon*>(pItem->GetIWeapon()) : NULL;
		if (pWeapon)
		{
			const int fireModeCount = pWeapon->GetNumOfFireModes();
			for (int j = 0; j < fireModeCount; ++j)
			{
				IFireMode* pFireMode = pWeapon->GetFireMode(j);
				CRY_ASSERT(pFireMode);

				IEntityClass* pAmmoTypeClass = pFireMode->GetAmmoType();
				if (pAmmoTypeClass)
				{
					const int ammoTypeCount = pInventory->GetAmmoCount(pAmmoTypeClass);
					const int ammoTypeCapacity = pInventory->GetAmmoCapacity(pAmmoTypeClass);

					if (ammoTypeCount < ammoTypeCapacity)
					{
						CRY_TODO(19, 3, 2010, "Add HUD hint for ammo full.");
						pWeapon->SetInventoryAmmoCount(pAmmoTypeClass, ammoTypeCapacity);
					}
				}

			}
		}
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetHealth(IFunctionHandler *pH, float health)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
		pActor->SetHealth((int)health);

	return pH->EndFunction();
}
//------------------------------------------------------------------------
int CScriptBind_Actor::DamageInfo(IFunctionHandler *pH, ScriptHandle shooter, ScriptHandle target, ScriptHandle weapon, ScriptHandle projectile, float damage, const char *damageType, Vec3 hitDirection)
{
	EntityId shooterID = shooter.n;
	EntityId targetID = target.n;
	EntityId weaponID = weapon.n;
	EntityId projectileID = projectile.n;
	CProjectile *pProjectile = g_pGame->GetWeaponSystem()->GetProjectile(projectileID);
	IEntityClass *pProjectileClass = (pProjectile ? pProjectile->GetEntity()->GetClass() : 0);
	CActor *pActor = GetActor(pH);
	if (pActor && pProjectileClass)
	{
		pActor->DamageInfo(shooterID, weaponID, pProjectileClass, damage, damageType, hitDirection);
	}
	return pH->EndFunction();
}

int CScriptBind_Actor::GetLowHealthThreshold(IFunctionHandler *pH)
{
	return pH->EndFunction(g_pGameCVars->g_playerLowHealthThreshold);
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetMaxHealth(IFunctionHandler *pH, float health)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->SetMaxHealth((int)health);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetHealth(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return pH->EndFunction(pActor->GetHealth());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetMaxHealth(IFunctionHandler *pH)
{
  CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return pH->EndFunction(pActor->GetMaxHealth());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetArmor(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return pH->EndFunction(pActor->GetArmor());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetMaxArmor(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return pH->EndFunction(pActor->GetMaxArmor());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::QueueAnimationState(IFunctionHandler *pH, const char *animationState)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->QueueAnimationState(animationState);
	
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::CreateCodeEvent(IFunctionHandler *pH,SmartScriptTable params)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return (pActor->CreateCodeEvent(params));
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetCurrentAnimationState(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	const char * value = "<no state>";
	if (IAnimationGraphState * pState = pActor->GetAnimationGraphState())
		value = pState->GetCurrentStateName();

	return pH->EndFunction(value);
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetAnimationInput( IFunctionHandler *pH, const char * inputID, const char * value )
{
	CActor *pActor = GetActor(pH);
	if (pActor)
		return pH->EndFunction(pActor->SetAnimationInput(inputID,value));

	return pH->EndFunction();
}

int CScriptBind_Actor::TrackViewControlled( IFunctionHandler *pH, int characterSlot )
{
	CActor *pActor = GetActor(pH);
	if (pActor)
	{
		ICharacterInstance *pCharacter = pActor->GetEntity()->GetCharacter(characterSlot);
		if (pCharacter)
			return pH->EndFunction((pCharacter->GetISkeletonAnim()->GetTrackViewStatus()?true:false));
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::PauseAnimationGraph(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (pActor)
		pActor->GetAnimationGraphState()->Pause(true, eAGP_PlayAnimationNode);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::ResumeAnimationGraph(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (pActor)
		pActor->GetAnimationGraphState()->Pause(false, eAGP_PlayAnimationNode);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetSpectatorMode(IFunctionHandler *pH, int mode, ScriptHandle targetId)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->SetSpectatorModeAndOtherEntId(mode, (EntityId)targetId.n);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetSpectatorMode(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	return pH->EndFunction(pActor->GetSpectatorMode());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetSpectatorState(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	return pH->EndFunction(pActor->GetSpectatorState());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetSpectatorTarget(IFunctionHandler* pH)
{
	CActor* pActor = GetActor(pH);
	if(!pActor)
		return pH->EndFunction();

	return pH->EndFunction(pActor->GetSpectatorTarget());
}

//------------------------------------------------------------------------
int CScriptBind_Actor::Fall(IFunctionHandler *pH, Vec3 hitPos)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	// [Mikko] 11.10.2007 - Moved the check here, since it was causing too much trouble in CActor.Fall().
	// The point of this filtering is to mostly mask out self-induced collision damage on friendly NPCs
	// which are playing special animations.
	if(!g_pGameCVars->g_enableFriendlyFallAndPlay)
	{
		if (IAnimatedCharacter* pAC = pActor->GetAnimatedCharacter())
		{
			if ((pAC->GetPhysicalColliderMode() == eColliderMode_NonPushable) ||
				(pAC->GetPhysicalColliderMode() == eColliderMode_PushesPlayersOnly))
			{
				// Only mask for player friendly NPCs.
				if (pActor->GetEntity() && pActor->GetEntity()->GetAI())
				{
					IAIObject* pAI = pActor->GetEntity()->GetAI();
					IAIActor* pAIActor = pAI->CastToIAIActor();
					if (pAIActor && pAIActor->GetParameters().m_nSpecies == 0)
					{
						return pH->EndFunction();
					}
				}
			}
		}
	}

	pActor->Fall(hitPos);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::LooseHelmet(IFunctionHandler *pH, Vec3 hitDir, Vec3 hitPos)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	return pH->EndFunction(pActor->LooseHelmet(hitDir, hitPos));
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetExtraHitLocationInfo(IFunctionHandler *pH, int slot, int partId)
{
	CActor* pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	IEntity* pEntity = pActor->GetEntity();
	if (!pEntity)
		return pH->EndFunction();

	ICharacterInstance* pCharacter = pActor->GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return pH->EndFunction();

	IPhysicalEntity* pPE = pCharacter->GetISkeletonPose()->GetCharacterPhysics();
	if (!pPE)
		return pH->EndFunction();

	pe_status_pos posStatus;
	pe_params_part part;
	part.partid = partId;

	if (!pPE->GetParams(&part) || !pPE->GetStatus(&posStatus))
		return pH->EndFunction();

	SmartScriptTable result = Script::GetCachedTable(pH, 3);

	Matrix34 worldTM = Matrix34::Create(Vec3(posStatus.scale, posStatus.scale, posStatus.scale), posStatus.q, posStatus.pos);
	Matrix34 partWorldTM = worldTM * Matrix34::Create(Vec3(part.scale, part.scale, part.scale), part.q, part.pos);

	result->SetValue("partId", partId);
	result->SetValue("ipart", part.ipart);
	result->SetValue("mass", part.mass);
	result->SetValue("scale", part.scale);
	result->SetValue("pos", partWorldTM.GetTranslation());
	result->SetValue("dir", partWorldTM.GetColumn1());

	ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
	if (!pSkeletonPose)
		return pH->EndFunction();

	result->SetToNull("attachmentName");
	result->SetToNull("boneName");

	// 1000 is a magic number in CryAnimation
	// see CAttachmentManager::PhysicalizeAttachment
	const int firstAttachmentPartId = 1000;
	
	if (partId >= firstAttachmentPartId)
	{
		IAttachmentManager *pAttchmentManager = pCharacter->GetIAttachmentManager();
		if (IAttachment* pAttachment = pAttchmentManager->GetInterfaceByIndex(partId-firstAttachmentPartId))
			result->SetValue("attachmentName", pAttachment->GetName());
	}
	else
	{
		if (const char* boneName = pSkeletonPose->GetJointNameByID(pSkeletonPose->getBonePhysParentOrSelfIndex(partId)))
			result->SetValue("boneName", boneName);
	}

	return pH->EndFunction(result);
}


//------------------------------------------------------------------------
int CScriptBind_Actor::StandUp(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->StandUp();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetForcedLookDir(IFunctionHandler *pH, CScriptVector dir)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(GetActor(pH));
	if (pPlayer)
	{
		pPlayer->SetForcedLookDir(dir.Get());
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::ClearForcedLookDir(IFunctionHandler *pH)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(GetActor(pH));
	if (pPlayer)
	{
		pPlayer->ClearForcedLookDir();
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetPhysicalizationProfile(IFunctionHandler *pH, const char *profile)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	
	uint32 p = 0;
	if (!stricmp(profile, "alive"))
		p = eAP_Alive;
	else if (!stricmp(profile, "unragdoll"))
		p = eAP_NotPhysicalized;
	else if (!stricmp(profile, "ragdoll"))
	{
		if (!pActor->GetLinkedVehicle())
			p = eAP_Ragdoll;
		else
			p = eAP_Alive;
	}
	else if (!stricmp(profile, "spectator"))
		p = eAP_Spectator;
	else if (!stricmp(profile, "sleep"))
		p = eAP_Sleep;
	else
		return pH->EndFunction();

	//Don't turn ragdoll while grabbed
	if(p==eAP_Ragdoll && !pActor->CanRagDollize())
		return pH->EndFunction();

	pActor->GetGameObject()->SetAspectProfile(eEA_Physics, p);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetPhysicalizationProfile(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	uint8 profile=pActor->GetGameObject()->GetAspectProfile(eEA_Physics);
	const char *profileName;
	if (profile == eAP_Alive)
		profileName="alive";
	else if (profile == eAP_NotPhysicalized)
		profileName = "unragdoll";
	else if (profile == eAP_Ragdoll)
		profileName = "ragdoll";
	else if (profile == eAP_Sleep)
		profileName = "sleep";
	else if (profile == eAP_Spectator)
		profileName = "spectator";
	else
		return pH->EndFunction();

	return pH->EndFunction(profileName);
}


//------------------------------------------------------------------------
int CScriptBind_Actor::AttachVulnerabilityEffect(IFunctionHandler *pH, int characterSlot, int partid, Vec3 hitPos, float radius, const char* effect, const char* attachmentIdentifier)
{
  CActor *pActor = GetActor(pH);  
	if (!pActor)
		return pH->EndFunction();

  IEntity* pEntity = pActor->GetEntity();
  ICharacterInstance* pChar = pEntity->GetCharacter(characterSlot);

  if (!pChar || !effect)  
    return pH->EndFunction();

  //fallback: use nearest attachment
  float minDiff = radius*radius;  
  IAttachment* pClosestAtt = 0;
  
  IAttachmentManager* pMan = pChar->GetIAttachmentManager();
  for (int i=0; i<pMan->GetAttachmentCount(); ++i)
  {
    IAttachment* pAtt = pMan->GetInterfaceByIndex(i);
    
    float diff = (hitPos - pAtt->GetAttWorldAbsolute().t).len2();        
    if (diff < minDiff)
    {
      // only use specified attachments 
      if (attachmentIdentifier[0] && !strstr(pAtt->GetName(), attachmentIdentifier))
        continue;

      minDiff = diff; 
      pClosestAtt = pAtt;      
    }   
    //CryLog("diff: %.2f, att: %s", diff, attName.c_str());
  }

  if (!pClosestAtt)
    return pH->EndFunction();

  //CryLog("AttachVulnerabilityEffect: closest att %s, attaching effect %s", pClosestAtt->GetName(), effect);
  
  CEffectAttachment *pEffectAttachment = new CEffectAttachment(effect, Vec3(ZERO), Vec3(0,1,0), 1.f);

  pClosestAtt->AddBinding(pEffectAttachment);
  pClosestAtt->HideAttachment(0);
  
  return pH->EndFunction(pClosestAtt->GetName());    
}

int CScriptBind_Actor::GetClosestAttachment(IFunctionHandler *pH, int characterSlot, Vec3 testPos, float maxDistance, const char* suffix)
{
  CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

  IEntity* pEntity = pActor->GetEntity();
  ICharacterInstance* pChar = pEntity->GetCharacter(characterSlot);

  if (!pChar)  
    return pH->EndFunction();

  //fallback: use nearest attachment
  float minDiff = maxDistance*maxDistance;  
  IAttachment* pClosestAtt = 0;
  
  IAttachmentManager* pMan = pChar->GetIAttachmentManager();
  int count = pMan->GetAttachmentCount();
  
  for (int i=0; i<count; ++i)
  {
    IAttachment* pAtt = pMan->GetInterfaceByIndex(i);		
		
    if (pAtt->IsAttachmentHidden() || !pAtt->GetIAttachmentObject())
			continue;
		
		AABB bbox(AABB::CreateTransformedAABB(Matrix34(pAtt->GetAttWorldAbsolute()),pAtt->GetIAttachmentObject()->GetAABB()));
		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(bbox,false,ColorB(255,0,0,100),eBBD_Faceted);
		
    //float diff = (testPos - pAtt->GetWMatrix().GetTranslation()).len2();
		float diff((testPos - bbox.GetCenter()).len2());
		
    if (diff < minDiff)
    {
      //CryLogAlways("%s distance: %.1f", pAtt->GetName(), sqrt(diff));

      if (suffix[0] && !strstr(pAtt->GetName(), suffix))
        continue;

      minDiff = diff; 
      pClosestAtt = pAtt;      
    }
  }

  if (!pClosestAtt)
    return pH->EndFunction();
  
	//FIXME FIXME: E3 hack
	char attachmentName[64];
	cry_copyStringUntilFindChar(attachmentName, pClosestAtt->GetName(), sizeof(attachmentName), '.');

	strlwr(attachmentName);
	//

  return pH->EndFunction(attachmentName);    
}

//------------------------------------------------------------------------
int CScriptBind_Actor::ResetVulnerabilityEffects(IFunctionHandler *pH, int characterSlot)
{
  CActor *pActor = GetActor(pH);  
	if (!pActor)
		return pH->EndFunction();

  IEntity* pEntity = pActor->GetEntity();

  ICharacterInstance* pChar = pEntity->GetCharacter(characterSlot);

  if (pChar)  
  {
    IAttachmentManager* pMan = pChar->GetIAttachmentManager();
    for (int i=0; i<pMan->GetAttachmentCount(); ++i)
    {
      IAttachment* pAtt = pMan->GetInterfaceByIndex(i);
      if (strstr(pAtt->GetName(), "vulnerable"))
        pAtt->ClearBinding();
    }
  }
  return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetCloseColliderParts(IFunctionHandler *pH, int characterSlot, Vec3 hitPos, float radius)
{
  // find nearest physic. parts to explosion center
  // for now we just return the closest part (using the AABB)  
  
  CActor *pActor = GetActor(pH);  
	if (!pActor)
		return pH->EndFunction();

  IEntity* pEntity = pActor->GetEntity();

  ICharacterInstance* pChar = pEntity->GetCharacter(characterSlot);

  if (pChar && pChar->GetISkeletonPose()->GetCharacterPhysics())  
  { 
    IPhysicalEntity* pPhysics = pChar->GetISkeletonPose()->GetCharacterPhysics();
    
    pe_status_nparts nparts;
    int numParts = pPhysics->GetStatus(&nparts);    

    float minLenSq = radius*radius + 0.1f;
    int minLenPart = -1;
    
    pe_status_pos status;

    for (int i=0; i<numParts; ++i)
    {
      status.ipart = i;
      if (pPhysics->GetStatus(&status))
      { 
        AABB box(status.pos+status.BBox[0], status.pos+status.BBox[1]);
             
        // if hitpos inside AABB, return
        if (box.IsContainPoint(hitPos))
        {
          minLenPart = i;          
          break;
        }

        // else find closest distance 
        float lenSq = Distance::Point_AABBSq(hitPos, box);
        if (lenSq < minLenSq)
        {
          minLenSq = lenSq;
          minLenPart = i;          
        }
      }      
    }

    // get material from selected part
    static ISurfaceTypeManager* pSurfaceMan = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();

    if (minLenPart != -1)
    {
	     pe_params_part params;
      params.ipart = minLenPart;
      if (pPhysics->GetParams(&params))
      { 
        phys_geometry* pGeom = params.pPhysGeomProxy ? params.pPhysGeomProxy : params.pPhysGeom;
        if (pGeom->surface_idx > 0 &&  pGeom->surface_idx < params.nMats)
				{
					if (ISurfaceType *pSurfaceType=pSurfaceMan->GetSurfaceType(pGeom->pMatMapping[pGeom->surface_idx]))
						return pH->EndFunction(params.partid, pSurfaceType->GetName(), pSurfaceType->GetType());
				}
      }

      return pH->EndFunction(params.partid);
    }    
  }
  return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::CreateIKLimb( IFunctionHandler *pH, int slot, const char *limbName, const char *rootBone, const char *midBone, const char *endBone, int flags)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
		pActor->CreateIKLimb(slot,limbName,rootBone,midBone,endBone,flags);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::CreateFXBoneAttachment(IFunctionHandler* pH, int iCharacterSlot, const char* szBoneName, const char *szAttachmentName)
{
	CActor* pActor = GetActor(pH);
	if (pActor)
	{
		ICharacterInstance* pCharacter = pActor->GetEntity()->GetCharacter(iCharacterSlot);
		if (pCharacter)
		{
			IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
			IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(szAttachmentName);
			if (!pIAttachment)
				pIAttachment = pIAttachmentManager->CreateAttachment(szAttachmentName, CA_BONE, szBoneName, false, false, true);
		}
	}

	return pH->EndFunction();
};

//------------------------------------------------------------------------
int CScriptBind_Actor::ResetScores(IFunctionHandler *pH)
{
	CActor *pActor = (CActor*)(m_pGameFW->GetClientActor());
	if(!pActor)
		return pH->EndFunction();

	SAFE_HUD_FUNC(ResetScoreBoard());

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::RenderScore(IFunctionHandler *pH, ScriptHandle player, int kills, int deaths, int ping)
{
	CActor *pActor = (CActor*)(m_pGameFW->GetClientActor());
	if(!pActor)
		return pH->EndFunction();

	int teamKills = 0;
	int pp = 0;

	if (pH->GetParamCount()>4 && pH->GetParamType(5)==svtNumber)
		pH->GetParam(5, teamKills);
	
	if (pH->GetParamCount()>5 && pH->GetParamType(6)==svtNumber)
		pH->GetParam(6, pp);

	SAFE_HUD_FUNC(AddToScoreBoard((EntityId)player.n, kills, deaths, ping, teamKills, pp));

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::CheckInventoryRestrictions(IFunctionHandler *pH, const char *itemClassName)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor->CheckInventoryRestrictions(itemClassName))
		return pH->EndFunction(1);

	return pH->EndFunction();
}


//------------------------------------------------------------------------
int CScriptBind_Actor::HolsterItem(IFunctionHandler *pH, bool holster)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->HolsterItem(holster);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::DropItem(IFunctionHandler *pH, ScriptHandle itemId)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	float impulse=1.0f;
	bool bydeath=false;

	if (pH->GetParamCount()>1 && pH->GetParamType(2)==svtNumber)
		pH->GetParam(2, impulse);

	if (pH->GetParamCount()>2 && pH->GetParamType(3)==svtNumber||pH->GetParamType(2)==svtBool)
		pH->GetParam(3, bydeath);

	pActor->DropItem((EntityId)itemId.n, impulse, true, bydeath);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::PickUpItem(IFunctionHandler *pH, ScriptHandle itemId)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->PickUpItem((EntityId)itemId.n, true);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SelectLastItem(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->SelectLastItem(true);

	return pH->EndFunction();
}


//------------------------------------------------------------------------
int CScriptBind_Actor::SelectItemByName(IFunctionHandler *pH, const char *name)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->SelectItemByName(name, true);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SelectItem(IFunctionHandler *pH, ScriptHandle itemId)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->SelectItem(itemId.n, true);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SelectNextItem(IFunctionHandler *pH, int direction, bool keepHistory, const char *category)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->SelectNextItem(direction, keepHistory, category);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::IsGhostPit(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
  if (!pActor)
    return pH->EndFunction();

  bool hidden = false;
	 
  if (IVehicle* pVehicle = pActor->GetLinkedVehicle())
  {
    IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId());
    if (pSeat)
    { 
      if (IVehicleView* pView = pSeat->GetView(pSeat->GetCurrentView()))
        hidden = pView->IsPassengerHidden();
    }
  }

  return pH->EndFunction(hidden);   
}
//------------------------------------------------------------------------
int CScriptBind_Actor::IsPerkActive(IFunctionHandler *pH, int perkIndex)
{
	CActor *pActor = GetActor(pH);

	CPlayer* pPlayer = static_cast<CPlayer*>(pActor);

	CRY_ASSERT_MESSAGE(perkIndex >= 0, string().Format("A lua script passed IsPerkActive a bad perk number (%d)!", perkIndex));

	if (pPlayer && perkIndex >= 0)
	{
		return pH->EndFunction(pPlayer->IsPerkActive((EPerks)perkIndex));
	}

	return pH->EndFunction(0);
}

//------------------------------------------------------------------------
int CScriptBind_Actor::ActivateNanoSuit(IFunctionHandler *pH, int on)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if(pActor->GetActorClass() != CPlayer::GetActorClassType())
		return pH->EndFunction();

	//((CPlayer*)pActor)->ActivateNanosuit((on)?true:false);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetNanoSuitMode(IFunctionHandler *pH, int mode)
{
	/*CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();
	if(mode<0 || mode>=NANOMODE_LAST)
		return pH->EndFunction();
	if(pActor->GetActorClass() != CPlayer::GetActorClassType())
		return pH->EndFunction();

	if(CNanoSuit *pSuit = ((CPlayer*)pActor)->GetNanoSuit())
		pSuit->SetMode((ENanoMode)mode);
	else
		GameWarning("Lua tried to set NanoMode on not activated/existing Nanosuit of Player %s!", pActor->GetEntity()->GetName());*/

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetNanoSuitMode(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor || pActor->GetActorClass() != CPlayer::GetActorClassType())    
		return pH->EndFunction(0);

	if(pActor->HasNanoSuit())
		return pH->EndFunction((int)(pActor->GetActorSuitGameParameters().GetMode()));

	return pH->EndFunction(0);
}

//------------------------------------------------------------------------
int CScriptBind_Actor::GetNanoSuitEnergy(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
  if (!pActor || pActor->GetActorClass() != CPlayer::GetActorClassType())    
		return pH->EndFunction(0);
  	
  if(pActor->HasNanoSuit())
		return pH->EndFunction((int)(pActor->GetActorSuitGameParameters().GetEnergy().Get()));
	
  return pH->EndFunction(0);
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetNanoSuitEnergy(IFunctionHandler *pH, int energy)
{
	CActor *pActor = GetActor(pH);
  if (!pActor || pActor->GetActorClass() != CPlayer::GetActorClassType())    
		return pH ->EndFunction();
	
  //if(CNanoSuit *pSuit = ((CPlayer*)pActor)->GetNanoSuit())
		//pSuit->SetSuitEnergy((float)energy);
	
  return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SendPlayerSuitEvent(IFunctionHandler *pH, unsigned int eventIndex)
{
	CActor *pActor = GetActor(pH);
	CPlayer* pPlayer = NULL;
	if (pActor->IsPlayer())
		pPlayer = (CPlayer*) pActor;
	if (pPlayer)
	{
		SNanoSuitEvent event;
		event.event = (ENanoSuitEvent) eventIndex;
		if (!(eventIndex >= eNanoSuitEvent_LAST))
			pPlayer->SendActorSuitEvent(event);
	}
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::NanoSuitHit(IFunctionHandler *pH, int damage, const char *hitType)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if(pActor->GetActorClass() != CPlayer::GetActorClassType())
		return pH->EndFunction();

	if (pH->GetParamCount()>2)
	{
		bool explosion;
		Vec3 epos;

		pH->GetParam(3, explosion);
		pH->GetParam(4, epos);

		if (explosion && pActor->HasNanoSuit())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_EXPLOSION;
			event.vParam = epos;
			pActor->SendActorSuitEvent(event);

			return pH->EndFunction();
		}
	}

	if(pActor->HasNanoSuit())
	{
		SNanoSuitEvent event;
		event.event = eNanoSuitEvent_HIT;
		event.nParam = damage;
		event.sParam = hitType;
		pActor->SendActorSuitEvent(event);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetCloakMode(IFunctionHandler* pH, int cloakMode)
{
	CActor* pActor = GetActor( pH );
	if( !pActor )
	{
		return pH->EndFunction();
	}

	IEntity* pEntity = pActor->GetEntity();
	if (pEntity != NULL)
	{
		CloakEntity(pEntity, (ECloakMode)cloakMode);
		CloakCurrentItemForActor(pActor, (ECloakMode)cloakMode);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::SetSearchBeam(IFunctionHandler *pH, Vec3 dir)
{
  CActor *pActor = GetActor(pH);
  if (!pActor || pActor->GetActorClass() != CAlien::GetActorClassType())
    return pH->EndFunction();
  
  ((CAlien*)pActor)->SetSearchBeamGoal(dir);

  return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::IsFlying(IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	if (pActor)
	{
		pe_status_living livStat;
		IPhysicalEntity *pPhysEnt = pActor->GetEntity()->GetPhysics();

		if (!pPhysEnt)
			return pH->EndFunction();

		if(pPhysEnt->GetStatus(&livStat))
			return pH->EndFunction(livStat.bFlying!=0);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::PlayAction(IFunctionHandler *pH, const char* action)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->PlayAction(action, "");

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::RequestMechanic(IFunctionHandler *pH, const char* mechanic)
{
	CActor *pActor = GetActor(pH);
	if (!pActor)
		return pH->EndFunction();

	pActor->RequestMechanic(mechanic);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
void CScriptBind_Actor::CloakRenderProxy(IEntityRenderProxy* pRenderProxy, ECloakMode cloakMode)
{
	uint8 mask = pRenderProxy->GetMaterialLayersMask();
	uint32 blend = pRenderProxy->GetMaterialLayersBlend();
	mask = mask&~MTL_LAYER_DYNAMICFROZEN;
	pRenderProxy->SetMaterialLayersMask( (cloakMode == eCloakMode_Enabled) ? (mask|MTL_LAYER_CLOAK) : (mask&~MTL_LAYER_CLOAK) );
	pRenderProxy->SetMaterialLayersBlend( (blend & 0xffffff00) | ((mask & MTL_LAYER_DYNAMICFROZEN) ? 0xff : 0x00) );
}

//------------------------------------------------------------------------
void CScriptBind_Actor::CloakEntity(IEntity* pEntity, ECloakMode cloakMode)
{
	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*) pEntity->GetProxy( ENTITY_PROXY_RENDER );
	if (pRenderProxy == NULL)
	{
		return;
	}

	CloakRenderProxy(pRenderProxy, cloakMode);
}

//------------------------------------------------------------------------
void CScriptBind_Actor::CloakCurrentItemForActor(CActor* pActor, ECloakMode cloakMode)
{
	EntityId currentItemEntityId = pActor->GetInventory()->GetCurrentItem();

	IEntity* pCurrentItemEntity = gEnv->pEntitySystem->GetEntity( currentItemEntityId );
	if (pCurrentItemEntity == NULL)
	{
		return;
	}

	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)pCurrentItemEntity->GetProxy( ENTITY_PROXY_RENDER );
	if (pRenderProxy == NULL)
	{
		return;
	}

	CloakRenderProxy(pRenderProxy, cloakMode);
}

//------------------------------------------------------------------------
int CScriptBind_Actor::EnableSwitchingItems( IFunctionHandler *pH, bool enable )
{
	CActor *pActor = GetActor(pH);
	if (pActor)
	{
		pActor->EnableSwitchingItems(enable);
	}
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::EnableIronSights( IFunctionHandler *pH, bool enable )
{
	CActor *pActor = GetActor(pH);
	if (pActor)
	{
		pActor->EnableIronSights(enable);
	}
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::EnablePickingUpItems( IFunctionHandler *pH, bool enable )
{
	CActor *pActor = GetActor(pH);
	if (pActor)
	{
		pActor->EnablePickingUpItems(enable);
	}
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::ForceItemSuffix( IFunctionHandler *pH, const char* suffix)
{
	CActor *pActor = GetActor(pH);
	if (pActor)
	{
		IItem *iitem = pActor->GetCurrentItem();
		if (iitem)
		{
			CItem *item = (CItem*)iitem;
			item->SetActionSuffix(suffix, suffix);
		}
	}
	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Actor::ResetItemSuffix( IFunctionHandler *pH)
{
	CActor *pActor = GetActor(pH);
	if (pActor)
	{
		IItem *iitem = pActor->GetCurrentItem();
		if (iitem)
		{
			CItem *item = (CItem*)iitem;
			item->ResetActionSuffix();
		}
	}
	return pH->EndFunction();
}
