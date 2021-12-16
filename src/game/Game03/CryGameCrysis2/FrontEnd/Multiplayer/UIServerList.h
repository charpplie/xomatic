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
#ifndef __UI_SERVER_LIST_H__
#define __UI_SERVER_LIST_H__

#pragma once

struct IFlashPlayer;

class CUIServerList
{
public:
	typedef CryFixedStringT<32> TFixedString32;

	struct SServerInfo 
	{
		SServerInfo():
		m_numPlayers(0),
		m_maxPlayers(0),
		m_ping(-1),
		m_serverId(-1)
	{
	}

	TFixedString32    m_hostName;
	TFixedString32    m_mapName;
	TFixedString32		m_gameTypeName;
	TFixedString32    m_gameVersion;

	int				m_numPlayers;
	int				m_maxPlayers;
	int       m_ping;

	int       m_serverId;
	CrySessionID  m_sessionId;
	};

public:
	CUIServerList();
	~CUIServerList();

	const CUIServerList::SServerInfo* GetServerInfo(const int index) const;

	void AddServer(const CUIServerList::SServerInfo &serverInfo, IFlashPlayer *pFlashPlayer);

  void ClearServerList(IFlashPlayer *pFlashPlayer);
  void StopServerListUpdate();
  void JoinServer(int serverIdx);
	void RefreshServerList(IFlashPlayer *pFlashPlayer);

	const int GetTotalServers() const { return m_allServers.size(); }
	const int GetStartIndex() const { return m_startIndex; }
	const int GetNumEntries() const { return m_numEntries; }
	void SetStartIndex(const int startIndex) { m_startIndex = startIndex; }
	void SetNumEntries(const int numEntries) { m_numEntries = numEntries; }

private:
	void AddServerToFlash(const int idx, const CUIServerList::SServerInfo &serverInfo, IFlashPlayer *pFlashPlayer);

	typedef std::vector<CUIServerList::SServerInfo> ServerInfoVector;
	typedef std::vector<int> DisplayedServersVector;

	CRY_TODO(10, 02, 2010, "Look at the posibility of changing m_allServers to be a fixed array instead of a vector which may cause fragmentation.");
	ServerInfoVector       m_allServers;
	DisplayedServersVector m_all;

	int m_startIndex;
	int m_numEntries;
};
#endif /*__UI_SERVER_LIST_H__*/
