#ifndef __CRYSIMPLESERVER__
#define __CRYSIMPLESERVER__

#include <string>

extern bool g_Success;

class	SEnviropment
{
public:
	std::string		m_Root;
	std::string		m_Compiler;
	std::string		m_Cache;
	std::string		m_Temp;
	std::string		m_Error;

	std::string   m_FailEMail;
	std::string   m_MailServer;
	uint32_t      m_port;
	uint32_t      m_MailInterval; // Time to send mail in minutes

	bool					m_Caching;

	std::string		m_FallbackServer;
	long					m_FallbackTreshold;

	static SEnviropment&	Instance();
};

class CCrySimpleServer
{
	void			Init();
public:
						CCrySimpleServer(const char* pShaderModel,const char* pDst,const char* pSrc,const char* pEntryFunction);
						CCrySimpleServer();
};

#endif
