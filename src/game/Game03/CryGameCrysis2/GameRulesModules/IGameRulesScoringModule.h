/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for the game rule module to handle scoring points
	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef _GAME_RULES_SCORING_MODULE_H_
#define _GAME_RULES_SCORING_MODULE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameRulesSystem.h>
#include "GameRulesTypes.h"

class IGameRulesScoringModule
{
public:
	virtual ~IGameRulesScoringModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;

	virtual TGameRulesScoreInt GetPlayerPointsByType(EGRST pointsType) const = 0;
	virtual TGameRulesScoreInt GetTeamPointsByType(EGRST pointsType) const = 0;
	virtual void	DoScoringForDeath(IActor *pActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type) = 0;
	virtual void	OnPlayerScoringEvent(EntityId playerId, EGRST type) = 0;
	virtual void	OnTeamScoringEvent(int teamId, EGRST pointsType) = 0;
	virtual int		GetStartTeamScore() = 0;
	virtual int		GetMaxTeamScore() = 0;
};

#endif // _GAME_RULES_SCORING_MODULE_H_
