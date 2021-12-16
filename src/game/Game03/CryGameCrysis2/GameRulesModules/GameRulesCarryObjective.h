/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a carry objective (take something to a place)
	-------------------------------------------------------------------------
	History:
	- 27:10:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_CARRY_OBJECTIVE_H_
#define _GAME_RULES_CARRY_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesEntityObjective.h"
#include "IEntitySystem.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "IGameFramework.h"
#include "GameRulesTypes.h"
#include "Utility/CryFixedArray.h"

class CGameRulesCarryObjective :	public IGameRulesEntityObjective,
									public IEntityEventListener,
									public IGameRulesClientConnectionListener,
									public IGameRulesTeamChangedListener,
									public IGameRulesKillListener,
									public IGameFrameworkListener
{
public:
	CGameRulesCarryObjective();
	~CGameRulesCarryObjective();

	// IGameRulesEntityObjective
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void EnableCompletion(int teamId, bool enable);
	virtual void EnableScoring(int teamId, bool enable);

	virtual bool IsComplete(int teamId);

	virtual void AddEntityId(int type, EntityId entityId, int index, bool isNewEntity);
	virtual void RemoveEntityId(int type, EntityId entityId);
	virtual void ClearEntities(int type);
	virtual bool IsEntityFinished(int type, int index);
	virtual bool CanRemoveEntity(int type, int index);
	virtual void SetWaveNumber(int num, int waveCount) { }

	virtual void OnHostMigration(bool becomeServer);
	// ~IGameRulesEntityObjective

	// IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event );
	// ~IEntityEventListener

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnClientDisconnect(int channelId, EntityId playerId) {};
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	// ~IGameRulesKillListener

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* pNextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

protected:

	static const int CARRY_OBJECTIVE_MAX_CARRY_ENTITIES = 10;		// TODO: Specify this in the xml
	static const int CARRY_OBJECTIVE_MAX_TARGET_ENTITIES = 10;		// TODO: Specify this in the xml
	static const int CARRY_OBJECTIVE_MAX_PLAYERS = 16;

	typedef CryFixedArray<EntityId, CARRY_OBJECTIVE_MAX_PLAYERS> TEntityIdArray;

	struct SCarryEntity
	{
		int m_entityTeamId;

		EntityId m_entityId;
		EntityId m_deployerId;
		EntityId m_baseId;  // lazily evaluated, use GetBaseEntityIdForCarryEnt()

		float m_deployingTime;

		bool m_isActive;
		bool m_isFinished;
		bool m_isItem;
	};

	struct STargetEntity
	{
		int m_entityTeamId;
		int m_objectiveTeamId;

		EntityId m_entityId;

		bool m_isActive;

		TEntityIdArray m_insideEntities;
	};

	enum EPickupType
	{
		ePT_Proximity = 0,
		ePT_Item = 1,
	};

  void CleanUpCarryEntity(SCarryEntity *pCarryEntity, int index);

  void CleanUpTargetEntity(STargetEntity *pTargetEntity);

	SCarryEntity *GetCarryEntity(EntityId actorId, int &resultIndex);

	void SvSetDeployerId(int index, EntityId newId);

	void ClEntityCompleted(SCarryEntity *pCarryEntity);
	void ClSetIcon(STargetEntity *pTargetEntity);
	void ClDeployerIdChanged(int index, EntityId newId);

	EntityId GetAttachedEntity(EntityId parentEid, int chrSlot, char attachmentInterfaceName[]);

	EntityId GetBaseEntityIdForCarryEnt(SCarryEntity* pCarryEntity);

	SCarryEntity m_carryEntities[CARRY_OBJECTIVE_MAX_CARRY_ENTITIES];
	STargetEntity m_targetEntities[CARRY_OBJECTIVE_MAX_TARGET_ENTITIES];

	int m_friendlyBaseIcon;
	int m_hostileBaseIcon;
	int m_iconPriority;
	int m_entitiesCompleted[2];

	int m_clientDeployingIndex;

	float m_deployTime;
	float m_clientDeployingTime;

	bool m_scoringEnabled[2];			// Todo: support games without 2 teams
	bool m_removeOnCompletion;
	bool m_invertTargets;
	bool m_useDeployTime;
	bool m_requireOwnAtBase;

	EPickupType m_carryPickupType;
};

#endif // _GAME_RULES_CARRY_OBJECTIVE_H_
