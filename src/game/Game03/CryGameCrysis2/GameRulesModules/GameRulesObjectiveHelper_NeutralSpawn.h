/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Helper objective, spawns an entity at the position of another entity
		i.e. Spawns a gun at a base
	-------------------------------------------------------------------------
	History:
	- 27:11:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_OBJECTIVE_HELPER_NEUTRAL_SPAWN_H_
#define _GAME_RULES_OBJECTIVE_HELPER_NEUTRAL_SPAWN_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesObjective.h"
#include "IEntitySystem.h"
#include "GameRulesModules/IGameRulesModuleRMIListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"

class CGameRulesObjectiveHelper_NeutralSpawn :
										public IGameRulesObjective,
										public IEntityEventListener,
										public IGameRulesModuleRMIListener,
										public IGameRulesClientConnectionListener
{
public:
	CGameRulesObjectiveHelper_NeutralSpawn();

	// IGameRulesObjective
	virtual ~CGameRulesObjectiveHelper_NeutralSpawn();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void Enable(int teamId, bool enable);
	virtual bool IsComplete(int teamId);

	virtual void OnHostMigration(bool becomeServer);
	// ~IGameRulesObjective

	// IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event );
	// ~IEntityEventListener

	// IGameRulesModuleRMIListener
	virtual void OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params) {};
	virtual void OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params);
	// ~IGameRulesModuleRMIListener

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnClientDisconnect(int channelId, EntityId playerId) {};
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId);
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

protected:

	enum ESpawnEntityState
	{
		eSES_Waiting,
		eSES_Spawning,
		eSES_Spawned,
	};

	struct SSpawnEntity
	{
		Vec3 m_startingPosition;
		EntityId m_positionEntityId;
		EntityId m_spawnedEntityId;
		float m_timeToSpawn;
		ESpawnEntityState m_state;
		bool m_inBase;
	};

	typedef std::vector<SSpawnEntity> TSpawnEntityVec;
	typedef CryFixedStringT<32> TFixedString;
	typedef std::vector<int> TIntVec;

	bool IsEnabled();
	void CleanupEntities();
	void SpawnEntity(SSpawnEntity *pSpawnEntity);
	void UpdateAllIcons();
	void UpdateIcon(SSpawnEntity *pSpawnEntity);
	
	TSpawnEntityVec m_entities;
	TIntVec m_availableSpawnPoints;

	Vec3 m_spawnOffset;
	Vec3 m_spawnRotation;

	IEntityClass *m_pSpawnEntityClass;
	const IEntityClass *m_pPositionEntityClass;

	int m_totalEntitiesSpawned;
	int m_moduleRMIIndex;
	int m_baseIcon;
	int m_iconPriority;
	int m_maxAtOnce;

	float m_resetOnRemovedTime;

	bool m_teamEnabled[2];
	bool m_resetOnRemoved;
};

#endif // _GAME_RULES_OBJECTIVE_HELPER_NEUTRAL_SPAWN_H_
