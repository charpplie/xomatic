#if !defined(AFX_STDAFX_H__68E70A2F_B863_11D1_98AD_0040051EDCE7__INCLUDED_)
#define AFX_STDAFX_H__68E70A2F_B863_11D1_98AD_0040051EDCE7__INCLUDED_

#if !defined(NOT_USE_CRY_STRING)
#define NOT_USE_CRY_STRING
#endif
#if !defined(NOT_USE_CRY_MEMORY_MANAGER)
#define NOT_USE_CRY_MEMORY_MANAGER
#endif

#define NO_XENON_INTRINSICS // Make sure we don't end up using xenon code.

// Turn off debug stl stuff.
#define _HAS_ITERATOR_DEBUGGING 0
#define _SECURE_SCL 0
#define _SECURE_SCL_THROWS 0

// Turn of _DEBUG around stl headers - this is to stop the classes having a first iterator pointer,
// which breaks some binary compatibility. This is only really needed because the Max morpher modifier
// exposes a std::vector as part of the interface to the dll and for some reason expects that to work.
#if defined(_DEBUG)
#	undef _DEBUG
#endif

// Following pragmas (copied from afx.h) guarantee that we will have no errors like the following one:
// "nafxcw.lib(afxmem.obj) : error LNK2005: "void * __cdecl operator new[](unsigned int)" (??_U@YAPAXI@Z) already defined in libcpmt.lib(newaop.obj)"
#ifndef _UNICODE
	#ifdef _DEBUG
		#pragma comment(lib, "nafxcwd.lib")
	#else
		#pragma comment(lib, "nafxcw.lib")
	#endif
#else
	#ifdef _DEBUG
		#pragma comment(lib, "uafxcwd.lib")
	#else
		#pragma comment(lib, "uafxcw.lib")
	#endif
#endif

#include <platform.h>

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <string>
typedef std::string string;
typedef std::string tstring;

#include <Max.h>																			// MAX's main header file

#include "../PolyBumpCommon.h"

// 3DSMax backward compatibility
#if MAX_PRODUCT_VERSION_MAJOR >= 9
	#define MAXSTR MSTR
#else //MAX_PRODUCT_VERSION_MAJOR >= 9
	#define MAXSTR TSTR
#endif //MAX_PRODUCT_VERSION_MAJOR >= 9


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#include "Cry_Math.h"

#endif // !defined(AFX_STDAFX_H__68E70A2F_B863_11D1_98AD_0040051EDCE7__INCLUDED_)
