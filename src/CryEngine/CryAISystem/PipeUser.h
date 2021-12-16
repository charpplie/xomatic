/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   PipeUser.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2 Mar 2009			 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _PIPE_USER_
#define _PIPE_USER_

#if _MSC_VER > 1000
#pragma once
#endif

// created by Petar

#include "GoalPipe.h"
#include "AIActor.h"
#include "IAISystem.h"
#include "AIHideObject.h"
#include "Reference.h"

struct PathFollowerParams;

struct CoverUsageInfo
{
	CoverUsageInfo()
		: lowLeft(false)
		, lowCenter(false)
		, lowRight(false)
		, highLeft(false)
		, highCenter(false)
		, highRight(false)
		, lowCompromised(false)
		, highCompromised(false)
	{
	};

	CoverUsageInfo(bool state)
		: lowLeft(state)
		, lowCenter(state)
		, lowRight(state)
		, highLeft(state)
		, highCenter(state)
		, highRight(state)
		, lowCompromised(state)
		, highCompromised(state)
	{
	};


	bool lowLeft:1;
	bool lowCenter:1;
	bool lowRight:1;
	bool lowCompromised:1;
	bool highLeft:1;
	bool highCenter:1;
	bool highRight:1;
	bool highCompromised:1;
};

// Structured that can be passed to FindHidePoint to return some information for debugging.
struct SDebugHideSpotQuery
{
	bool found;
	Vec3 hidePos;
	Vec3 hideDir;
	struct SHideSpot
	{
		SHideSpot(const Vec3& p, const Vec3& d, float w) : pos(p), dir(d), weight(w) {}
		Vec3 pos, dir;
		float weight;
	};
	std::vector<SHideSpot> spots;
};

enum EAimState
{
	AI_AIM_NONE,				// No aiming requested
	AI_AIM_WAITING,			// Aiming requested, but not yet ready.
	AI_AIM_OBSTRUCTED,	// Aiming obstructed.
	AI_AIM_READY,
	AI_AIM_FORCED,
};


class CGoalPipe;

class CPipeUser
	: public CAIActor
	, public CPipeUserAdapter
{
public:
	CPipeUser();
	virtual ~CPipeUser();

	virtual const IPipeUser* CastToIPipeUser() const { return this; }
	virtual IPipeUser* CastToIPipeUser() { return this; }

	Vec3 GetForcedStartPos() const { return m_forcePathGenerationFrom; };

	virtual void	RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData=NULL);
	virtual void	RecordSnapshot();

	virtual void Reset(EObjectResetType type);
	virtual void SetName(const char *pName);
	void GetStateFromActiveGoals(SOBJECTSTATE &state);
	CGoalPipe *GetGoalPipe(const char *name);
	void RemoveActiveGoal(int nOrder);

	void SetAttentionTarget(CWeakRef<CAIObject> refTarget);

	virtual void ClearPotentialTargets() {};
	virtual void ClearPendingEvents() {};
	void SetLastOpResult(CWeakRef<CAIObject> refObject);
	bool IsCoverCompromised();
	bool IsTakingCover(float distanceThreshold);
	bool IsMovingToCover();

  /// fUrgency would normally be a value from AISPEED_WALK and similar. This returns the "normal" movement
  /// speed associated with each for the current stance - as a guideline for the AI system to use in its requests. 
  /// For vehicles the urgency is irrelevant. 
  /// if slowForStrafe is set then the returned speed depends on last the body/move direction
//  virtual float GetNormalMovementSpeed(float fUrgency, bool slowForStrafe) const {return 0.0f;}
  /// Returns the speed (as GetNormalMovementSpeeds) used to do accurate maneuvering/tight turning. 
//  virtual float GetManeuverMovementSpeed() {return 0.0f;}

	virtual void GetMovementSpeedRange(float fUrgency, bool slowForStrafe, float& normalSpeed, float& minSpeed, float& maxSpeed) const { normalSpeed = 0; minSpeed = 0; maxSpeed = 0; }

  /// Steers the puppet outdoors and makes it avoid the immediate obstacles (or slow down). This finds all the potential
  /// AI objects that must be navigated around, and calls NavigateAroundAIObject on each.
  /// targetPos is a reasonably stable position ahead on the path that should be steered towards.
  /// fullUpdate indicates if this is called from a full or "dry" update
  /// Returns true if some steering needed to be done
  virtual bool NavigateAroundObjects(const Vec3 & targetPos, bool fullUpdate, bool bSteerAroundTarget=true) {return false;}

  /// returns the path follower if it exists (it may be created if appropriate). May return 0
  class IPathFollower *GetPathFollower();

	// Finds hide point in graph based on specified search method.
	virtual Vec3 FindHidePoint(float fSearchDistance, const Vec3 &hideFrom, int nMethod, IAISystem::ENavigationType navType, bool bSameOk=false,float minDist=0, SDebugHideSpotQuery* pDebug = 0) {return GetPos();}
  virtual void RequestPathTo(const Vec3 &pos, const Vec3 &dir, bool allowDangerousDestination, int forceTargetBuildingId = -1, float endTol = std::numeric_limits<float>::max(), float endDistance = 0.0f, CAIObject* pTargetObject = 0);
	virtual void RequestPathInDirection(const Vec3 &pos, float distance, CWeakRef<CAIObject> refTargetObject, float endDistance = 0.0f);
  /// inherited
	virtual void SetPathToFollow(const char *pathName);
	virtual void SetPathAttributeToFollow(bool bSpline);

	const char* GetPathToFollow() const { return m_pathToFollowName.c_str(); }
	/// returns the entry point on the designer created path.
	virtual bool GetPathEntryPoint(Vec3& entryPos, bool reverse, bool startNearest) const;
  /// use a predefined path - returns true if successful
  virtual bool UsePathToFollow(bool reverse, bool startNearest, bool loop);
  virtual void SetPointListToFollow( const std::list<Vec3>& pointList,IAISystem::ENavigationType navType,bool bSpline );
  virtual bool UsePointListToFollow( void );
	virtual bool IsDevalued(IAIObject *pObject) { return false; }
	virtual void ClearDevalued() {};
	virtual void Forget(CAIObject *pDummyObject) {}
	virtual void UpdateLook(void) {}
	virtual void Navigate3d(CAIObject *pTarget) {}
	inline virtual void MakeIgnorant(bool ignorant) {m_bCanReceiveSignals = !ignorant;};
	CGoalPipe *GetCurrentGoalPipe() { return m_pCurrentGoalPipe;}
  const CGoalPipe *GetCurrentGoalPipe() const { return m_pCurrentGoalPipe;}
  CGoalPipe *GetActiveGoalPipe() {return m_pCurrentGoalPipe ? m_pCurrentGoalPipe->GetLastSubpipe() : 0;}
  const CGoalPipe *GetActiveGoalPipe() const {return m_pCurrentGoalPipe ? m_pCurrentGoalPipe->GetLastSubpipe() : 0;}
  const char *GetActiveGoalPipeName() const {const CGoalPipe *pipe = GetActiveGoalPipe(); return pipe ? pipe->GetName() : "No Active GoalPipe";}
	void ResetCurrentPipe( bool resetAlways );
	int GetGoalPipeId() const;
	void ResetLookAt();
	bool SetLookAtPointPos( const Vec3& point, bool priority=false );
	bool SetLookAtDir( const Vec3& dir, bool priority=false );
	void CreateLookAtTarget();
	void ResetBodyTargetDir();
	void SetBodyTargetDir( const Vec3& dir );
	const Vec3 &GetBodyTargetDir() const { return m_vBodyTargetDir; }
	// Sets the allowed start and end of the path strafing distance. 
	void SetAllowedStrafeDistances(float start, float end, bool whileMoving) {}

	void SetExtraPriority(float priority){ m_AttTargetPersistenceTimeout = priority; };
	float GetExtraPriority() { return m_AttTargetPersistenceTimeout; };

	// Set/reset the looseAttention target to some existing object
	//id is used when clearing the loose attention target
	// m_looseAttentionId is the id of the "owner" of the last SetLooseAttentionTarget(target) operation
	// if the given id is not the same, it means that a more recent SetLooseAttentionTarget() has been set
	// by someone else so the requester can't clear the loose att target	int SetLooseAttentionTarget(CAIObject* pObject=NULL,int id=0);
	int SetLooseAttentionTarget(CWeakRef<CAIObject> refObject, int id = -1);

	void SetLookStyle(ELookStyle eLookStyle);

	ELookStyle GetLookStyle();

	// Set/reset the looseAttention target to the lookAtPoint and its position
	inline int SetLooseAttentionTarget(Vec3& pos)
	{
		SetLookAtPointPos(pos);
		return ++m_looseAttentionId ;
	}
	
	Vec3 GetLooseAttentionPos() ;
	
	inline int GetLooseAttentionId()
	{
		return m_looseAttentionId;
	}

	void RegisterAttack(const char *name);
	void RegisterRetreat(const char *name);
	void RegisterWander(const char *name);
	void RegisterIdle(const char *name);
	bool SelectPipe(int id,const char *name, CWeakRef<CAIObject> refArgument, int goalPipeId=0, bool resetAlways=false);
	IGoalPipe* InsertSubPipe(int id, const char * name, CWeakRef<CAIObject> refArgument, int goalPipeId = 0);
	bool CancelSubPipe(int goalPipeId);
	bool RemoveSubPipe(int goalPipeId, bool keepInserted = false);
	bool IsUsingPipe(const char *name) const;
	bool IsUsingPipe(int goalPipeId) const;
	bool AbortActionPipe( int goalPipeId );
	bool SetCharacter(const char *character, const char* behaviour = NULL);
	bool IsUsing3DNavigation(); // returns true if it's currently using a 3D navigation 

	virtual void Pause(bool pause);
	virtual bool IsPaused() const;

	// Returns true if the puppet wants to fire.
	bool AllowedToFire() const { return (m_fireMode != FIREMODE_OFF) && (m_fireMode != FIREMODE_AIM) && (m_fireMode != FIREMODE_AIM_SWEEP); }
	// Sets the fire mode and shoot target selection (attention target or lastopresult).
	virtual void SetFireMode(EFireMode mode);
	virtual void SetFireTarget(CWeakRef<CAIObject> refTargetObject);
	virtual EFireMode GetFireMode() const { return m_fireMode; }
	// Inherited
	virtual bool	IsAgent() const { return true; }

	// Inherited from IPipeUser
	virtual CAIObject *GetRefPoint() {if (m_refRefPoint.IsNil()) CreateRefPoint(); return m_refRefPoint.GetAIObject();}

	virtual void SetRefPointPos(const Vec3 &pos);
	virtual void SetRefPointPos(const Vec3 &pos, const Vec3 &dir);
	virtual void SetRefShapeName(const char* shapeName);
	virtual const char* GetRefShapeName() const;
	virtual Vec3 GetProbableTargetPosition();
	SShape*	GetRefShape();

	void CreateRefPoint();

	// Sets an actor target request.
	virtual void SetActorTargetRequest(const SAIActorTargetRequest& req);
	// 
	void ClearActorTargetRequest();
	//  
	const SAIActorTargetRequest* GetActiveActorTargetRequest() const;

	virtual void IgnoreCurrentHideObject(float timeOut);

	virtual EntityId GetLastUsedSmartObjectId() const { return m_idLastUsedSmartObject; }
	virtual void ClearPath( const char* dbgString );

	/// Called when it is detected that our current path is invalid - if we want
	/// to continue (assuming we're using it) we should request a new one
	void PathIsInvalid();

	// Check if specified hideobject was recently unreachable.
	bool	WasHideObjectRecentlyUnreachable(const Vec3& pos) const;

	/// Returns the last live target position.
	const Vec3&	GetLastLiveTargetPosition() const { return m_lastLiveTargetPos; }
	float				GetTimeSinceLastLiveTarget() const { return m_timeSinceLastLiveTarget; }

	virtual IAIObject* GetAttentionTarget() const { return m_refAttentionTarget.GetAIObject(); }
	//virtual CWeakRef<CAIObject> GetAttentionTarget() const { return m_refAttentionTarget; }
	virtual IAIObject *GetAttentionTargetAssociation() const
	{
		CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
		return (pAttentionTarget ? pAttentionTarget->GetAssociation().GetAIObject() : NULL);
	}

	virtual CAIObject *GetLastOpResult() { return m_refLastOpResult.GetAIObject(); }
	virtual CAIObject* GetSpecialAIObject(const char* objName, float range = 0.0f);

	inline virtual EAITargetThreat GetAttentionTargetThreat() const { return m_AttTargetThreat; }
	inline virtual EAITargetType GetAttentionTargetType() const { return m_AttTargetType; }

	void ClearInvalidatedSOLinks();
	void InvalidateSOLink( CSmartObject* pObject, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper ) const;
	bool IsSOLinkInvalidated( CSmartObject* pObject, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper ) const;
	bool ConvertPathToSpline( IAISystem::ENavigationType navType );

	bool IsInCover() const;
	void SetInCover(bool inCover);
	void SetCoverCompromised();

	void SetMovingToCover(bool movingToCover);

	AsyncState GetCoverUsageInfo(CoverUsageInfo& usageInfo);

	inline void SetLastSOExitHelperPos(const Vec3& pos)
	{
		m_vLastSOExitHelper = pos;
	}

	inline virtual Vec3 GetLastSOExitHelperPos()
	{
		return m_vLastSOExitHelper;
	}

	virtual SHideSpotInfo::EHideSpotType GetCurrentHideObjectPos(Vec3 &hidePos, Vec3 &objectDir);

	typedef std::set< std::pair< CSmartObject*, std::pair< SmartObjectHelper*, SmartObjectHelper* > > > TSetInvalidatedSOLinks;
	mutable TSetInvalidatedSOLinks m_invalidatedSOLinks;

	// DEBUG MEMBERS
	EGoalOperations m_lastExecutedGoalop;
	//-----------------------------------

	CWeakRef<CAIObject> m_refLastOpResult;

	// position of potentially usable smart object
	Vec3			m_posLookAtSmartObject;

	CNavPath	m_Path;
	CNavPath	m_OrigPath;
	Vec3			m_PathDestinationPos;		//! The last point on path before 
	int				m_nMovementPurpose;			//! Defines the way the movement to the target, 0 = strict 1 = fastest, forget end direction.
	float			m_fTimePassed; //! how much time passed since last full update

	CWeakRef<CAIObject>	m_refPathFindTarget;		//! The target object of the current path find request, if applicable.
	
	/// If non-zero then any path tracing should regenerate the path from this position (and then set it to zero)
	Vec3			m_forcePathGenerationFrom;
	bool			m_bLooseAttention;		// true when we have don't have to look exactly at our target all the time

	CWeakRef<CAIObject>	m_refLooseAttentionTarget;

	bool					m_bPriorityLookAtRequested;

	CAIHideObject	m_CurrentHideObject;

	Vec3			m_vLastMoveDir;	// were was moving last - to be used while next path is generated
	bool			m_bKeepMoving;
	int				m_nPathDecision;

	bool			m_IsSteering;	// if stering around local obstacle now
  float     m_flightSteeringZOffset; // flight navigation only 

	ENavSOMethod	m_eNavSOMethod; // defines which method to use for navigating through a navigational smart object
	bool					m_navSOEarlyPathRegen;
	EntityId			m_idLastUsedSmartObject;

	ListPositions		m_DEBUGCanTargetPointBeReached;
	Vec3						m_DEBUGUseTargetPointRequest;

	SNavSOStates	m_currentNavSOStates;
	SNavSOStates	m_pendingNavSOStates;

	int	m_actorTargetReqId;

	// The movement reason is for debug purposes only.
	enum EMovementReason
	{
		AIMORE_UNKNOWN,
		AIMORE_TRACE,
		AIMORE_MOVE,
		AIMORE_MANEUVER,
		AIMORE_SMARTOBJECT,
	};
	int		m_DEBUGmovementReason;

	virtual void DebugDrawGoals();

	// goal pipe notifications - used by action graph nodes
	typedef std::multimap< int, std::pair< IGoalPipeListener*, const char* > > TMapGoalPipeListeners;
	TMapGoalPipeListeners m_mapGoalPipeListeners;
	void NotifyListeners( int goalPipeId, EGoalPipeEvent event );
	void NotifyListeners( CGoalPipe* pPipe, EGoalPipeEvent event, bool includeSubPipes=false );
	virtual void RegisterGoalPipeListener( IGoalPipeListener* pListener, int goalPipeId, const char* debugClassName );
	virtual void UnRegisterGoalPipeListener( IGoalPipeListener* pListener, int goalPipeId );

	int		CountGroupedActiveGoals();	// 
	void	ClearGroupedActiveGoals();	//

	EAimState	GetAimState() const;

	void SetNavSOFailureStates();

	enum ESpecialAIObjects
	{
		AISPECIAL_HIDEPOINT_LASTOP,
		AISPECIAL_LAST_HIDEOBJECT,
		AISPECIAL_PROBTARGET,
		AISPECIAL_PROBTARGET_IN_TERRITORY,
		AISPECIAL_PROBTARGET_IN_REFSHAPE,
		AISPECIAL_PROBTARGET_IN_TERRITORY_AND_REFSHAPE,
		AISPECIAL_ATTTARGET_IN_TERRITORY,
		AISPECIAL_ATTTARGET_IN_REFSHAPE,
		AISPECIAL_ATTTARGET_IN_TERRITORY_AND_REFSHAPE,
		AISPECIAL_ANIM_TARGET,
		AISPECIAL_GROUP_TAC_POS,
		AISPECIAL_GROUP_TAC_LOOK,
		AISPECIAL_VEHICLE_AVOID_POS,
		COUNT_AISPECIAL	// Must be last
	};

	CAIObject* GetOrCreateSpecialAIObject(ESpecialAIObjects type);


protected:
	bool Serialize(TSerialize ser, class CObjectTracker& objectTracker);	
	void ClearActiveGoals();
	bool ProcessBranchGoal(QGoal &Goal, bool &bSkipAdd);
	bool ProcessRandomGoal(QGoal &Goal, bool &bSkipAdd);
	bool ProcessClearGoal(QGoal &Goal, bool &bSkipAdd);
	bool GetBranchCondition(QGoal &Goal);

	virtual IPathFollower* CreatePathFollower(const PathFollowerParams &params);

	CWeakRef<CAIObject> m_refAttentionTarget;
	float	m_AttTargetPersistenceTimeout;	// to make sure not to jitter between targets; once target is selected, make it stay for some time 
	EAITargetThreat m_AttTargetThreat;
	EAITargetThreat m_AttTargetExposureThreat;
	EAITargetType m_AttTargetType;

	bool m_bInCover;
	bool m_bMovingToCover;

	EFireMode	m_fireMode;							// Currently set firemode.
	CWeakRef<CAIObject> m_refFireTarget;
	bool			m_fireModeUpdated;			// Flag indicating if the fire mode has been just updated.
	bool			m_outOfAmmoSent;				// Flag indicating if OnOutOfAmmo signal was sent for current fireMode session.
	bool			m_wasReloading;
	VectorOGoals	m_vActiveGoals;
	bool				m_bBlocked;
	bool				m_bStartTiming;
	float				m_fEngageTime;
	CGoalPipe		*m_pCurrentGoalPipe;
	VectorSet< int > m_notAllowedSubpipes; // sub-pipes that we tried to remove or cancel even before inserting them
	bool				m_bFirstUpdate;
	int				m_looseAttentionId;
	IAISystem::ENavigationType m_CurrentNodeNavType;
	EAimState		m_aimState;
	float			m_spreadFireTime;
	Vec3			m_vBodyTargetDir;

  /// Name of the predefined path that we may subsequently be asked to follow
  string m_pathToFollowName;
  bool m_bPathToFollowIsSpline;

	string				m_refShapeName;
	SShape*				m_refShape;
	Vec3					m_vLastSOExitHelper;

	CStrongRef<CAIObject>	m_refSpecialObjects[COUNT_AISPECIAL];

	typedef std::pair<float, Vec3>	FloatVecPair;
	typedef std::list<FloatVecPair>	TimeOutVec3List;
	TimeOutVec3List m_recentUnreachableHideObjects;

	Vec3	m_lastLiveTargetPos;
	float	m_timeSinceLastLiveTarget;

  /// path follower is only set if we're using that class to follow a path
  class IPathFollower *m_pPathFollower;

	SAIActorTargetRequest*	m_pActorTargetRequest;

private:
	Vec3 SetPointListToFollowSub( Vec3 a, Vec3 b, Vec3 c, std::list<Vec3>& newPointList, float step );

	// Just here to avoid a warning and to make sure the correct version is used
	void SetAttentionTarget(IAIObject *pObject) {assert(false);}

	CStrongRef<CAIObject>		m_refRefPoint;					// a tag point the puppet uses to remember a position
	CStrongRef<CAIObject> m_refLookAtTarget;				// a look point - which the loose attention target may be set to!

	uint8 m_paused;


	void CoverUsageInfoRayComplete(const QueuedRayID& rayID, const RayCastResult& result);

	struct CoverUsageInfoState
	{
		CoverUsageInfoState()
			: state(AsyncReady)
			, rayCount()
			, result(false)
		{
			for (uint32 i = 0; i < 6; ++i)
				rayID[i] = 0;
		}

		void Reset()
		{
			for (uint32 i = 0; i < 6; ++i)
			{
				if (rayID[i])
				{
					gAIEnv.pRayCaster->Cancel(rayID[i]);
					rayID[i] = 0;
				}
			}

			state = AsyncReady;
			rayCount = 0;
			result = CoverUsageInfo(false);
		}

		QueuedRayID rayID[6];
		AsyncState state;
		uint8 rayCount;

		CoverUsageInfo result;
	};

	CoverUsageInfoState m_coverUsageInfoState;
};

inline const CPipeUser* CastToCPipeUserSafe(const IAIObject* pAI) { return pAI ? pAI->CastToCPipeUser() : 0; }
inline CPipeUser* CastToCPipeUserSafe(IAIObject* pAI) { return pAI ? pAI->CastToCPipeUser() : 0; }

#endif
