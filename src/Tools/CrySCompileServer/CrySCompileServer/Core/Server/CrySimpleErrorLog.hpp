#ifndef __CRYSIMPLEERRORLOG__
#define __CRYSIMPLEERRORLOG__

#include <list>
#include "CrySimpleMutex.hpp"

struct SCompileError
{
	std::string sError;
	std::string sErrorFile;
	std::string sShaderProgram;
	std::vector<uint8_t> request;
};
typedef std::list<SCompileError> tdFailed;

class CCrySimpleErrorLog
{
	tdFailed										m_Log;
	CCrySimpleMutex							m_LogMutex;
	unsigned int                m_lastMailTime;
public:

	CCrySimpleErrorLog();
	void												Init();
	void												Add(const std::vector<uint8_t>& rData,const std::string &errorDescription);
	void												SendMail();
	void                        Tick();

	static CCrySimpleErrorLog&	Instance();
};

#endif
