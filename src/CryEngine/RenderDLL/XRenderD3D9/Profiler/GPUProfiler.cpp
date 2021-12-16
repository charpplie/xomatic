/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#include "StdAfx.h"
#include "GPUProfiler.h"
#include "GPUProfilerImpl.h"

class CNullProfiler: public CGPUProfiler
{
public:
	virtual void Enable() {}
	virtual void Disable() {}

	virtual void BeginFrame() {}
	virtual void EndFrame() {}

	virtual void SetBudgetInMilliseconds(float budget) {}

	virtual void OpenSegment(const char name[]) {}
	virtual void OpenSegmentWithBudget(const char name[], float value) {}
	virtual void OpenSegmentWithPercentageBudget(const char name[], float percentage) {}
	virtual void CloseSegment(const char name[]) {}

	virtual void GetStats(Stats* stats) const {}

	virtual void Draw(IRenderer& renderer) {}
};

CGPUProfiler* CGPUProfiler::Create(DEVICE_PTR device)
{
#ifdef ENABLE_GPU_TIMERS
	return new CGPUProfilerImpl(device);
#else
	return new CNullProfiler();
#endif
}

CGPUProfiler::Stats::Stats()
	: GPUFrameTime(0.0f)
{
}