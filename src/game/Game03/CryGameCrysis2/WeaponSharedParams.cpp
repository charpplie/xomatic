/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------

Description: Stores all shared weapon parameters (shared by class)...
Allows for some memory savings...

-------------------------------------------------------------------------
History:
- 30:1:2008   10:54 : Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "Game.h"
#include "WeaponSharedParams.h"
#include "WeaponSystem.h"

#define ReadOptionalParams(paramString, param) {										\
	const IItemParamsNode* paramsNode	= pRoot->GetChild(paramString);	\
	if(paramsNode) {																									\
		if(!p##param) { p##param = new S##param; }											\
		Read##param(paramsNode);																				\
	} else { SAFE_DELETE(p##param); } }			


CWeaponSharedParams::~CWeaponSharedParams()
{
	ResetInternal();
}

//===================================================
void CWeaponSharedParams::ResetInternal()
{
	firemodeParams.clear();
	zoommodeParams.clear();

	SAFE_DELETE(pPickAndThrowParams);
	SAFE_DELETE(pTurretParams);
}

//============================================================
void CWeaponSharedParams::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->AddObject(aiWeaponDescriptor);
	pSizer->AddObject(zoomedMovementModifiers);

	//Reload Magazine Params
	pSizer->AddObject(reloadMagazineParams);

	//Ammo Params
	pSizer->AddObject(ammoParams);

	//Zoom shared data
	pSizer->AddObject(zoommodeParams);	


	//Fire shared data
	pSizer->AddObject(firemodeParams);	
	
	pSizer->AddObject(pickupSound);

	pSizer->AddObject(pPickAndThrowParams);
	pSizer->AddObject(pTurretParams);

}

void CWeaponSharedParams::ReadWeaponParams(const IItemParamsNode* pRoot, CItemSharedParams* pItemParams)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	const IItemParamsNode* params							= pRoot->GetChild("params");
	const IItemParamsNode* firemodes					= pRoot->GetChild("firemodes");
	const IItemParamsNode* zoommodes					= pRoot->GetChild("zoommodes");
	const IItemParamsNode* ammos							= pRoot->GetChild("ammos");
	const IItemParamsNode* aiData							= pRoot->GetChild("ai_descriptor");
	const IItemParamsNode* pMovementModifiers	= pRoot->GetChild("MovementModifiers");
	const IItemParamsNode* pMagazineParams		= pRoot->GetChild("ReloadMagazineParams");
	const IItemParamsNode* pickAndThrow				= pRoot->GetChild("PickAndThrowParams");
	const IItemParamsNode* turret							= pRoot->GetChild("turret");

	string melee_attack_firemode;
	CItemParamReader reader(params);

	reader.Read("melee_attack_firemode", melee_attack_firemode);
	reader.Read("ammo_pickup_sound", pickupSound);
	
	ReadFireModeParams(firemodes, pItemParams, melee_attack_firemode);
	ReadZoomModeParams(zoommodes, pItemParams);
	ReadAmmoParams(ammos);
	ReadAIParams(aiData);
	ReadMovementModifierParams(pMovementModifiers);
	ReadReloadMagazineParams(pMagazineParams);

	ReadOptionalParams("PickAndThrowParams", PickAndThrowParams);

	if(turret)
	{
		if(!pTurretParams)
		{
			pTurretParams = new STurretParams();
		}

		ReadTurretParams(turret, params);
	}
	else
	{
		SAFE_DELETE(pTurretParams);
	}

	SetValid(true);
}

//------------------------------------------------------------------------
void CWeaponSharedParams::ReadZoomModeParams(const IItemParamsNode* pNode, CItemSharedParams* pItemParams)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	zoommodeParams.clear();

	if (!pNode)
		return;

	int numZoommodes = pNode->GetChildCount();

	// find the default params
	const IItemParamsNode* defaultZM = NULL;

	for (int i = 0; i < numZoommodes; i++)
	{
		const IItemParamsNode* zmParams = pNode->GetChild(i);
		const char *typ = zmParams->GetAttribute("type");

		if (typ && !strcmpi(typ, "default"))
		{
			defaultZM = zmParams;
			break;
		}
	}

	for (int i = 0; i < numZoommodes; i++)
	{
		const IItemParamsNode* zmParams = pNode->GetChild(i);

		int enabled = 1;
		const char *name = zmParams->GetAttribute("name");
		const char *typ = zmParams->GetAttribute("type");
		zmParams->GetAttribute("enabled", enabled);

		if (!typ || !typ[0])
		{
			GameWarning("Missing type for zoommode in weapon! Skipping...");
			continue;
		}

		if (!strcmpi(typ, "default"))
			continue;

		if (!name || !name[0])
		{
			GameWarning("Missing name for zoommode in weapon! Skipping...");
			continue;
		}

		SParentZoomModeParams newZoommode;

		newZoommode.initialiseParams.modeName = name;
		newZoommode.initialiseParams.modeType = typ;
		newZoommode.initialiseParams.enabled = (enabled != 0);
	
		ReadZoomMode(defaultZM, &newZoommode.baseZoomMode, true);
		ReadZoomMode(zmParams, &newZoommode.baseZoomMode, false);

		TAccessoryParamsMap::iterator end = pItemParams->accessoryparams.end();

		for (TAccessoryParamsMap::iterator ait = pItemParams->accessoryparams.begin(); ait != end; ++ait)
		{
			const SAccessoryParams* pAccessoryParams = &ait->second;

			const IItemParamsNode* defaultPatch = FindAccessoryPatchForMode("zoommodes", "default", pAccessoryParams);
			const IItemParamsNode* zmPatch = FindAccessoryPatchForMode("zoommodes", name, pAccessoryParams);

			if(defaultPatch || zmPatch)
			{
				SZoomModeParams accessoryZoommode;

				ReadZoomMode(defaultZM, &accessoryZoommode, true);

				if(defaultPatch)
				{
					ReadZoomMode(defaultPatch, &accessoryZoommode, false);
				}

				ReadZoomMode(zmParams, &accessoryZoommode, false);

				if(zmPatch)
				{
					ReadZoomMode(zmPatch, &accessoryZoommode, false);
				}

				newZoommode.accessoryChangedParams.insert(SParentZoomModeParams::TAccessoryZoomModeMap::value_type(ait->first, accessoryZoommode));
			}
		}

		zoommodeParams.push_back(newZoommode);
	}
}

void CWeaponSharedParams::ReadZoomMode(const IItemParamsNode* pZMNode, SZoomModeParams* pZoomMode, bool defaultInit)
{
	const IItemParamsNode *zoom = NULL;
	const IItemParamsNode *actions = NULL;
	const IItemParamsNode *spreadMod = NULL;
	const IItemParamsNode *recoilMod = NULL; 
	const IItemParamsNode *zoomSway = NULL;
	const IItemParamsNode *scope = NULL;

	if(pZMNode)
	{
		zoom = pZMNode->GetChild("zoom");
		actions = pZMNode->GetChild("actions");
		spreadMod = pZMNode->GetChild("spreadMod");
		recoilMod = pZMNode->GetChild("recoilMod"); 
		zoomSway = pZMNode->GetChild("zoomSway");
		scope = pZMNode->GetChild("scope");
	}

	pZoomMode->zoomParams.Reset(zoom, defaultInit);
	pZoomMode->actions.Reset(actions, defaultInit);
	pZoomMode->spreadModParams.Reset(spreadMod, defaultInit);
	pZoomMode->recoilModParams.Reset(recoilMod, defaultInit);
	pZoomMode->zoomSway.Reset(zoomSway, defaultInit);
	pZoomMode->scopeParams.Reset(scope, defaultInit);
}

//------------------------------------------------------------------------
void CWeaponSharedParams::ReadFireModeParams(const IItemParamsNode* pNode, CItemSharedParams* pItemParams, const char* meleeFireMode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	firemodeParams.clear();
	
	if (!pNode)
		return;

	bool meleeFound = false;

	int numFiremodes = pNode->GetChildCount();

	// find the default params
	const IItemParamsNode* defaultFM = NULL;

	for (int i = 0; i < numFiremodes; i++)
	{
		const IItemParamsNode* fmParams = pNode->GetChild(i);
		const char* typ = fmParams->GetAttribute("type");

		if (typ && !strcmpi(typ, "default"))
		{
			defaultFM = fmParams;
			break;
		}
	}

	int fireModeId = 0;
	for (int i = 0; i < numFiremodes; i++)
	{
		const IItemParamsNode *fmParams = pNode->GetChild(i);

		bool isMelee = false;
		int enabled = 1;
		int secondary = 0;
		
		const char *name = fmParams->GetAttribute("name");
		const char *typ = fmParams->GetAttribute("type");
		
		fmParams->GetAttribute("enabled", enabled);
		fmParams->GetAttribute("secondary", secondary);

		if (!typ || !typ[0])
		{
			GameWarning("Missing type for firemode in weapon! Skipping...");
			continue;
		}

		if (!strcmpi(typ, "default"))
			continue;

		if (!name || !name[0])
		{
			GameWarning("Missing name for firemode in weapon! Skipping...");
			continue;
		}
	
		SParentFireModeParams newFiremode;

		if(!meleeFound)
		{
			if(!strcmp(name, meleeFireMode))
			{
				isMelee = true;
				meleeFound = true;
				enabled = 0;
			}
		}

		newFiremode.initialiseParams.isMelee = isMelee;
		newFiremode.initialiseParams.modeName = name;
		newFiremode.initialiseParams.modeType = typ;
		newFiremode.initialiseParams.enabled = (enabled != 0);
		newFiremode.initialiseParams.secondary = (secondary != 0);

		ReadFireMode(defaultFM, &newFiremode.baseFireMode, true);
		ReadFireMode(fmParams, &newFiremode.baseFireMode, false);

		TAccessoryParamsMap::iterator end = pItemParams->accessoryparams.end();

		for (TAccessoryParamsMap::iterator ait = pItemParams->accessoryparams.begin(); ait != end; ++ait)
		{
			const SAccessoryParams* pAccessoryParams = &ait->second;

			const IItemParamsNode* defaultPatch = FindAccessoryPatchForMode("firemodes", "default", pAccessoryParams);
			const IItemParamsNode* fmPatch = FindAccessoryPatchForMode("firemodes", name, pAccessoryParams);

			if(defaultPatch || fmPatch)
			{
				SFireModeParams accessoryFiremode;

				ReadFireMode(defaultFM, &accessoryFiremode, true);
				
				if(defaultPatch)
				{
					ReadFireMode(defaultPatch, &accessoryFiremode, false);
				}
		
				ReadFireMode(fmParams, &accessoryFiremode, false);

				if(fmPatch)
				{
					ReadFireMode(fmPatch, &accessoryFiremode, false);
				}

				newFiremode.accessoryChangedParams.insert(SParentFireModeParams::TAccessoryFireModeMap::value_type(ait->first, accessoryFiremode));
			}
		}

		firemodeParams.push_back(newFiremode);
	}
}

void CWeaponSharedParams::ReadFireMode(const IItemParamsNode* pFMNode, SFireModeParams* pFireMode, bool defaultInit)
{
	const IItemParamsNode* fire = NULL;
	const IItemParamsNode* tracer = NULL;
	const IItemParamsNode* armourPiercingPerkTracer = NULL;
	const IItemParamsNode* actions = NULL;
	const IItemParamsNode* muzzleflash = NULL;
	const IItemParamsNode* muzzlesmoke = NULL;
	const IItemParamsNode* reject = NULL;
	const IItemParamsNode* spinup = NULL;
	const IItemParamsNode* heating = NULL;
	const IItemParamsNode* recoil = NULL;
	const IItemParamsNode* proceduralRecoil = NULL;
	const IItemParamsNode* spread = NULL;
	const IItemParamsNode* bulletBelt = NULL;
	const IItemParamsNode* shotgun = NULL;
	const IItemParamsNode* burstParams = NULL; 
	const IItemParamsNode* throwParams = NULL;
	const IItemParamsNode* charge = NULL;
	const IItemParamsNode* effect = NULL;
	const IItemParamsNode* rapid = NULL;
	const IItemParamsNode* grenades = NULL;
	const IItemParamsNode* beam = NULL;
	const IItemParamsNode* hiteffect = NULL;
	const IItemParamsNode* melee = NULL;
	const IItemParamsNode* cone = NULL;
	const IItemParamsNode* plant = NULL;
	const IItemParamsNode *detectorBurst = NULL;
	
	if(pFMNode)
	{
		fire = pFMNode->GetChild("fire");
		tracer = pFMNode->GetChild("tracer");
		armourPiercingPerkTracer = pFMNode->GetChild("armourPiercingPerkTracer");
		actions = pFMNode->GetChild("actions");
		muzzleflash = pFMNode->GetChild("muzzleflash");
		muzzlesmoke = pFMNode->GetChild("muzzlesmoke");
		reject = pFMNode->GetChild("reject");
		spinup = pFMNode->GetChild("spinup");
		heating = pFMNode->GetChild("heating");
		recoil = pFMNode->GetChild("recoil");
		proceduralRecoil = pFMNode->GetChild("proceduralRecoil");
		spread = pFMNode->GetChild("spread");
		bulletBelt = pFMNode->GetChild("bulletBelt");
		shotgun = pFMNode->GetChild("shotgun");
		burstParams = pFMNode->GetChild("burst");
		throwParams = pFMNode->GetChild("throw");
		charge = pFMNode->GetChild("charge");
		effect = pFMNode->GetChild("effect");
		rapid = pFMNode->GetChild("rapid");
		grenades = pFMNode->GetChild("grenades");
		beam = pFMNode->GetChild("beam");
		hiteffect = pFMNode->GetChild("hiteffect");
		melee = pFMNode->GetChild("melee");
		cone = pFMNode->GetChild("cone");
		plant = pFMNode->GetChild("plant");
		detectorBurst = pFMNode->GetChild("burstDetector");
	}

	pFireMode->fireparams.Reset(fire, defaultInit);
	pFireMode->tracerparams.Reset(tracer, defaultInit);
	pFireMode->armourPiercingPerkTracerParams.Reset(armourPiercingPerkTracer ? armourPiercingPerkTracer : tracer, defaultInit);
	pFireMode->recoilparamsCopy.Reset(recoil, defaultInit);
	pFireMode->proceduralRecoilParams.Reset(proceduralRecoil, defaultInit);
	pFireMode->spreadparamsCopy.Reset(spread, defaultInit);
	pFireMode->actions.Reset(actions, defaultInit);
	pFireMode->muzzleflash.Reset(muzzleflash, defaultInit);
	pFireMode->muzzlesmoke.Reset(muzzlesmoke, defaultInit);
	pFireMode->reject.Reset(reject, defaultInit);
	pFireMode->spinup.Reset(spinup, defaultInit);
	pFireMode->heatingparams.Reset(heating, defaultInit);
	pFireMode->bulletBelletParams.Reset(bulletBelt, defaultInit); //should be optional
	pFireMode->shotgunparams.Reset(shotgun, defaultInit); //Only want this if type shotgun
	pFireMode->burstparams.Reset(burstParams, defaultInit); //Only want this if type burst
	pFireMode->throwparams.Reset(throwParams, defaultInit); //Only want this if type throw
	pFireMode->throwExParams.Reset(throwParams, defaultInit); //Only want this if type throwEx
	pFireMode->chargeparams.Reset(charge, defaultInit); //only want this if type charge
	pFireMode->chargeeffect.Reset(effect, defaultInit); //only want this if type charge
	pFireMode->rapidparams.Reset(rapid, defaultInit); //only want this if type rapid
	pFireMode->lTagGrenades.Reset(grenades, defaultInit); //only want this if type ltag
	pFireMode->drillerParams.Reset(fire, defaultInit); //only want this if type tunnellinggrenade
	pFireMode->beamparams.Reset(beam, defaultInit); //only want this if type beam 
	pFireMode->effectparams.Reset(effect, defaultInit); //only want this if type beam 
	pFireMode->hitbeameffectparams.Reset(hiteffect, defaultInit); //only want this if type beam 
	pFireMode->meleeparams.Reset(melee, defaultInit); // only want this if type melee
	pFireMode->coneparams.Reset(cone, defaultInit); //only want this if type impulsebeam
	pFireMode->coneeffectparams.Reset(effect, defaultInit); //only want this if type impulsebeam
	pFireMode->plantparams.Reset(plant, defaultInit); //only want this if type plant
	pFireMode->detectorburstparams.Reset(detectorBurst, defaultInit); //only want this if type pingerdetectorburst
	pFireMode->detectorbursteffectparams.Reset(effect, defaultInit);  //only want this if type pingerdetectorburst

		
}


//------------------------------------------------------------------------
void CWeaponSharedParams::ReadAmmoParams(const IItemParamsNode* pNode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	ammoParams.ammo.clear();
	ammoParams.accessoryAmmo.clear();
	ammoParams.bonusAmmo.clear();
	ammoParams.minDroppedAmmo.clear();
	
	if (!pNode)
		return;

	const int numAmmo = pNode->GetChildCount();

	ammoParams.ammo.reserve(numAmmo);
	ammoParams.accessoryAmmo.reserve(numAmmo);
	ammoParams.bonusAmmo.reserve(numAmmo);
	ammoParams.minDroppedAmmo.reserve(numAmmo);

	for (int i = 0; i < numAmmo; i++)
	{
		const IItemParamsNode* ammoNode = pNode->GetChild(i);
		
		if (!strcmpi(ammoNode->GetName(), "ammo"))
		{
			int extra = 0;
			int amount = 0;
			int accessoryAmmo = 0;
			int minAmmo = 0;

			const char* name = ammoNode->GetAttribute("name");
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			CRY_ASSERT_TRACE(pClass, ("Ammo '%s' class not found in class registry, for weapon", name));

			ammoNode->GetAttribute("amount", amount);
			ammoNode->GetAttribute("extra", extra);
			ammoNode->GetAttribute("accessoryAmmo", accessoryAmmo);
			ammoNode->GetAttribute("minAmmo", minAmmo);

			SWeaponAmmo ammoInfo(pClass, amount);

			if (accessoryAmmo)
			{
				ammoInfo.count = accessoryAmmo;
				ammoParams.ammo.push_back(ammoInfo);
				ammoParams.accessoryAmmo.push_back(ammoInfo);
			}
			else if (amount)
			{
				ammoParams.ammo.push_back(ammoInfo);
			}

			if (extra)
			{
				ammoInfo.count = extra;
				ammoParams.bonusAmmo.push_back(ammoInfo);
			}

			if (minAmmo)
			{
				ammoInfo.count = minAmmo;
				ammoParams.minDroppedAmmo.push_back(ammoInfo);
			}
		}
	}
}

//----------------------------------------------------------------------
void CWeaponSharedParams::ReadReloadMagazineParams(const IItemParamsNode* pNode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (!pNode)
		return;

	reloadMagazineParams.magazineEvent = pNode->GetAttribute("magazineEvent");
	reloadMagazineParams.magazineAttachment = pNode->GetAttribute("magazineAttachment");
	reloadMagazineParams.supportsMagazineSwap = (!reloadMagazineParams.magazineEvent.empty());
}

//------------------------------------------------------------------------
void CWeaponSharedParams::ReadMovementModifierParams(const IItemParamsNode* pNode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	int numZoommodes = zoommodeParams.size();

	//Set default values for all zoom-modes
	zoomedMovementModifiers.reserve(numZoommodes);

	for (int i = 0; i < numZoommodes; i++)
	{
		zoomedMovementModifiers.push_back(defaultMovementModifiers);
	}

	//Read actual values if present
	if (pNode)
	{
		float firingMovementScale = 1.f;
		float firingRotationScale = 1.f;

		//Get first the default/base ones
		pNode->GetAttribute("firingSpeedScale", firingMovementScale);
		pNode->GetAttribute("firingRotationScale", firingRotationScale);
	
		pNode->GetAttribute("speedScale", defaultMovementModifiers.movementSpeedScale);
		pNode->GetAttribute("rotationScale", defaultMovementModifiers.rotationSpeedScale);
		pNode->GetAttribute("coverLeanRotationScale", defaultMovementModifiers.coverLeanRotationSpeedScale);

		defaultMovementModifiers.firingMovementSpeedScale = defaultMovementModifiers.movementSpeedScale * firingMovementScale;
		defaultMovementModifiers.firingRotationSpeedScale = defaultMovementModifiers.rotationSpeedScale * firingRotationScale;

		//Patch defined zoom-modes with corresponding values
		for(int i = 0; i < numZoommodes; i++)
		{
			const IItemParamsNode* pZoomModeModifier = pNode->GetChild(zoommodeParams[i].initialiseParams.modeName.c_str());

			if (pZoomModeModifier)
			{
				SPlayerMovementModifiers& zoomedModifiers = zoomedMovementModifiers[i];

				pZoomModeModifier->GetAttribute("speedScale", zoomedModifiers.movementSpeedScale);
				pZoomModeModifier->GetAttribute("rotationScale", zoomedModifiers.rotationSpeedScale);
				pZoomModeModifier->GetAttribute("coverLeanRotationScale", zoomedModifiers.coverLeanRotationSpeedScale);

				zoomedModifiers.firingMovementSpeedScale = zoomedModifiers.movementSpeedScale * firingMovementScale;
				zoomedModifiers.firingRotationSpeedScale = zoomedModifiers.rotationSpeedScale * firingRotationScale;
			}
		}
	}
}

//------------------------------------------------------------------------
const IItemParamsNode* CWeaponSharedParams::FindAccessoryPatchForMode(const char* modeType, const char *modeName, const SAccessoryParams* pAccessory)
{
	const IItemParamsNode *modes = pAccessory->params ? pAccessory->params->GetChild(modeType) : NULL;
	if (modes)
	{
		int n = modes->GetChildCount();
		for (int i=0; i<n; i++)
		{
			const IItemParamsNode *modePatch = modes->GetChild(i);
			const char *name = modePatch->GetAttribute("name");
			if (name && !stricmp(name, modeName))
			{
				return modePatch;
			}
			const char *typ = modePatch->GetAttribute("type");
			if (typ && !stricmp(typ, modeName))
			{
				return modePatch;
			}
		}		
	}
	
	return NULL;
}

void CWeaponSharedParams::ReadPickAndThrowParams(const IItemParamsNode* pNode)
{
	CItemParamReader reader(pNode);
	
	pPickAndThrowParams->grabTypesParams[ePTGrabType_2H].Read( pNode, "GrabParams_2h");
	pPickAndThrowParams->grabTypesParams[ePTGrabType_2H_Pole].Read( pNode, "GrabParams_2h_Pole");
	pPickAndThrowParams->grabTypesParams[ePTGrabType_1H].Read( pNode, "GrabParams_1h");
	pPickAndThrowParams->grabTypesParams[ePTGrabType_NPCFront].Read( pNode, "GrabParams_NPCFront");
	pPickAndThrowParams->grabTypesParams[ePTGrabType_NPCBack].Read( pNode, "GrabParams_NPCBack");
}


void SPickAndThrowParams::SGrabTypeParams::Read( const IItemParamsNode* pMainNode, const char* name )
{
	const IItemParamsNode* pNode = pMainNode ? pMainNode->GetChild( name ) : NULL;
	if (!pNode)
		return;
		
	if (const IItemParamsNode *pAimAnimsNode = pNode->GetChild("aimAnims"))
		aimAnims.Read( pAimAnimsNode );
		
	CItemParamReader reader( pNode );

	reader.Read( "helper", helper );
	reader.Read( "attachment", attachment );
	reader.Read( "grab_action", grab_action );
	reader.Read( "throw_action", throw_action );
	reader.Read( "drop_action", drop_action );
	reader.Read( "melee_action", melee_action );
	reader.Read( "melee_hit_action", melee_hit_action );
	reader.Read( "melee_delay", melee_delay );
	reader.Read( "melee_damage_scale", melee_damage_scale );
	reader.Read( "melee_impulse_scale", melee_impulse_scale );
	reader.Read( "melee_mfxLibrary", melee_mfxLibrary );

	// default values (not for all fields)
	timePicking = 0.2f;
	timeToStickAtPicking = 0.1f;
	timeThrowing = 0.5f;
	timeToFreeAtThrowing = 0.1f;
	throwSpeed = 12;
	timeDropping = 0.5f;
	timeToFreeAtDropping = 0.1f;
	dropSpeedForward = 2.f;

	reader.Read("timePicking", timePicking );
	reader.Read("timeToStickAtPicking", timeToStickAtPicking );

	reader.Read("timeThrowing", timeThrowing );
	reader.Read("timeToFreeAtThrowing", timeToFreeAtThrowing );
	reader.Read("throwSpeed", throwSpeed );
	reader.Read("dropSpeedForward", dropSpeedForward );

	reader.Read("timeDropping", timeDropping );
	reader.Read("timeToFreeAtDropping", timeToFreeAtDropping );
	
	reader.Read( "timeToKillGrabbedNPC", timeToKillGrabbedNPC );
}



//------------------------------------------------------------------------
void CWeaponSharedParams::ReadAIParams(const IItemParamsNode* pNode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// Still set gravity descriptor even if we don't have it specified in the XML
	// the first firemode (which isn't melee) will be the default firemode entered

	int numFiremodes = firemodeParams.size();

	for (int i = 0; i < numFiremodes; i++)
	{
		if(!firemodeParams[i].initialiseParams.isMelee && firemodeParams[i].baseFireMode.fireparams.ammo_type_class)
		{
			if (const SAmmoParams *pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(firemodeParams[i].baseFireMode.fireparams.ammo_type_class))
			{
				if (pAmmoParams->pParticleParams && !is_unused(pAmmoParams->pParticleParams->gravity))
				{
					aiWeaponDescriptor.projectileGravity = pAmmoParams->pParticleParams->gravity;
				}
				break;
			}
		}
	}

	if (pNode)
	{
		pNode->GetAttribute("speed", aiWeaponDescriptor.fSpeed);
		pNode->GetAttribute("damage_radius", aiWeaponDescriptor.fDamageRadius);
		pNode->GetAttribute("charge_time", aiWeaponDescriptor.fChargeTime);
		pNode->GetAttribute("burstBulletCountMin", aiWeaponDescriptor.burstBulletCountMin);
		pNode->GetAttribute("burstBulletCountMax", aiWeaponDescriptor.burstBulletCountMax);
		pNode->GetAttribute("burstPauseTimeMin", aiWeaponDescriptor.burstPauseTimeMin);
		pNode->GetAttribute("burstPauseTimeMax", aiWeaponDescriptor.burstPauseTimeMax);
		pNode->GetAttribute("singleFireTriggerTime", aiWeaponDescriptor.singleFireTriggerTime);
		pNode->GetAttribute("spreadRadius", aiWeaponDescriptor.spreadRadius);
		pNode->GetAttribute("coverFireTime", aiWeaponDescriptor.coverFireTime);
		pNode->GetAttribute("sweep_width", aiWeaponDescriptor.sweepWidth);
		pNode->GetAttribute("sweep_frequency", aiWeaponDescriptor.sweepFrequency);
		pNode->GetAttribute("draw_time", aiWeaponDescriptor.drawTime);
		pNode->GetAttribute("projectileGravity", aiWeaponDescriptor.projectileGravity);

		aiWeaponDescriptor.smartObjectClass = pNode->GetAttribute("smartobject_class");
		aiWeaponDescriptor.firecmdHandler = pNode->GetAttribute("handler");
		
		int	signalOnShoot(0);
		pNode->GetAttribute("signal_on_shoot", signalOnShoot);
		
		aiWeaponDescriptor.bSignalOnShoot = signalOnShoot != 0;

		const IItemParamsNode* offset = pNode->GetChild("weaponOffset");

		if(offset)
		{
			ReadAIOffsets(offset);
		}
	}
}

//------------------------------------------------------------------------
void CWeaponSharedParams::ReadAIOffsets(const IItemParamsNode *aiOffsetData)
{
	if(aiOffsetData->GetChild("useEyeOffset"))
	{
		aiWeaponOffsets.useEyeOffset = true;
	}
	else
	{
		int n = aiOffsetData->GetChildCount();
		for (int i=0; i<n; ++i)
		{
			const IItemParamsNode *offsetNode = aiOffsetData->GetChild(i);
			const char *stance = offsetNode->GetAttribute("stanceId");
			// retrieve enum value from scripts, using stance name
			int curStanceInt(STANCE_NULL);
			if(!gEnv->pScriptSystem->GetGlobalValue(stance,curStanceInt))
				continue;
			EStance	curStance((EStance)curStanceInt);
			if(curStance==STANCE_NULL)
				continue;
			Vec3	curOffset;
			if(!offsetNode->GetAttribute("weaponOffset", curOffset))
				continue;
			aiWeaponOffsets.stanceWeponOffset[curStance] = curOffset;
			if(!offsetNode->GetAttribute("weaponOffsetLeanLeft", curOffset))
				continue;
			aiWeaponOffsets.stanceWeponOffsetLeanLeft[curStance] = curOffset;
			if(!offsetNode->GetAttribute("weaponOffsetLeanRight", curOffset))
				continue;
			aiWeaponOffsets.stanceWeponOffsetLeanRight[curStance] = curOffset;
		}
	}
}

void CWeaponSharedParams::ReadTurretParams(const IItemParamsNode* pTurret, const IItemParamsNode* pParams)
{
	CItemParamReader reader(pParams);

	reader.Read("rocket_firemode", pTurretParams->rocketFireMode);
	reader.Read("radar_helper", pTurretParams->radarHelper);
	reader.Read("tripod_helper", pTurretParams->tripodHelper);
	reader.Read("barrel_helper", pTurretParams->barrelHelper);
	reader.Read("fire_helper", pTurretParams->fireHelper);
	reader.Read("rocket_helper", pTurretParams->rocketHelper);
	reader.Read("HoverHeight", pTurretParams->desiredHoverHeight);
	
	if (const IItemParamsNode* search = pTurret->GetChild("search"))
		pTurretParams->searchParams.Reset(search);

	if (const IItemParamsNode* fire = pTurret->GetChild("fire"))
		pTurretParams->fireParams.Reset(fire); 
}
