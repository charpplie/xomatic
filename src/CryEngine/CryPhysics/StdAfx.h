// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4AA14050_1B79_4A11_9D24_4E209BF87E2C__INCLUDED_)
#define AFX_STDAFX_H__4AA14050_1B79_4A11_9D24_4E209BF87E2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <CryModuleDefs.h>
#define eCryModule eCryM_Physics

#undef NO_XENON_INTRINSICS
#define NO_XENON_INTRINSICS
#undef XENON_INTRINSICS

// Enables SPU memory stream debugging 
//#define MEMSTREAM_DEBUG 1
#define MEMSTREAM_DEBUG_TAG (0xcafebabe)

#if defined(MEMSTREAM_DEBUG)
#define MEMSTREAM_DEBUG_ASSERT(x) if (!(x)) { snPause(); }
#else
#define MEMSTREAM_DEBUG_ASSERT(x) 
#endif 

// Enables SPU function table debugging 
//#define ENABLE_SPU_FUNCTABLE_DEBUG 1
#define SPU_FUNCTABLE_DEBUG_TAG (0xbaadf00d)

#ifndef PHYSICS_EXPORTS
#define PHYSICS_EXPORTS
#endif

#define ENTITY_PROFILER_ENABLED 

#pragma warning (disable : 4554 4305 4244 4996)
#pragma warning (disable : 6326) //Potential comparison of a constant with another constant

#define CONSTANT_COMPARISON_OK PREFAST_SUPPRESS_WARNING(6326)
#define SIZEOF_ARRAY_OK				 PREFAST_SUPPRESS_WARNING(6384)
#define LOCAL_NAME_OVERRIDE_OK PREFAST_SUPPRESS_WARNING(6246)
#define INDEX_NOT_OUT_OF_RANGE PREFAST_SUPPRESS_WARNING(6201)
#define NO_BUFFER_OVERRUN			 PREFAST_SUPPRESS_WARNING(6385 6386)

#if defined(XENON) || defined(PS3) && !defined(__SPU__)
#define USE_DYN_POOL
#define NEW_OVERRIDEN
void StartDynAllocs();
int EndDynAllocs();
void SetDynPoolSize(int size);
int FlushDynPool();
int DynAllocsTracked();
int SetDynAllocState(void*);
void RestoreDynAllocState(int);
int GetFreeDynSize();
#else
inline void StartDynAllocs() {}
inline int DynAllocsFailed(int iCaller=0) { return 0; }
inline int EndDynAllocs() { return 0; }
inline void SetDynPoolSize(int size) {}
inline int FlushDynPool() { return 1; }
inline int DynAllocsTracked() { return 0; }
inline int SetDynAllocState(void*) { return 0; }
inline void RestoreDynAllocState(int) {}
inline int GetFreeDynSize() { return 0; }
#endif

#include <platform.h>

#ifndef __SPU__
#include <vector>
#include <map>
#include <algorithm>
#include <float.h>

#include "CryThread.h"
#endif

#include "Cry_Math.h"
#include "Cry_XOptimise.h"

#define NO_CRY_STREAM

#ifndef NO_CRY_STREAM
#include "Stream.h"
#else
#ifndef __SPU__
#include "ISystem.h"
#include "ILog.h"
#endif
class CStream {
public:
	bool WriteBits(unsigned char *pBits, uint32 nSize) { return true; }
	bool ReadBits(unsigned char *pBits, uint32 nSize) { return true; }
	bool Write(bool b) { return true; }
	bool Write(char c) { return true; }
	bool Write(unsigned char uc) { return true; }
	bool Write(float f) { return true; }
	bool Write(unsigned short us) { return true; }
	bool Write(short s) { return true; }
	bool Write(int i) { return true; }
	bool Write(unsigned int ui) { return true; }
	bool Write(const Vec3 &v) { return true; }
	bool Write(const Ang3 &v) { return true; }
	bool Read(bool &b) { return true; }
	bool Read(char &c) { return true; }
	bool Read(unsigned char &uc) { return true; }
	bool Read(unsigned short &us) { return true; }
	bool Read(short &s) { return true; }
	bool Read(int &i) { return true; }
	bool Read(unsigned int &ui) { return true; }
	bool Read(float &f) { return true; }
	bool Read(Vec3 &v) { return true; }
	bool Read(Ang3 &v) { return true; }
	bool WriteNumberInBits(int n,size_t nSize) { return true; }
	bool WriteNumberInBits(unsigned int n,size_t nSize) { return true; }
	bool ReadNumberInBits(int &n,size_t nSize) { return true; }
	bool ReadNumberInBits(unsigned int &n,size_t nSize) { return true; }
	bool Seek(size_t dwPos = 0) { return true; }
	size_t GetReadPos() { return 0; }
	unsigned char *GetPtr() const { return 0; };
	size_t GetSize() const { return 0; }
	bool SetSize(size_t indwBitSize) { return true; }
};
#endif

#ifdef WIN64
#undef min
#undef max
#endif

//#define PHYS_JOB_SIMULATION
#define USE_SOLVER_JOB
#define USE_CLOTH_JOB
#define USE_ROPE_JOB
//#define USE_RWI_JOB
#if defined(PS3) || defined(PHYS_JOB_SIMULATION)
#define USE_PHYS_JOBS
#endif
// *Note*: This is now job aware!
#define JOB_LOCAL(...) SPU_LOCAL_FOR_JOBS(__VA_ARGS__)

#ifndef __SPU__
#include "CrySizer.h"
#else
#define SIZER_COMPONENT_NAME(a, b)
struct ICrySizer
{
	void AddObject(void*, unsigned int){}
	void AddObject(void*, unsigned int, int){}
};

#define FRAME_PROFILER(a,b,c)
#define FUNCTION_PROFILER(a,b)
#endif

#define ENTGRID_2LEVEL

// TODO: reference additional headers your program requires here
#include "primitives.h"
#include "utils.h"
#include "physinterface.h"

#if defined(__SPU__)
#define __spu_debug_break snPause();
#undef MAX_PHYS_THREADS
#define MAX_PHYS_THREADS 0
inline int get_iCaller() { return 0; }
inline int get_iCaller_int() { return 0; }
#define CryGetTicks() 0
struct Lock_dummy
{
	Lock_dummy(){}
	Lock_dummy(Lock_dummy_param){}
	Lock_dummy(int, int){}
	Lock_dummy(int&){}
	Lock_dummy(unsigned int&){}
	Lock_dummy(volatile int&){}
	void SetActive(int=0){}
};
#define ReadLock Lock_dummy
#define WriteLock Lock_dummy
#define ReadLockCond Lock_dummy
#define WriteLockCond Lock_dummy
#else
#define __spu_debug_break
#if !defined(XENON) && MAX_PHYS_THREADS<=1
extern int g_physThreadId;
inline int IsPhysThread() { return iszero((int)CryGetCurrentThreadId()-g_physThreadId); }
inline void MarkAsPhysThread() { g_physThreadId = CryGetCurrentThreadId(); }
inline void MarkAsPhysWorkerThread(int*) {}
inline int get_iCaller() { return IsPhysThread()^1; }
inline int get_iCaller_int() { return 0; }
#else 
TLS_DECLARE(int*,g_pidxPhysThread) 
inline int IsPhysThread() {
	int dummy = 0;
	INT_PTR ptr = (INT_PTR)TLS_GET(INT_PTR, g_pidxPhysThread);
	ptr += (INT_PTR)&dummy-ptr & (ptr-1>>sizeof(INT_PTR)*8-1 ^ ptr>>sizeof(INT_PTR)*8-1);
	return *(int*)ptr; 
}
void MarkAsPhysThread();
void MarkAsPhysWorkerThread(int*);
inline int get_iCaller() {
	int dummy = MAX_PHYS_THREADS;
	INT_PTR ptr = (INT_PTR)TLS_GET(INT_PTR,g_pidxPhysThread);
	ptr += (INT_PTR)&dummy-ptr & (ptr-1>>sizeof(INT_PTR)*8-1 ^ ptr>>sizeof(INT_PTR)*8-1);
	return *(int*)ptr; 
}
#define get_iCaller_int get_iCaller
#endif
#endif

#ifdef USE_DYN_POOL
int DynAllocsFailed(int iCaller=get_iCaller());
#endif

#ifndef MAIN_THREAD_NONWORKER
#define FIRST_WORKER_THREAD 1
#else
#define FIRST_WORKER_THREAD 0
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4AA14050_1B79_4A11_9D24_4E209BF87E2C__INCLUDED_)
