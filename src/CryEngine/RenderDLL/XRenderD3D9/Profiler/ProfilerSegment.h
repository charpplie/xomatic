/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#pragma once 

#ifndef PROFILER_SEGMENT_H
#define PROFILER_SEGMENT_H

#include "GPUProfiler.h"

struct IRenderer;

class CPerfCounters;

//=============================================================================

class CProfilerSegment
{
public:
	CProfilerSegment(LPDIRECT3DDEVICE9 device, const char name[], unsigned int level,CProfilerSegment* parent = 0);
	~CProfilerSegment();

	void ClearRecordingStatus();
	void ClearRecordingStatusForChildren();

	void BeginRecording();
	void EndRecording();

	CProfilerSegment* AttachSegment(const char name[], unsigned int level);
	CProfilerSegment* GetParent() const;

	void SetBudget(float value);
	void SetPercentageBudget(float percentage);

	void AccumulateCounters(ULONGLONG* totalCycles);
	void AccumulateCountersForChildren( ULONGLONG* totalCycles );
	void Draw(IRenderer& renderer, int base, int* x, int* y, ULONGLONG totalCycles);

	void DrawElement(IRenderer& renderer, int base, int x, int y, ULONGLONG totalCycles);
	void DrawChildren(IRenderer& renderer, int base, int* x, int* y, ULONGLONG totalCycles);

	void DrawLabelWithTime(IRenderer &renderer, int x, int y);

	ULONGLONG GetGPUCycles() const;

	const char* Name() const;

private:
	char m_name[128];

	CPerfCounters* m_counters;
	CProfilerSegment* m_parent;

	unsigned int m_level;

	typedef std::list<CProfilerSegment*> Segments;
	Segments m_children;

	float m_budgetValue;
	bool m_isBudgetInPercentage;

	bool m_isRecording;
	bool m_hasRecordedThisFrame;

	void DrawUnaccountedFor(IRenderer& renderer, int base, int x, int* y, ULONGLONG totalCycles);

	void DrawBar(IRenderer &renderer, int x, int y, float milliseconds, float budget);
};

#endif