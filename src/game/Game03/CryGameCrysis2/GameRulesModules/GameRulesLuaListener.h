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

#ifndef _GAME_RULES_LUA_LISTENER_H_
#define _GAME_RULES_LUA_LISTENER_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesPickupListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesRevivedListener.h"
#include "GameRulesModules/IGameRulesSurvivorCountListener.h"

class CGameRulesLuaListener:	public IGameRulesClientConnectionListener,
								public IGameRulesKillListener,
								public IGameRulesPickupListener,
								public IGameRulesTeamChangedListener,
								public IGameRulesRevivedListener,
								public IGameRulesSurvivorCountListener
{
public:
	CGameRulesLuaListener();
	~CGameRulesLuaListener();

	void InitLuaTable(IScriptTable *pTable);

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId);
	virtual void OnClientDisconnect(int channelId, EntityId playerId);
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId);
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesPickupListener
	virtual void OnItemPickedUp(EntityId itemId, EntityId actorId);
	virtual void OnItemDropped(EntityId itemId, EntityId actorId) {};
	virtual void OnPickupEntityAttached(EntityId entityId, EntityId actorId) {};
	virtual void OnPickupEntityDetached(EntityId entityId, EntityId actorId, bool isOnRemove) {};
	// ~IGameRulesPickupListener

	// IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	// ~IGameRulesKillListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// IGameRulesRevivedListener
	virtual void EntityRevived(EntityId entityId);
	// ~IGameRulesRevivedListener

	// IGameRulesSurvivorCountListener
	virtual void SvSurvivorCountRefresh(int count, const EntityId survivors[], int numKills);
	// ~IGameRulesSurvivorCountListener

protected:
	SmartScriptTable m_objectivesScript;
	SmartScriptTable m_scriptHitInfo;
	SmartScriptTable m_survivorsTable;

	HSCRIPTFUNCTION m_funcOnClientConnect;
	HSCRIPTFUNCTION m_funcOnClientDisconnect;
	HSCRIPTFUNCTION m_funcOnClientEnteredGame;
	HSCRIPTFUNCTION m_funcOnOwnClientEnteredGame;

	HSCRIPTFUNCTION m_funcOnEntityKilled;

	HSCRIPTFUNCTION m_funcOnItemPickedUp;

	HSCRIPTFUNCTION m_funcOnChangedTeam;

	HSCRIPTFUNCTION m_funcOnEntityRevived;

	HSCRIPTFUNCTION m_funcOnSvSurvivorCountRefresh;

	IScriptSystem *m_pScriptSystem;
};

#endif // _GAME_RULES_LUA_LISTENER_H_
