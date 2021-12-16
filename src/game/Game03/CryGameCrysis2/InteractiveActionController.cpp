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

#include "StdAfx.h"
#include "InteractiveActionController.h"

#include "Item.h"
#include "Player.h"

#include "Environment/InteractiveObjectRegistry.h"

#define INTERACTION_DEFAULT_CORRECTION_DELAY 0.4f

void CInteractiveActionController::OnEnter( EntityId interactiveObjectId )
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized!");

	m_interactiveObjectId = interactiveObjectId;
	m_state = eState_PositioningUser;
	m_runningTime	= 0.0f;
	m_remainingAnimationTime = 1.0f;

	CalculateStartTargetLocation();

	//This will start a deselect animation (if any), before holstering
	if (CItem* pCurrentItem = static_cast<CItem*>(m_pControlledPlayer->GetCurrentItem()))
	{
		pCurrentItem->StartDeselection();
	}

	if (IAnimatedCharacter* pAnimatedCharacter = m_pControlledPlayer->GetAnimatedCharacter())
	{
		pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_NonPushable, eColliderModeLayer_Game, "CInteractiveActionController::OnEnter()");
		pAnimatedCharacter->SetMovementControlMethods(eMCM_Entity, eMCM_Entity);
	}
}

void CInteractiveActionController::OnEnterByName( const char* interaction )
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized!");

	m_interactionName = interaction;

	OnEnter(0);
}

void CInteractiveActionController::OnLeave()
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized!");

	m_interactionName = "";
	m_interactiveObjectId = 0;
	m_state = eState_None;
	m_runningTime	= m_remainingAnimationTime = 0.0f;
	m_correctionDelay = INTERACTION_DEFAULT_CORRECTION_DELAY;

	m_pControlledPlayer->GetAnimationGraphState()->SetInput("Action","idle");

	m_pControlledPlayer->HolsterItem(false);

	if (IAnimatedCharacter* pAnimatedCharacter = m_pControlledPlayer->GetAnimatedCharacter())
	{
		pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Undefined, eColliderModeLayer_Game, "CInteractiveActionController::OnLeave()");
	}
}

void CInteractiveActionController::Update( float frameTime,  const SActorFrameMovementParams& movement )
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized!");
	CRY_ASSERT_MESSAGE(m_state != eState_None, "Non valid state!");

	m_runningTime += frameTime;

	SCharacterMoveRequest movementRequest;
	movementRequest.type = eCMT_Fly;
	movementRequest.jumping = false;
	movementRequest.allowStrafe = false;
	movementRequest.prediction.nStates = 0;

	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_targetLocation.t, 0.3f, ColorB(0, 192, 0));

	if (m_state == eState_PositioningUser)
	{
		m_state = CorrectUserPosition(frameTime, movement, movementRequest);	
		DoMovementRequest(movementRequest);
	}
	else if (m_state == eState_PlayingAnimation)
	{
		m_state = UpdateAnimationState(frameTime, movement, movementRequest);
		DoMovementRequest(movementRequest);
	}
	else if (m_state == eState_Done)
	{
		NotifyFinished();
	}
}

CInteractiveActionController::EState CInteractiveActionController::CorrectUserPosition( float frameTime, const SActorFrameMovementParams& movement, SCharacterMoveRequest& movementRequest )
{
	CRY_ASSERT_MESSAGE(m_targetLocation.IsValid(), "Target location invalid!");

	const float timeToReachTarget = m_correctionDelay;
	const float correctionFactor = (float)__fsel(-timeToReachTarget, 1.0f, clamp(m_runningTime/(timeToReachTarget+FLT_EPSILON), 0.0f, 1.0f));

	IEntity* pUserEntity = m_pControlledPlayer->GetEntity();

	QuatT currentLocation (pUserEntity->GetWorldPos(), pUserEntity->GetWorldRotation());
	
	Quat currentTargetRot = Quat::CreateSlerp(currentLocation.q, m_targetLocation.q, correctionFactor); 
	currentTargetRot.Normalize();
	movementRequest.rotation = (currentLocation.q.GetInverted() * currentTargetRot);
	movementRequest.rotation.Normalize();

	if (correctionFactor < 0.9999f)
	{
		movementRequest.velocity = (frameTime > 0.0f) ? ((m_targetLocation.t - currentLocation.t) * correctionFactor / frameTime) : ZERO;

		return eState_PositioningUser;
	}
	else
	{
		//User location should be corrected at this point, but just in case
		movementRequest.velocity.zero();
		pUserEntity->SetPosRotScale(m_targetLocation.t, currentLocation.q, pUserEntity->GetScale());

		m_pControlledPlayer->HolsterItem(true);

		StartAnimations();

		return eState_PlayingAnimation;
	}

}

CInteractiveActionController::EState CInteractiveActionController::UpdateAnimationState( float frameTime, const SActorFrameMovementParams& movement, SCharacterMoveRequest& movementRequest )
{
	m_remainingAnimationTime -= frameTime;
	
	const QuatT& relativeAnimMovement = 	m_pControlledPlayer->GetAnimationRelativeMovement();
	movementRequest.rotation = relativeAnimMovement.q;
	movementRequest.velocity = (frameTime > 0.0f) ? (m_pControlledPlayer->GetEntity()->GetWorldRotation() * relativeAnimMovement.t) / frameTime : ZERO;
	
	return (m_remainingAnimationTime <= 0.0f) ? eState_Done : eState_PlayingAnimation;
}

void CInteractiveActionController::DoMovementRequest( const SCharacterMoveRequest& movementRequest )
{
	IAnimatedCharacter* pAnimatedCharacter = m_pControlledPlayer->GetAnimatedCharacter();
	if (pAnimatedCharacter)
	{
		pAnimatedCharacter->AddMovement(movementRequest);
	}

	m_pControlledPlayer->SetViewRotation((m_pControlledPlayer->GetEntity()->GetWorldRotation() * movementRequest.rotation).GetNormalized());
	m_pControlledPlayer->GetAnimationGraphState()->SetInput("Action","interactiveAction");
}

void CInteractiveActionController::StartAnimations()
{
	const int interactionUserAnimLayer = 0;
	const int interactionObjectAnimLayer = 0;

	const SInteractionParams* pInteractionParams = NULL;
	if (m_interactiveObjectId)
		pInteractionParams = g_pGame->GetInteractiveObjectsRegistry().GetInteractionParamsForEntity(m_interactiveObjectId);
	else if (!m_interactionName.empty())
		pInteractionParams = g_pGame->GetInteractiveObjectsRegistry().GetInteractionParamsByName(m_interactionName.c_str());

	const float playBackSpeed = pInteractionParams ? pInteractionParams->speed : 1.0f; 
	assert(playBackSpeed > 0.0f);
	if (pInteractionParams)
	{
		CAnimationPlayerProxy* pUserAnimProxy = m_pControlledPlayer->GetAnimatedCharacter() ? m_pControlledPlayer->GetAnimatedCharacter()->GetAnimationPlayerProxy(0) : NULL;
		if (pUserAnimProxy)
		{
			CryCharAnimationParams animationParams;
			animationParams.m_nLayerID = interactionUserAnimLayer;
			animationParams.m_nFlags |= CA_DISABLE_MULTILAYER;
			animationParams.m_fTransTime = 0.15f;
			pUserAnimProxy->StartAnimation(m_pControlledPlayer->GetEntity(), pInteractionParams->userAnimation.c_str(), animationParams, playBackSpeed);
		}

		//Extract animation time (set fallback value in case asset does not exist or something)
		ICharacterInstance* pUserCharacter = m_pControlledPlayer->GetEntity()->GetCharacter(0);
		if (pUserCharacter)
		{
			IAnimationSet* pAnimationSet = pUserCharacter->GetIAnimationSet();
			CRY_ASSERT(pAnimationSet);

			int animationId = pAnimationSet->GetAnimIDByName(pInteractionParams->userAnimation.c_str());
			m_remainingAnimationTime = (animationId >= 0) ? (pAnimationSet->GetDuration_sec(animationId) / playBackSpeed) : 1.0f;
		}

		if (m_interactiveObjectId)
		{
			IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity(m_interactiveObjectId);
			ICharacterInstance* pObjectCharacter = pObjectEntity ? pObjectEntity->GetCharacter(0) : NULL;
			if (pObjectCharacter)
			{
				CryCharAnimationParams animationParams;
				animationParams.m_nLayerID = interactionObjectAnimLayer;
				animationParams.m_fTransTime = 0.15f;
				animationParams.m_fPlaybackSpeed = playBackSpeed;
				animationParams.m_nFlags |= CA_REPEAT_LAST_KEY;
				pObjectCharacter->GetISkeletonAnim()->StartAnimation(pInteractionParams->objectAnimation.c_str(), animationParams);
			}
		}
	}
}


void CInteractiveActionController::NotifyFinished()
{
	if (m_interactiveObjectId)
	{
		IEntity* pInteractiveObject = gEnv->pEntitySystem->GetEntity(m_interactiveObjectId);
		if (pInteractiveObject)
		{
			HSCRIPTFUNCTION scriptStopUse(NULL);
			IScriptTable *pScriptTable = pInteractiveObject->GetScriptTable();

			if (pScriptTable)
			{
				pScriptTable->GetValue("StopUse", scriptStopUse);

				if (scriptStopUse)
				{
					Script::Call(gEnv->pScriptSystem,scriptStopUse,pScriptTable,ScriptHandle(m_pControlledPlayer->GetEntityId()));
				}
			}

			gEnv->pScriptSystem->ReleaseFunc(scriptStopUse);
		}
	}

	m_pControlledPlayer->EndInteractiveAction(m_interactiveObjectId);
}

void CInteractiveActionController::CalculateStartTargetLocation()
{
	IEntity* pObjectEntity = NULL;
	const SInteractionParams* pInteractionParams = NULL;
	
	if (m_interactiveObjectId)
	{
		pObjectEntity = gEnv->pEntitySystem->GetEntity(m_interactiveObjectId);
		pInteractionParams = g_pGame->GetInteractiveObjectsRegistry().GetInteractionParamsForEntity(m_interactiveObjectId);
	}
	else if (!m_interactionName.empty())
	{
		pInteractionParams = g_pGame->GetInteractiveObjectsRegistry().GetInteractionParamsByName(m_interactionName.c_str());
	}
	
	if (pObjectEntity)
	{
		m_targetLocation.t = pObjectEntity->GetWorldPos();
		m_targetLocation.q = pObjectEntity->GetWorldRotation();

		ICharacterInstance* pObjectCharacter = pObjectEntity->GetCharacter(0);
		if (pObjectCharacter && pInteractionParams)
		{
			m_correctionDelay = pInteractionParams->correctionDelay;

			int16 jointId = pObjectCharacter->GetISkeletonPose()->GetJointIDByName(pInteractionParams->helperName.c_str());
			if (jointId >= 0)
			{
				const QuatT& helperLocation = pObjectCharacter->GetISkeletonPose()->GetAbsJointByID(jointId);
				m_targetLocation.t = m_targetLocation.t + (m_targetLocation.q * helperLocation.t);
				m_targetLocation.q = m_targetLocation.q * helperLocation.q;
			}
			else
			{
				GameWarning("Helper '%s' not found, default to object location as target player position.", pInteractionParams->helperName.c_str());
			}
		}
		else
		{
			GameWarning("No character or parameters available, default to object location as target player position.");
		}
	}
	else
	{
		m_correctionDelay = pInteractionParams ? pInteractionParams->correctionDelay : INTERACTION_DEFAULT_CORRECTION_DELAY;
		m_targetLocation.t = m_pControlledPlayer->GetEntity()->GetWorldPos();
		m_targetLocation.q = m_pControlledPlayer->GetEntity()->GetWorldRotation();
	}
}