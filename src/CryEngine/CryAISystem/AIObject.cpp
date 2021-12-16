// AIObject.cpp: implementation of the CAIObject class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AIObject.h"
#include "CAISystem.h"
#include "AILog.h"
#include "Graph.h"
#include "Leader.h"
#include "ObjectTracker.h"
#include "GoalOp.h"

#include <float.h>
#include <ISystem.h>
#include <ILog.h>
#include <ISerialize.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



#define _ser_value_(val) ser.Value(#val, val)

CAIObject::CAIObject():
m_vPosition(ZERO),
m_entityID(0),
m_bEnabled(true),
m_lastNavNodeIndex(0),
m_bLastNearForbiddenEdge(false),
m_fRadius(.0f),
m_bCanReceiveSignals(true),
m_pFormation(0),
m_nObjectType(0),
m_objectSubType(CAIObject::STP_NONE),
m_bUpdatedOnce(false),
m_bUncheckedBody(false),
m_vFirePosition(ZERO),
m_vFireDir(ZERO),
m_vBodyDir(ZERO),
m_vMoveDir(ZERO),
m_vView(ZERO),
m_vLastPosition(ZERO),
m_bLastActionSucceed(false),
m_groupId(-1),
m_species(-1),
m_bTouched(false)
{
	AILogComment("CAIObject (%p)", this);
}

CAIObject::~CAIObject()
{
	AILogComment("~CAIObject  %s (%p)", GetName(), this);

	ReleaseFormation();
}

void CAIObject::SetPos(const Vec3& pos, const Vec3& dirForw)
{
	CCCPOINT(CAIObject_SetPos);

	if (_isnan(pos.x) || _isnan(pos.y) || _isnan(pos.z))
	{
		AIWarning("NotANumber tried to be set for position of AI entity %s", GetName());
		return;
	}

	m_vLastPosition = m_vPosition;

	assert( m_vLastPosition.IsValid() );

	IAIActorProxy* pProxy = GetProxy();
	if (pProxy)
	{
		SAIBodyInfo bodyInfo;
		pProxy->QueryBodyInfo( bodyInfo );

		assert( bodyInfo.vEyeDir. IsValid() );
		assert( bodyInfo.vEyePos. IsValid() );
		assert( bodyInfo.vEyeDir. IsValid() );
		assert( bodyInfo.vFirePos.IsValid() );

		m_vPosition = bodyInfo.vEyePos;
		assert( m_vPosition.IsValid() );

		m_vFirePosition = bodyInfo.vFirePos;

		m_vFireDir = bodyInfo.vFireDir;

		SetMoveDir(bodyInfo.vMoveDir);
		SetBodyDir(bodyInfo.vBodyDir);
		SetViewDir(bodyInfo.vEyeDir);
	}
	else
	{
		m_vPosition = pos;
		assert( m_vPosition.IsValid() );

		m_vMoveDir = dirForw;
		m_vBodyDir = dirForw;
		SetViewDir(dirForw);
	}

	if (!IsEquivalent(m_vLastPosition,pos, VEC_EPSILON))
	{
		// notify smart objects, too
		IEntity* pEntity = GetEntity();
		if (pEntity)
		{
			GetAISystem()->NotifyAIObjectMoved(pEntity, SEntityEvent(ENTITY_EVENT_XFORM));
		}
		if (m_nObjectType == AIANCHOR_COMBAT_HIDESPOT || m_nObjectType == AIANCHOR_COMBAT_HIDESPOT_SECONDARY)
		{
			m_lastNavNodeIndex = 0;
		}
	}

	if (m_pFormation)
	{
		m_pFormation->Update();
	}
}

Vec3 CAIObject::GetPhysicsPos() const
{
	IEntity* pIEntity = GetEntity();
	if ( pIEntity )
		return pIEntity->GetWorldPos();

	return GetPos();
}

void CAIObject::SetType(unsigned short type)
{
	m_nObjectType = type;
}

void CAIObject::SetSubType(CAIObject::ESubTypes subType)
{
	m_objectSubType = subType;
}

void CAIObject::SetAssociation(CWeakRef<CAIObject> refAssociation)
{
	m_refAssociation = refAssociation;
}

const char *  CAIObject::GetName() const
{
	return m_name.c_str();
}

void CAIObject::SetName(const char *pName)
{
	m_name = pName;
}

void CAIObject::SetBodyDir(const Vec3 &dir)
{
	m_vBodyDir = dir;
	//FIXME: The direction sent from game should be already normalized!
	m_vBodyDir.NormalizeSafe();
	//if (m_pFormation)
	//	m_pFormation->Update(this);
}

void CAIObject::SetMoveDir(const Vec3 &dir)
{
	m_vMoveDir = dir;
	//FIXME: The direction sent from game should be already normalized!
	m_vMoveDir.NormalizeSafe();
	//if (m_pFormation)
	//	m_pFormation->Update(this);
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIObject::SetViewDir(const Vec3 &dir)
{
	m_vView = dir;

	//FIXME: The direction sent from game should be already normalized!
	m_vView.NormalizeSafe();

	//if (m_pFormation)
	//	m_pFormation->Update(this);
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIObject::Reset(EObjectResetType type)
{
	m_lastNavNodeIndex = 0;
	m_bLastNearForbiddenEdge = false;

	ReleaseFormation();

	m_bEnabled = true;
	m_bUncheckedBody = false;
	m_bUpdatedOnce = false;
	m_bCanReceiveSignals = true;
	m_bTouched = false;
}



//
//
//------------------------------------------------------------------------------------------------------------------------
void CAIObject::SetRadius(float fRadius)
{
	m_fRadius = fRadius;
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIObject::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	ser.Value("m_refThis", m_refThis);
	ser.Value("m_bEnabled", m_bEnabled);
	ser.Value("m_bTouched", m_bTouched);
	ser.Value("m_bUncheckedBody", m_bUncheckedBody);
	ser.Value("m_bCanReceiveSignals", m_bCanReceiveSignals);
	// Do not cache the result of GetAnchorNavNode across serialisation
	if (m_lastNavNodeIndex == ~0)
		m_lastNavNodeIndex = 0;
	gAIEnv.pGraph->SerializeNodePointer(ser, "m_pLastNavNode", m_lastNavNodeIndex);
	ser.Value("m_bLastNearForbiddenEdge", m_bLastNearForbiddenEdge);
	ser.Value("m_vLastPosition", m_vLastPosition);
	objectTracker.SerializeObjectPointer(ser, "Formation", m_pFormation, false);

	// m_movementAbility.Serialize(ser); // not needed as ability is constant
	ser.Value("m_bUpdatedOnce", m_bUpdatedOnce);
	ser.Value("m_bLastActionSucceed", m_bLastActionSucceed);
	// todo m_listWaitGoalOps
	ser.Value("m_nObjectType",m_nObjectType);
	ser.EnumValue("m_objectSubType", m_objectSubType, STP_NONE, STP_MAXVALUE);
	ser.Value("m_vPosition",m_vPosition);
	ser.Value("m_vFirePosition",m_vFirePosition);
	ser.Value("m_vFireDir",m_vFireDir);
	ser.Value("m_vBodyDir",m_vBodyDir);
	ser.Value("m_vMoveDir",m_vMoveDir);
	ser.Value("m_vView",m_vView);
	// todo m_pAssociation
	ser.Value("m_fRadius",m_fRadius);
	// Danny todo m_entitiesToSkipInPathFinding
	ser.Value("m_groupId",m_groupId);
	m_refAssociation.Serialize(ser, "m_refAssociation");
	// m_listWaitGoalOps is not serialized but recreated after serializing goal pipe, when reading, in CPipeUser::Serialize()
	ser.Value("m_entityID", m_entityID);
	ser.Value("m_name", m_name);

	return false;
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIObject::CreateFormation(const char * szName, Vec3 vTargetPos)
{
	if (m_pFormation)
	{
		GetAISystem()->ReleaseFormation(GetWeakRef(this),true);
	}

	m_pFormation = 0;

	if (!szName)
		return false;

	CCCPOINT(CAIObject_CreateFormation);

	m_pFormation = GetAISystem()->CreateFormation(GetWeakRef(this),szName,vTargetPos);
	return (m_pFormation!= NULL);
}

//
//------------------------------------------------------------------------------------------------------------------------
bool CAIObject::ReleaseFormation(void)
{
	if(m_pFormation)
	{
		CCCPOINT(CAIObject_ReleaseFormation);
		GetAISystem()->ReleaseFormation(GetWeakRef(this),true);
		m_pFormation = 0;
		return true;
	}
	return false;
}

//
//------------------------------------------------------------------------------------------------------------------------
Vec3	CAIObject::GetVelocity() const
{
	IAIActorProxy* pProxy = GetProxy();
	if ( !pProxy )
		return ZERO;

	IPhysicalEntity* pPhysicalEntity = pProxy->GetPhysics();
	if ( !pPhysicalEntity )
		return ZERO;

	CCCPOINT(CAIObject_GetVelocity);

	// if living entity return that vel since that is actualy the rate of
	// change of position
	pe_status_living status;
	if (pPhysicalEntity->GetStatus(&status))
		return status.vel;

	pe_status_dynamics  dSt;
	pPhysicalEntity->GetStatus(&dSt);

	return dSt.v;

}

//
//------------------------------------------------------------------------------------------------------------------------
const char* CAIObject::GetEventName(unsigned short eType) const
{
	switch (eType)
	{
	case AIEVENT_ONVISUALSTIMULUS:
		return "OnVisualStimulus";
	case AIEVENT_ONPATHDECISION:
		return "OnPathDesision";
	case AIEVENT_ONSOUNDEVENT:
		return "OnSoundEvent";
	case AIEVENT_AGENTDIED:
		return "AgentDied";
	case AIEVENT_SLEEP:
		return "Sleep";
	case AIEVENT_WAKEUP:
		return "Wakeup";
	case AIEVENT_ENABLE:
		return "Enable";
	case AIEVENT_DISABLE:
		return "Disable";
	case AIEVENT_PATHFINDON:
		return "PathfindOn";
	case AIEVENT_PATHFINDOFF:
		return "PathfindOff";
	case AIEVENT_CLEAR:
		return "Clear";
	case AIEVENT_DROPBEACON:
		return "DropBeacon";
	case AIEVENT_USE:
		return "Use";
	default:
		return "undefined";
	}
}

//
//------------------------------------------------------------------------------------------------------------------------
void CAIObject::Event(unsigned short eType, SAIEVENT *pEvent)
{
	switch (eType)
	{
	case AIEVENT_DISABLE:
		m_bEnabled = false;
		break;
	case AIEVENT_ENABLE:
		m_bEnabled = true;
		break;
	case AIEVENT_SLEEP:
		m_bEnabled = false;
		break;
	case AIEVENT_WAKEUP:
		m_bEnabled = true;
		break;
	default:
		break;
	}
}

IPhysicalEntity* CAIObject::GetPhysics(bool wantCharacterPhysics) const
{
	if (IEntity* pEntity = GetEntity())
		return pEntity->GetPhysics();

	return 0;
}


//====================================================================
// GetEntityID
//====================================================================
unsigned CAIObject::GetEntityID() const
{
	return m_entityID;
}

//====================================================================
// SetEntityID
//====================================================================
void CAIObject::SetEntityID(unsigned ID) 
{
	m_entityID = ID;
}

//====================================================================
// GetEntity
//
// just a little helper
//====================================================================

IEntity* CAIObject::GetEntity() const 
{
	IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
	return pEntitySystem ? pEntitySystem->GetEntity(m_entityID) : 0;
}


//====================================================================
// SetFormationUpdateSight
//====================================================================
void CAIObject::SetFormationUpdateSight(float range,float minTime,float maxTime)
{
	if(m_pFormation)
		m_pFormation->SetUpdateSight(range,minTime,maxTime);
}

//====================================================================
// IsHostile
//====================================================================
bool CAIObject::IsHostile(const IAIObject* pOther, bool /*bUsingAIIgnorePlayer*/) const
{
	if(!pOther)
		return false;
	unsigned short nType=((CAIObject*)pOther)->GetType();
	return (
		nType == AIOBJECT_GRENADE ||
		nType == AIOBJECT_RPG 
		//		||m_Parameters.m_bSpeciesHostility && pOther && pOther->GetParameters().m_bSpeciesHostility 
		//			&& m_Parameters.m_nSpecies != pOther->GetParameters().m_nSpecies &&
		//			pOther->GetParameters().m_nSpecies>=0
		);
}

//====================================================================
// SetGroupId
//====================================================================
void CAIObject::SetGroupId(int id)
{
	m_groupId = id;
}

//====================================================================
// GetGroupId
//====================================================================
int CAIObject::GetGroupId() const
{
	return m_groupId;
}

//====================================================================
// SetSpecies
//====================================================================
void CAIObject::SetSpecies(int species)
{
	m_species = species;
}

//====================================================================
// GetSpecies
//====================================================================
int CAIObject::GetSpecies() const
{
	return m_species;
}


//===================================================================
// GetAnchorNavNode
//===================================================================
const GraphNode* CAIObject::GetAnchorNavNode()
{
	if ((m_nObjectType != AIANCHOR_COMBAT_HIDESPOT) && (m_nObjectType != AIANCHOR_COMBAT_HIDESPOT_SECONDARY))
	{
		AIAssert(!!!"Only AI Objects of type AIANCHOR_COMBAT_HIDESPOT or AIANCHOR_COMBAT_HIDESPOT_SECONDARY"
			" should be supplied to function CAIObject::GetAnchorNavNode()");
		return 0;
	}

	CGraph* pGraph = gAIEnv.pGraph;

	if (m_lastNavNodeIndex == 0)
	{
		m_lastNavNodeIndex = pGraph->GetEnclosing(
			GetPos(), 
			IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE |
			IAISystem::NAV_VOLUME | IAISystem::NAV_LAYERED_NAV_MESH);

		if (m_lastNavNodeIndex == 0)
		{
			m_lastNavNodeIndex = ~0;
		}
	}

	return (m_lastNavNodeIndex != ~0) ? pGraph->GetNode(m_lastNavNodeIndex) : 0;
}

//===================================================================
// GetAIDebugRecord
//===================================================================
IAIDebugRecord*	CAIObject::GetAIDebugRecord()
{
#ifdef CRYAISYSTEM_DEBUG
	if(m_pMyRecord==NULL && HasSelfReference()) 
		m_pMyRecord = s_pRecorder->AddUnit(GetSelfReference());
	return m_pMyRecord;
#else
	return NULL;
#endif //CRYAISYSTEM_DEBUG
}

tAIObjectID CAIObject::GetAIObjectID() const
{
	tAIObjectID nID = m_refThis.GetObjectID();
	assert(nID);
	return nID;
}

bool CAIObject::IsEnabled() const
{
	return m_bEnabled;
}

void CAIObject::SetWeaponDescriptor( const AIWeaponDescriptor& descriptor )
{

}

bool CAIObject::HasFormation()
{
	return m_pFormation!=NULL;
}

bool CAIObject::IsAgent() const
{
	return false;
}

void CAIObject::SetLastActionStatus( bool bSucceed )
{
	m_bLastActionSucceed = bSucceed;
}

IAIActorProxy* CAIObject::GetProxy() const
{
	return NULL;
}

bool CAIObject::IsUpdatedOnce() const
{
	return m_bUpdatedOnce;
}

IAIObject* CAIObject::GetSpecialAIObject( const char* objName, float range /*= 0.0f*/ )
{
	return NULL;
}

void CAIObject::Release()
{
	delete this;
}

const Vec3 & CAIObject::GetViewDir() const
{
	return m_vView;
}

const Vec3 & CAIObject::GetMoveDir() const
{
	return m_vMoveDir;
}

const Vec3 & CAIObject::GetFireDir() const
{
	return m_vFireDir;
}

const Vec3 & CAIObject::GetFirePos() const
{
	return m_vFirePosition;
}

const Vec3 & CAIObject::GetBodyDir() const
{
	return m_vBodyDir;
}

float CAIObject::GetRadius() const
{
	return m_fRadius;
}

unsigned short CAIObject::GetAIType() const
{
	return m_nObjectType;
}

IAIObject::ESubTypes CAIObject::GetSubType() const
{
	return m_objectSubType;
}

const Vec3 & CAIObject::GetPos() const
{
	m_bTouched = true; return m_vPosition;
}

IAIObject::EFieldOfViewResult CAIObject::IsPointInFOV( const Vec3& pos, float distanceScale /*= 1.0f*/ ) const
{
	return eFOV_Outside;
}

//====================================================================
// SAIActorTargetRequest Serialize 
//====================================================================
void SAIActorTargetRequest::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
	ser.BeginGroup("SAIActorTargetRequest");
	{
		_ser_value_(id);
		if(id != 0)
		{
			_ser_value_(approachLocation);
			_ser_value_(approachDirection);
			_ser_value_(animLocation);
			_ser_value_(animDirection);
			_ser_value_(vehicleName);
			_ser_value_(vehicleSeat);
			_ser_value_(speed);
			_ser_value_(directionTolerance);
			_ser_value_(startArcAngle);
			_ser_value_(startWidth);
			_ser_value_(signalAnimation);
			_ser_value_(projectEndPoint);
			_ser_value_(lowerPrecision);
			_ser_value_(useAssetAlignment);
			_ser_value_(animation);
			ser.EnumValue("stance", stance, STANCE_NULL, STANCE_LAST);
			// TODO: Pointers!
			//		TAnimationGraphQueryID * pQueryStart;
			//		TAnimationGraphQueryID * pQueryEnd;
		}
	}
	ser.EndGroup();
}

//====================================================================
// SAIPredictedCharacterState Serialize 
//====================================================================
void SAIPredictedCharacterState::Serialize( TSerialize ser )
{
	ser.Value("position", position);
	ser.Value("velocity", velocity);
	ser.Value("predictionTime", predictionTime);
}

//====================================================================
// SAIPredictedCharacterStates Serialize 
//====================================================================
void SAIPredictedCharacterStates::Serialize( TSerialize ser )
{
	ser.BeginGroup("SAIPredictedCharacterStates");
	{
		ser.Value("nStates", nStates);
		int counter(0);
		char stateGroupName[32];
		for(int i(0); i<maxStates; ++i, ++counter)
		{
			sprintf(stateGroupName, "State_%d", counter);
			ser.BeginGroup(stateGroupName);
			{
				states[i].Serialize(ser);
			} ser.EndGroup();
		}
	}	ser.EndGroup();
}
//====================================================================
// SOBJECTSTATE Serialize 
//====================================================================
void SOBJECTSTATE::Serialize( TSerialize ser, class CObjectTracker& objectTracker )
{
	ser.BeginGroup("SOBJECTSTATE");
	{
		ser.Value("jump", jump);		
		ser.Value("bCloseContact", bCloseContact);
		ser.ValueWithDefault("fDesiredSpeed", fDesiredSpeed, 1.f);
		ser.Value("fMovementUrgency", fMovementUrgency);
		ser.Value("fire", fire);
		ser.Value("bodystate", bodystate);
		ser.Value("vMoveDir", vMoveDir);
		ser.Value("vMoveTarget", vMoveTarget);
		ser.Value("vInflectionPoint", vInflectionPoint);
		ser.Value("vForcedNavigation", vForcedNavigation);
		ser.Value("vBodyTargetDir", vBodyTargetDir);
		ser.Value("vLookTargetPos", vLookTargetPos);
		ser.EnumValue("eTargetType", eTargetType, AITARGET_NONE, AITARGET_LAST);
		ser.EnumValue("eTargetThreat", eTargetThreat, AITHREAT_NONE, AITHREAT_LAST);
		ser.Value("bTargetEnabled", bTargetEnabled);
		ser.Value("nTargetType", nTargetType);
		ser.Value("nAuxSignal", nAuxSignal);
		ser.Value("fAuxDelay", fAuxDelay);
		ser.Value("nAuxPriority", nAuxPriority);
		ser.Value("szAuxSignalText", szAuxSignalText);
		ser.Value("lean", lean);
		ser.Value("peekOver", peekOver);
		ser.Value("aimTargetIsValid", aimTargetIsValid);
		ser.Value("weaponAccessories", weaponAccessories);

		ser.Value("fireSecondary", fireSecondary);
		ser.Value("fireMelee", fireMelee);
		ser.Value("aimObstructed", aimObstructed);
		ser.Value("forceWeaponAlertness", forceWeaponAlertness);
		ser.Value("fHitProbability", fHitProbability);
		ser.Value("fProjectileSpeedScale", fProjectileSpeedScale);
		ser.Value("vShootTargetPos", vShootTargetPos);
		ser.Value("vAimTargetPos", vAimTargetPos);
		ser.Value("allowStrafing", allowStrafing);
		ser.Value("allowEntityClampingByAnimation", allowEntityClampingByAnimation);
		ser.Value("fDistanceToPathEnd", fDistanceToPathEnd);
		ser.Value("curActorTargetFinishPos", curActorTargetFinishPos);
		//		ser.Value("remainingPath", remainingPath);
		ser.Value("bHurryNow", bHurryNow);
		ser.Value("fDistanceFromTarget", fDistanceFromTarget);
		ser.EnumValue("eTargetStuntReaction", eTargetStuntReaction, AITSR_NONE, AITSR_LAST);
		ser.EnumValue("curActorTargetPhase", curActorTargetPhase, eATP_None, eATP_Error);
		actorTargetReq.Serialize(ser, objectTracker);
		// serialize signals
		ser.BeginGroup("SIGNALS");
		{
			int signalsCount = vSignals.size();
			ser.Value("signalsCount", signalsCount);
			if(ser.IsReading())
			{
				vSignals.resize( signalsCount );
			}
			int counter(0);
			char signalGroupName[32];
			for (DynArray<AISIGNAL>::iterator ai(vSignals.begin());ai!=vSignals.end();++ai,++counter)
			{
				sprintf(signalGroupName, "Signal_%d", counter);
				ser.BeginGroup(signalGroupName);
				{
					AISIGNAL& signal = *ai;
					signal.Serialize(ser, objectTracker);
				}
				ser.EndGroup();
			}
		}
		ser.EndGroup();
		ser.Value("predictedCharacterStates", predictedCharacterStates);
	}
	ser.EndGroup();
}


//====================================================================
// AgentPerceptionParameters Serialize 
//====================================================================
void AgentPerceptionParameters::Serialize(TSerialize ser)
{
	ser.BeginGroup("AgentPerceptionParameters");
	{
		_ser_value_(sightRange);
		_ser_value_(sightRangeVehicle);
		_ser_value_(sightNearRange);
		_ser_value_(sightDelay);
		_ser_value_(velBase);					
		_ser_value_(velScale);
		_ser_value_(FOVPrimary);
		_ser_value_(FOVSecondary);
		_ser_value_(stanceScale);
		_ser_value_(audioScale);
		_ser_value_(bThermalVision);
		_ser_value_(camoScale);
		_ser_value_(heatScale);
		_ser_value_(targetPersistence);
		_ser_value_(perceptionScale.audio);
		_ser_value_(perceptionScale.visual);
		_ser_value_(bulletHitRadius);
		_ser_value_(minAlarmLevel);
		_ser_value_(sightEnvScaleNormal);
		_ser_value_(sightEnvScaleAlarmed);
	}
	ser.EndGroup();
}

//====================================================================
// AgentPerceptionParameters Serialize 
//====================================================================
void AgentParameters::Serialize(TSerialize ser)
{
	ser.BeginGroup( "AgentParameters" );
	{
		m_PerceptionParams.Serialize(ser);

		_ser_value_(m_nGroup);
		_ser_value_(m_CombatClass);
		_ser_value_(m_fAccuracy);
		_ser_value_(m_fPassRadius);
		_ser_value_(m_fStrafingPitch);		//if this > 0, will do a strafing draw line firing. 04/12/05 Tetsuji
		_ser_value_(m_fDistanceToHideFrom);
		_ser_value_(m_fAttackRange);
		_ser_value_(m_fCommRange);
		_ser_value_(m_fAttackZoneHeight);
		_ser_value_(m_fPreferredCombatDistance);
		_ser_value_(m_weaponAccessories);
		_ser_value_(m_bSpeciesHostility);
		_ser_value_(m_fGroupHostility);
		_ser_value_(m_fMeleeRange);
		_ser_value_(m_fMeleeHitRange);
		_ser_value_(m_nSpecies);
		_ser_value_(m_nRank);
		_ser_value_(m_bPerceivePlayer);
		_ser_value_(m_fAwarenessOfPlayer);
		_ser_value_(m_bSpecial);
		_ser_value_(m_bInvisible);
		_ser_value_(m_fCloakScale);
		_ser_value_(m_fCloakScaleTarget);
		_ser_value_(m_lookIdleTurnSpeed);
		_ser_value_(m_lookCombatTurnSpeed);
		_ser_value_(m_aimTurnSpeed);
		_ser_value_(m_fireTurnSpeed);
		_ser_value_(m_bAiIgnoreFgNode);
		_ser_value_(m_fGrenadeThrowDistScale);
	} 
	ser.EndGroup();
}


//--------------------------------------------------------------------
