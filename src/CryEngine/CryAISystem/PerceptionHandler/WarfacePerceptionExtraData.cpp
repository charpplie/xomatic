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

#include "StdAfx.h"
#include "WarfacePerceptionExtraData.h"
#include "StringUtils.h"

// Default min threat list
static EAITargetThreat g_DefaultMinAudioThreats[AISOUND_LAST] =
{
	AITHREAT_NONE,			//AISOUND_GENERIC
	AITHREAT_INTERESTING,	//AISOUND_COLLISION
	AITHREAT_THREATENING,	//AISOUND_COLLISION_LOUD
	AITHREAT_INTERESTING,	//AISOUND_MOVEMENT
	AITHREAT_THREATENING,	//AISOUND_MOVEMENT_LOUD
	AITHREAT_THREATENING,	//AISOUND_WEAPON
	AITHREAT_AGGRESSIVE,	//AISOUND_EXPLOSION
};

//////////////////////////////////////////////////////////////////////////
CWarfacePerceptionExtraData::CWarfacePerceptionExtraData()
: m_fAggressiveAudioRange(10.0f)
, m_fThreateningAudioRange(20.0f)
, m_fInterestingAudioRange(30.0f)
, m_fNearReactVisualTime(0.25f)
, m_fAggressiveVisualTime(0.05f)
, m_fThreateningVisualTime(0.15f)
, m_fInterestingVisualTime(0.25f)
, m_fTargetStationaryTime(0.0f)
, m_fTargetLongMemoryTime(0.0f)
, m_bVisualCanForget(true)
{
	// Copy defaults
	memcpy(m_MinAudioThreats, g_DefaultMinAudioThreats, sizeof(EAITargetThreat)*AISOUND_LAST);
}

//////////////////////////////////////////////////////////////////////////
CWarfacePerceptionExtraData::~CWarfacePerceptionExtraData()
{

}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionExtraData::SetData(uint32 uDataId, ScriptAnyValue dataAny)
{
	static const uint32 uHash_AggressiveAudioRange		= CryStringUtils::CalculateHash("AggressiveAudioRange");
	static const uint32 uHash_ThreateningAudioRange		= CryStringUtils::CalculateHash("ThreateningAudioRange");
	static const uint32 uHash_InterestingAudioRange		= CryStringUtils::CalculateHash("InterestingAudioRange");
	static const uint32 uHash_NearReactVisualTime		= CryStringUtils::CalculateHash("NearReactVisualTime");
	static const uint32 uHash_AggressiveVisualTime		= CryStringUtils::CalculateHash("AggressiveVisualTime");
	static const uint32 uHash_ThreateningVisualTime		= CryStringUtils::CalculateHash("ThreateningVisualTime");
	static const uint32 uHash_InterestingVisualTime		= CryStringUtils::CalculateHash("InterestingVisualTime");
	static const uint32 uHash_TargetStationaryTime		= CryStringUtils::CalculateHash("TargetStationaryTime");
	static const uint32 uHash_TargetLongMemoryTime		= CryStringUtils::CalculateHash("TargetLongMemoryTime");
	static const uint32 uHash_VisualCanForget			= CryStringUtils::CalculateHash("VisualCanForget");

	bool bResult = true;
	if (uHash_AggressiveAudioRange == uDataId)
	{
		m_fAggressiveAudioRange = dataAny.number;
	}
	else if (uHash_ThreateningAudioRange == uDataId)
	{
		m_fThreateningAudioRange = dataAny.number;
	}
	else if (uHash_InterestingAudioRange == uDataId)
	{
		m_fInterestingAudioRange = dataAny.number;
	}
	else if (uHash_NearReactVisualTime == uDataId)
	{
		m_fNearReactVisualTime = dataAny.number;
	}
	else if (uHash_AggressiveVisualTime == uDataId)
	{
		m_fAggressiveVisualTime = dataAny.number;
	}
	else if (uHash_ThreateningVisualTime == uDataId)
	{
		m_fThreateningVisualTime = dataAny.number;
	}
	else if (uHash_InterestingVisualTime == uDataId)
	{
		m_fInterestingVisualTime = dataAny.number;
	}
	else if (uHash_TargetStationaryTime == uDataId)
	{
		m_fTargetStationaryTime = dataAny.number;
	}
	else if (uHash_TargetLongMemoryTime == uDataId)
	{
		m_fTargetLongMemoryTime = dataAny.number;
	}
	else if (uHash_VisualCanForget == uDataId)
	{
		m_bVisualCanForget = dataAny.b;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
EAITargetThreat CWarfacePerceptionExtraData::GetMinAudioThreat(EAISoundStimType eSoundType) const
{
	const bool bSoundValid = (eSoundType > 0 && eSoundType < AISOUND_LAST);
	CRY_ASSERT(bSoundValid);

	return (bSoundValid ? m_MinAudioThreats[eSoundType] : AITHREAT_NONE);
}

//////////////////////////////////////////////////////////////////////////
bool CWarfacePerceptionExtraData::SetMinAudioThreat(EAISoundStimType eSoundType, EAITargetThreat eMinThreat)
{
	const bool bSoundValid = (eSoundType > 0 && eSoundType < AISOUND_LAST);
	const bool bThreatValid = (eMinThreat > 0 && eMinThreat < AITHREAT_LAST);

	bool bResult = (bSoundValid && bThreatValid);
	CRY_ASSERT(bResult);
	if (bResult)
		m_MinAudioThreats[eSoundType] = eMinThreat;

	return bResult;
}
