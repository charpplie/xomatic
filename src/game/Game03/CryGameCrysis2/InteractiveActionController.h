/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Controls player when playing an interactive animation

-------------------------------------------------------------------------
History:
- 19:12:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _INTERACTIVE_ACTION_CONTROLLER_H_
#define _INTERACTIVE_ACTION_CONTROLLER_H_

class CPlayer;
struct SActorFrameMovementParams;
struct SCharacterMoveRequest;

class CInteractiveActionController
{
	enum EState
	{
		eState_None = 0,
		eState_PositioningUser,
		eState_PlayingAnimation,
		eState_Done
	};

public:
	CInteractiveActionController()
		: m_pControlledPlayer(NULL)
		, m_interactiveObjectId(0)
		, m_state(eState_None)
		, m_targetLocation(IDENTITY, ZERO)
		, m_runningTime(0.0f)
		, m_remainingAnimationTime(0.0f)
		, m_interactionName("")
		, m_correctionDelay(0.4f)
	{

	}

	ILINE void InitWithPlayer(CPlayer* pPlayer) { assert(m_pControlledPlayer == NULL); m_pControlledPlayer = pPlayer; }
	ILINE bool HasFinished() const { return m_state == eState_Done; }

	void OnEnter(EntityId interactiveObjectId);
	void OnEnterByName(const char* interaction);
	void OnLeave();
	void Update(float frameTime, const SActorFrameMovementParams& movement);

	
private:

	EState CorrectUserPosition(float frameTime, const SActorFrameMovementParams& movement, SCharacterMoveRequest& movementRequest);
	EState UpdateAnimationState(float frameTime, const SActorFrameMovementParams& movement, SCharacterMoveRequest& movementRequest);
	void DoMovementRequest(const SCharacterMoveRequest& movementRequest);
	void NotifyFinished();

	void StartAnimations();
	void CalculateStartTargetLocation();

	CryFixedStringT<32> m_interactionName;
	QuatT				m_targetLocation;
	CPlayer*		m_pControlledPlayer;
	EntityId		m_interactiveObjectId;
	float				m_runningTime;
	float				m_remainingAnimationTime;
	float				m_correctionDelay;
	EState			m_state;
};

#endif //_INTERACTIVE_ACTION_CONTROLLER_H_