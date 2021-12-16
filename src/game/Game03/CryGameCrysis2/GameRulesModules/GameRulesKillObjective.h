/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a kill objective (destroy an entity)
	-------------------------------------------------------------------------
	History:
	- 23:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_KILL_OBJECTIVE_H_
#define _GAME_RULES_KILL_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesEntityObjective.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesModuleRMIListener.h"

class CGameRulesKillObjective :		public IGameRulesEntityObjective,
									public IGameRulesKillListener,
									public IGameRulesTeamChangedListener,
									public IGameRulesModuleRMIListener
{
public:
	CGameRulesKillObjective();

	// IGameRulesObjective
	virtual ~CGameRulesKillObjective();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void EnableCompletion(int teamId, bool enable);
	virtual void EnableScoring(int teamId, bool enable) {};

	virtual bool IsComplete(int teamId);

	virtual void AddEntityId(int type, EntityId entityId, int index, bool isNewEntity);
	virtual void RemoveEntityId(int type, EntityId entityId);
	virtual void ClearEntities(int type);
	virtual bool IsEntityFinished(int type, int index);
	virtual bool CanRemoveEntity(int type, int index) { return true; };
	virtual void SetWaveNumber(int num, int waveCount) { }

	virtual void OnHostMigration(bool becomeServer) {}
	// ~IGameRulesObjective

	// IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	// ~IGameRulesKillListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// IGameRulesModuleRMIListener
	virtual void OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params) {};
	virtual void OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params);
	// ~IGameRulesModuleRMIListener

protected:
	struct SKillEntity
	{
		static const int MAX_PROXY_ENTITIES = 3;
		EntityId m_proxyEntityIds[MAX_PROXY_ENTITIES];
		EntityId m_targetId;
		int m_proxyEntityIdCount;
		int m_teamId;
		int m_currentIcon;
		bool m_isDead;
		bool m_isActive;
	};

	void EntityKilled(EntityId targetId, EntityId shooterId);
	void SetIcon(SKillEntity *pKillEntity);
	void ShowHUDMessage(int teamId, const char *pFriendlyMessage, const char *pHostileMessage, EntityId aboutPlayerId, const char *pOwnMessage);
	bool CheckRange(EntityId shooterId, EntityId targetId);
	void CleanUpEntity(SKillEntity *pKillEntity);

	static const int KILL_OBJECTIVE_MAX_ENTITIES = 4;		// TODO: Specify this in the xml
	SKillEntity m_entities[KILL_OBJECTIVE_MAX_ENTITIES];

	int m_targetsRemainingForTeams[2];		// TODO: cope with games that don't use 2 teams
	int m_friendlyIcon;
	int m_hostileIcon;
	int m_moduleRMIIndex;

	EntityId m_lookAtEntityId;

	float m_maxDistanceSqr;

	bool m_completingEnabled[2];
	bool m_useIcons;
	bool m_checkRange;

	const static int k_maxAudioSignalLength = 16;
	char m_killedSignal[k_maxAudioSignalLength];

};

#endif // _GAME_RULES_KILL_OBJECTIVE_H_
