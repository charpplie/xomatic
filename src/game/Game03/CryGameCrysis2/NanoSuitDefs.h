/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Common definitions for the Nanosuit v2 
-------------------------------------------------------------------------
History:
- 3-3-2008: Created by Benito G.R.
-21-10-2009: Updated for new suit design energy management

*************************************************************************/

#pragma once

#ifndef _NANOSUIT_DEFS_H_
#define _NANOSUIT_DEFS_H_

struct SActorStats;

#include <ItemParams.h>
#include "ItemParamReader.h"

#define ReadValue(hold, param)	reader.Read(#param, hold.param)
#define ReadAttributeByName(hold, param, attributeName, defaultValue) hold.param = defaultValue; reader.ReadAttribute(#param, attributeName, hold.param)

enum ENanoSuitMode
{
	eNanoSuitMode_Invalid = -1,
	eNanoSuitMode_Power	= 0,
	eNanoSuitMode_Armor,
	eNanoSuitMode_Stealth,
	eNanoSuitMode_Tactical,
	eNanoSuitMode_Last
};

enum ENanoSuitState
{
	eNanoSuitState_Invalid					= 0<<0,
	eNanoSuitState_Normal					= 1<<0,
	eNanoSuitState_Critical					= 1<<1,
	eNanoSuitState_Disabled					= 1<<2,
};

enum ENanoSuitEvent
{
	eNanoSuitEvent_SUITPOWER = 0,
	eNanoSuitEvent_SHOT,
	eNanoSuitEvent_HIT,
	eNanoSuitEvent_EXPLOSION,
	eNanoSuitEvent_EMP_DISCHARGE,
	eNanoSuitEvent_DISABLE_SUIT,
	eNanoSuitEvent_REBOOT_SUIT,
	eNanoSuitEvent_THROW,
	eNanoSuitEvent_MELEE,
	eNanoSuitEvent_MELEE_HIT,
	eNanoSuitEvent_ACTION,
	eNanoSuitEvent_POWERJUMP,
	eNanoSuitEvent_SLIDE_ACTIVE,
	eNanoSuitEvent_ENEMY_RAMMING_RUN,
	eNanoSuitEvent_ENEMY_RAMMING_JUMP,
	eNanoSuitEvent_ENEMY_RAMMING_SLIDE,
	eNanoSuitEvent_COMBO_LINK,
	eNanoSuitEvent_KVOLT_CHARGE,
	eNanoSuitEvent_ENERGY_DRAIN,
	eNanoSuitEvent_CLOAK,
	eNanoSuitEvent_UNCLOAK,
	eNanoSuitEvent_ONAFTER_MODE_ACTIVATION,
	eNanoSuitEvent_CLOAK_INTERFERENCE,
	eNanoSuitEvent_SUIT_CLIENTSTARTUP,
	eNanoSuitEvent_STEALTH_KILL,
	eNanoSuitEvent_AIRFRICTION,
	eNanoSuitEvent_CLOAKAWARENESS,
	eNanoSuitEvent_LAST
};

struct SNanoSuitEvent
{
	SNanoSuitEvent():
	sParam(NULL),
	fParam(0.0f),
	bParam(false),
	nParam(0)
	{
	
	}

	ENanoSuitEvent event;
	bool  bParam;
	int		nParam;
	float	fParam;
	Vec3	vParam;
	const char* sParam;
};

struct SNanoSuitEnergyParams
{
	SNanoSuitEnergyParams()
		: maximumEnergy(100.0f)
		, energyCriticalMercyTime(2.0f)
		, disableTime(5.0f)
		, regenerationDelay(0.5f)
		, regenerationRate(0.0f)
		, consumptionRateMin(0.0f)
		, consumptionRateMax(0.0f)
		, maxSpeedReferenceForConsumption(0.0f)
		, speedToConsumptionRateCurve(1.0f)
	{

	}

	void GetMemoryStatistics(ICrySizer* s) {};

	float maximumEnergy;						//Main energy reserve 
	float energyCriticalMercyTime;	//Mercy time when energy reaches critical level
	float disableTime;							//Default disable time

	float regenerationDelay;
	float regenerationRate;

	float consumptionRateMin;
	float consumptionRateMax;
	float maxSpeedReferenceForConsumption;
	float speedToConsumptionRateCurve;
};


struct SNanoSuitEnergy
{

	struct SUpdateContext
	{
		explicit SUpdateContext(float _frameTime, float _movementSpeed, ENanoSuitState _state)
			: frameTime(_frameTime)
			, movementSpeed(_movementSpeed)
			, suitState(_state)
		{

		}

		float frameTime;
		float movementSpeed;
		ENanoSuitState suitState;
	};

	SNanoSuitEnergy()
	{
		Reset();
	}

	void Reset()
	{
		m_currentEnergy = m_baseParams.maximumEnergy;
		m_consumptionRateMin = m_baseParams.consumptionRateMin;
		m_consumptionRateMax = m_baseParams.consumptionRateMax;
		m_maxSpeedReferenceForConsumption = m_baseParams.maxSpeedReferenceForConsumption;
		m_speedToConsumptionRateCurve = m_baseParams.speedToConsumptionRateCurve;
		m_regenerationDelay = m_baseParams.regenerationDelay;

		m_dynamicEnergyImmediate = 0.0f;
		m_dynamicScaleEnergy = 1.0f;		
		m_inCriticalStateTime = m_disabledTimeOut = 0.0f;
		m_recharging = false;
		m_isPermanentlyDisabled = false;
		m_lastSuitState = eNanoSuitState_Normal;
	}

	ILINE float Get() const { return m_currentEnergy; };
	ILINE float GetMaxEnergy() const { return m_baseParams.maximumEnergy; }
	ILINE void	SetBaseParameters(const SNanoSuitEnergyParams& params) { m_baseParams = params; }
	ILINE void	AddImmediateConsumption(float energyCost) { m_dynamicEnergyImmediate += energyCost; };
	ILINE void	ScaleEnergyUse(float scale) { m_dynamicScaleEnergy *= scale; };

	ILINE void	NetSet(float energy) { m_currentEnergy = energy; }
	
	ILINE bool	IsMercyTimeOver() const { return (m_inCriticalStateTime >= m_baseParams.energyCriticalMercyTime); }
	ILINE bool	IsRecoveredFromMercyTime() const { return (m_inCriticalStateTime <= 0.0f); }
	ILINE bool  IsReadyToReboot() const { return (m_disabledTimeOut <= 0.0f && !m_isPermanentlyDisabled); }
	ILINE bool	IsRecharging() const { return m_recharging; }

	void	PatchModeModifiers(const SNanoSuitEnergyParams& mods);
	void  RestoreToDefaultModifiers();
	void  OnDisabled( bool isPermanent = false, bool isDefaultTime = true, float time = 0 );
	void  OnRebooted();

	void	Update(const SUpdateContext& updateContext);

	void	Serialize(TSerialize ser);

private:

	void	UpdateNormal(const SUpdateContext& updateContext);
	void  UpdateDisabled(const SUpdateContext& updateContext);
	void	UpdateCritical(const SUpdateContext& updateContext);


	void ClearFrameValues()
	{
		m_dynamicEnergyImmediate = 0.0f;
		m_dynamicScaleEnergy = 1.0f;
	}

	float GetFrameConsumption(const SUpdateContext& updateContext) const;

	float m_currentEnergy; 
	
	float m_consumptionRateMin;
	float m_consumptionRateMax;
	float m_maxSpeedReferenceForConsumption;
	float m_speedToConsumptionRateCurve;

	float m_dynamicEnergyImmediate;
	float m_dynamicScaleEnergy;

	float m_inCriticalStateTime;
	float m_disabledTimeOut;
	float m_regenerationDelay;
	bool m_recharging;
	bool m_isPermanentlyDisabled;
	ENanoSuitState m_lastSuitState;
	CTimeValue m_lastConsumptionTime;

	SNanoSuitEnergyParams	m_baseParams;
};

//Parameters that might affect player abilities here
struct SNanoSuitProperties
{
	SNanoSuitProperties()
	{
		Reset();
	}

	void Reset()
	{
		speedScale = 1.0f;
		sprintSpeedScale = 1.0f;
		rotationSpeedScale = 1.0f;
		superJumpScale = 1.0f;
		meleeStrengthScale = 1.0f;
		meleeStrengthVehicleScale = 1.0f;
		meleeDamageScale = 1.0f;
		throwStrengthScale = 1.0f;
		noiseSupression = 0.0f;
		damageAbsorption = 0.0f;
		damageAbsorptionMelee = 0.0f;
		hitReactionAbsorption = 0.0f;
		camouflage = 0.0f;
		aiClassThreat = 0.0f;
	}

	void GetMemoryStatistics(ICrySizer* s) {};
	void Serialize(TSerialize ser);

	float speedScale;
	float sprintSpeedScale;
	float rotationSpeedScale;
	float superJumpScale;
	float meleeStrengthScale;
	float meleeStrengthVehicleScale;
	float meleeDamageScale;
	float throwStrengthScale;
	float noiseSupression;
	float damageAbsorption;
	float damageAbsorptionMelee;
	float hitReactionAbsorption;
	float camouflage;
	float aiClassThreat;
};

#include "ISerialize.h"
struct SNanoSuitGameParameters
{
	SNanoSuitGameParameters()
	{
		Reset();
	}

	void Reset()
	{
		m_props.Reset();
		m_cloakEnabled = m_suitPowerActive = m_isMenuOpen = false;

		m_mode = eNanoSuitMode_Armor;
		m_state = eNanoSuitState_Normal;
		m_energy.Reset();
	}

	void ApplyGameParametersMods(const SNanoSuitProperties& mod);
	void SetBaseGameParameters(const SNanoSuitProperties& baseParams) { m_propsBase = baseParams; };

	void GetMemoryStatistics(ICrySizer* s) const {};

	ILINE const SNanoSuitProperties& GetProps() const { return m_props; }
	ILINE void SetMode(ENanoSuitMode in_mode) { m_mode = in_mode; }
	ILINE ENanoSuitMode GetMode() const { return m_mode; }
	ILINE ENanoSuitState GetState() const { return m_state; }
	ILINE const SNanoSuitEnergy& GetEnergy() const { return m_energy; }
	ILINE SNanoSuitEnergy& GetEnergy() { return m_energy; }
	ILINE float GetSoundParamValue() const { return static_cast<float>(m_mode + 1) + ( IsSuitPowerActive() ? 0.5f : 0.f ); }	

	ILINE bool IsCloakEnabled() const { return m_cloakEnabled; }
	ILINE bool IsSuitPowerActive() const { return m_suitPowerActive; }
	ILINE void SetCloaked(bool set) { m_cloakEnabled = set; }

	ILINE bool GetIsMenuOpen() const { return m_isMenuOpen; }
	ILINE void SetSuitMenuOpenStatus(bool IsOpen) { m_isMenuOpen = IsOpen; }

	const char* ModeToStringParam() const;

	void Serialize( TSerialize ser );

private:
	friend class CNanoSuit;
	friend class CNanoSuitParams;
	SNanoSuitProperties m_props;

	ENanoSuitMode		m_mode;
	ENanoSuitState		m_state;
	SNanoSuitEnergy		m_energy;
	bool				m_cloakEnabled;
	bool				m_suitPowerActive;
	bool				m_isMenuOpen;

	SNanoSuitProperties m_propsBase;

};


class CNanoSuit;

struct INanoMode
{
	virtual void Release() = 0;
	
	virtual void Activate(bool activate) = 0;
	virtual bool CanActivateSuitPower() const = 0;
	virtual void ActivateSuitPower(bool activate) = 0;
	virtual void Update(float frameTime) = 0;
	virtual void ProcessEvent(const SNanoSuitEvent& event) = 0;
	virtual void OnStateChanged(ENanoSuitState newState) = 0;

	virtual void GetMemoryUsage(ICrySizer* s) const= 0;

	virtual ENanoSuitMode GetType() const = 0;
};


#endif
