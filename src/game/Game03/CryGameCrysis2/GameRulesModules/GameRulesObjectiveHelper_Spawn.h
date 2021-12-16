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
	- 23:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_OBJECTIVE_HELPER_SPAWN_H_
#define _GAME_RULES_OBJECTIVE_HELPER_SPAWN_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesObjective.h"
#include "GameRulesModules/IGameRulesPickupListener.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesModuleRMIListener.h"
#include "IGameFramework.h"
#include "IEntitySystem.h"

class CGameRulesObjectiveHelper_Spawn :		public IGameRulesObjective,
											public IGameRulesPickupListener,
											public IGameRulesKillListener,
											public IGameRulesTeamChangedListener,
											public IGameRulesClientConnectionListener,
											public IGameFrameworkListener,
											public IGameRulesModuleRMIListener,
											public IEntityEventListener
{
public:
	CGameRulesObjectiveHelper_Spawn();

	// IGameRulesObjective
	virtual ~CGameRulesObjectiveHelper_Spawn();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void Enable(int teamId, bool enable);
	virtual bool IsComplete(int teamId);

	virtual void OnHostMigration(bool becomeServer);
	// ~IGameRulesObjective

	// IGameRulesPickupListener
	virtual void OnItemPickedUp(EntityId itemId, EntityId actorId);
	virtual void OnItemDropped(EntityId itemId, EntityId actorId);
	virtual void OnPickupEntityAttached(EntityId entityId, EntityId actorId);
	virtual void OnPickupEntityDetached(EntityId entityId, EntityId actorId, bool isOnRemove);
	// ~IGameRulesPickupListener

	// IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	// ~IGameRulesKillListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnClientDisconnect(int channelId, EntityId playerId);
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId);
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* pNextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	// IGameRulesModuleRMIListener
	virtual void OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params);
	virtual void OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params);
	// ~IGameRulesModuleRMIListener

	// IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity, SEntityEvent &event );
	// ~IEntityEventListener

protected:
	enum ESpawnEntityState
	{
		ESES_AtBase,
		ESES_Carried,
		ESES_Dropped,
	};

	struct SSpawnEntity
	{
		static const int NUM_TEAMS = 2;

		EntityId m_positionEntityId;
		EntityId m_spawnedEntityId[NUM_TEAMS];
		EntityId m_carrierEntityId[NUM_TEAMS];

		ESpawnEntityState m_state[NUM_TEAMS];
	};

	struct SClientEntity
	{
		SClientEntity(EntityId id, EntityId carrierId, int teamId, EntityId spawnerId, ESpawnEntityState state)
		{
			m_id = id;
			m_teamId = teamId;
			m_carrierId = carrierId;
			m_spawnerId = spawnerId;
			m_currentIconEntityId = 0;
			m_state = state;
			m_currentIcon = int(EGRMO_Unknown);
			m_currentIconPriority = 0;
		}

		EntityId m_id;
		EntityId m_carrierId;
		EntityId m_spawnerId;
		EntityId m_currentIconEntityId;
		int m_teamId;
		int m_currentIcon;
		int m_currentIconPriority;
		ESpawnEntityState m_state;
	};

	typedef std::vector<SSpawnEntity> TSpawnEntityVec;
	typedef std::vector<SClientEntity> TClientEntityVec;
	typedef CryFixedStringT<32> TFixedString;

	void AddEntity(EntityId itemId, EntityId spawnerId, ESpawnEntityState state);
	void SpawnEntity(SSpawnEntity *pDetails);
	void CheckForItemCarrier(EntityId playerId);
	void SvItemDropped(SSpawnEntity *pSpawnEntity, int teamIndex);
	void SetIcon(SClientEntity *pClientEntity, bool force);
	SClientEntity *FindClientEntity(EntityId itemId);
	void ShowHUDMessage(int teamId, const char *pFriendlyMessage, const char *pHostileMessage, EntityId aboutPlayerId, const char *pOwnMessage);
	bool IsCorrectTeam(int teamId1, int teamId2);
	int GetItemTeamId(int spawnerTeamId);

	void ApplyCarrierSpeedMultiplier(EntityId playerId, float mult);

	void ClItemDropped(EntityId itemId);
	void ClClearEntities();
	void EntityScriptCall(const char* scriptFuncName, EntityId entityId);

	void OnItemOrEntityPickup( EntityId entityId, EntityId actorId );
	void OnItemOrEntityDrop( EntityId entityId, EntityId actorId );

	TSpawnEntityVec m_entities;
	TClientEntityVec m_clientEntities;

	TFixedString m_textMessagePickUpFriendly;
	TFixedString m_textMessagePickUpHostile;

	TFixedString m_textMessageDroppedFriendly;		// TODO: Use these!
	TFixedString m_textMessageDroppedHostile;

	Vec3 m_spawnOffset;
	Vec3 m_spawnRotation;

	IEntityClass *m_pSpawnEntityClass;
	const IEntityClass *m_pPositionEntityClass;

	int m_friendlyIconDropped;
	int m_hostileIconDropped;
	int m_friendlyIconCarried;
	int m_hostileIconCarried;
	int m_friendlyIconAtBase;
	int m_hostileIconAtBase;
	int m_moduleRMIIndex;

	float m_carrierSpeedMult;

	bool m_teamEnabled[2];
	bool m_invertTeams;
	bool m_resetOnDropped;
	bool m_resetOnRemoved;

	bool m_prefixSpawnAtName;
};

#endif // _GAME_RULES_OBJECTIVE_HELPER_SPAWN_H_
