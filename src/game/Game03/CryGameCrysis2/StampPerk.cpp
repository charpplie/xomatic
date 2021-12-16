#include "StdAfx.h"
#include "StampPerk.h"

#include "GameCVars.h"
#include "SShootHelper.h"
#include "Player.h"
#include "Game.h"
#include "GameActions.h"
#include "PerkIconData.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

StampPerk::StampPerk()
{
	m_timesToDisplayPrompt = CPerk::GetInstance()->GetVars()->perk_Stamp_displayPromptUntilStampedThisManyTimes;
	m_damage = 0;
	m_radius = 0.0f;
}

void StampPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_Reset:
		case EPE_Spawn:
		{
			m_rechargeTimer = 0.f;
			SetState(ESS_none);
			break;
		}
		case EPE_OverrideFallVelocity:
		{
			if(m_state == ESS_stamping)
			{
				Vec3* pDesiredVel = static_cast<Vec3*>(data);
				pDesiredVel->Set(0.0f, 0.0f, -CPerk::GetInstance()->GetVars()->perk_Stamp_fallspeed);
			}
			break;
		}
		case EPE_OverrideLandVelocity:
		{
			if(m_state == ESS_stamping || IsTier(eTierTwo))
			{
				Vec3* pLandVelocity = static_cast<Vec3*>(data);
				pLandVelocity->z = 0.0f;
			}
			break;
		}
		case EPE_StampMelee:
		{
			if(InRequiredSuitMode())
			{
				SOnActionData* actionData = static_cast<SOnActionData*>(data);
				if(actionData->activationMode == eAAM_OnPress && m_state == ESS_jumping && m_rechargeTimer <= 0.f)
				{
					CCCPOINT(Perk_Stamp_Triggered);
					actionData->handled = true;
					m_rechargeTimer = CPerk::GetInstance()->GetVars()->perk_Stamp_rechargeTime;
					SetState(ESS_stamping);
					m_ownerPlayer->GetAnimationGraphState()->SetInput("Action", "stamp");
					
					if (m_timesToDisplayPrompt > 0)
					{
						-- m_timesToDisplayPrompt;
					}
				}
			}
			break;
		}
		case EPE_Jump:
		SetState(ESS_jumping);
		break;

		case EPE_EnterSwimming:
		if(m_state == ESS_stamping)
		{
			m_ownerPlayer->GetAnimationGraphState()->SetInput("Action", "idle");
		}
		SetState(ESS_none);
		break;

		case EPE_Die:
		SetState(ESS_none);
		break;

		case EPE_Landed:
		{
			if(m_state == ESS_stamping)
			{
				CCCPOINT(Perk_Stamp_HitGround);
				m_ownerPlayer->GetAnimationGraphState()->SetInput("Action", "idle");
				float* pFallspeed = static_cast<float*>(data);
				m_damage = CalculateDamage(*pFallspeed);
				m_radius = CalculateRadius(*pFallspeed);
			}
			SetState(ESS_none);
		}

		case EPE_AnimationEvent:
		{
			const char* pAnimParameter = static_cast<const char*>(data);
			//damage can be zero if you stamp into water
			if(m_damage != 0 && strcmpi(pAnimParameter,"stampDamage") == 0)
			{
				CCCPOINT(Perk_Stamp_DoDamage);
				assert(m_state == ESS_none);
				EntityId entityId = m_ownerPlayer->GetEntityId();
				CAudioSignalPlayer::JustPlay("Perk_Stamp", entityId);
				SShootHelper::Explode(entityId, "stamp", m_ownerPlayer->GetEntity()->GetPos(), m_ownerPlayer->GetEntity()->GetForwardDir(), m_damage, m_radius);
				SNanoSuitEvent event;	//technically it's not a shot be we want to nanosuit to react in the same way
				event.event = eNanoSuitEvent_SHOT;
				m_ownerPlayer->SendActorSuitEvent(event);
				m_damage = 0;
				m_radius = 0.0f;
			}

			break;
		}
		default:
		{
			IPerk::HandleEvent(perkEvent, data);
			break;
		}
	}
}


int StampPerk::CalculateDamage(float fallspeed)
{
	const CPerk::SPerkVars* pPerkVars = CPerk::GetInstance()->GetVars();

	PlayerPluginAssert(pPerkVars->perk_Stamp_maxDamageSpeed > pPerkVars->perk_Stamp_minDamageSpeed, "Max damage speed %f, min damage speed %f", pPerkVars->perk_Stamp_maxDamageSpeed, pPerkVars->perk_Stamp_minDamageSpeed);
	PlayerPluginAssert(pPerkVars->perk_Stamp_maxDamage > pPerkVars->perk_Stamp_minDamage, "Max damage %f, min damage %f", pPerkVars->perk_Stamp_maxDamage, pPerkVars->perk_Stamp_minDamage);

	float clampedFallspeed = clamp(fallspeed, pPerkVars->perk_Stamp_minDamageSpeed, pPerkVars->perk_Stamp_maxDamageSpeed);

	float damageRange = pPerkVars->perk_Stamp_maxDamage - pPerkVars->perk_Stamp_minDamage;
	float fallspeedRange = pPerkVars->perk_Stamp_maxDamageSpeed - pPerkVars->perk_Stamp_minDamageSpeed;

	float normalizedFallspeed = (clampedFallspeed - pPerkVars->perk_Stamp_minDamageSpeed)/fallspeedRange;
	PlayerPluginAssert(normalizedFallspeed >= 0.0f && normalizedFallspeed <= 1.0f, "Normalized fall speed should be between 0 and 1, but it's %f", normalizedFallspeed);

	int damage = (int) ((normalizedFallspeed * damageRange) + pPerkVars->perk_Stamp_minDamage);

	PlayerPluginLog("Fallspeed %.2f equates to Damage %d", fallspeed, damage);
	return damage;
}

float StampPerk::CalculateRadius(float fallspeed)
{
	const CPerk::SPerkVars* pPerkVars = CPerk::GetInstance()->GetVars();

	float clampedFallspeed = clamp(fallspeed, pPerkVars->perk_Stamp_minDamageSpeed, pPerkVars->perk_Stamp_maxDamageSpeed);

	float damageRadiusRange = pPerkVars->perk_Stamp_maxRadius - pPerkVars->perk_Stamp_minRadius;
	float fallspeedRange = pPerkVars->perk_Stamp_maxDamageSpeed - pPerkVars->perk_Stamp_minDamageSpeed;

	float normalizedFallspeed = (clampedFallspeed - pPerkVars->perk_Stamp_minDamageSpeed)/fallspeedRange;
	PlayerPluginAssert(normalizedFallspeed >= 0.0f && normalizedFallspeed <= 1.0f, "Normalized fall speed should be between 0 and 1, but it's %f", normalizedFallspeed);

	float radius = ((normalizedFallspeed * damageRadiusRange) + pPerkVars->perk_Stamp_minRadius);

	PlayerPluginLog("Fallspeed %.2f equates to Radius %.2f", fallspeed, radius);

	return radius;
}

const void* StampPerk::GetData(EPlayerPlugInData dataType)
{
	switch(dataType)
	{
		case EPD_Stamp:
		{
			m_stamping = (m_state == ESS_stamping);
			return &m_stamping;
		}
	}

	return NULL;
}

void StampPerk::Update(const float dt)
{
	m_rechargeTimer = max(m_rechargeTimer - dt, 0.f);
	CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());

	if (iconData)
	{
		bool fullyCharged = m_rechargeTimer <= 0.f;

		iconData->SetIconDrainAmount(m_perkId, m_rechargeTimer / CPerk::GetInstance()->GetVars()->perk_Stamp_rechargeTime, fullyCharged);

		if (InRequiredSuitMode() && m_state == ESS_jumping && fullyCharged && m_timesToDisplayPrompt > 0)
		{
			// Create a prompt telling the player to use their stamp perk!
			// NB: Seriously, it's called "special"? THAT'S the name of the melee input? Sheesh! Why's it not called "melee"? [TF]
			CControllerInputRenderInfo promptInfo;
			promptInfo.CreateForInput("crysis2_common", "special");

			CUIButtonPromptRegion::SetOnScreenMessageText("buttonPrompts", "@ui_prompt_stamp", & promptInfo);
		}
	}
}

void StampPerk::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == CPlayer::ASPECT_PERK_STAMP_CLIENT)
	{
		if(ser.IsWriting())
		{
			m_stamping = (m_state == ESS_stamping);
		}
		bool previousStamp = m_stamping;
		ser.Value("stamp", m_stamping);
		if(ser.IsReading() && !previousStamp && m_stamping)
		{
			m_state = ESS_stamping;
			m_ownerPlayer->GetAnimationGraphState()->SetInput("Action", "stamp");
		}
	}
}

void StampPerk::SetState(EStampState state)
{
	m_state = state;
	CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_STAMP_CLIENT);
}