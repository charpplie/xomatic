/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2008.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Perception extra data for puppets using Warface method
  
 -------------------------------------------------------------------------
  History:
  - 05:20:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __WARFACEEXTRADATA_H__
#define __WARFACEEXTRADATA_H__

#include "IPerceptionHandler.h"

class CWarfacePerceptionExtraData : public IPerceptionExtraData
{
public:
	CWarfacePerceptionExtraData();
	virtual ~CWarfacePerceptionExtraData();

	virtual bool SetData(uint32 uDataId, ScriptAnyValue dataAny);

	// Accessors
	float GetAggressiveAudioRange() const { return m_fAggressiveAudioRange; }
	float GetThreateningAudioRange() const { return m_fThreateningAudioRange; }
	float GetInterestingAudioRange() const { return m_fInterestingAudioRange; }
	float GetNearReactVisualTime() const { return m_fNearReactVisualTime; }
	float GetAggressiveVisualTime() const { return m_fAggressiveVisualTime; }
	float GetThreateningVisualTime() const { return m_fThreateningVisualTime; }
	float GetInterestingVisualTime() const { return m_fInterestingVisualTime; }
	float GetTargetStationaryTime() const { return m_fTargetStationaryTime; }
	float GetTargetLongMemoryTime() const { return m_fTargetLongMemoryTime; }
	bool CanVisualForget() const  { return m_bVisualCanForget; }

	EAITargetThreat GetMinAudioThreat(EAISoundStimType eSoundType) const;
	bool SetMinAudioThreat(EAISoundStimType eSoundType, EAITargetThreat eMinThreat);

private:
	float m_fAggressiveAudioRange;
	float m_fThreateningAudioRange;
	float m_fInterestingAudioRange;

	float m_fNearReactVisualTime;
	float m_fAggressiveVisualTime;
	float m_fThreateningVisualTime;
	float m_fInterestingVisualTime;

	float m_fTargetStationaryTime;
	float m_fTargetLongMemoryTime;

	bool m_bVisualCanForget;

	EAITargetThreat m_MinAudioThreats[AISOUND_LAST];
};

#endif //__WARFACEEXTRADATA_H__
