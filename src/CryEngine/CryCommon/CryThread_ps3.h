/////////////////////////////////////////////////////////////////////////////
//
// Crytek Source File
// Copyright (C), Crytek Studios, 2001-2007.
//
// Description: Implementation of the CryThread API for PS3.
//
// History:
// Jun 22, 2007: Created by Sascha Demetrio
// Jan 30, 2008: New PPU <-> SPU aware implementation (Sascha Demetrio).
//
/////////////////////////////////////////////////////////////////////////////

#if !defined __CRYTHREAD_PS3_H__
#define __CRYTHREAD_PS3_H__ 1

//#define PS3_USE_POSIX_LOCKS 1
#undef PS3_USE_POSIX_LOCKS




#if defined __CRYCG__
	#include <PS3CryCache.h>
#endif













// Define this to enable lock debugging.  This macro enables runtime sanity
// checks.  With lock debugging, the owner of the lock is always stored in the
// lock instances (even for fast locks).
//#undef CRYTHREAD_DEBUG_LOCKS
#define CRYTHREAD_DEBUG_LOCKS 1

// These headers are not needed when compiling 
// for SPU 
#if !defined __SPU__
  #include <sys/synchronization.h>
  #include <sys/sys_time.h>
#endif

// Implementation of locks and conditions that work on PS3.
//
// These locks and conditions are suitable for synchronization between PPU and
// SPU.

#undef PPU_SYS

#if defined CRYTHREAD_DEBUG_LOCKS

extern void _CryLockError();

#define PPU_SYS(CALL) do { \
	int _err = (CALL); \
	if (_err != 0) \
	{ \
		fprintf(stderr, \
				"PPU_SYS: " __FILE__ ", line %d: " \
				"call '%s' returned error code 0x%08x\n", \
				__LINE__, #CALL, _err); \
		_CryLockError(); \
	} \
} while (false)
#else
#define PPU_SYS(CALL) ((void)(CALL))
#endif

// Common base class for fast and recursive locks.
//
// The base class contains all the data fields, the fast lock will not use the
// 'm_Counter' and 'm_Owner' fields of the lock.
//
// The size of a lock is always 128 bytes, because no other data may be within
// the same cache line.
class __attribute__ ((aligned (128))) CryLockBase
{
protected:
	// Atomic state variable.
	//  0 = free.
	//  1 = locked, no waiters on PPU.
	// -1 = locked, waiters on PPU are possible.
	long m_LockState;

	// Recursion counter.
	uint32 m_Counter;

	// The thread owner (as returned by GetCurrentThreadId()).
	uint32 m_Owner;

	int32 m_Padding1[1];

	// m_Mutex: A system mutex (sys_mutex_t).  Initialized to ~0U, will be created on
	//          demand (i.e. when a PPU thread is being blocked on the lock).
	// m_Cond:  A system condition (sys_mutex_t).  Initialized to ~0U and created
	//          together with the mutex (on demand).
	//avoid breaking strict aliasing rules
	union 
	{
		struct  
		{
			uint32 m_Mutex;
			uint32 m_Cond;
		};
		int64 mutCondLongVal;
	} mutCond;

	// Padding to make the lock 128 bytes large.
	int32 m_Padding2[26];

#if !defined __SPU__
	// Create the system mutex and condition.
	//
	// The method returns the system mutex ID.  The method will create the
	// system mutex on demand.  When the method returns, both m_Mutex and m_Cond
	// are initialized.
	sys_mutex_t Init()
	{
		if (mutCond.m_Mutex == ~0U)
		{
			sys_mutex_t mutex;
			sys_mutex_attribute_t mutex_attr;
			sys_mutex_attribute_initialize(mutex_attr);
			PPU_SYS(sys_mutex_create(&mutex, &mutex_attr));
			sys_cond_t cond;
			sys_cond_attribute_t cond_attr;
			sys_cond_attribute_initialize(cond_attr);
			PPU_SYS(sys_cond_create(&cond, mutex, &cond_attr));

			// Note: we're writing m_Mutex and m_Cond in a single atomic operation.
			if (CryInterlockedCompareExchange64(
						&mutCond.mutCondLongVal,
						((int64)mutex << 32) | cond,
						~0LL) == ~0LL)
			{
				return mutex;
			}
			else
			{
				PPU_SYS(sys_cond_destroy(cond));
				PPU_SYS(sys_mutex_destroy(mutex));
			}
		}
		return mutCond.m_Mutex;
	}
#endif

	// Send a notification to at least one of the PPU threads blocked on the
	// lock.
	//
	// This method is called only if a PPU thread may be blocked on the lock.
	void Notify()
	{
#if !defined __SPU__
		sys_mutex_t mutex = mutCond.m_Mutex;
		// Note: It is important to acquire the mutex before sending the signal.
		// Otherwise we might send the signal after the other thread has set the
		// state to -1 but _before_ it enters the wait call, possibly causing a
		// deadlock situation.
		PPU_SYS(sys_mutex_lock(mutex, 0));
		PPU_SYS(sys_cond_signal(mutCond.m_Cond));
		PPU_SYS(sys_mutex_unlock(mutex));


#endif
	}

	CryLockBase()
		: m_LockState(0), m_Counter(0), m_Owner(0)
	{
		mutCond.m_Mutex = ~0U;
		mutCond.m_Cond	= ~0U;



	}

	// If the destructor is executed on SPU and the system mutex and condition
	// have been created, then a destruction notification is sent to the PPU.
	~CryLockBase()
	{
		if (mutCond.m_Mutex != ~0U)
		{
#if !defined __SPU__
			PPU_SYS(sys_cond_destroy((sys_cond_t)mutCond.m_Cond));
			PPU_SYS(sys_mutex_destroy((sys_memory_t)mutCond.m_Mutex));


#endif
		}
	}

#ifdef _DEBUG
public:
	bool IsLocked() const { return m_LockState != 0; }
#endif
};

template <>
class __attribute__ ((aligned (128))) CryLockT<CRYLOCK_FAST>
	: public CryLockBase
{
	CryLockT(const CryLockT<CRYLOCK_FAST> &);
	CryLockT<CRYLOCK_FAST> &operator= (const CryLockT<CRYLOCK_FAST> &);

public:
	CryLockT() { }

#if !defined __SPU__
	void Lock()


#endif
	{
#if !defined __SPU__
		if (CryInterlockedCompareExchange(&m_LockState, 1, 0) != 0)
		{
			sys_mutex_t mutex = Init();
			sys_cond_t cond = mutCond.m_Cond;
			PPU_SYS(sys_mutex_lock(mutex, 0));
			while (CryInterlockedExchange(&m_LockState, -1) != 0)
				PPU_SYS(sys_cond_wait(cond, 0));
			PPU_SYS(sys_mutex_unlock(mutex));
		}
# if defined CRYTHREAD_DEBUG_LOCKS
		m_Owner = GetCurrentThreadId();
# endif
		__lwsync();


#endif
	}

	bool TryLock()
	{
#if !defined __SPU__
		if (CryInterlockedCompareExchange(&m_LockState, 1, 0) != 0)


#endif
			return false;
		else
		{
#if !defined __SPU__ && defined CRYTHREAD_DEBUG_LOCKS
			m_Owner = GetCurrentThreadId();
#endif
#if !defined __SPU__
			__lwsync();
#endif
			return true;
		}
	}

#if !defined __SPU__
	void Unlock()


#endif
	{
#if !defined __SPU__ && defined CRYTHREAD_DEBUG_LOCKS
		const uint32 currentThreadId = GetCurrentThreadId();
		if (m_Owner != currentThreadId)
		{
			// Dump a message to the console and continue.
			fprintf(stderr,
					"CryLockT<CRYLOCK_FAST>::Unlock(): "
					"caller (thread %u) is not owner (thread %u) of the lock\n",
					static_cast<unsigned>(currentThreadId),
					static_cast<unsigned>(m_Owner));
			_CryLockError();
		}
#endif
#if !defined __SPU__
		if (CryInterlockedExchange(&m_LockState, 0) < 0)


#endif
			Notify();
	}
};

template <>
class __attribute__ ((aligned (128))) CryLockT<CRYLOCK_RECURSIVE>
	: public CryLockBase
{
	CryLockT(const CryLockT<CRYLOCK_RECURSIVE> &);
	CryLockT<CRYLOCK_RECURSIVE> &operator= (const CryLockT<CRYLOCK_RECURSIVE> &);

public:
	CryLockT() { }

#if !defined __SPU__
	void Lock()


#endif
	{
		const uint32 currentThreadId = GetCurrentThreadId();

#if !defined __SPU__
		if (CryInterlockedCompareExchange(&m_LockState, 1, 0) == 0)
		{
			m_Counter = 1;
			m_Owner = currentThreadId;
		}
		else
		{
			if (currentThreadId == m_Owner)
				++m_Counter;
			else
			{
				sys_mutex_t mutex = Init();
				sys_cond_t cond = mutCond.m_Cond;
				PPU_SYS(sys_mutex_lock(mutex, 0));
				while (CryInterlockedExchange(&m_LockState, -1) != 0)
					PPU_SYS(sys_cond_wait(cond, 0));
				PPU_SYS(sys_mutex_unlock(mutex));
				m_Counter = 1;
				m_Owner = currentThreadId;
			}
		}














#endif
	}

	bool TryLock()
	{
		const uint32 currentThreadId = GetCurrentThreadId();

#if !defined __SPU__
		if (CryInterlockedCompareExchange(&m_LockState, 1, 0) == 0)
		{
			m_Counter = 1;
			m_Owner = currentThreadId;
			return true;
		}
		else
		{
			if (m_Owner == currentThreadId)
			{
				++m_Counter;
				return true;
			}
		}
		return false;
















#endif
	}

#if !defined __SPU__
	void Unlock()


#endif
	{
		assert(m_Owner == GetCurrentThreadId());
#if !defined __SPU__ && defined CRYTHREAD_DEBUG_LOCKS
		const uint32 currentThreadId = GetCurrentThreadId();
		if (m_Owner != currentThreadId)
		{
			// Dump a message to the console and continue.
			fprintf(stderr,
					"CryLockT<CRYLOCK_RECURSIVE>::Unlock(): "
					"caller (thread %u) is not owner (thread %u) of the lock\n",
					static_cast<unsigned>(currentThreadId),
					static_cast<unsigned>(m_Owner));
			_CryLockError();
		}
#endif
		if (--m_Counter == 0)
		{
#if !defined __SPU__
			m_Owner = 0;
			if (CryInterlockedExchange(&m_LockState, 0) < 0)
				Notify();




#endif
		}
	}
};

namespace NPPU { class CJobManSPU; }

template <typename LockT>
class __attribute__ ((aligned (128))) CryCondBase
{
	friend class NPPU::CJobManSPU;

#if !defined __SPU__
	void PPUNotify()
	{
		sys_mutex_t mutex = mutCond.m_Mutex;
		sys_cond_t cond = mutCond.m_Cond;

		PPU_SYS(sys_mutex_lock(mutex, 0));
		PPU_SYS(sys_cond_signal_all(cond));
		PPU_SYS(sys_mutex_unlock(mutex));
	}
#endif

#if eEOC_CondNotifySingle != eEOC_CondNotify && !defined __SPU__
	void PPUNotifySingle()
	{
		sys_mutex_t mutex = mutCond.m_Mutex;
		sys_cond_t cond = mutCond.m_Cond;

		PPU_SYS(sys_mutex_lock(mutex, 0));
		PPU_SYS(sys_cond_signal(cond));
		PPU_SYS(sys_mutex_unlock(mutex));
	}
#endif

#if !defined __SPU__
	void PPUDestroy()
	{
		PPU_SYS(sys_cond_destroy((sys_cond_t)mutCond.m_Cond));
		PPU_SYS(sys_mutex_destroy((sys_memory_t)mutCond.m_Mutex));
	}
#endif

protected:
	// Condition counter.
	//
	// The counter is incremented whenever the condition is signalled.
	volatile uint32 m_Counter;

	// The number of PPU threads waiting on the condition.
	int32 m_PPUWaiting;

	// 32 bits padding.
	//
	// This class re-uses the eEOC_CondDestroy op-code for destroying
	// the system mutex and condition and the eEOC_CondNotify* op-code for
	// sending notifications.  This padding makes sure that the 'm_Mutex' and
	// 'm_Cond' fields are at the same offset within the class as their
	// counterparts in CryLockBase.
	int32 m_Padding[2];

	// m_Mutex: A system mutex (sys_mutex_t).  Initialized to ~0U, will be created on
	//          demand (i.e. when a PPU thread is being blocked on the lock).
	// m_Cond:  A system condition (sys_mutex_t).  Initialized to ~0U and created
	//          together with the mutex (on demand).
	union 
	{
		struct  
		{
			uint32 m_Mutex;
			volatile uint32 m_Cond;
		};
		int64 mutCondLongVal;
	} mutCond;

#if !defined __SPU__
	// Create the system mutex and condition.
	//
	// The method returns the system mutex ID.  The method will create the
	// system mutex on demand.  When the method returns, both m_Mutex and m_Cond
	// are initialized.
	sys_mutex_t Init()
	{
		if (mutCond.m_Mutex == ~0U)
		{
			sys_mutex_t mutex;
			sys_mutex_attribute_t mutex_attr;
			sys_mutex_attribute_initialize(mutex_attr);
			PPU_SYS(sys_mutex_create(&mutex, &mutex_attr));
			sys_cond_t cond;
			sys_cond_attribute_t cond_attr;
			sys_cond_attribute_initialize(cond_attr);
			PPU_SYS(sys_cond_create(&cond, mutex, &cond_attr));

			// Note: we're writing m_Mutex and m_Cond in a single atomic operation.
			if (CryInterlockedCompareExchange64(
						&mutCond.mutCondLongVal,
						((int64)mutex << 32) | cond,
						~0LL) == ~0LL)
			{
				return mutex;
			}
			else
			{
				PPU_SYS(sys_cond_destroy(cond));
				PPU_SYS(sys_mutex_destroy(mutex));
			}
		}
		return mutCond.m_Mutex;
	}
#endif

	CryCondBase(const CryCondBase &);
	CryCondBase &operator= (const CryCondBase &);

public:
	CryCondBase()
		: m_Counter(0), m_PPUWaiting(0)
	{
		mutCond.m_Mutex = ~0U;
		mutCond.m_Cond  = ~0U;



	}

	~CryCondBase()
	{
		if (mutCond.m_Mutex != ~0U)
		{
#if !defined __SPU__
			PPU_SYS(sys_cond_destroy((sys_cond_t)mutCond.m_Cond));
			PPU_SYS(sys_mutex_destroy((sys_memory_t)mutCond.m_Mutex));


#endif
		}
	}

	void Wait(LockT &lock)
	{
#if !defined __SPU__
		const uint32 counter = m_Counter;
		sys_mutex_t mutex = Init();
		sys_cond_t cond = mutCond.m_Cond;

		PPU_SYS(sys_mutex_lock(mutex, 0));
		CryInterlockedIncrement((int *)&m_PPUWaiting);
		lock.Unlock();
		while (counter == m_Counter)
			PPU_SYS(sys_cond_wait(cond, 0));
		CryInterlockedDecrement((int *)&m_PPUWaiting);
		PPU_SYS(sys_mutex_unlock(mutex));
		lock.Lock();







#endif
	}

	bool TimedWait(LockT &lock, uint32 milliseconds)
	{
#if !defined __SPU__
		const uint32 counter = m_Counter;
		system_time_t startTime = sys_time_get_system_time();
		sys_mutex_t mutex = Init();
		sys_cond_t cond = mutCond.m_Cond;
		bool gotNotified = false;

		PPU_SYS(sys_mutex_lock(mutex, 0));
		CryInterlockedIncrement((int *)&m_PPUWaiting);
		lock.Unlock();
		int64 timer = milliseconds * 1000;
		while (true)
		{
			if (counter != m_Counter)
			{
				gotNotified = true;
				break;
			}
			system_time_t now = sys_time_get_system_time();
			int64 waitTime = timer - (now - startTime);
			if (waitTime <= 0)
				break;
			int waitStatus = sys_cond_wait(cond, waitTime);
			if (waitStatus == ETIMEDOUT)
				break;
#if defined CRYTHREAD_DEBUG_LOCKS
			if (waitStatus != CELL_OK)
			{
				fprintf(stderr,
						"CryCond::TimedWait(lock, %u): "
						"sys_cond_wait(cond=%u, timeout=%u) returned error code 0x%08x\n",
						static_cast<unsigned>(milliseconds),
						static_cast<unsigned>(cond),
						static_cast<unsigned>(waitTime),
						waitStatus);
				_CryLockError();
			}
#endif
		}
		CryInterlockedDecrement((int *)&m_PPUWaiting);
		PPU_SYS(sys_mutex_unlock(mutex));
		lock.Lock();
		return gotNotified;













#endif
	}

	// Note: The exec-on-PPU op-codes eEOC_CondNotify and eEOC_CondNotifySingle
	// will map to the same op-code which performs a sys_cond_signal_all() call
	// on the specified condition.

	void Notify()
	{
#if !defined __SPU__
		CryInterlockedIncrement((int *)&m_Counter);
		if (m_PPUWaiting > 0)
		{
			// If m_PPUWaiting is non-zero, then we _know_ that the system lock and
			// condition have been initialized - no need to call Init() here.
			// Howver, we still must acquire the lock, otherwise the signal could
			// get lost between the atomic increment on m_PPUWaiting and the wait
			// call.
			sys_mutex_t mutex = mutCond.m_Mutex;
			sys_cond_t cond = mutCond.m_Cond;
			PPU_SYS(sys_mutex_lock(mutex, 0));
			PPU_SYS(sys_cond_signal_all(mutCond.m_Cond));
			PPU_SYS(sys_mutex_unlock(mutex));
		}





#endif
	}

	// This implementation of NotifySingle() will wake up at least one waiting
	// PPU thread and/or SPU job.
	//
	// When used between PPU threads only, the method typically wakes up a
	// single PPU thread.  Between PPU and SPU the behaviour is different:
	//
	// - When called from PPU, the method will _always_ wake up _all_ SPU jobs
	//   waiting on the condition.  If one or more PPU threads are waiting, then
	//   at least one of the PPU threads will receive the notifications.
	//
	// - When called from SPU, then this method is equivalent to Notify().  All
	//   waiting PPU threads and SPU jobs will receive the notifications.
	void NotifySingle()
	{
#if !defined __SPU__
		CryInterlockedIncrement((int *)&m_Counter);
		if (m_PPUWaiting > 0)
		{
			sys_mutex_t mutex = mutCond.m_Mutex;
			sys_cond_t cond = mutCond.m_Cond;
			PPU_SYS(sys_mutex_lock(mutex, 0));
			PPU_SYS(sys_cond_signal(mutCond.m_Cond));
			PPU_SYS(sys_mutex_unlock(mutex));
		}




#endif
	}
};

class CryConditionVariable : public CryCondBase< CryLockT<CRYLOCK_RECURSIVE> >
{
	CryConditionVariable(const CryConditionVariable &);
	CryConditionVariable &operator= (const CryConditionVariable &);

public:
	CryConditionVariable() { }
};

#define _CRYTHREAD_HAVE_LOCK 1










#include <CryThread_pthreads.h>



#endif

// vim:ts=2:sw=2:expandtab

