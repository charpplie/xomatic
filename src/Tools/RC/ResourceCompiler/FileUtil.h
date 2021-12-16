////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   fileutil.h
//  Version:     v1.00
//  Created:     5/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __fileutil_h__
#define __fileutil_h__
#pragma once

namespace FileUtil
{
	//	Magic number explanation:
	//	Both epochs are Gregorian. 1970 - 1601 = 369. Assuming a leap
	//	year every four years, 369 / 4 = 92. However, 1700, 1800, and 1900
	//	were NOT leap years, so 89 leap years, 280 non-leap years.
	//	89 * 366 + 280 * 365 = 134744 days between epochs. Of course
	//	60 * 60 * 24 = 86400 seconds per day, so 134744 * 86400 =
	//	11644473600 = SECS_BETWEEN_EPOCHS.
	//
	//	This result is also confirmed in the MSDN documentation on how
	//	to convert a time_t value to a win32 FILETIME.
	#define SECS_BETWEEN_EPOCHS (__int64(11644473600))
		/* 10^7 */
	#define SECS_TO_100NS (__int64(10000000)) 

	// Find all files matching filespec.
	bool ScanDirectory( const string &path,const string &filespec,std::vector<string> &files,bool recursive, const char* dirToIgnore=0 );

	bool CreateDirectoryRecursive(const char* szPathIn);

	// converts the FILETIME to the C Timestamp (compatible with dbghelp.dll)
	inline DWORD FiletimeToUnixTime(const FILETIME& ft)
	{
		return (DWORD)((((__int64&)ft) / SECS_TO_100NS) - SECS_BETWEEN_EPOCHS);
	}

	// converts the C Timestamp (compatible with dbghelp.dll) to FILETIME
	inline FILETIME UnixTimeToFiletime(DWORD nCTime)
	{
		__int64 time = (nCTime + SECS_BETWEEN_EPOCHS) * SECS_TO_100NS;
		return (FILETIME&)time;
	}

	// returns file time stamps
	inline bool GetFileTimes( const char *filename,FILETIME *ftimeModify, FILETIME*ftimeCreate )
	{
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		hFind = FindFirstFile( filename,&FindFileData );
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		FindClose(hFind);
		if (ftimeCreate)
		{
			ftimeCreate->dwLowDateTime = FindFileData.ftCreationTime.dwLowDateTime;
			ftimeCreate->dwHighDateTime = FindFileData.ftCreationTime.dwHighDateTime;
		}
		if (ftimeModify)
		{
			ftimeModify->dwLowDateTime = FindFileData.ftLastWriteTime.dwLowDateTime;
			ftimeModify->dwHighDateTime = FindFileData.ftLastWriteTime.dwHighDateTime;
		}
		return true;
	}

	inline FILETIME GetLastWriteFileTime( const char *filename )
	{
		FILETIME fileTime;

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile( filename,&FindFileData );

		if (hFind == INVALID_HANDLE_VALUE)
		{
			fileTime.dwLowDateTime = 0;
			fileTime.dwHighDateTime = 0;
		}
		else
		{
			fileTime.dwLowDateTime = FindFileData.ftLastWriteTime.dwLowDateTime;
			fileTime.dwHighDateTime = FindFileData.ftLastWriteTime.dwHighDateTime;
			FindClose(hFind);
		}

		return fileTime;
	}

	inline bool FileTimeIsValid( const FILETIME& fileTime )
	{
		return (fileTime.dwLowDateTime != 0) || (fileTime.dwHighDateTime != 0);
	}

	inline bool FileTimesAreEqual(const FILETIME& fileTime0, const FILETIME& fileTime1)
	{
		return (fileTime0.dwLowDateTime == fileTime1.dwLowDateTime)
			&& (fileTime0.dwHighDateTime == fileTime1.dwHighDateTime);
	}

	inline void SetFileTimes(const char* const filename, const FILETIME& fileTime)
	{
		const HANDLE hf = CreateFile(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		SetFileTime(hf, &fileTime, &fileTime, &fileTime);
		CloseHandle(hf);
	}

	inline __int64 GetFileSize(const char* const filename)
	{
		WIN32_FILE_ATTRIBUTE_DATA fileAttr;

		const BOOL ok = GetFileAttributesEx(filename, GetFileExInfoStandard, &fileAttr);

		const __int64 fileSize = (__int64)
			((((unsigned __int64)((unsigned __int32)fileAttr.nFileSizeHigh)) << 32) | 
			((unsigned __int32)fileAttr.nFileSizeLow));

		return (ok && (fileSize >= 0)) ? fileSize : -1;
	}
};

#endif // __fileutil_h__
