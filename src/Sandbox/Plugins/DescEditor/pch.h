#pragma once

#ifndef FORCE_NOT_USE_CRY_MEMORY_MANAGER
#	ifdef NOT_USE_CRY_MEMORY_MANAGER
#	undef NOT_USE_CRY_MEMORY_MANAGER
#	endif
#endif

#ifdef __RECODE__
#define _XTPLIB_VISUALSTUDIO_VERSION "vc110"
#endif

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
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

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
//#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0501	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

// prevent inclusion of conflicting definitions of INT8_MIN etc
#define _INTSAFE_H_INCLUDED_

#include <CryModuleDefs.h>
#define eCryModule eCryM_Launcher
#define RWI_NAME_TAG "RayWorldIntersection(Editor)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(Editor)"

#define _CRT_RAND_S
#define _HAS_EXCEPTIONS 1

#include <platform.h>

#include "ProjectDefines.h"

#pragma warning(disable: 4103)	// Alignment change after include
#pragma warning(once: 4264)	// Virtual function override warnings, as MFC headers do not pass them.
#pragma warning(disable: 4266)
#pragma warning(once : 4263)

#include <afxcontrolbars.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdlgs.h>

// MFC text conversions.
#include <afxconv.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

// Shell Extensions.
#include <Shlwapi.h>

#ifdef WIN64
//#include <ObjBase.h>
#include <atlbase.h>
#endif

//#ifdef _AMD64_
//#include <io.h>
//#include "mfc_amd64_fix.h"
//#endif

// Resource includes
#include "Resource.h"

#ifdef _DEBUG

#ifndef WIN64
#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>
//#define   calloc(s,t)       _calloc_dbg(s, t, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif //WIN64

//////////////////////////////////////////////////////////////////////////
// Main Editor include.
//////////////////////////////////////////////////////////////////////////
#include "EditorDefs.h"

#ifdef _DEBUG
#ifdef assert
#undef assert
#define assert CRY_ASSERT
#endif
#endif

#include "CryEdit.h"
#include "EditTool.h"
#include "PluginManager.h"

#include "Util/Variable.h"

#include <IGame.h>
#include <ISplines.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <CryListenerSet.h>

#define USE_PYTHON_SCRIPTING
#define BOOST_PYTHON_STATIC_LIB

#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Property.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Enumeration.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Reflection.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Util.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\NetSerialize.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Dispatcher.h>