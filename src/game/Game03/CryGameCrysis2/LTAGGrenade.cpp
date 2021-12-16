/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Rocket

-------------------------------------------------------------------------
History:
- 25:02:2009   11:15 : Created by Filipe Amim
- 16:09:2009   Re-factor to work together with LTagSingle (Benito G.R.)

*************************************************************************/
#include "StdAfx.h"
#include <ISound.h>
#include "LTAGGrenade.h"
#include "Game.h"
#include "Bullet.h"
#include "GameRules.h"
#include "Player.h"
#include "GameCVars.h"
#include "Environment/BattleDust.h"
#include "Utility/CryWatch.h"
#include "IAIActor.h"

int CLTAGGrenade::s_attachNameID = 0;

//------------------------------------------------------------------------
CLTAGGrenade::CLTAGGrenade()
:	m_launchLoc(0,0,0)
,	m_grenadeType(ELTAGGrenadeType_STICKY)
,	m_stuckPos(0.f,0.f,0.f)
,	m_normal(0, 0, 0)
,	m_stuck(false)
,	m_stuckToLivingEntity(false)
,	m_lifeTime(0)
,	m_bounced(false)
,	m_stickedEntityId(0)
,	m_state(EState_NONE)
,	m_activeTime(0)
,	m_extraArmTime(0)
,	m_armedSoundId(INVALID_SOUNDID)
,   m_characterAttachment(NULL)
,	m_enemyKilledByCollision(false)
{
	
}

//------------------------------------------------------------------------
CLTAGGrenade::~CLTAGGrenade()
{
	if(m_characterAttachment)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_stickedEntityId);
		if (pEntity)
		{
			IAttachmentManager* pAttachManager = pEntity->GetCharacter(0)->GetIAttachmentManager();
			pAttachManager->RemoveAttachmentByInterface(m_characterAttachment);
		}
	}

	GetGameObject()->ReleaseProfileManager(this);
}

//------------------------------------------------------------------------
void CLTAGGrenade::HandleEvent(const SGameObjectEvent &event)
{
	if (CheckAnyProjectileFlags(ePFlag_destroying))		
		return;

	if (!gEnv->bServer || IsDemoPlayback())
		return;

	switch (event.event)
	{
	case eGFE_OnCollision:
		OnCollision(event);
		break;
	}

	CProjectile::HandleEvent(event);

	CActor* pStuckEntity = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_stickedEntityId));
	if (pStuckEntity && pStuckEntity->GetHealth() <= 0)
		m_enemyKilledByCollision = true;
}

//------------------------------------------------------------------------
void CLTAGGrenade::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CProjectile::Launch(pos, dir, velocity, speedScale);
	//	m_ownerId = GetWeapon()->GetOwner()->GetId();

	OnLaunch();

	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	m_launchLoc = pos;

	m_extraArmTime = grenadeParams.m_armTime * 1.15f;
	m_activeTime = 0;

	if (m_grenadeType == ELTAGGrenadeType_RICOCHET)
		ChangeState(ERicochetState_FLYING);

	ChangeTexture(grenadeParams.m_fireMaterial);
	ChangeTrail(grenadeParams.m_fireTrail);
}


//------------------------------------------------------------------------
void CLTAGGrenade::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	inherited::Update(ctx, updateSlot);
	StateUpdate(ctx);
	ProxymityGrenadeUpdate(ctx);
	UpdateLTagTimeOut();
}


//------------------------------------------------------------------------
void CLTAGGrenade::UpdateLTagTimeOut()
{
	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	if (m_totalLifetime < grenadeParams.m_ltagLifetime)
		return;

	if(m_pAmmoParams->quietRemoval)
		Destroy();
	else
		ExplodeGrenade();
}


//------------------------------------------------------------------------
void CLTAGGrenade::ProxymityGrenadeUpdate(SEntityUpdateContext &ctx)
{
	switch (m_grenadeType)
	{
		case ELTAGGrenadeType_STICKY:
		{
			m_lifeTime += ctx.fFrameTime;

			bool proximityDetected = ProximityDetector();

			bool explodeGrenade = (IsProximityEnabled() && proximityDetected);

			if (explodeGrenade && m_activeTime < m_extraArmTime && proximityDetected)
				explodeGrenade = false;

			if (explodeGrenade)
				ExplodeGrenade();
			else
			{
				const Vec3 & newPos = GetEntity()->GetWorldPos();
				m_last = newPos;
			}

			break;
		}

		case ELTAGGrenadeType_RICOCHET:
		{
			if (m_state == ERicochetState_PROXY)
			{
				bool proximityDetected = ProximityDetector();
				if (proximityDetected)
					ChangeState(ERicochetState_ARMED);
			}
			else if (m_state == ERicochetState_ARMED)
			{
				if (m_activeTime > m_extraArmTime)
					ExplodeGrenade();
			}
			break;
		}
	}
}

//------------------------------------------------------------------------
void CLTAGGrenade::StateUpdate(SEntityUpdateContext &ctx)
{
	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	m_activeTime += ctx.fFrameTime;

	switch (m_state)
	{
	case EStickyState_SAFE:
		if (!IsOwnerToClose())
		{
			ChangeState(EStickyState_ARMING);
			m_activeTime = 0;
		}
		break;

	case EStickyState_UNSAFE:
		if (!g_pGame->GetGameRules()->IsMultiplayerDeathmatch())
		{
			if (IsOwnerToClose())
			{
				ChangeState(EStickyState_SAFE);
			}
		}
		break;

	case EStickyState_ARMING:
		if (IsOwnerToClose())
		{
			ChangeState(EStickyState_SAFE);
		}
		else if (m_activeTime >= grenadeParams.m_armTime)
		{
			ChangeState(EStickyState_UNSAFE);
		}
		break;


	case ERicochetState_FLYING:
		m_normal = GetEntity()->GetWorldTM().GetColumn0();
		m_activeTime = 0;
		break;

	case ERicochetState_PROXY:
		m_normal = GetEntity()->GetWorldTM().GetColumn0();
		m_activeTime = 0;
		break;

	case ERicochetState_ARMED:
		m_normal = GetEntity()->GetWorldTM().GetColumn0();
		break;
	}
}


//------------------------------------------------------------------------
void CLTAGGrenade::StateEnter(EState state)
{
	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	switch (state)
	{
	case EStickyState_SAFE:
		PlaySound(grenadeParams.m_disarmSound);
		ChangeTexture(grenadeParams.m_disarmMaterial);
		ChangeTrail(grenadeParams.m_disarmTrail);
		break;

	case EStickyState_UNSAFE:
		{
			m_armedSoundId = PlaySound(grenadeParams.m_armedSound);
			ChangeTexture(grenadeParams.m_armedMaterial);
			ChangeTrail(grenadeParams.m_armedTrail);
			ISound* pSound = GetSoundProxy()->GetSound(m_armedSoundId);
			if (pSound)
				pSound->GetInterfaceDeprecated()->SetLoopMode(true);

			if (g_pGame->GetGameRules()->IsMultiplayerDeathmatch())
			{
#if 0
//			IEntity* pOwner = gEnv->pEntitySystem->GetEntity(m_ownerId);
				EntityId clientActorId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
				int clientActorTeam = g_pGame->GetGameRules()->GetTeam(clientActorId);
				int ownerTeam = g_pGame->GetGameRules()->GetTeam(m_ownerId);
#endif

				CActor* clientActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
				
				if (clientActor && clientActor->IsFriendlyEntity(m_ownerId))
				{
					CryLog("LTAGGrenade::StateEnter() entering EStickyState_UNSAFE - grenade owner is friendlyEntity to current player clientActor - setting safe armed material");
					ChangeTexture(grenadeParams.m_armedSafeMaterial);	// TODO replace with new material when its available - blue
					ChangeTrail(grenadeParams.m_armedSafeTrail);		// crucially controls the glow colour
				}
				else
				{
					CryLog("LTAGGrenade::StateEnter() entering EStickyState_UNSAFE - grenade owner is NOT friendlyEntity to current player clientActor - setting unsafe armed material");
					ChangeTexture(grenadeParams.m_armedMaterial);
					ChangeTrail(grenadeParams.m_armedTrail);
				}
			}
			else
			{
				ChangeTexture(grenadeParams.m_armedMaterial);
				ChangeTrail(grenadeParams.m_armedTrail);
			}
		}
		break;

	case EStickyState_ARMING:
		PlaySound(grenadeParams.m_armSound);
		ChangeTexture(grenadeParams.m_armMaterial);
		ChangeTrail(grenadeParams.m_armTrail);
		break;


	case ERicochetState_PROXY:
		ChangeTexture(grenadeParams.m_bounceMaterial);
		ChangeTrail(grenadeParams.m_bounceTrail);
		break;

	case ERicochetState_ARMED:
		m_activeTime = 0.0f;
		ChangeTexture(grenadeParams.m_armMaterial);
		ChangeTrail(grenadeParams.m_armTrail);
		break;
	}
}


//------------------------------------------------------------------------
void CLTAGGrenade::StateExit(EState state)
{
	switch (state)
	{
	case EStickyState_UNSAFE:
		StopSound(m_armedSoundId);
		m_armedSoundId = INVALID_SOUNDID;
		break;
	}
}


//------------------------------------------------------------------------
void CLTAGGrenade::ChangeState(EState state)
{
	//CryLog("CLTAGGrenade::ChangeState() from %d to %d\n", m_state, state);
	StateExit(m_state);
	m_state = state;
	StateEnter(m_state);

	if(gEnv->bServer)
	{
		CHANGED_NETWORK_STATE(this, eEA_GameServerStatic);
	}
}

void CLTAGGrenade::OnCollision(const SGameObjectEvent &event)
{
	EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;
	int trgId = 1;
	int srcId = 0;
	IPhysicalEntity *pTarget = pCollision->pEntity[trgId];
	IEntity* pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;
	IActor* pHitActor =
		pTargetEntity ?
		(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId())) :
	0;

	m_normal = pCollision->n;

	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	switch (m_grenadeType)
	{
	case ELTAGGrenadeType_STICKY:
		PlaySound(grenadeParams.m_bounceSound);
		Stick(pCollision);
		ChangeState(EStickyState_SAFE);
		break;

	case ELTAGGrenadeType_RICOCHET:
		if (m_state == ERicochetState_FLYING)
			ChangeState(ERicochetState_PROXY);
		break;
	}

	m_bounced = true;
}

//-----------------------------------------------
bool CLTAGGrenade::AttachToCharacter(IEntity& pEntity, ICharacterInstance& pCharacter, const char* boneName)
{
	char attachName[16] = "";		
	sprintf(attachName, "LTAG_%d", s_attachNameID++);

	m_characterAttachment = pCharacter.GetIAttachmentManager()->CreateAttachment(attachName, CA_BONE, boneName, false, false, true); 

	if(!m_characterAttachment)
		return false;

	GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

	m_characterAttachment->SetAttRelativeDefault(QuatT(m_stuckRot, m_stuckPos));

	CEntityAttachment *pEntityAttachment = new CEntityAttachment();
	pEntityAttachment->SetEntityId(GetEntityId());

	m_characterAttachment->AddBinding(pEntityAttachment);

	IAIObject* pAI = pEntity.GetAI();
	if (pAI)
	{
		IAIActor* pAiActor = pAI->CastToIAIActor();
		if (pAiActor)
			pAiActor->SetSignal(0, "OnLTagStick", this->GetEntity());
	}

	return true;
}



//-----------------------------------------------
void CLTAGGrenade::Stick(EventPhysCollision *pCollision)
{
	if(gEnv->bServer)
	{
		assert(pCollision);
		int trgId = 1;
		int srcId = 0;
		IPhysicalEntity *pTarget = pCollision->pEntity[trgId];

		if(ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(pCollision->idmat[trgId]))
		{
			if (pSurfaceType->GetBreakability() == 1)
				return;
		}


		if (pTarget == GetEntity()->GetPhysics())
		{
			trgId = 0;
			srcId = 1;
			pTarget = pCollision->pEntity[trgId];
		}

		IEntity *pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;

		if (pTarget && (!pTargetEntity || (pTargetEntity->GetId() != m_ownerId)))
		{
			//Ignore water collision
			if(pCollision->idmat[trgId] == CBullet::GetWaterMaterialId())
				return;

			Matrix34 mat;
			m_stuck = true;
			if(!pTargetEntity)
			{
				m_stuckPos = pCollision->pt+(pCollision->n*0.1f);
				Matrix33 orientation3 = Matrix33::CreateOrientation(-pCollision->n,GetEntity()->GetWorldTM().TransformVector(Vec3(0,0,1)),gf_PI);
				Matrix34 orientation, position;
				orientation.SetIdentity();
				orientation.SetRotation33(orientation3);
				position.SetIdentity();
				position.SetTranslation(m_stuckPos);
				mat = position * orientation;

				GetEntity()->SetWorldTM(mat);

				AttachTo(NULL);
			}
			else
			{
				m_stickedEntityId = pTargetEntity->GetId();

				ICharacterInstance* pCharInstance = pTargetEntity->GetCharacter(0);

				CActor* pLivingEntity = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId()));
				if (pLivingEntity && pCharInstance)	
				{			
					m_stuckJoint = pCollision->partid[trgId];

					ISkeletonPose* skeleton = pCharInstance->GetISkeletonPose();
					
					const char* boneName = skeleton->GetJointNameByID(m_stuckJoint);
					QuatT joint = skeleton->GetAbsJointByID(m_stuckJoint);
					Matrix34 invTM = pTargetEntity->GetWorldTM() * Matrix34(joint);
					
					invTM.Invert();
					m_stuckPos = invTM * pCollision->pt;
					m_stuckRot = Quat(invTM) * GetEntity()->GetRotation();

					m_stuck = AttachToCharacter(*pTargetEntity, *pCharInstance, boneName);
					m_stuckToLivingEntity = m_stuck && pLivingEntity->GetHealth() > 0;
					if (!m_stuckToLivingEntity)
						m_stickedEntityId = 0;
				}
				else
				{
					m_stuckPos = pCollision->pt+(pCollision->n*0.1f);
					Matrix33 orientation3 = Matrix33::CreateOrientation(-pCollision->n,GetEntity()->GetWorldTM().TransformVector(Vec3(0,0,1)),gf_PI);
					Matrix34 orientation, position;
					orientation.SetIdentity();
					orientation.SetRotation33(orientation3);
					position.SetIdentity();
					position.SetTranslation(m_stuckPos);
					mat = position * orientation;

					AttachTo(pTargetEntity);
					GetEntity()->SetWorldTM(mat);	
				}
			}

			m_normal = pCollision->n;

			CWeapon* pWeapon = GetWeapon();
			if(pWeapon)
				pWeapon->OnProjectileCollided(GetEntityId(), pTarget, pCollision->pt);

			CHANGED_NETWORK_STATE(this, eEA_GameServerStatic);
		}

		//*
		IPhysicalEntity* pPhysEnt = GetEntity()->GetPhysics();
		if (m_stuck && pPhysEnt)
		{
			pe_params_part pp;
			pp.flagsAND = ~geom_colltype_explosion;
			pp.mass = 0.0f;
			pPhysEnt->SetParams(&pp);
		}
		/**/
	}
}


//------------------------------------------------------------------------
bool CLTAGGrenade::ProximityDetector()
{
	IEntity* pOwner = gEnv->pEntitySystem->GetEntity(m_ownerId);
	if (!pOwner)
		return false;

	if (m_enemyKilledByCollision)
		return true;

	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	const Vec3& proxyCenter = GetEntity()->GetWorldPos();
	const float proxyRadius = grenadeParams.m_boxDimension;

	// broadphase with box intersection
	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	IPhysicalEntity** pPhysicalEntities = 0;
	int objType = ent_living;
	const Vec3 boxMin = proxyCenter - Vec3(proxyRadius, proxyRadius, proxyRadius);
	const Vec3 boxMax = proxyCenter + Vec3(proxyRadius, proxyRadius, proxyRadius);
	int numEntities = pPhysicalWorld->GetEntitiesInBox(boxMin, boxMax, pPhysicalEntities, objType);
	CActor* pOwnerActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_ownerId));

	const static IEntityClass* sHologramClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Hologram");

	for (int j = 0; j < numEntities; ++j)
	{
		IPhysicalEntity* pPhysicalEntity = pPhysicalEntities[j];
		IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);
		if (!pEntity)
			continue;
		if (pEntity == pOwner)
			continue;
		if (pOwnerActor && pOwnerActor->IsFriendlyEntity(pEntity->GetId()))
			continue;
		if (g_pGame->GetGameRules()->IsDead(pEntity->GetId()))
			continue;
		if(pEntity->GetClass() == sHologramClass)
			continue;

		/*	IAIObject* pAI = pEntity->GetAI();
		bool isHostile = pAI ? pAI->IsHostile(pOwner->GetAI()) : false;
		if (!isHostile)
		continue;*/

		return true;
	}

	return false;
}


//------------------------------------------------------------------------
bool CLTAGGrenade::IsProximityEnabled() const
{
	return
		(m_grenadeType == ELTAGGrenadeType_STICKY && m_state == EStickyState_UNSAFE) ||
		(m_grenadeType == ELTAGGrenadeType_RICOCHET && m_state == ERicochetState_PROXY);
}


//------------------------------------------------------------------------
void CLTAGGrenade::ExplodeGrenade(IActor* pHitActor /*= 0*/)
{
	if (gEnv->bServer)
	{
		const bool ownerToClose = IsOwnerToClose();

		if (ownerToClose && pHitActor)
			pHitActor->Fall(Vec3(0, 0, 0), true);

		if (!ownerToClose)
		{
			if (m_stuck)
				Explode(true, false, Vec3(0,0,0), m_normal);
			else
				Explode(true);
		}
	}
}


//------------------------------------------------------------------------
bool CLTAGGrenade::IsOwnerToClose()
{
	const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

	if (grenadeParams.m_safeExplosion <= 0.0f)
		return false;

	IEntity* pOwner = gEnv->pEntitySystem->GetEntity(m_ownerId);
	if (!pOwner)
		return false;

	const Vec3& grenadePosition = GetEntity()->GetWorldPos();
	const Vec3& ownerPosition = pOwner->GetWorldPos();
	const float dp2 = (ownerPosition - grenadePosition).len2();
	return (dp2 <= grenadeParams.m_safeExplosion*grenadeParams.m_safeExplosion);
}



int CLTAGGrenade::PlaySound(const string& soundName)
{
	int id = GetSoundProxy()->PlaySound(
		soundName, /*GetEntity()->GetWorldPos()*/ Vec3(0, 0, 0),
		FORWARD_DIRECTION, FLAG_SOUND_DEFAULT_3D,
		eSoundSemantic_Projectile,
		0, 0);
	return id;
}


//------------------------------------------------------------------------
void CLTAGGrenade::StopSound(int id)
{
	if (id == INVALID_SOUNDID)
		return;
	GetSoundProxy()->StopSound(id);
}



void CLTAGGrenade::ChangeTexture(const string& textureName)
{
	if (textureName.empty())
		return;

	IEntity* pEntity = GetEntity();
	IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(textureName.c_str());
	if (pMaterial)
		pEntity->SetSlotMaterial(0, pMaterial);
}



void CLTAGGrenade::ChangeTrail(const string& trailString)
{
	AttachEffect(false, m_trailEffectId);
	m_trailEffectId = AttachEffect(true, 0, trailString.c_str(), Vec3(0,0,0), Vec3(0,1,0));
}

//------------------------------------------------------------------------
bool CLTAGGrenade::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags)
{
	if(aspect == eEA_GameServerStatic)
	{
		uint type = m_grenadeType;
		ser.Value("pos", m_stuckPos, 'wrl3');
		ser.Value("norm", m_normal, 'wrl3');
		ser.Value("rot", m_stuckRot, 'ori1');
		ser.Value("joint", m_stuckJoint, 'i16');
		ser.Value("stuckEntity", m_stickedEntityId, 'eid');
		ser.Value("type", type, 'ui2');

		m_grenadeType = (ELTAGGrenadeType) type;

		ser.Value("stuck", static_cast<CLTAGGrenade*>(this), &CLTAGGrenade::IsStuck, &CLTAGGrenade::NetSetStuck, 'bool');
		ser.Value("state", static_cast<CLTAGGrenade*>(this), &CLTAGGrenade::NetGetState, &CLTAGGrenade::NetSetState, 'ui5');

		
	}

	return inherited::NetSerialize(ser, aspect, profile, pflags);
}

//------------------------------------------------------------------------
void CLTAGGrenade::NetSetStuck(bool stuck)
{
	if(stuck && !m_stuck)
	{
		IEntity* pTargetEntity = m_stickedEntityId ? gEnv->pEntitySystem->GetEntity(m_stickedEntityId) : NULL;
		ICharacterInstance* pCharacter = pTargetEntity ? pTargetEntity->GetCharacter(0) : NULL;

		if (gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_stickedEntityId) && pCharacter)
		{
			m_stuckToLivingEntity = true;
			const char* boneName = pCharacter->GetISkeletonPose()->GetJointNameByID(m_stuckJoint);
			AttachToCharacter(*pTargetEntity, *pCharacter, boneName);			
		}
		else
		{
			Matrix34 mat; 
			Matrix33 orientation3 = Matrix33::CreateOrientation(m_normal,GetEntity()->GetWorldTM().TransformVector(Vec3(0,0,1)),gf_PI);
			Matrix34 orientation, position;
			orientation.SetIdentity();
			orientation.SetRotation33(orientation3);
			position.SetIdentity();
			position.SetTranslation(m_stuckPos);
			mat = position * orientation;

			GetEntity()->SetWorldTM(mat);
			AttachTo(pTargetEntity);
		}

		m_stuck = stuck;
	}
}

//------------------------------------------------------------------------
void CLTAGGrenade::AttachTo(IEntity * pTargetEntity)
{
	if(pTargetEntity)
	{
		CryLog ("PROJECTILE: Sticking %s '%s' to %s '%s'...", GetEntity()->GetClass()->GetName(), GetEntity()->GetName(), pTargetEntity->GetClass()->GetName(), pTargetEntity->GetName());
		pTargetEntity->AttachChild(GetEntity());
		GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);
	}
	else
	{
		CryLog ("PROJECTILE: Sticking %s '%s' to its current position in the world...", GetEntity()->GetClass()->GetName(), GetEntity()->GetName());
		GetGameObject()->SetAspectProfile(eEA_Physics, ePT_Static);
	}
}

//------------------------------------------------------------------------
void CLTAGGrenade::NetSetState(int inState)
{
	ChangeState(static_cast<EState>(inState));		// TODO remove if not needed. Presume we need to cast away the enum to use this
}


void CLTAGGrenade::ReInitFromPool()
{
	CProjectile::ReInitFromPool();

	m_grenadeType = ELTAGGrenadeType_STICKY;
	m_normal.zero();
	m_stuck = false;
	m_stuckToLivingEntity = false;
	m_lifeTime = 0.0f;
	m_bounced = false;
	m_stickedEntityId = 0;
	m_state = EState_NONE;
	m_armedSoundId = INVALID_SOUNDID;
}

const SLTagGrenadeParams::SCommonParams& CLTAGGrenade::GetModeParams() const
{
	assert(m_pAmmoParams->pLTagParams);

	switch(m_grenadeType)
	{
	case ELTAGGrenadeType_STICKY:
		return m_pAmmoParams->pLTagParams->m_sticky;

	case ELTAGGrenadeType_RICOCHET:
		return m_pAmmoParams->pLTagParams->m_ricochet;
	}

	assert(0);

	return m_pAmmoParams->pLTagParams->m_ricochet;
}


void CLTAGGrenade::Explode( bool destroy, bool impact/*=false*/, const Vec3 &pos/*=ZERO*/, const Vec3 &normal/*=FORWARD_DIRECTION*/, const Vec3 &vel/*=ZERO*/, EntityId targetId/*=0 */, float explosionScale/*=1.f*/)
{
	if (gEnv->bServer)
	{
		const SExplosionParams* pExplosionParams = m_pAmmoParams->pExplosion;
		if (pExplosionParams)
		{
			Vec3 dir(0,0,1);
			if (impact && vel.len2()>0)
				dir = vel.normalized();
			else if (normal.len2()>0)
				dir = -normal;

			m_hitPoints = 0;

			// marcok: using collision pos sometimes causes explosions to have no effect. Anton advised to use entity pos
			Vec3 epos = pos.len2()>0 ? (pos + dir * 0.2f) : GetEntity()->GetWorldPos();

		CGameRules *pGameRules = g_pGame->GetGameRules();
		float minRadius = pExplosionParams->minRadius;
		float maxRadius = pExplosionParams->maxRadius;

			const SLTagGrenadeParams::SCommonParams& grenadeParams = GetModeParams();

			ExplosionInfo explosionInfo(
				m_ownerId, GetEntityId(), 0, (float)m_damage, epos, dir, minRadius, maxRadius, pExplosionParams->minPhysRadius, pExplosionParams->maxPhysRadius, 0.0f, pExplosionParams->pressure, pExplosionParams->holeSize, pGameRules->GetHitTypeId(pExplosionParams->type.c_str()));
			explosionInfo.SetEffect(grenadeParams.m_explosionEffect.c_str(), pExplosionParams->effectScale, pExplosionParams->maxblurdist);
			switch (m_grenadeType)
			{
				case ELTAGGrenadeType_RICOCHET: explosionInfo.SetEffectClass("LTAG_ricochet"); break;
				case ELTAGGrenadeType_STICKY: explosionInfo.SetEffectClass("LTAG_sticky"); break;
			}

			if (impact)
				explosionInfo.SetImpact(normal, vel, targetId);

			if (gEnv->bServer)
			{
				pGameRules->QueueExplosion(explosionInfo);

				// add battle dust as well
				CBattleDust* pBD = pGameRules->GetBattleDust();
				if(pBD)
					pBD->RecordEvent(eBDET_Explosion, pos, GetEntity()->GetClass());
			}
		}

		SetShouldBeOnHud(false);

		if (destroy)
			Destroy();
	}
}

bool CLTAGGrenade::ShouldKnockTarget() const
{
	return true;
}
