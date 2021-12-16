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
#include "IGameStatistics.h"
#include "TelemetryObjects.h"
#include "PoolAllocator.h"
#include "Octree.h"

class CObjectLayer;
class CTelemetryPathObject;
class CTelemetryEventTimeline;

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

class CTelemetryRepository
{
public:
	typedef COctree<STelemetryEvent*, STelemOctreeAgent> TTelemOctree;

	CTelemetryRepository();
	~CTelemetryRepository();

	TTelemOctree* getOctree() { return &m_octree; }

	ETelemRenderMode getRenderMode() const { return m_renderMode; }
	void setRenderMode(ETelemRenderMode mode) { m_renderMode = mode; }

	CTelemetryPathObject* CreatePathObject(CTelemetryTimeline* timeline);
	void ClearData(bool clearEditorObjs);

	CTelemetryTimeline* AddTimeline(CTelemetryTimeline& toAdd);

	CObjectLayer* getTelemetryLayer();

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

private:
	CTelemetryRepository(const CTelemetryRepository& other);
	CTelemetryRepository& operator=(const CTelemetryRepository& rhs);

private:
	typedef std::vector<CTelemetryTimeline*> TTimelineContainer;
	typedef stl::TPoolAllocator<STelemetryEvent, stl::PSyncNone, 16> TEventAllocator;

	typedef std::vector<CTelemetryPathObject*> TViewPathContainer;

	TTelemOctree m_octree;
	TTimelineContainer m_timelines;
	TViewPathContainer m_viewPaths;
	CTelemetryEventTimeline* m_eventView;
	TEventAllocator m_eventAllocator;
	ETelemRenderMode m_renderMode;
	CObjectLayer* m_telemLayer;
};

//////////////////////////////////////////////////////////////////////////

#endif // _TELEMETRYREPOSITORY_H__
