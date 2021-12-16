/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 22:8:2005   12:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include <IEntitySystem.h>
#include <IScriptSystem.h>
#include <IActionMapManager.h>
#include <IGameObject.h>
#include <IGameObjectSystem.h>
#include <IVehicleSystem.h>
#include "WeaponSystem.h"
#include "Weapon.h"
#include "ISerialize.h"
#include "ScriptBind_Weapon.h"
#include "Player.h"
#include "GameRules.h"
#include "ItemParamReader.h"
#include "Projectile.h"

#include "Laser.h"
#include "Flashlight.h"
#include "IronSight.h"
#include "Single.h"
#include "Environment/BattleDust.h"
#include "CryCharAnimationParams.h"
#include "ItemSharedParams.h"
#include "WeaponSharedParams.h"
#include "HUD/HUD.h"

#include "IPlayerInput.h"
#include <IWorldQuery.h>

#include "HUD/HUD.h"
#include "TacticalManager.h"

CWeapon::SWeaponCrosshairStats CWeapon::s_crosshairstats;

float CWeapon::s_dofValue = 0.0f;
float CWeapon::s_dofSpeed = 0.0f;
float CWeapon::s_focusValue = 0.0f;
uint8 CWeapon::s_requestedActions = 0;

//------------------------------------------------------------------------
CWeapon::CWeapon()
: m_fm(0),
	m_melee(0),
	m_zm(0),
	m_zmId(0),
	m_pFiringLocator(0),
	m_fire_alternation(false),
  m_destination(0,0,0),
	m_restartZoom(false),
	m_restartZoomStep(0),
	m_targetOn(false),
	m_weaponRaised(false),
	m_weaponLowered(false),
	m_switchingFireMode(false),
	m_switchLeverLayers(false),
	m_nextShotTime(0.0f),
	m_zoomedTime(0.0f),
	m_lastRecoilUpdate(0),
	m_forcingRaise(false),
	m_doingMagazineSwap(false),
	m_delayedFireActionTimeOut(0.0f),
	m_delayedZoomActionTimeOut(0.0f),
	m_isClientOwnerOverride(false),
	m_minDropAmmoAvailable(true),
	m_isDeselecting(false),
	m_isRegisteredAmmoWithInventory(false),
	m_listeners(1)
{
	RegisterActions();

	m_firemode = 0;
	m_reloadState = eNRS_NoReload;
	m_isFiring = false;
	m_isFiringStarted = false;
	m_fireCounter = 0;
	m_expended_ammo = 0;
	m_shootCounter = 0;
	m_meleeCounter = 0;
	m_doMelee = false;
	m_netInitialised = false;
	m_lastRecvInventoryAmmo = 0;
}

//------------------------------------------------------------------------1
CWeapon::~CWeapon()
{
	// deactivate everything
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); ++it)
		(*it)->Activate(false);
	// deactivate zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); ++it)
		(*it)->Activate(false);

	// clean up firemodes
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); ++it)
		(*it)->Release();
	// clean up zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); ++it)
		(*it)->Release();

	if (m_pFiringLocator)
		m_pFiringLocator->WeaponReleased();

	if(IInventory *pInventory = GetActorInventory(GetOwnerActor()))
	{
		if (pInventory->FindItem(GetEntityId()) >= 0)
		{
			UnregisterUsedAmmoWithInventory(pInventory);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::InitItemFromParams()
{
	BaseClass::InitItemFromParams();
	
	InitAmmo();
	InitFireModes();
	InitZoomModes();
	InitAIData();

	CacheRaisePose();
}

//------------------------------------------------------------------------
void CWeapon::InitFireModes()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_firemodes.resize(0);
	m_fmIds.clear();
	m_fm = 0;

	int n = m_weaponsharedparams->firemodeParams.size();
	
	int fireModeId = 0;
	for (int i = 0; i<n; i++)
	{
		const SParentFireModeParams* pNewParams = &m_weaponsharedparams->firemodeParams[i];

		CFireMode *pFireMode = g_pGame->GetWeaponSystem()->CreateFireMode(pNewParams->initialiseParams.modeType);
		
		if (!pFireMode)
		{
			GameWarning("Cannot create firemode '%s' in weapon '%s'! Skipping...", pNewParams->initialiseParams.modeType.c_str(), GetEntity()->GetName());
			continue;
		}

		pFireMode->SetName(pNewParams->initialiseParams.modeName);
		pFireMode->InitFireMode(this, pNewParams, fireModeId);
		pFireMode->Enable(pNewParams->initialiseParams.enabled);
		pFireMode->SetSecondary(pNewParams->initialiseParams.secondary);
		pFireMode->PostInit();

		m_firemodes.push_back(pFireMode);
		m_fmIds.insert(TFireModeIdMap::value_type(pNewParams->initialiseParams.modeName.c_str(), fireModeId));

		if(pNewParams->initialiseParams.isMelee)
		{
			m_melee = pFireMode;
		}

		fireModeId++;
	}

	SetCurrentFireMode(0);
}

//------------------------------------------------------------------------
void CWeapon::InitZoomModes()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_zoommodes.resize(0);
	m_zmIds.clear();
	m_zmId = 0;
	m_zm = 0;

	int n = m_weaponsharedparams->zoommodeParams.size();

	for (int i = 0; i < n; i++)
	{
		const SParentZoomModeParams* pNewParams = &m_weaponsharedparams->zoommodeParams[i];

		CIronSight *pZoomMode = static_cast<CIronSight*>(g_pGame->GetWeaponSystem()->CreateZoomMode(pNewParams->initialiseParams.modeType));
		if (!pZoomMode)
		{
			GameWarning("Cannot create zoommode '%s' in weapon '%s'! Skipping...", pNewParams->initialiseParams.modeType.c_str(), GetEntity()->GetName());
			continue;
		}

		pZoomMode->InitZoomMode(this, pNewParams, m_zoommodes.size());
		pZoomMode->Enable(pNewParams->initialiseParams.enabled);

		m_zoommodes.push_back(pZoomMode);
		m_zmIds.insert(TZoomModeIdMap::value_type(pNewParams->initialiseParams.modeName.c_str(), m_zoommodes.size()-1));
	}
}

//------------------------------------------------------------------------
void CWeapon::InitAmmo()
{
	m_ammo = m_weaponsharedparams->ammoParams.ammo;
	m_bonusammo = m_weaponsharedparams->ammoParams.bonusAmmo;
}

void CWeapon::InitAIData()
{
	if(m_weaponsharedparams->aiWeaponDescriptor.smartObjectClass != "")
	{
		const char* smartObjectClassProperties = NULL;
		// check if the smartobject class has been overridden in the level
		SmartScriptTable props;
		if (GetEntity()->GetScriptTable() && GetEntity()->GetScriptTable()->GetValue("Properties", props))
		{
			if(!props->GetValue("soclasses_SmartObjectClass",smartObjectClassProperties) || 
				(smartObjectClassProperties ==NULL || smartObjectClassProperties[0]==0))
			{
				props->SetValue("soclasses_SmartObjectClass",m_weaponsharedparams->aiWeaponDescriptor.smartObjectClass.c_str());
			}
		}
	}
}

//------------------------------------------------------------------------
bool CWeapon::Init( IGameObject * pGameObject )
{
	if (!BaseClass::Init(pGameObject))
		return false;

	g_pGame->GetWeaponScriptBind()->AttachTo(this);

	if(!IsMounted())
		GetEntity()->SetFlags(GetEntity()->GetFlags()|ENTITY_FLAG_ON_RADAR);

	g_pGame->GetTacticalManager()->AddEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Item);

	return true;
}

//------------------------------------------------------------------------
void CWeapon::Release()
{
	g_pGame->GetTacticalManager()->RemoveEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Item);
	RemoveOwnerAttachedAccessories();
	delete this;
}

//------------------------------------------------------------------------
void CWeapon::SNetWeaponData::NetSerialise(TSerialize ser)
{
	ser.Value("firemode", m_firemode, 'ui3'); // 0 - 7
	ser.Value("reload", m_reload, 'reld');
	ser.Value("raised", m_raised, 'bool');
	ser.Value("firing", m_isFiring, 'bool');
	ser.Value("zoomState", m_zoomState, 'bool');
	ser.Value("ammo", m_weapon_ammo, 'ammo');
	ser.Value("invammo", m_inventory_ammo, 'ammo');
	ser.Value("fired", m_fireCounter, 'ui8'); // 0 - 255
	ser.Value("meleeCount", m_meleeCounter, 'ui8');
	ser.Value("expendedAmmo", m_expended_ammo, 'ui8');
}

static int calcNumShots(uint8 prevCount, uint8 newCount)
{
	int result = 0;

	if(newCount == prevCount)
	{
		result = 0;
	}
	else if(newCount > prevCount)
	{
		result = newCount - prevCount;	
	}
	else // newCount < prevCount <-- wrap case
	{
		result = (255 - prevCount) + newCount + 1; // this needs to account for 0 being a shot as well, so we +1 to include that
	}
	
	return result;
}

//------------------------------------------------------------------------
bool CWeapon::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (!BaseClass::NetSerialize(ser, aspect, profile, flags))
		return false;

	if(aspect == ASPECT_STREAM)
	{
		SNetWeaponData data;
		
		if(ser.IsWriting())
		{
			data.m_weapon_ammo = NetGetCurrentAmmoCount();
			data.m_raised = NetGetWeaponRaised();
			data.m_firemode = GetCurrentFireMode();
			data.m_reload = GetReloadState();
			data.m_isFiring = m_isFiring;
			data.m_fireCounter = m_fireCounter;
			data.m_meleeCounter = m_meleeCounter;
			data.m_inventory_ammo = 0;
			data.m_expended_ammo = m_expended_ammo;
			EZoomState zoomState = GetZoomState();
			data.m_zoomState = (zoomState == eZS_ZoomingIn || zoomState == eZS_ZoomedIn);
			
			CActor *owner = GetOwnerActor();
			if (owner)
			{
				IInventory *inv = owner->GetInventory();
				if (inv)
				{
					IFireMode *ifm = GetFireMode(GetCurrentFireMode());
					if (ifm)
					{
						data.m_inventory_ammo = GetInventoryAmmoCount(ifm->GetAmmoType());
					}
				}
			}
			

			NetStateSent();
		}

		data.NetSerialise(ser);

		if(ser.IsReading())
		{
			bool allowUpdate = NetAllowUpdate(); // for base CWeapon, effectively if !Local

			if(m_firemode != data.m_firemode)
			{
				SetCurrentFireMode(data.m_firemode);
			}

			if(m_reloadState != data.m_reload)
			{
				ClSetReloadState(data.m_reload);
			}

			if(m_weaponRaised != data.m_raised)
			{
				if(allowUpdate)
					RaiseWeapon(data.m_raised);
			}

			CActor *owner = GetOwnerActor();
			int ammo_diff = 0;
			if (owner)
			{
				if(owner->IsClient())
				{
						ammo_diff = calcNumShots(data.m_expended_ammo, m_expended_ammo); // see if we've fired any extra shots before getting this update
				}

				// this should probably go in its own aspect
				if(m_lastRecvInventoryAmmo != data.m_inventory_ammo)
				{
					IInventory *inv = owner->GetInventory();
					if (inv)
					{
						IFireMode *ifm = GetFireMode(GetCurrentFireMode());
						if (ifm)
						{
							inv->SetAmmoCount(ifm->GetAmmoType(), data.m_inventory_ammo - ammo_diff);
						}
					}
					m_lastRecvInventoryAmmo = data.m_inventory_ammo;
				}
			}

			NetSetCurrentAmmoCount(data.m_weapon_ammo - ammo_diff);

			if(allowUpdate)
			{
				m_isFiring = data.m_isFiring;
				m_shootCounter += m_netInitialised ? calcNumShots(m_fireCounter, data.m_fireCounter) : 0;
				m_fireCounter = data.m_fireCounter; 

				if(m_meleeCounter != data.m_meleeCounter)
				{
					m_meleeCounter = data.m_meleeCounter;
					m_doMelee = true;
					m_shootCounter = 0;
				}

				if(m_shootCounter > 0 || m_isFiring || m_doMelee)
					RequireUpdate(eIUS_FireMode);	// force update

				EZoomState zoomState = GetZoomState();
				bool isZooming = (zoomState == eZS_ZoomingIn || zoomState == eZS_ZoomedIn);
				if (isZooming != data.m_zoomState)
				{
					if (data.m_zoomState)
					{
						StartZoom(m_owner.GetId(), 1);
					}
					else
					{
						StopZoom(m_owner.GetId());
					}
				}
			}

			m_netInitialised = true;
		}

	}
	
	// zoom modes are not serialized

	return true;
}

//------------------------------------------------------------------------
void CWeapon::FullSerialize( TSerialize ser )
{
	BaseClass::FullSerialize(ser);

	ser.BeginGroup("WeaponAmmo");
	if(ser.IsReading())
	{
		m_ammo.clear();
		m_bonusammo.clear();
	}
	TAmmoVector::iterator it = m_ammo.begin();
	int ammoAmount = m_ammo.size();
	ser.Value("AmmoAmount", ammoAmount);
	for(int i = 0; i < ammoAmount; ++i)
	{
		string name;
		int amount = 0;
		if(ser.IsWriting() && it->pAmmoClass)
		{
			name = it->pAmmoClass->GetName();
			amount = it->count;
			++it;
		}

		ser.BeginGroup("Ammo");
		ser.Value("AmmoName", name);
		ser.Value("Bullets", amount);
		ser.EndGroup();

		if(ser.IsReading())
		{
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			assert(pClass);

			SWeaponAmmoUtils::SetAmmo(m_ammo, pClass, amount);
		}
	}
	it = m_bonusammo.begin();
	ammoAmount = m_bonusammo.size();
	ser.Value("BonusAmmoAmount", ammoAmount);
	for(int i = 0; i < ammoAmount; ++i)
	{
		string name;
		int amount = 0;
		if(ser.IsWriting() && it->pAmmoClass)
		{
			name = it->pAmmoClass->GetName();
			amount = it->count;
			++it;
		}

		ser.BeginGroup("Ammo");
		ser.Value("AmmoName", name);
		ser.Value("Bullets", amount);
		ser.EndGroup();

		if(ser.IsReading())
		{
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			assert(pClass);

			SWeaponAmmoUtils::SetAmmo(m_bonusammo, pClass, amount);
		}
	}
	
	ser.Value("minDropAmmoAvail", m_minDropAmmoAvailable);
	ser.EndGroup();

	if(ser.IsReading())
		s_crosshairstats.Reset();

	if(GetOwnerId())
	{
		ser.BeginGroup("WeaponStats");

		//if (m_fm)
     // m_fm->Serialize(ser);
		int numFiremodes = GetNumOfFireModes();
		ser.Value("numFiremodes", numFiremodes);
		if(ser.IsReading())
		{
			assert ( numFiremodes == GetNumOfFireModes() );
			if(numFiremodes != GetNumOfFireModes())
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Num of firemodes changed - loading will be corrupted.");
		}
		for(int i = 0; i < numFiremodes; ++i)
			m_firemodes[i]->Serialize(ser);
    
		bool hasZoom = (m_zm)?true:false;
		ser.Value("hasZoom", hasZoom);
		if(hasZoom)
		{
			int zoomMode = m_zmId;
			ser.Value("ZoomMode", zoomMode);
			bool isZoomed = m_zm->IsZoomed();
			ser.Value("Zoomed", isZoomed);
			int zoomStep = m_zm->GetCurrentStep();
			ser.Value("ZoomStep", zoomStep);

			m_zm->Serialize(ser);

			if(ser.IsReading())
			{
				if(m_zmId != zoomMode)
					SetCurrentZoomMode(zoomMode);

				m_restartZoom = isZoomed;
				m_restartZoomStep = (zoomStep<=1)?zoomStep:1; //Only enter first zoom step, not succesive

        if (!isZoomed)
          m_zm->ExitZoom();
			}
		}

		bool reloading = false;
		if(ser.IsWriting())    
			reloading = m_fm ? m_fm->IsReloading() : 0;
		ser.Value("FireModeReloading", reloading);
		if(ser.IsReading() && reloading)
			Reload();
		ser.Value("Alternation", m_fire_alternation);
		ser.EndGroup();
	}
}

//------------------------------------------------------------------------
void CWeapon::PostSerialize()
{
	m_nextShotTime = 0.0f; //Reset this after loading

	BaseClass::PostSerialize();

	for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
		FixAccessories(GetAccessoryParams(it->first), true);

	//Firemodes 
	int numFiremodes = GetNumOfFireModes();
	for(int i = 0; i < numFiremodes; ++i)
		m_firemodes[i]->PostSerialize();

}

//------------------------------------------------------------------------
void CWeapon::SerializeLTL(TSerialize ser)
{
	BaseClass::SerializeLTL(ser);

	ser.BeginGroup("WeaponAmmo");
	if(ser.IsReading())
	{
		m_ammo.clear();
		m_bonusammo.clear();
		m_nextShotTime = 0.0f; //Reset this after loading
	}
	TAmmoVector::iterator it = m_ammo.begin();
	int ammoAmount = m_ammo.size();
	ser.Value("AmmoAmount", ammoAmount);
	for(int i = 0; i < ammoAmount; ++i, ++it)
	{
		string name;
		int amount = 0;
		if(ser.IsWriting() && it->pAmmoClass)
		{
			name = it->pAmmoClass->GetName();
			amount = it->count;
		}

		ser.BeginGroup("Ammo");
		ser.Value("AmmoName", name);
		ser.Value("Bullets", amount);
		ser.EndGroup();

		if(ser.IsReading())
		{
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			assert(pClass);

			SWeaponAmmoUtils::SetAmmo(m_ammo, pClass, amount);
		}
	}
	ser.EndGroup();

	if(GetOwnerId())
	{
		ser.BeginGroup("WeaponStats");

		//if (m_fm)
		// m_fm->Serialize(ser);
		int numFiremodes = GetNumOfFireModes();
		ser.Value("numFiremodes", numFiremodes);
		if(ser.IsReading())
		{
			assert ( numFiremodes == GetNumOfFireModes() );
			if(numFiremodes != GetNumOfFireModes())
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Num of firemodes changed - loading will be corrupted.");
		}
		for(int i = 0; i < numFiremodes; ++i)
			m_firemodes[i]->Serialize(ser);

		int currentFireMode = GetCurrentFireMode();
		ser.Value("currentFireMode", currentFireMode);
		if(ser.IsReading())
			SetCurrentFireMode(currentFireMode);

		bool hasZoom = (m_zm)?true:false;
		ser.Value("hasZoom", hasZoom);
		if(hasZoom)
		{
			int zoomMode = m_zmId;
			ser.Value("ZoomMode", zoomMode);
			bool isZoomed = m_zm->IsZoomed();
			ser.Value("Zoomed", isZoomed);
			int zoomStep = m_zm->GetCurrentStep();
			ser.Value("ZoomStep", zoomStep);

			m_zm->Serialize(ser);

			if(ser.IsReading())
			{
				if(m_zmId != zoomMode)
					SetCurrentZoomMode(zoomMode);

				m_restartZoom = isZoomed;
				m_restartZoomStep = zoomStep;

				if (!isZoomed)
					m_zm->ExitZoom();
			}
		}

		bool reloading = false;
		if(ser.IsWriting())    
			reloading = m_fm ? m_fm->IsReloading() : 0;
		ser.Value("FireModeReloading", reloading);
		if(ser.IsReading() && reloading)
			Reload();
		ser.Value("Alternation", m_fire_alternation);
		ser.EndGroup();

	}
}

//------------------------------------------------------------------------
void CWeapon::Update( SEntityUpdateContext& ctx, int update)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (IsDestroyed())
		return;

	switch (update)
	{
	case eIUS_FireMode:
		{
			NetUpdateFireMode();
			
			if (m_fm)
				m_fm->Update(ctx.fFrameTime, ctx.nFrameID);
			if (m_melee)
				m_melee->Update(ctx.fFrameTime, ctx.nFrameID);

			break;
		}

	case eIUS_Zooming:
		if (m_zm)
			m_zm->Update(ctx.fFrameTime, ctx.nFrameID);
		break;
	}
  
	BaseClass::Update(ctx, update);


	if (update==eIUS_General)
	{
		if (fabsf(s_dofSpeed)>0.001f)
		{
			s_dofValue += s_dofSpeed*ctx.fFrameTime;
			s_dofValue = CLAMP(s_dofValue, 0, 1);

			//GameWarning("Actual DOF value = %f",m_dofValue);
			if(s_dofSpeed < 0.0f)
			{
				s_focusValue -= s_dofSpeed*ctx.fFrameTime*150.0f;
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", 20.0f + s_focusValue);
			}
			gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", s_dofValue);
		}
	}

	CActor* pOwner = GetOwnerActor();
	if (pOwner)
	{
		CPlayer* pPlayer = pOwner->IsPlayer() ? static_cast<CPlayer*>(pOwner) : 0;
		if (pPlayer)
		{
			if (pPlayer->m_weaponFPAiming.GetSprintFactor() == 0)
			{
				m_delayedFireActionTimeOut = max(m_delayedFireActionTimeOut-ctx.fFrameTime, 0);
			}

			if (m_delayedZoomActionTimeOut > 0.0f)
			{
				CRY_ASSERT(pOwner->IsPlayer());
				SPlayerStats* pStats = static_cast<SPlayerStats*>(pOwner->GetActorStats());
				m_delayedZoomActionTimeOut -= ctx.fFrameTime;
				if (m_zm && m_delayedZoomActionTimeOut <= 0.0f)
				{
					m_delayedZoomActionTimeOut = 0.0f;
					SPlayerStats* pStats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
					m_zm->StartZoom(m_delayedZoomStayZoomedVal, true, 1);
				}
			}
		}
	}
}

void CWeapon::PostUpdate(float frameTime )
{
	if (m_fm)
		m_fm->PostUpdate(frameTime);
}


//------------------------------------------------------------------------
void CWeapon::ProcessEvent(SEntityEvent& event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	switch(event.event)
	{
	case ENTITY_EVENT_HIDE:
		{
			if(m_fm && !m_fm->AllowZoom())
				m_fm->Cancel();
			StopFire();
			ClearInputFlags();
		}
		break;

	case ENTITY_EVENT_RESET:
		{
			//Leaving game mode
			if(gEnv->pSystem->IsEditor() && !event.nParam[0])
			{
				m_listeners.Clear(); 
				break;
			}
		}
		break;

	case ENTITY_EVENT_ANIM_EVENT:
		{
			if (m_weaponsharedparams->reloadMagazineParams.supportsMagazineSwap)
			{
				const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
				ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
				if (pAnimEvent && pCharacter)
				{
					AnimationEvent(pCharacter, *pAnimEvent);
				}
			}
		}
		break;
	}

	BaseClass::ProcessEvent(event);
}

bool CWeapon::ResetParams()
{
	bool success = BaseClass::ResetParams();

	m_weaponsharedparams = g_pGame->GetGameSharedParametersStorage()->GetWeaponSharedParameters(GetEntity()->GetClass()->GetName(), false); 

	if(!m_weaponsharedparams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Uninitialised weapon params. Has weaponParams=\"1\" been included in your xml for item %s?", GetEntity()->GetName());

		return false;
	}
	
	return success; 
}

void CWeapon::PreResetParams()
{
	// deactivate everything
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); ++it)
		(*it)->Activate(false);
	// deactivate zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); ++it)
		(*it)->Activate(false);

	// clean up firemodes
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); ++it)
		(*it)->Release();

	// clean up zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); ++it)
		(*it)->Release();
}

//------------------------------------------------------------------------
void CWeapon::Reset()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_firemodes.resize(0);
	m_fm = 0;

	m_zoommodes.resize(0);
	m_zm = 0;

	BaseClass::Reset();
	ClearInputFlags();

	SetCurrentFireMode(0);
	SetCurrentZoomMode(0);

	// have to refix them here.. (they get overriden by SetCurrentFireMode above)
	for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
		FixAccessories(GetAccessoryParams(it->first), true);

	SetDefaultIdleAnimation(eIGS_FirstPerson, "idle");
	SetDefaultIdleAnimation(eIGS_ThirdPerson, "idle");
	SetDefaultIdleAnimation(eIGS_Owner, GetParams().idle);
	m_zoomedTime = 0.0f;
	m_doingMagazineSwap = false;
	m_isDeselecting = false;
}

//------------------------------------------------------------------------
void CWeapon::UpdateFPView(float frameTime)
{
	BaseClass::UpdateFPView(frameTime);

	UpdateCrosshair(frameTime);
	if (m_fm)
		m_fm->UpdateFPView(frameTime);
	if (m_zm)
		m_zm->UpdateFPView(frameTime);

	UpdateFPIdling(frameTime);
}

//------------------------------------------------------------------------
void CWeapon::UpdateFPIdling(float frameTime)
{
	CPlayer* pClient = static_cast<CPlayer*>(GetOwnerActor());
	if (pClient == NULL)
		return;

	IPlayerInput* pClientInput = pClient->GetPlayerInput();
	if (pClientInput == NULL)
		return;

	static float lastIdleBreakRefTime = 0.0f;
	const float currentTime = gEnv->pTimer->GetAsyncCurTime();

	//Benito: Perhaps play idle breaks in a layer bellow "actions", so it always gets overridden?
	if (IsBusy() || (m_fm && m_fm->IsFiring()) || (m_zm && m_zm->IsZoomed()))
	{
		lastIdleBreakRefTime = currentTime;
	}
	else 
	{
		const float idleBreakDelay = g_pGameCVars->cl_idleBreaksDelayTime;
		const float lastInputTime = pClientInput->GetLastRegisteredInputTime();
		const float referenceTime = (lastIdleBreakRefTime > lastInputTime) ? lastIdleBreakRefTime : lastInputTime;

		if ((currentTime - referenceTime) > idleBreakDelay)
		{
			PlayAction(g_pItemStrings->idle_break);
			lastIdleBreakRefTime = currentTime;
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::MeleeAttack()
{
	if (!CanMeleeAttack())
		return;

	if (m_melee)
	{
		if (m_fm)
		{
			if(!m_fm->AllowZoom())
				m_fm->Cancel();

			m_fm->StopFire();
			if(m_fm->IsReloading())
			{
				if(m_fm->CanCancelReload())
					RequestCancelReload();
				else
				{
					RequestCancelReload();
					return;
				}
			}
		}
		m_melee->Activate(true);
		m_melee->StartFire();
		m_melee->StopFire();
	}
}

//------------------------------------------------------------------------
bool CWeapon::CanMeleeAttack() const
{
	if(m_modifying || m_transitioning)
		return false;

	return m_melee && m_melee->CanFire();
}

//------------------------------------------------------------------------
void CWeapon::Select(bool select)
{
	CActor *pOwner = GetOwnerActor();
	if (select && (IsDestroyed() || (pOwner && pOwner->GetHealth() <= 0)))
		return;
	
	bool isClient = pOwner?pOwner->IsClient():false;

	//If actor is grabbed by player, don't let him select weapon
	if (select && pOwner && pOwner->GetActorStats() && pOwner->GetActorStats()->isGrabbed)
	{
		pOwner->HolsterItem(true);
		return;
	}

	BaseClass::Select(select);

	if (!select)
	{
		if(IsZoomed() || IsZoomingInOrOut())
			ExitZoom();

		if(isClient)
		{
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0.0f);
			ResetOwnerAnimationsInLayer();
		}

		m_switchingFireMode = false;
		m_isDeselecting = false;

		SetBusy(false);

		if (gEnv->bServer)
		{
			if (m_fmIds.size() && m_firemode != 0)
			{
				// If we're not in the default firemode, switch back
				SetCurrentFireMode(0);
			}
		}
	}

		if(m_weaponRaised)
		{
			SetWeaponRaised(false);
			SetDefaultIdleAnimation(eIGS_Owner,GetParams().idle);
		}

	if(GetOwnerId()==LOCAL_PLAYER_ENTITY_ID)
		FadeCrosshair(0, 1.0f, WEAPON_FADECROSSHAIR_SELECT);

	if(!select)
		SetNextShotTime(false);

	if (m_fm)
		m_fm->Activate(select);

	if (m_zm && !IsDualWield())
		m_zm->Activate(select);

	if(select)
		SetNextShotTime(true);
}

//---------------------------------------------------------------------
uint32 CWeapon::StartDeselection()
{
	if (m_isDeselecting)
		return GetCurrentAnimationTime(eIGS_Owner);

	StopFire();

	if(IsZoomed() || IsZoomingInOrOut())
	{
		ExitZoom();
	}

	PlayAction(g_pItemStrings->deselect);
	m_isDeselecting = true;
	SetBusy(true);

	return GetCurrentAnimationTime(eIGS_Owner);
}


//------------------------------------------------------------------------
void CWeapon::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	//Need to check if in game for proper ammo/bonus reseting
	bool inGame = (!gEnv->bEditor) || (gEnv->bEditorGameMode);
	if (inGame)
	{
		CActor* pOwner = GetOwnerActor();
		IInventory* pOwnerInventory = pOwner ? pOwner->GetInventory() : NULL;

		//Note: Net-synch should work, since this code is executed in server and  client,
		//      and as long as server has the right ammo/bonus count, it should be fine
		//			This code as well needs to be executed before the owner gets reseted
		if (pOwnerInventory)
		{
			bool ownerIsAI = !pOwner->IsPlayer();
			if (ownerIsAI)
			{
				OnDroppedByAI(pOwnerInventory);
			}
			else
			{
				OnDroppedByPlayer(pOwnerInventory);

				bool  ownerIsClient = pOwner->IsClient();
				bool 	doDualWieldAmmoExchange = IsDualWieldMaster() && ((gEnv->bMultiplayer && IsServer()) || (ownerIsClient));
				if (doDualWieldAmmoExchange)
				{
					OnDualdWieldWeaponDropped(pOwnerInventory);
				}
			}

			if (pOwnerInventory->FindItem(GetEntityId()) >= 0)
			{
				UnregisterUsedAmmoWithInventory(pOwnerInventory);
			}
		}
	}

	BaseClass::Drop(impulseScale, selectNext, byDeath);
}

//------------------------------------------------------------------------
void CWeapon::SetFiringLocator(IWeaponFiringLocator *pLocator)
{
	if (m_pFiringLocator && m_pFiringLocator != pLocator)
		m_pFiringLocator->WeaponReleased();
	m_pFiringLocator = pLocator;
};

//------------------------------------------------------------------------
IWeaponFiringLocator *CWeapon::GetFiringLocator() const
{
	return m_pFiringLocator;
};

//------------------------------------------------------------------------
void CWeapon::AddEventListener(IWeaponEventListener *pListener, const char *who)
{
	m_listeners.Add(pListener, who);
}

//------------------------------------------------------------------------
void CWeapon::RemoveEventListener(IWeaponEventListener *pListener)
{
	m_listeners.Remove(pListener);
}

//------------------------------------------------------------------------
Vec3 CWeapon::GetFiringPos(const Vec3 &probableHit) const
{
	if (m_fm)
		return m_fm->GetFiringPos(probableHit);

	return Vec3(0,0,0);
}

//------------------------------------------------------------------------
Vec3 CWeapon::GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const
{
	if (m_fm)
		return m_fm->GetFiringDir(probableHit, firingPos);

	return Vec3(0,0,0);
}

//------------------------------------------------------------------------
void CWeapon::StartFire(const Vec3 *fireTarget)
{
	CActor *pOwner = GetOwnerActor();
  if (IsDestroyed())
    return;

	if(pOwner)
	{
		bool isPlayer = pOwner->IsPlayer();
		if((pOwner->GetHealth() <= 0) || (!pOwner->CanFire()))
			return;
		//Dual socoms for AI
		if(!gEnv->bMultiplayer && !isPlayer && IsDualWieldMaster() && FireSlave(GetOwnerId(),true))
			return;
	}

	if (m_fm)
	{
		m_fm->StartFire();
	}
}

//------------------------------------------------------------------------
void CWeapon::StopFire()
{
	//Stop slave
	if(IsDualWieldMaster())
		FireSlave(GetOwnerId(), false);

	if (m_fm)
	{
		m_fm->StopFire();
	}

	if (m_delayedFireActionTimeOut > 0)
	{
		SetBusy(false);
	}

	m_delayedFireActionTimeOut = 0.0f;
}

//------------------------------------------------------------------------
bool CWeapon::CanFire() const
{
	return m_fm && m_fm->CanFire();
}

//------------------------------------------------------------------------
void CWeapon::StartZoom(EntityId actorId, int zoomed)
{
	if (m_zm == NULL)
		return;

	CActor *pOwner = GetOwnerActor();
	if (pOwner == NULL)
		return;

	if (IsDestroyed() || (pOwner->GetHealth() <= 0))
		return;

	if (m_fm && !((CFireMode*)m_fm)->CanZoom())
		return;

	CActor *pActor = static_cast<CActor*>(pOwner);
	if (pActor)
	{
		if (!pActor->CanUseIronSights())
		{
			return;
		}
	}

	bool interruptingZoomOutWithZoom = (m_zm->IsZoomingInOrOut() && !m_zm->IsZoomingIn());	//This allows us to re-zoom while the zooming out anim is still playing giving better responsiveness

	bool stayZoomed = (m_zm->IsZoomed() && ((m_zm->IsZoomingInOrOut() && !m_zm->IsToggle()) || interruptingZoomOutWithZoom ));

	if (!m_zm->IsZoomed() || interruptingZoomOutWithZoom)
	{
		bool delayZoom = false;
		if (pOwner->IsPlayer())
		{
			SPlayerStats* pStats = static_cast<SPlayerStats*>(pOwner->GetActorStats());
			if (pStats->bSprinting)
			{
				pStats->bIgnoreSprinting = true;
				m_delayedZoomActionTimeOut = GetParams().sprintToZoomDelay;
				m_delayedZoomStayZoomedVal = stayZoomed;
				delayZoom = true;
			}
		}
		if (!delayZoom && m_delayedZoomActionTimeOut==0)
		{
			m_zm->StartZoom(stayZoomed, true, zoomed);
		}
		RequestSetZoomState(true);
	}
}

//------------------------------------------------------------------------
void CWeapon::StopZoom(EntityId actorId)
{
	if (m_zm && m_delayedZoomActionTimeOut == 0.0f)
	{
		m_zm->StopZoom();
		RequestSetZoomState(false);
	}
	else
	{
		m_delayedZoomActionTimeOut = 0.0f;
	}
}

//------------------------------------------------------------------------
bool CWeapon::CanZoom() const
{
	return m_zm && m_zm->CanZoom();
}

//------------------------------------------------------------------------
void CWeapon::ExitZoom(bool force)
{
	if (m_zm)
		m_zm->ExitZoom(force);
}

//------------------------------------------------------------------------
bool CWeapon::CanModify() const
{
	return true;
}

//------------------------------------------------------------------------
bool CWeapon::IsZoomed() const
{
	return m_zm && m_zm->IsZoomed();
}

//------------------------------------------------------------------------
bool CWeapon::IsZoomingInOrOut() const
{
	return m_zm && m_zm->IsZoomingInOrOut();
}

//------------------------------------------------------------------------
bool CWeapon::IsZoomingIn() const
{
	if (m_zm)
	{
		return (m_zm->GetZoomState() == eZS_ZoomingIn);
	}
	
	return false;
}

//------------------------------------------------------------------------
EZoomState CWeapon::GetZoomState() const
{
	if (m_zm)
		return m_zm->GetZoomState();
	return eZS_ZoomedOut;
}

//------------------------------------------------------------------------
float CWeapon::GetZoomInTime() const
{
	float time = m_zm ? m_zm->GetZoomInTime() : 0.0f;
	
	return time;
}

//------------------------------------------------------------------------
bool CWeapon::ShouldSnapToTarget() const
{
	if (m_zm)
	{
		if (m_zm->AllowsZoomSnap())
		{
			EZoomState zoomState = m_zm->GetZoomState();

			if (zoomState == eZS_ZoomingIn)
		return true;

			if (zoomState == eZS_ZoomedIn)
	{
		//Parameterize this value per weapon if required
		const float zoomedSnapTime = 0.25f;
		return ((gEnv->pTimer->GetAsyncCurTime() - m_zoomedTime) < zoomedSnapTime);
	}
		}
	}

	return false;
}

//---------------------------------------------------------------------
bool CWeapon::IsReloading() const
{
	return m_fm && m_fm->IsReloading();
}


bool CWeapon::IsValidAssistTarget(IEntity *pEntity, IEntity *pSelf,bool includeVehicles/*=false*/)
{
	if(!pEntity)
		return false;

	IActor *pActor = m_pGameFramework->GetIActorSystem()->GetActor(pEntity->GetId());
	IAIObject *pAI = pEntity->GetAI();

	if(!pActor && includeVehicles && pAI)
	{
		IVehicle *pVehicle = m_pGameFramework->GetIVehicleSystem()->GetVehicle(pEntity->GetId()); 
		if (pVehicle && pVehicle->GetStatus().health > 0.f && pAI->IsHostile(pSelf->GetAI(),false))
			return true;
	}

	// Check for target validity
	if(!pActor)
		return false;

	if (!pAI)
	{
		if (pActor->IsPlayer() && pEntity != pSelf && !pEntity->IsHidden() &&
				pActor->GetHealth() > 0)
		{
			int ownteam=g_pGame->GetGameRules()->GetTeam(pSelf->GetId());
			int targetteam=g_pGame->GetGameRules()->GetTeam(pEntity->GetId());
			
			// Assist aiming on non-allied players only
			return (targetteam == 0 && ownteam == 0) || (targetteam != 0 && targetteam != ownteam);
		}
		else
		{
			return false;
		}
	}
	
	return (pEntity != pSelf &&!pEntity->IsHidden() && 
		pActor->GetHealth() > 0 &&	pAI->GetAIType() != AIOBJECT_VEHICLE &&
		pAI->IsHostile(pSelf->GetAI(),false));
}

//------------------------------------------------------------------------
void CWeapon::RestartZoom(bool force)
{
	if(m_restartZoom || force)
	{
		if(m_zm && !IsBusy() && m_zm->CanZoom())
		{
			m_zm->StartZoom(true, false, m_restartZoomStep);

			m_restartZoom = false;
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::MountAt(const Vec3 &pos)
{
	BaseClass::MountAt(pos);

//Diesel
//325
//	GetEntity()->RegisterInAISystem(AIObjectParams(AIOBJECT_MOUNTEDWEAPON));

//320
	AIObjectParameters params;
	GetEntity()->RegisterInAISystem(AIOBJECT_MOUNTEDWEAPON, params);
//Diesel


}

//------------------------------------------------------------------------
void CWeapon::MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles)
{
	BaseClass::MountAtEntity(entityId, pos, angles);

	if (gEnv->bServer && !m_bonusammo.empty())
	{
		TAmmoVector::const_iterator bonusAmmoEndCit = m_bonusammo.end();
		for (TAmmoVector::const_iterator bonusAmmoCit = m_bonusammo.begin(); bonusAmmoCit != bonusAmmoEndCit; ++bonusAmmoCit)
		{
			const SWeaponAmmo& bonusAmmo = *bonusAmmoCit;
			SetInventoryAmmoCount(bonusAmmo.pAmmoClass, GetInventoryAmmoCount(bonusAmmo.pAmmoClass)+bonusAmmo.count);
		}

		m_bonusammo.clear();
	}
}

//------------------------------------------------------------------------
void CWeapon::Reload(bool force)
{
	CActor *pOwner = GetOwnerActor();
	bool isClient = false;

	if(pOwner)
	{ 
		isClient = pOwner->IsClient();
		if(pOwner->IsPlayer())
		{
			//Player can not reload while superjumping
			if(pOwner->IsSuperJumping())
				return;
		}
		else
		{
			if(IsDualWieldMaster())
				ReloadSlave();
		}
	}

	if (m_fm)
	{
		if (m_fm->CanReload() || force)
		{
			int zoomStep = m_zm ? m_zm->GetCurrentStep() : 0;
			m_fm->Reload(zoomStep);

			if (!pOwner || pOwner->IsClient())
				RequestReload();

			SetDefaultIdleAnimation(eIGS_Owner, GetParams().idle);
		}
		else if (m_fm->OutOfAmmo())
		{
			SetDefaultIdleAnimation(eIGS_Owner, GetParams().idle_empty);
		}
	}
}

//------------------------------------------------------------------------
bool CWeapon::CanReload() const
{
	if (m_fm)
		return m_fm->CanReload();
	return true;
}

//------------------------------------------------------------------------
bool CWeapon::OutOfAmmo(bool allFireModes) const
{
	if (!allFireModes)
		return m_fm && m_fm->OutOfAmmo();

	for (int i=0; i<m_firemodes.size(); i++)
		if (!m_firemodes[i]->OutOfAmmo())
			return false;

	return true;
}

//------------------------------------------------------------------------
bool CWeapon::LowAmmo(float thresholdPerCent) const
{
	return m_fm && m_fm->LowAmmo(thresholdPerCent);
}

//------------------------------------------------------------------------
int CWeapon::GetAmmoCount(IEntityClass* pAmmoType) const
{
	return SWeaponAmmoUtils::GetAmmoCount(m_ammo, pAmmoType);
}

//------------------------------------------------------------------------
void CWeapon::SetAmmoCount(IEntityClass* pAmmoType, int count)
{
	SWeaponAmmoUtils::SetAmmo(m_ammo, pAmmoType, count);

	CHANGED_NETWORK_STATE(this, ASPECT_STREAM);

	// send game event
	IEntity *pOwnerEntity = gEnv->pEntitySystem->GetEntity(m_owner.GetId());
	if(pOwnerEntity)
	{
		m_pGameFramework->GetIGameplayRecorder()->Event(pOwnerEntity,GameplayEvent(eGE_AmmoCount,GetEntity()/*->GetClass()*/->GetName(),float(count),(void*)GetFireModeIdxWithAmmo(pAmmoType)));
	}

	OnSetAmmoCount(GetOwnerId());
}

//------------------------------------------------------------------------
bool CWeapon::CanPickUpAmmo(IInventory* pDestinationInventory)
{
	for (TAmmoVector::iterator it = m_ammo.begin(); it != m_ammo.end(); ++it)
	{
		IEntityClass* pAmmoClass = it->pAmmoClass;
		if(SWeaponAmmoUtils::FindAmmoConst(m_weaponsharedparams->ammoParams.accessoryAmmo, pAmmoClass) != NULL)
		{
			continue;
		}

		int actualCount = pDestinationInventory->GetAmmoCount(pAmmoClass);
		int maxCapacity = pDestinationInventory->GetAmmoCapacity(pAmmoClass);
		
		if (maxCapacity == 0 || actualCount < maxCapacity)
		{		
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
int CWeapon::GetInventoryAmmoCount(IEntityClass* pAmmoType) const
{
	if (m_hostId)
	{
		IVehicle *pVehicle=m_pGameFramework->GetIVehicleSystem()->GetVehicle(m_hostId);
		if (pVehicle)
			return pVehicle->GetAmmoCount(pAmmoType);

		return 0;
	}

	if (g_pGameCVars->g_infiniteAmmo)
	{
		return 999;
	}

	IInventory *pInventory=GetActorInventory(GetOwnerActor());
	if (!pInventory)
		return 0;

	return pInventory->GetAmmoCount(pAmmoType);
}

//------------------------------------------------------------------------
void CWeapon::SetInventoryAmmoCount(IEntityClass* pAmmoType, int count)
{
	if(!pAmmoType)
		return;

	if (m_hostId)
	{
		IVehicle *pVehicle = m_pGameFramework->GetIVehicleSystem()->GetVehicle(m_hostId);
		if (pVehicle)
			pVehicle->SetAmmoCount(pAmmoType, count);
		
		return;
	}

	IInventory* pInventory = GetActorInventory(GetOwnerActor());
	
	SetInventoryAmmoCountInternal(pInventory, pAmmoType, count);
}

//-----------------------------------------------------------------------
void CWeapon::SetInventoryAmmoCountInternal(IInventory* pInventory, IEntityClass* pAmmoType, int count)
{
	if (pInventory)
	{
		IActor* pInventoryOwner = pInventory->GetActor();
		bool isLocalClient = pInventoryOwner ? pInventoryOwner->IsClient() : false;

		bool ammoChanged = false;
		bool ammoFull = false;
		const int capacity = pInventory->GetAmmoCapacity(pAmmoType);
		const int current = pInventory->GetAmmoCount(pAmmoType);
		if(count >= capacity)
		{
			//If still there's some place, full inventory to maximum...
			if(current < capacity)
			{
				ammoChanged = true;
				ammoFull = true;
				pInventory->SetAmmoCount(pAmmoType,capacity);
			}
		}
		else
		{
			ammoChanged = true;
			pInventory->SetAmmoCount(pAmmoType, count);
		}

		CHANGED_NETWORK_STATE(this, ASPECT_STREAM);

		if(isLocalClient && ammoChanged)
		{
			const int difference = min(count, capacity) - current;

			SHUDEvent eventPickUp(eHUDEvent_OnAmmoPickUp);
			eventPickUp.AddData(SHUDEventData((void*)GetIWeapon()));
			eventPickUp.AddData(SHUDEventData(difference));
			eventPickUp.AddData(SHUDEventData((void*)pAmmoType));
			CHUD::CallEvent(eventPickUp);

/*
			if(difference>0)
			{
				SHUDEvent eventMessage(eHUDEvent_AddOnScreenMessage);
				const char* msg = "Picked up %2 %1";
				const char* ammoName = pAmmoType->GetName();
				CryFixedStringT<8> ammo;
				ammo.Format("%d", difference);
				eventMessage.ReserveData(3);
				eventMessage.AddData(SHUDEventData((void*)msg));
				eventMessage.AddData(SHUDEventData((void*)ammoName));
				eventMessage.AddData(SHUDEventData((void*)ammo.c_str()));
				CHUD::CallEvent(eventMessage);
			}

			if(ammoFull)
			{
				SHUDEvent eventFull(eHUDEvent_AddOnScreenMessage);
				const char* msg = "%1 ammo full";
				const char* param = pAmmoType->GetName();
				eventFull.AddData(SHUDEventData((void*)msg));
				eventFull.AddData(SHUDEventData((void*)param));
				CHUD::CallEvent(eventFull);
			}
*/
		}
	}
}

//------------------------------------------------------------------------
IFireMode *CWeapon::GetFireMode(int idx) const
{
	if (idx >= 0 && idx < m_firemodes.size())
		return m_firemodes[idx];
	return 0;
}

//------------------------------------------------------------------------
IFireMode *CWeapon::GetFireMode(const char *name) const
{
	TFireModeIdMap::const_iterator it = m_fmIds.find(CONST_TEMP_STRING(name));
	if (it == m_fmIds.end())
		return 0;

	return GetFireMode(it->second);
}

//------------------------------------------------------------------------
int CWeapon::GetFireModeIdxWithAmmo(const IEntityClass* pAmmoClass) const
{
	TFireModeIdMap::const_iterator it = m_fmIds.begin(),itEnd =m_fmIds.end() ;
	for(;it != itEnd; ++it)
	{
		IFireMode* pFireMode = GetFireMode(it->second);
		if(pFireMode && pFireMode->GetAmmoType() == pAmmoClass)
			return it->second;
	}
	return -1;
}

//------------------------------------------------------------------------
int CWeapon::GetFireModeIdx(const char *name) const
{
	TFireModeIdMap::const_iterator it = m_fmIds.find(CONST_TEMP_STRING(name));
	if (it != m_fmIds.end())
		return it->second;
	return -1;
}

//------------------------------------------------------------------------
int CWeapon::GetCurrentFireMode() const
{
	return m_firemode;
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentFireMode(int idx)
{
	if (m_firemodes.empty())
		return;

	if (m_fm)
		m_fm->Activate(false);

	if (idx >= m_firemodes.size())
		m_fm = 0;
	else
		m_fm = m_firemodes[idx];

	if (m_fm)
	{
		m_fm->Activate(true);

		if (IsServer())
		{
			if(GetOwnerId())
			{
				m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponFireModeChanged, m_fm->GetName(), (float)idx, (void *)GetEntityId()));
			}

			CHANGED_NETWORK_STATE(this, ASPECT_STREAM);
		}
	}

	m_firemode = idx;

	if (!IsZoomed())
	{
		ResetActionSuffix(0.f);
	}

	OnFireModeChanged(m_firemode);
}


//------------------------------------------------------------------------
void CWeapon::SetCurrentFireMode(const char *name)
{
	TFireModeIdMap::iterator it = m_fmIds.find(CONST_TEMP_STRING(name));
	if (it == m_fmIds.end())
		return;

	SetCurrentFireMode(it->second);
}

//------------------------------------------------------------------------
void CWeapon::ChangeFireMode()
{
	int newId = GetNextFireMode(GetCurrentFireMode());

	if (newId != GetCurrentFireMode())
	{
		RequestFireMode(newId);
	}
}

//------------------------------------------------------------------------
int CWeapon::GetNextFireMode(int currMode) const
{
	if (m_firemodes.empty() || (currMode > (m_firemodes.size()-1)))
		return 0;

	int t = currMode;
	do {
		t++;
		if (t == m_firemodes.size())
			t = 0;
		if (IFireMode* pFM = GetFireMode(t))
			if(pFM->IsEnabled() && !IsFiremodeDisabledByAccessory(t))
				return t;
	} while(t!=currMode);

	return t;
}

//------------------------------------------------------------------------
void CWeapon::EnableFireMode(int idx, bool enable)
{
	IFireMode *pFireMode = GetFireMode(idx);
	if (pFireMode)
		pFireMode->Enable(enable);
}

//------------------------------------------------------------------------
bool CWeapon::IsFiremodeDisabledByAccessory(int idx) const
{
	for (TAccessoryMap::const_iterator ait = m_accessories.begin(); ait!= m_accessories.end(); ++ait)
	{
		const SAccessoryParams* pParams = GetAccessoryParams(ait->first);

		int numDisabledFiremodes = pParams->disableFiremodes.size();

		for (int i = 0; i < numDisabledFiremodes; i++)
		{
			if(GetFireModeIdx(pParams->disableFiremodes[i]) == idx)
			{
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------
IZoomMode *CWeapon::GetZoomMode(int idx) const
{
	if (idx >= 0 && idx < m_zoommodes.size())
		return m_zoommodes[idx];
	return 0;
}

//------------------------------------------------------------------------
IZoomMode *CWeapon::GetZoomMode(const char *name) const
{
	TZoomModeIdMap::const_iterator it = m_zmIds.find(CONST_TEMP_STRING(name));
	if (it == m_zmIds.end())
		return 0;

	return GetZoomMode(it->second);
}

//------------------------------------------------------------------------
int CWeapon::GetZoomModeIdx(const char *name) const
{
	TZoomModeIdMap::const_iterator it = m_zmIds.find(CONST_TEMP_STRING(name));
	if (it != m_zmIds.end())
		return it->second;
	return -1;
}

//------------------------------------------------------------------------
int CWeapon::GetCurrentZoomMode() const
{
	return m_zmId;
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentZoomMode(int idx)
{
	if (m_zoommodes.empty())
		return;

	m_zm = m_zoommodes[idx];
	m_zmId = idx;
	m_zm->Activate(true);
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentZoomMode(const char *name)
{
	TZoomModeIdMap::iterator it = m_zmIds.find(CONST_TEMP_STRING(name));
	if (it == m_zmIds.end())
		return;

	SetCurrentZoomMode(it->second);
}

//------------------------------------------------------------------------
void CWeapon::ChangeZoomMode()
{
	if (m_zoommodes.empty())
		return;
/*
	if (m_zmId+1<m_zoommodes.size())
		SetCurrentZoomMode(m_zmId+1);
	else if (m_zoommodes.size()>1)
		SetCurrentZoomMode(0);
		*/
	int t = m_zmId;
	do {
		t++;
		if (t == m_zoommodes.size())
			t = 0;
		if (GetZoomMode(t)->IsEnabled())
			SetCurrentZoomMode(t);
	} while(t!=m_zmId);
}

//------------------------------------------------------------------------
void CWeapon::EnableZoomMode(int idx, bool enable)
{
	IZoomMode *pZoomMode = GetZoomMode(idx);
	if (pZoomMode)
		pZoomMode->Enable(enable);
}

//------------------------------------------------------------------------
bool CWeapon::IsServerSpawn(IEntityClass* pAmmoType) const
{
	return g_pGame->GetWeaponSystem()->IsServerSpawn(pAmmoType);
}

//------------------------------------------------------------------------
CProjectile *CWeapon::SpawnAmmo(IEntityClass* pAmmoType, bool remote)
{
	if(gEnv->bServer && g_pGame->GetGameRules())
	{
		if(CBattleDust* pBD = g_pGame->GetGameRules()->GetBattleDust())
		{
			pBD->RecordEvent(eBDET_ShotFired, GetEntity()->GetWorldPos(), GetEntity()->GetClass());
		}
	}

	return g_pGame->GetWeaponSystem()->SpawnAmmo(pAmmoType, remote);
}

//------------------------------------------------------------------------
void CWeapon::SetCrosshairVisibility(bool visible)
{
	s_crosshairstats.visible = visible;
}

//------------------------------------------------------------------------
bool CWeapon::GetCrosshairVisibility() const
{
	return s_crosshairstats.visible;
}

//------------------------------------------------------------------------
void CWeapon::SetCrosshairOpacity(float opacity)
{
	s_crosshairstats.opacity = opacity; 
}

//------------------------------------------------------------------------
float CWeapon::GetCrosshairOpacity() const
{
	return s_crosshairstats.opacity;
}

//------------------------------------------------------------------------
void CWeapon::FadeCrosshair(float from, float to, float time)
{
	s_crosshairstats.fading = true;
	s_crosshairstats.fadefrom = from;
	s_crosshairstats.fadeto = to;
	s_crosshairstats.fadetime = MAX(0, time);
	s_crosshairstats.fadetimer = s_crosshairstats.fadetime;

	SetCrosshairOpacity(from);
}

//------------------------------------------------------------------------
void CWeapon::UpdateCrosshair(float frameTime)
{
	if (s_crosshairstats.fading)
	{
		if (s_crosshairstats.fadetimer>0.0f)
		{
			s_crosshairstats.fadetimer -= frameTime;
			if (s_crosshairstats.fadetimer<0.0f)
				s_crosshairstats.fadetimer=0.0f;

			float t = (s_crosshairstats.fadetime-s_crosshairstats.fadetimer)/s_crosshairstats.fadetime;
			float d = (s_crosshairstats.fadeto-s_crosshairstats.fadefrom);

			if (t >= 1.0f)
				s_crosshairstats.fading = false;

			if (d < 0.0f)
				t = 1.0f-t;

			if(s_crosshairstats.fadefrom == s_crosshairstats.fadeto)
				s_crosshairstats.opacity = s_crosshairstats.fadeto;
			else
				s_crosshairstats.opacity = fabsf(t*d);
		}
		else
		{
			s_crosshairstats.opacity = s_crosshairstats.fadeto;
			s_crosshairstats.fading = false;
		}
	}

	if(m_restartZoom)
		RestartZoom();
}

//------------------------------------------------------------------------
void CWeapon::AccessoriesChanged()
{
	int i = 0;
	for (TFireModeVector::iterator it=m_firemodes.begin(); it!=m_firemodes.end(); ++it)
	{
		for (TFireModeIdMap::iterator iit=m_fmIds.begin(); iit!=m_fmIds.end(); ++iit)
		{
			if (iit->second == i)
			{
				CFireMode* pFireMode = static_cast<CFireMode*>(*it);

				const SParentFireModeParams* parentParams = pFireMode->GetParentShared();	

				for (TAccessoryMap::iterator ait=m_accessories.begin(); ait!=m_accessories.end(); ++ait)
				{
					SParentFireModeParams::TAccessoryFireModeMap::const_iterator accessoryParamsIt = parentParams->accessoryChangedParams.find(ait->first);

					if(accessoryParamsIt != parentParams->accessoryChangedParams.end())
					{
						const SFireModeParams* newParams = &accessoryParamsIt->second;

						pFireMode->ResetSharedParams(newParams);
						break;
					}				
				}	
				break;
			}
		}

		++i;
	}

	i = 0;
	for (TZoomModeVector::iterator it=m_zoommodes.begin(); it!=m_zoommodes.end(); ++it)
	{
		for (TZoomModeIdMap::iterator iit=m_zmIds.begin(); iit!=m_zmIds.end(); ++iit)
		{
			if (iit->second == i)
			{
				
				CIronSight* pZoomMode = static_cast<CIronSight*>(*it);

				const SParentZoomModeParams* parentParams = pZoomMode->GetParentShared();	

				for (TAccessoryMap::iterator ait=m_accessories.begin(); ait!=m_accessories.end(); ++ait)
				{
					SParentZoomModeParams::TAccessoryZoomModeMap::const_iterator accessoryParamsIt = parentParams->accessoryChangedParams.find(ait->first);

					if(accessoryParamsIt != parentParams->accessoryChangedParams.end())
					{
						const SZoomModeParams* newParams = &accessoryParamsIt->second;

						pZoomMode->ResetSharedParams(newParams);
						break;
					}				
				}	
				break;
			}
		}
		++i;
	}

	//Second SOCOM
	bool isDualWield = IsDualWieldMaster();
	CWeapon *dualWield = 0;

	if (isDualWield)
	{
		IItem *slave = GetDualWieldSlave();
		if (slave && slave->GetIWeapon())
			dualWield = static_cast<CWeapon *>(slave);
		else
			isDualWield = false;
	}

	if(isDualWield)
		dualWield->AccessoriesChanged();

	if(IActor *pOwner = GetOwnerActor())
		if(pOwner->IsClient())
			SAFE_HUD_FUNC(UpdateCrosshair());
}

//------------------------------------------------------------------------
float CWeapon::GetSpinUpTime() const
{
	if (m_fm)
		return m_fm->GetSpinUpTime();
	return 0.0f;
}

//------------------------------------------------------------------------
float CWeapon::GetSpinDownTime() const
{
	if (m_fm)
		return m_fm->GetSpinDownTime();
	return 0.0f;
}

//------------------------------------------------------------------------
void CWeapon::SetHostId(EntityId hostId)
{
	m_hostId=hostId;
}

//------------------------------------------------------------------------
EntityId CWeapon::GetHostId() const
{
	return m_hostId;
}

//------------------------------------------------------------------------
void CWeapon::FixAccessories(const SAccessoryParams *params, bool attach)
{
	if (!attach)
	{
		if (params)
		{
			for (int i = 0; i < params->firemodes.size(); i++)
			{
				if (params->exclusive && GetFireModeIdx(params->firemodes[i]) != -1)
				{
					EnableFireMode(GetFireModeIdx(params->firemodes[i]), false);
				}
			}
			if (IFireMode * pFM = GetFireMode(GetCurrentFireMode()))
			{
				if(!pFM->IsEnabled())
					ChangeFireMode();
			}
			
			if (GetZoomModeIdx(params->zoommode) != -1)
			{
				EnableZoomMode(GetZoomModeIdx(params->zoommode), false);
				ChangeZoomMode();
			}
		}
	}
	else if(params)
	{
		if (!params->switchToFireMode.empty())
		{
			SetCurrentFireMode(params->switchToFireMode.c_str());
		}
		else if(IsFiremodeDisabledByAccessory(GetCurrentFireMode()))
		{
			ChangeFireMode();
		}

		for (int i = 0; i < params->firemodes.size(); i++)
		{
			if (GetFireModeIdx(params->firemodes[i]) != -1)
			{
				GetFireMode(params->firemodes[i].c_str())->Enable(true);
			}
		}
		if (GetZoomModeIdx(params->zoommode) != -1)
		{
			EnableZoomMode(GetZoomModeIdx(params->zoommode), true);
			SetCurrentZoomMode(GetZoomModeIdx(params->zoommode));
		}
		
	}	
}

//------------------------------------------------------------------------
void CWeapon::SetDestinationEntity(EntityId targetId)
{
  // default: Set bbox center as destination
  IEntity* pEntity = gEnv->pEntitySystem->GetEntity(targetId);

  if (pEntity)
  {
    AABB box;
    pEntity->GetWorldBounds(box);
    
    SetDestination(box.GetCenter());
  }
}

//------------------------------------------------------------------------
bool CWeapon::PredictProjectileHit(IPhysicalEntity *pShooter, const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speed,
																	 Vec3& predictedPosOut, float& projectileSpeedOut,
																	 Vec3* pTrajectory, unsigned int* trajectorySizeInOut) const
{
	IFireMode *pFireMode = GetFireMode(GetCurrentFireMode());
	if (!pFireMode)
		return false;

	IEntityClass* pAmmoType = pFireMode->GetAmmoType();
	if (!pAmmoType)
		return false;

	CProjectile* pTestProjectile = g_pGame->GetWeaponSystem()->SpawnAmmo(pAmmoType);
	if (!pTestProjectile)
		return false;
	IPhysicalEntity* pProjectilePhysEntity = pTestProjectile->GetEntity()->GetPhysics();
	if (!pProjectilePhysEntity)
		return false;


	projectileSpeedOut = pTestProjectile->GetSpeed();

	pTestProjectile->SetVelocity(pos, dir, velocity, speed/projectileSpeedOut);

	pe_params_flags particleFlags;
	particleFlags.flagsAND = ~(pef_log_collisions & pef_traceable & pef_log_poststep);
	pProjectilePhysEntity->SetParams(&particleFlags);

	pe_params_particle partPar;
	partPar.pColliderToIgnore = pShooter;
	pProjectilePhysEntity->SetParams(&partPar);

	pe_params_pos paramsPos;
	paramsPos.iSimClass = 6;
	paramsPos.pos = pos;
	pProjectilePhysEntity->SetParams(&paramsPos);

	unsigned int n = 0;
	const unsigned int maxSize = trajectorySizeInOut ? *trajectorySizeInOut : 0;

	if (pTrajectory && n < maxSize)
		pTrajectory[n++] = pos;

	const float	dt = 0.12f;
	const float	lifeTime = pTestProjectile->GetLifeTime();

	int stationary = 0;
	Vec3 lastPos = pos;

	for (float t = 0.0f; t < lifeTime; t += dt)
	{
		pProjectilePhysEntity->StartStep(dt);
		pProjectilePhysEntity->DoStep(dt);

		pe_status_pos	statusPos;
		pProjectilePhysEntity->GetStatus(&statusPos);
		float distSq = Distance::Point_PointSq(lastPos, statusPos.pos);
		lastPos = statusPos.pos;

		// Early out when almost stationary.
		if (distSq < sqr(0.01f))
			stationary++;
		else
			stationary = 0;
		if (stationary > 2)
			break;

		if (pTrajectory && n < maxSize)
		{
			pe_status_pos	statusPos;
			pProjectilePhysEntity->GetStatus(&statusPos);
			pTrajectory[n++] = statusPos.pos;
		}
	}

	if (trajectorySizeInOut)
		*trajectorySizeInOut = n;

	pe_status_pos	statusPos;
	pProjectilePhysEntity->GetStatus(&statusPos);
	pTestProjectile->Destroy();
	//gEnv->pEntitySystem->RemoveEntity(pTestProjectile->GetEntity()->GetId(), true);

	predictedPosOut = statusPos.pos;

	return true;
}


//------------------------------------------------------------------------
const AIWeaponDescriptor& CWeapon::GetAIWeaponDescriptor( ) const
{
	return m_weaponsharedparams->aiWeaponDescriptor;
}


//------------------------------------------------------------------------
void CWeapon::OnDestroyed()
{ 
  BaseClass::OnDestroyed();

  if (m_fm)
  {
    if (m_fm->IsFiring())
      m_fm->StopFire();
  }
}

bool CWeapon::HasAttachmentAtHelper(const char *helper)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(m_pGameFramework->GetClientActor());
	if(pPlayer)
	{
		IInventory *pInventory = pPlayer->GetInventory();
		if(pInventory)
		{
			for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); ++it)
			{
				const SAccessoryParams *params = GetAccessoryParams(it->first.c_str());
				if (params && !strcmp(params->attach_helper.c_str(), helper))
				{	
					// found a child item that can be used
					return true;
				}
			}
			for (int i = 0; i < pInventory->GetAccessoryCount(); i++)
			{
				const char* name = pInventory->GetAccessory(i);

				if (name)
				{
					const SAccessoryParams *invAccessory = GetAccessoryParams(name);
					if (invAccessory && !strcmp(invAccessory->attach_helper.c_str(), helper))
					{
						// found an accessory in the inventory that can be used
						return true;
					}
				}
			}

		}
	}
	return false;
}

//--------------------------------------------------------------------------
void CWeapon::GetAttachmentsAtHelper(const char *helper, CCryFixedStringListT<5, 30> &attachments)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(m_pGameFramework->GetClientActor());
	if(pPlayer)
	{
		IInventory *pInventory = pPlayer->GetInventory();
		if(pInventory)
		{
			attachments.Clear();
			for (int i = 0; i < pInventory->GetAccessoryCount(); i++)
			{
				const char* name = pInventory->GetAccessory(i);

				if (name)
				{
					const SAccessoryParams *invAccessory = GetAccessoryParams(name);
					if (invAccessory && !strcmp(invAccessory->attach_helper.c_str(), helper))
					{
						attachments.Add(name);
					}
				}
			}
		}
	}
}

//----------------------------------------------

struct CWeapon::PlayLeverLayer
{
	PlayLeverLayer(CWeapon *_weapon, bool _activate): weapon(_weapon), activate(_activate){};
	CWeapon *weapon;
	bool activate;

	void execute(CItem *_this)
	{
		if(activate)
		{
			weapon->StopLayer(g_pItemStrings->lever_layer_2);
			weapon->PlayLayer(g_pItemStrings->lever_layer_1);
		}
		else
		{
			weapon->StopLayer(g_pItemStrings->lever_layer_1);
			weapon->PlayLayer(g_pItemStrings->lever_layer_2);
		}
	}
};

void CWeapon::StartChangeFireMode()
{
	//Check if the weapon has enough firemodes (Melee is always there)
	if(m_fmIds.size() <= 2)
	{
		return;
	}

	int zoomed = 0;

	if(m_zm)
		zoomed = m_zm->GetCurrentStep();

	//Deactivate target display if needed
	if(m_fm && !m_fm->AllowZoom() && IsTargetOn())
		m_fm->Cancel();

	StopFire();
	SetBusy(true);
	m_switchingFireMode = true;

	if(zoomed)
	{
		//Only sound
		PlayAction(g_pItemStrings->change_firemode_zoomed, 0, false);

		//TODO: (SOCOM has anim)... if we get animations for all the weapons remove this...
		if(SupportsDualWield(GetEntity()->GetClass()->GetName()))
		{
			GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_Owner)>>1, CSchedulerAction<PlayLeverLayer>::Create(PlayLeverLayer(this, m_switchLeverLayers)), false);
			//GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<EndChangeFireModeAction>::Create(EndChangeFireModeAction(this)), false);
		}
		else
		{
			GetScheduler()->TimerAction(0, CSchedulerAction<PlayLeverLayer>::Create(PlayLeverLayer(this, m_switchLeverLayers)), false);
			//GetScheduler()->TimerAction(350, CSchedulerAction<EndChangeFireModeAction>::Create(EndChangeFireModeAction(this)), false);
		}
		EndChangeFireMode();
		return;
	}

	//If NANO speed selected...
	float speedOverride = -1.0f;
	//if(GetOwnerActor())
	//{
		//speedOverride = GetOwnerActor()->GetActorSuitGameParameters().actionSpeed_scale;
	//}

	IFireMode* newFiremode = GetFireMode(GetNextFireMode(GetCurrentFireMode()));
	ItemString actionName;

	if(newFiremode)
	{
		actionName = m_fm->GetChangeFiremodeAction(newFiremode->GetName());
	}
	
	if (speedOverride > 0.0f)
		PlayAction((!actionName.empty()) ? actionName : g_pItemStrings->change_firemode, 0, false, CItem::eIPAF_Default, speedOverride);
	else
		PlayAction((!actionName.empty()) ? actionName : g_pItemStrings->change_firemode, 0, false);

	GetScheduler()->TimerAction(0, CSchedulerAction<PlayLeverLayer>::Create(PlayLeverLayer(this, m_switchLeverLayers)), false);
	//GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<EndChangeFireModeAction>::Create(EndChangeFireModeAction(this)), false);

	//Check left socom
	IItem *pItemSlave = GetDualWieldSlave();
	CWeapon *pWeaponSlave = (pItemSlave != NULL) ? static_cast<CWeapon*>(pItemSlave->GetIWeapon()): NULL;
	if(pWeaponSlave)
	{
		pWeaponSlave->StartChangeFireMode();
	}

	EndChangeFireMode();
}

void CWeapon::EndChangeFireMode()
{
	//Real change is here
	ChangeFireMode();

	m_switchLeverLayers = !m_switchLeverLayers;	
	
	SetBusy(false);
	ForcePendingActions();

	m_switchingFireMode = false;
}

//-----------------------------------------------------------------
EntityId CWeapon::GetLaserAttachment() const
{
	for (TAccessoryMap::const_iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
	{
		if (it->first == g_pItemStrings->LAM || it->first == g_pItemStrings->LAMRifle)
			return it->second;
	}

	return 0;
}

//-----------------------------------------------------------------
bool CWeapon::IsLaserAttached() const
{
	return GetLaserAttachment() != 0;
}

//------------------------------------------------------------
EntityId CWeapon::GetFlashlightAttachment() const
{
	for (TAccessoryMap::const_iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
	{
		if (it->first == g_pItemStrings->LAMFlashLight || it->first == g_pItemStrings->LAMRifleFlashLight)
			return it->second;
	}

	return 0;
}

//-----------------------------------------------------------------
bool CWeapon::IsFlashlightAttached() const
{
	return GetFlashlightAttachment() != 0;
}

//-----------------------------------------------------------------
void CWeapon::ActivateLaser(bool activate)
{
	EntityId laserID = GetLaserAttachment();

	//Only if Laser is attached
	if (laserID != 0)
	{
		CLaser* pLaser = static_cast<CLaser*>(m_pItemSystem->GetItem(laserID));
		if (pLaser)
			pLaser->ActivateLaser(activate);
	}
	else
	{
		GameWarning("No Laser attached!! Laser could not be activated/deactivated");
	}
}

//------------------------------------------------------------------
void CWeapon::ActivateFlashlight(bool activate)
{
	EntityId flashlightID = GetFlashlightAttachment();

	//Only if Flashlight is attached
	if (flashlightID != 0)
	{
		CFlashLight* pLam = static_cast<CFlashLight*>(m_pItemSystem->GetItem(flashlightID));
		if (pLam)
			pLam->ActivateLight(activate);
	}
	else
	{
		GameWarning("No Flashlight attached!! Light could not be activated/deactivated");
	}
}

//------------------------------------------------------------------
bool CWeapon::IsLaserActivated() const
{
	EntityId laserID = GetLaserAttachment();

	//Only if LAM is attached
	if (laserID != 0)
	{
		CLaser* pLaser = static_cast<CLaser*>(m_pItemSystem->GetItem(laserID));
		if (pLaser)
			return pLaser->IsLaserActivated();
	}

	return false;
}

//------------------------------------------------------------------
bool CWeapon::IsFlashlightActivated() const
{
	EntityId flashlightID = GetFlashlightAttachment();

	//Only if LAM is attached
	if (flashlightID != 0)
	{
		CFlashLight* pFlashlight = static_cast<CFlashLight*>(m_pItemSystem->GetItem(flashlightID));
		if (pFlashlight)
			return pFlashlight->IsLightActivated();
	}

	return false;
}

//---------------------------------------------------
void CWeapon::TriggerMeleeReaction()
{
	PlayAction(g_pItemStrings->meleeReaction, 1);
}

//---------------------------------------------------
void CWeapon::RaiseWeapon(bool raise, bool faster /* = false */, bool force)
{
	const int playActionFlags = eIPAF_FirstPerson | eIPAF_ThirdPerson | eIPAF_Owner | eIPAF_OwnerAnimGraph | eIPAF_Sound | eIPAF_Animation;

	if(m_sharedparams->params.raiseable)
	{
		if(raise && !m_weaponRaised)
		{
			m_forcingRaise=true;

			//Play the sound anyways if necessary...
			CActor* pOwnerActor = GetOwnerActor();
			
			if ((pOwnerActor != NULL) && pOwnerActor->IsClient())
			{
				CPlayer *pPlayer = static_cast<CPlayer*>(pOwnerActor);

				SPlayerStats *stats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
				if(stats)
				{
					Vec3 vel = stats->velocity;
					if(vel.z<0.0f)
						vel.z = 0.0f;

					float velLenghtSqr = vel.len2();
					static float lastFXTime = 0.0f;
					if(velLenghtSqr>25.0f && ((gEnv->pTimer->GetCurrTime()-lastFXTime)>0.5f))
					{
						lastFXTime = gEnv->pTimer->GetCurrTime();

						pPlayer->PlaySound(CPlayer::ESound_Hit_Wall,true,"speed",0.6f);
						//FX feedback
						IMovementController *pMC = pPlayer->GetMovementController();
						if(pMC)
						{
							SMovementState state;
							pMC->GetMovementState(state);
							IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect("collisions.footsteps.dirt");
							if (pEffect)
							{
								Matrix34 tm = IParticleEffect::ParticleLoc(state.eyePosition + state.eyeDirection*0.5f);
								pEffect->Spawn(true,tm);
							}	
						}		
					}
				}

				//Weapon zoomed, reloading, ...
				if(!force && (IsBusy() || IsZoomingInOrOut() || IsZoomed() || IsModifying() || (m_fm && (m_fm->IsFiring()||m_fm->IsReloading()))))
					return;
			}

			if(m_fm && !m_fm->AllowZoom())
				m_fm->Cancel();

			StopFire();

			SetWeaponRaised(true);

			PlayAction(g_pItemStrings->raise, 0, false, playActionFlags | eIPAF_RepeatLastFrame);

			SetDefaultIdleAnimation(eIGS_Owner,g_pItemStrings->idle_raised);

			RequestWeaponRaised(true);

			m_weaponRaised = true;
		}
		else if(!raise && m_weaponRaised)
		{
			m_forcingRaise=false;

			float animationSpeed = 1.0f;
			if (faster)
				animationSpeed = 2.5f;
			CRY_FIXME(21, 10, 2009, "filipe - lower animation should not have eIPAF_RepeatLastFrame but it does not work properly without that flag")
			PlayAction(g_pItemStrings->lower, 0, false, playActionFlags | eIPAF_RepeatLastFrame, animationSpeed);

			SetDefaultIdleAnimation(eIGS_Owner, GetParams().idle);			

			SetWeaponRaised(false);

			RequestWeaponRaised(false);

			ForcePendingActions();

		}
	}
}

//-----------------------------------------------------
void CWeapon::StartUse(EntityId userId)
{
	BaseClass::StartUse(userId);

	SendMusicLogicEvent(eMUSICLOGICEVENT_WEAPON_MOUNT);
}

//-----------------------------------------------------
void CWeapon::StopUse(EntityId userId)
{
	SendMusicLogicEvent(eMUSICLOGICEVENT_WEAPON_UNMOUNT);

	BaseClass::StopUse(userId);

	if(m_stats.mounted)
	{
		if(IsZoomed() || IsZoomingInOrOut())
			ExitZoom();

		StopFire(); //Stop firing just in case
	}
}

//------------------------------------------------------
bool CWeapon::CheckAmmoRestrictions(EntityId pickerId)
{
	IActor* pPicker = m_pGameFramework->GetIActorSystem()->GetActor(pickerId);
	if(pPicker)
	{
		IInventory *pInventory = pPicker->GetInventory();
		if(pInventory)
		{
			if (HasCompatibleAmmo(pInventory))
			{
				//Check for accessories that give ammo
				if (CheckAmmoRestrictionsForAccessories(pickerId))
				{
					return true;
				}

				return CheckAmmoRestrictionsForBonusAndMagazineAmmo(*pInventory);
			}
		}		
	}

	return false;
}

//-------------------------------------------------------------
int CWeapon::GetMaxZoomSteps()
{
	if(m_zm)
		return m_zm->GetMaxZoomSteps();

	return 0;
}

//-------------------------------------------------------------
bool CWeapon::FireSlave(EntityId actorId, bool fire)
{
	CWeapon *dualWield = NULL;

	IItem *slave = GetDualWieldSlave();

	if (slave && slave->GetIWeapon())
		dualWield = static_cast<CWeapon *>(slave);

	if(!dualWield)
		return false;

	if(!fire)
	{
		dualWield->StopFire();
		return true;
	}

	m_fire_alternation = !m_fire_alternation;

	if (!m_fire_alternation && dualWield->CanFire())
	{
		dualWield->StartFire();
		return true;
	}

	return false;
}

//--------------------------------------------------------
void CWeapon::ReloadSlave()
{
	CWeapon *dualWield = NULL;

	IItem *slave = GetDualWieldSlave();

	if (slave && slave->GetIWeapon())
		dualWield = static_cast<CWeapon *>(slave);

	if(!dualWield)
		return;

	dualWield->Reload();
}

//----------------------------------------------------------
void CWeapon::SendMusicLogicEvent(EMusicLogicEvents event)
{
	CActor* pOwner = GetOwnerActor();
	if(pOwner && pOwner->IsClient() && pOwner->GetHealth()>0)
	{
		m_pGameFramework->GetMusicLogic()->SetEvent(event);
	}
}

//----------------------------------------------------------
void CWeapon::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));
	GetInternalMemoryUsage(s);
}

void CWeapon::GetInternalMemoryUsage(ICrySizer * s) const	
{
	{
		SIZER_COMPONENT_NAME(s, "FireModes");
		s->AddObject(m_fmIds);
		s->AddObject(m_firemodes);						
	}
	{
		SIZER_COMPONENT_NAME(s, "ZoomModes");
		s->AddObject(m_zmIds);
		s->AddObject(m_zoommodes);		
	}

	{
		SIZER_COMPONENT_NAME(s, "Ammo");
		s->AddObject(m_ammo);
		s->AddObject(m_bonusammo);
	}

	s->AddObject(m_listeners);
	CItem::GetInternalMemoryUsage(s); // collect memory of parent class
}

//------------------------------------------------------
bool CWeapon::AIUseEyeOffset() const
{
	return m_weaponsharedparams->aiWeaponOffsets.useEyeOffset;
}

//--------------------------------------------------------
bool CWeapon::AIUseOverrideOffset(EStance stance, float lean, float peekOver, Vec3& offset) const
{
	// do checks for if(found) here
	if(m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffsetLeanLeft.empty() || 
		m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffsetLeanRight.empty() || m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffset.empty())
		return false;

	TStanceWeaponOffset::const_iterator itrOffsetLeft(m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffsetLeanLeft.find(stance));
	if(itrOffsetLeft == m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffsetLeanLeft.end())
		return false;

	TStanceWeaponOffset::const_iterator itrOffsetRight(m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffsetLeanRight.find(stance));
	if(itrOffsetRight == m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffsetLeanRight.end())
		return false;

	TStanceWeaponOffset::const_iterator itrOffset(m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffset.find(stance));
	if(itrOffset == m_weaponsharedparams->aiWeaponOffsets.stanceWeponOffset.end())
		return false;

	const Vec3& normal(itrOffset->second);
	const Vec3& lLeft(itrOffsetLeft->second);
	const Vec3& lRightt(itrOffsetRight->second);

	offset = SStanceInfo::GetOffsetWithLean(lean, peekOver, normal, lLeft, lRightt, Vec3(ZERO) );

	return true;
}

//----------------------------------------------------------
namespace
{
	bool IsFriendlyEntity(IEntity* pEntity, IActor* pAI, IActor* pPlayer)
	{
		//Only for actors (not vehicles)
		if(pAI && pEntity && pEntity->GetAI())
		{
			if(!pEntity->GetAI()->IsHostile(pPlayer->GetEntity()->GetAI(),false))
				return true;
			else
				return false;
		}


		//Special case (Animated objects), check for script table value "bFriendly"
		//Check script table (maybe is not possible to grab)
		if(pEntity)
		{
			SmartScriptTable props;
			IScriptTable* pScriptTable = pEntity->GetScriptTable();
			if(!pScriptTable || !pScriptTable->GetValue("Properties", props))
				return false;

			int isFriendly = 0;
			if(props->GetValue("bNoFriendlyFire", isFriendly) && isFriendly!=0)
				return true;
		}

		//for vehicles
		if( pEntity && pEntity->GetId() )
		{
			IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
			if ( pVehicle )
			{
				if ( pPlayer->GetEntity() && pVehicle->HasFriendlyPassenger( pPlayer->GetEntity() ) )
					return true;
			}				
		}

		return false;
	}
}

//----------------------------------------------------------
bool CWeapon::FilterView(SViewParams &viewParams)
{
	bool ret = BaseClass::FilterView(viewParams);

	if(m_zm && m_zm->IsZoomed())
	{
		m_zm->FilterView(viewParams);
	}

	return ret;
}

//--------------------------------------------------
void CWeapon::PostFilterView(struct SViewParams &viewParams)
{
	BaseClass::PostFilterView(viewParams);

	if(m_zm && m_zm->IsZoomed())
		m_zm->PostFilterView(viewParams);
}

//-------------------------------------------------
void CWeapon::OnZoomIn()
{
	bool hasSniperScope = false;;
	for(TAccessoryMap::const_iterator it = m_accessories.begin();it!=m_accessories.end();++it)
	{
		CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(it->second));
		if(pItem && pItem->GetParams().scopeAttachment)
		{
			if(const SAccessoryParams *params = GetAccessoryParams(it->first))
			{
				pItem->DrawSlot(eIGS_FirstPerson,false);
				ResetCharacterAttachment(eIGS_FirstPerson, params->attach_helper.c_str(), params->attachToOwner);
				pItem->DrawSlot(eIGS_Aux1,false);
				SetCharacterAttachment(eIGS_FirstPerson, params->attach_helper, pItem->GetEntity(), eIGS_Aux1, params->attachToOwner);
				hasSniperScope = true;
			}	
		}
	}

	if(!hasSniperScope)
	{
		Hide(true);
	}
}

//-------------------------------------------------
void CWeapon::OnZoomOut()
{
	bool hasSniperScope = false;;
	for(TAccessoryMap::const_iterator it = m_accessories.begin();it!=m_accessories.end();++it)
	{
		CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(it->second));
		if(pItem && pItem->GetParams().scopeAttachment)
		{
			if(const SAccessoryParams *params = GetAccessoryParams(it->first))
			{
				pItem->DrawSlot(eIGS_Aux1,false);
				ResetCharacterAttachment(eIGS_FirstPerson, params->attach_helper.c_str(), params->attachToOwner);
				pItem->DrawSlot(eIGS_FirstPerson,false);
				SetCharacterAttachment(eIGS_FirstPerson, params->attach_helper, pItem->GetEntity(), eIGS_FirstPerson, params->attachToOwner);
				hasSniperScope = true;
			}	

		}
	}

	if(!hasSniperScope)
	{
		Hide(false);
	}
}
//-------------------------------------------------
void CWeapon::OnZoomedIn()
{
	m_zoomedTime = gEnv->pTimer->GetAsyncCurTime();
}

//-------------------------------------------------
void CWeapon::OnZoomedOut()
{
	m_zoomedTime = 0.0f;
	ForcePendingActions();
}

//-------------------------------------------------
bool CWeapon::GetScopePosition(Vec3& pos)
{
	for(TAccessoryMap::const_iterator it = m_accessories.begin();it!=m_accessories.end();++it)
	{
		CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(it->second));
		if(pItem && pItem->GetParams().scopeAttachment)
		{
			if(const SAccessoryParams *params = GetAccessoryParams(it->first))
			{
				pos = GetSlotHelperPos(eIGS_FirstPerson,params->attach_helper.c_str(),true);
				Matrix33 rot = GetSlotHelperRotation(eIGS_FirstPerson,params->attach_helper.c_str(),true);
				Vec3 dirZ = rot.GetColumn1();
				if(pItem->GetParams().scopeAttachment==1)
				{
					const float sniperZOfffset = 0.029f;
					pos += (sniperZOfffset*dirZ);
				}
				else if(pItem->GetParams().scopeAttachment==2)
				{
					const float lawZOffset = -0.028f;
					const float lawXOffset = -0.017f;
					pos += (lawZOffset*dirZ);
					Vec3 dirX = rot.GetColumn2();
					pos += (lawXOffset*dirX);
				}

				return true;
			}	
		}
	}
	return false;
}

//------------------------------------------------------
void CWeapon::SetNextShotTime(bool activate)
{
	if(activate)
	{
		// MUST BE CALLED from Select(true), after firemode activation
		// Prevent exploit fire rate by switching weapons
		if(m_fm && m_nextShotTime > 0.0f)
		{
			CTimeValue time = gEnv->pTimer->GetFrameStartTime();
			float dt = m_nextShotTime - time.GetSeconds();
			if(dt > 0.0f)
				m_fm->SetNextShotTime(dt);
			m_nextShotTime = 0.0f;
		}
	}
	else
	{
		// MUST BE CALLED from Select(false), before firemode deactivation
		// save game time when the weapon can next be fired
		m_nextShotTime = 0.0f;
		if(m_fm)
		{
			float delay = m_fm->GetNextShotTime();
			if(delay > 0.0f)
			{
				CTimeValue time = gEnv->pTimer->GetFrameStartTime();
				m_nextShotTime = time.GetSeconds() + delay;
			}
		}
	}
}

//-----------------------------------------------------
bool CWeapon::CanDrop() const
{
	return (BaseClass::CanDrop() && !IsModifying());
}

//--------------------------------------------------------
bool CWeapon::CanBeRaised() 
{ 
	return m_sharedparams->params.raiseable; 
}

//------------------------------------------------------
void CWeapon::CacheRaisePose()
{
	const char* name = m_sharedparams->params.pose.c_str();
	m_raisePose = (uint8)eWeaponRaisedPose_None;

	if (strcmp(name, "nw") == 0)
		m_raisePose = (uint8)eWeaponRaisedPose_Fists;
	else if (strcmp(name, "pistol") == 0)
		m_raisePose = (uint8)eWeaponRaisedPose_Pistol;
	else if (strcmp(name, "rifle") == 0)
		m_raisePose = (uint8)eWeaponRaisedPose_Rifle;
	else if (strcmp(name, "rocket") == 0)
		m_raisePose = (uint8)eWeaponRaisedPose_Rocket;
	else if (strcmp(name, "mg") == 0)
		m_raisePose = (uint8)eWeaponRaisedPose_MG;

}

//--------------------------------------------------------
bool CWeapon::Query(EWeaponQuery query, const void* param /*=NULL*/)
{
	switch(query)
	{
		case eWQ_Has_Accessory_Flashlight:
			{
				return IsFlashlightAttached();
			}
			break;

		case eWQ_Has_Accessory_Laser:
			{
				return IsLaserAttached();
			}
			break;

		case eWQ_Is_Flashlight_Activated:
			{
				return IsFlashlightActivated();
			}
			break;
		
		case eWQ_Is_Laser_Activated:
			{
				return IsLaserActivated();
			}
			break;

		case eWQ_Activate_Flashlight:
			{
				if(!param)
					return false;
				ActivateFlashlight(*((bool*)param));
				CWeapon* pSlaveWeapon = GetDualWieldSlave()? static_cast<CWeapon*>(GetDualWieldSlave()->GetIWeapon()): NULL;
				if(pSlaveWeapon)
					pSlaveWeapon->ActivateFlashlight(*((bool*)param));
			}
			break;

		case eWQ_Activate_Laser:
			{
				if(!param)
					return false;
				ActivateLaser(*((bool*)param));
				CWeapon* pSlaveWeapon = GetDualWieldSlave()? static_cast<CWeapon*>(GetDualWieldSlave()->GetIWeapon()): NULL;
				if(pSlaveWeapon)
					pSlaveWeapon->ActivateLaser(*((bool*)param));
			}
			break;

		// Crysis2 AI should not raise weapon (can add this later once we implement a proper way to select additives for specific stances)
		//case eWQ_Raise_Weapon:
		//	{
		//
		//		if(!param)
		//			return false;
		//
		//		RaiseWeapon(*((bool*)param));
		//		CWeapon* pSlaveWeapon = GetDualWieldSlave()? static_cast<CWeapon*>(GetDualWieldSlave()->GetIWeapon()): NULL;
		//		if(pSlaveWeapon)
		//			pSlaveWeapon->RaiseWeapon(*((bool*)param));
		//	}
		//	break;

		default:
				return false;
			
	}

	return true;
}

//--------------------------------------------------------
void CWeapon::ApplyFPViewRecoil(int nFrameId, float recoilX, float recoilY)
{
	if(m_lastRecoilUpdate == nFrameId)
		return;

	m_lastRecoilUpdate = nFrameId;

	if(	CActor *pOwner = GetOwnerActor() )
	{
		pOwner->AddViewAngleOffsetForFrame(Ang3(recoilY, 0.0f, recoilX));
	}
}

//--------------------------------------------------------
bool CWeapon::IsAttachmentEquipped(const char *attachmentName) const
{
	if(!attachmentName || m_accessories.empty())
		return false;

	TAccessoryMap::const_iterator cit = m_accessories.begin();
	TAccessoryMap::const_iterator end = m_accessories.end();

	while(cit != end)
	{
		if(!strcmp(attachmentName, cit->first.c_str()))
			return true;

		++cit;
	}

	return false;
}


//--------------------------------------------------------
float CWeapon::GetMovementModifier() const
{
	bool firing = m_fm ? m_fm->IsFiring() : false;

	if (!firing)
		return GetPlayerMovementModifiers().movementSpeedScale;
	else
		return GetPlayerMovementModifiers().firingMovementSpeedScale;
}

//--------------------------------------------------------
float CWeapon::GetRotationModifier() const
{
	bool firing = m_fm ? m_fm->IsFiring() : false;

	if (!firing)
		return GetPlayerMovementModifiers().rotationSpeedScale;
	else
		return GetPlayerMovementModifiers().firingRotationSpeedScale;
}

//--------------------------------------------------------
float CWeapon::GetCoverAndLeanModifier() const
{
	return GetPlayerMovementModifiers().coverLeanRotationSpeedScale;
}

//--------------------------------------------------------
const SPlayerMovementModifiers& CWeapon::GetPlayerMovementModifiers() const
{
	if (IsZoomed())
	{
		assert((m_zmId >= 0) && (m_zmId < m_weaponsharedparams->zoomedMovementModifiers.size()));

		return m_weaponsharedparams->zoomedMovementModifiers[m_zmId];
	}
	else
		return m_weaponsharedparams->defaultMovementModifiers;
}

//--------------------------------------------------------
bool CWeapon::ShouldSendOnShootHUDEvent() const
{
	return !m_fm->IsSilenced();
}

bool CWeapon::UpdateAimAnims(SParams_WeaponFPAiming &aimAnimParams, bool &releaseCameraBone)
{
	releaseCameraBone = m_releaseCameraBone;

	if(m_sharedparams->params.hasAimAnims)
	{
		const SCachedItemAnimation check = GenerateAnimCacheID(m_sharedparams->params.aimAnims.anim[0].c_str());
		uint32 changeFlags = aimAnimParams.UpdateStatus(check);
		aimAnimParams.transitionTime = -1.0f;
		aimAnimParams.overlayFactor = (float)__fsel(m_fpOverlayOverrideEndTime - gEnv->pTimer->GetCurrTime(), m_fpOverlayOverride, 1.0f);

		if(IsZoomed() || IsZoomingIn())
		{
			aimAnimParams.overlayFactor *= m_sharedparams->params.ironsightAimAnimFactor;
		}

		if (changeFlags != 0)
		{
			bool suffixChangeMostSignificant = (changeFlags & (1<<eCF_Suffix)) && !(changeFlags & (1<<eCF_String));
			if (suffixChangeMostSignificant)
			{
				aimAnimParams.transitionTime = m_actionSuffixBlendTime;
			}

			TempResourceName name;
			for (int i=0; i<WeaponAimAnim::Total; i++)
			{
				int animationId = FindCachedAnimationId(m_sharedparams->params.aimAnims.anim[i], aimAnimParams.characterInst);
				aimAnimParams.SetAnimation(i, animationId);
			}
		}

		return true;
	}

	return false;
}

void CWeapon::ResetActionSuffix(float blendTime)
{
	BaseClass::ResetActionSuffix(blendTime);

	if (m_fm)
	{
		if (m_fm->GetSuffix())
		{
			m_actionSuffix	 = m_fm->GetSuffix();
		}
		if (m_fm->GetSuffixAG())
		{
			m_actionSuffixAG = m_fm->GetSuffixAG();
		}
	}
	m_actionSuffixBlendTime = blendTime;
}

// returns a non-localised name for development purposes
const char *CWeapon::GetName()
{
	return GetEntity()->GetClass()->GetName();
}

bool CWeapon::IsOwnerSuperJumpingOrUsingAirFriction() const
{
	CActor* pOwner = GetOwnerActor();
	if ((pOwner != NULL) && pOwner->IsPlayer())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pOwner);
		return (pPlayer->IsSuperJumping() || pPlayer->IsAirFrictionActive());
	}

	return false;
}

void CWeapon::OnPlayerSuperJumped( bool superJumped )
{
	if (superJumped)
	{
		if (!gEnv->bMultiplayer && (IsZoomed() || IsZoomingIn()))
		{
			StopZoom(GetOwnerId());
		}

		if (IsReloading())
		{
			m_fm->CancelReload();
			ResetOwnerAnimationsInLayer(ITEM_OWNER_ACTION_LAYER);
		}
	}
	else
	{
		ForcePendingActions();
	}
}

void CWeapon::OnPlayerAirFrictionActivate( bool activate )
{
	// Same logic as super jump
	OnPlayerSuperJumped(activate);

	if (activate)
	{
		PlayAction(g_pItemStrings->airFrictionOn, 1, false, eIPAF_Default | eIPAF_RepeatLastFrame);
	}
	else
	{
		ResetOwnerAnimationsInLayer(ITEM_OWNER_ACTION_LAYER + 1); //Friction on layer
	}
}

bool CWeapon::IsOwnerClient() const
{
	CActor* pOwner = GetOwnerActor();
	return pOwner? pOwner->IsClient() : m_isClientOwnerOverride;
}

void CWeapon::SetOwnerClientOverride(bool isClient)
{
	m_isClientOwnerOverride = isClient;
}



void CWeapon::OnFireWhenOutOfAmmo()
{
	if (IsReloading())
		return;

	if (IsRippedOff())
		return;

	if (CanReload())
		Reload();
	else if (CanDeselect())
		AutoSelectNextItem();
}



void CWeapon::AutoSelectNextItem()
{
	ClearInputFlags();

	CActor *pOwner = GetOwnerActor();
	if (pOwner && IsOwnerClient() && !IsMounted())
	{
		IInventory *pInventory = pOwner->GetInventory();
		IItemSystem *pItemSystem = m_pGameFramework->GetIItemSystem();

		EntityId primaryWeaponId = 0;
		EntityId secondaryWeaponId = 0;

		int numItems = pInventory->GetCount();
		for (int i = 0; i < numItems; ++ i)
		{
			EntityId itemId = pInventory->GetItem(i);
			IItem *pItem = pItemSystem->GetItem(itemId);
			if (pItem)
			{
				CWeapon *pWeapon = static_cast<CWeapon*>(pItem->GetIWeapon());
				if (pWeapon && !pWeapon->OutOfAmmo(false))
				{
					// Weapon is a candidate for switching to
					const char* const pWeaponCategory = m_pItemSystem->GetItemCategory(pWeapon->GetEntity()->GetClass()->GetName());
					if (!stricmp(pWeaponCategory, "primary"))
					{
						primaryWeaponId = pWeapon->GetEntityId();
						break;
					}
					else if (!secondaryWeaponId && !stricmp(pWeaponCategory, "secondary"))
					{
						secondaryWeaponId = pWeapon->GetEntityId();
					}
				}
			}
		}

		EntityId itemToSelect = primaryWeaponId ? primaryWeaponId : secondaryWeaponId;
		if (itemToSelect)
			pOwner->ScheduleItemSwitch(itemToSelect, true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CWeapon::PickUpAmmo( EntityId pickerId )
{
	IActor* pPickerActor = m_pGameFramework->GetIActorSystem()->GetActor(pickerId);
	IInventory* pInventory = pPickerActor ? pPickerActor->GetInventory() : NULL;

	CRY_ASSERT(pPickerActor);
	CRY_ASSERT(pInventory);

	if (!pInventory)
		return;

	int pickedUpAmmo = 0;

		const int bonusAmmoTypeCount = m_bonusammo.size();
		int bonusAmmoTypeEmptied = 0;

		TAmmoVector::iterator bonusAmmoEndIt = m_bonusammo.end();
		for (TAmmoVector::iterator bonusAmmoIt = m_bonusammo.begin(); bonusAmmoIt != bonusAmmoEndIt; ++bonusAmmoIt)
		{
			IEntityClass* pAmmoClass = bonusAmmoIt->pAmmoClass;
			const int bonusAmmoCount = bonusAmmoIt->count;
			const int currentAmmoCount = pInventory->GetAmmoCount(pAmmoClass);
			const int maxAmmoCapacity = pInventory->GetAmmoCapacity(pAmmoClass);
			
			if ((bonusAmmoCount + currentAmmoCount) >= maxAmmoCapacity)
			{
				const int remaningBonusAmmo = bonusAmmoCount - (maxAmmoCapacity - currentAmmoCount);
				bonusAmmoIt->count = remaningBonusAmmo;
			pickedUpAmmo += (maxAmmoCapacity - currentAmmoCount);
				SetInventoryAmmoCountInternal(pInventory, pAmmoClass, maxAmmoCapacity);
			}
			else
			{
				bonusAmmoTypeEmptied++;
				bonusAmmoIt->count = 0;
			pickedUpAmmo += (bonusAmmoCount);
				SetInventoryAmmoCountInternal(pInventory, pAmmoClass, currentAmmoCount + bonusAmmoCount);
			}
		}

	// Pick magazine one, only if I have this weapon (not only compatible)
	bool pickerHasThisWeapon = pInventory->GetCountOfClass(GetEntity()->GetClass()->GetName()) != 0;
	if (pickerHasThisWeapon)
		{
		TAmmoVector::iterator ammoEndCit = m_ammo.end();
		for (TAmmoVector::iterator ammoCit = m_ammo.begin(); ammoCit != ammoEndCit; ++ammoCit)
		{
			IEntityClass* pAmmoClass = ammoCit->pAmmoClass;
			const int magazineAmmoCount = ammoCit->count;
			const int currentAmmoCount = pInventory->GetAmmoCount(pAmmoClass);
			const int maxAmmoCapacity = pInventory->GetAmmoCapacity(pAmmoClass);

			if ((magazineAmmoCount + currentAmmoCount) >= maxAmmoCapacity)
			{
				const int remaningMagazineAmmo = magazineAmmoCount - (maxAmmoCapacity - currentAmmoCount);
				ammoCit->count = remaningMagazineAmmo;
				pickedUpAmmo += (maxAmmoCapacity - currentAmmoCount);
				SetInventoryAmmoCountInternal(pInventory, pAmmoClass, maxAmmoCapacity);
			}
			else
				{
				ammoCit->count = 0;
				pickedUpAmmo += magazineAmmoCount;
				SetInventoryAmmoCountInternal(pInventory, pAmmoClass, currentAmmoCount + magazineAmmoCount);
				}
			}
		}

	if (IsServer())
		{
			pPickerActor->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClPickUp(), CActor::PickItemParams(GetEntityId(), m_stats.selected, true, true), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
		}

		PlayAction(g_pItemStrings->pickedup_ammo);

	// Finally check if we run out of bonus and/or magazine ammo
	bool clearBonusAmmo = (bonusAmmoTypeCount > 0) && (bonusAmmoTypeEmptied == bonusAmmoTypeCount);
	if (clearBonusAmmo)
	{
		m_bonusammo.clear();
	}

	//In SP, if we have this weapon, as soon as we get any ammo from it, it's deleted
	if (!gEnv->bMultiplayer)
	{
		if ((pickerHasThisWeapon) &&(pickedUpAmmo > 0))
		{
			RemoveEntity();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CWeapon::HasSomeAmmoToPickUp(EntityId pickerId) const
{
	IActor* pPickerActor = m_pGameFramework->GetIActorSystem()->GetActor(pickerId);
	IInventory* pInventory = pPickerActor ? pPickerActor->GetInventory() : NULL;

	CRY_ASSERT(pPickerActor);
	CRY_ASSERT(pInventory);

	if (pInventory)
	{
		bool anyBonusAmmoLeft = !m_bonusammo.empty();

		if (anyBonusAmmoLeft)
		{
			return true;
		}

		bool pickerHasThisWeapon = pInventory->GetCountOfClass(GetEntity()->GetClass()->GetName()) != 0;
		if (pickerHasThisWeapon)
		{
			TAmmoVector::const_iterator ammoEndCit = m_ammo.end();
			for (TAmmoVector::const_iterator ammoCit = m_ammo.begin(); ammoCit != ammoEndCit; ++ammoCit)
			{
				if (ammoCit->count > 0)
					return true;
			}
		}
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////
ColorF CWeapon::GetSilhouetteColor() const
{
	return HasSomeAmmoToPickUp(m_pGameFramework->GetClientActorId()) ? ColorF(0.6015625f, 0.83203125f, 0.71484375f, 1.0f) : ColorF(0.89411f, 0.10588f, 0.10588f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CWeapon::RegisterUsedAmmoWithInventory( IInventory* pInventory )
{
	if (pInventory && (m_isRegisteredAmmoWithInventory == false))
	{
		TAmmoVector::const_iterator ammoMapEndCit = m_ammo.end();
		for (TAmmoVector::const_iterator ammoCit = m_ammo.begin(); ammoCit != ammoMapEndCit; ++ammoCit)
		{
			pInventory->AddAmmoUser(ammoCit->pAmmoClass);
		}

		m_isRegisteredAmmoWithInventory = true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CWeapon::UnregisterUsedAmmoWithInventory( IInventory* pInventory )
{
	if (pInventory && (m_isRegisteredAmmoWithInventory == true))
	{
		TAmmoVector::const_iterator ammoMapEndCit = m_ammo.end();
		for (TAmmoVector::const_iterator ammoCit = m_ammo.begin(); ammoCit != ammoMapEndCit; ++ammoCit)
		{
			pInventory->RemoveAmmoUser(ammoCit->pAmmoClass);
		}

		m_isRegisteredAmmoWithInventory = false;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CWeapon::HasCompatibleAmmo( IInventory* pInventory ) const
{
	if (pInventory)
	{
		TAmmoVector::const_iterator ammoMapEndCit = m_ammo.end();
		for (TAmmoVector::const_iterator ammoCit = m_ammo.begin(); ammoCit != ammoMapEndCit; ++ammoCit)
		{
			// Some weapon in the inventory uses a compatible ammo type, so it can be picked up
			if (pInventory->GetNumberOfUsersForAmmo(ammoCit->pAmmoClass) > 0)
			{
				return true;
			}
		}	
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CWeapon::CheckAmmoRestrictionsForAccessories(EntityId pickerId) const
{
	if(!m_accessories.empty())
	{
		TAccessoryMap::const_iterator accessoryEndCit = m_accessories.end();
		for (TAccessoryMap::const_iterator accessoryCit = m_accessories.begin(); accessoryCit != accessoryEndCit; ++accessoryCit)
		{
			if(CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(accessoryCit->second)))
			{
				if(pItem->GivesAmmo() && pItem->CheckAmmoRestrictions(pickerId))
				{
					return true;
				}
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CWeapon::CheckAmmoRestrictionsForBonusAndMagazineAmmo( IInventory& inventory ) const
{
	if (!m_bonusammo.empty())
	{
		const TAmmoVector::const_iterator bonusAmmoEndCit = m_bonusammo.end();
		for (TAmmoVector::const_iterator bonusAmmoCit = m_bonusammo.begin(); bonusAmmoCit != bonusAmmoEndCit; ++bonusAmmoCit)
		{
			IEntityClass* pAmmoClass = bonusAmmoCit->pAmmoClass;
			const int invAmmo  = inventory.GetAmmoCount(pAmmoClass);
			const int invLimit = inventory.GetAmmoCapacity(pAmmoClass);

			if (invAmmo >= invLimit)
			{
				return false;
			}
		}
	}

	if(!m_ammo.empty())
	{
		const TAmmoVector& accesoryAmmoMap = m_weaponsharedparams->ammoParams.accessoryAmmo;
		const TAmmoVector::const_iterator ammoEndCit = m_ammo.end();

		for (TAmmoVector::const_iterator ammoCit = m_ammo.begin(); ammoCit != ammoEndCit; ++ammoCit)
		{
			IEntityClass* pAmmoClass = ammoCit->pAmmoClass;
			const int invAmmo  = inventory.GetAmmoCount(pAmmoClass);
			const int invLimit = inventory.GetAmmoCapacity(pAmmoClass);

			if ((invAmmo >= invLimit) && (SWeaponAmmoUtils::FindAmmoConst(accesoryAmmoMap, pAmmoClass) == NULL))
			{
				return false;
			}
		}
	}

	return true;
}

void CWeapon::ShowDebugInfo()
{
#if !defined(_RELEASE)
	
	IActor* pClientActor = m_pGameFramework->GetClientActor();
	IInventory* pInventory = pClientActor ? pClientActor->GetInventory() : NULL;

	if (!pInventory)
	{
		GameWarning("Can not display debug info for item, client actor or his inventory is missing");
		return;
	}

	IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

	SAuxGeomRenderFlags oldFlags = pRenderAux->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone); 
	pRenderAux->SetRenderFlags(newFlags);

	const Vec3 baseText = GetEntity()->GetWorldPos();
	const Vec3 textLineOffset(0.0f, 0.0f, 0.14f);
	const float textColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float lineCounter = 0.0f;

	IEntityClass* pWeaponClass = GetEntity()->GetClass();

	bool pickerHasThisWeapon = pInventory->GetCountOfClass(pWeaponClass->GetName()) != 0;

	gEnv->pRenderer->DrawLabelEx(baseText, 1.25f, textColor, true, false, pickerHasThisWeapon ? "'Weapon (%s) in inventory" : "Weapon (%s) NOT in inventory", pWeaponClass->GetName());
	
	lineCounter += 2.0f;
	gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * lineCounter), 1.25f, textColor, true, false, "---- Bonus Ammo -------" );

	TAmmoVector::iterator bonusAmmoEndIt = m_bonusammo.end();
	for (TAmmoVector::iterator bonusAmmoIt = m_bonusammo.begin(); bonusAmmoIt != bonusAmmoEndIt; ++bonusAmmoIt)
	{
		const SWeaponAmmo& bonusAmmo = *bonusAmmoIt;
		if (bonusAmmo.pAmmoClass)
		{
			lineCounter += 1.0f;
			gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * lineCounter), 1.25f, textColor, true, false, "  Ammo: '%s' - %d", bonusAmmo.pAmmoClass->GetName(), bonusAmmo.count);
		}
	}

	lineCounter += 2.0f;
	gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * lineCounter), 1.25f, textColor, true, false, "---- Magazine Ammo -------" );

	TAmmoVector::iterator ammoEndCit = m_ammo.end();
	for (TAmmoVector::iterator ammoCit = m_ammo.begin(); ammoCit != ammoEndCit; ++ammoCit)
	{
		const SWeaponAmmo& ammo = *ammoCit;
		if (ammo.pAmmoClass)
		{
			lineCounter += 1.0f;
			gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * lineCounter), 1.25f, textColor, true, false, "  Ammo: '%s' - %d", ammo.pAmmoClass->GetName(), ammo.count);
		}

	}

	pRenderAux->SetRenderFlags(oldFlags);

#endif
}
