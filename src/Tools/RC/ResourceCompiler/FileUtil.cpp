////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   fileutil.cpp
//  Version:     v1.00
//  Created:     5/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FileUtil.h"
#include <StringUtils.h>

#include <io.h>

//////////////////////////////////////////////////////////////////////////
// returns true if 'dir' is a subdirectory of 'baseDir' or same directory as 'baseDir' 
// note: returns false in case of wrong names passed
static bool IsSubdirOrSameDir(const char* dir, const char* baseDir)
{
	char szFullPathDir[2*1024];
	if (!_fullpath(szFullPathDir, dir, sizeof(szFullPathDir)))
	{
		return false;
	}

	char szFullPathBaseDir[2*1024];
	if (!_fullpath(szFullPathBaseDir, baseDir, sizeof(szFullPathBaseDir)))
	{
		return false;
	}

	const char* p = szFullPathDir;
	const char* q = szFullPathBaseDir;
	for(;;++p, ++q)
	{
		if (tolower(*p) == tolower(*q))
		{
			if (*p == 0)
			{
				// dir is exactly same as baseDir
				return true;
			}
			continue;
		}

		if ((*p=='/' || *p=='\\') && (*q=='/' || *q=='\\'))
		{
			continue;
		}

		if (*p == 0)
		{
			// dir length is shorter than baseDir length. so it's not a subdir
			return false;			
		}

		if (*q == 0)
		{
			// baseDir is shorter than dir. so may be it's a subdir.
			const bool isSubdir = (*p=='/' || *p=='\\');
			return isSubdir;
		}

		return false;
	}
}

//////////////////////////////////////////////////////////////////////////
// the paths must have trailing slash
static bool ScanDirectoryRecursive( const string &root,const string &path,const string &file,std::vector<string> &files, bool recursive, const char* dirToIgnore )
{
	__finddata64_t c_file;
	intptr_t hFile;

	bool anyFound = false;

	string fullPath = PathHelpers::Join(root, path);

	if (dirToIgnore)
	{
		if (IsSubdirOrSameDir(fullPath.c_str(), dirToIgnore))
		{
			return anyFound;
		}
	}

	fullPath = PathHelpers::Join(fullPath, file);

	if ( (hFile = _findfirst64( fullPath.c_str(), &c_file )) != -1L )
	{
		// Find the rest of the files.
		do {
			if ((!(c_file.attrib & _A_SUBDIR)) && CryStringUtils::MatchWildcardIgnoreCase(c_file.name, file.c_str()))
			{
				anyFound = true;
				files.push_back( PathHelpers::Join(path, c_file.name) );
			}
		}	while (_findnext64( hFile, &c_file ) == 0);
		_findclose( hFile );
	}

	if (recursive)
	{
		fullPath = PathHelpers::Join(PathHelpers::Join(root, path), "*.*");
		if( (hFile = _findfirst64( fullPath.c_str(), &c_file )) != -1L )
		{
			// Find directories.
			do {
				if (c_file.attrib & _A_SUBDIR)
				{
					// If recursive.
					if (c_file.name[0] != '.')
					{
						if (ScanDirectoryRecursive( root,PathHelpers::Join(path, c_file.name),file,files,recursive,dirToIgnore ))
							anyFound = true;
					}
				}
			}	while (_findnext64( hFile, &c_file ) == 0);
			_findclose( hFile );
		}
	}

	return anyFound;
}

//////////////////////////////////////////////////////////////////////////
namespace FileUtil
{
	bool ScanDirectory( const string &path,const string &file,std::vector<string> &files, bool recursive, const char* dirToIgnore )
	{
		return ScanDirectoryRecursive(path,"",file,files,recursive,dirToIgnore );
	}

	bool CreateDirectoryRecursive(const char* szPathIn)
	{
		char tempPath[MAX_PATH];
		strcpy_s(tempPath,szPathIn);
		
		const char *szPath = tempPath;

		char newPath[MAX_PATH];
		char *q = newPath;

		memset(newPath,0,sizeof(newPath));

		const char* p = szPath;
		bool bUNCPath = false;
		// Check for UNC path.
		if (strlen(szPath) > 2 && szPath[0] == '\\' && szPath[1] == '\\')
		{
			// UNC path given, Skip first 2 slashes.
			*q++ = *p++;
			*q++ = *p++;
		}

		for (; *p; )
		{
			while (*p != '\\' && *p != '/' && *p)
			{
				*q++ = *p++;
			}
			_mkdir( newPath );
			if (*p)
			{
				if (*p != '/')
				{
					*q++ = *p++;
				}
				else
				{
					*q++ = '\\';
					p++;
				}
			}
		}
		if (_mkdir(szPath) != ENOENT)
			return true;
		return false;
	}
}
