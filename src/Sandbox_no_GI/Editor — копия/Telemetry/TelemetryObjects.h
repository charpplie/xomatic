////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryObjects.h
//  Version:     v1.00
//  Created:     25/12/2009 by Sergey Mikhtonyuk
//  Description: Structures for representing telemetry data
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_TELEMETRYOBJECTS_H__
#define		_TELEMETRYOBJECTS_H__

# pragma once

#include <ITelemetrySystem.h>

namespace Telemetry
{

static const uint32 INVALID_STATE_ID = 0xffffffff;

//////////////////////////////////////////////////////////////////////////
struct STelemetryNode;
class CTelemetryTimeline;
class CTelemetryRepository;
//////////////////////////////////////////////////////////////////////////

enum EStatOwner
{
	eStatOwner_AI,
	eStatOwner_Player,
	eStatOwner_Player_AITeam,

	eStatOwner_Num,
};

enum ETimelineFilterOperator
{
	eTFO_Any,						// 'not filtered'
	eTFO_Equals,
	eTFO_NotEquals,
	eTFO_LessThan,
	eTFO_GreaterThan,
	eTFO_LessThanOrEqual,
	eTFO_GreaterThanOrEqual,
};

struct STimelineSimpleFilter
{
	STimelineSimpleFilter() : m_operator(eTFO_Any) {}
	STimelineSimpleFilter(const string& operatorString)
	{
		if(operatorString == "==")
			m_operator = eTFO_Equals;
		else if(operatorString == "!=")
			m_operator = eTFO_NotEquals;
		else if(operatorString == "<")
			m_operator = eTFO_LessThan;
		else if(operatorString == ">")
			m_operator = eTFO_GreaterThan;
		else if(operatorString == "<=")
			m_operator = eTFO_LessThanOrEqual;
		else if(operatorString == ">=")
			m_operator = eTFO_GreaterThanOrEqual;
		else 
			m_operator = eTFO_Any;
	}

	ETimelineFilterOperator m_operator;
	string m_state;
	CAnyValue m_comparator;

	bool Filter(const CAnyValue& data)
	{
		bool filterOk = true;

		switch(m_operator)
		{
		case eTFO_Equals:
			filterOk = (m_comparator == data);
			break;

		case eTFO_NotEquals:
			filterOk = (m_comparator != data);
			break;

		case eTFO_LessThan:
		case eTFO_LessThanOrEqual:
		case eTFO_GreaterThan:
		case eTFO_GreaterThanOrEqual:
			break;

		case eTFO_Any:
			filterOk = true;
			break;

		default:
			assert(false);	
			filterOk = false;
			break;
		}

		return filterOk;
	}
};

//////////////////////////////////////////////////////////////////////////

struct STelemetryNode
{
	typedef std::map<string, string> TStates;

	STelemetryNode();
	STelemetryNode(const char* name);

	EStatOwner GetType() const { return m_type; }
	const char* GetName() const { return m_name.c_str(); }
	void SetName(const char* name);
	const TStates& GetStates() const { return m_states; }
	void AddState(const char* key, const char* value) { m_states.insert(std::make_pair<string, string>(key, value)); }

	void swap(STelemetryNode& other)
	{
		std::swap(m_type, other.m_type);
		std::swap(m_name, other.m_name);
		std::swap(m_states, other.m_states);
	}

private:

	EStatOwner m_type;
	CryStackStringT<char, 64> m_name;
	TStates m_states;
};

typedef boost::shared_ptr<STelemetryNode> TTelemNodePtr;

//////////////////////////////////////////////////////////////////////////

typedef std::vector<CTelemetryTimeline*> TTimelines;

class CTelemetryTimeline : public ITelemetryTimeline
{
public:
	CTelemetryTimeline();
	CTelemetryTimeline(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner);
	~CTelemetryTimeline();

	// ITelemetryTimeline
	virtual const char* GetName() const
	{
		return m_name.c_str();
	}
	virtual uint32 GetEventCount() const
	{
		return m_events.size();
	}
	virtual const STelemetryEvent* const* GetEvents() const
	{
		return &m_events[0];
	}
	virtual ITelemetryRepository* const GetRepository() const
	{
		return reinterpret_cast<ITelemetryRepository* const>(m_repo);
	}
	// ~ITelemetryTimeline

	void Init(CTelemetryRepository* repo, const char* name, TTelemNodePtr owner);

	void Swap(CTelemetryTimeline& other);

	void UpdateBBox();

	TTelemNodePtr GetOwner() const
	{
		return m_owner;
	}

	void SetOwner(TTelemNodePtr owner)
	{
		m_owner = owner;
	}

	void SetName(const char* name)
	{
		m_name = name;
	}

	const AABB& GetBBox() const
	{
		return m_bbox;
	}

	uint32 GetNumberOfStateDescriptions() const { return m_stateDescriptions.size(); }
	const char* GetStateDescription(uint32 i) const 
	{
		if(i < m_stateDescriptions.size())
			return m_stateDescriptions[i]; 

		return "";
	}
	uint32 AddOrGetState(const char* stateName);
	uint32 GetState(const char* stateName);
	void AppendEvent(STelemetryEvent& event);

	void Clear();

	const STelemetryEvent& GetEvent(size_t index) const 
	{ 
		return *m_events[index]; 
	}

	STelemetryEvent& GetEvent(size_t index)
	{ 
		return *m_events[index]; 
	}

	STelemetryEvent* GetNearestEventToTime(uint64 time, uint64 accuracy);
	STelemetryEvent* GetPreviousEvent(uint64 time);

	uint64 GetStartTime() const { return m_startTimeMs; }
	uint64 GetEndTime() const { return m_endTimeMs; }

	ColorB GetDebugColor() const { return m_debugColor; }
	void SetDebugColor(ColorB color) { m_debugColor = color; }

	bool IsPositionTrack() const { return m_bPositionTrack; }
	void SetPositionTrack(bool isPos) { m_bPositionTrack = isPos; }

	bool ShouldRender() const { return m_bShouldRender; }
	bool IsSelected() const { return m_bSelected; }
	void SetRendering(bool render, bool selected) { m_bShouldRender = render; m_bSelected = selected; }

	void Filter(STimelineSimpleFilter filter);
	bool IsFiltered() const { return (m_filter.m_operator != eTFO_Any); }

private:
	string m_name;
	AABB m_bbox;

	std::vector<string> m_stateDescriptions;
	std::vector<STelemetryEvent*> m_events;
	CTelemetryRepository* m_repo;
	TTelemNodePtr m_owner;

	uint64 m_startTimeMs;
	uint64 m_endTimeMs;

	ColorB m_debugColor;

	STimelineSimpleFilter m_filter;

	bool m_bPositionTrack;
	bool m_bShouldRender;
	bool m_bSelected;
};

}

//////////////////////////////////////////////////////////////////////////

namespace std
{
	inline void swap(Telemetry::STelemetryNode& _Left, Telemetry::STelemetryNode& _Right)
	{
		_Left.swap(_Right);
	}

	inline void swap(Telemetry::STelemetryEvent& _Left, Telemetry::STelemetryEvent& _Right)
	{
		_Left.swap(_Right);
	}

	inline void swap(Telemetry::CTelemetryTimeline& _Left, Telemetry::CTelemetryTimeline& _Right)
	{
		_Left.Swap(_Right);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#endif // __TELEMETRYOBJECTS_H__
