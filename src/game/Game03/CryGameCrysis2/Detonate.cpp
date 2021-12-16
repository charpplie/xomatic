/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Detonate.h"
#include "WeaponSystem.h"
#include "Item.h"
#include "Weapon.h"
#include "Actor.h"
#include "Projectile.h"
#include "Binocular.h"
#include "C4.h"

#include "WeaponSharedParams.h"

//------------------------------------------------------------------------
CDetonate::CDetonate()
{
	m_canDetonate = false;
}

//------------------------------------------------------------------------
CDetonate::~CDetonate()
{
}

//------------------------------------------------------------------------
struct CDetonate::ExplodeAction
{
	ExplodeAction(CDetonate *_detonate): pDetonate(_detonate) {};
	CDetonate *pDetonate;

	void execute(CItem *_this)
	{
		CActor *pOwner = pDetonate->m_pWeapon->GetOwnerActor();
		bool outOfAmmo = pDetonate->m_pWeapon->OutOfAmmo(false);

		if(pOwner && outOfAmmo)
		{
			pOwner->SelectLastItem(true, true);
		}
	}
};

void CDetonate::Update(float frameTime, uint32 frameId)
{
	CSingle::Update(frameTime, frameId);

	if (m_detonationTimer>0.0f)
	{
		m_detonationTimer-=frameTime;

		if (m_detonationTimer<=0.0f)
		{
			m_detonationTimer=0.0f;

			bool detonated = Detonate();

			if (detonated && m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsClient())
				m_pWeapon->GetScheduler()->TimerAction(uint32(m_pWeapon->GetCurrentAnimationTime(eIGS_Owner)), CSchedulerAction<ExplodeAction>::Create(this), false);
		}
		else
			m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CDetonate::Activate(bool activate)
{
	CSingle::Activate(activate);
	
	m_detonationTimer = 0.0f;
}

//------------------------------------------------------------------------
bool CDetonate::CanReload() const
{
	return false;
}

//------------------------------------------------------------------------
bool CDetonate::CanFire(bool considerAmmo) const
{
	bool detonateAllowed = false;

	if(m_detonationTimer<=0.0f)
	{
		if(gEnv->bServer)
		{
			if(IFireMode* pFM = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()))
			{
				detonateAllowed = pFM->GetProjectileId() != 0;
			}
		}
		else
		{
			detonateAllowed = m_canDetonate;
		}
	}
	
	return detonateAllowed;
}

//------------------------------------------------------------------------
void CDetonate::StartFire()
{
	if (CanFire(false))
	{
		m_pWeapon->RequireUpdate(eIUS_FireMode);
		m_detonationTimer = 0.1f;
		m_pWeapon->PlayAction(GetShared()->actions.fire);
		m_pWeapon->RequestDetonate();
	}
}

//------------------------------------------------------------------------
const char *CDetonate::GetCrosshair() const
{
	return "";
}

//------------------------------------------------------------------------
bool CDetonate::Detonate(bool net)
{
	bool detonatedAll = true;

	if (m_pWeapon->IsServer())
	{
		CActor *pOwner=m_pWeapon->GetOwnerActor();
		if (!pOwner)
			return false;

		if (IFireMode* pFM = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode()))
		{
			std::vector<EntityId> undetonatedList;
			undetonatedList.clear();

			while(EntityId projectileId = pFM->RemoveProjectileId())
			{
				if (CProjectile *pProjectile = g_pGame->GetWeaponSystem()->GetProjectile(projectileId))
				{
					if(pProjectile->Detonate())
					{
						g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, pProjectile->GetEntity()->GetClass()->GetName(), 1, (void *)m_pWeapon->GetEntityId()));
					}
					else
					{
						stl::push_back_unique(undetonatedList, projectileId);
					}
				}
			}

			while(!undetonatedList.empty())
			{
				pFM->SetProjectileId(undetonatedList.back());
				undetonatedList.pop_back();
				detonatedAll = false;
			}
		}
	}

	return detonatedAll;
}

//------------------------------------------------------------------------
void CDetonate::NetShoot(const Vec3 &hit, int ph)
{
	Detonate(true);
}
