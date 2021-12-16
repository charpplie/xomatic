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

#ifndef __VEHICLEMOVEMENTSHAREDPARAMETERS_H__
#define __VEHICLEMOVEMENTSHAREDPARAMETERS_H__

struct SVehicleGears
{
	enum { kMaxGears = 10 };

	enum { kReverse = 0, kNeutral, kFirst };

	float	averageWheelRadius;
	int		curGear;
	float	curRpm;
	int		accelerating;
	float	targetRpm;
	float	timer;
};

struct SSharedHandling
{
	float	acceleration, decceleration, topSpeed, reverseSpeed;
	float	reductionAmount, reductionRate;
	float	compressionBoost, compressionBoostHandBrake;
	float	backFriction, frontFriction, frictionOffset;
	float	grip1, grip2;	// Grip fraction at zero slip speed and grip fraction at high slip speed (usually 1.0f).
	float	gripK;				// 1.0f / slipSpeed.
	float	accelMultiplier1, accelMultiplier2;
	float	handBrakeDecceleration, handBrakeDeccelerationPowerLock;
	bool	handBrakeLockFront, handBrakeLockBack;
	float	handBrakeFrontFrictionScale, handBrakeBackFrictionScale;
	float	handBrakeAngCorrectionScale, handBrakeLateralCorrectionScale;
	float	handBrakeRotationDeadTime;
};

struct SSharedCorrection
{
	float lateralSpring;
	float angSpring;
};

struct SSharedVehicleGears
{
	float	ratios[SVehicleGears::kMaxGears];
	float	invRatios[SVehicleGears::kMaxGears];
	int		numGears;
	float minChangeUpTime;
	float minChangeDownTime;
};

class CVehicleMovementSharedParams
{

protected:
	mutable uint32			m_refs;
	bool					m_valid;

public:
	CVehicleMovementSharedParams(): m_refs(0), m_valid(false) {};
	virtual ~CVehicleMovementSharedParams(){};

	virtual void AddRef() const { ++m_refs; };
	virtual uint32 GetRefCount() const { return m_refs; };
	virtual void Release() const { 
		if (--m_refs <= 0)
			delete this;
	};

	virtual bool Valid() const { return m_valid; };
	virtual void SetValid(bool valid) { m_valid=valid; };

	void GetMemoryStatistics(ICrySizer *s){};

	bool								isBreakingOnIdle;
	float								steerSpeed, steerSpeedMin;			// Steer speed at vMaxSteerMax and steer speed at v = 0.
	float								kvSteerMax;							// Reduce steer max at vMaxSteerMax.
	float								v0SteerMax;							// Max steering angle in deg at v = 0.
	float								steerSpeedScaleMin;					// Scale for sens at zero vel.
	float								steerSpeedScale;					// Scale for sens at vMaxSteerMax.
	float								steerRelaxation;					// Relaxation speed to center in degrees.
	float								vMaxSteerMax;						// Speed at which entire kvSteerMax is subtracted from v0SteerMax.
	float								pedalLimitMax;						// At vMaxSteerMax pedal is clamped to 1 - pedalLimitMax.
	float								suspDampingMin, suspDampingMax, suspDampingMaxSpeed;
	float								stabiMin, stabiMax;
	float								rpmRelaxSpeed, rpmInterpSpeed, rpmGearShiftSpeed, airbrakeTime;
	float								bumpMinSusp, bumpMinSpeed, bumpIntensityMult;
	SSharedVehicleGears					gears;
	SSharedHandling						handling;
	SSharedCorrection							correction;
};

class CVehicleMovementSharedParamsList
{
	typedef std::map<string, _smart_ptr<CVehicleMovementSharedParams> > TSharedParamsMap;
public:
	CVehicleMovementSharedParamsList() {};
	virtual ~CVehicleMovementSharedParamsList() {};

	void Reset() { m_params.clear(); };

	CVehicleMovementSharedParams* Register(const char *className, const CVehicleMovementSharedParams& params);
	void UpdateSharedParams(const char *className, const CVehicleMovementSharedParams& params);
	CVehicleMovementSharedParams* GetSharedParams(const char *className, bool create);

	void GetMemoryStatistics(ICrySizer *s)
	{
		s->AddContainer(m_params);
		for (TSharedParamsMap::iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
		{
			s->Add(iter->first);
			iter->second->GetMemoryStatistics(s);
		}
	}

	TSharedParamsMap m_params;
};

#endif