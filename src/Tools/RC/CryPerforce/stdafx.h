/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id: StdAfx.h,v 1.0 2009/01/12 15:18:23 SergeySokov Exp wwwrun $
$DateTime$
Description:  source file that includes just the standard includes
-------------------------------------------------------------------------
History:
- 2009/01/12 15:18 : Created by Sergey Sokov
*************************************************************************/

#pragma once

#define NOT_USE_CRY_MEMORY_MANAGER
//#define NOT_USE_CRY_STRING

// Include Configuration settings for Standart Template Library
#include <STLPortConfig.h>

#include <assert.h>

#define CRY_ASSERT_TRACE assert
#define CRY_ASSERT assert

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

//#include "ixml.h"
#include <platform.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

//#include "../../../SDKs/XenonSDK/Include/win32/vs2005/d3d9.h"
//#include <xboxmath.h>
//#include <XGraphics.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some string constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>

// STL headers.
#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <map>
#include <set>

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

#include "CryVersion.h"
#include "ConvertContext.h"

extern HMODULE g_hInst;

#ifndef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(x) { switch(false){ case false:case(x):break; } }
#endif
