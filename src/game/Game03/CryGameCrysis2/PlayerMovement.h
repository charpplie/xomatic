// takes input rotations and movement requests, and produces physical impulses/turns based upon current player state

#ifndef __PLAYERMOVEMENT_H__
#define __PLAYERMOVEMENT_H__

#pragma once

#include "Player.h"

class CPlayerMovement
{
public:
	// NOTE: Removed const from player, so that PlayerMovement ZeroG can play sounds through it.
	CPlayerMovement(CPlayer& player, const SActorFrameMovementParams& movement, float frameTime );
	void Process( CPlayer& player );
	void Commit( CPlayer& player );

	void AddVelocity( const Vec3 &velocityDelta );

private:
	const float m_frameTime;
	const SPlayerParams& m_params;
	const SPlayerStats& m_stats;
	SActorFrameMovementParams m_movement;
	CPlayer& m_player;
	const Quat& m_viewQuat;
	const Quat& m_baseQuat;
	const Vec3& m_upVector;
	Vec3 m_worldPos;
	const int m_actions;
	const CPlayer::EActionFlags m_actionFlags;

	void ProcessFlyMode();
	void ProcessFlyingZeroG();
	void ProcessSwimming();
	void ProcessOnGroundOrJumping( CPlayer& player );
	bool StartJump(Vec3& jumpVec, Vec3& move, const Matrix34& baseMtxZ);
	void ProcessTurning();
	Vec3 CalculateVelocityWithAirFrictionPerkActivated(const Vec3& desiredVelocity);
	void ProcessSliding();

	void ProcessMovementOnLedge();
	bool ShouldGetOutBoost(Vec3& entityPos);

	void AdjustMovementForEnvironment( Vec3& movement, bool sprinting );

	bool IsMovingForward() const;
	bool ShouldContinueSliding() const;
	bool ShouldSprint() const;
	bool ShouldProcessTurning() const;

	const Vec3 CalculateInAirJumpExtraVelocity(const Vec3& desiredVelocity) const;

	SCharacterMoveRequest m_request; // our primary output... how to move!
	
	Vec3 m_velocity; // from CPlayer... gets updated here and committed
	
	float m_stickySurfaceTimer;

	bool m_hasJumped;
	bool m_swimJumping;
	bool m_jumped;
};


#endif
