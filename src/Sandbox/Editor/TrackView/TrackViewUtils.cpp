////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2009
// -------------------------------------------------------------------------
//  File name:   TrackViewUtils.cpp
//  Created:     7/4/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackViewUtils.h"
#include <ISplines.h>

//////////////////////////////////////////////////////////////////////////
static int GetKeys( IAnimSequence *pSequence,CTrackViewUtils::SelectedKeys &selectedKeys,bool selectedOnly,float t0=0,float t1=0 )
{
	selectedKeys.bAllOfSameType = true;
	selectedKeys.trackType = (EAnimTrackType)-1;

	if (pSequence == NULL)
	{
		assert("(pSequence == NULL) in CTrackViewUtils::GetSelectedKeys()" && false);
		return 0;
	}

	int trackType = -1;
	bool noTimeRange = !(t0 < t1);

	int nNodeCount = pSequence->GetNodeCount();
	for (int node = 0; node < nNodeCount; node++)
	{
		IAnimNode *pAnimNode = pSequence->GetNode(node);

		int nTrackCount = pAnimNode->GetTrackCount();
		for (int track = 0; track < nTrackCount; track++)
		{
			IAnimTrack *pAnimTrack = pAnimNode->GetTrackByIndex(track);
			if (pAnimTrack->GetSubTrackCount() == 0)
			{
				int nKeyCount = pAnimTrack->GetNumKeys();
				for (int nkey = 0; nkey < nKeyCount; nkey++)
				{
					float keyTime = pAnimTrack->GetKeyTime(nkey);
					bool timeRangeOk = noTimeRange || (t0 <= keyTime && keyTime <= t1);
					if ((!selectedOnly || pAnimTrack->IsKeySelected(nkey))
						&& timeRangeOk)
					{
						CTrackViewUtils::SelectedKey key;
						key.pNode = pAnimNode;
						key.pTrack = pAnimTrack;
						key.nKey = nkey;
						selectedKeys.keys.push_back( key );

						if (trackType == -1)
							trackType = pAnimTrack->GetType();
						else if (trackType != pAnimTrack->GetType())
						{
							selectedKeys.bAllOfSameType = false;
						}
					}
				}
			}

			int nSubTrackCount = pAnimTrack->GetSubTrackCount();
			for (int subtrack = 0; subtrack < nSubTrackCount; subtrack++)
			{
				IAnimTrack *pSubAnimTrack = pAnimTrack->GetSubTrack(subtrack);

				int nKeyCount = pSubAnimTrack->GetNumKeys();
				for (int nkey = 0; nkey < nKeyCount; nkey++)
				{
					float keyTime = pSubAnimTrack->GetKeyTime(nkey);
					bool timeRangeOk = noTimeRange || (t0 <= keyTime && keyTime <= t1);
					if ((!selectedOnly || pSubAnimTrack->IsKeySelected(nkey))
						&& timeRangeOk)
					{
						CTrackViewUtils::SelectedKey key;
						key.pNode = pAnimNode;
						key.pTrack = pSubAnimTrack;
						key.nKey = nkey;
						selectedKeys.keys.push_back( key );

						if (trackType == -1)
							trackType = pSubAnimTrack->GetType();
						else if (trackType != pSubAnimTrack->GetType())
						{
							selectedKeys.bAllOfSameType = false;
						}
					}
				}
			}
		}
	}

	if (selectedKeys.keys.size() == 0)
		selectedKeys.bAllOfSameType = false;

	if (selectedKeys.bAllOfSameType)
	{
		selectedKeys.trackType = (EAnimTrackType)trackType;
	}

	return (int)selectedKeys.keys.size();
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewUtils::GetSelectedKeys( IAnimSequence *pSequence,SelectedKeys &selectedKeys )
{
	return GetKeys(pSequence, selectedKeys, true);
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewUtils::GetAllKeys( IAnimSequence *pSequence,SelectedKeys &selectedKeys )
{
	return GetKeys(pSequence, selectedKeys, false);
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewUtils::GetSelectedTracks( IAnimSequence *pSequence,SelectedTracks &selectedTracks )
{
	selectedTracks.bAllOfSameType = true;
	selectedTracks.bHasRotationTrack = false;
	selectedTracks.trackType = (EAnimTrackType)-1;

	int trackType = -1;

	int nNodeCount = pSequence ? pSequence->GetNodeCount() : 0;
	for (int node = 0; node < nNodeCount; node++)
	{
		IAnimNode *pAnimNode = pSequence->GetNode(node);

		int nTrackCount = pAnimNode->GetTrackCount();
		for (int track = 0; track < nTrackCount; track++)
		{
			IAnimTrack *pAnimTrack = pAnimNode->GetTrackByIndex(track);

			if (pAnimTrack->GetFlags() & IAnimTrack::ATRACK_SELECTED)
			{
				SelectedTrack t;
				t.pNode = pAnimNode;
				t.pTrack = pAnimTrack;
				t.m_nSubTrackIndex = -1;
				selectedTracks.tracks.push_back( t );

				if (trackType == -1)
					trackType = pAnimTrack->GetType();
				else if (trackType != pAnimTrack->GetType())
				{
					selectedTracks.bAllOfSameType = false;
				}
			}
			int nSubTrackCount = pAnimTrack->GetSubTrackCount();
			for (int subtrack = 0; subtrack < nSubTrackCount; subtrack++)
			{
				IAnimTrack *pSubAnimTrack = pAnimTrack->GetSubTrack(subtrack);

				if (pSubAnimTrack->GetFlags() & IAnimTrack::ATRACK_SELECTED)
				{
					SelectedTrack t;
					t.pNode = pAnimNode;
					t.pTrack = pSubAnimTrack;
					t.m_nSubTrackIndex = subtrack;
					selectedTracks.tracks.push_back( t );

					if(pAnimTrack->GetType() == ATRACK_EULER_XYZ)
						selectedTracks.bHasRotationTrack = true;

					if (trackType == -1)
						trackType = pSubAnimTrack->GetType();
					else if (trackType != pSubAnimTrack->GetType())
					{
						selectedTracks.bAllOfSameType = false;
					}
				}
			}
		}
	}

	if (selectedTracks.tracks.size() == 0)
		selectedTracks.bAllOfSameType = false;

	if (selectedTracks.bAllOfSameType)
	{
		selectedTracks.trackType = (EAnimTrackType)trackType;
	}

	return (int)selectedTracks.tracks.size();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewUtils::IsOneTrackSelected(const SelectedTracks& selectedTracks)
{
	if(selectedTracks.tracks.size() == 0)
		return false;

	if(selectedTracks.tracks.size() == 1)
		return true;

	if(selectedTracks.bAllOfSameType == false)
		return false;

	int baseIndex = -1;
	for(int i=0; i<(int)selectedTracks.tracks.size(); ++i)
	{
		// It's not a subtrack.
		if(selectedTracks.tracks[i].m_nSubTrackIndex < 0)
			return false;
		if(i==0)
			baseIndex = selectedTracks.tracks[i].pTrack->GetParameterType()
									- selectedTracks.tracks[i].m_nSubTrackIndex;
		else
		{
			// It's not a subtrack belong to a same parent track.
			if(baseIndex != 
											selectedTracks.tracks[i].pTrack->GetParameterType()
											- selectedTracks.tracks[i].m_nSubTrackIndex)
				return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewUtils::GetTracksInGroup( IAnimSequence *pSequence, IAnimNode *pGroupNode, 
																				AnimParamType paramType, SelectedTracks &result )
{
	result.bAllOfSameType = true;
	result.bHasRotationTrack = false;				// Don't care in this case.
	result.trackType = (EAnimTrackType)-1;	// Don't care in this case.

	int nNodeCount = pSequence ? pSequence->GetNodeCount() : 0;
	for (int node = 0; node < nNodeCount; node++)
	{
		IAnimNode *pAnimNode = pSequence->GetNode(node);

		if (pAnimNode->GetParent() != pGroupNode)
			continue;

		int nTrackCount = pAnimNode->GetTrackCount();
		for (int track = 0; track < nTrackCount; track++)
		{
			IAnimTrack *pAnimTrack = pAnimNode->GetTrackByIndex(track);

			if (pAnimTrack->GetParameterType() == paramType)
			{
				SelectedTrack t;
				t.pNode = pAnimNode;
				t.pTrack = pAnimTrack;
				t.m_nSubTrackIndex = -1;
				result.tracks.push_back( t );
			}
			int nSubTrackCount = pAnimTrack->GetSubTrackCount();
			for (int subtrack = 0; subtrack < nSubTrackCount; subtrack++)
			{
				IAnimTrack *pSubAnimTrack = pAnimTrack->GetSubTrack(subtrack);

				if (pSubAnimTrack->GetParameterType() == paramType)
				{
					SelectedTrack t;
					t.pNode = pAnimNode;
					t.pTrack = pSubAnimTrack;
					t.m_nSubTrackIndex = subtrack;
					result.tracks.push_back( t );
				}
			}
		}
	}

	return (int)result.tracks.size();
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewUtils::GetKeysInTimeRange( IAnimSequence *pSequence,SelectedKeys &selectedKeys, float t0, float t1 )
{
	return GetKeys(pSequence, selectedKeys, false, t0, t1);
}