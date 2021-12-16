/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements shared parameters for vehicles

-------------------------------------------------------------------------
History:
- Created by Sascha Hoba

*************************************************************************/

#include "StdAfx.h"
#include "VehicleMovementSharedParams.h"

CVehicleMovementSharedParams* CVehicleMovementSharedParamsList::Register( const char *className, const CVehicleMovementSharedParams& params )
{
	TSharedParamsMap::iterator it=m_params.find(CONST_TEMP_STRING(className));
	if (it!=m_params.end())
		return it->second;

	CVehicleMovementSharedParams *newParams = new CVehicleMovementSharedParams();

	newParams->airbrakeTime = params.airbrakeTime;
	newParams->bumpIntensityMult = params.bumpIntensityMult;
	newParams->bumpMinSpeed = params.bumpMinSpeed;
	newParams->bumpMinSusp = params.bumpMinSusp;
	newParams->isBreakingOnIdle = params.isBreakingOnIdle;
	newParams->kvSteerMax = params.kvSteerMax;
	newParams->pedalLimitMax = params.pedalLimitMax;
	newParams->rpmGearShiftSpeed = params.rpmGearShiftSpeed;
	newParams->rpmInterpSpeed = params.rpmInterpSpeed;
	newParams->rpmRelaxSpeed = params.rpmRelaxSpeed;
	newParams->stabiMax = params.stabiMax;
	newParams->stabiMin = params.stabiMin;
	newParams->steerRelaxation = params.steerRelaxation;
	newParams->steerSpeed = params.steerSpeed;
	newParams->steerSpeedMin = params.steerSpeedMin;
	newParams->steerSpeedScale = params.steerSpeedScale;
	newParams->steerSpeedScaleMin = params.steerSpeedScaleMin;
	newParams->suspDampingMax = params.suspDampingMax;
	newParams->suspDampingMaxSpeed = params.suspDampingMaxSpeed;
	newParams->suspDampingMin = params.suspDampingMin;
	newParams->v0SteerMax = params.v0SteerMax;
	newParams->vMaxSteerMax = params.vMaxSteerMax;
	newParams->correction.angSpring = params.correction.angSpring;
	newParams->correction.lateralSpring = params.correction.lateralSpring;
	newParams->gears = params.gears;
	newParams->handling.acceleration = params.handling.acceleration;
	newParams->handling.accelMultiplier1 = params.handling.accelMultiplier1;
	newParams->handling.accelMultiplier2 = params.handling.accelMultiplier2;
	newParams->handling.backFriction = params.handling.backFriction;
	newParams->handling.compressionBoost = params.handling.compressionBoost;
	newParams->handling.compressionBoostHandBrake = params.handling.compressionBoostHandBrake;
	newParams->handling.decceleration = params.handling.decceleration;
	newParams->handling.frictionOffset = params.handling.frictionOffset;
	newParams->handling.frontFriction = params.handling.frontFriction;
	newParams->handling.grip1 = params.handling.grip1;
	newParams->handling.grip2 = params.handling.grip2;
	newParams->handling.gripK = params.handling.gripK;
	newParams->handling.handBrakeAngCorrectionScale = params.handling.handBrakeAngCorrectionScale;
	newParams->handling.handBrakeBackFrictionScale = params.handling.handBrakeBackFrictionScale;
	newParams->handling.handBrakeDecceleration = params.handling.handBrakeDecceleration;
	newParams->handling.handBrakeDeccelerationPowerLock = params.handling.handBrakeDeccelerationPowerLock;
	newParams->handling.handBrakeFrontFrictionScale = params.handling.handBrakeFrontFrictionScale;
	newParams->handling.handBrakeLateralCorrectionScale = params.handling.handBrakeLateralCorrectionScale;
	newParams->handling.handBrakeLockBack = params.handling.handBrakeLockBack;
	newParams->handling.handBrakeLockFront = params.handling.handBrakeLockFront;
	newParams->handling.handBrakeRotationDeadTime = params.handling.handBrakeRotationDeadTime;
	newParams->handling.reductionAmount = params.handling.reductionAmount;
	newParams->handling.reductionRate = params.handling.reductionRate;
	newParams->handling.reverseSpeed = params.handling.reverseSpeed;
	newParams->handling.topSpeed = params.handling.topSpeed;

	m_params.insert(TSharedParamsMap::value_type(className, newParams));

	return newParams;
}

void CVehicleMovementSharedParamsList::UpdateSharedParams( const char *className, const CVehicleMovementSharedParams& params )
{
	TSharedParamsMap::iterator it=m_params.find(CONST_TEMP_STRING(className));
	if (it!=m_params.end())
	{
		CVehicleMovementSharedParams* pParams = it->second;

		if(pParams)
		{
			pParams->airbrakeTime = params.airbrakeTime;
			pParams->bumpIntensityMult = params.bumpIntensityMult;
			pParams->bumpMinSpeed = params.bumpMinSpeed;
			pParams->bumpMinSusp = params.bumpMinSusp;
			pParams->isBreakingOnIdle = params.isBreakingOnIdle;
			pParams->kvSteerMax = params.kvSteerMax;
			pParams->pedalLimitMax = params.pedalLimitMax;
			pParams->rpmGearShiftSpeed = params.rpmGearShiftSpeed;
			pParams->rpmInterpSpeed = params.rpmInterpSpeed;
			pParams->rpmRelaxSpeed = params.rpmRelaxSpeed;
			pParams->stabiMax = params.stabiMax;
			pParams->stabiMin = params.stabiMin;
			pParams->steerRelaxation = params.steerRelaxation;
			pParams->steerSpeed = params.steerSpeed;
			pParams->steerSpeedMin = params.steerSpeedMin;
			pParams->steerSpeedScale = params.steerSpeedScale;
			pParams->steerSpeedScaleMin = params.steerSpeedScaleMin;
			pParams->suspDampingMax = params.suspDampingMax;
			pParams->suspDampingMaxSpeed = params.suspDampingMaxSpeed;
			pParams->suspDampingMin = params.suspDampingMin;
			pParams->v0SteerMax = params.v0SteerMax;
			pParams->vMaxSteerMax = params.vMaxSteerMax;
			pParams->correction.angSpring = params.correction.angSpring;
			pParams->correction.lateralSpring = params.correction.lateralSpring;
			pParams->gears = params.gears;
			pParams->handling.acceleration = params.handling.acceleration;
			pParams->handling.accelMultiplier1 = params.handling.accelMultiplier1;
			pParams->handling.accelMultiplier2 = params.handling.accelMultiplier2;
			pParams->handling.backFriction = params.handling.backFriction;
			pParams->handling.compressionBoost = params.handling.compressionBoost;
			pParams->handling.compressionBoostHandBrake = params.handling.compressionBoostHandBrake;
			pParams->handling.decceleration = params.handling.decceleration;
			pParams->handling.frictionOffset = params.handling.frictionOffset;
			pParams->handling.frontFriction = params.handling.frontFriction;
			pParams->handling.grip1 = params.handling.grip1;
			pParams->handling.grip2 = params.handling.grip2;
			pParams->handling.gripK = params.handling.gripK;
			pParams->handling.handBrakeAngCorrectionScale = params.handling.handBrakeAngCorrectionScale;
			pParams->handling.handBrakeBackFrictionScale = params.handling.handBrakeBackFrictionScale;
			pParams->handling.handBrakeDecceleration = params.handling.handBrakeDecceleration;
			pParams->handling.handBrakeDeccelerationPowerLock = params.handling.handBrakeDeccelerationPowerLock;
			pParams->handling.handBrakeFrontFrictionScale = params.handling.handBrakeFrontFrictionScale;
			pParams->handling.handBrakeLateralCorrectionScale = params.handling.handBrakeLateralCorrectionScale;
			pParams->handling.handBrakeLockBack = params.handling.handBrakeLockBack;
			pParams->handling.handBrakeLockFront = params.handling.handBrakeLockFront;
			pParams->handling.handBrakeRotationDeadTime = params.handling.handBrakeRotationDeadTime;
			pParams->handling.reductionAmount = params.handling.reductionAmount;
			pParams->handling.reductionRate = params.handling.reductionRate;
			pParams->handling.reverseSpeed = params.handling.reverseSpeed;
			pParams->handling.topSpeed = params.handling.topSpeed;
		}
	}
}

CVehicleMovementSharedParams* CVehicleMovementSharedParamsList::GetSharedParams( const char *className, bool create )
{
	TSharedParamsMap::iterator it=m_params.find(CONST_TEMP_STRING(className));
	if (it!=m_params.end())
		return it->second;

	if (create)
	{
		CVehicleMovementSharedParams *params = new CVehicleMovementSharedParams();
		m_params.insert(TSharedParamsMap::value_type(className, params));

		return params;
	}

	return 0;
}