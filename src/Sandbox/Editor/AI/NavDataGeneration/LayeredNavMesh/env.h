#ifndef ENV_H
#define ENV_H

// NOTE Mai 8, 2008: <pvl> one of these should be active - selects the way
// of getting time, printing messages and possibly more.  Either we access
// this functionality via CE or POSIX API.
#define CRY_ENGINE
//#define UNIX

#ifdef CRY_ENGINE
// ****************************************************************************
// NOTE Mai 7, 2008: <pvl> the following mess is what you get when you try
// to use just a small part of CryEngine, small enough for you to avoid
// using StdAfx.h . :-((((

// NOTE Mai 9, 2008: <pvl> unfortunately, following lines also pull in a lot
// of unwanted stuff (because of CE's bad physical code design).  Whoever
// #include's this file will need to explicitly qualify (with LayeredNavMesh
// namespace prefix) every mention of DynArray<> and Vec3 since CryEngine
// contains same-named unrelated classes (in the global namespace, too - that's
// why "using LayeredNavMesh::Vec3" along with "using LayeredNavMesh" won't help).

#include "CryModuleDefs.h"
#pragma warning (disable : 4005)	// to disable AUTO_TYPE_STRUCT redefinition  warning
#include "platform.h"				// for de-virtualisation macros necessary to compile ISystem.h
#pragma warning (default : 4005)
#include "ISystem.h"				// for gEnv

// NOTE Mai 7, 2008: <pvl> required to get time functionality
#undef AUTO_STRUCT_INFO
#define AUTO_STRUCT_INFO		// CTimeValue uses that, get rid of it here
#include "TimeValue.h"
#include "ITimer.h"

// NOTE Mai 8, 2008: <pvl> logging (note the printf bit - particularly nasty
// but sort of unavoidable considering that I'd like to stay away from writing
// my own portability API here)
#include "../AILog.h"
#if !defined(PS3)
#	define printf AILogAlways
#endif

// (MATT) Extended this simple stopwatch class - to use correctly:
// Create, Start(), Stop(), GetElapsed()
// or:
// Create, Resume(), Pause(), Resume(), Pause(), ... GetElapsed()
// {2008/11/05}

class Stopwatch {
	CTimeValue m_start;
	CTimeValue m_end;
	CTimeValue m_accumulator;
public:
	void Start () { m_start = gEnv->pTimer->GetAsyncTime(); m_accumulator.SetValue(0); }
	void Stop () { m_end = gEnv->pTimer->GetAsyncTime(); }
	void Pause () { m_end = gEnv->pTimer->GetAsyncTime(); m_accumulator += m_end - m_start; }
	void Resume () { m_start = gEnv->pTimer->GetAsyncTime(); }
	float GetElapsed () const { return (m_end - m_start + m_accumulator).GetSeconds(); }
};

/**
 * The following pair of functions convert between LayeredNavMesh::Vec3 and
 * ::Vec3 both in terms of C++ type and coordinate system convention.
 */
#include "Vec3.h"
inline LayeredNavMesh::Vec3 ConvertVec3 (const Vec3 & v)
{
	return LayeredNavMesh::Vec3 (v.y, v.z, v.x);
}

inline ::Vec3 ConvertVec3 (const LayeredNavMesh::Vec3 & v)
{
	return Vec3 (v.z, v.x, v.y);
}


#elif defined (UNIX)
// ****************************************************************************
// NOTE Mai 8, 2008: <pvl> as of now, stuff in "UNIX" section is untested

#include <sys/time.h>

static inline double GetTimeUsec()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}	

class Stopwatch {
	double m_start;
	double m_end;
public:
	Stopwatch () : m_start (0.0), m_end (0.0 { }
	void Start () { m_start = GetTimeUsec (); }
	void Stop () { m_end = GetTimeUsec (); }
	double GetElapsed () const { return m_end - m_start; }
};

#endif // CRY_ENGINE & co.

#endif // ENV_H