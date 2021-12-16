/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 02:07:2009   13:16 : Created by Frank Harrison

*************************************************************************/
#ifndef __GAMERULESTYPES_H__
#define __GAMERULESTYPES_H__

#include "AutoEnum.h"

class CActor;

#define AEGameModeList(f) \
	f(eGM_SinglePlayer) \
	f(eGM_AllOrNothing) \
	f(eGM_Assault) \
	f(eGM_BombTheBase) \
	f(eGM_CaptureTheFlag) \
	f(eGM_Countdown) \
	f(eGM_CrashSite) \
	f(eGM_Extraction) \
	f(eGM_InstantAction) \
	f(eGM_PowerStruggleLite) \
	f(eGM_TeamInstantAction) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EGameMode, AEGameModeList, eGM_INVALID_GAMEMODE, eGM_NUM_GAMEMODES);

typedef int8 EGameRulesScoreType; // if this type changes update compression policy in ScoreChangeParams
typedef int16 TGameRulesScoreInt; // if this type changes update compression policy and score limits in ScoreChangeParams

#define EGRSTList(f) \
	f(EGRST_PlayerKill)							/*0*/ \
	f(EGRST_PlayerKillAssist)				/*1*/ \
	f(EGRST_PlayerTeamKill)					/*2*/ \
	f(EGRST_CTF_FlagPickedUp)				/*3*/ \
	f(EGRST_CTF_FlagCompleted)			/*4*/ \
	f(EGRST_CTF_FlagRetrieved)			/*5*/ \
	f(EGRST_Suicide)								/*6*/ \
	f(EGRST_SingleHanded)						/*7*/ \
	f(EGRST_FlagAssist)							/*8*/ \
	f(EGRST_SelfDestructPrevention)	/*9*/ \
	f(EGRST_CaptureObjectiveHeld)					\
	f(EGRST_CaptureObjectiveTaken)				\
	f(EGRST_CaptureObjectiveCompleted)		\
	f(EGRST_CaptureObjectivesDefended)		\
	f(EGRST_AON_Win)											\
	f(EGRST_AON_Draw)											\
	f(EGRST_CarryObjectiveTaken)					\
	f(EGRST_CarryObjectiveRetrieved)			\
	f(EGRST_CarryObjectiveCarrierKilled)	\
	f(EGRST_CarryObjectiveDefended)				\
	f(EGRST_CarryObjectiveCompleted)			\
	f(EGRST_CarryObjectiveHeld)						\
	f(EGRST_BombTheBaseCompleted)					\
	f(EGRST_KingOfTheHillObjectiveHeld)		\
	
AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EGRST, EGRSTList, EGRST_Unknown, EGRST_Num);

union UGameRulesScoreData
{
	struct
	{
		EntityId  victim;
	}
	PlayerKill;
};

struct SGameRulesScoreInfo
{
	static const int16 SCORE_MAX = SHRT_MAX;
	static const int16 SCORE_MIN = SHRT_MIN;

	UGameRulesScoreData data;
	EGameRulesScoreType type;
	TGameRulesScoreInt score;

	explicit SGameRulesScoreInfo(EGameRulesScoreType typeIn, TGameRulesScoreInt scoreIn)
	{
		memset (& data, 0, sizeof(data));
		type = typeIn;

		score = scoreIn;
	}

	explicit SGameRulesScoreInfo( const SGameRulesScoreInfo& _score )
	: data(_score.data)
	, type(_score.type)
	, score(_score.score)
	{
		// ...
	}

	ILINE SGameRulesScoreInfo & AttachVictim(EntityId id)
	{
		data.PlayerKill.victim = id;
		return *this;
	}
};

enum EGameRulesTargetType
{
	EGRTT_Unknown = -1,
	EGRTT_Neutral = 0,
	EGRTT_Friendly = 1,
	EGRTT_Hostile = 2
};

struct SGameRulesNotificationInfo
{
	SGameRulesNotificationInfo()
						: message(0), arg1(0), arg2(0), type(EGRTT_Neutral) {};

	SGameRulesNotificationInfo(EGameRulesTargetType _type, const char *_message, const char *_arg1, const char *_arg2)
						: message(_message), arg1(_arg1), arg2(_arg2), type(_type) {};

	const char *message;
	const char *arg1;
	const char *arg2;

	EGameRulesTargetType type;
};

enum EGameRulesWinner
{
	EGRW_Player = 1,
	EGRW_OtherPlayer = 2,
	EGRW_PlayersTeam = 3,
	EGRW_OpponentsTeam = 4,
	EGRW_NoWinner = 5,
	EGRW_Unknown // no more after me.
};

enum EGameOverReason
{
	EGOR_TimeLimitReached = 1,
	EGOR_ScoreLimitReached = 2,
	EGOR_ObjectivesCompleted = 3,
	EGOR_NoLivesRemaining = 4,
	EGOR_Unknown // no more after me.
};

enum EGameOverType
{
	EGOT_Lose = -1,
	EGOT_Draw = 0,
	EGOT_Win = 1,
	EGOT_Unknown
};

struct GameRulesGameOverInfo{
	EGameRulesWinner gamewinner;
	int showhud;
};

// TODO: Remove this enum, use the ID values specified in the XML
#define MissionObjectiveList(f)		        \
	f(EGRMO_CTF_Base_with_flag_blue)           /*00*/ \
	f(EGRMO_CTF_Base_with_flag_red)            /*01*/ \
	f(EGRMO_CTF_Base_no_flag_blue)             /*02*/ \
	f(EGRMO_CTF_Base_no_flag_red)              /*03*/ \
	f(EGRMO_CTF_Flag_carried_blue)             /*04*/ \
	f(EGRMO_CTF_Flag_carried_red)              /*05*/ \
	f(EGRMO_CTF_Flag_dropped_blue)             /*06*/ \
	f(EGRMO_CTF_Flag_dropped_red)              /*07*/ \
	f(EGRMO_ExplosiveThreat_blue)              /*08*/\
	f(EGRMO_ExplosiveThreat_red)               /*09*/ \
	f(EGRMO_PSL_CapturePoint_neutral)          /*10*/ \
	f(EGRMO_PSL_CapturePoint_neutralToBlue)    /*11*/ \
	f(EGRMO_PSL_CapturePoint_neutralToRed)     /*12*/ \
	f(EGRMO_PSL_Base_blue)                     /*13*/ \
	f(EGRMO_PSL_Base_red)                      /*14*/ \
	f(EGRMO_PSL_Weapon_blue)                   /*15*/ \
	f(EGRMO_PSL_Weapon_Red)                    /*16*/ \
	f(EGRMO_PSL_Weapon_carried_blue)           /*17*/ \
	f(EGRMO_PSL_Weapon_carried_red)            /*18*/ \
	f(EGRMO_PSL_CapturePoint_redToBlue)        /*19*/ \
	f(EGRMO_PSL_CapturePoint_blueToRed)        /*20*/ \
                                     \
	f(EGRMO_AS_IntelPoint_blue)                /*21*/ \
	f(EGRMO_AS_IntelPoint_red)                 /*22*/ \
	f(EGRMO_AS_IntelPoint_blueToRed)           /*23*/ \
	f(EGRMO_EXT_Weapon_blue)                   /*24*/ \
	f(EGRMO_EXT_Weapon_red)                    /*25*/ \
	f(EGRMO_EXT_Weapon_carried_blue)           /*26*/ \
	f(EGRMO_EXT_Weapon_carried_red)            /*27*/ \
	f(EGRMO_EXT_ExtractionPoint_blue)          /*28*/ \
	f(EGRMO_EXT_ExtractionPoint_red)           /*29*/ \
	/* capture site*/                                 \
	f(EGRMO_CS_CapturePoint_neutral)           /*30*/ \
	f(EGRMO_CS_CapturePoint_neutralToBlue)     /*31*/ \
	f(EGRMO_CS_CapturePoint_neutralToRed)      /*32*/ \
	f(EGRMO_CS_CapturePoint_redToBlue)         /*33*/ \
	f(EGRMO_CS_CapturePoint_blueToRed)         /*34*/ \
	/*f(EGRMO_CS_Base_blue) not in XML?*/             \
	/*f(EGRMO_CS_Base_red) not in XML?*/              \
	f(EGRMO_AON_EliminatedMarker)              /*35*/ \
	f(EGRMO_CD_TimeBonus_neutral)              /*36*/ \
	f(EGRMO_CD_TimeBonus_neutralToBlue)        /*37*/ \
	f(EGRMO_CD_TimeBonus_neutralToRed)         /*38*/ \
	f(EGRMO_CD_TimeBonus_redToBlue)            /*39*/ \
	f(EGRMO_CD_TimeBonus_blueToRed)            /*40*/ \
	f(EGRMO_FriendlyHolo)											 /*41*/ \
	f(EGRMO_AS_IntelPoint_redToBlue)           /*42*/ \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EGameRulesMissionObjectives, MissionObjectiveList, EGRMO_Unknown, EGRMO_Max);

struct SGameRulesMissionObjectiveInfo
{
	EntityId targetId;
	EGameRulesMissionObjectives objectiveType;
};

enum ECTFFlagState
{
	ECTF_FLAG_STATE_INBASE,
	ECTF_FLAG_STATE_CARRIED,
	ECTF_FLAG_STATE_DROPPED,
};

struct CTFFlagStateChange
{
	ECTFFlagState currentState;
	EntityId carrierId;
	bool isLocalTeam;
};

struct SBattleLogMessageInfo
{
	int      hitType;
	EntityId shooterId;
	EntityId targetId;
};

struct STeamChangeInfo
{
	int oldTeamNum;
	int newTeamNum;
};

struct SGameRulesListener
{
	virtual void GameOver(EGameOverType localWinner) {}
	virtual void EnteredGame() {}
	virtual void EndGameNear(EntityId id) {}
	virtual void ClientEnteredGame( EntityId clientId ) {}
	virtual void ClientDisconnect( EntityId clientId ) {}
	virtual void OnActorDeath( CActor* pActor ) {};
	virtual void SvOnTimeLimitExpired() {};

	void GetMemoryUsage(ICrySizer *pSizer ) const {}
};


#endif //__GAMERULESTYPES_H__
