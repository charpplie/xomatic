/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   IAgent.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2 Mar 2009			 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(IAgent.h)

#ifndef _IAGENT_H_
#define _IAGENT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "Cry_Math.h"
#include "AgentParams.h"
#include "IScriptSystem.h"
#include "IAISystem.h"
#include "SerializeFwd.h"
#include "IPhysics.h"
#include <algorithm>
#include <limits>


#ifdef LINUX
#	include "platform.h"
#endif

#ifdef max
#undef max
#endif

struct IWeapon;
struct IEntity;
struct GraphNode;
struct IAIActorProxy;

class CSmartObject;
class IPersonalInterestManager;
struct IPersonalBehaviorTree;
struct VisionID;

// AI Register types

enum EAIRegister
{
	AI_REG_NONE,
	AI_REG_LASTOP,
	AI_REG_REFPOINT,
	AI_REG_ATTENTIONTARGET,
	AI_REG_HIDEOBJECT,
	AI_REG_LAST
};

//	 Defines for AIObject types.

#define	AIOBJECT_NONE 				200
#define AIOBJECT_DUMMY				0

#define AIOBJECT_AIACTOR			1
#define AIOBJECT_CAIACTOR			2
#define AIOBJECT_PIPEUSER			3
#define AIOBJECT_CPIPEUSER		4
#define	AIOBJECT_PUPPET				5
#define AIOBJECT_CPUPPET			6
#define AIOBJECT_VEHICLE			7
#define AIOBJECT_CVEHICLE			8
#define AIOBJECT_TARGET				9

#define AIOBJECT_AWARE				10
#define AIOBJECT_ATTRIBUTE		11
#define AIOBJECT_WAYPOINT			12
#define AIOBJECT_HIDEPOINT		13
#define AIOBJECT_SNDSUPRESSOR	14
#define AIOBJECT_HELICOPTER			40
#define AIOBJECT_CAR				50
#define AIOBJECT_BOAT				60
#define AIOBJECT_AIRPLANE			70
#define AIOBJECT_2D_FLY				80
#define AIOBJECT_MOUNTEDWEAPON		90
#define AIOBJECT_GLOBALALERTNESS	94
#define AIOBJECT_LEADER				95
#define AIOBJECT_ORDER				96
#define	AIOBJECT_PLAYER 			100

#define	AIOBJECT_GRENADE 		150
#define	AIOBJECT_RPG 			151

#define AI_USE_HIDESPOTS	(1<<14)


// Signal ids
#define AISIGNAL_INCLUDE_DISABLED 0
#define AISIGNAL_DEFAULT 1
#define AISIGNAL_PROCESS_NEXT_UPDATE 3
#define AISIGNAL_NOTIFY_ONLY 9
#define AISIGNAL_ALLOW_DUPLICATES 10
#define AISIGNAL_RECEIVED_PREV_UPDATE 30	// Internal AI system use only, like AISIGNAL_DEFAULT but used for logging/recording.
											// A signal sent in the previous update and processed in the current (AISIGNAL_PROCESS_NEXT_UPDATE).
#define AISIGNAL_INITIALIZATION -100

// Anchors
#define	AIANCHOR_FIRST												200
#define	AIANCHOR_COMBAT_HIDESPOT							320
#define	AIANCHOR_COMBAT_HIDESPOT_SECONDARY		330
#define	AIANCHOR_COMBAT_TERRITORY             342

#define	AIANCHOR_REINFORCEMENT_SPOT						400
#define	AIANCHOR_NOGRENADE_SPOT								405
#define AIANCHOR_ADVANTAGE_POINT							329

// Summary:
//	 Event types.
#define AIEVENT_ONVISUALSTIMULUS	2
#define AIEVENT_ONPATHDECISION	3
#define AIEVENT_ONSOUNDEVENT		4
#define AIEVENT_AGENTDIED				5
#define AIEVENT_SLEEP						6
#define AIEVENT_WAKEUP					7
#define AIEVENT_ENABLE					8
#define AIEVENT_DISABLE					9
#define AIEVENT_PATHFINDON			11
#define AIEVENT_PATHFINDOFF			12
#define AIEVENT_CLEAR						15
//#define AIEVENT_MOVEMENT_CONTROL	16			// Based on parameters lets object know if he is allowed to move.

//
#define AIEVENT_DROPBEACON			17
#define AIEVENT_USE							19
#define AIEVENT_CLEARACTIVEGOALS	22
#define AIEVENT_DRIVER_IN				23		// To enable/disable AIVehicles.
#define AIEVENT_DRIVER_OUT			24			// To enable/disable AIVehicles.
#define AIEVENT_FORCEDNAVIGATION	25
#define AIEVENT_ADJUSTPATH			26
#define AIEVENT_LOWHEALTH				27
#define AIEVENT_ONBULLETRAIN			28

#define AIEVENT_PLAYER_STUNT_SPRINT 101
#define AIEVENT_PLAYER_STUNT_JUMP 102
#define AIEVENT_PLAYER_STUNT_PUNCH 103
#define AIEVENT_PLAYER_STUNT_THROW 104
#define AIEVENT_PLAYER_STUNT_THROW_NPC 105
#define AIEVENT_PLAYER_THROW 106
#define AIEVENT_PLAYER_STUNT_CLOAK 107
#define AIEVENT_PLAYER_STUNT_UNCLOAK 108

// Summary:
//	 System Events types.
#define AISYSEVENT_DISABLEMODIFYER	1


#define AIREADIBILITY_INTERESTING		5
#define AIREADIBILITY_SEEN				10
#define AIREADIBILITY_LOST				20
#define AIREADIBILITY_NORMAL			30
#define AIREADIBILITY_NOPRIORITY		1

#define AIGOALPIPE_LOOP				0
#define AIGOALPIPE_RUN_ONCE			1		// Todo: Not working yet - see PipeUser.cpp
#define AIGOALPIPE_NOTDUPLICATE	2
#define AIGOALPIPE_HIGHPRIORITY		4		// It will be not removed when a goal pipe is selected.
#define AIGOALPIPE_SAMEPRIORITY		8		// Sets the priority to be the same as active goal pipe.
#define AIGOALPIPE_DONT_RESET_AG	16		// Don't reset the AG Input (by default AG Action input is reset to idle).
#define AIGOALPIPE_KEEP_LAST_SUBPIPE	32	// Keeps the last inserted subpipe.
#define AIGOALPIPE_KEEP_ON_TOP	64			// Keeps the inserted subpipe on the top for its duration, FIFO.

enum ESignalFilter
{
	SIGNALFILTER_SENDER,
	SIGNALFILTER_LASTOP,
	SIGNALFILTER_GROUPONLY,
	SIGNALFILTER_SPECIESONLY,
	SIGNALFILTER_ANYONEINCOMM,
	SIGNALFILTER_TARGET,
	SIGNALFILTER_SUPERGROUP,
	SIGNALFILTER_SUPERSPECIES,
	SIGNALFILTER_SUPERTARGET,
	SIGNALFILTER_NEARESTGROUP,
	SIGNALFILTER_NEARESTSPECIES,
	SIGNALFILTER_NEARESTINCOMM,
	SIGNALFILTER_HALFOFGROUP,
	SIGNALFILTER_LEADER,
	SIGNALFILTER_GROUPONLY_EXCEPT,
	SIGNALFILTER_ANYONEINCOMM_EXCEPT,
	SIGNALFILTER_LEADERENTITY,
	SIGNALFILTER_NEARESTINCOMM_SPECIES,
	SIGNALFILTER_NEARESTINCOMM_LOOKING,
	SIGNALFILTER_FORMATION,
	SIGNALFILTER_FORMATION_EXCEPT,
	SIGNALFILTER_READABILITY = 100,
	SIGNALFILTER_READABILITYAT,         // Readability anticipation.
	SIGNALFILTER_READABILITYRESPONSE,   // Readability response.
};

// AI Object filters: define them as one-bit binary masks (1,2,4,8 etc..)
#define AIOBJECTFILTER_SAMESPECIES		1 // AI objects of the same species of the querying object.
#define AIOBJECTFILTER_SAMEGROUP		2 // AI objects of the same group of the querying object or with no group.
#define AIOBJECTFILTER_NOGROUP			4 // AI objects with Group ID ==AI_NOGROUP.
#define AIOBJECTFILTER_INCLUDEINACTIVE	8 // AI objects and Inactive AI Objects.

#define AI_NOGROUP	-1 // group id == AI_NOGROUP -> the entity has no group

// AI Object search methods.
//-------------------- for anchors -----------------------------
#define	AIANCHOR_NEAREST										0
#define	AIANCHOR_NEAREST_IN_FRONT						1
#define	AIANCHOR_RANDOM_IN_RANGE						2
#define	AIANCHOR_RANDOM_IN_RANGE_FACING_AT	3
#define	AIANCHOR_NEAREST_FACING_AT					4
#define	AIANCHOR_NEAREST_TO_REFPOINT				5
#define	AIANCHOR_FARTHEST										6
#define	AIANCHOR_BEHIND_IN_RANGE						7
#define	AIANCHOR_LEFT_TO_REFPOINT						8
#define	AIANCHOR_RIGHT_TO_REFPOINT					9
#define	AIANCHOR_HIDE_FROM_REFPOINT					10
//-------------------- bit flags -------------------------------
#define AIANCHOR_SEES_TARGET				(1 << 6)
#define	AIANCHOR_BEHIND							(1 << 7)
//-------------------- anchors over ----------------------------


#define HM_NEAREST							0
#define HM_FARTHEST_FROM_TARGET				1
#define HM_NEAREST_TO_TARGET				2
#define HM_FARTHEST_FROM_GROUP				3
#define HM_NEAREST_TO_GROUP					4							
#define HM_LEFTMOST_FROM_TARGET				5	
#define HM_RIGHTMOST_FROM_TARGET			6	
#define HM_RANDOM							7	
#define HM_FRONTLEFTMOST_FROM_TARGET		8	
#define HM_FRONTRIGHTMOST_FROM_TARGET		9	
#define HM_NEAREST_TO_FORMATION				10						
#define HM_FARTHEST_FROM_FORMATION			11					
#define HM_NEAREST_TOWARDS_TARGET				12		// Same as HM_NEAREST_TO_TARGET, but forces the selection amongst obstacles towards the target only.
#define HM_NEAREST_BACKWARDS						13
#define HM_NEAREST_PREFER_SIDES		14
//#define HM_NEAREST_HALF_SIDE			15				// Consider half search distance on sides and back.
#define HM_NEAREST_TO_ME			16					// Same as HM_NEAREST, but the distance considered is to the puppet itself even if point is searched with HM_AROUND_LASTOP.
#define HM_NEAREST_TOWARDS_TARGET_PREFER_SIDES		17
#define HM_NEAREST_TOWARDS_TARGET_LEFT_PREFER_SIDES		18
#define HM_NEAREST_TOWARDS_TARGET_RIGHT_PREFER_SIDES		19
#define HM_NEAREST_TOWARDS_REFPOINT		20
//------------------------------------- The onces below all use lastOp result.

//
#define HM_BEHIND_VEHICLES					21
#define HM_USEREFPOINT						22
#define HM_USEHIDEOBJECT					23
#define HM_ASKLEADER						24
#define HM_ASKLEADER_NOSAME					25

#define HM_INCLUDE_SOFTCOVERS		(1 << 6)
#define HM_IGNORE_ENEMIES				(1 << 7)
#define HM_BACK									(1 << 8)
#define HM_AROUND_LASTOP				(1 << 9)
#define HM_FROM_LASTOP					(1 << 10)
#define HM_USE_LASTOP_RADIUS		(1 << 11)
#define HM_ON_SIDE							(1 << 12)		// To choose spot on left/right side of cover, not behind.

#define HM_NEAREST_TO_LASTOPRESULT			(HM_NEAREST+HM_AROUND_LASTOP)
#define HM_RANDOM_AROUND_LASTOPRESULT		(HM_RANDOM+HM_AROUND_LASTOP)

#define PERCEPTION_MAX_VALUE 30.f
#define PERCEPTION_TARGET_SEEN_VALUE 20.f
#define PERCEPTION_SOMETHING_SEEN_VALUE 10.f

#define	AISPEED_ZERO    (0.0f)
#define	AISPEED_SLOW    (0.21f)
#define	AISPEED_WALK    (0.4f)
#define	AISPEED_RUN     (1.0f)
#define AISPEED_SPRINT  (1.4f)

#define AI_JUMP_CHECK_COLLISION (1<<0)
#define AI_JUMP_ON_GROUND				(1<<1)
#define AI_JUMP_RELATIVE				(1<<2)
#define AI_JUMP_MOVING_TARGET		(1<<3)

#define AI_LOOKAT_CONTINUOUS (1<<0)
#define AI_LOOKAT_USE_BODYDIR (1<<1)

enum EJumpAnimType
{
	JUMP_ANIM_FLY,
	JUMP_ANIM_LAND
};

enum ELookMotivation
{
	AILOOKMOTIVATION_LOOK = 0,
	AILOOKMOTIVATION_GLANCE,
	AILOOKMOTIVATION_STARTLE,
	AILOOKMOTIVATION_DOUBLETAKE
};


//===================================================================
// MovementUrgencyToIndex
//===================================================================
inline int MovementUrgencyToIndex(float urgency)
{
	int sign = urgency < 0.0f ? -1 : 1;
	if(urgency < 0) urgency = -urgency;

	if(urgency < (AISPEED_ZERO+AISPEED_SLOW)/2)
		return 0;
	else if(urgency < (AISPEED_SLOW+AISPEED_WALK)/2)
		return 1 * sign;	// slow
	else if(urgency < (AISPEED_WALK+AISPEED_RUN)/2)
		return 2 * sign;	// walk
	else if(urgency < (AISPEED_RUN+AISPEED_SPRINT)/2)
		return 3 * sign;	// run
	else
		return 4 * sign;	// sprint
}

//===================================================================
// IndexToMovementUrgency
//===================================================================
inline float IndexToMovementUrgency(int idx)
{
	float sign = idx < 0 ? -1.0f : 1.0f;
	if(idx < 0) idx = -idx;
	switch(idx)
	{
	case 0: return AISPEED_ZERO * sign;
	case 1: return AISPEED_SLOW * sign;
	case 2: return AISPEED_WALK * sign;
	case 3: return AISPEED_RUN * sign;
	default: return AISPEED_SPRINT * sign;
	}
}

// Description:
//		The coverage half-angle of a hidespot anchor. The total coverage angle is twice this angle.
#define	HIDESPOT_COVERAGE_ANGLE_COS			0.5f		// cos(60 degrees)

// Summary:
//		Path finder blockers types.
enum ENavigationBlockers
{
	PFB_NONE,
	PFB_ATT_TARGET,
	PFB_REF_POINT,
	PFB_BEACON,
	PFB_DEAD_BODIES,
	PFB_EXPLOSIVES,
	PFB_PLAYER,
	PFB_BETWEEN_NAV_TARGET,
};

enum EOPWaitType
{
	WAIT_ALL = 0,		// Wait for all group to be finished.
	WAIT_ANY = 1,		// Wait for any goal in the group to be finished.
	WAIT_ANY_2 = 2,		// Wait for any goal in the group to be finished.
	WAIT_LAST,
};

enum EOPBranchType
{
	IF_ACTIVE_GOALS = 0,				// Default as it was in CryEngine1 - jumps if there are active (not finished) goal operations.
	IF_ACTIVE_GOALS_HIDE,				// Default as it was in CryEngine1 - jumps if there are active goals or hide spot wasn't found.
	IF_NO_PATH,							// Jumps if there wasn't a path in the last "pathfind" goal operation.
	IF_PATH_STILL_FINDING,				// Jumps if the current pathfind request hasn't been completed yet.
	IF_IS_HIDDEN,						// Jumps if the last "hide" goal operation has succeed and distance to hide point is small.
	IF_CAN_HIDE,						// Jumps if the last "hide" goal operation has succeed.
	IF_CANNOT_HIDE,						// Jumps if the last "hide" goal operation has failed.
	IF_STANCE_IS,						// Jumps if the stance of this PipeUser is equal to value specified as 5-th argument.
	IF_FIRE_IS,							// Jumps if the argument of the last "firecmd" on this PipeUser is equal to 5-th argument.
	IF_HAS_FIRED,						// Jumps if the PipeUser just fired - fire flag passed to actor.
	IF_NO_LASTOP,						// Jumps if m_pLastOpResult is NULL (for example if "locate" goalOp returned NULL).
	IF_SEES_LASTOP,						// Jumps if m_pLastOpResult is visible from here.
	IF_SEES_TARGET,						// Jumps if attention target is visible from here.
	IF_CAN_SHOOT_TARGET,				// Jumps if nothing between weapon and attention target (can shoot).
	IF_CAN_MELEE,						// If current weapon has melee fire mode.
	IF_NO_ENEMY_TARGET,					// Jumps if it hasn't got an enemy attention target.
	IF_PATH_LONGER,						// Jumps if current path is longer than 5-th argument.
	IF_PATH_SHORTER,					// Jumps if current path is shorter than 5-th argument.
	IF_PATH_LONGER_RELATIVE,			// Jumps if current path is longer than (5-th argument) times the distance to requested destination.
	IF_NAV_WAYPOINT_HUMAN,				// Jumps if the current navigation graph is waypoint (use for checking indoor).
	IF_NAV_TRIANGULAR,					// Jumps if the current navigation graph is triangular (use for checking outdoor).
	IF_TARGET_DIST_LESS,				// Jumps if the distance to target is less.
	IF_TARGET_DIST_LESS_ALONG_PATH,		// Jumps if the distance along path to target is less.
	IF_TARGET_DIST_GREATER,				// Jumps if the distance to target is more.
	IF_TARGET_IN_RANGE,					// Jumps if the distance to target is less than the attackRannge.
	IF_TARGET_OUT_OF_RANGE,				// Jumps if the distance to target is more than the attackRannge.
	IF_TARGET_TO_REFPOINT_DIST_LESS,	// Jumps if the distance between target and refpoint is less.
	IF_TARGET_TO_REFPOINT_DIST_GREATER,	// Jumps if the distance between target and refpoint is more.
	IF_TARGET_LOST_TIME_MORE,			// Jumps if target lost time is more.
	IF_TARGET_LOST_TIME_LESS,			// Jumps if target lost time is less.
	IF_LASTOP_DIST_LESS,				// Jumps if the distance to last op result is less.
	IF_LASTOP_DIST_LESS_ALONG_PATH,		// Jumps if the distance to last op result along path is less.
	IF_TARGET_MOVED_SINCE_START,		// Jumps if the distance between current targetPos and targetPos when pipe started more than threshold.
	IF_TARGET_MOVED,					// Jumps if the distance between current targetPos and targetPos when pipe started (or last time it was checked) more than threshold.
	IF_EXPOSED_TO_TARGET,				// Jumps if the upper torso of the agent is visible towards the target.
	IF_COVER_COMPROMISED,				// Jumps if the current cover cannot be used for hiding or if the hide spots does not exists.
	IF_COVER_NOT_COMPROMISED,			// Negated version of IF_COVER_COMPROMISED.
	IF_COVER_SOFT,						// If current cover is soft cover.
	IF_COVER_NOT_SOFT,					// If current cover is not soft cover.
	IF_CAN_SHOOT_TARGET_PRONED,
	IF_CAN_SHOOT_TARGET_CROUCHED,
	IF_CAN_SHOOT_TARGET_STANDING,
	IF_COVER_FIRE_ENABLED,				// If cover fire is not enabled .
	IF_RANDOM,
	IF_LASTOP_FAILED,
	IF_LASTOP_SUCCEED,
	BRANCH_ALWAYS,						// Unconditional jump.
	NOT = 0x100,
};

enum EAnimationMode
{
	AIANIM_INVALID,
	AIANIM_SIGNAL = 1,		// 
	AIANIM_ACTION,		
};

enum EStance
{
	STANCE_NULL = -1,
	STANCE_STAND = 0,
	STANCE_CROUCH = 1,
	STANCE_PRONE = 2,
	STANCE_RELAXED = 3,
	STANCE_STEALTH = 4,
	STANCE_LOW_COVER = 5,
	STANCE_HIGH_COVER = 6,
	STANCE_SWIM = 7,
	STANCE_ZEROG = 8,
	// This value must be last
	STANCE_LAST
};

inline const char* GetStanceName(int stance)
{
	switch(stance)
	{
	case STANCE_NULL:
		return "Null";
	case STANCE_STAND:
		return "Stand";
	case STANCE_CROUCH:
		return "Crouch";
	case STANCE_PRONE:
		return "Prone";
	case STANCE_RELAXED:
		return "Relaxed";
	case STANCE_STEALTH:
		return "Stealth";
	case STANCE_LOW_COVER:
		return "LowCover";
	case STANCE_HIGH_COVER:
		return "HighCover";
	case STANCE_SWIM:
		return "Swim";
	case STANCE_ZEROG:
		return "Zero-G";
	default:
		return "<Unknown>";
	}
};


// Description:
//		Look style essentially define the transition style when look target is set.
//		Hard style is fast (surprised, startled), while soft is slow, relaxed.
//		NOLOWER variations only turn upper body, do not change actual body direction.
enum ELookStyle
{
	LOOKSTYLE_DEFAULT = 0,
	LOOKSTYLE_HARD,
	LOOKSTYLE_HARD_NOLOWER,
	LOOKSTYLE_SOFT,
	LOOKSTYLE_SOFT_NOLOWER,
	LOOKSTYLE_COUNT
};

enum EFireMode
{
	FIREMODE_OFF = 0,					// Do not fire.
	FIREMODE_BURST = 1,					// Fire in bursts - living targets only.
	FIREMODE_CONTINUOUS = 2,			// Fire continuously - living targets only.
	FIREMODE_FORCED = 3,				// Fire continuously - allow any target.
	FIREMODE_AIM = 4,					// Aim target only - allow any target.
	FIREMODE_SECONDARY = 5,				// Fire secondary weapon (grenades,....).
	FIREMODE_SECONDARY_SMOKE = 6,		// Fire smoke grenade.
	FIREMODE_MELEE = 7,					// Melee.
	FIREMODE_KILL = 8,					// No missing, shoot directly at the target, no matter what aggression/attackRange/accuracy is.
	FIREMODE_BURST_WHILE_MOVING = 9,	// [mikko] to be renamed.
	FIREMODE_PANIC_SPREAD = 10,
	FIREMODE_BURST_DRAWFIRE = 11,
	FIREMODE_MELEE_FORCED = 12,	// Melee, without distance restrictions.
	FIREMODE_BURST_SNIPE = 13,
	FIREMODE_AIM_SWEEP = 14,
	FIREMODE_BURST_ONCE = 15, // Fire a single burst, then go to FIREMODE_OFF.
};

enum EAIGoalFlags
{ // Todo: names are not coherent, but they can used together; rename all of them?


	AILASTOPRES_USE			= 0x01, //
	AILASTOPRES_LOOKAT	= 0x02,
	AI_LOOK_FORWARD			= 0x04, 
	AI_DONT_STEER_AROUND_TARGET = 0x08,		// Used in stick goal (not using AI_MOVE_BACKWARD) .
	AI_MOVE_BACKWARD = 0x08,				// Default direction.
	AI_MOVE_RIGHT = 0x10,
	AI_MOVE_LEFT =0x20,
	AI_MOVE_FORWARD = 0x40,
	AI_MOVE_BACKRIGHT =0x80,
	AI_MOVE_BACKLEFT =0x100,
	AI_MOVE_TOWARDS_GROUP = 0x200,			// Towards the other group members.
	AI_REQUEST_PARTIAL_PATH = 0x400,
	AI_BACKOFF_FROM_TARGET = 0x800,
	AI_BREAK_ON_LIVE_TARGET = 0x1000,
	AI_RANDOM_ORDER = 0x2000,
	AI_CONSTANT_SPEED = 0x2000,				// Used in stick goal (not using AI_RANDOM_ORDER) .
	AI_USE_TIME = 0x4000,					// When param can be interpreted as time or distance, use time.
	AI_STOP_ON_ANIMATION_START = 0x8000,
	AI_USE_TARGET_MOVEMENT = 0x10000,		// When requesting a direction (AI_MOVE_LEFT/RIGHT etc), make it relative to target movement direction.
	AI_ADJUST_SPEED = 0x20000,				// Used in stick goal forces to adjust the speed based on the target.
	AI_CHECK_SLOPE_DISTANCE = 0x40000,		// Used in backoff goal, checks the actual distance on slopes rather than horizontal.
};

// Description:
//		Unit properties for group behavior (see CLeader/CLeaderAction).
//		These bit flags can be combined based on the unit capabilities.
enum EUnitProperties
{
	UPR_COMBAT_GROUND = 1,			// The unit can take part in ground actions.
	UPR_COMBAT_FLIGHT	= 2,		// The unit can take part in flight actions.
	UPR_COMBAT_MARINE	= 4,		// The unit can take part in marine actions.
	UPR_COMBAT_RECON	= 8,		// The unit can take part in recon actions.
	UPR_ALL						= -1,
};

enum EAIUseAction
{
	AIUSEOP_NONE,
	AIUSEOP_PLANTBOMB,
	AIUSEOP_VEHICLE,
	AIUSEOP_RPG
};

enum	ELeaderAction{
	LA_NONE,
	LA_HIDE,
	LA_HOLD,
	LA_ATTACK,
	LA_SEARCH,
	LA_FOLLOW,
	LA_USE,
	LA_USE_VEHICLE,
	LA_LAST,	// Make sure this one is always the last!
};

enum	ELeaderActionSubType{
	LAS_DEFAULT=0,
	LAS_ATTACK_ROW,
	LAS_ATTACK_CIRCLE,
	LAS_ATTACK_FLANK,
	LAS_ATTACK_FLANK_HIDE,
	LAS_ATTACK_FOLLOW_LEADER,
	LAS_ATTACK_FUNNEL,
	LAS_ATTACK_LEAPFROG,
	LAS_ATTACK_HIDE_COVER,
	LAS_ATTACK_FRONT,
	LAS_ATTACK_CHAIN,
	LAS_ATTACK_COORDINATED_FIRE1,
	LAS_ATTACK_COORDINATED_FIRE2,
	LAS_ATTACK_USE_SPOTS,
	LAS_ATTACK_HIDE,
	LAS_ATTACK_SWITCH_POSITIONS,
	LAS_ATTACK_CHASE,
	LAS_SEARCH_DEFAULT,
	LAS_SEARCH_COVER,
	LAS_LAST	// Make sure this one is always the last!
};

enum EAIAlertStatus
{
	AIALERTSTATUS_SAFE,
	AIALERTSTATUS_UNSAFE,
	AIALERTSTATUS_READY,
	AIALERTSTATUS_ACTION
};

enum EAITargetThreat
{
	AITHREAT_NONE,
	AITHREAT_INTERESTING,
	AITHREAT_THREATENING,
	AITHREAT_AGGRESSIVE,
	AITHREAT_LAST	// For serialization.
};

enum EAITargetType
{
	// Atomic types.
	AITARGET_NONE,				// No target.
	AITARGET_SOUND,				// Primary sensory from sound event.
	AITARGET_MEMORY,			// Primary sensory from vis event, not visible.
	AITARGET_VISUAL,			// Primary sensory from vis event, visible.

	// Backwards compatibility for scriptbind.
	AITARGET_ENEMY,
	AITARGET_FRIENDLY,
	AITARGET_BEACON,
	AITARGET_GRENADE,
	AITARGET_RPG,

	// For serialization.
	AITARGET_LAST	
};

enum EAITargetZone
{
	AIZONE_IGNORE = 0,			// Ignoring target zones

	AIZONE_KILL,
	AIZONE_COMBAT_NEAR,
	AIZONE_COMBAT_FAR,
	AIZONE_WARN,
	AIZONE_OUT,

	AIZONE_LAST
};

enum EAIWeaponAccessories
{
	AIWEPA_NONE = 0,
	AIWEPA_LASER = 0x0001,
	AIWEPA_COMBAT_LIGHT = 0x0002,
	AIWEPA_PATROL_LIGHT = 0x0004,
};

// Summary:
//	 Enum used as flags for entity searching.
enum EAIEntityFilter
{
	AIEF_INVALID = -1,
	AIEF_ANY,
	AIEF_GROUP,
	AIEF_SPECIES,
};

// Summary:
//	 Enum used as flags for entity searching.
enum EAIEntitySearchFlag
{
	AIESF_INVALID = -1,
	AIESF_NEAREST = 1,
	AIESF_FURTHEST,
	AIESF_RANDOM,
};

struct IAIObject;
struct IAIActor;
struct IAIGroup;

// Note:
//	 In practice, rather than modifying/setting all these values explicitly when
//	 making a pathfind request, I suggest you set up bunch of constant sets
//	 of properties, and use them.
struct AgentPathfindingProperties
{
	AgentPathfindingProperties(
		IAISystem::tNavCapMask navCapMask, 
		float triangularResistanceFactor,
		float waypointResistanceFactor,
		float flightResistanceFactor,
		float volumeResistanceFactor,
		float roadResistanceFactor,
		float waterResistanceFactor,
		float maxWaterDepth, 
		float minWaterDepth,
		float exposureFactor,
		float dangerCost,
		float zScale,
		uint32 _customNavCapsMask = 0,
		float radius = 0.3f,
		float height = 2.0f,
		float maxSlope = 0.0f,
		int id = -1,
		bool avoidObstacles = true)
		: navCapMask(navCapMask), triangularResistanceFactor(triangularResistanceFactor), waypointResistanceFactor(waypointResistanceFactor),
		flightResistanceFactor(flightResistanceFactor), volumeResistanceFactor(volumeResistanceFactor), roadResistanceFactor(roadResistanceFactor),
		waterResistanceFactor(waterResistanceFactor), maxWaterDepth(maxWaterDepth), minWaterDepth(minWaterDepth),
		exposureFactor(exposureFactor), dangerCost(dangerCost),
		zScale(zScale), customNavCapsMask(_customNavCapsMask),
		radius(radius), height(height), maxSlope(maxSlope), id(id),
		avoidObstacles(avoidObstacles)
		{if (maxWaterDepth < minWaterDepth) maxWaterDepth = minWaterDepth;}

	AgentPathfindingProperties() {SetToDefaults();}

	void SetToDefaults() {
		navCapMask = IAISystem::NAVMASK_ALL;
		triangularResistanceFactor = 0.0f;
		waypointResistanceFactor = 0.0f;
		flightResistanceFactor = 0.0f;
		volumeResistanceFactor = 0.0f;
		roadResistanceFactor = 0.0f;
		waterResistanceFactor = 0.0f;
		maxWaterDepth = 10000.0f;
		minWaterDepth = -10000.0f;
		exposureFactor = 0.0f;
		dangerCost = 0.0f;		// Cost (in meters) per dead body in the destination node!
		zScale = 1.0f;
		customNavCapsMask = 0;
		radius = 0.3f;
		height = 2.0f;
		maxSlope = 0.0f;
		id = -1;
		avoidObstacles = true;
	}

	// Description:
	//	 Kind of nodes this agent can traverse. 
	//	 Expected to normally be set just once when the agent is created.
	IAISystem::tNavCapMask navCapMask;

	// Description:
	//	 Basic extra costs associated with traversing each node type.
	//	 Everything below here could be reasonably set/modified each time
	//	 a pathfind request is made.
	float triangularResistanceFactor;
	float waypointResistanceFactor;
	float flightResistanceFactor;
	float volumeResistanceFactor;
	float roadResistanceFactor;
	// Summary:
	//	 Additional cost modifiers.
	float waterResistanceFactor;
	// Summary:
	//	 Only travel if water depth is between min and max values.
	float maxWaterDepth;
	float minWaterDepth;
	float exposureFactor;
	float dangerCost;
	// Summary:
	//	 Quantity the z component should be scaled by when calculating distances.
	float zScale;
	uint32 customNavCapsMask;
	
	float radius;
	float height;
	float maxSlope;

	// NOTE Aug 17, 2009: <pvl> unique id of this instance among others.  If there
	// were an array of these structs this could have been an index into that array.
	int id;
	bool avoidObstacles;
};

struct AgentMovementSpeeds
{
	enum EAgentMovementUrgency {AMU_SLOW, AMU_WALK, AMU_RUN, AMU_SPRINT, AMU_NUM_VALUES};
	enum EAgentMovementStance {AMS_RELAXED, AMS_COMBAT, AMS_STEALTH, AMS_LOW_COVER, AMS_HIGH_COVER, AMS_CROUCH, AMS_PRONE, AMS_SWIM, AMS_NUM_VALUES};

	struct SSpeedRange
	{
		float def, min, max;
	};

	AgentMovementSpeeds()
	{
		memset(this, 0, sizeof(*this));
	}

	void SetBasicSpeeds(float slow, float walk, float run, float sprint, float maneuver)
	{
		for (int s = 0 ; s < AMS_NUM_VALUES; ++s)
		{
			speedRanges[s][AMU_SLOW].def = slow;
			speedRanges[s][AMU_SLOW].min = maneuver;
			speedRanges[s][AMU_SLOW].max = slow;

			speedRanges[s][AMU_WALK].def = walk;
			speedRanges[s][AMU_WALK].min = maneuver;
			speedRanges[s][AMU_WALK].max = walk;

			speedRanges[s][AMU_RUN].def = run;
			speedRanges[s][AMU_RUN].min = maneuver;
			speedRanges[s][AMU_RUN].max = run;

			speedRanges[s][AMU_SPRINT].def = sprint;
			speedRanges[s][AMU_SPRINT].min = maneuver;
			speedRanges[s][AMU_SPRINT].max = sprint;
		}

	}

	void SetRanges(int stance, int urgency, float sdef, float smin, float smax)
	{
		assert(stance >= 0 && stance < AMS_NUM_VALUES);
		assert(urgency >= 0 && urgency < AMU_NUM_VALUES);
		speedRanges[stance][urgency].def = sdef;
		speedRanges[stance][urgency].min = smin;
		speedRanges[stance][urgency].max = smax;
	}

	void CopyRanges(int stance, int toUrgency, int fromUrgency)
	{
		assert(stance >= 0 && stance < AMS_NUM_VALUES);
		assert(toUrgency >= 0 && toUrgency < AMU_NUM_VALUES);
		assert(fromUrgency >= 0 && fromUrgency < AMU_NUM_VALUES);
		speedRanges[stance][toUrgency] = speedRanges[stance][fromUrgency];
	}

	const SSpeedRange& GetRange(int stance, int urgency) const
	{
		assert(stance >= 0 && stance < AMS_NUM_VALUES);
		assert(urgency >= 0 && urgency < AMU_NUM_VALUES);
		return speedRanges[stance][urgency];
	}

	// (MATT) Non-const version needed for serialisation {2009/04/23}
	SSpeedRange& GetRange(int stance, int urgency)
	{
		assert(stance >= 0 && stance < AMS_NUM_VALUES);
		assert(urgency >= 0 && urgency < AMU_NUM_VALUES);
		return speedRanges[stance][urgency];
	}

private:
	SSpeedRange speedRanges[AMS_NUM_VALUES][AMU_NUM_VALUES];
};

enum EAgentAvoidanceAbilities
{
	eAvoidance_NONE				= 0,

	eAvoidance_Vehicles			= 0x01,		// Agent can avoid vehicles
	eAvoidance_Puppets			= 0x02,		// Agent can avoid puppets
	eAvoidance_Players			= 0x04,		// Agent can avoid players

	eAvoidance_StaticObstacle	= 0x10,		// Agent can avoid static physical objects (non-pushable)
	eAvoidance_PushableObstacle	= 0x20,		// Agent can avoid pushable objects

	eAvoidance_DamageRegion		= 0x100,	// Agent can avoid damage regions

	eAvoidance_ALL				= 0xFFFF,
	eAvoidance_DEFAULT			= eAvoidance_ALL, // Avoid all by default
};

// Description:
//	 Structure containing all the relevant information describing the way agent moves (can strafe,speeds,inertia,...).
struct AgentMovementAbility
{
	bool b3DMove;
	bool bUsePathfinder;
	bool usePredictiveFollowing;
	bool allowEntityClampingByAnimation;
	float maxAccel;
	float maxDecel;
	float	minTurnRadius;
	float	maxTurnRadius;
	float avoidanceRadius;
	float pathLookAhead;				// How far to look ahead when path following.
	float pathRadius;					// How wide the path should be treated.
	float pathSpeedLookAheadPerSpeed;	// How far to look ahead to do speed control (mult this by the speed); -ve means use pathLookAhead.
	float cornerSlowDown;				// Slow down at corners: 0 to 1.
	float slopeSlowDown;				// Slow down on slopes uphill (0 to 1).
	float optimalFlightHeight;
	float minFlightHeight;
	float maxFlightHeight;
	float	maneuverTrh;				// Cosine of forward^desired angle, after which to start maneuvering
	float	velDecay;					// How fast velocity decays with cos(fwdDir^pathDir) .
	float	pathFindPrediction;			// Time in seconds how much the path finder will predict the start of the path.
	// Description:
	//	 Enables/disables the path getting regenerated during tracing (needed for crowd control and dynamic
	//	 updates to graph) - regeneration interval (in seconds) - 0 disables.
	float pathRegenIntervalDuringTrace;
	// Description:
	//	 Enables/disables teleporting when path following.
	bool teleportEnabled;
	// Description:
	//	 Sets to true if the movement speed should be lowered in low light conditions.
	bool  lightAffectsSpeed;
	// Description:
	//	 Adjusts the movement speed based on the angel between body dir and move dir.
	//	 Enables/disables the algorithm to attempt to handle the agent getting stuck during tracing.
	bool  resolveStickingInTrace;
	float directionalScaleRefSpeedMin;
	float directionalScaleRefSpeedMax;
	// Description:
	//	Adjusts how the agent pathfollows around dynamic obstacles, both pushable and non-pushable
	uint32 avoidanceAbilities;
	bool  pushableObstacleWeakAvoidance;
	float pushableObstacleAvoidanceRadius;

	AgentMovementSpeeds movementSpeeds;

	AgentPathfindingProperties pathfindingProperties;

	AgentMovementAbility():b3DMove(false),bUsePathfinder(true), usePredictiveFollowing(false), allowEntityClampingByAnimation(false),
		maxAccel(std::numeric_limits<float>::max()), maxDecel(std::numeric_limits<float>::max()),
		minTurnRadius(0), maxTurnRadius(0),
		avoidanceRadius(0), pathLookAhead(3.0f), pathRadius(1.0f), pathSpeedLookAheadPerSpeed(-1), cornerSlowDown(0.0f), slopeSlowDown(1.f),
		optimalFlightHeight(0), minFlightHeight(0), maxFlightHeight(0), maneuverTrh(0.f),
		velDecay(0.0f), pathFindPrediction(0), resolveStickingInTrace(true), pathRegenIntervalDuringTrace(0.0f),
		teleportEnabled(false), lightAffectsSpeed(false),
		directionalScaleRefSpeedMin(-1.0f), directionalScaleRefSpeedMax(-1.0f),
		avoidanceAbilities(eAvoidance_DEFAULT), pushableObstacleWeakAvoidance(false), pushableObstacleAvoidanceRadius(0.35f)

	{}
};


struct AIObjectParameters
{
	AgentParameters m_sParamStruct;
	AgentMovementAbility m_moveAbility;

	AIObjectParameters() {}
	AIObjectParameters(const AgentParameters& sParamStruct, const AgentMovementAbility& moveAbility)
		: m_sParamStruct(sParamStruct), m_moveAbility(m_moveAbility) {}
};

struct AIWeaponDescriptor
{
	string	firecmdHandler;
	float	fSpeed;						// Relevant for projectiles only.
	float	fDamageRadius;				// Explosion radius for rockets/grenades.
	float	fChargeTime;
	float	fRangeMax;
	float	fRangeMin;
	bool	bSignalOnShoot;
	int burstBulletCountMin;			// The burst bullet count is scaled between min and max based on several factors (distance, visibility).
	int burstBulletCountMax;			// Each burst is followed by a pause.
	float burstPauseTimeMin;			// The burst pause is scale between min and max.
	float burstPauseTimeMax;
	float singleFireTriggerTime;		// Interval between single shot shots. Scale randomly between 70-125%. Set to -1 for continuous fire.
	float spreadRadius;					// Miss spread radius in meters.
	float coverFireTime;				// How long the shooting should continue after the target is not visible anymore.
	float drawTime;						// How long the AI will shoot around the target before aiming it precisely 
	float sweepWidth;					// How wide is the sweep left/right around the target direction during 
	float sweepFrequency;				// How fast the sweep left/right around the target direction during
	Vec3 projectileGravity;		// What's the gravity of the fired projectile - Used only for predicting grenade landing
	string smartObjectClass;

	AIWeaponDescriptor(const string& fcHandler="instant", float chargeTime=-1.0f,float speed=-1.0f, float damageRadius=-1.0f):
	firecmdHandler(fcHandler),fSpeed(speed),fDamageRadius(damageRadius),fChargeTime(chargeTime),
		fRangeMax(1000.f),fRangeMin(1.f),bSignalOnShoot(false), burstBulletCountMin(1), burstBulletCountMax(10),
		burstPauseTimeMin(0.8f), burstPauseTimeMax(3.5f), singleFireTriggerTime(-1.0f), spreadRadius(1.0f), coverFireTime(2.0f),
		drawTime(3.0f), sweepWidth(2.5f), sweepFrequency(3.0f), projectileGravity(0.0f, 0.0f, -9.8f)
	{
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(smartObjectClass);
		pSizer->AddObject(firecmdHandler);
	}
};

// Memory fire control
enum EMemoryFireType
{
	eMFT_Disabled = 0,			// Never allowed to fire at memory
	eMFT_UseCoverFireTime,		// Can fire at memory using the weapon's cover fire time
	eMFT_Always,				// Always allowed to fire at memory

	eMFT_COUNT,
};

// Summary:
//	 Interface to the fire command handler.
struct IFireCommandHandler
{
	// Summary:
	//	 Gets the name identifier of the handler.
	virtual const char*	GetName() = 0;
	// Description:
	//	 Reset is called each time.
	virtual void	Reset() = 0;
	// Description:
	//	 Update is called on each AI update when the target is valid.
	virtual bool	Update(IAIObject* pTarget, bool canFire, EFireMode fireMode, const AIWeaponDescriptor& descriptor, Vec3& outShootTargetPos) = 0;
	// Description:
	//	 Checks if it's ok to shoot with this weapon in fireVector - to see if no friends are hit, no ostacles nearby hit, etc.
	virtual bool	ValidateFireDirection(const Vec3& fireVector, bool mustHit) = 0;
	// Description:
	//	 Deletes the handler.
	virtual void Release() = 0;
	// Description:
	//	 Draws debug information.
	virtual void DebugDraw() = 0;
	// Description:
	//	 Returns true if default effect should be used for special firemode.
	virtual bool UseDefaultEffectFor(EFireMode fireMode) const = 0;
	// Description:
	//	 Called whenever weapon is reloaded.
	// Summary:
	//	 Reloads a weapon.
	virtual void OnReload() = 0;
	// Description:
	//	 Called whenever shot is done (if weapon descriptor defines EnableWeaponListener).
	// Summary:
	//	 Shot event function.
	virtual void OnShoot() = 0;
	// Description:
	//	 Returns how many shots were done.
	virtual int	 GetShotCount() const = 0;
	
};

// Description:
//	 Fire command handler descriptor/factory.
struct IFireCommandDesc
{
	//	 Returns the name identifier of the handler.
	virtual const char*	GetName() = 0;
	// Summary:
	//	 Creates new instance of a fire command handler.
	virtual IFireCommandHandler*	Create(IAIActor *pShooter) = 0;
	// Summary:
	//	 Deletes the factory.
	virtual void Release() = 0;
};

// Description:
//	 Helper class for declaring fire command descriptors.
template <class T>
class CFireCommandDescBase : public IFireCommandDesc
{
public:
	CFireCommandDescBase(const char* name) : m_name(name) {};
	virtual const char*	GetName() { return m_name.c_str(); }
	virtual IFireCommandHandler*	Create(IAIActor* pOwner) { return new T(pOwner); }
	virtual void Release() { delete this; }
	
protected:
	string	m_name;
};
#define CREATE_FIRECOMMAND_DESC(name, type) new CFireCommandDescBase<type>((name))


// Description:
//	 Structure that contains critical hidespot information.
struct SHideSpotInfo
{
	enum EHideSpotType
	{
		eHST_TRIANGULAR,
		eHST_WAYPOINT,
		eHST_ANCHOR,
		eHST_SMARTOBJECT,
		eHST_VOLUME,
		eHST_DYNAMIC,
		eHST_INVALID,
	};

	EHideSpotType type;
	Vec3 pos;
	Vec3 dir;

	SHideSpotInfo() : type(eHST_INVALID), pos(ZERO), dir(ZERO) {}
	SHideSpotInfo(EHideSpotType type, const Vec3& pos, const Vec3& dir) : type(type), pos(pos), dir(dir) {}
};

// Description:
//	 Memento used in CanTargetPointBeReached etc.
class CTargetPointRequest
{
public:
	CTargetPointRequest() : result(eTS_false), pathID(-1) {}
	CTargetPointRequest(const Vec3 &targetPoint, bool continueMovingAtEnd = true) 
		: targetPoint(targetPoint), pathID(-1), continueMovingAtEnd(continueMovingAtEnd) {}
	ETriState GetResult() const {return result;}
	const Vec3& GetPosition() const {return targetPoint;}
	void SetResult(ETriState res) {result = res;}
private:
	// Data is internal to AI (CNavPath)!
	friend class CNavPath;
	Vec3 targetPoint;
	Vec3 splitPoint;
	int itIndex;
	int itBeforeIndex;
	// Summary:
	//	 Used to identify the path this was valid for.
	int pathID;
	bool continueMovingAtEnd;
	ETriState result;
};


#include "IGoalPipe.h"


struct SAIEVENT
{
	bool bFuzzySight;
	bool bPathFound;
	int nDeltaHealth;
	float fThreat;
	int nType;
	int nFlags;
	Vec3 vPosition;
	Vec3 vForcedNavigation;
	EntityId sourceId;
	EntityId targetId;

	SAIEVENT():bFuzzySight(false),nDeltaHealth(0),fThreat(0.f),nType(0),nFlags(0),
		bPathFound(false),vForcedNavigation(ZERO),sourceId(0),targetId(0){}

};

enum EObjectResetType
{
	AIOBJRESET_INIT,
	AIOBJRESET_SHUTDOWN,
};


struct SOBJECTSTATE;
struct AISIGNAL_EXTRA_DATA;


class CAIActor;
class CAIAttribute;
class CAIPlayer;
class CLeader;
struct IPipeUser;
class CPipeUser;
struct IPuppet;
class CPuppet;
struct IAIVehicle;
class CAIVehicle;
struct IBlackBoard;
struct IPerceptionHandlerModifier;
struct IAIRateOfDeathHandler;




typedef enum EBehaviorEvent
{
	BehaviorStarted,
	BehaviorInterrupted,
	BehaviorFinished,
	BehaviorFailed,
};

struct IActorBehaviorListener;




struct IActorBehaviorListener
{
	virtual void BehaviorEvent(IAIObject* actor, EBehaviorEvent event) = 0;
	virtual void BehaviorChanged(IAIObject* actor, const char* current, const char* previous) = 0;
};


struct IPipeUser;

enum EGoalPipeEvent
{

	ePN_OwnerRemoved,  // Sent if pipeUser is removed.
	ePN_Deselected,    // Sent if replaced by selecting other pipe.
	ePN_Finished,      // Sent if reached end of pipe.
	ePN_Suspended,     // Sent if other pipe was inserted.
	ePN_Resumed,       // Sent if resumed after finishing inserted pipes.
	ePN_Removed,       // Sent if inserted pipe was removed with RemovePipe().

	ePN_AnimStarted,   // Sent when exact positioning animation is started.
	ePN_RefPointMoved, // Sent to the last inserted goal pipe when the ref. point is moved.
};

struct IGoalPipeListener;

// Todo: Figure out better way to handle this, the structure is almost 1:1 to the SActorTargetParams.
typedef uint32 TAnimationGraphQueryID;
struct SAIActorTargetRequest
{
	SAIActorTargetRequest() :
		id(0),
		approachLocation(0,0,0),
		approachDirection(0,0,0),
		animLocation(0,0,0),
		animDirection(0,0,0),
		animFinalLocation(0,0,0),
		vehicleSeat(0),
		speed(0),
		directionTolerance(0),
		startArcAngle(0),
		startWidth(0),
		signalAnimation(true),
		projectEndPoint(true),
		lowerPrecision(false),
		useAssetAlignment(false),
		stance(STANCE_NULL),
		pQueryStart(0),
		pQueryEnd(0)
	{
	}

	void Reset()
	{
		id = 0;
		approachLocation.Set(0,0,0);
		approachDirection.Set(0,0,0);
		animLocation.Set(0,0,0);
		animDirection.Set(0,0,0);
		//do not reset this, it's needed as "last position"
		//animFinalLocation.Set(0,0,0);
		vehicleSeat = 0;
		speed = 0;
		directionTolerance = 0;
		startArcAngle = 0;
		startWidth = 0;
		signalAnimation = true;
		projectEndPoint = true;
		lowerPrecision = false;
		useAssetAlignment = false;
		stance = STANCE_NULL;
		pQueryStart = 0;
		pQueryEnd = 0;
		vehicleName = "";
		animation = "";
	}

	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

	int id;	// id=0 means invalid
	Vec3 approachLocation;
	Vec3 approachDirection;
	Vec3 animLocation;
	Vec3 animDirection;
	Vec3 animFinalLocation;
	string vehicleName;
	int vehicleSeat;
	float speed;
	float directionTolerance;
	float startArcAngle;
	float startWidth;
	bool signalAnimation;
	bool projectEndPoint;
	bool lowerPrecision; // Lower precision should be true when passing through a navSO.
	bool useAssetAlignment;
	string animation;
	EStance stance;
	TAnimationGraphQueryID * pQueryStart;
	TAnimationGraphQueryID * pQueryEnd;
};

struct IPipeUser
{
	virtual ~IPipeUser() {}
	virtual bool SelectPipe(int id, const char *name, IAIObject *pArgument = 0, int goalPipeId = 0, bool resetAlways = false)=0;
	virtual IGoalPipe* InsertSubPipe(int id, const char *name, IAIObject *pArgument = 0, int goalPipeId = 0)=0;
	virtual bool CancelSubPipe(int goalPipeId)=0;
	virtual bool RemoveSubPipe(int goalPipeId, bool keepInserted = false)=0;
	virtual bool IsUsingPipe(const char *name) const = 0;
	virtual bool IsUsingPipe(int goalPipeId) const = 0;

	virtual void Pause(bool pause) = 0;
	virtual bool IsPaused() const = 0;

	virtual IAIObject *GetAttentionTarget() const =0;
	virtual IAIObject *GetAttentionTargetAssociation() const =0;
	virtual EAITargetThreat GetAttentionTargetThreat() const = 0;
	virtual EAITargetType GetAttentionTargetType() const = 0;
	virtual void SetAttentionTarget(IAIObject *pObject) {;}
	virtual IAIObject *GetLastOpResult()=0;
	virtual void ResetCurrentPipe( bool bAlways) {;}
	virtual IAIObject* GetSpecialAIObject(const char* objName, float range = 0.0f) =0;
	virtual void MakeIgnorant(bool ignorant) = 0;
	virtual bool SetCharacter(const char *character, const char* behaviour = NULL)=0;
	virtual bool IsCoverCompromised() = 0;
	virtual bool IsTakingCover(float distanceThreshold) = 0;
	virtual bool IsMovingToCover() = 0;

	//virtual void Devalue(IAIObject *pObject, bool bDevaluePuppets) = 0;
	virtual bool IsDevalued(IAIObject *pObject) = 0;
	virtual void ClearDevalued() = 0;

	virtual void RegisterGoalPipeListener( IGoalPipeListener* pListener, int goalPipeId, const char* debugClassName ) = 0;
	virtual void UnRegisterGoalPipeListener( IGoalPipeListener* pListener, int goalPipeId ) = 0;
	virtual int GetGoalPipeId() const = 0;

	virtual void ResetLookAt() = 0;
	virtual bool SetLookAtPointPos( const Vec3& point, bool priority=false ) = 0;
	virtual bool SetLookAtDir( const Vec3& dir, bool priority=false ) = 0;

	virtual void ResetBodyTargetDir() = 0;
	virtual void SetBodyTargetDir( const Vec3& dir ) = 0;
	virtual const Vec3 &GetBodyTargetDir() const = 0;

	virtual void SetAllowedStrafeDistances(float start, float end, bool whileMoving) = 0;

	virtual void SetExtraPriority( float priority ) = 0;
	virtual float GetExtraPriority(void) = 0;

	virtual IAIObject* GetRefPoint()=0;
	virtual void SetRefPointPos(const Vec3 &pos)=0; 
	virtual void SetRefPointPos(const Vec3 &pos, const Vec3 &dir)=0;
	virtual void SetRefShapeName(const char* shapeName) = 0;
	virtual const char* GetRefShapeName() const = 0;

	virtual void SetActorTargetRequest(const SAIActorTargetRequest& req) = 0;

	virtual EntityId GetLastUsedSmartObjectId() const=0;
	virtual void ClearPath( const char* dbgString )=0;

	virtual void SetFireMode(EFireMode mode) = 0;
	virtual EFireMode GetFireMode() const = 0;

	// Summary:
	//	 Adds the current hideobject position to ignore list (will be ignored for specified time).
	virtual void IgnoreCurrentHideObject(float timeOut = 10.0f) = 0;

	// Summary:
	//	 Returns most probable target position or the target if it is visible.
	virtual Vec3 GetProbableTargetPosition() = 0;

	virtual Vec3 GetLastSOExitHelperPos() = 0;

	virtual SHideSpotInfo::EHideSpotType GetCurrentHideObjectPos(Vec3 &hidePos, Vec3 &objectDir) = 0;
};


struct IGoalPipeListener
{
	virtual void OnGoalPipeEvent( IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId ) = 0;

private:
	friend class CPipeUser;

	typedef DynArray< std::pair< IPipeUser*, int > > VectorRegisteredPipes;
	VectorRegisteredPipes _vector_registered_pipes;

protected:
	virtual ~IGoalPipeListener()
	{
		assert( _vector_registered_pipes.empty() );
		while ( _vector_registered_pipes.empty() == false )
		{
			_vector_registered_pipes.back().first->UnRegisterGoalPipeListener( this, _vector_registered_pipes.back().second );
		}
	}
};

struct ISmartObjectManager
{
	virtual void RemoveSmartObject(IEntity *pEntity) = 0;

	virtual const char* GetSmartObjectStateName( int id ) const = 0;

	virtual void RegisterSmartObjectState( const char* sStateName ) = 0;
	virtual void AddSmartObjectState( IEntity* pEntity, const char* sState ) = 0;
	virtual void RemoveSmartObjectState( IEntity* pEntity, const char* sState ) = 0;
	virtual bool CheckSmartObjectStates( IEntity* pEntity, const char* patternStates ) const = 0;
	virtual void SetSmartObjectState( IEntity* pEntity, const char* sStateName ) = 0;
	virtual void ModifySmartObjectStates( IEntity* pEntity, const char* listStates ) = 0;

	virtual void DrawSOClassTemplate( IEntity* pEntity ) = 0;//TODO make const
	virtual void GetSOClassTemplateIStatObj( IEntity* pEntity, IStatObj**& pStatObjects ) = 0;//TODO make this const at some point if possible

	virtual void ReloadSmartObjectRules() = 0;

	virtual int SmartObjectEvent( const char* sEventName, IEntity*& pUser, IEntity*& pObject, const Vec3* pExtraPoint = NULL, bool bHighPriority = false ) = 0;

	virtual SmartObjectHelper* GetSmartObjectHelper( const char* className, const char* helperName ) const = 0;
};

struct IAIAction;

UNIQUE_IFACE struct IAIActionManager : public IGoalPipeListener
{
	// returns an existing AI Action from the library or NULL if not found
	VIRTUAL IAIAction* GetAIAction( const char* sName ) = 0;

	// returns an existing AI Action by its index in the library or NULL index is out of range
	VIRTUAL IAIAction* GetAIAction( size_t index ) = 0;

	VIRTUAL void ReloadActions() = 0;

	// aborts specific AI Action (specified by goalPipeId) or all AI Actions (goalPipeId == 0) in which pEntity is a user
	VIRTUAL void AbortAIAction( IEntity* pEntity, int goalPipeId = 0 ) = 0;

	// finishes specific AI Action (specified by goalPipeId) for the pEntity as a user
	VIRTUAL void FinishAIAction( IEntity* pEntity, int goalPipeId ) = 0;
};


struct SSoundPerceptionDescriptor
{
	float				fMinDist;		// Minimum distance from sound origin to recognize it
	float				fRadiusScale;	// Scalar value applied to radius to amplify or reduce the effect of the sound
	float				fSoundTime;		// How long to remember the sound
	float				fBaseThreat;	// Base threat value
	float				fLinStepMin;	// Linear step ranges used based on ratio of distance to sound and sound radius
	float				fLinStepMax;

	SSoundPerceptionDescriptor() { Set(); }
	void Set(float _fMinDist = 0.0f, float _fRadiusScale = 1.0f, float _fSoundTime = 0.0f, float _fBaseThreat = 0.0f, float _fLinStepMin = 0.0f, float _fLinStepMax = 1.0f)
	{
		fMinDist = _fMinDist;
		fRadiusScale = _fRadiusScale;
		fSoundTime = _fSoundTime;
		fBaseThreat = _fBaseThreat;
		fLinStepMin = _fLinStepMin;
		fLinStepMax = _fLinStepMax;
	}
};


struct IPuppet 
{
	enum EPostureType
	{
		POSTURE_NULL = 0,
		POSTURE_AIM = 1,
		POSTURE_HIDE = 2,
	};

	typedef struct SPostureInfo
	{
		SPostureInfo()
			: type(POSTURE_NULL)
			, lean(0.0f)
			, stance(STANCE_NULL)
			, priority(0.0f)
			, parentId(-1)
			, enabled(true)
		{}

		SPostureInfo(EPostureType _type, const char* _name, float _lean, float _peekOver, EStance _stance, float _priority, const char* _agInput = "", int _parentId = -1)
			: type(_type)
			, lean(_lean)
			, peekOver(_peekOver)
			, stance(_stance)
			, priority(_priority)
			, parentId(_parentId)
			, enabled(true)
			, agInput(_agInput)
			, name(_name)
		{
		}

		void GetMemoryUsage(ICrySizer *pSizer) const
		{
			pSizer->AddObject(agInput);
			pSizer->AddObject(name);
		}
		EPostureType type;
		float lean;
		float peekOver;
		float priority;
		EStance stance;
		int parentId;
		bool enabled;
		string agInput;
		string name;
	};
		
	virtual ~IPuppet() {}
	virtual void UpTargetPriority(const IAIObject *pTarget, float fPriorityIncrement)=0;
	virtual void UpdateBeacon()=0;

	virtual void ResetPostures() = 0;
	virtual int SetPosture(const SPostureInfo& posture) = 0;
	virtual bool GetPosture(int postureId, SPostureInfo* posture = 0) const = 0;
	virtual int GetPostureId(const char* postureName) const = 0;
	virtual bool GetPostureByName(const char* postureName, SPostureInfo* posture = 0) const = 0;
	virtual void SetPosturePriority(int postureId, float priority) = 0;
	virtual float GetPosturePriority(int postureId) const = 0;

	virtual const VisionID& GetVisionID() const = 0;
	virtual bool CanSee(const VisionID& otherVisionID) const = 0;

	virtual IAIObject* MakeMeLeader()=0;

	// Summary:
	//	 Gets the owner of a dummy AI Object
	virtual IAIObject* GetEventOwner(IAIObject *pObject) const = 0;

	// Summary:
	//	 Where to shoot if need to miss now (target is another AI) - select some point around target .
	virtual Vec3 ChooseMissPoint_Deprecated(const Vec3 &targetPos) const = 0;

	// Return Value:
	//	 true and sends signal "OnFriendInWay" if there is a friendly agent in the line of fire.
	// Description: 
	//	 Checks if there are friends in the line of fire.
	//	 If the cheapTest flag is set, only cone test is performed, otherwise raycasting.
	virtual bool CheckFriendsInLineOfFire(const Vec3& fireDirection, bool cheapTest) = 0;

	// Summary:
	//	 Returns a specified point projected on floor/ground.
	virtual Vec3 GetFloorPosition(const Vec3& pos) = 0;

	// Description:
	//	 Gets the distance of an AI object to this, along this path.
	// Note:
	//	 Must be called with bInit=true first time and then false 
	//	 other time to avoid considering path regeneration after.
	virtual float	GetDistanceAlongPath(const Vec3& pos, bool bInit) = 0;
	// Summary: 
	//	 Sets the shape that defines the Puppet territory.
	virtual void SetTerritoryShapeName(const char* shapeName) = 0;
	virtual const char* GetTerritoryShapeName() const = 0;
	virtual const char* GetWaveName() const = 0;
	virtual bool IsPointInsideTerritoryShape(const Vec3& vPos, bool bCheckHeight) const = 0;
	virtual bool ConstrainInsideTerritoryShape(Vec3& vPos, bool bCheckHeight) const = 0;

	virtual void EnableFire(bool enable) =0;
	virtual bool IsFireEnabled() const=0;
	virtual void EnableCoverFire(bool enable) =0;
	virtual bool IsCoverFireEnabled() const =0;
	virtual bool GetPosAlongPath(float dist, bool extrapolateBeyond, Vec3& retPos) const = 0;
	virtual IFireCommandHandler* GetFirecommandHandler() const = 0;

	// Description:
	//	 Sets the aproximation radius where speed starts to fall off (negative values mean no change)
	virtual void SetSpeedFalloffRadius(float fWalk = -1.f, float fRun = -1.0f, float fSprint = -1.0f) {;}
	// Description:
	//	 Changes flag so this puppet can be shoot or not.
	virtual void SetCanBeShot(bool bCanBeShot) {;}
	virtual bool GetCanBeShot() const { return true; }
	// Set how this puppet treats firing at its memory target
	virtual void SetMemoryFireType(EMemoryFireType eType) {;}
	virtual EMemoryFireType GetMemoryFireType() const { return eMFT_UseCoverFireTime; }
	virtual bool CanMemoryFire() const { return true; }

	// Modify perception descriptions for visual and sound stimulus
	virtual bool GetSoundPerceptionDescriptor(EAISoundStimType eType, SSoundPerceptionDescriptor& sDescriptor) const = 0;
	virtual bool SetSoundPerceptionDescriptor(EAISoundStimType eType, const SSoundPerceptionDescriptor& sDescriptor) = 0;

	// Helper to get the puppet's perceived position of an agent
	// Returns TRUE if puppet has a perceived location for this target
	virtual bool GetPerceivedTargetPos(IAIObject *pTarget, Vec3 &vPos) const = 0;

	// ROD Handler
	virtual void SetRODHandler(IAIRateOfDeathHandler *pHandler) = 0;
	virtual void ClearRODHandler() = 0;
};

struct IAIVehicle 
{
	virtual ~IAIVehicle() {}
};

UNIQUE_IFACE struct IAISignalExtraData
{
	virtual void Serialize( TSerialize ser ) = 0;
	virtual const char* GetObjectName() const = 0;
	virtual void SetObjectName( const char* objectName ) = 0;

	// To/from script table
	virtual void ToScriptTable(SmartScriptTable &table) const = 0;
	virtual void FromScriptTable(const SmartScriptTable &table) = 0;

	IAISignalExtraData() : string1( NULL ), string2( NULL ) {}

	Vec3 point;
	Vec3 point2;
	ScriptHandle nID;
	float fValue;
	int iValue;
	int iValue2;
	const char* string1;
	const char* string2;
};


// Put new signal here!


struct AISIGNAL
{
	//AISIGNAL():point(0,0,0){}
	int						nSignal;
	//private:
	uint32				m_nCrcText;

	//CryString				strText;
	EntityId	senderID;
	IAISignalExtraData*		pEData;

	// for 64b alignment
	static const int SIGNAL_STRING_LENGTH = 50;
	char					strText[SIGNAL_STRING_LENGTH];
	// for 512b alignment
	//	char					strText[498];

	inline bool Compare(uint32 crc) const 
	{
		return m_nCrcText == crc;
	}

	AISIGNAL()
	: nSignal(0)
	, senderID(0)
	, pEData(NULL)
	, m_nCrcText(0)
	{
		strText[0]=0;
	}
	void Serialize( TSerialize ser, class CObjectTracker& objectTracker );

};

struct PATHPOINT
{
	Vec3 vPos;
	Vec3 vDir;
	PATHPOINT(const Vec3& pos=ZERO,const Vec3& dir=ZERO):vPos(pos),vDir(dir){}
};
typedef DynArray<PATHPOINT>	PATHPOINTVECTOR;

// Summary:
//	 Current phase of actor target animation
enum EActorTargetPhase
{
	eATP_None,
	eATP_Waiting,
	eATP_Starting,
	eATP_Started,
	eATP_Playing,
	eATP_StartedAndFinished,
	eATP_Finished,
	eATP_Error,
};

enum EAITargetStuntReaction
{
	AITSR_NONE,
	AITSR_SEE_STUNT_ACTION,
	AITSR_SEE_CLOAKED,
	AITSR_LAST
};

// Summary:
//	 Current phase of actor target animation.
enum ERequestedGrenadeType
{
	eRGT_INVALID = -1,

	eRGT_ANY,
	eRGT_SMOKE_GRENADE,
	eRGT_FLASHBANG_GRENADE,
	eRGT_FRAG_GRENADE,
	eRGT_EMP_GRENADE,

	eRGT_COUNT,
};

struct SAIPredictedCharacterState
{
	SAIPredictedCharacterState():predictionTime(0.f),position(0,0,0),velocity(0,0,0){}
	void Set(const Vec3 &pos, const Vec3 vel, float predT) {
		position = pos; velocity = vel; predictionTime = predT;}
	void Serialize( TSerialize ser );

	Vec3 position; 
	Vec3 velocity; 
	float predictionTime; // Time of prediction relative to when it was made.
};

struct SAIPredictedCharacterStates
{
	SAIPredictedCharacterStates():nStates(0){}
	void Serialize( TSerialize ser );

	static const int maxStates = 32;
	SAIPredictedCharacterState states[maxStates];
	int nStates;
};

struct SOBJECTSTATE
{

	// Actor's movement\looking\firing related.
	bool	jump;
	bool	fire;
	bool	fireSecondary;
	bool	fireMelee;

	bool	aimObstructed;
	bool	aimTargetIsValid;	// 
	bool	forceWeaponAlertness;

	ERequestedGrenadeType	requestedGrenadeType;
	int		weaponAccessories;	// Weapon accessories to enable.
	int		bodystate;
	float	lean;
	float peekOver;
	float	fHitProbability;
	float	fProjectileSpeedScale;
	Vec3	vShootTargetPos;	// The requested position to shoot at. This value must be passed directly to weapon system,
	// the AI system has decided already if the shot should miss or hit. Notes: can be (0,0,0) if not begin requested!
	Vec3	vAimTargetPos;		// The requested position to aim at. This value is slightly different from the vShootTarget, and is
	// used for visual representation to where to shoot at. Notes: can be (0,0,0) if not begin requested!
	Vec3	vLookTargetPos;		// The requested position to look at. Notes: can be (0,0,0) if not begin requested!

	ELookStyle eLookStyle;

	Vec3	vMoveDir;
	Vec3	vForcedNavigation;
	Vec3	vBodyTargetDir;
	// Hint to animation system what will happen in the future.
	SAIPredictedCharacterStates predictedCharacterStates;

	// Precision movement support: A replacement for the old velocity based movement system above.
	// NOTE: If the move target and inflection point are equal (and non-zero) they mark the end of the path.
	Vec3 vMoveTarget;				// If non-zero, contains the absolute position the agent should (and can) move towards.
	Vec3 vInflectionPoint;	// If non-zero, contains the estimated future move target once the agent reaches the current move target.

	// Description:
	//		AISPEED_ZERO, AISPEED_SLOW, AISPEED_WALK, AISPEED_RUN, AISPEED_SPRINT. This affects animation and thus speed only indirectly,
	//		due to it affecting the available max/min speeds.
	// See also:
	//		AISPEED_ZERO, AISPEED_SLOW, AISPEED_WALK, AISPEED_RUN, AISPEED_SPRINT
	float fMovementUrgency; 
	float fDesiredSpeed; //< in m/s
	bool allowStrafing;	// Whether strafing is allowed or not.
	bool allowEntityClampingByAnimation;

	PATHPOINTVECTOR	remainingPath;
	float	fDistanceToPathEnd;

	EActorTargetPhase			curActorTargetPhase;								// Return value.
	Vec3									curActorTargetFinishPos;				// Return value.
	SAIActorTargetRequest	actorTargetReq;

	bool	bCloseContact;
	bool	bHurryNow;	// Needs to skip animation transitions - going from idle to combat.
	bool bReevaluate;
	bool bTargetEnabled;
	EAITargetType eTargetType;
	EAITargetThreat eTargetThreat;
	float fDistanceFromTarget;
	EAITargetStuntReaction eTargetStuntReaction;
	int nTargetType;
	DynArray<AISIGNAL> vSignals;
	int nAuxSignal;					// Aux signal id.
	string szAuxSignalText;			// Aux signal name.
	float fAuxDelay;				// Aux signal delay (delay before the signal is being processed).
	int nAuxPriority;				// Aux signal priority (only higher priority will interrupt the current one).

	SOBJECTSTATE():
		vMoveDir(0.0f,0.0f,0.0f),
		vBodyTargetDir(0.0f,0.0f,0.0f),
		vLookTargetPos(0.0f,0.0f,0.0f),
		fMovementUrgency(AISPEED_WALK),
		fDesiredSpeed(1.0f),
		//		bExactPositioning(false),
		fDistanceToPathEnd(-1.0f),
		allowStrafing(false),
		forceWeaponAlertness(false),
		eLookStyle(LOOKSTYLE_DEFAULT)
	{
		FullReset();
	}

	void Reset()
	{
		bCloseContact = false;
		eTargetThreat = AITHREAT_NONE;
		eTargetType = AITARGET_NONE;
		eTargetStuntReaction = AITSR_NONE;
		vMoveDir(0,0,0);
		vForcedNavigation(0.0f, 0.0f, 0.0f);

		predictedCharacterStates.nStates = 0;
		fProjectileSpeedScale=1.f;
		bTargetEnabled = false;
		bHurryNow = false;
	}

	void FullReset()
	{
		Reset();

		vLookTargetPos.Set(0.0f,0.0f,0.0f);
		vBodyTargetDir.Set(0.0f,0.0f,0.0f);
		vShootTargetPos.Set(0.0f, 0.0f, 0.0f);
		vAimTargetPos.Set(0.0f, 0.0f, 0.0f);
		fDistanceToPathEnd = -1.0f;

		vMoveTarget.zero();
		vInflectionPoint.zero();

		jump = false;
		fire = false;
		fireSecondary = false;
		fireMelee = false;
		requestedGrenadeType = eRGT_ANY;
		aimObstructed = false;
		aimTargetIsValid = false;
		weaponAccessories = AIWEPA_NONE;
		allowStrafing = false;
		bodystate = 0;
		bCloseContact = false;
		fDistanceFromTarget = 0;
		actorTargetReq.Reset();
		curActorTargetPhase = eATP_None;
		curActorTargetFinishPos.Set(0,0,0);
		allowEntityClampingByAnimation = false;

		nTargetType = 0;
		fAuxDelay = 0.0f;
		nAuxPriority = 0;
		fMovementUrgency = AISPEED_WALK;
		fDesiredSpeed = 1.0f;
		fHitProbability = 1.0f;

		lean = 0.0f;
		peekOver = 0.0f;

		vSignals.clear();
		remainingPath.clear();
		szAuxSignalText.clear();
	}

	bool operator==(SOBJECTSTATE &other)
	{
		if (fire == other.fire)
			if (bodystate == other.bodystate)
				return true;
		return false;
	}

	void Serialize( TSerialize ser, class CObjectTracker& objectTracker );
};

// PROXY STATUS TYPES to check
#define	AIPROXYSTATUS_INVEHICLE	1

struct SAIBodyInfo
{
	Vec3		vEyePos;		
	Vec3		vEyeDir;		// Direction of head	(where I'm looking at).
	Vec3		vEyeDirAnim;	// Direction of head	from animation .
	Vec3		vBodyDir;		// Direction of entity "body" (my forward).
	Vec3		vMoveDir;		// Direction of entity movement.
	Vec3		vUpDir;			// Direction of entity (my up).
	Vec3		vFireDir;		// Firing direction.
	Vec3		vFirePos;		// Firing position.
	float		maxSpeed;		// Maximum speed i can move at.
	float		normalSpeed;	// The "normal" (walking) speed.
	float		minSpeed;		// Minimum speed i need to move anywhere (otherwise I'll consider myself stopped).
	EStance	stance;
	AABB		stanceSize;		// Approximate local bounds of the stance, relative to the entity position.
	AABB		colliderSize;	// Approximate local bounds of the stance, relative to the entity position.
	bool		isAiming;
	bool		isFiring;
	float		lean;			// The amount the character is leaning.
	float		peekOver;	// The amount the character is peaking-over.
	float		slopeAngle;

	IEntity *linkedVehicleEntity;
	IEntity *linkedDriverEntity;

	SAIBodyInfo() : vEyePos(ZERO), vEyeDir(ZERO), vEyeDirAnim(ZERO), vBodyDir(ZERO), vMoveDir(ZERO), vUpDir(ZERO), vFireDir(ZERO), vFirePos(ZERO),
		maxSpeed(0), normalSpeed(0), minSpeed(0), stance(STANCE_NULL),
		isAiming(false), isFiring(false), linkedVehicleEntity(0), linkedDriverEntity(0), lean(0.0f), slopeAngle(0.0f)
	{
		stanceSize.min=Vec3(ZERO);
		stanceSize.max=Vec3(ZERO);
		colliderSize.min=Vec3(ZERO);
		colliderSize.max=Vec3(ZERO);
	}
};

struct IWeapon;

struct SAIWeaponInfo
{
	SAIWeaponInfo()
		: lastShotTime((int64)0)
		, ammoCount(0)
		, clipSize(0)
		, outOfAmmo(false)
		, shotIsDone(false)
		, canMelee(false)
		,	hasLightAccessory(false)
		, hasLaserAccessory(false)
		, isReloading(false)
		, isFiring(false)
	{
	}

	CTimeValue lastShotTime;
	int ammoCount;
	int clipSize;

	bool canMelee : 1;
	bool outOfAmmo : 1;
	bool lowAmmo : 1;
	bool shotIsDone : 1;
	bool hasLightAccessory : 1;
	bool hasLaserAccessory : 1;
	bool isReloading : 1;
	bool isFiring : 1;
};


struct IPhysicalEntity;

enum EAIAGInput
{
	AIAG_SIGNAL,
	AIAG_ACTION,
	AIAG_COVERBODYDIR,
};

struct SAIBodyInfoQuery
{
	SAIBodyInfoQuery(const Vec3& pos, const Vec3& trg, EStance _stance, float _lean=0.0f, float _peekOver=0.0f, bool _defaultPose=false)
		: stance(_stance)
		, lean(_lean)
		, peekOver(_peekOver)
		, defaultPose(_defaultPose)
		, position(pos)
		, target(trg)
	{
	}

	SAIBodyInfoQuery(EStance _stance, float _lean=0.0f, float _peekOver=0.0f, bool _defaultPose=false)
		: stance(_stance)
		, lean(_lean)
		, peekOver(_peekOver)
		, defaultPose(_defaultPose)
		, position(ZERO)
		, target(ZERO)
	{
	}

	SAIBodyInfoQuery()
		: stance(STANCE_NULL)
		, lean(0.0f)
		, peekOver(0.0f)
		, defaultPose(true)
		, position(ZERO)
		, target(ZERO)
	{
	}

	EStance stance;
	float lean;
	float peekOver;
	bool defaultPose;
	Vec3 position;
	Vec3 target;
};


struct IAICommunicationHandler
{
	enum ECommunicationHandlerEvent
	{
		SoundStarted = 0,
		SoundFinished,
		SoundCancelled,
		SoundFailed,
		VoiceStarted,
		VoiceFinished,
		VoiceCancelled,
		VoiceFailed,
		ActionStarted,
		ActionCancelled,
		ActionFailed,
		SignalStarted,
		SignalFinished,
		SignalCancelled,
		SignalFailed,
	};

	enum EAnimationMethod
	{
		AnimationMethodSignal = 0,
		AnimationMethodAction,
	};

	struct IEventListener
	{
		virtual void OnCommunicationHandlerEvent(ECommunicationHandlerEvent type, void* idOrName, EntityId actorID) = 0;
	};

	virtual tSoundID PlaySound(const char* name, IEventListener* listener = 0) = 0;
	virtual void StopSound(tSoundID soundID) = 0;

	virtual tSoundID PlayVoice(const char* name, IEventListener* listener = 0) = 0;
	virtual void StopVoice(tSoundID soundID) = 0;

	virtual void PlayAnimation(const char* name, EAnimationMethod method, IEventListener* listener = 0) = 0;
	virtual void StopAnimation(const char* name, EAnimationMethod method) = 0;
};


UNIQUE_IFACE struct IAIActorProxy
{

	// (MATT) These methods were merged from IUnknownProxy {2009/01/26}
	//----------------------------------------------------------------
	virtual int Update(SOBJECTSTATE *, bool bFullUpdate) = 0;
	virtual bool CheckUpdateStatus() = 0;
	virtual void EnableUpdate(bool enable) = 0;
	virtual bool IsEnabled() const = 0;
	virtual int GetAlertnessState() const = 0;
	virtual bool IsCurrentBehaviorExclusive() const = 0;
	virtual bool SetCharacter( const char* character, const char* behaviour=NULL ) = 0;
	virtual const char* GetCharacter()  =0;
	virtual void QueryBodyInfo( SAIBodyInfo& bodyInfo ) = 0;
	virtual bool QueryBodyInfo( const SAIBodyInfoQuery& query, SAIBodyInfo& bodyInfo ) = 0;
	virtual	void QueryWeaponInfo( SAIWeaponInfo& weaponInfo ) = 0;
	virtual	EntityId GetLinkedDriverEntityId() = 0;
	virtual	bool IsDriver() = 0;
	virtual	EntityId GetLinkedVehicleEntityId() = 0;
	virtual void Release()=0;
	virtual void Reset(EObjectResetType type) = 0;
	virtual void Serialize( TSerialize ser ) = 0;

	virtual IAICommunicationHandler* GetCommunicationHandler() = 0;

	virtual IPhysicalEntity* GetPhysics(bool wantCharacterPhysics=false) = 0;
	virtual void DebugDraw(int iParam = 0) = 0;
	virtual const char* GetCurrentReadibilityName() const = 0;

	// This will internally keep a counter to allow stacking of such commands
	virtual void SetForcedExecute(bool forced) = 0;
	virtual bool IsForcedExecute() const = 0;

	// Description:
	//	 Gets the corners of the tightest projected bounding rectangle in 2D world space coordinates.
	virtual void GetWorldBoundingRect(Vec3& FL, Vec3& FR, Vec3& BL, Vec3& BR, float extra=0) const = 0;
	virtual bool SetAGInput(EAIAGInput input, const char* value) = 0;
	virtual bool ResetAGInput(EAIAGInput input) = 0;
	virtual bool IsSignalAnimationPlayed( const char* value ) = 0;
	virtual bool IsActionAnimationStarted( const char* value ) = 0;
	virtual bool IsAnimationBlockingMovement() const = 0;
	virtual EActorTargetPhase GetActorTargetPhase() const = 0;
	virtual void PlayAnimationAction( const struct IAIAction* pAction, int goalPipeId ) = 0;
	virtual void AnimationActionDone( bool succeeded ) = 0;

	// Description:
	//	 Returns the number of bullets shot since the last call to the method.
	virtual int GetAndResetShotBulletCount() = 0;

	virtual void	EnableWeaponListener(bool needsSignal) = 0;
	virtual void UpdateMind(SOBJECTSTATE *state) = 0;

	virtual bool IsDead() const { return false; }
	virtual int GetActorHealth() = 0;
	virtual int GetActorMaxHealth() = 0;
	virtual int GetActorArmor() = 0;
	virtual int GetActorMaxArmor() = 0;
	virtual bool GetActorIsFallen() const = 0;

	virtual IWeapon *GetCurrentWeapon() const = 0;
	virtual IWeapon *GetSecWeapon( ERequestedGrenadeType prefGrenadeType=eRGT_ANY ) const = 0;
	virtual AIWeaponDescriptor GetSecWeaponDescriptor(ERequestedGrenadeType prefGrenadeType=eRGT_ANY) const = 0;
	virtual IEntity* GetGrabbedEntity() const = 0;

	virtual const char* GetVoiceLibraryName() const = 0;
	virtual const char* GetCommunicationConfigName() const = 0;
	virtual const char* GetBehaviorSelectionTreeName() const = 0;

	// Description:
	//	 Needed for debug drawing.
	virtual bool IsUpdateAlways() const = 0;
	virtual bool IfShouldUpdate() = 0;


	// (MATT) These methods were merged from IPuppetProxy {2009/01/26}
	//----------------------------------------------------------------
	virtual bool PredictProjectileHit(const Vec3& throwDir, float vel, Vec3& posOut, float& speedOut,
		ERequestedGrenadeType prefGrenadeType = eRGT_ANY, Vec3* pTrajectory = 0, unsigned int* trajectorySizeInOut = 0) = 0;

	// Description:
	//	 Plays readability sound.
	virtual int PlayReadabilitySound(const char* szReadability, bool stopPreviousSound, float responseDelay = 0.0f) = 0;
	// Description:
	//	 Toggles readability testing on/off (called from a console command).
	virtual void TestReadabilityPack(bool start, const char* szReadability) = 0;
	// Description:
	//	 Returns the amount of time to block the readabilities of same name in a group.
	virtual void GetReadabilityBlockingParams(const char* text, float& time, int& id) = 0;
	// Description:
	//	 Informs CryAction Readability Manager about external system (like CryAISystem for example) playing
	//	 another readability.
	virtual void SetReadabilitySound(tSoundID nSoundId, bool bStopPrevious) = 0;

	virtual bool QueryCurrentAnimationSpeedRange(float& smin, float& smax) = 0;

	// Description:
	//	 Gets name of the running behavior.
	virtual const char* GetCurrentBehaviorName() const { return NULL; }

	// Description:
	//	 Gets name of the previous behavior.
	virtual const char* GetPreviousBehaviorName() const { return NULL; }

	virtual void UpdateMeAlways(bool doUpdateMeAlways) = 0;

	virtual void SetBehaviour(const char* szBehavior, const IAISignalExtraData* pData = 0) = 0;
};

enum EWaypointNodeType {WNT_UNSET, WNT_WAYPOINT, WNT_HIDE,  WNT_ENTRYEXIT, WNT_EXITONLY, WNT_HIDESECONDARY};
enum EWaypointLinkType {WLT_AUTO_PASS = 320, WLT_AUTO_IMPASS = -320, WLT_EDITOR_PASS = 321, WLT_EDITOR_IMPASS = -321, WLT_UNKNOWN_TYPE = 0};

// Summary
//		AI actions order.
#define AIORD_ATTACK 			"ORD_ATTACK"			// 
#define AIORD_SEARCH 			"ORD_SEARCH"			// 
#define AIORD_HIDE 				"ORD_HIDE"				// 
#define AIORD_HOLD 				"ORD_HOLD"				// 
#define AIORD_GOTO 				"ORD_GOTO"				// 
#define AIORD_USE 				"ORD_USE"				// 
#define AIORD_FOLLOW 			"ORD_FOLLOW"			// 
#define AIORD_IDLE	 			"ORD_IDLE"				// 
#define AIORD_REPORTDONE		"ORD_DONE"				// 
#define AIORD_REPORTFAIL		"ORD_FAIL"				// 
#define AIORD_LEAVE_VEHICLE		"ORD_LEAVE_VEHICLE"
#define USE_ORDER_ENABLED		"USE_ORDER_ENABLED"
#define USE_ORDER_DISABLED		"USE_ORDER_DISABLED"




#endif //_IAGENT_H_
