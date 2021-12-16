/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 19:12:2005   12:10 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ImpulseBeam.h"
#include "Weapon.h"
#include "Actor.h"
#include "Game.h"
#include "GameRules.h"
#include <ISound.h>
#include <IEntitySystem.h>
#include <IVehicleSystem.h>

#include "Projectile.h"
#include "WeaponSharedParams.h"
#include "VectorSet.h"
#include "ImpulseBeamProjectile.h"


//------------------------------------------------------------------------
CImpulseCone::CImpulseCone()
:	m_charging(false),
m_charged(false),
m_cooldownTime(0.0f),
m_firing(false),
m_spinUpFXId(-1)
{
	m_mfIds[0] = m_mfIds[1] = 0;
}

//------------------------------------------------------------------------
CImpulseCone::~CImpulseCone()
{
}


//std::vector<Vec3> gpoints;

//------------------------------------------------------------------------
void CImpulseCone::Update(float frameTime, uint32 frameId)
{
	//for (std::vector<Vec3>::iterator it=gpoints.begin(); gpoints.end()!=it;++it)
	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(*it, 0.35f, ColorB(255, 128, 0, 255));

	bool keepUpdating=false;

	if (m_charging)
	{
		keepUpdating=true;

		if (m_chargeTime>0.0f)
		{
			m_chargeTime -= frameTime;

			if (m_chargeTime>0.0f)
			{
				m_pWeapon->RequireUpdate(eIUS_FireMode);
			}
			else
			{
				m_chargeTime = 0.0f;
				m_charging = false;
				m_charged = true;
 			}
		}
	}

	if (m_cooldownTime>0.0f)
	{
		keepUpdating=true;

		m_cooldownTime-=frameTime;
		if (m_cooldownTime<0.0f)
		{
			m_cooldownTime=0.0f;

			if (m_firing)
				StartFire();
		}
	}

	if (keepUpdating)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CImpulseCone::Activate(bool activate)
{
	m_charged=false;
	m_charging=false;
	m_cooldownTime=0.0f;
	m_firing=false;

	if(!activate)
		MuzzleFlash(false);
}

//------------------------------------------------------------------------
bool CImpulseCone::CanFire(bool considerAmmo) const
{
	return !m_charging && !m_firing && m_cooldownTime<=0.0f && !m_pWeapon->IsBusy();
}

//------------------------------------------------------------------------
void CImpulseCone::StartFire()
{
	if (!CanFire(true))
		return;

	m_firing=true;

	m_pWeapon->PlayAction(m_fireParams->actions.spin_up, 0, false, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending);
	m_pWeapon->RequireUpdate(eIUS_FireMode);
	int slot = m_pWeapon->IsOwnerFP()? eIGS_FirstPerson: eIGS_ThirdPerson;
	m_spinUpFXId = m_pWeapon->AttachEffect(slot, -1, true, m_fireParams->coneparams.spin_up_effect.c_str(), slot == 0? m_fireParams->coneparams.spin_up_effect_helper.c_str() : m_fireParams->coneparams.spin_up_effect_helper_tp.c_str(), Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, 0.5f);

	m_charging = true;
	m_charged = false;
	m_chargeTime = m_fireParams->coneparams.charge_time;

	m_pWeapon->EnableUpdate(true, eIUS_FireMode);
}

//------------------------------------------------------------------------
void CImpulseCone::StopFire()
{
	m_firing=false;

	if(m_spinUpFXId > 0)
	{
		m_pWeapon->AttachEffect(m_pWeapon->IsOwnerFP()? eIGS_FirstPerson: eIGS_ThirdPerson, m_spinUpFXId, false);
		m_spinUpFXId = -1;
	}
	if(!m_charging && !m_charged)
		return;

	if (m_charging || m_charged)
	{
		m_pWeapon->PlayAction(m_fireParams->actions.blast);

		float dt=m_charged?1.0f:1.0f-m_chargeTime/m_fireParams->coneparams.charge_time;
		float impulse=m_fireParams->coneparams.min_impulse+(m_fireParams->coneparams.impulse-m_fireParams->coneparams.min_impulse)*dt;

		Blast(impulse);

		m_cooldownTime=m_fireParams->coneparams.cooldown;
		if(m_cooldownTime>0.0f)
			m_pWeapon->RequireUpdate(eIUS_FireMode);
	}

	m_charging = false;
	m_charged = false;

	m_pWeapon->EnableUpdate(false, eIUS_FireMode);
}

//------------------------------------------------------------------------
void CImpulseCone::Blast(float impulse)
{
	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	if (!pMC)
		return;

	SMovementState info;
	pMC->GetMovementState(info);

	Vec3 pos = info.weaponPosition;
	Vec3 dir = info.fireDirection.normalized();
	Vec3 vel = dir * 5.0f;

	IEntityClass* pAmmoClass =
		gEnv->pEntitySystem->GetClassRegistry()->FindClass("impulseBeamProjectile");
	if (!pAmmoClass)
		return;
	CImpulseBeamProjectile* pAmmo = (CImpulseBeamProjectile*)m_pWeapon->SpawnAmmo(pAmmoClass, false);
	if (!pAmmo)
		return;
	pAmmo->Launch(pos, dir, vel, 1.0f);
	pAmmo->SetParams(
		m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), 0, 0.f, 0.f, 0.f, 0, 0);
	pAmmo->SetImpulseBeamParams(&m_fireParams->coneparams, impulse);

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), 0, 0, info.weaponPosition, info.fireDirection.normalized(), ZERO);
}

//------------------------------------------------------------------------
void CImpulseCone::Contact(IPhysicalEntity *pPhysicalEntity, const Vec3 &dir, float impulse, int partId, Vec3 point)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);
	CActor *pActor = NULL;

	if(pEntity)
	{
		if (IActor *pIActor=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()))
			pActor = static_cast<CActor *>(pIActor);
		else if (IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId()))
			impulse *=m_fireParams->coneparams.impulse_vehicle_scale;
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
			uai.impulse.z=m_fireParams->coneparams.up_impulse;
			uai.point=bone;

			pCharacter->GetISkeletonPose()->GetCharacterPhysics()->Action(&uai);
		}

		// directional impulse
		pe_action_impulse bai;
		bai.partid=partId>-1?partId:boneId;
		bai.impulse=dir*impulse*m_fireParams->coneparams.impulse_human_scale;
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
			m_fireParams->coneparams.damage * (impulse/m_fireParams->coneparams.impulse), 0.0f, 0, -1,g_pGame->GetGameRules()->GetHitTypeId(m_fireParams->coneparams.hit_type.c_str()), point, dir, -dir);

		g_pGame->GetGameRules()->ClientHit(hitInfo);
	}

}

//------------------------------------------------------------
void CImpulseCone::SetupEmitters(bool attach)
{
	if (attach)
	{		
		int id = m_pWeapon->GetStats().fp ? 0 : 1;
		Vec3 offset(ZERO);

		if (m_fireParams->coneeffectparams.helper[id].empty())
		{ 
			// if no helper specified, try getting pos from firing locator
			IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();            

			if (pLocator && pLocator->GetFiringPos(m_pWeapon->GetEntityId(), this, offset))
				offset = m_pWeapon->GetEntity()->GetWorldTM().GetInvertedFast() * offset;
		}

		if (!m_fireParams->coneeffectparams.effect[0].empty())
		{   
			m_mfIds[0] = m_pWeapon->AttachEffect(eIGS_FirstPerson, -1, true, m_fireParams->coneeffectparams.effect[0].c_str(), 
				m_fireParams->coneeffectparams.helper[0].c_str(), offset, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}
		if (!m_fireParams->coneeffectparams.effect[1].empty())
		{
			m_mfIds[1] = m_pWeapon->AttachEffect(eIGS_ThirdPerson, -1, true, m_fireParams->coneeffectparams.effect[1].c_str(), 
				m_fireParams->coneeffectparams.helper[1].c_str(), offset, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}
	}
	else
	{
		m_mfIds[0] = m_pWeapon->AttachEffect(eIGS_FirstPerson, m_mfIds[0], false);
		m_mfIds[1] = m_pWeapon->AttachEffect(eIGS_ThirdPerson, m_mfIds[1], false);
	}
}

//-------------------------------------------------------
void CImpulseCone::MuzzleFlash(bool attach)
{
	if (attach)
	{    
		int slot = m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
		int id = m_pWeapon->GetStats().fp ? 0 : 1;

		if (!m_fireParams->coneeffectparams.effect[id].empty() && !m_pWeapon->GetEntity()->IsHidden())
		{
			if (!m_mfIds[id])
				SetupEmitters(true);		

			IParticleEmitter *pEmitter = m_pWeapon->GetEffectEmitter(m_mfIds[id]);
			if (pEmitter)
				pEmitter->EmitParticle();
		}		  	
	}
	else
	{
		SetupEmitters(false);
	}

}

//-----------------------------------------------------------
void CImpulseCone::GetMemoryUsage(ICrySizer * s) const
{	
		s->AddObject(this, sizeof(*this));
		CFireMode::GetInternalMemoryUsage(s); // collect memory of parent class
}

//-----------------------------------------------------------
void CImpulseCone::OnZoomStateChanged()
{

}
