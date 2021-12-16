/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: CryLobby session handler implementation.

-------------------------------------------------------------------------
History:
- 08:12:2009 : Created By Ben Johnson

*************************************************************************/
#ifndef __CRYLOBBY_SESSION_HANDLER_H__
#define __CRYLOBBY_SESSION_HANDLER_H__

#include <IGameSessionHandler.h>
#include "ICryLobby.h"
#include "ICryMatchMaking.h"

class CCryLobbySessionHandler : public IGameSessionHandler
{
public:
	CCryLobbySessionHandler();
	virtual ~CCryLobbySessionHandler();

	// IGameSessionHandler
	virtual void CreateSession(const SGameStartParams * pGameStartParams);
	virtual void LeaveSession();
	
	virtual void StartSession();
	virtual void EndSession();
	// ~IGameSessionHandler

	//Callbacks
	static void SessionStartCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);
	static void SessionEndCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);

protected:
	bool m_gameActive;				//true between start and end session (during a game)
};

#endif //__CRYLOBBY_SESSION_HANDLER_H__