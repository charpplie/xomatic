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
#include "StdAfx.h"
#include "CryLobbySessionHandler.h"
#include "GameLobby.h"

//-------------------------------------------------------------------------
CCryLobbySessionHandler::CCryLobbySessionHandler()
{
	g_pGame->GetIGameFramework()->SetGameSessionHandler(this);
	m_gameActive = false;
}

//-------------------------------------------------------------------------
CCryLobbySessionHandler::~CCryLobbySessionHandler()
{
	CRY_ASSERT(!m_gameActive);
	g_pGame->ClearGameSessionHandler(); // Must clear pointer in game if cry action deletes the handler.
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::CreateSession(const SGameStartParams * pGameStartParams)
{
	if( !(pGameStartParams->flags & eGSF_LocalOnly) && pGameStartParams->flags & eGSF_Server )
	{
		// Calls StartGameContext if successful
		CGameLobby* pGameLobby = g_pGame->GetGameLobby();
		if (pGameLobby)
			pGameLobby->CreateSession(pGameStartParams);
	}
	else
	{
		g_pGame->GetIGameFramework()->StartGameContext(pGameStartParams);
	}
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::LeaveSession()
{
	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
		pGameLobby->DeleteSession();
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::StartSession()
{
	ICryMatchMaking* pMatchmaking = gEnv->pNetwork->GetLobby()->GetMatchMaking();
	if (pMatchmaking)
	{
		ECryLobbyError error = pMatchmaking->SessionStart(g_pGame->GetGameLobby()->GetSessionId(), 0, SessionStartCallback, this);
		CRY_ASSERT(error == eCLE_Success);
	}
}

//-------------------------------------------------------------------------
void CCryLobbySessionHandler::EndSession()
{
	ICryMatchMaking* pMatchmaking = gEnv->pNetwork->GetLobby()->GetMatchMaking();
	if (pMatchmaking)
	{
		ECryLobbyError error = pMatchmaking->SessionEnd(g_pGame->GetGameLobby()->GetSessionId(), 0, SessionEndCallback, this);
		CRY_ASSERT(error == eCLE_Success);
	}
}


//static-------------------------------------------------------------------------
void CCryLobbySessionHandler::SessionStartCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	CCryLobbySessionHandler* pThis = static_cast<CCryLobbySessionHandler*>(pArg);
	if(error == eCLE_Success)
	{
		pThis->m_gameActive = true;
	}
}

//static-------------------------------------------------------------------------
void CCryLobbySessionHandler::SessionEndCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	CCryLobbySessionHandler* pThis = static_cast<CCryLobbySessionHandler*>(pArg);
	if(error == eCLE_Success)
	{
		pThis->m_gameActive = false;

		CRY_TODO(04, 03, 2010, "Session End Should be called between games and return to the lobby");
		pThis->LeaveSession();
	}
}
