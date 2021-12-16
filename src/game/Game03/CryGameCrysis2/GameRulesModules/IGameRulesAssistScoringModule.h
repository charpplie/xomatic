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
	- 14:09:2009  : Created by James Bamford

*************************************************************************/

#ifndef _GAME_RULES_ASSIST_SCORING_MODULE_H_
#define _GAME_RULES_ASSIST_SCORING_MODULE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameRulesSystem.h>

class IGameRulesAssistScoringModule
{
public:
	virtual ~IGameRulesAssistScoringModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;

	virtual void	SvPlayerJoined(EntityId playerId) = 0;
	virtual void	SvPlayerLeft(EntityId playerId) = 0;
	virtual void  SvOnPlayerHit(const HitInfo &info) = 0;
//virtual void  ClAwardAssistKillPoints(EntityId victimId) = 0;
	virtual void	SvDoScoringForDeath(IActor *pActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type) = 0;
	virtual EntityId	SvGetMostRecentAttacker(EntityId targetId) = 0;

	virtual void  SvBecomeServer() = 0;		// Host migration
};

#endif // _GAME_RULES_ASSIST_SCORING_MODULE_H_
