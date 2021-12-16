/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:12:2005   14:01 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "AmmoPickup.h"
#include "Weapon.h"
#include "Actor.h"
#include "IItemSystem.h"
#include "ItemParamReader.h"
#include "GameCVars.h"
#include "Game.h"
#include "WeaponSharedParams.h"
#include <ISound.h>

#define EXPLOSIVE_GRENADE 0
#define SMOKE_GRENADE			1
#define FLASHBANG_GRENADE	2
#define EMP_GRENADE				3

//------------------------------------------------------------------------
void CAmmoPickup::PostInit( IGameObject * pGameObject )
{
	// fix for hud displaying the wrong ammo name
	if (!m_ammoName.empty())
	{
		SetEntityProperty("AmmoName", m_ammoName.c_str());
		SetEntityProperty("Count", m_ammoCount);
	}
	else
	{
		const char *ammoName=0;
		if (GetEntityProperty("AmmoName", ammoName) && ammoName && ammoName[0])
		{
			m_ammoName=ammoName;
			GetEntityProperty("Count", m_ammoCount);
		}
	}

	if (m_modelName.empty())
	{
		const char *model=0;
		if (GetEntityProperty("objModel", model) && model && model[0])
			m_modelName=model;
	}

	if (!m_modelName.empty())
		SetGeometry(eIGS_ThirdPerson, m_modelName.c_str());

	CWeapon::PostInit(pGameObject);
}

//------------------------------------------------------------------------
bool CAmmoPickup::CanUse(EntityId userId) const
{
	return false;
}

//------------------------------------------------------------------------
bool CAmmoPickup::CanPickUp(EntityId pickerId) const
{
	return true;
}

//------------------------------------------------------------------------
void CAmmoPickup::SerializeSpawnInfo( TSerialize ser )
{
	uint16 ammoClassId;
	string modelName;
	int count;
	ser.Value("modelName", modelName);
	ser.Value("classId", ammoClassId, 'ui16');
	ser.Value("count", count, 'ammo');
	m_modelName=modelName;

	static char ammoClassName[129]={0};
	g_pGame->GetIGameFramework()->GetNetworkSafeClassName(ammoClassName, 128, ammoClassId);

	m_ammoName=ammoClassName;
	m_ammoCount=count;
}

//------------------------------------------------------------------------
ISerializableInfoPtr CAmmoPickup::GetSpawnInfo()
{
	struct SInfo : public ISerializableInfo
	{
		string modelName;
		uint16 ammoClassId;
		uint32 count;
		void SerializeWith( TSerialize ser )
		{
			ser.Value("modelName", modelName);
			ser.Value("classId", ammoClassId, 'ui16');
			ser.Value("count", count, 'ammo');
		}
	};

	SInfo *p=new SInfo();
	p->modelName=m_modelName.c_str();
	p->ammoClassId=0;
	p->count=m_ammoCount;

	g_pGame->GetIGameFramework()->GetNetworkSafeClassId(p->ammoClassId, m_ammoName.c_str());

	return p;
}

//------------------------------------------------------------------------
void CAmmoPickup::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char* setup)
{
	SetOwnerId(pickerId);

	CActor *pActor=GetActor(pickerId);

	if (!pActor)
		return;

	IInventory *pInventory = GetActorInventory(pActor);
	if (!pInventory)
		return;

	if (IsServer())
	{
		// bonus ammo is always put in the actor's inv
		if (!m_bonusammo.empty())
		{
			TAmmoVector::const_iterator bonusAmmoEndCit = m_bonusammo.end();
			for (TAmmoVector::const_iterator bonusAmmoCit = m_bonusammo.begin(); bonusAmmoCit != bonusAmmoEndCit; ++bonusAmmoCit)
			{
				const SWeaponAmmo& currentAmmo = *bonusAmmoCit; 

				SetInventoryAmmoCount(currentAmmo.pAmmoClass, GetInventoryAmmoCount(currentAmmo.pAmmoClass)+currentAmmo.count);

				if(pActor->IsPlayer())
				{
					ShouldSwitchGrenade(currentAmmo.pAmmoClass);
					OnIncendiaryAmmoPickedUp(currentAmmo.pAmmoClass, currentAmmo.count);
				}
			}

			m_bonusammo.clear();
		}

		TAmmoVector::const_iterator ammoEndCit = m_ammo.end();
		for (TAmmoVector::const_iterator ammoCit = m_ammo.begin(); ammoCit != ammoEndCit; ++ammoCit)
		{
			const SWeaponAmmo& currentAmmo = *ammoCit; 

			SetInventoryAmmoCount(currentAmmo.pAmmoClass, GetInventoryAmmoCount(currentAmmo.pAmmoClass)+currentAmmo.count);

			if(pActor->IsPlayer())
			{
				ShouldSwitchGrenade(currentAmmo.pAmmoClass);
				OnIncendiaryAmmoPickedUp(currentAmmo.pAmmoClass, currentAmmo.count);
			}
		}

		if (!m_ammoName.empty() && m_ammoCount)
		{
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(m_ammoName.c_str());
			SetInventoryAmmoCount(pClass, GetInventoryAmmoCount(pClass)+m_ammoCount);

			if(pActor->IsPlayer())
			{
				ShouldSwitchGrenade(pClass);
				OnIncendiaryAmmoPickedUp(pClass,m_ammoCount);
			}
		}

		TriggerRespawn();
	}

	//Play sound
	if(!m_weaponsharedparams->pickupSound.empty())
	{
		IEntity *pPicker = m_pEntitySystem->GetEntity(pickerId);
		if(pPicker)
		{
			IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)pPicker->GetProxy(ENTITY_PROXY_SOUND);

			if(pSoundProxy)
			{
				//Execute sound at picker position
				pSoundProxy->PlaySound(m_weaponsharedparams->pickupSound, pPicker->GetWorldPos(),FORWARD_DIRECTION, FLAG_SOUND_DEFAULT_3D, eSoundSemantic_Weapon);
			}
		}
	}

	RemoveEntity();
}

//--------------------------------------------
bool CAmmoPickup::CheckAmmoRestrictions(EntityId pickerId)
{
	IActor* pPicker = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pickerId);
	if(pPicker)
	{
		IInventory *pInventory = pPicker->GetInventory();
		if(pInventory)
		{
			for (TAmmoVector::const_iterator it = m_ammo.begin(); it != m_ammo.end(); ++it)
			{
				const SWeaponAmmo& currentAmmo = *it; 

				int invAmmo  = pInventory->GetAmmoCount(currentAmmo.pAmmoClass);
				int invLimit = pInventory->GetAmmoCapacity(currentAmmo.pAmmoClass);

				if(invAmmo>=invLimit && (!gEnv->IsEditor()))
					return false;
			}

			const char *ammoName=0;
			if (GetEntityProperty("AmmoName", ammoName) && ammoName && ammoName[0])
			{
				int count=0;
				GetEntityProperty("Count", count);
				if (count)
				{
					IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammoName);
					if(pClass)
					{
						int invAmmo  = pInventory->GetAmmoCount(pClass);
						int invLimit = pInventory->GetAmmoCapacity(pClass);
						if(invAmmo>=invLimit)
							return false;
					}
				}
			}
		}
	}

	return true;
}

//---------------------------------------------------------------
void CAmmoPickup::ShouldSwitchGrenade(IEntityClass* pClass)
{
/*
		bool flashbang = (pClass==CItem::sFlashbangGrenade);
		bool smoke     = (pClass==CItem::sSmokeGrenade);
		bool emp       = (pClass==CItem::sEMPGrenade);
		bool explosive = (pClass==CItem::sExplosiveGrenade);
	
		if(!flashbang && !smoke && !emp && !explosive)
			return;
	
		CActor* pPlayer = GetOwnerActor();
		
		if(!pPlayer)
			return;
	
		COffHand* pOffHand = static_cast<COffHand*>(pPlayer->GetWeaponByClass(CItem::sOffHandClass));
	
		if(pOffHand)
		{
			if(IFireMode* fm = pOffHand->GetFireMode(pOffHand->GetCurrentFireMode()))
			{
				if(fm->OutOfAmmo())
				{
					if(explosive)
						pOffHand->RequestFireMode(EXPLOSIVE_GRENADE);
					else if(smoke)
						pOffHand->RequestFireMode(SMOKE_GRENADE);
					else if(flashbang)
						pOffHand->RequestFireMode(FLASHBANG_GRENADE);
					else if(emp)
						pOffHand->RequestFireMode(EMP_GRENADE);
				}
			}
		}*/
	
}

//----------------------------------------------------
void CAmmoPickup::OnIncendiaryAmmoPickedUp(IEntityClass *pClass,int count)
{
	bool incendiary = (pClass == CItem::sIncendiaryAmmo);

	if(incendiary && !gEnv->bMultiplayer)
	{
		CActor* pPlayer = GetOwnerActor();

		if(!pPlayer)
			return;

		if(IInventory *pInventory = pPlayer->GetInventory())
		{
			if(pInventory->GetCountOfClass("FY71IncendiaryAmmo")==0)
			{
				const int clipSize = 30;
				m_pItemSystem->GiveItem(pPlayer,"FY71IncendiaryAmmo",false,false,false);
				int amount = count-clipSize;
				amount = MAX(0,amount);
				pInventory->SetAmmoCount(pClass,amount);
			}
		}
	}
}
