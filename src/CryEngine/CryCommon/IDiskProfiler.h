/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: IDiskProfiler.h,v 1.0 2008/03/28 11:11:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for profiling disk IO
-------------------------------------------------------------------------
History:
- 28:3:2008   11:11 : Created by Anton Kaplanyan
*************************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(IDiskProfiler.h)

#ifndef __diskprofile_h__
#define __diskprofile_h__

#include <platform.h>
#include "ISystem.h"
#include <ITimer.h>

#if defined(ENABLE_PROFILERS)
	#define USE_DISK_PROFILER
#endif

class IDiskProfiler;

// Disk operations type
enum EDiskProfileOperationType
{
	edotRead					= 1<<0,
	edotWrite					= 1<<1,
	edotOpen					= 1<<2,
	edotClose					= 1<<3,
	edotSeek					= 1<<4,
	edotCompress			= 1<<5,
	edotDecompress		= 1<<6,
};

//////////////////////////////////////////////////////////////////////////
// Disk Profiler statistics
//////////////////////////////////////////////////////////////////////////

struct SDiskProfileStatistics
{
	float		m_beginIOTime;
	float		m_endIOTime;
	uint32	m_nIOType;					// EDiskProfileOperationType flags
	int			m_threadId;
	size_t	m_size;							// size of IO operation, in bytes
	string m_strFile;
	inline const bool operator < (const SDiskProfileStatistics& s) const 
	{
		return m_beginIOTime < s.m_beginIOTime;
	}
};



//////////////////////////////////////////////////////////////////////////
// Disk Profile interface class
//////////////////////////////////////////////////////////////////////////
UNIQUE_IFACE class IDiskProfiler
{
	friend class CDiskProfileTimer;
public:
	virtual void Update() = 0;
	virtual bool IsEnabled() const = 0;
	
	virtual DiskOperationInfo GetStatistics() const = 0;
	virtual void RegisterStatistics(SDiskProfileStatistics* pStatistics) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Disk Profile timer
//////////////////////////////////////////////////////////////////////////
class CDiskProfileTimer
{
public:
	CDiskProfileTimer( const uint32 nIOType, const size_t nIOSize, IDiskProfiler* pProfiler, const char * pFileName )
		: m_pStatistics(NULL)
	{
		if(pProfiler && pProfiler->IsEnabled())
		{
			m_pStatistics = new SDiskProfileStatistics;
			m_pStatistics->m_threadId = GetCurrentThreadId();
			m_pStatistics->m_beginIOTime = gEnv->pTimer->GetAsyncCurTime();
			m_pStatistics->m_endIOTime = -1.f;	// uninitialized
			m_pStatistics->m_nIOType = nIOType;
			m_pStatistics->m_size = nIOSize;
			m_pStatistics->m_strFile = pFileName;
			pProfiler->RegisterStatistics(m_pStatistics);
		}
	}

	~CDiskProfileTimer()
	{
		if(m_pStatistics)
			m_pStatistics->m_endIOTime = gEnv->pTimer->GetAsyncCurTime();
	}

protected:
	SDiskProfileStatistics* m_pStatistics;
};

#ifdef USE_DISK_PROFILER
#	define PROFILE_DISK(type, size, name)	CDiskProfileTimer _profile_disk_io_##type(type, size, gEnv->pSystem->GetIDiskProfiler(), name);
#else
#	define PROFILE_DISK(type, size, name)
#endif // USE_DISK_PROFILER

#define PROFILE_DISK_READ(size)	PROFILE_DISK(edotRead, size, 0)
#define PROFILE_DISK_SEEK_WITHNAME(name)	PROFILE_DISK(edotSeek, 0, name)
#define PROFILE_DISK_SEEK	PROFILE_DISK(edotSeek, 0, 0)
#define PROFILE_DISK_DECOMPRESS	PROFILE_DISK(edotDecompress, 0, 0)
#define PROFILE_DISK_WRITE	PROFILE_DISK(edotWrite, 0, 0)
#define PROFILE_DISK_COMPRESS	PROFILE_DISK(edotCompress, 0, 0)
#define PROFILE_DISK_OPEN	PROFILE_DISK(edotOpen, 0, 0)
#define PROFILE_DISK_CLOSE PROFILE_DISK(edotClose, 0, 0)
#endif // __diskprofile_h__
