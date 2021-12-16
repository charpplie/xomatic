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

#include "StdAfx.h"
#include "UILeaderboards.h"
#include "Flash/Flash.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/Multiplayer/MPMenuHub.h"
#include "HUD/HUD.h"
#include "StringUtils.h"
#include "IPlayerProfiles.h"
#include "Menus/OptionsManager.h"

#ifndef _RELEASE
#include "Utility/CryWatch.h"
static int g_debugLeaderboardsUI = 0;
#endif

//------------------------------------------------------------------------
CUILeaderboards::CUILeaderboards()
{
	Reset();
	
	m_currentTask = CryLobbyInvalidTaskID;

#ifndef _RELEASE
	if (gEnv->pConsole)
	{
		REGISTER_CVAR(g_debugLeaderboardsUI, 0, VF_NULL, "Debugging for UI online leaderboards");
	}
#endif
}

//------------------------------------------------------------------------
CUILeaderboards::~CUILeaderboards()
{
#ifndef _RELEASE
	if (gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("g_debugLeaderboardsUI", true);
	}
#endif
}

void CUILeaderboards::Reset()
{
	m_pFlashPlayer = NULL;

	m_startIndex = 0;
	m_numEntries = 0;

	m_atLeaderboardEnd = false;

	m_currentLeaderboardRequestType = eLRT_LocalUser;
	m_currentLeaderboardIdx = 0;
}

//------------------------------------------------------------------------
void CUILeaderboards::EnterLeaderboard()
{
	CFlashFrontEnd *flashMenu = g_pGame->GetFlashMenu();
	if (flashMenu)
		m_pFlashPlayer = flashMenu->GetFlash();

	SetLocalUserName(m_pFlashPlayer);

#if defined(XENON)
	uint32 leaderboardId = CPersistantStats::GetLeaderboardIdFromIndex(m_currentLeaderboardIdx);
	SendLeaderboardChangedToFlash(m_pFlashPlayer, (ELeaderboards)leaderboardId);
#endif
	
	GetLocalUserLeaderboard();
}

//------------------------------------------------------------------------
void CUILeaderboards::LeaveLeaderboard()
{
	Reset();
}

//------------------------------------------------------------------------
void CUILeaderboards::GetLocalUserLeaderboard()
{
#if defined(XENON)
	ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
	if(stats && m_numEntries>0)
	{
		m_currentLeaderboardRequestType = eLRT_LocalUser;

		uint32 leaderboardId = CPersistantStats::GetLeaderboardIdFromIndex(m_currentLeaderboardIdx);
		ECryLobbyError error = stats->StatsReadLeaderBoardByRankForUser(leaderboardId, 0, m_numEntries, &m_currentTask, ReadLeaderboardCallback, this);
		if (error == eCLE_Success)
		{
			UpdateStatusFlash(m_pFlashPlayer);
		}
		else
		{
			CHUD* pHUD = g_pGame->GetHUD();
			CMPMenuHub* pMPMenu = g_pGame->GetFlashMenu() ? g_pGame->GetFlashMenu()->GetMPMenu() : NULL;
			if (pHUD && pMPMenu)
			{
				TFixedString32 errorMsg;
				errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
				errorMsg = pHUD->LocalizeString(errorMsg.c_str());
				pMPMenu->ShowErrorDialog("LeaderboardError", errorMsg.c_str());
			}

			GameWarning("Leaderboard read failed error:%d", (int)error);
		}
	}
#endif
}

//------------------------------------------------------------------------
void CUILeaderboards::MoveTopLeaderboard()
{
#if defined(XENON)
	if ((m_numEntries>0) && (m_startIndex > 1))
	{
		if(ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats())
		{
			m_currentLeaderboardRequestType = eLRT_Range;

			uint32 leaderboardId = CPersistantStats::GetLeaderboardIdFromIndex(m_currentLeaderboardIdx);
			ECryLobbyError error = stats->StatsReadLeaderBoardByRankForRange(leaderboardId, 1, m_numEntries, &m_currentTask, ReadLeaderboardCallback, this);

			if (error == eCLE_Success)
			{
				UpdateStatusFlash(m_pFlashPlayer);
			}
			else
			{
				CHUD* pHUD = g_pGame->GetHUD();
				CMPMenuHub* pMPMenu = g_pGame->GetFlashMenu() ? g_pGame->GetFlashMenu()->GetMPMenu() : NULL;
				if (pHUD && pMPMenu)
				{
					TFixedString32 errorMsg;
					errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
					errorMsg = pHUD->LocalizeString(errorMsg.c_str());
					pMPMenu->ShowErrorDialog("LeaderboardError", errorMsg.c_str());
				}

				GameWarning("Leaderboard read failed error:%d", (int)error);
			}
		}
	}
#endif
}

//------------------------------------------------------------------------
void CUILeaderboards::MoveLeaderboard(bool up)
{
#if defined(XENON)
	bool canMove = true;

	if ((up && (m_startIndex <= 1)) || (!up && (m_atLeaderboardEnd==true)))	// At top, or bottom of leaderboard
		canMove = false;

	if ((m_numEntries>0) && canMove)
	{
		int startIndex = up ? MAX(m_startIndex - m_numEntries + 1, 1) : MAX(m_startIndex + m_numEntries - 1, 1);

		ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
		if(stats && (startIndex != m_startIndex))
		{
			m_currentLeaderboardRequestType = eLRT_Range;

			uint32 leaderboardId = CPersistantStats::GetLeaderboardIdFromIndex(m_currentLeaderboardIdx);
			ECryLobbyError error = stats->StatsReadLeaderBoardByRankForRange(leaderboardId, startIndex, m_numEntries, &m_currentTask, ReadLeaderboardCallback, this);
			if (error == eCLE_Success)
			{
				UpdateStatusFlash(m_pFlashPlayer);
			}
			else
			{
				CHUD* pHUD = g_pGame->GetHUD();
				CMPMenuHub* pMPMenu = g_pGame->GetFlashMenu() ? g_pGame->GetFlashMenu()->GetMPMenu() : NULL;
				if (pHUD && pMPMenu)
				{
					TFixedString32 errorMsg;
					errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
					errorMsg = pHUD->LocalizeString(errorMsg.c_str());
					pMPMenu->ShowErrorDialog("LeaderboardError", errorMsg.c_str());
				}
				
				GameWarning("Leaderboard read failed error:%d", (int)error);
			}

			CRY_ASSERT(error == eCLE_Success);
		}
	}
#endif
}

//------------------------------------------------------------------------
void CUILeaderboards::GetNextLeaderboardType(bool previous)
{
#if defined(XENON)
	ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();

	int leaderboardIdx = m_currentLeaderboardIdx;

	if (previous)
		leaderboardIdx--;
	else
		leaderboardIdx++;

	if (leaderboardIdx >= eLB_Num)
		leaderboardIdx = 0;
	else if (leaderboardIdx < 0)
		leaderboardIdx = eLB_Num - 1;

	if (leaderboardIdx != m_currentLeaderboardIdx)
	{
		m_currentLeaderboardIdx = leaderboardIdx;
		uint32 leaderboardId = CPersistantStats::GetLeaderboardIdFromIndex(m_currentLeaderboardIdx);
		SendLeaderboardChangedToFlash(m_pFlashPlayer, (ELeaderboards)leaderboardId);

		GetLocalUserLeaderboard();
	}
#endif
}

//------------------------------------------------------------------------
void CUILeaderboards::SetLocalUserName(IFlashPlayer *pFlashPlayer)
{
	if (!pFlashPlayer)
		return;

	IPlayerProfileManager *pPlayerProfileManager = g_pGame->GetOptions()->GetProfileManager();
	if (pPlayerProfileManager)
	{
		IPlayerProfile *pProfile = pPlayerProfileManager->GetCurrentProfile(pPlayerProfileManager->GetCurrentUser());
		if (pProfile)
		{
			pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("SetLocalUserName"), SFlashVarValue(pProfile->GetName()) );
		}
	}
}

//------------------------------------------------------------------------
void CUILeaderboards::UpdateStatusFlash(IFlashPlayer *pFlashPlayer)
{
	if (!pFlashPlayer)
		return;

	const char* status = NULL;
	bool busy = false;

	if (m_currentTask!=CryLobbyInvalidTaskID)
	{
		status = "@ui_menu_leaderboard_status_gathering_data";
		busy = true;
	}
	else
	{
		status = "@ui_menu_leaderboard_status_done";
	}

	SFlashVarValue args[2] = { busy, status };

	pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("UpdateStatus"), args, 2 );
}

//------------------------------------------------------------------------
void CUILeaderboards::SendLeaderboardToFlash(IFlashPlayer *pFlashPlayer, SCryStatsLeaderBoardReadResult* pResult)
{
	if (!pFlashPlayer || !pResult)
		return;

	if (pResult->numRows > 0)
	{
		m_startIndex = pResult->pRows[0].rank;
		m_atLeaderboardEnd = (pResult->totalNumBoardRows == pResult->pRows[pResult->numRows-1].rank);

		CryFixedArray<SLeaderboardEntry, 10> fixedStringsArray;

		for (uint32 i = 0; i < pResult->numRows; i++)
		{
			SLeaderboardEntry entry;
			entry.m_name = pResult->pRows[i].name;
			entry.m_rank = CryStringUtils::toString( pResult->pRows[i].rank ).c_str();
			entry.m_score = GetScoreString(pResult->id, pResult->pRows[i].data.score.score);

			int colNum = pResult->pRows[i].data.numColumns;
			if(colNum > 0)
			{
				entry.m_column0 = GetScoreString(pResult->id, pResult->pRows[i].data.pColumns[0].data.m_int64);

				if (colNum > 1)
					entry.m_column1 = GetScoreString(pResult->id, pResult->pRows[i].data.pColumns[1].data.m_int64);

				if (colNum > 2)
					entry.m_column2 = GetScoreString(pResult->id, pResult->pRows[i].data.pColumns[2].data.m_int64);

				if (colNum > 3)
					entry.m_column3 = GetScoreString(pResult->id, pResult->pRows[i].data.pColumns[3].data.m_int64);
			}

			fixedStringsArray.push_back(entry);
		}

		TFixedString16 totalRows;
		totalRows = CryStringUtils::toString( pResult->totalNumBoardRows ).c_str();

		int size = fixedStringsArray.size();
		if(size)
		{
			std::vector<const char*> pushArray;
			pushArray.reserve(size + 1);

			pushArray.push_back(totalRows.c_str());

			for (int i(0); i<size; ++i)
			{
				pushArray.push_back(fixedStringsArray[i].m_rank.c_str());
				pushArray.push_back(fixedStringsArray[i].m_name);
				pushArray.push_back(fixedStringsArray[i].m_score.c_str());
				pushArray.push_back(fixedStringsArray[i].m_column0.c_str());
				pushArray.push_back(fixedStringsArray[i].m_column1.c_str());
				pushArray.push_back(fixedStringsArray[i].m_column2.c_str());
				pushArray.push_back(fixedStringsArray[i].m_column3.c_str());
			}

			pFlashPlayer->SetVariableArray(FVAT_ConstStrPtr, FRONTEND_SUBSCREEN_PATH_SET("m_allValues"), 0, &pushArray[0], pushArray.size());
		}
	}

	pFlashPlayer->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("UpdateLeaderboard"));
}

//------------------------------------------------------------------------
const char * CUILeaderboards::GetScoreString(CryStatsLeaderBoardID leaderboardId, uint64 score)
{
	static TFixedString16 result;
	result.clear();

#if defined(XENON)
	if (leaderboardId == eLB_KillDeathRatio)
		result.Format("%.2f", CPersistantStats::ConvertToFloatFromLeaderboardScore(score));
	else if (leaderboardId == eLB_Accuracy)
		result.Format("%.2f %%", CPersistantStats::ConvertToFloatFromLeaderboardScore(score));
	else
#endif
		result.Format("%d", score);

	return result.c_str();
}

#if defined(XENON)
//------------------------------------------------------------------------
void CUILeaderboards::SendLeaderboardChangedToFlash(IFlashPlayer *pFlashPlayer, ELeaderboards leaderboardId)
{
	TFixedString32 LeaderboardName, ScoreName, Column0Name, Column1Name, Column2Name, Column3Name;
	TFixedString8	NumColumns = "0";

	switch (leaderboardId)
	{
	case eLB_XP:
			LeaderboardName = "@ui_menu_leaderboard_title_xp";
			ScoreName = "@ui_menu_leaderboard_heading_xp";

			NumColumns = "4";

			Column0Name = "@ui_menu_leaderboard_heading_armorxp";
			Column1Name = "@ui_menu_leaderboard_heading_powerxp";
			Column2Name = "@ui_menu_leaderboard_heading_stealthxp";
			Column3Name = "@ui_menu_leaderboard_heading_tacticalxp";
		break;
	case eLB_Kills:
			LeaderboardName = "@ui_menu_leaderboard_title_kills";
			ScoreName = "@ui_menu_leaderboard_heading_kills";
		break;
	case eLB_KillDeathRatio:
			LeaderboardName = "@ui_menu_leaderboard_title_kdratio";
			ScoreName = "@ui_menu_leaderboard_heading_kdratio";
		break;
	case eLB_Accuracy:
			LeaderboardName = "@ui_menu_leaderboard_title_accuracy";
			ScoreName = "@ui_menu_leaderboard_heading_accuracy";
		break;
	case eLB_KillStreak:
			LeaderboardName = "@ui_menu_leaderboard_title_killstreak";
			ScoreName = "@ui_menu_leaderboard_heading_kills";
		break;
	default:
		break;
	};

	SFlashVarValue args[7] = { LeaderboardName.c_str(), NumColumns.c_str(), ScoreName.c_str(), Column0Name.c_str(), Column1Name.c_str(), Column2Name.c_str(), Column3Name.c_str() };
	pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("SwitchLeaderboard"), args, 7);
}
#endif

//------------------------------------------------------------------------
void CUILeaderboards::ProcessLeaderboardRead(SCryStatsLeaderBoardReadResult* pResult, ECryLobbyError error)
{
	UpdateStatusFlash(m_pFlashPlayer);

	if (error == eCLE_Success)
	{
		SendLeaderboardToFlash(m_pFlashPlayer, pResult);
	}
	else
	{
		CHUD* pHUD = g_pGame->GetHUD();
		CMPMenuHub* pMPMenu = g_pGame->GetFlashMenu() ? g_pGame->GetFlashMenu()->GetMPMenu() : NULL;
		if (pHUD && pMPMenu)
		{
			TFixedString32 errorMsg;
			errorMsg.Format("@ui_menu_lobby_error_%d", (int)error);
			errorMsg = pHUD->LocalizeString(errorMsg.c_str());
			pMPMenu->ShowErrorDialog("LeaderboardError", errorMsg.c_str());
		}

		GameWarning("Leaderboard read failed error:%d", (int)error);
	}
}

//static---------------------------------------
void CUILeaderboards::ReadLeaderboardCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryStatsLeaderBoardReadResult* pResult, void* pArg)
{
	CUILeaderboards *pUILeaderboards = static_cast<CUILeaderboards*>(pArg);
	if (pUILeaderboards->GetCurrentTask() == taskID)
	{
		pUILeaderboards->SetCurrentTask(CryLobbyInvalidTaskID);
		pUILeaderboards->ProcessLeaderboardRead(pResult, error);

#ifndef _RELEASE
		if (g_debugLeaderboardsUI>0)
		{
			CryLogAlways("ReadLeaderboardCallback: task returned: %d error: %d", (int)taskID, (int)error);

			if (error == eCLE_Success)
			{
				// Print all
				CryLogAlways("Stats Read %d rows total board rows %d, id %d", pResult->numRows, pResult->totalNumBoardRows, pResult->id);
				for (uint32 i = 0; i < pResult->numRows; i++)
				{
					CryLogAlways("Rank %d Name %s score %llu", pResult->pRows[i].rank, pResult->pRows[i].name, pResult->pRows[i].data.score.score);
					if(pResult->pRows[i].data.numColumns > 0)
					{
						for(int j = 0; j < pResult->pRows[i].data.numColumns; j++)
						{
							CryLogAlways("\tColumn Data %d - %llu", pResult->pRows[i].data.pColumns[j].columnID, pResult->pRows[i].data.pColumns[j].data.m_int64);
						}
					}
				}
			}
		}
#endif
	}
	else
	{
#ifndef _RELEASE
		if (g_debugLeaderboardsUI>0)
		{
			CryLogAlways("ReadLeaderboardCallback: old task returned: %d error: %d", (int)taskID, (int)error);
		}
#endif
	}
}


#ifndef _RELEASE
//------------------------------------------------------------------------
void CUILeaderboards::Update(float dt)
{
	if (g_debugLeaderboardsUI>0)
	{
		CryWatch("Leaderboard: %d, startidx: %d, numentries: %d,  type:%d", m_currentLeaderboardIdx, m_startIndex, m_numEntries, m_currentLeaderboardRequestType);
		CryWatch("		current task: %d ", (int)m_currentTask);
	}
}

//------------------------------------------------------------------------
void CUILeaderboards::ReadFromLeaderboards()
{
	CryLogAlways("ReadFromLeaderboards");

#if defined(XENON)
	ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
	if(stats)
	{
		for (uint32 i = 0; i < eLB_Num; i++)
		{
			uint32 leaderboardId = CPersistantStats::GetLeaderboardIdFromIndex(i);
			ECryLobbyError error = stats->StatsReadLeaderBoardByRankForUser(leaderboardId, 0, 10, NULL, ReadLeaderboardCallback, this);
			CRY_ASSERT(error == eCLE_Success);
		}
	}
#endif
}
#endif