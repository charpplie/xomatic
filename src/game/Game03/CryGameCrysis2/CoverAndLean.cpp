/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 4:9:2009: Created by Filipe Amim

*************************************************************************/
#include "StdAfx.h"
#include "GameCVars.h"
#include "CoverAndLean.h"
#include "CoverAndLeanDraw.h"
#include "CoverAndLeanMath.h"
#include "Player.h"
#include "Weapon.h"
#include "IItemSystem.h"
#include "GameCVars.h"
#include "PlayerMovementController.h"
#include "IWorldQuery.h"
#include "HUD/HUD.h"


uint32 CCoverAndLean::m_PhysicsCallbackRegistered = 0;


CCoverAndLean::SEdgeReference::SEdgeReference()
:	m_point(ZERO)
,	m_normal(ZERO)
,	m_tangent(ZERO)
,	m_leanMode(ELeanMode_NONE)
,	m_minVector(ZERO)
,	m_maxVector(ZERO)
,	m_minAngle(0.0f)
,	m_maxAngle(0.0f)
,	m_leftLimit(Vec3(ZERO), 0)
,	m_rightLimit(Vec3(ZERO), 0)
,	m_depth(0)
,	m_direction(ZERO)
{
	
}



CCoverAndLean::CCoverAndLean()
	:	m_pPlayer(0)
	,	m_pWeapon(0)
	,	m_leanAmountBias(0.1f)
	,	m_raiseAmountBias(0.1f)
	,	m_maxRaise(0.5f)
	,	m_maxLean(1.5f)
	,	m_leanAmount(0)
	,	m_leanDetectionApparture(1.25f)
	,	m_leaning(false)
	,	m_canLean(false)
	,	m_lastZoomed(false)
	,	m_maxDistanceToEdge(1.5f)
	,	m_leanBlend(0)
	,	m_viewFrontVec(ZERO)
	,	m_intersectionPoint(ZERO)
	,	m_viewLeanVec(ZERO)
{
	m_coverSurfaceIdx = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_cover")->GetId();

	if (!m_PhysicsCallbackRegistered)
	{
		gEnv->pPhysicalWorld->AddEventClient(EventPhysRWIResult::id, RWIResultCoverLean, 1);
	}

	++m_PhysicsCallbackRegistered;
}

CCoverAndLean::~CCoverAndLean()
{
	--m_PhysicsCallbackRegistered;

	if (!m_PhysicsCallbackRegistered)
	{
		gEnv->pPhysicalWorld->RemoveEventClient(EventPhysRWIResult::id, RWIResultCoverLean, 1);
	}
}


void CCoverAndLean::Reset(const IItemParamsNode* pRootNode)
{
  if(!pRootNode)
    return;

	const IItemParamsNode* pParamNode = pRootNode->GetChild("CoverLean");
	if (!pParamNode)
		return;

	pParamNode->GetAttribute("leanAmountBias", m_leanAmountBias);
	pParamNode->GetAttribute("raiseAmountBias", m_raiseAmountBias);
	pParamNode->GetAttribute("maxRaise", m_maxRaise);
	pParamNode->GetAttribute("maxLean", m_maxLean);
	float leanApperatureInDegrees = 45;
	pParamNode->GetAttribute("leanDetectionApparture", leanApperatureInDegrees);
	m_leanDetectionApparture = cry_tanf(DEG2RAD(leanApperatureInDegrees));
	pParamNode->GetAttribute("maxDistanceToEdge", m_maxDistanceToEdge);
}



void CCoverAndLean::Update(CPlayer* pPlayer)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_pPlayer = pPlayer;

	const Vec3 rayStart = GetViewPosition();
	const Vec3 rayDirection = m_pPlayer->GetEntity()->GetForwardDir();
	const float rayLength = m_maxDistanceToEdge;

	::DetectCoverBox(
		m_pPlayer, rayStart, rayDirection, rayLength, m_coverSurfaceIdx,
		&m_coverBox, &m_intersectionPoint);


	m_pWeapon = m_pPlayer->GetWeapon(m_pPlayer->GetCurrentItemId());
	if (!m_pWeapon)
		return;

	bool prevCanLean = m_canLean;
	bool prevLeaning = m_leaning;
	ELeanMode prevLeanMode = m_edgeReference.m_leanMode;

	if (m_pPlayer->GetStance() == STANCE_STAND)
		DetectLeanAroundReferences();
	else if (m_pPlayer->GetStance() == STANCE_CROUCH)
		DetectLeanOverReferences();
	m_canLean = UpdateLeanLogic();
	if (!m_canLean)
		m_leaning = false;

	SmoothLeaning();

	if(prevCanLean!=m_canLean || prevLeaning!=m_leaning || prevLeanMode!=m_edgeReference.m_leanMode)
		UpdateReadibility();

#ifndef _RELEASE
	DebugDrawEdgeReference(m_pPlayer, m_edgeReference, m_canLean, GetViewPosition(), m_viewFrontVec, m_viewLeanVec, m_leanAmount);
	DebugDrawCurrentBox(m_coverBox);
#endif
}



bool CCoverAndLean::UpdateLeanLogic()
{
	bool isZoomed = m_pWeapon->IsZoomed() || m_pWeapon->IsZoomingInOrOut();
	bool zooming = isZoomed && !m_lastZoomed;
	bool wasLeaning = m_leaning;
	m_lastZoomed = isZoomed;
	bool canLeanAround = false;
	if (m_leaning)
	{
		m_leaning = CanLean();
		canLeanAround = m_leaning;
	}
	else
	{
		canLeanAround = CanStartLean();
		if (!m_leaning && zooming && canLeanAround)
		{
			m_leaning = true;
		}
	}
	if (!isZoomed)
	{
		m_leaning = false;
	}
	SActorParams *pParams = m_pPlayer->GetActorParams();
	if (m_leaning)
	{
		pParams->vLimitDir = (m_edgeReference.m_minVector + m_edgeReference.m_maxVector).GetNormalized();
		pParams->vLimitRangeH = cry_acosf(m_edgeReference.m_minVector.Dot(m_edgeReference.m_maxVector))*0.5f;
		pParams->vLimitRangeVUp = m_edgeReference.m_maxAngle;
		pParams->vLimitRangeVDown = m_edgeReference.m_minAngle;
		pParams->speedMultiplier = 1.0f;
	}

	if (wasLeaning && !m_leaning)
		StopingLeaning();
	else
		UpdateWeaponRaise(canLeanAround, false);

	return canLeanAround;
}



void CCoverAndLean::UpdateReadibility()
{
	SHUDEvent leanEvent;
	leanEvent.eventType = eHUDEvent_OnLeanChanged;
	leanEvent.eventIntData = m_edgeReference.m_leanMode;
	leanEvent.eventIntData2 = m_canLean ? 1 : 0;
	leanEvent.eventIntData3 = m_leaning ? 1 : 0;
	CHUD::CallEvent(leanEvent);

	if (!CanStartLean())
		UpdateWeaponRaise(false, true);
}



void CCoverAndLean::UpdateWeaponRaise(bool raiseWeapon, bool noSwitchAnimation)
{
	if (!m_pWeapon->CanBeRaised() || m_pWeapon->IsDualWieldSlave())
		return;
	m_pWeapon->RaiseWeapon(raiseWeapon, noSwitchAnimation);
}



void CCoverAndLean::SmoothLeaning()
{
	const float leanBlendValue = m_leaning ? 1.0f : 0.0f;
	const float leanSmoothVelocity = 20.0f;
	Interpolate(m_leanBlend, leanBlendValue, leanSmoothVelocity, gEnv->pTimer->GetFrameTime());
}



void CCoverAndLean::DetectLeanAroundReferences()
{
	const Vec3 playerViewPosition = GetViewPosition();
	const Vec3 splitPlaneNormal = m_pPlayer->GetEntity()->GetWorldTM().GetColumn2();
	const Vec3 playerFrontVector = m_pPlayer->GetEntity()->GetForwardDir();
	const Vec3 playerRightVector = m_pPlayer->GetEntity()->GetWorldTM().GetColumn0();
	Plane splitPlane = Plane::CreatePlane(splitPlaneNormal, playerViewPosition);
	Plane playerPlane = Plane::CreatePlane(playerFrontVector, playerViewPosition);
	const float minCoverWidth = 1.0f;

	int closestFace = GetClosestBoxFaceIndex(m_coverBox, playerViewPosition);

	if (!m_leaning)
	{
		SSegment splitSegment;
		if (closestFace != -1 && SplitPolygon(m_coverBox.m_worldPolygons[closestFace], splitPlane, &splitSegment))
		{
			float rightDistance = 0;
			float leftDistance = 0;
			int rightIndex = 0;
			int leftIndex = 1;

			splitSegment.m_normal = HorizontalNormal(splitSegment.m_normal);
			Vec3 segmentTangent = Vec3(-splitSegment.m_normal.y, splitSegment.m_normal.x, 0);

			if (splitSegment.m_vertices[0].GetSquaredDistance(splitSegment.m_vertices[1]) > minCoverWidth*minCoverWidth)
			{
				float distance[2];
				Vec3 p = ClosestPointToPlane(playerPlane, splitSegment.m_vertices[0]);
				distance[0] = playerRightVector.Dot(p - playerViewPosition);
				p = ClosestPointToPlane(playerPlane, splitSegment.m_vertices[1]);
				distance[1] = playerRightVector.Dot(p - playerViewPosition);

				int vertexIndex = (abs(distance[1]) > abs(distance[0])) ? 0 : 1;
				m_edgeReference.m_point = splitSegment.m_vertices[vertexIndex];
				m_edgeReference.m_normal = splitSegment.m_normal;

				if (distance[0] < 0 && distance[1] < 0)
					m_edgeReference.m_leanMode = ELeanMode_LEAN_AROUND_RIGHT;
				else if (distance[0] > 0 && distance[1] > 0)
					m_edgeReference.m_leanMode = ELeanMode_LEAN_AROUND_LEFT;
				else
					m_edgeReference.m_leanMode = distance[vertexIndex] < 0 ? ELeanMode_LEAN_AROUND_LEFT : ELeanMode_LEAN_AROUND_RIGHT;

				m_edgeReference.m_tangent = m_edgeReference.m_leanMode == ELeanMode_LEAN_AROUND_LEFT ? segmentTangent : -segmentTangent;

				m_edgeReference.m_direction = Vec3(0, 0, 1);
				m_edgeReference.m_depth = GetBoxDepthFromFace(m_coverBox, closestFace);
			}
		}
	}

	Vec3 egdePoint = m_edgeReference.m_point;
	if (playerFrontVector.Dot(m_edgeReference.m_tangent) > 0)
		egdePoint -= m_edgeReference.m_normal * m_edgeReference.m_depth;

	m_viewFrontVec = m_pPlayer->GetEntity()->GetWorldRotation().GetColumn1();
	m_viewLeanVec = m_edgeReference.m_leanMode == ELeanMode_LEAN_AROUND_RIGHT ? playerRightVector : -playerRightVector;

	m_edgeReference.m_minVector = -m_edgeReference.m_normal - m_edgeReference.m_tangent*1.25f;
	m_edgeReference.m_minVector.Normalize();

	m_edgeReference.m_maxVector = -m_edgeReference.m_normal + m_edgeReference.m_tangent*0.45f;
	m_edgeReference.m_maxVector.Normalize();

	m_leanAmount = (egdePoint - playerViewPosition).Dot(m_viewLeanVec) + m_leanAmountBias;
	m_leanAmount += 0.1f;
	m_leanAmount = CLAMP(m_leanAmount, -m_maxLean*0.5f, m_maxLean);

	m_edgeReference.m_minAngle = DEG2RAD(-45.0f);
	m_edgeReference.m_maxAngle = DEG2RAD(45.0f);
}



void CCoverAndLean::DetectLeanOverReferences()
{
	const Vec3 playerViewPosition = GetViewPosition();
	const Vec3 viewDirection = m_pPlayer->GetViewQuat().GetColumn1();
	const Vec3 splitPlaneNormal = m_pPlayer->GetEntity()->GetWorldTM().GetColumn0();
	const Vec3 playerFrontVector = m_pPlayer->GetEntity()->GetForwardDir();
	const Vec3 horizontalSplitPlaneNormal = m_pPlayer->GetEntity()->GetWorldTM().GetColumn2();
	Plane splitPlane = Plane::CreatePlane(splitPlaneNormal, playerViewPosition);
	Plane playerPlane = Plane::CreatePlane(playerFrontVector, playerViewPosition);
	Plane horizontalPlane = Plane::CreatePlane(horizontalSplitPlaneNormal, playerViewPosition);
	const float minCoverWidth = 0.5f;

	int closestFace = GetClosestBoxFaceIndex(m_coverBox, playerViewPosition);
	if (closestFace != -1)
	{
		SSegment verticalSplitSegment;
		bool splitted = SplitPolygon(m_coverBox.m_worldPolygons[closestFace], splitPlane, &verticalSplitSegment);
		float faceDirection = playerFrontVector.Dot(verticalSplitSegment.m_normal);
		if (splitted && faceDirection < 0.25f)
		{
			SSegment horizontalSplitSegment;
			splitted = SplitPolygon(m_coverBox.m_worldPolygons[closestFace], horizontalPlane, &horizontalSplitSegment);
			if (splitted && horizontalSplitSegment.m_vertices[0].GetSquaredDistance(horizontalSplitSegment.m_vertices[1]) > minCoverWidth*minCoverWidth)
			{
				int leftIndex = 0;
				int rightIndex = 1;
				if (
					Plane::CreatePlane(m_edgeReference.m_direction, m_edgeReference.m_point)
					.DistFromPlane(horizontalSplitSegment.m_vertices[0]) > 0)
				{
					leftIndex = 1;
					rightIndex = 0;
				}

				m_edgeReference.m_leanMode = ELeanMode_LEAN_OVER;
				m_edgeReference.m_point = verticalSplitSegment.m_vertices[0].z > verticalSplitSegment.m_vertices[1].z ? verticalSplitSegment.m_vertices[0] : verticalSplitSegment.m_vertices[1];
				m_edgeReference.m_normal = HorizontalNormal(verticalSplitSegment.m_normal);
				m_edgeReference.m_tangent = Vec3(0, 0, -1);
				m_edgeReference.m_direction = Vec3(m_edgeReference.m_normal.y, -m_edgeReference.m_normal.x, 0);

				m_edgeReference.m_leftLimit = Plane::CreatePlane(m_edgeReference.m_direction, horizontalSplitSegment.m_vertices[leftIndex]);
				m_edgeReference.m_rightLimit = Plane::CreatePlane(-m_edgeReference.m_direction, horizontalSplitSegment.m_vertices[rightIndex]);
				m_edgeReference.m_depth = GetBoxDepthFromFaceAndRay(m_coverBox, closestFace, m_edgeReference.m_point, playerFrontVector);
			}
		}
	}

	Vec3 egdePoint = m_edgeReference.m_point;
	if (viewDirection.z < 0)
		egdePoint += playerFrontVector * m_edgeReference.m_depth;

	m_viewFrontVec = playerFrontVector;
	m_viewLeanVec = Vec3(0, 0, 1);

	const Vec3 tangent = Vec3(m_edgeReference.m_normal.y, -m_edgeReference.m_normal.x, 0);
	m_edgeReference.m_minVector = (-m_edgeReference.m_normal + tangent*2).GetNormalized();
	m_edgeReference.m_maxVector = (-m_edgeReference.m_normal - tangent*2).GetNormalized();

	const float minRaise = -m_maxRaise*0.5f;
	const Plane plane = Plane::CreatePlane(m_edgeReference.m_normal, egdePoint);
	Vec3 ipoint;
	if(Intersect::Ray_Plane(Ray(playerViewPosition, viewDirection), plane, ipoint))
	{
		m_leanAmount = egdePoint.z - ipoint.z;
		m_leanAmount += m_raiseAmountBias;
		m_leanAmount = CLAMP(m_leanAmount, minRaise, m_maxRaise);
	}

	Vec3 toEdge = (egdePoint - (playerViewPosition + Vec3(0, 0, minRaise))).GetNormalized();
	m_edgeReference.m_maxAngle = cry_asinf(toEdge.z);
	toEdge = (egdePoint - (playerViewPosition + Vec3(0, 0, m_maxRaise))).GetNormalized();
	m_edgeReference.m_minAngle = cry_asinf(toEdge.z);
}



bool CCoverAndLean::CanStartLean() const
{
	if (!CanLean())
		return false;

	const Vec3 viewPosition = GetViewPosition();
	const Vec3 viewDirection = m_pPlayer->GetEntity()->GetForwardDir();

	// player view needs to be between min and max view vectors
	if (!IsBetweenVectors(m_edgeReference.m_minVector, m_edgeReference.m_maxVector, m_viewFrontVec))
		return false;

	// edge needs to be close to the player
	const float coverDistance = m_maxDistanceToEdge;
	Vec3 viewToEdge = ClosestPointToLine(m_edgeReference.m_point, m_edgeReference.m_direction, viewPosition) - viewPosition;
	viewToEdge.z = 0;
	if (viewToEdge.len2() > (coverDistance*coverDistance))
		return false;

	// player needs to be in edge side
	if ((m_edgeReference.m_point-viewPosition).Dot(m_edgeReference.m_tangent) > 0)
		return false;

	// do not let player use last valid edge if he jumps or goes too far way from crouch cover
	const float distanceThreshold = 0.15f;
	if (m_edgeReference.m_leanMode == ELeanMode_LEAN_OVER)
	{
		const Vec3 iPoint = LineIntersect2D(m_edgeReference.m_point, m_edgeReference.m_direction, viewPosition, viewDirection);
		const float distanceToEdgePoint = iPoint.GetSquaredDistance(m_edgeReference.m_point);
		if (distanceToEdgePoint > distanceThreshold*distanceThreshold)
		return false;
	}

	// need to have space around the lean direction
	const float cylRadius = 0.15f;
	const float cylLength = 0.75f;
	if (CylinderCastTest(m_edgeReference.m_point - m_edgeReference.m_tangent*cylRadius*1.5f, -m_edgeReference.m_normal, cylLength, cylRadius) != false)
		return false;
	if ((m_edgeReference.m_leanMode == ELeanMode_LEAN_AROUND_LEFT || m_edgeReference.m_leanMode == ELeanMode_LEAN_AROUND_RIGHT) &&
		CylinderCastTest(
		m_edgeReference.m_point - m_edgeReference.m_tangent*cylRadius*2.5f + m_edgeReference.m_normal*cylRadius*1.5f,
			m_edgeReference.m_tangent, cylLength, cylRadius) != false)
		return false;

	return true;
}



bool CCoverAndLean::CanLean() const
{
	const Vec3 viewPosition = GetViewPosition();
	const Vec3 viewDirection = m_pPlayer->GetEntity()->GetForwardDir();

	// cannot lean while carring a heavy weapon
	if (m_pPlayer->HasHeavyWeaponEquipped())
		return false;

	// lean edge needs to be of a valid type
	if (m_edgeReference.m_leanMode == ELeanMode_NONE)
		return false;

	// do not let player use last valid edge if he jumps
	const float distanceThreshold = 0.15f;
	if (m_edgeReference.m_leanMode != ELeanMode_LEAN_OVER)
	{
		if (cry_fabsf(m_edgeReference.m_point.z - viewPosition.z) > distanceThreshold)
			return false;
	}

	// crouch cover cannot be too tall
	const float maxHeight = 1.35f;
	const Vec3 playerPosition = m_pPlayer->GetEntity()->GetWorldPos();
	if (m_edgeReference.m_leanMode == ELeanMode_LEAN_OVER && m_edgeReference.m_point.z > playerPosition.z + maxHeight)
		return false;

	return true;
}



Vec3 CCoverAndLean::GetViewPosition() const
{
	Vec3 viewOffset = m_pPlayer->GetEntity()->GetWorldRotation() * m_pPlayer->GetStanceInfo(m_pPlayer->GetStance())->viewOffset;
	const Vec3 viewPosition = m_pPlayer->GetEntity()->GetWorldPos() + viewOffset;
	return viewPosition;
}



void CCoverAndLean::StopingLeaning()
{
	SActorParams* pParams = m_pPlayer->GetActorParams();
	pParams->vLimitDir.zero();
	pParams->vLimitRangeH = 0.0f;
	pParams->vLimitRangeVUp = 0.0f;
	pParams->vLimitRangeVDown = 0.0f;
	pParams->speedMultiplier = 1.0f;
}



float CCoverAndLean::GetLeanAmount() const
{
	if (m_leanBlend<0.01f || (m_edgeReference.m_leanMode!=ELeanMode_LEAN_AROUND_LEFT && m_edgeReference.m_leanMode!=ELeanMode_LEAN_AROUND_RIGHT))
		return 0;
	float leanAmount = m_edgeReference.m_leanMode==ELeanMode_LEAN_AROUND_RIGHT ? m_leanAmount : -m_leanAmount;
	return leanAmount * m_leanBlend;
}



float CCoverAndLean::GetRaiseAmount() const
{
	if (m_leanBlend<0.01f || m_edgeReference.m_leanMode != ELeanMode_LEAN_OVER)
		return 0;
	return m_leanAmount * m_leanBlend;
}



void CCoverAndLean::AdjustMovement(Vec3* pMoveVector) const
{
	if (!m_pPlayer)
		return;

	const Vec3 viewPosition = GetViewPosition();
	const Vec3 playerPosition = m_pPlayer->GetEntity()->GetWorldPos();
	const Vec3 viewDirection = m_pPlayer->GetEntity()->GetForwardDir();
	const Vec3 nextPlayerPosition = playerPosition + (*pMoveVector) * gEnv->pTimer->GetFrameTime();
	Vec3 normalMove = m_edgeReference.m_normal * pMoveVector->Dot(m_edgeReference.m_normal);

	// limit movement while leaning over
	if (m_leaning && m_edgeReference.m_leanMode == ELeanMode_LEAN_OVER)
	{
		const float maxDistanceThreshold = 0.75f;
		const float minDistanceThreshold = 0.25f;
		Vec3 tangentMove = m_edgeReference.m_direction * pMoveVector->Dot(m_edgeReference.m_direction);

		Vec3 p1 = ClosestPointToLine(m_edgeReference.m_point, m_edgeReference.m_direction, nextPlayerPosition);
		p1.z = 0;
		Vec3 p2 = Vec3(nextPlayerPosition.x, nextPlayerPosition.y, 0);
		float distanceToEdge = p1.GetSquaredDistance(p2);

		if (distanceToEdge > maxDistanceThreshold*maxDistanceThreshold && normalMove.Dot(m_edgeReference.m_normal) > 0)
			normalMove = Vec3(ZERO);
		if (distanceToEdge < minDistanceThreshold*minDistanceThreshold && normalMove.Dot(m_edgeReference.m_normal) < 0)
			normalMove = Vec3(ZERO);

		if (m_edgeReference.m_leftLimit.DistFromPlane(nextPlayerPosition) < 0 && tangentMove.Dot(m_edgeReference.m_direction) < 0)
		{
			ray_hit rayHit;
			const Vec3 rayStart = viewPosition - m_edgeReference.m_direction * 0.5f;
			if (!RaycastCoverProxies(m_pPlayer, rayStart, -m_edgeReference.m_normal, 2.0f, &rayHit, m_coverSurfaceIdx))
				tangentMove = Vec3(ZERO);
		}
		if (m_edgeReference.m_rightLimit.DistFromPlane(nextPlayerPosition) < 0 && tangentMove.Dot(m_edgeReference.m_direction) > 0)
		{
			ray_hit rayHit;
			const Vec3 rayStart = viewPosition + m_edgeReference.m_direction * 0.5f;
			if (!RaycastCoverProxies(m_pPlayer, rayStart, -m_edgeReference.m_normal, 2.0f, &rayHit, m_coverSurfaceIdx))
				tangentMove = Vec3(ZERO);
		}

		*pMoveVector = normalMove + tangentMove;
	}

	// move player to best lean around position
	if (m_leaning && (m_edgeReference.m_leanMode == ELeanMode_LEAN_AROUND_LEFT || m_edgeReference.m_leanMode == ELeanMode_LEAN_AROUND_RIGHT))
	{
		const float bestPositionRadius = 0.5f;
		const float moveSpeedMult = 4.0f;
		const float maxMoveSpeed = 4.0f;
		const Vec3 bestPosition = m_edgeReference.m_point + m_edgeReference.m_normal*bestPositionRadius + m_edgeReference.m_tangent*bestPositionRadius;
		Vec3 moveVec = (bestPosition - playerPosition) * moveSpeedMult;
		moveVec.z = 0;
		if (moveVec.len2() > maxMoveSpeed*maxMoveSpeed)
			moveVec = moveVec.GetNormalized() * maxMoveSpeed;

		*pMoveVector = moveVec;
	}
}
