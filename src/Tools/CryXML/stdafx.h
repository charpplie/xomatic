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

#define CRY_STRING
#include <platform.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "Cry_Math.h"
