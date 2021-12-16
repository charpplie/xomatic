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
#include "Rapid.h"
#include "Actor.h"
#include "Player.h"
#include "ISound.h"
#include "Game.h"
#include "GameCVars.h"
#include <IViewSystem.h>

#include "WeaponSharedParams.h"
#include "ScreenEffects.h"

#include "Utility/CryWatch.h"

//------------------------------------------------------------------------
CRapid::CRapid()
: m_fireLoopSoundId(INVALID_SOUNDID)
, m_spinUpSoundId(INVALID_SOUNDID)
{
	m_rapidFlags = eRapidFlag_none;
	m_startFiringTime.SetSeconds(0.0f);
}

//------------------------------------------------------------------------
CRapid::~CRapid()
{
}

//------------------------------------------------------------------------
void CRapid::Activate(bool activate)
{
	CSingle::Activate(activate);

	if (!activate)
	{
		if (m_fireLoopSoundId != INVALID_SOUNDID)
		{
			m_pWeapon->StopSound(m_fireLoopSoundId);
			m_fireLoopSoundId = INVALID_SOUNDID;
		}

		if (m_spinUpSoundId != INVALID_SOUNDID)
		{
			m_pWeapon->StopSound(m_spinUpSoundId);
			m_spinUpSoundId = INVALID_SOUNDID;
		}
	}

	m_rotation_angle = 0.0f;
	m_speed = 0.0f;
	m_rapidFlags &= ~(eRapidFlag_accelerating|eRapidFlag_decelerating|eRapidFlag_startedFiring);
	m_acceleration = 0.0f;

	// initialize rotation xforms
	UpdateRotation(0.0f);

	m_fireLoopSoundId = INVALID_SOUNDID;
	m_spinUpSoundId = INVALID_SOUNDID;

		Firing(false);
}

//------------------------------------------------------------------------
void CRapid::Update(float frameTime, uint32 frameId)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

  CSingle::Update(frameTime, frameId);

	if (m_speed <= 0.0f && m_acceleration < 0.0001f)
	{
		FinishDeceleration();
		return;
	}

	CActor* pOwnerActor = m_pWeapon->GetOwnerActor();
	bool isOwnerClient = pOwnerActor ? pOwnerActor->IsClient() : false;
	bool isOwnerPlayer = pOwnerActor ? pOwnerActor->IsPlayer() : false;

	m_pWeapon->RequireUpdate(eIUS_FireMode);

	m_speed = m_speed + m_acceleration*frameTime;

	if (m_speed > m_fireParams->rapidparams.max_speed)
	{
		m_speed = m_fireParams->rapidparams.max_speed;
		m_rapidFlags &= ~eRapidFlag_accelerating;
	}

	if ((m_speed >= m_fireParams->rapidparams.min_speed) && !(m_rapidFlags & eRapidFlag_decelerating))
	{
		float dt = 1.0f;
		if (cry_fabsf(m_speed)>0.001f && cry_fabsf(m_fireParams->rapidparams.max_speed>0.001f))
			dt=m_speed/m_fireParams->rapidparams.max_speed;
		m_next_shot_dt = 60.0f/(m_fireParams->fireparams.rate*dt);

		bool canShoot = CanFire(false);

		if (canShoot)
		{
			if (!OutOfAmmo())
			{
				if (m_rapidFlags & eRapidFlag_netShooting)
					Firing(true);
				else
					Firing(Shoot(true));

				if (m_firing && isOwnerClient && !(m_fireParams->rapidparams.camshake_rotate.IsZero() && m_fireParams->rapidparams.camshake_shift.IsZero()))
				{
					IView *pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
					if (pView)            
							pView->SetViewShake(Ang3(m_fireParams->rapidparams.camshake_rotate), m_fireParams->rapidparams.camshake_shift, m_next_shot_dt/m_fireParams->rapidparams.camshake_perShot, m_next_shot_dt/m_fireParams->rapidparams.camshake_perShot, 0, 1);            
				}
			}
			else
			{
				StopFire();
			}
		}
	}
	else if (m_firing)
	{
		StopFire();
		if (OutOfAmmo() && isOwnerPlayer)
		{
			m_pWeapon->Reload();
		}
	}

	if ((m_speed < m_fireParams->rapidparams.min_speed) && (m_acceleration < 0.0f) && !(m_rapidFlags & eRapidFlag_decelerating))
		Accelerate(m_fireParams->rapidparams.deceleration);

	UpdateRotation(frameTime);
	UpdateSound(frameTime);
}

//------------------------------------------------------------------------
void CRapid::StartReload(int zoomed)
{
	if (IsFiring())
		Accelerate(m_fireParams->rapidparams.deceleration);
	Firing(false);

	CSingle::StartReload(zoomed);
}

//------------------------------------------------------------------------
void CRapid::StartFire()
{
	if (m_pWeapon->IsBusy() || !CanFire(true))
	{
		//Clip empty sound
		if(!CanFire(true) && !m_reloading)
		{
			int ammoCount = m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class);

			if (GetClipSize()==0)
				ammoCount = m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class);

			if(ammoCount<=0)
			{
				m_pWeapon->PlayAction(m_fireParams->actions.empty_clip);
				m_pWeapon->OnFireWhenOutOfAmmo();
			}
		}
		return;
	}
	else if(m_pWeapon->IsWeaponLowered())
	{
		m_pWeapon->PlayAction(m_fireParams->actions.null_fire);
		return;
	}

	m_rapidFlags &= ~eRapidFlag_netShooting;

	m_pWeapon->EnableUpdate(true, eIUS_FireMode);

	//SpinUpEffect(true);
	Accelerate(m_fireParams->rapidparams.acceleration);

	m_rapidFlags |= eRapidFlag_startedFiring;
	m_firstFire = true;

	m_startFiringTime = gEnv->pTimer->GetAsyncTime();

	m_pWeapon->RequestStartFire();
}

//------------------------------------------------------------------------
void CRapid::StopFire()
{
	bool acceleratingOrDecelerating = (m_rapidFlags & (eRapidFlag_accelerating|eRapidFlag_decelerating)) != 0;
	if (!m_firing && (m_reloading || !acceleratingOrDecelerating))
		return;

	m_firing = false;
	m_rapidFlags &= ~(eRapidFlag_startedFiring|eRapidFlag_rapidFiring);

  if(m_acceleration >= 0.0f)
  {
	  Accelerate(m_fireParams->rapidparams.deceleration);

    if (m_pWeapon->IsDestroyed())
      FinishDeceleration();
  }

  SpinUpEffect(false);
	SmokeEffect();

	if (m_pWeapon->IsBusy() && !m_pWeapon->IsZoomingInOrOut())
		return;

	m_pWeapon->StopActionAnimations();
		
	const bool shouldTriggerStopFiringAnim =
		m_fireParams->rapidparams.min_firingTimeToStop==0.0f ||
		((gEnv->pTimer->GetAsyncTime().GetSeconds() - m_startFiringTime.GetSeconds()) > m_fireParams->rapidparams.min_firingTimeToStop);
	if (shouldTriggerStopFiringAnim)
	{
		//stop rapid fire sound before stop fire action
		if (m_fireLoopSoundId != INVALID_SOUNDID)
		{
			m_pWeapon->StopSound(m_fireLoopSoundId);
			m_fireLoopSoundId = INVALID_SOUNDID;
		}
		m_pWeapon->PlayAction(
			m_fireParams->actions.stop_rapid_fire, 0, false, CItem::eIPAF_Default,
			-1.0f, GetFireAnimationWeight(), GetFireFFeedbackWeight());
	}

	m_startFiringTime.SetSeconds(0.0f);

	m_pWeapon->RequestStopFire();
}

//------------------------------------------------------------------------
void CRapid::NetStartFire()
{
	m_rapidFlags |= eRapidFlag_netShooting;
	
	m_pWeapon->EnableUpdate(true, eIUS_FireMode);

	//SpinUpEffect(true);
	Accelerate(m_fireParams->rapidparams.acceleration);
}

//------------------------------------------------------------------------
void CRapid::NetStopFire()
{
	if(m_acceleration >= 0.0f)
	{
		Accelerate(m_fireParams->rapidparams.deceleration);

	  if (m_pWeapon->IsDestroyed())
		  FinishDeceleration();
	}
	
	SpinUpEffect(false);

	if(m_firing)
		SmokeEffect();

	m_firing = false;

	m_pWeapon->PlayAction(
		m_fireParams->actions.stop_rapid_fire, 0, false, CItem::eIPAF_Default,
		-1.0f, GetFireAnimationWeight(), GetFireFFeedbackWeight());
	m_pWeapon->ResetOwnerAnimationsInLayer(ITEM_OWNER_ACTION_LAYER);
	m_startFiringTime.SetSeconds(0.0f);

	m_rapidFlags |= ~ eRapidFlag_netShooting;

	if (m_fireLoopSoundId != INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_fireLoopSoundId);
		m_fireLoopSoundId = INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
float CRapid::GetSpinUpTime() const
{
	return m_fireParams->rapidparams.min_speed/m_fireParams->rapidparams.acceleration;
}

//------------------------------------------------------------------------
float CRapid::GetSpinDownTime() const
{
	return m_fireParams->rapidparams.max_speed/m_fireParams->rapidparams.deceleration;
}

//------------------------------------------------------------------------
void CRapid::Accelerate(float acc)
{
	m_acceleration = acc;

	if (acc > 0.0f)
	{
    if (!IsFiring())
      SpinUpEffect(true);

		m_rapidFlags |= eRapidFlag_accelerating;
		m_rapidFlags &= ~eRapidFlag_decelerating;

		m_spinUpSoundId = m_pWeapon->PlayAction(m_fireParams->actions.spin_up, 0, false, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending);
	}
	else
	{
		m_rapidFlags |= eRapidFlag_decelerating;
		m_rapidFlags &= ~eRapidFlag_accelerating;

		if(m_speed>0.0f)
		{
			m_pWeapon->PlayAction(m_fireParams->actions.spin_down);

			if(m_firing)
			{
				m_pWeapon->PlayAction(m_fireParams->actions.spin_down_tail);
			}
		}

		if (m_spinUpSoundId != INVALID_SOUNDID)
		{
			m_pWeapon->StopSound(m_spinUpSoundId);
			m_spinUpSoundId = INVALID_SOUNDID;
		}
		
    if (IsFiring())
		  SpinUpEffect(false);
	}
}

//------------------------------------------------------------------------
void CRapid::FinishDeceleration()
{
  m_rapidFlags &= ~eRapidFlag_decelerating;
  m_speed = 0.0f;
  m_acceleration = 0.0f;
  
  if (m_fireLoopSoundId != INVALID_SOUNDID)
  {
    m_pWeapon->StopSound(m_fireLoopSoundId);
    m_fireLoopSoundId = INVALID_SOUNDID;
  }
  
  m_pWeapon->EnableUpdate(false, eIUS_FireMode);  
}

//------------------------------------------------------------------------
void CRapid::Firing(bool firing)
{
	SpinUpEffect(false);

	if (m_firing != firing)
	{
		if (!m_firing)
			m_pWeapon->PlayAction(m_fireParams->actions.blast);
		else
			StopFire();
	}
	m_firing = firing;
}

//------------------------------------------------------------------------
void CRapid::UpdateRotation(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	m_rotation_angle -= m_speed*frameTime*2.0f*3.141592f;
	Ang3 angles(0,m_rotation_angle,0);

	int slot = (m_pWeapon->GetStats().fp || m_pWeapon->GetStats().mounted) ? eIGS_FirstPerson : eIGS_ThirdPerson;
	Matrix34 tm = Matrix33::CreateRotationXYZ(angles);
	if (!m_fireParams->rapidparams.barrel_attachment.empty())
		m_pWeapon->SetCharacterAttachmentLocalTM(slot, m_fireParams->rapidparams.barrel_attachment.c_str(), tm);
	if (!m_fireParams->rapidparams.engine_attachment.empty())
		m_pWeapon->SetCharacterAttachmentLocalTM(slot, m_fireParams->rapidparams.engine_attachment.c_str(), tm);
}

//------------------------------------------------------------------------
void CRapid::UpdateSound(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (m_speed >= 0.00001f)
	{	
		const uint32 decelAndRapid = eRapidFlag_decelerating|eRapidFlag_rapidFiring;

		if ((m_speed >= m_fireParams->rapidparams.min_speed) && !(m_rapidFlags & decelAndRapid))
		{
			m_rapidFlags |= eRapidFlag_rapidFiring;
			if(m_fireLoopSoundId != INVALID_SOUNDID)	//stop any previous playing sounds
			{
				m_pWeapon->StopSound(m_fireLoopSoundId);
				m_fireLoopSoundId = INVALID_SOUNDID;
			}
			m_fireLoopSoundId = m_pWeapon->PlayAction(
				m_fireParams->actions.rapid_fire, 0, true, CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation,
				-1.0f, 1.0f, GetFireFFeedbackWeight());
		}
		else if ((m_speed < m_fireParams->rapidparams.min_speed) && ((m_rapidFlags & decelAndRapid) ==  decelAndRapid))
		{
			m_rapidFlags &= ~eRapidFlag_rapidFiring;
			if(m_fireLoopSoundId != INVALID_SOUNDID)
			{
				m_pWeapon->StopSound(m_fireLoopSoundId);
				m_fireLoopSoundId = INVALID_SOUNDID;
			}
		}

		if (m_fireLoopSoundId != INVALID_SOUNDID)
		{
			ISound *pSound = m_pWeapon->GetISound(m_fireLoopSoundId);
			if (pSound)
			{
				float ammo = 1.0f;
				int clipSize = GetClipSize();
				int ammoCount = GetAmmoCount();
				if(clipSize > 0 && ammoCount >= 0)
				{
					ammo = (float) ammoCount / clipSize;
				}

#if !defined(_RELEASE)
				if (g_pGameCVars->i_debug_sounds)
				{
					CryWatch("ammo_left param %.2f", ammo);
				}
#endif
				pSound->SetParam("ammo_left", ammo, false);
			}

			if (m_speed >= m_fireParams->rapidparams.min_speed)      
				m_spinUpSoundId = INVALID_SOUNDID;      
		}

		if (CActor* pOwnerActor = m_pWeapon->GetOwnerActor())
		{
			if (m_firing)
			{
				if(ICharacterInstance* pOwnerCharacter = pOwnerActor->GetEntity()->GetCharacter(0))
				  pOwnerCharacter->GetISkeletonAnim()->SetAdditiveWeight(ITEM_OWNER_ACTION_LAYER, GetFireAnimationWeight());
			}
		}
	}
	else if (m_fireLoopSoundId != INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_fireLoopSoundId);
		m_fireLoopSoundId = INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
bool CRapid::AllowZoom() const
{
	return true; //!m_firing && !m_startedToFire;
}

const char *CRapid::GetType() const
{
	return "Rapid";
}

void CRapid::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));	
	CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
}


int CRapid::PlayActionSAFlags(int flags) {
	int ret = (flags | CItem::eIPAF_Animation);
	if (!m_fireParams->rapidparams.shoot_play_sound)
		ret &= ~CItem::eIPAF_Sound;
	return ret;
};


void CRapid::OnZoomStateChanged()
{
	if (m_firing)
	{
		m_pWeapon->PlayAction(
			m_fireParams->actions.rapid_fire, 0, true, (CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation) & ~CItem::eIPAF_Sound,
			-1.0f, GetFireAnimationWeight(), GetFireFFeedbackWeight());
	}
}

