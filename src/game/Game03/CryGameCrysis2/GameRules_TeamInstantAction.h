/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 24:6:2009  : Created by Colin Gulliver

*************************************************************************/
#ifndef __GAMERULES_TEAMINSTANTACTION_H__
#define __GAMERULES_TEAMINSTANTACTION_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRules_InstantAction.h"


class CGameRules_TeamInstantAction :	public CGameRules_InstantAction
{
public:
	CGameRules_TeamInstantAction();
	virtual ~CGameRules_TeamInstantAction();

	virtual bool Init( IGameObject * pGameObject );

	virtual void SvOnStartGame();
	virtual void SvRestartGame(bool forceInGame);

	virtual void SvOnKill(EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type);
	virtual void SvOnHit(const HitInfo *hit);

	virtual void SvAssignKillPoints(EntityId targetId, EntityId shooterId);

	virtual void SvRevivePlayer(EntityId playerId);
	virtual void SvOnGameEnd( unsigned int winnerId, EGameEndReason reason );

	virtual void SvOnChangeTeam(EntityId playerId, int teamId);

	virtual void SvCheckTimeLimit();
	virtual bool SvPlayerCountOk();

	virtual void SvResetScore(EntityId playerId);

	int GetTeamScore(int teamId);
	void SvIncreaseTeamScore(int teamId, int increase);
	virtual void SvCheckTeamScore(int teamId);
	void SvIncreasePlayerScore(EntityId playerId, int increase);

	virtual void SvSetupPlayer(EntityId playerId);
	virtual void SvCleanUpPlayer(EntityId playerId);

	DECLARE_CLIENT_RMI_NOATTACH(ClTeamVictory, UI8Params, eNRT_ReliableUnordered);

	virtual void SvGotoState(EGR_GameState newState);

	virtual void ClOnSetTeam(EntityId playerId, int teamId);

	DECLARE_CLIENT_RMI_NOATTACH(ClAssistKill, EntityParams, eNRT_ReliableUnordered);
public :
	enum ETIASynchedEntityKeys
	{
		ETIA_KEY_TEAM_KILLS = EIA_KEY_MAX,		// Must remain first in enum!
		ETIA_KEY_PLAYER_SCORE,
		ETIA_KEY_MAX,							// Must remain last in enum!
	};
	enum ETIASynchedGlobalKeys
	{
		ETIA_KEY_TEAM_SCORE_1,
		ETIA_KEY_TEAM_SCORE_2,
	};
protected:

	void SvResetTeamScore(int teamId);

	struct SAttackerStats
	{
		SAttackerStats() : shooterId(0), hitTime(0.f)
		{};
		SAttackerStats(EntityId _shooterId, float _hitTime) : shooterId(_shooterId), hitTime(_hitTime)
		{};

		EntityId shooterId;
		float hitTime;
	};
	struct STIAPlayerData
	{
		STIAPlayerData() : attackerStats()
		{};

		std::vector<SAttackerStats> attackerStats;
	};
	typedef std::map<EntityId, STIAPlayerData>	TTIAPlayerDataMap;
	TTIAPlayerDataMap m_tiaPlayerValues;
}; 

#endif //__GAMERULES_TEAMINSTANTACTION__
