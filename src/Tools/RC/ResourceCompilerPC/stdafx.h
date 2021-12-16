// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define NOT_USE_CRY_MEMORY_MANAGER

// Include Configuration settings for Standart Template Library
#include <STLPortConfig.h>

#include <assert.h>

#define CRY_ASSERT_TRACE assert

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

#define NO_XENON_INTRINSICS // Make sure we don't end up using xenon code.

#include <platform.h>

typedef string tstring;

#include <malloc.h>			// malloc()
// Windows Header Files:
#ifdef WIN64
#include "PortableString.h"
typedef CPortableString string;
#else
#include <atlbase.h>
#include <atlstr.h>
#endif


#include <float.h>

#include <memory>
#include <set>
#include <map>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some string constructors will be explicit

#include "IXml.h"

#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <CryHeaders.h>
#include <primitives.h>
#include <smartptr.h>
#include <physinterface.h>
#include <CrySizer.h>
// TODO: reference additional headers your program requires here
#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr) (sizeof(arr)/sizeof((arr)[0]))

#ifndef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(x)
#endif

#include "ResourceCompilerPC.h"
#include "StringUtils.h"

#include "CryFile.h"

#include <VertexFormats.h>

#ifndef WIN64
#include "Dbghelp.h"
#endif

#include "..\ResourceCompiler\FileUtil.h"
#include "..\ResourceCompiler\IRCLog.h"
#include "..\ResourceCompiler\SwapEndianness.h"

#endif
