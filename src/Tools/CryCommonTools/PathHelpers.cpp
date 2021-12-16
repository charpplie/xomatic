#include "StdAfx.h"
#include "PathHelpers.h"

string PathHelpers::FindExtension(const string& path)
{
	size_t periodPos = path.rfind('.');
	size_t baseLength = (periodPos != string::npos ? periodPos + 1 : path.length());
	return path.substr(baseLength, string::npos);
}

string PathHelpers::ReplaceExtension(const string& path, const string& newExtension)
{
	size_t periodPos = path.rfind('.');
	size_t baseLength = (periodPos != string::npos ? periodPos : path.length());
	return path.substr(0, baseLength) + "." + newExtension;
}

string PathHelpers::RemoveExtension(const string& path)
{
	size_t periodPos = path.rfind('.');
	size_t baseLength = (periodPos != string::npos ? periodPos : path.length());
	return path.substr(0, baseLength);
}

string PathHelpers::GetDirectory(const string& path)
{
	int slashPos;
	for (slashPos = int(path.size()) - 1; slashPos >= 0 && strchr("/\\", path[slashPos]) == 0; --slashPos);
	if (slashPos == -1)
		return "";
	return path.substr(0, slashPos);
}

string PathHelpers::GetFilename(const string& path)
{
	int slashPos;
	for (slashPos = int(path.size()) - 1; slashPos >= 0 && strchr("/\\", path[slashPos]) == 0; --slashPos);
	if (slashPos == -1)
		return path;
	return path.substr(slashPos + 1, string::npos);
}

string PathHelpers::AddSeparator(const string& path)
{
	if (path.empty())
		return string("");
	int last = path[path.length() - 1];
	if (last != '/' && last != '\\')
		return path + '\\';
	return path;
}

string PathHelpers::RemoveSeparator(const string& path)
{
	if (path.empty())
		return path;
	int last = path[path.length() - 1];
	if (last != '/' && last != '\\')
		return path;
	return path.substr(0, path.length() - 1);
}

string PathHelpers::Join(const string& path1, const string& path2)
{
	if (path1.empty())
		return path2;
	return PathHelpers::AddSeparator(path1) + path2;
}

string PathHelpers::GetParentDirectory(const string& path)
{
	string pathWithoutSlash = PathHelpers::RemoveSeparator(path);
	return PathHelpers::GetDirectory(path);
}

bool PathHelpers::MatchWildcard(const string& text, const string& wildcard)
{
	const char* pString = &text[0], *pWildcard = &wildcard[0];
	// skip the obviously the same starting substring
	while (*pWildcard && *pWildcard != '*' && *pWildcard != '?')
		if (*pString != *pWildcard)
			return false; // must be exact match unless there's a wildcard character in the wildcard string
		else
			++pString, ++pWildcard;

	if (!*pString)
	{
		// this will only match if there are no non-wild characters in the wildcard
		for(; *pWildcard; ++pWildcard)
			if (*pWildcard != '*' && *pWildcard != '?')
				return false;
		return true;
	}

	switch(*pWildcard)
	{
	case '\0':
		return false; // the only way to match them after the leading non-wildcard characters is !*pString, which was already checked

		// we have a wildcard with wild character at the start.
	case '*':
		{
			// merge consecutive ? and *, since they are equivalent to a single *
			while (*pWildcard == '*' || *pWildcard == '?')
				++pWildcard;

			if (!*pWildcard)
				return true; // the rest of the string doesn't matter: the wildcard ends with *

			for (; *pString; ++pString)
				if (MatchWildcard(pString, pWildcard))
					return true;

			return false;
		}
	case '?':
		return MatchWildcard(pString+1, pWildcard + 1) || MatchWildcard(pString, pWildcard+1);
	default:
		assert (0);
		return false;
	}
}

bool PathHelpers::IsRelative(const string& path)
{
	bool isRelative = true;
	isRelative = isRelative && (path.size() < 2 || path[1] != ':'); // Does path start with drive specifier?
	isRelative = isRelative && (path.size() < 2 || path[0] != '\\' || path[1] != '\\'); // Is UNC path?
	isRelative = isRelative && (path.size() < 1 || (path[0] != '\\' && path[0] != '/')); // Starts with slash?
	return isRelative;
}

#if !defined(CRY_STRING)
wstring PathHelpers::FindExtension(const wstring& path)
{
	size_t periodPos = path.rfind('.');
	size_t baseLength = (periodPos != wstring::npos ? periodPos + 1 : path.length());
	return path.substr(baseLength, wstring::npos);
}

wstring PathHelpers::ReplaceExtension(const wstring& path, const wstring& newExtension)
{
	size_t periodPos = path.rfind('.');
	size_t baseLength = (periodPos != wstring::npos ? periodPos : path.length());
	return path.substr(0, baseLength) + L"." + newExtension;
}

wstring PathHelpers::RemoveExtension(const wstring& path)
{
	size_t periodPos = path.rfind('.');
	size_t baseLength = (periodPos != wstring::npos ? periodPos : path.length());
	return path.substr(0, baseLength);
}

wstring PathHelpers::GetDirectory(const wstring& path)
{
	int slashPos;
	for (slashPos = int(path.size()) - 1; slashPos >= 0 && strchr("/\\", path[slashPos]) == 0; --slashPos);
	if (slashPos == -1)
		return L"";
	return path.substr(0, slashPos);
}

wstring PathHelpers::GetFilename(const wstring& path)
{
	int slashPos;
	for (slashPos = int(path.size()) - 1; slashPos >= 0 && strchr("/\\", path[slashPos]) == 0; --slashPos);
	if (slashPos == -1)
		return path;
	return path.substr(slashPos + 1, wstring::npos);
}

wstring PathHelpers::AddSeparator(const wstring& path)
{
	if (path.empty())
		return wstring(L"");
	int last = path[path.length() - 1];
	if (last != '/' && last != '\\')
		return path + L'\\';
	return path;
}

wstring PathHelpers::RemoveSeparator(const wstring& path)
{
	if (path.empty())
		return path;
	int last = path[path.length() - 1];
	if (last != '/' && last != '\\')
		return path;
	return path.substr(0, path.length() - 1);
}

wstring PathHelpers::Join(const wstring& path1, const wstring& path2)
{
	if (path1.empty())
		return path2;
	return PathHelpers::AddSeparator(path1) + path2;
}

wstring PathHelpers::GetParentDirectory(const wstring& path)
{
	wstring pathWithoutSlash = PathHelpers::RemoveSeparator(path);
	return PathHelpers::GetDirectory(path);
}

bool PathHelpers::MatchWildcard(const wstring& text, const wstring& wildcard)
{
	const wchar_t* pString = &text[0], *pWildcard = &wildcard[0];
	// skip the obviously the same starting substring
	while (*pWildcard && *pWildcard != '*' && *pWildcard != '?')
		if (*pString != *pWildcard)
			return false; // must be exact match unless there's a wildcard character in the wildcard wstring
		else
			++pString, ++pWildcard;

	if (!*pString)
	{
		// this will only match if there are no non-wild characters in the wildcard
		for(; *pWildcard; ++pWildcard)
			if (*pWildcard != '*' && *pWildcard != '?')
				return false;
		return true;
	}

	switch(*pWildcard)
	{
	case '\0':
		return false; // the only way to match them after the leading non-wildcard characters is !*pString, which was already checked

		// we have a wildcard with wild character at the start.
	case '*':
		{
			// merge consecutive ? and *, since they are equivalent to a single *
			while (*pWildcard == '*' || *pWildcard == '?')
				++pWildcard;

			if (!*pWildcard)
				return true; // the rest of the wstring doesn't matter: the wildcard ends with *

			for (; *pString; ++pString)
				if (MatchWildcard(pString, pWildcard))
					return true;

			return false;
		}
	case '?':
		return MatchWildcard(pString+1, pWildcard + 1) || MatchWildcard(pString, pWildcard+1);
	default:
		assert (0);
		return false;
	}
}

bool PathHelpers::IsRelative(const wstring& path)
{
	bool isRelative = true;
	isRelative = isRelative && (path.size() < 2 || path[1] != ':'); // Does path start with drive specifier?
	isRelative = isRelative && (path.size() < 2 || path[0] != '\\' || path[1] != '\\'); // Is UNC path?
	isRelative = isRelative && (path.size() < 1 || (path[0] != '\\' && path[0] != '/')); // Starts with slash?
	return isRelative;
}
#endif //!defined(CRY_STRING)
