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

#ifndef __TARGET_TRACK_H__
#define __TARGET_TRACK_H__

#include "TargetTrackCommon.h"

class CDebugDrawContext;

class CTargetTrack
{
public:
	CTargetTrack();
	~CTargetTrack();

	// Used for sorting, compares value of track
	bool operator ()(const CTargetTrack* a, const CTargetTrack* b) const
	{
		assert(a && b);
		return a->m_fTrackValue > b->m_fTrackValue;
	}

	tAIObjectID GetAIObjectID() const { return m_aiObjectId; }
	tAIObjectID GetAIGroupOwnerID() const { return m_aiGroupOwnerId; }
	const Vec3& GetTargetPos() const { return m_vTargetPos; }
	float GetTrackValue() const { return m_fTrackValue; }
	float GetFirstAggressiveTime() const { return m_fFirstAggressiveTime; }
	EAITargetType GetTargetType() const { return m_eTargetType; }
	EAITargetThreat GetTargetThreat() const { return m_eTargetThreat; }

	void Init(tAIObjectID aiGroupOwnerId, tAIObjectID aiObjectId, uint32 uConfigHash);
	void ResetForPool();
	void Serialize(TSerialize ser);

	void OnAgentRemoved();
	bool Update(float fCurrTime, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy);

	// Invoke a stimulus on the track
	bool InvokeStimulus(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash);
	
	// Create/update the pulse for the given stimulus
	bool TriggerPulse(uint32 uStimulusNameHash, uint32 uPulseNameHash);

#ifdef TARGET_TRACK_DEBUG
	// Debugging
	void SetLastDebugDrawTime(float fTime) const { m_fLastDebugDrawTime = fTime; }
	float GetLastDebugDrawTime() const { return m_fLastDebugDrawTime; }
	void SetDebugGraphIndex(uint32 uDebugGraphIndex) { m_uDebugGraphIndex = uDebugGraphIndex; }
	uint32 GetDebugGraphIndex() const { return m_uDebugGraphIndex; }
	void DebugDraw(CDebugDrawContext &dc, int iIndex, float &fColumnX, float &fColumnY, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy) const;
#endif //TARGET_TRACK_DEBUG

private:
	// Stimuli invocation - records the last time a stimulus was invoked
	struct SStimulusInvocation
	{
		// Pulse triggers
		struct SPulseTrigger
		{
			uint32 uPulseNameHash;
			float fTriggerTime;
			mutable bool bObsolete;

			SPulseTrigger() : uPulseNameHash(0), fTriggerTime(0.0f), bObsolete(false) { }
			SPulseTrigger(uint32 hash) : uPulseNameHash(hash), fTriggerTime(0.0f), bObsolete(false) { }

			static bool IsObsolete(const SPulseTrigger& pulseTrigger)
			{
				return pulseTrigger.bObsolete;
			}

			void Serialize(TSerialize ser);
		};
		typedef std::vector<SPulseTrigger> TPulseTriggersContainer;
		TPulseTriggersContainer m_pulseTriggers;

		Vec3 m_vLastPos;
		TargetTrackHelpers::SEnvelopeData m_envelopeData;
		EAITargetThreat m_eTargetThreat;
		TargetTrackHelpers::EAIEventStimulusType m_eStimulusType;
		bool m_bMustRun;

		SStimulusInvocation() : m_vLastPos(ZERO), m_eTargetThreat(AITHREAT_NONE), m_eStimulusType(TargetTrackHelpers::eEST_Generic), m_bMustRun(false)
		{

		}

		bool IsRunning() const;
		void Serialize(TSerialize ser);
	};

#ifdef TARGET_TRACK_DOTARGETTHREAT
	void UpdateTargetThreat(EAITargetThreat &outTargetThreat, float fCurrTime, const SStimulusInvocation &invoke);
#endif //TARGET_TRACK_DOTARGETTHREAT

#ifdef TARGET_TRACK_DOTARGETTYPE
	bool UpdateTargetType(EAITargetType &outTargetType, EAITargetThreat eTargetThreat, const SStimulusInvocation &invoke);
#endif //TARGET_TRACK_DOTARGETTYPE

	// Update helpers for stimulus invocations
	void UpdateStimulusInvoke(SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent) const;
	void UpdateStimulusPulse(SStimulusInvocation &invoke, uint32 uPulseNameHash) const;
	void UpdatePulseValue(SStimulusInvocation::SPulseTrigger &pulseTrigger) const;

	// Helpers to calculate the current value of a stimulus invocation
	float UpdateStimulusValue(float fCurrTime, SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy);
	float GetStimulusEnvelopeValue(float fCurrTime, const SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig) const;
	float GetStimulusPulseValue(float fCurrTime, const SStimulusInvocation &invoke, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig) const;
	float GetStimulusModifierValue(const SStimulusInvocation &invoke, TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy, const TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig) const;
	float GetStimulusTotalValue(float fCurrTime, float fEnvelopeValue, float fPulseValue, float fModValue) const;

	typedef std::map<uint32, SStimulusInvocation> TStimuliInvocationContainer;
	TStimuliInvocationContainer m_StimuliInvocations;

	Vec3 m_vTargetPos;
	EAITargetType m_eTargetType;
	EAITargetThreat m_eTargetThreat;
	tAIObjectID m_aiGroupOwnerId;
	tAIObjectID m_aiObjectId;
	uint32 m_uConfigHash;
	int m_iLastUpdateFrame;
	float m_fTrackValue;
	float m_fObsoleteTimer;
	float m_fFirstAggressiveTime;

#ifdef TARGET_TRACK_DEBUG
	// Debugging
	mutable float m_fLastDebugDrawTime;
	uint32 m_uDebugGraphIndex;
#endif //TARGET_TRACK_DEBUG
};

#endif //__TARGET_TRACK_H__
