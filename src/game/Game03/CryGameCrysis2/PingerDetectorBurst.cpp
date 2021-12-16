/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Pinger detector burst

-------------------------------------------------------------------------
History:
- 13:08:08	Benito G.R.

*************************************************************************/
#include "StdAfx.h"
#include "PingerDetectorBurst.h"
#include "Weapon.h"
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameRules.h"
#include <IEntitySystem.h>
#include <IVehicleSystem.h>

#include "WeaponSharedParams.h"
#include "VectorSet.h"
#include "Single.h"


//------------------------------------------------------------------------
CDetectorBurst::CDetectorBurst()
: m_blastTimer(-1.0f)
{

}

//------------------------------------------------------------------------
CDetectorBurst::~CDetectorBurst()
{
}

//------------------------------------------------------------------------
void CDetectorBurst::Update(float frameTime, uint32 frameId)
{
	bool keepUpdating = false;

	if(m_blastTimer > 0.0f)
	{
		m_blastTimer -= frameTime;
		if(m_blastTimer <= 0.0f)
		{
			m_blastTimer = -1.0f;
			Blast(m_fireParams->detectorburstparams.impulse);
		}
	}

	if(keepUpdating)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CDetectorBurst::Activate(bool activate)
{
	m_firing=false;
	m_blastTimer = -1.0f;
}

//------------------------------------------------------------------------
bool CDetectorBurst::CanFire(bool considerAmmo) const
{
	return !m_firing;
}

//------------------------------------------------------------------------
void CDetectorBurst::StartFire()
{
	if (!CanFire(true))
		return;

	m_firing=true;

}

//------------------------------------------------------------------------
void CDetectorBurst::StopFire()
{
	if (m_firing)
	{
		m_pWeapon->PlayAction(m_fireParams->actions.blast);

		m_blastTimer = m_fireParams->detectorburstparams.blastDelay;
		if(m_blastTimer <= 0.0f)
		{
			m_blastTimer = -1.0f;
			Blast(m_fireParams->detectorburstparams.impulse);
		}
		else
		{
			m_pWeapon->RequireUpdate(eIUS_FireMode);
		}

	}

	m_firing = false;
}

//------------------------------------------------------------------------
void CDetectorBurst::Blast(float impulse)
{
	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	if (!pMC)
		return;

	SMovementState info;
	pMC->GetMovementState(info);

	const Vec3 boxSize = m_fireParams->detectorburstparams.boxSize;
	const Vec3 boxCenter = info.pos + info.eyeDirection * boxSize.y * 0.5f;
	const Vec3 r(boxSize.y * 0.5f, boxSize.y * 0.5f, boxSize.y * 0.5f);
	const Vec3 boxMin(boxCenter-r);
	const Vec3 boxMax(boxCenter+r);

	IPhysicalEntity *pSkip[5]={0};
	CSingle::GetSkipEntities(m_pWeapon, &pSkip[0], 4);

	IPhysicalEntity **pList=0;
	int n = gEnv->pPhysicalWorld->GetEntitiesInBox(boxMin, boxMax, pList, ent_living | ent_rigid | ent_sleeping_rigid);
	if (n > 0)
	{

		AABB box;
		box.min(-boxSize.x*0.5f, 0.0f, -boxSize.z*0.5f);
		box.max(boxSize.x*0.5f, boxSize.y, boxSize.z*0.5f);

		OBB obb(OBB::CreateOBBfromAABB(Matrix33::CreateRotationVDir(info.eyeDirection), box));

		pe_status_pos ep;
		pe_params_bbox ebb;

		VectorSet<EntityId> processedEntities;
		processedEntities.reserve(n);
		for (int i=0;i<n; i++)
		{
			if (pList[i]==pSkip[0]||
				pList[i]==pSkip[1]||
				pList[i]==pSkip[2]||
				pList[i]==pSkip[3]||
				pList[i]==pSkip[4])
				continue;

			if(IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pList[i]))
			{
				if(processedEntities.find(pEntity->GetId()) != processedEntities.end())
					continue;
				processedEntities.insert(pEntity->GetId());
			}

			if (pList[i]->GetForeignData(PHYS_FOREIGN_ID_ENTITY)==pSkip[0]->GetForeignData(PHYS_FOREIGN_ID_ENTITY))
				continue;

			if (!pList[i]->GetStatus(&ep))
				continue;
			if (!pList[i]->GetParams(&ebb))
				continue;


			AABB aabb;
			aabb.min=ebb.BBox[0];
			aabb.max=ebb.BBox[1];

			if (Overlap::AABB_OBB(aabb, info.eyePosition, obb))
				Contact(pList[i], info.eyeDirection, impulse);
		}
	}

	int slot = m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
	int id = m_pWeapon->GetStats().fp ? 0 : 1;

	if (!m_fireParams->detectorbursteffectparams.effect.empty())
	{
		IParticleEffect* pParticle = gEnv->pParticleManager->FindEffect(m_fireParams->detectorbursteffectparams.effect.c_str());
		if(pParticle)
		{
			Matrix34 fxMtx = Matrix34::CreateFromVectors(info.eyeDirection.Cross(Vec3Constants<float>::fVec3_OneZ), 
				info.eyeDirection, Vec3Constants<float>::fVec3_OneZ, info.eyePosition + m_fireParams->detectorbursteffectparams.offset);
			pParticle->Spawn(true, fxMtx);
		}
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), 0, 0, info.weaponPosition, info.fireDirection.normalized(), ZERO);
}

//------------------------------------------------------------------------
void CDetectorBurst::Contact(IPhysicalEntity *pPhysicalEntity, const Vec3 &dir, float impulse, int partId, Vec3 point)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);
	CActor *pActor = NULL;

	if(pEntity)
	{
		if (IActor *pIActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()))
			pActor = static_cast<CActor *>(pIActor);
		else if (IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId()))
			impulse *= m_fireParams->detectorburstparams.impulse_vehicle_scale;
	}

	if (pActor)
	{
		if(pActor->HasNanoSuit())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_DISABLE_SUIT;
			event.fParam = m_fireParams->detectorburstparams.empEffectTime;
			event.sParam = "Suit_Disabled";
			pActor->SendActorSuitEvent(event);
		}
	
		//return;
	
		pActor->Fall(Vec3Constants<float>::fVec3_Zero, true);

		int boneId = pActor->GetBoneID(BONE_SPINE);
		Vec3 bone(ZERO);

		ICharacterInstance *pCharacter=pActor->GetEntity()->GetCharacter(0);
		assert(pCharacter);

		// directional impulse
		pe_action_impulse bai;
		bai.partid=partId>-1?partId:boneId;
		bai.impulse=dir*impulse*m_fireParams->detectorburstparams.impulse_human_scale;
		bai.point=partId>-1?point:bone;
		pCharacter->GetISkeletonPose()->GetCharacterPhysics()->Action(&bai);

	}
	else
	{
		pe_action_impulse ai;
		ai.partid=partId;
		if (partId>-1)
			ai.point=point;
		ai.impulse=dir*impulse;

		pPhysicalEntity->Action(&ai);
	}

	//Generate hit as well
	if(pEntity)
	{
		HitInfo hitInfo(m_pWeapon->GetOwnerId(), pEntity->GetId(), m_pWeapon->GetEntityId(),
			m_fireParams->detectorburstparams.damage, 0.0f, 0, -1,g_pGame->GetGameRules()->GetHitTypeId(m_fireParams->detectorburstparams.hit_type.c_str()), point, dir, -dir);

		g_pGame->GetGameRules()->ClientHit(hitInfo);
	}

}


//-----------------------------------------------------------
void CDetectorBurst::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));
	CFireMode::GetInternalMemoryUsage(s);	// collect memory of parent class
}

//-----------------------------------------------------------
void CDetectorBurst::OnZoomStateChanged()
{

}