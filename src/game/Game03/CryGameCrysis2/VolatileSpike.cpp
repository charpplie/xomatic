/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 16:04:2009   12:00 : Created by Claire Allan

*************************************************************************/
#include "StdAfx.h"
#include "VolatileSpike.h"

#include <IRenderAuxGeom.h>
#include "Actor.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "Player.h"

int CVolatileSpike::s_attachNameID = 0;

static const uint32 CHARACTER_TYPES = ent_all;
static const uint32 CHARACTER_FLAGS = rwi_stop_at_pierceable | rwi_ignore_back_faces | (geom_colltype_ray|geom_colltype13)<<rwi_colltype_bit;
static const uint32 STATIC_TYPES = ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid;
static const uint32 STATIC_FLAGS = (geom_colltype_ray|geom_colltype13)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces|8;

CVolatileSpike::CVolatileSpike()
{
	m_characterAttachment = NULL;
	m_numCachedStaticEntities = 0;
	m_floor	= NULL;
	m_backWall = NULL;
	m_explosionTimer = g_pGameCVars->g_volatileSpike.decalEmitterLife;
	m_numDecalsSpawned = 0;
	m_numSteamEmittersSpawned = 0;
	m_stuckLife = 0.0f;
	m_flags = 0;
}

CVolatileSpike::~CVolatileSpike()
{
	//--- TODO Look at this! 
	//--- Directly release the profile manager here, this is because the projectile system is not correctly destroying remote projectiles leaving a hanging
	//--- profile manager. This is not the correct solution but while the system is in flux its not worth spending time on overhaulling the underlying system.
	GetGameObject()->ReleaseProfileManager(this);

	if(m_characterAttachment)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_stuckEntityID);
		ICharacterInstance* pCharacter = pEntity ? pEntity->GetCharacter(0) : NULL;
		IAttachmentManager* pAttachManager = pCharacter ? pCharacter->GetIAttachmentManager() : NULL;

		if (pAttachManager)
		{
			pAttachManager->RemoveAttachmentByInterface(m_characterAttachment);
		}
	}
}

//------------------------------------------------------------------------
void CVolatileSpike::PostInit(IGameObject *pGameObject)
{
	m_stuckEntityID = 0;
	m_stuckJointID = -1;
	m_offsetPosition.Set(0.f,0.f,0.f);
	m_offsetRotation.SetIdentity();
	m_constraintParams.pt[0].Set(0.f,0.f,0.f);
	m_flags = 0;

	inherited::PostInit(pGameObject);
}

//----------------------------------------------
void CVolatileSpike::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	CRY_ASSERT(m_pAmmoParams->pVSpikeParams);

	if(IsFlagSet(VS_STUCK))
	{
		m_stuckLife += ctx.fFrameTime;
	}

	// Spawn explosion visual effect just before actual explosion, so that the acid looks like it has time to travel through the air
	if((!IsFlagSet(VS_HAS_SPAWNED_EXPLOSION_EFFECT)) && (m_stuckLife >= m_pAmmoParams->pVSpikeParams->visualExplosionTime))
	{
		SpawnExplosionEffect();		
	}

	inherited::Update(ctx, updateSlot);

	if (IsFlagSet(VS_CONSTRAINT))
	{
		IActor* pStickedActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_stuckEntityID);

		if(pStickedActor && pStickedActor->GetEntity()->GetPhysics()->Action(&m_constraintParams))
		{
			SetFlag(VS_CONSTRAINT,false);
		}
	}

	if(IsFlagSet(VS_HAS_SPAWNED_EXPLOSION_EFFECT))
	{
		UpdateDecals(ctx);
	}

#if !defined(_RELEASE)
	static bool debugRenderDecals = false;
	if(debugRenderDecals)
	{
		DebugRenderDecals();
	}

	static bool debugRender = false;
	if(debugRender)
	{
		DebugRender();
	}
#endif
}

//------------------------------------------------------------------------
void CVolatileSpike::HandleEvent(const SGameObjectEvent &event)
{
	CRY_ASSERT(m_pAmmoParams->pVSpikeParams);

	if (event.event == eGFE_OnCollision)
	{	
		EventPhysCollision *pCollision = (EventPhysCollision* )event.ptr;
		SendClientPerkEvent(EPE_BulletTrail, pCollision->pt);

		if(!gEnv->bMultiplayer || gEnv->bServer)
		{
			PlayImpactSound();

			GetEntity()->SetTimer(ePTIMER_LIFETIME, m_pAmmoParams->pVSpikeParams->actualExplosionTime);

			// switch collision target id's to ensure target is not the spike itself
			int trgId = 1;
			int srcId = 0;
			IPhysicalEntity *pTarget = pCollision->pEntity[trgId];

			if (pTarget == GetEntity()->GetPhysics())
			{
				trgId = 0;
				srcId = 1;
				pTarget = pCollision->pEntity[trgId];
			}

			IEntity *pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;
			
			if (pTarget && (!pTargetEntity || (pTargetEntity->GetId() != m_ownerId)))
			{
				//Special cases
				if(pTargetEntity)
				{
					CActor* pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId()));

					if(pActor && pActor->GetHealth() > 0)
					{
						if(pActor->IsFriendlyEntity(m_ownerId))
						{
							Explode(true);
							return;
						}

						m_constraintParams.partid[0] = pCollision->partid[trgId];
						m_constraintParams.pt[0] = pCollision->pt;
						m_stuckJointID = pCollision->partid[trgId];
											
						if(StickToCharacter(pTargetEntity,pCollision))
						{
							CGameRules *pGameRules = g_pGame->GetGameRules();
							int hitMatId = pGameRules->GetHitMaterialIdFromSurfaceId(pCollision->idmat[trgId]);
							Vec3 dir(0, 0, 0);

							if (pCollision->vloc[0].GetLengthSquared() > 1e-6f)
							{
								dir = pCollision->vloc[0].GetNormalized();
							}

							HitInfo hitInfo(m_ownerId, pTargetEntity->GetId(), m_weaponId, 10000.f, 0.0f, hitMatId, pCollision->partid[trgId],
													m_hitTypeId, pCollision->pt, dir, pCollision->n);

							hitInfo.remote = false;  //only the server checks for collisions so always propagate
							hitInfo.projectileId = GetEntityId();
							hitInfo.bulletType = m_pAmmoParams->bulletType;

							pGameRules->ClientHit(hitInfo);
						}
					}
					else
					{
						//Do not attach to items
						if(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pTargetEntity->GetId()))
						{
							return;
						}

						StickToEntity(pTargetEntity, pCollision->pt);
					}
				}
				else
				{
					StickToStaticObject(pCollision->pt);
				}
			}

			CHANGED_NETWORK_STATE(this, eEA_GameServerStatic);
			return;
		}
	}
	else
	{
		inherited::HandleEvent(event);
	}
}

//-----------------------------------------------------------------------
void CVolatileSpike::SpawnExplosionEffect()
{
	CRY_ASSERT(m_pAmmoParams->pVSpikeParams);

	if(m_pAmmoParams->pVSpikeParams->explosionEffect)
	{
		Vec3 entityCentrePos;
		GetEntityCentre(&entityCentrePos);
		m_pAmmoParams->pVSpikeParams->explosionEffect->Spawn(true, IParticleEffect::ParticleLoc(entityCentrePos));			
	}
	SetFlag(VS_HAS_SPAWNED_EXPLOSION_EFFECT,true);

	CacheFloorAndBackWall();
	CacheDirectionsToStaticEntitiesForExplosionDecals();
}

//-----------------------------------------------------------------------
void CVolatileSpike::StickToStaticObject(Vec3 collisionPos)
{
	Matrix34 rotationMtx = Matrix34(GetEntity()->GetRotation());
	Vec3 offsetPos(0.f, 0.f/*-0.25f*/, 0.f); //Make this a serialised variable

	offsetPos = rotationMtx * offsetPos;

	GetEntity()->SetPos(collisionPos + offsetPos);

	GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

	m_offsetPosition = collisionPos + offsetPos;
	m_offsetRotation = GetEntity()->GetRotation();

	SetFlag(VS_STUCK,true);
	SetFlag(VS_STUCK_TO_CHARACTER,false);
}

//------------------------------------------------------------------------
void CVolatileSpike::StickToEntity(IEntity* pTargetEntity, Vec3 collisionPos)
{
	Quat targetRotation = pTargetEntity->GetRotation();
	targetRotation.Invert();

	Quat newRotation = targetRotation * GetEntity()->GetRotation();

	Vec3 dir(0.f,0.25f,0.f);

	dir = Matrix34(GetEntity()->GetRotation()) * dir;

	Vec3 pos = (collisionPos - dir) - pTargetEntity->GetPos();

	pos = targetRotation * pos;

	m_offsetPosition = collisionPos;
	m_offsetRotation = GetEntity()->GetRotation();
	SetFlag(VS_STUCK,true);
	m_stuckEntityID = pTargetEntity->GetId();

	pTargetEntity->AttachChild(GetEntity());
	
	SetAspectProfile(eEA_Physics, ePT_None);

	GetEntity()->SetRotation(newRotation);
	GetEntity()->SetPos(pos);
}

//--------------------------------------------------
void CVolatileSpike::AttachToCharacter(ICharacterInstance* pCharacter, const char* boneName)
{
	GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

	char attachName[16] = "";		
	sprintf(attachName, "VSPIKE_%d", s_attachNameID++);

	m_characterAttachment = pCharacter->GetIAttachmentManager()->CreateAttachment(attachName, CA_BONE, boneName, false, false, true); 
	if(m_characterAttachment)
	{
		m_characterAttachment->SetAttRelativeDefault(QuatT(m_offsetRotation, m_offsetPosition));

		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());

		m_characterAttachment->AddBinding(pEntityAttachment);
	}
}

//------------------------------------------------------------------------
bool CVolatileSpike::StickToCharacter(IEntity* pActor, EventPhysCollision* pCollision)
{
	ICharacterInstance* pCharacter = pActor->GetCharacter(0);
	if(!pCharacter)
		return false;

	m_stuckEntityID = pActor->GetId();
	SetFlag(VS_STUCK_TO_CHARACTER,true);
	SetFlag(VS_STUCK,true);
	
	ISkeletonPose* skeleton = pCharacter->GetISkeletonPose();
	
	assert(m_stuckJointID >= 0);
	
	QuatT joint = skeleton->GetAbsJointByID(m_stuckJointID);
	Matrix34 invTM = pActor->GetWorldTM() * Matrix34(joint);

	invTM.Invert();
	m_offsetPosition = invTM * pCollision->pt;
	m_offsetRotation = Quat(invTM) * GetEntity()->GetRotation();

	assert(m_offsetRotation.IsUnit());
	GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

	if(!FindWallContact(pActor, GetEntity()->GetRotation(), pCollision->pt))
	{
		const char* boneName = skeleton->GetJointNameByID(m_stuckJointID);

		AttachToCharacter(pCharacter, boneName);
		
		if(IEntityPhysicalProxy* pPhysicsProxy = (IEntityPhysicalProxy*)pActor->GetProxy(ENTITY_PROXY_PHYSICS))
		{
			Matrix34 currentMtx(GetEntity()->GetRotation());
			Vec3 dir(0.f,1.f,0.f);
			dir = currentMtx * dir;	

			pe_params_bbox entityBB;
			pActor->GetPhysics()->GetParams(&entityBB);
			const Vec3 entityCentre = (entityBB.BBox[0] + entityBB.BBox[1])*0.5f;
					
			pPhysicsProxy->AddImpulse(-1, entityCentre, dir*1000.f, true, 1.f);
		}
	}

	return true;
}

//------------------------------------------------------------------------
void CVolatileSpike::Explode(bool destroy, bool impact, const Vec3 &pos, const Vec3 &normal, const Vec3 &vel, EntityId targetId, float explosionScale)
{	
	CRY_ASSERT(m_pAmmoParams->pVSpikeParams);

	// If no collision, then destroy projectile
	if(!IsFlagSet(VS_STUCK) && !IsFlagSet(VS_STUCK_TO_CHARACTER))
	{
		Destroy();
		return;
	}

	// When the VS sticks into a wall, the shell stays around after the explosion for a short period of time, then 
	if(IsFlagSet(VS_HAS_EXPLODED))
	{
		CRY_ASSERT_MESSAGE(IsFlagSet(VS_IS_STUCK_IN_WALL),"Volatile spike trying to explode twice, and not stuck into a wall");
		Destroy();
		return;
	}

	if(!IsFlagSet(VS_HAS_SPAWNED_EXPLOSION_EFFECT))
	{
		SpawnExplosionEffect();
	}

	if(IsFlagSet(VS_STUCK) && (!IsFlagSet(VS_STUCK_TO_CHARACTER) || IsFlagSet(VS_SEND_CONSTRAINT)))
	{
		SetFlag(VS_IS_STUCK_IN_WALL,true);
		SetLifeTime(m_pAmmoParams->pVSpikeParams->projectileLifeAfterExplosion);
	}

	// Don't destroy the projectile, just hide it until finished processing the explosion effect
	bool actuallyDestroy = false;
	if(!IsFlagSet(VS_IS_STUCK_IN_WALL))
	{
		GetEntity()->Hide(true);
		GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_UPDATE_HIDDEN); // Make sure entity updates when hidden
		GetEntity()->Activate(true); // Entity has to be activated after its hidden, otherwise no update
	}

	inherited::Explode(actuallyDestroy, impact, pos, normal, vel, targetId, explosionScale);

	if(m_stuckEntityID)
	{ 
		CActor* pStuckActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_stuckEntityID));

		if(pStuckActor && pStuckActor->GetHealth() < 0)
		{
			HitInfo hitInfo;
			hitInfo.type = g_pGame->GetGameRules()->GetHitTypeId(m_pAmmoParams->pExplosion->type.c_str());
			pStuckActor->GetDamageEffectController()->OnKill(&hitInfo);
		}
	}

	SetFlag(VS_HAS_EXPLODED,true);
}

//------------------------------------------------------------------------
bool CVolatileSpike::FindWallContact(IEntity* pEntity, Quat currentRotation, Vec3 currentPosition)
{
	Matrix34 currentMtx(currentRotation);
	Vec3 dir(0.f,3.5f,0.f);

	dir = currentMtx * dir;	
	
	ray_hit hit;

	if(gEnv->pPhysicalWorld->RayWorldIntersection(currentPosition, dir, STATIC_TYPES, STATIC_FLAGS, &hit, 1, GetEntity()->GetPhysics(), pEntity->GetPhysics()))
	{
		ray_hit characterHit;

		if(!gEnv->bServer)
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_stuckEntityID);
			ICharacterInstance* pCharacter = pEntity ? pEntity->GetCharacter(0) : NULL;
			QuatT joint = pCharacter ? pCharacter->GetISkeletonPose()->GetAbsJointByID(m_stuckJointID) : QuatT(ZERO);
			
			if(!joint.t.IsZero())
			{
				joint.t = pEntity->GetLocalTM() * joint.t;
				dir = (hit.pt - joint.t) * 1.5f;
			}
		}


		if(gEnv->pPhysicalWorld->RayWorldIntersection(hit.pt, -dir, CHARACTER_TYPES, CHARACTER_FLAGS, &characterHit, 1, GetEntity()->GetPhysics()))
		{
			m_constraintParams.pt[0] = characterHit.pt;

		}
		else if (gEnv->bServer)
		{
			CRY_ASSERT_MESSAGE(0, "FAILED TO FIND THE EXIT POINT");
			m_constraintParams.pt[0] = hit.pt;
		}

		StickToStaticObject(hit.pt);

		m_constraintParams.pBuddy = hit.pCollider;
		m_constraintParams.damping = 1.0f;

		m_constraintParams.yzlimits[0] = 10;
		m_constraintParams.yzlimits[1] = -10;
		m_constraintParams.pt[1] = hit.pt;
		m_constraintParams.flags = constraint_no_enforcement|world_frames;

		m_offsetPosition = currentPosition;
		m_offsetRotation = currentRotation;

		SetFlag(VS_CONSTRAINT,true);
		SetFlag(VS_SEND_CONSTRAINT,true);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
void CVolatileSpike::DebugRender()
{
	Matrix34 currentMtx = GetEntity()->GetLocalTM();
	Vec3 offsetPos(0.f,1.f,0.f);
	Vec3 worldPos = GetEntity()->GetWorldPos();

	offsetPos = currentMtx * offsetPos;

	Vec3 dir = offsetPos - worldPos;

	const float coneRadius = 0.1f;
	const float coneHeight = 0.5f;

	gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(worldPos - dir*coneHeight, dir, coneRadius, coneHeight, ColorF(1,0,0,1));
}

//------------------------------------------------------------------------
bool CVolatileSpike::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags)
{
	if(aspect == eEA_GameServerStatic)
	{
		ser.Value("rot", m_offsetRotation, 'ori1');
		ser.Value("pos", m_offsetPosition, 'wrl3');
		ser.Value("constraintPos", m_constraintParams.pt[0], 'wrl3');
		ser.Value("stuckEntity", m_stuckEntityID, 'eid');
		ser.Value("stuckJoint", m_stuckJointID, 'i16');

		bool stuckToCharacter = IsFlagSet(VS_STUCK_TO_CHARACTER);
		ser.Value("stuckToCharacter", stuckToCharacter, 'bool');
		SetFlag(VS_STUCK_TO_CHARACTER,stuckToCharacter);

		bool sendConstraint = IsFlagSet(VS_SEND_CONSTRAINT);
		ser.Value("constraint", sendConstraint, 'bool');
		SetFlag(VS_SEND_CONSTRAINT,sendConstraint);

		ser.Value("stuck", static_cast<CVolatileSpike*>(this), &CVolatileSpike::IsStuck, &CVolatileSpike::SetStuck, 'bool');
	}

	return inherited::NetSerialize(ser, aspect, profile, pflags);
}

//------------------------------------------------------------------------
void CVolatileSpike::SetStuck(bool stuck)
{	
	if (!IsFlagSet(VS_STUCK) && stuck) 
	{
		PlayImpactSound();

		IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(m_stuckEntityID);

		if(IsFlagSet(VS_SEND_CONSTRAINT))    
		{
			if(pTargetEntity)
			{
				SetFlag(VS_CONSTRAINT,true);
				m_constraintParams.partid[0] = m_stuckJointID;

				GetEntity()->SetPos(m_offsetPosition);
				GetEntity()->SetRotation(m_offsetRotation);

				FindWallContact(pTargetEntity, m_offsetRotation, m_offsetPosition);
			}
		}
		else if (IsFlagSet(VS_STUCK_TO_CHARACTER))
		{
			if (pTargetEntity)
			{
				ICharacterInstance* pCharacter = pTargetEntity->GetCharacter(0);
				IEntityPhysicalProxy* pPhysicsProxy = (IEntityPhysicalProxy*)pTargetEntity->GetProxy(ENTITY_PROXY_PHYSICS);

				if(pPhysicsProxy)
				{
					Matrix34 currentMtx(GetEntity()->GetRotation());
					Vec3 dir(0.f,1.f,0.f);
					dir = currentMtx * dir;	

					pe_params_bbox entityBB;
					pTargetEntity->GetPhysics()->GetParams(&entityBB);
					const Vec3 entityCentre = (entityBB.BBox[0] + entityBB.BBox[1])*0.5f;

					pPhysicsProxy->AddImpulse(-1, entityCentre, dir*1000.f, true, 1.f);
				}

				const char* boneName = pCharacter->GetISkeletonPose()->GetJointNameByID(m_stuckJointID);
				AttachToCharacter(pCharacter, boneName);
			}
		}
		else
		{
			if (pTargetEntity)
			{
				GetEntity()->SetRotation(m_offsetRotation);
				StickToEntity(pTargetEntity, m_offsetPosition);
			}
			else
			{
				GetEntity()->SetRotation(m_offsetRotation);
				GetEntity()->SetPos(m_offsetPosition);
			}
		}

		SetAspectProfile(eEA_Physics, ePT_None);
	}

	SetFlag(VS_STUCK,stuck);
}

//-----------------------------------------------------------------------
void CVolatileSpike::PlayImpactSound()
{
	// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound("Sounds/crysiswars2:weapons:VolatileSpike/Impale", FLAG_SOUND_EVENT|FLAG_SOUND_DEFAULT_3D);

	if (pSound) 
	{
		pSound->SetSemantic(eSoundSemantic_Weapon);
		pSound->SetPosition(GetEntity()->GetPos());
		pSound->Play();
	}
}

//-----------------------------------------------------------------------
void CVolatileSpike::CacheFloorAndBackWall()
{
	IPhysicalWorld* world = gEnv->pPhysicalWorld;
	ray_hit rayHit;
	Vec3 entityPos;
	Vec3 line;

	GetEntityCentre(&entityPos);

	const int maxHits = 1;

	// Cache floor
	line.Set(0.0f,0.0f,-1.0f);
	line *= m_pAmmoParams->pExplosion->maxRadius;
	if(world->RayWorldIntersection(entityPos,line,STATIC_TYPES,STATIC_FLAGS,&rayHit,maxHits))
	{
		m_floor = rayHit.pCollider;
	}

	// Cache back wall
	line = GetEntity()->GetForwardDir();
	line *= m_pAmmoParams->pExplosion->maxRadius;
	if(world->RayWorldIntersection(entityPos,line,STATIC_TYPES,STATIC_FLAGS,&rayHit,maxHits))
	{
		m_backWall = rayHit.pCollider;
	}
}

//-----------------------------------------------------------------------
void CVolatileSpike::CacheDirectionsToStaticEntitiesForExplosionDecals()
{
	Vec3 entityPos;
	GetEntityCentre(&entityPos);
	const Vec3 radiusVec(m_pAmmoParams->pExplosion->maxRadius,m_pAmmoParams->pExplosion->maxRadius,m_pAmmoParams->pExplosion->maxRadius);
	const Vec3 boxMin = entityPos - radiusVec;
	const Vec3 boxMax = entityPos + radiusVec;
	IPhysicalEntity** physicalEntities = NULL;
	const int entityCount = gEnv->pPhysicalWorld->GetEntitiesInBox(boxMin,boxMax,physicalEntities,STATIC_TYPES);

	for(int e=0; e<entityCount; ++e)
	{
		IPhysicalEntity* physicsEntity = physicalEntities[e];
		pe_status_pos physicsData;		

		physicsEntity->GetStatus(&physicsData);

		AABB entityAABB(physicsData.BBox[0]+physicsData.pos,physicsData.BBox[1]+physicsData.pos);
		AABB explosionAABB(boxMin,boxMax);

		AABB entityExplosionOverlapAABB(entityAABB);
		entityExplosionOverlapAABB.ClipToBox(explosionAABB);

		Vec3 directionToEntity;
		directionToEntity = entityExplosionOverlapAABB.GetCenter() - entityPos;
		directionToEntity.Normalize();

		float overlapVolume = entityExplosionOverlapAABB.GetVolume();

		if(overlapVolume > 0.0f)
		{
			// Cache the directions to the largest entities for firing decals at
			int listSize = MIN((m_numCachedStaticEntities+1),(VOLATILE_SPIKE_MAX_STATIC_OBJECTS));
			for(int s=0; s<listSize; ++s)
			{
				if(overlapVolume > m_staticEntity[s].volume)
				{
					int endMovingIndex = MIN((m_numCachedStaticEntities-1),(VOLATILE_SPIKE_MAX_STATIC_OBJECTS-2));
					for(int m=endMovingIndex; m>=s; --m)
					{
						CRY_ASSERT_MESSAGE((m+1 >= 0) && (m+1 < VOLATILE_SPIKE_MAX_STATIC_OBJECTS),"Volatile Spike cached static entity index out of range");
						CRY_ASSERT_MESSAGE((m >= 0) && (m < VOLATILE_SPIKE_MAX_STATIC_OBJECTS),"Volatile Spike cached static entity index out of range");
						m_staticEntity[m+1].volume = m_staticEntity[m].volume;
						m_staticEntity[m+1].dirTo = m_staticEntity[m].dirTo;
					}

					CRY_ASSERT_MESSAGE((s >= 0) && (s < VOLATILE_SPIKE_MAX_STATIC_OBJECTS),"Volatile Spike cached static entity index out of range");
					m_staticEntity[s].volume = overlapVolume;
					m_staticEntity[s].dirTo = directionToEntity;
					m_numCachedStaticEntities = MIN(m_numCachedStaticEntities+1,VOLATILE_SPIKE_MAX_STATIC_OBJECTS);
					CRY_ASSERT_MESSAGE(m_numCachedStaticEntities < 1000,"Volatile spike has too many cached static objects");
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------
void CVolatileSpike::UpdateDecals(SEntityUpdateContext &ctx)
{
	if(m_explosionTimer > 0.0f)
	{
		m_explosionTimer = MAX(0.0f,m_explosionTimer-ctx.fFrameTime);

		Vec3 entityCentrePos;
		Vec3 line;

		GetEntityCentre(&entityCentrePos);

		// Spawn decals on floor and back wall half way through explosion
		if((!IsFlagSet(VS_HAS_SPAWNED_DECALS_ON_WALL_AND_FLOOR)) && (m_explosionTimer < (g_pGameCVars->g_volatileSpike.decalEmitterLife*0.5f)))
		{
			if(m_backWall)
			{
				line = GetEntity()->GetForwardDir();
				const bool spawnSmoke = true;
				CreateDecal(entityCentrePos,line,m_pAmmoParams->pExplosion->maxRadius,g_pGameCVars->g_volatileSpike.decalMaxSize,spawnSmoke);
			}

			if(m_floor)
			{
				line(0.0f,0.0f,-1.0f);
				const bool spawnSmoke = true;
				CreateDecal(entityCentrePos,line,m_pAmmoParams->pExplosion->maxRadius,g_pGameCVars->g_volatileSpike.decalMaxSize,spawnSmoke);
			}
			SetFlag(VS_HAS_SPAWNED_DECALS_ON_WALL_AND_FLOOR,true);
		}
		else if(m_numCachedStaticEntities > 0)
		{
			int numDecalsLeftToSpawn = g_pGameCVars->g_volatileSpike.maxDecals - m_numDecalsSpawned;
			if(IsFlagSet(VS_HAS_SPAWNED_DECALS_ON_WALL_AND_FLOOR))
			{
				if(m_floor)
				{
					numDecalsLeftToSpawn--;
				}
				if(m_backWall)
				{
					numDecalsLeftToSpawn--;
				}
			}

			if(numDecalsLeftToSpawn > 0)
			{
				int cachedEntityIndex = m_numDecalsSpawned;

				// If ran out of cached entities, then randomly pick one
				if(cachedEntityIndex >= m_numCachedStaticEntities)
				{
					cachedEntityIndex = Random(m_numCachedStaticEntities);
				}

				line = m_staticEntity[cachedEntityIndex].dirTo;

				bool spawnSteam = (m_numSteamEmittersSpawned < (g_pGameCVars->g_volatileSpike.maxRandomSteamEmitters+2)); // +2 for floor and back wall

				// Grow decals over time
				float lerpScale = 1.0f;
				if(g_pGameCVars->g_volatileSpike.decalEmitterLife > 0.0f)
				{
					lerpScale = 1.0f - (m_explosionTimer / g_pGameCVars->g_volatileSpike.decalEmitterLife);
				}
				float decalSize = LERP(g_pGameCVars->g_volatileSpike.decalMinSize,g_pGameCVars->g_volatileSpike.decalMaxSize,lerpScale);

				CreateDecal(entityCentrePos,line,m_pAmmoParams->pExplosion->maxRadius,decalSize,spawnSteam);
			}
		}

		// If spawned max number of decals, then stop...
		if(m_numDecalsSpawned == g_pGameCVars->g_volatileSpike.maxDecals)
		{
			m_explosionTimer = 0.0f;
		}
	}
}

//------------------------------------------------------------------------
bool CVolatileSpike::CreateDecal(const Vec3 &origin, const Vec3 &dir, float distance, float decalSize, bool spawnSteam)
{
	CryEngineDecalInfo decal;
	IPhysicalWorld* world = gEnv->pPhysicalWorld;
	ray_hit rayHit;
	const Vec3 line = dir * distance;
	const int maxHits = 1;

	CRY_ASSERT(m_pAmmoParams->pVSpikeParams);

	if (m_pAmmoParams->pVSpikeParams->decalMaterials.size() == 0)
	{
		GameWarning("CVolatileSpike:CreateDecal( ), no materials available for decal generation");
		return false;
	}

	if(world->RayWorldIntersection(origin,line,STATIC_TYPES,STATIC_FLAGS,&rayHit,maxHits))
	{
		// Init decal data
		decal.vPos									= rayHit.pt;
		decal.vNormal								= rayHit.n;
		decal.fLifeTime							= g_pGameCVars->g_volatileSpike.decalLife;
		decal.fGrowTime							= g_pGameCVars->g_volatileSpike.decalGrowTime;
		decal.fSize									= decalSize;
		decal.bAdjustPos						= true;
		decal.fAngle								= Random() * gf_PI2;
		decal.vHitDirection					= dir;
		decal.bSkipOverlappingTest	= true;
		decal.bAssemble							= false;
		decal.preventDecalOnGround	= false;

		int nameIndex = Random(m_pAmmoParams->pVSpikeParams->decalMaterials.size());
		strcpy(decal.szMaterialName,m_pAmmoParams->pVSpikeParams->decalMaterials[nameIndex]->GetName());

		// Get render node for decal
		if(rayHit.pCollider)
		{
			if(IRenderNode* pRenderNode = (IRenderNode*)rayHit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC))
			{
				decal.ownerInfo.pRenderNode = pRenderNode;
			}
			else if(IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(rayHit.pCollider))
			{
				IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
				if(pRenderProxy)
				{
					decal.ownerInfo.pRenderNode = pRenderProxy->GetRenderNode();
				}
			}
		}

		// Create decal
		gEnv->p3DEngine->CreateDecal(decal);
		m_numDecalsSpawned++;

		// Spawn steam on decals location
		if(spawnSteam && m_pAmmoParams->pVSpikeParams->steamEffect)
		{
			m_pAmmoParams->pVSpikeParams->steamEffect->Spawn(true, IParticleEffect::ParticleLoc(rayHit.pt));
			m_numSteamEmittersSpawned++;
		}
	}

	return (rayHit.pCollider != NULL);
}

//------------------------------------------------------------------------
void CVolatileSpike::GetEntityCentre(Vec3* entityCentre) const
{
	AABB aabb;
	GetEntity()->GetWorldBounds(aabb);
	(*entityCentre) = aabb.GetCenter();
}

//------------------------------------------------------------------------
void CVolatileSpike::DebugRenderDecals()
{
	Vec3 dir;
	IPhysicalWorld* world = gEnv->pPhysicalWorld;
	ray_hit rayHit;
	Vec3 line;
	const int maxHits = 1;
	const float sphereRadius = 0.3f;
	Vec3 entityPos;

	GetEntityCentre(&entityPos);

	SAuxGeomRenderFlags renderFlags;
	renderFlags.SetAlphaBlendMode(e_AlphaBlended);
	renderFlags.SetFillMode(e_FillModeSolid);
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(renderFlags);

	if(m_floor)
	{
		dir.Set(0.0f,0.0f,-1.0f);
		line = dir * m_pAmmoParams->pExplosion->maxRadius;

		if(world->RayWorldIntersection(entityPos,line,STATIC_TYPES,STATIC_FLAGS,&rayHit,maxHits))
		{
			ColorB floorCol(0,0,255,128);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(rayHit.pt,sphereRadius,floorCol);
		}
	}

	if(m_backWall)
	{
		dir = GetEntity()->GetForwardDir();
		line = dir * m_pAmmoParams->pExplosion->maxRadius;

		if(world->RayWorldIntersection(entityPos,line,STATIC_TYPES,STATIC_FLAGS,&rayHit,maxHits))
		{
			ColorB backWallCol(0,255,255,128);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(rayHit.pt,sphereRadius,backWallCol);
		}
	}

	for(int i=0; i<m_numCachedStaticEntities; i++)
	{
		dir = m_staticEntity[i].dirTo;
		line = dir * m_pAmmoParams->pExplosion->maxRadius;

		if(world->RayWorldIntersection(entityPos,line,STATIC_TYPES,STATIC_FLAGS,&rayHit,maxHits))
		{
			ColorB sphereCol(0,255,0,128);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(rayHit.pt,sphereRadius,sphereCol);
		}
	}

	ColorB centreColor(255,0,0,128);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(entityPos,sphereRadius,centreColor);

	static float fontSize = 12.0f;
	gEnv->pRenderer->DrawLabel(entityPos,fontSize,"%d",m_numDecalsSpawned);
}
