#include "StdAfx.h"
#include "PlayerPlugin_Perk_SniperCountermeasures.h"
#include "Player.h"
#include "PerkIconData.h"
#include "IWorldQuery.h"
#include "GameCVars.h"
#include "Utility/CryWatch.h"
#include "GameRules.h"
#include "Battlechatter.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

// Maintain a doubly-linked list of entered instances, for use in static GetInstanceBelongingTo function
static CPlayerPlugin_Perk_SniperCountermeasures * s_first = NULL;
static CPlayerPlugin_Perk_SniperCountermeasures * s_last = NULL;

void CPlayerPlugin_Perk_SniperCountermeasures::Enter()
{
	if (s_first)
	{
		s_first->m_prev = this;
	}
	else
	{
		s_last = this;
	}

	m_prev = NULL;
	m_next = s_first;
	s_first = this;

	m_cumulativePowerDrain = 0.f;
	m_beingTargetted = false;

	IPerk::Enter();

	if(m_ownerPlayer->IsClient())
	{
		m_targettedSignal.SetSignal("SniperCountermeasuresTargetted");
		m_isPlayingTargetted = false;
	}
}

void CPlayerPlugin_Perk_SniperCountermeasures::Leave()
{
	IPerk::Leave();

	if (this == s_first) { assert (m_prev == NULL); s_first = m_next; } else { assert (m_prev); assert (m_prev->m_next == this); m_prev->m_next = m_next; }
	if (this == s_last)  { assert (m_next == NULL); s_last = m_prev;  } else { assert (m_next); assert (m_next->m_prev == this); m_next->m_prev = m_prev; }

	m_next = NULL;
	m_prev = NULL;
}

CPlayerPlugin_Perk_SniperCountermeasures * CPlayerPlugin_Perk_SniperCountermeasures::GetInstanceBelongingTo(EntityId who)
{
	for (CPlayerPlugin_Perk_SniperCountermeasures * eachOne = s_first; eachOne; eachOne=eachOne->m_next)
	{
		if (eachOne->m_ownerPlayer->GetEntityId() == who)
		{
			return eachOne;
		}
	}

	return NULL;
}

void CPlayerPlugin_Perk_SniperCountermeasures::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_Reset:
		case EPE_Spawn:
		m_perkAntiSniperEnergy = 1.f;
		break;

		case EPE_FlashbangScale:
		if(IsTier(eTierTwo))
		{
			const float scale = CPerk::GetInstance()->GetVars()->perk_countermeasuresFlashBangScale;
			float* pData = (float*) data;
			(*pData) *= scale;

			CAudioSignalPlayer::JustPlay("Perk_VisorProtector", m_ownerPlayer->GetEntityId());
			CCCPOINT(Perk_VisorProtector_ScaleFlashbangEffect);			
		}
		break;
	}

	IPerk::HandleEvent (perkEvent, data);
}

void CPlayerPlugin_Perk_SniperCountermeasures::Update(const float dt)
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();

	if (m_beingTargetted && InRequiredSuitMode())
	{
		BATTLECHATTER(BC_SniperCountermeasures, m_ownerPlayer->GetEntityId());
		m_perkAntiSniperEnergy = max(perkVars->perk_antiSniperEnergyMinimum, m_perkAntiSniperEnergy - dt * perkVars->perk_antiSniperEnergyUseRate * m_cumulativePowerDrain);
		m_cumulativePowerDrain = 0.f;
	}
	else
	{
		m_perkAntiSniperEnergy = min(1.f, m_perkAntiSniperEnergy + dt * perkVars->perk_antiSniperEnergyRechargeRate);
	}

	CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());

	if (iconData)
	{
		float energyFraction = (m_perkAntiSniperEnergy - perkVars->perk_antiSniperEnergyMinimum) / (1.f - perkVars->perk_antiSniperEnergyMinimum);
		iconData->SetIconDrainAmount(ePerk_Countermeasures, 1.f - energyFraction, m_beingTargetted);

		if(m_beingTargetted)
		{
			if(!m_isPlayingTargetted)
			{
				m_targettedSignal.Play(m_ownerPlayer->GetEntityId());
				m_isPlayingTargetted = true;
			}
		}
		else
		{
			if(m_isPlayingTargetted)
			{
				m_targettedSignal.Stop(m_ownerPlayer->GetEntityId());
				m_isPlayingTargetted = false;
			}
		}
	}

	m_beingTargetted = false;
}

//===================================================================================

void CPlayerPlugin_TriggerSniperCountermeasures::Enter()
{
	m_currentTarget = 0;
	m_scopeActive = false;
	m_feedbackAmount = 0.0f;

	CPlayerPlugin::Enter();
	m_targettingSignal.SetSignal("SniperCountermeasuresTargetting");
}

void CPlayerPlugin_TriggerSniperCountermeasures::Update(const float dt)
{
	// Update current target for local player only - for remote players, m_currentTarget will be set in NetSerialize
	if (m_ownerPlayer->IsClient())
	{
		EntityId targetId = (m_scopeActive) ? m_ownerPlayer->GetGameObject()->GetWorldQuery()->GetLookAtEntityId() : 0;

		if (m_currentTarget != targetId)
		{
			m_currentTarget = targetId;
			CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_SNIPERCOUNTERMEASURES_CLIENT);
		}
	}

	// Inform current target that he's being watched with a sniper rifle, and update how much screen/sway feedback THIS player gets for targeting him
	float changeSwayAmount = dt * -0.5f;

	if (m_currentTarget)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_currentTarget);
		if(pActor && pActor->GetHealth() > 0 && !m_ownerPlayer->IsFriendlyEntity(m_currentTarget))
		{
			CPlayerPlugin_Perk_SniperCountermeasures * sniperCounter = CPlayerPlugin_Perk_SniperCountermeasures::GetInstanceBelongingTo(m_currentTarget);
			if (sniperCounter && sniperCounter->IsEntered())
			{
				sniperCounter->InformBeingLookedAt();

				if (sniperCounter->InRequiredSuitMode() && sniperCounter->GetPowerRemaining() > 0.f)
				{
					changeSwayAmount = dt * 2.f;
				}
			}
		}
	}

	float fNewFeedbackAmount = clamp(m_feedbackAmount + changeSwayAmount, 0.f, 1.f);

	if (m_ownerPlayer->IsClient())
	{
		gEnv->p3DEngine->SetPostEffectParam("AlienInterference_Amount", fNewFeedbackAmount * fNewFeedbackAmount * CPerk::GetInstance()->GetVars()->perk_antiSniperScreenEffectAmount);
		
		EntityId ownerId = m_ownerPlayer->GetEntityId();
		bool isPlaying = m_targettingSignal.IsPlaying(ownerId);

		if(fNewFeedbackAmount > 0.0f && !isPlaying)
		{
			m_targettingSignal.Play(ownerId);
		}
		else if (fNewFeedbackAmount == 0.0f && isPlaying)
		{
			m_targettingSignal.Stop(ownerId);
		}		
	}

	m_feedbackAmount = fNewFeedbackAmount;
}

void CPlayerPlugin_TriggerSniperCountermeasures::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == CPlayer::ASPECT_PERK_SNIPERCOUNTERMEASURES_CLIENT)
	{
		ser.Value("antiSniperTarget", m_currentTarget, 'eid');
	}
}

void CPlayerPlugin_TriggerSniperCountermeasures::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_ScopeActive:
		PlayerPluginLog ("Scope active changing from %d to %d", m_scopeActive, *(bool*)data);
		m_scopeActive = *(bool*)data;
		break;

		case EPE_OverrideSwayAmount:
		{
			float * theValue = (float *)data;
			float additional = CPerk::GetInstance()->GetVars()->perk_antiSniperSwayMult * m_feedbackAmount;
			PlayerPluginWatch ("Sway %.3f=>%.3f (feedbackAmount=%.2f)", *theValue, *theValue + additional, m_feedbackAmount);
			*theValue += additional;
		}
		break;

		case EPE_SetTeam:
		case EPE_Die:
		m_ownerPlayer->LeavePlayerPlugin(this);
		break;
	}

	CPlayerPlugin::HandleEvent(perkEvent, data);
}
