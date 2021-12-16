// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "ResourceCompilerHelper.h"
#include "platform.h"

// The RC output that should be logged by the editor
#define LOG_RC_FORMAT "EditorFbxImport: %s"

// RC listener that parses output information into QStrings
// Note: This is not suitable for huge outputs, since all lines are kept in memory
class CRcListener : public IResourceCompilerListener
{
public:
	// Construct RC listener with specified minimum severity level to use when logging
	CRcListener(MessageSeverity minimumLevel) : m_minimumLevel(minimumLevel), m_pLog(gEnv->pSystem->GetILog())
	{
		assert(m_pLog);
	}

private:
	// Handle message from RC
	void OnRCMessage(MessageSeverity severity, const char* text) override
	{
		if (severity >= m_minimumLevel)
		{
			switch(severity)
			{
			case MessageSeverity_Warning:
				m_pLog->LogWarning(LOG_RC_FORMAT, text);
				break;
			case MessageSeverity_Info:
			case MessageSeverity_Debug:
				m_pLog->LogAlways(LOG_RC_FORMAT, text);
				break;
			case MessageSeverity_Error:
			default:
				m_pLog->LogError(LOG_RC_FORMAT, text);
				break;
			}
		}
	}

	// No copy/assign
	CRcListener(const CRcListener &);
	void operator =(const CRcListener &);

	const MessageSeverity m_minimumLevel;
	ILog * const m_pLog;
};