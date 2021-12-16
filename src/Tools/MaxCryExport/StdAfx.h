#ifndef __STDAFX_H__
#define __STDAFX_H__

// Turn off debug stl stuff.
#define _HAS_ITERATOR_DEBUGGING 0
#define _SECURE_SCL 0

// Turn of _DEBUG around stl headers - this is to stop the classes having a first iterator pointer,
// which breaks some binary compatibility. This is only really needed because the Max morpher modifier
// exposes a std::vector as part of the interface to the dll and for some reason expects that to work.
#if defined(_DEBUG)
#	undef _DEBUG
#endif //defined(_DEBUG)

#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <set>
using std::string;
typedef std::string tstring;

//#define NOT_USE_CRY_MEMORY_MANAGER

// Just here so shared files that #include "stdafx.h" can compile.
#include <assert.h>

#define CRY_ASSERT_TRACE assert

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

#define NO_XENON_INTRINSICS // Make sure we don't end up using xenon code.
//#define NOT_USE_CRY_STRING // defined in project settings

#include "max.h"
#include "UtilExp.h"
#include "modstack.h"
#include "bipexp.h"
#include "phyexp.h"
#include "stdmat.h"
#include "iparamm2.h"
#include "commdlg.h"
#include "utilapi.h"
#include "macrorec.h"
#include "decomp.h" 
#include "dummy.h"
#include "assert1.h"

// [MichaelS] Dodgy workaround to allow ISystem.h to parse correctly.
struct IXmlUtils;

#define Ray CryRay

// workaround to avoid namespace collision with cryengine clamp
#define clamp clamp_cryengine

// workaround to avoid conflicts between 'empty()' defined in <ivec.h> and 'empty' declared in Max's "Value.h"
#define empty CryEmpty
#include <intrin.h>
#undef empty

// [MichaelS] Dodgy workaround to avoid name clash between Array and a Max class of that name.
#define Array CryArray
#include <platform.h>
#include "CryHeaders.h"
#undef Array

#undef clamp

#undef Ray

#include "StlUtils.h"
#include <smartptr.h>

// In v9 Max replaced NoRemap() with DefaultRemapDir()
#if (MAX_PRODUCT_VERSION_MAJOR >= 9)
#	define MAX_DEFAULT_REMAP DefaultRemapDir
#else // (MAX_PRODUCT_VERSION_MAJOR < 9)
#	define MAX_DEFAULT_REMAP NoRemap
#endif //(MAX_PRODUCT_VERSION_MAJOR < 9)

#if defined(_DEBUG)
#	undef _DEBUG
#	include <atlbase.h>
#	define _DEBUG
#else //defined(_DEBUG)
#	include <atlbase.h>
#endif //defined(_DEBUG)
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#endif