////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryRepository.cpp
//  Version:     v1.00
//  Created:     22/12/2009 by Sergey Mikhtonyuk
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "TelemetryRepository.h"

//////////////////////////////////////////////////////////////////////////

namespace Telemetry
{

CTelemetryRepository::CTelemetryRepository()
: m_renderMode(eTLRM_Markers)
, m_startTimeMs(std::numeric_limits<uint64>::max())
, m_endTimeMs(0)
, m_startFilterTimeMs(0)
, m_currentTimeMs(0)
, m_endFilterTimeMs(std::numeric_limits<uint64>::max())
{
	ClearData(false);
}

//////////////////////////////////////////////////////////////////////////

CTelemetryRepository::~CTelemetryRepository()
{
}

//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline* CTelemetryRepository::AddTimeline(CTelemetryTimeline &toAdd)
{
	CTelemetryTimeline* ntl = new CTelemetryTimeline();
	ntl->Swap(toAdd);
	m_timelines.push_back(ntl);

	m_startTimeMs = min(m_startTimeMs, toAdd.GetStartTime());
	m_endTimeMs = max(m_endTimeMs, toAdd.GetEndTime());

	if(CONST_TEMP_STRING(ntl->GetName()) != "position")
	{
		ntl->SetPositionTrack(false);
	}
	else
	{
		ntl->SetPositionTrack(true);
	}

	return ntl;
}

uint CTelemetryRepository::GetTimelineCount() const
{
	return m_timelines.size();
}

CTelemetryTimeline* CTelemetryRepository::GetTimeline(uint index) const
{
	return m_timelines[index];
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryRepository::ClearData(bool clearEditorObjs)
{
	for(size_t i = 0, size = m_timelines.size(); i != size; ++i)
		delete m_timelines[i];
	m_timelines.clear();
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryRepository::Render()
{
	IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
	if(!pAux)
		return;

	ColorB white(255,255,255);
	ColorB col(255, 100, 100);
	ColorB selectedCol(255,255,0);

	for(TTimelines::const_iterator it = m_timelines.begin(), end = m_timelines.end(); it != end; ++it)
	{
		// todo: cull against view using path bbox?
		//	also use octree culling from CTelemetryEventTimeline
		CTelemetryTimeline* pTimeline = *it;
		if(!pTimeline->ShouldRender())
			continue;

		col = pTimeline->GetDebugColor();

		if(pTimeline->IsPositionTrack())
		{
			// find first event within the filtered time range
			uint32 evntIndex = 1;
			uint32 size = pTimeline->GetEventCount();
			for(evntIndex = 1; evntIndex != size; ++evntIndex)
			{
				if(pTimeline->GetEvent(evntIndex).timeMs >= m_startFilterTimeMs)
					break;
			}

			ColorB col = pTimeline->IsSelected() ? selectedCol : white;

			for(; evntIndex < size-1; ++evntIndex)
			{
				STelemetryEvent& nextEvent = pTimeline->GetEvent(evntIndex+1);
				if(nextEvent.timeMs > m_endFilterTimeMs)
					break;		

				pAux->DrawLine(pTimeline->GetEvent(evntIndex).position + TELEM_VERTICAL_ADJ, col, nextEvent.position + TELEM_VERTICAL_ADJ, col);
			}

			// also draw an arrow over the currently selected time
			STelemetryEvent* pEvent = pTimeline->GetPreviousEvent(m_currentTimeMs);
			if(pEvent)
			{
				pAux->DrawCone(pEvent->position + Vec3(0.0f, 0.0f, 2.0f), Vec3(0.0f,0.0f,-1.0f), 0.3f, 1.0f, col, true);
			}
		}
		else
		{
			// if a callback exists for this name, let it render the timeline
			if(stl::find_in_map(m_renderCallbacks, pTimeline->GetName(), NULL))
			{
				(m_renderCallbacks[pTimeline->GetName()])(pTimeline);
			}
			else
			{
				// default rendering

	 			for(size_t i = 0, size = pTimeline->GetEventCount(); i != size; ++i)
	 			{
					STelemetryEvent& evnt = pTimeline->GetEvent(i);

					if(evnt.timeMs >= m_startFilterTimeMs && evnt.timeMs < m_endFilterTimeMs && evnt.filterOk)
					{
	 					Vec3 pnt = evnt.position + TELEM_VERTICAL_ADJ;

						pAux->DrawSphere(pnt, 0.2f, pTimeline->IsSelected() ? selectedCol : col);
					}
				}
			}
		}
	}
}

void CTelemetryRepository::RegisterTimelineRenderer(const char* timelineName, TRenderCallback pCallback)
{
	m_renderCallbacks[timelineName] = pCallback;
}

}