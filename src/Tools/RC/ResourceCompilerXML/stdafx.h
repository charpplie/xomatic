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

#include <platform.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define NOMINMAX

#include <vector>
#include <map>

#include <stdio.h>
#include <tchar.h>
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some string constructors will be explicit
//
//#include <atlbase.h>
//#include <atlstr.h>

#include <windows.h>

extern HMODULE g_hInst;

#ifndef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(x)
#endif
