/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Armor mode implementation
-------------------------------------------------------------------------
History:
- 10-3-2008: Created by Benito G.R.
- 20-10-2009: Renamed to armor mode

*************************************************************************/

#include "StdAfx.h"
#include "NanoSuit_v2.h"
#include "NanoModeArmor.h"
#include "Player.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

CNanoModeArmor::CNanoModeArmor(CNanoSuit& nanoSuit)
: m_nanoSuit(nanoSuit)
{

}

CNanoModeArmor::~CNanoModeArmor()
{

}

//-----------------------------------
void CNanoModeArmor::Release()
{
	delete this;
}

//-------------------------------------
void CNanoModeArmor::Activate(bool activate)
{
	if(activate)
	{
		CCCPOINT(Nanosuit_ArmorMode_On);

		m_nanoSuit.ProcessSuitAction("Activate_Armor");

		ActivateSuitPower(m_nanoSuit.IsSuitPowerActive());
	}
	else
	{
		CCCPOINT(Nanosuit_ArmorMode_Off);

		m_nanoSuit.ActivateSuitState("ArmorModeON", false);
	}
}

//---------------------------------
void CNanoModeArmor::Update(float frameTime)
{

}

//-------------------------------------
void CNanoModeArmor::ProcessEvent(const SNanoSuitEvent& event)
{

	switch(event.event)
	{
	
		case eNanoSuitEvent_ONAFTER_MODE_ACTIVATION:
			{
				m_nanoSuit.ProcessSuitAction("AfterActivate_Armor");
			}			
			break;

		case eNanoSuitEvent_MELEE:
			{
				bool armorActive = m_nanoSuit.IsSuitPowerActive();
				m_nanoSuit.ProcessSuitAction(armorActive ? "Melee_ArmorActive" : "Melee_Armor");
			}
			break;

		case eNanoSuitEvent_MELEE_HIT:
			{
				m_nanoSuit.ProcessSuitAction("Melee_Hit");
			}
			break;

		default:
			break;
	}

}

//-------------------------------------------
void CNanoModeArmor::OnStateChanged(ENanoSuitState newState)
{

}

//---------------------------------------------
void CNanoModeArmor::ActivateSuitPower(bool activate)
{
	if (!activate)
	{
		m_nanoSuit.m_suitGameParams.GetEnergy().RestoreToDefaultModifiers();
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModePassiveProperties(GetType()));		

		if (m_nanoSuit.GetOwner().IsClient() && m_nanoSuit.m_suitShapeDeformation.IsAnyTransitionEnabled())
		{
			m_nanoSuit.m_suitShapeDeformation.ExecuteShapeTransition(CNanoSuitShapeDeformation::eST_SuitModeDeactivate, 0.5f);
		}
	}
	else
	{
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModeActiveProperties(GetType()));
		m_nanoSuit.m_suitGameParams.GetEnergy().PatchModeModifiers(m_nanoSuit.s_nanoSuitParams.GetModeEnergyMod(GetType()));

		if (m_nanoSuit.GetOwner().IsClient())
		{
			m_nanoSuit.m_suitShapeDeformation.ExecuteShapeTransition(CNanoSuitShapeDeformation::eST_SuitModeActivateArmor, 0.35f);
		}
	}

	m_nanoSuit.ActivateSuitState("ArmorModeON", activate);
}

//-----------------------------------
bool CNanoModeArmor::CanActivateSuitPower() const
{
	return true;
}

//-------------------------------------
ENanoSuitMode CNanoModeArmor::GetType() const
{
	return eNanoSuitMode_Armor;
}

//--------------------------------------------
void CNanoModeArmor::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);

}