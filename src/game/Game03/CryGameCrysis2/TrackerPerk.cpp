#include "StdAfx.h"
#include "TrackerPerk.h"

#include "Perk.h"
#include "Player.h"
#include "GameRules.h"
#include "ParticleParams.h"
#ifndef _LIB
#include "ParticleParams_TypeInfo.h"
#include "Common_TypeInfo.h"
#endif

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

TrackerPerk::TrackerPerk(const char* particleEffect)
{
	if(gEnv->bMultiplayer)	//this doesn't actually want to be multiplayer only - but for now it fixes the assert from SP not having the asset (Jake told me too)
	{
		m_effect = gEnv->pParticleManager->FindEffect(particleEffect, "TrackerPerk::TrackerPerk");

		if (!GetISystem()->IsDedicated())
		{
			CRY_ASSERT_MESSAGE(m_effect, ("Unable to find particleEffect %s", particleEffect));
		}
	}
	else
	{
		m_effect = NULL;
	}
}

void TrackerPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_Footstep:
		{
			CRY_ASSERT(m_ownerPlayer->IsClient());
			if(m_effect)
			{
				EntityId entityId = *static_cast<EntityId*>(data);
				IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
				if(pEntity)
				{
					Vec3 particlePos = pEntity->GetWorldPos();
					if(!m_ownerPlayer->IsFriendlyEntity(entityId) && Distance::Point_Point(m_ownerPlayer->GetEntity()->GetWorldPos(), particlePos) < CPerk::GetInstance()->GetVars()->perk_tracker_maxDistanceToSpawn)
					{
						SetParticleLifetimeFromPlayerCount();
						m_effect->Spawn(true, IParticleEffect::ParticleLoc(particlePos, pEntity->GetForwardDir(), 1.0f));
						CCCPOINT(Perk_Tracker_PlacedTrackerParticle);
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

void TrackerPerk::SetParticleLifetimeFromPlayerCount()
{
	CRY_TODO(25, 11, 2009, "Adjusting the particle lifetime breaks the colour changing particle...");
	/*
	const ParticleParams& particleParams = m_effect->GetParticleParams();
	ParticleParams adjustedParams = particleParams;	//This is slow but currently the only way to do this with the particle system

	float lifetime = 5.0f;
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if(pGameRules)
	{
		const CPerk::SPerkVars *perkVars = CPerk::GetInstance()->GetVars();
		lifetime = (16.0f - pGameRules->GetPlayerCount()) * perkVars->perk_tracker_secondsPerFreePlayerSlot;
		lifetime = clamp(lifetime, perkVars->perk_tracker_minSeconds, perkVars->perk_tracker_maxSeconds);
		PlayerPluginLog("Lifetime %.2f with %d free player slots * %.2f then clamped(%.2f, %.2f)", lifetime, 16 - pGameRules->GetPlayerCount(), perkVars->perk_tracker_secondsPerFreePlayerSlot, perkVars->perk_tracker_minSeconds, perkVars->perk_tracker_maxSeconds);
	}
	adjustedParams.fParticleLifeTime = lifetime;
	m_effect->SetParticleParams(adjustedParams);
	*/
}
