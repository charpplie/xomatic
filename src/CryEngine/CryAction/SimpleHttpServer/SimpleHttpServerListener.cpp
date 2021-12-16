#include "StdAfx.h"
#include "ISimpleHttpServer.h"
#include "SimpleHttpServerListener.h"

//#include <sstream>

CSimpleHttpServerListener CSimpleHttpServerListener::s_singleton;

ISimpleHttpServer* CSimpleHttpServerListener::s_http_server = NULL;

CSimpleHttpServerListener& CSimpleHttpServerListener::GetSingleton(ISimpleHttpServer* http_server)
{
	s_http_server = http_server;

	return s_singleton;
}

CSimpleHttpServerListener& CSimpleHttpServerListener::GetSingleton()
{
	return s_singleton;
}

CSimpleHttpServerListener::CSimpleHttpServerListener()
{
}

CSimpleHttpServerListener::~CSimpleHttpServerListener()
{

}

static void EscapeString(string& inout)
{
	static const char		chars[]	= {'&',			'<',			'>',			'\"',			'\'',			'%'		};
	static const char*	repls[]	= {"&amp;",	"&lt;",		"&gt;",		"&quot;",	"&apos;",	"&#37;"	};

	for (size_t i = 0; i < sizeof(chars)/sizeof(char); ++i)
	{
		size_t off = 0;
		while (true)
		{
			off = inout.find(chars[i], off);
			if (off == string::npos)
				break;
			inout.replace(off, sizeof(char), repls[i]);
			off += strlen(repls[i]);
		}
	}
}

void CSimpleHttpServerListener::Update()
{
	for (size_t i = 0; i < m_commands.size(); ++i)
	{
		gEnv->pConsole->AddOutputPrintSink(this);
		gEnv->pConsole->ExecuteString(m_commands[i].c_str());
		gEnv->pConsole->RemoveOutputPrintSink(this);

		EscapeString(m_output);

		s_http_server->SendResponse(ISimpleHttpServer::eSC_Okay, ISimpleHttpServer::eCT_XML,
			string().Format("<?xml version=\"1.0\"?><methodResponse><params><param><value><string>%s</string></value></param></params></methodResponse>", m_output.c_str()), false);
		m_output.resize(0);
	}

	m_commands.resize(0);
}

void CSimpleHttpServerListener::Print(const char* inszText)
{
	m_output += string().Format("%s\n", inszText);
}

void CSimpleHttpServerListener::OnStartResult(bool started, EResultDesc desc)
{
	if (started)
		gEnv->pLog->LogToConsole("HTTP: server successfully started");
	else
	{
		string sdesc;
		switch (desc)
		{
		case eRD_Failed:
			sdesc = "failed starting server";
			break;

		case eRD_AlreadyStarted:
			sdesc = "server already started";
			break;
		}
		gEnv->pLog->LogToConsole("HTTP: %s", sdesc.c_str());

		gEnv->pConsole->ExecuteString("http_stopserver");
	}
}

//void CSimpleHttpServerListener::OnClientAuthorized(string clientAddr)
//{
//	gEnv->pLog->LogToConsole("HTTP: client from %s is authorized", clientAddr.c_str());
//}
//
//void CSimpleHttpServerListener::OnAuthorizedClientLeft(string clientAddr)
//{
//	gEnv->pLog->LogToConsole("HTTP: client from %s is gone", clientAddr.c_str());
//}

void CSimpleHttpServerListener::OnGetRequest(string url, string client)
{
	gEnv->pLog->LogToConsole("HTTP: client from %s is making a GET request: %s", client.c_str(), url.c_str());

#define HTTP_ROOT "/Libs/CryHttp"
#define HTTP_FILE "/index.mhtml"

	if (url != HTTP_FILE)
	{
		s_http_server->SendResponse(ISimpleHttpServer::eSC_BadRequest, ISimpleHttpServer::eCT_HTML, "<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD></HTML>", true);
		return;
	}

	string page;
	FILE* file = fopen(PathUtil::GetGameFolder() + HTTP_ROOT + HTTP_FILE, "rb");
	if (file)
	{
		if ( 0 == fseek(file, 0, SEEK_END) )
		{
			long size = ftell(file);
			if (size > 0)
			{
				page.resize(size, '0');
				int nres = fseek(file, 0, SEEK_SET); CRY_ASSERT(nres==0);
				fread((char*)page.data(), 1, page.size(), file);
			}
		}
		fclose(file);
	}

	if ( page.empty() )
	{
		s_http_server->SendResponse(ISimpleHttpServer::eSC_NotFound, ISimpleHttpServer::eCT_HTML, "<HTML><HEAD><TITLE>Not Found</TITLE></HEAD></HTML>", true);
		return;
	}

	size_t index = page.find("$time$");
	//std::stringstream ss; ss << gEnv->pTimer->GetAsyncCurTime();
	char timestamp[33]; _snprintf(timestamp, sizeof(timestamp)-1, "%f", gEnv->pTimer->GetAsyncCurTime()); timestamp[sizeof(timestamp)-1] = 0;
	page.replace(index, strlen("$time$"), timestamp);
	s_http_server->SendWebpage(page);
}

void CSimpleHttpServerListener::OnRpcRequest(string xml, string client)
{
	gEnv->pLog->LogToConsole("HTTP: client from %s is making an RPC request: %s", client.c_str(), xml.c_str());

	string command;
	XmlNodeRef root = gEnv->pSystem->LoadXmlFromString( xml.c_str() );
	if ( root && root->isTag("methodCall") )
	{
		XmlNodeRef methodName = root->getChild(0);
		if ( methodName && methodName->isTag("methodName") )
		{
			command = methodName->getContent(); // command name must go first
			XmlNodeRef params = root->getChild(1);
			if (params)
			{
				if ( params->isTag("params") )
				{
					int nparams = params->getChildCount();
					for (int i = 0; i < nparams; ++i)
					{
						XmlNodeRef param = params->getChild(i);
						XmlNodeRef value = param->getChild(0);
						if ( value && value->isTag("value") )
						{
							// we treat all types as strings
							string svalue = " ";
							if ( value->getChildCount() > 0 )
							{
								XmlNodeRef type = value->getChild(0);
								if (type)
									svalue += type->getContent();
							}
							else
								svalue += value->getContent();
							command += svalue;
						}
						else
							goto L_error;
					}
				}
				else
					goto L_error;
			}
			// a command without params is allowed

			// execute the command on the server
			gEnv->pLog->LogToConsole("HTTP: received xmlrpc command: %s", command.c_str());
			m_commands.push_back(command);

			return;
		}
	}

L_error:
	s_http_server->SendResponse(ISimpleHttpServer::eSC_BadRequest, ISimpleHttpServer::eCT_HTML, "<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD></HTML>", true);
}

