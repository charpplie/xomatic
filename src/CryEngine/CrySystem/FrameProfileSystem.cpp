////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   FrameProfileSystem.cpp
//  Version:     v1.00
//  Created:     24/6/2003 by Timur,Sergey,Wouter.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FrameProfileSystem.h"

#include <ILog.h>
#include <IRenderer.h>
#include <IInput.h>
#include <StlUtils.h>
#include <IConsole.h>
#include <IHardwareMouse.h>
#include <IThreadTask.h>
#include <IGame.h>
#include <IGameFramework.h>

#include "Sampler.h"
#include "CryThread.h"
#include "Timer.h"

#if defined(LINUX)
#include "platform.h"
#endif







#ifdef WIN32
#include <psapi.h>
static HMODULE hPsapiModule;
typedef BOOL (WINAPI *FUNC_GetProcessMemoryInfo)( HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD );
static FUNC_GetProcessMemoryInfo pfGetProcessMemoryInfo;
static bool m_bNoPsapiDll;
#endif

extern int CryMemoryGetAllocatedSize();

#ifdef USE_FRAME_PROFILER

#define MAX_PEAK_PROFILERS 30
#define MAX_ABSOLUTEPEAK_PROFILERS 100
//! Peak tolerance in milliseconds.
#define PEAK_TOLERANCE 10.0f

//////////////////////////////////////////////////////////////////////////
// CFrameProfilerTimer static variable.
//////////////////////////////////////////////////////////////////////////
CFrameProfileSystem* CFrameProfileSystem::s_pFrameProfileSystem = 0;
bool CFrameProfilerTimer::g_bThreadSafe = false;
int64 CFrameProfilerTimer::g_nTicksPerSecond = 0;
double CFrameProfilerTimer::g_fSecondsPerTick = 0.0;
CryCriticalSection CFrameProfileSystem::m_staticProfilersLock;

//////////////////////////////////////////////////////////////////////////
// CFrameProfilerTimer implementation.
//////////////////////////////////////////////////////////////////////////
void CFrameProfilerTimer::Init( bool bThreadSafe ) // called once
{
	if (g_fSecondsPerTick != 0.f && g_bThreadSafe == bThreadSafe)
		return;

	g_bThreadSafe = bThreadSafe;

#if defined(WIN32) || defined(WIN64)
	if (g_bThreadSafe)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&g_nTicksPerSecond);
	}
	else
	{
		HKEY hKey;
		unsigned nMhz = 0;
		DWORD dwSize = sizeof(nMhz);
		if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey)
			&&ERROR_SUCCESS == RegQueryValueEx (hKey, "~MHz", NULL, NULL, (LPBYTE)&nMhz, &dwSize))
		{
			g_nTicksPerSecond = nMhz * 1000000;
		}
	}
#else
  QueryPerformanceFrequency((LARGE_INTEGER*)&g_nTicksPerSecond);
#endif

	g_fSecondsPerTick = 1.0 / (double)g_nTicksPerSecond;
}

int64 CFrameProfilerTimer::GetTicks()
{
#if defined(WIN32) || defined(WIN64) || defined(XENON)
	if (g_bThreadSafe)
	{
		int64 nTicks;
		QueryPerformanceCounter((LARGE_INTEGER*)&nTicks);
		return nTicks;
	}
#endif
	return CryGetTicks();
}

//////////////////////////////////////////////////////////////////////////
// FrameProfilerSystem Implementation.
//////////////////////////////////////////////////////////////////////////

int CFrameProfileSystem::profile_callstack = 0;
int CFrameProfileSystem::profile_log = 0;
uint32 CFrameProfileSystem::s_nFilterThreadId = 0;

//////////////////////////////////////////////////////////////////////////
CFrameProfileSystem::CFrameProfileSystem() 
: m_nCurSample(-1), m_ProfilerThreads(GetCurrentThreadId())
{
	s_pFrameProfileSystem = this;
#ifdef WIN32
	hPsapiModule = NULL;
	pfGetProcessMemoryInfo = NULL;
	m_bNoPsapiDll = false;
#endif

	s_nFilterThreadId = 0;

	// Allocate space for 1024 profilers.
	m_profilers.reserve( 1024 );
	m_pCurrentCustomSection = 0;
	m_bEnabled = false;
	m_totalProfileTime = 0;
	m_frameStartTime = 0;
	m_frameTime = 0;
	m_frameLostTime = 0;
	m_callOverheadTime = 0;
	m_pRenderer = 0;
	m_displayQuantity = SELF_TIME;

	m_bCollect = false;
	m_nThreadSupport = 0;
	m_bDisplay = false;
	m_bDisplayMemoryInfo = false;
	m_bLogMemoryInfo = false;

	m_peakTolerance = PEAK_TOLERANCE;

	m_pGraphProfiler = 0;








	m_timeGraphCurrentPos = 0;
	m_bCollectionPaused = false;
	m_bDrawGraph = false;

	m_selectedRow = -1;
	
	m_bEnableHistograms = false;
	m_histogramsMaxPos = 200;
	m_histogramsHeight = 16;
	m_histogramsCurrPos = 0;

	m_bSubsystemFilterEnabled = false;
	m_subsystemFilter = PROFILE_RENDERER;
	m_maxProfileCount = 999;
	m_histogramScale = 100;

	m_bDisplayedProfilersValid = false;
	m_bNetworkProfiling = false;
	m_bMemoryProfiling = true;
	//m_pProfilers = &m_netTrafficProfilers;
	m_pProfilers = &m_profilers;

	m_nLastPageFaultCount = 0;
	m_nPagesFaultsLastFrame = 0;
	m_bPageFaultsGraph = false;
	m_nPagesFaultsPerSec = 0;

	m_pSampler = new CSampler;

	//////////////////////////////////////////////////////////////////////////
	// Initialize subsystems list.
	memset( m_subsystems,0,sizeof(m_subsystems) );
	m_subsystems[PROFILE_RENDERER].name = "Renderer";
	m_subsystems[PROFILE_3DENGINE].name = "3DEngine";
	m_subsystems[PROFILE_PARTICLE].name = "Particle";
	m_subsystems[PROFILE_ANIMATION].name = "Animation";
	m_subsystems[PROFILE_AI].name = "AI";
	m_subsystems[PROFILE_ENTITY].name = "Entity";
	m_subsystems[PROFILE_PHYSICS].name = "Physics";
	m_subsystems[PROFILE_SOUND].name = "Sound";
	m_subsystems[PROFILE_MUSIC].name = "Music";
	m_subsystems[PROFILE_GAME].name = "Game";
	m_subsystems[PROFILE_ACTION].name = "Action";
	m_subsystems[PROFILE_EDITOR].name = "Editor";
	m_subsystems[PROFILE_NETWORK].name = "Network";
	m_subsystems[PROFILE_SYSTEM].name = "System";
	m_subsystems[PROFILE_INPUT].name = "Input";
	m_subsystems[PROFILE_SYNC].name = "Sync";

	for (int i=0;i<sizeof(m_subsystems)/sizeof(m_subsystems[0]);i++)
	{
		m_subsystems[i].budgetTime = 66.6f;
		m_subsystems[i].maxTime = 0.0f;
	}

	m_subsystems[PROFILE_AI].budgetTime = 5.0f;
	m_subsystems[PROFILE_ACTION].budgetTime = 3.0f;
	m_subsystems[PROFILE_GAME].budgetTime = 2.0f;

	m_peakDisplayDuration = 8.0f;
	m_offset = 0.0f;

	m_bRenderAdditionalSubsystems = false;

	m_perfStatsDumpCountdown = 0.f;
	m_perfStatsScreenshotCountdown = 0.f;
	m_pPerfStatsDumpPeriodCVar = NULL;
	m_pTimerSmoothingCVar = NULL;
};

//////////////////////////////////////////////////////////////////////////
CFrameProfileSystem::~CFrameProfileSystem()
{








	g_bProfilerEnabled = false;
	delete m_pSampler;

	// Delete graphs for all frame profilers.
#ifdef WIN32
	if (hPsapiModule)
		::FreeLibrary( hPsapiModule );
	hPsapiModule = NULL;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Init( ISystem *pSystem, int nThreadSupport )
{
	m_pSystem = pSystem;
	m_nThreadSupport = nThreadSupport;

	gEnv->callbackStartSection = &StartProfilerSection;
	gEnv->callbackEndSection = &EndProfilerSection;

	CFrameProfilerTimer::Init(m_nThreadSupport >= 2);

	REGISTER_CVAR(profile_callstack,0,0,"Logs all Call Stacks of the selected profiler function for one frame" );
	REGISTER_CVAR(profile_log,0,0,"Logs profiler output" );
	REGISTER_FLOAT("e_PerfStatsDumpPeriod", 0.0f, VF_NULL,"Time period for dumping performance stats");
}

//////////////////////////////////////////////////////////////////////////
float CFrameProfileSystem::TicksToSeconds (int64 nTime)
{
	return CFrameProfilerTimer::TicksToSeconds(nTime);
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Done()
{
	for (int i = 0; i < (int)m_profilers.size(); i++)
	{
		if (m_profilers[i])
		{
			SAFE_DELETE( m_profilers[i]->m_pGraph );
			SAFE_DELETE( m_profilers[i]->m_pOfflineHistory );
		}
	}
	for (int i = 0; i < (int)m_netTrafficProfilers.size(); i++)
	{
		if (m_netTrafficProfilers[i])
		{
			SAFE_DELETE( m_netTrafficProfilers[i]->m_pGraph );
			SAFE_DELETE( m_netTrafficProfilers[i]->m_pOfflineHistory );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetProfiling(bool on, bool display, char *prefix, ISystem *pSystem)
{
	Enable( on,display );
	m_pPrefix = prefix;
	if(on && m_nCurSample<0)
	{
		m_nCurSample = 0;
		CryLogAlways("Profiling data started (%s), prefix = \"%s\"", display ? "display only" : "tracing", prefix);

		m_frameTimeOfflineHistory.m_selfTime.reserve(1000);
		m_frameTimeOfflineHistory.m_count.reserve(1000);
	}
	else if(!on && m_nCurSample>=0)
	{
		CryLogAlways("Profiling data finished");
		{
#ifdef WIN32
			// find the "frameprofileXX" filename for the file
			char outfilename[32] = "frameprofile.dat";
			// while there is such file already
			for (int i = 0; (GetFileAttributes (outfilename) != INVALID_FILE_ATTRIBUTES) && i < 1000; ++i)
				sprintf (outfilename, "frameprofile%02d.dat", i);

			FILE *f = fopen(outfilename, "wb");
			if(!f)
			{
				CryFatalError("Could not write profiling data to file!");
			}
			else
			{
				int i;
				// Find out how many profilers was active.
				int numProf = 0;

				for (i = 0; i < (int)m_pProfilers->size(); i++)
				{
					CFrameProfiler *pProfiler = (*m_pProfilers)[i];
					if (pProfiler && pProfiler->m_pOfflineHistory)
						numProf++;
				}

				fwrite("FPROFDAT", 8, 1, f);                // magic header, for what its worth
				int version = 2;                            // bump this if any of the format below changes
				fwrite(&version, sizeof(int), 1, f); 

				int numSamples = m_nCurSample;
				fwrite(&numSamples, sizeof(int), 1, f);   // number of samples per group (everything little endian)
				int mapsize = numProf+1; // Plus 1 global.
				fwrite(&mapsize, sizeof(int), 1, f);

				// Write global profiler.
				fwrite( "__frametime",strlen("__frametime")+1,1, f);
				int len = (int)m_frameTimeOfflineHistory.m_selfTime.size();
				assert( len == numSamples );
				for(i = 0; i<len; i++)
				{
					fwrite( &m_frameTimeOfflineHistory.m_selfTime[i], 1, sizeof(int),   f);
					fwrite( &m_frameTimeOfflineHistory.m_count[i], 1, sizeof(short), f);
				};

				// Write other profilers.
				for (i = 0; i < (int)m_pProfilers->size(); i++)
				{
					CFrameProfiler *pProfiler = (*m_pProfilers)[i];
					if (!pProfiler || !pProfiler->m_pOfflineHistory)
						continue;

					const char *name = GetFullName(pProfiler);
					//int slen = strlen(name)+1;
					fwrite(name, strlen(name)+1,1,f);
					
					len = (int)pProfiler->m_pOfflineHistory->m_selfTime.size();
					assert( len == numSamples );
					for(int i = 0; i<len; i++)
					{
						fwrite( &pProfiler->m_pOfflineHistory->m_selfTime[i], 1, sizeof(int),   f);
						fwrite( &pProfiler->m_pOfflineHistory->m_count[i], 1, sizeof(short), f);
					};

					// Delete offline data, from profiler.
					SAFE_DELETE( pProfiler->m_pOfflineHistory );
				};
				fclose(f);
				CryLogAlways("Profiling data saved to file '%s'",outfilename);
			};
#endif
		};
		m_frameTimeOfflineHistory.m_selfTime.clear();
		m_frameTimeOfflineHistory.m_count.clear();
		m_nCurSample = -1;
	};
};

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Enable( bool bCollect,bool bDisplay )
{
	if (m_bEnabled != bCollect)
	{
#ifdef WIN32
		if (bCollect)
		{
			CryLogAlways( "SetThreadAffinityMask to %d",(int)1 );
			if (SetThreadAffinityMask( GetCurrentThread(),1 ) == 0)
			{
				CryLogAlways( "SetThreadAffinityMask Failed" );
			}
		}
		else
		{
			DWORD_PTR mask1,mask2;
			GetProcessAffinityMask( GetCurrentProcess(),&mask1,&mask2 );
			CryLogAlways( "SetThreadAffinityMask to %d",(int)mask1 );
			if (SetThreadAffinityMask( GetCurrentThread(),mask1) == 0)
			{
				CryLogAlways( "SetThreadAffinityMask Failed" );
			}
			if(m_bCollectionPaused)
			{
				if(gEnv->pHardwareMouse)
				{
					gEnv->pHardwareMouse->DecrementCounter();
				}
			}
		}
#endif


		Reset();
	}

	m_bEnabled = bCollect;
	m_bDisplay = bDisplay;
	m_bDisplayedProfilersValid = false;








}

void CFrameProfileSystem::EnableHistograms( bool bEnableHistograms )
{
	if (m_bEnableHistograms != bEnableHistograms)
	{
		
	}
	m_bEnableHistograms = bEnableHistograms;
	m_bDisplayedProfilersValid = false;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartSampling( int nMaxSamples )
{
	if (m_pSampler)
	{
		m_pSampler->SetMaxSamples( nMaxSamples );
		m_pSampler->Start();
	}
}

//////////////////////////////////////////////////////////////////////////
CFrameProfiler* CFrameProfileSystem::GetProfiler( int index ) const
{
	assert( index >= 0 && index < (int)m_profilers.size() );
	return m_profilers[index];
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Reset()
{
	//gEnv->callbackStartSection = &StartProfilerSection;
	//gEnv->callbackEndSection = &EndProfilerSection;

	m_absolutepeaks.clear();
	m_ProfilerThreads.Reset(m_pSystem);
	m_pCurrentCustomSection = 0;
	m_totalProfileTime = 0;
	m_frameStartTime = 0;
	m_frameTime = 0;
	m_frameLostTime = 0;
	m_bCollectionPaused = false;

	int i;
	// Iterate over all profilers update thier history and reset them.
	for (i = 0; i < (int)m_profilers.size(); i++)
	{
		CFrameProfiler *pProfiler = m_profilers[i];
		// Reset profiler.
		if(pProfiler)
		{
			pProfiler->m_totalTimeHistory.Clear();
			pProfiler->m_selfTimeHistory.Clear();
			pProfiler->m_countHistory.Clear();
			pProfiler->m_sumTotalTime = 0;
			pProfiler->m_sumSelfTime = 0;
			pProfiler->m_totalTime = 0;
			pProfiler->m_selfTime = 0;
			pProfiler->m_count = 0;
			pProfiler->m_displayedValue = 0;
	//		pProfiler->m_displayedCurrentValue = 0;
			pProfiler->m_variance = 0;
		}
	}
	// Iterate over all profilers update thier history and reset them.
	for (i = 0; i < (int)m_netTrafficProfilers.size(); i++)
	{
		CFrameProfiler *pProfiler = m_netTrafficProfilers[i];
		if(pProfiler)
		{
			// Reset profiler.
			pProfiler->m_totalTimeHistory.Clear();
			pProfiler->m_selfTimeHistory.Clear();
			pProfiler->m_countHistory.Clear();
			pProfiler->m_sumTotalTime = 0;
			pProfiler->m_sumSelfTime = 0;
			pProfiler->m_totalTime = 0;
			pProfiler->m_selfTime = 0;
			pProfiler->m_count = 0;
		}
	}

	{
#ifdef WIN32
		int threadPriority = GetThreadPriority( GetCurrentThread() );
		SetThreadPriority( GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL );
#endif
		// Compute call overhead.
		static CFrameProfiler staticFrameProfiler( GetISystem(),"CallOverhead",PROFILE_SYSTEM );

		bool bPrevState = gEnv->bProfilerEnabled;
		gEnv->bProfilerEnabled = true;
		m_callOverheadTime = 0;
		int64 overheadTime = CFrameProfilerTimer::GetTicks();
		for (int n = 0; n < 1000; n++)
		{
			CFrameProfilerSection frameProfilerSection( &staticFrameProfiler );
		}
		overheadTime = CFrameProfilerTimer::GetTicks() - overheadTime;
		m_callOverheadTime = overheadTime / 1000;
		if (m_callOverheadTime < 0)
			m_callOverheadTime = 0;
		gEnv->bProfilerEnabled = bPrevState;

#ifdef WIN32
		SetThreadPriority( GetCurrentThread(),threadPriority );
#endif
		CryLogAlways( "Call overhead: %ld",(long)m_callOverheadTime );
	}
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddFrameProfiler( CFrameProfiler *pProfiler )
{
	CryAutoCriticalSection lock(m_profilersLock);

	assert( pProfiler );
	if (!pProfiler)
		return;

	// Set default thread id.
	pProfiler->m_threadId = GetMainThreadId();
	if ((EProfiledSubsystem)pProfiler->m_subsystem == PROFILE_NETWORK_TRAFFIC)
	{
		m_netTrafficProfilers.push_back( pProfiler );
	}
	else
	{
		m_profilers.push_back( pProfiler );
	}
}

static int nMAX_THREADED_PROFILERS = 256;

void CFrameProfileSystem::SProfilerThreads::Reset(ISystem* pSystem)
{
	m_aThreadStacks[0].pProfilerSection = 0;
	if (!m_pReservedProfilers)
	{
		// Allocate reserved profilers;
		for (int i = 0; i < nMAX_THREADED_PROFILERS; i++)
		{
			CFrameProfiler* pProf = new CFrameProfiler(pSystem, "");
			pProf->m_pNextThread = m_pReservedProfilers;
			m_pReservedProfilers = pProf;
		}
	}
}

CFrameProfiler* CFrameProfileSystem::SProfilerThreads::NewThreadProfiler(CFrameProfiler* pMainProfiler, TThreadId nThreadId)
{
	// Create new profiler for thread.
	WriteLock lock(m_lock);
	if (!m_pReservedProfilers)
		return 0;
	CFrameProfiler* pProfiler = m_pReservedProfilers;
	m_pReservedProfilers = pProfiler->m_pNextThread;

	// Init.
	memset(pProfiler, 0, sizeof(*pProfiler));
	pProfiler->m_name = pMainProfiler->m_name;
	pProfiler->m_subsystem = pMainProfiler->m_subsystem;
	pProfiler->m_pISystem = pMainProfiler->m_pISystem;
	pProfiler->m_threadId = nThreadId;

	// Insert in frame profiler list.
	pProfiler->m_pNextThread = pMainProfiler->m_pNextThread;
	pMainProfiler->m_pNextThread = pProfiler;

	return pProfiler;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartProfilerSection( CFrameProfilerSection *pSection )
{
	//if (CFrameProfileSystem::s_pFrameProfileSystem->m_bCollectionPaused)
	//	return;

	// Find thread instance to collect profiles in.
	TThreadId nThreadId = GetCurrentThreadId();
	if (nThreadId != s_pFrameProfileSystem->GetMainThreadId())
	{
		if (!s_pFrameProfileSystem->m_nThreadSupport)
			return;
		pSection->m_pFrameProfiler = s_pFrameProfileSystem->m_ProfilerThreads.GetThreadProfiler( pSection->m_pFrameProfiler, nThreadId );
		if (!pSection->m_pFrameProfiler)
			return;
	}

	// Push section on stack for current thread.
	s_pFrameProfileSystem->m_ProfilerThreads.PushSection( pSection, nThreadId );
	pSection->m_excludeTime = 0;
	pSection->m_startTime = CFrameProfilerTimer::GetTicks();
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndProfilerSection( CFrameProfilerSection *pSection )
{
	//if (CFrameProfileSystem::s_pFrameProfileSystem->m_bCollectionPaused)
	//	return;

	int64 endTime = CFrameProfilerTimer::GetTicks();

	if (!s_pFrameProfileSystem->m_nThreadSupport)
	{
		if (GetCurrentThreadId() != s_pFrameProfileSystem->GetMainThreadId())
			return;
	}

	CFrameProfiler *pProfiler = pSection->m_pFrameProfiler;
	if (!pProfiler)
		return;

	assert(GetCurrentThreadId() == pProfiler->m_threadId);

	int64 totalTime = endTime - pSection->m_startTime;
	int64 selfTime = totalTime - pSection->m_excludeTime;
	if (totalTime < 0)
		totalTime = 0;
	if (selfTime < 0)
		selfTime = 0;

	if (s_nFilterThreadId && GetCurrentThreadId() != s_nFilterThreadId)
	{
		selfTime = 0;
		totalTime = 0;
		pProfiler->m_count = 0;
	}

/*
	if(pProfiler->m_count!=0 && (pProfiler->m_count%4000)==0)
	if(strcmp(pProfiler->m_name,"CryFree")==0 || strcmp(pProfiler->m_name,"CryMalloc")==0)
	{
		float fVal = TranslateToDisplayValue(pProfiler->m_totalTime);

		char str[256];

		sprintf(str,"EndProfilerSection %d %s %f\n",pProfiler->m_count,pProfiler->m_name,fVal);
		OutputDebugString(str);

		if(pProfiler->m_count>=8000)
		{
			OutputDebugString("-------------------------------------- >8000\n");
		}
	}
*/
	pProfiler->m_count++;
	pProfiler->m_selfTime += selfTime;
	pProfiler->m_totalTime += totalTime;

	s_pFrameProfileSystem->m_ProfilerThreads.PopSection( pSection, pProfiler->m_threadId );
	if (pSection->m_pParent)
	{
		// If we have parent, add this counter total time to parent exclude time.
		pSection->m_pParent->m_excludeTime += totalTime + s_pFrameProfileSystem->m_callOverheadTime;
		if (!pProfiler->m_pParent && pSection->m_pParent->m_pFrameProfiler)
		{
			pSection->m_pParent->m_pFrameProfiler->m_bHaveChildren = 1;
			pProfiler->m_pParent = pSection->m_pParent->m_pFrameProfiler;
		}
	}
	else
		pProfiler->m_pParent = 0;

	if (profile_callstack)
	{
		if (pProfiler == s_pFrameProfileSystem->m_pGraphProfiler)
		{
			float fMillis = CFrameProfilerTimer::TicksToMilliseconds(totalTime);
			CryLogAlways( "Function Profiler: %s  (time=%.2fms)",(const char*)s_pFrameProfileSystem->GetFullName(pSection->m_pFrameProfiler), fMillis );
			GetISystem()->debug_LogCallStack();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartMemoryProfilerSection( CFrameProfilerSection *pSection )
{
	CryAutoCriticalSection lock(m_staticProfilersLock);

	// Find thread instance to collect profiles in.
	TThreadId nThreadId = GetCurrentThreadId();
	pSection->m_pFrameProfiler = s_pFrameProfileSystem->m_ProfilerThreads.GetThreadProfiler( pSection->m_pFrameProfiler, nThreadId );
	if (pSection->m_pFrameProfiler)
	{
		// Push section on stack for current thread.
		s_pFrameProfileSystem->m_ProfilerThreads.PushSection( pSection, nThreadId );
		pSection->m_excludeTime = 0;
		pSection->m_startTime = CryMemoryGetAllocatedSize();
	}
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndMemoryProfilerSection( CFrameProfilerSection *pSection )
{
	CryAutoCriticalSection lock(m_staticProfilersLock);

	int64 endTime = CryMemoryGetAllocatedSize();

	CFrameProfiler *pProfiler = pSection->m_pFrameProfiler;
	if (!pProfiler)
		return;

	assert(GetCurrentThreadId() == pProfiler->m_threadId);

	int64 totalTime = endTime - pSection->m_startTime;
	int64 selfTime = totalTime - pSection->m_excludeTime;
	if (totalTime < 0)
		totalTime = 0;
	if (selfTime < 0)
		selfTime = 0;

	if (s_nFilterThreadId && GetCurrentThreadId() != s_nFilterThreadId)
	{
		selfTime = 0;
		totalTime = 0;
		pProfiler->m_count = 0;
	}

	// Ignore allocation functions.
	if (0 == _stricmp(pProfiler->m_name,"CryMalloc") ||
		0 == _stricmp(pProfiler->m_name,"CryRealloc") ||
		0 == _stricmp(pProfiler->m_name,"CryFree"))
	{
		selfTime = 0;
		totalTime = 0;
	}

	pProfiler->m_count++;
	pProfiler->m_selfTime += selfTime;
	pProfiler->m_totalTime += totalTime;

	s_pFrameProfileSystem->m_ProfilerThreads.PopSection( pSection, pProfiler->m_threadId );
	if (pSection->m_pParent)
	{
		// If we have parent, add this counter total time to parent exclude time.
		pSection->m_pParent->m_excludeTime += totalTime;
		if (!pProfiler->m_pParent && pSection->m_pParent->m_pFrameProfiler)
		{
			pSection->m_pParent->m_pFrameProfiler->m_bHaveChildren = 1;
			pProfiler->m_pParent = pSection->m_pParent->m_pFrameProfiler;
		}
	}
	else
		pProfiler->m_pParent = 0;
}

//////////////////////////////////////////////////////////////////////////
CFrameProfilerSection const* CFrameProfileSystem::GetCurrentProfilerSection()
{
	// Return current (main-thread) profile section.
	return m_ProfilerThreads.GetMainSection();
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartCustomSection( CCustomProfilerSection *pSection )
{
	if (!m_bNetworkProfiling)
		return;

	pSection->m_excludeValue = 0;
	pSection->m_pParent = m_pCurrentCustomSection;
	m_pCurrentCustomSection = pSection;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndCustomSection( CCustomProfilerSection *pSection )
{
	if (!m_bNetworkProfiling || m_bCollectionPaused)
		return;

	int total = *pSection->m_pValue;
	int self = total - pSection->m_excludeValue;

	CFrameProfiler *pProfiler = pSection->m_pFrameProfiler;
	pProfiler->m_count++;
	pProfiler->m_selfTime += self;
	pProfiler->m_totalTime += total;

	m_pCurrentCustomSection = pSection->m_pParent;
	if (m_pCurrentCustomSection)
	{
		// If we have parent, add this counter total time to parent exclude time.
		m_pCurrentCustomSection->m_pFrameProfiler->m_bHaveChildren = 1;
		m_pCurrentCustomSection->m_excludeValue += total;
		pProfiler->m_pParent = m_pCurrentCustomSection->m_pFrameProfiler;
	}
	else
		pProfiler->m_pParent = 0;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartFrame()
{
	m_bCollect = m_bEnabled && !m_bCollectionPaused;
	
	if (m_bCollect)
	{
		CFrameProfilerTimer::Init(m_nThreadSupport >= 2);
		m_ProfilerThreads.Reset(m_pSystem);
		m_pCurrentCustomSection = 0;
		m_frameStartTime = CFrameProfilerTimer::GetTicks();
	}
	g_bProfilerEnabled = m_bCollect;
	/*
	if (m_displayQuantity == SUBSYSTEM_INFO)
	{
		for (int i = 0; i < PROFILE_LAST_SUBSYSTEM; i++)
		{
			//m_subsystems[i].selfTime = 0;
		}
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
float CFrameProfileSystem::TranslateToDisplayValue( int64 val )
{
	if (!m_bNetworkProfiling && !m_bMemoryProfiling)
		return CFrameProfilerTimer::TicksToMilliseconds(val);
	else if (m_displayQuantity == ALLOCATED_MEMORY)
		return (float)(val >> 10); // In Kilobytes
	else if (m_displayQuantity == ALLOCATED_MEMORY_BYTES)
		return (float)val; // In bytes
	else if (m_bNetworkProfiling)
		return (float)val;
	return (float)val;
}

const char* CFrameProfileSystem::GetFullName( CFrameProfiler *pProfiler )
{
	if (pProfiler->m_threadId == GetMainThreadId())
		return pProfiler->m_name;

	// Add thread name.
	static char sFullName[256];
	const char* sThreadName = CryThreadGetName(pProfiler->m_threadId);
	if (sThreadName)
	{
		_snprintf(sFullName,sizeof(sFullName),"%s @%s", pProfiler->m_name, sThreadName);
		sFullName[sizeof(sFullName)-1] = 0;
	}
	else
	{
		_snprintf(sFullName,sizeof(sFullName),"%s @%d", pProfiler->m_name, pProfiler->m_threadId);
		sFullName[sizeof(sFullName)-1] = 0;
	}
	return sFullName;
}

//////////////////////////////////////////////////////////////////////////
const char* CFrameProfileSystem::GetModuleName( CFrameProfiler *pProfiler )
{
	return m_subsystems[pProfiler->m_subsystem].name;
}

const char* CFrameProfileSystem::GetModuleName( int num ) const
{
	assert( num >= 0 && num < PROFILE_LAST_SUBSYSTEM);
	if ( num < GetModuleCount() )
		return m_subsystems[num].name;
	else
		return 0;
}

const int CFrameProfileSystem::GetModuleCount( void ) const
{
	return (int)PROFILE_LAST_SUBSYSTEM;
}

const float CFrameProfileSystem::GetOverBudgetRatio( int modulenumber ) const
{
	assert( modulenumber >= 0 && modulenumber < PROFILE_LAST_SUBSYSTEM);
	if ( modulenumber < GetModuleCount() && m_subsystems[modulenumber].totalAnalized )
		return (float)m_subsystems[modulenumber].totalOverBudget/(float)m_subsystems[modulenumber].totalAnalized;
	else
		return 0.0f;
}

///////////////////////////////////////////////////////////////////////////
// Performance stats logging
// Work in progress on C1C
// Please speak to Miles Clapham if you're considering using/changing this
///////////////////////////////////////////////////////////////////////////
char* Base64Encode(const uint8 *buffer, int len)
{
	static const char base64Dict[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
																			 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
																			 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
																			 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
	int b64Len = ((len + 2) / 3) * 4;
	char *b64Buf = new char[b64Len + 1];
	int byteCount = 0;
	int cycle = 0;

	for (int i=0; i<b64Len; i++)
	{
		uint8 val = 0;
		
		switch (cycle)
		{
		case 0:
			val = buffer[byteCount] >> 2;
			break;
		case 1:
			val = (buffer[byteCount++] & 0x3) << 4;
			val |= buffer[byteCount] >> 4;
			break;
		case 2:
			val = (buffer[byteCount++] & 0xf) << 2;
			val |= buffer[byteCount] >> 6;
			break;
		case 3:
			val = buffer[byteCount++] & 0x3f;
			break;
		}

		cycle = (cycle + 1) & 3;

		b64Buf[i] = base64Dict[val];
	}

	b64Buf[b64Len] = '\0';

	return b64Buf;
}

void CFrameProfileSystem::PerfStatsLoggingTick()
{
	if (!m_pPerfStatsDumpPeriodCVar)
	{
		m_pPerfStatsDumpPeriodCVar = gEnv->pConsole->GetCVar("e_PerfStatsDumpPeriod");
	}

	if (m_pPerfStatsDumpPeriodCVar)
	{
		float perfStatsDumpPeriod = m_pPerfStatsDumpPeriodCVar->GetFVal();
		if ((perfStatsDumpPeriod > 0.f) && (gEnv->pTimer->GetCurrTime(ITimer::ETIMER_GAME) > 0.f))	// only start once in game
		{
			if (!m_pTimerSmoothingCVar)
			{
				m_pTimerSmoothingCVar = gEnv->pConsole->GetCVar("t_Smoothing");
			}

			if (m_pTimerSmoothingCVar)
			{
				m_pTimerSmoothingCVar->Set(0);
			}

			if (m_perfStatsDumpCountdown <= 0.f)
			{
				PerfStatsDumpFrameRecords();
				m_perfStatFrameRecords.clear();
				m_perfStatsDumpCountdown = perfStatsDumpPeriod;
			}

			m_perfStatsDumpCountdown -= CFrameProfilerTimer::TicksToSeconds(m_frameTime);

			PerfStatsAddFrameRecord();
		}
	}
}

class CCompareFrameProfilersSelfTime
{
public:
	bool operator() (const CFrameProfiler *p1, const CFrameProfiler *p2)
	{
		return p1->m_selfTime > p2->m_selfTime;
	}
};

void CFrameProfileSystem::PerfStatsAddFrameRecord()
{
	size_t numProfilers = m_pProfilers->size();

	// we want to avoid reallocation of m_perfStatDumpProfilers
	// even if numProfilers is quite large (in the thousands), it'll only be tens of KB
	m_perfStatDumpProfilers.reserve(MAX(numProfilers, m_perfStatDumpProfilers.size()));
	m_perfStatDumpProfilers.resize(0);

	for (size_t i=0; i<numProfilers; i++)
	{
    CFrameProfiler *pProfiler = (*m_pProfilers)[i];

    if (CFrameProfilerTimer::TicksToMilliseconds(pProfiler->m_selfTime) > 10.f &&
				pProfiler->m_threadId == GetMainThreadId())
		{
			m_perfStatDumpProfilers.push_back(pProfiler);
		}
	}

	std::sort(m_perfStatDumpProfilers.begin(), m_perfStatDumpProfilers.end(), CCompareFrameProfilersSelfTime());
	m_perfStatDumpProfilers.resize(MIN(m_perfStatDumpProfilers.size(), 10));

	SPerfStatFrameRecord perfStatFrameRecord;

	perfStatFrameRecord.m_frameTimeInS = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);
	perfStatFrameRecord.m_frameLengthInMS = CFrameProfilerTimer::TicksToMilliseconds(m_frameTime);

	perfStatFrameRecord.m_MTLoadInMS = perfStatFrameRecord.m_frameLengthInMS;

	IRenderer::SRenderTimes renderTimes;
	gEnv->pRenderer->GetRenderTimes(renderTimes);

	perfStatFrameRecord.m_RTWaitingForMTInMS = renderTimes.fWaitForMain * 1000.f;
	perfStatFrameRecord.m_MTWaitingForRTInMS = renderTimes.fWaitForRender * 1000.f;
	perfStatFrameRecord.m_RTWaitingForGPUInMS = renderTimes.fWaitForGPU * 1000.f;
	perfStatFrameRecord.m_RTLoadInMS = renderTimes.fTimeProcessedRT * 1000.f;
	perfStatFrameRecord.m_GPUIdleInPercent = renderTimes.fTimeGPUIdlePercent;
















	{
		perfStatFrameRecord.m_FMODLoadInPercent = 0.f;
		perfStatFrameRecord.m_SPUDXPSLoadInMS = 0.f;
	}

	IMemoryManager::SProcessMemInfo processMemInfo;
	GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
	perfStatFrameRecord.m_mainMemUsageInMB = (int)(processMemInfo.PagefileUsage/(1024.f * 1024.f));

	size_t vidMem, lastVidMem;
	gEnv->pRenderer->GetVideoMemoryUsageStats(vidMem, lastVidMem);
	perfStatFrameRecord.m_vidMemUsageInMB = vidMem;

	I3DEngine::SObjectsStreamingStatus objectsStreamingStatus;
	gEnv->p3DEngine->GetObjectsStreamingStatus(objectsStreamingStatus);
	perfStatFrameRecord.m_cgfStreamingMemUsedInB = objectsStreamingStatus.nAllocatedBytes;
	perfStatFrameRecord.m_cgfStreamingMemRequiredInB = objectsStreamingStatus.nMemRequired;

	int nShadowVolPolys;
	gEnv->pRenderer->GetPolyCount(perfStatFrameRecord.m_numTris, nShadowVolPolys);

	perfStatFrameRecord.m_numDrawCalls = gEnv->pRenderer->GetCurrentNumberOfDrawCalls();
	perfStatFrameRecord.m_numPostEffects = *((int*)gEnv->pRenderer->EF_Query(EFQ_NumActivePostEffects));

	PodArray<CDLight*> *pLights = gEnv->p3DEngine->GetDynamicLightSources();
	const uint32 nDynamicLights = pLights->Count();
	
	if (nDynamicLights)
	{
		uint32 nShadowCastingLights = 0;

		for (int i=0; i<nDynamicLights; i++)
		{
			CDLight *pLight = pLights->GetAt(i);

			if (pLight->m_Flags & DLF_CASTSHADOW_MAPS)
			{
				nShadowCastingLights++;
			}
		}

		perfStatFrameRecord.m_numForwardLights = nDynamicLights;
		perfStatFrameRecord.m_numForwardShadowCastingLights = nShadowCastingLights;
	}
	
	Matrix33 m = Matrix33(gEnv->pRenderer->GetCamera().GetMatrix());
	perfStatFrameRecord.m_rot = RAD2DEG(Ang3::GetAnglesXYZ(m));
	perfStatFrameRecord.m_pos = gEnv->pRenderer->GetCamera().GetPosition();

	m_perfStatsScreenshotCountdown -= CFrameProfilerTimer::TicksToSeconds(m_frameTime);

	if (m_perfStatsScreenshotCountdown < 0.f)
	{
		m_perfStatsScreenshotCountdown += 1.f;
		perfStatFrameRecord.m_pScreenshotBuf = NULL; //gEnv->pRenderer->ReadFrameBuffer()
	}
	else
	{
		perfStatFrameRecord.m_pScreenshotBuf = NULL;
	}

	size_t numDumpProfilers = m_perfStatDumpProfilers.size();
	perfStatFrameRecord.m_frameProfilerRecords.reserve(numDumpProfilers);

	for (size_t i=0; i<numDumpProfilers; i++)
	{
    CFrameProfiler *pProfiler = m_perfStatDumpProfilers[i];
		SPerfStatFrameProfilerRecord profilerRecord;

		profilerRecord.m_pProfiler = pProfiler;
		profilerRecord.m_count = pProfiler->m_count;
		profilerRecord.m_totalTime = CFrameProfilerTimer::TicksToMilliseconds(pProfiler->m_totalTime);
		profilerRecord.m_selfTime = CFrameProfilerTimer::TicksToMilliseconds(pProfiler->m_selfTime);

		perfStatFrameRecord.m_frameProfilerRecords.push_back(profilerRecord);
	}

	m_perfStatFrameRecords.push_back(perfStatFrameRecord);
}

void CFrameProfileSystem::PerfStatsDumpFrameRecords()
{
	string perfStatsLogFilename = PerfStatsGetLogFilename();
	const char* modeStr = "at";

	//first time we have opened this file, open for write
	if(m_perfStatsFileName!=perfStatsLogFilename)
	{
		modeStr = "wt";
		m_perfStatsFileName = perfStatsLogFilename;
	}

	FILE *pPerfStatsFile = fxopen(m_perfStatsFileName.c_str(), modeStr);

	if (pPerfStatsFile)
	{
		CryLogAlways("Dumping perf stats log to '%s'\n", m_perfStatsFileName.c_str());

		for (size_t i=0; i<m_perfStatFrameRecords.size(); i++)
		{
			SPerfStatFrameRecord &frameRecord = m_perfStatFrameRecords[i];

			char *encodedScreenshotBuf = frameRecord.m_pScreenshotBuf ? Base64Encode(frameRecord.m_pScreenshotBuf, 3 + (frameRecord.m_pScreenshotBuf[0] * frameRecord.m_pScreenshotBuf[1] * 3)) : NULL;

			fprintf(pPerfStatsFile, "%f %f %f %f %f %f %f %f %f %f %d %d %d %d %d %d %d %d %d %f %f %f %f %f %f %s",
				frameRecord.m_frameTimeInS, frameRecord.m_frameLengthInMS,
				frameRecord.m_MTLoadInMS, frameRecord.m_MTWaitingForRTInMS,
				frameRecord.m_RTLoadInMS, frameRecord.m_RTWaitingForMTInMS, frameRecord.m_RTWaitingForGPUInMS,
				frameRecord.m_GPUIdleInPercent, frameRecord.m_SPUDXPSLoadInMS, frameRecord.m_FMODLoadInPercent,
				frameRecord.m_mainMemUsageInMB, frameRecord.m_vidMemUsageInMB,
				frameRecord.m_cgfStreamingMemUsedInB, frameRecord.m_cgfStreamingMemRequiredInB,
				frameRecord.m_numTris, frameRecord.m_numDrawCalls,
				frameRecord.m_numPostEffects, frameRecord.m_numForwardLights, frameRecord.m_numForwardShadowCastingLights,
				frameRecord.m_pos.x, frameRecord.m_pos.y, frameRecord.m_pos.z, frameRecord.m_rot.x, frameRecord.m_rot.y, frameRecord.m_rot.z,
				encodedScreenshotBuf ? encodedScreenshotBuf : "");

			delete[] encodedScreenshotBuf;
			delete[] frameRecord.m_pScreenshotBuf;

			for (size_t j=0; j<frameRecord.m_frameProfilerRecords.size(); j++)
			{
				SPerfStatFrameProfilerRecord &frameProfilerRecord = frameRecord.m_frameProfilerRecords[j];
				fprintf(pPerfStatsFile, " { '%s' '%s' %d %f %f }", string(GetFullName(frameProfilerRecord.m_pProfiler), 64).c_str(),
																													 string(GetModuleName(frameProfilerRecord.m_pProfiler), 64).c_str(),
																													 frameProfilerRecord.m_count, frameProfilerRecord.m_totalTime, frameProfilerRecord.m_selfTime);
			}

			fprintf(pPerfStatsFile, "\n");
		}

		fclose(pPerfStatsFile);
	}
}

string CFrameProfileSystem::PerfStatsGetLogFilename()
{
	string logFilename = "perflog.log";
	const char *mapName = NULL;

	if(gEnv->pGame)
	{
		mapName = gEnv->pGame->GetIGameFramework()->GetLevelName();
	}

	if(mapName)
	{
		const char *nameNoDir = strrchr(mapName, '/');

		//strip directory for now
		if(nameNoDir)
			mapName = nameNoDir+1;

		logFilename = "perflog_";
		logFilename += mapName;
		logFilename += ".log";
	}

	return logFilename;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndFrame()
{
	if (m_pSampler)
		m_pSampler->Update();

	if (!m_bEnabled)
		m_totalProfileTime = 0;

	if (!m_bEnabled && !m_bNetworkProfiling)
		return;

#if defined(WIN32) || defined(PS3)

	bool bPaused = false;

	bPaused = (GetKeyState(VK_SCROLL) & 1);
	/*
	if (GetISystem()->GetIInput())
	{
		static TKeyName scrollKey("scrolllock");
		bPaused = (GetISystem()->GetIInput()->GetKeyState(scrollKey) & 1);
	}
	*/
	// Will pause or resume collection.
	if (bPaused != m_bCollectionPaused)
	{
		if (bPaused)
		{
			// Must be paused.
			if(gEnv->pHardwareMouse)
			{
				gEnv->pHardwareMouse->IncrementCounter();
			}
		}
		else
		{
			// Must be resumed.
			if(gEnv->pHardwareMouse)
			{
				gEnv->pHardwareMouse->DecrementCounter();
			}
		}
	}
	if (m_bCollectionPaused != bPaused)
	{
		m_bDisplayedProfilersValid = false;
	}
	m_bCollectionPaused = bPaused;

#endif

	if (m_bCollectionPaused || (!m_bCollect && !m_bNetworkProfiling))
		return;

	FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_SYSTEM,g_bProfilerEnabled );

	float smoothTime = CFrameProfilerTimer::TicksToSeconds(m_totalProfileTime);
	float smoothFactor = 1.f - m_pSystem->GetITimer()->GetProfileFrameBlending(&smoothTime);

	int64 endTime = CFrameProfilerTimer::GetTicks();
	m_frameTime = endTime - m_frameStartTime;
	m_totalProfileTime += m_frameTime;

	PerfStatsLoggingTick();

	//////////////////////////////////////////////////////////////////////////
	// Lets see how many page faults we got.
	//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) && !defined(WIN64)

	// PSAPI is not supported on window9x
	// so, don't use it
	if (!m_bNoPsapiDll)
	{
		// Load psapi dll.
		if (!pfGetProcessMemoryInfo)
		{
			hPsapiModule = ::LoadLibrary( "psapi.dll" );
			if (hPsapiModule)
			{
				pfGetProcessMemoryInfo = (FUNC_GetProcessMemoryInfo)(::GetProcAddress(hPsapiModule,"GetProcessMemoryInfo" ));
			}
			else
				m_bNoPsapiDll = true;
		}
		if (pfGetProcessMemoryInfo)
		{
			PROCESS_MEMORY_COUNTERS pc;
			pfGetProcessMemoryInfo( GetCurrentProcess(),&pc,sizeof(pc) );
			m_nPagesFaultsLastFrame = (int)(pc.PageFaultCount - m_nLastPageFaultCount);
			m_nLastPageFaultCount = pc.PageFaultCount;
			static float fLastPFTime = 0;
			static int nPFCounter = 0;
			nPFCounter += m_nPagesFaultsLastFrame;
			float fCurr = CFrameProfilerTimer::TicksToMilliseconds(endTime);
			if ((fCurr - fLastPFTime) >= 1000)
			{
				fLastPFTime = fCurr;
				m_nPagesFaultsPerSec = nPFCounter;
				nPFCounter = 0;
			}
		}
	}
#endif
	//////////////////////////////////////////////////////////////////////////


	static ICVar* pVarMode = m_pSystem->GetIConsole()->GetCVar("profile_weighting");
	int nWeightMode = pVarMode ? pVarMode->GetIVal() : 0;

	int64 selfAccountedTime = 0;

	m_frameTimeHistory.Add( CFrameProfilerTimer::TicksToMilliseconds(m_frameTime) );
	m_frameTimeLostHistory.Add( CFrameProfilerTimer::TicksToMilliseconds(m_frameLostTime) );

	float fPeakTolerance = m_peakTolerance;
	if (m_bMemoryProfiling)
	{
		fPeakTolerance = 0;
	}





























	// Iterate over all profilers update thier history and reset them.
	for (int i = 0; i < (int)m_pProfilers->size(); i++)
	{
		CFrameProfiler *pProfiler = (*m_pProfilers)[i];

		if (!pProfiler)
			continue;

		// Skip this profiler if its filtered out.
		if (m_bSubsystemFilterEnabled && pProfiler->m_subsystem != (uint8)m_subsystemFilter)
			continue;

		//filter stall profilers
		if((int)m_displayQuantity != STALL_TIME && pProfiler->m_isStallProfiler || 
			(int)m_displayQuantity == STALL_TIME && !pProfiler->m_isStallProfiler)
			continue;

		if (pProfiler->m_threadId == GetMainThreadId())
		{
			selfAccountedTime += pProfiler->m_selfTime;
			pProfiler->m_sumTotalTime += pProfiler->m_totalTime;
			pProfiler->m_sumSelfTime += pProfiler->m_selfTime;
		}

		float aveValue;
		float currentValue;
		float variance;

		float fDisplay_TotalTime = TranslateToDisplayValue(pProfiler->m_totalTime);
		float fDisplay_SelfTime = TranslateToDisplayValue(pProfiler->m_selfTime);

		bool bEnablePeaks = nWeightMode == 0;
		
		switch ((int)m_displayQuantity)
		{
		case SELF_TIME:
		case PEAK_TIME:
		case COUNT_INFO:
		case STALL_TIME:
			currentValue = fDisplay_SelfTime;
			aveValue = pProfiler->m_selfTimeHistory.GetAverage();
			variance = (currentValue - aveValue) * (currentValue - aveValue);
			break;
		case TOTAL_TIME:
			currentValue = fDisplay_TotalTime;
			aveValue = pProfiler->m_totalTimeHistory.GetAverage();
			variance = (currentValue - aveValue) * (currentValue - aveValue);
			break;
		case SELF_TIME_EXTENDED:
			currentValue = fDisplay_SelfTime;
			aveValue = pProfiler->m_selfTimeHistory.GetAverage();
			variance = (currentValue - aveValue) * (currentValue - aveValue);
			bEnablePeaks = false;
			break;
		case TOTAL_TIME_EXTENDED:
			currentValue = fDisplay_TotalTime;
			aveValue = pProfiler->m_totalTimeHistory.GetAverage();
			variance = (currentValue - aveValue) * (currentValue - aveValue);
			bEnablePeaks = false;
			break;
		case SUBSYSTEM_INFO:
			currentValue = (float)pProfiler->m_count;
			aveValue = pProfiler->m_selfTimeHistory.GetAverage();
			variance = (currentValue - aveValue) * (currentValue - aveValue);
			if (pProfiler->m_subsystem < PROFILE_LAST_SUBSYSTEM)
			{
				m_subsystems[pProfiler->m_subsystem].selfTime += aveValue;
			}
			break;
		case STANDART_DEVIATION:
			// Standart Deviation.
			aveValue = pProfiler->m_selfTimeHistory.GetStdDeviation();
			aveValue *= 100.0f;
			currentValue = aveValue;
			variance = 0;
			break;

		case ALLOCATED_MEMORY:
			//currentValue = pProfiler->m_selfTime / 1024.0f;
			//FIX this
			//fDisplay_SelfTime = fDisplay_SelfTime;
			//fDisplay_TotalTime = fDisplay_TotalTime;
			currentValue = pProfiler->m_selfTimeHistory.GetMax();
			aveValue = pProfiler->m_selfTimeHistory.GetAverage();
			variance = (currentValue - aveValue) * (currentValue - aveValue);
			break;
		};

		if((SUBSYSTEM_INFO != m_displayQuantity) && (GetAdditionalSubsystems()))
		{
			float faveValue = pProfiler->m_selfTimeHistory.GetAverage();
			if (pProfiler->m_subsystem < PROFILE_LAST_SUBSYSTEM)
			{
				m_subsystems[pProfiler->m_subsystem].selfTime += faveValue;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Records Peaks.
		uint64 frameID = gEnv->pRenderer->GetFrameID(false);
		if (bEnablePeaks)
		{
			float prevValue = pProfiler->m_selfTimeHistory.GetLast();
			float peakValue = fDisplay_SelfTime;

			if (pProfiler->m_latestFrame != frameID - 1) {
				prevValue = 0.0f;
			}

			if ((peakValue-prevValue) > fPeakTolerance)
			{
				SPeakRecord peak;
				peak.pProfiler = pProfiler;
				peak.peakValue = peakValue;
				peak.avarageValue = pProfiler->m_selfTimeHistory.GetAverage();
				peak.count = pProfiler->m_count;
				peak.pageFaults = m_nPagesFaultsLastFrame;
				peak.when = CFrameProfilerTimer::TicksToSeconds(m_totalProfileTime);
				AddPeak( peak );

				// Call peak callbacks.
				if (!m_peakCallbacks.empty())
				{
					for (int n = 0; n < (int)m_peakCallbacks.size(); n++)
					{
						m_peakCallbacks[n]->OnFrameProfilerPeak( pProfiler,peakValue );
					}
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////

		pProfiler->m_latestFrame = frameID;
		pProfiler->m_totalTimeHistory.Add( fDisplay_TotalTime );
		pProfiler->m_selfTimeHistory.Add( fDisplay_SelfTime );
		pProfiler->m_countHistory.Add( pProfiler->m_count );

//		pProfiler->m_displayedCurrentValue = nWeightMode > 0 ? currentValue : aveValue;
		pProfiler->m_displayedValue = pProfiler->m_displayedValue*smoothFactor + currentValue*(1.0f - smoothFactor);
		pProfiler->m_variance = pProfiler->m_variance*smoothFactor + variance*(1.0f - smoothFactor);

		if (m_bMemoryProfiling)
		{
			pProfiler->m_displayedValue = currentValue;
		}

		if (m_bEnableHistograms)
		{
			if (!pProfiler->m_pGraph)
			{
				// Create graph.
				pProfiler->m_pGraph = new CFrameProfilerGraph;
			}
			// Update values in histogram graph.
			if (m_histogramsMaxPos != pProfiler->m_pGraph->m_data.size())
			{
				pProfiler->m_pGraph->m_width = m_histogramsMaxPos;
				pProfiler->m_pGraph->m_height = m_histogramsHeight;
				pProfiler->m_pGraph->m_data.resize( m_histogramsMaxPos );
			}
			float millis;
			if (m_displayQuantity == TOTAL_TIME || m_displayQuantity == TOTAL_TIME_EXTENDED)
				millis = m_histogramScale * pProfiler->m_totalTimeHistory.GetLast();
			else
				millis = m_histogramScale * pProfiler->m_selfTimeHistory.GetLast();
			if (millis < 0) millis = 0;
			if (millis > 255) millis = 255;
			pProfiler->m_pGraph->m_data[m_histogramsCurrPos] = 255-FtoI(millis); // must use ftoi.
		}

		if (m_nCurSample >= 0)
		{
			UpdateOfflineHistory( pProfiler );
		}

		// Reset profiler.
		pProfiler->m_totalTime = 0;
		pProfiler->m_selfTime = 0;
		pProfiler->m_count = 0;
	}

	for (int i = 0; i < PROFILE_LAST_SUBSYSTEM; i++)
	{
		m_subsystems[i].totalAnalized ++;
		if (m_subsystems[i].selfTime>m_subsystems[i].budgetTime)
		{
			m_subsystems[i].totalOverBudget ++;
		}

		m_subsystems[i].maxTime = (float)__fsel(m_subsystems[i].maxTime - m_subsystems[i].selfTime, m_subsystems[i].maxTime, m_subsystems[i].selfTime);
	}

	m_frameLostTime = m_frameTime - selfAccountedTime;

	if (m_nCurSample >= 0)
	{
		// Keep offline global time history.
		m_frameTimeOfflineHistory.m_selfTime.push_back( FtoI(CFrameProfilerTimer::TicksToMilliseconds(m_frameTime)*1000) );
		m_frameTimeOfflineHistory.m_count.push_back(1);
		m_nCurSample++;
	}
	//AdvanceFrame( m_pSystem );

	// Reset profile callstack var.
	profile_callstack = 0;
}


//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::UpdateOfflineHistory( CFrameProfiler *pProfiler )
{
	if (!pProfiler->m_pOfflineHistory)
	{
		pProfiler->m_pOfflineHistory = new CFrameProfilerOfflineHistory;
		pProfiler->m_pOfflineHistory->m_count.reserve( 1000+m_nCurSample*2 );
		pProfiler->m_pOfflineHistory->m_selfTime.reserve( 1000+m_nCurSample*2 );
	}
	int prevCont = (int)pProfiler->m_pOfflineHistory->m_selfTime.size();
	int newCount = m_nCurSample+1;
	pProfiler->m_pOfflineHistory->m_selfTime.resize( newCount );
	pProfiler->m_pOfflineHistory->m_count.resize( newCount );

	unsigned int micros = FtoI(CFrameProfilerTimer::TicksToMilliseconds(pProfiler->m_selfTime)*1000);
	unsigned short count = pProfiler->m_count;
	for (int i = prevCont; i < newCount; i++)
	{
		pProfiler->m_pOfflineHistory->m_selfTime[i] = micros;
		pProfiler->m_pOfflineHistory->m_count[i] = count;
	}
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddPeak( SPeakRecord &peak )
{
	// Add peak.
	if (m_peaks.size() > MAX_PEAK_PROFILERS)
		m_peaks.pop_back();

	if(m_pSystem->IsDedicated())
		m_pSystem->GetILog()->Log("Peak: name:'%s' val:%.2f avg:%.2f cnt:%d",
			(const char*)GetFullName(peak.pProfiler),
			peak.peakValue,
			peak.avarageValue,
			peak.count);

	/*
	// Check to see if this function is already a peak.
	for (int i = 0; i < (int)m_peaks.size(); i++)
	{
		if (m_peaks[i].pProfiler == peak.pProfiler)
		{
			m_peaks.erase( m_peaks.begin()+i );
			break;
		}
	}
	*/
	m_peaks.insert( m_peaks.begin(),peak );

	// Add to absolute value

	for (int i = 0; i < (int)m_absolutepeaks.size(); i++)
	{
		if (m_absolutepeaks[i].pProfiler == peak.pProfiler)
			if (m_absolutepeaks[i].peakValue < peak.peakValue)
			{
				m_absolutepeaks.erase( m_absolutepeaks.begin()+i );
				break;
			}
			else
				return;

	}

	bool bInserted = false;
	for (size_t i = 0; i < m_absolutepeaks.size(); ++i) {
		if (m_absolutepeaks[i].peakValue < peak.peakValue) {
			m_absolutepeaks[i] = peak;
			bInserted = true;
			break;
		}
	}

	if (!bInserted && m_absolutepeaks.size() < MAX_ABSOLUTEPEAK_PROFILERS) {
		m_absolutepeaks.push_back(peak);
	}

}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetDisplayQuantity( EDisplayQuantity quantity )
{
	m_displayQuantity = quantity;
	m_bDisplayedProfilersValid = false;
	if (m_displayQuantity == SELF_TIME_EXTENDED || m_displayQuantity == TOTAL_TIME_EXTENDED)
		EnableHistograms(true);
	else
		EnableHistograms(false);

	if (m_displayQuantity == ALLOCATED_MEMORY || m_displayQuantity == ALLOCATED_MEMORY_BYTES)
	{
		m_bMemoryProfiling = true;
		gEnv->callbackStartSection = &StartMemoryProfilerSection;
		gEnv->callbackEndSection = &EndMemoryProfilerSection;
	}
	else
	{
		gEnv->callbackStartSection = &StartProfilerSection;
		gEnv->callbackEndSection = &EndProfilerSection;
		m_bMemoryProfiling = false;
	}
};

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetSubsystemFilter( bool bFilterSubsystem,EProfiledSubsystem subsystem )
{
	m_bSubsystemFilterEnabled = bFilterSubsystem;
	m_subsystemFilter = subsystem;
	m_bDisplayedProfilersValid = false;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetSubsystemFilterThread( const char *sFilterThreadName )
{
	if (sFilterThreadName[0] != 0)
	{
		s_nFilterThreadId = GetISystem()->GetIThreadTaskManager()->GetThreadByName(sFilterThreadName);
	}
	else
	{
		s_nFilterThreadId = 0;
	}
	if (s_nFilterThreadId)
	{
		m_filterThreadName = sFilterThreadName;
	}
	else
		m_filterThreadName = "";
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetSubsystemFilter( const char *szFilterName )
{
	bool bFound = false;
	for (int i = 0; i < PROFILE_LAST_SUBSYSTEM; i++)
	{
		if (!m_subsystems[i].name)
			continue;
		if (stricmp(m_subsystems[i].name,szFilterName) == 0)
		{
			SetSubsystemFilter( true,(EProfiledSubsystem)i );
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		// Check for count limit.
		int nCount = atoi(szFilterName);
		if (nCount > 0)
			m_maxProfileCount = nCount;
		else
			SetSubsystemFilter( false,PROFILE_ANY );
	}
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddPeaksListener( IFrameProfilePeakCallback *pPeakCallback )
{
	// Only add one time.
	stl::push_back_unique( m_peakCallbacks,pPeakCallback );
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RemovePeaksListener( IFrameProfilePeakCallback *pPeakCallback )
{
	stl::find_and_erase( m_peakCallbacks,pPeakCallback );
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EnableMemoryProfile( bool bEnable )
{
	if (bEnable != m_bDisplayMemoryInfo)
		m_bLogMemoryInfo = true;
	m_bDisplayMemoryInfo = bEnable;
}
















































































#else

// dummy functions if no frame profiler is used, needs to be in the cpp file for PS3
// or else devirtualization failes(problems with devirtualized inline functions and generating of these wrappers
CFrameProfiler* CFrameProfileSystem::GetProfiler( int index ) const {return NULL;}
void CFrameProfileSystem::StartFrame() {}
void CFrameProfileSystem::EndFrame() {}
void CFrameProfileSystem::AddPeaksListener( IFrameProfilePeakCallback *pPeakCallback ) {}
void CFrameProfileSystem::RemovePeaksListener( IFrameProfilePeakCallback *pPeakCallback ) {}
float CFrameProfileSystem::TicksToSeconds (int64 nTime) { return 0.0f; }
void CFrameProfileSystem::AddFrameProfiler( CFrameProfiler *pProfiler ){}

#endif

#include UNIQUE_VIRTUAL_WRAPPER(IFrameProfileSystem)




