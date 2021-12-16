//---------------------------------------
// Basic actor parameters structure, which stores game parameters (physics, stance, movement...) 
//
// 15-09-2008 Benito G.R.
// ----------------------------------------

#ifndef _BASIC_ACTOR_PARAMETERS_H_
#define _BASIC_ACTOR_PARAMETERS_H_

#include "GameParameters.h"

struct IItemParamsNode;

struct SBasicActorLivingParams
{
	SBasicActorLivingParams():
	gravity(9.81f),
	mass(80.0f),
	player_mass(120.0f),
	air_resistance(0.5f),
	k_air_control(0.9f),
	max_vel_ground(16.0f),
	min_slide_angle(45.0f),
	max_climb_angle(50.0f),
	min_fall_angle(50.0f),
	time_impulse_recover(1.0f),
	inertia(0.0f),
	inertia_accel(0.0f),
	collider_mat("mat_player_collider")
	{

	}

	void ReadParams(const IItemParamsNode *params);

	float	gravity;
	float mass;
	float player_mass;
	float air_resistance;
	float k_air_control;
	float max_vel_ground;
	float min_slide_angle;
	float max_climb_angle;
	float min_fall_angle;
	float time_impulse_recover;
	float inertia;
	float inertia_accel;
	string collider_mat;
};

struct SBasicActorPhysicParams
{
	SBasicActorPhysicParams():
	flags(0),
	mass(80),
	player_mass(120),
	stiffness_scale(73)
	{
	}

	void ReadParams(const IItemParamsNode *params);

	int flags;
	float mass;
	float player_mass;
	float stiffness_scale;

	SBasicActorLivingParams livingParams;
};

struct SBasicActorParams : IGameSharedParameters
{
protected:
	mutable uint32	m_refs;
	bool					m_valid;
public:
	SBasicActorParams(): m_refs(0), m_valid(false) {};
	virtual ~SBasicActorParams() 
	{
		int x = 0;
	};

	//IGameSharedParameters
	virtual void AddRef() const { ++m_refs; };
	virtual uint32 GetRefCount() const { return m_refs; };
	virtual void Release() const { 
		if (--m_refs <= 0)
			delete this;
	};

	virtual bool Valid() const { return m_valid; };
	virtual void SetValid(bool valid) { m_valid=valid; };

	virtual void GetMemoryUsage(ICrySizer *s) const{};

	virtual const char* GetDataType() const { return "BasicActorParams"; }

	SBasicActorPhysicParams m_physicParams;
};

#endif
