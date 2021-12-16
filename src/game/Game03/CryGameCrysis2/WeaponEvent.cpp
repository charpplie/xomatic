/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 9:12:2005   10:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Weapon.h"
#include "Player.h"
#include "GameRules.h"
#include "GameCVars.h"
#include <IActorSystem.h>
#include <IAISystem.h>
#include <IAgent.h>
#include "Audio/GameAudio.h"

#include "ItemSharedParams.h"
#include "HUD/HUD.h"
#include "HUD/HUDEventDispatcher.h"
#include "WeaponSharedParams.h"
#include "Battlechatter.h"
#include "AmmoParams.h"
#include "WeaponSystem.h"
#include "FireMode.h"


#define BROADCAST_WEAPON_EVENT(event, params)	\
	for (TWeaponEventListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next()) \
		notifier->event params;


//------------------------------------------------------------------------
void CWeapon::OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3&vel)
{
	BROADCAST_WEAPON_EVENT(OnShoot, (this, shooterId, ammoId, pAmmoType, pos, dir, vel));

	//FIXME:quick temporary solution
	IActor *pClientActor=m_pGameFramework->GetClientActor();

	CActor *pActor = static_cast<CActor*> (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId));

	if (pActor)
	{
		pActor->HandleEvent(SGameObjectEvent(eCGE_OnShoot,eGOEF_ToExtensions));

		if(pActor->HasNanoSuit())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_SHOT;
			pActor->SendActorSuitEvent(event);
		}

		if (m_fm && !m_fm->IsSilenced() && pActor->GetActorClass() == CPlayer::GetActorClassType())
		{
			CPlayer *pPlayer=static_cast<CPlayer*>(pActor);
			pPlayer->SonarVisPerkRegisterGunFire(pAmmoType);
		}

		if (ShouldSendOnShootHUDEvent())
		{
			SHUDEvent event(eHUDEvent_OnShoot);
			event.AddData(SHUDEventData((int)shooterId));
			CHUD::CallEvent(event);
		}

		if(!gEnv->bMultiplayer && IsServer())
		{
			if (pActor == pClientActor)
			{
				if (IAIObject *pAIObject=pActor->GetEntity()->GetAI())
				{
					gEnv->pAISystem->SendSignal(SIGNALFILTER_LEADER, 1, "OnEnableFire",	pAIObject, 0);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::OnStartFire(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnStartFire, (this, shooterId));

	/*if (gEnv->bServer)
	{
		if(CActor* pOwner = static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId)))
		{
			if (pOwner->GetActorClass()==CPlayer::GetActorClassType())
			{
				CPlayer *pPlayer = static_cast<CPlayer *>(pOwner);
				if(CNanoSuit *pSuit = pPlayer->GetNanoSuit())
				{
					if (pSuit->IsInvulnerable())
						pSuit->SetInvulnerability(false);
				}
			}
		}
	}*/
}

//------------------------------------------------------------------------
void CWeapon::OnStopFire(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnStopFire, (this, shooterId));
}


//------------------------------------------------------------------------
void CWeapon::OnFireModeChanged(int currentFireMode)
{
	BROADCAST_WEAPON_EVENT(OnFireModeChanged, (this, currentFireMode));
}


//------------------------------------------------------------------------
void CWeapon::OnStartReload(EntityId shooterId, IEntityClass* pAmmoType)
{
	BROADCAST_WEAPON_EVENT(OnStartReload, (this, shooterId, pAmmoType));

	if (CActor *pActor = GetOwnerActor())
	{
		if (IAIObject *pAIObject=pActor->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal( SIGNALFILTER_SENDER, 1, "OnReload", pAIObject);
	}

	IFireMode *pFireMode = GetFireMode(GetCurrentFireMode());
	if (pFireMode)
	{
		if(GetInventoryAmmoCount(pAmmoType) < pFireMode->GetClipSize())
		{
			BATTLECHATTER(BC_LowAmmo, shooterId);
		}
		else
		{
			BATTLECHATTER(BC_Reloading, shooterId);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::OnEndReload(EntityId shooterId, IEntityClass* pAmmoType)
{
	BROADCAST_WEAPON_EVENT(OnEndReload, (this, shooterId, pAmmoType));

	if (CActor *pActor = GetOwnerActor())
	{
		if (IAIObject *pAIObject=pActor->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal( SIGNALFILTER_SENDER, 1, "OnReloadDone", pAIObject);
	}

	if (m_doingMagazineSwap)
	{
		const char* magazineAttachment = m_weaponsharedparams->reloadMagazineParams.magazineAttachment.c_str();
		HideCharacterAttachment(eIGS_FirstPerson, magazineAttachment, false);
		HideCharacterAttachment(eIGS_ThirdPerson, magazineAttachment, false);
		m_doingMagazineSwap = false;
	}
}

void CWeapon::OnSetAmmoCount(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnSetAmmoCount, (this, shooterId));
}

//------------------------------------------------------------------------
void CWeapon::OnOutOfAmmo(IEntityClass* pAmmoType)
{
	BROADCAST_WEAPON_EVENT(OnOutOfAmmo, (this, pAmmoType));

/*	- no need to send signal here - puppet will check ammo when fires
	if (CActor *pActor = GetOwnerActor())
	{
		if (IAIObject *pAIObject=Actor->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal( SIGNALFILTER_SENDER, 1, "OnOutOfAmmo", pAIObject);
	}
*/
}

//------------------------------------------------------------------------
void CWeapon::OnReadyToFire()
{
	BROADCAST_WEAPON_EVENT(OnReadyToFire, (this));
}

//------------------------------------------------------------------------
void CWeapon::OnPickedUp(EntityId actorId, bool destroyed)
{
	BROADCAST_WEAPON_EVENT(OnPickedUp, (this, actorId, destroyed));

	BaseClass::OnPickedUp(actorId, destroyed);

	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_NO_PROXIMITY);
	GetEntity()->SetFlags(GetEntity()->GetFlags() & ~ENTITY_FLAG_ON_RADAR);

	if(GetISystem()->IsSerializingFile() == 1)
		return;

	CActor *pActor = GetActor(actorId);
	if (!pActor)
		return;

	RegisterUsedAmmoWithInventory(GetActorInventory(pActor));

	// bonus ammo is always put in the actor's inv
	if (!m_bonusammo.empty())
	{
		for (TAmmoVector::iterator it = m_bonusammo.begin(); it != m_bonusammo.end(); ++it)
		{
			const SWeaponAmmo& currentBonusAmmo = *it;

			SetInventoryAmmoCount(currentBonusAmmo.pAmmoClass, GetInventoryAmmoCount(currentBonusAmmo.pAmmoClass)+currentBonusAmmo.count);
		}

		m_bonusammo.clear();
	}
	
	// current ammo is only added to actor's inv, if we already have this weapon
	if (destroyed && m_sharedparams->params.unique)
	{
		for (TAmmoVector::iterator it = m_ammo.begin(); it!=m_ammo.end(); ++it)
		{
			//Only add ammo to inventory, if not accessory ammo (accessories give ammo already)
			const SWeaponAmmo& currentAmmo = *it;
			const SWeaponAmmo* pAccessoryAmmo = SWeaponAmmoUtils::FindAmmo(m_weaponsharedparams->ammoParams.accessoryAmmo, currentAmmo.pAmmoClass);
			if(pAccessoryAmmo != NULL)
			{
				SetInventoryAmmoCount(currentAmmo.pAmmoClass, GetInventoryAmmoCount(currentAmmo.pAmmoClass)+currentAmmo.count);
			}
		}
	}

	if(!gEnv->bMultiplayer && !m_initialSetup.empty() && pActor->IsClient())
	{
		for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
			FixAccessories(GetAccessoryParams(it->first), true);
	}

	//Need to increase ammo in current mag
	if (pActor->GetActorClass() == CPlayer::GetActorClassType())
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(pActor);
		if(pPlayer && pPlayer->IsPerkActive(ePerk_HeavyLoadout))
		{
			IEntityClass* pCurrentAmmoClass = m_fm ? m_fm->GetAmmoType() : NULL;
			if (pCurrentAmmoClass)
			{
				const SWeaponAmmo* pCurrentAmmo = SWeaponAmmoUtils::FindAmmo(m_ammo, pCurrentAmmoClass);

				if ((pCurrentAmmo) && (pCurrentAmmo->count > 0) && (GetInventoryAmmoCount(pCurrentAmmo->pAmmoClass) > 0))
				{
					SetAmmoCount(pCurrentAmmo->pAmmoClass, (int) (pCurrentAmmo->count * 1.5f));
				}
			}
		}
	}

	m_expended_ammo = 0;
}

//------------------------------------------------------------------------
void CWeapon::OnDropped(EntityId actorId)
{
	BROADCAST_WEAPON_EVENT(OnDropped, (this, actorId));

	BaseClass::OnDropped(actorId);

	GetEntity()->SetFlags(GetEntity()->GetFlags() & ~ENTITY_FLAG_NO_PROXIMITY);
	GetEntity()->SetFlags(GetEntity()->GetFlags() | ENTITY_FLAG_ON_RADAR);

	m_expended_ammo = 0;
}

//------------------------------------------------------------------------
void CWeapon::OnDroppedByAI(IInventory* pAIInventory)
{
	CRY_ASSERT(pAIInventory);

	const TAmmoVector& minDroppedAmmoMap = m_weaponsharedparams->ammoParams.minDroppedAmmo;
	const bool checkForMinAmmoDrops = m_minDropAmmoAvailable && !minDroppedAmmoMap.empty();

	TFireModeVector::const_iterator firemodesEndIt = m_firemodes.end();
	for (TFireModeVector::const_iterator firemodeCit = m_firemodes.begin(); firemodeCit != firemodesEndIt ; ++firemodeCit)
	{
		const CFireMode* pFiremode = *firemodeCit;
		if (pFiremode)
		{
			IEntityClass* pFiremodeAmmo = pFiremode->GetAmmoType();
			if (pFiremodeAmmo)
			{
				// Additional ammo which always goes to the magazine, when ai drops a weapon
				int minAmmoDrop = 0;
				if (checkForMinAmmoDrops)
				{
					const SWeaponAmmo* pMinDroppedAmmo = SWeaponAmmoUtils::FindAmmoConst(minDroppedAmmoMap, pFiremodeAmmo);
					if (pMinDroppedAmmo != NULL)
					{
						minAmmoDrop = pMinDroppedAmmo->count;
						SWeaponAmmo* pCurrentAmmo = SWeaponAmmoUtils::FindAmmo(m_ammo, pFiremodeAmmo);
						if (pCurrentAmmo)
						{
							pCurrentAmmo->count = min(minAmmoDrop + pCurrentAmmo->count, pFiremode->GetClipSize());
						}
						else
						{
							m_ammo.push_back(SWeaponAmmo(pFiremodeAmmo, min(minAmmoDrop, pFiremode->GetClipSize())));
						}
					}
				}

				// Exchange also ammo pool from inventory to bonus ammo map, for next user who picks it up
				{
					const int ammoCount = pAIInventory->GetAmmoCount(pFiremodeAmmo);

					if (ammoCount > 0)
					{
						SetInventoryAmmoCount(pFiremodeAmmo, 0);
						SWeaponAmmoUtils::SetAmmo(m_bonusammo, pFiremodeAmmo, ammoCount);
					}
					else
					{
						//Put always some extra rounds when dropped by AI
						const SWeaponAmmo* pDefaultBonusAmmo = SWeaponAmmoUtils::FindAmmoConst(m_weaponsharedparams->ammoParams.bonusAmmo, pFiremodeAmmo);
						if (pDefaultBonusAmmo)
						{
							SetInventoryAmmoCount(pFiremodeAmmo, 0);
							SWeaponAmmoUtils::SetAmmo(m_bonusammo, pFiremodeAmmo, max(minAmmoDrop, Random(pDefaultBonusAmmo->count)));
						}
					}
				}
			}
		}
	}

	m_minDropAmmoAvailable = false;
}

//------------------------------------------------------------------------
void CWeapon::OnDroppedByPlayer(IInventory* pPlayerInventory)
{
	CRY_ASSERT(pPlayerInventory);

	TFireModeVector::const_iterator firemodesEndIt = m_firemodes.end();
	for (TFireModeVector::const_iterator firemodeCit = m_firemodes.begin(); firemodeCit != firemodesEndIt ; ++firemodeCit)
	{
		const CFireMode* pFiremode = *firemodeCit;
		if (pFiremode)
		{
			IEntityClass* pFiremodeAmmo = pFiremode->GetAmmoType();
			if (pFiremodeAmmo)
			{
				// Exchange also ammo pool from inventory to bonus ammo map, for next user who picks it up
				const int ammoCount = pPlayerInventory->GetAmmoCount(pFiremodeAmmo);
				if (ammoCount > 0)
				{
					SetInventoryAmmoCount(pFiremodeAmmo, 0);
					SWeaponAmmoUtils::SetAmmo(m_bonusammo, pFiremodeAmmo, ammoCount);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::OnDualdWieldWeaponDropped(IInventory* pPlayerInventory)
{
	CRY_ASSERT(pPlayerInventory);

	IEntityClass* pAmmoType = m_fm ? m_fm->GetAmmoType() : NULL;
	if(pAmmoType)
	{
		const int ammoCount	= GetAmmoCount(pAmmoType);
		IItem* pDualWieldSlave = GetDualWieldSlave();
		CWeapon* pWeaponSlave = pDualWieldSlave ? static_cast<CWeapon*>(pDualWieldSlave->GetIWeapon()) : NULL;
		if(pWeaponSlave)
		{
			//Check for same ammo as master
			const int slaveAmmo = pWeaponSlave->GetAmmoCount(pAmmoType);
			if(slaveAmmo > ammoCount)
			{
				//Swap ammo counts, it's like dropping the gun with less ammo
				pWeaponSlave->SetAmmoCount(pAmmoType,ammoCount);
				SetAmmoCount(pAmmoType,slaveAmmo);
			}
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::OnMelee(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnMelee, (this, shooterId));
}

//------------------------------------------------------------------------
void CWeapon::OnStartTargetting(IWeapon *pWeapon)
{
	BROADCAST_WEAPON_EVENT(OnStartTargetting,(this));
}

//------------------------------------------------------------------------
void CWeapon::OnStopTargetting(IWeapon *pWeapon)
{
	BROADCAST_WEAPON_EVENT(OnStopTargetting,(this));
}

//------------------------------------------------------------------------
void CWeapon::OnSelected(bool selected)
{
	BaseClass::OnSelected(selected);

	BROADCAST_WEAPON_EVENT(OnSelected,(this, selected));
}

//------------------------------------------------------------------------
void CWeapon::OnEnterFirstPerson()
{
	BaseClass::OnEnterFirstPerson();

	if (m_fm)
	{
		m_fm->Activate(false);
		m_fm->Activate(true);
	}
}

//------------------------------------------------------------------------
void CWeapon::OnEnterThirdPerson()
{
	BaseClass::OnEnterThirdPerson();

	if (m_fm)
	{
		m_fm->Activate(false);
		m_fm->Activate(true);
	}
}

//---------------------------------------------------------------------------
void CWeapon::AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event)
{
	const SReloadMagazineParams& magazineParams = m_weaponsharedparams->reloadMagazineParams;

	bool magazineSwapEvent = event.m_EventName && (stricmp(event.m_EventName, magazineParams.magazineEvent.c_str()) == 0);


	if (magazineSwapEvent)
	{
		int slot = (GetEntity()->GetCharacter(eIGS_FirstPerson) == pCharacter) ? eIGS_FirstPerson : eIGS_ThirdPerson;

		//Spawn effect only once (in FP the event might come twice, one for FP, one for shadow character)
		if (!m_doingMagazineSwap)
		{
			const char* helperBone = (event.m_BonePathName && event.m_BonePathName[0]) ? event.m_BonePathName : "";
			if (event.m_CustomParameter && event.m_CustomParameter[0])
			{
				SpawnEffect(slot, event.m_CustomParameter, helperBone);
			}
			m_doingMagazineSwap = true;
		}

		HideCharacterAttachment(slot, magazineParams.magazineAttachment.c_str(), true);
	}
}
