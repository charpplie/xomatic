/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
Special IPlayerInput that handles 'mind controlling', the act of temporarily
taking over the control of an otherwise AI-controlled CPlayer.

It tries to mimic the way the AI controls a CPlayer as much as possible.
-------------------------------------------------------------------------
History:
- 22:10:2009: Created by Sven Van Soom

*************************************************************************/

#ifndef __MINDCONTROLINPUT_H__
#define __MINDCONTROLINPUT_H__

#pragma once

#include "IActionMapManager.h"
#include "IPlayerInput.h"
#include "Player.h"

class CPlayer;

class CMindControlInput : public IPlayerInput, public IActionListener
{
public:

	enum EMoveButtonMask
	{
		eMBM_Forward	= (1 << 0),
		eMBM_Back			= (1 << 1),
		eMBM_Left			= (1 << 2),
		eMBM_Right		= (1 << 3)
	};

	enum ECameraMode
	{
		eCM_None,
		eCM_FollowBehind,
		eCM_FollowFront,
		eCM_FollowSide,
		eCM_FollowTop,
		eCM_LookAtSlaveFromPlayer,

		eCM_COUNT,
	};

	CMindControlInput( CPlayer * pPlayer, CPlayer* pMaster );
	~CMindControlInput();

	// IPlayerInput
	virtual void PreUpdate();
	virtual void Update();
	virtual void PostUpdate();
	virtual void SetState( const SSerializedPlayerInput& input );
	virtual void GetState( SSerializedPlayerInput& input );
	virtual void Reset();
	virtual void DisableXI(bool disabled);
	virtual EInputType GetType() const { return MINDCONTROL_INPUT; };
	ILINE virtual uint32 GetMoveButtonsState() const { return 0; } // We do exactly what the does: movebuttons are NOT returned!
	ILINE virtual uint32 GetActions() const { return 0; } // We do exactly what the AIInput does: actions are NOT returned!
	virtual float GetLastRegisteredInputTime() const { return 0; }
	virtual void GetMemoryUsage(ICrySizer * s) const {s->Add(*this);}
	virtual void ClearXIMovement();
	virtual void SerializeSaveGame( TSerialize ser ) {}
	// ~IPlayerInput

	// IActionListener
	virtual void OnAction( const ActionId& action, int activationMode, float value );
	// ~IActionListener

	ECameraMode GetCameraMode() const { return m_cameraMode; }
	Ang3 GetCameraDeltaAngles() const { return m_cameraDeltaAngles; }

private:

	void PreUpdateStance(CMovementRequest& mr);
	void PreUpdateMovementAndRotation(CMovementRequest& mr);
	void PreUpdateAiming(CMovementRequest& mr);
	void PreUpdateLooking(CMovementRequest& mr);
	void PreUpdateBodyTarget(CMovementRequest& mr);

	void AdjustMoveButtonState( EMoveButtonMask buttonMask, int activationMode );
	bool CheckMoveButtonStateChanged( EMoveButtonMask buttonMask, int activationMode );
	void ApplyMovement(Vec3 delta);
	float MapControllerValue(float value, float scale, float curve, bool inverse);
	void UpdateXIRotation();

	bool OnActionSprint(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionCrouch(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionXIMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIDisconnect(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionMenuOpen(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMenuClose(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMenuClick(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionAllowStrafingToggle(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionAttackLeftTrigger(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionAttackRightTrigger(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionToggleMediumWeapon(EntityId entityId, const ActionId& actionId, int activationMode, float value);

private:

	CPlayer* m_pPlayer;
	CPlayer* m_pMaster;

	uint32 m_moveButtonState;

	bool m_bUseXIInput;

	// Moving
	enum EMoveMode
	{
		eMM_MoveTarget,
		eMM_MoveRelative,

		eMM_COUNT,
	};
	EMoveMode m_moveMode;
	Vec3 m_moveTarget;
	bool m_isSprinting; // set only by PreUpdateMovementAndRotation
	bool m_pressedSprintingButton; // set in the action handler
	bool m_allowStrafing; // set in the action handler
	Vec3 m_xi_deltaMovement; // set in the action handler
	Ang3 m_xi_deltaRotationRaw; // set in the action handler
	Ang3 m_xi_deltaRotation; // calculated from m_xi_deltaRotationRaw
	Vec3 m_deltaMovement;

	// Aiming & Firing
	enum EAimMode
	{
		eAM_None,
		eAM_AimAtPlayer,
		eAM_AimAtCamera,
		eAM_AimForward,
		eAM_AimDirection,

		eAM_COUNT,
	};
	EAimMode m_aimMode;
	bool m_suppressAiming;
	Vec3 m_aimTarget; // set only by PreUpdateAiming
	Ang3 m_aimDeltaAngles; // set only by PreUpdateAiming
	bool m_isFiring; // set in the action handler (note that the Weapon code has it's own independent action handler

	// Looking
	enum ELookMode
	{
		eLM_None,
		eLM_LookAtPlayer,
		eLM_LookAtCamera,
		eLM_LookForward,
		eLM_LookDirection,

		eLM_COUNT,
	};
	ELookMode m_lookMode;
	Vec3 m_lookTarget; // set only by PreUpdateLooking
	Ang3 m_lookDeltaAngles; // set only by PreUpdateLooking

	EStance m_stance; // set only by PreUpdateStance
	AgentMovementSpeeds::EAgentMovementUrgency m_agentMovementUrgency; // set only by PreUpdateMovementAndRotation

	// Stance
	AgentMovementSpeeds::EAgentMovementStance m_agentMovementStance; // set in the action handler, player can set this and it will select the stance

	// Camera
	ECameraMode m_cameraMode;
	Ang3 m_cameraDeltaAngles;

	// Menu
	bool m_menuOpen;
	float m_menuOpenness;

private:
	static TActionHandler<CMindControlInput>	s_actionHandler;
};

#endif // __MINDCONTROLINPUT_H__
