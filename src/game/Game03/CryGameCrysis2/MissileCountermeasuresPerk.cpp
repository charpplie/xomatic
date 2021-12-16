#include "StdAfx.h"
#include "MissileCountermeasuresPerk.h"

#include "GameCVars.h"
#include "SShootHelper.h"
#include "Player.h"
#include "Perk.h"
#include "Projectile.h"
#include "PerkIconData.h"
#include "GameRules.h"
#include "Battlechatter.h"

MissileCountermeasuresPerk::MissileCountermeasuresPerk()
{
	m_timer = 0.0f;
	m_numTimesFired = 0;
	m_everRead = false;
}

void MissileCountermeasuresPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_Reset:
	case EPE_Spawn:
		{
			m_timer = 0.f;
			break;
		}
	case EPE_SvActivateMissileCountermeasures:
		{
			if(m_timer == 0.0f)
			{
				ActivateCountermeasures();
				assert(gEnv->bServer);
			}
			
			break;
		}
	default:
		{
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}

void MissileCountermeasuresPerk::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == CPlayer::ASPECT_PERK_MISSILECOUNTERMEASURES_SERVER)
	{
		uint8 newNumTimesFired = m_numTimesFired;

		ser.Value("numTimesFired", newNumTimesFired, 'ui2');

		if (ser.IsReading())
		{
			if (m_everRead == false)
			{
				m_numTimesFired = newNumTimesFired;
				m_everRead = true;
			}
			else
			{
				if (m_numTimesFired != newNumTimesFired)
				{
					ActivateCountermeasures();
					m_numTimesFired = newNumTimesFired;
				}
			}
		}

	}
}

void MissileCountermeasuresPerk::Update(const float dt)
{
	m_timer = max(m_timer - dt, 0.f);

	CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());
	if (iconData)
	{
		float timeFraction = clamp(m_timer/CPerk::GetInstance()->GetVars()->perk_chaffRechargeTime, 0.0f, 1.0f);
		iconData->SetIconDrainAmount(ePerk_MissileCountermeasures, timeFraction, m_timer == 0.0f);
	}
}

void MissileCountermeasuresPerk::ActivateCountermeasures()
{
	const float k_spread = CPerk::GetInstance()->GetVars()->perk_chaffSpread;
	const float k_spreadD2 = k_spread * 0.5f;

	Vec3 dir((k_spread * cry_frand())-k_spreadD2, (k_spread * cry_frand())-k_spreadD2, 1.0f);
	dir.normalize();
	Vec3 pos = m_ownerPlayer->GetEntity()->GetPos() + dir;

	SShootHelper::Shoot(m_ownerPlayer->GetEntityId(), "chaff", "frag", pos, dir, 0);

	CAudioSignalPlayer::JustPlay("Perk_MissileCountermeasures", pos);

	// Increase m_numTimesFired by 1: m_numTimesFired is net serialized
	// NB: This number wraps around to 0 quite quickly (after only a few strikes) to reduce bandwidth
	m_numTimesFired ++;
	m_numTimesFired &= 3;

	if(gEnv->bServer)
	{
		CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_MISSILECOUNTERMEASURES_SERVER);
	}

	m_timer = CPerk::GetInstance()->GetVars()->perk_chaffRechargeTime;

	BATTLECHATTER(BC_MissileCountermeasures, m_ownerPlayer->GetEntityId());
}
