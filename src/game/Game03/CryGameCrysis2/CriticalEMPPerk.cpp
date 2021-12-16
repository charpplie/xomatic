#include "StdAfx.h"
#include "CriticalEMPPerk.h"

#include "GameCVars.h"
#include "SShootHelper.h"
#include "Player.h"
#include "Perk.h"
#include "Projectile.h"
#include "PerkIconData.h"

void CriticalEMPPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_Reset:
		case EPE_Spawn:
		{
			m_timer = 0.f;
			break;
		}
		case EPE_OverrideHealth:
		{
			int* pHealth = (int*) data;
			int health = *pHealth;
			const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
			if(health > 0 && health < perkVars->perk_empHealthTriggerAmount && health < m_ownerPlayer->GetHealth() && InRequiredSuitMode())
			{
				if(m_timer <= 0.f)
				{
					Explode(perkVars);
				}
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

void CriticalEMPPerk::Update(const float dt)
{
	m_timer = max(m_timer - dt, 0.f);
	CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());

	if (iconData)
	{
		iconData->SetIconDrainAmount(m_perkId, m_timer / CPerk::GetInstance()->GetVars()->perk_criticalEMPRechargeTime, m_timer <= 0.f);
	}
}

void CriticalEMPPerk::Explode(const CPerk::SPerkVars * perkVars)
{
	IEntity *pEntity = m_ownerPlayer->GetEntity();
	SShootHelper::Explode(m_ownerPlayer->GetEntityId(), m_weaponName, pEntity->GetPos(), pEntity->GetForwardDir(), 0);
	m_timer = perkVars->perk_criticalEMPRechargeTime;
}
