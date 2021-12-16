/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   PipeUser.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2 Mar 2009	: Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#include "StdAfx.h"
#include "CAISystem.h"
#include <ISystem.h>
#include <IConsole.h>
#include <ITimer.h>
#include "GoalOp.h"
#include "PipeUser.h"
#include "Puppet.h"
#include "Leader.h"
#include "ObjectTracker.h"
#include "AIActions.h"
#include "PathFollower.h"
#include <ISerialize.h>
#include "GroupSystem/GroupSystem.h"
#include "GroupSystem/GroupMember.h"
#include "Reference.h"
#include "CodeCoverageTracker.h"


CPipeUser::CPipeUser() :
  m_fTimePassed(0)
, m_bInCover(false)
, m_bMovingToCover(false)
, m_pPathFollower(0)
, m_nMovementPurpose( 0 )
, m_vLastMoveDir(ZERO)
, m_bKeepMoving(false)
, m_bFirstUpdate(true)
, m_bBlocked(false)
, m_pCurrentGoalPipe(NULL)
, m_fireMode(FIREMODE_OFF)
, m_fireModeUpdated(false)
, m_bLooseAttention( false )
, m_IsSteering(false)
, m_DEBUGmovementReason(AIMORE_UNKNOWN)
, m_AttTargetPersistenceTimeout(0.0f)
, m_AttTargetThreat(AITHREAT_NONE)
, m_AttTargetExposureThreat(AITHREAT_NONE)
, m_AttTargetType(AITARGET_NONE)
, m_bPathToFollowIsSpline( false )
, m_lastLiveTargetPos(ZERO)
, m_timeSinceLastLiveTarget(-1.0f)
, m_refShape(0)
, m_looseAttentionId(0)
, m_aimState(AI_AIM_NONE)
, m_pActorTargetRequest(0)
, m_DEBUGUseTargetPointRequest(ZERO)
, m_PathDestinationPos(ZERO)
, m_navSOEarlyPathRegen(false)
, m_actorTargetReqId(1)
, m_spreadFireTime(0.0f)
, m_lastExecutedGoalop(eGO_LAST)
, m_paused(0)
{
	CCCPOINT(CPipeUser_CPipeUser);

	_fastcast_CPipeUser = true;

	// (MATT) SetName could be called to ensure we're consistent, rather than the code above {2009/02/03}

	m_CurrentHideObject.m_HideSmartObject.pChainedUserEvent = NULL;
	m_CurrentHideObject.m_HideSmartObject.pChainedObjectEvent = NULL;
}

static bool bNotifyListenersLock = false;

CPipeUser::~CPipeUser(void)
{
	CCCPOINT(CPipeUser_Destructor);

	// (MATT) Can't a call to Reset be included here? {2009/02/04}

	// Don't call NotifyListeners. m_pCurrentPipe has already been set to NULL!
	//NotifyListeners(m_pCurrentGoalPipe, ePN_OwnerRemoved, true);

	m_coverUsageInfoState.Reset();

	while ( !m_mapGoalPipeListeners.empty() )
		UnRegisterGoalPipeListener( m_mapGoalPipeListeners.begin()->second.first, m_mapGoalPipeListeners.begin()->first );

	// Delete special AI objects, and reset array.
	for(unsigned i = 0; i < COUNT_AISPECIAL; ++i)
		m_refSpecialObjects[i].Release();

	SAFE_DELETE(m_pPathFollower);
	SAFE_DELETE(m_pActorTargetRequest);
}

void CPipeUser::CreateRefPoint()
{
	if (m_refRefPoint.IsSet())
		return;
	GetAISystem()->CreateDummyObject(m_refRefPoint, string(GetName()) + "_RefPoint");
	CAIObject *pRefPoint = m_refRefPoint.GetAIObject();
	assert(pRefPoint);

	if (pRefPoint)
	{
		pRefPoint->SetPos(GetPos(), GetMoveDir());
		pRefPoint->SetSubType(STP_REFPOINT);
	}
}

void CPipeUser::CreateLookAtTarget()
{
	if (m_refLookAtTarget.IsSet())
		return;
	GetAISystem()->CreateDummyObject(m_refLookAtTarget, string(GetName()) + "_LookAtTarget");
	CAIObject *pLookAtTarget = m_refLookAtTarget.GetAIObject();
	pLookAtTarget->SetPos(GetPos()+GetMoveDir());
	pLookAtTarget->SetSubType(STP_LOOKAT);
}


//
//
void CPipeUser::Reset(EObjectResetType type)
{
	CCCPOINT(CPipeUser_Reset);

	CAIActor::Reset(type);
	SetAttentionTarget(NILREF);
	m_PathDestinationPos.zero();
	m_refPathFindTarget.Reset();
	m_IsSteering = false;
	m_fireMode = FIREMODE_OFF;
	SetFireTarget(NILREF);
	m_fireModeUpdated = false;
	m_outOfAmmoSent = false;
	m_wasReloading = false;
	m_actorTargetReqId = 1;
	ResetLookAt();
	ResetBodyTargetDir();

	m_coverUsageInfoState.Reset();

	m_aimState = AI_AIM_NONE;

	m_posLookAtSmartObject.zero();
	
	m_eNavSOMethod = nSOmNone;
	m_navSOEarlyPathRegen = false;
	m_pendingNavSOStates.Clear();
	m_currentNavSOStates.Clear();

	m_CurrentNodeNavType = IAISystem::NAV_UNSET;
	m_idLastUsedSmartObject = 0;
	ClearInvalidatedSOLinks();

	m_CurrentHideObject.m_HideSmartObject.Clear();
	m_forcePathGenerationFrom.zero();
	m_bFirstUpdate = true;

	m_lastLiveTargetPos.Set(0,0,0);
	m_timeSinceLastLiveTarget = -1.0f;
	m_spreadFireTime = 0.0f;

	m_recentUnreachableHideObjects.clear();

	m_bPathToFollowIsSpline = false;

	m_refShapeName.clear();
	m_refShape = 0;

	m_paused = 0;

	SAFE_DELETE(m_pActorTargetRequest);

	// Delete special AI objects, and reset array.
	for(unsigned i = 0; i < COUNT_AISPECIAL; ++i)
		m_refSpecialObjects[i].Release();

	m_notAllowedSubpipes.clear();
	SelectPipe( 0, "_first_", NILREF, 0, true );
	m_Path.Clear("CPipeUser::Reset m_Path");
	m_DEBUGCanTargetPointBeReached.clear();
	m_DEBUGUseTargetPointRequest.zero();
}

void CPipeUser::SetName(const char *pName)
{
	CCCPOINT(CPipeUser_SetName);

	CAIObject::SetName(pName);
	char name[256];

	CAIObject *pRefPoint = m_refRefPoint.GetAIObject();
	if(pRefPoint)
	{
		sprintf(name,"%s_RefPoint",pName);
		pRefPoint->SetName(name);
	}

	CAIObject *pLookAtTarget = m_refLookAtTarget.GetAIObject();
	if(pLookAtTarget)
	{
		sprintf(name,"%s_LookAtTarget",pName);
		pLookAtTarget->SetName(name);
	}
}

//
//---------------------------------------------------------------------------------------------------------
bool CPipeUser::ProcessBranchGoal(QGoal &Goal,bool &bSkipAdd)
{
	if (Goal.op != eGO_BRANCH)
		return false;

	bSkipAdd = Goal.bBlocking;

	bool bNot = (Goal.params.nValue & NOT) != 0;

	if (GetBranchCondition(Goal) ^ bNot)
	{
		// string labels are already converted to integer relative offsets
		// TODO: cut the string version of Jump in a few weeks
		if (Goal.params.str.empty())
			bSkipAdd |= m_pCurrentGoalPipe->Jump(Goal.params.nValueAux);
		else
			bSkipAdd |= m_pCurrentGoalPipe->Jump(Goal.params.str);
	}

	return true;
}

//
//---------------------------------------------------------------------------------------------------------
bool CPipeUser::GetBranchCondition(QGoal &Goal)
{
	int branchType = Goal.params.nValue & (~NOT);

	// Fetch attention target, because it's used in many cases
	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();

	switch(branchType)
	{
	case IF_RANDOM:
		{
			if (ai_frand() < Goal.params.fValue)
				return true;
		}
		break;
	case IF_NO_PATH:
		{
			if (m_nPathDecision == PATHFINDER_NOPATH)
				return true;
		}
		break;
	case IF_PATH_STILL_FINDING:
		{
			if (m_nPathDecision == PATHFINDER_STILLFINDING)
				return true;
		}
		break;
	case IF_IS_HIDDEN:	// branch if already at hide spot 
		{
			if (!m_CurrentHideObject.IsValid())
				return false;
			Vec3 diff(m_CurrentHideObject.GetLastHidePos() - GetPos());
			diff.z=0.f;
			if (diff.len2() < Goal.params.fValue*Goal.params.fValue)
				return true;
		}
		break;
	case IF_CAN_HIDE:	// branch if hide spot was found
		{
			if (m_CurrentHideObject.IsValid())
				return true;
		}
		break;
	case IF_CANNOT_HIDE:
		{
			if (!m_CurrentHideObject.IsValid())
				return true;
		}
		break;
	case IF_STANCE_IS:	// branch if stance is equal to params.fValue
		{
			if (m_State.bodystate == Goal.params.fValue)
				return true;
		}
		break;
	case IF_HAS_FIRED:	// jumps if the PipeUser just fired - fire flag passed to actor
		{
			if (m_State.fire)
				return true;
		}
		break;
	case IF_FIRE_IS:	// branch if last "firecmd" argument was equal to params.fValue
		{
			bool state = Goal.params.fValue > 0.5f;
			if (AllowedToFire() == state)
				return true;
		}
		break;
	case IF_NO_LASTOP:
		{
			if (m_refLastOpResult.IsNil())
				return true;
		}
		break;
	case IF_SEES_LASTOP:
		{
			CAIObject *pLastOpResult = m_refLastOpResult.GetAIObject();
			if (pLastOpResult && GetAISystem()->CheckObjectsVisibility(this, pLastOpResult, Goal.params.fValue))
				return true;
		}
		break;
	case IF_SEES_TARGET:
		{
			if (Goal.params.fValueAux >= 0.0f)
			{
				if (pAttentionTarget)
				{
					CCCPOINT(CPipeUser_GetBranchCondition_IF_SEES_TARGET);
					SAIBodyInfo bi;
					if (GetProxy() && GetProxy()->QueryBodyInfo(SAIBodyInfoQuery((EStance)(int)Goal.params.fValueAux, 0.0f, 0.0f, false), bi))
					{
						const Vec3& pos = bi.vEyePos;
						Vec3 dir = pAttentionTarget->GetPos() - pos;
						if (dir.GetLengthSquared() > sqr(Goal.params.fValue))
							dir.SetLength(Goal.params.fValue);
						if (!gAIEnv.pRayCaster->Cast(RayCastRequest(pos, dir, COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER)))
							return true;
					}
				}
			}
			{
				if (pAttentionTarget && GetAISystem()->CheckObjectsVisibility(this, pAttentionTarget, Goal.params.fValue))
					return true;
			}
		}
		break;
	case IF_TARGET_LOST_TIME_MORE:
		{
			CPuppet* pPuppet = CastToCPuppet();
			if (pPuppet && pPuppet->m_targetLostTime > Goal.params.fValue)
				return true;
		}
		break;

	case IF_TARGET_LOST_TIME_LESS:
		{
			CPuppet* pPuppet = CastToCPuppet();
			if (pPuppet && pPuppet->m_targetLostTime <= Goal.params.fValue)
				return true;
		}
		break;
	case IF_EXPOSED_TO_TARGET:
		{
			if (pAttentionTarget)
			{
				CCCPOINT(CPipeUser_GetBranchCondition_IF_EXPOSED_TO_TARGET);
				Vec3	pos = GetPos();
				SAIBodyInfo bi;
				if (GetProxy() && GetProxy()->QueryBodyInfo(SAIBodyInfoQuery(STANCE_CROUCH, 0.0f, 0.0f, false), bi))
					pos = bi.vEyePos;

				Vec3	dir(pAttentionTarget->GetPos() - pos);
				dir.SetLength(Goal.params.fValue);
				float	dist;
				bool exposed = !IntersectSweptSphere(0, dist, Lineseg(pos, pos + dir), Goal.params.fValueAux, AICE_ALL);
				if (exposed)
					return true;
			}
		}
		break;
	case IF_CAN_SHOOT_TARGET_PRONED:
		{
			CCCPOINT(CPipeUser_GetbranchCondition_IF_CAN_SHOOT_TARGET_PRONED);

			CPuppet* pPuppet = CastToCPuppet();
			if (pPuppet->CanFireInStance(STANCE_PRONE))
				return true;
		}
		break;
	case IF_CAN_SHOOT_TARGET_CROUCHED:
		{
			CCCPOINT(CPipeUser_GetbranchCondition_IF_CAN_SHOOT_TARGET_CROUCHED);

			CPuppet* pPuppet = CastToCPuppet();
			if (pPuppet->CanFireInStance(STANCE_CROUCH))
				return true;
		}
		break;
	case IF_CAN_SHOOT_TARGET_STANDING:
		{
			CCCPOINT(CPipeUser_GetbranchCondition_IF_CAN_SHOOT_TARGET_STANDING);

			CPuppet* pPuppet = CastToCPuppet();
			if (pPuppet->CanFireInStance(STANCE_STAND))
				return true;
		}
		break;
	case IF_CAN_SHOOT_TARGET:
		{
			Vec3 pos, dir;
			CPuppet* pPuppet = CastToCPuppet();
			bool aimOK = GetAimState()!=AI_AIM_OBSTRUCTED;
			if (pAttentionTarget && pPuppet && pPuppet->GetFirecommandHandler() &&
				aimOK &&
				pPuppet->CanAimWithoutObstruction(pAttentionTarget->GetPos()) &&
				pPuppet->GetFirecommandHandler()->ValidateFireDirection(pAttentionTarget->GetPos() - GetFirePos(), false))
			{
				CCCPOINT(CPipeUser_GetBranchCondition_IF_CAN_SHOOT_TARGET);
				return true;
		}
		}
		break;
	case IF_CAN_MELEE:
		{
			SAIWeaponInfo weaponInfo;
			GetProxy()->QueryWeaponInfo(weaponInfo);
			if (weaponInfo.canMelee)
				return true;
		}
		break;
	case IF_TARGET_DIST_LESS:
	case IF_TARGET_DIST_GREATER:
	case IF_TARGET_IN_RANGE:
	case IF_TARGET_OUT_OF_RANGE:
		{
			CCCPOINT(CPipeUser_GetBranchCondition_IF_TARGET_A);
			if (pAttentionTarget)
			{
				const Vec3 &vPos = GetPos();
				const Vec3 &vTargetPos = pAttentionTarget->GetPos();
				float fDist = Distance::Point_Point(vPos, vTargetPos);
				if (branchType == IF_TARGET_DIST_LESS)
				{
					return (fDist <= Goal.params.fValue);
				}	
				if (branchType == IF_TARGET_DIST_GREATER)
				{
					return (fDist > Goal.params.fValue);
				}
				if (branchType == IF_TARGET_IN_RANGE)
				{
					return (fDist <= m_Parameters.m_fAttackRange);
				}
				if (branchType == IF_TARGET_OUT_OF_RANGE)
				{
					return (fDist > m_Parameters.m_fAttackRange);
				}
			}
		}
		break;
	case IF_TARGET_MOVED_SINCE_START:
	case IF_TARGET_MOVED:
		{
			CGoalPipe *pLastPipe(m_pCurrentGoalPipe->GetLastSubpipe());
			if (!pLastPipe)	// this should NEVER happen
			{
				AIError("CPipeUser::ProcessBranchGoal can get pipe. User: %s",GetName());
				return true;
			}

			if (pAttentionTarget)
			{
				CCCPOINT(CPipeUser_GetBranchCondition_IF_TARGET_MOVED);
				bool ret = Distance::Point_Point(pLastPipe->GetAttTargetPosAtStart(), pAttentionTarget->GetPos()) > Goal.params.fValue;
				if(branchType == IF_TARGET_MOVED)
					pLastPipe->SetAttTargetPosAtStart(pAttentionTarget->GetPos());
				return ret;
			}
		}
		break;
	case IF_NO_ENEMY_TARGET:
		{
			CCCPOINT(CPipeUser_GetBranchCondition_IF_NO_ENEMY_TARGET);
			if (!pAttentionTarget || !IsHostile(pAttentionTarget))
				return true;
		}
		break;
	case IF_PATH_LONGER:	// branch if current path is longer than params.fValue
		{
			float dbgPathLength(m_Path.GetPathLength(false));
			if (dbgPathLength > Goal.params.fValue)
				return true;
		}
		break;
	case IF_PATH_SHORTER:
		{
			float dbgPathLength(m_Path.GetPathLength(false));
			if (dbgPathLength <= Goal.params.fValue)
				return true;
		}
		break;
	case IF_PATH_LONGER_RELATIVE:	// branch if current path is longer than (params.fValue) times the distance to destination
		{
			Vec3 pathDest(m_Path.GetParams().end);
			float pathLength  = m_Path.GetPathLength(false);
			float dist = Distance::Point_Point(GetPos(), pathDest);
			if (pathLength >= dist * Goal.params.fValue)
				return true;
		}
		break;
	case  IF_NAV_WAYPOINT_HUMAN:	// branch if current navigation graph is waypoint
		{
			int nBuilding;
			IVisArea *pArea;
			IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(GetPos(), nBuilding, pArea, m_movementAbility.pathfindingProperties.navCapMask );
			if (navType == IAISystem::NAV_WAYPOINT_HUMAN)
				return true;
		}
		break;
	case  IF_NAV_TRIANGULAR:	// branch if current navigation graph is triangular
		{
			int nBuilding;
			IVisArea *pArea;
			IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(GetPos(), nBuilding, pArea, m_movementAbility.pathfindingProperties.navCapMask );
			if (navType == IAISystem::NAV_TRIANGULAR)
				return true;
		}
	case IF_COVER_COMPROMISED:
	case IF_COVER_NOT_COMPROMISED:	// jumps if the current cover cannot be used for hiding or if the hide spots does not exists.
		{
			CCCPOINT(CPipeUser_GetBranchCondition_IF_COVER_COMPROMISED);
			bool bCompromised = true;
			if (pAttentionTarget)
				bCompromised = m_CurrentHideObject.IsCompromised(this, pAttentionTarget->GetPos());

			bool bResult = bCompromised;
			if (Goal.params.nValue == IF_COVER_NOT_COMPROMISED)
				bResult = !bResult;

			if (bResult)
					return true;
			}
		break;
	case IF_COVER_FIRE_ENABLED:	// branch if cover firemode is  enabled
		{
			CPuppet* pPuppet = CastToCPuppet();
			if (pPuppet)
				return !pPuppet->IsCoverFireEnabled();
		}
		break;
	case IF_COVER_SOFT:
		{
			bool isEmptyCover = m_CurrentHideObject.IsCoverPathComplete() && m_CurrentHideObject.GetCoverWidth(true) < 0.1f;
			bool soft = !m_CurrentHideObject.IsObjectCollidable() || isEmptyCover;
			if (soft)
				return true;
		}
		break;
	case IF_COVER_NOT_SOFT:
		{
			bool isEmptyCover = m_CurrentHideObject.IsCoverPathComplete() && m_CurrentHideObject.GetCoverWidth(true) < 0.1f;
			bool soft = !m_CurrentHideObject.IsObjectCollidable() || isEmptyCover;
			if (soft)
				return true;
		}
		break;

	case IF_LASTOP_FAILED:
		{
			if (m_pCurrentGoalPipe && m_pCurrentGoalPipe->GetLastResult() == eGOR_FAILED)
				return true;
		}
		break;

	case IF_LASTOP_SUCCEED:
		{
			if (m_pCurrentGoalPipe && m_pCurrentGoalPipe->GetLastResult() == eGOR_SUCCEEDED)
				return true;
		}
		break;

	case BRANCH_ALWAYS:	// branches always
		return true;
		break;

	case IF_ACTIVE_GOALS_HIDE:
		if (!m_vActiveGoals.empty() || !m_CurrentHideObject.IsValid())
			return true;
		break;

	default://IF_ACTIVE_GOALS
		if (!m_vActiveGoals.empty())
			return true;
		break;
	}

	return false;
}

bool CPipeUser::IsInCover() const
{
	return m_bInCover;
}

bool CPipeUser::IsMovingToCover()
{
	return m_bMovingToCover;
}

void CPipeUser::SetMovingToCover(bool movingToCover)
{
	if (m_bMovingToCover != movingToCover)
	{
		if (movingToCover)
			SetSignal(1, "OnMovingToCover", 0, 0, gAIEnv.SignalCRCs.m_nOnMovingToCover);

		m_bMovingToCover = movingToCover;
	}
}


void CPipeUser::SetInCover(bool inCover)
{
	if (m_bInCover != inCover)
	{
		if (inCover)
			SetSignal(1, "OnEnterCover", 0, 0, gAIEnv.SignalCRCs.m_nOnEnterCover);
		else
			SetSignal(1, "OnLeaveCover", 0, 0, gAIEnv.SignalCRCs.m_nOnLeaveCover);

		m_bInCover = inCover;
	}
}

void CPipeUser::SetCoverCompromised()
{
	if (m_bInCover || m_bMovingToCover)
	{
		m_bInCover = false;
		SetSignal(1, "OnCoverCompromised", 0, 0, gAIEnv.SignalCRCs.m_nOnCoverCompromised);
		IgnoreCurrentHideObject(5.0f);	// Just so we don't use the same one again!
	}
}

//
//---------------------------------------------------------------------------------------------------------
bool CPipeUser::IsCoverCompromised()
{
	CAIObject * pAttentionTarget = m_refAttentionTarget.GetAIObject();
	if (!pAttentionTarget)
		return false;

  if (m_CurrentHideObject.IsValid())
		return m_CurrentHideObject.IsCompromised(this, pAttentionTarget->GetPos());

  return true;
}

//
//---------------------------------------------------------------------------------------------------------
bool CPipeUser::IsTakingCover(float distanceThreshold)
{
	if (IsInCover())
		return true;

	if ((distanceThreshold > 0.0f) && (IsMovingToCover()))
		return m_State.fDistanceToPathEnd < distanceThreshold;

	return false;
}

//
//-------------------------------------------------------------------------------------------------------------
enum ECoverUsageCheckLocation
{
	LowLeft = 0,
	LowCenter,
	LowRight,
	HighLeft,
	HighCenter,
	HighRight,
};

AsyncState CPipeUser::GetCoverUsageInfo(CoverUsageInfo& coverUsageInfo)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (m_coverUsageInfoState.state == AsyncComplete)
	{
		coverUsageInfo = m_coverUsageInfoState.result;
		m_coverUsageInfoState.state = AsyncReady;

		return AsyncComplete;
	}

	IAIObject* attentionTarget = GetAttentionTarget();
	if (!attentionTarget)
	{
		coverUsageInfo = CoverUsageInfo(false);
		
		return AsyncComplete;
	}
	
	if (m_coverUsageInfoState.state == AsyncReady)
	{
		m_coverUsageInfoState.state = AsyncInProgress;

		CAIHideObject& hideObject = m_CurrentHideObject;

		const Vec3&	target = attentionTarget->GetPos();
		const float offset = GetParameters().m_fPassRadius;

		bool lowCover = hideObject.HasLowCover();
		bool highCover = hideObject.HasHighCover();

		Vec3 checkOrigin[6];
		bool checkResult[6];

		m_coverUsageInfoState.result.lowCompromised = !lowCover;
		m_coverUsageInfoState.result.highCompromised = !highCover;

		float leftEdge;
		float rightEdge;
		float leftUmbra;
		float rightUmbra;

		if (lowCover)
		{
			bool compromised = false;

			hideObject.GetCoverDistances(true, target, compromised, leftEdge, rightEdge, leftUmbra, rightUmbra);
			float left = max(leftEdge, leftUmbra);
			float right = min(rightEdge, rightUmbra);

			left += offset;
			right -= offset;

			Vec3 originOffset(0.0f, 0.0f, 0.7f);

			checkOrigin[LowLeft] = hideObject.GetPointAlongCoverPath(left) + originOffset;
			checkOrigin[LowRight] = hideObject.GetPointAlongCoverPath(right) + originOffset;
			checkOrigin[LowCenter] = 0.5f * (checkOrigin[LowLeft] + checkOrigin[LowRight]);

			checkResult[LowLeft] = hideObject.IsLeftEdgeValid(true);
			checkResult[LowCenter] = true;
			checkResult[LowRight] = hideObject.IsRightEdgeValid(true);

			m_coverUsageInfoState.result.lowCompromised = compromised;
		}
		else
		{
			checkResult[LowLeft] = false;
			checkResult[LowRight] = false;
			checkResult[LowCenter] = false;

			m_coverUsageInfoState.result.lowLeft = false;
			m_coverUsageInfoState.result.lowCenter = false;
			m_coverUsageInfoState.result.lowRight = false;
		}

		if (highCover)
		{
			bool compromised = false;

			hideObject.GetCoverDistances(false, target, compromised, leftEdge, rightEdge, leftUmbra, rightUmbra);
			float left = max(leftEdge, leftUmbra);
			float right = min(rightEdge, rightUmbra);

			left += offset;
			right -= offset;

			Vec3 originOffset(0.0f, 0.0f, 1.5f);

			checkOrigin[HighLeft] = hideObject.GetPointAlongCoverPath(left) + originOffset;
			checkOrigin[HighRight] = hideObject.GetPointAlongCoverPath(right) + originOffset;
			checkOrigin[HighCenter] = 0.5f * (checkOrigin[HighLeft] + checkOrigin[HighRight]);

			checkResult[HighLeft] = hideObject.IsLeftEdgeValid(false);
			checkResult[HighCenter] = true;
			checkResult[HighRight] = hideObject.IsRightEdgeValid(false);

			m_coverUsageInfoState.result.highCompromised = compromised;
		}
		else
		{
			checkResult[HighLeft] = false;
			checkResult[HighCenter] = false;
			checkResult[HighRight] = false;

			m_coverUsageInfoState.result.highLeft = false;
			m_coverUsageInfoState.result.highCenter = false;
			m_coverUsageInfoState.result.highRight = false;
		}

		for (int i = 0; i < 6; ++i)
		{
			if (!checkResult[i])
				continue;

			Vec3 dir = target - checkOrigin[i];
			if (dir.GetLengthSquared2D() > 3.0f*3.0f)
				dir.SetLength(3.0f);

			m_coverUsageInfoState.rayID[i] = gAIEnv.pRayCaster->Queue(
				RayCastRequest::HighPriority, RayCastRequest(checkOrigin[i], dir, COVER_OBJECT_TYPES, HIT_COVER | HIT_SOFT_COVER),
				functor(*this, &CPipeUser::CoverUsageInfoRayComplete));
			++m_coverUsageInfoState.rayCount;
		}
	}

	return m_coverUsageInfoState.state;
}

void CPipeUser::CoverUsageInfoRayComplete(const QueuedRayID& rayID, const RayCastResult& result)
{
	--m_coverUsageInfoState.rayCount;

	for (uint32 i = 0; i < 6; ++i)
	{
		if (m_coverUsageInfoState.rayID[i] != rayID)
			continue;
		
		m_coverUsageInfoState.rayID[i] = 0;

		switch (i)
		{
		case LowLeft:
			m_coverUsageInfoState.result.lowLeft = result.hitCount != 0;
			break;
		case LowCenter:
			m_coverUsageInfoState.result.lowCenter = result.hitCount != 0;
			break;
		case LowRight:
			m_coverUsageInfoState.result.lowRight = result.hitCount != 0;
			break;
		case HighLeft:
			m_coverUsageInfoState.result.highLeft = result.hitCount != 0;
			break;
		case HighCenter:
			m_coverUsageInfoState.result.highCenter = result.hitCount != 0;
			break;
		case HighRight:
			m_coverUsageInfoState.result.highRight = result.hitCount != 0;
			break;
		default:
			assert(0);
			break;
		}
	}

	if (m_coverUsageInfoState.rayCount == 0)
		m_coverUsageInfoState.state = AsyncComplete;
}


//
//---------------------------------------------------------------------------------------------------------
bool CPipeUser::ProcessRandomGoal(QGoal &Goal,bool &bSkipAdd)
{
	if (Goal.op != eGO_RANDOM)
		return false;
	bSkipAdd = true;
	if (Goal.params.fValue > ai_rand() % 100)	// 0 - never jump // 100 - always jump
		m_pCurrentGoalPipe->Jump(Goal.params.nValue);
	return true;
}

//
//---------------------------------------------------------------------------------------------------------
bool CPipeUser::ProcessClearGoal(QGoal &Goal,bool &bSkipAdd)
{
	if (Goal.op != eGO_CLEAR)
		return false;
	ClearActiveGoals();

	//Luciano - CLeader should manage the release of formation 
	//					GetAISystem()->FreeFormationPoint(m_Parameters.m_nGroup,this);
	if (Goal.params.fValue)
	{
		SetAttentionTarget(NILREF);
		ClearPotentialTargets();
	}
	else
		bSkipAdd = true;
	return true;
}

void CPipeUser::SetNavSOFailureStates()
{
	if ( m_eNavSOMethod != nSOmNone )
	{
		if ( !m_currentNavSOStates.IsEmpty() )
		{
			gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), m_currentNavSOStates.sAnimationFailUserStates );
			IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity( m_currentNavSOStates.objectEntId );
			if ( pObjectEntity )
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObjectEntity, m_currentNavSOStates.sAnimationFailObjectStates );
			m_currentNavSOStates.Clear();
		}

		if ( !m_pendingNavSOStates.IsEmpty() )
		{
			gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), m_pendingNavSOStates.sAnimationFailUserStates );
			IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity( m_pendingNavSOStates.objectEntId );
			if ( pObjectEntity )
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObjectEntity, m_pendingNavSOStates.sAnimationFailObjectStates );
			m_pendingNavSOStates.Clear();
		}
	}

	AIAssert(m_pendingNavSOStates.IsEmpty() && m_currentNavSOStates.IsEmpty());

	// Stop exact positioning.
	m_State.actorTargetReq.Reset();

	m_eNavSOMethod = nSOmNone;
	m_navSOEarlyPathRegen = false;
	m_forcePathGenerationFrom.zero();
}

void CPipeUser::ClearPath( const char* dbgString )
{
	SetNavSOFailureStates();

  // Don't change the pathfinder result since it has to reflect the result of
  // the last pathfind. If it is set to NoPath then flowgraph AIGoto nodes will
  // fail at the end of the path.
	m_Path.Clear( dbgString );
	m_OrigPath.Clear( dbgString );
	m_refPathFindTarget.Reset();
}

//
//--------------------------------------------------------------------------------------------------------
void CPipeUser::GetStateFromActiveGoals(SOBJECTSTATE &state)
{
	// Reset the movement reason at each update, it will be setup by the correct goal operation.
	m_DEBUGmovementReason = AIMORE_UNKNOWN;

	FUNCTION_PROFILER(gEnv->pSystem,PROFILE_AI);

	if (m_bFirstUpdate)
	{
		m_bFirstUpdate = false;
		SetLooseAttentionTarget(NILREF);
	}

	if (IsPaused())
		return;

	// (MATT) Track whether we will add this to the active goals, executed in subsequent frames {2009/10/14}
	bool bSkipAdd = false;
	if (m_pCurrentGoalPipe)
	{
		if (m_pCurrentGoalPipe->CountSubpipes() >= 10)
		{
			AIWarning("%s has too many (%d) subpipes. Pipe <%s>", GetName(), m_pCurrentGoalPipe->CountSubpipes(),
											m_pCurrentGoalPipe->GetName());
		}

		if (!m_bBlocked)	// if goal queue not blocked
		{
			QGoal Goal;

			EPopGoalResult pgResult;
			while ((pgResult = m_pCurrentGoalPipe->PopGoal(Goal,this)) == ePGR_Succeed)
			{
				// Each Process instruction first checks to see if this goal is of the right type, immediately returning false if not
				if(ProcessBranchGoal(Goal, bSkipAdd))
				{
					// Now, having either taken the branch or not:
					if (bSkipAdd)
						break; // blocking - don't execute the next instruction until next frame
					continue; // non-blocking - continue with execution immediately
				}
				if(ProcessRandomGoal(Goal, bSkipAdd))  // RandomJump would be a better name
					break;
				if(ProcessClearGoal(Goal, bSkipAdd))
					break;
				EGoalOpResult result = Goal.pGoalOp->Execute(this);
				if (result == eGOR_IN_PROGRESS)
					break;
				else
				{
					if (result != eGOR_DONE)
						m_pCurrentGoalPipe->SetLastResult(result);
				}
			}

			if (pgResult != ePGR_BreakLoop)
			{
				if (!Goal.pGoalOp)
				{
					if(m_pCurrentGoalPipe->IsLoop())
					{
						m_pCurrentGoalPipe->Reset();
						ClearActiveGoals();
						if(GetAttentionTarget())
							m_pCurrentGoalPipe->SetAttTargetPosAtStart(GetAttentionTarget()->GetPos());
					}
				}
				else
				{
					if (!bSkipAdd)
					{
						//FIXME
						//todo: remove this, currently happens coz "lookat" never finishes - actor can't rotate sometimes
						if(m_vActiveGoals.size()<110)
						{
							m_vActiveGoals.push_back(Goal);
						}
						else
						{
							// (MATT) Test if the lookat problem still occurs {2009/10/20}
							assert(false);
						}
						m_bBlocked = Goal.bBlocking;
					}
				}
			}
		}
	}
	if (m_vActiveGoals.size() >= 10)
	{
		const QGoal& Goal = m_vActiveGoals.back();
		AIWarning("%s has too many (%d) active goals. Pipe <%s>; last goal <%s>", GetName(), m_vActiveGoals.size(),
			m_pCurrentGoalPipe ? m_pCurrentGoalPipe->GetName() : "_no_pipe_",
			Goal.op == eGO_LAST ? Goal.sPipeName.c_str() : m_pCurrentGoalPipe->GetGoalOpName(Goal.op));
		assert(m_vActiveGoals.size() < 100);
	}

	if (!m_vActiveGoals.empty())
	{
		for (size_t i = 0; i < m_vActiveGoals.size(); i++)
		{
			QGoal& Goal = m_vActiveGoals[i];

			m_lastExecutedGoalop = Goal.op;

			/*
			ITimer *pTimer = gEnv->pTimer;
			int val = gAIEnv.CVars.ProfileGoals;

			CTimeValue timeLast;

			if (val)
				timeLast = pTimer->GetAsyncTime();
*/
			EGoalOpResult result = Goal.pGoalOp->Execute(this);
			/*if (val)
			{
				CTimeValue timeCurr = pTimer->GetAsyncTime();
				float f = (timeCurr-timeLast).GetSeconds();
				timeLast=timeCurr;

				const char* goalName = Goal.op == eGO_LAST ? Goal.sPipeName.c_str() : m_pCurrentGoalPipe->GetGoalOpName(Goal.op);
				GetAISystem()->m_mapDEBUGTimingGOALS[CONST_TEMP_STRING(goalName)] = f;
			}
			*/
			if (result != eGOR_IN_PROGRESS)
			{
				if (Goal.bBlocking) 
				{
					if (result != eGOR_DONE)
						m_pCurrentGoalPipe->SetLastResult(result);
					m_bBlocked = false;
				}

				RemoveActiveGoal(i);
				if (!m_vActiveGoals.empty())
					--i;
			}
		}
	}
	m_Path.GetPath( m_State.remainingPath );

	if (m_bKeepMoving && m_State.vMoveDir.IsZero(.01f) && !m_vLastMoveDir.IsZero(.01f))
		m_State.vMoveDir = m_vLastMoveDir;
	else if (!m_State.vMoveDir.IsZero(.01f))
		m_vLastMoveDir = m_State.vMoveDir;
}

void CPipeUser::SetLastOpResult(CWeakRef<CAIObject> refObject)
{
	m_refLastOpResult = refObject;
}


void CPipeUser::SetAttentionTarget(CWeakRef<CAIObject> refTarget)
{
	CCCPOINT(CPipeUser_SetAttentionTarget);
	CAIObject *pTarget = refTarget.GetAIObject();

	if (!pTarget)
	{
		m_AttTargetThreat = AITHREAT_NONE;
		m_AttTargetExposureThreat = AITHREAT_NONE;
		m_AttTargetType = AITARGET_NONE;
	}
	else if (m_refAttentionTarget!=refTarget && m_pCurrentGoalPipe)
	{
		AIAssert(m_pCurrentGoalPipe);
		CGoalPipe *pLastPipe(m_pCurrentGoalPipe->GetLastSubpipe());
		if(pLastPipe && pTarget)
			pLastPipe->SetAttTargetPosAtStart(pTarget->GetPos());
	}

	m_refAttentionTarget = refTarget;

	RecorderEventData recorderEventData(pTarget!=NULL?pTarget->GetName():"<none>");
	RecordEvent(IAIRecordable::E_ATTENTIONTARGET, &recorderEventData);

	// TO DO: move this call to AddToVisibleList when Leader will be able to read all the visual/sound events from the puppet
	// Note: Usually the group should exists, but the CPipeUser:Reset() is also called in the pipeuser constructor,
	// which later calls this method.
	CAIGroup* pGroup = GetAISystem()->GetAIGroup(GetGroupId());
	if(pGroup)
		pGroup->OnUnitAttentionTargetChanged();
}

//====================================================================
// RequestPathTo
//====================================================================
void CPipeUser::RequestPathTo(
	const Vec3 &pos
	,const Vec3 &dir
	,bool allowDangerousDestination
	,int forceTargetBuildingId
	,float endTol
	,float endDistance
	,CAIObject* pTargetObject)
{
	CCCPOINT(CPipeUser_RequestPathTo);

	m_OrigPath.Clear("CPipeUser::RequestPathTo m_OrigPath");
	m_nPathDecision=PATHFINDER_STILLFINDING;
	m_refPathFindTarget = GetWeakRef(pTargetObject);
  // add a slight offset to avoid starting below the floor
	Vec3 myPos = m_forcePathGenerationFrom.IsZero() ? GetPhysicsPos() + Vec3(0, 0, 0.05f) : m_forcePathGenerationFrom;
	
	Vec3	endDir = dir;
	// Check the type of the way the movement to the target, 0 = strict 1 = fastest, forget end direction.
	if( m_nMovementPurpose == 1 )
		endDir.Set( 0, 0, 0 );

  gAIEnv.pPathfinder->RequestPathTo(myPos, pos, endDir, this, allowDangerousDestination, forceTargetBuildingId, endTol, endDistance);
}

//===================================================================
// RequestPathInDirection
//===================================================================
void CPipeUser::RequestPathInDirection(const Vec3 &pos, float distance, CWeakRef<CAIObject> refTargetObject, float endDistance)
{
	m_OrigPath.Clear("CPipeUser::RequestPathInDirection m_OrigPath");
	m_nPathDecision=PATHFINDER_STILLFINDING;
	m_refPathFindTarget = refTargetObject;
  Vec3 myPos = GetPhysicsPos() + Vec3(0, 0, 0.05f);
  gAIEnv.pPathfinder->RequestPathInDirection(myPos, pos, distance, this, endDistance);
}

//====================================================================
// GetGoalPipe
//====================================================================
CGoalPipe *CPipeUser::GetGoalPipe(const char *name)
{
	CGoalPipe *pPipe = (CGoalPipe*) gAIEnv.pPipeManager->OpenGoalPipe(name);

	if (pPipe)
		return pPipe;
	else
		return 0;
}

//====================================================================
// IsUsingPipe
//====================================================================
bool CPipeUser::IsUsingPipe(const char *name) const
{
	CGoalPipe *pPipe = (CGoalPipe*) gAIEnv.pPipeManager->OpenGoalPipe(name);
	const CGoalPipe* pExecutingPipe = m_pCurrentGoalPipe;

	if (!pExecutingPipe)
		return false;

	while ( pExecutingPipe->IsInSubpipe() )
	{
		if (pExecutingPipe->GetNameAsString() == name)
			return true;
		pExecutingPipe = pExecutingPipe->GetSubpipe();
	}
	return pExecutingPipe->GetNameAsString() == name;
}

bool CPipeUser::IsUsingPipe(int goalPipeId) const
{
	const CGoalPipe* pExecutingPipe = m_pCurrentGoalPipe;
	while ( pExecutingPipe )
	{
		if (pExecutingPipe->GetEventId() == goalPipeId)
			return true;
		pExecutingPipe = pExecutingPipe->GetSubpipe();
	}
	return false;
}

//====================================================================
// RemoveActiveGoal
//====================================================================
void CPipeUser::RemoveActiveGoal(int nOrder)
{
	if (m_vActiveGoals.empty())
		return;
	int size = (int)m_vActiveGoals.size();

	if (size == 1)
	{
		m_vActiveGoals.front().pGoalOp->Reset(this);
		m_vActiveGoals.clear();
		return;
	}

	m_vActiveGoals[nOrder].pGoalOp->Reset(this);
	
	if (nOrder != size-1)
		m_vActiveGoals[nOrder] = m_vActiveGoals[size-1];

	m_vActiveGoals.pop_back();
}

bool CPipeUser::AbortActionPipe( int goalPipeId )
{
	CGoalPipe* pPipe = m_pCurrentGoalPipe;
	while (pPipe && pPipe->GetEventId() != goalPipeId)
		pPipe = pPipe->GetSubpipe();
	if ( !pPipe )
		return false;

	// high priority pipes are already aborted or not allowed to be aborted
	if (pPipe->IsHighPriority())
		return false;

	// aborted actions become high priority actions
	pPipe->HighPriority();

	// cancel all pipes inserted after this one unless another action is found
	while ( pPipe && pPipe->GetSubpipe() )
	{
		if ( pPipe->GetSubpipe()->GetNameAsString() == "_action_" )
			break;
		else if ( pPipe->GetSubpipe()->GetEventId() )
			CancelSubPipe( pPipe->GetSubpipe()->GetEventId() );
		pPipe = pPipe->GetSubpipe();
	}
	return true;
}

bool CPipeUser::SelectPipe(int mode, const char *name, CWeakRef<CAIObject> refArgument, int goalPipeId, bool resetAlways)
{
	if ( GetEntity() )
		gAIEnv.pAIActionManager->AbortAIAction( GetEntity() );

	refArgument.ValidateOrReset();

	if (m_pCurrentGoalPipe && !resetAlways)
	{
		if (m_pCurrentGoalPipe->GetNameAsString() == name)
		{
			CCCPOINT(CPipeUser_SelectPipe_A);

			if (refArgument.IsSet())
				m_pCurrentGoalPipe->SetRefArgument(refArgument);
			m_pCurrentGoalPipe->SetLoop((mode & AIGOALPIPE_RUN_ONCE)==0);
			return true;
		}
	}

	CGoalPipe* pHigherPriority = m_pCurrentGoalPipe;
	while ( pHigherPriority && !pHigherPriority->IsHighPriority() )
		pHigherPriority = pHigherPriority->GetSubpipe();

	if ( !pHigherPriority && refArgument.IsSet() )
	{
		SetLastOpResult(refArgument);
	}

	if (!pHigherPriority || resetAlways)
	{
		CCCPOINT(CPipeUser_SelectPipe_B);

		// reset some stuff with every goal-pipe change -----------------------------------------------------------------------------------
		m_State.bHurryNow = true;
		// strafing dist should be reset 
		CPuppet* pPuppet = CastToCPuppet();
		if (pPuppet)
		{
			pPuppet->SetAllowedStrafeDistances(0, 0, false);
			pPuppet->SetAdaptiveMovementUrgency(0, 0, 0);
			pPuppet->SetDelayedStance(STANCE_NULL);
		}

		// reset some stuff here
		if (GetProxy()&& (mode & AIGOALPIPE_DONT_RESET_AG)==0)
			GetProxy()->ResetAGInput(AIAG_ACTION);

		SetNavSOFailureStates();
	}

	CGoalPipe* pPipe = gAIEnv.pPipeManager->IsGoalPipe(name);
	if ( pPipe )
	{
		if(GetAttentionTarget())
			pPipe->SetAttTargetPosAtStart(GetAttentionTarget()->GetPos());
		else
			pPipe->SetAttTargetPosAtStart(ZERO);

		pPipe->SetRefArgument(refArgument);
	}
	else
	{
		AIError("SelectPipe: Goalpipe '%s' does not exists. Selecting default goalpipe '_first_' instead.", name);
		pPipe = gAIEnv.pPipeManager->IsGoalPipe( "_first_" );
		AIAssert( pPipe );
		pPipe->GetRefArgument().Reset();
	}

	if (!pHigherPriority || resetAlways)
		ResetCurrentPipe( resetAlways );
	m_pCurrentGoalPipe = pPipe;		// this might be too slow, in which case we will go back to registration
	m_pCurrentGoalPipe->Reset();
	m_pCurrentGoalPipe->SetEventId(goalPipeId);

	if ( pHigherPriority && !resetAlways)
		m_pCurrentGoalPipe->SetSubpipe( pHigherPriority );
	m_pCurrentGoalPipe->SetLoop((mode & AIGOALPIPE_RUN_ONCE)==0);

	GetAISystem()->Record(this, IAIRecordable::E_GOALPIPESELECTED, name);

	if(pPipe)
	{
		RecorderEventData recorderEventData(name);
		RecordEvent(IAIRecordable::E_GOALPIPESELECTED, &recorderEventData);
	}
	else
	{
		string	str(name);
		str += "[Not Found!]";
		RecorderEventData recorderEventData(str.c_str());
		RecordEvent(IAIRecordable::E_GOALPIPESELECTED, &recorderEventData);
	}

	ClearInvalidatedSOLinks();

	CCCPOINT(CPipeUser_SelectPipe_End);
	return true;
}

void CPipeUser::Pause(bool pause)
{
	m_paused += pause ? 1 : -1;

	assert(m_paused <= 8);
}

bool CPipeUser::IsPaused() const
{
	return m_paused > 0;
}


// used by action system to remove "_action_" inserted goal pipe
// which would notify goal pipe listeners and resume previous goal pipe
bool CPipeUser::RemoveSubPipe( int goalPipeId, bool keepInserted )
{
	if ( !goalPipeId )
		return false;

	if ( !m_pCurrentGoalPipe )
	{
		m_notAllowedSubpipes.insert( goalPipeId );
		return false;
	}

	// find the last goal pipe in the list
	CGoalPipe* pTargetPipe = NULL;
	CGoalPipe* pPrevPipe = m_pCurrentGoalPipe;
	while ( pPrevPipe->GetSubpipe() )
	{
		pPrevPipe = pPrevPipe->GetSubpipe();
		if ( pPrevPipe->GetEventId() == goalPipeId )
			pTargetPipe = pPrevPipe;
	}

	if ( keepInserted == false && pTargetPipe != NULL )
	{
		while ( CGoalPipe* pSubpipe = pTargetPipe->GetSubpipe() )
		{
			if ( pSubpipe->IsHighPriority() )
				break;
			if ( pSubpipe->GetEventId() )
				CancelSubPipe( pSubpipe->GetEventId() );
			else
				pTargetPipe = pSubpipe;
		}
	}

	if ( m_pCurrentGoalPipe->RemoveSubpipe(goalPipeId, keepInserted, true) )
	{
		NotifyListeners( goalPipeId, ePN_Removed );

		// find the last goal pipe in the list
		CGoalPipe* pCurrent = m_pCurrentGoalPipe;
		while ( pCurrent->GetSubpipe() )
			pCurrent = pCurrent->GetSubpipe();

		// only if the removed pipe was the last one in the list
		if ( pCurrent != pPrevPipe )
		{
			// remove goals
			ClearActiveGoals();

			// and resume the new last goal pipe
			SetLastOpResult( pCurrent->GetRefArgument() );
			NotifyListeners( pCurrent, ePN_Resumed );
		}
		return true;
	}
	else
	{
		m_notAllowedSubpipes.insert( goalPipeId );
		return false;
	}
}

// used by ai flow graph nodes to cancel inserted goal pipe
// which would notify goal pipe listeners that operation was failed
bool CPipeUser::CancelSubPipe( int goalPipeId )
{
	if ( !m_pCurrentGoalPipe )
	{
		if ( goalPipeId != 0 )
			m_notAllowedSubpipes.insert( goalPipeId );
		return false;
	}

	CGoalPipe* pCurrent = m_pCurrentGoalPipe;
	while ( pCurrent->GetSubpipe() )
		pCurrent = pCurrent->GetSubpipe();
	bool bClearNeeded = !goalPipeId || pCurrent->GetEventId() == goalPipeId;

	// enable this since it might be disabled from canceled goal pipe (and planed to enable it again at pipe end)
	m_bCanReceiveSignals = true;

	if ( m_pCurrentGoalPipe->RemoveSubpipe(goalPipeId, true, true) )
	{
		NotifyListeners( goalPipeId, ePN_Deselected );

		if ( bClearNeeded )
		{
			ClearActiveGoals();

			// resume the parent goal pipe
			pCurrent = m_pCurrentGoalPipe;
			while ( pCurrent->GetSubpipe() )
				pCurrent = pCurrent->GetSubpipe();
			SetLastOpResult( pCurrent->GetRefArgument() );
			NotifyListeners( pCurrent, ePN_Resumed );
		}

		return true;
	}
	else
	{
		if ( goalPipeId != 0 )
			m_notAllowedSubpipes.insert( goalPipeId );
		return false;
	}
}

IGoalPipe* CPipeUser::InsertSubPipe(int mode, const char * name, CWeakRef<CAIObject> refArgument, int goalPipeId)
{
	if (!m_pCurrentGoalPipe)
		return NULL;

	if ( goalPipeId != 0 )
	{
		VectorSet< int >::iterator itFind = m_notAllowedSubpipes.find( goalPipeId );
		if ( itFind != m_notAllowedSubpipes.end() )
		{
			CCCPOINT(CPipeUser_InsertSubPipe_NotAllowed);

			m_notAllowedSubpipes.erase( itFind );
			return NULL;
		}
	}

	if (m_pCurrentGoalPipe->CountSubpipes() >= 8)
	{
		AIWarning("%s has too many (%d) subpipes. Pipe <%s>; inserting <%s>", GetName(), m_pCurrentGoalPipe->CountSubpipes(),
			m_pCurrentGoalPipe->GetName(), name);
	}

	bool bExclusive = (mode & AIGOALPIPE_NOTDUPLICATE) != 0;
	bool bHighPriority = (mode & AIGOALPIPE_HIGHPRIORITY) != 0; // it will be not removed when a goal pipe is selected
	bool bSamePriority = (mode & AIGOALPIPE_SAMEPRIORITY) != 0; // sets the priority to be the same as active goal pipe
	bool bKeepOnTop = (mode & AIGOALPIPE_KEEP_ON_TOP) != 0; // will keep this pipe at the top of the stack

//	if (m_pCurrentGoalPipe->m_sName == name && !goalPipeId)
//		return NULL;

	// first lets find the goalpipe
	CGoalPipe* pPipe = gAIEnv.pPipeManager->IsGoalPipe( name );
	if ( !pPipe )
	{
		AIWarning("InsertSubPipe: Goalpipe '%s' does not exists. No goalpipe inserted.", name);

		string	str(name);
		str += "[Not Found!]";
		RecorderEventData recorderEventData(str.c_str());
		RecordEvent(IAIRecordable::E_GOALPIPEINSERTED, &recorderEventData);

		return NULL;
	}

	if ( bHighPriority )
		pPipe->HighPriority();

	if ( bKeepOnTop )
		pPipe->m_bKeepOnTop = true;

	// now find the innermost pipe
	CGoalPipe* pExecutingPipe = m_pCurrentGoalPipe;
	int i=0;
	while ( pExecutingPipe->IsInSubpipe() )
	{
		CCCPOINT(CPipeUser_InsertSubPipe_IsInSubpipe);

		if( bExclusive && pExecutingPipe->GetNameAsString() == name )
			return NULL;
		if ( pExecutingPipe->GetSubpipe()->m_bKeepOnTop )
			break;
		if ( !bSamePriority && !bHighPriority )
		{
			if ( pExecutingPipe->GetSubpipe()->IsHighPriority() )
				break;
		}
		pExecutingPipe = pExecutingPipe->GetSubpipe();
		++i;
	}
	
	if(i>10)
	{
		CGoalPipe* pCurrentPipe = m_pCurrentGoalPipe;
		AIWarning("%s: More than 10 subpipes",GetName());
		int j=0;
		while ( pCurrentPipe->IsInSubpipe() )
		{
			const string& sDebugName = pCurrentPipe->GetDebugName();
			AILogAlways("%d) %s",j++, sDebugName.empty() ? pCurrentPipe->GetName() : sDebugName.c_str());
			pCurrentPipe = pCurrentPipe->GetSubpipe();
		}
	}
	CCCPOINT(CPipeUser_InsertSubPipe_A);

	if ( bExclusive && pExecutingPipe->GetNameAsString() == name )
		return NULL;
	if ( !pExecutingPipe->GetSubpipe() )
	{
		if ( !m_vActiveGoals.empty() )
		{
			if ( m_bBlocked )
			{
				// pop the last executing goal
				ClearActiveGoals();
				//RemoveActiveGoal(m_vActiveGoals.size()-1);

				// but make sure we end up executing it again
				pExecutingPipe->ReExecuteGroup();
			}
			else
			{
				ClearActiveGoals();
			}
		}

		NotifyListeners( pExecutingPipe, ePN_Suspended );

		// (MATT) Do we actually need to check? {2009/03/13}
		if ( refArgument.IsSet() )
			SetLastOpResult( refArgument );

		CCCPOINT(CPipeUser_InsertSubPipe_Unblock);

		// unblock current pipe
		m_bBlocked = false;
	}

	pExecutingPipe->SetSubpipe(pPipe);

	if(GetAttentionTarget())
		pPipe->SetAttTargetPosAtStart(GetAttentionTarget()->GetPos());
	else
		pPipe->SetAttTargetPosAtStart(ZERO);
	pPipe->SetRefArgument(refArgument);
	pPipe->SetEventId(goalPipeId);

	GetAISystem()->Record(this, IAIRecordable::E_GOALPIPEINSERTED, name);

	RecorderEventData recorderEventData(name);
	RecordEvent(IAIRecordable::E_GOALPIPEINSERTED, &recorderEventData);

	CCCPOINT(CPipeUser_InsertSubPipe_End);

	return pPipe;
}

void CPipeUser::ClearActiveGoals()
{
	if (!m_vActiveGoals.empty())
	{
		VectorOGoals::iterator li, liEnd = m_vActiveGoals.end();
		for (li = m_vActiveGoals.begin(); li != liEnd; ++li)
			li->pGoalOp->Reset(this);

		m_vActiveGoals.clear();
	}
	m_bBlocked = false;
}

void CPipeUser::ResetCurrentPipe( bool resetAlways )
{
	GetAISystem()->Record(this, IAIRecordable::E_GOALPIPERESETED,
		m_pCurrentGoalPipe ? m_pCurrentGoalPipe->GetName() : 0);

	RecorderEventData recorderEventData(m_pCurrentGoalPipe ? m_pCurrentGoalPipe->GetName() : 0);
	RecordEvent(IAIRecordable::E_GOALPIPERESETED, &recorderEventData);

	CGoalPipe* pPipe = m_pCurrentGoalPipe;
	while ( pPipe )
	{
		NotifyListeners( pPipe, ePN_Deselected );
		if ( resetAlways || !pPipe->GetSubpipe() || !pPipe->GetSubpipe()->IsHighPriority() )
			pPipe = pPipe->GetSubpipe();
		else
			break;
	}

	if ( !pPipe )
		ClearActiveGoals();
	else
		pPipe->SetSubpipe( NULL );

	if ( m_pCurrentGoalPipe )
	{
		delete m_pCurrentGoalPipe;
		m_pCurrentGoalPipe = 0;
	}

	if ( !pPipe )
	{
		m_bCanReceiveSignals = true;
		m_bKeepMoving = false;
		ResetLookAt();
		ResetBodyTargetDir();
	}
}

int CPipeUser::GetGoalPipeId() const
{
	const CGoalPipe* pPipe = m_pCurrentGoalPipe;
	if ( !pPipe )
		return -1;
	while ( pPipe->GetSubpipe() )
		pPipe = pPipe->GetSubpipe();
	return pPipe->GetEventId();
}

void CPipeUser::ResetLookAt()
{
	m_bPriorityLookAtRequested = false;
	if ( m_bLooseAttention )
	{
		SetLooseAttentionTarget(NILREF);
	}
}

bool CPipeUser::SetLookAtPointPos( const Vec3& point, bool priority )
{
	if(!priority&&m_bPriorityLookAtRequested)
		return false;
	m_bPriorityLookAtRequested = priority;

	CCCPOINT(CPipeUser_SetLookAtPoint);

	CreateLookAtTarget();
	m_refLooseAttentionTarget = m_refLookAtTarget;
	CAIObject *pLookAtTarget = m_refLookAtTarget.GetAIObject();
	pLookAtTarget->SetPos( point );
	m_bLooseAttention = true;

	// check only in 2D
	Vec3 desired( point - GetPos() );
	desired.z = 0;
	desired.NormalizeSafe();
	SAIBodyInfo bodyInfo;
	GetProxy()->QueryBodyInfo( bodyInfo );
	Vec3 current( bodyInfo.vEyeDirAnim );
	current.z = 0;
	current.NormalizeSafe();

	// cos( 11.5deg ) ~ 0.98
	return 0.98f <= current.Dot( desired );
}

bool CPipeUser::SetLookAtDir( const Vec3& dir, bool priority )
{
	if(!priority&&m_bPriorityLookAtRequested)
		return false;
	m_bPriorityLookAtRequested = priority;

	CCCPOINT(CPipeUser_SetLookAtDir);

	Vec3 vDir = dir;
	if ( vDir.NormalizeSafe() )
	{
		CreateLookAtTarget();
		m_refLooseAttentionTarget = m_refLookAtTarget;
		CAIObject *pLookAtTarget = m_refLookAtTarget.GetAIObject();
		pLookAtTarget->SetPos( GetPos() + vDir * 100.0f );
		m_bLooseAttention = true;

		// check only in 2D
		Vec3 desired( vDir );
		desired.z = 0;
		desired.NormalizeSafe();
		SAIBodyInfo bodyInfo;
		GetProxy()->QueryBodyInfo( bodyInfo );
		Vec3 current( bodyInfo.vEyeDirAnim );
		current.z = 0;
		current.NormalizeSafe();

		// cos( 11.5deg ) ~ 0.98
		if ( 0.98f <= current.Dot(desired) )
		{
			SetLooseAttentionTarget(NILREF);
			return true;
		}
		else
			return false;
	}
	else
		return true;
}


void CPipeUser::ResetBodyTargetDir()
{
	m_vBodyTargetDir.zero();
}


void CPipeUser::SetBodyTargetDir( const Vec3& dir )
{
	m_vBodyTargetDir = dir.GetNormalizedSafe(Vec3Constants<float>::fVec3_Zero);
}


Vec3 CPipeUser::GetLooseAttentionPos()
{
	CCCPOINT(CPipeUser_GetLooseAttentionPos);

	if (m_bLooseAttention)
	{
		CAIObject* pLooseAttentionTarget = m_refLooseAttentionTarget.GetAIObject();
		if (pLooseAttentionTarget)
		{
			return pLooseAttentionTarget->GetPos();
		}
	}

	return ZERO;
}


CAIObject* CPipeUser::GetSpecialAIObject(const char* objName, float range)
{
	if (!objName) return NULL;

	CCCPOINT(CPipeUser_GetSpecialAIObject);

	CAIObject* pObject = NULL;

	CWeakRef<CPipeUser> refThis = GetWeakRef(this);

	if( range <= 0.0f )
		range = 20.0f;

	if(strcmp(objName, "player") == 0)
	{
		pObject= GetAISystem()->GetPlayer();
	}
	else if(strcmp(objName, "self") == 0)
	{
		pObject = this;
	}
	else if(strcmp(objName, "beacon") == 0)
	{
		pObject = static_cast<CAIObject*>(GetAISystem()->GetBeacon(GetGroupId()));
	}
	else if(strcmp(objName, "refpoint") == 0)
	{
		CPipeUser* pPipeUser = CastToCPipeUser();
		if (pPipeUser)
			pObject = pPipeUser->GetRefPoint();
	}
	else if(strcmp(objName, "formation") == 0)
	{
		CLeader* pLeader = GetAISystem()->GetLeader(GetGroupId());
		// is leader managing my formation?

		// (MATT) Either create one system that will suffice or consider choosing by different string etc {2008/09/02}
		if (gAIEnv.configuration.eCompatibilityMode == ECCM_GAME04 || gAIEnv.configuration.eCompatibilityMode == ECCM_WARFACE)
		{

			CGroupMember* pMember = gAIEnv.pGroupSystem->GetGroupMember( GetEntityID() );
			if( pMember != NULL )
			{
				pObject = pMember->GetFormationPosAIObject();
			}
			if (!pObject)
			{
				// lets send a NoFormationPoint event
				SetSignal(1,"OnNoFormationPoint", 0,0,gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
			}

		}
		else
		{
			CAIObject *pFormationOwner = (pLeader ? pLeader->GetFormationOwner().GetAIObject() : 0);
			if(pFormationOwner && pFormationOwner->m_pFormation)
			{
				// check if I already have one
				pObject= pLeader->GetFormationPoint( refThis ).GetAIObject();
				if(!pObject)
					pObject= pLeader->GetNewFormationPoint( refThis ).GetAIObject();
			}
			else
			{
				CCCPOINT(CPipeUser_GetSpecialAIObject_A);

				CAIObject	*pLastOp = GetLastOpResult();
				// check if Formation point is already acquired to lastOpResult
				if(pLastOp && pLastOp->GetSubType()==CAIObject::STP_FORMATION)
					pObject = pLastOp;
				else
				{
					CAIObject *pBoss=static_cast<CAIObject*>(GetAISystem()->GetNearestObjectOfTypeInRange(this, AIOBJECT_PUPPET, 0, range, AIFAF_HAS_FORMATION|AIFAF_INCLUDE_DEVALUED|AIFAF_SAME_GROUP_ID));
					if(!pBoss)
						pBoss=static_cast<CAIObject*>(GetAISystem()->GetNearestObjectOfTypeInRange(this, AIOBJECT_VEHICLE, 0, range, AIFAF_HAS_FORMATION|AIFAF_INCLUDE_DEVALUED|AIFAF_SAME_GROUP_ID));
					if(pBoss)
					{
						// check if I already have one
						pObject = pBoss->m_pFormation->GetFormationPoint(refThis);
						if(!pObject)
							pObject = pBoss->m_pFormation->GetNewFormationPoint(refThis);
					}
				}
				if (!pObject)
				{
					// lets send a NoFormationPoint event
					SetSignal(1,"OnNoFormationPoint", 0,0,gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
					if(pLeader)
						pLeader->SetSignal(1,"OnNoFormationPoint",GetEntity(),0,gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
				}
			}

		}

	}
	else if(strcmp(objName, "formation_ref") == 0)
	{
		CLeader* pLeader = GetAISystem()->GetLeader(GetGroupId());
		// is leader managing my formation?

		CGroupMember* pMember = gAIEnv.pGroupSystem->GetGroupMember( GetEntityID() );
		if( pMember != NULL )
		{
			pObject = pMember->GetFormationRefAIObject();
		}
		if (!pObject)
		{
			// lets send a NoFormationPoint event
			SetSignal(1,"OnNoFormationReference", 0,0,gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
		}
	}
	else if(strcmp(objName, "formation_id") == 0)
	{
		
		CAIObject	*pLastOp=GetLastOpResult();
		// check if Formation point is already aquired to lastOpResult
		
		if(pLastOp && pLastOp->GetSubType()==CAIObject::STP_FORMATION)
			pObject = pLastOp;
		else	// if not - find available formation point
		{
				int groupid((int)range);
				CAIObject *pBoss(NULL);
				CAISystem::AIObjects::iterator ai;
				if ( (ai=GetAISystem()->m_mapGroups.find(groupid)) != GetAISystem()->m_mapGroups.end() )
				{
					for (;ai!=GetAISystem()->m_mapGroups.end() && !pBoss;)
					{
						if (ai->first != groupid)
							break;
						CAIObject *pObj = ai->second.GetAIObject();
						if (pObj && pObj != this && pObj->m_pFormation)
						{
							pBoss = pObj;
						}
						++ai;
					}
				}
				if(pBoss && pBoss->m_pFormation)
				{
					// check if I already have one

					pObject = pBoss->m_pFormation->GetFormationPoint(refThis);
					if(!pObject)
						pObject = pBoss->m_pFormation->GetNewFormationPoint(refThis);
				}

			if (!pObject)
			{
				// lets send a NoFormationPoint event
				SetSignal(1,"OnNoFormationPoint", 0,0, gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
			}
		}

	}
	else if(strcmp(objName, "formation_special") == 0)
	{
		CLeader* pLeader = GetAISystem()->GetLeader(GetGroupId());
		if(pLeader)
			pObject = pLeader->GetNewFormationPoint(refThis, SPECIAL_FORMATION_POINT).GetAIObject();

		if (!pObject)
		{
			// lets send a NoFormationPoint event
			SetSignal(1,"OnNoFormationPoint", 0,0, gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
			if(pLeader)
				pLeader->SetSignal(1,"OnNoFormationPoint",GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnNoFormationPoint);
		}
	}
	else if(strcmp(objName, "atttarget") == 0)
	{
		CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
		pObject= pAttentionTarget;
	}
	else if(strcmp(objName, "hidepoint_lastop") == 0)
	{
		CCCPOINT(CPipeUser_GetSpecialAIObject_hidepoint_lastop);
		int nbid;
		IVisArea *iva;

		Vec3 hideFrom;
		CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
		if (pAttentionTarget)
			hideFrom = pAttentionTarget->GetPos();
		else
			hideFrom = GetPos();

		Vec3 pos = FindHidePoint(range, hideFrom, HM_IGNORE_ENEMIES+ HM_NEAREST + HM_AROUND_LASTOP, gAIEnv.pNavigation->CheckNavigationType(GetPos(),nbid,iva,m_movementAbility.pathfindingProperties.navCapMask));
		if (!IsEquivalent(pos,GetPos(),0.01f))
		{
			pObject = GetOrCreateSpecialAIObject(AISPECIAL_HIDEPOINT_LASTOP);
			pObject->SetPos(pos);
		}
	}
	else if(strcmp(objName, "last_hideobject") == 0)
	{
		if(m_CurrentHideObject.IsValid())
		{
			pObject = GetOrCreateSpecialAIObject(AISPECIAL_LAST_HIDEOBJECT);
			pObject->SetPos(m_CurrentHideObject.GetObjectPos());
		}
	}	
	else if(strcmp(objName, "lookat_target") == 0)
	{
		if ( m_bLooseAttention && m_refLooseAttentionTarget.IsValid() )
			pObject = m_refLooseAttentionTarget.GetAIObject();
	}
	else if(strcmp(objName, "probtarget") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_PROBTARGET);
		pObject->SetPos(GetProbableTargetPosition());
	}
	else if(strcmp(objName, "probtarget_in_territory") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_PROBTARGET_IN_TERRITORY);
		Vec3	pos = GetProbableTargetPosition();
		// Clamp the point to the territory shape.
		CPuppet* pPuppet = CastToCPuppet();
		if(pPuppet && pPuppet->GetTerritoryShape())
				pPuppet->GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

		pObject->SetPos(pos);
	}
	else if(strcmp(objName, "probtarget_in_refshape") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_PROBTARGET_IN_REFSHAPE);
		Vec3	pos = GetProbableTargetPosition();
		// Clamp the point to ref shape
		if(GetRefShape())
			GetRefShape()->ConstrainPointInsideShape(pos, true);
		pObject->SetPos(pos);
	}
	else if(strcmp(objName, "probtarget_in_territory_and_refshape") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_PROBTARGET_IN_TERRITORY_AND_REFSHAPE);
		Vec3	pos = GetProbableTargetPosition();
		// Clamp the point to ref shape
		if(GetRefShape())
			GetRefShape()->ConstrainPointInsideShape(pos, true);
		// Clamp the point to the territory shape.
		CPuppet* pPuppet = CastToCPuppet();
		if(pPuppet && pPuppet->GetTerritoryShape())
				pPuppet->GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

		pObject->SetPos(pos);
	}
	else if(strcmp(objName, "atttarget_in_territory") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_ATTTARGET_IN_TERRITORY);
		if(GetAttentionTarget())
		{
			Vec3	pos = GetAttentionTarget()->GetPos();
			// Clamp the point to the territory shape.
			CPuppet* pPuppet = CastToCPuppet();
			if(pPuppet && pPuppet->GetTerritoryShape())
					pPuppet->GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

			pObject->SetPos(pos);
		}
		else
		{
			pObject->SetPos(GetPos());
		}
	}
	else if(strcmp(objName, "atttarget_in_refshape") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_ATTTARGET_IN_REFSHAPE);
		if(GetAttentionTarget())
		{
			Vec3	pos = GetAttentionTarget()->GetPos();
			// Clamp the point to ref shape
			if(GetRefShape())
				GetRefShape()->ConstrainPointInsideShape(pos, true);
			// Update pos
			pObject->SetPos(pos);
		}
		else
		{
			pObject->SetPos(GetPos());
		}
	}
	else if(strcmp(objName, "atttarget_in_territory_and_refshape") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_ATTTARGET_IN_TERRITORY_AND_REFSHAPE);
		if(GetAttentionTarget())
		{
			Vec3	pos = GetAttentionTarget()->GetPos();
			// Clamp the point to ref shape
			if(GetRefShape())
				GetRefShape()->ConstrainPointInsideShape(pos, true);
			// Clamp the point to the territory shape.
			CPuppet* pPuppet = CastToCPuppet();
			if(pPuppet && pPuppet->GetTerritoryShape())
					pPuppet->GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

			pObject->SetPos(pos);
		}
		else
		{
			pObject->SetPos(GetPos());
		}
	}
	else if(strcmp(objName, "animtarget") == 0)
	{
		if(m_pActorTargetRequest)
		{			pObject = GetOrCreateSpecialAIObject(AISPECIAL_ANIM_TARGET);
			pObject->SetPos(m_pActorTargetRequest->approachLocation, m_pActorTargetRequest->approachDirection);
		}
	}
	else if(strcmp(objName, "group_tac_pos") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_GROUP_TAC_POS);
	}
	else if(strcmp(objName, "group_tac_look") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_GROUP_TAC_LOOK);
	}
	else if(strcmp(objName, "vehicle_avoid") == 0)
	{
		pObject = GetOrCreateSpecialAIObject(AISPECIAL_VEHICLE_AVOID_POS);
	}
	else
	{
		pObject = GetAISystem()->GetAIObjectByName(objName);
	}
	return pObject;
}


//====================================================================
// GetTargetScore
//====================================================================
static int	GetTargetScore(CAIObject* pAI, CAIObject* pTarget)
{
	// Returns goodness value based on the target type.
	int	score = 0;

	if(pAI->IsHostile(pTarget)) score += 10;
	if(pTarget->GetSubType() == IAIObject::STP_SOUND) score += 1;
	if(pTarget->GetSubType() == IAIObject::STP_MEMORY) score += 2;
	if(pTarget->IsAgent()) score += 5;

	return score;
}

//====================================================================
// GetProbableTargetPosition
//====================================================================
Vec3 CPipeUser::GetProbableTargetPosition()
{
	if(m_timeSinceLastLiveTarget >= 0.0f && m_timeSinceLastLiveTarget < 2.0f)
	{
		// The current target is fresh, use it.
		return m_lastLiveTargetPos;
	}
	else
	{
		CCCPOINT(CPipeUser_GetProbableTargetPosition);
		// - Choose the nearest target that is more fresh than ours
		//   among the units sharing the same combat class in the group
		// - If no fresh targets are not available, try to choose the most
		//   important attention target
		// - If no attention target is available, try beacon
		// - If there is no beacon, just return the position of self.
		bool	foundLive = false;
		bool	foundAny = false;

		Vec3	nearestKnownTarget;
		CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
		if(pAttentionTarget)
			nearestKnownTarget = pAttentionTarget->GetPos();
		else
			nearestKnownTarget = GetPos();

		float	nearestLiveDist = FLT_MAX;
		Vec3	nearestLivePos = nearestKnownTarget;

		float	nearestAnyDist = FLT_MAX;
		Vec3	nearestAnyPos = nearestKnownTarget;
		int		nearestAnyScore = 0;

		int	combatClass = m_Parameters.m_CombatClass;

		int groupId = GetGroupId();

		CAISystem::AIObjects::iterator gi = GetAISystem()->m_mapGroups.find(groupId);
		CAISystem::AIObjects::iterator gend = GetAISystem()->m_mapGroups.end();

		for(; gi != gend; ++gi)
		{
			if(gi->first != groupId) break;
			CAIActor* pAIActor = CastToCAIActorSafe(gi->second.GetAIObject());
			if(!pAIActor || pAIActor->GetParameters().m_CombatClass != combatClass) continue;
			if(!pAIActor->IsEnabled()) continue;

			CPuppet* pPuppet = pAIActor->CastToCPuppet();
			if(pPuppet)
			{
				// Find nearest fresh live target.
				float	otherTargetTime = pPuppet->GetTimeSinceLastLiveTarget();
				if(otherTargetTime >= 0.0f && otherTargetTime < 2.0f && otherTargetTime < m_timeSinceLastLiveTarget)
				{
					float	dist = Distance::Point_PointSq(nearestKnownTarget, pPuppet->GetLastLiveTargetPosition());
					if(dist < nearestLiveDist)
					{
						nearestLiveDist = dist;
						nearestLivePos = pPuppet->GetLastLiveTargetPosition();
						foundLive = true;
					}
				}

				// Fall back.
				if(!foundLive && !pAttentionTarget && pPuppet->GetAttentionTarget())
				{
					const Vec3& targetPos = pPuppet->GetAttentionTarget()->GetPos();
					float	dist = Distance::Point_PointSq(nearestAnyPos, targetPos);
					int	score = GetTargetScore(this, pPuppet);
					if(score >= nearestAnyScore && dist < nearestAnyDist)
					{
						nearestAnyDist = dist;
						nearestAnyPos = targetPos;
						nearestAnyScore = score;
						foundAny = true;
					}
				}
			}
		}

		if (foundLive)
		{
			return nearestLivePos;
		}
		else if (pAttentionTarget)
		{
			return pAttentionTarget->GetPos();
		}
		else if (foundAny)
		{
			return nearestAnyPos;
		}
		else
		{
			IAIObject*	beacon = GetAISystem()->GetBeacon(GetGroupId());
			if(beacon)
				return beacon->GetPos();
			else
				return GetPos();
		}
	}
}

//====================================================================
// GetOrCreateSpecialAIObject
//====================================================================
CAIObject* CPipeUser::GetOrCreateSpecialAIObject(ESpecialAIObjects type)
{
	//fix for /analyze warning
	assert(type < COUNT_AISPECIAL);

	CStrongRef<CAIObject> &refSpecialObj = m_refSpecialObjects[type];

	if(!refSpecialObj.IsNil())
		return refSpecialObj.GetAIObject();

	CCCPOINT(CPipeUser_GetOrCreateSpecialAIObject);

	ESubTypes	subType = STP_SPECIAL;

	string	name = GetName();
	switch(type)
	{
	case AISPECIAL_HIDEPOINT_LASTOP:
		name += "_*HidePointLastop"; break;
	case AISPECIAL_LAST_HIDEOBJECT:
		name += "_*LastHideObj"; break;
	case AISPECIAL_PROBTARGET:
		name += "_*ProbTgt"; break;
	case AISPECIAL_PROBTARGET_IN_TERRITORY:
		name += "_*ProbTgtInTerr"; break;
	case AISPECIAL_PROBTARGET_IN_REFSHAPE:
		name += "_*ProbTgtInRefShape"; break;
	case AISPECIAL_PROBTARGET_IN_TERRITORY_AND_REFSHAPE:
		name += "_*ProbTgtInTerrAndRefShape"; break;
	case AISPECIAL_ATTTARGET_IN_TERRITORY:
		name += "_*AttTgtInTerr"; break;
	case AISPECIAL_ATTTARGET_IN_REFSHAPE:
		name += "_*AttTgtInRefShape"; break;
	case AISPECIAL_ATTTARGET_IN_TERRITORY_AND_REFSHAPE:
		name += "_*AttTgtInTerrAndRefShape"; break;
	case AISPECIAL_ANIM_TARGET:
		name += "_*AnimTgt";
		subType = STP_ANIM_TARGET;
		break;
	case AISPECIAL_GROUP_TAC_POS:
		name += "_GroupTacPos";
		subType = STP_FORMATION;
		break;
	case AISPECIAL_GROUP_TAC_LOOK:
		name += "_GroupTacLook";
		break;
	case AISPECIAL_VEHICLE_AVOID_POS:
		name += "_VehicleAvoid";
		break;
	default:
		{
			char	buf[64];
			_snprintf(buf, 64, "_*Special%02d", (int)type);
			name += buf;
			break;
		}
	}

	GetAISystem()->CreateDummyObject(refSpecialObj, name, subType);
	return refSpecialObj.GetAIObject();
}

//====================================================================
// PathIsInvalid
//====================================================================
void CPipeUser::PathIsInvalid()
{
	ClearPath("CPipeUser::PathIsInvalid m_Path");
	m_State.vMoveDir.Set(0.0f, 0.0f, 0.0f);
	m_State.fDesiredSpeed = 0.0f;
}

bool CPipeUser::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	CCCPOINT(CPipeUser_Serialize);

	// m_mapGoalPipeListeners must not change here! there's no need to serialize it!
	
  if(ser.IsReading())
		ResetCurrentPipe( true );

	const bool bNeedsPostSerialize = CAIActor::Serialize(ser, objectTracker);

	// serialize members
	m_refRefPoint.Serialize(ser,"m_refRefPoint");
	m_refLookAtTarget.Serialize(ser,"m_refRefLookAtTarget");
	m_refAttentionTarget.Serialize(ser,"m_refAttentionTarget");

	ser.Value("m_AttTargetPersistenceTimeout",m_AttTargetPersistenceTimeout);
	ser.EnumValue("m_AttTargetThreat", m_AttTargetThreat, AITHREAT_NONE, AITHREAT_LAST);
	ser.EnumValue("m_AttTargetExposureThreat", m_AttTargetExposureThreat, AITHREAT_NONE, AITHREAT_LAST);
	ser.EnumValue("m_AttTargetType", m_AttTargetType, AITARGET_NONE, AITARGET_LAST);

	m_refLastOpResult.Serialize(ser,"m_refLastOpResult");

	ser.Value("m_nMovementPurpose", m_nMovementPurpose);
	ser.Value("m_fTimePassed", m_fTimePassed);
	ser.Value("m_bInCover", m_bInCover);
	ser.Value("m_bMovingToCover", m_bMovingToCover);
	ser.Value("LooseAttention", m_bLooseAttention);


	m_refLooseAttentionTarget.Serialize(ser, "m_refLooseAttentionTarget");

	ser.Value("m_looseAttentionId",m_looseAttentionId);

	m_refLastOpResult.Serialize(ser,"m_refLookAtTarget");

	m_CurrentHideObject.Serialize(ser,objectTracker);

	ser.Value("m_nPathDecision", m_nPathDecision);
	ser.Value("m_IsSteering",m_IsSteering);


	int	fireMode = (int)m_fireMode;
	ser.Value("m_fireMode", fireMode);
	if(ser.IsReading())
		m_fireMode = (EFireMode)fireMode;

	m_refFireTarget.Serialize(ser, "m_refFireTarget");

	ser.Value("m_fireModeUpdated",m_fireModeUpdated);
	ser.Value("m_outOfAmmoSent",m_outOfAmmoSent);
	ser.Value("m_wasReloading", m_wasReloading );
 	ser.Value("m_bFirstUpdate",m_bFirstUpdate);
	ser.Value("m_pathToFollowName",m_pathToFollowName);
	ser.Value("m_bPathToFollowIsSpline",m_bPathToFollowIsSpline);
	ser.EnumValue("m_CurrentNodeNavType",m_CurrentNodeNavType,IAISystem::NAV_UNSET,IAISystem::NAV_MAX_VALUE);

	// Serialize special objects.
	char	specialName[64];
	for(unsigned i = 0; i < COUNT_AISPECIAL; ++i)
	{
			_snprintf(specialName, 64, "m_pSpecialObjects%d", i);
		m_refSpecialObjects[i].Serialize(ser, specialName);
	}

	// serialize executing pipe
	if(ser.IsReading())
	{
		string	goalPipeName;
		ser.Value( "PipeName", goalPipeName);

		// always create a new goal pipe
		CGoalPipe *pPipe = NULL;
		if ( goalPipeName != "none" )
		{
			pPipe = new CGoalPipe(goalPipeName, true);
			ser.BeginGroup( "DynamicPipe" );
				pPipe->SerializeDynamic( ser );
			ser.EndGroup();
		}

		m_pCurrentGoalPipe = pPipe;		// this might be too slow, in which case we will go back to registration
		if ( m_pCurrentGoalPipe )
		{
			m_pCurrentGoalPipe->Reset();
			m_pCurrentGoalPipe->Serialize( ser, objectTracker, m_vActiveGoals );
		}

		m_notAllowedSubpipes.clear();
	}
	else
	{
		if (m_pCurrentGoalPipe)
		{
			ser.Value( "PipeName", m_pCurrentGoalPipe->GetNameAsString());
			ser.BeginGroup( "DynamicPipe" );
				m_pCurrentGoalPipe->SerializeDynamic( ser );
			ser.EndGroup();

			m_pCurrentGoalPipe->Serialize( ser, objectTracker, m_vActiveGoals );
		}
		else
		{
			ser.Value( "PipeName", string("none") );
		}
	}

	// this stuff can get reset when selecting pipe - so, serialize it after
	ser.Value("Blocked", m_bBlocked);
	ser.Value("KeepMoving", m_bKeepMoving);
	ser.Value("LastMoveDir", m_vLastMoveDir);

	ser.Value("m_PathDestinationPos", m_PathDestinationPos);

	m_Path.Serialize(ser, objectTracker);
	m_OrigPath.Serialize(ser, objectTracker);
  if(ser.IsWriting())
  {
    if(ser.BeginOptionalGroup("m_pPathFollower", m_pPathFollower!=NULL))
    {
      m_pPathFollower->Serialize(ser, objectTracker);
			m_pPathFollower->AttachToPath(&m_Path);
      ser.EndGroup();
    }
  }
  else
  {
    SAFE_DELETE(m_pPathFollower);
    if(ser.BeginOptionalGroup("m_pPathFollower", true))
    {
      m_pPathFollower = new CPathFollower();
      m_pPathFollower->Serialize(ser, objectTracker);
			m_pPathFollower->AttachToPath(&m_Path);
      ser.EndGroup();
    }
  }
	ser.Value( "m_posLookAtSmartObject", m_posLookAtSmartObject );
	ser.Value( "m_forcePathGenerationFrom", m_forcePathGenerationFrom );
	
	ser.BeginGroup("UnreachableHideObjectList");
	{
		int count  = m_recentUnreachableHideObjects.size();
		ser.Value("UnreachableHideObjectList_size",count);
		if ( ser.IsReading() )
			m_recentUnreachableHideObjects.clear();
		TimeOutVec3List::iterator it = m_recentUnreachableHideObjects.begin();
		float time(0);
		Vec3 point(0);
		for(int i=0;i<count;i++)
		{
			char name[32];
			if(ser.IsWriting())
			{
				time = it->first;
				point = it->second;
				++it;
			}
			sprintf(name,"time_%d",i);
			ser.Value(name,time);
			sprintf(name,"point_%d",i);
			ser.Value(name,point);
			if(ser.IsReading())
				m_recentUnreachableHideObjects.push_back(std::make_pair(time,point));
		}
		ser.EndGroup();
	}

	ser.EnumValue( "m_eNavSOMethod", m_eNavSOMethod, nSOmNone, nSOmLast );
	ser.Value( "m_navSOEarlyPathRegen", m_navSOEarlyPathRegen );
	ser.Value( "m_idLastUsedSmartObject", m_idLastUsedSmartObject );

	m_currentNavSOStates.Serialize(ser, objectTracker);
	m_pendingNavSOStates.Serialize(ser, objectTracker);

	ser.Value( "m_actorTargetReqId", m_actorTargetReqId);

	ser.Value( "m_refShapeName", m_refShapeName);
	if(ser.IsReading())
		m_refShape = GetAISystem()->GetGenericShapeOfName(m_refShapeName.c_str());

	if(ser.IsWriting())
	{
		if(ser.BeginOptionalGroup("m_pActorTargetRequest", m_pActorTargetRequest != 0))
		{
			m_pActorTargetRequest->Serialize(ser, objectTracker);
			ser.EndGroup();
		}
	}
	else
	{
		if(ser.BeginOptionalGroup("m_pActorTargetRequest", true))
		{
			if (!m_pActorTargetRequest)
				m_pActorTargetRequest = new SAIActorTargetRequest;
			m_pActorTargetRequest->Serialize(ser, objectTracker);
			ser.EndGroup();
		}
	}

	return bNeedsPostSerialize;
}

bool CPipeUser::SetCharacter(const char *character, const char* behaviour)
{
	return GetProxy()->SetCharacter( character, behaviour );
}

void CPipeUser::NotifyListeners( CGoalPipe* pPipe, EGoalPipeEvent event, bool includeSubPipes )
{
	if (pPipe && pPipe->GetEventId())
		NotifyListeners(pPipe->GetEventId(), event);
	if(includeSubPipes && pPipe)
		NotifyListeners(pPipe->GetSubpipe(), event, true);
}

void CPipeUser::NotifyListeners( int goalPipeId, EGoalPipeEvent event )
{
	if ( !goalPipeId )
		return;

	// recursion?
	if ( bNotifyListenersLock )
		return;

	bNotifyListenersLock = true;

	TMapGoalPipeListeners::iterator it, itEnd = m_mapGoalPipeListeners.end();
	for ( it = m_mapGoalPipeListeners.find( goalPipeId ); it != itEnd && it->first == goalPipeId; ++it )
	{
		std::pair< IGoalPipeListener*, const char* > & second = it->second;
		second.first->OnGoalPipeEvent( this, event, goalPipeId );
	}

	bNotifyListenersLock = false;
}

void CPipeUser::RegisterGoalPipeListener( IGoalPipeListener* pListener, int goalPipeId, const char* debugClassName )
{
	// you can't add listeners while notifying them
	AIAssert( !bNotifyListenersLock );

	// zero means don't listen it
	AIAssert( goalPipeId );

	m_mapGoalPipeListeners.insert(std::make_pair( goalPipeId, std::make_pair(pListener, debugClassName) ));

	pListener->_vector_registered_pipes.push_back( std::pair< IPipeUser*, int >( this, goalPipeId ) );
}

void CPipeUser::UnRegisterGoalPipeListener( IGoalPipeListener* pListener, int goalPipeId )
{
	// you can't remove listeners while notifying them
	AIAssert( !bNotifyListenersLock );

	IGoalPipeListener::VectorRegisteredPipes::iterator itFind = std::find(
		pListener->_vector_registered_pipes.begin(),
		pListener->_vector_registered_pipes.end(),
		std::pair< IPipeUser*, int >( this, goalPipeId ) );
	if ( itFind != pListener->_vector_registered_pipes.end() )
		pListener->_vector_registered_pipes.erase( itFind );

	TMapGoalPipeListeners::iterator it, itEnd = m_mapGoalPipeListeners.end();
	for ( it = m_mapGoalPipeListeners.find( goalPipeId ); it != itEnd && it->first == goalPipeId; ++it )
	{
		if ( it->second.first == pListener )
		{
			m_mapGoalPipeListeners.erase( it );

			// listeners shouldn't register them twice!
#ifdef _DEBUG
			itEnd = m_mapGoalPipeListeners.end();
			for ( it = m_mapGoalPipeListeners.find( goalPipeId ); it != itEnd && it->first == goalPipeId; ++it )
				AIAssert( it->second.first != pListener );
#endif

			return;
		}
	}
	//assert( !"Trying to unregister not registered listener" );
}
/*
void CPipeUser::OnGoalPipeEvent( IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId )
{
	AIAssert ( this == pPipeUser );

	switch ( event )
	{
	case ePN_Deselected: // sent if replaced by selecting other pipe
	case ePN_Finished:   // sent if reached end of pipe
	case ePN_Removed:    // sent if inserted pipe was removed with RemovePipe()
		{
			AIAssert(m_pendingNavSOStates.IsEmpty() && m_currentNavSOStates.IsEmpty());
			m_eNavSOMethod = nSOmNone;
			m_navSOEarlyPathRegen = false;
		}
		break;

	case ePN_Suspended:  // sent if other pipe was inserted
	case ePN_Resumed:    // sent if resumed after finishing inserted pipes
		break;
	}
}
*/

//
//-------------------------------------------------------------------------------------------
bool CPipeUser::IsUsing3DNavigation()
{
	bool can3D = (m_movementAbility.pathfindingProperties.navCapMask & (IAISystem::NAV_FLIGHT | IAISystem::NAV_VOLUME | IAISystem::NAV_WAYPOINT_3DSURFACE)) != 0;
	bool can2D = (m_movementAbility.pathfindingProperties.navCapMask & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_FREE_2D | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_LAYERED_NAV_MESH)) != 0;

	if (can3D && !can2D)
		return true;
	if (can2D && !can3D)
		return false;

	bool use3D = false;
	if (!m_Path.Empty() && m_movementAbility.b3DMove)
	{
		const PathPointDescriptor *current = m_Path.GetPrevPathPoint();
		const PathPointDescriptor *next = m_Path.GetNextPathPoint();
		bool current3D = true;
		bool next3D = true;
		if (current)
		{
			m_CurrentNodeNavType = current->navType;
			if ((current->navType & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_ROAD))
				|| current->navType == IAISystem::NAV_UNSET)
				current3D = false;
		}
		if (next)
		{
			if ((next->navType & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_ROAD))
				|| next->navType == IAISystem::NAV_UNSET)
				next3D = false;
		}
		use3D = (current3D || next3D);
	}
	else 
	{
		if(m_CurrentNodeNavType==IAISystem::NAV_UNSET)
		{ 
			int nBuildingID;
			IVisArea *pArea;
			m_CurrentNodeNavType = gAIEnv.pNavigation->CheckNavigationType(GetPhysicsPos(), nBuildingID, pArea, m_movementAbility.pathfindingProperties.navCapMask);
		}

		if (!((m_CurrentNodeNavType & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_ROAD | IAISystem::NAV_FREE_2D))
			|| m_CurrentNodeNavType == IAISystem::NAV_UNSET))
			use3D = true;
	}
	return use3D;
}
//
//-------------------------------------------------------------------------------------------
void CPipeUser::DebugDrawGoals()
{
	// Debug draw all active goals.
	for (size_t i = 0; i < m_vActiveGoals.size(); i++)
	{
		if (m_vActiveGoals[i].pGoalOp)
			m_vActiveGoals[i].pGoalOp->DebugDraw(this);
	}
}

//====================================================================
// SetPathToFollow
//====================================================================
void CPipeUser::SetPathToFollow(const char *pathName)
{
  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("CPipeUser::SetPathToFollow %s path = %s", GetName(), pathName);
	m_pathToFollowName = pathName;
	m_bPathToFollowIsSpline = false;
}

void CPipeUser::SetPathAttributeToFollow(bool bSpline)
{
	m_bPathToFollowIsSpline = bSpline;
}

//===================================================================
// GetPathEntryPoint
//===================================================================
bool CPipeUser::GetPathEntryPoint(Vec3& entryPos, bool reverse, bool startNearest) const
{
  if (gAIEnv.CVars.DebugPathFinding)
    AILogAlways("CPipeUser::GetPathEntryPoint %s path = %s", GetName(), m_pathToFollowName.c_str());

	if (m_pathToFollowName.length() == 0)
		return false;

	SShape path1;
	if (!gAIEnv.pNavigation->GetDesignerPath(m_pathToFollowName.c_str(), path1))
  {
    AIWarning("CPipeUser::GetPathEntryPoint %s unable to find path %s - check path is not marked as a road", GetName(), m_pathToFollowName.c_str());
		return false;
  }
	if (path1.shape.empty())
		return false;

	if (startNearest)
	{
		float	d;
		path1.NearestPointOnPath(GetPhysicsPos(), d, entryPos);
	}
	else if (reverse)
	{
		entryPos = *path1.shape.rbegin();
	}
	else
	{
		entryPos = *path1.shape.begin();
	}

	return true;
}

//====================================================================
// UsePathToFollow
//====================================================================
bool CPipeUser::UsePathToFollow(bool reverse, bool startNearest, bool loop)
{
  if (m_pathToFollowName.length() == 0)
    return false;
  ClearPath("CPipeUser::UsePathToFollow m_Path");
  SShape path1;
	if (!gAIEnv.pNavigation->GetDesignerPath(m_pathToFollowName.c_str(), path1))
    return false;
  if (path1.shape.empty())
    return false;

	IAISystem::ENavigationType	navType(path1.navType);

	if(startNearest)
	{
		float	d;
		Vec3	nearestPoint;
		ListPositions::const_iterator nearest = path1.NearestPointOnPath(GetPhysicsPos(), d, nearestPoint);

		m_Path.PushBack(PathPointDescriptor(navType, nearestPoint));

		if(reverse)
		{
			ListPositions::const_reverse_iterator rnearest(nearest);
			ListPositions::const_reverse_iterator	rbegin(path1.shape.rbegin());
			ListPositions::const_reverse_iterator	rend(path1.shape.rend());
			for (ListPositions::const_reverse_iterator it = rnearest; it != rend; ++it)
			{
				const Vec3 &pos = *it;
				m_Path.PushBack(PathPointDescriptor(navType, pos));
			}
			if(loop)
			{
				for (ListPositions::const_reverse_iterator it = rbegin; it != rnearest; ++it)
				{
					const Vec3 &pos = *it;
					m_Path.PushBack(PathPointDescriptor(navType, pos));
				}
				m_Path.PushBack(PathPointDescriptor(navType, nearestPoint));
			}
		}
		else
		{
			for (ListPositions::const_iterator it = nearest; it != path1.shape.end(); ++it)
			{
				const Vec3 &pos = *it;
				m_Path.PushBack(PathPointDescriptor(navType, pos));
			}
			if(loop)
			{
				for (ListPositions::const_iterator it = path1.shape.begin(); it != nearest; ++it)
				{
					const Vec3 &pos = *it;
					m_Path.PushBack(PathPointDescriptor(navType, pos));
				}
				m_Path.PushBack(PathPointDescriptor(navType, nearestPoint));
			}
		}
	}
	else
	{
		if (reverse)
		{
			for (ListPositions::reverse_iterator it = path1.shape.rbegin() ; it != path1.shape.rend() ; ++it)
			{
				const Vec3 &pos = *it;
				m_Path.PushBack(PathPointDescriptor(navType, pos));
			}
		}
		else
		{
			for (ListPositions::const_iterator it = path1.shape.begin() ; it != path1.shape.end() ; ++it)
			{
				const Vec3 &pos = *it;
				m_Path.PushBack(PathPointDescriptor(navType, pos));
			}
		}
	}

	// convert ai path to be splined
	// this spline is not enough one and allows a possibility that a conversion fails
	// 29/10/2006 Tetsuji
	if ( m_bPathToFollowIsSpline )
	{
		ConvertPathToSpline( navType );
		return true;
	}

  SNavPathParams params;
  params.precalculatedPath = true;
  m_Path.SetParams(params);

  m_nPathDecision = PATHFINDER_PATHFOUND;
  m_Path.PushFront(PathPointDescriptor(navType, GetPhysicsPos()));
  m_OrigPath = m_Path;
  return true;
}

//===================================================================
// ConvertPathToSpline
//===================================================================
bool CPipeUser::ConvertPathToSpline( IAISystem::ENavigationType	navType )
{

	if ( m_Path.GetPath().empty() )
		return false;

	TPathPoints::const_iterator li,liend;
	li		= m_Path.GetPath().begin();
	liend	= m_Path.GetPath().end();

	std::list<Vec3> pointListLocal;

	while ( li != liend )
	{
		pointListLocal.push_back( li->vPos );
		++li;
	}

	m_Path.Clear("CPipeUser::ConvertPathToSpline");
	SetPointListToFollow( pointListLocal, navType, true );

	m_OrigPath.Clear("CPipeUser::ConvertPathBySpline");

	SNavPathParams params;
	params.precalculatedPath = true;
	m_Path.SetParams(params);

	m_nPathDecision = PATHFINDER_PATHFOUND;
	m_OrigPath = m_Path;

	return true;

}

//-------------------------------------------------------------------------------------------------
SHideSpotInfo::EHideSpotType CPipeUser::GetCurrentHideObjectPos(Vec3 &hidePos, Vec3 &objectDir)
{
	if(!m_CurrentHideObject.IsValid())
	{
		return SHideSpotInfo::eHST_INVALID;
	}
	CRY_ASSERT(m_CurrentHideObject.GetHideSpotType() != SHideSpotInfo::eHST_INVALID);

	hidePos = m_CurrentHideObject.m_vLastHidePos;
	objectDir = m_CurrentHideObject.GetObjectDir();
	CRY_ASSERT(!objectDir.IsZero());

	return m_CurrentHideObject.GetHideSpotType();
}

//-------------------------------------------------------------------------------------------------

Vec3 CPipeUser::SetPointListToFollowSub( Vec3 a, Vec3 b, Vec3 c, std::list<Vec3>& newPointList, float step )
{
	float ideallength = 4.0f;

	if ( m_bPathToFollowIsSpline == true )
		ideallength = 1.0f;

	// calculate normalized spline(2) ( means each line has the same length )
	// x(1-t)^2 + 2yt(1-t)+ zt^2

	// when we suppose x=(0,0,0) then spline(2) is below
	// 2yt(1-t) +zt^2 =s
	// (z-2y)t^t + 2yt - s = 0;

	// to get t from a length
	// t = -2y +- sqrt( 4y^2 - 4(z-2y)s ) / 2(z-2y)

	// these 3 points (a,b,c) makes a spline(2)
	// if there is a problem, 
	// point a will be skipped then ( b, c, d ) will be calculated next time.

	Vec3 oa = (a + b) / 2.0f;
	Vec3 ob = b;
	Vec3 oc = (b + c) / 2.0f;

	Vec3 s = ob - oa;	// src
	Vec3 d = oc - oa;	// destination

	// when points are ideantical
	if ( s.GetLength() < 0.0001f )
		return b;

	// when (0,0,0),d and s make line, we just pass these point to the path system

	float	y,z,len;
	Vec3	nl =  s * ideallength / s.GetLength();

	for ( int i = 0; i< 3; i++ )
	{
		// select a stable element for the calculation.
		if (i==0)
			y = s.x, z = d.x, len =nl.x;
		if (i==1)
			y = s.y, z = d.y, len =nl.y;
		if (i==2)
			y = s.z, z = d.z, len =nl.z;
		
		// solve a formula for get t from length (ax*x + bx + c = 0)
		float ca = ( z - 2.0f * y );
		float cb = 2.0f * y;
		float cc = -len;

		float sq = cb * cb  - 4.0f * ca * cc ;
		if ( sq >= 0.0f && ca != 0.0f )
		{
			sq = sqrtf( sq );
			float t1 = ( -cb + sq ) / ( 2.0f * ca );
			float t2 = ( -cb - sq ) / ( 2.0f * ca );
			float t = ( t1 > 0.0f && t1 < step ) ? t1 : t2;

			if ( t > 0.0f && t < step )
			{
				for ( float u = 0.0f; u < step; u += t )
				{
					Vec3 np = 2.0f * u * ( 1.0f - u ) * s + u * u * d ;
					np += oa;
					newPointList.push_back( np );
				}
				if ( !newPointList.empty() )
				{
					Vec3 ret = newPointList.back();
					ret = ( ret  - c ) * 2.0f + c;
					newPointList.pop_back();
					return ret;
				}
			}
		}
	}
	newPointList.push_back( oa );
	return b;

}

//===================================================================
// SetPointListToFollow
//===================================================================
void CPipeUser::SetPointListToFollow( const std::list<Vec3>& pointList,IAISystem::ENavigationType navType,bool bSpline )
{
	ClearPath("CPipeUser::SetPointListToFollow m_Path");

	static bool debugsw = false;

	if ( debugsw == true )
	{
		for ( std::list<Vec3>::const_iterator it = pointList.begin(); it != pointList.end() ; ++it )
		{
			const Vec3 &pos = *it;
			m_Path.PushBack(PathPointDescriptor(navType, pos));
		}
		return;
	}

	// To guarantee at least 3 points for B-Spline(2)
	int	howmanyPoints = pointList.size();

	if ( howmanyPoints < 2)
		return;

	std::list<Vec3>::const_iterator itLocal;

	if ( howmanyPoints == 2 || bSpline == false )
	{
		for ( itLocal = pointList.begin(); itLocal != pointList.end() ; ++itLocal )
		{
			const Vec3 &pos = *itLocal;
			m_Path.PushBack(PathPointDescriptor(navType, pos));
		}
		return;
	}

	std::list<Vec3>::const_iterator itX = pointList.begin();
	std::list<Vec3>::const_iterator itY = itX;
	std::list<Vec3>::const_iterator itZ = itX;

	++itY;
	++itZ,++itZ;

	Vec3 nextStart = *itX;
	Vec3 mid = *itY;
	Vec3 end = *itZ;

	nextStart = ( nextStart - mid ) *2.0f + mid;

	std::list<Vec3> pointListLocal;

	// divde each line

	for ( int i = 0 ; i < howmanyPoints - 3 ; ++itX,++itY,++itZ,i++ )
	{
		pointListLocal.clear();
		nextStart = SetPointListToFollowSub( nextStart, *itY, *itZ, pointListLocal, 1.0f );

		for ( itLocal = pointListLocal.begin(); itLocal != pointListLocal.end() ; ++itLocal )
		{
			const Vec3 &pos = *itLocal;
			m_Path.PushBack(PathPointDescriptor(navType, pos));
		}
	}

	// at the last point, we need special treatment to make the curve reach the end point.
	mid = *itY;
	end = *itZ;
	end = ( end - mid ) * 2.0f + mid;

	pointListLocal.clear();
	nextStart = SetPointListToFollowSub( nextStart, *itY, end, pointListLocal, 1.0f );
	if ( pointListLocal.empty() )
	{
		const Vec3 &pos = nextStart;
		m_Path.PushBack(PathPointDescriptor(navType, pos));
	}
	else
	{
		for ( itLocal = pointListLocal.begin(); itLocal != pointListLocal.end() ; ++itLocal )
		{
			const Vec3 &pos = *itLocal;
			m_Path.PushBack(PathPointDescriptor(navType, pos));
		}
	}

	// terminate the end point ( skip if the final point of the spline is close to the end )
	{
		const Vec3 &pos = *itZ;
		if ( ( pos - nextStart ).GetLength() > 1.0f )
			m_Path.PushBack(PathPointDescriptor(navType, pos));
	}

	//m_Path.PushFront(PathPointDescriptor(navType, GetPhysicsPos()));
}

//===================================================================
// UsePointListToFollow
//===================================================================
bool CPipeUser::UsePointListToFollow( void )
{
	m_OrigPath.Clear("CPipeUser::UsePointListToFollow m_OrigPath");

	SNavPathParams params;
	params.precalculatedPath = true;
	m_Path.SetParams(params);

	m_nPathDecision = PATHFINDER_PATHFOUND;
	m_OrigPath = m_Path;

	return true;

}
//-------------------------------------------------------------------------------------------------
void CPipeUser::SetFireMode(EFireMode mode)
{
//	m_fireAtLastOpResult = shootLastOpResult;
	m_outOfAmmoSent = false;
	m_wasReloading = false;
	if(m_fireMode == mode)
		return;
	m_fireMode = mode;
	m_fireModeUpdated = true;
	m_spreadFireTime = ai_frand()*10.0f;
}

//-------------------------------------------------------------------------------------------------
void CPipeUser::SetFireTarget(CWeakRef<CAIObject> refTargetObject)
{
	m_refFireTarget = refTargetObject;
}

//-------------------------------------------------------------------------------------------------
void CPipeUser::SetRefPointPos( const Vec3& pos )
{
	CCCPOINT(CPipeUser_SetRefPointPos);

	CreateRefPoint();
	CAIObject *pRefPoint = m_refRefPoint.GetAIObject();
	AIAssert(pRefPoint);

	static bool bSetRefPointPosLock = false;
	bool bNotify = !bSetRefPointPosLock && m_pCurrentGoalPipe && !pRefPoint->GetPos().IsEquivalent( pos, 0.001f );
	pRefPoint->SetPos(pos, GetMoveDir());

	if ( GetAISystem()->IsRecording( this, IAIRecordable::E_REFPOINTPOS ) )
	{
		static char buffer[32];
		sprintf(buffer,"<%.0f %.0f %.0f>", pos.x, pos.y, pos.z);
		GetAISystem()->Record(this, IAIRecordable::E_REFPOINTPOS, buffer);
	}

	RecorderEventData recorderEventData(pos);
	RecordEvent(IAIRecordable::E_REFPOINTPOS, &recorderEventData);

	if ( bNotify )
	{
		bSetRefPointPosLock = true;
		NotifyListeners( m_pCurrentGoalPipe->GetLastSubpipe(), ePN_RefPointMoved );
		bSetRefPointPosLock = false;
	}
}

void CPipeUser::SetRefPointPos( const Vec3& pos, const Vec3& dir )
{
	CCCPOINT(CPipeUser_SetRefPointPos_Dir);

	CreateRefPoint();
	CAIObject *pRefPoint = m_refRefPoint.GetAIObject();
	AIAssert(pRefPoint);

	static bool bSetRefPointPosDirLock = false;
	bool bNotify = !bSetRefPointPosDirLock && m_pCurrentGoalPipe && !pRefPoint->GetPos().IsEquivalent( pos, 0.001f );
	pRefPoint->SetPos( pos, dir );

	if ( GetAISystem()->IsRecording( this, IAIRecordable::E_REFPOINTPOS ) )
	{
		static char buffer[32];
		sprintf(buffer,"<%.0f %.0f %.0f>", pos.x, pos.y, pos.z);
		GetAISystem()->Record(this, IAIRecordable::E_REFPOINTPOS, buffer);
	}

	RecorderEventData recorderEventData(pos);
	RecordEvent(IAIRecordable::E_REFPOINTPOS, &recorderEventData);

	if ( bNotify )
	{
		bSetRefPointPosDirLock = true;
		NotifyListeners( m_pCurrentGoalPipe->GetLastSubpipe(), ePN_RefPointMoved );
		bSetRefPointPosDirLock = false;
	}
}

//
//---------------------------------------------------------------------------------------------------------------------------
void CPipeUser::IgnoreCurrentHideObject(float timeOut)
{
	const Vec3&	pos(m_CurrentHideObject.GetObjectPos());

	// Check if the object is already in the list (should not).
	TimeOutVec3List::const_iterator end(m_recentUnreachableHideObjects.end());
	for (TimeOutVec3List::const_iterator it = m_recentUnreachableHideObjects.begin(); it != end; ++it)
	{
		if(Distance::Point_PointSq(it->second, pos) < sqr(0.1f))
			return;
	}

	// Add the object pos to the list.
	m_recentUnreachableHideObjects.push_back(FloatVecPair(timeOut, pos));

	while (m_recentUnreachableHideObjects.size() > 5)
		m_recentUnreachableHideObjects.pop_front();
}

//
//---------------------------------------------------------------------------------------------------------------------------
bool CPipeUser::WasHideObjectRecentlyUnreachable(const Vec3& pos) const
{
	TimeOutVec3List::const_iterator end(m_recentUnreachableHideObjects.end());
	for(TimeOutVec3List::const_iterator it = m_recentUnreachableHideObjects.begin(); it != end; ++it)
	{
		if(Distance::Point_PointSq(it->second, pos) < sqr(0.1f))
			return true;
	}
	return false;
}


//
//---------------------------------------------------------------------------------------------------------------------------
int CPipeUser::SetLooseAttentionTarget(CWeakRef<CAIObject> refObject,int id)
{	

	CAIObject *pObject = refObject.GetAIObject();
	if(pObject || id == m_looseAttentionId || id==-1)
	{
		m_bLooseAttention = pObject!=NULL;
		if(m_bLooseAttention)
		{
			m_refLooseAttentionTarget = refObject;
			++m_looseAttentionId ;
		}
		else 
		{
			m_refLooseAttentionTarget.Reset();
		}
	}
	return m_looseAttentionId;
}

//
//---------------------------------------------------------------------------------------------------------------------------
void CPipeUser::SetLookStyle(ELookStyle eLookTargetStyle)
{	
	m_State.eLookStyle = eLookTargetStyle;
}

//---------------------------------------------------------------------------------------------------------------------------
ELookStyle CPipeUser::GetLookStyle()
{	
	return m_State.eLookStyle;
}

//
//---------------------------------------------------------------------------------------------------------------------------
int	CPipeUser::CountGroupedActiveGoals()
{
int count(0);

	for (size_t i = 0; i < m_vActiveGoals.size(); i++)
	{
		QGoal& goal = m_vActiveGoals[i];
		if (goal.eGrouping == IGoalPipe::eGT_GROUPED && goal.op != eGO_WAIT)
		 ++count;
	}
	return count;
}

//
//---------------------------------------------------------------------------------------------------------------------------
void	CPipeUser::ClearGroupedActiveGoals()
{
	for (size_t i = 0; i < m_vActiveGoals.size(); i++)
	{
		QGoal& goal = m_vActiveGoals[i];
		if (goal.eGrouping == IGoalPipe::eGT_GROUPED && goal.op != eGO_WAIT)
		{
			RemoveActiveGoal(i);
			if(!m_vActiveGoals.empty())
				--i;
		}
	}
}


//
//----------------------------------------------------------------------------------------------
void CPipeUser::SetRefShapeName(const char* shapeName)
{
	// Set and resolve the shape name.
	m_refShapeName = shapeName;
	m_refShape = GetAISystem()->GetGenericShapeOfName(m_refShapeName.c_str());
	if(!m_refShape)
		m_refShapeName.clear();
}

//
//----------------------------------------------------------------------------------------------
const char* CPipeUser::GetRefShapeName() const
{
	return m_refShapeName.c_str();
}

//
//----------------------------------------------------------------------------------------------
SShape*	CPipeUser::GetRefShape()
{
	return m_refShape;
}

//
//----------------------------------------------------------------------------------------------
void	CPipeUser::RecordSnapshot()
{
	// Currently not used
}

//
//----------------------------------------------------------------------------------------------
void	CPipeUser::RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData)
{
#ifdef CRYAISYSTEM_DEBUG
	CRecorderUnit *pRecord = (CRecorderUnit*)GetAIDebugRecord();
	if(pRecord!=NULL)
	{
		pRecord->RecordEvent(event, pEventData);
	}
#endif //CRYAISYSTEM_DEBUG
}

//===================================================================
// GetPathFollower
//===================================================================
IPathFollower *CPipeUser::GetPathFollower()
{
  // only create this if appropriate (human - set by config etc)
  if (!m_pPathFollower && m_movementAbility.usePredictiveFollowing)
  {
		// Set non-default values (most get overridden during movement anyhow)
		PathFollowerParams params;
		params.navCapMask = m_movementAbility.pathfindingProperties.navCapMask;
		params.passRadius = m_movementAbility.pathfindingProperties.radius;
		params.pathRadius = m_movementAbility.pathRadius;
		params.pathLookAheadDist = m_movementAbility.pathLookAhead;	// Obsolete
		params.maxSpeed = 0.0f;																			// Obsolete
		params.stopAtEnd = true;
		params.use2D = !m_movementAbility.b3DMove;

		m_pPathFollower = CreatePathFollower(params);
		assert(m_pPathFollower);
		if (m_pPathFollower)
		{
			m_pPathFollower->AttachToPath(&m_Path);
		}
  }
  return m_pPathFollower;
}

//===================================================================
// CreatePathFollower
//===================================================================
IPathFollower* CPipeUser::CreatePathFollower(const PathFollowerParams &params)
{
	return new CPathFollower(params);
}

//
//----------------------------------------------------------------------------------------------
EAimState CPipeUser::GetAimState() const
{
	return m_aimState;
}

void CPipeUser::ClearInvalidatedSOLinks()
{
	m_invalidatedSOLinks.clear();
}

void CPipeUser::InvalidateSOLink( CSmartObject* pObject, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper ) const
{
	m_invalidatedSOLinks.insert(std::make_pair( pObject, std::make_pair(pFromHelper, pToHelper) ));
}

bool CPipeUser::IsSOLinkInvalidated( CSmartObject* pObject, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper ) const
{
	return m_invalidatedSOLinks.find(std::make_pair( pObject, std::make_pair(pFromHelper, pToHelper) )) != m_invalidatedSOLinks.end();
}

void CPipeUser::SetActorTargetRequest(const SAIActorTargetRequest& req)
{
	CAIObject*	pTarget = GetOrCreateSpecialAIObject(AISPECIAL_ANIM_TARGET);

	if(!m_pActorTargetRequest)
		m_pActorTargetRequest = new SAIActorTargetRequest;
	*m_pActorTargetRequest = req;
	pTarget->SetPos(m_pActorTargetRequest->approachLocation, m_pActorTargetRequest->approachDirection);
}

const SAIActorTargetRequest* CPipeUser::GetActiveActorTargetRequest() const
{
	CAIObject *pPathFindTarget = m_refPathFindTarget.GetAIObject();
	if( pPathFindTarget && pPathFindTarget->GetSubType() == STP_ANIM_TARGET && m_pActorTargetRequest)
		return m_pActorTargetRequest;
	return 0;
}

void CPipeUser::ClearActorTargetRequest()
{
	SAFE_DELETE(m_pActorTargetRequest);
}

