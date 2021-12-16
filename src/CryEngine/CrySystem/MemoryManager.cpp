
#include "StdAfx.h"
#include "MemoryManager.h"
#include "platform.h"
#include "MemReplay.h"

#if defined(WIN32) && !defined(XENON)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <Psapi.h>
#endif





extern long g_TotalAllocatedMemory;

//////////////////////////////////////////////////////////////////////////
CCryMemoryManager::CCryMemoryManager()
{
}

//////////////////////////////////////////////////////////////////////////
CCryMemoryManager* CCryMemoryManager::GetInstance()
{
	static CCryMemoryManager s_instance;
	return &s_instance;
}

//////////////////////////////////////////////////////////////////////////
bool CCryMemoryManager::GetProcessMemInfo( SProcessMemInfo &minfo )
{
	ZeroStruct(minfo);

#if defined(WIN32) && !defined(XENON)

	//////////////////////////////////////////////////////////////////////////
	typedef BOOL (WINAPI *GetProcessMemoryInfoProc)( HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD );

	PROCESS_MEMORY_COUNTERS pc;
	ZeroStruct(pc);
	pc.cb = sizeof(pc);
	static HMODULE hPSAPI = LoadLibrary("psapi.dll");
	if (hPSAPI)
	{
		static GetProcessMemoryInfoProc pGetProcessMemoryInfo = (GetProcessMemoryInfoProc)GetProcAddress(hPSAPI, "GetProcessMemoryInfo");
		if (pGetProcessMemoryInfo)
		{
			if (pGetProcessMemoryInfo( GetCurrentProcess(), &pc, sizeof(pc) ))
			{
				minfo.PageFaultCount = pc.PageFaultCount;
				minfo.PeakWorkingSetSize = pc.PeakWorkingSetSize;
				minfo.WorkingSetSize = pc.WorkingSetSize;
				minfo.QuotaPeakPagedPoolUsage = pc.QuotaPeakPagedPoolUsage;
				minfo.QuotaPagedPoolUsage = pc.QuotaPagedPoolUsage;
				minfo.QuotaPeakNonPagedPoolUsage = pc.QuotaPeakNonPagedPoolUsage;
				minfo.QuotaNonPagedPoolUsage = pc.QuotaNonPagedPoolUsage;
				minfo.PagefileUsage = pc.PagefileUsage;
				minfo.PeakPagefileUsage = pc.PeakPagefileUsage;
				return true;
			}
		}
	}













#endif
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CCryMemoryManager::FakeAllocation( long size )
{
	CryInterlockedExchangeAdd(&g_TotalAllocatedMemory, size);
}

//////////////////////////////////////////////////////////////////////////
IMemReplay* CCryMemoryManager::GetIMemReplay()
{
#if CAPTURE_REPLAY_LOG
	return CMemReplay::GetInstance();
#else
	static IMemReplay m;
	return &m;
#endif
}

//////////////////////////////////////////////////////////////////////////
extern "C"
{
	CRYMEMORYMANAGER_API void CryGetIMemoryManagerInterface( void **pIMemoryManager )
	{
		// Static instance of the memory manager
		*pIMemoryManager = CCryMemoryManager::GetInstance();
	}

};