//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
//  Created: 9/8/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

class CTrackViewTrack;
class CTrackViewAnimNode;
class CTrackViewNode;

class CTrackViewAnimNodeFactory
{
public:
	CTrackViewAnimNode *BuildAnimNode(IAnimSequence *pSequence, IAnimNode *pAnimNode, CTrackViewNode *pParentNode);
};

class CTrackViewTrackFactory
{
public:
	CTrackViewTrack *BuildTrack(IAnimTrack *pTrack, CTrackViewAnimNode *pTrackAnimNode, 
		CTrackViewNode *pParentNode, bool bIsSubTrack = false, unsigned int subTrackIndex = 0);
};