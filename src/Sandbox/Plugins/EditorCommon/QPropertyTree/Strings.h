#pragma once

#ifndef SERIALIZATION_STANDALONE
#include <platform.h>

typedef CryStringT<char> string;
typedef CryStringT<wchar_t> wstring;
#else
#include <string>
using std::string;
using std::wstring;
#endif
