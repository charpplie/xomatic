/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 28:10:2005   16:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "IronSight.h"
#include "Player.h"
#include "GameCVars.h"
#include "Single.h"

#include "WeaponSharedParams.h"
#include "ScreenEffects.h"
#include "Game.h"
#include "HUD/HUD.h"
#include "Utility/CryWatch.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#define PHYS_FOREIGN_ID_DOF_QUERY PHYS_FOREIGN_ID_USER+3


//------------------------------------------------------------------------
CIronSight::CIronSight()
: m_pWeapon(0), 
m_savedFoVScale(0.0f),
m_zoomed(false),
m_zoomingIn(false),
m_zoomTimer(0.0f),
m_zoomTime(0.0f),
m_focus(1.0f),
m_startFoV(0.0f),
m_endFoV(0.0f),
m_smooth(false),
m_enabled(true),
m_currentStep(0),
m_initialNearFov(55.0f),
m_maxDoF(100.0f),
m_minDoF(0.0f),
m_averageDoF(50.0f),
m_swayTime(0.0f),
m_swayCycle(0.0f),
m_lastRecoil(0.0f),
m_leaveZoomTimeDelay(0.0f),
m_zoomOutScheduled(false)
{
}

//------------------------------------------------------------------------
CIronSight::~CIronSight()
{
	m_zoomParams = 0; 
}

//------------------------------------------------------------------------
void CIronSight::InitZoomMode(IWeapon *pWeapon, const SParentZoomModeParams* pParams, uint32 id)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);
	m_zmIdx = id;

	m_zoomParams = &pParams->baseZoomMode;
	m_parentZoomParams = pParams;
}

//------------------------------------------------------------------------
void CIronSight::Update(float frameTime, uint32 frameId)
{
	bool keepUpdating=false;
	bool isClient = m_pWeapon->IsOwnerClient();

	float doft = 1.0f;
	if (!m_zoomed)
		doft = 0.0f;

	if (m_zoomTime > 0.0f)	// zoomTime is set to 0.0 when zooming ends
	{
		keepUpdating=true;
		float t = CLAMP(1.0f-m_zoomTimer/m_zoomTime, 0.0f, 1.0f);
		float fovScale;

		if (m_smooth)
		{
			if (m_startFoV > m_endFoV)
				doft = t;
			else
				doft = 1.0f-t;

			fovScale = m_startFoV+t*(m_endFoV-m_startFoV);
		}
		else
		{
			fovScale = m_startFoV;
			if (t>=1.0f)
				fovScale = m_endFoV;
		}

		OnZoomStep(m_startFoV>m_endFoV, t);

		SetActorFoVScale(fovScale, true, true);

		if(isClient && m_zoomParams->zoomParams.scope_mode)
		{
			AdjustNearFov(t*1.25f,m_startFoV>m_endFoV);
		}

		if (t>=1.0f)
		{
			SetIsZoomed(m_zoomingIn);
			m_zoomTime = 0.0f;
		}
	}

	bool updateDof = isClient && (g_pGameCVars->g_dof_ironsight != 0);
	if (updateDof)
	{
		UpdateDepthOfField(frameTime, doft);
	}

	bool wasZooming = m_zoomTimer>0.0f;
	if (wasZooming || m_zoomed)
	{
		m_zoomTimer -= frameTime;
		if (m_zoomTimer<0.0f)
		{
			m_zoomTimer=0.0f;
			if (wasZooming)
			{
				if (m_zoomingIn)
				{
					OnZoomedIn();
				}
				else
				{
					OnZoomedOut();
				}
			}
		}

		if (m_focus < 1.0f)
		{
			m_focus += frameTime*1.5f;
		}
		else if (m_focus > 1.0f)
		{
			m_focus = 1.0f;
		}

		if (isClient)
		{
			//float t=m_zoomTimer/m_zoomTime;
			if (m_zoomTime > 0.0f)
			{
				//t=1.0f-max(t, 0.0f);
				gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 1.0f);
			}

			// try to convert user-defined settings to IronSight system (used for Core)
			float userActive;
			gEnv->p3DEngine->GetPostEffectParam("Dof_User_Active", userActive);
			if (userActive > 0.0f)
			{
				float focusRange;
				float focusDistance;
				gEnv->p3DEngine->GetPostEffectParam("Dof_User_FocusRange", focusRange);
				gEnv->p3DEngine->GetPostEffectParam("Dof_User_FocusDistance", focusDistance);
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMin", focusDistance);
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMax", focusDistance);
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", focusDistance+focusRange*0.5f);
			}
			else
			{
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMin", g_pGameCVars->g_dofset_minScale * m_minDoF);
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMax", g_pGameCVars->g_dofset_maxScale * m_averageDoF);
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", g_pGameCVars->g_dofset_limitScale * m_averageDoF);
			}
		}

		keepUpdating=true;
	}

	m_leaveZoomTimeDelay = max(m_leaveZoomTimeDelay - frameTime, 0.0f);
	if (m_zoomOutScheduled && m_leaveZoomTimeDelay == 0.0f && !m_pWeapon->IsInputFlagSet(CWeapon::eWeaponAction_Zoom))
	{
		LeaveZoom();
		m_zoomOutScheduled = false;
	}

	if (keepUpdating)
		m_pWeapon->RequireUpdate(eIUS_Zooming);
}

//------------------------------------------------------------------------
void CIronSight::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CIronSight::Activate(bool activate)
{
	if (!m_zoomed && !m_zoomingIn && !activate)
	{
		return;
	}

	if (!activate && m_zoomed && m_zoomingIn && m_zoomParams->zoomParams.dof)
		ClearDoF();

	if (!activate && !m_zoomParams->zoomParams.suffix.empty())
		m_pWeapon->ResetActionSuffix(m_zoomParams->zoomParams.zoom_out_time);

	SetIsZoomed(false);
	m_zoomingIn = false;

	m_currentStep = 0;
	m_lastRecoil = 0.0f;
	m_leaveZoomTimeDelay = 0.0f;
	m_zoomOutScheduled = false;

	SetActorFoVScale(1.0f, true, true);

	ResetTurnOff();
	if (!activate)
	{
		ClearBlur();
	}

	if(!activate && m_zoomParams->zoomParams.scope_mode)
	{
		ResetNearFov();
	}
}

//------------------------------------------------------------------------
bool CIronSight::CanZoom() const
{
	return true;
}

//------------------------------------------------------------------------
bool CIronSight::StartZoom(bool stayZoomed, bool fullZoomout, int zoomStep)
{
	if (m_pWeapon->IsBusy()) // || (IsToggle() && IsZooming())) //This second check has been removed because we want to be able to interrupt a zoom-out with a new zoom in
	{
		return false;
	}

	if(m_pWeapon->IsOwnerClient())
	{
		if (g_pGame->GetScreenEffects())
			g_pGame->GetScreenEffects()->ProcessZoomInEffect();
	}

	m_zoomOutScheduled = false;

	if (!m_zoomed || stayZoomed)
	{
		EnterZoom(m_zoomParams->zoomParams.zoom_in_time, m_zoomParams->zoomParams.layer.c_str(), true, zoomStep);
		m_currentStep = zoomStep;
	}
	else
	{
		int currentStep = m_currentStep;
		int nextStep = currentStep+1;

		if (nextStep > m_zoomParams->zoomParams.stages.size())
		{
			if (!stayZoomed)
			{
				if (fullZoomout)
				{
					StopZoom();
				}
				else
				{
					float oFoV = GetZoomFoVScale(currentStep);
					m_currentStep = 0;
					float tFoV = GetZoomFoVScale(m_currentStep);
					ZoomIn(m_zoomParams->zoomParams.stage_time, oFoV, tFoV, true, m_currentStep);
					return true;
				}
			}
		}
		else
		{
			float oFoV = GetZoomFoVScale(currentStep);
			float tFoV = GetZoomFoVScale(nextStep);

			ZoomIn(m_zoomParams->zoomParams.stage_time, oFoV, tFoV, true, nextStep);

			m_currentStep = nextStep;

			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
void CIronSight::StopZoom()
{
	ScheduleLeaveZoom();
	m_currentStep = 0;
}

//------------------------------------------------------------------------
void CIronSight::ExitZoom(bool force)
{
	if (force)
	{
		LeaveZoom();
	}
	else if (m_zoomed && m_zoomTime==0.0f)
	{
		ScheduleLeaveZoom();
		m_currentStep = 0;
	}
}

//------------------------------------------------------------------------
void CIronSight::ZoomIn()
{
	if (m_pWeapon->IsBusy())
		return;

	if (!m_zoomed)
	{
		EnterZoom(m_zoomParams->zoomParams.zoom_in_time, m_zoomParams->zoomParams.layer.c_str(), true);
		m_currentStep = 1;
		m_leaveZoomTimeDelay = m_zoomParams->zoomParams.zoom_out_delay;
	}
	else
	{
		int currentStep = m_currentStep;
		int nextStep = currentStep+1;

		if (nextStep > m_zoomParams->zoomParams.stages.size())
			return;
		else
		{
			float oFoV = GetZoomFoVScale(currentStep);
			float tFoV = GetZoomFoVScale(nextStep);

			ZoomIn(m_zoomParams->zoomParams.stage_time, oFoV, tFoV, true, nextStep);

			m_currentStep = nextStep;
		}
	}
}

//------------------------------------------------------------------------
bool CIronSight::ZoomOut()
{
	if (m_pWeapon->IsBusy())
		return false;

	if (!m_zoomed)
	{
		EnterZoom(m_zoomParams->zoomParams.zoom_in_time, m_zoomParams->zoomParams.layer.c_str(), true);
		m_currentStep = 1;
		m_leaveZoomTimeDelay = m_zoomParams->zoomParams.zoom_out_delay;
	}
	else
	{
		int currentStep = m_currentStep;
		int nextStep = currentStep-1;

		if (nextStep < 1)
			return false;
		else
		{
			float oFoV = GetZoomFoVScale(currentStep);
			float tFoV = GetZoomFoVScale(nextStep);

			ZoomIn(m_zoomParams->zoomParams.stage_time, oFoV, tFoV, true, nextStep);

			m_currentStep = nextStep;
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
bool CIronSight::IsZoomed() const
{
	return m_zoomed;
}

//------------------------------------------------------------------------
bool CIronSight::IsZoomingInOrOut() const
{
	return m_zoomTimer>0.0f;
}

//------------------------------------------------------------------------
bool CIronSight::IsZoomingIn() const
{
	return m_zoomingIn;
}

//------------------------------------------------------------------------
EZoomState CIronSight::GetZoomState() const
{
	if (IsZoomingInOrOut())
	{
		if (IsZoomingIn())
			return eZS_ZoomingIn;
		else
			return eZS_ZoomingOut;
	}
	else
	{
		if (IsZoomed())
			return eZS_ZoomedIn;
		else
			return eZS_ZoomedOut;
	}
}

//------------------------------------------------------------------------
void CIronSight::Enable(bool enable)
{
	m_enabled = enable;
}

//------------------------------------------------------------------------
bool CIronSight::IsEnabled() const
{
	return m_enabled;
}

//------------------------------------------------------------------------
void CIronSight::EnterZoom(float time, const char *zoom_layer, bool smooth, int zoomStep)
{
	if (IsZoomingInOrOut() && !m_zoomingIn)
	{
		OnZoomedOut();
	}
	ResetTurnOff();

	m_pWeapon->FadeCrosshair(1.0f, 0.0f, WEAPON_FADECROSSHAIR_ZOOM);

	float oFoV = GetZoomFoVScale(0);
	float tFoV = GetZoomFoVScale(zoomStep);

	ZoomIn(time, oFoV, tFoV, smooth, zoomStep);

	const SZoomParams &zoomParams = m_zoomParams->zoomParams;
	m_pWeapon->SetActionSuffix(zoomParams.suffix.c_str(), zoomParams.suffixAG.c_str(), m_zoomParams->zoomParams.zoom_in_time);	

	m_pWeapon->StopActionAnimations(0.f);

	m_pWeapon->PlayAction(m_zoomParams->actions.zoom_in, 0, false, CItem::eIPAF_Default);

	OnEnterZoom();
}

void CIronSight::ScheduleLeaveZoom()
{
	if (!m_pWeapon->IsReloading() && m_zoomed)
	{
		m_zoomOutScheduled = true;
		m_leaveZoomTimeDelay = m_zoomParams->zoomParams.zoom_out_delay;
	}
	else
		LeaveZoom();
}

void CIronSight::LeaveZoom()
{
	float time = m_zoomParams->zoomParams.zoom_out_time;

	if (!m_zoomed && !m_zoomingIn)
		return;

	if (IsZoomingInOrOut() && m_zoomingIn)
	{
		OnZoomedIn();
	}
	ResetTurnOff();

	m_pWeapon->FadeCrosshair(0.0f, 1.0f, WEAPON_FADECROSSHAIR_ZOOM);

	float oFoV = GetZoomFoVScale(0);
	float tFoV = GetZoomFoVScale(1);

	ZoomOut(time, tFoV, oFoV, true, 0);

	m_pWeapon->StopLayer(m_zoomParams->zoomParams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	m_pWeapon->PlayAction(m_zoomParams->actions.zoom_out, 0, false, CItem::eIPAF_Default);

	m_pWeapon->ResetActionSuffix(m_zoomParams->zoomParams.zoom_out_time);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_pWeapon->GetParams().idle);
	m_currentStep = 0;

	OnLeaveZoom();
}

//------------------------------------------------------------------------
void CIronSight::ResetTurnOff()
{
	static ItemString idle = "idle";
	m_savedFoVScale = 0.0f;
	m_pWeapon->StopLayer(m_zoomParams->zoomParams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, idle);
}

//------------------------------------------------------------------------

struct CIronSight::DisableTurnOffAction
{
	DisableTurnOffAction(CIronSight *_ironSight): ironSight(_ironSight) {};
	CIronSight *ironSight;

	void execute(CItem *pWeapon)
	{
		const SZoomParams &zoomParams = ironSight->m_zoomParams->zoomParams;
		pWeapon->SetActionSuffix(zoomParams.suffix.c_str() , zoomParams.suffixAG.c_str(), zoomParams.zoom_in_time);

		ironSight->OnZoomedIn();
	}
};

struct CIronSight::EnableTurnOffAction
{
	EnableTurnOffAction(CIronSight *_ironSight): ironSight(_ironSight) {};
	CIronSight *ironSight;

	void execute(CItem *pWeapon)
	{
		ironSight->OnZoomedOut();
	}
};

void CIronSight::TurnOff(bool enable, bool smooth, bool anim)
{
	const SZoomParams &zoomParams = m_zoomParams->zoomParams;
	if (!enable && (m_savedFoVScale > 0.0f))
	{
		OnEnterZoom();

		float oFoV = GetZoomFoVScale(0);
		float tFoV = m_savedFoVScale;

		ZoomIn(zoomParams.zoom_out_time, oFoV, tFoV, smooth, 0);

		if (anim)
		{
			m_pWeapon->SetActionSuffix(zoomParams.suffix.c_str(), zoomParams.suffixAG.c_str(), zoomParams.zoom_in_time);
			m_pWeapon->PlayAction(m_zoomParams->actions.zoom_in);
		}

		m_pWeapon->GetScheduler()->TimerAction((uint32)(zoomParams.zoom_out_time*1000), CSchedulerAction<DisableTurnOffAction>::Create(this), false);
		m_savedFoVScale = 0.0f;
	}
	else if (m_zoomed && enable)
	{
		m_pWeapon->SetBusy(true);
		m_savedFoVScale = GetActorFoVScale();

		OnLeaveZoom();

		float oFoV = GetZoomFoVScale(0);
		float tFoV = m_savedFoVScale;

		ZoomOut(zoomParams.zoom_out_time, tFoV, oFoV, smooth, 0);

		m_pWeapon->StopLayer(zoomParams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
		m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_pWeapon->GetParams().idle);

		if (anim)
		{
			m_pWeapon->PlayAction(m_zoomParams->actions.zoom_out);
			m_pWeapon->ResetActionSuffix(zoomParams.zoom_out_time);
		}

		m_pWeapon->GetScheduler()->TimerAction((uint32)(zoomParams.zoom_out_time*1000), CSchedulerAction<EnableTurnOffAction>::Create(this), false);
	}
}

//------------------------------------------------------------------------
void CIronSight::ZoomIn(float time, float from, float to, bool smooth, int nextStep)
{
	m_zoomTime = time;
	m_zoomTimer = m_zoomTime;
	m_startFoV = from;
	m_endFoV = to;
	m_smooth = smooth;

	float totalFoV = abs(m_endFoV-m_startFoV);
	float ownerFoV = GetActorFoVScale();

	m_startFoV = ownerFoV;

	float deltaFoV = abs(m_endFoV-m_startFoV)/totalFoV;

	if (deltaFoV < totalFoV)
	{
		m_zoomTime = (deltaFoV/totalFoV)*time;
		m_zoomTimer = m_zoomTime;
	}

	m_zoomingIn = true;

	if(!m_zoomed)
		m_initialNearFov = *(float*)gEnv->pRenderer->EF_Query(EFQ_DrawNearFov);

	OnZoom(time, nextStep);

	m_pWeapon->RequireUpdate(eIUS_Zooming);
}

//------------------------------------------------------------------------
void CIronSight::ZoomOut(float time, float from, float to, bool smooth, int nextStep)
{
	m_zoomTimer = time;
	m_zoomTime = m_zoomTimer;

	m_startFoV = from;
	m_endFoV = to;
	m_smooth = smooth;


	float totalFoV = abs(m_endFoV-m_startFoV);
	float ownerFoV = GetActorFoVScale();

	m_startFoV = ownerFoV;

	float deltaFoV = abs(m_endFoV-m_startFoV);

	if (deltaFoV < totalFoV)
	{
		m_zoomTime = max((deltaFoV/totalFoV)*time, 0.01f);
		m_zoomTimer = m_zoomTime;
	}

	m_zoomingIn = false;

	OnZoom(time, nextStep);

	m_pWeapon->RequireUpdate(eIUS_Zooming);
}

//------------------------------------------------------------------------
void CIronSight::OnEnterZoom()
{
	if (m_pWeapon->IsOwnerClient())
	{
		if (g_pGameCVars->g_dof_ironsight != 0)
		{
			if (m_zoomParams->zoomParams.dof)
			{
				gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 1);
				gEnv->p3DEngine->SetPostEffectParamString("Dof_MaskTexName", m_zoomParams->zoomParams.dof_mask);
			}
			else
			{
				gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 0);
			}
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1);
		}

		if (m_zoomParams->zoomParams.blur_amount > 0.0f)
		{
			gEnv->p3DEngine->SetPostEffectParam("FilterMaskedBlurring_Amount", m_zoomParams->zoomParams.blur_amount);
			gEnv->p3DEngine->SetPostEffectParamString("FilterMaskedBlurring_MaskTexName", m_zoomParams->zoomParams.blur_mask);
		}
	}

	if (IFireMode* pFireMode = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()))
	{
		pFireMode->OnZoomStateChanged();
	}

	m_swayTime = 0.0f;
}

void CIronSight::SetIsZoomed(bool isZoomed)
{
	if (m_zoomed != isZoomed)
	{
		m_zoomed = isZoomed;

		CActor* pActor = m_pWeapon->GetOwnerActor();
		if (pActor && pActor->IsPlayer())
		{
			static_cast<CPlayer*>(pActor)->SendPerkEvent(EPE_IronSightActive, & isZoomed);
		}

		if (m_zoomed)
			m_pWeapon->OnZoomedIn();
		else
			m_pWeapon->OnZoomedOut();
	}
}

//------------------------------------------------------------------------
void CIronSight::OnZoomedIn()
{
	m_pWeapon->PlayLayer(m_zoomParams->zoomParams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_zoomParams->actions.idle);

	SetIsZoomed(true);

	if (m_pWeapon->IsOwnerClient())
	{
		CCCPOINT(PlayerWeapon_IronSightOn);

		if(m_zoomParams->zoomParams.dof)
		{
			m_focus = 1.0f;

			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1);
		}

		ICharacterInstance* pCharacter = m_pWeapon->GetEntity()->GetCharacter(IItemView::eIGS_FirstPerson);
		if (m_zoomParams->zoomParams.hide_weapon && pCharacter != 0)
		{
			pCharacter->HideMaster(1);
		}
	}

	ApplyZoomMod(m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()));
	m_swayCycle = 0.25f * ((cry_rand() & 1) ? -1.f : 1.f);
	m_lastRecoil = 0.0f;

}

//------------------------------------------------------------------------
void CIronSight::OnZoom(float time, int targetStep)
{
	if(m_pWeapon->IsOwnerClient())
	{
		SHUDEvent event;
		event.eventType = eHUDEvent_OnZoom;
		event.eventIntData = targetStep;
		event.eventIntData2 = m_zoomParams->zoomParams.stages.size();

		if(targetStep > 0 && targetStep <= event.eventIntData2)
			event.eventFloatData = m_zoomParams->zoomParams.stages[targetStep-1];
		else
			event.eventFloatData = 2.0f;

		CHUD::CallEvent(event);
	}
}

//------------------------------------------------------------------------
void CIronSight::OnLeaveZoom()
{
	ClearBlur();
	ClearDoF();

	if (IFireMode* pFireMode = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()))
	{
		pFireMode->OnZoomStateChanged();
	}

	ICharacterInstance* pCharacter = m_pWeapon->GetEntity()->GetCharacter(IItemView::eIGS_FirstPerson);
	if (m_pWeapon->IsClient() && m_zoomParams->zoomParams.hide_weapon && pCharacter != 0)
	{
		pCharacter->HideMaster(0);
	}
}

//------------------------------------------------------------------------
void CIronSight::OnZoomedOut()
{
	SetIsZoomed(false);
	ClearDoF();

	if (m_pWeapon->IsOwnerClient())
	{
		CCCPOINT(PlayerWeapon_IronSightOff);

		if(g_pGame->GetScreenEffects())
			g_pGame->GetScreenEffects()->ProcessZoomOutEffect();
	}

	//Reset spread and recoil modifications
	IFireMode* pFireMode = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode());

	if(pFireMode)
	{
		pFireMode->ResetSpreadMod();
		pFireMode->ResetRecoilMod();
	}

	if(m_zoomParams->zoomParams.scope_mode)
		ResetNearFov();

}

//------------------------------------------------------------------------
void CIronSight::OnZoomStep(bool zoomingIn, float t)
{
	m_focus = 0.0f;
}

//------------------------------------------------------------------------
void CIronSight::UpdateDepthOfField(float frameTime, float t)
{
	IView* pView = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
	if (pView)
	{
		static CDeferredRaycastHelper deferredRaycastHelper;

		const SViewParams* pViewParams = pView->GetCurrentParams();
		if (pViewParams)
		{
			Matrix33 viewRotation = (Matrix33)pViewParams->rotation;

			Vec3 start = pViewParams->position;
			Vec3 dir = viewRotation.GetColumn1();

			IPhysicalEntity* pSkipEntities[10];
			int nSkip = CSingle::GetSkipEntities(m_pWeapon, pSkipEntities, 10);
			// jitter the direction (non-uniform disk sampling ... we want to bias the center in this case)
			f32 cosTheta, sinTheta;
			f32 theta = Random() * gf_PI2;
			f32 spreadAngle = DEG2RAD(g_pGameCVars->g_dof_sampleAngle)/2.0f;
			f32 scale = tan_tpl(spreadAngle);
			f32 radiusSqrt = scale * Random();
			sincos_tpl(theta, &cosTheta, &sinTheta);
			f32 x = radiusSqrt * cosTheta;
			f32 y = radiusSqrt * sinTheta;

			Vec3 xOff = x * viewRotation.GetColumn0();
			Vec3 yOff = y * viewRotation.GetColumn2();

			// jitter
			if (true)
			{
				dir += xOff + yOff;
				dir.Normalize();
			}
			const static float minRelaxSpeed = 10.0f;
			const static float maxRelaxSpeed = 1.0f;

			f32 delta;

			deferredRaycastHelper->CastRay(start, 1000.0f*dir, ent_all, rwi_pierceability(10)|rwi_ignore_back_faces);
			const ray_hit *pLastHit = deferredRaycastHelper->GetRayHit();

			bool validHit = (pLastHit != NULL) && (pLastHit->dist > 0.0f);
			if (validHit)
			{
				delta = g_pGameCVars->g_dof_minHitScale*pLastHit->dist - m_minDoF;
				Limit(delta, -g_pGameCVars->g_dof_minAdjustSpeed, g_pGameCVars->g_dof_minAdjustSpeed);
				//delta *= fabs(delta/minAdjustSpeed);
				m_minDoF += delta * frameTime;

				delta = g_pGameCVars->g_dof_maxHitScale*pLastHit->dist - m_maxDoF;
				Limit(delta, -g_pGameCVars->g_dof_maxAdjustSpeed, g_pGameCVars->g_dof_maxAdjustSpeed);
				//delta *= fabs(delta/maxAdjustSpeed);
				m_maxDoF += delta * frameTime;
			}

			if (m_maxDoF - g_pGameCVars->g_dof_distAppart < m_minDoF)
			{
				m_maxDoF = m_minDoF + g_pGameCVars->g_dof_distAppart;
			}
			else
			{
				// relax max to min
				delta = m_minDoF - m_maxDoF;
				Limit(delta, -maxRelaxSpeed, maxRelaxSpeed);
				//delta *= fabs(delta/maxRelaxSpeed);
				m_maxDoF += delta * frameTime;
			}

			// the average is relaxed to the center between min and max
			m_averageDoF = (m_maxDoF - m_minDoF)/2.0f;
			Limit(delta, -g_pGameCVars->g_dof_averageAdjustSpeed, g_pGameCVars->g_dof_averageAdjustSpeed);
			//delta *= fabs(delta/averageAdjustSpeed);
			m_averageDoF += delta * frameTime;
		}
	}
}
//------------------------------------------------------------------------
void CIronSight::Serialize(TSerialize ser)
{
}

//------------------------------------------------------------------------
void CIronSight::SetActorFoVScale(float fovScale, bool recoil, bool hbob)
{
	if (!m_pWeapon->GetOwnerActor())
		return;

	SActorParams *pActorParams = m_pWeapon->GetOwnerActor()->GetActorParams();
	if (!pActorParams)
		return;

	pActorParams->viewFoVScale = fovScale;

	if (hbob)
	{
		float mult = GetHBobFromFoVScale(fovScale);
		pActorParams->weaponInertiaMultiplier = mult;
		pActorParams->weaponBobbingMultiplier = mult;
	}


}

//------------------------------------------------------------------------
float CIronSight::GetActorFoVScale() const
{
	if (!m_pWeapon->GetOwnerActor())
		return 1.0f;

	SActorParams *pActorParams = m_pWeapon->GetOwnerActor()->GetActorParams();
	if (!pActorParams)
		return 1.0f;

	return pActorParams->viewFoVScale;
}


//------------------------------------------------------------------------
float CIronSight::GetHBobFromFoVScale(float scale) const
{
	float mag = GetMagFromFoVScale(scale);
	if (mag<=1.001f)
		return 1.0f;

	return 1.0f/(mag*m_zoomParams->zoomParams.hbob_ratio);
}

//------------------------------------------------------------------------
float CIronSight::GetRecoilFromFoVScale(float scale) const
{
	float mag = GetMagFromFoVScale(scale);
	if (mag<=1.001f)
		return 1.0f;

	return 1.0f/(mag*m_zoomParams->zoomParams.recoil_ratio);
}

//------------------------------------------------------------------------
float CIronSight::GetMagFromFoVScale(float scale) const
{
	assert(scale>0.0f);
	return 1.0f/scale;
}

//------------------------------------------------------------------------
float CIronSight::GetFoVScaleFromMag(float mag) const
{
	assert(mag>0.0f);
	if (mag >= 1.0f)
		return 1.0f/mag;
	else
		return 1.0f;
}

//------------------------------------------------------------------------
float CIronSight::GetZoomFoVScale(int step) const
{
	if (!step)
		return 1.0f;

	return 1.0f/m_zoomParams->zoomParams.stages[step-1];
}

//------------------------------------------------------------------------
void CIronSight::ClearDoF()
{
	if (m_pWeapon->IsOwnerClient())
	{
		gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0.0f);
	}
}

//------------------------------------------------------------------------
void CIronSight::ClearBlur()
{
	if (m_pWeapon->IsOwnerClient())
	{
		gEnv->p3DEngine->SetPostEffectParam("FilterMaskedBlurring_Amount", 0.0f);
	}
}


//-------------------------------------------------------------------------
void CIronSight::AdjustNearFov(float time, bool zoomIn)
{
	float newFov = -1.0f;
	if(zoomIn && (m_currentStep==1))
	{
		if(time>1.0f)
			time = 1.0f;
		newFov = (m_initialNearFov*(1.0f-time))+(m_zoomParams->zoomParams.scope_nearFov*time);
	}
	else if(!zoomIn && !m_currentStep)
	{
		newFov = (m_initialNearFov*time)+(m_zoomParams->zoomParams.scope_nearFov*(1.0f-time));
		if(time>1.0f)
			newFov = m_initialNearFov;
	}
	if(newFov>0.0f)
		gEnv->pRenderer->EF_Query(EFQ_DrawNearFov,(INT_PTR)&newFov);
}

//------------------------------------------------------------------------
void CIronSight::ResetNearFov()
{
	if(m_pWeapon->IsOwnerClient())
	{
		AdjustNearFov(1.1f,false);
	}
}

//-------------------------------------------------------------------------
void CIronSight::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));
}

//--------------------------------------------------------------------------
void CIronSight::ApplyZoomMod(IFireMode* pFM)
{
	if(pFM)
	{
		pFM->PatchSpreadMod(m_zoomParams->spreadModParams);
		pFM->PatchRecoilMod(m_zoomParams->recoilModParams);
	}
}

//--------------------------------------------------------------------------
bool CIronSight::IsToggle()
{
	return false;
}

//-----------------------------------------------------------------------
void CIronSight::FilterView(SViewParams &viewparams)
{
	float x,y;

	ZoomSway(gEnv->pTimer->GetFrameTime(),x,y);

	CActor* pOwnerActor = m_pWeapon->GetOwnerActor();
	CRY_ASSERT_MESSAGE(pOwnerActor, "(Ironsight:FilterView) This should never happen. This is supposed to be the player's current weapon, but it's not!");

	const Ang3 swayAngleOffset(x, 0.0f, y);

	//There seems to be a case in which something goes wrong, but very unlikely to reproduce...
	if (pOwnerActor)
	{
		pOwnerActor->AddViewAngleOffsetForFrame(swayAngleOffset);
	}
}

//--------------------------------------------------------------------------
void CIronSight::ZoomSway(float time, float &x, float&y)
{
	static bool  firing = false;

	bool wasFiring = firing;

	//Update while not firing...
	if(IFireMode* pFM = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()))
	{
		if(pFM->IsFiring())
			firing = true;
		else
			firing = false;
	}

	//Reset cycle after firing
	if(wasFiring && !firing)
		m_swayTime = m_zoomParams->zoomSway.stabilizeTime*(1.0f-m_zoomParams->zoomSway.scaleAfterFiring);

	//Just a simple sin/cos function
	float dtX = cry_sinf(m_swayCycle*gf_PI*4.0f);
	float dtY = -cry_cosf(m_swayCycle*gf_PI*2.0f);

	float fNextSwayCycle = m_swayCycle + (0.3f * time); 
	m_swayCycle = static_cast<float>(__fsel(fNextSwayCycle - 1.0f, fNextSwayCycle - 1.0f, fNextSwayCycle));

	m_swayTime += time;

	//Strength scale
	float strengthScale = 1.0f;
	float settleTimeScale = 1.0f;
	float stanceScale = 1.0f;
	CPlayer* pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());

	if(pPlayer)
	{
		if(pPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power && pPlayer->GetActorSuitGameParameters().IsSuitPowerActive())
		{
			strengthScale = m_zoomParams->zoomSway.strengthScale;
			settleTimeScale = m_zoomParams->zoomSway.strengthScaleTime;
		}

		//Stance mods
		if(pPlayer->GetStance()==STANCE_CROUCH)
		{
			stanceScale = m_zoomParams->zoomSway.crouchScale;
		}
	}

	//Time factor
	float factor = m_zoomParams->zoomSway.minScale;
	float settleTime = m_zoomParams->zoomSway.stabilizeTime*settleTimeScale;
	if(m_swayTime<settleTime)
	{
		factor = (settleTime-m_swayTime)/settleTime;
		if(factor<m_zoomParams->zoomSway.minScale)
			factor = m_zoomParams->zoomSway.minScale;
	}

	//Final displacement
	float swayAmount = factor * strengthScale * stanceScale;
	float additionalSway = 0.f;

	if (pPlayer)
	{
		pPlayer->SendPerkEvent(EPE_OverrideSwayAmount, & additionalSway);
	}

	x = dtX * (m_zoomParams->zoomSway.maxX + additionalSway) * swayAmount;
	y = dtY * (m_zoomParams->zoomSway.maxY + additionalSway) * swayAmount;
}

//======================================================
void CIronSight::PostFilterView(SViewParams & viewparams)
{
	if(m_zoomParams->zoomParams.scope_mode)
	{
		if(IFireMode* pFM = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()))
		{
			Vec3 front = viewparams.rotation.GetColumn1();
			if(pFM->IsFiring())
			{
				float strengthScale = 1.0f;
				CPlayer* pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());
				if(pPlayer)
				{
					if(pPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power && pPlayer->GetActorSuitGameParameters().IsSuitPowerActive())
						strengthScale = 2.0f;
				}

				float currentRecoil = pFM->GetRecoil();
				if(currentRecoil>1.5f)
					currentRecoil = 1.5f + Random(-1.25f,0.65f);
				else if(currentRecoil>0.6f)
					currentRecoil = currentRecoil + Random(-0.4f,0.3f);

				float scale = 0.01f * currentRecoil * strengthScale;
				front *=scale;
				viewparams.position += front;

				m_lastRecoil = currentRecoil;
			}
			else
			{
				const float decay = 75.0f;
				float currentRecoil = m_lastRecoil - (decay*gEnv->pTimer->GetFrameTime());
				float scale = 0.005f * currentRecoil;
				scale = CLAMP(scale,0.0f,1.0f);
				front *=scale;
				viewparams.position += front;

				m_lastRecoil = max(0.0f,currentRecoil);
			}
		}
	}
}

//===================================================
int CIronSight::GetMaxZoomSteps() const
{
	return m_zoomParams->zoomParams.stages.size();
}

//===================================================
float CIronSight::GetZoomInTime() const
{
	//Returns the left time to reach "zoomed state"
	return m_zoomTimer;
}

//===================================================
float CIronSight::GetZoomTransition() const
{
	if (m_zoomTimer <= 0)
		return 0.0f;
	float transition = (m_zoomTimer / m_zoomTime);
	if (m_zoomingIn)
		transition = 1.0f - transition;
	return transition;
}

//===================================================
bool CIronSight::AllowsZoomSnap() const
{
	return m_zoomParams->zoomParams.target_snap_enabled;
}
