/******************************************************************** 
StalTech Source File.
Copyright (C), StalTech Studios, 2008-2009.
-------------------------------------------------------------------------
File name:   GoalOp_Crysis2.cpp
Description: Crysis2 goalops
             These should move into GameDLL when interfaces allow!
-------------------------------------------------------------------------
History:
- 18:11:2009 - Created by Márcio Martins
*********************************************************************/


#include "StdAfx.h"
#include "GoalOp_Crysis2.h"

#include "PipeUser.h"
#include "Puppet.h"
#include "DebugDrawContext.h"
#include "HideSpot.h"


#include "Communication/CommunicationManager.h"


namespace COPCrysis2Utils
{
	//
	//-------------------------------------------------------------------------------------------------------------
	static void TurnBodyToTargetInCover(CPipeUser* pPipeUser, const Vec3& targetPos)
	{
		Vec2 vBodyDir2D(pPipeUser->GetBodyDir());
		vBodyDir2D.Normalize();
		Vec2 vTargetDir2D(targetPos - pPipeUser->GetPhysicsPos());
		bool bLeft = vBodyDir2D.Cross(vTargetDir2D) >= 0;

		const char* szDir = bLeft ? "Lft" : "Rgt";
		pPipeUser->GetProxy()->SetAGInput(AIAG_COVERBODYDIR, szDir);
	}
}

//
//-------------------------------------------------------------------------------------------------------------
IGoalOp * CGoalOpFactoryCrysis2::GetGoalOp( const char * sGoalOpName, IFunctionHandler *pH, int firstParam, GoalParameters &params ) const
{
	EGoalOperations op = CGoalPipe::GetGoalOpEnum(sGoalOpName);

	switch (op)
	{
	case eGO_USECOVER:
		{
			params.nValue = 0;
			params.nValueAux = 0;
			params.bValue = 0;

			pH->GetParam(firstParam, params.nValue);				// ECoverUsage

			if (pH->GetParamCount() > firstParam)
				pH->GetParam(firstParam+1, params.nValueAux);	// ECoverUsageLocation

			if (pH->GetParamCount() > firstParam+1)
				pH->GetParam(firstParam+2, params.bValue);		// UseLastOpAsBackup
		}
		break;
	case eGO_ADJUSTAIM:
		{
			params.nValue = 0;
			params.fValue = 0.0f; // timeout
			int	hide = 0;
			int	useLastOpResultAsBackup = 0;
			int allowProne = 0;

			pH->GetParam(firstParam, hide);
			if (pH->GetParamCount() > firstParam)
				pH->GetParam(firstParam+1, params.fValue);	// timeout
			if (pH->GetParamCount() > firstParam+1)
				pH->GetParam(firstParam+2, useLastOpResultAsBackup);
			if (pH->GetParamCount() > firstParam+2)
				pH->GetParam(firstParam+3, allowProne);

			if (hide)
				params.nValue |= 0x1;

			if (useLastOpResultAsBackup)
				params.nValue |= 0x2;

			if (allowProne)
				params.nValue |= 0x4;
		}
		break;
	case eGO_HIDE:
		{
			params.nValue = AI_REG_HIDEOBJECT;
			params.bValue = true;

			pH->GetParam(firstParam, params.nValue); // location
			if (pH->GetParamCount() > firstParam)
				pH->GetParam(firstParam+1, params.bValue);	// exact
		}
		break;
	case eGO_COMMUNICATE:
		{
			const char* commName = 0;
			const char* channelName = 0;

			pH->GetParam(firstParam, commName);						// name
			pH->GetParam(firstParam + 1, channelName);		// channel
			pH->GetParam(firstParam + 2, params.fValue);	// expirity

			if (pH->GetParamCount() > firstParam + 2) 
				pH->GetParam(firstParam + 3, params.bValue); // ordering (0 == Ordered // 1 == Unordered)
		
			if (pH->GetParamCount() > firstParam + 3) 
			{
				params.fValueAux = AI_REG_NONE;
				pH->GetParam(firstParam + 4, params.fValueAux); // target register (using a float here since the range is short)
			}

			params.nValue = gAIEnv.pCommunicationManager->GetCommunicationID(commName);
			params.nValueAux = gAIEnv.pCommunicationManager->GetChannelID(channelName);
		}
		break;
	default:
		return NULL;
	}

	return GetGoalOp( op, params );
}

//
//-------------------------------------------------------------------------------------------------------------
IGoalOp * CGoalOpFactoryCrysis2::GetGoalOp( EGoalOperations op, GoalParameters &params ) const
{
	IGoalOp *pResult = NULL;

	switch (op)
	{
	case eGO_USECOVER:
		{
			pResult = new COPCrysis2UseCover( (COPCrysis2UseCover::ECoverUsage)params.nValue, (ECoverUsageLocation)params.nValueAux, params.bValue );
		}
		break;
	case eGO_ADJUSTAIM:
		{
			pResult = new COPCrysis2AdjustAim((params.nValue & 0x01) != 0, (params.nValue & 0x02) != 0, (params.nValue & 0x04) != 0, params.fValue);
		}
		break;
	case eGO_HIDE:
		{
			pResult = new COPCrysis2Hide(static_cast<EAIRegister>(params.nValue), params.bValue);
		}
		break;
	case eGO_COMMUNICATE:
		{
			pResult = new COPCrysis2Communicate(params.nValue, params.nValueAux, params.fValue, static_cast<EAIRegister>(int(params.fValueAux)),
				params.bValue ? SCommunicationRequest::Unordered : SCommunicationRequest::Ordered);
		}
		break;
	default:
		return NULL;
	}

	return pResult;
}


//
//-------------------------------------------------------------------------------------------------------------
COPCrysis2UseCover::COPCrysis2UseCover(ECoverUsage usage, ECoverUsageLocation location, bool useLastOpAsBackup):
	m_coverUsage(usage),
	m_coverUsageLocation(location),
	m_coverMoveState(eCMS_None),
	m_useLastOpAsBackup(useLastOpAsBackup), 
	m_coverCompromised(false),
	m_targetReached(false),
	m_leftInvalidDist(FLT_MAX),
	m_rightInvalidDist(-FLT_MAX),
	m_moveStartTime(0.0f),
	m_leftInvalidStartTime(0.0f),
	m_rightInvalidStartTime(0.0f),
	m_pTracer(0)
{
	if (m_coverUsageLocation == eCUL_None)
		m_coverUsageLocation = eCUL_Automatic;
}

COPCrysis2UseCover::COPCrysis2UseCover(const XmlNodeRef& node) :
	m_coverUsage(s_xml.GetBool(node, "hide", true) ? COPCrysis2UseCover::eCU_Hide : COPCrysis2UseCover::eCU_UnHide),
	m_coverUsageLocation(eCUL_None),
	m_coverMoveState(eCMS_None),
	m_useLastOpAsBackup(s_xml.GetBool(node, "useLastOpAsBackup")), 
	m_coverCompromised(false),
	m_targetReached(false),
	m_leftInvalidDist(FLT_MAX),
	m_rightInvalidDist(-FLT_MAX),
	m_moveStartTime(0.f),
	m_leftInvalidStartTime(0.f),
	m_rightInvalidStartTime(0.f),
	m_pTracer(0)
{
	s_xml.GetCoverLocation(node, "location", m_coverUsageLocation);
	if (m_coverUsageLocation == eCUL_None)
	{
		m_coverUsageLocation = eCUL_Automatic;
	}
}

COPCrysis2UseCover::~COPCrysis2UseCover()
{
	SAFE_DELETE(m_pTracer);
}

//
//-------------------------------------------------------------------------------------------------------------
void COPCrysis2UseCover::Reset(CPipeUser* pPipeUser)
{
/*	if (pPipeUser && !pPipeUser->m_CurrentHideObject.IsSmartObject())
	{
		pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
		pPipeUser->m_State.lean = 0.0f;
		pPipeUser->m_State.peekOver = 0.0f;
	}
*/
	if (pPipeUser && pPipeUser->m_CurrentHideObject.IsSmartObject())
		return;

	m_leftInvalidStartTime.SetSeconds(0.0f);
	m_rightInvalidStartTime.SetSeconds(0.0f);
	m_moveStartTime.SetSeconds(0.0f);
	m_coverCompromised = false;
	m_targetReached = false;
	m_leftInvalidDist = FLT_MAX;
	m_rightInvalidDist = -FLT_MAX;
	m_coverMoveState = eCMS_None;

	if (m_pTracer)
		m_pTracer->Reset(pPipeUser);
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCrysis2UseCover::Execute(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!pPipeUser->m_CurrentHideObject.IsValid())
	{
		Reset(pPipeUser);

		return eGOR_FAILED;
	}

	if (m_targetReached)
	{
		//Reset(pPipeUser);

		return eGOR_SUCCEEDED;
	}
/*
	if (pPipeUser->m_CurrentHideObject.IsSmartObject())
		return UseCoverSO(pPipeUser);
	else
			*/

	// TODO(Marcio): Add some timeout to avoid getting stuck
	return UseCover(pPipeUser);
}
/*
//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCrysis2UseCover::UseCoverSO(CPipeUser* pPipeUser)
{
	if (!m_targetReached)
	{
		// on first call of Execute just return false
		// this will prevent calling Execute twice in
		// only one Update in the beginning of goalop
		m_targetReached = true;
		return AIGOR_IN_PROGRESS;
	}

	if(m_bUnHide)
		return UseCoverSOUnHide(pPipeUser);
	else
		return UseCoverSOHide(pPipeUser);
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCrysis2UseCover::UseCoverSOUnHide(CPipeUser* pPipeUser)
{
//	if(!pPipeUser->m_CurrentHideObject.IsUsingCover())
//		return false;

	IEntity* pUser = pPipeUser->GetEntity();
	IEntity* pObject = pPipeUser->m_CurrentHideObject.GetSmartObject().pObject->GetEntity();
	int id = gAIEnv.pSmartObjectManager->SmartObjectEvent( "Unhide", pUser, pObject );
	if ( !id )
	{
		pPipeUser->m_CurrentHideObject.Invalidate();
		pPipeUser->SetCoverCompromised();
		m_fTimeOut = -1.0f;
		return AIGOR_FAILED;
	}

	pPipeUser->m_CurrentHideObject.SetUsingCover( false );
	return AIGOR_IN_PROGRESS;
}
//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCrysis2UseCover::UseCoverSOHide(CPipeUser* pPipeUser)
{
//	if(pPipeUser->m_CurrentHideObject.IsUsingCover())
//		return false;

	IEntity* pUser = pPipeUser->GetEntity();
	IEntity* pObject = pPipeUser->m_CurrentHideObject.GetSmartObject().pObject->GetEntity();
	int id = gAIEnv.pSmartObjectManager->SmartObjectEvent( "Hide", pUser, pObject );
	if ( !id )
	{
		pPipeUser->m_CurrentHideObject.Invalidate();
		pPipeUser->SetCoverCompromised();
		m_fTimeOut = -1.0f;
		return AIGOR_FAILED;
	}

	pPipeUser->m_CurrentHideObject.SetUsingCover( true );
	return AIGOR_IN_PROGRESS;
}
*/


void COPCrysis2UseCover::UpdateCoverLocations(CPipeUser* pPipeUser, bool useLowCover, const Vec3& target, ECoverUsage coverUsage)
{
	float leftEdge;
	float rightEdge;
	float leftUmbra;
	float rightUmbra;

	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;
	hideObject.GetCoverDistances(useLowCover, target, m_coverCompromised, leftEdge, rightEdge, leftUmbra, rightUmbra);

	float left = 0.0f;
	float right = 0.0f;

	const float offset = 0.1f + pPipeUser->GetParameters().m_fPassRadius * 1.35f;

//	if (coverUsage == eCU_Hide)
	{
		left = max(leftEdge, leftUmbra) + offset;
		right = min(rightEdge, rightUmbra) - offset;
	}
	//else if (coverUsage == eCU_UnHide)
/*	{
		float maxUmbraDisplace = 1.5f;
		if ((fabsf(leftUmbra) - fabsf(leftEdge)) > maxUmbraDisplace)
			leftUmbra = leftEdge - maxUmbraDisplace;
		
		if ((fabsf(rightUmbra) - fabsf(rightEdge)) > maxUmbraDisplace)
			rightUmbra = rightEdge + maxUmbraDisplace;

		left = min(leftEdge, leftUmbra) + offset;
		right = max(rightEdge, rightUmbra) - offset;
	}
*/
	m_leftEdge = hideObject.GetPointAlongCoverPath(left);
	m_rightEdge = hideObject.GetPointAlongCoverPath(right);
	m_center = hideObject.GetPointAlongCoverPath(0.5f * (leftEdge + rightEdge));

	CTimeValue now = GetAISystem()->GetFrameStartTime();

	float leftInvalidTime = (now - m_leftInvalidStartTime).GetSeconds();
	float rightInvalidTime = (now - m_rightInvalidStartTime).GetSeconds();

	if (leftInvalidTime > 0.01f)
	{
		if(left < m_leftInvalidDist)
			m_leftEdge += hideObject.GetCoverPathDir() * (m_leftInvalidDist - left);

		if(right < m_leftInvalidDist)
			m_rightEdge += hideObject.GetCoverPathDir() * (m_leftInvalidDist - right);
	}

	if(rightInvalidTime > 0.01f)
	{
		if(left > m_rightInvalidDist)
			m_leftEdge += hideObject.GetCoverPathDir() * (m_rightInvalidDist - left);

		if(right > m_rightInvalidDist)
			m_rightEdge += hideObject.GetCoverPathDir() * (m_rightInvalidDist - right);
	}
}


//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCrysis2UseCover::UseCover(CPipeUser* pPipeUser)
{
	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	if(pPipeUser->m_CurrentHideObject.IsSmartObject())
		return eGOR_FAILED;

	CPuppet* pPuppet = pPipeUser->CastToCPuppet();
	if (!pPuppet)
		return eGOR_FAILED;

	CoverUsageInfo coverUsageInfo;

	if (!hideObject.IsCoverPathComplete())
	{
		hideObject.HurryUpCoverPathGen();

		return eGOR_IN_PROGRESS;
	}

	if ((m_coverMoveState == eCMS_None) && (m_coverUsageLocation == eCUL_Automatic))
	{
		if (pPipeUser->GetCoverUsageInfo(coverUsageInfo) != AsyncComplete)
			return eGOR_IN_PROGRESS;
	}

	IAIObject* pTarget = pPipeUser->GetAttentionTarget();
	if (!pTarget)
	{
		if(m_useLastOpAsBackup)
			pTarget = pPipeUser->GetLastOpResult();

		if (!pTarget)
		{
			Reset(pPipeUser);

			return eGOR_FAILED;
		}
	}

	CTimeValue now = GetAISystem()->GetFrameStartTime();

	const Vec3&	pos = pPipeUser->GetPhysicsPos();
	const Vec3& targetPos = pTarget->GetPos();
	
	UpdateInvalidSeg(pPipeUser);

	if (fabsf(m_leftInvalidDist) > hideObject.GetMaxCoverPathLen() * 0.5f)
		m_leftInvalidStartTime = now;
	
	if (fabsf(m_rightInvalidDist) > hideObject.GetMaxCoverPathLen() * 0.5f)
		m_rightInvalidStartTime = now;

	if (m_coverMoveState == eCMS_None)
	{
		m_moveTarget = pos;
		m_coverCompromised = false;

		bool useLowCover = hideObject.HasLowCover();

		UpdateCoverLocations(pPipeUser, useLowCover, targetPos, m_coverUsage);

		if (!hideObject.IsNearCover(pPipeUser))
			m_coverCompromised = true;

		ECoverUsageLocation location = m_coverUsageLocation;
		if (location == eCUL_Automatic)
			location = GetAutomaticCoverLocationChoice(pPipeUser, coverUsageInfo);
		
		if (location == eCUL_None)
			m_coverCompromised = true;
		else
			m_moveTarget = GetCoverLocationPosition(location);

		if (!m_coverCompromised)
		{
			m_pose = m_moveTarget + Vec3(0.0f, 0.0f, 1.5f);

			Vec3	dir = m_moveTarget - pos;
			float	dist = dir.GetLength2D();

			float	moveTargetOnLine = hideObject.GetDistanceAlongCoverPath(m_moveTarget);
			float	posOnLine = hideObject.GetDistanceAlongCoverPath(pos);
			float	error = posOnLine - moveTargetOnLine;

			ECoverHeight coverHeight = eCH_None;

			if (fabsf(error) > 0.05f)
			{
				coverHeight = GetCoverHeight(pPipeUser, targetPos, pos, m_moveTarget);

				m_moveStartTime = now;
				m_coverMoveState = eCMS_Moving;

				CNavPath& path = pPipeUser->m_Path;
				path.Clear("OPUseCover Move");
				path.PushBack(PathPointDescriptor(IAISystem::NAV_UNSET, pos));
				path.PushBack(PathPointDescriptor(IAISystem::NAV_UNSET, m_moveTarget));

				SNavPathParams params;
				params.start = pos;
				params.end = m_moveTarget;
				params.allowDangerousDestination = true;
				params.continueMovingAtEnd = false;
				params.endDistance = 0.0f;
				params.precalculatedPath = true;
				path.SetParams(params);

				if (!m_pTracer)
					m_pTracer = new COPTrace(true, 0.0f);

				if (m_pTracer->Execute(pPipeUser) == eGOR_SUCCEEDED)
				{
					m_coverMoveState = eCMS_Moving;
					m_targetReached = true;
				}
			}
			else
			{
				coverHeight = GetCoverHeightAt(pPipeUser, targetPos, pos);

				COPCrysis2Utils::TurnBodyToTargetInCover(pPipeUser, targetPos);

				m_targetReached = true;
			}

			if (coverHeight != eCH_None)
			{
				switch (coverHeight)
				{
				case eCH_Low:
					pPipeUser->SetSignal(AISIGNAL_DEFAULT, "OnMoveInLowCover", pPipeUser->GetEntity(), 0, 
						gAIEnv.SignalCRCs.m_nOnMoveInLowCover);
					break;
				case eCH_High:
					pPipeUser->SetSignal(AISIGNAL_DEFAULT, "OnMoveInHighCover", pPipeUser->GetEntity(), 0,
						gAIEnv.SignalCRCs.m_nOnMoveInHighCover);
					break;
				default:
					assert(0);
					break;
				}
			}
		}
	}

	if(m_coverMoveState != eCMS_None)
	{
		float	moveElapsed = (now - m_moveStartTime).GetSeconds();

		ExecuteDry(pPipeUser);

		if (moveElapsed > 5.0f)
		{
			m_coverMoveState = eCMS_None;
			m_coverCompromised = true;
		}
	}

	float leftInvalidTime = (m_leftInvalidStartTime.GetMilliSeconds() > 0) && (now - m_leftInvalidStartTime).GetSeconds();
	float rightInvalidTime = (m_leftInvalidStartTime.GetMilliSeconds() > 0) && (now - m_rightInvalidStartTime).GetSeconds();

	if ((leftInvalidTime > 3.0f) && (rightInvalidTime > 3.0f))
		m_coverCompromised = true;

	if (!hideObject.IsValid())
		m_coverCompromised = true;

	if (m_coverCompromised)
	{
		pPipeUser->SetCoverCompromised();

		hideObject.Invalidate();

		return eGOR_FAILED;
	}

	pPipeUser->SetBodyTargetDir(hideObject.GetObjectDir());

	return eGOR_IN_PROGRESS;
}

void COPCrysis2UseCover::ExecuteDry(CPipeUser* pPipeUser)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if(pPipeUser->m_CurrentHideObject.IsSmartObject())
		return;

	if(!pPipeUser->m_CurrentHideObject.IsCoverPathComplete())
		return;

	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	CPuppet* pPuppet = pPipeUser->CastToCPuppet();
	if(!pPuppet)
		return;

	const Vec3&	pos = pPipeUser->GetPhysicsPos();

	if (m_coverMoveState == eCMS_Moving)
	{/*
		float	moveTargetOnLine = hideObject.GetDistanceAlongCoverPath(m_moveTarget);
		float	posOnLine = hideObject.GetDistanceAlongCoverPath(pos);
		float	error = cry_fabsf(posOnLine - moveTargetOnLine);

		pPuppet->AdjustMovementUrgency(pPipeUser->m_State.fMovementUrgency, error, &m_maxPathLen);

		Vec3 dir = m_moveTarget - pos;
		float	dist = dir.GetLength2D();
		
		dir.z = 0.0f;
		dir.NormalizeSafe();

		float	speed = clamp(0.5f + (dist * 0.666f), 0.0f, 1.0f);
		float normalSpeed, minSpeed, maxSpeed;
		pPipeUser->GetMovementSpeedRange(pPipeUser->m_State.fMovementUrgency, pPipeUser->m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

		if (error < 0.1f)
		{
			m_targetReached = true;
			m_coverMoveState = eCMS_None;

			pPipeUser->m_State.fDesiredSpeed = 0.0f;
			pPipeUser->m_State.vMoveDir = ZERO;
			pPipeUser->m_State.fDistanceToPathEnd = 0.0f;
		}
		else
		{
			if (error < 0.3f)
				speed *= 0.75f;

			pPipeUser->m_State.fDesiredSpeed = speed * normalSpeed;
			pPipeUser->m_State.vMoveDir = dir;
			pPipeUser->m_State.fDistanceToPathEnd = dist;

			pPipeUser->m_DEBUGmovementReason = CPipeUser::AIMORE_MOVE;
		}
		*/
		if (m_pTracer)
		{
			if (m_pTracer->ExecuteTrace(pPipeUser, true))
			{
				m_targetReached = true;
				m_coverMoveState = eCMS_None;

				delete m_pTracer;
				m_pTracer = 0;
			}
		}
	}
}

IAIObject* COPCrysis2UseCover::GetTarget(CPipeUser* pPipeUser) const
{
	IAIObject* pTarget = pPipeUser->GetAttentionTarget();
	if (!pTarget)
	{
		if(m_useLastOpAsBackup)
			pTarget = pPipeUser->GetLastOpResult();
	}

	return pTarget;
}

float COPCrysis2UseCover::ScoreByLocationDistance(CPipeUser* pPipeUser, const Vec3& pos, const Vec3& locationPos) const
{
	float distance = (locationPos - pos).len(); 
	float maxDistance = (m_leftEdge - m_rightEdge).len();
	if (maxDistance < distance)
		maxDistance = distance;

	assert(distance <= maxDistance);

	float score = 1.0f;
	if (maxDistance >= 0.1f)
		score = 1.0f - min(1.0f, distance / maxDistance);

	return score;
}

float COPCrysis2UseCover::ScoreByDistanceToTarget(CPipeUser* pPipeUser, const Vec3& locationPos, const Vec3& target) const
{
	float locationToTarget = (locationPos - target).len();

	const float ConsiderClose = 10.0f;
	const float ConsiderFar = 25.0f;

	bool isClose = (locationToTarget < ConsiderClose);
	bool isFar = (locationToTarget > ConsiderFar);

	float score = 1.0f;
	if (isClose)
		score = (locationToTarget / ConsiderClose);
	else if (isFar)
		score = 1.0f - max(1.0f, (locationToTarget - ConsiderFar) / ConsiderFar);

	return score;
}

float COPCrysis2UseCover::ScoreByAngle(CPipeUser* pPipeUser, const Vec3& locationPos, const Vec3& target) const
{
	Vec3 locationToTargetDir = (target - locationPos).GetNormalized();
	float angleCos = locationToTargetDir.Dot(pPipeUser->m_CurrentHideObject.GetObjectDir());
	if (angleCos <= 0.1f)
		return 0.0f;

	return angleCos;
}

float COPCrysis2UseCover::ScoreByCoverage(CPipeUser* pPipeUser, bool hasLowCover, bool hasHighCover, ECoverUsage coverUsage, ECoverUsageLocation location) const
{
	float coverageScore = 0.0f;

	if (coverUsage == eCU_UnHide)
	{
		if (hasHighCover && (location == eCUL_Center))
			coverageScore = -1.0f;
		else
			coverageScore = 1.0f;
	}
	else if (coverUsage == eCU_Hide)
	{
		coverageScore += hasHighCover ? 0.5f : 0.0f;
		coverageScore += hasLowCover ? 0.5f : 0.0f;
	}

	return coverageScore;
}

const Vec3& COPCrysis2UseCover::GetCoverLocationPosition(ECoverUsageLocation location) const
{
	switch (location)
	{
	case eCUL_Left:
		return m_leftEdge;
	case eCUL_Right:
		return m_rightEdge;
	case eCUL_Center:
		return m_center;
	default:
		assert(0);
		break;
	}

	return Vec3Constants<float>::fVec3_Zero;
}

float COPCrysis2UseCover::GetCoverLocationScore(CPipeUser* pPipeUser, const CoverUsageInfo& coverUsageInfo,
																								ECoverUsage coverUsage, ECoverUsageLocation location) const
{
	IAIObject* pTarget = GetTarget(pPipeUser);
	if (!pTarget)
		return 0.0f;

	float score = 0.0f;
	bool hasLowCover = false;
	bool hasHighCover = false;

	Vec3 locationPos = GetCoverLocationPosition(location);

	switch (location)
	{
	case eCUL_Left:
		hasLowCover = coverUsageInfo.lowLeft;
		hasHighCover = coverUsageInfo.highLeft;
		break;
	case eCUL_Right:
		hasLowCover = coverUsageInfo.lowRight;
		hasHighCover = coverUsageInfo.highRight;
		break;
	case eCUL_Center:
		hasLowCover = coverUsageInfo.lowCenter;
		hasHighCover = coverUsageInfo.highCenter;
		break;
	default:
		assert(0);
		break;
	}

	const Vec3& pos = pPipeUser->GetPos();
	const Vec3& target = pTarget->GetPos();

	if (!hasLowCover && !hasHighCover)
		return 0.0f;

	switch (coverUsage)
	{
	case eCU_Hide:
		{
			score += 0.5f * ScoreByLocationDistance(pPipeUser, pos, locationPos);
			//score += 0.0f * ScoreByAngle(pPipeUser, locationPos, target);
			score += 0.4f * ScoreByDistanceToTarget(pPipeUser, locationPos, target);
			score += 0.1f * ScoreByCoverage(pPipeUser, hasLowCover, hasHighCover, coverUsage, location);
		}
		break;
	case eCU_UnHide:
		{
			score += 0.4f * ScoreByLocationDistance(pPipeUser, pos, locationPos);
			//score += 0.0f * ScoreByAngle(pPipeUser, locationPos, target);
			score += 0.4f * ScoreByDistanceToTarget(pPipeUser, locationPos, target);
			score += 0.2f * ScoreByCoverage(pPipeUser, hasLowCover, hasHighCover, coverUsage, location);
		}
		break;
	default:
		break;
	}

	return score;
}

ECoverUsageLocation COPCrysis2UseCover::GetAutomaticCoverLocationChoice(CPipeUser* pPipeUser, 
																																				const CoverUsageInfo& coverUsageInfo)
{
	IAIObject* pTarget = GetTarget(pPipeUser);
	if (!pTarget)
		return eCUL_None;

	LocationScores& locationScores = m_locationScores;
	locationScores.clear();

	locationScores.push_back(CoverLocationScore(eCUL_Left,
		GetCoverLocationScore(pPipeUser, coverUsageInfo, m_coverUsage, eCUL_Left)));
	locationScores.push_back(CoverLocationScore(eCUL_Center, 
		GetCoverLocationScore(pPipeUser, coverUsageInfo, m_coverUsage, eCUL_Center)));
	locationScores.push_back(CoverLocationScore(eCUL_Right, 
		GetCoverLocationScore(pPipeUser, coverUsageInfo, m_coverUsage, eCUL_Right)));

	std::sort(locationScores.begin(), locationScores.end());

	if (m_coverUsage == eCU_UnHide)
	{
		CPuppet* pPuppet = pPipeUser->CastToCPuppet();

		uint32 locationCount = locationScores.size();
		for (uint32 i = 0; i < locationCount; ++i)
		{
			const CoverLocationScore& best = locationScores[i];
			if (best.score > 0.0f)
			{
				if (pPuppet)
				{
					IPuppet::SPostureInfo posture;
					if (m_coverUsage == eCU_UnHide)
					{
						uint32 checks = CPuppet::DefaultPostureChecks;
						if (pPuppet->GetFireMode() == FIREMODE_OFF)
							checks &= ~CPuppet::CheckPostureAimability;

						if (pPuppet->SelectAimPostureAt(&posture, GetCoverLocationPosition(best.location),
							pTarget->GetPos(), checks, true))
							return best.location;
					}
				}
			}
		}
	}
	else
	{
		const CoverLocationScore& best = locationScores.front();
		if (best.score > 0.0f)
			return best.location;
	}

	return eCUL_None;
}

//
//-------------------------------------------------------------------------------------------------------------
COPCrysis2UseCover::ECoverHeight COPCrysis2UseCover::GetCoverHeight(CPipeUser* pPipeUser, const Vec3& target, const Vec3& from, const Vec3& to)
{
	// TODO(marcio): Think of a better way!
	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	float fromDistance = hideObject.GetDistanceAlongCoverPath(from);
	float toDistance = hideObject.GetDistanceAlongCoverPath(to);
	float delta = toDistance - fromDistance;
	float fraction = cry_fabsf(pPipeUser->GetParameters().m_fPassRadius / delta);

	uint32 highCount = 0;
	uint32 lowCount = 0;

	float curr = -fraction;
	while(curr < 1.0f)
	{
		curr += fraction;
		if (curr > 1.0f)
			curr = 1.0f;

		bool hasLow = false;
		bool hasHigh = false;

		float distance = fromDistance + curr * delta;

		hideObject.GetCoverHeightAlongCoverPath(distance, target, hasLow, hasHigh);

		highCount += hasHigh ? 1 : 0;
		lowCount += hasLow ? 1 : 0;
	}

	if (highCount >= lowCount)
		return eCH_High;
	else if ((cry_fabsf(delta) > 2.0f) && (highCount >= lowCount*0.75f))
		return eCH_High;
	else
		return eCH_Low;

	return eCH_None;
}

COPCrysis2UseCover::ECoverHeight COPCrysis2UseCover::GetCoverHeightAt(CPipeUser* pPipeUser, const Vec3& target, const Vec3& pos)
{
	// TODO(marcio): Think of a better way!
	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	bool hasLow = false;
	bool hasHigh = false;

	float distance = hideObject.GetDistanceAlongCoverPath(pos);
	hideObject.GetCoverHeightAlongCoverPath(distance, target, hasLow, hasHigh);

	if (hasHigh)
		return eCH_High;
	else if (hasLow)
		return eCH_Low;

	return eCH_None;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPCrysis2UseCover::UpdateInvalidSeg(CPipeUser* pPipeUser)
{
	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	Vec3 pos = pPipeUser->GetPhysicsPos();
	float distAlongCoverPath = hideObject.GetDistanceAlongCoverPath(pos);
	float	radius = pPipeUser->GetParameters().m_fPassRadius + 0.3f;

	m_leftInvalidDist = -1000.0f;
	m_rightInvalidDist = 1000.0f;

	// Loop through the nearby puppets and check if they are invalidating some of the cover path.
	CAISystem::AIObjectOwners::iterator aiEnd = GetAISystem()->m_Objects.end();

	for (AutoAIObjectIter it(GetAISystem()->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET)); it->GetObject(); it->Next())
	{
		// Skip every object that is not a puppet.
		CPuppet* pPuppet = it->GetObject()->CastToCPuppet();
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
void COPCrysis2UseCover::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPCrysis2UseCover");
	{
		ser.Value("m_targetReached",m_targetReached);
		ser.Value("m_coverCompromised",m_coverCompromised);
		ser.Value("m_center",m_center);
		ser.Value("m_leftEdge",m_leftEdge);
		ser.Value("m_rightEdge",m_rightEdge);
		ser.Value("m_pose",m_pose);
		ser.Value("m_moveTarget",m_moveTarget);
		ser.Value("m_moveStartTime",m_moveStartTime);
		ser.Value("m_useLastOpAsBackup",m_useLastOpAsBackup);
		ser.Value("m_leftInvalidStartTime",m_leftInvalidStartTime);
		ser.Value("m_leftInvalidDist",m_leftInvalidDist);
		ser.Value("m_leftInvalidPos",m_leftInvalidPos);
		ser.Value("m_rightInvalidStartTime",m_rightInvalidStartTime);
		ser.Value("m_rightInvalidDist",m_rightInvalidDist);
		ser.Value("m_rightInvalidPos",m_rightInvalidPos);

		ser.EnumValue("m_coverUsage", m_coverUsage, eCU_Hide, eCU_UnHide);
		ser.EnumValue("m_coverUsageLocation", m_coverUsageLocation, eCUL_None, eCUL_Center);
		ser.EnumValue("m_coverMoveState", m_coverMoveState, eCMS_None, eCMS_Moving);
	}

	ser.EndGroup();
}

//
//-------------------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------------------
void COPCrysis2UseCover::DebugDraw(CPipeUser* pPipeUser) const
{
	if (!pPipeUser)
		return;

	CAIHideObject& hideObject = pPipeUser->m_CurrentHideObject;

	if (!hideObject.IsCoverPathComplete())
		return;

	CDebugDrawContext dc;

	// Hide pos
	dc->DrawCone(m_center + Vec3(0, 0, 0.3f), Vec3(0, 0, -1), 0.1f, 0.3f, ColorB(255, 0, 0));
	
	// Unhide pos
	dc->DrawCone(m_leftEdge + Vec3(0, 0, 0.3f), Vec3(0, 0, -1), 0.1f, 0.3f, ColorB(0, 255, 0));
	dc->DrawCone(m_rightEdge + Vec3(0, 0, 0.3f), Vec3(0, 0, -1), 0.1f, 0.3f, ColorB(0, 255, 0));

	// Pos
	dc->DrawSphere(m_pose, 0.2f, ColorB(255, 255, 255));

	// Draw invalid segments.
	CTimeValue now = GetAISystem()->GetFrameStartTime();
	float leftInvalidTime = (now - m_leftInvalidStartTime).GetSeconds();
	float rightInvalidTime = (now - m_rightInvalidStartTime).GetSeconds();

	if(leftInvalidTime > 0.001f)
		dc->DrawArrow(m_leftInvalidPos + Vec3(0, 0, 0.25f), -hideObject.GetCoverPathDir() * (0.25f + leftInvalidTime), 0.3f, ColorB(255, 0, 0, 200));

	if(rightInvalidTime > 0.001f)
		dc->DrawArrow(m_rightInvalidPos + Vec3(0, 0, 0.25f), hideObject.GetCoverPathDir() * (0.25f + rightInvalidTime), 0.3f, ColorB(255, 0, 0, 200));

	static string text;

	switch (m_coverUsage)
	{
	case eCU_Hide:
		text = "HIDE";
		break;
	case eCU_UnHide:
		text = "UNHIDE";
		break;
	default:
		assert(0);
		break;
	}

	text += " ";

	switch (m_coverUsageLocation)
	{
	case eCUL_None:
		text += "($4None$o)";
		break;
	case eCUL_Automatic:
		text += "(Automatic)";
		break;
	case eCUL_Left:
		text += "(Left)";
		break;
	case eCUL_Right:
		text += "(Right)";
		break;
	case eCUL_Center:
		text += "(Center)";
		break;
	default:
		assert(0);
		break;
	}

	if (m_coverMoveState == eCMS_Moving)
	{
		static string formatter;
		formatter.Format(" $4Moving$o [%.2fs]", (now - m_moveStartTime).GetSeconds());
		text += formatter;
	}

	const Vec3 agentHead = pPipeUser->GetPos();
	const ColorB white(255, 255, 255, 255);
	const float	fontSize = 1.25f;

	float x, y, z;
	dc->ProjectToScreen(agentHead.x, agentHead.y, agentHead.z, &x, &y, &z);
	if ((z < 0.0f) || (z > 1.0f))
		return;

	x *= dc->GetWidth() * 0.01f;
	y *= dc->GetHeight() * 0.01f;

	y += 100.0f;
	dc->Draw2dLabel(x, y, fontSize, white, true, "%s", text.c_str());
}




//
//-------------------------------------------------------------------------------------------------------------
COPCrysis2AdjustAim::COPCrysis2AdjustAim(bool hide, bool useLastOpAsBackup, bool allowProne, float timeOut) :
	m_hide(hide),
	m_useLastOpAsBackup(useLastOpAsBackup),
	m_allowProne(allowProne),
	m_timeOut(timeOut),
	m_bestPostureId(-1)
{
	m_startTime.SetMilliSeconds(0);
	m_nextUpdate = 0.0f;
}

//
//-------------------------------------------------------------------------------------------------------------
COPCrysis2AdjustAim::COPCrysis2AdjustAim(const XmlNodeRef& node) :
	m_hide(s_xml.GetBool(node, "hide")),
	m_useLastOpAsBackup(s_xml.GetBool(node, "useLastOpResultAsBackup")),
	m_allowProne(s_xml.GetBool(node, "allowProne")),
	m_timeOut(0.f),
	m_bestPostureId(-1)
{
	node->getAttr("timeout", m_timeOut);

	m_startTime.SetMilliSeconds(0);
	m_nextUpdate = RandomizeTimeInterval();
}

//
//-------------------------------------------------------------------------------------------------------------
float COPCrysis2AdjustAim::RandomizeTimeInterval()
{
	return 0.5f + ai_frand() * 0.25f;
}

//
//-------------------------------------------------------------------------------------------------------------
EGoalOpResult COPCrysis2AdjustAim::Execute( CPipeUser* pPipeUser )
{
	CCCPOINT(COPCrysis2AdjustAim_Execute);
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CPuppet* pPuppet = pPipeUser->CastToCPuppet();
	if(!pPuppet)
		return eGOR_FAILED;

	CTimeValue now(GetAISystem()->GetFrameStartTime());
	if (m_startTime.GetMilliSeconds() == 0)
		m_startTime = now;

	CAIObject *pLastOpResult = pPipeUser->m_refLastOpResult.GetAIObject();

	Vec3 targetPos;
	
	if(pPipeUser->GetAttentionTarget())
		targetPos = pPipeUser->GetAttentionTarget()->GetPos();
	else if(m_useLastOpAsBackup && pLastOpResult)
		targetPos = pLastOpResult->GetPos();
	else
		return eGOR_FAILED;

	float	distToTarget = Distance::Point_Point(targetPos, pPipeUser->GetPos());
	bool isMoving = (pPipeUser->GetVelocity().GetLengthSquared() > 0.1f) || (!pPipeUser->m_State.vMoveDir.IsZero() && pPipeUser->m_State.fDesiredSpeed > 0.01f);
	float	elapsed = (now - m_startTime).GetSeconds();

	SAIWeaponInfo	weaponInfo;
	pPipeUser->GetProxy()->QueryWeaponInfo(weaponInfo);

	if (elapsed >= m_nextUpdate && weaponInfo.isReloading)
		m_nextUpdate += 0.2f;

	if (isMoving)
	{
		int idx = MovementUrgencyToIndex(pPipeUser->m_State.fMovementUrgency);
		if (idx <= AISPEED_WALK)
		{
			if (elapsed >= m_nextUpdate)
			{
				IPuppet::SPostureInfo posture;
				bool stanceOk = false;

				if (m_hide)
					stanceOk = pPuppet->SelectHidePosture(&posture, targetPos, false, m_allowProne && pPipeUser->m_State.fDistanceToPathEnd < 1.3f);
				else
				{
					uint32 checks = CPuppet::DefaultPostureChecks;
					if (pPuppet->GetFireMode() == FIREMODE_OFF)
						checks &= ~CPuppet::CheckPostureAimability;

					stanceOk = pPuppet->SelectAimPosture(&posture, targetPos, checks, false, m_allowProne && pPipeUser->m_State.fDistanceToPathEnd < 1.3f);
				}

				if (stanceOk && gAIEnv.CVars.DrawGoals != 0)
					m_bestPostureId = pPuppet->GetPostureId(posture.name.c_str());
				else
					m_bestPostureId = -1;

				if (stanceOk)
					pPipeUser->m_State.bodystate = posture.stance;

				m_nextUpdate += RandomizeTimeInterval();
				if (pPipeUser->m_State.bodystate == STANCE_PRONE)
					m_nextUpdate += 2.0f;
			}
		}
		else
		{
			if(elapsed >= m_nextUpdate)
			{
				m_nextUpdate += 1.0f;

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
		if (elapsed >= m_nextUpdate)
		{
			bool stanceOk = false;
			IPuppet::SPostureInfo posture;
	
			if(m_hide)
				stanceOk = pPuppet->SelectHidePosture(&posture, targetPos, true, m_allowProne);
			else
			{
				uint32 checks = CPuppet::DefaultPostureChecks;
				if (pPuppet->GetFireMode() == FIREMODE_OFF)
					checks &= ~CPuppet::CheckPostureAimability;

				stanceOk = pPuppet->SelectAimPosture(&posture, targetPos, checks, true, m_allowProne);
			}

			if (stanceOk && gAIEnv.CVars.DrawGoals != 0)
				m_bestPostureId = pPuppet->GetPostureId(posture.name.c_str());
			else
				m_bestPostureId = -1;

			if(stanceOk)
			{
				pPipeUser->m_State.bodystate = posture.stance;
				pPipeUser->m_State.lean = posture.lean;
				pPipeUser->m_State.peekOver = posture.peekOver;

				if (!posture.agInput.empty())
				{
					pPipeUser->GetProxy()->SetAGInput(AIAG_ACTION, posture.agInput.c_str());

					if (posture.lean == 0.0f)
						COPCrysis2Utils::TurnBodyToTargetInCover(pPipeUser, targetPos);
				}
				else
				{
					pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
				}
			}
			else
			{
				// Could not find new pose, keep stance and remove peek.
				pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);
				pPipeUser->m_State.lean = 0;
				pPipeUser->m_State.peekOver = 0;
			}

			m_nextUpdate += RandomizeTimeInterval();
			if (pPipeUser->m_State.bodystate == STANCE_PRONE)
				m_nextUpdate += 2.0f;
		}
	}

	if ((m_timeOut > 0.0f) && (elapsed >= m_timeOut))
		return eGOR_SUCCEEDED;

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
void COPCrysis2AdjustAim::Reset( CPipeUser* pPipeUser )
{
	pPipeUser->m_State.lean = 0.0f;
	pPipeUser->m_State.peekOver = 0.0f;
	pPipeUser->GetProxy()->ResetAGInput(AIAG_ACTION);

	m_startTime.SetMilliSeconds(0);
	m_nextUpdate = 0.0f;
	m_bestPostureId = -1;
}

//
//-------------------------------------------------------------------------------------------------------------
void COPCrysis2AdjustAim::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPCrysis2AdjustAim");
	{
		ser.Value("m_startTime", m_startTime);	
		ser.Value("m_nextUpdate", m_nextUpdate);	
		ser.Value("m_hide", m_hide);	
		ser.Value("m_useLastOpAsBackup", m_useLastOpAsBackup);	
	}
	ser.EndGroup();
}


//
//-------------------------------------------------------------------------------------------------------------
void COPCrysis2AdjustAim::DebugDraw(CPipeUser* pPipeUser) const
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

	CPuppet* pPuppet = pPipeUser->CastToCPuppet();
	if (!pPuppet)
		return;

	static string text;

	if (m_hide)
		text = "HIDE ";
	else
		text = "AIM ";

	if (m_bestPostureId > -1)
	{
		IPuppet::SPostureInfo posture;
		pPuppet->GetPosture(m_bestPostureId, &posture);

		text += posture.name;
	}
	else
		text += "<$4No Posture$o>";

	const Vec3 agentHead = pPipeUser->GetPos();
	const ColorB white(255, 255, 255, 255);
	const float	fontSize = 1.25f;

	float x, y, z;
	dc->ProjectToScreen(agentHead.x, agentHead.y, agentHead.z, &x, &y, &z);
	if ((z < 0.0f) || (z > 1.0f))
		return;

	x *= dc->GetWidth() * 0.01f;
	y *= dc->GetHeight() * 0.01f;

	y += 100.0f;
	dc->Draw2dLabel(x, y, fontSize, white, true, "%s", text.c_str());
}

COPCrysis2Hide::COPCrysis2Hide(EAIRegister location, bool exact)
: m_location(location)
, m_exact(exact)
, m_pPathfinder(0)
, m_pTracer(0)
{
}

COPCrysis2Hide::COPCrysis2Hide(const XmlNodeRef& node) :
	m_location(AI_REG_NONE),
	m_exact(s_xml.GetBool(node, "exact", true)),
	m_pPathfinder(0),
	m_pTracer(0)
{
	s_xml.GetRegister(node, "register", m_location, CGoalOpXMLReader::MANDATORY);
}

COPCrysis2Hide::~COPCrysis2Hide()
{
	m_refHideTarget.Release();

	SAFE_DELETE(m_pPathfinder);
	SAFE_DELETE(m_pTracer);
}

EGoalOpResult COPCrysis2Hide::Execute(CPipeUser* pPipeUser)
{
	CCCPOINT(COPCrysis2Hide_Execute);

	CAISystem *pSystem = GetAISystem();

	if (m_refHideTarget.IsNil())
	{
		CCCPOINT(COPCrysis2Hide_Execute_NoHideTarget);

		pPipeUser->ClearPath("COPCrysis2Hide::Execute");

		if (m_location == AI_REG_REFPOINT)
		{
			CCCPOINT(COPCrysis2Hide_Execute_RefPoint);

			if (IAIObject* pRefPoint = pPipeUser->GetRefPoint())
			{
				// Setup the hide object.
				Vec3 pos = pRefPoint->GetPos();
				Vec3 dir = pRefPoint->GetBodyDir();

				if (dir.IsZero())
				{
					Vec3 target(pPipeUser->GetAttentionTarget() ? pPipeUser->GetAttentionTarget()->GetPos() : pos);
					dir = (target - pos).GetNormalizedSafe();
				}
			
				SHideSpot	hideSpot(SHideSpotInfo::eHST_ANCHOR, pos, dir);
				CAIHideObject& currentHideObject = pPipeUser->m_CurrentHideObject;
				currentHideObject.Set(&hideSpot, hideSpot.info.pos, hideSpot.info.dir);

				if (!currentHideObject.IsValid() ||
					!GetAISystem()->IsHideSpotOccupied(pPipeUser, hideSpot.info.pos))
				{
					Reset(pPipeUser);

					pPipeUser->SetInCover(false);

					return eGOR_FAILED;
				}
			
				pPipeUser->SetMovingToCover(true);

				CreateHideTarget(pPipeUser->GetName(), pos);
			}
		}
		else // AI_REG_HIDEOBJECT
		{
			CCCPOINT(COPCrysis2Hide_Execute_HideObject);

			if (pPipeUser->m_CurrentHideObject.IsValid())
			{
				// TODO: Here we can calculate which place in the cover is best to move to
				// instead of moving to the center first
				Vec3 pos = pPipeUser->m_CurrentHideObject.GetLastHidePos();

				pPipeUser->SetMovingToCover(true);

				CreateHideTarget(pPipeUser->GetName(), pos);
			}
			else
			{
				Reset(pPipeUser);

				pPipeUser->SetInCover(false);

				return eGOR_FAILED;
			}
		}

		// is it a smart object?
		if (pPipeUser->m_CurrentHideObject.GetSmartObject().pRule)
		{
			int id = GetAISystem()->AllocGoalPipeId();

			gAIEnv.pSmartObjectManager->UseSmartObject( pPipeUser->m_CurrentHideObject.GetSmartObject().pUser,
				pPipeUser->m_CurrentHideObject.GetSmartObject().pObject, pPipeUser->m_CurrentHideObject.GetSmartObject().pRule, id);

			pPipeUser->SetMovingToCover(false);

			return eGOR_SUCCEEDED;
		}

		pPipeUser->m_nPathDecision = PATHFINDER_STILLFINDING;
		pPipeUser->SetMovingToCover(true);
	}

	if (!m_refHideTarget.IsNil())
	{
		if (!m_pPathfinder)
		{
      if (gAIEnv.CVars.DebugPathFinding)
      {
				const Vec3 &vPos = m_refHideTarget->GetPos();
        AILogAlways("COPCrysis2Hide::Execute %s pathfinding to (%5.2f, %5.2f, %5.2f)", pPipeUser->GetName(),
          vPos.x, vPos.y, vPos.z);
      }

			m_pPathfinder = new COPPathFind("", m_refHideTarget.GetAIObject());

			pPipeUser->SetMovingToCover(true);
		}

		if (!m_pTracer && (m_pPathfinder->Execute(pPipeUser) != eGOR_IN_PROGRESS))
		{
			if (pPipeUser->m_nPathDecision == PATHFINDER_PATHFOUND)
			{
        if (gAIEnv.CVars.DebugPathFinding)
        {
					const Vec3 &vPos = m_refHideTarget->GetPos();
          AILogAlways("COPCrysis2Hide::Execute %s Creating trace to hide target (%5.2f, %5.2f, %5.2f)", pPipeUser->GetName(),
            vPos.x, vPos.y, vPos.z);
        }

				m_pTracer = new COPTrace(m_exact, 0.05f);
			}
			else
			{
				CCCPOINT(COPCrysis2Hide_Execute_Unreachable);

				// Could not reach the point, mark it ignored so that we do not try to pick it again.
				pPipeUser->IgnoreCurrentHideObject(10.0f);

				Reset(pPipeUser);

				return eGOR_FAILED;
			}
		}

		if (m_pTracer)
		{
			if (m_pTracer->Execute(pPipeUser) != eGOR_IN_PROGRESS)
			{
				CCCPOINT(COPCrysis2Hide_Execute_A);

				Reset(pPipeUser);

				pPipeUser->SetInCover(true);
				pPipeUser->SetSignal(1, "OnHideSpotReached", 0, 0, gAIEnv.SignalCRCs.m_nOnHideSpotReached);

				return eGOR_SUCCEEDED;
			}
			else
			{
				CCCPOINT(COPCrysis2Hide_Execute_B);

				pPipeUser->SetMovingToCover(true);
			}
		}
	}

	return eGOR_IN_PROGRESS;
}

void COPCrysis2Hide::ExecuteDry(CPipeUser* pPipeUser) 
{
	CCCPOINT(COPCrysis2Hide_ExecuteDry);

  if (m_pTracer)
    m_pTracer->ExecuteTrace(pPipeUser, false);
}

void COPCrysis2Hide::Reset(CPipeUser* pPipeUser)
{
	CCCPOINT(COPCrysis2Hide_Reset);

	SAFE_DELETE(m_pPathfinder);
	SAFE_DELETE(m_pTracer);

  if (pPipeUser)
  {
		pPipeUser->SetMovingToCover(false);

		pPipeUser->ClearPath("COPCrysis2Hide::Reset");
	}

	m_refHideTarget.Release();
}

void COPCrysis2Hide::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("COPCrysis2Hide");
	{
		m_refHideTarget.Serialize(ser, "m_refHideTarget");
 
		ser.EnumValue("m_location", m_location, AI_REG_NONE, AI_REG_LAST);
		ser.Value("m_exact", m_exact);

		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("Tracer", m_pTracer != NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pTracer->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
			if(ser.BeginOptionalGroup("Pathfinder", m_pPathfinder != NULL))
			{
				PREFAST_SUPPRESS_WARNING(6011) m_pPathfinder->Serialize(ser, objectTracker);
				ser.EndGroup();
			}		
		}
		else
		{
      SAFE_DELETE(m_pTracer);

			if(ser.BeginOptionalGroup("Tracer", true))
			{
				m_pTracer = new COPTrace(m_exact);
				m_pTracer->Serialize(ser, objectTracker);
				ser.EndGroup();
			}

      SAFE_DELETE(m_pPathfinder);

			if(ser.BeginOptionalGroup("Pathfinder", true))
			{
				m_pPathfinder = new COPPathFind("");
				m_pPathfinder->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}

		ser.EndGroup();
	}
}

void COPCrysis2Hide::CreateHideTarget(const char* name, const Vec3 &pos)
{
	CCCPOINT(COPCrysis2Hide_CreateHideTarget);

	assert(m_refHideTarget.IsNil());

	stack_string objectName(name);
	objectName += "_HideTarget";

	GetAISystem()->CreateDummyObject(m_refHideTarget, objectName);

	assert(m_refHideTarget.GetAIObject());

	m_refHideTarget.GetAIObject()->SetPos(pos);
}


COPCrysis2Communicate::COPCrysis2Communicate(
	CommunicationID commID, CommunicationChannelID channelID, float expirity, EAIRegister target, 
	SCommunicationRequest::EOrdering ordering)
	: m_commID(commID)
	, m_channelID(channelID)
	, m_ordering(ordering)
	, m_expirity(expirity)
	, m_target(target)
{
}

COPCrysis2Communicate::COPCrysis2Communicate(const XmlNodeRef& node) :
	m_expirity(0.f),
	m_target(AI_REG_NONE)
{
	const char* szName = s_xml.GetMandatoryString(node, "name");
	CCommunicationManager* pCommunicationManager = gAIEnv.pCommunicationManager;
	m_commID = pCommunicationManager->GetCommunicationID(szName);
	m_channelID = pCommunicationManager->GetChannelID(szName);
  
	m_ordering = s_xml.GetBool(node, "ordered") ? SCommunicationRequest::Ordered : SCommunicationRequest::Unordered;
	s_xml.GetMandatory(node, "expiry", m_expirity);
	s_xml.GetRegister(node, "register", m_target);
}

COPCrysis2Communicate::~COPCrysis2Communicate()
{
}

EGoalOpResult COPCrysis2Communicate::Execute(CPipeUser* pPipeUser)
{
	SCommunicationRequest request;
	request.actorID = pPipeUser->GetEntityID();
	request.commID = m_commID;
	request.channelID = m_channelID;
	request.configID = gAIEnv.pCommunicationManager->GetConfigID(pPipeUser->GetProxy()->GetCommunicationConfigName());
	request.contextExpirity = m_expirity;
	request.ordering = m_ordering;

	IAIObject* target = 0;

	switch (m_target)
	{
	case AI_REG_LASTOP:
		target = pPipeUser->GetLastOpResult();
		break;
	case AI_REG_ATTENTIONTARGET:
		target = pPipeUser->GetAttentionTarget();
		break;
	case AI_REG_REFPOINT:
		target = pPipeUser->GetRefPoint();
		break;
	case AI_REG_HIDEOBJECT:
		request.target = pPipeUser->m_CurrentHideObject.GetObjectPos();
		break;
	default:
		break;
	}

	if (target)
	{
		request.targetID = target->GetEntityID();
		if (!request.targetID)
			request.target = target->GetPos();
	}
	
	gAIEnv.pCommunicationManager->PlayCommunication(request);

	return eGOR_DONE;
}