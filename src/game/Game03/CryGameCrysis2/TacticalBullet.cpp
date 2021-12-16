#include "StdAfx.h"
#include "TacticalBullet.h"
#include "Game.h"
#include "WeaponSystem.h"
#include "Actor.h"
#include "GameRules.h"
#include "Actor2/Actor2.h"
#include "Actor2/ComponentDataRegistry.h"

#include "Bullet.h"



CTacticalBullet* CTacticalBullet::m_mainBulllet = 0;



class TacticalBulletState
{
public:
	TacticalBulletState(CTacticalBullet* owner)
		:	m_owner(owner) {}

	virtual void Enter() {}
	virtual void Update(float deltaTime) {}
	virtual void HandleEvent(const SGameObjectEvent &event) {}


protected:
	CTacticalBullet* GetOwner() const
	{
		return m_owner;
	}

private:
	CTacticalBullet* m_owner;
};



class ActiveState: public TacticalBulletState
{
public:
	ActiveState(CTacticalBullet* owner)
		:	TacticalBulletState(owner) {}


	virtual void Enter()
	{
		GetOwner()->StartBulletAction();
	}


	virtual void Update(float deltaTime)
	{
		GetOwner()->UpdateBulletAction(deltaTime);
	}

private:
};



class ActivatingState: public TacticalBulletState
{
public:
	ActivatingState(CTacticalBullet* owner, float activationtime)
		:	TacticalBulletState(owner)
		,	m_activationTime(activationtime) {}


	virtual void Enter()
	{
		GetOwner()->SpawnActivationEffect(GetOwner()->m_params->m_activationEffectName);
	}


	virtual void Update(float deltaTime)
	{
		m_activationTime -= deltaTime;
		if (m_activationTime <= 0.0f)
		{
			CTacticalBullet::m_mainBulllet = 0;
			GetOwner()->ChangeState(new ActiveState(GetOwner()));
		}
	}


private:
	float m_activationTime;
};



class FlyingState: public TacticalBulletState
{
public:
	FlyingState(CTacticalBullet* owner, float stickTimeOut)
		:	TacticalBulletState(owner)
		,	m_stickTimeOut(stickTimeOut) {}


	virtual void Update(float deltaTime)
	{
		m_stickTimeOut -= deltaTime;
		if(m_stickTimeOut <= 0.0f)
		{
			if(GetOwner()->GetWeapon())
				GetOwner()->GetWeapon()->OnProjectileCollided(GetOwner()->GetEntityId(), NULL, Vec3Constants<float>::fVec3_Zero);
			GetOwner()->Destroy();
		}
	}


	virtual void HandleEvent(const SGameObjectEvent &event)
	{
		if (GetOwner()->m_destroying)
			return;

		if (event.event == eGFE_OnCollision)
		{
			StickAndActivateBullet(event);
		}
	}

private:

	void StickAndActivateBullet(const SGameObjectEvent &event)
	{
		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;

		IEntity* pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1]:0;
		if (pTarget == GetOwner()->GetEntity())
			return;

		GetOwner()->Stick(pCollision);
		GetOwner()->ChangeState(new ActivatingState(GetOwner(), GetOwner()->m_params->m_activationTime));
	}

	float m_stickTimeOut;
};




class GoToPointState : public FlyingState
{
public:
	GoToPointState(CTacticalBullet* owner, float stickTimeOut)
		:	FlyingState(owner, stickTimeOut) {}


	virtual void Update(float deltaTime)
	{
		if (CTacticalBullet::m_mainBulllet)
		{
			const Vec3& targetPos = CTacticalBullet::m_mainBulllet->GetEntity()->GetWorldPos();
			const Vec3& actualPos = GetOwner()->GetEntity()->GetWorldPos();

			pe_status_dynamics status;
			pe_status_pos pos;
			GetOwner()->GetEntity()->GetPhysics()->GetStatus(&status);
			GetOwner()->GetEntity()->GetPhysics()->GetStatus(&pos);

			Vec3 actualVelocity = status.v;
			Vec3 actualDirection = actualVelocity;
			actualDirection.Normalize();
			float actualSpeed = actualVelocity.len();

			Vec3 targetDir = targetPos - actualPos;
			targetDir.Normalize();

			Vec3 smoothedDirection = actualDirection;
			Interpolate(smoothedDirection, targetDir, GetOwner()->m_params->m_hommingSpeed, deltaTime);

			pe_action_set_velocity action;
			action.v = smoothedDirection * actualSpeed;
			GetOwner()->GetEntity()->GetPhysics()->Action(&action);

			TacticalBulletState::Update(deltaTime);
		}
		else
		{
			CRY_FIXME(28, 7, 2009, "filipe - should not just kill the bullet. change target maybe?");
			GetOwner()->Destroy();
		}
	}


	virtual void HandleEvent(const SGameObjectEvent &event)
	{
		if (GetOwner()->m_destroying)
			return;

		if (event.event == eGFE_OnCollision)
		{
			IntensifyMainBullet();
			GetOwner()->Destroy();
		}
	}

private:
	void IntensifyMainBullet()
	{
		CTacticalBullet::m_mainBulllet->ChangeState(
			new ActivatingState(CTacticalBullet::m_mainBulllet, CTacticalBullet::m_mainBulllet->m_params->m_activationTime));
		CTacticalBullet::m_mainBulllet->IntensifyActionEffect();
	}
};




CTacticalBullet::CTacticalBullet()
:	m_attached(false)
,	m_params(new Params())
,	m_parentId(0)
,	m_actualState(0)
,	m_activationEffectId(0)
,	m_activationEffectSize(1.0f)
{
}



CTacticalBullet::~CTacticalBullet()
{
	delete m_params;
	delete m_actualState;

	if (CTacticalBullet::m_mainBulllet == this)
	{
		CTacticalBullet::m_mainBulllet = 0;
	}
}




void CTacticalBullet::ChangeState(TacticalBulletState* nextState)
{
	CRY_FIXME(27, 7, 2009, "filipe - this delete has to go. It produces 'delete this' in some state transitions that are troublesome.");
	delete m_actualState;

	nextState->Enter();
	m_actualState = nextState;
}



void CTacticalBullet::HandleEvent(const SGameObjectEvent &event)
{
	if (m_actualState)
	{
		m_actualState->HandleEvent(event);
	}
}



void CTacticalBullet::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	if (m_actualState)
	{
		m_actualState->Update(ctx.fFrameTime);
	}
}



void CTacticalBullet::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CProjectile::Launch(pos, dir, velocity, speedScale);

	m_params->m_activationTime = GetParam("activationTime", m_params->m_activationTime);
	m_params->m_effectRadius = GetParam("radius", m_params->m_effectRadius);
	m_params->m_effectRadiusSum = GetParam("radiusSum", m_params->m_effectRadiusSum);
	m_params->m_hommingSpeed = GetParam("hommingSpeed", m_params->m_hommingSpeed);

	const char* def = 0;
	m_params->m_activationEffectName = GetParam("activationEffectName", def);

	if (CTacticalBullet::m_mainBulllet == 0)
	{
		CTacticalBullet::m_mainBulllet = this;
		ChangeState(
			new FlyingState(
				this, m_params->m_stickTime));
	}
	else
	{
		ChangeState(
			new GoToPointState(
				this, m_params->m_stickTime));
	}
}



void CTacticalBullet::Stick(EventPhysCollision *pCollision)
{
	assert(pCollision);
	int trgId = 1;
	int srcId = 0;
	IPhysicalEntity *pTarget = pCollision->pEntity[trgId];

	if (pTarget == GetEntity()->GetPhysics())
	{
		trgId = 0;
		srcId = 1;
		pTarget = pCollision->pEntity[trgId];
	}

	m_pParentPhysicalEntity = pTarget;
	m_pParentPhysicalEntity->AddRef();
	m_ParentPhysicalEntityPartID = pCollision->partid[trgId];

	IEntity *pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;

	if (pTarget && (!pTargetEntity || (pTargetEntity->GetId() != m_ownerId)))
	{
		//Ignore water collision
		if(pCollision->idmat[trgId] == CBullet::GetWaterMaterialId())
			return;

		Vec3 pos;
		Matrix34 mat;
		if(!pTargetEntity)
		{
			//Calculate new position and orientation 

			pos = pCollision->pt+(pCollision->n*0.01f);
			mat.SetIdentity();
			//mat.SetRotation33(Matrix33::CreateOrientation(-pCollision->n,GetEntity()->GetWorldTM().TransformVector(Vec3(0,0,1)),gf_PI));
			mat.SetTranslation(pos);
			GetEntity()->SetWorldTM(mat);

			//GetGameObject()->SetAspectProfile(eEA_Physics, ePT_Static);
			GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);
		}
		else
		{
			Matrix34 mat = pTargetEntity->GetWorldTM();
			mat.Invert();
			pos = pCollision->pt+(pCollision->n*0.01f);
			mat.SetIdentity();
			//mat.SetRotation33(Matrix33::CreateOrientation(-pCollision->n,GetEntity()->GetWorldTM().TransformVector(Vec3(0,0,1)),gf_PI));
			mat.SetTranslation(pos);

			//Dephysicalize and stick
			//GetGameObject()->SetAspectProfile(eEA_Physics, ePT_Static);
			GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

			pTargetEntity->AttachChild(GetEntity());
			GetEntity()->SetWorldTM(mat);
			m_parentId = pTargetEntity->GetId();
		}

		m_attached = true;
		CWeapon* pWeapon = GetWeapon();
		if(pWeapon)
			pWeapon->OnProjectileCollided(GetEntityId(), pTarget, pCollision->pt);
	}
	//*
	IPhysicalEntity* pPhysEnt = GetEntity()->GetPhysics();
	if (pPhysEnt)
	{
		pe_params_part pp;
		pp.flagsAND = 0;
		pp.mass = 0.0f;
		pPhysEnt->SetParams(&pp);
	}
	/**/
}





void CTacticalBullet::SpawnActivationEffect(const char* particlesName)
{
	m_activationEffectId = AttachEffect(true, 0, particlesName, Vec3(0,0,0), Vec3(0,1,0));
}



void CTacticalBullet::SpawnExplosionEffect()
{
	IEntity* pOwner = gEnv->pEntitySystem->GetEntity(m_ownerId);
	if (!pOwner)
		return;
	const Vec3& proxyCenter = GetEntity()->GetWorldPos();

	const char* effectName = m_pAmmoParams->pExplosion->effectName;
	IParticleEffect* particleEffect = gEnv->pParticleManager->FindEffect(effectName);
	Matrix34 loc;
	loc.SetIdentity();
	loc.SetTranslation(proxyCenter);
	particleEffect->Spawn(true, loc);
}



void CTacticalBullet::StartBulletAction()
{
}



void CTacticalBullet::UpdateBulletAction( float deltaTime )
{
}



void CTacticalBullet::IntensifyActionEffect()
{
	m_params->m_effectRadius += m_params->m_effectRadiusSum;

	IParticleEmitter* emmiter = GetEntity()->GetParticleEmitter(m_activationEffectId);
	if (emmiter)
	{
		m_activationEffectSize += 0.5f;
		SpawnParams particleParams;
		particleParams.fSizeScale = m_activationEffectSize;
		emmiter->SetSpawnParams(particleParams);
	}
}






void CTacticalEMP::StartBulletAction()
{
	SpawnExplosionEffect();

	IEntity* pOwner = gEnv->pEntitySystem->GetEntity(m_ownerId);
	if (!pOwner)
		return;
	const Vec3& proxyCenter = GetEntity()->GetWorldPos();

	// broadphase with box intersection
	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	IPhysicalEntity** pPhysicalEntities = 0;
	int objType = ent_living;
	const Vec3 boxMin = proxyCenter - Vec3(GetRadius(), GetRadius(), GetRadius());
	const Vec3 boxMax = proxyCenter + Vec3(GetRadius(), GetRadius(), GetRadius());
	int numEntities = pPhysicalWorld->GetEntitiesInBox(boxMin, boxMax, pPhysicalEntities, objType);

	for (unsigned int j = 0; j < uint32(numEntities); ++j)
	{
		IPhysicalEntity* pPhysicalEntity = pPhysicalEntities[j];
		IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);
		if (!pEntity)
			continue;
		if (g_pGame->GetGameRules()->IsDead(pEntity->GetId()))
			continue;

		{
			CActor2* actor = (CActor2*)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
			actor->SendFrameworkEvent(ComponentDataRegistry::ExternalEvent.emp, 0, 0);
		}
	}

	Destroy();
}







void CTacticalExplosion::StartBulletAction()
{
	Explode(true, true, Vec3(0,0,0), Vec3(0, 0, -1));
}

