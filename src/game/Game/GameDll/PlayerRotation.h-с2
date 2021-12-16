// takes input rotations and movement requests, and produces physical impulses/turns based upon current player state

#ifndef __PLAYERROTATION_H__
#define __PLAYERROTATION_H__

#pragma once

#include "Player.h"
#include "GameCVars.h"
#include "AutoAimManager.h"

class CPlayerRotation
{
private:
	enum ESnapTarget
	{
		eSnapTarget_None = 0,
		eSnapTarget_Primary, 
		eSnapTarget_Secondary,
		eSnapTarget_OnTarget,
	};

public:
	CPlayerRotation(
		const CPlayer& player,
		const SActorFrameMovementParams& movement,
		const SAimAccelerationParams& horizontalAcceleration,
		const SAimAccelerationParams& verticalAcceleration,
		float frameTime);
	void Process();
	void Commit( CPlayer& player );

private:
	const float m_frameTime;
	const SPlayerParams& m_params;
	const SPlayerStats& m_stats;
	const CPlayer& m_player;
	const int m_actions;

	Ang3 m_deltaAngles;
	
	void GetStanceAngleLimits(float & minAngle,float & maxAngle);
	ILINE float GetLocalPitch()
	{
		return asin((m_baseQuat.GetInverted() * m_viewQuat.GetColumn1()).z);
	}
	
	void ProcessNormalRoll();
	void ProcessAngularImpulses();
	void ProcessNormal();
	void ProcessLeanAndPeek();
	void ProcessSliding();

	bool ShouldProcessTargetAssistance() const;
	void ProcessTargetAssistance();
	void TargetAimAssistance(float& followH, float& followV, float& scale);

	void ClampAngles();

	Vec3 GetSnapTargetDirection(EntityId snapTargetId) const;

	Vec3 PickBestSnapDirection(const Vec3& aimPos, const Vec3& aimDirection, const Vec3& targetDir, const SAutoaimTarget& aaTarget, const float onTargetWeight);

	const SAimAccelerationParams& m_horizontalAcceleration;
	const SAimAccelerationParams& m_verticalAcceleration;

  Ang3 m_viewAngles;
	Quat m_viewQuat;
	Quat m_viewQuatFinal;
	Quat m_baseQuat;

	Quat m_baseQuatLinked;
	Quat m_viewQuatLinked;

	EntityId m_follow_target_id;
	Vec3 m_follow_target_dir;
	Vec3 m_snap_target_dir;

	float m_viewRoll;
	Vec3 m_upVector;
	Ang3 m_viewAnglesOffset;
	float m_leanAmount;	  
	float	m_desiredLeanAmount;
	float	m_desiredPeekOverAmount;
	float m_peekOverAmount;

	// angular impulses - from stats
	float m_angularImpulseTime;
	Ang3 m_angularImpulse;
	Ang3 m_angularVel;
	// internal working vars
	Ang3 m_angularImpulseDelta;
	Vec3 m_desiredVelocity;

	int8 m_desiredAimSnapId;
	static int8 s_lastAimSnapId;
	static bool s_lastTimeRequestedSnapping;
};

/*
CPlayerRotation movementProcessing(*this);
movementProcessing.Process();
movementProcessing.Commit(*this);
*/

#endif
