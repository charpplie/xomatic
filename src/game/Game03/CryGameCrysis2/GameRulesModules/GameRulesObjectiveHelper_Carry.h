/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Helper objective, tracks entities that can be carried
		i.e. Javelin missile, Extraction gun, BombTheBase bomb
	-------------------------------------------------------------------------
	History:
	- 26:11:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_OBJECTIVE_HELPER_CARRY_H_
#define _GAME_RULES_OBJECTIVE_HELPER_CARRY_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesObjective.h"
#include "GameRulesModules/IGameRulesPickupListener.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesModuleRMIListener.h"
#include "IEntitySystem.h"

class CGameRulesObjectiveHelper_Carry :	
											public IGameRulesObjective,
											public IGameRulesPickupListener,
											public IGameRulesKillListener,
											public IGameRulesClientConnectionListener,
											public IGameRulesTeamChangedListener,
											public IGameRulesModuleRMIListener,
											public IEntitySystemSink
{
public:
	CGameRulesObjectiveHelper_Carry();

	// IGameRulesObjective
	virtual ~CGameRulesObjectiveHelper_Carry();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void Enable(int teamId, bool enable);
	virtual bool IsComplete(int teamId);

	virtual void OnHostMigration(bool becomeServer) {}		// Host migration
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

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnClientDisconnect(int channelId, EntityId playerId);
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId);
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// IGameRulesModuleRMIListener
	virtual void OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params);
	virtual void OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params);
	// ~IGameRulesModuleRMIListener

	// IEntitySystemSink
	virtual bool OnBeforeSpawn( SEntitySpawnParams &params ) { return true; };
	virtual void OnSpawn( IEntity *pEntity,SEntitySpawnParams &params );
	virtual bool OnRemove( IEntity *pEntity );
	virtual void OnEvent( IEntity *pEntity, SEntityEvent &event );
	// ~IEntitySystemSink

	static EntityId S_GetBaseEntityIdForCarryEnt( const EntityId carryEid, EntityId* baseEid );

	void GetMemoryUsage(class ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_entities);		
	}
protected:
	enum EEntityState
	{
		eES_AtBase = 0,
		eES_Carried,
		eES_Dropped,
	};

	enum EPickupType
	{
		ePT_Proximity = 0,
		ePT_Item = 1,
	};

	enum EPickupAction
	{
		ePA_Pickup = 0,
		ePA_Remove,
		ePA_Nothing,
	};

	struct SEntityDetails
	{
		EntityId m_id;
		EntityId m_carrierId;
		EntityId m_iconEntityId;
		EntityId m_baseId;  // lazily evaluated, use GetBaseEntityIdForCarryEnt()

		int m_currentIcon;

		EEntityState m_state;

		float m_droppedTime;

		void GetMemoryUsage(ICrySizer *pSizer) const{}
	};

	typedef std::vector<SEntityDetails> TEntityDetailsVec;
	typedef CryFixedStringT<32> TFixedString;
	typedef CryFixedStringT<64> TSoundNameString;

	bool CheckForAndDropItem(EntityId playerId);
	SEntityDetails *FindEntityDetails(EntityId entId);
	void CallScriptFunction(IEntity *pEntity, const char *pFuncName, EntityId otherEnt);

	void UpdateIcon(SEntityDetails *pEntityDetails );
	void ClItemPickedUp(SEntityDetails *pEntityDetails, const int oldCarrierId);
	void ClItemDropped(SEntityDetails *pEntityDetails, EntityId previousCarrierId, bool isOnRemove);

	void OnItemOrEntityPickup( EntityId entityId, EntityId actorId );
	void OnItemOrEntityDrop( EntityId entityId, EntityId actorId, bool isOnRemove );

	void ProcessSoundForCarryStateChange( SEntityDetails* pEntityDetails, const int oldCarrierId, const bool isOnRemove );
	void UpdateTeamAlarmSound(const int alarmTeam, const char newSnd[], const Vec3* newPos);
	
	EntityId GetBaseEntityIdForCarryEnt( SEntityDetails* pEntityDetails );

	TEntityDetailsVec m_entities;

	TFixedString m_friendlyPickedUpString;
	TFixedString m_hostilePickedUpString;
	TFixedString m_friendlyDroppedString;
	TFixedString m_hostileDroppedString;

	TSoundNameString m_baseAlarmSound;
	TSoundNameString m_enemyPickupFriendlySound;
	TSoundNameString m_friendlyPickupEnemySound;
	TSoundNameString m_enemyPickupEnemySound;
	TSoundNameString m_friendlyPickupFriendlySound;
	TSoundNameString m_enemyCompleteSound;
	TSoundNameString m_friendlyCompleteSound;

	tSoundID m_teamActiveAlarmSound[2];

	IEntityClass *m_pEntityClass;

	int m_moduleRMIIndex;
	int m_droppedIconNeutral;
	int m_droppedIconFriendly;
	int m_droppedIconHostile;
	int m_carriedIconFriendly;
	int m_carriedIconHostile;

	EPickupType m_pickupType;

	EPickupAction m_hostilePickAc;
	EPickupAction m_friendlyPickAc;

	float m_droppedRemoveTime;

	float m_baseDefenceRadius;
	float m_baseDefenceRadiusHeight;

	bool m_hostileCanPickAtBase;
	bool m_friendlyCanPickAtBase;

	bool m_teamEnabled[2];
	bool m_removeOnDropped;
	bool m_useSpawnPOIs;
};

#endif // _GAME_RULES_OBJECTIVE_HELPER_CARRY_H_
