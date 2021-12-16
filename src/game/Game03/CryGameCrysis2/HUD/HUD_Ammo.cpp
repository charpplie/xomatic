#include "StdAfx.h"
#include "HUD_Ammo.h"

#include "IActorSystem.h"
#include "IItemSystem.h"

#include "HUD/HUD.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUDState.h"
#include "Flash/Flash.h"
#include "HUD/HUD_UnifiedAsset.h"


//////////////////////////////////////////////////////////////////////////


static const int NUM_CHAMBERS = 6;


CHUD_Ammo::CHUD_Ammo()
: m_forceLowAmmoOn( 0 )
, m_leavingBattleArea(false)
, m_showingUse(false)
, m_pWeaponClass(NULL)
, m_validAssets(true)
, m_objectRoot(NULL)
, m_objectAmmo(NULL)
, m_objectLowAmmo(NULL)
, m_objectFM(NULL)
, m_lowAmmoWarning(false)
, m_lowClipWarning(false)
{
	Reset();
	gEnv->pConsole->Register("hud_forceLowAmmoOn", &m_forceLowAmmoOn, m_forceLowAmmoOn, 0, "Draw safe areas.");
}



CHUD_Ammo::~CHUD_Ammo()
{
	gEnv->pConsole->UnregisterVariable("hud_forceLowAmmoOn", true);
}



void CHUD_Ammo::Init()
{
	IHUDAsset* pAsset = GetAsset();
	HUD_FLASVAROBJ_REG(pAsset, "_root", m_objectRoot);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.AmmoDisplay", m_objectAmmo);
	HUD_FLASVAROBJ_REG_DI(pAsset, "LowAmmunitionDisplay", m_objectLowAmmo, m_displayInfoLowAmmo);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.FireModeIcon", m_objectFM);
}



void CHUD_Ammo::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot);
	HUD_FLASHOBJ_SAFERELEASE(m_objectAmmo);
	HUD_FLASHOBJ_SAFERELEASE(m_objectLowAmmo);
	HUD_FLASHOBJ_SAFERELEASE(m_objectFM);
}



void CHUD_Ammo::OnHUDEvent(const SHUDEvent& event)
{
	switch (event.eventType)
	{
	case eHUDEvent_OnItemSelected:
		{
			OnItemSelected((EntityId)event.eventIntData);
		}
		break;
	case eHUDEvent_OnReloaded:
		{
			IWeapon* pData = (IWeapon*)(event.eventPtrData);
			if(pData)
				OnReloaded(pData);
		}
		break;
	case eHUDEvent_OnSetAmmoCount:
		{
			IWeapon* pData = (IWeapon*)(event.eventPtrData);
			if(pData)
				OnSetAmmoCount(pData);
		}
		break;
	case eHUDEvent_OnFireModeChanged:
		{
			IWeapon* pData = (IWeapon*)(event.eventPtrData);
			if(pData)
				OnFireModeChanged(pData, event.eventIntData);
		}
		break;
	case eHUDEvent_OnAmmoPickUp:
		{
			IWeapon* pData = static_cast<IWeapon*>(event.GetData(0).GetPtr());
			if(pData)
			{
				IFireMode* pFM = pData->GetFireMode(pData->GetCurrentFireMode());
				if(pFM && pFM->GetAmmoType())
				{
					if(!m_ammoType.compare(pFM->GetAmmoType()->GetName()))
					{
						UpdateWeaponEnvironment(pData);
						UpdateLowAmmoWarning();
					}
				}
			}
		}
		break;
	case eHUDEvent_OnPlayerDeath:
		{
			m_lowAmmoWarning = false;
		}
		break;
	case eHUDEvent_LeavingBattleArea :
		m_leavingBattleArea = true;
		break;
	case eHUDEvent_ReturningToBattleArea :
		m_leavingBattleArea = false;
		break;
	case eHUDEvent_ShowingUsablePrompt :
		m_showingUse = true;
		break;
	default:
		{
			CRY_ASSERT(event.eventType==eHUDEvent_HidingUsablePrompt);
			m_showingUse = true;
		}
		break;
	}
}



void CHUD_Ammo::Update(float frameTime)
{
	if(!m_validAssets)
		return;

	if(m_dirty & CHUD_Ammo::eDF_Ammo)
	{
		m_objectRoot->SetMember("Ammo", SFlashVarValue(m_weaponAmmo));
	}

	if(m_dirty & CHUD_Ammo::eDF_InventoryAmmo)
	{
		m_objectRoot->SetMember("Pool", SFlashVarValue(m_inventoryAmmo));
	}

	if(m_dirty & CHUD_Ammo::eDF_LowWarnings)
	{
		if(m_lowAmmoWarning)
		{
			m_objectAmmo->GotoAndStop(3);
		}
		else if(m_lowClipWarning)
		{
			m_objectAmmo->GotoAndStop(2);
		}
		else
		{
			m_objectAmmo->GotoAndStop(1);
		}
	}

	m_dirty = CHUD_Ammo::eDF_None;

	const bool shouldShowLowAmmoWarning = (m_lowAmmoWarning || m_forceLowAmmoOn) && !m_leavingBattleArea && !m_showingUse;

	if(shouldShowLowAmmoWarning != m_displayInfoLowAmmo.GetVisible())
	{
		SHUDEvent event(eHUDEvent_HUDElementVisibility);
		event.AddData(SHUDEventData(shouldShowLowAmmoWarning));
		event.AddData(SHUDEventData((int)eHUDElement_interactions));
		CHUD::CallEvent(event);
		m_displayInfoLowAmmo.SetVisible(shouldShowLowAmmoWarning);
		m_objectLowAmmo->SetDisplayInfo(m_displayInfoLowAmmo);
	}
}



void CHUD_Ammo::OnItemSelected(EntityId itemId)
{
	IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
	if(!pItem)
	{
		Reset();
		return;
	}

	IWeapon* pWeapon = pItem->GetIWeapon();
	if(!pWeapon)
	{
		Reset();
		return;
	}

	UpdateWeaponName(itemId);
	UpdateAmmo( pWeapon );
}



void CHUD_Ammo::OnReloaded(IWeapon* pWeapon)
{
	UpdateAmmo( pWeapon );
}

void CHUD_Ammo::OnSetAmmoCount(IWeapon* pWeapon)
{
	UpdateAmmo( pWeapon );
}

void CHUD_Ammo::OnShoot(IWeapon* pWeapon)
{
	UpdateWeaponStats(pWeapon);
}



void CHUD_Ammo::OnFireModeChanged(IWeapon * pWeapon, int currentFireMode)
{
	if(!m_validAssets)
		return;

	IFireMode* pFireMode = pWeapon->GetFireMode(currentFireMode);
	if(!pFireMode)
		return;

	if(m_fireMode.compare(pFireMode->GetName()))
	{
		m_objectFM->SetMember("m_oldFireMode", m_fireMode.c_str());
		m_fireMode = pFireMode->GetName();
		m_objectFM->SetMember("m_newFireMode", m_fireMode.c_str());
		m_objectFM->GotoAndPlay(2);
		UpdateAmmo( pWeapon );
	}
}



void CHUD_Ammo::UpdateWeaponEnvironment(IWeapon* pWeapon)
{
	IFireMode* pFM = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if(!pFM)
		return;

	int clipSize = pFM->GetClipSize();
	if(m_clipSize != clipSize)
	{
		m_clipSize = clipSize;
		m_dirty |= CHUD_Ammo::eDF_ClipSize;
		m_dirty |= CHUD_Ammo::eDF_Ammo;
	}

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;

	IEntityClass* pAmmoType = pFM->GetAmmoType();
	if(!pAmmoType)
		return;

	if(m_ammoType.compare(pAmmoType->GetName()))
	{
		m_ammoType = pAmmoType->GetName();
		m_dirty |= CHUD_Ammo::eDF_Ammo;
	}

	int inventoryAmmo = pActor->GetInventory()->GetAmmoCount(pAmmoType);
	if(m_inventoryAmmo != inventoryAmmo)
	{
		m_inventoryAmmo = inventoryAmmo;
		m_dirty |= CHUD_Ammo::eDF_InventoryAmmo;
	}
}



void CHUD_Ammo::UpdateWeaponStats(IWeapon* pWeapon)
{
	IFireMode* pFM = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if(!pFM)
		return;

	int weaponAmmo = pFM->GetAmmoCount();
	if(m_weaponAmmo != weaponAmmo)
	{
		m_weaponAmmo = weaponAmmo;
		m_dirty |= CHUD_Ammo::eDF_Ammo;
	}
}



void CHUD_Ammo::UpdateLowAmmoWarning()
{
	const bool lowClip = (m_clipSize>0) && (m_weaponAmmo < m_clipSize*0.3f);
	const bool lowAmmo = (m_clipSize>0) && (m_inventoryAmmo+m_weaponAmmo < m_clipSize);

	if(lowClip != m_lowClipWarning)
	{
		m_lowClipWarning = lowClip;
		m_dirty |= CHUD_Ammo::eDF_LowWarnings;
	}

	if(lowAmmo != m_lowAmmoWarning)
	{
		m_lowAmmoWarning = lowAmmo;
		m_dirty |= CHUD_Ammo::eDF_LowWarnings;
	}
}



void CHUD_Ammo::UpdateAmmo( IWeapon* pWeapon )
{
	UpdateWeaponEnvironment(pWeapon);
	UpdateWeaponStats(pWeapon);
	UpdateLowAmmoWarning();
}



void CHUD_Ammo::UpdateWeaponName(const EntityId id)
{
	const IEntity* pWeapon = gEnv->pEntitySystem->GetEntity(id);
	if(!pWeapon)
		return;

	IEntityClass* pClass = pWeapon->GetClass();
	if(m_pWeaponClass != pClass)
	{
		m_pWeaponClass = pClass;
		m_dirty |= CHUD_Ammo::eDF_WeaponName;
	}
}



void CHUD_Ammo::Reset()
{
	m_clipSize = 0;
	m_weaponAmmo = 0;
	m_inventoryAmmo = 0;
	m_fireMode="Single";
	m_ammoType = "bullet";
	m_pWeaponClass = NULL;
	m_dirty = eDF_All;
	UpdateLowAmmoWarning();
}


//////////////////////////////////////////////////////////////////////////
