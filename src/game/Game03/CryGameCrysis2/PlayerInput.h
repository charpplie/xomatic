// handles turning actions into CMovementRequests and setting player state
// for the local player

#ifndef __PLAYERINPUT_H__
#define __PLAYERINPUT_H__

#pragma once

#include "IActionMapManager.h"
#include "IPlayerInput.h"
#include "Player.h"

class CPlayer;
struct SPlayerStats;

#define MAX_FREE_CAM_DATA_POINTS 20

//#if defined(_RELEASE)
//#define FREE_CAM_SPLINE_ENABLED 0
//#else
#define FREE_CAM_SPLINE_ENABLED 1
//#endif


class CPlayerInput : public IPlayerInput, public IActionListener
{
public:

#if FREE_CAM_SPLINE_ENABLED
	struct SFreeCamPointData
	{
		SFreeCamPointData()
			: valid(false), distanceFromLast(0.f)
		{ }

		bool valid;
		Vec3 position;
		Vec3 lookAtPosition;
		float distanceFromLast;
	};

	SFreeCamPointData m_freeCamData[MAX_FREE_CAM_DATA_POINTS];

	typedef spline::CatmullRomSpline<Vec3> TFreeCamSpline;

	TFreeCamSpline m_freeCamSpline;
	TFreeCamSpline m_freeCamLookAtSpline;

	bool m_freeCamPlaying;
	int m_freeCamCurrentIndex;
	float m_freeCamPlayTimer;
	float m_freeCamTotalPlayTime;
#endif

	enum EMoveButtonMask
	{
		eMBM_Forward	= (1 << 0),
		eMBM_Back			= (1 << 1),
		eMBM_Left			= (1 << 2),
		eMBM_Right		= (1 << 3)
	};

	CPlayerInput( CPlayer * pPlayer );
	~CPlayerInput();

	// IPlayerInput
	virtual void PreUpdate();
	virtual void Update();
	virtual void PostUpdate();
	virtual void SetState( const SSerializedPlayerInput& input );
	virtual void GetState( SSerializedPlayerInput& input );
	virtual void Reset();
	virtual void DisableXI(bool disabled);
	virtual void ClearXIMovement();
	virtual void GetMemoryUsage(ICrySizer * s) const {s->Add(*this);}
	virtual EInputType GetType() const { return PLAYER_INPUT;	};
	ILINE virtual uint32 GetMoveButtonsState() const { return m_moveButtonState; }
	ILINE virtual uint32 GetActions() const { return m_actions; }
	virtual float GetLastRegisteredInputTime() const { return m_lastRegisteredInputTime; }
	virtual void SerializeSaveGame( TSerialize ser );
	ILINE Ang3& GetRawControllerInput() { return m_xi_deltaRotationRaw; }

	// ~IPlayerInput

	// IActionListener
	virtual void OnAction( const ActionId& action, int activationMode, float value );
	// ~IActionListener

	void ClearCrouchAction() {m_actions &= ~ACTION_CROUCH;}
	void ClearAllExceptAction(uint32 actionFlags);
	
	void AddFlyCamPoint();
	void AddFlyCamPoint(Vec3 pos, Vec3 lookAtPos);
	void FlyCamPlay();

private:

	static void GetCCTVEntityRotationLimits(EntityId cctvEid, float *outLimitV, float *outLimitH);

	void CCTVSpectatorPreUpdate(const Ang3* deltaRotation);
	void HandleMovingDetachedCamera(const Ang3 &deltaRotation, const Vec3 &m_deltaMovement);
	EStance FigureOutStance();
	void AdjustMoveButtonState( EMoveButtonMask buttonMask, int activationMode );
	bool CheckMoveButtonStateChanged( EMoveButtonMask buttonMask, int activationMode );
	float MapControllerValue(float value, float scale, float curve, bool inverse);
	Ang3 UpdateXIInputs(const Ang3& inputAngles, bool bScaling = true);

	void ApplyMovement(Vec3 delta);
	const Vec3 &FilterMovement(const Vec3 &desired);

	bool CanMove() const;

	void OpenSuitMenu();

	bool OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionVRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value); // needed so player can shake unfreeze while in a vehicle
	bool OnActionVRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionJumpOrGlide(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionDeployAir(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionDeployGround(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionCrouch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSprint(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionLeanLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionLeanRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionUse(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionAttackRightTrigger(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionStealthKill(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	
	// Nanosuit
	bool OnActionSuitMenuOpen(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSuitMenuClose(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSuitMenuCloseNoSelect(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSuitSelectXAxis(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSuitSelectYAxis(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSwitchSuitMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSuitPower(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	// Cheats
	bool OnActionThirdPerson(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionGodMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionAIDebugDraw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionPDrawHelpers(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionDMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionXIRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIDisconnect(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionInvertMouse(EntityId entityId, const ActionId& actionId, int activationMode, float value);
		
	bool OnActionFlyCamMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamMoveUp(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamMoveDown(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamSpeedUp(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamSpeedDown(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamTurbo(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamSetPoint(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamPlay(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyCamClear(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionPerks(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMap(EntityId entityId, const ActionId& actionId, int activationMode, float value);

private:

	void NormalizeInput(float& fX, float& fY, float fCoeff, float fCurve);
	void DrawDebugInfo();
	void GetMappingParameters(float& fCoeff, float& fCurve) const;

	void SetSliding(bool set);
	void UpdateSuitSelectionInputs(float dt);
	void ClearSuitSelectionInputs();
	void ChooseSelectedSuitMode();
	ENanoSuitMode GetSuitModeFromAxes();

	bool PerformJump(bool quickPress);

	ENanoSuitMode m_lastHighlightedMode;

	CPlayer* m_pPlayer;

	uint32 m_actions;
	uint32 m_lastActions;

	CPlayer::EActionFlags m_actionFlags;

	Vec3 m_deltaMovement;
	Vec3 m_xi_deltaMovement;
	
	Ang3 m_deltaRotation;
	Ang3 m_xi_deltaRotation;
	Ang3 m_xi_deltaRotationRaw;

	Vec3 m_flyCamDeltaMovement;
	Ang3 m_flyCamDeltaRotation;
	bool m_flyCamTurbo;

	float m_speedLean;
	float m_jumpPressTime;
	
	uint32 m_moveButtonState;
	Vec3 m_filteredDeltaMovement;

	int m_lastSerializeFrameID;

	float m_suitSelectInputX;
	float m_suitSelectInputY;
	float m_filteredSuitSelectInputX;
	float m_filteredSuitSelectInputY;

	float m_lastSensitivityFactor;
	float m_lastRegisteredInputTime;
	float m_suitMenuOpenButtonPressedTime;
	
	bool m_requestSuperJump;
	bool m_bDisabledXIRot;
	bool m_bUseXIInput;

	bool m_suitMenuOpen;

	struct SDebugDrawStats
	{
		SDebugDrawStats() : lastRaw(ZERO), lastProcessed(ZERO), vOldAimPos(ZERO) {}

		Ang3 lastRaw;
		Ang3 lastProcessed;
		Vec3 vOldAimPos;
	} m_debugDrawStats;

	static TActionHandler<CPlayerInput>	s_actionHandler;
};




class CAIInput : public IPlayerInput
{
public:
	enum EMoveButtonMask
	{
		eMBM_Forward	= (1 << 0),
		eMBM_Back			= (1 << 1),
		eMBM_Left			= (1 << 2),
		eMBM_Right		= (1 << 3)
	};

	CAIInput( CPlayer * pPlayer );
	~CAIInput();

	// IPlayerInput
	virtual void PreUpdate() {};
	virtual void Update() {};
	virtual void PostUpdate() {};

	virtual void OnAction( const ActionId& action, int activationMode, float value ) {};

	virtual void SetState( const SSerializedPlayerInput& input );
	virtual void GetState( SSerializedPlayerInput& input );

	virtual void Reset() {};
	virtual void DisableXI(bool disabled) {};
	virtual void ClearXIMovement() {};

	ILINE virtual uint32 GetMoveButtonsState() const { return 0; }
	ILINE virtual uint32 GetActions() const { return 0; }

	virtual void GetMemoryUsage(ICrySizer * s) const {s->Add(*this);}

	virtual EInputType GetType() const
	{
		return AI_INPUT;
	};

	virtual float GetLastRegisteredInputTime() const { return 0.0f; }
	virtual void SerializeSaveGame( TSerialize ser ) {}
	// ~IPlayerInput

private:
	CPlayer* m_pPlayer;
	SPlayerStats* m_pStats;
	Vec3 m_deltaMovement;
};

#endif
