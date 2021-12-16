/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of an entity objective that uses a lua table
	-------------------------------------------------------------------------
	History:
	- 25:11:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_LUA_ENTITY_OBJECTIVE_H_
#define _GAME_RULES_LUA_ENTITY_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesLuaListener.h"
#include "IGameRulesEntityObjective.h"

class CGameRulesLuaEntityObjective :	public CGameRulesLuaListener,
										public IGameRulesEntityObjective
{
public:
	CGameRulesLuaEntityObjective();
	~CGameRulesLuaEntityObjective();

	// IGameRulesEntityObjective
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void EnableCompletion(int teamId, bool enable);
	virtual void EnableScoring(int teamId, bool enable);

	virtual bool IsComplete(int teamId);

	virtual void AddEntityId(int type, EntityId entityId, int index, bool isNewEntity);
	virtual void RemoveEntityId(int type, EntityId entityId);
	virtual void ClearEntities(int type);
	virtual bool IsEntityFinished(int type, int index);
	virtual bool CanRemoveEntity(int type, int index);
	virtual void SetWaveNumber(int num, int waveCount);

	virtual void OnHostMigration(bool becomeServer);
	// ~IGameRulesEntityObjective

protected:
	HSCRIPTFUNCTION m_funcUpdate;
	HSCRIPTFUNCTION m_funcOnStartGame;
	HSCRIPTFUNCTION m_funcEnableCompletion;
	HSCRIPTFUNCTION m_funcEnableScoring;
	HSCRIPTFUNCTION m_funcIsComplete;
	HSCRIPTFUNCTION m_funcAddEntityId;
	HSCRIPTFUNCTION m_funcRemoveEntityId;
	HSCRIPTFUNCTION m_funcClearEntities;
	HSCRIPTFUNCTION m_funcIsEntityFinished;
	HSCRIPTFUNCTION m_funcCanRemoveEntity;
	HSCRIPTFUNCTION m_funcSetWaveNumber;
	HSCRIPTFUNCTION m_funcOnHostMigration;
};

#endif // _GAME_RULES_LUA_ENTITY_OBJECTIVE_H_
