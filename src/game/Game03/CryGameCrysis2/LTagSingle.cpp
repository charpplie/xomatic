/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Description: 

Ltag version of single firemode. Requires some extra functionality
for animation, projectile spawning, etc

-------------------------------------------------------------------------
History:
- 15:09:09   Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "LTagSingle.h"

#include "Actor.h"
#include "WeaponSharedParams.h"
#include "GameRules.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct CLTagSingle::EndCockingAction
{
public:
	EndCockingAction(CLTagSingle *_pLtagSingle): m_pLTagSingle(_pLtagSingle) {};

	void execute(CItem *item) 
	{
		m_pLTagSingle->m_cocking = false;
	}

private:
	CLTagSingle *m_pLTagSingle;
};


struct CLTagSingle::ScheduleReload
{
public:
	ScheduleReload(CWeapon *pWeapon)
	{
		m_pWeapon = pWeapon;
	}
	void execute(CItem *item) 
	{
		m_pWeapon->SetBusy(false);
		m_pWeapon->Reload();
	}
private:
	CWeapon* m_pWeapon;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CLTagSingle::CLTagSingle()
: m_grenadeType((ELTAGGrenadeType)0)
, m_fpModelInitialised(false)
{

}

CLTagSingle::~CLTagSingle()
{

}

void CLTagSingle::Activate(bool activate)
{
	inherited::Activate(activate);

	//Ensure correct grenade type is selected and the attachments are updated first and third person
	if(activate)
	{
		if(m_fireParams->lTagGrenades.grenades[m_grenadeType].c_str()[0] == '\0')
		{
			NextGrenadeType();
		}
		else
		{
			const ItemString& grenadeModel = m_fireParams->lTagGrenades.grenades[m_grenadeType];
			if (!grenadeModel.empty())
			{
				if (ICharacterInstance* pCharacterTP = m_pWeapon->GetEntity()->GetCharacter(eIGS_ThirdPerson))
				{
					UpdateGrenadeAttachment(pCharacterTP, "currentshell", grenadeModel.c_str());
					UpdateGrenadeAttachment(pCharacterTP, "newshell", grenadeModel.c_str());
				}
			}
		}
	}
	else
	{
		//the first person model is not necessarily available at this point so defer to updateFPView
		m_fpModelInitialised = false;
	}
}

void CLTagSingle::UpdateFPView(float frameTime)
{
	inherited::UpdateFPView(frameTime);

	if(!m_fpModelInitialised)
	{
		const ItemString& grenadeModel = m_fireParams->lTagGrenades.grenades[m_grenadeType];
		if (!grenadeModel.empty())
		{
			if (ICharacterInstance* pCharacterFP = m_pWeapon->GetEntity()->GetCharacter(eIGS_FirstPerson))
			{
				UpdateGrenadeAttachment(pCharacterFP, "currentshell", grenadeModel.c_str());
				UpdateGrenadeAttachment(pCharacterFP, "newshell", grenadeModel.c_str());
				m_fpModelInitialised = true;
			}
		}
	}
}

bool CLTagSingle::Shoot( bool resetAnimation, bool autoreload/*=true*/, bool isRemote )
{
	IEntityClass* pAmmoClass = m_fireParams->fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(pAmmoClass);

	CActor *pActor = m_pWeapon->GetOwnerActor();

	bool playerIsShooter = pActor ? pActor->IsPlayer() : false;
	bool clientIsShooter = pActor ? pActor->IsClient() : false;

	if (!CanFire(true))
	{
		if ((ammoCount <= 0) && !m_reloading && !m_reloadPending)
		{
			m_pWeapon->PlayAction(m_fireParams->actions.empty_clip);
			m_pWeapon->Reload();			
		}

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
	Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(dir);

	const char* weaponAction = GetFireAction(ammoCount);
	const char *action = (ammoCount == 1) || (m_pWeapon->IsZoomed())? m_fireParams->actions.fire.c_str() : m_fireParams->actions.fire_cock.c_str();

	int flags = CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_CleanBlending|CItem::eIPAF_Shoot;
	flags = PlayActionSAFlags(flags);

	m_pWeapon->PlayAction(action, 0, false, flags);
	m_pWeapon->PlayAction(weaponAction, 0, false, flags);

	//Check for fire+cocking anim
	uint32 time = m_pWeapon->GetCurrentAnimationTime(eIGS_FirstPerson);
	m_cocking = true;
	m_pWeapon->GetScheduler()->TimerAction(time-100, CSchedulerAction<EndCockingAction>::Create(this), false);

	CheckNearMisses(hit, pos, dir, (hit-pos).len(), 1.0f);

	assert(pAmmoClass == CItem::sLTagGrenade);
	CProjectile* pAmmo = m_pWeapon->SpawnAmmo(pAmmoClass, false);
	CLTAGGrenade* pGrenade = static_cast<CLTAGGrenade*>(pAmmo);

	if (pGrenade)
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();
	
		pGrenade->SetParams(m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), 
			m_fireParams->fireparams.damage, m_fireParams->fireparams.damage_drop_min_distance, m_fireParams->fireparams.damage_drop_per_meter, m_fireParams->fireparams.damage_drop_min_damage,
			pGameRules->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str()), m_fireParams->fireparams.bullet_pierceability_modifier);
		// this must be done after owner is set
		pGrenade->InitWithAI();
		pGrenade->SetDestination(m_pWeapon->GetDestination());
		pGrenade->SetGrenadeType(m_grenadeType);
		pGrenade->Launch(pos, dir, vel, m_speed_scale);

		m_projectileId = pGrenade->GetEntity()->GetId();
	}

	if (pAmmo && pAmmo->IsPredicted() && gEnv->bClient && gEnv->bServer && pActor && pActor->IsClient())
	{
		pAmmo->GetGameObject()->BindToNetwork();
	}

	if (m_pWeapon->IsServer())
	{
		const char *ammoName = pAmmoClass != NULL ? pAmmoClass->GetName() : NULL;
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, ammoName, 1, (void *)m_pWeapon->GetEntityId()));
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, pAmmoClass, pos, dir, vel);

	MuzzleFlashEffect(true); 
	RejectEffect();
	RecoilImpulse(pos, dir);

	m_fired = true;
	m_next_shot += m_next_shot_dt;

	if (++m_barrelId == m_fireParams->fireparams.barrel_count)
		m_barrelId = 0;

	ammoCount--;

	int clipSize = GetClipSize();
	if (clipSize != -1)
	{
		if (clipSize!=0)
			m_pWeapon->SetAmmoCount(pAmmoClass, ammoCount);
		else
			m_pWeapon->SetInventoryAmmoCount(pAmmoClass, ammoCount);
	}

	if (OutOfAmmo())
	{
		m_pWeapon->OnOutOfAmmo(pAmmoClass);

		if (autoreload && (!pActor || pActor->IsPlayer()))
		{
			m_pWeapon->SetBusy(true);
			m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<ScheduleReload>::Create(m_pWeapon), false);
		}
	}

	m_pWeapon->RequestShoot(pAmmoClass, pos, dir, vel, hit, m_speed_scale, pAmmo? pAmmo->GetGameObject()->GetPredictionHandle() : 0, false);

	m_firePending = false;

	return true;
}


const char* CLTagSingle::GetFireAction( int ammoCount ) const
{
	switch(ammoCount)
	{
		case 1:
			return m_fireParams->actions.fire_last0.c_str();

		case 2:
			return m_fireParams->actions.fire_last1.c_str();
	
		case 3:
			return m_fireParams->actions.fire_last2.c_str();
	}

	return m_fireParams->actions.fire_normal.c_str();
}


void CLTagSingle::NextGrenadeType()
{
	int grenadeType = (ELTAGGrenadeType)((m_grenadeType + 1) % ELTAGGrenadeType_LAST);

	while(grenadeType != m_grenadeType)
	{
		if(m_fireParams->lTagGrenades.grenades[grenadeType].c_str()[0] != '\0')
		{
			m_grenadeType = (ELTAGGrenadeType)grenadeType;
			m_pWeapon->PlayAction(g_pItemStrings->change_firemode);
			SwitchGrenades();
		
			//m_pWeapon->GetGameObject()->ChangedNetworkState(eEA_GameClientStatic);

			return;
		}

		if (++grenadeType >= ELTAGGrenadeType_LAST)
		{
			grenadeType = (ELTAGGrenadeType)0;
		}
	}
}

const char * CLTagSingle::GetType() const
{
	return "LTagSingle";
}

void CLTagSingle::SwitchGrenades()
{
	CActor* pOwner = m_pWeapon->GetOwnerActor();
	if (pOwner == NULL)
		return;

	const ItemString& grenadeModel = m_fireParams->lTagGrenades.grenades[m_grenadeType];
	if (grenadeModel.empty())
		return;

	if (ICharacterInstance* pCharacterTP = m_pWeapon->GetEntity()->GetCharacter(eIGS_ThirdPerson))
	{
		UpdateGrenadeAttachment(pCharacterTP, "currentshell", grenadeModel.c_str());
		UpdateGrenadeAttachment(pCharacterTP, "newshell", grenadeModel.c_str());
	}

	if (!pOwner->IsClient())
		return;

	if (ICharacterInstance* pCharacterFP = m_pWeapon->GetEntity()->GetCharacter(eIGS_FirstPerson))
	{
		UpdateGrenadeAttachment(pCharacterFP, "currentshell", grenadeModel.c_str());
		UpdateGrenadeAttachment(pCharacterFP, "newshell", grenadeModel.c_str());
	}
}

void CLTagSingle::UpdateGrenadeAttachment( ICharacterInstance* pCharacter, const char* attachmentName, const char* model )
{
	CRY_ASSERT(pCharacter);

	IAttachment* pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName(attachmentName);
	if (pAttachment == NULL)
		return;

	IStatObj* pGrenadeModel = gEnv->p3DEngine->LoadStatObj(model);
	if (pGrenadeModel == NULL)
		return;

	IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
	if (pAttachmentObject && (pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_StatObj))
	{
		CCGFAttachment* pCGFAttachment = static_cast<CCGFAttachment*>(pAttachmentObject);
		pCGFAttachment->pObj = pGrenadeModel;  //pObj is an smart pointer, it should take care of releasing/AddRef
	}
	else
	{
		CCGFAttachment* pCGFAttachment = new CCGFAttachment();
		pCGFAttachment->pObj = pGrenadeModel;
		pAttachment->AddBinding(pCGFAttachment);
	}
}

const char* CLTagSingle::GetBestReloadAction(int ammoCount)
{
	int inventoryCount = m_pWeapon->GetInventoryAmmoCount(GetAmmoType());;

	if (ammoCount <= 1)
	{
		switch(inventoryCount)
		{
		case 1:
			return "reload_chamber_empty_last1";

		case 2:
			return "reload_chamber_empty_last2";

		default:
			return m_fireParams->actions.reload_chamber_empty.c_str();
		}
	}
	else
	{
		return m_fireParams->actions.reload_chamber_full.c_str();
	}
}

//CA: MP design have chosen to have only one grenade type so this is no longer necessary, and there were problems with delegate authority
//Leaving here for now in case the fickle designers change their mind again
/*bool CLTagSingle::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if(aspect == eEA_GameClientStatic)
	{
		ser.Value("grenType", static_cast<CLTagSingle*>(this), &CLTagSingle::NetGetGrenadeType, &CLTagSingle::NetSetGrenadeType, 'ui2');
	}

	return true;
}

void CLTagSingle::NetSetGrenadeType(uint type)
{
	CActor* pOwner = m_pWeapon->GetOwnerActor();
	if(pOwner && !pOwner->IsClient() && type != m_grenadeType)
	{
		m_grenadeType = (ELTAGGrenadeType)type;
		m_pWeapon->PlayAction(g_pItemStrings->change_firemode);
		SwitchGrenades();
	}
}*/

void CLTagSingle::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle)
{
	//TODO: CHECK THIS IS DOING EVERYTHING IT NEEDS TO AND NOTHING ELSE
	IEntityClass* ammo = m_fireParams->fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);

	CActor* pActor = m_pWeapon->GetOwnerActor();
	bool playerIsShooter = pActor?pActor->IsPlayer():false;

	const char* weaponAction = GetFireAction(ammoCount);
	const char *action = (ammoCount == 1) || (m_pWeapon->IsZoomed())? m_fireParams->actions.fire.c_str() : m_fireParams->actions.fire_cock.c_str();

	int flags = CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_CleanBlending;
	flags = PlayActionSAFlags(flags);

	m_pWeapon->PlayAction(action, 0, false, flags);
	m_pWeapon->PlayAction(weaponAction, 0, false, flags);

	CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, true);
	CLTAGGrenade* pGrenade = static_cast<CLTAGGrenade*>(pAmmo);

	if (pGrenade)
	{
		CGameRules* pGameRules = g_pGame->GetGameRules();

		pGrenade->SetParams(m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), 
			m_fireParams->fireparams.damage, 0.f, 0.f, 0.f, pGameRules->GetHitTypeId(m_fireParams->fireparams.hit_type.c_str()),
			m_fireParams->fireparams.bullet_pierceability_modifier);
		// this must be done after owner is set
		pGrenade->InitWithAI();
		pGrenade->SetDestination(m_pWeapon->GetDestination());
		pGrenade->SetGrenadeType(m_grenadeType);
		pGrenade->Launch(pos, dir, vel, m_speed_scale);

		m_projectileId = pGrenade->GetEntity()->GetId();
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
	m_next_shot = 0.0f;

	ammoCount--;

	if(ammoCount<0)
		ammoCount = 0;

	if (m_pWeapon->IsServer())
	{
		int clipSize = GetClipSize();
		if (clipSize != -1)
		{
			if (clipSize != 0)
				m_pWeapon->SetAmmoCount(ammo, ammoCount);
			else
				m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
		}
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	if (OutOfAmmo())
		m_pWeapon->OnOutOfAmmo(ammo);

	if (pAmmo && predictionHandle && pActor)
	{
		pAmmo->GetGameObject()->RegisterAsValidated(pActor->GetGameObject(), predictionHandle);
		pAmmo->GetGameObject()->BindToNetwork();
	}
	else if (pAmmo && pAmmo->IsPredicted() && gEnv->bClient && gEnv->bServer && pActor && pActor->IsClient())
	{
		pAmmo->GetGameObject()->BindToNetwork();
	}

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

