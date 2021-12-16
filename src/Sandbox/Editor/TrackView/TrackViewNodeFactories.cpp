//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
//  Created: 9/8/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackViewNodeFactories.h"
#include "TrackViewAnimNode.h"
#include "TrackViewCameraNode.h"
#include "TrackViewTrack.h"
#include "TrackViewGeomCacheAnimationTrack.h"

CTrackViewAnimNode *CTrackViewAnimNodeFactory::BuildAnimNode(IAnimSequence *pSequence, IAnimNode *pAnimNode, CTrackViewNode *pParentNode)
{
	if (pAnimNode->GetType() == eAnimNodeType_Camera)
	{
		return new CTrackViewCameraNode(pSequence, pAnimNode, pParentNode);
	}

	return new CTrackViewAnimNode(pSequence, pAnimNode, pParentNode);
}

CTrackViewTrack *CTrackViewTrackFactory::BuildTrack(IAnimTrack *pTrack, CTrackViewAnimNode *pTrackAnimNode, 
	CTrackViewNode *pParentNode, bool bIsSubTrack, unsigned int subTrackIndex)
{
#if defined(USE_GEOM_CACHES)
	if (pTrack->GetParameterType() == eAnimParamType_TimeRanges && pTrackAnimNode->GetType() == eAnimNodeType_GeomCache)
	{
		return new CTrackViewGeomCacheAnimationTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
	}
#endif

	return new CTrackViewTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
}