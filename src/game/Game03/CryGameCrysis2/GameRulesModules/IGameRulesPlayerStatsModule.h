/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for the game rule module to handle player scores and stats
	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef _GAME_RULES_PLAYER_STATS_MODULE_H_
#define _GAME_RULES_PLAYER_STATS_MODULE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameRulesSystem.h>

struct SGameRulesPlayerStat
{
	enum EFlags
	{
		PLYSTATFL_NULL											= (0),
		PLYSTATFL_HASSPAWNEDTHISROUND				= (1 << 0),
		PLYSTATFL_DIEDINEXTRATIMETHISROUND	= (1 << 1),
	};

	SGameRulesPlayerStat(EntityId _playerId)
	{
		playerId = _playerId;
		Clear();
	}

	void Clear()
	{
		points = kills = assists = deaths = deathsThisRound = headshots = teamKills = ping = successfulExplosions = flags = 0;
	}

	void NetSerialize(TSerialize ser)
	{
		ser.Value("points", points, 'i16');
		ser.Value("kills", kills, 'ui16');
		ser.Value("assists", assists, 'ui16');
		ser.Value("deaths", deaths, 'ui16');
		ser.Value("deathsThisRound", deathsThisRound, 'ui16');
		ser.Value("teamKills", teamKills, 'ui16');
		ser.Value("headshots", headshots, 'ui16');
		ser.Value("ping", ping, 'ui16');
		ser.Value("expl", successfulExplosions, 'ui8');
		ser.Value("flags", flags, 'ui8');
	}

	void IncrementTimesPlayerKilled( const EntityId otherPlayerId )
	{
		TKilledByCounter::iterator res = killedByCounts.find( otherPlayerId );
		if( res == killedByCounts.end() )
		{
			killedByCounts.insert(TKilledByCounter::value_type(otherPlayerId, 1));
			return;
		}

		res->second ++;
	}

	uint8 GetTimesKilledPlayer( const EntityId otherPlayerId ) const
	{
		TKilledByCounter::const_iterator res = killedByCounts.find( otherPlayerId );
		if( res == killedByCounts.end() )
		{
			return 0;
		}

		return res->second;
	}

	// TKilledByCounter will be as big as the number of entities killed.
	//   The registration for a player is not cleaned when a player exits.
	//   This may be desired behaviour.
	CRY_TODO( 09, 03, 2010, "Replace TKilledByCounter (a std::map) with something faster.")
	typedef std::map<EntityId, uint8> TKilledByCounter;
	TKilledByCounter killedByCounts;

	EntityId playerId;
	int16 points;
	uint16 kills;
	uint16 assists;
	uint16 deaths;
	uint16 deathsThisRound;
	uint16 teamKills;
	uint16 headshots;
	uint16 ping;
	uint8 successfulExplosions;
	uint8 flags;  // see EFlags above
};

class IGameRulesPlayerStatsModule
{
public:
	virtual ~IGameRulesPlayerStatsModule() {}

	virtual void	Init(XmlNodeRef xml) = 0;
	virtual void	Reset() = 0;
	virtual void	Update(float frameTime) = 0;

	virtual void	OnStartNewRound() = 0;

	virtual bool	NetSerialize( EntityId playerId, TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) = 0;

	virtual void	CreatePlayerStats(EntityId playerId) = 0;
	virtual void	RemovePlayerStats(EntityId playerId) = 0;

	virtual void	ClearAllPlayerStats() = 0;

	virtual void	OnPlayerKilled(const HitInfo &info) = 0;
	virtual void	IncreasePoints(EntityId playerId, int amount) = 0;
	virtual void	ProcessSuccessfulExplosion(EntityId playerId) = 0;
	virtual void	IncrementAssistKills(EntityId playerId) = 0;
	virtual void	IncreaseKillCount( const EntityId playerId, const EntityId victimId ) = 0;

	virtual int		GetNumPlayerStats() = 0;
	virtual const SGameRulesPlayerStat*  GetNthPlayerStats(int n) = 0;
	virtual const SGameRulesPlayerStat*  GetPlayerStats(EntityId playerId) = 0;
};

#endif // _GAME_RULES_PLAYER_STATS_MODULE_H_