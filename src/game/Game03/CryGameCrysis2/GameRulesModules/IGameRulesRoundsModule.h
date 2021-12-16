/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 26:10:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GameRulesRoundsModule_h_
#define _GameRulesRoundsModule_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "SerializeFwd.h"
#include "IGameObject.h"
#include "GameRulesTypes.h"

class IGameRulesRoundsModule
{
public:
	virtual ~IGameRulesRoundsModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;
	virtual void Update(float frameTime) = 0;

	virtual void OnStartGame() = 0;

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;

	virtual void OnEndGame(int teamId, EntityId playerId, EGameOverReason reason) = 0;
	virtual int GetRoundNumber() = 0;
	virtual int GetRoundsRemaining() = 0;

	virtual int GetPrimaryTeam() = 0;

	virtual bool IsInProgress() const	= 0;
	virtual bool IsRestarting() const = 0;
	virtual bool IsRestartingRound(int round) const = 0;
};

#endif // _GameRulesRoundsModule_h_