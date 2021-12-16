/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 7:2:2006   15:38 : Created by Márcio Martins

*************************************************************************/
#ifndef __GAMERULES_H__
#define __GAMERULES_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IGameObject.h>
#include <IGameRulesSystem.h>
#include "Game.h"
//#include "Actor.h"
#include "SynchedStorage.h"
#include <queue>
#include "Voting.h"
#include "IViewSystem.h"
#include "RecursiveDimensionalClustering.h"
#include "GameRulesTypes.h"
#include "PlayerPlugin_ScoreRewards.h"
#include "Perk.h"
#include "TeamPerks.h"

#define MAX_CONCURRENT_EXPLOSIONS 64

struct IActor;
class CActor;
class CPlayer;

struct IGameObject;
struct IActorSystem;
struct SGameRulesScoreInfo;
struct SGameRulesPlayerStat;

class CEquipmentLoadout;
class CBattleDust;
class CMPTutorial;
class CTeamPerks;
class CBattlechatter;
class CAreaAnnouncer;
class CExplosionGameEffect;

class IGameRulesPickupListener;
class IGameRulesClientConnectionListener;
class IGameRulesTeamChangedListener;
class IGameRulesKillListener;
class IGameRulesModuleRMIListener;
class IGameRulesRevivedListener;
class IGameRulesSurvivorCountListener;
class IGameRulesPlayerStatsListener;
class IGameRulesRoundsListener;
class IGameRulesClientScoreListener;

#include "GameRulesModules/GameRulesModulesRegistration.h"

#define GAMERULES_INVOKE_ON_TEAM(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=m_playerteams.end()) \
	{ \
	const TPlayers &_players=_team->second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
	GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel, GetChannelId(*_player)); \
	} \
} \

#define GAMERULES_INVOKE_ON_TEAM_NOLOCAL(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=m_playerteams.end()) \
	{ \
	const TPlayers &_players=_team->second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
	GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel|eRMI_NoLocalCalls, GetChannelId(*_player)); \
	} \
} \


#define ACTOR_INVOKE_ON_TEAM(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=playerteams.end()) \
	{ \
	const TPlayers &_players=_team.second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
		{ \
		IActor *pActor=GetActorByEntityId(*_player); \
		if (pActor) \
		pActor->GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel, GetChannelId(*_player)); \
		} \
	} \
} \


#define ACTOR_INVOKE_ON_TEAM_NOLOCAL(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=playerteams.end()) \
	{ \
	const TPlayers &_players=_team.second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
		{ \
		IActor *pActor=GetActorByEntityId(*_player); \
		if (pActor) \
		pActor->GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel|eRMI_NoLocalCalls, GetChannelId(*_player)); \
		} \
	} \
} \

#define EQUIPMENT_LOADOUT_NUM_SLOTS     14

#define GAME_RULES_TOTAL_RMI_COUNT 45

class CGameRules :	public CGameObjectExtensionHelper<CGameRules, IGameRules, GAME_RULES_TOTAL_RMI_COUNT>, 
										public IViewSystemListener,
										public IGameFrameworkListener,
										public IHostMigrationEventListener,
										public IEntityEventListener
{
public:

	typedef std::vector<EntityId>								TPlayers;
	typedef std::vector<EntityId>								TSpawnLocations;
	typedef std::vector<EntityId>								TSpawnGroups;
	typedef std::map<EntityId, TSpawnLocations>	TSpawnGroupMap;
	typedef std::map<EntityId, int>							TBuildings;

	typedef struct TObjective
	{
		TObjective(): status(0), entityId(0) {};
		TObjective(int sts, EntityId eid): status(sts), entityId(eid) {};

		int				status;
		EntityId	entityId;

		void GetMemoryUsage( ICrySizer *pSizer ) const {/*nothing*/ }

	} TObjective;

	typedef std::map<string, TObjective> TObjectiveMap;
	typedef std::map<int, TObjectiveMap> TTeamObjectiveMap;

	typedef std::vector<SGameRulesListener*> TGameRulesListenerVec;

	typedef std::map<IEntity *, float> TExplosionAffectedEntities;

	enum ERadarTagReason
	{
		eRTR_General = 0,
		eRTR_AssaultBinoculars,
		eRTR_Last
	};

	// This structure contains the necessary information to create a new player
	// actor from a migrating one (a new player actor is created for each
	// reconnecting client and needs to be identical to the original actor on
	// the original server, or at least as close as possible)
	struct SMigratingPlayerInfo 
	{
		CryFixedStringT<HOST_MIGRATION_MAX_PLAYER_NAME_SIZE>	m_originalName;
		Vec3					m_pos;
		Ang3					m_ori;
		EntityId			m_originalEntityId;
		int						m_team;
		int						m_health;
		TNetChannelID	m_channelID;
		bool					m_inUse;
		//TODO: Health, inventory, currently selected weapon, ammo counts, suit mode, score

		SMigratingPlayerInfo() : m_inUse(false), m_channelID(0) {}

		void SetChannelID(uint16 id) { assert(id > 0); m_channelID = id; }

		void SetData(const char* inOriginalName, EntityId inOriginalEntityId, int inTeam, const Vec3& inPos, const Ang3& inOri, int inHealth)
		{
			m_originalName = inOriginalName;
			m_originalEntityId = inOriginalEntityId;
			m_team = inTeam;
			m_pos = inPos;
			m_ori = inOri;
			m_health = inHealth;

			m_inUse = true;
		}

		void Reset() { m_inUse = false; m_channelID = 0; }

		bool InUse() { return m_inUse; }
	};

	struct SHostMigrationItemInfo
	{
		SHostMigrationItemInfo()
		{
			Reset();
		}

		void Reset()
		{
			m_inUse = false;
		}

		void Set(EntityId itemId, EntityId ownerId, bool isUsed, bool isSelected)
		{
			m_itemId = itemId;
			m_ownerId = ownerId;
			m_isUsed = isUsed;
			m_isSelected = isSelected;

			m_inUse = true;
		}

		EntityId m_itemId;
		EntityId m_ownerId;
		bool m_isUsed;
		bool m_isSelected;

		bool m_inUse;
	};

	enum EHostMigrationState
	{
		eHMS_NotMigrating,
		eHMS_WaitingForPlayers,
		eHMS_Resuming,
	};

	CGameRules();
	virtual ~CGameRules();
	//IGameObjectExtension
	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject );
	virtual void InitClient(int channelId);
	virtual void PostInitClient(int channelId);
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	virtual void SerializeSpawnInfo( TSerialize ser ) {}
	virtual ISerializableInfoPtr GetSpawnInfo() {return 0;}
	virtual void Update( SEntityUpdateContext& ctx, int updateSlot );
	virtual void HandleEvent( const SGameObjectEvent& );
	virtual void ProcessEvent( SEntityEvent& );
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority( bool auth );
	virtual void PostUpdate( float frameTime );
	virtual void PostRemoteSpawn() {};
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;
	
	//~IGameObjectExtension

	// IViewSystemListener
	virtual bool OnBeginCutScene(IAnimSequence* pSeq, bool bResetFX);
	virtual bool OnEndCutScene(IAnimSequence* pSeq);
	virtual void OnPlayCutSceneSound(IAnimSequence* pSeq, ISound* pSound) {};
	virtual bool OnCameraChange(const SCameraParams& cameraParams){ return true; };
	// ~IViewSystemListener

	//IGameRules
	virtual bool ShouldKeepClient(int channelId, EDisconnectionCause cause, const char *desc) const;
	virtual void PrecacheLevel();
	virtual void OnConnect(struct INetChannel *pNetChannel);
	virtual void OnDisconnect(EDisconnectionCause cause, const char *desc); // notification to the client that he has been disconnected

	virtual bool OnClientConnect(int channelId, bool isReset);
	virtual void OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient);
	virtual bool OnClientEnteredGame(int channelId, bool isReset);

	virtual void OnEntitySpawn(IEntity *pEntity);
	virtual void OnEntityRespawn(IEntity *pEntity);
	virtual void OnEntityRemoved(IEntity *pEntity);
	
	virtual void OnItemDropped(EntityId itemId, EntityId actorId);
	virtual void OnItemPickedUp(EntityId itemId, EntityId actorId);

	virtual void OnPickupEntityAttached(EntityId entityId, EntityId actorId);
	virtual void OnPickupEntityDetached(EntityId entityId, EntityId actorId, bool isOnRemove);

	virtual void SendTextMessage(ETextMessageType type, const char *msg, uint32 to=eRMI_ToAllClients, int channelId=-1,
		const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0);
	virtual void SendChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg);
	virtual bool CanReceiveChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId) const;

#if !defined(_RELEASE)
	virtual void SendNetConsoleCommand(const char *msg, unsigned int to, int channelId = -1);
#endif

	virtual void ForbiddenAreaWarning(bool active, int timer, EntityId targetId);

	virtual void IncreasePoints(EntityId who, const SGameRulesScoreInfo & scoreInfo);

	virtual void ResetGameTime();
	virtual void SetRemainingGameTime(float seconds);
	virtual float GetRemainingGameTime() const;
	virtual float GetRemainingGameTimeNotZeroCapped() const;
	virtual float GetCurrentGameTime() const;
	virtual bool IsTimeLimited() const;

	virtual void ResetRoundTime();
	virtual float GetRemainingRoundTime() const;
	virtual bool IsRoundTimeLimited() const;

	virtual void ResetPreRoundTime();
	virtual float GetRemainingPreRoundTime() const;

	virtual void ResetReviveCycleTime();
	virtual float GetRemainingReviveCycleTime() const;

	virtual void ResetGameStartTimer(float time=-1);
	virtual float GetRemainingStartTimer() const;
	virtual float GetServerTime() const;

	virtual bool OnCollision(const SGameCollision& event);

	virtual void ClearAllMigratingPlayers(void);
	virtual void SetChannelForMigratingPlayer(const char* name, uint16 channelID);
	virtual void StoreMigratingPlayer(IActor* pActor);
	//~IGameRules

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* pNextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	// IHostMigrationEventListener
	virtual bool OnInitiate(SHostMigrationInfo& hostMigrationInfo);
	virtual bool OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo) { return true; }
	virtual bool OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo);
	virtual bool OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo);
	virtual bool OnReconnectClient(SHostMigrationInfo& hostMigrationInfo);
	virtual bool OnFinalise(SHostMigrationInfo& hostMigrationInfo);
	virtual bool OnTerminate(SHostMigrationInfo& hostMigrationInfo) { return true; }
	// ~IHostMigrationEventListener

	// IEntityEventListener
	virtual void OnEntityEvent(IEntity *pEntity, SEntityEvent &event);
	// ~IEntityEventListener

	void ClDoSetTeam(int teamId, EntityId entityId);

	void OnHostMigrationGotLocalPlayer(CPlayer *pPlayer);

	int GetScoreLimit() const { return m_scoreLimit; }
	int GetRoundLimit() const { return m_roundLimit; }
	float GetTimeLimit() const { return m_timeLimit; }

	int GetMigratingPlayerIndex(TNetChannelID channelID);

	virtual void RegisterConsoleCommands(IConsole *pConsole);
	virtual void UnregisterConsoleCommands(IConsole *pConsole);
	virtual void RegisterConsoleVars(IConsole *pConsole);
	void UnregisterConsoleVars(IConsole *pConsole);

	virtual void OnRevive(IActor *pActor, const Vec3 &pos, const Quat &rot, int teamId);
	virtual void OnReviveInVehicle(IActor *pActor, EntityId vehicleId, int seatId, int teamId);
	virtual void OnKill(IActor *pActor, EntityId shooterId, EntityId projectileId, const char *weaponClassName, int damage, int hit_joint, int hit_type);
	virtual void OnVehicleDestroyed(EntityId id);
	virtual void OnVehicleSubmerged(EntityId id, float ratio);
	virtual void OnTextMessage(ETextMessageType type, const char *msg,
		const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0);
	virtual void OnChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg, bool teamChatOnly);
	virtual void OnKillMessage(EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type);
	virtual void OnActorDeath( CActor* pActor );

	IActor *GetActorByChannelId(int channelId) const;
	IActor *GetActorByEntityId(EntityId entityId) const;
	const char *GetActorNameByEntityId(EntityId entityId) const;
	ILINE const char *GetActorName(IActor *pActor) const;
	ILINE CVotingSystem* GetVotingSystem() const { return m_pVotingSystem; };
	ILINE CBattlechatter* GetBattlechatter() const { return m_pBattlechatter; };
	ILINE CAreaAnnouncer* GetAreaAnnouncer() const { return m_pAreaAnnouncer; };

	int GetChannelId(EntityId entityId) const;
	ILINE int GetNumChannels() const { return m_channelIds.size(); }

	bool IsDead(EntityId entityId) const;
	bool IsSpectator(EntityId entityId) const;
	void ShowScores(bool show);

	//------------------------------------------------------------------------
	// player
	virtual IActor *SpawnPlayer(int channelId, const char *name, const char *className, const Vec3 &pos, const Ang3 &angles);
	virtual IActor *ChangePlayerClass(int channelId, const char *className);
	void RevivePlayerMP(IActor *pActor, IEntity *pSpawnPoint, int teamId=0, bool clearInventory=true);
	virtual void RevivePlayer(IActor *pActor, const Vec3 &pos, const Ang3 &angles, int teamId=0, bool clearInventory=true);
	virtual void RevivePlayerInVehicle(IActor *pActor, EntityId vehicleId, int seatId, int teamId=0, bool clearInventory=true);
	virtual void RenamePlayer(IActor *pActor, const char *name);
	virtual string VerifyName(const char *name, IEntity *pEntity=0);
	virtual bool IsNameTaken(const char *name, IEntity *pEntity=0);
	virtual void KillPlayer(IActor *pActor, bool dropItem, bool ragdoll, EntityId shooterId, EntityId weaponId, float damage,
													int hitJoint, int hit_type, const Vec3 &impulse, EntityId projectileId);
	virtual void MovePlayer(IActor *pActor, const Vec3 &pos, const Quat& orientation);
	virtual void ChangeTeam(IActor *pActor, int teamId);
	virtual void ChangeTeam(IActor *pActor, const char *teamName);
	//tagging time serialization limited to 0-60sec
	virtual void AddTaggedEntity(EntityId shooter, EntityId targetId, bool temporary = false, float time = 15.0f, ERadarTagReason reason = eRTR_General);
	virtual void RequestAddTempRadarEntity(EntityId shooter, EntityId targetId, float time = 15.0f);
	virtual int GetPlayerCount(bool inGame=false) const;
	int GetLivingPlayerCount() const;
	virtual int GetSpectatorCount(bool inGame=false) const;
	virtual EntityId GetPlayer(int idx);
	virtual void GetPlayers(TPlayers &players) const;
	virtual void GetPlayersClient(TPlayers &players) const;
	EntityId GetCoopPlayerHost() { return m_coopPlayerHost; }
	EntityId GetCoopPlayerClient() { return m_coopPlayerClient; }
	virtual bool IsPlayer( EntityId playerId ) const;
	virtual bool IsPlayerInGame(EntityId playerId) const;
	virtual bool IsPlayerActivelyPlaying(EntityId playerId, bool mustBeAlive=false) const;	// [playing / dead / waiting to respawn (inc spectating while dead): true] [not yet joined game / selected Spectate: false]
	virtual bool IsChannelInGame(int channelId) const;
  virtual void StartVoting(IActor *pActor, EVotingState t, EntityId id, const char* param);
  virtual void Vote(IActor *pActor, bool yes);
  virtual void EndVoting(bool success);
	int GetTotalAlivePlayerCount( const EntityId skipPlayerId ) const;
	bool CanPlayerSwitchItem( EntityId playerId );
	bool UseNanoSuit( EntityId playerId );
	bool RulesUseWeaponLoadouts();
	bool RulesUsePerkLoadouts();
	bool GetMigratedPlayerLocation(uint16 channelId, Vec3& pos, Ang3& ori);
	void SetupMigratedPlayer(uint16 channelId);

	//------------------------------------------------------------------------
	// teams
	virtual int CreateTeam(const char *name);
	virtual void RemoveTeam(int teamId);
	virtual const char *GetTeamName(int teamId) const;
	virtual int GetTeamId(const char *name) const;
	virtual int GetTeamCount() const;
	virtual int GetTeamPlayerCount(int teamId, bool inGame=false) const;
	virtual int GetTeamChannelCount(int teamId, bool inGame=false) const;
	virtual EntityId GetTeamPlayer(int teamId, int idx);
	EntityId GetTeamActivePlayer(int teamId, int idx) const;

	virtual int GetTeamsScore(int teamId) const;	// Can't be called GetTeamScore() because of TIA function. All gunna be refactored anyways.
	virtual void SetTeamsScore(int teamId, int score);
	virtual int  GetTeamRoundScore(int teamId) const;
	virtual void SetTeamRoundScore(int teamId, int score);

	virtual int SvGetTeamsScoreScoredThisRound(int teamId) const;
	virtual void SvCacheRoundStartTeamScores();
	virtual void ClientScoreEvent(EGameRulesScoreType scoreType, int points);

	virtual void GetTeamPlayers(int teamId, TPlayers &players);
	
	virtual void SetTeam(int teamId, EntityId entityId);
	virtual int GetTeam(EntityId entityId) const;
	virtual int GetChannelTeam(int channelId) const;

	//------------------------------------------------------------------------
	// objectives
	virtual void AddObjective(int teamId, const char *objective, int status, EntityId entityId);
	virtual void SetObjectiveStatus(int teamId, const char *objective, int status);
	virtual void SetObjectiveEntity(int teamId, const char *objective, EntityId entityId);
	virtual void RemoveObjective(int teamId, const char *objective);
	virtual void ResetObjectives();
	virtual TObjectiveMap *GetTeamObjectives(int teamId);
	virtual TObjective *GetObjective(int teamId, const char *objective);
	virtual void UpdateObjectivesForPlayer(int channelId, int teamId);

	//------------------------------------------------------------------------
	// materials
	virtual int RegisterHitMaterial(const char *materialName);
	virtual int GetHitMaterialId(const char *materialName) const;
	virtual ISurfaceType *GetHitMaterial(int id) const;
	virtual int GetHitMaterialIdFromSurfaceId(int surfaceId) const;
	virtual void ResetHitMaterials();

	//------------------------------------------------------------------------
	// hit type
	virtual int RegisterHitType(const char *type);
	virtual int GetHitTypeId(const char *type) const;
	virtual const char *GetHitType(int id) const;
	virtual const char *GetHitType(int id, const char* defaultValue) const;
	virtual int GetHitTypesCount() const;
	virtual void ResetHitTypes();

	//------------------------------------------------------------------------
	// freezing
	virtual bool IsFrozen(EntityId entityId) const;

//Diesel new
	//virtual void FreezeEntity(EntityId entityId, bool freeze, bool vapor, bool force=false); //Diesel new

	// spawn
	virtual void AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest);
	virtual void RemoveSpawnLocation(EntityId id, bool isBaseSpawn);
	virtual int GetSpawnLocationCount() const;
	virtual EntityId GetSpawnLocation(int idx) const;
#if 0
	virtual void GetSpawnLocations(TSpawnLocations &locations) const;
	virtual bool IsSpawnLocationSafe(EntityId playerId, EntityId spawnLocationId, float safeDistance, bool ignoreTeam, float zoffset) const;
	virtual bool IsSpawnLocationFarEnough(EntityId spawnLocationId, float minDistance, const Vec3 &testPosition) const;
	virtual bool TestSpawnLocationWithEnvironment(EntityId spawnLocationId, EntityId playerId, float offset=0.0f, float height=0.0f) const;
	virtual EntityId GetSpawnLocation(EntityId playerId, bool ignoreTeam, bool includeNeutral, EntityId groupId=0, float minDistToDeath=0.0f, const Vec3 &deathPos=Vec3(0,0,0), float *pZOffset=0) const;
#endif
	virtual EntityId GetFirstSpawnLocation(int teamId=0, EntityId groupId=0) const;

	float GetClosestEnemyDistSqrToPlayer(int playerTeamID, Vec3 pos) const;
	float GetClosestTeamMateDistSqrToPlayer(int playerTeamID, Vec3 pos) const;

#if 0
	EntityId GetSpawnLocationFurthestAwayFromAnyPlayer(EntityId playerId, const Vec3 &deathPos, float minDistToDeathSqr) const;
	EntityId GetSpawnLocationFurthestAwayFromEnemy(const TSpawnLocations &spawnLocations, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr) const;
	EntityId GetSpawnLocationClosestToTeamMate(const TSpawnLocations &spawnLocations, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr) const;
	int GetNumberOfBaseSpawnLocations(void) const;
	int GetNumberOfNonBaseSpawnLocations(void) const;
	EntityId GetSpawnLocationHQMode(EntityId playerId, const Vec3 &deathPos) const;
	int CalculateTeamFromEntity(int entityId) const;
	int CalculateConflicts(float entityRDCRadius, std::vector<EntityRDCGroup> *outResultGroups) const;
	EntityId GetSpawnLocationClosestToConflict(const TSpawnLocations &spawnLocations, const std::vector<EntityRDCGroup> &conflicts, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr) const;
	EntityId GetSpawnLocationFrontlineMode(EntityId playerId, const Vec3 &deathPos, bool useTeamSpawnPoints) const;
	void VisuallyTestFrontlineSpawnMode(void);

	EntityId GetSpawnLocationTeam(EntityId playerId, const Vec3 &deathPos) const;
	EntityId GetSpawnLocationTeamFirst() const;
	EntityId GetSpawnLocationNonTeamGame(EntityId playerId, const Vec3 &deathPos);
	EntityId GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos);
#endif

	EntityId GetSpawnLocationNew(EntityId playerId, float &zOffset);
	float GetMinEnemyDist() const;
	float GetClosestTeamMateDistSqr(int teamId, const Vec3& pos, EntityId skipId=-1) const;
	float GetClosestPlayerDistSqr(const EntityId spawnLocationId, const EntityId skipId) const;
	int GetEnemyTeamId(int myTeamId) const;
	bool IsSpawnUsed( const EntityId spawnId );

	//------------------------------------------------------------------------
	// spawn groups
	virtual void AddSpawnGroup(EntityId groupId);
	virtual void AddSpawnLocationToSpawnGroup(EntityId groupId, EntityId location);
	virtual void RemoveSpawnLocationFromSpawnGroup(EntityId groupId, EntityId location);
	virtual void RemoveSpawnGroup(EntityId groupId);
	virtual EntityId GetSpawnLocationGroup(EntityId spawnId) const;
	virtual int GetSpawnGroupCount() const;
	virtual EntityId GetSpawnGroup(int idx) const;
	virtual void GetSpawnGroups(TSpawnLocations &groups) const;
	virtual bool IsSpawnGroup(EntityId id) const;
	virtual bool AllowNullSpawnGroups() const;

	virtual void RequestSpawnGroup(EntityId spawnGroupId);
	virtual void SetPlayerSpawnGroup(EntityId playerId, EntityId spawnGroupId);
	virtual EntityId GetPlayerSpawnGroup(IActor *pActor);

	virtual void SetTeamDefaultSpawnGroup(int teamId, EntityId spawnGroupId);
	virtual EntityId GetTeamDefaultSpawnGroup(int teamId);
	virtual void CheckSpawnGroupValidity(EntityId spawnGroupId);

	//------------------------------------------------------------------------
	// game	
	virtual void Restart();
	virtual void NextLevel();
	virtual void ResetEntities();
	virtual void OnEndGame();
	virtual void EnteredGame();
	virtual void GameOver(EGameOverType localWinner);
	virtual void EndGameNear(EntityId id);
	virtual void ClientDisconnect_NotifyListeners( EntityId clientId );
	virtual void ClientEnteredGame_NotifyListeners( EntityId clientId );
	virtual void OnActorDeath_NotifyListeners( CActor* pActor );
	virtual void SvOnTimeLimitExpired_NotifyListeners();
	virtual void EntityRevived_NotifyListeners( EntityId entityId );
	virtual void SvSurvivorCountRefresh_NotifyListeners( int count, const EntityId survivors[], int numKills );
	virtual void ClPlayerStatsNetSerializeReadDeath_NotifyListeners(const SGameRulesPlayerStat* s, uint16 prevDeathsThisRound, uint8 prevFlags);
	virtual void OnRoundStart_NotifyListeners();
	virtual void ClRoundsNetSerializeReadState_NotifyListeners(int newState, int curState);
	virtual void ClientSimpleHit(const SimpleHitInfo &simpleHitInfo);
	virtual void ServerSimpleHit(const SimpleHitInfo &simpleHitInfo);
	virtual void KnockActorDown( EntityId actorEntityId );
	

  virtual void ClientHit(const HitInfo &hitInfo);
	virtual void ServerHit(const HitInfo &hitInfo);
	virtual void ProcessServerHit(const HitInfo &hitInfo);
	void ProcessLocalHit(const HitInfo& hitInfo);

	void AddLocalHitImpulse(const HitInfo& hitInfo);

	void CullEntitiesInExplosion(const ExplosionInfo &explosionInfo);
	virtual void ClientExplosion(ExplosionInfo &explosionInfo);
	virtual void QueueExplosion(const ExplosionInfo &explosionInfo);
	
	virtual void CreateEntityRespawnData(EntityId entityId);
	virtual bool HasEntityRespawnData(EntityId entityId) const;
	virtual void ScheduleEntityRespawn(EntityId entityId, bool unique, float timer);
	virtual void AbortEntityRespawn(EntityId entityId, bool destroyData);
	void DoEntityRespawn(EntityId id);

	virtual void ScheduleEntityRemoval(EntityId entityId, float timer, bool visibility);
	virtual void AbortEntityRemoval(EntityId entityId);

	virtual void UpdateEntitySchedules(float frameTime);
	void FlushEntitySchedules();
  virtual void ProcessQueuedExplosions();
	virtual void ProcessServerExplosion(ExplosionInfo &explosionInfo);
	
	virtual void ForceScoreboard(bool force);
	virtual void FreezeInput(bool freeze);

	virtual bool IsProjectile(EntityId id) const;

	virtual void ShowStatus();

	void UpdateTeamPerkCount(int teamId, ETeamPerks teamPerkID, EntityId activatorId, int change);

	CTeamPerks *GetTeamPerks(int teamId);

	IPhysicalEntity* GetEntityToSkipByExplosion(const ExplosionInfo& explosionInfo) const;
	void	NotifyPingExplosion(const ExplosionInfo& explosionInfo);

	ILINE CEquipmentLoadout *GetEquipmentLoadout() const { return m_pEquipmentLoadout; }

#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	void ReconfigureVoiceGroups(EntityId id,int old_team,int new_team);
#endif

	float GameRulesPlayerSpeedAdjustment(EntityId playerId);

	CBattleDust* GetBattleDust() const;
	CMPTutorial* GetMPTutorial() const;

	int GetCurrentStateId() const { return m_currentStateId; }
	
	//misc 
	// Next time CGameRules::OnCollision is called, it will skip this entity and return false
	// This will prevent squad mates to be hit by the player
	void SetEntityToIgnore(EntityId id) { m_ignoreEntityNextCollision = id;}

	template<typename T>
	void SetSynchedGlobalValue(TSynchedKey key, const T &value)
	{
		assert(gEnv->bServer);
		g_pGame->GetSynchedStorage()->SetGlobalValue(key, value);
	};

	template<typename T>
	bool GetSynchedGlobalValue(TSynchedKey key, T &value)
	{
		if (!g_pGame->GetSynchedStorage())
			return false;
		return g_pGame->GetSynchedStorage()->GetGlobalValue(key, value);
	}

	int GetSynchedGlobalValueType(TSynchedKey key) const
	{
		if (!g_pGame->GetSynchedStorage())
			return eSVT_None;
		return g_pGame->GetSynchedStorage()->GetGlobalValueType(key);
	}

	template<typename T>
	void SetSynchedEntityValue(EntityId id, TSynchedKey key, const T &value)
	{
		assert(gEnv->bServer);
		g_pGame->GetSynchedStorage()->SetEntityValue(id, key, value);
	}
	template<typename T>
	bool GetSynchedEntityValue(EntityId id, TSynchedKey key, T &value)
	{
		return g_pGame->GetSynchedStorage()->GetEntityValue(id, key, value);
	}
	
	int GetSynchedEntityValueType(EntityId id, TSynchedKey key) const
	{
		return g_pGame->GetSynchedStorage()->GetEntityValueType(id, key);
	}

	void ResetSynchedStorage()
	{
		g_pGame->GetSynchedStorage()->Reset();
	}

	void ForceSynchedStorageSynch(int channel);


	void PlayerPosForRespawn(CPlayer* pPlayer, bool save);
	void SPNotifyPlayerKill(EntityId targetId, EntityId weaponId, bool bHeadShot);

	//compare gamerules class (replace by enum)
	bool IsGameRulesClass(const char *cls);

	bool IsMultiplayerDeathmatch()
	{
		return gEnv->bMultiplayer && !IsGameRulesClass("Coop");
	}

	bool IsMultiplayerCampaign()
	{
		return gEnv->bMultiplayer && IsGameRulesClass("Coop");
	}

	struct ChatMessageParams
	{
		uint8 type;
		EntityId sourceId;
		EntityId targetId;
		string msg;
		bool onlyTeam;

		ChatMessageParams() {};
		ChatMessageParams(EChatMessageType _type, EntityId src, EntityId trg, const char *_msg, bool _onlyTeam)
		: type(_type),
			sourceId(src),
			targetId(trg),
			msg(_msg),
			onlyTeam(_onlyTeam)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("type", type, 'ui3');
			ser.Value("source", sourceId, 'eid');
			if (type == eChatToTarget)
				ser.Value("target", targetId, 'eid');
			ser.Value("message", msg);
			ser.Value("onlyTeam", onlyTeam, 'bool');
		}
	};

	struct ForbiddenAreaWarningParams
	{
		int timer;
		bool active;
		ForbiddenAreaWarningParams() {};
		ForbiddenAreaWarningParams(bool act, int time) : active(act), timer(time)
		{}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("active", active, 'bool');
			ser.Value("timer", timer, 'ui5');
		}
	};

	struct BoolParam
	{
		bool success;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("success", success, 'bool');
		}
	};

	struct TextMessageParams
	{
		uint8	type;
		string msg;

		uint8 nparams;
		string params[4];

		TextMessageParams() {};
		TextMessageParams(ETextMessageType _type, const char *_msg)
		: type(_type),
			msg(_msg),
			nparams(0)
		{
		};
		TextMessageParams(ETextMessageType _type, const char *_msg, 
			const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0)
		: type(_type),
			msg(_msg),
			nparams(0)
		{
			if (!AddParam(p0)) return;
			if (!AddParam(p1)) return;
			if (!AddParam(p2)) return;
			if (!AddParam(p3)) return;
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("type", type, 'ui3');
			ser.Value("message", msg);
			ser.Value("nparams", nparams, 'ui3');

			for (int i=0;i<nparams; ++i)
				ser.Value("param", params[i]);
		}

		bool AddParam(const char *param)
		{
			if (!param || nparams>3)
				return false;
			params[nparams++]=param;
			return true;
		}
	};

	struct NetConsoleCommandParams
	{
		string m_commandString;

		NetConsoleCommandParams() {};
		NetConsoleCommandParams(const char * cmdIn) : m_commandString (cmdIn) {}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("cmd", m_commandString);
		}
	};

	struct SetTeamParams
	{
		int				teamId;
		EntityId	entityId;

		SetTeamParams() {};
		SetTeamParams(EntityId _entityId, int _teamId)
		: entityId(_entityId),
			teamId(_teamId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("teamId", teamId, 'team');
		}
	};

	struct ChangeTeamParams
	{
		EntityId	entityId;
		int				teamId;

		ChangeTeamParams() {};
		ChangeTeamParams(EntityId _entityId, int _teamId)
			: entityId(_entityId),
				teamId(_teamId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("teamId", teamId, 'team');
		}
	};

	struct SpectatorModeParams
	{
		EntityId	entityId;
		uint8			mode;
		EntityId	targetId;
		bool			resetAll;

		SpectatorModeParams() {};
		SpectatorModeParams(EntityId _entityId, uint8 _mode, EntityId _target, bool _reset)
			: entityId(_entityId),
				mode(_mode),
				targetId(_target),
				resetAll(_reset)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("mode", mode, 'ui3');
			ser.Value("targetId", targetId, 'eid');
			ser.Value("resetAll", resetAll, 'bool');
		}
	};

	struct RenameEntityParams
	{
		EntityId	entityId;
		string		name;

		RenameEntityParams() {};
		RenameEntityParams(EntityId _entityId, const char *name)
			: entityId(_entityId),
				name(name)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("name", name);
		}
	};

	struct PostInitParams
	{
		PostInitParams() {}
		PostInitParams(const CTimeValue &_gameStartedTime, const CTimeValue &_roundEndTime, const CTimeValue &_preRoundEndTime, CTimeValue &_reviveCycleEndTime) :
			gameStartedTime(_gameStartedTime), roundEndTime(_roundEndTime), preRoundEndTime(_preRoundEndTime), reviveCycleEndTime(_reviveCycleEndTime) {}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("gameStartedTime", gameStartedTime, 'tnet');
			ser.Value("roundEndTime", roundEndTime, 'tnet');
			ser.Value("preRoundEndTime", preRoundEndTime, 'tnet');
			ser.Value("reviveCycleEndTime", reviveCycleEndTime, 'tnet');
		}

		CTimeValue gameStartedTime;
		CTimeValue roundEndTime;
		CTimeValue preRoundEndTime;
		CTimeValue reviveCycleEndTime;
	};

	struct SetGameTimeParams
	{
		CTimeValue time;

		SetGameTimeParams() {};
		SetGameTimeParams(const CTimeValue& _time)
		: time(_time)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("time", time, 'tnet');
		}
	};

	struct VictoryTeamParams
	{
		int winningTeamId;
		uint8 reason; // EGameOverReason

		VictoryTeamParams() {};
		VictoryTeamParams(int _winningTeamId, uint8 _reason)
		: winningTeamId(_winningTeamId), reason(_reason)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("team", winningTeamId, 'team');
			ser.Value("reason", reason, 'ui2');
		}
	};

	struct VictoryPlayerParams
	{
		EntityId playerId;
		uint8 reason; // EGameOverReason

		VictoryPlayerParams() {};
		VictoryPlayerParams(EntityId _playerId, uint8 _reason)
			: playerId(_playerId), reason(_reason)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("playerId", playerId, 'eid');
			ser.Value("reason", reason, 'ui2');
		}
	};

  struct StartVotingParams
  {
    string        param;
    EntityId      entityId;
    EVotingState  vote_type;
    StartVotingParams(){}
    StartVotingParams(EVotingState st, EntityId id, const char* cmd):vote_type(st),entityId(id),param(cmd){}
    void SerializeWith(TSerialize ser)
    {
      ser.EnumValue("type",vote_type,eVS_none,eVS_last);
      ser.Value("entityId",entityId,'eid');
      ser.Value("param",param);
    }
  };

  struct VotingStatusParams
  {
    EVotingState  state;
    int           timeout;
    EntityId      entityId;
    string        description;
    VotingStatusParams(){}
    VotingStatusParams(EVotingState s, int t, EntityId e, const char* d):state(s),timeout(t),entityId(e),description(d){}
    void SerializeWith(TSerialize ser)
    {
      ser.EnumValue("state", state, eVS_none, eVS_last);
      ser.Value("timeout", timeout, 'ui8');
      ser.Value("entityId", entityId,'eid');
      ser.Value("description", description);
    }
  };

	struct EntityParams
	{
		EntityId entityId;
		EntityParams() {};
		EntityParams(EntityId entId)
		: entityId(entId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
		}
	};

	// TODO: Roll this and SGameRulesScoreInfo into one structure!
	struct ScoreChangeParams
	{
		EntityId m_killedEntityId;
		TGameRulesScoreInt m_changeToScore;
		EGameRulesScoreType m_type;

		ScoreChangeParams() {};
		ScoreChangeParams(EntityId killedEntId, TGameRulesScoreInt changeToScore, EGameRulesScoreType theType) :
			m_killedEntityId(killedEntId),
			m_changeToScore(changeToScore),
			m_type(theType)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", m_killedEntityId, 'eid');   // 'eid' = special policy for serializing entity IDs
			ser.Value("addToScore", m_changeToScore, 'i16');	// 'i16' = can serialize values in the range -32,768 to -32,767
			ser.Value("type", m_type, 'ui5');                 // 'ui5' = can serialize values in the range 0 to 31
		}
	};

	struct NoParams
	{
		NoParams() {};
		void SerializeWith(const TSerialize& ser) {};
	};


	struct SpawnGroupParams
	{
		EntityId entityId;
		SpawnGroupParams() {};
		SpawnGroupParams(EntityId entId)
			: entityId(entId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct SetObjectiveParams
	{
		SetObjectiveParams(): status(0), entityId(0) {};
		SetObjectiveParams(const char *nm, int st, EntityId id): name(nm), status(st), entityId(id) {};

		EntityId entityId;
		int status;
		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
			ser.Value("status", status, 'hSts');
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct TempRadarTaggingParams
	{
		EntityId  shooterId;
		EntityId  targetId;
		float			m_time;
		ERadarTagReason		m_reason;
		TempRadarTaggingParams() : shooterId(0), targetId(0), m_time(0.0f), m_reason(eRTR_General) {};
		TempRadarTaggingParams(EntityId shtId, EntityId tgtId, float time = 15.0f, ERadarTagReason reason = eRTR_General)
			: shooterId(shtId), targetId(tgtId), m_time(time), m_reason(reason)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("shooterId", shooterId, 'eid');
			ser.Value("targetId", targetId, 'eid');
			ser.Value("time", m_time, 'tm63');
			ser.EnumValue("reason",m_reason,eRTR_General,eRTR_Last);
		}
	};

	struct InformTaggedByCCTVParams
	{
		EntityId  taggerId;
		InformTaggedByCCTVParams() : taggerId(0) {};
		InformTaggedByCCTVParams(EntityId tgrId)
			: taggerId(tgrId)
		{
		}
		void SerializeWith(TSerialize ser)
		{
			ser.Value("taggerId", taggerId, 'eid');
		}
	};

	struct SetObjectiveStatusParams
	{
		SetObjectiveStatusParams(): status(0) {};
		SetObjectiveStatusParams(const char *nm, int st): name(nm), status(st) {};
		int status;
		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
			ser.Value("status", status, 'hSts');
		}
	};

	struct SetObjectiveEntityParams
	{
		SetObjectiveEntityParams(): entityId(0) {};
		SetObjectiveEntityParams(const char *nm, EntityId id): name(nm), entityId(id) {};

		EntityId entityId;
		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct RemoveObjectiveParams
	{
		RemoveObjectiveParams() {};
		RemoveObjectiveParams(const char *nm): name(nm) {};

		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
		}
	};

	struct DamageIndicatorParams
	{
		DamageIndicatorParams() {};
		DamageIndicatorParams(EntityId shtId, EntityId wpnId, const Vec3 &inDir, float inDamage, uint16 inProjectileClassId, uint8 inHitTypeId)
				: shooterId(shtId), weaponId(wpnId), dir(inDir), damage(inDamage), projectileClassId(inProjectileClassId), hitTypeId(inHitTypeId) {};

		Vec3 dir;
		EntityId shooterId;
		EntityId weaponId;
		float damage;
		uint16 projectileClassId;
		uint8 hitTypeId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("shooterId", shooterId, 'eid');
			ser.Value("weaponId", weaponId, 'eid');
			ser.Value("dir", dir, 'dir1');
			ser.Value("damage", damage, 'dmg');
			ser.Value("projectileClassId", projectileClassId, 'ui16');
			ser.Value("hitTypeId", hitTypeId, 'ui8');
		}
	};
	
	// used in the RMI that send movies (trackview) synq info to the client
  struct TSynqMoviesParams
  {
    struct TPlayingMovie
    {
      string    m_Name;
      float     m_fTime;
    };
    
    std::vector<TPlayingMovie>  m_aPlayingMovies;  
    
    TSynqMoviesParams() {}
    
    void SerializeWith(TSerialize ser)
    {
      int iNumPlayingMovies = m_aPlayingMovies.size();
      ser.Value( "Num", iNumPlayingMovies );
      
      m_aPlayingMovies.resize( iNumPlayingMovies );

      for (int i=0; i<m_aPlayingMovies.size(); ++i)
      {
        ser.Value( "Name", m_aPlayingMovies[i].m_Name );
        ser.Value( "Time", m_aPlayingMovies[i].m_fTime );
      }
    }
  };


	// used in the RMI that send movies (trackview) synq info to the client
  struct TFinishedOnLoadMoviesParams
  {
    std::vector<string> m_aMovies;
    
    TFinishedOnLoadMoviesParams() {}
    
    void SerializeWith(TSerialize ser)
    {
      int iNumMovies = m_aMovies.size();
      ser.Value( "Num", iNumMovies );
      
      m_aMovies.resize( iNumMovies );
      
      for (int i=0; i<m_aMovies.size(); ++i)
      {
        string& s = m_aMovies[i];
        ser.Value( "Name", s );
      }
    }
  };

  struct EquipmentLoadoutParams
  {
		uint8 m_contents[EQUIPMENT_LOADOUT_NUM_SLOTS];

	  EquipmentLoadoutParams()
	  {
			memset(&m_contents,0,sizeof(m_contents));
		}

	  void SerializeWith(TSerialize ser)
	  {
			string name;
			for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
			{
				name.Format("slot%d", i);
				ser.Value(name.c_str(), m_contents[i], 'ui8');
			}
	  }
  };

	struct SModuleRMIEntityParams
	{
		EntityId m_entityId;
		int m_listenerIndex;
		int m_data;

		SModuleRMIEntityParams() {};
		SModuleRMIEntityParams(int listenerIndex, EntityId entId, int data)
				: m_listenerIndex(listenerIndex), m_entityId(entId), m_data(data)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("listenerIndex", m_listenerIndex, 'ui8');
			ser.Value("entityId", m_entityId, 'eid');
			ser.Value("data", m_data, 'ui8');
		}
	};

	struct SModuleRMITwoEntityParams
	{
		EntityId m_entityId1;
		EntityId m_entityId2;
		int m_listenerIndex;
		int m_data;

		SModuleRMITwoEntityParams() {};
		SModuleRMITwoEntityParams(int listenerIndex, EntityId entId1, EntityId entId2, int data)
				: m_listenerIndex(listenerIndex), m_entityId1(entId1), m_entityId2(entId2), m_data(data)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("listenerIndex", m_listenerIndex, 'ui8');
			ser.Value("entityId1", m_entityId1, 'eid');
			ser.Value("entityId2", m_entityId2, 'eid');
			ser.Value("data", m_data, 'ui8');
		}
	};

	struct SHostMigrationClientRequestParams
	{
		SHostMigrationClientRequestParams()
		{
			m_timeToAutoRevive = 0.f;
		}

		void SerializeWith(TSerialize ser)
		{
			m_loadoutParams.SerializeWith(ser);

			ser.Value("timeToAutoRevive", m_timeToAutoRevive, 'fsec');
		}

		CGameRules::EquipmentLoadoutParams m_loadoutParams;
		float m_timeToAutoRevive;
	};

	struct SHostMigrationClientControlledParams
	{
		SHostMigrationClientControlledParams()
		{
			m_pAmmoParams = NULL;
			m_doneEnteredGame = false;
			m_doneSetAmmo = false;
		}

		~SHostMigrationClientControlledParams()
		{
			SAFE_DELETE_ARRAY(m_pAmmoParams);
		}

		bool IsDone()
		{
			return (m_doneEnteredGame && m_doneSetAmmo);
		}

		struct SAmmoParams
		{
			IEntityClass *m_pAmmoClass;
			int m_count;
		};

		Quat m_viewQuat;
		Vec3 m_position;		// Save this since the new server may not have it stored correctly (lag dependent)

		SAmmoParams *m_pAmmoParams;

		int m_numAmmoParams;
		int m_numExpectedItems;

		bool m_doneEnteredGame;
		bool m_doneSetAmmo;
	};

	struct SPredictionParams
	{
		int predictionHandle;

		SPredictionParams() : predictionHandle(0) {}
		SPredictionParams(int hndl) : predictionHandle(hndl) {}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		}
	};

	struct SMidMigrationJoinParams
	{
		SMidMigrationJoinParams() : m_state(0), m_timeSinceStateChanged(0.f) {}
		SMidMigrationJoinParams(int state, float timeSinceStateChanged) : m_state(state), m_timeSinceStateChanged(timeSinceStateChanged) {}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("state", m_state, 'ui2');
			ser.Value("timeSinceStateChanged", m_timeSinceStateChanged, 'fsec');
		}

		int m_state;
		float m_timeSinceStateChanged;
	};

	DECLARE_SERVER_RMI_NOATTACH(SvRequestSimpleHit, SimpleHitInfo, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestHit, HitInfo, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClExplosion, ExplosionInfo, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestChatMessage, ChatMessageParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClChatMessage, ChatMessageParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClTaggedEntity, EntityParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestTempRadarEntity, TempRadarTaggingParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClTempRadarEntity, TempRadarTaggingParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClInformTaggedByCCTV, InformTaggedByCCTVParams, eNRT_UnreliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClBoughtItem, BoolParam, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestRename, RenameEntityParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRenameEntity, RenameEntityParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestChangeTeam, ChangeTeamParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSpectatorMode, SpectatorModeParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetTeam, SetTeamParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClTextMessage, TextMessageParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClSetObjective, SetObjectiveParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetObjectiveStatus, SetObjectiveStatusParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetObjectiveEntity, SetObjectiveEntityParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClDamageIndicator, DamageIndicatorParams, eNRT_UnreliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClPostInit, PostInitParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetGameStartedTime, SetGameTimeParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetRoundTime, SetGameTimeParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetPreRoundTime, SetGameTimeParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetReviveCycleTime, SetGameTimeParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetGameStartTimer, SetGameTimeParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvVote, NoParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvVoteNo, NoParams, eNRT_ReliableUnordered);
  DECLARE_SERVER_RMI_NOATTACH(SvStartVoting, StartVotingParams, eNRT_ReliableUnordered);
  DECLARE_CLIENT_RMI_NOATTACH(ClVotingStatus, VotingStatusParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClEnteredGame, NoParams, eNRT_ReliableUnordered);
	
	DECLARE_CLIENT_RMI_NOATTACH( ClVictoryTeam,           VictoryTeamParams,            eNRT_ReliableOrdered );
	DECLARE_CLIENT_RMI_NOATTACH( ClVictoryPlayer,         VictoryPlayerParams,          eNRT_ReliableOrdered );

	DECLARE_CLIENT_RMI_NOATTACH(ClAddPoints,      	      ScoreChangeParams,            eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestRevive, EntityParams, eNRT_ReliableUnordered);
	
	DECLARE_CLIENT_RMI_NOATTACH( ClSetServerOverrideLoadout,EquipmentLoadoutParams,       eNRT_ReliableOrdered );
	DECLARE_SERVER_RMI_NOATTACH( SvSetEquipmentLoadout,EquipmentLoadoutParams,       eNRT_ReliableOrdered );

	// This is a small hack that makes sure all objective entities are listed in the same order on all clients
	DECLARE_CLIENT_RMI_NOATTACH( ClModuleRMISingleEntity,	SModuleRMIEntityParams,			eNRT_ReliableOrdered );
	DECLARE_CLIENT_RMI_NOATTACH( ClModuleRMIDoubleEntity,	SModuleRMITwoEntityParams,		eNRT_ReliableOrdered );

	DECLARE_SERVER_RMI_NOATTACH(SvAutoReviveOptOut, EntityParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvSuccessfulFlashBang, EntityParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClNetConsoleCommand, NetConsoleCommandParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvHostMigrationRequestSetup, SHostMigrationClientRequestParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClHostMigrationFinished, NoParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClMidMigrationJoin, SMidMigrationJoinParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClPredictionFailed, SPredictionParams, eNRT_ReliableUnordered);

	virtual void AddHitListener(IHitListener* pHitListener);
	virtual void RemoveHitListener(IHitListener* pHitListener);

	virtual void AddGameRulesListener(SGameRulesListener* pRulesListener);
	virtual void RemoveGameRulesListener(SGameRulesListener* pRulesListener);

	struct STeamScore
	{
		STeamScore() {};
		STeamScore(uint16 teamScore, uint16 roundTeamScore)
			: m_teamScore(teamScore), m_roundTeamScore(roundTeamScore), m_teamScoreRoundStart(teamScore) {}
		STeamScore(uint16 teamScore, uint16 roundTeamScore, uint16 teamScoreRoundStart )
			: m_teamScore(teamScore), m_roundTeamScore(roundTeamScore), m_teamScoreRoundStart(teamScoreRoundStart) {}
		uint16						m_teamScore;
		uint16						m_roundTeamScore;
		uint16						m_teamScoreRoundStart;
	};

	typedef std::map<int, EntityId>				TTeamIdEntityIdMap;
	typedef std::map<EntityId, int>				TEntityTeamIdMap;
	typedef std::map<int, TPlayers>				TPlayerTeamIdMap;
	typedef std::map<int, EntityId>				TChannelTeamIdMap;
	typedef std::map<string, int>				TTeamIdMap;
	typedef std::map<int, STeamScore>			TTeamScoresMap;
	typedef std::map<int, CTeamPerks>			TTeamPerkMap;

	typedef std::map<int, int>						THitMaterialMap;
	typedef std::map<int, string>					THitTypeMap;

#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	typedef std::map<int, _smart_ptr<IVoiceGroup> >		TTeamIdVoiceGroupMap;
#endif

	typedef struct SCollisionHitInfo
	{
		Vec3		pos;
		Vec3		normal;
		Vec3		dir;
		Vec3		velocity;

		Vec3		target_velocity;
		float		target_mass;
		EntityId	targetId;
		pe_type		target_type;

		int			materialId;
		bool		dir_null;
		bool		backface;

		SCollisionHitInfo()
			: pos(0,0,0),
			normal(0,0,0),
			dir(0,0,0),
			velocity(0,0,0),
			target_velocity(0,0,0),
			target_mass(0.f),
			targetId(0),
			target_type(PE_NONE),
			materialId(0),
			backface(false),
			dir_null(false)
		{}
	};

	typedef struct SEntityRespawnData
	{
		SmartScriptTable	properties;
		Vec3							position;
		Quat							rotation;
		Vec3							scale;
		int								flags;
		IEntityClass			*pClass;

#ifdef _DEBUG
		string						name;
#endif

		void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing*/ }
	};

	typedef struct SEntityRespawn
	{
		bool							unique;
		float							timer;	

		void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing*/ }
	};

	typedef struct SEntityRemovalData
	{
		float							timer;
		float							time;
		bool							visibility;

		void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing*/ }
	};

	typedef std::map<EntityId, SEntityRespawnData>	TEntityRespawnDataMap;
	typedef std::map<EntityId, SEntityRespawn>			TEntityRespawnMap;
	typedef std::map<EntityId, SEntityRemovalData>	TEntityRemovalMap;

	typedef std::vector<IHitListener*> THitListenerVec;

	const SEntityRespawnData *GetEntityRespawnData(EntityId entityId) const;

protected:
	static void CmdDebugSpawns(IConsoleCmdArgs *pArgs);
	static void CmdDebugTeams(IConsoleCmdArgs *pArgs);
	static void CmdDebugObjectives(IConsoleCmdArgs *pArgs);
	static void CmdGiveScore(IConsoleCmdArgs *pArgs);

	void InitSessionStatistics();
	void SaveSessionStatistics();
	bool NetSerializeTelemetry( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
public:
	static const char** S_GetGameModeNamesArray() { return s_gameModeNames; }
	ILINE EGameMode GetGameMode() const { return m_gameMode; }

	void CreateScriptHitInfo(SmartScriptTable &scriptHitInfo, const HitInfo &hitInfo);
	void CreateScriptExplosionInfo(SmartScriptTable &scriptExplosionInfo, const ExplosionInfo &explosionInfo);
	
	void SuccessfulFlashBang(const ExplosionInfo &explosionInfo);

#ifdef _DEBUG
	static inline bool DbgSetAssertOnFailureToFindHitType(bool onOff)
	{
		bool old = s_dbgAssertOnFailureToFindHitType;
		s_dbgAssertOnFailureToFindHitType = onOff;
		return old;
	}
#endif

protected:
	void UpdateAffectedEntitiesSet(TExplosionAffectedEntities &affectedEnts, const pe_explosion *pExplosion);
	void AddOrUpdateAffectedEntity(TExplosionAffectedEntities &affectedEnts, IEntity* pEntity, float affected);
	void CommitAffectedEntitiesSet(SmartScriptTable &scriptExplosionInfo, TExplosionAffectedEntities &affectedEnts);
	void RemoveFriendlyAffectedEntities(const ExplosionInfo &explosionInfo, TExplosionAffectedEntities &affectedEntities);
	void ChatLog(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg);

	void UpdateCoopPlayersIds();
	void KnockBackPendingActors();

	// fill source/target dependent params in m_collisionTable
public:
	void PrepCollisionForScript(const SCollisionHitInfo& colHitInfo, SmartScriptTable& collisionTable);
protected:
	void PrepCollision(int src, int trg, const SGameCollision& event, IEntity* pTarget, SCollisionHitInfo &result);

	void CallScript(IScriptTable *pScript, const char *name)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->EndCall();
	};
	template<typename P1>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2, const P3 &p3)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3, typename P4>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3); m_pScriptSystem->PushFuncParam(p4);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3, typename P4, typename P5>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3); m_pScriptSystem->PushFuncParam(p4); m_pScriptSystem->PushFuncParam(p5);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	void CallScript(IScriptTable *pScript, const char *name, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3); m_pScriptSystem->PushFuncParam(p4); m_pScriptSystem->PushFuncParam(p5); m_pScriptSystem->PushFuncParam(p6);
		m_pScriptSystem->EndCall();
	};

	void FinishHostMigrating();
	void OnResumeAfterHostMigrating();
	void GiveItemToMigratingPlayer(IItemSystem *pItemSystem, SHostMigrationItemInfo *pItemInfo);

	static const char* s_gameModeNames[];
	EGameMode m_gameMode;

	IGameFramework			*m_pGameFramework;
	IGameplayRecorder		*m_pGameplayRecorder;
	ISystem							*m_pSystem;
	IActorSystem				*m_pActorSystem;
	IEntitySystem				*m_pEntitySystem;
	IScriptSystem				*m_pScriptSystem;
	IMaterialManager		*m_pMaterialManager;
	SmartScriptTable		m_script;
	SmartScriptTable		m_clientScript;
	SmartScriptTable		m_serverScript;
	SmartScriptTable		m_clientStateScript;
	SmartScriptTable		m_serverStateScript;
	HSCRIPTFUNCTION			m_onCollisionFunc;
	SmartScriptTable		m_collisionTable;
	SmartScriptTable		m_collisionTableSource;
	SmartScriptTable		m_collisionTableTarget;
	CScoreRewardTable		m_scoreRewardTable;

	INetChannel					*m_pClientNetChannel;

	std::vector<int>		m_channelIds;
	
	TTeamIdMap				m_teams;
	TEntityTeamIdMap		m_entityteams;
	TTeamIdEntityIdMap		m_teamdefaultspawns;
	TPlayerTeamIdMap		m_playerteams;
	TChannelTeamIdMap		m_channelteams;
	TTeamScoresMap			m_teamscores;
	TTeamPerkMap			m_teamperks;
	int						m_teamIdGen;

	THitMaterialMap			m_hitMaterials;
	int									m_hitMaterialIdGen;

	THitTypeMap					m_hitTypes;
	int									m_hitTypeIdGen;

	SmartScriptTable		m_scriptHitInfo;
	SmartScriptTable		m_scriptExplosionInfo;
  
  typedef std::queue<ExplosionInfo*>	TExplosionPtrQueue;
  TExplosionPtrQueue    m_queuedExplosions;
	TExplosionPtrQueue    m_queuedExplosionsAwaitingLinetests;
	ExplosionInfo					m_explosions[MAX_CONCURRENT_EXPLOSIONS];
	bool									m_explosionValidities[MAX_CONCURRENT_EXPLOSIONS];

	int										m_numConcurrentExplosions;

	typedef std::queue<HitInfo> THitQueue;
	THitQueue						m_queuedHits;
	int									m_processingHit;

	TEntityRespawnDataMap	m_respawndata;
	TEntityRespawnMap			m_respawns;
	TEntityRemovalMap			m_removals;

	TTeamObjectiveMap		m_objectives;

#if 0
	TSpawnLocations			m_spawnLocations;
	TSpawnLocations			m_baseSpawnLocations;
	TSpawnLocations			m_nonBaseSpawnLocations;
#endif
	TSpawnGroupMap			m_spawnGroups;

	// need this to mark all the used spawn-points to fix issue on restarting level
	// when players are placed at the same location, some may end up underground
	typedef std::map< EntityId, float > TSpawnPointUseTime;
	TSpawnPointUseTime m_SpawnPointUseTime;

	int									m_currentStateId;
	float								m_lastHitFeedback;

	THitListenerVec     m_hitListeners;

	CTimeValue					m_gameStartedTime;	// time the game started at.
	CTimeValue					m_roundEndTime;	// time the round will end. 0 for unlimited
	CTimeValue					m_preRoundEndTime;	// time the pre round will end. 0 for no preround
	CTimeValue					m_reviveCycleEndTime; // time for reinforcements.
	CTimeValue					m_gameStartTime; // time for game start, <= 0 means game started already
	CTimeValue					m_gameEndedTime; // Time game went into postmatch - 0.0f if not happened yet. Set locally on game end, if time starts to be too inaccurate across client/server consider net syncing
	CTimeValue					m_cachedServerTime; // server time as of the last call to CGameRules::Update(...)
	float						m_timeLimit;
	int							m_scoreLimit;
	int							m_roundLimit;

	CEquipmentLoadout               *m_pEquipmentLoadout;

#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	TTeamIdVoiceGroupMap	m_teamVoiceGroups;
#endif

	CBattleDust					*m_pBattleDust;
	CMPTutorial					*m_pMPTutorial;
  CVotingSystem       *m_pVotingSystem;
	CPerk								*m_pPerkInstance;
	CBattlechatter      *m_pBattlechatter;
	CAreaAnnouncer			*m_pAreaAnnouncer;
	CExplosionGameEffect	*m_pExplosionGameEffect;

	TGameRulesListenerVec	m_rulesListeners;
	static int					s_invulnID;
	static int          s_barbWireID;

#ifdef _DEBUG
	static bool         s_dbgAssertOnFailureToFindHitType;
#endif

	EntityId					  m_ignoreEntityNextCollision;

	bool                m_timeOfDayInitialized;

	EntityId						m_coopPlayerHost;
	EntityId						m_coopPlayerClient;
	std::vector<EntityId> m_pendingActorsToBeKnockedDown;
 
  // constants used in the movie synq code
  enum 
  {	
    NUM_FRAMES_CHECKING_MOVIES_SYNQ = 20,       // how many frames the client keep checking for stalls after load
    FRAME_TIME_FOR_MOVIES_SYNQ_TRESHOLD = 400   // (ms). a frame with a delta time bigger than this makes the client to request a synq to the server
  };
    
	
	int                 m_iFramesLeftCheckingMoviesSynq;   // used at start of the connection, to avoid load unstabilities when synchronizing movies (trackview) with the server

	
	// Used to store the pertinent details of migrating player entities so they
	// can be reconstructed as close as possible to their state prior to migration
	SMigratingPlayerInfo* m_pMigratingPlayerInfo;
	uint32 m_migratingPlayerMaxCount;

	SHostMigrationClientRequestParams* m_pHostMigrationParams;
	SHostMigrationClientControlledParams* m_pHostMigrationClientParams;

	SHostMigrationItemInfo *m_pHostMigrationItemInfo;
	uint32 m_hostMigrationItemMaxCount;

	float m_hostMigrationTimeStateChanged;			// Time when the host migration started (from timer->GetAsyncCurTime())
	EHostMigrationState m_hostMigrationState;

// Define pointers and accessor functions for each module type
#define GAMERULES_MODULE_LIST_FUNC(type, name, lowerCase, useInEditor) \
	protected:	\
		type *m_##lowerCase##Module;	\
	public:	\
	type *Get##name##Module() { return m_##lowerCase##Module; }

	GAMERULES_MODULE_TYPES_LIST

#undef GAMERULES_MODULE_LIST_FUNC

protected:
	typedef std::vector<IGameRulesPickupListener*> TPickupListenersVec;
	typedef std::vector<IGameRulesClientConnectionListener*> TClientConnectionListenersVec;
	typedef std::vector<IGameRulesTeamChangedListener*> TTeamChangedListenersVec;
	typedef std::vector<IGameRulesKillListener*> TKillListenersVec;
	typedef std::vector<IGameRulesModuleRMIListener*> TModuleRMIListenersVec;
	typedef std::vector<IGameRulesRevivedListener*> TRevivedListenersVec;
	typedef std::vector<IGameRulesSurvivorCountListener*> TSurvivorCountListenersVec;
	typedef std::vector<IGameRulesPlayerStatsListener*> TPlayerStatsListenersVec;
	typedef std::vector<IGameRulesRoundsListener*> TRoundsListenersVec;
	typedef std::vector<IGameRulesClientScoreListener*> TClientScoreListenersVec;
	TPickupListenersVec m_pickupListeners;
	TClientConnectionListenersVec m_clientConnectionListeners;
	TTeamChangedListenersVec m_teamChangedListeners;
	TKillListenersVec m_killListeners;
	TModuleRMIListenersVec m_moduleRMIListenersVec;
	TRevivedListenersVec m_revivedListenersVec;
	TSurvivorCountListenersVec m_survivorCountListenersVec;
	TPlayerStatsListenersVec m_playerStatsListenersVec;
	TRoundsListenersVec m_roundsListenersVec;
	TClientScoreListenersVec m_clientScoreListenersVec;

public:
	void RegisterPickupListener(IGameRulesPickupListener *pListener);
	void UnRegisterPickupListener(IGameRulesPickupListener *pListener);

	void RegisterClientConnectionListener(IGameRulesClientConnectionListener *pListener);
	void UnRegisterClientConnectionListener(IGameRulesClientConnectionListener *pListener);

	void RegisterTeamChangedListener(IGameRulesTeamChangedListener *pListener);
	void UnRegisterTeamChangedListener(IGameRulesTeamChangedListener *pListener);

	void RegisterRevivedListener( IGameRulesRevivedListener *pListener );
	void UnRegisterRevivedListener( IGameRulesRevivedListener *pListener );

	void RegisterSurvivorCountListener( IGameRulesSurvivorCountListener *pListener );
	void UnRegisterSurvivorCountListener( IGameRulesSurvivorCountListener *pListener );

	void RegisterPlayerStatsListener( IGameRulesPlayerStatsListener *pListener );
	void UnRegisterPlayerStatsListener( IGameRulesPlayerStatsListener *pListener );

	void RegisterRoundsListener( IGameRulesRoundsListener *pListener );
	void UnRegisterRoundsListener( IGameRulesRoundsListener *pListener );

	void RegisterClientScoreListener( IGameRulesClientScoreListener *pListener );
	void UnRegisterClientScoreListener( IGameRulesClientScoreListener *pListener );

	void RegisterKillListener(IGameRulesKillListener *pKillsListener);
	void UnRegisterKillListener(IGameRulesKillListener *pKillsListener);

	int RegisterModuleRMIListener(IGameRulesModuleRMIListener *pRMIListener);
	void UnRegisterModuleRMIListener(int index);

	void OwnClientConnected_NotifyListeners();

	void OnEntityKilled(const HitInfo &hitInfo);

	float GetRemainingHostMigrationTimeoutTime() const;
	float GetHostMigrationTimeTillResume() const;
	EHostMigrationState GetHostMigrationState() const { return m_hostMigrationState; }

	void CallEntityScriptFunction(EntityId entityId, const char *functionName)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
		if (pEntity)
		{
			IScriptTable *pScript = pEntity->GetScriptTable();
			if (pScript && pScript->GetValueType(functionName) == svtFunction)
			{
				m_pScriptSystem->BeginCall(pScript, functionName);
				m_pScriptSystem->PushFuncParam(pScript);
				m_pScriptSystem->EndCall();
			}
		}
	};

	void SvIncreaseSynchedEntityValue(EntityId entityId, int key, int increaseAmount)
	{
		int data = 0;
		GetSynchedEntityValue(entityId, key, data);
		data += increaseAmount;
		SetSynchedEntityValue(entityId, key, data);
	};

	const char *GetEntityName(EntityId inEntityId) const
	{
		IEntity *entity = gEnv->pEntitySystem->GetEntity(inEntityId);
		if (entity)
		{
			return entity->GetName();
		}
		
		return "<NULL>";
	}
	
	
  // TODO TEMP until game modules can send/recv RMIs (or we decide how we're going to do networks)
	void SendRMI_SvRequestSpectatorMode(EntityId entityId, uint8 mode, EntityId target, bool reset, unsigned where)
	{
		SpectatorModeParams  params (entityId, mode, target, reset);
		GetGameObject()->InvokeRMIWithDependentObject(SvRequestSpectatorMode(), params, where, params.entityId);
	}	
	
  // TODO TEMP until game modules can send/recv RMIs (or we decide how we're going to do networks)
	void SendRMI_SvAutoReviveOptOut(EntityId subject, unsigned where)
	{
		EntityParams  params (subject);
		GetGameObject()->InvokeRMI(SvAutoReviveOptOut(), params, where);
	}

	ILINE void SanityCheckHitInfo(const HitInfo &hitInfo, const char * funcName)
	{
		SanityCheckHitData(hitInfo.dir, hitInfo.shooterId, hitInfo.targetId, hitInfo.weaponId, hitInfo.type, funcName);
	}

#if !defined(_RELEASE)
	void SanityCheckHitData(const Vec3 & dir, EntityId shooter, EntityId target, EntityId weapon, uint16 hitType, const char * funcName);
#else
	ILINE void SanityCheckHitData(const Vec3 & dir, EntityId shooter, EntityId target, EntityId weapon, uint16 hitType, const char * funcName) {}
#endif

	private:
		void ProcessDeferredMaterialEffects();
		ILINE int GetFreeExplosionIndex();

}; 

#endif //__GAMERULES_H__
