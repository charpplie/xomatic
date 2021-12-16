/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements wheel based vehicle movement

-------------------------------------------------------------------------
History:
- Created by stan fichele

*************************************************************************/
#ifndef __VEHICLEMOVEMENTARCADEWHEELED_H__
#define __VEHICLEMOVEMENTARCADEWHEELED_H__


#include "Network/NetActionSync.h"
#include "IVehicleSystem.h"
#include "VehicleMovementBase.h"


#if 0

// There is no need to net serialise

class CVehicleMovementArcadeWheeled;
class CNetworkMovementArcadeWheeled
{
public:
	CNetworkMovementArcadeWheeled();
	CNetworkMovementArcadeWheeled(CVehicleMovementArcadeWheeled *pMovement);

	typedef CVehicleMovementArcadeWheeled * UpdateObjectSink;

	bool operator == (const CNetworkMovementArcadeWheeled &rhs)
	{
		//return (abs(m_steer-rhs.m_steer)<0.001f) && (abs(m_pedal-rhs.m_pedal)<0.001f) && (m_brake==rhs.m_brake);
		return false;
	};

	bool operator != (const CNetworkMovementArcadeWheeled &rhs)
	{
		return !this->operator==(rhs);
	};

	void UpdateObject( CVehicleMovementArcadeWheeled *pMovement );
	void Serialize(TSerialize ser, EEntityAspects aspects);

	static const NetworkAspectType CONTROLLED_ASPECT = eEA_GameClientDynamic;

private:
	float m_steer;
	float m_pedal;
	bool m_brake;
	bool m_boost;
};
#endif

/*
	Wheel
*/
struct SVehicleWheel
{
	IVehiclePart* wheelPart;

	float radius;
    float mass;
	float invMass;
	float inertia;
	float invInertia;

	Vec3 offset;
	float bottomOffset;				// offset + (zAxis * bottomOffset) == bottom point of wheel
	int axleIndex;					// 0 == back wheel, 1 == front wheel
	int bCanLock;					// 0 or 1

	// Calculated Each Frame
	Vec3 frictionDir[2];		// [0] == inline friction direction, [1] == lateral friction direction
	Vec3 worldOffset;
	Vec3 contactNormal;
	float contact;
	float lastW;
	float w;
	bool locked;				// Handbraking

	float slipSpeed;

	float suspLen;
	float compression;
};


/*
	Chassis
*/
struct SVehicleChassis
{
	float radius;
    float mass;
	float inertia;

	float invMass;
	float invInertia;

	Vec3 vel;
	Vec3 angVel;

	int contactIdx0, contactIdx1, contactIdx2, contactIdx3;		// These are used to calculate the base contact normal, and are indexes to the wheels that used

	bool collision1;
	bool collision2;
};

/*
	Gears
*/
struct SVehicleGears
{
	// Const
	enum {kMaxGears = 10};
	enum {kReverse=0, kNeutral, kFirst};
	float ratios[kMaxGears];
	float invRatios[kMaxGears];
	int numGears;
	float minChangeUpTime;
	float minChangeDownTime;
	float averageWheelRadius;

	// Dynamic
	int curGear;
	float curRpm;
	int accelerating;
	float targetRpm;
	float timer;
};


class CVehicleMovementArcadeWheeled
	: public CVehicleMovementBase
{
	//friend class CNetworkMovementArcadeWheeled;
public:
	enum {maxWheels=10};

public:
	CVehicleMovementArcadeWheeled();
	~CVehicleMovementArcadeWheeled();

	// IVehicleMovement
	virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
	virtual void PostInit();
	virtual void Reset();
	virtual void Release();
	virtual void Physicalize();
	virtual void PostPhysicalize();

	virtual EVehicleMovementType GetMovementType() { return eVMT_Land; } 

	virtual bool StartEngine(EntityId driverId);  
	virtual void StopEngine();
	virtual void OnEvent(EVehicleMovementEvent event, const SVehicleMovementEventParams& params);
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);

	virtual void ProcessAI(const float deltaTime);
	virtual void ProcessMovement(const float deltaTime);
	virtual void Update(const float deltaTime);  
	virtual void UpdateSounds(const float deltaTime);

	virtual bool RequestMovement(CMovementRequest& movementRequest);
	virtual void GetMovementState(SMovementState& movementState);

	virtual pe_type GetPhysicalizationType() const { return PE_WHEELEDVEHICLE; };
	virtual bool UseDrivingProxy() const { return true; };
	virtual int GetWheelContacts() const { return m_wheelContacts; }
	virtual int GetBlownTires() const { return m_blownTires; }

	void GetMemoryStatistics(ICrySizer * s);

	virtual void Serialize(TSerialize ser, EEntityAspects aspects);
	virtual void SetAuthority( bool auth )
	{
		// m_netActionSync.CancelReceived();
	};
	// ~IVehicleMovement

	// CVehicleMovementBase
	virtual void OnValuesTweaked();
	// ~CVehicleMovementBase

	// IVehicleObject
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

protected:

	virtual bool InitPhysics(const CVehicleParams& table);
	virtual void InitSurfaceEffects();  

	virtual void UpdateSuspension(const float deltaTime);
	void UpdateSuspensionSound(const float deltaTime);
	void UpdateBrakes(const float deltaTime);

	virtual void UpdateSurfaceEffects(const float deltaTime);
	virtual void UpdateGameTokens(const float deltaTime);

	virtual void Boost(bool enable);
	virtual void ApplyBoost(float speed, float maxSpeed, float strength, float deltaTime);

	virtual bool DoGearSound();
	virtual float GetMinRPMSoundRatio() { return 0.f; }

	virtual void DebugDrawMovement(const float deltaTime);

	float GetMaxSteer(float speedRel);
	float GetSteerSpeed(float speedRel);

	virtual float GetWheelCondition() const;
	void SetEngineRPMMult(float mult, int threadSafe=0);

	float CalcSteering(float steer, float speedRel, float rotateYaw, float dt);
	void TickGears(float dt, float averageWheelSpeed, float throttle, float forwardSpeed);
	void InternalPhysicsTick(float dt);
	void GetCurrentWheelStatus(IPhysicalEntity* pPhysics);

#if !defined(_RELEASE)
	void DebugCheat(float dt);
#endif

protected:

	pe_params_car m_carParams;
	pe_status_vehicle m_vehicleStatus;	
	pe_action_drive m_action;

	float m_engineMaxRPM, m_engineIdleRPM, m_engineShiftDownRPM;

	float m_steerMax;             // max steering angle in deg
	float m_steerSpeedMin;        // steer speed at v=0
	float m_steerSpeed;           // steer speed at vMaxSteerMax	
	float m_v0SteerMax;           // max steering angle in deg at v=0
	float m_kvSteerMax;           // reduces steer max at vMaxSteerMax
	float m_steerSpeedScaleMin;   // scale for sens at zero vel
	float m_steerSpeedScale;      // scale for sens at vMaxSteerMax

	float m_steerRelaxation;      // relaxation speed to center in degrees
	float m_vMaxSteerMax;         // speed at which entire kvSteerMax is substracted from v0SteerMax		
	float m_pedalLimitMax;        // at vMaxSteerMax pedal is clamped to 1-pedalLimitMax

	float m_rpmTarget;
	float m_rpmRelaxSpeed, m_rpmInterpSpeed, m_rpmGearShiftSpeed;	
	float m_lastBump, m_compressionMax;
	float m_bumpMinSusp, m_bumpMinSpeed, m_bumpIntensityMult;
	float m_load, m_rpmScalePrev;
	float m_airbrakeTime;
	float m_brakeTimer;
	float m_reverseTimer;
	bool m_initialHandbreak;

	bool m_gearSoundPending;
	bool m_isBreakingOnIdle;  
	float m_suspDampingMin, m_suspDampingMax, m_suspDamping, m_suspDampingMaxSpeed;  
	float m_stabiMin, m_stabiMax, m_stabi;
	float m_speedSuspUpdated;    

	// Network related
	//CNetActionSync<CNetworkMovementArcadeWheeled> m_netActionSync;

	float m_lostContactTimer;
	float m_tireBlownTimer;
	float m_forceSleepTimer;
	bool  m_bForceSleep;

	SVehicleGears m_gears;

	float m_latFriction;
	int8 m_wheelContacts;
	int8 m_passengerCount;
	int8 m_currentGear;
	int8 m_blownTires;
	int   m_lastDebugFrame;

	struct Handling
	{
		float acceleration;
		float decceleration;
		float handBrakeDecceleration;
		float handBrakeDeccelerationPowerLock;
		float topSpeed;
		float reverseSpeed;

		float grip1;			// Grip fraction at zero slip speed
		float grip2;			// Grip fraction at high slip speed (usually 1.f)
		float gripK;			// 1.f/slipSpeed

		float accelMultiplier1;
		float accelMultiplier2;

		float reductionAmount;
		float reductionRate;

		float backFriction;
		float frontFriction;
		float frictionOffset;

		bool handBrakeLockFront;
		bool handBrakeLockBack;
		float handBrakeFrontFrictionScale;
		float handBrakeBackFrictionScale;
		float handBrakeAngCorrectionScale;
		float handBrakeLateralCorrectionScale;

		float compressionScale;
		float compressionBoost;
		float compressionBoostHandBrake;

		float handBrakeRotationDeadTime;	// Power Slide
		float handBrakePowerSlideTimer;
		bool canPowerSlide;
		float powerSlideDir;

		// dynamic
		Vec3 contactNormal;
	};

	Handling m_handling;

	struct Correction
	{
		float lateralSpring;
		float angSpring;
	};

	Correction m_correction;

	float m_damageRPMScale;

	SVehicleChassis m_chassis;
	SVehicleWheel   m_wheels[maxWheels];
	pe_status_wheel m_wheelStatus[maxWheels];
	int             m_wheelStatusLock;
	int             m_numWheels;
	float           m_invNumWheels;

	float m_invTurningRadius;

	//------------------------------------------------------------------------------
	// AI related
	// PID controller for speed control.	

	float	m_steering;
	float	m_prevAngle;

	CMovementRequest m_aiRequest;

	float m_submergedRatioMax;	// to avoid calling vehicle functions in ProcessMovement()

	bool m_netLerp;				// This is true when the vehicle is not locally controlled by the player
};

#endif
