////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ThreadProfiler.h
//  Version:     v1.00
//  Created:     24/6/2003 by Timur
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ThreadProfiler_h__
#define __ThreadProfiler_h__
#pragma once

#include "platform.h"
#include "FrameProfiler.h"

#if defined(USE_FRAME_PROFILER) && defined(WIN32) && !defined(WIN64) || defined(XENON)
#define THREAD_SAMPLER
#endif

#ifdef THREAD_SAMPLER
//////////////////////////////////////////////////////////////////////////
//! the system which does the gathering of stats
class CThreadProfiler
{
public:
	CThreadProfiler();
	~CThreadProfiler();

	void Start();
	void Stop();
	void Render();

private:
	bool m_bStarted;
	std::vector<float> m_lastActive;
	int m_nDisplayedThreads;




	class TThreadSampler *m_pSampler;

};

#else // THREAD_SAMPLER

class CThreadProfiler
{
public:
	void Start() {};
	void Stop() {};
	void Render() {};
};

#endif

#endif // __ThreadProfiler_h__
