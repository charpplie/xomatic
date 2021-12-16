////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   PipeClient.h
//  Version:     v1.00
//  Created:     24/12/2009 by Sergey Mikhtonyuk
//  Description: Named pipe client for data stream from StatsTool
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_PIPECLIENT_H__
#define		_PIPECLIENT_H__

# pragma once

#include "CryThread.h"

//////////////////////////////////////////////////////////////////////////

struct IPipeClientListener
{
	virtual void OnMessage(const string& msg) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CPipeClient : public CryThread<CPipeClient>
{
public:
	static const uint32 DEFAULT_BUFFER_SIZE = 10*1024;

	CPipeClient();
	~CPipeClient();

	void OpenConnection(const char* pipeName, IPipeClientListener* listener, uint32 bufferSize = DEFAULT_BUFFER_SIZE);
	void CloseConnection();
	bool isConnected();

	virtual void Run();

private:
	bool TryOpenConnection();
	void ReceiveMessage();

private:
	char* m_buffer;
	uint32 m_bufSize;

	// HACK: IsStarted() of CryThread has data races
	bool m_isRunning;
	string m_pipeName;
	HANDLE m_pipe;
	string m_message;
	IPipeClientListener* m_listener;
};

#endif // __PIPECLIENT_H__
