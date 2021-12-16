/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle victory upon being the last player left
	-------------------------------------------------------------------------
	History:
	- 06:11:2009  : Created by Thomas Houghton

*************************************************************************/

#ifndef _GAME_RULES_SURVIVOR_ONE_CONDITIONS_PLAYER_H_
#define _GAME_RULES_SURVIVOR_ONE_CONDITIONS_PLAYER_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesStandardVictoryConditionsPlayer.h"

class CGameRules;

class CGameRulesSurvivorOneVictoryConditions :	public CGameRulesStandardVictoryConditionsPlayer
{
private:
	typedef CGameRulesStandardVictoryConditionsPlayer  inherited;

protected:
	int  m_svLatestSurvCount;
	EntityId  m_svLatestSurvList[16];

public:

public:
	CGameRulesSurvivorOneVictoryConditions();
	~CGameRulesSurvivorOneVictoryConditions();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	// IGameRulesVictoryConditionsModule
	virtual void	ClVictoryTeam(int teamId, EGameOverReason reason) {};
	virtual void	ClVictoryPlayer(int playerId, EGameOverReason reason);
	// ~IGameRulesVictoryConditionsModule

	// IGameRulesSurvivorCountListener
	virtual void SvSurvivorCountRefresh(int count, const EntityId survivors[], int numKills);
	// ~IGameRulesSurvivorCountListener

protected:
	virtual void OnEndGamePlayer(EntityId playerId, EGameOverReason reason);

	// CGameRulesStandardVictoryConditionsPlayer
	virtual void TimeLimitExpired();
	// ~CGameRulesStandardVictoryConditionsPlayer

};

#endif // _GAME_RULES_SURVIVOR_ONE_CONDITIONS_PLAYER_H_