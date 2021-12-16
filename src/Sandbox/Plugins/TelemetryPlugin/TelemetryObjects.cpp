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

//////////////////////////////////////////////////////////////////////////
// CTelemetryNode
//////////////////////////////////////////////////////////////////////////

STelemetryNode::STelemetryNode()
: m_type(eStatOwner_Num)
{ }

//////////////////////////////////////////////////////////////////////////

STelemetryNode::STelemetryNode(const char* name)
{
	setName(name);
}

//////////////////////////////////////////////////////////////////////////

void STelemetryNode::setName(const char* name)
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
{
	m_bbox.Reset();
}

//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline::CTelemetryTimeline(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner)
: m_name(name)
, m_owner(owner)
, m_repo(repo)
{
	m_bbox.Reset();
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

	for(size_t i = 0, size = m_events.size(); i != size; ++i)
		m_events[i]->timeline = this;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::UpdateBBox()
{
	m_bbox.Reset();
	for(size_t i = 0, size = m_events.size(); i != size; ++i)
		m_bbox.Add(m_events[i]->position);
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryTimeline::AppendEvent(STelemetryEvent& event)
{
	assert(m_repo);
	m_events.push_back(m_repo->newEvent(event));
	m_events.back()->timeline = this;
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

