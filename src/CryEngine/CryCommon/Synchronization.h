//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
// Modified 2008-06, Scott Peter
//	Refactored into PSwap and PLock components, shortened names

//---------------------------------------------------------------------------
#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

//---------------------------------------------------------------------------
// Synchronization policies, for classes (e.g. containers, allocators) that may
// or may not be multithread-safe.
//
// Policies should be used as a template argument to such classes,
// and these class implementations should then utilise the policy, as a base-class or member.
//
//---------------------------------------------------------------------------

#include "MultiThread.h"
#include "CryThread.h"

namespace stl
{
	//
	// Lock policies, for exclusive access for a time period, controlled by a semaphore.
	// Has the following components:
	//
	//		_Semaphore
	//				// Controlling semaphore object, will be inherited by subclass.
	//		Lock
	//				// Scoped locking class, acquires & releases semaphore
	//

	// Spin-locking policy, suitable for very short lock times.
	struct PSyncMultiThread
	{
		PSyncMultiThread()
			: _Semaphore(0)
			{}

		struct Lock: NoCopy, WriteLock
		{
			Lock(const PSyncMultiThread& p)
				: WriteLock(p._Semaphore)
				{}
		};

		volatile int& CheckUnlocked() const
		{
			assert(_Semaphore == 0);
			return _Semaphore;
		};

	#if defined(_DEBUG) && !defined(PS3)
		struct DebugLock: NoCopy, WriteLock
		{
			DebugLock(const PSyncMultiThread& p)
				: WriteLock(p.CheckUnlocked())
				{}
		};
	#else
		struct DebugLock: NoCopy
		{
			DebugLock(const PSyncMultiThread&)
				{}
		};
	#endif

	private:
		mutable volatile int _Semaphore;
	};

	// Non-locking (single-threaded) policy

#if defined(_DEBUG) && !defined(PS3)

	// In debug mode, verify non-simultaneous access.
	struct PSyncNone: PSyncMultiThread
	{
		typedef DebugLock Lock;
	};

#else

	// In release mode, no locking.
	struct PSyncNone
	{
		struct Lock: NoCopy
		{
			Lock(const PSyncNone&)
			{}
		};
	};

#endif

}

#endif // __SYNCHRONIZATION_H__
