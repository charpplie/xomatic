////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   XmlDataSource.h
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Loads data from XML files using StatsTool - temp
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __XMLDATASOURCE_H__
#define __XMLDATASOURCE_H__

#include "PipeClient.h"
#include "PipeMessageParser.h"

#include "IDataSource.h"

namespace Telemetry
{

class CTelemetryRepository;

class CXmlDataSource : public IDataSource
{
public:
	CXmlDataSource(CTelemetryRepository& rep);
	virtual ~CXmlDataSource();

	virtual bool Open();
	virtual bool Update();
	virtual void Close();

private:
	bool OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title);
	void Connect();

	// telemetry pipe (temp until DB is ready)
	string m_statsToolPath;
	CTelemetryRepository& m_repository;
	CPipeClient m_pipe;
	CPipeMessageParser m_pipeMsgClient;
	static const uint32 PIPE_BUFFER_SIZE = 100*1024;

	HANDLE m_statsToolProcess;
};

}

#endif //__XMLDATASOURCE_H__