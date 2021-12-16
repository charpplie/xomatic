#include "StdAfx.h"
#include "HUD_EnergyAndSuitMode.h"
#include "HUD.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CHUD_EnergyAndSuitMode::CHUD_EnergyAndSuitMode() :
m_suitMode(eNanoSuitMode_Invalid),
m_lastSuitMode(eNanoSuitMode_Invalid),
m_energy(-1),
m_suitState(eNanoSuitState_Normal),
m_dirty(CHUD_EnergyAndSuitMode::eDF_None),
m_powerActive(false),
m_objectRoot(NULL),
m_objectEnergy(NULL),
m_objectHealth(NULL),
m_healthCritical(false)
{
	for(int i=0; i<eNanoSuitMode_Last; ++i)
	{
		m_objectSuitModes[i] = NULL;
	}
}



CHUD_EnergyAndSuitMode::~CHUD_EnergyAndSuitMode()
{
}



void CHUD_EnergyAndSuitMode::Init()
{
	IHUDAsset* pAsset = GetAsset();
	HUD_FLASVAROBJ_REG(pAsset, "_root", m_objectRoot);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.Energy_Bar", m_objectEnergy);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.Health_Bar", m_objectHealth);

	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.SuitMode_Power", m_objectSuitModes[eNanoSuitMode_Power]);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.SuitMode_Armor", m_objectSuitModes[eNanoSuitMode_Armor]);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.SuitMode_Stealth", m_objectSuitModes[eNanoSuitMode_Stealth]);
	HUD_FLASVAROBJ_REG(pAsset, "Root_Ammo.SuitMode_Tactical", m_objectSuitModes[eNanoSuitMode_Tactical]);
}



void CHUD_EnergyAndSuitMode::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot);
	HUD_FLASHOBJ_SAFERELEASE(m_objectEnergy);
	HUD_FLASHOBJ_SAFERELEASE(m_objectHealth);

	HUD_FLASHOBJ_SAFERELEASE(m_objectSuitModes[eNanoSuitMode_Power]);
	HUD_FLASHOBJ_SAFERELEASE(m_objectSuitModes[eNanoSuitMode_Armor]);
	HUD_FLASHOBJ_SAFERELEASE(m_objectSuitModes[eNanoSuitMode_Stealth]);
	HUD_FLASHOBJ_SAFERELEASE(m_objectSuitModes[eNanoSuitMode_Tactical]);
}



void CHUD_EnergyAndSuitMode::OnHUDEvent(const SHUDEvent& event)
{
	switch(event.eventType)
	{
	case eHUDEvent_OnEnergyChanged :
		SetEnergy(event.GetData(0).GetFloat());
		break;
	case eHUDEvent_OnSuitModeChanged :
		{
			SetSuitMode(static_cast<ENanoSuitMode>(event.eventIntData));
		}
		break;
	case eHUDEvent_OnSuitStateChanged :
		SetSuitState((ENanoSuitState)event.eventIntData);
		break;
	case eHUDEvent_OnHealthChanged :
		SetHealth(event.GetData(0).GetFloat());
		break;
	case eHUDEvent_OnSuitPowerActivated:
		SuitPowerActivated(event.GetData(0).GetBool());
		break;
	}
}



void CHUD_EnergyAndSuitMode::Update(float frameTime)
{
	uint16 nextFrameDirtyFlags = CHUD_EnergyAndSuitMode::eDF_None;

	IHUDAsset* pBar = GetAsset();

	if(m_dirty & CHUD_EnergyAndSuitMode::eDF_Energy)
	{
		m_objectRoot->SetMember("Energy", m_energy);
	}

	if(m_dirty & CHUD_EnergyAndSuitMode::eDF_Health)
	{
		m_objectRoot->SetMember("Health", m_health);
	}

	if(m_dirty & CHUD_EnergyAndSuitMode::eDF_HealthCritical)
	{
		m_objectHealth->GotoAndStop( (m_healthCritical) ? 2 : 1);
	}

	if(m_dirty & CHUD_EnergyAndSuitMode::eDF_SuitState)
	{
		m_objectEnergy->GotoAndStop( (m_suitState == eNanoSuitState_Critical) ? 2 : 1);
	}

	if(m_dirty & CHUD_EnergyAndSuitMode::eDF_SuitMode)
	{
		if(m_lastSuitMode != eNanoSuitMode_Invalid)
		{
			m_objectSuitModes[m_lastSuitMode]->GotoAndStop(1);
		}
		if(m_suitMode != eNanoSuitMode_Invalid)
		{
			m_objectSuitModes[m_suitMode]->GotoAndStop(2);
		}
		m_lastSuitMode = m_suitMode;
	}

	if(m_dirty & CHUD_EnergyAndSuitMode::eDF_Power)
	{
		pBar->SetVariable("Root_Suit.SuitMode.Animation._visible", m_powerActive);
	}

	m_dirty = nextFrameDirtyFlags;
}



void CHUD_EnergyAndSuitMode::SetEnergy(float energy)
{
	int energyRounded = (int)(min(energy, 100.0f)+0.5f);
	if(m_energy != energyRounded)
	{
		m_energy = energyRounded;
		m_dirty |= CHUD_EnergyAndSuitMode::eDF_Energy;
	}
}



void CHUD_EnergyAndSuitMode::SetHealth(float health)
{
	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	assert(pClientActor);
	if (pClientActor)
	{
		const int maxHealth = pClientActor->GetMaxHealth();
		const int healthRounded = (int)(100.0f * ( (float)min(health, maxHealth)/(float)maxHealth ) );
		if(m_health != healthRounded)
		{
			m_health = healthRounded;
			m_dirty |= CHUD_EnergyAndSuitMode::eDF_Health;

			const bool healthCritical = m_health < 20;
			if(m_healthCritical != healthCritical)
			{
				m_healthCritical = healthCritical;
				m_dirty |= CHUD_EnergyAndSuitMode::eDF_HealthCritical;
			}

		}
	}
}



void CHUD_EnergyAndSuitMode::SetSuitMode(const ENanoSuitMode mode)
{
	if(m_suitMode != mode)
	{
		m_suitMode = mode;
		m_dirty |= CHUD_EnergyAndSuitMode::eDF_SuitMode;
	}
}



void CHUD_EnergyAndSuitMode::SetSuitState(ENanoSuitState state)
{
	if(state != m_suitState)
	{
		m_suitState = state;
		m_dirty |= CHUD_EnergyAndSuitMode::eDF_SuitState;
	}
}



void CHUD_EnergyAndSuitMode::SuitPowerActivated(const bool activated)
{
	if(m_powerActive != activated)
	{
		m_powerActive = activated;
		//m_dirty |= CHUD_EnergyAndSuitMode::eDF_Power;
	}
}



//////////////////////////////////////////////////////////////////////////
