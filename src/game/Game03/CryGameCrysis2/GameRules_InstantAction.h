/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 18:6:2009  : Created by Colin Gulliver

*************************************************************************/
#ifndef __GAMERULES_INSTANTACTION_H__
#define __GAMERULES_INSTANTACTION_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRules_SinglePlayer.h"


class CGameRules_InstantAction :	public CGameRules_SinglePlayer
{
public:

	CGameRules_InstantAction();
	virtual ~CGameRules_InstantAction();

	virtual bool Init( IGameObject * pGameObject );

	virtual void SvOnClientConnect(int channelId, bool reset, const char *name);
	virtual void SvOnClientDisconnect(int channelId);
	virtual void SvOnClientEnteredGame(int channelId, IEntity *player, bool reset, bool loadingSaveGame);
	virtual void SvOnStartLevel();
	virtual void SvOnStartGame();

	virtual void SvOnKill(EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type);
	virtual void SvOnHit(const HitInfo *hit);
	virtual bool SvOnFreeze(EntityId targetId, EntityId shooterId, EntityId weaponId, float value);
	virtual void SvOnExplosion(const ExplosionInfo *explosion);
	virtual void SvOnChangeTeam(EntityId playerId, int teamId);
	virtual void SvOnSetTeam(EntityId playerId, int teamId);
	virtual void SvOnItemPickedUp(EntityId itemId, EntityId actorId);
	virtual void SvOnItemDropped(EntityId itemId, EntityId actorId);
	virtual void SvOnAddTaggedEntity(EntityId shooterId, EntityId targetId);
	virtual void SvOnVehicleDestroyed(EntityId vehicleId);
	virtual void SvOnVehicleSubmerged(EntityId vehicleId, float ratio);

	virtual void SvOnTimer(int timerId, int64 msec);
	virtual void SvOnUpdate(float frameTime);

	virtual void SvRestartGame(bool forceInGame);

	virtual void SvOnReset();

	virtual void SvCheckPlayerScoreLimit(EntityId playerId, int kills);
	virtual void SvResetScore(EntityId playerId);
	virtual void SvCheckTimeLimit();
	virtual void SvRevivePlayer(EntityId playerId);
	virtual void SvReviveAllPlayers();
	virtual void SvResetPlayers();

	virtual void SvSetupPlayer(EntityId playerId);
	virtual void SvCleanUpPlayer(EntityId playerId);

	enum EGameEndReason
	{
		EGER_ScoreLimitReached,
		EGER_TimeLimitReached,
	};
	virtual void SvOnGameEnd(unsigned int winnerId, EGameEndReason reason);

	virtual bool SvPlayerCountOk();

	virtual void ClOnConnect();
	virtual void ClOnDisconnect(EDisconnectionCause cause, const char *desc);
	virtual void ClOnActorAction(IEntity *player, const ActionId& action, int activationMode, float value);
	virtual void ClOnStartLevel();
	virtual void ClOnStartGame();

	virtual void ClOnRevive(EntityId playerId, const Vec3 &pos, const Quat &rot, int teamId);
	virtual void ClOnReviveInVehicle(EntityId playerId, EntityId vehicleId, int seatId, int teamId);

	virtual void ClOnKill(EntityId targetId, EntityId shooterId, int weaponClassId, float damage, int material, int hit_type);
	virtual void ClOnHit(const HitInfo *hit);
	virtual void ClOnFreeze(EntityId targetId, EntityId shooterId, EntityId weaponId, float value);
	virtual void ClOnExplosion(const ExplosionInfo *explosion);
	virtual void ClOnChangeTeam(EntityId playerId, int teamId);
	virtual void ClOnSetTeam(EntityId playerId, int teamId);
	virtual void ClOnItemPickedUp(EntityId itemId, EntityId actorId);
	virtual void ClOnItemDropped(EntityId itemId, EntityId actorId);
	virtual void ClOnAddTaggedEntity(EntityId shooterId, EntityId targetId);
	virtual void ClOnVehicleDestroyed(EntityId vehicleId);
	virtual void ClOnVehicleSubmerged(EntityId vehicleId, float ratio);

	virtual void ClOnTimer(int timerId, int64 msec);
	virtual void ClOnUpdate(float frameTime);

	virtual void ClRequestSpawnGroup(EntityId spawnGroupId);


#if 0
	DECLARE_SERVER_RMI_NOATTACH(SvRequestRevive, EntityParams, eNRT_ReliableUnordered);
#endif

	DECLARE_CLIENT_RMI_NOATTACH(ClVictory, EntityParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClNoWinner, NoParams, eNRT_ReliableUnordered);

	struct KillParams
	{
		KillParams() {};
		KillParams(EntityId _targetId, EntityId _shooterId, int _weaponClassId, float _damage, int _material, int _hit_type, const Vec3 &_impulse)
		:	targetId(_targetId),
			shooterId(_shooterId),
			weaponClassId(_weaponClassId),
			damage(_damage),
			material(_material),
			hit_type(_hit_type),
			impulse(_impulse)
		{};

		EntityId targetId;
		EntityId shooterId;
		int weaponClassId;
		float damage;
		int material;
		int hit_type;
		Vec3 impulse;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("targetId", targetId, 'eid');
			ser.Value("shooterId", shooterId, 'eid');
			ser.Value("weaponClassId", weaponClassId, 'ui16');
			ser.Value("damage", damage, 'dmg');
			ser.Value("material", material, 'mat');
			ser.Value("hit_type", hit_type, 'hTyp');
			ser.Value("impulse", impulse, 'kImp');
		};
	};
	DECLARE_CLIENT_RMI_NOATTACH(ClKill, KillParams, eNRT_ReliableUnordered);

	struct SPlayerData
	{
		SPlayerData() : deathTime(0.f), deathPos(ZERO)
		{};

		float deathTime;
		Vec3 deathPos;
	};
	typedef std::map<EntityId, SPlayerData>	TPlayerDataMap;
	TPlayerDataMap m_playerValues;

	struct UI8Params
	{
		UI8Params() {};
		UI8Params(uint8 _data)
		:	data(_data)
		{};

		uint8 data;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("data", data, 'ui8');
		};
	};
	DECLARE_CLIENT_RMI_NOATTACH(ClStateChanged, UI8Params, eNRT_ReliableUnordered);

	enum EGR_GameState
	{
		EGR_Reset = 0,
		EGR_PreGame,
		EGR_InGame,
		EGR_PostGame,
	};
	virtual void SvGotoState(EGR_GameState newState);
	enum EIASynchedEntityKeys
	{
		EIA_KEY_KILLS,
		EIA_KEY_DEATHS,
		EIA_KEY_HEADSHOTS,
		EIA_KEY_PING,
		EIA_KEY_MAX,		// Must remain last in enum!
	};
protected:
	EGR_GameState m_state;
	float m_stateChangeTime;
	bool m_starting;
	bool m_forceInGame;
}; 

#endif //__GAMERULES_INSTANTACTION_H__
