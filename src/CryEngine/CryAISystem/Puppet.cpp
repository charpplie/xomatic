/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   Puppet.cpp
$Id$
Description: Implementation of the CPuppet class.

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "Puppet.h"
#include "AIPlayer.h"
#include "AIVehicle.h"
#include "Leader.h"
#include "GoalOp.h"

#include "AICollision.h"
#include "HideSpot.h"
#include "FlightNavRegion.h"
#include "ObjectTracker.h"
#include "SmartObjects.h"
#include "PathFollower.h"
#include "SmartPathFollower.h"
#include "FireCommand.h"
#include "PerceptionManager.h"

#include "CentralInterestManager.h"
#include "PerceptionHandler/IPerceptionHandler.h"
#include "TargetSelection/TargetTrackManager.h"

#include "PNoise3.h"
#include "ISystem.h"
#include "DebugDrawContext.h"

const float CPuppet::COMBATCLASS_IGNORE_TRESHOLD = 0.0001f;

uint32 CPuppet::m_RWIUsers = 0;

inline float SmoothStep(float a, float b, float x)
{
	x = (x - a) / (b - a);
	x = clamp(x,0.0f,1.0f);
	return x*x * (3.0f - 2.0f*x);
}

struct SSortedHideSpot
{
	SSortedHideSpot(float weight, SHideSpot* pHideSpot) : weight(weight), pHideSpot(pHideSpot) {}
	inline bool operator<(const SSortedHideSpot& rhs) const { return weight > rhs.weight; }	// highest weight first.
	float weight;
	SHideSpot* pHideSpot;
};



template < class Value > void SerializeWeakRefMap( TSerialize &ser, const char* sName, std::map < CWeakRef<CAIObject>, Value > &map )
{
	ser.BeginGroup(sName);
	int size = map.size();
	ser.Value("mapSize", size);

	// If reading, iterate over serialised data and add to container
	if (ser.IsReading())
	{
		map.clear();
		for (int i=0; i<size; i++)
		{
			CWeakRef<CAIObject> k;
			Value v;
			ser.BeginGroup("mapPair");
			k.Serialize(ser,"key");
			ser.Value("value", v);
			ser.EndGroup();
			map.insert( std::make_pair(k,v) );
		}
	}
	// If writing, iterate over container and serialise data
	else if (ser.IsWriting())
	{
		for (typename std::map < CWeakRef<CAIObject>, Value >::iterator it = map.begin(); it != map.end(); it++)
		{
			CWeakRef<CAIObject> k = it->first;
			ser.BeginGroup("mapPair");
			k.Serialize(ser,"key");
			ser.Value("value", it->second);
			ser.EndGroup();
		}
	}

	ser.EndGroup();
}





//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPuppet::CPuppet()
: m_fAccuracyMotionAddition( 0.f )
, m_fAccuracySupressor( 0.f )
, m_fLastUpdateTime( 0.0f )
, m_fCurrentAwarenessLevel( PERCEPTION_SOMETHING_SEEN_VALUE/ 2.f )
, m_bWarningTargetDistance(false)
, m_Alertness(-1)
, m_pPerceptionHandler(NULL)
, m_pRODHandler(NULL)
, m_pFireCmdHandler(0)
, m_pFireCmdGrenade(0)
, m_bGrenadeThrowRequested(false)
, m_eGrenadeThrowRequestType(eRGT_INVALID)
, m_iGrenadeThrowTargetType(0)
, m_allowedStrafeDistanceStart(0)
, m_allowedStrafeDistanceEnd(0)
, m_allowStrafeLookWhileMoving(false)
, m_strafeStartDistance(0.0f)
, m_adaptiveUrgencyMin(0)
, m_adaptiveUrgencyMax(0)
, m_adaptiveUrgencyScaleDownPathLen(0)
, m_adaptiveUrgencyMaxPathLen(0)
, m_fTargetAwareness(0)
, m_friendOnWayCounter(0)
, m_fLastTimeAwareOfPlayer(0)
, m_playerAwarenessType(PA_NONE)
, m_bCloseContact(false)
, m_timeSinceTriggerPressed(0)
, m_friendOnWayElapsedTime(0)
, m_distForWalk(2.0f)
, m_distForRun(5.0f)
, m_distForSprint(10.0f)
, m_bCanBeShot(true)
, m_eMemoryFireType(eMFT_UseCoverFireTime)
, m_territoryShape(0)
, m_allowedToHitTarget(false)
, m_allowedToUseExpensiveAccessory(false)
, m_firingReactionTimePassed(false)
, m_firingReactionTime(0.0f)
, m_targetApproach(0.0f)
, m_targetFlee(0.0f)
, m_targetApproaching(false)
, m_targetFleeing(false)
, m_lastTargetValid(false)
, m_lastTargetPos(0,0,0)
, m_lastTargetSpeed(0)
, m_bDryUpdate(false)
, m_FOVPrimaryCos(0)
, m_FOVSecondaryCos(0)
, m_chaseSpeed(0.0f)
, m_chaseSpeedRate(0.0f)
, m_lastChaseUrgencyDist(-1)
, m_lastChaseUrgencySpeed(-1)
, m_vehicleAvoidingTime(0.0f)
, m_targetLastMissPoint(0,0,0)
, m_targetFocus(0.0f)
, m_targetZone(AIZONE_OUT)
, m_targetPosOnSilhouettePlane(0,0,0)
, m_targetDistanceToSilhouette(FLT_MAX)
, m_targetBiasDirection(0,0,-1)
, m_targetEscapeLastMiss(0.0f)
, m_targetDamageHealthThr(0.0f)
, m_targetDamageHealthThrHistory(0)
, m_targetSeenTime(0)
, m_targetLostTime(0)
, m_targetLastMissPointSelectionTime(0.0f)
, m_burstEffectTime(0.0f)
, m_burstEffectState(0)
, m_targetDazzlingTime(0.0f)
, m_bCoverFireEnabled(false)
, m_lastSteerTime(0.0f)
, m_SeeTargetFrom(ST_HEAD)
, m_delayedStance(STANCE_NULL)
, m_delayedStanceMovementCounter(0)
, m_outOfAmmoTimeOut(0.0f)
, m_lastPuppetVisCheck(0)
, m_weaponSpinupTime(0)
, m_lastAimObstructionResult(true)
, m_updatePriority(AIPUP_LOW)
, m_vForcedNavigation(ZERO)
, m_adjustpath(0)
, m_LastShotsCount(0)
, m_steeringOccupancyBias(0)
, m_steeringAdjustTime(0)
, m_steeringEnabled(false)
, m_alarmedTime(0.0f)
, m_alarmedLevel(0.0f)
, m_fireDisabled(0)
, m_closeRangeStrafing(false)
, m_lastBodyDir(ZERO)
, m_bodyTurningSpeed(0)
, m_damagePartsUpdated(false)
{
	CCCPOINT(CPuppet_CPuppet);

	_fastcast_CPuppet = true;

	// todo: make it safe, not just casting
	m_pFireCmdGrenade = static_cast<CFireCommandGrenade*>(GetAISystem()->CreateFirecommandHandler("grenade", this));
	if(m_pFireCmdGrenade)
		m_pFireCmdGrenade->Reset();

	ResetPostures();
	SetDefaultAimPostures();
	SetDefaultHidePostures();

	AILogComment("CPuppet::CPuppet (%p)", this);

	if (!m_RWIUsers)
	{
		gEnv->pPhysicalWorld->AddEventClient(EventPhysRWIResult::id, RWIResultLOS, 1);
	}

	++m_RWIUsers;
}

CPuppet::~CPuppet()
{
	CCCPOINT(CPuppet_Destructor);

  AILogComment("CPuppet::~CPuppet %s (%p)", GetName(), this);

	if (m_validTargetState.rayID != 0)
		gAIEnv.pRayCaster->Cancel(m_validTargetState.rayID);

	SAFE_DELETE(m_pPerceptionHandler);

	if(m_pFireCmdGrenade) m_pFireCmdGrenade->Release();

	if(m_pFireCmdHandler) m_pFireCmdHandler->Release();

	if (m_pCurrentGoalPipe)
		ResetCurrentPipe( true );

	delete m_targetDamageHealthThrHistory;

	--m_RWIUsers;
	if (!m_RWIUsers)
	{
		gEnv->pPhysicalWorld->RemoveEventClient(EventPhysRWIResult::id, RWIResultLOS, 1);
	}
}

//===================================================================
// SetPerceptionHandler
//===================================================================
bool CPuppet::SetPerceptionHandler()
{
	if (gAIEnv.CVars.TargetTracking != 1)
	{
		return false;
	}

	int iTypeId;
	
	switch (gAIEnv.configuration.eCompatibilityMode)
	{
		case ECCM_NONE:    iTypeId = ePHT_Dummy;   break;
		case ECCM_CRYSIS:  iTypeId = ePHT_Crysis;  break;
		case ECCM_CRYSIS2: iTypeId = ePHT_Crysis2;  break;
		case ECCM_GAME04:  iTypeId = ePHT_Crysis;  break;
		case ECCM_WARFACE: iTypeId = ePHT_Warface; break;
		
		default:           iTypeId = ePHT_Dummy;
	}
	
	return SetPerceptionHandler(iTypeId);
}

//===================================================================
// SetPerceptionHandler
//===================================================================
bool CPuppet::SetPerceptionHandler(int iTypeId)
{
	bool bResult = false;

	if (!m_pPerceptionHandler || m_pPerceptionHandler->GetType() != iTypeId)
	{
		SAFE_DELETE(m_pPerceptionHandler);

		// Create new one using register
		m_pPerceptionHandler = CPerceptionHandlerAutoRegBase::Create(iTypeId, GetWeakRef(this));
		bResult = (m_pPerceptionHandler != NULL);
	}

	CRY_ASSERT(m_pPerceptionHandler);
	return bResult;
}

//===================================================================
// SetRODHandler
//===================================================================
void CPuppet::SetRODHandler(IAIRateOfDeathHandler *pHandler)
{
	CRY_ASSERT(pHandler);

	m_pRODHandler = pHandler;
}

//===================================================================
// ClearRODHandler
//===================================================================
void CPuppet::ClearRODHandler()
{
	m_pRODHandler = NULL;
}

//===================================================================
// GetPerceivedTargetPos
//===================================================================
bool CPuppet::GetPerceivedTargetPos(IAIObject *pTarget, Vec3 &vPos) const
{
	CRY_ASSERT(pTarget);

	bool bResult = false;
	vPos.zero();

	if (pTarget)
	{
		CAIObject *pTargetObj = (CAIObject*)pTarget;
		CWeakRef<CAIObject> refTargetAssociation = pTargetObj->GetAssociation();
		if (refTargetAssociation.IsValid())
			pTargetObj = refTargetAssociation.GetAIObject();

		// Check potential targets
		PotentialTargetMap targetMap;
		if (m_pPerceptionHandler && m_pPerceptionHandler->GetPotentialTargets(targetMap))
		{
			PotentialTargetMap::const_iterator itTarget = targetMap.find(GetWeakRef(pTargetObj));
			if (itTarget != targetMap.end())
			{
				// Found, use perceived if not visual or aggressive
				if (itTarget->second.type == AITARGET_VISUAL && itTarget->second.threat == AITHREAT_AGGRESSIVE)
				{
					IEntity *pTargetEntity = pTargetObj->GetEntity();
					CRY_ASSERT(pTargetEntity);
					vPos = pTargetEntity->GetWorldPos();
				}
				else
				{
					CWeakRef<CAIObject> refDummyRep = itTarget->second.refDummyRepresentation.GetWeakRef();
					if (refDummyRep.IsValid())
						vPos = refDummyRep.GetAIObject()->GetPos();
				}
				bResult = true;
			}
		}
	}

	return bResult;
}

//===================================================================
// ParseParameters
//===================================================================
void CPuppet::ParseParameters(const AIObjectParameters &params)
{
	CCCPOINT(CPuppet_ParseParameters);

	CAIActor::ParseParameters( params );

	 // do custom parse on the parameters
	m_Parameters  = params.m_sParamStruct;

	TransformFOV();

	if (m_Parameters.m_nSpecies>=0)
		GetAISystem()->AddToSpecies(this,m_Parameters.m_nSpecies);

	m_Parameters.m_fAccuracy = clamp(m_Parameters.m_fAccuracy,0.0f,1.0f);
	
	// Use default initially
	SetPerceptionHandler();

	GetAISystem()->NotifyEnableState(this, m_bEnabled);
}

//===================================================================
// SetWeaponDescriptor
//===================================================================
void CPuppet::SetWeaponDescriptor(const AIWeaponDescriptor& descriptor)
{
	m_CurrentWeaponDescriptor = descriptor;
//	if(m_CurrentWeaponDescriptor.bSignalOnShoot)
	GetProxy()->EnableWeaponListener(m_CurrentWeaponDescriptor.bSignalOnShoot);

	// Make sure the fire command handler is up to date.

	// If the current fire command handler is already correct, do not recreate it.
	if(m_pFireCmdHandler && stricmp(descriptor.firecmdHandler.c_str(), m_pFireCmdHandler->GetName()) == 0)
	{
		m_pFireCmdHandler->Reset();
		return;
	}

	// Release the old handler and create new.
	SAFE_RELEASE(m_pFireCmdHandler);
	m_pFireCmdHandler = GetAISystem()->CreateFirecommandHandler(descriptor.firecmdHandler, this);
	if(m_pFireCmdHandler)
		m_pFireCmdHandler->Reset();
}

//===================================================================
// AdjustTargetVisibleRange
//===================================================================
float CPuppet::AdjustTargetVisibleRange(const CAIActor& observer, float fVisibleRange) const
{
	float fRangeScale = 1.0f;

	// Adjust using my light level if the observer is affected by light
	if (observer.GetParameters().m_PerceptionParams.isAffectedByLight)
	{
		const EAILightLevel targetLightLevel = GetLightLevel();
		switch (targetLightLevel)
		{
			//	case AILL_LIGHT: SOMSpeed
			case AILL_MEDIUM: fRangeScale *= gAIEnv.CVars.SightRangeMediumIllumMod; break;
			case AILL_DARK:	fRangeScale *= gAIEnv.CVars.SightRangeDarkIllumMod; break;
		}
	}

	// Scale down sight range when target is underwater based on distance
	const float fCachedWaterOcclusionValue = GetCachedWaterOcclusionValue();
	if (fCachedWaterOcclusionValue > FLT_EPSILON)
	{
		const Vec3& observerPos = observer.GetPos();
		const float fDistance = Distance::Point_Point(GetPos(), observerPos);
		const float fDistanceFactor = (fVisibleRange > FLT_EPSILON ? GetAISystem()->GetVisPerceptionDistScale(fDistance/fVisibleRange) : 0.0f);

		const float fWaterOcclusionEffect = 2.0f*fCachedWaterOcclusionValue + (1-fDistanceFactor)*0.5f;

		fRangeScale *= (fWaterOcclusionEffect > 1.0f ? 0.0f : 1.0f-fWaterOcclusionEffect);
	}

	// Return new range
	return fVisibleRange * fRangeScale;
}

//===================================================================
// Update
//===================================================================
void CPuppet::Update(EObjectUpdate type)
{
	CCCPOINT(CPuppet_Update);
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);
	// 
	if (!IsEnabled())
	{
		AIWarning("CPuppet::Update trying to update disabled  %s", GetName());
		AIAssert(0);
		return;
	}
	// There should never be puppets without proxies.
	if (!GetProxy())
	{
		AIWarning("CPuppet::Update Puppet %s does not have proxy!", GetName());
		AIAssert(0);
		return;
	}
	// There should never be puppets without physics.
	if (!GetPhysics())
	{
		AIWarning("CPuppet::Update Puppet %s does not have physics!", GetName());
		AIAssert(0);
		return;
	}
	// dead puppets should never be updated
	if (m_bEnabled && GetProxy()->GetActorHealth() < 1)
	{
		AIWarning("CPuppet::Update Trying to update dead character >> %s ", GetName());
//		AIAssert(0);
		return;
	}

	CPipeUser::Update(type);

	if (!stricmp(gAIEnv.CVars.DrawPerceptionHandlerModifiers,GetName()))
		DebugDrawPerceptionHandlerModifiers();

	const float dt = GetAISystem()->GetFrameDeltaTime();

	// Update body angle and body turn speed
	float turnAngle = Ang3::CreateRadZ(m_lastBodyDir, GetBodyDir());
	if (dt > 0.0f)
		m_bodyTurningSpeed = turnAngle / dt;
	else
		m_bodyTurningSpeed = 0;
	m_lastBodyDir = GetBodyDir();


	m_bDryUpdate = (type == AIUPDATE_DRY);


	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();

	if (!m_bDryUpdate)
	{
		if (m_CurrentHideObject.IsValid())
		{
			m_CurrentHideObject.Update(this);
			if (pAttentionTarget && m_CurrentHideObject.IsCompromised(this, pAttentionTarget->GetPos()))
				SetCoverCompromised();
		}

		// Update some special objects if they have been recently used.
		CAIObject* pSpecial = 0;

		Vec3 probTarget(0,0,0);

		pSpecial = m_refSpecialObjects[AISPECIAL_PROBTARGET_IN_TERRITORY].GetAIObject();
		if (pSpecial && pSpecial->m_bTouched)
		{
			pSpecial->m_bTouched = false;
			if (probTarget.IsZero())
				probTarget = GetProbableTargetPosition();
			Vec3 pos = probTarget;
			// Clamp the point to the territory shape.
			if (GetTerritoryShape())
				GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

			pSpecial->SetPos(pos);
		}

		pSpecial = m_refSpecialObjects[AISPECIAL_PROBTARGET_IN_REFSHAPE].GetAIObject();
		if (pSpecial && pSpecial->m_bTouched)
		{
			pSpecial->m_bTouched = false;
			if (probTarget.IsZero())
				probTarget = GetProbableTargetPosition();
			Vec3 pos = probTarget;
			// Clamp the point to ref shape
			if (GetRefShape())
				GetRefShape()->ConstrainPointInsideShape(pos, true);

			pSpecial->SetPos(pos);
		}

		pSpecial = m_refSpecialObjects[AISPECIAL_PROBTARGET_IN_TERRITORY_AND_REFSHAPE].GetAIObject();
		if (pSpecial && pSpecial->m_bTouched)
		{
			pSpecial->m_bTouched = false;
			if (probTarget.IsZero())
				probTarget = GetProbableTargetPosition();
			Vec3 pos = probTarget;
			// Clamp the point to ref shape
			if (GetRefShape())
				GetRefShape()->ConstrainPointInsideShape(pos, true);
			// Clamp the point to the territory shape.
			if (GetTerritoryShape())
				GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

			pSpecial->SetPos(pos);
		}

		pSpecial = m_refSpecialObjects[AISPECIAL_ATTTARGET_IN_REFSHAPE].GetAIObject();

		if (pSpecial && pSpecial->m_bTouched)
		{
			pSpecial->m_bTouched = false;
			if (pAttentionTarget)
			{
				Vec3	pos = pAttentionTarget->GetPos();
				// Clamp the point to ref shape
				if (GetRefShape())
					GetRefShape()->ConstrainPointInsideShape(pos, true);
				// Update pos
				pSpecial->SetPos(pos);
			}
			else
			{
				pSpecial->SetPos(GetPos());
			}
		}

		pSpecial = m_refSpecialObjects[AISPECIAL_ATTTARGET_IN_TERRITORY].GetAIObject();
		if (pSpecial && pSpecial->m_bTouched)
		{
			pSpecial->m_bTouched = false;
			if (pAttentionTarget)
			{
				Vec3	pos = pAttentionTarget->GetPos();
				// Clamp the point to ref shape
				if (GetTerritoryShape())
					GetTerritoryShape()->ConstrainPointInsideShape(pos, true);
				// Update pos
				pSpecial->SetPos(pos);
			}
			else
			{
				pSpecial->SetPos(GetPos());
			}
		}

		pSpecial = m_refSpecialObjects[AISPECIAL_ATTTARGET_IN_TERRITORY_AND_REFSHAPE].GetAIObject();
		if (pSpecial && pSpecial->m_bTouched)
		{
			pSpecial->m_bTouched = false;
			if (pAttentionTarget)
			{
				Vec3	pos = pAttentionTarget->GetPos();
				// Clamp the point to ref shape
				if (GetRefShape())
					GetRefShape()->ConstrainPointInsideShape(pos, true);
				// Clamp the point to the territory shape.
				if (GetTerritoryShape())
					GetTerritoryShape()->ConstrainPointInsideShape(pos, true);

				pSpecial->SetPos(pos);
			}
			else
			{
				pSpecial->SetPos(GetPos());
			}
		}

	}

	// make sure to update direction when entity is not moved
	SAIBodyInfo bodyInfo;
	GetProxy()->QueryBodyInfo( bodyInfo);
	SetPos( bodyInfo.vEyePos );
  SetBodyDir( bodyInfo.vBodyDir );
  SetMoveDir( bodyInfo.vMoveDir );
	SetViewDir( bodyInfo.vEyeDir );

	UpdateHealthTracking();
	m_damagePartsUpdated = false;

	UpdateVision();

	// Handle navigational smart object start and end states before executing goalpipes.
	if ( m_eNavSOMethod == nSOmSignalAnimation || m_eNavSOMethod == nSOmActionAnimation )
	{
		if (m_currentNavSOStates.IsEmpty())
		{
			if(m_State.curActorTargetPhase == eATP_Started || m_State.curActorTargetPhase == eATP_StartedAndFinished)
			{
				// Copy the set state to the currently used state.
				m_currentNavSOStates = m_pendingNavSOStates;
				m_pendingNavSOStates.Clear();

				// keep track of last used smart object
				m_idLastUsedSmartObject = m_currentNavSOStates.objectEntId;

				// keep track of last used smart object
				m_idLastUsedSmartObject = m_currentNavSOStates.objectEntId;
			}
		}

		if(m_State.curActorTargetPhase == eATP_StartedAndFinished || m_State.curActorTargetPhase == eATP_Finished)
		{
			// modify smart object states
			IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity( m_currentNavSOStates.objectEntId );
			gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), m_currentNavSOStates.sAnimationDoneUserStates );
			if ( pObjectEntity )
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObjectEntity, m_currentNavSOStates.sAnimationDoneObjectStates );

			m_currentNavSOStates.Clear();
			m_forcePathGenerationFrom.zero();
			m_eNavSOMethod = nSOmNone;
			m_navSOEarlyPathRegen = false;
		}
	}

  static bool doDryUpdateCall = true;

	if (!m_bDryUpdate)
	{	
		CTimeValue fCurrentTime = GetAISystem()->GetFrameStartTime();
		if (m_fLastUpdateTime.GetSeconds()>0)
			m_fTimePassed = min(0.5f, (fCurrentTime - m_fLastUpdateTime).GetSeconds());
		else
			m_fTimePassed = 0;
		m_fLastUpdateTime = fCurrentTime;

		m_State.Reset();

		// affect puppet parameters here----------------------------------------
		if (m_bCanReceiveSignals)
		{
			UpdatePuppetInternalState();

			// Marcio: Update the attention target, since most likely the AIOBJECTSTATE evaluation
			// will only happen this frame, and then we'll miss it
			pAttentionTarget = m_refAttentionTarget.GetAIObject();
		}

		GetStateFromActiveGoals(m_State);

		// Store current position to debug stream.
		{
			RecorderEventData recorderEventData(GetPos());
			RecordEvent(IAIRecordable::E_AGENTPOS, &recorderEventData);
		}

		// Store current direction to debug stream.
		{
			RecorderEventData recorderEventData(GetViewDir());
			RecordEvent(IAIRecordable::E_AGENTDIR, &recorderEventData);
		}

		// Store current attention target position to debug stream.
		if(pAttentionTarget)
		{
			RecorderEventData recorderEventData(pAttentionTarget->GetPos());
			RecordEvent(IAIRecordable::E_ATTENTIONTARGETPOS, &recorderEventData);
		}

		m_friendOnWayCounter -= GetAISystem()->GetUpdateInterval();

		// Update last know target position, and last target position constrained into the territory.
		if(pAttentionTarget && (pAttentionTarget->IsAgent() || (pAttentionTarget->GetType() == AIOBJECT_TARGET)))
		{
			m_lastLiveTargetPos = pAttentionTarget->GetPos();
			m_timeSinceLastLiveTarget = 0.0f;
		}
		else
		{
			if (m_timeSinceLastLiveTarget >= 0.0f)
				m_timeSinceLastLiveTarget += m_fTimePassed;
		}

		m_lightLevel = GetAISystem()->GetLightManager()->GetLightLevelAt(GetPos(), this, &m_usingCombatLight);
	}
  else if (doDryUpdateCall)
  {
    //if approaching then always update
    for (size_t i = 0; i < m_vActiveGoals.size(); i++)
    {
      QGoal& Goal = m_vActiveGoals[i];
      Goal.pGoalOp->ExecuteDry(this);
    }
  }

	if (m_delayedStance != STANCE_NULL)
	{
		bool wantsToMove = !m_State.vMoveDir.IsZero() && m_State.fDesiredSpeed > 0.0f;
		if (wantsToMove)
		{
			m_delayedStanceMovementCounter++;
			if (m_delayedStanceMovementCounter > 3)
			{
				m_State.bodystate = m_delayedStance;
				m_delayedStance = STANCE_NULL;
				m_delayedStanceMovementCounter = 0;
			}
		}
		else
		{
			m_delayedStanceMovementCounter = 0;
		}
	}

	// Handle navigational smart object failure case. The start and finish states are handled
	// before executing the goalpipes, but since trace goalop can set the error too, the error is handled
	// here right after the goalops are executed. If this was done later, the state syncing code below would
	// clear the flag.
	if ( m_eNavSOMethod == nSOmSignalAnimation || m_eNavSOMethod == nSOmActionAnimation )
	{
		if ( m_State.curActorTargetPhase == eATP_Error )
		{
			// Invalidate the current link
			PathPointDescriptor::SmartObjectNavDataPtr pSmartObjectNavData = m_Path.GetLastPathPointAnimNavSOData();

			if ( pSmartObjectNavData )
			{
				const SSmartObjectNavData* pNavData = gAIEnv.pGraph->GetNodeManager().GetNode(pSmartObjectNavData->fromIndex)->GetSmartObjectNavData();
				InvalidateSOLink( pNavData->pSmartObject, pNavData->pHelper, gAIEnv.pGraph->GetNodeManager().GetNode(pSmartObjectNavData->toIndex)->GetSmartObjectNavData()->pHelper );
			}

			// modify smart object states
			if ( !m_currentNavSOStates.IsEmpty() )
			{
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), m_currentNavSOStates.sAnimationFailUserStates );
				IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity( m_currentNavSOStates.objectEntId );
				if ( pObjectEntity )
					gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObjectEntity, m_currentNavSOStates.sAnimationFailObjectStates );
				m_currentNavSOStates.Clear();
			}

			if ( !m_pendingNavSOStates.IsEmpty() )
			{
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), m_pendingNavSOStates.sAnimationFailUserStates );
				IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity( m_pendingNavSOStates.objectEntId );
				if ( pObjectEntity )
					gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObjectEntity, m_pendingNavSOStates.sAnimationFailObjectStates );
				m_pendingNavSOStates.Clear();
			}

			m_eNavSOMethod = nSOmNone;
			m_navSOEarlyPathRegen = false;
			m_forcePathGenerationFrom.zero();
		}
	}

	//
	// Sync the actor target phase with the AIProxy
	// The syncing should only happen here. The Trace goalop is allowed to
	// make a request and set error state if necessary.
	//

	bool	animationStarted = false;

	if(m_State.curActorTargetPhase == eATP_Waiting || m_State.curActorTargetPhase == eATP_Starting)
	{
	}
	else if(m_State.curActorTargetPhase == eATP_Started)
	{
//		m_State.actorTargetPhase = eATP_Playing;
		animationStarted = true;
	}
	else if(m_State.curActorTargetPhase == eATP_StartedAndFinished)
	{
		animationStarted = true;
//		m_State.actorTargetPhase = eATP_None;
		m_State.actorTargetReq.Reset();
	}
	else if(m_State.curActorTargetPhase == eATP_Finished)
	{
		m_State.curActorTargetPhase = eATP_None;
		m_State.actorTargetReq.Reset();
	}
	else if(m_State.curActorTargetPhase == eATP_Error)
	{
		// m_State.actorTargetReq shouldn't get reset here but only when path
		// is cleared after COPTrace has processed the error, otherwise
		// COPTrace may finish with AIGOR_SUCCEED instead of AIGOR_FAILED.
	}

	// Inform the listeners of current goal pipe that an animation has been started.
	if(animationStarted && m_eNavSOMethod == nSOmNone)
	{
		CGoalPipe* pCurrent = m_pCurrentGoalPipe;
		if ( pCurrent )
		{
			while ( pCurrent->GetSubpipe() )
				pCurrent = pCurrent->GetSubpipe();
			NotifyListeners( pCurrent, ePN_AnimStarted );
		}
	}


	//--------------------------------------------------------
	// Update the look at and strafe logic.
	if ( GetSubType() == CAIObject::STP_2D_FLY )
		UpdateLookTarget3D(pAttentionTarget); //this is only for the scout now.
	else
		UpdateLookTarget(pAttentionTarget);

	//--------------------------------------------------------
	// Update the body target
	m_State.vBodyTargetDir = m_vBodyTargetDir;


	if (pAttentionTarget)
	{
		m_State.nTargetType = pAttentionTarget->GetType();
		m_State.bTargetEnabled = pAttentionTarget->IsEnabled();
	}
	else
	{
		m_State.nTargetType = -1;
		m_State.bTargetEnabled = false;
		m_State.eTargetThreat = AITHREAT_NONE;
		m_State.eTargetType = AITARGET_NONE;
	}

	m_targetDazzlingTime = max(0.0f, m_targetDazzlingTime - dt);
	if (GetAttentionTargetType() == AITARGET_VISUAL && GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
	{
		m_targetLostTime = 0.0f;
		m_targetSeenTime = min(m_targetSeenTime + dt, 10.0f);
	}
	else
	{
		m_targetLostTime += dt;
		m_targetSeenTime = max(0.0f, m_targetSeenTime - dt);
	}

	FireCommand(gEnv->pTimer->GetFrameTime());

	if (gAIEnv.CVars.UpdateProxy)
	{	
		// Force stance etc
//		int lastStance = m_State.bodystate;
		bool forcedPosture = false;
		if (strcmp(gAIEnv.CVars.ForcePosture, "0"))
		{
			IPuppet::SPostureInfo posture;

			if (GetPostureByName(gAIEnv.CVars.ForcePosture, &posture))
			{
				forcedPosture = true;

				m_State.bodystate = posture.stance;
				m_State.lean = posture.lean;
				m_State.peekOver = posture.peekOver;

				if (!posture.agInput.empty())
					GetProxy()->SetAGInput(AIAG_ACTION, posture.agInput.c_str());
				else
					GetProxy()->ResetAGInput(AIAG_ACTION);
			}
		}

		if (!forcedPosture)
		{
			if (gAIEnv.CVars.ForceStance > -1)
				m_State.bodystate = gAIEnv.CVars.ForceStance;

			if (strcmp(gAIEnv.CVars.ForceAGAction, "0"))
				GetProxy()->SetAGInput(AIAG_ACTION, gAIEnv.CVars.ForceAGAction);
		}

		if (strcmp(gAIEnv.CVars.ForceAGSignal, "0"))
			GetProxy()->SetAGInput(AIAG_SIGNAL, gAIEnv.CVars.ForceAGSignal);

    if (gAIEnv.CVars.ForceAllowStrafing > -1)
      m_State.allowStrafing = gAIEnv.CVars.ForceAllowStrafing != 0;

    const char * forceLookAimTarget = gAIEnv.CVars.ForceLookAimTarget;
    if (strcmp(forceLookAimTarget, "none") != 0)
    {
      Vec3 targetPos = GetPos();
      if (strcmp(forceLookAimTarget, "x") == 0)
        targetPos += Vec3(10, 0, 0);
      else if (strcmp(forceLookAimTarget, "y") == 0)
        targetPos += Vec3(0, 10, 0);
      else if (strcmp(forceLookAimTarget, "xz") == 0)
        targetPos += Vec3(10, 0, 10);
      else if (strcmp(forceLookAimTarget, "yz") == 0)
        targetPos += Vec3(0, 10, 10);
      else
      {
        IEntity *pEntity = gEnv->pEntitySystem->FindEntityByName(forceLookAimTarget);
        if (pEntity)
          targetPos = pEntity->GetPos();
        else
          targetPos.zero();
      }
      m_State.vLookTargetPos = targetPos;
      m_State.vAimTargetPos = targetPos;
			m_State.aimTargetIsValid = true;
    }

		if (!m_bDryUpdate)
		{
			// If secondary fire went through, clear throw grenade request
			if (m_State.fireSecondary && m_bGrenadeThrowRequested)
				m_bGrenadeThrowRequested = false;
		}

		// Always update the AI proxy, also during dry updates. The Animation system
		// needs correct and constantly updated predictions to correctly set animation
		// parameters.
		// (MATT) Try avoiding UpdateMind, which triggers script, signal and behaviour code, if only a dry update {2009/12/06}
		GetProxy()->Update(&m_State, !m_bDryUpdate);

		// Restore foced state.
		//m_State.bodystate = lastStance;

		if (IsEnabled()) // the puppet may have been killed above in this loop (say, in GetProxy()->Update(&m_State))
			UpdateAlertness();
	}
	if (m_Parameters.m_fAwarenessOfPlayer >0)
		CheckAwarenessPlayer();

	// update close contact info
	if(m_bCloseContact && (GetAISystem()->GetFrameStartTime() - m_CloseContactTime).GetSeconds()>1.5f)
		m_bCloseContact = false;

	// Time out unreachable hidepoints.
	TimeOutVec3List::iterator end(m_recentUnreachableHideObjects.end());
	for(TimeOutVec3List::iterator it = m_recentUnreachableHideObjects.begin(); it != end;)
	{
		it->first -= GetAISystem()->GetFrameDeltaTime();
		if(it->first < 0.0f)
			it = m_recentUnreachableHideObjects.erase(it);
		else
			++it;
	}

	UpdateCloakScale();


	// Make sure we haven't played with that value during our update
	assert(m_bDryUpdate == (type == AIUPDATE_DRY));

	if(!m_bDryUpdate)
	{
		// Health
		RecorderEventData recorderEventData((float)GetProxy()->GetActorHealth());
		RecordEvent(IAIRecordable::E_HEALTH, &recorderEventData);
	}
}

//===================================================================
// UpdateTargetMovementState
//===================================================================
void CPuppet::UpdateTargetMovementState()
{
	const float dt = m_fTimePassed;
	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();

	if(!pAttentionTarget || dt < 0.00001f)
	{
		m_lastTargetValid = false;
		m_targetApproaching = false;
		m_targetFleeing = false;
		m_targetApproach = 0;
		m_targetFlee = 0;
		return;
	}

	bool	targetValid = false;
	if (pAttentionTarget->GetType() == AIOBJECT_PLAYER || 
			pAttentionTarget->GetType() == AIOBJECT_CPUPPET ||
			pAttentionTarget->GetType() == AIOBJECT_TARGET)
	{
		targetValid = true;
	}

	Vec3	targetPos = pAttentionTarget->GetPos();
	Vec3	targetDir = pAttentionTarget->GetMoveDir();
	float	targetSpeed = pAttentionTarget->GetVelocity().GetLength();
	const Vec3&	puppetPos = GetPos();

	if(!m_lastTargetValid)
	{
		m_lastTargetValid = true;
		m_lastTargetSpeed = targetSpeed;
		m_lastTargetPos = targetPos;
	}
	const float	fleeMin = 10.0f;
	const float	approachMax = 20.0f;

	if(!targetValid && m_lastTargetValid)
	{
		targetPos = m_lastTargetPos;
		targetSpeed = m_lastTargetSpeed;
	}

	{
		float	curDist = Distance::Point_Point(targetPos, puppetPos);
		float	lastDist = Distance::Point_Point(m_lastTargetPos, puppetPos);

		Vec3	dirTargetToPuppet = puppetPos - targetPos;
		dirTargetToPuppet.NormalizeSafe();

		float	dot = (1.0f + dirTargetToPuppet.Dot(targetDir)) * 0.5f;

		bool movingTowards = curDist < lastDist && targetSpeed > 0; //fabsf(curDist - lastDist) > 0.01f;
		bool movingAway = curDist > lastDist && targetSpeed > 0; //fabsf(curDist - lastDist) > 0.01f;

		if(curDist < approachMax && movingTowards)
			m_targetApproach += targetSpeed * sqr(dot) * 0.25f;
		else
			m_targetApproach -= dt * 2.0f;

		if(curDist > fleeMin && movingAway)
			m_targetFlee += targetSpeed * sqr(1.0f - dot) * 0.1f;
		else
			m_targetFlee -= dt * 2.0f;

		m_lastTargetPos = targetPos;
	}


	m_targetApproach = clamp(m_targetApproach, 0.0f, 10.0f);
	m_targetFlee = clamp(m_targetFlee, 0.0f, 10.0f);

	bool	approaching = m_targetApproach > 9.9f;
	bool	fleeing = m_targetFlee > 9.9f;

	if(approaching != m_targetApproaching)
	{
		m_targetApproaching = approaching;
		if(m_targetApproaching)
		{
			m_targetApproach = 0;
			SetSignal(1, "OnTargetApproaching", pAttentionTarget->GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnTargetApproaching);
		}
	}

	if(fleeing != m_targetFleeing)
	{
		m_targetFleeing = fleeing;
		if(m_targetFleeing)
		{
			m_targetFlee = 0;
			SetSignal(1, "OnTargetFleeing", pAttentionTarget->GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnTargetFleeing);
		}
	}
}

//===================================================================
// UpdateAlertness
//===================================================================
void CPuppet::UpdateAlertness()
{
	CRY_ASSERT(IsEnabled());

	int nextAlertness = GetProxy()->GetAlertnessState();
	if (m_Alertness != nextAlertness && m_Parameters.m_nSpecies && m_Parameters.m_bSpeciesHostility)
	{
		if (m_Alertness >= 0 && GetAISystem()->m_AlertnessCounters[m_Alertness] > 0)
			--GetAISystem()->m_AlertnessCounters[m_Alertness];
		CRY_ASSERT(NUM_ALERTNESS_COUNTERS > nextAlertness && nextAlertness >= 0);
		if(NUM_ALERTNESS_COUNTERS > nextAlertness && nextAlertness >= 0)
		++GetAISystem()->m_AlertnessCounters[nextAlertness];
	}
	m_Alertness = nextAlertness;
}

//===================================================================
// ResetAlertness
//===================================================================
void CPuppet::ResetAlertness()
{
	// get alertness before doing GetProxy()->Reset()!!!
	if (m_Alertness >= 0 && m_Parameters.m_nSpecies && m_Parameters.m_bSpeciesHostility &&
		GetAISystem()->m_AlertnessCounters[m_Alertness] > 0)
		--GetAISystem()->m_AlertnessCounters[m_Alertness];
	m_Alertness = -1;
}

//===================================================================
// GetEventOwner
//===================================================================
IAIObject* CPuppet::GetEventOwner(IAIObject *pObject) const
{
	IAIObject *pOwner = 0;
	
	if (m_pPerceptionHandler)
	{
		CWeakRef<CAIObject> refObject = GetWeakRefSafe((CAIObject*)pObject);
		pOwner = m_pPerceptionHandler->GetEventOwner(refObject);
	}

	return pOwner;
}

//===================================================================
// GetEventOwner
//===================================================================
CAIObject* CPuppet::GetEventOwner(CWeakRef<CAIObject> refOwned) const
{
	if (!refOwned.IsValid()) return 0;
	// TO DO: for memory/sound target etc, probably it's better to set their association equal to the owner
	// instead of searching for it here
	return (m_pPerceptionHandler ? m_pPerceptionHandler->GetEventOwner(refOwned) : NULL);
}

//===================================================================
// UpdateTargetSelection
//===================================================================
bool CPuppet::UpdateTargetSelection(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased)
{
	bool bResult = false;

	switch (gAIEnv.CVars.TargetTracking)
	{
		case 2: // Target tracks
			if (GetTargetTrackBestTarget(refBestTarget, pTargetInfo, bCurrentTargetErased))
			{
				if (pTargetInfo && pTargetInfo->type == AITARGET_VISUAL && pTargetInfo->threat >= AITHREAT_AGGRESSIVE)
					SetAlarmed();

				bResult = true;
			}
			break;

		case 1: // Perception handlers
			if (m_pPerceptionHandler)
			{
				bResult = m_pPerceptionHandler->GetBestTarget(refBestTarget, pTargetInfo, bCurrentTargetErased);
			}
			break;

		case 0: // Disabled
		default:
			bResult = false;
			break;
	}

	return bResult;
}

//===================================================================
// GetTargetTrackBestTarget
//===================================================================
bool CPuppet::GetTargetTrackBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased) const
{
	bool bResult = false;

	refBestTarget.Reset();
	pTargetInfo = NULL;
	bCurrentTargetErased = false;

	tAIObjectID objectId = GetAIObjectID();
	CRY_ASSERT(objectId > 0);

	tAIObjectID targetId = 0;
	gAIEnv.pTargetTrackManager->Update(objectId);
	if (gAIEnv.pTargetTrackManager->GetDesiredTarget(objectId, TargetTrackHelpers::eDTM_Select_Highest, targetId, pTargetInfo))
	{
		refBestTarget = gAIEnv.pObjectContainer->GetWeakRef(targetId);
		bResult = true;
	}
	else
	{
		bCurrentTargetErased = true;
	}

	return bResult;
}

//===================================================================
// UpdatePuppetInternalState
//===================================================================
void CPuppet::UpdatePuppetInternalState()
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);
	CCCPOINT(CPuppet_UpdatePuppetInternalState);

	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();

	assert(!m_bDryUpdate);
	// Update alarmed state
	m_alarmedTime -= m_fTimePassed;
	if (m_alarmedTime < 0.0f)
		m_alarmedTime = 0.0f;

	// Sight range threshold.
	// The sight attenuation range envelope changes based on the alarmed state.
	// The threshold is changed smoothly.
	const float alarmLevelChangeTime = 3.0f;
	const float alarmLevelGoal = IsAlarmed() ? 1.0f : 0.0f;
	m_alarmedLevel += (alarmLevelGoal - m_alarmedLevel) * (m_fTimePassed / alarmLevelChangeTime);
	Limit(m_alarmedLevel, 0.0f, 1.0f);

	CWeakRef<CAIObject> refBestTarget;
	SAIPotentialTarget* bestTargetEvent = 0;
	bool currentTargetErased = false;
	
	if (UpdateTargetSelection(refBestTarget, bestTargetEvent, currentTargetErased))
	{
		m_State.eTargetStuntReaction = AITSR_NONE;
		m_State.eTargetType = bestTargetEvent->type;
		m_State.eTargetThreat = bestTargetEvent->threat;

		CAIObject *bestTarget = refBestTarget.GetAIObject();
		if (bestTarget != pAttentionTarget)
		{
			// New attention target
			SetAttentionTarget( GetWeakRef(bestTarget));
			m_AttTargetPersistenceTimeout = m_Parameters.m_PerceptionParams.targetPersistence;
			bestTargetEvent->bNeedsUpdating = false;

			// When seeing a visible target, check for stunt reaction.
			if (bestTargetEvent->type == AITARGET_VISUAL && bestTargetEvent->threat == AITHREAT_AGGRESSIVE)
			{
				if (CAIPlayer* pPlayer = bestTarget->CastToCAIPlayer())
				{
					if (IsHostile(pPlayer))
					{
						bool isStunt = pPlayer->IsDoingStuntActionRelatedTo(GetPos(), m_Parameters.m_PerceptionParams.sightRange/5.0f);
						if (m_targetLostTime > m_Parameters.m_PerceptionParams.stuntReactionTimeOut && isStunt)
							m_State.eTargetStuntReaction = AITSR_SEE_STUNT_ACTION;
						else if (pPlayer->IsCloakEffective())
							m_State.eTargetStuntReaction = AITSR_SEE_CLOAKED;
					}
				}
			}
			
			EntityId bestTargetId = bestTarget ? bestTarget->GetEntityID() : 0;
			if (bestTarget && !bestTargetId)
			{
				CWeakRef<CAIObject> refAssociation = bestTarget->GetAssociation();
				if (refAssociation.IsValid())
					bestTargetId = refAssociation.GetAIObject()->GetEntityID();
			}

			bool bSameTarget = false;
			if (pAttentionTarget)
			{
				CWeakRef<CAIObject> refRealTarget = GetLiveTarget(GetWeakRef(pAttentionTarget));
				if (refRealTarget.IsValid())
					bSameTarget = (refRealTarget.GetObjectID() == refBestTarget.GetObjectID());
			}

			// Inform AI of change
			IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
			CRY_ASSERT(pData);
			pData->nID = bestTargetId;
			pData->fValue = (bSameTarget ? 1.0f : 0.0f);
			pData->iValue = bestTargetEvent->type;
			pData->iValue2 = bestTargetEvent->threat;
			SetSignal(0, "OnNewAttentionTarget", GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnNewAttentionTarget);
		}
		else if (pAttentionTarget)
		{
			if(m_AttTargetThreat != bestTargetEvent->threat)
			{
				IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
				pData->iValue = bestTargetEvent->threat;
				SetSignal(0, "OnAttentionTargetThreatChanged", GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnAttentionTargetThreatChanged);
			}

			// Handle state change of the current attention target.

			// The state lowering.
			if (m_AttTargetThreat >= AITHREAT_AGGRESSIVE && bestTargetEvent->threat < AITHREAT_AGGRESSIVE)
			{
				// Aggressive -> threatening
				if (bestTargetEvent->type == AITARGET_VISUAL || bestTargetEvent->type == AITARGET_MEMORY)
					SetSignal(0, "OnNoTargetVisible", GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnNoTargetVisible);
			}
			else if (m_AttTargetThreat >= AITHREAT_THREATENING && bestTargetEvent->threat < AITHREAT_THREATENING)
			{
				// Threatening -> interesting
				SetSignal(0, "OnNoTargetAwareness", GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnNoTargetAwareness);
			}

			// The state rising.
			// Use the exposure threat to trigger the signals. This will result multiple
			// same signals (like on threatening sound) to be sent if the exposure is
			// crossing the threshold. This should allow more responsive behaviors, but
			// at the same time prevent too many signals.
			// The signal is resent only if the exposure is crossing the current or higher threshold.
			if (bestTargetEvent->bNeedsUpdating || bestTargetEvent->exposureThreat > bestTargetEvent->threat || m_AttTargetType != bestTargetEvent->type)
			{
				bestTargetEvent->bNeedsUpdating = false;

				if (m_AttTargetExposureThreat <= AITHREAT_AGGRESSIVE && bestTargetEvent->exposureThreat >= AITHREAT_AGGRESSIVE)
				{
					// Threatening -> aggressive
					if (bestTargetEvent->type == AITARGET_VISUAL)
					{
						if (CAIPlayer* pPlayer = bestTarget->CastToCAIPlayer())
						{
							if (IsHostile(pPlayer))
							{
								bool isStunt = pPlayer->IsDoingStuntActionRelatedTo(GetPos(), m_Parameters.m_PerceptionParams.sightRange/5.0f);
								if (m_targetLostTime > m_Parameters.m_PerceptionParams.stuntReactionTimeOut && isStunt)
									m_State.eTargetStuntReaction = AITSR_SEE_STUNT_ACTION;
								else if (pPlayer->IsCloakEffective())
									m_State.eTargetStuntReaction = AITSR_SEE_CLOAKED;
							}
						}
					}
				}
			}
		}
		// Keep track of the current state, used to track the state transitions.
		m_AttTargetType = bestTargetEvent->type;
		m_AttTargetThreat = bestTargetEvent->threat;
		m_AttTargetExposureThreat = bestTargetEvent->exposureThreat;
	}
	else
	{
		// No attention target, reset if the current target is erased.
		// The check for current target erased allows to forcefully set the
		// attention target.
		if (pAttentionTarget)
		{
			if (currentTargetErased)
				SetAttentionTarget(NILREF);
		}

		m_AttTargetType = AITARGET_NONE;
		m_AttTargetThreat = AITHREAT_NONE;
		m_AttTargetExposureThreat = AITHREAT_NONE;

		m_State.eTargetType = AITARGET_NONE;
		m_State.eTargetThreat = AITHREAT_NONE;
	}

	// update devaluated points
	DevaluedMap::iterator di, next;
	for (di = m_mapDevaluedPoints.begin(); di != m_mapDevaluedPoints.end(); di = next)
	{
		next = di; 
		++next;
		di->second -= m_fTimePassed;
		if (di->second < 0 || !di->first.IsValid())
		{
			CCCPOINT(CPuppet_UpdatePuppetInternalState);

			m_mapDevaluedPoints.erase(di);
	}
	}

	UpdateTargetMovementState();


	// Update attention-target related info:
	// distance from target, in/out of territory status
	if (pAttentionTarget)
	{
		const Vec3 &vAttTargetPos = pAttentionTarget->GetPos();

		m_State.fDistanceFromTarget = Distance::Point_Point(vAttTargetPos, GetPos());
		
		if (m_territoryShape)
		{
			m_attTargetOutOfTerritory.bState = !m_territoryShape->IsPointInsideShape( vAttTargetPos, false );			
		}
		else
		{
			m_attTargetOutOfTerritory.bState = false;
		}
	}
	else
	{
		m_attTargetOutOfTerritory.bState = false;
	}

	if (m_attTargetOutOfTerritory.CheckUpdate())
	{
		bool bState = m_attTargetOutOfTerritory.bState;
		const char *sSignal = ( bState ? "OnTargetOutOfTerritory" : "OnTargetInTerritory" );
		SetSignal(AISIGNAL_DEFAULT, sSignal, NULL, NULL, 0);
	}
}


//===================================================================
// Event
//===================================================================
void CPuppet::Event(unsigned short eType, SAIEVENT *pEvent)
{
//	GetAISystem()->Record(this, IAIRecordable::E_EVENT, GetEventName(eType));
	bool bWasEnabled = m_bEnabled;

	switch (eType)
	{
		case AIEVENT_DROPBEACON:
			UpdateBeacon();
		break;
		case AIEVENT_CLEAR:
			{
				CCCPOINT(CPuppet_Event_Clear);

				ClearActiveGoals();
				m_bLooseAttention = false;
				GetAISystem()->FreeFormationPoint( GetWeakRef(this) );
				SetAttentionTarget(NILREF);
				m_bBlocked = false;
				m_bCanReceiveSignals = true;
			}
		break;
		case AIEVENT_CLEARACTIVEGOALS:
			ClearActiveGoals();
			m_bBlocked = false;
		break;
		case AIEVENT_DISABLE:
			{
				m_bEnabled = false;
//			m_bUncheckedBody = false;
			
				// Reset and disable the agent's target track
				const tAIObjectID aiObjectId = GetAIObjectID();
				gAIEnv.pTargetTrackManager->ResetAgent(aiObjectId);
				gAIEnv.pTargetTrackManager->SetAgentEnabled(aiObjectId, false);
				
				GetAISystem()->UpdateGroupStatus(GetGroupId());
				GetAISystem()->NotifyEnableState(this, m_bEnabled);
				UnregisterVision();
			}
		break;
		case AIEVENT_ENABLE:
			if (GetProxy()->GetActorHealth() < 1)
			{
// can happen when rendering dead bodies? AI should not be enabled
//				AIAssert(!"Trying to enable dead character!");
				return;
			}
			m_bEnabled = true;
			gAIEnv.pTargetTrackManager->SetAgentEnabled(GetAIObjectID(), true);
			GetAISystem()->UpdateGroupStatus(GetGroupId());
			GetAISystem()->NotifyEnableState(this, m_bEnabled);
			RegisterVision();
		break;
		case AIEVENT_SLEEP:
			m_fireMode = FIREMODE_OFF;
			m_bUncheckedBody = true;
			if ( GetProxy()->GetLinkedVehicleEntityId() == 0 )
			{
				m_bEnabled = false;
				GetAISystem()->NotifyEnableState(this, m_bEnabled);
			}
		break;
		case AIEVENT_WAKEUP:
			ClearActiveGoals();
			m_bLooseAttention = false;
			SetAttentionTarget(NILREF);
			m_bEnabled = true;
			GetAISystem()->NotifyEnableState(this, m_bEnabled);
			m_bUncheckedBody = false;
			GetAISystem()->UpdateGroupStatus(GetGroupId());
		break;
		case AIEVENT_ONVISUALSTIMULUS:
				HandleVisualStimulus(pEvent);
			break;
		case AIEVENT_ONPATHDECISION:
				HandlePathDecision(pEvent);
			break;
		case AIEVENT_ONSOUNDEVENT:
				HandleSoundEvent(pEvent);
			break;
		case AIEVENT_ONBULLETRAIN:
				HandleBulletRain(pEvent);
			break;
		case AIEVENT_AGENTDIED:
			{
				CAISystem* pAISystem = GetAISystem();

				pAISystem->NotifyTargetDead(this);

				m_bUncheckedBody = true;
				m_bEnabled = false;
				pAISystem->NotifyEnableState(this, m_bEnabled);

				pAISystem->RemoveFromGroup(GetGroupId(), this);

				pAISystem->ReleaseFormationPoint(this);
				gAIEnv.pPathfinder->CancelAnyPathsFor(this);
				ReleaseFormation();

				ResetCurrentPipe( true );
				m_State.vSignals.clear();

				// Create indication of dead body.
				pAISystem->RegisterDeadBody(m_Parameters.m_nSpecies, GetPos());

				ResetAlertness();

				pAISystem->OnAgentDeath(GetEntityID());

				if (GetProxy())
					GetProxy()->Reset(AIOBJRESET_SHUTDOWN);
			}
			break;
		case AIEVENT_FORCEDNAVIGATION:
			m_vForcedNavigation = pEvent->vForcedNavigation;
			break;
		case AIEVENT_ADJUSTPATH:
			m_adjustpath =  pEvent->nType;
			break;
		default:
			// triggering non-existing event - script error?
			// no error - various e AIEVENT_PLAYER_STUNT_ go here - processed in AIPlayer
			//AIAssert(0);
			break;
	}

	// Activate tree if enabled state is changing
	if (bWasEnabled != m_bEnabled)
	{
		const TBSSProfileUserId userId = GetBehaviorTreeUserId();
		GetAISystem()->GetBSSProfileManager()->EnableUser(userId, m_bEnabled);

		// remove from alertness counters
		ResetAlertness();
	}
}


//===================================================================
// HandleVisualStimulus
//===================================================================
void CPuppet::HandleVisualStimulus(SAIEVENT* pAIEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	const float fVisualPerceptionScale = m_Parameters.m_PerceptionParams.perceptionScale.visual;
	if (gAIEnv.CVars.IgnoreVisualStimulus != 0 || m_Parameters.m_bAiIgnoreFgNode || fVisualPerceptionScale <= 0.0f)
		return;

	if (gAIEnv.pTargetTrackManager->IsEnabled())
	{
		// Check if in range (using perception scale)
		if (eFOV_Outside != IsPointInFOV(pAIEvent->vPosition, fVisualPerceptionScale))
		{
			gAIEnv.pTargetTrackManager->HandleStimulusEvent(GetAIObjectID(), pAIEvent, TargetTrackHelpers::eEST_Visual);
		}
	}
	else if (m_pPerceptionHandler)
		m_pPerceptionHandler->HandleVisualStimulus(pAIEvent);
}

//===================================================================
// CheckPointInView
//===================================================================
IAIObject::EFieldOfViewResult CPuppet::CheckPointInFOV(const Vec3& vPoint, float fSightRange) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	EFieldOfViewResult eResult = eFOV_Outside;
	Vec3 vDirection = vPoint - GetPos();
	const float fDirectionLengthSq = vDirection.GetLengthSquared();
	vDirection.NormalizeSafe();

	// lets see if it is outside of its vision range
	if (fDirectionLengthSq > 0.1f && fDirectionLengthSq <= sqr(fSightRange))
	{
		const Vec3 vViewDir = GetViewDir().GetNormalizedSafe();
		const float fDot = vDirection.Dot(vViewDir);

		// Check for the omni-directional special case.
		if (m_FOVSecondaryCos <= -1.0f || fDot >= m_FOVSecondaryCos)
		{
			const bool bInPrimaryFOV = (m_FOVPrimaryCos <= -1.0f || fDot >= m_FOVPrimaryCos);
			eResult = bInPrimaryFOV ? eFOV_Primary : eFOV_Secondary;
		}
	}

	return eResult;
}

//===================================================================
// IsObjectInView
//===================================================================
IAIObject::EFieldOfViewResult CPuppet::IsObjectInFOV(CAIObject* pTarget, float distanceScale) const
{
	CCCPOINT(CPuppet_IsObjectInFOVCone);
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	const Vec3& vTargetPos = pTarget->GetPos();
	const float fSightRange = GetMaxTargetVisibleRange(pTarget) * distanceScale;
	return CheckPointInFOV(vTargetPos, fSightRange);
}

//===================================================================
// IsPointInView
//===================================================================
IAIObject::EFieldOfViewResult CPuppet::IsPointInFOV(const Vec3 &pos, float distanceScale) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	
	const float fSightRange = m_Parameters.m_PerceptionParams.sightRange * distanceScale;
	return CheckPointInFOV(pos, fSightRange);
}

//===================================================================
// GetPersonalInterestManager
//===================================================================
CPersonalInterestManager* CPuppet::GetPersonalInterestManager()
{
	return(CCentralInterestManager::GetInstance()->FindPIM(GetEntity()));
}

//===================================================================
// UpdateLookTarget
//===================================================================
void CPuppet::UpdateLookTarget(CAIObject *pTarget)
{
	CCCPOINT(CPuppet_UpdateLookTarget);
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	Vec3 lookTarget(0,0,0);

	if( pTarget == NULL)
	{
		CPersonalInterestManager* pPIM = GetPersonalInterestManager();
		if (pPIM && pPIM->IsInterested())
		{
			CWeakRef<CAIObject> refInterest = pPIM->GetInterestDummyPoint();
			CAIObject *pInterest = refInterest.GetAIObject();
			if (pInterest)
			{
				ELookStyle eStyle = pPIM->GetLookingStyle();
				if(eStyle == LOOKSTYLE_HARD || eStyle == LOOKSTYLE_SOFT)
				{
					SetAllowedStrafeDistances(999999.0f, 999999.0f, true);
				}
				SetLookStyle(eStyle);
				pTarget = pInterest;
			}
		}
	}

	// Don't look at targets that aren't at least interesting
	if (pTarget && m_refAttentionTarget.GetAIObject() == pTarget && GetAttentionTargetThreat() == AITHREAT_NONE)
		pTarget = NULL;

	// Update look direction and strafing
	bool lookAtTarget = false;
	// If not moving allow to look at target.
	if (m_State.fDesiredSpeed < 0.01f || m_Path.Empty())
		lookAtTarget = true;

	// Check if strafing should be allowed.
	UpdateStrafing();
	if (m_State.allowStrafing)
		lookAtTarget = true;
	if (m_bLooseAttention)
	{
		CAIObject *pLooseAttentionTarget = m_refLooseAttentionTarget.GetAIObject();
		if (pLooseAttentionTarget)
			pTarget = pLooseAttentionTarget;
	}
	if (m_refFireTarget.IsValid() && m_fireMode != FIREMODE_OFF)
		pTarget = GetFireTargetObject();

	if(m_fireMode == FIREMODE_MELEE || m_fireMode == FIREMODE_MELEE_FORCED)
	{
		if (pTarget)
		{
			lookTarget = pTarget->GetPos();
			lookTarget.z = GetPos().z;
			lookAtTarget = true;
		}
	}

	bool	use3DNav = IsUsing3DNavigation();
	bool isMoving = m_State.fDesiredSpeed > 0.0f && m_State.curActorTargetPhase == eATP_None && !m_State.vMoveDir.IsZero();

	Vec3 dirToTarget(0,0,0);
	float distToTarget = FLT_MAX;
	if (pTarget)
	{
		dirToTarget = pTarget->GetPos() - GetPos();
		distToTarget = dirToTarget.GetLength();
		if(distToTarget > 0.0001f)
			dirToTarget /= distToTarget;

		// Allow to look at the target when it is almost at the movement direction or very close.
		if (isMoving)
		{
			Vec3	move(m_State.vMoveDir);
			if (!use3DNav)
				move.z = 0.0f;
			move.NormalizeSafe(Vec3(0,0,0));
			if (distToTarget < 2.5f || move.Dot(dirToTarget) > cosf(DEG2RAD(60)))
				lookAtTarget = true;
		}
	}

	if (lookAtTarget && pTarget)
	{
		Vec3	vTargetPos = pTarget->GetPos();

		const float	maxDeviation = distToTarget * sinf(DEG2RAD(15));

		if(distToTarget > GetParameters().m_fPassRadius)
		{
			lookTarget = vTargetPos;
			Limit(lookTarget.z, GetPos().z - maxDeviation, GetPos().z + maxDeviation);
		}

		// Clamp the lookat height when the target is close.
		int TargetType = pTarget->GetType();
		if (distToTarget < 1.0f ||
			(TargetType == AIOBJECT_DUMMY || TargetType == AIOBJECT_HIDEPOINT || TargetType == AIOBJECT_WAYPOINT ||
			TargetType > AIOBJECT_PLAYER) && distToTarget < 5.0f) // anchors & dummy objects
		{
			if(!use3DNav)
			{
				lookTarget = vTargetPos;
				Limit(lookTarget.z, GetPos().z - maxDeviation, GetPos().z + maxDeviation);
			}
		}
	}
	else if (isMoving)
	{
		// Check if strafing should be allowed.

		// Look forward or to the movement direction
		Vec3  lookAheadPoint;
		float	lookAheadDist = 2.5f;

		if(m_pPathFollower)
		{
      float junk;
			lookAheadPoint = m_pPathFollower->GetPathPointAhead(lookAheadDist, junk);
		}
		else
		{
			if(!m_Path.GetPosAlongPath(lookAheadPoint, lookAheadDist, !m_movementAbility.b3DMove, true))
				lookAheadPoint = GetPhysicsPos();
		}

		// Since the path height is not guaranteed to follow terrain, do not even try to look up or down.
		lookTarget = lookAheadPoint;

		// Make sure the lookahead position is far enough so that the catchup logic in the path following
		// together with look-ik does not get flipped.
		Vec3  delta = lookTarget - GetPhysicsPos();
		delta.z = 0.0f;
		float dist = delta.GetLengthSquared();
		if (dist < sqr(1.0f))
		{
			float u = 1.0f - sqrtf(dist);
			Vec3 safeDir = GetBodyDir();
			safeDir.z = 0;
			delta = delta + (safeDir - delta) * u;
		}
		delta.Normalize();

		lookTarget = GetPhysicsPos() + delta * 40.0f;
		lookTarget.z = GetPos().z;
	}
	else
	{
		// Disable look target.
		lookTarget.zero();
	}

	if (!m_posLookAtSmartObject.IsZero())
	{
		// The SO lookat should override the lookat target in case not requesting to fire and not using lookat goalop.
		if (!m_bLooseAttention && m_fireMode == FIREMODE_OFF)
		{
			lookTarget = m_posLookAtSmartObject;
		}
	}

	if (!lookTarget.IsZero())
	{
		if (m_allowStrafeLookWhileMoving && m_fireMode != FIREMODE_OFF && GetFireTargetObject())
		{
			float distSqr = Distance::Point_Point2DSq(GetFireTargetObject()->GetPos(), GetPos());
			if (!m_closeRangeStrafing)
			{
				// Outside the range
				const float thr = GetParameters().m_PerceptionParams.sightRange * 0.12f;
				if (distSqr < sqr(thr))
					m_closeRangeStrafing = true;
			}
			if (m_closeRangeStrafing)
			{
				// Inside the range
				m_State.allowStrafing = true;
				const float thr = GetParameters().m_PerceptionParams.sightRange * 0.12f + 2.0f;
				if (distSqr > sqr(thr))
					m_closeRangeStrafing = false;
			}
		}

		float distSqr = Distance::Point_Point2DSq(lookTarget, GetPos());
		if (distSqr < sqr(2.0f))
		{
			Vec3 fakePos = GetPos() + GetBodyDir() * 2.0f;
			if (distSqr < sqr(0.7f))
				lookTarget = fakePos;
			else
			{
				float speed = m_State.vMoveDir.GetLength();
				speed = clamp(speed,0.0f,10.f);
				float d = sqrtf(distSqr);
				float u = 1.0f - (d - 0.7f) / (2.0f - 0.7f);
				lookTarget += speed/10 * u * (fakePos - lookTarget);
			}
		}
	}

	// for the invehicle gunners
	if ( GetProxy() )
	{
		SAIBodyInfo bodyInfo;
		GetProxy()->QueryBodyInfo(bodyInfo);

		if ( bodyInfo.linkedVehicleEntity )
		{
			if ( GetProxy()->GetActorIsFallen() )
			{
				lookTarget.zero();
			}
			else
			{
				CAIObject* pUnit = (CAIObject*)bodyInfo.linkedVehicleEntity->GetAI();
				if (pUnit)
				{
					if (pUnit->CastToCAIVehicle())
					{
						lookTarget.zero();
						CAIObject *pLooseAttentionTarget = m_refLooseAttentionTarget.GetAIObject();
						if ( m_bLooseAttention && pLooseAttentionTarget )
							pTarget = pLooseAttentionTarget;
						if ( pTarget )
						{
							lookTarget = pTarget->GetPos();
							m_State.allowStrafing = false;
						}
					}
				}
			}
		}
	}

	float lookTurnSpeed = GetAlertness() > 0 ? m_Parameters.m_lookCombatTurnSpeed : m_Parameters.m_lookIdleTurnSpeed;
	if (lookTurnSpeed <= 0.0f)
		m_State.vLookTargetPos = lookTarget;
	else
		m_State.vLookTargetPos = InterpolateLookOrAimTargetPos(m_State.vLookTargetPos, lookTarget, lookTurnSpeed);
}

//===================================================================
// InterpolateLookOrAimTargetPos
//===================================================================
Vec3 CPuppet::InterpolateLookOrAimTargetPos(const Vec3& current, const Vec3& target, float maxRatePerSec)
{
	if (!target.IsZero())
	{
		// Interpolate.
		Vec3	curDir;
		if (current.IsZero())
		{
			curDir = GetBodyDir(); // GetViewDir();
		}
		else
		{
			curDir = current - GetPos();
			curDir.Normalize();
		}

		Vec3	reqDir = target - m_vLastPosition; // GetPos();
		float dist = reqDir.NormalizeSafe();

		// Slerp
		float cosAngle = curDir.Dot(reqDir);

		const float eps = 1e-6f;
		const float maxRate = maxRatePerSec * GetAISystem()->GetFrameDeltaTime();
		const float thr = cosf(maxRate);

		if (cosAngle > thr || maxRate < eps)
		{
			return target;
		}
		else
		{
			float angle = acos_tpl(cosAngle);

			// Allow higher rate when over 90degrees.
			float scale = 1.0f + clamp((angle - gf_PI/4.0f)/(gf_PI/4.0f), 0.0f, 1.0f)*2.0f;
			if (angle < eps || angle < maxRate*scale)
				return target;

			float t = (maxRate*scale) / angle;

			Quat	curView;
			curView.SetRotationVDir(curDir);
			Quat	reqView;
			reqView.SetRotationVDir(reqDir);

			Quat	view;
			view.SetSlerp(curView, reqView, t);

			return GetPos() + (view * FORWARD_DIRECTION) * dist;
		}
	}

	// Clear look target.
	return target;
}

//===================================================================
// UpdateLookTarget3D
//===================================================================
void CPuppet::UpdateLookTarget3D(CAIObject *pTarget)
{
	// this is for the scout mainly

	m_State.vLookTargetPos.zero();
	m_State.aimTargetIsValid = false;	//	m_State.vAimTargetPos.zero();

	CAIObject *pLooseAttentionTarget = m_refLooseAttentionTarget.GetAIObject();
	if ( m_bLooseAttention && pLooseAttentionTarget )
	{
		pTarget = pLooseAttentionTarget;
		m_State.vLookTargetPos = pTarget->GetPos();
}
	m_State.vForcedNavigation =m_vForcedNavigation;

}
//===================================================================
// AdjustPath
//===================================================================
void CPuppet::AdjustPath()
{
	const AgentPathfindingProperties& pathProps = m_movementAbility.pathfindingProperties;

	if (pathProps.avoidObstacles)
	{
		if (!AdjustPathAroundObstacles())
		{
			AILogEvent("CPuppet::AdjustPath Unable to adjust path around obstacles for %s", GetName());
			ClearPath("CPuppet::AdjustPath m_Path");
			m_nPathDecision = PATHFINDER_NOPATH;
			return;
		}
	}

	if (m_adjustpath > 0)
	{
		IAISystem::tNavCapMask navCapMask = pathProps.navCapMask;
		int nBuildingID;
		IVisArea *pArea;
		IAISystem::ENavigationType currentNavType	= gAIEnv.pNavigation->CheckNavigationType(GetPos(), nBuildingID, pArea, navCapMask);
		ConvertPathToSpline(currentNavType);
	}
}

//===================================================================
// AdjustMovementUrgency
//===================================================================
void CPuppet::AdjustMovementUrgency(float& urgency, float pathLength, float* maxPathLen)
{
	if (!maxPathLen)
		maxPathLen = &m_adaptiveUrgencyMaxPathLen;

	if(m_adaptiveUrgencyScaleDownPathLen > 0.0001f)
	{
		if(pathLength > *maxPathLen)
		{
			*maxPathLen = pathLength;

			int scaleDown = 0;
			float scale = m_adaptiveUrgencyScaleDownPathLen;
			while(scale > m_adaptiveUrgencyMaxPathLen && scaleDown < 4)
			{
				scale /= 2.0f;
				scaleDown++;
			}

			int minIdx = MovementUrgencyToIndex(m_adaptiveUrgencyMin);
			int maxIdx = MovementUrgencyToIndex(m_adaptiveUrgencyMax);
			int idx = maxIdx - scaleDown;
			if(idx < minIdx) idx = minIdx;

			// Special case for really short paths.
			if (*maxPathLen < 1.2f && idx > 2)
				idx = 2; // walk

			urgency = IndexToMovementUrgency(idx);
		}
	}
}

//===================================================================
// HandlePathDecision
//===================================================================
void CPuppet::HandlePathDecision(SAIEVENT *pEvent)
{
	static bool dumpOrigPath = false;
	if (dumpOrigPath)
	{
		gAIEnv.pPathfinder->GetNavPath().Dump("AI system path");
		dumpOrigPath = false;
	}


  if (pEvent->bPathFound && !gAIEnv.pPathfinder->GetNavPath().Empty())
	{
    if (gAIEnv.CVars.DebugPathFinding)
    {
      Vec3 end(-1, -1, -1);
      Vec3 start(-1, -1, -1);
      Vec3 next(-1, -1, -1);
      if (!gAIEnv.pPathfinder->GetNavPath().Empty())
      {
        end = gAIEnv.pPathfinder->GetNavPath().GetLastPathPos();
        start = gAIEnv.pPathfinder->GetNavPath().GetPrevPathPoint()->vPos;
        next = gAIEnv.pPathfinder->GetNavPath().GetNextPathPos();
      }
      const SNavPathParams &params = gAIEnv.pPathfinder->GetNavPath().GetParams();
      AILogAlways("CPuppet::HandlePathDecision %s found path to (%5.2f, %5.2f, %5.2f) from (%5.2f, %5.2f, %5.2f) Stored path end is (%5.2f, %5.2f, %5.2f) Next is (%5.2f, %5.2f, %5.2f)",
        GetName(), 
        end.x, end.y, end.z, 
        start.x, start.y, start.z, 
        params.end.x, params.end.y, params.end.z,
        next.x, next.y, next.z);
    }

    m_nPathDecision = PATHFINDER_PATHFOUND;

		// Check if there is a pending NavSO which has not started playing yet and cancel it.
		// Path regeneration while the animation is playing is ok, but should be inhibited while preparing for actor target.
		// This may happen if some goal operation or such requests path regeneration
		// while the actor target has been requested. This should not happen.
		if(m_eNavSOMethod != nSOmNone && m_State.curActorTargetPhase == eATP_Waiting)
		{
			if(gAIEnv.CVars.DebugPathFinding)
			{
				if(!m_State.actorTargetReq.animation.empty())
					AILogAlways("CPuppet::HandlePathDecision %s got new path during actor target request (phase:%d, anim:%s). Please investigate, this case should not happen.",
						GetName(), (int)m_State.curActorTargetPhase, m_State.actorTargetReq.animation.c_str());
				else
					AILogAlways("CPuppet::HandlePathDecision %s got new path during actor target request (phase:%d, vehicle:%s, seat:%d). Please investigate, this case should not happen.",
						GetName(), (int)m_State.curActorTargetPhase, m_State.actorTargetReq.vehicleName.c_str(), m_State.actorTargetReq.vehicleSeat);
			}
			SetNavSOFailureStates();
		}

		// Check if there is a pending end-of-path actor target request which has not started playing yet and cancel it.
		// Path regeneration should be inhibited while preparing for actor target.
		if(GetActiveActorTargetRequest() && m_State.curActorTargetPhase == eATP_Waiting)
		{
			if(gAIEnv.CVars.DebugPathFinding)
			{
				if(!m_State.actorTargetReq.animation.empty())
					AILogAlways("CPuppet::HandlePathDecision %s got new path during actor target request (phase:%d, anim:%s). Please investigate, this case should not happen.",
					GetName(), (int)m_State.curActorTargetPhase, m_State.actorTargetReq.animation.c_str());
				else
					AILogAlways("CPuppet::HandlePathDecision %s got new path during actor target request (phase:%d, vehicle:%s, seat:%d). Please investigate, this case should not happen.",
					GetName(), (int)m_State.curActorTargetPhase, m_State.actorTargetReq.vehicleName.c_str(), m_State.actorTargetReq.vehicleSeat);
			}
			// Stop exact positioning.
			m_State.actorTargetReq.Reset();
		}

		// lets get it from the ai system
		// [Mikko] The AIsystem working path version does not always match the 
		// pipeuser path version. When new path is found, for it to be used.
		// In some weird cases the new path version ca even be the same as the
		// current one. Make sure the new path version is different than the current.
		int oldVersion = m_Path.GetVersion();
		m_Path = gAIEnv.pPathfinder->GetNavPath();
		m_Path.SetVersion(oldVersion+1);

		// Store the path destination point for later checks.
		if(!m_Path.Empty())
			m_PathDestinationPos = m_Path.GetLastPathPos();

		// Cut the path end
		float trimLength = m_Path.GetParams().endDistance;
		if (fabsf(trimLength) > 0.0001f)
			m_Path.TrimPath(trimLength, IsUsing3DNavigation());

		// Trim the path so that it will only reach until the first navSO animation.
		m_Path.PrepareNavigationalSmartObjects(this);
		
		// Update adaptive urgency control before the path gets processed further
		AdjustMovementUrgency(m_State.fMovementUrgency, m_Path.GetPathLength(IsUsing3DNavigation()));

		AdjustPath();
		
		m_OrigPath = m_Path;
    // AdjustPath may have failed 
    if (m_nPathDecision == PATHFINDER_PATHFOUND && m_OrigPath.GetPath().size() < 2)
    {
      AIWarning("CPuppet::HandlePathDecision (%5.2f, %5.2f, %5.2f) original path size = %d for %s",
				GetPos().x, GetPos().y, GetPos().z, m_OrigPath.GetPath().size(), GetName());
    }
	
	}
	else
	{
		if (gAIEnv.CVars.DebugPathFinding)
		{
			AILogAlways("CPuppet::HandlePathDecision %s No path", GetName());
		}
		ClearPath("CPuppet::HandlePathDecision m_Path");
		//m_OrigPath.Clear("CPuppet::HandlePathDecision m_OrigPath");
		m_nPathDecision = PATHFINDER_NOPATH;
	} 
}

//===================================================================
// Devalue
//===================================================================
void CPuppet::Devalue(IAIObject *pObject, bool bDevaluePuppets, float	fAmount)
{
	DevaluedMap::iterator di;

	unsigned short type = ((CAIObject*)pObject)->GetType();

	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
	CWeakRef<CAIObject> refObject = GetWeakRefSafe((CAIObject*)pObject);

	if (pObject == pAttentionTarget)
		SetAttentionTarget(NILREF);

	if ((type == AIOBJECT_PUPPET) && !bDevaluePuppets)
		return;

	if (type == AIOBJECT_PLAYER)
		return;

	CCCPOINT(CPuppet_Devalue);

	// remove it from map of pending events, so that it does not get reacquire
	if (m_pPerceptionHandler)
		m_pPerceptionHandler->RemoveEvent(refObject);

	if ((di = m_mapDevaluedPoints.find(refObject)) == m_mapDevaluedPoints.end())
		m_mapDevaluedPoints.insert(DevaluedMap::iterator::value_type(refObject,fAmount));
}

//===================================================================
// IsDevalued
//===================================================================
bool CPuppet::IsDevalued(IAIObject *pObject)
{
	CWeakRef<CAIObject> refObject = GetWeakRefSafe((CAIObject*)pObject);
	return m_mapDevaluedPoints.find( refObject ) != m_mapDevaluedPoints.end();
}

//===================================================================
// ClearDevalued
//===================================================================
void CPuppet::ClearDevalued()
{
	m_mapDevaluedPoints.clear();
}

//===================================================================
// OnObjectRemoved
//===================================================================
void CPuppet::OnObjectRemoved(CAIObject *pObject)
{
	CAIActor::OnObjectRemoved(pObject);

	if (m_pPerceptionHandler)
		m_pPerceptionHandler->RemoveEvent(GetWeakRef(pObject));

  if (!m_steeringObjects.empty())
    m_steeringObjects.erase(std::remove(m_steeringObjects.begin(), m_steeringObjects.end(), pObject), m_steeringObjects.end());

	// Devalued points take care of themselves with refs

	RemoveFromGoalPipe(pObject);
}

//===================================================================
// UpTargetPriority
//===================================================================
void CPuppet::UpTargetPriority(const IAIObject* pTarget, float fPriorityIncrement)
{
	if (m_pPerceptionHandler)
		m_pPerceptionHandler->UpTargetPriority(GetWeakRef((CAIObject*)pTarget), fPriorityIncrement);
}

//===================================================================
// HandleSoundEvent
//===================================================================
void CPuppet::HandleSoundEvent(SAIEVENT* pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	const float fAudioPerceptionScale = m_Parameters.m_PerceptionParams.perceptionScale.audio;
	if (gAIEnv.CVars.IgnoreSoundStimulus != 0 || m_Parameters.m_bAiIgnoreFgNode || fAudioPerceptionScale <= 0.0f)
		return;

	if (gAIEnv.pTargetTrackManager->IsEnabled())
	{
		// Check if in range (using perception scale)
		const Vec3 &vMyPos = GetPos();
		const float fSoundDistance = vMyPos.GetDistance(pEvent->vPosition) * (1.0f / fAudioPerceptionScale);
		if (fSoundDistance <= pEvent->fThreat)
		{
			gAIEnv.pTargetTrackManager->HandleStimulusEvent(GetAIObjectID(), pEvent, TargetTrackHelpers::eEST_Sound);
		}
	}
	if (m_pPerceptionHandler)
		m_pPerceptionHandler->HandleSoundEvent(pEvent);
}

//===================================================================
// HandleBulletRain
//===================================================================
void CPuppet::HandleBulletRain(SAIEVENT *pEvent)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	if (gAIEnv.CVars.IgnoreBulletRainStimulus != 0 || m_Parameters.m_bAiIgnoreFgNode)
		return;

	IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
	pData->point = pEvent->vPosition;
	pData->nID = pEvent->sourceId;
	SetSignal(0, "OnBulletRain", GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnBulletRain);

	if (gAIEnv.pTargetTrackManager->IsEnabled())
		gAIEnv.pTargetTrackManager->HandleStimulusEvent(GetAIObjectID(), pEvent, TargetTrackHelpers::eEST_BulletRain);
	else if (m_pPerceptionHandler)
		m_pPerceptionHandler->HandleBulletRain(pEvent);
}

//===================================================================
// GetMovementSpeedRange
//===================================================================
void CPuppet::GetMovementSpeedRange(float fUrgency, bool slowForStrafe, float& normalSpeed, float& minSpeed, float& maxSpeed) const
{
	AgentMovementSpeeds::EAgentMovementUrgency urgency;
	AgentMovementSpeeds::EAgentMovementStance stance;

	bool vehicle = GetType() == AIOBJECT_VEHICLE;

	if (fUrgency < 0.5f * (AISPEED_SLOW + AISPEED_WALK))
		urgency = AgentMovementSpeeds::AMU_SLOW;
	else if (fUrgency < 0.5f * (AISPEED_WALK + AISPEED_RUN))
		urgency = AgentMovementSpeeds::AMU_WALK;
	else if (fUrgency < 0.5f * (AISPEED_RUN + AISPEED_SPRINT))
		urgency = AgentMovementSpeeds::AMU_RUN;
	else
		urgency = AgentMovementSpeeds::AMU_SPRINT;
/*	{
		if (slowForStrafe)
			urgency = AgentMovementSpeeds::AMU_RUN;
		else
			urgency = AgentMovementSpeeds::AMU_SPRINT;
	}*/
  
	if (IsAffectedByLightLevel() && m_movementAbility.lightAffectsSpeed)
	{
		// Disable sprinting in dark light conditions.
		if (urgency == AgentMovementSpeeds::AMU_SPRINT)
		{
			if (GetLightLevel() == AILL_DARK)
				urgency = AgentMovementSpeeds::AMU_RUN;
		}
	}

	SAIBodyInfo bodyInfo;
	GetProxy()->QueryBodyInfo(bodyInfo);

	switch (bodyInfo.stance)
	{
	case STANCE_STEALTH: stance = AgentMovementSpeeds::AMS_STEALTH; break;
	case STANCE_CROUCH: stance = AgentMovementSpeeds::AMS_CROUCH; break;
	case STANCE_PRONE: stance = AgentMovementSpeeds::AMS_PRONE; break;
	case STANCE_SWIM: stance = AgentMovementSpeeds::AMS_SWIM; break;
	case STANCE_RELAXED: stance = AgentMovementSpeeds::AMS_RELAXED; break;
	case STANCE_LOW_COVER: stance = AgentMovementSpeeds::AMS_LOW_COVER; break;
	case STANCE_HIGH_COVER: stance = AgentMovementSpeeds::AMS_HIGH_COVER; break;
	default: stance = AgentMovementSpeeds::AMS_COMBAT; break;
	}

	const float artificialMinSpeedMult = 1.0f;

	AgentMovementSpeeds::SSpeedRange	fwdRange = m_movementAbility.movementSpeeds.GetRange(stance, urgency);
	fwdRange.min *= artificialMinSpeedMult;
	normalSpeed = fwdRange.def;
	minSpeed = fwdRange.min;
	maxSpeed = fwdRange.max;

	if (m_movementAbility.directionalScaleRefSpeedMin > 0.0f)
	{
		float desiredSpeed = normalSpeed;
		float desiredTurnSpeed = m_bodyTurningSpeed;
		float travelAngle = Ang3::CreateRadZ(GetBodyDir(), GetMoveDir()); //m_State.vMoveDir);

		const float refSpeedMin = m_movementAbility.directionalScaleRefSpeedMin;
		const float refSpeedMax = m_movementAbility.directionalScaleRefSpeedMax;

		// When a locomotion is slow (0.5-2.0f), then we can do this motion in all direction more or less at the same speed 
		float t = sqr(clamp((desiredSpeed - refSpeedMin) / (refSpeedMax - refSpeedMin), 0.0f, 1.0f));
		float scaleLimit = clamp(0.8f*(1-t) + 0.1f*t, 0.3f, 1.0f); //never scale more then 0.4 down 

		//adjust desired speed for turns
		float speedScale = 1.0f - fabsf(desiredTurnSpeed*0.40f)/gf_PI;
		speedScale = clamp(speedScale, scaleLimit, 1.0f);

		//adjust desired speed when strafing and running backward
		float strafeSlowDown = (gf_PI - fabsf(travelAngle*0.60f))/gf_PI;
		strafeSlowDown = clamp(strafeSlowDown, scaleLimit, 1.0f);

		//adjust desired speed when running uphill & downhill
		float slopeSlowDown = (gf_PI - fabsf(DEG2RAD(bodyInfo.slopeAngle)/12.0f))/gf_PI;
		slopeSlowDown = clamp(slopeSlowDown, scaleLimit, 1.0f);

		float scale = min(speedScale, min(strafeSlowDown, slopeSlowDown));

		normalSpeed *= scale;
		Limit(normalSpeed, minSpeed, maxSpeed);
//		minSpeed *= scale;
//		maxSpeed *= scale;
	}


/*
//	float fwd = m_movementAbility.movementSpeeds.GetSpeed(urgency, stance, AgentMovementSpeeds::AMD_FWD);
//	float speed = fwd;

	// todo Danny: really should only allow this speed control whilst strafing, since otherwise 
	// idle->move transitions will not get selected properly. However, if the speed control is not
	// done always then we end up seeing very fast back/sideways movement.
	if (slowForStrafe)
	{
		AgentMovementSpeeds::SSpeedRange	sideRange = m_movementAbility.movementSpeeds.GetRange(stance, urgency, AgentMovementSpeeds::AMD_SIDE);
		AgentMovementSpeeds::SSpeedRange	bwdRange = m_movementAbility.movementSpeeds.GetRange(stance, urgency, AgentMovementSpeeds::AMD_BWD);

		sideRange.min *= artificialMinSpeedMult;
		bwdRange.min *= artificialMinSpeedMult;

//		float side = m_movementAbility.movementSpeeds.GetSpeed(urgency, stance, AgentMovementSpeeds::AMD_SIDE);
//		float bwd = m_movementAbility.movementSpeeds.GetSpeed(urgency, stance, AgentMovementSpeeds::AMD_BWD);

		Vec3 velDir = GetVelocity();
		Vec3 dir = GetBodyDir();
		velDir.z = dir.z = 0.0f;
		velDir.NormalizeSafe(); dir.NormalizeSafe();
		float dot = clamp(velDir.Dot(dir), -1.0f, 1.0f);
		float angle = acos_tpl(dot);
		float angleScale = 1.0f - 2.0f * (angle / gf_PI);
		float strafeScale = sgn(angleScale) * sqr(angleScale);	// angleScale

		if (angleScale > 0.0f)
		{
			const float t = strafeScale; //SmoothStep(0.0f, 1.0f, strafeScale);
			normalSpeed = t * fwdRange.def + (1 - t) * sideRange.def;
			minSpeed = t * fwdRange.min + (1 - t) * sideRange.min;
			maxSpeed = t * fwdRange.max + (1 - t) * sideRange.max;
		}
		else
    {
			const float t = 1 + strafeScale; //SmoothStep(-1.0f, 0.0f, strafeScale);
			normalSpeed = t * sideRange.def + (1 - t) * bwdRange.def;
			minSpeed = t * sideRange.min + (1 - t) * bwdRange.min;
			maxSpeed = t * sideRange.max + (1 - t) * bwdRange.max;
		}
	}

	Limit(normalSpeed, minSpeed, maxSpeed);

	// Slow down based on lighting conditions.
	if (IsAffectedByLightLevel() && m_movementAbility.lightAffectsSpeed)
	{
		if (GetLightLevel() == AILL_DARK)
			normalSpeed = minSpeed + (normalSpeed - minSpeed) * gAIEnv.CVars.MovementSpeedMediumIllumMod;
		else if (GetLightLevel() == AILL_MEDIUM)
			normalSpeed = minSpeed + (normalSpeed - minSpeed) * gAIEnv.CVars.MovementSpeedDarkIllumMod;
	}
*/
	//	return speed;
}


//===================================================================
// GetObjectType
//===================================================================
CPuppet::EAIObjectType CPuppet::GetObjectType(const CAIObject *ai, unsigned short type)
{
  if (type == AIOBJECT_PLAYER)
    return AIOT_PLAYER;
  else if (type == AIOBJECT_PUPPET)
    return AIOT_AGENTSMALL;
  else if (type == AIOBJECT_VEHICLE)
  {
    // differentiate between medium and big vehicles (e.g. tank can drive over jeep)
    return AIOT_AGENTMED;
  }
  else
    return AIOT_UNKNOWN;
}

//===================================================================
// CreatePathFollower
//===================================================================
IPathFollower* CPuppet::CreatePathFollower(const PathFollowerParams &params)
{
	IPathFollower *pResult = NULL;

	if (gAIEnv.CVars.UseSmartPathFollower == 1)
	{
		CSmartPathFollower *pSmartPathFollower = new CSmartPathFollower(params, &m_pathAdjustmentObstacles);
		pResult = pSmartPathFollower;
	}
	else
	{
		pResult = new CPathFollower(params);
	}

	assert(pResult);
	return pResult;
}

//===================================================================
// GetNavInteraction
//===================================================================
CPuppet::ENavInteraction CPuppet::GetNavInteraction(const CAIObject *navigator, const CAIObject *obstacle)
{
  SAIBodyInfo info;
	
	IAIActorProxy* pProxy = obstacle->GetProxy();
	if (!pProxy)
		return NI_IGNORE;

  pProxy->QueryBodyInfo(info);
  if (info.linkedVehicleEntity)
    return NI_IGNORE;

  unsigned short navigatorType = navigator->GetType();
  unsigned short obstacleType = obstacle->GetType();

  bool enemy = navigator->IsHostile(obstacle);

  EAIObjectType navigatorOT = GetObjectType(navigator, navigatorType);
  EAIObjectType obstacleOT = GetObjectType(obstacle, obstacleType);

  switch (navigatorOT)
  {
  case AIOT_UNKNOWN: 
  case AIOT_PLAYER: 
    return NI_IGNORE;
  case AIOT_AGENTSMALL: 
    // don't navigate around their enemies, unless the enemy is bigger
/*    if (enemy)
      return obstacleOT > navigatorOT ? NI_STEER : NI_IGNORE;
    else*/
      return NI_STEER;
  case AIOT_AGENTMED: 
  case AIOT_AGENTBIG: 
    // don't navigate around their enemies, unless the enemy is same size or bigger
    if (enemy)
      return obstacleOT >= navigatorOT ? NI_STEER : NI_IGNORE;
    else
      return NI_STEER;
  default:
    AIError("GetNavInteraction: Unhandled switch case %d", navigatorOT);
    return NI_IGNORE;
  }
}


CPuppet::ENavInteraction CPuppet::NavigateAroundObjectsBasicCheck(const CAIObject *object) const
{
	//These checks have been factored out of the old NavigateAroundObjectsBasicCheck so that they can be used
	//by the old version as well as the new approach in NavigateAroundObjects

	ENavInteraction ret = NI_IGNORE;

	if (object)
	{
		//to make sure we skip disable entities (hidden in the game, etc)
		IEntity *pEntity(object->GetEntity());
		if(pEntity && pEntity->IsActive())
		{
			if (AIOBJECT_VEHICLE == object->GetType() || object->IsEnabled()) // vehicles are not enabled when idle, so don't skip them
			{
				if (this != object)
				{
					ret = GetNavInteraction(this, object);
				}
			}	
		}
	}

	return ret;
}

//===================================================================
// NavigateAroundObjectsBasicCheck
//===================================================================
bool CPuppet::NavigateAroundObjectsBasicCheck(const Vec3 & targetPos, const Vec3 &myPos, bool in3D, const CAIObject *object, float extraDist)
{
	bool ret = false;

	//now calling the factored out function
  ENavInteraction navInteraction = NavigateAroundObjectsBasicCheck(object);
  if (navInteraction != NI_IGNORE)
	{
		Vec3 objectPos = object->GetPos();
		Vec3 delta = objectPos - myPos;
		const CAIActor* pActor = object->CastToCAIActor();
		float avoidanceR = m_movementAbility.avoidanceRadius; 
		avoidanceR += pActor->m_movementAbility.avoidanceRadius;
		avoidanceR += extraDist;
		float avoidanceRSq = square(avoidanceR);

		// skip if we're not close
		float distSq = delta.GetLengthSquared();
		if (!(distSq > avoidanceRSq))
		{
			ret = true;
		}
	}

	return ret;
}

//===================================================================
// NavigateAroundObjectsInternal
//===================================================================
bool CPuppet::NavigateAroundObjectsInternal(const Vec3 & targetPos, const Vec3 &myPos, bool in3D, const CAIObject *object)
{
    // skip if we're not trying to move towards
  Vec3 objectPos = object->GetPos();
    Vec3 delta = objectPos - myPos;
    float dot = m_State.vMoveDir.Dot(delta);
    if (dot < 0.001f)
    return false;
  const CAIActor* pActor = object->CastToCAIActor();
    float avoidanceR = m_movementAbility.avoidanceRadius; 
    avoidanceR += pActor->m_movementAbility.avoidanceRadius;
    float avoidanceRSq = square(avoidanceR);

    // skip if we're not close
    float distSq = delta.GetLengthSquared();
    if (distSq > avoidanceRSq)
    return false;

  return NavigateAroundAIObject(targetPos, object, myPos, objectPos, true, in3D);
}



//===================================================================
// NavigateAroundObjects
//===================================================================
bool CPuppet::NavigateAroundObjects(const Vec3 &targetPos, bool fullUpdate, bool bSteerAroundTarget)
{
  FUNCTION_PROFILER( GetISystem(),PROFILE_AI );
  bool in3D = IsUsing3DNavigation();
  const Vec3 myPos = GetPhysicsPos();

  bool steering = false;
	bool lastSteeringEnabled = m_steeringEnabled;
	bool steeringEnabled = m_State.vMoveDir.GetLength() > 0.01f && m_State.fDesiredSpeed > 0.01f;

  CTimeValue curTime = GetAISystem()->GetFrameStartTime();
  float deltaT = (curTime - m_lastSteerTime).GetSeconds();
  static float timeForUpdate = 0.5f;

  if (steeringEnabled && (deltaT > timeForUpdate || !lastSteeringEnabled))
  {
		FRAME_PROFILER("NavigateAroundObjects Gather Objects", gEnv->pSystem, PROFILE_AI)

    m_lastSteerTime = curTime;
    m_steeringObjects.clear();

		//This approach now uses a cylinder overlap to collect the entities surrounding
		//this puppet. The cylinder has a height of 2.5m, the center of its bottom is 
		//at this puppets position. The radius used for the cylinder is the minimum 
		//of 2.5m (should probably be made configurable) and the distance to my final
		//goal. That way, when approaching the destpos we don't check any further.
		float radius = min(2.5f, Distance::Point_Point(myPos, m_Path.GetLastPathPos()));
		Lineseg lseg(myPos, myPos + Vec3(0.0f, 0.0f, 2.5f));

		//We are interested in living and dynamic entities
		static EAICollisionEntities colfilter = static_cast<EAICollisionEntities>(ent_living | AICE_DYNAMIC);

		int32 ent[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		//Gather up to 10 entities surrounding this puppet
		if (OverlapCylinder(lseg, radius, colfilter, GetPhysics(), 0, ent, 10))
		{

			//loop over the id-array until we find an id of 0
			for (int32 *entityid = ent; *entityid; ++entityid)
			{
				IEntity *entity = gEnv->pSystem->GetIEntitySystem()->GetEntity(*entityid);
				if (entity)
				{
					CAIObject *aiob = static_cast<CAIObject*>(entity->GetAI());

					//If this AIObject passes the test, push it back onto the vector
					if (NI_IGNORE != NavigateAroundObjectsBasicCheck(aiob))
					{
						m_steeringObjects.push_back(aiob);
					}
				}
			}
		}
	}


	if (steeringEnabled)
	{
		if (GetType() == AIOBJECT_PUPPET && !in3D)
		{
			FRAME_PROFILER("NavigateAroundObjects Update Steering", gEnv->pSystem, PROFILE_AI)

			bool check = fullUpdate;
			if (m_updatePriority == AIPUP_VERY_HIGH || m_updatePriority == AIPUP_HIGH)
				check = true;

			if (check || !lastSteeringEnabled)
			{
				AABB selfBounds;
				GetEntity()->GetWorldBounds(selfBounds);
				selfBounds.min.z -= 0.25f;
				selfBounds.max.z += 0.25f;

				// Build occupancy map
				const float radScale = 1.0f - clamp(m_steeringAdjustTime - 1.0f, 0.0f, 1.0f);
				const float selfRad = m_movementAbility.pathRadius * radScale;
				m_steeringOccupancy.Reset(GetPhysicsPos(), GetBodyDir(), m_movementAbility.avoidanceRadius * 2.0f);

				for (unsigned i = 0, ni = m_steeringObjects.size(); i < ni; ++i)
				{
					const CAIActor* pActor = m_steeringObjects[i]->CastToCAIActor();
					if (!pActor) // || !pActor->IsActive())
						continue;

					// Check that the height overlaps.
					IEntity* pActorEnt = pActor->GetEntity();
					AABB bounds;
					pActorEnt->GetWorldBounds(bounds);
					if (selfBounds.min.z >= bounds.max.z || selfBounds.max.z <= bounds.min.z)
						continue;

					if (pActor->GetType() == AIOBJECT_VEHICLE)
					{
						AABB localBounds;
						pActor->GetLocalBounds(localBounds);
						const Matrix34& tm = pActorEnt->GetWorldTM();

						SAIRect3 r;
						GetFloorRectangleFromOrientedBox(tm, localBounds, r);
						// Extend the box based on velocity.
						Vec3 vel = pActor->GetVelocity();
						float speedu = r.axisu.Dot(vel) * 0.25f;
						float speedv = r.axisv.Dot(vel) * 0.25f;
						if (speedu > 0)
							r.max.x += speedu;
						else
							r.min.x += speedu;
						if (speedv > 0)
							r.max.y += speedv;
						else
							r.min.y += speedv;

						// Extend the box based on the 
						r.min.x -= selfRad;
						r.min.y -= selfRad;
						r.max.x += selfRad;
						r.max.y += selfRad;

						const unsigned maxpts = 4;
						Vec3 rect[maxpts];
						rect[0] = r.center + r.axisu * r.min.x + r.axisv * r.min.y;
						rect[1] = r.center + r.axisu * r.max.x + r.axisv * r.min.y;
						rect[2] = r.center + r.axisu * r.max.x + r.axisv * r.max.y;
						rect[3] = r.center + r.axisu * r.min.x + r.axisv * r.max.y;

/*						unsigned last = maxpts-1;
						for (unsigned i = 0; i < maxpts; ++i)
						{
							GetAISystem()->AddDebugLine(rect[last], rect[i], 255,0,0, 0);
							last = i;
						}*/

						float x = r.axisu.Dot(m_steeringOccupancy.GetCenter() - r.center);
						float y = r.axisv.Dot(m_steeringOccupancy.GetCenter() - r.center);

						const float eps = 0.1f;
						if (x >= r.min.x - eps && x <= r.max.x + eps && y >= r.min.y - eps && y <= r.max.y + eps)
						{
//							GetAISystem()->AddDebugLine(m_steeringOccupancy.GetCenter()+Vec3(0,0,-10), m_steeringOccupancy.GetCenter()+Vec3(0,0,10), 0,0,0, 0);
							m_steeringOccupancy.AddObstructionDirection(r.center - m_steeringOccupancy.GetCenter());
						}
						else
						{
							unsigned last = maxpts-1;
							for (unsigned j = 0; j < maxpts; ++j)
							{
								m_steeringOccupancy.AddObstructionLine(rect[last], rect[j]);
								last = j;
							}
						}
					}
					else
					{
						const Vec3& pos = pActor->GetPhysicsPos();
						const float rad = pActor->GetMovementAbility().pathRadius;

						const Vec3& bodyDir = pActor->GetBodyDir();
						Vec3 right(bodyDir.y, -bodyDir.x, 0);
						right.Normalize();

						m_steeringOccupancy.AddObstructionCircle(pos, selfRad + rad);
						Vec3 vel = pActor->GetVelocity();
						if (vel.GetLengthSquared() > sqr(0.1f))
							m_steeringOccupancy.AddObstructionCircle(pos + vel*0.25f + right*rad*0.3f, selfRad + rad);
					}
				}
			}


			// Steer around
			Vec3 oldMoveDir = m_State.vMoveDir;
			m_State.vMoveDir = m_steeringOccupancy.GetNearestUnoccupiedDirection(m_State.vMoveDir, m_steeringOccupancyBias);

			if (gAIEnv.CVars.DebugDrawCrowdControl > 0)
			{
				Vec3 pos = GetPhysicsPos() + Vec3(0,0,0.75f);
				GetAISystem()->AddDebugLine(pos, pos + oldMoveDir * 2.0f, 196,0,0, 0);
				GetAISystem()->AddDebugLine(pos, pos + m_State.vMoveDir * 2.0f, 255,196,0, 0);
			}

			if (fabsf(m_steeringOccupancyBias) > 0.1f)
			{
				steering = true;

				Vec3 aheadPos;
				if (m_pPathFollower)
				{
					float junk;
					aheadPos = m_pPathFollower->GetPathPointAhead(m_movementAbility.pathRadius, junk);
					if(Distance::Point_Point2DSq(aheadPos, GetPhysicsPos()) < square(m_movementAbility.avoidanceRadius))
					{
						m_pPathFollower->Advance(m_movementAbility.pathRadius*0.3f);
					}
				}
				else
				{
					aheadPos = m_Path.CalculateTargetPos(myPos, 0.f, 0.f, m_movementAbility.pathRadius, true);
					if(!m_Path.Empty() && Distance::Point_Point2DSq(aheadPos, GetPhysicsPos()) < square(m_movementAbility.avoidanceRadius))
					{
						Vec3 pathNextPoint;
						if (m_Path.GetPosAlongPath(pathNextPoint, m_movementAbility.pathRadius*0.3f, true, false))
							m_Path.UpdatePathPosition(pathNextPoint, 100.0f, true, false);
					}
				}

				float slowdown = 1.0f - (oldMoveDir.Dot(m_State.vMoveDir) + 1.0f) * 0.5f;

				float normalSpeed, minSpeed, maxSpeed;
				GetMovementSpeedRange(m_State.fMovementUrgency, m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

				m_State.fDesiredSpeed += (minSpeed - m_State.fDesiredSpeed) * slowdown;

				if (slowdown > 0.1f)
					m_steeringAdjustTime += m_fTimePassed;
			}
			else
			{
				m_steeringAdjustTime -= m_fTimePassed;
			}

			Limit(m_steeringAdjustTime, 0.0f, 3.0f);
		}
		else
		{
			FRAME_PROFILER("NavigateAroundObjects Update Steering Old", gEnv->pSystem, PROFILE_AI)
			// Old type steering for the rest of the objects.
			unsigned nObj = m_steeringObjects.size();
			for (unsigned i = 0 ; i < nObj ; ++i)
			{
				const CAIObject *object = m_steeringObjects[i];
				if (NavigateAroundObjectsInternal(targetPos, myPos, in3D, object))
					steering = true;
			}
		}
	}

	m_steeringEnabled = steeringEnabled;

	return steering;
}

//===================================================================
// NavigateAroundAIObject
//===================================================================
bool CPuppet::NavigateAroundAIObject(const Vec3 &targetPos, const CAIObject *obstacle, const Vec3 &myPos, 
                                     const Vec3 &objectPos, bool steer, bool in3D)
{
  FUNCTION_PROFILER( GetISystem(),PROFILE_AI );
  if (steer)
  {
    if (in3D)
      return SteerAround3D(targetPos, obstacle, myPos, objectPos);
    else
      if (obstacle->GetType() == AIOBJECT_VEHICLE)
        return SteerAroundVehicle(targetPos, obstacle, myPos, objectPos);
      else
        return SteerAroundPuppet(targetPos, obstacle, myPos, objectPos);
  }
  else
  {
    AIError("NavigateAroundAIObject - only handles steering so far");
    return false;
  }
}

//===================================================================
// SteerAroundFlight
//===================================================================
bool CPuppet::SteerAroundFlight(const Vec3 &targetPos, const CAIObject *object, const Vec3 &myPos, const Vec3 &objectPos)
{
	const CAIActor* pActor = object->CastToCAIActor();
	const CPuppet* pPuppet = object->CastToCPuppet();
  if (!pActor)
    return false;

  if (pPuppet && pPuppet->m_IsSteering)
    return false;

	float avoidanceR = m_movementAbility.avoidanceRadius;
	avoidanceR += pActor->m_movementAbility.avoidanceRadius;
	float avoidanceRSq = square(avoidanceR);

  Vec3 myVel = GetVelocity();
  Vec3 otherVel = object->GetVelocity();

  if (myVel.GetLength() < 1.0f)
    return false;

  // if we are not steering already then only start if we would actually hit.
  if (!m_IsSteering)
  {
    Vec3 dir = (myVel - otherVel).GetNormalizedSafe();

    Vec3 i0, i1;
    static float checkDistScale = 3.0f;
    if (!Intersect::Lineseg_Sphere(Lineseg(myPos, myPos + dir * avoidanceR * checkDistScale), Sphere(objectPos, avoidanceR), i0, i1))
      return false;
  }

  float height, heightAbove;
  gAIEnv.pNavigation->GetFlightNavRegion()->GetHeightAndSpaceAt(myPos, height, heightAbove);
  float alt = myPos.z - height;

  static float avoidanceAlt = 20.0f;
  float criticalAltForDescent = avoidanceAlt;

  bool goUp = false;
  if (alt < criticalAltForDescent)
    goUp = true;
  else
    goUp = myPos.z > objectPos.z;

  m_flightSteeringZOffset = goUp ? avoidanceAlt : -avoidanceAlt;

  return true;
}


//===================================================================
// SteerAroundPuppet
//===================================================================
bool CPuppet::SteerAroundPuppet(const Vec3 &targetPos, const CAIObject *object, const Vec3 &myPos, const Vec3 &objectPos)
{
	const CPuppet* pPuppet = object->CastToCPuppet();
	if (!pPuppet)
		return false;

  float avoidanceR = m_movementAbility.avoidanceRadius;
  avoidanceR += pPuppet->m_movementAbility.avoidanceRadius;
  float avoidanceRSq = square(avoidanceR);

  float maxAllowedSpeedMod = 10.0f;
  Vec3 steerOffset(ZERO);

  bool outside = true;
  if (m_lastNavNodeIndex && (gAIEnv.pGraph->GetNodeManager().GetNode(m_lastNavNodeIndex)->navType & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_ROAD)) == 0)
    outside = false;

  Vec3 delta = objectPos - myPos;
  // skip if we're not close
  float distSq = delta.GetLengthSquared();
  if (distSq > avoidanceRSq)
    return false;

  // don't overtake unless we're in more of a hurry
  static float overtakeScale = 2.0f;
  static float overtakeOffset = 0.1f;
  static float criticalOvertakeMoveDot = 0.4f;

  float holdbackDist = 0.6f;
  if (object->GetType() == AIOBJECT_VEHICLE)
    holdbackDist = 15.0f;

  if (m_State.vMoveDir.Dot(pPuppet->GetState()->vMoveDir) > criticalOvertakeMoveDot)
  {
		float myNormalSpeed, myMinSpeed, myMaxSpeed;
		GetMovementSpeedRange(m_State.fMovementUrgency, m_State.allowStrafing, myNormalSpeed, myMinSpeed, myMaxSpeed);
//    float myNormalSpeed = GetNormalMovementSpeed(m_State.fMovementUrgency, true);
//    float otherNormalSpeed = pPuppet->GetNormalMovementSpeed(pPuppet->m_State.fMovementUrgency, true);
		float otherNormalSpeed, otherMinSpeed, otherMaxSpeed;
		pPuppet->GetMovementSpeedRange(pPuppet->m_State.fMovementUrgency, pPuppet->m_State.allowStrafing, otherNormalSpeed, otherMinSpeed, otherMaxSpeed);

		if (!outside || myNormalSpeed < overtakeScale * otherNormalSpeed + overtakeOffset)
    {
			float maxDesiredSpeed = max(myMinSpeed, object->GetVelocity().Dot(m_State.vMoveDir));

      float dist = sqrtf(distSq);
      dist -= holdbackDist;

      float extraFrac = dist / holdbackDist;
      Limit(extraFrac, -0.1f, 0.1f);
      maxDesiredSpeed *= 1.0f + extraFrac;

      if (m_State.fDesiredSpeed > maxDesiredSpeed)
        m_State.fDesiredSpeed = maxDesiredSpeed;

      return true;
    }
    avoidanceR *= 0.75f;
    avoidanceRSq = square(avoidanceR);
  }

  // steer around
  Vec3 aheadPos;
  if (m_pPathFollower)
  {
    float junk;
    aheadPos = m_pPathFollower->GetPathPointAhead(m_movementAbility.pathRadius, junk);
    if(outside && Distance::Point_Point2DSq(aheadPos, objectPos) < square(avoidanceR))
    {
      static float advanceFrac = 0.2f;
      m_pPathFollower->Advance(advanceFrac * avoidanceR);
    }
  }
  else
  {
    aheadPos = m_Path.CalculateTargetPos(myPos, 0.f, 0.f, m_movementAbility.pathRadius, true);
    if(outside && !m_Path.Empty() && Distance::Point_Point2DSq(aheadPos, objectPos) < square(avoidanceR))
    {
      static float advanceFrac = 0.2f;
      Vec3 pathNextPoint;
      if (m_Path.GetPosAlongPath(pathNextPoint, advanceFrac * avoidanceR, true, false))
        m_Path.UpdatePathPosition(pathNextPoint, 100.0f, true, false);
    }
  }

  Vec3	toTargetDir(objectPos - myPos);
  toTargetDir.z = 0.f;
  float toTargetDist = toTargetDir.NormalizeSafe();
  Vec3 forward2D(m_State.vMoveDir);
  forward2D.z = 0.f;
  forward2D.NormalizeSafe();
  Vec3	steerSideDir(forward2D.y, -forward2D.x, 0.f);
  // +ve when approaching, -ve when receeding
  float	toTargetDot = forward2D.Dot(toTargetDir);
  Limit(toTargetDot, 0.5f, 1.0f);

  // which way to turn
  float steerSign = steerSideDir.Dot(toTargetDir);
  steerSign = steerSign > 0.0f ? 1.0f : -1.0f;

  float toTargetDistScale = 1.0f - (toTargetDist / avoidanceR);
  Limit(toTargetDistScale, 0.0f, 1.0f);
  toTargetDistScale = sqrtf(toTargetDistScale);

  Vec3 thisSteerOffset = -toTargetDistScale * steerSign * steerSideDir * toTargetDot;
  if (!outside)
    thisSteerOffset *= 0.1f;

  steerOffset += thisSteerOffset;

	float normalSpeed, minSpeed, maxSpeed;
	GetMovementSpeedRange(m_State.fMovementUrgency, m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);
//  float normalSpeed = GetNormalMovementSpeed(m_State.fMovementUrgency, true);
  if (m_State.fDesiredSpeed > maxAllowedSpeedMod * normalSpeed)
    m_State.fDesiredSpeed = maxAllowedSpeedMod * normalSpeed;

  if (!steerOffset.IsZero())
  {
    m_State.vMoveDir += steerOffset;
    m_State.vMoveDir.NormalizeSafe();
  }
	return true;
}

//===================================================================
// SteerAround3D
// Danny TODO make this 3D!!!
//===================================================================
bool CPuppet::SteerAround3D(const Vec3 &targetPos, const CAIObject *object, const Vec3 &myPos, const Vec3 &objectPos)
{
  // flight is special
  if (!m_Path.Empty() && m_Path.GetNextPathPoint()->navType == IAISystem::NAV_FLIGHT)
    return SteerAroundFlight(targetPos, object, myPos, objectPos);

  const CAIActor* pActor = object->CastToCAIActor();
  float avoidanceR = m_movementAbility.avoidanceRadius;
  avoidanceR += pActor->m_movementAbility.avoidanceRadius;
  float avoidanceRSq = square(avoidanceR);

  Vec3 delta = objectPos - myPos;
  // skip if we're not close
  float distSq = delta.GetLengthSquared();
  if (distSq > square(avoidanceR))
    return false;

  Vec3 aheadPos = m_Path.CalculateTargetPos(myPos, 0.f, 0.f, m_movementAbility.pathRadius, true);
  if(Distance::Point_Point2DSq(aheadPos, objectPos) < square(avoidanceR))
	{
    static float advanceFrac = 0.2f;
		Vec3 pathNextPoint;
    if (m_Path.GetPosAlongPath(pathNextPoint, advanceFrac * avoidanceR, true, false))
  		m_Path.UpdatePathPosition(pathNextPoint, 100.0f, true, false);
	}
	Vec3	toTargetDir(objectPos - myPos);
	toTargetDir.z=0.f;
	float toTargetDist = toTargetDir.NormalizeSafe();
	Vec3	forward2D(m_State.vMoveDir);
	forward2D.z = 0.f;
	forward2D.NormalizeSafe();
	Vec3	steerDir(forward2D.y, -forward2D.x, 0.f);

  // +ve when approaching, -ve when receeding
	float	toTargetDot = forward2D.Dot(toTargetDir);
  // which way to turn
  float steerSign = steerDir.Dot(toTargetDir);
  steerSign = steerSign > 0.0f ? 1.0f : -1.0f;

  float toTargetDistScale = 1.0f - (toTargetDist / avoidanceR);
  Limit(toTargetDistScale, 0.0f, 1.0f);
  toTargetDistScale = sqrtf(toTargetDistScale);

  m_State.vMoveDir = m_State.vMoveDir - toTargetDistScale * steerSign * steerDir * toTargetDot;
	m_State.vMoveDir.NormalizeSafe();

  return true;
}

//===================================================================
// SteerAroundVehicles
//===================================================================
bool CPuppet::SteerAroundVehicle(const Vec3 &targetPos, const CAIObject *object, const Vec3 &myPos, const Vec3 &objectPos)
{
	// if vehicle is in the same formation (convoy) - don't steer around it, otherwise can't stay in formation
	if (const CAIVehicle *pVehicle = object->CastToCAIVehicle())
	{
		if(GetAISystem()->SameFormation(this, pVehicle))
			return false;
	}
  // currently puppet algorithm seems to work ok.
  return SteerAroundPuppet(targetPos, object, myPos, objectPos);
}

//===================================================================
// CreateFormation
//===================================================================
bool CPuppet::CreateFormation(const char * szName, Vec3 vTargetPos)
{
	// special handling for beacons :) It will create a formation point where the current target of the
	// puppet is.
	if (!stricmp(szName,"beacon"))
	{
		UpdateBeacon();
		return (m_pFormation!=NULL);
	}
	return CAIObject::CreateFormation(szName, vTargetPos);
}

//===================================================================
// Reset
//===================================================================
void CPuppet::Reset(EObjectResetType type)
{
	AILogComment("CPuppet::Reset %s (%p)", GetName(), this);

	CPipeUser::Reset(type);

	CAISystem *pAISystem = GetAISystem();

	m_steeringOccupancy.Reset(Vec3(0,0,0), Vec3(0,1,0), 1.0f);
	m_steeringOccupancyBias = 0;
	m_steeringAdjustTime = 0;

	m_mapDevaluedPoints.clear();

	m_steeringObjects.clear();

	ClearPotentialTargets();
	m_fLastUpdateTime = 0.0f;

	m_CloseContactTime = GetAISystem()->GetFrameStartTime();
	m_fLastNavTest = 0.0f;

	//Reset target movement tracking
	m_targetApproaching = false;
	m_targetFleeing = false;
	m_targetApproach = 0;
	m_targetFlee = 0;
	m_lastTargetValid = false;
	m_allowedStrafeDistanceStart = 0.0f;
	m_allowedStrafeDistanceEnd = 0.0f;
	m_allowStrafeLookWhileMoving = false;
	m_strafeStartDistance = 0;
	m_closeRangeStrafing = false;

	m_bGrenadeThrowRequested = false;
	m_eGrenadeThrowRequestType = eRGT_INVALID;
	m_iGrenadeThrowTargetType = 0;

	m_friendOnWayCounter = 0.0f;
	m_lastAimObstructionResult = true;
	m_updatePriority = AIPUP_LOW;

	m_adaptiveUrgencyMin = 0.0f;
	m_adaptiveUrgencyMax = 0.0f;
	m_adaptiveUrgencyScaleDownPathLen = 0.0f;
	m_adaptiveUrgencyMaxPathLen = 0.0f;

	m_delayedStance = 0;
	m_delayedStanceMovementCounter = 0;

	m_timeSinceTriggerPressed= 0.0f;
	m_friendOnWayElapsedTime = 0.0f;

	if(m_pFireCmdHandler)
		m_pFireCmdHandler->Reset();

	m_PFBlockers.clear();

	m_fTargetAwareness = 0.0f;

	m_fireTargetCache.Reset();

	m_CurrentHideObject.Set(0, ZERO, ZERO);
	m_InitialPath.clear();

	SetAvoidedVehicle(NILREF);
	//make sure i'm added to groupsMap; no problem if there already. 
	pAISystem->AddToGroup(this, GetGroupId());

	// Initially allowed to hit target if not using ambient fire system
	const bool bAmbientFireEnabled = (gAIEnv.CVars.AmbientFireEnable != 0);
	m_allowedToHitTarget = !bAmbientFireEnabled;

	m_allowedToUseExpensiveAccessory = false;
	m_firingReactionTimePassed = false;
	m_firingReactionTime = 0.0f;
	m_outOfAmmoTimeOut = 0.0f;

	m_currentNavSOStates.Clear();
	m_pendingNavSOStates.Clear();

	m_targetSilhouette.Reset();
	m_targetLastMissPoint.Set(0,0,0);
	m_targetFocus = 0.0f;
	m_targetZone = AIZONE_OUT;
	m_targetPosOnSilhouettePlane.Set(0,0,0);
	m_targetDistanceToSilhouette = FLT_MAX;
	m_targetBiasDirection.Set(0,0,-1);
	m_targetEscapeLastMiss = 0.0f;
	m_targetSeenTime = 0;
	m_targetLostTime = 0;
	m_targetLastMissPointSelectionTime.SetSeconds(0.0f);
	m_weaponSpinupTime = 0;

	m_alarmedTime = 0.0f;
	m_alarmedLevel = 0.0f;

	m_targetDamageHealthThr = -1.0f;
	m_burstEffectTime = 0.0f;
	m_burstEffectState = 0;
	m_targetDazzlingTime = 0.0f;

	if(m_targetDamageHealthThrHistory) m_targetDamageHealthThrHistory->Reset();
	m_targetHitPartIndex.clear();

	m_vForcedNavigation = ZERO;
	m_adjustpath  = 0;
	m_SeeTargetFrom = ST_HEAD;

	m_lastPuppetVisCheck = 0;
	m_LastShotsCount = 0;

	m_lastBodyDir = ZERO;
	m_bodyTurningSpeed = 0;

	m_damagePartsUpdated = false;

	// Clear the W/T names we use for lookup
	m_territoryShape = 0;
	m_territoryShapeName.clear();

	// Default perception descriptors
	m_SoundPerceptionDescriptor[AISOUND_GENERIC].Set(0.0f, 1.0f, 2.0f, PERCEPTION_INTERESTING_THR, 1.0f, 0.25f);
	m_SoundPerceptionDescriptor[AISOUND_COLLISION].Set(0.0f, 1.0f, 4.0f, PERCEPTION_INTERESTING_THR, 1.0f, 0.5f);
	m_SoundPerceptionDescriptor[AISOUND_COLLISION_LOUD].Set(0.0f, 1.0f, 4.0f, PERCEPTION_THREATENING_THR, 1.0f, 0.5f);
	m_SoundPerceptionDescriptor[AISOUND_MOVEMENT].Set(0.0f, 1.0f, 2.0f, PERCEPTION_INTERESTING_THR, 1.0f, 0.5f);
	m_SoundPerceptionDescriptor[AISOUND_MOVEMENT_LOUD].Set(0.0f, 1.0f, 4.0f, PERCEPTION_THREATENING_THR, 1.0f, 0.5f);
	m_SoundPerceptionDescriptor[AISOUND_WEAPON].Set(0.0f, 1.0f, 4.0f, PERCEPTION_AGGRESSIVE_THR, 1.0f, 0.25f);
	m_SoundPerceptionDescriptor[AISOUND_EXPLOSION].Set(0.0f, 1.0f, 6.0f, PERCEPTION_AGGRESSIVE_THR, 1.0f, 0.75f);


	m_fireDisabled = 0;

	ResetAlertness();
	GetAISystem()->NotifyEnableState(this, m_bEnabled);

	if (type == AIOBJRESET_SHUTDOWN)
		UnregisterVision();
}

//===================================================================
// SDynamicHidePositionNotInNavType
//===================================================================
struct SDynamicHidePositionNotInNavType
{
  SDynamicHidePositionNotInNavType(IAISystem::tNavCapMask mask) : mask(mask) {}
  bool operator()(const SHideSpot &hs)
  {
    if (hs.info.type != SHideSpotInfo::eHST_DYNAMIC)
      return false;
    int nBuildingID; IVisArea *pVis;
    IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(hs.info.pos, nBuildingID, pVis, mask);
    return (navType & mask) == 0;
  }
  IAISystem::tNavCapMask mask;
};

//===================================================================
// IsSmartObjectHideSpot
//===================================================================
inline bool IsSmartObjectHideSpot(const std::pair<float, SHideSpot> &hsPair)
{
	return hsPair.second.info.type == SHideSpotInfo::eHST_SMARTOBJECT;
}

//===================================================================
// HasPointInRange
//===================================================================
static bool	HasPointInRange(const std::vector<Vec3>& list, const Vec3& pos, float range)
{
	float	r(sqr(range));
	for (unsigned i = 0, ni = list.size(); i < ni; ++i)
		if(Distance::Point_PointSq(list[i], pos) < r)
			return true;
	return false;
}

//===================================================================
// Check if path to new spot will cross the target fire dir
//===================================================================
/*static bool	IsRightOf(const Vec3& Pos, const Vec3& Dir,const Vec3& Target)
{
  Vec3 vDiff = Target - Pos;
  float fDotLeft = (vDiff.x * -Dir.y) + (vDiff.y * Dir.x);

  return (fDotLeft > 0.0f);
}

//===================================================================
// Check if path to new spot will cross the target fire dir
//===================================================================
static bool	IsBehindOf(const Vec3& Pos, const Vec3& Dir,const Vec3& Target)
{
  Vec3 vDiff = Target - Pos;
  float fDotFront = (vDiff.x * Dir.x) + (vDiff.y * Dir.y);

  return (fDotFront < 0.0f);
}
*/
//===================================================================
// Check if path to new spot will cross the target fire dir
//===================================================================
/*static bool	IsCrossingTargetDir(const Vec3& OriginalPos, const Vec3& PretendedNewPos, const Vec3& TargetPos, const Vec3& TargetDir)
{
  bool bRet = false;

  Vec3 DirToOriginalPos(TargetPos - OriginalPos);
  Vec3 DirToPretendedPos(TargetPos - PretendedNewPos);

  DirToOriginalPos.z = 0;
  DirToOriginalPos.NormalizeSafe();

  DirToPretendedPos.z = 0;
  DirToPretendedPos.NormalizeSafe();

  //GetAISystem()->AddPerceptionDebugLine("tgt", TargetPos, (TargetPos + (TargetDir * 20.0f)), 0, 0, 255, 5.f, 5.f);

  if(IsBehindOf(TargetPos, TargetDir, OriginalPos) == false || IsBehindOf(TargetPos, TargetDir, PretendedNewPos) == false)
  {
    if( IsRightOf(TargetPos, TargetDir, OriginalPos) != IsRightOf(TargetPos, TargetDir, PretendedNewPos) ) 
    {
      bRet = true;
    }
  }
  
  return (bRet);
}
*/
//===================================================================
// Check if path to new spot will cross the target fire dir
//===================================================================
/*static bool CheckVisibilityBetweenTwoPoints(const Vec3& Pos1, const Vec3& Pos2)
{
  bool bRet = false;
  ray_hit hit;

  bRet = 0 == gEnv->pPhysicalWorld->RayWorldIntersection(Pos2, (Pos1 - Pos2), COVER_OBJECT_TYPES, HIT_COVER | HIT_SOFT_COVER, &hit, 1);

  if( gAIEnv.CVars.DrawHideSpotSearchRays == 1 )
  {
    if(bRet == true)
      GetAISystem()->AddDebugLine(Pos1, Pos2, 0, 255, 0, 20.f);
    else
      GetAISystem()->AddDebugLine(Pos1, Pos2, 255, 0, 0, 20.f);
  }

  return (bRet);
}
*/
//===================================================================
// Check if path to new spot will cross the target fire dir
//===================================================================
/*
static bool	CanShootTargetFromThere(const Vec3& PretendedNewPos, const Vec3& TargetPos)
{
  bool bRet = false;
  Vec3 vPos = PretendedNewPos;
  vPos.z += 1.f;

  float fLeanDist = 0.7f;
  Vec3 vDir = TargetPos - PretendedNewPos;
  vDir.Normalize();

  Matrix33 orientation = Matrix33::CreateRotationVDir( vDir, 0 );
  Vec3 vRight = orientation.TransformVector(Vec3(1,0,0));

  bRet = CheckVisibilityBetweenTwoPoints(vPos + (vRight * fLeanDist), TargetPos + (vRight * fLeanDist));

  if(bRet == false)
  {
    bRet =CheckVisibilityBetweenTwoPoints(vPos + (-vRight * fLeanDist), TargetPos + (-vRight * fLeanDist));
  }

  return (bRet);
}
*/
//===================================================================
// GetHidePoint
//===================================================================
Vec3 CPuppet::FindHidePoint(float searchDistance, const Vec3 &hideFrom, int nMethod,
														IAISystem::ENavigationType navType, bool bSameOk, float minDist,
														SDebugHideSpotQuery* pDebug)
{
	CCCPOINT(CPuppet_FindHidePoint);
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	if (pDebug)
	{
		pDebug->found = false;
	}

	// reset hiding behind smart objects
	m_CurrentHideObject.ClearSmartObject();

	if (nMethod & HM_USE_LASTOP_RADIUS)
	{
		CAIObject *pLastOpResult = m_refLastOpResult.GetAIObject();
		if (pLastOpResult && pLastOpResult->GetRadius() > 0.0f)
			searchDistance = pLastOpResult->GetRadius();
		nMethod &= ~HM_USE_LASTOP_RADIUS;
	}

	// first get all hide spots within the search distance
	static MultimapRangeHideSpots hidespots;
	static MapConstNodesDistance traversedNodes;
	hidespots.clear();
	traversedNodes.clear();

	bool skipNavigationTest = 0 != (m_movementAbility.pathfindingProperties.navCapMask & IAISystem::NAV_VOLUME);
	if ((nMethod & HM_AROUND_LASTOP) == 0)
	{
		GetAISystem()->GetHideSpotsInRange(hidespots, traversedNodes, GetPos(), searchDistance, 
			m_movementAbility.pathfindingProperties.navCapMask, GetParameters().m_fPassRadius, skipNavigationTest,
			GetEntity(), m_lastNavNodeIndex, this);
	}
	else
	{
		CAIObject *pLastOpResult = m_refLastOpResult.GetAIObject();
		if (pLastOpResult)
		{
			GetAISystem()->GetHideSpotsInRange(hidespots, traversedNodes, pLastOpResult->GetPos(), searchDistance,
				m_movementAbility.pathfindingProperties.navCapMask, GetParameters().m_fPassRadius, skipNavigationTest,
				GetEntity(), m_lastNavNodeIndex, this);
		}
		else
		{
			// no hiding points - NO LAST OP RESULT
			// generate signal that there is no Hiding place 
			AISignalExtraData* pData = new AISignalExtraData;
			pData->fValue = searchDistance;
			SetSignal(1, "OnNoHidingPlace", GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnNoHidingPlace);
			m_CurrentHideObject.Invalidate();
			return GetPos();
		}
		if(nMethod == HM_RANDOM_AROUND_LASTOPRESULT) 
			nMethod = HM_RANDOM;
	}


	float minDistanceSqr = sqr(minDist);
	float searchDistanceSqr = sqr(searchDistance);

	bool	bIncludeSoftCovers = (nMethod & HM_INCLUDE_SOFTCOVERS) != 0;
	bool	bIgnoreEnemies = (nMethod & HM_IGNORE_ENEMIES) != 0;
	bool	bBackwards	= (nMethod & HM_BACK) != 0;
	bool	bLastOp	= (nMethod & HM_AROUND_LASTOP) != 0;
	bool	bHideBehind	= (nMethod & HM_ON_SIDE) == 0;
	nMethod &= ~(HM_INCLUDE_SOFTCOVERS | HM_IGNORE_ENEMIES | HM_BACK | HM_AROUND_LASTOP | HM_USE_LASTOP_RADIUS | HM_ON_SIDE);

	// Collect the hide points into a list and sort them by goodness.
	// Then validate the list from the best to worst.
	static std::vector<SSortedHideSpot> sortedHideSpots;
	sortedHideSpots.clear();

	IAISystem::tNavCapMask navCapMask = GetMovementAbility().pathfindingProperties.navCapMask;
	const float passRadius = GetParameters().m_fPassRadius;

	CAIObject *pLastOpResult = m_refLastOpResult.GetAIObject();
	Vec3 searchPos(bLastOp && pLastOpResult ? pLastOpResult->GetPos() : GetPos());
	const Vec3& refPointPos = GetRefPoint()->GetPos();
	Vec3 dirToRefPoint = refPointPos - searchPos;
	float distToRefPoint = dirToRefPoint.NormalizeSafe();
	Vec3 normToRefPoint(dirToRefPoint.y, -dirToRefPoint.x, 0);
	normToRefPoint.Normalize();

	Vec3	dirToTarget(hideFrom - searchPos);
	dirToTarget.z = 0;
	dirToTarget.NormalizeSafe();

	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();

	// Colled active enemies to avoid.
	std::vector<CAIActor*> enemies;
	if (!bIgnoreEnemies)
	{
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CPuppet* pPuppet = it->GetAIObject();
			if (pPuppet && IsHostile(pPuppet))
				enemies.push_back(pPuppet);
		}
	}

	const MultimapRangeHideSpots::iterator liEnd = hidespots.end();
	for (MultimapRangeHideSpots::iterator li = hidespots.begin(); li != liEnd; ++li)
	{
		SHideSpot &HS = li->second;
		if (bIncludeSoftCovers && HS.info.type == SHideSpotInfo::eHST_SMARTOBJECT)
			continue;

		if (!bIncludeSoftCovers)
		{
			if(HS.IsSecondary())
				continue;
		}

		float dot = 1.0f;
		if (!HS.info.dir.IsZero() )
		{
			// check that the enemy is vaguely in the dir of the hide anchor
			Vec3 dirHideToEnemy = hideFrom - HS.info.pos;
			dirHideToEnemy.NormalizeSafe();
			dot = dirHideToEnemy.Dot( HS.info.dir );
			if (dot < HIDESPOT_COVERAGE_ANGLE_COS)
				continue;
		}

		float dist = li->first;
		float distSqr = sqr(dist);

		if (HS.pObstacle)
		{
			GetAISystem()->AdjustOmniDirectionalCoverPosition(HS.info.pos, HS.info.dir, max(HS.pObstacle->fApproxRadius, 0.0f), 
				passRadius, hideFrom, bHideBehind);
		}
		// These are designer set secondary hidespots. Handle them as omni-directional points.
		if (HS.pAnchorObject && HS.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY)
		{
			GetAISystem()->AdjustOmniDirectionalCoverPosition(HS.info.pos, HS.info.dir, 0.1f, 0.0f, hideFrom);
		}
		if (HS.pNavNode && HS.pNavNode->navType == IAISystem::NAV_WAYPOINT_HUMAN &&
			HS.pNavNode->GetWaypointNavData()->type == WNT_HIDESECONDARY)
		{
			GetAISystem()->AdjustOmniDirectionalCoverPosition(HS.info.pos, HS.info.dir, 0.1f, 0.0f, hideFrom);
		}

		if (minDist > 0)
		{
			// exclude obstacles too close 
			if (nMethod == HM_NEAREST_BACKWARDS || nMethod == HM_NEAREST_TOWARDS_REFPOINT) //to self
			{
				if (distSqr < minDistanceSqr)
					continue;
			}
			else					// to target
			{
				if (Distance::Point_PointSq(HS.info.pos, hideFrom) < minDistanceSqr)
					continue;
			}
		}

		if (bBackwards)
		{
			if ((hideFrom - searchPos).GetNormalizedSafe().Dot((HS.info.pos - searchPos).GetNormalizedSafe()) > 0.2f) 
				continue;
		}

		bool indoor = HS.pNavNode && (HS.pNavNode->navType == IAISystem::NAV_WAYPOINT_HUMAN || HS.pNavNode->navType == IAISystem::NAV_WAYPOINT_3DSURFACE);

		// check if there are enemies around
		float avoidEnemiesCoeff = 1.0f;
		if (!bIgnoreEnemies)
		{
			for (unsigned i = 0, ni = enemies.size(); i < ni; ++i)
			{
				const CAIActor* pFoeActor = enemies[i];
				float safeDist = indoor ? pFoeActor->GetParameters().m_fDistanceToHideFrom/2.0f : pFoeActor->GetParameters().m_fDistanceToHideFrom;
				float distFoeSqr = Distance::Point_PointSq(HS.info.pos, pFoeActor->GetPos());
				if (distFoeSqr < sqr(safeDist))
					avoidEnemiesCoeff *= sqrtf(distFoeSqr) / safeDist;
			}
		}

		// Compromising.
		if (!indoor && nMethod != HM_RANDOM)
		{
			// allow him to use only the hidespots closer to him than to the enemy
			Vec3 dirHideToEnemy = hideFrom - HS.info.pos;
			Vec3 dirHide = HS.info.pos - searchPos;
			if (dirHide.GetLengthSquared() > dirHideToEnemy.GetLengthSquared())
				continue;
		}

		// Calculate the weight (goodness) of the cover point.
		float weight = 0;

		switch (nMethod)
		{
		case HM_RANDOM:
			{
				weight = ai_frand() + avoidEnemiesCoeff;
			}
			break;
		case HM_NEAREST:
		case HM_NEAREST_TO_ME:
			{
				float directDist = Distance::Point_Point(HS.info.pos, nMethod==HM_NEAREST? searchPos: GetPos());
				weight = (searchDistance - directDist) * avoidEnemiesCoeff * ((3+dot)/4);
			}
			break;

		case HM_NEAREST_TOWARDS_REFPOINT:
			{
				const float rad = min(searchDistance/2, distToRefPoint);
				Vec3 diff = HS.info.pos - (searchPos + dirToRefPoint * rad);
				float u = normToRefPoint.Dot(diff) * 2.0f;
				float v = dirToRefPoint.Dot(diff);
				float dd = sqr(u) + sqr(v);
				if (dd > sqr(rad))
					continue;
				float val = sqr(distToRefPoint) - dd;
				val *= avoidEnemiesCoeff;
				weight = val;
			}
			break;

		case HM_NEAREST_TOWARDS_TARGET_PREFER_SIDES:
		case HM_NEAREST_TOWARDS_TARGET_LEFT_PREFER_SIDES:
		case HM_NEAREST_TOWARDS_TARGET_RIGHT_PREFER_SIDES:
			{
				Vec3	ax(-dirToTarget.y, dirToTarget.x, 0);
				Vec3	ay(dirToTarget);
				Vec3	diff(HS.info.pos - searchPos);
				float	x = ax.Dot(diff) * 0.5f;	// prefer sides
				float	y = ay.Dot(diff);
				// Skip points which are behind the agent.
				if(y < -1.0f)
					continue;
				// Skip points at right side.
				if(nMethod == HM_NEAREST_TOWARDS_TARGET_LEFT_PREFER_SIDES && x > 1.0f)
					continue;
				// Skip points at left side.
				if(nMethod == HM_NEAREST_TOWARDS_TARGET_RIGHT_PREFER_SIDES && x < -1.0f)
					continue;
				weight = (searchDistance - sqrtf(x*x + y*y)) * avoidEnemiesCoeff;
			}
			break;

		case HM_NEAREST_PREFER_SIDES:
			{
				Vec3	ax(-dirToTarget.y, dirToTarget.x, 0);
				Vec3	ay(dirToTarget);
				Vec3	diff(HS.info.pos - searchPos);
				float	x = ax.Dot(diff) * 0.5f;	// prefer sides
				float	y = ay.Dot(diff);
				weight = (searchDistance - sqrtf(x*x + y*y)) * avoidEnemiesCoeff * ((3+dot)/4);
			}
			break;

		case HM_NEAREST_TO_TARGET:
		case HM_NEAREST_TOWARDS_TARGET:
			{
				Vec3 one = hideFrom-searchPos;
				Vec3	two = HS.info.pos-searchPos;
				float distanceObstacle_hideFrom = (HS.info.pos-hideFrom).GetLength();
				weight = (20000 - distanceObstacle_hideFrom) * avoidEnemiesCoeff;
				if (one.Dot(two) <= 0 || (nMethod==HM_NEAREST_TO_TARGET && distanceObstacle_hideFrom > one.GetLength()))
					continue;
			}
			break;
		case HM_FARTHEST_FROM_TARGET:
			{
				Vec3 one = hideFrom-searchPos;
				Vec3	two = HS.info.pos-searchPos;
				if (one.Dot(two)>0)
					continue;
				weight = ((HS.info.pos-hideFrom).len2()) * avoidEnemiesCoeff * ((3+dot)/4);
			}
			break;
		case HM_NEAREST_BACKWARDS:
			{
				Vec3 one = hideFrom-searchPos;
				Vec3	two = HS.info.pos-searchPos;
				weight = (2000.f - dist) * avoidEnemiesCoeff;
				if( !m_movementAbility.b3DMove )
					one.z = two.z = .0f;
				one.normalize();
				two.normalize();
				if (one.Dot(two)>-0.7f)
					continue;
			}
			break;
		case HM_LEFTMOST_FROM_TARGET:
			{
				Vec3 one = hideFrom-searchPos;
				Vec3	two = HS.info.pos-searchPos;
				float zcross = one.x*two.y - one.y*two.x;
				zcross = (2000 + zcross) * avoidEnemiesCoeff * ((3+dot)/4);
				weight = zcross;
			}
			break;
		case HM_FRONTLEFTMOST_FROM_TARGET:
			{
				Vec3 one = hideFrom - searchPos;
				Vec3	two = HS.info.pos - searchPos;
				float zcross = one.x*two.y - one.y*two.x;
				one.Normalize();
				two.Normalize();
				float f = one.Dot(two);
				if (f<0.2)
					continue;
				weight = (2000 + zcross) * avoidEnemiesCoeff * ((3+dot)/4);
			}
			break;
		case HM_RIGHTMOST_FROM_TARGET:
			{
				Vec3 one = hideFrom-searchPos;
				Vec3	two = HS.info.pos-searchPos;
				float zcross = one.x*two.y - one.y*two.x;
				weight = (2000 - zcross) * avoidEnemiesCoeff * ((3+dot)/4);
			}
			break;
		case HM_FRONTRIGHTMOST_FROM_TARGET:
			{
				Vec3 one = hideFrom - searchPos;
				Vec3	two = HS.info.pos - searchPos;
				float zcross = one.x*two.y - one.y*two.x;
				one.Normalize();
				two.Normalize();
				float f = one.Dot(two);
				if (f<0.2)
					continue;
				weight = (2000 - zcross) * avoidEnemiesCoeff * ((3+dot)/4);
			}
			break;
			// add more here as needed
		}

		sortedHideSpots.push_back(SSortedHideSpot(weight, &HS));
	}

	// Sort the hidespots and start validating them starting from the best match.
	std::sort(sortedHideSpots.begin(), sortedHideSpots.end());

	if (pDebug)
	{
		for (unsigned i = 0, ni = sortedHideSpots.size(); i < ni; ++i)
		{
			SSortedHideSpot& spot = sortedHideSpots[i];
			SHideSpot &HS = *spot.pHideSpot;
			pDebug->spots.push_back(SDebugHideSpotQuery::SHideSpot(HS.info.pos, HS.info.dir, spot.weight));
		}
	}

	bool bCheckForSameObject(!bSameOk &&
		nMethod != HM_NEAREST &&
		nMethod != HM_NEAREST_PREFER_SIDES &&
		nMethod != HM_RANDOM);
	if (nMethod == HM_NEAREST_TOWARDS_REFPOINT)
		bCheckForSameObject = true;

	Vec3 playerPos(ZERO);
	bool bPlayerHostile = false;
	CAIObject* pPlayer = GetAISystem()->GetPlayer();
	if(pPlayer)
	{
		playerPos = (pPlayer->GetEntity()? pPlayer->GetEntity()->GetPos(): pPlayer->GetPos());
		bPlayerHostile = IsHostile(pPlayer);
	}

	// Setup territory check.
	// Territory describes an area where the hidespots should be limited to.
	SShape*	territoryShape = GetTerritoryShape();

	const CPathObstacles &pathAdjustmentObstacles = GetPathAdjustmentObstacles();

	// Check if dead bodies should be avoided.
	const unsigned maxDeadBodySpots = 3;
	Vec3	deadBodySpots[maxDeadBodySpots];
	unsigned	nDeadBodySpots = 0;
	float			deadBodyBlockerDistSq = 0.0f;
	// Check if the dead body navigation blocker is set to disable navigation around dead bodies.
	const int ignoreDeadBodies = gAIEnv.CVars.IgnoreDeadBodies;
	if (!ignoreDeadBodies)
	{
	TMapBlockers::const_iterator itr(m_PFBlockers.find(PFB_DEAD_BODIES));
	if(itr != m_PFBlockers.end() && (*itr).second < -0.01f)
	{
		nDeadBodySpots = GetAISystem()->GetDangerSpots(static_cast<const IAIObject*>(this), 40.0f, deadBodySpots, 0, maxDeadBodySpots, IAISystem::DANGER_DEADBODY);
		deadBodyBlockerDistSq = sqr(-(*itr).second);
	}
	}

	// Collect list of occupied hide object positions.
	std::vector<Vec3>	occupiedSpots;
	GetAISystem()->GetOccupiedHideObjectPositions(this, occupiedSpots);

	AIAssert(GetProxy());
	SAIBodyInfo bi;
	GetProxy()->QueryBodyInfo(SAIBodyInfoQuery(STANCE_STAND, 0.0f, 0.0f, true), bi);

	for (unsigned i = 0, ni = sortedHideSpots.size(); i < ni; ++i)
	{
		SHideSpot &HS = *sortedHideSpots[i].pHideSpot;

		// check distance of player to the obstacle
		if (pPlayer)
		{
			IAISystem::ENavigationType obstacleNavType;
			bool b2D = !HS.pNavNode || (obstacleNavType= HS.pNavNode->navType) != IAISystem::NAV_WAYPOINT_3DSURFACE && 
				obstacleNavType != IAISystem::NAV_VOLUME && obstacleNavType != IAISystem::NAV_FLIGHT; 
			float distPlayerSqr =	b2D?	Distance::Point_Point2DSq(HS.info.pos,playerPos): Distance::Point_PointSq(HS.info.pos,playerPos);
			float thr(0);
			if (HS.pNavNode && HS.pNavNode->navType == IAISystem::NAV_WAYPOINT_HUMAN || !HS.pObstacle )
				thr = 1.2f;
			else 
				thr =  HS.pObstacle->fApproxRadius+1.5f;
			if (distPlayerSqr < sqr(thr))//check if player is close to the obstacle
			{
				//for player's friends, check if player is behind the obstacle, 
				// and discard it unless the puppet is searching right around the player
				if(bPlayerHostile) 
					continue;
				else if (!(bLastOp && m_refLastOpResult.GetAIObject() == pPlayer) && (hideFrom - HS.info.pos).Dot(playerPos - HS.info.pos) <0)
					continue;
			}
		}

		// If if the same hide object should not be used.
		if (bCheckForSameObject && IsEquivalent(m_CurrentHideObject.GetObjectPos(), HS.info.pos))
			continue;

		// Skip points outside territory.
		if (territoryShape && !territoryShape->IsPointInsideShape(HS.info.pos, true))
			continue;

		// Skip used hide objects.
		if (HasPointInRange(occupiedSpots, HS.info.pos, 0.5f))
			continue;

		// Skip recent unreachable hidespots.
		if (WasHideObjectRecentlyUnreachable(HS.info.pos))
			continue;

		// Skip hidespots next to dead bodies, if that is to be avoided.
		if (nDeadBodySpots)
		{
			bool	closeToDeadBody(false);
			for (unsigned j = 0; j < nDeadBodySpots; ++j)
			{
				if (Distance::Point_PointSq(deadBodySpots[j], HS.info.pos) < deadBodyBlockerDistSq)
				{
					closeToDeadBody = true;
					break;
				}
			}
			if (closeToDeadBody)
				continue;
		}

		// skip points close to explosion craters
		if (GetAISystem()->GetPerceptionManager()->IsPointInRadiusOfStimulus(AISTIM_EXPLOSION, HS.info.pos))
			continue;

		if (HS.pObstacle)
		{
			// Discard the point if it is inside another vegetation obstacle.
			bool	insideObstacle(false);
			for (MultimapRangeHideSpots::iterator li = hidespots.begin(); li != hidespots.end(); ++li)
			{
				const ObstacleData* pOtherObstacle = li->second.pObstacle;
				if (pOtherObstacle == HS.pObstacle)
					continue;
				if (!pOtherObstacle || !pOtherObstacle->IsCollidable())
					continue;
				if(Distance::Point_Point2DSq(HS.info.pos, pOtherObstacle->vPos) < sqr(max(pOtherObstacle->fApproxRadius, 0.0f)))
				{
					insideObstacle = true;
					break;
				}
			}
			if(insideObstacle)
				continue;
		}

		if (HS.pNavNode && HS.pNavNode->navType == IAISystem::NAV_TRIANGULAR)
		{
			if (gAIEnv.pNavigation->IsPointForbidden(HS.info.pos, passRadius, 0, 0))
				continue;
			if (pathAdjustmentObstacles.IsPointInsideObstacles(HS.info.pos))
				continue;
		}

		// The spot is valid, use it!
		// Move the spot away from the wall.
		if (HS.info.type == SHideSpotInfo::eHST_ANCHOR || HS.info.type == SHideSpotInfo::eHST_WAYPOINT)
		{
			if ((HS.pAnchorObject && HS.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT) ||
				(HS.pNavNode && HS.pNavNode->navType == IAISystem::NAV_WAYPOINT_HUMAN &&
				HS.pNavNode->GetWaypointNavData()->type == WNT_HIDE))
			{
				GetAISystem()->AdjustDirectionalCoverPosition(HS.info.pos, HS.info.dir, passRadius, 0.75f);
			}
		}

		// Do final physics validation for dynamic hide points.
		if (HS.info.type == SHideSpotInfo::eHST_DYNAMIC)
		{
			if (!GetAISystem()->GetDynHideObjectManager()->ValidateHideSpotLocation(HS.info.pos, bi, HS.entityId))
				continue;
		}

		m_CurrentHideObject.Set(&HS, HS.info.pos, HS.info.dir);

		//	GetAISystem()->AddDebugSphere(retPoint+Vec3(0,0,1), 0.5f,  255,0,0, 8.0f);

		if (HS.info.type == SHideSpotInfo::eHST_SMARTOBJECT)
			m_CurrentHideObject.SetSmartObject( HS.SOQueryEvent );

		if (pDebug)
		{
			pDebug->hidePos = HS.info.pos;
			pDebug->hideDir = HS.info.dir;
			pDebug->found = true;
		}

		return HS.info.pos;	
	}

	// no hiding points - not with your specified filter or range
	// generate signal that there is no Hiding place 
	AISignalExtraData* pData = new AISignalExtraData;
	pData->fValue = searchDistance;
	SetSignal(1,"OnNoHidingPlace",GetEntity(),pData, gAIEnv.SignalCRCs.m_nOnNoHidingPlace);

	// Invalidate the current hidepoint if not close to it.
	//if(bCheckForSameObject || !m_CurrentHideObject.IsNearCover(this))
	m_CurrentHideObject.Invalidate();

	return GetPos();
}

//====================================================================
// GetAccuracy
//	calculate current accuracy - account for distance, target stance, ...
//	at close range always hit, at attackRange always miss
//	if accuracy in properties is 1 - ALWAYS hit
//	if accuracy in properties is 0 - ALWAYS miss
//====================================================================
float CPuppet::GetAccuracy(const CAIObject *pTarget) const 
{

	//---------parameters------------------------------------------
	const float	absoluteAccurateTrh(5.f);	// if closer than this - always hit
//	const float	accurateTrhSlope(.3f);		// slop from aways-hit to nominal accuracy
	const float	nominalAccuracyStartAt(.3f);		// at what attack range nominal accuracy is on
	const float	nominalAccuracyStopAt(.73f);		// at what attack range nominal accuracy is starting to fade to 0
	//-------------------------------------------------------------

	float	unscaleAccuracy = GetParameters().m_fAccuracy;
	float	curAccuracy = unscaleAccuracy;
	if(curAccuracy <= 0.00001f)
		return 0.f;

	if(!IsAllowedToHitTarget())
		curAccuracy *= 0.0f;

	if(!pTarget)
		return curAccuracy;

	const CAIActor* pTargetActor = pTarget->CastToCAIActor();

	curAccuracy = max(0.f, curAccuracy - m_fAccuracySupressor);	// account for soft cover
	//	curAccuracy -= m_fAccuracyMotionAddition;	// account for movement
	float	distance((pTarget->GetPos()-GetPos()).len());

	if(distance<absoluteAccurateTrh)	// too close - always hit
		return 1.f;
	if(distance>=GetParameters().m_fAttackRange)	// too far - always miss
		return 0.f;
	// at what DISTANCE nominal accuracy is on
	float	nominalAccuracyStartDistance(max(absoluteAccurateTrh+1.f, GetParameters().m_fAttackRange*nominalAccuracyStartAt));
	// at what DISTANCE nominal accuracy is starting to fade to 0
	float	nominalAccuracyStopDistance(min(GetParameters().m_fAttackRange-.1f, GetParameters().m_fAttackRange*nominalAccuracyStopAt));

	// scale down accuracy if target prones 
	if(pTargetActor && pTargetActor->GetProxy())
	{
		SAIBodyInfo	targetBodyInfo;
		pTarget->GetProxy()->QueryBodyInfo(targetBodyInfo);
		if(targetBodyInfo.stance == STANCE_STAND)
			curAccuracy *= 1.0f;
		else if(targetBodyInfo.stance == STANCE_CROUCH)
			curAccuracy *= 0.8f;
		else if(targetBodyInfo.stance == STANCE_LOW_COVER)
			curAccuracy *= 0.65f;
		else if(targetBodyInfo.stance == STANCE_HIGH_COVER)
			curAccuracy *= 0.8f;
		else if(targetBodyInfo.stance == STANCE_PRONE)
			curAccuracy *= 0.3f;
	}

	// scale down accuracy if shooter moves
	if(GetPhysics())
	{
		static pe_status_dynamics  dSt;
		GetPhysics()->GetStatus( &dSt );
		float fSpeed= dSt.v.GetLength();
		if(fSpeed > 1.0f)
		{
			if(fSpeed > 5.0f)
				fSpeed = 5.0f;
			if(IsCoverFireEnabled())
				fSpeed /=2.f;
			curAccuracy *= 3.f/(3.f +fSpeed);
		}
	}
	
	if(distance<nominalAccuracyStartDistance)	// 1->nominal interpolation
	{
		float slop((1.f-curAccuracy)/(nominalAccuracyStartDistance-absoluteAccurateTrh));
		float scaledAccuracy(curAccuracy + slop*(nominalAccuracyStartDistance - (distance-absoluteAccurateTrh)));
		return scaledAccuracy;
	}
	if(distance>nominalAccuracyStopDistance)	// nominal->0 interpolation
	{
		float slop((curAccuracy)/(GetParameters().m_fAttackRange-nominalAccuracyStopDistance));
		float scaledAccuracy(slop*(GetParameters().m_fAttackRange-distance));
		return scaledAccuracy;
	}
	return curAccuracy;
}

//====================================================================
// GetFireTargetObject
//====================================================================
CAIObject* CPuppet::GetFireTargetObject() const
{
	CAIObject *pResult = m_refFireTarget.GetAIObject();
	if (pResult)
		return pResult;
	pResult = m_refAttentionTarget.GetAIObject();

	CCCPOINT(CPuppet_GetFireTargetObject);
	return pResult;
}

//====================================================================
// RequestThrowGrenade
//====================================================================
void CPuppet::RequestThrowGrenade(ERequestedGrenadeType eGrenadeType, int iRegType)
{
	m_bGrenadeThrowRequested = true;
	m_eGrenadeThrowRequestType = eGrenadeType;
	m_iGrenadeThrowTargetType = iRegType;
}


//====================================================================
// FireCommand
//====================================================================
void CPuppet::FireCommand(float updateTime)
{
	CCCPOINT(CPuppet_FireCommand);
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	m_timeSinceTriggerPressed += updateTime;

	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
	if(pAttentionTarget && (pAttentionTarget->IsAgent() || (pAttentionTarget->GetType() == AIOBJECT_TARGET)))
		m_lastLiveTargetPos = pAttentionTarget->GetPos();

	bool	lastAim = m_State.aimTargetIsValid;

	// Secondary fire if using a secondary fire mode or a grenade throw was requested
	const bool bIsSecondaryFire = IsSecondaryFireCommand() || m_bGrenadeThrowRequested;
	const bool bIsMeleeFire = IsMeleeFireCommand();

	// Reset aiming and shooting, they will be set in to code below if needed.
	m_State.aimTargetIsValid = false;
	m_State.fire = false;

	// Only reset if not doing a secondary fire anymore
	if (!bIsSecondaryFire && m_State.fireSecondary)
	{
		m_State.fireSecondary = false;
		m_pFireCmdGrenade->Reset();
	}

	// Same for melee
	if (!bIsMeleeFire && m_State.fireMelee)
	{
		m_State.fireMelee = false;
	}

	// Choose target to shoot at.
	CWeakRef<CAIObject>	refChooseTarget;
	if (m_refFireTarget.IsValid())
		refChooseTarget = m_refFireTarget;
	else
		refChooseTarget = m_refAttentionTarget;

	if (bIsSecondaryFire)
	{
		if (m_fireModeUpdated)
		{
			m_pFireCmdGrenade->Reset();
			m_fireModeUpdated = false;
		}

		ERequestedGrenadeType eReqType = eRGT_ANY;
		if (m_bGrenadeThrowRequested)
		{
			// Get target
			switch (m_iGrenadeThrowTargetType)
			{
				case AI_REG_ATTENTIONTARGET:
					refChooseTarget = m_refAttentionTarget;
					break;

				case AI_REG_LASTOP:
					refChooseTarget = m_refLastOpResult;
					break;

				case AI_REG_REFPOINT:
					refChooseTarget = GetWeakRef(GetRefPoint());
					break;

				default:
					CRY_ASSERT_MESSAGE(0, "Unhandled reg type for requested grenade throw");
			}

			eReqType = m_eGrenadeThrowRequestType;
		}
		else if (m_fireMode == FIREMODE_SECONDARY_SMOKE)
		{
			if (m_refLastOpResult.IsValid())
				refChooseTarget = m_refLastOpResult;
			eReqType = eRGT_SMOKE_GRENADE;
		}

		if (refChooseTarget.IsValid())
		{
			CAIObject * const pTarget = refChooseTarget.GetAIObject();
			m_State.vAimTargetPos = pTarget->GetPos();
			m_State.aimTargetIsValid = true;
			m_aimState = AI_AIM_READY;
			FireSecondary(pTarget, eReqType);
		}
		return;
	}
	else if (bIsMeleeFire)
	{
		if (refChooseTarget.IsValid())
		{
			// Do not aim, use look only.
			CAIObject * const pTarget = refChooseTarget.GetAIObject();
			m_State.vAimTargetPos = pTarget->GetPos();
			m_State.aimTargetIsValid = false;
			m_aimState = AI_AIM_NONE; // AI_AIM_READY;
			FireMelee(pTarget);
		}
		return;
	}

	if (!m_pFireCmdHandler)
		return;

	if (m_fireMode == FIREMODE_OFF)
	{
		m_friendOnWayElapsedTime = 0.0f;
	}

	bool	targetValid = false;
	Vec3	aimTarget(0,0,0);
	CAIObject * const pTarget = refChooseTarget.GetAIObject();

	// Check if the target is valid based on the current fire mode.
	if (m_fireMode == FIREMODE_AIM || m_fireMode == FIREMODE_AIM_SWEEP ||
		m_fireMode == FIREMODE_FORCED || m_fireMode == FIREMODE_PANIC_SPREAD)
	{
		// Aiming and forced fire has the loosest checking.
		// As long as the target exists, it can be used.
		if (pTarget)
			targetValid = true;
	}
	else if (m_fireMode != FIREMODE_OFF)
	{
		// The rest of the first modes require that the target exists and is an agent or a vehicle with player inside.
		// The IsActive() handles both cases.
		if (pTarget)
		{
			if (CAIActor* pTargetActor = pTarget->CastToCAIActor())
			{
				if (pTargetActor->IsActive())
					targetValid = true;
			}
			else if (pTarget->GetType() == AIOBJECT_TARGET)
			{
				targetValid = true;
			}
			else if (pTarget->GetSubType() == IAIObject::STP_MEMORY)
			{
				targetValid = true;
			}
		}
	}

	// The loose attention target is more important than the normal target.
	if (gAIEnv.configuration.eCompatibilityMode != ECCM_WARFACE)
		if (m_bLooseAttention && m_refLooseAttentionTarget != pTarget && GetSubType() != CAIObject::STP_2D_FLY )
			targetValid = false;

	float distToTargetSqr = FLT_MAX;

	bool canFire = targetValid && !m_fireDisabled && AllowedToFire();
	if (canFire)
	{
		if (pTarget)
		{
			// Allow forced fire even when outside the specified range.
			if(m_fireMode != FIREMODE_FORCED)
			{
				// Check if within fire range
				distToTargetSqr = Distance::Point_PointSq(pTarget->GetPos(), GetFirePos());
				if (distToTargetSqr > sqr(m_Parameters.m_fAttackRange))
				{
					// Out of range.
					canFire = false;
					targetValid = false;
				}
			}
		}
	}

	bool useLiveTargetForMemory = m_targetLostTime < m_CurrentWeaponDescriptor.coverFireTime;

	// As a default handling, aim at the target.
	// The fire command handler will alter the target position later,
	// but a sane default value is provided here for at least aiming.
	if (m_fireMode != FIREMODE_OFF && targetValid && !m_fireDisabled)
	{
		m_weaponSpinupTime += GetAISystem()->GetFrameDeltaTime();

		aimTarget = pTarget->GetPos();

		// Marcio: Added check for the zero-ness of m_lastLiveTargetPos. 
		// m_lastLiveTargetPos can be zero in the first few updates. It will happen if a guy spawns directly into combat.
		if(useLiveTargetForMemory && pTarget && !m_lastLiveTargetPos.IsZero() && (pTarget->GetSubType() == IAIObject::STP_MEMORY))
			aimTarget = m_lastLiveTargetPos;

		// When using laser, aim slightly lower so that it does not look bad when hte laser asset
		// is pointing directly at the player.
		const int enabledAccessories = GetParameters().m_weaponAccessories;
		if (enabledAccessories & AIWEPA_LASER)
		{
			// Make sure the laser is visibile when aiming directly at the player
			aimTarget.z -= 0.15f;
		}

		if	( GetSubType() != CAIObject::STP_2D_FLY )
		{
			float distSqr = Distance::Point_Point2DSq(aimTarget, GetFirePos());
			if (distSqr < sqr(2.0f))
			{
				Vec3 safePos = GetFirePos() + GetBodyDir() * 2.0f;
				if (distSqr < sqr(0.7f))
					aimTarget = safePos;
				else
				{
					float speed = m_State.vMoveDir.GetLength();
					speed = clamp(speed,0.0f,10.f);

					float d = sqrtf(distSqr);
					float u = 1.0f - (d - 0.7f) / (2.0f - 0.7f);
					aimTarget += speed/10 * u * (safePos - aimTarget);
				}
			}
		}
		
		//GetAISystem()->AddDebugLine(GetPos(), aimTarget, 255,255,255, 0.15f);

		if (!m_pFireCmdHandler || m_pFireCmdHandler->UseDefaultEffectFor(m_fireMode))
		{
			switch(m_fireMode)
			{
			case FIREMODE_PANIC_SPREAD:
				HandleWeaponEffectPanicSpread(pTarget, aimTarget, canFire);
				break;
			case FIREMODE_BURST_DRAWFIRE:
				HandleWeaponEffectBurstDrawFire(pTarget, aimTarget, canFire);
				break;
			case FIREMODE_BURST_SNIPE:
				HandleWeaponEffectBurstSnipe(pTarget, aimTarget, canFire);
				break;
			case FIREMODE_BURST_WHILE_MOVING:
				HandleWeaponEffectBurstWhileMoving(pTarget, aimTarget, canFire);
				break;
			case FIREMODE_AIM_SWEEP:
				HandleWeaponEffectAimSweep(pTarget, aimTarget, canFire);
				break;
			};
		}

		aimTarget = UpdateTargetTracking(GetWeakRef(pTarget), aimTarget);
	}
	else
	{
		ResetTargetTracking();

		m_targetBiasDirection *= 0.5f;
		m_burstEffectTime = 0.0f;
		m_burstEffectState = 0;
		m_weaponSpinupTime = 0;

		// (Kevin) Warface still needs target tracking to be updated when the AI cannot fire their weapon
		if (gAIEnv.configuration.eCompatibilityMode == ECCM_WARFACE)
		{
			aimTarget = UpdateTargetTracking(GetWeakRef(pTarget), aimTarget);
		}
	}

	if (m_fireMode == FIREMODE_OFF)
		aimTarget.Set(0,0,0);

	// When moving and not allowed to strafe, limit the aiming to targets towards the movement target.
	if (GetSubType() != CAIObject::STP_2D_FLY)
	{
		bool isMoving = m_State.fDesiredSpeed > 0.0f && m_State.curActorTargetPhase == eATP_None && !m_State.vMoveDir.IsZero();
		if (isMoving && !m_State.allowStrafing)
		{
			Vec3 dirToTarget = aimTarget - GetFirePos();
			float distToTarget = dirToTarget.NormalizeSafe();
			const float maximumShootingAngle = (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2) ? 90.0f : 30.0f;
			const float thr = cosf(DEG2RAD(maximumShootingAngle));
			if (m_State.vMoveDir.Dot(dirToTarget) < thr)
			{
				aimTarget.Set(0,0,0);
				targetValid = false;
				canFire = false;
			}
		}
	}

	// Aim target interpolation.
	float turnSpeed = -1;
	if (canFire)
		turnSpeed = m_Parameters.m_fireTurnSpeed;
	else
		turnSpeed = m_Parameters.m_aimTurnSpeed;

	if (turnSpeed <= 0.0f)
		m_State.vAimTargetPos = aimTarget;
	else
		m_State.vAimTargetPos = InterpolateLookOrAimTargetPos(m_State.vAimTargetPos, aimTarget, turnSpeed);

	// Check if the aim is obstructed and cancel aiming if it is.
	m_State.aimObstructed = false;
	if (m_fireMode != FIREMODE_FORCED && m_fireMode != FIREMODE_OFF)
	{
		if (!m_State.vAimTargetPos.IsZero())
		{
			// Aiming, check towards aim target.
			if (!CanAimWithoutObstruction(m_State.vAimTargetPos))
				m_State.aimObstructed = true;
		}
		else
		{
			// Not aiming, check along forward direction.
			//if (!CanAimWithoutObstruction(GetFirePos() + GetBodyDir() * 10.0f))
				m_State.aimObstructed = true;
		}
	}

	// Disable firing when obstructed.
	if (m_State.aimObstructed)
	{
		m_State.aimTargetIsValid = false;
		targetValid = false;
		canFire = false;
	}

	/*	if (m_State.aimLook)
	{
	GetAISystem()->AddDebugLine(GetFirePos(), aimTarget, 255,196,0, 0.5f);
	GetAISystem()->AddDebugLine(GetFirePos(), m_State.vAimTargetPos, 255,0,0, 0.5f);
	GetAISystem()->AddDebugLine(m_State.vAimTargetPos, m_State.vAimTargetPos+Vec3(0,0,0.5f), 255,0,0, 0.5f);
	}*/

	// Make the shoot target the same as the aim target by default.
	m_State.vShootTargetPos = m_State.vAimTargetPos;

	SAIWeaponInfo	weaponInfo;
	GetProxy()->QueryWeaponInfo(weaponInfo);

	if (m_wasReloading && !weaponInfo.isReloading)
		SetSignal(1, "OnReloaded", GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnReloaded);

	if (m_fireMode != FIREMODE_OFF)
	{
		if (weaponInfo.outOfAmmo || weaponInfo.isReloading)
			canFire = false;

		if (!m_outOfAmmoSent)
		{
			if (weaponInfo.outOfAmmo)
			{
				SetSignal(1,"OnOutOfAmmo",GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnOutOfAmmo);

				m_outOfAmmoSent = true;
				m_outOfAmmoTimeOut = 0.0f;
				m_burstEffectTime = 0.0f;
				m_burstEffectState = 0;
				if (m_pFireCmdHandler)
					m_pFireCmdHandler->OnReload();
			}
		}
		else
		{
			if (!weaponInfo.outOfAmmo)
				m_outOfAmmoSent = false;
			else if ((gAIEnv.configuration.eCompatibilityMode != ECCM_WARFACE) && !weaponInfo.isReloading)
			{
				m_outOfAmmoTimeOut += updateTime;
				if (m_outOfAmmoTimeOut > 3.0f)
					m_outOfAmmoSent = false;
			}
		}
	}

	m_wasReloading = weaponInfo.isReloading;

	if (m_State.vAimTargetPos.IsZero())
	{
		m_State.aimTargetIsValid = false;
		if (m_State.aimObstructed)
		{
			// Something has been detected to be obstructing the aiming.
			m_aimState = AI_AIM_OBSTRUCTED;
		}
		else
		{
			// No aiming requested.
			m_aimState = AI_AIM_NONE;
		}
	}
	else
	{
		m_State.aimTargetIsValid = true;
		SAIBodyInfo bodyInfo;
		GetProxy()->QueryBodyInfo(bodyInfo);

		if (bodyInfo.isAiming)
		{
			// The animation has finished the aim transition.
			m_aimState = AI_AIM_READY;
		}
		else
		{
			if (!lastAim)
			{
				// The game has not yet responded to the aiming request, wait.
				m_aimState = AI_AIM_WAITING;
			}
			else
			{
				// The game has disabled aiming while it was previously possible, treat as if the aiming is obscured.
				m_aimState = AI_AIM_OBSTRUCTED;
			}
		}
	}

	if (m_pFireCmdHandler && m_fireMode != FIREMODE_OFF)
	{
		// If just starting to fire, reset the fire handler.
		if (m_fireModeUpdated)
		{
			m_pFireCmdHandler->Reset();
			m_fireModeUpdated = false;
			// Reset the friend on way filter too.
			m_friendOnWayCounter = 0.0f;
		}

		// [mikko] - This workaround sets the memory target position temporarily
		// to the last known live target position for better covering fire behavior.
		Vec3	workaroundTemp;
		if (useLiveTargetForMemory && pTarget && pTarget->GetSubType() == IAIObject::STP_MEMORY)
		{
			workaroundTemp = pTarget->GetPos();
			if(!m_lastLiveTargetPos.IsZero())
				pTarget->SetPos(m_lastLiveTargetPos);
		}

		m_State.fire = m_pFireCmdHandler->Update(pTarget, canFire, m_fireMode, m_CurrentWeaponDescriptor, m_State.vAimTargetPos);

		// workaround continued.
		if (useLiveTargetForMemory && pTarget && pTarget->GetSubType() == IAIObject::STP_MEMORY)
			pTarget->SetPos(workaroundTemp);
	}

	if (m_State.fire)
	{
		m_timeSinceTriggerPressed = 0.0f;
		m_State.vShootTargetPos = m_State.vAimTargetPos;
	}

	// Warn about conflicting fire state.
	if (m_State.fire && !canFire)
		AIWarning("CPuppet::FireCommand(): state.fire = true && canFire == false");

	// Update accessories.
	m_State.weaponAccessories = 0;

	const int enabledAccessories = GetParameters().m_weaponAccessories;

	if (enabledAccessories & AIWEPA_LASER)
		m_State.weaponAccessories |= AIWEPA_LASER;

	if (IsAllowedToUseExpensiveAccessory())
	{
		if (enabledAccessories & AIWEPA_COMBAT_LIGHT)
		{
			if (GetAlertness() > 0)
				m_State.weaponAccessories |= AIWEPA_COMBAT_LIGHT;
		}
		if (enabledAccessories & AIWEPA_PATROL_LIGHT)
		{
			m_State.weaponAccessories |= AIWEPA_PATROL_LIGHT;
		}
	}
}

//===================================================================
// HandleWeaponEffectBurstWhileMoving
//===================================================================
void CPuppet::HandleWeaponEffectBurstWhileMoving(CAIObject* pTarget, Vec3& aimTarget, bool& canFire)
{
	if (canFire)
	{
		// Allow to shoot while moving.
		bool isAgent = pTarget->IsAgent();
		bool isMemory = pTarget->GetSubType() == STP_MEMORY;
		bool isSimpleTarget = pTarget->GetType() == AIOBJECT_TARGET;

		if (pTarget && (isAgent || isMemory || isSimpleTarget))
		{
			m_burstEffectTime += m_fTimePassed;

			bool sprinting = m_State.fMovementUrgency >= 0.5f * (AISPEED_RUN + AISPEED_SPRINT);
			const float burstLenght = sprinting ? 1.5f : 2.5f; //IsCoverFireEnabled() ? 4.0f : 3.0f;

			if (gAIEnv.configuration.eCompatibilityMode != ECCM_CRYSIS2)
			{
				if(m_burstEffectTime > 3.5f)
					m_burstEffectTime = 0.0f;
			}

			const float facingThr = cosf(DEG2RAD(sprinting ? 10.0f : 30.0f));
			Vec3 targetDir = aimTarget - GetFirePos();
			targetDir.Normalize();

			Vec3 aimDir;
			if (gAIEnv.configuration.eCompatibilityMode == ECCM_GAME04)
			{
				// Kevin - Use actual firing direction for aiming
				// TODO INTEGRATION : Part of "fire and look direction attached to skeleton" fix
				SAIBodyInfo bodyInfo;
				GetProxy()->QueryBodyInfo(bodyInfo);
				aimDir = bodyInfo.vFireDir;
			}
			else
			{
				aimDir = GetBodyDir(); //m_State.vAimTargetPos - GetFirePos();
			}
			
//			aimDir.Normalize();
/*			if (GetBodyDir().Dot(aimDir) > facingThr &&
				((m_burstEffectTime > 0.1f && m_burstEffectTime < burstLenght) ||*/

			bool allowedToFire;

			// Crysis 2 doesn't care about burst control here, since it's already taken care by the fire handler
			// and we don't care about the facing direction that much, shooting sideways is fine; as long as it's 'forward'
			if (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2)
			{
				allowedToFire = aimDir.Dot(targetDir) > 0.0f;
			}
			else
			{
				allowedToFire = (aimDir.Dot(targetDir) > facingThr && m_burstEffectTime > 0.1f && m_burstEffectTime < burstLenght) ||
					(m_State.fDistanceToPathEnd > 0.0f && m_State.fDistanceToPathEnd < gAIEnv.CVars.BurstWhileMovingDestinationRange);
			}

			if (!allowedToFire)
			{
				canFire = false;
				aimTarget.Set(0,0,0);
			}
		}
		else
		{
			canFire = false;
			aimTarget.Set(0,0,0);
			m_burstEffectTime = 0.0f;
			m_burstEffectState = 0;
		}
	}
}

//===================================================================
// HandleWeaponEffectBurstDrawFire
//===================================================================
void CPuppet::HandleWeaponEffectBurstDrawFire(CAIObject* pTarget, Vec3& aimTarget, bool& canFire)
{
	float drawFireTime = m_CurrentWeaponDescriptor.drawTime;
	if (m_CurrentWeaponDescriptor.fChargeTime > 0)
		drawFireTime += m_CurrentWeaponDescriptor.fChargeTime;

	const float minDist = 5.0f;

	if (Distance::Point_PointSq(aimTarget, GetFirePos()) > sqr(minDist))
	{
		if (m_burstEffectTime < drawFireTime)
		{
			Vec3 shooterGroundPos = GetPhysicsPos();
			Vec3 targetGroundPos;
			Vec3 targetPos = aimTarget;

			if (pTarget->GetProxy())
			{
				targetGroundPos = pTarget->GetPhysicsPos();
				targetGroundPos.z -= 0.25f;
			}
			else
			{
				// Assume about human height target.
				targetGroundPos = targetPos;
				targetGroundPos.z -= 1.5f;
			}

			float floorHeight = min(targetGroundPos.z, shooterGroundPos.z);

/*			GetAISystem()->AddDebugLine(Vec3(shooterGroundPos.x, shooterGroundPos.y, floorHeight),
			Vec3(targetGroundPos.x, targetGroundPos.y, floorHeight), 128,128,128, 0.5f);
			Vec3 mid = (shooterGroundPos+targetGroundPos)/2;
			GetAISystem()->AddDebugLine(Vec3(mid.x, mid.y, floorHeight-1),
			Vec3(mid.x, mid.y, floorHeight+1), 255,128,128, 0.5f);
*/
			Vec3 dirTargetToShooter = shooterGroundPos - targetGroundPos;
			dirTargetToShooter.z = 0.0f;
			float dist = dirTargetToShooter.NormalizeSafe();

			const Vec3& firePos = GetFirePos();

			float endHeight = targetGroundPos.z;
			float startHeight = floorHeight - (firePos.z - floorHeight);

			float t;
			if (m_CurrentWeaponDescriptor.fChargeTime > 0)
				t = clamp((m_burstEffectTime-m_CurrentWeaponDescriptor.fChargeTime)/(drawFireTime-m_CurrentWeaponDescriptor.fChargeTime), 0.0f, 1.0f);
			else
				t = clamp(m_burstEffectTime/drawFireTime, 0.0f, 1.0f);

			CPNoise3* pNoise = gEnv->pSystem->GetNoiseGen();
			float noiseScale = clamp((m_burstEffectTime-0.5f), 0.0f, 1.0f);
			float noise = noiseScale * pNoise->Noise1D(m_spreadFireTime + m_burstEffectTime*m_CurrentWeaponDescriptor.sweepFrequency);
			Vec3 right(dirTargetToShooter.y, -dirTargetToShooter.x, 0);

			aimTarget = targetGroundPos + right * (noise * m_CurrentWeaponDescriptor.sweepWidth);
			aimTarget.z = startHeight + (endHeight - startHeight) * (1- sqr(1-t));

			// Clamp to bottom plane.
			if (aimTarget.z < floorHeight && fabsf(aimTarget.z - firePos.z) > 0.01f)
			{
				float u = (floorHeight - firePos.z) / (aimTarget.z - firePos.z);
				aimTarget = firePos + (aimTarget - firePos) * u;
			}
/*
			GetAISystem()->AddDebugLine(firePos, targetGroundPos, 255,255,255, 0.5f);
			GetAISystem()->AddDebugLine(firePos, m_State.vAimTargetPos, 255,255,255, 0.5f);
			GetAISystem()->AddDebugLine(firePos, aimTarget, 255,255,255, 0.5f);
			GetAISystem()->AddDebugLine(aimTarget, aimTarget+Vec3(0,0,0.5f), 255,255,255, 0.5f);
			*/
		}
		else if (m_targetLostTime > m_CurrentWeaponDescriptor.drawTime)
		{
			float amount = clamp((m_targetLostTime - m_CurrentWeaponDescriptor.drawTime) / m_CurrentWeaponDescriptor.drawTime, 0.0f, 1.0f);

			Vec3 forw = GetBodyDir();
			Vec3 right(forw.y, -forw.x, 0);
			Vec3 up(0,0,1);
			right.NormalizeSafe();
			float distToTarget = Distance::Point_Point(aimTarget, GetPos());

			float t = m_spreadFireTime + m_burstEffectTime*m_CurrentWeaponDescriptor.sweepFrequency;
			float mag = amount * m_CurrentWeaponDescriptor.sweepWidth/2;

			CPNoise3* pNoise = gEnv->pSystem->GetNoiseGen();

			float ox = sinf(t*1.5f) * mag + pNoise->Noise1D(t) * mag;
			float oy = pNoise->Noise1D(t + 33.0f)/2 * mag;

			aimTarget += ox*right + oy*up;
		}

		if (m_burstEffectTime < 0.2f)
		{
			if (!m_State.vAimTargetPos.IsZero())
			{
				const Vec3& pos = GetPos();
				// When starting to fire, make sure the aim target as fully contracted.
				//						if (Distance::Point_PointSq(aimTarget, m_State.vAimTargetPos) > sqr(0.5f))

				if (m_State.vAimTargetPos.z > (aimTarget.z + 0.25f))
				{
					// Current aim target still to high.
					canFire = false;
				}
				else
				{
					// Check for distance.
					const float distToCurSq = Distance::Point_Point2DSq(pos, m_State.vAimTargetPos);
					const float thr = sqr(Distance::Point_Point2D(pos, aimTarget) + 0.5f);
					if (distToCurSq > thr)
						canFire = false;
				}
			}
		}
		if (canFire) // && m_aimState != AI_AIM_OBSTRUCTED)
			m_burstEffectTime += m_fTimePassed;
	}
}

//===================================================================
// HandleWeaponEffectBurstSnipe
//===================================================================
void CPuppet::HandleWeaponEffectBurstSnipe(CAIObject* pTarget, Vec3& aimTarget, bool& canFire)
{
	CCCPOINT(CPuppet_HandleWeaponEffectBurstSnipe);

	float drawFireTime = m_CurrentWeaponDescriptor.drawTime;
	if (m_CurrentWeaponDescriptor.fChargeTime > 0)
		drawFireTime += m_CurrentWeaponDescriptor.fChargeTime;

	// Make it look like the sniper is aiming for headshots.
	float headHeight = aimTarget.z;
	const CAIActor* pLiveTarget = GetLiveTarget( GetWeakRef(pTarget) ).GetAIObject();
	if (pLiveTarget && pLiveTarget->GetProxy())
	{
		IPhysicalEntity* pPhys = pLiveTarget->GetProxy()->GetPhysics(true);
		if (!pPhys)
			pPhys = pLiveTarget->GetPhysics();
		if (pPhys)
		{
			pe_status_pos statusPos;
			pPhys->GetStatus(&statusPos);
			float minz = statusPos.BBox[0].z + statusPos.pos.z;
			float maxz = statusPos.BBox[1].z + statusPos.pos.z + 0.25f;

			// Rough sanity check.
			if (headHeight >= minz && headHeight <= maxz)
				headHeight = maxz;
		}
	}

//	GetAISystem()->AddDebugLine(Vec3(aimTarget.x-1, aimTarget.y, headHeight),
//		Vec3(aimTarget.x+1, aimTarget.y, headHeight), 255,255,255, 0.2f);

	const Vec3& firePos = GetFirePos();
	Vec3 dirTargetToShooter = aimTarget - firePos;
	dirTargetToShooter.z = 0.0f;
	float dist = dirTargetToShooter.NormalizeSafe();
	Vec3 right(dirTargetToShooter.y, -dirTargetToShooter.x, 0);
	Vec3 up(0,0,1);
	float noiseScale = 1.0f;

	if (m_burstEffectState == 0)
	{
		if (canFire && m_aimState != AI_AIM_OBSTRUCTED)
			m_burstEffectTime += m_fTimePassed;

		// Aim towards the head position.
		if (m_burstEffectTime < drawFireTime)
		{
			Vec3 shooterGroundPos = GetPhysicsPos();
			Vec3 targetGroundPos;

			//					float floorHeight = min(targetGroundPos.z, shooterGroundPos.z);

			//					GetAISystem()->AddDebugLine(Vec3(shooterGroundPos.x, shooterGroundPos.y, floorHeight),
			//					Vec3(targetGroundPos.x, targetGroundPos.y, floorHeight), 128,128,128, 0.5f);
			//					Vec3 mid = (shooterGroundPos+targetGroundPos)/2;
			//					GetAISystem()->AddDebugLine(Vec3(mid.x, mid.y, floorHeight-1),
			//					Vec3(mid.x, mid.y, floorHeight+1), 255,128,128, 0.5f);

			

			float endHeight = aimTarget.z; //targetGroundPos.z;
			float startHeight = aimTarget.z-0.5f; //floorHeight - (firePos.z - floorHeight);

			float t;
			if (m_CurrentWeaponDescriptor.fChargeTime > 0)
				t = clamp((m_burstEffectTime-m_CurrentWeaponDescriptor.fChargeTime)/(drawFireTime-m_CurrentWeaponDescriptor.fChargeTime), 0.0f, 1.0f);
			else
				t = clamp(m_burstEffectTime/drawFireTime, 0.0f, 1.0f);

			noiseScale = t;

			//					aimTarget = targetGroundPos + right * (noise * m_CurrentWeaponDescriptor.sweepWidth);
//			aimTarget = aimTarget + ox*right + oy*up;
			aimTarget.z = startHeight + (endHeight - startHeight) * t;

			// Clamp to bottom plane.
			//					if (aimTarget.z < floorHeight && fabsf(aimTarget.z - firePos.z) > 0.01f)
			//					{
			//						float u = (floorHeight - firePos.z) / (aimTarget.z - firePos.z);
			//						aimTarget = firePos + (aimTarget - firePos) * u;
			//					}

			//						GetAISystem()->AddDebugLine(firePos, targetGroundPos, 255,255,255, 0.5f);
			//						GetAISystem()->AddDebugLine(firePos, m_State.vAimTargetPos, 255,255,255, 0.5f);
			//						GetAISystem()->AddDebugLine(firePos, aimTarget, 255,255,255, 0.5f);
			//						GetAISystem()->AddDebugLine(aimTarget, aimTarget+Vec3(0,0,0.5f), 255,255,255, 0.5f);
		}
		else
		{
			m_burstEffectState = 1;
			m_burstEffectTime = -1;
		}
	}
	else if (m_burstEffectState == 1)
	{
		if (m_targetLostTime > m_CurrentWeaponDescriptor.drawTime)
		{
			if (m_burstEffectTime < 0)
				m_burstEffectTime = 0;

			if (canFire && m_aimState != AI_AIM_OBSTRUCTED)
				m_burstEffectTime += m_fTimePassed;

			// Target getting lost, aim above the head.
			float amount = clamp((m_targetLostTime - m_CurrentWeaponDescriptor.drawTime) / m_CurrentWeaponDescriptor.drawTime, 0.0f, 1.0f);

			Vec3 forw = GetBodyDir();
			Vec3 rightVector(forw.y, -forw.x, 0);
			Vec3 upVector(0,0,1);
			rightVector.NormalizeSafe();
			float distToTarget = Distance::Point_Point(aimTarget, GetFirePos());

//			float t = m_spreadFireTime + m_burstEffectTime*m_CurrentWeaponDescriptor.sweepFrequency;
			float mag = amount * m_CurrentWeaponDescriptor.sweepWidth/2 * clamp((distToTarget - 1.0f)/5.0f, 0.0f, 1.0f);

			CPNoise3* pNoise = gEnv->pSystem->GetNoiseGen();

			float tsweep = m_burstEffectTime*m_CurrentWeaponDescriptor.sweepFrequency;

			float ox = sinf(tsweep) * mag; // + pNoise->Noise1D(t) * mag;
			float oy = 0; //pNoise->Noise1D(t + 33.0f)/4 * mag;

			aimTarget.z = aimTarget.z + (headHeight - aimTarget.z) * clamp(m_burstEffectTime, 0.0f, 1.0f);
			aimTarget += ox*rightVector + oy*upVector;
		}
	}

	float noiseTime = m_spreadFireTime;
	m_spreadFireTime += m_fTimePassed;
		
	noiseTime *= m_CurrentWeaponDescriptor.sweepFrequency * 2;
	CPNoise3* pNoise = gEnv->pSystem->GetNoiseGen();
	float nx = pNoise->Noise1D(noiseTime) * noiseScale * 0.1f;
	float ny = pNoise->Noise1D(noiseTime + 33.0f) * noiseScale * 0.1f;
	aimTarget += right*nx + up*ny;

//	GetAISystem()->AddDebugLine(aimTarget, firePos, 255,0,0, 10.0f);

}

//===================================================================
// HandleWeaponEffectAimSweep
//===================================================================
void CPuppet::HandleWeaponEffectAimSweep(CAIObject* pTarget, Vec3& aimTarget, bool& canFire)
{
	float drawFireTime = m_CurrentWeaponDescriptor.drawTime;
	if (m_CurrentWeaponDescriptor.fChargeTime > 0)
		drawFireTime += m_CurrentWeaponDescriptor.fChargeTime;

	// Make it look like the sniper is aiming for headshots.
	float headHeight = aimTarget.z;
	const CAIActor* pLiveTarget = GetLiveTarget(GetWeakRef(pTarget)).GetAIObject();
	if (pLiveTarget && pLiveTarget->GetProxy())
	{
		IPhysicalEntity* pPhys = pLiveTarget->GetProxy()->GetPhysics(true);
		if (!pPhys)
			pPhys = pLiveTarget->GetPhysics();
		if (pPhys)
		{
			pe_status_pos statusPos;
			pPhys->GetStatus(&statusPos);
			float minz = statusPos.BBox[0].z + statusPos.pos.z;
			float maxz = statusPos.BBox[1].z + statusPos.pos.z + 0.25f;

			// Rough sanity check.
			if (headHeight >= minz && headHeight <= maxz)
				headHeight = maxz;
		}
	}

	if (m_burstEffectTime < 0)
		m_burstEffectTime = 0;

	if (m_aimState != AI_AIM_OBSTRUCTED)
		m_burstEffectTime += m_fTimePassed;

	// Target getting lost, aim above the head.

	Vec3 forw = GetBodyDir();
	Vec3 right(forw.y, -forw.x, 0);
	Vec3 up(0,0,1);
	right.NormalizeSafe();

//	float t = m_spreadFireTime + m_burstEffectTime*m_CurrentWeaponDescriptor.sweepFrequency;
	float mag = m_CurrentWeaponDescriptor.sweepWidth/2;

	CPNoise3* pNoise = gEnv->pSystem->GetNoiseGen();

	float distToTarget = Distance::Point_Point(aimTarget, GetFirePos());
	float dscale = clamp((distToTarget - 1.0f)/5.0f, 0.0f, 1.0f);

	float tsweep = m_burstEffectTime*m_CurrentWeaponDescriptor.sweepFrequency;

	float ox = sinf(tsweep) * mag * dscale; // + pNoise->Noise1D(t) * mag;
	float oy = 0; //pNoise->Noise1D(t + 33.0f)/4 * mag;

	aimTarget.z = aimTarget.z + (headHeight - aimTarget.z) * clamp(m_burstEffectTime/2, 0.0f, 1.0f);
	aimTarget += ox*right + oy*up;


	float noiseTime = m_spreadFireTime;
	m_spreadFireTime += m_fTimePassed;

	float noiseScale = clamp(m_burstEffectTime, 0.0f, 1.0f) * dscale;

	noiseTime *= m_CurrentWeaponDescriptor.sweepFrequency * 2;
	float nx = pNoise->Noise1D(noiseTime) * noiseScale * 0.1f;
	float ny = pNoise->Noise1D(noiseTime + 33.0f) * noiseScale * 0.1f;
	aimTarget += right*nx + up*ny;
}

//===================================================================
// HandleWeaponEffectPanicSpread
//===================================================================
void CPuppet::HandleWeaponEffectPanicSpread(CAIObject* pTarget, Vec3& aimTarget, bool& canFire)
{
	// Don't start moving until the aim is ready.
	if (m_aimState == AI_AIM_READY)
	{
		m_burstEffectTime += GetAISystem()->GetFrameDeltaTime();
		m_spreadFireTime += GetAISystem()->GetFrameDeltaTime();
	}

	// Calculate aside-to-side wiggly motion when requesting the spread fire.
	float t = m_spreadFireTime;

	Vec3 forw = GetBodyDir();
	Vec3 right(forw.y, -forw.x, 0);
	Vec3 up(0,0,1);
	right.NormalizeSafe();
	float distToTarget = Distance::Point_Point(aimTarget, GetPos());

	float speed = 1.7f;
	float spread = DEG2RAD(40.0f);

	t *= speed;
	float mag = distToTarget*tanf(spread/2);
	mag *= 0.25f + min(m_burstEffectTime / 0.5f, 1.0f)*0.75f;	// Scale the effect down when starting.

	CPNoise3* pNoise = gEnv->pSystem->GetNoiseGen();

	float ox = sinf(t*1.7f) * mag + pNoise->Noise1D(t) * mag;
	float oy = pNoise->Noise1D(t*0.98432f + 33.0f) * mag;

	aimTarget += ox*right + oy*up;
}

//===================================================================
// FireSecondary
//===================================================================
void CPuppet::FireSecondary(CAIObject* pTarget, ERequestedGrenadeType prefGrenadeType) 
{
	// Fire mode is set to off if we cannot do a secondary fire via fire mode

	const bool bIsSecondaryFireCommand = IsSecondaryFireCommand();

	if (!pTarget || !m_pFireCmdGrenade)
	{
		if (bIsSecondaryFireCommand)
			SetFireMode(FIREMODE_OFF);

		return;
	}

	AIWeaponDescriptor descriptor = GetProxy()->GetSecWeaponDescriptor(prefGrenadeType);
	Vec3 targetPos = pTarget->GetPos();

	m_State.requestedGrenadeType = prefGrenadeType;
	m_State.fireSecondary = m_pFireCmdGrenade->Update(pTarget, true, m_fireMode, descriptor, targetPos);
	m_State.vShootTargetPos = targetPos;
	m_State.fProjectileSpeedScale = m_pFireCmdGrenade->GetThrowSpeedScale();

//	if (m_State.fireSecondary && bIsSecondaryFireCommand)
//		SetFireMode(FIREMODE_OFF);

	if (m_State.fireSecondary && m_State.vShootTargetPos.IsZero())
	{
		// Attempt to throw grenade failed
		m_bGrenadeThrowRequested = false;
		m_State.fireSecondary = false;
	}
}


//===================================================================
// FireMelee
//===================================================================
void CPuppet::FireMelee(CAIObject* pTarget) 
{
	SAIWeaponInfo weaponInfo;
	GetProxy()->QueryWeaponInfo(weaponInfo);

	// Check may change Crysis behaviour
	m_State.fireMelee = false;

	// Wait for mercy time to expire
	if(!CanDamageTargetWithMelee())
		return;

	// Wait until at close range and until facing approx the right direction.
	if (m_fireMode != FIREMODE_MELEE_FORCED && (Distance::Point_PointSq(pTarget->GetPos(), GetPos()) > m_Parameters.m_fMeleeHitRange))
		return;

	Vec3 dirToTarget = pTarget->GetPos() - GetPos();
	dirToTarget.z = 0.0f;
	dirToTarget.Normalize();

	float dot = dirToTarget.Dot(GetViewDir());
	const float MELEE_LOOK_TRESHOLD = cosf(DEG2RAD(20.0f));
	if (m_fireMode != FIREMODE_MELEE_FORCED && dot < MELEE_LOOK_TRESHOLD)
		return;

	if (GetAttentionTargetType() == AITARGET_MEMORY)
		return;

	// Execute the melee.
	m_State.fireMelee = true;

	SetSignal(1,"OnMeleeExecuted",GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnMeleeExecuted);
}


//===================================================================
// Compromising
// Evaluates whether the chosen navigation point will expose us too much to the target
//===================================================================
bool CPuppet::Compromising(const Vec3& pos, const Vec3& dir, const Vec3& hideFrom, const Vec3& objectPos, const Vec3& searchPos, bool bIndoor, bool bCheckVisibility) const
{
	if( !bIndoor )
	{
		// allow him to use only the hidespots closer to him than to the enemy
		Vec3 dirHideToEnemy = hideFrom - pos;
		Vec3 dirHide = pos - searchPos;
		if ( dirHide.GetLengthSquared() > dirHideToEnemy.GetLengthSquared() )
			return true;
	}
	// finally: check is the enemy visible from there
	if ( bCheckVisibility && GetAISystem()->CheckPointsVisibility(pos, hideFrom, 5.0f) )
		return true;

	return false;
}

//===================================================================
// TransformFOV
//===================================================================
void CPuppet::TransformFOV()
{
	float	FOVPrimary = m_Parameters.m_PerceptionParams.FOVPrimary;
	float	FOVSecondary = m_Parameters.m_PerceptionParams.FOVSecondary;

	if (FOVPrimary < 0.0f || FOVPrimary > 360.0f )	// see all around
	{
		m_FOVPrimaryCos = -1;
		m_FOVSecondaryCos = -1;
	}
	else 
	{
		if( FOVSecondary >= 0.0f && FOVPrimary > FOVSecondary )
			FOVSecondary = FOVPrimary;

		m_FOVPrimaryCos = cosf( DEG2RAD( FOVPrimary / 2.0f ) );

		if (FOVSecondary < 0.0f || FOVSecondary > 360.0f )	// see all around
			m_FOVSecondaryCos = -1.0f;
		else 
			m_FOVSecondaryCos = cosf( DEG2RAD( FOVSecondary / 2.0f ) );
	}
}

//===================================================================
// SetParameters
//===================================================================
void CPuppet::SetParameters(AgentParameters & sParams)
{
	AILogComment("CPuppet::SetParameters %s (%p)", GetName(), this);

	CAISystem *pAISystem = GetAISystem();

	if (sParams.m_nSpecies != m_Parameters.m_nSpecies)
	{
		GetAISystem()->AddToSpecies(this,sParams.m_nSpecies); // TODO(Marcio): maybe move into AIActor?
		SetSpecies(sParams.m_nSpecies);
	}

	SetGroupId(sParams.m_nGroup);
	
	CAIGroup*	pGroup = pAISystem->GetAIGroup(GetGroupId());
	// (MATT) Here we AI asserted on pGroup, which causes problems when loading some levels in debug
	// because the groupID is initialised to -1, which is used in some levels, and causes no group
	// to be created. I don't really see why this was ever necessary. {2008/08/21}
	if (pGroup)
		pGroup->SetUnitRank((IAIObject*)this,sParams.m_nRank);

	m_Parameters = sParams;

	TransformFOV();
}

//===================================================================
// CheckAwarenessPlayer
//===================================================================
void CPuppet::CheckAwarenessPlayer()
{
	CAIObject *pPlayer = GetAISystem()->GetPlayer();

	if (!pPlayer)
		return;

	Vec3 lookDir = pPlayer->GetViewDir();
	Vec3 relPos = (GetPos() - pPlayer->GetPos());
	float dist = relPos.GetLength();
	if(dist>0)
		relPos /= dist;
	float fdot;
	float threshold;
	bool bCheckPlayerLooking = false;
	if(dist<=1.2f)
	{
		fdot = GetMoveDir().Dot(-relPos);
		threshold = 0;
	}
	else
	{
		bCheckPlayerLooking = true;
		fdot = lookDir.Dot(relPos);
		threshold = cosf(atanf(0.5f/dist)); // simulates a circle of 0.5m radius centered on AIObject, 
	}																	//checks if the player is looking inside it

	if (fdot > threshold && eFOV_Outside != IsObjectInFOV(pPlayer))
	{
		if(m_fLastTimeAwareOfPlayer ==0 && bCheckPlayerLooking)
		{
			IEntity * pEntity = NULL;
			IPhysicalEntity* pPlayerPhE = (pPlayer->GetProxy() ? pPlayer->GetPhysics() : NULL);

			if (RayCastResult result = gAIEnv.pRayCaster->Cast(RayCastRequest(pPlayer->GetPos(), lookDir * (dist+0.5f),
				COVER_OBJECT_TYPES+ent_living, HIT_COVER|HIT_SOFT_COVER, &pPlayerPhE, pPlayerPhE ? 1 : 0)))
			{
				IPhysicalEntity* pCollider =  result[0].pCollider;
				pEntity = (IEntity*) pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
			}

			if (!(pEntity && pEntity==GetEntity()))
			{
				//m_fLastTimePlayerLooking = 0;
				return;
			}
		}
		float fCurrentTime =  GetAISystem()->GetFrameStartTime().GetSeconds();
		if(m_fLastTimeAwareOfPlayer ==0)
			m_fLastTimeAwareOfPlayer = fCurrentTime;
		else if(fCurrentTime - m_fLastTimeAwareOfPlayer >= GetParameters().m_fAwarenessOfPlayer)
		{
			IAISignalExtraData* 	pData = GetAISystem()->CreateSignalExtraData();
			if(pData)
				pData->fValue = dist;
			m_playerAwarenessType = dist<=GetParameters().m_fMeleeRange ? PA_STICKING: PA_LOOKING;
			IEntity* pUserEntity = GetEntity();
			IEntity* pObjectEntity = pPlayer->GetEntity();
			gAIEnv.pSmartObjectManager->SmartObjectEvent(bCheckPlayerLooking ? "OnPlayerLooking" : "OnPlayerSticking",pUserEntity,pObjectEntity);
			SetSignal(1,  bCheckPlayerLooking ? "OnPlayerLooking" : "OnPlayerSticking",pPlayer->GetEntity(),pData, bCheckPlayerLooking ? gAIEnv.SignalCRCs.m_nOnPlayerLooking : gAIEnv.SignalCRCs.m_nOnPlayerSticking);
			m_fLastTimeAwareOfPlayer = fCurrentTime;
		}
	}
	else
	{
		IEntity* pUserEntity = GetEntity();
		IEntity* pObjectEntity = pPlayer->GetEntity();
		if(m_playerAwarenessType==PA_LOOKING)
		{
			SetSignal(1, "OnPlayerLookingAway" ,0 ,0, gAIEnv.SignalCRCs.m_nOnPlayerLookingAway);
			gAIEnv.pSmartObjectManager->SmartObjectEvent( "OnPlayerLookingAway", pUserEntity, pObjectEntity );
		}
		else if(m_playerAwarenessType==PA_STICKING)
		{
			gAIEnv.pSmartObjectManager->SmartObjectEvent( "OnPlayerGoingAway", pUserEntity, pObjectEntity );
			SetSignal(1, "OnPlayerGoingAway", 0,0, gAIEnv.SignalCRCs.m_nOnPlayerGoingAway);
		}
		m_fLastTimeAwareOfPlayer = 0;
		m_playerAwarenessType = PA_NONE;
	}
}

//===================================================================
// RemoveFromGoalPipe
//===================================================================
void CPuppet::RemoveFromGoalPipe(CAIObject* pObject)
{
	/*
	if (m_pCurrentGoalPipe)
	{
		if (m_pCurrentGoalPipe->IsInSubpipe())
		{
			CGoalPipe *pPipe = m_pCurrentGoalPipe;
			do
			{
				if (pPipe->m_pArgument == pObject)
					pPipe->m_pArgument = 0;
			}
			while (pPipe = pPipe->GetSubpipe());
		}
		else
		{
			if (m_pCurrentGoalPipe->m_pArgument == pObject)
				m_pCurrentGoalPipe->m_pArgument = 0;
		}
	}
	*/
}

//===================================================================
// Serialize
//===================================================================
bool CPuppet::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
//	if(ser.IsReading())
//		Reset();  // Luc: already reset by AI System

	bool bNeedsPostSerialize = false;

	ser.BeginGroup( "AIPuppet" );
	{
		bNeedsPostSerialize = CPipeUser::Serialize( ser, objectTracker );

		ser.Value("m_targetApproach",m_targetApproach);
		ser.Value("m_targetFlee",m_targetFlee);
		ser.Value("m_targetApproaching",m_targetApproaching);
		ser.Value("m_targetFleeing",m_targetFleeing);
		ser.Value("m_lastTargetValid",m_lastTargetValid);
		ser.Value("m_lastTargetPos",m_lastTargetPos);
		ser.Value("m_lastTargetSpeed",m_lastTargetSpeed);
		ser.Value("m_fLastUpdateTime",m_fLastUpdateTime);
		ser.Value("m_weaponSpinupTime", m_weaponSpinupTime);

		ser.Value("m_allowedToHitTarget", m_allowedToHitTarget);
		ser.Value("m_bCoverFireEnabled",m_bCoverFireEnabled);
		ser.Value("m_firingReactionTimePassed", m_firingReactionTimePassed);
		ser.Value("m_firingReactionTime", m_firingReactionTime);
		ser.Value("m_outOfAmmoTimeOut", m_outOfAmmoTimeOut);
		ser.Value("m_allowedToUseExpensiveAccessory", m_allowedToUseExpensiveAccessory);

		ser.Value("m_adaptiveUrgencyMin",m_adaptiveUrgencyMin);
		ser.Value("m_adaptiveUrgencyMax",m_adaptiveUrgencyMax);
		ser.Value("m_adaptiveUrgencyScaleDownPathLen",m_adaptiveUrgencyScaleDownPathLen);
		ser.Value("m_adaptiveUrgencyMaxPathLen",m_adaptiveUrgencyMaxPathLen);
		ser.Value("m_chaseSpeed",m_chaseSpeed);
		ser.Value("m_lastChaseUrgencyDist", m_lastChaseUrgencyDist);
		ser.Value("m_lastChaseUrgencySpeed", m_lastChaseUrgencySpeed);
		ser.Value("m_chaseSpeedRate",m_chaseSpeedRate);
		ser.Value("m_delayedStance", m_delayedStance);
		ser.Value("m_delayedStanceMovementCounter", m_delayedStanceMovementCounter);

		ser.Value("m_bGrenadeThrowRequested", m_bGrenadeThrowRequested);
		ser.EnumValue("m_eGrenadeThrowRequestType", m_eGrenadeThrowRequestType, eRGT_INVALID, eRGT_COUNT);
		ser.Value("m_iGrenadeThrowTargetType", m_iGrenadeThrowTargetType);

		ser.Value("m_allowedStrafeDistanceStart", m_allowedStrafeDistanceStart);
		ser.Value("m_allowedStrafeDistanceEnd", m_allowedStrafeDistanceEnd);
		ser.Value("m_allowStrafeLookWhileMoving", m_allowStrafeLookWhileMoving);
		ser.Value("m_strafeStartDistance", m_strafeStartDistance);

		// (MATT) Yet another custom serialiser - must all be properly structured later {2009/03/23}
		SerializeWeakRefMap(ser, "devaluedPoints", m_mapDevaluedPoints);

		ser.Value( "m_playerAwarenessType", (int&)m_playerAwarenessType );
		ser.Value( "m_fLastTimeAwareOfPlayer", m_fLastTimeAwareOfPlayer );
		ser.Value( "m_bCloseContact",m_bCloseContact);
		ser.Value( "m_vForcedNavigation",m_vForcedNavigation);
		ser.Value( "m_adjustpath",m_adjustpath);

		ser.Value("m_alarmedTime", m_alarmedTime);

		ser.BeginGroup("InitialPath");
		{
			int pointCount = m_InitialPath.size();
			ser.Value("pointCount", pointCount);
			Vec3 point;
			char name[16];
			if(ser.IsReading())
			{
				m_InitialPath.clear();

				for(int i=0;i<pointCount;i++)
				{
					sprintf(name, "Point_%d", i);
					ser.Value(name,point);
					m_InitialPath.push_back(point);
				}
			}
			else
			{
				TPointList::iterator itend = m_InitialPath.end();
				int i=0;
				for(TPointList::iterator it = m_InitialPath.begin();it!=itend;++it, i++)
				{
					sprintf(name, "Point_%d", i);
					point = *it;
					ser.Value(name,point);
				}
			}
		}
		ser.EndGroup();
		
		ser.Value("m_vehicleAvoidingTime",m_vehicleAvoidingTime);
		m_refAvoidedVehicle.Serialize(ser,"M_refAvoidedVehicle");

		// Territory
		ser.Value("m_territoryShapeName", m_territoryShapeName);
		if(ser.IsReading())
			m_territoryShape = GetAISystem()->GetGenericShapeOfName(m_territoryShapeName.c_str());

		// Target tracking
		ser.BeginGroup("TargetTracking");
		{
			// The target tracking is update frequently, so just reset them here.
			if(ser.IsReading())
			{
				m_targetSilhouette.Reset();
				m_targetLastMissPoint.Set(0,0,0);
				m_targetFocus = 0.0f;
				m_targetZone = AIZONE_OUT;
				m_targetDistanceToSilhouette = FLT_MAX;
				m_targetBiasDirection.Set(0,0,-1);
				m_targetEscapeLastMiss = 0.0f;
				m_targetLastMissPointSelectionTime.SetSeconds(0.0f);
				m_burstEffectTime = 0.0f;
				m_burstEffectState = 0;
				m_targetHitPartIndex.clear();
			}

			// Serialize the more important values.
			ser.Value("m_targetDamageHealthThr", m_targetDamageHealthThr);
			ser.Value("m_targetSeenTime", m_targetSeenTime);
			ser.Value("m_targetLostTime", m_targetLostTime);
			ser.Value("m_targetDazzlingTime", m_targetDazzlingTime);
	}
	ser.EndGroup();
}
	ser.EndGroup();

	if (ser.IsReading())
	{
		GetAISystem()->NotifyEnableState(this, m_bEnabled);
		m_steeringOccupancy.Reset(Vec3(0,0,0), Vec3(0,1,0), 1.0f);
		m_steeringOccupancyBias = 0;
		m_steeringAdjustTime = 0;
	}

	SetPerceptionHandler();
	if (m_pPerceptionHandler)
	{
		ser.BeginGroup( "PendingEvents" );
		{
			ser.Value("m_fCurrentAwarenessLevel",m_fCurrentAwarenessLevel);
			bNeedsPostSerialize |= m_pPerceptionHandler->Serialize(ser, objectTracker);
		}
		ser.EndGroup();
	}

	return bNeedsPostSerialize;
}

//===================================================================
// PostSerialize
//===================================================================
void CPuppet::PostSerialize(bool bReading)
{
	CPipeUser::PostSerialize(bReading);

	AIAssert(m_pPerceptionHandler);
	if (m_pPerceptionHandler)
	{
		m_pPerceptionHandler->PostSerialize(bReading);
	}
}

//===================================================================
// AddEvent
//===================================================================
SAIPotentialTarget* CPuppet::AddEvent(CWeakRef<CAIObject> refObject, SAIPotentialTarget& ed)
{
	return (m_pPerceptionHandler ? m_pPerceptionHandler->AddEvent(refObject, ed) : NULL);
}

//===================================================================
// MakeMeLeader
//===================================================================
IAIObject* CPuppet::MakeMeLeader()
{
	if (GetGroupId() == -1 || !GetGroupId())
	{
		AIWarning("CPuppet::MakeMeLeader: Invalid GroupID ... %d", GetGroupId());
		return NULL;
	}

	CLeader* pLeader = (CLeader*) GetAISystem()->GetLeader(GetGroupId());
	if (pLeader)
	{
		CWeakRef<CAIObject> refObject = pLeader->GetAssociation();
		if (refObject != this)
			pLeader->SetAssociation( GetWeakRef(this) );
	}
	else
	{
		pLeader = (CLeader*) GetAISystem()->CreateAIObject(AIOBJECT_LEADER, this, 0);

		CCCPOINT(CPuppet_MakeMeLeader);
	}
	return pLeader;
}

void CPuppet::ResetPerception()
{
	CAIActor::ResetPerception();

	if(m_pPerceptionHandler)
	{
		m_pPerceptionHandler->ClearPotentialTargets();
		m_pPerceptionHandler->ClearTempTarget();
	}

	SetAttentionTarget(NILREF);
}

//===================================================================
// AdjustSpeed
//===================================================================
void CPuppet::AdjustSpeed(CAIObject* pNavTarget, float distance)
{
	if(!pNavTarget)
		return;

	if(GetType() != AIOBJECT_PUPPET || IsUsing3DNavigation())
  {
	  // Danny/Kirill TODO: make some smart AdjastSpeed for vehicles chasing, currently just force maximum speed for vehicles always
//	  m_State.fDesiredSpeed = 1.0f;
	  return;
  }

	CCCPOINT(CPuppet_AdjustSpeed);

  // puppet speed control
	CTimeValue fCurrentTime =  GetAISystem()->GetFrameStartTime();
	float timeStep = (fCurrentTime - m_SpeedControl.fLastTime).GetSeconds();

	// evaluate the potential target speed
	float targetSpeed = 0.0f;
	Vec3 targetVel(ZERO);
  Vec3 targetPos = pNavTarget->GetPos();
	IPhysicalEntity * pPhysProxy(NULL);
	if(pNavTarget->GetProxy() && pNavTarget->GetPhysics())
  {
		pPhysProxy = pNavTarget->GetPhysics();
  }
	else if(pNavTarget->GetSubType() == CAIObject::STP_FORMATION)
	{
		CAIObject *pOwner=pNavTarget->GetAssociation().GetAIObject();
		if(pOwner && pOwner->GetProxy() && pOwner->GetPhysics())
			pPhysProxy = pOwner->GetPhysics();
	}

	if (pPhysProxy)
	{
		pe_status_dynamics status;
		pPhysProxy->GetStatus(&status);
		targetVel = status.v;
	}
	else if (fCurrentTime>m_SpeedControl.fLastTime)
	{
		targetVel = (m_SpeedControl.vLastPos - GetPos())/(fCurrentTime - m_SpeedControl.fLastTime).GetSeconds();
	}

	targetSpeed = targetVel.GetLength();

  // get/estimate the path distance to the target point
  float distToEnd = m_State.fDistanceToPathEnd;

  float distToTarget = Distance::Point_Point2D(GetPos(), targetPos);
  distToEnd = max(distToEnd, distToTarget);

  distToEnd -= distance;
  if (distToEnd < 0.0f)
    distToEnd = 0.0f;

	float walkSpeed, runSpeed, sprintSpeed, junk0, junk1;
	GetMovementSpeedRange(AISPEED_WALK, false, junk0, junk1, walkSpeed);
	GetMovementSpeedRange(AISPEED_RUN, false, junk0, junk1, runSpeed);
	GetMovementSpeedRange(AISPEED_SPRINT, false, junk0, junk1, sprintSpeed);

	// ramp up/down the urgency
	static float distForWalk = 4.0f;
	static float distForRun = 6.0f;
	static float distForSprint = 10.0f;

	if (m_lastChaseUrgencyDist > 0)
	{
		if (m_lastChaseUrgencyDist == 4)
		{
			// Sprint
			if (distToEnd < distForSprint - 1.0f)
				m_lastChaseUrgencyDist = 3;
		}
		else if (m_lastChaseUrgencyDist == 3)
		{
			// Run
			if (distToEnd > distForSprint)
				m_lastChaseUrgencyDist = 4;
			if (distToEnd < distForWalk - 1.0f)
				m_lastChaseUrgencyDist = 2;
		}
		else
		{
			// Walk
			if (distToEnd > distForRun)
				m_lastChaseUrgencyDist = 3;
		}
	}
	else
	{
		m_lastChaseUrgencyDist = 0;	// zero
		if (distToEnd > distForRun)
			m_lastChaseUrgencyDist = 4;	// sprint
		else if (distToEnd > distForWalk)
			m_lastChaseUrgencyDist = 3;	// run
		else
			m_lastChaseUrgencyDist = 2;	// walk
	}

	float urgencyDist = IndexToMovementUrgency(m_lastChaseUrgencyDist);


	if (m_lastChaseUrgencySpeed > 0)
	{
		if (m_lastChaseUrgencyDist == 4)
		{
			// Sprint
			if (targetSpeed < runSpeed)
				m_lastChaseUrgencySpeed = 3;
		}
		else if (m_lastChaseUrgencyDist == 3)
		{
			// Run
			if (targetSpeed > runSpeed * 1.2f)
				m_lastChaseUrgencySpeed = 4;
			if (targetSpeed < walkSpeed)
				m_lastChaseUrgencySpeed = 2;
		}
		else
		{
			// Walk
			if (targetSpeed > walkSpeed * 1.2f)
				m_lastChaseUrgencySpeed = 3;
			if (targetSpeed < 0.001f)
				m_lastChaseUrgencySpeed = 0;
		}
	}
	else
	{
		if (targetSpeed > runSpeed * 1.2f)
			m_lastChaseUrgencySpeed = 4;	// sprint
		else if (targetSpeed > walkSpeed * 1.2f)
			m_lastChaseUrgencySpeed = 3;	// run
		else if (targetSpeed > 0.0f)
			m_lastChaseUrgencySpeed = 2;	// walk
		else
			m_lastChaseUrgencySpeed = 0;	// zero
	}

	float urgencySpeed = IndexToMovementUrgency(m_lastChaseUrgencySpeed);


/*	float urgencyDist = AISPEED_ZERO;
	if (distToEnd > distForRun)
		urgencyDist = AISPEED_SPRINT;
	else if (distToEnd > distForWalk)
		urgencyDist = AISPEED_RUN;
	else
		urgencyDist = AISPEED_WALK;

	float urgencySpeed = AISPEED_ZERO;

	if (targetSpeed > runSpeed)
		urgencySpeed = AISPEED_SPRINT;
	else if (targetSpeed > walkSpeed)
		urgencySpeed = AISPEED_RUN;
	else if (targetSpeed > 0.0f)
		urgencySpeed = AISPEED_WALK;
	else
		urgencySpeed = AISPEED_ZERO;*/

	float urgency = max(urgencySpeed, urgencyDist);

	m_State.fMovementUrgency = urgency;
	m_State.predictedCharacterStates.nStates = 0;

	float normalSpeed, minSpeed, maxSpeed;
	GetMovementSpeedRange(urgency, m_State.allowStrafing, normalSpeed, minSpeed, maxSpeed);

	if (targetSpeed < minSpeed)
		targetSpeed = minSpeed;

  // calculate the speed required to match the target speed, and also the
  // speed desired to trace the path without worrying about the target movement.
  // Then take the maximum of each. 
  // Also we have to make sure that we don't overshoot the path, otherwise we'll 
  // keep stopping.
  // m_State.fDesiredSpeed will/should already include/not include slowing at end
  // depending on if the target is moving
    // If dist to path end > lagDistance * 2 then use path speed control
    // if between lagDistance*2 and lagDistance blend between path speed control and absolute speed
    // if less than than then blend to 0
  static float maxExtraLag = 2.0f;
  static float speedForMaxExtraLag = 1.5f;

  float lagDistance = 0.1f + maxExtraLag * targetSpeed / speedForMaxExtraLag;
  Limit(lagDistance, 0.0f, maxExtraLag);

    float frac = distToEnd / lagDistance;
  Limit(frac, 0.0f, 2.2f);
  float chaseSpeed;
  if (frac < 1.0f)
    chaseSpeed = frac * targetSpeed + (1.0f - frac) * minSpeed;
  else if (frac < 2.0f)
    chaseSpeed = (2.0f - frac) * targetSpeed + (frac - 1.0f) * maxSpeed;
    else
    chaseSpeed = maxSpeed * (frac - 1.0f);

  static float chaseSpeedSmoothTime = 1.0f;
  SmoothCD(m_chaseSpeed, m_chaseSpeedRate, timeStep, chaseSpeed, chaseSpeedSmoothTime);

  if (m_State.fDesiredSpeed > m_chaseSpeed)
    m_State.fDesiredSpeed = m_chaseSpeed;

}

// const float CSpeedControl::m_CMaxDist = 3.0f;

//===================================================================
// SAIPotentialTarget::Serialize
//===================================================================
void SAIPotentialTarget::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.EnumValue("type", type, AITARGET_NONE, AITARGET_LAST);
	ser.Value("priority", priority);
	ser.Value("upPriority", upPriority);
	ser.Value("upPriorityTime", upPriorityTime);
	ser.Value("soundTime", soundTime);
	ser.Value("soundMaxTime", soundMaxTime);
	ser.Value("soundThreatLevel", soundThreatLevel);
	ser.Value("soundPos", soundPos);
	ser.Value("visualFrameId", visualFrameId);
	ser.Value("visualTime", visualTime);
	ser.Value("visualMaxTime", visualMaxTime);
	ser.Value("visualPos", visualPos);
	ser.EnumValue("visualType", visualType, VIS_NONE, VIS_LAST);
	ser.EnumValue("threat", threat, AITHREAT_NONE, AITHREAT_LAST);
	ser.Value("threatTime", threatTime);
	ser.Value("exposure", exposure);
	ser.Value("threatTimeout", threatTimeout);
	ser.Value("indirectSight", indirectSight);

	refDummyRepresentation.Serialize(ser, "refDummyRepresentation");
}

//===================================================================
// UpdateBeacon
//===================================================================
void CPuppet::UpdateBeacon()
{
	CCCPOINT(CPuppet_UpdateBeacon);

	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
	CAIObject *pLastOpResult = m_refLastOpResult.GetAIObject();

	if (pAttentionTarget)
		GetAISystem()->UpdateBeacon(GetGroupId(), pAttentionTarget->GetPos(), pAttentionTarget);
	else if (pLastOpResult)
		GetAISystem()->UpdateBeacon(GetGroupId(), pLastOpResult->GetPos(), pLastOpResult);
}

//===================================================================
// CheckTargetInRange
//===================================================================
bool CPuppet::CheckTargetInRange(Vec3& vTargetPos)
{

	Vec3	vTarget = vTargetPos - GetPos();
	float	targetDist2 = vTarget.GetLengthSquared();

	// don't shoot if the target is not in range
	float fMinDistance = m_CurrentWeaponDescriptor.fRangeMin;// m_FireProperties.GetMinDistance();
	float fMaxDistance = m_CurrentWeaponDescriptor.fRangeMax;// m_FireProperties.GetMaxDistance();
	if(targetDist2 < fMinDistance*fMinDistance && fMinDistance>0)
	{
		if(!m_bWarningTargetDistance)
		{
			SetSignal(0, "OnTargetTooClose", GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnTargetTooClose);
			m_bWarningTargetDistance = true;
		}
		return false;
	}
	else if(targetDist2 > fMaxDistance*fMaxDistance && fMaxDistance>0)
	{
		if(!m_bWarningTargetDistance)
		{
			SetSignal(0, "OnTargetTooFar", GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnTargetTooFar);
			m_bWarningTargetDistance = true;
		}
		return false;
	}
	else
		m_bWarningTargetDistance = false;

	return true;
}

//===================================================================
// CanFireInStance
//===================================================================
bool CPuppet::CanFireInStance(EStance stance, float fDistanceRatio /*= 0.9f*/) const
{
	bool bResult = false;

	CAIObject* pLiveTarget = GetLiveTarget(m_refLastOpResult).GetAIObject();
	if (pLiveTarget)
	{
		// Try to use perceived location
		Vec3 vTargetPos;
		if (!GetPerceivedTargetPos(pLiveTarget, vTargetPos))
			vTargetPos = pLiveTarget->GetPos();

		// Do a partial check along the potential fire direction based on distance ratio, to see if aim is blocked
		const float fDistance = vTargetPos.GetDistance(GetPos()) * clamp(fDistanceRatio,0.0f,1.0f);
		bResult = CheckFireLineOfSight(vTargetPos, fDistance, 0.5f, stance);
	}

	return bResult;
}

//===================================================================
// ResetSpeedControl
//===================================================================
void CPuppet::ResetSpeedControl()
{
	m_SpeedControl.Reset(GetPos(),GetAISystem()->GetFrameStartTime());
  m_chaseSpeed = 0.0f;
  m_chaseSpeedRate = 0.0f;
	m_lastChaseUrgencyDist = -1;
	m_lastChaseUrgencySpeed = -1;
}

//===================================================================
// GetPathAgentNavigationBlockers
//===================================================================
// (MATT) This method is very nearly const - just the GetRefPoint call prevents that {2009/04/02}
void CPuppet::GetPathAgentNavigationBlockers(NavigationBlockers &navigationBlockers, const struct PathfindRequest *pRequest)
{
	CCCPOINT(CPuppet_AddNavigationBlockers);
	CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();

	static float cost = 5.0f; //1000.0f;
	static bool radialDecay = true;
	static bool directional = true;

	TMapBlockers::const_iterator itr(m_PFBlockers.find(PFB_ATT_TARGET));
	float	curRadius(itr!=m_PFBlockers.end() ? (*itr).second : 0.f);
	float	sign(1.0f);
	if(curRadius < 0.0f) { sign = -1.0f; curRadius = -curRadius; }
	// see if attention target needs to be avoided
	if ( curRadius > 0.f &&
		pAttentionTarget && IsHostile(pAttentionTarget))
	{
		float r( curRadius );
		if (pRequest)
		{
			static float extra = 1.5f;
			float d1 = extra * Distance::Point_Point(pAttentionTarget->GetPos(), pRequest->startPos);
			float d2 = extra * Distance::Point_Point(pAttentionTarget->GetPos(), pRequest->endPos);
			r = min(min(d1, d2), curRadius);
		}
		NavigationBlocker	enemyBlocker(pAttentionTarget->GetPos(), r * sign, 0.f, cost, radialDecay, directional);
		navigationBlockers.push_back(enemyBlocker);
	}

	// avoid player
	itr = m_PFBlockers.find(PFB_PLAYER);
	curRadius = itr != m_PFBlockers.end() ? (*itr).second : 0.f;
	sign = 1.0f;
	if(curRadius < 0.0f) { sign = -1.0f; curRadius = -curRadius; }
	if (curRadius > 0.0f)
	{
		CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());
		if (pPlayer)
		{
			NavigationBlocker	blocker(pPlayer->GetPos() + pPlayer->GetBodyDir()*curRadius/2, curRadius * sign, 0.f, cost, radialDecay, directional);
			navigationBlockers.push_back(blocker);
		}
	}

	// avoid player
	itr = m_PFBlockers.find(PFB_BETWEEN_NAV_TARGET);
	curRadius = itr != m_PFBlockers.end() ? (*itr).second : 0.f;
	sign = 1.0f;
	if(curRadius < 0.0f) { sign = -1.0f; curRadius = -curRadius; }
	if (curRadius > 0.0f)
	{
		float biasTowardsTarget = 0.7f;
		Vec3 mid = pRequest->endPos * biasTowardsTarget + GetPos() * (1 - biasTowardsTarget);
		curRadius = min(curRadius, Distance::Point_Point(pRequest->endPos, GetPos()) * 0.8f);
		NavigationBlocker	blocker(mid, curRadius * sign, 0.f, cost, radialDecay, directional);
		navigationBlockers.push_back(blocker);
	}


	// see if ref point needs to be avoided
	itr = m_PFBlockers.find(PFB_REF_POINT);
	curRadius = itr!=m_PFBlockers.end() ? (*itr).second : 0.f;
	sign = 1.0f;
	if(curRadius < 0.0f) { sign = -1.0f; curRadius = -curRadius; }
	if ( curRadius > 0.f )
	{
		Vec3 vRefPointPos = GetRefPoint()->GetPos();
		float r( curRadius );
		if (pRequest)
		{
			static float extra = 1.5f;
			float d1 = extra * Distance::Point_Point(vRefPointPos, pRequest->startPos);
			float d2 = extra * Distance::Point_Point(vRefPointPos, pRequest->endPos);
			r = min(min(d1, d2), curRadius);
		}
		NavigationBlocker	enemyBlocker(vRefPointPos, r, 0.f, cost * sign, radialDecay, directional);
		navigationBlockers.push_back(enemyBlocker);
	}

	itr = m_PFBlockers.find(PFB_BEACON);
	curRadius = itr!=m_PFBlockers.end() ? (*itr).second : 0.f;
	sign = 1.0f;
	if(curRadius < 0.0f) { sign = -1.0f; curRadius = -curRadius; }
	IAIObject* pBeacon;
	if ( curRadius > 0.f && (pBeacon=GetAISystem()->GetBeacon(GetGroupId())))
	{
		float r( curRadius );
		if (pRequest)
		{
			static float extra = 1.5f;
			float d1 = extra * Distance::Point_Point(pBeacon->GetPos(), pRequest->startPos);
			float d2 = extra * Distance::Point_Point(pBeacon->GetPos(), pRequest->endPos);
			r = min(min(d1, d2), curRadius);

		}
		NavigationBlocker	enemyBlocker(pBeacon->GetPos(), r, 0.f, cost * sign, radialDecay, directional);
		navigationBlockers.push_back(enemyBlocker);
	}

	// Avoid dead bodies
	float	deadRadius=0.0f;
	const int ignoreDeadBodies = gAIEnv.CVars.IgnoreDeadBodies;
	if (!ignoreDeadBodies)
	{
		itr = m_PFBlockers.find(PFB_DEAD_BODIES);
		deadRadius = itr != m_PFBlockers.end() ? (*itr).second : 0.f;
	}

	itr = m_PFBlockers.find(PFB_EXPLOSIVES);
	float	explosiveRadius = itr != m_PFBlockers.end() ? (*itr).second : 0.f;

	if(fabsf(deadRadius) > 0.01f || fabsf(explosiveRadius) > 0.01f)
	{
		const unsigned int maxn = 3;
		Vec3	positions[maxn];
		unsigned int types[maxn];
		unsigned int n = GetAISystem()->GetDangerSpots(static_cast<const IAIObject*>(this), 40.0f, positions, types, maxn, IAISystem::DANGER_ALL);
		for(unsigned i = 0; i < n; i++)
		{
			float r=explosiveRadius;
			if(types[i] == IAISystem::DANGER_DEADBODY)
			{
				if (ignoreDeadBodies)
					continue;

				r = deadRadius;
			}

			// Skip completely blocking blocking blockers which are too close.
			if (r < 0.0f && Distance::Point_PointSq(GetPos(), positions[i]) < sqr(fabsf(r) + 2.0f))
				continue;
			sign = 1.0f;
			if(r < 0.0f) { sign = -1.0f; r = -r; }
			NavigationBlocker	enemyBlocker(positions[i], r, 0.f, cost * sign, radialDecay, directional);
			navigationBlockers.push_back(enemyBlocker);
		}
	}
}

//===================================================================
// SetPFBlockerRadius
//===================================================================
void CPuppet::SetPFBlockerRadius(int blockerType, float radius)
{
	m_PFBlockers[blockerType] = radius;
}

//===================================================================
// CanTargetPointBeReached
//===================================================================
ETriState CPuppet::CanTargetPointBeReached(CTargetPointRequest &request)
{
	m_DEBUGCanTargetPointBeReached.push_back(request.GetPosition());
	return m_Path.CanTargetPointBeReached(request, this, true);
}

//===================================================================
// UseTargetPointRequest
//===================================================================
bool CPuppet::UseTargetPointRequest(const CTargetPointRequest &request)
{
	m_DEBUGUseTargetPointRequest = request.GetPosition();
	return m_Path.UseTargetPointRequest(request, this, true);
}

//===================================================================
// CheckFriendsInLineOfFire
//===================================================================
bool CPuppet::CheckFriendsInLineOfFire(const Vec3& fireDirection, bool cheapTest)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	if (m_updatePriority != AIPUP_VERY_HIGH)
		cheapTest = true;

	const Vec3& firePos = GetFirePos();
	bool friendOnWay = false;

	CAIPlayer *pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());
	if (pPlayer && !pPlayer->IsHostile(this))
	{
		if (IsFriendInLineOfFire(pPlayer, firePos, fireDirection, cheapTest))
			friendOnWay = true;
	}

	if (!friendOnWay)
	{
		const float checkRadiusSqr = sqr(fireDirection.GetLength() + 2.0f);

		const int species = GetParameters().m_nSpecies;

		const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
			CPuppet* pFriend = it->GetAIObject();
			// Skip for self
			if (!pFriend || pFriend == this)
				continue;
			if (IsHostile(pFriend)) continue;
			// Check against only puppets (ignore vehicles).
			if (pFriend->GetType() != AIOBJECT_PUPPET)
			continue;
			if (!pFriend->IsEnabled())
			continue;
		//FIXME this should never happen - puppet should always have proxy and physics - for Luciano to fix
			if (!pFriend->GetProxy() && !pFriend->GetPhysics())
			continue;
			if (Distance::Point_PointSq(pFriend->GetPos(), firePos) > checkRadiusSqr)
			continue;

			// Skip friends in vehicles.
			if (pFriend->GetProxy()->GetLinkedVehicleEntityId())
				continue;

			if (IsFriendInLineOfFire(pFriend, firePos, fireDirection, cheapTest))
		{
			friendOnWay = true;
			break;
		}
	}
	}
	

	if(friendOnWay)
		m_friendOnWayElapsedTime += GetAISystem()->GetUpdateInterval();
	else
		m_friendOnWayElapsedTime = 0.0f;

	return friendOnWay;
}

//===================================================================
// IsFriendInLineOfFire
//===================================================================
bool CPuppet::IsFriendInLineOfFire(CAIObject* pFriend, const Vec3& firePos, const Vec3& fireDirection, bool cheapTest) //, const Vec3& conePos, const Vec3& coneDir, float coneMinDistSqr, float coneMaxDistSqr)
{
	if (!pFriend->GetProxy())
		return false;
	IPhysicalEntity* pPhys = pFriend->GetProxy()->GetPhysics(true);
	if (!pPhys)
		pPhys = pFriend->GetPhysics();
	if (!pPhys)
		return false;

	const float detectionSide = 0.2f;
	Vec3 fudge(detectionSide,detectionSide,detectionSide);

	pe_status_pos statusPos;
	pPhys->GetStatus(&statusPos);
	AABB	bounds(statusPos.BBox[0] - fudge + statusPos.pos, statusPos.BBox[1] + fudge + statusPos.pos);

	bool hitBounds = Overlap::Lineseg_AABB(Lineseg(firePos, firePos + fireDirection), bounds);

	if (cheapTest)
		return hitBounds;

	if (!hitBounds)
			return false;

		ray_hit	hit;
	if (!gAIEnv.pWorld->CollideEntityWithBeam(pPhys, firePos, fireDirection, 0.2f, &hit))
			return false;

	// Send signal to the friend informing that a friendly agent is blocking the 
	if(m_friendOnWayCounter < 0.01f)
	{
		IAISignalExtraData* 	pData = GetAISystem()->CreateSignalExtraData();
		if(pData)
		{
			pData->point = pFriend->GetPhysicsPos();
			pe_status_dynamics  dSt;
			pFriend->GetPhysics()->GetStatus( &dSt );
			pData->fValue = dSt.v.GetLength();
		}
		SetSignal(1, "OnFriendInWay", pFriend->GetEntity(), pData, gAIEnv.SignalCRCs.m_nOnFriendInWay);
		m_friendOnWayCounter = 2.0f;
	}

	return true;
}

//===================================================================
// GetFloorPosition
//===================================================================
Vec3 CPuppet::GetFloorPosition(const Vec3& pos)
{
	Vec3	floorPos(pos);
	if(GetFloorPos(floorPos, pos, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_STATIC))
		return floorPos;
	return pos;
}

//===================================================================
// AdjustPathAroundObstacles
//===================================================================
bool CPuppet::AdjustPathAroundObstacles()
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);
  if (gAIEnv.CVars.AdjustPathsAroundDynamicObstacles == 0)
    return true;

  CalculatePathObstacles();

  return m_Path.AdjustPathAroundObstacles(m_pathAdjustmentObstacles, m_movementAbility.pathfindingProperties.navCapMask);
}

//===================================================================
// CalculatePathObstacles
//===================================================================
void CPuppet::CalculatePathObstacles()
{
  m_pathAdjustmentObstacles.CalculateObstaclesAroundPuppet(this);
}

//===================================================================
// SetCloseContact
//===================================================================
void CPuppet::SetCloseContact(bool bContact) 
{
	if(bContact && !m_bCloseContact)
		m_CloseContactTime = GetAISystem()->GetFrameStartTime();
	m_bCloseContact = bContact;
}

//===================================================================
// UpdateEntitiesToSkipInPathfinding
//===================================================================
void CPuppet::UpdateEntitiesToSkipInPathfinding()
{

}

//===================================================================
// GetShootingStatus
//===================================================================
void CPuppet::GetShootingStatus(SShootingStatus& ss)
{
	ss.fireMode = m_fireMode;
	ss.timeSinceTriggerPressed = m_timeSinceTriggerPressed;
	ss.triggerPressed = m_State.fire;
	ss.friendOnWay = m_friendOnWayElapsedTime > 0.001f;
	ss.friendOnWayElapsedTime = m_friendOnWayElapsedTime;
}

//===================================================================
// GetDistanceAlongPath
//===================================================================
float CPuppet::GetDistanceAlongPath(const Vec3& pos, bool bInit)
{
	Vec3 myPos(GetEntity() ? GetEntity()->GetPos() : GetPos());

	if(m_nPathDecision != PATHFINDER_PATHFOUND) 
		return 0;

	if(bInit)
	{
		m_InitialPath.clear();
		TPathPoints::const_iterator liend = m_OrigPath.GetPath().end();
		for(TPathPoints::const_iterator li =m_OrigPath.GetPath().begin();li!=liend;++li)
			m_InitialPath.push_back(li->vPos);

	}
	if(!m_InitialPath.empty())
	{
		float mindist = 10000000.f;
		float mindistObj = 10000000.f;
		TPointList::const_iterator listart =m_InitialPath.begin();
		TPointList::const_iterator li =listart;
		TPointList::const_iterator liend = m_InitialPath.end();
		TPointList::const_reverse_iterator lilast = m_InitialPath.rbegin();
		TPointList::const_iterator linext = li;
		TPointList::const_iterator liMyPoint = liend;
		TPointList::const_iterator liObjPoint = liend;
		Vec3 p1(ZERO);
		++linext;
		int count = 0;
		int myCount=0;
		int objCount = 0;
		float objCoeff=0;
		int maxCount = m_InitialPath.size()-1;
		for(;linext!=liend;++li,++linext)
		{
			float t,u;
			float distObj = Distance::Point_Lineseg(pos,Lineseg(*li,*linext),t);
			float mydist = Distance::Point_Lineseg(myPos,Lineseg(*li,*linext),u);
			if(distObj<mindistObj && (count==0 && t<0 || t>=0 && t<=1 || count==maxCount && t>1))
			{
				liObjPoint = li;
				mindistObj = distObj;
				objCount = count;
				objCoeff = t;
			}
			if(mydist<mindist && (count==0 && u<0 || u>=0 && u<=1 || count==maxCount && u>1))
			{
				liMyPoint = li;
				mindist = mydist;
				myCount = count;
			}
			count++;
		}
		
		
		// check if object is outside the path
		if(objCoeff<=0 && objCount==0)
			return Distance::Point_Point(pos,*listart) + Distance::Point_Point(myPos,*listart);
		else if(objCoeff>=1 && objCount>=count-1)
			return -(Distance::Point_Point(pos,*lilast) + Distance::Point_Point(myPos,*lilast));

		if(liMyPoint!=liend && liObjPoint != liend && liMyPoint!=liObjPoint)	
		{
			if(objCount>myCount)
			{
				// other object is ahead
				++liMyPoint;
				float dist = Distance::Point_Point(*liObjPoint,pos);
				if(liMyPoint!=liend)
					dist += Distance::Point_Point(*liMyPoint,myPos);

				li = liMyPoint;
				linext = li;
				++linext;

				for(;li!=liObjPoint && linext!=liend; ++li,++linext)
					dist += Distance::Point_Point(*li,*linext);

				return -dist;
			}
			else
			{
				// other object is back
				++liObjPoint;
				float dist = Distance::Point_Point(*liMyPoint,myPos);
				if(liObjPoint!=liend)
					dist += Distance::Point_Point(*liObjPoint,pos);

				li = liObjPoint;
				linext = li;
				++linext;

				for(;li!=liMyPoint && linext!=liend; ++li,++linext)
					dist += Distance::Point_Point(*li,*linext);

				return dist;
			}
		}
		// check just positions, object is on the same path segment
		Vec3 myOrientation(ZERO);
		if(GetPhysics())
		{
			pe_status_dynamics  dSt;
			GetPhysics()->GetStatus( &dSt );
			myOrientation = dSt.v;
		}
		if(myOrientation.IsEquivalent(ZERO))
			myOrientation = GetViewDir();

		Vec3 dir = pos-myPos;
		float dist= dir.GetLength();

		return(myOrientation.Dot(dir) <0  ? dist:-dist);

	}
	// no path
	return 0;
}

//===================================================================
// ClearPotentialTargets
//===================================================================
void CPuppet::ClearPotentialTargets() 
{
	if (m_pPerceptionHandler)
		m_pPerceptionHandler->ClearPotentialTargets();
}

//===================================================================
// GetPotentialTargets
//===================================================================
bool CPuppet::GetPotentialTargets(PotentialTargetMap &targetMap) const
{
	bool bResult = false;

	switch (gAIEnv.CVars.TargetTracking)
	{
		case 2: // Target tracks
		{
			CWeakRef<CAIObject> refBestTarget;
			SAIPotentialTarget* bestTargetEvent = 0;
			bool currentTargetErased = false;

			if (GetTargetTrackBestTarget(refBestTarget, bestTargetEvent, currentTargetErased) && bestTargetEvent)
			{
				targetMap.insert(PotentialTargetMap::value_type(refBestTarget, *bestTargetEvent));
				bResult = true;
			}
		}
		break;

		case 1: // Perception handlers
			if (m_pPerceptionHandler)
			{
				bResult = m_pPerceptionHandler->GetPotentialTargets(targetMap);
			}
			break;

		case 0: // Disabled
		default:
			bResult = false;
			break;
	}

	return bResult;
}

//===================================================================
// AddAggressiveTarget
//===================================================================
bool CPuppet::AddAggressiveTarget(IAIObject *pTarget)
{
	bool bResult = false;

	if (m_pPerceptionHandler)
	{
		CWeakRef<CAIObject> refTarget = GetWeakRefSafe((CAIObject*)pTarget);
		bResult = m_pPerceptionHandler->AddAggressiveTarget(refTarget);
	}

	return bResult;
}

//===================================================================
// SetTempTargetPriority
//===================================================================
bool CPuppet::SetTempTargetPriority(ETempTargetPriority priority)
{
	bool bResult = false;

	if (m_pPerceptionHandler)
		bResult = m_pPerceptionHandler->SetTempTargetPriority(priority);

	return bResult;
}

//===================================================================
// UpdateTempTarget
//===================================================================
bool CPuppet::UpdateTempTarget(const Vec3& vPosition)
{
	bool bResult = false;

	if (m_pPerceptionHandler)
		bResult = m_pPerceptionHandler->UpdateTempTarget(vPosition);

	return bResult;
}

//===================================================================
// ClearTempTarget
//===================================================================
bool CPuppet::ClearTempTarget()
{
	bool bResult = false;

	if (m_pPerceptionHandler)
		bResult = m_pPerceptionHandler->ClearTempTarget();

	return bResult;
}

//===================================================================
// DropTarget
//===================================================================
bool CPuppet::DropTarget(IAIObject *pTarget)
{
	bool bResult = false;

	if (m_pPerceptionHandler)
	{
		CWeakRef<CAIObject> refTarget = GetWeakRefSafe((CAIObject*)pTarget);
		bResult = m_pPerceptionHandler->DropTarget(refTarget);
	}

	return bResult;
}

// Description: 
//   Set the aproximation radius where speed starts to fall off (negative values mean no change)
// Arguments:
//   Walk/Run/Sprint speed fallouf radius
// Return:
//  
void CPuppet::SetSpeedFalloffRadius(float fWalk, float fRun, float fSprint)
{
  if (fWalk >= 0.0f)
  {
    m_distForWalk = fWalk;
  }
  if (fRun >= 0.0f)
  {
    m_distForRun = fRun;
  }
  if (fSprint >= 0.0f)
  {
    m_distForSprint = fSprint;
  }
}

// Description: 
//   Change flag so this puppet can be shoot or not
// Arguments:
//   bCanBeShot - If the puppet can be shoot or not
// Return:
//  
void CPuppet::SetCanBeShot(bool bCanBeShot)
{
  m_bCanBeShot = bCanBeShot;
}


// Description: 
//   Read flag if this puppet can be shoot or not
// Arguments:
// Return:
//   bool - If the puppet can be shoot or not
bool CPuppet::GetCanBeShot() const
{
  return ( m_bCanBeShot ) ;
}

//===================================================================
// SetMemoryFireType
//===================================================================
void CPuppet::SetMemoryFireType(EMemoryFireType eType)
{
	CRY_ASSERT(eType < eMFT_COUNT);
	m_eMemoryFireType = eType;
}

//===================================================================
// GetMemoryFireType
//===================================================================
EMemoryFireType CPuppet::GetMemoryFireType() const
{
	return m_eMemoryFireType;
}

//===================================================================
// CanMemoryFire
//===================================================================
bool CPuppet::CanMemoryFire() const
{
	bool bResult = true;

	if (m_targetLostTime > FLT_EPSILON)
	{
		switch (m_eMemoryFireType)
		{
			case eMFT_Disabled:
				bResult = false;
				break;

			case eMFT_UseCoverFireTime:
				{
					const float fCoverTime = GetCoverFireTime();
					bResult = (m_targetLostTime <= fCoverTime);
				}
				break;

			case eMFT_Always:
				bResult = true;
				break;

			default:
				CRY_ASSERT_MESSAGE(false, "Unhandled EMemoryFireType in CPuppet::CanMemoryFire()");
				break;
		}
	}

	return bResult;
}

//===================================================================
// SetTerritoryShapeName
//===================================================================
void CPuppet::SetTerritoryShapeName(const char* shapeName)
{
	assert(shapeName);

	if (m_territoryShapeName.compare(shapeName))
	{
		m_territoryShapeName = shapeName;
		
		if (m_territoryShapeName.compare("<None>"))
		{
			m_territoryShape = GetAISystem()->GetGenericShapeOfName(shapeName);	// i.e. m_territoryShapeName

			if (m_territoryShape)
			{
				// Territory shapes should be really simple
				size_t size = m_territoryShape->shape.size();
				if (size > 8)
				{
					AIWarning("Territory shape %s for %s has %d points.  Territories should not have more than 8 points",
						shapeName, GetName(), size);
				}
			}
			else
			{
				m_territoryShapeName += " (not found)";
				m_territoryShape = 0;
			}
		}
		else
		{
			m_territoryShape = 0;
		}
	}
}

//===================================================================
// GetTerritoryShapeName
//===================================================================
const char* CPuppet::GetTerritoryShapeName() const
{
	return (gEnv->bEditor && !gEnv->bEditorGameMode)
		? m_Parameters.m_sTerritoryName.c_str()
		: m_territoryShapeName.c_str();
}

//===================================================================
// IsPointInsideTerritoryShape
//===================================================================
bool CPuppet::IsPointInsideTerritoryShape(const Vec3& vPos, bool bCheckHeight) const
{
	bool bResult = true;

	const SShape *pTerritory = GetTerritoryShape();
	if (pTerritory)
	{
		bResult = pTerritory->IsPointInsideShape(vPos, bCheckHeight);
	}

	return bResult;
}

//===================================================================
// ConstrainInsideTerritoryShape
//===================================================================
bool CPuppet::ConstrainInsideTerritoryShape(Vec3& vPos, bool bCheckHeight) const
{
	bool bResult = true;

	const SShape *pTerritory = GetTerritoryShape();
	if (pTerritory)
	{
		bResult = pTerritory->ConstrainPointInsideShape(vPos, bCheckHeight);
	}

	return bResult;
}

//===================================================================
// GetSoundPerceptionDescriptor
//===================================================================
bool CPuppet::GetSoundPerceptionDescriptor(EAISoundStimType eType, SSoundPerceptionDescriptor& sDescriptor) const
{
	CRY_ASSERT(eType >= 0 && eType < AISOUND_LAST);
	bool bResult = false;

	if (eType >= 0 && eType < AISOUND_LAST)
	{
		sDescriptor = m_SoundPerceptionDescriptor[eType];
		bResult = true;
	}

	return bResult;
}

//===================================================================
// SetSoundPerceptionDescriptor
//===================================================================
bool CPuppet::SetSoundPerceptionDescriptor(EAISoundStimType eType, const SSoundPerceptionDescriptor& sDescriptor)
{
	CRY_ASSERT(eType >= 0 && eType < AISOUND_LAST);
	bool bResult = false;

	if (eType >= 0 && eType < AISOUND_LAST)
	{
		m_SoundPerceptionDescriptor[eType] = sDescriptor;
		bResult = true;
	}

	return bResult;
}

//===================================================================
// GetSightFOV
//===================================================================
void CPuppet::GetSightFOV(float &fPrimaryFOV, float &fSecondaryFOV) const
{
	fPrimaryFOV = m_FOVPrimaryCos;
	fSecondaryFOV = m_FOVSecondaryCos;
}

//===================================================================
// GetDamageParts
//===================================================================
DamagePartVector* CPuppet::GetDamageParts()
{
	if (!m_damagePartsUpdated)
	{
		UpdateDamageParts(m_damageParts);
		m_damagePartsUpdated = true;
	}

	return &m_damageParts;
}

//===================================================================
// GetValidPositionNearby
//===================================================================
bool CPuppet::GetValidPositionNearby(const Vec3 &proposedPosition, Vec3 &adjustedPosition) const
{
  adjustedPosition = proposedPosition;
  if (!GetFloorPos(adjustedPosition, proposedPosition, 1.0f, 2.0f, walkabilityDownRadius, AICE_ALL))
    return false;

  static float maxFloorDeviation = 1.0f;
  if (fabsf(adjustedPosition.z - proposedPosition.z) > maxFloorDeviation)
    return false;

  if (!CheckBodyPos(adjustedPosition, AICE_ALL))
    return false;

  unsigned nodeIndex = gAIEnv.pGraph->GetEnclosing(
    adjustedPosition, m_movementAbility.pathfindingProperties.navCapMask, m_Parameters.m_fPassRadius,
		m_lastNavNodeIndex);

  return nodeIndex != 0;
}

//===================================================================
// GetTeleportPosition
//===================================================================
bool CPuppet::GetTeleportPosition(Vec3 &teleportPos) const
{
  Vec3 curPos = GetPos();
  teleportPos.zero();

  IAISystem::tNavCapMask navCapMask = m_movementAbility.pathfindingProperties.navCapMask;
  navCapMask &= IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN;

  int nBuildingID;
  IVisArea *pArea;
  IAISystem::ENavigationType currentNavType = gAIEnv.pNavigation->CheckNavigationType(curPos, nBuildingID, pArea, navCapMask);

  gAIEnv.pNavigation->GetNavRegion(currentNavType, gAIEnv.pGraph)->GetTeleportPosition(curPos, teleportPos, GetName());

  if (!teleportPos.IsZero())
    return true;
  else
    return false;
}

void CPuppet::ResetPostures()
{
	m_postureInfos.clear();
}

void CPuppet::SetDefaultHidePostures()
{
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_HIDE, "HideProne", 0.0f, 0.0f, STANCE_PRONE, 5.0f));
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_HIDE, "HideCrouch", 0.0f, 0.0f, STANCE_CROUCH, 3.0f));
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_HIDE, "HideStealth", 0.0f, 0.0f, STANCE_STEALTH, 2.0f));
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_HIDE, "HideStand", 0.0f, 0.0f, STANCE_STAND, 1.0f));
}

void CPuppet::SetDefaultAimPostures()
{
	int crouchId = -1;
	int standId = -1;
	int coverId = -1;

	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimProne", 0.0f, 0.0f, STANCE_PRONE, 5.0f));
	crouchId = SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimCrouch", 0.0f, 0.0f, STANCE_CROUCH, 4.0f));

	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimCrouchRightLean", 1.0f, 0.0f, STANCE_CROUCH, 3.0f, "peekRight", crouchId));
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimCrouchLeftLean", -1.0f, 0.0f, STANCE_CROUCH, 3.0f, "peekLeft", crouchId));

	standId = SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimStand", 0.0f, 0.0f, STANCE_STAND, 2.0f));
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimStandRightLean", 1.0f, 0.0f, STANCE_STAND, 1.0f, "peekRight", standId));
	SetPosture(IPuppet::SPostureInfo(IPuppet::POSTURE_AIM, "AimStandLeftLean", -1.0f, 0.0f, STANCE_STAND, 1.0f, "peekLeft", standId));
}

int CPuppet::SetPosture(const IPuppet::SPostureInfo& posture)
{
	m_postureInfos.push_back(posture);

	return m_postureInfos.size()-1;
}

bool CPuppet::GetPosture(int postureId, IPuppet::SPostureInfo* posture) const
{
	if ((postureId < 0) || (postureId >= m_postureInfos.size()))
		return false;

	if (posture)
		*posture = m_postureInfos[postureId];

	return true;
}

int CPuppet::GetPostureId(const char* postureName) const
{
	for (int i = 0; i < m_postureInfos.size(); ++i)
	{
		if (m_postureInfos[i].name == postureName)
			return i;
	}

	return -1;
}

bool CPuppet::GetPostureByName(const char* postureName, IPuppet::SPostureInfo* posture) const
{
	for (int i = 0; i < m_postureInfos.size(); ++i)
	{
		const IPuppet::SPostureInfo& info = m_postureInfos[i];
		if (info.name == postureName)
		{
			if (posture)
				*posture = info;
			return true;
		}
	}

	return false;
}

void CPuppet::SetPosturePriority(int postureId, float priority)
{
	if ((postureId < 0) || (postureId >= m_postureInfos.size()))
		return;

	m_postureInfos[postureId].priority = priority;
}

float CPuppet::GetPosturePriority(int postureId) const
{
	if ((postureId < 0) || (postureId >= m_postureInfos.size()))
		return 0.0f;

	const IPuppet::SPostureInfo& postureInfo = m_postureInfos[postureId];

	return GetPosturePriority(postureInfo.parentId) + postureInfo.priority;
}

//===================================================================
// SelectAimPosture
//===================================================================
bool CPuppet::SelectAimPosture(SPostureInfo* postureOut, const Vec3& targetPos, uint32 checks, bool allowLean, bool allowProne)
{
	return SelectAimPostureAt(postureOut, GetPhysicsPos(), targetPos, checks, allowLean, allowProne);
}

//===================================================================
// SelectAimPostureAt
//===================================================================
bool CPuppet::SelectAimPostureAt(SPostureInfo* postureOut, const Vec3& pos, const Vec3& targetPos, uint32 checks, bool allowLean, bool allowProne, float rayDistanceFraction)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	m_runningPostures.resize(0);
	for (int i = 0; i < m_postureInfos.size(); ++i)
	{
		const SPostureInfo& postureInfo = m_postureInfos[i];

		if ((postureInfo.type == POSTURE_AIM) &&
			(allowLean || (cry_fabsf(postureInfo.lean) < 0.01f)) &&
			(allowProne || (postureInfo.stance != STANCE_PRONE)))
		{
			m_runningPostures.push_back(SRunningPosture(i));
		}
	}

	for (RunningPostures::iterator itPosture = m_runningPostures.begin(); itPosture != m_runningPostures.end(); ++itPosture)
	{
		SRunningPosture &runningPosture = *itPosture;
		int a = 2;
	}

	const bool stickyStance = true;

	if (stickyStance)
	{
		std::sort(m_runningPostures.begin(), m_runningPostures.end(), 
			SStickyStancePostureSorter(this, m_postureInfos, (EStance)m_State.bodystate));
	}
	else
	{
		std::sort(m_runningPostures.begin(), m_runningPostures.end(), SPostureSorter(this));
	}

	int	targetVis = -1;
	int	targetVisAim = -1;

	float	distToTarget = Distance::Point_Point(targetPos, pos);

	bool isInDoors = false;
	if(m_lastNavNodeIndex && (gAIEnv.pGraph->GetNodeManager().GetNode(m_lastNavNodeIndex)->navType & IAISystem::NAV_WAYPOINT_HUMAN))
		isInDoors = true;

	std::vector<IPhysicalEntity*> skipList;
	GetPhysicsEntitiesToSkip(skipList);

	// Update the postures.
	int postureCount = (int)m_runningPostures.size();
	for(int i = 0; i < postureCount; ++i)
	{
		SRunningPosture& posture = m_runningPostures[i];
		const SPostureInfo& info = m_postureInfos[posture.postureId];

		if (info.stance == STANCE_PRONE)
		{
			if (isInDoors)
				continue;

			float checkDist = allowProne ? 50.0f : 8.0f;
			if (distToTarget < checkDist)
				continue;
		}

		if(info.stance == STANCE_CROUCH && distToTarget < 5.0f)
			continue;

		if(info.stance == STANCE_STEALTH && distToTarget < 3.5f)
			continue;

		SAIBodyInfo bodyInfo;
		bool canContinue = true;

		if (!posture.processed)
		{
		 if (canContinue = GetProxy()->QueryBodyInfo(
			 SAIBodyInfoQuery(pos, targetPos, info.stance, info.lean, info.peekOver, false), bodyInfo))
		 {
				posture.eye = bodyInfo.vEyePos;
				posture.weapon = bodyInfo.vFirePos;
				posture.processed = true;
		 }
		}

		canContinue &= info.enabled;

		if (canContinue)
		{
			posture.targetVis = true;
			posture.targetAim = true;

			Vec3 dir = targetPos - posture.weapon;

			// Try to avoid humans.
			if (checks & CheckPostureFriendlyFire)
			{
				Ray	fireRay(posture.weapon, dir);

				AutoAIObjectIter it(GetAISystem()->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET));
				for ( ; it->GetObject(); it->Next())
				{
					IAIObject* pObject = it->GetObject();
					if (!pObject->IsEnabled())
						continue;

					CPuppet* pPuppet = pObject->CastToCPuppet();
					if (!pPuppet)
						continue;

					if (pPuppet == this)
						continue;

					if (pPuppet->IsHostile(this))
						continue;

					Vec3 puppetPos = pPuppet->GetPos();

					if(Distance::Point_PointSq(pos, puppetPos) > sqr(7.0f))
						continue;

					if(Distance::Point_PointSq(pos, puppetPos) > sqr(7.0f))
						continue;

					Vec3 puppetPhysPos = pPuppet->GetPhysicsPos();

					// Hit test against the collider (not anim skeleton).
					ray_hit	hit2;
					if (gAIEnv.pWorld->CollideEntityWithBeam(pPuppet->GetPhysics(false),
						posture.weapon, dir, 0.05f, &hit2))
					{
						posture.targetAim = false;
						break;
					}

					// Additional check for the head, in case of leaning.
					if (fabsf(pPuppet->GetState()->lean) > 0.01f)
					{
						// The reason using the weapon position here is that it is close to the body and 
						// better represents the leaned out upper body.
						Vec3	toeToHead = puppetPos - puppetPhysPos;
						float	len = toeToHead.GetLength();
						if(len > 0.0001f)
							toeToHead *= (len - 0.3f) / len;

						Vec3 dummy;
						if(Intersect::Ray_Sphere(fireRay, Sphere(puppetPhysPos + toeToHead, 0.4f + 0.05f), dummy, dummy))
						{
							posture.targetAim = false;
							break;
						}
					}
				}
			}

			ray_hit hit;

			if (checks & CheckPostureLeanability)
			{
				// Check if the path from the non-lean state to the lean state is clear.
				if(info.parentId != -1)
				{
					bool pathClear = true;

					for (int p = 0; p < postureCount; ++p)
					{
						SRunningPosture& parentPosture = m_runningPostures[p]; 

						if (parentPosture.postureId == info.parentId)
						{
							const SPostureInfo& parentInfo = m_postureInfos[parentPosture.postureId];

							if (!parentPosture.processed)
							{
								SAIBodyInfo parentBodyInfo;

								if (GetProxy()->QueryBodyInfo(
									SAIBodyInfoQuery(pos, targetPos, parentInfo.stance, parentInfo.lean, parentInfo.peekOver, false), 
									parentBodyInfo))
								{
									parentPosture.eye = parentBodyInfo.vEyePos;
									parentPosture.weapon = parentBodyInfo.vFirePos;
									parentPosture.processed = true;
								}
							}

							if (gAIEnv.pRayCaster->Cast(RayCastRequest(parentPosture.eye, parentPosture.eye - posture.eye,
								COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER, skipList.empty() ? 0 : &skipList[0], skipList.size())))
							{
								pathClear = false;
							}
							break;
						}
					}

					if (!pathClear)
						continue;

					// TODO: Do the simple sphere test as above too with friends.
				}
			}

			if (checks & CheckPostureVisibility)
			{
				// Check if the aiming and looking is obstructed from the changed stance.
				// checking just half the distance - not to shoot long rays
				dir = targetPos - posture.eye;
				if (gAIEnv.pRayCaster->Cast(RayCastRequest(posture.eye, dir * rayDistanceFraction,
					COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER, skipList.empty() ? 0 : &skipList[0], skipList.size())))
					posture.targetVis = false;
			}


			if (checks & CheckPostureAimability)
			{
				dir = targetPos - posture.weapon;
				// checking just half the distance - not to shoot long rays
				if (gAIEnv.pRayCaster->Cast(RayCastRequest(posture.weapon, dir * 0.95f,
					COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER, skipList.empty() ? 0 : &skipList[0], skipList.size())))
					posture.targetAim = false;
			}

			bool drawGoal = gAIEnv.CVars.DrawGoals != 0;
			if(drawGoal)
			{
				CDebugDrawContext dc;

				Vec3 debugDir = targetPos - posture.eye;
				ColorB eyec(230, 120, 40, 255);
				dc->DrawSphere(posture.eye, 0.05f, eyec, true);
				dc->DrawLine(posture.eye, eyec, posture.eye+debugDir*0.5f, eyec);

				debugDir = targetPos - posture.weapon;
				ColorB weaponc(40, 120, 250, 255);
				dc->DrawSphere(posture.weapon, 0.05f, weaponc, true);
				dc->DrawLine(posture.weapon, weaponc, posture.weapon+debugDir*0.5f, weaponc);
			}


			if ((checks & CheckPostureAimability) && (checks & CheckPostureVisibility) && (!posture.targetVis || !posture.targetAim))
				continue;
			else if ((checks & CheckPostureAimability) && !posture.targetAim)
				continue;
			else if ((checks & CheckPostureVisibility) && !posture.targetVis)
				continue;

			if (postureOut)
				*postureOut = m_postureInfos[m_runningPostures[i].postureId];

			return true;
		}
	}

	return false;
}

//===================================================================
// SelectHidePosture
//===================================================================
bool CPuppet::SelectHidePosture(SPostureInfo* postureOut, const Vec3& targetPos, bool allowLean, bool allowProne)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	m_runningPostures.resize(0);
	for (int i = 0; i < m_postureInfos.size(); ++i)
	{
		const SPostureInfo& postureInfo = m_postureInfos[i];

		if ((postureInfo.type == POSTURE_HIDE) &&
			(allowLean || (cry_fabsf(postureInfo.lean) < 0.01f)) &&
			(allowProne || (postureInfo.stance != STANCE_PRONE)))
		{
			m_runningPostures.push_back(SRunningPosture(i));		
		}
	}

	std::sort(m_runningPostures.begin(), m_runningPostures.end(), SPostureSorter(this));

	const Vec3& pos = GetPos();

	int	targetVis = -1;
	int	targetVisAim = -1;

	float	distToTarget = Distance::Point_Point(targetPos, GetPos());

	// Update the postures.
	int postureCount = (int)m_runningPostures.size();
	for(int i = 0; i < postureCount; ++i)
	{
		SRunningPosture& posture = m_runningPostures[i];
		const SPostureInfo& info = m_postureInfos[posture.postureId];

		if(info.stance == STANCE_PRONE && distToTarget < 25.0f)
			continue;

		if(info.stance == STANCE_CROUCH && distToTarget < 5.0f)
			continue;

		if(info.stance == STANCE_STEALTH && distToTarget < 3.5f)
			continue;

		SAIBodyInfo bodyInfo;
		bool canContinue = true;

		if (!posture.processed)
		{
			if (canContinue = GetProxy()->QueryBodyInfo(
				SAIBodyInfoQuery(pos, targetPos, info.stance, info.lean, info.peekOver, false), bodyInfo))
		 {
			 posture.eye = bodyInfo.vEyePos;
			 posture.weapon = bodyInfo.vFirePos;
			 posture.processed = true;
		 }
		}

		canContinue &= info.enabled;

		if (canContinue)
		{
			posture.eye = bodyInfo.vEyePos;
			posture.weapon = bodyInfo.vFirePos;

			posture.targetVis = true;
			posture.targetAim = true;

			if(info.parentId != -1)
			{
				bool pathClear = true;

				for (int p = 0; p < postureCount; ++p)
				{
					SRunningPosture& parentPosture = m_runningPostures[p]; 

					if (parentPosture.postureId == info.parentId)
					{
						const SPostureInfo& parentInfo = m_postureInfos[parentPosture.postureId];

						if (!parentPosture.processed)
						{
							SAIBodyInfo parentBodyInfo;

							if (GetProxy()->QueryBodyInfo(
								SAIBodyInfoQuery(pos, targetPos, parentInfo.stance, parentInfo.lean, parentInfo.peekOver, false), 
								parentBodyInfo))
							{
								parentPosture.eye = parentBodyInfo.vEyePos;
								parentPosture.weapon = parentBodyInfo.vFirePos;
								parentPosture.processed = true;
							}
						}

						if (gAIEnv.pRayCaster->Cast(RayCastRequest(parentPosture.eye, parentPosture.eye - posture.eye,
							COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER)))
						{
							pathClear = false;
						}
						break;
					}
				}

				if (!pathClear)
					continue;

				// TODO: Do the simple sphere test as above too with friends.
			}

			Vec3 dir = targetPos - posture.eye;

			// Check if the aiming and looking is obstructed from the changed stance.
			// checking just half the distance - not to shoot long rays
			if (gAIEnv.pRayCaster->Cast(RayCastRequest(posture.eye, dir * 0.5f, COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER)))
				continue;

			dir = targetPos - posture.weapon;
			// checking just half the distance - not to shoot long rays
			if (gAIEnv.pRayCaster->Cast(RayCastRequest(posture.weapon, dir * 0.5f, 
				COVER_OBJECT_TYPES, HIT_COVER|HIT_SOFT_COVER)))
				posture.targetAim = false;

			// Choose if the target is good to use.
			if(targetVisAim == -1 && !posture.targetVis && !posture.targetAim)
			{
				targetVisAim = i;
				break;
			}

			if(targetVis == -1 && !posture.targetVis && fabsf(info.lean) < 0.01f)
				targetVis = i;
		}
	}

	if (targetVisAim != -1)
	{
		if (postureOut)
			*postureOut = m_postureInfos[m_runningPostures[targetVisAim].postureId];

		return true;
	}
	else if (targetVis != -1)
	{
		if (postureOut)
			*postureOut = m_postureInfos[m_runningPostures[targetVis].postureId];

		return true;
	}

	return false;
}

void CPuppet::EnableFire(bool enable)
{
	m_fireDisabled += enable ? -1 : 1;

	assert(m_fireDisabled <= 8);
}

bool CPuppet::IsFireEnabled() const
{
	return m_fireDisabled == 0;
}


//===================================================================
// SetAllowedStrafeDistances
//===================================================================
void CPuppet::SetAllowedStrafeDistances(float start, float end, bool whileMoving)
{
	m_allowedStrafeDistanceStart = start; 
	m_allowedStrafeDistanceEnd = end;
	m_allowStrafeLookWhileMoving = whileMoving;

	m_strafeStartDistance = 0.0f;

	UpdateStrafing();
}

//===================================================================
// SetAdaptiveMovementUrgency
//===================================================================
void CPuppet::UpdateStrafing()
{
	m_State.allowStrafing = false;
	if (m_State.fDistanceToPathEnd > 0)
	{
//		if (!m_State.vAimTargetPos().IsZero())
		{
			if (m_allowedStrafeDistanceStart > 0.001f)
			{
				// Calculate the max travelled distance.
				float	distanceMoved = m_OrigPath.GetPathLength(false) - m_State.fDistanceToPathEnd;
				m_strafeStartDistance = max(m_strafeStartDistance, distanceMoved);

				if (m_strafeStartDistance < m_allowedStrafeDistanceStart)
					m_State.allowStrafing = true;
			}

			if (m_allowedStrafeDistanceEnd > 0.001f)
			{
				if (m_State.fDistanceToPathEnd < m_allowedStrafeDistanceEnd)
					m_State.allowStrafing = true;
			}
		}
	}
}

//===================================================================
// SetAdaptiveMovementUrgency
//===================================================================
void CPuppet::SetAdaptiveMovementUrgency(float minUrgency, float maxUrgency, float scaleDownPathlen)
{
	m_adaptiveUrgencyMin = minUrgency;
	m_adaptiveUrgencyMax = maxUrgency;
	m_adaptiveUrgencyScaleDownPathLen = scaleDownPathlen;
	m_adaptiveUrgencyMaxPathLen = 0.0f;
}

//===================================================================
// SetDelayedStance
//===================================================================
void CPuppet::SetDelayedStance(int stance)
{
	m_delayedStance = stance;
	m_delayedStanceMovementCounter = 0;
}

//===================================================================
// GetPhysics
//===================================================================
IPhysicalEntity* CPuppet::GetPhysics(bool wantCharacterPhysics) const
{
	if(GetProxy() && GetProxy()->GetPhysics(wantCharacterPhysics))
		return GetProxy()->GetPhysics(wantCharacterPhysics);

//	AIWarning("CPuppet::GetPhysics Puppet %s does not have physics!", GetName());
//	AIAssert(0);

	return NULL;
}

//===================================================================
// GetPosAlongPath
//===================================================================
bool CPuppet::GetPosAlongPath(float dist, bool extrapolateBeyond, Vec3& retPos) const
{
	return m_Path.GetPosAlongPath(retPos, dist, !m_movementAbility.b3DMove, extrapolateBeyond);
}

//===================================================================
// CheckCloseContact
//===================================================================
void CPuppet::CheckCloseContact(IAIObject* pTarget, float distSq) 
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	if (GetAttentionTarget() == pTarget && distSq < sqr(GetParameters().m_fMeleeRange) && !m_bCloseContact)
	{
		SetSignal(1, "OnCloseContact", pTarget->GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnCloseContact);		
		SetCloseContact(true);
	}
}

//===================================================================
// GetLiveTarget
//===================================================================
CWeakRef<CAIActor> CPuppet::GetLiveTarget(const CWeakRef<CAIObject> refTarget) const
{
	// (MATT) I think this coudl be static :/ {2009/02/11}
	CCCPOINT(CPuppet_GetLiveTarget);

	CWeakRef<CAIActor> refResult;
	CAIObject *pTarget = refTarget.GetAIObject();
	if (pTarget)
	{
		CAIActor* pActor = pTarget->CastToCAIActor();
		if (pActor && pActor->IsActive() && pActor->IsAgent())
			refResult = StaticCast<CAIActor>(refTarget);
		else
		{
			CAIActor* pAssociatedActor = CastToCAIActorSafe( pTarget->GetAssociation().GetAIObject() );
			if (pAssociatedActor && pAssociatedActor->IsEnabled() && pAssociatedActor->IsAgent())
			{
				refResult = GetWeakRef( pAssociatedActor );
			}
		}
	}
	return refResult;
}

void CPuppet::RegisterVision()
{
	if (m_visionID)
		return;

	IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();
	IEntity* pEntity = GetEntity();
	IAIObject* pAI = pEntity->GetAI();

	m_visionID = visionMap.CreateVisionID(pEntity->GetName());

	ObserverParams observerParams;
	observerParams.entityID = GetEntityID();
	//assert(pAI->GetSpecies() < 32);
	observerParams.factionMask = 0xFFFFFFFF; //~(1 << pAI->GetSpecies());
	observerParams.typeMask = 0xFFFFFFFF; // only looks for deadies
	observerParams.eyePos = GetPos();
	observerParams.eyeDir = pAI->GetBodyDir(); // view dir?

	float primaryFOV = 60.0f;
	float secondaryFOV = 120.0f;
	GetSightFOV(primaryFOV, secondaryFOV);
	observerParams.primaryFoVCos = primaryFOV;
	observerParams.peripheralFoVCos = secondaryFOV;

	visionMap.RegisterObserver(m_visionID, observerParams);

	ObservableParams observableParams;
	observableParams.faction = pAI->GetSpecies();
	observableParams.type = 0;
	observableParams.posCount = 1;
	observableParams.pos[0] = GetPos();

	visionMap.RegisterObservable(m_visionID, observableParams);
}

void CPuppet::UnregisterVision()
{
	if (!m_visionID)
		return;

	IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();

	visionMap.UnregisterObserver(m_visionID);
	visionMap.UnregisterObservable(m_visionID);
	m_visionID = VisionID();
}

void CPuppet::UpdateVision()
{
	if (m_visionID)
	{
		assert(m_visionID);
		IVisionMap& visionMap = gEnv->pAISystem->GetVisionMap();

		const Vec3& pos = GetPos();
		const Vec3& dir = GetEntity()->GetAI()->GetBodyDir();

		ObserverParams observerParams;
		observerParams.eyePos = pos;
		observerParams.eyeDir = dir;
		visionMap.ObserverChanged(m_visionID, observerParams,
			eChangedPosition | eChangedOrientation);

		ObservableParams observableParams;
		observableParams.posCount = 1;
		observableParams.pos[0] = pos;
		visionMap.ObservableChanged(m_visionID, observableParams,
			eChangedPosition);
	}
	else
	{
		RegisterVision();
	}
}

bool CPuppet::CanSee(const VisionID& otherVisionID) const
{
	return gEnv->pAISystem->GetVisionMap().IsVisible(this->GetVisionID(), otherVisionID);
}
