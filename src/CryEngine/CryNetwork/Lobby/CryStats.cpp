#include "StdAfx.h"
#include "CryStats.h"

CCryStats::CCryStats(CCryLobby* pLobby, CCryLobbyService* pService)
{
	CRY_ASSERT_MESSAGE(pLobby, "CCryStats::CCryStats: Lobby not specified");
	CRY_ASSERT_MESSAGE(pService, "CCryStats::CCryStats: Service not specified");

	m_pLobby = pLobby;
	m_pService = pService;

	for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
	{
		m_pTask[i] = NULL;
	}
}

ECryLobbyError CCryStats::Initialise()
{
	for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
	{
		CRY_ASSERT_MESSAGE(m_pTask[i], "CCryStats: Task base pointers not setup");
		m_pTask[i]->used = false;
	}

	return eCLE_Success;
}

ECryLobbyError CCryStats::Terminate()
{
	for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
	{
		CRY_ASSERT_MESSAGE(m_pTask[i], "CCryStats: Task base pointers not setup");
		
		if (m_pTask[i]->used)
		{
			FreeTask(i);
		}
	}

	return eCLE_Success;
}

ECryLobbyError CCryStats::StartTask(uint32 eTask, bool startRunning, CryStatsTaskID* pSTaskID, CryLobbyTaskID* pLTaskID, CrySessionHandle h, void* pCb, void* pCbArg)
{
	CryLobbyTaskID lobbyTaskID = m_pLobby->CreateTask();

	if (lobbyTaskID != CryLobbyInvalidTaskID)
	{
		for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
		{
			STask* pTask = m_pTask[i];

			CRY_ASSERT_MESSAGE(pTask, "CCryStats: Task base pointers not setup");

			if (!pTask->used)
			{
				pTask->lTaskID = lobbyTaskID;
				pTask->error = eCLE_Success;
				pTask->startedTask = eTask;
				pTask->subTask = eTask;
				pTask->session = h;
				pTask->pCb = pCb;
				pTask->pCbArg = pCbArg;
				pTask->used = true;
				pTask->running = startRunning;
				pTask->canceled = false;

				for (uint32 j = 0; j < MAX_STATS_PARAMS; j++)
				{
					pTask->paramsMem[j] = TMemInvalidHdl;
					pTask->paramsNum[j] = 0;
				}

				if (pSTaskID)
				{
					*pSTaskID = i;
				}

				if (pLTaskID)
				{
					*pLTaskID = lobbyTaskID;
				}

				return eCLE_Success;
			}
		}
	}

	return eCLE_TooManyTasks;
}

void CCryStats::FreeTask(CryStatsTaskID sTaskID)
{
	STask* pTask = m_pTask[sTaskID];

	CRY_ASSERT_MESSAGE(pTask, "CCryStats: Task base pointers not setup");

	for (uint32 i = 0; i < MAX_STATS_PARAMS; i++)
	{
		if (pTask->paramsMem[i] != TMemInvalidHdl)
		{
			m_pLobby->MemFree(pTask->paramsMem[i]);
			pTask->paramsMem[i] = TMemInvalidHdl;
			pTask->paramsNum[i] = 0;
		}
	}

	m_pLobby->ReleaseTask(pTask->lTaskID);

	pTask->used = false;
}

void CCryStats::CancelTask(CryLobbyTaskID lTaskID)
{
	LOBBY_AUTO_LOCK;

	CryLogAlways("[Lobby]Try cancel task %d", lTaskID);

	if (lTaskID != CryLobbyInvalidTaskID)
	{
		for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
		{
			STask* pTask = m_pTask[i];

			CRY_ASSERT_MESSAGE(pTask, "CCryStats: Task base pointers not setup");

			if (pTask->used && (pTask->lTaskID == lTaskID))
			{
				CryLogAlways("[Lobby] Task %d canceled", lTaskID);
				pTask->pCb = NULL;
				pTask->canceled = true;

				break;
			}
		}
	}
}

ECryLobbyError CCryStats::CreateTaskParamMem(CryStatsTaskID sTaskID, uint32 param, const void* pParamData, size_t paramDataSize)
{
	STask* pTask = m_pTask[sTaskID];

	CRY_ASSERT_MESSAGE(pTask, "CCryStats: Task base pointers not setup");

	pTask->paramsMem[param] = m_pLobby->MemAlloc(paramDataSize);
	void* p = m_pLobby->MemGetPtr(pTask->paramsMem[param]);

	if (p)
	{
		if (pParamData)
		{
			memcpy(p, pParamData, paramDataSize);
		}

		return eCLE_Success;
	}

	return eCLE_OutOfMemory;
}

void CCryStats::UpdateTaskError(CryStatsTaskID sTaskID, ECryLobbyError error)
{
	STask* pTask = m_pTask[sTaskID];

	CRY_ASSERT_MESSAGE(pTask, "CCryStats: Task base pointers not setup");

	if (pTask->error == eCLE_Success)
	{
		pTask->error = error;
	}
}

