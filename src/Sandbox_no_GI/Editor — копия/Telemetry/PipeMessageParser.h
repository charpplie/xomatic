////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   PipeMessageParser.h
//  Version:     v1.00
//  Created:     24/12/2009 by Sergey Mikhtonyuk
//  Description: Parses messages comming from the pipe
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef		_PIPEMESSAGEPARSER_H__
#define		_PIPEMESSAGEPARSER_H__

# pragma once

#include "PipeClient.h"
#include "TelemetryRepository.h"
#include "TelemetryObjects.h"

namespace Telemetry
{

class CPipeMessageParser : public IPipeClientListener
{
	enum EOpCode
	{
		eOC_Begin = 0,
		eOC_End = 1,
		eOC_Enter = 2,
		eOC_Leave = 3,
		eOC_Timeline = 4,
		eOC_Event = 5,

		eOC_Unknown = 255,
	};

public:

	CPipeMessageParser(CTelemetryRepository &repo);

	virtual void OnMessage(const string& message);

	bool DeliverMessages();

private:
	void OnBegin();
	void OnEnd();
	void OnEnter(const char* name);
	void OnLeave();
	void OnTimeline(const char* name);
	void OnEvent(const char* params);

	void ProcessTimelines(size_t startFrom);
	size_t FindPositionTimeline(size_t startFrom);

	void ClearDoneBuffer();

	bool IsPathTimeline(const char* name);

private:
	TTimelines m_currentBuffer;
	TTimelines m_doneBuffer;

	CTelemetryRepository& m_repository;
	CryMutex m_lock;
	size_t m_depth;
	size_t m_endProcessed;
	TTelemNodePtr m_currentOwner;
	bool m_currentPath;
};

}

#endif // __PIPEMESSAGEPARSER_H__
