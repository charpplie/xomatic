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
#include "Scope.h"
#include "Player.h"
#include "IWorldQuery.h"
#include "GameCVars.h"

#include "Game.h"

#include "WeaponSharedParams.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//---------------------------------------------
CScope::CScope():
m_showTimer(-1.0f),
m_hideTimer(-1.0f)
{

}

//------------------------------------------------------------------------
void CScope::Update(float frameTime, uint32 frameId)
{
	CIronSight::Update(frameTime, frameId);

	if (m_showTimer>0.0f)
	{
		m_showTimer-=frameTime;
		if (m_showTimer<=0.0f)
		{
			m_showTimer=-1.0f;
			//m_pWeapon->Hide(false);
			m_pWeapon->SendMusicLogicEvent(eMUSICLOGICEVENT_SNIPERMODE_LEAVE);
			m_pWeapon->OnZoomOut();
		}

		m_pWeapon->RequireUpdate(eIUS_Zooming);
	}

	if (m_hideTimer>0.0f)
	{
		m_hideTimer-=frameTime;
		if (m_hideTimer<=0.0f)
		{
			if(m_pWeapon->IsOwnerClient())
			{
				m_hideTimer=-1.0f;
				//m_pWeapon->Hide(true);
				m_pWeapon->OnZoomIn();
				int iZoom = 0;
				if(!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_assault"))
					iZoom = 1;
				if(!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_sniper"))
					iZoom = 2;


				if(iZoom != 0)
				{
					SAFE_HUD_FUNC(GetScopes()->ShowScope(iZoom));
					m_pWeapon->SendMusicLogicEvent(eMUSICLOGICEVENT_SNIPERMODE_ENTER);
				}
			}

		}

		m_pWeapon->RequireUpdate(eIUS_Zooming);
	}
}

//------------------------------------------------------------------------
void CScope::Activate(bool activate)
{
	if (!activate)
	{
		if (m_zoomed || m_zoomTimer>0.0f)
		{
			if(	!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_default") ||
				!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_assault") ||
				!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_sniper"))
			{
				SAFE_HUD_FUNC(GetScopes()->ShowScope(0));
			}
		}
	}

	CIronSight::Activate(activate);
}

//------------------------------------------------------------------------
void CScope::InformActorOfScope(bool active)
{
	IActor *pActor=m_pWeapon->GetOwnerActor();
	if (pActor && pActor->IsPlayer())
	{
		CPlayer *pPlayer=static_cast<CPlayer*>(pActor);
		pPlayer->SendPerkEvent(EPE_ScopeActive, &active);
		CCCPOINT_IF(active, PlayerWeapon_SniperScopeOn);
		CCCPOINT_IF(!active, PlayerWeapon_SniperScopeOff);
	}
}

//------------------------------------------------------------------------
void CScope::OnEnterZoom()
{
	CIronSight::OnEnterZoom();
	m_hideTimer = 0.15f;
	m_showTimer = -1.0f;

	InformActorOfScope(true);
}

//------------------------------------------------------------------------
void CScope::OnLeaveZoom()
{
	CIronSight::OnLeaveZoom();
	m_showTimer = 0.025f;
	m_hideTimer = -1.0f;

	if(	!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_default") ||
		!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_assault") ||	
		!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_sniper"))
	{
		SAFE_HUD_FUNC(GetScopes()->ShowScope(0));
	}

	InformActorOfScope(false);
}

//-------------------------------------------------------------
void CScope::OnZoomStep(bool zoomingIn, float t)
{
	CIronSight::OnZoomStep(zoomingIn, t);

	// only call the HUD when we have finished(!) zooming (t == 1.0f)
	if (t > 0.999f)
	{
		if(	!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_default") ||
			!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_assault") ||
			!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_sniper"))
		{
			SAFE_HUD_FUNC(GetScopes()->SetScopeZoomMode(m_currentStep, GetScopeShared()->scopeParams.scope));
		}
		else if(!strcmp(m_zoomParams->scopeParams.scope.c_str(),"scope_binoculars"))
		{
			SAFE_HUD_FUNC(GetScopes()->SetBinocularsZoomMode(m_currentStep));
		}
	}
}

//-------------------------------------------------------------
void CScope::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));
}
