/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements wheel based vehicle movement

-------------------------------------------------------------------------
History:
- Created by stan fichele
- Adjusted for SDK: Sascha Hoba

*************************************************************************/
#ifndef __VEHICLEMOVEMENTARCADEWHEELED_H__
#define __VEHICLEMOVEMENTARCADEWHEELED_H__

#define ENABLE_VEHICLE_DEBUG 1

#include "VehicleMovementBase.h"
#include "VehicleMovementSharedParams.h"

/*
	Wheel
*/
struct SVehicleWheel
{
	inline SVehicleWheel()
	{
		offset					= Vec3(0.0f, 0.0f, 0.0f);
		frictionDir[0]			= Vec3(0.0f, 0.0f, 0.0f);
		frictionDir[1]			= Vec3(0.0f, 0.0f, 0.0f);
		worldOffset				= Vec3(0.0f, 0.0f, 0.0f);
		contactNormal			= Vec3(0.0f, 0.0f, 0.0f);
		wheelPart				= NULL;
		radius					= 0.0f;
		mass					= 0.0f;
		invMass					= 0.0f;
		inertia					= 0.0f;
		invInertia				= 0.0f;
		bottomOffset			= 0.0f;
		contact					= 0.0f;
		lastW					= 0.0f;
		w						= 0.0f;
		slipSpeed				= 0.0f;
		suspLen					= 0.0f;
		compression				= 0.0f;
		waterLevel				= WATER_LEVEL_UNKNOWN;
		axleIndex				= 0;
		bCanLock				= 0;
		locked					= false;
	}

	Vec3 offset;
	Vec3 frictionDir[2];				// [0] == inline friction direction, [1] == lateral friction direction
	Vec3 worldOffset;
	Vec3 contactNormal;

	IVehiclePart* wheelPart;

	float radius;
	float mass;
	float invMass;
	float inertia;
	float invInertia;
	float bottomOffset;					// offset + (zAxis * bottomOffset) == bottom point of wheel
	float contact;
	float lastW;
	float w;
	float slipSpeed;
	float suspLen;
	float compression;
	float	waterLevel;

	int axleIndex;						// 0 == back wheel, 1 == front wheel
	int bCanLock;						// 0 or 1

	bool locked;						// Handbraking
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

class CVehicleMovementArcadeWheeled
	: public CVehicleMovementBase
{
	//friend class CNetworkMovementArcadeWheeled;
public:
	enum {maxWheels=10};
	enum {k_frictionNotSet, k_frictionUseLowLevel, k_frictionUseHiLevel}; // Friction State

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

	virtual void GetMemoryStatistics(ICrySizer * pSizer);

	virtual void Serialize(TSerialize ser, EEntityAspects aspects);
	virtual void SetAuthority( bool auth ){};
	// ~IVehicleMovement

	// IVehicleObject
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

protected:

	virtual bool InitPhysics(const CVehicleParams& table);
	virtual void InitSurfaceEffects();  

	virtual void UpdateSuspension(const float deltaTime);
	void UpdateSuspensionSound(const float deltaTime);
	void UpdateBrakes(const float deltaTime);

	virtual void UpdateSurfaceEffects(const float deltaTime);

	virtual void Boost(bool enable);
	virtual void ApplyBoost(float speed, float maxSpeed, float strength, float deltaTime);

	virtual bool DoGearSound();
	virtual float GetMinRPMSoundRatio() { return 0.f; }

#if ENABLE_VEHICLE_DEBUG
	virtual void DebugDrawMovement(const float deltaTime);
#endif

	float GetMaxSteer(float speedRel);
	float GetSteerSpeed(float speedRel);

	virtual float GetWheelCondition() const;
	void SetEngineRPMMult(float mult, int threadSafe=0);

	float CalcSteering(float steer, float speedRel, float rotateYaw, float dt);
	void TickGears(float dt, float averageWheelSpeed, float throttle, float forwardSpeed);
	void EnableLowLevelPhysics(int state, int bThreadSafe);
	void InternalPhysicsTick(float dt);
	void GetCurrentWheelStatus(IPhysicalEntity* pPhysics);

#if ENABLE_VEHICLE_DEBUG
	void DebugCheat(float dt);
#endif

	void UpdateWaterLevels();
	void ResetWaterLevels();
	void UpdateSharedParams(const char* sharedParamsName, const CVehicleParams& table);
	void CreateDefaultParams(const char* sharedParamsName, const CVehicleParams& params);
	void SetSharedParams(const CVehicleParams& table, CVehicleMovementSharedParams& sharedParams);

protected:
	friend class CVehicleMovementSharedParams;

	pe_params_car m_carParams;
	pe_status_vehicle m_vehicleStatus;	
	pe_action_drive m_action;

	float m_steerMax;	// max steering angle in deg
	float m_rpmTarget;
	float m_lastBump, m_compressionMax;	
	float	m_rpmScalePrev;
	float m_brakeTimer;
	float m_reverseTimer;
	bool m_initialHandbreak;

	bool m_gearSoundPending;

	float m_suspDamping;
	float m_stabi;
	float m_speedSuspUpdated;    

	float m_lostContactTimer;
	float m_tireBlownTimer;
	float m_forceSleepTimer;
	bool  m_bForceSleep;

	SVehicleGears m_gears;

	int8 m_wheelContacts;
	int8 m_passengerCount;
	int8 m_currentGear;
	int8 m_blownTires;
	int   m_lastDebugFrame;

	struct Handling
	{
		float	compressionScale;
		float	handBrakePowerSlideTimer;
		bool	canPowerSlide;
		float	powerSlideDir;
		Vec3	contactNormal;
	};

	Handling m_handling;

	float m_damageRPMScale;

	SVehicleChassis m_chassis;

	typedef std::vector<SVehicleWheel> TWheelArray;
	typedef std::vector<pe_status_wheel> TPEStatusWheelArray;

	TWheelArray			m_wheels;
	TPEStatusWheelArray	m_wheelStatus;
	size_t				m_iWaterLevelUpdate;
	
	int					m_wheelStatusLock;
	int					m_numWheels;
	float				m_invNumWheels;

	float m_invTurningRadius;

	//------------------------------------------------------------------------------
	// AI related
	// PID controller for speed control.	

	float	m_steering;
	float	m_prevAngle;

	CMovementRequest m_aiRequest;

	float m_submergedRatioMax;	// to avoid calling vehicle functions in ProcessMovement()

	int m_frictionState;
	bool m_netLerp;				// This is true when the vehicle is not locally controlled by the player

	_smart_ptr<class CVehicleMovementSharedParams> m_pSharedParams;
};

#endif