/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Mounted machine gun that can be ripped off by the player
and move around with it
-------------------------------------------------------------------------
History:
- 20:01:2009: Created by Benito G.R.
  30:09:2009: Ported from Rippable turret gun

*************************************************************************/

#include "StdAfx.h"
#include "HMG.h"
#include "GameActions.h"
#include "Game.h"
#include "Player.h"
#include "ScreenEffects.h"
#include "ItemSharedParams.h"
#include "GameRules.h"
#include "HUD/HUD.h"
#include "Battlechatter.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool s_ripOffPromptIsVisible = false;

static void DoRipOffPrompt(EntityId who, bool doIt)
{
	if (who == g_pGame->GetIGameFramework()->GetClientActorId() && doIt != s_ripOffPromptIsVisible)
	{
		SHUDEvent interactionEvent;
		interactionEvent.eventType = eHUDEvent_OnInteractionRequest;
		interactionEvent.ReserveData(4);

		if (doIt)
		{
			interactionEvent.AddData(SHUDEventData(true));
			interactionEvent.AddData(SHUDEventData(const_cast<char*>("@ui_interaction_ripoff")));
#if defined(PS3)
			interactionEvent.AddData(SHUDEventData(const_cast<char*>("PS3_R3")));
#else
			interactionEvent.AddData(SHUDEventData(const_cast<char*>("XBox360_RS")));
#endif
			interactionEvent.AddData(SHUDEventData(-1.0f));
		}
		else
		{
			interactionEvent.AddData(SHUDEventData(doIt));
			interactionEvent.AddData(SHUDEventData(NULL));
			interactionEvent.AddData(SHUDEventData(NULL));
			interactionEvent.AddData(SHUDEventData(-1.0f));
		}

		CHUD::CallEvent(interactionEvent);
		s_ripOffPromptIsVisible = doIt;
	}
}

struct CHMG::StopUseAction
{
	StopUseAction(CHMG *_weapon, EntityId _userId): pHMGWeapon(_weapon), userId(_userId) {};

	CHMG *pHMGWeapon;
	EntityId userId;

	void execute(CItem *_this)
	{
		pHMGWeapon->StopUse(userId);
	}
};

struct CHMG::EndRippingOff
{
	EndRippingOff(CHMG *_weapon): pHMGWeapon(_weapon){};

	CHMG *pHMGWeapon;

	void execute(CItem *_this)
	{
		pHMGWeapon->FinishRipOff();
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CHMG::CHMG():
m_rippedOff(false),
m_rippingOff(false),
m_linkedParentId(0),
m_currentSuitMode(eNanoSuitMode_Invalid),
m_rotatingSoundID(INVALID_SOUNDID),
m_framesToStopRotationSound(0),
m_lastZAngle(0)
{
	RegisterActions();
}


TActionHandler<CHMG>	CHMG::s_actionHandler;

void CHMG::RegisterActions()
{
	if (s_actionHandler.GetNumHandlers() == 0)
	{
#define ADD_HANDLER(action, func) s_actionHandler.AddHandler(actions.action, &CHMG::func)
		const CGameActions& actions = g_pGame->Actions();

		ADD_HANDLER(special,OnActionRipOff);
		ADD_HANDLER(weapon_change_firemode,OnActionFiremode);
#undef ADD_HANDLER
	}
}


void CHMG::OnReset()
{
	BaseClass::OnReset();

	m_rippedOff = m_rippingOff = false;
	m_stats.mounted = true;
	m_rotatingSoundID = INVALID_SOUNDID;

	//De-physicalize
	Physicalize(false, false);

	RequireUpdate(eIUS_General);

	if (m_linkedParentId != 0)
	{
		IEntity* pLinkedParent = gEnv->pEntitySystem->GetEntity(m_linkedParentId);
		if (pLinkedParent)
		{
			pLinkedParent->AttachChild(GetEntity());
		}
		m_linkedParentId = 0;
	}
}


void CHMG::UpdateFPView(float frameTime)
{
	BaseClass::UpdateFPView(frameTime);

	/*if(!gEnv->bMultiplayer)*/
	{
		if(!m_rippedOff && !m_rippingOff)
		{
			CActor* pOwner = GetOwnerActor();
			if(!pOwner)
				return;

			ENanoSuitMode currentMode = pOwner->GetActorSuitGameParameters().GetMode();
			if(m_currentSuitMode != currentMode)
			{
				m_currentSuitMode = currentMode;

				DoRipOffPrompt(GetOwnerId(), true);
			}
		}
	}
}


void CHMG::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(m_rippingOff)
		return;

	bool filtered = false;
	bool handled = s_actionHandler.Dispatch(this, actorId, actionId, activationMode, value, filtered);
	if(!handled || !filtered)
		BaseClass::OnAction(actorId, actionId, activationMode, value);
}


bool CHMG::OnActionRipOff(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	//TODO: This has to be synchronized for MP
	if((activationMode == eAAM_OnPress) && !m_rippedOff && !m_rippingOff)
	{
		TryRipOffGun();
		ClearInputFlag(eWeaponAction_Fire);
		return true;
	}

	return false;
}


bool CHMG::OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	//DO NOTHING... (filters command, and doesn't allow to switch fire modes

	return true;
}


void CHMG::Use(EntityId userId)
{
	if (!m_owner.GetId())
		StartUse(userId);
	else if (m_owner.GetId() == userId)
	{
		if (m_rippedOff)
		{
			StartDeselection();
			uint32 deselectionTime = GetCurrentAnimationTime(eIGS_Owner);
			m_scheduler.TimerAction(deselectionTime, CSchedulerAction<StopUseAction>::Create(StopUseAction(this, userId)), false);
		}
		else
		{
			StopUse(userId);
		}
	}
}


bool CHMG::CanUse(EntityId userId) const
{
	if(m_rippedOff)
	{
		EntityId ownerId = m_owner.GetId();
		if(ownerId == 0 || ownerId == userId)
			return true;
	}

	return BaseClass::CanUse(userId);
}


void CHMG::StartUse(EntityId userId)
{
	m_currentSuitMode = eNanoSuitMode_Invalid;

	if(m_rippedOff)
	{
		// holster user item here
		SetOwnerId(userId);

		SetUnMountedConfiguration();

		CActor* pOwner = GetOwnerActor();
		if (!pOwner)
			return;

		Physicalize(false, false);

		m_pItemSystem->SetActorItem(pOwner, GetEntityId(), true);

		m_stats.used = true;

		EnableUpdate(true, eIUS_General);
		RequireUpdate(eIUS_General);

		// TODO: precreate this table
		SmartScriptTable locker(gEnv->pScriptSystem);
		locker->SetValue("locker", ScriptHandle(GetEntityId()));
		locker->SetValue("lockId", ScriptHandle(GetEntityId()));
		locker->SetValue("lockIdx", 1);
		pOwner->GetGameObject()->SetExtensionParams("Interactor", locker);

		SAFE_HUD_FUNC(GetCrosshair()->SetUsability(0));

		if (IsServer())
		{
			pOwner->GetGameObject()->InvokeRMI(CActor::ClStartUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);
			g_pGame->GetGameRules()->AbortEntityRemoval(GetEntityId());
		}

	}
	else
	{
		BaseClass::StartUse(userId);
		PlayAction(g_pItemStrings->idle);

		if (IsOwnerFP())
		{
			DrawSlot(eIGS_FirstPerson, true, true);
		}
	}

	CActor* pOwner = GetOwnerActor();
	if (pOwner)
	{
		CHANGED_NETWORK_STATE(pOwner, CPlayer::ASPECT_CURRENT_ITEM);
	}
}


void CHMG::StopUse(EntityId userId)
{
	if(m_rippedOff)
	{
		CActor *pActor = GetOwnerActor();
		if (!pActor)
			return;

		if (pActor->GetHealth()>0)
			pActor->SelectLastItem(true);

		pActor->GetAnimationGraphState()->SetInput("Action","idle");

		EnableUpdate(false);

		m_stats.used = false;

		//SetOwnerId(0);

		CloakEnable(false, pActor->GetActorSuitGameParameters().IsCloakEnabled());

		// TODO: precreate this table
		SmartScriptTable locker(gEnv->pScriptSystem);
		locker->SetValue("locker", ScriptHandle(GetEntityId()));
		locker->SetValue("lockId", ScriptHandle(0));
		locker->SetValue("lockIdx", 0);
		pActor->GetGameObject()->SetExtensionParams("Interactor", locker);

		if (IsServer())
			pActor->GetGameObject()->InvokeRMI(CActor::ClStopUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);

		BaseClass::Drop(5.0f);

/*
		if(OutOfAmmo(false))
		{
			ApplySurfaceFX("Crysis2_NanoSuitFX.Cloak.sparks_small");
		}*/

	}
	else
	{
		if (m_isFiring)
			StopFire();
		DoRipOffPrompt(GetOwnerId(), false);
		SetViewMode(eIVM_ThirdPerson);
		DrawSlot(eIGS_ThirdPerson, true);
		BaseClass::StopUse(userId);
	}

	m_currentSuitMode = eNanoSuitMode_Invalid;
}


void CHMG::TryRipOffGun()
{
	CActor *pActor = GetOwnerActor();
	if(!pActor)
		return;

	PerformRipOff(pActor);
	
	if(gEnv->bServer)
	{
		CHANGED_NETWORK_STATE(this, ASPECT_RIPOFF);
	}
	else
	{
		GetGameObject()->InvokeRMI(SvRequestRipOff(), EmptyParams(), eRMI_ToServer);
	}
}

void CHMG::PerformRipOff(CActor* pOwner)
{
	UnlinkMountedGun();
	AttachToHand(true);
	StopFire();

	PlayAction(g_pItemStrings->rip_off, 0, false, eIPAF_Default | eIPAF_CleanBlending);
	m_rippingOff = true;

	DoRipOffPrompt(GetOwnerId(), false);

	int timeDelay = GetCurrentAnimationTime(eIGS_Owner);
	timeDelay = (timeDelay > 0) ? timeDelay : 2000;
	GetScheduler()->TimerAction(timeDelay, CSchedulerAction<EndRippingOff>::Create(EndRippingOff(this)), false);

	if(!pOwner->IsThirdPerson() && !(m_stats.viewmode&eIVM_FirstPerson))
	{
		SetViewMode(eIVM_FirstPerson);
	}

	SetUnMountedConfiguration();

	pOwner->GetAnimationGraphState()->SetInput("Action", "idle");

	ResetActionSuffix();

	if (gEnv->bServer)
	{
		TriggerRespawn();
	}

	BATTLECHATTER(BC_Ripoff, GetOwnerId());
}



void CHMG::UnlinkMountedGun()
{
	CActor* pActor = GetOwnerActor();
	if (pActor)
	{
		pActor->LinkToMountedWeapon(0);
		if (GetEntity()->GetParent())
		{
			m_linkedParentId = GetEntity()->GetParent()->GetId();
			GetEntity()->DetachThis();
		}
	}
	m_stats.mounted = false;
}



bool CHMG::UpdateAimAnims( SParams_WeaponFPAiming &aimAnimParams, bool &releaseCameraBone )
{
	if (!m_rippedOff)
	{
		const SCachedItemAnimation check = GenerateAnimCacheID(m_sharedparams->params.mountedAimAnims.anim[0].c_str());
		uint32 changeFlags = aimAnimParams.UpdateStatus(check);
		aimAnimParams.transitionTime = -1.0f;
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
				int animationId = FindCachedAnimationId(m_sharedparams->params.mountedAimAnims.anim[i], aimAnimParams.characterInst);
				aimAnimParams.SetAnimation(i, animationId);
			}
		}

		releaseCameraBone = m_releaseCameraBone;

		return true;
	}

	return BaseClass::UpdateAimAnims(aimAnimParams, releaseCameraBone);
}


void CHMG::Update( SEntityUpdateContext& ctx, int slot )
{
	BaseClass::Update(ctx, slot);
	
	if (m_rotatingSoundID!=INVALID_SOUNDID)
	{
		if (m_framesToStopRotationSound>0)
		{
			--m_framesToStopRotationSound;
			RequireUpdate( eIUS_General );
		}
		else
		{
			StopSound(m_rotatingSoundID);
			m_rotatingSoundID = INVALID_SOUNDID;
		}
	}

	//Helper for editor placing
	bool showDebugHelper = gEnv->bEditor && !gEnv->bEditorGameMode;
	if (showDebugHelper)
	{
		IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

		const Matrix34& weaponTM = GetEntity()->GetWorldTM();
		const Vec3 point1 = weaponTM.GetTranslation();
		const Vec3 point2 = point1 - (m_sharedparams->pMountParams->ground_distance * weaponTM.GetColumn2());
		const Vec3 point3 = point2 - (m_sharedparams->pMountParams->body_distance * weaponTM.GetColumn1());

		pRenderAux->DrawLine(point1, ColorB(0, 192, 0), point2, ColorB(0, 192, 0), 3.0f);
		pRenderAux->DrawLine(point2, ColorB(0, 192, 0), point3, ColorB(0, 192, 0), 3.0f);
		pRenderAux->DrawSphere(point3, 0.15f, ColorB(192, 0, 0));

		RequireUpdate(eIUS_General);
	}
}



void CHMG::UpdateIKMounted( IActor* pActor, const Vec3& vGunXAxis )
{

}

void CHMG::SetUnMountedConfiguration()
{
	//Switch to second fire mode (first one is supposed to be for "mounted" state)
	if(GetFireMode(1) && (strcmp(GetFireMode(1)->GetType(), "Melee") != 0))
	{
		RequestFireMode(1);
	}

	ExitZoom();

	//Second zoom mode is supposed to be unmounted
	if(GetZoomMode(1))
	{
		EnableZoomMode(1, true);
		SetCurrentZoomMode(1);
	}

	//Just in case, it was not clear properly
	CActor* pOwner = GetOwnerActor();
	if ((pOwner != NULL) && pOwner->IsClient())
	{
		float defaultFov = 55.0f;
		gEnv->pRenderer->EF_Query(EFQ_DrawNearFov,(INT_PTR)&defaultFov);
	}
}


void CHMG::ProcessEvent(SEntityEvent& event)
{
	if ((event.event == ENTITY_EVENT_XFORM) && IsMounted() && GetOwnerId())
	{
		const float Z_EPSILON = 0.001f;
		float zAngle = GetEntity()->GetWorldAngles().z;
		bool anglesAreEquivalent = (fabs(zAngle-m_lastZAngle)<Z_EPSILON);
		if (!anglesAreEquivalent)
		{
			if (m_rotatingSoundID==INVALID_SOUNDID)
				m_rotatingSoundID = PlayAction(g_pItemStrings->rotate_mounted);
			m_framesToStopRotationSound = 1;
			RequireUpdate( eIUS_General );
			m_lastZAngle = zAngle;
		}
			
		int flags = (int)event.nParam[0];
		if ((flags & ENTITY_XFORM_FROM_PARENT) && !(flags & ENTITY_XFORM_USER))
		{
			if (CActor* pOwnerActor = GetOwnerActor())
			{
				pOwnerActor->UpdateMountedGunController(true);
			}
		}
	}

	BaseClass::ProcessEvent(event);
}

void CHMG::Select(bool select)
{
	if (select == IsSelected())
		return;
	BaseClass::Select(select);
}

void CHMG::FadeCrosshair( float from, float to, float time )
{
	if (IsMounted())
	{
		BaseClass::FadeCrosshair(from, to, time);
	}
	else
	{
		s_crosshairstats.fading = true;
		s_crosshairstats.fadefrom = from;
		s_crosshairstats.fadeto = 1.0f;		//Always on 
		s_crosshairstats.fadetime = MAX(0, time);
		s_crosshairstats.fadetimer = s_crosshairstats.fadetime;

		SetCrosshairOpacity(from);
	}
}




bool CHMG::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if (!BaseClass::NetSerialize(ser, aspect, profile, flags))
		return false;

	if(aspect == ASPECT_RIPOFF)
	{
		ser.Value("ripOff", static_cast<CHMG*>(this), &CHMG::IsRippingOrRippedOff, &CHMG::SetRippingOff, 'bool');
	}

	return true;
}

void CHMG::InitClient(int channelId)
{
	BaseClass::InitClient(channelId);

	IActor *pActor = GetOwnerActor();

	if(pActor && (m_rippingOff || m_rippedOff))
	{
		EntityId ownerID = pActor->GetEntity()->GetId();
		GetGameObject()->InvokeRMIWithDependentObject(ClRipOff(), SRipOffParams(ownerID), eRMI_ToClientChannel, ownerID, channelId);	
	}
}

void CHMG::SetRippingOff(bool ripOff)
{
	if(ripOff && !m_rippingOff && !m_rippedOff)
	{
		CActor *pActor = GetOwnerActor();
		if(pActor)
		{
			PerformRipOff(pActor);
		}
	}
}



void CHMG::FinishRipOff()
{
	m_rippingOff = false;
	m_rippedOff = true;
	ApplyViewLimit(GetOwnerId(), false);
	PlayAction(GetParams().idle);
}



IMPLEMENT_RMI(CHMG, SvRequestRipOff)
{
	CHECK_OWNER_REQUEST();

	if (!m_rippingOff && !m_rippedOff)
	{
		CActor *pActor = GetOwnerActor();
		if(pActor)
		{
			PerformRipOff(pActor);
			CHANGED_NETWORK_STATE(this, ASPECT_RIPOFF);
		}
	}

	return true;
}

IMPLEMENT_RMI(CHMG, ClRipOff)
{
	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(params.ownerId);
	if(pActor && (!m_rippingOff || m_rippedOff))
	{
		StartUse(params.ownerId);
		PerformRipOff((CActor*)pActor);
	}

	return true;
}
