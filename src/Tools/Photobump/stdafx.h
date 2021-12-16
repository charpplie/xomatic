// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls

//STL headers
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <string>

#include <math.h>

#include "ForwardDecl.h"

#include "gl/gl.h"
#include "gl/glu.h"
//#include "extgl.h"

//#include "opencv/cv.h"
//#include "opencv/cvaux.h"
//#include "opencv/highgui.h"

#include "CCamera.h"
#include "Utils.h"

//////////////////////////////////////////////////////////////////////////
#pragma warning (disable: 4786) // identifier length warning (stl templates generate very long names)

//utils
//////////////////////////////////////////////////////////////////////////	
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)			{ if(p) { delete (p);		(p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete[] (p);		(p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)			{ if(p) { (p)->Release();	(p)=NULL; } }
#endif

/////////////////////////////////////////////////////////////////////////////
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef	unsigned char		uchar;
typedef	unsigned short	ushort;
typedef	unsigned int		uint;

template <class T> inline void ZeroStruct( T &t ) { memset( &t,0,sizeof(t) ); }

/////////////////////////////////////////////////////////////////////////////
struct string_nocase_lt 
{
	bool operator()( const std::string &s1,const std::string &s2 ) const 
	{
		return stricmp(s1.c_str(),s2.c_str()) < 0;
	}
};

//////////////////////////////////////////////////////////////////////////
inline void MyOutputDebugString(const char *szMessage,...)
{
	va_list	argptr;
	char		msg[4096];

	va_start (argptr,szMessage);
	vsprintf (msg,szMessage,argptr);
	va_end (argptr);	

	OutputDebugString(msg);
}

//////////////////////////////////////////////////////////////////////////
inline void MyAfxMessageBox(const char *szMessage,...)
{
	va_list	argptr;
	char		msg[4096];

	va_start (argptr,szMessage);
	vsprintf (msg,szMessage,argptr);
	va_end (argptr);	

	AfxMessageBox(msg);
}

//////////////////////////////////////////////////////////////////////////
inline void	Break()
{
	_asm int 3;
}

//////////////////////////////////////////////////////////////////////////
inline void	MyError(const char *szMessage,...)
{
	va_list	argptr;
	char		msg[4096];

	va_start (argptr,szMessage);
	vsprintf (msg,szMessage,argptr);
	va_end (argptr);	

	OutputDebugString(msg);

	Break();
}

#endif // _AFX_NO_AFXCMN_SUPPORT

