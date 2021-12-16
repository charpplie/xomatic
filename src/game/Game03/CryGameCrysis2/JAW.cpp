/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Class for specific JAW rocket launcher functionality
-------------------------------------------------------------------------
History:
- 22:06:2007: Created by Benito G.R.
- 30:10:2009: Ported from RocketLauncher

*************************************************************************/

#include "StdAfx.h"
#include "JAW.h"

#include "GameActions.h"
#include "Actor.h"
#include "GameRules.h"
#include "GameCVars.h"
#include "Player.h"
#include "Single.h"

#include "ItemSharedParams.h"
#include "WeaponSharedParams.h"

namespace
{
	TActionHandler<CJaw>	g_actionHandler;

	const float hideTimeAfterDrop = 20.0f;
	const float autoDropDelayTime = 1.5f;
	const char* smokeEffectHelper = "smoke_effect";
	const char* smokeEffectName = "Crysis2_weapon_jaw.gun_smoke";
}



struct CJaw::DropAction
{
	DropAction(CActor* _pWeaponOwner, EntityId _weaponId, bool _selectNext)
		:	pWeaponOwner(_pWeaponOwner), weaponId(_weaponId), selectNext(_selectNext) {};

	CActor* pWeaponOwner;
	EntityId weaponId;
	bool selectNext;

	void execute(CItem *_this)
	{
		pWeaponOwner->DropItem(weaponId, 2.0f, selectNext, false);
	}
};




CJaw::CJaw()
{
	m_auxSlotUsed = m_auxSlotUsedBQS = false;
	m_firedRockets = 0;
	m_autoDropping = false;
	m_controllingRocket = false;
	m_autoDropPendingTimer = 0.0f;
	m_zoomTriggerDown = false;
	m_fireTriggerDown = false;
	m_firePending = false;
	m_dropped = false;
	m_dropTime = 0.0f;
	m_smokeActive = false;

	if (g_actionHandler.GetNumHandlers() == 0)
	{
#		define ADD_HANDLER(action, func) g_actionHandler.AddHandler(actions.action, &CJaw::func)
		const CGameActions& actions = g_pGame->Actions();

		ADD_HANDLER(zoom, OnActionJawZoom);
		ADD_HANDLER(attack2_xi, OnActionJawZoom);

#		undef ADD_HANDLER
	}
}



void CJaw::Update(SEntityUpdateContext& ctx, int slot)
{
	CWeapon::Update(ctx, slot);

	if (slot != eIUS_General)
		return;

	UpdatePendingShot();

	if (!m_fireTriggerDown)
		AutoZoomOut();

	if (m_autoDropping)
	{
		m_autoDropPendingTimer += ctx.fFrameTime;
		if (CanAutoDrop())
		{
			m_autoDropping = false;
			DoAutoDrop();
		}
	}

	if (m_dropped)
	{
		m_dropTime += ctx.fFrameTime;
		if (m_dropTime > hideTimeAfterDrop)
		{
			Hide(true);
			EnableUpdate(false);
			ReleaseSmokeEffect();
		}
		else
		{
			RequireUpdate();
		}
	}
}



//////////////////////////////////////////////////////////////////////////
void CJaw::OnReset()
{
	if(m_stats.backAttachment==eIBA_Unknown)
		m_auxSlotUsed = true;

	BaseClass::OnReset();

	ReleaseSmokeEffect();

	if(m_stats.backAttachment==eIBA_Unknown)
	{
		SetViewMode(0);
		DrawSlot(eIGS_ThirdPersonAux,true);
	}
	else
		m_stats.first_selection = false;

	m_firedRockets = 0;
	m_dropped = false;
	m_dropTime = 0.0f;
	m_zoomTriggerDown = false;
	m_fireTriggerDown = false;
	m_firePending = false;
	Hide(false);

	SetFiringLocator(this);

	m_listeners.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CJaw::ProcessEvent(SEntityEvent &event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	BaseClass::ProcessEvent(event);

	if(event.event == ENTITY_EVENT_RESET)
	{
		//Exiting game mode
		if(gEnv->IsEditor() && !event.nParam[0])
		{
			if(!GetOwner())
			{
				DrawSlot(eIGS_ThirdPerson,false);
				DrawSlot(eIGS_ThirdPersonAux,true);
				m_auxSlotUsed = true;
			}
		}
	}	
}

//////////////////////////////////////////////////////////////////////////
bool CJaw::SetAspectProfile(EEntityAspects aspect, uint8 profile)
{
	if(aspect!=eEA_Physics)
		return BaseClass::SetAspectProfile(aspect, profile);

	bool ok = false;
	if(!gEnv->bMultiplayer && gEnv->pSystem->IsSerializingFile() && m_auxSlotUsedBQS)
		ok = true;

	int slot = (m_auxSlotUsed||ok)?eIGS_ThirdPersonAux:eIGS_ThirdPerson;

	if (aspect == eEA_Physics)
	{
		switch (profile)
		{
		case eIPhys_PhysicalizedStatic:
			{
				SEntityPhysicalizeParams params;
				params.type = PE_STATIC;
				params.nSlot = slot;

				GetEntity()->Physicalize(params);

				return true;
			}
			break;
		case eIPhys_PhysicalizedRigid:
			{
				SEntityPhysicalizeParams params;
				params.type = PE_RIGID;
				params.nSlot = slot;
				params.mass = m_sharedparams->params.mass;

				pe_params_buoyancy buoyancy;
				buoyancy.waterDamping = 1.5;
				buoyancy.waterResistance = 1000;
				buoyancy.waterDensity = 0;
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
					params.nSlot = slot;
					pPhysicsProxy->Physicalize(params);
				}
			}
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CJaw::Select(bool select)
{
	if(select && m_auxSlotUsed)
	{
		DrawSlot(eIGS_ThirdPersonAux,false);
		m_auxSlotUsed = false;
	}

	BaseClass::Select(select);
}

//////////////////////////////////////////////////////////////////////////
void CJaw::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char* setup)
{

	BaseClass::PickUp(pickerId,sound,select,keepHistory, setup);

	if(m_auxSlotUsed)
	{
		DrawSlot(eIGS_ThirdPersonAux,false);
		m_auxSlotUsed = false;
	}

	if(GetOwnerActor() && !GetOwnerActor()->IsPlayer())
		m_stats.first_selection = false;
}

//////////////////////////////////////////////////////////////////////////
void CJaw::FullSerialize(TSerialize ser)
{
	BaseClass::FullSerialize(ser);

	ser.Value("auxSlotUsed", m_auxSlotUsed);
	ser.Value("smokeActive", m_smokeActive);

	if (!ser.IsReading())
	{
		m_auxSlotUsedBQS = m_auxSlotUsed;
	}
}

//////////////////////////////////////////////////////////////////////////
void CJaw::PostSerialize()
{
	BaseClass::PostSerialize();

	if(m_auxSlotUsed)
	{
		SetViewMode(0);
		DrawSlot(eIGS_ThirdPersonAux,true);
	}
	else
		DrawSlot(eIGS_ThirdPersonAux,false);

	if (m_smokeActive)
		Pickalize(false, true);
}


//////////////////////////////////////////////////////////////////////////
void CJaw::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	CActor* pOwner = GetOwnerActor();
	//Don't let the player drop it if it has not been opened
	if(m_stats.first_selection && pOwner && pOwner->IsPlayer() && pOwner->GetHealth()>0 && !byDeath)
		return;

	if(pOwner && !pOwner->IsPlayer())
	{
		//In this case goes to the clip, no the inventory

		if(m_minDropAmmoAvailable && !m_weaponsharedparams->ammoParams.minDroppedAmmo.empty())
		{
			TAmmoVector::const_iterator end = m_weaponsharedparams->ammoParams.minDroppedAmmo.end();

			for(TAmmoVector::const_iterator it = m_weaponsharedparams->ammoParams.minDroppedAmmo.begin(); it != end; it++)
			{
				SWeaponAmmoUtils::SetAmmo(m_ammo, it->pAmmoClass, it->count);
			}

			m_minDropAmmoAvailable = false;
		}
	}

	BaseClass::Drop(impulseScale,selectNext,byDeath);
	if (pOwner && !selectNext)
	{
		IInventory* pInventory = pOwner->GetInventory();
		EntityId pNextJawItem = pInventory->GetItemByClass(GetEntity()->GetClass());
		m_pItemSystem->SetActorItem(pOwner, pNextJawItem, true);
	}

	if (!gEnv->pSystem->IsEditorMode())
	{
		m_dropped = true;
		EnableUpdate(true);
		Pickalize(false,true);
		CreateSmokeEffect();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CJaw::CanPickUp(EntityId userId) const
{
	CActor *pActor = GetActor(userId);
	IInventory *pInventory=GetActorInventory(pActor);

	if (m_sharedparams->params.pickable && m_stats.pickable && !m_stats.flying && (!m_owner.GetId() || m_owner.GetId()==userId) && !m_stats.selected && !GetEntity()->IsHidden())
	{
		if (pInventory && pInventory->FindItem(GetEntityId())!=-1)
			return false;
	}
	else
		return false;

	uint8 uniqueId = m_pItemSystem->GetItemUniqueId(GetEntity()->GetClass()->GetName());

	if(pInventory && (pInventory->GetCountOfUniqueId(uniqueId)>=3))
	{
		if(pActor->IsClient())
			g_pGame->GetGameRules()->OnTextMessage(eTextMessageCenter, "@mp_CannotCarryMoreLAW");
		return false;
	}

	return true;
		
}

//////////////////////////////////////////////////////////////////////////
void CJaw::GetAttachmentsAtHelper(const char *helper, CCryFixedStringListT<5, 30> &attachments)
{
	//Do nothing...
	//Rocket launcher has an special helper for the scope, but it must be skipped by the HUD
}

//////////////////////////////////////////////////////////////////////////
void CJaw::AutoDrop()
{
	m_autoDropping = true;
	m_controllingRocket = false;
	m_autoDropPendingTimer = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void CJaw::OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel)
{
	BaseClass::OnShoot(shooterId, ammoId, pAmmoType, pos, dir, vel);

	SvActivateMissileCountermeasures(shooterId, pos, dir);	//Rocket launchers can trigger missile countermeasures perk
}

//////////////////////////////////////////////////////////////////////////
void CJaw::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	CWeapon::OnAction(actorId, actionId, activationMode, value);
	g_actionHandler.Dispatch(this,actorId,actionId,activationMode,value);
}

//////////////////////////////////////////////////////////////////////////
void CJaw::SvActivateMissileCountermeasures(EntityId shooterId, const Vec3 &pos, const Vec3 &dir)
{
	if(gEnv->bServer)
	{
		float fov = sinf(DEG2RAD(CPerk::GetInstance()->GetVars()->perk_chaffFOV));
		IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		while (CActor* pActor = (CActor*)pIter->Next())
		{
			if(pActor->IsPlayer() && pActor->GetEntityId() != shooterId)
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
				if (pPlayer->IsPerkActive(ePerk_MissileCountermeasures))
				{
					AABB playerAABB;
					pPlayer->GetEntity()->GetWorldBounds(playerAABB);
					Vec3 centre = playerAABB.GetCenter() - pos;
					float radius = playerAABB.GetRadius();
					float dist = centre.Dot(dir);
					if (dist>0.0f)
					{
						float maxrange = (dist*fov) + radius;
						centre -= dist*dir;
						if (centre.len2() < maxrange*maxrange)
						{
							pPlayer->SendPerkEvent(EPE_SvActivateMissileCountermeasures);
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CJaw::StartFire(const Vec3 *fireTarget)
{
	if (!CanFire())
		return;

	CActor* pOwner = GetOwnerActor();
	if (pOwner && !pOwner->IsPlayer())
	{
		BaseClass::StartFire();
		return;
	}

	if (m_controllingRocket)
		return;

	if(!IsZoomingInOrOut())
	{
		if (m_zm)
			m_zm->ZoomIn();
	}

	m_fireTriggerDown = true;
	m_firePending = true;
}

//////////////////////////////////////////////////////////////////////////
void CJaw::StopFire()
{
	if (!m_firePending)
		AutoZoomOut();

	BaseClass::StopFire();

	m_fireTriggerDown = false;
}

//////////////////////////////////////////////////////////////////////////
bool CJaw::CanFire() const
{
	if (m_firedRockets==0)
	{
		return BaseClass::CanFire();
	}
	
	return false;
}



bool CJaw::CanAutoDrop()
{
	const float autoDropTimeDelay = autoDropDelayTime;
	if (m_autoDropPendingTimer < autoDropTimeDelay)
		return false;

	IZoomMode* pZoomMode = GetZoomMode(GetCurrentZoomMode());
	if (!pZoomMode)
		return true;
	return !(pZoomMode->IsZoomed() || pZoomMode->IsZoomingInOrOut());
}



void CJaw::DoAutoDrop()
{
	if(m_fm)
	{
		m_firedRockets--;

		CActor* pOwner = GetOwnerActor();
		// no need to auto-drop for AI
		if(pOwner && !pOwner->IsPlayer())
			return;

		if((GetAmmoCount(m_fm->GetAmmoType())<=0)&&(m_firedRockets<=0))
		{
			if( pOwner )
			{
				uint8 uniqueId = m_pItemSystem->GetItemUniqueId(GetEntity()->GetClass()->GetName());
				bool selectNext = pOwner->GetInventory()->GetCountOfUniqueId(uniqueId) <= 1;

				PlayAction(g_pItemStrings->drop);
				uint32 dropAnimationTime = GetCurrentAnimationTime(eIGS_Owner);
				m_scheduler.TimerAction(
					dropAnimationTime,
					CSchedulerAction<DropAction>::Create(DropAction(pOwner, GetEntityId(), selectNext)),
					false);
			}
			if(!(gEnv->bMultiplayer || gEnv->IsEditor()))
				g_pGame->GetGameRules()->ScheduleEntityRemoval(GetEntityId(), hideTimeAfterDrop, true);
		}
	}
}



void CJaw::HideRocket()
{
	HideCharacterAttachment(eIGS_FirstPerson, "rocket", true);
	HideCharacterAttachment(eIGS_ThirdPerson, "rocket", true);
}



bool CJaw::CanDeselect() const
{
	return !m_controllingRocket;
}



bool CJaw::OnActionJawZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	m_zoomTriggerDown = (activationMode == eAAM_OnPress || activationMode == eAAM_OnHold);
	return false;
}




bool CJaw::GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit)
{
	return false;
}



bool CJaw::GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos)
{
	int slot = eIGS_ThirdPerson;
	CActor* pOwnerActor = GetOwnerActor();
	if (pOwnerActor && !pOwnerActor->IsThirdPerson())
		slot = eIGS_FirstPerson;

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter || !pCharacter->IsCharacterVisible())
		return false;

	Matrix34 fireHelperLocation = GetCharacterAttachmentWorldTM(slot, "muzzleflash_effect");
	pos = fireHelperLocation.TransformPoint(Vec3(ZERO));
	return true;
}



bool CJaw::GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos)
{
	return false;
}



bool CJaw::GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos)
{
	return false;
}



bool CJaw::GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir)
{
	return false;
}



void CJaw::WeaponReleased()
{
}


void CJaw::CreateSmokeEffect()
{
	if (!m_smokeActive)
	{
		IAttachment* pAttachment = GetSmokeHelper();

		if (!pAttachment)
			return;

		SEntitySlotInfo slotInfo;
		GetEntity()->GetSlotInfo(eIGS_ThirdPerson, slotInfo);

		CEffectAttachment* pEffectAttachment = new CEffectAttachment(
			smokeEffectName,
			Vec3(0,0,0), Vec3(0,1,0), 1.0f);
		pEffectAttachment->CreateEffect(Matrix34(pAttachment->GetAttWorldAbsolute()));
		if (!pEffectAttachment->GetEmitter())
		{
			delete pEffectAttachment;
			return;
		}

		pAttachment->AddBinding(pEffectAttachment);
		m_smokeActive = true;
	}
}



void CJaw::ReleaseSmokeEffect()
{
	if (!m_smokeActive)
		return;

	IAttachment* pAttachment = GetSmokeHelper();
	if (pAttachment)
		pAttachment->ClearBinding();

	m_smokeActive = false;
}



struct IAttachment* CJaw::GetSmokeHelper() const
{
	SEntitySlotInfo slotInfo;
	GetEntity()->GetSlotInfo(eIGS_ThirdPerson, slotInfo);

	if (!slotInfo.pCharacter)
		return 0;

	ICharacterInstance* pCharacter = slotInfo.pCharacter;
	IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment* pAttachment = pAttachmentManager->GetInterfaceByName(smokeEffectHelper);

	return pAttachment;
}



float CJaw::GetMovementModifier() const
{
	if (!m_controllingRocket)
		return GetPlayerMovementModifiers().movementSpeedScale;
	else
		return GetPlayerMovementModifiers().firingMovementSpeedScale;
}



float CJaw::GetRotationModifier() const
{
	if (!m_controllingRocket)
		return GetPlayerMovementModifiers().rotationSpeedScale;
	else
		return GetPlayerMovementModifiers().firingRotationSpeedScale;
}



void CJaw::UpdatePendingShot()
{
	if (m_firePending && CanFire() && IsZoomed())
	{
		m_controllingRocket = true;
		m_firePending = false;
		HideRocket();
		BaseClass::StartFire();
	}
}

//////////////////////////////////////////////////////////////////////////
void CJaw::AutoZoomOut()
{
	if (!m_zoomTriggerDown)
		ExitZoom();
}

//////////////////////////////////////////////////////////////////////////
ColorF CJaw::GetSilhouetteColor() const
{
	return (!m_dropped) ? ColorF(0.89411f, 0.89411f, 0.10588f, 1.0f) : ColorF(0.89411f, 0.10588f, 0.10588f, 1.0f);
}