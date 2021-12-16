////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryObjects.cpp
//  Version:     v1.00
//  Created:     25/12/2009 by Sergey Mikhtonyuk
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "TelemetryObjects.h"
#include "TelemetryRepository.h"


namespace Telemetry
{

//////////////////////////////////////////////////////////////////////////
// CTelemetryNode
//////////////////////////////////////////////////////////////////////////

STelemetryNode::STelemetryNode()
: m_type(eStatOwner_Num)
{ }

//////////////////////////////////////////////////////////////////////////

STelemetryNode::STelemetryNode(const char* name)
{
	SetName(name);
}

//////////////////////////////////////////////////////////////////////////

void STelemetryNode::SetName(const char* name)
{
	m_name = name;

	if(m_name == "player")
		m_type = eStatOwner_Player;
	else if(m_name == "ai_actor" || m_name == "entity")
		m_type = eStatOwner_AI;
	else
		m_type = eStatOwner_Num;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CTelemetryTimeline
//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline::CTelemetryTimeline()
: m_repo(0)
, m_bPositionTrack(false)
, m_bShouldRender(true)
, m_bSelected(false)
, m_startTimeMs(std::numeric_limits<uint64>::max())
, m_endTimeMs(0)
{
	m_bbox.Reset();
	m_debugColor = ColorB(255,100,100);
}

//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline::CTelemetryTimeline(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner)
: m_name(name)
, m_owner(owner)
, m_repo(repo)
, m_bPositionTrack(false)
, m_bShouldRender(true)
, m_bSelected(false)
, m_startTimeMs(std::numeric_limits<uint64>::max())
, m_endTimeMs(0)
{
	m_bbox.Reset();
	m_debugColor = ColorB(255,100,100);
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::Init(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner)
{
	m_name = name;
	m_owner = owner;
	m_repo = repo;
	m_bbox.Reset();
}

//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline::~CTelemetryTimeline()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::Swap(CTelemetryTimeline &other)
{
	std::swap(m_name, other.m_name);
	std::swap(m_owner, other.m_owner);
	std::swap(m_bbox, other.m_bbox);
	std::swap(m_events, other.m_events);
	std::swap(m_repo, other.m_repo);
	std::swap(m_stateDescriptions, other.m_stateDescriptions);

	for(size_t i = 0, size = m_events.size(); i != size; ++i)
		m_events[i]->pTimeline = this;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::UpdateBBox()
{
	m_bbox.Reset();
	for(size_t i = 0, size = m_events.size(); i != size; ++i)
		m_bbox.Add(m_events[i]->position);
}

//////////////////////////////////////////////////////////////////////////

uint32 CTelemetryTimeline::AddOrGetState(const char* stateName)
{
	for(uint32 i=0; i<m_stateDescriptions.size(); ++i)
	{
		if(m_stateDescriptions[i] == stateName)
			return i;
	}

	m_stateDescriptions.push_back(stateName);
	return m_stateDescriptions.size()-1;
}

//////////////////////////////////////////////////////////////////////////

uint32 CTelemetryTimeline::GetState(const char* stateName)
{
	for(uint32 i=0; i<m_stateDescriptions.size(); ++i)
	{
		if(m_stateDescriptions[i] == stateName)
			return i;
	}

	return INVALID_STATE_ID;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::AppendEvent(STelemetryEvent& event)
{
	assert(m_repo);

	m_startTimeMs = min(event.timeMs, m_startTimeMs);
	m_endTimeMs = max(event.timeMs, m_endTimeMs);

	m_events.push_back(m_repo->newEvent(event));
	m_events.back()->pTimeline = this;
	m_bbox.Add(m_events.back()->position);
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::Clear() 
{
	assert(m_events.empty() || m_repo);

	if(!m_events.empty())
	{
		for(size_t i = 0, size = m_events.size(); i != size; ++i)
			m_repo->deleteEvent(m_events[i]);

		m_events.clear(); 
		m_bbox.Reset(); 
	}
}

//////////////////////////////////////////////////////////////////////////

STelemetryEvent* CTelemetryTimeline::GetNearestEventToTime(uint64 time, uint64 accuracy)
{
// 	if(time > m_endTimeMs || time < m_startTimeMs)
// 	{
// 		return NULL;
// 	}

	for(size_t i=0, size = m_events.size(); i != size; ++i)
	{
		int64 timeDiff = ((int64)time - m_events[i]->timeMs);
		
		if(abs(timeDiff) <= accuracy && m_events[i]->filterOk)
		{
			return m_events[i];
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

STelemetryEvent* CTelemetryTimeline::GetPreviousEvent(uint64 time)
{
	size_t i=0;
	size_t size = m_events.size();
	while(i < size && m_events[i]->timeMs <= time)
	{
		++i;
	}

	if(i == size)
		return NULL; // past the end of the timeline

	return m_events[i];
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::Filter(STimelineSimpleFilter filter)
{
	m_filter = filter;

	uint32 stateId = GetState(filter.m_state);

	for(size_t i=0, size = m_events.size(); i != size; ++i)
	{
		bool ok;
		if(stateId != INVALID_STATE_ID)
		{
			// filtering on a valid property
			ok = filter.Filter(m_events[i]->states[stateId]);
		}
		else
		{
			// filtering on an invalid property: usually means 'clear the filter'
			ok = true;
		}

		m_events[i]->filterOk = ok;
	}
}

}