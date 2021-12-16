////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   PathUtil.h
//  Version:     v1.00
//  Created:     19/10/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: Defines namespace PathUtil for operations on files paths.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CryPath_h__
#define __CryPath_h__
#pragma once

#include <string>

namespace PathUtil
{
	//! Split full file name to path and filename
	//! @param filepath [IN] Full file name including path.
	//! @param path [OUT] Extracted file path.
	//! @param filename [OUT] Extracted file (without extension).
	//! @param ext [OUT] Extracted files extension.
	inline void Split( const std::string &filepath,std::string &path,std::string &filename,std::string &fext )
	{
		path = filename = fext = std::string();
		if (filepath.empty())
			return;
		const char *str = filepath.c_str();
		const char* pext = str + filepath.length()-1;
		for (const char* p = str + filepath.length()-1; p >= str; --p)
		{
			switch(*p)
			{
			case ':':
			case '/':
			case '\\':
				path = filepath.substr(0,p-str+1);
				filename = filepath.substr(p-str+1,pext-p);
				return;
			case '.':
				// there's an extension in this file name
				fext = filepath.substr(p-str+1);
				pext = p;
				break;
			}
		}
		filename = filepath;
	}

	//! Split full file name to path and filename
	//! @param filepath [IN] Full file name including path.
	//! @param path [OUT] Extracted file path.
	//! @param file [OUT] Extracted file (with extension).
	inline void Split( const std::string &filepath,std::string &path,std::string &file )
	{
		std::string fext;
		Split(filepath,path,file,fext);
		file += fext;
	}

	//! Extract extension from full specified file path.
	inline std::string GetExt( const std::string &filepath )
	{
		const char *str = filepath.c_str();
		for (const char* p = str + filepath.length()-1; p >= str; --p)
		{
			switch(*p)
			{
			case ':':
			case '/':
			case '\\':
				// we've reached a path separator - it means there's no extension in this name
				return std::string();
			case '.':
				// there's an extension in this file name
				return filepath.substr(p-str+1);
			}
		}
		return std::string();
	}

	//! Extract path from full specified file path.
	inline std::string GetPath( const std::string &filepath )
	{
		const char *str = filepath.c_str();
		for (const char* p = str + filepath.length()-1; p >= str; --p)
		{
			switch(*p)
			{
			case ':':
			case '/':
			case '\\':
				return filepath.substr(0,p-str+1);
			}
		}
		return filepath;
	}

	//! Extract file name with extension from full specified file path.
	inline std::string GetFile( const std::string &filepath )
	{
		const char *str = filepath.c_str();
		for (const char* p = str + filepath.length()-1; p >= str; --p)
		{
			switch(*p)
			{
			case ':':
			case '/':
			case '\\':
				return filepath.substr(p-str+1);
			}
		}
		return filepath;
	}

	//! Replace extension for given file.
	inline void RemoveExtension( std::string &filepath )
	{
		const char *str = filepath.c_str();
		for (const char* p = str + filepath.length()-1; p >= str; --p)
		{
			switch(*p)
			{
			case ':':
			case '/':
			case '\\':
				// we've reached a path separator - it means there's no extension in this name
				return;
			case '.':
				// there's an extension in this file name
				filepath = filepath.substr(0,p-str);
				return;
			}
		}
		// it seems the file name is a pure name, without path or extension
	}

	//! Extract file name without extension from full specified file path.
	inline std::string GetFileName( const std::string &filepath )
	{
		std::string file = filepath;
		RemoveExtension(file);
		return GetFile(file);
	}

	//! Removes the trailing slash or backslash from a given path.
	inline std::string RemoveSlash( const std::string &path )
	{
		if (path.empty() || (path[path.length()-1] != '/' && path[path.length()-1] != '\\'))
			return path;
		return path.substr(0,path.length()-1);
	}

	//! add a backslash if needed
	inline std::string AddBackslash( const std::string &path )
	{
		if (path.empty() || path[path.length()-1] == '\\')
			return path;
		if (path[path.length()-1] == '/')
			return path.substr(0,path.length()-1) + "\\";
		return path + "\\";
	}

	//! Replace extension for given file.
	inline std::string ReplaceExtension( const std::string &filepath,const std::string &ext )
	{
		std::string str = filepath;
		RemoveExtension(str);
		if (!ext.empty() && ext[0] != '.')
		{
			str += ".";
		}
		str += ext;
		return str;
	}

	//! Makes a fully specified file path from path and file name.
	inline std::string Make( const std::string &path,const std::string &file )
	{
		return AddBackslash(path) + file;
	}

	//! Makes a fully specified file path from path and file name.
	inline std::string Make( const std::string &dir,const std::string &filename,const std::string &ext )
	{
		std::string path = filename;
		path = ReplaceExtension(path,ext);
		path = AddBackslash(dir) + path;
		return path;
	}

	//! Makes a fully specified file path from path and file name.
	inline std::string MakeFullPath( const std::string &relativePath )
	{
		char path_buffer[_MAX_PATH];
		if (_fullpath(path_buffer,relativePath.c_str(),_MAX_PATH))
		{
			return path_buffer;
		}
		else
			return relativePath;
	}

	inline std::string GetParentDirectory (const std::string& strFilePath, int nGeneration = 1)
	{
		for (const char* p = strFilePath.c_str() + strFilePath.length() - 2; // -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
			p >= strFilePath.c_str();
			--p)
		{
			switch (*p)
			{
			case ':':
				return std::string (strFilePath.c_str(), p);
				break;
			case '/':
			case '\\':
				// we've reached a path separator - return everything before it.
				if (!--nGeneration)
					return std::string(strFilePath.c_str(), p);
				break;
			}
		};
		// it seems the file name is a pure name, without path or extension
		return std::string();
	}

	inline std::string StripDirectoryFromFront(const std::string& sPath)
	{
		int nSlashPosition = sPath.find('\\');
		if (nSlashPosition == std::string::npos)
			return "";
		return sPath.substr(nSlashPosition + 1);
	}

	// Assumes that directories are separated by '\\'.
	inline std::string GetDirectoryFromFront(const std::string& sPath)
	{
		int nSlashPosition = sPath.find('\\');
		if (nSlashPosition == std::string::npos)
			return sPath;
		return sPath.substr(0, nSlashPosition);
	}

	inline std::string GetRelativePath(const std::string& sDirectory, const std::string& sPath)
	{
		// Convert both paths to a canonical form.
		char szBuffer[_MAX_PATH];
		GetFullPathName(sDirectory.c_str(), _MAX_PATH, szBuffer, 0);
		GetLongPathName(szBuffer, szBuffer, _MAX_PATH);
		std::string sCanonicalDirectory = szBuffer;
		GetFullPathName(sPath.c_str(), _MAX_PATH, szBuffer, 0);
		GetLongPathName(szBuffer, szBuffer, _MAX_PATH);
		std::string sCanonicalPath = szBuffer;

		// Replace / with \ in paths.
		std::replace(sCanonicalDirectory.begin(), sCanonicalDirectory.end(), '/', '\\');
		std::replace(sCanonicalPath.begin(), sCanonicalPath.end(), '/', '\\');

		// Check whether the paths are on the same drive - this works for UNC paths, but only
		// by a little bit of luck. Still, fixing it would only make the code more complex without
		// any benefit, so leave it as is - MichaelS.
		if (GetDirectoryFromFront(sCanonicalPath) != GetDirectoryFromFront(sCanonicalDirectory))
			return "";
		sCanonicalDirectory = StripDirectoryFromFront(sCanonicalDirectory);
		sCanonicalPath = StripDirectoryFromFront(sCanonicalPath);

		// Loop until we find directories that differ.
		while (GetDirectoryFromFront(sCanonicalPath) == GetDirectoryFromFront(sCanonicalDirectory) &&
			sCanonicalPath != "" && sCanonicalDirectory != "")
		{
			sCanonicalDirectory = StripDirectoryFromFront(sCanonicalDirectory);
			sCanonicalPath = StripDirectoryFromFront(sCanonicalPath);
		}

		// The relative path needs to move us out of the current directory and into the new directory.
		// Begin by creating a path that moves out of the current directory.
		std::string sRelativePath = "";
		while (GetDirectoryFromFront(sCanonicalDirectory) != "")
		{
			sCanonicalDirectory = StripDirectoryFromFront(sCanonicalDirectory);
			sRelativePath += "..\\";
		}

		// Now add the path from the parent directory to the child directory.
		std::string sDirectoryName;
		while ((sDirectoryName = GetDirectoryFromFront(sCanonicalPath)) != "")
		{
			sCanonicalPath = StripDirectoryFromFront(sCanonicalPath);
			sRelativePath += sDirectoryName;
			sRelativePath += "\\";
		}

		// Remove the final slash.
		sRelativePath = RemoveSlash(sRelativePath);

		return sRelativePath;
	}

	inline bool IsAbsolutePath(const std::string& sPath)
	{
		return sPath.size() >= 2 && (sPath[1] == ':' || sPath.substr(0, 2) == "\\\\");
	}

	inline bool IsInDirectory(const std::string& sDirectory, const std::string& sPath)
	{
		// Try to find a relative path from the directory to the absolute path. Note if sPath
		// is already a relative path, it will be converted to a full path by GetRelativePath.
		std::string sRelativePath = GetRelativePath(sDirectory, sPath);

		// If the two paths are on different drives, it is impossible to create a relative path,
		// so GetRelativePath returns "".
		if (sRelativePath == "")
			return false;

		// If sPath is not within sDirectory, then the relative path will start with ..
		if (sRelativePath.substr(0, 2) == "..")
			return false;

		return true;
	}

	// returns true if the std::string matches the wildcard
	inline bool MatchWildcard (const char* szString, const char* szWildcard)
	{
		const char* pString = szString, *pWildcard = szWildcard;
		// skip the obviously the same starting substring
		while (*pWildcard && *pWildcard != '*' && *pWildcard != '?')
			if (*pString != *pWildcard)
				return false; // must be exact match unless there's a wildcard character in the wildcard std::string
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
					return true; // the rest of the std::string doesn't matter: the wildcard ends with *

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
};

#endif //__CryPath_h__
