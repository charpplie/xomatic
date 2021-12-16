#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#include "ILogFile.h"
#include <cstdio>
#include <string>

class LogFile : public ILogFile
{
public:
	LogFile(const std::string& filePath);
	~LogFile();

	bool IsOpen() const;

	// ILogFile
	virtual void Log(MessageSeverity severity, const char* message);

private:
	std::FILE* m_file;
};

#endif //__LOGFILE_H__
