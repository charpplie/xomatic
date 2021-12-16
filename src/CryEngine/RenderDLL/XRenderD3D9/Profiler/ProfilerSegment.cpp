/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#include "StdAfx.h"
#include "ProfilerSegment.h"
#include "PerfCounters.h"
#include "ValueBar.h"

const float BAR_SIZE = 150;

//=============================================================================
// Fran: To be exposed as "skin"

static float offset_x = 250;
static float offset_y = 0;

static float height = 6.0f;

//=============================================================================

CProfilerSegment::CProfilerSegment(LPDIRECT3DDEVICE9 device, const char name[], unsigned int level, CProfilerSegment* parent)
	: m_level(level)
	, m_isRecording(false)
	, m_hasRecordedThisFrame(false)
	, m_parent(parent)
	, m_budgetValue(0.0f)
	, m_isBudgetInPercentage(false)
{
	strncpy(m_name, name, 127);
	m_counters = new CPerfCounters(device);
}

CProfilerSegment::~CProfilerSegment()
{
	delete m_counters;
}

void CProfilerSegment::ClearRecordingStatus()
{
	m_counters->Clear();
	m_hasRecordedThisFrame = false;
	ClearRecordingStatusForChildren();
}

void CProfilerSegment::ClearRecordingStatusForChildren()
{
	Segments::iterator pos = m_children.begin();
	Segments::iterator end = m_children.end();

	for (; pos != end; ++pos)
	{
		(*pos)->ClearRecordingStatus();
	}
}

void CProfilerSegment::BeginRecording()
{
	static int max_level = 99;
	if (m_level > max_level)
		return;

	if (m_isRecording)
		return;

	m_hasRecordedThisFrame = true;

	m_counters->Start();
	m_isRecording = true;
}

void CProfilerSegment::EndRecording()
{
	if (!m_isRecording)
		return;

	m_counters->Stop();
	m_isRecording = false;
}

// bool CProfilerSegment::HasRecordedThisFrame() const
// {
// 	return m_hasRecordedThisFrame;
// }

CProfilerSegment* CProfilerSegment::AttachSegment(const char name[], unsigned int level)
{
	Segments::iterator pos = m_children.begin();
	Segments::iterator end = m_children.end();

	for (; pos != end; ++pos)
	{
		if (!strcmp(name, (*pos)->Name()))
		{
			return *pos;
		}
	}

	CProfilerSegment* segment = new CProfilerSegment(m_counters->GetDevice(), name, level, this);
	m_children.push_back(segment);
	return segment;
}

void CProfilerSegment::SetBudget(float value)
{
	m_budgetValue = value;
	m_isBudgetInPercentage = false;
}

void CProfilerSegment::SetPercentageBudget(float percentage)
{
	m_budgetValue = percentage;
	m_isBudgetInPercentage = true;
}

CProfilerSegment* CProfilerSegment::GetParent() const
{
	return m_parent;
}

void CProfilerSegment::AccumulateCounters(ULONGLONG* totalCycles)
{
	*totalCycles += m_counters->GetGPUCycles();

	AccumulateCountersForChildren(totalCycles);
}

void CProfilerSegment::AccumulateCountersForChildren(ULONGLONG* totalCycles)
{
	Segments::iterator pos = m_children.begin();
	Segments::iterator end = m_children.end();

	for (; pos != end; ++pos)
	{
		(*pos)->AccumulateCounters(totalCycles);
	}
}

void CProfilerSegment::Draw(IRenderer& renderer, int base, int* x, int* y, ULONGLONG totalCycles)
{
	int originalX = *x;
	*x += 20;

	DrawElement(renderer, base, *x, *y, totalCycles);
	DrawChildren(renderer, base, x, y, totalCycles);

	*x = originalX;
	DrawUnaccountedFor(renderer, base, originalX + 20, y, totalCycles);
}

void CProfilerSegment::DrawElement(IRenderer &renderer, int base, int x, int y, ULONGLONG totalCycles)
{
	ULONGLONG cycles = m_counters->GetGPUCycles();

	float totalTime = totalCycles / (500.0f * 1000.0f);
	float percentage = (float) cycles / totalCycles;
	float budget = m_isBudgetInPercentage ? m_budgetValue : m_budgetValue / totalTime;

	DrawLabelWithTime(renderer, x, y);
	DrawBar(renderer, base, y, percentage, budget);
}

void CProfilerSegment::DrawChildren(IRenderer& renderer, int base, int* x, int* y, ULONGLONG totalCycles)
{
	Segments::iterator pos = m_children.begin();
	Segments::iterator end = m_children.end();

	for (; pos != end; ++pos)
	{ 
		CProfilerSegment& segment = *(*pos);
		float milliseconds = CPerfCounters::ConvertToMilliseconds(segment.GetGPUCycles());
		if (milliseconds < 0.01f)
			continue;

		*y += 12;
		segment.Draw(renderer, base, x, y, totalCycles);
	}
}

void CProfilerSegment::DrawLabelWithTime(IRenderer &renderer, int x, int y)
{
	float milliseconds = GetGPUCycles() / (500.0f * 1000.0f);

	char buffer[1024];
	sprintf(buffer, "%s %2.2fms", Name(), milliseconds);

	ColorF clr = ColorF(0, 0.80f - (m_level * 0.10f), 0, 1);
	renderer.Draw2dLabel((float) x, (float) y, 1.0f, &clr.r, false, buffer);

	sprintf(buffer, "Cache [V:%2.0f / T:%2.0f]", 100.0f * m_counters->GetVertexCacheHitRatio(), 100.0f * m_counters->GetTextureCacheHitRatio());
	renderer.Draw2dLabel((float) x + 800, (float) y, 1.0f, &clr.r, false, buffer);
}

ULONGLONG CProfilerSegment::GetGPUCycles() const
{
	return m_counters->GetGPUCycles();
}

const char* CProfilerSegment::Name() const
{
	return m_name;
}

void CProfilerSegment::DrawUnaccountedFor(IRenderer& renderer, int base, int x, int* y, ULONGLONG totalCycles)
{
	ULONGLONG childrenTotalCycles = 0;
	AccumulateCountersForChildren(&childrenTotalCycles);
	
	if (childrenTotalCycles == 0)
		return;
	if (childrenTotalCycles >= GetGPUCycles())
		return;

	*y += 12;

	float value = (float) (GetGPUCycles() - childrenTotalCycles) / totalCycles;
	float milliseconds = (GetGPUCycles() - childrenTotalCycles) / (500.0f * 1000.0f);

	char buffer[1024];
	sprintf(buffer, "N/A %2.2fms\n", milliseconds);

	ColorF clr = ColorF(0.20f, 0.20f, 0.20f,1);
	renderer.Draw2dLabel((float) x + 20, (float) *y, 1.0f, &clr.r, false, buffer);

	CValueBar bar(renderer, BAR_SIZE, height);	
	bar.SetMax(1.0f);
	bar.SetValue(value);
	bar.SetTarget(0.0f);
	bar.SetColor(0.20f, 0.20f, 0.20f);
	bar.Draw(base + offset_x, *y + offset_y);
}

void CProfilerSegment::DrawBar(IRenderer &renderer, int x, int y, float width, float budget)
{
	CValueBar bar(renderer, BAR_SIZE, height);	
	bar.SetMax(1.0f);
	bar.SetTarget(budget);
	bar.SetValue(width);
	bar.SetColor(0.0f, 0.80f - (m_level * 0.10f), 0.0f);
	bar.Draw(x + offset_x, y + offset_y);
}