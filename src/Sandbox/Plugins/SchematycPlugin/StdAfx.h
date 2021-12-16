#pragma once

#include <STLPortConfig.h>

#if defined(_DEBUG)
#define _STLP_DEBUG 1	// STL Port in debug for debug builds.
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN	// Exclude rarely-used stuff from Windows headers.
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

#ifndef WINVER								// Allow use of features specific to Windows XP or later.
#define WINVER 0x0600					// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT					// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0600		// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS				// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410	// Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE							// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600			// Change this to the appropriate value to target other versions of IE.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// Some CString constructors will be explicit.

#include <afxwin.h>         // MFC core and standard components.
#include <afxext.h>         // MFC extensions.

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes.
#include <afxodlgs.h>       // MFC OLE dialog classes.
#include <afxdisp.h>        // MFC Automation classes.
#endif

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>					// MFC ODBC database classes.
#endif

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>					// MFC DAO database classes.
#endif

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>				// MFC support for Internet Explorer 4 Common Controls.
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>					// MFC support for Windows Common Controls
#endif

#pragma warning(disable: 4244)	// warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800)	// 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4266)	// no override available for virtual member function from base 'CObject'; function is hidden

// CryEngine headers.

#include <CryModuleDefs.h>
#include <platform.h>

// STL headers.

#include <vector>
#include <list>
#include <map>	
#include <set>
#include <algorithm>

// Additional CryEngine headers.

#include <SandboxAPI.h>
#include <ISystem.h>
#include <functor.h>

#include "Util/EditorUtils.h"
#include "IEditor.h"
#include "Util/PathUtil.h"
#include "Util/SmartPtr.h"
#include "Util/Variable.h"

// MFC & XTToolkit Pro headers.

#define _XTP_INCLUDE_DEPRECATED

#ifdef __RECODE__
#define _XTPLIB_VISUALSTUDIO_VERSION "vc110"
#endif

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#include <AfxWin.h>
#include <AfxOle.h>
#include <AfxExt.h>
#include <AfxDisp.h>
#include <AfxCmn.h>
#include <AfxControlBars.h>
#include <AfxPropertyGridCtrl.h>
#include <XTToolkitPro.h>

#undef max
#undef min

// Schematyc headers and declarations.

#if defined(SCHEMATYC_PLUGIN_EXPORTS)
#define SCHEMATYC_PLUGIN_API __declspec(dllexport)
#else
#define SCHEMATYC_PLUGIN_API __declspec(dllimport)
#endif

#include <TemplateUtils/TemplateUtils_Delegate.h>
#include <TemplateUtils/TemplateUtils_Signal.h>

#include <Schematyc/Schematyc_GUID.h>
#include <Schematyc/Schematyc_ICompiler.h>
#include <Schematyc/Schematyc_IDoc.h>
#include <Schematyc/Schematyc_IFramework.h>
#include <Schematyc/Schematyc_ILibRegistry.h>
#include <Schematyc/Schematyc_ILog.h>

IEditor* GetIEditor();
HINSTANCE GetHInstance();