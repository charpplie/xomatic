#include "StdAfx.h"
#include "CryLiveLobby.h"
#include "CryLiveStats.h"

CCryLiveStats::CCryLiveStats(CCryLobby* pLobby, CCryLobbyService* pService) : CCryStats(pLobby, pService)
{
	// Make the CCryStats base pointers point to our data so we can use the common code in CCryStats
	for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
	{
		CCryStats::m_pTask[i] = &m_task[i];
	}

	m_numLeaderBoards = 0;
	m_numUserData = 0;

	m_profileSettings.numSettings = 0;
	m_profileSettings.settings[0].id = XPROFILE_TITLE_SPECIFIC1;
	m_profileSettings.settings[1].id = XPROFILE_TITLE_SPECIFIC2;
	m_profileSettings.settings[2].id = XPROFILE_TITLE_SPECIFIC3;
}

void CCryLiveStats::Tick(CTimeValue tv)
{
	if (m_pLobby->MutexTryLock())
	{
		for (uint32 i = 0; i < MAX_STATS_TASKS; i++)
		{
			STask* pTask = &m_task[i];

			if (pTask->used && pTask->running)
			{
				switch (pTask->subTask)
				{
				case eT_StatsWriteLeaderBoards:
					TickStatsWriteLeaderBoards(i);
					break;

				case eT_StatsReadLeaderBoardByRankForRange:
				case eT_StatsReadLeaderBoardByRankForUser:
				case eT_StatsReadLeaderBoardByUserID:
					TickStatsReadLeaderBoard(i);
					break;

				case eT_StatsWriteUserData:
					TickStatsWriteUserData(i);
					break;

				case eT_StatsReadUserData:
					TickStatsReadUserData(i);
					break;
				}
			}
		}

		m_pLobby->MutexUnlock();
	}
}

ECryLobbyError CCryLiveStats::StartTask(ETask etask, bool startRunning, uint32 user, CryStatsTaskID* pSTaskID, CryLobbyTaskID* pLTaskID, CrySessionHandle h, void* pCb, void* pCbArg)
{
	CryStatsTaskID tmpSTaskID;
	CryStatsTaskID* pUseSTaskID = pSTaskID ? pSTaskID : &tmpSTaskID;
	ECryLobbyError error = CCryStats::StartTask(etask, startRunning, pUseSTaskID, pLTaskID, h, pCb, pCbArg);

	if (error == eCLE_Success)
	{
		STask* pTask = &m_task[*pUseSTaskID];

		ZeroMemory(&pTask->overlapped, sizeof(pTask->overlapped));
		pTask->enumerate = INVALID_HANDLE_VALUE;
		pTask->user = user;
	}

	return error;
}

void CCryLiveStats::StartSubTask(ETask etask, CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	ZeroMemory(&pTask->overlapped, sizeof(pTask->overlapped));
	pTask->subTask = etask;
}

void CCryLiveStats::StartTaskRunning(CryStatsTaskID sTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* pTask = &m_task[sTaskID];

	if (pTask->used)
	{
		pTask->running = true;

		switch (pTask->startedTask)
		{
		case eT_StatsRegisterLeaderBoards:
		case eT_StatsRegisterUserData:
			StopTaskRunning(sTaskID);
			break;

		case eT_StatsWriteLeaderBoards:
			StartStatsWriteLeaderBoards(sTaskID);
			break;

		case eT_StatsReadLeaderBoardByRankForRange:
			StartStatsReadLeaderBoardByRankForRange(sTaskID);
			break;

		case eT_StatsReadLeaderBoardByRankForUser:
			StartStatsReadLeaderBoardByRankForUser(sTaskID);
			break;

		case eT_StatsReadLeaderBoardByUserID:
			StartStatsReadLeaderBoardByUserID(sTaskID);
			break;

		case eT_StatsWriteUserData:
			StartStatsWriteUserData(sTaskID);
			break;

		case eT_StatsReadUserData:
			StartStatsReadUserData(sTaskID);
			break;
		}
	}
}

void CCryLiveStats::EndTask(CryStatsTaskID sTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* pTask = &m_task[sTaskID];

	if (pTask->used)
	{
		if (pTask->pCb)
		{
			switch (pTask->startedTask)
			{
			case eT_StatsRegisterLeaderBoards:
			case eT_StatsWriteLeaderBoards:
			case eT_StatsRegisterUserData:
			case eT_StatsWriteUserData:
				((CryStatsCallback)pTask->pCb)(pTask->lTaskID, pTask->error, pTask->pCbArg);
				break;

			case eT_StatsReadLeaderBoardByRankForRange:
			case eT_StatsReadLeaderBoardByRankForUser:
			case eT_StatsReadLeaderBoardByUserID:
				EndStatsReadLeaderBoard(sTaskID);
				break;

			case eT_StatsReadUserData:
				EndStatsReadUserData(sTaskID);
				break;
			}
		}

		CryLogAlways("[Lobby] Stats EndTask %d (%d) Result %d", pTask->startedTask, pTask->subTask, pTask->error);

		FreeTask(sTaskID);
	}
}

void CCryLiveStats::StopTaskRunning(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	if (pTask->used)
	{
		pTask->running = false;

		if (pTask->enumerate != INVALID_HANDLE_VALUE)
		{
			CloseHandle(pTask->enumerate);
			pTask->enumerate = INVALID_HANDLE_VALUE;
		}

		TO_GAME(&CCryLiveStats::EndTask, this, sTaskID);
	}
}

ECryLobbyError CCryLiveStats::TickTask(CryStatsTaskID sTaskID)
{
	if (XHasOverlappedIoCompleted(&m_task[sTaskID].overlapped))
	{
		DWORD ret = XGetOverlappedExtendedError(&m_task[sTaskID].overlapped);

		switch (ret)
		{
		case ERROR_IO_INCOMPLETE:
			return eCLE_Pending;

		default:
			return CryLiveLobbyGetErrorFromLive(ret); 
		}
	}

	return eCLE_Pending;
}

ECryLobbyError CCryLiveStats::StatsRegisterLeaderBoards(SCryStatsLeaderBoardWrite* pBoards, uint32 numBoards, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	CryStatsTaskID sTaskID;

	error = StartTask(eT_StatsRegisterLeaderBoards, false, 0, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

	if (error == eCLE_Success)
	{
		if ((m_numLeaderBoards + numBoards) > m_leaderBoards.size())
		{
			m_leaderBoards.resize(m_leaderBoards.size() + numBoards);
		}

		for (uint32 i = 0; i < numBoards; i++)
		{
			SRegisterLeaderBoardData* pRegisterData = &m_leaderBoards[m_numLeaderBoards];
			SCryStatsLeaderBoardWrite* pBoardData = &pBoards[i];

			pRegisterData->boardID = pBoardData->id;
			pRegisterData->scoreID = pBoardData->data.score.id;
			pRegisterData->numColumns = pBoardData->data.numColumns;

			if (pRegisterData->numColumns > 0)
			{
				pRegisterData->pColumns = new SRegisterLeaderBoardData::SColumn[pRegisterData->numColumns];

				for (uint32 j = 0; j < pRegisterData->numColumns; j++)
				{
					SRegisterLeaderBoardData::SColumn* pRegisterColumn = &pRegisterData->pColumns[j];
					SCryStatsLeaderBoardUserColumn* pBoardColumn = &pBoardData->data.pColumns[j];

					pRegisterColumn->columnID = pBoardColumn->columnID;
					pRegisterColumn->dataID = pBoardColumn->data.m_id;
					pRegisterColumn->dataType = pBoardColumn->data.m_type;
				}
			}
			else
			{
				pRegisterData->pColumns = NULL;
			}

			m_numLeaderBoards++;
		}

		FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
	}

	CryLogAlways("[Lobby] Start StatsRegisterLeaderBoards return %d", error);

	return error;
}

ECryLobbyError CCryLiveStats::StatsWriteLeaderBoards(CrySessionHandle session, uint32 user, SCryStatsLeaderBoardWrite* pBoards, uint32 numBoards, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	CCryLiveLobbyService* pService = (CCryLiveLobbyService*)m_pService;
	XUID xuids[1];

	ECryLobbyError error = pService->GetUserXUID(user, &xuids[0]);

	if (error == eCLE_Success)
	{
		SCryStatsLeaderBoardWrite* pLeaderBoards[1];
		uint32 numLeaderBoards[1];

		pLeaderBoards[0] = pBoards;
		numLeaderBoards[0] = numBoards;

		error = StatsWriteLeaderBoards(session, xuids, pLeaderBoards, numLeaderBoards, 1, pTaskID, cb, pCbArg);
	}

	return error;
}

ECryLobbyError CCryLiveStats::StatsWriteLeaderBoards(CrySessionHandle session, CryUserID* pUserIDs, SCryStatsLeaderBoardWrite** ppBoards, uint32* pNumBoards, uint32 numUsers, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	TMemHdl memHdl = m_pLobby->MemAlloc(numUsers*sizeof(XUID));
	ECryLobbyError error = eCLE_Success;

	if (memHdl != TMemInvalidHdl)
	{
		XUID* pXUIDs = (XUID*)m_pLobby->MemGetPtr(memHdl);

		for (uint32 i = 0; i < numUsers; i++)
		{
			if (pUserIDs[i] == CryUserInvalidID)
			{
				error = eCLE_InvalidUser;
				break;
			}

			pXUIDs[i] = ((SCryLiveUserID*)pUserIDs[i].get())->xuid;
		}

		if (error == eCLE_Success)
		{
			error = StatsWriteLeaderBoards(session, pXUIDs, ppBoards, pNumBoards, numUsers, pTaskID, cb, pCbArg);
		}

		m_pLobby->MemFree(memHdl);
	}
	else
	{
		error = eCLE_OutOfMemory;
	}

	return error;
}

#define STATS_PARAM_WRITE_XUIDS						0
#define STATS_PARAM_WRITE_NUM_XUIDS				0
#define STATS_PARAM_WRITE_VIEWS						1
#define STATS_PARAM_WRITE_CURRENT_ID			1
#define STATS_PARAM_WRITE_NUM_VIEWS				2
#define STATS_PARAM_WRITE_ALL_VIEWS				3
#define STATS_PARAM_WRITE_ALL_PROPERTIES	4
ECryLobbyError CCryLiveStats::StatsWriteLeaderBoards(CrySessionHandle session, XUID* pXUIDs, SCryStatsLeaderBoardWrite** ppBoards, uint32* pNumBoards, uint32 numUsers, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg)
{
	CCryLiveMatchMaking* pMatchMaking = (CCryLiveMatchMaking*)m_pService->GetMatchMaking();

	if (pMatchMaking)
	{
		CrySessionHandle h = pMatchMaking->GetSessionHandleFromGameSessionHandle(session);
		ECryLobbyError error = eCLE_Success;

		if (h != CrySessionInvalidHandle)
		{
			CryStatsTaskID sTaskID;

			error = StartTask(eT_StatsWriteLeaderBoards, false, 0, &sTaskID, pTaskID, h, cb, pCbArg);

			if (error == eCLE_Success)
			{
				STask* pTask = &m_task[sTaskID];

				error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_XUIDS, pXUIDs, numUsers*sizeof(XUID));
				pTask->paramsNum[STATS_PARAM_WRITE_NUM_XUIDS] = numUsers;
				pTask->paramsNum[STATS_PARAM_WRITE_CURRENT_ID] = 0;

				if (error == eCLE_Success)
				{
					error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_VIEWS, NULL, numUsers*sizeof(XSESSION_VIEW_PROPERTIES*));

					if (error == eCLE_Success)
					{
						error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_NUM_VIEWS, pNumBoards, numUsers*sizeof(uint32));

						if (error == eCLE_Success)
						{
							uint32 numViews = 0;
							uint32 numProperties = 0;

							for (uint32 i = 0; i < numUsers; i++)
							{
								numViews += pNumBoards[i];

								for (uint32 j = 0; j < pNumBoards[i]; j++)
								{
									numProperties += 1 + ppBoards[i][j].data.numColumns;
								}
							}

							error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_ALL_VIEWS, NULL, numViews*sizeof(XSESSION_VIEW_PROPERTIES));

							if (error == eCLE_Success)
							{
								error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_ALL_PROPERTIES, NULL, numProperties*sizeof(XUSER_PROPERTY));

								if (error == eCLE_Success)
								{
									XSESSION_VIEW_PROPERTIES** ppViews = (XSESSION_VIEW_PROPERTIES**)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_VIEWS]);
									XSESSION_VIEW_PROPERTIES* pViews = (XSESSION_VIEW_PROPERTIES*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_ALL_VIEWS]);
									XUSER_PROPERTY* pProperties = (XUSER_PROPERTY*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_ALL_PROPERTIES]);
									uint32 viewIndex = 0;
									uint32 propertyIndex = 0;

									for (uint32 i = 0; (i < numUsers) && (error == eCLE_Success); i++)
									{
										ppViews[i] = &pViews[viewIndex];
										viewIndex += pNumBoards[i];

										for (uint32 j = 0; (j < pNumBoards[i]) && (error == eCLE_Success); j++)
										{
											XSESSION_VIEW_PROPERTIES* pView = &ppViews[i][j];
											SCryStatsLeaderBoardWrite* pBoard = &ppBoards[i][j];
											SCryLobbyUserData score;
											SLiveUserData liveData;

											pView->dwViewId = pBoard->id;
											pView->dwNumProperties = 1 + pBoard->data.numColumns;
											pView->pProperties = &pProperties[propertyIndex];

											score.m_id = pBoard->data.score.id;
											score.m_type = eCLUDT_Int64;
											score.m_int64 = pBoard->data.score.score;

											propertyIndex += 1 + pBoard->data.numColumns;
											
											error = CryUserDataToLiveUserData(&score, &liveData);

											if (error == eCLE_Success)
											{
												XUSER_PROPERTY* pProperty = &pView->pProperties[0];

												if (liveData.typeContext)
												{
													pProperty->dwPropertyId = liveData.context.dwContextId;
													pProperty->value.type = XUSER_DATA_TYPE_INT32;
													pProperty->value.nData = liveData.context.dwValue;
												}
												else
												{
													*pProperty = liveData.property;
												}

												for (uint32 k = 0; (k < pBoard->data.numColumns) && (error == eCLE_Success); k++)
												{
													error = CryUserDataToLiveUserData(&pBoard->data.pColumns[k].data, &liveData);

													if (error == eCLE_Success)
													{
														pProperty = &pView->pProperties[1 + k];

														if (liveData.typeContext)
														{
															pProperty->dwPropertyId = liveData.context.dwContextId;
															pProperty->value.type = XUSER_DATA_TYPE_INT32;
															pProperty->value.nData = liveData.context.dwValue;
														}
														else
														{
															*pProperty = liveData.property;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}

				if (error == eCLE_Success)
				{
					FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
				}
				else
				{
					FreeTask(sTaskID);
				}
			}
		}
		else
		{
			error = eCLE_InvalidSession;
		}

		CryLogAlways("[Lobby] Start StatsWriteLeaderBoards return %d", error);

		return error;
	}

	return eCLE_InternalError;
}

void CCryLiveStats::StartStatsWriteLeaderBoards(CryStatsTaskID sTaskID)
{
	CCryLiveMatchMaking* pMatchMaking = (CCryLiveMatchMaking*)m_pService->GetMatchMaking();
	STask* pTask = &m_task[sTaskID];

	if (pMatchMaking)
	{
		if (pTask->error == eCLE_Success)
		{
			HANDLE sessionHandle = pMatchMaking->GetLiveSessionHandle(pTask->session);

			if (sessionHandle == INVALID_HANDLE_VALUE)
			{
				UpdateTaskError(sTaskID, eCLE_InvalidSession);
			}

			if (pTask->error == eCLE_Success)
			{
				XUID* pXUIDs = (XUID*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_XUIDS]);
				XSESSION_VIEW_PROPERTIES** ppViews = (XSESSION_VIEW_PROPERTIES**)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_VIEWS]);
				uint32* pNumBoards = (uint32*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_NUM_VIEWS]);
				uint32 currentID = pTask->paramsNum[STATS_PARAM_WRITE_CURRENT_ID];

				UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XSessionWriteStats(sessionHandle, pXUIDs[currentID], pNumBoards[currentID], ppViews[currentID], &pTask->overlapped)));
			}
		}
	}
	else
	{
		UpdateTaskError(sTaskID, eCLE_InternalError);
	}

	CryLogAlways("[Lobby] XSessionWriteStats (%d of %d) result %d", pTask->paramsNum[STATS_PARAM_WRITE_CURRENT_ID] + 1, pTask->paramsNum[STATS_PARAM_WRITE_NUM_XUIDS], pTask->error);

	if (pTask->error != eCLE_Success)
	{
		StopTaskRunning(sTaskID);
	}
}

void CCryLiveStats::TickStatsWriteLeaderBoards(CryStatsTaskID sTaskID)
{
	ECryLobbyError error = TickTask(sTaskID);

	if (error != eCLE_Pending)
	{
		STask* pTask = &m_task[sTaskID];

		UpdateTaskError(sTaskID, error);

		if (pTask->error == eCLE_Success)
		{
			pTask->paramsNum[STATS_PARAM_WRITE_CURRENT_ID]++;

			if (pTask->paramsNum[STATS_PARAM_WRITE_CURRENT_ID] < pTask->paramsNum[STATS_PARAM_WRITE_NUM_XUIDS])
			{
				StartStatsWriteLeaderBoards(sTaskID);
			}
			else
			{
				StopTaskRunning(sTaskID);
			}
		}
		else
		{
			StopTaskRunning(sTaskID);
		}
	}
}

ECryLobbyError CCryLiveStats::GetLeaderBoardSpec(CryStatsLeaderBoardID board, XUSER_STATS_SPEC* pSpec)
{
	for (uint32 i = 0; i < m_numLeaderBoards; i++)
	{
		SRegisterLeaderBoardData* pBoard = &m_leaderBoards[i];

		if (pBoard->boardID == board)
		{
			pSpec->dwViewId = pBoard->boardID;
			pSpec->dwNumColumnIds = pBoard->numColumns;

			for (uint32 j = 0; j < pBoard->numColumns; j++)
			{
				pSpec->rgwColumnIds[j] = pBoard->pColumns[j].columnID;
			}

			return eCLE_Success;
		}
	}

	return eCLE_LeaderBoardNotRegistered;
}

#define STATS_PARAM_READ_LEADERBOARD_XUIDS				0			// ptr, num
#define STATS_PARAM_READ_LEADERBOARD_NUM					0			// num
#define STATS_PARAM_READ_LEADERBOARD_SPEC					1			// ptr
#define STATS_PARAM_READ_LEADERBOARD_BOARD				1			// num
#define STATS_PARAM_READ_LEADERBOARD_RESULTS			2			// ptr
#define STATS_PARAM_READ_LEADERBOARD_START				2			// num
#define STATS_PARAM_READ_LEADERBOARD_GAME_ROWS		3			// ptr
#define STATS_PARAM_READ_LEADERBOARD_GAME_COLUMNS	4			// ptr
void CCryLiveStats::StartEnumerate(CryStatsTaskID sTaskID, uint32 bufferSize)
{
	STask* pTask = &m_task[sTaskID];

	UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_RESULTS, NULL, bufferSize));

	if (pTask->error == eCLE_Success)
	{
		UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XEnumerate(pTask->enumerate, m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_RESULTS]), bufferSize, NULL, &pTask->overlapped)));
	}
}

ECryLobbyError CCryLiveStats::StatsReadLeaderBoardByRankForRange(CryStatsLeaderBoardID board, uint32 startRank, uint32 num, CryLobbyTaskID* pTaskID, CryStatsReadLeaderBoardCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	ECryLobbyError error = eCLE_Success;
	CryStatsTaskID sTaskID;

	error = StartTask(eT_StatsReadLeaderBoardByRankForRange, false, 0, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

	if (error == eCLE_Success)
	{
		STask* pTask = &m_task[sTaskID];

		pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_BOARD] = board;
		pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_START] = startRank;
		pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_NUM] = num;

		FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
	}

	return error;
}

void CCryLiveStats::StartStatsReadLeaderBoardByRankForRange(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_SPEC, NULL, sizeof(XUSER_STATS_SPEC)));

	if (pTask->error == eCLE_Success)
	{
		XUSER_STATS_SPEC* pSpec = (XUSER_STATS_SPEC*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_SPEC]);

		UpdateTaskError(sTaskID, GetLeaderBoardSpec(pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_BOARD], pSpec));

		if (pTask->error == eCLE_Success)
		{
			DWORD bufferSize;

			UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XUserCreateStatsEnumeratorByRank(0, pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_START], pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_NUM], 1, pSpec, &bufferSize, &pTask->enumerate)));

			if (pTask->error == eCLE_Success)
			{
				StartEnumerate(sTaskID, bufferSize);
			}
		}
	}

	CryLogAlways("[Lobby] StartStatsReadLeaderBoardByRankForRange result %d", pTask->error);

	if (pTask->error != eCLE_Success)
	{
		StopTaskRunning(sTaskID);
	}
}

void CCryLiveStats::TickStatsReadLeaderBoard(CryStatsTaskID sTaskID)
{
	ECryLobbyError error = TickTask(sTaskID);

	if (error != eCLE_Pending)
	{
		UpdateTaskError(sTaskID, error);
		StopTaskRunning(sTaskID);
	}
}

void CCryLiveStats::EndStatsReadLeaderBoard(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	if (pTask->error == eCLE_Success)
	{
		XUSER_STATS_READ_RESULTS* pResults = (XUSER_STATS_READ_RESULTS*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_RESULTS]);
		XUSER_STATS_VIEW *pView = &pResults->pViews[0];

		for (uint32 i = 0; i < m_numLeaderBoards; i++)
		{
			SRegisterLeaderBoardData* leaderBoard = &m_leaderBoards[i];

			if (leaderBoard->boardID == pView->dwViewId)
			{
				SCryStatsLeaderBoardReadResult result;

				result.id = pView->dwViewId;
				result.numRows = pView->dwNumRows;
				result.totalNumBoardRows = pView->dwTotalViewRows;

				UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_GAME_ROWS, NULL, result.numRows*sizeof(result.pRows[0])));

				if (pTask->error == eCLE_Success)
				{
					uint32 numColumns = 0;
					uint32 columnIndex = 0;
					SCryStatsLeaderBoardUserColumn* pColumns = NULL;

					result.pRows = (SCryStatsLeaderBoardReadRow*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_GAME_ROWS]);

					for (uint32 j = 0; j < pView->dwNumRows; j++)
					{
						numColumns += pView->pRows[j].dwNumColumns;
					}

					if (numColumns > 0)
					{
						UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_GAME_COLUMNS, NULL, numColumns*sizeof(result.pRows[0].data.pColumns[0])));

						if (pTask->error == eCLE_Success)
						{
							pColumns = (SCryStatsLeaderBoardUserColumn*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_GAME_COLUMNS]);
						}
					}

					if (pTask->error == eCLE_Success)
					{
						TMemHdl added = TMemInvalidHdl;
						bool* pAdded = NULL;

						if (pTask->startedTask == eT_StatsReadLeaderBoardByUserID)
						{
							added = m_pLobby->MemAlloc(pView->dwNumRows*sizeof(bool));

							if (added == TMemInvalidHdl)
							{
								UpdateTaskError(sTaskID, eCLE_OutOfMemory);
							}
							else
							{
								pAdded = (bool*)m_pLobby->MemGetPtr(added);
								memset(pAdded, 0, pView->dwNumRows*sizeof(bool));
							}
						}

						for (uint32 j = 0; (j < pView->dwNumRows) && (pTask->error == eCLE_Success); j++)
						{
							uint32 indexAdd = j;

							if (pAdded)
							{
								uint32 bestIndex = 0;
								uint32 bestRank = 0xffffffff;

								for (uint32 k = 0; k < pView->dwNumRows; k++)
								{
									if (!pAdded[k] && (pView->pRows[k].dwRank < bestRank))
									{
										bestRank = pView->pRows[k].dwRank;
										bestIndex = k;
									}
								}

								indexAdd = bestIndex;
								pAdded[indexAdd] = true;
							}

							SCryStatsLeaderBoardReadRow* pRow = &result.pRows[j];
							XUSER_STATS_ROW* pLiveRow = &pView->pRows[indexAdd];

							pRow->rank = pLiveRow->dwRank;
							strncpy(pRow->name, pLiveRow->szGamertag, CRYLOBBY_USER_NAME_LENGTH);

							pRow->data.score.id = leaderBoard->scoreID;
							pRow->data.score.score = pLiveRow->i64Rating;
							pRow->data.numColumns = pLiveRow->dwNumColumns;

							if ((pLiveRow->dwNumColumns > 0) && pColumns)
							{
								pRow->data.pColumns = &pColumns[columnIndex];
								columnIndex += pLiveRow->dwNumColumns;

								for (uint32 k = 0; (k < pLiveRow->dwNumColumns) && (pTask->error == eCLE_Success); k++)
								{
									SCryStatsLeaderBoardUserColumn* pColumn = &pRow->data.pColumns[k];
									XUSER_STATS_COLUMN* pLiveColumn = &pLiveRow->pColumns[k];
									bool found = false;

									for (uint32 l = 0; l < leaderBoard->numColumns; l++)
									{
										if (leaderBoard->pColumns[l].columnID == pLiveColumn->wColumnId)
										{
											SLiveUserData liveData;

											found = true;
											pColumn->columnID = pLiveColumn->wColumnId;

											liveData.typeContext = false;
											liveData.property.dwPropertyId = leaderBoard->pColumns[l].dataID;
											liveData.property.value = pLiveColumn->Value;

											UpdateTaskError(sTaskID, LiveUserDataToCryUserData(&liveData, &pColumn->data, (ECryLobbyUserDataType)leaderBoard->pColumns[l].dataType));
											break;
										}
									}

									if (!found)
									{
										UpdateTaskError(sTaskID, eCLE_UserDataNotRegistered);
									}
								}
							}
							else
							{
								pRow->data.pColumns = NULL;
							}
						}

						if (added != TMemInvalidHdl)
						{
							m_pLobby->MemFree(added);
						}

						((CryStatsReadLeaderBoardCallback)pTask->pCb)(pTask->lTaskID, pTask->error, &result, pTask->pCbArg);
					}
				}

				break;
			}
		}
	}
	
	if (pTask->error != eCLE_Success)
	{
		((CryStatsReadLeaderBoardCallback)pTask->pCb)(pTask->lTaskID, pTask->error, NULL, pTask->pCbArg);
	}
}

ECryLobbyError CCryLiveStats::StatsReadLeaderBoardByRankForUser(CryStatsLeaderBoardID board, uint32 user, uint32 num, CryLobbyTaskID* pTaskID, CryStatsReadLeaderBoardCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	ECryLobbyError error = eCLE_Success;
	CryStatsTaskID sTaskID;

	error = StartTask(eT_StatsReadLeaderBoardByRankForUser, false, 0, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

	if (error == eCLE_Success)
	{
		STask* pTask = &m_task[sTaskID];

		pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_BOARD] = board;
		pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_START] = user;
		pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_NUM] = num;

		FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
	}

	return error;
}

void CCryLiveStats::StartStatsReadLeaderBoardByRankForUser(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_SPEC, NULL, sizeof(XUSER_STATS_SPEC)));

	if (pTask->error == eCLE_Success)
	{
		XUSER_STATS_SPEC* pSpec = (XUSER_STATS_SPEC*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_SPEC]);

		UpdateTaskError(sTaskID, GetLeaderBoardSpec(pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_BOARD], pSpec));

		if (pTask->error == eCLE_Success)
		{
			CCryLiveLobbyService* pService = (CCryLiveLobbyService*)m_pService;
			XUID xuid;

			UpdateTaskError(sTaskID, pService->GetUserXUID(pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_START], &xuid));

			if (pTask->error == eCLE_Success)
			{
				DWORD bufferSize;

				UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XUserCreateStatsEnumeratorByXuid(0, xuid, pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_NUM], 1, pSpec, &bufferSize, &pTask->enumerate)));

				if (pTask->error == eCLE_Success)
				{
					StartEnumerate(sTaskID, bufferSize);
				}
			}
		}
	}

	CryLogAlways("[Lobby] StartStatsReadLeaderBoardByRankForUser result %d", pTask->error);

	if (pTask->error != eCLE_Success)
	{
		StopTaskRunning(sTaskID);
	}
}

ECryLobbyError CCryLiveStats::StatsReadLeaderBoardByUserID(CryStatsLeaderBoardID board, CryUserID* pUserIDs, uint32 numUserIDs, CryLobbyTaskID* pTaskID, CryStatsReadLeaderBoardCallback cb, void* pCbArg)
{
	ECryLobbyError error = eCLE_Success;
	CryStatsTaskID sTaskID;

	LOBBY_AUTO_LOCK;

	error = StartTask(eT_StatsReadLeaderBoardByUserID, false, 0, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

	if (error == eCLE_Success)
	{
		error = CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_XUIDS, NULL, numUserIDs*sizeof(XUID));

		if (error == eCLE_Success)
		{
			STask* pTask = &m_task[sTaskID];
			XUID* pXUIDs = (XUID*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_XUIDS]);

			for (uint32 i = 0; i < numUserIDs; i++)
			{
				if (pUserIDs[i] == CryUserInvalidID)
				{
					error = eCLE_InvalidUser;
					break;
				}
				
				pXUIDs[i] = ((SCryLiveUserID*)pUserIDs[i].get())->xuid;
			}

			if (error == eCLE_Success)
			{
				pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_BOARD] = board;
				pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_XUIDS] = numUserIDs;

				FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
			}
		}

		if (error != eCLE_Success)
		{
			FreeTask(sTaskID);
		}
	}

	CryLogAlways("[Lobby] Start StatsReadLeaderBoardByUserID return %d", error);

	return error;
}

void CCryLiveStats::StartStatsReadLeaderBoardByUserID(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_SPEC, NULL, sizeof(XUSER_STATS_SPEC)));

	if (pTask->error == eCLE_Success)
	{
		XUSER_STATS_SPEC* pSpec = (XUSER_STATS_SPEC*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_SPEC]);

		UpdateTaskError(sTaskID, GetLeaderBoardSpec(pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_BOARD], pSpec));

		if (pTask->error == eCLE_Success)
		{
			XUID* pXUIDs = (XUID*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_XUIDS]);
			uint32 numXUIDs = pTask->paramsNum[STATS_PARAM_READ_LEADERBOARD_XUIDS];
			DWORD bufferSize = 0;

			DWORD ret = XUserReadStats(0, numXUIDs, pXUIDs, 1, pSpec, &bufferSize, NULL, NULL);

			if ((ERROR_INSUFFICIENT_BUFFER == ret) && (bufferSize > 0))
			{
				UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_LEADERBOARD_RESULTS, NULL, bufferSize));

				if (pTask->error == eCLE_Success)
				{
					XUSER_STATS_READ_RESULTS* pResults = (XUSER_STATS_READ_RESULTS*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_LEADERBOARD_RESULTS]);

					UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XUserReadStats(0, numXUIDs, pXUIDs, 1, pSpec, &bufferSize, pResults, &pTask->overlapped)));
				}
			}
			else
			{
				UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(ret));
			}
		}
	}

	if (pTask->error != eCLE_Success)
	{
		StopTaskRunning(sTaskID);
	}
}

ECryLobbyError CCryLiveStats::StatsRegisterUserData(SCryLobbyUserData* pData, uint32 numData, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg)
{
	ECryLobbyError error = eCLE_Success;
	CryStatsTaskID sTaskID;

	LOBBY_AUTO_LOCK;

	error = StartTask(eT_StatsRegisterUserData, false, 0, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

	if (error == eCLE_Success)
	{
		if ((m_numUserData + numData) > m_userData.size())
		{
			m_userData.resize(m_userData.size() + numData);
		}

		if (m_profileSettings.numSettings == 0)
		{
			SProfileSettings::SSettings* pSettings = &m_profileSettings.settings[0];

			m_profileSettings.numSettings = 1;
			pSettings->userDataStart = 0;
			pSettings->userDataNum = 0;
			pSettings->userDataSize = 0;
		}

		for (uint32 i = 0; (i < numData) && (error == eCLE_Success); i++)
		{
			uint32 itemSize;

			switch (pData[i].m_type)
			{
			case eCLUDT_Int64:
				itemSize = sizeof(pData[i].m_int64);
				break;

			case eCLUDT_Int32:
				itemSize = sizeof(pData[i].m_int32);
				break;

			case eCLUDT_Int16:
				itemSize = sizeof(pData[i].m_int16);
				break;

			case eCLUDT_Int8:
				itemSize = sizeof(pData[i].m_int8);
				break;

			case eCLUDT_Float64:
				itemSize = sizeof(pData[i].m_f32);
				break;

			case eCLUDT_Float32:
				itemSize = sizeof(pData[i].m_f64);
				break;

			case eCLUDT_Int64NoEndianSwap:
				itemSize = sizeof(pData[i].m_int64);
				break;

			default:
				itemSize = 0;
				CRY_ASSERT_MESSAGE(0, "CCryLiveStats::StatsRegisterUserData: Undefined data type");
				break;
			}

			SProfileSettings::SSettings* pSettings = &m_profileSettings.settings[m_profileSettings.numSettings - 1];

			if (pSettings->userDataSize + itemSize > XPROFILE_SETTING_MAX_SIZE)
			{
				if (m_profileSettings.numSettings == MAX_PROFILE_TITLE_SETTINGS)
				{
					error = eCLE_OutOfUserData;
				}
				else
				{
					pSettings = &m_profileSettings.settings[m_profileSettings.numSettings];

					m_profileSettings.numSettings++;
					pSettings->userDataStart = m_numUserData;
					pSettings->userDataNum = 0;
					pSettings->userDataSize = 0;
				}
			}

			if (error == eCLE_Success)
			{
				m_userData[m_numUserData].id = pData[i].m_id;
				m_userData[m_numUserData].type = pData[i].m_type;
				pSettings->userDataNum++;
				pSettings->userDataSize += itemSize;

				m_numUserData++;
			}
		}

		FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
	}

	CryLogAlways("[Lobby] Start StatsRegisterUserData return %d", error);

	return error;
}

#define STATS_PARAM_WRITE_USER_DATA_SETTINGS			0
#define STATS_PARAM_WRITE_USER_DATA_BLOCKS_START	1
ECryLobbyError CCryLiveStats::StatsWriteUserData(uint32 user, SCryLobbyUserData* pData, uint32 numData, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	ECryLobbyError error = eCLE_Success;

	if (m_numUserData > 0)
	{
		CryStatsTaskID sTaskID;

		error = StartTask(eT_StatsWriteUserData, false, user, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

		if (error == eCLE_Success)
		{
			error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_USER_DATA_SETTINGS, NULL, m_profileSettings.numSettings*sizeof(XUSER_PROFILE_SETTING));

			if (error == eCLE_Success)
			{
				STask* pTask = &m_task[sTaskID];
				XUSER_PROFILE_SETTING* pLiveSettings = (XUSER_PROFILE_SETTING*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_USER_DATA_SETTINGS]);

				for (uint32 i = 0; (i < m_profileSettings.numSettings) && (error == eCLE_Success); i++)
				{
					SProfileSettings::SSettings* pSettings = &m_profileSettings.settings[i];
					XUSER_PROFILE_SETTING* pLiveSetting = &pLiveSettings[i];

					error = CreateTaskParamMem(sTaskID, STATS_PARAM_WRITE_USER_DATA_BLOCKS_START + i, NULL, pSettings->userDataSize);

					if (error == eCLE_Success)
					{
						uint32 bufferPos = 0;

						pLiveSetting->dwSettingId = pSettings->id;
						pLiveSetting->data.type = XUSER_DATA_TYPE_BINARY;
						pLiveSetting->data.binary.pbData = (BYTE*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_USER_DATA_BLOCKS_START + i]);
						pLiveSetting->data.binary.cbData = pSettings->userDataSize;

						for (uint32 j = 0; j < pSettings->userDataNum; j++)
						{
							if (pData[pSettings->userDataStart + j].m_id == m_userData[pSettings->userDataStart + j].id)
							{
								AddPacketSCryLobbyUserData(pLiveSetting->data.binary.pbData, pLiveSetting->data.binary.cbData, &bufferPos, &pData[pSettings->userDataStart + j]);
							}
							else
							{
								error = eCLE_UserDataMissMatch;
							}
						}
					}
				}

				if (error == eCLE_Success)
				{
					FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
				}
			}

			if (error != eCLE_Success)
			{
				FreeTask(sTaskID);
			}
		}
	}
	else
	{
		error = eCLE_NoUserDataRegistered;
	}

	CryLogAlways("[Lobby] Stats StatsWriteUserData return %d", error);

	return error;
}

void CCryLiveStats::StartStatsWriteUserData(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];
	XUSER_PROFILE_SETTING* pLiveSettings = (XUSER_PROFILE_SETTING*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_WRITE_USER_DATA_SETTINGS]);

	UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XUserWriteProfileSettings(pTask->user, m_profileSettings.numSettings, pLiveSettings, &pTask->overlapped)));

	CryLogAlways("[Lobby] Stats StartStatsWriteUserData result %d", pTask->error);

	if (pTask->error != eCLE_Success)
	{
		StopTaskRunning(sTaskID);
	}
}

void CCryLiveStats::TickStatsWriteUserData(CryStatsTaskID sTaskID)
{
	ECryLobbyError error = TickTask(sTaskID);

	if (error != eCLE_Pending)
	{
		UpdateTaskError(sTaskID, error);
		StopTaskRunning(sTaskID);
	}
}

ECryLobbyError CCryLiveStats::StatsReadUserData(uint32 user, CryLobbyTaskID* pTaskID, CryStatsReadUserDataCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	ECryLobbyError error = eCLE_Success;

	if (m_numUserData > 0)
	{
		CryStatsTaskID sTaskID;
		CCryLiveLobbyService* pService = (CCryLiveLobbyService*)m_pService;
		XUID xuid;

		error = pService->GetUserXUID(user, &xuid);

		if (error == eCLE_Success)
		{
			error = StartTask(eT_StatsReadUserData, false, user, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

			if (error == eCLE_Success)
			{
				m_task[sTaskID].xuid = xuid;
				FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
			}
		}
	}
	else
	{
		error = eCLE_NoUserDataRegistered;
	}
	
	CryLogAlways("[Lobby] Stats StatsReadUserData return %d", error);

	return error;
}

ECryLobbyError CCryLiveStats::StatsReadUserData(uint32 user, CryUserID userID, CryLobbyTaskID* pTaskID, CryStatsReadUserDataCallback cb, void* pCbArg)
{
	LOBBY_AUTO_LOCK;

	ECryLobbyError error = eCLE_Success;

	if (m_numUserData > 0)
	{
		CryStatsTaskID sTaskID;

		error = StartTask(eT_StatsReadUserData, false, user, &sTaskID, pTaskID, CrySessionInvalidHandle, cb, pCbArg);

		if (error == eCLE_Success)
		{
			m_task[sTaskID].xuid = ((SCryLiveUserID*)userID.get())->xuid;
			FROM_GAME(&CCryLiveStats::StartTaskRunning, this, sTaskID);
		}
	}
	else
	{
		error = eCLE_NoUserDataRegistered;
	}

	CryLogAlways("[Lobby] Stats StatsReadUserData return %d", error);

	return error;
}

#define STATS_PARAM_READ_USER_DATA_SETTINGIDS			0
#define STATS_PARAM_READ_USER_DATA_RESULTS				1
#define STATS_PARAM_READ_USER_DATA_GAME_RESULTS		2
void CCryLiveStats::StartStatsReadUserData(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_USER_DATA_SETTINGIDS, NULL, m_profileSettings.numSettings*sizeof(DWORD)));

	if (pTask->error == eCLE_Success)
	{
		DWORD* pSettings = (DWORD*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_USER_DATA_SETTINGIDS]);
		DWORD bufferSize = 0;

		for (uint32 i = 0; i < m_profileSettings.numSettings; i++)
		{
			pSettings[i] = m_profileSettings.settings[i].id;
		}

		DWORD ret = XUserReadProfileSettingsByXuid(0, pTask->user, 1, &pTask->xuid, m_profileSettings.numSettings, pSettings, &bufferSize, NULL, NULL);
	
		if ((ret == ERROR_INSUFFICIENT_BUFFER) && (bufferSize > 0))
		{
			UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_USER_DATA_RESULTS, NULL, bufferSize));

			if (pTask->error == eCLE_Success)
			{
				XUSER_READ_PROFILE_SETTING_RESULT* pResults = (XUSER_READ_PROFILE_SETTING_RESULT*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_USER_DATA_RESULTS]);

				UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(XUserReadProfileSettingsByXuid(0, pTask->user, 1, &pTask->xuid, m_profileSettings.numSettings, pSettings, &bufferSize, pResults, &pTask->overlapped)));
			}
		}
		else
		{
			UpdateTaskError(sTaskID, CryLiveLobbyGetErrorFromLive(ret));
		}
	}

	CryLogAlways("[Lobby] Stats StartStatsReadUserData result %d", pTask->error);

	if (pTask->error != eCLE_Success)
	{
		StopTaskRunning(sTaskID);
	}
}

void CCryLiveStats::TickStatsReadUserData(CryStatsTaskID sTaskID)
{
	ECryLobbyError error = TickTask(sTaskID);

	if (error != eCLE_Pending)
	{
		UpdateTaskError(sTaskID, error);
		StopTaskRunning(sTaskID);
	}
}

void CCryLiveStats::EndStatsReadUserData(CryStatsTaskID sTaskID)
{
	STask* pTask = &m_task[sTaskID];

	if (pTask->error == eCLE_Success)
	{
		UpdateTaskError(sTaskID, CreateTaskParamMem(sTaskID, STATS_PARAM_READ_USER_DATA_GAME_RESULTS, NULL, m_numUserData*sizeof(SCryLobbyUserData)));

		if (pTask->error == eCLE_Success)
		{
			SCryLobbyUserData* pResults = (SCryLobbyUserData*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_USER_DATA_GAME_RESULTS]);
			XUSER_READ_PROFILE_SETTING_RESULT* pLiveProfileSettingResult = (XUSER_READ_PROFILE_SETTING_RESULT*)m_pLobby->MemGetPtr(pTask->paramsMem[STATS_PARAM_READ_USER_DATA_RESULTS]);

			for (uint32 i = 0; (i < pLiveProfileSettingResult->dwSettingsLen) && (pTask->error == eCLE_Success); i++)
			{
				XUSER_PROFILE_SETTING *pLiveSettings = &pLiveProfileSettingResult->pSettings[i];
				uint8* pBuffer = pLiveSettings->data.binary.pbData;
				uint32 buffserSize = pLiveSettings->data.binary.cbData;

				if (pBuffer)
				{
					uint32 userDataStart = 0xffffffff;
					uint32 userDataNum;

					for (uint32 j = 0; j < m_profileSettings.numSettings; j++)
					{
						SProfileSettings::SSettings* pSettings = &m_profileSettings.settings[j];

						if (pLiveSettings->dwSettingId == pSettings->id)
						{
							userDataStart = pSettings->userDataStart;
							userDataNum = pSettings->userDataNum;
							break;
						}
					}

					if (userDataStart < 0xffffffff)
					{
						uint32 bufferPos = 0;

						for (uint32 j = 0; j < userDataNum; j++)
						{
							SCryLobbyUserData* pResult = &pResults[userDataStart + j];
							SRegisterUserData* pRegistered = &m_userData[userDataStart + j];

							pResult->m_id = pRegistered->id;
							pResult->m_type = (ECryLobbyUserDataType)pRegistered->type;
							RemovePacketSCryLobbyUserData(pBuffer, buffserSize, &bufferPos, pResult);
						}
					}
					else
					{
						UpdateTaskError(sTaskID, eCLE_ReadDataNotWritten);
					}
				}
				else
				{
					UpdateTaskError(sTaskID, eCLE_ReadDataNotWritten);
				}
			}

			if (pTask->error == eCLE_Success)
			{
				((CryStatsReadUserDataCallback)pTask->pCb)(pTask->lTaskID, pTask->error, pResults, m_numUserData, pTask->pCbArg);
			}
		}
	}

	if (pTask->error != eCLE_Success)
	{
		((CryStatsReadUserDataCallback)pTask->pCb)(pTask->lTaskID, pTask->error, NULL, 0, pTask->pCbArg);
	}
}
