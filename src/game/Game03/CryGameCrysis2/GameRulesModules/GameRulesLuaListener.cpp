/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a class that implements the gamerules listener
		classes and passes the calls onto lua

	-------------------------------------------------------------------------
	History:
	- 25:11:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesLuaListener.h"
#include "GameRules.h"

//-------------------------------------------------------------------------
CGameRulesLuaListener::CGameRulesLuaListener()
{
	m_funcOnClientConnect = NULL;
	m_funcOnClientDisconnect = NULL;
	m_funcOnClientEnteredGame = NULL;
	m_funcOnOwnClientEnteredGame = NULL;

	m_funcOnEntityKilled = NULL;

	m_funcOnItemPickedUp = NULL;

	m_funcOnChangedTeam = NULL;

	m_pScriptSystem = NULL;

	m_funcOnEntityRevived = NULL;

	m_funcOnSvSurvivorCountRefresh = NULL;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	CRY_ASSERT(pGameRules);

	pGameRules->RegisterClientConnectionListener(this);
	pGameRules->RegisterKillListener(this);
	pGameRules->RegisterPickupListener(this);
	pGameRules->RegisterTeamChangedListener(this);
	pGameRules->RegisterRevivedListener(this);
	pGameRules->RegisterSurvivorCountListener(this);
}

//-------------------------------------------------------------------------
CGameRulesLuaListener::~CGameRulesLuaListener()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterClientConnectionListener(this);
		pGameRules->UnRegisterKillListener(this);
		pGameRules->UnRegisterPickupListener(this);
		pGameRules->UnRegisterTeamChangedListener(this);
		pGameRules->UnRegisterRevivedListener(this);
		pGameRules->UnRegisterSurvivorCountListener(this);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::InitLuaTable( IScriptTable *pTable )
{
	m_pScriptSystem = gEnv->pScriptSystem;

	m_objectivesScript = pTable;

	if (m_objectivesScript.GetPtr())
	{
		m_objectivesScript->GetValue("OnClientConnect", m_funcOnClientConnect);
		m_objectivesScript->GetValue("OnClientDisconnect", m_funcOnClientDisconnect);
		m_objectivesScript->GetValue("OnClientEnteredGame", m_funcOnClientEnteredGame);
		m_objectivesScript->GetValue("OnOwnClientEnteredGame", m_funcOnOwnClientEnteredGame);

		m_objectivesScript->GetValue("OnEntityKilled", m_funcOnEntityKilled);

		m_objectivesScript->GetValue("OnItemPickedUp", m_funcOnItemPickedUp);

		m_objectivesScript->GetValue("OnChangedTeam", m_funcOnChangedTeam);

		m_objectivesScript->GetValue("OnEntityRevived", m_funcOnEntityRevived);

		m_objectivesScript->GetValue("OnSvSurvivorCountRefresh", m_funcOnSvSurvivorCountRefresh);

		m_scriptHitInfo.Create(m_pScriptSystem);
		m_survivorsTable.Create(m_pScriptSystem);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnClientConnect( int channelId, bool isReset, EntityId playerId )
{
	if (m_objectivesScript.GetPtr() && m_funcOnClientConnect)
	{
		Script::Call(m_pScriptSystem, m_funcOnClientConnect, m_objectivesScript, channelId, isReset, ScriptHandle(playerId));
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnClientDisconnect( int channelId, EntityId playerId )
{
	if (m_objectivesScript.GetPtr() && m_funcOnClientDisconnect)
	{
		Script::Call(m_pScriptSystem, m_funcOnClientDisconnect, m_objectivesScript, channelId, ScriptHandle(playerId));
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnClientEnteredGame( int channelId, bool isReset, EntityId playerId )
{
	if (m_objectivesScript.GetPtr() && m_funcOnClientEnteredGame)
	{
		Script::Call(m_pScriptSystem, m_funcOnClientEnteredGame, m_objectivesScript, channelId, isReset, ScriptHandle(playerId));
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnOwnClientEnteredGame()
{
	if (m_objectivesScript.GetPtr() && m_funcOnOwnClientEnteredGame)
	{
		Script::Call(m_pScriptSystem, m_funcOnOwnClientEnteredGame, m_objectivesScript);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnEntityKilled( const HitInfo &hitInfo )
{
	if (m_objectivesScript.GetPtr() && m_funcOnEntityKilled)
	{
		g_pGame->GetGameRules()->CreateScriptHitInfo(m_scriptHitInfo, hitInfo);
		Script::Call(m_pScriptSystem, m_funcOnEntityKilled, m_objectivesScript, m_scriptHitInfo);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnItemPickedUp( EntityId itemId, EntityId actorId )
{
	if (m_objectivesScript.GetPtr() && m_funcOnItemPickedUp)
	{
		Script::Call(m_pScriptSystem, m_funcOnItemPickedUp, m_objectivesScript, ScriptHandle(itemId), ScriptHandle(actorId));
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if (m_objectivesScript.GetPtr() && m_funcOnChangedTeam)
	{
		Script::Call(m_pScriptSystem, m_funcOnChangedTeam, m_objectivesScript, ScriptHandle(entityId), oldTeamId, newTeamId);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::EntityRevived( EntityId entityId )
{
	if (m_objectivesScript.GetPtr() && m_funcOnEntityRevived)
	{
		Script::Call(m_pScriptSystem, m_funcOnEntityRevived, m_objectivesScript, ScriptHandle(entityId));
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaListener::SvSurvivorCountRefresh( int count, const EntityId survivors[], int numKills )
{
	if (m_objectivesScript.GetPtr() && m_funcOnSvSurvivorCountRefresh)
	{
		int existingEntriesCount = m_survivorsTable->Count();

		// Fill the table with EntityIds
		for (int i = 0; i < count; ++ i)
		{
			ScriptHandle entityHandle(survivors[i]);
			m_survivorsTable->SetAt(i + 1, entityHandle);
		}

		// If the table still has entries remaining from the previous time we used it, replace them with nulls
		for (int i = count; i < existingEntriesCount; ++ i)
		{
			m_survivorsTable->SetNullAt(i + 1);
		}

		Script::Call(m_pScriptSystem, m_funcOnSvSurvivorCountRefresh, m_objectivesScript, m_survivorsTable, numKills);
	}
}
