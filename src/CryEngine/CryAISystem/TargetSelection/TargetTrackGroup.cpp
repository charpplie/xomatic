/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Contains an agent's target tracks and handles updating them
  
 -------------------------------------------------------------------------
  History:
  - 02:01:2010: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "TargetTrackGroup.h"
#include "TargetTrackManager.h"
#include "TargetTrack.h"

#ifdef TARGET_TRACK_DOTARGETTHREAT
	#include "Puppet.h"
#endif //TARGET_TRACK_DOTARGETTHREAT

#ifdef TARGET_TRACK_DEBUG
	#include "DebugDrawContext.h"
	#include "IDebugHistory.h"
	#include "IGame.h"
	#include "IGameFramework.h"
#endif //TARGET_TRACK_DEBUG

//////////////////////////////////////////////////////////////////////////
CTargetTrackGroup::CTargetTrackGroup(TargetTrackHelpers::ITargetTrackPoolProxy *pTrackPoolProxy, tAIObjectID aiObjectId, uint32 uConfigHash, int nTargetLimit)
: m_pTrackPoolProxy(pTrackPoolProxy)
, m_aiObjectId(aiObjectId)
, m_aiLastBestTargetId(0)
, m_uConfigHash(uConfigHash)
, m_nTargetLimit(max(nTargetLimit,0))
, m_bNeedSort(false)
, m_bEnabled(true)
{
	assert(m_pTrackPoolProxy);
	assert(m_aiObjectId > 0);
	assert(m_uConfigHash > 0);

	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	CWeakRef<CAIObject> refAIObject = gAIEnv.pObjectContainer->GetWeakRef(aiObjectId);
	const bool bAIObjectValid = refAIObject.IsValid();

	// Create dummy representation for the dummy potential target
	char szName[256];
	_snprintf(szName,256,"Target Track Perception Dummy for %s", bAIObjectValid ? refAIObject.GetAIObject()->GetName() : "NOBODY");
	GetAISystem()->CreateDummyObject(m_dummyPotentialTarget.refDummyRepresentation, szName);
	
	CAIObject *pDummyRep = m_dummyPotentialTarget.refDummyRepresentation.GetAIObject();
	assert(pDummyRep);
	if (pDummyRep && bAIObjectValid)
	{
		pDummyRep->SetAssociation(refAIObject);
	}

#ifdef TARGET_TRACK_DEBUG
	m_fLastGraphUpdate = 0.0f;
	m_pDebugHistoryManager = gEnv->pGame->GetIGameFramework()->CreateDebugHistoryManager();
	assert(m_pDebugHistoryManager);

	memset(m_bDebugGraphOccupied, 0, sizeof(m_bDebugGraphOccupied));
#endif //TARGET_TRACK_DEBUG
}

//////////////////////////////////////////////////////////////////////////
CTargetTrackGroup::~CTargetTrackGroup()
{
	Reset();

#ifdef TARGET_TRACK_DEBUG
	SAFE_RELEASE(m_pDebugHistoryManager);
#endif //TARGET_TRACK_DEBUG
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::Reset()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	// Add all active tracks back to the pool
	TTargetTrackContainer::iterator itTrack = m_TargetTracks.begin();
	TTargetTrackContainer::iterator itTrackEnd = m_TargetTracks.end();
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		CTargetTrack *pTrack = itTrack->second;
		assert(pTrack);

		m_pTrackPoolProxy->AddTargetTrackToPool(pTrack);
	}

	m_TargetTracks.clear();
	m_SortedTracks.clear();
	m_bNeedSort = false;

#ifdef TARGET_TRACK_DEBUG
	m_pDebugHistoryManager->Clear();
#endif //TARGET_TRACK_DEBUG
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::Serialize_Write(TSerialize ser)
{
	assert(ser.IsWriting());

	int iTrackCount = m_TargetTracks.size();
	ser.Value("iTrackCount", iTrackCount);

	TTargetTrackContainer::iterator itTrack = m_TargetTracks.begin();
	TTargetTrackContainer::iterator itTrackEnd = m_TargetTracks.end();
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		const tAIObjectID targetId = itTrack->first;
		CTargetTrack *pTrack = itTrack->second;
		assert(targetId > 0 && pTrack);

		CWeakRef<CAIObject> refTarget = gAIEnv.pObjectContainer->GetWeakRef(targetId);
		assert(refTarget.IsValid());

		ser.BeginGroup("Track");
		{
			refTarget.Serialize(ser);

			pTrack->Serialize(ser);
		}
		ser.EndGroup();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::Serialize_Read(TSerialize ser)
{
	assert(ser.IsReading());

	assert(m_TargetTracks.empty());

	int iTrackCount = 0;
	ser.Value("iTrackCount", iTrackCount);

	for (int iTrack = 0; iTrack < iTrackCount; ++iTrack)
	{
		CWeakRef<CAIObject> refTarget;

		ser.BeginGroup("Track");
		{
			refTarget.Serialize(ser);
			assert(refTarget.IsValid());

			CTargetTrack *pTrack = GetTargetTrack(refTarget.GetObjectID());
			assert(pTrack);
			if (pTrack)
			{
				pTrack->Serialize(ser);
			}
		}
		ser.EndGroup();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::OnAgentRemoved(tAIObjectID aiTargetId)
{
	TTargetTrackContainer::iterator itTrack = m_TargetTracks.find(aiTargetId);
	if (itTrack != m_TargetTracks.end())
	{
		CTargetTrack *pTrack = itTrack->second;
		assert(pTrack);

		pTrack->OnAgentRemoved();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::Update(TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const float fCurrTime = GetAISystem()->GetFrameStartTime().GetSeconds();

	m_SortedTracks.clear();
	m_SortedTracks.reserve(m_TargetTracks.size());

	// Update the tracks
	TTargetTrackContainer::iterator itTrack = m_TargetTracks.begin();
	TTargetTrackContainer::iterator itTrackEnd = m_TargetTracks.end();
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		CTargetTrack *pTrack = itTrack->second;
		assert(pTrack);

		if (pTrack->Update(fCurrTime, pConfigProxy))
		{
			m_SortedTracks.push_back(pTrack);
		}
	}

	m_bNeedSort = (!m_SortedTracks.empty());
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::UpdateSortedTracks(bool bForced)
{
	if (m_bNeedSort || bForced)
	{
		m_bNeedSort = false;
		std::sort(m_SortedTracks.begin(), m_SortedTracks.end(), CTargetTrack());
	}
}

//////////////////////////////////////////////////////////////////////////
CTargetTrack* CTargetTrackGroup::GetTargetTrack(tAIObjectID aiTargetId)
{
	CTargetTrack *pTrack = NULL;

	TTargetTrackContainer::iterator itTrack = m_TargetTracks.find(aiTargetId);
	if (itTrack != m_TargetTracks.end())
	{
		pTrack = itTrack->second;
	}
	else
	{
		// Request a new track to use from the pool
		pTrack = m_pTrackPoolProxy->GetUnusedTargetTrackFromPool();
		assert(pTrack);

		pTrack->Init(m_aiObjectId, aiTargetId, m_uConfigHash);
		m_TargetTracks[aiTargetId] = pTrack;

#ifdef TARGET_TRACK_DEBUG
		// Create debug history for it
		uint32 uDebugGraphIndex;
		if (FindFreeGraphSlot(uDebugGraphIndex))
		{
			m_bDebugGraphOccupied[uDebugGraphIndex] = true;
			pTrack->SetDebugGraphIndex(uDebugGraphIndex);

			CWeakRef<CAIObject> refTarget = gAIEnv.pObjectContainer->GetWeakRef(aiTargetId);
			CAIObject *pTarget = refTarget.GetAIObject();
			assert(pTarget);

			IDebugHistory* pDebugHistory = m_pDebugHistoryManager->CreateHistory(pTarget->GetName());
			if (pDebugHistory)
			{
				pDebugHistory->SetVisibility(false);
				pDebugHistory->SetupScopeExtent(-360.0f, 360.0f, 0.0f, 200.0f);
			}
		}
#endif //TARGET_TRACK_DEBUG

	}

	assert(pTrack);
	return pTrack;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::HandleStimulusEvent(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(!stimulusEvent.m_sStimulusName.empty());

	bool bResult = false;

	if (stimulusEvent.m_targetId > 0)
	{
		CTargetTrack *pTrack = GetTargetTrack(stimulusEvent.m_targetId);
		bResult = HandleStimulusEvent_Target(stimulusEvent, uStimulusNameHash, pTrack);
	}
	else
	{
		bResult = HandleStimulusEvent_All(stimulusEvent, uStimulusNameHash);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::TriggerPulse(uint32 uStimulusNameHash, uint32 uPulseNameHash)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bResult = true;

	TTargetTrackContainer::iterator itTrack = m_TargetTracks.begin();
	TTargetTrackContainer::iterator itTrackEnd = m_TargetTracks.end();
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		CTargetTrack *pTrack = itTrack->second;
		assert(pTrack);

		bResult &= pTrack->TriggerPulse(uStimulusNameHash, uPulseNameHash);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::HandleStimulusEvent_All(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bResult = true;

	TTargetTrackContainer::iterator itTrack = m_TargetTracks.begin();
	TTargetTrackContainer::iterator itTrackEnd = m_TargetTracks.end();
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		bResult &= HandleStimulusEvent_Target(stimulusEvent, uStimulusNameHash, itTrack->second);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::HandleStimulusEvent_Target(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash, CTargetTrack *pTrack)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(pTrack);

	return (pTrack && pTrack->InvokeStimulus(stimulusEvent, uStimulusNameHash));
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::IsPotentialTarget(tAIObjectID aiTargetId) const
{
	assert(aiTargetId > 0);

	TTargetTrackContainer::const_iterator itTrack = m_TargetTracks.find(aiTargetId);
	return (itTrack != m_TargetTracks.end());
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::IsDesiredTarget(tAIObjectID aiTargetId) const
{
	assert(aiTargetId > 0);

	return (aiTargetId == m_aiLastBestTargetId);
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::GetDesiredTarget(TargetTrackHelpers::EDesiredTargetMethod eMethod, tAIObjectID &outTargetId, SAIPotentialTarget* &pOutTargetInfo)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bResult = false;

	outTargetId = 0;
	pOutTargetInfo = NULL;

	CTargetTrack *pBestTrack = GetBestTrack(eMethod);
	if (pBestTrack)
	{
		UpdateTargetRepresentation(pBestTrack, outTargetId, pOutTargetInfo);
		bResult = true;
	}

	m_aiLastBestTargetId = outTargetId;

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
CTargetTrack* CTargetTrackGroup::GetBestTrack(TargetTrackHelpers::EDesiredTargetMethod eMethod)
{
	CTargetTrack *pResult = NULL;
	CTargetTrack *pBest = NULL;

	UpdateSortedTracks();

	switch (eMethod & TargetTrackHelpers::eDTM_SELECTION_MASK)
	{
		case TargetTrackHelpers::eDTM_Select_Highest:
		{
			TSortedTracks::iterator itBestTrack = m_SortedTracks.begin();
			TSortedTracks::iterator itTrackEnd = m_SortedTracks.end();
			for (; itBestTrack != itTrackEnd; ++itBestTrack)
			{
				pBest = *itBestTrack;
				if (pBest && TestTrackAgainstFilters(pBest, eMethod))
				{
					pResult = pBest;
					break;
				}
			}
		}
		break;

		case TargetTrackHelpers::eDTM_Select_Lowest:
		{
			TSortedTracks::reverse_iterator itBestTrack = m_SortedTracks.rbegin();
			TSortedTracks::reverse_iterator itTrackEnd = m_SortedTracks.rend();
			for (; itBestTrack != itTrackEnd; ++itBestTrack)
			{
				pBest = *itBestTrack;
				if (pBest && TestTrackAgainstFilters(pBest, eMethod))
				{
					pResult = pBest;
					break;
				}
			}
		}
		break;

		default:
			CRY_ASSERT_MESSAGE(false, "CTargetTrackGroup::GetBestTrack Unhandled desired target method");
			break;
	}

	return pResult;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::TestTrackAgainstFilters(CTargetTrack *pTrack, TargetTrackHelpers::EDesiredTargetMethod eMethod) const
{
	assert(pTrack);

	bool bResult = true;

	// [Kevin:26.02.2010] Need a better method than using the track manager here...
	CTargetTrackManager *pManager = gAIEnv.pTargetTrackManager;
	assert(pManager);

	const uint32 uFilterBitmask = eMethod & TargetTrackHelpers::eDTM_FILTER_MASK;
	const tAIObjectID aiTargetId = pTrack->GetAIObjectID();

	if (uFilterBitmask & TargetTrackHelpers::eDTM_Filter_LimitDesired)
	{
		/*const int iTargetLimit = pManager->GetTargetLimit(aiTargetId);
		bResult &= (iTargetLimit <= 0 || pManager->GetDesiredTargetCount(aiTargetId, m_aiObjectId) < iTargetLimit);*/
	}

	if (uFilterBitmask & TargetTrackHelpers::eDTM_Filter_LimitPotential)
	{
		/*const int iTargetLimit = pManager->GetTargetLimit(aiTargetId);
		bResult &= (iTargetLimit <= 0 || pManager->GetPotentialTargetCount(aiTargetId, m_aiObjectId) < iTargetLimit);*/
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::UpdateTargetRepresentation(const CTargetTrack *pBestTrack, tAIObjectID &outTargetId, SAIPotentialTarget* &pOutTargetInfo)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const tAIObjectID aiTargetId = pBestTrack->GetAIObjectID();
	CWeakRef<CAIObject> refTarget = gAIEnv.pObjectContainer->GetWeakRef(aiTargetId);
	assert(refTarget.IsValid());
	CAIObject *pTarget = refTarget.GetAIObject();

	CAIObject *pDummyRep = m_dummyPotentialTarget.refDummyRepresentation.GetAIObject();
	assert(pDummyRep);

	pDummyRep->SetPos(pBestTrack->GetTargetPos());
	pDummyRep->SetAssociation(refTarget);

	EAITargetType eTargetType = AITARGET_NONE;
	EAITargetThreat eTargetThreat = AITHREAT_NONE;
	GetTargetInfo(aiTargetId, eTargetType, eTargetThreat);
	switch (eTargetType)
	{
		case AITARGET_VISUAL:
		{
			pDummyRep->SetSubType(IAIObject::STP_NONE);
			outTargetId = aiTargetId;
		}
		break;

		case AITARGET_MEMORY:
		{
			pDummyRep->SetSubType(IAIObject::STP_MEMORY);
			outTargetId = pDummyRep->GetAIObjectID();
		}
		break;

		case AITARGET_SOUND:
		{
			pDummyRep->SetSubType(IAIObject::STP_SOUND);
			outTargetId = pDummyRep->GetAIObjectID();
		}
		break;

		case AITARGET_NONE:
		{
			pDummyRep->SetSubType(IAIObject::STP_NONE);
			outTargetId = pDummyRep->GetAIObjectID();
		}
		break;

		default:
			CRY_ASSERT_MESSAGE(0, "CTargetTrackGroup::UpdateDummyTargetRep Unhandled AI target type");
			break;
	}

	// Update the dummy target
	m_dummyPotentialTarget.bNeedsUpdating = false;
	m_dummyPotentialTarget.type = eTargetType;
	m_dummyPotentialTarget.threat = eTargetThreat;
	m_dummyPotentialTarget.exposureThreat = eTargetThreat;

	pOutTargetInfo = &m_dummyPotentialTarget;
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::GetTargetInfo(tAIObjectID aiTargetId, EAITargetType &outType, EAITargetThreat &outThreat) const
{
	assert(aiTargetId > 0);

	bool bResult = false;
	outType = AITARGET_NONE;
	outThreat = AITHREAT_NONE;

	TTargetTrackContainer::const_iterator itTrack = m_TargetTracks.find(aiTargetId);
	if (itTrack != m_TargetTracks.end())
	{
		CTargetTrack *pTrack = itTrack->second;
		assert(pTrack);

		bResult = true;
		outType = pTrack->GetTargetType();
		outThreat = pTrack->GetTargetThreat();

#ifdef TARGET_TRACK_DOTARGETTHREAT
		ModifyTargetThreat(pTrack, outThreat);
#endif //TARGET_TRACK_DOTARGETTHREAT

	}

	return bResult;
}

#ifdef TARGET_TRACK_DOTARGETTHREAT
//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::ModifyTargetThreat(const CTargetTrack *pTrack, EAITargetThreat &outThreat) const
{
	assert(pTrack);

	bool bResult = false;

	const float fFirstAggressiveTime = pTrack->GetFirstAggressiveTime();
	if (outThreat >= AITHREAT_AGGRESSIVE && fFirstAggressiveTime > 0.0f)
	{
		CWeakRef<CAIObject> refOwner = gAIEnv.pObjectContainer->GetWeakRef(m_aiObjectId);
		CWeakRef<CAIObject> refTarget = gAIEnv.pObjectContainer->GetWeakRef(pTrack->GetAIObjectID());
		assert(refOwner.IsValid() && refTarget.IsValid());
		CPuppet *pOwnerPuppet = CastToCPuppetSafe(refOwner.GetAIObject());
		CAIObject *pTarget = refTarget.GetAIObject();

		if (pOwnerPuppet && pTarget)
		{
			const AgentParameters &parameters = pOwnerPuppet->GetParameters();
			const float fCurrTime = GetAISystem()->GetFrameStartTime().GetSeconds();
			const float fDT = (fCurrTime - fFirstAggressiveTime) * parameters.m_PerceptionParams.perceptionScale.visual;

			const float fMinTime = (pOwnerPuppet->IsAlarmed() ? gAIEnv.CVars.SOMMinimumCombat : gAIEnv.CVars.SOMMinimumRelaxed);
			bool bMakeThreatening = (fDT < fMinTime);
			
			if (!bMakeThreatening)
			{
				const Vec3& vOwnerPos = pOwnerPuppet->GetPos();
				const Vec3& vTargetPos = pTrack->GetTargetPos();
				const float fDistance = vOwnerPos.GetSquaredDistance(vTargetPos);

				const float fSightRange = pOwnerPuppet->GetMaxTargetVisibleRange(pTarget);
				const float fDistanceSightRatio = fDistance / (fSightRange > FLT_EPSILON ? fSightRange*fSightRange: 1.0f);

				const float fEnvMin = parameters.m_PerceptionParams.sightEnvScaleNormal;
				const float fEnvMax = parameters.m_PerceptionParams.sightEnvScaleAlarmed;
				const float fEnvScale = (fEnvMin + pOwnerPuppet->GetPerceptionAlarmLevel() * (fEnvMax - fEnvMin));
				const float fAggressiveSpeed = (pOwnerPuppet->IsAlarmed() ? gAIEnv.CVars.SOMSpeedCombat : gAIEnv.CVars.SOMSpeedRelaxed) * fEnvScale;

				bMakeThreatening = (fDT < fDistanceSightRatio * fAggressiveSpeed);
			}

			if (bMakeThreatening)
			{
				// Force to threatening for now
				outThreat = AITHREAT_THREATENING;
				bResult = true;
			}
		}
	}

	return bResult;
}
#endif //TARGET_TRACK_DOTARGETTHREAT

#ifdef TARGET_TRACK_DEBUG
//////////////////////////////////////////////////////////////////////////
bool CTargetTrackGroup::FindFreeGraphSlot(uint32 &outIndex) const
{
	outIndex = UINT_MAX;

	for (uint32 i = 0; i < DEBUG_GRAPH_OCCUPIED_SIZE; ++i)
	{
		if (!m_bDebugGraphOccupied[i])
		{
			outIndex = i;
			break;
		}
	}

	return (outIndex < UINT_MAX);
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::DebugDrawTracks(TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy, bool bLastDraw)
{
	CDebugDrawContext dc;
	float fColumnX = 1.0f;
	float fColumnY = 11.0f;
	float fColumnGraphX = fColumnX + 475.0f;
	float fColumnGraphY = fColumnY;
	const float fGraphWidth = 150.0f;
	const float fGraphHeight = 150.0f;
	const float fGraphMargin = 5.0f;

	static float s_fGraphUpdateInterval = 0.125f;
	bool bUpdateGraph = false;
	const float fCurrTime = GetAISystem()->GetFrameStartTime().GetSeconds();
	if (fCurrTime - m_fLastGraphUpdate >= s_fGraphUpdateInterval)
	{
		m_fLastGraphUpdate = fCurrTime;
		bUpdateGraph = true;
	}

	CWeakRef<CAIObject> refObject = gAIEnv.pObjectContainer->GetWeakRef(m_aiObjectId);
	CAIObject *pObject = refObject.GetAIObject();
	assert(pObject);
	if (!pObject || !pObject->IsEnabled())
		return;

	const ColorB textCol(255,255,255,255);
	dc->Draw2dLabel(fColumnX, fColumnY, 1.5f, textCol, false, "Target Tracks for Agent \'%s\':", pObject->GetName());
	fColumnY += 20.0f;

	UpdateSortedTracks();

	TTargetTrackContainer::const_iterator itTrack = m_TargetTracks.begin();
	TTargetTrackContainer::const_iterator itTrackEnd = m_TargetTracks.end();
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		const CTargetTrack *pTrack = itTrack->second;
		assert(pTrack);

		// TODO Not returning right distance?
		const int iIndex = (int)std::distance(m_SortedTracks.begin(), std::find(m_SortedTracks.begin(), m_SortedTracks.end(), pTrack));
		pTrack->DebugDraw(dc, iIndex, fColumnX, fColumnY, pConfigProxy);
		fColumnY += 20.0f;

		if (bUpdateGraph)
		{
			CWeakRef<CAIObject> refTarget = gAIEnv.pObjectContainer->GetWeakRef(pTrack->GetAIObjectID());
			CAIObject *pTarget = refTarget.GetAIObject();
			assert(pTarget);

			const uint32 uDebugGraphIndex = pTrack->GetDebugGraphIndex();
			const uint32 uGraphX = uDebugGraphIndex % 3;
			const uint32 uGraphY = uDebugGraphIndex / 4;

			// Debug graph update
			IDebugHistory* pDebugHistory = m_pDebugHistoryManager->GetHistory(pTarget->GetName());
			if (pDebugHistory)
			{
				pDebugHistory->SetupLayoutAbs(fColumnGraphX+(fGraphWidth+fGraphMargin)*uGraphX, fColumnGraphY+(fGraphHeight+fGraphMargin)*uGraphY, fGraphWidth, fGraphHeight, fGraphMargin);
				pDebugHistory->AddValue(pTrack->GetTrackValue());
				pDebugHistory->SetVisibility(!bLastDraw);
			}
		}

		pTrack->SetLastDebugDrawTime(fCurrTime);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTargetTrackGroup::DebugDrawTargets(int nMode, int nTargetedCount, bool bExtraInfo)
{
	assert(nMode > 0);

	CDebugDrawContext dc;
	const ColorB visualColor(255,0,0,255);
	const ColorB memoryColor(255,255,0,255);
	const ColorB soundColor(0,255,0,255);
	const ColorB invalidColor(120,120,120,255);
	const float fProbableRatio = 0.45f;
	
	const float fCurrTime = GetAISystem()->GetFrameStartTime().GetSeconds();

	CWeakRef<CAIObject> refObject = gAIEnv.pObjectContainer->GetWeakRef(m_aiObjectId);
	CAIObject *pObject = refObject.GetAIObject();
	assert(pObject);
	if (!pObject || !pObject->IsEnabled())
		return;

	const Vec3& vPos = pObject->GetPos();

	UpdateSortedTracks();

	TSortedTracks::const_iterator itTrack = m_SortedTracks.begin();
	TSortedTracks::const_iterator itTrackEnd = m_SortedTracks.end();
	bool bFirst = true;
	for (; itTrack != itTrackEnd; ++itTrack)
	{
		const CTargetTrack *pTrack = *itTrack;
		assert(pTrack);

		ColorB drawColor;
		EAITargetType eType = pTrack->GetTargetType();
		EAITargetThreat eThreat = pTrack->GetTargetThreat();

		bool bThreatModified = false;
#ifdef TARGET_TRACK_DOTARGETTHREAT
		bThreatModified = ModifyTargetThreat(pTrack, eThreat);
#endif //TARGET_TRACK_DOTARGETTHREAT

		bool bIsBlinking = false;
		if (bThreatModified)
		{
			bIsBlinking = (int)(pTrack->GetLastDebugDrawTime() * 4.0f) % 2 == 0;
		}

		switch (eType)
		{
			case AITARGET_VISUAL:
			{
				drawColor = visualColor;
			}
			break;

			case AITARGET_MEMORY:
			{
				drawColor = memoryColor;
			}
			break;

			case AITARGET_SOUND:
			{
				drawColor = soundColor;
			}
			break;

			case AITARGET_NONE:
			{
				drawColor = invalidColor;
			}
			break;

			default:
				CRY_ASSERT_MESSAGE(0, "CTargetTrackGroup::DebugDrawTargets Unhandled target type");
				break;
		}
		if (!bFirst)
		{
			drawColor.r = uint8((float)drawColor.r * fProbableRatio);
			drawColor.g = uint8((float)drawColor.g * fProbableRatio);
			drawColor.b = uint8((float)drawColor.b * fProbableRatio);
			drawColor.a = uint8((float)drawColor.a * fProbableRatio);
		}

		const Vec3& vTargetPos = pTrack->GetTargetPos();
		const Vec3 vTargetToPos = (vPos-vTargetPos).GetNormalizedSafe();
		const Vec3 vTargetToPosRight = Vec3Constants<float>::fVec3_OneZ.Cross(vTargetToPos).GetNormalizedSafe();

		if (!bIsBlinking)
		{
			dc->DrawLine(vPos, drawColor, vTargetPos, drawColor, 2.0f);
			dc->DrawLine(vTargetPos, drawColor, vTargetPos+vTargetToPos+vTargetToPosRight*0.5f, drawColor, 2.0f);
			dc->DrawLine(vTargetPos, drawColor, vTargetPos+vTargetToPos-vTargetToPosRight*0.5f, drawColor, 2.0f);
		}

		if (bExtraInfo)
		{
			dc->Draw3dLabel(vTargetPos+(vPos-vTargetPos)*0.5f, 1.25f, "%.3f", pTrack->GetTrackValue());
		}

		pTrack->SetLastDebugDrawTime(fCurrTime);

		if (bFirst && nMode == 1)
			break;
		bFirst = false;
	}

	dc->Draw3dLabel(vPos + Vec3(0.0f,0.0f,1.0f), 1.5f, "%d", nTargetedCount);
}
#endif //TARGET_TRACK_DEBUG
