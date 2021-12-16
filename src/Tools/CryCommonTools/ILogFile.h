#ifndef __ILOGFILE_H__
#define __ILOGFILE_H__

#include <cstdarg>
#include <cstdio>

class ILogFile
{
public:
	enum MessageSeverity
	{
		MessageSeverity_Debug,
		MessageSeverity_Info,
		MessageSeverity_Warning,
		MessageSeverity_Error
	};

	virtual void Log(MessageSeverity severity, const char* message) = 0;
};

inline void Log(ILogFile* context, ILogFile::MessageSeverity severity, const char* messageFmt, ...)
{
	char buffer[2048];
	{
		va_list args;
		va_start(args, messageFmt);
		vsprintf(buffer, messageFmt, args);
		va_end(args);
	}

	context->Log(severity, buffer);
}

#endif //__ILOGFILE_H__
