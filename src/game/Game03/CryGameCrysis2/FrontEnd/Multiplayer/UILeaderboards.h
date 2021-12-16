/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: UI Leaderboards

-------------------------------------------------------------------------
History:
- 03/2010: Created by Ben Johnson

*************************************************************************/

#ifndef __UI_LEADERBOARDS_H__
#define __UI_LEADERBOARDS_H__

#pragma once

struct IFlashPlayer;
#include "PersistantStats.h"
#include <ICryLobby.h>
#include <ICryStats.h>

struct SCryStatsLeaderBoardReadResult;

CRY_TODO(08, 03, 2010, "Leaderboards currently only xenon. Certain methods will required refactoring when PS3 leaderboards are available.")

class CUILeaderboards
{
public:
	CUILeaderboards();
	~CUILeaderboards();

	void Reset();

	void EnterLeaderboard();
	void LeaveLeaderboard();

#ifndef _RELEASE
	void Update(float dt);
	void ReadFromLeaderboards();
#endif

	void SetEntriesSize(int num) { m_numEntries = num; }

	void GetLocalUserLeaderboard();
	void MoveTopLeaderboard();
	void MoveLeaderboard(bool up);
	void GetNextLeaderboardType(bool previous);

private:

	typedef CryFixedStringT<32> TFixedString32;
	typedef CryFixedStringT<16> TFixedString16;
	typedef CryFixedStringT<8> TFixedString8;

	enum ELeaderboardRequestType
	{
		eLRT_LocalUser,
		eLRT_User,
		eLRT_Range,
	};

	struct SLeaderboardEntry
	{
		SLeaderboardEntry() : m_name(0) { }

		const char * m_name;
		TFixedString16 m_rank;
		TFixedString16 m_score;
		TFixedString16 m_column0;
		TFixedString16 m_column1;
		TFixedString16 m_column2;
		TFixedString16 m_column3;
	};

	const char * GetScoreString(CryStatsLeaderBoardID leaderboardId, uint64 score);

	void ProcessLeaderboardRead(SCryStatsLeaderBoardReadResult* pResult, ECryLobbyError error);

	CryLobbyTaskID GetCurrentTask() { return m_currentTask; }
	void SetCurrentTask(CryLobbyTaskID task) { m_currentTask = task; }

	// Static Callbacks
	static void ReadLeaderboardCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryStatsLeaderBoardReadResult* pResult, void* pArg);

	// Flash functions
	void UpdateStatusFlash(IFlashPlayer *pFlashPlayer);
	void SendLeaderboardToFlash(IFlashPlayer *pFlashPlayer, SCryStatsLeaderBoardReadResult* pResult);

#if defined(XENON)
	void SendLeaderboardChangedToFlash(IFlashPlayer *pFlashPlayer, ELeaderboards leaderboardId);
#endif // ~XENON

	void SetLocalUserName(IFlashPlayer *pFlashPlayer);

	IFlashPlayer *m_pFlashPlayer; // Pointer to menu flash player.

	CryLobbyTaskID m_currentTask;

	int	m_startIndex;
	int m_currentNum;
	int m_numEntries;
	int m_currentLeaderboardIdx;
	
	bool m_atLeaderboardEnd;

	ELeaderboardRequestType m_currentLeaderboardRequestType;
};
#endif /*__UI_LEADERBOARDS_H__*/