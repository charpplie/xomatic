#ifndef __CRYLIVESTATS_H__
#define __CRYLIVESTATS_H__

#pragma once

#include "Lobby/CryStats.h"

class CCryLiveStats : public CCryStats
{
public:
																	CCryLiveStats(CCryLobby* pLobby, CCryLobbyService* pService);

					void										Tick(CTimeValue tv);

	virtual	ECryLobbyError					StatsRegisterLeaderBoards(SCryStatsLeaderBoardWrite* pBoards, uint32 numBoards, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg);
	virtual	ECryLobbyError					StatsWriteLeaderBoards(CrySessionHandle session, uint32 user, SCryStatsLeaderBoardWrite* pBoards, uint32 numBoards, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg);
	virtual	ECryLobbyError					StatsWriteLeaderBoards(CrySessionHandle session, CryUserID* pUserIDs, SCryStatsLeaderBoardWrite** ppBoards, uint32* pNumBoards, uint32 numUsers, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg);
	virtual ECryLobbyError					StatsReadLeaderBoardByRankForRange(CryStatsLeaderBoardID board, uint32 startRank, uint32 num, CryLobbyTaskID* pTaskID, CryStatsReadLeaderBoardCallback cb, void* pCbArg);
	virtual ECryLobbyError					StatsReadLeaderBoardByRankForUser(CryStatsLeaderBoardID board, uint32 user, uint32 num, CryLobbyTaskID* pTaskID, CryStatsReadLeaderBoardCallback cb, void* pCbArg);
	virtual ECryLobbyError					StatsReadLeaderBoardByUserID(CryStatsLeaderBoardID board, CryUserID* pUserIDs, uint32 numUserIDs, CryLobbyTaskID* pTaskID, CryStatsReadLeaderBoardCallback cb, void* pCbArg);

	virtual ECryLobbyError					StatsRegisterUserData(SCryLobbyUserData* pData, uint32 numData, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg);
	virtual ECryLobbyError					StatsWriteUserData(uint32 user, SCryLobbyUserData* pData, uint32 numData, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg);
	virtual ECryLobbyError					StatsReadUserData(uint32 user, CryLobbyTaskID* pTaskID, CryStatsReadUserDataCallback cb, void* pCbArg);
	virtual ECryLobbyError					StatsReadUserData(uint32 user, CryUserID userID, CryLobbyTaskID* pTaskID, CryStatsReadUserDataCallback cb, void* pCbArg);

private:
	enum ETask
	{
		eT_StatsRegisterLeaderBoards,
		eT_StatsWriteLeaderBoards,
		eT_StatsReadLeaderBoardByRankForRange,
		eT_StatsReadLeaderBoardByRankForUser,
		eT_StatsReadLeaderBoardByUserID,
		eT_StatsRegisterUserData,
		eT_StatsWriteUserData,
		eT_StatsReadUserData
	};

	struct SRegisterLeaderBoardData
	{
		CryStatsLeaderBoardID							boardID;
		CryLobbyUserDataID								scoreID;
		uint32														numColumns;

		struct SColumn
		{
			CryLobbyUserDataID							dataID;
			uint16													columnID;
			uint16													dataType;
		};
		
		SColumn*													pColumns;
	};

	struct SRegisterUserData
	{
		uint16														id;
		uint16														type;
	};

	struct  STask : public CCryStats::STask
	{
		XUID															xuid;
		XOVERLAPPED												overlapped;
		HANDLE														enumerate;
		uint32														user;
		uint32														count;
	};

	ECryLobbyError											StartTask(ETask etask, bool startRunning, uint32 user, CryStatsTaskID* pSTaskID, CryLobbyTaskID* pLTaskID, CrySessionHandle h, void* pCb, void* pCbArg);
	void																StartSubTask(ETask etask, CryStatsTaskID sTaskID);
	void																StartTaskRunning(CryStatsTaskID sTaskID);
	void																EndTask(CryStatsTaskID sTaskID);
	void																StopTaskRunning(CryStatsTaskID sTaskID);
	ECryLobbyError											TickTask(CryStatsTaskID sTaskID);

	void																StartEnumerate(CryStatsTaskID sTaskID, uint32 bufferSize);
	ECryLobbyError											GetLeaderBoardSpec(CryStatsLeaderBoardID board, XUSER_STATS_SPEC* pSpec);

	ECryLobbyError											StatsWriteLeaderBoards(CrySessionHandle session, XUID* pXUIDs, SCryStatsLeaderBoardWrite** ppBoards, uint32* pNumBoards, uint32 numUsers, CryLobbyTaskID* pTaskID, CryStatsCallback cb, void* pCbArg);
	void																StartStatsWriteLeaderBoards(CryStatsTaskID sTaskID);
	void																TickStatsWriteLeaderBoards(CryStatsTaskID sTaskID);

	void																StartStatsReadLeaderBoardByRankForRange(CryStatsTaskID sTaskID);
	void																TickStatsReadLeaderBoard(CryStatsTaskID sTaskID);
	void																EndStatsReadLeaderBoard(CryStatsTaskID sTaskID);
	void																StartStatsReadLeaderBoardByRankForUser(CryStatsTaskID sTaskID);

	void																StartStatsReadLeaderBoardByUserID(CryStatsTaskID sTaskID);

	void																StartStatsWriteUserData(CryStatsTaskID sTaskID);
	void																TickStatsWriteUserData(CryStatsTaskID sTaskID);

	void																StartStatsReadUserData(CryStatsTaskID sTaskID);
	void																TickStatsReadUserData(CryStatsTaskID sTaskID);
	void																EndStatsReadUserData(CryStatsTaskID sTaskID);

	STask																m_task[MAX_STATS_TASKS];
	DynArray<SRegisterLeaderBoardData>	m_leaderBoards;
	DynArray<SRegisterUserData>					m_userData;
	uint32															m_numLeaderBoards;
	uint32															m_numUserData;

	struct SProfileSettings
	{
		uint32														numSettings;

		#define MAX_PROFILE_TITLE_SETTINGS	3
		struct SSettings
		{
			uint32													id;
			uint32													userDataStart;
			uint32													userDataNum;
			uint32													userDataSize;
		}																	settings[MAX_PROFILE_TITLE_SETTINGS];
	}																		m_profileSettings;
};

#endif // __CRYLIVESTATS_H__
