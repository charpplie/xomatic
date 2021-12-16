/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
A way to manage writing a group of files, especially to get close to an atomic commit.
Should also be easy to reimplement over a network protocol, etc.

-------------------------------------------------------------------------
History:
- 17:07:2006  : Created by Matthew Jack

Notes:
- 

*************************************************************************/

#ifndef __ATOMICFILES_H__
#define __ATOMICFILES_H__

//#include "StdAfx.h"

#pragma once

class CAtomicFiles
{
public:
	// Construct empty object
	CAtomicFiles(void);

	// Add a file, string data, with no currency metadata
	void AddFile( const char *path, const string &data );

	// Add a file, with an expected CRC32 of the _old_ file
	void AddFile( const char *path, const string &data, unsigned int oldCrc32 );

	// Lock the files
	// Returns all files that cannot be locked
	std::vector<string> AcquireFiles( void );

	// Check files for currency
	// Returns all files that are not current, including those with no metadata or that failed to open
	std::vector<string> CheckCurrency( void );

	// (Re)open all files, write content and close
	// Returns any files that cannot be reopened
	std::vector<string> WriteFiles( void );

	// Concatenate all files and append them to a file, with a message
	// Returns bool on success
	bool AppendToFile( const char * path, string message);

	// Concatenate all files and dump to the log, with a message
	void DumpToLog( string message);

	// Close and unlock all files
	void UnlockFiles( void );

	// Clear the object
	// Automatic Unlock
	void Clear( void );

	// Destructor
	// Automatic Unlock and Clear
	~CAtomicFiles(void);

protected:

	struct FileData {
		FileData(void) {
			handler = NULL;
			validCrc = false;
		}
		string content;
		string path;
		unsigned int oldCrc32;
		bool validCrc;
		FILE *handler;
	};

	std::vector<FileData> m_files;
};

#endif // __ATOMICFILES_H__