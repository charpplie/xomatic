#include "StdAfx.h"
#include "ResistantPerk.h"

#include "Player.h"
#include "Perk.h"
#include "GameRules.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

ResistantPerk::ResistantPerk(EPlayerPlugInEvent type, const char* hitType, const float *pDamageMultiplier, const char* feedbackSignal, const char * particleName)
{
	m_damageModifierType = type;
	m_hitTypeId = g_pGame->GetGameRules()->GetHitTypeId(hitType);
	m_pDamageMultiplier = pDamageMultiplier;
	m_feedback.SetSignal(feedbackSignal);
	m_particleEffectName = particleName;
}

ResistantPerk::~ResistantPerk()
{
	if(IsDeflectionPerk())
	{
		if(m_deflectionScreenEffect.IsFlagSet(GAME_EFFECT_INITIALISED))
		{
			m_deflectionScreenEffect.Release();
		}
	}
}

void ResistantPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_DamageHandlingShooter:
		case EPE_DamageHandlingTarget:
		{
			if(m_damageModifierType == perkEvent)
			{
				PlayerPluginAssert(gEnv->bServer, "EPE_DamageHandling[shooter|target] should only be used on a server");
				SDamageHandling* pDamageInfo = static_cast<SDamageHandling*>(data);

				if(pDamageInfo->pHitInfo->type == m_hitTypeId && InRequiredSuitMode() && 
					((pDamageInfo->pHitInfo->shooterId != m_ownerPlayer->GetEntityId() && perkEvent == EPE_DamageHandlingTarget) || (perkEvent == EPE_DamageHandlingShooter)))
				{
					float fractionOn = CalculateFractionApplicable(pDamageInfo->pHitInfo);
					if (fractionOn > 0.f)
					{
						float scaleDamage = 1.f - (1.f - * m_pDamageMultiplier) * fractionOn;

						pDamageInfo->damageMultiplier *= scaleDamage;

						CCCPOINT_IF(m_perkId == ePerk_MeleeDefense, Perk_MeleeDefense_ScaledDamage);
						CCCPOINT_IF(m_perkId == ePerk_FragResistant, Perk_FragResistant_ScaledDamage);
						CCCPOINT_IF(m_perkId == ePerk_Deflection, Perk_Deflection_ScaledDamage);
						CCCPOINT_IF(m_perkId == ePerk_RearDeflection, Perk_RearDeflection_ScaledDamage);
						CCCPOINT_IF(m_perkId == ePerk_ArmourPiercing, Perk_ArmourPiercing_ScaledDamage);

						PlayerPluginLog("Fraction on = %.3f, scaling %s damage by %.3f (full effect = %.3f) - multiplier is now %.3f", fractionOn, g_pGame->GetGameRules()->GetHitType(m_hitTypeId), scaleDamage, *m_pDamageMultiplier, pDamageInfo->damageMultiplier);
					}
				}
			}
			break;
		}
	
		case EPE_DamageHandlingFeedback:
		{
			const HitInfo * info = static_cast<HitInfo*>(data);

			EntityId ownerId = m_ownerPlayer->GetEntityId();
			if(info->type == m_hitTypeId && InRequiredSuitMode() &&
				((m_damageModifierType == EPE_DamageHandlingShooter && ownerId == info->shooterId) ||
				(m_damageModifierType == EPE_DamageHandlingTarget && ownerId == info->targetId)))
			{
				float fractionOn = CalculateFractionApplicable(info);
				if (fractionOn > 0.f)
				{
					CCCPOINT_IF(m_perkId == ePerk_MeleeDefense, Perk_MeleeDefense_AudioAndVisuals);
					CCCPOINT_IF(m_perkId == ePerk_FragResistant, Perk_FragResistant_AudioAndVisuals);
					CCCPOINT_IF(m_perkId == ePerk_Deflection, Perk_Deflection_AudioAndVisuals);
					CCCPOINT_IF(m_perkId == ePerk_RearDeflection, Perk_RearDeflection_AudioAndVisuals);
					CCCPOINT_IF(m_perkId == ePerk_ArmourPiercing, Perk_ArmourPiercing_Audio);

					m_feedback.Play(m_ownerPlayer->GetEntityId());
					if(fractionOn != 1.0f)
					{
						m_feedback.SetParam(m_ownerPlayer->GetEntityId(), "angle", fractionOn);
					}

					if (m_particleEffectName)
					{
						PlayerPluginAssert(! info->pos.IsZero(), "Want to do particle effect '%s' but hit position is zero, i.e. info not been initialized properly");
						PlayerPluginAssert(  info->dir.IsUnit(), "Want to do particle effect '%s' but hit direction isn't a unit vector, i.e. info not been initialized properly");

						IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect(m_particleEffectName, "Particle.SpawnEffect");
						if (pEffect)
						{
							pEffect->Spawn(true, IParticleEffect::ParticleLoc(info->pos, info->dir * -fractionOn, 2.f * fractionOn));
						}
						else
						{
							PlayerPluginLog("No particle effect by the name of '%s'", m_particleEffectName);
						}
					}

					// Handle deflection 1st person effect
					bool isPlayerClient = (info->targetId == gEnv->pGame->GetIGameFramework()->GetClientActorId());
					if(isPlayerClient && (IsDeflectionPerk()))
					{ 
						m_deflectionScreenEffect.Deflect(m_ownerPlayer->GetEntity()->GetRotation(),info->dir);
					}
				}
			}
			break;
		}

		default:
		IPerk::HandleEvent (perkEvent, data);
		break;
	}
}

void ResistantPerk::Enter()
{
	IPerk::Enter();

	if(IsDeflectionPerk())
	{
		if(m_deflectionScreenEffect.IsFlagSet(GAME_EFFECT_INITIALISED) == false)
		{
			SDeflectionGameEffectParams deflectionScreenEffectParams;
			deflectionScreenEffectParams.screenEffectId = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects()->GetEffectIdByName("cw2_player_fx","cw2_deflection");
			deflectionScreenEffectParams.autoUpdatesWhenActive = false;
			m_deflectionScreenEffect.Initialise(&deflectionScreenEffectParams);
			m_deflectionScreenEffect.SetActive(true);
		}
	}
}
