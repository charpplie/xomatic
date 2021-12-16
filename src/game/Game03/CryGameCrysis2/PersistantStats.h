/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 04:02:2010		Created by Ben Parbury
*************************************************************************/

#ifndef __PERSISTANTSTATS_H__
#define __PERSISTANTSTATS_H__


#include "GameRulesTypes.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesClientScoreListener.h"
#include "PlayerProgression.h"
#include <IGameRulesSystem.h>
#include <ICryStats.h>

#include "AutoEnum.h"

struct IPlayerProfile;
struct ICryPak;
struct IFlashPlayer;
class CPlayer;

#define IntPersistantStats(f) \
	f(EIPS_KillsSuitPower, eSF_RemoteClients) \
	f(EIPS_KillsSuitArmor, eSF_RemoteClients) \
	f(EIPS_KillsSuitStealth, eSF_RemoteClients) \
	f(EIPS_KillsSuitTactical, eSF_RemoteClients) \
	f(EIPS_KillsNoSuit, eSF_RemoteClients) \
	\
	f(EIPS_DeathsSuitPower, eSF_RemoteClients) \
	f(EIPS_DeathsSuitArmor, eSF_RemoteClients) \
	f(EIPS_DeathsSuitStealth, eSF_RemoteClients) \
	f(EIPS_DeathsSuitTactical, eSF_RemoteClients) \
	f(EIPS_DeathsNoSuit, eSF_RemoteClients) \
	\
	f(EIPS_Suicides, eSF_RemoteClients) \
	\
	f(EIPS_TeamRadar, eSF_LocalClient) \
	f(EIPS_EMPStrike, eSF_LocalClient) \
	f(EIPS_SatStrike, eSF_LocalClient) \
	f(EIPS_SuitBoost, eSF_LocalClient) \
	\
	f(EIPS_SkillKills, eSF_LocalClient) \
	f(EIPS_DogtagsCollected, eSF_LocalClient) \
	f(EIPS_FirstBlood, eSF_LocalClient) \
	\
	f(EIPS_KillAssists, eSF_LocalClient) \
	f(EIPS_FlagCaptures, eSF_LocalClient) \
	f(EIPS_CaptureObjectives, eSF_LocalClient) \
	f(EIPS_CarryObjectives, eSF_LocalClient) \
	f(EIPS_BombTheBase, eSF_LocalClient) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EIntPersistantStats, IntPersistantStats, EIPS_Invalid, EIPS_Max);

#define FloatPersistantStats(f) \
	f(EFPS_Overall, eSF_LocalClient) \
	\
	f(EFPS_SuitPowerTime, eSF_LocalClient) \
	f(EFPS_SuitArmorTime, eSF_LocalClient) \
	f(EFPS_SuitStealthTime, eSF_LocalClient) \
	f(EFPS_SuitTacticalTime, eSF_LocalClient) \
	f(EFPS_NoSuitTime, eSF_LocalClient) \
	\
	f(EFPS_DistanceAir, eSF_LocalClient) \
	f(EFPS_DistanceRan, eSF_LocalClient) \
	f(EFPS_DistanceSwum, eSF_LocalClient) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EFloatPersistantStats, FloatPersistantStats, EFPS_Invalid, EFPS_Max);

#define StreakIntPersistantStats(f) \
	f(ESIPS_Kills, eSF_RemoteClients) \
	f(ESIPS_Deaths, eSF_RemoteClients) \
	f(ESIPS_Win, eSF_LocalClient) \
	f(ESIPS_Lose, eSF_LocalClient) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EStreakIntPersistantStats, StreakIntPersistantStats, ESIPS_Invalid, ESIPS_Max);

#define StreakFloatPersistantStats(f) \
	f(ESFPS_TimeAlive, eSF_LocalClient) \
	f(ESFPS_DistanceAir, eSF_LocalClient) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EStreakFloatPersistantStats, StreakFloatPersistantStats, ESFPS_Invalid, ESFPS_Max);

#define MapPersistantStats(f) \
	f(EMPS_Gamemodes, eSF_LocalClient) \
	f(EMPS_GamesWon, eSF_LocalClient) \
	f(EMPS_GamesDrawn, eSF_LocalClient) \
	f(EMPS_GamesLost, eSF_LocalClient) \
	\
	f(EMPS_Levels, eSF_LocalClient) \
	\
	f(EMPS_WeaponHits, eSF_LocalClient) \
	f(EMPS_WeaponShots, eSF_RemoteClients) \
	f(EMPS_WeaponKills, eSF_RemoteClients) \
	f(EMPS_WeaponHeadshots, eSF_LocalClient) \
	f(EMPS_WeaponUsage, eSF_RemoteClients) \
	f(EMPS_KillsByDamageType, eSF_RemoteClients) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EMapPersistantStats, MapPersistantStats, EMPS_Invalid, EMPS_Max);

#define DerivedIntPersistantStats(f) \
	f(EDIPS_GamesPlayed) \
	f(EDIPS_GamesWon) \
	f(EDIPS_GamesLost) \
	f(EDIPS_GamesDrawn) \
	\
	f(EDIPS_Kills) \
	f(EDIPS_Deaths) \
	\
	f(EDIPS_Hits) \
	f(EDIPS_Shots) \
	f(EDIPS_Misses) \
	f(EDIPS_Headshots) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EDerivedIntPersistantStats, DerivedIntPersistantStats, EDIPS_Invalid, EDIPS_Max);

#define DerivedFloatPersistantStats(f) \
	f(EDFPS_WinLoseRatio) \
	f(EDFPS_KillDeathRatio) \
	f(EDFPS_Accuracy) \
	f(EDFPS_AliveTime) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EDerivedFloatPersistantStats, DerivedFloatPersistantStats, EDFPS_Invalid, EDFPS_Max);

#define DerivedStringPersistantStats(f) \
	f(EDSPS_FavouriteSuitMode) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EDerivedStringPersistantStats, DerivedStringPersistantStats, EDSPS_Invalid, EDSPS_Max);

#define DerivedFloatMapPersistantStats(f) \
	f(EDFMPS_Accuracy) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EDerivedFloatMapPersistantStats, DerivedFloatMapPersistantStats, EDFMPS_Invalid, EDFMPS_Max);

#define DerivedIntMapPersistantStats(f) \
	f(EDIMPS_GamesPlayed) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EDerivedIntMapPersistantStats, DerivedIntMapPersistantStats, EDIMPS_Invalid, EDIMPS_Max);

#define DerivedStringMapPersistantStats(f) \
	f(EDSMPS_FavouriteGamemode) \
	f(EDSMPS_FavouritePrimaryWeapon) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EDerivedStringMapPersistantStats, DerivedStringMapPersistantStats, EDSMPS_Invalid, EDSMPS_Max);

enum StatsFlags
{
	eSF_LocalClient			= 0,				//stats that are calculated for local client (All stats hence it's zero)
	eSF_RemoteClients		= BIT(0),		//stats that are tracked for remote clients
	//TODO some ServerLocalClient stats for stats that are tracked by server and local clients
};

#if defined(XENON)
//If you want to change this you'll need to submit an updated xlast for Microsoft
#include "Network/Lobby/Xenon_LobbyDefinitions.h"

enum ELeaderboards
{
	eLB_Start = STATS_VIEW_XP,

	eLB_XP = STATS_VIEW_XP,
	eLB_Kills = STATS_VIEW_KILLS,
	eLB_KillDeathRatio = STATS_VIEW_KILLDEATHRATIO,	//float
	eLB_Accuracy = STATS_VIEW_ACCURACY,			//float
	eLB_KillStreak = STATS_VIEW_KILLSTREAK,

	eLB_Num = 5		//5 is the maximum number allowed to upload per session for xbox360
};

//Note for float stats need to use on result ConvertToFloatFromLeaderboardScore

enum EXPColumnLeaderboards
{
	eXPCL_Start = STATS_COLUMN_XP_XP_ARMOUR,

	eXPCL_Armour = STATS_COLUMN_XP_XP_ARMOUR,
	eXPCL_Power = STATS_COLUMN_XP_XP_POWER,
	eXPCL_Stealth = STATS_COLUMN_XP_XP_STEALTH,
	eXPCL_Tactical = STATS_COLUMN_XP_XP_TACTICAL,

	eXPCL_num = 4
};

#endif

class CPersistantStats : public SGameRulesListener,
	public IGameRulesKillListener,
	public IPlayerProgressionEventListener,
	public IWeaponEventListener,
	public IItemSystemListener,
	public IGameRulesClientScoreListener
{
public:
	CPersistantStats();
	virtual ~CPersistantStats();

	static CPersistantStats* GetInstance();

	void Init();
	void Update(const float dt);
	void Save();

	ILINE int GetStat(EIntPersistantStats stat);
	ILINE float GetStat(EFloatPersistantStats stat);
	ILINE int GetStat(EStreakIntPersistantStats stat);
	ILINE float GetStat(EStreakFloatPersistantStats stat);
	int GetStat(const char* name, EMapPersistantStats);
	int GetDerivedStat(EDerivedIntPersistantStats stat);
	float GetDerivedStat(EDerivedFloatPersistantStats stat);
	const char *GetDerivedStat(EDerivedStringPersistantStats);
	const char *GetDerivedStat(EDerivedStringMapPersistantStats);
	int GetDerivedStat(const char* name, EDerivedIntMapPersistantStats);
	float GetDerivedStat(const char* name, EDerivedFloatMapPersistantStats);

	int GetCurrentStat(EntityId actorId, EStreakIntPersistantStats stat);

	virtual void ClientHit(CPlayer *pPlayer, const HitInfo&);

	void SendStatsToFlash(IFlashPlayer *pFlashPlayer);

	//SGameRulesListener
	virtual void GameOver(EGameOverType localWinner);
	virtual void EnteredGame();
	virtual void ClientDisconnect( EntityId clientId );
	//~SGameRulesListener

	//IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	//~IGameRulesKillListener

	//IPlayerProgressionEventListener
	virtual void OnEvent(EPPType type, bool skillKill);
	//~IPlayerProgressionEventListener

	// IWeaponEventListener
	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void OnFireModeChanged(IWeapon *pWeapon, int currentFireMode){}
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {}
	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {}
	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {}
	virtual void OnReadyToFire(IWeapon *pWeapon) {}
	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed) {}
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId) {}
	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId) {}
	virtual void OnStartTargetting(IWeapon *pWeapon) {}
	virtual void OnStopTargetting(IWeapon *pWeapon) {}
	virtual void OnSelected(IWeapon *pWeapon, bool selected) {}
	virtual void OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId) {}
	//~IWeaponEventListener

	//IItemSystemListener
	virtual void OnSetActorItem(IActor *pActor, IItem *pItem );
	virtual void OnDropActorItem(IActor *pActor, IItem *pItem ) {}
	virtual void OnSetActorAccessory(IActor *pActor, IItem *pItem ) {}
	virtual void OnDropActorAccessory(IActor *pActor, IItem *pItem ){}
	//~IItemSystemListener

	//IGameRulesClientScoreListener
	virtual void ClientScoreEvent(EGameRulesScoreType scoreType, int points);
	//~IGameRulesClientScoreListener

#if defined(XENON)
	void RegisterLeaderboards();
	bool AreLeaderboardsRegistered() { return m_registeredLeaderboards; }

	static uint32 GetLeaderboardIdFromIndex(int index) { return eLB_Start + index; }
	static	uint32 GetColumnIdFromIndex(int index) { return eXPCL_Start + index; }

	static int64 ConvertToLeaderboardScoreFromFloat(float value);
	static float ConvertToFloatFromLeaderboardScore(int64 value);
#endif

	void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
protected:
	static CPersistantStats* s_persistantStats_instance;

	template<class T>
	struct SStreak
	{
		T m_curVal;
		T m_maxVal;

		SStreak();

		void Increment();
		void Increment(T inc);
		void Reset();
	};

	struct SMap
	{
		typedef std::map<CryFixedStringT<32>, int> MapNameToCount;
		MapNameToCount m_map;

		SMap();

		void Update(const char* name, int amount = 1);
		void Clear();

		void Save(const char* name, IPlayerProfile* pProfile) const;
		void Load(const char* name, const IPlayerProfile* pProfile);

		MapNameToCount::iterator FindOrInsert(const char* name);

		int GetStat(const char* name) const;
		int GetTotal() const;

#ifndef _RELEASE
		void watch() const;
#endif
	};

	struct SPersistantStatsStrings
	{
		CryFixedStringT<32> m_title;
		CryFixedStringT<32> m_value;
	};
	
	void GetStatStrings(EIntPersistantStats stat, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(EFloatPersistantStats stat, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(EStreakIntPersistantStats stat, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(EStreakFloatPersistantStats stat, SPersistantStatsStrings *statsStrings);
	
	void GetStatStrings(EDerivedIntPersistantStats stat, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(EDerivedFloatPersistantStats stat, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(EDerivedStringPersistantStats stat, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(EDerivedStringMapPersistantStats stat, SPersistantStatsStrings *statsStrings);
	
	void GetStatStrings(const char* name, EDerivedIntMapPersistantStats stat, const char* paramString, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(const char* name, EDerivedFloatMapPersistantStats stat, const char* paramString, SPersistantStatsStrings *statsStrings);
	void GetStatStrings(const char* name, EMapPersistantStats stat, const char* paramString, SPersistantStatsStrings *statsStrings);

	struct SSessionStats
	{
		SSessionStats();

		void Add(const SSessionStats* pSessionStats);
		void Clear();

		ILINE int GetStat(EIntPersistantStats stat) const;
		ILINE float GetStat(EFloatPersistantStats stat) const;
		ILINE int GetStat(EStreakIntPersistantStats stat) const;
		ILINE float GetStat(EStreakFloatPersistantStats stat) const;
		int GetStat(const char* name, EMapPersistantStats) const;
		int GetDerivedStat(EDerivedIntPersistantStats stat) const;
		float GetDerivedStat(EDerivedFloatPersistantStats stat) const;

		int m_intStats[EIPS_Max];
		float m_floatStats[EFPS_Max];
		SStreak<int> m_streakIntStats[ESIPS_Max];
		SStreak<float> m_streakFloatStats[ESFPS_Max];
		SMap m_mapStats[EMPS_Max];
	};

	const static int MAX_GROUP_STATS = 64;
	typedef CryFixedArray<SPersistantStatsStrings, MAX_GROUP_STATS> TPersistantStatsStringsArray;
	typedef std::vector<const char*> TStringsPushArray;

	void SetupSendStatsGroupToFlash(IFlashPlayer *pFlashPlayer, TPersistantStatsStringsArray &fixedStringsArray, const char *name);
	void SendStatsGroupToFlash(IFlashPlayer *pFlashPlayer, TPersistantStatsStringsArray &fixedStringsArray, TStringsPushArray &pushArray);

	//Sessions are stored for every player for the current session
	typedef std::map<EntityId, SSessionStats> ActorSessionMap;
	ActorSessionMap m_sessionStats;

	//local clients stats that are updated at the end of session
	SSessionStats m_clientPersistantStats;

	SSessionStats* GetActorSessionStats(EntityId actorId);
	SSessionStats* GetClientPersistantStats();

	void PostGame();

	void Load();
	
	bool LoadFromProfile(SSessionStats* pSessionStats);
	bool SaveToProfile(const SSessionStats* pSessionStats);

#if defined(XENON)
	int64 static GetLeaderboardScoreFromId(uint32 id);
	int64 static GetColumnScoreFromColumnId(uint32 columnId);
	uint32 static GetColumnDataIdFromColumnId(uint32 columnId);
	static void RegisterLeaderboardsCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);

	void WriteToLeaderboards();
	static void WriteLeaderboardsCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);

	SCryStatsLeaderBoardWrite m_board[eLB_Num];
	SCryStatsLeaderBoardUserColumn m_column[eXPCL_num];

	bool m_registeredLeaderboards;
	bool m_writingLeaderboardData;
#endif

	int GetBinaryVersionHash(uint32 flags);
	bool SaveBinary(const char* filename, const SSessionStats* pSessionStats, uint32 flags, bool description);

	template <class T>
	void WriteBinData(ICryPak* pPak, T *data, size_t length, size_t elems, FILE *handle);

	void AddListeners();
	void ClearListeners();
	int GetClientStatSuitIndex();
	int GetActorStatSuitIndex(EntityId actorId);
	int GetActorStatSuitIndex(IActor* pActor);

	typedef std::map<EntityId, EntityId> ActorWeaponListenerMap;
	ActorWeaponListenerMap m_actorWeaponListener;

	void SetNewWeaponListener(IWeapon* pWeapon, EntityId weaponId, EntityId actorId);
	void RemoveWeaponListener(EntityId weaponId);
	void RemoveAllWeaponListeners();

	const char* GetActorItemName(EntityId actorId);
	const char* GetItemName(EntityId weaponId);
	const char* GetItemName(IItem* pItem);

	static float GetRatio(int a, int b);

#ifndef _RELEASE
	void debugSessionStats(const SSessionStats* pSession, const float dt);

	static void CmdDumpTelemetryDescription(IConsoleCmdArgs* pCmdArgs);
	template <class T>
	static void CreateDescriptionNode(XmlNodeRef base, int &pos, const char* codeName, size_t size, const char* type, const char* mapName, T testValue);
#endif

};

#endif
