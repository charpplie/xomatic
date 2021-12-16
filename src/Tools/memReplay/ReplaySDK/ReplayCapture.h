#pragma once

#include "ReplayLogReader.h"

class CWinThread;

class ReplayCapture
{
public:
	ReplayCapture();

	bool BeginCapture(const char* filename, IReplayListener& listeners);
	void EndCapture();

private:
	static UINT __cdecl ThreadProxy(LPVOID param);

private:
	UINT ThreadMain();

private:
	std::string m_filename;
	IReplayListener* m_listener;

	CWinThread* m_thread;

	SOCKET m_clientSock;
};
