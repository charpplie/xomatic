/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Multiplayer UI Handling

-------------------------------------------------------------------------
History:
- 02/2010: Created by Ben Johnson

*************************************************************************/
#ifndef __MP_MENU_HUB_H__
#define __MP_MENU_HUB_H__

#pragma once

#include "UIServerList.h"
#include "FrontEnd/WarningsManager.h"
#include "FrontEnd/MenuData.h"
#include "FrontEnd/FlashFrontEnd.h"

struct IFlashPlayer;

class CUILeaderboards;

class CMPMenuHub : IHUDWarningsListener
{
public:

	enum EMPMenuCommand
	{
		eMPFEC_none,
		eMPFEC_mp_load,
		eMPFEC_back_to_mainmenu,
		eMPFEC_set_lobby_type,
		eMPFEC_serverlist_scrollUp,
		eMPFEC_serverlist_scrollDown,
		eMPFEC_serverlist_join,
		eMPFEC_set_serverlist_size,
		eMPFEC_leaderboard_move,
		eMPFEC_leaderboard_change,
		eMPFEC_leaderboard_set_size,
		eMPFEC_leaderboard_move_to_user,
		eMPFEC_leaderboard_move_to_top,
	};

public:
	CMPMenuHub();
	~CMPMenuHub();

	// IHUDWarningsListener
	virtual bool OnWarningReturn(THUDWarningId id, const char* returnValue);
	virtual void OnWarningRemoved(THUDWarningId id);
	// ~IHUDWarningsListener

	void InitFrontEndMenu();
	void DestroyFrontEndMenu();
	void InitIngameMenu();
	void DestroyIngameMenu();

	bool HandleCommand(const char* pCommand, const char* pArgs, void* pUserData);
	bool HandleCommand(EMPMenuCommand command, const char* pArgs, void* pUserData);

	void AddServer(const CUIServerList::SServerInfo &serverInfo);

	void EnterScreen(const CFlashFrontEnd::EFlashMenus menu, const EFlashFrontEndScreens screen);
	void LeaveScreen(const CFlashFrontEnd::EFlashMenus menu, const EFlashFrontEndScreens screen);

	void Update(float frameTime);

	void StartSearching();
	void SearchComplete();
  void StopServerListUpdate();

	void ShowLoadingDialog(const char *dialogName, const char *param = NULL);
	void HideLoadingDialog(const char *dialogName);
	void ShowErrorDialog(const char *dialogName, const char *param = NULL);

	void Disconnected(int value);
	void ConnectionFailed();

	void UpdateNatType(const char* natType);

	void SetFlashPlayer(IFlashPlayer *player) { m_pFlashUI = player; }

private:
  void JoinServer(int serverIdx);


	void RegisterCommand(EMPMenuCommand command, const char *name);
	const EMPMenuCommand GetCommand(const char *name) const;

	typedef std::map<uint32, EMPMenuCommand> TMPCommandMap;

	TMPCommandMap m_commands;
	EFlashFrontEndScreens m_currentScreen;

	// FrontEnd Only Classes
	CUIServerList* m_pServerList;
	CUILeaderboards* m_pLeaderboards;

	bool m_bLoadingDialog;

	IFlashPlayer *m_pFlashUI;
};
#endif /*__MP_MENU_HUB_H__*/
