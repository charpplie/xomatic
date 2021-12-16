#pragma once

#define NOMINMAX
#define VC_EXTRALEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <tchar.h>

#include <algorithm>
#include <assert.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <time.h>

#include "ReplaySdk/common.h"

#include "ReplaySdk/SharedPtr.h"
#include "ReplaySdk/FileRAII.h"

#include "ReplaySdk/Allocator.h"
#include "ReplaySdk/Serialise.h"

typedef u32 TAddress;
typedef u32 TThreadId;

#define for_citerator(ct, c, i) for (ct::const_iterator i = c.begin(), i##End = c.end(); i != i##End; ++ i)

namespace stdext
{

	template<typename FirstT, typename SecondT> inline
		size_t hash_value(const std::pair<FirstT, SecondT>& pair)
	{	// hash _Keyval to size_t value one-to-one
		return hash_value(pair.first) ^ hash_value(pair.second);
	}

}
