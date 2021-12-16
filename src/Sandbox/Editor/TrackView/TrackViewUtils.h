////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2009
// -------------------------------------------------------------------------
//  File name:   TrackViewUtils.h
//  Created:     7/4/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TrackViewUtils_h__
#define __TrackViewUtils_h__
#pragma once

#if _MSC_VER > 1000
#pragma once
#endif

#include <IMovieSystem.h>

//////////////////////////////////////////////////////////////////////////
class CTrackViewUtils
{
public:
	struct SelectedKey
	{
		IAnimNode  *pNode;
		IAnimTrack *pTrack;
		int         nKey;
	};
	struct SelectedKeys
	{
		bool bAllOfSameType;
		EAnimTrackType trackType; // If all selected keys are of the same track type, this member will be filled
		std::vector<SelectedKey> keys;
	};

	struct SelectedTrack
	{
		IAnimNode  *pNode;
		IAnimTrack *pTrack;
		int m_nSubTrackIndex;
	};
	struct SelectedTracks
	{
		bool bAllOfSameType;
		bool bHasRotationTrack;
		EAnimTrackType trackType; // If all selected keys are of the same track type, this member will be filled
		std::vector<SelectedTrack> tracks;
	};

	// Return array of selected keys from the given sequence.
	static int GetSelectedKeys( IAnimSequence *pSequence,SelectedKeys &selectedKeys );	
	static int GetAllKeys( IAnimSequence *pSequence,SelectedKeys &selectedKeys );
	static int GetSelectedTracks( IAnimSequence *pSequence,SelectedTracks &selectedTracks );
	// Check whether only one track or subtracks belong to one same track is/are selected.
	static bool IsOneTrackSelected(const SelectedTracks& selectedTracks);
	// Return array of tracks of a type belong to a group node.
	static int GetTracksInGroup( IAnimSequence *pSequence, IAnimNode *pGroupNode, AnimParamType paramType, SelectedTracks &tracks );
	static int GetKeysInTimeRange( IAnimSequence *pSequence,SelectedKeys &selectedKeys, float t0, float t1 );
};


//////////////////////////////////////////////////////////////////////////
struct ITrackViewEventsListener
{
	// Called when Key selection changes.
	virtual void OnKeySelectionChange() = 0;
};


#endif //__TrackViewUtils_h__
