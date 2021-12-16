#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma once

// Turn off debug stl stuff.
#define _HAS_ITERATOR_DEBUGGING 0
#define _SECURE_SCL 0

#define NOT_USE_CRYMEMORYMANAGER
#define NOT_USE_CRY_MEMORY_MANAGER
#define NOT_USE_CRY_STRING

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

#include <string>
typedef std::string string;
typedef std::string tstring;

#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <set>
using std::string;
typedef std::string tstring;

#define NO_XENON_INTRINSICS // Make sure we don't end up using xenon code.
//#define NOT_USE_CRY_STRING // defined in project settings

//#include "max.h"
//#include "UtilExp.h"
//#include "modstack.h"
//#include "bipexp.h"
//#include "phyexp.h"
//#include "stdmat.h"
//#include "iparamm2.h"
//#include "commdlg.h"
//#include "utilapi.h"
//#include "macrorec.h"
//#include "decomp.h" 
//#include "dummy.h"

#include <assert.h>

#define CRY_ASSERT_TRACE assert
#define CRY_ASSERT assert

#include <platform.h>
#include <tchar.h>

// Turn of _DEBUG around stl headers - this is to stop the classes having a first iterator pointer,
// which breaks some binary compatibility. This is only really needed because the Max morpher modifier
// exposes a std::vector as part of the interface to the dll and for some reason expects that to work.
#if defined(_DEBUG)
#	undef _DEBUG
#endif //defined(_DEBUG)

#include <string>
using std::string;

#include <map>
#include <set>
#include "StlUtils.h"

#include <Windows.h>
#include <smartptr.h>

#endif //__STDAFX_H__