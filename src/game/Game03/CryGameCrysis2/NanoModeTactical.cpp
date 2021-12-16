/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Tactical mode implementation
-------------------------------------------------------------------------
History:
- 3-2-2009: Created by Benito G.R.

*************************************************************************/
#include "StdAfx.h"
#include "NanoSuit_v2.h"
#include "NanoModeTactical.h"
#include "Player.h"
#include "Item.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

CNanoModeTactical::CNanoModeTactical(CNanoSuit& nanoSuit)
: m_nanoSuit(nanoSuit)
{
}

CNanoModeTactical::~CNanoModeTactical()
{
}

//-----------------------------------
void CNanoModeTactical::Release()
{
	delete this;
}

//-------------------------------------
void CNanoModeTactical::Activate(bool activate)
{
	if(activate)
	{
		CCCPOINT(Nanosuit_TacticalMode_On);

		m_nanoSuit.ProcessSuitAction("Activate_Tactical");
		ActivateSuitPower(m_nanoSuit.IsSuitPowerActive());
	}
	else
	{
		CCCPOINT(Nanosuit_TacticalMode_Off);

		ActivateSuitVisor(false);
		m_nanoSuit.ActivateSuitState("TacticalModePassive", false);
	}
}

//-----------------------------------------------------------------
void CNanoModeTactical::Update(float frameTime)
{
	bool checkCloseVisor = m_nanoSuit.GetOwner().IsClient() && m_nanoSuit.GetGameParams().IsSuitPowerActive();

	if (checkCloseVisor)
	{
		if (m_nanoSuit.GetOwner().IsSprinting())
		{
			m_nanoSuit.ActivateSuitPower(false);
		}
	}
}

//-------------------------------------
void CNanoModeTactical::ProcessEvent(const SNanoSuitEvent& event)
{
	switch(event.event)
	{
	
	case eNanoSuitEvent_ONAFTER_MODE_ACTIVATION:
		{
			m_nanoSuit.ProcessSuitAction("AfterActivate_Tactical");
			
			//Needs to be here as well for first update
			m_nanoSuit.ActivateSuitState("TacticalModePassive", !m_nanoSuit.IsSuitPowerActive());
		}			
		break;

	case eNanoSuitEvent_MELEE:
		{
			m_nanoSuit.ProcessSuitAction("Melee");
		}
		break;

	case eNanoSuitEvent_MELEE_HIT:
		{
			m_nanoSuit.ProcessSuitAction("Melee_Hit");
		}		

	default:
		break;
	}
}

//-------------------------------------------
void CNanoModeTactical::OnStateChanged(ENanoSuitState newState)
{

}

//---------------------------------------------
void CNanoModeTactical::ActivateSuitPower(bool activate)
{
	if (!activate)
	{
		m_nanoSuit.m_suitGameParams.GetEnergy().RestoreToDefaultModifiers();
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModePassiveProperties(GetType()));
	}
	else
	{
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModeActiveProperties(GetType()));
		m_nanoSuit.m_suitGameParams.GetEnergy().PatchModeModifiers(m_nanoSuit.s_nanoSuitParams.GetModeEnergyMod(GetType()));
	}

	m_nanoSuit.ActivateSuitState("TacticalModePassive", !activate);

	ActivateSuitVisor(activate);
}

//-----------------------------------
bool CNanoModeTactical::CanActivateSuitPower() const
{
	CPlayer& suitOwner = m_nanoSuit.GetOwner();
	
	bool canUseVisor = !suitOwner.GetActorStats()->mountedWeaponID && !suitOwner.IsSprinting() && !suitOwner.HasHeavyWeaponEquipped() && !suitOwner.IsInPickAndThrowMode();
	canUseVisor &= !suitOwner.IsSwimming();

	return (canUseVisor);
}

//-------------------------------------
void CNanoModeTactical::ActivateSuitVisor(bool activate)
{
	CItem* pCurrentItem = static_cast<CItem*>(m_nanoSuit.GetOwner().GetCurrentItem());

	bool isVisorOn = pCurrentItem ? (pCurrentItem->GetEntity()->GetClass() == CItem::sBinocularsClass) : false;

	if (activate && !isVisorOn)
	{
		m_nanoSuit.GetOwner().SelectItemByName("Binoculars", true);
	}
	else if (!activate && isVisorOn)
	{
		m_nanoSuit.GetOwner().SelectLastItem(false, true);
	}
}

//-------------------------------------
ENanoSuitMode CNanoModeTactical::GetType() const
{
	return eNanoSuitMode_Tactical;
}

//--------------------------------------
void CNanoModeTactical::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);
}


