// GoalOp.h: interface for the CGoalOp class.
//////////////////////////////////////////////////////////////////////

#ifndef __GOALOP_H_
#define __GOALOP_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "AIHideObject.h"
#include "AIPIDController.h"
#include "ITacticalPointSystem.h"
#include "TimeValue.h"

#include <list>

struct IAIObject;
class  CAIObject;
class  CPipeUser;
class CPuppet;
struct GraphNode;

class COPPathFind;
class COPTrace;


enum ECoverUsageLocation
{
	eCUL_None,
	eCUL_Automatic,
	eCUL_Left,
	eCUL_Right,
	eCUL_Center,
};


class CGoalOpXMLReader
{
	template <typename T>
	class CXMLAttrReader
	{
		typedef std::pair<const char*, T> TRecord;
		std::vector<TRecord> m_dictionary;

	public:
		void Add(const char* szKey, T tValue) { m_dictionary.push_back(TRecord(szKey, tValue)); }
		
		bool Get(const XmlNodeRef& node, const char* szAttrName, T& tValue, bool bMandatory = false)
		{
			const char* szKey = node->getAttr(szAttrName);
			for (typename std::vector<TRecord>::iterator it = m_dictionary.begin(), end = m_dictionary.end(); it != end; ++it)
			{
				if (!stricmp(it->first, szKey))
				{
					tValue = it->second;
					return true;
				}
			}

			if (bMandatory)
			{
				AIError("Unable to get mandatory attribute '%s' of node '%s'.",
					szAttrName, node->getTag());
			}

			return false;
		}
	};

	CXMLAttrReader<EAnimationMode>  m_dictAnimationMode;
	CXMLAttrReader<bool>            m_dictBools;
	CXMLAttrReader<ECoverUsageLocation> m_dictCoverLocation;
	CXMLAttrReader<EFireMode>       m_dictFireMode;
	CXMLAttrReader<ELookMotivation> m_dictLook;
	CXMLAttrReader<EAIRegister>     m_dictRegister;
	CXMLAttrReader<ESignalFilter>   m_dictSignalFilter;
	CXMLAttrReader<EStance>         m_dictStance;
	CXMLAttrReader<float>           m_dictUrgency;

public:
	CGoalOpXMLReader();

	template <typename T>
	bool GetMandatory(const XmlNodeRef& node, const char* szAttrName, T& value)
	{
		if (node->getAttr(szAttrName, value))
		{
			return true;
		}

		AIError("Unable to get mandatory attribute '%s' of node '%s'.",
			szAttrName, node->getTag());

		return false;
	}

	bool GetAnimationMode(const XmlNodeRef& node, const char* szAttrName, EAnimationMode&  eValue, bool bMandatory = false)
		{ return m_dictAnimationMode.Get(node, szAttrName, eValue, bMandatory); }
	bool GetCoverLocation(const XmlNodeRef& node, const char* szAttrName, ECoverUsageLocation& eValue, bool bMandatory = false)
		{ return m_dictCoverLocation.Get(node, szAttrName, eValue, bMandatory); }
	bool GetFireMode     (const XmlNodeRef& node, const char* szAttrName, EFireMode&       eValue, bool bMandatory = false)
		{ return m_dictFireMode.Get(node, szAttrName, eValue, bMandatory); }
	bool GetLook         (const XmlNodeRef& node, const char* szAttrName, ELookMotivation& eValue, bool bMandatory = false)
		{ return m_dictLook.Get(node, szAttrName, eValue, bMandatory); }
	bool GetRegister     (const XmlNodeRef& node, const char* szAttrName, EAIRegister&     eValue, bool bMandatory = false)
		{ return m_dictRegister.Get(node, szAttrName, eValue, bMandatory); }
	bool GetSignalFilter (const XmlNodeRef& node, const char* szAttrName, ESignalFilter&   eValue, bool bMandatory = false)
		{ return m_dictSignalFilter.Get(node, szAttrName, eValue, bMandatory); }
	bool GetStance       (const XmlNodeRef& node, const char* szAttrName, EStance&         eValue, bool bMandatory = false)
		{ return m_dictStance.Get(node, szAttrName, eValue, bMandatory); }
	bool GetUrgency      (const XmlNodeRef& node, const char* szAttrName, float&           fValue, bool bMandatory = false)
	{ return m_dictUrgency.Get(node, szAttrName, fValue, bMandatory); }

	bool GetBool(const XmlNodeRef& node, const char* szAttrName, bool bDefaultValue = false);
	bool GetMandatoryBool(const XmlNodeRef& node, const char* szAttrName);

	const char* GetMandatoryString(const XmlNodeRef& node, const char* szAttrName);

	enum { MANDATORY = 1 };
};

// Return value type of the IGoalOp::Execute
enum EGoalOpResult
{
	eGOR_NONE,         // Default value; should not be returned
	eGOR_IN_PROGRESS,  // Keep on executing
	eGOR_SUCCEEDED,    // The goalop succeeded
	eGOR_FAILED,       // The goalop failed
	eGOR_DONE,         // The goalop is finished (neutral, do not change result state)
	eGOR_LAST
};


struct IGoalOp
{
	virtual ~IGoalOp() {}
	
	virtual void          DebugDraw (CPipeUser* pPipeUser) const = 0;
	virtual EGoalOpResult Execute   (CPipeUser* pPipeUser) = 0;
	/// Stripped-down execute - should only be implemented if this goalop needs
	/// to be really responsive (gets called every frame - i.e. on the dry update)
	virtual void          ExecuteDry(CPipeUser* pPipeUser) = 0;
	virtual bool          IsBlocking() { return true; }
	virtual void          Reset     (CPipeUser* pPipeUser) = 0;
	virtual void          Serialize (TSerialize ser, CObjectTracker& objectTracker) = 0;
};


// (MATT) CGoalOp must be a smart pointer for it's use in containers of QGoals {2009/10/13}
class CGoalOp : public IGoalOp, public _i_reference_target_t
{
public:
	virtual ~CGoalOp() {}

	virtual void          DebugDraw (CPipeUser* pPipeUser) const {}
	virtual EGoalOpResult Execute   (CPipeUser* pPipeUser) { return eGOR_DONE; }
	virtual void          ExecuteDry(CPipeUser* pPipeUser) {}
	virtual void          Reset     (CPipeUser* pPipeUser) {}
	virtual void          Serialize (TSerialize ser, CObjectTracker& objectTracker) {}
	
protected:
	static CGoalOpXMLReader s_xml;
};


////////////////////////////////////////////////////////////
//
//				ACQUIRE TARGET - acquires desired target, even if it is outside of view
//
////////////////////////////////////////////////////////
class COPAcqTarget: public CGoalOp
{
public:
	// Empty or invalid sTargetName means "use the LastOp"
	COPAcqTarget(const string& sTargetName) : m_sTargetName(sTargetName) {}
	COPAcqTarget(const XmlNodeRef& node) : m_sTargetName(s_xml.GetMandatoryString(node, "name")) {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);

private:
	string m_sTargetName;
};


////////////////////////////////////////////////////////////
//
//				APPROACH - makes agent approach to "distance" from current att target
//
////////////////////////////////////////////////////////
class COPApproach: public CGoalOp
{
public:
	COPApproach(float fEndDistance, float fEndAccuracy=1.f, float fDuration = 0.0f,
		bool bUseLastOpResult = false, bool bLookAtLastOp = false,
		bool bForceReturnPartialPath = false, bool bStopOnAnimationStart = false, const char* szNoPathSignalText = 0);
	COPApproach(const XmlNodeRef& node);
	~COPApproach();

	virtual void          DebugDraw (CPipeUser* pPipeUser) const;
	virtual EGoalOpResult Execute   (CPipeUser* pPipeUser);
	virtual void          ExecuteDry(CPipeUser* pPipeUser);
	virtual void          Reset     (CPipeUser* pPipeUser);
	virtual void          Serialize (TSerialize ser, CObjectTracker& objectTracker);

private:
	float m_fLastDistance;
	float m_fInitialDistance;
	bool	m_bUseLastOpResult;
	bool	m_bLookAtLastOp;
  bool	m_stopOnAnimationStart;
	float m_fEndDistance;/// Stop short of the path end
	float m_fEndAccuracy;/// allow some error in the stopping position
  float m_fDuration; /// Stop after this time (0 means disabled)
	bool	m_bForceReturnPartialPath;
	string	m_noPathSignalText;
	int		m_looseAttentionId;

	float GetEndDistance(CPipeUser* pPipeUser) const;

  // set to true when the path is first found
  bool m_bPathFound;
	
	COPTrace		*m_pTraceDirective;
	COPPathFind		*m_pPathfindDirective;
};


////////////////////////////////////////////////////////////
//
// FOLLOWPATH - makes agent follow the predefined path stored in the operand
//
////////////////////////////////////////////////////////

class COPFollowPath: public CGoalOp
{
public:
  /// pathFindToStart: whether the operand should path-find to the start of the path, or just go there in a straight line
  /// reverse: whether the operand should navigate the path from the first point to the last, or in reverse
  /// startNearest: whether the operand should start from the first point on the path, or from the nearest point on the path to his current location
  /// loops: how many times to traverse the path (goes round in a loop)
	COPFollowPath(bool pathFindToStart, bool reverse, bool startNearest, int loops, float fEndAccuracy, bool bUsePointList);
	COPFollowPath(const XmlNodeRef& node);
	~COPFollowPath();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
private:

	COPTrace		*m_pTraceDirective;
  COPPathFind *m_pPathFindDirective;
  CStrongRef<CAIObject> m_refPathStartPoint;
  bool	m_pathFindToStart;
	bool	m_reversePath;
	bool	m_startNearest;
	bool	m_bUsePointList;
	int		m_loops;
	int		m_loopCounter;
	float	m_notMovingTime;
	CTimeValue	m_lastTime;
	bool	m_returningToPath;
	float m_fEndAccuracy;

};

////////////////////////////////////////////////////////////
//
//				BACKOFF - makes agent back off to "distance" from current att target
//
////////////////////////////////////////////////////////
class COPBackoff: public CGoalOp
{
public:
  /// If distance < 0 then the backoff is done from the agent's current position,
  /// else it is done from the target position.
  /// If duration > 0 then backoff stops after that duration
	COPBackoff(float distance, float duration = 0.f, int filter=0, float minDistance=0.f);
	COPBackoff(const XmlNodeRef& node);
	~COPBackoff();

	void Reset(CPipeUser* pPipeUser);
	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void DebugDraw(CPipeUser* pPipeUser) const;
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
private:
	void ResetNavigation(CPipeUser* pPipeUser);
	void ResetMoveDirections();

private:
	bool m_bUseTargetPosition;	// distance from target or from own current position 
	int	m_iDirection;
	int m_iCurrentDirectionIndex;
	std::vector<int> m_MoveDirections;

	float m_fDistance;
	float m_fDuration;
	CTimeValue m_fInitTime;
	CTimeValue m_fLastUpdateTime;
	bool	m_bUseLastOp;
	bool	m_bLookForward;
	Vec3	m_moveStart;
	Vec3	m_moveEnd;
	int		m_currentDirMask;
	float m_fMinDistance;
	Vec3	m_vBestFailedBackoffPos;
	float m_fMaxDistanceFound;
	bool	m_bTryingLessThanMinDistance;
	int		m_looseAttentionId;
	bool	m_bRandomOrder;
	bool	m_bCheckSlopeDistance;

	COPTrace		*m_pTraceDirective;
	COPPathFind		*m_pPathfindDirective;
  CStrongRef<CAIObject> m_refBackoffPoint;
};

////////////////////////////////////////////////////////////
//
//				TIMEOUT - counts down a timer...
//
////////////////////////////////////////////////////////
class COPTimeout: public CGoalOp
{

	CTimeValue	m_startTime;
	float				m_fIntervalMin, m_fIntervalMax;
	float				m_fCurrentInterval;

public:
	COPTimeout(float intervalMin, float intervalMax = 0);
	COPTimeout(const XmlNodeRef& node);
	~COPTimeout();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker)
	{
		// Luciano - TO DO: serialize it properly
		m_startTime.SetSeconds(0.0f);
	}
};

////////////////////////////////////////////////////////////
//
//				STRAFE makes agent strafe left or right (1,-1)... 0 stops strafing
//
////////////////////////////////////////////////////////
class COPStrafe: public CGoalOp
{
	
public:
	COPStrafe(float distanceStart, float distanceEnd, bool strafeWhileMoving);
	COPStrafe(const XmlNodeRef& node);
	~COPStrafe();

	float m_fDistanceStart;
	float m_fDistanceEnd;
	bool	m_bStrafeWhileMoving;

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//				FIRECMD - 1 allowes agent to fire, 0 forbids agent to fire
//
////////////////////////////////////////////////////////
class COPFireCmd: public CGoalOp
{
public:
	COPFireCmd(int firemode, bool useLastOpResult, float intervalMin, float intervalMax);
	COPFireCmd(const XmlNodeRef& node);
	~COPFireCmd();

	EFireMode	m_command;
	bool	m_bUseLastOpResult;

	void Reset(CPipeUser* pPipeUser);
	EGoalOpResult Execute(CPipeUser* pPipeUser);

protected:

	float m_fIntervalMin;
	float m_fIntervalMax;
	float m_fCurrentInterval;
	CTimeValue m_startTime;
};

////////////////////////////////////////////////////////////
//
//				BODYCMD - controls agents stance (0- stand, 1-crouch, 2-prone)
//
////////////////////////////////////////////////////////
class COPBodyCmd: public CGoalOp
{
public:
	COPBodyCmd(EStance bodystate, bool delayed=false);
	COPBodyCmd(const XmlNodeRef& node);
	~COPBodyCmd(){}

	EStance m_nBodyState;
	bool m_bDelayed;	// this stance has to be set at the end of next/current trace

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//				RUNCMD - makes the agent run if 1, walk if 0
//
////////////////////////////////////////////////////////
class COPRunCmd: public CGoalOp
{
	float ConvertUrgency(float speed);

	float m_fMaxUrgency;
	float m_fMinUrgency;
	float m_fScaleDownPathLength;

public:
	COPRunCmd(float maxUrgency, float minUrgency, float scaleDownPathLength);
	COPRunCmd(const XmlNodeRef& node);
	~COPRunCmd() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};


////////////////////////////////////////////////////////////
//
//				LOOKAROUND - looks in a random direction
//
////////////////////////////////////////////////////////
class COPLookAround : public CGoalOp
{
	float m_fLastDot;
	float m_fLookAroundRange;
	float	m_fIntervalMin, m_fIntervalMax;
	float	m_fTimeOut;
	float	m_fScanIntervalRange;
	float	m_fScanTimeOut;
	CTimeValue m_startTime;
	CTimeValue m_scanStartTime;
	ELookStyle m_eLookStyle;
	bool	m_breakOnLiveTarget;
	bool	m_useLastOp;
	float	m_lookAngle;
	float	m_lookZOffset;
	float	m_lastLookAngle;
	float	m_lastLookZOffset;
	int		m_looseAttentionId;
	bool	m_bInitialized;
	Vec3	m_initialDir;
	bool	m_checkForObstacles;
	void	UpdateLookAtTarget(CPipeUser* pPipeUser);
	Vec3	GetLookAtDir(CPipeUser* pPipeUser, float angle, float dz) const;
	void	SetRandomLookDir();

public:
	void Reset(CPipeUser* pPipeUser);
	COPLookAround(float lookAtRange, float scanIntervalRange, float intervalMin, float intervalMax, bool bBodyTurn, bool breakOnLiveTarget, bool useLastOp, bool checkForObstacles);
	COPLookAround(const XmlNodeRef& node);
	~COPLookAround();

	virtual EGoalOpResult Execute(CPipeUser* pPipeUser);
	virtual void ExecuteDry(CPipeUser* pPipeUser);
	virtual void DebugDraw(CPipeUser* pPipeUser) const;
	virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
};


////////////////////////////////////////////////////////////
//
//				PATHFIND - generates a path to desired aiobject
//
////////////////////////////////////////////////////////
class COPPathFind : public CGoalOp
{
	CWeakRef<CAIObject> m_refTarget;
	string m_sObjectName;
	bool m_bKeepMoving;
  float m_fDirectionOnlyDistance;
	float m_fEndTol;
	float m_fEndDistance;
	Vec3 m_TargetPos;
  Vec3 m_TargetOffset;
	// if >= 0 forces the path destination to be within a particular building
	int  m_nForceTargetBuildingID;
public:

	bool m_bWaitingForResult;

  /// fDirectionOnlyDistance > 0 means that we should attempt to pathfind that distance in the direction
  /// between the supposed path start/end points
	COPPathFind(const char *szName, IAIObject *pTarget = 0, float fEndTol = 0.0f, float fEndDistance = 0.0f, bool bKeepMoving=false, float fDirectionOnlyDistance = 0.0f);
	COPPathFind(const XmlNodeRef& node);
	~COPPathFind();

	void Reset(CPipeUser* pPipeUser);
	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
	void SetForceTargetBuildingId(int id) {m_nForceTargetBuildingID = id;}
  void SetTargetOffset(const Vec3 &offset) {m_TargetOffset = offset;}
};


////////////////////////////////////////////////////////////
//
//				LOCATE - locates an aiobject in the map
//
////////////////////////////////////////////////////////
class COPLocate : public CGoalOp
{
	string m_sName;
	unsigned int m_nObjectType;
	float m_fRange;
public:
	COPLocate(const char *szName, unsigned int ot = 0, float range = 0) : m_fRange(range) { if (szName) m_sName = szName; m_nObjectType = ot;}
	COPLocate(const XmlNodeRef& node);
	~COPLocate() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//				TRACE - makes agent follow generated path... does nothing if no path generated
//
////////////////////////////////////////////////////////
class COPTrace : public CGoalOp
{
private:
  bool  m_bDisabledPendingFullUpdate; // set when we we should follow the path no more
  float	m_ManeuverDist;
  CTimeValue m_ManeuverTime;
  /// For things like helicopters that land then this gets set to some offset used at the end of the
  /// path. Then at the path end the descent is gradually controlled - path only finishes when the
  /// agent touches ground
  float m_landHeight;
  /// Specifies how high we should be above the path at the current point
  float m_workingLandingHeightOffset;
  // The ground position and facing direction
  Vec3 m_landingPos;
  Vec3 m_landingDir;

  bool m_bExactFollow;
	bool m_bForceReturnPartialPath;
  Vec3	m_lastPosition;
  CTimeValue m_lastTime;
  float	m_TimeStep;
  CWeakRef<CPipeUser> m_refOperand;
	int		m_looseAttentionId;
  /// keep track of how long we've been tracing a path for - however, this is more of a time
  /// since the last "event" - it can get reset when we regenerate the path etc
  float m_fTotalTracingTime;
  bool m_inhibitPathRegen;
  bool	m_waitingForPathResult;
  bool	m_earlyPathRegen;
	bool	m_bSteerAroundPathTarget; 
	bool	m_bControllSpeed;	// indicates whether this goalop will control/modify PIpeUser's speed

  /// How far we've moved since the very start
  float m_fTravelDist;
  /// what time we started getting executed
  CTimeValue m_startTime;

	enum ETraceActorTgtRequest
	{
		eTATR_None,			// no request
		eTATR_NavSO,			// navigational smart object
		eTATR_EndOfPath,	// end of path exact positioning
	};

	/// flag indicating the the trace requested exact positioning.
	ETraceActorTgtRequest	m_actorTargetRequester;
	ETraceActorTgtRequest	m_pendingActorTargetRequester;
  CStrongRef<CAIObject> m_refNavTarget;
  bool m_stopOnAnimationStart;

public:
  enum EManeuver{eMV_None, eMV_Back, eMV_Fwd};
  enum EManeuverDir {eMVD_Clockwise, eMVD_AntiClockwise};
  enum EEndMode {eEM_FixedDistance, eEM_MinimumDistance};
  EManeuver	m_Maneuver;
  EManeuverDir m_ManeuverDir;
  EEndMode m_EndMode;

  /// Aim to stay/stop this far from the target.
//	float	m_fEndDistance;
  /// we aim to stop within m_fAccuracy of the target minus m_fStickDistance (normally do better than this)
  float m_fEndAccuracy;
  /// finish tracing after this duration
//  float m_fDuration;

	bool	m_passingStraightNavSO;

  void Reset(CPipeUser* pPipeUser);
  /// Don't know what bExactFollow means!
  /// fEndDistance is the distance to stop before the end (handles smart objects etc)
  /// fEndAccuracy is the allowed stopping error at the end
  /// fDuration is used to make us stop gracefully after a certain time
  /// bForceReturnPartialPath is used when we regen the path internally
  /// bStopOnAnimationStart would make trace finish once the exact positioning animation at the end of path is started
  COPTrace( bool bExactFollow, float fEndAccuracy = 1.0f,
		bool bForceReturnPartialPath = false, bool bStopOnAnimationStart = false, EEndMode endMode = eEM_FixedDistance);
  COPTrace(const XmlNodeRef& node);
  ~COPTrace();

	inline void SetSteerAroundPathTarget(bool steer) {m_bSteerAroundPathTarget = steer;}
	inline void SetControlSpeed(bool bValue) {m_bControllSpeed = bValue;}

  void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
  EGoalOpResult Execute(CPipeUser* pPipeUser);
  bool ExecuteTrace(CPipeUser* pPipeUser, bool fullUpdate);
	bool IsPathRegenerationInhibited() const { return m_inhibitPathRegen || m_passingStraightNavSO; }
	void DebugDraw(CPipeUser* pPipeUser) const;
protected:
  void CreateDummyFromNode(const Vec3 &pos, const char* ownerName);
  bool Execute2D(CPipeUser* pPipeUser, bool fullUpdate);
  bool Execute3D(CPipeUser* pPipeUser, bool fullUpdate);
  bool ExecutePathFollower(CPipeUser* pPipeUser, bool fullUpdate);
  void ExecuteManeuver(CPipeUser* pPipeUser, const Vec3& pathDir);
  bool ExecutePreamble(CPipeUser* pPipeUser);
  bool ExecutePostamble(CPipeUser* pPipeUser, bool &reachedEnd, bool fullUpdate, bool twoD);
  /// returns true when landing is completed
  bool ExecuteLanding(CPipeUser* pPipeUser, const Vec3 &pathEnd);
};

////////////////////////////////////////////////////////////
//
//				IGNOREALL - 1, puppet does not reevaluate threats, 0 evaluates again
//
////////////////////////////////////////////////////////
class COPIgnoreAll : public CGoalOp
{
	bool	m_bParam;
public:
	COPIgnoreAll(bool param) { m_bParam = param; }
	COPIgnoreAll(const XmlNodeRef& node) { m_bParam = s_xml.GetBool(node, "id", true); }
	~COPIgnoreAll() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};


////////////////////////////////////////////////////////////
//
//				SIGNAL - send a signal to himself or other agents
//
////////////////////////////////////////////////////////
class COPSignal : public CGoalOp
{
	int m_nSignalID;
	string m_sSignal;
	ESignalFilter m_cFilter;
	CAIObject *m_pTarget;
	bool m_bSent;
	int	m_iDataValue;
public:
	COPSignal(int param, const string& sSignal, ESignalFilter cFilter, int data) : m_bSent(false), m_nSignalID(param), m_pTarget(0), m_cFilter(cFilter), m_sSignal(sSignal), m_iDataValue(data) {};
	COPSignal(const XmlNodeRef& node);
	~COPSignal() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//				SCRIPT - execute a piece of script
//
////////////////////////////////////////////////////////
class COPScript : public CGoalOp
{
	SmartScriptFunction m_scriptCode;
	ScriptAnyValue m_userData;
public:
	COPScript(const SmartScriptFunction& scriptCode, const ScriptAnyValue& userData);
	COPScript(const XmlNodeRef& node);
	virtual ~COPScript();


	EGoalOpResult Execute(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//				DEVALUE - devalues current attention target 
//
////////////////////////////////////////////////////////
class COPDeValue : public CGoalOp
{
	bool m_bDevaluePuppets;
	bool m_bClearDevalued;
public:
	COPDeValue(int nPuppetsAlso = 0,bool bClearDevalued = false) { m_bDevaluePuppets = (nPuppetsAlso!=0);m_bClearDevalued =bClearDevalued; }
	COPDeValue(const XmlNodeRef& goalOpNode);
	~COPDeValue() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//			HIDE - makes agent find closest hiding place and then hide there
//
////////////////////////////////////////////////////////
class COPHide : public CGoalOp
{
	CStrongRef<CAIObject> m_refHideTarget;
	Vec3 m_vHidePos;
	Vec3 m_vLastPos;
	float m_fSearchDistance;
	float m_fMinDistance;
	int	  m_nEvaluationMethod;
	COPPathFind *m_pPathFindDirective;
	COPTrace *m_pTraceDirective;
	bool m_bLookAtHide;
	bool m_bLookAtLastOp;
	bool m_bAttTarget;
	bool m_bEndEffect;
	int m_iActionId;
	int m_looseAttentionId;

public:
	COPHide(float distance, float minDistance, int method, bool bExact, bool bLookatLastOp)
	{
		m_fSearchDistance=distance; m_nEvaluationMethod = method;
		m_fMinDistance = minDistance;
		m_pPathFindDirective = 0;
		m_pTraceDirective = 0;
		m_bLookAtHide = bExact;
		m_bLookAtLastOp = bLookatLastOp;
		m_iActionId = 0;
		m_looseAttentionId = 0;
		m_vHidePos.Set(0,0,0);
		m_vLastPos.Set(0,0,0);
		m_bAttTarget = false;
		m_bEndEffect = false;
	}

	~COPHide() {}
	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
private:
	void CreateHideTarget(string sName, const Vec3 &vPos);
	bool IsBadHiding(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//			TacticalPos - makes agent find a tactical position (and go there)
//
///////////////////////////////////////////////////////////

class COPTacticalPos : public CGoalOp
{
	// Evaluation, pathfinding and moving to the point is regulated by a state machine
	enum eTacticalPosState
	{
		eTPS_INVALID,
		eTPS_QUERY_INIT,
		eTPS_QUERY,
		eTPS_PATHFIND_INIT,
		eTPS_PATHFIND,
		eTPS_TRACE_INIT,
		eTPS_TRACE,
	};

	int	m_iOptionUsed;                                       // Stored so that later signals (say, on reaching point) can give the option number
	CStrongRef<CAIObject> m_refHideTarget;
	Vec3 m_vHidePos;
	Vec3 m_vLastPos;
	COPPathFind *m_pPathFindDirective;
	COPTrace *m_pTraceDirective;
	bool m_bLookAtHide;
	EAIRegister m_nReg;
	eTacticalPosState m_state;
	CTacticalPointQueryInstance m_queryInstance;             // Help that managed the async queries
	Vec3 m_vLastHidePos;

public:
	COPTacticalPos( int tacQueryID, EAIRegister nReg ) 
		: m_nReg(nReg)
		, m_state(eTPS_QUERY_INIT)
		, m_pPathFindDirective(0)
		, m_pTraceDirective(0)
		, m_iOptionUsed(-1)
		, m_bLookAtHide(false)
	{
		Reset(NULL);
		m_queryInstance.SetQueryID(tacQueryID);
	}

	COPTacticalPos(const XmlNodeRef& node);

  ~COPTacticalPos();
	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	bool IsBadHiding(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

protected:
	enum { eTPGOpState_NoPointFound, eTPGOpState_PointFound, eTPGOpState_DestinationReached };
	void SendStateSignal(CPipeUser* pPipeUser, int nState );
};

////////////////////////////////////////////////////////////
//
//			Look - make an agent look somewhere (operates on looktarget)
//
///////////////////////////////////////////////////////////

class COPLook : public CGoalOp
{
	ELookStyle m_eLookThere;
	ELookStyle m_eLookBack;
	int m_nReg;
	int m_nLookID;	// keep track of this look command
	bool m_bInitialised;
	float m_fLookTime, m_fTimeLeft;

public:
	COPLook( int lookMode, bool bBodyTurn, int nReg );
	COPLook(const XmlNodeRef& node);
	~COPLook();
	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
};

////////////////////////////////////////////////////////////
//
//			USECOVER - hide/unhide at current hide spot
//
////////////////////////////////////////////////////////
class COPUseCover : public CGoalOp
{

public:
	COPUseCover(bool unHideNow, bool useLastOpAsBackup, float intervalMin, float intervalMax, bool speedUpTimeOutWhenNoTarget);
	~COPUseCover() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);

	void Reset(CPipeUser* pPipeUser);
	void DebugDraw(CPipeUser* pPipeUser) const;
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
protected:

	EGoalOpResult UseCoverSO(CPipeUser* pPipeUser);
	EGoalOpResult UseCoverEnv(CPipeUser* pPipeUser);
	void ChooseCoverUsage(CPipeUser* pPipeUser);
	void RandomizeTimeOut(CPipeUser* pPipeUser);
	void UpdateInvalidSeg(CPipeUser* pPipeUser);

	enum EUncoverState
	{
		UCS_NONE,
		UCS_HOLD,
		UCS_MOVE,
	};

	EUncoverState	m_ucs;
	EUncoverState	m_ucsLast;
	//	float					m_ucsTime;
	int						m_ucsSide;
	float					m_ucsWaitTime;
	bool					m_ucsMoveIn;
	CTimeValue		m_ucsStartTime;

	bool				m_speedUpTimeOutWhenNoTarget;
	bool				m_targetReached;
	bool				m_coverCompromised;
	Vec3				m_hidePos;
	Vec3				m_peekPosRight;
	Vec3				m_peekPosLeft;
	Vec3				m_pose;
	Vec3				m_moveTarget;
	ECoverUsage m_curCoverUsage;
	float				m_weaponOffset;
	float				m_intervalMin;
	float				m_intervalMax;
	float				m_fTimeOut;
	CTimeValue	m_startTime;
	bool				m_bUnHide;
	bool				m_useLastOpAsBackup;

	//	float				m_leftInvalidTime;
	CTimeValue	m_leftInvalidStartTime;
	float				m_leftInvalidDist;
	Vec3				m_leftInvalidPos;
	//	float				m_rightInvalidTime;
	CTimeValue	m_rightInvalidStartTime;
	float				m_rightInvalidDist;
	Vec3				m_rightInvalidPos;

	bool				m_leftEdgeCheck;
	bool				m_rightEdgeCheck;

	float				m_maxPathLen;

	EStance			m_moveStance;

	bool				m_DEBUG_checkValid;
	Vec3				m_DEGUG_checkPos[6];
	Vec3				m_DEGUG_checkDir[6];
	bool				m_DEBUG_checkRes[6];
};


////////////////////////////////////////////////////////////
//
//			STICK - the agent keeps at a constant distance to his target
//
// regenerate path if target moved for more than m_fTrhDistance
////////////////////////////////////////////////////////
class COPStick : public CGoalOp
{
public:
//	COPStick(float stickDistance, float accuracy, float duration, bool bUseLastOp, bool bLookatLastOp, 
//    bool bContinuous, bool bTryShortcutNavigation, bool bForceReturnPartialPath=false, bool bStopOnAnimationStart=false, bool bConstantSpeed=false);
		COPStick(float stickDistance, float accuracy, float duration, int flags, int flagsAux, COPTrace::EEndMode endMode = COPTrace::eEM_FixedDistance);
	COPStick(const XmlNodeRef& node);
	~COPStick();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

private:
  virtual void DebugDraw(CPipeUser* pPipeUser) const;
  /// Keep a list of "safe" points of the stick target. This will be likely to have a valid
  /// path between the point and the stick target - so when teleporting is necessary one
  /// of these points (probably the one furthest back and invisible) can be chosen
  void UpdateStickTargetSafePoints(CPipeUser* pPipeUser);
  /// Attempts to teleport, and if it's necessary/practical does it, and returns true.
  bool TryToTeleport(CPipeUser * pPipeUser);
  /// sets teleporting info to the default don't-need-to-teleport state
  void ClearTeleportData();
  void RegeneratePath(CPipeUser* pPipeUser, const Vec3 &destination);

	float GetEndDistance(CPipeUser* pPipeUser) const;

  /// Stores when/where the stick target was
  struct SSafePoint
  {
    SSafePoint(const Vec3 &pos = ZERO, unsigned nodeIndex = 0, bool safe = false, const CTimeValue &time = CTimeValue()) : 
			pos(pos), nodeIndex(nodeIndex), safe(safe), time(time) {}
		void Serialize(TSerialize ser);
    Vec3 pos;
    unsigned nodeIndex;
    bool safe; // used to store _unsafe_ locations too
    CTimeValue time;
    // if stored in a container, then we need to be told the object tracker
    static CObjectTracker *pObjectTracker;
  };
  /// Point closest to the stick target is at the front
  typedef MiniQueue<SSafePoint, 32> TStickTargetSafePoints;
  TStickTargetSafePoints m_stickTargetSafePoints;
  /// distance between safe points
  float m_safePointInterval;
  /// teleport current/destination locations
  Vec3 m_teleportCurrent, m_teleportEnd;
  /// time at which the old teleport position was checked
  CTimeValue m_lastTeleportTime;
  /// time at which the operand was last visible
  CTimeValue m_lastVisibleTime;

  // teleport params
  /// Only teleport if the resulting apparent speed would not exceed this.
  /// If 0 it disables teleporting
  float m_maxTeleportSpeed;
  // Only teleport if the operand's path (if it exists) is longer than this
  float m_pathLengthForTeleport;
  // Don't teleport closer than this to player
  float m_playerDistForTeleport;


  Vec3	m_vLastUsedTargetPos;
  float	m_fTrhDistance;
  CWeakRef<CAIObject> m_refStickTarget; 
  CWeakRef<CAIObject> m_refSightTarget;

  COPTrace::EEndMode m_EndMode;
  float m_fApproachTime;
  float m_fHijackDistance;

  /// Aim to stay/stop this far from the target
  float	m_fStickDistance;
  /// we aim to stop within m_fAccuracy of the target minus m_fStickDistance
  float m_fEndAccuracy;
  /// Stop after this time (0 means disabled)
  float m_fDuration;
  bool	m_bContinuous;		// stick OR just approach moving target
  bool	m_bTryShortcutNavigation;	//
  bool	m_bUseLastOpResult;
  bool	m_bLookAtLastOp;	// where to look at
  bool	m_bInitialized;
  bool	m_bForceReturnPartialPath;
  bool	m_stopOnAnimationStart;
  float m_targetPredictionTime;
	bool	m_bConstantSpeed;

  // used for estimating the target position movement
  CTimeValue m_lastTargetPosTime;
  Vec3 m_lastTargetPos;
  Vec3 m_smoothedTargetVel;

	int		m_looseAttentionId;

  // set to true when the path is first found
  bool m_bPathFound;
	bool m_bSteerAroundPathTarget;
  COPTrace		*m_pTraceDirective;
  COPPathFind		*m_pPathfindDirective;
};


////////////////////////////////////////////////////////////
//
//			FORM - this agent creates desired formation
//
////////////////////////////////////////////////////////
// (MATT) Appears probably useless. Consider removing {2009/02/27}
class COPForm : public CGoalOp
{
	string m_sName;
	
public:
	COPForm(const char*name) : m_sName(name) {}
	COPForm(const XmlNodeRef& node) : m_sName(s_xml.GetMandatoryString(node, "name")) {}
	~COPForm() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};


////////////////////////////////////////////////////////////
//
//			CLEAR - clears the actions for the operand puppet
//
////////////////////////////////////////////////////////
class COPClear : public CGoalOp
{
	string m_sName;
	
public:
	COPClear() {}
	~COPClear() {}

	EGoalOpResult Execute(CPipeUser* pPipeUser);
};


////////////////////////////////////////////////////////////
//
//			LOOKAT - look toward a specified direction
//
////////////////////////////////////////////////////////
class COPLookAt : public CGoalOp
{
	float m_fStartAngle;
	float m_fEndAngle;
	float m_fLastDot;
	ELookStyle m_eLookStyle;
	bool m_bUseLastOp;
	bool m_bContinuous;
	bool m_bInitialized;
	bool m_bUseBodyDir;
public:
	COPLookAt(float startangle, float endangle, int mode = 0, bool bBodyTurn = true, bool bUseLastOp = false); 
	COPLookAt(const XmlNodeRef& node);
	~COPLookAt();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void ResetLooseTarget(CPipeUser* pPipeUser, bool bForceReset = false);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
};


////////////////////////////////////////////////////////////
//
//				CONTINUOUS - continuous movement, keep on going in last movement direction. Not to stop while tracing path. 
//
////////////////////////////////////////////////////////
class COPContinuous: public CGoalOp
{
	bool	m_bKeepMoving;
public:
	COPContinuous(bool bKeepMoving);
	COPContinuous(const XmlNodeRef& goalOpNode);
	~COPContinuous();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
};

////////////////////////////////////////////////////////////
//
//				STEER - Makes the puppet try to reach a point and stay near it all the time using the "steering behavior engine"
//
////////////////////////////////////////////////////////
class COPSteer: public CGoalOp
{
public:
	COPSteer(float fSteerDistance = 0.0f, float fPathLenLimit = 0.0f);
	COPSteer(const XmlNodeRef& node);
	~COPSteer();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void DebugDraw(CPipeUser* pPipeUser) const;
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

private:
	void RegeneratePath(CPipeUser* pPipeUser, const Vec3 &destination);

	Vec3									m_vLastUsedTargetPos;
	CTimeValue						m_fLastRegenTime;

	float m_fPathLenLimit;
	float	m_fSteerDistanceSqr;
	float m_fLastDistance;
	float m_fMinEndDistance, m_fMaxEndDistance;
	float m_fEndAccuracy;
	bool m_bNeedHidespot;
	bool m_bFirstExec;

	COPTrace		*m_pTraceDirective;
	COPPathFind	*m_pPathfindDirective;
	CAIObject		*m_pPosDummy;
};

////////////////////////////////////////////////////////
//
//	waitsignal - waits for a signal and counts down a timer...
//
////////////////////////////////////////////////////////
class COPWaitSignal: public CGoalOp
{
	string m_sSignal;
	enum _edMode
	{
		edNone,
		edString,
		edInt,
		edId
	} m_edMode;
	string m_sObjectName;
	int m_iValue;
	EntityId m_nID;

	CTimeValue m_startTime;
	float m_fInterval;
	bool m_bSignalReceived;

public:
	COPWaitSignal(const XmlNodeRef& node);
	
	// (MATT) Note that it appears all but the first form could be removed {2008/08/09}
	COPWaitSignal( const char* sSignal, float fInterval = 0 );
	COPWaitSignal( const char* sSignal, const char* sObjectName, float fInterval = 0 );
	COPWaitSignal( const char* sSignal, int iValue, float fInterval = 0 );
	COPWaitSignal( const char* sSignal, EntityId nID, float fInterval = 0 );

	EGoalOpResult Execute( CPipeUser* pPipeUser );
	void Reset( CPipeUser* pPipeUser) ;
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

	bool NotifySignalReceived( CAIObject* pPipeUser, const char* szText, IAISignalExtraData* pData );
};

////////////////////////////////////////////////////////
//
//	animation -  sets AG input.
//
////////////////////////////////////////////////////////
class COPAnimation : public CGoalOp
{
protected:
	EAnimationMode m_eMode;
	string m_sValue;

	bool m_bAGInputSet;

public:
	COPAnimation( EAnimationMode mode, const char* value );
	COPAnimation(const XmlNodeRef& node);
	
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

	EGoalOpResult Execute( CPipeUser* pPipeUser );
	void Reset( CPipeUser* pPipeUser );
};

////////////////////////////////////////////////////////
//
//	exact positioning animation to be played at the end of the path
//
////////////////////////////////////////////////////////
class COPAnimTarget: public CGoalOp
{
	string m_sAnimName;
	float  m_fStartWidth;
	float  m_fDirectionTolerance;
	float  m_fStartArcAngle;
	Vec3   m_vApproachPos;
	bool   m_bSignal;
	bool   m_bAutoAlignment;

public:
	COPAnimTarget( bool signal, const char* animName, float startWidth, float dirTolerance, float startArcAngle, const Vec3& approachPos, bool autoAlignment );
	COPAnimTarget(const XmlNodeRef& node);

	EGoalOpResult Execute( CPipeUser* pPipeUser );
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
};

////////////////////////////////////////////////////////
//
//	wait for group of goals to be executed
//
////////////////////////////////////////////////////////
class COPWait: public CGoalOp
{
	EOPWaitType	m_WaitType;
	int					m_BlockCount;	//	number of non-blocking goals in the block

public:
	COPWait( int waitType, int blockCount );
	COPWait(const XmlNodeRef& node);

	EGoalOpResult Execute( CPipeUser* pPipeUser );
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
};

////////////////////////////////////////////////////////
//
//	Adjust aim while staying still.
//
////////////////////////////////////////////////////////
class COPAdjustAim: public CGoalOp
{
	CTimeValue	m_startTime;
	float				m_evalTime;
	bool				m_hide;
	bool				m_useLastOpAsBackup;
	bool				m_allowProne;

	float	RandomizeEvalTime();

public:
	COPAdjustAim(bool hide, bool useLastOpAsBackup, bool allowProne);

	virtual EGoalOpResult Execute(CPipeUser* pPipeUser);
	virtual void Reset( CPipeUser* pPipeUser );
	virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
	virtual void DebugDraw(CPipeUser* pPipeUser) const;
};

////////////////////////////////////////////////////////////
//
// PROXIMITY - Send signal on proximity
//
////////////////////////////////////////////////////////

class COPProximity : public CGoalOp
{
public:
	COPProximity(float radius, const string& signalName, bool signalWhenDisabled, bool visibleTargetOnly);
	COPProximity(const XmlNodeRef& node);
	~COPProximity();

	virtual EGoalOpResult Execute(CPipeUser* pPipeUser);
	virtual void Reset(CPipeUser* pPipeUser);
	virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
	virtual void DebugDraw(CPipeUser* pPipeUser) const;

private:
	float m_radius;
	bool m_triggered;
	bool m_signalWhenDisabled;
	bool m_visibleTargetOnly;
	string m_signalName;
	CWeakRef<CAIObject> m_refProxObject;
};

////////////////////////////////////////////////////////////
//
//				MOVETOWARDS - move specified distance towards last op result
//
////////////////////////////////////////////////////////
class COPMoveTowards: public CGoalOp
{
public:
	COPMoveTowards(float distance, float duration);
	COPMoveTowards(const XmlNodeRef& node);
	~COPMoveTowards();

	void Reset(CPipeUser* pPipeUser);
	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void DebugDraw(CPipeUser* pPipeUser) const;
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
private:
	void ResetNavigation(CPipeUser* pPipeUser);

private:

	float GetEndDistance(CPipeUser* pPipeUser) const;

	float m_distance;
	float m_duration;
	int		m_looseAttentionId;
	Vec3	m_moveStart;
	Vec3	m_moveEnd;
	float	m_moveDist;
	float	m_moveSearchRange;

	COPTrace* m_pTraceDirective;
	COPPathFind* m_pPathfindDirective;
};


////////////////////////////////////////////////////////////
//
// DODGE - Dodges a target
//
////////////////////////////////////////////////////////

class COPDodge: public CGoalOp
{
public:
	COPDodge(float distance, bool useLastOpAsBackup);
	COPDodge(const XmlNodeRef& node);
	~COPDodge();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
	void DebugDraw(CPipeUser* pPipeUser) const;

private:

	typedef std::pair<Vec3, float>	Vec3FloatPair;

	bool OverlapSegmentAgainstAvoidPos(const Vec3& from, const Vec3& to, float rad, const std::vector<Vec3FloatPair>& avoidPos);
	void GetNearestPuppets(CPuppet* pSelf, const Vec3& pos, float radius, std::vector<Vec3FloatPair>& positions);

	float m_distance;
	bool m_useLastOpAsBackup;
	float m_endAccuracy;
	CTimeValue m_lastTime;
	float m_notMovingTime;
	COPTrace* m_pTraceDirective;
	std::vector<Vec3FloatPair> m_avoidPos;

	std::vector<Vec3> m_DEBUG_testSegments;
	Matrix33	m_basis;
	Vec3 m_targetPos;
	Vec3 m_targetView;
	Vec3 m_bestPos;
};



////////////////////////////////////////////////////////////
//
// COMPANIONSTICK - Special stick for companion that introduces
//	speed control and rubberband-like movement
//
////////////////////////////////////////////////////////

class COPCompanionStick: public CGoalOp
{
public:
	COPCompanionStick( float fNotReactingDistance, float fForcedMoveDistance, float fLeaderInfluenceRange );
	COPCompanionStick(const XmlNodeRef& node);
	~COPCompanionStick();

	EGoalOpResult Execute(CPipeUser* pPipeUser);
	void ExecuteDry(CPipeUser* pPipeUser);
	void Reset(CPipeUser* pPipeUser);
	void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
	void DebugDraw(CPipeUser *pOperandconst) const;

private:
	void RegeneratePath(CPipeUser* pPipeUser, const Vec3 &destination);
	void AdjustSpeed(CPipeUser* pPipeUser);

	Vec3									m_vLastUsedTargetPos;
	Vec3									m_vCurrentTargetPos;	// caution: different from the point used to generate path
	CTimeValue						m_fLastRegenTime;

	float m_fMoveWillingness;
	float	m_fLeaderInfluenceRange;
	float	m_fNotReactingDistance;
	float	m_fForcedMoveDistance;

	float m_fPathLenLimit;
	float	m_fSteerDistanceSqr;
	float m_fLastDistance;
	float m_fMinEndDistance, m_fMaxEndDistance;
	float m_fEndAccuracy;
	bool m_bNeedHidespot;
	bool m_bFirstExec;

	COPTrace		*m_pTraceDirective;
	COPPathFind	*m_pPathfindDirective;
	CAIObject		*m_pPosDummy;
};



////////////////////////////////////////////////////////////
//
// DUMMY - Does nothing except for returning AIGOR_DONE
//
////////////////////////////////////////////////////////

class COPDummy : public CGoalOp
{
public:
	virtual EGoalOpResult Execute(CPipeUser* pPipeUser);
};



#endif	// #ifndef __GOALOP_H_
