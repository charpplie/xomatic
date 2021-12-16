/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Interface for the AI target track manager
  
 -------------------------------------------------------------------------
  History:
  - 02:25:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __ITARGET_TRACK_MANAGER_H__
#define __ITARGET_TRACK_MANAGER_H__

#include "IAISystem.h"

struct ITargetTrackManager
{
	virtual ~ITargetTrackManager() {}

	// Target class mods
	virtual bool SetTargetClassThreat(tAIObjectID aiObjectId, float fClassThreat) = 0;
	virtual float GetTargetClassThreat(tAIObjectID aiObjectId) const = 0;

	// Incoming stimulus handling
	virtual bool HandleStimulusEvent(tAIObjectID aiTargetId, const char* szStimulusName, const Vec3 &vPos, float fRadius) = 0;
	virtual bool HandleStimulusEvent(tAIObjectID aiObjectId, tAIObjectID aiTargetId, const char* szStimulusName, const Vec3 &vPos) = 0;
	virtual bool TriggerPulse(tAIObjectID aiObjectId, const char* szStimulusName, const char* szPulseName) = 0;
};

#endif //__ITARGET_TRACK_MANAGER_H__
