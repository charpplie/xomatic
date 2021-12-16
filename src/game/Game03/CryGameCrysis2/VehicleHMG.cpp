/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Mounted machine gun that can be ripped off by the player
and move around with it - vehicle mounted version
-------------------------------------------------------------------------
History:
- 16:02:2010: Created by SNH

*************************************************************************/

#include "StdAfx.h"
#include "VehicleHMG.h"

#include "Actor.h"
#include <IViewSystem.h>
#include <IVehicleSystem.h>
#include "Single.h"
#include "GameCVars.h"
#include "Player.h"
#include "Game.h"

//------------------------------------------------------------------------
CVehicleHMG::CVehicleHMG()
: m_pVehicle(0)
, m_pPart(0)
, m_dtWaterLevelCheck(0.f)
, m_pOwnerSeat(NULL)
, m_pSeatUser(NULL)
{  
}

//------------------------------------------------------------------------
void CVehicleHMG::StartUse(EntityId userId)
{
 	if (!IsRippingOff() && GetEntity()->GetParent())
 	{ 
 		m_pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(GetEntity()->GetParent()->GetId());
 		assert(m_pVehicle && "Using VehicleWeapons on non-vehicles may lead to unexpected behavior.");
 
 		if (m_pVehicle)
 		{   
 			m_pPart = m_pVehicle->GetWeaponParentPart(GetEntityId()); 
 			m_pOwnerSeat = m_pVehicle->GetWeaponParentSeat(GetEntityId());
 			m_pSeatUser = m_pVehicle->GetSeatForPassenger(userId);
 		}
 	}

	CHMG::StartUse(userId);
}

//------------------------------------------------------------------------
void CVehicleHMG::StartFire(const Vec3 *fireTarget)
{
 	if (!CheckWaterLevel())
 		return;

	CHMG::StartFire();
}

//------------------------------------------------------------------------
void CVehicleHMG::Update( SEntityUpdateContext& ctx, int update)
{
	CHMG::Update(ctx, update);

	// update the saved mount direction, this is used to orient the actor
	//	in the mounted gun controller
	const Matrix34& ori = GetEntity()->GetLocalTM();
	m_stats.mount_dir = ori.TransformVector(FORWARD_DIRECTION);
	m_stats.mount_dir.z = 0.0f;
	m_stats.mount_dir.NormalizeSafe();

 	if(update==eIUS_General)
 	{ 
 		if (m_fm && m_fm->IsFiring())
 		{
 			m_dtWaterLevelCheck -= ctx.fFrameTime;      
 
 			if (m_dtWaterLevelCheck <= 0.f)
 			{ 
 				if (!CheckWaterLevel())        
 					StopFire();          
 
 				m_dtWaterLevelCheck = 2.0f;
 			}
 		}
	}
}

//------------------------------------------------------------------------
bool CVehicleHMG::CheckWaterLevel() const
{
	// if not submerged at all, skip water level check
	if (m_pVehicle && m_pVehicle->GetStatus().submergedRatio < 0.01f)
		return true;

	if (gEnv->p3DEngine->IsUnderWater(GetEntity()->GetWorldPos()))
		return false;

	return true;
}

//------------------------------------------------------------------------
void CVehicleHMG::SetAmmoCount(IEntityClass* pAmmoType, int count)
{ 
 	IActor* pOwner = GetOwnerActor();
 
	if (pOwner && !pOwner->IsPlayer() && count < SWeaponAmmoUtils::GetAmmoCount(m_ammo, pAmmoType))
 		return;

	CHMG::SetAmmoCount(pAmmoType, count);    
}

//------------------------------------------------------------------------
void CVehicleHMG::SetInventoryAmmoCount(IEntityClass* pAmmoType, int count)
{
 	IActor* pOwner = GetOwnerActor();
 
 	if (m_pVehicle && pOwner && !pOwner->IsPlayer())
 	{
 		if (count < m_pVehicle->GetAmmoCount(pAmmoType))
 			return;
 	}

	CHMG::SetInventoryAmmoCount(pAmmoType, count);
}

//------------------------------------------------------------------------
bool CVehicleHMG::CanZoom() const
{
	if (!CHMG::CanZoom())
		return false;

	if(m_pVehicle && !IsRippedOff())
	{
		if (m_pSeatUser != m_pOwnerSeat)
			return false;

		IActor* pActor = GetOwnerActor();
		IVehicleSeat* pSeat = (pActor != NULL) ? m_pVehicle->GetSeatForPassenger(pActor->GetEntityId()) : NULL;
		IVehicleView* pView = (pSeat != NULL) ? pSeat->GetView(pSeat->GetCurrentView()) : NULL;
		if ((pView != NULL) && pView->IsThirdPerson())
			return false;
	}

	return true;
}

//----------------------------------------------------------------------
void CVehicleHMG::PerformRipOff(CActor* pOwner)
{
	SVehicleEventParams params;
	params.entityId = GetEntityId();
	m_pVehicle->BroadcastVehicleEvent(eVE_WeaponRemoved, params);

	CHMG::PerformRipOff(pOwner);
}

void CVehicleHMG::FinishRipOff()
{
	CHMG::FinishRipOff();

 	if(m_pOwnerSeat != NULL)
 	{
 		if(m_pOwnerSeat == m_pSeatUser)
 		{
 			m_pOwnerSeat->Lock(true);
 		}
 
 		m_pOwnerSeat->Exit(true);
 	}
 
 	m_pVehicle = NULL;
 	m_pSeatUser = NULL;
 	m_pPart = NULL;
 	m_pOwnerSeat = NULL;
}