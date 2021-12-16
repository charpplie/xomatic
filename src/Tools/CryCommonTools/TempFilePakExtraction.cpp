////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TempFilePakExtraction.cpp
//  Version:     v1.00
//  Created:     14/5/2008 by Doug Binks (douglas).
//  Compilers:   Visual Studio.NET 2005
//  Description: Opens a temporary file for read only access, where the file could be
//				 located in a zip or pak file. Note that if the file specified
//				 already exists it does not delete it when finished.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "TempFilePakExtraction.h"
#include "PathHelpers.h"
#include "IPakSystem.h"


TempFilePakExtraction::TempFilePakExtraction(const char* filename, IPakSystem* pPakSystem)
: m_strOriginalFileName(filename)
, m_strTempFileName(filename)
{
	if (!pPakSystem)
	{
		return;
	}

	{
		FILE* const fileOnDisk = fopen(m_strOriginalFileName.c_str(), "rb");
		if (fileOnDisk)
		{
			fclose(fileOnDisk);
			return;
		}
	}

	// use temporary filename and path

	// Get the temp path.
	static const DWORD BUFSIZE = 512;
	TCHAR szTempName[BUFSIZE] = "";
	static const DWORD dwBufSize = BUFSIZE;
	TCHAR lpPathBuffer[BUFSIZE];
	const DWORD dwRetVal = GetTempPath(
		dwBufSize,     // length of the buffer
		lpPathBuffer); // buffer for path 
	if ((dwRetVal > dwBufSize) || (dwRetVal == 0))
	{
		return;
	}

	// Create a temporary file. 
	const UINT uRetVal = GetTempFileName(
		lpPathBuffer, // directory for tmp files
		TEXT("RC_"),  // temp file name prefix
		0,            // create unique name
		szTempName);  // buffer for name
	if (uRetVal == 0)
	{
		return;
	}

	const string fileExt = PathHelpers::FindExtension(m_strOriginalFileName);
	const string strTempName = PathHelpers::ReplaceExtension(string(szTempName), fileExt);

	if (pPakSystem->ExtractNoOverwrite(m_strOriginalFileName.c_str(), strTempName.c_str()))
	{
		m_strTempFileName = strTempName;
		// make file to read only
		_chmod(m_strTempFileName.c_str(), _S_IREAD);
	}
}


TempFilePakExtraction::~TempFilePakExtraction()
{
	if (m_strOriginalFileName != m_strTempFileName)
	{
		// delete temporary file (must enable writing before doing so)
		_chmod(m_strTempFileName.c_str(), _S_IREAD | _S_IWRITE);
		DeleteFile(m_strTempFileName.c_str());
	}
}
