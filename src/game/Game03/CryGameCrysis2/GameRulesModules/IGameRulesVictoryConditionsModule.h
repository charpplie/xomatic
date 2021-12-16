/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for the game rule module to handle victory conditions
	-------------------------------------------------------------------------
	History:
	- 08:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef _IGAME_RULES_VICTORY_CONDITIONS_MODULE_H_
#define _IGAME_RULES_VICTORY_CONDITIONS_MODULE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameRulesSystem.h>
#include "GameRulesTypes.h"

class IGameRulesVictoryConditionsModule
{
public:
	virtual ~IGameRulesVictoryConditionsModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;
	virtual void	Update(float frameTime) = 0;

	virtual void OnStartGame() = 0;
	// TODO: Need a way to let the victory have different parameters.
	virtual void	ClVictoryTeam(int teamId, EGameOverReason reason) = 0;
	virtual void	ClVictoryPlayer(int playerId, EGameOverReason reason) = 0;
};

#endif // _IGAME_RULES_VICTORY_CONDITIONS_MODULE_H_