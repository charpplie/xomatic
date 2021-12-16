/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Energy management for suit
-------------------------------------------------------------------------
History:
- 18-9-2008: Created by Benito G.R.
-21-10-2009: Updated for new suit design energy management

*************************************************************************/

#include "StdAfx.h"
#include "NanoSuitDefs.h"


void SNanoSuitEnergy::Update(const SUpdateContext& updateContext)
{
	switch(updateContext.suitState)
	{
	case eNanoSuitState_Normal:
		{
			UpdateNormal(updateContext);
			break;
		}

	case eNanoSuitState_Critical:
		{
			UpdateCritical(updateContext);
			break;
		}

	case eNanoSuitState_Disabled:
		{
			UpdateDisabled(updateContext);
			break;
		}

	default:
		{
			CRY_ASSERT_MESSAGE(0, "Invalid suit state");
		}
	}

	ClearFrameValues();

	m_lastSuitState = updateContext.suitState;
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::UpdateNormal(const SUpdateContext& updateContext)
{
	const float frameEnergyConsumption = (m_dynamicEnergyImmediate + GetFrameConsumption(updateContext)) * m_dynamicScaleEnergy;
	float energyRecharge = 0.0f;
	m_recharging = false;

	CTimeValue now = gEnv->pTimer->GetAsyncTime();

	if(frameEnergyConsumption > 0.0f)
	{
		m_lastConsumptionTime = gEnv->pTimer->GetAsyncTime();
	}
	else if(now.GetSeconds() - m_lastConsumptionTime.GetSeconds() > m_regenerationDelay)
	{
		energyRecharge = m_baseParams.regenerationRate * updateContext.frameTime;
		m_recharging = m_currentEnergy < m_baseParams.maximumEnergy;
	}

	m_currentEnergy = CLAMP( m_currentEnergy - frameEnergyConsumption + energyRecharge , 0.0f , m_baseParams.maximumEnergy);

	//const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	//gEnv->pRenderer->Draw2dLabel(50.0f, 150.0f, 2.0f, white, false, "Energy Normal: %.2f", m_currentEnergy);
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::UpdateDisabled(const SUpdateContext& updateContext)
{
	if (!m_isPermanentlyDisabled)
		m_disabledTimeOut -= updateContext.frameTime;

	m_currentEnergy = 0.0f;

	//const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	//gEnv->pRenderer->Draw2dLabel(50.0f, 150.0f, 2.0f, white, false, "Disabled timeout: %.2f", m_disabledTimeOut);
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::UpdateCritical(const SUpdateContext& updateContext)
{
	if (updateContext.suitState != m_lastSuitState)
	{
		m_inCriticalStateTime = 0.0f;
	}

	m_inCriticalStateTime += (GetFrameConsumption(updateContext) > 0.0f) ? updateContext.frameTime : - updateContext.frameTime;

	m_currentEnergy = 0.0f;

	//const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	//gEnv->pRenderer->Draw2dLabel(50.0f, 150.0f, 2.0f, white, false, "In Mercy time for: %.2f", m_inCriticalStateTime);
}

//////////////////////////////////////////////////////////////////////////
float SNanoSuitEnergy::GetFrameConsumption(const SUpdateContext& updateContext) const
{
	bool shouldScaleToSpeed = (m_maxSpeedReferenceForConsumption > 0.0f);
	float frameConsumption = 0.0f;

	if (shouldScaleToSpeed)
	{
		assert(m_consumptionRateMin <= m_consumptionRateMax);
		assert(m_speedToConsumptionRateCurve > 0.0f);

		const float speedFactor = pow(updateContext.movementSpeed / m_maxSpeedReferenceForConsumption, m_speedToConsumptionRateCurve);
		frameConsumption = (m_consumptionRateMin + (speedFactor * (m_consumptionRateMax - m_consumptionRateMin))) * updateContext.frameTime;
	}
	else
	{
		frameConsumption = m_consumptionRateMax * updateContext.frameTime;
	}

	return frameConsumption;
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::PatchModeModifiers(const SNanoSuitEnergyParams& mods)
{
	m_consumptionRateMin = mods.consumptionRateMin;
	m_consumptionRateMax = mods.consumptionRateMax;
	m_maxSpeedReferenceForConsumption = mods.maxSpeedReferenceForConsumption;
	m_speedToConsumptionRateCurve = mods.speedToConsumptionRateCurve;
	m_regenerationDelay = mods.regenerationDelay;
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::RestoreToDefaultModifiers()
{
	m_consumptionRateMin = m_baseParams.consumptionRateMin;
	m_consumptionRateMax = m_baseParams.consumptionRateMax;
	m_maxSpeedReferenceForConsumption = m_baseParams.maxSpeedReferenceForConsumption;
	m_speedToConsumptionRateCurve = m_baseParams.speedToConsumptionRateCurve;
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::OnDisabled( bool isPermanent, bool isDefaultTime, float time )
{
	m_isPermanentlyDisabled = isPermanent;
	m_disabledTimeOut = isDefaultTime ? m_baseParams.disableTime : time;
}

//////////////////////////////////////////////////////////////////////////
void SNanoSuitEnergy::OnRebooted()
{
	m_isPermanentlyDisabled = false;
	m_disabledTimeOut = 0;
}


//////////////////////////////////////////////////////////////////////////
void SNanoSuitGameParameters::ApplyGameParametersMods(const SNanoSuitProperties& mod)
{
	m_props.speedScale = m_propsBase.speedScale + mod.speedScale;
	m_props.sprintSpeedScale = m_propsBase.sprintSpeedScale + mod.sprintSpeedScale;
	m_props.rotationSpeedScale = m_propsBase.rotationSpeedScale + mod.rotationSpeedScale;
	m_props.superJumpScale = m_propsBase.superJumpScale + mod.superJumpScale;
	m_props.meleeStrengthScale = m_propsBase.meleeStrengthScale + mod.meleeStrengthScale;
	m_props.meleeStrengthVehicleScale = m_propsBase.meleeStrengthVehicleScale + mod.meleeStrengthVehicleScale;
	m_props.meleeDamageScale = m_propsBase.meleeDamageScale + mod.meleeDamageScale;
	m_props.throwStrengthScale = m_propsBase.throwStrengthScale + mod.throwStrengthScale;
	m_props.damageAbsorption = m_propsBase.damageAbsorption + mod.damageAbsorption;
	m_props.damageAbsorptionMelee = m_propsBase.damageAbsorption + mod.damageAbsorptionMelee;
	m_props.hitReactionAbsorption = m_propsBase.hitReactionAbsorption + mod.hitReactionAbsorption;
	m_props.camouflage = m_propsBase.camouflage + mod.camouflage;
	m_props.noiseSupression = m_propsBase.noiseSupression + mod.noiseSupression;
	m_props.aiClassThreat = m_propsBase.aiClassThreat + mod.aiClassThreat;
}


//////////////////////////////////////////////////////////////////////////
const char* SNanoSuitGameParameters::ModeToStringParam() const
{
	static const char modeParamTable[eNanoSuitMode_Last][4] = {"pow", "arm", "sth", "tac"};
	
	assert((m_mode >= 0) && (m_mode < eNanoSuitMode_Last));
	return modeParamTable[m_mode];
}
