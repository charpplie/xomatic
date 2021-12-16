////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   XmlDataSource.cpp
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Load xml data using StatsTool
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "XmlDataSource.h"

#include "TelemetryRepository.h"

namespace Telemetry
{

CXmlDataSource::CXmlDataSource(CTelemetryRepository& repo)
: m_repository(repo)
, m_pipeMsgClient(repo)
{
	char path[_MAX_PATH];
	GetCurrentDirectory(1024, path);

	m_statsToolPath = path;
	m_statsToolPath += "/Tools/StatTools/StatsTool/StatsTool2.exe";

	m_statsToolProcess = INVALID_HANDLE_VALUE;
}

CXmlDataSource::~CXmlDataSource()
{
// 	if(m_pipe.IsConnected())
// 		m_pipe.CloseConnection();
}

//////////////////////////////////////////////////////////////////////////

bool CXmlDataSource::Open()
{
	char filename[_MAX_PATH];

	struct stat desc;
	if(stat(m_statsToolPath.c_str(), &desc))
	{
		if(OpenFile(filename, sizeof(filename), "StatsTool2\0StatsTool2.exe\0", "StatsTool location"))
			m_statsToolPath = filename;
		else 
			return false;
	}

	if(OpenFile(filename, sizeof(filename), "XML files (*.xml)\0*.xml\0", "Choose telemetry file to open"))
	{
		if(!m_pipe.IsConnected())
			Connect();

		stack_string args = m_statsToolPath.c_str();
		args += " --open \"";
		args += filename;
		args += "\" --sandbox_connect";

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));

		CreateProcessA(m_statsToolPath.c_str(),
			(char*)args.c_str(),
			0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi);

		m_statsToolProcess = pi.hProcess;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlDataSource::Update()
{
	if(m_pipeMsgClient.DeliverMessages())
	{
		m_pipe.CloseConnection();

		if(m_statsToolProcess != INVALID_HANDLE_VALUE)
		{
			TerminateProcess(m_statsToolProcess, 0);
		}
	}

	return m_pipe.IsFinished();
}

//////////////////////////////////////////////////////////////////////////
void CXmlDataSource::Close()
{
}

//////////////////////////////////////////////////////////////////////////
bool CXmlDataSource::OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title)
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


//////////////////////////////////////////////////////////////////////////

void CXmlDataSource::Connect()
{
	if(!m_pipe.IsConnected())
	{
		char version[256];
		gEnv->pSystem->GetProductVersion().ToString(version);
		stack_string pipe_name("\\\\.\\pipe\\CryStatsToolPipe_");
		pipe_name += version;

		m_pipe.OpenConnection(pipe_name.c_str(), &m_pipeMsgClient, PIPE_BUFFER_SIZE);
	}
	else
	{
		m_pipe.CloseConnection();
	}
}

}