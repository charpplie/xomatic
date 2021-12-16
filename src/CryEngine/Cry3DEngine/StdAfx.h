////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   stdafx.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDAFX_H__8B93AD4E_EE86_4127_9BED_37AC6D0F978B__INCLUDED_3DENGINE)
#define AFX_STDAFX_H__8B93AD4E_EE86_4127_9BED_37AC6D0F978B__INCLUDED_3DENGINE

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning( disable : 6286 ) // (<non-zero constant> || <expres ion>) is always a non-zero constant. <expression> is never evaluated and might have side
#pragma warning( disable : 6237 ) // (<zero> && <expression>) is always zero. <expression> is never evaluated and might have side effects
#pragma warning( disable : 6236 ) // (<expression> || <non-zero constant>) is always a non-zero constant
#pragma warning( disable : 6235 ) // (<non-zero constant> || <expression>) is always a non-zero constant
#pragma warning( disable : 6239 ) // (<non-zero constant> && <expression>) always evaluates to the result of <expression>
#pragma warning( disable : 6240 ) // (<expression> && <non-zero constant>) always evaluates to the result of <expression>

//#define NOT_USE_CRY_MEMORY_MANAGER 

#include <CryModuleDefs.h>
#define eCryModule eCryM_3DEngine
#define RWI_NAME_TAG "RayWorldIntersection(3dEngine)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(3dEngine)"

#define CRY3DENGINE_EXPORTS

const int nThreadsNum = 3;

//#define DEFINE_MODULE_NAME "Cry3DEngine"
//#define  TRACE_ALL_ALLOCATIONS

//#define FORCE_STANDARD_ASSERT // fix edit and continue


#include <platform.h>

#pragma warning( error: 4018 ) 

#include <stdio.h>

#define MAX_PATH_LENGTH 512

#include <ITimer.h>
#include <IProcess.h>
#include <Cry_Math.h>
#include <Cry_XOptimise.h>
#include <Cry_Geo.h>
#include <ILog.h>
#include <ISystem.h>
#include <IConsole.h>
#include <IPhysics.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <IEntityRenderState.h>
#include <I3DEngine.h>
#include <ICryPak.h>
#include <CryFile.h>
#include <smartptr.h>
#include <CryArray.h>
#include <CryHeaders.h>
#include "Cry3DEngineBase.h"
#include <float.h>
#include "CryArray.h"
#include "cvars.h"
#include <CrySizer.h>
#include <StlUtils.h>
#include "Array2d.h"
#include "3dEngine.h"
#include "ObjMan.h"
#include "Vegetation.h"
#include "terrain.h"
#include "ObjectsTree.h"

// TODO refactor!
// 1. This is not the right place for defining a function like 'vsnprintf()'.
// 2. Subtle changes to the semantics of 'vsnprintf()' (null-termination) are
//    _not_ nice. Should use a different function name for that!
#ifdef _MSC_VER
inline int vsnprintf(char * buf, int size, const char * format, va_list & args)
{
	int res = _vsnprintf(buf, size, format, args);
	assert(res>=0 && res<size); // just to know if there was problems in past
	buf[size-1]=0;
	return res;
}
#else
namespace std
{
	// 'vsnprintf_safe()' must be within the 'std' namespace, otherwise the
	// #define below would break standard includes.
	inline int vsnprintf_safe(char *buf, int size, const char *format, va_list ap)
	{
		int res = vsnprintf(buf, size, format, ap);
		buf[size - 1] = 0;
		return res;
	}
}
using std::vsnprintf_safe;
#undef vsnprintf
#define vsnprintf vsnprintf_safe
#endif // _MSC_VER

inline int snprintf(char * buf, int size, const char * format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	int res = vsnprintf(buf, size, format, arglist);
	va_end(arglist);	
	return res;
}

template <class T> 
void AddToPtr(byte * & pPtr, T & rObj, EEndian eEndian)
{
  PREFAST_SUPPRESS_WARNING(6326) COMPILE_TIME_ASSERT(((sizeof(T)%4)==0));
  assert(!((int)pPtr&3));
  memcpy(pPtr,&rObj,sizeof(rObj)); 
  SwapEndian(*(T*)pPtr, eEndian);
  pPtr += sizeof(rObj);
  assert(!((int)pPtr&3));
}

template <class T> 
void AddToPtr(byte * & pPtr, int & nDataSize, T & rObj, EEndian eEndian)
{
  PREFAST_SUPPRESS_WARNING(6326) COMPILE_TIME_ASSERT(((sizeof(T)%4)==0));
  assert(!((int)pPtr&3));
  memcpy(pPtr,&rObj,sizeof(rObj)); 
  SwapEndian(*(T*)pPtr, eEndian);
  pPtr += sizeof(rObj);
  nDataSize -= sizeof(rObj);
  assert(nDataSize>=0);
  assert(!((int)pPtr&3));
}


inline void FixAlignment(byte * & pPtr, int & nDataSize)
{
  while((int)pPtr&3)
  {
    *pPtr = 222;
    pPtr++;
    nDataSize--;
  }
}

inline void FixAlignment(byte * & pPtr)
{
  while((int)pPtr&3)
  {
    *pPtr = 222;
    pPtr++;
  }
}

template <class T> 
void AddToPtr(byte * & pPtr, int & nDataSize, const T * pArray, int nElemNum, EEndian eEndian, bool bFixAlignment = false)
{
  assert(!((int)pPtr&3));
  memcpy(pPtr,pArray,nElemNum*sizeof(T)); 
  SwapEndian((T*)pPtr, nElemNum, eEndian);
  pPtr += nElemNum*sizeof(T);
  nDataSize -= nElemNum*sizeof(T);
  assert(nDataSize>=0);

  if(bFixAlignment)
    FixAlignment(pPtr,nDataSize);
  else
    assert(!((int)pPtr&3));
}

template <class T> 
void AddToPtr(byte * & pPtr, const T * pArray, int nElemNum, EEndian eEndian, bool bFixAlignment = false)
{
  assert(!((int)pPtr&3));
  memcpy(pPtr,pArray,nElemNum*sizeof(T)); 
  SwapEndian((T*)pPtr, nElemNum, eEndian);
  pPtr += nElemNum*sizeof(T);

  if(bFixAlignment)
    FixAlignment(pPtr);
  else
    assert(!((int)pPtr&3));
}

struct TriangleIndex
{
  TriangleIndex() { ZeroStruct(*this); }
  uint16 & operator [] (const int & n) { assert(n>=0 && n<3); return idx[n]; }
  const uint16 & operator [] (const int & n) const { assert(n>=0 && n<3); return idx[n]; }
  uint16 idx[3];
  uint16 nCull;
};

#if !defined(__SPU__)
	#define FUNCTION_PROFILER_3DENGINE FUNCTION_PROFILER_FAST( gEnv->pSystem, PROFILE_3DENGINE, m_bProfilerEnabled )


#endif

#endif // !defined(AFX_STDAFX_H__8B93AD4E_EE86_4127_9BED_37AC6D0F978B__INCLUDED_3DENGINE)
