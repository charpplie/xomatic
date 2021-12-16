// GoalOp.cpp: implementation of the CGoalOp class.
//
// 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext
//
//////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "GoalOp.h"
#include "Puppet.h"
#include "AIVehicle.h"
#include "AILog.h"
#include "ObjectTracker.h"
#include "IConsole.h"
#include "AICollision.h"
#include "NavRegion.h"
#include "PipeUser.h"
#include "Leader.h"
#include "DebugDrawContext.h"
#include "PathFollower.h"

#include "ISystem.h"
#include "ITimer.h"
#include "IPhysics.h"
#include "Cry_Math.h"
#include "ILog.h"
#include "ISerialize.h"
#include "TacticalPointSystem/TacticalPointSystem.h"
#include "GroupSystem/GroupSystem.h"
#include "GroupSystem/GroupMember.h"

#include "GameSpecific/GoalOp_Crysis2.h"


namespace
{
	const char* GetNameSafe(CPipeUser *pPipeUser)
	{
		return pPipeUser ? pPipeUser->GetName() : "<empty name>";
	}
}

// Ugly
#define	C_MaxDistanceForPathOffset 2 // threshold (in m) used in COPStick and COPApproach, to detect if the returned path
// is bringing the agent too far from the expected destination


CGoalOpXMLReader CGoalOp::s_xml;

CGoalOpXMLReader::CGoalOpXMLReader()
{
	m_dictAnimationMode.Add("Signal", AIANIM_SIGNAL);
	m_dictAnimationMode.Add("Action", AIANIM_ACTION);

	m_dictBools.Add("false", false);
	m_dictBools.Add("true",  true);

	m_dictCoverLocation.Add("None",      eCUL_None);
	m_dictCoverLocation.Add("Automatic", eCUL_Automatic);
	m_dictCoverLocation.Add("Left",      eCUL_Left);
	m_dictCoverLocation.Add("Right",     eCUL_Right);
	m_dictCoverLocation.Add("Center",    eCUL_Center);

	m_dictFireMode.Add("Off",              FIREMODE_OFF);
	m_dictFireMode.Add("Burst",            FIREMODE_BURST);
	m_dictFireMode.Add("Continuous",       FIREMODE_CONTINUOUS);
	m_dictFireMode.Add("Forced",           FIREMODE_FORCED);
	m_dictFireMode.Add("Aim",              FIREMODE_AIM);
	m_dictFireMode.Add("Secondary",        FIREMODE_SECONDARY);
	m_dictFireMode.Add("SecondarySmoke",   FIREMODE_SECONDARY_SMOKE);
	m_dictFireMode.Add("Melee",            FIREMODE_MELEE);
	m_dictFireMode.Add("Kill",             FIREMODE_KILL);
	m_dictFireMode.Add("BurstWhileMoving", FIREMODE_BURST_WHILE_MOVING);
	m_dictFireMode.Add("PanicSpeed",       FIREMODE_PANIC_SPREAD);
	m_dictFireMode.Add("BurstDrawFire",    FIREMODE_BURST_DRAWFIRE);
	m_dictFireMode.Add("MeleeForced",      FIREMODE_MELEE_FORCED);
	m_dictFireMode.Add("BurstSnipe",       FIREMODE_BURST_SNIPE);
	m_dictFireMode.Add("AimSweep",         FIREMODE_AIM_SWEEP);
	m_dictFireMode.Add("BurstOnce",        FIREMODE_BURST_ONCE);

	m_dictLook.Add("Look",       AILOOKMOTIVATION_LOOK);
	m_dictLook.Add("Glance",     AILOOKMOTIVATION_GLANCE);
	m_dictLook.Add("Startle",    AILOOKMOTIVATION_STARTLE);
	m_dictLook.Add("DoubleTake", AILOOKMOTIVATION_DOUBLETAKE);

	m_dictRegister.Add("LastOp",     AI_REG_LASTOP);
	m_dictRegister.Add("RefPoint",   AI_REG_REFPOINT);
	m_dictRegister.Add("AttTarget",  AI_REG_ATTENTIONTARGET);
	m_dictRegister.Add("HideObject", AI_REG_HIDEOBJECT);

	m_dictSignalFilter.Add("Sender",                  SIGNALFILTER_SENDER);
	m_dictSignalFilter.Add("LastOp",                  SIGNALFILTER_LASTOP);
	m_dictSignalFilter.Add("GroupOnly",               SIGNALFILTER_GROUPONLY);
	m_dictSignalFilter.Add("SpeciesOnly",             SIGNALFILTER_SPECIESONLY);
	m_dictSignalFilter.Add("AnyoneInComm",            SIGNALFILTER_ANYONEINCOMM);
	m_dictSignalFilter.Add("Target",                  SIGNALFILTER_TARGET);
	m_dictSignalFilter.Add("SuperGroup",              SIGNALFILTER_SUPERGROUP);
	m_dictSignalFilter.Add("SuperSpecies",            SIGNALFILTER_SUPERSPECIES);
	m_dictSignalFilter.Add("SuperTarget",             SIGNALFILTER_SUPERTARGET);
	m_dictSignalFilter.Add("NearestGroup",            SIGNALFILTER_NEARESTGROUP);
	m_dictSignalFilter.Add("NearestSpecies",          SIGNALFILTER_NEARESTSPECIES);
	m_dictSignalFilter.Add("NearestInComm",           SIGNALFILTER_NEARESTINCOMM);
	m_dictSignalFilter.Add("HalfOfGroup",             SIGNALFILTER_HALFOFGROUP);
	m_dictSignalFilter.Add("Leader",                  SIGNALFILTER_LEADER);
	m_dictSignalFilter.Add("GroupOnlyExcept",         SIGNALFILTER_GROUPONLY_EXCEPT);
	m_dictSignalFilter.Add("AnyoneInCommExcept",      SIGNALFILTER_ANYONEINCOMM_EXCEPT);
	m_dictSignalFilter.Add("LeaderEntity",            SIGNALFILTER_LEADERENTITY);
	m_dictSignalFilter.Add("NearestInCommSpecies",    SIGNALFILTER_NEARESTINCOMM_SPECIES);
	m_dictSignalFilter.Add("NearestInCommLooking",    SIGNALFILTER_NEARESTINCOMM_LOOKING);
	m_dictSignalFilter.Add("Formation",               SIGNALFILTER_FORMATION);
	m_dictSignalFilter.Add("FormationExcept",         SIGNALFILTER_FORMATION_EXCEPT);
	m_dictSignalFilter.Add("Readability",             SIGNALFILTER_READABILITY);
	m_dictSignalFilter.Add("ReadabilityAnticipation", SIGNALFILTER_READABILITYAT);
	m_dictSignalFilter.Add("ReadabilityResponse",     SIGNALFILTER_READABILITYRESPONSE);

	m_dictStance.Add("Null",      STANCE_NULL);
	m_dictStance.Add("Stand",     STANCE_STAND);
	m_dictStance.Add("Crouch",    STANCE_CROUCH);
	m_dictStance.Add("Prone",     STANCE_PRONE);
	m_dictStance.Add("Relaxed",   STANCE_RELAXED);
	m_dictStance.Add("Stealth",   STANCE_STEALTH);
	m_dictStance.Add("LowCover",  STANCE_LOW_COVER);
	m_dictStance.Add("HighCover", STANCE_HIGH_COVER);
	m_dictStance.Add("Swim",      STANCE_SWIM);
	m_dictStance.Add("ZeroG",     STANCE_ZEROG);
	
	m_dictUrgency.Add("Zero",   AISPEED_ZERO);
	m_dictUrgency.Add("Slow",   AISPEED_SLOW);
	m_dictUrgency.Add("Walk",   AISPEED_WALK);
	m_dictUrgency.Add("Run",    AISPEED_RUN);
	m_dictUrgency.Add("Sprint", AISPEED_SPRINT);
}

bool CGoalOpXMLReader::GetBool(const XmlNodeRef& node, const char* szAttrName, bool bDefaultValue)
{
	bool bValue;
	if (m_dictBools.Get(node, szAttrName, bValue))
	{
		return bValue;
	}

	return bDefaultValue;
}

bool CGoalOpXMLReader::GetMandatoryBool(const XmlNodeRef& node, const char* szAttrName)
{
	bool bValue;
	return m_dictBools.Get(node, szAttrName, bValue, true) ? bValue : false;
}

const char* CGoalOpXMLReader::GetMandatoryString(const XmlNodeRef& node, const char* szAttrName)
{
	const char* sz;
	if (!node->getAttr(szAttrName, &sz))
	{
		AIError("Unable to get mandatory string attribute '%s' of node '%s'.",
			szAttrName, node->getTag());
		sz = "";
	}

	return sz;
}


//#pragma optimize("", off)
//#pragma inline_depth(0)

// The end point accuracy to use when tracing and we don't really care. 0 results in exact positioning being used
// a small value (0.1) allows some sloppiness. Both should work.
static float defaultTraceEndAccuracy = 0.1f;

CObjectTracker *COPStick::SSafePoint::pObjectTracker = 0;


EGoalOpResult COPAcqTarget::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPAcqTarget_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	pPipeUser->m_bLooseAttention = false;

	CAIObject* pTargetObject = m_sTargetName.empty() ? 0 : pPipeUser->GetSpecialAIObject(m_sTargetName);

	pPipeUser->SetAttentionTarget(pTargetObject ? GetWeakRef(pTargetObject) : pPipeUser->m_refLastOpResult);

	return eGOR_DONE;
	}

COPApproach::COPApproach(float fEndDistance, float fEndAccuracy, float fDuration,
												 bool bUseLastOpResult, bool bLookAtLastOp,
												 bool bForceReturnPartialPath, bool bStopOnAnimationStart,
												 const char* szNoPathSignalText) :
m_fLastDistance(0.0f),
m_fInitialDistance(0.0f),
m_bUseLastOpResult(bUseLastOpResult),
m_bLookAtLastOp(bLookAtLastOp),
m_fEndDistance(fEndDistance),
m_fEndAccuracy(fEndAccuracy),
m_fDuration(fDuration),
m_bForceReturnPartialPath(bForceReturnPartialPath),
m_stopOnAnimationStart(bStopOnAnimationStart),
m_looseAttentionId(0),
m_pTraceDirective(0),
m_pPathfindDirective(0),
m_bPathFound(false)
{
	CCCPOINT(COPApproach_COPApproach);
}

COPApproach::COPApproach(const XmlNodeRef& node) :
	m_fLastDistance(0.f),
	m_fInitialDistance(0.f),
	m_bUseLastOpResult(s_xml.GetBool(node, "useLastOp")),
	m_bLookAtLastOp(s_xml.GetBool(node, "lookAtLastOp")),
	m_fEndDistance(0.f),
	m_fEndAccuracy(0.f),
	m_fDuration(0.f),
	m_bForceReturnPartialPath(s_xml.GetBool(node, "requestPartialPath")),
	m_stopOnAnimationStart(s_xml.GetBool(node, "stopOnAnimationStart")),
	m_looseAttentionId(0),
	m_pTraceDirective(0),
	m_pPathfindDirective(0),
	m_bPathFound(false)
{
	if (node->getAttr("time", m_fDuration))
	{
		m_fDuration = fabsf(m_fDuration);
	}
	else
	{
		s_xml.GetMandatory(node, "distance", m_fEndDistance);
	}

	node->getAttr("endAccuracy", m_fEndAccuracy);
}

COPApproach::~COPApproach()
{
	Reset(0);
}

void COPApproach::DebugDraw(CPipeUser* pPipeUser) const
{
	// TODO evgeny Consider an abstract class to avoid retyping m_pTraceDirective/m_pPathfindDirective
	if (m_pTraceDirective)
	{
		m_pTraceDirective->DebugDraw(pPipeUser);
	}

  if (m_pPathfindDirective)
{
		m_pPathfindDirective->DebugDraw(pPipeUser);
	}
}

EGoalOpResult COPApproach::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPApproach_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	int debugPathfinding = gAIEnv.CVars.DebugPathFinding;

	CAIObject* pTarget = static_cast<CAIObject*>(pPipeUser->GetAttentionTarget());
	 
	// Move strictly to the target point
	pPipeUser->m_nMovementPurpose = 0;

	if (!pTarget || m_bUseLastOpResult) 
	{
		CAIObject* pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();
		if (pLastOpResult)
		{
			pTarget = pLastOpResult;
		}
		else
		{
			// no target, nothing to approach to
			if (debugPathfinding)
				AILogAlways("COPApproach::Execute %s no target; resetting goalop", GetNameSafe(pPipeUser));
			Reset(pPipeUser);
			return eGOR_FAILED;
		}
	}

	// luciano - added check for formation points
	if(pTarget && !pTarget->IsEnabled())
	{
		if (debugPathfinding)
			AILogAlways("COPApproach::Execute %s target %s not enabled", GetNameSafe(pPipeUser), pTarget->GetName());
		Reset(pPipeUser);
		return eGOR_FAILED;
	}

	Vec3 mypos = pPipeUser->GetPos();

	Vec3 targetpos;
	Vec3 targetdir;
	if ((pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND) && m_bForceReturnPartialPath)
	{
		targetpos = pPipeUser->m_Path.GetLastPathPos();
		targetdir = pPipeUser->m_Path.GetEndDir();
	}
	else
	{
		targetpos = pTarget->GetPhysicsPos();
		targetdir = pTarget->GetMoveDir();
	}

	CWeakRef<CAIObject>& refLastOpResult = pPipeUser->m_refLastOpResult;
	if (!m_looseAttentionId && m_bLookAtLastOp && refLastOpResult.IsValid())
	{
		m_looseAttentionId = pPipeUser->SetLooseAttentionTarget(refLastOpResult);
	}

	if (pPipeUser->GetSubType() == CAIObject::STP_HELI)
	{
		// Make sure helicopter gets up first, and then starts moving
		if (CAIVehicle* pHeli = pPipeUser->CastToCAIVehicle())
			if (pHeli->HandleVerticalMovement(targetpos))
				return eGOR_IN_PROGRESS;
	}

	if (!pPipeUser->m_movementAbility.bUsePathfinder)
	{
		Vec3 projectedDist = mypos - targetpos;
		float dist = pPipeUser->m_movementAbility.b3DMove ? projectedDist.GetLength() : projectedDist.GetLength2D();

		float endDistance = GetEndDistance(pPipeUser);

		if ( dist < endDistance )
		{
			if (debugPathfinding)
				AILogAlways("COPApproach::Execute %s No pathfinder and reached end", GetNameSafe(pPipeUser));
			m_fInitialDistance = 0;
			Reset(pPipeUser);
			return eGOR_SUCCEEDED;
		}
		
		// no pathfinding - just approach
		pPipeUser->m_State.vMoveDir = targetpos - pPipeUser->GetPhysicsPos();
		pPipeUser->m_State.vMoveDir.Normalize();

		// Update the debug movement reason.
		pPipeUser->m_DEBUGmovementReason = CPipeUser::AIMORE_SMARTOBJECT;

		m_fLastDistance = dist;
		return eGOR_IN_PROGRESS;
	}

	if (!m_bPathFound && !m_pPathfindDirective)
	{
		// generate path to target
    float endTol = m_bForceReturnPartialPath || m_fEndAccuracy < 0.0f ? std::numeric_limits<float>::max() : m_fEndAccuracy;

		// override end distance if a duration has been set
		float endDistance = GetEndDistance(pPipeUser);
		m_pPathfindDirective = new COPPathFind("", pTarget, endTol, endDistance);
		pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

		if (m_pPathfindDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
		{
			if (pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
			{
				if (debugPathfinding)
					AILogAlways("COPApproach::Execute %s pathfinder no path", GetNameSafe(pPipeUser));
				// If special nopath signal is specified, send the signal.
				if(m_noPathSignalText.size() > 0)
					pPipeUser->SetSignal(0, m_noPathSignalText.c_str(), NULL);
				pPipeUser->m_State.vMoveDir.Set(0, 0, 0);
				Reset(pPipeUser);
				return eGOR_FAILED;
			}
		}
		return eGOR_IN_PROGRESS;
	}

  // trace/pathfinding gets deleted when we reach the end
  if (!m_pPathfindDirective)
	{
		if (debugPathfinding)
		{
			AILogAlways("COPApproach::Execute (%p) returning true due to no pathfinding directive %s",
				this,
				pPipeUser ? GetNameSafe(pPipeUser) : "");
		}
		Reset(pPipeUser);
		return eGOR_FAILED;
  }

  // actually trace the path - continue doing this even whilst regenerating (etc) the path
	EGoalOpResult doneTracing = eGOR_IN_PROGRESS;
  if (m_bPathFound)
		{
			if (!m_pTraceDirective)
			{
			if ((pTarget->GetSubType() == CAIObject::STP_ANIM_TARGET) && (m_fEndDistance > 0.0f || m_fDuration > 0.0f))
				{
        AILogAlways("COPApproach::Execute resetting approach distance from (endDist=%.1f duration=%.1f) to zero because the approach target is anim target. %s",
					m_fEndDistance, m_fDuration, pPipeUser ? GetNameSafe(pPipeUser) : "_no_pipe_user_");
        m_fEndDistance = 0.0f;
				m_fDuration = 0.0f;
				}

			TPathPoints::const_reference lastPathNode = pPipeUser->m_OrigPath.GetPath().back();
				Vec3 lastPos = lastPathNode.vPos;
			Vec3 requestedLastNodePos = pPipeUser->m_Path.GetParams().end;
				float dist = Distance::Point_Point(lastPos,requestedLastNodePos);
			float endDistance = GetEndDistance(pPipeUser);
			if (lastPathNode.navType != IAISystem::NAV_SMARTOBJECT && dist > endDistance+C_MaxDistanceForPathOffset)// && pPipeUser->m_Path.GetPath().size() == 1 )
				{
					AISignalExtraData* pData = new AISignalExtraData;
        pData->fValue = dist - endDistance;
				pPipeUser->SetSignal(0,"OnEndPathOffset",pPipeUser->GetEntity(),pData, gAIEnv.SignalCRCs.m_nOnEndPathOffset);
				}
				else
			{
				pPipeUser->SetSignal(0,"OnPathFound",NULL, 0, gAIEnv.SignalCRCs.m_nOnPathFound);
			}

      bool bExact = false;
//      m_pTraceDirective = new COPTrace(bExact, endDistance, m_fEndAccuracy, m_fDuration, m_bForceReturnPartialPath, m_stopOnAnimationStart);
			m_pTraceDirective = new COPTrace(bExact, m_fEndAccuracy, m_bForceReturnPartialPath, m_stopOnAnimationStart);
			}

		doneTracing = m_pTraceDirective->Execute(pPipeUser);
			

			
			// If this goal gets reseted during m_pTraceDirective->Execute it means that
			// a smart object was used for navigation which inserts a goal pipe which
			// does Reset on this goal which sets m_pTraceDirective to NULL! In this case
			// we should just report that this goal pipe isn't finished since it will be
			// reexecuted after finishing the inserted goal pipe
			if ( !m_pTraceDirective )
			return eGOR_IN_PROGRESS;

    // If the path has been traced, finish the operation
		if (doneTracing != eGOR_IN_PROGRESS)
			{
			if (debugPathfinding)
        AILogAlways("COPApproach::Execute (%p) finishing due to finished tracing %s", this, 
				pPipeUser ? GetNameSafe(pPipeUser) : "");
			Reset(pPipeUser);
      return doneTracing;
			}
		}

  // check pathfinder status
	switch(pPipeUser->m_nPathDecision)
			{
  case PATHFINDER_STILLFINDING:
		m_pPathfindDirective->Execute(pPipeUser);		
		return eGOR_IN_PROGRESS;

  case PATHFINDER_NOPATH:
		pPipeUser->m_State.vMoveDir.Set(0,0,0);
		if (debugPathfinding)
      AILogAlways("COPApproach::Execute (%p) resetting due to no path %s", this, 
			pPipeUser ? GetNameSafe(pPipeUser) : "");
				// If special nopath signal is specified, send the signal.
				if(m_noPathSignalText.size() > 0)
			pPipeUser->SetSignal(0,m_noPathSignalText.c_str(),NULL);
		Reset(pPipeUser);
		return eGOR_FAILED;

  case PATHFINDER_PATHFOUND:
    if(!m_bPathFound)
      {
      m_bPathFound = true;
			return Execute(pPipeUser);
      }
    break;
		}

	return eGOR_IN_PROGRESS;
}

void COPApproach::Reset(CPipeUser* pPipeUser)
{
	if (gAIEnv.CVars.DebugPathFinding)
	{
		AILogAlways("COPApproach::Reset %s", GetNameSafe(pPipeUser));
	}

	SAFE_DELETE(m_pPathfindDirective);
	SAFE_DELETE(m_pTraceDirective);
	
	m_bPathFound = false;

	m_fInitialDistance = 0;

	if (pPipeUser)
	{
		pPipeUser->ClearPath("COPApproach::Reset m_Path");
		if (m_bLookAtLastOp)
		{
			pPipeUser->SetLooseAttentionTarget(NILREF, m_looseAttentionId);
			m_looseAttentionId = 0;
		}
	}
}


void COPApproach::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPApproach");
	{
		ser.Value("m_fLastDistance",m_fLastDistance);
		ser.Value("m_fInitialDistance",m_fInitialDistance);
		ser.Value("m_bUseLastOpResult",m_bUseLastOpResult);
		ser.Value("m_bLookAtLastOp",m_bLookAtLastOp);
		ser.Value("m_fEndDistance",m_fEndDistance);
		ser.Value("m_fEndAccuracy",m_fEndAccuracy);
		ser.Value("m_fDuration", m_fDuration);
		ser.Value("m_bForceReturnPartialPath",m_bForceReturnPartialPath);
		ser.Value("m_stopOnAnimationStart", m_stopOnAnimationStart);
		ser.Value("m_noPathSignalText",m_noPathSignalText);
		ser.Value("m_looseAttentionId",m_looseAttentionId);
		ser.Value("m_bPathFound", m_bPathFound);

		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("PathFindDirective", m_pPathfindDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
      SAFE_DELETE(m_pPathfindDirective);
      if(ser.BeginOptionalGroup("PathFindDirective", true))
      {
        m_pPathfindDirective = new COPPathFind("");
        m_pPathfindDirective->Serialize(ser, objectTracker);
        ser.EndGroup();
      }
    }
		}
		ser.EndGroup();
}

//===================================================================
// GetEndDistance
//===================================================================
float COPApproach::GetEndDistance(CPipeUser* pPipeUser) const
{
	if (m_fDuration > 0.0f)
{
		//		float normalSpeed = pPipeUser->GetNormalMovementSpeed(pPipeUser->m_State.fMovementUrgency, false);
		float normalSpeed, smin, smax;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, false, normalSpeed, smin, smax);
		if (normalSpeed > 0.0f)
			return -normalSpeed * m_fDuration;
	}
	return m_fEndDistance;
}



//===================================================================
// ExecuteDry
//===================================================================
void COPApproach::ExecuteDry(CPipeUser* pPipeUser) 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
  if (m_pTraceDirective && m_bPathFound)
    m_pTraceDirective->ExecuteTrace(pPipeUser, false);
}


// COPFollowPath
//====================================================================
COPFollowPath::COPFollowPath(bool pathFindToStart, bool reverse, bool startNearest, int loops, float fEndAccuracy, bool bUsePointList) :
	m_pathFindToStart(pathFindToStart),
	m_reversePath(reverse),
	m_startNearest(startNearest),
	m_bUsePointList(bUsePointList),
	m_loops(loops),
	m_loopCounter(0),
	m_notMovingTime(0.0f),
	m_returningToPath(false)
{
	m_pTraceDirective = 0;
  m_pPathFindDirective = 0;
	m_fEndAccuracy  = fEndAccuracy;

}

COPFollowPath::COPFollowPath(const XmlNodeRef& node) :
	m_pathFindToStart(s_xml.GetBool(node, "pathFindToStart", true)),
	m_reversePath(s_xml.GetBool(node, "reversePath", true)),
	m_startNearest(s_xml.GetBool(node, "startNearest", true)),
	m_bUsePointList(s_xml.GetBool(node, "usePointList")),
	m_loops(0),
	m_loopCounter(0),
	m_notMovingTime(0.0f),
	m_returningToPath(false),
	m_pTraceDirective(0),
	m_pPathFindDirective(0),
	m_fEndAccuracy(0)
{
	s_xml.GetMandatory(node, "loops", m_loops);
	node->getAttr("endAccuracy", m_fEndAccuracy);
}

//====================================================================
// ~COPFollowPath
//====================================================================
COPFollowPath::~COPFollowPath()
{
	Reset(0);
}

//====================================================================
// Reset
//====================================================================
void COPFollowPath::Reset(CPipeUser* pPipeUser)
{
	CCCPOINT(COPFollowPath_Reset);

	// (MATT) Apparently we hang onto the path start dummy {2009/02/17}
	delete m_pTraceDirective;
	m_pTraceDirective = 0;
	delete m_pPathFindDirective;
	m_pPathFindDirective = 0;
	m_loopCounter = 0;
	m_notMovingTime = 0.0f;
	m_returningToPath = false;
	m_fEndAccuracy = defaultTraceEndAccuracy;
	if (pPipeUser)
		pPipeUser->ClearPath("COPFollowPath::Reset m_Path");
}

//====================================================================
// Serialize
//====================================================================
void COPFollowPath::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPFollowPath");
	{
		ser.Value("m_pathFindToStart",m_pathFindToStart);
		ser.Value("m_reversePath",m_reversePath);
		ser.Value("m_startNearest",m_startNearest);
		ser.Value("m_loops",m_loops);
		ser.Value("m_loopCounter",m_loopCounter);
		ser.Value("m_TraceEndAccuracy",m_fEndAccuracy);
		ser.Value("m_notMovingTime",m_notMovingTime);
		ser.Value("m_returningToPath",m_returningToPath);
		ser.Value("m_bUsePointList",m_bUsePointList);

		m_refPathStartPoint.Serialize(ser,"m_refPathStartPoint");
		
		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("PathFindDirective", m_pPathFindDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathFindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(false, m_fEndAccuracy);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
      SAFE_DELETE(m_pPathFindDirective);
			if(ser.BeginOptionalGroup("PathFindDirective", true))
			{
				m_pPathFindDirective = new COPPathFind("");
				m_pPathFindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		ser.EndGroup();
	}
}

//====================================================================
// Execute
//====================================================================
EGoalOpResult COPFollowPath::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CAIObject *pTarget = (CAIObject*)pPipeUser->GetAttentionTarget();
  CAISystem *pSystem = GetAISystem();

  // for a temporary, until all functionality have made.
  if ( m_bUsePointList == true && !( m_pathFindToStart == false && m_reversePath == false && m_startNearest == false ) )
  {
    AIWarning("COPFollowPath:: bUsePointList only support false,flase,false in first 3 paramters for %s", GetNameSafe(pPipeUser));
    return eGOR_FAILED;
  }

	CCCPOINT(COPFollowPath_Execute);

  // if we have a path, trace it. Once we get going (i.e. pathfind is finished, if we use it) we'll
  // always have a trace
  if (!m_pTraceDirective)
  {
    if (m_pPathFindDirective)
    {
			CCCPOINT(COPFollowPath_Execute_A);

      EGoalOpResult finishedPathFind = m_pPathFindDirective->Execute(pPipeUser);
      if (finishedPathFind != eGOR_IN_PROGRESS)
      {
        m_pTraceDirective = new COPTrace(false, m_fEndAccuracy);
        m_lastTime = pSystem->GetFrameStartTime();
      }
      else
        return eGOR_IN_PROGRESS;
    }
    else // no pathfind directive
    {
      //      m_pathFindToStart = pPipeUser->GetPathFindToStartOfPathToFollow(pathStartPos);
      if (m_pathFindToStart)
      {
				CCCPOINT(COPFollowPath_Execute_B);

				// Create the PathStartPoint if needed
				if (m_refPathStartPoint.IsNil())
					GetAISystem()->CreateDummyObject(m_refPathStartPoint, string(GetNameSafe(pPipeUser)) + "_PathStartPoint", CAIObject::STP_REFPOINT );

        AIAssert(m_refPathStartPoint);

        Vec3	entryPos;
        if(!pPipeUser->GetPathEntryPoint(entryPos, m_reversePath, m_startNearest))
        {
          AIWarning("COPFollowPath::Unable to find path entry point for %s - check path is not marked as a road", GetNameSafe(pPipeUser));
          return eGOR_FAILED;
        }
        m_refPathStartPoint->SetPos(entryPos);
        m_pPathFindDirective = new COPPathFind("FollowPath", m_refPathStartPoint.GetAIObject());

        return eGOR_IN_PROGRESS;
      }
      else
      {
				CCCPOINT(COPFollowPath_Execute_C);

        if ( m_bUsePointList == true )
        {
          bool pathOK = pPipeUser->UsePointListToFollow();
          if (!pathOK)
          {
            AIWarning("COPFollowPath::Execute Unable to use point list %s", GetNameSafe(pPipeUser));
            return eGOR_FAILED;
          }
        }
        else
        {
          bool pathOK = pPipeUser->UsePathToFollow(m_reversePath, m_startNearest, m_loops != 0);
          if (!pathOK)
          {
            AIWarning("COPFollowPath::Execute Unable to use follow path for %s - check path is not marked as a road", GetNameSafe(pPipeUser));
            return eGOR_FAILED;
          }
        }
        m_pTraceDirective = new COPTrace(false, m_fEndAccuracy);
        m_lastTime = pSystem->GetFrameStartTime();
      } // m_pathFindToStart
    } // path find directive
  }
  AIAssert(m_pTraceDirective);
  EGoalOpResult done = m_pTraceDirective->Execute(pPipeUser);

  // workaround: The following code tries to put a lost or stuck agent back on track.
  // It works together with a piece of in ExecuteDry which tracks the speed relative
  // to the requested speed and if it drops dramatically for certain time, this code
  // will trigger and try to move the agent back on the path. [Mikko]

  float timeout = 0.7f;
  if ( pPipeUser->GetType() == AIOBJECT_VEHICLE )
    timeout =7.0f;
  if ( pPipeUser->GetSubType() == CAIObject::STP_2D_FLY  )
    m_notMovingTime = 0.0f;

  if ( m_notMovingTime > timeout )
  {
		CCCPOINT(COPFollowPath_Execute_Stuck);

    // Stuck or lost, move to the nearest point on path.
    AIWarning("COPFollowPath::Entity %s has not been moving fast enough for %.1fs it might be stuck, find back to path.", GetNameSafe(pPipeUser), m_notMovingTime);

		// Create the PathStartPoint if needed
		// (MATT) And if definately is, sometimes. But fix this code duplication. {2009/02/18}
		if (m_refPathStartPoint.IsNil())
			GetAISystem()->CreateDummyObject( m_refPathStartPoint, string(GetNameSafe(pPipeUser)) + "_PathStartPoint", CAIObject::STP_REFPOINT );

    Vec3	entryPos;
    if(!pPipeUser->GetPathEntryPoint(entryPos, m_reversePath, true))
    {
      AIWarning("COPFollowPath::Unable to find path entry point for %s - check path is not marked as a road", GetNameSafe(pPipeUser));
      return eGOR_FAILED;
    }
    m_refPathStartPoint->SetPos(entryPos);
    m_pPathFindDirective = new COPPathFind("FollowPath", m_refPathStartPoint.GetAIObject());
    delete m_pTraceDirective;
    m_pTraceDirective = 0;
    m_notMovingTime = 0.0f;
    m_returningToPath = true;
  }

  if (done != eGOR_IN_PROGRESS || !m_pTraceDirective)
  {
    if ((m_pathFindToStart || m_returningToPath) && m_pPathFindDirective)
    {
			CCCPOINT(COPFollowPath_Execute_D);

      pPipeUser->SetSignal(1,"OnPathFindAtStart",0,0,gAIEnv.SignalCRCs.m_nOnPathFindAtStart);

      delete m_pPathFindDirective;
      m_pPathFindDirective = 0;
      delete m_pTraceDirective;
      bool pathOK = pPipeUser->UsePathToFollow(m_reversePath, (m_startNearest || m_returningToPath), false);
      if (!pathOK)
      {
        AIWarning("COPFollowPath::Execute Unable to use follow path for %s - check path is not marked as a road", GetNameSafe(pPipeUser));
        return eGOR_FAILED;
      }
      m_pTraceDirective = new COPTrace(false, m_fEndAccuracy );
      m_lastTime = pSystem->GetFrameStartTime();
      m_returningToPath = false;
      return eGOR_IN_PROGRESS;
    }
    else
    {
      if(m_loops != 0)
      {
				CCCPOINT(COPFollowPath_Execute_C);

        m_loopCounter++;
        if(m_loops != -1 && m_loopCounter >= m_loops)
        {
          Reset(pPipeUser);
          return eGOR_SUCCEEDED;
        }

        delete m_pTraceDirective;
        bool pathOK = pPipeUser->UsePathToFollow(m_reversePath, m_startNearest, m_loops != 0);
        if (!pathOK)
        {
          AIWarning("COPFollowPath::Execute Unable to use follow path for %s - check path is not marked as a road", GetNameSafe(pPipeUser));
          return eGOR_FAILED;
        }
        m_pTraceDirective = new COPTrace(false, m_fEndAccuracy);
        m_lastTime = pSystem->GetFrameStartTime();
        // For seamless operation, update trace already.
        m_pTraceDirective->Execute(pPipeUser);
        return eGOR_IN_PROGRESS;
      }
    }

    Reset(pPipeUser);
    return eGOR_SUCCEEDED;
  }

  return eGOR_IN_PROGRESS;
}

//====================================================================
// ExecuteDry
//====================================================================
void COPFollowPath::ExecuteDry(CPipeUser* pPipeUser) 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CAISystem *pSystem = GetAISystem();

  if (m_pTraceDirective)
	{
		CCCPOINT(COPFollowPath_ExecuteDry);

    m_pTraceDirective->ExecuteTrace(pPipeUser, false);

		// workaround: The following code together with some logic in the execute tries to keep track
		// if the agent is not moving for some time (is stuck), and pathfinds back to the path. [Mikko]
		CTimeValue	time(pSystem->GetFrameStartTime());
		float	dt((time - m_lastTime).GetSeconds());
		float	speed = pPipeUser->GetVelocity().GetLength();
		float	desiredSpeed = pPipeUser->m_State.fDesiredSpeed;
		if(desiredSpeed > 0.0f)
		{
			float ratio = clamp(speed / desiredSpeed, 0.0f, 1.0f);
			if(ratio < 0.1f)
				m_notMovingTime += dt;
			else
				m_notMovingTime -= dt;
			if(m_notMovingTime < 0.0f)
				m_notMovingTime = 0.0f;
		}
		m_lastTime = time;
	}
}

//===================================================================
// COPBackoff
//===================================================================
COPBackoff::COPBackoff(float distance, float duration, int filter, float minDistance)
{
	m_fDistance = distance;
	m_fDuration = duration;
	m_iDirection = filter & (AI_MOVE_FORWARD | AI_MOVE_BACKWARD | AI_MOVE_LEFT | AI_MOVE_RIGHT | AI_MOVE_TOWARDS_GROUP | AI_MOVE_BACKLEFT | AI_MOVE_BACKRIGHT);
	m_fMinDistance = minDistance;
	if(m_iDirection==0)
		m_iDirection = AI_MOVE_BACKWARD;

	//m_iAvailableDirections = m_iDirection;

	m_bUseLastOp = (filter & AILASTOPRES_USE) !=0;
	m_bLookForward = (filter & AI_LOOK_FORWARD) !=0;
	m_bUseTargetPosition = (filter & AI_BACKOFF_FROM_TARGET)!=0;
	m_bRandomOrder = (filter & AI_RANDOM_ORDER)!=0;
	m_bCheckSlopeDistance = (filter & AI_CHECK_SLOPE_DISTANCE) !=0;

	if(m_fDistance<0.f)
	{
		m_fDistance = -m_fDistance;
	}
	m_pPathfindDirective = 0;
	m_pTraceDirective = 0;
	m_currentDirMask = AI_MOVE_BACKWARD;
	m_vBestFailedBackoffPos.Set(0,0,0);
	m_fMaxDistanceFound = 0.f;
	m_bTryingLessThanMinDistance = false;
	m_looseAttentionId = 0;
	ResetMoveDirections();
}

COPBackoff::COPBackoff(const XmlNodeRef& node) :
	m_fDistance(0.f),
	m_fDuration(0.f),
	m_fMinDistance(0.f),
	m_pPathfindDirective(0),
	m_pTraceDirective(0),
	m_currentDirMask(AI_MOVE_BACKWARD),
	m_vBestFailedBackoffPos(ZERO),
	m_fMaxDistanceFound(0.f),
	m_bTryingLessThanMinDistance(false),
	m_looseAttentionId(0),
	m_bUseLastOp(s_xml.GetBool(node, "useLastOp")),
	m_bLookForward(s_xml.GetBool(node, "lookForward")),
	m_bUseTargetPosition(s_xml.GetBool(node, "backOffFromTarget")),
	m_bRandomOrder(s_xml.GetBool(node, "randomOrder")),
	m_bCheckSlopeDistance(s_xml.GetBool(node, "checkSlopeDistance"))
{
	s_xml.GetMandatory(node, "distance", m_fDistance);
	if(m_fDistance < 0.f)
	{
		m_fDistance = -m_fDistance;
	}
	
	node->getAttr("maxDuration", m_fDuration);

	m_iDirection = (s_xml.GetBool(node, "moveForward")        ? AI_MOVE_FORWARD       : 0) |
	               (s_xml.GetBool(node, "moveBackward")       ? AI_MOVE_BACKWARD      : 0) |
		             (s_xml.GetBool(node, "moveLeft")           ? AI_MOVE_LEFT          : 0) |
		             (s_xml.GetBool(node, "moveRight")          ? AI_MOVE_RIGHT         : 0) |
		             (s_xml.GetBool(node, "moveBackLeft")       ? AI_MOVE_BACKLEFT      : 0) |
		             (s_xml.GetBool(node, "moveBackRight")      ? AI_MOVE_BACKRIGHT     : 0) |
		             (s_xml.GetBool(node, "moveTowardsGroup")   ? AI_MOVE_TOWARDS_GROUP : 0);
	if (m_iDirection == 0)
	{
		m_iDirection = AI_MOVE_BACKWARD;
	}

	node->getAttr("minDistance", m_fMinDistance);
	
	ResetMoveDirections();
}

//===================================================================
// COPBackoff
//===================================================================
COPBackoff::~COPBackoff()
{
  Reset(0);
}

//===================================================================
// Reset
//===================================================================
void COPBackoff::Reset(CPipeUser* pPipeUser)
{
	ResetNavigation(pPipeUser);
	m_fMaxDistanceFound = 0;
	m_bTryingLessThanMinDistance = false;
	m_vBestFailedBackoffPos.Set(0,0,0);
	m_currentDirMask = AI_MOVE_BACKWARD;
	ResetMoveDirections();
}

//===================================================================
// SetMoveDirections
//===================================================================
void COPBackoff::ResetMoveDirections()
{
	m_iCurrentDirectionIndex=0;
	m_MoveDirections.clear();
	
	int mask = (AI_MOVE_FORWARD | AI_MOVE_BACKWARD | AI_MOVE_LEFT | AI_MOVE_RIGHT | AI_MOVE_TOWARDS_GROUP | AI_MOVE_BACKLEFT | AI_MOVE_BACKRIGHT);
	mask &= m_iDirection;

	int currentdir = 1;
	while(currentdir <= mask)
	{
		if((mask & currentdir)!=0)
			m_MoveDirections.push_back(currentdir);
		currentdir<<=1;
	}
	if(m_bRandomOrder)
	{
		int size = m_MoveDirections.size();
		for(int i=0;i<size;++i)
		{
			int other = ai_rand() % size;
			int temp = m_MoveDirections[i];
			m_MoveDirections[i] = m_MoveDirections[other];
			m_MoveDirections[other] = temp;
		}
	}
}

//===================================================================
// ResetNavigation
//===================================================================
void COPBackoff::ResetNavigation(CPipeUser* pPipeUser)
{
	if(m_pPathfindDirective)
		delete m_pPathfindDirective;
	m_pPathfindDirective = 0;
	if(m_pTraceDirective)
		delete m_pTraceDirective;
	m_pTraceDirective = 0;

	if (pPipeUser)
		pPipeUser->ClearPath("COPBackoff::Reset m_Path");

	if (m_refBackoffPoint.IsNil())
	{
		m_refBackoffPoint.Release();
		if(m_bLookForward && pPipeUser )
		{
			pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
		}
	}
	m_looseAttentionId=0;
}

//===================================================================
// Execute
//===================================================================
void COPBackoff::ExecuteDry(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
  if (m_pTraceDirective)
    m_pTraceDirective->ExecuteTrace(pPipeUser, false);
}

//===================================================================
// Execute
//===================================================================
EGoalOpResult COPBackoff::Execute(CPipeUser* pPipeUser)
{ 
	CCCPOINT(COPBackoff_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  m_fLastUpdateTime = GetAISystem()->GetFrameStartTime();

  if (!m_pPathfindDirective)
  {
		CAIObject *pTarget = NULL;

		if(m_bUseLastOp)
		{
			pTarget = pPipeUser->m_refLastOpResult.GetAIObject();
			if (!pTarget)
			{
					Reset(pPipeUser);
					return eGOR_FAILED;
			}
		}
		else
		{
			pTarget = (CAIObject*)pPipeUser->GetAttentionTarget();
			if (!pTarget)
			{
				if (!(pTarget = pPipeUser->m_refLastOpResult.GetAIObject()))
				{
					Reset(pPipeUser);
					return eGOR_FAILED;
				}
			}
		}
		
		int currentDir = m_iCurrentDirectionIndex >=(int) m_MoveDirections.size()? 0 : m_MoveDirections[m_iCurrentDirectionIndex];
		if(m_iCurrentDirectionIndex < (int)m_MoveDirections.size())
			m_iCurrentDirectionIndex++;

		if(currentDir == 0)
		{
			if(m_fMaxDistanceFound>0.f)
			{
				// a path less than mindistance required has been found anyway
				CAIObject *pBackoffPoint = m_refBackoffPoint.GetAIObject();
				m_pPathfindDirective = new COPPathFind("Backoff", pBackoffPoint, std::numeric_limits<float>::max(), 0.0f, false, m_fDistance);
				pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;
				m_bTryingLessThanMinDistance = true;
				if (m_pPathfindDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
				{
					if (pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
					{
						pPipeUser->m_State.vMoveDir.Set(0,0,0);
						pPipeUser->SetSignal(1,"OnBackOffFailed" ,0, 0, gAIEnv.SignalCRCs.m_nOnBackOffFailed);
						Reset(pPipeUser);	
						return eGOR_FAILED;
					}
				}
			}
			else
			{
				pPipeUser->m_State.vMoveDir.Set(0,0,0);
				pPipeUser->SetSignal(1,"OnBackOffFailed",0, 0, gAIEnv.SignalCRCs.m_nOnBackOffFailed);
				Reset(pPipeUser);	
				return eGOR_FAILED;
			}
		}

    Vec3 mypos = pPipeUser->GetPhysicsPos();
		Vec3 tgpos = pTarget->GetPhysicsPos();

    // evil workaround
		CAIActor* pTargetActor = pTarget->CastToCAIActor();
		if (pTargetActor && pTarget->GetType()==AIOBJECT_VEHICLE)
			tgpos+=pTargetActor->m_State.vMoveDir*10.f;

		Vec3 vMoveDir = tgpos - mypos;
    // zero the z value because for human backoffs it seems that when the z values differ 
    // by a large amount at close range the direction tracing doesn't work so well
    if (!pPipeUser->IsUsing3DNavigation())
      vMoveDir.z = 0.0f;

		Vec3 avoidPos;
		
		if (m_bUseTargetPosition)
			avoidPos = tgpos;
		else
			avoidPos = mypos;

		// This check assumes that the reason of the backoff
		// if to move away to specified direction away from a point to avoid.
		// If the agent is already far enough, stop immediately.
		if(Distance::Point_Point(avoidPos, mypos) > m_fDistance)
		{
			Reset(pPipeUser);	
			return eGOR_SUCCEEDED;
		}


		switch(currentDir)
		{
		case AI_MOVE_FORWARD:
			break;
		case AI_MOVE_RIGHT:
			vMoveDir.Set(vMoveDir.y, -vMoveDir.x, 0.0f);
			break;
		case AI_MOVE_LEFT:
			vMoveDir.Set(-vMoveDir.y, vMoveDir.x, 0.0f);
			break;
		case AI_MOVE_BACKLEFT:
			vMoveDir = vMoveDir.GetRotated(Vec3Constants<float>::fVec3_OneZ,gf_PI/4);
			break;
		case AI_MOVE_BACKRIGHT:
			vMoveDir = vMoveDir.GetRotated(Vec3Constants<float>::fVec3_OneZ,-gf_PI/4);
			break;
		case AI_MOVE_TOWARDS_GROUP:
			{
				// Average direction from the target towards the group.
				vMoveDir.Set(0,0,0);
				int	groupId = pPipeUser->GetGroupId();
				CAISystem::AIObjects::iterator it = GetAISystem()->m_mapGroups.find(groupId);
				CAISystem::AIObjects::iterator end = GetAISystem()->m_mapGroups.end();
				for (; it != end && it->first == groupId; ++it)
				{
					CAIObject* pObject = it->second.GetAIObject();
					if (!pObject || !pObject->IsEnabled() || pObject->GetType() != AIOBJECT_PUPPET)
						continue;
					vMoveDir += pObject->GetPos() - tgpos;
				}
				if (!pPipeUser->IsUsing3DNavigation())
					vMoveDir.z = 0.0f;
			}
			break;
		case AI_MOVE_BACKWARD:
		default://
			vMoveDir = -vMoveDir;
			break;
		}
		vMoveDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);
		vMoveDir *= m_fDistance;
		Vec3 backoffPos = avoidPos + vMoveDir;

		// do some distance adjustment for slopes, backoff along terrain
		if(m_bCheckSlopeDistance)
		{
			unsigned thisNodeIndex = gAIEnv.pGraph->GetEnclosing(backoffPos,
				pPipeUser->m_movementAbility.pathfindingProperties.navCapMask,
				pPipeUser->m_Parameters.m_fPassRadius,
				pPipeUser->m_lastNavNodeIndex, 0.0f, 0, false, GetNameSafe(pPipeUser));
			pPipeUser->m_lastNavNodeIndex = thisNodeIndex;

			GraphNode* pThisNode = gAIEnv.pGraph->GetNodeManager().GetNode(thisNodeIndex);
			if (pThisNode && (pThisNode->navType & IAISystem::NAV_TRIANGULAR))
			{
				float terrainLevel = gEnv->p3DEngine->GetTerrainElevation(backoffPos.x, backoffPos.y);
				Vec3 adjustedBackoffPos(backoffPos.x,backoffPos.y,terrainLevel);
				Vec3 dirOnTerrain( adjustedBackoffPos - avoidPos );

				float distOnTerrain = dirOnTerrain.GetLength();
				if(distOnTerrain> 0 )
				{
					vMoveDir = dirOnTerrain * m_fDistance / distOnTerrain;
					backoffPos = avoidPos + vMoveDir;
				}
			}
		}

		m_moveStart = mypos;
		m_moveEnd = backoffPos;
		
		Vec3 offset(0,0,0.5f);
		GetAISystem()->AddDebugSphere(m_moveStart+offset, 0.1f, 0,255,255, 5);
		GetAISystem()->AddDebugSphere(backoffPos+offset, 0.2f, 0,255,255, 5);
		GetAISystem()->AddDebugLine(m_moveStart+offset, backoffPos+offset, 0,255,255, 5);

		// (MATT) Really crucial part! {2009/02/04}
		// create a dummy object to pathfind towards
		if (m_refBackoffPoint.IsNil())
		{
			GetAISystem()->CreateDummyObject( m_refBackoffPoint, string(GetNameSafe(pPipeUser)) + "_BackoffPoint" );
			m_refBackoffPoint.GetAIObject()->SetPos(backoffPos);
		}

		if(m_bLookForward )
			m_looseAttentionId = pPipeUser->SetLooseAttentionTarget(m_refBackoffPoint);

				// start pathfinding
		CAIObject *pBackoffPoint = m_refBackoffPoint.GetAIObject();
		m_pPathfindDirective = new COPPathFind("Backoff", pBackoffPoint, std::numeric_limits<float>::max(), 0.0f, false, m_fDistance);
		pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;
		if (m_pPathfindDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
		{
			if (pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
			{
				ResetNavigation(pPipeUser);	
				return eGOR_IN_PROGRESS;// check next direction
			}
		}
    return eGOR_IN_PROGRESS;
  } // !m_pPathfindDirective

  if (pPipeUser->m_nPathDecision==PATHFINDER_PATHFOUND)
  {
		if(m_fMinDistance>0)
		{
			float dist = Distance::Point_Point(pPipeUser->GetPos(),pPipeUser->m_OrigPath.GetLastPathPos());
			if(dist<m_fMinDistance)
			{
				if(m_fMaxDistanceFound < dist)
				{
					m_fMaxDistanceFound = dist;
					CAIObject *pBackoffPoint = m_refBackoffPoint.GetAIObject();
					m_vBestFailedBackoffPos = pBackoffPoint->GetPos();
				}
				ResetNavigation(pPipeUser);	
				return eGOR_IN_PROGRESS;// check next direction
			}
		}
    if (!m_pTraceDirective)
    {
      m_pTraceDirective = new COPTrace(false, defaultTraceEndAccuracy);
      pPipeUser->m_Path.GetParams().precalculatedPath = true; // prevent path regeneration
      pPipeUser->SetSignal(0,"OnPathFound",NULL, 0, gAIEnv.SignalCRCs.m_nOnPathFound);
      m_fInitTime = GetAISystem()->GetFrameStartTime();
    }

    if(m_fDuration>0 && (GetAISystem()->GetFrameStartTime() - m_fInitTime).GetSeconds() > m_fDuration)
    {
      Reset(pPipeUser);
      return eGOR_SUCCEEDED;
    }

    // keep tracing
    EGoalOpResult done = m_pTraceDirective->Execute(pPipeUser);
    if (done != eGOR_IN_PROGRESS)
    {
      Reset(pPipeUser);
      return done;
    }
    // If this goal gets reseted during m_pTraceDirective->Execute it means that
    // a smart object was used for navigation which inserts a goal pipe which
    // does Reset on this goal which sets m_pTraceDirective to NULL! In this case
    // we should just report that this goal pipe isn't finished since it will be
    // reexecuted after finishing the inserted goal pipe
    if ( !m_pTraceDirective )
      return eGOR_IN_PROGRESS;
  }
  else if (pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
  {
		if(m_bTryingLessThanMinDistance)
			pPipeUser->SetSignal(1,"OnBackOffFailed",0, 0, gAIEnv.SignalCRCs.m_nOnBackOffFailed);	

		//redo next time for next direction...
		if(m_bTryingLessThanMinDistance)
		{
			ResetNavigation(pPipeUser); 
			return eGOR_IN_PROGRESS;
		}
		else
		{//... unless the less-than-min distance path has already been tried
			Reset(pPipeUser);	
			return eGOR_FAILED;
		}

  }
  else
  {
    m_pPathfindDirective->Execute(pPipeUser);
  }

  return eGOR_IN_PROGRESS;
}

void COPBackoff::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{

	ser.BeginGroup("COPBackoff");
	{
		ser.Value("m_bUseTargetPosition",m_bUseTargetPosition);
		ser.Value("m_iDirection",m_iDirection);
		ser.Value("m_iCurrentDirectionIndex",m_iCurrentDirectionIndex);
		objectTracker.SerializeValueContainer(ser,"m_MoveDirections",m_MoveDirections);
		ser.Value("m_fDistance",m_fDistance);
		ser.Value("m_fDuration",m_fDuration);
		ser.Value("m_fInitTime",m_fInitTime);
		ser.Value("m_fLastUpdateTime",m_fLastUpdateTime);
		ser.Value("m_bUseLastOp",m_bUseLastOp);
		ser.Value("m_bLookForward",m_bLookForward);
		ser.Value("m_moveStart",m_moveStart);
		ser.Value("m_moveEnd",m_moveEnd);
		ser.Value("m_currentDirMask",m_currentDirMask);
		ser.Value("m_fMinDistance",m_fMinDistance);
		ser.Value("m_vBestFailedBackoffPos",m_vBestFailedBackoffPos);
		ser.Value("m_fMaxDistanceFound",m_fMaxDistanceFound);
		ser.Value("m_bTryingLessThanMinDistance",m_bTryingLessThanMinDistance);
		ser.Value("m_looseAttentionId",m_looseAttentionId);
		ser.Value("m_bRandomOrder",m_bRandomOrder);

		m_refBackoffPoint.Serialize(ser,"m_refBackoffPoint");
		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("PathFindDirective", m_pPathfindDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			SAFE_DELETE(m_pPathfindDirective);
			if(ser.BeginOptionalGroup("PathFindDirective", true))
			{
				m_pPathfindDirective = new COPPathFind("");
				m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		ser.EndGroup();

	}
}

void COPBackoff::DebugDraw(CPipeUser* pPipeUser) const
{
	Vec3	dir(m_moveEnd - m_moveStart);
	dir.NormalizeSafe();
	CDebugDrawContext dc;
	ColorB color(255, 255, 255);
	dc->DrawLine(m_moveStart + Vec3(0, 0, 0.25f), color, m_moveEnd + Vec3(0, 0, 0.25f), color);
	dc->DrawCone(m_moveEnd + Vec3(0, 0, 0.25f), dir, 0.3f, 0.8f, color);
}

COPTimeout::COPTimeout(float intervalMin, float intervalMax)
{
	m_fIntervalMin = intervalMin;
	m_fIntervalMax = (intervalMax > 0) ? intervalMax : m_fIntervalMin;
	Reset(0);
}

COPTimeout::COPTimeout(const XmlNodeRef& node) :
	m_fIntervalMin(0.f),
	m_fIntervalMax(-1.f)
{
	if (!node->getAttr("interval", m_fIntervalMin))
	{
		s_xml.GetMandatory(node, "intervalMin", m_fIntervalMin);
	}
	
	node->getAttr("intervalMax", m_fIntervalMax);
	if (m_fIntervalMax <= 0)
	{
		m_fIntervalMax = m_fIntervalMin;
	}

	Reset(0);
}

COPTimeout::~COPTimeout()
{
}

void COPTimeout::Reset(CPipeUser* pPipeUser)
{
	m_fCurrentInterval = m_fIntervalMin + (m_fIntervalMax - m_fIntervalMin) * ((float)ai_rand() / (float)RAND_MAX);
	m_startTime.SetSeconds(0.0f);
}

EGoalOpResult COPTimeout::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CTimeValue time = GetAISystem()->GetFrameStartTime();
	if( m_startTime.GetSeconds() < 0.001f )
		m_startTime = time;
	
	CTimeValue timeElapsed = time - m_startTime;
	if( timeElapsed.GetSeconds() > m_fCurrentInterval )
		return eGOR_DONE;

	return eGOR_IN_PROGRESS;
}


COPStrafe::COPStrafe(float distanceStart, float distanceEnd, bool strafeWhileMoving)
{
	m_fDistanceStart = distanceStart;
	m_fDistanceEnd = distanceEnd;
	m_bStrafeWhileMoving = strafeWhileMoving;
}

COPStrafe::COPStrafe(const XmlNodeRef& node) :
	m_fDistanceStart(0.f),
	m_bStrafeWhileMoving(s_xml.GetBool(node, "strafeWhileMoving"))
{
	if (!node->getAttr("distance", m_fDistanceStart))
	{
		node->getAttr("distanceStart", m_fDistanceStart);
	}
	if (!node->getAttr("distanceEnd", m_fDistanceEnd))
	{
		m_fDistanceEnd = m_fDistanceStart;
	}
}

COPStrafe::~COPStrafe()
{
}

EGoalOpResult COPStrafe::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CPuppet *pPuppet = pPipeUser->CastToCPuppet();
	if(pPuppet)
		pPuppet->SetAllowedStrafeDistances(m_fDistanceStart, m_fDistanceEnd, m_bStrafeWhileMoving);
	return eGOR_DONE;
}

COPFireCmd::COPFireCmd(int firemode, bool useLastOpResult, float intervalMin, float intervalMax):
m_command((EFireMode)firemode),
m_bUseLastOpResult(useLastOpResult),
m_fIntervalMin(intervalMin),
m_fIntervalMax(intervalMax),
m_fCurrentInterval(-1.f)
{
	m_startTime.SetSeconds(0.0f);
}

COPFireCmd::COPFireCmd(const XmlNodeRef& node) :
	m_bUseLastOpResult(s_xml.GetBool(node, "useLastOp")),
	m_fIntervalMin(-1.f),
	m_fIntervalMax(-1.f),
	m_fCurrentInterval(-1.f)
{
	s_xml.GetFireMode(node, "mode", m_command);

	if (!node->getAttr("timeout", m_fIntervalMin))
	{
		node->getAttr("timeoutMin", m_fIntervalMin);
	}
	
	node->getAttr("timeoutMax", m_fIntervalMax);

	m_startTime.SetSeconds(0.f);
}

COPFireCmd::~COPFireCmd()
{
}

void  COPFireCmd::Reset(CPipeUser* pPipeUser)
{
	m_fCurrentInterval = -1.f;
	m_startTime.SetSeconds(0.0f);
}

EGoalOpResult COPFireCmd::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPFireCmd_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CTimeValue time = GetAISystem()->GetFrameStartTime();
	// if this is a first time
	if( m_startTime.GetSeconds() < 0.001f )
	{
		m_startTime = time;
		if(m_fIntervalMin<0.f)
			m_fCurrentInterval = -1.f;
		else if(m_fIntervalMax>0.f)
			m_fCurrentInterval = Random(m_fIntervalMin, m_fIntervalMax);
		else
			m_fCurrentInterval = m_fIntervalMin;
		pPipeUser->SetFireMode(m_command);
		if (m_bUseLastOpResult)
			pPipeUser->SetFireTarget(pPipeUser->m_refLastOpResult);
		else
			pPipeUser->SetFireTarget(NILREF);
	}

	CTimeValue timeElapsed = time - m_startTime;
	if(m_fCurrentInterval<0.f || timeElapsed.GetSeconds()>m_fCurrentInterval)
	{
		// stop firing if was timed
		if(m_fCurrentInterval>0.f)
			pPipeUser->SetFireMode(FIREMODE_OFF);
		Reset(pPipeUser);
		return eGOR_DONE;
	}

	return eGOR_IN_PROGRESS;

//	pPipeUser->SetFireMode(m_command, m_bUseLastOpResult);
//	return true;
}


COPBodyCmd::COPBodyCmd(EStance bodypos, bool delayed):
	m_nBodyState(bodypos),
	m_bDelayed(delayed)
{
}

COPBodyCmd::COPBodyCmd(const XmlNodeRef& node) :
	m_nBodyState(STANCE_NULL),
	m_bDelayed(s_xml.GetBool(node, "delayed"))
{
	s_xml.GetStance(node, "id", m_nBodyState, CGoalOpXMLReader::MANDATORY);
}

EGoalOpResult COPBodyCmd::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CPuppet* pPuppet = pPipeUser->CastToCPuppet();
	if (m_bDelayed)
	{
		if (pPuppet)
			pPuppet->SetDelayedStance(m_nBodyState);
	}
	else
{
	pPipeUser->m_State.bodystate = m_nBodyState;
		if (pPuppet)
			pPuppet->SetDelayedStance(STANCE_NULL);
	}

	return eGOR_DONE;
}


COPRunCmd::COPRunCmd(float maxUrgency, float minUrgency, float scaleDownPathLength) :
	m_fMaxUrgency(ConvertUrgency(maxUrgency)),
	m_fMinUrgency(ConvertUrgency(minUrgency)),
	m_fScaleDownPathLength(scaleDownPathLength)
{
}

COPRunCmd::COPRunCmd(const XmlNodeRef& node) :
	m_fMaxUrgency(0.f),
	m_fMinUrgency(0.f),
	m_fScaleDownPathLength(0.f)
{
	if (!s_xml.GetUrgency(node, "urgency", m_fMaxUrgency))
	{
		if (!s_xml.GetUrgency(node, "maxUrgency", m_fMaxUrgency))
		{
			s_xml.GetUrgency(node, "id", m_fMaxUrgency);
		}
	}

	if ((gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS) || (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2))
	{
		s_xml.GetUrgency(node, "minUrgency", m_fMinUrgency);
		node->getAttr("scaleDownPathLength", m_fScaleDownPathLength);
	}
	
	m_fMaxUrgency = ConvertUrgency(m_fMaxUrgency);
	m_fMinUrgency = ConvertUrgency(m_fMinUrgency);
}

float COPRunCmd::ConvertUrgency(float speed)
{
  // does NOT match CFlowNode_AIBase<TDerived, TBlocking>::SetSpeed
	if (speed < -4.5f )
		return -AISPEED_SPRINT;
	else if (speed < -3.5f )
		return -AISPEED_RUN;
	else if (speed < -2.5f )
		return -AISPEED_WALK;
	else if (speed < -1.5f )
		return -AISPEED_SLOW;
	else if (speed < -0.5f )
		return AISPEED_SLOW;
	else if (speed <= 0.5f)
		return AISPEED_WALK;
	else if (speed < 1.5f)
		return AISPEED_RUN;
  else 
		return AISPEED_SPRINT;
}

EGoalOpResult COPRunCmd::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	pPipeUser->m_State.fMovementUrgency = m_fMaxUrgency;

	if (m_fScaleDownPathLength > 0.00001f)
	{
		// Set adaptive
		CPuppet *pPuppet = pPipeUser->CastToCPuppet();
		if (pPuppet)
			pPuppet->SetAdaptiveMovementUrgency(m_fMinUrgency, m_fMaxUrgency, m_fScaleDownPathLength);
	}
	else
{
		// Reset adaptive
		CPuppet *pPuppet = pPipeUser->CastToCPuppet();
		if (pPuppet)
			pPuppet->SetAdaptiveMovementUrgency(0,0,0);
	}

	return eGOR_DONE;
}

// todo luc fixme
COPLookAt::COPLookAt(float startangle, float endangle, int mode, bool bBodyTurn, bool bUseLastOp) 
{
	m_fStartAngle = startangle; 
	m_fEndAngle = endangle;
	m_bUseLastOp = bUseLastOp;
	m_bContinuous = (mode & AI_LOOKAT_CONTINUOUS)!=0;
	m_bUseBodyDir = (mode & AI_LOOKAT_USE_BODYDIR)!=0;
	//m_looseAttentionId = 0;
	m_bInitialized = false;

	m_eLookStyle = (bBodyTurn ? LOOKSTYLE_SOFT : LOOKSTYLE_SOFT_NOLOWER);
}

COPLookAt::COPLookAt(const XmlNodeRef& node) :
	m_fStartAngle(0.f),
	m_fEndAngle(0.f),
	m_bUseLastOp(s_xml.GetBool(node, "useLastOp")),
	m_bContinuous(s_xml.GetBool(node, "continuous")),
	m_bUseBodyDir(s_xml.GetBool(node, "useBodyDir")),
	m_eLookStyle(s_xml.GetBool(node, "bodyTurn", true) ? LOOKSTYLE_SOFT : LOOKSTYLE_SOFT_NOLOWER)
{
	node->getAttr("startAngle", m_fStartAngle);
	node->getAttr("endAngle", m_fEndAngle);
}

COPLookAt::~COPLookAt()
{
}
void COPLookAt::ResetLooseTarget(CPipeUser* pPipeUser, bool bForceReset)
{
	if(bForceReset || !m_bContinuous)
	{
		pPipeUser->SetLookStyle(LOOKSTYLE_DEFAULT);
		pPipeUser->SetLooseAttentionTarget(NILREF);//, (bForceReset || m_bContinuous? -1 :m_looseAttentionId));
		//m_looseAttentionId = 0;
	}
}
void COPLookAt::Reset(CPipeUser* pPipeUser)
{
	ResetLooseTarget(pPipeUser);
	m_bInitialized = false;
}

EGoalOpResult COPLookAt::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPLookAt_Execute)
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if(m_fStartAngle<-450.0f)	// stop looking at - disable looseAttentionTarget
	{
		ResetLooseTarget(pPipeUser,true);
		return eGOR_SUCCEEDED;
	}

	// first time	
	//if (!m_looseAttentionId)
	if (!m_bInitialized)
	{ 
		m_bInitialized = true;
		if ( !m_fStartAngle && !m_fEndAngle && !m_bUseLastOp )
		{
			//			Reset(pPipeUser);
			return eGOR_SUCCEEDED;
		}
		
		pPipeUser->SetLookStyle(m_eLookStyle);

		Vec3 mypos = pPipeUser->GetPos();
		Vec3 myangles = pPipeUser->GetMoveDir();
		
		if ( (m_fEndAngle == 0 ) && (m_fStartAngle==0) )
		{
			CAIObject *pOrient;	// the guy who will provide our orientation
			CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();
			if (pPipeUser->GetAttentionTarget() && !m_bUseLastOp)
				pOrient = static_cast<CAIObject*>(pPipeUser->GetAttentionTarget());
			else if (pLastOpResult)
				pOrient = pLastOpResult;
			else
			{
				Reset(pPipeUser);
				return eGOR_FAILED;	// sorry no target and no last operation target
			}

			CWeakRef<CAIObject> refOrient = GetWeakRef(pOrient);
			pPipeUser->SetLooseAttentionTarget(refOrient); // m_looseAttentionId =
			if(m_bContinuous) // will keep the look at target after this goalop ends
			{
				Reset(pPipeUser);// Luc 1-aug-07: this reset was missing and it broke non continuous lookat with looping goalpipes (never finishing in the second loop)
			return eGOR_SUCCEEDED;
			} // else, wait for orienting like the loose att target
		}
		else
		{
			// lets place it at a random spot around the operand
			myangles.z = 0;
	
			float fi = (((float)(ai_rand() & 255) / 255.0f ) * (float)fabs(m_fEndAngle - m_fStartAngle));
			fi+=m_fStartAngle;
				
			Matrix33 m = Matrix33::CreateRotationXYZ( DEG2RAD(Ang3(0,0,fi)) ); 
			myangles = m * (Vec3)myangles;
		myangles*=20.f;
		Vec3 pos = mypos+(Vec3)myangles;
			/*m_looseAttentionId =*/ pPipeUser->SetLooseAttentionTarget(pos);//use operand LookAtTarget
		}

	}
	else	// keep on looking, orient like the loose att target
	{
		if (m_bContinuous)
			return eGOR_SUCCEEDED;
	Vec3 mypos = pPipeUser->GetPos();
		Vec3 myDir = m_bUseBodyDir ? pPipeUser->GetBodyDir() : pPipeUser->GetViewDir();
		Vec3 otherpos = pPipeUser->GetLooseAttentionPos();
		if(otherpos.IsZero(1.f) && pPipeUser->GetAttentionTarget())
			otherpos = pPipeUser->GetAttentionTarget()->GetPos();

		Vec3 otherDir = otherpos - mypos;
		if(m_bUseBodyDir && !pPipeUser->GetMovementAbility().b3DMove)
		{
			myDir.z =0;
			myDir.NormalizeSafe();
			otherDir.z = 0;
		}
		otherDir.Normalize();

		float f = myDir.Dot(otherDir);
		if ((f > 0.98f && !m_bContinuous) )
		{
			Reset(pPipeUser);
			return eGOR_SUCCEEDED;
		}
	}
	return eGOR_IN_PROGRESS;
}

//
//----------------------------------------------------------------------------------------------------------
void COPLookAt::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{

	ser.BeginGroup("COPLookAt");
	{
		ser.Value("m_fStartAngle",m_fStartAngle);
		ser.Value("m_fEndAngle",m_fEndAngle);
		ser.Value("m_fLastDot",m_fLastDot);
		ser.Value("m_bUseLastOp",m_bUseLastOp);
		ser.Value("m_bContinuous",m_bContinuous);
		ser.Value("m_bUseBodyDir",m_bUseBodyDir);
		//ser.Value("m_looseAttentionId",m_looseAttentionId);
		ser.Value("m_bInitialized",m_bInitialized);

		ser.EndGroup();
	}

}

//
//----------------------------------------------------------------------------------------------------------
COPLookAround::COPLookAround(float lookAtRange, float scanIntervalRange, float intervalMin, float intervalMax, bool bBodyTurn, bool breakOnLiveTarget, bool bUseLastOp, bool checkForObstacles) :
	m_fLookAroundRange(lookAtRange),
	m_fTimeOut(0.0f),
	m_fScanTimeOut(0.0f),
	m_fScanIntervalRange(scanIntervalRange),
	m_fIntervalMin(intervalMin),
	m_fIntervalMax(intervalMax),
	m_breakOnLiveTarget(breakOnLiveTarget),
	m_useLastOp(bUseLastOp),
	m_lookAngle(0.0f),
	m_lookZOffset(0.0f),
	m_lastLookAngle(0.0f),
	m_lastLookZOffset(0.0f),
	m_bInitialized(false),
	m_looseAttentionId(0),
	m_initialDir(1,0,0),
	m_checkForObstacles(checkForObstacles)
{
	if(m_fIntervalMax < m_fIntervalMin) m_fIntervalMax = m_fIntervalMin;
	m_fTimeOut = m_fIntervalMin + (m_fIntervalMax - m_fIntervalMin) * ai_frand();
	m_fScanTimeOut = m_fScanIntervalRange + (0.2f + ai_frand() * 0.8f);

	m_eLookStyle = (bBodyTurn ? LOOKSTYLE_SOFT : LOOKSTYLE_SOFT_NOLOWER);
}

COPLookAround::COPLookAround(const XmlNodeRef& node) :
	m_fLookAroundRange(0.f),
	m_fTimeOut(0.f),
	m_fScanTimeOut(0.f),
	m_fScanIntervalRange(-1.f),
	m_fIntervalMin(-1.f),
	m_fIntervalMax(-1.f),
	m_breakOnLiveTarget(s_xml.GetBool(node, "breakOnLiveTarget")),
	m_useLastOp(s_xml.GetBool(node, "useLastOp")),
	m_lookAngle(0.f),
	m_lookZOffset(0.f),
	m_lastLookAngle(0.f),
	m_lastLookZOffset(0.f),
	m_bInitialized(false),
	m_looseAttentionId(0),
	m_initialDir(1.f, 0.f, 0.f),
	m_eLookStyle(s_xml.GetBool(node, "bodyTurn", true) ? LOOKSTYLE_SOFT : LOOKSTYLE_SOFT_NOLOWER),
	m_checkForObstacles(false)
{
	s_xml.GetMandatory(node, "lookAroundRange", m_fLookAroundRange);
	node->getAttr("scanRange", m_fScanIntervalRange);
	
	if (node->getAttr("interval", m_fIntervalMin))
	{
		m_fIntervalMax = m_fIntervalMin;
	}
	node->getAttr("intervalMin", m_fIntervalMin);
	node->getAttr("intervalMax", m_fIntervalMax);
	if (m_fIntervalMax < m_fIntervalMin)
	{
		m_fIntervalMax = m_fIntervalMin;
	}
	
	m_fTimeOut = m_fIntervalMin + (m_fIntervalMax - m_fIntervalMin) * ai_frand();
	m_fScanTimeOut = m_fScanIntervalRange + (0.2f + ai_frand() * 0.8f);

	node->getAttr("checkForObstacles", m_checkForObstacles);
}

COPLookAround::~COPLookAround()
{
}

void COPLookAround::UpdateLookAtTarget(CPipeUser* pPipeUser)
{
	m_lastLookAngle = m_lookAngle;
	m_lastLookZOffset = m_lookZOffset;

	if(!m_checkForObstacles)
	{
		SetRandomLookDir();
	}
	else	
	{
		const Vec3& opPos = pPipeUser->GetPos();
		Vec3	dir;

		int limitRays = 4;
		for(uint32 i = 0; i < limitRays; ++i)
		{
			SetRandomLookDir();
			dir = GetLookAtDir(pPipeUser, m_lookAngle, m_lookZOffset);
			// TODO : Defer this ray cast
			if(GetAISystem()->CheckPointsVisibility(opPos, opPos + dir, 5.0f))
				break;
		}
	}
}

void COPLookAround::SetRandomLookDir()
{
	// Random angle within range.
	float	range(gf_PI);
	if(m_fLookAroundRange > 0)
		range = DEG2RAD(m_fLookAroundRange);
	range = clamp(range, 0.0f, gf_PI);
	m_lookAngle = (1.0f - ai_frand() * 2.0f) * range;
	m_lookZOffset = (1.1f * ai_frand() - 1.0f) * 3.0f;	// Look more down then up
}

Vec3 COPLookAround::GetLookAtDir(CPipeUser* pPipeUser, float angle, float dz) const
{
	Vec3 forward = m_initialDir;
	if(!pPipeUser->GetState()->vMoveDir.IsZero())
		forward = pPipeUser->GetState()->vMoveDir;
	if(m_useLastOp )
	{
		CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();
		if (pLastOpResult)
			forward = (pLastOpResult->GetPos() - pPipeUser->GetPos()).GetNormalizedSafe();
	}
	forward.z = 0.0f;
	forward.NormalizeSafe();
	Vec3 right(forward.y, -forward.x, 0.0f);

	const float lookAtDist = 20.0f;
	float	dx = (float)cry_sinf(angle) * lookAtDist;
	float	dy = (float)cry_cosf(angle) * lookAtDist;

	return right * dx + forward * dy + Vec3(0,0,dz);
}

EGoalOpResult COPLookAround::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if (!m_bInitialized)
	{
		m_initialDir = pPipeUser->GetBodyDir();
		m_bInitialized = true;
	}

	if (!m_looseAttentionId)
	{
		m_fLastDot = 10.0f;

		UpdateLookAtTarget(pPipeUser);
		Vec3	dir = GetLookAtDir(pPipeUser, m_lookAngle, m_lookZOffset);
		Vec3 pos(pPipeUser->GetPos() + dir);
		m_looseAttentionId = pPipeUser->SetLooseAttentionTarget(pos);
		m_startTime = m_scanStartTime = GetAISystem()->GetFrameStartTime();

		pPipeUser->SetLookStyle(m_eLookStyle);
	}
	else
	{
		int id = pPipeUser->GetLooseAttentionId();
		if(id && id!=m_looseAttentionId)
		{
			//something else is using the operand loose target; aborting
			Reset(pPipeUser);
			return eGOR_FAILED;
		}

		if (m_breakOnLiveTarget && pPipeUser->GetAttentionTarget() && pPipeUser->GetAttentionTarget()->IsAgent())
			{
				pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
				m_looseAttentionId = 0;
				Reset(pPipeUser);
				return eGOR_SUCCEEDED;
			}


		// (MATT) Test for possible div by 0. Unpretty but should work. {2009/04/28}
		if (pPipeUser->GetAttentionTarget() && (pPipeUser->GetAttentionTarget()->GetPos().GetDistance(pPipeUser->GetPos()) < 0.0001f))
			return eGOR_FAILED;

		ExecuteDry(pPipeUser);

		if(m_fTimeOut < 0.0f)
		{
			// If no time out is specified, we bail out once the target is reached.
			const Vec3& opPos = pPipeUser->GetPos();
			const Vec3& opViewDir = pPipeUser->GetViewDir();
			Vec3 dirToTarget = pPipeUser->GetLooseAttentionPos() - opPos;
			dirToTarget.NormalizeSafe();

			float f = opViewDir.Dot(dirToTarget);
			if(f > 0.99f || fabsf(f - m_fLastDot) < 0.001f)
			{
				Reset(pPipeUser);
				return eGOR_SUCCEEDED;
			}
			else
				m_fLastDot = f;
		}
		else
		{
			// If time out is specified, keep looking around until the time out finishes.
			float	elapsed = (GetAISystem()->GetFrameStartTime() - m_startTime).GetSeconds();
			if(elapsed > m_fTimeOut)
			{
				Reset(pPipeUser);
				return eGOR_SUCCEEDED;
			}
		}
	}

	return eGOR_IN_PROGRESS;
}

void COPLookAround::ExecuteDry(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if (!m_bInitialized)
		return;

	float	scanElapsed = (GetAISystem()->GetFrameStartTime() - m_scanStartTime).GetSeconds();

	if (pPipeUser->GetAttentionTarget())
	{
		// Interpolate the initial dir towards the current attention target slowly.
		Vec3	reqDir = pPipeUser->GetAttentionTarget()->GetPos() - pPipeUser->GetPos();
		reqDir.Normalize();

		const float maxRatePerSec = DEG2RAD(25.0f);
		const float maxRate = maxRatePerSec * GetAISystem()->GetFrameDeltaTime();
		const float thr = cosf(maxRate);

		float cosAngle = m_initialDir.Dot(reqDir);
		if (cosAngle > thr)
		{
			m_initialDir = reqDir;
		}
		else
		{
			float angle = acos_tpl(cosAngle);
			float t = 0.0f;
			if(angle != 0.0f)
				t = maxRate / angle;
			Quat	curView;
			curView.SetRotationVDir(m_initialDir);
			Quat	reqView;
			reqView.SetRotationVDir(reqDir);
			Quat	view;
			view.SetSlerp(curView, reqView, t);
			m_initialDir = view * FORWARD_DIRECTION;
		}
	}

	// Smooth transition from last value to new value if scanning, otherwise just as fast as possible.
	float	t = 1.0f;
	if (m_fScanTimeOut > 0.0f)
		t = (1.0f - cosf(clamp(scanElapsed / m_fScanTimeOut, 0.0f, 1.0f) * gf_PI)) / 2.0f;
	Vec3 dir = GetLookAtDir(pPipeUser, m_lastLookAngle + (m_lookAngle - m_lastLookAngle) * t, m_lastLookZOffset + (m_lookZOffset - m_lastLookZOffset) * t);
	Vec3 pos = pPipeUser->GetPos() + dir;

	m_looseAttentionId = pPipeUser->SetLooseAttentionTarget(pos);

			// Once one sweep is finished, start another.
			if(scanElapsed > m_fScanTimeOut)
			{
				UpdateLookAtTarget(pPipeUser);
				m_scanStartTime = GetAISystem()->GetFrameStartTime();
				m_fScanTimeOut = m_fScanIntervalRange + (0.2f + ai_frand() * 0.8f);
			}
		}

void COPLookAround::Reset(CPipeUser* pPipeUser)
{
	m_bInitialized = false;
	if(pPipeUser)
	{
		pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
		m_looseAttentionId = 0;
	}

	m_fTimeOut = m_fIntervalMin + (m_fIntervalMax - m_fIntervalMin) * ai_frand();
	m_fScanTimeOut = m_fScanIntervalRange + (0.2f + ai_frand() * 0.8f);

	m_lookAngle = 0.0f;
	m_lookZOffset = 0.0f;
	m_lastLookAngle = 0.0f;
	m_lastLookZOffset = 0.0f;

	m_fLastDot = 0;
//	m_fTimeOut = 0;
	//m_fScanTimeOut = 0;
	
	pPipeUser->SetLookStyle(LOOKSTYLE_DEFAULT);
}

//===================================================================
// DebugDraw
//===================================================================
void COPLookAround::DebugDraw(CPipeUser* pPipeUser) const
{
	float	scanElapsed = (GetAISystem()->GetFrameStartTime() - m_scanStartTime).GetSeconds();
	float	t(1.0f);
	if(m_fScanTimeOut > 0.0f)
		t = (1.0f - cosf(clamp(scanElapsed / m_fScanTimeOut, 0.0f, 1.0f) * gf_PI)) / 2.0f;
	Vec3	dir = GetLookAtDir(pPipeUser, m_lastLookAngle + (m_lookAngle - m_lastLookAngle) * t, m_lastLookZOffset + (m_lookZOffset - m_lastLookZOffset) * t);

	CDebugDrawContext dc;
	
	Vec3 pos = pPipeUser->GetPos() - Vec3(0,0,1);

	// Initial dir
	dc->DrawLine(pos, ColorB(255, 255, 255), pos + m_initialDir * 3.0f, ColorB(255, 255, 255, 128));

	// Current dir
	dc->DrawLine(pos, ColorB(255, 0, 0), pos + dir * 3.0f, ColorB(255, 0, 0, 128), 3.0f);
}


void COPLookAround::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{

	ser.BeginGroup("COPLookAround");
	{
		ser.Value("m_fLastDot",m_fLastDot);
		ser.Value("m_fLookAroundRange",m_fLookAroundRange);
		ser.Value("m_fIntervalMin",m_fIntervalMin);
		ser.Value("m_fIntervalMax",m_fIntervalMax);
		ser.Value("m_fTimeOut",m_fTimeOut);
		ser.Value("m_fScanIntervalRange",m_fScanIntervalRange);
		ser.Value("m_fScanTimeOut",m_fScanTimeOut);
		ser.Value("m_startTime",m_startTime);
		ser.Value("m_scanStartTime",m_scanStartTime);
		ser.Value("m_breakOnLiveTarget",m_breakOnLiveTarget);
		ser.Value("m_useLastOp",m_useLastOp);
		ser.Value("m_lookAngle",m_lookAngle);
		ser.Value("m_lookZOffset",m_lookZOffset);
		ser.Value("m_lastLookAngle",m_lastLookAngle);
		ser.Value("m_lastLookZOffset",m_lastLookZOffset);
		ser.Value("m_looseAttentionId",m_looseAttentionId);

		ser.EndGroup();
	}

}
//
//-------------------------------------------------------------------------------------------------------------------------------
COPPathFind::COPPathFind(const char *szName, IAIObject *pTarget, float fEndTol, float fEndDistance, bool bKeepMoving, float fDirectionOnlyDistance):
m_bWaitingForResult(false),
m_sObjectName(szName),
m_bKeepMoving(bKeepMoving),
m_fDirectionOnlyDistance(fDirectionOnlyDistance),
m_nForceTargetBuildingID(-1),
m_fEndTol(fEndTol),
	m_fEndDistance(fEndDistance),
m_TargetPos(ZERO),
m_TargetOffset(ZERO)
{ 
	// Inside the AI system, we know that all IAIObjects are CAIObjects
	m_refTarget = GetWeakRef((CAIObject*)pTarget);
}

//
//-------------------------------------------------------------------------------------------------------------------------------
COPPathFind::COPPathFind(const XmlNodeRef& node) :
	m_sObjectName(s_xml.GetMandatoryString(node, "name")),
	m_bWaitingForResult(false),
	m_bKeepMoving(s_xml.GetBool(node, "keepMoving")),
	m_fDirectionOnlyDistance(0.f),
	m_nForceTargetBuildingID(-1),
	m_fEndTol(0.f),
	m_fEndDistance(0.f),
	m_TargetPos(ZERO),
	m_TargetOffset(ZERO)
{
	if (!m_sObjectName.empty())
	{
		m_refTarget = GetWeakRef(GetAISystem()->GetAIObjectByName(m_sObjectName.c_str()));
	}
}

//
//-------------------------------------------------------------------------------------------------------------------------------
COPPathFind::~COPPathFind() 
{ 
}

//
//-------------------------------------------------------------------------------------------------------------------------------
void COPPathFind::Reset(CPipeUser* pPipeUser)
{
	CCCPOINT(COPPathFind_Reset);

	m_bWaitingForResult = false;
	m_nForceTargetBuildingID = -1;
	// (MATT) Note that apparently we don't reset the target here {2009/02/17}
}

//
//-------------------------------------------------------------------------------------------------------------------------------
EGoalOpResult COPPathFind::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPPathFind_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);


	// (MATT) When it comes to calls to the pathfinder, this seems redundant as hell {2009/01/29}

	if (!m_bWaitingForResult)
	{
		if (m_sObjectName.size()>0)
		{
			CAIObject* pSpecialTarget = pPipeUser->GetSpecialAIObject(m_sObjectName);
			if(pSpecialTarget)
			{
				CCCPOINT(COPPathFind_Execute_SpecialTarget);

				// else just issue request to pathfind to target
				m_TargetPos = pSpecialTarget->GetPos();
        m_TargetPos += m_TargetOffset;
				Vec3 opdir = pSpecialTarget->GetMoveDir();
        if (m_fDirectionOnlyDistance > 0.0f)
        {
          pPipeUser->RequestPathInDirection(m_TargetPos, m_fDirectionOnlyDistance, NILREF, m_fEndDistance);
        }
        else
        {
					pPipeUser->RequestPathTo(m_TargetPos, opdir, pSpecialTarget->GetSubType()!=CAIObject::STP_FORMATION,
						m_nForceTargetBuildingID, m_fEndTol, m_fEndDistance, pSpecialTarget);
        }
				m_bWaitingForResult = true;
			}
		}
	}

	CAIObject * const pTarget = m_refTarget.GetAIObject();
	if (!m_bWaitingForResult)
	{
		if (!pTarget)
		{
			// it target not specified, use last op result
			CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();
			if  (!pLastOpResult)
			{
				pPipeUser->SetSignal(0,"OnNoPathFound" ,0, 0, gAIEnv.SignalCRCs.m_nOnNoPathFound);
				pPipeUser->m_nPathDecision = PATHFINDER_NOPATH;
				Reset(pPipeUser);
				return eGOR_FAILED;	// no last op result, return...
			}
			m_TargetPos = pLastOpResult->GetPos();
      m_TargetPos += m_TargetOffset;
			Vec3 opdir = pLastOpResult->GetMoveDir();

			CCCPOINT(COPPathFind_Execute_LastOp);

      if (m_fDirectionOnlyDistance > 0.0f)
      {
        pPipeUser->RequestPathInDirection(m_TargetPos, m_fDirectionOnlyDistance, NILREF, m_fEndDistance);
      }
      else
      {
				pPipeUser->RequestPathTo(m_TargetPos, opdir, pLastOpResult->GetSubType()!=CAIObject::STP_FORMATION,
					m_nForceTargetBuildingID, m_fEndTol, m_fEndDistance, pLastOpResult);
      }
			m_bWaitingForResult = true;
		}
		else
		{
			// else just issue request to pathfind to target
			m_TargetPos = pTarget->GetPos();
      m_TargetPos += m_TargetOffset;
			Vec3 opdir = pTarget->GetMoveDir();

			CCCPOINT(COPPathFind_Execute_Target);

      if (m_fDirectionOnlyDistance > 0.0f)
      {
        pPipeUser->RequestPathInDirection(m_TargetPos, m_fDirectionOnlyDistance, NILREF, m_fEndDistance);
      }
      else
      {
				pPipeUser->RequestPathTo(m_TargetPos, opdir, pTarget->GetSubType()!=CAIObject::STP_FORMATION,
					m_nForceTargetBuildingID, m_fEndTol, m_fEndDistance, pTarget);
      }
			m_bWaitingForResult = true;
		}
	}

	if (m_bWaitingForResult)
	{
		if (pPipeUser->m_nPathDecision)
		{
			m_bWaitingForResult = false;
			if(pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)// || !pPipeUser->m_Path.GetPathEndIsAsRequested())
			{
				pPipeUser->SetSignal(0,"OnNoPathFound" ,0, 0, gAIEnv.SignalCRCs.m_nOnNoPathFound);

				Reset(pPipeUser);
				return eGOR_FAILED;
			}
			else
			{
				if(pPipeUser->m_Path.GetPath().empty())
          pPipeUser->SetSignal(0,"OnNoPathFound");
        // if the path is found, but the end is far from what's requested, then
        // approach/stick will send an OnEndPathOffset signal if necessary
		
			Reset(pPipeUser);
				return eGOR_SUCCEEDED;
			}
		}
	}
	return eGOR_IN_PROGRESS;
}



void COPPathFind::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPPathFind");
	{
		ser.Value("m_sObjectName",m_sObjectName);
		ser.Value("m_bWaitingForResult",m_bWaitingForResult);
		ser.Value("m_bKeepMoving",m_bKeepMoving);
		ser.Value("m_TargetPos",m_TargetPos);
		ser.Value("m_fDirectionOnlyDistance",m_fDirectionOnlyDistance);
		ser.Value("m_nForceTargetBuildingID",m_nForceTargetBuildingID);

		m_refTarget.Serialize(ser, "m_refTarget");

		ser.Value("m_fEndTol",m_fEndTol);
		ser.Value("m_TargetOffset",m_TargetOffset);
		ser.EndGroup();
	}
}

COPLocate::COPLocate(const XmlNodeRef& node) :
	m_sName(node->getAttr("name")),
	m_nObjectType(0),
	m_fRange(0.f)
{
	if (m_sName.empty())
	{
		AIWarning("Goalop 'Locate': no AI object name provided; trying to fetch id instead.");
		s_xml.GetMandatory(node, "id", m_nObjectType);
	}
	
	node->getAttr("range", m_fRange);
}

EGoalOpResult COPLocate::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPLocate_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	
	if (m_sName.empty())
	{
		pPipeUser->SetLastOpResult( GetWeakRef( static_cast<CAIObject*>(GetAISystem()->GetNearestObjectOfTypeInRange(pPipeUser->GetPos(),m_nObjectType,0,m_fRange?m_fRange:50))) );
		if (pPipeUser->GetLastOpResult())
			return eGOR_SUCCEEDED;
		else
			return eGOR_FAILED;
	}

	CAIObject* pObject = pPipeUser->GetSpecialAIObject(m_sName, m_fRange);

	// don't check the range if m_fRange doesn't really represent a range :-/
	if ( m_sName != "formation_id" )
	{
		// check with m_fRange optional parametar
		if (m_fRange && pObject && (pPipeUser->GetPos() - pObject->GetPos()).GetLengthSquared() > m_fRange*m_fRange )
			pObject = NULL;
	}

	// always set lastOpResult even if pObject is NULL!!!
	pPipeUser->SetLastOpResult( GetWeakRef(pObject) );

	CCCPOINT(COPLocate_Execute_A);

//Diesel cut
	if (!pObject)
	{
	//	gEnv->pLog->LogError("Failed to locate '%s'", m_sName.c_str());
	//	return eGOR_FAILED;
		return eGOR_NONE; //Diesel new
	}
//Diesel cut

	return eGOR_SUCCEEDED;
}


//===================================================================
// COPTrace
//===================================================================
COPTrace::COPTrace( bool bExactFollow, float fEndAccuracy, bool bForceReturnPartialPath, bool bStopOnAnimationStart, EEndMode endMode ) : 
	m_bDisabledPendingFullUpdate(false),
	m_bExactFollow(bExactFollow),
	m_fEndAccuracy(fEndAccuracy),
	m_bForceReturnPartialPath(bForceReturnPartialPath),
	m_stopOnAnimationStart(bStopOnAnimationStart),
	m_Maneuver(eMV_None),
	m_ManeuverDir(eMVD_Clockwise),
	m_EndMode(endMode),
	m_ManeuverDist(0),
	m_ManeuverTime(0.0f),
	m_landHeight(0.0f),
	m_landingDir(ZERO),
	m_landingPos(ZERO),
	m_workingLandingHeightOffset(0.0f), 
	m_TimeStep(0.1f),
	m_lastTime(-1.0f),
  m_startTime(-1.0f),
	m_fTotalTracingTime(0),
	m_lastPosition(ZERO),
  m_fTravelDist(0.0f),
	m_inhibitPathRegen(false),
	m_looseAttentionId(0),
	m_waitingForPathResult(false), 
	m_actorTargetRequester(eTATR_None), 
	m_pendingActorTargetRequester(eTATR_None),
	m_earlyPathRegen(false), 
	m_passingStraightNavSO(false),
	m_bSteerAroundPathTarget(true),
	m_bControllSpeed(true)
{
  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("COPTrace::COPTrace %p", this);
}

COPTrace::COPTrace(const XmlNodeRef& node) :
	m_bDisabledPendingFullUpdate(false),
	m_bExactFollow(s_xml.GetBool(node, "exactFollow")),
	m_fEndAccuracy(0.f),
	m_bForceReturnPartialPath(s_xml.GetBool(node, "forceReturnPartialPath")),
	m_stopOnAnimationStart(s_xml.GetBool(node, "stopOnAnimationStart")),
	m_Maneuver(eMV_None),
	m_ManeuverDir(eMVD_Clockwise),
	m_EndMode(eEM_FixedDistance),
	m_ManeuverDist(0),
	m_ManeuverTime(0.f),
	m_landHeight(0.f),
	m_landingDir(ZERO),
	m_landingPos(ZERO),
	m_workingLandingHeightOffset(0.f), 
	m_TimeStep(0.1f),
	m_lastTime(-1.f),
	m_startTime(-1.f),
	m_fTotalTracingTime(0),
	m_lastPosition(ZERO),
	m_fTravelDist(0.f),
	m_inhibitPathRegen(false),
	m_looseAttentionId(0),
	m_waitingForPathResult(false), 
	m_actorTargetRequester(eTATR_None), 
	m_pendingActorTargetRequester(eTATR_None),
	m_earlyPathRegen(false), 
	m_passingStraightNavSO(false),
	m_bSteerAroundPathTarget(true),
	m_bControllSpeed(true)
{
	node->getAttr("endAccuracy", m_fEndAccuracy);
}

//===================================================================
// ~COPTrace
//===================================================================
COPTrace::~COPTrace()
{
	CCCPOINT(COPTrace_Destructor);

	CPipeUser * const pPipeUser = m_refOperand.GetAIObject();
  if (pPipeUser)
  {
    gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);
		pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
    m_looseAttentionId = 0;
    pPipeUser->m_State.fDesiredSpeed = 0.0f;

    pPipeUser->ClearPath("COPTrace::~COPTrace m_Path");
  }

	m_refOperand.Reset();
	m_refNavTarget.Release();

  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("COPTrace::~COPTrace %p %s", this, pPipeUser ? GetNameSafe(pPipeUser) : "");
}


//===================================================================
// Reset
//===================================================================
void COPTrace::Reset(CPipeUser* pPipeUser)
{
	CCCPOINT(COPTrace_Reset);

  if (pPipeUser && gAIEnv.CVars.DebugPathFinding)
    AILogAlways("COPTrace::Reset %s", GetNameSafe(pPipeUser));

  if (pPipeUser)
  {
		CCCPOINT(COPTrace_Reset_A);

    pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
    m_looseAttentionId = 0;
		// If we have regenerated the path as part of the periodic regeneration
		// then the request must be cancelled. However, don't always cancel requests
		// because if stick is trying to regenerate the path and we're
		// resetting because we reached the end of the current one, then we'll stop
		// stick from getting its pathfind request.
		// Generally the m_Path.Empty condition decides between these cases, but
		// Danny todo: not sure if it will always work.
		if (!pPipeUser->m_Path.Empty())
    gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);
		pPipeUser->m_nPathDecision = PATHFINDER_ABORT;
    pPipeUser->m_State.fDesiredSpeed = 0.0f;
    pPipeUser->ClearPath("COPTrace::Reset m_Path");
  }

	// (MATT) Surely we should reset the stored operand here - but the dummy might best stay, if it exists already {2009/02/18}	
	m_refOperand.Reset();
	m_refNavTarget.Release();

  m_fTotalTracingTime = 0.0f;
  m_Maneuver = eMV_None;
  m_ManeuverDist = 0;
  m_ManeuverTime = GetAISystem()->GetFrameStartTime();
  m_landHeight = 0.0f;
  m_landingDir.zero();
  m_workingLandingHeightOffset = 0.0f;
  m_inhibitPathRegen = false;
  m_bDisabledPendingFullUpdate = false;
	m_actorTargetRequester = eTATR_None;
	m_pendingActorTargetRequester = eTATR_None;
	m_waitingForPathResult = false;
	m_earlyPathRegen = false;
	m_passingStraightNavSO = false;
	m_fTravelDist = 0;

  m_TimeStep =  0.1f;
  m_lastTime = -1.0f;

	if (pPipeUser)
	{
		if (m_bExactFollow )
		{
			pPipeUser->m_bLooseAttention = false;
		}
		pPipeUser->ClearInvalidatedSOLinks();
	}
}

//===================================================================
// Serialize
//===================================================================
void COPTrace::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.Value("m_bDisabledPendingFullUpdate",m_bDisabledPendingFullUpdate);
	ser.Value("m_ManeuverDist",m_ManeuverDist);
	ser.Value("m_ManeuverTime",m_ManeuverTime);
	ser.Value("m_landHeight",m_landHeight);
	ser.Value("m_workingLandingHeightOffset",m_workingLandingHeightOffset);
	ser.Value("m_landingPos",m_landingPos);
	ser.Value("m_landingDir",m_landingDir);

	ser.Value("m_bExactFollow",m_bExactFollow);
	ser.Value("m_bForceReturnPartialPath",m_bForceReturnPartialPath);
	ser.Value("m_lastPosition",m_lastPosition);
  ser.Value("m_lastTime",m_lastTime);
  ser.Value("m_startTime",m_startTime);
  ser.Value("m_fTravelDist",m_fTravelDist);
	ser.Value("m_TimeStep",m_TimeStep);

	m_refOperand.Serialize(ser, "m_refOperand");

	ser.EnumValue("m_Maneuver",m_Maneuver,eMV_None,  eMV_Fwd);
	ser.EnumValue("m_ManeuverDir",m_ManeuverDir,eMVD_Clockwise, eMVD_AntiClockwise);

	ser.Value("m_fTotalTracingTime",m_fTotalTracingTime);
	ser.Value("m_inhibitPathRegen",m_inhibitPathRegen);
	ser.Value("m_fEndAccuracy",m_fEndAccuracy);
	ser.Value("m_looseAttentionId",m_looseAttentionId);

	ser.Value("m_waitingForPathResult", m_waitingForPathResult);
	ser.Value("m_earlyPathRegen", m_earlyPathRegen);
	ser.Value("m_bSteerAroundPathTarget",m_bSteerAroundPathTarget);
	ser.Value("m_passingStraightNavSO", m_passingStraightNavSO);
	ser.EnumValue("m_actorTargetRequester", m_actorTargetRequester, eTATR_None, eTATR_EndOfPath);
	ser.EnumValue("m_pendingActorTargetRequester", m_pendingActorTargetRequester, eTATR_None, eTATR_EndOfPath);
	ser.Value("m_stopOnAnimationStart", m_stopOnAnimationStart);

	m_refNavTarget.Serialize(ser, "m_refNavTarget");
}


//===================================================================
// Execute
//===================================================================
EGoalOpResult COPTrace::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPTrace_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool done = ExecuteTrace( pPipeUser, true );

  // if waiting for a path (especially with continuous stick) continue to
  // try to follow any path that we have.
  if (pPipeUser->m_nPathDecision == PATHFINDER_STILLFINDING)
  {
    if (done)
      pPipeUser->m_State.fDesiredSpeed = 0.0f;
    return eGOR_IN_PROGRESS;
  }
  else
  {
		// Done tracing, allow to try to use invalid objects again.
		if(done)
		{
			// Kevin - Clean up residual data that is causing problems elsewhere
			pPipeUser->m_State.fDistanceToPathEnd = 0.f;

			pPipeUser->ClearInvalidatedSOLinks();
			if ( pPipeUser->m_State.curActorTargetPhase == eATP_Error )
				return eGOR_FAILED;
		}
		return done ? eGOR_SUCCEEDED : eGOR_IN_PROGRESS;
  }
}

//===================================================================
// ExecuteTrace
// this is the same old COPTrace::Execute method
// but now, smart objects should control when it
// should be executed and how its return value
// will be interpreted
//===================================================================
bool COPTrace::ExecuteTrace(CPipeUser * const pPipeUser, bool fullUpdate)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
  
	// Set our stored operand
	if (m_refOperand != pPipeUser)
		m_refOperand = GetWeakRef(pPipeUser);

	// workaround: Special case fix for drivers in fall&play
	if (pPipeUser->GetType() == AIOBJECT_VEHICLE)
	{
		// Check if the vehicle driver is tranq'd and do not move if it is.
		CPipeUser* pDriverAI = 0;
		if (EntityId driverId = pPipeUser->GetProxy()->GetLinkedDriverEntityId())
			if (IEntity* pDriverEntity = gEnv->pEntitySystem->GetEntity(driverId))
				pDriverAI = CastToCPipeUserSafe(pDriverEntity->GetAI());

		if (pDriverAI && pDriverAI->GetProxy())
		{
			if (pDriverAI->GetProxy()->GetActorIsFallen())
			{
				// The driver is unable to drive, do not drive.
				pPipeUser->m_State.vMoveDir.Set(0,0,0);
				pPipeUser->m_State.fDesiredSpeed = 0.0f;
				pPipeUser->m_State.predictedCharacterStates.nStates = 0;
				return false; // Trace not finished.
			}
		}
	}

	// Wait for pathfinder to return a result.
	if(m_waitingForPathResult)
	{
		if(pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND)
		{
			// Path found, continue.
			m_waitingForPathResult = false;
		}
		else if(pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
		{
			// Could not find path, fail.
			m_waitingForPathResult = false;
			m_bDisabledPendingFullUpdate = true;
			return true;
		}
		else
		{
			// Wait for the path finder result, disable movement and wait.
			pPipeUser->m_State.vMoveDir.Set(0,0,0);
			pPipeUser->m_State.fDesiredSpeed = 0.0f;
			pPipeUser->m_State.predictedCharacterStates.nStates = 0;
			return false;
		}
	}

	// Wait until in full update, and terminate the goalop.
  if (m_bDisabledPendingFullUpdate)
  {
    if (fullUpdate)
    {
      m_bDisabledPendingFullUpdate = false;
			Reset(pPipeUser);
      return true;
    }
    else
    {
			pPipeUser->m_State.vMoveDir.Set(0,0,0);
			pPipeUser->m_State.fDesiredSpeed = 0.0f;
			pPipeUser->m_State.predictedCharacterStates.nStates = 0;
      return true; // gets ignored
    }
  }

	bool	isUsing3DNavigation = pPipeUser->IsUsing3DNavigation();

	// Handle exact positioning and vaSOs.
	bool	forceRegenPath = false;
	bool	exactPositioning = false;

	// Handle exact positioning.
	if(pPipeUser->m_State.curActorTargetPhase == eATP_Error)
	{
		if(m_actorTargetRequester == eTATR_None)
		{
			m_actorTargetRequester = m_pendingActorTargetRequester;
			m_pendingActorTargetRequester = eTATR_None;
		}
		if(m_actorTargetRequester == eTATR_EndOfPath)
		{
			if (gAIEnv.CVars.DebugPathFinding)
				AILogAlways("COPTrace::ExecuteTrace resetting since error occurred during exact positioning %s", 
				pPipeUser ? GetNameSafe(pPipeUser) : "");

			if(fullUpdate)
			{
				Reset(pPipeUser);
				return true;
			}
			else
			{
				// TODO: Handle better the error case!
				pPipeUser->m_State.vMoveDir.Set(0,0,0);
				pPipeUser->m_State.fDesiredSpeed = 0.0f;
				pPipeUser->m_State.predictedCharacterStates.nStates = 0;
				m_bDisabledPendingFullUpdate = true;
				return true;
			}
		}
		else if(m_actorTargetRequester == eTATR_NavSO)
		{
			// Exact positioning has been failed at navigation smart object, regenerate path.
			forceRegenPath = true;
			m_inhibitPathRegen = false;
			m_earlyPathRegen = false;
		}
		m_actorTargetRequester = eTATR_None;
		m_pendingActorTargetRequester = eTATR_None;
	}
	else if(pPipeUser->m_State.curActorTargetPhase == eATP_Playing)
	{
		// While playing, keep the trace & prediction running.
		exactPositioning = true;
		// Do not update the trace while waiting for the animation to finish.
		if ( !pPipeUser->m_navSOEarlyPathRegen )
		return false;
	}
	else if(pPipeUser->m_State.curActorTargetPhase == eATP_Starting || pPipeUser->m_State.curActorTargetPhase == eATP_Started)
	{
		exactPositioning = true;

		// Regenerate path while playing the animation.
		if (m_pendingActorTargetRequester != eTATR_None)
		{
		m_actorTargetRequester = m_pendingActorTargetRequester;
		m_pendingActorTargetRequester = eTATR_None;
		}
//		AIAssert(!m_earlyPathRegen);
			if(pPipeUser->m_State.curActorTargetPhase == eATP_Started && m_actorTargetRequester == eTATR_NavSO)
			{
			if(pPipeUser->m_navSOEarlyPathRegen && !pPipeUser->m_State.curActorTargetFinishPos.IsZero())
				{
					// Path find from the predicted animation end position.
					pPipeUser->m_forcePathGenerationFrom = pPipeUser->m_State.curActorTargetFinishPos;
					forceRegenPath = true;
					m_earlyPathRegen = true;
					// Do not allow to regenerate the path until the current animation has finished.
					m_inhibitPathRegen = true;

					// Update distance traveled during the exactpos anim.
					Vec3 futurePos = pPipeUser->m_forcePathGenerationFrom;
					m_fTravelDist += !isUsing3DNavigation ? Distance::Point_Point2D(futurePos, m_lastPosition) : Distance::Point_Point(futurePos, m_lastPosition);
					m_lastPosition = futurePos;
				}
			}

		if (m_stopOnAnimationStart && m_actorTargetRequester == eTATR_EndOfPath)
		{
			if (fullUpdate)
			{
				Reset(pPipeUser);
				return true;
				}
			else
			{
				pPipeUser->m_State.vMoveDir.Set(0,0,0);
				pPipeUser->m_State.fDesiredSpeed = 0.0f;
				pPipeUser->m_State.predictedCharacterStates.nStates = 0;
				m_bDisabledPendingFullUpdate = true;
				return true;
			}
		}
	}
	else if(pPipeUser->m_State.curActorTargetPhase == eATP_Finished || pPipeUser->m_State.curActorTargetPhase == eATP_StartedAndFinished)
	{
		if(m_actorTargetRequester == eTATR_EndOfPath)
		{
			m_actorTargetRequester = eTATR_None;
			// Exact positioning has been finished at the end of the path, the trace is completed.
			if (gAIEnv.CVars.DebugPathFinding)
				AILogAlways("COPTrace::ExecuteTrace resetting since exact position reached/animation finished %s", 
				pPipeUser ? GetNameSafe(pPipeUser) : "");

			if (fullUpdate)
			{
				Reset(pPipeUser);
				return true;
			}
			else
			{
				pPipeUser->m_State.vMoveDir.Set(0,0,0);
				pPipeUser->m_State.fDesiredSpeed = 0.0f;
				pPipeUser->m_State.predictedCharacterStates.nStates = 0;
				m_bDisabledPendingFullUpdate = true;
				return true;
			}
		}
		else if(m_actorTargetRequester == eTATR_NavSO)
		{
			// Exact positioning has been finished at navigation smart object, regenerate path.
			if(!m_earlyPathRegen || pPipeUser->m_State.curActorTargetPhase == eATP_StartedAndFinished)
			{
				forceRegenPath = true;
				m_actorTargetRequester = eTATR_None;

				// Update distance traveled during the exactpos anim.
				Vec3 opPos = pPipeUser->GetPhysicsPos();
				m_fTravelDist += !isUsing3DNavigation ? Distance::Point_Point2D(opPos, m_lastPosition) : Distance::Point_Point(opPos, m_lastPosition);
				m_lastPosition = opPos;
			}

			m_waitingForPathResult = true;
			m_inhibitPathRegen = false;
			m_earlyPathRegen = false;
		}
		else
		{
			// A pending exact positioning request, maybe from previously interrupted trace.
			// Regenerate path, since the current path may be bogus because it was generated
			// before the animation had finished.
			forceRegenPath = true;
			m_actorTargetRequester = eTATR_None;
			m_waitingForPathResult = true;
			m_inhibitPathRegen = false;
			m_earlyPathRegen = false;
		}
	}
	else if(pPipeUser->m_State.curActorTargetPhase == eATP_Waiting)
	{
/*		if(m_pendingActorTargetRequester == eTATR_NavSO)
		{
			// Check if the navSO is still valid.
			PathPointDescriptor::SmartObjectNavDataPtr pSmartObjectNavData = pPipeUser->m_Path.GetLastPathPointAnimNavSOData();
			// Navigational SO at the end of the current path.
			const GraphNode* pFromNode = pSmartObjectNavData->pFrom;
			const GraphNode* pToNode = pSmartObjectNavData->pTo;

			if(gAIEnv.pSmartObjectManager->GetNavigationalSmartObjectActionType(pPipeUser, pFromNode, pToNode) == nSOmNone)
			{
				// The navSO state has changed, regenerate path.
				pPipeUser->SetNavSOFailureStates();
				const SSmartObjectNavData* pNavData = pSmartObjectNavData->pFrom->GetSmartObjectNavData();
				pPipeUser->InvalidateSOLink( pNavData->pSmartObject, pNavData->pHelper, pSmartObjectNavData->pTo->GetSmartObjectNavData()->pHelper );

				forceRegenPath = true;
				m_inhibitPathRegen = false;
				m_earlyPathRegen = false;
				m_actorTargetRequester = eTATR_None;
				m_pendingActorTargetRequester = eTATR_None;

				if (gAIEnv.CVars.DebugPathFinding)
				{
					CSmartObject* pObject = pFromNode->GetSmartObjectNavData()->pSmartObject;
					AILogAlways("COPTrace::ExecuteTrace %s regenerating path because the navSO '%s' state does not match anymore.",
						GetNameSafe(pPipeUser), pObject->GetEntity()->GetName());
				}

			}
		} */
		exactPositioning = true;
	}

	if(m_lastTime>0.0f)
	{
		m_TimeStep = (GetAISystem()->GetFrameStartTime() - m_lastTime).GetSeconds();
		if(m_TimeStep<=0)
			m_TimeStep = 0.0f;
	}
	else
	{
		// Reset the action input before starting to move.
		// Calling SetAGInput instead of ResetAGInput since we want to reset
		// the input even if some other system has set it to non-idle
		pPipeUser->GetProxy()->SetAGInput( AIAG_ACTION, "idle" );

		// Change the SO state to match the movement.
		IEntity* pEntity = pPipeUser->GetEntity();
		IEntity* pNullEntity = NULL;
		if ( gAIEnv.pSmartObjectManager->SmartObjectEvent( "OnMove", pEntity, pNullEntity ) != 0 )
			return false;
	}

  if (m_startTime < 0.0f)
    m_startTime  = GetAISystem()->GetFrameStartTime();
	m_lastTime = GetAISystem()->GetFrameStartTime();
  m_fTotalTracingTime += m_TimeStep;

  float distToSmartObject = std::numeric_limits<float>::max();
	if(!exactPositioning)
	{
		if (pPipeUser->GetPathFollower() && gAIEnv.CVars.PredictivePathFollowing)
      distToSmartObject = pPipeUser->GetPathFollower()->GetDistToSmartObject();
    else
      distToSmartObject = pPipeUser->m_Path.GetDistToSmartObject(!isUsing3DNavigation);
  }
  m_passingStraightNavSO = distToSmartObject < 1.0f;

	// interpret -ve end distance as the total distance to trace by subtracting it from the
	// original path length
/*	if (m_fEndDistance < 0.0f && pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND)
	{
		float pathLen = pPipeUser->m_Path.GetPathLength(true); // todo danny use 2/3D and cache
		m_fEndDistance += pathLen;
		if (m_fEndDistance < 0.0f)
			m_fEndDistance = 0.0f;

	}
	// override end distance if a duration has been set
	if (m_fDuration > 0.0f)
	{
		float pathLen = pPipeUser->m_Path.GetPathLength(true); // todo danny use 2/3D and cache
		float timeLeft = m_fDuration - (GetAISystem()->GetFrameStartTime() - m_startTime).GetSeconds();
		float normalSpeed = pPipeUser->GetNormalMovementSpeed(pPipeUser->m_State.fMovementUrgency, true);
		if (normalSpeed > 0.0f)
		{
			float distance = max(normalSpeed * timeLeft, 0.0f);
			m_fEndDistance = max(pathLen - distance, 0.0f);
		}
	}*/

	static float exactPosTriggerDistance = 5.0f; //10.0f;

	if (fullUpdate)
	{
		// Try to trigger exact positioning
		// Trigger the positioning when path is found (sometimes it is still in progress because of previous navSO usage),
		// and when close enough or just finished trace without following it.
		if((pPipeUser->m_State.fDistanceToPathEnd >= 0.0f && pPipeUser->m_State.fDistanceToPathEnd <= exactPosTriggerDistance) ||
				pPipeUser->m_Path.GetPathLength(isUsing3DNavigation) <= exactPosTriggerDistance)
		{
			if(pPipeUser->m_State.curActorTargetPhase == eATP_None)
			{
				// Handle the exact positioning request
				PathPointDescriptor::SmartObjectNavDataPtr pSmartObjectNavData = pPipeUser->m_Path.GetLastPathPointAnimNavSOData();

				if (gAIEnv.CVars.DebugPathFinding)
				{
					if(!pPipeUser->m_pendingNavSOStates.IsEmpty())
					{
						IEntity*	pEnt = gEnv->pEntitySystem->GetEntity(pPipeUser->m_pendingNavSOStates.objectEntId);
						if(pEnt)
							AILogAlways("COPTrace::ExecuteTrace %s trying to use exact positioning while a navSO (entity=%s) is still active.",
								GetNameSafe(pPipeUser), pEnt->GetName());
						else
							AILogAlways("COPTrace::ExecuteTrace %s trying to use exact positioning while a navSO (entityId=%d) is still active.",
								GetNameSafe(pPipeUser), pPipeUser->m_pendingNavSOStates.objectEntId);
					}
				}

				if(pSmartObjectNavData && pSmartObjectNavData->fromIndex && pSmartObjectNavData->toIndex)
				{
					// Navigational SO at the end of the current path.
					const GraphNode* pFromNode = gAIEnv.pGraph->GetNode(pSmartObjectNavData->fromIndex);
					const GraphNode* pToNode = gAIEnv.pGraph->GetNode(pSmartObjectNavData->toIndex);
					// Fill in the actor target request, and figure out the navSO method.
					if(gAIEnv.pSmartObjectManager->PrepareNavigateSmartObject(pPipeUser, pFromNode, pToNode) && pPipeUser->m_eNavSOMethod != nSOmNone)
					{
						pPipeUser->m_State.actorTargetReq.id = ++pPipeUser->m_actorTargetReqId;
						pPipeUser->m_State.actorTargetReq.lowerPrecision = true;
						m_pendingActorTargetRequester = eTATR_NavSO;

						// In case we hit here because the path following has finished, keep the trace alive.
						exactPositioning = true;

						// Enforce to use the current path.
						pPipeUser->m_Path.GetParams().inhibitPathRegeneration = true;
						gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);

						// TODO: these are debug variables, should be perhaps initialised somewhere else.
						pPipeUser->m_DEBUGCanTargetPointBeReached.clear();
						pPipeUser->m_DEBUGUseTargetPointRequest.zero();
					}
					else
					{
						// Failed to use the navSO. Instead of resetting the goalop, set the state
						// to error, to prevent the link being reused.
						forceRegenPath = true;
						m_earlyPathRegen = false;
						pPipeUser->m_State.actorTargetReq.Reset();
						m_actorTargetRequester = eTATR_None;
						m_pendingActorTargetRequester = eTATR_None;

						const SSmartObjectNavData* pNavData = gAIEnv.pGraph->GetNode(pSmartObjectNavData->fromIndex)->GetSmartObjectNavData();
						pPipeUser->InvalidateSOLink( pNavData->pSmartObject, pNavData->pHelper, gAIEnv.pGraph->GetNode(pSmartObjectNavData->toIndex)->GetSmartObjectNavData()->pHelper );

					}
				}
				else if(const SAIActorTargetRequest* pReq = pPipeUser->GetActiveActorTargetRequest())
				{
					// Actor target requested at the end of the path.
					pPipeUser->m_State.actorTargetReq = *pReq;
					pPipeUser->m_State.actorTargetReq.id = ++pPipeUser->m_actorTargetReqId;
					pPipeUser->m_State.actorTargetReq.lowerPrecision = false;
					m_pendingActorTargetRequester = eTATR_EndOfPath;
					// In case we hit here because the path following has finished, keep the trace alive.
					exactPositioning = true;

					// Enforce to use the current path.
					pPipeUser->m_Path.GetParams().inhibitPathRegeneration = true;
					gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);

					// TODO: these are debug variables, should be perhaps initialised somewhere else.
					pPipeUser->m_DEBUGCanTargetPointBeReached.clear();
					pPipeUser->m_DEBUGUseTargetPointRequest.zero();
				}
			}
			else if(pPipeUser->m_State.curActorTargetPhase != eATP_Error)
			{
				// The exact positioning is in motion but not yet playing, keep the trace alive.
				exactPositioning = true;
			}
		}
	}

	// If this path was generated with the pathfinder quietly regenerate the path
	// periodically in case something has moved.
	if (forceRegenPath || (fullUpdate && gAIEnv.CVars.CrowdControlInPathfind != 0 && 
		pPipeUser->m_movementAbility.pathRegenIntervalDuringTrace > 0.01f &&
		!pPipeUser->m_Path.GetParams().precalculatedPath &&
		!pPipeUser->m_Path.GetParams().inhibitPathRegeneration &&
		!m_inhibitPathRegen && !m_passingStraightNavSO))
	{
		if (forceRegenPath || m_fTotalTracingTime > pPipeUser->m_movementAbility.pathRegenIntervalDuringTrace)
		{
			// only regenerate if there isn't a new path that's about to come our way anyway
			if (!gAIEnv.pPathfinder->IsFindingPathFor(pPipeUser))
			{
				// Store the request params in the path
				const SNavPathParams& params = pPipeUser->m_Path.GetParams();
				int origPathDecision = pPipeUser->m_nPathDecision;
				if (gAIEnv.CVars.DebugPathFinding)
				{
					AILogAlways("COPTrace::ExecuteTrace %s regenerating path to (%5.2f, %5.2f, %5.2f) (origPathDecision = %d)",
						GetNameSafe(pPipeUser), params.end.x, params.end.y, params.end.z, origPathDecision);
				}
				float endTol = m_bForceReturnPartialPath || m_fEndAccuracy < 0.0f ? std::numeric_limits<float>::max() : m_fEndAccuracy;
				float endDist = params.endDistance;
				if (endDist < 0.0f)
					endDist = min(-0.01f, endDist + m_fTravelDist);
				
				if (params.isDirectional)
				{
					float searchRange = Distance::Point_Point(params.end, pPipeUser->GetPhysicsPos());
					pPipeUser->RequestPathInDirection(params.end, searchRange, pPipeUser->m_refPathFindTarget, endDist);
				}
				else
				{
					CAIObject* pPathFindTarget = pPipeUser->m_refPathFindTarget.GetAIObject();
					pPipeUser->RequestPathTo(params.end, params.endDir, params.allowDangerousDestination, params.nForceBuildingID, endTol, endDist, pPathFindTarget);
				}
				// Keep the travel dist per path to keep the end distance calculation correct over multiple regenerations.
				m_fTravelDist = 0.0f;

				// If not forcing the regen because of navSO, pretend that the state of the path is still the same.
				if(!forceRegenPath)
					pPipeUser->m_nPathDecision = origPathDecision;
			}
			m_fTotalTracingTime = 0.0f;

			// The path was forced to be regenerated when a navigational smartobject has been passed.
			if(forceRegenPath)
			{
				m_waitingForPathResult = true;
				return false;
			}
		}
	}


  bool traceResult = false;

  if (pPipeUser->GetPathFollower() && gAIEnv.CVars.PredictivePathFollowing)
  {
    traceResult = ExecutePathFollower(pPipeUser, fullUpdate);
  }
  else
  {
	  // If the current path segment involves 3D, then use 3D following
    if (isUsing3DNavigation)
		  traceResult = Execute3D(pPipeUser, fullUpdate);
	  else
		  traceResult = Execute2D(pPipeUser, fullUpdate);
  }

	// If exact positioning is in motion, do not allow to finish the goal op, just yet.
	// This may happen in some cases when the exact positioning start position is just over
	// the starting point of a navSO, or during the animation playback to keep the
	// prediction warm.
	if(exactPositioning)
		traceResult = false;

  // prevent future updates unless we get an external reset
	if (traceResult && pPipeUser->m_nPathDecision != PATHFINDER_STILLFINDING)
	{
		if (fullUpdate)
		{
			Reset(pPipeUser);
			return true;
		}
		else
		{
			pPipeUser->m_State.vMoveDir.Set(0,0,0);
			pPipeUser->m_State.fDesiredSpeed = 0.0f;
			pPipeUser->m_State.predictedCharacterStates.nStates = 0;
			m_bDisabledPendingFullUpdate = true;
			return false;
		}
	}

  return traceResult;
}

//===================================================================
// ExecuteManeuver
//===================================================================
void COPTrace::ExecuteManeuver(CPipeUser* pPipeUser, const Vec3& steerDir)
{
  if (fabs(pPipeUser->m_State.fDesiredSpeed) < 0.001f)
  {
    m_Maneuver = eMV_None;
    return;
  }

  // Update the debug movement reason.
  pPipeUser->m_DEBUGmovementReason = CPipeUser::AIMORE_MANEUVER;

  // first work out which direction to rotate (normally const over the whole
  // maneuver). Subsequently work out fwd/backwards. Then steer based on
  // the combination of the two

  float	cosTrh = pPipeUser->m_movementAbility.maneuverTrh;
  if(pPipeUser->m_movementAbility.maneuverTrh >= 1.f || pPipeUser->m_IsSteering)
    return;

  Vec3 myDir = pPipeUser->GetMoveDir();
  if (pPipeUser->m_State.fMovementUrgency < 0.0f)
    myDir *= -1.0f;
  myDir.z = 0.0f;
  myDir.NormalizeSafe();
  Vec3 reqDir = steerDir;
  reqDir.z = 0.0f;
  reqDir.NormalizeSafe();
  Vec3 myVel = pPipeUser->GetVelocity();
  Vec3 myPos = pPipeUser->GetPhysicsPos();

  float diffCos = reqDir.Dot(myDir);
  // if not maneuvering then require a big angle change to enter it
  if(diffCos>cosTrh && m_Maneuver == eMV_None)
    return;

  // prevent very small wiggles.
  static float maneuverTimeMinLimit = 0.3f;
  static float maneuverTimeMaxLimit = 5.0f;
  float manTime =  m_Maneuver != eMV_None ? (GetAISystem()->GetFrameStartTime() - m_ManeuverTime).GetSeconds() : 0.0f;


  // if maneuvering only stop when closely lined up
  static float exitDiffCos = 0.98f;
  if (diffCos > exitDiffCos && m_Maneuver != eMV_None && manTime > maneuverTimeMinLimit)
  {
    m_Maneuver = eMV_None;
    return;
  }

  // workaround for instant turning
  Vec3 camPos = GetISystem()->GetViewCamera().GetPosition();
  Vec3 opPos = pPipeUser->GetPhysicsPos();
  float distSq = Distance::Point_Point(camPos, opPos);
  static float minDistSq = square(5.0f);
  if (distSq > minDistSq)
  {
    bool visible = GetISystem()->GetViewCamera().IsSphereVisible_F(Sphere(opPos, pPipeUser->GetRadius()));
    if (!visible)
    {
      float x = reqDir.Dot(myDir);
      float y = reqDir.Dot(Vec3(-myDir.y, myDir.x, 0.0f));
      // do it in steps to help physics resolve penetrations...
      float rotAngle = 0.02f * atan2f(y, x);
      Quat q;
      q.SetRotationAA(rotAngle, Vec3(0, 0, 1));

      IEntity * pEntity = pPipeUser->GetEntity();
      Quat qCur = pEntity->GetRotation();
      pEntity->SetRotation(q * qCur);
      m_Maneuver = eMV_None;
      return;
    }
  }

  // set the direction
  Vec3 dirCross = myDir.Cross(reqDir);
  m_ManeuverDir = dirCross.z > 0.0f ? eMVD_AntiClockwise : eMVD_Clockwise;

  bool movingFwd = myDir.Dot(myVel) > 0.0f;

  static float maneuverDistLimit = 5;

  // start a new maneuver?
  if (m_Maneuver == eMV_None)
  {
		m_Maneuver = eMV_Back;
    m_ManeuverDist = 0.5f * maneuverDistLimit;
    m_ManeuverTime = GetAISystem()->GetFrameStartTime();
  }
  else
  {
    // reverse direction when we accumulate sufficient distance in the direction
    // we're supposed to be going in
    Vec3 delta = myPos - m_lastPosition;
    float dist = fabs(delta.Dot(myDir));
    if (movingFwd && m_Maneuver == eMV_Back)
      dist = 0.0f;
    else if (!movingFwd && m_Maneuver == eMV_Fwd)
      dist = 0.0f;
    m_ManeuverDist += dist;

		if ( manTime > maneuverTimeMaxLimit )
    {
      m_Maneuver = m_Maneuver == eMV_Fwd ? eMV_Back : eMV_Fwd;
      m_ManeuverDist = 0.0f;
      m_ManeuverTime = GetAISystem()->GetFrameStartTime();
    }
		else if ( m_Maneuver == eMV_Back )
		{
			if ( fabsf( reqDir.Dot( myDir ) ) < cosf( DEG2RAD( 85.0f ) ) )
			{
				m_Maneuver = eMV_Fwd;
				m_ManeuverDist = 0.0f;
				m_ManeuverTime = GetAISystem()->GetFrameStartTime();
			}
		}
		else
		{
			if ( fabsf( reqDir.Dot( myDir ) ) > cosf( DEG2RAD( 5.0f ) ) )
			{
				m_Maneuver = eMV_Back;
				m_ManeuverDist = 0.0f;
				m_ManeuverTime = GetAISystem()->GetFrameStartTime();
			}
  }

	}

  // now turn these into actual requests
	float normalSpeed, minSpeed, maxSpeed;
	pPipeUser->GetMovementSpeedRange(AISPEED_WALK, false, normalSpeed, minSpeed, maxSpeed);

	pPipeUser->m_State.fDesiredSpeed = minSpeed; //pPipeUser->GetManeuverMovementSpeed();
  if (m_Maneuver == eMV_Back)
		pPipeUser->m_State.fDesiredSpeed = -5.0;

  Vec3 leftDir(-myDir.y, myDir.x, 0.0f);

  if (m_ManeuverDir == eMVD_AntiClockwise)
    pPipeUser->m_State.vMoveDir = leftDir;
  else
    pPipeUser->m_State.vMoveDir = -leftDir;

  if (pPipeUser->m_State.fMovementUrgency < 0.0f)
    pPipeUser->m_State.vMoveDir *= -1.0f;
}

//===================================================================
// ExecutePreamble
//===================================================================
bool COPTrace::ExecutePreamble(CPipeUser* pPipeUser)
{
	CCCPOINT(COPTrace_ExecutePreamble);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (m_lastPosition.IsZero())
    m_lastPosition = pPipeUser->GetPhysicsPos();

	if (m_refNavTarget.IsNil())
	{
		if (pPipeUser->m_Path.Empty())
		{
    pPipeUser->m_State.fDesiredSpeed = 0.0f;
			m_inhibitPathRegen = true;
		}
		else
		{
			// Obtain a NavTarget
			CreateDummyFromNode(pPipeUser->GetPhysicsPos(), GetNameSafe(pPipeUser));
			m_inhibitPathRegen = false;
		}
	}
	else
	{
		m_inhibitPathRegen = false;
	}

	return m_inhibitPathRegen;
}

//===================================================================
// ExecutePostamble
//===================================================================
bool COPTrace::ExecutePostamble(CPipeUser* pPipeUser, bool &reachedEnd, bool fullUpdate, bool twoD)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  // do this after maneuver since that needs to track how far we moved
  Vec3 opPos = pPipeUser->GetPhysicsPos();
  m_fTravelDist += twoD ? Distance::Point_Point2D(opPos, m_lastPosition) : Distance::Point_Point(opPos, m_lastPosition);
  m_lastPosition = opPos;

  // only consider trace to be done once the agent has stopped.
  if (reachedEnd && m_fEndAccuracy >= 0.0f)
  {
    Vec3 vel = pPipeUser->GetVelocity();
    vel.z = 0.0f;
    float speed = vel.Dot(pPipeUser->m_State.vMoveDir);
    static float criticalSpeed = 0.01f;
    if (speed > criticalSpeed)
    {
      if (gAIEnv.CVars.DebugPathFinding)
        AILogAlways("COPTrace reached end but waiting for speed %5.2f to fall below %5.2f %s", 
        speed, criticalSpeed,
        pPipeUser ? GetNameSafe(pPipeUser) : "");
      reachedEnd = false;
      pPipeUser->m_State.fDesiredSpeed = 0.0f;
      m_inhibitPathRegen = true;
    }
  }

  // only consider trace to be done once the agent has stopped.
  if (reachedEnd)
  {
    pPipeUser->m_State.fDesiredSpeed = 0.0f;
    m_inhibitPathRegen = true;
  }

	// [Mikko] Commented out since the path is already cut after pathfinding.
	// Leaving this on, will prevent the path regen when it should not.
	/*  if (distToEnd < m_fEndAccuracy)
	m_inhibitPathRegen = true;*/

  if (reachedEnd)
  {
    // we reached the end of the path
/*    if (!pPipeUser->m_Path.Empty())
    {
      if (gAIEnv.CVars.DebugPathFinding)
        AILogAlways("COPTrace reached end so clearing path %s", 
        pPipeUser ? GetNameSafe(pPipeUser) : "");
      pPipeUser->ClearPath("COPTrace m_Path reached end");
    }*/
    pPipeUser->m_State.fDesiredSpeed = 0.0f;
    return true;
  }

  if (fullUpdate)
  {
		unsigned thisNodeIndex = gAIEnv.pGraph->GetEnclosing(opPos,
      pPipeUser->m_movementAbility.pathfindingProperties.navCapMask,
      pPipeUser->m_Parameters.m_fPassRadius,
			pPipeUser->m_lastNavNodeIndex, 0.0f, 0, false, GetNameSafe(pPipeUser));
    pPipeUser->m_lastNavNodeIndex = thisNodeIndex;

    static float forbiddenTol = 1.5f;
		GraphNode* pThisNode = gAIEnv.pGraph->GetNodeManager().GetNode(thisNodeIndex);
    if (pThisNode && 
      pThisNode->navType & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_ROAD) && 
      gAIEnv.pNavigation->IsPointOnForbiddenEdge(opPos, forbiddenTol, 0, 0, false))
      pPipeUser->m_bLastNearForbiddenEdge = true;
    else
      pPipeUser->m_bLastNearForbiddenEdge = false;
  }

  // code below here checks/handles dynamic objects
  if (pPipeUser->m_Path.GetParams().precalculatedPath)
    return false;

  static bool doSteering = true;
  if (doSteering && !pPipeUser->m_bLastNearForbiddenEdge)
  {
		pPipeUser->m_IsSteering = pPipeUser->NavigateAroundObjects(opPos, fullUpdate,m_bSteerAroundPathTarget);
  if (pPipeUser->m_IsSteering)
    pPipeUser->m_State.predictedCharacterStates.nStates = 0;
  }
  return false;
}

//===================================================================
// ExecutePathFollower
//===================================================================
bool COPTrace::ExecutePathFollower(CPipeUser* pPipeUser, bool fullUpdate)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if (m_TimeStep <= 0)
		return false;

  if (ExecutePreamble(pPipeUser))
    return true;

	CCCPOINT(COPTrace_ExecutePathFollower);

  static float predictionDeltaTime = 0.1f;
  static float predictionTime = 1.0f;

  IPathFollower *pathFollower = pPipeUser->GetPathFollower();
  AIAssert(pathFollower);

	float normalSpeed, minSpeed, maxSpeed;
	pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, pPipeUser->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

  // Update the path follower - really these things shouldn't change
	// TODO: Leaving const_cast here it indicate how awful this is!
  PathFollowerParams &params = const_cast<PathFollowerParams&>(pathFollower->GetParams());
	params.minSpeed = minSpeed;
	params.maxSpeed = maxSpeed;
	params.normalSpeed = clamp(normalSpeed, params.minSpeed, params.maxSpeed);

  params.endDistance = 0.0f;

	// [Mikko] Slight workaround to make the formation following better.
	// When following formation, the agent is often very close to the formation.
	// This code forces the actor to lag a bit behind, which helps to follow the formation much more smoothly.
	// TODO: Revive approach-at-distance without cutting the path and add that as extra parameter for stick.
	if (m_pendingActorTargetRequester == eTATR_None && pPipeUser->m_Path.GetParams().continueMovingAtEnd)
	{
		CAIObject *pPathFindTarget = pPipeUser->m_refPathFindTarget.GetAIObject();
		if (pPathFindTarget && pPathFindTarget->GetSubType() == IAIObject::STP_FORMATION)
		params.endDistance = 1.0f;
	}

  params.maxAccel = pPipeUser->m_movementAbility.maxAccel;
  params.maxDecel = pPipeUser->m_movementAbility.maxDecel;
  params.stopAtEnd = !pPipeUser->m_Path.GetParams().continueMovingAtEnd;

  PathFollowResult result;

  static PathFollowResult::TPredictedStates predictedStates;

	// Lower the prediction calculations for puppets which have lo update priority (means invisible or distant).
	bool highPriority = true;
	if (CPuppet* pPuppet = pPipeUser->CastToCPuppet())
	{
		if (pPuppet->GetUpdatePriority() != AIPUP_VERY_HIGH && pPuppet->GetUpdatePriority() != AIPUP_HIGH)
			highPriority = false;
	}
	result.desiredPredictionTime = highPriority ? predictionTime : 0.0f;
	int nPredictedStates = (int)(result.desiredPredictionTime / predictionDeltaTime + 0.5f);
  predictedStates.resize(nPredictedStates);
	result.predictedStates = highPriority ? &predictedStates: 0;
  result.predictionDeltaTime = predictionDeltaTime;

  Vec3 curPos = pPipeUser->GetPhysicsPos();
  Vec3 curVel = pPipeUser->GetVelocity();

  const bool targetReachable = pathFollower->Update(result, curPos, curVel, m_TimeStep);

  Vec3 desiredMoveDir = result.velocityOut;
  float desiredSpeed = desiredMoveDir.NormalizeSafe();

  pPipeUser->m_State.fDesiredSpeed = desiredSpeed;
  pPipeUser->m_State.vMoveDir = desiredMoveDir;
  pPipeUser->m_State.fDistanceToPathEnd = pathFollower->GetDistToEnd(&curPos);

  int num = min((int) predictedStates.size(), (int) SAIPredictedCharacterStates::maxStates);
  pPipeUser->m_State.predictedCharacterStates.nStates = num;
  for (int i = 0 ; i < num ; ++i)
  {
    const PathFollowResult::SPredictedState &state = predictedStates[i];
    pPipeUser->m_State.predictedCharacterStates.states[i].Set(state.pos, state.vel, (1 + i) * predictionDeltaTime);
  }

	// Precision movement support: A replacement for the old velocity based movement system above.
	// NOTE: If the move target and inflection point are equal (and non-zero) they mark the end of the path.
	pPipeUser->m_State.vMoveTarget			= result.followTargetPos;
	pPipeUser->m_State.vInflectionPoint = result.inflectionPoint;

  bool retVal = ExecutePostamble(pPipeUser, result.reachedEnd, fullUpdate, pathFollower->GetParams().use2D);
  return retVal;
}

//====================================================================
// Execute2D
//====================================================================
bool COPTrace::Execute2D(CPipeUser * const pPipeUser, bool fullUpdate)
{
  if (ExecutePreamble(pPipeUser))
    return true;

  // input
  Vec3 fwdDir = pPipeUser->GetMoveDir();
  if (pPipeUser->m_State.fMovementUrgency < 0.0f)
    fwdDir *= -1.0f;
  Vec3 opPos = pPipeUser->GetPhysicsPos();
  pe_status_dynamics  dSt;
  pPipeUser->GetPhysics()->GetStatus(&dSt);
  Vec3 opVel = m_Maneuver==eMV_None ? dSt.v : fwdDir*5.0f;
  float lookAhead = pPipeUser->m_movementAbility.pathLookAhead;
  float pathRadius = pPipeUser->m_movementAbility.pathRadius;
  bool resolveSticking = pPipeUser->m_movementAbility.resolveStickingInTrace;

  // output
  Vec3	steerDir(ZERO);
  float distToEnd = 0.0f;
  float distToPath = 0.0f;
  Vec3 pathDir(ZERO);
  Vec3 pathAheadDir(ZERO);
  Vec3 pathAheadPos(ZERO);

  bool isResolvingSticking = false;

  bool stillTracingPath = pPipeUser->m_Path.UpdateAndSteerAlongPath(
    steerDir, distToEnd, distToPath, isResolvingSticking,
    pathDir, pathAheadDir, pathAheadPos,
    opPos, opVel, lookAhead, pathRadius, m_TimeStep, resolveSticking, true);
  pPipeUser->m_State.fDistanceToPathEnd = max(0.0f, distToEnd);

  pathAheadDir.z = 0.0f;
  pathAheadDir.NormalizeSafe();
  Vec3 steerDir2D(steerDir);
  steerDir2D.z = 0.0f;
  steerDir2D.NormalizeSafe();

  // Update the debug movement reason.
  pPipeUser->m_DEBUGmovementReason = CPipeUser::AIMORE_TRACE;

  distToEnd -= /*m_fEndDistance*/ -pPipeUser->m_Path.GetDiscardedPathLength();
  bool reachedEnd = false;
  if (stillTracingPath && distToEnd>.1f)
  {
    Vec3 targetPos;
    if (m_refNavTarget && pPipeUser->m_Path.GetPosAlongPath(targetPos, lookAhead, true, true))
      m_refNavTarget->SetPos(targetPos);

    //turning maneuvering 
    static bool doManouever = true;
    if (doManouever)
      ExecuteManeuver(pPipeUser, steerDir);

    if(m_Maneuver!=eMV_None )
    {
			Vec3 curPos = pPipeUser->GetPhysicsPos();
			m_fTravelDist += Distance::Point_Point2D(curPos, m_lastPosition);
			m_lastPosition = curPos;
      // prevent path regen
      m_fTotalTracingTime = 0.0f;
      return false;
    }

		//    float normalSpeed = pPipeUser->GetNormalMovementSpeed(pPipeUser->m_State.fMovementUrgency, true);
		//    float slowSpeed = pPipeUser->GetManeuverMovementSpeed();
		//    if (slowSpeed > normalSpeed) slowSpeed = normalSpeed;
		float normalSpeed, minSpeed, maxSpeed;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, pPipeUser->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

    // These will be adjusted to the range 0-1 to select a speed between slow and normal
    float dirSpeedMod = 1.0f;
    float curveSpeedMod = 1.0f;
    float endSpeedMod = 1.0f;
    float slopeMod = 1.0f;
    float moveDirMod = 1.0f;

    // speed falloff - slow down if current direction is too different from desired
    if (pPipeUser->GetType()== AIOBJECT_VEHICLE)
    {
      static float offset = 1.0f; // if this is > 1 then there is a "window" where no slow-down occurs
      float	velFalloff = offset * pathAheadDir.Dot(fwdDir);
      float	velFalloffD=1-velFalloff;
      if (velFalloffD > 0.0f && pPipeUser->m_movementAbility.velDecay > 0.0f)
        dirSpeedMod = velFalloff/(velFalloffD*pPipeUser->m_movementAbility.velDecay);
    }

    // slow down due to the current/desired move direction - is sensitive to glitchy anim transitions
    // Shouldn't need this anyway with a spiffy animation system...

		// [mikko] Commented out since this was causing the character to get stuck if the velocity is really small.
		// TODO Danny: Delete once new path following is in place.
/*    if (gAIEnv.CVars.PuppetDirSpeedControl && pPipeUser->GetType()== AIOBJECT_PUPPET && distToEnd > lookAhead)
    {
      Vec3 vel = pPipeUser->GetVelocity();
      static float moveDirScale = 1.0f;
      Vec3 curDir(vel.x, vel.y, 0.0f);
			float	len = curDir.GetLength();
			if(len > 0.0001f)
			{
				curDir /= len;
				float dot = curDir.Dot(steerDir2D);
				moveDirMod = dot;
			}
    }*/

    // slow down due to the path curvature
    float lookAheadForSpeedControl;
    if (pPipeUser->m_movementAbility.pathSpeedLookAheadPerSpeed < 0.0f)
      lookAheadForSpeedControl = -pPipeUser->m_movementAbility.pathSpeedLookAheadPerSpeed * lookAhead * pPipeUser->m_State.fMovementUrgency;
    else
      lookAheadForSpeedControl = pPipeUser->m_movementAbility.pathSpeedLookAheadPerSpeed * pPipeUser->GetVelocity().GetLength();

    if (lookAheadForSpeedControl > 0.0f)
    {
      Vec3 pos, dir;
      float lowestPathDot = 0.0f;
      bool curveOK = pPipeUser->m_Path.GetPathPropertiesAhead(lookAheadForSpeedControl, true, pos, dir, 0, lowestPathDot, true);
      Vec3 thisPathSegDir = (pPipeUser->m_Path.GetNextPathPoint()->vPos - pPipeUser->m_Path.GetPrevPathPoint()->vPos);
      thisPathSegDir.z = 0.0f;
      thisPathSegDir.NormalizeSafe();
      float thisDot = thisPathSegDir.Dot(steerDir2D);
      if (thisDot < lowestPathDot)
        lowestPathDot = thisDot;
      if (curveOK)
      {
        float a = 1.0f - 2.0f * pPipeUser->m_movementAbility.cornerSlowDown; // decrease this to make the speed drop off quicker with angle
        float b = 1.0f - a;
        curveSpeedMod = a + b * lowestPathDot;
      }

      // slow down at end
      if (m_fEndAccuracy >= 0.0f && m_EndMode != eEM_MinimumDistance)
      {
        static float slowDownDistScale = 2.0f;
        static float minEndSpeedMod = 0.1f;
        float slowDownDist = slowDownDistScale * lookAheadForSpeedControl;
        float workingDistToEnd = m_fEndAccuracy + distToEnd - 0.2f * lookAheadForSpeedControl;
        if (slowDownDist > 0.1f && workingDistToEnd < slowDownDist)
        {
          endSpeedMod = workingDistToEnd / slowDownDist;
          Limit(endSpeedMod, minEndSpeedMod, 1.0f);
        }
      }
    }

		float slopeModCoeff = pPipeUser->m_movementAbility.slopeSlowDown;
    // slow down when going down steep slopes (especially stairs!)
		if (pPipeUser->m_lastNavNodeIndex && slopeModCoeff > 0 && gAIEnv.pGraph->GetNode(pPipeUser->m_lastNavNodeIndex)->navType == IAISystem::NAV_WAYPOINT_HUMAN)
    {
      static float slowDownSlope = 0.5f;
      float pathHorDist = steerDir.GetLength2D();
      if (pathHorDist > 0.0f && steerDir.z < 0.0f)
      {
				float slope = -steerDir.z / pathHorDist * slopeModCoeff;
        slopeMod = 1.0f - slope / slowDownSlope;
        static float minSlopeMod = 0.5f;
        Limit(slopeMod, minSlopeMod, 1.0f);
      }
    }

    // slow down when going up steep slopes
		if(slopeModCoeff > 0)
    {
      IPhysicalEntity * pPhysics = pPipeUser->GetPhysics();
      pe_status_living status;
      int valid = pPhysics->GetStatus(&status);
			if (valid)
			{
      if (!status.bFlying)
      {
        Vec3 sideDir(-steerDir2D.y, steerDir2D.x, 0.0f);
        Vec3 slopeN = status.groundSlope - status.groundSlope.Dot(sideDir) * sideDir;
        slopeN.NormalizeSafe();
        // d is -ve for uphill
        float d = steerDir2D.Dot(status.groundSlope);
        Limit(d, -0.99f, 0.99f);
        // map d=-1 -> -inf, d=0 -> 1, d=1 -> inf
					float uphillSlopeMod = (1 + d / (1.0f - square(d)))*slopeModCoeff;
        static float minUphillSlopeMod = 0.5f;
        if (uphillSlopeMod < minUphillSlopeMod)
          uphillSlopeMod = minUphillSlopeMod;
        if (uphillSlopeMod < 1.0f)
          slopeMod = min(slopeMod, uphillSlopeMod);
      }
    }
    }

    float maxMod = min(min(min(min(dirSpeedMod, curveSpeedMod), endSpeedMod), slopeMod), moveDirMod);
    Limit(maxMod, 0.0f, 1.0f);
    //    maxMod = pow(maxMod, 2.0f);
		
		if( m_bControllSpeed == true )
		{
			float newDesiredSpeed = (1.0f - maxMod) * minSpeed + maxMod * normalSpeed;

			float change = newDesiredSpeed - pPipeUser->m_State.fDesiredSpeed;
			if (change > m_TimeStep * pPipeUser->m_movementAbility.maxAccel)
				change = m_TimeStep * pPipeUser->m_movementAbility.maxAccel;
			else if (change < -m_TimeStep * pPipeUser->m_movementAbility.maxDecel)
				change = -m_TimeStep * pPipeUser->m_movementAbility.maxDecel;
			pPipeUser->m_State.fDesiredSpeed += change;
		}

    pPipeUser->m_State.vMoveDir = steerDir2D;
    if (pPipeUser->m_State.fMovementUrgency < 0.0f)
      pPipeUser->m_State.vMoveDir *= -1.0f;

    // prediction
//    static bool doPrediction = true;
    pPipeUser->m_State.predictedCharacterStates.nStates = 0;
  }
  else
  {
    reachedEnd = true;
  }

  return ExecutePostamble(pPipeUser, reachedEnd, fullUpdate, true);
}

//====================================================================
// Execute3D
//====================================================================
bool COPTrace::Execute3D(CPipeUser* pPipeUser, bool fullUpdate)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  if (ExecutePreamble(pPipeUser))
    return true;

  // Ideally do this check less... but beware we might regen the path and the end might change a bit(?)
  if (fullUpdate)
  {
    if (pPipeUser->GetType() == AIOBJECT_VEHICLE && m_fEndAccuracy == 0.0f && !pPipeUser->m_Path.GetPath().empty())
    {
      Vec3 endPt = pPipeUser->m_Path.GetPath().back().vPos;
      // ideally we would use AICE_ALL here, but that can result in intersection with the heli when it
      // gets to the end of the path...
      bool gotFloor = GetFloorPos(m_landingPos, endPt, 0.5f, 1.0f, 1.0f, AICE_STATIC);
      if (gotFloor)
        m_landHeight = 2.0f;
      else
        m_landHeight = 0.0f;
      if (m_workingLandingHeightOffset > 0.0f)
        m_inhibitPathRegen = true;
    }
    else
    {
      m_landHeight = 0.0f;
      m_inhibitPathRegen = false;
    }
  }

  // input
  Vec3 opPos = pPipeUser->GetPhysicsPos();
  Vec3 fakeOpPos = opPos;
  fakeOpPos.z -= m_workingLandingHeightOffset;
  if (pPipeUser->m_IsSteering)
    fakeOpPos.z -= pPipeUser->m_flightSteeringZOffset;

  pe_status_dynamics  dSt;
  pPipeUser->GetPhysics()->GetStatus(&dSt);
  Vec3 opVel = dSt.v;
  float lookAhead = pPipeUser->m_movementAbility.pathLookAhead;
  float pathRadius = pPipeUser->m_movementAbility.pathRadius;
  bool resolveSticking = pPipeUser->m_movementAbility.resolveStickingInTrace;

  // output
  Vec3	steerDir(ZERO);
  float distToEnd = 0.0f;
  float distToPath = 0.0f;
  Vec3 pathDir(ZERO);
  Vec3 pathAheadDir(ZERO);
  Vec3 pathAheadPos(ZERO);

  bool isResolvingSticking = false;

  bool stillTracingPath = pPipeUser->m_Path.UpdateAndSteerAlongPath(steerDir, distToEnd, distToPath, isResolvingSticking,
    pathDir, pathAheadDir, pathAheadPos,
    fakeOpPos, opVel, lookAhead, pathRadius, m_TimeStep, resolveSticking, false);
  pPipeUser->m_State.fDistanceToPathEnd = max(0.0f, distToEnd);

  // Update the debug movement reason.
  pPipeUser->m_DEBUGmovementReason = CPipeUser::AIMORE_TRACE;

  distToEnd -= distToPath;
  distToEnd -= m_landHeight * 2.0f;
  if (distToEnd < 0.0f)
    stillTracingPath = false;

  if (!stillTracingPath && m_landHeight > 0.0f)
  {
    return ExecuteLanding(pPipeUser, m_landingPos);
  }

  distToEnd -= /*m_fEndDistance*/ -pPipeUser->m_Path.GetDiscardedPathLength();
  bool reachedEnd = !stillTracingPath;
  if (stillTracingPath && distToEnd > 0.0f)
  {
    Vec3 targetPos;
    if (m_refNavTarget && pPipeUser->m_Path.GetPosAlongPath(targetPos, lookAhead, true, true))
      m_refNavTarget->SetPos(targetPos);

		//    float normalSpeed = pPipeUser->GetNormalMovementSpeed(pPipeUser->m_State.fMovementUrgency, true);
		//    float slowSpeed = pPipeUser->GetManeuverMovementSpeed();
		//    if (slowSpeed > normalSpeed) slowSpeed = normalSpeed;
		float normalSpeed, minSpeed, maxSpeed;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, pPipeUser->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

    // These will be adjusted to the range 0-1 to select a speed between slow and normal
    float dirSpeedMod = 1.0f;
    float curveSpeedMod = 1.0f;
    float endSpeedMod = 1.0f;
    float moveDirMod = 1.0f;

    // slow down due to the path curvature
    float lookAheadForSpeedControl;
    if (pPipeUser->m_movementAbility.pathSpeedLookAheadPerSpeed < 0.0f)
      lookAheadForSpeedControl = lookAhead * pPipeUser->m_State.fMovementUrgency;
    else
      lookAheadForSpeedControl = pPipeUser->m_movementAbility.pathSpeedLookAheadPerSpeed * pPipeUser->GetVelocity().GetLength();

    lookAheadForSpeedControl -= distToPath;
    if (lookAheadForSpeedControl < 0.0f)
      lookAheadForSpeedControl = 0.0f;

    if (lookAheadForSpeedControl > 0.0f)
    {
      Vec3 pos, dir;
      float lowestPathDot = 0.0f;
      bool curveOK = pPipeUser->m_Path.GetPathPropertiesAhead(lookAheadForSpeedControl, true, pos, dir, 0, lowestPathDot, true);
      Vec3 thisPathSegDir = (pPipeUser->m_Path.GetNextPathPoint()->vPos - pPipeUser->m_Path.GetPrevPathPoint()->vPos).GetNormalizedSafe();
      float thisDot = thisPathSegDir.Dot(steerDir);
      if (thisDot < lowestPathDot)
        lowestPathDot = thisDot;
      if (curveOK)
      {
        float a = 1.0f - 2.0f * pPipeUser->m_movementAbility.cornerSlowDown; // decrease this to make the speed drop off quicker with angle
        float b = 1.0f - a;
        curveSpeedMod = a + b * lowestPathDot;
      }
    }

    // slow down at end 
    if (m_fEndAccuracy >= 0.0f)
    {
      static float slowDownDistScale = 1.0f;
      float slowDownDist = slowDownDistScale * lookAheadForSpeedControl;
      float workingDistToEnd = m_fEndAccuracy + distToEnd - 0.2f * lookAheadForSpeedControl;
      if (slowDownDist > 0.1f && workingDistToEnd < slowDownDist)
      {
        // slow speeds are for manouevering - here we want something that will actually be almost stationary
				minSpeed *= 0.1f;
        endSpeedMod = workingDistToEnd / slowDownDist;
        Limit(endSpeedMod, 0.0f, 1.0f);
        m_workingLandingHeightOffset = (1.0f - endSpeedMod) * m_landHeight;
      }
      else
      {
        m_workingLandingHeightOffset = 0.0f;
      }
    }

    float maxMod = min(min(min(dirSpeedMod, curveSpeedMod), endSpeedMod), moveDirMod);
    Limit(maxMod, 0.0f, 1.0f);

		float newDesiredSpeed = (1.0f - maxMod) * minSpeed + maxMod * normalSpeed;
    float change = newDesiredSpeed - pPipeUser->m_State.fDesiredSpeed;
    if (change > m_TimeStep * pPipeUser->m_movementAbility.maxAccel)
      change = m_TimeStep * pPipeUser->m_movementAbility.maxAccel;
		else if (change < -m_TimeStep * pPipeUser->m_movementAbility.maxDecel)
			change = -m_TimeStep * pPipeUser->m_movementAbility.maxDecel;
    pPipeUser->m_State.fDesiredSpeed += change;

    pPipeUser->m_State.vMoveDir = steerDir;
    if (pPipeUser->m_State.fMovementUrgency < 0.0f)
      pPipeUser->m_State.vMoveDir *= -1.0f;

    // prediction
//    static bool doPrediction = true;
    pPipeUser->m_State.predictedCharacterStates.nStates = 0;
  }
  else
  {
    reachedEnd = true;
  }

  return ExecutePostamble(pPipeUser, reachedEnd, fullUpdate, false);
}

//===================================================================
// ExecuteLanding
//===================================================================
bool COPTrace::ExecuteLanding(CPipeUser* pPipeUser, const Vec3 &pathEnd)
{
	assert(m_refNavTarget);

  m_inhibitPathRegen = true;
	float normalSpeed, minSpeed, maxSpeed;
	pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, false, normalSpeed, minSpeed, maxSpeed);
	//  float slowSpeed = pPipeUser->GetManeuverMovementSpeed();
  Vec3 opPos = pPipeUser->GetPhysicsPos();

  Vec3 horMoveDir = pathEnd - opPos;
  horMoveDir.z = 0.0f;
  float error = horMoveDir.NormalizeSafe();

  Limit(error, 0.0f, 1.0f);
	float horSpeed = 0.3f * minSpeed * error;
  float verSpeed = 1.0f;

  pPipeUser->m_State.vMoveDir = horSpeed * horMoveDir - Vec3(0, 0, verSpeed);
  pPipeUser->m_State.vMoveDir.NormalizeSafe();
  pPipeUser->m_State.fDesiredSpeed = sqrtf(square(horSpeed) + square(verSpeed));

  if (pPipeUser->m_State.fMovementUrgency < 0.0f)
    pPipeUser->m_State.vMoveDir *= -1.0f;

  // set look dir
  if (m_landingDir.IsZero())
  {
    if (gAIEnv.CVars.DebugPathFinding)
      AILogAlways("COPTrace::ExecuteLanding starting final landing %s", 
      pPipeUser ? GetNameSafe(pPipeUser) : "");
    m_landingDir = pPipeUser->GetMoveDir();
    m_landingDir.z = 0.0f;
    m_landingDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);
  }
  Vec3 navTargetPos = opPos + 100.0f * m_landingDir;
	m_refNavTarget->SetPos(navTargetPos);

  if (!pPipeUser->m_bLooseAttention)
  {
		m_looseAttentionId= pPipeUser->SetLooseAttentionTarget(m_refNavTarget);
  }

  // 
  pe_status_collisions stat;
  stat.pHistory = 0;
  int collisions = pPipeUser->GetPhysics()->GetStatus(&stat);
  if (collisions > 0)
    return true;
  else 
    return false;
}

//===================================================================
// DebugDraw
//===================================================================
void COPTrace::DebugDraw(CPipeUser* pPipeUser) const
{
	if(IsPathRegenerationInhibited())
	{
		CDebugDrawContext dc;
		dc->Draw3dLabel(pPipeUser->GetPhysicsPos(), 1.5f, "PATH LOCKED\n%s %s", m_inhibitPathRegen ? "Inhibit" : "", m_passingStraightNavSO ? "NavSO" : "");
	}
}

//===================================================================
// CreateDummyFromNode
//===================================================================
void COPTrace::CreateDummyFromNode(const Vec3 &pos, const char* ownerName)
{
	stack_string name("navTarget_");

	if(ownerName != NULL)
		name += ownerName;

	GetAISystem()->CreateDummyObject(m_refNavTarget, name, CAIObject::STP_REFPOINT);
	m_refNavTarget.GetAIObject()->SetPos(pos);
}


EGoalOpResult COPIgnoreAll::Execute(CPipeUser* pPipeUser)
{
//	pPipeUser->m_bUpdateInternal = !m_bParam;
	pPipeUser->m_bCanReceiveSignals = !m_bParam;
	return eGOR_DONE;
}

COPSignal::COPSignal(const XmlNodeRef& node) :
	m_nSignalID(1),
	m_sSignal(node->getAttr("name")),
	m_cFilter(SIGNALFILTER_SENDER),
	m_pTarget(0),
	m_bSent(false),
	m_iDataValue(0)
{
	node->getAttr("id", m_nSignalID);
	s_xml.GetSignalFilter(node, "filter", m_cFilter);
	node->getAttr("data", m_iDataValue);
}

EGoalOpResult COPSignal::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPSignal_Execute);

	if (m_bSent)
	{
		m_bSent = false;
		return eGOR_DONE;
	}

	AISignalExtraData* pData = new AISignalExtraData;
	pData->iValue = m_iDataValue;

 	if (!m_cFilter)	// if you are sending to yourself
	{
		pPipeUser->SetSignal(m_nSignalID,m_sSignal.c_str(), pPipeUser->GetEntity(), pData );
		m_bSent = true;
		return eGOR_IN_PROGRESS;
	}

	switch (m_cFilter)
	{
		case SIGNALFILTER_LASTOP:
			{
			// signal to last operation target
				CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();
				if (pLastOpResult)
			{
					CAIActor* pOperandActor = pLastOpResult->CastToCAIActor();
				if(pOperandActor)
					pOperandActor->SetSignal(m_nSignalID, m_sSignal.c_str(), pPipeUser->GetEntity(), pData);
			}
			}
			break;
		case SIGNALFILTER_TARGET:
			{
				CAIActor* pTargetActor = m_pTarget->CastToCAIActor();
				if(pTargetActor)
					pTargetActor->SetSignal(m_nSignalID, m_sSignal.c_str(),pPipeUser->GetEntity(), pData);
			}
			break;
		default:
			// signal to species, group or anyone within comm range
			GetAISystem()->SendSignal(m_cFilter, m_nSignalID, m_sSignal.c_str(), pPipeUser, pData);
			break;

	}

	m_bSent = true;
	return eGOR_IN_PROGRESS; 
}

COPScript::COPScript(const SmartScriptFunction& scriptCode, const ScriptAnyValue& userData)
{
	m_scriptCode = scriptCode;
	m_userData = gEnv->pScriptSystem->CloneAny(userData);
}

COPScript::COPScript(const XmlNodeRef& node)
{
	string sCode = node->getAttr("code");
	if (!sCode.empty())
	{
		IScriptSystem* pScriptSystem = gEnv->pScriptSystem;
		SmartScriptFunction scriptCode(pScriptSystem,
			pScriptSystem->CompileBuffer(sCode.c_str(), sCode.length(), "Script GoalOp"));
		if (scriptCode)
		{
			m_scriptCode = scriptCode;
			m_userData = gEnv->pScriptSystem->CloneAny(node->getAttr("data"));
		}
		else
		{
			AIError("Goalop '%s': Failed to compile Lua code: %s", node->getTag(), sCode.c_str());
		}
	}
}

COPScript::~COPScript()
{
	gEnv->pScriptSystem->ReleaseAny(m_userData);
}

EGoalOpResult COPScript::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(CCOPScript_Execute);

	if (!m_scriptCode)
		return eGOR_FAILED;

	bool result = false;

	// TODO(Márcio): Set the function environment here, instead of setting globals
	gEnv->pScriptSystem->SetGlobalValue("entity", pPipeUser->GetEntity()->GetScriptTable());
	gEnv->pScriptSystem->SetGlobalValue("entity_id", pPipeUser->GetEntity()->GetId());
	gEnv->pScriptSystem->SetGlobalValue("userdata", m_userData);

	if (Script::CallReturn(gEnv->pScriptSystem, m_scriptCode, m_userData, result))
	{
		return result ? eGOR_SUCCEEDED : eGOR_FAILED;
	}

	return eGOR_FAILED;
}


COPDeValue::COPDeValue(const XmlNodeRef& goalOpNode) :
	m_bDevaluePuppets(s_xml.GetBool(goalOpNode, "devaluePuppets")),
	m_bClearDevalued(s_xml.GetBool(goalOpNode, "clearDevalued"))
{
}

EGoalOpResult COPDeValue::Execute(CPipeUser* pPipeUser)
{
	if(m_bClearDevalued)
		pPipeUser->ClearDevalued();
	else if (pPipeUser->GetAttentionTarget())
	{
		if( CPuppet *pPuppet = pPipeUser->CastToCPuppet() )
			pPuppet->Devalue(pPipeUser->GetAttentionTarget(), m_bDevaluePuppets);
}
	return eGOR_DONE;
}

EGoalOpResult COPHide::Execute(CPipeUser* pPipeUser)
		{
	CCCPOINT(COPHide_Execute);

	CAISystem *pSystem = GetAISystem();

	if ( m_iActionId )
	{
		CCCPOINT(COPHide_Execute_SO);

		m_bEndEffect = false; // never use any end effects with smart objects
		pPipeUser->SetInCover(pPipeUser->m_bLastActionSucceed);
		pPipeUser->SetMovingToCover(false);
		m_iActionId = 0;

		// this code block is just copied from below
		Reset(pPipeUser);
		if (IsBadHiding(pPipeUser))
		{
			pPipeUser->SetSignal(1,"OnBadHideSpot" ,0, 0, gAIEnv.SignalCRCs.m_nOnBadHideSpot);
			return eGOR_FAILED;
		}
		else
		{
			pPipeUser->SetSignal(1,"OnHideSpotReached", 0, 0, gAIEnv.SignalCRCs.m_nOnHideSpotReached);
			return eGOR_SUCCEEDED;
		}
	}

	CAIObject * const pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();

	if (m_refHideTarget.IsNil())
	{
		CCCPOINT(COPHide_Execute_NoHideTarget);

		m_bEndEffect = false;
		pPipeUser->ClearPath("COPHide::Execute m_Path");
		m_bAttTarget = true;

		// Extract and remove the flags from the evaluation method
		int flagmask = 	(HM_INCLUDE_SOFTCOVERS | HM_IGNORE_ENEMIES | HM_BACK | HM_AROUND_LASTOP | HM_FROM_LASTOP);
		int flags = m_nEvaluationMethod & flagmask;
		m_nEvaluationMethod &= ~flagmask;

		if (!pPipeUser->GetAttentionTarget())
		{
			m_bAttTarget = false;
			if ((m_nEvaluationMethod != HM_USEREFPOINT) && (m_nEvaluationMethod != HM_USEHIDEOBJECT))
			{
				if (!pLastOpResult)
				{
					Reset(pPipeUser);
					pPipeUser->SetInCover(false);

					return eGOR_FAILED;
				}
			}
		}

		// lets create the place where we will hide

		CLeader* pLeader = GetAISystem()->GetLeader(pPipeUser);

		Vec3 vHidePos;

		if (m_nEvaluationMethod == HM_USEREFPOINT)
		{
			CCCPOINT(COPHide_Execute_UserRefPoint);

			if (pPipeUser)
			{
				if (IAIObject* pRefPoint = pPipeUser->GetRefPoint())
				{
					// Setup the hide object.
					vHidePos = pRefPoint->GetPos();
					Vec3 vHideDir = pRefPoint->GetBodyDir();

					if (vHideDir.IsZero())
					{
						Vec3	target(pPipeUser->GetAttentionTarget() ? pPipeUser->GetAttentionTarget()->GetPos() : vHidePos);
						vHideDir = (target - vHidePos).GetNormalizedSafe();
					}
				
					SHideSpot	hs(SHideSpotInfo::eHST_ANCHOR, vHidePos, vHideDir);
					pPipeUser->m_CurrentHideObject.Set(&hs, hs.info.pos, hs.info.dir);

					if (GetAISystem()->IsHideSpotOccupied(pPipeUser, hs.info.pos))
					{
						Reset(pPipeUser);
						pPipeUser->SetInCover(false);
						return eGOR_FAILED;
					}
				
					pPipeUser->SetMovingToCover(true);
					CreateHideTarget(GetNameSafe(pPipeUser), vHidePos);
					return eGOR_IN_PROGRESS;
				}
			}
		}
		else if (m_nEvaluationMethod == HM_USEHIDEOBJECT)
		{
			CCCPOINT(COPHide_Execute_UserRefPoint);

			if (pPipeUser)
			{
				if (pPipeUser->m_CurrentHideObject.IsValid())
				{
					// TODO: Here we can calculate which place in the cover is best to move to
					// instead of moving to the center first
					vHidePos = pPipeUser->m_CurrentHideObject.GetLastHidePos();

					pPipeUser->SetMovingToCover(true);
					CreateHideTarget(GetNameSafe(pPipeUser), vHidePos);

					return eGOR_IN_PROGRESS;
				}
			}
		}
		else
		{
			Reset(pPipeUser);
			pPipeUser->SetInCover(false);

			return eGOR_FAILED;
		}

		int nbid;
		IVisArea *iva;
		Vec3 searchPos = pLastOpResult && (flags & HM_AROUND_LASTOP)!=0 ? pLastOpResult->GetPos():pPipeUser->GetPos();
		IAISystem::ENavigationType	navType = gAIEnv.pNavigation->CheckNavigationType(searchPos, nbid, iva, pPipeUser->m_movementAbility.pathfindingProperties.navCapMask);


		Vec3 hideFrom;
		if (m_bAttTarget)
			hideFrom = pPipeUser->GetAttentionTarget()->GetPos();
		else if (pLastOpResult && (flags & HM_FROM_LASTOP) != 0)
			hideFrom = pLastOpResult->GetPos();
		else if (CAIObject* pBeacon = (CAIObject*)GetAISystem()->GetBeacon(pPipeUser->GetGroupId()))
			hideFrom = pBeacon->GetPos();
		else
			hideFrom = pPipeUser->GetPos();	// Just in case there is nothing to hide from (not even beacon), at least try to hide.

		// Store our current hidepos
		Vec3 vLastHidePos = pPipeUser->m_CurrentHideObject.GetObjectPos();

		// Put the flags back in
		m_nEvaluationMethod |= flags;
		vHidePos = pPipeUser->FindHidePoint( m_fSearchDistance, hideFrom, m_nEvaluationMethod, navType, true, m_fMinDistance );
		// MERGE : (MATT) boolean argument above is bSameOK, was false for G04, this op little used so switched to true optimistically {2008/01/07:12:13:48}

		if (!pPipeUser->m_CurrentHideObject.IsValid())
		{
			Reset(pPipeUser);
			pPipeUser->SetInCover(false);

			return eGOR_FAILED;
		}

		// shouldn't do this because FindHidePoint() has already set the right value in m_vLastHidePos
		//pPipeUser->m_vLastHidePos = vHidePos;
		
		// is it a smart object?
		if ( pPipeUser->m_CurrentHideObject.GetSmartObject().pRule )
		{
			int id = GetAISystem()->AllocGoalPipeId();
			gAIEnv.pSmartObjectManager->UseSmartObject( pPipeUser->m_CurrentHideObject.GetSmartObject().pUser,
				pPipeUser->m_CurrentHideObject.GetSmartObject().pObject, pPipeUser->m_CurrentHideObject.GetSmartObject().pRule, id );

			pPipeUser->SetMovingToCover(false);
			return eGOR_SUCCEEDED;
		}

		if(m_bLookAtLastOp && pLastOpResult )
		{
			m_looseAttentionId = pPipeUser->SetLooseAttentionTarget(pPipeUser->m_refLastOpResult);
		}

		CreateHideTarget(GetNameSafe(pPipeUser), vHidePos);
		pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

		pPipeUser->SetMovingToCover(true);
	}
	else // matches: if (m_refHideTarget.IsNil())
	{

		if (!m_pPathFindDirective)
		{
      if (gAIEnv.CVars.DebugPathFinding)
      {
				const Vec3 &vPos = m_refHideTarget->GetPos();
        AILogAlways("COPHide::Execute %s pathfinding to (%5.2f, %5.2f, %5.2f)", GetNameSafe(pPipeUser),
          vPos.x, vPos.y, vPos.z);
      }

			// request the path
			m_pPathFindDirective = new COPPathFind("",m_refHideTarget.GetAIObject());
			
			pPipeUser->SetMovingToCover(true);

			return eGOR_IN_PROGRESS;
		}
		else
		{
			if (!m_pTraceDirective)
			{
				if (m_pPathFindDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
				{
					if (pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND)
					{
            if (gAIEnv.CVars.DebugPathFinding)
            {
							const Vec3 &vPos = m_refHideTarget->GetPos();
              AILogAlways("COPHide::Execute %s Creating trace to hide target (%5.2f, %5.2f, %5.2f)", GetNameSafe(pPipeUser),
                vPos.x, vPos.y, vPos.z);
            }
						m_pTraceDirective = new COPTrace(m_bLookAtHide, defaultTraceEndAccuracy);
					}
					else
					{
						CCCPOINT(COPHide_Execute_Unreachable);

						// Could not reach the point, mark it ignored so that we do not try to pick it again.
						pPipeUser->IgnoreCurrentHideObject(10.0f);

						Reset(pPipeUser);
						return eGOR_FAILED;
					}
				}
			}
			else
			{
				if (m_pTraceDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
				{
					CCCPOINT(COPHide_Execute_A);

					Reset(pPipeUser);

					if (IsBadHiding(pPipeUser))
					{
						pPipeUser->SetSignal(1,"OnBadHideSpot" ,0, 0, gAIEnv.SignalCRCs.m_nOnBadHideSpot);
						return eGOR_FAILED;
					}
					else
					{
						pPipeUser->SetInCover(true);
						pPipeUser->SetSignal(1,"OnHideSpotReached", 0, 0, gAIEnv.SignalCRCs.m_nOnHideSpotReached);
						return eGOR_SUCCEEDED;
					}
				}
				else
				{
					CCCPOINT(COPHide_Execute_B);

					pPipeUser->SetMovingToCover(true);

					if (!m_bEndEffect)
					{
						const Vec3 &vHidePos = m_refHideTarget->GetPos();
						const Vec3 &vOperandPos = pPipeUser->GetPos();

						// we are on our last approach
						Vec3 vOperandToHide = vOperandPos-vHidePos;
						if ( vOperandToHide.GetLengthSquared() < 9.f)
						{
							// you are less than 3 meters from your hide target
							Vec3 vViewDir = pPipeUser->GetMoveDir();
							Vec3 vHideDir = vOperandToHide.GetNormalized();

							if (vViewDir.Dot(vHideDir) > 0.8f) 
							{
								//	pPipeUser->SetSignal(1,"HIDE_END_EFFECT");
								m_bEndEffect = true;
							}
						}
					}
				}
			}
		}
	}

	return eGOR_IN_PROGRESS;
}

void COPHide::ExecuteDry(CPipeUser* pPipeUser) 
{
	CCCPOINT(COPHide_ExecuteDry);

  if (m_pTraceDirective)
    m_pTraceDirective->ExecuteTrace(pPipeUser, false);
}

bool COPHide::IsBadHiding(CPipeUser* pPipeUser)
{
	CCCPOINT(COPHide_IsBadHiding);

	IAIObject *pTarget = pPipeUser->GetAttentionTarget();

	if (!pTarget)
		return false;

	Vec3 ai_pos = pPipeUser->GetPos();
	Vec3 target_pos = pTarget->GetPos();
	ray_hit hit;

	std::vector<IPhysicalEntity*> skipList;
	pPipeUser->GetPhysicsEntitiesToSkip(skipList);
	if (CAIActor* pActor = pTarget->CastToCAIActor())
		pActor->GetPhysicsEntitiesToSkip(skipList);

/*	IPhysicalEntity	*skipList[4];
	int skipCount(1);
	skipList[0] = pPipeUser->GetProxy()->GetPhysics();
	if(pTarget->GetProxy() && pTarget->GetProxy()->GetPhysics())
	{
		skipList[skipCount++] = pTarget->GetProxy()->GetPhysics();
		if(pTarget->GetProxy()->GetPhysics(true))
			skipList[skipCount++] = pTarget->GetProxy()->GetPhysics(true);
	}*/

	int rayresult = 0;
	rayresult = gAIEnv.pWorld->RayWorldIntersection(target_pos,ai_pos-target_pos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,
		&hit,1, skipList.empty() ? 0 : &skipList[0], skipList.size());
	if (rayresult)
	{
		// check possible leaning direction
		if (Distance::Point_PointSq(hit.pt, ai_pos) < sqr(3.0f))
		{
			Vec3 dir = ai_pos-target_pos;
			float zcross =  dir.y*hit.n.x - dir.x*hit.n.y;
			if (zcross < 0)
				pPipeUser->SetSignal(1,"OnRightLean", 0, 0, gAIEnv.SignalCRCs.m_nOnRightLean);
			else
				pPipeUser->SetSignal(1,"OnLeftLean", 0, 0, gAIEnv.SignalCRCs.m_nOnLeftLean);
		}
		return false;
	}

/*
	// check if it is prone-here spot
	Vec3	floorPos;
	if( GetFloorPos(floorPos, ai_pos, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL))
	{
		floorPos.z += .2f;
		rayresult = pWorld->RayWorldIntersection(floorPos,target_pos-floorPos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,&hit,1);
		if (rayresult)
		{
			pPipeUser->SetSignal(1,"OnProneHideSpot");
			return false;
		}
	}
*/
	// try lowering yourself 1 meter
	// no need to lover self - just check if the hide is as high as you are; if not - it is a lowHideSpot
	ai_pos = pPipeUser->GetPos();
//	ai_pos.z-=1.f;
	rayresult = gAIEnv.pWorld->RayWorldIntersection(ai_pos,target_pos-ai_pos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,
		&hit,1, skipList.empty() ? 0 : &skipList[0], skipList.size());
	if (!rayresult)
	{
		// also switch bodypos automagically
//		if (!pPipeUser->GetParameters().m_bSpecial)

		// try lowering yourself 1 meter - see if this hides me
		ai_pos.z-=1.f;
		rayresult = gAIEnv.pWorld->RayWorldIntersection(ai_pos,target_pos-ai_pos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,
			&hit,1, skipList.empty() ? 0 : &skipList[0], skipList.size());
//		if (rayresult)
		{
			pPipeUser->SetSignal(1,"OnLowHideSpot", 0, 0, gAIEnv.SignalCRCs.m_nOnLowHideSpot);
			return false;
		}
		return true;
	}
	return false;
//	return true;
}


void COPHide::Reset(CPipeUser* pPipeUser)
{
	CCCPOINT(COPHide_Reset);

	if (m_pPathFindDirective)
		delete m_pPathFindDirective;
	m_pPathFindDirective = 0;

	if (m_pTraceDirective) 
		delete m_pTraceDirective;
	m_pTraceDirective = 0;

  if (pPipeUser)
  {
		pPipeUser->SetMovingToCover(false);

    pPipeUser->ClearPath("COPHide::Reset m_Path");
		if ( m_bLookAtLastOp )
		{
			pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
			m_looseAttentionId = 0;
		}
	}

	m_refHideTarget.Release();
}

void COPHide::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPHide");
	{
		m_refHideTarget.Serialize(ser, "m_refHideTarget");

		ser.Value("m_vHidePos",m_vHidePos);
		ser.Value("m_vLastPos",m_vLastPos);
		ser.Value("m_fSearchDistance",m_fSearchDistance);
		ser.Value("m_fMinDistance",m_fMinDistance);
		ser.Value("m_nEvaluationMethod",m_nEvaluationMethod);
		ser.Value("m_bLookAtHide",m_bLookAtHide);
		ser.Value("m_bLookAtLastOp",m_bLookAtLastOp);
		ser.Value("m_bAttTarget",m_bAttTarget);
		ser.Value("m_bEndEffect",m_bEndEffect);
		ser.Value("m_iActionId",m_iActionId);
		ser.Value("m_looseAttentionId",m_looseAttentionId);
		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("PathFindDirective", m_pPathFindDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathFindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
      SAFE_DELETE(m_pPathFindDirective);
			if(ser.BeginOptionalGroup("PathFindDirective", true))
			{
				m_pPathFindDirective = new COPPathFind("");
				m_pPathFindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}

		ser.EndGroup();
	}
}

void COPHide::CreateHideTarget(string sName, const Vec3 &vPos)
{
	CCCPOINT(COPHide_CreateHideTarget);

	assert(m_refHideTarget.IsNil());
	GetAISystem()->CreateDummyObject(m_refHideTarget, sName + "_HideTarget");
	m_refHideTarget.GetAIObject()->SetPos(vPos);
}


////////////////////////////////////////////////////////////////////////////////////

EGoalOpResult COPTacticalPos::Execute(CPipeUser* pPipeUser)
{
	EGoalOpResult	ret = eGOR_FAILED;
	CPuppet *pPuppet = CastToCPuppetSafe(pPipeUser); 
	if (!pPuppet)
		return ret;
	
	CTacticalPointSystem *pTPS = gAIEnv.pTacticalPointSystem;
	CAISystem *pSystem = GetAISystem();
	
	switch(m_state)
	{
	case eTPS_QUERY_INIT:
		{
			// Start an async TPS query

			// Clear any existing path
			pPipeUser->ClearPath("COPTacticalPos::Execute"); // Maybe this could be delayed
			// Store our current hidepos
			m_vLastHidePos = pPipeUser->m_CurrentHideObject.GetObjectPos();
			
			// Set up context
			QueryContext context;
			InitQueryContextFromPuppet(pPipeUser->CastToCPuppet(),context);
			m_queryInstance.SetContext(context);

			// Start the query
			m_queryInstance.Execute(eTPQF_LockResults);
			m_state = eTPS_QUERY;
		}
		// Fall through! Status may have changed immediately.
	case eTPS_QUERY:
		{
			// Wait for results from the TPS query

			ETacticalPointQueryState eTPSState = m_queryInstance.GetStatus();
			switch( eTPSState )
			{
			case eTPSQS_InProgress:
				// Query is still being processed
				{
					ret = eGOR_IN_PROGRESS;
				}
				break;
			case eTPSQS_Error:
				// Got an error - abort this op.
			case eTPSQS_Fail:
				{
					// We found no points so we have nowhere to go - but there were no errors
					Reset(pPipeUser);

					// This doesn't quite make sense if we're going to set a refpoint

					// Wrapping with old methods...
					pPipeUser->SetInCover(false);
					pPipeUser->m_CurrentHideObject.Invalidate();

					ret = eGOR_FAILED;
				}
				break;

			case eTPSQS_Success:
				{
					m_queryInstance.UnlockResults();
					// We found a point successfully, so we can continue with the op
					m_iOptionUsed = m_queryInstance.GetOptionUsed();

					if( m_iOptionUsed < 0 )
						SendStateSignal(pPipeUser, eTPGOpState_NoPointFound);
					else
						SendStateSignal(pPipeUser, eTPGOpState_PointFound);

					STacticalPointResult point = m_queryInstance.GetBestResult();
					assert(point.IsValid());

					// Wrapping old methods...
					pPipeUser->m_CurrentHideObject.Invalidate();
					switch(m_nReg)
					{
					case AI_REG_REFPOINT:
						if (point.flags & eTPDF_AIObject)
						{
							if (CAIObject* pAIObject = static_cast<CAIObject*>(pSystem->GetAIObject(point.aiObjectId)))
							{
								pPipeUser->SetRefPointPos(pAIObject->GetPos(), pAIObject->GetBodyDir());

								Reset(pPipeUser);
								return eGOR_SUCCEEDED;
							}
						}

						// we can expect a position. vObjDir may not be set if this is not a hidespot, but should be zero.
						pPipeUser->SetRefPointPos(point.vPos, point.vObjDir);
						Reset(pPipeUser);
						return eGOR_SUCCEEDED;
					case AI_REG_LASTOP:
						{
							if (CAIObject* pAIObject = static_cast<CAIObject*>(pSystem->GetAIObject(point.aiObjectId)))
							{
								pPipeUser->SetLastOpResult(pAIObject->GetSelfReference());
								Reset(pPipeUser);
								return eGOR_SUCCEEDED;
							}
						}
						return eGOR_FAILED;
					case AI_REG_HIDEOBJECT:
						{
							if (point.flags & eTPDF_Hidespot)
							{
								CRY_ASSERT_MESSAGE(!GetAISystem()->IsHideSpotOccupied(pPipeUser, point.vObjPos),
									"The world is falling apart! Go ahead... call Marcio!");

								SHideSpot	hs(SHideSpotInfo::eHST_ANCHOR, point.vObjPos, point.vObjDir);
								hs.pAnchorObject = static_cast<CAIObject*>(pSystem->GetAIObject(point.aiObjectId));

								pPipeUser->m_CurrentHideObject.Set(&hs, point.vObjPos, point.vObjDir);
								Reset(pPipeUser);
								return eGOR_SUCCEEDED;
							}
						}
						return eGOR_FAILED;
					default:
						if (point.flags & eTPDF_Mask_AbstractHidespot )
						{
							// Wrapping old methods...
							// This doesn't make us go there, more provides debugging and setup when we arrive.

							// (MATT) I'm having to disable this for now, which might be a mistake, until I can expose it cleanly {2009/07/31}
							//pPipeUser->m_CurrentHideObject.Set(&(point.hidespot), point.vPos, point.vObjDir);
						}
					}

					// Have we chosen the same hidespot again?
					if ( pPipeUser->m_CurrentHideObject.IsValid() && m_vLastHidePos.IsEquivalent( pPipeUser->m_CurrentHideObject.GetObjectPos()) )
						pPipeUser->SetSignal(1,"OnSameHidespotAgain", 0, 0, 0);  // TODO: Add CRC

					// TODO : (MATT)  {2007/05/23:11:58:45} workaround - Switching urgency in the hide op isn't good.
					// If it's a short distance to the hidepoint, running generally looks bad, so force to walk
					Vec3 vHideVec = point.vPos - pPipeUser->GetPos();
					float fHideDist = vHideVec.GetLength();

					if (fHideDist < 5.0f)	
						pPipeUser->m_State.fMovementUrgency = AISPEED_WALK;
					if (fHideDist < 0.1f) // TODO: Follow all this execution path thru properly. And simplify.
						SendStateSignal(pPipeUser, eTPGOpState_DestinationReached);

					pSystem->CreateDummyObject(m_refHideTarget, string(GetNameSafe(pPipeUser)) + "_HideTarget");
					m_refHideTarget.GetAIObject()->SetPos(point.vPos);
					pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

					ret = eGOR_IN_PROGRESS;
					m_state = eTPS_PATHFIND_INIT;
				}
			}
		}
		break;

	case eTPS_PATHFIND_INIT:
		if (!m_pPathFindDirective)
		{
			CAIObject *pHideTarget = m_refHideTarget.GetAIObject();
			assert(pHideTarget);

			if (gAIEnv.CVars.DebugPathFinding)
			{
				
				const Vec3 &vHidePos = pHideTarget->GetPos();
				AILogAlways("COPHide::Execute %s pathfinding to (%5.2f, %5.2f, %5.2f)", GetNameSafe(pPipeUser),
					vHidePos.x, vHidePos.y, vHidePos.z);
			}

			// request the path
			m_pPathFindDirective = new COPPathFind("",pHideTarget);
			ret = eGOR_IN_PROGRESS;
		}
		m_state = eTPS_PATHFIND;
		//fallthrough
	case eTPS_PATHFIND:
		ret = eGOR_IN_PROGRESS; // Always in progress if we are at the pathfind state! (Let path finder do its job)
		if (m_pPathFindDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
		{
			CAIObject *pHideTarget = m_refHideTarget.GetAIObject();
			if (pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND)
			{
				if (gAIEnv.CVars.DebugPathFinding)
				{
					// We should have hide target
					const Vec3 &vHidePos = pHideTarget->GetPos();
					AILogAlways("COPHide::Execute %s Creating trace to hide target (%5.2f, %5.2f, %5.2f)", GetNameSafe(pPipeUser),
						vHidePos.x, vHidePos.y, vHidePos.z);
				}

				m_state = eTPS_TRACE_INIT;
			}
			else
			{
				// Could not reach the point, mark it ignored so that we do not try to pick it again.
				pPipeUser->IgnoreCurrentHideObject(10.0f);

				Reset(pPipeUser);
				SendStateSignal(pPipeUser,eTPGOpState_DestinationReached);

				ret = eGOR_SUCCEEDED;
			}
		}
		break;
	case eTPS_TRACE_INIT:
		{
			if (!m_pTraceDirective)
				m_pTraceDirective = new COPTrace(m_bLookAtHide, defaultTraceEndAccuracy);
			m_state = eTPS_TRACE;
		}
		//falltrough
	case eTPS_TRACE:
		{
			ret = eGOR_IN_PROGRESS;
			if (m_pTraceDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
			{
				Reset(pPipeUser);
				SendStateSignal(pPipeUser,eTPGOpState_DestinationReached);

				ret = eGOR_SUCCEEDED;
			}
			break;
		}
	}
	return ret;
}


void COPTacticalPos::ExecuteDry(CPipeUser* pPipeUser) 
{
	if (m_pTraceDirective)
		m_pTraceDirective->ExecuteTrace(pPipeUser, false);
}

bool COPTacticalPos::IsBadHiding(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_AI );
	
	IAIObject *pTarget = pPipeUser->GetAttentionTarget();

	if (!pTarget)
		return false;

	Vec3 ai_pos = pPipeUser->GetPos();
	Vec3 target_pos = pTarget->GetPos();
	ray_hit hit;
	IPhysicalEntity	*skipList[4];
	int skipCount(1);
	skipList[0] = pPipeUser->GetProxy()->GetPhysics();
	if(pTarget->GetProxy() && pTarget->GetProxy()->GetPhysics())
	{
		skipList[skipCount++] = pTarget->GetProxy()->GetPhysics();
		if(pTarget->GetProxy()->GetPhysics(true))
			skipList[skipCount++] = pTarget->GetProxy()->GetPhysics(true);
	}
	int rayresult(0);
		rayresult = gAIEnv.pWorld->RayWorldIntersection(target_pos,ai_pos-target_pos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,&hit,1, &skipList[0], skipCount);
		if (rayresult)
		{
			// check possible leaning direction
			if ((hit.pt-ai_pos).GetLengthSquared()<9.f)
			{
				Vec3 dir = ai_pos-target_pos;
				float zcross =  dir.y*hit.n.x - dir.x*hit.n.y;
				if (zcross < 0)
					pPipeUser->SetSignal(1,"OnRightLean", 0, 0, gAIEnv.SignalCRCs.m_nOnRightLean);
				else
					pPipeUser->SetSignal(1,"OnLeftLean", 0, 0, gAIEnv.SignalCRCs.m_nOnLeftLean);
			}
			return false;
		}

		
		// try lowering yourself 1 meter
		// no need to lover self - just check if the hide is as high as you are; if not - it is a lowHideSpot
		ai_pos = pPipeUser->GetPos();
		//	ai_pos.z-=1.f;
			rayresult = gAIEnv.pWorld->RayWorldIntersection(ai_pos,target_pos-ai_pos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,&hit,1, &skipList[0], skipCount);
			if (!rayresult)
			{
				// try lowering yourself 1 meter - see if this hides me
				ai_pos.z-=1.f;
					rayresult = gAIEnv.pWorld->RayWorldIntersection(ai_pos,target_pos-ai_pos,COVER_OBJECT_TYPES,HIT_COVER|HIT_SOFT_COVER,&hit,1, &skipList[0], skipCount);
					//		if (rayresult)
				{
					pPipeUser->SetSignal(1,"OnLowHideSpot", 0, 0, gAIEnv.SignalCRCs.m_nOnLowHideSpot);
					return false;
				}
				return true;
			}
			return false;
}

COPTacticalPos::COPTacticalPos(const XmlNodeRef& node) :
	m_nReg(AI_REG_NONE),
	m_state(eTPS_QUERY_INIT),
	m_pPathFindDirective(0),
	m_pTraceDirective(0),
	m_iOptionUsed(-1),
	m_bLookAtHide(false)
{
	CTacticalPointSystem* pTPS = gAIEnv.pTacticalPointSystem;
	int tacQueryID = 0;
	
	const char* szQueryName = node->getAttr("name");
	if (stricmp(szQueryName, ""))
	{
		tacQueryID = pTPS->GetQueryID(szQueryName);
		if (!tacQueryID)
		{
			AIWarning("Goalop 'TacticalPos': Query '%s' does not exist (yet).", szQueryName);
		}
	}
	else
	{
		if (s_xml.GetMandatory(node, "id", tacQueryID))
		{
			if (!pTPS->GetQueryName(tacQueryID))
			{
				AIError("Goalop 'TacticalPos': Query with id %d could not be found.", tacQueryID);
			}
		}
		else
		{
			AIError("Goalop 'TacticalPos': Neither query name nor id was provided.");
		}
	}

	if (tacQueryID)
	{
		s_xml.GetRegister(node, "register", m_nReg);
	}

	Reset(NULL);
	m_queryInstance.SetQueryID(tacQueryID);
}

COPTacticalPos::~COPTacticalPos() 
{
	Reset(NULL);
}

void COPTacticalPos::Reset(CPipeUser* pPipeUser)
{ 
	m_state = eTPS_QUERY_INIT;

	m_queryInstance.UnlockResults();
	m_queryInstance.Cancel();

	m_vLastHidePos.zero();

	SAFE_DELETE(m_pPathFindDirective);
	SAFE_DELETE(m_pTraceDirective);

	if (pPipeUser)
	{
//		pPipeUser->SetInCover(false);
		pPipeUser->SetMovingToCover(false);
		pPipeUser->ClearPath("COPHide::Reset m_Path");
	}

	m_refHideTarget.Release();
}

////////////////////////////////////////////////////////////////////////////////////

void COPTacticalPos::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPTacticalPos");
	{
		m_refHideTarget.Serialize(ser, "m_refHideTarget");
		
		ser.Value("m_vHidePos",m_vHidePos);
		ser.Value("m_vLastPos",m_vLastPos);
		ser.Value("m_bLookAtHide",m_bLookAtHide);
		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("PathFindDirective", m_pPathFindDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathFindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
      SAFE_DELETE(m_pPathFindDirective);
			if(ser.BeginOptionalGroup("PathFindDirective", true))
			{
				m_pPathFindDirective = new COPPathFind("");
				m_pPathFindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}

		ser.EndGroup();
	}
}

////////////////////////////////////////////////////////////////////////////////////

void COPTacticalPos::SendStateSignal(CPipeUser* pPipeUser, int nState)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_AI );
		
	CTacticalPointSystem *pTPS = gAIEnv.pTacticalPointSystem;
	IAISignalExtraData *pEData = gEnv->pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SetSignal

	int queryID = m_queryInstance.GetQueryID();
	pEData->string1 = pTPS->GetQueryName( queryID );
	if( nState != eTPGOpState_NoPointFound )
	{
		pEData->string2 = pTPS->GetOptionLabel( m_queryInstance.GetQueryID(), m_iOptionUsed );
	}

	switch( nState )
	{
	case eTPGOpState_NoPointFound:
		pPipeUser->SetSignal(1,"OnTPSDestNotFound", 0, pEData, gAIEnv.SignalCRCs.m_nOnTPSDestinationNotFound);
		break;
	case eTPGOpState_PointFound:
		pPipeUser->SetSignal(1,"OnTPSDestFound", 0, pEData, gAIEnv.SignalCRCs.m_nOnTPSDestinationFound);
		break;
	case eTPGOpState_DestinationReached:
		pPipeUser->SetSignal(1,"OnTPSDestReached", 0, pEData, gAIEnv.SignalCRCs.m_nOnTPSDestinationReached);
		break;
	}	
}

////////////////////////////////////////////////////////////////////////////////////

COPLook::COPLook( int lookMode, bool bBodyTurn, int nReg ) : m_nReg(nReg)
{
	m_bInitialised = false;
	m_fTimeLeft = 0.0f;


	// Pick the soft/hard lookstyles that match the bodyturn parameter
	ELookStyle eSoft, eHard;
	if (bBodyTurn)
	{
		eSoft = LOOKSTYLE_SOFT;
		eHard = LOOKSTYLE_HARD;
	}
	else
	{
		eSoft = LOOKSTYLE_SOFT_NOLOWER;
		eHard = LOOKSTYLE_HARD_NOLOWER;
	}

	// Convert into lookstyles
	m_eLookThere = LOOKSTYLE_DEFAULT;
	m_eLookBack = LOOKSTYLE_DEFAULT;

	switch(lookMode)
	{
	case AILOOKMOTIVATION_LOOK:
		m_fLookTime = 1.5f;
		m_eLookThere = eSoft;
		m_eLookBack = eSoft;
		break;
	case AILOOKMOTIVATION_GLANCE:
		m_fLookTime = 0.8f;
		m_eLookThere = eHard;
		m_eLookBack = eHard;
		break;
	case AILOOKMOTIVATION_STARTLE:
		m_fLookTime = 0.8f;
		m_eLookThere = eHard;
		m_eLookBack = eSoft;
		break;
	case AILOOKMOTIVATION_DOUBLETAKE:
		m_fLookTime = 1.2f;
		m_eLookThere = eSoft;
		m_eLookBack = eHard;
		break;
	default: 
		assert(false && "Unknown look motivation");
	}
}

COPLook::COPLook(const XmlNodeRef& node) :
	m_bInitialised(false),
	m_fTimeLeft(0.f)
{
	// Pick the soft/hard look styles that match the body turn parameter
	ELookStyle eSoft, eHard;
	if (s_xml.GetBool(node, "bodyTurn", true))
	{
		eSoft = LOOKSTYLE_SOFT;
		eHard = LOOKSTYLE_HARD;
	}
	else
	{
		eSoft = LOOKSTYLE_SOFT_NOLOWER;
		eHard = LOOKSTYLE_HARD_NOLOWER;
	}

	// Convert into look styles
	m_eLookThere = LOOKSTYLE_DEFAULT;
	m_eLookBack = LOOKSTYLE_DEFAULT;

	ELookMotivation eLookMode;
	if (s_xml.GetLook(node, "id", eLookMode, CGoalOpXMLReader::MANDATORY))
	{
		switch (eLookMode)
		{
		case AILOOKMOTIVATION_LOOK:
			m_fLookTime = 1.5f;
			m_eLookThere = eSoft;
			m_eLookBack = eSoft;
			break;
		case AILOOKMOTIVATION_GLANCE:
			m_fLookTime = 0.8f;
			m_eLookThere = eHard;
			m_eLookBack = eHard;
			break;
		case AILOOKMOTIVATION_STARTLE:
			m_fLookTime = 0.8f;
			m_eLookThere = eHard;
			m_eLookBack = eSoft;
			break;
		case AILOOKMOTIVATION_DOUBLETAKE:
			m_fLookTime = 1.2f;
			m_eLookThere = eSoft;
			m_eLookBack = eHard;
			break;
		default: 
			assert(false && "Unknown look motivation");
		}
	}
}

COPLook::~COPLook() 
{
}

EGoalOpResult COPLook::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPLook_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAISystem *pAISystem = GetAISystem();
	CPuppet *pPuppet = CastToCPuppetSafe(pPipeUser);
	if (!pPuppet) return eGOR_FAILED;

	// Initialisation
	if (!m_bInitialised)
	{
		// Get the look target
		CWeakRef<CAIObject> refLookTarget;
		switch (m_nReg)
		{
			case AI_REG_LASTOP:
				refLookTarget = GetWeakRef(pPuppet->GetLastOpResult());
				break;
			case AI_REG_REFPOINT:
				refLookTarget = GetWeakRef(pPuppet->GetRefPoint());
				break;
			default:
				AIError("COPLook failed - unknown AI register");
		}

		if (!refLookTarget.IsValid()) return eGOR_FAILED;

		m_nLookID = pPipeUser->SetLooseAttentionTarget( refLookTarget, m_nLookID);
		pPipeUser->SetLookStyle( m_eLookThere );
		pPipeUser->SetLookAtPointPos( refLookTarget.GetAIObject()->GetPos(), true/*priority: overrides other possible look targets*/ );
		m_fTimeLeft = m_fLookTime;
		m_bInitialised = true;
	}

	// Just wait for time to expire
	// Should probably sit in ExecuteDry

	// We can now check pPuppet->GetLookIKStatus() to decide when to end
	// A value of 1.0f means that we're now looking in the right direction
	// A value < 0 means that Look IK will not arrive at desired target (for various possible reasons)
	// So we should quite on this condition also.
	//CryLogAlways( "look IK status [%f]", pPuppet->GetLookIKStatus() );
	
	//m_fTimeLeft -= pAISystem->GetFrameDeltaTime(); Not accurate
	m_fTimeLeft -= pAISystem->GetUpdateInterval();
	if (m_fTimeLeft < 0.0f)
	{
		Reset(pPipeUser);
		return eGOR_SUCCEEDED;
	}
	

	return eGOR_IN_PROGRESS;
}

void COPLook::ExecuteDry(CPipeUser* pPipeUser) 
{
}

void COPLook::Reset(CPipeUser* pPipeUser)
{
	pPipeUser->SetLooseAttentionTarget(NILREF,m_nLookID);
	pPipeUser->SetLookStyle( m_eLookBack );
	pPipeUser->ResetLookAt();
	m_bInitialised = false;
}

void COPLook::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPLook");
	{
		// TODO : (MATT) Insert serialisation code here {2007/09/14:14:55:01}
		assert(false && "Insert code here");
		ser.EndGroup();
	}
}


//
//----------------------------------------------------------------------------------------------------------

EGoalOpResult COPForm::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	pPipeUser->CreateFormation(m_sName.c_str());
	return eGOR_DONE;
}

EGoalOpResult COPClear::Execute(CPipeUser* pPipeUser)
{
	return eGOR_DONE;
}


//
//----------------------------------------------------------------------------------------------------------
COPStick::COPStick(float fStickDistance, float fEndAccuracy, float fDuration, int flags, int flagsAux, COPTrace::EEndMode endMode):
/*                   bool bUseLastOp, bool bLookatLastOp, bool bContinuous, 
                   bool bTryShortcutNavigation, bool bForceReturnPartialPath,
bool bStopOnAnimationStart, bool bConstantSpeed):
(params.nValue & AILASTOPRES_USE) != 0, (params.nValue & AILASTOPRES_LOOKAT) != 0,
(params.nValueAux & 0x01) == 0, (params.nValueAux & 0x02) != 0, (params.nValue & AI_REQUEST_PARTIAL_PATH)!=0,
(params.nValue & AI_STOP_ON_ANIMATION_START)!=0, (params.nValue & AI_CONSTANT_SPEED)!=0);
*/
m_vLastUsedTargetPos(0,0,0),
m_fTrhDistance(1.0f),	// regenerate path if target moved for more than this
m_EndMode(endMode),
m_fApproachTime(-1.0f),
m_fHijackDistance(-1.0f),
m_fStickDistance(fStickDistance),
m_fEndAccuracy(fEndAccuracy),
m_fDuration(fDuration),
m_bContinuous((flagsAux & 0x01) == 0),
m_bTryShortcutNavigation((flagsAux & 0x02) != 0),
m_bUseLastOpResult((flags & AILASTOPRES_USE) != 0),
m_bLookAtLastOp((flags & AILASTOPRES_LOOKAT) != 0),
m_bInitialized(false),
m_bForceReturnPartialPath((flags & AI_REQUEST_PARTIAL_PATH)!=0),
m_stopOnAnimationStart((flags & AI_STOP_ON_ANIMATION_START)!=0),
m_targetPredictionTime(0.0f),
m_pTraceDirective(NULL),
m_pPathfindDirective(NULL),
m_looseAttentionId(0),
m_bPathFound(false),
m_bSteerAroundPathTarget((flags & AI_DONT_STEER_AROUND_TARGET)==0)
{
  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("COPStick::COPStick %p", this);
  m_smoothedTargetVel.zero();
  m_lastTargetPos.zero();
  m_safePointInterval = 1.0f;
  m_maxTeleportSpeed = 10.0f;
  m_pathLengthForTeleport = 20.0f;
  m_playerDistForTeleport = 3.0f;

  m_lastVisibleTime.SetValue(0);
  ClearTeleportData();

	if (m_bContinuous)
	{
		// Continuous stick defaults to speed adjustment, allow to make it constant.
		if ((flags & AI_CONSTANT_SPEED) !=0)
			m_bConstantSpeed = true;
		else
			m_bConstantSpeed = false;
}
	else
	{
		// Non-continuous stick defaults to constant speed, allow to enable adjustment.
		if ((flags & AI_ADJUST_SPEED) !=0)
			m_bConstantSpeed = false;
		else
			m_bConstantSpeed = true;
	}
}

//
//----------------------------------------------------------------------------------------------------------
COPStick::COPStick(const XmlNodeRef& node) :
	m_vLastUsedTargetPos(ZERO),
	m_fTrhDistance(1.f),	// regenerate path if target moved for more than this
	m_EndMode(stricmp(node->getTag(), "Stick") ? COPTrace::eEM_MinimumDistance : COPTrace::eEM_FixedDistance),
	m_fApproachTime(-1.f),
	m_fHijackDistance(-1.f),
	m_fStickDistance(0.f),
	m_fEndAccuracy(0.f),
	m_fDuration(0.f),
	m_bContinuous(s_xml.GetBool(node, "continuous", false)),
	m_bTryShortcutNavigation(s_xml.GetBool(node, "tryShortcutNavigation")),
	m_bUseLastOpResult(s_xml.GetBool(node, "useLastOp")),
	m_bLookAtLastOp(s_xml.GetBool(node, "lookAtLastOp")),
	m_bInitialized(false),
	m_bForceReturnPartialPath(s_xml.GetBool(node, "requestPartialPath")),
	m_stopOnAnimationStart(s_xml.GetBool(node, "stopOnAnimationStart")),
	m_targetPredictionTime(0.f),
	m_pTraceDirective(0),
	m_pPathfindDirective(0),
	m_looseAttentionId(0),
	m_bPathFound(false),
	m_bSteerAroundPathTarget(s_xml.GetBool(node, "steerAroundTarget", true)),
	m_smoothedTargetVel(ZERO),
	m_lastTargetPos(ZERO),
	m_safePointInterval(1.f),
	m_maxTeleportSpeed(10.f),
	m_pathLengthForTeleport(20.f),
	m_playerDistForTeleport(3.f)
{
	if (node->getAttr("duration", m_fDuration))
	{
		m_fDuration = fabsf(m_fDuration);
	}
	else
	{
		node->getAttr("distance", m_fStickDistance);
	}
	
	if (!node->getAttr("endAccuracy", m_fEndAccuracy))
	{
		m_fEndAccuracy = m_fStickDistance;
	}

	// Random variation on the end distance.
	float fDistanceVariation;
	if (node->getAttr("distanceVariation", fDistanceVariation))
	{
		if (fDistanceVariation > 0.01f)
		{
			float u = (ai_rand() % 10) / 9.f;
			m_fStickDistance = (m_fStickDistance > 0.f) ? max(0.f, m_fStickDistance - u * fDistanceVariation) 
			                                            : min(0.f, m_fStickDistance + u * fDistanceVariation);
		}
	}
	
	if (gAIEnv.CVars.DebugPathFinding)
	{
		AILogAlways("COPStick::COPStick %p", this);
	}
	
	m_lastVisibleTime.SetValue(0);
	ClearTeleportData();

	if (m_bContinuous)
	{
		// Continuous stick defaults to speed adjustment, allow to make it constant.
		m_bConstantSpeed = s_xml.GetBool(node, "constantSpeed");
	}
	else
	{
		// Non-continuous stick defaults to constant speed, allow to enable adjustment.
		m_bConstantSpeed = !s_xml.GetBool(node, "adjustSpeed");
	}
}

//
//----------------------------------------------------------------------------------------------------------
COPStick::~COPStick()
{
	SAFE_DELETE(m_pPathfindDirective);
	SAFE_DELETE(m_pTraceDirective);

  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("COPStick::~COPStick %p", this);
}


//
//----------------------------------------------------------------------------------------------------------
void COPStick::Reset(CPipeUser* pPipeUser)
{
	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPStick::Reset %s", pPipeUser ? GetNameSafe(pPipeUser) : "");
	m_refStickTarget.Reset();
	m_refSightTarget.Reset();

	SAFE_DELETE(m_pPathfindDirective);
	SAFE_DELETE(m_pTraceDirective);

	m_bPathFound = false;
	m_vLastUsedTargetPos.zero();	

	m_smoothedTargetVel.zero();
	m_lastTargetPos.zero();

	ClearTeleportData();

	if (pPipeUser)
  {
		pPipeUser->ClearPath("COPStick::Reset m_Path");
		if(m_bLookAtLastOp)
		{
			pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
			m_looseAttentionId = 0;
		}
  }
}

//
//----------------------------------------------------------------------------------------------------------
void COPStick::SSafePoint::Serialize(TSerialize ser)
{
	ser.BeginGroup("StickSafePoint");
	ser.Value("pos",pos);
	gAIEnv.pGraph->SerializeNodePointer(ser, "node", nodeIndex);
	ser.Value("safe",safe);
	ser.Value("time",time);
	ser.EndGroup();
}

//
//----------------------------------------------------------------------------------------------------------
void COPStick::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{

	ser.BeginGroup("COPStick");
	{
		ser.Value("m_vLastUsedTargetPos",m_vLastUsedTargetPos);
		ser.Value("m_fTrhDistance",m_fTrhDistance);
		ser.Value("m_fStickDistance",m_fStickDistance);
		ser.Value("m_fEndAccuracy",m_fEndAccuracy);
    ser.Value("m_fDuration", m_fDuration);
		ser.Value("m_bContinuous",m_bContinuous);
		ser.Value("m_bLookAtLastOp",m_bLookAtLastOp);
		ser.Value("m_bTryShortcutNavigation",m_bTryShortcutNavigation);
		ser.Value("m_bUseLastOpResult",m_bUseLastOpResult);
    ser.Value("m_targetPredictionTime", m_targetPredictionTime);
		ser.Value("m_bPathFound", m_bPathFound);
		ser.Value("m_bInitialized",m_bInitialized);
		ser.Value("m_bConstantSpeed",m_bConstantSpeed);
		ser.Value("m_teleportCurrent",m_teleportCurrent);
		ser.Value("m_teleportEnd",m_teleportEnd);
		ser.Value("m_lastTeleportTime",m_lastTeleportTime);
		ser.Value("m_lastVisibleTime",m_lastVisibleTime);
		ser.Value("m_maxTeleportSpeed",m_maxTeleportSpeed);
		ser.Value("m_pathLengthForTeleport",m_pathLengthForTeleport);
		ser.Value("m_playerDistForTeleport",m_playerDistForTeleport);
		ser.Value("m_bForceReturnPartialPath",m_bForceReturnPartialPath);
    ser.Value("m_stopOnAnimationStart", m_stopOnAnimationStart);
		ser.Value("m_lastTargetPosTime",m_lastTargetPosTime);
		ser.Value("m_lastTargetPos",m_lastTargetPos);
		ser.Value("m_smoothedTargetVel",m_smoothedTargetVel);
		ser.Value("m_looseAttentionId",m_looseAttentionId);
		ser.Value("m_bSteerAroundPathTarget",m_bSteerAroundPathTarget);

    SSafePoint::pObjectTracker = &objectTracker; ser.Value("m_stickTargetSafePoints", m_stickTargetSafePoints);
		ser.Value("m_safePointInterval",m_safePointInterval);

		m_refStickTarget.Serialize(ser,"m_refStickTarget");
		m_refSightTarget.Serialize(ser,"m_refSightTarget");

		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("PathFindDirective", m_pPathfindDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
      SAFE_DELETE(m_pPathfindDirective);
			if(ser.BeginOptionalGroup("PathFindDirective", true))
			{
				m_pPathfindDirective = new COPPathFind("");
				m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
  }
		ser.EndGroup();
	}

//===================================================================
// GetEndDistance
//===================================================================
float COPStick::GetEndDistance(CPipeUser* pPipeUser) const
{
	if (m_fDuration > 0.0f)
	{
		float normalSpeed, minSpeed, maxSpeed;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, false, normalSpeed, minSpeed, maxSpeed);
		if (normalSpeed > 0.0f)
			return -normalSpeed * m_fDuration;
	}
	return m_fStickDistance;
}

//
//----------------------------------------------------------------------------------------------------------
void COPStick::RegeneratePath(CPipeUser* pPipeUser, const Vec3 &destination)
{
	if (!pPipeUser)
		return;

  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("COPStick::RegeneratePath %s", pPipeUser ? GetNameSafe(pPipeUser) : "");
	m_pPathfindDirective->Reset(pPipeUser);
	m_pTraceDirective->m_fEndAccuracy = m_fEndAccuracy;
	m_vLastUsedTargetPos = destination;
	pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

  const Vec3 opPos = pPipeUser->GetPhysicsPos();

	// Check for direct connection first
	if (m_bTryShortcutNavigation)
	{
		int nbid;
		IVisArea *iva;
		IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(opPos, nbid, iva, pPipeUser->m_movementAbility.pathfindingProperties.navCapMask);
		CNavRegion *pRegion = gAIEnv.pNavigation->GetNavRegion(navType, gAIEnv.pGraph);
		if (pRegion)
		{
			Vec3	from = opPos;
			Vec3	to = destination;

			NavigationBlockers	navBlocker;
			if(pRegion->CheckPassability(from, to, pPipeUser->GetParameters().m_fPassRadius, navBlocker, pPipeUser->GetMovementAbility().pathfindingProperties.navCapMask))
			{
				pPipeUser->ClearPath("COPStick::RegeneratePath m_Path");
				
				if (navType == IAISystem::NAV_TRIANGULAR)
				{
					// Make sure not to enter forbidden area.
					if (gAIEnv.pNavigation->IsPathForbidden(opPos, destination))
						return;
					if (gAIEnv.pNavigation->IsPointForbidden(destination, pPipeUser->GetParameters().m_fPassRadius))
						return;
				}

				PathPointDescriptor	pt;
				pt.navType = navType;

				pt.vPos = from;
				pPipeUser->m_Path.PushBack(pt);

				float endDistance = GetEndDistance(pPipeUser);
				if (fabsf(endDistance) > 0.0001f)
				{
					// Handle end distance.
					float dist;
					if (pPipeUser->IsUsing3DNavigation())
						dist = Distance::Point_Point(from, to);
					else
						dist = Distance::Point_Point2D(from, to);

					float d;
					if (endDistance > 0.0f)
						d = dist - endDistance;
					else
						d = -endDistance;
					float u = clamp(d / dist, 0.0001f, 1.0f);

					pt.vPos = from + u * (to - from);

					pPipeUser->m_Path.PushBack(pt);
				}
				else
				{
					pt.vPos = to;
					pPipeUser->m_Path.PushBack(pt);
				}

				pPipeUser->m_Path.SetParams(SNavPathParams(from, to, Vec3(ZERO), Vec3(ZERO), -1, false, endDistance, true));

				pPipeUser->m_OrigPath = pPipeUser->m_Path;
				pPipeUser->m_nPathDecision = PATHFINDER_PATHFOUND;
			}
		}
	}
}

//===================================================================
// DebugDraw
//===================================================================
void COPStick::DebugDraw(CPipeUser* pPipeUser) const
{
	CDebugDrawContext dc;
  unsigned nPts = m_stickTargetSafePoints.Size();
  unsigned iPt = 0;
  TStickTargetSafePoints::SConstIterator itEnd = m_stickTargetSafePoints.End();
  for (TStickTargetSafePoints::SConstIterator it = m_stickTargetSafePoints.Begin() ; it != itEnd ; ++it, ++iPt)
  {
    const SSafePoint &safePoint = *it;
    float frac = ((float) iPt ) / nPts;
		ColorB color;
    if (safePoint.safe)
      color.Set(0, 255 * frac, 255 * (1.0f - frac));
    else
      color.Set(255, 0, 0);
    dc->DrawSphere(safePoint.pos, 0.2f, color);
  }
  
	if(m_pPathfindDirective)
		m_pPathfindDirective->DebugDraw(pPipeUser);
	if(m_pTraceDirective)
		m_pTraceDirective->DebugDraw(pPipeUser);
}

//===================================================================
// UpdateStickTargetSafePositions
//===================================================================
void COPStick::UpdateStickTargetSafePoints(CPipeUser* pPipeUser)
{
  Vec3 curPos;
  CLeader* pLeader = GetAISystem()->GetLeader(pPipeUser->GetGroupId());
  CAIObject *pOwner = pLeader ? pLeader->GetFormationOwner().GetAIObject() : 0;
  if (pOwner)
    curPos = pOwner->GetPhysicsPos();
  else
    curPos = m_refStickTarget.GetAIObject()->GetPhysicsPos();

  Vec3 opPos = pPipeUser->GetPhysicsPos();
  if (GetAISystem()->WouldHumanBeVisible(opPos, false))
    m_lastVisibleTime = GetAISystem()->GetFrameStartTime();

  unsigned lastNavNodeIndex = m_refStickTarget.GetAIObject()->m_lastNavNodeIndex;
  if (!m_stickTargetSafePoints.Empty())
  {
    Vec3 delta = curPos - m_stickTargetSafePoints.Front().pos;
    float dist = delta.GetLength();
    if (dist < m_safePointInterval)
      return;
    lastNavNodeIndex = m_stickTargetSafePoints.Front().nodeIndex;
  }

	unsigned int curNodeIndex = gAIEnv.pGraph->GetEnclosing(
    curPos, pPipeUser->m_movementAbility.pathfindingProperties.navCapMask,
		pPipeUser->m_Parameters.m_fPassRadius, lastNavNodeIndex, 0.0f, 0, false, GetNameSafe(pPipeUser));
  const GraphNode *pCurNode = gAIEnv.pGraph->GetNode(curNodeIndex);

  if (pCurNode && pCurNode->navType == IAISystem::NAV_TRIANGULAR && pCurNode->GetTriangularNavData()->isForbidden)
	{
    pCurNode = 0;
		curNodeIndex = 0;
	}

  int maxNumPoints = 1 + (int)(m_pathLengthForTeleport / m_safePointInterval);
  if (m_stickTargetSafePoints.Size() >= maxNumPoints || m_stickTargetSafePoints.Full())
    m_stickTargetSafePoints.PopBack();

  // incur copy cost by adding to the start, but after the initial hit, no allocation costs. 
  // Also, this way allows us to easily remove points from the end of the list. 
  // This list should contain 
  bool safe = pCurNode != 0;

  Vec3 floorPos;
  if (safe && !GetFloorPos(floorPos, curPos, 0.0f, 3.0f, 0.1f, AICE_STATIC))
    safe = false;

  m_stickTargetSafePoints.PushFront(SSafePoint(curPos, curNodeIndex, safe, GetAISystem()->GetFrameStartTime()));
}

//===================================================================
// ClearTeleportData
//===================================================================
void COPStick::ClearTeleportData()
{
  m_teleportCurrent.zero();
  m_teleportEnd.zero();
}

//===================================================================
// TryToTeleport
// This works by first detecting if we need to teleport. If we do, then
// we identify and store the teleport destination location. However, we
// don't move pPipeUser there immediately - we continue following our path. But we
// move a "ghost" from the initial position to the destination. If it
// reaches the destination without either the ghost or the real pPipeUser
// being seen by the player then pPipeUser (may) get teleported. If teleporting
// happens then m_stickTargetSafePoints needs to be updated.
//===================================================================
bool COPStick::TryToTeleport(CPipeUser * pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  if (m_stickTargetSafePoints.Size() < 2)
  {
    ClearTeleportData();
    return false;
  }

  if (pPipeUser->m_nPathDecision != PATHFINDER_NOPATH)
  {
    float curPathLen = pPipeUser->m_Path.GetPathLength(false);
    Vec3 stickPos = m_refStickTarget.GetAIObject()->GetPhysicsPos();
    if (pPipeUser->m_Path.Empty())
      curPathLen += Distance::Point_Point(pPipeUser->GetPhysicsPos(), stickPos);
    else
      curPathLen += Distance::Point_Point(pPipeUser->m_Path.GetLastPathPos(), stickPos);
    if (curPathLen < m_pathLengthForTeleport)
    {
      ClearTeleportData();
      return false;
    }
    else
    {
      // Danny/Luc todo issue some readability by sending a signal - the response to that signal 
      // should ideally stop the path following 
    }
  }

  if (m_lastVisibleTime == GetAISystem()->GetFrameStartTime())
  {
    ClearTeleportData();
    return false;
  }

  if (m_teleportEnd.IsZero() && !m_stickTargetSafePoints.Empty())
  {
    const Vec3 playerPos = GetAISystem()->GetPlayer()->GetPhysicsPos();
    unsigned nPts = m_stickTargetSafePoints.Size();

    int index = 0;
    TStickTargetSafePoints::SIterator itEnd = m_stickTargetSafePoints.End();
    for (TStickTargetSafePoints::SIterator it = m_stickTargetSafePoints.Begin() ; it != itEnd ; ++it, ++index)
    {
      const SSafePoint &sp = *it;
      if (!sp.safe)
        return false;

      float playerDistSq = Distance::Point_PointSq(sp.pos, playerPos);
      if (playerDistSq < square(m_playerDistForTeleport))
        continue;

      TStickTargetSafePoints::SIterator itNext = it;
      ++itNext;
      if (itNext == itEnd)
      {
        m_teleportEnd = sp.pos;
        break;
      }
      else if (!itNext->safe)
      {
        m_teleportEnd = sp.pos;
        break;
      }
    }
  }

  if (m_teleportEnd.IsZero())
    return false;

  Vec3 curPos = pPipeUser->GetPhysicsPos();
  if (m_teleportCurrent.IsZero())
  {
    m_teleportCurrent = curPos;
    m_lastTeleportTime = GetAISystem()->GetFrameStartTime();

    // If player hasn't seen operand for X seconds then move along by that amount
    Vec3 moveDir = m_teleportEnd - m_teleportCurrent;
    float distToEnd = moveDir.NormalizeSafe();

    float dt = (GetAISystem()->GetFrameStartTime() - m_lastVisibleTime).GetSeconds();
    float dist = dt * m_maxTeleportSpeed;
    if (dist > distToEnd)
      dist = distToEnd;
    m_teleportCurrent += dist * moveDir;

    return false;
  }

  // move the ghost
  Vec3 moveDir = m_teleportEnd - m_teleportCurrent;
  float distToEnd = moveDir.NormalizeSafe();

  CTimeValue thisTime = GetAISystem()->GetFrameStartTime();
  float dt = (thisTime - m_lastTeleportTime).GetSeconds();
  m_lastTeleportTime = thisTime;

  float dist = dt * m_maxTeleportSpeed;
  bool reachedEnd = false;
  if (dist > distToEnd)
  {
    reachedEnd = true;
    dist = distToEnd;
  }
  m_teleportCurrent += dist * moveDir;

  if (GetAISystem()->WouldHumanBeVisible(m_teleportCurrent, false))
  {
    ClearTeleportData();
    return false;
  }

  if (reachedEnd)
  {
    AILogEvent("COPStick::TryToTeleport teleporting %s to (%5.2f, %5.2f, %5.2f)", 
      GetNameSafe(pPipeUser), m_teleportEnd.x, m_teleportEnd.y, m_teleportEnd.z);
    Vec3 floorPos = m_teleportEnd;
    GetFloorPos(floorPos, m_teleportEnd, 0.0f, 3.0f, 0.1f, AICE_ALL);
    pPipeUser->GetEntity()->SetPos(floorPos );
    pPipeUser->m_State.fDesiredSpeed = 0;
    pPipeUser->m_State.vMoveDir.zero();
    pPipeUser->ClearPath("Teleport");
    RegeneratePath(pPipeUser, m_refStickTarget.GetAIObject()->GetPos());

    unsigned nPts = m_stickTargetSafePoints.Size();
    unsigned nBack = 10;
    unsigned iPt = 0;
    TStickTargetSafePoints::SIterator itEnd = m_stickTargetSafePoints.End();
    for (TStickTargetSafePoints::SIterator it = m_stickTargetSafePoints.Begin() ; it != itEnd ; ++it, ++iPt)
    {
      SSafePoint &sp =  *it;
      if (iPt == nPts-1 || sp.pos == m_teleportEnd)
      {
        iPt -= min(iPt, nBack);
        TStickTargetSafePoints::SIterator itFirst = m_stickTargetSafePoints.Begin();
        itFirst += iPt;
        m_stickTargetSafePoints.Erase(itFirst, m_stickTargetSafePoints.End());
        break;
      }
    }
    ClearTeleportData();
    return true;
  }

  return false;
}

//
//----------------------------------------------------------------------------------------------------------
EGoalOpResult COPStick::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPStick_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAISystem *pSystem = GetAISystem();

	// Check to see if objects have disappeared since last call
	// Note that at least in the first iteration, they won't be there to start with
	if ( (m_refSightTarget.IsSet() && !m_refSightTarget.IsValid()) ||
			 (m_refStickTarget.IsSet() && !m_refStickTarget.IsValid()) )
	{
		CCCPOINT(COPStick_Execute_TargetRemoved);
		if (gAIEnv.CVars.DebugPathFinding)
			AILogAlways("COPStick::Execute (%p) resetting due stick/sight target removed", this);
		Reset(NULL);
	}

	// Do not mind the target direction when approaching.
	pPipeUser->m_nMovementPurpose = 1;
	
	CPuppet *pPuppet = pPipeUser->CastToCPuppet();

	const Vec3 opPos = pPipeUser->GetPhysicsPos();

	if(!m_refStickTarget.IsValid())// first time = lets stick to target
	{
		m_refStickTarget = GetWeakRef( (CAIObject*) pPipeUser->GetAttentionTarget());

		if( !m_refStickTarget.IsValid() || m_bUseLastOpResult )
		{

			if (pPipeUser->m_refLastOpResult.IsValid())
				m_refStickTarget = pPipeUser->m_refLastOpResult;
			else
			{
        if (gAIEnv.CVars.DebugPathFinding)
          AILogAlways("COPStick::Execute resetting due to no stick target %s", 
          pPipeUser ? GetNameSafe(pPipeUser) : "");
				// no target, nothing to approach to
				Reset(pPipeUser);
				return eGOR_FAILED;
			}
		}

		// keep last op. result as sight target
		if( !m_refSightTarget.IsValid() && m_bLookAtLastOp && pPipeUser->m_refLastOpResult.IsValid() )
			m_refSightTarget =  pPipeUser->m_refLastOpResult;

		CAIObject * const pStickTarget = m_refStickTarget.GetAIObject();
		const CAIObject * const pSightTarget = m_refSightTarget.GetAIObject();

		if(pStickTarget->GetSubType() == CAIObject::STP_ANIM_TARGET && m_fStickDistance > 0.0f)
		{
			AILogAlways("COPStick::Execute resetting stick distance from %.1f to zero because the stick target is anim target. %s",
				m_fStickDistance, pPipeUser ? GetNameSafe(pPipeUser) : "_no_operand_");
			m_fStickDistance = 0.0f;
		}

		// Create pathfinder operation
		{
			FRAME_PROFILER("OPStick::Execute(): Create Pathfinder/Tracer!", GetISystem(), PROFILE_AI);

			Vec3 stickPos = pStickTarget->GetPhysicsPos();
    if (gAIEnv.CVars.DebugPathFinding)
				AILogAlways("COPStick::Execute (%p) Creating pathfind/trace directives to (%5.2f, %5.2f, %5.2f) %s", this,
				stickPos.x, stickPos.y, stickPos.z, 
      pPipeUser ? GetNameSafe(pPipeUser) : "");
			float endTol = m_bForceReturnPartialPath || m_fEndAccuracy < 0.0f ? std::numeric_limits<float>::max() : m_fEndAccuracy;
			// override end distance if a duration has been set
		float endDistance = GetEndDistance(pPipeUser);
			m_pPathfindDirective = new COPPathFind("", pStickTarget, endTol, endDistance);
    bool exactFollow = !pSightTarget && !pPipeUser->m_bLooseAttention;
			m_pTraceDirective = new COPTrace(exactFollow, m_fEndAccuracy, m_bForceReturnPartialPath, m_stopOnAnimationStart, m_EndMode ); 
			if(m_pTraceDirective)
				m_pTraceDirective->SetSteerAroundPathTarget(m_bSteerAroundPathTarget);
		RegeneratePath(pPipeUser, stickPos);

			// TODO: This is workaround to prevent the Alien Scouts not to use the speed control.
			// Use better test to use the speed control _only_ when it is really needed (human formations).
		if (pPuppet && !pPipeUser->m_movementAbility.b3DMove && !m_bInitialized)
			{
				pPuppet->ResetSpeedControl();
				m_bInitialized = true;
			}

		if(pStickTarget == pPipeUser)
			{
			AILogAlways("COPStick::Execute sticking to self %s ", pPipeUser ? GetNameSafe(pPipeUser) : "_no_operand_");
			Reset(pPipeUser);
			return eGOR_SUCCEEDED;
			}
		}
	}

	CAIObject * const pStickTarget = m_refStickTarget.GetAIObject();
	const CAIObject * const pSightTarget = m_refSightTarget.GetAIObject();

	// Special case for formation points, do not stick to disabled points.
	if (pStickTarget && pStickTarget->GetSubType() == IAIObject::STP_FORMATION)
	{
		if (!pStickTarget->IsEnabled())
		{
			// Wait until the formation becomes active again.
			pPipeUser->m_State.vMoveDir.Set(0,0,0);
			return eGOR_IN_PROGRESS;
		}
	}

	if(!m_bContinuous && pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
	{
    if (gAIEnv.CVars.DebugPathFinding)
      AILogAlways("COPStick::Execute (%p) resetting due to non-continuous and no path %s", this, 
      pPipeUser ? GetNameSafe(pPipeUser) : "");
		Reset(pPipeUser);
		return eGOR_FAILED;
	}

	//make sure the guy looks in correct direction
	if (m_bLookAtLastOp && pSightTarget)
	{
		m_looseAttentionId = pPipeUser->SetLooseAttentionTarget(m_refSightTarget);
	}

  // trace gets deleted when we reach the end and it's not continuous
  if (!m_pTraceDirective)
  {
    if (gAIEnv.CVars.DebugPathFinding)
      AILogAlways("COPStick::Execute (%p) returning true due to no trace directive %s", this, 
      pPipeUser ? GetNameSafe(pPipeUser) : "");
		Reset(pPipeUser);
    return eGOR_FAILED;
  }

	bool b3D = pPipeUser->IsUsing3DNavigation();

  // actually trace the path - continue doing this even whilst regenerating (etc) the path
	EGoalOpResult doneTracing = eGOR_IN_PROGRESS;
  if (m_bPathFound)
	{
		FRAME_PROFILER("OPStick::Execute(): Tracer Execute!", GetISystem(), PROFILE_AI);

    // if using AdjustSpeed then force sprint at this point - will get overridden later
		if (!m_bConstantSpeed && pPuppet && (!m_pTraceDirective || m_pTraceDirective->m_Maneuver == COPTrace::eMV_None) &&
			pPuppet->GetType() == AIOBJECT_PUPPET && !b3D)
      pPuppet->m_State.fMovementUrgency = AISPEED_SPRINT;

		doneTracing = m_pTraceDirective->Execute(pPipeUser);

		if (pPuppet && !m_bConstantSpeed && (!m_pTraceDirective || m_pTraceDirective->m_Maneuver == COPTrace::eMV_None))
			pPuppet->AdjustSpeed(pStickTarget,m_fStickDistance);

		// If the path has been traced, finish the operation if the operand is not sticking continuously.
		if (doneTracing != eGOR_IN_PROGRESS && !m_bContinuous && m_EndMode != COPTrace::eEM_MinimumDistance)
    {
      if (gAIEnv.CVars.DebugPathFinding)
        AILogAlways("COPStick::Execute (%p) finishing due to non-continuous and finished tracing %s", this, 
        pPipeUser ? GetNameSafe(pPipeUser) : "");
			Reset(pPipeUser);
			return doneTracing;
    }

		if (doneTracing != eGOR_IN_PROGRESS)
			m_pPathfindDirective->m_bWaitingForResult = false;

		// m_pStickTarget might be set to NULL as result of passing thru a navigation smart objects
		// on the path by executing an AI action which inserts a goal pipe which calls Reset() for
		// all active goals including this one... in this case it would be fine if we just end this
		// Execute() cycle and wait for the next one
		if ( !pStickTarget )
			return eGOR_IN_PROGRESS;
	}

	// We should never get asserted here! If this assert hits, then the m_pStickTarget was changed during trace,
	// which should never happen!
	// (MATT) That might not be true since the ref refactor {2009/03/23}
	AIAssert(pStickTarget);

  Vec3 targetPos = pStickTarget->GetPhysicsPos();

	if(m_EndMode == COPTrace::eEM_MinimumDistance)
	{
		const float HIJACK_DISTANCE = 0.2f;
		const float pathDistLeft = pPipeUser->m_Path.GetPathLength(false);
		const bool bHijack = (doneTracing != eGOR_IN_PROGRESS && !m_bContinuous) || (m_bPathFound && pathDistLeft < HIJACK_DISTANCE);

		if (bHijack)
		{

			if (m_fApproachTime == -1.0f)
			{
				const float MIN_APPROACH_TIME = 0.3f;
				m_fApproachTime = GetAISystem()->GetFrameStartTime().GetSeconds() + MIN_APPROACH_TIME;
				m_fHijackDistance = (pStickTarget->GetPhysicsPos() - pPipeUser->GetPhysicsPos()).GetLength2D();
			}
			else if (m_fApproachTime <= GetAISystem()->GetFrameStartTime().GetSeconds())
			{
				const float oldDistance = (targetPos - pPipeUser->GetLastPosition()).GetLength2D();
				const float newDistance = (targetPos - pPipeUser->GetPos()).GetLength2D();

				// Ideally, this check should be framerate dependent
				const float MOVEMENT_STOPPED_SECOND_EPSILON = 0.05f;
				if(newDistance >= oldDistance - MOVEMENT_STOPPED_SECOND_EPSILON)
				{
					// We're done!
					m_fApproachTime = -1.0f;
					m_fHijackDistance = -1.0f;
					if (gAIEnv.CVars.DebugPathFinding)
						AILogAlways("COPStick::Execute (%p) finishing due to non-continuous and finished tracing %s", this, 
						pPipeUser ? GetNameSafe(pPipeUser) : "");
					Reset(pPipeUser);
					return eGOR_SUCCEEDED;
				}
			}

			// Ignore pathfinding and force movement in the direction of our target.
			// This will get us as close to a target as physically possible.
			float normalSpeed, minSpeed, maxSpeed;
			pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, pPipeUser->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);
			float fRemainingDistance = (pStickTarget->GetPhysicsPos() - pPipeUser->GetPhysicsPos()).GetLength2D();
			CRY_ASSERT(m_fHijackDistance > 0.0f);
			// slow down as we approach the target
			pPipeUser->m_State.fDesiredSpeed = normalSpeed * fRemainingDistance / m_fHijackDistance;
			pPipeUser->m_State.vMoveDir = (pStickTarget->GetPhysicsPos() - pPipeUser->GetPhysicsPos()).GetNormalizedSafe(Vec3Constants<float>::fVec3_Zero);
		}
	}

  if (m_maxTeleportSpeed > 0.0f && pPipeUser->m_movementAbility.teleportEnabled)
  {
    UpdateStickTargetSafePoints(pPipeUser);
    if (TryToTeleport(pPipeUser))
    {
      // teleport has happened
    }
  }

  switch(pPipeUser->GetType())
  {
    // Danny disabled this prediction for squadmates - it makes them tend to overshoot the end of their paths
//  case AIOBJECT_PUPPET:
//    m_targetPredictionTime = pPipeUser->m_pLastNavNode && pPipeUser->m_pLastNavNode->navType == IAISystem::NAV_TRIANGULAR ? 1.0f : 0.0f;
//    break;
  case AIOBJECT_VEHICLE:
    m_targetPredictionTime = 2.0f;
    break;
  default:
    m_targetPredictionTime = 0.0f;
    break;
  }

  if (m_targetPredictionTime > 0.0f)
  {
    CTimeValue curTime = GetAISystem()->GetFrameStartTime();
    if (m_lastTargetPos.IsZero())
    {
      m_lastTargetPos = targetPos;
      m_lastTargetPosTime = curTime;
      m_smoothedTargetVel.zero();
    }
    else
    {
      float dt = (curTime - m_lastTargetPosTime).GetSeconds();
      Vec3 targetVel(ZERO);
      if (dt > 0.0f)
      {
        targetVel = (targetPos - m_lastTargetPos);
        if (targetVel.GetLengthSquared() > 5.0f) // try to catch sudden jumps
          targetVel.zero();
        else
          targetVel /= dt;
        // Danny todo make this time timestep independent (exp dependency on dt)
        float frac = 0.1f;
        m_smoothedTargetVel = frac * targetVel + (1.0f - frac) * m_smoothedTargetVel;
        m_lastTargetPos = targetPos;
        m_lastTargetPosTime = curTime;
      }
    }
  }
  else
  {
    m_smoothedTargetVel.zero();
    m_lastTargetPos = targetPos;
  }
  Vec3 targetOffset = m_smoothedTargetVel * m_targetPredictionTime;

  if (pPipeUser->m_lastNavNodeIndex && gAIEnv.pGraph->GetNode(pPipeUser->m_lastNavNodeIndex)->navType == IAISystem::NAV_TRIANGULAR)
  {
    // ensure offset doesn't cross forbidden
    Vec3 newPt;
    if (gAIEnv.pNavigation->IntersectsForbidden(targetPos, targetPos+targetOffset, newPt))
    {
      float prevDist = targetOffset.GetLength2D();
      float newDist = Distance::Point_Point2D(targetPos, newPt);
      if (newDist > prevDist && newDist > 0.0f)
        newPt = targetPos + (prevDist / newDist) * (newPt - targetPos);
      targetOffset = newPt - targetPos;
    }
  }
  m_pPathfindDirective->SetTargetOffset(targetOffset);
  targetPos += targetOffset;

	Vec3 targetVector;
	if(pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND && m_bForceReturnPartialPath)
		targetVector = (pPipeUser->m_Path.GetLastPathPos() - opPos);
	else
		targetVector = (targetPos - opPos);

	if(!b3D)
		targetVector.z = 0.0f;
	float curDist = targetVector.GetLength();

	if (pPipeUser->m_State.vMoveDir.IsZero(.05f) && pPipeUser->GetAttentionTarget() && pPipeUser->GetAttentionTarget() != pStickTarget)
		pPipeUser->m_bLooseAttention = false;

	// check if need to regenerate path
	float targetMoveDist = b3D ? (m_vLastUsedTargetPos - targetPos).GetLength() : (m_vLastUsedTargetPos - targetPos).GetLength2D();

	// If the target is moving approximately to the same direction, do not update the path so often.
	bool	targetDirty = false;

	// [AlexMcC] If the target hasn't moved, there's no good reason to update our path.
	// Mikko's comment below explains why the path can be updated too frequently otherwise.
	// If we don't check to see if the target has moved, we could update our path every
	// frame and never move.
	if( targetMoveDist > m_fTrhDistance )
	{
		if( !pPipeUser->m_Path.Empty() )
		{
			// Use the stored destination point instead of the last path node since the path may be cut because of navSO.
			Vec3 pathEnd = pPipeUser->m_PathDestinationPos;
			Vec3	dir(pathEnd - opPos);
			if(!b3D)
				dir.z = 0;
			dir.NormalizeSafe();

			Vec3	dirToTarget(targetPos - pathEnd);
			if(!b3D)
				dirToTarget.z = 0;
			dirToTarget.NormalizeSafe();

			float	regenDist = m_fTrhDistance;
			if( dirToTarget.Dot( dir ) < cosf(DEG2RAD(8.0f)) )
				regenDist *= 5.0f;

			if( targetMoveDist > regenDist )
					targetDirty = true;

			// when near the path end force more frequent updates
			float pathDistLeft = pPipeUser->m_Path.GetPathLength(false);
			float pathEndError = b3D ? (pathEnd - targetPos).GetLength() : (pathEnd - targetPos).GetLength2D();

			// TODO/workaround! [Mikko] This prevent the path to regenerated every frame in some special cases in Crysis Core level
			// where quite a few behaviors are sticking to a long distance (7-10m).
			// The whole stick regeneration logic is flawed mostly because pPipeUser->m_PathDestinationPos is not always
			// the actual target position. The pathfinder may adjust the end location and not keep the requested end pos
			// if the target is not reachable. I'm sure there are other really nasty cases about this path invalidation logic too.
			const GraphNode* pLastNode = gAIEnv.pGraph->GetNode(pPipeUser->m_lastNavNodeIndex);
			if (pLastNode && pLastNode->navType == IAISystem::NAV_VOLUME)
				pathEndError = max(0.0f, pathEndError - GetEndDistance(pPipeUser));

			if (pathEndError > 0.1f && pathDistLeft < 2.0f * pathEndError)
			{
				targetDirty = true;
			}

		}
		else
		{
			targetDirty = true;
		}
	}

	{
		FRAME_PROFILER("OPStick::Execute(): Final Path Processing!", GetISystem(), PROFILE_AI)

		// If the target pos moves substantially update
		// If we're not already close to the target but it's moved a bit then update.
		// Be careful about forcing an update too often - especially if we're nearly there.
		if (targetDirty && m_pTraceDirective->m_Maneuver == COPTrace::eMV_None && !m_pTraceDirective->m_passingStraightNavSO &&
		(pPipeUser->m_State.curActorTargetPhase == eATP_None || pPipeUser->m_State.curActorTargetPhase == eATP_Error) &&
		!pPipeUser->m_Path.GetParams().precalculatedPath && !pPipeUser->m_Path.GetParams().inhibitPathRegeneration)
		{
		if (pPipeUser->m_nPathDecision != PATHFINDER_STILLFINDING)
      RegeneratePath(pPipeUser, targetPos);
		}

		// check pathfinder status
	switch(pPipeUser->m_nPathDecision)
		{
		case PATHFINDER_STILLFINDING:
			{
				IVisArea *pGoalArea; 
				int leaderBuilding = -1;
				int nBuilding;
				CAIActor* pStickActor = pStickTarget->CastToCAIActor();
				bool targetIndoor = gAIEnv.pNavigation->CheckNavigationType(pStickTarget->GetPhysicsPos(),nBuilding,pGoalArea,
					pStickActor ? pStickActor->m_movementAbility.pathfindingProperties.navCapMask & IAISystem::NAV_WAYPOINT_HUMAN : IAISystem::NAV_WAYPOINT_HUMAN) 
					== IAISystem::NAV_WAYPOINT_HUMAN;
				m_pPathfindDirective->SetForceTargetBuildingId(targetIndoor ? leaderBuilding : -1);
				m_pPathfindDirective->Execute(pPipeUser);		
				m_pPathfindDirective->SetForceTargetBuildingId(-1);
        return eGOR_IN_PROGRESS;
			}

		case PATHFINDER_NOPATH:
			pPipeUser->m_State.vMoveDir.Set(0,0,0);
			if (!m_bContinuous)
			{
      if (gAIEnv.CVars.DebugPathFinding)
					AILogAlways("COPStick::Execute (%p) resetting due to no path %s", this, 
        pPipeUser ? GetNameSafe(pPipeUser) : "");
			Reset(pPipeUser);
				return eGOR_FAILED;
			}
			else 
			{
				return eGOR_IN_PROGRESS;
			}

		case PATHFINDER_PATHFOUND:
			// do not spam the AI with the OnPathFound signal (stick goal regenerates the path frequently)
			if(!m_bPathFound)
			{
				m_bPathFound = true;
				TPathPoints::const_reference lastPathNode = pPipeUser->m_OrigPath.GetPath().back();
				Vec3 lastPos = lastPathNode.vPos;
				Vec3 requestedLastNodePos = pPipeUser->m_Path.GetParams().end;
				// send signal to AI
				float dist = b3D ? Distance::Point_Point(lastPos,requestedLastNodePos) : Distance::Point_Point2D(lastPos,requestedLastNodePos);
				if (lastPathNode.navType != IAISystem::NAV_SMARTOBJECT && dist > m_fStickDistance+C_MaxDistanceForPathOffset)
				{
					AISignalExtraData* pData = new AISignalExtraData;
					pData->fValue = dist - m_fStickDistance;
					pPipeUser->SetSignal(0,"OnEndPathOffset",pPipeUser->GetEntity(),pData ,gAIEnv.SignalCRCs.m_nOnEndPathOffset);
				}
				else 
				pPipeUser->SetSignal(0,"OnPathFound",NULL, 0, gAIEnv.SignalCRCs.m_nOnPathFound);

        return Execute(pPipeUser);
			}
		}
	}

	return eGOR_IN_PROGRESS;
}

//===================================================================
// ExecuteDry
// Note - it is very important we don't call Reset on ourself from here
// else we might restart ourself subsequently
//===================================================================
void COPStick::ExecuteDry(CPipeUser* pPipeUser) 
{
	CAIObject * const pStickTarget = m_refStickTarget.GetAIObject();

  if (m_pTraceDirective && pStickTarget)
  {
		Vec3 targetVector;
		if(pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND && m_bForceReturnPartialPath)
			targetVector = (pPipeUser->m_Path.GetLastPathPos() - pPipeUser->GetPhysicsPos());
		else
			targetVector = (pStickTarget->GetPhysicsPos() - pPipeUser->GetPhysicsPos());

		if(!pPipeUser->m_movementAbility.b3DMove)
		  targetVector.z = 0.0f;
	  float curDist = targetVector.GetLength();

    CPuppet *pPuppet = pPipeUser->CastToCPuppet();
    
		if (!m_bConstantSpeed && pPuppet && (!m_pTraceDirective || m_pTraceDirective->m_Maneuver == COPTrace::eMV_None) &&
      pPuppet->GetType() == AIOBJECT_PUPPET && !pPuppet->IsUsing3DNavigation())
      pPuppet->m_State.fMovementUrgency = AISPEED_SPRINT;

    if(m_bPathFound)
      m_pTraceDirective->ExecuteTrace(pPipeUser, false);

		if (pPuppet && !m_bConstantSpeed && (!m_pTraceDirective || m_pTraceDirective->m_Maneuver == COPTrace::eMV_None))
			pPuppet->AdjustSpeed(pStickTarget,m_fStickDistance);
	}
}


//
//----------------------------------------------------------------------------------------------------------
COPContinuous::COPContinuous(bool bKeeoMoving):
m_bKeepMoving(bKeeoMoving)
{

}

//
//----------------------------------------------------------------------------------------------------------
COPContinuous::COPContinuous(const XmlNodeRef& goalOpNode) :
	m_bKeepMoving(s_xml.GetBool(goalOpNode, "keepMoving"))
{

}

//
//----------------------------------------------------------------------------------------------------------
COPContinuous::~COPContinuous()
{

}

//
//----------------------------------------------------------------------------------------------------------
void COPContinuous::Reset(CPipeUser* pPipeUser)
{
}

//
//----------------------------------------------------------------------------------------------------------
EGoalOpResult COPContinuous::Execute(CPipeUser* pPipeUser)
{
	pPipeUser->m_bKeepMoving = m_bKeepMoving;
	return eGOR_DONE;
}


//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
COPSteer::COPSteer( float fSteerDistance, float fPathLenLimit )
: m_vLastUsedTargetPos(0,0,0),
m_fMinEndDistance(0.f),
m_fMaxEndDistance(0.f),
m_pTraceDirective(NULL),
m_pPathfindDirective(NULL),
m_bFirstExec(true),
m_fPathLenLimit(fPathLenLimit),
m_fEndAccuracy(0.f)
{
	m_fSteerDistanceSqr = fSteerDistance * fSteerDistance;
}

//
//----------------------------------------------------------------------------------------------------------
COPSteer::COPSteer(const XmlNodeRef& node) :
	m_vLastUsedTargetPos(ZERO),
	m_fMinEndDistance(0.f),
	m_fMaxEndDistance(0.f),
	m_pTraceDirective(0),
	m_pPathfindDirective(0),
	m_bFirstExec(true),
	m_fPathLenLimit(0.f),
	m_fEndAccuracy(0.f)
{
	float fSteerDistance;
	if (!s_xml.GetMandatory(node, "distance", fSteerDistance))
	{
		fSteerDistance = 0.f;
	}
	
	m_fSteerDistanceSqr = fSteerDistance * fSteerDistance;
	
	s_xml.GetMandatory(node, "pathLengthLimit", m_fPathLenLimit);
}

//
//----------------------------------------------------------------------------------------------------------
COPSteer::~COPSteer()
{
	SAFE_DELETE( m_pPathfindDirective );
	SAFE_DELETE( m_pTraceDirective );

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPSteer::~COPSteer %p", this);
}

//
//----------------------------------------------------------------------------------------------------------
void COPSteer::DebugDraw(CPipeUser* pPipeUser) const
{

}

//
//----------------------------------------------------------------------------------------------------------
void COPSteer::Reset(CPipeUser* pPipeUser)
{
	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPSteer::Reset %s", pPipeUser ? GetNameSafe(pPipeUser) : "");

	SAFE_DELETE( m_pPathfindDirective );
	SAFE_DELETE( m_pTraceDirective );

	m_bNeedHidespot = true;
	m_vLastUsedTargetPos.zero();

	if (pPipeUser)
	{
		pPipeUser->ClearPath("COPSteer::Reset m_Path");
	}
}

//
//----------------------------------------------------------------------------------------------------------
EGoalOpResult COPSteer::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_AI );

	if( pPipeUser->m_nPathDecision == PATHFINDER_STILLFINDING && m_pPathfindDirective != NULL )
	{
		m_pPathfindDirective->Execute(pPipeUser);
		return eGOR_IN_PROGRESS;
	}

	CAISystem *pAISystem = GetAISystem();
	CPuppet *pPuppet = pPipeUser->CastToCPuppet();

	if( m_bFirstExec == true )
	{
		CGroupMember* pGroupMember = gAIEnv.pGroupSystem->GetGroupMember(pPuppet->GetEntityID());
		if( pGroupMember == NULL )
		{
			pGroupMember = gAIEnv.pGroupSystem->CreateGroupMember(pPuppet->GetEntityID());
		}

		pGroupMember->UseSteering(true, m_fSteerDistanceSqr);

		m_pPosDummy = pGroupMember->GetFormationPosAIObject();
		m_bFirstExec = false;
	}
	
	if ( m_pPosDummy == NULL ) 
	{
		pPipeUser->SetSignal(0,"OnSteerFailed",pPipeUser->GetEntity(),NULL, 0); 
		AILogAlways("COPSteer::Execute - Error - No point to steer to: %s", GetNameSafe(pPipeUser));

		return eGOR_FAILED;
	}

	if( pPuppet->GetPos().GetSquaredDistance2D( m_pPosDummy->GetPos() ) < 0.3f )
	{
		return eGOR_SUCCEEDED;
	}
	else
	{
		CAIObject *pTarget =  m_pPosDummy;
		
		bool bForceReturnPartialPath = false;
		
		//if (pPipeUser->m_nPathDecision != PATHFINDER_PATHFOUND) m_fLastTime = -1.0f;

		// Check for unsuitable agents
		if(pPipeUser->GetSubType() == CAIObject::STP_HELI 
			|| !pPipeUser->m_movementAbility.bUsePathfinder)
		{
			AILogAlways("COPSteer::Execute - Error - Unsuitable agent: %s", GetNameSafe(pPipeUser));
			Reset(pPipeUser); 
			return eGOR_FAILED;
		}

		if(pTarget->GetSubType() == CAIObject::STP_ANIM_TARGET)
		{
			AILogAlways("COPSteer::Execute - Error - Unsuitable target: %s", GetNameSafe(pPipeUser));
			Reset(pPipeUser); 
			return eGOR_FAILED;
		}

		// Move strictly to the target point.
		// MTJ: Not sure this should stay
		pPipeUser->m_nMovementPurpose = 0;

		// Where are we heading eventually?
		Vec3 vAgentPosition(pPipeUser->GetPos());
		float fDistToEndTarget = vAgentPosition.GetDistance(pTarget->GetPos());

		pPipeUser->SetInCover(false);
		pPipeUser->m_CurrentHideObject.Invalidate();
		
		// If we have a hidespot, that's our target
		// Otherwise it's the attention target
		if (m_pPosDummy) 
			pTarget = m_pPosDummy;

		switch( m_pPathfindDirective ? pPipeUser->m_nPathDecision : PATHFINDER_MAXVALUE)
		{
		case PATHFINDER_MAXVALUE:
			{
				// Generate path to target
				float endTol = (bForceReturnPartialPath || m_fEndAccuracy < 0.0f ? std::numeric_limits<float>::max() : m_fEndAccuracy);
				m_pPathfindDirective = new COPPathFind("", pTarget, 0.0f, 0.0f, true, m_fPathLenLimit);
				pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;
				if (m_pPathfindDirective->Execute(pPipeUser) && pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
				{
					// If special nopath signal is specified, send the signal.
					pPipeUser->m_State.vMoveDir.Set(0,0,0);
					return eGOR_FAILED;			
				}
				break;
			}
		case PATHFINDER_PATHFOUND:
			{
				// if we have a path, trace it
				if (pPipeUser->m_OrigPath.Empty() || pPipeUser->m_OrigPath.GetPathLength( true ) < 0.01f )
				{
					AILogAlways("COPSteer::Execute - Origpath is empty: %s. Regenerating.", GetNameSafe(pPipeUser));
					/*RegeneratePath( pPipeUser, m_pPosDummy->GetPos() );
					pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;*/
					return eGOR_FAILED;
				}
				else
				{
					float pathsize = pPipeUser->m_OrigPath.GetPathLength(true);

					// If we need a tracer, create one
					if (!m_pTraceDirective)
					{
						// We create another goalop to achieve this (sucky approach)
						bool bExact = true;
						bForceReturnPartialPath = false;

						m_pTraceDirective = new COPTrace(bExact, m_fEndAccuracy, bForceReturnPartialPath);
						TPathPoints::const_reference lastPathNode = pPipeUser->m_OrigPath.GetPath().back();
						Vec3 vLastPos = lastPathNode.vPos;
						Vec3 vRequestedLastNodePos = pPipeUser->m_Path.GetParams().end;
						float dist = Distance::Point_Point(vLastPos,vRequestedLastNodePos);

						// MTJ: What is this for?
						if (lastPathNode.navType != IAISystem::NAV_SMARTOBJECT && dist > C_MaxDistanceForPathOffset)// && pPipeUser->m_Path.GetPath().size() == 1 )
						{
							AISignalExtraData* pData = new AISignalExtraData;
							pData->fValue = dist;// - m_fMinEndDistance;
							pPipeUser->SetSignal(0,"OnEndPathOffset",pPipeUser->GetEntity(),pData, gAIEnv.SignalCRCs.m_nOnEndPathOffset);
						}
						else
						{
							pPipeUser->SetSignal(0,"OnPathFound",NULL, 0, gAIEnv.SignalCRCs.m_nOnPathFound);
						}
					}

					// Keep tracing - previous code will stop us when close enough
					// MTJ: What previous code?
					bool bDone = (m_pTraceDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS);

					float fDistToHopTarget = vAgentPosition.GetDistance(pTarget->GetPos());

					if (bDone) 
					{
						pPipeUser->SetSignal(0,"OnFormationPointReached",pPipeUser->GetEntity(),NULL, 0); // MTJ: Provide CRC
						return eGOR_SUCCEEDED;
					}
				}

				break;
			}
		case PATHFINDER_NOPATH:
			{
				pPipeUser->SetSignal(0,"OnGetToFormationPointFailed",pPipeUser->GetEntity(),NULL, 0); // MTJ: Provide CRC
				return eGOR_FAILED;
			}
		default:
			{
				// MTJ: Still pathfinding I guess?
				m_pPathfindDirective->Execute(pPipeUser);
				break;
			}
		}
	}
	// Run me again next tick
	return eGOR_IN_PROGRESS;
}

void COPSteer::ExecuteDry(CPipeUser* pPipeUser) 
{
	if( pPipeUser->m_nPathDecision == PATHFINDER_STILLFINDING && m_pPathfindDirective != NULL )
	{
		m_pPathfindDirective->Execute(pPipeUser);
		return;
	}

	if (m_pTraceDirective && m_pPosDummy)
	{
		bool bTargetDirty = false;

		if( pPipeUser->GetPos().GetSquaredDistance2D( m_pPosDummy->GetPos() ) < m_fSteerDistanceSqr )
		{

		}
		else
		{
			Vec3 targetVector;
			if(pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND /*&& m_bForceReturnPartialPath*/)
				targetVector = (pPipeUser->m_Path.GetLastPathPos() - pPipeUser->GetPhysicsPos());
			else
				targetVector = (m_pPosDummy->GetPhysicsPos() - pPipeUser->GetPhysicsPos());

			{
					Vec3 targetPos = m_pPosDummy->GetPos();
					float targetMoveDist = (m_vLastUsedTargetPos - targetPos).GetLength2D();

			// Use the stored destination point instead of the last path node since the path may be cut because of navSO.
					Vec3 pathEnd = pPipeUser->m_PathDestinationPos;
					Vec3	dir(pathEnd - pPipeUser->GetPos());
					
					dir.z = 0;
					dir.NormalizeSafe();
			
					Vec3	dirToTarget(targetPos - pathEnd);
					
					dirToTarget.z = 0;
					dirToTarget.NormalizeSafe();
			
					float	regenDist = 0.3f;
					if( dirToTarget.Dot( dir ) < cosf(DEG2RAD(8.0f)) )
						regenDist *= 5.0f;
			
					if( targetMoveDist > regenDist )
						bTargetDirty = true;
			
					// when near the path end force more frequent updates
					//float pathDistLeft = pPipeUser->m_Path.GetPathLength(false);
					//float pathEndError = (pathEnd - targetPos).GetLength2D();
			
					//// TODO/workaround! [Mikko] This prevent the path to regenerated every frame in some special cases in Crysis Core level
					//// where quite a few behaviors are sticking to a long distance (7-10m).
					//// The whole stick regeneration logic is flawed mostly because pPipeUser->m_PathDestinationPos is not always
					//// the actual target position. The pathfinder may adjust the end location and not keep the requested end pos
					//// if the target is not reachable. I'm sure there are other really nasty cases about this path invalidation logic too.
					//const GraphNode* pLastNode = gAIEnv.pGraph->GetNode(pPipeUser->m_lastNavNodeIndex);
					//if (pLastNode && pLastNode->navType == IAISystem::NAV_VOLUME)
					//	pathEndError = max(0.0f, pathEndError - pathDistLeft/*GetEndDistance(pPipeUser)*/);
			
					//if (pathEndError > 0.1f && pathDistLeft < 2.0f * pathEndError)
					//{
					//	//bTargetDirty = true;
					//}
			}

			if(!pPipeUser->m_movementAbility.b3DMove)
				targetVector.z = 0.0f;
			float curDist = targetVector.GetLength();

			CPuppet *pPuppet = pPipeUser->CastToCPuppet();

			if( /*pPipeUser->m_nPathDecision != PATHFINDER_STILLFINDING && */bTargetDirty )
				RegeneratePath(pPipeUser, m_pPosDummy->GetPos());
			else
				m_pTraceDirective->ExecuteTrace(pPipeUser, false);

		}
	}
}

//
//----------------------------------------------------------------------------------------------------------
void COPSteer::RegeneratePath(CPipeUser* pPipeUser, const Vec3 &destination)
{
	if (!pPipeUser)
		return;

	CTimeValue time = GetAISystem()->GetFrameStartTime();
	
	if( (time - m_fLastRegenTime).GetMilliSeconds() < 100.f )
		return;
	
	 m_fLastRegenTime = time;

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPSteer::RegeneratePath %s", pPipeUser ? GetNameSafe(pPipeUser) : "");
	m_pPathfindDirective->Reset(pPipeUser);
	if( m_pTraceDirective )
		m_pTraceDirective->m_fEndAccuracy = m_fEndAccuracy;
	m_vLastUsedTargetPos = destination;
	pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

	const Vec3 opPos = pPipeUser->GetPhysicsPos();

	// Check for direct connection if distance is less than fixed value
	if ( opPos.GetSquaredDistance( destination ) < 2500.f )
	{		
		int nbid;
		IVisArea *iva;
		IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(opPos, nbid, iva, pPipeUser->m_movementAbility.pathfindingProperties.navCapMask);
		CNavRegion *pRegion = gAIEnv.pNavigation->GetNavRegion(navType, gAIEnv.pGraph);
		if (pRegion)
		{
			Vec3	from = opPos;
			Vec3	to = destination;

			NavigationBlockers	navBlocker;
			if(pRegion->CheckPassability(from, to, pPipeUser->GetParameters().m_fPassRadius, navBlocker, pPipeUser->GetMovementAbility().pathfindingProperties.navCapMask))
			{
				pPipeUser->ClearPath("COPSteer::RegeneratePath m_Path");

				if (navType == IAISystem::NAV_TRIANGULAR)
				{
					// Make sure not to enter forbidden area.
					if (gAIEnv.pNavigation->IsPathForbidden(opPos, destination))
						return;
					if (gAIEnv.pNavigation->IsPointForbidden(destination, pPipeUser->GetParameters().m_fPassRadius))
						return;
				}

				PathPointDescriptor	pt;
				pt.navType = navType;

				pt.vPos = from;
				pPipeUser->m_Path.PushBack(pt);

				float endDistance = 0.5f;//m_fStickDistance;
				if (fabsf(endDistance) > 0.0001f)
				{
					// Handle end distance.
					float dist;
					if (pPipeUser->IsUsing3DNavigation())
						dist = Distance::Point_Point(from, to);
					else
						dist = Distance::Point_Point2D(from, to);

					float d;
					if (endDistance > 0.0f)
						d = dist - endDistance;
					else
						d = -endDistance;
					float u = clamp(d / dist, 0.0001f, 1.0f);

					pt.vPos = from + u * (to - from);

					pPipeUser->m_Path.PushBack(pt);
				}
				else
				{
					pt.vPos = to;
					pPipeUser->m_Path.PushBack(pt);
				}

				pPipeUser->m_Path.SetParams(SNavPathParams(from, to, Vec3(ZERO), Vec3(ZERO), -1, false, endDistance, true));

				pPipeUser->m_OrigPath = pPipeUser->m_Path;
				pPipeUser->m_nPathDecision = PATHFINDER_PATHFOUND;
			}
		}
	}
}

//
//----------------------------------------------------------------------------------------------------------
void COPSteer::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPSteer");
	{
		ser.Value("m_vLastUsedTargetPos",m_vLastUsedTargetPos);
		ser.Value("m_fLastDistance",m_fLastDistance);
		ser.Value("m_fMinEndDistance",m_fMinEndDistance);
		ser.Value("m_fMaxEndDistance",m_fMaxEndDistance);
		ser.Value("m_bNeedHidespot",m_bNeedHidespot);
		ser.Value("m_fEndAccuracy",m_fEndAccuracy);


		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		else
		{
			SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		ser.EndGroup();
	}
}


//
//----------------------------------------------------------------------------------------------------------
COPWaitSignal::COPWaitSignal(const XmlNodeRef& node) :
	m_sSignal(s_xml.GetMandatoryString(node, "name")),
	m_edMode(edNone),
	m_fInterval(0.f)
{
	node->getAttr("interval", m_fInterval);
	Reset( NULL );
}

//
//----------------------------------------------------------------------------------------------------------
COPWaitSignal::COPWaitSignal( const char* sSignal, float fInterval /*= 0*/ )
{
	m_edMode = edNone;
	m_sSignal = sSignal;
	m_fInterval = fInterval;
	Reset( NULL );
}

COPWaitSignal::COPWaitSignal( const char* sSignal, const char* sObjectName, float fInterval /*= 0*/ )
{
	m_edMode = edString;
	m_sObjectName = sObjectName;

	m_sSignal = sSignal;
	m_fInterval = fInterval;
	Reset( NULL );
}

COPWaitSignal::COPWaitSignal( const char* sSignal, int iValue, float fInterval /*= 0*/ )
{
	m_edMode = edInt;
	m_iValue = iValue;

	m_sSignal = sSignal;
	m_fInterval = fInterval;
	Reset( NULL );
}

COPWaitSignal::COPWaitSignal( const char* sSignal, EntityId nID, float fInterval /*= 0*/ )
{
	m_edMode = edId;
	m_nID = nID;

	m_sSignal = sSignal;
	m_fInterval = fInterval;
	Reset( NULL );
}

bool COPWaitSignal::NotifySignalReceived( CAIObject* pPipeUser, const char* szText, IAISignalExtraData* pData )
{
	CCCPOINT(COPWaitSignal_NotifySignalReceived);

	if ( !m_bSignalReceived && m_sSignal == szText )
	{
		if ( m_edMode == edNone )
			m_bSignalReceived = true;
		else if ( pData )
		{
			CCCPOINT(OPWaitSignal_NotifySignalReceived_Data);
			// (MATT) Note that it appears all the data forms could be removed {2008/08/09}
			if ( m_edMode == edString && m_sObjectName == pData->GetObjectName() )
				m_bSignalReceived = true;
			else if ( m_edMode == edInt && m_iValue == pData->iValue )
				m_bSignalReceived = true;
			else if ( m_edMode == edId && m_nID == pData->nID.n )
				m_bSignalReceived = true;
		}
	}
	return m_bSignalReceived;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPWaitSignal::Reset( CPipeUser* pPipeUser )
{
	if ( pPipeUser )
	{
		CAIActor::ListWaitGoalOps& rGoalOpVec = pPipeUser->m_listWaitGoalOps;
		const CAIActor::ListWaitGoalOps::iterator cGoalOpEnd = rGoalOpVec.end();
		CAIActor::ListWaitGoalOps::iterator it = std::find( rGoalOpVec.begin(), cGoalOpEnd, this );
		if(it != cGoalOpEnd)
			rGoalOpVec.erase( it );
	}
	else
	{
		// only reset this on initialize!
		// later it may happen that this is called from signal handler (by inserting a goal pipe)
		m_bSignalReceived = false;
	}
	m_startTime.SetSeconds(0.0f);
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPWaitSignal::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if ( m_bSignalReceived )
	{
		m_bSignalReceived = false;
		return eGOR_DONE;
	}

	if ( m_fInterval <= 0.0f )
	{
		if ( !m_startTime.GetValue() )
		{
			pPipeUser->m_listWaitGoalOps.insert(pPipeUser->m_listWaitGoalOps.begin(), this );//push_front
			m_startTime.SetMilliSeconds(1);
		}
		return eGOR_IN_PROGRESS;
	}

	CTimeValue time = GetAISystem()->GetFrameStartTime();
	if ( !m_startTime.GetValue() )
	{
		m_startTime = time;
		pPipeUser->m_listWaitGoalOps.insert(pPipeUser->m_listWaitGoalOps.begin(), this );//push_front
	}

	CTimeValue timeElapsed = time - m_startTime;
	if ( timeElapsed.GetSeconds() > m_fInterval )
		return eGOR_DONE;

	return eGOR_IN_PROGRESS;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPWaitSignal::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	// please ask me when you want to change [tetsuji]
	ser.BeginGroup("COPWaitSignal");
	{
		ser.Value("m_bSignalReceived",m_bSignalReceived);
		if (ser.IsReading())
		{
			m_startTime.SetSeconds(0.0f);
		}
		ser.EndGroup();
	}
	// please ask me when you want to change [dejan]
}



//----------------------------------------------------------------------------------------------------------
// COPAnimAction
//----------------------------------------------------------------------------------------------------------
COPAnimation::COPAnimation( EAnimationMode mode, const char* value )
	: m_eMode( mode )
	, m_sValue( value )
	, m_bAGInputSet( false )
{
}

//
//----------------------------------------------------------------------------------------------------------
COPAnimation::COPAnimation(const XmlNodeRef& node)
: m_sValue(s_xml.GetMandatoryString(node, "action")),
  m_bAGInputSet(false),
  m_eMode(AIANIM_INVALID)
{
	s_xml.GetAnimationMode(node, "mode", m_eMode, CGoalOpXMLReader::MANDATORY);
}

//
//-------------------------------------------------------------------------------------------------------------
void COPAnimation::Reset(CPipeUser* pPipeUser)
{
	m_bAGInputSet = false;
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPAnimation::Execute( CPipeUser* pPipeUser )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	AIAssert(pPipeUser);
	if ( !pPipeUser->GetProxy() )
		return eGOR_FAILED;

	if ( m_bAGInputSet )
	{
		switch( m_eMode )
		{
		case AIANIM_SIGNAL:
			return pPipeUser->GetProxy()->IsSignalAnimationPlayed(m_sValue) ? eGOR_SUCCEEDED : eGOR_IN_PROGRESS;
		case AIANIM_ACTION:
			return pPipeUser->GetProxy()->IsActionAnimationStarted(m_sValue) ? eGOR_SUCCEEDED : eGOR_IN_PROGRESS;
		}
		AIAssert( !"Setting an invalid AG input has returned true!" );
		return eGOR_FAILED;
	}

	// set the value here
	if ( !pPipeUser->GetProxy()->SetAGInput(m_eMode == AIANIM_ACTION ? AIAG_ACTION : AIAG_SIGNAL, m_sValue) )
	{
		AIWarning( "Can't set value '%s' on AG input '%s' of PipeUser '%s'", m_sValue.c_str(), m_eMode == AIANIM_ACTION ? "Action" : "Signal", GetNameSafe(pPipeUser) );
		return eGOR_FAILED;
	}
	m_bAGInputSet = true;

	return eGOR_IN_PROGRESS;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPAnimation::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPAnimation");
	{
		ser.EnumValue("m_eMode",m_eMode,AIANIM_SIGNAL,AIANIM_ACTION);
		ser.Value("m_sValue",m_sValue);
		ser.Value("m_bAGInputSet",m_bAGInputSet);
		ser.EndGroup();
	}
}

//
//-------------------------------------------------------------------------------------------------------------
void COPAnimTarget::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPAnimTarget");
	{
		ser.Value("m_bSignal",m_bSignal);
		ser.Value("m_sAnimName",m_sAnimName);
		ser.Value("m_fStartWidth",m_fStartWidth);
		ser.Value("m_fDirectionTolerance",m_fDirectionTolerance);
		ser.Value("m_fStartArcAngle",m_fStartArcAngle);
		ser.Value("m_vApproachPos",m_vApproachPos);
		ser.Value("m_bAutoAlignment",m_bAutoAlignment);
		ser.EndGroup();
	}
}


//----------------------------------------------------------------------------------------------------------
// COPAnimTarget
//----------------------------------------------------------------------------------------------------------
COPAnimTarget::COPAnimTarget( bool signal, const char* animName, float startWidth, float dirTolerance, float startArcAngle, const Vec3& approachPos, bool autoAlignment )
	: m_bSignal( signal )
	, m_sAnimName( animName )
	, m_fStartWidth( startWidth )
	, m_fDirectionTolerance( dirTolerance )
	, m_fStartArcAngle( startArcAngle )
	, m_vApproachPos( approachPos )
	, m_bAutoAlignment( autoAlignment )
{
}

//
//----------------------------------------------------------------------------------------------------------
COPAnimTarget::COPAnimTarget(const XmlNodeRef& node)
: m_sAnimName(s_xml.GetMandatoryString(node, "name")),
	m_fStartWidth(0.f),
	m_fDirectionTolerance(0.f),
	m_fStartArcAngle(0.f),
	m_vApproachPos(ZERO),
	m_bAutoAlignment(s_xml.GetBool(node, "autoAlignment"))
{
	EAnimationMode eAnimationMode;
	if (!s_xml.GetAnimationMode(node, "mode", eAnimationMode, CGoalOpXMLReader::MANDATORY))
	{
		eAnimationMode = AIANIM_SIGNAL;
	}
	m_bSignal = (eAnimationMode == AIANIM_SIGNAL);

	s_xml.GetMandatory(node, "startWidth", m_fStartWidth);
	s_xml.GetMandatory(node, "directionTolerance", m_fDirectionTolerance);
	s_xml.GetMandatory(node, "startArcAngle", m_fStartArcAngle);
	
	float fApproachRadius;
	if (node->getAttr("approachRadius", fApproachRadius))
	{
		m_vApproachPos = Vec3(fApproachRadius);
	}
}


EGoalOpResult COPAnimTarget::Execute( CPipeUser* pPipeUser )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	IAIObject* pTarget = pPipeUser->GetLastOpResult();
	if ( pTarget )
	{
		SAIActorTargetRequest	req;
		if ( !m_vApproachPos.IsZero() )
		{
			req.approachLocation = m_vApproachPos;
			req.approachDirection = pTarget->GetPos() - m_vApproachPos;
			req.approachDirection.NormalizeSafe(Vec3(0,1,0));
		}
		else
		{
			req.approachLocation = pTarget->GetPos();
			req.approachDirection = pTarget->GetMoveDir();
		}
		req.animLocation = pTarget->GetPos();
		req.animDirection = pTarget->GetMoveDir();
		if ( pPipeUser->IsUsing3DNavigation() == false )
		{
			req.animDirection.z = 0.0f;
			req.animDirection.NormalizeSafe();
		}
		req.directionTolerance = DEG2RAD(m_fDirectionTolerance);
		req.startArcAngle = m_fStartArcAngle;
		req.startWidth = m_fStartWidth;
		req.signalAnimation = m_bSignal;
		req.useAssetAlignment = m_bAutoAlignment;
		req.animation = m_sAnimName;

		pPipeUser->SetActorTargetRequest(req);
	}
	return eGOR_DONE;
}


//
//-------------------------------------------------------------------------------------------------------------
COPUseCover::COPUseCover(bool unHideNow, bool useLastOpAsBackup, float intervalMin, float intervalMax, bool speedUpTimeOutWhenNoTarget):
	m_fTimeOut(0),
	m_bUnHide(unHideNow),
	m_useLastOpAsBackup(useLastOpAsBackup), 
	m_speedUpTimeOutWhenNoTarget(speedUpTimeOutWhenNoTarget),
	m_curCoverUsage(USECOVER_NONE),
	m_weaponOffset(0.3f),
	m_coverCompromised(false),
	m_targetReached(false),
	//	m_leftInvalidTime(0.0f),
	//	m_rightInvalidTime(0.0f),
	m_leftInvalidDist(1000.0f),
	m_rightInvalidDist(-1000.0f),
	m_DEBUG_checkValid(false),
	m_leftEdgeCheck(true),
	m_rightEdgeCheck(true),
	m_maxPathLen(0)
{
	m_intervalMin = intervalMin;
	if(intervalMax > 0 )
		m_intervalMax = intervalMax;
	else
		m_intervalMax = m_intervalMin;
	m_startTime.SetSeconds(0.0f);

	m_ucs = m_ucsLast = UCS_NONE;
	m_ucsStartTime.SetSeconds(0.0f);
	m_leftInvalidStartTime.SetSeconds(0.0f);
	m_rightInvalidStartTime.SetSeconds(0.0f);
	m_curCoverUsage = USECOVER_NONE;
	//	m_ucsTime = 0.0f;
}


//
//-------------------------------------------------------------------------------------------------------------
void COPUseCover::Reset(CPipeUser* pPipeUser)
{
	if(pPipeUser && !pPipeUser->m_CurrentHideObject.IsSmartObject())
	{
		pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
		pPipeUser->m_State.lean = 0.0f;
	}

	if(pPipeUser && pPipeUser->m_CurrentHideObject.IsSmartObject())
	{
		if ( m_fTimeOut >= 0.0f )
			return;
	}

	m_startTime.SetSeconds(0.0f);
	m_curCoverUsage = USECOVER_NONE;
	m_coverCompromised = false;
	m_targetReached = false;
	m_leftInvalidStartTime.SetSeconds(0.0f);
	m_rightInvalidStartTime.SetSeconds(0.0f);
	m_leftInvalidDist = 1000.0f;
	m_rightInvalidDist = -1000.0f;
	m_leftEdgeCheck = true;
	m_rightEdgeCheck = true;
	m_DEBUG_checkValid = false;
	m_fTimeOut = 0;
	m_ucs = m_ucsLast = UCS_NONE;
	m_ucsStartTime.SetSeconds(0.0f);
	m_startTime.SetSeconds(0.0f);
	m_maxPathLen = 0;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPUseCover::RandomizeTimeOut(CPipeUser* pPipeUser)
{
	m_fTimeOut = m_intervalMin + (m_intervalMax - m_intervalMin) * ((float)ai_rand() / (float)RAND_MAX);
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPUseCover::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if( !pPipeUser->m_CurrentHideObject.IsValid() )
	{
		m_fTimeOut = -1.0f;
		Reset(pPipeUser);
		return eGOR_FAILED;
	}

	if(m_fTimeOut < 0.0001f)
	{
		RandomizeTimeOut(pPipeUser);
		m_startTime = GetAISystem()->GetFrameStartTime();
	}

	if(m_targetReached)
	{
		//		m_fTimeOut -= dt;
		//		if(m_fTimeOut < 0.0f)
		float	elapsed = (GetAISystem()->GetFrameStartTime() - m_startTime).GetSeconds();
		if(elapsed > m_fTimeOut)
		{
			Reset(pPipeUser);
			return eGOR_SUCCEEDED;
		}
	}

	if( pPipeUser->m_CurrentHideObject.IsSmartObject() )
		return UseCoverSO(pPipeUser);
	else
		return UseCoverEnv(pPipeUser);
}


//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPUseCover::UseCoverSO(CPipeUser* pPipeUser)
{
	if (!m_targetReached)
	{
		// on first call of Execute just return false
		// this will prevent calling Execute twice in
		// only one Update in the beginning of goalop
		m_targetReached = true;
		return eGOR_IN_PROGRESS;
	}

//	if(!pPipeUser->m_CurrentHideObject.IsUsingCover() != m_bUnHide)
//		return false;

	IEntity* pUser = pPipeUser->GetEntity();
	IEntity* pObject = pPipeUser->m_CurrentHideObject.GetSmartObject().pObject->GetEntity();

	const char* szEventName = m_bUnHide ? "Unhide" : "Hide";
	int id = gAIEnv.pSmartObjectManager->SmartObjectEvent( szEventName, pUser, pObject );
	if ( !id )
	{
		pPipeUser->m_CurrentHideObject.Invalidate();
		pPipeUser->SetCoverCompromised();
		m_fTimeOut = -1.0f;
		return eGOR_FAILED;
	}

	pPipeUser->m_CurrentHideObject.SetUsingCover( !m_bUnHide );
	return eGOR_IN_PROGRESS;
}

//
//-------------------------------------------------------------------------------------------------------------
inline const Vec3& ChooseLeftSide(bool leftClipped, bool rightClipped, const Vec3& peekPosLeft, const Vec3& peekPosRight, int& sideOut)
{
/*	if(leftClipped && !rightClipped)
	{
		// Right
		sideOut = 1;
		return peekPosRight;
	}
	else*/
	{
		// Left
		sideOut = -1;
		return peekPosLeft;
	}
}

//
//-------------------------------------------------------------------------------------------------------------
inline const Vec3& ChooseRightSide(bool leftClipped, bool rightClipped, const Vec3& peekPosLeft, const Vec3& peekPosRight, int& sideOut)
{
/*	if(rightClipped && !leftClipped)
	{
		// Left
		sideOut = -1;
		return peekPosLeft;
	}
	else*/
	{
		// Right
		sideOut = 1;
		return peekPosRight;
	}
}

inline bool ShouldMoveOut(float error, int side)
{
	// Get outside the range
	if(side == 0)
	{
		if(fabsf(error) > 0.3f)
			return true;
	}
	else if(side < 0)
	{
		if(error > 0)
			return true;
	}
	else if(side > 0)
	{
		if(error < 0)
			return true;
	}

	return false;
}

inline bool ShouldMoveIn(float error, int side)
{
	if(side == 0)
	{
		if(fabsf(error) > 0.3f)
			return true;
	}
	else if(side < 0)
	{
		if(error < 0)
			return true;
	}
	else if(side > 0)
	{
		if(error > 0)
			return true;
	}

	return false;
}


//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPUseCover::UseCoverEnv(CPipeUser* pPipeUser)
{
	CAIHideObject& hide = pPipeUser->m_CurrentHideObject;

	CPuppet*	puppet = pPipeUser->CastToCPuppet();
	if(!puppet)
		return eGOR_FAILED;

	if(pPipeUser->m_CurrentHideObject.IsSmartObject())
		return eGOR_FAILED;

	if(!hide.IsCoverPathComplete())
	{
		hide.HurryUpCoverPathGen();
		return eGOR_IN_PROGRESS;
	}

	IAIObject* pTarget = pPipeUser->GetAttentionTarget();
	if(!pTarget)
	{
		if(m_useLastOpAsBackup)
			pTarget = pPipeUser->GetLastOpResult();
		if(!pTarget)
		{
			Reset(pPipeUser);
			return eGOR_FAILED;
		}
	}

	Vec3	targetPos = pTarget->GetPos();


	bool	cannotUnhide(false);
	if(m_bUnHide && m_curCoverUsage == USECOVER_NONE)
	{
		ChooseCoverUsage(pPipeUser);
		// If the cover usage is none it means there is no way to unhide, signal to change cover.
		cannotUnhide = m_curCoverUsage == USECOVER_NONE;
	}

//	if(m_bUnHide)
//		assert(m_curCoverUsage >= USECOVER_STRAFE_LEFT_STANDING && m_curCoverUsage <= USECOVER_CENTER_CROUCHED);

	UpdateInvalidSeg(pPipeUser);

	if(fabsf(m_leftInvalidDist) > hide.GetMaxCoverPathLen()/2.0f)
		m_leftInvalidStartTime = GetAISystem()->GetFrameStartTime();	//reset
	if(fabsf(m_rightInvalidDist) > hide.GetMaxCoverPathLen()/2.0f)
		m_rightInvalidStartTime = GetAISystem()->GetFrameStartTime();	//reset

	float	elapsed = (GetAISystem()->GetFrameStartTime() - m_startTime).GetSeconds();
	float	timeLeft = m_fTimeOut - elapsed;

	const Vec3&	opPos = pPipeUser->GetPhysicsPos();

	if(m_ucs == UCS_NONE)
	{
		m_ucsStartTime = GetAISystem()->GetFrameStartTime();
		m_ucsSide = 0;

		m_moveTarget = opPos;

		float	peekOverLeft = -0.1f; // + m_weaponOffset;
		float	peekOverRight = -0.1f; // - m_weaponOffset;

		if(m_bUnHide)
		{
/*			if(m_curCoverUsage == USECOVER_STRAFE_TOP_LEFT_STANDING || m_curCoverUsage == USECOVER_STRAFE_TOP_RIGHT_STANDING)
			{
				// Stay inside the cover
				peekOverLeft += -0.6f;
				peekOverRight += -0.6f;
			}*/
		}
		else
		{
			// Stay inside the cover
			peekOverLeft += -0.6f;
			peekOverRight += -0.6f;
		}

		m_coverCompromised = false;
		bool	umbraClampedLeft = false, umbraClampedRight = false;
		bool	useLowCover = hide.HasLowCover();

		hide.GetCoverPoints(useLowCover, peekOverLeft, peekOverRight, targetPos, m_hidePos,
			m_peekPosLeft, m_peekPosRight, umbraClampedLeft, umbraClampedRight, m_coverCompromised);

		// TODO: Enable again once the animations can keep the AI at right spot.
		// If drifting too far away from the path, mark the cover compromised.
		//	if(!hide.IsNearCover(pPipeUser))
		//		m_coverCompromised = true;

		if(!hide.IsLeftEdgeValid(useLowCover))
			umbraClampedLeft = true;
		if(!hide.IsRightEdgeValid(useLowCover))
			umbraClampedRight = true;

		// Clamp to movement range.
		float	leftDist = hide.GetDistanceAlongCoverPath(m_peekPosLeft);
		float	rightDist = hide.GetDistanceAlongCoverPath(m_peekPosRight);

		float leftInvalidTime = (GetAISystem()->GetFrameStartTime() - m_leftInvalidStartTime).GetSeconds();
		float rightInvalidTime = (GetAISystem()->GetFrameStartTime() - m_rightInvalidStartTime).GetSeconds();

		if(leftInvalidTime > 0.01f)
		{
			if(leftDist < m_leftInvalidDist)
			{
				m_peekPosLeft += hide.GetCoverPathDir() * (m_leftInvalidDist - leftDist);
				umbraClampedLeft = true;
			}
			if(rightDist < m_leftInvalidDist)
			{
				m_peekPosRight += hide.GetCoverPathDir() * (m_leftInvalidDist - rightDist);
				umbraClampedRight = true;
			}
		}

		if(rightInvalidTime > 0.01f)
		{
			if(leftDist > m_rightInvalidDist)
			{
				m_peekPosLeft += hide.GetCoverPathDir() * (m_rightInvalidDist - leftDist);
				umbraClampedLeft = true;
			}
			if(rightDist > m_rightInvalidDist)
			{
				m_peekPosRight += hide.GetCoverPathDir() * (m_rightInvalidDist - rightDist);
				umbraClampedRight = true;
			}
		}

		umbraClampedLeft = umbraClampedLeft || !m_leftEdgeCheck;
		umbraClampedRight = umbraClampedRight || !m_rightEdgeCheck;

		m_hidePos = (m_peekPosLeft + m_peekPosRight) * 0.5f;
		m_moveStance = STANCE_STAND;

		if(m_bUnHide)
		{
			switch(m_curCoverUsage)
			{
			case USECOVER_STRAFE_LEFT_STANDING:
				m_moveTarget = ChooseLeftSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,1.5f);
				break;
			case USECOVER_STRAFE_RIGHT_STANDING:
				m_moveTarget = ChooseRightSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,1.5f);
				break;
			case USECOVER_STRAFE_TOP_STANDING:
				m_ucsSide = 0;
				m_moveTarget = m_hidePos;
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,1.5f);
				break;
			case USECOVER_STRAFE_TOP_LEFT_STANDING:
				m_moveTarget = ChooseLeftSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,1.5f);
				break;
			case USECOVER_STRAFE_TOP_RIGHT_STANDING:
				m_moveTarget = ChooseRightSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,1.5f);
				break;
			case USECOVER_STRAFE_LEFT_CROUCHED:
				m_moveTarget = ChooseLeftSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,0.7f);
				break;
			case USECOVER_STRAFE_RIGHT_CROUCHED:
				m_moveTarget = ChooseRightSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
//				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,0.7f);
				break;
			case USECOVER_CENTER_CROUCHED:
				m_ucsSide = 0;
//				m_moveTarget = m_hidePos;
				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,0.7f);
				break;
			default:
				// Unhandled case.
//				AIAssert(0);
				m_ucsSide = 0;
				m_moveStance = STANCE_CROUCH;
				m_pose = m_moveTarget + Vec3(0,0,0.7f);
				break;
			}

			float	moveTargetOnLine = hide.GetDistanceAlongCoverPath(m_moveTarget);
			float	posOnLine = hide.GetDistanceAlongCoverPath(opPos);
			float	dist = fabsf(moveTargetOnLine - posOnLine);
//			if(dist > 0.4f && hide.HasLowCover())
//				m_moveStance = STANCE_CROUCH;
			if(dist > 1.5f)
				m_moveStance = STANCE_STEALTH;
			else
				m_moveStance = STANCE_CROUCH;
		}
		else
		{
			if(hide.GetCoverWidth(hide.HasLowCover()) > 1.6f)
			{
				if(Distance::Point_Point2DSq(opPos, m_peekPosLeft) < Distance::Point_Point2DSq(opPos, m_peekPosRight))
				{
					m_moveTarget = ChooseLeftSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
					m_pose = m_moveTarget + Vec3(0,0,1.5f);
				}
				else
				{
					m_moveTarget = ChooseRightSide(umbraClampedLeft, umbraClampedRight, m_peekPosLeft, m_peekPosRight, m_ucsSide);
					m_pose = m_moveTarget + Vec3(0,0,1.5f);
				}
			}
			else
			{
				m_ucsSide = 0;
				m_moveTarget = m_hidePos;
				m_pose = m_moveTarget + Vec3(0,0,0.7f);
			}

			float	moveTargetOnLine = hide.GetDistanceAlongCoverPath(m_moveTarget);
			float	posOnLine = hide.GetDistanceAlongCoverPath(opPos);
			float	dist = fabsf(moveTargetOnLine - posOnLine);

			if(dist > 2.0f && hide.HasHighCover())
				m_moveStance = STANCE_STAND;
			else
				m_moveStance = STANCE_CROUCH;
		}



		// Steer towards move target.
		Vec3	delta = m_moveTarget - opPos;
		float	dist = delta.GetLength2D();
		float	moveTargetOnLine = hide.GetDistanceAlongCoverPath(m_moveTarget);
		float	posOnLine = hide.GetDistanceAlongCoverPath(opPos);
		float	error = posOnLine - moveTargetOnLine;

		if(fabsf(error) > 0.3f || m_ucsLast == UCS_NONE)
		{
			// Move
			m_ucsLast = m_ucs = UCS_MOVE;
			m_maxPathLen = 0.0f;

/*			if(m_bUnHide)
				m_ucsMoveIn = !ShouldMoveOut(error, m_ucsSide);
			else
				m_ucsMoveIn = ShouldMoveIn(error, m_ucsSide);*/
			m_ucsMoveIn = error < 0.0f;

			pPipeUser->m_State.bodystate = m_moveStance;
			pPipeUser->m_State.lean = 0.0f;
			pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);

		}
		else
		{
			// Hold
			m_ucsLast = m_ucs = UCS_HOLD;
			m_targetReached = true;

			m_ucsWaitTime = 0.6f + ai_frand() * 0.4f;

			EStance	curStance = (EStance)pPipeUser->m_State.bodystate;
			float	curLean = pPipeUser->m_State.lean;

//			if(puppet->GetFireMode() != FIREMODE_OFF)
			if(m_bUnHide)
			{
				IPuppet::SPostureInfo posture;
				if(puppet->SelectAimPosture(&posture, targetPos))
				{
					pPipeUser->m_State.bodystate = posture.stance;
					pPipeUser->m_State.lean = posture.lean;
					pPipeUser->m_State.peekOver = posture.peekOver;

					if (!posture.agInput.empty())
						pPipeUser->GetProxy()->SetAGInput(AIAG_ACTION, posture.agInput.c_str());
					else
						pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
				}
				else
				{
					// Could not find new pose, keep stance and remove peek.
					pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
					pPipeUser->m_State.lean = 0;
					pPipeUser->m_State.peekOver = 0;
				}
			}
			else
			{
				// Update stance
				IPuppet::SPostureInfo posture;
				if(puppet->SelectHidePosture(&posture, targetPos))
				{
					pPipeUser->m_State.bodystate = posture.stance;
					pPipeUser->m_State.lean = posture.lean;
					pPipeUser->m_State.peekOver = posture.peekOver;

					if (!posture.agInput.empty())
						pPipeUser->GetProxy()->SetAGInput(AIAG_ACTION, posture.agInput.c_str());
					else
						pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
				}
				else
				{
					// Could not find new pose, keep stance and remove peek.
					pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
					pPipeUser->m_State.lean = 0;
					pPipeUser->m_State.peekOver = posture.peekOver;
				}
			}

			if((timeLeft - m_ucsWaitTime) < 1.0f)
			{
				// Try to avoid really small timeout at the end.
				m_ucsWaitTime = timeLeft + 0.1f;
			}
			else
			{
				// Compensate for time taken to change the stance.
				if(curStance != (EStance)pPipeUser->m_State.bodystate)
					m_ucsWaitTime += 0.4f;
				// Compensate for time taken to change the lean.
				if(fabsf(curLean - pPipeUser->m_State.lean) > 0.01f)
					m_ucsWaitTime += 0.5f;

				if((timeLeft - m_ucsWaitTime) < 0.5f)
				{
					// Try to avoid really small timeout at the end.
					m_ucsWaitTime = timeLeft + 0.1f;
				}

			}
		}
	}

	if(m_ucs != UCS_NONE)
	{
		float	ucsElapsed = (GetAISystem()->GetFrameStartTime() - m_ucsStartTime).GetSeconds();

		if(m_ucs == UCS_MOVE)
		{
			// TODO: Use trace
			ExecuteDry(pPipeUser);
			// Failsafe
			if(ucsElapsed > 3.0f)
			{
				m_ucs = UCS_NONE;
				m_coverCompromised = true;
			}
		}
		else if(m_ucs == UCS_HOLD)
		{
			pPipeUser->m_State.fDistanceToPathEnd = 0;
			pPipeUser->m_State.fDesiredSpeed = 0;
			pPipeUser->m_State.vMoveDir.Set(0,0,0);

			// Change decision after a short timeout.
			if(ucsElapsed > m_ucsWaitTime)
				m_ucs = UCS_NONE;
		}
	}


	float leftInvalidTime = (GetAISystem()->GetFrameStartTime() - m_leftInvalidStartTime).GetSeconds();
	float rightInvalidTime = (GetAISystem()->GetFrameStartTime() - m_rightInvalidStartTime).GetSeconds();

	if(leftInvalidTime > 3.0f && rightInvalidTime > 3.0f)
		m_coverCompromised = true;

	if(!hide.IsValid())
		m_coverCompromised = true;

	if(m_coverCompromised || cannotUnhide)
	{
		pPipeUser->SetCoverCompromised();
		hide.Invalidate();
		return eGOR_FAILED;
	}

	return eGOR_IN_PROGRESS;
}


void COPUseCover::ExecuteDry(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if(pPipeUser->m_CurrentHideObject.IsSmartObject())
		return;
	if(!pPipeUser->m_CurrentHideObject.IsCoverPathComplete())
		return;

	CAIHideObject& hide = pPipeUser->m_CurrentHideObject;

	CPuppet*	puppet = pPipeUser->CastToCPuppet();
	if(!puppet)
		return;

	const Vec3&	opPos = pPipeUser->GetPhysicsPos();

	if(m_ucs == UCS_MOVE)
	{
		// TODO: Use trace


		// Steer towards move target.

		Vec3	delta = m_moveTarget - opPos;
		float	dist = delta.GetLength2D();

		float	moveTargetOnLine = hide.GetDistanceAlongCoverPath(m_moveTarget);
		float	posOnLine = hide.GetDistanceAlongCoverPath(opPos);
		float	error = posOnLine - moveTargetOnLine;
		float	errorNorm = error;
		if(m_ucsMoveIn) errorNorm = -errorNorm;

		puppet->AdjustMovementUrgency(pPipeUser->m_State.fMovementUrgency, error, &m_maxPathLen);

		Vec3	dir(0,0,0);
		float	speed = 0.25f;
		if(m_ucsSide != 0 && errorNorm > -0.3f)
		{
			if(m_ucsMoveIn)
				dir = hide.GetCoverPathDir();
			else
				dir = -hide.GetCoverPathDir();
/*			if(!m_ucsMoveIn)
			{
				if(m_ucsSide < 0)
					dir = -hide.GetCoverPathDir();
				else
					dir = hide.GetCoverPathDir();
			}
			else
			{
				if(m_ucsSide < 0)
					dir = hide.GetCoverPathDir();
				else
					dir = -hide.GetCoverPathDir();
			}*/
		}
		else
		{
			//			delta = m_moveTarget - hide.ProjectPointOnCoverPath(opPos);
			dir	= delta;
			dir.z = 0;
			dir.NormalizeSafe();
			speed = clamp(0.5f + (dist/1.5f), 0.0f, 1.0f);
		}

		float normalSpeed, minSpeed, maxSpeed;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, pPipeUser->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

		//		float normalSpeed = pPipeUser->GetNormalMovementSpeed(pPipeUser->m_State.fMovementUrgency, true);

		pPipeUser->m_State.fDesiredSpeed = speed * normalSpeed;
		pPipeUser->m_State.vMoveDir = dir;
		pPipeUser->m_State.fDistanceToPathEnd = dist;

		pPipeUser->m_DEBUGmovementReason = CPipeUser::AIMORE_MOVE;

		if(m_ucsMoveIn)
		{
			if(error > 0.0f)
//			if(!ShouldMoveOut(error, m_ucsSide))
				m_ucs = UCS_NONE;
		}
		else
		{
			if(error < 0.0f)
//			if(!ShouldMoveIn(error, m_ucsSide))
				m_ucs = UCS_NONE;
		}
	}
}

enum ECheckPosIdx
{
	CHECK_HIGH_MIDDLE,
	CHECK_HIGH_LEFT,
	CHECK_HIGH_RIGHT,
	CHECK_LOW_MIDDLE,
	CHECK_LOW_LEFT,
	CHECK_LOW_RIGHT,
	CHECK_NONE,
};

//
//-------------------------------------------------------------------------------------------------------------
void COPUseCover::ChooseCoverUsage(CPipeUser* pPipeUser)
{
	// Collect all possible options for this hide spot.
	CAIHideObject& hide = pPipeUser->m_CurrentHideObject;

	// Check if the sides are obstructed.
	IAIObject* pTarget = pPipeUser->GetAttentionTarget();
	if(!pTarget)
	{
		if(m_useLastOpAsBackup)
			pTarget = pPipeUser->GetLastOpResult();
	}
	if(!pTarget)
	{
		m_curCoverUsage = USECOVER_NONE;
		hide.SetCoverUsage(m_curCoverUsage);
		return;
	}

	const Vec3&	targetPos = pTarget->GetPos();
	const Vec3&	opPos = pPipeUser->GetPhysicsPos();

	float	peekOverLeft = max(0.1f, 0.3f + m_weaponOffset);
	float	peekOverRight = max(0.1f, 0.3f - m_weaponOffset);

	

	Vec3	tmp;
	Vec3	checkPos[6];
	bool	checkRes[6] = { false, false, false, false, false, false };
	bool	umbraLeftClamped, umbraRightClamped, comp;

	hide.GetCoverPoints(true, peekOverLeft, peekOverRight, targetPos, checkPos[CHECK_LOW_MIDDLE],
		checkPos[CHECK_LOW_LEFT], checkPos[CHECK_LOW_RIGHT], umbraLeftClamped, umbraRightClamped, comp);
	checkPos[CHECK_LOW_MIDDLE].z += 0.7f;
	checkPos[CHECK_LOW_LEFT].z += 0.7f;
	checkPos[CHECK_LOW_RIGHT].z += 0.7f;
	checkRes[CHECK_LOW_LEFT] = !umbraLeftClamped;
	checkRes[CHECK_LOW_RIGHT] = !umbraLeftClamped;


	hide.GetCoverPoints(false, peekOverLeft, peekOverRight, targetPos, checkPos[CHECK_HIGH_MIDDLE],
		checkPos[CHECK_HIGH_LEFT], checkPos[CHECK_HIGH_RIGHT], umbraLeftClamped, umbraRightClamped, comp);
	checkPos[CHECK_HIGH_MIDDLE].z += 1.5f;
	checkPos[CHECK_HIGH_LEFT].z += 1.5f;
	checkPos[CHECK_HIGH_RIGHT].z += 1.5f;
	checkRes[CHECK_HIGH_LEFT] = !umbraLeftClamped;
	checkRes[CHECK_HIGH_RIGHT] = !umbraLeftClamped;

	for(unsigned i = 0; i < 6; ++i)
	{
		ray_hit hit;

		if(!checkRes[i]) continue;

		Vec3	dir = targetPos - checkPos[i];
		float len = dir.GetLength();
		if(len > 6.0f)
			dir *= 6.0f / len;

		checkRes[i] = gAIEnv.pWorld->RayWorldIntersection(checkPos[i], dir, COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER, &hit, 1) == 0;

		m_DEGUG_checkPos[i] = checkPos[i];
		m_DEGUG_checkDir[i] = dir;
		m_DEBUG_checkRes[i] = checkRes[i];
	}

	m_DEBUG_checkValid = true;

	typedef	std::vector<std::pair<ECoverUsage, ECheckPosIdx> >	VectorIntInt;

	VectorIntInt	coverUsage;

	bool	isWideCover = hide.GetCoverWidth(true) > 2.0f;

	if(hide.HasLowCover() && hide.GetCoverWidth(true) > 0.1f)
	{
		if(hide.HasHighCover() && hide.GetCoverWidth(false) > 0.1f)
		{
			if(hide.IsLeftEdgeValid(false))
			{
				if(checkRes[CHECK_HIGH_LEFT])
				{
					coverUsage.push_back(std::make_pair(USECOVER_STRAFE_LEFT_STANDING, CHECK_HIGH_LEFT));
				}
			}
			if(hide.IsRightEdgeValid(false))
			{
				if(checkRes[CHECK_HIGH_RIGHT])
				{
					coverUsage.push_back(std::make_pair(USECOVER_STRAFE_RIGHT_STANDING, CHECK_HIGH_RIGHT));
				}
			}
		}
		else
		{
			if(checkRes[CHECK_HIGH_MIDDLE])
			{
				coverUsage.push_back(std::make_pair(USECOVER_STRAFE_TOP_STANDING, CHECK_HIGH_MIDDLE));
			}
		}

		if(hide.IsLeftEdgeValid(true))
		{
			if(checkRes[CHECK_LOW_LEFT])
			{
				coverUsage.push_back(std::make_pair(USECOVER_STRAFE_LEFT_CROUCHED, CHECK_LOW_LEFT));
			}
		}
		if(hide.IsRightEdgeValid(true))
		{
			if(checkRes[CHECK_LOW_RIGHT])
			{
				coverUsage.push_back(std::make_pair(USECOVER_STRAFE_RIGHT_CROUCHED, CHECK_LOW_RIGHT));
			}
		}
	}
	else
	{
		if(checkRes[CHECK_LOW_MIDDLE])
		{
			coverUsage.push_back(std::make_pair(USECOVER_CENTER_CROUCHED, CHECK_LOW_MIDDLE));
		}
	}

	// Fail safe in case the better checks failed completely.
	if(coverUsage.empty())
	{
		if(hide.HasLowCover() && hide.GetCoverWidth(true) > 0.1f)
		{
			if(hide.HasHighCover() && hide.GetCoverWidth(false) > 0.1f)
			{
				if(hide.IsLeftEdgeValid(false))
					coverUsage.push_back(std::make_pair(USECOVER_STRAFE_LEFT_STANDING, CHECK_NONE));
				if(hide.IsRightEdgeValid(false))
					coverUsage.push_back(std::make_pair(USECOVER_STRAFE_RIGHT_STANDING, CHECK_NONE));
			}
			else
				coverUsage.push_back(std::make_pair(USECOVER_STRAFE_TOP_STANDING, CHECK_NONE));

			if(hide.IsLeftEdgeValid(true))
				coverUsage.push_back(std::make_pair(USECOVER_STRAFE_LEFT_CROUCHED, CHECK_NONE));
			if(hide.IsRightEdgeValid(true))
				coverUsage.push_back(std::make_pair(USECOVER_STRAFE_RIGHT_CROUCHED, CHECK_NONE));
		}
		else
			coverUsage.push_back(std::make_pair(USECOVER_CENTER_CROUCHED, CHECK_NONE));
	}

	// Remove the last option if it does not
	size_t	removeCount = 0;
	for(VectorIntInt::iterator it = coverUsage.begin(); it != coverUsage.end(); ++it)
	{
		if(it->first == hide.GetCoverUsage())
		{
			++removeCount;
		}
	}

	if(removeCount < coverUsage.size())
	{
		for(VectorIntInt::iterator it = coverUsage.begin(); it != coverUsage.end();)
		{
			if(it->first == hide.GetCoverUsage())
				it = coverUsage.erase(it);
			else
				++it;
		}
	}

	// Shuffle.
	if(!coverUsage.empty())
	{
		std::random_shuffle(coverUsage.begin(), coverUsage.end());
		m_curCoverUsage = coverUsage.front().first;

		switch(coverUsage.front().second)
		{
		case CHECK_HIGH_MIDDLE:
			m_leftEdgeCheck = true;
			m_rightEdgeCheck = true;
			break;
		case CHECK_HIGH_LEFT:
			m_leftEdgeCheck = true;
			m_rightEdgeCheck = checkRes[CHECK_HIGH_RIGHT];
			break;
		case CHECK_HIGH_RIGHT:
			m_leftEdgeCheck = checkRes[CHECK_HIGH_LEFT];
			m_rightEdgeCheck = true;
			break;
		case CHECK_LOW_MIDDLE:
			m_leftEdgeCheck = true;
			m_rightEdgeCheck = true;
			break;
		case CHECK_LOW_LEFT:
			m_leftEdgeCheck = true;
			m_rightEdgeCheck = checkRes[CHECK_LOW_RIGHT];
			break;
		case CHECK_LOW_RIGHT:
			m_leftEdgeCheck = checkRes[CHECK_LOW_LEFT];
			m_rightEdgeCheck = true;
			break;
		default:
			m_leftEdgeCheck = true;
			m_rightEdgeCheck = true;
			break;
		}
	}
	else
		m_curCoverUsage = USECOVER_NONE;

	hide.SetCoverUsage(m_curCoverUsage);

	// Reset state
	m_ucs = UCS_NONE;
	m_ucsStartTime.SetSeconds(0.0f);
//	m_ucsTime = 0.0f;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPUseCover::UpdateInvalidSeg(CPipeUser* pPipeUser)
{
	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	Vec3 pos = pPipeUser->GetPhysicsPos();
	float distAlongCoverPath = hideObject.GetDistanceAlongCoverPath(pos);
	float	radius = pPipeUser->GetParameters().m_fPassRadius + 0.3f;

	m_leftInvalidDist = -1000.0f;
	m_rightInvalidDist = 1000.0f;

	// Loop through the nearby puppets and check if they are invalidating some of the cover path.
	CAISystem::AIObjectOwners::iterator aiEnd = GetAISystem()->m_Objects.end();

	for (CAISystem::AIObjectOwners::iterator aiObject = GetAISystem()->m_Objects.begin() ; aiObject != aiEnd ; ++aiObject)
	{
		// Skip every object that is not a puppet.
		CAIObject* pObject = aiObject->second.GetAIObject();
		uint16 type = pObject->GetType();

		if ((type != AIOBJECT_PLAYER) && (type != AIOBJECT_PUPPET))
			continue;

		CPuppet* pPuppet = pObject->CastToCPuppet();
		if(!pPuppet)
			continue;

		// Skip self.
		if(pPuppet == pPipeUser)
			continue;

		// Skip puppets which are too far away from use and the path.
		Vec3 otherPos = pPuppet->GetPhysicsPos();
		if(Distance::Point_Point2DSq(pos, otherPos) > sqr(12.0f))
			continue;

		if(hideObject.GetDistanceToCoverPath(otherPos) > 1.5f)
			continue;

		float	otherDistAlongCoverPath = hideObject.GetDistanceAlongCoverPath(otherPos);
		if(fabsf(otherDistAlongCoverPath) > (hideObject.GetMaxCoverPathLen() * 0.5f))
			continue;

		float	otherRadius = pPuppet->GetParameters().m_fPassRadius + 0.3f;

		if(otherDistAlongCoverPath < distAlongCoverPath)
		{
			// Left side
			otherDistAlongCoverPath += otherRadius;
			if (otherDistAlongCoverPath > m_leftInvalidDist)
			{
				m_leftInvalidDist = otherDistAlongCoverPath;
				m_leftInvalidPos = hideObject.ProjectPointOnCoverPath(otherPos) + otherRadius * hideObject.GetCoverPathDir();
			}
		}
		else
		{
			// Right side
			otherDistAlongCoverPath -= otherRadius;
			if (otherDistAlongCoverPath < m_rightInvalidDist)
			{
				m_rightInvalidDist = otherDistAlongCoverPath;
				m_rightInvalidPos = hideObject.ProjectPointOnCoverPath(otherPos) - otherRadius * hideObject.GetCoverPathDir();
			}
		}
	}
}

//
//-------------------------------------------------------------------------------------------------------------
void COPUseCover::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPUseCover");
	{
		ser.Value("m_speedUpTimeOutWhenNoTarget",m_speedUpTimeOutWhenNoTarget);
		ser.Value("m_targetReached",m_targetReached);
		ser.Value("m_coverCompromised",m_coverCompromised);
		ser.Value("m_hidePos",m_hidePos);
		ser.Value("m_peekPosRight",m_peekPosRight);
		ser.Value("m_peekPosLeft",m_peekPosLeft);
		ser.Value("m_pose",m_pose);
		ser.Value("m_moveTarget",m_moveTarget);
		ser.EnumValue("m_curCoverUsage", m_curCoverUsage, USECOVER_NONE, USECOVER_LAST);
		ser.Value("m_weaponOffset",m_weaponOffset);
		ser.Value("m_intervalMin",m_intervalMin);
		ser.Value("m_intervalMax",m_intervalMax);
		ser.Value("m_fTimeOut",m_fTimeOut);
		ser.Value("m_startTime",m_startTime);
		ser.Value("m_bUnHide",m_bUnHide);
		ser.Value("m_useLastOpAsBackup",m_useLastOpAsBackup);
		//		ser.Value("m_leftInvalidTime",m_leftInvalidTime);
		ser.Value("m_leftInvalidStartTime",m_leftInvalidStartTime);
		ser.Value("m_leftInvalidDist",m_leftInvalidDist);
		ser.Value("m_leftInvalidPos",m_leftInvalidPos);
		//		ser.Value("m_rightInvalidTime",m_rightInvalidTime);
		ser.Value("m_rightInvalidStartTime",m_rightInvalidStartTime);
		ser.Value("m_rightInvalidDist",m_rightInvalidDist);
		ser.Value("m_rightInvalidPos",m_rightInvalidPos);
		ser.Value("m_leftEdgeCheck",m_leftEdgeCheck);
		ser.Value("m_rightEdgeCheck",m_rightEdgeCheck);
		ser.EnumValue("m_ucs",m_ucs, UCS_NONE, UCS_HOLD);
		ser.EnumValue("m_ucsLast",m_ucs, UCS_NONE, UCS_HOLD);
		//		ser.Value("m_ucsTime",m_ucsTime);
		ser.Value("m_ucsStartTime",m_ucsStartTime);
		ser.Value("m_ucsWaitTime",m_ucsWaitTime);
		ser.Value("m_ucsSide",m_ucsSide);
		ser.Value("m_ucsMoveIn", m_ucsMoveIn);
		ser.EnumValue("m_moveStance",m_moveStance, STANCE_NULL, STANCE_LAST);
	}
	ser.EndGroup();
}

//
//-------------------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------------------
void COPUseCover::DebugDraw(CPipeUser* pPipeUser) const
{
	if(!pPipeUser)
		return;

	CAIHideObject& hide = pPipeUser->m_CurrentHideObject;

	if(!hide.IsCoverPathComplete())
		return;

	CDebugDrawContext dc;

	// Hide pos
	dc->DrawCone(m_hidePos + Vec3(0, 0, 0.3f), Vec3(0, 0, -1), 0.1f, 0.3f, ColorB(255, 0, 0));
	// Unhide pos
	dc->DrawCone(m_peekPosLeft + Vec3(0, 0, 0.3f), Vec3(0, 0, -1), 0.1f, 0.3f, ColorB(0, 255, 0));
	dc->DrawCone(m_peekPosRight + Vec3(0, 0, 0.3f), Vec3(0, 0, -1), 0.1f, 0.3f, ColorB(0, 255, 0));
	// Pos
	dc->DrawSphere(m_pose, 0.2f, ColorB(255, 255, 255));

	// Draw invalid segments.
	float leftInvalidTime = (GetAISystem()->GetFrameStartTime() - m_leftInvalidStartTime).GetSeconds();
	float rightInvalidTime = (GetAISystem()->GetFrameStartTime() - m_rightInvalidStartTime).GetSeconds();

	if(leftInvalidTime > 0.001f)
		dc->DrawArrow(m_leftInvalidPos + Vec3(0, 0, 0.25f), -hide.GetCoverPathDir() * (0.25f + leftInvalidTime), 0.3f, ColorB(255, 0, 0, 200));
	if(rightInvalidTime > 0.001f)
		dc->DrawArrow(m_rightInvalidPos + Vec3(0, 0, 0.25f), hide.GetCoverPathDir() * (0.25f + rightInvalidTime), 0.3f, ColorB(255, 0, 0, 200));

	if(m_DEBUG_checkValid)
	{
		for(unsigned i = 0; i < 6; ++i)
		{
			ColorB	color(255, 255, 255);
			if(!m_DEBUG_checkRes[i])
				color.Set(255, 0, 0, 255);
			dc->DrawCone(m_DEGUG_checkPos[i], m_DEGUG_checkDir[i].GetNormalized(), 0.05f, 0.2f, color);
		}
	}
	const char*	szMsg = "";
	if(m_ucs == UCS_NONE)
		szMsg = "UCS_NONE";
	else if(m_ucs == UCS_MOVE)
	{
		if(m_ucsMoveIn)
			szMsg = "UCS_MOVE in";
		else
			szMsg = "UCS_MOVE out";
	}
	else if(m_ucs == UCS_HOLD)
		szMsg = "UCS_HOLD";

	float	elapsed = (GetAISystem()->GetFrameStartTime() - m_ucsStartTime).GetSeconds();

	dc->Draw3dLabel(pPipeUser->GetPhysicsPos() + Vec3(0, 0, 0.3f), 1.0f, "%s %s - %.2fs", m_bUnHide ? "UNHIDE" : "HIDE", szMsg, elapsed);


/*
	IAIObject* pTarget = pPipeUser->GetAttentionTarget();
	if(!pTarget)
	{
		if(m_useLastOpAsBackup)
			pTarget = pPipeUser->GetLastOpResult();
		if(!pTarget)
			return;
	}

	Vec3	targetPos = pTarget->GetPos();

	Vec3	hidePos, peekPosLeft, peekPosRight;
	bool	umbraClampedLeft = false, umbraClampedRight = false, coverCompromised = false;

	hide.GetCoverPoints(true, -0.1f, -0.1f, targetPos, hidePos,
		peekPosLeft, peekPosRight, umbraClampedLeft, umbraClampedRight, coverCompromised);

	dc->DrawLine(hidePos + Vec3(0,0,0.5f), ColorB(255,255,255), hidePos + Vec3(0,0,0.5f) + hide.GetCoverPathDir(),ColorB(255,255,255));

	dc->DrawCone(hidePos + Vec3(0, 0, 0.5f), Vec3(0, 0, -1), 0.05f, 0.2f, ColorB(255, 255, 255, coverCompromised ? 128 : 255));
	dc->DrawCone(peekPosLeft + Vec3(0, 0, 0.5f), Vec3(0, 0, -1), 0.05f, 0.2f, ColorB(255, 64, 0, umbraClampedLeft ? 128 : 255));
	dc->DrawCone(peekPosRight + Vec3(0, 0, 0.5f), Vec3(0, 0, -1), 0.05f, 0.2f, ColorB(255, 128, 0, umbraClampedRight ? 128 : 255));
*/
}

//
//-------------------------------------------------------------------------------------------------------------
COPWait::COPWait( int waitType, int blockCount ):
m_WaitType(static_cast<EOPWaitType>(waitType)),
m_BlockCount(blockCount)
{

}

//
//-------------------------------------------------------------------------------------------------------------
COPWait::COPWait(const XmlNodeRef& node) :
	m_WaitType(WAIT_ALL),
	m_BlockCount(0)
{
	const char* szWaitType = node->getAttr("for");
	if (!stricmp(szWaitType, "Any"))
	{
		m_WaitType = WAIT_ANY;
	}
	else
	{
		if (!stricmp(szWaitType, "Any2"))
		{
			m_WaitType = WAIT_ANY_2;
		}
	}
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPWait::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	int	currentBlockCount = pPipeUser->CountGroupedActiveGoals();
	bool	done = false;
	switch(m_WaitType)
	{
		case WAIT_ALL:
				done = currentBlockCount == 0;
				break;
		case WAIT_ANY:
				done = currentBlockCount < m_BlockCount;
				break;
		case WAIT_ANY_2:
				done = currentBlockCount+1 < m_BlockCount;
				break;
		default:
				done = true;
	}
	if(done)
		pPipeUser->ClearGroupedActiveGoals();

	return done ? eGOR_DONE : eGOR_IN_PROGRESS;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPWait::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPWait");
	{
		ser.EnumValue("m_WaitType", m_WaitType, WAIT_ALL, WAIT_LAST);
		ser.Value("m_BlockCount", m_BlockCount);	
		ser.EndGroup();
	}
}


//
//-------------------------------------------------------------------------------------------------------------
COPAdjustAim::COPAdjustAim(bool hide, bool useLastOpAsBackup, bool allowProne) :
m_hide(hide),
m_useLastOpAsBackup(useLastOpAsBackup),
m_allowProne(allowProne)
{
	m_startTime.SetMilliSeconds(0);
	m_evalTime = RandomizeEvalTime();
}

//
//-------------------------------------------------------------------------------------------------------------
float COPAdjustAim::RandomizeEvalTime()
{
	return 0.4f + ai_frand() * 0.3f;
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPAdjustAim::Execute( CPipeUser* pPipeUser )
{
	CCCPOINT(COPAdjustAim_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CPuppet*	puppet = pPipeUser->CastToCPuppet();
	if(!puppet)
		return eGOR_FAILED;

	CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();

	Vec3	targetPos;
	if(pPipeUser->GetAttentionTarget())
	{
		targetPos = pPipeUser->GetAttentionTarget()->GetPos();
	}
	else if(m_useLastOpAsBackup && pLastOpResult)
	{
		targetPos = pLastOpResult->GetPos();
	}
	else
	{
		// Cannot adjust, no target at all.
		return eGOR_FAILED;
	}

	if(m_startTime.GetSeconds() < 0.001f)
	{
		// Set time in past to force the first eval.
		m_startTime = GetAISystem()->GetFrameStartTime() - CTimeValue(10.0f);
	}

	float	elapsed = (GetAISystem()->GetFrameStartTime() - m_startTime).GetSeconds();


	float	distToTarget = Distance::Point_Point(targetPos, pPipeUser->GetPos());

	bool isMoving = pPipeUser->GetVelocity().GetLength() > 0.01f || (pPipeUser->m_State.vMoveDir.GetLength() > 0.01f && pPipeUser->m_State.fDesiredSpeed > 0.01f);

	SAIWeaponInfo	weaponInfo;
	pPipeUser->GetProxy()->QueryWeaponInfo(weaponInfo);
	if (elapsed > m_evalTime && weaponInfo.isReloading)
		m_evalTime = elapsed + 0.2f;

	if (isMoving)
	{
		int idx = MovementUrgencyToIndex(pPipeUser->m_State.fMovementUrgency);
		if (idx <= AISPEED_WALK)
		{
			if (elapsed > m_evalTime)
			{
				IPuppet::SPostureInfo posture;

				bool stanceOk = false;
				if (m_hide)
					stanceOk = puppet->SelectHidePosture(&posture, targetPos, false, m_allowProne && pPipeUser->m_State.fDistanceToPathEnd < 1.3f);
				else
					stanceOk = puppet->SelectAimPosture(&posture, targetPos, false, m_allowProne && pPipeUser->m_State.fDistanceToPathEnd < 1.3f);

				if (stanceOk)
					pPipeUser->m_State.bodystate = posture.stance;

				m_startTime = GetAISystem()->GetFrameStartTime();
				m_evalTime = RandomizeEvalTime();
				if (pPipeUser->m_State.bodystate == STANCE_PRONE)
					m_evalTime += 2.5f;
			}

		}
		else
		{
	if(elapsed > m_evalTime)
	{
				m_evalTime = 1.0f;
				m_startTime = GetAISystem()->GetFrameStartTime() - CTimeValue(10.0f);

				if (m_hide)
					pPipeUser->m_State.bodystate = distToTarget > 8.0f ? STANCE_STEALTH : STANCE_STAND;
				else
					pPipeUser->m_State.bodystate = STANCE_STAND;
			}
		}

		pPipeUser->m_State.lean = 0;
		pPipeUser->m_State.peekOver = 0;
		pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
	}
	else
	{
		if (elapsed > m_evalTime)
		{
			IPuppet::SPostureInfo posture;
		
			bool stanceOk = false;
			if(m_hide)
				stanceOk = puppet->SelectHidePosture(&posture, targetPos, true, m_allowProne);
			else
				stanceOk = puppet->SelectAimPosture(&posture, targetPos, true, m_allowProne);

			if(stanceOk)
			{
				pPipeUser->m_State.bodystate = posture.stance;
				pPipeUser->m_State.lean = posture.lean;
				pPipeUser->m_State.peekOver = posture.peekOver;

				if (!posture.agInput.empty())
					pPipeUser->GetProxy()->SetAGInput(AIAG_ACTION, posture.agInput.c_str());
				else
					pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
			}
			else
			{
				// Could not find new pose, keep stance and remove peek.
				pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
				pPipeUser->m_State.lean = 0;
				pPipeUser->m_State.peekOver = 0;
			}

			m_startTime = GetAISystem()->GetFrameStartTime();
			m_evalTime = RandomizeEvalTime();
			if (pPipeUser->m_State.bodystate == STANCE_PRONE)
				m_evalTime += 2.5f;
	}
	}


/*
	int	n = (int)floorf((float)elapsed / 5.0f) % 4;

	if(n == 0)
	{
		pPipeUser->GetProxy()->SetAGInput(AIAG_ACTION, "peekLeft");
		pPipeUser->m_State.lean = -1.0f;
	}
	else if(n == 1)
	{
		pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
		pPipeUser->m_State.lean = 0;
	}
	else if(n == 2)
	{
		pPipeUser->GetProxy()->SetAGInput(AIAG_ACTION, "peekRight");
		pPipeUser->m_State.lean = 1.0f;
	}
	else
	{
		pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
		pPipeUser->m_State.lean = 0;
	}
*/

	return eGOR_IN_PROGRESS;
}


//
//-------------------------------------------------------------------------------------------------------------
void COPAdjustAim::Reset( CPipeUser* pPipeUser )
{
	pPipeUser->m_State.lean = 0.0f;
	pPipeUser->m_State.peekOver = 0.0f;
	pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);

	m_startTime.SetMilliSeconds(0);
	m_evalTime = 0.0f;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPAdjustAim::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPAdjustAim");
	{
		ser.Value("m_startTime", m_startTime);	
		ser.Value("m_evalTime", m_evalTime);
		ser.Value("m_hide", m_hide);	
		ser.Value("m_useLastOpAsBackup", m_useLastOpAsBackup);	
	}
	ser.EndGroup();
}


//
//-------------------------------------------------------------------------------------------------------------
void COPAdjustAim::DebugDraw(CPipeUser* pPipeUser) const
{
	if(!pPipeUser)
		return;

//	m_selector.DebugDraw(pPipeUser);

	Vec3	basePos = pPipeUser->GetPhysicsPos();
	Vec3	targetPos = pPipeUser->GetProbableTargetPosition();

	CDebugDrawContext dc;

/*	for(unsigned i = 0; i < 8; ++i)
	{
		if(!m_postures[i].valid) continue;
		dc->DrawCone(m_postures[i].weapon, m_postures[i].weaponDir, 0.05f, m_postures[i].targetAim ? 0.4f : 0.2f, ColorB(255,0,0, m_postures[i].targetAim ? 255 : 128));
		dc->DrawSphere(m_postures[i].eye, 0.1f, ColorB(255,255,255, m_postures[i].targetVis ? 255 : 128));
		dc->DrawLine(basePos, ColorB(255,255,255), m_postures[i].eye, ColorB(255,255, 255));

		if((int)i == m_bestPosture)
		{
			dc->DrawLine(m_postures[i].weapon, ColorB(255,0,0), targetPos, ColorB(255,0,0,0));
			dc->DrawLine(m_postures[i].eye, ColorB(255, 255, 255), targetPos, ColorB(255, 255, 255, 0));
		}
	}
*/

	Vec3	toeToHead = pPipeUser->GetPos() - pPipeUser->GetPhysicsPos();
	float	len = toeToHead.GetLength();
	if(len > 0.0001f)
		toeToHead *= (len - 0.3f) / len;
	dc->DrawSphere(pPipeUser->GetPhysicsPos() + toeToHead, 0.4f, ColorB(200, 128, 0, 90));

	dc->DrawLine(pPipeUser->GetFirePos(), ColorB(255, 0, 0), targetPos, ColorB(255, 0, 0, 64));
	dc->DrawLine(pPipeUser->GetPos(), ColorB(255, 255, 255), targetPos, ColorB(255, 255, 255, 64));
}

//===================================================================
// COPProximity
//===================================================================
COPProximity::COPProximity(float radius, const string& signalName, bool signalWhenDisabled, bool visibleTargetOnly) :
	m_radius(radius),
	m_signalName(signalName),
	m_signalWhenDisabled(signalWhenDisabled),
	m_visibleTargetOnly(visibleTargetOnly),
	m_triggered(false)
{
}

COPProximity::COPProximity(const XmlNodeRef& node) :
	m_radius(0.f),
	m_signalName(s_xml.GetMandatoryString(node, "signal")),
	m_signalWhenDisabled(s_xml.GetBool(node, "signalWhenDisabled")),
	m_visibleTargetOnly(s_xml.GetBool(node, "visibleTargetOnly")),
	m_triggered(false)
{
	s_xml.GetMandatory(node, "radius", m_radius);
}

//===================================================================
// ~COPProximity
//===================================================================
COPProximity::~COPProximity()
{
}

//===================================================================
// Execute
//===================================================================
EGoalOpResult COPProximity::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if (m_triggered)
		return eGOR_SUCCEEDED;

	CPuppet* pOperandPuppet = pPipeUser->CastToCPuppet();

	CAIObject* pTarget = (CAIObject*)pPipeUser->GetAttentionTarget();
	if (!pTarget)
	{
		Reset(pPipeUser);
		return eGOR_FAILED;
	}

	CCCPOINT(COPProximity_Execute);

	if (!m_refProxObject.IsValid())
	{
		CCCPOINT(COPProximity_Execute_NotValid);

		m_refProxObject = GetWeakRef(pPipeUser->GetLastOpResult());
		if (!m_refProxObject.IsValid())
		{
			Reset(pPipeUser);
			return eGOR_FAILED;
		}
		if (m_radius < 0.0f)
			m_radius = m_refProxObject.GetAIObject()->GetRadius();
	}

	CAIObject * const pProxObject = m_refProxObject.GetAIObject();
	CPipeUser * const pTargetPipeuser = pProxObject->CastToCPipeUser();
	bool trigger = false;

	bool b3D = (pTargetPipeuser && pTargetPipeuser->IsUsing3DNavigation() ) || (pTargetPipeuser && pTargetPipeuser->IsUsing3DNavigation());
	float distSqr = b3D? Distance::Point_PointSq(pTarget->GetPos(), pProxObject->GetPos()) : 
		Distance::Point_Point2DSq(pTarget->GetPos(), pProxObject->GetPos());

	if (m_signalWhenDisabled && !pProxObject->IsEnabled())
		trigger = true;

	if (!m_visibleTargetOnly || (m_visibleTargetOnly && pPipeUser->GetAttentionTargetType() == AITARGET_VISUAL))
	{
		if (distSqr < sqr(m_radius))
			trigger = true;
	}

	if (trigger)
	{
		CCCPOINT(COPProximity_Execute_Trigger);

		AISignalExtraData* pData = new AISignalExtraData;
		pData->fValue = sqrtf(distSqr);
		pPipeUser->SetSignal(0, m_signalName.c_str(), pPipeUser->GetEntity(), pData);
		m_triggered = true;
		return eGOR_SUCCEEDED;
	}

	return eGOR_IN_PROGRESS;
}

//===================================================================
// Reset
//===================================================================
void COPProximity::Reset(CPipeUser* pPipeUser)
{
	m_triggered = false;
	m_refProxObject.Reset();
}

//===================================================================
// Serialize
//===================================================================
void COPProximity::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup( "AICOPProximity" );
	{
		ser.Value("m_radius",m_radius);
		ser.Value("m_triggered",m_triggered);
		ser.Value("m_signalName",m_signalName);
		ser.Value("m_signalWhenDisabled",m_signalWhenDisabled);
		ser.Value("m_visibleTargetOnly",m_visibleTargetOnly);

		m_refProxObject.Serialize(ser, "m_ref_ProxObject");
	}
	ser.EndGroup();
}

//===================================================================
// DebugDraw
//===================================================================
void COPProximity::DebugDraw(CPipeUser* pPipeUser) const
{
	CDebugDrawContext dc;
	
	const CAIObject * const pProxObject = m_refProxObject.GetAIObject();
	if (pProxObject)
	{
		dc->DrawWireSphere(pProxObject->GetPos(), m_radius, ColorB(255, 255, 255));

		CPuppet* pOperandPuppet = pPipeUser->CastToCPuppet();
		if (!pOperandPuppet)
			return;
		CAIObject* pTarget = (CAIObject*)pPipeUser->GetAttentionTarget();
		if (pTarget)
		{
			Vec3 delta = pTarget->GetPos() - pProxObject->GetPos();
			delta.NormalizeSafe();
			delta *= m_radius;
			ColorB color;
			if (pPipeUser->GetAttentionTargetType() == AITARGET_VISUAL)
				color.Set(255, 255, 255, 255);
			else
				color.Set(255, 255, 255, 128);
			dc->DrawSphere(pProxObject->GetPos() + delta, 0.25f, color);
			dc->DrawLine(pProxObject->GetPos() + delta, color, pTarget->GetPos(), color);
		}
	}
}

//===================================================================
// COPMoveTowards
//===================================================================
COPMoveTowards::COPMoveTowards(float distance, float duration) :
	m_distance(distance),
	m_duration(duration),
	m_pPathfindDirective(0),
	m_pTraceDirective(0),
	m_looseAttentionId(0),
	m_moveDist(0),
	m_moveSearchRange(0),
	m_moveStart(0,0,0),
	m_moveEnd(0,0,0)
{
}

COPMoveTowards::COPMoveTowards(const XmlNodeRef& node) :
	m_distance(0.f),
	m_duration(0.f),
	m_pPathfindDirective(0),
	m_pTraceDirective(0),
	m_looseAttentionId(0),
	m_moveDist(0.f),
	m_moveSearchRange(0.f),
	m_moveStart(ZERO),
	m_moveEnd(ZERO)
{
	if (node->getAttr("time", m_duration))
	{
		m_duration = fabsf(m_duration);
	}
	else
	{
		s_xml.GetMandatory(node, "distance", m_distance);
	}
	
	float variation = 0.f;
	node->getAttr("variation", variation);
	
	// Random variation on the end distance.
	if (variation > 0.01f)
	{
		float u = (ai_rand() % 10) / 9.0f;
		m_distance = (m_distance > 0.0f) ? max(0.f, m_distance - u * variation)
		                                 : min(0.f, m_distance + u * variation);
	}
}

//===================================================================
// ~COPMoveTowards
//===================================================================
COPMoveTowards::~COPMoveTowards()
{
	Reset(0);
}

//===================================================================
// Reset
//===================================================================
void COPMoveTowards::Reset(CPipeUser* pPipeUser)
{
	ResetNavigation(pPipeUser);
}

//===================================================================
// ResetNavigation
//===================================================================
void COPMoveTowards::ResetNavigation(CPipeUser* pPipeUser)
{
	if (m_pPathfindDirective)
		delete m_pPathfindDirective;
	m_pPathfindDirective = 0;
	if (m_pTraceDirective)
		delete m_pTraceDirective;
	m_pTraceDirective = 0;

	if (pPipeUser)
	{
		gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);
		pPipeUser->ClearPath("COPBackoff::Reset m_Path");
	}

	if (m_looseAttentionId)
	{
		if (pPipeUser)
		pPipeUser->SetLooseAttentionTarget(NILREF,m_looseAttentionId);
		m_looseAttentionId = 0;
	}
}

//===================================================================
// Execute
//===================================================================
void COPMoveTowards::ExecuteDry(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if (m_pTraceDirective)
		m_pTraceDirective->ExecuteTrace(pPipeUser, false);
}

//===================================================================
// GetEndDistance
//===================================================================
float COPMoveTowards::GetEndDistance(CPipeUser* pPipeUser) const
{
	if (m_duration > 0.0f)
	{
		float normalSpeed, minSpeed, maxSpeed;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, false, normalSpeed, minSpeed, maxSpeed);
		//		float normalSpeed = pPipeUser->GetNormalMovementSpeed(pPipeUser->m_State.fMovementUrgency, false);
		if (normalSpeed > 0.0f)
			return normalSpeed * m_duration;
	}
	return m_distance;
}

//===================================================================
// Execute
//===================================================================
EGoalOpResult COPMoveTowards::Execute(CPipeUser* pPipeUser)
{ 
	CCCPOINT(COPMoveTowards_Execute);
FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!m_pPathfindDirective)
	{
		CAIObject* pTarget = pPipeUser->m_refLastOpResult.GetAIObject();
		if (!pTarget)
		{
			Reset(pPipeUser);
			return eGOR_FAILED;
		}

		Vec3 operandPos = pPipeUser->GetPhysicsPos();
		Vec3 targetPos = pTarget->GetPhysicsPos();

		float moveDist = GetEndDistance(pPipeUser);
		float searchRange = Distance::Point_Point(operandPos, targetPos);
//		moveDist = min(moveDist, searchRange);
		searchRange = max(searchRange, moveDist);
		searchRange *= 2.0f;

		// start pathfinding
		gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);
		m_pPathfindDirective = new COPPathFind("MoveTowards", pTarget, std::numeric_limits<float>::max(),
			-moveDist, false, searchRange);
		pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

		// Store params for debug drawing.
		m_moveStart = operandPos;
		m_moveEnd = targetPos;
		m_moveDist = moveDist;
		m_moveSearchRange = searchRange;

		if (m_pPathfindDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS)
		{
			if (pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
			{
				ResetNavigation(pPipeUser);	
				return eGOR_FAILED;
			}
		}
		return eGOR_IN_PROGRESS;
	}


	if (pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND)
	{
		if (pPipeUser->m_Path.GetPath().size() == 1)
		{
			AIWarning("COPMoveTowards::Entity %s Path has only one point.", GetNameSafe(pPipeUser));
			return eGOR_FAILED;
		}

		if (!m_pTraceDirective)
		{
			m_pTraceDirective = new COPTrace(false, defaultTraceEndAccuracy);
//			pPipeUser->m_Path.GetParams().precalculatedPath = true; // prevent path regeneration
		}

		// keep tracing
		EGoalOpResult done = m_pTraceDirective->Execute(pPipeUser);
		if (done != eGOR_IN_PROGRESS)
		{
			Reset(pPipeUser);
			return done;
		}
		// If this goal gets reseted during m_pTraceDirective->Execute it means that
		// a smart object was used for navigation which inserts a goal pipe which
		// does Reset on this goal which sets m_pTraceDirective to NULL! In this case
		// we should just report that this goal pipe isn't finished since it will be
		// reexecuted after finishing the inserted goal pipe
		if (!m_pTraceDirective)
			return eGOR_IN_PROGRESS;
	}
	else if (pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
	{
		Reset(pPipeUser);	
		return eGOR_FAILED;
	}
	else
	{
		m_pPathfindDirective->Execute(pPipeUser);
	}

	return eGOR_IN_PROGRESS;
}

//===================================================================
// Serialize
//===================================================================
void COPMoveTowards::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPMoveTowards");
	{
		ser.Value("m_distance", m_distance);
		ser.Value("m_duration", m_duration);
		ser.Value("m_moveStart", m_moveStart);
		ser.Value("m_moveEnd", m_moveEnd);
		ser.Value("m_moveDist", m_moveDist);
		ser.Value("m_moveSearchRange", m_moveSearchRange);

		if(ser.IsWriting())
		{
			if (ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective != NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if (ser.BeginOptionalGroup("PathFindDirective", m_pPathfindDirective != NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
			SAFE_DELETE(m_pTraceDirective);
			if (ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(true);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			SAFE_DELETE(m_pPathfindDirective);
			if (ser.BeginOptionalGroup("PathFindDirective", true))
			{
				m_pPathfindDirective = new COPPathFind("");
				m_pPathfindDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
	}
	ser.EndGroup();
}

//===================================================================
// DebugDraw
//===================================================================
void COPMoveTowards::DebugDraw(CPipeUser* pPipeUser) const
{
	Vec3	dir(m_moveEnd - m_moveStart);
	dir.Normalize();

	Vec3 start = m_moveStart;
	Vec3 end = m_moveEnd;
	start.z += 0.5f;
	end.z += 0.5f;

	CDebugDrawContext dc;

	dc->DrawRangeArc(start, dir, DEG2RAD(60.0f), m_moveDist, m_moveDist - 0.5f, ColorB(255, 255, 255, 16), ColorB(255, 255, 255), true);
	dc->DrawRangeArc(start, dir, DEG2RAD(60.0f), m_moveSearchRange, 0.2f, ColorB(255, 0, 0, 128), ColorB(255, 0, 0), true);

	dc->DrawLine(start, ColorB(255, 255, 255), end, ColorB(255, 255, 255));
}


//====================================================================
// COPDodge
//====================================================================
COPDodge::COPDodge(float distance, bool useLastOpAsBackup) :
m_distance(distance),
m_useLastOpAsBackup(useLastOpAsBackup),
m_notMovingTime(0.0f)
{
	m_pTraceDirective = 0;
	m_basis.SetIdentity();
	m_targetPos.Set(0,0,0);
	m_targetView.Set(0,0,0);
}

COPDodge::COPDodge(const XmlNodeRef& node) :
	m_distance(0),
	m_useLastOpAsBackup(s_xml.GetBool(node, "useLastOpAsBackup")),
	m_endAccuracy(0.f),
	m_notMovingTime(0.0f),
	m_pTraceDirective(0),
	m_targetPos(ZERO),
	m_targetView(ZERO)
{
	s_xml.GetMandatory(node, "radius", m_distance);

	m_basis.SetIdentity();
}

//====================================================================
// ~COPDodge
//====================================================================
COPDodge::~COPDodge()
{
	Reset(0);
}

//====================================================================
// Reset
//====================================================================
void COPDodge::Reset(CPipeUser* pPipeUser)
{
	delete m_pTraceDirective;
	m_pTraceDirective = 0;
	m_notMovingTime = 0;
	m_basis.SetIdentity();
	m_targetPos.Set(0,0,0);
	m_targetView.Set(0,0,0);
	if (pPipeUser)
		pPipeUser->ClearPath("COPDodge::Reset");
}

//====================================================================
// Serialize
//====================================================================
void COPDodge::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPDodge");
	{
		ser.Value("m_distance", m_distance);
		ser.Value("m_useLastOpAsBackup", m_useLastOpAsBackup);
		ser.Value("m_lastTime", m_lastTime);
		ser.Value("m_notMovingTime", m_notMovingTime);
		ser.Value("m_endAccuracy", m_endAccuracy);
		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("TraceDirective", m_pTraceDirective!=NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		else
		{
			SAFE_DELETE(m_pTraceDirective);
			if(ser.BeginOptionalGroup("TraceDirective", true))
			{
				m_pTraceDirective = new COPTrace(false, m_endAccuracy);
				m_pTraceDirective->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		ser.EndGroup();
	}
}

//====================================================================
// CheckSegmentAgainstAvoidPos
//====================================================================
bool COPDodge::OverlapSegmentAgainstAvoidPos(const Vec3& from, const Vec3& to, float rad, const std::vector<Vec3FloatPair>& avoidPos)
{
	Lineseg	movement(from, to);
	float t;
	for (unsigned i = 0, ni = avoidPos.size(); i < ni; ++i)
	{
		const float radSq = sqr(avoidPos[i].second + rad);
		if (Distance::Point_LinesegSq(avoidPos[i].first, movement, t) < radSq)
			return true;
	}
	return false;
}

//====================================================================
// GetNearestPuppets
//====================================================================
void COPDodge::GetNearestPuppets(CPuppet* pSelf, const Vec3& pos, float radius, std::vector<Vec3FloatPair>& positions)
{
	const float radiusSq = sqr(radius);
	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet* pPuppet= it->GetAIObject();
		if (!pPuppet || pPuppet == pSelf) 
			continue;
		Vec3 delta = pPuppet->GetPhysicsPos() - pos;
		if (delta.GetLengthSquared() < radiusSq)
		{
			const float passRadius = pPuppet->GetParameters().m_fPassRadius;
			positions.push_back(std::make_pair(pPuppet->GetPos(), passRadius));

			if (Distance::Point_Point2DSq(pPuppet->m_Path.GetLastPathPos(), pos) < radiusSq)
				positions.push_back(std::make_pair(pPuppet->m_Path.GetLastPathPos(), passRadius));
		}
	}
}


//====================================================================
// Execute
//====================================================================
EGoalOpResult COPDodge::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPDodge_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!m_pTraceDirective)
	{
		FRAME_PROFILER( "Dodge/CalculatePathTree", gEnv->pSystem, PROFILE_AI);

		IAIObject* pTarget = pPipeUser->GetAttentionTarget();
		CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();
		if (!pTarget && m_useLastOpAsBackup)
			pTarget = pLastOpResult;

		if (!pTarget)
		{
			Reset(pPipeUser);
			return eGOR_FAILED;
		}

		Vec3 center = pPipeUser->GetPhysicsPos();
		m_targetPos = pTarget->GetPos();
		m_targetView = pTarget->GetViewDir();

		Vec3 dir = m_targetPos - center;
		dir.Normalize();
		m_basis.SetRotationVDir(dir);

		IAISystem::tNavCapMask navCap = pPipeUser->m_movementAbility.pathfindingProperties.navCapMask;
		const float passRadius = pPipeUser->GetParameters().m_fPassRadius;

		int nBuildingID;
		IVisArea* pVisArea;
		IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(center, nBuildingID, pVisArea, navCap);

		m_avoidPos.clear();
		GetNearestPuppets(pPipeUser->CastToCPuppet(), pPipeUser->GetPhysicsPos(), m_distance + 2.0f, m_avoidPos);

		// Calc path.
		const int iters = 7;
		Vec3	pos[iters];

		for (int i = 0; i < iters; ++i)
		{
			float a = ((float)i / (float)iters) * gf_PI2;
			float x = cosf(a);
			float y = sinf(a);
			float d = m_distance * (0.9f + ai_frand()*0.2f);

			Vec3 delta = x*m_basis.GetColumn0() + y*m_basis.GetColumn2() + 0.5f*m_basis.GetColumn1();
			delta.Normalize();
			pos[i] = center + delta * d;

			Vec3 hitPos;
			float hitDist;
			if (IntersectSweptSphere(&hitPos, hitDist, Lineseg(center, pos[i]), passRadius*1.1f, AICE_ALL))
				pos[i] = center + delta * hitDist;

			m_DEBUG_testSegments.push_back(center);
			m_DEBUG_testSegments.push_back(pos[i]);
		}

		Lineseg	targetLOS(m_targetPos, m_targetPos + m_targetView * Distance::Point_Point(center, m_targetPos)*2.0f);
		float t;
		Distance::Point_Lineseg(center, targetLOS, t);
		Vec3 nearestPointOnTargetView = targetLOS.GetPoint(t);

		float bestDist = 0.0f;
		int bestIdx = -1;
		for (int i = 0; i < iters; ++i)
		{
			float d = Distance::Point_PointSq(center, pos[i]);
			if (d > bestDist)
			{
				if (!OverlapSegmentAgainstAvoidPos(center, pos[i], passRadius, m_avoidPos))
				{
					bestDist = d;
					bestIdx = i;
				}
			}
		}

		if (bestIdx == -1)
		{
			Reset(pPipeUser);
			return eGOR_FAILED;
		}

		m_bestPos = pos[bestIdx];

		// Set the path
		gAIEnv.pPathfinder->CancelAnyPathsFor(pPipeUser);
		pPipeUser->ClearPath("COPDodge::Execute generate path");

		SNavPathParams params(center, m_bestPos);
		params.precalculatedPath = true;
		pPipeUser->m_Path.SetParams(params);

		pPipeUser->m_nPathDecision = PATHFINDER_PATHFOUND;
		pPipeUser->m_Path.PushBack(PathPointDescriptor(navType, center));
		pPipeUser->m_Path.PushBack(PathPointDescriptor(navType, m_bestPos));

		pPipeUser->m_OrigPath = pPipeUser->m_Path;

		if (CPuppet* pPuppet = pPipeUser->CastToCPuppet())
		{
			// Update adaptive urgency control before the path gets processed further
			pPuppet->AdjustMovementUrgency(pPuppet->m_State.fMovementUrgency,
				pPuppet->m_Path.GetPathLength(pPuppet->IsUsing3DNavigation()));
		}

		m_pTraceDirective = new COPTrace(false, m_endAccuracy);
		m_lastTime = GetAISystem()->GetFrameStartTime();
	}

	AIAssert(m_pTraceDirective);
	EGoalOpResult done = m_pTraceDirective->Execute(pPipeUser);
	if (m_pTraceDirective == NULL)
		return eGOR_IN_PROGRESS;

	AIAssert(m_pTraceDirective);

	// workaround: The following code tries to put a lost or stuck agent back on track.
	// It works together with a piece of in ExecuteDry which tracks the speed relative
	// to the requested speed and if it drops dramatically for certain time, this code
	// will trigger and try to move the agent back on the path. [Mikko]

	float timeout = 1.5f;
	if (pPipeUser->GetType() == AIOBJECT_VEHICLE )
		timeout = 7.0f;

	if (m_notMovingTime > timeout)
	{
		// Stuck or lost, move to the nearest point on path.
		AIWarning("COPDodge::Entity %s has not been moving fast enough for %.1fs it might be stuck, abort.", GetNameSafe(pPipeUser), m_notMovingTime);
		Reset(pPipeUser);
		return eGOR_FAILED;
	}

	if (done != eGOR_IN_PROGRESS)
	{
		Reset(pPipeUser);
		return done;
	}
	if (!m_pTraceDirective)
	{
		Reset(pPipeUser);
		return eGOR_FAILED;
	}

	return eGOR_IN_PROGRESS;
}

//====================================================================
// ExecuteDry
//====================================================================
void COPDodge::ExecuteDry(CPipeUser* pPipeUser) 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CAISystem *pSystem = GetAISystem();

	if (m_pTraceDirective)
	{
		m_pTraceDirective->ExecuteTrace(pPipeUser, false);

		// workaround: The following code together with some logic in the execute tries to keep track
		// if the agent is not moving for some time (is stuck), and pathfinds back to the path. [Mikko]
		CTimeValue	time(pSystem->GetFrameStartTime());
		float	dt((time - m_lastTime).GetSeconds());
		float	speed = pPipeUser->GetVelocity().GetLength();
		float	desiredSpeed = pPipeUser->m_State.fDesiredSpeed;
		if(desiredSpeed > 0.0f)
		{
			float ratio = clamp(speed / desiredSpeed, 0.0f, 1.0f);
			if(ratio < 0.1f)
				m_notMovingTime += dt;
			else
				m_notMovingTime -= dt;
			if(m_notMovingTime < 0.0f)
				m_notMovingTime = 0.0f;
		}
		m_lastTime = time;
	}
}


//===================================================================
// DebugDraw
//===================================================================
void COPDodge::DebugDraw(CPipeUser* pPipeUser) const
{
	if (m_pTraceDirective)
		m_pTraceDirective->DebugDraw(pPipeUser);

	const float passRadius = pPipeUser->GetParameters().m_fPassRadius;

	Vec3 center = pPipeUser->GetPhysicsPos();

	CDebugDrawContext dc;

	for (unsigned i = 0, ni = m_DEBUG_testSegments.size(); i < ni; i += 2)
	{
		dc->DrawLine(m_DEBUG_testSegments[i], ColorB(255, 255, 255), m_DEBUG_testSegments[i + 1], ColorB(255, 255, 255));
		dc->DrawWireSphere(m_DEBUG_testSegments[i + 1], passRadius * 1.1f, ColorB(255, 255, 255));
	}

	dc->DrawSphere(m_bestPos, 0.2f, ColorB(255, 0, 0));

	for (unsigned i = 0, ni = m_avoidPos.size(); i < ni; ++i)
		dc->DrawWireSphere(m_avoidPos[i].first, m_avoidPos[i].second, ColorB(255, 0, 0));
}


//
//-------------------------------------------------------------------------------------------------------------
COPCompanionStick::COPCompanionStick( float fNotReactingDistance, float fForcedMoveDistance, float fLeaderInfluenceRange )
: m_vLastUsedTargetPos(0,0,0),
m_vCurrentTargetPos(0,0,0),
m_fMinEndDistance(0.f),
m_fMaxEndDistance(0.f),
m_pTraceDirective(NULL),
m_pPathfindDirective(NULL),
m_bFirstExec(true),
m_fPathLenLimit(20),
m_fEndAccuracy(0.01f),
m_fMoveWillingness(0.0f),
m_fLeaderInfluenceRange(5.0f),
m_fNotReactingDistance(1.0f),
m_fForcedMoveDistance(2.0f)
{
	m_fSteerDistanceSqr = 100;//fSteerDistance * fSteerDistance;

	if( fNotReactingDistance > FLT_EPSILON )
		m_fNotReactingDistance = fNotReactingDistance;

	if( fForcedMoveDistance > FLT_EPSILON )
		m_fForcedMoveDistance = fForcedMoveDistance;

	if( fLeaderInfluenceRange > FLT_EPSILON )
		m_fLeaderInfluenceRange = fLeaderInfluenceRange;
}

//
//-------------------------------------------------------------------------------------------------------------
COPCompanionStick::COPCompanionStick(const XmlNodeRef& node)
: m_vLastUsedTargetPos(ZERO),
	m_vCurrentTargetPos(ZERO),
	m_fMinEndDistance(0.f),
	m_fMaxEndDistance(0.f),
	m_pTraceDirective(0),
	m_pPathfindDirective(0),
	m_bFirstExec(true),
	m_fPathLenLimit(20),
	m_fSteerDistanceSqr(100.f),
	m_fEndAccuracy(0.01f),
	m_fMoveWillingness(0.f),
	m_fLeaderInfluenceRange(5.f),
	m_fNotReactingDistance(1.f),
	m_fForcedMoveDistance(2.f)
{
	s_xml.GetMandatory(node, "leaderInfluenceRange", m_fLeaderInfluenceRange);
	s_xml.GetMandatory(node, "notReactingDistance", m_fNotReactingDistance);
	s_xml.GetMandatory(node, "forcedMoveDistance", m_fForcedMoveDistance);
}

//
//-------------------------------------------------------------------------------------------------------------
COPCompanionStick::~COPCompanionStick()
{
	SAFE_DELETE( m_pPathfindDirective );
	SAFE_DELETE( m_pTraceDirective );

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPSteer::~COPSteer %p", this);
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCompanionStick::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_AI );

	if( pPipeUser->m_nPathDecision == PATHFINDER_STILLFINDING && m_pPathfindDirective != NULL )
	{
		m_pPathfindDirective->Execute(pPipeUser);
		return eGOR_IN_PROGRESS;
	}

	CAISystem *pAISystem = GetAISystem();
	CPuppet *pPuppet = pPipeUser->CastToCPuppet();

	if( m_bFirstExec == true )
	{
		CGroupMember* pGroupMember = gAIEnv.pGroupSystem->GetGroupMember(pPuppet->GetEntityID());
		if( pGroupMember == NULL )
		{
			pGroupMember = gAIEnv.pGroupSystem->CreateGroupMember(pPuppet->GetEntityID());
		}

		pGroupMember->UseSteering(true, m_fSteerDistanceSqr);

		m_pPosDummy = pGroupMember->GetFormationPosAIObject();
		m_bFirstExec = false;
	}

	if ( m_pPosDummy == NULL ) 
	{
		pPipeUser->SetSignal(0,"OnSteerFailed",pPipeUser->GetEntity(),NULL, 0); 
		AILogAlways("COPSteer::Execute - Error - No point to steer to: %s", GetNameSafe(pPipeUser));

		return eGOR_FAILED;
	}

	if( pPuppet->GetPos().GetSquaredDistance2D( m_pPosDummy->GetPos() ) < 0.3f )
	{
		return eGOR_SUCCEEDED;
	}
	else
	{
		CAIObject *pTarget =  m_pPosDummy;

		bool bForceReturnPartialPath = false;

		//if (pPipeUser->m_nPathDecision != PATHFINDER_PATHFOUND) m_fLastTime = -1.0f;

		// Check for unsuitable agents
		if(pPipeUser->GetSubType() == CAIObject::STP_HELI 
			|| !pPipeUser->m_movementAbility.bUsePathfinder)
		{
			AILogAlways("COPSteer::Execute - Error - Unsuitable agent: %s", GetNameSafe(pPipeUser));
			Reset(pPipeUser); 
			return eGOR_FAILED;
		}

		if(pTarget->GetSubType() == CAIObject::STP_ANIM_TARGET)
		{
			AILogAlways("COPSteer::Execute - Error - Unsuitable target: %s", GetNameSafe(pPipeUser));
			Reset(pPipeUser); 
			return eGOR_FAILED;
		}

		// Move strictly to the target point.
		// MTJ: Not sure this should stay
		pPipeUser->m_nMovementPurpose = 0;

		// Where are we heading eventually?
		Vec3 vAgentPosition(pPipeUser->GetPos());
		float fDistToEndTarget = vAgentPosition.GetDistance(pTarget->GetPos());

		pPipeUser->SetInCover(false);
		pPipeUser->m_CurrentHideObject.Invalidate();

		// If we have a hidespot, that's our target
		// Otherwise it's the attention target
		if (m_pPosDummy) 
			pTarget = m_pPosDummy;

		switch( m_pPathfindDirective ? pPipeUser->m_nPathDecision : PATHFINDER_MAXVALUE)
		{
		case PATHFINDER_MAXVALUE:
			{
				// Generate path to target
				float endTol = (bForceReturnPartialPath || m_fEndAccuracy < 0.0f ? std::numeric_limits<float>::max() : m_fEndAccuracy);
				m_pPathfindDirective = new COPPathFind("", pTarget, 0.0f, 0.0f, true, m_fPathLenLimit);
				pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;
				if (m_pPathfindDirective->Execute(pPipeUser) && pPipeUser->m_nPathDecision == PATHFINDER_NOPATH)
				{
					// If special nopath signal is specified, send the signal.
					pPipeUser->m_State.vMoveDir.Set(0,0,0);
					return eGOR_FAILED;			
				}
				break;
			}
		case PATHFINDER_PATHFOUND:
			{
				// if we have a path, trace it
				if (pPipeUser->m_OrigPath.Empty() || pPipeUser->m_OrigPath.GetPathLength( true ) < 0.01f )
				{
					AILogAlways("COPSteer::Execute - Origpath is empty: %s. Regenerating.", GetNameSafe(pPipeUser));
					/*RegeneratePath( pPipeUser, m_pPosDummy->GetPos() );
					pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;*/
					return eGOR_FAILED;
				}
				else
				{
					float pathsize = pPipeUser->m_OrigPath.GetPathLength(true);

					// If we need a tracer, create one
					if (!m_pTraceDirective)
					{
						// We create another goalop to achieve this (sucky approach)
						bool bExact = true;
						bForceReturnPartialPath = false;

						m_pTraceDirective = new COPTrace(bExact, m_fEndAccuracy, bForceReturnPartialPath);
						m_pTraceDirective->SetControlSpeed( false );
						TPathPoints::const_reference lastPathNode = pPipeUser->m_OrigPath.GetPath().back();
						Vec3 vLastPos = lastPathNode.vPos;
						Vec3 vRequestedLastNodePos = pPipeUser->m_Path.GetParams().end;
						float dist = Distance::Point_Point(vLastPos,vRequestedLastNodePos);

						// MTJ: What is this for?
						if (lastPathNode.navType != IAISystem::NAV_SMARTOBJECT && dist > C_MaxDistanceForPathOffset)// && pPipeUser->m_Path.GetPath().size() == 1 )
						{
							AISignalExtraData* pData = new AISignalExtraData;
							pData->fValue = dist;// - m_fMinEndDistance;
							pPipeUser->SetSignal(0,"OnEndPathOffset",pPipeUser->GetEntity(),pData, gAIEnv.SignalCRCs.m_nOnEndPathOffset);
						}
						else
						{
							pPipeUser->SetSignal(0,"OnPathFound",NULL, 0, gAIEnv.SignalCRCs.m_nOnPathFound);
						}
					}

					// Keep tracing - previous code will stop us when close enough
					// MTJ: What previous code?
					bool bDone = (m_pTraceDirective->Execute(pPipeUser) != eGOR_IN_PROGRESS);

					float fDistToHopTarget = vAgentPosition.GetDistance(pTarget->GetPos());

					if (bDone) 
					{
						pPipeUser->SetSignal(0,"OnFormationPointReached",pPipeUser->GetEntity(),NULL, 0); // MTJ: Provide CRC
						return eGOR_SUCCEEDED;
					}
				}

				break;
			}
		case PATHFINDER_NOPATH:
			{
				pPipeUser->SetSignal(0,"OnGetToFormationPointFailed",pPipeUser->GetEntity(),NULL, 0); // MTJ: Provide CRC
				return eGOR_FAILED;
			}
		default:
			{
				// MTJ: Still pathfinding I guess?
				m_pPathfindDirective->Execute(pPipeUser);
				break;
			}
		}
	}

	// Run me again next tick
	return eGOR_IN_PROGRESS;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPCompanionStick::ExecuteDry(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	if( pPipeUser->m_nPathDecision == PATHFINDER_STILLFINDING && m_pPathfindDirective != NULL )
	{
		m_pPathfindDirective->Execute(pPipeUser);
		return;
	}

	if (m_pTraceDirective && m_pPosDummy)
	{
		bool bTargetDirty = false;

		//if( pPipeUser->GetPos().GetSquaredDistance2D( m_pPosDummy->GetPos() ) < m_fSteerDistanceSqr )
		//{

		//}
		//else
		//{
			Vec3 targetVector;
			if(pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND /*&& m_bForceReturnPartialPath*/)
				targetVector = (pPipeUser->m_Path.GetLastPathPos() - pPipeUser->GetPhysicsPos());
			else
				targetVector = (m_pPosDummy->GetPhysicsPos() - pPipeUser->GetPhysicsPos());

			{
				Vec3 targetPos = m_pPosDummy->GetPos();
				float targetMoveDist = (m_vLastUsedTargetPos - targetPos).GetLength2D();

				// Use the stored destination point instead of the last path node since the path may be cut because of navSO.
				Vec3 pathEnd = pPipeUser->m_PathDestinationPos;
				Vec3	dir(pathEnd - pPipeUser->GetPos());

				dir.z = 0;
				dir.NormalizeSafe();

				Vec3	dirToTarget(targetPos - pathEnd);

				dirToTarget.z = 0;
				dirToTarget.NormalizeSafe();

				float	regenDist = 0.3f;
				if( dirToTarget.Dot( dir ) < cosf(DEG2RAD(8.0f)) )
					regenDist *= 5.0f;

				if( targetMoveDist > regenDist )
					bTargetDirty = true;

				// when near the path end force more frequent updates
				//float pathDistLeft = pPipeUser->m_Path.GetPathLength(false);
				//float pathEndError = (pathEnd - targetPos).GetLength2D();

				//// TODO/workaround! [Mikko] This prevent the path to regenerated every frame in some special cases in Crysis Core level
				//// where quite a few behaviors are sticking to a long distance (7-10m).
				//// The whole stick regeneration logic is flawed mostly because pPipeUser->m_PathDestinationPos is not always
				//// the actual target position. The pathfinder may adjust the end location and not keep the requested end pos
				//// if the target is not reachable. I'm sure there are other really nasty cases about this path invalidation logic too.
				//const GraphNode* pLastNode = gAIEnv.pGraph->GetNode(pPipeUser->m_lastNavNodeIndex);
				//if (pLastNode && pLastNode->navType == IAISystem::NAV_VOLUME)
				//	pathEndError = max(0.0f, pathEndError - pathDistLeft/*GetEndDistance(pPipeUser)*/);

				//if (pathEndError > 0.1f && pathDistLeft < 2.0f * pathEndError)
				//{
				//	//bTargetDirty = true;
				//}
			}

			if(!pPipeUser->m_movementAbility.b3DMove)
				targetVector.z = 0.0f;
			float curDist = targetVector.GetLength();

			const Vec3& vDummyPos = m_pPosDummy->GetPos();
			m_vCurrentTargetPos = vDummyPos;

			if( /*pPipeUser->m_nPathDecision != PATHFINDER_STILLFINDING && */bTargetDirty )
				RegeneratePath(pPipeUser, vDummyPos);
			else
				m_pTraceDirective->ExecuteTrace(pPipeUser, false);

			
		AdjustSpeed( pPipeUser );
		
	}
}

//
//-------------------------------------------------------------------------------------------------------------
void COPCompanionStick::Reset(CPipeUser* pPipeUser)
{
	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPSteer::Reset %s", pPipeUser ? GetNameSafe(pPipeUser) : "");

	SAFE_DELETE( m_pPathfindDirective );
	SAFE_DELETE( m_pTraceDirective );

	m_bNeedHidespot = true;
	m_vLastUsedTargetPos.zero();
	m_vCurrentTargetPos.zero();

	if (pPipeUser)
	{
		pPipeUser->ClearPath("COPSteer::Reset m_Path");
	}
}

//
//-------------------------------------------------------------------------------------------------------------
void COPCompanionStick::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{

}

//
//-------------------------------------------------------------------------------------------------------------
void COPCompanionStick::DebugDraw(CPipeUser* pPipeUser) const
{
	CPuppet *pPuppet = pPipeUser->CastToCPuppet();

	float normalSpeed, minSpeed, maxSpeed;
	pPuppet->GetMovementSpeedRange(pPuppet->m_State.fMovementUrgency, pPuppet->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

	float fMaxSpeed = 6.0f;//maxSpeed;

	// if companion out of camera increase its max speed
	CCamera &cam = GetISystem()->GetViewCamera();
	Sphere s( pPuppet->GetPos(), pPuppet->GetRadius() * 2.0f );
	if( cam.IsSphereVisible_FH( s ) == CULL_EXCLUSION )
	{
		fMaxSpeed *= 2.0f;
	}

	//float fDesiredSpeed = 3.0f;
	float fDistanceToLeaderRatio = 1.0f; // 1 - means far, 0 - means very close
	float fLeaderSpeed = 0.0f;
	float fDistanceToTarget = 0.0f;
	Vec3 vLabelPos = m_vLastUsedTargetPos;

	// match speed to group leader
	CGroupMember*		pMember = gAIEnv.pGroupSystem->GetGroupMember( pPipeUser->CastToCAIActor()->GetEntityID() );
	const CGroupMember*		pLeader = pMember->GetLeader();
	if( pLeader != NULL )
	{
		const CAIActor* pActor = pLeader->GetActor();
		fLeaderSpeed = pActor->GetVelocity().GetLength2D();
		float fDistToLeader = pActor->GetPos().GetDistance( pPuppet->GetPos() );

		fDistanceToLeaderRatio = fDistToLeader/5;
		if( fDistanceToLeaderRatio > 1.0f )
			fDistanceToLeaderRatio = 1.0f;

		vLabelPos = pMember->GetFormationPos();
	}

	// tune speed depending on target distance
	{
		fDistanceToTarget = pPuppet->GetPos().GetDistance( m_vLastUsedTargetPos );
		fDistanceToLeaderRatio = fDistanceToTarget/10;
		if( fDistanceToLeaderRatio > 1.0f )
			fDistanceToLeaderRatio = 1.0f;
		//fMaxSpeed *= fDistToTargetRation;
	}

	CDebugDrawContext dc;

	dc->Draw3dLabel( vLabelPos, 1.0f, "\nSpeed %.2f\nDesired Speed %.2f\nMaxSpeed %.f\ndestDist %.2f\nLeader speed %.2f\n\nWillingness %.3f"
		, pPuppet->CastToCAIActor()->GetVelocity().GetLength()
		, pPuppet->m_State.fDesiredSpeed
		, fMaxSpeed
		, fDistanceToTarget
		, fLeaderSpeed
		, m_fMoveWillingness
		);

	dc->DrawSphere(m_vLastUsedTargetPos, 0.55f, ColorB(0, 128, 255, 150));
}

//
//----------------------------------------------------------------------------------------------------------
void COPCompanionStick::RegeneratePath(CPipeUser* pPipeUser, const Vec3 &destination)
{
	if (!pPipeUser)
		return;

	CTimeValue time = GetAISystem()->GetFrameStartTime();

	if( (time - m_fLastRegenTime).GetMilliSeconds() < 100.f )
		return;

	m_fLastRegenTime = time;

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("COPCompanionStick::RegeneratePath %s", pPipeUser ? GetNameSafe(pPipeUser) : "");
	m_pPathfindDirective->Reset(pPipeUser);
	if( m_pTraceDirective )
		m_pTraceDirective->m_fEndAccuracy = m_fEndAccuracy;
	m_vLastUsedTargetPos = destination;
	pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;

	const Vec3 opPos = pPipeUser->GetPhysicsPos();

	// Check for direct connection if distance is less than fixed value
	if ( opPos.GetSquaredDistance( destination ) < 2500.f )
	{		
		int nbid;
		IVisArea *iva;
		IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(opPos, nbid, iva, pPipeUser->m_movementAbility.pathfindingProperties.navCapMask);
		CNavRegion *pRegion = gAIEnv.pNavigation->GetNavRegion(navType, gAIEnv.pGraph);
		if (pRegion)
		{
			Vec3	from = opPos;
			Vec3	to = destination;

			NavigationBlockers	navBlocker;
			if(pRegion->CheckPassability(from, to, pPipeUser->GetParameters().m_fPassRadius, navBlocker, pPipeUser->GetMovementAbility().pathfindingProperties.navCapMask))
			{
				pPipeUser->ClearPath("COPCompanionStick::RegeneratePath m_Path");

				if (navType == IAISystem::NAV_TRIANGULAR)
				{
					// Make sure not to enter forbidden area.
					if (gAIEnv.pNavigation->IsPathForbidden(opPos, destination))
						return;
					if (gAIEnv.pNavigation->IsPointForbidden(destination, pPipeUser->GetParameters().m_fPassRadius))
						return;
				}

				PathPointDescriptor	pt;
				pt.navType = navType;

				pt.vPos = from;
				pPipeUser->m_Path.PushBack(pt);

				float endDistance = 0.5f;//m_fStickDistance;
				if (fabsf(endDistance) > 0.0001f)
				{
					// Handle end distance.
					float dist;
					if (pPipeUser->IsUsing3DNavigation())
						dist = Distance::Point_Point(from, to);
					else
						dist = Distance::Point_Point2D(from, to);

					float d;
					if (endDistance > 0.0f)
						d = dist - endDistance;
					else
						d = -endDistance;
					float u = clamp(d / dist, 0.0001f, 1.0f);

					pt.vPos = from + u * (to - from);

					pPipeUser->m_Path.PushBack(pt);
				}
				else
				{
					pt.vPos = to;
					pPipeUser->m_Path.PushBack(pt);
				}

				pPipeUser->m_Path.SetParams(SNavPathParams(from, to, Vec3(ZERO), Vec3(ZERO), -1, false, endDistance, true));

				pPipeUser->m_OrigPath = pPipeUser->m_Path;
				pPipeUser->m_nPathDecision = PATHFINDER_PATHFOUND;
			}
		}
	}
}

//
//----------------------------------------------------------------------------------------------------------
void COPCompanionStick::AdjustSpeed( CPipeUser* pPipeUser ) 
{
	CPuppet *pPuppet = pPipeUser->CastToCPuppet();

	float normalSpeed, minSpeed, maxSpeed;
	pPuppet->GetMovementSpeedRange(pPuppet->m_State.fMovementUrgency, pPuppet->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

	float fMaxSpeed = 6.0f;//maxSpeed;

	// if companion out of camera increase its max speed
	CCamera &cam = GetISystem()->GetViewCamera();
	Sphere s( pPuppet->GetPos(), pPuppet->GetRadius() * 2.0f );
	if( cam.IsSphereVisible_FH( s ) == CULL_EXCLUSION )
	{
		fMaxSpeed *= 2.0f;
	}

	//float fDesiredSpeed = 3.0f;
	float fLeaderInfluence = 1.0f; // 1 - means far, 0 - means very close
	float fLeaderSpeed = 0.0f;
	float fDistanceToTarget = 0.0f;
	
	// match speed to group leader
	CGroupMember*		pMember = gAIEnv.pGroupSystem->GetGroupMember( pPipeUser->CastToCAIActor()->GetEntityID() );
	const CGroupMember*		pLeader = pMember->GetLeader();
	if( pLeader != NULL )
	{
		const CAIActor* pActor = pLeader->GetActor();
		fLeaderSpeed = pActor->GetVelocity().GetLength2D();		
	}

	// tune speed depending on target distance
	fDistanceToTarget = pPuppet->GetPos().GetDistance( m_vCurrentTargetPos );
	fLeaderInfluence = fDistanceToTarget / m_fLeaderInfluenceRange;
	if( fLeaderInfluence > 1.0f )
		fLeaderInfluence = 1.0f;

	// determine puppet's willingness to move:
	
	// if pipe user stopped close to his final destination (not the point that was used
	// to generate the path - m_vLastUsedTargetPos, which is important for moving targets)
	// than zero its willingness to move
	if( pPuppet->m_State.fDesiredSpeed < 0.01f && fDistanceToTarget < m_fNotReactingDistance )
	{
		m_fMoveWillingness = 0.0f;
	}
	// if pipe user is already moving then its willingness to move is set to 1
	else if( pPuppet->m_State.fDesiredSpeed > 0.0f )
	{
		m_fMoveWillingness = 1.0f;
	}
	else
	{
		// if destination is more than 2 meters away, than be willing to move
		if( fDistanceToTarget > m_fForcedMoveDistance )
			m_fMoveWillingness = 1.0f;
		else	// else be more willing with time. 
			m_fMoveWillingness += GetAISystem()->GetFrameDeltaTime() * 0.2f * fDistanceToTarget;
	}

	if( m_fMoveWillingness < 1.0f )
	{
		pPuppet->m_State.fDesiredSpeed = 0.0f;
	}
	else
	{
		if( fDistanceToTarget > m_fLeaderInfluenceRange )	
			pPuppet->m_State.fDesiredSpeed = fMaxSpeed;
		else if( fDistanceToTarget < 3.0f )
			pPuppet->m_State.fDesiredSpeed = max( 1.0f, min( fMaxSpeed, fLeaderSpeed ) );
		else
			pPuppet->m_State.fDesiredSpeed = fMaxSpeed * fLeaderInfluence + fLeaderSpeed * (1.0f - fLeaderInfluence);
	}
}


EGoalOpResult COPDummy::Execute(CPipeUser* pPipeUser)
{
	// This goalop does nothing except for returning AIGOR_DONE
	return eGOR_DONE;
}
