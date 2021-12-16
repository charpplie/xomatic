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
#include "Single.h"
#include "Item.h"
#include "Weapon.h"
#include "Projectile.h"
#include "Actor.h"
#include "Player.h"
#include "Game.h"
#include "GameCVars.h"
#include "WeaponSystem.h"
#include <IEntitySystem.h>
#include "ISound.h"
#include <IVehicleSystem.h>
#include <IMaterialEffects.h>
#include "GameRules.h"
#include <Cry_GeoDistance.h>

#include "NanoSuit_v2.h"

#include "IronSight.h"

#include "IRenderer.h"
#include "IRenderAuxGeom.h"	

#include "WeaponSharedParams.h"
#include "ScreenEffects.h"

#include "Audio/AudioSignalPlayer.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "HUD/HUD.h"

#pragma warning(disable: 4244)

#if defined(_DEBUG)
	#define DEBUG_AIMING_AND_SHOOTING
#endif

#ifdef DEBUG_AIMING_AND_SHOOTING

//---------------------------------------------------------------------------
// TODO remove when aiming/fire direction is working
// debugging aiming dir
struct DBG_shoot{
	Vec3	src;
	Vec3	dst;
};

const	int	DGB_ShotCounter(3);
int	DGB_curIdx(-1);
int	DGB_curLimit(-1);
DBG_shoot DGB_shots[DGB_ShotCounter];
// remove over
//---------------------------------------------------------------------------

#endif

// SProbableHitInfo
CSingle::SProbableHitInfo::SProbableHitInfo()
{
	m_hit.zero();
	m_state = eProbableHitDeferredState_none;
}

void CSingle::SProbableHitInfo::OnDataReceived(const EventPhysRWIResult *pRWIResult)
{
	if(pRWIResult->nHits)
	{
		m_hit = pRWIResult->pHits[0].pt;
	}

	m_state = eProbableHitDeferredState_done;
}

//------------------------------------------------------------------------
CSingle::CSingle()
: m_projectileId(0),
	m_enabled(true),	
	m_spinUpTimer(0.0f),
	m_speed_scale(1.0f),
	m_bLocked(false),
	m_fStareTime(0.0f),
	m_spinUpEffectId(0),
	m_lockedTarget(0),
	m_spinUpTime(0),
	m_firstShot(true),
	m_next_shot(0),
	m_next_shot_dt(0),
	m_emptyclip(false),
	m_reloading(false),
	m_firing(false),
	m_fired(false),
	m_firstFire(false),
  m_heatEffectId(0),
  m_heatSoundId(INVALID_SOUNDID),
  m_barrelId(0),
	m_autoaimTimeOut(AUTOAIM_TIME_OUT),
	m_bLocking(false),
	m_autoAimHelperTimer(-1.0f),
	m_reloadCancelled(false),
	m_reloadStartFrame(0),
	m_reloadPending(false),
	m_firePending(false),
	m_lastModeStrength(false),
	m_cocking(false),
	m_smokeEffectId(0),
	m_nextHeatTime(0.0f),
	m_saved_next_shot(0.0f),
	m_fireAnimationWeight(1.0f)
{	
	m_soundVariationParam = floor_tpl(Random(1.1f,3.9f));		//1.0, 2.0f or 3.0f
	m_targetSpot.Set(0.0f,0.0f,0.0f);
}

//------------------------------------------------------------------------
CSingle::~CSingle()
{
	ClearTracerCache();
	m_fireParams = 0;

	while(!m_queuedProbableHits.empty())
		m_queuedProbableHits.pop();
}

//------------------------------------------------------------------------
void CSingle::InitFireMode(IWeapon* pWeapon, const SParentFireModeParams* pParams, uint32 id)
{
	BaseClass::InitFireMode(pWeapon, pParams, id);

	m_recoil.Init(m_pWeapon, this);

	ResetParams(NULL);
	CacheTracer();
}

//----------------------------------------------------------------------
void CSingle::PostInit()
{
	CacheAmmoGeometry();
}

//----------------------------------------------------------------------
void CSingle::InitSharedParams(const SFireModeParams* pParams)
{
	m_fireParams = pParams;
}

//------------------------------------------------------------------------
void CSingle::Update(float frameTime, uint32 frameId)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool keepUpdating=false;

	if (m_firePending)
		StartFire();

	CActor *pActor = m_pWeapon->GetOwnerActor();

	if (m_fireParams->fireparams.autoaim && m_pWeapon->IsSelected() && pActor && pActor->IsClient())
	{
		//For the LAW only use "cruise-mode" while you are using the zoom... 
		if(!m_fireParams->fireparams.autoaim_zoom || (m_fireParams->fireparams.autoaim_zoom && m_pWeapon->IsZoomed()))
			UpdateAutoAim(frameTime);
		else if(m_fireParams->fireparams.autoaim_zoom && !m_pWeapon->IsZoomed() && (m_bLocked || m_bLocking) )
			Unlock();

		keepUpdating=true;
	}

	if (m_spinUpTime>0.0f)
	{
		m_spinUpTime -= frameTime;
		if (m_spinUpTime<=0.0f)
		{
			m_spinUpTime=0.0f;
			Shoot(true);
		}

		keepUpdating=true;
	}
	else
	{
		if (m_next_shot>0.0f)
		{
			m_next_shot -= frameTime;
			if (m_next_shot<=0.0f)
			{
				m_next_shot=0.0f;
				if(!m_reloading)
				{
					m_pWeapon->SetBusy(false);
					m_pWeapon->ForcePendingActions(CWeapon::eWeaponAction_Fire);
				}
			}

			keepUpdating=true;
		}
	}

	if (IsReadyToFire())
		m_pWeapon->OnReadyToFire();

	// update spinup effect
	if (m_spinUpTimer>0.0f)
	{
		m_spinUpTimer -= frameTime;
		if (m_spinUpTimer <= 0.0f)
		{
			m_spinUpTimer = 0.0f;
			if (m_spinUpEffectId)
				SpinUpEffect(false);
		}

		keepUpdating=true;
	}


	m_recoil.Update(frameTime, m_fired, frameId, m_firstFire);
	UpdateHeat(frameTime);

	if (m_fired)
		m_firstFire = false;
	m_fired = false;

 	while(!m_queuedProbableHits.empty())
	{
		SProbableHitInfo *probableHit = m_queuedProbableHits.front();
		if(probableHit->m_state >= eProbableHitDeferredState_done)
		{
			NetShootDeferred(probableHit->m_hit);
			probableHit->m_state = eProbableHitDeferredState_none;
			m_queuedProbableHits.pop();
		}
		else
		{
			break;
		}
	}

	if(!m_queuedProbableHits.empty())
	{
		keepUpdating = true;
	}

	if (keepUpdating)
		m_pWeapon->RequireUpdate(eIUS_FireMode);

#ifdef DEBUG_AIMING_AND_SHOOTING
	//---------------------------------------------------------------------------
	// TODO remove when aiming/fire direction is working
	// debugging aiming dir

	static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
	if(pAimDebug->GetIVal()!=0)
	{
		const ColorF	queueFireCol( .4f, 1.0f, 0.4f, 1.0f );
		for(int dbgIdx(0);dbgIdx<DGB_curLimit; ++dbgIdx)
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( DGB_shots[dbgIdx].src, queueFireCol, DGB_shots[dbgIdx].dst, queueFireCol );
	}

	if (g_pGameCVars->aim_assistCrosshairDebug && m_fireParams->fireparams.crosshair_assist_range>0.f && m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsClient())
	{
		// debug only
		bool bHit(false); ray_hit rayhit; rayhit.pCollider=0;
		Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE, &bHit, &rayhit);
		Vec3 pos = GetFiringPos(hit);
		Vec3 dir = GetFiringDir(hit, pos);
		CrosshairAssistAiming(pos, dir, &rayhit);

		keepUpdating = true;
	}
	//---------------------------------------------------------------------------
#endif

	UpdateFireAnimationWeight(frameTime);
}

void CSingle::PostUpdate(float frameTime)
{
	bool ok = false;
	bool startTarget = false;

	if (m_targetSpotSelected)
	{
		
		if(m_autoAimHelperTimer>0.0f)
		{
			m_autoAimHelperTimer -= frameTime;
			
			if(m_autoAimHelperTimer<=0.0f)
			{
				bool bHit = true;
				Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE,&bHit);
				if(bHit && !OutOfAmmo())
				{
					m_targetSpotSelected = true;
					m_targetSpot = hit;
					startTarget = true;
					ok = true;
				}
				else
					m_targetSpot.Set(0.0f,0.0f,0.0f);
			}
		}
		else
		{
			ok = true;
		}

		const SAmmoParams *pAmmoParams = GetAmmoParams();
		if (pAmmoParams && ok)
		{
			if (pAmmoParams->physicalizationType != ePT_None)
			{
				float speed = pAmmoParams->speed;
				if (speed == 0.0f)
					speed = 60.0f;
        Vec3 hit = m_targetSpot;
				Vec3 pos = m_lastAimSpot;

				float x, y;
				Vec3 diff = hit - pos;
				y = diff.z;
				diff.z = 0;
				x = diff.GetLength();
				float angle = GetProjectileFiringAngle(speed,9.8f,x,y);
				Matrix33 m = Matrix33::CreateRotationVDir(diff.normalize());
				m.OrthonormalizeFast();
				Ang3 aAng = RAD2DEG(Ang3::GetAnglesXYZ(m));
				aAng.x = angle;
				Ang3 ang2 = DEG2RAD(aAng);
				Matrix33 m2 = Matrix33::CreateRotationXYZ(ang2);
				Vec3 dir3 = m2.GetColumn(1);
				dir3 = dir3.normalize() * WEAPON_HIT_RANGE;
				Vec3 spot = pos + dir3;

				if(spot.z > m_targetSpot.z)
				{
					m_pWeapon->SetAimLocation(spot);
					m_pWeapon->SetTargetLocation(m_targetSpot);
				}
				else
				{
					startTarget = false;
				}
			}
			if(startTarget)
			{
				m_pWeapon->ActivateTarget(true);		//Activate Targeting on the weapon
				m_pWeapon->OnStartTargetting(m_pWeapon);
			}

		}

	}
}

void CSingle::UpdateFPView(float frameTime)
{
	if (m_targetSpotSelected)
	{
		Vec3 hit = m_targetSpot;
		m_lastAimSpot = GetFiringPos(hit);
	}

	UpdateBulletBelt();
}

void CSingle::UpdateBulletBelt()
{
	if(m_fireParams->bulletBelletParams.numBullets > 0)
	{
		int ammoCount = m_pWeapon->GetAmmoCount(GetAmmoType());

		if(ammoCount < m_fireParams->bulletBelletParams.numBullets)
		{
			ICharacterInstance* pCharacter = m_pWeapon->GetEntity()->GetCharacter(eIGS_FirstPerson);

			if(pCharacter)
			{
				if(m_reloading)
				{
					ISkeletonAnim* pSkelAnim = pCharacter->GetISkeletonAnim();

					if(pSkelAnim && pSkelAnim->GetLayerTime(0) > m_fireParams->bulletBelletParams.beltRefillReloadFraction)
					{
						return;
					}
				}

				ISkeletonPose* pSkelPose = pCharacter->GetISkeletonPose();

				if(pSkelPose)
				{
					int bulletNum = ammoCount+1;
					string jointName;

					jointName.Format("%s%d", m_fireParams->bulletBelletParams.jointName.c_str(), bulletNum);

					int16 jointId = pSkelPose->GetJointIDByName(jointName.c_str());

					CRY_ASSERT_MESSAGE(jointId >= 0, "Invalid joint name in bullet belt params");

					QuatT newQuat;

					newQuat.SetIdentity();
					newQuat.SetTranslation(Vec3(0.f, -2.f, 2.0f));

					pSkelPose->SetPostProcessQuat(jointId, newQuat);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
bool CSingle::IsValidAutoAimTarget(IEntity* pEntity, int partId /*= 0*/)
{  
  IActor *pActor = 0;				
  IVehicle* pVehicle = 0;
  
  if (pEntity->IsHidden())
    return false;

  AABB box;
  pEntity->GetLocalBounds(box);
  float vol = box.GetVolume();  
  
  if (vol < m_fireParams->fireparams.autoaim_minvolume || vol > m_fireParams->fireparams.autoaim_maxvolume)
  {
    //CryLogAlways("volume check failed: %f", vol);
    return false;
  }
 
	CActor* pPlayer = m_pWeapon->GetOwnerActor();

	if(!pPlayer)
		return false;

  pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
	if (pActor == NULL)
		return false;

  if (pActor->GetHealth() > 0.f && 
		pActor->GetEntity()->GetAI() && pActor->GetEntity()->GetAI()->IsHostile(pPlayer->GetEntity()->GetAI(),false))
    return true;
  
  pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId()); 
  if (gEnv->bMultiplayer && pVehicle && pVehicle->GetStatus().health > 0.f)
	{
		//Check for teams
		if(CGameRules* pGameRules = g_pGame->GetGameRules())
		{
			if(pGameRules->GetTeam(pVehicle->GetEntityId())!=pGameRules->GetTeam(pPlayer->GetEntityId()))
				return true;
		}
		return false;
	}
	

	if (pVehicle && pVehicle->GetStatus().health > 0.f &&
		pVehicle->GetEntity()->GetAI() && pVehicle->GetEntity()->GetAI()->IsHostile(pPlayer->GetEntity()->GetAI(),false))
    return true;
    
  return false;
}

//------------------------------------------------------------------------
bool CSingle::CheckAutoAimTolerance(const Vec3& aimPos, const Vec3& aimDir)
{
  // todo: this check is probably not sufficient
  IEntity *pLocked = gEnv->pEntitySystem->GetEntity(m_lockedTarget);
	if(!pLocked)
		return false;

  AABB bbox;
  pLocked->GetWorldBounds(bbox);
  Vec3 targetPos = bbox.GetCenter();
  Vec3 dirToTarget = (targetPos - aimPos).normalize();
  float dot = aimDir.Dot(dirToTarget);
  Matrix33 mat = Matrix33::CreateRotationVDir(dirToTarget);
  Vec3 right = mat.GetColumn(0).normalize();
  Vec3 maxVec = (targetPos - aimPos) + (right * m_fireParams->fireparams.autoaim_tolerance);
  float maxDot = dirToTarget.Dot(maxVec.normalize());
  
  return (dot >= maxDot);
}

void CSingle::Lock(EntityId targetId, int partId /*=0*/)
{
	m_lockedTarget = targetId;
	m_bLocking = false;
	m_bLocked = true;
	m_autoaimTimeOut = AUTOAIM_TIME_OUT;

	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if (pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimLocked(m_lockedTarget));

			_smart_ptr< ISound > pBeep = gEnv->pSoundSystem->CreateSound("sounds/interface:hud:target_lock", 0);
			if (pBeep)
			{
				pBeep->SetSemantic(eSoundSemantic_HUD);
				pBeep->Play();
			}
		}
	}
}

void CSingle::ResetLock()
{
	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if ((m_bLocking || m_bLocked) && pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimUnlock(m_lockedTarget));
		}
	}

	m_bLocked = false;
	m_bLocking = false;
	m_lockedTarget = 0;
	m_fStareTime = 0.0f;
	m_autoaimTimeOut = AUTOAIM_TIME_OUT;
}

void CSingle::Unlock()
{
	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if (pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimUnlock(m_lockedTarget));
		}
	}

	m_bLocked = false;
	m_bLocking = false;
	m_lockedTarget = 0;
	m_fStareTime = 0.0f;
	m_autoaimTimeOut = AUTOAIM_TIME_OUT;
}

void CSingle::StartLocking(EntityId targetId, int partId /*=0*/)
{
	// start locking
	m_lockedTarget = targetId;
	m_bLocking = true;
	m_bLocked = false;
	m_fStareTime = 0.0f;

	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if (pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimLocking(m_lockedTarget));
		}
	}
}

//------------------------------------------------------------------------
void CSingle::UpdateAutoAim(float frameTime)
{
	CActor *pOwner = m_pWeapon->GetOwnerActor();
  if (!pOwner || !pOwner->IsClient())
    return;
	
  // todo: use crosshair/aiming dir
	IMovementController *pMC = pOwner->GetMovementController();
	if (!pMC)
		return;

	SMovementState state;
	pMC->GetMovementState(state);

	Vec3 aimDir = state.eyeDirection;
	Vec3 aimPos = state.eyePosition;
	
	float maxDistance = m_fireParams->fireparams.autoaim_distance;
  
  ray_hit ray;
  
  IPhysicalEntity* pSkipEnts[10];
  int nSkipEnts = GetSkipEntities(m_pWeapon, pSkipEnts, 10);
  	
	const int objects = ent_all;
	const int flags = (geom_colltype_ray << rwi_colltype_bit) | rwi_colltype_any | (8 & rwi_pierceability_mask) | (geom_colltype14 << rwi_colltype_bit);

	int result = gEnv->pPhysicalWorld->RayWorldIntersection(aimPos, aimDir * 2.f * maxDistance, 
		objects, flags, &ray, 1, pSkipEnts, nSkipEnts);		

  bool hitValidTarget = false;
  IEntity* pEntity = 0;

  if (result && ray.pCollider)
	{	
		pEntity = (IEntity *)ray.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);    	        
    if (pEntity && IsValidAutoAimTarget(pEntity))
      hitValidTarget = true;
  }

	if(m_bLocked)
		m_autoaimTimeOut -= frameTime;

  if (hitValidTarget && ray.dist <= maxDistance)
  {	
    if (m_bLocked)
		{
			if ((m_lockedTarget != pEntity->GetId()) && m_autoaimTimeOut<=0.0f)
				StartLocking(pEntity->GetId());
		}
		else
		{	
			if (!m_bLocking || m_lockedTarget!=pEntity->GetId())
				StartLocking(pEntity->GetId());
			else
				m_fStareTime += frameTime;
		}
	}
	else if(!hitValidTarget && m_bLocking)
	{
		m_pWeapon->RequestUnlock();
		Unlock();
	}
	else
	{
		// check if we're looking far away from our locked target
		if ((m_bLocked && !(ray.dist<=maxDistance && CheckAutoAimTolerance(aimPos, aimDir))) || (!m_bLocked && m_lockedTarget && m_fStareTime != 0.0f))
    { 
			if(!m_fireParams->fireparams.autoaim_timeout)
			{
				m_pWeapon->RequestUnlock();
				Unlock();
			}
    }
	}

  if (m_bLocking && !m_bLocked && m_fStareTime >= m_fireParams->fireparams.autoaim_locktime && m_lockedTarget)
		m_pWeapon->RequestLock(m_lockedTarget);
	else if(m_bLocked && hitValidTarget && m_lockedTarget!=pEntity->GetId())
		m_pWeapon->RequestLock(pEntity->GetId());
  else if (m_bLocked)
	{
    // check if target still valid (can e.g. be killed)
    IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_lockedTarget);	
		if ((pEntity && !IsValidAutoAimTarget(pEntity)) || (m_fireParams->fireparams.autoaim_timeout && m_autoaimTimeOut<=0.0f))
		{
			m_pWeapon->RequestUnlock();
			Unlock();
		}
	}
}

//------------------------------------------------------------------------
void CSingle::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CSingle::ResetParams(const IItemParamsNode *params)
{
	ResetRecoilMod();
	ResetSpreadMod();
}

//------------------------------------------------------------------------
void CSingle::Activate(bool activate)
{
	m_fired = m_firstFire = m_firing = m_reloading = m_emptyclip = m_cocking = false;
	m_spinUpTime = 0.0f;
	m_next_shot = 0.0f;
	if (m_fireParams->fireparams.rate > 0.0f)
		m_next_shot_dt = 60.0f/m_fireParams->fireparams.rate;
	else
		m_next_shot_dt = 0.001f;

  m_barrelId = 0;
  m_mfIds.resize(m_fireParams->fireparams.barrel_count);
  
	if(CanOverheat() && !activate)
		RestoreOverHeating(false);
  
	m_heat = 0.0f;
	m_overheat = 0.0f;
	m_reloadPending = false;

	if(CanOverheat() && activate)
		RestoreOverHeating(true);

  m_pWeapon->StopSound(m_heatSoundId);  
  m_heatSoundId = INVALID_SOUNDID;  
  
  if (!activate && m_heatEffectId)
    m_heatEffectId = m_pWeapon->AttachEffect(0, m_heatEffectId, false);
	if (!activate && m_smokeEffectId)
		m_heatEffectId = m_pWeapon->AttachEffect(0, m_smokeEffectId, false);

	m_targetSpotSelected = false;
	m_reloadCancelled = false;

  if (!activate)
	{
	  SetupEmitters(false);
	}
	else if (m_pWeapon->GetStats().fp)
	{
		//Pre-setup muzzleflash emitters for FP gun, to avoid first shot lagging
		SetupEmitters(true);
	}
  	
  SpinUpEffect(false);

  m_firstShot = activate;
	
	ResetLock();

  if (activate && m_fireParams->fireparams.autoaim)
    m_pWeapon->RequireUpdate(eIUS_FireMode);  

  m_fStareTime = 0.f;    

	ResetRecoil();  

	if(m_pWeapon->IsZoomed())
	{
		if(activate)
		{
			if(IZoomMode* pZm = m_pWeapon->GetZoomMode(m_pWeapon->GetCurrentZoomMode()))
				pZm->ApplyZoomMod(this);
		}
		else
		{
			ResetRecoilMod();
			ResetSpreadMod();
		}
	}

	if (!activate)
	{
		Cancel();
	}
	else if( /*activate && */ m_pWeapon->GetOwnerId() == gEnv->pGame->GetIGameFramework()->GetClientActorId() )
	{
		SHUDEvent hudCrosshairEvent( eHUDEvent_OnCrosshairSelected );
		hudCrosshairEvent.AddData( m_fireParams->fireparams.crosshair );
		CHUD::CallEvent(hudCrosshairEvent);
	}
}

//------------------------------------------------------------------------
int CSingle::GetAmmoCount() const
{
	return m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class);
}

//------------------------------------------------------------------------
int CSingle::GetClipSize() const
{
	const int clipSize = m_fireParams->fireparams.clip_size;
	if(clipSize > 0)
	{
		if(CActor* pOwner = m_pWeapon->GetOwnerActor())
		{
			if (pOwner->IsPlayer())
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(pOwner);
				if(pPlayer->IsPerkActive(ePerk_HeavyLoadout))
				{
					return (int) (clipSize * 1.5f);
				}
			}
			else
			{
				return (m_fireParams->fireparams.ai_infiniteAmmo == false) ? clipSize : -1;
			}
		}
	}

	return clipSize;
}

//------------------------------------------------------------------------
bool CSingle::OutOfAmmo() const
{
	int clipSize = GetClipSize();
	if (clipSize!=0)
		return m_fireParams->fireparams.ammo_type_class && clipSize != -1 && m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class) < 1;

	return m_fireParams->fireparams.ammo_type_class && (m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class) < 1);
}

//------------------------------------------------------------------------
bool CSingle::LowAmmo(float thresholdPerCent) const
{
	int clipSize = GetClipSize();
	if (clipSize!=0)
		return m_fireParams->fireparams.ammo_type_class && clipSize != -1 && m_pWeapon->GetAmmoCount(m_fireParams->fireparams.ammo_type_class) < (int)(thresholdPerCent*clipSize);

	return m_fireParams->fireparams.ammo_type_class && (m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class) < (int)(thresholdPerCent*clipSize));
}

//------------------------------------------------------------------------
bool CSingle::CanReload() const
{
	CActor * pActor = m_pWeapon->GetOwnerActor();
	
	int clipSize = GetClipSize();

	bool isAI = pActor ? !pActor->IsPlayer() : false;

	if(m_fireParams->fireparams.bullet_chamber)
		clipSize += 1;

	if (clipSize!=0)
	{
		return !m_reloading && !m_reloadPending && (GetAmmoCount() < clipSize) && 
			((m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class) > 0) || isAI);
	}
	return false;
}

bool CSingle::IsReloading()
{
	return m_reloading || m_reloadPending;
}

//------------------------------------------------------------------------
void CSingle::Reload(int zoomed)
{
	StartReload(zoomed);
}

//------------------------------------------------------------------------
bool CSingle::CanFire(bool considerAmmo) const
{
	return !m_reloading && !m_reloadPending && (m_next_shot<=0.0f) && (m_spinUpTime<=0.0f) && (m_overheat<=0.0f) &&
		!m_pWeapon->IsBusy() && (!considerAmmo || !OutOfAmmo() || !m_fireParams->fireparams.ammo_type_class || GetClipSize() == -1);
}

//------------------------------------------------------------------------
void CSingle::StartFire()
{
	CCCPOINT(Single_StartFire);

	if (m_fireParams->fireparams.aim_helper && !m_targetSpotSelected)
	{
		m_targetSpotSelected = true;
		SetAutoAimHelperTimer(m_fireParams->fireparams.aim_helper_delay);
		m_pWeapon->GetGameObject()->EnablePostUpdates(m_pWeapon);
		return;
	}

	if (m_pWeapon->IsBusy())
	{
		if (inrange(m_next_shot, 0.0f, g_pGameCVars->g_fireStabilizationThreshold))
		{
			m_firePending = true;
		}

		return;
	}


	if (m_fireParams->fireparams.spin_up_time>0.0f)
	{
		m_firing = true;
		m_spinUpTime = m_fireParams->fireparams.spin_up_time;

		m_pWeapon->PlayAction(m_fireParams->actions.spin_up);
		SpinUpEffect(true);
	}
	else
	{
		m_firing = Shoot(true);
		if (m_firing)
			m_pWeapon->SetBusy(true);
	}

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CSingle::StopFire()
{
	if (!m_firing)
		return;

	CCCPOINT(Single_StopFire);

	if (m_targetSpotSelected)
	{

		CActor* pOwner = m_pWeapon->GetOwnerActor();

		if(pOwner && !pOwner->CanFire())
		{
			Cancel();
		}
		else
		{
			if (m_fireParams->fireparams.spin_up_time>0.0f)
			{
				m_firing = true;
				m_spinUpTime = m_fireParams->fireparams.spin_up_time;

				m_pWeapon->PlayAction(m_fireParams->actions.spin_up);
				SpinUpEffect(true);
			}
			else
			{
				m_firing = Shoot(true);
			}

			m_targetSpotSelected = false;
			m_pWeapon->RequireUpdate(eIUS_FireMode);
			m_pWeapon->ActivateTarget(false);
			m_pWeapon->OnStopTargetting(m_pWeapon);
			m_pWeapon->GetGameObject()->DisablePostUpdates(m_pWeapon);
		}

	}
	
	
	SmokeEffect();

	m_firing = false;
}

bool CSingle::IsSilenced() const
{
	return m_fireParams->fireparams.is_silenced;
}

//------------------------------------------------------------------------
const char *CSingle::GetType() const
{
	return "Single";
}

//------------------------------------------------------------------------
IEntityClass* CSingle::GetAmmoType() const
{
	return m_fireParams->fireparams.ammo_type_class;
}

//------------------------------------------------------------------------
const SAmmoParams* CSingle::GetAmmoParams() const
{
	IEntityClass *pAmmoClass = GetAmmoType();

	return (pAmmoClass ? g_pGame->GetWeaponSystem()->GetAmmoParams(pAmmoClass) : NULL);
}

//------------------------------------------------------------------------
float CSingle::GetSpinUpTime() const
{
	return m_fireParams->fireparams.spin_up_time;
}

//------------------------------------------------------------------------
float CSingle::GetSpinDownTime() const
{
	return m_fireParams->fireparams.spin_down_time;
}

//------------------------------------------------------------------------
float CSingle::GetNextShotTime() const
{
  return m_next_shot;
}

//------------------------------------------------------------------------
void CSingle::SetNextShotTime(float time)	
{
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (pActor && pActor->IsPlayer())
	{
		CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
		if(pPlayer->IsPerkActive(ePerk_ConcentratedFire))
		{
			const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
			time *= perkVars->perk_ConcentratedFire_rateScale;
			CCCPOINT(Perk_ConcentratedFire_ScaledFireSpeed);
		}
	}

	m_next_shot = time;
	if (time>0.0f)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
float CSingle::GetFireRate() const
{
  return m_fireParams->fireparams.rate;
}

//------------------------------------------------------------------------
void CSingle::Enable(bool enable)
{
	m_enabled = enable;
}

//------------------------------------------------------------------------
bool CSingle::IsEnabled() const
{
	return m_enabled;
}

//------------------------------------------------------------------------
struct CSingle::FillAmmoAction
{
	FillAmmoAction(CSingle *_single, int reloadStartFrame):
	single(_single), _reloadStartFrame(reloadStartFrame){};
	
	CSingle *single;
	int _reloadStartFrame;

	void execute(CItem *_this)
	{
		if(single->m_reloadStartFrame == _reloadStartFrame)
			single->FillAmmo();
	}
};

//------------------------------------------------------------------------
struct CSingle::EndReloadAction
{
	EndReloadAction(CSingle *_single, int zoomed, int reloadStartFrame):
	single(_single), _zoomed(zoomed), _reloadStartFrame(reloadStartFrame){};

	CSingle *single;
	int _zoomed;
	int _reloadStartFrame;

	void execute(CItem *_this)
	{
		if(single->m_reloadStartFrame == _reloadStartFrame)
			single->EndReload(_zoomed);
	}
};

struct CSingle::StartReload_SliderBack
{
	StartReload_SliderBack(CSingle *_single): single(_single) {};
	CSingle *single;

	void execute(CItem *_this)
	{
		_this->StopLayer(single->m_fireParams->fireparams.slider_layer);
	}
};

void CSingle::CancelReload()
{
	m_reloadCancelled = true;
	m_reloadPending = false;
	EndReload(0);
}

const char* CSingle::GetBestReloadAction(int ammoCount)
{
	if ((ammoCount>0) && (ammoCount < (GetClipSize()+1)))
		return m_fireParams->actions.reload_chamber_full.c_str();
	else
		return m_fireParams->actions.reload_chamber_empty.c_str();
}


void CSingle::StartReload(int zoomed)
{
	CCCPOINT(Single_StartReload);

	CActor* pOwnerActor = m_pWeapon->GetOwnerActor();

	m_reloading = true;
	if (zoomed != 0)
		m_pWeapon->ExitZoom(true);
	m_pWeapon->SetBusy(true);
	
	const char *action = m_fireParams->actions.reload.c_str();
	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;

	m_pWeapon->OnStartReload(m_pWeapon->GetOwnerId(), ammo);

	//When interrupting reload to melee, scheduled reload action can get a bit "confused"
	//This way we can verify that the scheduled EndReloadAction matches this StartReload call... 
	m_reloadStartFrame = gEnv->pRenderer->GetFrameID();

	if (m_fireParams->fireparams.bullet_chamber)
	{
		action = GetBestReloadAction(m_pWeapon->GetAmmoCount(ammo));
	}

	float speedOverride = -1.0f;
	if (pOwnerActor)
	{
		speedOverride = pOwnerActor->GetReloadSpeedScale();
	}

	m_pWeapon->PlayAction(action,0,false,CItem::eIPAF_Default,speedOverride);

	uint32 animTime = 0;
	uint32 fillAmmoTime = 0;
	if (pOwnerActor && pOwnerActor->IsPlayer())
	{
		animTime = m_pWeapon->GetCurrentAnimationTime(eIGS_Owner);
		fillAmmoTime = uint32(animTime * m_fireParams->fireparams.fillAmmoReloadFraction);
	}
	else
	{
		animTime = (uint32)(GetShared()->fireparams.ai_reload_time * 1000.0f);
		fillAmmoTime = animTime;
	}
	m_pWeapon->GetScheduler()->TimerAction(fillAmmoTime, CSchedulerAction<FillAmmoAction>::Create(FillAmmoAction(this, m_reloadStartFrame)), false);
	m_pWeapon->GetScheduler()->TimerAction(animTime, CSchedulerAction<EndReloadAction>::Create(EndReloadAction(this, zoomed, m_reloadStartFrame)), false);
	int time = (MAX(0,(animTime - m_fireParams->fireparams.slider_layer_time)));

	//Proper end reload timing for MP only (for clients, not host)
	if (gEnv->bClient && !gEnv->bServer)
	{
		if (pOwnerActor && pOwnerActor->IsClient() && !gEnv->bServer)
			m_reloadPending=true;
	}

	m_pWeapon->GetScheduler()->TimerAction(time, CSchedulerAction<StartReload_SliderBack>::Create(this), false);
}

//------------------------------------------------------------------------
void CSingle::EndReload(int zoomed)
{
	CCCPOINT(Single_EndReload);
	m_reloading = false;
	m_emptyclip = false;
	m_spinUpTime = m_firing?m_fireParams->fireparams.spin_up_time:0.0f;

	if (m_pWeapon->IsServer() && !m_reloadCancelled)
	{	
		m_pWeapon->SendEndReload();

		m_pWeapon->GetGameObject()->Pulse('bang');
	}

	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	m_pWeapon->OnEndReload(m_pWeapon->GetOwnerId(), ammo);

	m_reloadStartFrame = 0;
	m_reloadCancelled = false;
	m_pWeapon->SetBusy(false);
	m_pWeapon->ForcePendingActions(CWeapon::eWeaponAction_Reload); 
}

//------------------------------------------------------------------------
void CSingle::FillAmmo()
{
	CCCPOINT(Single_FillAmmo);

	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;

	if (m_pWeapon->IsServer() && !m_reloadCancelled)
	{
		CActor * pOwnerActor = m_pWeapon->GetOwnerActor();
		bool ai= pOwnerActor ? !pOwnerActor->IsPlayer() : false;

		int ammoCount = m_pWeapon->GetAmmoCount(ammo);
		int inventoryCount = m_pWeapon->GetInventoryAmmoCount(m_fireParams->fireparams.ammo_type_class);
		const int clipSize = GetClipSize();
		int refill = min(inventoryCount, clipSize - ammoCount);

		if (m_fireParams->fireparams.bullet_chamber && (ammoCount>0) && (ammoCount < clipSize+1) && ((inventoryCount-refill)>0))
			ammoCount += ++refill;
		else
			ammoCount += refill;

		if(ai)
			ammoCount = clipSize;

		m_pWeapon->SetAmmoCount(ammo, ammoCount);

		if ((m_fireParams->fireparams.max_clips != -1) && !ai)
			m_pWeapon->SetInventoryAmmoCount(ammo, m_pWeapon->GetInventoryAmmoCount(ammo)-refill);

		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponReload, GetAmmoType()->GetName(), refill, (void *)(m_pWeapon->GetEntityId())));
	}
}

//------------------------------------------------------------------------
struct CSingle::RezoomAction
{
	RezoomAction(){};
	void execute(CItem *pItem)
	{
		CWeapon *pWeapon=static_cast<CWeapon *>(pItem);
		IZoomMode *pIZoomMode = pWeapon->GetZoomMode(pWeapon->GetCurrentZoomMode());

		if (pIZoomMode)
		{
			CIronSight *pZoomMode=static_cast<CIronSight *>(pIZoomMode);
			pZoomMode->TurnOff(false);
		}
	}
};

struct CSingle::Shoot_SliderBack
{
	Shoot_SliderBack(CSingle *_single): pSingle(_single) {};
	CSingle *pSingle;

	void execute(CItem *pItem)
	{
		pItem->StopLayer(pSingle->m_fireParams->fireparams.slider_layer);
	}
};

struct CSingle::CockAction
{
	CSingle *pSingle;
	CockAction(CSingle *_single): pSingle(_single) {};
	void execute(CItem *pItem)
	{
		pItem->PlayAction(pSingle->m_fireParams->actions.cock);
		pItem->GetScheduler()->TimerAction(pItem->GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<RezoomAction>::Create(), false);

		const int currentAnimTime = pItem->GetCurrentAnimationTime(eIGS_Owner);
		const int time = MAX(0,currentAnimTime - pSingle->m_fireParams->fireparams.slider_layer_time);
		pItem->GetScheduler()->TimerAction(time, CSchedulerAction<Shoot_SliderBack>::Create(pSingle), false);
	}
};

class CSingle::ScheduleAutoReload
{
public:
	ScheduleAutoReload(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) 
	{
		_pWeapon->SetBusy(false);
		_pWeapon->OnFireWhenOutOfAmmo();
	}
private:
	CWeapon *_pWeapon;
};

namespace
{
  struct CompareEntityDist
  {
    CompareEntityDist(const Vec3& to) : m_to(to) {}
    
    ILINE bool operator()( const IEntity* lhs, const IEntity* rhs ) const
    { 
      return m_to.GetSquaredDistance(lhs->GetWorldPos()) < m_to.GetSquaredDistance(rhs->GetWorldPos());
    }
    
    Vec3 m_to;
  };
}

bool CSingle::CrosshairAssistAiming(const Vec3& firingPos, Vec3& firingDir, ray_hit* pRayhit)
{ 
  // farcry-style crosshair-overlap aim assistance
  
  IEntity* pSelf = m_pWeapon->GetOwner();
  if (!pSelf)
    return false;

  IEntity* pEntity = pRayhit->pCollider ? gEnv->pEntitySystem->GetEntityFromPhysics(pRayhit->pCollider) : 0;  
  if (pEntity && m_pWeapon->IsValidAssistTarget(pEntity, pSelf, false))
    return false;

  const CCamera& cam = gEnv->pRenderer->GetCamera();
  Lineseg lineseg(cam.GetPosition(), cam.GetPosition()+m_fireParams->fireparams.crosshair_assist_range*cam.GetViewdir());  
  
  float t = 0.f;  
  AABB bounds;
  int debugY = 100;
  std::vector<IEntity*> ents;
/*    
	int highestIndex = 0;
	const CHUDRadar::RadarEntity *pEntities = CHUD::Get()->GetRadar()->GetNearbyEntities(highestIndex);
  for(int i=0,n=highestIndex; i<n; ++i)  
  {
		EntityId id = pEntities[i].m_id;
		if(id == 0)
			continue;
    // fast reject everything behind player, or too far away from line of view
    // rest is sorted by distance and checked against the crosshair in screen-space    
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id);
		if(!pEntity)
			continue;

    Vec3 wpos = pEntity->GetWorldPos();
    
    if (!(pEntity && firingPos.GetSquaredDistance(wpos) < sqr(m_fireParams->fireparams.crosshair_assist_range) && firingDir.Dot((wpos-firingPos).GetNormalized()) > 0.5f))
      continue;

    if (!m_pWeapon->IsValidAssistTarget(pEntity, pSelf, false))
      continue;
    
    pEntity->GetLocalBounds(bounds);
    
    // sufficient for entities of interest
    if (bounds.GetVolume() > 1000.f || Distance::Point_LinesegSq(pEntity->GetWorldTM()*bounds.GetCenter(), lineseg, t) > sqr(10.f)) 
      continue;

    ents.push_back(pEntity);
  }
*/
  if (ents.empty())
    return false;

  // make sure camera is set correctly; can still be set to ortho projection by text rendering or similar (ask Andrej)  
  gEnv->pRenderer->PushMatrix();
  gEnv->pRenderer->SetCamera(cam);

  const float crosshairSize = g_pGameCVars->aim_assistCrosshairSize; // should be queried from HUD, but not possible yet            
  float cx = crosshairSize/gEnv->pRenderer->GetWidth() * 100.f / 2.f;
  float cy = crosshairSize/gEnv->pRenderer->GetHeight() * 100.f / 2.f;            
   
  std::sort(ents.begin(), ents.end(), CompareEntityDist(firingPos));  
  
  IEntity* pTarget = 0;
  Vec3 newPos, curr;
  
  for (int i=0,n=ents.size(); i<n; ++i)
  {
    IEntity* pEntity = ents[i];
    
    Vec3 wpos = pEntity->GetWorldPos();
    Quat rot = pEntity->GetWorldRotation();
    pEntity->GetLocalBounds(bounds);

    static Vec3 points[8];
    points[0] = wpos + rot * bounds.min;
    points[1] = wpos + rot * Vec3(bounds.min.x, bounds.max.y, bounds.min.z);
    points[2] = wpos + rot * Vec3(bounds.max.x, bounds.max.y, bounds.min.z);
    points[3] = wpos + rot * Vec3(bounds.max.x, bounds.min.y, bounds.min.z);
    points[4] = wpos + rot * Vec3(bounds.min.x, bounds.min.y, bounds.max.z);
    points[5] = wpos + rot * Vec3(bounds.min.x, bounds.max.y, bounds.max.z);    
    points[6] = wpos + rot * Vec3(bounds.max.x, bounds.min.y, bounds.max.z);
    points[7] = wpos + rot * bounds.max;
    
    Vec2 smin(100,100), smax(-100,-100);

    for (int i=0; i<8; ++i)
    {
      gEnv->pRenderer->ProjectToScreen(points[i].x, points[i].y, points[i].z, &curr.x, &curr.y, &curr.z);
      smin.x = min(smin.x, curr.x); smin.y = min(smin.y, curr.y);
      smax.x = max(smax.x, curr.x); smax.y = max(smax.y, curr.y);      
    }

    smin.x -= 50.f; smin.y -= 50.f;
    smax.x -= 50.f; smax.y -= 50.f;

    bool xin = smin.x <= -cx && smax.x >= cx;
    bool yin = smin.y <= -cy && smax.y >= cy;    
    bool overlap = (xin || abs(smin.x) <= cx || abs(smax.x) <= cx) && (yin || abs(smin.y) <= cy || abs(smax.y) <= cy);

    if (g_pGameCVars->aim_assistCrosshairDebug)
    {
      float color[] = {1,1,1,1};
      gEnv->pRenderer->Draw2dLabel(100,debugY+=20,1.3f,color,false,"%s: min (%.1f %.1f), max: (%.1f %.1f) %s", pEntity->GetName(), smin.x, smin.y, smax.x, smax.y, overlap?"OVERLAP":"");    
    }
    
    if (overlap && !pTarget)
    { 
      pe_status_dynamics dyn;
      if (pEntity->GetPhysics() && pEntity->GetPhysics()->GetStatus(&dyn))
        newPos = dyn.centerOfMass;
      else
        newPos = wpos + rot*bounds.GetCenter();  

      // check path to new target pos 
      ray_hit chkhit;
      IPhysicalEntity* pSkip = m_pWeapon->GetEntity()->GetPhysics(); // we shouldn't need all child entities for skipping at this point (subject to be proven)
      if (gEnv->pPhysicalWorld->RayWorldIntersection(firingPos, 1.1f*(newPos-firingPos), ent_all, (13&rwi_pierceability_mask), &chkhit, 1, &pSkip, 1))
      {
        IEntity *pFound = chkhit.pCollider ? gEnv->pEntitySystem->GetEntityFromPhysics(chkhit.pCollider) : 0;
        if (pFound != pEntity)
          continue;
      }

      pTarget = pEntity;
      
      if (!g_pGameCVars->aim_assistCrosshairDebug)
        break;
    }
  }   

  gEnv->pRenderer->PopMatrix();

  if (pTarget)
  { 
    firingDir = newPos - firingPos;
    firingDir.Normalize();
    
    if (g_pGameCVars->aim_assistCrosshairDebug)
    {
      IPersistantDebug* pDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
      pDebug->Begin("CHAimAssistance", false);
      pDebug->AddLine(firingPos, newPos, ColorF(0,1,0,1), 0.5f);
      pDebug->AddSphere(newPos, 0.3f, ColorF(1,0,0,1), 0.5f);
    }    
  }
  
  return true;
}


bool CSingle::Shoot(bool resetAnimation, bool autoreload, bool isRemote)
{
	CCCPOINT(Single_Shoot);
	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);

	CActor *pActor = m_pWeapon->GetOwnerActor();
	
	bool playerIsShooter = pActor?pActor->IsPlayer():false;
  bool clientIsShooter = pActor?pActor->IsClient():false;

	int clipSize = GetClipSize();

	if (clipSize==0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	m_firePending = false;
	if (!CanFire(true))
	{
		if ((ammoCount <= 0) && !m_reloading && !m_reloadPending)
			m_pWeapon->PlayAction(m_fireParams->actions.empty_clip);
		return false;
	}
	else if(m_pWeapon->IsWeaponLowered())
	{
		m_pWeapon->PlayAction(m_fireParams->actions.null_fire);
		return false;
	}

  bool bHit = false;
  ray_hit rayhit;	 
  rayhit.pCollider = 0;
  
  Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE, &bHit, &rayhit);
	Vec3 pos = GetFiringPos(hit);
	Vec3 dir = GetFiringDir(hit, pos);		//Spread was already applied if required in GetProbableHit( )
	Vec3 vel = GetFiringVelocity(dir);

  if (!gEnv->bMultiplayer && clientIsShooter && m_fireParams->fireparams.crosshair_assist_range > 0.0f && !m_pWeapon->IsZoomed())  
    CrosshairAssistAiming(pos, dir, &rayhit);      

	int flags = CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_CleanBlending;

	const char *action = m_fireParams->actions.fire_cock.c_str();
	if (ammoCount == 1 || (m_fireParams->fireparams.no_cock && m_pWeapon->IsZoomed()) || (m_fireParams->fireparams.unzoomed_cock && m_pWeapon->IsZoomed()))
	{
		action = m_fireParams->actions.fire.c_str();
		flags |= CItem::eIPAF_Shoot;
	}
	
	if (m_firstShot)
	{
		m_firstShot = false;
		flags|=CItem::eIPAF_NoBlend;
	}
	
	if (!playerIsShooter && m_fireParams->proceduralRecoilParams.enabled)
	{
		flags|=CItem::eIPAF_ProceduralRecoil; // this tells the Item not to play an animation
		if (pActor)
			pActor->ProceduralRecoil(m_fireParams->proceduralRecoilParams.duration, m_fireParams->proceduralRecoilParams.strength);
	}

	flags = PlayActionSAFlags(flags);
	
	if(action[0])
	{
		m_pWeapon->PlayAction(action, 0, false, flags, -1.0f, GetFireAnimationWeight(), GetFireFFeedbackWeight());
	}

	// debug
  static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
  if (pAimDebug->GetIVal()) 
  {
    IPersistantDebug* pDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
    pDebug->Begin("CSingle::Shoot", false);
    pDebug->AddSphere(hit, 0.05f, ColorF(0,0,1,1), 10.f);
    pDebug->AddDirection(pos, 0.25f, dir, ColorF(0,0,1,1), 1.f);
  }
/*
	DebugShoot shoot;
	shoot.pos=pos;
	shoot.dir=dir;
	shoot.hit=hit;
	g_shoots.push_back(shoot);*/
	
	if (!m_fireParams->fireparams.totally_silent)
	{
		CheckNearMisses(hit, pos, dir, (hit-pos).len(), 1.0f);
	}

	CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, false);
	if (pAmmo)
	{
    CGameRules* pGameRules = g_pGame->GetGameRules();

		float damage = m_fireParams->fireparams.damage;
		if(m_fireParams->fireparams.secondary_damage && !playerIsShooter)
			damage = m_fireParams->fireparams.ai_vs_player_damage;

		pAmmo->SetParams(
			m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), (int)damage, m_fireParams->fireparams.damage_drop_min_distance,
			m_fireParams->fireparams.damage_drop_per_meter, m_fireParams->fireparams.damage_drop_min_damage, pGameRules->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str()),
			m_fireParams->fireparams.bullet_pierceability_modifier);
		// this must be done after owner is set
		pAmmo->InitWithAI();
    
    if (m_bLocked)
      pAmmo->SetDestination(m_lockedTarget);
    else
      pAmmo->SetDestination(m_pWeapon->GetDestination());

    pAmmo->Launch(pos, dir, vel, m_speed_scale);
		pAmmo->SetKnocksTargetInfo( m_fireParams );
   
		pAmmo->SetRemote(isRemote);

		const STracerParams * bestTracerParams = PickBestTracerParams();

		int frequency = bestTracerParams->frequency;

		bool emit = false;
		if (frequency > 0)
		{
			if(m_pWeapon->GetStats().fp)
				emit = (!bestTracerParams->geometryFP.empty() || !bestTracerParams->effectFP.empty()) && ((ammoCount == clipSize) || (ammoCount%frequency == 0));
			else
				emit = (!bestTracerParams->geometry.empty() || !bestTracerParams->effect.empty()) && ((ammoCount == clipSize) || (ammoCount%frequency==0));
		}

		if (emit)
			EmitTracer(pos,hit,bestTracerParams);

		m_projectileId = pAmmo->GetEntity()->GetId();
	}

	if (pAmmo && pAmmo->IsPredicted() && gEnv->bClient && gEnv->bServer && pActor && pActor->IsClient())
	{
		pAmmo->GetGameObject()->BindToNetwork();
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = ammo != NULL ? ammo->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, 1, (void *)m_pWeapon->GetEntityId()));
	}

	MuzzleFlashEffect(true); 
	RejectEffect();
  RecoilImpulse(pos, dir);

	m_fired = true;
	SetNextShotTime(m_next_shot + m_next_shot_dt);

  if (++m_barrelId == m_fireParams->fireparams.barrel_count)
    m_barrelId = 0;
	
	ammoCount--;

	if(m_fireParams->fireparams.fake_fire_rate && playerIsShooter )
	{
		//Hurricane fire rate fake
		ammoCount -= Random(m_fireParams->fireparams.fake_fire_rate);
		if(ammoCount<0)
			ammoCount = 0;
	}

	if (clipSize != -1)
	{
		if (clipSize!=0)
			m_pWeapon->SetAmmoCount(ammo, ammoCount);
		else
			m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
	}

	bool dounzoomcock=(m_pWeapon->IsZoomed() && !OutOfAmmo() && m_fireParams->fireparams.unzoomed_cock);

	if (!m_fireParams->fireparams.slider_layer.empty() && (dounzoomcock || (ammoCount<1)))
	{
		const char *slider_back_layer = m_fireParams->fireparams.slider_layer.c_str();
		m_pWeapon->PlayLayer(slider_back_layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}

	if (dounzoomcock)
	{
		IZoomMode *pIZoomMode = m_pWeapon->GetZoomMode(m_pWeapon->GetCurrentZoomMode());
		if (pIZoomMode && pIZoomMode->IsZoomed())
		{
			CIronSight *pZoomMode=static_cast<CIronSight *>(pIZoomMode);
			pZoomMode->TurnOff(true);

			m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<CockAction>::Create(this), false);
		}
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	if (OutOfAmmo())
	{
		m_pWeapon->OnOutOfAmmo(ammo);
		m_pWeapon->PlayAction(m_fireParams->actions.shot_last_bullet);

		if (m_pWeapon->IsServer() && autoreload && (!pActor || pActor->IsPlayer()))
		{
			m_pWeapon->SetBusy(true);
			int curTime = (int)(m_pWeapon->GetParams().autoReloadDelay * 1000.0f);
			m_pWeapon->GetScheduler()->TimerAction(curTime, CSchedulerAction<ScheduleAutoReload>::Create(m_pWeapon), false);
		}
	}

#ifdef DEBUG_AIMING_AND_SHOOTING
	//---------------------------------------------------------------------------
	// TODO remove when aiming/fire direction is working
	// debugging aiming dir
	if(++DGB_curLimit>DGB_ShotCounter)	DGB_curLimit = DGB_ShotCounter;
	if(++DGB_curIdx>=DGB_ShotCounter)	DGB_curIdx = 0;
	DGB_shots[DGB_curIdx].dst=pos+dir*200.f;
	DGB_shots[DGB_curIdx].src=pos;
	//---------------------------------------------------------------------------
#endif

	//CryLog("RequestShoot - pos(%f,%f,%f), dir(%f,%f,%f), hit(%f,%f,%f)", pos.x, pos.y, pos.z, dir.x, dir.y, dir.z, hit.x, hit.y, hit.z);
	m_pWeapon->RequestShoot(ammo, pos, dir, vel, hit, m_speed_scale, pAmmo? pAmmo->GetGameObject()->GetPredictionHandle() : 0, false);

	return true;
}

//------------------------------------------------------------------------
bool CSingle::ShootFromHelper(const Vec3 &eyepos, const Vec3 &probableHit) const
{
	Vec3 dp(eyepos-probableHit);
	return dp.len2()>(WEAPON_HIT_MIN_DISTANCE*WEAPON_HIT_MIN_DISTANCE);
}

//------------------------------------------------------------------------
bool CSingle::HasFireHelper() const
{ 
  return !m_fireParams->fireparams.helper[m_pWeapon->GetStats().fp?0:1].empty();
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFireHelperPos() const
{
  if (HasFireHelper())
  {
    int id = m_pWeapon->GetStats().fp?0:1;
    int slot = id ? eIGS_ThirdPerson: eIGS_FirstPerson;

    return m_pWeapon->GetSlotHelperPos(slot, m_fireParams->fireparams.helper[id].c_str(), true);
  }

  return Vec3(ZERO);
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFireHelperDir() const
{
  if (HasFireHelper())
  {
    int id = m_pWeapon->GetStats().fp?0:1;
    int slot = id ? eIGS_ThirdPerson : eIGS_FirstPerson;

    return m_pWeapon->GetSlotHelperRotation(slot, m_fireParams->fireparams.helper[id].c_str(), true).GetColumn(1);
  }  

  return FORWARD_DIRECTION;
}

namespace
{
  IPhysicalEntity* GetSkipPhysics(IEntity* pEntity)
  {
    IPhysicalEntity* pPhysics = pEntity->GetPhysics();    
    if (pPhysics && pPhysics->GetType() == PE_LIVING)
    {
      if (ICharacterInstance* pCharacter = pEntity->GetCharacter(0)) 
      {
        if (IPhysicalEntity* pCharPhys = pCharacter->GetISkeletonPose()->GetCharacterPhysics())
          pPhysics = pCharPhys;
      }
    }    
    return pPhysics;
  }
}

//------------------------------------------------------------------------
int CSingle::GetSkipEntities(CWeapon* pWeapon, IPhysicalEntity** pSkipEnts, int nMaxSkip)
{
  int nSkip = 0;
  
  if (CActor *pActor = pWeapon->GetOwnerActor())  
  {
		if (nSkip < nMaxSkip)
    {
			if (IPhysicalEntity* pPhysics = GetSkipPhysics(pActor->GetEntity()))
					pSkipEnts[nSkip++] = pPhysics;
    }
	
		IVehicle* pVehicle = pActor->GetLinkedVehicle();
    if (!pVehicle)
		{
			if (nSkip < nMaxSkip)
      {
        if (IPhysicalEntity* pPhysics = pWeapon->GetEntity()->GetPhysics())
          pSkipEnts[nSkip++] = pPhysics;
      }
		}
		else
    {
      // skip vehicle and all child entities
      IEntity* pVehicleEntity = pVehicle->GetEntity();
      
      if (nSkip < nMaxSkip)
        pSkipEnts[nSkip++] = pVehicleEntity->GetPhysics();

      int count = pVehicleEntity->GetChildCount(); 
      for (int c=0; c<count&&nSkip<nMaxSkip; ++c)
      {
        if (IPhysicalEntity* pPhysics = GetSkipPhysics(pVehicleEntity->GetChild(c)))        
          pSkipEnts[nSkip++] = pPhysics;        
      }
    }
  }  

  return nSkip;
}

//------------------------------------------------------------------------
Vec3 CSingle::GetProbableHit(float maxRayLength, bool *pbHit, ray_hit *pHit) const
{
	static Vec3 pos(ZERO), dir(FORWARD_DIRECTION);

	CActor *pActor = m_pWeapon->GetOwnerActor();
	IEntity *pWeaponEntity = m_pWeapon->GetEntity();

	static IPhysicalEntity* pSkipEntities[10];
	int nSkip = GetSkipEntities(m_pWeapon, pSkipEntities, 10);

	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();      
	if (pLocator)
	{
		Vec3 hit(ZERO);
		if (pLocator->GetProbableHit(m_pWeapon->GetEntityId(), this, hit))
			return hit;
	}

	float rayLength = maxRayLength;
	const SAmmoParams *pAmmoParams = GetAmmoParams();
	if (pAmmoParams)
	{
		//Benito - It could happen that the lifetime/speed are zero, so ensure at least some minimum distance check
		rayLength = clamp(min(pAmmoParams->speed * pAmmoParams->lifetime, rayLength), 5.0f, rayLength);
	}

	bool requiresProbableHitTest = false;

	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	if (pMC)
	{ 
		SMovementState info;
		pMC->GetMovementState(info);

		pos = info.weaponPosition;

		if (!pActor->IsPlayer())
		{
			dir = (info.fireTarget-pos).normalized();
		}
		else
		{
			dir = info.fireDirection;
			requiresProbableHitTest = true; // Only players use probably hit info, AI uses fire target always
		}

		CRY_ASSERT(dir.IsUnit());

		dir = ApplySpread(dir, GetSpread()) * rayLength;
	}
	else
	{ 
		// fallback    
		pos = GetFiringPos(Vec3Constants<float>::fVec3_Zero);
		dir = rayLength * GetFiringDir(Vec3Constants<float>::fVec3_Zero, pos);
	}

	if (requiresProbableHitTest)
	{
		static ray_hit hit;	

		// use the ammo's pierceability
		uint32 flags=(geom_colltype_ray|geom_colltype13)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces;
		uint8 pierceability = 8;
		if (pAmmoParams && pAmmoParams->pParticleParams && !is_unused(pAmmoParams->pParticleParams->iPierceability))
		{
			pierceability=pAmmoParams->pParticleParams->iPierceability;
		}
		flags |= pierceability;

		if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir, ent_all, flags, &hit, 1, pSkipEntities, nSkip))
		{
			if (pbHit)
				*pbHit=true;
			if (pHit)
				*pHit=hit;

			// players in vehicles need to check position isn't behind target (since info.weaponPosition is 
			//	actually the camera pos - argh...)
			if(pbHit && *pbHit && pActor && pActor->GetLinkedVehicle())
			{
				Matrix34 tm = m_pWeapon->GetEntity()->GetWorldTM();
				Vec3 wppos = tm.GetTranslation();

				// find the plane perpendicular to the aim direction that passes through the weapon position
				Vec3 n = dir.GetNormalizedSafe();
				float d = -(n.Dot(wppos));
				Plane	plane(n, d);
				float dist = plane.DistFromPlane(pos);

				if(dist < 0.0f)
				{
					// now do a new intersection test forwards from the point where the previous rwi intersected the plane...
					Vec3 newPos = pos - dist * n;
					if (gEnv->pPhysicalWorld->RayWorldIntersection(newPos, dir, ent_all,
						rwi_stop_at_pierceable|rwi_ignore_back_faces, &hit, 1, pSkipEntities, nSkip))
					{
						*pbHit=true;
						if (pHit)
							*pHit=hit;
					}
				}
			}

			return hit.pt;
		}
	}


	if (pbHit)
		*pbHit=false;

	return pos + dir;
}

//------------------------------------------------------------------------
void CSingle::DeferGetProbableHit(float maxRayLength)
{
	static Vec3 pos(ZERO), dir(FORWARD_DIRECTION);
	CActor *pActor = m_pWeapon->GetOwnerActor();
	IEntity *pWeaponEntity = m_pWeapon->GetEntity();

	static IPhysicalEntity* pSkipEntities[10];
	int nSkip = GetSkipEntities(m_pWeapon, pSkipEntities, 10);

	float rayLength = maxRayLength;
	const SAmmoParams *pAmmoParams = GetAmmoParams();
	if (pAmmoParams)
	{
		rayLength = min(pAmmoParams->speed * pAmmoParams->lifetime, rayLength);
	}

	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	if (pMC)
	{ 
		SMovementState info;
		pMC->GetMovementState(info);

		pos = info.weaponPosition;

		if (!pActor->IsPlayer())
		{      
			dir = (info.fireTarget-pos).normalized();
		}
		else
		{
			dir = info.fireDirection;
			CRY_ASSERT(dir.IsUnit());
		}
		dir = ApplySpread(dir, GetSpread()) * rayLength;
	}
	else
	{ 
		// fallback    
		pos = GetFiringPos(Vec3Constants<float>::fVec3_Zero);
		dir = rayLength * GetFiringDir(Vec3Constants<float>::fVec3_Zero, pos);
	}

	static ray_hit hit;

	uint32 flags=(geom_colltype_ray|geom_colltype13)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces;
	uint8 pierceability=8;
	if (pAmmoParams && pAmmoParams->pParticleParams && !is_unused(pAmmoParams->pParticleParams->iPierceability))
		pierceability=pAmmoParams->pParticleParams->iPierceability;
	flags |= pierceability;

	SProbableHitInfo *probableHit = NULL;
	for(int i=0; i < MAX_PROBABLE_HITS; i++)
	{
		if(m_probableHits[i].m_state == eProbableHitDeferredState_none)
		{
			probableHit = &m_probableHits[i];
			break;
		}
	}

	assert(probableHit);

	if(probableHit)	// maybe too many shots during a client's loading screen?
	{
		probableHit->m_hit = pos + dir;	// default value, if we hit nothing
		probableHit->m_raycastHelper.SetReceiver(probableHit);
		probableHit->m_raycastHelper->CastRay(pos, dir, ent_all, flags, pSkipEntities, nSkip);
		probableHit->m_state = eProbableHitDeferredState_dispatched;

		m_queuedProbableHits.push(probableHit);
	}
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFiringPos(const Vec3 &probableHit) const
{
  static Vec3 pos;
	
  IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  { 
		if (pLocator->GetFiringPos(m_pWeapon->GetEntityId(), this, pos))
      return pos;
  }
	
  int id = m_pWeapon->GetStats().fp ? 0 : 1;
  int slot = id ? eIGS_ThirdPerson : eIGS_FirstPerson;
  
  pos = m_pWeapon->GetEntity()->GetWorldPos();
  
	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	
  if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);

		pos = info.weaponPosition;

		// FIXME
		// should be getting it from MovementCotroller (same for AIProxy::QueryBodyInfo)
		// update: now AI always should be using the fire_pos from movement controller
		if (/*pActor->IsPlayer() && */(HasFireHelper() && ShootFromHelper(pos, probableHit)))
		{
			// FIXME
			// making fire pos be at eye when animation is not updated (otherwise shooting from ground)
			bool	isCharacterVisible(false);
			
			IEntity *pEntity(pActor->GetEntity());
			ICharacterInstance * pCharacter(pEntity->GetCharacter(0));

			if(pCharacter && pCharacter->IsCharacterVisible()!=0)
				isCharacterVisible = true;

			if(isCharacterVisible)
				pos = m_pWeapon->GetSlotHelperPos(slot, m_fireParams->fireparams.helper[id].c_str(), true);
		}
	}
  else
  {
    // when no MC, fall back to helper
    if (HasFireHelper())
    {
      pos = m_pWeapon->GetSlotHelperPos(slot, m_fireParams->fireparams.helper[id].c_str(), true);
    }
  }

	return pos;
}


//------------------------------------------------------------------------
Vec3 CSingle::GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const
{
  static Vec3 dir;

	if (m_fireParams->fireparams.autoaim && m_fireParams->fireparams.autoaim_autofiringdir)
	{
		if (m_bLocked)
		{
			IEntity *pEnt = gEnv->pEntitySystem->GetEntity(m_lockedTarget);
			if (pEnt)
			{ 
				AABB bbox;
				pEnt->GetWorldBounds(bbox);
				Vec3 center = bbox.GetCenter();
				IActor *pAct = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_lockedTarget);
				if (pAct)
				{
					if (IMovementController *pMV = pAct->GetMovementController())
					{
						SMovementState ms;
						pMV->GetMovementState(ms);
						center = ms.eyePosition;
					}
				}				
				dir = (center - firingPos).normalize();
				return dir;
			}
		}
	}
	
	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  { 
    if (pLocator->GetFiringDir(m_pWeapon->GetEntityId(), this, dir, probableHit, firingPos))
      return dir;		
  }

  int id = m_pWeapon->GetStats().fp?0:1;
  int slot = id ? eIGS_ThirdPerson : eIGS_FirstPerson;

  dir = m_pWeapon->GetEntity()->GetWorldTM().GetColumn1();

	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);

		dir = info.fireDirection;

    if (HasFireHelper() && ShootFromHelper(info.weaponPosition, probableHit))
    {
      if (!pActor->IsPlayer())
			{
        dir = (info.fireTarget-firingPos).normalized();
			}
      else if(!probableHit.IsZero())
			{
				dir = (probableHit-firingPos).normalized();
			}
	  }
    else if (!pActor->IsThirdPerson())
		{
			if(!probableHit.IsZero())
			{
				//This direction contains already the spread calculation
				dir = (probableHit-firingPos).normalized();
			}
		}
	}  
  else
  {
    // if no MC, fall back to helper    
    if (HasFireHelper())
    { 
      dir = m_pWeapon->GetSlotHelperRotation(slot, m_fireParams->fireparams.helper[id].c_str(), true).GetColumn(1);
    }
  }
  
	return dir;
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFiringVelocity(const Vec3 &dir) const
{
	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  {
    Vec3 vel;
    if (pLocator->GetFiringVelocity(m_pWeapon->GetEntityId(), this, vel, dir))
      return vel;
  }

	CActor *pActor=m_pWeapon->GetOwnerActor();
	if (pActor)
	{
		IPhysicalEntity *pPE=pActor->GetEntity()->GetPhysics();
		if (pPE)
		{
			pe_status_dynamics sv;
			if (pPE->GetStatus(&sv))
			{
				if (sv.v.len2()>0.01f)
				{
					float dot=sv.v.GetNormalized().Dot(dir);
					if (dot<0.0f)
						dot=0.0f;


					//CryLogAlways("velocity dot: %.3f", dot);

					return sv.v*dot;
				}
			}
		}
	}

	return Vec3(0,0,0);
}

//------------------------------------------------------------------------
Vec3 CSingle::GetOwnerVelocity() const
{
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (pActor)
	{
		SActorStats* pActorStats = pActor->GetActorStats();
		
		return pActorStats ? pActorStats->velocity : ZERO; 
	}

	return ZERO;
}

//------------------------------------------------------------------------
Vec3 CSingle::ApplySpread(const Vec3 &dir, float spread, int quadrant) const
{
	//Claire: Spread can now be quadrant based to allow us to get a more even spread on the shotgun
	//Quad 0 = +x, +z
	//Quad 1 = +x, -z
	//Quad 2 = -x, -z
	//Quad 3 = -x, +z

	Ang3 angles=Ang3::GetAnglesXYZ(Matrix33::CreateRotationVDir(dir));
	float rx = 0.f;
	float rz = 0.f;

	if(quadrant < 0)
	{
		CCCPOINT(Single_ApplySpreadRandom);

		rx = Random() - 0.5f;
		rz = Random() - 0.5f;
	}
	else
	{
		CCCPOINT(Single_ApplySpreadQuadrant);
		CRY_ASSERT_MESSAGE(quadrant < 4, "Invalid quadrant provided to apply spread");

		rx = Random(0.5f);
		rz = Random(0.5f);

		if(quadrant > 1)
		{
			rx *= -1.f; 
		}

		if(quadrant == 1 || quadrant == 2)
		{
			rz *= -1.f;
		}
	}
	
	angles.x+=rx*DEG2RAD(spread);
	angles.z+=rz*DEG2RAD(spread);

	return Matrix33::CreateRotationXYZ(angles).GetColumn(1).normalized();
}

const STracerParams* CSingle::PickBestTracerParams()
{
	CActor* pOwner = m_pWeapon->GetOwnerActor();
	if(pOwner && pOwner->IsPlayer() && (static_cast<CPlayer*>(pOwner))->IsPerkActive(ePerk_ArmourPiercing))
	{
		CCCPOINT(Perk_ArmourPiercing_Tracer);
		return & m_fireParams->armourPiercingPerkTracerParams;
	}

	// By default, use the weapon's normal tracers; this function should never return NULL
	return & m_fireParams->tracerparams;
}

//------------------------------------------------------------------------
Vec3 CSingle::GetTracerPos(const Vec3 &firingPos, const STracerParams* useTracerParams)
{
	int id=m_pWeapon->GetStats().fp?0:1;
	int slot=id ? eIGS_ThirdPerson : eIGS_FirstPerson;
	const char * helper = useTracerParams->helper[id].c_str();

	if (!helper[0])
		return firingPos;

	return m_pWeapon->GetSlotHelperPos(slot, helper, true);
}

//------------------------------------------------------------------------
void CSingle::SetupEmitters(bool attach)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (attach)
	{		
		int id = m_pWeapon->GetStats().fp ? 0 : 1;
    Vec3 offset(ZERO);

    if (m_fireParams->muzzleflash.helper[id].empty())
    { 
      // if no helper specified, try getting pos from firing locator
      IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();            
      
      if (pLocator && pLocator->GetFiringPos(m_pWeapon->GetEntityId(), this, offset))
        offset = m_pWeapon->GetEntity()->GetWorldTM().GetInvertedFast() * offset;
    }
            
		if (!m_fireParams->muzzleflash.effect[0].empty())
		{   
			m_mfIds[m_barrelId].mfId[0] = m_pWeapon->AttachEffect(eIGS_FirstPerson, -1, true, m_fireParams->muzzleflash.effect[0].c_str(), 
        m_fireParams->muzzleflash.helper[0].c_str(), offset, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}
		if (!m_fireParams->muzzleflash.effect[1].empty())
		{
			m_mfIds[m_barrelId].mfId[1] = m_pWeapon->AttachEffect(eIGS_ThirdPerson, -1, true, m_fireParams->muzzleflash.effect[1].c_str(), 
				m_fireParams->muzzleflash.helper[1].c_str(), offset, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}
	}
	else
	{
    for (int i=0; i<m_mfIds.size(); ++i)
    {
      m_mfIds[i].mfId[0] = m_pWeapon->AttachEffect(eIGS_FirstPerson, m_mfIds[i].mfId[0], false);
      m_mfIds[i].mfId[1] = m_pWeapon->AttachEffect(eIGS_ThirdPerson, m_mfIds[i].mfId[1], false);
    }
	}
}

//------------------------------------------------------------------------
void CSingle::MuzzleFlashEffect(bool attach)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  // muzzle effects & lights are permanently attached and emitted on attach==true
  // calling with attach==false removes the emitters
  if (attach)
	{
		FRAME_PROFILER("CSingle::MuzzleFlashEffect() Attach Effect", GetISystem(), PROFILE_AI);

    int slot = m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
    int id = m_pWeapon->GetStats().fp ? 0 : 1;

    if (!m_fireParams->muzzleflash.effect[id].empty() && !m_pWeapon->GetEntity()->IsHidden())
	  {
		  if (m_barrelId < m_mfIds.size())
		  {
				IParticleEmitter *pEmitter = NULL;
				bool spawnEffect = true;

				// In mp only spawn muzzle flashes which are visible
				if(g_pGameCVars->g_muzzleFlashCull)
				{
					EntityId weaponOwnerEntityId = m_pWeapon->GetOwnerId();
					EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();

					if(weaponOwnerEntityId != localClientId) // Always want to spawn muzzle flash for client
					{
						Vec3 cameraPos = gEnv->p3DEngine->GetCurrentCamera().GetPosition();
						Vec3 emitterPos;

						emitterPos = m_pWeapon->GetSlotHelperPos(slot, m_fireParams->muzzleflash.helper[id].c_str(), true);

						float emitterDistFromCameraSq = (emitterPos - cameraPos).GetLengthSquared();

						Sphere emitterSphere(emitterPos, 2.0f);
						if((emitterDistFromCameraSq > g_pGameCVars->g_muzzleFlashCullDistance) ||	// If too far in distance
								(gEnv->p3DEngine->GetCurrentCamera().IsSphereVisible_F(emitterSphere) == false) ) // If not in frustum
						{
							spawnEffect = false;
						}
					}
				}

				if(spawnEffect)
				{
			  		if (!m_mfIds[m_barrelId].mfId[id])
				  		SetupEmitters(true);		

			  		pEmitter = m_pWeapon->GetEffectEmitter(m_mfIds[m_barrelId].mfId[id]);
			  }

			  if (pEmitter)
				  pEmitter->EmitParticle();
		  }
	  }		  
  		
		bool shouldSendAIEvent = (m_fireParams->muzzleflash.aiVisibilityRadius > 0.0f) && (m_pWeapon->GetOwner() && m_pWeapon->GetOwner()->GetAI());
    if (shouldSendAIEvent)
		{
			gEnv->pAISystem->DynOmniLightEvent(m_pWeapon->GetOwner()->GetWorldPos(),
				m_fireParams->muzzleflash.aiVisibilityRadius, AILE_MUZZLE_FLASH, m_pWeapon->GetOwnerId());
		}
	}
  else
  {
		FRAME_PROFILER("CSingle::MuzzleFlashEffect() Detttach Effect", GetISystem(), PROFILE_AI);

	  SetupEmitters(false);
  }
}

void CSingle::SmokeEffect(bool effect)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (effect)
	{		
		Vec3 dir(0,1.0f,0);
		CActor *pActor = m_pWeapon->GetOwnerActor();
		IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
		if (pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);
			dir = info.aimDirection;
		}

		int slot = m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
		int id = m_pWeapon->GetStats().fp ? 0 : 1;

		if(m_smokeEffectId)
		{
			m_smokeEffectId = m_pWeapon->AttachEffect(0,m_smokeEffectId,false);
		}

		if (!m_fireParams->muzzlesmoke.effect[id].empty())
		{		
			m_smokeEffectId = m_pWeapon->AttachEffect(slot, -1, true, m_fireParams->muzzlesmoke.effect[id].c_str(), m_fireParams->muzzlesmoke.helper[id].c_str());
		}
	}
}

//------------------------------------------------------------------------
void CSingle::SpinUpEffect(bool attach)
{ 
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  m_pWeapon->AttachEffect(0, m_spinUpEffectId, false);
	m_spinUpEffectId=0;

	if (attach)
	{
		int slot = m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
		int id = m_pWeapon->GetStats().fp ? 0 : 1;

		if (!m_fireParams->spinup.effect[0].empty() || !m_fireParams->spinup.effect[1].empty())
		{
			m_spinUpEffectId = m_pWeapon->AttachEffect(slot, 0, true, m_fireParams->spinup.effect[id].c_str(), 
        m_fireParams->spinup.helper[id].c_str(), Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}

		m_spinUpTimer = (uint32)(m_fireParams->spinup.time[id]);
	}
  else
  {
    //CryLog("[%s] spinup effect (false)", m_pWeapon->GetEntity()->GetName());
  }
}

//------------------------------------------------------------------------
void CSingle::RejectEffect()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const SEffectParams& rejectEffect = m_fireParams->reject;

	const int slot = m_pWeapon->GetStats().fp ? 0 : 1;
	const bool doRejectEffect = (g_pGameCVars->i_rejecteffects != 0) && (!rejectEffect.effect[slot].empty());

	if (doRejectEffect)
	{
		//First check if we can skip/cull the effect (not for the local player)
		if (m_pWeapon->IsOwnerClient() == false)
		{
			const CCamera& camera = gEnv->p3DEngine->GetCurrentCamera();
			const Vec3 cameraPos = camera.GetPosition();
			const Vec3 effectPosition = m_pWeapon->GetEntity()->GetWorldPos();

			const float fDist2 = (cameraPos-effectPosition).len2();

			// Too far, skip
			if (fDist2 > g_pGameCVars->g_rejectEffectCullDistance)			
			{
				return; 
			}

			// Not in the view ?
			if(g_pGameCVars->g_rejectEffectVisibilityCull)
			{
				Sphere emitterSphere(effectPosition, 2.0f);
				if(camera.IsSphereVisible_F(emitterSphere) == false)
				{
					return;
				}
			}
		}

		const float frameTime = gEnv->pTimer->GetFrameTime();

		const Vec3 shellDirection = m_pWeapon->GetSlotHelperRotation(slot, rejectEffect.helper[slot], true).GetColumn1();
		Vec3 velocity = GetOwnerVelocity();
		float shellSpeed = velocity.NormalizeSafe();
		shellSpeed *= shellDirection.Dot(velocity);
		
		m_pWeapon->SpawnEffect(slot, rejectEffect.effect[slot].c_str(), rejectEffect.helper[slot].c_str(),
		rejectEffect.offset + (velocity * shellSpeed * frameTime), shellDirection, rejectEffect.scale[slot], shellSpeed);

	}
}

//------------------------------------------------------------------------
int CSingle::GetDamage() const
{
	return m_fireParams->fireparams.damage;
}

//------------------------------------------------------------------------
float CSingle::GetHeat() const
{
	return m_heat;
}

//------------------------------------------------------------------------
void CSingle::UpdateHeat(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (CanOverheat())
	{
		CActor *pActor=m_pWeapon->GetOwnerActor();
		IAIObject *pOwnerAI=(pActor && pActor->GetEntity()) ? pActor->GetEntity()->GetAI() : 0;
		bool isOwnerAIControlled = pOwnerAI && pOwnerAI->GetAIType() != AIOBJECT_PLAYER;

		if(isOwnerAIControlled)
			return;

		float oldheat=m_heat;

		if (m_overheat>0.0f)
		{
			m_overheat-=frameTime;
			if (m_overheat<=0.0f)
			{
				m_overheat=0.0f;
				
        m_pWeapon->StopSound(m_heatSoundId);
				m_pWeapon->PlayAction(m_fireParams->actions.cooldown);				
			}
		}
		else
		{
			float add=0.0f;
			float sub=0.0f;

			if (m_fired)
			{
				add=m_fireParams->heatingparams.attack;
				if(pActor && pActor->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power && pActor->GetLinkedVehicle())
				{
					const static float k_VehicleCombatMode_ScaleOverheat = 0.5f;
					add=m_fireParams->heatingparams.attack * k_VehicleCombatMode_ScaleOverheat;	
				}
			}
			else if (m_next_shot<=0.0001f)
			{
				sub=frameTime/m_fireParams->heatingparams.decay;
			}

			m_heat += add-sub;
			m_heat = CLAMP(m_heat, 0.0f, 1.0f);

      static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
      if (pAimDebug && pAimDebug->GetIVal() > 1)
      {
        float color[] = {1,1,1,1};
        gEnv->pRenderer->Draw2dLabel(300, 300, 1.2f, color, false, "    + %.2f", add);
        gEnv->pRenderer->Draw2dLabel(300, 315, 1.2f, color, false, "    - %.2f", sub);
        gEnv->pRenderer->Draw2dLabel(300, 335, 1.3f, color, false, "heat: %.2f", m_heat);
      }

			if (m_heat >= 0.999f && oldheat<0.999f)
			{
				m_overheat=m_fireParams->heatingparams.duration;

				StopFire();
				m_heatSoundId = m_pWeapon->PlayAction(m_fireParams->actions.overheating);

				int slot = m_pWeapon->GetStats().fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
				if (!m_fireParams->heatingparams.effect[slot].empty())
        {
          if (m_heatEffectId)
            m_heatEffectId = m_pWeapon->AttachEffect(0, m_heatEffectId, false);

					m_heatEffectId = m_pWeapon->AttachEffect(slot, 0, true, m_fireParams->heatingparams.effect[slot].c_str(), m_fireParams->heatingparams.helper[slot].c_str());
        }
			}
		}

		if (m_heat>=0.0001f)
			m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
bool CSingle::DoesFireRayIntersectFrustum(const Vec3& vPos, bool& firePosInFrustum)
{
	if(g_pGameCVars->g_mpCullShootProbablyHits)
	{
		// If the firing position is inside the frustum, then the ray definitely intersects with the frustum
		firePosInFrustum = (gEnv->p3DEngine->GetCurrentCamera().IsPointVisible(vPos)==CULL_OVERLAP);
	}
	else
	{
		firePosInFrustum = true; // Set to true so culling is off
	}

	if(firePosInFrustum)
	{
		return true; // If the fire pos is in frustum, then ray definitely intersects frustum
	}
	else
	{
		Vec3 vDir = GetFiringDir(Vec3Constants<float>::fVec3_Zero, vPos); // Get direction without spread applied
		Ray fireRay(vPos,vDir);
		Vec3 collisionPoint;
		const bool isSingleSided = false;
		bool rayIntersectsPlane = false;
		const Plane* frustumPlane = NULL;
		int planeIntersectCount = 0;
		Vec3 intersectedPlanePoints[FRUSTUM_PLANES];

		// Check ray against every frustum plane
		for(int p=0; p<FRUSTUM_PLANES; p++)
		{
			frustumPlane = gEnv->p3DEngine->GetCurrentCamera().GetFrustumPlane(p);
			rayIntersectsPlane = Intersect::Ray_Plane(fireRay,*frustumPlane,collisionPoint,isSingleSided);
			if(rayIntersectsPlane)
			{
				intersectedPlanePoints[planeIntersectCount] = collisionPoint;
				planeIntersectCount++;
			}
		}

		// If there is more than 1 collision then the ray does collide with the frustum, use the middle point between each collision and
		// test if that point is inside the frustum
		bool averagePointInFrustum = false;
		Vec3 averagePoint;
		if(planeIntersectCount > 1)
		{
			for(int p=0; p<planeIntersectCount; p++)
			{
				for(int c=0; c<planeIntersectCount; c++)
				{
					if(c!=p)
					{
						averagePoint = LERP(intersectedPlanePoints[p],intersectedPlanePoints[c],0.5f);
						averagePointInFrustum = (gEnv->p3DEngine->GetCurrentCamera().IsPointVisible(averagePoint) == CULL_OVERLAP);
						if(averagePointInFrustum)
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool CSingle::DoesFireLineSegmentIntersectFrustum(const Vec3& start, const Vec3& end)
{
	Lineseg lineSeg(start,end);
	const Plane* frustumPlane = NULL;
	
	Vec3 collisionPoint;
	const bool isSingleSided = false;

	// Test line segment against each frustum plane
	for(int p=0; p<FRUSTUM_PLANES; p++)
	{
		frustumPlane = gEnv->p3DEngine->GetCurrentCamera().GetFrustumPlane(p);

		if(Intersect::Segment_Plane(lineSeg,*frustumPlane,collisionPoint,isSingleSided))
		{
			return true;
		}
	}
	
	return false;
}
void CSingle::NetShoot(const Vec3 &hit, int predictionHandle)
{
	Vec3 vPos = GetFiringPos(hit);
	Vec3 vHit(hit);
	if(vHit.IsZero())
	{
		bool firePosInFrustum = false;
		bool doGetProbablyHit = DoesFireRayIntersectFrustum(vPos,firePosInFrustum);

		if(doGetProbablyHit)
		{
			IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();      
			bool defer = true;

			if (pLocator && pLocator->GetProbableHit(m_pWeapon->GetEntityId(), this, vHit))
			{
				vPos = GetFiringPos(vHit); // recalc pos now we have an actual hit
				defer = false;
			}

			if(defer)
			{
				DeferGetProbableHit(WEAPON_HIT_RANGE);
				m_pWeapon->RequireUpdate(eIUS_FireMode);
				return;
			}
		}
	}
	
	Vec3 vDir = GetFiringDir(vHit, vPos);
	Vec3 vVel = GetFiringVelocity(vDir);

	NetShootEx(vPos, vDir, vVel, vHit, 1.0f, predictionHandle);
}

void CSingle::NetShootDeferred(const Vec3 &inHit)
{
	Vec3 vHit(inHit);
	Vec3 vPos = GetFiringPos(vHit);
	Vec3 vDir = GetFiringDir(vHit, vPos);		//Spread was already applied if required in DeferGetProbableHit( )
	Vec3 vVel = GetFiringVelocity(vDir);

	// if not visible and not intersecting the fustrum
	if(!(gEnv->p3DEngine->GetCurrentCamera().IsPointVisible(vPos) == CULL_OVERLAP) && DoesFireLineSegmentIntersectFrustum(vPos,vHit) == false)
	{
		vHit.zero(); // Line segment is not in frustum, so zero out the hit so no effects are spawned for it
	}

	NetShootEx(vPos, vDir, vVel, vHit, 1.0f, 0);
}

//------------------------------------------------------------------------
void CSingle::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle)
{
	CCCPOINT(Single_NetShoot);

	IEntityClass* ammo		= m_fireParams->fireparams.ammo_type_class;
	CActor* pActor				= m_pWeapon->GetOwnerActor();
	int clipSize					= GetClipSize();
	int ammoCount					= (clipSize == 0) ? m_pWeapon->GetInventoryAmmoCount(ammo) : m_pWeapon->GetAmmoCount(ammo);
	bool playerIsShooter	= pActor ? pActor->IsPlayer() : false;
	const char *action		= (ammoCount == 1) ? m_fireParams->actions.fire.c_str() : m_fireParams->actions.fire_cock.c_str();

	m_pWeapon->ResetAllAnimations();

	int flags = CItem::eIPAF_Default|CItem::eIPAF_NoBlend|CItem::eIPAF_RestartAnimation;
	flags = PlayActionSAFlags(flags);
	m_pWeapon->PlayAction(action, 0, false, flags);

	bool isVisibleToClient = (hit.IsZero() == false);
	
	CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, true);
	if (pAmmo)
	{
    int hitTypeId = g_pGame->GetGameRules()->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str());			
		pAmmo->SetParams(
			m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), m_fireParams->fireparams.damage,
			m_fireParams->fireparams.damage_drop_min_distance, m_fireParams->fireparams.damage_drop_per_meter, m_fireParams->fireparams.damage_drop_min_damage,
			hitTypeId, m_fireParams->fireparams.bullet_pierceability_modifier);
		
		if (m_bLocked)
			pAmmo->SetDestination(m_lockedTarget);
		else
			pAmmo->SetDestination(m_pWeapon->GetDestination());

		pAmmo->SetRemote(true);

		m_speed_scale=extra;
		pAmmo->Launch(pos, dir, vel, m_speed_scale);

		const STracerParams * bestTracerParams = PickBestTracerParams();
		bool emit = (bestTracerParams->frequency > 0) ? (!bestTracerParams->geometry.empty() || !bestTracerParams->effect.empty()) && (ammoCount == clipSize || ((ammoCount % bestTracerParams->frequency) == 0)) : false;

		if ((emit) && (isVisibleToClient))
			EmitTracer(pos,hit,bestTracerParams);

		m_projectileId = pAmmo->GetEntity()->GetId();
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = ammo != NULL ? ammo->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, 1, (void *)m_pWeapon->GetEntityId()));
	}
	if(isVisibleToClient)
	{
		MuzzleFlashEffect(true);
		RejectEffect();
	}

  RecoilImpulse(pos, dir);

	m_fired = true;
	m_next_shot = 0.0f;

  if (++m_barrelId == m_fireParams->fireparams.barrel_count)
    m_barrelId = 0;

	ammoCount = max(ammoCount-1, 0);

	if(m_fireParams->fireparams.fake_fire_rate && playerIsShooter )
	{
		//Hurricane fire rate fake
		ammoCount -= Random(m_fireParams->fireparams.fake_fire_rate);
	}
	if (m_pWeapon->IsServer())
	{
		if (clipSize != -1)
		{
			if (clipSize != 0)
				m_pWeapon->SetAmmoCount(ammo, ammoCount);
			else
				m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
		}
	}

	if ((ammoCount<1) && !m_fireParams->fireparams.slider_layer.empty())
	{
		const char *slider_back_layer = m_fireParams->fireparams.slider_layer.c_str();
		m_pWeapon->PlayLayer(slider_back_layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	if (OutOfAmmo())
		m_pWeapon->OnOutOfAmmo(ammo);

	if (pAmmo && predictionHandle && pActor)
	{
		pAmmo->GetGameObject()->RegisterAsValidated(pActor->GetGameObject(), predictionHandle);
		pAmmo->GetGameObject()->BindToNetwork();
	}

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CSingle::ReplayShoot()
{
	CCCPOINT(Single_ReplayShoot);

	Vec3 hit = ZERO;
	Vec3 pos = GetFiringPos(hit);

	bool firePosInFrustum = false;
	bool isVisibleToClient = DoesFireRayIntersectFrustum(pos, firePosInFrustum);

	if(isVisibleToClient)
	{
		bool bHit = false;
		ray_hit rayhit;	 
		rayhit.pCollider = 0;

		hit = GetProbableHit(WEAPON_HIT_RANGE, &bHit, &rayhit);
		pos = GetFiringPos(hit); // Recalc pos now we have a hit position

		if(firePosInFrustum == false) // If inside frustum, then definitely intersects so ignore
		{
			if(DoesFireLineSegmentIntersectFrustum(pos, hit) == false)
			{
				isVisibleToClient = false;
			}
		}
	}

	Vec3 dir = GetFiringDir(hit, pos);
	Vec3 vel = GetFiringVelocity(dir);

	NetShootEx(pos, dir, vel, hit, 1, 0);
}

//------------------------------------------------------------------------
void CSingle::RecoilImpulse(const Vec3& firingPos, const Vec3& firingDir)
{
//	m_recoil.RecoilImpulse(firingPos, firingDir);
}

//------------------------------------------------------------------------
void CSingle::CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	IAISystem* pAISystem = gEnv->pAISystem;
	EntityId ownerId = m_pWeapon->GetOwnerId();
	if (pAISystem && (ownerId != 0))
	{
		// Associate event with vehicle if the shooter is in a vehicle (tank cannon shot, etc)
		CActor* pActor = m_pWeapon->GetOwnerActor();
		IVehicle* pVehicle = pActor ? pActor->GetLinkedVehicle() : NULL;
		if (pVehicle)
		{
			ownerId = pVehicle->GetEntityId();
		}

		SAIStimulus stim(AISTIM_BULLET_WHIZZ, 0, ownerId, 0, pos, dir, range);
		pAISystem->RegisterStimulus(stim);
	}
}

//------------------------------------------------------------------------
void CSingle::CacheTracerForParams(const STracerParams & params)
{
	if (!params.geometry.empty())
	{
		IStatObj *pStatObj = gEnv->p3DEngine->LoadStatObj(params.geometry.c_str());
		if (pStatObj)
		{
			pStatObj->AddRef();
			m_tracerCache.push_back(pStatObj);
		}
	}
	if (!params.geometryFP.empty() && (params.geometryFP!=params.geometry))
	{
		IStatObj *pStatObjFP = gEnv->p3DEngine->LoadStatObj(params.geometryFP.c_str());
		if (pStatObjFP)
		{
			pStatObjFP->AddRef();
			m_tracerCache.push_back(pStatObjFP);
		}
	}
}

//------------------------------------------------------------------------
void CSingle::CacheTracer()
{
	CacheTracerForParams(m_fireParams->tracerparams);
	CacheTracerForParams(m_fireParams->armourPiercingPerkTracerParams);
}

//-----------------------------------------------------------------------
void CSingle::CacheAmmoGeometry()
{
	const SAmmoParams *pAmmoParams = GetAmmoParams();
	if (pAmmoParams)
	{
		pAmmoParams->CacheGeometry();
	}
}

//------------------------------------------------------------------------
void CSingle::ClearTracerCache()
{
	for (std::vector<IStatObj *>::iterator it=m_tracerCache.begin(); it!=m_tracerCache.end(); ++it)
		(*it)->Release();

	m_tracerCache.resize(0);
}
//------------------------------------------------------------------------
void CSingle::SetName(const char *name)
{
	m_name = name;
}
//------------------------------------------------------------------------
float CSingle::GetProjectileFiringAngle(float v, float g, float x, float y)
{
	float angle=0.0,t,a;

	// Avoid square root in script
	float d = cry_sqrtf(max(0.0f,powf(v,4)-2*g*y*powf(v,2)-powf(g,2)*powf(x,2)));
	if(d>=0)
	{
		a=powf(v,2)-g*y;
		if (a-d>0) {
			t=cry_sqrtf(2*(a-d)/powf(g,2));
			angle = (float)acos_tpl(x/(v*t));	
			float y_test;
			y_test=float(-v*sin_tpl(angle)*t-g*powf(t,2)/2);
			if (fabsf(y-y_test)<0.02f)
				return RAD2DEG(-angle);
			y_test=float(v*sin_tpl(angle)*t-g*pow(t,2)/2);
			if (fabsf(y-y_test)<0.02f)
				return RAD2DEG(angle);
		}
		t = cry_sqrtf(2*(a+d)/powf(g,2));
		angle = (float)acos_tpl(x/(v*t));	
		float y_test=float(v*sin_tpl(angle)*t-g*pow(t,2)/2);

		if (fabsf(y-y_test)<0.02f)
			return RAD2DEG(angle);

		return 0;
	}
	return 0;
}
//------------------------------------------------------------------------
void CSingle::Cancel()
{
	if(m_targetSpotSelected)
	{
		m_pWeapon->ActivateTarget(false);
		m_pWeapon->OnStopTargetting(m_pWeapon);
		m_pWeapon->GetGameObject()->DisablePostUpdates(m_pWeapon);
	}
	m_targetSpotSelected = false;
}
//------------------------------------------------------------------------
bool CSingle::AllowZoom() const
{
	return !m_targetSpotSelected && !m_cocking;
}
//------------------------------------------------------------------------

void CSingle::GetMemoryUsage(ICrySizer * s) const
{	
	s->AddObject(this, sizeof(*this));
	GetInternalMemoryUsage(s);
}

void CSingle::GetInternalMemoryUsage(ICrySizer * s) const
{
	s->AddObject(m_name);
	s->AddObject(m_recoil);	
//	s->AddObject(m_tracerCache); //Diesel cut
	s->AddObject(m_mfIds);	
	//s->AddObject(m_queuedProbableHits);			
	CFireMode::GetInternalMemoryUsage(s); // collect memory of parent class
}


//-------------------------------------------------------------------------------


void CSingle::PatchSpreadMod(const SSpreadModParams &sSMP)
{
	m_recoil.PatchSpreadMod(sSMP);
}


void CSingle::ResetSpreadMod()
{
	m_recoil.ResetSpreadMod(m_fireParams->spreadparamsCopy);
}


void CSingle::PatchRecoilMod(const SRecoilModParams &sRMP)
{
	m_recoil.PatchRecoilMod(sRMP);
}


void CSingle::ResetRecoilMod()
{
	m_recoil.ResetRecoilMod(m_fireParams->recoilparamsCopy);

}

//-----------------------------------------------------------------------------
EntityId CSingle::RemoveProjectileId()
{
	EntityId ret = m_projectileId;
	m_projectileId = 0;
	return ret;
}

//--------------------------------------------------------------
void CSingle::EmitTracer(const Vec3& pos, const Vec3& destination, const STracerParams * useTracerParams)
{
	CCCPOINT(Single_EmitTracer);

	CTracerManager::STracerParams params;
	params.position = GetTracerPos(pos, useTracerParams);
	params.destination = destination;

	if(m_pWeapon->GetStats().fp)
	{
		Vec3 dir = (destination-params.position);
		float lenght = dir.NormalizeSafe();
		if(lenght<(g_pGameCVars->tracer_min_distance*0.5f))
			return;
		params.position += (dir*g_pGameCVars->tracer_min_distance*0.5f);
	}

	if(m_pWeapon->GetStats().fp)
	{
		params.effect = useTracerParams->effectFP.c_str();
		params.geometry = useTracerParams->geometryFP.c_str();
		params.speed = useTracerParams->speedFP;
	}
	else
	{
		params.effect = useTracerParams->effect.c_str();
		params.geometry = useTracerParams->geometry.c_str();
		params.speed = useTracerParams->speed;
	}

	
	params.lifetime = useTracerParams->lifetime;
	params.delayBeforeDestroy = useTracerParams->delayBeforeDestroy;

	g_pGame->GetWeaponSystem()->GetTracerManager().EmitTracer(params);
}

//------------------------------------------------
void CSingle::RestoreOverHeating(bool activate)
{
	if(activate)
	{
		if(m_nextHeatTime>0.0f)
		{
			CTimeValue time = gEnv->pTimer->GetFrameStartTime();
			float dt = m_nextHeatTime - time.GetSeconds();
			if(dt > 0.0f)
				m_heat = max(dt,1.0f);
			if(dt > 1.0f)
				m_overheat = dt - 1.0f;
			m_nextHeatTime = 0.0f;
		}
	}
	else
	{
		m_nextHeatTime = 0.0f;
		if(m_heat>0.0f)
		{
			CTimeValue time = gEnv->pTimer->GetFrameStartTime();
			m_nextHeatTime = time.GetSeconds() + m_heat + m_overheat;
		}
	}
}

//----------------------------------------------------
bool CSingle::CanOverheat() const
{
	return m_fireParams->heatingparams.attack > 0.0f;
}

//----------------------------------------------------
void CSingle::OnZoomStateChanged()
{

}

//----------------------------------------------------
const char *CSingle::GetSuffix() const 
{ 
	return m_fireParams->fireparams.suffix.empty() ? NULL : m_fireParams->fireparams.suffix.c_str(); 
};

//----------------------------------------------------
const char *CSingle::GetSuffixAG() const 
{ 
	return m_fireParams->fireparams.suffixAG.empty() ? NULL : m_fireParams->fireparams.suffixAG.c_str(); 
};

//----------------------------------------------------
bool CSingle::DampRecoilEffects() const
{
	CActor* pActor = m_pWeapon->GetOwnerActor();
	if (pActor)
	{
		bool powerModeActive = pActor->GetActorSuitGameParameters().GetMode()==eNanoSuitMode_Power && pActor->GetActorSuitGameParameters().IsSuitPowerActive();
		return powerModeActive;
	}
	return false;
}

//----------------------------------------------------
void CSingle::UpdateFireAnimationWeight(float frameTime)
{
	const SFireModeParams* pShared = GetShared();
	float desireAnimationWeight = 1.0f;

	if (!DampRecoilEffects())
	{
		desireAnimationWeight = (m_pWeapon->IsZoomed() || m_pWeapon->IsZoomingIn()) ? pShared->fireparams.ironsight_fire_anim_damp : 1.f;
	}
	else
	{
		desireAnimationWeight = pShared->fireparams.powermode_fire_anim_damp;
	}

	Interpolate(m_fireAnimationWeight, desireAnimationWeight, 4.0f, frameTime);
}

//----------------------------------------------------
float CSingle::GetFireAnimationWeight() const
{
	return m_fireAnimationWeight;
}

//----------------------------------------------------
float CSingle::GetFireFFeedbackWeight() const
{
	if (!DampRecoilEffects())
		return 1.0f;
	return GetShared()->fireparams.powermode_ffeedback_damp;
}

//----------------------------------------------------
const char* CSingle::GetChangeFiremodeAction(const char* newFiremode)
{
	SFiremodeActions::TFiremodeActionMap::const_iterator it = m_fireParams->actions.changeFiremodeActions.find(newFiremode);

	if(it != m_fireParams->actions.changeFiremodeActions.end())
	{
		return (*it).second;
	}

	return NULL;
}
