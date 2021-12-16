////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryRepository.h
//  Version:     v1.00
//  Created:     22/12/2009 by Sergey Mikhtonyuk
//  Description: Storage class for all telemetry data
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_TELEMETRYREPOSITORY_H__
#define		_TELEMETRYREPOSITORY_H__

# pragma once

#include "CrySizer.h"
#include <ITelemetrySystem.h>

#include "TelemetryObjects.h"
#include "PoolAllocator.h"

class CObjectLayer;

namespace Telemetry
{

//////////////////////////////////////////////////////////////////////////

enum ETelemRenderMode
{
	eTLRM_Markers,
	eTLRM_Density,
};

#define TELEM_VERTICAL_ADJ Vec3(0, 0, 1)

//////////////////////////////////////////////////////////////////////////

struct STelemOctreeAgent
{
	Vec3 getPosition(STelemetryEvent* evnt) const
	{
		return evnt->position;
	}
};

//////////////////////////////////////////////////////////////////////////

class CTelemetryRepository : public ITelemetryRepository
{
public:
	CTelemetryRepository();
	~CTelemetryRepository();

	// ITelemetryRepository
	virtual void RegisterTimelineRenderer(const char* timelineName, TRenderCallback pCallback);

	virtual uint64 GetFilterTimeStart() const { return m_startFilterTimeMs; }
	virtual uint64 GetFilterTimeEnd() const { return m_endFilterTimeMs; }
	// ~ITelemetryRepository

	ETelemRenderMode getRenderMode() const { return m_renderMode; }
	void setRenderMode(ETelemRenderMode mode) { m_renderMode = mode; }

	void ClearData(bool clearEditorObjs);

	CTelemetryTimeline* AddTimeline(CTelemetryTimeline& toAdd);
	uint GetTimelineCount() const;
	CTelemetryTimeline* GetTimeline(uint index) const;

	STelemetryEvent* newEvent() { return new(m_eventAllocator.Allocate()) STelemetryEvent(); }
	STelemetryEvent* newEvent(STelemetryEvent& e) 
	{ 
		STelemetryEvent* ne = newEvent();
		std::swap(*ne, e);
		return ne; 
	}

	void deleteEvent(STelemetryEvent* e) 
	{ 
		if(e)
		{
			e->~STelemetryEvent();
			m_eventAllocator.Deallocate(e);
		} 
	}

	void Render();

	uint64 GetStartTime() const { return m_startTimeMs; }
	uint64 GetEndTime() const { return m_endTimeMs; }
	void SetFilteredTime(uint64 start, uint64 end) { m_startFilterTimeMs = start; m_endFilterTimeMs = end; }
	void SetCurrentSelectedTime(uint64 timeMs) { m_currentTimeMs = timeMs; }

private:
	CTelemetryRepository(const CTelemetryRepository& other);
	CTelemetryRepository& operator=(const CTelemetryRepository& rhs);

private:
	typedef stl::TPoolAllocator<STelemetryEvent, stl::PSyncNone, 16> TEventAllocator;

	// render callbacks
	std::map<string, TRenderCallback> m_renderCallbacks;

	// earliest and latest event times stored
	uint64 m_startTimeMs;
	uint64 m_endTimeMs;

	// currently selected filter time
	uint64 m_startFilterTimeMs;
	uint64 m_endFilterTimeMs;

	uint64 m_currentTimeMs;

	TTimelines m_timelines;
	TEventAllocator m_eventAllocator;
	ETelemRenderMode m_renderMode;
};

//////////////////////////////////////////////////////////////////////////
}

#endif // _TELEMETRYREPOSITORY_H__
