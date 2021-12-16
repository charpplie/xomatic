/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   IAISystem.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include DEVIRTUALIZE_HEADER_FIX(IAISystem.h)

#ifndef _IAISYSTEM_H_
#define _IAISYSTEM_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SerializeFwd.h"
#include "Cry_Math.h"
#include "TimeValue.h"
#include "AIFormationDescriptor.h"
#include "IAIRecorder.h"

struct AgentPathfindingProperties;
struct INavigation;
struct IAIPathFinder;
struct IPhysicalEntity;
struct IPhysicalWorld;
class ICrySizer;
struct IRenderer;
struct ISystem;
struct IGame;
struct IEntity;
struct IClient;
struct IAIDebugRenderer;
struct IAgentProxy;
struct IAgent;
struct IAIObject;
struct IAIActor;
struct IAIGroup;
struct IGoalPipe;
struct IVisArea;
struct ObstacleData;
struct IAISignalExtraData;
struct IFireCommandDesc;
struct IFireCommandHandler;
struct IEmotionalSystem;
struct ICoordinationManager;
struct ICommunicationManager;
struct ICoverSystem;
struct ISelectionTreeManager;
struct IFunctionHandler;
struct IEntityClass;
struct SmartObjectCondition;
class CSmartObjectClass;
struct IAIAction;
struct IAIRecorder;
class ICentralInterestManager;
struct ITacticalPointSystem;
struct ITargetTrackManager;
struct IBSSProfileManager;
struct IStatObj;
struct Sphere;
class CSmartObjectClass;
class CSmartObjectAction;
struct IAIActionManager;
struct IAIAction;
struct ISmartObjectManager;
struct HidespotQueryContext;
class IVisionMap;
class IAISystemListener;
struct IAIObjectManager;


namespace NewBehaviourTree
{
	class TreeProfileManager;
};


// Define the oldest AI Area file version that can still be read
#define BAI_AREA_FILE_VERSION_READ 19
// Define the AI Area file version that will be written
#define BAI_AREA_FILE_VERSION_WRITE 21

#define	AIFAF_VISIBLE_FROM_REQUESTER	0x0001 	// Requires whoever is requesting the anchor to also have a line of sight to it.
//#define AIFAF_VISIBLE_TARGET					0x0002 	// Requires that there is a line of sight between target and anchor.
#define AIFAF_INCLUDE_DEVALUED 				0x0004 	// Don't care if the object is devalued.
#define AIFAF_INCLUDE_DISABLED				0x0008 	// Don't care if the object is disabled.
#define AIFAF_HAS_FORMATION						0x0010 	// Want only formations owners.
#define AIFAF_LEFT_FROM_REFPOINT			0x0020 	// Requires that the anchor is left from the target.
#define AIFAF_RIGHT_FROM_REFPOINT			0x0040 	// Requires that the anchor is right from the target.
#define AIFAF_INFRONT_OF_REQUESTER		0x0080 	// Requires that the anchor is within a 30-degree cone in front of requester.
#define AIFAF_SAME_GROUP_ID						0x0100 	// Want same group id.
#define AIFAF_DONT_DEVALUE 						0x0200 	// Do not devalue the object after having found it.
#define AIFAF_USE_REFPOINT_POS				0x0400	// Use ref point position instead of AI object pos.


#define SMART_OBJECTS_XML "Libs/SmartObjects.xml"
#define AI_ACTIONS_PATH "Libs/ActionGraphs"

#define TEMPORARY_SO_ENTITY_PTR ((IEntity*)~NULL)

static const unsigned AI_MAX_FILTERS = 4;
// The type is sometimes converted to a mask and stored in an dword (32bits), no more than 32 subtypes.
static const unsigned AI_MAX_STIMULI = 32;
// The subtype is sometimes converted to a mask and stored in a byte (8bits), no more than 8 subtypes.
static const unsigned AI_MAX_SUBTYPES = 8;


typedef std::list<IAIObject*> TAIObjectList;
typedef std::vector<int> TSubActionList;


enum EnumAreaType
{
	AREATYPE_PATH,
	AREATYPE_FORBIDDEN,
	AREATYPE_FORBIDDENBOUNDARY,
	AREATYPE_NAVIGATIONMODIFIER,
	AREATYPE_OCCLUSION_PLANE,
	AREATYPE_EXTRALINKCOST,
	AREATYPE_GENERIC,
	AREATYPE_PERCEPTION_MODIFIER,
};

/// The first word refers to how the nodes are initially connected
/// The second word refers to how the node connections are subsequently modified - partial means that links only get disabled.
enum EWaypointConnections 
{
	WPCON_DESIGNER_NONE, 
	WPCON_DESIGNER_PARTIAL, 
	WPCON_AUTO_NONE, 
	WPCON_AUTO_PARTIAL, 
	WPCON_MAXVALUE = WPCON_AUTO_PARTIAL
};

// ENavModifierType: Values are important and some types have been removed
enum ENavModifierType
{
	NMT_INVALID = -1,
	NMT_WAYPOINTHUMAN = 0,
	NMT_VOLUME = 1,
	NMT_FLIGHT = 2,
	NMT_WATER = 3,
	NMT_WAYPOINT_3DSURFACE = 4,
	NMT_EXTRA_NAV_COST = 5,
	NMT_FREE_2D = 7,
	NMT_TRIANGULATION = 8,
	NMT_LAYERED_NAV_MESH
};

enum EAILightLevel
{
	AILL_NONE,		// No affect.
	AILL_LIGHT,		// Light.
	AILL_MEDIUM,	// Medium.
	AILL_DARK,		// Dark.
	AILL_LAST,		// This always has to be the last one.
};

// AI sound event types. They are roughly in the order of priority.
// The priority is not enforced but may be used as a hint when handling the sound event.
enum EAISoundStimType
{
	AISOUND_GENERIC,				// Generic sound event
	AISOUND_COLLISION,				// Sound event from collisions.
	AISOUND_COLLISION_LOUD,			// Sound event from collisions, loud.
	AISOUND_MOVEMENT,				// Movement related sound event
	AISOUND_MOVEMENT_LOUD,			// Movement related sound event, very loud, like walking in water or a vehicle sound.
	AISOUND_WEAPON,					// Weapon firing related sound event
	AISOUND_EXPLOSION,				// Explosion related sound event
	AISOUND_LAST,
};

// Different grenade events reported into the AIsystem
enum EAIGrenadeStimType
{
	AIGRENADE_THROWN,
	AIGRENADE_COLLISION,
	AIGRENADE_FLASH_BANG,
	AIGRENADE_SMOKE,
	AIGRENADE_LAST,
};

// Different light events reported into the AIsystem
enum EAILightEventType
{
	AILE_GENERIC,
	AILE_MUZZLE_FLASH,
	AILE_FLASH_LIGHT,
	AILE_LASER,
	AILE_LAST,
};


enum EAIStimulusType
{
	AISTIM_SOUND,
	AISTIM_COLLISION,
	AISTIM_EXPLOSION,
	AISTIM_BULLET_WHIZZ,
	AISTIM_BULLET_HIT,
	AISTIM_GRENADE,
	AISTIM_LAST,
};

enum EAIStimProcessFlags
{
	AISTIMPROC_EMPTY = 0,
	AISTIMPROC_FILTER_LINK_WITH_PREVIOUS = 0x01,	// Uses the stimulus filtering from prev stim.
	AISTIMPROC_NO_UPDATE_MEMORY = 0x02,				// This won't update the mem target position of the source
};

enum EActionType
{
	eAT_None = 0,
	eAT_Action,
	eAT_PriorityAction,
	eAT_Approach,
	eAT_PriorityApproach,
	eAT_ApproachAction,
	eAT_PriorityApproachAction,
	eAT_AISignal,
	eAT_AnimationSignal,
	eAT_AnimationAction,
	eAT_PriorityAnimationSignal,
	eAT_PriorityAnimationAction,
};

enum ESORuleType
{
	eSORuleType_TimeBased,
	eSORuleType_Navigation,
};

enum SAICollisionObjClassification
{
	AICOL_SMALL,
	AICOL_MEDIUM,
	AICOL_LARGE,
};

// Description:
//	Stimulus Filter describes how the stimulus filter works.
//	When a new stimulus is processed and it is within the radius of existing stimulus
//	the new stimulus is discarded. If the merge option is specified, the new stimulus
//	Will be merged into the existing stimulus iff the stimulus time is less than
//	the processDelay of the stimulus type.
//	The type specifies which one type of existing stimulus that will be considered as filter.
//	The subType specifies a mask of all possible subtypes that will be considered as filter.
//	Before the radius is checked, the radius of the existing stimulus is scaled by the scale.
enum EAIStimulusFilterMerge
{
	AISTIMFILTER_DISCARD,						// Discard new stimulus when inside existing stimulus.
	AISTIMFILTER_MERGE_AND_DISCARD,	// Merge new stimulus when inside existing stimulus iff the lifetime of
	// existing stimulus is less than processDelay, else discard.
};


struct SNavigationShapeParams
{
	SNavigationShapeParams(
		const char *szPathName = 0,
		EnumAreaType areaType = AREATYPE_PATH,
		bool pathIsRoad = true,
		bool closed = false,
		const Vec3* points = 0,
		unsigned nPoints = 0,
		float fHeight = 0,
		int nNavType = 0,
		int nAuxType = 0,
		EAILightLevel lightLevel = AILL_NONE,
		float fNodeAutoConnectDistance = 0,
		EWaypointConnections waypointConnections = WPCON_DESIGNER_NONE,
		bool bVehiclesInHumanNav =false,
		bool bCalculate3DNav = true,
		bool bCalculateLNM = true,
		bool bLNMExclusionVolume = true,
		float f3DNavVolumeRadius = 10.0f,
		float extraLinkCostFactor = 0.0f,
		float fReductionPerMetre = 0.0f,
		float fReductionMax = 1.0f,
		const char* szPFPropertiesList = 0)
		: szPathName(szPathName), areaType(areaType), pathIsRoad(pathIsRoad), closed(closed), points(points), nPoints(nPoints), fHeight(fHeight),
		nNavType(nNavType), nAuxType(nAuxType), fNodeAutoConnectDistance(fNodeAutoConnectDistance),
		waypointConnections(waypointConnections), bVehiclesInHumanNav(bVehiclesInHumanNav), bCalculate3DNav(bCalculate3DNav), bCalculateLNM(bCalculateLNM),
		bLNMExclusionVolume(bLNMExclusionVolume), f3DNavVolumeRadius(f3DNavVolumeRadius), 
		extraLinkCostFactor(extraLinkCostFactor), fReductionPerMetre(fReductionPerMetre), fReductionMax(fReductionMax), lightLevel(lightLevel),
		szPFPropertiesList(szPFPropertiesList)
	{}

	const char *szPathName;
	EnumAreaType areaType;
	bool pathIsRoad;
	bool closed;
	const Vec3* points;
	unsigned nPoints;
	float fHeight;
	int	nNavType;
	int nAuxType;
	float fNodeAutoConnectDistance;
	EWaypointConnections waypointConnections;
	bool bVehiclesInHumanNav;
	bool bCalculate3DNav;
	bool bCalculateLNM;
	bool bLNMExclusionVolume;
	EAILightLevel lightLevel;
	float f3DNavVolumeRadius;
	/// Cost of links going through this shape gets multiplied by (1 + extraCostScale) - should be >= 0
	/// for A* heuristic to be valid.
	float extraLinkCostFactor;
	// size of the triangles to create when it's a nav modifier that adds extra triangles for
	// Parameters for PerceptionModifier
	float fReductionPerMetre;
	float fReductionMax;
	const char *szPFPropertiesList;
};

// AI Stimulus record.
// Stimuli are used to tell the perception manager about important events happening in the gameworld
// such as sounds or grenades.
// When the stimulus is processed, it can be merged with previous stimuli in order to prevent
// too many stimuli to cause too many reactions.
struct SAIStimulus
{
	SAIStimulus() : sourceId(0), targetId(0), pos(0,0,0), dir(0,0,0), radius(0), type(0), subType(0), flags(0)
	{
	}

	SAIStimulus(EAIStimulusType type, unsigned char subType, EntityId sourceId, EntityId targetId,
		const Vec3& pos, const Vec3& dir, float radius, unsigned char flags = 0) :
	sourceId(sourceId), targetId(targetId), pos(pos), dir(dir),
		radius(radius), type(type), subType(subType), flags(flags)
	{
	}

	EntityId sourceId;				// The source of the stimulus
	EntityId targetId;				// Optional target of the stimulus
	Vec3 pos;									// Location of the stimulus
	Vec3 dir;									// Optional direction of the stimulus -  for now, should be pre-normalised
	float radius;							// Radius of the stimulus
	unsigned char type;				// Stimulation type
	unsigned char subType;		// Stimulation sub-type
	unsigned char flags;			// Processing flags
};



struct SAIStimulusFilter
{
	inline void Set(unsigned char t, unsigned char st, EAIStimulusFilterMerge m, float s)
	{
		scale = s;
		merge = m;
		type = t;
		subType = st;
	}

	float scale;							// How much the existing stimulus radius is scaled before checking.
	unsigned char merge;			// Filter merge type, see EAIStimulusFilterMerge.
	unsigned char type;				// Stimulus type to match against.
	unsigned char subType;		// Stimulus subtype _mask_ to match against, or 0 if not applied.
};

// Stimulus descriptor.
// Defines a stimulus type and how the incoming stimuli of the specified type should be handled.
//
// Common stimuli defined in EAIStimulusType are already registered in the perception manager.
// For game specific stimuli, the first stim type should be LAST_AISTIM.
//
// Example:
//		memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
//		desc.SetName("Collision");
//		desc.processDelay = 0.15f;
//		desc.duration[AICOL_SMALL] = 7.0f;
//		desc.duration[AICOL_MEDIUM] = 7.0f;
//		desc.duration[AICOL_LARGE] = 7.0f;
//		desc.filterTypes = (1<<AISTIM_COLLISION) | (1<<AISTIM_EXPLOSION);
//		desc.nFilters = 2;
//		desc.filters[0].Set(AISTIM_COLLISION, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.9f); // Merge nearby collisions
//		desc.filters[1].Set(AISTIM_EXPLOSION, 0, AISTIMFILTER_DISCARD, 2.5f); // Discard collision near explosions
//		pPerceptionMgr->RegisterStimulusDesc(AISTIM_COLLISION, desc);
//
struct SAIStimulusTypeDesc
{
	inline void SetName(const char* n)
	{
		assert(strlen(n) < 32);
		strncpy(name, n, 32);
	}

	float processDelay;								// Delay before the stimulus is actually sent.
	unsigned int filterTypes;					// Mask of all types of filters contained in the filter list.
	float duration[AI_MAX_SUBTYPES];	// Duration of the stimulus, accessed by the subType of SAIStimulus.
	SAIStimulusFilter filters[AI_MAX_FILTERS];	// The filter list.
	char name[32];										// Name of the stimulus.
	unsigned char nFilters;						// Number of filters in the filter list.
};






struct AIBalanceStats
{
	int nAllowedDeaths;
	int nTotalPlayerDeaths;
	int nEnemiesKilled;
	int nShotsFires;
	int nShotsHit;
	int nCheckpointsHit;
	int nVehiclesDestroyed;
	int nTotalEnemiesInLevel;
	int nSilentKills;

	float fAVGEnemyLifetime;
	float fAVGPlayerLifetime;
	float fTotalTimeSeconds;

	bool bFinal;

	AIBalanceStats()
	{
		bFinal = false;
		nAllowedDeaths=0;
		nTotalPlayerDeaths=0;
		nEnemiesKilled=0;
		nShotsFires=0;
		nShotsHit=0;
		nCheckpointsHit=0;
		nVehiclesDestroyed=0;
		nTotalEnemiesInLevel=0;
		nSilentKills=0;

		fAVGPlayerLifetime = 0;
		fAVGEnemyLifetime = 0;
		fTotalTimeSeconds =0;
	}

};





struct SmartObjectCondition
{
	string	sUserClass;
	string	sUserState;
	string	sUserHelper;

	string	sObjectClass;
	string	sObjectState;
	string	sObjectHelper;

	float	fDistanceFrom;
	float	fDistanceTo;
	float	fOrientationLimit;
	bool bHorizLimitOnly;
	float	fOrientationToTargetLimit;

	float	fMinDelay;
	float	fMaxDelay;
	float	fMemory;

	float	fProximityFactor;
	float	fOrientationFactor;
	float	fVisibilityFactor;
	float	fRandomnessFactor;

	float	fLookAtOnPerc;
	string	sUserPreActionState;
	string	sObjectPreActionState;
	EActionType	eActionType;
	string	sAction;
	bool	bEarlyPathRegeneration;
	string	sUserPostActionState;
	string	sObjectPostActionState;

	int		iMaxAlertness;
	bool	bEnabled;
	string	sName;
	string	sFolder;
	string	sDescription;
	int		iOrder;

	int		iRuleType; // 0 - normal rule; 1 - navigational rule;
	string	sEvent;
	string	sChainedUserEvent;
	string	sChainedObjectEvent;
	string	sEntranceHelper;
	string	sExitHelper;

	int		iTemplateId;

	// exact positioning related
	float	fApproachSpeed;
	int		iApproachStance;
	string	sAnimationHelper;
	string	sApproachHelper;
	float	fStartWidth;
	float	fDirectionTolerance;
	float	fStartArcAngle;

	bool operator == ( const SmartObjectCondition& other ) const
	{
		return
			iTemplateId == other.iTemplateId &&

			iOrder == other.iOrder &&
			sUserClass == other.sUserClass &&
			sUserState == other.sUserState &&
			sUserHelper == other.sUserHelper &&

			sObjectClass == other.sObjectClass &&
			sObjectState == other.sObjectState &&
			sObjectHelper == other.sObjectHelper &&

			iRuleType == other.iRuleType &&
			sEvent == other.sEvent &&
			sEntranceHelper == other.sEntranceHelper &&
			sExitHelper == other.sExitHelper &&

			fDistanceFrom == other.fDistanceFrom &&
			fDistanceTo == other.fDistanceTo &&
			fOrientationLimit == other.fOrientationLimit &&
			bHorizLimitOnly == other.bHorizLimitOnly &&
			fOrientationToTargetLimit == other.fOrientationToTargetLimit &&

			fMinDelay == other.fMinDelay &&
			fMaxDelay == other.fMaxDelay &&
			fMemory == other.fMemory &&

			fProximityFactor == other.fProximityFactor &&
			fOrientationFactor == other.fOrientationFactor &&
			fVisibilityFactor == other.fVisibilityFactor &&
			fRandomnessFactor == other.fRandomnessFactor &&

			fLookAtOnPerc == other.fLookAtOnPerc &&
			sUserPreActionState == other.sUserPreActionState &&
			sObjectPreActionState == other.sObjectPreActionState &&
			eActionType == other.eActionType &&
			sAction == other.sAction &&
			bEarlyPathRegeneration == other.bEarlyPathRegeneration &&
			sUserPostActionState == other.sUserPostActionState &&
			sObjectPostActionState == other.sObjectPostActionState &&

			iMaxAlertness == other.iMaxAlertness &&
			bEnabled == other.bEnabled &&
			sName == other.sName &&
			sFolder == other.sFolder &&
			sDescription == other.sDescription &&

			fApproachSpeed == other.fApproachSpeed &&
			iApproachStance == other.iApproachStance &&
			sAnimationHelper == other.sAnimationHelper &&
			sApproachHelper == other.sApproachHelper &&
			fStartWidth == other.fStartWidth &&
			fDirectionTolerance == other.fDirectionTolerance &&
			fStartArcAngle == other.fStartArcAngle;
	}

	bool operator < ( const SmartObjectCondition& other ) const
	{
		return iOrder < other.iOrder;
	}
};
typedef	std::list< SmartObjectCondition > SmartObjectConditions;




struct CSmartObjectRuleData
{
	string	sUserClass;
	string	sUserState;
	string	sUserHelper;

	string	sObjectClass;
	string	sObjectState;
	string	sObjectHelper;

	float	fDistanceFrom;
	float	fDistanceTo;
	float	fOrientationLimit;
	bool	bHorizLimitOnly;
	float	fOrientationToTargetLimit;

	float	fMinDelay;
	float	fMaxDelay;
	float	fMemory;

	float	fProximityFactor;
	float	fOrientationFactor;
	float	fVisibilityFactor;
	float	fRandomnessFactor;

	float	fLookAtOnPerc;
	string	sUserPreActionState;
	string	sObjectPreActionState;
	EActionType eActionType;
	string	sAction;
	bool	bEarlyPathRegeneration;
	string	sUserPostActionState;
	string	sObjectPostActionState;

	int		iMaxAlertness;

	ESORuleType	eSORuleType;
	string	sEvent;
	string	sChainedUserEvent;
	string	sChainedObjectEvent;

	string	sEntranceHelper;
	string	sExitHelper;

	// exact positioning related
	float	fApproachSpeed;
	int		iApproachStance;
	string	sAnimationHelper;
	string	sApproachHelper;
	float	fStartWidth;
	float	fDirectionTolerance;
	float	fStartArcAngle;

	bool operator == ( const CSmartObjectRuleData& other ) const
	{
		return
			sUserClass == other.sUserClass &&
			sUserState == other.sUserState &&
			sUserHelper == other.sUserHelper &&

			sObjectClass == other.sObjectClass &&
			sObjectState == other.sObjectState &&
			sObjectHelper == other.sObjectHelper &&

			eSORuleType == other.eSORuleType &&
			sEvent == other.sEvent &&
			sEntranceHelper == other.sEntranceHelper &&
			sExitHelper == other.sExitHelper &&

			fDistanceFrom == other.fDistanceFrom &&
			fDistanceTo == other.fDistanceTo &&
			fOrientationLimit == other.fOrientationLimit &&
			bHorizLimitOnly == other.bHorizLimitOnly &&
			fOrientationToTargetLimit == other.fOrientationToTargetLimit &&

			fMinDelay == other.fMinDelay &&
			fMaxDelay == other.fMaxDelay &&
			fMemory == other.fMemory &&

			fProximityFactor == other.fProximityFactor &&
			fOrientationFactor == other.fOrientationFactor &&
			fVisibilityFactor == other.fVisibilityFactor &&
			fRandomnessFactor == other.fRandomnessFactor &&

			fLookAtOnPerc == other.fLookAtOnPerc &&
			sUserPreActionState == other.sUserPreActionState &&
			sObjectPreActionState == other.sObjectPreActionState &&
			eActionType == other.eActionType &&
			sAction == other.sAction &&
			bEarlyPathRegeneration == other.bEarlyPathRegeneration &&
			sUserPostActionState == other.sUserPostActionState &&
			sObjectPostActionState == other.sObjectPostActionState &&

			iMaxAlertness == other.iMaxAlertness &&

			fApproachSpeed == other.fApproachSpeed &&
			iApproachStance == other.iApproachStance &&
			sAnimationHelper == other.sAnimationHelper &&
			sApproachHelper == other.sApproachHelper &&
			fStartWidth == other.fStartWidth &&
			fDirectionTolerance == other.fDirectionTolerance &&
			fStartArcAngle == other.fStartArcAngle;
	}
};
typedef	std::list< CSmartObjectRuleData >	VectorSORuleData;

struct SmartObjectHelper
{
	SmartObjectHelper() : templateHelperIndex(-1) {}
	QuatT qt;
	string name;
	string description;
	int templateHelperIndex;
};

// MapSOHelpers contains all smart object helpers sorted by name of the smart object class to which they belong
typedef std::multimap<string, SmartObjectHelper > MapSOHelpers;

// AI object iterator interface, see IAISystem::GetFirst.
struct IAIObjectIter
{
	// Advance to next object.
	virtual void Next() = 0;
	// Returns the current object.
	virtual IAIObject* GetObject() = 0;
	// Delete the iterator.
	virtual void Release() = 0;
};

// Helper class for AI object iterator.
class AutoAIObjectIter
{
public:
	AutoAIObjectIter() : m_pIter(NULL) {}
	AutoAIObjectIter(IAIObjectIter* it) : m_pIter(NULL) { Assign(it); }
	~AutoAIObjectIter() { Assign(NULL); }
	IAIObjectIter* operator->() { return m_pIter; }
	IAIObjectIter* GetPtr() { return m_pIter; }
	void Assign(IAIObjectIter* it) { SAFE_RELEASE(m_pIter); m_pIter = it; }
private:
	IAIObjectIter*	m_pIter;
};




// AI event listener
struct IAIEventListener
{
	virtual void OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender) = 0;
};

struct IAIAlertnessPredicate
{
	virtual bool ConsiderAIObject(IAIObject* pAIObject) const = 0;
};

struct SAIDetectionLevels
{
	SAIDetectionLevels(float puppetExposure, float puppetThreat, float vehicleExposure, float vehicleThreat) :
puppetExposure(puppetExposure),
puppetThreat(puppetThreat),
vehicleExposure(vehicleExposure),
vehicleThreat(vehicleThreat)
{
}

SAIDetectionLevels()
{
	Reset();
}

inline void Append(const SAIDetectionLevels& levels)
{
	puppetExposure = max(puppetExposure, levels.puppetExposure);
	puppetThreat = max(puppetThreat, levels.puppetThreat);
	vehicleExposure = max(vehicleExposure, levels.vehicleExposure);
	vehicleThreat = max(vehicleThreat, levels.vehicleThreat);
}

inline void Reset()
{
	puppetExposure = 0;
	puppetThreat = 0;
	vehicleExposure = 0;
	vehicleThreat = 0;
}

float puppetExposure;
float puppetThreat;
float vehicleExposure;
float vehicleThreat;
};

struct PathfindingHeuristicProperties;
typedef float (*CustomCostFunction)(const void*, const void*, const PathfindingHeuristicProperties &);

// Description:
//		Interface to AI system. Defines functions to control the ai system.
UNIQUE_IFACE struct IAISystem
{
	/// Flags used by the GetGroupCount.
	enum EGroupFlags
	{
		GROUP_ALL =			0x01,		// Returns all agents in the group (default).
		GROUP_ENABLED =	0x02,		// Returns only the count of enabled agents (exclusive with all).
		GROUP_MAX =			0x04,		// Returns the maximum number of agents during the game (can be combined with all or enabled).
	};

	/// Flags used by the GetDangerSpots.
	enum EDangerSpots
	{
		DANGER_DEADBODY =	0x01,		// Include dead bodies.
		DANGER_EXPLOSIVE = 0x02,	// Include explosives - unexploded grenades.
		DANGER_EXPLOSION_SPOT = 0x04,	// Include recent explosions.
		DANGER_ALL = DANGER_DEADBODY | DANGER_EXPLOSIVE,
	};

	/// Indication of (a) what a graph node represents and (b) what kind of graph node an AI entity
	/// can navigate. In the latter case it can be used as a bit mask.
	enum ENavigationType {
		NAV_UNSET              = 1 << 0,
		NAV_TRIANGULAR         = 1 << 1,
		NAV_WAYPOINT_HUMAN     = 1 << 2,
		NAV_WAYPOINT_3DSURFACE = 1 << 3,
		NAV_FLIGHT             = 1 << 4,
		NAV_VOLUME             = 1 << 5,
		NAV_ROAD               = 1 << 6,
		NAV_SMARTOBJECT        = 1 << 7,
		NAV_FREE_2D            = 1 << 8,
		NAV_LAYERED_NAV_MESH   = 1 << 9,
		NAV_CUSTOM_NAVIGATION	 = 1 << 10,
		NAV_MAX_VALUE          = NAV_CUSTOM_NAVIGATION
	};
	enum {NAV_TYPE_COUNT = 11};

	/// two masks that summarise the basic abilities
	enum {
		NAVMASK_SURFACE = NAV_TRIANGULAR | NAV_WAYPOINT_HUMAN | NAV_ROAD | NAV_SMARTOBJECT | NAV_LAYERED_NAV_MESH,
		NAVMASK_AIR = NAV_FLIGHT | NAV_VOLUME | NAV_SMARTOBJECT,
		NAVMASK_ALL = NAV_TRIANGULAR | NAV_WAYPOINT_HUMAN | NAV_WAYPOINT_3DSURFACE | NAV_ROAD | NAV_FLIGHT | NAV_VOLUME | NAV_SMARTOBJECT | NAV_FREE_2D | NAV_CUSTOM_NAVIGATION | NAV_LAYERED_NAV_MESH,
	};

		/// Filter flags for IAISystem::GetFirstAIObject
	enum EGetFirstFilter
	{
		OBJFILTER_TYPE,			// Include only objects of specified type (type 0 means all objects).
		OBJFILTER_GROUP,		// Include only objects of specified group id.
		OBJFILTER_SPECIES,	// Include only objects of specified species.
		OBJFILTER_DUMMYOBJECTS,	// Include only dummy objects
	};

	enum EResetReason
	{
		RESET_INTERNAL,			// Called by the AI system itself
		RESET_ENTER_GAME,
		RESET_EXIT_GAME,
		RESET_INTERNAL_LOAD,	// Called by the AI system itself
	};

	/// Bit mask using ENavigationType
	// NOTE Aug 14, 2009: <pvl> NavCapMask is no longer a primitive type.  This
	// thin wrapper around primitive unsigned is necessary to transparently support
	// the LNM.  The LNM breaks the design assumptions of the current system by
	// producing meshes tailored to various agent type capabilities.  While the
	// question "can triangulation be used" was well-formed it doesn't make sense
	// in the context of the LNM where there's multiple meshes, out of which some
	// might be useable and some not.
	//
	// To narrow the choice down to a single mesh, still making the rest of
	// the system work as it did before, 's_lnmBits' are set aside to be used
	// to discriminate among LNM meshes built for different agent types.
	// Operators are overloaded so that non-LNM parts of the system never see
	// the LNM part.  LNM-aware parts can request the LNM bits explicitly.
	//
	// Note also that the LNM bits aren't used as a bitmask.
	class NavCapMask {
		static const unsigned s_maskWidth = 8 * 4/*sizeof (unsigned)*/;
		static const unsigned s_lnmBits = 8;
		// ATTN Aug 17, 2009: <pvl> if you change the following you also need to
		// change 'pathfindProperties.lua'
		static const unsigned s_lnmShift = s_maskWidth - s_lnmBits;
		static const unsigned s_lnmMask = ((1 << s_lnmBits) - 1) << s_lnmShift;
		unsigned m_navCaps;
	public:
		NavCapMask () : m_navCaps (0) { }
		NavCapMask (unsigned caps) : m_navCaps (caps) { }

		// NOTE Oct 21, 2009: <pvl> only legacy (pre-LNM) code can sometimes
		// use NavCapMask as unsigned.  That code doesn't expect to see LNM bits
		// so strip them off.
		operator unsigned () const { return m_navCaps & ~s_lnmMask; }

		NavCapMask & operator &= (unsigned rhs) { m_navCaps &= rhs; return *this; }
		NavCapMask & operator |= (unsigned rhs) { m_navCaps |= rhs; return *this; }

		unsigned GetLnmCaps () const { return (m_navCaps & s_lnmMask) >> s_lnmShift; }
		void SetLnmCaps (unsigned lnmData) { m_navCaps |= lnmData << s_lnmShift; }

		// NOTE Oct 21, 2009: <pvl> unlike operator unsigned() this one returns
		// the unedited mask including the LNM part
		unsigned GetFullMask () const { return m_navCaps; }

		void Serialize (TSerialize ser);
	};
	typedef NavCapMask tNavCapMask;

	














	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Basic////////////////////////////////////////////////////////////////////////////////////////////////////////

	IAISystem() {}
	virtual ~IAISystem() {}

	virtual bool Init() = 0;

	virtual void Reload() {;}
	virtual void Reset(EResetReason reason) = 0;
	virtual void Release() = 0;

	//If disabled most things early out
	virtual void Enable(bool enable=true) = 0;

	//Every frame (multiple time steps per frame possible?)		//TODO find out
	//	currentTime - AI time since game start in seconds (GetCurrentTime)
	//	frameTime - since last update (GetFrameTime)
	virtual void Update(CTimeValue currentTime, float frameTime) = 0;

	virtual bool RegisterListener(IAISystemListener* pListener) = 0;
	virtual bool UnregisterListener(IAISystemListener* pListener) = 0;

	// Registers AI event listener. Only events overlapping the sphere will be sent.
	// Register can be called again to update the listener position, radius and flags.
	// If pointer to the listener is specified it will be used instead of the pointer to entity.
	virtual void RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags) = 0;
	virtual void UnregisterAIEventListener(IAIEventListener* pListener) = 0;

	virtual void Event( int eventT, const char *) = 0;
	virtual IAISignalExtraData* CreateSignalExtraData() const = 0;
	virtual void FreeSignalExtraData( IAISignalExtraData* pData ) const = 0;
	virtual void SendSignal(unsigned char cFilter, int nSignalId,const char *szText,  IAIObject *pSenderObject, IAISignalExtraData* pData=NULL) = 0;
	virtual void SendAnonymousSignal(int nSignalId,const char *szText, const Vec3 &pos, float fRadius, IAIObject *pSenderObject,IAISignalExtraData* pData=NULL) = 0;

	//Basic////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////







	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Time/Updates/////////////////////////////////////////////////////////////////////////////////////////////////

	//Over-ride auto-disable for distant AIs
	virtual bool GetUpdateAllAlways() const = 0;

	// Returns the current time (seconds since game began) that AI should be working with - 
	// This may be different from the system so that we can support multiple updates per
	// game loop/update.
	virtual CTimeValue GetFrameStartTime() const = 0;

	// Time interval between this and the last update
	virtual float GetFrameDeltaTime() const = 0;

	// returns the basic AI system update interval
	virtual float GetUpdateInterval() const = 0;

	// profiling
	virtual int GetAITickCount() = 0;

	//Time/Updates/////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////





	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//FileIO///////////////////////////////////////////////////////////////////////////////////////////////////////

	// save/load
	virtual void Serialize( TSerialize ser ) = 0;

	//! Set a path for the current level as working folder for level-specific metadata
	virtual void SetLevelPath(const char *sPath) = 0;

	/// this called before loading (level load/serialization)
	virtual void FlushSystem(void) = 0;
	virtual void FlushSystemNavigation(void) = 0;

	virtual void LoadNavigationData(const char * szLevel, const char * szMission, bool demandLoadLNMs = false) = 0;
	virtual void LoadCover(const char * szLevel, const char * szMission) = 0;

	virtual bool LoadNavMesh(const char * navModifName, const char * agentTypeName) = 0;
	virtual bool UnloadNavMesh(const char * navModifName, const char * agentTypeName) = 0;

	virtual void OnMissionLoaded() = 0;

	//FileIO///////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////





	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Debugging////////////////////////////////////////////////////////////////////////////////////////////////////

	// AI DebugDraw
	virtual IAIDebugRenderer* GetAIDebugRenderer() = 0;
	virtual IAIDebugRenderer* GetAINetworkDebugRenderer()  = 0;
	virtual void SetAIDebugRenderer				(IAIDebugRenderer* pAIDebugRenderer)				= 0;
	virtual void SetAINetworkDebugRenderer(IAIDebugRenderer* pAINetworkDebugRenderer) = 0;

	// debug recorder
	virtual bool	IsRecording( const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event ) const = 0;
	virtual void	Record( const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event, const char* pString ) const = 0;
	virtual void	GetRecorderDebugContext(SAIRecorderDebugContext* &pContext) = 0;
	virtual void	AddDebugLine( const Vec3& start, const Vec3& end, uint8 r, uint8 g, uint8 b, float time ) = 0;
	virtual void	AddDebugSphere(const Vec3& pos, float radius, uint8 r, uint8 g, uint8 b, float time) = 0;

	virtual void	DebugReportHitDamage(IEntity* pVictim, IEntity* pShooter, float damage, const char* material) = 0;
	virtual void	DebugReportDeath(IAIObject* pVictim) = 0;

	// functions to let external systems (e.g. lua) access the AI logging functions. 
	// the external system should pass in an identifier (e.g. "<Lua> ")
	virtual void Warning(const char * id, const char * format, ...)const PRINTF_PARAMS(3, 4) = 0;
	virtual void Error(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4) = 0;
	virtual void LogProgress(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4) = 0;
	virtual void LogEvent(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4) = 0;
	virtual void LogComment(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4) = 0;

	virtual bool IsAIInDevMode() = 0;

	// Draws a fake tracer around the player.
	virtual void DebugDrawFakeTracer(const Vec3& pos, const Vec3& dir) = 0;


	virtual	void GetMemoryStatistics(ICrySizer *pSizer) = 0;

	// debug members ============= DO NOT USE
	virtual void DebugDraw() = 0;

	//Debugging////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Get Subsystems///////////////////////////////////////////////////////////////////////////////////////////////
	virtual IAIRecorder *GetIAIRecorder() = 0;
	virtual INavigation *GetINavigation() = 0;
	virtual IAIPathFinder *GetIAIPathFinder() = 0;
	virtual ICentralInterestManager* GetCentralInterestManager(void) = 0;
	virtual ITacticalPointSystem* GetTacticalPointSystem(void) = 0;
	virtual IBSSProfileManager* GetBSSProfileManager(void) { return NULL; }
	virtual IEmotionalSystem *			GetEmotionalSystem() = 0;
	virtual IEmotionalSystem const *	GetEmotionalSystem() const = 0;
	virtual ICoordinationManager* GetCoordinationManager() const = 0;
	virtual ICommunicationManager* GetCommunicationManager() const = 0;
	virtual ICoverSystem* GetCoverSystem() const = 0;
	virtual ISelectionTreeManager* GetSelectionTreeManager() const = 0;
	virtual ITargetTrackManager* GetTargetTrackManager() const = 0;

	virtual ISmartObjectManager* GetSmartObjectManager() = 0;
	virtual IAIObjectManager* GetAIObjectManager() = 0;
	//Get Subsystems///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//AI Actions///////////////////////////////////////////////////////////////////////////////////////////////////
	VIRTUAL IAIActionManager* GetAIActionManager() = 0;
	//AI Actions///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Leader/Formations////////////////////////////////////////////////////////////////////////////////////////////
	virtual void EnumerateFormationNames(unsigned int maxNames, const char** names, unsigned int* nameCount) const = 0;
	virtual int GetGroupCount(int nGroupID, int flags = GROUP_ALL, int type = 0) = 0;
	virtual IAIObject* GetGroupMember(int groupID, int index, int flags = GROUP_ALL, int type = 0) = 0;
	//Leader/Formations////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Goal Pipes///////////////////////////////////////////////////////////////////////////////////////////////////
	//TODO: get rid of this; => it too many confusing uses to remove just yet
	virtual int AllocGoalPipeId() const = 0;
	//Goal Pipes///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////











	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Navigation / Pathfinding/////////////////////////////////////////////////////////////////////////////////////
	virtual bool CreateNavigationShape(const SNavigationShapeParams &params) = 0;
	virtual void DeleteNavigationShape(const char *szPathName) = 0;
	virtual bool DoesNavigationShapeExists(const char * szName, EnumAreaType areaType, bool road = false) = 0;
	virtual const char*	GetEnclosingGenericShapeOfType(const Vec3& pos, int type, bool checkHeight) = 0;
	virtual bool IsPointInsideGenericShape(const Vec3& pos, const char* shapeName, bool checkHeight) = 0;
	virtual float DistanceToGenericShape(const Vec3& pos, const char* shapeName, bool checkHeight) = 0;
	virtual bool ConstrainInsideGenericShape(Vec3& pos, const char* shapeName, bool checkHeight) = 0;
	virtual const char*	CreateTemporaryGenericShape(Vec3* points, int npts, float height, int type) = 0;
	virtual void EnableGenericShape(const char* shapeName, bool state) = 0;

	virtual const ObstacleData GetObstacle(int nIndex) = 0;

	// Pathfinding properties
	virtual void AssignPFPropertiesToPathType(const string& sPathType, const AgentPathfindingProperties& properties) = 0;
	virtual const AgentPathfindingProperties* GetPFPropertiesOfPathType(const string& sPathType) = 0;
	virtual string GetPathTypeNames() = 0;

	/// Checks for each point in the array if it is inside path obstacles of the specified requester. The corresponding result
	/// value will be set to true if the point is inside obstacle. Returns number of points inside the obstacles.
	virtual unsigned int CheckPointsInsidePathObstacles(IAIObject* requester, const Vec3* points, bool* results, unsigned int n) = 0;

	/// Register a spherical region that causes damage (so should be avoided in pathfinding). pID is just
	/// a unique identifying - so if this is called multiple times with the same pID then the damage region
	/// will simply be moved. If radius <= 0 then the region is disabled.
	virtual void RegisterDamageRegion(const void *pID, const Sphere &sphere) = 0;
	struct SBuildingInfo
	{
		EWaypointConnections waypointConnections;
		float fNodeAutoConnectDistance;
	};

	//Navigation / Pathfinding/////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////








	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Hide spots///////////////////////////////////////////////////////////////////////////////////////////////////

	// Returns specified number of nearest hidespots. It considers the hidespots in graph and anchors.
	// Any of the pointers to return values can be null. Returns number of hidespots found.
	virtual unsigned int GetHideSpotsInRange(IAIObject* requester, const Vec3& reqPos,
		const Vec3& hideFrom, float minRange, float maxRange, bool collidableOnly, bool validatedOnly,
		unsigned int maxPts, Vec3* coverPos, Vec3* coverObjPos, Vec3* coverObjDir, float* coverRad, bool* coverCollidable) = 0;
	// This method will soon replace the one above. /Jonas
	virtual unsigned int GetHideSpots(HidespotQueryContext& queryContext) const = 0;
	// Marcio: HAX
	virtual void SetHideSpotOccupied(const Vec3& pos, bool occupied) = 0;
	// Returns a point which is a valid distance away from a wall in front of the point.
	virtual void AdjustDirectionalCoverPosition(Vec3& pos, const Vec3& dir, float agentRadius, float testHeight) = 0;

	//Hide spots///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////




	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Perception///////////////////////////////////////////////////////////////////////////////////////////////////

	// current global AI alertness value (what's the most alerted puppet)
	virtual int GetAlertness() const = 0;
	virtual int GetAlertness(const IAIAlertnessPredicate& alertnessPredicate) = 0;
	// Returns the exposure and threat levels as perceived by the specified AIobject.
	// If the AIobject is null, the local player values are returns
	virtual void GetDetectionLevels(IAIObject *pObject, SAIDetectionLevels& levels) = 0;
	virtual void SetAssesmentMultiplier(unsigned short type, float fMultiplier) = 0;
	virtual void SetSpeciesThreatMultiplier(int nSpeciesID, float fMultiplier) = 0;
	virtual void SetPerceptionDistLookUp( float* pLookUpTable, int tableSize ) = 0;	//look up table to be used when calculating visual time-out increment
	/// Fills the array with possible dangers, returns number of dangers.
	virtual unsigned int GetDangerSpots(const IAIObject* requester, float range, Vec3* positions, unsigned int* types, unsigned int n, unsigned int flags) = 0;
	virtual const char* GetCustomOnSeenSignal(int combatClass) = 0;
	virtual void RegisterStimulus(const SAIStimulus& stim) = 0;
	virtual void IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time) = 0;
	virtual	void DynOmniLightEvent(const Vec3& pos, float radius, EAILightEventType type, EntityId shooterId, float time = 5.0f) = 0;
	virtual	void DynSpotLightEvent(const Vec3& pos, const Vec3& dir, float radius, float fov, EAILightEventType type, EntityId shooterId, float time = 5.0f) = 0;

	virtual IVisionMap& GetVisionMap() = 0;

	//Perception///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////




	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//WTF are these?///////////////////////////////////////////////////////////////////////////////////////////////
	virtual IAIObject* GetBeacon(unsigned short nGroupID) = 0;
	virtual void UpdateBeacon(unsigned short nGroupID, const Vec3 & vPos, IAIObject *pOwner = 0) = 0;

	virtual	void RegisterFirecommandHandler(IFireCommandDesc* desc) = 0;
	virtual	IFireCommandHandler* CreateFirecommandHandler(const char* name, IAIActor *pShooter) = 0;

	virtual void AddCombatClass(int combatClass, float* pScalesVector, int size, const char* szCustomSignal) = 0;
	virtual float ProcessBalancedDamage(IEntity* pShooterEntity, IEntity* pTargetEntity, float damage, const char* damageType) = 0;
	virtual void NotifyDeath(IAIObject* pVictim) {}
	virtual bool ParseTables(int firstTable, bool parseMovementAbility, IFunctionHandler* pH, AIObjectParameters& aiParams, bool& updateAlways) = 0;
	//WTF are these?///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



};

# if defined(DEVIRT_GEN_MANGLE_NAMES)
#if !defined (_IAGENT_H_)	
#include "../CryAISystem/GraphStructures.h"
#endif
#endif //DEVIRT_GEN_MANGLE_NAMES

#endif //_IAISYSTEM_H_

