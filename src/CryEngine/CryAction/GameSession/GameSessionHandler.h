/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Default session handler implementation.

-------------------------------------------------------------------------
History:
- 08:12:2009 : Created By Ben Johnson

*************************************************************************/
#ifndef __GAME_SESSION_HANDLER_H__
#define __GAME_SESSION_HANDLER_H__

#include <IGameSessionHandler.h>

class CGameSessionHandler : public IGameSessionHandler
{
public:
	CGameSessionHandler();
	virtual ~CGameSessionHandler();

	// IGameSessionHandler
	VIRTUAL void CreateSession(const SGameStartParams * pGameStartParams);
	VIRTUAL void EndSession();
	
	VIRTUAL void StartSession();
	VIRTUAL void LeaveSession();
	// ~IGameSessionHandler
};

#endif //__GAME_SESSION_HANDLER_H__