#ifndef __STRINGHELPERS_H__
#define __STRINGHELPERS_H__

namespace StringHelpers
{
	bool StartsWith(const string& str, const string& pattern);
	bool StartsWithIgnoreCase(const string& str, const string& pattern);
	bool EndsWith(const string& str, const string& pattern);
	bool EndsWithIgnoreCase(const string& str, const string& pattern);
	string TrimLeft(const string& s);
	string TrimRight(const string& s);
	string Trim(const string& s);
	string RemoveDuplicateSpaces(const string& s);
	string MakeLowerCase(const string& s);
	string MakeUpperCase(const string& s);
	string Replace(const string& s, char oldChar, char newChar);

	template <typename O, typename I> O ConvertString(const I& in)
	{
		O out;
		ConvertStringByRef(out, in);
		return out;
	}

	void ConvertStringByRef(string& out, const string& in);

#if !defined(CRY_STRING)
	bool StartsWith(const wstring& str, const wstring& pattern);
	bool StartsWithIgnoreCase(const wstring& str, const wstring& pattern);
	bool EndsWith(const wstring& str, const wstring& pattern);
	bool EndsWithIgnoreCase(const wstring& str, const wstring& pattern);
	wstring TrimLeft(const wstring& s);
	wstring TrimRight(const wstring& s);
	wstring Trim(const wstring& s);
	wstring RemoveDuplicateSpaces(const wstring& s);
	wstring MakeLowerCase(const wstring& s);
	wstring MakeUpperCase(const wstring& s);
	wstring Replace(const wstring& s, wchar_t oldChar, wchar_t newChar);

	void ConvertStringByRef(wstring& out, const string& in);
	void ConvertStringByRef(string& out, const wstring& in);
	void ConvertStringByRef(wstring& out, const wstring& in);
#endif //!defined(CRY_STRING)
}

#endif //__STRINGHELPERS_H__
