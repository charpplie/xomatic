/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Maintains an interest level of an agent's perception on a
				target, used for determining the agent's target
  
 -------------------------------------------------------------------------
  History:
  - 02:01:2010: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "TargetTrack.h"
#include "TargetTrackModifiers.h"

#ifdef TARGET_TRACK_DEBUG
	#include "DebugDrawContext.h"
#endif //TARGET_TRACK_DEBUG

//////////////////////////////////////////////////////////////////////////
bool CTargetTrack::SStimulusInvocation::IsRunning() const
{
	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	bool bResult = m_bMustRun;
	if (!bResult)
	{
		const float fCurrTime = pAISystem->GetFrameStartTime().GetSeconds();
		const float fUpdateRate = pAISystem->GetUpdateInterval();
		bResult = (fCurrTime - m_envelopeData.m_fLastInvokeTime <= fUpdateRate*2.0f);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::SStimulusInvocation::SPulseTrigger::Serialize(TSerialize ser)
{
	ser.Value("uPulseNameHash", uPulseNameHash);
	ser.Value("fTriggerTime", fTriggerTime);
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::SStimulusInvocation::Serialize(TSerialize ser)
{
	ser.Value("m_vLastPos", m_vLastPos);
	ser.Value("m_fCurrentValue", m_envelopeData.m_fCurrentValue);
	ser.Value("m_fStartTime", m_envelopeData.m_fStartTime);
	ser.Value("m_fLastInvokeTime", m_envelopeData.m_fLastInvokeTime);
	ser.Value("m_fLastRunningValue", m_envelopeData.m_fLastRunningValue);
	ser.Value("m_fLastReleasingValue", m_envelopeData.m_fLastReleasingValue);

	ser.Value("m_pulseTriggers", m_pulseTriggers);
}

//////////////////////////////////////////////////////////////////////////
CTargetTrack::CTargetTrack()
: m_vTargetPos(ZERO)
, m_eTargetType(AITARGET_NONE)
, m_eTargetThreat(AITHREAT_NONE)
, m_aiGroupOwnerId(0)
, m_aiObjectId(0)
, m_uConfigHash(0)
, m_iLastUpdateFrame(0)
, m_fTrackValue(0.0f)
, m_fObsoleteTimer(0.0f)
, m_fFirstAggressiveTime(0.0f)
{
#ifdef TARGET_TRACK_DEBUG
	m_fLastDebugDrawTime = 0.0f;
	m_uDebugGraphIndex = 0;
#endif //TARGET_TRACK_DEBUG
}

//////////////////////////////////////////////////////////////////////////
CTargetTrack::~CTargetTrack()
{

}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::Init(tAIObjectID aiGroupOwnerId, tAIObjectID aiObjectId, uint32 uConfigHash)
{
	assert(aiGroupOwnerId > 0);
	assert(aiObjectId > 0);
	assert(uConfigHash > 0);

	m_aiGroupOwnerId = aiGroupOwnerId;
	m_aiObjectId = aiObjectId;
	m_uConfigHash = uConfigHash;
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::ResetForPool()
{
	m_vTargetPos.zero();
	m_eTargetType = AITARGET_NONE;
	m_eTargetThreat = AITHREAT_NONE;

	m_aiGroupOwnerId = 0;
	m_aiObjectId = 0;
	m_uConfigHash = 0;

	m_iLastUpdateFrame = 0;
	m_fTrackValue = 0.0f;
	m_fObsoleteTimer = 0.0f;
	m_fFirstAggressiveTime = 0.0f;
	m_StimuliInvocations.clear();
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::Serialize(TSerialize ser)
{
	ser.Value("m_fTrackValue", m_fTrackValue);
	ser.Value("m_fObsoleteTimer", m_fObsoleteTimer);
	ser.Value("m_fFirstAggressiveTime", m_fFirstAggressiveTime);
	ser.Value("m_StimuliInvocations", m_StimuliInvocations);
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::OnAgentRemoved()
{
	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	m_fObsoleteTimer = (m_fObsoleteTimer < FLT_EPSILON ? pAISystem->GetFrameStartTime().GetSeconds() : m_fObsoleteTimer);
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrack::Update(float fCurrTime, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	bool bResult = true;

	const int iFrameId = pAISystem->GetAITickCount();
	if (iFrameId != m_iLastUpdateFrame)
	{
		bResult = false;
		m_iLastUpdateFrame = iFrameId;

		float fLastInvokeTime = 0.0f;
		m_fTrackValue = 0.0f;
		EAITargetType eNewTargetType = AITARGET_NONE;
		EAITargetThreat eNewTargetThreat = AITHREAT_NONE;

		TStimuliInvocationContainer::iterator itStimulusInvoke = m_StimuliInvocations.begin();
		TStimuliInvocationContainer::iterator itStimulusInvokeEnd = m_StimuliInvocations.end();
		for (; itStimulusInvoke != itStimulusInvokeEnd; ++itStimulusInvoke)
		{
			const uint32 uStimulusHash = itStimulusInvoke->first;
			SStimulusInvocation& invoke = itStimulusInvoke->second;

			const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig = NULL;
			if (pConfigProxy->GetTargetTrackStimulusConfig(m_uConfigHash, uStimulusHash, pStimulusConfig))
			{
				const float fStimulusValue = UpdateStimulusValue(fCurrTime, invoke, pStimulusConfig, pConfigProxy);

				// Update position to most recent invocation's info
				if (invoke.m_envelopeData.m_fLastInvokeTime > fLastInvokeTime)
				{
					fLastInvokeTime = invoke.m_envelopeData.m_fLastInvokeTime;

					m_vTargetPos = invoke.m_vLastPos;
				}

				// Track value and threat uses the highest
				if (fStimulusValue > m_fTrackValue)
				{
					m_fTrackValue = fStimulusValue;

#ifdef TARGET_TRACK_DOTARGETTHREAT
					UpdateTargetThreat(eNewTargetThreat, fCurrTime, invoke);
#endif //TARGET_TRACK_DOTARGETTHREAT
				}

#ifdef TARGET_TRACK_DOTARGETTYPE
				bResult = UpdateTargetType(eNewTargetType, eNewTargetThreat, invoke);
#endif //TARGET_TRACK_DOTARGETTYPE
			}

			SStimulusInvocation::TPulseTriggersContainer::iterator itNewEnd = std::remove_if(invoke.m_pulseTriggers.begin(), invoke.m_pulseTriggers.end(), SStimulusInvocation::SPulseTrigger::IsObsolete);
			invoke.m_pulseTriggers.erase(itNewEnd, invoke.m_pulseTriggers.end());
		}

		m_eTargetThreat = eNewTargetThreat;
		m_eTargetType = eNewTargetType;
	}

	if (!bResult || m_eTargetThreat < AITHREAT_AGGRESSIVE)
	{
		m_fFirstAggressiveTime = 0.0f;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
#ifdef TARGET_TRACK_DOTARGETTHREAT
void CTargetTrack::UpdateTargetThreat(EAITargetThreat &outTargetThreat, float fCurrTime, const SStimulusInvocation &invoke)
{
	outTargetThreat = max(outTargetThreat, invoke.m_eTargetThreat);

	if (m_fFirstAggressiveTime <= 0.0f && m_eTargetThreat < AITHREAT_AGGRESSIVE && outTargetThreat >= AITHREAT_AGGRESSIVE)
	{
		m_fFirstAggressiveTime = fCurrTime;
	}
}
#endif //TARGET_TRACK_DOTARGETTHREAT

//////////////////////////////////////////////////////////////////////////
#ifdef TARGET_TRACK_DOTARGETTYPE
bool CTargetTrack::UpdateTargetType(EAITargetType &outTargetType, EAITargetThreat eTargetThreat, const SStimulusInvocation &invoke)
{
	bool bResult = false;

	// Update the target type
	if (m_fTrackValue > 0.0f)
	{
		switch (invoke.m_eStimulusType)
		{
			case TargetTrackHelpers::eEST_Visual:
			{
				if (outTargetType < AITARGET_VISUAL)
				{
					outTargetType = (invoke.IsRunning() && invoke.m_eTargetThreat == AITHREAT_AGGRESSIVE ? AITARGET_VISUAL : AITARGET_MEMORY);
				}
			}
			break;

			case TargetTrackHelpers::eEST_Sound:
			case TargetTrackHelpers::eEST_BulletRain:
			case TargetTrackHelpers::eEST_Generic:
			{
				if (outTargetType < AITARGET_SOUND)
					outTargetType = AITARGET_SOUND;
			}
			break;
		}

		bResult = (outTargetType != AITARGET_NONE);
	}

	return bResult;
}
#endif //TARGET_TRACK_DOTARGETTYPE

//////////////////////////////////////////////////////////////////////////
bool CTargetTrack::InvokeStimulus(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bResult = false;

	if (uStimulusNameHash > 0)
	{
		TStimuliInvocationContainer::iterator itInvoke = m_StimuliInvocations.find(uStimulusNameHash);
		if (itInvoke != m_StimuliInvocations.end())
		{
			SStimulusInvocation &invoke = itInvoke->second;
			UpdateStimulusInvoke(invoke, stimulusEvent);
		}
		else
		{
			SStimulusInvocation invoke;
			UpdateStimulusInvoke(invoke, stimulusEvent);
			m_StimuliInvocations[uStimulusNameHash] = invoke;
		}

		bResult = true;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrack::TriggerPulse(uint32 uStimulusNameHash, uint32 uPulseNameHash)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bResult = false;

	if (uStimulusNameHash > 0 && uPulseNameHash > 0)
	{
		TStimuliInvocationContainer::iterator itInvoke = m_StimuliInvocations.find(uStimulusNameHash);
		if (itInvoke != m_StimuliInvocations.end())
		{
			SStimulusInvocation &invoke = itInvoke->second;
			UpdateStimulusPulse(invoke, uPulseNameHash);
		}
		else
		{
			SStimulusInvocation invoke;
			UpdateStimulusPulse(invoke, uPulseNameHash);
			m_StimuliInvocations[uStimulusNameHash] = invoke;
		}

		bResult = true;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::UpdateStimulusInvoke(SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent) const
{
	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	const float fCurrTime = pAISystem->GetFrameStartTime().GetSeconds();

	if (!invoke.IsRunning())
	{
		invoke.m_envelopeData.m_fStartTime = fCurrTime;
	}

	if (!stimulusEvent.m_vTargetPos.IsZero())
	{
		invoke.m_vLastPos = stimulusEvent.m_vTargetPos;
	}
	else
	{
		CWeakRef<CAIObject> refOwner = gAIEnv.pObjectContainer->GetWeakRef(stimulusEvent.m_ownerId);
		CAIObject *pOwner = refOwner.GetAIObject();
		if (pOwner)
		{
			invoke.m_vLastPos = pOwner->GetPos();
		}
		else
		{
			CRY_ASSERT_MESSAGE(0, "No position could be set from invoked stimulus event!");
		}
	}


	invoke.m_envelopeData.m_fLastInvokeTime = fCurrTime;
	invoke.m_eTargetThreat = stimulusEvent.m_eTargetThreat;
	invoke.m_eStimulusType = stimulusEvent.m_eStimulusType;
	invoke.m_bMustRun = true;
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::UpdateStimulusPulse(SStimulusInvocation &invoke, uint32 uPulseNameHash) const
{
	assert(uPulseNameHash > 0);

	SStimulusInvocation::TPulseTriggersContainer::iterator itPulse = invoke.m_pulseTriggers.begin();
	SStimulusInvocation::TPulseTriggersContainer::iterator itPulseEnd = invoke.m_pulseTriggers.end();
	for (; itPulse != itPulseEnd; ++itPulse)
	{
		SStimulusInvocation::SPulseTrigger &pulseTrigger = *itPulse;
		if (pulseTrigger.uPulseNameHash == uPulseNameHash)
		{
			UpdatePulseValue(pulseTrigger);
			return;
		}
	}

	// Add new entry
	SStimulusInvocation::SPulseTrigger pulseTrigger(uPulseNameHash);
	UpdatePulseValue(pulseTrigger);
	invoke.m_pulseTriggers.push_back(pulseTrigger);
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrack::UpdatePulseValue(SStimulusInvocation::SPulseTrigger &pulseTrigger) const
{
	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	const float fCurrTime = pAISystem->GetFrameStartTime().GetSeconds();

	pulseTrigger.fTriggerTime = fCurrTime;
	pulseTrigger.bObsolete = false;
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrack::UpdateStimulusValue(float fCurrTime, SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig,
										TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy)
{
	assert(pStimulusConfig);
	assert(pConfigProxy);

	const float fEnvelopeValue = GetStimulusEnvelopeValue(fCurrTime, invoke, pStimulusConfig);
	const float fPulseValue = GetStimulusPulseValue(fCurrTime, invoke, pStimulusConfig);
	const float fModValue = GetStimulusModifierValue(invoke, pConfigProxy, pStimulusConfig);

	invoke.m_envelopeData.m_fCurrentValue = fEnvelopeValue;

	if (invoke.IsRunning())
		invoke.m_envelopeData.m_fLastRunningValue = fEnvelopeValue;
	else
		invoke.m_envelopeData.m_fLastReleasingValue = fEnvelopeValue;

	invoke.m_bMustRun = false;
	
	return GetStimulusTotalValue(fCurrTime, fEnvelopeValue, fPulseValue, fModValue);
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrack::GetStimulusTotalValue(float fCurrTime, float fEnvelopeValue, float fPulseValue, float fModValue) const
{
	float fObsoleteDecay = 1.0f;
	if (m_fObsoleteTimer > 0.0f)
	{
		float fDeadForgetTimer = gAIEnv.CVars.TargetTracks_DeadForgetTime;
		fDeadForgetTimer = (fDeadForgetTimer > FLT_EPSILON ? fDeadForgetTimer : 1.0f);
		fObsoleteDecay = 1.0f - (fCurrTime - m_fObsoleteTimer) / fDeadForgetTimer;
	}

	return (fEnvelopeValue + fPulseValue) * fModValue * fObsoleteDecay;
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrack::GetStimulusEnvelopeValue(float fCurrTime, const SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(pStimulusConfig);

	if (invoke.IsRunning())
	{
		// Ignore (skipped if last release value is non-zero)
		const bool bMustIgnore = (invoke.m_envelopeData.m_fLastReleasingValue > FLT_EPSILON);
		const float fIgnoreEnd = invoke.m_envelopeData.m_fStartTime + (!bMustIgnore ? pStimulusConfig->m_fIgnore : 0.0f);
		if (!bMustIgnore && fCurrTime < fIgnoreEnd)
		{
			return 0.0f;
		}

		// Attack
		const float fAttackEnd = fIgnoreEnd + pStimulusConfig->m_fAttack;
		if (fCurrTime <= fAttackEnd)
		{
			const float fDuration = fAttackEnd - fIgnoreEnd;
			const float fAttackRatio = (fCurrTime - fIgnoreEnd) / (fDuration > FLT_EPSILON ? fDuration : 1.0f);
			return invoke.m_envelopeData.m_fLastReleasingValue + (pStimulusConfig->m_fPeak - invoke.m_envelopeData.m_fLastReleasingValue) * fAttackRatio;
		}

		// Decay
		const float fDecayEnd = fAttackEnd + pStimulusConfig->m_fDecay;
		if (fCurrTime <= fDecayEnd)
		{
			const float fDuration = fDecayEnd - fAttackEnd;
			const float fDecayRatio = (fCurrTime - fAttackEnd) / (fDuration > FLT_EPSILON ? fDuration : 1.0f);
			return (pStimulusConfig->m_fPeak - fDecayRatio * (pStimulusConfig->m_fPeak - pStimulusConfig->m_fPeak * pStimulusConfig->m_fSustainRatio));
		}

		// Sustain
		return pStimulusConfig->m_fPeak * pStimulusConfig->m_fSustainRatio;
	}

	// Release
	const float fReleaseStart = invoke.m_envelopeData.m_fLastInvokeTime;
	const float fReleaseEnd = fReleaseStart + pStimulusConfig->m_fRelease;
	const float fReleaseDuration = fReleaseEnd - fReleaseStart;
	const float fReleaseRatio = (fCurrTime - fReleaseStart) / (fReleaseDuration > FLT_EPSILON ? fReleaseDuration : 1.0f);
	return max(0.0f, invoke.m_envelopeData.m_fLastRunningValue - fReleaseRatio * invoke.m_envelopeData.m_fLastRunningValue);
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrack::GetStimulusPulseValue(float fCurrTime, const SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(pStimulusConfig);

	// Apply combined pulse value
	float fPulseValue = 0.0f;
	SStimulusInvocation::TPulseTriggersContainer::const_iterator itPulse = invoke.m_pulseTriggers.begin();
	SStimulusInvocation::TPulseTriggersContainer::const_iterator itPulseEnd = invoke.m_pulseTriggers.end();
	for (; itPulse != itPulseEnd; ++itPulse)
	{
		const SStimulusInvocation::SPulseTrigger& pulseTrigger = *itPulse;

		TargetTrackHelpers::STargetTrackStimulusConfig::TPulseContainer::const_iterator itPulseDef = pStimulusConfig->m_pulses.find(pulseTrigger.uPulseNameHash);
		if (itPulseDef != pStimulusConfig->m_pulses.end())
		{
			const TargetTrackHelpers::STargetTrackPulseConfig& pulseDef = itPulseDef->second;

			const float fDT = fCurrTime - pulseTrigger.fTriggerTime;
			const float fRatio = clamp((pulseDef.m_fDuration > FLT_EPSILON ? 1.0f - fDT/pulseDef.m_fDuration : 0.0f), 0.0f, 1.0f);
			fPulseValue += pulseDef.m_fValue * fRatio;

			if (fRatio <= 0.0f)
			{
				pulseTrigger.bObsolete = true;
			}
		}
	}

	return fPulseValue;
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrack::GetStimulusModifierValue(const SStimulusInvocation &invoke, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(pStimulusConfig);
	assert(pConfigProxy);

	// Value is used as a product
	float fModValue = 1.0f;

	TargetTrackHelpers::STargetTrackStimulusConfig::TModifierContainer::const_iterator itMod = pStimulusConfig->m_modifiers.begin();
	TargetTrackHelpers::STargetTrackStimulusConfig::TModifierContainer::const_iterator itModEnd = pStimulusConfig->m_modifiers.end();
	for (; itMod != itModEnd; ++itMod)
	{
		const TargetTrackHelpers::STargetTrackModifierConfig& modifierInfo = itMod->second;
		const ITargetTrackModifier *pModifier = pConfigProxy->GetTargetTrackModifier(modifierInfo.m_uId);

		if (pModifier)
		{
			fModValue *= pModifier->GetModValue(this, invoke.m_vLastPos, invoke.m_envelopeData, modifierInfo);
		}
	}

	return fModValue;
}

#ifdef TARGET_TRACK_DEBUG
//////////////////////////////////////////////////////////////////////////
void CTargetTrack::DebugDraw(CDebugDrawContext &dc, int iIndex, float &fColumnX, float &fColumnY, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy) const
{
	CWeakRef<CAIObject> refObject = gAIEnv.pObjectContainer->GetWeakRef(m_aiObjectId);
	CAIObject *pObject = refObject.GetAIObject();
	assert(pObject);
	if (!pObject)
		return;

	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	const float fCurrTime = pAISystem->GetFrameStartTime().GetSeconds();

	const ColorB textCol(255,255,255,255);
	const ColorB textActiveCol(0,128,0,255);

	dc->Draw2dLabel(fColumnX, fColumnY, 1.5f, iIndex > 0 ? textCol : textActiveCol, false, "Track \'%s\' [%.3f - %d]", pObject->GetName(), m_fTrackValue, iIndex+1);

	TStimuliInvocationContainer::const_iterator itStimulusInvoke = m_StimuliInvocations.begin();
	TStimuliInvocationContainer::const_iterator itStimulusInvokeEnd = m_StimuliInvocations.end();
	for (; itStimulusInvoke != itStimulusInvokeEnd; ++itStimulusInvoke)
	{
		fColumnY += 15.0f;

		const uint32 uStimulusHash = itStimulusInvoke->first;
		const SStimulusInvocation& invoke = itStimulusInvoke->second;
		string sStimulusName = "Unknown";
		float fStimulusValue = 0.0f;
		float fEnvelopeValue = 0.0f;
		float fPulseValue = 0.0f;
		float fModValue = 1.0f;

		const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig = NULL;
		if (pConfigProxy->GetTargetTrackStimulusConfig(m_uConfigHash, uStimulusHash, pStimulusConfig))
		{
			sStimulusName = pStimulusConfig->m_sStimulus;
			fEnvelopeValue = GetStimulusEnvelopeValue(fCurrTime, invoke, pStimulusConfig);
			fPulseValue = GetStimulusPulseValue(fCurrTime, invoke, pStimulusConfig);
			fModValue = GetStimulusModifierValue(invoke, pConfigProxy, pStimulusConfig);
			fStimulusValue = GetStimulusTotalValue(fCurrTime, fEnvelopeValue, fPulseValue, fModValue);
		}

		dc->Draw2dLabel(fColumnX+5.0f, fColumnY, 1.2f, textCol, false, "Stimulus \'%s\' @ [%.3f : E = %.3f : P = %.3f : M = %.3f]", sStimulusName.c_str(), fStimulusValue, fEnvelopeValue, fPulseValue, fModValue);
		fColumnY += 15.0f;

		dc->Draw2dLabel(fColumnX+10.0f, fColumnY, 1.0f, textCol, false, "Running: %d (P = %d)", invoke.IsRunning() ? 1 : 0, invoke.m_pulseTriggers.size());
		dc->Draw2dLabel(fColumnX+150.0f, fColumnY, 1.0f, textCol, false, "Start: %.3f", invoke.m_envelopeData.m_fStartTime);
		dc->Draw2dLabel(fColumnX+220.0f, fColumnY, 1.0f, textCol, false, "Last: %.3f", invoke.m_envelopeData.m_fLastInvokeTime);
	}
}
#endif //TARGET_TRACK_DEBUG
