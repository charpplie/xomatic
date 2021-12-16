/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Definitions for various modifiers to target tracks
  
 -------------------------------------------------------------------------
  History:
  - 02:08:2010: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "TargetTrackModifiers.h"
#include "TargetTrackManager.h"
#include "TargetTrack.h"

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceModifier::CTargetTrackDistanceModifier()
{

}

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceModifier::~CTargetTrackDistanceModifier()
{

}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackDistanceModifier::IsMatchingTag(const char* szTag) const
{
	return stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackDistanceModifier::GetTag() const
{
	return "Distance";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackDistanceModifier::GetModValue(const CTargetTrack *pTrack, const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData &envelopeData,
												const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
	assert(pTrack);

	float fDistance = 0.0f;

	CWeakRef<CAIObject> refOwner = gAIEnv.pObjectContainer->GetWeakRef(pTrack->GetAIGroupOwnerID());
	if (refOwner.IsValid())
	{
		CAIObject *pOwner = refOwner.GetAIObject();
		assert(pOwner);

		const Vec3 &vOwnerPos = pOwner->GetPos();
		fDistance = vPos.GetDistance(vOwnerPos);

		// Convert to ratio using limit
		if (modConfig.m_fLimit > FLT_EPSILON)
		{
			fDistance = ((modConfig.m_fLimit-fDistance) / modConfig.m_fLimit);
		}
	}

	return (fDistance * modConfig.m_fValue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTargetTrackHostileModifier::CTargetTrackHostileModifier()
{

}

//////////////////////////////////////////////////////////////////////////
CTargetTrackHostileModifier::~CTargetTrackHostileModifier()
{

}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackHostileModifier::IsMatchingTag(const char* szTag) const
{
	return stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackHostileModifier::GetTag() const
{
	return "Hostile";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackHostileModifier::GetModValue(const CTargetTrack *pTrack, const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData &envelopeData,
											   const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
	assert(pTrack);

	bool bIsHostile = false;

	CWeakRef<CAIObject> refOwner = gAIEnv.pObjectContainer->GetWeakRef(pTrack->GetAIGroupOwnerID());
	CWeakRef<CAIObject> refTarget = gAIEnv.pObjectContainer->GetWeakRef(pTrack->GetAIObjectID());
	if (refOwner.IsValid() && refTarget.IsValid())
	{
		CAIObject *pOwner = refOwner.GetAIObject();
		CAIObject *pTarget = refTarget.GetAIObject();
		assert(pOwner && pTarget);

		bIsHostile = pOwner->IsHostile(pTarget);
	}

	return (1.0f * bIsHostile ? modConfig.m_fValue : 1.0f);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTargetTrackClassThreatModifier::CTargetTrackClassThreatModifier()
{

}

//////////////////////////////////////////////////////////////////////////
CTargetTrackClassThreatModifier::~CTargetTrackClassThreatModifier()
{

}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackClassThreatModifier::IsMatchingTag(const char* szTag) const
{
	return stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackClassThreatModifier::GetTag() const
{
	return "ClassThreat";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackClassThreatModifier::GetModValue(const CTargetTrack *pTrack, const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData &envelopeData,
												   const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
	assert(pTrack);

	tAIObjectID aiObjectId = pTrack->GetAIObjectID();
	const float fClassThreat = gAIEnv.pTargetTrackManager->GetTargetClassThreat(aiObjectId);

	return (fClassThreat * modConfig.m_fValue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceIgnoreModifier::CTargetTrackDistanceIgnoreModifier()
{

}

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceIgnoreModifier::~CTargetTrackDistanceIgnoreModifier()
{

}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackDistanceIgnoreModifier::IsMatchingTag(const char* szTag) const
{
	return stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackDistanceIgnoreModifier::GetTag() const
{
	return "DistanceIgnore";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackDistanceIgnoreModifier::GetModValue(const CTargetTrack *pTrack, const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData &envelopeData,
													  const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
	assert(pTrack);

	// Ignore modifier doesn't run when the envelope was not fully released last time
	if (envelopeData.m_fLastReleasingValue > FLT_EPSILON)
		return 1.0f;

	float fDistance = 0.0f;

	CWeakRef<CAIObject> refOwner = gAIEnv.pObjectContainer->GetWeakRef(pTrack->GetAIGroupOwnerID());
	if (refOwner.IsValid())
	{
		CAIObject *pOwner = refOwner.GetAIObject();
		assert(pOwner);

		const Vec3 &vOwnerPos = pOwner->GetPos();
		fDistance = vPos.GetDistance(vOwnerPos);

		// Convert to ratio using limit
		if (modConfig.m_fLimit > FLT_EPSILON)
		{
			fDistance /= modConfig.m_fLimit;
		}
	}

	const float fCurrTime = GetAISystem()->GetFrameStartTime().GetSeconds();
	const float fDT = (fCurrTime - envelopeData.m_fStartTime);

	return (fDT <= fDistance * modConfig.m_fValue ? 0.0f : 1.0f);
}
