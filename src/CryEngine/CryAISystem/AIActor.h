/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2004.
-------------------------------------------------------------------------
File name:   AIActor.h
$Id$
Description: Header for the CAIActor class

-------------------------------------------------------------------------
History:
14:12:2006 -  Created by Kirill Bulatsev

*********************************************************************/

#ifndef AIACTOR_H
#define AIACTOR_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAIActor.h"
#include "IAgent.h"
#include "AIObject.h"
#include "BlackBoard.h"
#include "ValueHistory.h"
#include "IBSSProfileManager.h"

// forward declarations
class COPWaitSignal;
struct IPerceptionHandlerModifier;

class SelectionTree;
class SelectionVariables;

enum EObjectUpdate
{
	AIUPDATE_FULL,
	AIUPDATE_DRY,
};

// Structure reflecting the physical entity parts.
// When choosing the target location to hit, the AI chooses amongst one of these.
struct SAIDamagePart
{
	SAIDamagePart() : pos(0,0,0), damageMult(0.0f), volume(0.0f), surfaceIdx(0) {}
	Vec3	pos;				// Position of the part.
	float	damageMult;	// Damage multiplier of the part, can be used to choose the most damage causing part.
	float	volume;			// The volume of the part, can be used to choose the largest part to hit.
	int		surfaceIdx;	// The index of the surface material.
};

typedef std::vector<SAIDamagePart>	DamagePartVector;
typedef std::vector<IPerceptionHandlerModifier*> TPerceptionHandlerModifiersVector;


/*! Basic ai object class. Defines a framework that all puppets and points of interest
	later follow.
*/
class CAIActor 
	: public CAIObject
	, public IAIActor
	, public IBSSProfileUser
{
public:
  CAIActor();
  virtual ~CAIActor();

  virtual const IAIActor* CastToIAIActor() const { return this; }
  virtual IAIActor* CastToIAIActor() { return this; }

	SelectionTree* GetBehaviorSelectionTree() const;
	SelectionVariables* GetBehaviorSelectionVariables() const;
	void ResetBehaviorSelectionTree();
	bool ProcessBehaviorSelectionTreeSignal(const char* signalName, uint32 signalCRC);
	bool UpdateBehaviorSelectionTree();
	void DebugDrawBehaviorSelectionTree();


  ////////////////////////////////////////////////////////////////////////////////////////
  //IAIPathAgent//////////////////////////////////////////////////////////////////////////
  virtual IEntity *GetPathAgentEntity() const;
  virtual const char *GetPathAgentName() const;
  virtual unsigned short GetPathAgentType() const;
  virtual float GetPathAgentPassRadius() const;
  virtual Vec3 GetPathAgentPos() const;
  virtual Vec3 GetPathAgentVelocity() const;
  virtual void GetPathAgentNavigationBlockers(NavigationBlockers &navigationBlockers, const struct PathfindRequest *pRequest);
  virtual Vec3 GetForcedStartPos() const;

  virtual const AgentMovementAbility &GetPathAgentMovementAbility() const;
  virtual void PathEvent(SAIEVENT *pEvent);

  virtual unsigned int GetPathAgentLastNavNode() const;
  virtual void SetPathAgentLastNavNode(unsigned int lastNavNode);

  virtual void SetPathToFollow( const char* pathName );
  virtual void SetPathAttributeToFollow( bool bSpline );
  virtual void SetPointListToFollow( const std::list<Vec3>& pointList,IAISystem::ENavigationType navType,bool bSpline );

  //Path finding avoids blocker type by radius. 
  virtual void SetPFBlockerRadius(int blockerType, float radius);

  //Can path be modified to use request.targetPoint?  Results are cacheded in request.
  virtual ETriState CanTargetPointBeReached(CTargetPointRequest& request);

  //Is request still valid/use able
  virtual bool UseTargetPointRequest(const CTargetPointRequest& request);//??

  virtual bool GetValidPositionNearby(const Vec3& proposedPosition, Vec3& adjustedPosition) const;
  virtual bool GetTeleportPosition(Vec3& teleportPos) const;
  //IAIPathAgent//////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////

  //===================================================================
  // inherited virtual interface functions
  //===================================================================
  virtual void Reset(EObjectResetType type);
	virtual void OnObjectRemoved(CAIObject *pObject );
  virtual SOBJECTSTATE * GetState(void) {return &m_State;}
  virtual const SOBJECTSTATE * GetState(void) const {return &m_State;}
	virtual void SetSignal(int nSignalID, const char * szText, IEntity *pSender=0, IAISignalExtraData* pData=NULL, uint32 crcCode = 0);
	virtual void NotifySignalReceived(const char* szText, IAISignalExtraData* pData=NULL, uint32 crcCode = 0 );
	virtual bool Serialize( TSerialize ser, class CObjectTracker& objectTracker );
	virtual void Update(EObjectUpdate type);
	virtual void UpdateDisabled(EObjectUpdate type);	// when AI object is disabled still may need to send some signals  
	virtual IAIActorProxy* GetProxy() const { if (!m_pProxy) CacheProxy(); return m_pProxy; };
	virtual bool CanAcquireTarget(IAIObject* pOther) const;
	virtual void ResetPerception();
	virtual bool IsHostile(const IAIObject* pOther, bool bUsingAIIgnorePlayer=true) const;
	virtual void ParseParameters( const AIObjectParameters &params);
	virtual void Event(unsigned short eType, SAIEVENT *pEvent);
	virtual void SetGroupId(int id);

	virtual void RegisterBehaviorListener(IActorBehaviorListener* listener);
	virtual void UnregisterBehaviorListener(IActorBehaviorListener* listener);
	virtual void BehaviorEvent(EBehaviorEvent event);
	virtual void BehaviorChanged(const char* current, const char* previous);

	// IBSSProfileUser
	virtual bool GetBTUserName(string &sOut) const;
	virtual IScriptTable* GetBTUserTable() const;
	virtual IBlackBoard* GetBTUserBlackBoard() const;
	virtual bool CanBTUserTreeRun() const;
	virtual void OnBTUserEvent(EBTUserEvents event, IPersonalBehaviorTree *pTree, const string& sSignal, const ScriptAnyValue* pArgValue = NULL);

	//===================================================================
	// virtual functions rooted here
	//===================================================================

	/// Should populate the list of entities that we don't want to consider when pathfinding
	virtual void UpdateEntitiesToSkipInPathfinding() {}
	const std::vector<IPhysicalEntity *> &GetEntitiesToSkipInPathfinding() const {return m_entitiesToSkipInPathFinding;}
	/// This should add any navigation blockers to the list that are specific to the 
	/// AI object (e.g. locations it wishes to avoid when pathfinding).
	/// pfr may be 0. if it's non-zero then the nav blockers can be tailored to suit
	/// the specific path
	
	// Returns a list containing the information about the parts that can be shot at.
	virtual DamagePartVector*	GetDamageParts() { return 0; }

	const AgentParameters &GetParameters() const { return m_Parameters;}
	virtual void SetParameters(AgentParameters &params);
	virtual const AgentMovementAbility& GetMovementAbility() const {return m_movementAbility;}
	virtual void SetMovementAbility(AgentMovementAbility &params) {m_movementAbility = params;}
	// (MATT) There is now a method to serialise these {2009/04/23}
	
	virtual bool IsLowHealthPauseActive() const { return false; }
	virtual IEntity* GetGrabbedEntity() const {return 0;}	// consider only player grabbing things, don't care about NPC

	void GetLocalBounds(AABB& bbox) const;
	
	// Perception handler modifiers
	virtual bool AddPerceptionHandlerModifier(IPerceptionHandlerModifier *pModifier);
	virtual bool RemovePerceptionHandlerModifier(IPerceptionHandlerModifier *pModifier);
	bool GetPerceptionHandlerModifiers(TPerceptionHandlerModifiersVector &outModifiers);

	//===================================================================
	// non-virtual functions
	//===================================================================

	// Get PersonalBehaviorTree
	TBSSProfileUserId GetBehaviorTreeUserId() const { return m_uBSSProfileUserId; }
	bool SetBehaviorTreeProfile(const char* szProfile);
	// Returns true if the agent cannot be seen from the specified point.
	bool IsInvisibleFrom(const Vec3& pos) const;
	void NotifyDeath();
	// Returns true if the cloak is effective (includes check for cloak and 
	bool IsCloakEffective() const;
	// returns current min/max cloak distances (those are fading in/out)
	float GetCloakMinDist( ) const;
	float GetCloakMaxDist( ) const;

	// Used to adjust the visibility range for an object checking if they can see me
	virtual float AdjustTargetVisibleRange(const CAIActor& observer, float fVisibleRange) const;

	// Returns the maximum visible range to the target
	virtual float GetMaxTargetVisibleRange(const CAIObject* pTarget) const;

	// Gets the light level at actors position.
	inline EAILightLevel GetLightLevel() const { return m_lightLevel; }

	// Populates list of physics entities to skip for raycasting.
	virtual void GetPhysicsEntitiesToSkip(std::vector<IPhysicalEntity*>& skips) const;

	inline void RegisterDetectionLevels(const SAIDetectionLevels& levels) { m_detectionMax.Append(levels); }
	inline void ResetDetectionLevels() { m_detectionMax.Reset(); }
	inline const SAIDetectionLevels& GetDetectionLevels() const { return m_detectionSnapshot; }
	inline void SnapshotDetectionLevels() { m_detectionSnapshot = m_detectionMax; }

	SOBJECTSTATE		m_State;
	AgentParameters	m_Parameters;
	AgentMovementAbility	m_movementAbility;

	// list of wait goal operations to be notified when a signal is received
	// list of pointers is a waste of resources (allocated in  16 byte chunks, no locality)
	// use vector instead
//	typedef std::list< COPWaitSignal* > ListWaitGoalOps;
	typedef std::vector< COPWaitSignal* > ListWaitGoalOps;
	ListWaitGoalOps m_listWaitGoalOps;

	CValueHistory<float>* m_healthHistory;
	
	// WIP

	struct SProbableTarget
	{
		SProbableTarget(CAIObject* pTarget, int los = -1) :
			pTarget(pTarget), lineOfSight(los), updates(0), dirty(false)
		{
			// Empty
		}

		CAIObject* pTarget;
		int lineOfSight;
		int updates;
		bool dirty;
	};

	std::vector<SProbableTarget>	m_probableTargets;
	std::vector<SProbableTarget>	m_priorityTargets;


	void	AddProbableTarget(CAIObject* pTarget)
	{
		for (unsigned i = 0, ni = m_probableTargets.size(); i < ni; ++i)
		{
			if (m_probableTargets[i].pTarget == pTarget)
			{
				m_probableTargets[i].dirty = false;
				return;
			}
		}
		m_probableTargets.push_back(SProbableTarget(pTarget));
	}

	void	ClearProbableTargets()
	{
		for (unsigned i = 0, ni = m_probableTargets.size(); i < ni; ++i)
			m_probableTargets[i].dirty = true;
	}

	int	HasLineOfSightTo(CAIObject* pTarget)
	{
		for (unsigned i = 0, ni = m_probableTargets.size(); i < ni; ++i)
		{
			const SProbableTarget& prob = m_probableTargets[i];
			if (prob.pTarget == pTarget)
			{
				if (prob.dirty || prob.updates > 3)
					return -1;
				return prob.lineOfSight;
			}
		}
		return -1;
	}

	void	UpdateProbableTargets()
	{
		for (unsigned i = 0; i < m_probableTargets.size(); )
		{
			m_probableTargets[i].updates++;
			if (m_probableTargets[i].dirty)
			{
				m_probableTargets[i] = m_probableTargets.back();
				m_probableTargets.pop_back();
			}
			else
				++i;
		}
	}
	
	void	UpdatePriorityTargets()
	{
		for (unsigned i = 0; i < m_priorityTargets.size(); )
		{
			m_priorityTargets[i].updates++;
			if (m_priorityTargets[i].updates > 2)
			{
				m_priorityTargets[i] = m_priorityTargets.back();
				m_priorityTargets.pop_back();
			}
			else
				++i;
		}
	}

	void	RefreshPriorityTarget(CAIObject* pTarget, int los)
	{
		for (unsigned i = 0, ni = m_priorityTargets.size(); i < ni; ++i)
		{
			if (m_priorityTargets[i].pTarget == pTarget)
			{
				m_priorityTargets[i].lineOfSight = los;
				m_priorityTargets[i].updates = 0;
				return;
			}
		}
		m_priorityTargets.push_back(SProbableTarget(pTarget, los));
	}

	virtual void EnablePerception(bool enable);
	virtual bool IsPerceptionEnabled() const;

	virtual bool	IsActive() const { return false; }

	inline bool IsUsingCombatLight() const { return m_usingCombatLight; }

	inline float GetCachedWaterOcclusionValue() const { return m_cachedWaterOcclusionValue; }

	virtual IBlackBoard*	GetBlackBoard()	{ return &m_blackBoard; }
	virtual IBlackBoard*	GetBehaviorBlackBoard() { return &m_behaviorBlackBoard; }


protected:
	void SerializeMovementAbility(TSerialize ser);

	void UpdateHealthHistory();
	//process cloak_scale fading
	void UpdateCloakScale();

	// Updates the properties of the damage parts.
	void	UpdateDamageParts(DamagePartVector& parts);

	// Debug draw for perception handler modifiers
	void DebugDrawPerceptionHandlerModifiers();

	mutable IAIActorProxy *m_pProxy;		// (MATT) This is a cache, which may be unset {2009/04/03}

	/// entities that should not affect pathfinding - e.g. if we're carrying a physical object, or we
	/// know our destination is inside a specific entity.
	std::vector<IPhysicalEntity *> m_entitiesToSkipInPathFinding;

	EAILightLevel m_lightLevel;
	bool m_usingCombatLight;
	int8 m_perceptionDisabled;

	float m_cachedWaterOcclusionValue;

	Vec3 m_vLastFullUpdatePos;

	SAIDetectionLevels	m_detectionMax;
	SAIDetectionLevels	m_detectionSnapshot;
	CBlackBoard		m_blackBoard;
	CBlackBoard		m_behaviorBlackBoard;

	TBSSProfileUserId m_uBSSProfileUserId;

	TPerceptionHandlerModifiersVector m_perceptionHandlerModifiers;

	typedef std::set<IActorBehaviorListener*> BehaviorListeners;
	BehaviorListeners m_behaviorListeners;

	std::auto_ptr<SelectionTree> m_behaviorSelectionTree;
	std::auto_ptr<SelectionVariables> m_behaviorSelectionVariables;
private:

	void CacheProxy() const;	// It's just updating a mutatable cache pointer
};

inline const CAIActor* CastToCAIActorSafe(const IAIObject* pAI) { return pAI ? pAI->CastToCAIActor() : 0; }
inline CAIActor* CastToCAIActorSafe(IAIObject* pAI) { return pAI ? pAI->CastToCAIActor() : 0; }

#endif 
