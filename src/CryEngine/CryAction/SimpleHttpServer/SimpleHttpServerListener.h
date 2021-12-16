#ifndef __SIMPLEHTTPSERVERLISTENER_H__
#define __SIMPLEHTTPSERVERLISTENER_H__

#pragma once

class CSimpleHttpServerListener : public IHttpServerListener, public IOutputPrintSink
{
public:
	static CSimpleHttpServerListener& GetSingleton(ISimpleHttpServer* http_server);
	static CSimpleHttpServerListener& GetSingleton();

	void Update();

private:
	CSimpleHttpServerListener();
	~CSimpleHttpServerListener();

	void OnStartResult(bool started, EResultDesc desc);

	//void OnClientAuthorized(string clientAddr);
	//void OnAuthorizedClientLeft(string clientAddr);

	void OnGetRequest(string url, string client);
	void OnRpcRequest(string xml, string client);

	string m_output;
	void Print(const char* inszText);

	typedef std::vector<string> TCommandsVec;
	TCommandsVec m_commands;

	static CSimpleHttpServerListener s_singleton;
	static ISimpleHttpServer* s_http_server;
};

#endif

