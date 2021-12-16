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

#ifndef _GameRulesStateModule_h_
#define _GameRulesStateModule_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameObject.h"
#include "SerializeFwd.h"

class IGameRulesStateModule
{
public:
	enum EGR_GameState
	{
		EGRS_Reset = 0,
		EGRS_PreGame,
		EGRS_InGame,
		EGRS_PostGame,
		EGRS_MAX		// Must remain last
	};

	virtual ~IGameRulesStateModule() {}

	virtual void Init(XmlNodeRef xml) = 0;
	virtual void PostInit() = 0;

	virtual void OnGameRestart() = 0;
	virtual void OnGameReset() = 0;
	virtual void OnGameEnd() = 0;

	virtual EGR_GameState GetGameState() = 0;

	virtual void Update(float frameTime) = 0;

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;
};

#endif // _GameRulesStateModule_h_