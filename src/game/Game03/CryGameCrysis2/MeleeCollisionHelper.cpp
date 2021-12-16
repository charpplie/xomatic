/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------

Description: Helper class for melee collision detection

-------------------------------------------------------------------------
History:
- 10:03:2010   Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "MeleeCollisionHelper.h"

#include "Game.h"
#include "GameRules.h"
#include "AutoAimManager.h"

#include <ICryAnimation.h>

void CMeleeCollisionHelper::OnDataReceived( const EventPhysRWIResult *pRWIResult )
{
	m_blocked = false;
	if (pRWIResult && pRWIResult->nHits > 0 && pRWIResult->pHits)
	{
		ray_hit *pHit = &pRWIResult->pHits[0];
		OnSuccesfulHit(*pHit);
	}
	else
	{
		OnFailedHit();
	}
}

void CMeleeCollisionHelper::OnDataReceived( const EventPhysPWIResult *pPWIResult )
{

}

void CMeleeCollisionHelper::OnDRWReset()
{

}

void CMeleeCollisionHelper::DoCollisionTest( const SCollisionTestParams& requestInfo )
{
	m_testParams = requestInfo;

	IEntity *pIgnoredEntity = gEnv->pEntitySystem->GetEntity(requestInfo.m_ignoredEntityIds[0]);
	IEntity *pIgnoredEntity2 = gEnv->pEntitySystem->GetEntity(requestInfo.m_ignoredEntityIds[1]);
	int ignoreCount = 0;
	IPhysicalEntity *pIgnoredEntityPhysics[2] = { NULL, NULL };
	if (pIgnoredEntity)
	{
		pIgnoredEntityPhysics[ignoreCount] = pIgnoredEntity->GetPhysics();
		ignoreCount += pIgnoredEntityPhysics[ignoreCount] ? 1 : 0;
	}
	if (pIgnoredEntity2)
	{
		pIgnoredEntityPhysics[ignoreCount] = pIgnoredEntity2->GetPhysics();
		ignoreCount += pIgnoredEntityPhysics[ignoreCount] ? 1 : 0;
	}

	m_raycastHelper->CastRay(requestInfo.m_pos, requestInfo.m_dir * requestInfo.m_distance, ent_all|ent_water, rwi_stop_at_pierceable|rwi_ignore_back_faces, pIgnoredEntityPhysics, ignoreCount);

	m_blocked = true;
}

EntityId CMeleeCollisionHelper::GetBestAutoAimTargetForUser( EntityId userId,  const Vec3& scanPosition, const Vec3& scanDirection, float range, float angle ) const
{
	CGameRules* pGameRules = g_pGame->GetGameRules();
	int playerTeam = pGameRules->GetTeam(userId);

	const TAutoaimTargets& aaTargets = g_pGame->GetAutoAimManager().GetAutoAimTargets();
	const int targetCount = aaTargets.size();

	EntityId nearestTargetId = 0;
	float nearestEnemyDst = range + 0.1f;

	for (int i = 0; i < targetCount; ++i)
	{
		const SAutoaimTarget& target = aaTargets[i];

		CRY_ASSERT(target.entityId != userId);

		//Skip friendly ai
		if ((gEnv->bMultiplayer == false) && (target.HasFlagSet(eAATF_AIHostile) == false))
			continue;

		//Skip team mates
		if ((target.teamNum > 1) && (pGameRules->GetTeam(target.entityId) == playerTeam))
			continue;

		const Vec3 targetPos = target.primaryAimPosition;
		Vec3 targetDir = (targetPos - scanPosition);
		const float distance = targetDir.NormalizeSafe();

		// Reject if the enemy is not near enough or too far away
		if ((distance >= nearestEnemyDst) || (distance > range))
			continue;

		// Reject if not inside the view cone
		float alignment = scanDirection.Dot(targetDir);
		if (alignment < angle)
			continue;

		// Auto aiming is checking against the vis-table every frame
		// It should be safe to assume, we have some 'valid' information about the visibility of the enemy in here
		if (!g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(target.entityId))
		{
			continue;
		}

		// We have a new candidate
		nearestEnemyDst = distance;
		nearestTargetId = target.entityId;

	}

	return nearestTargetId;
}

void CMeleeCollisionHelper::Impulse( IPhysicalEntity *pCollider, const Vec3 &position, const Vec3 &impulse, int partId, int ipart )
{
	assert(pCollider);

	IEntity * pEntity = (IEntity*) pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);

	if (pEntity)
	{
		if(IEntityPhysicalProxy* pPhysicsProxy = (IEntityPhysicalProxy*)pEntity->GetProxy(ENTITY_PROXY_PHYSICS))
		{
			pPhysicsProxy->AddImpulse(partId, position, impulse, true, 1.0f);
		}
	}
	else
	{
		pe_action_impulse ai;
		ai.partid = partId;
		ai.ipart = ipart;
		ai.point = position;
		ai.impulse = impulse;
		pCollider->Action(&ai);
	}
}

void CMeleeCollisionHelper::GenerateArtificialCollision( IEntity* pUser, IPhysicalEntity *pCollider, const Vec3 &position, const Vec3& normal, const Vec3 &speed, int partId, int ipart, int surfaceIdx )
{
	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
	int matPlayerColliderId = pSurfaceTypeManager->GetSurfaceTypeByName("mat_player_collider")->GetId();

	// create a physical collision to break trees
	pe_action_register_coll_event collision;

	collision.collMass = 0.005f; // this is actually ignored
	collision.partid[1] = partId;

	// collisions involving partId<-1 are to be ignored by game's damage calculations
	// usually created articially to make stuff break.
	collision.partid[0] = -2;
	collision.idmat[1] = surfaceIdx;
	collision.idmat[0] = matPlayerColliderId;
	collision.n = normal;
	collision.pt = position;
	collision.vSelf = speed;
	collision.v = Vec3(0,0,0);
	collision.pCollider = pCollider;

	ICharacterInstance *pUserCharacter = pUser ? pUser->GetCharacter(0) : NULL;
	ISkeletonPose* pSkeletonPose = pUserCharacter ? pUserCharacter->GetISkeletonPose() : NULL;
	IPhysicalEntity* pCharacterPhyscis = pSkeletonPose ? pSkeletonPose->GetCharacterPhysics() : NULL;
	if (pCharacterPhyscis)
	{
		pCharacterPhyscis->Action(&collision);
	}
}

bool CMeleeCollisionHelper::PerformMeleeOnAutoTarget( EntityId targetId )
{
	const SAutoaimTarget* pAutoTarget = g_pGame->GetAutoAimManager().GetTargetInfo(targetId);
	IEntity* pAutoTargetEntity = gEnv->pEntitySystem->GetEntity(targetId);

	if(!pAutoTarget || !pAutoTargetEntity)
		return false;

	ICharacterInstance* pCharacter = pAutoTargetEntity->GetCharacter(0);
	ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
	IPhysicalEntity* pTargetPhysics =  pSkeletonPose ? pSkeletonPose->GetCharacterPhysics() : NULL;
	if (!pTargetPhysics)
		return false;

	pe_params_part partParams;
	partParams.partid = pAutoTarget->physicsBoneId;
	int iPart = -1;
	int surfaceIdx = 0;
	if (pTargetPhysics->GetParams(&partParams))
	{
		phys_geometry* pGeom = partParams.pPhysGeomProxy ? partParams.pPhysGeomProxy : partParams.pPhysGeom;
		int geomSurfaceIdx = pGeom ? pGeom->surface_idx : -1;
		iPart = partParams.ipart;
		if (geomSurfaceIdx >= 0 &&  geomSurfaceIdx < partParams.nMats)
		{
			surfaceIdx = partParams.pMatMapping[geomSurfaceIdx];   
		}
	}

	ray_hit hit;
	hit.pt = pAutoTarget->primaryAimPosition;
	hit.n = -m_testParams.m_dir;
	hit.pCollider = pTargetPhysics;
	hit.surface_idx = surfaceIdx;
	hit.partid = pAutoTarget->physicsBoneId;
	hit.ipart = iPart;

	OnSuccesfulHit(hit);

	return true;
}
