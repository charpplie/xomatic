/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Camera mode implementations

-------------------------------------------------------------------------
History:
- 16:10:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "CameraModes.h"
#include "Player.h"
#include "GameCVars.h"
#include "Single.h"

#include "MindControlInput.h"
#include "MindControl.h"

#include <IViewSystem.h>
#include <IVehicleSystem.h>





CDefaultCameraMode::CDefaultCameraMode()
	:	m_bobCycle(0.0f)
	,	m_bobOffset(ZERO)
{
}



void CDefaultCameraMode::UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime)
{
	viewParams.viewID = 0;
	viewParams.fov = g_pGameCVars->cl_fov* clientPlayer.GetActorParams()->viewFoVScale * clientPlayer.GetActorParams()->viewFoVScale_Aux * (gf_PI/180.0f);

	const CCameraPose cameraPose =
		!clientPlayer.IsThirdPerson() ?
			FirstPersonCameraPose(clientPlayer, frameTime) : 
			ThirdPersonCameraPose(clientPlayer, viewParams);

	viewParams.position = cameraPose.GetPosition();
	viewParams.rotation = cameraPose.GetRotation();
}


CDefaultCameraMode::CCameraPose CDefaultCameraMode::FirstPersonCameraPose(const CPlayer& clientPlayer, float frameTime)
{
	const float cameraFactor = clientPlayer.GetCameraAnimationFactor();
	const CCameraPose firstPersonCameraPose = CCameraPose(
		clientPlayer.GetFPCameraPosition(),
		clientPlayer.GetViewQuatFinal());

	const CCameraPose animatedFirstPersonCamera = 
		(cameraFactor > 0.0f) && (clientPlayer.m_lastTorsoAimIKDir.len2() > 0.0f) ? 
			FirstPersonCameraAnimation(clientPlayer, firstPersonCameraPose, cameraFactor) :
			firstPersonCameraPose;

	return
		CCameraPose::Compose(
			animatedFirstPersonCamera,
			CCameraPose::Compose(
				ViewBobing(clientPlayer, frameTime),
				MountedGunView(clientPlayer, firstPersonCameraPose)));
}



CDefaultCameraMode::CCameraPose CDefaultCameraMode::FirstPersonCameraAnimation(const CPlayer& clientPlayer, const CCameraPose& firstPersonCamera, float cameraFactor) const
{
	const Matrix34& clientPlayerTM = clientPlayer.GetEntity()->GetWorldTM();

	const Quat lastTargetDirection = Quat::CreateRotationVDir(clientPlayer.m_lastTorsoAimIKDir);
	const Quat cameraRotationAnimation = (lastTargetDirection.GetInverted() * clientPlayer.GetCameraTran().q).GetNormalized();
	const Vec3 cameraPositionAnimation = clientPlayerTM.TransformVector(clientPlayer.GetCameraTran().t - clientPlayer.m_lastTorsoAimIKPos);
	const CCameraPose animatedCameraOffset = CCameraPose(cameraPositionAnimation, cameraRotationAnimation);
	const CCameraPose cameraAnimation = CCameraPose::Compose(firstPersonCamera, CCameraPose::Scale(animatedCameraOffset, cameraFactor));

	return cameraAnimation;
}



CDefaultCameraMode::CCameraPose CDefaultCameraMode::ThirdPersonCameraPose(const CPlayer& clientPlayer, const SViewParams& viewParams) const
{
	const Matrix34& clientPlayerTM = clientPlayer.GetEntity()->GetWorldTM();

	const Vec3 cameraDistanceVector = viewParams.rotation.GetColumn1() * -g_pGameCVars->cl_tpvDist;
	const Vec3 stanceViewOffset = clientPlayer.GetBaseQuat() * clientPlayer.GetStanceViewOffset(clientPlayer.GetStance());

	const float cameraYaw = g_pGameCVars->cl_tpvYaw;
	const Quat cameraYawRotation =
		cameraYaw > 0.001f ? 
			Quat::CreateRotationXYZ(Ang3(0, 0, cameraYaw * gf_PI/180.0f)) :
			Quat(IDENTITY);

	const Vec3 tpCameraPosition = clientPlayerTM.GetTranslation() + cameraDistanceVector + stanceViewOffset;
	const Quat tpCameraRotation = clientPlayer.GetViewQuatFinal() * cameraYawRotation;

	return CCameraPose(tpCameraPosition, tpCameraRotation);
}



CDefaultCameraMode::CCameraPose CDefaultCameraMode::ViewBobing(const CPlayer& clientPlayer, float frameTime)
{
	const float headBod = 0.1f;
	const float headBobLimit = 1.5f;

	CWeapon* pPlayerWeapon = clientPlayer.GetWeapon(clientPlayer.GetCurrentItemId());
	if (pPlayerWeapon && pPlayerWeapon->IsZoomed())
		return CCameraPose();

	EStance refStance = clientPlayer.GetStance();
	float standSpeed = clientPlayer.GetStanceMaxSpeed(refStance);
	float flatSpeed = clientPlayer.GetActorStats()->speedFlat;
	Vec3 playerVelocity = clientPlayer.GetActorStats()->velocity;
	bool onGround = clientPlayer.GetActorStats()->onGround > 0.0f;
	Quat viewQuatFinal = clientPlayer.GetViewQuatFinal();
	Quat baseQuat = clientPlayer.GetBaseQuat();


	Vec3 bobOffset(0,0,0);

	Vec3 vSpeed(0,0,0);
	if (standSpeed > 0.001f)
		vSpeed = (playerVelocity / standSpeed);

	float vSpeedLen(vSpeed.len());
	if (vSpeedLen>1.5f)
		vSpeed = vSpeed / vSpeedLen * 1.5f;

	float speedMul = 0;
	if (standSpeed > 0.001f)
		speedMul = (flatSpeed / standSpeed * 1.1f);

	speedMul = min(1.5f,speedMul);

	if (onGround)
	{
		const float kSpeedToBobFactor = g_pGameCVars->cl_speedToBobFactor;
		const float bobWidth = g_pGameCVars->cl_bobWidth;
		const float bobHeight = g_pGameCVars->cl_bobHeight;
		const float kStrafeHorzScale = g_pGameCVars->cl_strafeHorzScale;
		const float kBobWidth = bobWidth * speedMul;
		const float kBobHeight = bobHeight * speedMul;

		m_bobCycle += frameTime * kSpeedToBobFactor * standSpeed;

		if (speedMul < 0.1f)
			m_bobCycle = min(m_bobCycle + frameTime, 1.0f);

		if (m_bobCycle > 1.0f)
			m_bobCycle = m_bobCycle - 1.0f;

		Vec3 bobDir(cry_sinf(m_bobCycle*gf_PI*2.0f)*kBobWidth*speedMul,0,cry_sinf(m_bobCycle*gf_PI*4.0f)*kBobHeight*speedMul);

		if (speedMul > 0.01f)
		{
			float dot(viewQuatFinal.GetColumn0() * vSpeed);
			bobDir.x -= dot * kStrafeHorzScale;
		}

		bobOffset += bobDir;
		bobOffset -= baseQuat.GetColumn2() * 0.035f * speedMul;
	}
	else
	{
		m_bobCycle = 0;
	}

	float headBobScale = 1.0f;
	if (standSpeed * standSpeed > 0.001f)
	{
		headBobScale = flatSpeed / standSpeed;
	}
	headBobScale = min(1.0f, headBobScale);
	bobOffset = bobOffset * 2.0f * headBod * headBobScale;

	Interpolate(m_bobOffset, bobOffset, 3.95f, frameTime);

	float bobLenSq = m_bobOffset.GetLengthSquared();
	if (bobLenSq > headBobLimit*headBobLimit)
	{
		float bobLen = sqrt_tpl(bobLenSq);
		m_bobOffset *= headBobLimit/bobLen;
	}

	return CCameraPose(
		Vec3(ZERO),
		Quat(
			Ang3(
				m_bobOffset.z,
				m_bobOffset.x,
				m_bobOffset.x)));
}



CDefaultCameraMode::CCameraPose CDefaultCameraMode::MountedGunView(const CPlayer& clientPlayer, const CCameraPose& firstPersonCamera) const
{
	const bool hasMountedWeapon = clientPlayer.GetActorStats()->mountedWeaponID != 0;

	if (hasMountedWeapon)
	{
		const CWeapon* pCurrentWeapon = clientPlayer.GetWeapon(clientPlayer.GetCurrentItemId());
		const bool isWeaponZoomed = pCurrentWeapon ? pCurrentWeapon->IsZoomed() : false;
		const float weaponFPAimHoriz = clientPlayer.m_weaponFPAiming.GetHorizontalSweay();
		const float weaponFPAimVert = clientPlayer.m_weaponFPAiming.GetVerticalSweay();

		const float zoomFactor = 0.2f;
		const float horizontalDirection = isWeaponZoomed ? -1.0f : 1.0f;
		const float factor = isWeaponZoomed ? zoomFactor : 1.0f;
		const float rotationMultiplier = 0.05f * factor;
		const float positionmultiplier = 0.035f * factor;

		const Vec3 right = firstPersonCamera.GetRotation().GetColumn0();
		const Vec3 up = firstPersonCamera.GetRotation().GetColumn2();

		const Ang3 offsetAngles = Ang3(
			-weaponFPAimVert * rotationMultiplier,
			0.0f,
			-weaponFPAimHoriz * rotationMultiplier * horizontalDirection);
		const Vec3 offsetPosition =
			- weaponFPAimHoriz*right*positionmultiplier*horizontalDirection
			+ weaponFPAimVert*up*positionmultiplier;

		return CCameraPose(offsetPosition, Quat(offsetAngles));
	}
	else
	{
		return CCameraPose();
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CSpectatorFollowCameraMode::CSpectatorFollowCameraMode()
: m_lastSpectatorTarget(0)
, m_lastFrameId(0)
, m_position(ZERO)
, m_viewOffset(ZERO)
, m_entityPos(ZERO)
{

}


void CSpectatorFollowCameraMode::UpdateView( const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime )
{
	const EntityId spectatorTargetId = clientPlayer.GetSpectatorTarget();

	CActor* pTarget = (CActor*)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(spectatorTargetId);
	if(!pTarget)
		return;

	const Matrix34& targetWorldTM = pTarget->GetEntity()->GetWorldTM();

	Vec3 targetWorldPos = targetWorldTM.GetTranslation();
	targetWorldPos.z += 1.5f;

	const Ang3 worldAngles = Ang3::GetAnglesXYZ(Matrix33(targetWorldTM));
	const float rot = worldAngles.z;
	float distance = 3;

	if(IVehicle* pVehicle = pTarget->GetLinkedVehicle())
	{
		AABB vehicleBox;
		pVehicle->GetEntity()->GetLocalBounds(vehicleBox);
		distance = 2.0f * vehicleBox.GetRadius();
	}

	Vec3 goal;
	float zoom = 1.0f;
	goal.x = distance * zoom * cosf(rot + gf_PI*1.5f) + targetWorldPos.x;
	goal.y = distance * zoom * sinf(rot - gf_PI/2.0f) + targetWorldPos.y;

	AABB targetBounds;
	pTarget->GetEntity()->GetLocalBounds(targetBounds);
	goal.z = targetBounds.max.z;

	float offset = 0.75f;
	if(pTarget->GetLinkedVehicle())
		offset = 2.0f;
	goal.z += pTarget->GetEntity()->GetWorldPos().z + offset;

	// store / interpolate the offset, not the world pos (reduces percieved jitter in vehicles)
	m_viewOffset = goal-targetWorldPos;
	m_position = goal;
	m_entityPos = targetWorldPos;
	m_lastSpectatorTarget = spectatorTargetId;

	// do a ray cast to check for camera intersection
	static ray_hit hit;	
	IPhysicalEntity* pSkipEntities[10];
	int nSkip = 0;
	IItem* pItem = pTarget->GetCurrentItem();
	if (pItem)
	{
		CWeapon* pWeapon = (CWeapon*)pItem->GetIWeapon();
		if (pWeapon)
			nSkip = CSingle::GetSkipEntities(pWeapon, pSkipEntities, 10);
	}
	else if(IVehicle* pVehicle = pTarget->GetLinkedVehicle())
	{
		// vehicle drivers don't seem to have current items, so need to add the vehicle itself here
		nSkip = pVehicle->GetSkipEntities(pSkipEntities, 10);
	}

	const float minDist = 0.4f;	// how close we're allowed to get to the target
	const float wallSafeDistance = 0.2f; // how far to keep camera from walls

	const Vec3 dir = goal - targetWorldPos;

	primitives::sphere sphere;
	sphere.center = targetWorldPos;
	sphere.r = wallSafeDistance;

	geom_contact *pContact = 0;          
	const float hitDist = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(sphere.type, &sphere, dir, ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid,
		&pContact, 0, (geom_colltype_player<<rwi_colltype_bit) | rwi_stop_at_pierceable, 0, 0, 0, pSkipEntities, nSkip);

	// even when we have contact, keep the camera the same height above the target
	const float minHeightDiff = dir.z;

	if(hitDist > 0 && pContact)
	{
		goal = targetWorldPos + (hitDist * dir.GetNormalizedSafe());

		if(goal.z - targetWorldPos.z < minHeightDiff)
		{
			// can't move the camera far enough away from the player in this direction. Try moving it directly up a bit
			int numHits = 0;
			sphere.center = goal;

			// (move back just slightly to avoid colliding with the wall we've already found...)
			sphere.center -= dir.GetNormalizedSafe() * 0.05f;

			const float newHitDist = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(sphere.type, &sphere, Vec3(0,0,minHeightDiff), ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid,
				&pContact, 0, (geom_colltype_player<<rwi_colltype_bit) | rwi_stop_at_pierceable, 0, 0, 0, pSkipEntities, nSkip);

			float raiseDist = minHeightDiff - (goal.z - targetWorldPos.z) - wallSafeDistance;
			if(newHitDist != 0)
			{
				raiseDist = MIN(minHeightDiff, newHitDist);
			}

			raiseDist = MAX(0.0f, raiseDist);

			goal.z += raiseDist;
			targetWorldPos.z += raiseDist*0.8f;
		}
	}

	const int thisFrameId = gEnv->pRenderer->GetFrameID();
	if(thisFrameId - m_lastFrameId > 5)
	{
		// reset positions
		m_viewOffset = goal - targetWorldPos;
		m_entityPos = targetWorldPos;
		m_position = goal;
	}
	if(m_lastSpectatorTarget != spectatorTargetId)
	{
		m_viewOffset = goal - targetWorldPos;
		m_entityPos = targetWorldPos;
		m_position = goal;
		m_lastSpectatorTarget = spectatorTargetId;
	}
	m_lastFrameId = thisFrameId;

	if(pTarget->GetLinkedVehicle())
	{
		Interpolate(m_viewOffset, goal-targetWorldPos, 5.0f, viewParams.frameTime);
		m_entityPos = targetWorldPos;
		viewParams.position = targetWorldPos + m_viewOffset;
		m_position = viewParams.position;
	}
	else
	{
		const Vec3 camPosChange = goal - m_position;
		const Vec3 entPosChange = targetWorldPos - m_entityPos;

		if(camPosChange.GetLengthSquared() > 100.0f)
			m_position = goal;
		if(entPosChange.GetLengthSquared() > 100.0f)
			m_entityPos = targetWorldPos;

		Interpolate(m_position, goal, 5.0f, viewParams.frameTime);
		Interpolate(m_entityPos, targetWorldPos, 5.0f, viewParams.frameTime);
		viewParams.position = m_position;
	}

	Matrix33 rotation = Matrix33::CreateRotationVDir((m_entityPos - viewParams.position).GetNormalizedSafe());

	viewParams.rotation = GetQuatFromMat33(rotation);	
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CSpectatorFixedCameraMode::UpdateView( const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime )
{
	const SPlayerStats* pPlayerStats = static_cast<const SPlayerStats*>(clientPlayer.GetActorStats());

	if (pPlayerStats->spectatorInfo.mode == CActor::eASM_Fixed)
	{
		IEntity*  pLocationEntity = gEnv->pEntitySystem->GetEntity(clientPlayer.GetSpectatorFixedLocation());
		assert(pLocationEntity);
		const Vec3  locationPosition = (pLocationEntity ? pLocationEntity->GetWorldPos() : viewParams.position);

		if (!IsEquivalent(locationPosition, viewParams.position))  // ie. changed camera (probably)
		{
			viewParams.position = locationPosition;
			viewParams.rotation = pLocationEntity->GetWorldRotation();
		}

		//Benito: Camera should not write back to player.
		//Add maybe a return value which dictates if all those quaternions should be re-oriented towards the camera
		//m_io.baseQuat = m_io.viewQuat = m_io.viewQuatFinal = viewParams.rotation;
	}
	else if (pPlayerStats->spectatorInfo.mode == CActor::eASM_CCTV)
	{
		IEntity*  pCctvEntity = gEnv->pEntitySystem->GetEntity(clientPlayer.GetSpectatorCCTVCam());
		assert(pCctvEntity);
		const Vec3  cctvPosition = (pCctvEntity ? pCctvEntity->GetWorldPos() : viewParams.position);

		if (!IsEquivalent(cctvPosition, viewParams.position))  // ie. changed camera (probably)
		{
			viewParams.position = cctvPosition;
			viewParams.rotation = pCctvEntity->GetWorldRotation();

			//Benito: This should be done by the code which switches camera
			//player.m_stats.spectatorInfo.dataU.cctv.rotOffset.x = 0.f;
			//player.m_stats.spectatorInfo.dataU.cctv.rotOffset.z = 0.f;
		}

		//Benito: Camera should not write back to player.
		//Add maybe a return value which dictates if all those quaternions should be re-oriented towards the camera
		//m_io.baseQuat = m_io.viewQuat = m_io.viewQuatFinal = viewParams.rotation;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void CAnimationControlledCameraMode::UpdateView( const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime )
{
	const SPlayerStats* pStats = static_cast<const SPlayerStats*>(clientPlayer.GetActorStats());

	const Matrix34& clientPlayerTM = clientPlayer.GetEntity()->GetWorldTM();
	const QuatT localEyeLocation = clientPlayer.GetCameraTran();
	bool isRagDoll = pStats->isRagDoll;
	bool followCharacterHead = pStats->followCharacterHead.Value() != 0;

	viewParams.viewID = 3;

	viewParams.position = clientPlayerTM * localEyeLocation.t;

	if (followCharacterHead)
	{
		viewParams.rotation = clientPlayer.GetBaseQuat() * localEyeLocation.q; 
	}
	else
	{
		viewParams.rotation = clientPlayer.GetViewQuatFinal();
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CDeathCameraMode::CDeathCameraMode()
: m_lastFrameId(0)
, m_lastDeathCamSubject(0)
, m_lastFOVScale(1.0f)
, m_lastCameraPos(ZERO)
, m_lastTargetPos(ZERO)
{

}

void CDeathCameraMode::UpdateView( const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime )
{
	const SPlayerStats* pPlayerStats = static_cast<const SPlayerStats*>(clientPlayer.GetActorStats());

	CActor* pTarget = (CActor*)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pPlayerStats->deathCamSubject);
	if(!pTarget)
		return;

	const Matrix34& targetWorldTM = pTarget->GetEntity()->GetWorldTM();

	Vec3 camPos = viewParams.position;
	const float offset = 1.5f;
	camPos.z += offset;

	float heightOffset = 1.5f;
	const SStanceInfo* pSI = pTarget->GetStanceInfo(pTarget->GetStance());
	if(pSI)
	{
		heightOffset = pSI->viewOffset.z;
	}

	Vec3 targetPos = targetWorldTM.GetTranslation();
	targetPos.z += heightOffset;

	const int thisFrameId = gEnv->pRenderer->GetFrameID();
	m_lastCameraPos = camPos;
	m_lastTargetPos = targetPos;
	m_lastDeathCamSubject = pPlayerStats->deathCamSubject;

	// if more than a few frames have passed since our last update, invalidate the positions
	if(thisFrameId - m_lastFrameId > 5)
	{
		m_lastCameraPos = viewParams.position;	// interpolate from current camera pos
		m_lastTargetPos = targetPos;
		m_lastFOVScale = 1.0f;
	}
	// if target changed, reset positions
	if(m_lastDeathCamSubject != pPlayerStats->deathCamSubject)
	{
		m_lastCameraPos = camPos;
		m_lastTargetPos = targetPos;
		m_lastDeathCamSubject = pPlayerStats->deathCamSubject;
		m_lastFOVScale = 1.0f;
	}
	m_lastFrameId = thisFrameId;

	// slight zoom after 2s
	const float timeNow = gEnv->pTimer->GetCurrTime();
	const float distSq = (targetPos - camPos).GetLengthSquared();
	float scale = 1.0f;
	if((timeNow - clientPlayer.GetDeathTime() > 1.0f) && (distSq > 2500.0f))
	{
		// 1.0f at 50m, 0.3f at 100m+
		scale = 1.0f - (distSq - 2500.0f)/25000.0f;
		scale = CLAMP(scale, 0.3f, 1.0f);
	}

	Interpolate(m_lastCameraPos, camPos, 5.0f, viewParams.frameTime);
	Interpolate(m_lastTargetPos, targetPos, 5.0f, viewParams.frameTime);
	Interpolate(m_lastFOVScale, scale, 0.5f, viewParams.frameTime);

	viewParams.position = m_lastCameraPos;
	Vec3 dir = (m_lastTargetPos - m_lastCameraPos).GetNormalizedSafe();
	Matrix33 rotation = Matrix33::CreateRotationVDir(dir);	
	dir.z = 0.0f;

	// quick ray check to make sure there's not a wall in the way...
	static ray_hit hit;	
	IPhysicalEntity* pSkipEntities[10];
	int nSkip = 0;
	IItem* pItem = clientPlayer.GetCurrentItem();
	if (pItem)
	{
		CWeapon* pWeapon = (CWeapon*)pItem->GetIWeapon();
		if (pWeapon)
			nSkip = CSingle::GetSkipEntities(pWeapon, pSkipEntities, 10);
	}

	if (gEnv->pPhysicalWorld->RayWorldIntersection(viewParams.position, -dir, ent_static|ent_terrain|ent_rigid,
		rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1, pSkipEntities, nSkip))
	{
		dir.zero();
	}

	viewParams.position -= dir;

	viewParams.fov = g_pGameCVars->cl_fov * m_lastFOVScale * (gf_PI/180.0f);;
	viewParams.rotation = GetQuatFromMat33(rotation);	
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void CVehicleCameraMode::UpdateView( const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime )
{
	if (IVehicle* pVehicle = clientPlayer.GetLinkedVehicle())
	{
		pVehicle->UpdateView(viewParams, clientPlayer.GetEntityId());
		viewParams.viewID = 2;		
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CPartialAnimationControlledCameraMode::UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime)
{
	const SPlayerStats* pStats = static_cast<const SPlayerStats*>(clientPlayer.GetActorStats());

	const Matrix34& clientPlayerTM = clientPlayer.GetEntity()->GetWorldTM();
	const QuatT localEyeLocation = clientPlayer.GetCameraTran();
	const bool isThirdPerson = pStats->isThirdPerson;

	viewParams.viewID = 3;
	viewParams.blend = true;

	viewParams.rotation = clientPlayer.GetViewQuatFinal();

	if(isThirdPerson)
	{
		const Vec3 view3rdOffset =	(viewParams.rotation.GetColumn1() * -g_pGameCVars->cl_tpvDist) +  
																(clientPlayer.GetBaseQuat() * clientPlayer.GetStanceViewOffset(clientPlayer.GetStance()));
		viewParams.position = clientPlayerTM.GetTranslation() + view3rdOffset;
	}
	else
	{
		const Quat worldEyeOrientation = clientPlayer.GetEntity()->GetWorldRotation() * localEyeLocation.q;
		const Quat localChangeQuat = worldEyeOrientation / viewParams.rotation;

		const float blendFactor = GetBlendOrientationFactor(*pStats);

		viewParams.localRotationLast = Quat::CreateSlerp(IDENTITY, localChangeQuat, blendFactor);
		viewParams.rotation = viewParams.rotation * viewParams.localRotationLast;
		viewParams.rotation.Normalize();

		viewParams.position = clientPlayerTM.TransformPoint(localEyeLocation.t);
	}
}


float CPartialAnimationControlledCameraMode::GetBlendOrientationFactor( const SPlayerStats& playerStats ) const
{
	if (playerStats.ledgeStats.onLedge)
	{
		return g_pGameCVars->pl_ledgeClamber.cameraBlendWeight;
	}
	else if (playerStats.slideStats.slidingState != SPlayerStats::eSS_None)
	{
		float blendOrientationFactor = 0.3f;

		if (playerStats.slideStats.slidingState == SPlayerStats::eSS_ExitingSlide)
		{
			const float exitSlideThrehold = 0.45f;
			const float exitBlendFactor = clamp((gEnv->pTimer->GetAsyncCurTime() - playerStats.slideStats.slideExitTime) / exitSlideThrehold, 0.0f, 1.0f);

			blendOrientationFactor = blendOrientationFactor * (1.0f - exitBlendFactor);
		}

		return blendOrientationFactor;
	}

	return 0.0f;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if ENABLE_MINDCONTROL
void CMindControlledCameraMode::UpdateView( const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime )
{
	CMindControlMaster* pMasterMind = clientPlayer.GetMindControlMaster();
	CRY_ASSERT(pMasterMind);
	if (!pMasterMind)
		return;

	CMindControlSlave* pSlaveMind = pMasterMind->GetSlaveMind();
	CRY_ASSERT(pSlaveMind);
	if (!pSlaveMind)
		return;

	CPlayer* pSlave = pSlaveMind->GetPlayer();
	CRY_ASSERT(pSlave);
	if (!pSlave)
		return;

	CRY_ASSERT(pSlave->GetPlayerInput());
	if (!pSlave->GetPlayerInput())
		return;

	CRY_ASSERT(pSlave->GetPlayerInput()->GetType() == IPlayerInput::MINDCONTROL_INPUT);
	if (pSlave->GetPlayerInput()->GetType() != IPlayerInput::MINDCONTROL_INPUT)
		return;

	CMindControlInput* pMindControlInput = static_cast<CMindControlInput*>(pSlave->GetPlayerInput());
	CRY_ASSERT(pMindControlInput);
	CMindControlInput::ECameraMode cameraMode = pMindControlInput->GetCameraMode();
	Ang3 cameraDeltaAngles = pMindControlInput->GetCameraDeltaAngles();

	const Matrix34& slaveTM = pSlave->GetEntity()->GetWorldTM();
	
	viewParams.viewID = 0; // ???

	Vec3 stanceOffset(pSlave->GetStanceInfo(STANCE_STAND)->viewOffset); // Always stand because we don't want the camera offset to change when the stance changes
	stanceOffset.x = 0.0f;
	stanceOffset.y = -4.0f;

	switch(cameraMode)
	{
	case CMindControlInput::eCM_None:
		{
			CDefaultCameraMode defaultCamera;
			defaultCamera.UpdateView(clientPlayer, viewParams, frameTime);
			break;
		}
	case CMindControlInput::eCM_FollowBehind:
		{
			viewParams.position = slaveTM.GetTranslation();
			viewParams.rotation = pSlave->GetEntity()->GetWorldRotation();

			viewParams.position += viewParams.rotation * stanceOffset;
			break;
		}
	case CMindControlInput::eCM_FollowFront:
		{
			viewParams.position = slaveTM.GetTranslation();
			viewParams.rotation = pSlave->GetEntity()->GetWorldRotation() * Quat::CreateRotationZ( DEG2RAD( -180.0f ) ); // look from the front to the character

			viewParams.position += viewParams.rotation * stanceOffset;
			break;
		}
	case CMindControlInput::eCM_FollowSide:
		{
			viewParams.position = slaveTM.GetTranslation();
			viewParams.rotation = pSlave->GetEntity()->GetWorldRotation() * Quat::CreateRotationZ( DEG2RAD( -90.0f ) ); // look from the left of the character

			viewParams.position += viewParams.rotation * stanceOffset;
			break;
		}
	case CMindControlInput::eCM_FollowTop:
		{
			viewParams.position = slaveTM.GetTranslation();
			viewParams.rotation = pSlave->GetEntity()->GetWorldRotation() * Quat::CreateRotationX( DEG2RAD( -90.0f ) ); // look from the top

			Vec3 offset = Vec3(stanceOffset.x, stanceOffset.y, 0.0f);
			viewParams.position += viewParams.rotation * offset;
			break;
		}
	case CMindControlInput::eCM_LookAtSlaveFromPlayer:
		{
			const Matrix34& clientPlayerTM = clientPlayer.GetEntity()->GetWorldTM();

			viewParams.position = clientPlayerTM * clientPlayer.GetLocalEyePos();

			Vec3 playerToSlave( (slaveTM * pSlave->GetLocalEyePos()) - viewParams.position );
			playerToSlave.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);

			viewParams.rotation = Quat::CreateRotationVDir(playerToSlave , 0.0f);
			break;
		}
	default:
		CRY_ASSERT(false);
	}

}
#endif
