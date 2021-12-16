/*=============================================================================
Copyright (c) 2010 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Francesco Carucci

=============================================================================*/

#pragma once 

#ifndef PERF_COUNTERS_H
#define PERF_COUNTERS_H

























































typedef LPDIRECT3DDEVICE9 DEVICE_PTR;





class CPerfCounters
{
public:
	static void Enable(LPDIRECT3DDEVICE9 device) {}

	static float ConvertToMilliseconds(ULONGLONG cycles) { return cycles / (500.0f * 1000.0f); }
	static ULONGLONG ConvertToCycles(float milliseconds) { return (ULONGLONG) (milliseconds * (500.0f * 1000.0f)); }

	CPerfCounters(LPDIRECT3DDEVICE9 device) {}

	void Clear() {}

	void Start() {}
	void Stop() {}
	void GetValues() {}

	ULONGLONG GetGPUCycles() const { return 0; }

	float GetVertexCacheHitRatio() const { return 0.0f; }
	float GetTextureCacheHitRatio() const { return 0.0f; }

	DEVICE_PTR GetDevice() const { return 0; }
};



#endif