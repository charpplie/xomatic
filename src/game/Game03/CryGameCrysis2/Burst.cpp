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
#include "Burst.h"
#include "Actor.h"

#include "WeaponSharedParams.h"


//------------------------------------------------------------------------
CBurst::CBurst()
	:	m_fireTriggerDown(false)
{
}

//------------------------------------------------------------------------
CBurst::~CBurst()
{
}

//------------------------------------------------------------------------
void CBurst::Update(float frameTime, uint32 frameId)
{
	CSingle::Update(frameTime, frameId);

	if (m_bursting)
	{
		if (m_next_shot <= 0.0f)
		{
			// HACK: save current burst rate, and fake it so that the CanFire check in CSingle::Shoot passes...
			float saved_next_burst=m_next_burst;
			m_next_burst=0.0f;

			Shoot(true);
			m_burst_shot = m_burst_shot+1;

			if (!m_firing || (m_burst_shot >= m_fireParams->burstparams.nshots))
			{
				m_firing = false;
				m_bursting = false;
				m_burst_shot = 1;
				SmokeEffect();
				m_pWeapon->PlayAction(m_fireParams->actions.spin_down_tail);
			}

			m_next_burst=saved_next_burst;
		}
	}

	m_next_burst -= frameTime;
	if (m_next_burst <= 0.0f)
		m_next_burst = 0.0f;
}

//------------------------------------------------------------------------
void CBurst::Activate(bool activate)
{
	CSingle::Activate(activate);

	m_next_burst = 0.0f;
	m_next_burst_dt = 60.0f/(float)m_fireParams->burstparams.rate;
	m_bursting = false;
	m_burst_shot = 1;
	m_fireTriggerDown = false;
}

//------------------------------------------------------------------------
bool CBurst::CanFire(bool considerAmmo) const
{
	return CSingle::CanFire(considerAmmo) /*&& m_next_burst<=0.0f*/;
}

//------------------------------------------------------------------------
void CBurst::StartFire()
{
	if (!m_bursting && !m_fireTriggerDown)
	{
		m_fireTriggerDown = true;
		if (m_next_burst <= 0.0f)
		{
			CSingle::StartFire();

			if(m_fired)	//Only set if first shot was successful
			{
				int flags = CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_CleanBlending|CItem::eIPAF_NoBlend;

				IEntityClass* ammo = GetShared()->fireparams.ammo_type_class;
				
				// We need to add 1, because Single decreased one the ammo count already
				int ammoCount = (m_pWeapon->GetAmmoCount(ammo) + 1); 

				tSoundID burstSoundId = m_pWeapon->PlayAction(
					GetBurstFireAction(ammoCount),
					0, false, flags, -1.0f,
					GetFireAnimationWeight(), GetFireFFeedbackWeight());
				m_next_burst = m_next_burst_dt;
				m_bursting = true;

				if (m_fireParams->burstparams.useBurstSoundParam)
				{
					SetBurstSoundParam(burstSoundId, ammoCount);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CBurst::StopFire()
{
	m_fireTriggerDown = false;
}

//------------------------------------------------------------------------
const char *CBurst::GetType() const
{
	return "Burst";
}

//------------------------------------------------------------------------
const ItemString& CBurst::GetBurstFireAction(const int ammoCount) const
{
	switch(ammoCount)
	{
	case 1:
		return m_fireParams->actions.last1_fire;

	case 2:
		return m_fireParams->actions.last2_fire;
	}

	return m_fireParams->actions.burst_fire;
}

void CBurst::SetBurstSoundParam( tSoundID burstSoundId, const int ammoCount ) const
{
	if (burstSoundId != INVALID_SOUNDID)
	{
		CRY_ASSERT (ammoCount > 0);
		if (ISound* pBurstSound = m_pWeapon->GetISound(burstSoundId))
		{
			const int maxBurstShots = m_fireParams->burstparams.nshots;
			pBurstSound->SetParam("burst", (ammoCount > maxBurstShots) ? (float)maxBurstShots : (float)ammoCount);
		}
	}
}