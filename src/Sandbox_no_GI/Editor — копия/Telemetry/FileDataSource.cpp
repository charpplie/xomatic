////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   FileDataSource.cpp
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Load new-format data from file
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FileDataSource.h"

#include "TelemetryRepository.h"

namespace Telemetry
{

	CFileDataSource::CFileDataSource(CTelemetryRepository& repo)
		: m_repository(repo)
	{
	}

	CFileDataSource::~CFileDataSource()
	{
	}

	//////////////////////////////////////////////////////////////////////////

	bool CFileDataSource::Open()
	{
		char filename[_MAX_PATH];

		if(OpenFile(filename, sizeof(filename), "XML files (*.xml)\0*.xml\0", "Choose telemetry file to open"))
		{
			// TODO: process file contents
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CFileDataSource::Update()
	{
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CFileDataSource::Close()
	{
		
	}

	//////////////////////////////////////////////////////////////////////////
	bool CFileDataSource::OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title)
	{
		if(!filename || !bufSize)
			return false;

		filename[0] = '\0';

		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(OPENFILENAME);

		//ofn.hwndOwner = m_hWnd;
		ofn.lpstrFilter = filter;
		ofn.Flags = OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = bufSize;
		ofn.lpstrTitle = title;

		return GetOpenFileName(&ofn) != 0;
	}
}