#include "StdAfx.h"
#include "HUD_Debug.h"

#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"

#include "GameCVars.h"
#include "Player.h"
#include "GodMode.h"

//////////////////////////////////////////////////////////////////////////

#ifndef _RELEASE

CHUD_Debug::CHUD_Debug()
{
	CreateTextDisplay(eTTF_WeaponName, 0.8f, 0.85f, true);
	CreateTextDisplay(eTTF_FireModeName, 0.8f, 0.88f, true);
	CreateTextDisplay(eTTF_WeaponAmmo, 0.8f, 0.91f, false);
	CreateTextDisplay(eTTF_InventoryAmmo, 0.82f, 0.91f, false);
	CreateTextDisplay(eTTF_SuitMode, 0.15f, 0.85f, false);
	CreateTextDisplay(eTTF_Health, 0.15f, 0.88f, false);
	CreateTextDisplay(eTTF_Energy, 0.15f, 0.91f, false);

	CreateTextDisplay(eTTF_F1DebugKey, 0.1f, 0.02f, false);
	CreateTextDisplay(eTTF_F2DebugKey, 0.1f, 0.05f, false);
	CreateTextDisplay(eTTF_F3DebugKey, 0.25f, 0.02f, false);
	CreateTextDisplay(eTTF_F4DebugKey, 0.25f, 0.05f, false);
	CreateTextDisplay(eTTF_F10DebugKey, 0.5f, 0.02f, false);
	CreateTextDisplay(eTTF_F11DebugKey, 0.5f, 0.05f, false);

	SetTextDisplay(eTTF_Energy, "Energy: 0");
	SetTextDisplay(eTTF_Health, "Health: 1000");
}



CHUD_Debug::~CHUD_Debug()
{
}



void CHUD_Debug::Reload()
{
}



void CHUD_Debug::OnHUDEvent(const SHUDEvent& event)
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
			IWeapon* data = (IWeapon*)(event.eventPtrData);
			if(data)
				OnReloaded(data);
		}
		break;
	case eHUDEvent_OnSetAmmoCount:
		{
			IWeapon* data = (IWeapon*)(event.eventPtrData);
			if(data)
				OnSetAmmoCount(data);
		}
		break;
	case eHUDEvent_OnFireModeChanged:
		{
			IWeapon* data = (IWeapon*)(event.eventPtrData);
			if(data)
				OnFireModeChanged(data, event.eventIntData);
		}
		break;
	case eHUDEvent_OnEnergyChanged:
		{
			SetTextDisplay(eTTF_Energy, string().Format("Energy: %d", int_round(event.GetData(0).GetFloat())).c_str());
		}
		break;
	case eHUDEvent_OnHealthChanged:
		{
			SetTextDisplay(eTTF_Health, string().Format("Health: %d", int_round(event.GetData(0).GetFloat())).c_str());
		}
		break;
	case eHUDEvent_OnSuitModeChanged:
		{
			const char* data = static_cast<char*>(event.eventPtrData);
			if(data)
				SetTextDisplay(eTTF_SuitMode, string().Format("SuitMode: %s", data).c_str());
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
					}
				}
			}
		}
	}
}



void CHUD_Debug::Update(float frameTime)
{
	Displays::iterator it = m_displays.begin();
	Displays::iterator end = m_displays.end();
	for(; it!=end; ++it)
	{
		STextDisplay& display = (*it);
		if(display.m_temporary && display.m_timer > 0.0f)
		{
			display.m_timer -= frameTime;
			if(display.m_timer <= 0.0f)
			{
				display.m_timer = 0.0f;
			}
		}
	}
	if(g_pGame->GetHUD()->GetCVars()->hud_DMode)
		UpdateFKeys();
}



void CHUD_Debug::Draw()
{
	if(!g_pGame->GetHUD()->GetCVars()->hud_DMode)
		return;

	Displays::iterator it = m_displays.begin();
	Displays::iterator end = m_displays.end();
	for(; it!=end; ++it)
	{
		STextDisplay& display = (*it);
		if(!display.m_temporary || display.m_timer > 0.0f)
		{
			float color[4] = {display.m_intensity,display.m_intensity,display.m_intensity,display.m_intensity};
			gEnv->pRenderer->Draw2dLabel(display.m_position.x, display.m_position.y, 1.75f, color, false, display.m_text.c_str());
		}
	}
}



void CHUD_Debug::UpdateFKeys()
{
	CryFixedStringT<64> output;
	output.Format("Backspace: DMode On");
	SetTextDisplay(eTTF_F1DebugKey, output);

	CPlayer* pPlayer = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayer)
	{
		SetTextDisplay(eTTF_F2DebugKey, "No Player", 0.0f, 1.0f);
		SetTextDisplay(eTTF_F3DebugKey, "", 0.0f, 1.0f);
		SetTextDisplay(eTTF_F4DebugKey, "", 0.0f, 1.0f);
		SetTextDisplay(eTTF_F5DebugKey, "", 0.0f, 1.0f);
		SetTextDisplay(eTTF_F10DebugKey, "", 0.0f, 1.0f);
		SetTextDisplay(eTTF_F11DebugKey, "", 0.0f, 1.0f);
		return;
	}
	else
	{
		SetTextDisplay(eTTF_F2DebugKey, "", 0.0f, 1.0f);
	}

	if(pPlayer->GetFlyMode()>0)
	{
		output.Format("F3: FlyMode %s", pPlayer->GetFlyMode()==2?"On":"On (Collide)");
		SetTextDisplay(eTTF_F3DebugKey, output, 0.0f, 1.0f);
	}
	else
	{
		output = "F3: FlyMode Off";
		SetTextDisplay(eTTF_F3DebugKey, output, 0.0f, 0.3f);
	}

	CGodMode& godMode = CGodMode::GetInstance();
	if(godMode.IsGodModeActive())
	{
		output.Format("F4: %s On", godMode.GetCurrentStateString());
		SetTextDisplay(eTTF_F4DebugKey, output.c_str(), 0.0f, 1.0f);
	}
	else
	{
		output = "F4: GodMode Off";
		SetTextDisplay(eTTF_F4DebugKey, output.c_str(), 0.0f, 0.3f);
	}

	if(ICVar* pCVar = gEnv->pConsole->GetCVar("p_draw_helpers"))
	{
		output.Format("F10: p_draw_helpers: %s", pCVar->GetIVal()>0 ? "On" : "Off");
		SetTextDisplay(eTTF_F10DebugKey, output.c_str(), 0.0f, pCVar->GetIVal()>0 ? 1.0f : 0.3f);
	}

	if(ICVar* pCVar = gEnv->pConsole->GetCVar("ai_DebugDraw"))
	{
		output.Format("F11: ai_DebugDraw: %s", pCVar->GetIVal()>0 ? "On" : "Off");
		SetTextDisplay(eTTF_F11DebugKey, output.c_str(), 0.0f, pCVar->GetIVal()>0 ? 1.0f : 0.3f);
	}
}



void CHUD_Debug::ClearWeapon()
{
	SetTextDisplay(eTTF_FireModeName, "None", 4.0f);
	SetTextDisplay(eTTF_WeaponName, "None", 4.0f);
	SetTextDisplay(eTTF_InventoryAmmo, "/ 0");
	SetTextDisplay(eTTF_WeaponAmmo, "0");
}



void CHUD_Debug::SetTextDisplay(ETemporaryTextFlags id, const char* text, float timer, float intensity)
{
	Displays::iterator it = m_displays.begin();
	Displays::iterator end = m_displays.end();
	for(; it!=end; ++it)
	{
		STextDisplay& display = (*it);
		if(display.m_id == id)
		{
			display.m_text = text;
			display.m_timer = timer;
			display.m_intensity = intensity;
			return;
		}
	}
}



void CHUD_Debug::CreateTextDisplay(ETemporaryTextFlags id, float posX, float posY, bool temporary)
{
	posX *= gEnv->pRenderer->GetWidth();
	posY *= gEnv->pRenderer->GetHeight();

	Displays::iterator it = m_displays.begin();
	Displays::iterator end = m_displays.end();
	for(; it!=end; ++it)
	{
		STextDisplay& display = (*it);
		if(display.m_id == id)
		{
			display.m_position.x = posX;
			display.m_position.y = posY;
			return;
		}
	}
	m_displays.push_back(CHUD_Debug::STextDisplay(id, "", posX, posY, 0.0f, temporary));
}



void CHUD_Debug::UpdateWeaponEnvironment(IWeapon* pWeapon)
{
	IFireMode* pFM = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if(!pFM)
		return;

	m_clipSize = pFM->GetClipSize();

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;

	IEntityClass* pAmmoType = pFM->GetAmmoType();
	if(!pAmmoType)
		return;

	if(m_ammoType.compare(pAmmoType->GetName()))
	{
		m_ammoType = pAmmoType->GetName();
	}

	SetTextDisplay(eTTF_InventoryAmmo, string().Format("/ %d",pActor->GetInventory()->GetAmmoCount(pFM->GetAmmoType())).c_str());
}



void CHUD_Debug::UpdateWeaponStats(IWeapon* pWeapon)
{
	IFireMode* pFM = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
	if(!pFM)
		return;

	SetTextDisplay(eTTF_WeaponAmmo, string().Format("%d",pFM->GetAmmoCount()).c_str());
}



void CHUD_Debug::OnItemSelected(EntityId itemId)
{
	IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
	if(!pItem)
	{
		ClearWeapon();
		return;
	}

	IWeapon* pWeapon = pItem->GetIWeapon();
	if(!pWeapon)
	{
		ClearWeapon();
		return;
	}

	UpdateWeaponEnvironment(pWeapon);
	UpdateWeaponStats(pWeapon);
	const char* itemName = pItem->GetDisplayName();
	if(!itemName || !itemName[0])
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(itemId);
		if(pEntity)
		{
			itemName = pEntity->GetClass()->GetName();
		}
	}
	SetTextDisplay(eTTF_WeaponName, itemName ? itemName : "Invalid Item", 4.0f);
}



void CHUD_Debug::OnReloaded(IWeapon* pWeapon)
{
	UpdateWeaponEnvironment(pWeapon);
	UpdateWeaponStats(pWeapon);
}

void CHUD_Debug::OnSetAmmoCount(IWeapon* pWeapon)
{
	UpdateWeaponEnvironment(pWeapon);
	UpdateWeaponStats(pWeapon);
}

void CHUD_Debug::OnShoot(IWeapon* pWeapon)
{
	UpdateWeaponStats(pWeapon);
}



void CHUD_Debug::OnFireModeChanged(IWeapon * pWeapon, int currentFireMode)
{
	IFireMode* pFireMode = pWeapon->GetFireMode(currentFireMode);
	if(!pFireMode)
		return;

	SetTextDisplay(eTTF_FireModeName, pFireMode->GetName() ? pFireMode->GetName() : "Invalid Firemode", 4.0f);
}

#endif //_RELEASE
//////////////////////////////////////////////////////////////////////////