/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GameRulesStandardState_h_
#define _GameRulesStandardState_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesStateModule.h"

class CGameRules;

class CGameRulesStandardState : public IGameRulesStateModule
{
public:
	CGameRulesStandardState();
	virtual ~CGameRulesStandardState();

	// IGameRulesStateModule
	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();

	virtual void OnGameRestart();
	virtual void OnGameReset();
	virtual void OnGameEnd();

	virtual EGR_GameState GetGameState();

	virtual void Update(float frameTime);

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	// ~IGameRulesStateModule

protected:
	static void CmdSetState(IConsoleCmdArgs *pArgs);

	bool HasRequiredPlayers() const;
	void ChangeState(EGR_GameState newState);

	CGameRules *m_pGameRules;

	float m_scoreboardTime;
	float m_nextLevelTime;
	float m_minPlayerWarningTime;

	EGR_GameState m_state;

	bool m_isStarting;
	bool m_forceInGame;
	bool m_scoreboardShown;
	bool m_introMessageShown;
};

#endif // _GameRulesStandardState_h_