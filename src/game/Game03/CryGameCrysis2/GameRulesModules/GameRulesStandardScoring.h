/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle scoring points values
	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef _GAME_RULES_STANDARD_SCORING_H_
#define _GAME_RULES_STANDARD_SCORING_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesScoringModule.h"

class CGameRules;

class CGameRulesStandardScoring : public IGameRulesScoringModule
{
public:
	CGameRulesStandardScoring();
	virtual ~CGameRulesStandardScoring();

	// IGameRulesScoringModule
	virtual void	Init(XmlNodeRef xml);

	virtual TGameRulesScoreInt GetPlayerPointsByType(EGRST type) const;
	virtual TGameRulesScoreInt GetTeamPointsByType(EGRST type) const;
	virtual void	DoScoringForDeath(IActor *pActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type);
	virtual void	OnPlayerScoringEvent(EntityId playerId, EGRST type);
	virtual void	OnTeamScoringEvent(int teamId, EGRST pointsType);
	virtual int		GetStartTeamScore() { return m_startTeamScore; }
	virtual int		GetMaxTeamScore() { return m_maxTeamScore; }
	// ~IGameRulesScoringModule

protected:
	void	InitScoreData(XmlNodeRef xml, TGameRulesScoreInt *scoringData);
	TGameRulesScoreInt GetPointsByType(const TGameRulesScoreInt *scoringData, EGRST type) const;
	bool	ShouldScore(CGameRules* pGameRules) const;

	TGameRulesScoreInt m_playerScorePoints[EGRST_Num];
	TGameRulesScoreInt m_teamScorePoints[EGRST_Num];

	int m_maxTeamScore;
	int m_startTeamScore;
	bool m_useScoreAsTime; // Calculate score using time.
};

#endif // _GAME_RULES_STANDARD_SCORING_H_
