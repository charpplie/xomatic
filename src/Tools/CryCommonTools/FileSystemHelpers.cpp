#include "StdAfx.h"
#include "FileSystemHelpers.h"
#include "StringHelpers.h"
#include "PathHelpers.h"

namespace FileSystemHelpers
{
	enum FileFlags
	{
		FileFlags_Exists = 1,
		FileFlags_Directory = 2
	};
}

unsigned FileSystemHelpers::GetFileFlags(const tstring& path)
{
	unsigned int uFlags = 0;

	WIN32_FIND_DATA findData;
	HANDLE hSearch = FindFirstFile(PathHelpers::RemoveSeparator(path).c_str(), &findData);
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		uFlags |= FileFlags_Exists;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			uFlags |= FileFlags_Directory;
		FindClose(hSearch);
	}

	return uFlags;
}

void FileSystemHelpers::EnsureDirectoryExists(const tstring& directory)
{
	tstring path = PathHelpers::RemoveSeparator(directory);

	// Check whether path is a UNC path.
	size_t pos = 0;
	if (StringHelpers::StartsWith(path, _T("\\\\")))
		pos = 1;
	else
		pos = path.find_first_of(_T("/\\"), 0); // Assume that path starts with a drive letter.

	while (pos != tstring::npos)
	{
		pos = path.find_first_of(_T("/\\"), pos + 1);
		tstring ancestorDirectory = path.substr(0, pos);
		if ((GetFileFlags(ancestorDirectory) & FileFlags_Directory) == 0)
		{
			if (!CreateDirectory(ancestorDirectory.c_str(), 0))
				throw DirectoryCreationFailedError(StringHelpers::ConvertString<string>(tstring(_T("Error creating directory '")) + path + _T("'.")));
		}
	}
}
