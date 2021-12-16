/********************************************************************
CryGame Source File.
	Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   AIProxy.cpp
Version:     v1.00
Description: 

-------------------------------------------------------------------------
History:
- 7:10:2004   18:54 : Created by Kirill Bulatsev
- 2:12:2005           Now talks directly to CryAction interfaces (craig tiller)
	- 2 Mar 2009				: Evgeny Adamenkov: Removed parameter of type IRenderer in DebugDraw


	Notes:
	We can't assume that there is just one IGameObject to an entity, because we choose between (at least) Actor and Vehicle objects.


*********************************************************************/

#include "StdAfx.h"
#include "AIProxy.h"
#include <IAIAction.h>
#include "IAISystem.h"
#include "AIHandler.h"
#include "IItemSystem.h"
#include "IVehicleSystem.h"
#include "ICryAnimation.h"
#include "CryAction.h"

#include "RangeSignalingSystem/RangeSignaling.h"
#include "SignalTimers/SignalTimers.h"
#include "IAIDebugRenderer.h"

#include "AI/CommunicationHandler.h"

#include "IAIObject.h"
#include "IAIActor.h"
//
//----------------------------------------------------------------------------------------------------------
CAIProxy::CAIProxy(IGameObject *pGameObject):
m_pGameObject(pGameObject),
m_pAIHandler(0),
m_firing(false),
m_firingSecondary(false),
m_readibilityDelay(0.0f),
m_readibilityPlayed(false),
m_readibilityPriority(0),
m_readibilityId(0),
m_readibilityIgnored(0),
m_fMinFireTime(0),
m_WeaponShotIsDone(false),
m_NeedsShootSignal(false),
m_PrevAlertness(0),
m_IsDisabled(false),
m_UpdateAlways(false),
m_IsWeaponForceAlerted(false),
m_pIActor(0),
m_shotBulletCount(0),
m_lastShotTime(0.0f),
m_animActionGoalPipeId(0),
m_forcedExecute(0)
{
	// (MATT) This used to include a useAIHandler option, but it was always true {2009/04/06}
	// (MATT) This and serialisation are the only places this is set {2009/01/28}
	// (MATT) All the NULL checks on this could go - only Release clears the pointer {2009/04/20}
		m_pAIHandler = new CAIHandler(m_pGameObject);
		m_pAIHandler->Init(GetISystem());

		m_commHandler.reset(new CommunicationHandler(*this, *m_pGameObject->GetEntity()));

		if (IScriptTable* script = m_pGameObject->GetEntity()->GetScriptTable())
		{
			SmartScriptTable props;
			if (script->GetValue("Properties", props))
			{
				const char* voice;
				if (props->GetValue("esVoice", voice))
					m_voiceLibrary = voice;

				const char* commConfigName;
				if (props->GetValue("esCommConfig", commConfigName))
					m_commConfigName = commConfigName;

				const char* behaviorSelectionTreeName;
				if (props->GetValue("esBehaviorSelectionTree", behaviorSelectionTreeName))
					m_behaviorSelectionTreeName = behaviorSelectionTreeName;
			}
		}
	}

//
//----------------------------------------------------------------------------------------------------------
CAIProxy::~CAIProxy(void)
{
	Release();
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::Release()
{
	if( m_pAIHandler )
	{
		// Cleanly release AI handler
		m_pAIHandler->Release();
		delete m_pAIHandler;
		m_pAIHandler = NULL;
	}
	// Clear the cached pointer to the actor, who will probably soon be deleted
	m_pIActor = NULL;

	// (MATT) Currently cannot clear the gameobject pointer here - it must be preserved if an AI is removed, in case needed after deserialisation {2009/03/27}
}

//
//----------------------------------------------------------------------------------------------------------
EntityId CAIProxy::GetLinkedDriverEntityId()
{
	IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
	IVehicle* pVehicle = pVehicleSystem->GetVehicle(m_pGameObject->GetEntityId());
	if (pVehicle)
		if (IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(1))	//1 means driver
			return pVehicleSeat->GetPassenger();
	return 0;
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::IsDriver()
{
	// (MATT) Surely rather redundant {2009/01/27}
	if (IActor* pActor = GetActor())
	{
		if (pActor->GetLinkedVehicle())
		{
			IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
			IVehicle* pVehicle = pVehicleSystem->GetVehicle(pActor->GetLinkedVehicle()->GetEntityId());
			if (pVehicle)
			{
				if (IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(1))	//1 means driver
				{
					if (  m_pGameObject )
					{
						EntityId passengerId = pVehicleSeat->GetPassenger();
						EntityId selfId = m_pGameObject->GetEntityId();
						if ( passengerId == selfId )
							return true;
					}
				}
			}
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------
EntityId CAIProxy::GetLinkedVehicleEntityId()
{
	if (IActor* pActor = GetActor())
	{
		if (pActor->GetLinkedVehicle())
			return pActor->GetLinkedVehicle()->GetEntityId();
	}
	return 0;
}

//
//----------------------------------------------------------------------------------------------------------
void  CAIProxy::QueryBodyInfo( SAIBodyInfo& bodyInfo ) 
{
	IMovementController * pMC = m_pGameObject->GetMovementController();
	if (!pMC)
		return;

	SMovementState moveState;
	pMC->GetMovementState( moveState );

	bodyInfo.maxSpeed = moveState.maxSpeed;
	bodyInfo.minSpeed = moveState.minSpeed;
	bodyInfo.normalSpeed = moveState.normalSpeed;
	bodyInfo.stance = moveState.stance;
	bodyInfo.stanceSize = moveState.m_StanceSize;
	bodyInfo.colliderSize = moveState.m_ColliderSize;
	bodyInfo.vEyeDir = moveState.eyeDirection;
	bodyInfo.vEyeDirAnim = moveState.animationEyeDirection;
	bodyInfo.vEyePos = moveState.eyePosition;
	bodyInfo.vBodyDir = moveState.bodyDirection;
	bodyInfo.vMoveDir = moveState.movementDirection;
	bodyInfo.vUpDir = moveState.upDirection;
	bodyInfo.lean = moveState.lean;
	bodyInfo.peekOver = moveState.peekOver;
	bodyInfo.slopeAngle = moveState.slopeAngle;

	bodyInfo.vFirePos = moveState.weaponPosition;
	bodyInfo.vFireDir = moveState.aimDirection;
	bodyInfo.isAiming = moveState.isAiming; 
	bodyInfo.isFiring = moveState.isFiring;

	IActor*	pActor = GetActor();
	IVehicle* pVehicle = pActor ? pActor->GetLinkedVehicle() : 0;
	IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
	IEntitySystem* pEntitySystem = gEnv->pEntitySystem;

	bodyInfo.linkedVehicleEntity = pVehicle ? pVehicle->GetEntity() : 0;

	bodyInfo.linkedDriverEntity = 0;
	{
		// (MATT) Also redundant? {2009/01/27}
		pVehicle = pVehicleSystem->GetVehicle(m_pGameObject->GetEntityId());
		if ( pVehicle )
		{
			IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(1);//1 means driver
			if ( pVehicleSeat )
			{
				EntityId driverId = pVehicleSeat->GetPassenger();
				if ( driverId )
				{
					if ( pEntitySystem ) 
					{
						IEntity* pEntity = pEntitySystem->GetEntity( driverId );
						if ( pEntity )
						{
							bodyInfo.linkedDriverEntity = pEntity;
						}
					}
				}
			}
		}
	}
}

bool CAIProxy::QueryBodyInfo( const SAIBodyInfoQuery& query, SAIBodyInfo& bodyInfo )
{
	IMovementController* pMC = m_pGameObject->GetMovementController();
	if (!pMC)
		return false;

	SStanceState stanceState;
	if (!pMC->GetStanceState(SStanceStateQuery(query.position, query.target, query.stance, query.lean, query.peekOver, query.defaultPose), stanceState))
		return false;

	bodyInfo.maxSpeed = 0.0f;
	bodyInfo.minSpeed = 0.0f;
	bodyInfo.normalSpeed = 0.0f;
	bodyInfo.stance = query.stance;
	bodyInfo.stanceSize = stanceState.m_StanceSize;
	bodyInfo.colliderSize = stanceState.m_ColliderSize;
	bodyInfo.vEyeDir = stanceState.eyeDirection;
	bodyInfo.vEyeDirAnim.Set(0,0,0);
	bodyInfo.vEyePos = stanceState.eyePosition;
	bodyInfo.vBodyDir = stanceState.bodyDirection;
	bodyInfo.vMoveDir.zero();
	bodyInfo.vUpDir = stanceState.upDirection;
	bodyInfo.vFirePos = stanceState.weaponPosition;
	bodyInfo.vFireDir = stanceState.aimDirection;
	bodyInfo.isAiming = false; 
	bodyInfo.isFiring = false;
	bodyInfo.lean = stanceState.lean;
	bodyInfo.peekOver = stanceState.peekOver;

	bodyInfo.linkedVehicleEntity = 0;
	bodyInfo.linkedDriverEntity = 0;

	return true;
}


IEntity* CAIProxy::GetGrabbedEntity() const
{ 
	IActor*	pActor(GetActor());
	if(pActor)
	{
		return gEnv->pEntitySystem->GetEntity(pActor->GetGrabbedEntityId());
	}
	return 0;
}

const char* CAIProxy::GetVoiceLibraryName() const
{
	return m_voiceLibrary.c_str();
}

const char* CAIProxy::GetCommunicationConfigName() const
{
	return m_commConfigName.c_str();
}

const char* CAIProxy::GetBehaviorSelectionTreeName() const
{
	return m_behaviorSelectionTreeName.c_str();
}


IWeapon * CAIProxy::GetCurrentWeapon() const
{ 
	IItem* pItem = 0;
	IActor*	pActor = GetActor();
	if (!pActor)
		return 0;

	EntityId itemId = 0;

	if (IVehicle* pVehicle = pActor->GetLinkedVehicle())
	{
		// mounted weapon( gunner seat )
		itemId = pVehicle->GetCurrentWeaponId(m_pGameObject->GetEntityId());

		// vehicle weapon( driver seat )
		IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(1);//1 means driver
		if ( pVehicleSeat )
		{
			IEntity* pMyEntity = pActor->GetEntity();
			if ( pMyEntity )
			{
				EntityId driverId = pVehicleSeat->GetPassenger();
				EntityId myId = pMyEntity->GetId();
				if ( driverId == myId )
					return 0;
			}
		}

		pItem = CCryAction::GetCryAction()->GetIItemSystem()->GetItem( itemId );
	}
	else  
	{
		pItem = pActor->GetCurrentItem();
	}

	if (pItem)
		return pItem->GetIWeapon();

	return 0;
}

IWeapon * CAIProxy::GetSecWeapon(ERequestedGrenadeType prefGrenadeType) const
{
	CRY_ASSERT(prefGrenadeType > eRGT_INVALID && prefGrenadeType < eRGT_COUNT);

	IActor*	pActor = GetActor();
	IInventory * pInventory = (pActor != NULL) ? pActor->GetInventory() : NULL;
	if (!pInventory)
		return NULL;

	IEntityClassRegistry *pRegistery = gEnv->pEntitySystem->GetClassRegistry();
	CRY_ASSERT(pRegistery);

	EntityId itemEntity = 0;
	IAIObject* pAI = m_pGameObject->GetEntity()->GetAI();
	if (pAI && pAI->GetAIType()==AIOBJECT_PLAYER)
	{
		// Offhand weapon for player, can be any
		itemEntity = pInventory->GetItemByClass(pRegistery->FindClass("OffHand"));
	}
	else
	{
		// EntityIds of what types of grenades we have in our inventory
		// TODO Hardcoded class names are disgusting, need a better way
		EntityId pSecWeaponInventory[eRGT_COUNT] =
		{
			NULL, // eRGT_ANY (if passed in as preferred type, will be skipped)
			pInventory->GetItemByClass(pRegistery->FindClass("AISmokeGrenades")),
			pInventory->GetItemByClass(pRegistery->FindClass("AIFlashbangs")),
			pInventory->GetItemByClass(pRegistery->FindClass("AIGrenades")),
			pInventory->GetItemByClass(pRegistery->FindClass("AIEMPGrenades")),
		};

		// Check requested type first
		itemEntity = pSecWeaponInventory[prefGrenadeType];
		if (!itemEntity)
		{
			itemEntity = pSecWeaponInventory[eRGT_FRAG_GRENADE];
		}
		if (!itemEntity)
		{
			itemEntity = pSecWeaponInventory[eRGT_FLASHBANG_GRENADE];
		}
		if (!itemEntity)
		{
			itemEntity = pSecWeaponInventory[eRGT_SMOKE_GRENADE];
		}
		if (!itemEntity)
		{
			itemEntity = pSecWeaponInventory[eRGT_EMP_GRENADE];
		}
	}

	// Get weapon if we have something in our inventory
	if (itemEntity)
	{
		if (IItem *pItem = CCryAction::GetCryAction()->GetIItemSystem()->GetItem(itemEntity))
			return pItem->GetIWeapon();
	}

	return 0;
}

IFireController* CAIProxy::GetFireController()
{
	IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();

	if (IVehicle* pVehicle = (IVehicle*) pVehicleSystem->GetVehicle(m_pGameObject->GetEntityId()))
		return pVehicle->GetFireController();

	return NULL;
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::IsEnabled() const
{
	return !m_IsDisabled;
}


//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::EnableUpdate(bool enable)
{
	// DO NOT:
	//  1. call SetAIActivation on GameObject
	//  2. call Activate on Entity
	// (this will result in an infinite-ish loop)

	bool bNotifyListeners = false;

	if(enable)
	{
		if (m_IsDisabled) bNotifyListeners = true;

		IAIObject* pAI = m_pGameObject->GetEntity()->GetAI();
		m_IsDisabled=false;
		if (pAI)
			pAI->Event(AIEVENT_ENABLE, NULL);
	}
	else
	{
		if (!m_IsDisabled) bNotifyListeners = true;

		IAIObject* pAI = m_pGameObject->GetEntity()->GetAI();
		m_IsDisabled=true;
		if (pAI)
			pAI->Event(AIEVENT_DISABLE, NULL);
	}

	// Notify listeners
	if (bNotifyListeners)
	{
		TListeners::const_iterator listener = m_listeners.begin();
		TListeners::const_iterator listenerEnd = m_listeners.end();
		for (; listener != listenerEnd; ++listener)
		{
			(*listener)->OnAIProxyEnabled(!m_IsDisabled);
		}
	}

	if (IActor* pActor = GetActor())
	{
		pActor->OnAIProxyEnabled(!m_IsDisabled);
	}
}


//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::AddListener(IAIProxyListener *pListener)
{
	CRY_ASSERT(pListener);
	if (std::find(m_listeners.begin(), m_listeners.end(), pListener) == m_listeners.end())
	{
		m_listeners.push_back(pListener);

		// Notify of current status
		pListener->OnAIProxyEnabled(!m_IsDisabled);
	}
}


//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::RemoveListener(IAIProxyListener *pListener)
{
	CRY_ASSERT(pListener);
	stl::find_and_erase(m_listeners, pListener);
}


//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::UpdateMeAlways(bool doUpdateMeAlways) 
{
	m_UpdateAlways=doUpdateMeAlways;
	CheckUpdateStatus();
}


//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::CheckUpdateStatus()
{
	bool update = false;
	// DO NOT call Activate on Entity
	// (this will result in an infinite-ish loop)
	if (gEnv->pAISystem->GetUpdateAllAlways() || m_UpdateAlways)
		update = m_pGameObject->SetAIActivation(eGOAIAM_Always);
	else
		update = m_pGameObject->SetAIActivation(eGOAIAM_VisibleOrInRange);
	return update;
}

//
//----------------------------------------------------------------------------------------------------------
int CAIProxy::Update(SOBJECTSTATE *pStates, bool bFullUpdate)
{
	FUNCTION_PROFILER( gEnv->pSystem, PROFILE_AI );

	if(!CheckUpdateStatus())
		return 0;

	if( !m_pAIHandler )
		return 0;

	// Márcio: temporary workaround
	m_pAIHandler->UpdateReadability();

	if (bFullUpdate)
		UpdateMind(pStates);

	if(GetActorHealth()<=0 && GetPhysics()==NULL)
	{
		// killed actor is updated by AI ?
		return 0;
	}

	IActor*	pActor = GetActor();

	if(m_PrevAlertness==0 && GetAlertnessState()!=0)
	{

		// speed up transitions when getting alerted (idle-combat)
		{
			if (pActor != NULL)
				pActor->GetAnimationGraphState()->Hurry();
		}
	}
	m_PrevAlertness=GetAlertnessState();

	bool fire = pStates->fire;
	bool fireSec = pStates->fireSecondary;
	bool isAlive(true);


	IMovementController * pMC = m_pGameObject->GetMovementController();
	if (pMC)
	{
		SMovementState moveState;
		pMC->GetMovementState(moveState);
		isAlive = moveState.isAlive;

		if (!isAlive)
		{
			fire = false;
			fireSec = false;
		}

		Vec3 currentPos = m_pGameObject->GetEntity()->GetWorldPos();

		CMovementRequest mr;

		int alertness = GetAlertnessState();
		mr.SetAlertness(alertness);

		// [DAVID] Set Actor/AC alertness
		if (pActor != NULL)
			pActor->SetFacialAlertnessLevel(alertness);

		if( !pStates->vLookTargetPos.IsZero() )
			mr.SetLookTarget( pStates->vLookTargetPos );
		else
			mr.ClearLookTarget();

		mr.SetLookStyle( pStates->eLookStyle );

		if(fabsf(pStates->lean) > 0.01f)
			mr.SetLean(pStates->lean);
		else
			mr.ClearLean();

		if(fabsf(pStates->peekOver) > 0.01f)
			mr.SetPeekOver(pStates->peekOver);
		else
			mr.ClearPeekOver();

		// it is possible that we have a move target but zero speed, in which case we want to turn
		// on the spot.
		bool wantsToMove = !pStates->vMoveDir.IsZero();
		if(m_pAIHandler && m_pAIHandler->IsAnimationBlockingMovement())
			wantsToMove = false;

		if (!wantsToMove)
		{
			mr.ClearMoveTarget();
			mr.SetDesiredSpeed(0.0f);
			mr.SetPseudoSpeed(pStates->curActorTargetPhase == eATP_Waiting ? fabs(pStates->fMovementUrgency) : 0.0f);
			mr.ClearPrediction();
		}
		else
		{
			if (pStates->predictedCharacterStates.nStates > 0)
			{
				int num = min((int) pStates->predictedCharacterStates.nStates, (int) SPredictedCharacterStates::maxStates);
				SPredictedCharacterStates predStates;
				predStates.nStates = num;
				pe_status_pos status;
				GetPhysics()->GetStatus(&status);
				for (int iPred = 0 ; iPred != num ; ++iPred)
				{
					SAIPredictedCharacterState &pred = pStates->predictedCharacterStates.states[iPred];
					predStates.states[iPred].deltatime = pred.predictionTime;
					predStates.states[iPred].position = pred.position;
					predStates.states[iPred].velocity = pred.velocity;
					//predStates.states[iPred].rotation.SetIdentity();
					if (pStates->allowStrafing || pred.velocity.IsZero(0.01f))
						predStates.states[iPred].orientation = status.q;
					else
						predStates.states[iPred].orientation.SetRotationV0V1(Vec3(0, 1, 0), pred.velocity.GetNormalized());
				}
				mr.SetPrediction(predStates);
			}
			else
			{
				mr.ClearPrediction();
			}

			// If no real move target is available
			if (pStates->vMoveTarget.IsZero())
			{
				// OLD behavior: Estimate a future position (1m away based on velocity)
				mr.SetMoveTarget(currentPos + pStates->vMoveDir.GetNormalized());
			}
			else
			{
				// NEW behavior: Provide exact move target and inflection point based on real PathFollower data.
				mr.SetMoveTarget(pStates->vMoveTarget);
				mr.SetInflectionPoint(pStates->vInflectionPoint);
			}

			if (fabs(pStates->fDesiredSpeed * pStates->fMovementUrgency) > 0.00001f)
			{
				mr.SetDesiredSpeed( pStates->fMovementUrgency > 0.0f ? pStates->fDesiredSpeed : -pStates->fDesiredSpeed);
				mr.SetPseudoSpeed( fabs(pStates->fMovementUrgency) );
			}
			else
			{
				mr.SetDesiredSpeed(0.0f);
				mr.SetPseudoSpeed(0.0f);
			}
		}

		if(m_pAIHandler)
			m_pAIHandler->HandleExactPositioning(pStates, mr);

		mr.SetDistanceToPathEnd( pStates->fDistanceToPathEnd );
		mr.SetStance((EStance)pStates->bodystate);

		if (pStates->jump)
			mr.SetJump();

		if (pStates->aimTargetIsValid && !pStates->vAimTargetPos.IsZero())
			mr.SetAimTarget(pStates->vAimTargetPos);
		else
			mr.ClearAimTarget();

		if (fire || fireSec || pStates->fireMelee)
			mr.SetFireTarget( pStates->vShootTargetPos );
		else 
			mr.ClearFireTarget();

		mr.SetAllowStrafing( pStates->allowStrafing );

		if (pStates->vForcedNavigation.IsZero())
			mr.ClearForcedNavigation();
		else
			mr.SetForcedNavigation(pStates->vForcedNavigation);

		if (pStates->vBodyTargetDir.IsZero())
			mr.ClearBodyTarget();
		else
			mr.SetBodyTarget(currentPos + pStates->vBodyTargetDir);

		// try, and retry once
		if (!pMC->RequestMovement( mr ))
			pMC->RequestMovement( mr );
	}

	UpdateShooting(pStates, isAlive);

	// NOTE: This is not supported anymore in AnimatedCharacter, so it should be removed here.
	if (IAnimatedCharacter * pAnimChar = (m_pIActor != NULL) ? m_pIActor->GetAnimatedCharacter() : 0)
	{
		SAnimatedCharacterParams params = pAnimChar->GetParams();

		// NOTE: This is not supported anymore in AnimatedCharacter, so it should be removed here.
		if (pStates->allowEntityClampingByAnimation)
			params.flags |= eACF_AllowEntityClampingByAnimation;
		else
			params.flags &= ~eACF_AllowEntityClampingByAnimation;
		pAnimChar->SetParams( params );
	}

	return 0;
}


//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::UpdateShooting(SOBJECTSTATE *pStates, bool isAlive)
{
	m_WeaponShotIsDone = false;

	bool fire = pStates->fire && !pStates->aimObstructed;//|| (m_StartWeaponSpinUpTime>0);
	bool fireSec = pStates->fireSecondary;
	if(!isAlive)
	{
		fire = false;
		fireSec = false;
	}

	if(pStates->fireMelee)
	{
		if(isAlive)
		{
			if (IWeapon * pWeapon = GetCurrentWeapon())
				pWeapon->MeleeAttack();
		}
		fire = false;
		fireSec = false;
	}

	//---------------------------------------------------------------------------
	IWeapon *pWeapon = GetCurrentWeapon();
	if (pWeapon)
	{
		IFireMode *pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
		if (pFireMode)
			m_firing = pFireMode->IsFiring();
		bool raise = pStates->aimObstructed;
		pWeapon->Query(eWQ_Raise_Weapon, (const void*)&raise);
	}

	// firing control
	if (fire && !m_firing)
	{
		if (pWeapon && pWeapon->CanFire())
		{

			pWeapon->StartFire( &(pStates->vShootTargetPos) );
			m_firing = true;
		}
		else
		{
			if (IFireController* pFireController = GetFireController())
			{
				m_firing = true;
				pFireController->RequestFire(m_firing);
			}
		}
	}
	else if (!fire && m_firing)
	{
		if (pWeapon)
		{
			pWeapon->StopFire();
			m_firing = false;
		}
		else
		{
			if (IFireController* pFireController = GetFireController())
			{
				m_firing = false;
				pFireController->RequestFire(m_firing);
			}
		}
	}

	// secondary weapon firing control
	if (fireSec && !m_firingSecondary)
	{
		if (IWeapon *pSecWeapon=GetSecWeapon( pStates->requestedGrenadeType ))
		{
			SProjectileThrowParams throwParams;
			throwParams.fSpeedScale = pStates->fProjectileSpeedScale;
			throwParams.vThrowPoint = pStates->vShootTargetPos;
			pSecWeapon->PerformThrow(throwParams,GetActor());

			m_firingSecondary = true;

			IEntity* pEntity = m_pGameObject->GetEntity();
			if (pEntity)
			{
				// Signal to self to inform grenade is now being thrown
				IAISignalExtraData *pData = gEnv->pAISystem->CreateSignalExtraData();
				pData->iValue = pStates->requestedGrenadeType;
				pData->fValue = pStates->fProjectileSpeedScale;
				pData->point = pStates->vShootTargetPos;
				SendSignal(0, "OnGrenadeThrown", pEntity, pData);

#ifndef SP_DEMO
				if(pStates->requestedGrenadeType == eRGT_SMOKE_GRENADE)
				{
					SendSignal(0, "OnSmokeGrenadeThrown", pEntity, NULL);
					IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
					if(pAIActor)
						pAIActor->NotifySignalReceived( "OnSmokeGrenadeThrown" );
				}
#endif
			}
		}
	}
	else if (!fireSec && m_firingSecondary)
	{
		m_firingSecondary=false;
	}
}


//
//----------------------------------------------------------------------------------------------------------
void  CAIProxy::QueryWeaponInfo(SAIWeaponInfo& weaponInfo)
{
	IWeapon *pWeapon(GetCurrentWeapon());
	if(!pWeapon)
		return;

	weaponInfo.canMelee = pWeapon->CanMeleeAttack();
	weaponInfo.outOfAmmo = pWeapon->OutOfAmmo(false);
	weaponInfo.lowAmmo = pWeapon->LowAmmo(0.15f);
	weaponInfo.shotIsDone = m_WeaponShotIsDone;
	weaponInfo.lastShotTime = m_lastShotTime;

#ifdef AI_G4
	weaponInfo.hasLightAccessory = weaponInfo.hasLaserAccessory = false;
	weaponInfo.hasLightAccessory = weaponInfo.hasLightAccessory = false;
#else
	weaponInfo.hasLaserAccessory = pWeapon->Query(eWQ_Has_Accessory_Laser);
	weaponInfo.hasLightAccessory = pWeapon->Query(eWQ_Has_Accessory_Flashlight);
#endif

	if (IFireMode* pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode()))
	{
		weaponInfo.ammoCount = pFireMode->GetAmmoCount();
		weaponInfo.clipSize = pFireMode->GetClipSize();
		weaponInfo.isReloading = pFireMode ? pFireMode->IsReloading() : false;
		weaponInfo.isFiring = pFireMode ? pFireMode->IsFiring() : false;
	}
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::EnableWeaponListener(bool needsSignal)
{
	m_shotBulletCount = 0;
	m_lastShotTime.SetSeconds(0.0f);
	m_NeedsShootSignal = needsSignal;
	if (IWeapon * pWeapon = GetCurrentWeapon())
		pWeapon->AddEventListener(this, __FUNCTION__);
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,
					   const Vec3 &pos, const Vec3 &dir, const Vec3 &vel)
{
	IEntity* pEntity = m_pGameObject->GetEntity();
	if(!pEntity)
	{
		gEnv->pAISystem->Warning(">> ","CAIProxy::OnShoot null entity");
		return;
	}

//  	if (pEntity->GetAI() && pEntity->GetAI()->GetAIType() != AIOBJECT_PLAYER)
//  		gEnv->pAISystem->DebugDrawFakeTracer(pos, dir);

	m_shotBulletCount++;
	m_lastShotTime = gEnv->pTimer->GetFrameStartTime();

	m_WeaponShotIsDone = true;
	IPuppet *pPuppet = CastToIPuppetSafe(pEntity->GetAI());
	if(pPuppet && pPuppet->GetFirecommandHandler())
		pPuppet->GetFirecommandHandler()->OnShoot();

	if(!m_NeedsShootSignal)
		return;
	SendSignal(0, "WPN_SHOOT", pEntity, NULL);
	IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
	if(pAIActor)
		pAIActor->NotifySignalReceived( "WPN_SHOOT" );
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::OnDropped(IWeapon *pWeapon, EntityId actorId)
{
	m_NeedsShootSignal = false;
	if (pWeapon)
		pWeapon->RemoveEventListener(this);
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::OnSelected(IWeapon *pWeapon, bool selected)
{
	if (!selected)
	{
		m_NeedsShootSignal = false;
		if (pWeapon)
			pWeapon->RemoveEventListener(this);
	}
}

//
//----------------------------------------------------------------------------------------------------------
IPhysicalEntity* CAIProxy::GetPhysics(bool wantCharacterPhysics)
{
	if( wantCharacterPhysics )
	{
		ICharacterInstance* pCharacter(m_pGameObject->GetEntity()->GetCharacter(0));
		if(pCharacter)
			return pCharacter->GetISkeletonPose()->GetCharacterPhysics();
		return NULL;
	}
	return m_pGameObject->GetEntity()->GetPhysics();
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::DebugDraw(int iParam)
{
	if( !m_pAIHandler )
		return;

	IAIDebugRenderer* pDebugRenderer = gEnv->pAISystem->GetAIDebugRenderer();

	if( iParam == 1 )
	{
		// Stats target drawing.
		pDebugRenderer->TextToScreenColor(50,66,0.3f,0.3f,0.3f,1.f,"- Proxy Information --");

		const char *szCurrentBehaviour=0;
		const char *szPreviousBehaviour=0;
		const char *szFirstBehaviour=0;

		if (m_pAIHandler->m_pBehavior.GetPtr())
			m_pAIHandler->m_pBehavior->GetValue("Name",szCurrentBehaviour);

		if (m_pAIHandler->m_pPreviousBehavior.GetPtr())
			m_pAIHandler->m_pPreviousBehavior->GetValue("Name",szPreviousBehaviour);

		if (!m_pAIHandler->m_sFirstBehaviorName.empty())
			szFirstBehaviour = m_pAIHandler->m_sFirstBehaviorName.c_str();


		pDebugRenderer->TextToScreen(50,74,"BEHAVIOUR: %s",szCurrentBehaviour);
		pDebugRenderer->TextToScreen(50,76," PREVIOUS BEHAVIOUR: %s",szPreviousBehaviour);
		pDebugRenderer->TextToScreen(50,78," DESIGNER ASSIGNED BEHAVIOUR: %s",szFirstBehaviour);


		const char *szCurrentCharacter = m_pAIHandler->m_sCharacterName.c_str();
		const char *szPreviousCharacter = m_pAIHandler->m_sPrevCharacterName.c_str();
		const char *szFirstCharacter = m_pAIHandler->m_sFirstCharacterName.c_str();

		pDebugRenderer->TextToScreen(50,82,"CHARACTER: %s",szCurrentCharacter);
		pDebugRenderer->TextToScreen(50,84," PREVIOUS CHARACTER: %s",szPreviousCharacter);
		pDebugRenderer->TextToScreen(50,86," DESIGNER ASSIGNED CHARACTER: %s",szFirstCharacter);
		if( !m_readibilityName.empty() )
		{
			const float timeLeft = (m_readibilityDelay - gEnv->pAISystem->GetFrameStartTime()).GetSeconds();
			if( timeLeft > 0.0f )
				pDebugRenderer->TextToScreen(50,88," READIBILITY: %.1fs -> (%s) [%d]", timeLeft, m_readibilityName.c_str(), m_readibilityIgnored);
			else
				pDebugRenderer->TextToScreen(50,88," READIBILITY: %s [%d]", m_readibilityName.c_str(), m_readibilityIgnored);
		}


		if(m_pAIHandler)
		{
			const char*	szEPMsg = " - ";
			switch(GetActorTargetPhase())
			{
				//			case eATP_Cancel: szEPMsg = "Cancel"; break;
			case eATP_None: szEPMsg = " - "; break;
				//			case eATP_Request: szEPMsg = "Request"; break;
			case eATP_Waiting: szEPMsg = "Waiting"; break;
			case eATP_Starting: szEPMsg = "Starting"; break;
			case eATP_Started: szEPMsg = "Started"; break;
			case eATP_Playing: szEPMsg = "Playing"; break;
			case eATP_StartedAndFinished: szEPMsg = "Started/Finished"; break;
			case eATP_Finished: szEPMsg = "Finished"; break;
			case eATP_Error: szEPMsg = "Error"; break;
			}

			const char*	curSignalAnim = m_pAIHandler->DEBUG_GetCurrentSignaAnimationName();
			const char*	curActionAnim = m_pAIHandler->DEBUG_GetCurrentActionAnimationName();

			pDebugRenderer->TextToScreen(50,90,"ANIMATION: %s", m_pAIHandler->IsAnimationBlockingMovement() ? "MOVEMENT BLOCKED!" : "");
			pDebugRenderer->TextToScreen(50,92,"           ExactPos:%s", szEPMsg);
			pDebugRenderer->TextToScreen(50,94,"           Signal:%s", curSignalAnim ? curSignalAnim : "-");
			pDebugRenderer->TextToScreen(50,96,"           Action:%s", curActionAnim ? curActionAnim : "-");
		}
	}
	else if( iParam == 2 )
	{
		if( !m_readibilityName.empty() )
		{
			SAIBodyInfo bodyInfo;
			QueryBodyInfo( bodyInfo );

			ColorB color(235, 255, 0);
			const float timeLeft = (m_readibilityDelay - gEnv->pAISystem->GetFrameStartTime()).GetSeconds();
			if( timeLeft > 0.0f )
				color.Set(255, 255, 255, 26);
			pDebugRenderer->Draw3dLabelEx( bodyInfo.vEyePos + Vec3( 0, 0, 2 ), 1, color, true, false, m_readibilityName.c_str() );
		}
	}
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::Reset(EObjectResetType type)
{
	//m_forcedExecute = 0;
	m_IsDisabled = false;
	m_shotBulletCount = 0;
	m_lastShotTime.SetSeconds(0.0f);

	//if (gEnv->IsEditor())
	m_pGameObject->SetAIActivation(eGOAIAM_VisibleOrInRange); // we suppose the first update will set this properly.

	m_readibilityName.clear();
	m_readibilityDelay.SetSeconds(0.0f);
	m_readibilityPriority = 0;
	m_readibilityId = 0;
	m_readibilityIgnored = 0;
	m_readibilityPlayed = false;

	m_commHandler->Reset();

	if(m_pAIHandler)
		m_pAIHandler->Reset(type);

	IMovementController * pMC = m_pGameObject->GetMovementController();
	if (pMC)
	{
		CMovementRequest mr;
		mr.SetAlertness(0);

		// [DAVID] Clear Actor/AC alertness
		IActor* pActor = GetActor();
		if (pActor != NULL)
			pActor->SetFacialAlertnessLevel(0);

		mr.ClearLookTarget();
		mr.ClearActorTarget();
		mr.ClearLean();
		mr.ClearPeekOver();
		mr.ClearPrediction();
		// try, and retry once
		if (!pMC->RequestMovement( mr ))
			pMC->RequestMovement( mr );
	}

	// Notify signaler handlers
	CRY_ASSERT(m_pGameObject);
	CCryAction::GetCryAction()->GetRangeSignaling()->OnProxyReset(m_pGameObject->GetEntityId());
	CCryAction::GetCryAction()->GetSignalTimer()->OnProxyReset(m_pGameObject->GetEntityId());
}

IAICommunicationHandler* CAIProxy::GetCommunicationHandler()
{
	return m_commHandler.get();
}

void CAIProxy::SetForcedExecute(bool forced)
{
	m_forcedExecute += forced ? 1 : -1;
	assert(m_forcedExecute >= 0);	// Below zero? More disables then enables!
	assert(m_forcedExecute < 16); // Just a little sanity check

	if (IEntity* pEntity = m_pGameObject->GetEntity())
	{
		IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
		if (pAIActor)
		{
			if (forced)
				pAIActor->SetSignal(10, "OnForcedExecute", pEntity);
			else if (m_forcedExecute <= 0)
				pAIActor->SetSignal(10, "OnForcedExecuteComplete", pEntity);
		}
	}
}

bool CAIProxy::IsForcedExecute() const
{
	return m_forcedExecute > 0;
}

//
//----------------------------------------------------------------------------------------------------------
CTimeValue CAIProxy::GetEstimatedAGAnimationLength(EAIAGInput input, const char* value)
{
	if ( m_pAIHandler ) 
		return m_pAIHandler->GetEstimatedAGAnimationLength( input, value );
	else
		return CTimeValue();
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::SetAGInput(EAIAGInput input, const char* value)
{
	return (m_pAIHandler ? m_pAIHandler->SetAGInput(input, value) : false);
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::ResetAGInput(EAIAGInput input)
{
	return (m_pAIHandler ? m_pAIHandler->ResetAGInput(input) : false);
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::IsSignalAnimationPlayed( const char* value )
{
	return (m_pAIHandler ? m_pAIHandler->IsSignalAnimationPlayed(value) : true);
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::IsActionAnimationStarted( const char* value )
{
	return (m_pAIHandler ? m_pAIHandler->IsActionAnimationStarted(value) : true);
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::IsAnimationBlockingMovement() const
{
	return (m_pAIHandler ? m_pAIHandler->IsAnimationBlockingMovement() : false);
}

//
//----------------------------------------------------------------------------------------------------------
EActorTargetPhase CAIProxy::GetActorTargetPhase() const
{
	return (m_pAIHandler ? m_pAIHandler->GetActorTargetPhase() : eATP_None);
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::PlayAnimationAction( const IAIAction* pAction, int goalPipeId )
{
	CRY_ASSERT( pAction->GetAnimationName() != NULL && pAction->GetAnimationName()[0] != 0 );

	if ( m_animActionGoalPipeId != 0 )
		gEnv->pAISystem->GetAIActionManager()->AbortAIAction( m_pGameObject->GetEntity(), m_animActionGoalPipeId );
	m_animActionGoalPipeId = goalPipeId;

	IActor* pActor = GetActor();
	if (pActor != NULL)
	{
		if ( pAction->IsExactPositioning() )
			pActor->PlayExactPositioningAnimation( pAction->GetAnimationName(), pAction->IsSignaledAnimation(), pAction->GetAnimationPos(), 
			pAction->GetAnimationDir(), pAction->GetStartWidth(), pAction->GetStartArcAngle(), DEG2RAD(pAction->GetDirectionTolerance()) );
		else
			pActor->PlayAnimation( pAction->GetAnimationName(), pAction->IsSignaledAnimation() );
	}
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::AnimationActionDone( bool succeeded )
{
	if ( m_animActionGoalPipeId )
	{
		int tmp = m_animActionGoalPipeId;
		m_animActionGoalPipeId = 0;
		if ( succeeded )
			gEnv->pAISystem->GetAIActionManager()->FinishAIAction( m_pGameObject->GetEntity(), tmp );
		else
			gEnv->pAISystem->GetAIActionManager()->AbortAIAction( m_pGameObject->GetEntity(), tmp );
	}
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::UpdateMind(SOBJECTSTATE *state)
{

	// (MATT) Run delayed constructors on behaviour and character and update face {2008/09/05}
	if (m_pAIHandler)
		m_pAIHandler->Update();

	// (MATT) For signals that should wait until next update, mainly used by Troopers {2008/09/05}
	std::vector<AISIGNAL> nextUpdateSignals;

	if ( m_pGameObject->GetEntity()->GetAI()->IsUpdatedOnce() )
	{
		while (!state->vSignals.empty())
		{
			AISIGNAL sstruct = state->vSignals.front();
			state->vSignals.erase(0);

			int signal = sstruct.nSignal;
			if(signal == AISIGNAL_PROCESS_NEXT_UPDATE)
			{
				sstruct.nSignal = AISIGNAL_RECEIVED_PREV_UPDATE;
				nextUpdateSignals.push_back(sstruct);
			}
			else
			{
				const char *szText = sstruct.strText;
				IEntity* pSender = gEnv->pEntitySystem->GetEntity(sstruct.senderID);
				SendSignal(signal, szText, pSender, sstruct.pEData);

				if (sstruct.pEData)
					gEnv->pAISystem->FreeSignalExtraData( sstruct.pEData );
			}
		}

		std::vector<AISIGNAL>::iterator it = nextUpdateSignals.begin(), itEnd = nextUpdateSignals.end();
		for(; it!=itEnd; ++it)
			state->vSignals.push_back(*it);
	}
	else
	{
		// (MATT) Process init signals before first update - is this really needed? {2008/09/05}
		int size = state->vSignals.size();
		int i = 0;
		while ( i < size  )
		{
			AISIGNAL signal = state->vSignals[i];

			if ( signal.nSignal == -100 )
			{
				state->vSignals.erase( state->vSignals.begin() + i );
				gEnv->pLog->LogError("Initialisation signal used \"%s\"", signal.strText);

				// process only init. signals!
				IEntity* pSender = gEnv->pEntitySystem->GetEntity(signal.senderID);
				SendSignal( signal.nSignal, signal.strText, pSender, signal.pEData );
				if ( signal.pEData )
					gEnv->pAISystem->FreeSignalExtraData( signal.pEData );

				// TO DO: check if this is still necessary once the newly inserted signals will be processed on the next update
				if ( --size != state->vSignals.size() )
				{
					// a new signal was inserted while processing this one
					// so, let's start over
					size = state->vSignals.size();
				}
			}
			else
				i++;
		}
	}

	// (MATT) Readabilities  {2008/09/05}
	UpdateAuxSignal(state);

	if (!m_pAIHandler)
		return;

	m_pAIHandler->AIMind(state);

	if (m_IsWeaponForceAlerted != state->forceWeaponAlertness)
	{
		// (MATT) Sets WeaponAlertness to 0 {2008/09/05}
		m_pAIHandler->UpdateWeaponAlertness();
		m_IsWeaponForceAlerted = state->forceWeaponAlertness;
	}
}


//
//----------------------------------------------------------------------------------------------------------
IActor*	CAIProxy::GetActor() const
{
	if (!m_pIActor)
		m_pIActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_pGameObject->GetEntityId());
	return m_pIActor;
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::SendSignal(int signalID, const char * szText, IEntity *pSender, const IAISignalExtraData* pData)
{
	if( !m_pAIHandler )
		return;
	m_pAIHandler->AISignal( signalID, szText, pSender, pData );
}

// Updates the readibility signal handling, filters signals and calls the IAHandler to player when ready.
//----------------------------------------------------------------------------------------------------------
void CAIProxy::UpdateAuxSignal(SOBJECTSTATE* pStates)
{
	if (!m_pAIHandler)
		return;

	// Check to process a new signal.
	if (pStates->nAuxSignal)
	{
		// Filter out overlapping readibilities.
		bool	allowPlay = true;
		if (!m_readibilityName.empty())
		{
			// If the readibility has same priority or lower, skip it.
			if (pStates->nAuxPriority <= m_readibilityPriority || pStates->nAuxPriority >= 100)
				allowPlay = false;
		}

		if (allowPlay)
		{
			// No readibility playing currently, play the new one.
			m_readibilityName = pStates->szAuxSignalText;
			m_readibilityDelay = gEnv->pAISystem->GetFrameStartTime() + pStates->fAuxDelay;
			m_readibilityId = pStates->nAuxSignal;
			m_readibilityPriority = pStates->nAuxPriority;
			m_readibilityIgnored = 0;
			m_readibilityPlayed = false;
			gEnv->pAISystem->LogComment("AIProxy", "UpdateAuxSignal: Playing %s delay %f", m_readibilityName.c_str(), m_readibilityDelay.GetSeconds() );
		}
		else
		{
			gEnv->pAISystem->LogComment("AIProxy", "UpdateAuxSignal: Ignored %s (pri=%d) playing %s (pri=%d)", pStates->szAuxSignalText.c_str(), pStates->nAuxPriority, m_readibilityName.c_str(), m_readibilityPriority );
			m_readibilityIgnored++;
		}

		// Mark the aux signal as handled.
		pStates->nAuxSignal = 0;
	}

	// Update current readability
	if (!m_readibilityName.empty())
	{
		if (!m_readibilityPlayed)
		{
			if (gEnv->pAISystem->GetFrameStartTime() > m_readibilityDelay)
			{
				if (m_pAIHandler->DoReadibilityPack(m_readibilityName.c_str(), m_readibilityId))
				{
					// Played the readibility, wait for the sound event to completed.
					m_readibilityPlayed = true;
				}
				else
				{
					// Failed to play the readibility.
					m_readibilityName.clear();
					m_readibilityPlayed = false;
					m_readibilityDelay.SetSeconds(0.0f);
					m_readibilityId = 0;
					m_readibilityPriority = 0;
				}
			}
		}
		else
		{
			if (m_pAIHandler->HasReadibilitySoundFinished())
			{
				m_readibilityName.clear();
				m_readibilityPlayed = false;
				m_readibilityDelay.SetSeconds(0.0f);
				m_readibilityId = 0;
				m_readibilityPriority = 0;
			}
		}
	}
}

//
// gets the corners of the tightest projected bounding rectangle in 2D world space coordinates
//----------------------------------------------------------------------------------------------------------
void CAIProxy::GetWorldBoundingRect(Vec3& FL, Vec3& FR, Vec3& BL, Vec3& BR, float extra) const
{
	const Vec3 pos = m_pGameObject->GetEntity()->GetWorldPos();
	const Quat &quat = m_pGameObject->GetEntity()->GetRotation();
	Vec3 dir = quat * Vec3(0, 1, 0);
	dir.z = 0.0f;
	dir.NormalizeSafe();

	const Vec3 sideDir(dir.y, -dir.x, 0.0f);

	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)m_pGameObject->GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
	AABB bounds;
	pRenderProxy->GetLocalBounds(bounds);

	bounds.max.x += extra;
	bounds.max.y += extra;
	bounds.min.x -= extra;
	bounds.min.y -= extra;

	FL = pos + bounds.max.y * dir + bounds.min.x * sideDir;
	FR = pos + bounds.max.y * dir + bounds.max.x * sideDir;
	BL = pos + bounds.min.y * dir + bounds.min.x * sideDir;
	BR = pos + bounds.min.y * dir + bounds.max.x * sideDir;
}

//
//----------------------------------------------------------------------------------------------------------
int CAIProxy::GetAlertnessState() const
{
	if( !m_pAIHandler )
		return 0;
	return m_pAIHandler->m_CurrentAlertness;
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::IsCurrentBehaviorExclusive() const
{
	if( !m_pAIHandler )
		return false;
	return m_pAIHandler->m_CurrentExclusive;
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::SetCharacter(const char* character, const char* behavior)
{
	if (!m_pAIHandler)
		return true;
	bool ret =  m_pAIHandler->SetCharacter(character);
	if (behavior)
		m_pAIHandler->SetBehavior(behavior);
	return ret;
}

//
//----------------------------------------------------------------------------------------------------------
const char* CAIProxy::GetCharacter()
{
	if (!m_pAIHandler)
		return 0;
	return m_pAIHandler->GetCharacter();
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::TestReadabilityPack(bool start, const char* szReadability)
{
	m_pAIHandler->TestReadabilityPack(start, szReadability);
}

//
//----------------------------------------------------------------------------------------------------------
const char* CAIProxy::GetCurrentReadibilityName() const
{
	if(m_readibilityName.empty())
		return 0;
	else
		return m_readibilityName.c_str();
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::GetReadabilityBlockingParams(const char* text, float& time, int& id)
{
	if(!m_pAIHandler)
	{
		time = 0;
		id = 0;
		return;
	}
	CAIHandler::s_ReadabilityManager.GetReadabilityBlockingParams(m_pAIHandler, text, time ,id);
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::SetReadabilitySound(tSoundID nSoundId, bool bStopPrevious)
{
	// Stop the previously playing sound.
	if(m_pAIHandler && bStopPrevious == true && m_pAIHandler->m_ReadibilitySoundID != INVALID_SOUNDID)
	{
		IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*) m_pGameObject->GetEntity()->GetProxy( ENTITY_PROXY_SOUND );
		pSoundProxy->StopSound(m_pAIHandler->m_ReadibilitySoundID);

		m_pAIHandler->m_ReadibilitySoundID = INVALID_SOUNDID;
	}
}

//
//----------------------------------------------------------------------------------------------------------
void CAIProxy::Serialize( TSerialize ser )
{
	if (ser.IsReading())
	{
		m_pIActor = 0; // forces re-caching in QueryBodyInfo

		if (!m_pAIHandler)
		{
			m_pAIHandler = new CAIHandler(m_pGameObject);
			m_pAIHandler->Init(GetISystem());
		}
	}

	CRY_ASSERT(m_pAIHandler);

	ser.BeginGroup( "AIProxy" );

	m_pAIHandler->Serialize(ser);
	ser.Value("m_firing",m_firing);
	ser.Value("m_firingSecondary",m_firingSecondary);
	ser.Value("m_readibilityName",m_readibilityName);

	if (ser.IsReading())
	{
		// Read in time left and apply it to the delay
		float timeLeft = 0.0f;
		ser.Value("m_readibilityDelay",timeLeft);
		m_readibilityDelay = gEnv->pAISystem->GetFrameStartTime() + timeLeft;
	}
	else
	{
		// Serialize time left
		float timeLeft = (m_readibilityDelay - gEnv->pAISystem->GetFrameStartTime()).GetSeconds();
		ser.Value("m_readibilityDelay",timeLeft);
	}

	ser.Value("m_readibilityPlayed",m_readibilityPlayed);
	ser.Value("m_readibilityPriority",m_readibilityPriority);
	ser.Value("m_readibilityId",m_readibilityId);
	ser.Value("m_readibilityIgnored",m_readibilityIgnored);
	ser.Value("m_fMinFireTime",m_fMinFireTime);
	ser.Value("m_WeaponShotIsDone",m_WeaponShotIsDone);
	ser.Value("m_NeedsShootSignal",m_NeedsShootSignal);
	ser.Value("m_PrevAlertness",m_PrevAlertness);
	ser.Value("m_IsDisabled",m_IsDisabled);
	ser.Value("m_UpdateAlways",m_UpdateAlways);
	ser.Value("m_IsWeaponForceAlerted",m_IsWeaponForceAlerted);
	ser.Value("m_lastShotTime", m_lastShotTime);
	ser.Value("m_shotBulletCount", m_shotBulletCount);

	CheckUpdateStatus();

	ser.EndGroup();
}


//
//---------------------------------------------------------------------------------------------------------------
bool CAIProxy::PredictProjectileHit(const Vec3& throwDir, float vel, Vec3& posOut, float& speedOut,
									ERequestedGrenadeType prefGrenadeType, Vec3* pTrajectory, unsigned int* trajectorySizeInOut)
{
	IWeapon* pGrenadeWeapon = GetSecWeapon(prefGrenadeType);
	if (!pGrenadeWeapon)
		return false;

	SAIBodyInfo bodyInfo;
	QueryBodyInfo(bodyInfo);

	return pGrenadeWeapon->PredictProjectileHit(GetPhysics(), bodyInfo.vFirePos, throwDir, Vec3(0,0,0), vel,
		posOut, speedOut, pTrajectory, trajectorySizeInOut);
}

AIWeaponDescriptor CAIProxy::GetSecWeaponDescriptor(ERequestedGrenadeType prefGrenadeType=eRGT_ANY) const
{
	IWeapon* pGrenadeWeapon = GetSecWeapon(prefGrenadeType);
	if (!pGrenadeWeapon)
		return AIWeaponDescriptor();

	return pGrenadeWeapon->GetAIWeaponDescriptor();
}

//
//---------------------------------------------------------------------------------------------------------------
const char* CAIProxy::GetCurrentBehaviorName() const
{
	const char *szName = NULL;

	if (m_pAIHandler->m_pBehavior.GetPtr())
	{
		m_pAIHandler->m_pBehavior->GetValue("Name",szName);
	}

	return(szName);
}

//
//---------------------------------------------------------------------------------------------------------------
const char* CAIProxy::GetPreviousBehaviorName() const
{
	const char *szName = NULL;

	if (m_pAIHandler->m_pPreviousBehavior.GetPtr())
	{
		m_pAIHandler->m_pPreviousBehavior->GetValue("Name",szName);
	}

	return(szName);
}

//
//---------------------------------------------------------------------------------------------------------------
int CAIProxy::GetActorHealth()
{
	IActor*	pActor = GetActor();
	if (pActor != NULL)
		return pActor->GetHealth();

	IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
	IVehicle* pVehicle = pVehicleSystem->GetVehicle(m_pGameObject->GetEntityId());
	if (pVehicle)
		return (int)((1 - pVehicle->GetDamageRatio()) * 1000.0f);

	return 0;
}

//
//---------------------------------------------------------------------------------------------------------------
int CAIProxy::GetActorMaxHealth()
{
	IActor*	pActor = GetActor();
	if (pActor != NULL)
		return pActor->GetMaxHealth();

	IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
	IVehicle* pVehicle = pVehicleSystem->GetVehicle(m_pGameObject->GetEntityId());
	if (pVehicle)
		return 1000;

	return 1;
}

//
//---------------------------------------------------------------------------------------------------------------
int CAIProxy::GetActorArmor()
{
	IActor*	pActor = GetActor();
	if (pActor != NULL)
		return pActor->GetArmor();
	return 0;
}

//
//---------------------------------------------------------------------------------------------------------------
int CAIProxy::GetActorMaxArmor()
{
	IActor*	pActor = GetActor();
	if (pActor != NULL)
		return pActor->GetMaxArmor();
	return 0;
}

//
//---------------------------------------------------------------------------------------------------------------
bool CAIProxy::GetActorIsFallen() const
{
	IActor*	pActor = GetActor();
	if (pActor != NULL)
		return pActor->IsFallen();
	return false;
}

//
//---------------------------------------------------------------------------------------------------------------
bool CAIProxy::IsDead() const
{
	if( const IActor* pActor = (const_cast<CAIProxy*>(this))->GetActor() )
	{
			return pActor->IsDead();
	}
	IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
	IVehicle* pVehicle = pVehicleSystem->GetVehicle(m_pGameObject->GetEntityId());
	if ( pVehicle )
		return pVehicle->IsDestroyed();
	return false;
}

//
//----------------------------------------------------------------------------------------------------------
int CAIProxy::PlayReadabilitySound(const char* szReadability, bool stopPreviousSound, float responseDelay)
{
	if (!m_pAIHandler)
		return INVALID_SOUNDID;
	m_pAIHandler->DoReadibilityPack(szReadability, SIGNALFILTER_READABILITY, true, stopPreviousSound, responseDelay); // Sound only
	return m_pAIHandler->m_ReadibilitySoundID;
}

//
//----------------------------------------------------------------------------------------------------------
bool CAIProxy::QueryCurrentAnimationSpeedRange(float& smin, float& smax)
{
	if (!m_pAIHandler)
		return false;

	ICharacterInstance* pCharacter(m_pGameObject->GetEntity()->GetCharacter(0));
	if (!pCharacter)
		return false;
	ISkeletonAnim* pSkeleton = pCharacter->GetISkeletonAnim();
	if (!pSkeleton)
		return false;

	float weight = 0.0f;
	smin = 0.0f;
	smax = 0.0f;
	for (int i = 0, ni = pSkeleton->GetNumAnimsInFIFO(0); i < ni; ++i)
	{
		const CAnimation& anim = pSkeleton->GetAnimFromFIFO(0, i);
		if (!anim.m_bActivated) continue;

		int id = 0;
		if (anim.m_Parametric.m_nParametricID >= 0)
			id = anim.m_Parametric.m_nParametricID;
		else
			id = anim.m_Parametric.m_nAnimID[0];

		Vec2 range = pCharacter->GetIAnimationSet()->GetMinMaxSpeedAsset_msec(id);
		if (range.x >= 0.0f && range.y >= 0.0f)
		{
			smin += range.x * anim.m_fTransitionWeight;
			smax += range.y * anim.m_fTransitionWeight;
			weight += anim.m_fTransitionWeight;
		}
	}

	if (weight <= 0.0f)
		return false;

	weight = 1.0f / weight;
	smin *= weight;
	smax *= weight;

	if (smin < 0.0f || smax < smin)
		return false;

	return true;
}



void CAIProxy::SetBehaviour(const char* szBehavior, const IAISignalExtraData* pData)
{
	if (m_pAIHandler)
	{
		m_pAIHandler->SetBehavior(szBehavior, pData);
	}
}

// 
//----------------------------------------------------------------------------------------------------------
float CAIProxy::LinearInterp(float x, float k, float A, float B, float C)
{
	// (MATT) Surely this is in CryCommon somewhere {2009/01/27}
	if(x < k)
{
		x = x / k;
		return A + (B - A) * x;
	}
	else
	{
		x = (x - k) / (1.0f - k);
		return B + (C - B) * x;
	}
}

// 
//----------------------------------------------------------------------------------------------------------
float CAIProxy::QuadraticInterp( float x, float k, float A, float B, float C )
{
	// (MATT) Surely this is in CryCommon somewhere {2009/01/27}
	float a = (A - B - A*k + C*k) / (k - k*k);
	float b = -(A - B - A*k*k + C*k*k) / (k - k*k);
	float c = A;


	if(x < 1.0f)
		return a*x*x + b*x + c;
	else
		return (2.0f*a + b)*x + c - a;
}

#include UNIQUE_VIRTUAL_WRAPPER(IAIActorProxy)
