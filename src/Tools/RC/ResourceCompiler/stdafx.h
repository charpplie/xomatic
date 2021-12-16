// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define NOT_USE_CRY_MEMORY_MANAGER
//#define NOT_USE_CRY_STRING

// Include Configuration settings for Standart Template Library
#include <STLPortConfig.h>

#include <assert.h>

#define CRY_ASSERT_TRACE assert

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

#include "ixml.h"

#include <platform.h>

// Standard C headers.
#include <direct.h>

// STL headers.
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <map>
#include <set>

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <tchar.h>


// Windows Header Files:
#ifdef WIN64
#include "PortableString.h"
typedef CPortableString string;
#else
#include <atlbase.h>
#include <atlstr.h>
#endif

//#include <StlDbgAlloc.h>
// to make smoother transition back from cry to std namespace...
#define cry std
#define CRY_AS_STD

// emulate the facility present in the cry engine
#include "ILog.h"

#include <smartptr.h>

//////////////////////////////////////////////////////////////////////////
#include "ResComDefs.h"

#include <Cry_Math.h>
#include <primitives.h>
#include <CryHeaders.h>
#include <CryVersion.h>

#include "StlUtils.h"
#include "CryPath.h"

//////////////////////////////////////////////////////////////////////////
// globals.
//////////////////////////////////////////////////////////////////////////
extern void MessageBoxError( const char *format,... );

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

#ifndef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(x)
#endif
