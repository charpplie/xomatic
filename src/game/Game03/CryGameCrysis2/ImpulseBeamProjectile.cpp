/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Impulse Beam Projectile implementation.

-------------------------------------------------------------------------
History:
- 23:02:2009   18:49 : Created by Filipe Amim

*************************************************************************/
#include "StdAfx.h"
#include <IRenderAuxGeom.h>
#include <VectorSet.h>
#include <ISound.h>
#include <IEntitySystem.h>
#include <IVehicleSystem.h>

#include "Projectile.h"
#include "ImpulseBeamProjectile.h"
#include "Single.h"
#include "Game.h"
#include "Actor.h"
#include "WeaponSharedParams.h"
#include "ImpulseBeam.h"
#include "GameRules.h"


CImpulseBeamProjectile::CImpulseBeamProjectile()
{
}


CImpulseBeamProjectile::~CImpulseBeamProjectile()
{
}


void CImpulseBeamProjectile::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	m_position = pos;
	m_direction = dir;
	m_velocity = velocity.len() * speedScale;
	m_radius = 0;
}


void CImpulseBeamProjectile::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	// move impulse beam
	const float radiusPerSecond =
		cry_tanf(DEG2RAD(m_pConeData->angle)) * m_pConeData->speed;
	m_position += m_direction * m_pConeData->speed * ctx.fFrameTime;
	m_radius += radiusPerSecond * ctx.fFrameTime;

	// calculate boxes positions
	const float boxSize = m_radius * (1.0f / (float)numBoxes);
	Vec3 normal = Vec3(m_direction.y, -m_direction.x, 0);
	normal.Normalize();
	AABB boxes[numBoxes];
	for (int i = 0; i < numBoxes; ++i)
	{
		const Vec3 center = m_position + ((float)(i-((int)numBoxes >> 1)))*normal*boxSize;
		boxes[i].min = center - Vec3(boxSize, boxSize, boxSize*numBoxes*0.5f);
		boxes[i].max = center + Vec3(boxSize, boxSize, boxSize*numBoxes*0.5f);
	}

	// calculate intersections with world
	CWeapon* pWeapon = GetWeapon();
	IPhysicalEntity* pSkip[5]={0};
	if (pWeapon)
		CSingle::GetSkipEntities(pWeapon, &pSkip[0], 4);

	bool updating = false;
	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	for (int i = 0; i < numBoxes; ++i)
	{
		if (m_impulseBoxes[i].m_energyRemaining < 0)
			continue;
		updating = true;

		IPhysicalEntity** pPhysicalEntities = 0;
		int objType = ent_all & (~ent_areas);
		int numEntities = pPhysicalWorld->GetEntitiesInBox(boxes[i].min, boxes[i].max, pPhysicalEntities, objType);

		if (numEntities == 0)
			continue;

		bool colliding = false;

		for (unsigned int j = 0; j < numEntities; ++j)
		{
			IPhysicalEntity* pPhysicalEntity = pPhysicalEntities[j];
			IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);
			if (!pEntity)
				continue;
			if (pEntity == pWeapon->GetOwner())
				continue;
			if (pPhysicalEntity==pSkip[0]||
				pPhysicalEntity==pSkip[1]||
				pPhysicalEntity==pSkip[2]||
				pPhysicalEntity==pSkip[3]||
				pPhysicalEntity==pSkip[4])
				continue;

			Contact(
				pPhysicalEntity, m_impulseBoxes[i].m_energyRemaining, -1,
				(boxes[i].max+boxes[i].min)*0.5f);

			colliding = true;
		}

		const float dissipation =
			colliding ?	
				m_pConeData->impulse_dissipation_colliding : 
				m_pConeData->impulse_dissipation;
		m_impulseBoxes[i].m_energyRemaining -= dissipation * ctx.fFrameTime;
	}

	// show boxes
	// debug information only
/*	for (unsigned int i = 0; i < numBoxes; ++i)
	{
		const unsigned char color = (unsigned char)CLAMP(
			0.12f*m_impulseBoxes[i].m_energyRemaining,
			0.0f, 255.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(
			boxes[i], true, ColorB(color,color,0,255), eBBD_Faceted);
	}*/

	if (!updating)
		Destroy();
}


void CImpulseBeamProjectile::SetImpulseBeamParams(const SImpulseConeParams* pConeData, float energy)
{
	m_pConeData = pConeData;
	for (unsigned int i = 0; i < numBoxes; ++i)
		m_impulseBoxes[i].m_energyRemaining = energy;
}


void CImpulseBeamProjectile::Contact(IPhysicalEntity *pPhysicalEntity, float impulse, int partId, Vec3 point)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);
	CActor *pActor = NULL;

	if (m_affectedEntities.find(pEntity) != m_affectedEntities.end())
		return;
	m_affectedEntities.insert(pEntity);

	if(pEntity)
	{
		if (IActor *pIActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()))
			pActor = static_cast<CActor *>(pIActor);
		else if (IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId()))
			impulse *= m_pConeData->impulse_vehicle_scale;
	}

	if (pActor)
	{
		if (pActor->IsPlayer())
			return;

		pActor->Fall();

		int boneId = pActor->GetBoneID(BONE_SPINE);
		Vec3 bone(ZERO);

		ICharacterInstance *pCharacter=pActor->GetEntity()->GetCharacter(0);
		assert(pCharacter);

		// up impulse on the spine
		if (boneId>-1)
		{
			Matrix34 entMtx(pActor->GetEntity()->GetSlotWorldTM(0));
			bone=entMtx*pCharacter->GetISkeletonPose()->GetAbsJointByID(boneId).t;

			pe_action_impulse uai;
			uai.partid=boneId;
			uai.impulse=ZERO;
			uai.impulse.z=m_pConeData->up_impulse;
			uai.point=bone;

			pCharacter->GetISkeletonPose()->GetCharacterPhysics()->Action(&uai);
		}

		// directional impulse
		pe_action_impulse bai;
		bai.partid = (partId > -1) ? partId : boneId;
		bai.impulse = m_direction * impulse * m_pConeData->impulse_human_scale;
		bai.point = (partId > -1) ? point : bone;
		pCharacter->GetISkeletonPose()->GetCharacterPhysics()->Action(&bai);

	}
	else
	{
		pe_action_impulse ai;
		ai.partid=partId;
	//	if (partId>-1)
			ai.point = point;
		ai.impulse = m_direction*impulse;

		pPhysicalEntity->Action(&ai);
	}

	//Generate hit as well
	CWeapon* pWeapon = GetWeapon();
	if(pEntity)
	{
		HitInfo hitInfo(
			pWeapon->GetOwnerId(), pEntity->GetId(), pWeapon->GetEntityId(),
			m_pConeData->damage * (impulse/m_pConeData->impulse),
			0.0f, 0, -1, 
			g_pGame->GetGameRules()->GetHitTypeId(m_pConeData->hit_type.c_str()),
			point, m_direction, -m_direction);
		g_pGame->GetGameRules()->ClientHit(hitInfo);
	}
}
