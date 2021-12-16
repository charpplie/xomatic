#include "StdAfx.h"
#include "LogFile.h"

LogFile::LogFile(const std::string& filePath)
: m_file(0)
{
	m_file = std::fopen(filePath.c_str(), "w");
}

LogFile::~LogFile()
{
	if (m_file)
		fclose(m_file);
}

bool LogFile::IsOpen() const
{
	return m_file != 0;
}

void LogFile::Log(MessageSeverity severity, const char* message)
{
	const char* severityMessage = 0;
	switch (severity)
	{
	case MessageSeverity_Debug:   severityMessage = "   ";  break;
	case MessageSeverity_Info:    severityMessage = "   ";  break;
	case MessageSeverity_Warning: severityMessage = "W: ";  break;
	case MessageSeverity_Error:   severityMessage = "E: ";  break;
	default:                      severityMessage = "?: ";  break;
	}

	if (m_file)
	{
		fprintf(m_file, "%s%s\n", severityMessage, message);
		fflush(m_file);
	}
}
