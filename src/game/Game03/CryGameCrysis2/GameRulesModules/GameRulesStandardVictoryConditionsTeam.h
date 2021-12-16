/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Standard game rules module to handle victory conditions
	-------------------------------------------------------------------------
	History:
	- 08:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef _GAME_RULES_STANDARD_VICTORY_CONDITIONS_TEAM_H_
#define _GAME_RULES_STANDARD_VICTORY_CONDITIONS_TEAM_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesVictoryConditionsModule.h"
#include "GameRulesModules/IGameRulesSurvivorCountListener.h"

class CGameRules;

class CGameRulesStandardVictoryConditionsTeam : public IGameRulesVictoryConditionsModule,
												public IGameRulesSurvivorCountListener
{
public:
	CGameRulesStandardVictoryConditionsTeam();
	virtual ~CGameRulesStandardVictoryConditionsTeam();

	// IGameRulesVictoryConditionsModule
	virtual void	Init(XmlNodeRef xml);
	virtual void	Update(float frameTime);

	virtual void OnStartGame();

	virtual void	ClVictoryTeam(int teamId, EGameOverReason reason);
	virtual void	ClVictoryPlayer(int playerId, EGameOverReason reason) {};
	// ~IGameRulesVictoryConditionsModule

	// IGameRulesSurvivorCountListener
	virtual void SvSurvivorCountRefresh(int count, const EntityId survivors[], int numKills);
	// ~IGameRulesSurvivorCountListener

protected:

	struct STeamScoreResult
	{
		STeamScoreResult() : score(0), scoreTeamId(0) { };

		int score;
		int scoreTeamId;
	};

	void	GetMaxTeamScore(STeamScoreResult &result);
	void	GetMinTeamScore(STeamScoreResult &result);

	virtual void	CheckTimeLimit();
	virtual void  CheckScoreLimit();
	virtual void  CheckScoreAsTimeLimit();
	void	UpdateTimeLimitSounds();
	void	OnEndGame(int teamId, EGameOverReason reason);
	virtual void TimeLimitExpired();

	enum ESVC_Sounds
	{
		eSVC_1min = 0,
		eSVC_15secs,
		eSVC_5secs,
		eSVC_3secs,
		eSVC_end,
	};

	typedef CryFixedStringT<32> TFixedString; 
	TFixedString m_winString;
	TFixedString m_loseString;
	TFixedString m_drawString;

	CGameRules *m_pGameRules;

	ESVC_Sounds m_currentSound;

	bool  m_checkScore;
	bool  m_checkTime;
	bool	m_checkScoreAsTime;
	bool  m_winOnPrimaryTeamDead;
	bool  m_winOnSecondaryTeamDead;
	bool	m_decideRoundWinnerUsingRoundPoints;
};

#endif // _GAME_RULES_STANDARD_VICTORY_CONDITIONS_TEAM_H_