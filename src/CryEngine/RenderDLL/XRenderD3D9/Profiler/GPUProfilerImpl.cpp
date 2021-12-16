/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#include "StdAfx.h"
#include "GPUProfilerImpl.h"
#include "PerfCounters.h"
#include "ValueBar.h"

#include <IRenderer.h>


//=============================================================================

CGPUProfilerImpl::CGPUProfilerImpl(LPDIRECT3DDEVICE9 device) 
	: m_device(device)
	, m_enabled(false)
	, m_budgetInCycles(0)
	, m_currentLevel(0)
{
	CPerfCounters::Enable(device);
		
	m_segments = new CProfilerSegment(device, "Frame Time", m_currentLevel);
	m_currentSegment = m_segments;
}

CGPUProfilerImpl::~CGPUProfilerImpl()
{
	delete m_segments;
}

void CGPUProfilerImpl::Enable()
{
	m_enabled = true;
}

void CGPUProfilerImpl::Disable()
{
	m_enabled = false;
}

void CGPUProfilerImpl::BeginFrame()
{
	m_segments->ClearRecordingStatusForChildren();
	if (!IsEnabled())
		return;
	m_segments->BeginRecording();
}

void CGPUProfilerImpl::EndFrame()
{
	if (!IsEnabled())
		return;
	m_segments->EndRecording();
}

void CGPUProfilerImpl::SetBudgetInMilliseconds(float budget)
{
	m_budgetInCycles = CPerfCounters::ConvertToCycles(budget);
}

void CGPUProfilerImpl::OpenSegment(const char name[])
{
	if (!IsEnabled())
		return;

	assert(m_currentSegment != 0);

	++m_currentLevel;

	m_currentSegment = m_currentSegment->AttachSegment(name, m_currentLevel);
	m_currentSegment->BeginRecording();
}

void CGPUProfilerImpl::OpenSegmentWithBudget(const char name[], float value)
{
	if (!IsEnabled())
		return;

	assert(m_currentSegment != 0);

	++m_currentLevel;

	m_currentSegment = m_currentSegment->AttachSegment(name, m_currentLevel);
	m_currentSegment->SetBudget(value);
	m_currentSegment->BeginRecording();
}

void CGPUProfilerImpl::OpenSegmentWithPercentageBudget(const char name[], float percentage)
{
	if (!IsEnabled())
		return;

	assert(m_currentSegment != 0);

	++m_currentLevel;

	m_currentSegment = m_currentSegment->AttachSegment(name, m_currentLevel);
	m_currentSegment->SetPercentageBudget(percentage);
	m_currentSegment->BeginRecording();
}

void CGPUProfilerImpl::CloseSegment(const char name[])
{
	if (!IsEnabled())
		return;

	assert(m_currentSegment != 0);
	assert(!strcmp(m_currentSegment->Name(), name));

	--m_currentLevel;

	m_currentSegment->EndRecording();
	m_currentSegment = m_currentSegment->GetParent();
}


void CGPUProfilerImpl::GetStats(Stats* stats) const
{
	stats->GPUFrameTime = CPerfCounters::ConvertToMilliseconds(m_segments->GetGPUCycles());
}

void CGPUProfilerImpl::Draw(IRenderer& renderer)
{




	static float x = 150;
	static float y = 30;

	m_segments->DrawLabelWithTime(renderer, (int) x, (int) y);
	DrawBar(renderer, x, y);
	DrawChildren(renderer);




}

bool CGPUProfilerImpl::IsEnabled() const
{
	return m_enabled;
}

void CGPUProfilerImpl::DrawBar(IRenderer& renderer, float x, float y)
{
	float budgetInMilliseconds = CPerfCounters::ConvertToMilliseconds(m_budgetInCycles);

	if (budgetInMilliseconds <= 0.0f)
	{
		budgetInMilliseconds = 33.33f;
	}

	static float offset_x = 170;
	static float offset_y = 0;

	static float height = 10.0f;

	CValueBar bar(renderer, 200.0f, height);
	bar.SetMax(budgetInMilliseconds);
	bar.SetTarget(budgetInMilliseconds);
	bar.SetValue(CPerfCounters::ConvertToMilliseconds(m_segments->GetGPUCycles()));
	bar.SetColor(0.0f, 0.80f, 0.0f);
	bar.SetBackground(0.20f, 0.20f, 0.20f);
	bar.Draw(x + offset_x, y + offset_y);
}

void CGPUProfilerImpl::DrawChildren( IRenderer& renderer )
{
	static int start_y = 40;

	int x = 140;
	int y = start_y;

	ULONGLONG totalCycles = 0;
	m_segments->AccumulateCounters(&totalCycles);
	m_segments->DrawChildren(renderer, 140, &x, &y, m_budgetInCycles > 0 ? m_budgetInCycles : m_segments->GetGPUCycles());
}
