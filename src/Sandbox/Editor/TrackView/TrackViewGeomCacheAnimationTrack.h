//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
//  Created: 6/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(USE_GEOM_CACHES)
#include "IMovieSystem.h"
#include "TrackViewTrack.h"

////////////////////////////////////////////////////////////////////////////
// This class represents a time range track of a geom cache node in TrackView 
////////////////////////////////////////////////////////////////////////////
class CTrackViewGeomCacheAnimationTrack : public CTrackViewTrack
{
public:
	CTrackViewGeomCacheAnimationTrack(IAnimTrack *pTrack, CTrackViewAnimNode *pTrackAnimNode, 
		CTrackViewNode *pParentNode, bool bIsSubTrack = false, unsigned int subTrackIndex = 0) 
		: CTrackViewTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex) {}

	virtual CTrackViewKeyHandle CreateKey(const float time);
};
#endif