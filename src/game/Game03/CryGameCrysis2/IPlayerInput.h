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
	bool isDirty;
	float pseudoSpeed;

	SSerializedPlayerInput() :
		stance(STANCE_STAND),
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
		isDirty(false),
		pseudoSpeed(0.0f)
	{
	}

	void SerializeDir( TSerialize ser, Vec3& dir)
	{
		// Serialise look direction as yaw and elev. Gives better compression 
		// (Perhaps we should create a directional compression policy in the network layer?)
		// (This engine doesn't seem to have any polar conversions)
		float yaw, elev;
		
		if (!ser.IsReading())
		{
			float xy = dir.GetLengthSquared2D();
			if (xy > 0.001f)
			{
				yaw = atan2_tpl(dir.y,dir.x);
				elev = asin_tpl(clamp(dir.z, -1.f, +1.f));
			}
			else
			{
				yaw = 0.f;
				elev = (float)__fsel(dir.z, +1.f, -1.f) * (gf_PI*0.5f);
			}
		}

		ser.Value("playerLookYaw", yaw, 'pYaw');
		ser.Value("playerLookElev", elev, 'pElv');

		if (ser.IsReading())
		{
			float sinYaw, cosYaw;
			float sinElev, cosElev;

			sincos_tpl(yaw, &sinYaw, &cosYaw);
			sincos_tpl(elev, &sinElev, &cosElev);

			dir.x = cosYaw * cosElev;
			dir.y = sinYaw * cosElev;
			dir.z = sinElev;
		}
	}

	void Serialize( TSerialize ser )
	{
		ser.Value( "stance", stance, 'stnc' );
		//ser.Value( "bodystate", bodystate, 'bdst');
		// note: i'm not sure what some of these parameters mean, but i copied them from the defaults in serpolicy.h
		// however, the rounding mode for this value must ensure that zero gets sent as a zero, not anything else, or things break rather badly
		ser.Value( "deltaMovement", deltaMovement, 'pMov' );
//		ser.Value( "lookDirection", lookDirection, 'dir0' );
		SerializeDir(ser, lookDirection);
		ser.Value( "sprint", sprint, 'bool' );
//		ser.Value( "leanl", leanl, 'bool' );
//		ser.Value( "leanr", leanr, 'bool' );
		//ser.Value( "aiming", aiming, 'bool' );
		//ser.Value( "usinglookik", usinglookik, 'bool' );
		//ser.Value( "allowStrafing", allowStrafing, 'bool' );
		//ser.Value( "pseudoSpeed", pseudoSpeed, 'unit' );
		//ser.Value("ActionMap", actionMap, NSerPolicy::A_JumpyValue(0.0f, 127.0f, 7));
	}
};

struct IPlayerInput
{
	enum EInputType
	{
		PLAYER_INPUT,
		NETPLAYER_INPUT,
		AI_INPUT,
		DEDICATED_INPUT,
		MINDCONTROL_INPUT,
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
	virtual void ClearXIMovement() = 0;

	virtual EInputType GetType() const = 0;
	
	virtual uint32 GetMoveButtonsState() const = 0;
	virtual uint32 GetActions() const = 0;

	virtual float GetLastRegisteredInputTime() const = 0;
	virtual void SerializeSaveGame( TSerialize ser ) = 0;

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const =0;
};

#endif
