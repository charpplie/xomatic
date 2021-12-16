/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 20:04:2006   13:02 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "VehicleWeapon.h"
#include "Actor.h"
#include <IViewSystem.h>
#include <IVehicleSystem.h>
#include "Single.h"
#include "GameCVars.h"
#include "Player.h"
#include "Game.h"

//------------------------------------------------------------------------
CVehicleWeapon::CVehicleWeapon()
: m_pVehicle(0)
, m_pPart(0)
, m_dtWaterLevelCheck(0.f)
, m_pOwnerSeat(NULL)
, m_pSeatUser(NULL)
{  
}

//------------------------------------------------------------------------
bool CVehicleWeapon::Init( IGameObject * pGameObject )
{
  if (!CWeapon::Init(pGameObject))
    return false;

	m_properties.mounted=true;

  return true;
}

//------------------------------------------------------------------------
void CVehicleWeapon::PostInit( IGameObject * pGameObject )
{
  CWeapon::PostInit(pGameObject); 
}

//------------------------------------------------------------------------
void CVehicleWeapon::Reset()
{
  CWeapon::Reset();
}

//------------------------------------------------------------------------
void CVehicleWeapon::MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles)
{
  CWeapon::MountAtEntity(entityId, pos, angles);  
}

//------------------------------------------------------------------------
void CVehicleWeapon::StartUse(EntityId userId)
{
	if (m_owner.GetId() && userId != m_owner.GetId())
		return; 

  if (GetEntity()->GetParent())
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
  
	SetOwnerId(userId);
  Select(true);	
	m_stats.used = true;

	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

  if (OutOfAmmo(false))
    Reload(false);

  UseManualBlending(true);

	LowerWeapon(false);

	SendMusicLogicEvent(eMUSICLOGICEVENT_WEAPON_MOUNT);

}

//------------------------------------------------------------------------
void CVehicleWeapon::StopUse(EntityId userId)
{
	if (m_owner.GetId() && userId != m_owner.GetId())
		return;

	SendMusicLogicEvent(eMUSICLOGICEVENT_WEAPON_UNMOUNT);
 
  Select(false);  	
  m_stats.used = false;	

  UseManualBlending(false);

  SetOwnerId(0);

  EnableUpdate(false);

	if(IsZoomed() || IsZoomingInOrOut())
	{
		ExitZoom();
	}

	LowerWeapon(false);

}

//------------------------------------------------------------------------
void CVehicleWeapon::StartFire(const Vec3 *fireTarget)
{
  if (!CheckWaterLevel())
    return;

	if(!CanFire())
		return;

  CWeapon::StartFire();
}

//------------------------------------------------------------------------
void CVehicleWeapon::Update( SEntityUpdateContext& ctx, int update)
{
  CWeapon::Update(ctx, update);

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
  
		if (m_stats.mounted)
		{
			UpdateMounted(ctx.fFrameTime);
		}
	}
}

//------------------------------------------------------------------------
bool CVehicleWeapon::CheckWaterLevel() const
{
  // if not submerged at all, skip water level check
  if (m_pVehicle && m_pVehicle->GetStatus().submergedRatio < 0.01f)
    return true;
  
  if (gEnv->p3DEngine->IsUnderWater(GetEntity()->GetWorldPos()))
    return false;

  return true;
}

//------------------------------------------------------------------------
void CVehicleWeapon::SetAmmoCount(IEntityClass* pAmmoType, int count)
{ 
  IActor* pOwner = GetOwnerActor();
  
	if (pOwner && !pOwner->IsPlayer() && count < SWeaponAmmoUtils::GetAmmoCount(m_ammo, pAmmoType))
    return;
  
  CWeapon::SetAmmoCount(pAmmoType, count);    
}

//------------------------------------------------------------------------
void CVehicleWeapon::SetInventoryAmmoCount(IEntityClass* pAmmoType, int count)
{
  IActor* pOwner = GetOwnerActor();

  if (pOwner && !pOwner->IsPlayer() && m_pVehicle)
  {
    if (count < m_pVehicle->GetAmmoCount(pAmmoType))
      return;
  }

  CWeapon::SetInventoryAmmoCount(pAmmoType, count);
}

//------------------------------------------------------------------------
bool CVehicleWeapon::FilterView(SViewParams& viewParams)
{ 
  if (m_pOwnerSeat != m_pSeatUser)
    return false;

  if (m_camerastats.animating && !m_camerastats.helper.empty())
  { 
    viewParams.position = GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper, true);
    viewParams.rotation = Quat(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper, true));    
    viewParams.blend = false;
    
    if (g_pGameCVars->v_debugMountedWeapon)
    { 
      Vec3 local = GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper, false, true);
      Vec3 entity = GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper, false, false);
                 
      float color[] = {1,1,1,1};      
      gEnv->pRenderer->Draw2dLabel(50,400,1.3f,color,false,"cam_pos local(%.3f %.3f %.3f), entity(%.3f %.3f %.3f)", local.x, local.y, local.z, entity.x, entity.y, entity.z);
      
      Ang3 angLocal(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper, false, true));
      Ang3 angEntity(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper, false, true));      

      gEnv->pRenderer->Draw2dLabel(50,420,1.3f,color,false,"cam_rot local(%.3f %.3f %.3f), entity(%.3f %.3f %.3f)", angLocal.x, angLocal.y, angLocal.z, angEntity.x, angEntity.y, angEntity.z);
    }
        
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
bool CVehicleWeapon::GetAimBlending(OldBlendSpace& params)
{   
	/*
  float anglemin=0.f, anglemax=0.f;
  if (m_pPart && m_pPart->GetRotationLimits(0, anglemin, anglemax))
  { 
    if (!(anglemin == 0.f && anglemax == 0.f)) // no limits
    {
      Ang3 angles( m_pPart->GetLocalTM(true) );

      float limit = isneg(angles.x) ? anglemin : anglemax;
      float ratio = (limit != 0.f) ? min(1.f, angles.x/limit) : 0.f;

      params.m_turn = sgn(angles.x) * ratio;
      
      return true;
    }
  }
	*/

  return false;
}

//---------------------------------------------------------------------------
void CVehicleWeapon::UpdateIKMounted(IActor* pActor, const Vec3& vGunXAxis)
{
  // only apply IK when the weapons user is in the weapons owner seat
  if (m_pSeatUser == m_pOwnerSeat)
    CWeapon::UpdateIKMounted(pActor,vGunXAxis);
}

//------------------------------------------------------------------------
bool CVehicleWeapon::CanZoom() const
{
  if (!CWeapon::CanZoom())
    return false;

  if (m_pSeatUser != m_pOwnerSeat)
    return false;

  IActor* pActor = GetOwnerActor();
	IVehicleSeat* pSeat = (pActor != NULL) ? m_pVehicle->GetSeatForPassenger(pActor->GetEntityId()) : NULL;
	IVehicleView* pView = (pSeat != NULL) ? pSeat->GetView(pSeat->GetCurrentView()) : NULL;
  if ((pView != NULL) && pView->IsThirdPerson())
    return false;

  return true;
}

//---------------------------------------------------------------------
void CVehicleWeapon::UpdateFPView(float frameTime)
{
	CItem::UpdateFPView(frameTime);

	if(GetOwnerId() && m_pVehicle && (m_pVehicle->GetCurrentWeaponId(GetOwnerId(), true) != GetEntityId())) //only update primary weapon
		UpdateCrosshair(frameTime);
	if (m_fm)
		m_fm->UpdateFPView(frameTime);
	if (m_zm)
		m_zm->UpdateFPView(frameTime);
}
