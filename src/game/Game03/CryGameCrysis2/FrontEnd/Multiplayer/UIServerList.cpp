/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: UI Server List

-------------------------------------------------------------------------
History:
- 02/2010: Created by Ben Johnson

*************************************************************************/
#include "StdAfx.h"
#include "UIServerList.h"
#include "Flash/Flash.h"
#include "GameCVars.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"
#include "FrontEnd/FlashFrontEnd.h"

//------------------------------------------------------------------------
CUIServerList::CUIServerList()
{
	m_startIndex = 0;
	m_numEntries = 0;
}

//------------------------------------------------------------------------
CUIServerList::~CUIServerList()
{
}

//------------------------------------------------------------------------
const CUIServerList::SServerInfo* CUIServerList::GetServerInfo(const int index) const
{
	CRY_ASSERT(index>=0 && index<m_allServers.size());
	return &m_allServers[index];
}

//------------------------------------------------------------------------
void CUIServerList::ClearServerList(IFlashPlayer *pFlashPlayer)
{
	m_allServers.clear();

	m_startIndex = 0;

	pFlashPlayer->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("ClearServerList"));

	const int num = m_allServers.size();
	SFlashVarValue arg = num;
	pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("SetNumTotalServers"), arg);
}

//------------------------------------------------------------------------
void CUIServerList::AddServer(const CUIServerList::SServerInfo &serverInfo, IFlashPlayer *pFlashPlayer)
{
	int idx = m_allServers.size();

	m_allServers.push_back(serverInfo);

	const int num = m_allServers.size();
	SFlashVarValue arg = num;
	pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("SetNumTotalServers"), arg);

#ifndef _RELEASE
	const char * mustContainString = g_pGameCVars->net_onlyListGameServersContainingText->GetString();

	if (mustContainString && mustContainString[0])
	{
		bool ignoreIt = strstr (serverInfo.m_hostName, mustContainString) == NULL;
		CryLogAlways ("Will only list a server called '%s' if it contains '%s'... and it %s", serverInfo.m_hostName.c_str(), mustContainString, ignoreIt ? "doesn't! Ignoring it!": "does! Adding it to the list.");
		if (ignoreIt)
		{
			return;
		}
	}
#endif

	if (idx - m_startIndex >= m_numEntries)
	{
		return;
	}

	CCCPOINT(GameLobby_ShowJoinableGame);
	AddServerToFlash(idx, serverInfo, pFlashPlayer);
}

void CUIServerList::AddServerToFlash(int idx, const CUIServerList::SServerInfo &serverInfo, IFlashPlayer *pFlashPlayer)
{
	char convert[32]      = "\0";
	char uiNumPlayers[32] = "\0";
	itoa(serverInfo.m_numPlayers, convert, 10);
	if (serverInfo.m_numPlayers < 10)
	{
		convert[1] = convert[0];
		convert[0] = ' ';
	}
	convert[2] = '\0';

	strcpy(uiNumPlayers, convert);
	strcat(uiNumPlayers, " / \0");

	itoa(serverInfo.m_maxPlayers, convert, 10);
	if (serverInfo.m_maxPlayers < 10)
	{
		convert[1] = ' ';
	}
	convert[2] = '\0';

	strcat(uiNumPlayers, convert);

	SFlashVarValue args[] = {idx, serverInfo.m_serverId, serverInfo.m_hostName.c_str(), serverInfo.m_ping, uiNumPlayers, serverInfo.m_mapName.c_str(), serverInfo.m_gameTypeName.c_str()};
	pFlashPlayer->Invoke(FRONTEND_SUBSCREEN_PATH_SET("AddServer"), args, sizeof(args)/sizeof(args[0]));
}

void CUIServerList::RefreshServerList(IFlashPlayer *pFlashPlayer)
{
	CryLog("RefreshServerList");

	pFlashPlayer->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("ClearServerList"));

	const int num = m_allServers.size();
	SFlashVarValue arg = num;
	pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("SetNumTotalServers"), arg);

	for (int i=m_startIndex; ((i<(m_startIndex+m_numEntries)) && (i<m_allServers.size())); ++i)
	{
		CUIServerList::SServerInfo &serverInfo = m_allServers[i];
		AddServerToFlash(i, serverInfo, pFlashPlayer);
	}

}