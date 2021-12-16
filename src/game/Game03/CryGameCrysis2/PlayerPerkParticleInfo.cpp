/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Particle info for player perks ('Sonar Vision' and 'Cloak Awareness')
-------------------------------------------------------------------------
History:
- 22:5:2009: Created by Tim Furnish

*************************************************************************/

#include "StdAfx.h"
#include "PerkSonarVision.h"
#include "GameCVars.h"
#include "Player.h"

CPlayerPerkParticleInfo::CPlayerPerkParticleInfo()
{
	m_movementNoiseBurst = 0.f;

	for (int i=0; i < ePPPET_NumTypes; i ++)
	{
		m_emitterSlot[i] = -1;
	}
}

void CPlayerPerkParticleInfo::Reset()
{
	for (int i=0; i < ePPPET_NumTypes; i ++)
	{
		if (m_emitterSlot[i] != -1)
		{
			CryLog ("CPlayer::ResetScreenFX() <%p> - Setting m_emitterSlot[%d] from %d to -1", this, i, m_emitterSlot[i]);
			m_emitterSlot[i] = -1;
		}
	}
}

void CPlayerPerkParticleInfo::Tick(const SEntityUpdateContext& ctx)
{
	m_movementNoiseBurst *= expf(-CPerk::GetInstance()->GetVars()->perk_sonarVision_BoostFadeSpeed * ctx.fFrameTime);
}

void CPlayerPerkParticleInfo::AddNoiseBurst(float amount)
{
	m_movementNoiseBurst = max(m_movementNoiseBurst, amount);
}

bool CPlayerPerkParticleInfo::ShouldGenerateEffectsForEntity(EntityId thisEntity, CPlayer * pLocalPlayer, EPlayerPerkParticleElementID element)
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	if (perkVars->perk_particleTypesToHide & (1 << element))
	{
		return false;
	}

	switch (perkVars->perk_particleTypesGenerateOnWho)
	{
		case 0:
		return pLocalPlayer->IsFriendlyEntity(thisEntity) == false;
		break;

		case 1:
		return thisEntity != pLocalPlayer->GetEntityId();
		break;
	}

	return true;
}

IParticleEmitter * CPlayerPerkParticleInfo::GetEmitter(IEntity * pThisEntity, EPlayerPerkParticleEmitterType mode, const char * particleType)
{
	int slot = m_emitterSlot[mode];

	IParticleEmitter * pEmitter = pThisEntity->GetParticleEmitter(slot);

	if (pEmitter == NULL)
	{
		IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect(particleType);

		if (pEffect)
		{
			int slot = pThisEntity->LoadParticleEmitter(-1, pEffect);
			CryLog ("CPlayerPerkParticleInfo::GetEmitter() <%p> - Setting m_emitterSlot[%d] from %d to %d", this, mode, m_emitterSlot[mode], slot);
			m_emitterSlot[mode] = slot;
			pEmitter = pThisEntity->GetParticleEmitter(slot);
			pEmitter->SetRndFlags(ERF_RENDER_ALWAYS, true);
		}
	}

	return pEmitter;
}

float CPlayerPerkParticleInfo::CalculateBaseEffectScale(const Vec3 & pos, CPlayer * pLocalPlayer, float range, float reduceAmountWhenBusy)
{
	float scaleIt = 1.f - reduceAmountWhenBusy;
	range *= scaleIt;

	float distanceSquared = (pos - pLocalPlayer->GetEntity()->GetPos()).len2();
	float fractionOfRange = distanceSquared / (range * range);

	return (1.f - fractionOfRange) * scaleIt;
}

float CPlayerPerkParticleInfo::CalculateMovementModifierScale(IEntity * pThisEntity)
{
	ICharacterInstance *pCharacter = pThisEntity->GetCharacter(0);
	if (pCharacter)
	{
		ISkeletonAnim * pSkeletonAnim = pCharacter->GetISkeletonAnim();
		float overSpeedThreshold = pSkeletonAnim->GetCurrentVelocity().GetLength() + m_movementNoiseBurst - 0.5f;

		if (overSpeedThreshold > 0.f)
		{
			return overSpeedThreshold;
		}
	}

	return 0.f;
}

//////////////////////////////////////////////////////////////////////////
// Tests
