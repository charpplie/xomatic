/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Common definitions for the Nanosuit v2 
-------------------------------------------------------------------------
History:
- 4-3-2010: Created by Ian Masters for serialization purposes

*************************************************************************/

#include "StdAfx.h"
#include "NanoSuitDefs.h"

void SNanoSuitGameParameters::Serialize( TSerialize ser )
{
	// [Ian|02.03.10] I was going to save m_lastMode and m_lastState for completeness but they don't appear to be used anywhere.
	// [Ian|03.03.10] Will be revisiting these shortly to strip all references from engine.

	// Double check that int& is the same size as enum
	COMPILE_TIME_ASSERT(sizeof((int&)m_mode) == sizeof(m_mode));

	ser.Value("suitMode", (int&)m_mode);
	ser.Value("suitState", (int&)m_state);
	ser.Value("cloakEnabled", m_cloakEnabled);
	ser.Value("suitPowerActive", m_suitPowerActive);
	//ser.Value("isMenuOpen", m_isMenuOpen);

	m_props.Serialize(ser); // m_propsBase also?
	m_energy.Serialize(ser);
}

void SNanoSuitEnergy::Serialize(TSerialize ser)
{
	ser.Value("currentEnergy", m_currentEnergy);
	//ser.Value("consumptionRateMin", m_consumptionRateMin);
	//ser.Value("consumptionRateMax", m_consumptionRateMax);
	//ser.Value("maxSpeedReferenceForConsumption", m_maxSpeedReferenceForConsumption);
	//ser.Value("speedToConsumptionRateCurve", m_speedToConsumptionRateCurve);
	//ser.Value("dynamicEnergyImmediate", m_dynamicEnergyImmediate);
	//ser.Value("dynamicScaleEnergy", m_dynamicScaleEnergy);
	ser.Value("inCriticalStateTime", m_inCriticalStateTime);
	ser.Value("disabledTimeOut", m_disabledTimeOut);
	//ser.Value("regenerationDelay", m_regenerationDelay);
	ser.Value("recharging", m_recharging);
	ser.Value("isPermanentlyDisabled", m_isPermanentlyDisabled);
	ser.Value("lastSuitState", (int&)m_lastSuitState);
	// m_baseParams?
}

void SNanoSuitProperties::Serialize(TSerialize ser)
{
}
