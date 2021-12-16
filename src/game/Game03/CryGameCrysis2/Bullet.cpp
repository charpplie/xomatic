/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:10:2005   14:14 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Bullet.h"
#include "GameRules.h"
#include <IEntitySystem.h>
#include <IGameTokens.h>
#include <IMaterialEffects.h>
#include <IRenderAuxGeom.h>
#include "AmmoParams.h"
#include "WeaponSystem.h"
#include <ICryAnimation.h>
#include "IAIObject.h"

int CBullet::s_waterMaterialId = -1;
IEntityClass* CBullet::EntityClass = 0;

#ifdef DEBUG_BULLET_PENETRATION
	SDebugBulletPenetration CBullet::s_debugBulletPenetration;
#endif

//------------------------------------------------------------------------
CBullet::CBullet()
: m_damageFallOffAmount(0.0f)
, m_damageFallOffStart(0.0f)
, m_damageFalloffMin(0.0f)
, m_accumulatedDamageFallOffAfterPenetration(0.0f)
, m_bulletPierceability(0)
, m_penetrationCount(0)
, m_alive(true)
{

}

//------------------------------------------------------------------------
CBullet::~CBullet()
{
}

//--------------------------------------------------------------------------
bool CBullet::Init(IGameObject *pGameObject)
{
	if(s_waterMaterialId == -1)
	{
		ISurfaceType* pWaterSurface = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_water");
		if(pWaterSurface)
			s_waterMaterialId = pWaterSurface->GetId();
		else
			GameWarning("mat_water suface type not found!!");
	}

	return BaseClass::Init(pGameObject);
}

//------------------------------------------------------------------------
void CBullet::SetParams(EntityId ownerId, EntityId hostId, EntityId weaponId, int damage, float damageFallOffStart, float damageFallOffAmount, float damageFallOffMin, int hitTypeId, int8 bulletPierceabilityModifier)
{
	m_damageFallOffAmount = damageFallOffAmount;
	m_damageFallOffStart = damageFallOffStart;
	m_damageFalloffMin = damageFallOffMin;

	BaseClass::SetParams(ownerId, hostId, weaponId, damage, damageFallOffStart, damageFallOffAmount, damageFallOffMin, hitTypeId, bulletPierceabilityModifier);
}

//------------------------------------------------------------------------
int CBullet::GetRopeBoneId(const EventPhysCollision& collision, IEntity& target, IPhysicalEntity* pRopePhysicalEntity) const
{
	int boneId = -1;

	ICharacterInstance* pCharacterInstance = target.GetCharacter(0);
	if (!pCharacterInstance)
		return boneId;

	ISkeletonPose* pSkeletonPose = pCharacterInstance->GetISkeletonPose();
	if (!pSkeletonPose)
		return boneId;

	int auxPhys = 0;
	while (IPhysicalEntity* pPhysicalEntity = pSkeletonPose->GetCharacterPhysics(auxPhys))
	{
		if (pRopePhysicalEntity == pPhysicalEntity)
		{
			boneId = pSkeletonPose->GetAuxPhysicsBoneId(auxPhys, collision.partid[1]);
			break;
		}
		++auxPhys;
	}

	return boneId;
}

//------------------------------------------------------------------------
void CBullet::ProcessHit(CGameRules& gameRules, const EventPhysCollision& collision, IEntity& target, float damage, int hitMatId, const Vec3& hitDir)
{
	if(damage > 0.f)
	{
		EntityId targetId = target.GetId();
		int numHitActors = m_hitActors.size();
		bool alreadyHit = false;

		for (int i = 0; i < numHitActors; i++)
		{
			if (m_hitActors[i] == targetId)
			{
				alreadyHit = true;
				break;
			}
		}

		if(!alreadyHit)
		{
			HitInfo hitInfo(m_ownerId, targetId, m_weaponId,
				damage, 0.0f, hitMatId, collision.partid[1],
				m_hitTypeId, collision.pt, hitDir, collision.n);

			hitInfo.remote = IsRemote();
			hitInfo.projectileId = GetEntityId();
			hitInfo.bulletType = m_pAmmoParams->bulletType;
			hitInfo.knocksDown = CheckAnyProjectileFlags(ePFlag_knocksTarget) && ( damage > m_minDamageForKnockDown );
			hitInfo.knocksDownLeg = m_chanceToKnockDownLeg>0 && damage>m_minDamageForKnockDownLeg && m_chanceToKnockDownLeg>Random(100);

			IPhysicalEntity* pPhysicalEntity = collision.pEntity[1];
			if (pPhysicalEntity && pPhysicalEntity->GetType() == PE_ROPE)
			{
				hitInfo.partId = GetRopeBoneId(collision, target, pPhysicalEntity);
			}

			gameRules.ClientHit(hitInfo);

			m_hitActors.push_back(targetId);
		}
	}
}

//------------------------------------------------------------------------
void CBullet::HandleEvent(const SGameObjectEvent &event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	BaseClass::HandleEvent(event);

	if (event.event == eGFE_OnCollision)
	{
		if (CheckAnyProjectileFlags(ePFlag_destroying))
			return;

		EventPhysCollision *pCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);
		if (!pCollision)
			return;

		float finalDamage = GetFinalDamage(pCollision->pt);
		if (finalDamage <= 0.0f)
			m_alive = false;
        
		IEntity *pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1]:0;
		CGameRules *pGameRules = g_pGame->GetGameRules();
		int hitMatId = pGameRules->GetHitMaterialIdFromSurfaceId(pCollision->idmat[1]);

		Vec3 hitDir(0, 0, 0);
		if (pCollision->vloc[0].GetLengthSquared() > 1e-6f)
		{
			hitDir = pCollision->vloc[0].GetNormalized();
		}

		//================================= Process Hit =====================================
		//Only process hits that have a target
		if(pTarget)
		{
			if(FilterFriendlyAIHit(pTarget) == false)
			{
				ProcessHit(*pGameRules, *pCollision, *pTarget, finalDamage, hitMatId, hitDir);
			}
		}
		//====================================~ Process Hit ======================================

		//==================================== Notify AI    ======================================
		if (gEnv->pAISystem)
		{
			if (gEnv->pEntitySystem->GetEntity(m_ownerId))
			{
				ISurfaceType *pSurfaceType = pGameRules->GetHitMaterial(hitMatId);
				const ISurfaceType::SSurfaceTypeAIParams* pParams = pSurfaceType ? pSurfaceType->GetAIParams() : 0;
				const float radius = pParams ? pParams->fImpactRadius : 2.5f;
				const float soundRadius = pParams ? pParams->fImpactSoundRadius : 20.0f;

				SAIStimulus stim(AISTIM_BULLET_HIT, 0, m_ownerId, pTarget ? pTarget->GetId() : 0, pCollision->pt, pCollision->vloc[0].GetNormalizedSafe(ZERO), radius);
				gEnv->pAISystem->RegisterStimulus(stim);

				SAIStimulus stimSound(AISTIM_SOUND, AISTIM_BULLET_HIT, m_ownerId, 0, pCollision->pt, ZERO, soundRadius);
				gEnv->pAISystem->RegisterStimulus(stim);
			}
		}
		//=========================================~ Notify AI ===============================

		//========================================= Surface Pierceability ==============================
		if (pCollision->pEntity[0]->GetType() == PE_PARTICLE)
		{
			//If collided water
			if(pCollision->idmat[1] == s_waterMaterialId)
			{
				EmitUnderwaterTracer(pCollision->pt, pCollision->pt + (pCollision->vloc[0].GetNormalizedSafe() * 100.0f));
				return;
			}

			bool bulletPenetrationDisabled = (g_pGameCVars->g_bulletPenetrationEnable == 0);
			if (bulletPenetrationDisabled)
			{
				float bouncy, friction;
				uint32	pierceabilityMat;
				gEnv->pPhysicalWorld->GetSurfaceParameters(pCollision->idmat[1], bouncy, friction, pierceabilityMat);
				pierceabilityMat &= sf_pierceable_mask;

				if ((pCollision->idCollider == -1) || (pierceabilityMat <= GetBulletPierceability()))
				{
					//Update entity position before destroy for bullet whiz/threat trails
					GetEntity()->SetPos(pCollision->pt);	
					Destroy();
				}

			}
			else
			{
				HandlePierceableSurface(pCollision, pTarget, hitDir);

				if (ShouldDestroyBullet())
				{
					//Update entity position before destroy for bullet whiz/threat trails
					GetEntity()->SetPos(pCollision->pt);	
					Destroy();
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBullet::EmitUnderwaterTracer(const Vec3& pos, const Vec3& destination)
{
	const STrailParams* pTrail = m_pAmmoParams->pTrailUnderWater;

	if (!pTrail)
		return;

	CTracerManager::STracerParams params;
	params.position = pos;
	params.destination = destination;
	params.geometry = NULL;
	params.effect = pTrail->effect;
	params.speed = 35.0f;
	params.lifetime = 2.5f;
	params.delayBeforeDestroy = 0.f;

	g_pGame->GetWeaponSystem()->GetTracerManager().EmitTracer(params);
}

//////////////////////////////////////////////////////////////////////////
bool CBullet::FilterFriendlyAIHit(IEntity* pHitTarget)
{
	if (gEnv->bMultiplayer)
		return false;

	IActorSystem* pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
	IActor* pOwnerActor = pActorSystem->GetActor(m_ownerId);

	//Filter client hits against friendly AI
	if ((pOwnerActor != NULL) && (pOwnerActor->IsClient()))
	{
		IActor* pAITargetActor = pActorSystem->GetActor(pHitTarget->GetId());
		if(pAITargetActor && pHitTarget->GetAI() && !pHitTarget->GetAI()->IsHostile(pOwnerActor->GetEntity()->GetAI(),false))
		{
			g_pGame->GetGameRules()->SetEntityToIgnore(pHitTarget->GetId());
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
float CBullet::GetFinalDamage( const Vec3& hitPos ) const
{
	float damage = (float)m_damage - m_accumulatedDamageFallOffAfterPenetration;

	if(m_damageFallOffAmount > 0.f)
	{
		float distTravelledSq = (m_initial_pos - hitPos).GetLengthSquared();
		if(distTravelledSq > (m_damageFallOffStart*m_damageFallOffStart))
		{
			float distTravelled = cry_sqrtf(distTravelledSq);
			distTravelled -= m_damageFallOffStart;
			damage -= distTravelled * m_damageFallOffAmount;
		}
	}

	damage = max(damage, m_damageFalloffMin);

	return damage;
}

//////////////////////////////////////////////////////////////////////////
void CBullet::HandlePierceableSurface( const EventPhysCollision* pCollision, IEntity* pHitTarget, const Vec3& hitDirection )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	const SPierceabilityParams& pierceabilityParams = m_pAmmoParams->pierceabilityParams;
	
	const float maxPenetrationCount = 4;
	const float entryAngleDot = pCollision->n.Dot(hitDirection);
	bool backFace = (entryAngleDot >= 0);

#ifdef DEBUG_BULLET_PENETRATION
	bool debugBulletPenetration = (g_pGameCVars->g_bulletPenetrationDebug != 0);
#endif

	if (backFace == false)
	{
		//Front face hit, accumulate damage falloff after penetration
		float bouncy, friction;
		uint32	pierceabilityMat;
		gEnv->pPhysicalWorld->GetSurfaceParameters(pCollision->idmat[1], bouncy, friction, pierceabilityMat);
		pierceabilityMat &= sf_pierceable_mask;

#ifdef DEBUG_BULLET_PENETRATION
		const float damageBeforePenetration = GetDamageAfterPenetrationFallOff();
#endif

		m_penetrationCount++;

		//1- Check if collided surface might stop the bullet
		bool collisionStopsBullet = (pCollision->idCollider == -1) || (pierceabilityMat <= GetBulletPierceability()) || (m_penetrationCount >= maxPenetrationCount);
		if (collisionStopsBullet)
		{
#ifdef DEBUG_BULLET_PENETRATION
			if (debugBulletPenetration)
			{
				s_debugBulletPenetration.AddBulletHit(pCollision->pt, hitDirection, damageBeforePenetration, (pCollision->idCollider == -1) ? -1 : pierceabilityMat, false, true, false);
			}
#endif
			m_accumulatedDamageFallOffAfterPenetration += (float)m_damage;
			return;
		}

		//2- If not stopped, add fall off damage, and see if can still penetrate
		m_accumulatedDamageFallOffAfterPenetration += (float)m_damage * (pierceabilityParams.GetDamageFallOffForPierceability(pierceabilityMat) * 0.01f);
		
		bool needsBackFaceCheck = (GetDamageAfterPenetrationFallOff() > 0.0f) && pierceabilityParams.SurfaceRequiresBackFaceCheck(pierceabilityMat);

#ifdef DEBUG_BULLET_PENETRATION
		if (debugBulletPenetration)
		{
			if (ShouldDestroyBullet())
			{
				s_debugBulletPenetration.AddBulletHit(pCollision->pt, hitDirection, damageBeforePenetration, pierceabilityMat, false, true, false);
			}
		}
#endif

		if (needsBackFaceCheck)
		{
			//3- Raytrace backwards, to check thickness & exit point if any
			const float angleFactor = 1.0f/max(0.2f, -entryAngleDot);
			const float distCheck = pierceabilityParams.maxPenetrationThickness * angleFactor;

			SBackHitInfo hit;
			bool exitPointFound = RayTraceGeometry(pCollision, pCollision->pt + (hitDirection * (distCheck + 0.035f)), -hitDirection * distCheck ,&hit);

			if (exitPointFound)
			{
				//Exit point found, spawn effect
				IMaterialEffects* pMaterialEffects = g_pGame->GetIGameFramework()->GetIMaterialEffects();
				TMFXEffectId effectId = pMaterialEffects->GetEffectId(GetEntity()->GetClass(), pCollision->idmat[1]);
				if (effectId != InvalidEffectId)
				{
					SMFXRunTimeEffectParams params;
					params.src = GetEntityId();
					params.trg = pHitTarget ? pHitTarget->GetId() : 0;
					params.srcSurfaceId = pCollision->idmat[0];
					params.trgSurfaceId = pCollision->idmat[1]; 
					params.soundSemantic = eSoundSemantic_Physics_Collision;
					params.srcRenderNode = (pCollision->iForeignData[0] == PHYS_FOREIGN_ID_STATIC) ? (IRenderNode*)pCollision->pForeignData[0] : NULL;
					params.trgRenderNode = (pCollision->iForeignData[1] == PHYS_FOREIGN_ID_STATIC) ? (IRenderNode*)pCollision->pForeignData[1] : NULL;
					params.pos = hit.pt;
					params.normal = hitDirection; //Use bullet direction, more readable for exits than normal
					params.partID = pCollision->partid[1];
					params.dir[0] = -hitDirection;
					params.playflags = MFX_PLAY_ALL&(~MFX_PLAY_SOUND); //Do not play the sound on backface

					pMaterialEffects->ExecuteEffect(effectId, params);
				}

#ifdef DEBUG_BULLET_PENETRATION
				if (debugBulletPenetration)
				{
					s_debugBulletPenetration.AddBulletHit(pCollision->pt, hitDirection, damageBeforePenetration, pierceabilityMat, false, false, false);
					s_debugBulletPenetration.AddBulletHit(hit.pt, hitDirection, GetDamageAfterPenetrationFallOff(), pierceabilityMat, true, false, false);
				}
#endif
			}
			else
			{
#ifdef DEBUG_BULLET_PENETRATION
				if (debugBulletPenetration)
				{
					s_debugBulletPenetration.AddBulletHit(pCollision->pt, hitDirection, damageBeforePenetration, pierceabilityMat, false, true, true);
				}
#endif
				//Surface must be too thick, add enough fall off to destroy the bullet
				m_accumulatedDamageFallOffAfterPenetration += (float)m_damage;
			}

		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBullet::ShouldDestroyBullet() const
{
	return (GetDamageAfterPenetrationFallOff() <= 0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CBullet::ReInitFromPool()
{
	BaseClass::ReInitFromPool();

	m_accumulatedDamageFallOffAfterPenetration = 0.0f;
	m_penetrationCount = 0;
	m_alive = true;
	m_hitActors.clear();
}

//////////////////////////////////////////////////////////////////////////
bool CBullet::IsAlive() const
{
	return m_alive;
}

//////////////////////////////////////////////////////////////////////////
void CBullet::SetUpParticleParams(IEntity* pOwnerEntity, uint8 pierceabilityModifier)
{
	CRY_ASSERT(m_pPhysicalEntity);

	pe_params_particle pparams;
	pparams.pColliderToIgnore = pOwnerEntity ? pOwnerEntity->GetPhysics() : NULL;
	if (m_pAmmoParams)
	{
		bool bulletPenetrationDisabled = (g_pGameCVars->g_bulletPenetrationEnable == 0);
		if (bulletPenetrationDisabled)
		{
			pparams.iPierceability = max(0, min(m_pAmmoParams->pParticleParams->iPierceability + pierceabilityModifier, sf_max_pierceable));
		}
		else
		{
			pparams.iPierceability = max(0, min(m_pAmmoParams->pierceabilityParams.pierceability + pierceabilityModifier, sf_max_pierceable));
		}
		m_bulletPierceability = pparams.iPierceability;
	}
	m_pPhysicalEntity->SetParams(&pparams);
}

//////////////////////////////////////////////////////////////////////////

struct SPhysicsRayWrapper
{
	SPhysicsRayWrapper()
		: m_pRay(NULL)
	{
		primitives::ray rayData; 
		rayData.origin.zero(); 
		rayData.dir = FORWARD_DIRECTION;
		m_pRay = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::ray::type, &rayData);
	}

	~SPhysicsRayWrapper()
	{
		if (m_pRay)
		{
			m_pRay->Release();
			m_pRay = NULL;
		}
	}

	IGeometry* GetRay(const Vec3& pos, const Vec3& dir)
	{
		primitives::ray rayData;
		rayData.origin = pos;
		rayData.dir = dir;
		m_pRay->SetData(&rayData);

		return m_pRay;
	}

	void ResetRay()
	{
		primitives::ray rayData; 
		rayData.origin.zero(); 
		rayData.dir = FORWARD_DIRECTION;
		m_pRay->SetData(&rayData);
	}

private:

	IGeometry*	m_pRay;
};

bool CBullet::RayTraceGeometry( const EventPhysCollision* pCollision, const Vec3& pos, const Vec3& hitDirection, SBackHitInfo* pBackHitInfo )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	static SPhysicsRayWrapper physicsRayWrapper;
	
	bool exitPointFound = false;

	IPhysicalEntity* pCollider = pCollision->pEntity[1];
	assert(pCollider);

	pe_params_part partParams;
	partParams.partid = pCollision->partid[1];
	pe_status_pos posStatus;

	if (pCollider->GetParams(&partParams) && pCollider->GetStatus(&posStatus))
	{
		if (partParams.pPhysGeom && partParams.pPhysGeom->pGeom)
		{
			geom_world_data geomWorldData;
			geomWorldData.R = Matrix33(posStatus.q*partParams.q);
			geomWorldData.offset = posStatus.pos + (posStatus.q * partParams.pos);
			geomWorldData.scale = posStatus.scale * partParams.scale;

			geom_contact *pContacts;
			intersection_params intersectionParams;
			IGeometry* pRayGeometry = physicsRayWrapper.GetRay(pos, hitDirection);
			const Vec3 hitDirectionNormalized = hitDirection.GetNormalized();

			const int contactCount = partParams.pPhysGeom->pGeom->Intersect(pRayGeometry,&geomWorldData,0,&intersectionParams,pContacts);
			if (contactCount > 0)
			{
				WriteLockCond lockContactsBuffer(*intersectionParams.plock,0); lockContactsBuffer.SetActive(1);

				float bestDistance = 10.0f;
				
				for (int i = (contactCount-1); (i >= 0) && (pContacts[i].t < bestDistance) && ((pContacts[i].n*hitDirectionNormalized) < 0); i--)
				{
					bestDistance = (float)pContacts[i].t;
					pBackHitInfo->pt = pContacts[i].pt;
					exitPointFound = true;
				}
			}
		}
	}

	physicsRayWrapper.ResetRay();

	return exitPointFound;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_BULLET_PENETRATION

void SDebugBulletPenetration::AddBulletHit( const Vec3& hitPosition, const Vec3& hitDirection, float currentDamage, int8 surfacePierceability, bool isBackFace, bool stoppedBullet, bool tooThick )
{
	if (m_nextHit == MAX_DEBUG_BULLET_HITS)
	{
		m_nextHit = 0;
	}

	m_hitsList[m_nextHit].hitPosition = hitPosition;
	m_hitsList[m_nextHit].bulletDirection = hitDirection;
	m_hitsList[m_nextHit].damage = currentDamage;
	m_hitsList[m_nextHit].isBackFaceHit = isBackFace;
	m_hitsList[m_nextHit].stoppedBullet = stoppedBullet;
	m_hitsList[m_nextHit].tooThick = tooThick;
	m_hitsList[m_nextHit].surfacePierceability = surfacePierceability;
	m_hitsList[m_nextHit].lifeTime = (g_pGameCVars->g_bulletPenetrationDebugTimeout > 0.0f) ? g_pGameCVars->g_bulletPenetrationDebugTimeout : DEFAULT_DEBUG_BULLET_HIT_LIFETIME;

	m_nextHit++;
}

//////////////////////////////////////////////////////////////////////////
void SDebugBulletPenetration::Update(float frameTime)
{
	IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

	SAuxGeomRenderFlags oldFlags = pRenderAux->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone); 
	pRenderAux->SetRenderFlags(newFlags);

	const float baseDebugTimeOut = (g_pGameCVars->g_bulletPenetrationDebugTimeout > 0.0f) ? g_pGameCVars->g_bulletPenetrationDebugTimeout : DEFAULT_DEBUG_BULLET_HIT_LIFETIME;

	for (int i = 0; i < MAX_DEBUG_BULLET_HITS; ++i)
	{
		SDebugBulletHit& currentHit = m_hitsList[i];

		if (currentHit.lifeTime <= 0.0f)
		{
			continue;
		}

		currentHit.lifeTime -= frameTime;

		const float alpha = pow((currentHit.lifeTime / baseDebugTimeOut), 4.0f);
		const ColorB red(255, 0, 0, (uint8)(192 * alpha)), green(0, 255, 0, (uint8)(192 * alpha));
		const ColorB& hitColor = currentHit.stoppedBullet ? red : green;
		const Vec3 coneBase = currentHit.isBackFaceHit ? (currentHit.hitPosition + (currentHit.bulletDirection * 0.3f)) : (currentHit.hitPosition - (currentHit.bulletDirection * 0.2f)) ;
		const Vec3 lineEnd = (coneBase - (currentHit.bulletDirection * 0.3f));
		pRenderAux->DrawCone(coneBase, currentHit.bulletDirection, 0.12f, 0.2f, hitColor);
		pRenderAux->DrawLine(coneBase, hitColor, lineEnd, hitColor, 3.0f);

		const Vec3 baseText = (currentHit.isBackFaceHit) ? coneBase + (0.2f * currentHit.bulletDirection)  : lineEnd - (0.3f * currentHit.bulletDirection);
		const Vec3 textLineOffset(0.0f, 0.0f, 0.14f);
		const float textColor[4] = {1.0f, 1.0f, 1.0f, alpha};

		gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * 2.0f), 1.25f, textColor, true, false, "Damage: %.1f", currentHit.damage);
		gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * 3.0f), 1.25f, textColor, true, false, "Pierceability: %d", currentHit.surfacePierceability);
		gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * 4.0f), 1.25f, textColor, true, false, "%s", GetPenetrationLevelByPierceability(currentHit.surfacePierceability));
		gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * 5.0f), 1.25f, textColor, true, false, currentHit.tooThick ? "Too thick!" : "------");

	}

	pRenderAux->SetRenderFlags(oldFlags);
}

//////////////////////////////////////////////////////////////////////////
const char* SDebugBulletPenetration::GetPenetrationLevelByPierceability( int8 surfacePierceability ) const
{
	if (surfacePierceability == -1)
	{
		return "Terrain";
	}
	else if (surfacePierceability == 0)
	{
		return "Non pierceable";
	}
	else if (surfacePierceability < 4)
	{
		return "Level 1";
	}
	else if (surfacePierceability < 7)
	{
		return "Level 2";
	}
	else if (surfacePierceability < 10)
	{
		return "Level 3";
	}

	return "-----";
}

#endif