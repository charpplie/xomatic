/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#pragma once 

#ifndef GPUPROFILERXENON_H
#define GPUPROFILERXENON_H

#include "GPUProfiler.h"
#include "ProfilerSegment.h"

struct IRenderer;


//=============================================================================

class CGPUProfilerImpl: public CGPUProfiler
{
public:

	explicit CGPUProfilerImpl(LPDIRECT3DDEVICE9 device);
	~CGPUProfilerImpl();

	virtual void Enable();
	virtual void Disable();

	virtual void BeginFrame();
	virtual void EndFrame();

	virtual void SetBudgetInMilliseconds(float budget);

	virtual void OpenSegment(const char name[]);
	virtual void OpenSegmentWithBudget(const char name[], float value);
	virtual void OpenSegmentWithPercentageBudget(const char name[], float percentage);
	virtual void CloseSegment(const char name[]);

	virtual void GetStats(Stats* stats) const;

	virtual void Draw(IRenderer& renderer);

private:

	bool IsEnabled() const;

	void DrawBar(IRenderer& renderer, float x, float y);
	void DrawChildren(IRenderer& renderer);

	LPDIRECT3DDEVICE9 m_device;

	bool m_enabled;

	CProfilerSegment* m_segments;
	CProfilerSegment* m_currentSegment;

	ULONGLONG m_budgetInCycles;

	unsigned int m_currentLevel;
};

#endif

