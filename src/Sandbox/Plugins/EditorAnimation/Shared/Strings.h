#pragma once
#include <platform.h>

#if defined(RESOURCE_COMPILER)
typedef CryStringLocalT<char> string;
typedef CryStringLocalT<wchar_t> wstring;
#else
typedef CryStringT<char> string;
typedef CryStringT<wchar_t> wstring;
#endif

template <class Type>
struct less_stricmp : public std::binary_function<Type,Type,bool> 
{
	bool operator()( const string& left,const string& right ) const
	{
		return _stricmp(left.c_str(),right.c_str()) < 0;
	}
	bool operator()( const char* left,const char* right ) const
	{
		return _stricmp(left, right) < 0;
	}
};
