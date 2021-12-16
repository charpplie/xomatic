/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 27:10:2004   11:26 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Item.h"
#include "ItemSharedParams.h"
#include "Game.h"
#include "GameActions.h"
#include "IGameObject.h"
#include "ISerialize.h"
#include <IEntitySystem.h>

#include "Actor.h"
#include "Player.h"
#include "IActorSystem.h"
#include "IItemSystem.h"
#include "ActionMapManager.h"
#include "ScriptBind_Item.h"
#include "GameRules.h"
#include "GameCVars.h"
#include "Binocular.h"
#include "WeaponAttachmentManager.h"
#include "TacticalManager.h"


#pragma warning(disable: 4355)	// ´this´ used in base member initializer list

#if defined(USER_alexll)
#define ITEM_DEBUG_MEMALLOC
#endif

#ifdef ITEM_DEBUG_MEMALLOC
int gInstanceCount = 0;
#endif


IEntitySystem *CItem::m_pEntitySystem=0;
IItemSystem *CItem::m_pItemSystem=0;
IGameFramework *CItem::m_pGameFramework=0;
IGameplayRecorder*CItem::m_pGameplayRecorder=0;

IEntityClass* CItem::sC4Class = 0;
IEntityClass* CItem::sBinocularsClass = 0;
IEntityClass* CItem::sDebugGunClass = 0;
IEntityClass* CItem::sRefWeaponClass = 0;
IEntityClass* CItem::sDSG1Class = 0;
IEntityClass* CItem::sLTagGrenade = 0;
IEntityClass* CItem::sIncendiaryAmmo = 0;

//------------------------------------------------------------------------
CItem::CItem()
: m_scheduler(this),	// just to store the pointer.
	m_dualWieldMasterId(0),
	m_dualWieldSlaveId(0),
	m_postSerializeMountedOwner(0),
	m_parentId(0),
	m_effectGenId(0),
	m_hostId(0),
	m_fpOverlayOverride(1.0f),
	m_fpOverlayOverrideEndTime(0.0f),
	m_modifying(false),
	m_transitioning(false),
	m_cloaked(false),
	m_serializeCloaked(false),
	m_accessoryAmmoAvailable(true),
	m_noDrop(false),
	m_constraintId(0),
	m_serializeActivePhysics(0),
	m_serializeDestroyed(false),
	m_actionSuffixBlendTime(-1.0f),
	m_releaseCameraBone(false),
	m_userStealthyTime(0.0f),
	m_isUserStealthy(false),
	m_inIdleAction(false),
	m_idleSoundId(INVALID_SOUNDID)
{
#ifdef ITEM_DEBUG_MEMALLOC
	++gInstanceCount;
#endif
	memset(m_animationTime, 0, sizeof(m_animationTime));
}

//------------------------------------------------------------------------
CItem::~CItem()
{
	//AttachArms(false, false);
	AttachToBack(false);

	//Auto-detach from the parent
	if(m_parentId)
	{
		CItem *pParent= static_cast<CItem*>(m_pItemSystem->GetItem(m_parentId));
		if(pParent)
		{
			//When destroyed the item parent should not be busy
			pParent->SetBusy(false);

			for (TAccessoryMap::iterator it=pParent->m_accessories.begin(); it!=pParent->m_accessories.end(); ++it)
			{
				if(GetEntityId()==it->second)
				{
					pParent->AttachAccessory(it->first.c_str(),false,true);
					break;
				}
			}
		}
	}

	GetGameObject()->ReleaseProfileManager(this);

	if (CActor *pActor = GetOwnerActor())
	{
		if(IInventory *pInventory = GetActorInventory(pActor))
			pInventory->RemoveItem(GetEntityId());
	}

	if(!(GetISystem()->IsSerializingFile() && GetGameObject()->IsJustExchanging()))
		for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
			gEnv->pEntitySystem->RemoveEntity(it->second);

	if(m_pItemSystem)
		m_pItemSystem->RemoveItem(GetEntityId(),GetEntity()->GetClass()->GetName());

	 Quiet();

#ifdef ITEM_DEBUG_MEMALLOC
	 --gInstanceCount;
#endif
}

//------------------------------------------------------------------------
bool CItem::Init( IGameObject *pGameObject )
{
	SetGameObject(pGameObject);

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("CItem::Init Instance=%d %p Id=%d Class=%s", gInstanceCount, GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	if (!m_pGameFramework)
	{
		m_pEntitySystem = gEnv->pEntitySystem;
		m_pGameFramework= gEnv->pGame->GetIGameFramework();
		m_pGameplayRecorder = m_pGameFramework->GetIGameplayRecorder();
		m_pItemSystem = m_pGameFramework->GetIItemSystem();
		sC4Class = gEnv->pEntitySystem->GetClassRegistry()->FindClass("C4");
		sBinocularsClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Binoculars");
		sDebugGunClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DebugGun");
		sRefWeaponClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("RefWeapon");
		sDSG1Class = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DSG1");
		sLTagGrenade = gEnv->pEntitySystem->GetClassRegistry()->FindClass("LTagGrenade");
		sIncendiaryAmmo   = gEnv->pEntitySystem->GetClassRegistry()->FindClass("incendiarybullet");
	}

	if (!GetGameObject()->CaptureProfileManager(this))
		return false;

	// bind to network
	if (0 == (GetEntity()->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY)))
	{
		if (!GetGameObject()->BindToNetwork())
		{
			GetGameObject()->ReleaseProfileManager(this);
			return false;
		}
	}

	// register with item system
	m_pItemSystem->AddItem(GetEntityId(), this);
	m_pItemSystem->CacheItemGeometry(GetEntity()->GetClass()->GetName());

	// attach script bind
	g_pGame->GetItemScriptBind()->AttachTo(this);

	if(!ResetParams())
	{
		//failed to find all appropriate shared parameters bailing out
		return false;
	}

	m_noDrop = false;

	if (GetEntity()->GetScriptTable())
	{
		SmartScriptTable props;
		GetEntity()->GetScriptTable()->GetValue("Properties", props);
		ReadProperties(props);
	}

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("CItem::Init End %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	return true;
}

bool CItem::ResetParams()
{
	m_sharedparams = g_pGame->GetGameSharedParametersStorage()->GetItemSharedParameters(GetEntity()->GetClass()->GetName(), false);
		if(!m_sharedparams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Uninitialised item params. Has the xml been setup correctly for item %s?", GetEntity()->GetName());

		return false;
	}
	
	return true;
}

//------------------------------------------------------------------------
void CItem::Reset()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("CItem::Reset Start %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	ResetAllAnimations();

	if (IsModifying())
		ResetAccessoriesScreen(GetOwnerActor());

	ResetOwner();  
  m_scheduler.Reset();
  
	// detach any effects
	TEffectInfoMap temp = m_effects;
	TEffectInfoMap::iterator end = temp.end();
	for (TEffectInfoMap::iterator it=temp.begin(); it!=end;++it)
		AttachEffect(it->second.slot, it->first, false);
	m_effectGenId=0;

	InitItemFromParams();

	m_stateTable[0] = GetEntity()->GetScriptTable();
	if (!!m_stateTable[0])
	{
		m_stateTable[0]->GetValue("Server", m_stateTable[1]);
		m_stateTable[0]->GetValue("Client", m_stateTable[2]);
	}

	Quiet();

	ReAttachAccessories();
	AccessoriesChanged();

	for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
		FixAccessories(GetAccessoryParams(it->first), true);

	InitRespawn();

	Cloak(false);

	m_noDrop = false;

	if(m_sharedparams->params.has_first_select)
		m_stats.first_selection = true; //Reset (just in case)

	OnReset();

	m_releaseCameraBone = false;

	m_userStealthyTime = 0.0f;
	m_isUserStealthy = false;
	m_inIdleAction = false;
	m_idleSoundId = INVALID_SOUNDID;

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("  CItem::Reset End %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
	CryLogAlways("");
#endif

}

//------------------------------------------------------------------------
void CItem::ResetOwner()
{
  if (m_owner.GetId())
  {
    if (m_stats.used)
      StopUse(m_owner.GetId());

    CActor *pOwner = GetOwnerActor();
		if (!pOwner)
			return;

    if (pOwner->GetInventory()->FindItem(GetEntityId()) < 0)
		{
      SetOwnerId(0);
		}
  }
}

//------------------------------------------------------------------------
void CItem::PostInit( IGameObject * pGameObject )
{
	// prevent ai from automatically disabling weapons
	for (int i=0; i<4;i++)
		pGameObject->SetUpdateSlotEnableCondition(this, i, eUEC_WithoutAI);

	Reset();
	
	PatchInitialSetup();	
	InitialSetup();		//Must be called after Patch
}

//------------------------------------------------------------------------
void CItem::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CItem::CheckUserStealthyState(float frameTime)
{
	CActor* pOwnerActor = GetOwnerActor();
	if (gEnv->bMultiplayer || !IsSelected() || !pOwnerActor || !pOwnerActor->IsClient())
		return;

	if (m_userStealthyTime > 0.0f)
	{
		m_userStealthyTime -= frameTime;
	}
	else
	{
		m_isUserStealthy = false;
		m_userStealthyTime = 0.5f;
		ENanoSuitMode suitmode	= (pOwnerActor && pOwnerActor->HasNanoSuit()) ? pOwnerActor->GetActorSuitGameParameters().GetMode() : eNanoSuitMode_Tactical;
		if (suitmode == eNanoSuitMode_Stealth)
		{
			IEntity* pOwner = GetOwner();
			class CAllertnessPredicate : public IAIAlertnessPredicate
			{
			public:
				CAllertnessPredicate(EntityId userId) : m_userId(userId) {}

				virtual bool ConsiderAIObject(IAIObject* pAIObject) const
				{
					IPipeUser* pPipeUser = pAIObject->CastToIPipeUser();
					IAIObject* pTarget = pPipeUser->GetAttentionTarget();
					EntityId pTargetId = pTarget ? pTarget->GetEntityID() : 0;
					return pTargetId == m_userId;
				}
			private:
				EntityId m_userId;
			} allertnessPredicate(pOwner ? pOwner->GetId() : 0);
			int allertness = gEnv->pAISystem->GetAlertness(allertnessPredicate);
			if (allertness < 2)
				m_isUserStealthy = true;
		}
	}
}

//------------------------------------------------------------------------
void CItem::Update( SEntityUpdateContext& ctx, int slot )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (IsDestroyed())
		return;

	switch (slot)
	{
	case eIUS_Scheduler:
		m_scheduler.Update(ctx.fFrameTime);
		break;
	case eIUS_General:
		CheckUserStealthyState(ctx.fFrameTime);
		break;
	}
}

//------------------------------------------------------------------------
bool CItem::SetAspectProfile( EEntityAspects aspect, uint8 profile )
{
	//CryLogAlways("%s::SetProfile(%d: %s)", GetEntity()->GetName(), profile, profile==eIPhys_Physicalized?"Physicalized":"NotPhysicalized");

	if (aspect == eEA_Physics)
	{
		switch (profile)
		{
		case eIPhys_PhysicalizedStatic:
			{
				SEntityPhysicalizeParams params;
				params.type = PE_STATIC;
				params.nSlot = eIGS_ThirdPerson;

				GetEntity()->Physicalize(params);

				return true;
			}
			break;
		case eIPhys_PhysicalizedRigid:
			{
				SEntityPhysicalizeParams params;
				params.type = PE_RIGID;
				params.nSlot = eIGS_ThirdPerson;
				params.mass = m_sharedparams->params.mass;

				pe_params_buoyancy buoyancy;
				buoyancy.waterDamping = 1.5;
				buoyancy.waterResistance = 1000;
				buoyancy.waterDensity = 1;
				params.pBuoyancy = &buoyancy;

				GetEntity()->Physicalize(params);

				IPhysicalEntity *pPhysics = GetEntity()->GetPhysics();
				if (pPhysics)
				{
					pe_action_awake action;
					action.bAwake = m_owner.GetId()!=0;
					pPhysics->Action(&action);
				}
			}
			return true;
		case eIPhys_NotPhysicalized:
			{
				IEntityPhysicalProxy *pPhysicsProxy = GetPhysicalProxy();
				if (pPhysicsProxy)
				{
					SEntityPhysicalizeParams params;
					params.type = PE_NONE;
					params.nSlot = eIGS_ThirdPerson;
					pPhysicsProxy->Physicalize(params);
				}
			}
			return true;
		}
	}

	return false;
}

uint8 CItem::GetDefaultProfile( EEntityAspects aspect )
{
	if (aspect == eEA_Physics)
	{
		if (m_properties.pickable)
		{
			if (m_properties.physics)
				return eIPhys_PhysicalizedRigid;
			return eIPhys_PhysicalizedStatic;
		}
		return eIPhys_NotPhysicalized;
	}
	else
	{
		return 0;
	}
}

//------------------------------------------------------------------------
void CItem::HandleEvent( const SGameObjectEvent &evt )
{
}

//------------------------------------------------------------------------
void CItem::ProcessEvent(SEntityEvent &event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	switch (event.event)
	{
	case ENTITY_EVENT_TIMER:
		{
			switch (event.nParam[0])
			{
			case eIT_Flying:
				m_stats.flying = false;
				
				IgnoreCollision(false);

				//Add an small impulse, sometimes item keeps floating in the air
				IEntityPhysicalProxy *pPhysics = GetPhysicalProxy();
				if (pPhysics)
					pPhysics->AddImpulse(-1, Vec3(0.0f,0.0f,0.0f), Vec3(0.0f,0.0f,-1.0f)*m_sharedparams->params.drop_impulse, false, 1.0f);
				break;
			}
      break;
		}
	case ENTITY_EVENT_PICKUP:
		{
			if (event.nParam[0] == 1 && GetIWeapon())
			{
				// check if the entity picking up the item is cloaked
				IEntity* pEntity = gEnv->pEntitySystem->GetEntity((EntityId)event.nParam[1]);

				if(!pEntity)
					return;

				IEntityRenderProxy * pOwnerRP = (IEntityRenderProxy*) pEntity->GetProxy(ENTITY_PROXY_RENDER);
				IEntityRenderProxy * pItemRP = (IEntityRenderProxy*) GetEntity()->GetProxy(ENTITY_PROXY_RENDER);

				if (pItemRP && pOwnerRP)
				{ 
					pItemRP->SetMaterialLayersMask( pOwnerRP->GetMaterialLayersMask() );
					pItemRP->SetMaterialLayersBlend( pOwnerRP->GetMaterialLayersBlend() );
				}

				for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); ++it)
				{
					EntityId cur = (EntityId)it->second;
					IItem *attachment = m_pItemSystem->GetItem(cur);
					if (attachment)
					{
						pItemRP = (IEntityRenderProxy*) attachment->GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
						if (pItemRP && pOwnerRP)
						{
							pItemRP->SetMaterialLayersMask( pOwnerRP->GetMaterialLayersMask() );
							pItemRP->SetMaterialLayersBlend( pOwnerRP->GetMaterialLayersBlend() );
						}
					}
				}
			}
		}
		break;
  case ENTITY_EVENT_RESET:

		Reset();

		if (gEnv->IsEditor() && !m_stats.mounted)
		{
			IInventory *pInventory=GetActorInventory(GetOwnerActor());

			if (event.nParam[0]) // entering game mode in editor
				m_editorstats=SEditorStats(GetOwnerId(), pInventory?pInventory->GetCurrentItem()==GetEntityId():0);
			else // leaving game mode
			{

				if (m_editorstats.ownerId)
				{
					m_noDrop=true;
					if(IsDualWieldMaster())
						ResetDualWield();

					AttachToBack(false);

					int previousValue = g_pGameCVars->g_inventoryNoLimits;
					g_pGameCVars->g_inventoryNoLimits = 1;

					PickUp(m_editorstats.ownerId, false, false, false);

					g_pGameCVars->g_inventoryNoLimits = previousValue;

					IItemSystem *pItemSystem=g_pGame->GetIGameFramework()->GetIItemSystem();

					if (m_editorstats.current && pInventory && pInventory->GetCurrentItem()==GetEntityId())
					{
						//if (pInventory)
						pInventory->SetCurrentItem(0);
						pItemSystem->SetActorItem(GetActor(m_editorstats.ownerId), GetEntityId(), false);
					}
					else if (pInventory && pInventory->GetCurrentItem()==GetEntityId())
						pItemSystem->SetActorItem(GetActor(m_editorstats.ownerId), (EntityId)0, false);

				}
				else
				{
					if(GetIWeapon() && !GetParentId())
						Drop(0,false,false);

					SetOwnerId(0);

					RemoveAllAccessories();
					PatchInitialSetup();
					InitialSetup();
				}
			}
		}
    break;

/*
	case ENTITY_EVENT_XFORM: 
		{

			int flags = (int)event.nParam[0];
			
			// This first flag is specifically used for entity attachment updates, the ones we are interested in
			bool isAttachmentSystemUpdate = ((flags & ENTITY_XFORM_NO_PROPOGATE) != 0);		
			bool needsFPUpdate = m_stats.fp && isAttachmentSystemUpdate && !(flags & (ENTITY_XFORM_USER|ENTITY_XFORM_PHYSICS_STEP));
			if (needsFPUpdate)
			{
				// Force update here (This will be triggered only for FP weapon, when the entity location has been updated by the player attachment manager)
				// so that anything attached to it (eg muzzle flash) are in sync with the weapon, 
				// EntityObject update of the character has been disabled as well, to prevent double update
				if (ICharacterInstance* pFirstPersonWeaponCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson))
				{
					//Code extracted from CEntityObject::Update( )
					QuatT PhysicalPosition;
					PhysicalPosition.t = GetEntity()->GetWorldPos();
					PhysicalPosition.q = GetEntity()->GetWorldRotation();
					QuatTS AnimatedCharacter = QuatTS( GetEntity()->GetWorldTM() );

					float fDistance = (GetISystem()->GetViewCamera().GetPosition() - AnimatedCharacter.t).GetLength();
					float fZoomFactor = 0.001f + 0.999f*(RAD2DEG(GetISystem()->GetViewCamera().GetFov())/60.f);

					pFirstPersonWeaponCharacter->SkeletonPreProcess( PhysicalPosition, AnimatedCharacter, GetISystem()->GetViewCamera(),0 );
					pFirstPersonWeaponCharacter->SetPostProcessParameter( PhysicalPosition, AnimatedCharacter, 0, fDistance * fZoomFactor, 0xdead ); 
				}

			}

		}
		break;*/

	}
}

//------------------------------------------------------------------------
bool CItem::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags )
{
	NET_PROFILE_SCOPE("Item", ser.IsReading());

	if (aspect == eEA_Physics)
	{
		pe_type type = PE_NONE;
		switch (profile)
		{
		case eIPhys_PhysicalizedStatic:
			type = PE_STATIC;
			break;
		case eIPhys_PhysicalizedRigid:
			type = PE_RIGID;
			break;
		case eIPhys_NotPhysicalized:
			type = PE_NONE;
			break;
		default:
			return false;
		}

		if (type == PE_NONE)
			return true;

		IEntityPhysicalProxy * pEPP = (IEntityPhysicalProxy *) GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS);
		if (ser.IsWriting())
		{
			if (!pEPP || !pEPP->GetPhysicalEntity() || pEPP->GetPhysicalEntity()->GetType() != type)
			{
				gEnv->pPhysicalWorld->SerializeGarbageTypedSnapshot( ser, type, 0 );
				return true;
			}
		}
		else if (!pEPP)
		{
			return false;
		}

		pEPP->SerializeTyped( ser, type, pflags );
	}
	return true;
}

//------------------------------------------------------------------------
void CItem::FullSerialize( TSerialize ser )
{
	assert(ser.GetSerializationTarget() != eST_Network);
	if(ser.IsReading())
	{
		AttachToBack(false);
		m_actionSuffix.clear();
	}

	m_stats.Serialize(ser);

	EntityId ownerId = m_owner.GetId();
	ser.Value("ownerId", ownerId);
	ser.Value("parentId", m_parentId);
	ser.Value("hostId", m_hostId);
	ser.Value("masterID", m_dualWieldMasterId);
	ser.Value("slaveID", m_dualWieldSlaveId);
	m_serializeCloaked = m_cloaked;
	ser.Value("m_cloaked", m_serializeCloaked);
	ser.Value("accAmmoAvail", m_accessoryAmmoAvailable);
	m_serializeActivePhysics = Vec3(0,0,0);
	m_serializeDestroyed = IsDestroyed();
	m_serializeRigidPhysics = true;
	ser.Value("m_serializeDestroyed", m_serializeDestroyed);

	if(ser.IsWriting())
	{
		if(IPhysicalEntity *pPhys = GetEntity()->GetPhysics())
		{
			m_serializeRigidPhysics = (pPhys->GetType()==PE_RIGID);
			pe_status_dynamics dyn;
			if (pPhys->GetStatus(&dyn))
			{
				if(dyn.v.len() > 0.1f)
					m_serializeActivePhysics = dyn.v.GetNormalized();
			}
		}
	}

	ser.Value("m_serializeActivePhysics", m_serializeActivePhysics);
	ser.Value("m_serializeRigidPhysics", m_serializeRigidPhysics);

	m_actionSuffixSerializationHelper = m_actionSuffix;
	ser.Value("actionSuffix", m_actionSuffixSerializationHelper);

	if (ser.IsReading() && m_stats.mounted && m_sharedparams->params.usable)
	{
		if(m_owner.GetId())
		{
			IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_owner.GetId());
			CPlayer *pPlayer = static_cast<CPlayer*> (pActor);
			if(pPlayer)
			{
				if(m_stats.used)
					pPlayer->UseItem(GetEntityId());
				else
					StopUse(m_owner.GetId());
			}
			m_stats.used = false;
		}

		m_postSerializeMountedOwner = ownerId;
	}
	else
	{
		SetOwnerId(ownerId);
	}

	//serialize attachments
	int attachmentAmount = m_accessories.size();
	ser.Value("attachmentAmount", attachmentAmount);
	if(ser.IsWriting())
	{
		TAccessoryMap::iterator it = m_accessories.begin();
		for(; it != m_accessories.end(); ++it)
		{
			string name((it->first).c_str());
			EntityId id = it->second;
			ser.BeginGroup("Accessory");
			ser.Value("Name", name);
			ser.Value("Id", id);
			ser.EndGroup();
		}
	}
	else if(ser.IsReading())
	{
		m_accessories.clear();
		string name;
		for(int i = 0; i < attachmentAmount; ++i)
		{
			EntityId id = 0;
			ser.BeginGroup("Accessory");
			ser.Value("Name", name);
			ser.Value("Id", id);
#ifndef ITEM_USE_SHAREDSTRING
			m_accessories[name] = id;
#else
			m_accessories[ItemString(name)] = id;
#endif
			ser.EndGroup();
		}
	}

	if(ser.IsReading())
	{
		SetViewMode(m_stats.viewmode);

		//Back attachments
		if(m_stats.backAttachment!=eIBA_Unknown)
		{
			AttachToBack(true);
		}
	}
}

//------------------------------------------------------------------------
void CItem::PostSerialize()
{
	IActor* pOwner = GetOwnerActor();

	if(m_owner.GetId())
	{
		EntityId owner = m_owner.GetId();
		//Select(m_stats.selected);
		if(pOwner)
		{

			if (!m_stats.mounted && !IsDualWield())
			{
				IInventory* pInv = pOwner->GetInventory();
				if (pInv != NULL)
				{
					EntityId holstered = pInv->GetHolsteredItem();

					if(GetEntity()->GetClass() != CItem::sBinocularsClass)
					{			  
						if(m_stats.selected)
						{
							if(holstered != GetEntityId())
							{
								Drop(0,false,false);
								PickUp(owner, false, true, false);
								if(pOwner->GetEntity()->IsHidden())
									Hide(true); //Some AI is hidden in the levels by designers
							}      
						}
						else
						{ 
							if(holstered != GetEntityId())
								AttachToHand(false, true);
						}
					}
				}
			}
		}
		else
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Item %s has ownerId %i but owner actor does not exist!", GetEntity()->GetName(), m_owner.GetId());
	}

	if (m_stats.mounted && !m_hostId)
	{
		Vec3 olddir=m_stats.mount_dir;
		Vec3 oldaimdir=m_stats.mount_last_aimdir;
		MountAt(GetEntity()->GetWorldPos());

		m_stats.mount_dir=olddir;
		m_stats.mount_last_aimdir=oldaimdir;
	}

	ReAttachAccessories();
	AccessoriesChanged();

	//this fix breaks holding NPC serialization, when "use" was pressed during saving
	/*if(pOwner && this == pOwner->GetCurrentItem() && !pOwner->GetLinkedVehicle())
	{
		pOwner->HolsterItem(true);	//this "fixes" old attachments that are not replaced still showing up in the model ..
		pOwner->HolsterItem(false);
	}*/

	//Fix incorrect view mode (in same cases) and not physicalized item after dropping/picking (in same cases too)
	if(!pOwner && m_stats.dropped)
	{
		SetViewMode(eIVM_ThirdPerson);
		
		Pickalize(true, false);
		GetEntity()->EnablePhysics(true);

		Physicalize(true, m_serializeRigidPhysics);

		if(m_serializeActivePhysics.len())	//this fixes objects being frozen in air because they were rephysicalized
		{
			IEntityPhysicalProxy *pPhysics = GetPhysicalProxy();
			if (pPhysics)
				pPhysics->AddImpulse(-1, m_sharedparams->params.drop_impulse_pos, m_serializeActivePhysics*m_sharedparams->params.drop_impulse, true, 1.0f);
			m_serializeActivePhysics = Vec3(0,0,0);
		}

	}

	m_actionSuffix = m_actionSuffixSerializationHelper;
	if(pOwner && pOwner->IsPlayer() && IsSelected() && !stricmp(m_actionSuffixSerializationHelper.c_str(), "akimbo_"))
	{
		PlayAction(GetParams().idle);
	}
	else
	{
		ResetActionSuffix();
//		m_actionSuffix.clear();
	}
	m_actionSuffixSerializationHelper.clear();

	if(m_postSerializeMountedOwner)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_postSerializeMountedOwner);
		CPlayer *pPlayer = static_cast<CPlayer*> (pActor);
		if(pPlayer && m_sharedparams->params.usable)
		{
			m_stats.used = false;
			SetOwnerId(m_postSerializeMountedOwner);
			StopUse(pPlayer->GetEntityId());
			pPlayer->UseItem(GetEntityId());
			assert(m_owner.GetId());
		}
		m_postSerializeMountedOwner = 0;		
	}

	if(m_serializeCloaked)
		CloakSync(false);
	else
		CloakEnable(false, false);

	if(m_serializeDestroyed)
		OnDestroyed();
}

//------------------------------------------------------------------------
void CItem::SerializeLTL( TSerialize ser )
{
	ser.BeginGroup("ItemLTLSerialization");

	if(ser.IsReading()) //dual wield is handled by the inventory LTL serialization
	{
		SetDualWieldMaster(0);
		SetDualWieldSlave(0);
	}

	//serialize attachments
	int attachmentAmount = m_accessories.size();
	ser.Value("attachmentAmount", attachmentAmount);
	if(ser.IsWriting())
	{
		TAccessoryMap::iterator it = m_accessories.begin();
		for(; it != m_accessories.end(); ++it)
		{
			string name((it->first.c_str()));
			ser.BeginGroup("Accessory");
			ser.Value("Name", name);
			ser.EndGroup();
		}
	}
	else if(ser.IsReading())
	{
		while(m_accessories.size() > 0)
		{
			RemoveAccessory((m_accessories.begin())->first);
		}
		assert(m_accessories.size() == 0);

		string name;
		IActor *pActor = GetOwnerActor();
		for(int i = 0; i < attachmentAmount; ++i)
		{
			ser.BeginGroup("Accessory");
			ser.Value("Name", name);
			ser.EndGroup();

			if(pActor)
				AddAccessory(name.c_str());
		}
		if(attachmentAmount)
		{
			ReAttachAccessories();
			AccessoriesChanged();

			if(!IsSelected())
			{
				Hide(true);
				TAccessoryMap::const_iterator it=m_accessories.begin();
				TAccessoryMap::const_iterator end=m_accessories.end();
				for(;it!=end;++it)
				{
					if(CItem* pItem = static_cast<CItem*>(m_pItemSystem->GetItem(it->second)))
						pItem->Hide(true);
				}
			}
		}

	}

	ser.EndGroup();
}

//------------------------------------------------------------------------
void CItem::SetOwnerId(EntityId ownerId)
{
	if(ownerId)
	{
		g_pGame->GetTacticalManager()->RemoveEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Item);
		
		CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(ownerId));
		SActorWeakRef actorReference;
		if (pActor)
		{
			actorReference = pActor->GetWeakRef();
		}
		m_owner.Set(ownerId, actorReference);
	}
	else
	{
		g_pGame->GetTacticalManager()->AddEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Item);
		m_owner.Reset();
	}

	CHANGED_NETWORK_STATE(this, ASPECT_OWNER_ID);
}

//------------------------------------------------------------------------
EntityId CItem::GetOwnerId() const
{
	return m_owner.GetId();
}

//------------------------------------------------------------------------
void CItem::SetParentId(EntityId parentId)
{
	m_parentId = parentId;

	CHANGED_NETWORK_STATE(this, ASPECT_OWNER_ID);
}

//------------------------------------------------------------------------
EntityId CItem::GetParentId() const
{
	return m_parentId;
}

//------------------------------------------------------------------------
void CItem::SetHand(int hand)
{
	m_stats.hand = hand;

	int idx = 0;
	if (hand == eIH_Right)
		idx = 1;
	else if (hand == eIH_Left)
		idx = 2;

	if (m_fpgeometry[idx].name.empty())
		idx = 0;

	bool result=false;
	bool ok=true;

	CItem *pParent=m_parentId?static_cast<CItem*>(m_pItemSystem->GetItem(m_parentId)):NULL;
	if (pParent)
			ok = pParent->IsSelected();

	if (m_stats.mounted || (pParent && pParent->GetStats().mounted))
		ok=true;

	if (m_stats.viewmode&eIVM_FirstPerson && ok)
	{
		SGeometry &geometry = m_fpgeometry[idx];
		result = SetGeometry(eIGS_FirstPerson, geometry.name, geometry.position, geometry.angles, geometry.scale);
		if (!m_stats.mounted && (m_parentId == 0))
		{
			EnableEntitySystemFPCharacterUpdate(false);
		}
	}

	if (idx == 0)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);
		if (!pCharacter)
			return;
	}

	if (result)
		PlayAction(m_idleAnimation[eIGS_FirstPerson], 0, true, (eIPAF_Default|eIPAF_NoBlend)&~eIPAF_OwnerAnimGraph);
}

//------------------------------------------------------------------------
void CItem::Use(EntityId userId)
{
	if (m_sharedparams->params.usable && m_stats.mounted)
	{
		if (!m_owner.GetId())
			StartUse(userId);
		else if (m_owner.GetId() == userId)
			StopUse(userId);
	}
}

//------------------------------------------------------------------------
struct CItem::SelectAction
{
	void execute(CItem *_item)
	{
		_item->SetBusy(false);
		_item->ForcePendingActions();
	}
};

void CItem::Select(bool select)
{
	bool wasSelected = m_stats.selected;
	if(!m_owner.GetId())
		select = false;

	m_stats.selected=select;

	CheckViewChange();

  IAISystem* pAISystem = gEnv->pAISystem;

	CWeaponAttachmentManager* pWAM = GetOwnerActor()?GetOwnerActor()->GetWeaponAttachmentManager():NULL;

	if (select)
	{
		if (!wasSelected)
			ResetActionSuffix();

		if (IsDualWield())
		{
			//SetActionSuffix(m_params.dual_wield_suffix.c_str());
			if(m_dualWieldSlaveId)
				SetDualWieldSlave(m_dualWieldSlaveId);
			if(m_dualWieldMasterId)
				SetDualWieldMaster(m_dualWieldMasterId);
		}

		Hide(false);

		if (!m_stats.mounted && GetOwner())
		  GetEntity()->SetWorldTM(GetOwner()->GetWorldTM());	// move somewhere near the owner so the sound can play
		
		float speedOverride = 1.0f;
		CActor *pOwner = GetOwnerActor();
		if(pOwner && pOwner->IsPlayer())
		{
			CPlayer* pPlayer = static_cast<CPlayer*>(pOwner);
			speedOverride *= pPlayer->GetModifiableValues().GetValue(kPMV_WeaponSelectSpeedScale);
		}

		const char* select_animation;
	
		//LAW, shotgun have 2 different select animations
		IWeapon* pWeapon = GetIWeapon();
		if(m_sharedparams->params.has_first_select && m_stats.first_selection)
			select_animation = g_pItemStrings->first_select;
		else if (pWeapon && pWeapon->OutOfAmmo(false))
			select_animation = GetParams().select_empty;
		else
			select_animation = GetParams().select;

		if (!IsMounted())
			PlayAction(select_animation, 0, false, eIPAF_Default|eIPAF_NoBlend|eIPAF_RestartAnimation, speedOverride);
		m_stats.first_selection = false;

		ForceFPSkeletonUpdate();
		uint32 selectBusyTimer = 0;
		if (m_sharedparams->params.select_override == 0.0f)
			selectBusyTimer = MAX(250, GetCurrentAnimationTime(eIGS_Owner)) - 250;
		else
			selectBusyTimer = (uint32)m_sharedparams->params.select_override*1000;
		SetBusy(true);
		GetScheduler()->TimerAction(selectBusyTimer, CSchedulerAction<SelectAction>::Create(), false);

		AttachToHand(true);

		AttachToBack(false);

		if (pOwner)
		{
			// update smart objects states
			if (!gEnv->bMultiplayer && pAISystem)
			{
				//This is very expensive, ~150us on PS3, per call.
				IEntity* pOwnerEntity = pOwner->GetEntity();
				pAISystem->GetSmartObjectManager()->ModifySmartObjectStates( pOwnerEntity, GetEntity()->GetClass()->GetName() );
				pAISystem->GetSmartObjectManager()->ModifySmartObjectStates( pOwnerEntity, "WeaponDrawn" );
			}

			//[kirill] make sure AI gets passed the new weapon properties
			if(GetIWeapon() && pOwner->GetEntity() && pOwner->GetEntity()->GetAI())
				pOwner->GetEntity()->GetAI()->SetWeaponDescriptor(GetIWeapon()->GetAIWeaponDescriptor());
		}    
	}
	else
	{
		GetScheduler()->Reset(true);

		if (!m_stats.mounted && !AttachToBack(true))
		{
			SetViewMode(0);
			Hide(true);
		}

		// set no-weapon pose on actor (except for the Offhand)
		CActor *pOwner = GetOwnerActor();
		if (pOwner && g_pItemStrings)
			pOwner->PlayAction(GetParams().idle, ITEM_DESELECT_POSE);

		EnableUpdate(false);

		ReleaseStaticSounds();
		if(!m_stats.dropped) //This is done already in CItem::Drop (could cause problems with dual socom)
			ResetAccessoriesScreen(pOwner);

		if(!m_stats.dropped)
			AttachToHand(false);
		//AttachArms(false, false);

		if (m_stats.mounted)
			m_stats.fp=false; // so that OnEnterFirstPerson is called next select

		// update smart objects states
/* //Diesel cut
		if ( !gEnv->bMultiplayer && pAISystem && pOwnerActor )
		{
			CryFixedStringT<256> tmpString( "-WeaponDrawn," );
			tmpString += GetEntity()->GetClass()->GetName();
			pAISystem->GetSmartObjectManager()->ModifySmartObjectStates( pOwner->GetEntity(), tmpString.c_str() );
		}
*/ //Diesel cut
		RemoveOwnerAttachedAccessories();

	}

	if (IItem *pSlave=GetDualWieldSlave())
		pSlave->Select(select);

	// ensure attachments get cloaked
	if (select)
	{
		CloakSync(false);
	}
	else
		CloakEnable(false, false);

	if(CActor *pOwner = GetOwnerActor())
	{
		if(g_pGame && g_pGame->GetIGameFramework())		
			if(pOwner == g_pGame->GetIGameFramework()->GetClientActor())
				SAFE_HUD_FUNC(UpdateCrosshair(select?this:NULL));	//crosshair might change
	}

	OnSelected(select);
}

//------------------------------------------------------------------------
uint32 CItem::StartDeselection()
{
	return 0;
}

//------------------------------------------------------------------------
void CItem::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	bool isDWSlave=IsDualWieldSlave();
	bool isDWMaster=IsDualWieldMaster();
		
	CActor *pOwner = GetOwnerActor();
	
	if (isDWMaster)
	{
		IItem *pSlave=GetDualWieldSlave();
		if (pSlave)
		{
			pSlave->Drop(impulseScale, false, byDeath);
			ResetDualWield();
			
			if (!byDeath)
				Select(true);

			if (IsServer() && pOwner)
			{
				GetGameObject()->SetNetworkParent(0);
				if ((GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)) == 0)
					pOwner->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale, selectNext, byDeath), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
				//pOwner->GetGameObject()->InvokeRMI(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale), eRMI_ToRemoteClients);
			}
			m_pItemSystem->DropActorAccessory(pOwner, GetEntity()->GetId());

			//AI drop both socoms
			if (!byDeath || (pOwner && !pOwner->IsPlayer()))
				return;
		}
	}

	ResetDualWield();

	if (pOwner)
	{
		IInventory *pInventory = GetActorInventory(pOwner);
		if (pInventory && pInventory->GetCurrentItem() == GetEntity()->GetId())
			pInventory->SetCurrentItem(0);

		if (IsServer()) // don't send slave drops over network, the master is sent instead
		{
			GetGameObject()->SetNetworkParent(0);

			if (!isDWSlave)
			{
				if ((GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)) == 0)
					pOwner->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale, selectNext, byDeath), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
			}
		}
	}

	bool wasHidden = GetEntity()->IsHidden();

	m_stats.dropped = true;

	bool performCloakFade = m_cloaked;

	Select(false);
	SetViewMode(eIVM_ThirdPerson);
	AttachToHand(false,true);
	AttachToBack(false);

	if (performCloakFade)
	{
		CloakEnable(false, true);
	}

	// AI should ignore collisions from this item for a while
	// to not 'scare' himself and the friends around him
	gEnv->pAISystem->IgnoreStimulusFrom(GetEntity()->GetId(), AISTIM_COLLISION, 2.0f);

	Hide(false);
	GetEntity()->EnablePhysics(true);
	Physicalize(true, true);

	IgnoreCollision(true);

	EntityId ownerId = GetOwnerId();

	if (pOwner && gEnv->bServer)
	{
		// bump network priority for a moment
		pOwner->GetGameObject()->Pulse('bang');
		GetGameObject()->Pulse('bang');

		if (IMovementController * pMC = pOwner->GetMovementController())
		{
			SMovementState moveState;
			pMC->GetMovementState(moveState);

			Vec3 dir(ZERO);
			Vec3 vel(ZERO);
			dir.Set(0.0f,0.0f,-1.0f);

			if(pOwner->IsPlayer())
			{
				if(GetEntity()->GetPhysics())
				{
					Vec3 pos = moveState.eyePosition;
					dir = moveState.aimDirection;

					if (IPhysicalEntity *pPE=pOwner->GetEntity()->GetPhysics())
					{
						pe_status_dynamics sv;
						if (pPE->GetStatus(&sv))
						{
							if (sv.v.len2()>0.0f)
							{
								float dot=sv.v.GetNormalized().Dot(dir);
								if (dot<0.0f)
									dot=0.0f;
								vel=sv.v*dot;
							}
						}
					}
				}
			}

			IEntityPhysicalProxy *pPhysics = GetPhysicalProxy();
			if (pPhysics)
			{
				if (vel.len2()>0.0f)
				{
					if (IPhysicalEntity *pPE=pPhysics->GetPhysicalEntity())
					{
						pe_action_set_velocity asv;
						asv.v=vel;

						pPE->Action(&asv);
					}
				}

				Vec3 impulsePoint = GetEntity()->GetWorldTM().TransformPoint(m_sharedparams->params.drop_impulse_pos);
				pPhysics->AddImpulse(-1, impulsePoint, dir*m_sharedparams->params.drop_impulse*impulseScale, true, 1.0f);
			}
		}
	}

	if (CActor *pActor = GetOwnerActor())
	{
		if(IInventory *pInventory = GetActorInventory(pActor))
			pInventory->RemoveItem(GetEntity()->GetId());
	}
	SetOwnerId(0);

	Pickalize(true, true);
	EnableUpdate(false);

	if (g_pGame->GetGameRules())
	{
		g_pGame->GetGameRules()->OnItemDropped(GetEntityId(), pOwner?pOwner->GetEntityId():0);
	}

	if (pOwner && pOwner->IsClient())
	{
		if(!isDWSlave)
			ResetAccessoriesScreen(pOwner);

		if (CanSelect() && selectNext && pOwner->GetHealth()>0 && !isDWSlave)
		{
			CBinocular *pBinoculars = static_cast<CBinocular*>(pOwner->GetItemByClass(CItem::sBinocularsClass));

			if (pOwner->GetInventory() && pOwner->GetInventory()->GetLastItem()
				  && (!pBinoculars || pBinoculars->GetEntityId()!=pOwner->GetInventory()->GetLastItem()))
				pOwner->SelectLastItem(false);
			else
				pOwner->SelectNextItem(1, false);
		}
		if (CanSelect())
			m_pItemSystem->DropActorItem(pOwner, GetEntity()->GetId());
		else
			m_pItemSystem->DropActorAccessory(pOwner, GetEntity()->GetId());
	}

	Cloak(false);

	Quiet();

	OnDropped(ownerId);
}

//------------------------------------------------------------------------
void CItem::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char* setup)
{
	CActor *pActor = GetActor(pickerId);
	if (!pActor)
		return;

	TriggerRespawn();

	GetEntity()->EnablePhysics(false);
	Physicalize(false, false);
	Pickalize(false, false);

	bool soundEnabled = IsSoundEnabled();
	EnableSound(sound);

	SetViewMode(0);		
	SetOwnerId(pickerId);

	CopyRenderFlags(GetOwner());

	Hide(true);
	m_stats.dropped = false;
	m_stats.brandnew = false;

	// move the entity to picker position
	Matrix34 tm(pActor->GetEntity()->GetWorldTM());
	tm.AddTranslation(Vec3(0,0,2));
	GetEntity()->SetWorldTM(tm);

	bool alone = true;
	bool slave = false;
	IInventory *pInventory = pActor->GetInventory();
	if (!pInventory)
	{
		GameWarning("Actor '%s' has no inventory, when trying to pickup '%s'!",pActor->GetEntity()->GetName(),GetEntity()->GetName());
		return;
	}

	if (IsServer() && pActor->IsPlayer())
	{
		// go through through accessories map and give a copy of each to the player
		for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); ++it)
		{
			const char *name=it->first.c_str();
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			if(!pInventory->HasAccessory(pClass))
				g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, name, false, false, true);
			else if(CItem* pAccessory = static_cast<CItem*>(m_pItemSystem->GetItem(it->second)))
				pAccessory->OnPickedUp(pickerId,true);
				
		}

		for(TInitialSetup::iterator it = m_initialSetup.begin(); it != m_initialSetup.end(); ++it)
		{
			const char *name=it->c_str();
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			if(!pInventory->HasAccessory(pClass))
				g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, name, false, false, true);
		}
	}

	CItem *pBuddy=0;
	pBuddy = static_cast<CItem*>(m_pItemSystem->GetItem(pInventory->GetItemByClass(GetEntity()->GetClass())));
	if(pBuddy && pBuddy!=this)
	{
		if (!pBuddy->IsDualWield() && pBuddy->SupportsDualWield(GetEntity()->GetClass()->GetName()))
		{
			bool wasSelected = false;
			if (pBuddy->GetEntity()->GetId() == pInventory->GetCurrentItem())
			{
				pBuddy->Select(false);
				wasSelected = true;
			}
			EnableUpdate(true, eIUS_General);

			if(!pInventory->IsSerializingForLevelChange())			
			{
				SetDualWieldMaster(pBuddy->GetEntity()->GetId());
				pBuddy->SetDualWieldSlave(GetEntity()->GetId());
				pBuddy->SetDualSlaveAccessory();
				slave = true;
			}
			if (wasSelected)
				pBuddy->Select(true);

			//Set the same fire mode for both weapons
			IWeapon *pMasterWeapon = pBuddy->GetIWeapon();
			IWeapon *pSlaveWeapon = GetIWeapon();
			if(pMasterWeapon && pSlaveWeapon)
			{
				if(pMasterWeapon->GetCurrentFireMode()!=pSlaveWeapon->GetCurrentFireMode())
				{
					pSlaveWeapon->ChangeFireMode();
				}
			}
		}
		else
		{
			alone = false;
		}
	}


	OnPickedUp(pickerId, m_sharedparams->params.unique && !alone);	// some weapons will get ammo when picked up
																										// this will dictate the result of CanSelect() below
																										// so we'll grab the ammo before trying to select

	if (slave || alone || !m_sharedparams->params.unique)
	{
		uint8 uniqueId = m_pItemSystem->GetItemUniqueId(GetEntity()->GetClass()->GetName());
		bool hasItem = pActor->GetInventory()->GetCountOfUniqueId(uniqueId) != 0;

		// add to inventory
		pInventory->AddItem(GetEntity()->GetId());

		SPlayerStats *pStats = NULL;
		if(pActor && pActor->GetActorClassType() == CPlayer::GetActorClassType())
			pStats = static_cast<SPlayerStats*>(pActor->GetActorStats());

		if (select && !hasItem)
		{
			bool shouldCheck = (pActor->IsClient() || gEnv->bServer);
			if (!shouldCheck || (!pActor->GetLinkedVehicle() && !pActor->ShouldSwim()))
			{
				if(CanSelect() && !slave)
					m_pItemSystem->SetActorItem(pActor, GetEntity()->GetId(), keepHistory);
				else
					m_pItemSystem->SetActorAccessory(pActor, GetEntity()->GetId(), keepHistory);

				CHANGED_NETWORK_STATE(pActor, CPlayer::ASPECT_CURRENT_ITEM);
			}
		}

		EnableSound(soundEnabled);

		if (g_pGame->GetGameRules())
			g_pGame->GetGameRules()->OnItemPickedUp(GetEntityId(), pickerId);

		PlayAction(g_pItemStrings->pickedup);

		//AI back weapon attachments
		if(!gEnv->bMultiplayer && !IsSelected())
		{
			AttachToBack(true);
		}
	}
	else if (!slave && m_sharedparams->params.unique && !alone)
	{
		if (IsServer() && g_pGame->GetGameRules())
			g_pGame->GetGameRules()->OnItemPickedUp(GetEntityId(), pickerId);

		if (IsServer() && !IsDemoPlayback())
			RemoveEntity();

		PlayAction(g_pItemStrings->pickedup);
	}

	if (IsServer() && pActor)
	{
		GetGameObject()->SetNetworkParent(pickerId);
		if ((GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)) == 0)
		{
			pActor->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClPickUp(), CActor::PickItemParams(GetEntityId(), m_stats.selected, sound), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
		}
	}
}

//------------------------------------------------------------------------
void CItem::Physicalize(bool enable, bool rigid)
{
	int profile=eIPhys_NotPhysicalized;
	if (enable)
		profile=rigid?eIPhys_PhysicalizedRigid:eIPhys_PhysicalizedStatic;

	if (IsServer())
		GetGameObject()->SetAspectProfile(eEA_Physics, profile);
}

//------------------------------------------------------------------------
void CItem::Pickalize(bool enable, bool dropped)
{
	if (enable)
	{
		m_stats.flying = dropped;
		m_stats.dropped = true;
		m_stats.pickable = true;

		if(dropped)
		{
			GetEntity()->KillTimer(eIT_Flying);
			GetEntity()->SetTimer(eIT_Flying, m_sharedparams->params.fly_timer);
		}

		if(GetEntity()->IsSlotValid(eIGS_Aux0))
		{
			DrawSlot(eIGS_Aux0, true);
			DrawSlot(eIGS_FirstPerson, false);
			DrawSlot(eIGS_ThirdPerson, false);
		}
	}
	else
	{
		if(GetEntity()->IsSlotValid(eIGS_Aux0))
		{
			DrawSlot(eIGS_Aux0, false);
		}

		m_stats.flying = false;
		m_stats.pickable = false;
	}
}

//------------------------------------------------------------------------
void CItem::IgnoreCollision(bool ignore)
{
	IPhysicalEntity *pPE=GetEntity()->GetPhysics();
	if (!pPE)
		return;

	if (ignore)
	{
		CActor *pActor=GetOwnerActor();
		if (!pActor)
			return;

		IPhysicalEntity *pActorPE=pActor->GetEntity()->GetPhysics();
		if (!pActorPE)
			return;

		pe_action_add_constraint ic;
		ic.flags=constraint_inactive|constraint_ignore_buddy;
		ic.pBuddy=pActorPE;
		ic.pt[0].Set(0,0,0);
		m_constraintId=pPE->Action(&ic);
	}
	else
	{
		pe_action_update_constraint up;
		up.bRemove=true;
		up.idConstraint = m_constraintId;
		m_constraintId=0;
		pPE->Action(&up);
	}
}

//----------------------------------------------------------------------
void CItem::RegisterFPWeaponForRenderingAlways(bool registerRenderAlways)
{
	IEntityRenderProxy* pProxy = GetRenderProxy();
	IRenderNode* pRenderNode = pProxy ? pProxy->GetRenderNode() : NULL;
	if(pRenderNode)
	{
		pRenderNode->SetRndFlags(ERF_RENDER_ALWAYS, registerRenderAlways);
	}
}
//------------------------------------------------------------------------
void CItem::Impulse(const Vec3 &position, const Vec3 &direction, float impulse)
{
	if (direction.len2() <= 0.001f)
		return;

	IEntityPhysicalProxy *pPhysicsProxy = GetPhysicalProxy();
	if (pPhysicsProxy)
		pPhysicsProxy->AddImpulse(-1, position, direction.GetNormalized()*impulse, true, 1);
}

//------------------------------------------------------------------------
bool CItem::CanPickUp(EntityId userId) const
{
	bool canPickUp = m_sharedparams->params.pickable && m_stats.pickable && !m_stats.flying && !m_owner.GetId() && !GetEntity()->IsHidden();
	if (!canPickUp)
		return false;

	int teamId = g_pGame->GetGameRules()->GetTeam(GetEntity()->GetId());
	if (teamId && teamId != g_pGame->GetGameRules()->GetTeam(userId))
		return false;

	return true;
}

//------------------------------------------------------------------------
bool CItem::CanDrop() const
{
	if (m_sharedparams->params.droppable)
		return true;

	return false;
}

//------------------------------------------------------------------------
bool CItem::CanUse(EntityId userId) const
{
	bool bUsable = m_sharedparams->params.usable && m_properties.usable && IsMounted() && (!m_stats.used || (m_owner.GetId() == userId) && !GetEntity()->IsHidden());
	if (!bUsable)
		return (false);

	if (IsMounted() && (m_owner.GetId() != userId))
	{	
		// if not a machine gun on a vehicle, check if we're in front of it 
		CActor *pActor = GetActor(userId);
		if (!pActor)
			return (true); 
		if (pActor->GetLinkedVehicle())
			return (true);

		Vec3 vActorDir = pActor->GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION);
		
		Vec3 mountDirection = GetEntity()->GetParent() ? GetEntity()->GetParent()->GetWorldTM().TransformVector(m_stats.mount_dir) : m_stats.mount_dir;
		float fDot = vActorDir * mountDirection;

		if (fDot<0)
			return (false);		
	}

	return (true);
}

//------------------------------------------------------------------------
bool CItem::IsMounted() const
{
	return m_stats.mounted;
}

//-------------------------------------------------------------------------
bool CItem::IsMountable() const 
{ 
	return m_sharedparams->params.mountable; 
}

//--------------------------------------------------------------------------
const SParams &CItem::GetParams() const 
{ 
	return m_sharedparams->params; 
};

//---------------------------------------------------------------------
bool CItem::GivesAmmo() 
{ 
	return (m_accessoryAmmoAvailable && !m_sharedparams->bonusAccessoryAmmo.empty() || m_sharedparams->params.attachment_gives_ammo);
}

//---------------------------------------------------------------------
bool CItem::IsAutoDroppable() const
{
	return m_sharedparams->params.auto_droppable;
}

//---------------------------------------------------------------------
bool CItem::CanPickUpAutomatically() const
{
	return m_sharedparams->params.auto_pickable;
}

//------------------------------------------------------------------------
void CItem::SetMountedAngleLimits(float min_pitch, float max_pitch, float yaw_range)
{
	m_properties.mounted_min_pitch = min(min_pitch,max_pitch);//(min_pitch<=0.0f)?min_pitch:0.0f; //Assert values
	m_properties.mounted_max_pitch = max(min_pitch,max_pitch);//(max_pitch>=0.0f)?max_pitch:0.0f;
	m_properties.mounted_yaw_range = yaw_range;
}


//------------------------------------------------------------------------
Vec3 CItem::GetMountedAngleLimits() const
{
	if(m_stats.mounted)
		return Vec3(m_properties.mounted_min_pitch, m_properties.mounted_max_pitch, m_properties.mounted_yaw_range);
	else 
		return ZERO;
}

//------------------------------------------------------------------------
bool CItem::IsUsed() const
{
	return m_stats.used;
}

//------------------------------------------------------------------------
bool CItem::InitRespawn()
{
	if (IsServer() && m_respawnprops.respawn)
	{
		CGameRules *pGameRules=g_pGame->GetGameRules();
		assert(pGameRules);
		if (pGameRules)
			pGameRules->CreateEntityRespawnData(GetEntityId());

		return true;
	}

	return false;
};

//------------------------------------------------------------------------
void CItem::TriggerRespawn()
{
	if (!m_stats.brandnew || !IsServer())
		return;
	
	if (m_respawnprops.respawn)
	{

		CGameRules *pGameRules=g_pGame->GetGameRules();
		assert(pGameRules);
		if (pGameRules)
			pGameRules->ScheduleEntityRespawn(GetEntityId(), m_respawnprops.unique, m_respawnprops.timer);
	}
}

//------------------------------------------------------------------------
void CItem::EnableSelect(bool enable)
{
	m_stats.selectable = enable;
}

//------------------------------------------------------------------------
bool CItem::CanSelect() const
{
	return m_sharedparams->params.selectable && m_stats.selectable;
}

//------------------------------------------------------------------------
bool CItem::CanDeselect() const
{
	return true;
}

//------------------------------------------------------------------------
bool CItem::IsSelected() const
{
	return m_stats.selected;
}

bool CItem::GetAnimGraphInputs(CItem::TempAGInputName &pose, CItem::TempAGInputName &suffix)
{
	pose   = (IsDualWield() && !m_sharedparams->params.dual_wield_pose.empty()) ? m_sharedparams->params.dual_wield_pose.c_str() : m_sharedparams->params.pose.c_str();
	CRY_TODO(23,11, 2009, "Remove this underscore post-MS2! It's a quick hack to save changing all the weapon CAL files & the AI animGraph");
	if (m_actionSuffixAG.length() > 0)
	{
		suffix = "_"; suffix.append(m_actionSuffixAG.c_str());
	}
	else
	{
		suffix = m_actionSuffixAG.c_str();
	}

	return true;
}

//------------------------------------------------------------------------
void CItem::EnableSound(bool enable)
{
	m_stats.sound_enabled = enable;
}

//------------------------------------------------------------------------
bool CItem::IsSoundEnabled() const
{
	return m_stats.sound_enabled;
}

//------------------------------------------------------------------------
void CItem::MountAt(const Vec3 &pos)
{
	if (!m_sharedparams->params.mountable)
		return;

	m_stats.mounted = true;

	SetViewMode(eIVM_ThirdPerson);
	
	Matrix34 tm(GetEntity()->GetWorldTM());
	tm.SetTranslation(pos);
	GetEntity()->SetWorldTM(tm);

	m_stats.mount_dir = GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION);
}

//------------------------------------------------------------------------
void CItem::MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles)
{
	if (!m_sharedparams->params.mountable)
		return;

	IEntity *pHost = m_pEntitySystem->GetEntity(entityId);
	if (!pHost)
		return;

	m_hostId = entityId;
	m_stats.mounted = true;

	SetViewMode(eIVM_ThirdPerson);

	pHost->AttachChild(GetEntity(), 0);

	Matrix34 tm = Matrix33(Quat::CreateRotationXYZ(angles));
	tm.SetTranslation(pos);
	GetEntity()->SetLocalTM(tm);

	m_stats.mount_dir = GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION);
}


//------------------------------------------------------------------------
IEntity *CItem::GetOwner() const
{
	if (!m_owner.GetId())
		return 0;

	return m_pEntitySystem->GetEntity(m_owner.GetId());
}

//------------------------------------------------------------------------
CActor *CItem::GetOwnerActor() const
{
	return m_owner.GetActorRef().Get();
}

//------------------------------------------------------------------------
CActor *CItem::GetActor(EntityId actorId) const
{
	if(!m_pGameFramework)
		return NULL;

	IActor* pActor = m_pGameFramework->GetIActorSystem()->GetActor(actorId);
	if (!pActor)
		return NULL;

	return static_cast<CActor*>(pActor);
}

//------------------------------------------------------------------------
IInventory *CItem::GetActorInventory(IActor *pActor) const
{
	if (!pActor)
		return 0;

	return pActor->GetInventory();
}

//------------------------------------------------------------------------
CItem *CItem::GetActorItem(IActor *pActor) const
{
	if (!pActor)
		return 0;

	IInventory *pInventory = pActor->GetInventory();
	if (!pInventory)
		return 0;

	EntityId id = pInventory->GetCurrentItem();
	if (!id)
		return 0;

	return static_cast<CItem *>(m_pItemSystem->GetItem(id));
}

//------------------------------------------------------------------------
EntityId CItem::GetActorItemId(IActor *pActor) const
{
	if (!pActor)
		return 0;

	IInventory *pInventory = pActor->GetInventory();
	if (!pInventory)
		return 0;

	EntityId id = pInventory->GetCurrentItem();
	if (!id)
		return 0;

	return id;
}

//------------------------------------------------------------------------
CActor *CItem::GetActorByNetChannel(INetChannel *pNetChannel) const
{
	return static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel)));
}

//------------------------------------------------------------------------
const char *CItem::GetDisplayName() const
{
	return m_sharedparams->params.display_name.c_str();
}

//------------------------------------------------------------------------
void CItem::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	const char* temp = actionId.c_str();
	CallScriptEvent(eISET_Client, "OnAction", actorId, temp, activationMode, value, 0, 0, 0);
}

//------------------------------------------------------------------------
void CItem::StartUse(EntityId userId)
{
	if (!m_sharedparams->params.usable || m_owner.GetId())
		return;

	// holster user item here
	SetOwnerId(userId);

	CActor* pOwner=GetOwnerActor();
	if (!pOwner)
		return;
	
	m_pItemSystem->SetActorItem(pOwner, GetEntityId(), true);

	m_stats.used = true;

	ApplyViewLimit(userId, true);

	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	// TODO: precreate this table
	SmartScriptTable locker(gEnv->pScriptSystem);
	locker->SetValue("locker", ScriptHandle(GetEntityId()));
	locker->SetValue("lockId", ScriptHandle(GetEntityId()));
	locker->SetValue("lockIdx", 1);
	pOwner->GetGameObject()->SetExtensionParams("Interactor", locker);

	pOwner->LinkToMountedWeapon(GetEntityId());
	SAFE_HUD_FUNC(GetCrosshair()->SetUsability(0));

	if (IsServer())
		pOwner->GetGameObject()->InvokeRMI(CActor::ClStartUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);
}

//------------------------------------------------------------------------
void CItem::StopUse(EntityId userId)
{
	if (userId != m_owner.GetId())
		return;

	CActor *pActor = GetOwnerActor();
	if (!pActor)
		return;

	if (pActor->GetHealth()>0)
		pActor->SelectLastItem(true);

	pActor->GetAnimationGraphState()->SetInput("Action","idle");

	ApplyViewLimit(userId, false);
		
	EnableUpdate(false);

	m_stats.used = false;

	SetOwnerId(0);

	CloakEnable(false, pActor->GetActorSuitGameParameters().IsCloakEnabled());

	// TODO: precreate this table
	SmartScriptTable locker(gEnv->pScriptSystem);
	locker->SetValue("locker", ScriptHandle(GetEntityId()));
	locker->SetValue("lockId", ScriptHandle(0));
	locker->SetValue("lockIdx", 0);
	pActor->GetGameObject()->SetExtensionParams("Interactor", locker);

	pActor->LinkToMountedWeapon(0);

	if (IsServer())
		pActor->GetGameObject()->InvokeRMI(CActor::ClStopUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);
}

//------------------------------------------------------------------------
void CItem::ApplyViewLimit(EntityId userId, bool apply)
{
	CActor *pActor = GetActor(userId);
	if (!pActor)
		return;

	SActorParams *pParams = pActor->GetActorParams();

	if (apply)
	{
		SActorParams *pParams = pActor->GetActorParams();

		pParams->vLimitDir = m_stats.mount_dir;
		pParams->vLimitRangeH = DEG2RAD(m_properties.mounted_yaw_range);
		pParams->vLimitRangeVUp = DEG2RAD(m_properties.mounted_max_pitch);
		pParams->vLimitRangeVDown = DEG2RAD(m_properties.mounted_min_pitch);
		pParams->speedMultiplier = 0.0f;
	}
	else
	{
		pParams->vLimitDir.zero();
		pParams->vLimitRangeH = 0.0f;
		pParams->vLimitRangeV = 0.0f;
		pParams->vLimitRangeVUp = pParams->vLimitRangeVDown = 0.0f;
		pParams->speedMultiplier = 1.0f;
	}

}

//------------------------------------------------------------------------
void CItem::UseManualBlending(bool enable)
{
  IActor* pActor = GetOwnerActor();
  if (!pActor)
    return;

  if (ICharacterInstance* pCharInstance = pActor->GetEntity()->GetCharacter(0))
  {
    if (ISkeletonAnim* pSkeletonAnim = pCharInstance->GetISkeletonAnim())
    { 
      pSkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, 0.f, enable);
    }        
  } 
}

//------------------------------------------------------------------------
bool CItem::AttachToHand(bool attach, bool checkAttachment)
{
	//Skip mounted
	if (m_stats.mounted)
    return false;

	IEntity *pOwner = GetOwner();
	if (!pOwner)
		return false;

	ICharacterInstance *pOwnerCharacter = pOwner->GetCharacter(0);
	if (!pOwnerCharacter)
		return false;

	IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.attachment[m_stats.hand].c_str());

	if (!pAttachment)
	{
		GameWarning("Item owner '%s' doesn't have third-person item attachment point '%s'!", pOwner->GetName(), m_sharedparams->params.attachment[m_stats.hand].c_str());
		return false;
	}

	if (!attach)
	{
		if(checkAttachment)
		{
			if(IAttachmentObject *pAO = pAttachment->GetIAttachmentObject())
			{
				//The item can only be detached by itself
				if(pAO->GetAttachmentType()==IAttachmentObject::eAttachment_Entity)
				{
					CEntityAttachment* pEA = static_cast<CEntityAttachment*>(pAO);
					if(pEA->GetEntityId()!=GetEntityId())
						return false;
				}
			}
		}

		pAttachment->ClearBinding();

		EnableEntitySystemFPCharacterUpdate(true);
	}
	else
	{
		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());

		pAttachment->AddBinding(pEntityAttachment);
		pAttachment->HideAttachment(0);

		EnableEntitySystemFPCharacterUpdate(false);
	}

	return true;
}

//------------------------------------------------------------------------
bool CItem::AttachToBack(bool attach)
{
	if (gEnv->bMultiplayer || !m_sharedparams->params.attach_to_back)
		return false;

	IEntity *pOwner = GetOwner();
	if (!pOwner)
		return false;

	CActor* pActor = GetOwnerActor();
	CWeaponAttachmentManager* pWAM = pActor?pActor->GetWeaponAttachmentManager():NULL;

	//Do not attach on drop
	if(attach && m_stats.dropped)
		return false;

	if(!pWAM)
		return false;

	if(SActorStats* pStats = pActor->GetActorStats())
		if(pStats->isGrabbed && attach)
			return false;

	ICharacterInstance *pOwnerCharacter = pOwner->GetCharacter(0);
	if (!pOwnerCharacter)
		return false;

	IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = NULL;

	bool isCloaked = pActor->GetReplacementMaterial() != 0;

	CloakSync(false);

	if(attach)
	{
		pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.bone_attachment_01.c_str());
		m_stats.backAttachment = eIBA_Primary;
		if(pAttachment && pAttachment->GetIAttachmentObject())
		{
			pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.bone_attachment_02.c_str());
			m_stats.backAttachment = eIBA_Secondary;
		}
	}
	else if(m_stats.backAttachment == eIBA_Primary)
	{
		pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.bone_attachment_01.c_str());
	}
	else if(m_stats.backAttachment == eIBA_Secondary)
	{
		pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.bone_attachment_02.c_str());
	}
	else
	{
		return false;
	}
	
	if (!pAttachment)
	{
		if(m_stats.backAttachment == eIBA_Primary)
			GameWarning("Item owner '%s' doesn't have third-person item attachment point '%s'!", pOwner->GetName(), m_sharedparams->params.bone_attachment_01.c_str());
		else
			GameWarning("Item owner '%s' doesn't have third-person item attachment point '%s'!", pOwner->GetName(), m_sharedparams->params.bone_attachment_02.c_str());

		m_stats.backAttachment = eIBA_Unknown;
		return false;
	}

	if (!attach)
	{
		eItemBackAttachment temp = m_stats.backAttachment;
		m_stats.backAttachment = eIBA_Unknown;
		if(IAttachmentObject *pAO = pAttachment->GetIAttachmentObject())
		{
			//The item can only be detached by itself
			if(pAO->GetAttachmentType()==IAttachmentObject::eAttachment_Entity)
			{
				CEntityAttachment* pEA = static_cast<CEntityAttachment*>(pAO);
				if(pEA->GetEntityId()!=GetEntityId())
					return false;
			}
		}
		pAttachment->ClearBinding();
		if(temp == eIBA_Primary)
			pWAM->SetWeaponAttachment(false,m_sharedparams->params.bone_attachment_01.c_str(),GetEntityId());
		else
			pWAM->SetWeaponAttachment(false,m_sharedparams->params.bone_attachment_02.c_str(),GetEntityId());	
	}
	else
	{
		if(GetOwnerActor() && GetOwnerActor()->IsPlayer())
		{
			SetViewMode(0);
			Hide(true);

			if (IStatObj *pStatObj=GetEntity()->GetStatObj(eIGS_ThirdPerson))
			{
				CCGFAttachment *pCGFAttachment = new CCGFAttachment();
				pCGFAttachment->pObj = pStatObj;

				pAttachment->AddBinding(pCGFAttachment);
				pAttachment->HideAttachment(1);
				if (isCloaked)
					pAttachment->GetIAttachmentObject()->SetMaterial(pActor->GetReplacementMaterial());
				else
					pAttachment->HideInShadow(0);
			}
		}
		else
		{
			SetViewMode(eIVM_ThirdPerson);

			CEntityAttachment *pEntityAttachment = new CEntityAttachment();
			pEntityAttachment->SetEntityId(GetEntityId());

			pAttachment->AddBinding(pEntityAttachment);
			pAttachment->HideAttachment(0);

			//After QS/QL, hide if owner is hidden
			if(pOwner->IsHidden())
				Hide(true);
			else 
				Hide(false);
		}
		if(m_stats.backAttachment == eIBA_Primary)
			pWAM->SetWeaponAttachment(true,m_sharedparams->params.bone_attachment_01.c_str(),GetEntityId());
		else
			pWAM->SetWeaponAttachment(true,m_sharedparams->params.bone_attachment_02.c_str(),GetEntityId());	
	}

	return true;
}

//------------------------------------------------------------------------
void CItem::RequireUpdate(int slot)
{
	if (slot==-1)
		for (int i=0;i<4;i++)
			GetGameObject()->ForceUpdateExtension(this, i);	
	else
		GetGameObject()->ForceUpdateExtension(this, slot);
}

//------------------------------------------------------------------------
void CItem::EnableUpdate(bool enable, int slot)
{
	if (enable)
	{
		if (slot==-1)
			for (int i=0;i<4;i++)
				GetGameObject()->EnableUpdateSlot(this, i);
		else
			GetGameObject()->EnableUpdateSlot(this, slot);

	}
	else
	{
		if (slot==-1)
		{
			for (int i=0;i<4;i++)
				GetGameObject()->DisableUpdateSlot(this, i);
		}
		else
			GetGameObject()->DisableUpdateSlot(this, slot);
	}
}

//------------------------------------------------------------------------
void CItem::Hide(bool hide, bool remoteUpdate)
{
	if ((hide && m_stats.fp) || IsServer() || remoteUpdate)
	{
		GetEntity()->SetFlags(GetEntity()->GetFlags()|ENTITY_FLAG_UPDATE_HIDDEN);
	}
	else
	{
		GetEntity()->SetFlags(GetEntity()->GetFlags()&~ENTITY_FLAG_UPDATE_HIDDEN);
	}

	GetEntity()->Hide(hide);
}

//------------------------------------------------------------------------
void CItem::HideArms(bool hide)
{
	HideCharacterAttachment(eIGS_FirstPerson, ITEM_ARMS_ATTACHMENT_NAME, hide);
}

//------------------------------------------------------------------------
void CItem::HideItem(bool hide)
{
	HideCharacterAttachmentMaster(eIGS_FirstPerson, ITEM_ARMS_ATTACHMENT_NAME, hide);

	IEntity *pOwner = GetOwner();
	if (!pOwner)
		return;

	ICharacterInstance *pOwnerCharacter = pOwner->GetCharacter(0);
	if (!pOwnerCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.attachment[m_stats.hand].c_str());
	if (pAttachment)
		pAttachment->HideAttachment(hide?1:0);
}

//------------------------------------------------------------------------
void CItem::Cloak(bool cloak, IMaterial *cloakMat)
{
	if (cloak == m_cloaked || IsMounted())
		return;

	// check if the actor is cloaked
	CActor* pOwner = GetOwnerActor();
	if (!pOwner)
		return;
	if (!pOwner->GetEntity())
		return;

	IEntityRenderProxy * pOwnerRP = (IEntityRenderProxy*) pOwner->GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
	IEntityRenderProxy * pItemRP = (IEntityRenderProxy*) GetEntity()->GetProxy(ENTITY_PROXY_RENDER);

	//if (pItemRP && pOwnerRP)
	//{ 
	//	pItemRP->SetMaterialLayersMask( pOwnerRP->GetMaterialLayersMask() );
	//	pItemRP->SetMaterialLayersBlend( pOwnerRP->GetMaterialLayersBlend() );
	//}

	for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
	{
		if (CItem *pAccessory=static_cast<CItem *>(m_pItemSystem->GetItem(it->second)))
			pAccessory->Cloak(cloak, cloakMat);
	}

/*
	if(cloak)	//when switching view there are some errors without this check
	{
    CActor* pActor = GetOwnerActor();
    if (pActor && pActor->GetActorClass() == CPlayer::GetActorClassType())
    {
      CPlayer *plr = (CPlayer *)pActor;
      if(plr->GetNanoSuit() && plr->GetNanoSuit()->GetMode() != NANOMODE_CLOAK)
			{
				if (CItem *pSlave=static_cast<CItem *>(GetDualWieldSlave()))
					pSlave->Cloak(cloak, cloakMat);
        return;
			}
    }
	}
*/

	SEntitySlotInfo slotInfo;
	

	if (GetEntity()->GetSlotInfo(eIGS_FirstPerson, slotInfo))
	{
		if (slotInfo.pCharacter)
			SetMaterialRecursive(slotInfo.pCharacter, !cloak, NULL);
	}

	slotInfo=SEntitySlotInfo();
	if (GetEntity()->GetSlotInfo(eIGS_ThirdPerson, slotInfo))
	{
		if (slotInfo.pStatObj)
			GetEntity()->SetSlotMaterial(eIGS_ThirdPerson, NULL);
		else if (slotInfo.pCharacter)
			SetMaterialRecursive(slotInfo.pCharacter, !cloak, NULL);
	}

	if (CItem *pSlave=static_cast<CItem *>(GetDualWieldSlave()))
		pSlave->Cloak(cloak, cloakMat);

	m_cloaked = cloak;
}

void CItem::CloakEnable(bool enable, bool fade)
{
	IEntityRenderProxy * pItemRP = (IEntityRenderProxy*) GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
	if (pItemRP)
	{
		uint8 mask = pItemRP->GetMaterialLayersMask();
		uint32 blend = pItemRP->GetMaterialLayersBlend();
		if (!fade)
		{
			blend = (blend & 0xffff00ff) | (enable?0xff00 : 0x0000);
		}
		else
		{
			blend = (blend & 0xffff00ff) | (enable?0x0000 : 0xff00);
		}

		if (enable)
			mask = mask|MTL_LAYER_CLOAK;
		else
			mask = mask&~MTL_LAYER_CLOAK;
		ApplyMaterialLayerSettings(mask, blend);
	}
	m_cloaked = enable;
}

void CItem::CloakSync(bool fade)
{
	// check if the owner is cloaked
	IEntity* pOwner = GetOwner();

	if(!pOwner)
		return;

	IEntityRenderProxy* pOwnerRP = (IEntityRenderProxy*)pOwner->GetProxy(ENTITY_PROXY_RENDER);
	if (pOwnerRP)
	{
		uint8 ownerMask = pOwnerRP->GetMaterialLayersMask();
		bool isCloaked = (ownerMask&MTL_LAYER_CLOAK) != 0;
		CloakEnable(isCloaked, fade);
	}

	if (CItem* pSlave=static_cast<CItem*>(GetDualWieldSlave()))
		pSlave->CloakSync(fade);
}

void CItem::ApplyMaterialLayerSettings(uint8 mask, uint32 blend)
{
	IEntityRenderProxy * pItemRP = (IEntityRenderProxy*) GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
	if (pItemRP)
	{ 
		pItemRP->SetMaterialLayersMask(mask);
		pItemRP->SetMaterialLayersBlend(blend);
	}

	for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); ++it)
	{
		EntityId cur = (EntityId)it->second;
		IItem *attachment = m_pItemSystem->GetItem(cur);
		if (attachment)
		{
			pItemRP = (IEntityRenderProxy*) attachment->GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
			if (pItemRP)
			{
				pItemRP->SetMaterialLayersMask(mask);
				pItemRP->SetMaterialLayersBlend(blend);
			}
		}
	}
}


void CItem::SetMaterialRecursive(ICharacterInstance *charInst, bool undo, IMaterial *newMat)
{
	if (!charInst || (!undo && !newMat))
		return;

	if (undo)
		charInst->SetMaterial(NULL);
	else
	{
		IMaterial *oldMat = charInst->GetMaterial();
		if (newMat != oldMat)
		{
			charInst->SetMaterial(newMat);
		}
	}
	for (int i = 0; i < charInst->GetIAttachmentManager()->GetAttachmentCount(); i++)
	{
		IAttachment *attch = charInst->GetIAttachmentManager()->GetInterfaceByIndex(i);
		if (attch)
		{
			IAttachmentObject *obj = attch->GetIAttachmentObject();
			if (obj)
			{
				SetMaterialRecursive(obj->GetICharacterInstance(), undo, newMat);
				if (!obj->GetICharacterInstance())
				{
					if (undo)
						obj->SetMaterial(NULL);
					else
					{
						IMaterial *oldMat = obj->GetMaterial();
						if (oldMat != newMat)
						{
							obj->SetMaterial(newMat);
						}
					}
				}
			}
		}
	}
}

void CItem::TakeAccessories(EntityId receiver)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(receiver);
	IInventory *pInventory = GetActorInventory(pActor);

	if (pInventory)
	{
		for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); ++it)
		{
			const char* name = it->first;
			if (!pInventory->GetCountOfClass(name))
				g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, name, false, false, false);
		}
	}
}

void CItem::AddAccessoryAmmoToInventory(IEntityClass* pAmmoType, int count, CActor *pOwner /*= NULL */)
{
	if(!pOwner)
		pOwner = GetOwnerActor();
	IInventory *pInventory=GetActorInventory(pOwner);
	if (!pInventory)
		return;

	int capacity = pInventory->GetAmmoCapacity(pAmmoType);
	int current = pInventory->GetAmmoCount(pAmmoType);
	if((!gEnv->IsEditor()) && ((current+count) > capacity))
	{
		if(pOwner->IsClient())
			SAFE_HUD_FUNC(DisplayFlashMessage("@ammo_maxed_out", 2, ColorF(1.0f, 0,0), true, (string("@")+pAmmoType->GetName()).c_str()));

		//If still there's some place, full inventory to maximum...
		if(current<capacity)
		{
			pInventory->SetAmmoCount(pAmmoType,capacity);
		}
	}
	else
	{
		int newCount = current+count;
		pInventory->SetAmmoCount(pAmmoType, newCount);
	}
}

int CItem::GetAccessoryAmmoCount(IEntityClass* pAmmoType)
{
	IInventory *pInventory=GetActorInventory(GetOwnerActor());
	if (!pInventory)
		return 0;

	return pInventory->GetAmmoCount(pAmmoType);
}

bool CItem::CheckAmmoRestrictions(EntityId pickerId)
{
	IActor* pPicker = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pickerId);
	if(pPicker)
	{
		IInventory *pInventory = pPicker->GetInventory();
		if(pInventory)
		{
			if(pInventory->GetCountOfClass(GetEntity()->GetClass()->GetName()) == 0)
				return true;

			if(m_accessoryAmmoAvailable && !m_sharedparams->bonusAccessoryAmmo.empty())
			{
				for (TAccessoryAmmoMap::const_iterator it = m_sharedparams->bonusAccessoryAmmo.begin(); it != m_sharedparams->bonusAccessoryAmmo.end(); ++it)
				{
					int invAmmo  = pInventory->GetAmmoCount(it->first);
					int invLimit = pInventory->GetAmmoCapacity(it->first);

					if(invAmmo>=invLimit)
						return false;
				}
			}
		}
	}

	return true;
}

void CItem::GetMemoryUsage(ICrySizer *s) const
{	
	s->AddObject(this, sizeof(*this));
	GetInternalMemoryUsage(s);
}

void CItem::GetInternalMemoryUsage(ICrySizer *s) const
{
	m_camerastats.GetMemoryStatistics(s);
	m_properties.GetMemoryStatistics(s);
	s->AddObject(m_effects);
	s->AddObject(m_activelayers);
	s->AddObject(m_accessories);
	s->AddObject(m_initialSetup);
	s->AddObject(m_instanceActions);
	m_scheduler.GetMemoryStatistics(s);
	s->AddObject(m_actionSuffix);

}


SItemStrings* g_pItemStrings = 0;

SItemStrings::~SItemStrings()
{
	g_pItemStrings = 0;
}

SItemStrings::SItemStrings()
{
	g_pItemStrings = this;

	AmbienceSound = "AmbienceSound";
	airFrictionOn = "airFrictionOn";
	activate = "activate";
	begin_reload = "begin_reload";
	cannon = "cannon";
	change_firemode = "change_firemode";
	change_firemode_zoomed = "change_firemode_zoomed";
	deactivate = "deactivate";
	deselect = "deselect";
	destroy = "destroy";
	drop = "drop";
	enter_modify = "enter_modify";
	exit_reload_nopump = "exit_reload_nopump";
	exit_reload_pump = "exit_reload_pump";
	fire = "fire";
	idle = "idle";
	idle_break = "idle_break";
	idle_lastGrenade = "idle_lastGrenade";
	idle_raised = "idle_raised";
	leave_modify = "leave_modify";
	left_item_attachment = "left_weapon";
	lock = "lock";
	lower = "lower";
	meleeReaction = "meleeReaction";
	modify_layer = "modify_layer";
	nw = "nw";
	pickedup = "pickedup";
	pickedup_ammo = "pickedup_ammo";
	raise = "raise";
	reload_shell = "reload_shell";
	rip_off = "rip_off";
	right_item_attachment = "weapon";
	rotate_mounted = "rotate_mounted";
	turret = "turret";  
  enable_light = "enable_light";
  disable_light = "disable_light";
  use_light = "use_light";
	first_select = "first_select";
	LAM = "LAM";
	LAMRifle = "LAMRifle";
	LAMFlashLight = "LAMFlashLight";
	LAMRifleFlashLight = "LAMRifleFlashLight";
	Silencer = "Silencer";
	SOCOMSilencer = "SOCOMSilencer";
	lever_layer_1 = "lever_layer_1";
	lever_layer_2 = "lever_layer_2";

};

void CItem::DeselectItem( EntityId itemId )
{
	IItem *pItem = m_pItemSystem->GetItem(itemId);
	if (pItem)
	{
		if (pItem->GetOwnerId())
		{
			CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pItem->GetOwnerId()));
			if (pActor)
			{
				if (pActor->GetInventory()->GetCurrentItem() == pItem->GetEntityId())
				{
					pActor->SelectNextItem(1, false, m_pItemSystem->GetItemCategory(pItem->GetEntity()->GetClass()->GetName()));
				}
			}
		}
	}
}

void CItem::ResetActionSuffix(float blendTime) 
{ 
	m_actionSuffix = GetParams().suffix.c_str(); 
	m_actionSuffixAG = GetParams().suffixAG.c_str(); 
	m_actionSuffixBlendTime = blendTime;
};
 
const SMountParams* CItem::GetMountedParams() const
{
	return m_sharedparams->pMountParams;
}

void CItem::EnableEntitySystemFPCharacterUpdate( bool enable )
{
/*
	ICharacterInstance* pFirstPersonWeaponCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);

	if (pFirstPersonWeaponCharacter != NULL)
	{
		const int flags = enable ? (pFirstPersonWeaponCharacter->GetFlags() | CS_FLAG_UPDATE) : (pFirstPersonWeaponCharacter->GetFlags() & ~CS_FLAG_UPDATE);

		pFirstPersonWeaponCharacter->SetFlags(flags);
	}*/

}

ColorF CItem::GetSilhouetteColor() const
{
	return ColorF(0.6015625f, 0.83203125f, 0.71484375f, 1.0f);
}
