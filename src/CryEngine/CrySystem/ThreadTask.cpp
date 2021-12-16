////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   ThreadTask.cpp
//  Version:     v1.00
//  Created:     19/09/2006 by Timur.
//  Compilers:   Visual Studio 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ThreadTask.h"
#include "CPUDetect.h"
#include "IConsole.h"
#include "System.h"

#pragma warning(disable : 6312) // Possible infinite loop: use of the constant EXCEPTION_CONTINUE_EXECUTION in the exception-filter expression of a try-except. Execution restarts in the protected block
#pragma warning(disable : 6322) // Empty _except block

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //WIN32

#ifdef WIN64
	#pragma warning(disable: 4334)
#endif

#include "BitFiddling.h"










//////////////////////////////////////////////////////////////////////////
CThreadTask_Thread::CThreadTask_Thread( CThreadTaskManager *pTaskMgr,const char *sName,
																			 int nIndex,int nProcessor, int nThreadPriority, ThreadPoolHandle poolHandle/* = -1*/  )
{
	m_nThreadPriority = nThreadPriority;
	m_pTaskManager = pTaskMgr;
	m_sThreadName = sName;
	bStopThread = false;
	bRunning = false;
	m_hThreadHandle = 0;
	m_nThreadIndex = nIndex;
	m_nProcessor = nProcessor;
	m_poolHandle = poolHandle;
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::SingleUpdate()
{
	while (true)
	{
		m_pProcessingTask = NULL;
		
		{
			if (tasks.empty())
				break;
			// remove from queue
			m_pProcessingTask = tasks.pop();
		}

		if (m_pProcessingTask)
			m_pProcessingTask->m_pTask->OnUpdate();
		
//Diesel cut
		//if(m_pProcessingTask)	// push it back
		//	tasks.push(m_pProcessingTask);

		if (bStopThread)
			break;
	}

	if(m_poolHandle != -1)	// if this thread is in the pool, we need to reassign some tasks for it
		m_pTaskManager->BalanceThreadInPool(this);
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::Run()
{
	Init();

	bRunning = true;
	while (!bStopThread)
	{
		while (tasks.empty() && !bStopThread)
			m_waitForTasks.Wait();
		
		if (!bStopThread)
		{
			SingleUpdate();
		}
	}
	bRunning = false;
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::Cancel()
{
	bStopThread = true;
	m_waitForTasks.Set();
	Stop();

	// for blocking thread notify the blocking task
	if(m_nThreadIndex == -1)
		if(m_pProcessingTask && m_pProcessingTask->m_params.nFlags & THREAD_TASK_BLOCKING)	// check if we have a blocking task
			if(m_pProcessingTask->m_pTask)				// cancel it
				m_pProcessingTask->m_pTask->Stop();

	WaitForThread();
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::Terminate()
{
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::AddTask( SThreadTaskInfo *pTaskInfo )
{
	pTaskInfo->m_pThread = this;
	tasks.push(pTaskInfo);
	m_waitForTasks.Set();
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::RemoveTask( SThreadTaskInfo *pTaskInfo )
{
	if(m_pProcessingTask == pTaskInfo)
	{
		pTaskInfo->m_pThread = NULL;
		m_pProcessingTask = NULL;
		return;
	}

	// search for task(mirrored search because of locklessness)
	bool bFound = false;
	Tasks newTasks;
	while(!tasks.empty())
	{
		SThreadTaskInfo* pTask = tasks.pop();
		if(pTask == pTaskInfo)
		{
			pTaskInfo->m_pThread = NULL;
			bFound = true;
			break;
		}
		if(pTask)
			newTasks.push(pTask);
	}
	(void)bFound;
	assert(bFound);

	// fill back
	while(!newTasks.empty())
		tasks.push(newTasks.pop());
}

//////////////////////////////////////////////////////////////////////////
void CThreadTask_Thread::RemoveAllTasks()
{
	while(!tasks.empty())
		tasks.pop()->m_pThread = NULL;
}

void CThreadTask_Thread::Init()
{
#if defined(WIN32) || defined(XENON)
	// init thread
	m_hThreadHandle = GetCurrentThread();
#endif

	// Name this thread.
	CryThreadSetName( GetCurrentThreadId(),m_sThreadName );
#ifndef PS3
	// Set affinity
	if (m_nProcessor > 0)
	{
		ChangeProcessor(m_nProcessor);
	}
#endif
}

void CThreadTask_Thread::ChangeProcessor( int nProcessor )
{
	// note this function is not thread-safe
	m_nProcessor = nProcessor;
#ifdef WIN32
	DWORD_PTR mask1,mask2;
	GetProcessAffinityMask( GetCurrentProcess(),&mask1,&mask2 );
	if(BIT(m_nProcessor) & mask1)	// Check if we have this affinity
		SetThreadAffinityMask( m_hThreadHandle, BIT(m_nProcessor) );
	else // Reserve CPU 1 for main thread.
		SetThreadAffinityMask(m_hThreadHandle,(mask1 & (~1)) );
	assert(THREAD_PRIORITY_IDLE <= m_nThreadPriority && m_nThreadPriority <= THREAD_PRIORITY_TIME_CRITICAL);
	SetThreadPriority(m_hThreadHandle, m_nThreadPriority);








#else
	assert(0);
#endif
}


















//////////////////////////////////////////////////////////////////////////
CThreadTaskManager::CThreadTaskManager()
{



	m_nMaxThreads = MAX_TASK_THREADS_COUNT;
	
	SetThreadName( GetCurrentThreadId(),"Main" );

	m_systemThreads.push_back( GetCurrentThreadId() );






}

//////////////////////////////////////////////////////////////////////////
CThreadTaskManager::~CThreadTaskManager()
{
	CloseThreads();

	AUTO_MODIFYLOCK(m_threadsPoolsLock);
	while(!m_threadsPools.empty())
	{
		bool res = DestroyThreadsPool(m_threadsPools.begin()->m_hHandle);
		assert(res);
	}




}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::StopAllThreads()
{
	if (m_threads.empty())
		return;

	size_t i;
	// Start from 2nd thread, 1st is main thread.
	for (i = 1; i < m_threads.size(); i++)
	{
		CThreadTask_Thread *pThread  = m_threads[i];
		pThread->Cancel();
	}
	bool bAllStoped = true;
	do
	{
		bAllStoped = true;
		CrySleep(10);
		for (i = 1; i < m_threads.size(); i++)
		{
			CThreadTask_Thread *pThread = m_threads[i];
			// Needs ReadWriteBarrier here.
			if (pThread->bRunning)
				bAllStoped = false;
		}
	}
	while (!bAllStoped);
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::CloseThreads()
{
	if (m_threads.size() > 0)
		StopAllThreads();
	for (size_t i = MAIN_THREAD_INDEX,numThreads = m_threads.size(); i < numThreads; i++)
	{
		delete m_threads[i];
	}
	m_threads.clear();
	//make sure blocking threads are cancelled
	size_t nThreads = m_blockingThreads.size();
	for(size_t it = 0;it<nThreads;++it)
	{
		CThreadTask_Thread* thread = m_blockingThreads[it];
		thread->Cancel();
	}
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::InitThreads()
{
	CloseThreads();

	// Create a dummy thread that is used for main thread.
	m_threads.resize(1);
	m_threads[0] = new CThreadTask_Thread(this,"Main Thread",0,((CSystem*)gEnv->pSystem)->m_sys_main_CPU->GetIVal(),THREAD_PRIORITY_NORMAL);

	CCpuFeatures *pCPU = ((CSystem*)gEnv->pSystem)->GetCPUFeatures();

	int nThreads = min( (int)m_nMaxThreads,(int)pCPU->GetCPUCount() );

	if (nThreads < 1)
		nThreads = 1;
	int nAddThreads = nThreads-1;
	
	char str[32];
	m_threads.resize(1+nAddThreads);
	for (int i = 0; i < nAddThreads; i++)
	{
		int nIndex = i+1;
		int nCPU = i+1;
		sprintf( str,"TaskThread%d",i );
		if (i < MAX_TASK_THREADS_COUNT)
			nCPU = ((CSystem*)gEnv->pSystem)->m_sys_TaskThread_CPU[i]->GetIVal();

		m_threads[nIndex] = new CThreadTask_Thread(this,str,nIndex,nCPU,THREAD_PRIORITY_NORMAL);
		m_threads[nIndex]->Start(0, str, THREAD_PRIORITY_NORMAL, SIMPLE_THREAD_STACK_SIZE_KB*1024);
	}
	RescheduleTasks();
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::SetMaxThreadCount( int nMaxThreads )
{
	if (nMaxThreads == m_nMaxThreads)
		return;

	m_nMaxThreads = nMaxThreads;

	bool bReallocateThreads = false;
	if (m_nMaxThreads < (int)m_threads.size())
	{
		bReallocateThreads = true;
	}
	if (m_nMaxThreads > (int)m_threads.size())
	{
		CCpuFeatures *pCPU = ((CSystem*)gEnv->pSystem)->GetCPUFeatures();
		if (m_threads.size() < pCPU->GetCPUCount())
			bReallocateThreads = true;
	}
	if (bReallocateThreads)
	{
		CloseThreads();
		InitThreads();
	}
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::RegisterTask( IThreadTask *pTask,const SThreadTaskParams &options )
{
	if(!pTask)
	{
		assert(0);
		return;
	}
	SThreadTaskInfo *pTaskInfo = pTask->GetTaskInfo();
	pTaskInfo->m_pTask = pTask;
	pTaskInfo->m_params = options;

	if ((options.nFlags & THREAD_TASK_BLOCKING) == 0)
		ScheduleTask(pTaskInfo);
	else
	{
		// Blocking task will need it`s own thread.
		// allow normal thread priority to be altered by a margin, currently only used for PS3
		const int threadPriority = THREAD_PRIORITY_NORMAL/*+options.nPriorityOff*/;
		CThreadTask_Thread *pThread = 
			new CThreadTask_Thread(this,options.name,-1,options.nPreferedThread,threadPriority);
		pThread->Start(0, (char*)options.name, threadPriority, options.nStackSizeKB * 1024);
		pThread->AddTask(pTaskInfo);

		m_blockingThreads.push_back(pThread);
	}
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::UnregisterTask( IThreadTask *pTask )
{
	assert(pTask);
	if(!pTask)
		return;
	SThreadTaskInfo *pTaskInfo = pTask->GetTaskInfo();
	assert(pTaskInfo);

	// Remove from thread.
	if (pTaskInfo->m_pThread)
		pTaskInfo->m_pThread->RemoveTask( pTaskInfo );

	pTask->Stop();

	if (pTaskInfo->m_params.nFlags & THREAD_TASK_BLOCKING)
	{
		Threads::iterator end = m_blockingThreads.end();
		Threads::iterator toErase = std::find(m_blockingThreads.begin(), end, pTaskInfo->m_pThread);

		if (toErase != end) // impossible to find anything. no push_back done on m_blockingThreads
		{
			(*toErase)->Cancel();
			m_blockingThreads.erase(toErase);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::ScheduleTask( SThreadTaskInfo *pTaskInfo )
{
	size_t i;

	if (pTaskInfo->m_pThread)
	{
		assert(0);
		pTaskInfo->m_pThread->RemoveTask(pTaskInfo);
	}

	CThreadTask_Thread *pGoodThread = NULL;

	if(pTaskInfo->m_params.nFlags & THREAD_TASK_ASSIGN_TO_POOL)
	{
		AUTO_READLOCK(m_threadsPoolsLock);

		// find the pool
		CThreadsPool* pool = NULL;
		size_t nSize = m_threadsPools.size();
		for(i=0;i<nSize;++i)
			if(m_threadsPools[i].m_hHandle == pTaskInfo->m_params.nThreadsGroupId)
			pool = &m_threadsPools[i];

		if(pool)
		{
			// Find available thread for the task.
			for (i = 0; i < (int)pool->m_Threads.size(); ++i)
			{
				CThreadTask_Thread *pThread = pool->m_Threads[i];
				const bool threadIsFree = pThread->tasks.empty() && pThread->m_pProcessingTask == NULL;
				if (threadIsFree || pGoodThread == NULL)
				{
					pGoodThread = pThread;
					if(threadIsFree)
						break;
				}
			}
		}
		else
		{
			gEnv->pLog->LogError("[Error]Task manager: threads pool not found!");
			assert(0);
		}
	}
	else if (pTaskInfo->m_params.nPreferedThread >= 0 && pTaskInfo->m_params.nPreferedThread < (int)m_threads.size())
	{
		assert((int)m_threads.size() > pTaskInfo->m_params.nPreferedThread);
		// Assign task to desired thread.
		pGoodThread = m_threads[pTaskInfo->m_params.nPreferedThread];
	}
	else
	{
		// Find available thread for the task.
		for (i = MAIN_THREAD_INDEX+1; i < (int)m_threads.size(); i++)
		{
			CThreadTask_Thread *pThread = m_threads[i];
			if (pThread->tasks.empty() || pGoodThread == NULL)
			{
				pGoodThread = pThread;
				if(pThread->tasks.empty())
					break;
			}
		}
	}
	if (!pGoodThread && !m_threads.empty())
	{
		// Assign to last thread.
		pGoodThread = m_threads[m_threads.size()-1];
	}

	if(pGoodThread)
		pGoodThread->AddTask( pTaskInfo );
	else
		m_unassignedTasks.push(pTaskInfo);
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::RescheduleTasks()
{
	// Un-schedule all tasks.
	for (int i = 0; i < (int)m_threads.size(); i++)
	{
		while(!m_threads[i]->tasks.empty())
		{
			SThreadTaskInfo* pTask = m_threads[i]->tasks.pop();
			if(!pTask)
				break;
			if(pTask->m_params.nFlags & THREAD_TASK_BLOCKING)	// Do not schedule blocking tasks.
			{
				m_threads[i]->tasks.push(pTask);
				break;
			}
			pTask->m_pThread = NULL;
			m_unassignedTasks.push(pTask);
		}
	}

	while(!m_unassignedTasks.empty())
		ScheduleTask( m_unassignedTasks.pop() );
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::OnUpdate()
{
	FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_SYSTEM,g_bProfilerEnabled );

	// Emulate single update of the main thread.
	if (m_threads[0])
		m_threads[0]->SingleUpdate();

	// assign unassigned tasks
	while(!m_unassignedTasks.empty())
		ScheduleTask(m_unassignedTasks.pop());

	// balance all pools
	AUTO_READLOCK(m_threadsPoolsLock);
	size_t nSize = m_threadsPools.size();
	for(size_t itPool = 0;itPool < nSize;++itPool)
		BalanceThreadsPool(m_threadsPools[itPool].m_hHandle);
}

struct THREADNAME_INFO
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
};

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::SetThreadName( unsigned int dwThreadId,const char *sThreadName )
{
#if defined(WIN32) || defined(XENON)
	if (dwThreadId == -1)
	{
		dwThreadId = GetCurrentThreadId();
	}

	//////////////////////////////////////////////////////////////////////////
	// Rise exception to set thread name for debugger.
	//////////////////////////////////////////////////////////////////////////
	THREADNAME_INFO threadName;
	threadName.dwType = 0x1000;
	threadName.szName = sThreadName;
	threadName.dwThreadID = dwThreadId;
	threadName.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(threadName)/sizeof(DWORD), (ULONG_PTR*)&threadName );
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
#endif





	{
		m_threadNameLock.Lock();
		m_threadNames[dwThreadId] = sThreadName;
		m_threadNameLock.Unlock();
	}
}

//////////////////////////////////////////////////////////////////////////
const char* CThreadTaskManager::GetThreadName( unsigned int dwThreadId )
{
	CryAutoCriticalSection lock(m_threadNameLock);
	ThreadNames::const_iterator it = m_threadNames.find(dwThreadId);
	if (it != m_threadNames.end())
		return it->second.c_str();











		
	return "";
}

//////////////////////////////////////////////////////////////////////////
uint32 CThreadTaskManager::GetThreadByName( const char* sThreadName )
{
	CryAutoCriticalSection lock(m_threadNameLock);
	for (ThreadNames::const_iterator it = m_threadNames.begin(); it != m_threadNames.end(); ++it)
	{
		if (it->second.compareNoCase(sThreadName) == 0)
		{
			return it->first;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::AddSystemThread( uint32 nThreadId )
{
	CryAutoCriticalSection lock(m_systemThreadsLock);
	m_systemThreads.push_back(nThreadId);
}

//////////////////////////////////////////////////////////////////////////
void CThreadTaskManager::RemoveSystemThread( uint32 nThreadId )
{
	CryAutoCriticalSection lock(m_systemThreadsLock);
	stl::find_and_erase(m_systemThreads,nThreadId);
}

//////////////////////////////////////////////////////////////////////////
ThreadPoolHandle CThreadTaskManager::CreateThreadsPool( const ThreadPoolDesc& desc )
{
	AUTO_MODIFYLOCK(m_threadsPoolsLock);

	ThreadPoolHandle newId = m_threadsPools.empty() ? 0 : m_threadsPools.rbegin()->m_hHandle + 1;

	if(desc.AffinityMask == INVALID_AFFINITY)
	{
		assert(0);
		return -1;
	}

	// create the pool
	m_threadsPools.push_back(CThreadsPool());
	CThreadsPool& rPool = m_threadsPools.back();

	// assign the new handle
	rPool.m_hHandle = newId;

	// fill up the desc
	rPool.m_pDescription = desc;

	Threads& threads = rPool.m_Threads;

	std::vector<char> threadName(desc.sPoolName.size() + 30);
	uint32 iThread = 0;
	for(uint32 nIndex = 0;nIndex < sizeof(desc.AffinityMask) * 8;++nIndex)
	{
		// check if we have affinity mask bit set for this thread
		if(!(desc.AffinityMask & (1<<nIndex)))
			continue;

		// create a thread
		sprintf(&threadName[0], "%s%d", desc.sPoolName.c_str(), iThread);
		CThreadTask_Thread* thread = new CThreadTask_Thread(this, &threadName[0], iThread, nIndex, desc.nPriority, newId);

		// start thread
		thread->Start(0, (char*)&threadName[0], THREAD_PRIORITY_NORMAL, SIMPLE_THREAD_STACK_SIZE_KB*1024);

		// add to pool
		threads.push_back(thread);

		iThread++;
	}

	return newId;
}

const bool CThreadTaskManager::DestroyThreadsPool( const ThreadPoolHandle& handle )
{
	AUTO_MODIFYLOCK(m_threadsPoolsLock);

	CThreadsPool* pPool = NULL;
	size_t nSize = m_threadsPools.size();
	size_t iPool=0;
	for(;iPool<nSize;++iPool)
	{
		if(m_threadsPools[iPool].m_hHandle == handle)
		{
			pPool = &m_threadsPools[iPool];
			break;
		}
	}

	if(pPool)
	{
		Threads& threads = pPool->m_Threads;
		size_t nThreads = threads.size();
		for (size_t iThread = 0; iThread < nThreads; ++iThread)
		{
			CThreadTask_Thread* pThread = threads[iThread];
			pThread->Cancel();
			assert(!(pThread->bRunning));
			delete pThread;
		}

		m_threadsPools.erase(m_threadsPools.begin() + iPool);
		return true;
	}
	return false;
}

const bool CThreadTaskManager::GetThreadsPoolDesc(const ThreadPoolHandle handle, ThreadPoolDesc* pDesc) const
{
	AUTO_READLOCK(m_threadsPoolsLock);

	const CThreadsPool* pPool = NULL;
	size_t iPool=0, nSize = m_threadsPools.size();
	for(;iPool<nSize;++iPool)
	{
		if(m_threadsPools[iPool].m_hHandle == handle)
		{
			pPool = &m_threadsPools[iPool];
			break;
		}
	}

	if(pPool)
	{
		if(pDesc)
		{
			*pDesc = pPool->m_pDescription;
			return true;
		}
	}

	return false;
}

const bool CThreadTaskManager::SetThreadsPoolAffinity(const ThreadPoolHandle handle, const ThreadPoolAffinityMask AffinityMask)
{
	CThreadsPool* pPool = NULL;

	AUTO_MODIFYLOCK(m_threadsPoolsLock);

	size_t iPool=0, nSize = m_threadsPools.size();
	for(;iPool<nSize;++iPool)
	{
		if(m_threadsPools[iPool].m_hHandle == handle)
		{
			pPool = &m_threadsPools[iPool];
			break;
		}
	}

	if(pPool)
	{
		return pPool->SetAffinity(AffinityMask);
	}

	return false;
}

void CThreadTaskManager::BalanceThreadsPool( const ThreadPoolHandle& handle )
{
	CThreadsPool* pPool = NULL;

	AUTO_READLOCK(m_threadsPoolsLock);

	size_t iPool=0, nSize = m_threadsPools.size();
	for(;iPool<nSize;++iPool)
	{
		if(m_threadsPools[iPool].m_hHandle == handle)
		{
			pPool = &m_threadsPools[iPool];
			break;
		}
	}

	if(pPool)
	{
		// balancing tasks in the pool
		for(size_t itThread = 0, nThreads = pPool->m_Threads.size();itThread < nThreads; ++itThread )
		{
			CThreadTask_Thread* pThread = pPool->m_Threads[itThread];
			if(pThread->tasks.empty())	// found free thread(without tasks)
			{
				BalanceThreadInPool(pThread, &pPool->m_Threads);
			}
		}
	}
	else
		assert(0);
}

void CThreadTaskManager::BalanceThreadInPool( CThreadTask_Thread* pFreeThread, Threads* pThreads /* = NULL */ )
{
	assert(pFreeThread->m_poolHandle != -1);

	AUTO_READLOCK(m_threadsPoolsLock);

	if(pThreads == NULL)
	{
		CThreadsPool* pPool = NULL;
		size_t iPool=0, nSize = m_threadsPools.size();
		for(;iPool<nSize;++iPool)
		{
			if(m_threadsPools[iPool].m_hHandle == pFreeThread->m_poolHandle)
			{
				pPool = &m_threadsPools[iPool];
				break;
			}
		}

		if(pPool)
			pThreads = &pPool->m_Threads;
	}
	assert(pThreads);
	// search for thread with tasks
	for(size_t itAnotherThread = 0, nThreads = pThreads->size();itAnotherThread < nThreads; ++itAnotherThread )
	{
		CThreadTask_Thread* pAnotherThread = (*pThreads)[itAnotherThread];
		if(pFreeThread == pAnotherThread)
			continue;
		if(pAnotherThread->tasks.empty())
			continue;

		// we found a thread with more than one task
		SThreadTaskInfo* pTask = pAnotherThread->tasks.pop();
		if(pTask)
		{
			assert(pTask->m_pThread == pAnotherThread);
			// reassign the last task to another thread
			pFreeThread->AddTask(pTask);
			break;	// process next free thread
		}
	}
}

void CThreadTaskManager::MarkThisThreadForDebugging(const char *name, bool bDump) 
{
	bDump ? ::MarkThisThreadForDebugging(name) : ::UnmarkThisThreadFromDebugging();
}

#include UNIQUE_VIRTUAL_WRAPPER(IThreadTaskManager)

const bool CThreadTaskManager::CThreadsPool::SetAffinity( const ThreadPoolAffinityMask AffinityMask )
{



	// check if all threads in the pool are covered by the bits of this mask
	if(CountBits(AffinityMask) != m_Threads.size())
	{
		// wrong arguments
		return false;
	}

	// update affinity mask
	m_pDescription.AffinityMask = AffinityMask;

	size_t itThread = 0;
	for(uint32 nProcessorIndex = 0;nProcessorIndex < sizeof(AffinityMask) * 8;++nProcessorIndex)
	{
		assert(itThread < m_Threads.size());
		// check if we have affinity mask bit set for this thread
		if(!(AffinityMask & (1<<nProcessorIndex)))
			continue;

		// changin thread's affinity in the pool
		m_Threads[itThread]->ChangeProcessor(nProcessorIndex);
		++itThread;
	}
	return true;

}
