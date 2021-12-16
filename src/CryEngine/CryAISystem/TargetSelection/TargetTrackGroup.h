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

#ifndef __TARGET_TRACK_GROUP_H__
#define __TARGET_TRACK_GROUP_H__

#include "TargetTrackCommon.h"

class CTargetTrack;
struct IDebugHistoryManager;

// Needed for CreateDummyPotentialTarget
#include "PerceptionHandler/IPerceptionHandler.h"

class CTargetTrackGroup
{
public:
	CTargetTrackGroup(TargetTrackHelpers::ITargetTrackPoolProxy *pTrackPoolProxy, tAIObjectID aiObjectId, uint32 uConfigHash, int nTargetLimit);
	~CTargetTrackGroup();

	tAIObjectID GetAIObjectID() const { return m_aiObjectId; }
	tAIObjectID GetLastBestTargetID() const { return m_aiLastBestTargetId; }
	uint32 GetConfigHash() const { return m_uConfigHash; }
	int GetTargetLimit() const { return m_nTargetLimit; }
	bool IsEnabled() const { return m_bEnabled; }
	void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }

	void Reset();
	void Serialize_Write(TSerialize ser);
	void Serialize_Read(TSerialize ser);

	void OnAgentRemoved(tAIObjectID aiTargetId);
	void Update(TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy);

	bool HandleStimulusEvent(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash);
	bool TriggerPulse(uint32 uStimulusNameHash, uint32 uPulseNameHash);
	bool GetDesiredTarget(TargetTrackHelpers::EDesiredTargetMethod eMethod, tAIObjectID &outTargetId, SAIPotentialTarget* &pOutTargetInfo);

	bool IsPotentialTarget(tAIObjectID aiTargetId) const;
	bool IsDesiredTarget(tAIObjectID aiTargetId) const;

#ifdef TARGET_TRACK_DEBUG
	// Debugging
	void DebugDrawTracks(TargetTrackHelpers::ITargetTrackConfigProxy *pConfigProxy, bool bLastDraw);
	void DebugDrawTargets(int nMode, int nTargetedCount, bool bExtraInfo = false);
#endif //TARGET_TRACK_DEBUG

private:
	CTargetTrackGroup(CTargetTrackGroup const&) {}
	CTargetTrackGroup& operator =(CTargetTrackGroup const&) { return *this; }

	void UpdateSortedTracks(bool bForced = false);
	CTargetTrack* GetBestTrack(TargetTrackHelpers::EDesiredTargetMethod eMethod);
	bool TestTrackAgainstFilters(CTargetTrack *pTrack, TargetTrackHelpers::EDesiredTargetMethod eMethod) const;

	// Stimulus handling helpers
	bool HandleStimulusEvent_All(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash);
	bool HandleStimulusEvent_Target(const TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, uint32 uStimulusNameHash, CTargetTrack *pTrack);

	// Returns the target track to be used for the given id
	CTargetTrack* GetTargetTrack(tAIObjectID aiTargetId);
	void DeleteTargetTracks();

	bool GetTargetInfo(tAIObjectID aiTargetId, EAITargetType &outType, EAITargetThreat &outThreat) const;
	void UpdateTargetRepresentation(const CTargetTrack *pBestTrack, tAIObjectID &outTargetId, SAIPotentialTarget* &pOutTargetInfo);

#ifdef TARGET_TRACK_DOTARGETTHREAT
	bool ModifyTargetThreat(const CTargetTrack *pTrack, EAITargetThreat &outThreat) const;
#endif //TARGET_TRACK_DOTARGETTHREAT

	typedef std::map<tAIObjectID, CTargetTrack*> TTargetTrackContainer;
	TTargetTrackContainer m_TargetTracks;
	
	typedef std::vector<CTargetTrack*> TSortedTracks;
	TSortedTracks m_SortedTracks;

	TargetTrackHelpers::ITargetTrackPoolProxy *m_pTrackPoolProxy;
	tAIObjectID m_aiObjectId;
	tAIObjectID m_aiLastBestTargetId;
	uint32 m_uConfigHash;
	int m_nTargetLimit;
	bool m_bNeedSort;
	bool m_bEnabled;

	SAIPotentialTarget m_dummyPotentialTarget;

#ifdef TARGET_TRACK_DEBUG
	// Debugging
	bool FindFreeGraphSlot(uint32 &outIndex) const;

	float m_fLastGraphUpdate;
	IDebugHistoryManager* m_pDebugHistoryManager;
	enum { DEBUG_GRAPH_OCCUPIED_SIZE = 16 };
	bool m_bDebugGraphOccupied[DEBUG_GRAPH_OCCUPIED_SIZE];
#endif //TARGET_TRACK_DEBUG
};

#endif //__TARGET_TRACK_GROUP_H__
