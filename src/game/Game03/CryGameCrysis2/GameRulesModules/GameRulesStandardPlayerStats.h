/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle player scores and stats
	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef _GAME_RULES_PLAYER_STATS_H_
#define _GAME_RULES_PLAYER_STATS_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesPlayerStatsModule.h"
#include "IGameRulesRevivedListener.h"

class CGameRules;

class CGameRulesStandardPlayerStats :	public IGameRulesPlayerStatsModule,
																			public IGameRulesRevivedListener
{
public:
	CGameRulesStandardPlayerStats();
	virtual ~CGameRulesStandardPlayerStats();

	// IGameRulesPlayerStatsModule
	virtual void	Init(XmlNodeRef xml);
	virtual void	Reset(); // TODO : call
	virtual void	Update(float frameTime);

	virtual void	OnStartNewRound();

	virtual bool	NetSerialize( EntityId playerId, TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void	CreatePlayerStats(EntityId playerId);
	virtual void	RemovePlayerStats(EntityId playerId);
	virtual void	ClearAllPlayerStats();

	virtual void	OnPlayerKilled(const HitInfo &info);
	virtual void	IncreasePoints(EntityId playerId, int amount);
	virtual void	ProcessSuccessfulExplosion(EntityId playerId);
	virtual void	IncrementAssistKills(EntityId playerId);
	virtual void	IncreaseKillCount( const EntityId playerId, const EntityId victimId );
	
	virtual int		GetNumPlayerStats();
	virtual const SGameRulesPlayerStat*  GetNthPlayerStats(int n);
	virtual const SGameRulesPlayerStat* GetPlayerStats(EntityId playerId);
	// ~IGameRulesPlayerStatsModule
	
	// IGameRulesRevivedListener
	virtual void EntityRevived(EntityId entityId);
	// ~IGameRulesRevivedListener

protected:
	static const float GR_PLAYER_STATS_PING_UPDATE_INTERVAL;

	SGameRulesPlayerStat * GetPlayerStatsInternal(EntityId playerId);
	void SendHUDExplosionEvent();

	//typedef std::map<EntityId, SGameRulesPlayerStat> TPlayerStats;
	typedef std::vector<SGameRulesPlayerStat> TPlayerStats;

	TPlayerStats m_playerStats;

	float m_lastUpdatedPings;
	int m_dbgWatchLvl;
};

#endif // _GAME_RULES_PLAYER_STATS_H_