/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: KVolt bullet

-------------------------------------------------------------------------
History:
- 13:05:2009   15:00 : Created by Claire Allan

***********************************************************************/
#include "StdAfx.h"
#include "KVoltBullet.h"
#include <IRenderAuxGeom.h>
#include "Actor.h"
#include "GameRules.h"
#include "Player.h"
#include "WeaponSystem.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

SKVoltParams CKVoltBullet::s_kvoltParams;

//------------------------------------------------------------------------
CKVoltBullet::CKVoltBullet()
{  
}


//------------------------------------------------------------------------
CKVoltBullet::~CKVoltBullet()
{
	Destroy();
}

//------------------------------------------------------------------------
bool CKVoltBullet::Init(IGameObject *pGameObject)
{
	if (inherited::Init(pGameObject))
	{
		if(s_kvoltParams.isInitialised == false)
		{
			s_kvoltParams.damageRadius							= GetParam("DamageRadius", s_kvoltParams.damageRadius );
			s_kvoltParams.directHitDamageMultiplier = GetParam("DirectHitDamageMultiplier", s_kvoltParams.directHitDamageMultiplier );

			s_kvoltParams.pHitEffect						= m_pAmmoParams->GetCachedEffect("EffectHit");
			s_kvoltParams.pEnemyHitEffect				= m_pAmmoParams->GetCachedEffect("EnemyEffectHit");

			s_kvoltParams.isInitialised = true;
		}

		return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CKVoltBullet::HandleEvent(const SGameObjectEvent& event)
{
	if(event.event == eGFE_OnCollision)
	{
		EventPhysCollision *pCollision = (EventPhysCollision* )event.ptr;
		SendClientPerkEvent(EPE_BulletTrail, pCollision->pt);

		// switch collision target id's to ensure target is not the bullet itself
		int trgId = 1;
		int srcId = 0;
		IPhysicalEntity *pTarget = pCollision->pEntity[trgId];

		if (pTarget == GetEntity()->GetPhysics())
		{
			trgId = 0;
			srcId = 1;
			pTarget = pCollision->pEntity[trgId];
		}

		//Do not stick to breakable glass
		if(ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(pCollision->idmat[trgId]))
		{
			if(pSurfaceType->GetBreakability()==1)
			{
				return;
			}
		}

		Trigger(pCollision->pt);
		
		if(!IsRemote())
		{
			IEntity *pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;
			EntityId targetId =  pTargetEntity ? pTargetEntity->GetId() : 0;
			Vec3 dir(0, 0, 0);
			
			if (pCollision->vloc[srcId].GetLengthSquared() > 1e-6f)
			{
				dir = pCollision->vloc[srcId].GetNormalized();
			}

			DamageEnemiesInRange(s_kvoltParams.damageRadius, pCollision->pt, targetId);
			ProcessDamage(targetId, m_damage * s_kvoltParams.directHitDamageMultiplier, pCollision->partid[trgId], pCollision->pt, dir);
		}
	}

	inherited::HandleEvent(event);
}

//-------------------------------------------------------------------------
void CKVoltBullet::Trigger(Vec3 collisionPos)
{
	CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_ownerId));
	bool friendly = pActor && pActor->IsFriendlyEntity(g_pGame->GetIGameFramework()->GetClientActorId());

	IParticleEffect* pHitEffect = friendly ? s_kvoltParams.pHitEffect : s_kvoltParams.pEnemyHitEffect;

	if(pHitEffect)
	{
		CCCPOINT(KVolt_SpawnDestroyParticle);
		pHitEffect->Spawn(true, IParticleEffect::ParticleLoc(collisionPos));
	}

	Destroy();
}

//-----------------------------------------------------------------------
int CKVoltBullet::GetActorsInArea(float range, Vec3 pos, IPhysicalEntity**& pPhysicalEntities)
{
	const Vec3& center = pos;
	const Vec3& radiusVec = Vec3(range, range, range);
	const Vec3 boxMin = center - radiusVec;
	const Vec3 boxMax = center + radiusVec;
		
	return gEnv->pPhysicalWorld->GetEntitiesInBox(boxMin, boxMax, pPhysicalEntities, ent_living);
}

//-----------------------------------------------------------------------
void CKVoltBullet::DamageEnemiesInRange(float range, Vec3 pos, EntityId ignoreId)
{
	IPhysicalEntity** pPhysicalEntities = 0;
	const int numEntities = GetActorsInArea(range, pos, pPhysicalEntities);

	for (int i = 0; i < numEntities; ++i)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntities[i]);

		if (pEntity)
		{
			CCCPOINT(KVolt_DamageEnemy);

			unsigned int targetId = pEntity->GetId();

			if(targetId != ignoreId)
			{
				CActor* pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetId));

				if (pActor && (pActor->GetHealth() > 0) && !pActor->IsFriendlyEntity(m_ownerId))
				{
					Vec3 pos = pEntity->GetWorldPos();
					Vec3 dir = pos - m_initial_pos;

					ProcessDamage(targetId, (float)m_damage, -1, pos, dir);
				}
			}
		}
	}
}

void CKVoltBullet::ProcessDamage(EntityId targetId, float damage, int hitPartId, Vec3 pos, Vec3 dir)
{
	if(targetId)
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();

		HitInfo hitInfo(m_ownerId, targetId, m_weaponId, damage, 0.0f, 0, hitPartId, m_hitTypeId, pos, dir, -dir);

		hitInfo.remote = IsRemote(); 
		hitInfo.projectileId = GetEntityId();
		hitInfo.bulletType = m_pAmmoParams->bulletType;

		pGameRules->ClientHit(hitInfo);
	}
}