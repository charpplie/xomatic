/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2008.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Perception handling for puppets using Crysis method
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "CrysisPerceptionHandler.h"
#include "AIPlayer.h"
#include "AIVehicle.h"
#include "Puppet.h"

REGISTER_PERCEPTION_HANDLER(CCrysisPerceptionHandler)

//////////////////////////////////////////////////////////////////////////
CCrysisPerceptionHandler::CCrysisPerceptionHandler()
{

}

//////////////////////////////////////////////////////////////////////////
CCrysisPerceptionHandler::~CCrysisPerceptionHandler()
{

}

//////////////////////////////////////////////////////////////////////////
bool CCrysisPerceptionHandler::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
	int eventCount = m_TargetMap.size();
	ser.Value("eventCount", eventCount);

	// The events should be empty while reading.
	if (ser.IsReading())
		AIAssert(m_TargetMap.empty());

	AIAssert(m_pendingEvents.empty());
	
	char eventName[32];
	while(--eventCount>=0)
	{
		CWeakRef<CAIObject> refEventObject;
		SAIPotentialTarget eventData;
		if(ser.IsWriting())
		{
			PotentialTargetMap::iterator iEvent = m_TargetMap.begin();
			std::advance(iEvent, eventCount);
			refEventObject = iEvent->first;
			eventData = iEvent->second;
		}
		sprintf(eventName, "Event_%d", eventCount);
		ser.BeginGroup(eventName);
		{
			refEventObject.Serialize(ser, "refEventObject");
			eventData.Serialize(ser, objectTracker);
		}
		ser.EndGroup();
		if(ser.IsReading())
			m_pendingEvents.push_back(TPendingEvent(refEventObject, eventData));
	}

	return !m_pendingEvents.empty();
}

//////////////////////////////////////////////////////////////////////////
void CCrysisPerceptionHandler::PostSerialize(bool bReading)
{
	CBasePerceptionHandler::PostSerialize(bReading);

	//[AlexMc|03.02.10] these asserts might not be true if base or child classes start requesting postserialization
	AIAssert(bReading);
	AIAssert(!m_pendingEvents.empty());

	if (bReading)
	{
		TPendingEvents::iterator it = m_pendingEvents.begin();
		TPendingEvents::const_iterator itEnd = m_pendingEvents.end();
		for (; it != itEnd; ++it)
		{
			AIAssert(it->first.IsValid());
			AddEvent(it->first, it->second);
		}

		TPendingEvents().swap(m_pendingEvents); // Effective STL #17, the swap trick: empty the array and free its memory
	}
}

//////////////////////////////////////////////////////////////////////////
float CCrysisPerceptionHandler::GetPriorityMultiplier(CAIObject *pTarget, const AgentParameters& parameters) const
{
	float fObjectMultiplier = 1.0f;
	float fSpeciesMultiplier = 1.0f;
	float fResult = fObjectMultiplier;

	if (!pTarget)
		return fResult;

	// find object type multiplier, if one exists
	CAISystem::MapMultipliers::iterator mi = GetAISystem()->m_mapMultipliers.find(pTarget->GetType());
	if (mi != GetAISystem()->m_mapMultipliers.end())
		fObjectMultiplier = mi->second;

		int targetSpecies(-1);
	bool targetSpeciesHostility(false);
	CAIActor* pTargetActor = pTarget->CastToCAIActor();
	if (pTargetActor)
	{
		AgentParameters apTarget = pTargetActor->GetParameters();
		targetSpecies = apTarget.m_nSpecies;
		targetSpeciesHostility = apTarget.m_bSpeciesHostility;
	}
	// find species multiplier, if one exists
	mi = GetAISystem()->m_mapSpeciesThreatMultipliers.find(targetSpecies); 
	if (mi != GetAISystem()->m_mapSpeciesThreatMultipliers.end())
		fSpeciesMultiplier = mi->second;

	// species multiplication
	if (parameters.m_nSpecies != targetSpecies &&
		(parameters.m_bSpeciesHostility || targetSpecies < 0) &&
		(targetSpeciesHostility || parameters.m_nSpecies < 0))
		fResult = fSpeciesMultiplier * fObjectMultiplier;

	return fResult;
}

//////////////////////////////////////////////////////////////////////////
bool CCrysisPerceptionHandler::GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CRY_ASSERT(m_refOwner.IsValid());
	refBestTarget.Reset();
	pTargetInfo = NULL;
	bCurrentTargetErased = false;

	CPuppet *pOwner = m_refOwner.GetAIObject();
	const float fTimePassed = pOwner->m_fTimePassed;
	const IAIObject *pAttentionTarget = pOwner->GetAttentionTarget();
	const AgentParameters &parameters = pOwner->GetParameters();

	// Decay the current attention target preference.
	float fAttTargetPersistenceTimeout = pOwner->GetExtraPriority();
	const float targetPersistence = parameters.m_PerceptionParams.targetPersistence;
	if (fAttTargetPersistenceTimeout > 0.0f && pAttentionTarget)
	{
		const float dt = pAttentionTarget->GetSubType() == IAIObject::STP_MEMORY ? fTimePassed * 3.0f : fTimePassed;
		fAttTargetPersistenceTimeout = max(0.0f, fAttTargetPersistenceTimeout - dt);
	}
	const float curTargetPersistence = fAttTargetPersistenceTimeout / (targetPersistence > 0.0f ? targetPersistence : 1.0f);

	float bestTargetPriority = 0.0f;
	PotentialTargetMap::iterator itTarget = m_TargetMap.begin();
	for (; itTarget != m_TargetMap.end(); )
	{
		SAIPotentialTarget& ed = itTarget->second;
		CAIObject* pEventOwner = itTarget->first.GetAIObject();
		CAIActor* pEventOwnerActor = pEventOwner->CastToCAIActor();
		CAIObject *pDummyRep = ed.refDummyRepresentation.GetAIObject();

		// Update event sound
		float soundExposure = 0.0f;
		if (ed.soundTime > 0.0f)
		{
			ed.soundTime -= fTimePassed;
			if (ed.soundTime < 0.0f)
			{
				ed.soundTime = 0.0f;
				ed.soundMaxTime = 0.0f;
				ed.soundThreatLevel = 0.0f;
				ed.soundPos.Set(0,0,0);
			}
			else
			{
				soundExposure = LinStep(0.0f, ed.soundMaxTime * 0.8f, ed.soundTime) * ed.soundThreatLevel;
			}
		}

		// Update vis event
		float visualExposure = 0.0f;
		if (ed.visualFrameId == GetAISystem()->GetAITickCount())
		{
			// Handle delay
			if (ed.visualTime < parameters.m_PerceptionParams.sightDelay)
			{
				ed.visualType = SAIPotentialTarget::VIS_NONE;
				ed.visualTime = min(ed.visualTime+fTimePassed, ed.visualMaxTime);
			}
			else
			{
				ed.visualType = SAIPotentialTarget::VIS_VISIBLE;
				ed.visualPos = pEventOwner->GetPos();
				ed.visualTime = ed.visualMaxTime;

				// Set threat
				visualExposure = ed.visualThreatLevel;
			}
		}
		else if (ed.visualTime > FLT_EPSILON)
		{
			if (ed.visualType != SAIPotentialTarget::VIS_NONE && ed.visualTime >= (ed.visualMaxTime - 0.25f))
			{
				ed.visualType = SAIPotentialTarget::VIS_MEMORY;

				// (MATT) This could be a bugfix even for Crysis, but deserves dedicated testing :/ {2008/09/02}
				if (gAIEnv.configuration.eCompatibilityMode != ECCM_CRYSIS)
				{
					// Wipe any sound info, so that we don't confuse memory and sound
					// Maybe do this other way around also
					ed.soundTime = 0.0f;
					ed.soundMaxTime = 0.0f;
					ed.soundThreatLevel = 0.0f;
					ed.soundPos.zero();
				}
			}

			visualExposure = (ed.visualMaxTime > FLT_EPSILON ? (ed.visualTime/ed.visualMaxTime) : 0.0f);
			ed.visualTime = max(ed.visualTime-fTimePassed, 0.0f);
			if (ed.visualTime <= FLT_EPSILON)
			{
				ed.visualTime = 0.0f;
				ed.visualMaxTime = 0.0f;
				ed.visualThreatLevel = 0.0f;
			}
		}

		// Reaction speed.
		float exposureTarget = max(visualExposure, soundExposure);		

		float delta = exposureTarget - ed.exposure;
		if (delta > 0.0f)
		{
			float changeTime = pOwner->IsAlarmed() ? gAIEnv.CVars.SOMSpeedCombat : gAIEnv.CVars.SOMSpeedRelaxed;
			changeTime *= 1.0f - exposureTarget*0.5f;
			float rate = min(fTimePassed / changeTime, 1.0f);
			if (delta > rate) delta = rate;
			ed.exposure += delta;
		}
		else
		{
			// TODO: Parametrize
			float changeTime = parameters.m_PerceptionParams.forgetfulnessTarget / (pOwner->IsAlarmed() ? 4 : 8); //IsAlarmed() ? gAIEnv.CVars.SOMSpeedCombat : gAIEnv.CVars.SOMSpeedRelaxed;
			float rate = min(fTimePassed / changeTime, 1.0f);
			if (delta < -rate) delta = -rate;

			ed.exposure += delta;
		}

		EAITargetThreat newThreat = AITHREAT_NONE;
		float newTimeout = 0.0f;
		if (ed.exposure > PERCEPTION_AGGRESSIVE_THR)
		{
			newThreat = AITHREAT_AGGRESSIVE;
			newTimeout = parameters.m_PerceptionParams.forgetfulnessTarget;
			pOwner->SetAlarmed();
		}
		else if (ed.exposure > PERCEPTION_THREATENING_THR)
		{
			newThreat = AITHREAT_THREATENING;
			newTimeout = parameters.m_PerceptionParams.forgetfulnessSeek;
		}
		else if (ed.exposure > PERCEPTION_INTERESTING_THR)
		{
			newThreat = AITHREAT_INTERESTING;
			newTimeout = parameters.m_PerceptionParams.forgetfulnessMemory;
		}


		if (newThreat >= ed.threat)
		{
			ed.threat = newThreat;
			ed.threatTimeout = newTimeout;
		}
		ed.exposureThreat = newThreat;

		// Update the dummy location and direction.
		if (pDummyRep)
		{
			if (visualExposure >= soundExposure && !ed.visualPos.IsZero())
			{
				Vec3 dir = ed.visualPos - pDummyRep->GetPos();
				dir.Normalize();
				pDummyRep->SetPos(ed.visualPos, dir);
			}
			else if (!ed.soundPos.IsZero() && ed.exposure > PERCEPTION_INTERESTING_THR)
			{
				Vec3 dir = ed.soundPos - pDummyRep->GetPos();
				dir.Normalize();
				pDummyRep->SetPos(ed.soundPos);
			}
		}

		// Remove the potential target if the target is invisible or dead.
		if (pEventOwnerActor && (pEventOwnerActor->GetParameters().m_bInvisible || !pEventOwnerActor->IsActive()))
		{
			if (pEventOwner == pAttentionTarget || pDummyRep == pAttentionTarget)
				bCurrentTargetErased = true;
			m_TargetMap.erase(itTarget++);
			continue;
		}

		ed.threatTimeout -= fTimePassed;
		if (ed.threatTimeout < 0.0f)
		{
			if (ed.threat == AITHREAT_AGGRESSIVE)
			{
				// Enemy timed out.
				ed.threat = AITHREAT_THREATENING;
				ed.threatTimeout = parameters.m_PerceptionParams.forgetfulnessSeek;
			}
			else if (ed.threat == AITHREAT_THREATENING)
			{
				// Threatening timed out.
				ed.threat = AITHREAT_INTERESTING;
				ed.threatTimeout = parameters.m_PerceptionParams.forgetfulnessMemory;
				if (ed.visualType == SAIPotentialTarget::VIS_MEMORY)
					ed.visualType = SAIPotentialTarget::VIS_NONE;
			}
			else
			{
				// Interesting timed out.
				// Other timeouts have passed too - remove the event.
				if (ed.soundTime < 0.0001f && ed.visualTime < 0.0001f)
				{
					if (pEventOwner == pAttentionTarget || pDummyRep == pAttentionTarget)
						bCurrentTargetErased = true;
					m_TargetMap.erase(itTarget++);
					continue;
				}
			}
		}

		// Figure out event type.
		ed.type = AITARGET_NONE;
		if (ed.threat >= AITHREAT_INTERESTING)
		{
			if (ed.visualType == SAIPotentialTarget::VIS_VISIBLE)
			{
				// Do not allow indirect sightings to get aggressive because of sounds.
				if (ed.indirectSight && ed.threat >= AITHREAT_AGGRESSIVE && ed.soundThreatLevel > PERCEPTION_INTERESTING_THR)
					ed.type = AITARGET_SOUND;
				else
					ed.type = AITARGET_VISUAL;
			}
			else if (ed.visualType == SAIPotentialTarget::VIS_MEMORY)
			{
				ed.type = AITARGET_MEMORY;
				pDummyRep->SetSubType(IAIObject::STP_MEMORY);
			}
			else
			{
				ed.type = AITARGET_SOUND;
				pDummyRep->SetSubType(IAIObject::STP_SOUND);
			}

			// Update priority
			switch (ed.threat)
			{
			case AITHREAT_AGGRESSIVE: ed.priority = 1.5f; break;
			case AITHREAT_THREATENING: ed.priority = 1.0f; break;
			case AITHREAT_INTERESTING: ed.priority = 0.5f; break;
			default: ed.priority = 0.0f; break;
			}

			// 1) Use the distance as the base priority.
			Vec3 targetPos;
			if (ed.type == AITARGET_VISUAL && ed.threat == AITHREAT_AGGRESSIVE)
				targetPos = pEventOwner->GetPos();	
			else
				targetPos = pDummyRep->GetPos();
			float distanceMultiplier = 0.0f;
			float fSightRange = pOwner->GetMaxTargetVisibleRange(pEventOwner);
			if(fSightRange != 0.0f)
			{
				distanceMultiplier = 1.0f - min(Distance::Point_Point(targetPos, pOwner->GetPos()) / fSightRange, 1.0f);
			}
			ed.priority += distanceMultiplier;

			// 2) Scale the priority down for agents which are not making any sound.
			// This also includes the weapon sounds, so targets firing their weapons
			// will get higher priority.
			ed.priority *= 0.8f + soundExposure * 0.2f;

			// 3) Scale by the combined species and object type multiplier.
			ed.priority *= GetPriorityMultiplier(pEventOwner, parameters);

			// 4) Bias the current target.
			if (pEventOwner == pAttentionTarget)
				ed.priority += curTargetPersistence;
			ed.upPriorityTime -= fTimePassed;
			if (ed.upPriorityTime < 0.0f)
			{
				ed.upPriority = 0.0f;
				ed.upPriorityTime = 0.0f;
			}
			ed.priority += ed.upPriority;

			// Bias the selection of the most threatening target based on the combat class scale.
			const int targetCombatClass = pEventOwnerActor ? pEventOwnerActor->GetParameters().m_CombatClass : -1;
			float	scaledPriority = ed.priority * GetAISystem()->GetCombatClassScale(parameters.m_CombatClass, targetCombatClass);
			if (scaledPriority > 0.01f && scaledPriority > bestTargetPriority)
			{
				bestTargetPriority = scaledPriority;
				if (ed.type == AITARGET_VISUAL && ed.threat == AITHREAT_AGGRESSIVE)
					refBestTarget = GetWeakRef(pEventOwner);
				else
					refBestTarget = ed.refDummyRepresentation.GetWeakRef();
				pTargetInfo = &ed;
			}
		}
		else
		{
			ed.priority = 0.0f;
			ed.upPriorityTime = 0.0f;
			ed.upPriority = 0.0f;
		}

		// Affect stealth-o-meter for players
		ed.threatTime = 0.0f;
		switch (ed.threat)
		{
		case AITHREAT_AGGRESSIVE:
			{
				float t = ed.threatTimeout / parameters.m_PerceptionParams.forgetfulnessTarget;
				ed.threatTime = PERCEPTION_THREATENING_THR + (PERCEPTION_AGGRESSIVE_THR - PERCEPTION_THREATENING_THR) * t;
			}
			break;
		case AITHREAT_THREATENING:
			{
				float t = ed.threatTimeout / parameters.m_PerceptionParams.forgetfulnessSeek;
				ed.threatTime = PERCEPTION_INTERESTING_THR + (PERCEPTION_THREATENING_THR - PERCEPTION_INTERESTING_THR) * t;
			}
			break;
		case AITHREAT_INTERESTING:
			{
				float t = ed.threatTimeout / parameters.m_PerceptionParams.forgetfulnessMemory;
				ed.threatTime = PERCEPTION_INTERESTING_THR * t;
			}
			break;
		}

		AffectSOM(pOwner, pEventOwner, ed.exposure, ed.threatTime);

		++itTarget;
	}

	return (refBestTarget.IsValid());
}

//////////////////////////////////////////////////////////////////////////
void CCrysisPerceptionHandler::HandleSoundEvent(SAIEVENT* pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CPuppet *pOwner = m_refOwner.GetAIObject();
	if (!pOwner || !pOwner->m_bCanReceiveSignals)
		return;

	const AgentParameters &parameters = pOwner->GetParameters();

	Vec3 eventPos = pEvent->vPosition;
	const float fDistanceToEvent = Distance::Point_Point(eventPos, pOwner->GetPos());

	float fEventRadius = pEvent->fThreat;
	float fSoundThreatLevel = 0.0f;
	float fSoundTime = 0.0f;

	// Get descriptor
	SSoundPerceptionDescriptor sDescriptor;
	if (!pOwner->GetSoundPerceptionDescriptor((EAISoundStimType)pEvent->nType, sDescriptor))
		CRY_ASSERT_MESSAGE(0, "Missing Sound Perception Descriptor when handling a sound event");

	// Check minimum distance
	if (fabsf(sDescriptor.fMinDist) <= FLT_EPSILON || fDistanceToEvent > sDescriptor.fMinDist)
	{
		fEventRadius *= sDescriptor.fRadiusScale;
		const float fDistNorm = (fEventRadius > FLT_EPSILON ? fDistanceToEvent / fEventRadius : 0.0f);
		fSoundThreatLevel = sDescriptor.fBaseThreat * LinStep(sDescriptor.fLinStepMin, sDescriptor.fLinStepMax, fDistNorm);
		fSoundTime = sDescriptor.fSoundTime;
	}

	// Make sure that at max level the value reaches over the threshold.
	fSoundThreatLevel *= 1.1f;

	// Attenuate by perception parameter scaling.
	const float paramScale = parameters.m_PerceptionParams.audioScale * parameters.m_PerceptionParams.perceptionScale.audio;
	fSoundThreatLevel *= paramScale;

	// Skip really quiet sounds or too far away sounds
	if (fSoundThreatLevel < FLT_EPSILON || fDistanceToEvent > fEventRadius)
		return;

	IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
	CAIObject* pEventOwnerAIObject = pEventOwnerEntity ? (CAIObject*)pEventOwnerEntity->GetAI() : NULL;
	if (pEventOwnerAIObject)
	{
		if (CPuppet* pPuppet = pEventOwnerAIObject->CastToCPuppet())
		{
			if (!pOwner->IsHostile(pEventOwnerAIObject))
			{
				if (pEvent->nType == AISOUND_WEAPON)
				{
					// A living agent from the same species is shooting,
					// make the sound at the position where the agent is shooting at
					// and decoy it as an enemy.
					pEventOwnerAIObject = (CAIObject*)pPuppet->GetAttentionTarget();
					if (!pEventOwnerAIObject || !pEventOwnerAIObject->IsAgent())
						return;
					eventPos = pEventOwnerAIObject->GetPos();
					float fDistanceToTarget = Distance::Point_Point(eventPos, pOwner->GetPos());
					// (Kevin) Should below be distanceToTarget?
					float fDistNorm = min(fDistanceToEvent / fEventRadius, 1.0f);

					fSoundThreatLevel = sDescriptor.fBaseThreat * LinStep(sDescriptor.fLinStepMin, sDescriptor.fLinStepMax, fDistNorm);
				}
				else if (pEvent->nType != AISOUND_EXPLOSION)
				{
					return;
				}
			}
		}
		else
		{
			// Handle enemy sound.
			if (!pOwner->IsHostile(pEventOwnerAIObject))
				return;
		}
	}

	PotentialTargetMap::iterator ei = m_TargetMap.find(GetWeakRef(pEventOwnerAIObject));
	if (ei != m_TargetMap.end())
	{
		SAIPotentialTarget &ed = ei->second;

		// If there is already a VISUAL or MEMORY target, just don't do anything
		if ((ed.type == AITARGET_NONE) || (ed.type == AITARGET_SOUND))
		{
			if (fSoundThreatLevel >= ed.soundThreatLevel)
			{
				ed.soundTime = fSoundTime;
				ed.soundMaxTime = fSoundTime;
				ed.soundThreatLevel = fSoundThreatLevel;
				ed.soundPos = eventPos;
			}
		}
		else if (ed.type == AITARGET_MEMORY && gAIEnv.configuration.eCompatibilityMode == ECCM_WARFACE)
		{
			// Update the position to the sound position
			ed.visualPos = eventPos;
			ed.soundTime = fSoundTime;
			ed.soundMaxTime = fSoundTime;
			ed.soundThreatLevel = fSoundThreatLevel;
			ed.soundPos = eventPos;
		}
	}
	else
	{
		SAIPotentialTarget ed;

		ed.soundTime = fSoundTime;
		ed.soundMaxTime = fSoundTime;
		ed.soundThreatLevel = fSoundThreatLevel;
		ed.soundPos = eventPos;

		static int nameCounter = 0;
		char szName[256];
		_snprintf(szName,256,"Perception of %s S%d", pEventOwnerAIObject != NULL ? pEventOwnerAIObject->GetName() : "NOBODY", nameCounter++);
		GetAISystem()->CreateDummyObject(ed.refDummyRepresentation, szName);
		CAIObject *pDummyRep = ed.refDummyRepresentation.GetAIObject();

		if (pEventOwnerAIObject)
		{
			pDummyRep->SetAssociation(GetWeakRef(pEventOwnerAIObject));
			AddEvent(GetWeakRef(pEventOwnerAIObject), ed);
		}
		else
		{
			pDummyRep->SetEntityID(pEvent->sourceId);
			AddEvent(ed.refDummyRepresentation.GetWeakRef(), ed);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CCrysisPerceptionHandler::HandleVisualStimulus(SAIEVENT* pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CRY_ASSERT(m_refOwner.IsValid());

	CPuppet *pOwner = m_refOwner.GetAIObject();
	if (!pOwner || !pOwner->m_bCanReceiveSignals)
		return;

	const AgentParameters &parameters = pOwner->GetParameters();

	IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
	CAIObject* pEventOwner = pEventOwnerEntity ? (CAIObject*)pEventOwnerEntity->GetAI() : NULL;

	if (pEventOwner)
	{
		float fObjectMultiplier = 1.f;		
		CAISystem::MapMultipliers::iterator mi = GetAISystem()->m_mapMultipliers.find(pEventOwner->GetType());
		if (mi != GetAISystem()->m_mapMultipliers.end())
			fObjectMultiplier = mi->second;
		if (fObjectMultiplier > 1.0f)
			pOwner->m_bCanReceiveSignals = true;
	}

	// Update existing event if possible.
	CWeakRef<CAIObject> refEventAssociation = GetWeakRef(pEventOwner);
	if (pEventOwner && pEventOwner->GetType() == AIOBJECT_ATTRIBUTE)
		refEventAssociation = pEventOwner->GetAssociation();
	if (!refEventAssociation.IsValid())
		return;
	CAIObject *pEventAssociation = refEventAssociation.GetAIObject();

	const bool isAttribute = pEventOwner->GetType() == AIOBJECT_ATTRIBUTE;

	const float fRadius = pOwner->GetRadius();

	float distToTarget = Distance::Point_Point(pOwner->GetPos(), pEventOwner->GetPos());
	float sightRange = pOwner->GetMaxTargetVisibleRange(pEventOwner);
	const float envMin = parameters.m_PerceptionParams.sightEnvScaleNormal;
	const float envMax = parameters.m_PerceptionParams.sightEnvScaleAlarmed;
	float sightRangeThr = sightRange * (envMin + pOwner->GetPerceptionAlarmLevel() * (envMax - envMin));

	float sightRangeScale = 1.0f;
	float sightScale = 1.0f;
	float fPerceptionModifier = 1.0f;

	// Reduce sight range for attributes.
	if (isAttribute)
		sightRangeScale *= 0.5f;

	float stanceSize = 1.0f;
	float sightRangeMax = sightRange;
	CAIActor* pEventOwnerActor = pEventOwner->CastToCAIActor();
	if (pEventOwnerActor)
	{
		// Target stance
		SAIBodyInfo bi;
		if (pEventOwnerActor->GetProxy())
			pEventOwnerActor->GetProxy()->QueryBodyInfo(bi);
		float targetHeight = bi.stanceSize.GetSize().z;
		if (pEventOwnerActor->GetType() == AIOBJECT_VEHICLE)
			targetHeight = 0.0f;
		if (targetHeight > 0.0f)
			stanceSize = targetHeight / parameters.m_PerceptionParams.stanceScale;

		fPerceptionModifier = GetAISystem()->GetRayPerceptionModifier( pOwner->GetPos(), pEventOwner->GetPos(), pOwner->GetName() );

		bool targetUsesCombatLight = pEventOwnerActor->IsUsingCombatLight();
		if (targetUsesCombatLight || isAttribute)
			pOwner->SetAlarmed();

		// First calculate factors that will affect the sight range.

		// Target under water
		float fCachedWaterOcclusionValue = pEventOwnerActor->GetCachedWaterOcclusionValue();
		sightRangeScale *= 0.1f + (1 - fCachedWaterOcclusionValue) * 0.9f;

		if (!targetUsesCombatLight)
		{
			sightRangeScale *= 0.25f + stanceSize * 0.75f;
		}

/*			// calculate movement factor
		// harder to see if far and not moving
		float	vel2(0.0f);
		if(pAITargetObject->GetPhysics())
		{
			pe_status_dynamics	dyn;
			pAITargetObject->GetPhysics()->GetStatus(&dyn);
			vel2 = dyn.v.len2();
		}
		fNewIncrease*=(parameters.m_PerceptionParams.velBase + parameters.m_PerceptionParams.velScale*vel2);*/

		// Target scale factor
		if (parameters.m_PerceptionParams.bThermalVision)
			sightRangeScale *= pEventOwnerActor->m_Parameters.m_PerceptionParams.heatScale;
		else
			sightRangeScale *= pEventOwnerActor->m_Parameters.m_PerceptionParams.camoScale;

		// Flowgraph Target scale factor
		sightRangeScale *= parameters.m_PerceptionParams.perceptionScale.visual;

		// Secondly calculate factors that will affect the actual sighting value.

		// Scale the sighting based on the agent FOV.
		float fFOVPrimaryCos = 0.0f;
		float fFOVSecondaryCos = 0.0f;
		pOwner->GetSightFOV(fFOVPrimaryCos, fFOVSecondaryCos);
		if (fFOVPrimaryCos > 0.0f)
		{
			Vec3 dirToTarget = pEventOwnerActor->GetPos() - pOwner->GetPos();
			Vec3 viewDir = pOwner->GetViewDir();
			if (!pOwner->IsUsing3DNavigation())
			{
				dirToTarget.z = 0;
				viewDir.z = 0;
			}
			dirToTarget.Normalize();
			viewDir.Normalize();

			float dot = viewDir.Dot(dirToTarget);
			float fovFade = 1.0f;
			if (dot < fFOVPrimaryCos)
			{
				// Spread the FOV slightly when alarmed if the prim and sec fovs are different
				if (fabsf(fFOVSecondaryCos - fFOVPrimaryCos) > 0.001f)
				{
					const float thr = fFOVPrimaryCos + (fFOVSecondaryCos - fFOVPrimaryCos) * 0.5f * pOwner->GetPerceptionAlarmLevel();
					fovFade = LinStep(fFOVSecondaryCos, thr, dot);
				}
			}

			// When the target is really close, reduce the effect of the FOV.
			if (distToTarget < fRadius * 2)
				fovFade = max(fovFade, LinStep(fRadius * 2, fRadius, distToTarget));

			sightScale *= fovFade;
		}

		// Target cloak
		if (pEventOwnerActor->IsCloakEffective())
		{
			// Cloak on and not carrying anything
			// Fade out the perception increment based on the cloak distance parameters.
			float cloakMinDist = pEventOwnerActor->GetCloakMinDist();
			float cloakMaxDist = min(pEventOwnerActor->GetCloakMaxDist(), sightRange);

			sightScale *= 1.f - LinStep(cloakMinDist, cloakMaxDist, distToTarget)*pEventOwnerActor->m_Parameters.m_fCloakScale;
		}

		// Stance overrides
		if (gAIEnv.CVars.EnablePerceptionStanceVisibleRange != 0)
		{
			SAIBodyInfo bodyInfo;
			pEventOwnerActor->GetProxy()->QueryBodyInfo(bodyInfo);
			if (bodyInfo.stance == STANCE_CROUCH)
			{
				sightRangeMax = gAIEnv.CVars.CrouchVisibleRange;
			}
			else if (bodyInfo.stance == STANCE_PRONE)
			{
				sightRangeMax = gAIEnv.CVars.ProneVisibleRange;
			}
		}
	}

	// Calculate final threat level.
	float threatLevel = 0.0f;

	sightRange *= sightRangeScale * fPerceptionModifier;
	sightRangeThr *= sightRangeScale * fPerceptionModifier;

	// if target is outside new, modified sight range, dont't do anything about it
	if (sightRange > 0.0001f && sightRangeMax > distToTarget)
	{
		if (fabsf(sightRange - sightRangeThr) < 1e-6)
			threatLevel = 1.0f;
		else
			threatLevel = LinStep(sightRange, sightRangeThr, distToTarget);
		threatLevel *= sightScale;

		// Do not allow attributes to go aggressive.
		if (pEventOwner->GetType() == AIOBJECT_ATTRIBUTE)
		{
			const float thr = PERCEPTION_AGGRESSIVE_THR * 0.95f;
			threatLevel = min(threatLevel, thr);
		}

		if (threatLevel > 0.0001f)
		{
			// The threat level is considerable enough to be registered.

			// Reuse existing event or create a new one of the current target has not been seen yet.
			SAIPotentialTarget* pEvent = 0;
			PotentialTargetMap::iterator ei = m_TargetMap.find(refEventAssociation);
			if (ei == m_TargetMap.end())
			{
				// The event for this target does not exists yet, add new.
				FRAME_PROFILER("AddToVisibleList > new",gEnv->pSystem,PROFILE_AI );
				// This object has no associated event with it - create a new potential target
				SAIPotentialTarget ed;
				static int nameCounter = 0;
				char szName[256];
				_snprintf(szName,256,"Perception of %s V%d", pEventAssociation != NULL ? pEventAssociation->GetName() : "NOBODY", nameCounter++);
				GetAISystem()->CreateDummyObject(ed.refDummyRepresentation, szName);
				CAIObject *pDummyRep = ed.refDummyRepresentation.GetAIObject();
				pDummyRep->SetSubType(IAIObject::STP_MEMORY);
				pDummyRep->SetAssociation(refEventAssociation);

				pEvent = AddEvent(refEventAssociation, ed);
			}
			else
			{
				pEvent = &(ei->second);
			}

			if (!pEvent)
				return;

			// Combine the max threat level per target per update.
			if (pEvent->visualFrameId != GetAISystem()->GetAITickCount() || threatLevel > pEvent->visualThreatLevel)
			{
				if (pEventOwner->GetType() == AIOBJECT_ATTRIBUTE)
					pEvent->visualPos = pEventAssociation->GetPos();
				else
					pEvent->visualPos = pEventOwner->GetPos();
				pEvent->visualThreatLevel = threatLevel;
				pEvent->visualMaxTime = 3.0f;
				pEvent->visualFrameId = GetAISystem()->GetAITickCount();
				pEvent->indirectSight = pEventOwner->GetType() == AIOBJECT_ATTRIBUTE;
			}
		}
		else
		{
			// If the target exists already, zero out the threat value.
			SAIPotentialTarget* pEvent = 0;
			PotentialTargetMap::iterator ei = m_TargetMap.find(refEventAssociation);
			if (ei != m_TargetMap.end())
			{
				pEvent = &(ei->second);
				pEvent->visualThreatLevel = 0.0f;
			}
		}
	}
}

//===================================================================
// AffectSOM
//===================================================================
void CCrysisPerceptionHandler::AffectSOM(CPuppet* pOwner, CAIObject* pTarget, float exposure, float threat)
{
	assert(pOwner);
	if (pOwner->IsHostile(pTarget))
	{
		// affect stealth-o-meter for players
		if (CAIPlayer* pPlayer = pTarget->CastToCAIPlayer())
		{
			if (GetType() == AIOBJECT_VEHICLE)
				pPlayer->RegisterDetectionLevels(SAIDetectionLevels(0,0, exposure, threat));
			else
				pPlayer->RegisterDetectionLevels(SAIDetectionLevels(exposure, threat, 0,0));
		}
		else if (CAIVehicle* pVehicle = pTarget->CastToCAIVehicle()) // if this is player's vehicle
		{
			CAIPlayer* pPlayer2( (CAIPlayer*)GetAISystem()->GetPlayer() );
			if (	 !pPlayer2 	// no player? should not happen, just in case
				||  pPlayer2->GetParameters().m_nSpecies != pVehicle->GetParameters().m_nSpecies  // enemy vehicle, but not player's
				|| !pVehicle->IsPlayerInside())	// player is not driving (to make sure all the friendly vehicles don't affect SOM)
			{
				// Enemy vehicle
				if (GetType() == AIOBJECT_VEHICLE)
					pVehicle->RegisterDetectionLevels(SAIDetectionLevels(0,0, exposure, threat));
				else
					pVehicle->RegisterDetectionLevels(SAIDetectionLevels(exposure, threat, 0,0));
			}
			else
			{
				// Players vehicle.
				if (GetType() == AIOBJECT_VEHICLE)
					pPlayer2->RegisterDetectionLevels(SAIDetectionLevels(0,0, exposure, threat));
				else
					pPlayer2->RegisterDetectionLevels(SAIDetectionLevels(exposure, threat, 0,0));
			}
		}
		else if (CPuppet* pPuppet = pTarget->CastToCPuppet())
		{
			if (GetType() == AIOBJECT_VEHICLE)
				pPuppet->RegisterDetectionLevels(SAIDetectionLevels(0,0, exposure, threat));
			else
				pPuppet->RegisterDetectionLevels(SAIDetectionLevels(exposure, threat, 0,0));
		}
	}
	else if (pTarget->GetSubType() == IAIObject::STP_SOUND)
	{
		if (CAIPlayer* pPlayer = (CAIPlayer*)GetAISystem()->GetPlayer() )
		{
			if (GetType() == AIOBJECT_VEHICLE)
				pPlayer->RegisterDetectionLevels(SAIDetectionLevels(0,0, exposure, threat));
			else
				pPlayer->RegisterDetectionLevels(SAIDetectionLevels(exposure, threat, 0,0));
		}
	}
}
