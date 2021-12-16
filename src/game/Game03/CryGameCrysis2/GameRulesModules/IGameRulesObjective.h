/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for a player objective
	-------------------------------------------------------------------------
	History:
	- 21:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _IGAME_RULES_OBJECTIVE_H_
#define _IGAME_RULES_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameObject.h"
#include "SerializeFwd.h"

class IGameRulesObjective
{
public:
	virtual ~IGameRulesObjective() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void Update(float frameTime) = 0;

	virtual void OnStartGame() = 0;

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;

	virtual void Enable(int teamId, bool enable) = 0;
	virtual bool IsComplete(int teamId) = 0;

	virtual void OnHostMigration(bool becomeServer) = 0;
};

#endif // _IGAME_RULES_OBJECTIVE_H_
