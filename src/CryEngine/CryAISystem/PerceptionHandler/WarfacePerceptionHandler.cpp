/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2008.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Perception handling for puppets using Warface method
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "WarfacePerceptionHandler.h"
#include "WarfacePerceptionExtraData.h"
#include "Puppet.h"
#include "AIActor.h"

#include "GameUtils.h"
#include "IGameFramework.h"
#include "IPerceptionHandlerModifier.h"

REGISTER_PERCEPTION_HANDLER(CWarfacePerceptionHandler)

// Signals
static const char* g_szSignal_SoundHeard = "OnSoundHeard";
static const char* g_szSignal_VisualSight = "OnVisualSight";
static const char* g_szSignal_TargetStationary = "OnTargetStationary";
static const char* g_szSignal_TargetLongMemory = "OnTargetLongMemory";

//////////////////////////////////////////////////////////////////////////
CWarfacePerceptionHandler::CWarfacePerceptionHandler()
: m_pExtraData(NULL)
, m_iTempTargetPriority(eTTP_OverSounds)
, m_SoundOnlyOwnerId(0)
, m_bLastLongMemory(false)
{
	m_pExtraData = new CWarfacePerceptionExtraData;
}

//////////////////////////////////////////////////////////////////////////
CWarfacePerceptionHandler::~CWarfacePerceptionHandler()
{
	SAFE_DELETE(m_pExtraData);
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::IsDebugging() const
{
	bool bResult = false;

	CAISystem *pAISystem = GetAISystem();
	if (gAIEnv.CVars.DebugPerceptionManager != 0)
	{	
		const char* sStatsTarget = gAIEnv.CVars.StatsTarget;
		if (!m_refOwner.IsNil() || !stricmp(sStatsTarget, m_refOwner.GetAIObject()->GetName()))
		{
			bResult = true;
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::ClearPotentialTarget(SAIPotentialTarget &target)
{
	target.soundThreatLevel = 0.0f;
	target.threat = AITHREAT_NONE;
	target.type = AITARGET_NONE;

	CAIObject * const pDummyRep = target.refDummyRepresentation.GetAIObject();
	if (pDummyRep)
	{
		// Was it my target? Then we don't have a target anymore so clean up to make sure the AIMind doesn't send false signals
		CPuppet *pOwner = m_refOwner.GetAIObject();
		IAIObject *pAttTarget = pOwner ? pOwner->GetAttentionTarget() : NULL;
		const EntityId dummyId = pDummyRep->GetEntityID();
		const EntityId attTargetId = (pAttTarget ? pAttTarget->GetEntityID() : 0);
		if (attTargetId == dummyId)
		{
			SOBJECTSTATE *pState = pOwner->GetState();
			if (pState)
			{
				CWeakRef<CAIObject> refBestTarget;
				SAIPotentialTarget *pBestTargetInfo;
				bool bErased;
				bErased = !GetBestTarget(refBestTarget, pBestTargetInfo, bErased);

				pState->eTargetType = bErased ? AITARGET_NONE : pBestTargetInfo->type;
				pState->eTargetThreat = bErased ? AITHREAT_NONE : pBestTargetInfo->threat;
			}
		}

		target.refDummyRepresentation.Release();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::RemoveEvent(CWeakRef<CAIObject> refObject)
{
	CRY_ASSERT(refObject.IsValid());

	if (refObject.IsValid() && m_SoundOnlyOwnerId == refObject.GetAIObject()->GetEntityID())
	{
		ClearSoundTarget();
	}

	return CBasePerceptionHandler::RemoveEvent(refObject);
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::ClearPotentialTargets()
{
	CBasePerceptionHandler::ClearPotentialTargets();

	ClearTempTarget();
	ClearSoundTarget();
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::GetPotentialTargets(PotentialTargetMap &targetMap) const
{
	CBasePerceptionHandler::GetPotentialTargets(targetMap);

	// Add temp only one if applicable
	if (m_TempTarget.type != AITARGET_NONE)
	{
		targetMap.insert(PotentialTargetMap::iterator::value_type(m_TempTarget.refDummyRepresentation.GetWeakRef(), m_TempTarget));
	}

	// Add sound only one if applicable
	if (m_SoundOnlyTarget.type != AITARGET_NONE)
	{
		targetMap.insert(PotentialTargetMap::iterator::value_type(m_SoundOnlyTarget.refDummyRepresentation.GetWeakRef(), m_SoundOnlyTarget));
	}

	return !targetMap.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::SetTempTargetPriority(ETempTargetPriority priority)
{
	m_iTempTargetPriority = priority;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::UpdateTempTarget(const Vec3& vPosition)
{
	m_TempTarget.threat = AITHREAT_THREATENING;
	m_TempTarget.exposureThreat = AITHREAT_THREATENING;
	m_TempTarget.type = AITARGET_BEACON;

	const bool bIsZeroPos = vPosition.IsZero();
	if (bIsZeroPos)
	{
		CRY_ASSERT_MESSAGE(false, "Puppet is setting his temp target position to ZERO. Is this expected?");

		IAIRecordable::RecorderEventData recorderEventData("Temp Target set to ZERO position");
		m_refOwner.GetAIObject()->RecordEvent(IAIRecordable::E_LUACOMMENT, &recorderEventData);
	}

	if (m_TempTarget.refDummyRepresentation.IsNil())
	{
		CAIObject *pOwner = m_refOwner.GetAIObject();

		char szName[256];
		_snprintf(szName,256,"Temp_Target_Perception_%s", pOwner ? pOwner->GetName() : "NOBODY");
		GetAISystem()->CreateDummyObject(m_TempTarget.refDummyRepresentation, szName);
		m_TempTarget.refDummyRepresentation.GetAIObject()->SetSubType(IAIObject::STP_BEACON);
	}
	m_TempTarget.refDummyRepresentation.GetAIObject()->SetPos(vPosition);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::ClearTempTarget()
{
	ClearPotentialTarget(m_TempTarget);
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::ClearSoundTarget()
{
	// Clear sound only target
	m_SoundOnlyOwnerId = 0;
	ClearPotentialTarget(m_SoundOnlyTarget);
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::ResetTargetLifetime(EntityId targetOwnerId, SAIPotentialTarget &target)
{
	target.threatTime = 0.0f;

	if (target.threatTimeout <= 0.0f)
	{
		NotifyOwnerStationaryTarget(false, targetOwnerId, target);
	}

	target.threatTimeout = m_pExtraData->GetTargetStationaryTime();
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::UpdateTargetLifetime(EntityId targetOwnerId, SAIPotentialTarget &target)
{
	CRY_ASSERT(m_refOwner.IsValid());

	const float fTimePassed = m_refOwner.GetAIObject()->m_fTimePassed;
	target.threatTime += fTimePassed;

	if (target.threatTimeout > FLT_EPSILON)
	{
		target.threatTimeout -= fTimePassed;
		if (target.threatTimeout <= 0.0f)
		{
			target.threatTimeout = 0.0f;
			NotifyOwnerStationaryTarget(true, targetOwnerId, target);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::NotifyOwnerStationaryTarget(bool bStationary, EntityId targetOwnerId, const SAIPotentialTarget &target) const
{
	// Notify AI
	IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
	pData->nID = targetOwnerId;
	pData->iValue = bStationary ? 1 : 0;
	pData->fValue = target.threatTime;
	if (!target.refDummyRepresentation.IsNil())
		pData->point = target.refDummyRepresentation.GetAIObject()->GetPos();
	GetAISystem()->SendSignal(SIGNALFILTER_SENDER, AISIGNAL_DEFAULT, g_szSignal_TargetStationary, m_refOwner.GetAIObject(), pData);
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::NotifyOwnerLongMemoryTarget(bool bLongMemory, EntityId targetOwnerId, const SAIPotentialTarget *pTarget) const
{
	// Notify AI
	IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
	pData->nID = targetOwnerId;
	pData->iValue = bLongMemory ? 1 : 0;
	if (pTarget)
	{
		pData->fValue = pTarget->threatTime;
		if (!pTarget->refDummyRepresentation.IsNil())
			pData->point = pTarget->refDummyRepresentation.GetAIObject()->GetPos();
	}
	GetAISystem()->SendSignal(SIGNALFILTER_SENDER, AISIGNAL_DEFAULT, g_szSignal_TargetLongMemory, m_refOwner.GetAIObject(), pData);
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::GetTempTarget_Priority(ETempTargetPriority matchPriority, CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo)
{
	bool bResult = false;
	if (m_iTempTargetPriority == matchPriority && m_TempTarget.type != AITARGET_NONE)
	{
		refBestTarget = m_TempTarget.refDummyRepresentation.GetWeakRef();
		pTargetInfo = &m_TempTarget;
		bResult = true;
	}
	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CRY_ASSERT(m_refOwner.IsValid());
	bool bResult = false;
	refBestTarget.Reset();
	pTargetInfo = NULL;
	bCurrentTargetErased = false;

	CPuppet *pOwner = m_refOwner.GetAIObject();
	const IAIObject *pAttentionTarget = pOwner->GetAttentionTarget();
	const float fTimePassed = pOwner->m_fTimePassed;

	// Try temp target for always on top
	bResult = GetTempTarget_Priority(eTTP_Always, refBestTarget, pTargetInfo);

	// Cycle through target map for visual threats
	if (!bResult)
	{
		const int iFrameId = GetAISystem()->GetAITickCount();

		EntityId bestTargetId = 0;
		float fBestTargetPriority = 0.0f;
		PotentialTargetMap::iterator itNext = m_TargetMap.begin();
		while (itNext != m_TargetMap.end())
		{
			PotentialTargetMap::iterator itTarget = itNext++;

			SAIPotentialTarget& ed = itTarget->second;
			CAIObject* pEventOwner = itTarget->first.GetAIObject();
			CAIActor* pEventOwnerActor = pEventOwner->CastToCAIActor();
			CAIObject* pDummyRep = ed.refDummyRepresentation.GetAIObject();

			// Remove the potential target if the target is invisible or dead.
			if (pEventOwnerActor && (pEventOwnerActor->GetParameters().m_bInvisible || !pEventOwnerActor->IsActive()))
			{
				if (pEventOwner == pAttentionTarget || pDummyRep == pAttentionTarget)
					bCurrentTargetErased = true;
				m_TargetMap.erase(itTarget);
				continue;
			}

			// Priority targets are treated as if they are visual targets
			if (ed.upPriority > FLT_EPSILON)
			{
				ed.visualFrameId = iFrameId;
				ed.upPriority -= fTimePassed;
			}

			// Update vis event
			float fVisualExposure = 0.0f;
			if (ed.visualFrameId == iFrameId)
			{
				ed.visualType = SAIPotentialTarget::VIS_VISIBLE;
				ed.type = AITARGET_VISUAL;
				ed.visualPos = pEventOwner->GetPos();
				ResetTargetLifetime(pEventOwner->GetEntityID(), ed);

				pDummyRep->SetPos(ed.visualPos);

				// Use the highest threat. This ensures if we resume visual sighting, we keep the threat we were at before, and not what timeframe we are in now
				ed.exposureThreat = max(ed.exposureThreat, ed.threat);
			}
			else if (ed.visualTime >= 0.0f)
			{
				ed.visualType = SAIPotentialTarget::VIS_MEMORY;
				ed.type = AITARGET_MEMORY;
				UpdateTargetLifetime(pEventOwner->GetEntityID(), ed);

				if (m_pExtraData->CanVisualForget())
				{
					ed.visualTime = max(ed.visualTime-fTimePassed, 0.0f);
					if (ed.visualTime <= FLT_EPSILON)
					{
						if (pEventOwner == pAttentionTarget || pDummyRep == pAttentionTarget)
							bCurrentTargetErased = true;
						m_TargetMap.erase(itTarget);
						continue;
					}
				}
			}

			// Compare against current best
			const bool bUseThisTarget = (refBestTarget.IsSet() ? CompareBestTarget(pTargetInfo, &ed) : true);
			if (bUseThisTarget)
			{
				// Use as new best
				if (ed.type == AITARGET_VISUAL && ed.threat == AITHREAT_AGGRESSIVE)
					refBestTarget = GetWeakRef(pEventOwner);
				else
					refBestTarget = ed.refDummyRepresentation.GetWeakRef();
				bestTargetId = pEventOwner->GetEntityID();
				pTargetInfo = &ed;

				bResult = true;
			}
		}

		// Check long memory status
		const float fLongMemoryTimer = m_pExtraData->GetTargetLongMemoryTime();
		const bool bIsLongMemory = (pTargetInfo && fLongMemoryTimer > FLT_EPSILON ? pTargetInfo->threatTime >= m_pExtraData->GetTargetLongMemoryTime() : false);
		if (m_bLastLongMemory != bIsLongMemory)
		{
			m_bLastLongMemory = bIsLongMemory;
			NotifyOwnerLongMemoryTarget(bIsLongMemory, bestTargetId, pTargetInfo);
		}

		// Use temp target if applicable
		if (!bResult && m_TempTarget.type != AITARGET_NONE)
		{
			refBestTarget = m_TempTarget.refDummyRepresentation.GetWeakRef();
			pTargetInfo = &m_TempTarget;
			bResult = true;
		}
	}

	// Try temp target for over sounds
	if (!bResult)
		bResult = GetTempTarget_Priority(eTTP_OverSounds, refBestTarget, pTargetInfo);

	// Use sound target if applicable
	if (!bResult && m_SoundOnlyTarget.type != AITARGET_NONE)
	{
		refBestTarget = m_SoundOnlyTarget.refDummyRepresentation.GetWeakRef();
		pTargetInfo = &m_SoundOnlyTarget;
		bResult = true;
	}

	// Try temp target for last
	if (!bResult)
		bResult = GetTempTarget_Priority(eTTP_Last, refBestTarget, pTargetInfo);

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::CompareBestTarget(SAIPotentialTarget *pBestInfo, SAIPotentialTarget *pCurrInfo) const
{
	const bool bValid = (pBestInfo && pCurrInfo);
	CRY_ASSERT(bValid);
	if (!bValid)
		return false;

	// Use the one with the highest visual frame id (last saw)
	if (pBestInfo->visualFrameId != pCurrInfo->visualFrameId)
		return (pCurrInfo->visualFrameId > pBestInfo->visualFrameId);

	// Compare against threat level (must be at least interesting)
	if (pCurrInfo->threat == AITHREAT_NONE || pCurrInfo->threat < pBestInfo->threat)
		return false;

	// Compare against visual threat level
	if (pCurrInfo->visualThreatLevel < pBestInfo->visualThreatLevel)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::HandleSoundEvent(SAIEVENT* pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CRY_ASSERT(m_refOwner.IsValid());
	CRY_ASSERT(m_pExtraData);

	CPuppet *pOwner = m_refOwner.GetAIObject();
	if (!pOwner || !pOwner->m_bCanReceiveSignals)
		return;

	const AgentParameters &parameters = pOwner->GetParameters();
	bool bForceToAggressive = false;

	IPersistantDebug *pDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
	CRY_ASSERT(pDebug);
	const bool bIsDebugging = IsDebugging();

	// Disregard friendlies
	IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
	CAIObject* pEventOwner = pEventOwnerEntity ? (CAIObject*)pEventOwnerEntity->GetAI() : NULL;
	CAIActor* pTargetActor = pEventOwner ? pEventOwner->CastToCAIActor() : NULL;
	if (pTargetActor)
	{
		if (parameters.m_nSpecies == pTargetActor->GetParameters().m_nSpecies)
			return;

		// Check modifiers
		EntityId targetId = pTargetActor->GetEntityID();
		TPerceptionHandlerModifiersVector vecModifiers;
		if (pTargetActor->GetPerceptionHandlerModifiers(vecModifiers))
		{
			TPerceptionHandlerModifiersVector::iterator itModifier = vecModifiers.begin();
			TPerceptionHandlerModifiersVector::iterator itModifierEnd = vecModifiers.end();
			for (; itModifier != itModifierEnd; ++itModifier)
			{
				EStimulusHandlerResult result = (*itModifier)->OnSoundStimulus(pEvent, pOwner, targetId);
				if (eSHR_Ignore == result)
					return;

				bForceToAggressive |= (result == eSHR_MakeAggressive);
			}
		}
	}

	const Vec3 &vWorldPos(pOwner->GetEntity()->GetWorldPos());
	const float fSoundDistance = vWorldPos.GetDistance(pEvent->vPosition);
	const bool bHeardSound = (fSoundDistance <= pEvent->fThreat);

	EAITargetThreat iSoundThreat = AITHREAT_NONE;
	if (bHeardSound)
	{
		HandleHeardSoundEvent(pEvent, fSoundDistance, iSoundThreat, bForceToAggressive);
	}
	
	// Debugging
	if (bIsDebugging)
	{
		if (bHeardSound)
		{
			const float fAggressiveRange = m_pExtraData->GetAggressiveAudioRange();
			const float fThreateningRange = m_pExtraData->GetThreateningAudioRange();
			const float fInterestingRange = m_pExtraData->GetInterestingAudioRange();

			// Owner's audio ranges
			const Vec3 vDrawRangePos(vWorldPos.x,vWorldPos.y,vWorldPos.z+0.1f);
			pDebug->Begin("WarfacePerceptionHandler_HandleSoundEvent_Ranges", true);
			pDebug->AddPlanarDisc(vDrawRangePos, fThreateningRange, fInterestingRange, ColorF(0.0f,1.0f,0.0f,1.0f), 2.0f);
			pDebug->AddPlanarDisc(vDrawRangePos, fAggressiveRange, fThreateningRange, ColorF(1.0f,1.0f,0.0f,1.0f), 2.0f);
			pDebug->AddPlanarDisc(vDrawRangePos, 0.0f, fAggressiveRange, ColorF(1.0f,0.0f,0.0f,1.0f), 2.0f);
		}

		pDebug->Begin("WarfacePerceptionHandler_HandleSoundEvent", false);

		// Sound info
		pDebug->AddSphere(pEvent->vPosition, 0.5f, ColorF(0.0f,0.0f,1.0f,1.0f), 2.0f);
		pDebug->AddPlanarDisc(pEvent->vPosition, 0.0f, pEvent->fThreat, ColorF(1.0f,1.0f,1.0f,0.5f), 2.0f);

		if (bHeardSound)
		{
			string sTagStr;
			static char const* szSoundTypes[AISOUND_LAST] =
			{
				"Generic","Collision","Collision Loud","Movement","Movement Loud","Weapon","Explosion"
			};
			CRY_ASSERT(pEvent->nType >= 0 && pEvent->nType < AISOUND_LAST);
			IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
			sTagStr.Format("Heard Sound: Owner=%s, Range=%d, Type=%s", pEventOwnerEntity ? pEventOwnerEntity->GetName() : "None", iSoundThreat, szSoundTypes[pEvent->nType]);
			SEntityTagParams tagParams(pOwner->GetEntityID(), sTagStr.c_str());
			tagParams.visibleTime = 2.0f;
			pDebug->AddEntityTag(tagParams, "OnSoundHeard");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::HandleHeardSoundEvent(SAIEVENT *pEvent, float fSoundDistance, EAITargetThreat &iSoundThreat, bool bForceToAggressive)
{
	const float fAggressiveRange = m_pExtraData->GetAggressiveAudioRange();
	const float fThreateningRange = m_pExtraData->GetThreateningAudioRange();
	const float fInterestingRange = m_pExtraData->GetInterestingAudioRange();

	CPuppet *pOwner = m_refOwner.GetAIObject();
	IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
	CAIObject* pEventOwner = pEventOwnerEntity ? (CAIObject*)pEventOwnerEntity->GetAI() : NULL;
	EntityId eventOwnerId = pEventOwner ? pEventOwner->GetEntityID() : 0;
	Vec3 vEventPos = pEvent->vPosition;
	const Vec3 &vWorldPos(pOwner->GetEntity()->GetWorldPos());

	if (fSoundDistance <= fAggressiveRange || bForceToAggressive)
		iSoundThreat = AITHREAT_AGGRESSIVE;
	else if (fSoundDistance <= fThreateningRange)
		iSoundThreat = AITHREAT_THREATENING;
	else if (fSoundDistance <= fInterestingRange)
		iSoundThreat = AITHREAT_INTERESTING;
	else
		iSoundThreat = AITHREAT_NONE;

	// Base it on minimum threat
	EAITargetThreat iMinThreat = m_pExtraData->GetMinAudioThreat((EAISoundStimType)pEvent->nType);
	if (iSoundThreat > AITHREAT_NONE && iSoundThreat < iMinThreat)
		iSoundThreat = iMinThreat;

	// Notify AI
	IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
	pData->nID = pEvent->sourceId;
	pData->iValue = iSoundThreat;
	pData->iValue2 = pEvent->nType;
	pData->point = pEvent->vPosition;
	GetAISystem()->SendSignal(SIGNALFILTER_SENDER, AISIGNAL_DEFAULT, g_szSignal_SoundHeard, pOwner, pData);

	// Get descriptor
	SSoundPerceptionDescriptor sDescriptor;
	if (!pOwner->GetSoundPerceptionDescriptor((EAISoundStimType)pEvent->nType, sDescriptor))
		CRY_ASSERT_MESSAGE(0, "Missing Sound Perception Descriptor when handling a sound event");

	// Look to see if a visual/memory target made this sound
	CWeakRef<CAIObject> refEventOwner = GetWeakRef(pEventOwner);
	PotentialTargetMap::iterator ei = m_TargetMap.find(refEventOwner);
	if (ei != m_TargetMap.end())
	{
		SAIPotentialTarget *pVisTarget = &(ei->second);
		CRY_ASSERT(pVisTarget);

		// Elevate the visual threat if the sound threat is higher
		float fRatioToNextThreat = 0.0f;
		GetVisualThreatLevel(pVisTarget, refEventOwner, iSoundThreat, fRatioToNextThreat);

		// Update dummy representation's position if this wasn't an explosion sound
		if (pEvent->nType != AISOUND_EXPLOSION)
		{
			pVisTarget->refDummyRepresentation.GetAIObject()->SetPos(vEventPos);
			ResetTargetLifetime(eventOwnerId, *pVisTarget);
		}
	}
	else
	{
		bool bUseNewSoundThreat = (iSoundThreat > m_SoundOnlyTarget.threat);
		if (!bUseNewSoundThreat && iSoundThreat == m_SoundOnlyTarget.threat)
		{
			// Judge by proximity from me
			const float fCurrentDist = vWorldPos.GetSquaredDistance(m_SoundOnlyTarget.soundPos);
			const float fNewDist = vWorldPos.GetSquaredDistance(vEventPos);
			if (fNewDist < fCurrentDist)
			{
				bUseNewSoundThreat = true;
			}
			else if (!m_SoundOnlyOwnerId || eventOwnerId == m_SoundOnlyOwnerId)
			{
				// Always if its coming from the same person
				bUseNewSoundThreat = true;
			}
		}

		m_SoundOnlyTarget.bNeedsUpdating = bUseNewSoundThreat;
		if (bUseNewSoundThreat)
		{
			UpdateSoundOnlyTarget(refEventOwner, vEventPos, iSoundThreat);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::UpdateSoundOnlyTarget(CWeakRef<CAIObject> refEventOwner, const Vec3& vPos, EAITargetThreat iSoundThreat)
{
	m_SoundOnlyTarget.soundPos = vPos;
	m_SoundOnlyTarget.soundThreatLevel = 1.0f * iSoundThreat;
	m_SoundOnlyTarget.threat = iSoundThreat;
	m_SoundOnlyTarget.exposureThreat = iSoundThreat;
	m_SoundOnlyTarget.type = AITARGET_SOUND;

	CAIObject *pEventOwner = refEventOwner.GetAIObject();
	m_SoundOnlyOwnerId = pEventOwner ? pEventOwner->GetEntityID() : 0;

	if (m_SoundOnlyTarget.refDummyRepresentation.IsNil())
	{
		CAIObject *pOwner = m_refOwner.GetAIObject();

		char szName[256];
		_snprintf(szName,256,"Sound_Perception_%s", pOwner ? pOwner->GetName() : "NOBODY");
		GetAISystem()->CreateDummyObject(m_SoundOnlyTarget.refDummyRepresentation, szName);
		m_SoundOnlyTarget.refDummyRepresentation.GetAIObject()->SetAssociation(refEventOwner);
	}
	m_SoundOnlyTarget.refDummyRepresentation.GetAIObject()->SetPos(vPos);
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::UpTargetPriority(CWeakRef<CAIObject> refTarget, float fPriorityIncrement)
{
	PotentialTargetMap::iterator itTarget = m_TargetMap.find(refTarget);
	if (itTarget == m_TargetMap.end())
		return;

	SAIPotentialTarget& ed = itTarget->second;
	ed.upPriority = max(ed.upPriority, fPriorityIncrement);
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::AddAggressiveTarget(CWeakRef<CAIObject> refTarget)
{
	float fSightRange = 0.0f;
	float fSightRangeMax = 0.0f;
	float fDistToTarget = 0.0f;
	GetSightRange(refTarget, fSightRange, fSightRangeMax, fDistToTarget);

	HandleSeenVisualStimulus(refTarget, fSightRange, fDistToTarget, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionHandler::DropTarget(CWeakRef<CAIObject> refTarget)
{
	bool bResult = false;

	PotentialTargetMap::iterator itTarget = m_TargetMap.find(refTarget);
	if (itTarget != m_TargetMap.end())
	{
		SAIPotentialTarget& ed = itTarget->second;

		ClearPotentialTarget(ed);
		m_TargetMap.erase(itTarget);

		bResult = true;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::HandleVisualStimulus(SAIEVENT* pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CRY_ASSERT(m_refOwner.IsValid());

	CPuppet *pOwner = m_refOwner.GetAIObject();
	if (!pOwner || !pOwner->m_bCanReceiveSignals)
		return;

	const AgentParameters &parameters = pOwner->GetParameters();
	bool bForceToAggressive = false;

	IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
	CAIObject* pEventOwner = pEventOwnerEntity ? (CAIObject*)pEventOwnerEntity->GetAI() : NULL;
	CAIObject* pEventAssociation = pEventOwner;
	if (pEventOwner && pEventOwner->GetType() == AIOBJECT_ATTRIBUTE)
		pEventAssociation = (CAIObject*)pEventOwner->GetAssociation().GetAIObject();
	if (!pEventAssociation)
		return;

	// Disregard friendlies
	CAIActor* pTargetActor = pEventOwner->CastToCAIActor();
	if (pTargetActor)
	{
		if (parameters.m_nSpecies == pTargetActor->GetParameters().m_nSpecies)
			return;

		// Check modifiers
		EntityId targetId = pTargetActor->GetEntityID();
		TPerceptionHandlerModifiersVector vecModifiers;
		if (pTargetActor->GetPerceptionHandlerModifiers(vecModifiers))
		{
			TPerceptionHandlerModifiersVector::iterator itModifier = vecModifiers.begin();
			TPerceptionHandlerModifiersVector::iterator itModifierEnd = vecModifiers.end();
			for (; itModifier != itModifierEnd; ++itModifier)
			{
				EStimulusHandlerResult result = (*itModifier)->OnVisualStimulus(pEvent, pOwner, targetId);
				if (eSHR_Ignore == result)
					return;

				bForceToAggressive |= (result == eSHR_MakeAggressive);
			}
		}
	}

	CWeakRef<CAIObject> refEventOwner = GetWeakRef(pEventOwner);

	float fSightRange = 0.0f;
	float fSightRangeMax = 0.0f;
	float fDistToTarget = 0.0f;
	GetSightRange(refEventOwner, fSightRange, fSightRangeMax, fDistToTarget);

	// if target is outside new, modified sight range, dont't do anything about it
	if (fSightRangeMax > 0.0001f && fSightRangeMax > fDistToTarget)
	{
		HandleSeenVisualStimulus(refEventOwner, fSightRange, fDistToTarget, bForceToAggressive);
	}
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::GetSightRange(CWeakRef<CAIObject> refEventOwner, float &fSightRange, float &fSightRangeMax, float &fDistToTarget) const
{
	CRY_ASSERT(!refEventOwner.IsNil());
	CAIObject *pEventOwner = refEventOwner.GetAIObject();

	CPuppet *pOwner = m_refOwner.GetAIObject();
	CRY_ASSERT(pOwner);

	fSightRange = pOwner->GetMaxTargetVisibleRange(pEventOwner);
	fSightRangeMax = fSightRange;

	CAIActor* pTargetActor = pEventOwner->CastToCAIActor();
	const AgentParameters &parameters = pOwner->GetParameters();

	const float fRadius = pOwner->GetRadius();
	const Vec3 &vWorldPos(pOwner->GetEntity()->GetWorldPos());
	fDistToTarget = Distance::Point_Point(vWorldPos, pEventOwner->GetPos());

	float fSightScale = 1.0f;
	float fSightRangeScale = 1.0f;
	float fPerceptionModifier = 1.0f;
	if (pTargetActor)
	{
		const Vec3 &vWorldPos(pOwner->GetEntity()->GetWorldPos());
		fPerceptionModifier = GetAISystem()->GetRayPerceptionModifier(vWorldPos, pEventOwner->GetPos(), pOwner->GetName());

		// Target under water
		float fCachedWaterOcclusionValue = pTargetActor->GetCachedWaterOcclusionValue();
		fSightRangeScale *= 0.1f + (1 - fCachedWaterOcclusionValue) * 0.9f;

		// Flowgraph Target scale factor
		fSightRangeScale *= parameters.m_PerceptionParams.perceptionScale.visual;

		// When the target is really close, reduce the effect of the FOV.
		float fFOVPrimaryCos = 0.0f;
		float fFOVSecondaryCos = 0.0f;
		pOwner->GetSightFOV(fFOVPrimaryCos, fFOVSecondaryCos);
		if (fFOVPrimaryCos > 0.0f && fDistToTarget < fRadius * 2.0f)
		{
			fSightScale *= max(1.0f, LinStep(fRadius * 2.0f, fRadius, fDistToTarget));
		}

		// Stance overrides
		if (gAIEnv.CVars.EnablePerceptionStanceVisibleRange != 0)
		{
			SAIBodyInfo bodyInfo;
			pTargetActor->GetProxy()->QueryBodyInfo(bodyInfo);
			if (bodyInfo.stance == STANCE_CROUCH)
			{
				fSightRangeMax = gAIEnv.CVars.CrouchVisibleRange;
			}
			else if (bodyInfo.stance == STANCE_PRONE)
			{
				fSightRangeMax = gAIEnv.CVars.ProneVisibleRange;
			}
		}
	}
	fSightRange *= fSightRangeScale * fPerceptionModifier;
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::HandleSeenVisualStimulus(CWeakRef<CAIObject> refEventOwner, float fSightRange, float fDistToTarget, bool bForceToAggressive /*= false*/)
{
	CRY_ASSERT(!refEventOwner.IsNil());
	CAIObject *pEventOwner = refEventOwner.GetAIObject();

	CPuppet *pOwner = m_refOwner.GetAIObject();
	if (pEventOwner && pEventOwner->GetType() == AIOBJECT_ATTRIBUTE)
		refEventOwner = pEventOwner->GetAssociation();
	if (!refEventOwner.IsValid())
		return;
	CAIObject *pEventAssociation = refEventOwner.GetAIObject();

	// Reuse existing event or create a new one of the current target has not been seen yet.
	SAIPotentialTarget *pEvent = 0;
	PotentialTargetMap::iterator ei = m_TargetMap.find(refEventOwner);
	if (ei == m_TargetMap.end())
	{
		SAIPotentialTarget ed;
		static int nameCounter = 0;
		char szName[256];
		_snprintf(szName,256,"Memory Perception of %s %d", pEventAssociation != NULL ? pEventAssociation->GetName() : "NOBODY", nameCounter);
		GetAISystem()->CreateDummyObject(ed.refDummyRepresentation, szName);
		CAIObject *pDummyRep = ed.refDummyRepresentation.GetAIObject();
		pDummyRep->SetSubType(IAIObject::STP_MEMORY);
		pDummyRep->SetAssociation(refEventOwner);
		pEvent = AddEvent(refEventOwner, ed);
	}
	else
	{
		pEvent = &(ei->second);
	}

	if (pEvent)
	{
		float fRatioToNextThreat = 1.0f;
		GetVisualThreatLevel(pEvent, refEventOwner, (bForceToAggressive ? AITHREAT_AGGRESSIVE : AITHREAT_NONE), fRatioToNextThreat, fDistToTarget);

		if (pEventOwner->GetType() == AIOBJECT_ATTRIBUTE)
			pEvent->visualPos = pEventAssociation->GetPos();
		else
			pEvent->visualPos = pEventOwner->GetPos();
		pEvent->visualFrameId = GetAISystem()->GetAITickCount();
		pEvent->visualThreatLevel = CalculateVisualThreat(fSightRange, fDistToTarget);
		pEvent->indirectSight = pEventOwner->GetType() == AIOBJECT_ATTRIBUTE;

		// Notify AI
		IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
		pData->nID = pEventAssociation ? pEventAssociation->GetEntityID() : 0;
		pData->iValue = pEvent->threat;
		pData->point = pEvent->visualPos;
		GetAISystem()->SendSignal(SIGNALFILTER_SENDER, AISIGNAL_DEFAULT, g_szSignal_VisualSight, pOwner, pData);
		
		// Debugging
		if (IsDebugging())
		{
			IPersistantDebug *pDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
			CRY_ASSERT(pDebug);

			static const float fMaxRadius = 2.0f;
			static ColorF pColorMap[AITHREAT_LAST] =
			{
				ColorF(0.3f,0.3f,0.3f,1.0f),
				ColorF(0.0f,1.0f,0.0f,1.0f),
				ColorF(1.0f,1.0f,0.0f,1.0f),
				ColorF(1.0f,0.0f,0.0f,1.0f),
			};

			const Vec3 &vWorldPos(pOwner->GetEntity()->GetWorldPos());
			const float fProgressRadius = fMaxRadius - (fMaxRadius * fRatioToNextThreat);
			const int iNextThreat = min(int(pEvent->threat)+1, (int)AITHREAT_AGGRESSIVE);
			pDebug->Begin("WarfacePerceptionHandler_HandleVisualStimulus_Progress", true);
			pDebug->AddPlanarDisc(vWorldPos, 0.0f, fProgressRadius, pColorMap[iNextThreat], 2.0f);
			pDebug->AddPlanarDisc(vWorldPos, fProgressRadius, fMaxRadius, pColorMap[pEvent->threat], 2.0f);

			pDebug->Begin("WarfacePerceptionHandler_HandleVisualStimulus", false);
			pDebug->AddLine(vWorldPos, pEvent->visualPos, pColorMap[pEvent->threat], 2.0f);
			pDebug->AddSphere(pEvent->visualPos, 0.5f, pColorMap[pEvent->threat], 2.0f);
			if (pEvent->threat < AITHREAT_AGGRESSIVE)
			{
				string sTagStr;
				sTagStr.Format("Visual Target: Owner=%s, Threat=%d, RTN=%.3f", pEventOwner ? pEventOwner->GetName() : "None", pEvent->threat, fRatioToNextThreat);
				SEntityTagParams tagParams(pOwner->GetEntityID(), sTagStr.c_str());
				tagParams.visibleTime = 2.0f;
				pDebug->AddEntityTag(tagParams, "OnVisualTarget");
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::GetVisualThreatLevel(SAIPotentialTarget *pEvent, CWeakRef<CAIObject> refEventOwner, EAITargetThreat iBaseLevel, 
													 float &fRatioToNextThreat, float fDistToTarget)
{
	const float fNearReactTime		= m_pExtraData->GetNearReactVisualTime();
	const float fInterestingTime	= m_pExtraData->GetInterestingVisualTime();
	const float fThreateningTime	= fInterestingTime + m_pExtraData->GetThreateningVisualTime();
	const float fAggressiveTime		= fThreateningTime + m_pExtraData->GetAggressiveVisualTime();

	// Update threat level if not yet at aggressive
	if (pEvent->threat < AITHREAT_AGGRESSIVE)
	{
		const float fThisTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		if (pEvent->visualMaxTime > FLT_EPSILON)
		{
			pEvent->visualTime += (fThisTime - pEvent->visualMaxTime);
		}
		pEvent->visualMaxTime = fThisTime;

		CAIObject *pEventOwner = refEventOwner.GetAIObject();

		// Get associated sound threat if the sound owner is this target
		EAITargetThreat iSoundThreatLevel = AITHREAT_NONE;
		if (pEventOwner && m_SoundOnlyOwnerId == pEventOwner->GetEntityID())
		{
			iSoundThreatLevel = m_SoundOnlyTarget.threat;
		}
		iBaseLevel = max(iBaseLevel, iSoundThreatLevel);

		EAITargetThreat iThreatLevel = pEvent->threat;

		// Check if near react timer is valid
		bool bReactNear = false;
		if (m_refOwner.IsValid())
		{
			const AgentParameters &parameters = m_refOwner.GetAIObject()->GetParameters();
			if (fDistToTarget <= parameters.m_PerceptionParams.sightNearRange)
			{
				bReactNear = true;
			}
		}

		// Determine threat level based on how long I've made visual contact
		if ((bReactNear && pEvent->visualTime >= fNearReactTime) || pEvent->visualTime >= fAggressiveTime || iBaseLevel == AITHREAT_AGGRESSIVE)
		{
			pEvent->visualTime = max(pEvent->visualTime, fAggressiveTime);
			iThreatLevel = AITHREAT_AGGRESSIVE;
		}
		else if (pEvent->visualTime >= fThreateningTime || iBaseLevel == AITHREAT_THREATENING)
		{
			pEvent->visualTime = max(pEvent->visualTime, fThreateningTime);
			iThreatLevel = AITHREAT_THREATENING;
			fRatioToNextThreat = (fAggressiveTime > FLT_EPSILON ? (fAggressiveTime - pEvent->visualTime) / fAggressiveTime : 1.0f);
		}
		else if (pEvent->visualTime >= fInterestingTime || iBaseLevel == AITHREAT_INTERESTING)
		{
			pEvent->visualTime = max(pEvent->visualTime, fInterestingTime);
			iThreatLevel = AITHREAT_INTERESTING;
			fRatioToNextThreat = (fThreateningTime > FLT_EPSILON ? (fThreateningTime - pEvent->visualTime) / fThreateningTime : 1.0f);
		}
		else
		{
			iThreatLevel = AITHREAT_NONE;
			fRatioToNextThreat = (fInterestingTime > FLT_EPSILON ? (fInterestingTime - pEvent->visualTime) / fInterestingTime : 1.0f);
		}

		if (pEvent->threat != iThreatLevel)
		{
			pEvent->bNeedsUpdating = true;
			pEvent->threat = iThreatLevel;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
float CWarfacePerceptionHandler::CalculateVisualThreat(float fSightRange, float fDistToTarget) const
{
	// TODO Aggro concept should be considered here?

	const float fInvDistRatio = (1.0f - (fDistToTarget / fSightRange));

	return (fInvDistRatio);
}

//////////////////////////////////////////////////////////////////////////
void CWarfacePerceptionHandler::HandleBulletRain(SAIEVENT* pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	CRY_ASSERT(m_refOwner.IsValid());
	CPuppet *pOwner = m_refOwner.GetAIObject();
	
	const AgentParameters &parameters = pOwner->GetParameters();

	if (gAIEnv.CVars.IgnoreSoundStimulus != 0 || parameters.m_bAiIgnoreFgNode)
		return;

	IPersistantDebug *pDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
	CRY_ASSERT(pDebug);
	const bool bIsDebugging = IsDebugging();

	// Disregard friendlies
	IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
	CAIObject* pEventOwner = pEventOwnerEntity ? (CAIObject*)pEventOwnerEntity->GetAI() : NULL;
	CAIActor* pTargetActor = pEventOwner ? pEventOwner->CastToCAIActor() : NULL;
	if (pTargetActor)
	{
		if (parameters.m_nSpecies == pTargetActor->GetParameters().m_nSpecies)
			return;

		// Check modifiers
		/*EntityId targetId = pTargetActor->GetEntityID();
		TPerceptionHandlerModifiersVector vecModifiers;
		if (pTargetActor->GetPerceptionHandlerModifiers(vecModifiers))
		{
			TPerceptionHandlerModifiersVector::iterator itModifier = vecModifiers.begin();
			TPerceptionHandlerModifiersVector::iterator itModifierEnd = vecModifiers.end();
			for (; itModifier != itModifierEnd; ++itModifier)
			{
				if (!(*itModifier)->OnSoundStimulus(pEvent, pOwner, targetId))
					return;
			}
		}*/
	}
	
	// Update target position
	CWeakRef<CAIObject> refEventOwner = GetWeakRef(pEventOwner);
	PotentialTargetMap::iterator ei = m_TargetMap.find(refEventOwner);
	if (ei != m_TargetMap.end())
	{
		SAIPotentialTarget *pVisTarget = &(ei->second);
		CRY_ASSERT(pVisTarget);

		CAIObject *pDummyRepresentation = pVisTarget->refDummyRepresentation.GetAIObject();
		pDummyRepresentation->SetPos(pEventOwner->GetPos());
	}
	
	// Debugging
	if (bIsDebugging)
	{
		pDebug->Begin("WarfacePerceptionHandler_HandleBulletRain", false);

		string sTagStr;
		IEntity* pEventOwnerEntity = gEnv->pEntitySystem->GetEntity(pEvent->sourceId);
		sTagStr.Format("Bullet Rain: Owner=%s ", pEventOwnerEntity ? pEventOwnerEntity->GetName() : "None");
		SEntityTagParams tagParams(pOwner->GetEntityID(), sTagStr.c_str());
		tagParams.visibleTime = 2.0f;
		pDebug->AddEntityTag(tagParams, "OnBulletRain");
	}
}
