#pragma once

#pragma warning(disable: 4103) // '...\stlport\stl\_cprolog.h' : alignment changed after including header, may be due to missing #pragma pack(pop)

//#define NOT_USE_CRY_MEMORY_MANAGER

// STL Port in debug for debug builds
#if defined(_DEBUG)
//#  define _STLP_DEBUG 1
#endif
#include <STLPortConfig.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifndef WINVER
#define WINVER 0x0501 // Windows XP
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600	
#endif						

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#include <stdlib.h>
#include <afxwin.h>
#include <afxext.h>



/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <CryModuleDefs.h>
#include <platform.h>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>	
#include <set>
#include <algorithm>

#include <Cry_Math.h>