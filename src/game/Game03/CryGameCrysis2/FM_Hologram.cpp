/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:03:2009   09:50 : Created by Tom Berry
- 23:10:2009	 12:00 : Refactored by Claire Allan

TODO: 
- Needs the network side looking at, ideally the instruction to create the hologram & the start & destination pt could be transmitted once on a fire & forget basis
- Needs integration into the weapon looking at, fire rate should be inherited from the base weapon
- The hologram needs to clone the firing player correctly
- The hologram shouldn't leave footprints, disturb water etc

*************************************************************************/
#include "StdAfx.h"
#include "FM_Hologram.h"
#include "Weapon.h"
#include "Actor.h"
#include "Game.h"
#include "GameRules.h"
#include <ISound.h>
#include <IEntitySystem.h>

#include "Player.h"
#include "PlayerHologram.h"

//------------------------------------------------------------------------
CHologram::CHologram()
:	m_lastHoloID(0)
{
	m_attachmentLoop.SetSignal("HologramAttachmentLoop");
}

//------------------------------------------------------------------------
CHologram::~CHologram()
{
	m_attachmentLoop.Stop(m_pWeapon->GetEntityId());
}

void CHologram::CalculateTargetPos(Vec3& targetPos)
{
	const float targetRange = 100.f;
	const float aimUpWalkDist = 3.f;
	static const float aimUpDP = cosf(DEG2RAD(45.f));

	static Vec3 pos,dir; 
	CActor *pActor = m_pWeapon->GetOwnerActor();

	static IPhysicalEntity* pSkipEntities[10];
	int nSkip = GetSkipEntities(m_pWeapon, pSkipEntities, 10);

	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	
	assert(pMC);

	if (pMC)
	{ 
		SMovementState info;
		pMC->GetMovementState(info);

		assert(info.fireDirection.IsUnit());

		pos = info.weaponPosition;
		dir = info.fireDirection * targetRange;     

		if(Vec3(0.0f, 0.0f, 1.0f).Dot(info.fireDirection) > aimUpDP)
		{
			dir.z = 0.f;	
			targetPos = (dir.GetNormalized() * aimUpWalkDist) + pos;
		}
		else
		{
			static ray_hit hit;	

			// use the ammo's pierceability
			uint32 flags=(geom_colltype_ray|geom_colltype13)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces|8;

			if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir, ent_all, flags, &hit, 1, pSkipEntities, nSkip))
			{
				targetPos = hit.pt;
			}
			else
			{
				targetPos = pos + dir;
			}
		}
	}
}

//------------------------------------------------------------------------
void CHologram::Activate(bool activate)
{
	//gpoints.resize(0);
	CSingle::Activate(activate);

	m_firing = false;

	if(gEnv->bServer)
	{
		IEntityClass* ammo = GetShared()->fireparams.ammo_type_class;

		int ammoCount = m_pWeapon->GetAmmoCount(ammo);

		if(ammoCount <= GetShared()->fireparams.clip_size)
		{
			int inventoryCount = m_pWeapon->GetInventoryAmmoCount(ammo);

			int numAdd = min(GetShared()->fireparams.clip_size - ammoCount, inventoryCount);

			inventoryCount -= numAdd;
			ammoCount += numAdd;
			m_pWeapon->SetInventoryAmmoCount(ammo, inventoryCount);
		}

		m_pWeapon->SetAmmoCount(ammo, ammoCount);
	}

	if(activate)
	{
		if(!OutOfAmmo())
			m_attachmentLoop.Play(m_pWeapon->GetEntityId());
	}
	else
	{
		m_attachmentLoop.Stop(m_pWeapon->GetEntityId());
	}
}

//------------------------------------------------------------------------
bool CHologram::CanFire(bool considerAmmo) const
{
	return !m_pWeapon->IsBusy() && !(m_next_shot > 0.f) && !OutOfAmmo();
}

//------------------------------------------------------------------------
void CHologram::StartFire()
{
	if (!CanFire(true))
		return;

	Vec3 targetPos(0.f,0.f,0.f);
	CalculateTargetPos(targetPos);

	Vec3 scale(1.0f, 1.0f, 1.0f);

	CActor * pOwner = m_pWeapon->GetOwnerActor();

	if(pOwner)
	{
		Vec3 ownerPos = pOwner->GetEntity()->GetPos();
		Vec3 spawnDir = (targetPos - ownerPos);
		spawnDir.z = 0.0f;
		spawnDir.Normalize();
		Vec3 spawnPos = ownerPos + (spawnDir * g_pGameCVars->g_hologram_spawnOffsetDist);

		if (gEnv->bServer)
		{
			LaunchHologram(spawnPos, spawnDir, targetPos);
		}

		SetNextShotTime(m_next_shot_dt);

		SNanoSuitEvent event;
		event.event = eNanoSuitEvent_SHOT;
		pOwner->SendActorSuitEvent(event);
		
		m_pWeapon->PlayAction(GetShared()->actions.fire,0,false,CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation);

		m_pWeapon->RequestShoot(NULL, spawnPos, spawnDir, spawnDir, targetPos, m_speed_scale, 0, true);
	}			

	m_firing = true;
	m_fired = true;		

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CHologram::StopFire()
{
	m_firing = false;
	m_fired  = false;

	m_pWeapon->EnableUpdate(false, eIUS_FireMode);

	if(OutOfAmmo())
		m_attachmentLoop.Stop(m_pWeapon->GetEntityId());
}

//------------------------------------------------------------------------
void CHologram::NetStartFire()
{
}

//------------------------------------------------------------------------
void CHologram::NetStopFire()
{
}

//------------------------------------------------------------------------
bool CHologram::Shoot(bool resetAnimation, bool autoreload, bool isRemote ) 
{ 
	return true; 
}

void CHologram::NetShoot(const Vec3 &hit, int predictionHandle)
{
	m_pWeapon->PlayAction(GetShared()->actions.fire);
}

void CHologram::LaunchHologram(const Vec3 &pos, const Vec3 &dir, const Vec3 &targetPos)
{
	assert(gEnv->bServer);

	if(gEnv->bServer)
	{
		IEntityClass* ammo = GetShared()->fireparams.ammo_type_class;

		int ammoCount = m_pWeapon->GetAmmoCount(ammo);
		
		if(--ammoCount <= 0)
		{
			int inventoryCount = m_pWeapon->GetInventoryAmmoCount(ammo);

			ammoCount = min(GetShared()->fireparams.clip_size, inventoryCount);

			inventoryCount -= ammoCount;
			m_pWeapon->SetInventoryAmmoCount(ammo, inventoryCount);
		}

		m_pWeapon->SetAmmoCount(ammo, ammoCount);
	}
	
	CActor *pActor = m_pWeapon->GetOwnerActor();

	if (m_lastHoloID)
	{
		//--- Destroy the existing hologram!
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity( m_lastHoloID );
		IScriptTable* pScriptTable = pEntity ? pEntity->GetScriptTable() : 0;

		if (pScriptTable)
		{
			HSCRIPTFUNCTION scriptEvent(NULL);	
			pScriptTable->GetValue("HologramTerm", scriptEvent);

			if (scriptEvent)
				Script::Call(gEnv->pScriptSystem,scriptEvent,pScriptTable);

			gEnv->pScriptSystem->ReleaseFunc(scriptEvent);
		}

		m_lastHoloID = 0;
	}

	Vec3 scale(1.0f, 1.0f, 1.0f);
	Vec3 spawnDir = dir;
	spawnDir.z = 0.0f;
	spawnDir.Normalize();
	Quat targetRot(Matrix33::CreateRotationVDir(spawnDir));
	CPlayerHologram* pHologram = (CPlayerHologram*)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActor( 0, "Hologram", "Hologram", pos, targetRot, scale );
	assert(pHologram);
	pHologram->SetTarget(targetPos);
	g_pGame->GetGameRules()->SetTeam(g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId()), pHologram->GetEntityId());

	IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
	IItemSystem		*pItemSystem = pGameFramework->GetIItemSystem();

	const char *itemName = "SCAR";
	//Check item name before giving (it will resolve case sensitive 'issues')
	itemName = (const char*)(pItemSystem->Query(eISQ_Find_Item_By_Name, itemName));

	pItemSystem->GiveItem(pHologram, itemName, true, true, true);

	m_lastHoloID = pHologram->GetEntity()->GetId();
}

void CHologram::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle)
{
	if (gEnv->bServer)
	{
		m_pWeapon->PlayAction(GetShared()->actions.fire);
		LaunchHologram(pos, dir, hit);
	}
}

void CHologram::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));	
	CSingle::GetInternalMemoryUsage(s);			// collect memory of parent class
}
