/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 25:6:2009  : Created by Colin Gulliver

*************************************************************************/
#ifndef __GAMERULES_CAPTURETHEFLAG_H__
#define __GAMERULES_CAPTURETHEFLAG_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRules_TeamInstantAction.h"


class CGameRules_CaptureTheFlag :	public CGameRules_TeamInstantAction,
									public IEntityEventListener
{
public:
	CGameRules_CaptureTheFlag();
	virtual ~CGameRules_CaptureTheFlag();

	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject );
	virtual void Release();

	virtual void SvOnStartGame();
	virtual void SvOnClientEnteredGame(int channelId, IEntity *player, bool reset, bool loadingSaveGame);
	virtual void SvOnClientDisconnect(int channelId);

	virtual void SvOnKill(EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type);
	virtual void SvOnHit(const HitInfo *hit);
	virtual void SvOnChangeTeam(EntityId playerId, int teamId);

	virtual void SvOnUpdate(float frameTime);

	virtual void SvRestartGame(bool forceInGame);

	virtual void SvAssignKillPoints(EntityId targetId, EntityId shooterId);

	virtual void SvRevivePlayer(EntityId playerId);
	virtual void SvResetScore(EntityId playerId);

	virtual void SvGotoState(EGR_GameState newState);

	virtual void SvSetupPlayer(EntityId playerId);
	virtual void SvCleanUpPlayer(EntityId playerId);
	virtual void SvCheckTeamScore(int teamId);

	virtual void ClOnStartGame();

	virtual void ClOnKill(EntityId targetId, EntityId shooterId, int weaponClassId, float damage, int material, int hit_type);
	virtual void ClOnSetTeam( EntityId playerId, int teamId );

	void SvReset();
	void InitEntities();
	void GatherEntities();
	void SvCheckForDeadFlagCarrier(EntityId playerId);

	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event );

	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagReset, UI8Params, eNRT_ReliableOrdered);

	struct EntityTeamParams
	{
		EntityTeamParams() {};
		EntityTeamParams(EntityId _entityId, int _teamId)
			:	entityId(_entityId), teamId(_teamId)
		{};

		EntityId entityId;
		int teamId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("teamId", teamId, 'ui8');
		};
	};
	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagRetrieved, EntityTeamParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagCompleted, EntityTeamParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagPickedUp, EntityTeamParams, eNRT_ReliableOrdered);

	struct EntityTeamPosParams
	{
		EntityTeamPosParams() {};
		EntityTeamPosParams(EntityId _entityId, int _teamId, const Vec3 &_pos)
			:	entityId(_entityId), teamId(_teamId), pos(_pos)
		{};

		EntityId entityId;
		int teamId;
		Vec3 pos;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("teamId", teamId, 'ui8');
			ser.Value("pos", pos, 'wrl2');
		};
	};
	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagDropped, EntityTeamPosParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagAssistCapture, NoParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClOnFlagCapture, UI8Params, eNRT_ReliableUnordered);

	struct CTFStateParams
	{
		CTFStateParams() {};
		CTFStateParams(int _flag1State, EntityId _flag1CarrierId, int _flag2State, EntityId _flag2CarrierId)
			:	flag1State(_flag1State), flag1CarrierId(_flag1CarrierId), flag2State(_flag2State), flag2CarrierId(_flag2CarrierId)
		{};

		int flag1State;
		EntityId flag1CarrierId;
		int flag2State;
		EntityId flag2CarrierId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("flag1State", flag1State, 'ui8');
			ser.Value("flag1CarrierId", flag1CarrierId, 'eid');
			ser.Value("flag2State", flag2State, 'ui8');
			ser.Value("flag2CarrierId", flag2CarrierId, 'eid');
		};
	};
	DECLARE_CLIENT_RMI_NOATTACH(ClSetState, CTFStateParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClFlagDefended, NoParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClFlagCarrierProtected, NoParams, eNRT_ReliableUnordered);

protected:
	enum ECTFSynchedEntityKeys
	{
		ECTF_KEY_FLAGS = ETIA_KEY_MAX,			// Must remain first in enum!
		ECTF_KEY_MAX,							// Must remain last in enum!
	};

	enum ECTFFlagState
	{
		ECTF_FLAG_STATE_INBASE,
		ECTF_FLAG_STATE_CARRIED,
		ECTF_FLAG_STATE_DROPPED,
	};

	struct STeamInfo
	{
		STeamInfo() : flagId(0), baseId(0), carrierId(0), previousCarriers(), flagState(ECTF_FLAG_STATE_INBASE), flagDropTime(0.f), alarmSoundId(0)
		{};

		EntityId flagId;
		EntityId baseId;
		EntityId carrierId;
		std::vector<EntityId> previousCarriers;
		int flagState;
		float flagDropTime;
		std::vector<EntityId> insideIds;
		tSoundID alarmSoundId;
	};

	STeamInfo m_teamInfo[2];

	STeamInfo *GetTeamInfo(int teamId);

	void StartFlagResetTimer(int teamId);
	void StopFlagResetTimer(int teamId);

	void SvEntityEnterFlagArea(int flagTeamId, EntityId insideId);
	void SvEntityLeaveFlagArea(int flagTeamId, EntityId insideId);

	void ClFlagRetrieved(int teamId, EntityId playerId);

	void ResetFlag(int teamId);
	void ClFlagReset(int teamId);

	void SvFlagCompleted(int teamId, EntityId playerId);
	void ClFlagCompleted(int teamId, EntityId playerId);

	void FlagPickedUp(int teamId, EntityId playerId);
	void ClFlagPickedUp(int teamId, EntityId playerId);

	void SvFlagDropped(int teamId, EntityId playerId);
	void ClFlagDropped(int teamId, EntityId playerId, const Vec3 &flagPos);

	static void CmdDumpEntities(IConsoleCmdArgs *pArgs);

	void ClSetEntities();

	void ClRethinkMusic();
	tSoundID ClPlaySound(const char *soundName, const Vec3 &pos, uint32 flags, uint32 semantic);

	void SetFlagState(int teamId, ECTFFlagState state);

	struct SCTFPlayerData
	{
		SCTFPlayerData() : timeLastHitFlagCarrier(0.f)
		{};

		float timeLastHitFlagCarrier;
	};
	typedef std::map<EntityId, SCTFPlayerData>	TCTFPlayerDataMap;
	TCTFPlayerDataMap m_ctfPlayerValues;

private:
	bool m_clientStateSet;
}; 

#endif //__GAMERULES_CAPTURETHEFLAG__
