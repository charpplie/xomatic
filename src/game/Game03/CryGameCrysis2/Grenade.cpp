/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Grenades

-------------------------------------------------------------------------
History:
- 11:12:2009  10:30 : Created by Claire Allan

*************************************************************************/
#include "StdAfx.h"
#include "Grenade.h"
#include <IVehicleSystem.h>
#include "GameRules.h"
#include "IAIObject.h"
#include "HUD/HUD.h"

//------------------------------------------------------------------------
CGrenade::CGrenade()
{
}

//------------------------------------------------------------------------
CGrenade::~CGrenade()
{
}

//------------------------------------------------------------------------
void CGrenade::HandleEvent(const SGameObjectEvent &event)
{
	if (CheckAnyProjectileFlags(ePFlag_destroying))
		return;

	if (event.event == eGFE_OnCollision)
	{
		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;

		int srcId = 0;
		int trgId = 1;
		IEntity* pTargetEntity = NULL;

		// Notify AI system about grenades.
		if (gEnv->pAISystem)
		{
			IAIObject* pAI = 0;
			if ((pAI = GetEntity()->GetAI()) != NULL && pAI->GetAIType() == AIOBJECT_GRENADE)
			{
				// Associate event with vehicle if the shooter is in a vehicle (tank cannon shot, etc)
				EntityId ownerId = m_ownerId;
				IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(ownerId);
				if (pActor && pActor->GetLinkedVehicle() && pActor->GetLinkedVehicle()->GetEntityId())
					ownerId = pActor->GetLinkedVehicle()->GetEntityId();

				SAIStimulus stim(AISTIM_GRENADE, AIGRENADE_COLLISION, ownerId, GetEntityId(),
					GetEntity()->GetWorldPos(), ZERO, 12.0f);
				gEnv->pAISystem->RegisterStimulus(stim);
			}
		}

		if (m_pAmmoParams && m_pAmmoParams->pGrenadeParams)
		{
			if(!CheckAnyProjectileFlags(ePFlag_collided))
			{
				if(m_pAmmoParams->pGrenadeParams->onImpactLifetime > 0.f)
				{
					SetLifeTime(m_pAmmoParams->pGrenadeParams->onImpactLifetime);
				}

				if(m_pAmmoParams->pGrenadeParams->collisionDamage > 0.f)
				{
					ResolveTarget(pCollision, trgId, srcId, pTargetEntity);

					if(pTargetEntity && g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId()))
					{
						CGameRules *pGameRules = g_pGame->GetGameRules();
						int hitMatId = pGameRules->GetHitMaterialIdFromSurfaceId(pCollision->idmat[1]);

						Vec3 dir(0, 0, 0);
						if (pCollision->vloc[srcId].GetLengthSquared() > 1e-6f)
							dir = pCollision->vloc[srcId].GetNormalized();

						HitInfo hitInfo(m_ownerId, pTargetEntity->GetId(), m_weaponId, m_pAmmoParams->pGrenadeParams->collisionDamage, 0.0f, hitMatId, pCollision->partid[1],
							m_hitTypeId, pCollision->pt, dir, pCollision->n);

						hitInfo.remote = IsRemote();
						hitInfo.projectileId = GetEntityId();

						pGameRules->ClientHit(hitInfo);	
					}
				}
			}

			if(m_pAmmoParams->pGrenadeParams->detonateOnImpact || m_pAmmoParams->pGrenadeParams->detonateOnActorImpact)
			{
				ResolveTarget(pCollision, trgId, srcId, pTargetEntity);

				if(ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(pCollision->idmat[trgId]))
				{
					if(pSurfaceType->GetBreakability()<=0)
					{
						if(m_pAmmoParams->pGrenadeParams->detonateOnImpact || (pTargetEntity && g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId())))
						{
							Explode(true, true, pCollision->pt, pCollision->n, pCollision->vloc[srcId], pTargetEntity ? pTargetEntity->GetId() : NULL);
						}
					}
				}
			}
		}
	}

	BaseClass::HandleEvent(event);
}

//------------------------------------------------------------------------
void CGrenade::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale /*=1.0f*/)
{
	BaseClass::Launch(pos, dir, velocity, speedScale);

	if(!gEnv->bMultiplayer)
	{
		OnLaunch();

		IAIObject* pAI = 0;
		if ((pAI = GetEntity()->GetAI()) != NULL && pAI->GetAIType() == AIOBJECT_GRENADE)
		{
			IEntity *pOwnerEntity = gEnv->pEntitySystem->GetEntity(m_ownerId);
			pe_status_dynamics dyn;
			pe_status_dynamics dynProj;
			if (pOwnerEntity->GetPhysics() 
				&& pOwnerEntity->GetPhysics()->GetStatus(&dyn) 
				&& GetEntity()->GetPhysics()->GetStatus(&dynProj))
			{

				Vec3 ownerVel( dyn.v);
				Vec3 grenadeDir(dynProj.v.GetNormalizedSafe());

				// Trigger the signal at the predicted landing position.
				Vec3 predictedPos = pos;
				float dummySpeed;
				if (GetWeapon())
				GetWeapon()->PredictProjectileHit(pOwnerEntity->GetPhysics(), pos, dir, velocity, speedScale * m_pAmmoParams->speed, predictedPos, dummySpeed);
				
				// Associate event with vehicle if the shooter is in a vehicle (tank cannon shot, etc)
				EntityId ownerId = pOwnerEntity->GetId();
				IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(ownerId);
				if (pActor && pActor->GetLinkedVehicle() && pActor->GetLinkedVehicle()->GetEntityId())
					ownerId = pActor->GetLinkedVehicle()->GetEntityId();

				SAIStimulus stim(AISTIM_GRENADE, AIGRENADE_THROWN, ownerId, GetEntityId(),
														predictedPos, ZERO, 20.0f);
				gEnv->pAISystem->RegisterStimulus(stim);
			}
		}
	}
}



bool CGrenade::ShouldKnockTarget() const
{
	return true;
}
