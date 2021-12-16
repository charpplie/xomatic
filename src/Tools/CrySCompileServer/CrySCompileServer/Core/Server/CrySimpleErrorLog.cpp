#include "../Common.h"
#include "../StdTypes.hpp"
#include "../Error.hpp"
#include "../STLHelper.hpp"
#include "../Mailer.h"
#include "../tinyxml/tinyxml.h"
#include "CrySimpleErrorLog.hpp"
#include "CrySimpleServer.hpp"
#include <string>
#include <algorithm>

#ifdef _MSC_VER
#include <process.h>
#include <direct.h>
#endif
#ifdef UNIX
#include <pthread.h>
#endif

static unsigned int volatile g_bSendingMail = false;
static unsigned int volatile g_nMailNum = 0;

CCrySimpleErrorLog& CCrySimpleErrorLog::Instance()
{
	static CCrySimpleErrorLog g_Cache;
	return g_Cache;
}

CCrySimpleErrorLog::CCrySimpleErrorLog()
{
	m_lastMailTime = 0;
}

void CCrySimpleErrorLog::Init()
{
	
}

void CCrySimpleErrorLog::Add(const std::vector<uint8_t>& rData,const std::string &errorDescription)
{
	TiXmlDocument ReqParsed( "Request.xml" );
	if (rData.size() > 0)
	{
		ReqParsed.Parse( (const char*)(&rData[0]) );
	}

	CCrySimpleMutexAutoLock Lock(m_LogMutex);

	if (m_Log.size() > 100)
		return;

	SCompileError err;
	err.request = rData;
	err.sError = errorDescription;

	const TiXmlElement* pElement = ReqParsed.FirstChildElement();
	if (pElement)
	{
		const char *pProgram = pElement->Attribute( "Program" );
		if (pProgram)
		{
			size_t len = strlen(pProgram);
			err.request.resize(len);
			for (size_t i = 0; i < len; i++)
				err.request[i] = pProgram[i];
		}
	}

	m_Log.push_back( err );
}

void CCrySimpleErrorLog::SendMail()
{
	std::vector<std::string> Attachment;
	std::vector<std::string> Rcpt;

	std::string mailBody;

	tdFailed tempLog;
	{
		CCrySimpleMutexAutoLock Lock(m_LogMutex);
		m_Log.swap(tempLog);
	}
#if defined(_MSC_VER)
	{
		char compName[256];
		DWORD size = ARRAYSIZE(compName);

		typedef BOOL (WINAPI *FP_GetComputerNameExA)(COMPUTER_NAME_FORMAT, LPSTR, LPDWORD);
		FP_GetComputerNameExA pGetComputerNameExA = (FP_GetComputerNameExA) GetProcAddress(LoadLibrary("kernel32.dll"), "GetComputerNameExA");

		if (pGetComputerNameExA)
			pGetComputerNameExA(ComputerNamePhysicalDnsFullyQualified, compName, &size);
		else
			GetComputerName(compName, &size);

		mailBody += std::string("Report sent from ") + compName + "...\n\n";
	}
#endif
	{
		int a = 0;
		//Attachment.resize(m_Log.size());
		for (tdFailed::const_iterator it = tempLog.begin(); it != tempLog.end(); ++it)
		{
			const SCompileError &err = *it;

			char Text[1024];
			sprintf(Text,"%d-thread%d.txt",a+1, GetCurrentThreadId());
			std::string sErrorFile =	SEnviropment::Instance().m_Error+Text;
			std::replace( sErrorFile.begin( ),sErrorFile.end( ), '/','\\' );
			CSTLHelper::ToFile( sErrorFile,err.request);

			Attachment.push_back( sErrorFile );
			a++;

			if (a)
				mailBody += std::string("\n\n");
			mailBody += std::string("Failed to compile...\n") + "Attachment:          " + Text + "\n";
			mailBody += err.sError;
		}
	}

	//Attachment.resize(0);

	Rcpt.resize(1);
	Rcpt[0]	=	SEnviropment::Instance().m_FailEMail;

	{
		CSMTPMailer::tstrcol cc;
		CSMTPMailer::tstrcol bcc;
		CSMTPMailer::tstrcol attachments;

		CSMTPMailer mail("", "", SEnviropment::Instance().m_MailServer);
		bool res = mail.Send("ShaderCompiler@crytek.de", Rcpt, cc, bcc, "Remote shader compiler error log", mailBody, Attachment);
	}

	g_bSendingMail = false;
}

//////////////////////////////////////////////////////////////////////////
void CCrySimpleErrorLog::Tick()
{
	DWORD t = GetTickCount();
	if (t < m_lastMailTime || (t - m_lastMailTime) > SEnviropment::Instance().m_MailInterval*1000*60)
	{
		m_lastMailTime = t;
		if (m_Log.size() > 0 && !g_bSendingMail)
		{
			g_bSendingMail = true;
			g_nMailNum++;
			printf( "Sending Errors Mail %d\n",g_nMailNum );
			CCrySimpleErrorLog::Instance().SendMail();
		}
	}
}
