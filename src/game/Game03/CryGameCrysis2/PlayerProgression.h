/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 07:12:2009		Created by Ben Parbury
*************************************************************************/

#ifndef __PLAYERPROGRESSION_H__
#define __PLAYERPROGRESSION_H__


#include "AutoEnum.h"
#include "GameRulesTypes.h"
#include "NanoSuitDefs.h"
#include "SuitModeProgression.h"
#include "GameRulesModules/IGameRulesClientScoreListener.h"
#include "Utility/CryFixedArray.h"

class CPlayer;
class CSuitModeProgression;
struct IFlashPlayer;

enum EPPData
{
	EPP_Rank = 0,
	EPP_MaxRank,
	EPP_XP,
	EPP_XPToNextRank,
	EPP_XPLastMatch,
	EPP_MatchStartRank,
	EPP_MatchStartXPInCurrentRank,
	EPP_MatchStartXPToNextRank,
	EPP_MatchBonus,
	EPP_XPInCurrentRank,
	EPP_NextRank,
};

//Non-score events that give you xp
#define PlayerProgressionType(f)	\
	f(EPP_TeamRadar)								\
	f(EPP_EMPStrike)								\
	f(EPP_SatStrike)								\
	f(EPP_SuitBoost)								\
	f(EPP_DogtagCollection)					\
	f(EPP_Headshot)									\
	f(EPP_AirDeath)									\
	f(EPP_MeleeTakedown)						\
	f(EPP_PistolKill)								\
	f(EPP_BlindKill)								\
	f(EPP_Impale)										\
	f(EPP_FirstBlood)								\
	f(EPP_Rumbled)									\
	f(EPP_UnderwaterKill)						\
	f(EPP_RoadKill)									\
	f(EPP_NearDeathExperience)			\
	f(EPP_KillJoy)									\

AUTOENUM_BUILDENUMWITHTYPE_WITHNUM(EPPType, PlayerProgressionType, EPP_Max);

enum EUnlockType
{
	eUT_Invalid = -1,
	eUT_Loadout = 0,
	eUT_Weapon,
};


struct IPlayerProgressionEventListener
{
	virtual void OnEvent(EPPType type, bool skillKill) = 0;
};

class CPlayerProgression : public SGameRulesListener, public IGameRulesClientScoreListener
{
public:
	CPlayerProgression();
	virtual ~CPlayerProgression();

	static CPlayerProgression* GetInstance();

	void Init();

	void Event(EPPType type, bool skillKill = false);
	void ClientScoreEvent(EGameRulesScoreType type, int points);
	bool SkillKillEvent(CGameRules* pGameRules, IActor* pTargetActor, IActor* pShooterActor, const char* weaponClassName, int damage, int hit_joint, int hit_type);

	void Update(CPlayer* pPlayer, float deltaTime, int iHealth);

	//SGameRulesListener
	virtual void EnteredGame();
	virtual void GameOver(EGameOverType localWinner);

	const int GetData(EPPData dataType);
	const int GetSuitModeData(ENanoSuitMode suitMode, EPPSuitData dataType);

	const char* GetRankName(uint8 rank);
	bool HaveUnlocked(EUnlockType type, const char* name, bool &exists, int &unlockValue);

	void SendUnlocksToFlash(IFlashPlayer *pFlashPlayer);

	static void CmdGainXP(IConsoleCmdArgs* pCmdArgs);
	static void CmdGameEnd(IConsoleCmdArgs* pCmdArgs);
	static void CmdUnlockAll(IConsoleCmdArgs* pCmdArgs);
	static void CmdUnlocksNow(IConsoleCmdArgs* pCmdArgs);
	static void CmdResetXP(IConsoleCmdArgs* pCmdArgs);

	void AddEventListener(IPlayerProgressionEventListener* pEventListener);
	void RemoveEventListener(IPlayerProgressionEventListener* pEventListener);

protected:
	void InitRanks(const char* filename);
	void InitEvents(const char* filename);
	void InitFromProfile();
	void InitConsoleCommands();

	void SaveProfile();

	void SanityCheckRanks();
	void SanityCheckSuitLevels();

	int CalculateRankFromXp(int xp);

	void EventInternal(int points);

	void IncrementXP(int amount);
	void MatchBonus(const EGameOverType localWinner, const float totalTime);
	float WinModifier(const EGameOverType localWinner) const;

	static EUnlockType GetUnlockTypeFromName(const char* name);
	const char * GetUnlockTypeString(EUnlockType type);

	void ResetXP();
	void UpdateStartRank();

	void SendEventToListeners(EPPType, bool);

	typedef std::vector<IPlayerProgressionEventListener*> TEventListener;
	TEventListener m_eventListeners;

	struct SRank
	{
		SRank(XmlNodeRef node);

		const static int k_maxNameLength = 16;
		char m_name[k_maxNameLength];
		int m_xpRequired;
	};

	struct SUnlock
	{
		SUnlock(XmlNodeRef node, int rank);
		const static int k_maxNameLength = 16;

		char m_name[k_maxNameLength];
		EUnlockType m_type;
		int m_rank;
	};

	struct SFlashUnlockInfo
	{
		SFlashUnlockInfo() : m_name(0) { }

		const char * m_name;
		CryFixedStringT<16> m_type;
		CryFixedStringT<4> m_rank;
	};

	static CPlayerProgression* s_playerProgression_instance;

	const static int k_maxPossibleRanks = 80;
	typedef CryFixedArray<SRank, k_maxPossibleRanks> TRankElements;
	TRankElements m_ranks;
	int m_maxRank;

	const static int k_maxEvents = EPP_Max;
	int m_events[k_maxEvents];

	const static int k_maxUnlocks = 80;
	typedef CryFixedArray<SUnlock, k_maxUnlocks> TUnlockElements;
	TUnlockElements m_unlocks;

	const static int k_timeSlots = eNanoSuitMode_Last + 1;
	const static int k_deadTime = eNanoSuitMode_Last;
	//used to keep track of time in different suit modes (and time dead)
	float m_time[k_timeSlots];

	CSuitModeProgression *m_suitProgression[eNanoSuitMode_Last];

	int m_xp;
	int m_gameStartXp;
	int m_rank;
	int m_gameStartRank;
	int m_matchBonus;
};

#endif // __PLAYERPROGRESSION_H__
