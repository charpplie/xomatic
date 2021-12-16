// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NOT_USE_CRY_MEMORY_MANAGER

// Include Configuration settings for Standart Template Library
#include <STLPortConfig.h>

#include <cassert>

#define CRY_ASSERT_TRACE assert

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

#include <platform.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500		// min Win2000 for GetConsoleWindow()     Baustelle
#endif

// Windows Header Files:
#include <windows.h>



#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some string constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>

#include <vector>
#include <map>

extern HMODULE g_hInst;



#ifndef ReleasePpo
#define ReleasePpo(ppo) \
	if (*(ppo) != NULL) \
		{ \
		(*(ppo))->Release(); \
		*(ppo) = NULL; \
		} \
		else (VOID)0
#endif

#include "StaticAssert.h"

// TODO: reference additional headers your program requires here
