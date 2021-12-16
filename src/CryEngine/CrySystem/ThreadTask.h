////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   ThreadTask.h
//  Version:     v1.00
//  Created:     19/09/2006 by Timur.
//  Compilers:   Visual Studio 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#ifndef __thread_task_h__
#define __thread_task_h__

#include <IThreadTask.h>
#include <CryThread.h>
#include <MultiThread_Containers.h>
#include "CritSection.h"

#define MAIN_THREAD_INDEX 0

class CThreadTask_Thread;


void MarkThisThreadForDebugging(const char *name);
void UnmarkThisThreadFromDebugging();
void UpdateFPExceptionsMaskForThreads();


class CThreadTaskManager;
///
struct IThreadTaskThread
{
	virtual void Run() = 0;
	virtual void Cancel() = 0;
};
//////////////////////////////////////////////////////////////////////////
class CThreadTask_Thread : public CryThread<IThreadTaskThread>, public IThreadTask_Thread
{
protected:
	void Init();
public:
	CThreadTask_Thread( CThreadTaskManager *pTaskMgr,const char *sName,int nThreadIndex,int nProcessor,int nThreadPriority, ThreadPoolHandle poolHandle = -1 );
	
	virtual void Run();
	virtual void Cancel();
	virtual void Terminate();

	virtual void AddTask( SThreadTaskInfo *pTaskInfo );
	virtual void RemoveTask( SThreadTaskInfo *pTaskInfo );
	virtual void RemoveAllTasks();
	virtual void SingleUpdate();

	void ChangeProcessor(int nProcessor);
public:
	CThreadTaskManager *m_pTaskManager;
	string m_sThreadName;
	int m_nThreadIndex;					// -1 means the thread is blocking
	int m_nProcessor;
	int m_nThreadPriority;

	THREAD_HANDLE m_hThreadHandle;

	// Tasks running on this thread.
	typedef CryMT::CLocklessPointerQueue<SThreadTaskInfo> Tasks;
	Tasks tasks;

	// The task is being processing now
	SThreadTaskInfo*	m_pProcessingTask;

	CryEvent m_waitForTasks;

	// Set to true when thread must stop.
	volatile bool bStopThread;
	volatile bool bRunning;

	// handle of threads pool which this thread belongs to(if any)
	ThreadPoolHandle m_poolHandle;
};

//////////////////////////////////////////////////////////////////////////
class CThreadTaskManager : public IThreadTaskManager
{
private:
	typedef std::vector<CThreadTask_Thread*> Threads;
	// note: this struct is auxilary and NOT thread-safe
	// it is only for internal use inside the task manager
	struct CThreadsPool
	{
		ThreadPoolHandle	m_hHandle;
		Threads						m_Threads;
		ThreadPoolDesc		m_pDescription;
		const bool SetAffinity(const ThreadPoolAffinityMask AffinityMask);
		const bool operator < (const CThreadsPool& p) const { return m_hHandle < p.m_hHandle; }
		const bool operator == (const CThreadsPool& p) const { return m_hHandle == p.m_hHandle; }
	};

	typedef std::vector<CThreadsPool> ThreadsPools;

public:
	CThreadTaskManager();
	~CThreadTaskManager();

	void InitThreads();
	void CloseThreads();
	void StopAllThreads();

	//////////////////////////////////////////////////////////////////////////
	// IThreadTaskManager
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void RegisterTask( IThreadTask *pTask,const SThreadTaskParams &options );
	VIRTUAL void UnregisterTask( IThreadTask *pTask );
	VIRTUAL void SetMaxThreadCount( int nMaxThreads );
	VIRTUAL void SetThreadName( unsigned int dwThreadId,const char *sThreadName );
	VIRTUAL const char* GetThreadName( unsigned int dwThreadId );
	VIRTUAL uint32 GetThreadByName( const char* sThreadName );

	// Thread pool framework
	VIRTUAL ThreadPoolHandle CreateThreadsPool(const ThreadPoolDesc& desc);
	VIRTUAL const bool DestroyThreadsPool(const ThreadPoolHandle& handle);
	VIRTUAL const bool GetThreadsPoolDesc(const ThreadPoolHandle handle, ThreadPoolDesc* pDesc) const;
	VIRTUAL const bool SetThreadsPoolAffinity(const ThreadPoolHandle handle, const ThreadPoolAffinityMask AffinityMask);

	VIRTUAL void MarkThisThreadForDebugging(const char *name, bool bDump);
	//////////////////////////////////////////////////////////////////////////

	// This is on update function of the main thread.
	void OnUpdate();

	void AddSystemThread( uint32 nThreadId );
	void RemoveSystemThread( uint32 nThreadId );

	// Balancing tasks in the pool between threads
	void BalanceThreadsPool(const ThreadPoolHandle& handle);
	void BalanceThreadInPool(CThreadTask_Thread* pFreeThread, Threads* pThreads = NULL);

private:
	void ScheduleTask( SThreadTaskInfo *pTaskInfo );
	void RescheduleTasks();
private:

	// User created threads pools
	mutable CryReadModifyLock m_threadsPoolsLock;
	ThreadsPools m_threadsPools;

	// Physical threads available to system.
	Threads m_threads;

	// Threads with single blocking task attached.
	Threads m_blockingThreads;

	typedef CryMT::CLocklessPointerQueue<SThreadTaskInfo> Tasks;

	Tasks	m_unassignedTasks;

	mutable CryCriticalSection m_threadNameLock;
	typedef std::map<uint32,string> ThreadNames;
	ThreadNames m_threadNames;

	mutable CryCriticalSection m_systemThreadsLock;
	std::vector<uint32> m_systemThreads;

	// Max threads that can be executed at same time.
	int m_nMaxThreads;

};


#endif __thread_task_h__
