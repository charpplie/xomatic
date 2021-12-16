/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
A way to manage writing a group of files, especially to get close to an atomic commit
Should also be easy to reimplement over a network protocol, etc.

-------------------------------------------------------------------------
History:
- 17:07:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "AtomicFiles.h"

//-----------------------------------------------------------------------

CAtomicFiles::CAtomicFiles(void)
{
}

//-----------------------------------------------------------------------

CAtomicFiles::~CAtomicFiles(void)
{
	UnlockFiles();
	Clear();
}

//-----------------------------------------------------------------------

void CAtomicFiles::AddFile( const char *path, const string &data )
{
	m_files.push_back(FileData());
	FileData &fileData = m_files.back();
	fileData.content = data;
	fileData.path = path;
}

//-----------------------------------------------------------------------

void CAtomicFiles::AddFile( const char *path, const string &data, unsigned int oldCrc32 )
{
	m_files.push_back(FileData());
	FileData &fileData = m_files.back();
	fileData.content = data;
	fileData.oldCrc32 = oldCrc32;
	fileData.validCrc = true;
	fileData.path = path;
}

//-----------------------------------------------------------------------

std::vector<string> CAtomicFiles::AcquireFiles( void )
{
	std::vector<string> failures;

	// Open with append to gain write access without actually "touching" the file
	std::vector<FileData>::iterator iter;
	for ( iter = m_files.begin(); iter != m_files.end(); iter++ )
	{
		if (!iter->handler)
			iter->handler = fopen(iter->path, "at");
		if (!iter->handler)
			failures.push_back(string(iter->path));
	}

	return failures;
}

//-----------------------------------------------------------------------

std::vector<string> CAtomicFiles::CheckCurrency( void )
{
	std::vector<string> failures;

	std::vector<FileData>::iterator iter;
	for ( iter = m_files.begin(); iter != m_files.end(); iter++ )
	{
	}
	return failures;
}

//-----------------------------------------------------------------------

std::vector<string> CAtomicFiles::WriteFiles( void )
{
	std::vector<string> failures;

	std::vector<FileData>::iterator iter;
	for ( iter = m_files.begin(); iter != m_files.end(); iter++ )
	{
		if (iter->handler)
		{
			fclose(iter->handler);
			iter->handler = NULL;
		}
		iter->handler = fopen(iter->path, "wt");
		if (iter->handler)
			fwrite( iter->content.c_str(), sizeof(char), iter->content.size(), iter->handler);
	}

	return failures;
}

//-----------------------------------------------------------------------

void CAtomicFiles::UnlockFiles( void )
{
	std::vector<FileData>::iterator iter;
	for ( iter = m_files.begin(); iter != m_files.end(); iter++ )
	{
		if (iter->handler)
		{
			fclose(iter->handler);
			iter->handler = NULL;
		}
	}
}

//-----------------------------------------------------------------------

void CAtomicFiles::Clear( void )
{
	UnlockFiles();
	m_files.clear();
}

//-----------------------------------------------------------------------

// Concatenate all files and append them to a file, with a message and optional timestamp
// Returns bool on success
bool CAtomicFiles::AppendToFile( const char * path, string message)
{
	// Open file for append
	FILE *file = fopen(path, "at");
	if (!file) return false;

	fprintf(file, "\n\n%s\n", message.c_str());

	bool success = true;
	std::vector<FileData>::iterator iter;
	for ( iter = m_files.begin(); iter != m_files.end(); iter++ )
			success &= ( fwrite( iter->content.c_str(), sizeof(char), 
				iter->content.size(), file) == iter->content.size() );

	fclose(file);

	return success;
}

//-----------------------------------------------------------------------

// Concatenate all files and dump to the log, with a message and optional timestamp
// Returns bool on success
void CAtomicFiles::DumpToLog( string message )
{
	// Have to do this line-by-line to avoid truncation
	ILog * pLog = gEnv->pLog;

	pLog->Log(message);

	std::vector<FileData>::iterator iter;
	for ( iter = m_files.begin(); iter != m_files.end(); iter++ )
	{	
		int curPos = 0;
		string line;
		while (curPos < iter->content.size()-1)
		{
			line = iter->content.Tokenize("\n\r", curPos);
			pLog->Log(line);
		}
	}
}
