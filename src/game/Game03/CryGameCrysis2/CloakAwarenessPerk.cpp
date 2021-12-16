#include "StdAfx.h"
#include "CloakAwarenessPerk.h"

#include "Player.h"
#include "Perk.h"



CloakAwarenessPerk::CloakAwarenessPerk(const float* range, const float* timeTillFlicker1, const float* timeTillFlicker2, const float* timeTillFlicker3, const float* flickerTime)
{
	m_flickerTimer = 0.0f;

	m_timeTillFlicker[eTierOne] = timeTillFlicker1;
	m_timeTillFlicker[eTierTwo] = timeTillFlicker2;
	m_timeTillFlicker[eTierThree] = timeTillFlicker3;

	m_range = range;
	m_suitFlickerTime = flickerTime;
}

void CloakAwarenessPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_PlayerCloaking:
		{
			if(m_ownerPlayer->IsClient())
			{
				EntityId* pId = (EntityId*) data;
				EntityId entityId = *pId;
				IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId);
				if(pActor)
				{
					CPlayer* pPlayer = static_cast<CPlayer*>(pActor);

					if(ShouldGenerateParticles(pPlayer))
					{
						const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
						GenerateParticles(pPlayer, perkVars->perk_cloakAwareness_skelAlignedParticles_numToCreate);
					}
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

void CloakAwarenessPerk::Update(const float dt)
{
	if(m_ownerPlayer->IsClient())
	{
		m_flickerTimer+= dt;
		if(m_flickerTimer > (*m_timeTillFlicker[GetTier()]))
		{
			m_flickerTimer = 0.0f;

			IActorIteratorPtr pIt = g_pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
			while (IActor* pActor = pIt->Next())
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
				if(pPlayer->GetActorSuitGameParameters().IsCloakEnabled() && ShouldGenerateParticles(pPlayer))
				{
					const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
					GenerateParticles(pPlayer, perkVars->perk_cloakAwareness_skelAlignedParticles_numToCreateWhenMoving);
				}
			}
		}
	}
}

bool CloakAwarenessPerk::ShouldGenerateParticles(CPlayer* pPlayer)
{
	Vec3 worldPos = m_ownerPlayer->GetEntity()->GetWorldPos();
	float distance = worldPos.GetDistance(pPlayer->GetEntity()->GetWorldPos());
	if(distance < (*m_range))
	{
		return CPlayerPerkParticleInfo::ShouldGenerateEffectsForEntity(pPlayer->GetEntityId(), m_ownerPlayer, ePPPEI_CloakAwareness);
	}

	return false;
}

void CloakAwarenessPerk::GenerateParticles(CPlayer* pPlayer, int amount)
{
	pPlayer->GenerateCloakAwarenessParticles(amount);
}
