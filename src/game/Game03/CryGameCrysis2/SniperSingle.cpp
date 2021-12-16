/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 26:02:2010   Created by Filipe Amim

*************************************************************************/
#include "StdAfx.h"
#include "SniperSingle.h"
#include "IronSight.h"



CSniperSingle::CPumpAction::CPumpAction()
	:	m_running(false)
	,	m_timeCounter(0.0f)
	,	m_waiting(false)
{
}


void CSniperSingle::CPumpAction::Update(const CSniperSingle& pSniperSingle, float frameTime)
{
	if (!m_running)
		return;

	CWeapon* pWeapon = pSniperSingle.m_pWeapon;
	pWeapon->RequireUpdate(eIUS_FireMode);

	m_timeCounter = max(m_timeCounter - frameTime, 0.0f);

	if (m_waiting)
	{
		bool weaponZoomed = pWeapon->IsZoomed() || pWeapon->IsZoomingInOrOut();
		if (weaponZoomed || !pSniperSingle.IsWeaponSelected())
			return;
		m_waiting = false;
		m_timeCounter = Pump(pSniperSingle);
	}
	else
	{
		if (m_timeCounter == 0.0f)
			m_running = false;
	}
}



float CSniperSingle::CPumpAction::Pump(const CSniperSingle& pSniperSingle) const
{
	CWeapon* pWeapon = pSniperSingle.m_pWeapon;
	pWeapon->PlayAction(pSniperSingle.GetShared()->actions.cock);
	return pWeapon->GetCurrentAnimationTime(eIGS_Owner) / 1000.0f;
}



void CSniperSingle::CPumpAction::Schedule()
{
	m_running = true;
	m_waiting = true;
}




CSniperSingle::CSniperSingle()
{
	Reset();
}



void CSniperSingle::Activate(bool activate)
{
	CSingle::Activate(activate);
	Reset();
}



void CSniperSingle::Reset()
{
}



void CSniperSingle::Update(float frameTime, uint32 slot)
{
	CSingle::Update(frameTime, slot);

	m_pumpAction.Update(*this, frameTime);

	if (m_pWeapon->IsInputFlagSet(CWeapon::eWeaponAction_Zoom) && !IsWeaponZoomed() && CanZoom())
		RestartZoom();
}



bool CSniperSingle::Shoot(bool resetAnimation, bool autoreload, bool isRemote)
{
	bool fired = CSingle::Shoot(resetAnimation, autoreload, isRemote);
	bool lastShot = GetAmmoCount() == 0;
	if (fired && !lastShot)
		m_pumpAction.Schedule();
	return fired;
}



void CSniperSingle::StartFire()
{
	if (IsWeaponFullyZoomed() && m_pumpAction.IsRunning())
		m_pWeapon->ExitZoom(true);
	else if (!m_pumpAction.IsRunning())
		CSingle::StartFire();
}



bool CSniperSingle::CanZoom() const
{
	return !m_pumpAction.IsRunning() && IsWeaponSelected();
}



void CSniperSingle::RestartZoom() const
{
	IZoomMode* pZoomMode = m_pWeapon->GetZoomMode(m_pWeapon->GetCurrentZoomMode());
	if (pZoomMode)
		pZoomMode->ZoomIn();
}



bool CSniperSingle::IsWeaponSelected() const
{
	return m_pWeapon->IsSelected() && !m_pWeapon->IsDeselecting();
}



bool CSniperSingle::IsWeaponZoomed() const
{
	return m_pWeapon->IsZoomed() || m_pWeapon->IsZoomingInOrOut();
}



bool CSniperSingle::IsWeaponFullyZoomed() const
{
	return m_pWeapon->IsZoomed() && !m_pWeapon->IsZoomingInOrOut();
}
