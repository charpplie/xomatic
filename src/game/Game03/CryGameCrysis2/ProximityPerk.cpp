#include "StdAfx.h"
#include "ProximityPerk.h"

#include "Player.h"
#include "HUD/HUD.h"
#include "GameCVars.h"
#include "Utility/CryWatch.h"
#include "RecordingSystem.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

ProximityPerk::ProximityPerk(const float *pMaxRange, const float *pScanSpeed1, const float *pScanSpeed2, const float *pScanSpeed3)
{
	m_range = 0.0f;
	m_lastDetectedRange = 0.0f;

	m_pMaxRange = pMaxRange;
	m_pScanSpeed[eTierOne] = pScanSpeed1;
	m_pScanSpeed[eTierTwo] = pScanSpeed2;
	m_pScanSpeed[eTierThree] = pScanSpeed3;
	m_sound = INVALID_SOUNDID;
}

void ProximityPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_Reset:
	case EPE_Spawn:
		{
			m_range = 0.0f;
			m_lastDetectedRange = 0.0f;
			break;
		}
	default:
		{	
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}

void ProximityPerk::Update(const float dt)
{
	bool inKillCam = g_pGame->GetRecordingSystem() && g_pGame->GetRecordingSystem()->IsPlayingBack();
	if (m_ownerPlayer->IsClient() && InRequiredSuitMode() && m_ownerPlayer->GetSpectatorMode() == CActor::eASM_None && !inKillCam)
	{
		float maxRange=(*m_pMaxRange);
		bool detectedUpdate = false;
		float normalizedRange = m_range/maxRange;
		float detectedDistance = 0.0f;

		if(m_range>maxRange)
		{
			m_range = 0.0f;
			m_lastDetectedRange = 0.0f;
			detectedUpdate = true;
		}
		else if (findClosestTarget(&detectedDistance))
		{
			CCCPOINT(Perk_ProximityAlarm_DetectedNearbyPlayer);
			m_lastDetectedRange = detectedDistance;
			audioFeedback(normalizedRange, true);
			m_range = 0.0f;
			detectedUpdate = true;
		}
		else
		{
			m_range += dt * (*m_pScanSpeed[GetTier()]);
		}

		updateHud(m_range, detectedUpdate, m_lastDetectedRange);

		PlayerPluginWatch("Range %.2f Detected %s", m_range, m_lastDetectedRange > 0.0f ? "true" : "false");
	}
}

bool ProximityPerk::findClosestTarget(float* detectedDist)
{
	Vec3 viewDir=gEnv->pRenderer->GetCamera().GetViewdir();
	Vec3 playerPos = m_ownerPlayer->GetEntity()->GetWorldPos();

	float maxRangeSq=(*m_pMaxRange) * (*m_pMaxRange);
	float minDistSq = maxRangeSq;

	// find target
	IActorSystem *pActorSystem=gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	IActorIterator *pActorIterator=pActorSystem->CreateActorIterator();
	while (CActor* pActor = (CActor*)pActorIterator->Next())
	{
		Vec3 actorPos = pActor->GetEntity()->GetWorldPos();

		//suitable actor
		if (m_ownerPlayer->IsFriendlyEntity(pActor->GetEntityId()) || pActor->GetHealth() <= 0 || pActor->GetSpectatorMode() != CActor::eASM_None)
			continue;

		float lenSq = (actorPos-playerPos).len2();
		minDistSq = min(minDistSq, lenSq);
	}

	float minDist = cry_sqrtf(minDistSq);
	if(minDist < m_range)
	{
		(*detectedDist) = minDist;
		return true;
	}

	return false;
}

void ProximityPerk::Leave()
{
	updateHud(0.0f, false, 0.0f);
	IPerk::Leave();
}

void ProximityPerk::updateHud(float range, bool detectedUpdate, float detectedRange)
{
	SHUDEvent event;
	event.eventType = eHUDEvent_OnScanPerkChanged;
	event.eventFloatData = range;
	event.eventFloat2Data = detectedRange;
	event.eventIntData = detectedUpdate?1:0;
	CHUD::CallEvent(event);
}

void ProximityPerk::audioFeedback(float noramlizedRange, bool detected)
{
	if(detected)
	{
		if(m_sound != INVALID_SOUNDID)
		{
			ISound* previous = gEnv->pSoundSystem->GetSound(m_sound);
			if(previous && previous->IsPlaying())
			{
				previous->Stop();
			}
		}

		// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound("Sounds/crysiswars2:perks:perk_cloakaware_beep", FLAG_SOUND_EVENT);
		if (pSound)
		{
			pSound->SetSemantic(eSoundSemantic_HUD);
			pSound->SetParam("proximity", 1.0f - noramlizedRange);
			pSound->Play();
			m_sound = pSound->GetId(); // query for ID after Play might result in INVALID_ID, which is ok if the sound already got culled
		}
	}
}