#ifndef __PATH_H__
#define __PATH_H__

namespace PathHelpers
{
	string FindExtension(const string& path);
	string ReplaceExtension(const string& path, const string& newExtension);
	string RemoveExtension(const string& path);
	string GetDirectory(const string& path);
	string GetFilename(const string& path);
	string AddSeparator(const string& path);
	string RemoveSeparator(const string& path);
	string Join(const string& path1, const string& path2);
	string GetParentDirectory(const string& path);
	bool MatchWildcard(const string& text, const string& wildcard);
	bool IsRelative(const string& path);

#if !defined(CRY_STRING)
	wstring FindExtension(const wstring& path);
	wstring ReplaceExtension(const wstring& path, const wstring& newExtension);
	wstring RemoveExtension(const wstring& path);
	wstring GetDirectory(const wstring& path);
	wstring GetFilename(const wstring& path);
	wstring AddSeparator(const wstring& path);
	wstring RemoveSeparator(const wstring& path);
	wstring Join(const wstring& path1, const wstring& path2);
	wstring GetParentDirectory(const wstring& path);
	bool MatchWildcard(const wstring& text, const wstring& wildcard);
	bool IsRelative(const wstring& path);
#endif //!defined(CRY_STRING)
}

#endif //__PATH_H__
