/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Objectives module that uses a lua table
	-------------------------------------------------------------------------
	History:
	- 25:11:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_LUA_OBJECTIVES_H_
#define _GAME_RULES_LUA_OBJECTIVES_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesLuaListener.h"
#include "IGameRulesObjectivesModule.h"

class CGameRulesLuaObjectives : public CGameRulesLuaListener,
								public IGameRulesObjectivesModule
{
public:
	CGameRulesLuaObjectives();
	virtual ~CGameRulesLuaObjectives();

	// IGameRulesObjectivesModule
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();
	virtual void OnGameReset();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual bool HasCompleted(int teamId);

	virtual void OnHostMigration(bool becomeServer);		// Host migration
	// ~IGameRulesObjectivesModule

protected:
	HSCRIPTFUNCTION m_funcUpdate;
	HSCRIPTFUNCTION m_funcOnStartGame;
	HSCRIPTFUNCTION m_funcOnGameReset;
	HSCRIPTFUNCTION m_funcHasCompleted;
	HSCRIPTFUNCTION m_funcOnHostMigration;
};

#endif // _GAME_RULES_LUA_OBJECTIVES_H_
