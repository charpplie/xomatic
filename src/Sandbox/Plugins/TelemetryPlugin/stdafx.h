// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#include "targetver.h"

// HAX to resolve IEntity redefinition
#define __IEntity_INTERFACE_DEFINED__ 

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define NOT_USE_CRY_MEMORY_MANAGER
#include <platform.h>

// Re-disable virtual function override warnings, as MFC headers do not pass them.
#pragma warning(disable: 4264)
#pragma warning(disable: 4266)

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'float' to 'uint8', possible loss of data
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)

// Include XT Toolkit
#define _XTP_INCLUDE_DEPRECATED
#include <XTToolkitPro.h>

#include "Resource.h"



/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <algorithm>
#include <memory>

#include "CryExtension/CrySharedPtr.h"

#define SANDBOX_API
#define CRYEDIT_API
#include "IEditor.h"
#include "Include/SandboxAPI.h"

#include "Controls/NumberCtrl.h"

#include <IItem.h>


#ifdef WIN64
#pragma comment(lib, "../../../../Bin64/Editor.lib")
#else
#pragma comment(lib, "../../../../Bin32/Editor.lib")
#endif

