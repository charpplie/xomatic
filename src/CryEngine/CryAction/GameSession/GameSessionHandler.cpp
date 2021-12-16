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
#include "StdAfx.h"
#include "GameSessionHandler.h"
#include "CryAction.h"

//-------------------------------------------------------------------------
CGameSessionHandler::CGameSessionHandler()
{
}

//-------------------------------------------------------------------------
CGameSessionHandler::~CGameSessionHandler()
{
}

//-------------------------------------------------------------------------
void CGameSessionHandler::CreateSession(const SGameStartParams * pGameStartParams)
{
	CCryAction::GetCryAction()->StartGameContext(pGameStartParams);
}

//-------------------------------------------------------------------------
void CGameSessionHandler::EndSession()
{
}

//-------------------------------------------------------------------------
void CGameSessionHandler::StartSession()
{
}

//-------------------------------------------------------------------------
void CGameSessionHandler::LeaveSession()
{
}