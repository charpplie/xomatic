/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Interface to handle sessions.

-------------------------------------------------------------------------
History:
- 08:12:2009 : Created By Ben Johnson

*************************************************************************/

#ifndef __I_GAME_SESSION_HANDLER_H__
#define __I_GAME_SESSION_HANDLER_H__

struct SGameStartParams;

struct IGameSessionHandler
{
public:
	virtual ~IGameSessionHandler() {}

	virtual void CreateSession(const SGameStartParams * pGameStartParams) = 0;
	virtual void EndSession() = 0;
	
	virtual void StartSession() = 0;
	virtual void LeaveSession() = 0;
};

#endif //__I_GAME_SESSION_HANDLER_H__
