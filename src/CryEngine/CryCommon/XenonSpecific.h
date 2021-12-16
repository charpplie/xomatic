////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   XenonSpecific.h
//  Version:     v1.00
//  Created:     24/9/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __XenonSpecific_h__
#define __XenonSpecific_h__
#pragma once

//////////////////////////////////////////////////////////////////////////
// Turn off C++ structural exception handleling in Dinkumware STL.
//////////////////////////////////////////////////////////////////////////
#ifndef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 0
class exception {
public:
	exception() {};
	exception(const char * what) {};
	exception(const exception& right) {};
	virtual ~exception();
};
#endif
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Standart includes.
//////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <io.h>
#include <xtl.h>
#include <xbdm.h>
#include <ppcintrinsics.h>
#include <xboxmath.h>
#include <process.h>
#include <new.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
//////////////////////////////////////////////////////////////////////////

#define SIZEOF_PTR 4
//(sizeof(LPVOID))

#define _CPU_G5
//#define _CPU_SSE

#define XENON_INTRINSICS
//#define FP16_MESH

//
#define DEBUG_BREAK

#define DEPRICATED __declspec(deprecated)
#define TYPENAME(x) typeid(x).name()

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
typedef signed char         int8;
typedef signed short        int16;
typedef signed int					int32;
typedef signed __int64			int64;
typedef unsigned char				uint8;
typedef unsigned short			uint16;
typedef unsigned int				uint32;
typedef unsigned __int64		uint64;
typedef float               f32;
typedef double              f64;
typedef unsigned long       DWORD;  //biggest float-type on this machine

#ifndef _XENON_INTRINSICS
 typedef double              real;  //biggest float-type on tthis machine




#endif
//#define REAL_IS_FLOAT

typedef __w64 int INT_PTR, *PINT_PTR;
typedef __w64 unsigned int UINT_PTR, *PUINT_PTR;

typedef __w64 long LONG_PTR, *PLONG_PTR;
typedef __w64 unsigned long ULONG_PTR, *PULONG_PTR;

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
//////////////////////////////////////////////////////////////////////////
typedef void *THREAD_HANDLE;
typedef void *EVENT_HANDLE;

#define ILINE __forceinline

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

//////////////////////////////////////////////////////////////////////////
#define NEED_ENDIAN_SWAP

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Multi platform Hi resolution ticks function, should only be used for profiling.
//////////////////////////////////////////////////////////////////////////
int64 CryQueryPerformanceCounter();

__forceinline int64 CryGetTicks()
{
	return __mftb();
}

#ifndef FOURCC
typedef DWORD FOURCC;
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)			{ if(p) { delete (p);		(p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)			{ if(p) { (p)->Release();	(p)=NULL; } }
#endif

#define _MS_ALIGN(num) __declspec(align(num))

#define DEFINE_ALIGNED_DATA( type, name, alignment ) _declspec(align(alignment)) type name;
#define DEFINE_ALIGNED_DATA_STATIC( type, name, alignment ) static _declspec(align(alignment)) type name;
#define DEFINE_ALIGNED_DATA_CONST( type, name, alignment ) const _declspec(align(alignment)) type name;

//#if !defined(_LIB)
//#define _LIB 1
//#endif

#ifdef _LIB
#if !defined(USE_STATIC_NAME_TABLE)
#define USE_STATIC_NAME_TABLE 1
#endif
#endif

#define _STLP_WCE
//////////////////////////////////////////////////////////////////////////
#endif // __XenonSpecific_h__

