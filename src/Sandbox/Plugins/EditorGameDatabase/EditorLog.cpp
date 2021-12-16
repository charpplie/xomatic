//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "EditorLog.h"

EditorLog::TDelegate EditorLog::s_delegate;

EditorLog::EditorLog( const TDelegate& _delegate ) 
{
	CRY_ASSERT(_delegate);
	CRY_ASSERT(!s_delegate);

	s_delegate = _delegate;
}

EditorLog::~EditorLog()
{
	CRY_ASSERT(s_delegate);

	s_delegate = NULL;
}

void EditorLog::Log( const char* format, ... )
{
	if (!s_delegate)
		return;

	va_list args;
	va_start(args,format);
	{
		char logBuffer[1024];
		const size_t sizeLogBuffer = sizeof(logBuffer);

		char* pMessage = logBuffer;

		int written = _vsnprintf(pMessage, sizeLogBuffer, format, args);
		if (written < 0 || written >= sizeLogBuffer)
		{
			pMessage[sizeLogBuffer-1] = '\0';
		}
		
		s_delegate(pMessage);
	}
	va_end(args);
}
