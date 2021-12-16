/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 04:09:2009 : Created by James Bamford

*************************************************************************/

#ifndef __GAMERULESMPSPAWNING_H__
#define __GAMERULESMPSPAWNING_H__

#include "GameRulesModules/GameRulesSpawningBase.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "Utility/CryFixedArray.h"

#define DEBUG_SPAWNING_VISTABLE 1

#define VISTABLE_USE_DEFERRED_LINETESTS		1

#if VISTABLE_USE_DEFERRED_LINETESTS
#include "IDeferredRaycastManager.h"
#endif

class CGameRules;
#include "GameRules.h"	// only needed for CryLog below

#define ETSTList(f) \
	f(eTST_None)						/*0*/ \
	f(eTST_Standard)				/*1*/ \
	f(eTST_RoundSwap)				/*2*/ \

AUTOENUM_BUILDENUMWITHTYPE_WITHNUM(ETeamSpawnsType, ETSTList, eTST_NUM);

class CSvSpawningVisTable
{
protected:

	enum EVisibleState
	{
		EVFL_UNTESTED = 0,
		EVFL_VISIBLE,
		EVFL_NOT_VISIBLE
	};

#if VISTABLE_USE_DEFERRED_LINETESTS
	typedef struct STableElement : public IDeferredRaycastReceiver
#else
	typedef struct STableElement
#endif
	{
		EntityId m_playerEntityId;
		uint32 m_visibleState;
#if DEBUG_SPAWNING_VISTABLE
		float m_timeLastTested;
#endif

#if VISTABLE_USE_DEFERRED_LINETESTS
		CDeferredRaycastHelper	m_raycastHelper;
#endif

		STableElement()		// for placement new only
		{
			m_playerEntityId = 0;
			m_visibleState=EVFL_UNTESTED;
#if DEBUG_SPAWNING_VISTABLE
			m_timeLastTested=0.f;
#endif
		}

		STableElement(EntityId playerId)
		{
			m_playerEntityId = playerId;
			m_visibleState = EVFL_UNTESTED;
#if DEBUG_SPAWNING_VISTABLE
			m_timeLastTested = 0.0f;
#endif
		}

		virtual ~STableElement()
		{
			//CryLog("~STableElement() this=%p; m_raycastHelper=%p; helper's impl=%p", this, &m_raycastHelper, m_raycastHelper.m_pImpl);
		}

#if VISTABLE_USE_DEFERRED_LINETESTS
		void UpdateRaycastHelper()
		{
			//CryLog("CSvSpawningVisTable::STableElement::UpdateRaycastHelper() setting element for player %s raycasthelper's receiver to this=%p\n", g_pGame->GetGameRules()->GetEntityName(m_playerEntityId), this);
			m_raycastHelper.SetReceiver(this);
		}

		//async raycasts results callback
		virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);

		//async primitive casts results callback
		virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult) {}

		//reset data callback
		virtual void OnDRWReset() {}
#endif
	};

	typedef CryFixedArray<STableElement, 16> TPlayerElements;
	//typedef std::vector<STableElement>	TPlayerElements;
	typedef struct SSpawnTableVectorElement
	{
		TPlayerElements playerElements;
		EntityId spawnEntityId;							// may only be needed for debugging integrity of map to vector links
		EntityId altSpawnEntityId;					// if set then use this entityId's visibility instead of calculating our own
		bool doVisibilityTest;

		SSpawnTableVectorElement()
		{
			spawnEntityId=0;
			altSpawnEntityId=0;
			doVisibilityTest=false;
		}

		SSpawnTableVectorElement(EntityId inSpawnEntityId, TPlayerElements &players, bool doVisTest) :
			spawnEntityId(inSpawnEntityId),
			playerElements(players),
			altSpawnEntityId(0),
			doVisibilityTest(doVisTest)
		{
		}
	};

	typedef struct SSpawnTableMapElement
	{
		int index;			// index into the vector for this element

		SSpawnTableMapElement(int inIndex) : index(inIndex) { }
	};

	typedef std::vector<SSpawnTableVectorElement> TSpawnVisTableVec;
	typedef std::map<EntityId, SSpawnTableMapElement>	TSpawnVisTableMap;

	TPlayerElements m_players;	// to generate new vistable elements from if neccessary
	
	// This optimisation stops the vistable from coping with dynamically adding and removing spawn points on the fly
	// vector elements need to remain inplace during the lifetime of the game. On teardown they'll all be completely invalidated 
	TSpawnVisTableVec m_tableVec;	// used for iterations
	TSpawnVisTableMap m_tableMap; // used for fast lookups, references an element in the vector
	
	//static const int k_numberOfLineTestsToDoPerUpdate = 5;		// uses a cvar now
	static const float k_spawnLocationZOffset;
	static const float k_playerZOffset; // TODO - hook into actual head position of player characters

	unsigned int m_tableIndex;						// used for time sliced iteration of m_tableMap/Vec
	unsigned int m_elementsIndex;				// used for time sliced iteration of each m_tableMap/Vec elements player elements

	ETeamSpawnsType m_teamSpawnsType;
	CGameRules *m_pGameRules;

#if DEBUG_SPAWNING_VISTABLE
	float m_timeLastStartedNewUpdate;
	float m_timeLastFullTableUpdateTook;
	int m_numUpdatesSinceLastFullTableUpdate;
	int m_numUpdatesForLastFullUpdate;
	float m_lastTimeFullUpdateLogged;
	int m_numSpawnsVisTestedLastFullUpdate;
	int m_numSpawnsVisTested;
#endif

	void SetTableIndex(unsigned int newTableIndex);
	void SetElementsIndex(unsigned int newElementsIndex);

public:
	CSvSpawningVisTable();
	virtual ~CSvSpawningVisTable();

	void Init(ETeamSpawnsType teamSpawnsType);

	void AddSpawnLocation(EntityId location, bool doVisibilityTest);
	void RemoveSpawnLocation(EntityId location);
	void PlayerJoined(EntityId playerId);
	void PlayerLeft(EntityId playerId);

	bool RemoveElementForPlayer(EntityId playerId, TPlayerElements &elements);

	void IncrementTableIndex();
	bool GetNthTableElement(unsigned int n, const EntityId **outSpawnLocation, CSvSpawningVisTable::SSpawnTableVectorElement **outElement);
	bool HandleFindingAltSpawnLocation(SSpawnTableVectorElement *element, IEntity *locationEntity);
	bool GetNextElementToTest(const EntityId **outSpawnLocation, STableElement **outTableElement, int &outUsedTableIndex, int &outUsedElementsIndex);

	bool ShouldTestElement(EntityId spawnEntityId, const STableElement *tableElement, const IEntity *spawnEntity, const IEntity *playerEntity);
	void Update();
#if DEBUG_SPAWNING_VISTABLE
	void Debug();
	void VisuallyDebugVisTests();
	void VisuallyDebugAltSpawns();
	void VisuallyDebug();
#endif

	bool IsSpawnLocationVisibleByTeam(EntityId location, int teamId) const;
	bool CanPlayerSeeSpawnLocation(EntityId playerId, EntityId location) const;
};

class CGameRulesMPSpawningBase :	public CGameRulesSpawningBase
{
private:
	typedef CGameRulesSpawningBase inherited;

protected:
	bool m_teamGame;
	ETeamSpawnsType m_teamSpawnsType;
	bool m_handleAutoReviving;
	bool m_localOptedOut;
	float m_minTimeFromDeathTillRevive;
	float m_timeFromDeathTillAutoRevive;
	bool m_usingLua;
	bool m_allowMidRoundJoining;

	typedef std::map<int, float> TTeamMinEnemyDistMap;
	TTeamMinEnemyDistMap m_minEnemyDistanceMap;

	CSvSpawningVisTable m_vistable;

	enum EPointOfInterestState
	{
		EPOIS_NONE=0,
		EPOIS_AVOID,		// maintain a minimum distance away from POI when spawning
		EPOIS_ATTRACT,	// maintain a maximum distance away from POI when spawning
	};

	enum EPointOfInterestFlags
	{
		EPOIFL_NONE							= 0,
		EPOIFL_ENABLED					= (1<<0),
	};

	typedef struct SPointOfInterest
	{
		EntityId m_entityId;
		uint8 m_state;
		uint32 m_flags;		// maybe be state instead	

		union UStateData
		{
			struct 
			{
				float avoidDistance;
			}	avoid;
			struct 
			{
				float attractDistance;
			} attract;
		};

		UStateData m_stateData;

		SPointOfInterest()
		{
			memset(this, 0, sizeof(this));
		}

		void SetToAvoidEntity(EntityId entityId, float avoidDistance, bool enabled=true)
		{
			m_entityId=entityId;
			m_state=EPOIS_AVOID;
			m_stateData.avoid.avoidDistance=avoidDistance;
			if (enabled)
			{
				Enable();
			}
			else
			{
				Disable();
			}
		}

		void SetToAttractEntity(EntityId entityId, float attractDistance, bool enabled)
		{
			m_entityId=entityId;
			m_state=EPOIS_ATTRACT;
			m_stateData.attract.attractDistance=attractDistance;
			if (enabled)
			{
				Enable();
			}
			else
			{
				Disable();
			}
		}

		void Enable()
		{
			m_flags |= EPOIFL_ENABLED;
		}

		void Disable()
		{
			m_flags &= ~EPOIFL_ENABLED;
		}
	};

	typedef std::vector<SPointOfInterest>								TPointsOfInterest;
	TPointsOfInterest m_pointsOfInterest;

public:
	CGameRulesMPSpawningBase();
	virtual ~CGameRulesMPSpawningBase();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest);
	virtual void RemoveSpawnLocation(EntityId id, bool isBaseSpawn);
	virtual void PlayerJoined(EntityId playerId);
	virtual void PlayerLeft(EntityId playerId);
	virtual void SvOnPlayerKilled(const HitInfo &hitInfo);

	virtual void AddAvoidPOI(EntityId entityId, float avoidDistance, bool enabled);
	virtual void RemovePOI(EntityId entityId);
	virtual void EnablePOI(EntityId entityId);
	virtual void DisablePOI(EntityId entityId);

	virtual void ClRequestRevive(EntityId playerId);
	virtual void SvRequestRevive(EntityId playerId);
	virtual void PerformRevive(EntityId playerId, int teamId);

	virtual void HandleOptOutOfSpawn(EntityId entityId);
	virtual void SvRecordReviveOptOut(EntityId entityId);

	virtual EntityId GetSpawnLocation(EntityId playerId, float &zOffset);

	virtual int GetSpawnLocationTeam(EntityId spawnLocEid) const;

	virtual void ReviveAllPlayers(bool isReset);

	virtual ILINE bool HandleAutoRevive() const { return m_handleAutoReviving; }
	virtual ILINE float GetTimeFromDeathTillAutoRevive() const { return m_timeFromDeathTillAutoRevive; }
	virtual ILINE bool HasOptedOut() const { return m_localOptedOut; }

	virtual void HostMigrationInsertIntoReviveQueue(EntityId playerId, float timeTillRevive);

	bool CheckMidRoundJoining(EntityId playerId);

protected:
	float GetMinEnemyDist(int teamId, bool log=true);

	bool TestSpawnLocationWithEnvironment(EntityId spawnLocationId, EntityId playerId, float offset, float height) const;
	bool IsSpawnLocationSafeFromExplosives(EntityId spawnLocationId) const;
	bool IsSpawnLocationSafe(EntityId playerId, EntityId spawnLocationId, float safeDistance, bool ignoreTeam, float zoffset) const;
	bool IsSpawnLocationFarEnough(EntityId spawnLocationId, float minDistance, const Vec3 &testPosition) const;
	bool DoesSpawnLocationRespectPOIs(EntityId spawnLocationId) const;
	void DebugPOIs() const;

	EntityId GetSpawnLocationFurthestAwayFromAnyPlayer(EntityId playerId, const Vec3 &deathPos, float minDistToDeathSqr) const;
	EntityId GetSpawnLocationFurthestAwayFromEnemy(const TSpawnLocations &spawnLocations, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr);
	EntityId GetSpawnLocationClosestToTeamMate(const TSpawnLocations &spawnLocations, EntityId playerId, int playerTeamId, const Vec3 &deathPos, float minDistToDeathSqr);
	EntityId GetRandomSpawnLocation(const TSpawnLocations &spawnLocations, int playerTeamId) const;

	EntityId GetSpawnLocationNonTeamGame(EntityId playerId, const Vec3 &deathPos);
	virtual EntityId GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos) = 0;

	EntityId GetSpawnLocationOld(EntityId playerId, bool ignoreTeam, bool includeNeutral, EntityId groupId, float minDistToDeath, const Vec3 &deathPos, float *pZOffset) const;

	virtual void ClAutoReviveOptOut();

	int GetTeamSpawnLocations(int teamId, TSpawnLocations &results);
};

#endif // __GAMERULESMPSPAWNING_H__
