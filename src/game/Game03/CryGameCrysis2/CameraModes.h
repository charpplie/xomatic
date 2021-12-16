/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Camera mode implementations

-------------------------------------------------------------------------
History:
- 16:10:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _CAMERA_MODES_H_
#define _CAMERA_MODES_H_

#include "ICameraMode.h"

struct SPlayerStats;



class CDefaultCameraMode : public ICameraMode
{
private:

	struct CCameraPose
	{
	public:
		CCameraPose()
			:	m_position(ZERO)
			,	m_rotation(IDENTITY) {}
		CCameraPose(Vec3 positionOffset, Quat rotationOffset)
			:	m_position(positionOffset)
			,	m_rotation(rotationOffset) {}
		CCameraPose(const CCameraPose& copy)
			:	m_position(copy.m_position)
			,	m_rotation(copy.m_rotation) {}

		static CCameraPose Compose(const CCameraPose& lhv, const CCameraPose& rhv)
		{
			return CCameraPose(
				lhv.GetPosition() + rhv.GetPosition(),
				lhv.GetRotation() * rhv.GetRotation());
		}

		static CCameraPose Scale(const CCameraPose& lhv, float rhv)
		{
			if (rhv == 1.0f)
				return lhv;
			else
				return CDefaultCameraMode::CCameraPose(
					lhv.GetPosition() * rhv,
					Quat::CreateSlerp(IDENTITY, lhv.GetRotation(), rhv));
		}

		Vec3 GetPosition() const {return m_position;}
		Quat GetRotation() const {return m_rotation;}

	private:
		Vec3 m_position;
		Quat m_rotation;
	};

public:
	CDefaultCameraMode();

	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);

private:
	CCameraPose FirstPersonCameraPose(const CPlayer& clientPlayer, float frameTime);
	CCameraPose FirstPersonCameraAnimation(const CPlayer& clientPlayer, const CCameraPose& firstPersonCamera, float cameraFactor) const;
	CCameraPose ThirdPersonCameraPose(const CPlayer& clientPlayer, const SViewParams& viewParams) const;
	CCameraPose ViewBobing(const CPlayer& clientPlayer, float frameTime);
	CCameraPose MountedGunView(const CPlayer& clientPlayer, const CCameraPose& firstPersonCamera) const;

	float m_bobCycle;
	Vec3 m_bobOffset;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CSpectatorFollowCameraMode : public ICameraMode
{
public:
	CSpectatorFollowCameraMode();
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);

private:

	EntityId m_lastSpectatorTarget;
	int	 m_lastFrameId;
	Vec3 m_viewOffset;
	Vec3 m_position;
	Vec3 m_entityPos;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CSpectatorFixedCameraMode : public ICameraMode
{
public:
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);

};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CAnimationControlledCameraMode : public ICameraMode
{
public:
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CDeathCameraMode : public ICameraMode
{
public:
	CDeathCameraMode();
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);

private:
	int m_lastFrameId;
	EntityId m_lastDeathCamSubject;
	float m_lastFOVScale;
	
	Vec3 m_lastCameraPos;
	Vec3 m_lastTargetPos;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CVehicleCameraMode : public ICameraMode
{
public:
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CPartialAnimationControlledCameraMode : public ICameraMode
{
public:
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);

private:

	float GetBlendOrientationFactor(const SPlayerStats& playerStats) const;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if ENABLE_MINDCONTROL
class CMindControlledCameraMode : public ICameraMode
{
public:
	virtual void UpdateView(const CPlayer& clientPlayer, SViewParams& viewParams, float frameTime);
};
#endif



#endif
