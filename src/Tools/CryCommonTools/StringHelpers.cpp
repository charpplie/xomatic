#include "StdAfx.h"
#include "StringHelpers.h"
#include <cctype>
#include <algorithm>

bool StringHelpers::StartsWith(const string& str, const string& pattern)
{
	if (str.length() < pattern.length())
	{
		return false;
	}
	return std::memcmp(str.data(), pattern.data(), pattern.length()) == 0;
}

bool StringHelpers::StartsWithIgnoreCase(const string& str, const string& pattern)
{
	if (str.length() < pattern.length())
	{
		return false;
	}
	return memicmp(str.data(), pattern.data(), pattern.length()) == 0;
}

bool StringHelpers::EndsWith(const string& str, const string& pattern)
{
	if (str.length() < pattern.length())
	{
		return false;
	}
	return std::memcmp(str.data() + str.length() - pattern.length(), pattern.data(), pattern.length()) == 0;
}

bool StringHelpers::EndsWithIgnoreCase(const string& str, const string& pattern)
{
	if (str.length() < pattern.length())
	{
		return false;
	}
	return memicmp(str.data() + str.length() - pattern.length(), pattern.data(), pattern.length()) == 0;
}

string StringHelpers::TrimLeft(const string& s)
{
	const size_t first = s.find_first_not_of(" \r\t");
	return (first == s.npos) ? string() : s.substr(first);
}

string StringHelpers::TrimRight(const string& s)
{
	const size_t last = s.find_last_not_of(" \r\t");
	return (last == s.npos) ? s : s.substr(0,last+1);
}

string StringHelpers::Trim(const string& s)
{
	return TrimLeft(TrimRight(s));
}

string StringHelpers::RemoveDuplicateSpaces(const string& s)
{
	string res;
	bool spaceFound = false;

	for (size_t i = 0, n = s.length(); i < n; ++i)
	{
		if ((s[i]==' ') || (s[i]=='\r') || (s[i]=='\t'))
		{
			spaceFound = true;
		}
		else
		{
			if (spaceFound)
			{
				res += ' ';
				spaceFound = false;
			}
			res += s[i];
		}
	}

	if (spaceFound)
	{
		res += ' ';
	}
	return res;
}

string StringHelpers::MakeLowerCase(const string& s)
{
	string copy = s;
	std::transform(copy.begin(), copy.end(), copy.begin(), tolower);
	return copy;
}

string StringHelpers::MakeUpperCase(const string& s)
{
	string copy = s;
	std::transform(copy.begin(), copy.end(), copy.begin(), toupper);
	return copy;
}

string StringHelpers::Replace(const string& s, char oldChar, char newChar)
{
	string copy = s;
	std::replace(copy.begin(), copy.end(), oldChar, newChar);
	return copy;
}

void StringHelpers::ConvertStringByRef(string& out, const string& in)
{
	out = in;
}


#if !defined(CRY_STRING)
bool StringHelpers::StartsWith(const wstring& str, const wstring& pattern)
{
	if (str.length() < pattern.length())
	{
		return false;
	}
	return std::memcmp(str.data(), pattern.data(), pattern.length()*sizeof(wstring::value_type)) == 0;
}

bool StringHelpers::StartsWithIgnoreCase(const wstring& str, const wstring& pattern)
{
	const size_t patternLength = pattern.length();
	if (str.length() < patternLength)
	{
		return false;
	}
	for (size_t i = 0; i < patternLength; ++i)
	{
		if (towlower(str[i]) != towlower(pattern[i]))
		{
			return false;
		}
	}
	return true;
}

bool StringHelpers::EndsWith(const wstring& str, const wstring& pattern)
{
	if (str.length() < pattern.length())
	{
		return false;
	}
	return std::memcmp(str.data() + str.length() - pattern.length(), pattern.data(), pattern.length()*sizeof(wstring::value_type)) == 0;
}

bool StringHelpers::EndsWithIgnoreCase(const wstring& str, const wstring& pattern)
{
	const size_t patternLength = pattern.length();
	if (str.length() < patternLength)
	{
		return false;
	}
	for (size_t i = str.length() - patternLength, j = 0; i < patternLength; ++i, ++j)
	{
		if (towlower(str[i]) != towlower(pattern[j]))
		{
			return false;
		}
	}
	return true;
}

wstring StringHelpers::TrimLeft(const wstring& s)
{
	const size_t first = s.find_first_not_of(L" \r\t");
	return (first == s.npos) ? wstring() : s.substr(first);
}

wstring StringHelpers::TrimRight(const wstring& s)
{
	const size_t last = s.find_last_not_of(L" \r\t");
	return (last == s.npos) ? s : s.substr(0,last+1);
}

wstring StringHelpers::Trim(const wstring& s)
{
	return TrimLeft(TrimRight(s));
}

wstring StringHelpers::RemoveDuplicateSpaces(const wstring& s)
{
	wstring res;
	bool spaceFound = false;

	for (size_t i = 0, n = s.length(); i < n; ++i)
	{
		if ((s[i]==' ') || (s[i]=='\r') || (s[i]=='\t'))
		{
			spaceFound = true;
		}
		else
		{
			if (spaceFound)
			{
				res += ' ';
				spaceFound = false;
			}
			res += s[i];
		}
	}

	if (spaceFound)
	{
		res += ' ';
	}
	return res;
}

wstring StringHelpers::MakeLowerCase(const wstring& s)
{
	wstring copy = s;
	std::transform(copy.begin(), copy.end(), copy.begin(), towlower);
	return copy;
}

wstring StringHelpers::MakeUpperCase(const wstring& s)
{
	wstring copy = s;
	std::transform(copy.begin(), copy.end(), copy.begin(), towupper);
	return copy;
}

wstring StringHelpers::Replace(const wstring& s, wchar_t oldChar, wchar_t newChar)
{
	wstring copy = s;
	std::replace(copy.begin(), copy.end(), oldChar, newChar);
	return copy;
}

void StringHelpers::ConvertStringByRef(wstring& out, const string& in)
{
	std::vector<wchar_t> buf(in.length() + 1);
	if (mbstowcs(&buf[0], in.c_str(), buf.size()) < 0)
	{
		out = L"";
	}
	else
	{
		out = &buf[0];
	}
}

void StringHelpers::ConvertStringByRef(string& out, const wstring& in)
{
	std::vector<char> buf(4 * (in.length()+1));
	if (wcstombs(&buf[0], in.c_str(), buf.size()) < 0)
	{
		out = "";
	}
	else
	{
		out = &buf[0];
	}
}

void StringHelpers::ConvertStringByRef(wstring& out, const wstring& in)
{
	out = in;
}
#endif //!defined(CRY_STRING)
