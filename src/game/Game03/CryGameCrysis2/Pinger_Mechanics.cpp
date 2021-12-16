//--------------------------------------------
// Based on IMechanic from Actor2 (just for testing...)
//
// 16-08-2008
//--------------------------------------------------

#include "StdAfx.h"
#include "Pinger.h"
#include "Pinger_Mechanics.h"

MyMechanicRegistry* MyMechanicRegistry::m_sThis = NULL;

//Singleton stuff
MyMechanicRegistry& MyMechanicRegistry::ref()
{
	if(m_sThis == NULL)
	{
		m_sThis = new MyMechanicRegistry();
	}

	return (*m_sThis);
}

//Register all mechanics 
void MyMechanicRegistry::Init()
{
	//Register mechanics
	CPingerMechanic_Locomotion* pPM_LM = new CPingerMechanic_Locomotion();
	pPM_LM->Init();
	m_allMechanics.insert(TMechanicsMap::value_type(pPM_LM->GetName(), pPM_LM));
	CPingerMechanic_AnimAction* pPM_AD = new CPingerMechanic_AnimAction();
	pPM_AD->Init();
	m_allMechanics.insert(TMechanicsMap::value_type(pPM_AD->GetName(), pPM_AD));
	CPingerMechanic_Combat* pPM_CB = new CPingerMechanic_Combat();
	pPM_CB->Init();
	m_allMechanics.insert(TMechanicsMap::value_type(pPM_CB->GetName(), pPM_CB));

	//Register actions
	m_allActions.insert(TActionMap::value_type("PingerIdle", SMyAction("pinger_idle_01", 0.75f, 0.25f, true)));
	m_allActions.insert(TActionMap::value_type("PingerWalk", SMyAction("pinger_walk_forward_01", 1.0f, 0.25f, true)));
	m_allActions.insert(TActionMap::value_type("PingerPing", SMyAction("pinger_ping_01", 1.0f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerDie", SMyAction("pinger_idle_death_01", 1.0f, 0.125f, false)));
	m_allActions.insert(TActionMap::value_type("PingerTurnLeft", SMyAction("pinger_turn_left_45_01", 1.5f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerTurnRight", SMyAction("pinger_turn_right_45_01", 1.5f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerStrafeLeft", SMyAction("pinger_strafe_left_01", 1.0f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerStrafeRight", SMyAction("pinger_strafe_right_01", 1.0f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerMelee1", SMyAction("pinger_melee_01", 1.0f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerMelee2", SMyAction("pinger_melee_02", 1.0f, 0.125f, true)));
	m_allActions.insert(TActionMap::value_type("PingerHitLeft", SMyAction("pinger_hit_02", 1.0f, 0.15f, false, 1)));
	m_allActions.insert(TActionMap::value_type("PingerHitRight", SMyAction("pinger_hit_03", 1.0f, 0.15f, false, 1)));
}

//Destroy mechanics
void MyMechanicRegistry::Destroy()
{
	for(TMechanicsMap::iterator it = m_allMechanics.begin(); it != m_allMechanics.end(); ++it)
	{
		it->second->Release();
	}
	m_allMechanics.clear();
	m_allActions.clear();
}

//Get a mechanic by name
IMyMechanic* MyMechanicRegistry::GetMechanicByName(const char* mechanicName) const
{
	TMechanicsMap::const_iterator cit = m_allMechanics.find(CONST_TEMP_STRING(mechanicName));
	if(cit == m_allMechanics.end())
		return NULL; //Perhaps return a default mechanic here....

	return cit->second;
}

//Get an action by name
const SMyAction& MyMechanicRegistry::GetActionByName(const char* mechanicName) const
{
	static SMyAction defaultAction("", 1.0f, 0.125f, false);

	TActionMap::const_iterator cit = m_allActions.find(CONST_TEMP_STRING(mechanicName));
	if(cit == m_allActions.end())
		return defaultAction; 

	return cit->second;
}

//---------------------------------------------------------------------------
//Pinger mechanic locomotion

//Initialize stuff (probably could read parameters from XML and the like)
bool CPingerMechanic_Locomotion::Init()
{
	m_lmParams.maxSpeed = 4.5f;
	m_lmParams.accelerationTime = 1.35f;
	m_lmParams.maxturnSpeed = DEG2RAD(45.0f);

	return true;
}

//Update instance using provided parameters
bool CPingerMechanic_Locomotion::Update(CActor* pActor, mi_state* pState, const mi_params* pParameters)
{
	assert(pState && pParameters);
	assert(pParameters->type == eMIP_PINGER);
	assert(pState->type == eMIP_PINGER);

	const float frameTime = gEnv->pTimer->GetFrameTime();
	const Matrix33 entityMtx = Matrix33(pActor->GetEntity()->GetWorldRotation());
	
	CPinger::mi_pinger_state* pPingerState = static_cast<CPinger::mi_pinger_state*>(pState);
	const CPinger::mi_pinger_params* pPingerParams = static_cast<const CPinger::mi_pinger_params*>(pParameters);

	Vec3 desiredMoveDir(ZERO); float desiredSpeed = 0.0f;
	ExtractDesiredMoveDirection(pActor, pParameters, desiredMoveDir, desiredSpeed);

	//Remove Z-component from movement
	desiredMoveDir -= desiredMoveDir * (entityMtx * Matrix33::CreateScale(Vec3(0,0,1)));
	desiredMoveDir.NormalizeSafe();

	//Prepare request
	SCharacterMoveRequest frameMovementRequest;
	frameMovementRequest.type = eCMT_Normal;
	frameMovementRequest.rotation.SetIdentity();
	frameMovementRequest.velocity.zero();

	//Rotation --------------------
	Matrix33	orientationTarget;
	orientationTarget.SetRotationVDir(desiredMoveDir.GetNormalizedSafe());

	float smoothSpeed = pPingerState->speed;
	{
		Quat	targetQuat(orientationTarget);
		Quat	currQuat(entityMtx);
		Quat  targetRot = Quat::CreateSlerp( currQuat.GetNormalized(), targetQuat, (smoothSpeed < 0.2f) ? min(frameTime * 5.0f, 1.0f) : min(frameTime, 1.0f));

		frameMovementRequest.rotation = currQuat.GetInverted().GetNormalizedSafe() * targetRot;
	
		//Movement
		Vec3 moveRequest(desiredMoveDir);
		moveRequest -= moveRequest * (entityMtx * Matrix33::CreateScale(Vec3(0,0,1)));

		//Scale speed down when near path end
		const float thresholdPathEndDst = 4.0f;
		float decelScale = 1.0f;
		if(pPingerParams->moveRequest.fDistanceToPathEnd < thresholdPathEndDst)
		{
			decelScale =pow( 2.0f - (pPingerParams->moveRequest.fDistanceToPathEnd / thresholdPathEndDst), 2);
		}

		const float acceleration = (m_lmParams.maxSpeed / m_lmParams.accelerationTime) * decelScale; //Constant linear acceleration
		Interpolate(smoothSpeed, desiredSpeed, acceleration , frameTime);

		frameMovementRequest.velocity = desiredMoveDir * smoothSpeed;
	}

		//Send movement request
	if (IAnimatedCharacter* pAnimatedCharacter = pActor->GetAnimatedCharacter())
	{
		// synthesize a prediction
		frameMovementRequest.prediction.nStates = 0;
		frameMovementRequest.prediction.states[0].deltatime = 0.0f;
		frameMovementRequest.prediction.states[0].velocity = frameMovementRequest.velocity;
		frameMovementRequest.prediction.states[0].position = pActor->GetEntity()->GetWorldPos();
		frameMovementRequest.prediction.states[0].orientation = pActor->GetEntity()->GetWorldRotation();

		pAnimatedCharacter->AddMovement(frameMovementRequest);
	
		{
			const float idleSpeedThreshold = 0.2f;
			bool  wasMoving = (pPingerState->speed > idleSpeedThreshold);
			if(smoothSpeed <= idleSpeedThreshold)
				pActor->PlayAction("PingerIdle", "");
			else if(!wasMoving && (smoothSpeed > idleSpeedThreshold))
				pActor->PlayAction("PingerWalk", "");
		}
	}
	
	return true;
}

//Handle enter mechanic
void CPingerMechanic_Locomotion::OnEnter(CActor* pActor, mi_state* pState)
{
	IAnimatedCharacter* pAnimatedCharacter = pActor->GetAnimatedCharacter();
	if(pAnimatedCharacter)
	{
		pAnimatedCharacter->SetMovementControlMethods(eMCM_Entity, eMCM_Entity);
	}
	pActor->PlayAction("PingerIdle", "");
}

//Handle leave mechanic
void CPingerMechanic_Locomotion::OnLeave(CActor* pActor, mi_state* pState)
{

	//IAnimatedCharacter* pAnimatedCharacter = pActor->GetAnimatedCharacter();
	//if(pAnimatedCharacter)
	//{
		//pAnimatedCharacter->SetMovementControlMethods(eMCM_Entity, eMCM_Entity);
	//}

}

//Extract desired movement from parameters
void CPingerMechanic_Locomotion::ExtractDesiredMoveDirection(CActor* pActor, const mi_params* pParameters, Vec3& dir, float& speed)
{
	assert(pParameters->type == eMIP_PINGER);

	const CPinger::mi_pinger_params* pParams = static_cast<const CPinger::mi_pinger_params*> (pParameters);

	SMovementState state;
	pActor->GetMovementController()->GetMovementState(state);

	//Set view direction
	if(!pParams->moveRequest.vAimTargetPos.IsZero())
	{
		dir = ((pParams->moveRequest.vAimTargetPos - state.weaponPosition).GetNormalizedSafe());
	}
	else if(!pParams->moveRequest.vMoveDir.IsZero())
	{
		dir = (pParams->moveRequest.vMoveDir.GetNormalizedSafe());
	}
	else
	{
		dir = (pActor->GetEntity()->GetWorldRotation() * FORWARD_DIRECTION);
	}

	//Set speed
	speed = min(pParams->moveRequest.fDesiredSpeed, 1.0f) * m_lmParams.maxSpeed;
}

//---------------------------------------------------------------------------
//Pinger mechanic anim action (used for one shot animations, and animation driven movement)

//Init stuff
bool CPingerMechanic_AnimAction::Init()
{
	return true;
}

//Update just check if animation is playing, it will return false when animation FIFO is empty
bool CPingerMechanic_AnimAction::Update(CActor *pActor, mi_state *pState, const mi_params *pParameters)
{
	//Prepare empty request (needed?)
	if(IAnimatedCharacter* pAnimatedCharacter = pActor->GetAnimatedCharacter())
	{
		SCharacterMoveRequest frameMovementRequest;
		frameMovementRequest.type = eCMT_None;
		frameMovementRequest.rotation.SetIdentity();
		frameMovementRequest.velocity.zero();

		pAnimatedCharacter->AddMovement(frameMovementRequest);
	}

	ICharacterInstance* pCharacter = pActor->GetEntity()->GetCharacter(0);
	if(pCharacter)
	{
		return (pCharacter->GetISkeletonAnim()->GetNumAnimsInFIFO(0)>0);
	}

	return false;
}

//Handle on enter
void CPingerMechanic_AnimAction::OnEnter(CActor* pActor, mi_state* pState)
{
	IAnimatedCharacter* pAnimatedCharacter = pActor->GetAnimatedCharacter();
	if(pAnimatedCharacter)
	{
		pAnimatedCharacter->SetMovementControlMethods(eMCM_Animation, eMCM_Animation);
	}
}

//Handle on exit
void CPingerMechanic_AnimAction::OnLeave(CActor* pActor, mi_state* pState)
{

}

//-----------------------------------------------
// Pinger mechanic combat
bool CPingerMechanic_Combat::Init()
{
	m_cbParams.turnIdleThreshold = cry_cosf(DEG2RAD(5.0f));
	m_cbParams.turnLimitThreshold = cry_cosf(DEG2RAD(45.0f));
	m_cbParams.strafeLimitThreshold = cry_cosf(DEG2RAD(45.0f));

	return true;
}

bool CPingerMechanic_Combat::Update(CActor *pActor, mi_state *pState, const mi_params *pParameters)
{
	assert(pState && pParameters);
	assert(pParameters->type == eMIP_PINGER);
	assert(pState->type == eMIP_PINGER);

	const float frameTime = gEnv->pTimer->GetFrameTime();
	const Matrix33 entityMtx = Matrix33(pActor->GetEntity()->GetWorldRotation());

	CPinger::mi_pinger_state* pPingerState = static_cast<CPinger::mi_pinger_state*>(pState);
	const CPinger::mi_pinger_params* pPingerParams = static_cast<const CPinger::mi_pinger_params*>(pParameters);

	Vec3 desiredLookDir(ZERO); 
	ExtractDesiredLookDirection(pActor, pParameters, desiredLookDir);

	//Remove Z-component from movement
	desiredLookDir -= desiredLookDir * (entityMtx * Matrix33::CreateScale(Vec3(0,0,1)));
	desiredLookDir.NormalizeSafe();

	//Prepare request
	SCharacterMoveRequest frameMovementRequest;
	frameMovementRequest.type = eCMT_Normal;
	frameMovementRequest.rotation.SetIdentity();
	frameMovementRequest.velocity.zero();

	int strafeDir=ePLD_None;
/*	if (pPingerParams->moveRequest.fDesiredSpeed>0.001f)
	{
		strafeDir=ShouldStrafe(Vec2(pPingerParams->moveRequest.vMoveDir), Vec2(desiredLookDir));

		if (strafeDir!=pPingerState->strafingState)
		{
			pActor->PlayAction(strafeDir == ePLD_Left ? "PingerStrafeLeft" : "PingerStrafeRight", "");
			pPingerState->strafingState=strafeDir;
		}
	}
*/
	//Rotation --------------------
	Matrix33	orientationTarget;
	orientationTarget.SetRotationVDir(desiredLookDir);

	if (strafeDir==ePLD_None)
	{
		Vec2 currentDir(Vec2(entityMtx.GetColumn1()));
		Vec2 targetDir(Vec2(orientationTarget.GetColumn1()));
		currentDir.NormalizeSafe();
		targetDir.NormalizeSafe();

		int turnDir = ePLD_None; //0 is non, 1 left, 2 right
		if(pPingerState->turningState == ePLD_None)
			turnDir = ShouldStartTurning(currentDir, targetDir);
		else
			turnDir = ShouldContinueTurning(currentDir, targetDir);

		if(pPingerState->turningState != turnDir)
		{
			if(turnDir == ePLD_None)
				pActor->PlayAction("PingerIdle", "");
			else
				pActor->PlayAction( turnDir == ePLD_Left ? "PingerTurnLeft" : "PingerTurnRight", "");
		}
		pPingerState->turningState = turnDir;
	}

	if (pActor->GetHealth()>0)
	{
		ICharacterInstance *pCharacter=pActor->GetEntity()->GetCharacter(0);
		ISkeletonPose *pSkeletonPose=pCharacter?pCharacter->GetISkeletonPose():0;
		if (pSkeletonPose)
		{
			static const float customBlend[5] = { 0.04f, 0.06f, 0.08f, 0.6f, 0.6f };

			Vec3 target=pPingerParams->moveRequest.vAimTargetPos;
			if (target.IsZero())
				target=pPingerParams->moveRequest.vLookTargetPos;

			pSkeletonPose->SetLookIK(true,DEG2RAD(45.0f), target, customBlend);
		}
	}

	return true;
}

//Handle on enter
void CPingerMechanic_Combat::OnEnter(CActor* pActor, mi_state* pState)
{
	IAnimatedCharacter* pAnimatedCharacter = pActor->GetAnimatedCharacter();
	if(pAnimatedCharacter)
	{
		pAnimatedCharacter->SetMovementControlMethods(eMCM_Animation, eMCM_Animation);
	}
	pActor->PlayAction("PingerIdle", "");
}

//Handle on exit
void CPingerMechanic_Combat::OnLeave(CActor* pActor, mi_state* pState)
{
	assert(pState->type == eMIP_PINGER);

	CPinger::mi_pinger_state* pPingerState = static_cast<CPinger::mi_pinger_state*>(pState);

	pPingerState->turningState= ePLD_None;
}

//Extract desired movement from parameters
void CPingerMechanic_Combat::ExtractDesiredLookDirection(CActor* pActor, const mi_params* pParameters, Vec3& dir)
{
	assert(pParameters->type == eMIP_PINGER);

	const CPinger::mi_pinger_params* pParams = static_cast<const CPinger::mi_pinger_params*> (pParameters);

	SMovementState state;
	pActor->GetMovementController()->GetMovementState(state);

	//Set view direction
	if(!pParams->moveRequest.vAimTargetPos.IsZero())
	{
		dir = (pParams->moveRequest.vAimTargetPos - state.weaponPosition).GetNormalizedSafe();
	}
	else if(!pParams->moveRequest.vLookTargetPos.IsZero())
	{
		dir = (pParams->moveRequest.vLookTargetPos - state.eyePosition).GetNormalizedSafe();
	}
	else
	{
		dir = (pActor->GetEntity()->GetWorldRotation() * FORWARD_DIRECTION);
	}
}

int CPingerMechanic_Combat::ShouldStartTurning(const Vec2 currentDir, const Vec2 targetDir)
{
	const float dot = currentDir.Dot(targetDir);
	const float cross = currentDir.Cross(targetDir);

	if(dot < m_cbParams.turnLimitThreshold)
	{
		return (cross > 0.0f) ? ePLD_Left : ePLD_Right;
	}

	return ePLD_None;
}

int CPingerMechanic_Combat::ShouldContinueTurning(const Vec2 currentDir, const Vec2 targetDir)
{
	const float dot = currentDir.Dot(targetDir);
	const float cross = currentDir.Cross(targetDir);

	if(dot < m_cbParams.turnIdleThreshold)
	{
		return (cross > 0.0f) ? ePLD_Left : ePLD_Right;
	}

	return ePLD_None;
}

int CPingerMechanic_Combat::ShouldStrafe(const Vec2 moveDir, const Vec2 lookDir)
{
	const float dot = moveDir.Dot(lookDir);
	const float cross = moveDir.Cross(lookDir);

	if(dot < m_cbParams.strafeLimitThreshold)
	{
		return (cross > 0.0f) ? ePLD_Right : ePLD_Left;
	}

	return ePLD_None;
}