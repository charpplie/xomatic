/////////////////////////////////////////////////////////////////////////////
//
// Crytek Source File
// Copyright (C), Crytek Studios, 2001-2008.
//
// History:
// Jan 20, 2008: Created by Sascha Demetrio
//
/////////////////////////////////////////////////////////////////////////////

#include "CryThread_ps3.h"

#if !defined PS3_USE_POSIX_LOCKS && defined CRYTHREAD_DEBUG_LOCKS

// This function is called by the CryThread locks and conditions in case of an
// unexpected error (useful for setting a breakpoint).
void _CryLockError()
{
	puts("_CryLockError()");
}

#endif // !PS3_USE_POSIX_LOCKS && CRYTHREAD_DEBUG_LOCKS

#include "CryThreadImpl_pthreads.h"

// vim:ts=2:sw=2:expandtab

