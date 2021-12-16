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
#include "Rocket.h"
#include "Game.h"
#include "Bullet.h"
#include "GameRules.h"


//------------------------------------------------------------------------
CRocket::CRocket()
: m_launchLoc(0,0,0),
	m_safeExplosion(0),
	m_detonatorFired(false)
{
}

//------------------------------------------------------------------------
CRocket::~CRocket()
{
	AutoDropOwnerWeapon();
}

//------------------------------------------------------------------------
void CRocket::HandleEvent(const SGameObjectEvent &event)
{
	if (CheckAnyProjectileFlags(ePFlag_destroying))
		return;

	CProjectile::HandleEvent(event);

	if (!gEnv->bServer || IsDemoPlayback())
		return;

	if (event.event == eGFE_OnCollision && !m_detonatorFired)
	{		
		AutoDropOwnerWeapon();
		m_detonatorFired = true;

		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;
		if (m_safeExplosion>0.0f)
		{
			float dp2=(m_launchLoc-GetEntity()->GetWorldPos()).len2();
			if (dp2<=m_safeExplosion*m_safeExplosion)
			{
				DisableTrail();
				return;
			}
		}

		if(pCollision && pCollision->pEntity[0]->GetType()==PE_PARTICLE)
		{
			float bouncy, friction;
			uint32	pierceabilityMat;
			gEnv->pPhysicalWorld->GetSurfaceParameters(pCollision->idmat[1], bouncy, friction, pierceabilityMat);

			pe_params_particle params;
			
			if(pCollision->pEntity[0]->GetParams(&params)==0)
				SetDefaultParticleParams(&params);
			
			if((params.velocity>1.0f) && (pCollision->idmat[1] != CBullet::GetWaterMaterialId())
				&& (!pCollision->pEntity[1] || (pCollision->pEntity[1]->GetType() != PE_LIVING && pCollision->pEntity[1]->GetType() != PE_ARTICULATED)))
			{
				if(pierceabilityMat>params.iPierceability)
					return;
			}

		}

		IEntity* pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1] : 0;

		Explode(true, true, pCollision->pt, pCollision->n, pCollision->vloc[0], pTarget?pTarget->GetId():0);
	}
}

//------------------------------------------------------------------------
void CRocket::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CProjectile::Launch(pos, dir, velocity, speedScale);

	m_launchLoc=pos;

	m_safeExplosion = GetParam("safeexplosion", m_safeExplosion);

	if(CWeapon* pWeapon = GetWeapon())
	{
		if(pWeapon->IsAutoDroppable())
			pWeapon->AddFiredRocket();
	}
}



void CRocket::AutoDropOwnerWeapon()
{
	CWeapon* pWeapon = GetWeapon();
	if(!m_detonatorFired && pWeapon)
	{
		if(pWeapon->IsAutoDroppable())
			pWeapon->AutoDrop();
	}
}



void CRocket::EnableTrail()
{
	TrailEffect(true);
	TrailSound(true);

	pe_params_particle params;
	params.accThrust = m_pAmmoParams->pParticleParams->accThrust;
	GetEntity()->GetPhysics()->SetParams(&params);
}



void CRocket::DisableTrail()
{
	TrailEffect(false);
	TrailSound(false);

	pe_params_particle params;
	params.accThrust = 0.0;
	GetEntity()->GetPhysics()->SetParams(&params);
}



bool CRocket::ShouldKnockTarget() const
{
	return !m_detonatorFired;
}



void CRocket::ProcessEvent(SEntityEvent &event)
{
	if (event.event == ENTITY_EVENT_TIMER && event.nParam[0] == ePTIMER_LIFETIME)
	{
		if (!m_detonatorFired)
			Explode(true);
		else
			Destroy();
	}
	else
	{
		return CProjectile::ProcessEvent(event);
	}
}
