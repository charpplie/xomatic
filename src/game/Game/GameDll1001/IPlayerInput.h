#ifndef __IPLAYERINPUT_H__
#define __IPLAYERINPUT_H__

#pragma once

#include "IAgent.h" // for EStance
#include "ISerialize.h"
#include "IGameObject.h"

struct SSerializedPlayerInput
{
	uint8 stance;
	uint8 bodystate;
	Vec3 deltaMovement;
	Vec3 lookDirection;
	Vec3 bodyDirection;
	bool sprint;
	bool leanl;
	bool leanr;
	bool aiming;
	bool usinglookik;
	bool allowStrafing;
	float pseudoSpeed;

	SSerializedPlayerInput() :
		stance((uint8)STANCE_NULL),
		bodystate(0),
		deltaMovement(ZERO),
		lookDirection(FORWARD_DIRECTION),
		bodyDirection(FORWARD_DIRECTION),
		sprint(false),
		leanl(false),
		leanr(false),
		aiming(false),
		usinglookik(false),
		allowStrafing(true),
		pseudoSpeed(1.0f)
	{
	}

	void Serialize( TSerialize ser )
	{
		ser.Value( "stance", stance, 'stnc' );
		ser.Value( "bodystate", bodystate, 'stnc');
		// note: i'm not sure what some of these parameters mean, but i copied them from the defaults in serpolicy.h
		// however, the rounding mode for this value must ensure that zero gets sent as a zero, not anything else, or things break rather badly
		ser.Value( "deltaMovement", deltaMovement, 'pMov' );
		ser.Value( "lookDirection", lookDirection, 'dir0' );
		ser.Value( "sprint", sprint, 'bool' );
		ser.Value( "leanl", leanl, 'bool' );
		ser.Value( "leanr", leanr, 'bool' );
		ser.Value( "aiming", aiming, 'bool' );
		ser.Value( "usinglookik", usinglookik, 'bool' );
		ser.Value( "allowStrafing", allowStrafing, 'bool' );
		ser.Value( "pseudoSpeed", pseudoSpeed, 'sone' );
		//ser.Value("ActionMap", actionMap, NSerPolicy::A_JumpyValue(0.0f, 127.0f, 7));
	}
};

class CCameraInputHelper;
struct IPlayerInput
{
	static const EEntityAspects INPUT_ASPECT = eEA_GameClientDynamic;

	enum EInputType
	{
		PLAYER_INPUT,
		NETPLAYER_INPUT,
		AI_INPUT,
		DEDICATED_INPUT,
	};

	virtual ~IPlayerInput() {};

	virtual void PreUpdate() = 0;
	virtual void Update() = 0;
	virtual void PostUpdate() = 0;

	virtual void OnAction( const ActionId& action, int activationMode, float value ) = 0;

	virtual void SetState( const SSerializedPlayerInput& input ) = 0;
	virtual void GetState( SSerializedPlayerInput& input ) = 0;

	virtual void Reset() = 0;
	virtual void DisableXI(bool disabled) = 0;

	virtual EInputType GetType() const = 0;

	virtual void GetMemoryStatistics(ICrySizer * s) = 0;

	//this helper computes mode transitions and camera pitch
	virtual CCameraInputHelper *GetCameraInputHelper() const { return NULL; }

	virtual uint32 GetMoveButtonsState() const = 0;
	virtual uint32 GetActions() const = 0;

	virtual void SerializeSaveGame( TSerialize ser ) {};
};

#endif
