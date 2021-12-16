#include "StdAfx.h"
#include "HUD_WeaponSelect.h"

#include "HUD/HUD.h"
#include "Player.h"


//////////////////////////////////////////////////////////////////////////


CHUD_WeaponSelect::CHUD_WeaponSelect()
: m_dirty(CHUD_WeaponSelect::eDF_None)
, m_root(NULL)
, m_timerDisplay(0.0f)
, m_timerFade(0.0f)
{
}



CHUD_WeaponSelect::~CHUD_WeaponSelect()
{
}



void CHUD_WeaponSelect::Init()
{
	IHUDAsset* pAsset = GetAsset();
	HUD_FLASVAROBJ_REG(pAsset, "Root_WeaponSelect", m_root);
	HUD_FLASVAROBJ_REG(pAsset, "Root_WeaponSelect.WeaponRoll", m_infos[eWeapon_Primary].m_weaponObject);
}



void CHUD_WeaponSelect::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_root);
	HUD_FLASHOBJ_SAFERELEASE(m_infos[eWeapon_Primary].m_weaponObject);
}



void CHUD_WeaponSelect::OnHUDEvent(const SHUDEvent& event)
{
	switch (event.eventType)
	{
	case eHUDEvent_OnAmmoPickUp:
		{
			IEntityClass* pAmmoType = static_cast<IEntityClass*>(event.GetData(2).GetPtr());
			const int difference = event.GetData(1).GetInt();
			if(pAmmoType)
				OnAmmoPickup(pAmmoType, difference);
		}
		break;
	default:
		{
			CRY_ASSERT(event.eventType==eHUDEvent_OnItemSelected);
			OnItemSelected((EntityId)event.eventIntData);
		}
		break;
	}
}



void CHUD_WeaponSelect::Update(float frameTime)
{
	IHUDAsset* pAsset = GetAsset();

	float timerFade = m_timerFade;

	if(m_timerDisplay>0.0f)
	{
		m_timerDisplay -= frameTime;
	}
	else
	{
		if(timerFade>0.0f)
		{
			timerFade -= frameTime;
			m_dirty |= CHUD_WeaponSelect::eDF_Fading;
			m_timerFade = timerFade;
		}
	}

	if(m_dirty & CHUD_WeaponSelect::eDF_PrimaryWeapon)
	{
		UpdateWeaponDisplay(m_infos[eWeapon_Primary], m_root);
		m_root->SetMember("WeaponName", m_infos[eWeapon_Primary].m_pClass ? m_infos[eWeapon_Primary].m_displayName.c_str() : L"");
	}

	if(m_dirty & CHUD_WeaponSelect::eDF_Attachment)
	{
		m_root->SetMember("Attachment", "");
	}

	if(m_dirty & CHUD_WeaponSelect::eDF_Fading)
	{
		SFlashDisplayInfo info;
		info.SetAlpha(timerFade * 100.0f);
		m_root->SetDisplayInfo(info);
	}

	m_dirty = CHUD_WeaponSelect::eDF_None;
}



void CHUD_WeaponSelect::OnAmmoPickup(IEntityClass* pAmmoClass, const int difference)
{
}



void CHUD_WeaponSelect::OnItemSelected(const EntityId weaponId)
{

	
	if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(weaponId))
	{
		if(IItem * pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId))
		{
			if(IWeapon *pWeapon = pItem->GetIWeapon())
			{
				IEntityClass* pClass = pEntity->GetClass();

				if(UpdateWeaponData(weaponId, pClass, pItem, m_infos[eWeapon_Primary]))
				{
					m_dirty |= CHUD_WeaponSelect::eDF_PrimaryWeapon;
				}
				CRY_TODO(25, 3, 2010, "Replace this by a proper CVar when the HUDCVar change is integrated from UK. Don't do it before because integrating will be a PitA (TM) then!")
				m_timerDisplay = 2.0f;
				m_timerFade = 1.0f;
				m_dirty |= CHUD_WeaponSelect::eDF_Fading;
				return;
			}
		}
	}

	m_infos[eWeapon_Primary].Clear();
	m_dirty |= CHUD_WeaponSelect::eDF_PrimaryWeapon;
}



bool CHUD_WeaponSelect::UpdateWeaponData(const EntityId id, IEntityClass* pClass, IItem* pItem, SWeaponInfo& outInfo) const
{
	if(outInfo.m_pClass == pClass)
		return false;

	const char* name = pItem->GetDisplayName();
	if (!name || name[0] == '\0')
	{
		name = pClass->GetName();
	}

	outInfo.m_entityId = id;
	outInfo.m_displayName = g_pGame->GetHUD()->LocalizeStringW(name);
	outInfo.m_pOldClass = outInfo.m_pClass;
	outInfo.m_pClass = pClass;

	outInfo.m_pAmmoClass = NULL;

	IWeapon* pWeapon = pItem->GetIWeapon();

	IFireMode* pFM = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if(pFM)
	{
		outInfo.m_pAmmoClass = pFM->GetAmmoType();
	}
	return true;
}



void CHUD_WeaponSelect::UpdateWeaponDisplay(const SWeaponInfo& info, IFlashVariableObject* root) const
{
	if(info.m_pOldClass)
	{
		info.m_weaponObject->SetMember("m_oldWeapon", info.m_pOldClass->GetName());
	}
	else
	{
		info.m_weaponObject->SetMember("m_oldWeapon", "Empty");
	}

	if(info.m_pClass)
	{
		info.m_weaponObject->SetMember("m_newWeapon", info.m_pClass->GetName());
	}
	else
	{
		info.m_weaponObject->SetMember("m_newWeapon", "Empty");
	}

	info.m_weaponObject->GotoAndPlay("up");
}

//////////////////////////////////////////////////////////////////////////