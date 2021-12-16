#pragma once
#undef NOT_USE_CRY_MEMORY_MANAGER

#ifdef __RECODE__
#define _XTPLIB_VISUALSTUDIO_VERSION "vc110"
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0600 // Include Vista-specific functions
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x600		// Allow use of features specific to Windows Vista
#endif						

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0501	// Change this to the appropriate value to target IE 5.0 or later.
#endif


#ifdef _DEBUG
#ifdef assert
#undef assert
#define assert CRY_ASSERT
#endif
#endif

#ifdef _USRDLL
// We are not defining _USRDLL on purpose here. 
//
// Why? Having _USRDLL defined will cause CDynLinkLibrary instance to be
// created with cry-memory allocator that can not be freed within MFC dll.
//
// Consequence of this is that local classes are not registered in MFC
// factories, but it is not something we use for game objects anyway.
#error Will cause Sandbox to crash on exit. 
#endif
#include <afxwin.h>         // MFC core and standard components

#include <platform.h>
#pragma warning(disable: 4103)	// Alignment change after include
#pragma warning(once: 4264)	// Virtual function override warnings, as MFC headers do not pass them.
#pragma warning(disable: 4266)
#pragma warning(once : 4263)
#pragma warning (disable: 4264)

#include "EditorDefs.h"
#include "EditTool.h"

#include "Resource.h"
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Dispatcher.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Enumeration.h>
