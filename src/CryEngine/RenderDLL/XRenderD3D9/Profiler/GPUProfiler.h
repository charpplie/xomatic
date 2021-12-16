/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#pragma once 

#ifndef GPUPROFILER_H
#define GPUPROFILER_H

struct IRenderer;

typedef LPDIRECT3DDEVICE9 DEVICE_PTR;





class CGPUProfiler
{
public:
	struct Stats
	{
		Stats();

		float GPUFrameTime;
	};

	static CGPUProfiler* Create(DEVICE_PTR device);

	virtual void Enable() = 0;
	virtual void Disable() = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void SetBudgetInMilliseconds(float budget) = 0;

	virtual void OpenSegment(const char name[]) = 0;
	virtual void OpenSegmentWithBudget(const char name[], float value) = 0;
	virtual void OpenSegmentWithPercentageBudget(const char name[], float percentage) = 0;	
	virtual void CloseSegment(const char name[]) = 0;

	virtual void GetStats(Stats* stats) const = 0;

	virtual void Draw(IRenderer& renderer) = 0;
};

#endif

