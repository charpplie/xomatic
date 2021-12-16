/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for the game rule module to handle objectives
	-------------------------------------------------------------------------
	History:
	- 21:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _IGAME_RULES_OBJECTIVES_MODULE_H_
#define _IGAME_RULES_OBJECTIVES_MODULE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameRulesSystem.h>
#include "GameRulesTypes.h"
#include "SerializeFwd.h"

class IGameRulesObjectivesModule
{
public:
	virtual ~IGameRulesObjectivesModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void Update(float frameTime) = 0;

	virtual void OnStartGame() = 0;
	virtual void OnGameReset() = 0;

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;

	virtual bool HasCompleted(int teamId) = 0;

	virtual void OnHostMigration(bool becomeServer) = 0;		// Host migration
};

#endif // _IGAME_RULES_OBJECTIVES_MODULE_H_