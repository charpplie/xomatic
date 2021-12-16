////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   EventNode.cpp
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "EventNode.h"
#include "AnimTrack.h"
#include "TrackEventTrack.h"

#include <ISystem.h>

//////////////////////////////////////////////////////////////////////////
CAnimEventNode::CAnimEventNode()
{
	SetFlags( GetFlags()|ANODE_FLAG_CAN_CHANGE_NAME );
	m_lastEventKey = -1;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEventNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_TRACKEVENT);
}

//////////////////////////////////////////////////////////////////////////
int CAnimEventNode::GetParamCount() const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEventNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex == 0)
	{
		info.flags = 0;
		info.name = "Track Event";
		info.paramId = APARAM_TRACKEVENT;
		info.valueType = AVALUE_TRACKEVENT;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEventNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const	
{
	if (paramId == APARAM_TRACKEVENT)
	{
		GetParamInfo( 0,info );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEventNode::Animate( SAnimContext &ec )
{
	// Get track event
	int trackCount = NumTracks();
	for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
	{
		int trackType = m_tracks[paramIndex].paramId;
		IAnimTrack *pTrack = m_tracks[paramIndex].track;

		// Check for fire
		if (CTrackEventTrack *pEventTrack = (CTrackEventTrack*)pTrack)
		{
			IEventKey key;
			int nEventKey = pEventTrack->GetActiveKey(ec.time,&key);
			if (nEventKey != m_lastEventKey && nEventKey >= 0)
			{
				ec.sequence->TriggerTrackEvent(key.event, key.eventValue);
			}
			m_lastEventKey = nEventKey;
		}
	}
}

void CAnimEventNode::Reset()
{
	m_lastEventKey = -1;
}
