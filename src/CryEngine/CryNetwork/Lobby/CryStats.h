#ifndef __CRYSTATS_H__
#define __CRYSTATS_H__

#pragma once

#include "ICryStats.h"
#include "Lobby/CryLobby.h"

#define MAX_STATS_PARAMS	5
#define MAX_STATS_TASKS		5

typedef uint32 CryStatsTaskID;
const CryStatsTaskID CryStatsInvalidTaskID = 0xffffffff;

class CCryStats : public CMultiThreadRefCount, public ICryStats
{
public:
																	CCryStats(CCryLobby* pLobby, CCryLobbyService* pService);

					ECryLobbyError					Initialise();
					ECryLobbyError					Terminate();

	virtual bool										IsDead() const	{ return false; }

protected:

	struct  STask
	{
		CryLobbyTaskID								lTaskID;
		ECryLobbyError								error;
		uint32												startedTask;
		uint32												subTask;
		CrySessionHandle							session;
		void*													pCb;
		void*													pCbArg;
		TMemHdl												paramsMem[MAX_STATS_PARAMS];
		uint32												paramsNum[MAX_STATS_PARAMS];
		bool													used;
		bool													running;
		bool													canceled;
	};

	ECryLobbyError									StartTask(uint32 eTask, bool startRunning, CryStatsTaskID* pSTaskID, CryLobbyTaskID* pLTaskID, CrySessionHandle h, void* pCb, void* pCbArg);
	void														FreeTask(CryStatsTaskID sTaskID);
	void														CancelTask(CryLobbyTaskID lTaskID);
	ECryLobbyError									CreateTaskParamMem(CryStatsTaskID sTaskID, uint32 param, const void* pParamData, size_t paramDataSize);
	void														UpdateTaskError(CryStatsTaskID sTaskID, ECryLobbyError error);

	STask*													m_pTask[MAX_STATS_TASKS];
	CCryLobby*											m_pLobby;
	CCryLobbyService*								m_pService;
};

#endif // __CRYSTATS_H__
