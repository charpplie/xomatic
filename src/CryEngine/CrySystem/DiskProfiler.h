/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: DiskProfiler.h,v 1.0 2008/03/28 11:11:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for profiling disk IO
-------------------------------------------------------------------------
History:
- 28:3:2008   11:11 : Created by Anton Kaplanyan
*************************************************************************/


#ifndef __diskprofilesystem_h__
#define __diskprofilesystem_h__
#pragma once

#include <IDiskProfiler.h>
#include <CryThread.h>
#include <Cry_Color.h>



//////////////////////////////////////////////////////////////////////////
// Disk Profile main class
//////////////////////////////////////////////////////////////////////////
class CDiskProfiler : public IDiskProfiler
{
	friend class CDiskProfileTimer;
public:
	CDiskProfiler(ISystem* pSystem);
	VIRTUAL void Update();						// perframe update routine
	VIRTUAL DiskOperationInfo GetStatistics() const {
		return m_outStatistics;
	}
	VIRTUAL void RegisterStatistics(SDiskProfileStatistics* pStatistics);		// main statistics registering function

	VIRTUAL bool IsEnabled() const;

protected:

	// rendering routine
	void RenderBlock(const float timeStart, const float timeEnd, const ColorB threadColor, const ColorB IOTypeColor);
	void Render();

	typedef std::deque<SDiskProfileStatistics*> Statistics;
	typedef std::map<uint32, ColorB> ThreadColorMap;

	volatile bool				m_bEnabled;
	CryCriticalSection	m_csLock;				// MT-safe profiling
	Statistics					m_statistics;		// main statistics collector
	ThreadColorMap			m_threadsColorLegend;
	ISystem*						m_pSystem;

	int									m_nHeightOffset;	// offset from bottom of the screen, in pixels

	DiskOperationInfo m_outStatistics;
public:
	static int profile_disk;
	static int profile_disk_max_items;
	static float profile_disk_timeframe;		// max time for profiling timeframe
};

#endif // __diskprofilesystem_h__
