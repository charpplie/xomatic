/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Automatic.h"
#include "Actor.h"
#include "ScreenEffects.h"

#include "WeaponSharedParams.h"
#include "Game.h"

//------------------------------------------------------------------------
CAutomatic::CAutomatic()
{
}

//------------------------------------------------------------------------
CAutomatic::~CAutomatic()
{
}

//------------------------------------------------------------------------
void CAutomatic::StartReload(int zoomed)
{
	if(m_soundId)
	{
		m_pWeapon->StopSound(m_soundId);
		m_soundId = INVALID_SOUNDID;
	}

	m_firing = false;

	BaseClass::StartReload(zoomed);
}

//------------------------------------------------------------------------
void CAutomatic::StartFire()
{
	BaseClass::StartFire();

	if(m_firing && m_soundId==INVALID_SOUNDID && !m_fireParams->actions.automatic_fire.empty())
		m_soundId = m_pWeapon->PlayAction(m_fireParams->actions.automatic_fire);

	m_pWeapon->RequestStartFire();
}
//------------------------------------------------------------------------
void CAutomatic::Update(float frameTime, uint32 frameId)
{
	BaseClass::Update(frameTime, frameId);

	if (m_firing && CanFire(false))
	{
		m_firing = Shoot(true);

		if(!m_firing)
		{
			StopFire();
		}
	}
}

//------------------------------------------------------------------------
void CAutomatic::StopFire()
{
	if(m_firing)
		SmokeEffect();

	m_firing = false;

	if(m_soundId)
	{
		m_pWeapon->StopSound(m_soundId);
		m_soundId = INVALID_SOUNDID;
	}

	m_pWeapon->RequestStopFire();
}

//------------------------------------------------------------------------
const char *CAutomatic::GetType() const
{
	return "Automatic";
}

//---------------------------------------------------
void CAutomatic::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));	
	GetInternalMemoryUsage(s); 
}

void CAutomatic::GetInternalMemoryUsage(ICrySizer * s) const
{
	CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
}
