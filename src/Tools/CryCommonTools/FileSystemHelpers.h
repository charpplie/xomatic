#ifndef __FILESYSTEMHELPERS_H__
#define __FILESYSTEMHELPERS_H__

#include "Exceptions.h"

namespace FileSystemHelpers
{
	// Declare an exception to report when directory creation operations fail.
	struct DirectoryCreationFailedErrorTag;
	typedef Exception<DirectoryCreationFailedErrorTag> DirectoryCreationFailedError;

	unsigned GetFileFlags(const tstring& path);
	void EnsureDirectoryExists(const tstring& directory);
}

#endif //__FILESYSTEMHELPERS_H__
