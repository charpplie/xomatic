////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TimelinePreprocessor.h
//  Version:     v1.00
//  Created:     25/12/2009 by Sergey Mikhtonyuk
//  Description: Preprocessor for incomming telemetry data
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_TIMELINEPREPROCESSOR_H__
#define		_TIMELINEPREPROCESSOR_H__

# pragma once

#include "TelemetryObjects.h"

namespace Telemetry
{

class CTimelinePreprocessor
{
public:
	void DeduceEventPositions(CTelemetryTimeline** timelines, size_t numTimelines, size_t pathTimeline);

};

}

#endif // __TIMELINEPREPROCESSOR_H__
