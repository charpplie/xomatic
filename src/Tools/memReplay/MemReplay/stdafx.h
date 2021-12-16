#pragma once

#define VC_EXTRALEAN
#define NOMINMAX

#define _WIN32_WINNT 0x0501

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#define _AFX_ALL_WARNINGS

#include <afxwin.h>
#include <afxext.h>
#include <afxcview.h>
#include <afxdisp.h>

#include <afxdtctl.h>
#include <afxcmn.h>

#include <algorithm>
#include <assert.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <deque>

#include <WS2tcpip.h>

#include <afxdlgs.h>

#include "ReplaySDK/common.h"
#include "ReplaySDK/SharedPtr.h"
#include "ReplaySDK/FileRAII.h"
#include "ReplaySDK/Allocator.h"
#include "ReplaySDK/Serialise.h"

#define for_citerator(ct, c, i) for (ct::const_iterator i = c.begin(), i##End = c.end(); i != i##End; ++ i)

namespace stdext
{

	template<typename FirstT, typename SecondT> inline
		size_t hash_value(const std::pair<FirstT, SecondT>& pair)
	{	// hash _Keyval to size_t value one-to-one
		return hash_value(pair.first) ^ hash_value(pair.second);
	}

}

enum
{
	MEMTYPE_MAIN,
	MEMTYPE_RSX,
	MEMTYPE_COMBINED,
	MEMTYPE_COUNT
};
