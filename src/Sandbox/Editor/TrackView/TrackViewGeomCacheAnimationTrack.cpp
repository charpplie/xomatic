//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
//  Created: 9/8/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#if defined(USE_GEOM_CACHES)
#include "TrackViewGeomCacheAnimationTrack.h"
#include "TrackViewSequence.h"

#include <IGeomCache.h>

CTrackViewKeyHandle CTrackViewGeomCacheAnimationTrack::CreateKey(const float time)
{
	CTrackViewSequence *pSequence = GetSequence();
	CTrackViewSequenceNotificationContext context(GetSequence());

	CTrackViewKeyHandle keyHandle = CTrackViewTrack::CreateKey(time);	

	// Find editor object who owns this node.
	IEntity *pEntity = keyHandle.GetTrack()->GetAnimNode()->GetEntity();
	if (pEntity)
	{
		const IGeomCacheRenderNode *pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
		if (pGeomCacheRenderNode)
		{
			const IGeomCache *pGeomCache = pGeomCacheRenderNode->GetGeomCache();

			if (pGeomCache)
			{
				ITimeRangeKey timeRangeKey;
				keyHandle.GetKey(&timeRangeKey);
				timeRangeKey.m_duration = pGeomCache->GetDuration();
				timeRangeKey.m_endTime = timeRangeKey.m_duration;
				keyHandle.SetKey(&timeRangeKey);
				GetSequence()->OnKeysChanged();
			}
		}
	}	

	return keyHandle;
}
#endif