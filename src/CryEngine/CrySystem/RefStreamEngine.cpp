#include "StdAfx.h"
#include <ILog.h>
#include "RefStreamEngine.h"
#include "RefReadStream.h"
#include "RefReadStreamProxy.h"
#include "System.h"
#include "IConsole.h"
#include "BitFiddling.h"
//#define DEBUG_TRACE_CALLBACKS

#if !defined(PS3) 
#  define MAX_QUEUE_LENGTH 4*1024




#endif 

//////////////////////////////////////////////////////////////////////////
// useWorkerThreads is the number of worker threads  to use;
// currently only values 0 and 1 are supported: 0 - overlapped IO in the main thread, and 1 - overlapped IO in the worker thread
// MT: Main thread only
CRefStreamEngine::CRefStreamEngine (CCryPak* pPak, IMiniLog* pLog, ISystem * pSystem, unsigned useWorkerThreads, bool bOverlappedIO):
	m_pPak(pPak),
	m_pLog(pLog),
  m_pSystem(pSystem),
	m_nMaxReadDepth (1),
	m_nMaxQueueLength (MAX_QUEUE_LENGTH),
	m_nMaxIOMemPool (128*1024*1024),
#ifndef REFSTREAMENGINE_USE_CryThread
 #if defined(LINUX)
	m_hIOWorker (INVALID_HANDLE_VALUE),//only diff is here, but what can i do?
 #else
	m_hIOWorker (NULL),
 #endif
	m_dwWorkerThreadId(0),
#else
	m_pMainThread(NULL),
	m_pWorkerThread(NULL),
#endif
	m_bEnableOverlapped (bOverlappedIO),
	m_nSuspendCallbackTimeQuota(0),
	m_hThreadsPool(-1)
{
#ifdef REFSTREAMENGINE_USE_CryThread
	m_pMainThread = Self();
#else
	m_dwMainThreadId = GetCurrentThreadId();
#endif
	CheckOSCaps();

  m_queIOJobs.reserve(32);
  m_queIOExecuted.reserve(32);


	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&m_nPerfFreq))
	{
		m_nPerfFreq = 0;
		m_nSuspendCallbackTimeQuota = 1; // suspend forever
	}

	m_nCallbackTimeQuota.SetMilliSeconds(0);
	SetCallbackTimeQuota (50000);

	// reset current statistics
	m_Statistics.m_fDeltaTime = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();
	m_Statistics.m_nBytesRead = 0;

#ifndef REFSTREAMENGINE_USE_CryThread
	m_hIOJob = CreateEvent (NULL, FALSE, FALSE, NULL);
	m_hIOExecuted = CreateEvent (NULL, TRUE, FALSE, NULL);
	m_hDummyEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
#else
	m_flagIOJob = false;
	m_flagIOExecuted = false;
#endif
	memset (m_nSectorSizes, 0, sizeof(m_nSectorSizes));

#if !defined(LINUX)
	if (useWorkerThreads)
  {
		StartWorkerThread();
    // create thread pool
    m_descThreadPool.AffinityMask = 0;









		m_descThreadPool.AffinityMask |= 1 << 1;

    m_descThreadPool.nPriority = (uint32)THREAD_PRIORITY_BELOW_NORMAL; 
    ResizeThreadsPool(&m_descThreadPool);
  }
#endif
	// register system listener
	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
// MT: Main thread only
CRefStreamEngine::~CRefStreamEngine()
{
	if(m_hThreadsPool >= 0)	// destroy threads pool
	{
		m_descThreadPool.AffinityMask = INVALID_AFFINITY;
		ResizeThreadsPool(NULL);
	}

	StopWorkerThread();

	//m_setLockedStreams.clear();

	// fail all outstanding requests
	// we don't need to lock, since there's already no worker thread at this moment
	while (!m_queIOJobs.empty())
	{
		IReadStreamPtr pJob = m_queIOJobs.front();
    m_queIOJobs.erase(m_queIOJobs.begin()); 

		pJob->OnFinishRead(ERROR_ABORTED_ON_SHUTDOWN); // aborted
#ifdef REFSTREAMENGINE_USE_CryThread
		m_lockNotify.Lock();
		m_flagIOJob = true;
		m_condIOJob.Notify();
		m_lockNotify.Unlock();
#else
		SetEvent (m_hIOJob);
#endif
	}

	// finalize all the jobs that can be finalized
	while (!m_queIOExecuted.empty() && FinalizeIOJobs(FLAGS_DISABLE_CALLBACK_TIME_QUOTA) > 0)
		continue;

	// unregister system listener
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);

#ifndef REFSTREAMENGINE_USE_CryThread
	CloseHandle (m_hIOJob);
	CloseHandle (m_hIOExecuted);
	CloseHandle (m_hDummyEvent);
#endif
}

unsigned CRefStreamEngine::UpdateAndWait (unsigned nMilliseconds, unsigned nFlags)
{
	LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);
	if (IsMainThread())
		Update(nFlags);
	else
		Sleep(10);
	return Wait (nMilliseconds,nFlags);
}

// returns true if called from the main thread for this engine
bool CRefStreamEngine::IsMainThread()
{
#ifdef REFSTREAMENGINE_USE_CryThread
	return Self() == m_pMainThread;
#else
	return GetCurrentThreadId() == m_dwMainThreadId;
#endif
}

bool CRefStreamEngine::IsWorkerThread()
{
#ifdef REFSTREAMENGINE_USE_CryThread
	return Self() == m_pWorkerThread;
#else
	return GetCurrentThreadId() == m_dwWorkerThreadId;
#endif
} 

//////////////////////////////////////////////////////////////////////////
// Starts asynchronous read from the specified file
// MT: Main thread only
IReadStreamPtr CRefStreamEngine::StartRead (const StreamTaskType tSource, const char* szFilePathPC, IStreamCallback* pCallback, StreamReadParams* pParams)
{
	unsigned nFlags = 0;
	if (pParams)
		nFlags = pParams->nFlags;

	//////////////////////////////////////////////////////////////////////////
	// For testing. (When this flag is not set, StartRead is not always performed asynchronously.
	nFlags |= SRP_QUICK_STARTREAD;
	//////////////////////////////////////////////////////////////////////////

	// get rid of some jobs if there are too many in the queue
	if (!(nFlags & SRP_QUICK_STARTREAD))
	{
		while (GetNumJobs(ejtStarted) >= m_nMaxQueueLength)
		{
			m_pLog->LogWarning("StreamEngine: The number of jobs waiting %d >= max queue length %d, waiting to free up the queue", (int)GetNumJobs(ejtStarted), m_nMaxQueueLength);
			UpdateAndWait(20, FLAGS_DISABLE_CALLBACK_TIME_QUOTA);
		}
	}

	// at this moment the stream should self-register in this engine and the stream sets should get initialized
	char szFilePathBuf[CCryPak::g_nMaxPath];
	const char* szFilePath = NULL;

	// first try to find such file; if it's already pending, add a client to it only
	if(szFilePathPC)
	{
		m_pPak->RecordFile(NULL, szFilePathPC);
		szFilePath = m_pPak->AdjustFileName (szFilePathPC, szFilePathBuf, pParams && (pParams->nFlags & SRP_FLAGS_PATH_REAL) ? ICryPak::FLAGS_PATH_REAL: 0);
	}

	IReadStreamPtr pStream = new CRefReadStreamProxy(tSource, new CRefReadStream(szFilePath, this), pCallback, pParams);

	// update statistics
	UpdateStatistics(pStream);

	// register the stream
	AddIOJob (pStream);

	if (!(nFlags & SRP_QUICK_STARTREAD))
		Update(0);

	return pStream;
}

// signals that this stream needs to be executed (StartRead called)
void CRefStreamEngine::AddIOJob (IReadStreamPtr pJobStream)
{
	if (!HaveIOWorkerThread())
	{ // its very simple with single-threaded model: we just put the job to the queue
		// for the next update will execute it
		m_queIOJobs.push_back(pJobStream);
		SortIOJobs_NoLock();
	}
	else
	// for multi-threaded model, we need to put the job to the queue and signal the worker
	// thread about it.
	{
		// put to the queue
		{
			AUTO_LOCK_CS(m_csIOJobs);
			READ_WRITE_BARRIER
			m_queIOJobs.push_back(pJobStream);
			SortIOJobs_NoLock();
			READ_WRITE_BARRIER
		}
#ifdef REFSTREAMENGINE_USE_CryThread
		m_lockNotify.Lock();
		m_flagIOJob = true;
		m_condIOJob.Notify();
		m_lockNotify.Unlock();
#else
		SetEvent (m_hIOJob);
#endif
	} 
} 

//////////////////////////////////////////////////////////////////////////
// returns the size of the file; returns 0 if there's no such file.
unsigned CRefStreamEngine::GetFileSize (const char* szFilePathPC, unsigned nCryPakFlags)
{
	if(!szFilePathPC)
		return 0;

	char szFilePathBuf[m_pPak->g_nMaxPath];
	const char *szFilePath = m_pPak->AdjustFileName (szFilePathPC, szFilePathBuf, nCryPakFlags);

	// we didn't find the file size in the cache - open the file and query the size
#if defined(LINUX)
	HANDLE hFile = CreateFile (szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
#else
	HANDLE hFile = CreateFile (szFilePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
#endif
	if (hFile != INVALID_HANDLE_VALUE)
	{
		unsigned nFileSize = ::GetFileSize(hFile, NULL);
		CloseHandle (hFile);
		return nFileSize;
	}
	else
	{
		CCachedFileDataPtr pFileData = m_pPak->GetFileData(szFilePath);
		if (pFileData)
			return pFileData->GetFileEntry()->desc.lSizeUncompressed;
		else
		{
			m_pPak->OnMissingFile(szFilePathPC);
			return 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Gets called regularly, to finalize those proxies whose jobs have
// already been executed (e.g. to call the callbacks)
//  - to be called from the main thread only
//  - starts new jobs in the single-threaded model
void CRefStreamEngine::Update(unsigned nFlags)
{
  FUNCTION_PROFILER( m_pSystem, PROFILE_SYSTEM );
	LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

	unsigned numRemovedJobs = 0;
	unsigned numFinalizedJobs = 0;
	do {
	
		if (!HaveIOWorkerThread())
		{
			// If we're in single-threaded mode, update means the whole cycle:
			// start the jobs, wait for their IO completion routine and finalize them
			numRemovedJobs = StartIOJobs();

			// enter alertable state so that the IO completion routines can be called
			WaitForSingleObjectEx(m_hDummyEvent, 0, TRUE);
		}
		
		// If we're in multi-threaded mode, Update from main thread means
		// just finalization of executed in the worker IO jobs.
		numFinalizedJobs = FinalizeIOJobs(nFlags);

		// Update from worker thread shouldn't be called at all
		
		// We continue updating until all the jobs that are possible to move out
		// of the IO Job queue are moved. Even if no jobs were moved, but some were finalized,
		// the finalized jobs may have spawned some new jobs, we'll try to start those, too
		// but we don't let it go on forever because of the limits
	} while(numRemovedJobs | numFinalizedJobs);
}

//////////////////////////////////////////////////////////////////////////
// Only waits at most the specified amount of time for some IO to complete
// - may initiate a new job
// - may be called from any non-worker thread
unsigned CRefStreamEngine::Wait(unsigned nMilliseconds, unsigned nFlags)
{
	// for stream->Wait sync
	LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);
	if(!(nFlags & FLAGS_SILENT_WAIT))
	{
#ifdef REFSTREAMENGINE_USE_CryThread
		m_flagIOExecuted = false;
#else
		ResetEvent (m_hIOExecuted);
#endif
	}

	if (!IsMainThread())
	{
		// special case - this function is called from non-main thread
		// just wait until some io gets executed, if there's anything to wait for
		// no sense to wait here if there are no waiting or pending jobs
		if (GetNumJobs(ejtStarted|ejtPending) > 0)
		{
#ifdef REFSTREAMENGINE_USE_CryThread
			m_lockNotify.Lock();
			if (!m_flagIOExecuted)
				m_condIOExecuted.TimedWait(m_lockNotify, nMilliseconds);
			m_lockNotify.Unlock();
#else
			WaitForSingleObject(m_hIOExecuted, nMilliseconds);
#endif
		}
		return 0;
	}

	SetCallbackTimeQuota (nMilliseconds * 1000);

	if (HaveIOWorkerThread())
	{
		unsigned nFinalized = FinalizeIOJobs(nFlags); // finalize whatever may not have been finalized
		if (nFinalized)
			return nFinalized; // we don't wait if we finalized something
		// no sense to wait here if there are no waiting or pending jobs
		if (GetNumJobs(ejtStarted|ejtPending) > 0)
		{
#ifdef REFSTREAMENGINE_USE_CryThread
			m_lockNotify.Lock();
			if (!m_flagIOExecuted)
				m_condIOExecuted.TimedWait(m_lockNotify, nMilliseconds);
			m_lockNotify.Unlock();
#else
			WaitForSingleObject(m_hIOExecuted, nMilliseconds);
#endif
		}
	}
	else
	{
		// really wait for some IO to complete
		if (GetNumJobs(ejtPending) > 0) // no sense to wait here if there are no pending jobs
			SleepEx(nMilliseconds, TRUE);
		StartIOJobs(); // perhaps there's room for new tasks to be started now
	}
	return FinalizeIOJobs(nFlags);
}

// adds to the callback time quota in this frame, the specified number of microseconds
void CRefStreamEngine::AddCallbackTimeQuota (int nMicroseconds)
{
	m_nCallbackTimeQuota.SetMilliSeconds( (int64)(m_nCallbackTimeQuota.GetMilliSeconds() + nMicroseconds/1000.0f) );
}

void CRefStreamEngine::SetCallbackTimeQuota (int nMicroseconds)
{
	// if we have enough qouta then just  set it, else add it
	//if (m_nCallbackTimeQuota.GetValue() < 0)
	//	AddCallbackTimeQuota(nMicroseconds);
	//else
		m_nCallbackTimeQuota.SetMilliSeconds( nMicroseconds / 1000 );
}


bool CRefStreamEngine::IsCallbackTimeQuota(unsigned nFlags)
{
	if (m_nSuspendCallbackTimeQuota == 0
		&& !(nFlags&FLAGS_DISABLE_CALLBACK_TIME_QUOTA)
		&& m_nCallbackTimeQuota.GetValue() <= 0
		)
		return false;
	return true;
}

// In the Multi-Threaded model (with the IO Worker thread)
// removes the proxies from the IO Queue as needed, and the proxies may call their callbacks
unsigned CRefStreamEngine::FinalizeIOJobs(unsigned nFlags)
{
  FUNCTION_PROFILER( m_pSystem, PROFILE_SYSTEM );
	LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

	unsigned numFinalizedJobs = 0;
	// we fetch the executed jobs one-by-one, and finalize them
	// during finalization, the queue itself may be changed
	// just ignore callback quota for XBox
	// because of the streaming engine has a separate HW thread for itself
	
#ifdef DEBUG_TRACE_CALLBACKS
	std::vector<std::pair<float, string> > ñallStack;
	float totalTime = 0;
#endif

	if (!IsCallbackTimeQuota(nFlags))
		return 0;

	AUTO_LOCK_CS(m_csIOExecuted);
	{
		IReadStreamPtr pStream;
		while (!m_queIOExecuted.empty())
		{
			pStream = m_queIOExecuted.front();
      m_queIOExecuted.erase(m_queIOExecuted.begin()); 

			// to avoid locking the whole array during execution of the callbacks:
			AUTO_UNLOCK_CS(m_csIOExecuted);

			CTimeValue nStartTime, nEndTime;
			nStartTime = gEnv->pTimer->GetAsyncTime();
			pStream->FinalizeIO ();
			++numFinalizedJobs;
			nEndTime = gEnv->pTimer->GetAsyncTime();

			m_nCallbackTimeQuota -= nEndTime - nStartTime;

	#ifdef DEBUG_TRACE_CALLBACKS
			totalTime += nEndTime.GetMilliSeconds() - nStartTime.GetMilliSeconds();
			ñallStack.push_back(std::make_pair(nEndTime.GetMilliSeconds() - nStartTime.GetMilliSeconds(), pStream->GetName()));
	#endif

			if (!IsCallbackTimeQuota(nFlags))
				break;
		}
	}

	if(numFinalizedJobs)
	{
#ifdef REFSTREAMENGINE_USE_CryThread
		m_lockNotify.Lock();
		m_flagIOJob = true;
		m_condIOJob.Notify();
		m_lockNotify.Unlock();
#else
		SetEvent (m_hIOJob);
#endif
	}

#ifdef DEBUG_TRACE_CALLBACKS
	if(totalTime > 10.f)
	{
		gEnv->pLog->LogWarning("Streaming callbacks:");
		for(std::vector<std::pair<float, string> >::iterator it = ñallStack.begin();it != ñallStack.end();++it)
			gEnv->pLog->LogWarning("  Stream: %s, time: %.1fms", it->second, it->first);
	}
#endif

	return numFinalizedJobs;
}


// this will be the thread that executes everything that can take time
void CRefStreamEngine::IOWorkerThread ()
{
	do
	{
		// we start whatever IO jobs we have in the queue
		StartIOJobs();
		// we wait for new jobs to arrive or for the IO callbacks to be called
		// even if it was a callback, we check for the new jobs: some jobs may have
		// been suspended because of performance reasons, until the next callback;
		// besides, the callback might have spawned some new jobs.
		// the pending->executed move will happen in the callback
#ifdef REFSTREAMENGINE_USE_CryThread
		m_lockNotify.Lock();
		while (!m_flagIOJob)
		{
			m_condIOJob.Wait(m_lockNotify);
		}
		m_flagIOJob = false;
		m_lockNotify.Unlock();
#else
		WaitForSingleObjectEx(m_hIOJob, INFINITE, TRUE);
#endif
	}
	while (!m_bStopIOWorker);
	// wait for pending IO
	AUTO_LOCK_CS(m_csIOPending);
	for (int nRetries = 0; nRetries < 100 && !m_setIOPending.empty(); ++nRetries)
	{
		AUTO_UNLOCK_CS(m_csIOPending);
		SleepEx(300, TRUE);
	}
}


// sort the IO jobs in the IOQueue by priority
void CRefStreamEngine::SortIOJobs()
{
	if (HaveIOWorkerThread())
	{
		AUTO_LOCK_CS(m_csIOJobs);
		SortIOJobs_NoLock();
	}
	else
	{
		SortIOJobs_NoLock();
	}
}


//////////////////////////////////////////////////////////////////////////
// this sorts the IO jobs, without bothering about synchronization
void CRefStreamEngine::SortIOJobs_NoLock()
{
	std::stable_sort (m_queIOJobs.begin(), m_queIOJobs.end(), IReadStream::Order());
}

bool CRefStreamEngine::IsSuspended()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
// 
unsigned CRefStreamEngine::StartIOJobs()
{
  FUNCTION_PROFILER( m_pSystem, PROFILE_SYSTEM );

	unsigned numMovedJobs = 0;

	AUTO_LOCK_CS(m_csIOJobs);
	{
		AUTO_LOCK_CS(m_csIOPending);
		{
			IReadStreamPtr pEndJob = NULL; // the job that will mark the end of loop
			READ_WRITE_BARRIER

			// TODO: implement limitation on the number of simultaneous read requests
			while(!m_queIOJobs.empty() && m_setIOPending.size() < m_nMaxReadDepth && m_queIOExecuted.size() <= max((size_t)1, (size_t)CountBits(m_descThreadPool.AffinityMask)) && !IsSuspended())
			{
				IReadStreamPtr pStream = m_queIOJobs.front();
        m_queIOJobs.erase(m_queIOJobs.begin()); 

				m_setIOPending.insert (pStream);

				// temporary unlock both queue and set and start the reading
				bool bReadStarted;
				{
					AUTO_UNLOCK_CS(m_csIOPending);
					{
						AUTO_UNLOCK_CS(m_csIOJobs);
						// try to start reading
						bReadStarted = pStream->StartRead();
						Sleep(g_cvars.sys_streaming_sleep); // For testing.
					}
				}

				if (bReadStarted)
				{
					// in case of no error - this should be in most cases:
					// we started the operation successfully
					// perhaps now it's even already read and moved from Pending to Executed queue

					// in case of unrecoverable error:
					// we didn't start reading and can't do so. It's already moved into Executed queue as errorneous
					++numMovedJobs;

					pEndJob = NULL; // start the whole loop all over again.
				}
				else
				{
					// recoverable error - we'll try again next time
					m_queIOJobs.push_back(pStream);
					m_setIOPending.erase (pStream);

					if (pEndJob)
					{
						if (pEndJob == pStream)
							break; // we are looping - we can't start this job for the second time in a row now
					}
					else
					{
						pEndJob = pStream; // mark this job as the end of loop; we'll erase the marker if the job is started
					}
				}
			}
		}
		READ_WRITE_BARRIER
	}
	return numMovedJobs;
}

void CRefStreamEngine::OnIOJobFinishedRead( IReadStreamPtr pJobStream )
{
	{
		AUTO_LOCK_CS(m_csIOPending);
		{
			READ_WRITE_BARRIER
			m_setIOPending.erase (pJobStream);
			READ_WRITE_BARRIER
		}
	}

	// perhaps this will free the way for another IO job.
	// but we won't call StartIOJobs(), because this same funciton can only be
	// called as part of Waiting after which the caller is aware that some jobs may have been executed
#ifdef REFSTREAMENGINE_USE_CryThread
	m_lockNotify.Lock();
	m_flagIOJob = true;
	m_condIOJob.Notify();
	m_lockNotify.Unlock();
#else
	SetEvent (m_hIOJob);
#endif

}

void CRefStreamEngine::OnIOJobExecuted (IReadStreamPtr pJobStream)
{
	if (HaveIOWorkerThread() && (pJobStream->GetParams().nFlags & SRP_FLAGS_ASYNC_CALLBACK))
		pJobStream->FinalizeIO();

	{
		AUTO_LOCK_CS(m_csIOExecuted);
		{
			READ_WRITE_BARRIER
			m_queIOExecuted.push_back(pJobStream);
			READ_WRITE_BARRIER
		}
	}

#ifdef REFSTREAMENGINE_USE_CryThread
	m_lockNotify.Lock();
	m_flagIOExecuted = true;
	m_condIOExecuted.Notify();
	m_lockNotify.Unlock();
#else
	SetEvent (m_hIOExecuted);
#endif

}

void CRefStreamEngine::StopWorkerThread()
{
	if (HaveIOWorkerThread())
	{
		m_bStopIOWorker = true;
#ifdef REFSTREAMENGINE_USE_CryThread
		m_lockNotify.Lock();
		m_flagIOJob = true;
		m_condIOJob.Notify();
		m_lockNotify.Unlock();
		WaitForThread();
#else
		SetEvent(m_hIOJob);
		WaitForSingleObject (m_hIOWorker, INFINITE);
		CloseHandle (m_hIOWorker);
		m_hIOWorker = NULL;
#endif
	}
}

void CRefStreamEngine::StartWorkerThread()
{
	StopWorkerThread();
	m_bStopIOWorker = false;
#ifdef REFSTREAMENGINE_USE_CryThread
	Start(0, "Streaming Engine", THREAD_PRIORITY_NORMAL, (SIMPLE_THREAD_STACK_SIZE_KB)*1024);
	m_pWorkerThread = this;
#else
	m_hIOWorker = CreateThread (NULL, 0x8000,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(IOWorkerThreadProc), 
			this, CREATE_SUSPENDED, &m_dwWorkerThreadId);







	if (m_dwWorkerThreadId)
	{
		CryThreadSetName( m_dwWorkerThreadId,"Streaming" );

	}
	if(ResumeThread(m_hIOWorker) == -1)
	{
		assert(0);
	}
#endif
}

// this function checks for the OS version and disables some capabilities of Streaming Engine when needed
// currently, in Win 9x overlapped IO is disabled
void CRefStreamEngine::CheckOSCaps()
{
#if defined(WIN32)
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(os);
	if (!GetVersionEx(&os))
	{
		m_bEnableOverlapped = false; // just in case
		return;
	}

	if (os.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		m_pLog->LogWarning("StreamEngine: OS (platform %d) doesn't support streaming, turning overlapped IO off",os.dwPlatformId );
		// only NT supports overlapped IO
		m_bEnableOverlapped = false;
	}
#elif defined(_XBOX) || defined(LINUX) || defined(PS3)
	// in XBox, nothing to disable
	// PS3 supports overlapped I/O
#else
#error // if your OS doesn't support it, you should disable Overlapped IO here
	m_bEnableOverlapped = false;
#endif
}

//! Puts the memory statistics into the given sizer object
//! According to the specifications in interface ICrySizer
void CRefStreamEngine::GetMemoryStatistics(ICrySizer *pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "CRefStreamEngine");

	size_t nSize = sizeof(*this);

	// here we calculate the capacities of 3 arrays; we don't want a deadlock so we lock them one by one, 
	// small discrepancies because something can be moved somewhere don't matter
	{
		AUTO_LOCK_CS(m_csIOJobs);
		nSize += m_queIOJobs.size() * sizeof(IReadStream_AutoPtr_AutoDeque_MT::value_type);
	}

	{
		AUTO_LOCK_CS(m_csIOPending);
		nSize += m_setIOPending.size() * sizeof(IReadStream_AutoPtr_AutoSet_MT::value_type);
	}

	{
		AUTO_LOCK_CS(m_csIOExecuted);
		nSize += m_queIOExecuted.size() * sizeof(IReadStream_AutoPtr_AutoDeque_MT::value_type);
	}

	pSizer->AddObject(this, nSize);
}

//! Enables or disables callback time quota per frame
void CRefStreamEngine::SuspendCallbackTimeQuota ()
{
	if (m_nPerfFreq > 0)
	{
		++m_nSuspendCallbackTimeQuota;
	}
}
void CRefStreamEngine::ResumeCallbackTimeQuota()
{
	if (m_nPerfFreq > 0)
	{
		--m_nSuspendCallbackTimeQuota;
		if (m_nSuspendCallbackTimeQuota == 0 && m_nCallbackTimeQuota.GetValue() < 0)
		{
			m_nCallbackTimeQuota.SetValue(0);
		}
	}
}

ThreadPoolHandle CRefStreamEngine::GetThreadsPool() const
{
	return m_hThreadsPool;
}

void CRefStreamEngine::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			if (gEnv->pSystem->IsEditor())
				return;
			m_descThreadPool.AffinityMask = 0;
      if(HaveIOWorkerThread())
      {







        m_descThreadPool.AffinityMask |= 1 << 1;
        m_descThreadPool.AffinityMask |= 1 << 2;
        m_descThreadPool.AffinityMask |= 1 << 3;

        m_descThreadPool.nPriority = THREAD_PRIORITY_NORMAL;
      }
			ResizeThreadsPool(&m_descThreadPool);
			break;
		}
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			if (gEnv->pSystem->IsEditor())
				return;

			m_descThreadPool.AffinityMask = 0;
      if(HaveIOWorkerThread())
      {








			  m_descThreadPool.AffinityMask |= 1 << 0;
			  m_descThreadPool.AffinityMask |= 1 << 1;

			  m_descThreadPool.nPriority = THREAD_PRIORITY_BELOW_NORMAL;
      }
			ResizeThreadsPool(&m_descThreadPool);
      break;
    }
	}
}

void CRefStreamEngine::ResizeThreadsPool( ThreadPoolDesc* pPoolDesc )
{
	// destroy old pool if exists
	if(m_hThreadsPool >= 0)
	{
		// wait for all tasks in the pool
		AUTO_LOCK_CS(m_csIOJobs);
		int quota = 10000;
		while(!m_queIOExecuted.empty() || !m_queIOJobs.empty())
		{
			UpdateAndWait(100, FLAGS_DISABLE_CALLBACK_TIME_QUOTA);
			quota -= 100;
			if(quota < 0)
			{
				gEnv->pLog->LogError("[Error] Stream engine: Unable to stop streaming task, just dropping it");
				break;
			}
		}

		// destroy the pool
		AUTO_LOCK_CS(m_csIOPending);
		AUTO_LOCK_CS(m_csIOExecuted);
		gEnv->pSystem->GetIThreadTaskManager()->DestroyThreadsPool(m_hThreadsPool);
		m_hThreadsPool = -1;
	}

	// create a threads pool for async callbacks
	if(HaveIOWorkerThread() && pPoolDesc && pPoolDesc->AffinityMask != 0)	// only if we have async streaming engine
	{
		pPoolDesc->sPoolName = "StreamingPool";
		m_hThreadsPool = gEnv->pSystem->GetIThreadTaskManager()->CreateThreadsPool(*pPoolDesc);
	}
}

size_t CRefStreamEngine::GetNumJobs( uint32 jobType )
{
	size_t numJobs = 0;
	if(jobType & ejtStarted)
	{
		AUTO_LOCK_CS(m_csIOJobs);
		numJobs += m_queIOJobs.size();
	}
	if(jobType & ejtPending)
	{
		AUTO_LOCK_CS(m_csIOPending);
		numJobs += m_setIOPending.size();
	}
	if(jobType & ejtFinished)
	{
		AUTO_LOCK_CS(m_csIOExecuted);
		numJobs += m_queIOExecuted.size();
	}
	return numJobs;
}

void CRefStreamEngine::AbortJob( IReadStreamPtr pStream )
{
	bool bFound = false;

	// 1 phase - if we still have this job in the queue
	{
		AUTO_LOCK_CS(m_csIOJobs);
		IReadStream_AutoPtr_AutoDeque_MT::iterator it = std::find(m_queIOJobs.begin(), m_queIOJobs.end(), pStream);
		if(it != m_queIOJobs.end())
		{
			m_queIOJobs.erase(it);
			bFound = true;
		}
	}

	// 2 phase - if it's in the pending queue - cancel I/O
	if(!bFound)
	{
		AUTO_LOCK_CS(m_csIOPending);
		IReadStream_AutoPtr_AutoSet_MT::iterator it = m_setIOPending.find(pStream);
		if(it != m_setIOPending.end())
		{
			m_setIOPending.erase(it);
			bFound = true;
		}
	}

	// 3 phase - if it's already finished I/O and still not calling the callback now
	if(!bFound)
	{
		AUTO_LOCK_CS(m_csIOExecuted);
		IReadStream_AutoPtr_AutoDeque_MT::iterator it = std::find(m_queIOExecuted.begin(), m_queIOExecuted.end(), pStream);
		if(it != m_queIOExecuted.end())
		{
			m_queIOExecuted.erase(it);
			bFound = true;
		}
	}
	
	// otherwise it's already done
}

void CRefStreamEngine::GetStreamingStatistics( IStreamEngine::SStatistics& stats )
{
	AUTO_LOCK_CS(m_csStatistics);

	// fill statistics
	stats.m_nBytesRead = m_Statistics.m_nBytesRead;
	stats.m_fDeltaTime = gEnv->pTimer->GetAsyncTime().GetMilliSeconds() - m_Statistics.m_fDeltaTime;
	stats.vecHeavyAssets = m_Statistics.vecHeavyAssets;

	// reset current statistics
	m_Statistics.m_fDeltaTime = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();
	m_Statistics.m_nBytesRead = 0;
	m_Statistics.vecHeavyAssets.clear();
}

void CRefStreamEngine::UpdateStatistics( IReadStreamPtr pJobStream )
{
	AUTO_LOCK_CS(m_csStatistics);
	m_Statistics.m_nBytesRead += pJobStream->GetParams().nSize;

#define MAX_ASSETS 20	// max assets to collect
	if(pJobStream->GetParams().nSize > 64*1024 && pJobStream->GetFileSize() != 0)
	{
		m_Statistics.vecHeavyAssets.push_back(SStatistics::SAsset(pJobStream->GetName(), pJobStream->GetParams().nSize));
		std::sort(m_Statistics.vecHeavyAssets.begin(), m_Statistics.vecHeavyAssets.end());
		if(m_Statistics.vecHeavyAssets.size() > MAX_ASSETS)
			m_Statistics.vecHeavyAssets.resize(MAX_ASSETS);
	}
}
///////////////////////////////////////////////////////////////////////////
CStreamingTask::CStreamingTask(IReadStreamPtr pStream) : m_pStream(pStream)
{
	assert(0!=m_pStream);
}

// all methods must be executed in the SAME thread
CStreamingTask::~CStreamingTask()
{
}

void CStreamingTask::OnUpdate()
{
	FUNCTION_PROFILER( gEnv->pSystem, PROFILE_SYSTEM );
	gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging("Streaming",true);

	{
		AUTO_LOCK(m_pStream->GetCallbackLock());
		if(!m_pStream->IsFinished() && !m_pStream->IsError())
		{
			if((m_pStream->GetParams().nFlags & SRP_FLAGS_USER_DECOMPRESSION_IN_CALLBACKS) == 0)
				m_pStream->Decompress();
		}
		if((m_pStream->GetParams().nFlags & SRP_FLAGS_FORCE_SYNC_CALLBACKS) == 0)
			m_pStream->ExecuteAsyncCallback();
	}
	gEnv->pSystem->GetStreamEngine()->OnIOJobExecuted(m_pStream);
	gEnv->pSystem->GetIThreadTaskManager()->UnregisterTask(this);
	gEnv->pSystem->GetIThreadTaskManager()->MarkThisThreadForDebugging("Streaming",false);
	delete this;
}

void CStreamingTask::Stop()
{

}
