/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   SmartObjects.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- ?						: Created by ?
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include <ISystem.h>
#include <IXml.h>
#include <ICryAnimation.h>
#include <IConsole.h>

#include "AIObject.h"
#include "Puppet.h"
#include "CAISystem.h"
#include "DebugDrawContext.h"

#include "SmartObjectNavRegion.h"

#include "AIActions.h"
#include "SmartObjects.h"

#undef AUTO_SMART_OBJECT


CSmartObject::CState::MapSmartObjectStateIds	CSmartObject::CState::g_mapStateIds;
CSmartObject::CState::MapSmartObjectStates		CSmartObject::CState::g_mapStates;

CSmartObjectClass::MapClassesByName				CSmartObjectClass::g_AllByName;

CSmartObjectClass::VectorClasses				CSmartObjectClass::g_AllUserClasses;
CSmartObjectClass::VectorClasses::iterator		CSmartObjectClass::g_itAllUserClasses = CSmartObjectClass::g_AllUserClasses.end();

CSmartObjectManager::MapSmartObjectsByEntityId			CSmartObjectManager::g_AllSmartObjects;

IMaterial* CClassTemplateData::m_pHelperMtl = NULL;

CSmartObject* CSmartObjectManager::g_pTemporarySO = NULL;
QuatT CSmartObjectManager::g_tempSOLocation;
SmartObjectHelper CSmartObjectManager::g_pTempSORootHelper;

const static char *const g_sEmptyNavSO = "emptynavso";
const static float EMPTY_SO_USE_DISTANCE = 5.0f;

// this mask should be used for finding enclosing node of navigation smart objects
#define SMART_OBJECT_ENCLOSING_NAV_TYPES (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_VOLUME | IAISystem::NAV_LAYERED_NAV_MESH)


bool CCondition:: operator == ( const CCondition& other ) const
{
#define IfDiffReturnFalse(X) if (X != other.X) return false;
#define ElseIfDiffReturnFalse(X) else if (X != other.X) return false;

	IfDiffReturnFalse(iTemplateId)
	ElseIfDiffReturnFalse(pUserClass)
	ElseIfDiffReturnFalse(userStatePattern)
	ElseIfDiffReturnFalse(pObjectClass)
	ElseIfDiffReturnFalse(objectStatePattern)
	ElseIfDiffReturnFalse(sObjectHelper)
	ElseIfDiffReturnFalse(sUserHelper)
	ElseIfDiffReturnFalse(fDistanceFrom)
	ElseIfDiffReturnFalse(fDistanceTo)
	ElseIfDiffReturnFalse(fOrientationLimit)
	ElseIfDiffReturnFalse(bHorizLimitOnly)
	ElseIfDiffReturnFalse(fOrientationToTargetLimit)
	ElseIfDiffReturnFalse(fMinDelay)
	ElseIfDiffReturnFalse(fMaxDelay)
	ElseIfDiffReturnFalse(fMemory)
	ElseIfDiffReturnFalse(fProximityFactor)
	ElseIfDiffReturnFalse(fOrientationFactor)
	ElseIfDiffReturnFalse(fVisibilityFactor)
	ElseIfDiffReturnFalse(fRandomnessFactor)
	ElseIfDiffReturnFalse(fLookAtPerc)
	ElseIfDiffReturnFalse(userPostActionStates)
	ElseIfDiffReturnFalse(objectPostActionStates)
	ElseIfDiffReturnFalse(eActionType)
	ElseIfDiffReturnFalse(sAction)
	ElseIfDiffReturnFalse(userPreActionStates)
	ElseIfDiffReturnFalse(objectPreActionStates)
	ElseIfDiffReturnFalse(iMaxAlertness)
	ElseIfDiffReturnFalse(bEnabled)
	ElseIfDiffReturnFalse(sEvent)
	ElseIfDiffReturnFalse(sChainedUserEvent)
	ElseIfDiffReturnFalse(sChainedObjectEvent)
	ElseIfDiffReturnFalse(sName)
	ElseIfDiffReturnFalse(sDescription)
	ElseIfDiffReturnFalse(sFolder)
	ElseIfDiffReturnFalse(iOrder)
	ElseIfDiffReturnFalse(iRuleType)
	ElseIfDiffReturnFalse(sEntranceHelper)
	ElseIfDiffReturnFalse(sExitHelper)
	ElseIfDiffReturnFalse(fApproachSpeed)
	ElseIfDiffReturnFalse(iApproachStance)
	ElseIfDiffReturnFalse(sAnimationHelper)
	ElseIfDiffReturnFalse(sApproachHelper)
	ElseIfDiffReturnFalse(fStartWidth)
	ElseIfDiffReturnFalse(fDirectionTolerance)
	ElseIfDiffReturnFalse(fStartArcAngle)
	else
		return true;

#undef IfDiffReturnFalse
#undef ElseIfDiffReturnFalse
}

/////////////////////////////////////////
// CSmartObjectBase class implementation
/////////////////////////////////////////

Vec3 CSmartObjectBase::GetPos() const
{
	IEntity* pEntity = GetEntity();
	if ( !pEntity )
		return CSmartObjectManager::g_tempSOLocation.t;

	IAIObject* pAI = pEntity->GetAI();
	if ( pAI && pAI->IsEnabled() )
		return pAI->GetPos();
	return pEntity->GetWorldPos();
}

bool GetPelvisJointForRagdollizedActor(IEntity* pEntity, QuatT& pelvisJoint)
{
	IPhysicalEntity* pPE = pEntity->GetPhysics();
	if ( pPE && pEntity->GetAI() && pPE->GetType() == PE_ARTICULATED )
	{
		// it's a ragdollized actor -> special processing
		ICharacterInstance* pCharInstance = pEntity->GetCharacter(0);
		if ( pCharInstance )
		{
			ISkeletonPose* pSkeletonPose = pCharInstance->GetISkeletonPose();
			if ( pSkeletonPose )
			{
				int boneId = pSkeletonPose->GetJointIDByName( "Bip01 Pelvis" );
				if ( boneId >= 0 )
				{
					pelvisJoint = pSkeletonPose->GetAbsJointByID(boneId);
					return true;
				}
			}
		}
	}

	return false;
}

Vec3 CSmartObjectBase::GetHelperPos( const SmartObjectHelper* pHelper ) const
{
	IEntity* pEntity = GetEntity();
	if ( !pEntity )
		return CSmartObjectManager::g_tempSOLocation * pHelper->qt.t;

	QuatT pelvisJoint;
	if (GetPelvisJointForRagdollizedActor(pEntity, pelvisJoint))
	{
		Vec3 pos( pHelper->qt.t.z, -pHelper->qt.t.y, pHelper->qt.t.x );
		pos = pelvisJoint * pos;
		return pEntity->GetWorldTM().TransformPoint( pos );
	}

	return pEntity->GetWorldTM().TransformPoint( pHelper->qt.t );
}

Vec3 CSmartObjectBase::GetOrientation( const SmartObjectHelper* pHelper ) const
{
	IEntity* pEntity = GetEntity();
	if ( !pEntity )
	{
		if ( pHelper )
			return CSmartObjectManager::g_tempSOLocation.q * pHelper->qt.q * FORWARD_DIRECTION;
		else
			return CSmartObjectManager::g_tempSOLocation.q * FORWARD_DIRECTION;
	}

	QuatT pelvisJoint;
	if (GetPelvisJointForRagdollizedActor(pEntity, pelvisJoint))
	{
		Vec3 forward = pHelper ? pHelper->qt.q * FORWARD_DIRECTION : FORWARD_DIRECTION;
		forward.Set( forward.z, -forward.y, forward.x );
		forward = pelvisJoint.q * forward;
		return pEntity->GetWorldTM().TransformVector( forward );
	}

	if ( pHelper )
		return pEntity->GetWorldTM().TransformVector( pHelper->qt.q * FORWARD_DIRECTION );

	CAIActor* pAIActor = CastToCAIActorSafe(pEntity->GetAI());
	if ( pAIActor )
	{
		if ( pAIActor->GetState()->vMoveDir.IsZero( 0.1f ) )
			return pAIActor->GetViewDir();
		else
			return pAIActor->GetState()->vMoveDir;
	}
	return pEntity->GetWorldTM().TransformVector(Vec3(0,1,0));
}

CPuppet* CSmartObjectBase::GetPuppet() const
{
	return CastToCPuppetSafe(GetAI());
}

CAIActor* CSmartObjectBase::GetAIActor() const
{
	return CastToCAIActorSafe(GetAI());
}

/////////////////////////////////////////
// CSmartObject class implementation
/////////////////////////////////////////

CSmartObject::CSmartObject( EntityId entityId )
	: CSmartObjectBase( entityId )
	, m_fLookAtLimit(0.0f)
	, m_vLookAtPos(ZERO)
	, m_eValidationResult(eSOV_Unknown)
	, m_bHidden(false)
{
	m_States.insert( "Idle" );
	m_fRandom = float(ai_rand())/(2.0f*RAND_MAX);
	AIAssert(CSmartObjectManager::g_AllSmartObjects.find(entityId) == CSmartObjectManager::g_AllSmartObjects.end());
	CSmartObjectManager::g_AllSmartObjects[ entityId ] = this;
	IEntity* pEntity = GetEntity();
	AIAssert(pEntity);
	if (pEntity)
	{
		pEntity->SetSmartObject( this );
	}
}

CSmartObject::CSmartObject( ETemporarySOType soType )
: CSmartObjectBase( soType )
, m_fLookAtLimit(0.0f)
, m_vLookAtPos(ZERO)
, m_eValidationResult(eSOV_Unknown)
, m_bHidden(false)
{
	m_fRandom = float(ai_rand())/(2.0f*RAND_MAX);
}

CSmartObject::~CSmartObject()
{
	if ( m_entityId )
	{
		CSmartObjectManager::g_AllSmartObjects.erase( m_entityId );

		if (IEntity* pEntity = GetEntity())
		{
			pEntity->SetSmartObject( NULL );
		}
	}
}

void CSmartObject::Serialize( TSerialize ser )
{
	if ( ser.IsReading() )
	{
		m_Events.clear();
		m_mapLastUpdateTimes.clear();
		m_fLookAtLimit = 0;
		m_vLookAtPos.zero();
		m_mapNavNodes.clear();
	}

	ser.Value( "m_fRandom", m_fRandom );

	if ( ser.BeginOptionalGroup("States",true) )
	{
		if ( ser.IsReading() )
		{
			m_States.clear();
			int count;
			ser.Value( "Size", count );
			while ( count-- )
			{
				string value;
				ser.BeginGroup("i");
				{
					ser.Value( "Value", value );
					m_States.insert(CState( value ));
				}
				ser.EndGroup();
			}
		}
		else
		{
			int count = m_States.size();
			ser.Value( "Size", count );
			SetStates::iterator it, itEnd = m_States.end();
			for ( it = m_States.begin(); it != itEnd; ++it )
			{
				string value = it->c_str();
				ser.BeginGroup("i");
				{
					ser.Value( "Value", value );
				}
				ser.EndGroup();
			}
		}
		ser.EndGroup(); //States
	}
}

void CSmartObjectManager::SerializePointer( TSerialize ser, const char* name, CSmartObject* & pSmartObject )
{
	if ( ser.IsWriting() )
	{
		EntityId id = pSmartObject ? pSmartObject->GetEntityId() : 0;
		ser.Value( name, id );
	}
	else
	{
		EntityId id;
		ser.Value( name, id );
		if ( id )
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity( id );
			AIAssert( pEntity );
			pSmartObject = IsSmartObject( pEntity );
			AIAssert( pSmartObject );
		}
		else
			pSmartObject = NULL;
	}
}

void CSmartObjectManager::SerializePointer( TSerialize ser, const char* name, CCondition* & pRule )
{
	if ( ser.IsWriting() )
	{
		if(pRule)
			ser.Value( name, pRule->iOrder );
		else
		{
			int nulValue = -1;
			ser.Value( name, nulValue );
		}
	}
	else
	{
		pRule = NULL;
		int iOrder;
		ser.Value( name, iOrder );
		if ( iOrder == -1 )
			return;

		MapConditions& rules = gAIEnv.pSmartObjectManager->m_Conditions;
		MapConditions::iterator it, itEnd = rules.end();
		for( it = rules.begin(); it != itEnd; ++it )
			if ( it->second.iOrder == iOrder )
			{
				pRule = &it->second;
				return;
			}
	}
}

void CSmartObject::Use( CSmartObject* pObject, CCondition* pCondition, int eventId /*=0*/, bool bForceHighPriority /*=false*/ ) const
{
	CAIActionManager* pActionManager = gAIEnv.pAIActionManager;
	const char* sSignal = NULL;
	IAIAction* pAction = NULL;
	if ( !pCondition->sAction.empty() && pCondition->eActionType != eAT_None )
	{
		if ( pCondition->eActionType == eAT_AISignal )
			sSignal = pCondition->sAction;
		else
			pAction = pActionManager->GetAIAction( pCondition );

		if ( !pAction && !sSignal )
		{
			AIWarning( "Undefined AI action \"%s\"!", pCondition->sAction.c_str() );

			// undo states since action wasn't found
			if ( !pCondition->userPreActionStates.empty() )
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), pCondition->userPreActionStates.AsUndoString() );
			if ( this != pObject )
				if ( !pCondition->objectPreActionStates.empty() )
					gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObject->GetEntity(), pCondition->objectPreActionStates.AsUndoString() );
			return;
		}
	}

	int maxAlertness = pCondition->iMaxAlertness;
	if ( bForceHighPriority || pCondition->eActionType == eAT_PriorityAction || pCondition->eActionType == eAT_PriorityAnimationSignal || pCondition->eActionType == eAT_PriorityAnimationAction )
		maxAlertness += 100;

	CAIActor* pAIObject = GetAIActor();
	if (pAIObject && pAIObject->IsAgent())
	{
		IEntity* pObjectEntity = pObject->GetEntity();
		if ( sSignal )
		{
			AISignalExtraData* pExtraData = new AISignalExtraData;
			if ( eventId )
				pExtraData->iValue = eventId;
			pExtraData->nID = pObject->GetEntityId();

			pExtraData->point = pCondition->pObjectHelper ? pObject->GetHelperPos( pCondition->pObjectHelper ) : pObject->GetPos();

			pAIObject->SetSignal( 10, sSignal, pObjectEntity, pExtraData );

			// update states
			if ( !pCondition->userPostActionStates.empty() ) // check is next state non-empty
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), pCondition->userPostActionStates.AsString() );
			if ( this != pObject )
				if ( !pCondition->objectPostActionStates.empty() ) // check is next state non-empty
					gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObjectEntity, pCondition->objectPostActionStates.AsString() );

			return;
		}
		//else if (pAIObject->CastToCAIPlayer())
		//{
		//	pAIObject = NULL;
		//}
		else
		{
			if ( eventId || (pAction && !pAction->GetFlowGraph()) )
			{
				if ( !pAction->GetFlowGraph() )
				{
					// set reference point to say where to play the animation
					CAnimationAction* pAnimAction = static_cast< CAnimationAction* >( pAction );

					if ( pCondition->pObjectClass )
					{
						if ( pCondition->sAnimationHelper.c_str()[0] == '<' )
						{
							if ( pCondition->sAnimationHelper == "<Auto>" )
								pAnimAction->SetAutoTarget( pObject->GetPos(), pObject->GetOrientation(NULL) );
							else if ( pCondition->sAnimationHelper == "<AutoInverse>" )
								pAnimAction->SetAutoTarget( pObject->GetPos(), Quat::CreateRotationZ(gf_PI) * pObject->GetOrientation(NULL) );
							else
								AIError( "Invalid animation helper name '%s' in smart object rule '%s'!", pCondition->sAnimationHelper.c_str(), pCondition->sName.c_str() );

							// approach target might be different
							const SmartObjectHelper* pApproachHelper = pCondition->pObjectClass->GetHelper( pCondition->sApproachHelper );
							if ( pApproachHelper )
								pAnimAction->SetApproachPos( pObject->GetHelperPos(pApproachHelper) );
						}
						else if ( const SmartObjectHelper* pHelper = pCondition->pObjectClass->GetHelper(pCondition->sAnimationHelper) )
						{
							pAnimAction->SetTarget( pObject->GetHelperPos(pHelper), pObject->GetOrientation(pHelper) );

							// approach target might be different
							const SmartObjectHelper* pApproachHelper = pCondition->pObjectClass->GetHelper( pCondition->sApproachHelper );
							if ( pApproachHelper )
								pAnimAction->SetApproachPos( pObject->GetHelperPos(pApproachHelper) );
						}
						else
						{
							Vec3 direction = pObject->GetPos() - GetPos();
							direction.NormalizeSafe();
							pAnimAction->SetTarget( GetPos(), direction );
						}
					}
					else
					{
						pAnimAction->SetTarget( GetPos(), GetOrientation(NULL) );
					}
				}

				// if eventId was provided or animation was requested just execute the action immediately without sending a signal
				if ( pObject == CSmartObjectManager::g_pTemporarySO )
				{
					pActionManager->ExecuteAIAction( pAction, GetEntity(), GetEntity(), maxAlertness, eventId,
						pCondition->userPostActionStates.AsString(), "", pCondition->userPreActionStates.AsUndoString(), "" );
				}
				else
				{
					pActionManager->ExecuteAIAction( pAction, GetEntity(), pObject->GetEntity(), maxAlertness, eventId,
						pCondition->userPostActionStates.AsString(), pCondition->objectPostActionStates.AsString(),
						pCondition->userPreActionStates.AsUndoString(), pCondition->objectPreActionStates.AsUndoString() );
				}
			}
			else
			{
				AISignalExtraData* pExtraData = new AISignalExtraData;
				pExtraData->SetObjectName( string(pAction->GetName()) +
					",\"" + pCondition->userPostActionStates.AsString() + '\"' +
					",\"" + pCondition->objectPostActionStates.AsString() + '\"' +
					",\"" + pCondition->userPreActionStates.AsUndoString() + '\"' +
					",\"" + pCondition->objectPreActionStates.AsUndoString() + '\"' );
				pExtraData->iValue = maxAlertness;
				pAIObject->SetSignal( 10, "OnUseSmartObject", pObjectEntity, pExtraData, gAIEnv.SignalCRCs.m_nOnUseSmartObject);
			}

			return;
		}
	}

	if ( !pAIObject )
	{
		// the user isn't a puppet (or vehicle, or player)
		if ( sSignal )
		{
			AIWarning( "Attempt to send AI signal to an entity not registered as Puppet or Vehicle in AI system!" );

			// undo pre-action states
			if ( !pCondition->userPreActionStates.empty() ) // check is next state non-empty
				gAIEnv.pSmartObjectManager->ModifySmartObjectStates( GetEntity(), pCondition->userPreActionStates.AsUndoString() );
			if ( this != pObject )
				if ( !pCondition->objectPreActionStates.empty() ) // check is next state non-empty
					gAIEnv.pSmartObjectManager->ModifySmartObjectStates( pObject->GetEntity(), pCondition->objectPreActionStates.AsUndoString() );
		}
		else if ( pAction && pAction->GetFlowGraph() )
		{
			gAIEnv.pAIActionManager->ExecuteAIAction( pAction, GetEntity(), pObject->GetEntity(), 100, eventId,
				pCondition->userPostActionStates.AsString(), pCondition->objectPostActionStates.AsString(),
				pCondition->userPreActionStates.AsUndoString(), pCondition->objectPreActionStates.AsUndoString() );
		}
	}
}


/////////////////////////////////////////
// CSmartObjectClass class implementation
/////////////////////////////////////////

void CSmartObjectClass::RegisterSmartObject( CSmartObject* pSmartObject )
{
	AIAssert( std::find( m_allSmartObjectInstances.begin(), m_allSmartObjectInstances.end(), pSmartObject ) == m_allSmartObjectInstances.end() );
	m_allSmartObjectInstances.push_back( pSmartObject );
	pSmartObject->GetClasses().push_back( this );
}

void CSmartObjectClass::UnregisterSmartObject( CSmartObject* pSmartObject )
{
	CSmartObjectClass::VectorSmartObjects::iterator find
		= std::find( m_allSmartObjectInstances.begin(), m_allSmartObjectInstances.end(), pSmartObject );
	AIAssert( find != m_allSmartObjectInstances.end() );
	*find = m_allSmartObjectInstances.back();
	m_allSmartObjectInstances.pop_back();
	
	CSmartObjectClasses::iterator it = std::find( pSmartObject->GetClasses().begin(), pSmartObject->GetClasses().end(), this );
	AIAssert( it != pSmartObject->GetClasses().end() );
	pSmartObject->GetClasses().erase( it );
}

void CSmartObjectClass::DeleteSmartObject( CSmartObject& smartObject )
{
	// Find registered SO
	CSmartObjectClass::VectorSmartObjects::iterator it
		= std::find( m_allSmartObjectInstances.begin(), m_allSmartObjectInstances.end(), &smartObject );

	// If SO registered
	if ( it != m_allSmartObjectInstances.end() )
	{
		CSmartObjectClasses & vClasses = (*it)->GetClasses();

		// Find class for SO
		CSmartObjectClasses::iterator itClass = std::find( vClasses.begin(), vClasses.end(), this );
		AIAssert( itClass != vClasses.end() );

		if ( itClass != vClasses.end() )
			vClasses.erase( itClass );

		// If last class reference has just been deleted
		if ( vClasses.empty() )
			delete *it; // deletes the smart object!

		// 
		*it = m_allSmartObjectInstances.back();
		m_allSmartObjectInstances.pop_back();
	}
}

CSmartObjectClass::CSmartObjectClass( const char* className )
	: m_sName( className )
	, m_bSmartObjectUser( false )
	, m_bNeeded( false )
	, m_pTemplateData( NULL )
	, m_indexNextObject( ~0 )
{
	g_AllByName[ className ] = this;
}

CSmartObjectClass::~CSmartObjectClass()
{
	while ( !m_allSmartObjectInstances.empty() )
	{
		AIAssert(m_allSmartObjectInstances.back());
		DeleteSmartObject( *m_allSmartObjectInstances.back() );
	}

	VectorClasses::iterator it = std::find( g_AllUserClasses.begin(), g_AllUserClasses.end(), this );
	if ( it != g_AllUserClasses.end() )
	{
		// erase unordered vector element
		*it = g_AllUserClasses.back();
		g_AllUserClasses.pop_back();
	}
	g_itAllUserClasses = CSmartObjectClass::g_AllUserClasses.end();

	g_AllByName.erase( m_sName );
}

CSmartObjectClass* CSmartObjectClass::find( const char* sClassName )
{
	if ( g_AllByName.empty() )
		return NULL;
	MapClassesByName::iterator it = g_AllByName.find( CONST_TEMP_STRING(sClassName) );
	return it != g_AllByName.end() ? it->second : NULL;
}

const char* CSmartObjectClass::GetClassName( int id )
{
	MapClassesByName::iterator it = g_AllByName.begin();
	std::advance( it, id );
	if ( it != g_AllByName.end() )
		return it->first;
	return NULL;
}

void CSmartObjectClass::ClearHelperLinks()
{
	m_setNavHelpers.clear();
	m_vHelperLinks.clear();
}

bool CSmartObjectClass::AddHelperLink( CCondition* condition )
{
	AIAssert( condition->pObjectClass == this );
	SmartObjectHelper* pEntrance = GetHelper( condition->sEntranceHelper );
	SmartObjectHelper* pExit = GetHelper( condition->sExitHelper );
	if ( pEntrance && pExit )
	{
		m_setNavHelpers.insert( pEntrance );
		m_setNavHelpers.insert( pExit );

		HelperLink link = {
			condition->iRuleType == 1 ? 100.f : 0.f,
			pEntrance,
			pExit,
			condition
		};
		m_vHelperLinks.push_back( link );
		return true;
	}
	return false;
}

int CSmartObjectClass::FindHelperLinks(
	const SmartObjectHelper* from, const SmartObjectHelper* to, const CSmartObjectClass* pClass, float radius,
	CSmartObjectClass::HelperLink** pvHelperLinks, int iCount /*=1*/ )
{
	int numFound = 0;
	int i = m_vHelperLinks.size();
	while ( i-- && numFound < iCount )
	{
		HelperLink* pLink = &m_vHelperLinks[i];
		if ( pLink->from == from && pLink->to == to && pClass == pLink->condition->pUserClass && radius <= pLink->passRadius )
			pvHelperLinks[ numFound++ ] = pLink;
	}
	return numFound;
}

int CSmartObjectClass::FindHelperLinks(
	const SmartObjectHelper* from, const SmartObjectHelper* to, const CSmartObjectClass* pClass, float radius,
	const CSmartObject::SetStates& userStates, const CSmartObject::SetStates& objectStates,
	CSmartObjectClass::HelperLink** pvHelperLinks, int iCount /*=1*/ )
{
	int numFound = 0;
	int i = m_vHelperLinks.size();
	while ( i-- && numFound < iCount )
	{
		HelperLink* pLink = &m_vHelperLinks[i];
		if ( pLink->from == from && pLink->to == to && pClass == pLink->condition->pUserClass && radius <= pLink->passRadius &&
			pLink->condition->userStatePattern.Matches( userStates ) && pLink->condition->objectStatePattern.Matches( objectStates ) )
				pvHelperLinks[ numFound++ ] = pLink;
	}
	return numFound;
}


/////////////////////////////////////////
// CSmartObjectManager class implementation
/////////////////////////////////////////

typedef std::list< string > ListTokens;

int Tokenize( char*& str, int& length )
{
	char* token = str;
	int tokenLength = length;

	// Skip the previous token.
	token += tokenLength;

	// Skip any leading whitespace.
	while (*token && !((*token >= 'A' && *token <= 'Z') || (*token >= 'a' && *token <= 'z') || *token == '_'))
		++token;

	// Find the end of the token.
	tokenLength = 0;
	char* end = token;
	while ((*end >= '0' && *end <= '9') || (*end >= 'A' && *end <= 'Z') || (*end >= 'a' && *end <= 'z') || *end == '_')
		++end;
	tokenLength = (int)(end - token);

	str = token;
	length = tokenLength;

	return length;
}

void CSmartObjectManager::String2Classes( const string& sClass, CSmartObjectClasses& vClasses )
{
	CSmartObjectClass* pClass;

	// Make a temporary copy of the string - more efficient to do this once without memory allocations,
	// because then we can write temporary null-terminators into the buffer to avoid copying/string allocations
	// later.
	char classString[1024];
	AIAssert(int(sClass.size()) < sizeof(classString) - 1);
	int stringLength = min(int(sClass.size()), int(sizeof(classString)) - 1);
	memcpy(classString, sClass.c_str(), stringLength);
	classString[stringLength] = 0;

	char* token = classString;
	int tokenLength = 0;
	while (Tokenize(token, tokenLength))
	{
		// Temporarily null-terminate the token.
		char oldChar = token[tokenLength];
		token[tokenLength] = 0;

		pClass = GetSmartObjectClass(token);

		if ( pClass && std::find( vClasses.begin(), vClasses.end(), pClass ) == vClasses.end() )
			vClasses.push_back( pClass );

		token[tokenLength] = oldChar;
	};
}

void CSmartObjectManager::String2States( const char* listStates, CSmartObject::DoubleVectorStates& states )
{
	states.positive.clear();
	states.negative.clear();

	// Make a temporary copy of the string - more efficient to do this once without memory allocations,
	// because then we can write temporary null-terminators into the buffer to avoid copying/string allocations
	// later.
	char statesString[1024];
	int inputStringLength = int(strlen(listStates));
	int stringLength = min(inputStringLength, int(sizeof(statesString)) - 1);
	AIAssert(inputStringLength < sizeof(statesString) - 1);
	memcpy(statesString, listStates, stringLength);
	statesString[stringLength] = 0;

	// Split the string into positive and negative state lists.
	int hyphenPosition = -1;
	for (int i = 0; statesString[i]; ++i)
	{
		if (statesString[i] == '-')
		{
			hyphenPosition = i;
			break;
		}
	}

	char* positiveStateList = statesString;
	char* negativeStateList = 0;
	if (hyphenPosition >= 0)
	{
		statesString[hyphenPosition] = 0;
		negativeStateList = statesString + hyphenPosition + 1;
	}

	// Examine the two lists (positive and negative) to extract a list of states for each.
	CSmartObject::VectorStates* stateVectors[2] = {&states.positive, &states.negative};
	char* stateStrings[2] = {positiveStateList, negativeStateList};

	for (int stateVectorIndex = 0; stateVectorIndex < 2; ++stateVectorIndex)
	{
		char* token = stateStrings[stateVectorIndex];
		int tokenLength = 0;
		while (token && Tokenize(token, tokenLength))
		{
			// Temporarily null-terminate the token.
			char oldChar = token[tokenLength];
			token[tokenLength] = 0;

			stateVectors[stateVectorIndex]->push_back(token);

			token[tokenLength] = oldChar;
		}
	}
}

void CSmartObjectManager::String2StatePattern( const char* sPattern, CSmartObject::CStatePattern& pattern )
{
	pattern.clear();
	string item;
	CSmartObject::DoubleVectorStates dvs;
	while ( sPattern )
	{
		const char* i = strchr( sPattern, '|' );
		if ( !i )
		{
			item = sPattern;
			--i;
		}
		else
			item.assign( sPattern, i-sPattern );
		
		String2States(item, dvs );

		if ( !dvs.empty() )
			pattern.push_back( dvs );
		sPattern = i+1;
	}
}

CSmartObjectManager::CSmartObjectManager()
	: m_StateSameGroup( "SameGroup" )
	, m_StateSameSpecies( "SameSpecies" )
	, m_StateAttTarget( "AttTarget" )
	, m_StateBusy( "Busy" )
	, m_pPreOnSpawnEntity( NULL )
	,m_initialized(false)
{
	// register system states
	CSmartObject::CState( "Idle" );
	CSmartObject::CState( "Alerted" );
	CSmartObject::CState( "Combat" );
	CSmartObject::CState( "Dead" );

	g_pTempSORootHelper.qt.SetIdentity();
	g_pTempSORootHelper.name = "Root";
	g_pTempSORootHelper.description = "";

	CSmartObjectClass* pTempSOClass = GetSmartObjectClass( "Temporary" );
	pTempSOClass->AddHelper( &g_pTempSORootHelper );

	g_pTemporarySO = new CSmartObject( TEMPORARY_SO );
	g_pTemporarySO->GetClasses().push_back( pTempSOClass );
}

CSmartObjectManager::~CSmartObjectManager()
{
	ClearConditions();
}

void CSmartObjectManager::Serialize( TSerialize ser )
{
	if ( ser.IsReading() )
	{
		m_vDebugUse.clear();
	}

	if ( ser.BeginOptionalGroup("SmartObjects",true ))
	{
		if (ser.IsWriting())
		{
			int count = g_AllSmartObjects.size();
			ser.Value( "Count",count );
			MapSmartObjectsByEntityId::iterator it, itEnd = g_AllSmartObjects.end();
			for ( it = g_AllSmartObjects.begin(); it != itEnd; ++it )
			{
				ser.BeginGroup("Object");
				{
					CSmartObject* pObject = it->second;
					SerializePointer( ser, "id", pObject );
					if (pObject)
						pObject->Serialize( ser );
				}
				ser.EndGroup();
			}
		}
		else
		{
			int count = 0;
			ser.Value( "Count",count );
			for (int i = 0; i < count; i++)
			{
				ser.BeginGroup("Object");
				{
					CSmartObject* pObject = 0;
					SerializePointer( ser, "id", pObject );
					if (pObject)
						pObject->Serialize( ser );
				}
				ser.EndGroup();
			}
		}

		ser.EndGroup(); //SmartObjects
	}
}

void CSmartObjectManager::AddSmartObjectClass( CSmartObjectClass* soClass )
{
	CSmartObjectClass::MapSmartObjectsByPos& mapByPos = soClass->m_MapObjectsByPos;

	CSmartObjectClasses vClasses;

	// we must check already created instances of this class and 'convert' them to smart objects
	IEntitySystem* pEntitySystem = gEnv->pEntitySystem;
	if ( pEntitySystem )
	{
		IEntityItPtr it = pEntitySystem->GetEntityIterator();
		while ( IEntity* pEntity = it->Next() )
		{
			if ( !pEntity->IsGarbage() )
			{
				if ( ParseClassesFromProperties(pEntity, vClasses) )
				{
					if ( std::find( vClasses.begin(), vClasses.end(), soClass ) != vClasses.end() )
					{
						CSmartObject* smartObject = IsSmartObject( pEntity );
						if ( !smartObject )
							smartObject = new CSmartObject( pEntity->GetId() );
						soClass->RegisterSmartObject( smartObject );
						smartObject->m_fKey = smartObject->GetPos().x;
						mapByPos.insert( std::make_pair( smartObject->m_fKey, smartObject ) );
					}
				}
			}
		}
	}
}

bool CSmartObjectManager::ParseClassesFromProperties( const IEntity* pEntity, CSmartObjectClasses& vClasses )
{
	vClasses.clear();

	IScriptTable* pTable = pEntity->GetScriptTable();
	if ( pTable )
	{
		// ignore AISpawners
		string className = pEntity->GetClass()->GetName();
		className = className.substr( 0, 5 );
		if ( className == "Spawn" )
			return false;

		SmartScriptTable props;
		if ( pTable->GetValue("Properties", props) )
		{
			const char* szClass = NULL;
			props->GetValue( "soclasses_SmartObjectClass", szClass );

			const char* szClassInstance = NULL;
			SmartScriptTable propsInstance;
			if ( pTable->GetValue("PropertiesInstance", propsInstance) )
				propsInstance->GetValue( "soclasses_SmartObjectClass", szClassInstance );

			if ( szClass && *szClass )
				String2Classes(szClass, vClasses );
			if ( szClassInstance && *szClassInstance )
				String2Classes(szClassInstance, vClasses );
		}
	}

	return !vClasses.empty();
}

/*
CSmartObjectClass* CSmartObjectManager::GetSmartObjectClass( int iAnchorType, const char* className )
{
	CSmartObjectClass* pClass = CSmartObjectClass::find( iAnchorType );
	if ( pClass )
		return pClass;

	// If you are registering a new class then className must be provided
	AIAssert( className );

	CSmartObjectClass* result = new CSmartObjectClass( iAnchorType, className );
	m_SmartObjects[ result ]; // = MapSmartObjectsByState();
	AddSmartObjectClass( result );
	return result;
}

CSmartObjectClass* CSmartObjectManager::GetSmartObjectClass( IEntityClass* pEntityClass, const char* className )
{
	CSmartObjectClass* pClass = CSmartObjectClass::find( pEntityClass );
	if ( pClass )
		return pClass;

	// If you are registering a new class then className must be provided
	AIAssert( className );

	CSmartObjectClass* result = new CSmartObjectClass( pEntityClass, className );
	m_SmartObjects[ result ]; // = MapSmartObjectsByState();
	AddSmartObjectClass( result );
	return result;
}

CSmartObjectClass* CSmartObjectManager::GetSmartObjectClass( const char* className )
{
	if ( !stricmp(className, "AIOBJECT_PUPPET") )
		return GetSmartObjectClass( AIOBJECT_PUPPET, className );

	if ( !stricmp(className, "AIOBJECT_VEHICLE") )
		return GetSmartObjectClass( AIOBJECT_VEHICLE, className );

	if ( !stricmp(className, "AIOBJECT_PLAYER") )
		return GetSmartObjectClass( AIOBJECT_PLAYER, className );

	IEntityClass* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( className );
	if ( pEntityClass )
		return GetSmartObjectClass( pEntityClass, className );

	SmartScriptTable anchors;
	IScriptSystem* pScriptSystem = gEnv->pScriptSystem;
	if ( !pScriptSystem->GetGlobalValue( "AIAnchorTable", anchors ) )
		if ( pScriptSystem->ExecuteFile( "Scripts/AI/anchor.lua", false, false ) )
			pScriptSystem->GetGlobalValue( "AIAnchorTable", anchors );
	AIAssert( anchors.GetPtr() );

	int anchorType;
	if (anchors->GetValue( className, anchorType ))
		return GetSmartObjectClass( anchorType, className );

	return NULL;
}
*/

CSmartObjectClass* CSmartObjectManager::GetSmartObjectClass( const char* className )
{
	AIAssert( className );

	if ( !*className )
		return NULL;

	CSmartObjectClass* pClass = CSmartObjectClass::find( className );
	if ( pClass )
		return pClass;

	CSmartObjectClass* result = new CSmartObjectClass( className );
	AddSmartObjectClass( result );
	return result;
}

void CSmartObjectManager::SetSmartObjectConditions( SmartObjectConditions& conditions )
{
	ClearConditions();

	SmartObjectConditions::iterator it, itEnd = conditions.end();
	for ( it = conditions.begin(); it != itEnd; ++it )
		AddSmartObjectCondition( *it );

	// restore links to helpers
	MapSOHelpers::iterator itHelpers, itHelpersEnd = m_mapHelpers.end();
	for ( itHelpers = m_mapHelpers.begin(); itHelpers != itHelpersEnd; ++itHelpers )
	{
		CSmartObjectClass* pClass = GetSmartObjectClass( itHelpers->first );
		pClass->AddHelper( &itHelpers->second );
	}

	// find pointers to helpers to speed-up things
	MapConditions::iterator itConditions, itConditionsEnd = m_Conditions.end();
	for ( itConditions = m_Conditions.begin(); itConditions != itConditionsEnd; ++itConditions )
	{
		CCondition& condition = itConditions->second;
		if ( condition.pUserClass && !condition.sUserHelper.empty() )
			condition.pUserHelper = condition.pUserClass->GetHelper( condition.sUserHelper );
		else
			condition.pUserHelper = NULL;
		if ( condition.pObjectClass && !condition.sObjectHelper.empty() )
			condition.pObjectHelper = condition.pObjectClass->GetHelper( condition.sObjectHelper );
		else
			condition.pObjectHelper = NULL;
	}
}

void CSmartObjectManager::AddSmartObjectCondition( const SmartObjectCondition& conditionData )
{
	AIAssert(IsInitialized());

	CSmartObjectClass* pUserClass = GetSmartObjectClass( conditionData.sUserClass );
	AIAssert( pUserClass );
	if ( !pUserClass )
	{
		AIWarning( "WARNING: Smart Object class '%s' not found while trying to do CSmartObjectManager::AddSmartObjectCondition",
			conditionData.sUserClass.c_str() );
		return;
	}

	pUserClass->MarkAsSmartObjectUser();

	CCondition condition;
	condition.iTemplateId = conditionData.iTemplateId;

	condition.pUserClass = pUserClass;
	String2StatePattern( conditionData.sUserState, condition.userStatePattern );

	condition.pObjectClass = GetSmartObjectClass( conditionData.sObjectClass );
	AIAssert( condition.pObjectClass || conditionData.sObjectClass.empty() );
	if ( !condition.pObjectClass && !conditionData.sObjectClass.empty() )
	{
		AIWarning( "WARNING: Smart Object class '%s' not found while trying to do CSmartObjectManager::AddSmartObjectCondition",
			conditionData.sObjectClass.c_str() );
		return;
	}
	String2StatePattern( conditionData.sObjectState, condition.objectStatePattern );

	condition.sUserHelper = conditionData.sUserHelper.c_str();
	condition.pUserHelper = NULL;
	condition.sObjectHelper = conditionData.sObjectHelper.c_str();
	condition.pObjectHelper = NULL;

	condition.fDistanceFrom = conditionData.fDistanceFrom;
	condition.fDistanceTo = conditionData.fDistanceTo;
	condition.fOrientationLimit = conditionData.fOrientationLimit;
	condition.bHorizLimitOnly = conditionData.bHorizLimitOnly;
	condition.fOrientationToTargetLimit = conditionData.fOrientationToTargetLimit;

	condition.fMinDelay = conditionData.fMinDelay;
	condition.fMaxDelay = conditionData.fMaxDelay;
	condition.fMemory = conditionData.fMemory;

	condition.fProximityFactor = conditionData.fProximityFactor;
	condition.fOrientationFactor = conditionData.fOrientationFactor;
	condition.fVisibilityFactor = conditionData.fVisibilityFactor;
	condition.fRandomnessFactor = conditionData.fRandomnessFactor;

	condition.fLookAtPerc = conditionData.fLookAtOnPerc;
	String2States(conditionData.sUserPreActionState, condition.userPreActionStates );
	String2States(conditionData.sObjectPreActionState, condition.objectPreActionStates );
	condition.eActionType = conditionData.eActionType;
	condition.sAction = conditionData.sAction;
	condition.bEarlyPathRegeneration = conditionData.bEarlyPathRegeneration;
	String2States(conditionData.sUserPostActionState, condition.userPostActionStates );
	String2States(conditionData.sObjectPostActionState, condition.objectPostActionStates );

	condition.iMaxAlertness = conditionData.iMaxAlertness;
	condition.bEnabled = conditionData.bEnabled;
	condition.sName = conditionData.sName;
	condition.sDescription = conditionData.sDescription;
	condition.sFolder = conditionData.sFolder;
	condition.iOrder = conditionData.iOrder;
	condition.sEvent = conditionData.sEvent;
	condition.sChainedUserEvent = conditionData.sChainedUserEvent;
	condition.sChainedObjectEvent = conditionData.sChainedObjectEvent;

	condition.iRuleType = conditionData.iRuleType;
	condition.sEntranceHelper = conditionData.sEntranceHelper;
	condition.sExitHelper = conditionData.sExitHelper;

	condition.fApproachSpeed = conditionData.fApproachSpeed;
	condition.iApproachStance = conditionData.iApproachStance;
	condition.sAnimationHelper = conditionData.sAnimationHelper;
	condition.sApproachHelper = conditionData.sApproachHelper;
	condition.fStartWidth = conditionData.fStartWidth;
	condition.fDirectionTolerance = conditionData.fDirectionTolerance;
	condition.fStartArcAngle = conditionData.fStartArcAngle;

/*
	MapConditions::iterator it, itEnd = m_Conditions.end();
	for ( it = m_Conditions.begin(); it != itEnd; ++it )
		if ( condition.iOrder <= it->second.iOrder )
			++it->second.iOrder;
*/

	MapConditions::iterator where = m_Conditions.insert(std::make_pair( pUserClass, condition ));
	CCondition* pCondition = & where->second;

	if ( !condition.sEvent.empty() )
	{
		CEvent* pEvent = String2Event( condition.sEvent );
		pEvent->m_Conditions.insert(std::make_pair( pUserClass, pCondition ));
	}
	else if ( pCondition->bEnabled && pCondition->iRuleType == 0 )
	{
		// optimization: each user class should know what rules to use during update
		for ( int i = 0; i <= CLAMP( condition.iMaxAlertness, 0, 2 ); ++i )
			pUserClass->m_vActiveUpdateRules[i].push_back( pCondition );
	}
}

void CSmartObjectManager::ClearConditions()
{
	Reset();
	m_Conditions.clear();

	// only remove pointers to conditions
	// keep the events to preserve their descriptions
	MapEvents::iterator it = m_mapEvents.begin(), itEnd = m_mapEvents.end();
	MapEvents::iterator next;
	while ( it != itEnd )
	{
		next = it; ++next;
		if ( it->second.m_Conditions.size() )
		{
			it->second.m_Conditions.clear();
		}
		else
		{
			// an event not associated with any rule - delete the event
			m_mapEvents.erase( it );
		}
		it = next;
	}

	// clean the SOClasses pointing to active rules
	CSmartObjectClass::MapClassesByName::iterator itClasses = CSmartObjectClass::g_AllByName.begin();
	while ( itClasses != CSmartObjectClass::g_AllByName.end() )
	{
		itClasses->second->m_vActiveUpdateRules[0].clear();
		itClasses->second->m_vActiveUpdateRules[1].clear();
		itClasses->second->m_vActiveUpdateRules[2].clear();
		++itClasses;
	}
}

void CSmartObjectManager::Reset()
{
	CClassTemplateData::m_pHelperMtl = NULL;

	CSmartObjectClass::MapClassesByName::iterator itClasses = CSmartObjectClass::g_AllByName.begin();
	while ( itClasses != CSmartObjectClass::g_AllByName.end() )
	{
		delete (itClasses++)->second;
	}
	CSmartObjectClass::g_AllByName.clear();
	m_vDebugUse.clear();

	// re-register the "Temporary" class and assign it again to the temporary SO
	CSmartObjectClass* pTempSOClass = GetSmartObjectClass( "Temporary" );
	pTempSOClass->AddHelper( &g_pTempSORootHelper );
	g_pTemporarySO->GetClasses()[0] = pTempSOClass;

#ifdef AUTO_SMART_OBJECT
	m_autoSmartObjectList.clear();
#endif
}

void CSmartObjectManager::SoftReset()
{
	CClassTemplateData::m_pHelperMtl = NULL;

	CSmartObjectClass::MapClassesByName::iterator itClasses = CSmartObjectClass::g_AllByName.begin();
	while ( itClasses != CSmartObjectClass::g_AllByName.end() )
	{
		itClasses->second->m_MapObjectsByPos.clear();
		++itClasses;
	}
	m_vDebugUse.clear();
	//m_SmartObjects.clear();

	// re-register entities with the smart objects system
	SEntitySpawnParams params;
	IEntityIt* it = gEnv->pEntitySystem->GetEntityIterator();
	while ( !it->IsEnd() )
	{
		IEntity* pEntity = it->Next();
		DoRemove( pEntity );
		OnSpawn( pEntity, params );
	}
	it->Release();

	RebuildNavigation();
	m_bRecalculateUserSize = true;
}

void CSmartObjectManager::RecalculateUserSize()
{
	m_bRecalculateUserSize = false;

	// find navigation rules and add the size of the user to the used object class
	MapConditions::iterator itConditions, itConditionsEnd = m_Conditions.end();
	for ( itConditions = m_Conditions.begin(); itConditions != itConditionsEnd; ++itConditions )
	{
		// ignore event rules
		if ( !itConditions->second.sEvent.empty() )
			continue;

		CSmartObjectClass* pClass = itConditions->first;
		CCondition* pCondition = &itConditions->second;
		if ( pCondition->bEnabled && pCondition->pObjectClass && pCondition->iRuleType == 1 )
		{
			pCondition->pObjectClass->AddHelperLink( pCondition );
			pCondition->pObjectClass->AddNavUserClass( pCondition->pUserClass );
		}
	}
}

float CSmartObjectManager::CalculateDelayTime( CSmartObject* pUser, const Vec3& posUser,
										CSmartObject* pObject, const Vec3& posObject, CCondition* pCondition ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	if ( !pCondition->bEnabled )
		return -1.0f;

	if ( pUser == pObject )
	{
		// this is a relation on itself
		// just ignore all calculations (except randomness)

		float delta = 0.5f;

		// calculate randomness
		if ( pCondition->fRandomnessFactor )
		{
			delta *= 1.0f + ( pUser->m_fRandom + pObject->m_fRandom - 0.5f )*pCondition->fRandomnessFactor;
		}

		return pCondition->fMinDelay + (pCondition->fMaxDelay-pCondition->fMinDelay)*(1.0f-delta);
	}

	Vec3 direction = posObject - posUser;

	float limitFrom2 = pCondition->fDistanceFrom;
	float limitTo2 = pCondition->fDistanceTo;

//	if ( direction.x > limit2 || direction.x < -limit2 ||
//		 direction.y > limit2 || direction.y < -limit2 ||
//		 direction.z > limit2 || direction.z < -limit2 )
//		return -1.0f;

	limitFrom2 *= limitFrom2;
	limitTo2 *= limitTo2;
	float distance2 = direction.GetLengthSquared();

	if ( pCondition->fDistanceTo && (distance2 > limitTo2 || distance2 < limitFrom2) )
		return -1.0f;

	float dot = 2.0f;
	if ( pCondition->fOrientationLimit < 360.0f )
	{
		float cosLimit = 1.0f;
		if ( pCondition->fOrientationLimit != 0 )
			cosLimit = cosf( pCondition->fOrientationLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)

		dot = CalculateDot(pCondition, direction, pUser);

		if ( pCondition->fOrientationLimit < 0 )
		{
			dot = -dot;
			cosLimit = -cosLimit;
		}

		if ( dot < cosLimit )
			return -1.0f;
	}

	float delta = 0.0f;
	float offset = 0.0f;
	float divider = 0.0f;

	// calculate distance
	if ( pCondition->fProximityFactor )
	{
		delta += (1.0f - sqrtf(distance2/limitTo2)) * pCondition->fProximityFactor;
		if ( pCondition->fProximityFactor > 0 )
			divider += pCondition->fProximityFactor;
		else
		{
			offset -= pCondition->fProximityFactor;
			divider -= pCondition->fProximityFactor;
		}
	}

	// calculate orientation
	if ( pCondition->fOrientationFactor )
	{
		// TODO: Could be optimized
		float cosLimit = -1.0f;
		if ( pCondition->fOrientationLimit != 0 && pCondition->fOrientationLimit < 360.0f && pCondition->fOrientationLimit >= -360.0f )
			cosLimit = cosf( pCondition->fOrientationLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)
		if ( pCondition->fOrientationLimit < 0 )
			cosLimit = -cosLimit;

		if ( dot == 2.0f )
		{
			dot = CalculateDot(pCondition, direction, pUser);
			if ( pCondition->fOrientationLimit < 0 )
				dot = -dot;
		}

		float factor;
		if ( abs(pCondition->fOrientationLimit) <= 0.01f )
			factor = dot >= 0.99f ? 1.0f : 0.0f;
		else
			factor = (dot - cosLimit) / (1.0f - cosLimit);

		delta += factor * pCondition->fOrientationFactor;
		if ( pCondition->fOrientationFactor > 0 )
			divider += pCondition->fOrientationFactor;
		else
		{
			offset -= pCondition->fOrientationFactor;
			divider -= pCondition->fOrientationFactor;
		}
	}

	// calculate visibility
	if ( pCondition->fVisibilityFactor )
	{
		CPuppet* pPuppet = pUser->GetPuppet();
		if ( pPuppet && IAIObject::eFOV_Outside != pPuppet->IsPointInFOV(posObject) ) // is in visual range?
			if (GetAISystem()->CheckPointsVisibility( posUser, posObject, 0, pUser->GetPhysics(), pObject->GetPhysics() )) // is physically visible?
				delta += pCondition->fVisibilityFactor;
		if ( pCondition->fVisibilityFactor > 0 )
			divider += pCondition->fVisibilityFactor;
		else
		{
			offset -= pCondition->fVisibilityFactor;
			divider -= pCondition->fVisibilityFactor;
		}
	}

	if ( !pCondition->fProximityFactor && !pCondition->fOrientationFactor && !pCondition->fVisibilityFactor )
		delta = 0.5f;

	// calculate randomness
	if ( pCondition->fRandomnessFactor )
	{
		delta *= 1.0f + ( pUser->m_fRandom + pObject->m_fRandom - 0.5f )*pCondition->fRandomnessFactor;
	}

	if ( divider )
		delta = (delta+offset) / divider;

	return pCondition->fMinDelay + (pCondition->fMaxDelay-pCondition->fMinDelay)*(1.0f-delta);
}

/*
 * follows a try to update smart objects by condition
 *

void CSmartObjectManager::Update()
{
	m_UpdateStruct.m_Count = 100;

	MapConditions::const_iterator itConditionsEnd = m_Conditions.end();
	MapConditions::iterator & itCondition = m_UpdateStruct.m_itCondition;

	while ( m_UpdateStruct.m_Count-- > 0 )
	{
		if ( itCondition == itConditionsEnd )
			itCondition = m_Conditions.begin();
		if ( itCondition != itConditionsEnd )
		{
			const PairClassState& pairClassState = itCondition->first;
			CCondition* pCondition = &itCondition->second;
			if ( pCondition->bEnabled )
				UpdateCondition( pairClassState.first, pairClassState.second, pCondition );
			++itCondition;
			if ( itCondition != itConditionsEnd )
				break;
		}
		else
			break;
	}
}

void CSmartObjectManager::UpdateCondition( CSmartObjectClass* pClass, CSmartObject::CState state, CCondition* pCondition )
{
	MapSmartObjectsByClass::const_iterator itByClassEnd = m_SmartObjects.end();
	MapSmartObjectsByClass::iterator & itByClass = m_UpdateStruct.m_itByClass;

	while ( m_UpdateStruct.m_Count-- > 0 && itByClass != itByClassEnd && itByClass->first == pClass )
	{
		MapSmartObjectsByState & mapByState = itByClass->second;

		MapSmartObjectsByState::const_iterator itByStateEnd = mapByState.end();
		MapSmartObjectsByState::iterator & itByState = m_UpdateStruct.m_itByState;

		while ( m_UpdateStruct.m_Count-- > 0 && itByState != itByStateEnd && itByState->first == state )
		{
			CSmartObject* pSmartObjectUser = itByState->second;

			if ( pCondition->iMaxAlertness < 2 )
			{
				CAIObject* pAIObject = pSmartObjectUser->GetAIObject();
				if ( pAIObject )
				{
					int alertness = pAIObject->m_bEnabled ? pAIObject->GetProxy()->GetAlertnessState() : 1000;
					if ( alertness > pCondition->iMaxAlertness )
						continue;
				}
			}

			UpdateUser( pSmartObjectUser, pCondition );
		}
	}
}

void CSmartObjectManager::UpdateUser( CSmartObject* pUser, CCondition* pCondition )
{
	MapSmartObjectsByClass::iterator itClass = m_SmartObjects.find( pCondition->pObjectClass );
	if ( itClass != m_SmartObjects.end() )
	{
		Vec3 pos = pUser->GetPos();
		Vec3 bbMin = pos;
		Vec3 bbMax = pos;
		if ( pCondition->fDistanceLimit > 0 )
		{
			Vec3 d( pCondition->fDistanceLimit, pCondition->fDistanceLimit, pCondition->fDistanceLimit );
			bbMin -= d;
			bbMax += d;
		}

		MapSmartObjectsByState mapByState = itClass->second;

		MapSmartObjectsByState::const_iterator itByStateEnd = mapByState.end();
		MapSmartObjectsByState::iterator & itObject = m_UpdateStruct.m_itObject;

		if ( itObject == itByStateEnd )
			itObject = mapByState.find( pCondition->objectState );

		while ( m_UpdateStruct.m_Count-- > 0 && itObject != itByStateEnd && itObject->first == pCondition->objectState )
		{
			CSmartObject* pObject = itObject->second;

			// Check range first since it could be faster
			if ( pCondition->fDistanceLimit > 0 )
			{
				Vec3 objectPos = pObject->GetPos();
				if ( objectPos.x < bbMin.x || objectPos.x > bbMax.x ||
					objectPos.y < bbMin.y || objectPos.y > bbMax.y ||
					objectPos.z < bbMin.z || objectPos.z > bbMax.z )
						continue;
			}

			// Check the geometry model if needed
			if ( !pCondition->sObjectModel.empty() )
			{
				string strModel = pObject->GetModel();
				strModel.replace( '\\', '/' );
				if ( !strModel || stricmp( pCondition->sObjectModel, strModel ) )
					continue;
			}

			UpdateObject( pUser, pObject, pCondition );
		}
	}
}

void CSmartObjectManager::UpdateObject( CSmartObject* pUser, CSmartObject* pObject, CCondition* pCondition )
{
	// define a map of event updates
	typedef std::pair< CSmartObject*, CCondition* > PairObjectCondition;
	typedef std::map< PairObjectCondition, float > MapDelayTimes;
	MapDelayTimes mapDelayTimes;

	// calculated delta times should be stored only for now.
	// later existing events will be updated and for new objects new events will be added
	PairObjectCondition poc( pObject, pCondition );
	float fDelayTime = CalculateDelayTime( pUser, pObject, pCondition );
	if ( fDelayTime > 0.0f )
		mapDelayTimes.insert(std::make_pair( poc, fTimeElapsed/fDelayTime ));
	else if ( fDelayTime == 0.0f )
		mapDelayTimes.insert(std::make_pair( poc, 1.0f ));
}

*
* end of updating smart objects by condition
*/

int CSmartObjectManager::TriggerEvent( const char* sEventName, IEntity*& pUser, IEntity*& pObject, QueryEventMap* pQueryEvents /*= NULL*/, const Vec3* pExtraPoint /*= NULL*/, bool bHighPriority /*= false*/ )
{
	if(!IsInitialized())
		return 0;

	if ( !sEventName || !*sEventName )
		return 0;

	CSmartObject* pSOUser = NULL;
	if ( pUser )
	{
		if ( pUser->IsHidden() )
		{
			// ignore hidden entities
			return 0;
		}
		pSOUser = IsSmartObject( pUser );
		if ( !pSOUser )
			return 0; // requested user is not a smart object
	}
	CSmartObject* pSOObject = NULL;
	if ( pObject )
	{
		if ( pObject == TEMPORARY_SO_ENTITY_PTR )
		{
			// temporary smart object
			pSOObject = GetTemporarySmartObject();
		}
		else
		{
			if ( pObject->IsHidden() )
			{
				// ignore hidden entities
				return 0;
			}
			pSOObject = IsSmartObject( pObject );
			if ( !pSOObject )
				return 0; // requested object is not a smart object
		}
	}

	if ( pSOUser && pSOObject )
		return TriggerEventUserObject( sEventName, pSOUser, pSOObject, pQueryEvents, pExtraPoint, bHighPriority );
	else if ( pSOUser )
		return TriggerEventUser( sEventName, pSOUser, pQueryEvents, &pObject, pExtraPoint, bHighPriority );
	else if ( pSOObject )
		return TriggerEventObject( sEventName, pSOObject, pQueryEvents, &pUser, pExtraPoint, bHighPriority );

	// specific user or object is not requested
	// check the rules on all users and objects

	float minDelay = FLT_MAX;
	CCondition* pMinRule = NULL;
	CSmartObject* pMinUser = NULL;
	CSmartObject* pMinObject = NULL;
	CEvent* pEvent = String2Event( sEventName );

	MapPtrConditions::iterator itRules, itRulesEnd = pEvent->m_Conditions.end();
	for ( itRules = pEvent->m_Conditions.begin(); itRules != itRulesEnd; ++itRules )
	{
		CCondition* pRule = itRules->second;
		if ( !pRule->bEnabled )
			continue;

		// if we already have found a rule with a delay less than min. delay of this rule then ignore this rule
		if ( !pQueryEvents && minDelay < pRule->fMinDelay )
			continue;

		CSmartObjectClass::MapSmartObjectsByPos& mapByPos = pRule->pUserClass->m_MapObjectsByPos;
		if ( mapByPos.empty() )
			continue;

		CSmartObjectClass::MapSmartObjectsByPos::iterator itByPos, itByPosEnd = mapByPos.end();
		for ( itByPos = mapByPos.begin(); itByPos != itByPosEnd; ++itByPos )
		{
			pSOUser = itByPos->second;

			// ignore hidden entities
			if ( pSOUser->IsHidden() )
				continue;

			// proceed with next user if this one doesn't match states
			if ( !pRule->userStatePattern.Matches( pSOUser->GetStates() ) )
				continue;

			// now for this user check all objects matching the rule's conditions

			Vec3 soPos = pSOUser->GetPos();
			CPuppet* pPuppet = pSOUser->GetPuppet();
			IAIObject* pAttTarget = pPuppet ? pPuppet->GetAttentionTarget() : NULL;

			// ignore this user if it has no attention target and the rule says it's needed
			if ( !pExtraPoint && !pAttTarget && pRule->fOrientationToTargetLimit < 360.0f )
				continue;

			int alertness = pPuppet ? (pPuppet->IsEnabled() ? pPuppet->GetProxy()->GetAlertnessState() : 1000) : 0;
			if ( alertness > pRule->iMaxAlertness )
				continue; // ignore this user - too much alerted

			// first check does it maybe operate only on itself
			if ( !pRule->pObjectClass )
			{
				// calculate delay time
				float fDelayTime = CalculateDelayTime( pSOUser, soPos, pSOUser, soPos, pRule );
				if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
				{
					// mark this as best
					minDelay = fDelayTime;
					pMinUser = pMinObject = pSOUser;
					pMinRule = pRule;
				}

				// add it to the query list
				if ( pQueryEvents && fDelayTime >= 0.0f )
				{
					CQueryEvent q;
					q.pUser = pSOUser;
					q.pObject = pSOUser;
					q.pRule = pRule;
					q.pChainedUserEvent = NULL;
					q.pChainedObjectEvent = NULL;
					pQueryEvents->insert( std::make_pair(fDelayTime, q) );
				}

				// proceed with next user
				continue;
			}

			// get species and group id of the user
			int species = -1;
			int groupId = -1;
			CAIActor* pUserActor = pSOUser->GetAIActor();
			if ( pUserActor )
			{
				groupId = pUserActor->GetGroupId();
				species = pUserActor->GetParameters().m_nSpecies;
			}

			// adjust pos if pUserHelper is used
			Vec3 pos = pRule->pUserHelper ? pSOUser->GetHelperPos( pRule->pUserHelper ) : soPos;

			Vec3 bbMin = pos;
			Vec3 bbMax = pos;

			float limitTo = pRule->fDistanceTo;
			// adjust limit if pUserHelper is used
			if ( pRule->pUserHelper )
				limitTo += pRule->pUserHelper->qt.t.GetLength();
			// adjust limit if pObjectHelper is used
			if ( pRule->pObjectHelper )
				limitTo += pRule->pObjectHelper->qt.t.GetLength();
			Vec3 d( limitTo, limitTo, limitTo );
			bbMin -= d;
			bbMax += d;

			// calculate the limit in advance
			float orientationToTargetLimit = -2.0f; // unlimited
			if ( pRule->fOrientationToTargetLimit < 360.0f )
			{
				if ( pRule->fOrientationToTargetLimit == 0 )
					orientationToTargetLimit = 1.0f;
				else
					orientationToTargetLimit = cosf( pRule->fOrientationToTargetLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)

				if ( pRule->fOrientationToTargetLimit < 0 )
					orientationToTargetLimit = -orientationToTargetLimit;
			}

			// check all objects (but not the user) matching with condition's class and state
			CSmartObjectClass::MapSmartObjectsByPos& mapObjectByPos = pRule->pObjectClass->m_MapObjectsByPos;
			if ( mapObjectByPos.empty() )
				continue;

			CSmartObjectClass::MapSmartObjectsByPos::iterator itObjectByPos, itObjectByPosEnd = mapObjectByPos.upper_bound( bbMax.x );
			for ( itObjectByPos = mapObjectByPos.lower_bound( bbMin.x ); itObjectByPos != itObjectByPosEnd; ++itObjectByPos )
			{
				pSOObject = itObjectByPos->second;

				// the user must be different than target object!!!
				if ( pSOUser == pSOObject )
					continue;

				// ignore hidden entities
				if ( pSOObject->IsHidden() )
					continue;

				// Check range first since it could be faster
				Vec3 objectPos = pRule->pObjectHelper ? pSOObject->GetHelperPos( pRule->pObjectHelper ) : pSOObject->GetPos();
				assert( bbMin.x-limitTo <= objectPos.x && objectPos.x <= bbMax.x+limitTo );
				if ( objectPos.y < bbMin.y || objectPos.y > bbMax.y ||
					objectPos.z < bbMin.z || objectPos.z > bbMax.z )
						continue;

				// Also check the orientation limit to user's attention target
				if ( pRule->fOrientationToTargetLimit < 360.0f )
				{
					Vec3 objectDir = pSOObject->GetOrientation( pRule->pObjectHelper );
					Vec3 targetDir = (pExtraPoint ? *pExtraPoint : pAttTarget->GetPos()) - objectPos;
					targetDir.NormalizeSafe();
					float dot = objectDir.Dot( targetDir );

					if ( pRule->fOrientationToTargetLimit < 0 )
						dot = -dot;

					if ( dot < orientationToTargetLimit )
						continue;
				}

				// add virtual states
//				IAIObject* pAIObjectObject = pSOObject->GetAI();
				CAIActor* pAIObjectObject = pSOObject->GetAIActor();
				bool attTarget = false;
				bool sameGroupId = false;
				bool sameSpecies = false;
				if ( pAIObjectObject )
				{
					// check is the object attention target of the user
					attTarget = pPuppet && pPuppet->GetAttentionTarget() == pSOObject->GetAI();		//pAIObjectObject;
					if ( attTarget )
						pSOObject->m_States.insert( m_StateAttTarget );

					// check are the user and the object in the same group and species
					if ( groupId >= 0 || species >= 0 )
					{
						// check same species
						int spObject = pAIObjectObject->GetParameters().m_nSpecies;
						sameSpecies = species == spObject;
						if ( sameSpecies )
						{
							pSOObject->m_States.insert( m_StateSameSpecies );

							// if they are same species check are they in same group
							sameGroupId = groupId == pAIObjectObject->GetGroupId();
							if ( sameGroupId )
								pSOObject->m_States.insert( m_StateSameGroup );
						}
						else if ( species < 0 || spObject < 0 )
						{
							// if any of them has no species check are they in same group
							sameGroupId = groupId == pAIObjectObject->GetGroupId();
							if ( sameGroupId )
								pSOObject->m_States.insert( m_StateSameGroup );
						}
					}
				}

				// check object's state pattern and then remove virtual states
				bool bMatches = pRule->objectStatePattern.Matches( pSOObject->GetStates() );
				if ( attTarget )
					pSOObject->m_States.erase( m_StateAttTarget );
				if ( sameSpecies )
					pSOObject->m_States.erase( m_StateSameSpecies );
				if ( sameGroupId )
					pSOObject->m_States.erase( m_StateSameGroup );

				// ignore this object if it doesn't match precondition state
				if ( !bMatches )
					continue;

				// calculate delay time
				float fDelayTime = CalculateDelayTime( pSOUser, pos, pSOObject, objectPos, pRule );
				if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
				{
					minDelay = fDelayTime;
					pMinUser = pSOUser;
					pMinObject = pSOObject;
					pMinRule = pRule;
				}

				// add it to the query list
				if ( pQueryEvents && fDelayTime >= 0.0f )
				{
					CQueryEvent q;
					q.pUser = pSOUser;
					q.pObject = pSOObject;
					q.pRule = pRule;
					q.pChainedUserEvent = NULL;
					q.pChainedObjectEvent = NULL;
					pQueryEvents->insert( std::make_pair(fDelayTime, q) );
				}
			}
		}
	}

	if ( !pMinRule )
		return 0;

	// is querying only?
	if ( pQueryEvents )
		return -1;

	int id = GetAISystem()->AllocGoalPipeId();
	UseSmartObject( pMinUser, pMinObject, pMinRule, id, bHighPriority );
	pUser = pMinUser->GetEntity();
	pObject = pMinObject ? pMinObject->GetEntity() : NULL;
	return !pMinRule->sAction.empty() && pMinRule->eActionType != eAT_None ? id : -1;
}

int CSmartObjectManager::TriggerEventUserObject( const char* sEventName, CSmartObject* pUser, CSmartObject* pObject, QueryEventMap* pQueryEvents, const Vec3* pExtraPoint, bool bHighPriority /*=false*/ )
{
	float minDelay = FLT_MAX;
	CCondition* pMinRule = NULL;
	CEvent* pEvent = String2Event( sEventName );

	CPuppet* pPuppet = pUser->GetPuppet();
	IAIObject* pAttTarget = pPuppet ? pPuppet->GetAttentionTarget() : NULL;
	int alertness = pPuppet ? (pPuppet->IsEnabled() ? pPuppet->GetProxy()->GetAlertnessState() : 1000) : 0;

	CSmartObjectClasses::iterator itUserClasses, itUserClassesEnd = pUser->GetClasses().end();
	for ( itUserClasses = pUser->GetClasses().begin(); itUserClasses != itUserClassesEnd; ++itUserClasses )
	{
		CSmartObjectClass* pUserClass = *itUserClasses;
		MapPtrConditions::iterator itRules, itRulesEnd = pEvent->m_Conditions.upper_bound( pUserClass );
		for ( itRules = pEvent->m_Conditions.lower_bound( pUserClass ); itRules != itRulesEnd; ++itRules )
		{
			CCondition* pRule = itRules->second;
			if ( !pRule->bEnabled )
				continue;

			// if we already have found a rule with a delay less than min. delay of this rule then ignore this rule
			if ( !pQueryEvents && minDelay < pRule->fMinDelay )
				continue;

			// check interaction and rule types
			if ( pUser != pObject && !pRule->pObjectClass )
				continue;
			if ( pUser == pObject && pRule->pObjectClass )
				continue;

			// proceed with the next rule if user doesn't match states with this one
			if ( !pRule->userStatePattern.Matches( pUser->GetStates() ) )
				continue;

			if ( alertness > pRule->iMaxAlertness )
				continue; // ignore this rule - too much alerted

			// ignore this user if it has no attention target and the rule says it's needed
			if ( !pExtraPoint && !pAttTarget && pRule->fOrientationToTargetLimit < 360.0f )
				continue;

			// adjust pos if helpers are used
			Vec3 pos = pRule->pUserHelper ? pUser->GetHelperPos( pRule->pUserHelper ) : pUser->GetPos();

			// now for this user check the rule with requested object

			if ( pUser == pObject )
			{
				// calculate delay time
				float fDelayTime = CalculateDelayTime( pUser, pos, pUser, pos, pRule );
				if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
				{
					minDelay = fDelayTime;
					pMinRule = pRule;
				}

				// add it to the query list
				if ( pQueryEvents && fDelayTime >= 0.0f )
				{
					CQueryEvent q;
					q.pUser = pUser;
					q.pObject = pUser;
					q.pRule = pRule;
					q.pChainedUserEvent = NULL;
					q.pChainedObjectEvent = NULL;
					pQueryEvents->insert( std::make_pair(fDelayTime, q) );
				}

				continue;
			}

			// ignore rules which object's class is different
			CSmartObjectClasses& objectClasses = pObject->GetClasses();
			if ( std::find( objectClasses.begin(), objectClasses.end(), pRule->pObjectClass ) == objectClasses.end() )
				continue;

			// adjust pos if helpers are used
			Vec3 objectPos = pRule->pObjectHelper ? pObject->GetHelperPos( pRule->pObjectHelper ) : pObject->GetPos();

			// check the orientation limit to target
			if ( pRule->fOrientationToTargetLimit < 360.0f )
			{
				float cosLimit = 1.0f;
				if ( pRule->fOrientationToTargetLimit != 0 )
					cosLimit = cosf( pRule->fOrientationToTargetLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)

				Vec3 objectDir = pObject->GetOrientation( pRule->pObjectHelper );
				Vec3 targetDir = (pExtraPoint ? *pExtraPoint : pAttTarget->GetPos()) - objectPos;
				targetDir.NormalizeSafe();
				float dot = objectDir.Dot( targetDir );

				if ( pRule->fOrientationToTargetLimit < 0 )
				{
					dot = -dot;
					cosLimit = -cosLimit;
				}

				if ( dot < cosLimit )
					continue;
			}

			// get species and group id of the user
			int species = -1;
			int groupId = -1;
			CAIActor* pUserActor = pUser->GetAIActor();
			if ( pUserActor )
			{
				groupId = pUserActor->GetGroupId();
				species = pUserActor->GetParameters().m_nSpecies;
			}

			// add virtual states
			CAIActor* pAIObjectObject = pObject->GetAIActor();
			bool attTarget = false;
			bool sameGroupId = false;
			bool sameSpecies = false;
			if ( pAIObjectObject )
			{
				// check is the object attention target of the user
				attTarget = pPuppet && pPuppet->GetAttentionTarget() == pObject->GetAI();
				if ( attTarget )
					pObject->m_States.insert( m_StateAttTarget );

				// check are the user and the object in the same group and species
				if ( groupId >= 0 || species >= 0 )
				{
					// check same species
					int spObject = pAIObjectObject->GetParameters().m_nSpecies;
					sameSpecies = species == spObject;
					if ( sameSpecies )
					{
						pObject->m_States.insert( m_StateSameSpecies );

						// if they are same species check are they in same group
						sameGroupId = groupId == pAIObjectObject->GetGroupId();
						if ( sameGroupId )
							pObject->m_States.insert( m_StateSameGroup );
					}
					else if ( species < 0 || spObject < 0 )
					{
						// if any of them has no species check are they in same group
						sameGroupId = groupId == pAIObjectObject->GetGroupId();
						if ( sameGroupId )
							pObject->m_States.insert( m_StateSameGroup );
					}
				}
			}

			// check object's state pattern and then remove virtual states
			bool bMatches = pRule->objectStatePattern.Matches( pObject->GetStates() );
			if ( attTarget )
				pObject->m_States.erase( m_StateAttTarget );
			if ( sameSpecies )
				pObject->m_States.erase( m_StateSameSpecies );
			if ( sameGroupId )
				pObject->m_States.erase( m_StateSameGroup );

			// ignore this object if it doesn't match precondition state
			if ( !bMatches )
				continue;

			// calculate delay time
			float fDelayTime = CalculateDelayTime( pUser, pos, pObject, objectPos, pRule );
			if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
			{
				minDelay = fDelayTime;
				pMinRule = pRule;
			}

			// add it to the query list
			if ( pQueryEvents && fDelayTime >= 0.0f )
			{
				CQueryEvent q;
				q.pUser = pUser;
				q.pObject = pObject;
				q.pRule = pRule;
				q.pChainedUserEvent = NULL;
				q.pChainedObjectEvent = NULL;
				pQueryEvents->insert( std::make_pair(fDelayTime, q) );
			}
		}
	}

	if ( !pMinRule )
		return 0;

	// is querying only?
	if ( pQueryEvents )
		return -1;

	int id = GetAISystem()->AllocGoalPipeId();
	UseSmartObject( pUser, pObject, pMinRule, id, bHighPriority );
	return !pMinRule->sAction.empty() && pMinRule->eActionType != eAT_None ? id : -1;
}

int CSmartObjectManager::TriggerEventUser( const char* sEventName, CSmartObject* pUser, QueryEventMap* pQueryEvents, IEntity** ppObjectEntity, const Vec3* pExtraPoint, bool bHighPriority /*=false*/ )
{
	*ppObjectEntity = NULL;
	CSmartObject* pObject = NULL;

	// check the rules for all objects

	float minDelay = FLT_MAX;
	CCondition* pMinRule = NULL;
	CSmartObject* pMinObject = NULL;
	CEvent* pEvent = String2Event( sEventName );

	CSmartObjectClasses::iterator itUserClasses, itUserClassesEnd = pUser->GetClasses().end();
	for ( itUserClasses = pUser->GetClasses().begin(); itUserClasses != itUserClassesEnd; ++itUserClasses )
	{
		CSmartObjectClass* pUserClass = *itUserClasses;

		MapPtrConditions::iterator itRules, itRulesEnd = pEvent->m_Conditions.upper_bound( pUserClass );
		for ( itRules = pEvent->m_Conditions.lower_bound( pUserClass ); itRules != itRulesEnd; ++itRules )
		{
			CCondition* pRule = itRules->second;
			if ( !pRule->bEnabled )
				continue;

			// if we already have found a rule with a delay less than min. delay of this rule then ignore this rule
			if ( !pQueryEvents && minDelay < pRule->fMinDelay )
				continue;

			// proceed with next rule if the user doesn't match states
			if ( !pRule->userStatePattern.Matches( pUser->GetStates() ) )
				continue;

			Vec3 soPos = pUser->GetPos();

			CPuppet* pPuppet = pUser->GetPuppet();
			IAIObject* pAttTarget = pPuppet ? pPuppet->GetAttentionTarget() : NULL;

			// ignore this rule if the user has no attention target and the rule says it's needed
			if ( !pExtraPoint && !pAttTarget && pRule->fOrientationToTargetLimit < 360.0f )
				continue;

			int alertness = pPuppet ? (pPuppet->IsEnabled() ? pPuppet->GetProxy()->GetAlertnessState() : 1000) : 0;
			if ( alertness > pRule->iMaxAlertness )
				continue; // ignore this rule - too much alerted

			// now for this user check all objects matching the rule's conditions

			// first check does it maybe operate only on itself
			if ( !pRule->pObjectClass )
			{
				// calculate delay time
				float fDelayTime = CalculateDelayTime( pUser, soPos, pUser, soPos, pRule );
				if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
				{
					// mark this as best
					minDelay = fDelayTime;
					pMinObject = pUser;
					pMinRule = pRule;
				}

				// add it to the query list
				if ( pQueryEvents && fDelayTime >= 0.0f )
				{
					CQueryEvent q;
					q.pUser = pUser;
					q.pObject = pUser;
					q.pRule = pRule;
					q.pChainedUserEvent = NULL;
					q.pChainedObjectEvent = NULL;
					pQueryEvents->insert( std::make_pair(fDelayTime, q) );
				}

				// proceed with next rule
				continue;
			}

			// get species and group id of the user
			int species = -1;
			int groupId = -1;
			CAIActor* pUserActor = pUser->GetAIActor();
			if ( pUserActor )
			{
				groupId = pUserActor->GetGroupId();
				species = pUserActor->GetParameters().m_nSpecies;
			}

			// adjust pos if pUserHelper is used
			Vec3 pos = pRule->pUserHelper ? pUser->GetHelperPos( pRule->pUserHelper ) : soPos;

			Vec3 bbMin = pos;
			Vec3 bbMax = pos;

			float limitTo = pRule->fDistanceTo;
			// adjust limit if pUserHelper is used
			if ( pRule->pUserHelper)
				limitTo += pRule->pUserHelper->qt.t.GetLength();
			// adjust limit if pObjectHelper is used
			if ( pRule->pObjectHelper )
				limitTo += pRule->pObjectHelper->qt.t.GetLength();
			Vec3 d( limitTo, limitTo, limitTo );
			bbMin -= d;
			bbMax += d;

			// calculate the limit in advance
			float orientationToTargetLimit = -2.0f; // unlimited
			if ( pRule->fOrientationToTargetLimit < 360.0f )
			{
				if ( pRule->fOrientationToTargetLimit == 0 )
					orientationToTargetLimit = 1.0f;
				else
					orientationToTargetLimit = cosf( pRule->fOrientationToTargetLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)

				if ( pRule->fOrientationToTargetLimit < 0 )
					orientationToTargetLimit = -orientationToTargetLimit;
			}

			// check all objects (but not the user) matching with condition's class and state
			CSmartObjectClass::MapSmartObjectsByPos& mapObjectByPos = pRule->pObjectClass->m_MapObjectsByPos;
			if ( mapObjectByPos.empty() )
				continue;

			CSmartObjectClass::MapSmartObjectsByPos::iterator itObjectByPos, itObjectByPosEnd = mapObjectByPos.upper_bound( bbMax.x );
			for ( itObjectByPos = mapObjectByPos.lower_bound( bbMin.x ); itObjectByPos != itObjectByPosEnd; ++itObjectByPos )
			{
				pObject = itObjectByPos->second;

				// the user can not be the target object!!!
				if ( pUser == pObject )
					continue;

				// ignore hidden entities
				if ( pObject->IsHidden() )
					continue;

				// Check range first since it could be faster
				Vec3 objectPos = pRule->pObjectHelper ? pObject->GetHelperPos( pRule->pObjectHelper ) : pObject->GetPos();
				assert( bbMin.x-limitTo <= objectPos.x && objectPos.x <= bbMax.x+limitTo );
				if ( objectPos.y < bbMin.y || objectPos.y > bbMax.y ||
					objectPos.z < bbMin.z || objectPos.z > bbMax.z )
						continue;

				// Also check the orientation limit to user's attention target
				if ( pRule->fOrientationToTargetLimit < 360.0f )
				{
					Vec3 objectDir = pObject->GetOrientation( pRule->pObjectHelper );
					Vec3 targetDir = (pExtraPoint ? *pExtraPoint : pAttTarget->GetPos()) - objectPos;
					targetDir.NormalizeSafe();
					float dot = objectDir.Dot( targetDir );

					if ( pRule->fOrientationToTargetLimit < 0 )
						dot = -dot;

					if ( dot < orientationToTargetLimit )
						continue;
				}

				// add virtual states
//				IAIObject* pAIObjectObject = pObject->GetAI();
				CAIActor* pAIObjectObject = pObject->GetAIActor();
				bool attTarget = false;
				bool sameGroupId = false;
				bool sameSpecies = false;
				if ( pAIObjectObject )
				{
					// check is the object attention target of the user
					attTarget = pPuppet && pPuppet->GetAttentionTarget() == pObject->GetAI();
					if ( attTarget )
						pObject->m_States.insert( m_StateAttTarget );

					// check are the user and the object in the same group and species
					if ( groupId >= 0 || species >= 0 )
					{
						// check same species
						int spObject = pAIObjectObject->GetParameters().m_nSpecies;
						sameSpecies = species == spObject;
						if ( sameSpecies )
						{
							pObject->m_States.insert( m_StateSameSpecies );

							// if they are same species check are they in same group
							sameGroupId = groupId == pAIObjectObject->GetGroupId();
							if ( sameGroupId )
								pObject->m_States.insert( m_StateSameGroup );
						}
						else if ( species < 0 || spObject < 0 )
						{
							// if any of them has no species check are they in same group
							sameGroupId = groupId == pAIObjectObject->GetGroupId();
							if ( sameGroupId )
								pObject->m_States.insert( m_StateSameGroup );
						}
					}
				}

				// check object's state pattern and then remove virtual states
				bool bMatches = pRule->objectStatePattern.Matches( pObject->GetStates() );
				if ( attTarget )
					pObject->m_States.erase( m_StateAttTarget );
				if ( sameSpecies )
					pObject->m_States.erase( m_StateSameSpecies );
				if ( sameGroupId )
					pObject->m_States.erase( m_StateSameGroup );

				// ignore this object if it doesn't match precondition state
				if ( !bMatches )
					continue;

				// calculate delay time
				float fDelayTime = CalculateDelayTime( pUser, pos, pObject, objectPos, pRule );
				if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
				{
					minDelay = fDelayTime;
					pMinObject = pObject;
					pMinRule = pRule;
				}

				// add it to the query list
				if ( pQueryEvents && fDelayTime >= 0.0f )
				{
					CQueryEvent q;
					q.pUser = pUser;
					q.pObject = pObject;
					q.pRule = pRule;
					q.pChainedUserEvent = NULL;
					q.pChainedObjectEvent = NULL;
					pQueryEvents->insert( std::make_pair(fDelayTime, q) );
				}
			}
		}
	}

	if ( !pMinRule )
		return 0;

	// is querying only?
	if ( pQueryEvents )
		return -1;

	int id = GetAISystem()->AllocGoalPipeId();
	UseSmartObject( pUser, pMinObject, pMinRule, id, bHighPriority );
	*ppObjectEntity = pMinObject->GetEntity();
	return !pMinRule->sAction.empty() && pMinRule->eActionType != eAT_None ? id : -1;
}

int CSmartObjectManager::TriggerEventObject( const char* sEventName, CSmartObject* pObject, QueryEventMap* pQueryEvents, IEntity** ppUserEntity, const Vec3* pExtraPoint, bool bHighPriority /*=false*/ )
{
	// WARNING: Virtual states (AttTarget, SameGroup and SameSpecies) in this function are ignored!!!
	// TODO: Add virtual states...

	if ( !sEventName || !*sEventName )
		return 0;

	*ppUserEntity = NULL;
	CSmartObject* pUser = NULL;

	// check the rules on all users

	float minDelay = FLT_MAX;
	CCondition* pMinRule = NULL;
	CSmartObject* pMinUser = NULL;
	CEvent* pEvent = String2Event( sEventName );

	MapPtrConditions::iterator itRules, itRulesEnd = pEvent->m_Conditions.end();
	for ( itRules = pEvent->m_Conditions.begin(); itRules != itRulesEnd; ++itRules )
	{
		CCondition* pRule = itRules->second;
		if ( !pRule->bEnabled )
			continue;

		// if we already have found a rule with a delay less than min. delay of this rule then ignore this rule
		if ( !pQueryEvents && minDelay < pRule->fMinDelay )
			continue;

		// ignore rules which operate only on the user (without an object)
		if ( !pRule->pObjectClass )
			continue;

		// ignore rules which object's class is different
		if ( std::find( pObject->GetClasses().begin(), pObject->GetClasses().end(), pRule->pObjectClass ) == pObject->GetClasses().end() )
			continue;

		// proceed with next rule if this one doesn't match object states
		if ( !pRule->objectStatePattern.Matches( pObject->GetStates() ) )
			continue;

		// proceed with next rule if there are no users
		CSmartObjectClass::MapSmartObjectsByPos& mapUserByPos = pRule->pUserClass->m_MapObjectsByPos;
		if ( mapUserByPos.empty() )
			continue;

		// get object's position
		Vec3 objectPos = pObject->GetPos();

		// adjust pos if pUserHelper is used
		Vec3 pos = pRule->pObjectHelper ? pObject->GetHelperPos( pRule->pObjectHelper ) : objectPos;

		Vec3 bbMin = pos;
		Vec3 bbMax = pos;

		float limitTo = pRule->fDistanceTo;
		// adjust limit if pUserHelper is used
		if ( pRule->pUserHelper)
			limitTo += pRule->pUserHelper->qt.t.GetLength();
		// adjust limit if pObjectHelper is used
		if ( pRule->pObjectHelper )
			limitTo += pRule->pObjectHelper->qt.t.GetLength();
		Vec3 d( limitTo, limitTo, limitTo );
		bbMin -= d;
		bbMax += d;

		CSmartObjectClass::MapSmartObjectsByPos::iterator itUserByPos, itUserByPosEnd = mapUserByPos.upper_bound( bbMax.x );
		for ( itUserByPos = mapUserByPos.lower_bound( bbMin.x ); itUserByPos != itUserByPosEnd; ++itUserByPos )
		{
			pUser = itUserByPos->second;

			// now check does this user can use the object

			// don't let the user to be the object
			if ( pUser == pObject )
				continue;

			// ignore hidden entities
			if ( pUser->IsHidden() )
				continue;

			// proceed with next user if this one doesn't match states
			if ( !pRule->userStatePattern.Matches( pUser->GetStates() ) )
				continue;

			CPuppet* pPuppet = pUser->GetPuppet();
			IAIObject* pAttTarget = pPuppet ? pPuppet->GetAttentionTarget() : NULL;

			// ignore this user if it has no attention target and the rule says it's needed
			if ( !pExtraPoint && !pAttTarget && pRule->fOrientationToTargetLimit < 360.0f )
				continue;

			int alertness = pPuppet ? (pPuppet->IsEnabled() ? pPuppet->GetProxy()->GetAlertnessState() : 1000) : 0;
			if ( alertness > pRule->iMaxAlertness )
				continue; // ignore this user - too much alerted

			Vec3 userPos = pUser->GetPos();

			// adjust pos if pUserHelper is used
			userPos = pRule->pUserHelper ? pUser->GetHelperPos( pRule->pUserHelper ) : userPos;

			assert( bbMin.x-limitTo <= userPos.x && userPos.x <= bbMax.x+limitTo );
			if ( userPos.y < bbMin.y || userPos.y > bbMax.y ||
				userPos.z < bbMin.z || userPos.z > bbMax.z )
					continue;

			// check the orientation limit to target
			if ( pRule->fOrientationToTargetLimit < 360.0f )
			{
				float cosLimit = 1.0f;
				if ( pRule->fOrientationToTargetLimit != 0 )
					cosLimit = cosf( pRule->fOrientationToTargetLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)

				Vec3 objectDir = pObject->GetOrientation( pRule->pObjectHelper );
				Vec3 targetDir = (pExtraPoint ? *pExtraPoint : pAttTarget->GetPos()) - pos;
				targetDir.NormalizeSafe();
				float dot = objectDir.Dot( targetDir );

				if ( pRule->fOrientationToTargetLimit < 0 )
				{
					dot = -dot;
					cosLimit = -cosLimit;
				}

				if ( dot < cosLimit )
					continue;
			}

			// calculate delay time
			float fDelayTime = CalculateDelayTime( pUser, userPos, pObject, pos, pRule );
			if ( fDelayTime >= 0.0f && fDelayTime <= minDelay )
			{
				minDelay = fDelayTime;
				pMinUser = pUser;
				pMinRule = pRule;
			}

			// add it to the query list
			if ( pQueryEvents && fDelayTime >= 0.0f )
			{
				CQueryEvent q;
				q.pUser = pUser;
				q.pObject = pObject;
				q.pRule = pRule;
				q.pChainedUserEvent = NULL;
				q.pChainedObjectEvent = NULL;
				pQueryEvents->insert( std::make_pair(fDelayTime, q) );
			}
		}
	}

	if ( !pMinRule )
		return 0;

	// is querying only?
	if ( pQueryEvents )
		return -1;

	int id = GetAISystem()->AllocGoalPipeId();
	UseSmartObject( pMinUser, pObject, pMinRule, id, bHighPriority );
	*ppUserEntity = pMinUser->GetEntity();
	return !pMinRule->sAction.empty() && pMinRule->eActionType != eAT_None ? id : -1;
}

struct SOUpdateStats
{
	int classes;
	int users;
	int pairs;

	int hiddenUsers;
	int hiddenObjects;

	int totalRules;
	int ignoredNotNeededRules;
	int ignoredAttTargetRules;
	int ignoredStatesNotMatchingRules;

	int appliedUserOnlyRules;
	int appliedUserObjectRules;

	int ignoredNotInRangeObjects;
	int ignoredAttTargetNotInRangeObjects;
	int ignoredStateDoesntMatchObjects;

	void reset()
	{
		classes = 0;
		users = 0;
		pairs = 0;

		hiddenUsers = 0;
		hiddenObjects = 0;

		totalRules = 0;
		ignoredNotNeededRules = 0;
		ignoredAttTargetRules = 0;
		ignoredStatesNotMatchingRules = 0;

		appliedUserOnlyRules = 0;
		appliedUserObjectRules = 0;
		
		ignoredNotInRangeObjects = 0;
		ignoredAttTargetNotInRangeObjects = 0;
		ignoredStateDoesntMatchObjects = 0;
	}
};

SOUpdateStats currentUpdateStats;

void CSmartObjectManager::Update()
{
FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI)

#ifdef AUTO_SMART_OBJECT
	UpdateAutomatedSmartObject();
#endif

	currentUpdateStats.reset();

	static int totalNumPairsToUpdate = 0;
	static int maxNumPairsUpdatedPerFrame = 100;
	int pairsUpdatedThisFrame = 0;

	if ( CSmartObjectClass::g_itAllUserClasses == CSmartObjectClass::g_AllUserClasses.end() )
	{
		// this is the end of a full cycle and beginning of a new one
		maxNumPairsUpdatedPerFrame = (maxNumPairsUpdatedPerFrame + (totalNumPairsToUpdate / 6)) / 2 + 6;
		totalNumPairsToUpdate = 0;

		CSmartObjectClass::g_itAllUserClasses = CSmartObjectClass::g_AllUserClasses.begin();
		if ( CSmartObjectClass::g_itAllUserClasses != CSmartObjectClass::g_AllUserClasses.end() )
			(*CSmartObjectClass::g_itAllUserClasses)->FirstObject();
	}

	while ( pairsUpdatedThisFrame < maxNumPairsUpdatedPerFrame &&
		CSmartObjectClass::g_itAllUserClasses != CSmartObjectClass::g_AllUserClasses.end() )
	{
		CSmartObjectClass* pClass = *CSmartObjectClass::g_itAllUserClasses;
		++currentUpdateStats.classes;

		while ( pairsUpdatedThisFrame < maxNumPairsUpdatedPerFrame )
		{
			CSmartObject* pSmartObject = pClass->NextVisibleObject();
			if ( pSmartObject == NULL )
			{
				// continue with the next class
				++CSmartObjectClass::g_itAllUserClasses;
				if ( CSmartObjectClass::g_itAllUserClasses != CSmartObjectClass::g_AllUserClasses.end() )
					(*CSmartObjectClass::g_itAllUserClasses)->FirstObject();

				break;
			}
			++currentUpdateStats.users;

			int numPairsProcessed = Process( pSmartObject, pClass );
			pairsUpdatedThisFrame += numPairsProcessed;
			currentUpdateStats.pairs += numPairsProcessed;

			// update LookAt smart object position, but only if this is the last object's user class
			CSmartObjectClasses& classes = pSmartObject->GetClasses();
			int i = classes.size();
			while ( i-- )
			{
				CSmartObjectClass* current = classes[i];
				if ( pClass == current )
				{
					CPuppet* pPuppet = pSmartObject->GetPuppet();
					if ( pPuppet )
					{
						if ( pSmartObject->m_fLookAtLimit )
							pPuppet->m_posLookAtSmartObject = pSmartObject->m_vLookAtPos;
						else
							pPuppet->m_posLookAtSmartObject.zero();
					}
					pSmartObject->m_fLookAtLimit = 0.0f;
					break; // exit the loop! this was the last user class.
				}
				if ( current->IsSmartObjectUser() )
					break; // exit the loop! some other class is the last user class.
			}
		}
	}

	totalNumPairsToUpdate += pairsUpdatedThisFrame;
}

typedef std::pair< CSmartObject*, CCondition* > PairObjectCondition;
typedef std::pair< PairObjectCondition, float > PairDelayTime;
typedef std::vector< PairDelayTime > VecDelayTimes;

struct Pred_IgnoreSecond
{
	bool operator() ( const PairDelayTime& A, const PairDelayTime& B ) const
	{
		return A.first < B.first;
	}
};

int CSmartObjectManager::Process( CSmartObject* pSmartObjectUser, CSmartObjectClass* pClass )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	AIAssert( pSmartObjectUser );

	if ( pSmartObjectUser->IsHidden() )
	{
		// Ignore hidden entities - except the player!
		// Allows the camera to trigger objects in editor AI/Physics mode
		// Very few entities are registered but hidden, so we can afford this test
		IAIObject *pPlayer = GetAISystem()->GetPlayer();
		if ( !pPlayer || pSmartObjectUser->GetEntityId() != pPlayer->GetEntityID()) 
		{
			++currentUpdateStats.hiddenUsers;
			return 1;
		}
	}

	CPuppet* pPuppet = pSmartObjectUser->GetPuppet();
	// don't do this here! it should be done in Update() after processing the last class
	//if ( pPuppet )
	//	pPuppet->m_posLookAtSmartObject.zero();

	if ( pPuppet && (pPuppet->IsEnabled() == false || pPuppet->IsUpdatedOnce() == false) )
	{
		// ignore not enabled puppet-users
		++currentUpdateStats.hiddenUsers;
		return 1;
	}

	int result = 1;

	int species = -1;
	int groupId = -1;
	CAIActor* pUserActor = pSmartObjectUser->GetAIActor();
	if ( pUserActor )
	{
		groupId = pUserActor->GetGroupId();
		species = pUserActor->GetParameters().m_nSpecies;
	}

	// this entity is a smart object user - update his potential smart object targets
	CTimeValue lastUpdateTime(0.0f);
	CSmartObject::MapTimesByClass::iterator itFind = pSmartObjectUser->m_mapLastUpdateTimes.find( pClass );
	if ( itFind != pSmartObjectUser->m_mapLastUpdateTimes.end() )
		lastUpdateTime = itFind->second;

	float fTimeElapsed = (GetAISystem()->GetFrameStartTime() - lastUpdateTime).GetSeconds();
	if ( lastUpdateTime.GetMilliSecondsAsInt64() == 0 )
		fTimeElapsed = 0;
	if ( fTimeElapsed > 0.2f )
		fTimeElapsed = 0.2f;
	pSmartObjectUser->m_mapLastUpdateTimes[ pClass ] = GetAISystem()->GetFrameStartTime();

	IAIObject* pAttTarget = pPuppet ? pPuppet->GetAttentionTarget() : NULL;
	int alertness = pPuppet ? CLAMP( pPuppet->GetProxy()->GetAlertnessState(), 0, 2 ) : 0;

	Vec3 soPos = pSmartObjectUser->GetPos();
	
	// define a map of event updates
	static VecDelayTimes vecDelayTimes;
	vecDelayTimes.clear();

	// check all conditions matching with his class and state
	//MapConditions::iterator itConditions = m_Conditions.find( pClass );
	// optimized: use only the active rules
	CSmartObjectClass::VectorRules& activeRules = pClass->m_vActiveUpdateRules[ alertness ];
	CSmartObjectClass::VectorRules::iterator itConditions, itConditionsEnd = activeRules.end();
	for ( itConditions = activeRules.begin(); itConditions != itConditionsEnd; ++itConditions )
	{
		CCondition* pCondition = *itConditions;
		++currentUpdateStats.totalRules;

		// optimized: active rules have these already culled out
		/*
		if ( !pCondition->bEnabled || pCondition->iMaxAlertness < alertness || pCondition->iRuleType != 0 )
		{
			//++currentUpdateStats.ignoredDisabledIdleNavRules;
			continue;
		}
		// ignore event rules
		if ( !pCondition->sEvent.empty() )
		{
			//++currentUpdateStats.ignoredEventRules;
			continue;
		}
		*/

		// optimized: ignore the rules if there are no instances of the object class
		if ( pCondition->pObjectClass && pCondition->pObjectClass->IsNeeded() == false )
		{
			++currentUpdateStats.ignoredNotNeededRules;
			continue;
		}

		// ignore this rule if the user has no attention target and the rule says it's needed
		if ( !pAttTarget && pCondition->fOrientationToTargetLimit < 360.0f )
		{
			++currentUpdateStats.ignoredAttTargetRules;
			continue;
		}

		// go to next if this one doesn't match user's states
		if ( !pCondition->userStatePattern.Matches( pSmartObjectUser->GetStates() ) )
		{
			++currentUpdateStats.ignoredStatesNotMatchingRules;
			continue;
		}

		// check does it operate only on itself
		if ( !pCondition->pObjectClass )
		{
			// count objects because this method returns the number of relations processed
			++result;
			++currentUpdateStats.appliedUserOnlyRules;

			// calculated delta times should be stored only for now.
			// later existing events will be updated and for new objects new events will be added
			float fDelayTime = CalculateDelayTime( pSmartObjectUser, soPos, pSmartObjectUser, soPos, pCondition );
			if ( fDelayTime > 0.0f )
			{
				PairObjectCondition poc( pSmartObjectUser, pCondition );
				vecDelayTimes.push_back(std::make_pair( poc, fTimeElapsed/fDelayTime ));
			}
			else if ( fDelayTime == 0.0f )
			{
				PairObjectCondition poc( pSmartObjectUser, pCondition );
				vecDelayTimes.push_back(std::make_pair( poc, 1.0f ));
			}

			// this condition is done
			continue;
		}

		// adjust pos if pUserHelper is used
		Vec3 pos = pCondition->pUserHelper ? pSmartObjectUser->GetHelperPos( pCondition->pUserHelper ) : soPos;

		Vec3 bbMin = pos;
		Vec3 bbMax = pos;

		float limitTo = pCondition->fDistanceTo;
		// adjust limit if pUserHelper is used
		if ( pCondition->pUserHelper)
			limitTo += pCondition->pUserHelper->qt.t.GetLength();
		// adjust limit if pObjectHelper is used
		if ( pCondition->pObjectHelper )
			limitTo += pCondition->pObjectHelper->qt.t.GetLength();
		Vec3 d( limitTo, limitTo, limitTo );
		bbMin -= d;
		bbMax += d;

		// calculate the limit in advance
		float orientationToTargetLimit = -2.0f; // unlimited
		if ( pCondition->fOrientationToTargetLimit < 360.0f )
		{
			if ( pCondition->fOrientationToTargetLimit == 0 )
				orientationToTargetLimit = 1.0f;
			else
				orientationToTargetLimit = cosf( pCondition->fOrientationToTargetLimit / 360.0f * 3.1415926536f ); // limit is expressed as FOV (360 means unlimited)

			if ( pCondition->fOrientationToTargetLimit < 0 )
				orientationToTargetLimit = -orientationToTargetLimit;
		}

		// check all objects (but not the user) matching with condition's class and state
		CSmartObjectClass::MapSmartObjectsByPos& mapObjectsByPos = pCondition->pObjectClass->m_MapObjectsByPos;
		if ( mapObjectsByPos.empty() )
			continue;

		CSmartObjectClass::MapSmartObjectsByPos::iterator itByPos = mapObjectsByPos.lower_bound( bbMin.x );
		for ( ; itByPos != mapObjectsByPos.end() && itByPos->first <= bbMax.x; ++itByPos )
		{
			CSmartObject* pSmartObject = itByPos->second;
			++currentUpdateStats.appliedUserObjectRules;

			// the user can not be the target object!!!
			if ( pSmartObjectUser == pSmartObject )
				continue;

			// ignore hidden entities
			if ( pSmartObject->IsHidden() )
			{
				++currentUpdateStats.hiddenObjects;
				continue;
			}

			// Check range first since it could be faster
			Vec3 objectPos = pCondition->pObjectHelper ? pSmartObject->GetHelperPos(pCondition->pObjectHelper) : pSmartObject->GetPos();
			if ( objectPos.IsZero() )
				continue;
			assert( objectPos.x == 0 || bbMin.x-limitTo <= objectPos.x && objectPos.x <= bbMax.x+limitTo );
			if ( objectPos.y < bbMin.y || objectPos.y > bbMax.y ||
				objectPos.z < bbMin.z || objectPos.z > bbMax.z )
			{
				++currentUpdateStats.ignoredNotInRangeObjects;
				continue;
			}

			// Also check the orientation limit to user's attention target
			if ( pCondition->fOrientationToTargetLimit < 360.0f )
			{
				Vec3 objectDir = pSmartObject->GetOrientation( pCondition->pObjectHelper );
				Vec3 targetDir = pAttTarget->GetPos() - objectPos;
				targetDir.NormalizeSafe();
				float dot = objectDir.Dot( targetDir );

				if ( pCondition->fOrientationToTargetLimit < 0 )
					dot = -dot;

				if ( dot < orientationToTargetLimit )
				{
					++currentUpdateStats.ignoredAttTargetNotInRangeObjects;
					continue;
				}
			}

			// add virtual states
			IAIObject* pAIObjectObject = pSmartObject->GetAI();
			CAIActor* pAIObjectActor = pSmartObject->GetAIActor();
			bool attTarget = false;
			bool sameGroupId = false;
			bool sameSpecies = false;

			int spObject = -1;

			if ( pAIObjectActor )
			{
				// check is the object attention target of the user
				attTarget = pPuppet && pPuppet->GetAttentionTarget() == pAIObjectActor ;
				if ( attTarget )
					pSmartObject->m_States.insert( m_StateAttTarget );
				// Only actors has species.
				spObject = pAIObjectActor->GetParameters().m_nSpecies;
			}

			if ( pAIObjectObject )
			{
				// check are the user and the object in the same group and species
				if ( groupId >= 0 || species >= 0 )
				{
					// check same species
					sameSpecies = species == spObject;
					if ( sameSpecies )
					{
						pSmartObject->m_States.insert( m_StateSameSpecies );

						// if they are same species check are they in same group
						sameGroupId = groupId == pAIObjectObject->GetGroupId();
						if ( sameGroupId )
							pSmartObject->m_States.insert( m_StateSameGroup );
					}
					else if ( species < 0 || spObject < 0 )
					{
						// if any of them has no species check are they in same group
						sameGroupId = groupId == pAIObjectObject->GetGroupId();
						if ( sameGroupId )
							pSmartObject->m_States.insert( m_StateSameGroup );
					}
				}
			}

			// check object's state pattern and then remove virtual states
			bool bMatches = pCondition->objectStatePattern.Matches( pSmartObject->GetStates() );
			if ( attTarget )
				pSmartObject->m_States.erase( m_StateAttTarget );
			if ( sameSpecies )
				pSmartObject->m_States.erase( m_StateSameSpecies );
			if ( sameGroupId )
				pSmartObject->m_States.erase( m_StateSameGroup );

			// ignore this object if it doesn't match precondition state
			if ( !bMatches )
			{
				++currentUpdateStats.ignoredStateDoesntMatchObjects;
				continue;
			}

			// count objects because this method returns the number of relations processed
			++result;

			if ( pCondition->pObjectHelper )
				objectPos = pSmartObject->GetHelperPos( pCondition->pObjectHelper );

			// calculated delta times should be stored only for now.
			// later existing events will be updated and for new objects new events will be added
			float fDelayTime = CalculateDelayTime( pSmartObjectUser, pos, pSmartObject, objectPos, pCondition );
			if ( fDelayTime > 0.0f )
			{
				PairObjectCondition poc( pSmartObject, pCondition );
				vecDelayTimes.push_back(std::make_pair( poc, fTimeElapsed/fDelayTime ));
			}
			else if ( fDelayTime == 0.0f )
			{
				PairObjectCondition poc( pSmartObject, pCondition );
				vecDelayTimes.push_back(std::make_pair( poc, 1.0f ));
			}
		}
	}

	std::sort( vecDelayTimes.begin(), vecDelayTimes.end() );

	// now update his existing smart object events
	CSmartObject::VectorEvents & events = pSmartObjectUser->m_Events[ pClass ];
	int i = events.size(), use = -1, lookat = -1;
	while ( i-- )
	{
		float thisLookat = events[i].m_pCondition->fLookAtPerc;
		if ( thisLookat <= 0.0f || thisLookat >= 1.0f )
			thisLookat = FLT_MAX;

		PairObjectCondition poc( events[i].m_pObject, events[i].m_pCondition );
		VecDelayTimes::iterator itTimes = std::lower_bound( vecDelayTimes.begin(), vecDelayTimes.end(), std::make_pair(poc,0.0f), Pred_IgnoreSecond() );
		if ( itTimes != vecDelayTimes.end() && itTimes->first == poc )
		{
			events[i].m_Delay += itTimes->second;

			// remove from list. later all remained entries will be added as new events
			itTimes->second = -1.0f;
			
			// is this smart object ready to be used?
			if ( events[i].m_Delay >= 1 )
			{
				// and is it more important than current candidate?
				if ( use < 0 || events[i].m_Delay > events[use].m_Delay )
					use = i;
			}

			if ( pPuppet )
			{
				// check is this better lookat target
				if ( events[i].m_Delay > thisLookat )
				{
					thisLookat = (events[i].m_Delay - thisLookat) / (1.0f - thisLookat);
					if ( thisLookat > pSmartObjectUser->m_fLookAtLimit )
					{
						pSmartObjectUser->m_fLookAtLimit = thisLookat;
						lookat = i;
					}
				}
			}
		}
		else
		{
			// events for objects not satisfying the condition will be forgotten and then removed from this vector
			if ( events[i].m_pCondition->fMemory > 0.001f )
				events[i].m_Delay -= fTimeElapsed/events[i].m_pCondition->fMemory;
			else
				events[i].m_Delay = -1.0f;
		}

		// delete unimportant events
		if ( events[i].m_Delay < 0 )
		{
			events[i] = events.back();
			events.pop_back();

			// update 'use' if it was pointing to the last element
			if ( use == events.size() )
				use = i;

			// update 'lookat' if it was pointing to the last element
			if ( lookat == events.size() )
				lookat = i;
		}
	}

	// and finally all new smart object events will be added
	VecDelayTimes::iterator itTimes = vecDelayTimes.begin();
	while ( itTimes != vecDelayTimes.end() )
	{
		if ( itTimes->second >= 0 )
		{
			CSmartObjectEvent event =
			{
				itTimes->first.first,
				itTimes->first.second,
				0
			};
			events.push_back( event );
		}
		++itTimes;
	}

	if ( lookat >= 0 )
	{
		pSmartObjectUser->m_vLookAtPos = events[lookat].m_pObject->GetPos();
	}

	if ( use >= 0 )
	{
		UseSmartObject( pSmartObjectUser, events[use].m_pObject, events[use].m_pCondition );
		events[use] = events.back();
		events.pop_back();
	}

	// return number of relations processed
	return result;
}

void CSmartObjectManager::UseSmartObject( CSmartObject* pSmartObjectUser, CSmartObject* pSmartObject, CCondition* pCondition, int eventId /*=0*/, bool bForceHighPriority /*=false*/ )
{
	AIAssert( pSmartObject );

	CPuppet* pPuppet = pSmartObjectUser->GetPuppet();
	if ( pPuppet && pSmartObjectUser != pSmartObject )
	{
		// keep track of last used smart object
		pPuppet->m_idLastUsedSmartObject = pSmartObject->GetEntityId();
	}

	// update random factors
	pSmartObjectUser->m_fRandom = float(ai_rand())/(2.0f*RAND_MAX);
	pSmartObject->m_fRandom = float(ai_rand())/(2.0f*RAND_MAX);

	// update states
	if ( !pCondition->userPreActionStates.empty() ) // check is next state non-empty
		ModifySmartObjectStates( pSmartObjectUser, pCondition->userPreActionStates );
	if ( pSmartObjectUser != pSmartObject )
		if ( !pCondition->objectPreActionStates.empty() ) // check is next state non-empty
			ModifySmartObjectStates( pSmartObject, pCondition->objectPreActionStates );

	if ( gAIEnv.CVars.DrawSmartObjects )
	{
		if ( pCondition->eActionType != eAT_None && !pCondition->sAction.empty() )
		{
			switch ( pCondition->eActionType )
			{
			case eAT_Action:
				AILogComment("User %s should execute action %s on smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			case eAT_PriorityAction:
				AILogComment("User %s should execute high priority action %s on smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			case eAT_AISignal:
				AILogComment("User %s receives action signal %s to use smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			case eAT_AnimationSignal:
				AILogComment("User %s is going to play one shot animation %s on smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			case eAT_AnimationAction:
				AILogComment("User %s is going to play looping animation %s on smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			case eAT_PriorityAnimationSignal:
				AILogComment("User %s is going to play high priority one shot animation %s on smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			case eAT_PriorityAnimationAction:
				AILogComment("User %s is going to play high priority looping animation %s on smart object %s!", pSmartObjectUser->GetName(), pCondition->sAction.c_str(), pSmartObject->GetName() );
				break;
			}
		}
		else
			AILogComment("User %s should execute NULL action on smart object %s!", pSmartObjectUser->GetName(), pSmartObject->GetName() );
	}

	if ( gAIEnv.CVars.DrawSmartObjects )
	{
		// show in debug draw
		CDebugUse debugUse = { GetAISystem()->GetFrameStartTime(), pSmartObjectUser, pSmartObject };
		m_vDebugUse.push_back( debugUse );
	}

	if ( pCondition->eActionType != eAT_None && !pCondition->sAction.empty() )
		pSmartObjectUser->Use( pSmartObject, pCondition, eventId, bForceHighPriority );
}

bool CSmartObjectManager::PrepareUseNavigationSmartObject( SAIActorTargetRequest& req, SNavSOStates& states, CSmartObject* pSmartObjectUser, CSmartObject* pSmartObject,
																										 const CCondition* pCondition, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper/*, const Vec3 *pDestination*/)
{
	if(pCondition->eActionType != eAT_AnimationSignal && pCondition->eActionType != eAT_AnimationAction
		&& pCondition->eActionType != eAT_PriorityAnimationSignal && pCondition->eActionType != eAT_PriorityAnimationAction)
{
		return false;
	}

	// update random factors
	pSmartObjectUser->m_fRandom = float(ai_rand())/(2.0f*RAND_MAX);
	pSmartObject->m_fRandom = float(ai_rand())/(2.0f*RAND_MAX);

	// update states
	if ( !pCondition->userPreActionStates.empty() ) // check is next state non-empty
		ModifySmartObjectStates( pSmartObjectUser, pCondition->userPreActionStates );
	if ( !pCondition->objectPreActionStates.empty() ) // check is next state non-empty
		ModifySmartObjectStates( pSmartObject, pCondition->objectPreActionStates );

	const CClassTemplateData::CTemplateHelper* exitHelper = 0;
	if (pCondition->pObjectClass->m_pTemplateData && pToHelper->templateHelperIndex >= 0 &&
			pToHelper->templateHelperIndex < (int)pCondition->pObjectClass->m_pTemplateData->helpers.size())
	{
		exitHelper = &pCondition->pObjectClass->m_pTemplateData->helpers[pToHelper->templateHelperIndex];
	}

	req.animation = pCondition->sAction.c_str();
	req.signalAnimation = pCondition->eActionType == eAT_AnimationSignal || pCondition->eActionType == eAT_PriorityAnimationSignal;
	req.projectEndPoint = exitHelper ? exitHelper->project : true;
	req.startWidth = pCondition->fStartWidth;
	req.directionTolerance = DEG2RAD(pCondition->fDirectionTolerance);
	req.startArcAngle = pCondition->fStartArcAngle;

	// store post-action state changes to be used after the animation
	CPuppet* pPuppet = pSmartObjectUser->GetPuppet();
	assert( states.sAnimationDoneUserStates.empty() );
	assert( states.sAnimationDoneObjectStates.empty() );
	assert( states.sAnimationFailUserStates.empty() );
	assert( states.sAnimationFailObjectStates.empty() );
	
	states.objectEntId = pSmartObject->GetEntityId();
	states.sAnimationDoneUserStates = pCondition->userPostActionStates.AsString();
	states.sAnimationDoneObjectStates = pCondition->objectPostActionStates.AsString();
	states.sAnimationFailUserStates = pCondition->userPreActionStates.AsUndoString();
	states.sAnimationFailObjectStates = pCondition->objectPreActionStates.AsUndoString();

	if ( gAIEnv.CVars.DrawSmartObjects )
	{
		AILogComment("User %s is going to play %s animation %s on navigation smart object %s!", pSmartObjectUser->GetName(),
			pCondition->eActionType == eAT_AnimationSignal || pCondition->eActionType == eAT_PriorityAnimationSignal ? "one shot" : "looping", pCondition->sAction.c_str(), pSmartObject->GetName() );
	}










	return true;
}


CSmartObject* CSmartObjectManager::IsSmartObject( IEntity* pEntity )
{
	AIAssert( pEntity );
	return pEntity ? pEntity->GetSmartObject() : 0;
}

bool CSmartObjectManager::CheckSmartObjectStates( IEntity* pEntity, const CSmartObject::CStatePattern& pattern ) const
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( !pSmartObject )
		return false;
	
	return pattern.Matches( pSmartObject->GetStates() );
}

bool CSmartObjectManager::CheckSmartObjectStates( IEntity* pEntity, const char* patternStates ) const
{
	if(!IsInitialized())
		return false;
	AIAssert( pEntity );
	AIAssert( patternStates );
	CSmartObject::CStatePattern pattern;
	String2StatePattern( patternStates, pattern );
	return CheckSmartObjectStates( pEntity, pattern );
}
void CSmartObjectManager::ModifySmartObjectStates( IEntity* pEntity, const CSmartObject::DoubleVectorStates& states )
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( !pSmartObject )
	{
		// Sometimes this function is called during initialization which is right before OnSpawn
		// so this entity is not registered as a smart object yet.
		SEntitySpawnParams params;
		OnSpawn( pEntity, params );
		pSmartObject = IsSmartObject( pEntity );

		// mark this entity as just registered so we don't register it twice
		m_pPreOnSpawnEntity = pEntity;
	}
	if ( pSmartObject )
		ModifySmartObjectStates( pSmartObject, states );
}

void CSmartObjectManager::ModifySmartObjectStates( CSmartObject* pSmartObject, const CSmartObject::DoubleVectorStates& states ) const
{
	CSmartObject::VectorStates::const_iterator itStates, itStatesEnd = states.negative.end();
	for ( itStates = states.negative.begin(); itStates != itStatesEnd; ++itStates )
		RemoveSmartObjectState( pSmartObject, *itStates );

	itStatesEnd = states.positive.end();
	for ( itStates = states.positive.begin(); itStates != itStatesEnd; ++itStates )
		AddSmartObjectState( pSmartObject, *itStates );
}

void CSmartObjectManager::ModifySmartObjectStates( IEntity* pEntity, const char* listStates )
{
	if(!GetAISystem()->IsEnabled())
		return;
	if(!IsInitialized())
		return;
	AIAssert( pEntity );
	AIAssert( listStates );
	CSmartObject::DoubleVectorStates states;
	CSmartObjectManager::String2States(listStates, states);
	ModifySmartObjectStates( pEntity, states );
}
void CSmartObjectManager::SetSmartObjectState( IEntity* pEntity, CSmartObject::CState state ) const
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( pSmartObject )
		SetSmartObjectState( pSmartObject, state );
}

void CSmartObjectManager::SetSmartObjectState( CSmartObject* pSmartObject, CSmartObject::CState state ) const
{
	pSmartObject->m_States.clear();
	pSmartObject->m_States.insert( state );
}

void CSmartObjectManager::SetSmartObjectState( IEntity* pEntity, const char* sStateName )
{
	if(!GetAISystem()->IsEnabled())
		return;
	if (IsInitialized() && sStateName && *sStateName )
	{
		CSmartObject::CState state( sStateName );
		SetSmartObjectState( pEntity, state );
	}
}
void CSmartObjectManager::AddSmartObjectState( IEntity* pEntity, CSmartObject::CState state ) const
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( pSmartObject )
		AddSmartObjectState( pSmartObject, state );
}

void CSmartObjectManager::AddSmartObjectState( CSmartObject* pSmartObject, CSmartObject::CState state ) const
{
	if ( !pSmartObject->m_States.insert( state ).second )
		return;

	if ( state == m_StateBusy )
	{
		// check is the entity linked with an entity link named "Busy" and then set the "Busy" state to the linked entity as well
		IEntity* pEntity = pSmartObject->GetEntity();
		IEntityLink* pEntityLink = pEntity ? pEntity->GetEntityLinks() : NULL;
		while ( pEntityLink )
		{
			if ( !strcmp("Busy", pEntityLink->name) )
			{
				if ( IEntity* pEntity = gEnv->pEntitySystem->GetEntity( pEntityLink->entityId ) )
					AddSmartObjectState( pEntity, state );
				break;
			}
			pEntityLink = pEntityLink->next;
		}
	}
}

void CSmartObjectManager::AddSmartObjectState( IEntity* pEntity, const char* sState )
{
	if (IsInitialized() && sState && *sState )
	{
		CSmartObject::CState state( sState );
		AddSmartObjectState( pEntity, state );
	}
}
void CSmartObjectManager::RemoveSmartObjectState( IEntity* pEntity, CSmartObject::CState state ) const
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( pSmartObject )
		RemoveSmartObjectState( pSmartObject, state );
}

void CSmartObjectManager::RemoveSmartObjectState( CSmartObject* pSmartObject, CSmartObject::CState state ) const
{
	if ( !pSmartObject->m_States.erase( state ) )
		return;
	if ( state == m_StateBusy )
	{
		// check is the entity linked with an entity link named "Busy" and then set the "Busy" state to the linked entity as well
		IEntity* pEntity = pSmartObject->GetEntity();
		IEntityLink* pEntityLink = pEntity ? pEntity->GetEntityLinks() : NULL;
		while ( pEntityLink )
		{
			if ( !strcmp("Busy", pEntityLink->name) )
			{
				if ( IEntity* pEntity = gEnv->pEntitySystem->GetEntity( pEntityLink->entityId ) )
					RemoveSmartObjectState( pEntity, state );
				break;
			}
			pEntityLink = pEntityLink->next;
		}
	}
}

void CSmartObjectManager::RemoveSmartObjectState( IEntity* pEntity, const char* sState )
{
	if ( IsInitialized() && sState && *sState )
	{
		CSmartObject::CState state( sState );
		RemoveSmartObjectState( pEntity, state );
	}
}
void CSmartObjectManager::DebugDraw()
{
	CDebugDrawContext dc;
	dc->SetAlphaBlended(true);

	float drawDist2 = gAIEnv.CVars.AgentStatsDist;
	if ( drawDist2 <= 0 || !GetAISystem()->GetPlayer() )
		return;
	drawDist2 *= drawDist2;
	Vec3 playerPos = dc->GetCameraPos();

	CSmartObjectClass::MapClassesByName::iterator itClass, itClassesEnd = CSmartObjectClass::g_AllByName.end();
	for ( itClass = CSmartObjectClass::g_AllByName.begin(); itClass != itClassesEnd; ++itClass )
	{
		CSmartObjectClass::MapSmartObjectsByPos& mapByPos = itClass->second->m_MapObjectsByPos;
		CSmartObjectClass::MapSmartObjectsByPos::iterator itByPos;
		for ( itByPos = mapByPos.begin(); itByPos != mapByPos.end(); ++itByPos )
		{
			CSmartObject* pCurrentObject = itByPos->second;
			Vec3 from = pCurrentObject->GetPos();

			// don't draw too far objects
			if ( drawDist2 < (from-playerPos).GetLengthSquared() )
				continue;

			DrawTemplate( pCurrentObject, false );

			// [2/26/2009 evgeny] We don't draw or modify anything external here -- Do we need this block at all?
			CSmartObject::MapEventsByClass::iterator itEvents, itEventsEnd = pCurrentObject->m_Events.end();
			for ( itEvents = pCurrentObject->m_Events.begin(); itEvents != itEventsEnd; ++itEvents )
			{
				CSmartObject::VectorEvents & events = itEvents->second;
				float alpha = 2.0f;
				int i = events.size();
				while ( i-- )
				{
					CSmartObjectEvent& event = events[i];
					float delay = event.m_Delay;
					if (delay)
					{
						Vec3 to = event.m_pObject->GetPos();

						SmartObjectHelper* pFromHelper = event.m_pCondition->pUserHelper;
						SmartObjectHelper* pToHelper = event.m_pCondition->pObjectHelper;
						Vec3 fromPos = pFromHelper ? pCurrentObject->GetEntity()->GetWorldTM().TransformPoint( pFromHelper->qt.t ) : from;
						Vec3 toPos = pToHelper ? event.m_pObject->GetEntity()->GetWorldTM().TransformPoint( pToHelper->qt.t ) : to;

						Vec3 dir = toPos - fromPos;
						dir *= 0.5f*delay;
						Vec3 pt = fromPos + dir;
						ColorF yellow(0xFF00FFFF), green(0xFF00FF00);
						yellow.a = min( 1.0f, alpha );
						green.a = max( 0.01f, min(1.0f, alpha) );
						alpha *= 0.667f;
						//dc->DrawLineColor( fromPos, yellow, pt, green );
						pt = toPos - dir;
						//dc->DrawLineColor( toPos, yellow, pt, green );
					}
				}
			}

			string sClasses;

			ColorF pink(0xFF8080FFu), white(0xFFFFFFFFu), green(0x6080FF80u);

			//Matrix34 matrix = pCurrentObject->GetEntity()->GetWorldTM();
			CSmartObjectClasses& classes = pCurrentObject->GetClasses();
			CSmartObjectClasses::iterator itClasses, itClassesEnd2 =  classes.end();
			for ( itClasses = classes.begin(); itClasses != itClassesEnd2; ++itClasses )
			{
				CSmartObjectClass* pClass = *itClasses;

				if ( !sClasses.empty() )
					sClasses += ", ";
				sClasses += pClass->GetName();

				// draw arrows to show smart object helpers
				CSmartObjectClass::VectorHelpers& vHelpers = pClass->m_vHelpers;
				CSmartObjectClass::VectorHelpers::iterator itHelpers, itHelpersEnd = vHelpers.end();
				for ( itHelpers = vHelpers.begin(); itHelpers != itHelpersEnd; ++itHelpers )
				{
					SmartObjectHelper* pHelper = *itHelpers;

					Vec3 fromPos = pCurrentObject->GetHelperPos( pHelper );
					Vec3 toPos = fromPos + pCurrentObject->GetOrientation( pHelper ) * 0.2f;

					dc->DrawSphere( fromPos, 0.05f, green );
					//dc->DrawLineColor( fromPos, green, toPos, green );
					dc->DrawCone( toPos, toPos - fromPos, 0.05f, 0.1f, green );
				}


				// draw lines to show smart object helper links
				CSmartObjectClass::THelperLinks::iterator itLinks, itLinksEnd = pClass->m_vHelperLinks.end();
				for ( itLinks = pClass->m_vHelperLinks.begin(); itLinks != itLinksEnd; ++itLinks )
				{
					CSmartObjectClass::HelperLink& helperLink = *itLinks;
					SmartObjectHelper* pFromHelper = helperLink.from;
					SmartObjectHelper* pToHelper = helperLink.to;

					Vec3 fromPos = pCurrentObject->GetHelperPos( pFromHelper ); // matrix.TransformPoint( pFromHelper->qt.t );
					Vec3 toPos = pCurrentObject->GetHelperPos( pToHelper ); // matrix.TransformPoint( pToHelper->qt.t );

					//dc->DrawLineColor( fromPos, pink, toPos, pink );
					dc->DrawCone( toPos, toPos-fromPos, 0.05f, 0.2f, pink );
				}

				// [2/26/2009 evgeny] Same as above:
				// We don't draw or modify anything external here -- Do we need this block at all?
				CGraph* pGraph = gAIEnv.pGraph;
				if ( pGraph )
				{
					// first check if the graph is still empty
					CAllNodesContainer::Iterator it( pGraph->GetAllNodes(), IAISystem::NAV_TRIANGULAR );
					if ( it.GetNode() )
					{
						// draw lines to show how smart object helpers are linked to outside world
						CSmartObjectClass::SetHelpers::iterator itSetHelpers, itSetHelpersEnd = pClass->m_setNavHelpers.end();
						for ( itSetHelpers = pClass->m_setNavHelpers.begin(); itSetHelpers != itSetHelpersEnd; ++itSetHelpers )
						{
							const SmartObjectHelper* pHelper = *itSetHelpers;
							Vec3 helperPos = pCurrentObject->GetHelperPos( pHelper );
							//unsigned nodeIndices[2];
							//if ( pCurrentObject->GetNavNodes( nodeIndices, pHelper ) )
							//	dc->DrawLineColor( helperPos, pink, gAIEnv.pGraph->GetNodeManager().GetNode(nodeIndices[0])->GetPos(), white );
						}
					}
				}
			}

			ColorB yellow(255, 255, 0);
			ColorB green2(  0, 255, 0);
			const float fontSize = 1.15f;

			string s;
			CPuppet* pPuppet = pCurrentObject->GetPuppet();
			if ( pPuppet )
				s += "\n\n\n\n";

			// to prevent buffer overrun
			if ( sClasses.length() > 120 )
			{
				sClasses = sClasses.substr( 0, 120 );
				sClasses += "...";
			}

			dc->Draw3dLabelEx( from, fontSize, green2, true, true, s + sClasses );

			s += "\n";
			const CSmartObject::SetStates& states = pCurrentObject->GetStates();
			CSmartObject::SetStates::const_iterator itStates, itStatesEnd = states.end();
			for ( itStates = states.begin(); itStates != itStatesEnd; ++itStates )
			{
				s += itStates->c_str();
				s += ' ';
			}

			// to prevent buffer overrun
			if ( s.length() > 124 )
			{
				s = s.substr( 0, 124 );
				s += "...";
			}

			dc->Draw3dLabelEx( from, fontSize, yellow, true, true, s );
		}
	}
	
	// debug use actions
	CTimeValue fTime = GetAISystem()->GetFrameStartTime();
	int i = m_vDebugUse.size();
	while ( i-- )
	{
		float age = (fTime - m_vDebugUse[i].m_Time).GetSeconds();
		if ( age < 0.5f )
		{
			Vec3 from = m_vDebugUse[i].m_pUser->GetPos();
			Vec3 to = m_vDebugUse[i].m_pObject->GetPos();
			age = abs( 1.0f - 4.0f*age );
			age = abs( 1.0f - 2.0f*age );
			//ColorF color1(age, age, 1.0f, abs(1.0f-2.0f*age)), color2(1.0f-age, 1.0f-age, 1.0f, abs(1.0f-2.0f*age));
			//dc->DrawLineColor( from, color1, to, color2 );
		}
		else
		{
			m_vDebugUse[i] = m_vDebugUse.back();
			m_vDebugUse.pop_back();
		}
	}
}

void CSmartObjectManager::RescanSOClasses( IEntity* pEntity )
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	CSmartObjectClasses vClasses;
	if ( !ParseClassesFromProperties(pEntity, vClasses) )
	{
		if ( pSmartObject )
			DoRemove( pEntity );
	}
	if ( pSmartObject )
	{
		if ( vClasses.size() == pSmartObject->GetClasses().size() )
		{
			CSmartObjectClasses::iterator it1, it1End = vClasses.end();
			CSmartObjectClasses::iterator it2, it2Begin = pSmartObject->GetClasses().begin(), it2End = pSmartObject->GetClasses().end();
			for ( it1 = vClasses.begin(); it1 != it1End; ++it1 )
			{
				it2 = std::find( it2Begin, it2End, *it1 );
				if ( it2 == it2End )
					break;
			}
			if ( it1 == it1End )
				return;
		}
		DoRemove( pEntity );
		pSmartObject = NULL;
	}
	SEntitySpawnParams params;
	OnSpawn( pEntity, params );
}

// Implementation of IEntitySystemSink methods
///////////////////////////////////////////////
void CSmartObjectManager::OnSpawn( IEntity* pEntity, SEntitySpawnParams& params )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
/* Márcio: Enabling SmartObjects in multiplayer.
	if (gEnv->bMultiplayer)
		return;
*/
	if ( pEntity == m_pPreOnSpawnEntity )
	{
		// the entity for which OnSpawn was called just right before the current OnSpawn call
		m_pPreOnSpawnEntity = NULL;
		return;
	}
	m_pPreOnSpawnEntity = NULL;

	CSmartObjectClasses vClasses;
	if ( !ParseClassesFromProperties(pEntity, vClasses) )
		return;
	
	AIAssert(pEntity);
	CSmartObject* smartObject = IsSmartObject( pEntity );
	if ( !smartObject )
		smartObject = new CSmartObject( pEntity ? pEntity->GetId() : 0 );

	Vec3 size = smartObject->MeasureUserSize();
	size.y += size.z;
	bool hasSize = !size.IsZero();

	for ( uint32 i = 0; i < vClasses.size(); ++i )
	{
		CSmartObjectClass* pClass = vClasses[i];

		pClass->MarkAsNeeded();

		CSmartObjectClasses::iterator it = std::find( smartObject->GetClasses().begin(), smartObject->GetClasses().end(), pClass );
		if ( it == smartObject->GetClasses().end() )
		{
			pClass->RegisterSmartObject( smartObject );
			smartObject->m_fKey = pEntity->GetWorldPos().x;
			pClass->m_MapObjectsByPos.insert( std::make_pair( smartObject->m_fKey, smartObject ) );
		}

		// register each class in navigation
		RegisterInNavigation( smartObject, pClass );

		if ( hasSize )
		{
			CSmartObjectClass::UserSize userSize( size.x, size.y, size.z );
			pClass->m_StanceMaxSize += userSize;
		}
	}
}


void CSmartObjectManager::RemoveEntity( IEntity* pEntity )
{
	DoRemove(pEntity);
}

// Implementation of IEntitySystemSink methods
///////////////////////////////////////////////
bool CSmartObjectManager::OnRemove( IEntity* pEntity )
{
	return true;
}

void CSmartObjectManager::DoRemove( IEntity* pEntity )
{
	// remove all actions in which this entity is participating
	gAIEnv.pAIActionManager->OnEntityRemove( pEntity );

	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( pSmartObject )
	{
		// let's make sure AI navigation is not using it
		// in case this is a navigation smart object entity!
		UnregisterFromNavigation( pSmartObject );

		CDebugUse debugUse = { 0.0f, pSmartObject, pSmartObject };
		VectorDebugUse::iterator it = m_vDebugUse.begin();
		while ( it != m_vDebugUse.end() )
		{
			if ( it->m_pUser == pSmartObject || it->m_pObject == pSmartObject )
			{
				*it = m_vDebugUse.back();
				m_vDebugUse.pop_back();
			}
			else
				++it;
		}

		// must remove all references to removed smart object within events lists kept by smart objects registered as users
		CSmartObjectClass::VectorClasses::iterator itAllClasses, itAllClassesEnd = CSmartObjectClass::g_AllUserClasses.end();
		for ( itAllClasses = CSmartObjectClass::g_AllUserClasses.begin(); itAllClasses != itAllClassesEnd; ++itAllClasses )
		{
			CSmartObjectClass* pUserClass = *itAllClasses;

			CSmartObjectClass::MapSmartObjectsByPos& objects = pUserClass->m_MapObjectsByPos;
			CSmartObjectClass::MapSmartObjectsByPos::iterator itObjects, itObjectsEnd = objects.end();
			for ( itObjects = objects.begin(); itObjects != itObjectsEnd; ++itObjects )
			{
				CSmartObject* pCurrentObject = itObjects->second;
				CSmartObject::VectorEvents & events = pCurrentObject->m_Events[ pUserClass ];
				int i = events.size();
				while ( i-- )
				{
					if ( events[i].m_pObject == pSmartObject )
					{
						events[i] = events.back();
						events.pop_back();
					}
				}
			}
		}

		float x = pSmartObject->m_fKey;

		// don't keep a reference to vector classes, coz it will be modified on
		// each call to DeleteSmartObject and at the end it will be even deleted
		CSmartObjectClasses vClasses = pSmartObject->GetClasses();

		CSmartObjectClasses::iterator itClasses, itClassesEnd = vClasses.end();
		for ( itClasses = vClasses.begin(); itClasses != itClassesEnd; ++itClasses )
		{
			CSmartObjectClass::MapSmartObjectsByPos& objects = (*itClasses)->m_MapObjectsByPos;

			CSmartObjectClass::MapSmartObjectsByPos::iterator itByPos, itByPosEnd = objects.end();
			for( itByPos = objects.find(x); itByPos != itByPosEnd && itByPos->first == x; ++itByPos )
			{
				if ( itByPos->second == pSmartObject )
				{
					objects.erase( itByPos );
					break;
				}
			}

			(*itClasses)->DeleteSmartObject( *pSmartObject );
		}

		//delete pSmartObject;
	}
}

// Implementation of IEntitySystemSink methods
///////////////////////////////////////////////
void CSmartObjectManager::OnEvent( IEntity* pEntity, SEntityEvent& event )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	switch (event.event)
	{
	case ENTITY_EVENT_DONE:
		DoRemove( pEntity );
		break;
	case ENTITY_EVENT_HIDE:
		{
			CSmartObject* pSmartObject = IsSmartObject( pEntity );
			if ( pSmartObject )
				pSmartObject->Hide( true );
			break;
		}
	case ENTITY_EVENT_UNHIDE:
		{
			CSmartObject* pSmartObject = IsSmartObject( pEntity );
			if ( pSmartObject )
				pSmartObject->Hide( false );
			break;
		}
	case ENTITY_EVENT_INIT:
		{
			SEntitySpawnParams params;
			OnSpawn( pEntity, params );
			break;
		}
	case ENTITY_EVENT_XFORM:
		{
			CSmartObject* pSmartObject = IsSmartObject( pEntity );
			if ( pSmartObject )
			{
				pSmartObject->m_mapNavNodes.clear();
				pSmartObject->m_eValidationResult = eSOV_Unknown;

				float oldX = pSmartObject->m_fKey;
				float newX = pSmartObject->GetPos().x;
				if ( newX == oldX )
					return;

				pSmartObject->m_fKey = newX;

				CSmartObjectClasses& classes = pSmartObject->GetClasses();
				CSmartObjectClasses::iterator it, itEnd = classes.end();
				for ( it = classes.begin(); it != itEnd; ++it )
				{
					CSmartObjectClass::MapSmartObjectsByPos& mapByPos = (*it)->m_MapObjectsByPos;
					CSmartObjectClass::MapSmartObjectsByPos::iterator itByPos = mapByPos.find( oldX );

					//assert( itByPos != mapByPos.end() );
					// Warning!!! This is only workaround for the problem!
					if ( itByPos == mapByPos.end() )
						itByPos = mapByPos.begin();
					if ( itByPos == mapByPos.end() )
						return;
					
					while ( itByPos->second != pSmartObject )
					{
						++itByPos;
						//assert( itByPos != mapByPos.end() );
						if ( itByPos == mapByPos.end() )
						{
							// Warning!!! This is only workaround for the problem!
							itByPos = mapByPos.begin();
							//assert( itByPos != mapByPos.end() );

							while ( itByPos->second != pSmartObject )
							{
								++itByPos;
								//assert( itByPos != mapByPos.end() );
								if ( itByPos != mapByPos.end() )
									return;
							}
						}
						//assert( itByPos->first == oldX );
					}

					mapByPos.erase( itByPos );
					mapByPos.insert( std::make_pair(newX, pSmartObject) );
				}
			}
			break;
		}
	}
}

void CSmartObjectManager::RebuildNavigation()
{
	// find pointers to helpers to speed-up things
	MapConditions::iterator itConditions, itConditionsEnd = m_Conditions.end();
	for ( itConditions = m_Conditions.begin(); itConditions != itConditionsEnd; ++itConditions )
	{
		CCondition& condition = itConditions->second;

		// ignore event rules
		//	if ( !condition.sEvent.empty() )
		//		continue;

		if ( condition.pUserClass && !condition.sUserHelper.empty() )
			condition.pUserHelper = condition.pUserClass->GetHelper( condition.sUserHelper );
		else
			condition.pUserHelper = NULL;
		if ( condition.pObjectClass && !condition.sObjectHelper.empty() )
			condition.pObjectHelper = condition.pObjectClass->GetHelper( condition.sObjectHelper );
		else
			condition.pObjectHelper = NULL;
	}

	CAISystem* pAISystem = GetAISystem();
	CGraph* pGraph = gAIEnv.pGraph;
	CSmartObjectNavRegion* pNavRegion = gAIEnv.pNavigation->GetSmartObjectsNavRegion();

	pNavRegion->Clear();

	// reset list of links in all classes
	CSmartObjectClass::MapClassesByName::iterator itClasses, itClassesEnd = CSmartObjectClass::g_AllByName.end();
	for ( itClasses = CSmartObjectClass::g_AllByName.begin(); itClasses != itClassesEnd; ++itClasses )
	{
		itClasses->second->ClearHelperLinks();
	}

	// we need a list of potential users to 
	std::set< CSmartObjectClass* > setNavUserClasses;

	// add each navigation condition to object's class
	// MapConditions::iterator itConditions, itConditionsEnd = m_Conditions.end();
	for ( itConditions = m_Conditions.begin(); itConditions != itConditionsEnd; ++itConditions )
	{
		// ignore event rules
		if ( !itConditions->second.sEvent.empty() )
			continue;

		CCondition* pCondition = &itConditions->second;
		if ( pCondition->bEnabled && pCondition->pObjectClass && pCondition->iRuleType == 1 )
			pCondition->pObjectClass->AddHelperLink( pCondition );
	}

	// for each smart object instance create navigation graph nodes and links
	for ( itClasses = CSmartObjectClass::g_AllByName.begin(); itClasses != itClassesEnd; ++itClasses )
	{
		CSmartObjectClass* pClass = itClasses->second;
		if ( !pClass->m_setNavHelpers.empty() )
		{
			pClass->FirstObject();
			CSmartObject* pSmartObject;
			while ( pSmartObject = pClass->NextObject() )
			{
				RegisterInNavigation( pSmartObject, pClass );
			}
		}
	}
}

void CSmartObjectManager::RegisterInNavigation( CSmartObject* pSmartObject, CSmartObjectClass* pClass )
{
	CGraph* pGraph = gAIEnv.pGraph;

	// first check is the graph loaded
	CAllNodesContainer::Iterator it( pGraph->GetAllNodes(), SMART_OBJECT_ENCLOSING_NAV_TYPES );
	if ( !it.GetNode() )
		return;

	typedef std::map< SmartObjectHelper*, unsigned > MapHelpersToNodes;
	MapHelpersToNodes helpersToNodes;

	// create the nodes
	CSmartObjectClass::SetHelpers::iterator itHelpers, itHelpersEnd = pClass->m_setNavHelpers.end();
	for ( itHelpers = pClass->m_setNavHelpers.begin(); itHelpers != itHelpersEnd; ++itHelpers )
	{
		SmartObjectHelper* pHelper = *itHelpers;
		IEntity* pEntity = pSmartObject->GetEntity();

		// calculate position
		Vec3 pos = pEntity->GetWorldTM().TransformPoint( pHelper->qt.t );

		// create a new node
		unsigned newNodeIndex = pGraph->CreateNewNode( IAISystem::NAV_SMARTOBJECT, pos );
		GraphNode* pNewNode = pGraph->GetNodeManager().GetNode(newNodeIndex);
		pNewNode->GetSmartObjectNavData()->pSmartObject = pSmartObject;
		pNewNode->GetSmartObjectNavData()->pClass = pClass;
		pNewNode->GetSmartObjectNavData()->pHelper = pHelper;

		const char* navLinkFailedForUserName = NULL;

		std::set<unsigned> masks;
		for (CSmartObjectClass::THelperLinks::const_iterator it = pClass->m_vHelperLinks.begin(); pClass->m_vHelperLinks.end() != it; ++it)
		{

			if (it->condition && it->condition->pUserClass)
			{
				const AgentPathfindingProperties* props = GetPFPropertiesOfSoUser(it->condition->pUserClass->GetName());

				unsigned mask = (props) ? props->navCapMask.GetFullMask() : static_cast<unsigned>(IAISystem::tNavCapMask(SMART_OBJECT_ENCLOSING_NAV_TYPES));

				if (masks.find(mask) == masks.end())
				{
					// find nearest existing graph node
					unsigned nodeIndex = pGraph->GetEnclosing( pos, mask);

					// If valid node found
					if (nodeIndex > 0)
					{
						// connect the new node with the enclosing node (both ways)
						pGraph->Connect( nodeIndex, newNodeIndex );
						masks.insert(mask);
					}
					else	// Failed to find a node for this user at the helper position
					{
						navLinkFailedForUserName = it->condition->pUserClass->GetName().c_str();
					}
				}
			}
		}

		if (navLinkFailedForUserName)
		{
			AIWarning( "SO \"%s\" will be inaccessible for agent \"%s\" (& possibly other agents) at (%f %f %f)!",
				pSmartObject->GetName(),
				navLinkFailedForUserName,
				pos.x,
				pos.y,
				pos.z);
		}

		//if we didnt connect anything up, use the old way of doing it
		if (masks.empty())
		{
			// find nearest existing graph node
			unsigned nodeIndex = pGraph->GetEnclosing( pos,  SMART_OBJECT_ENCLOSING_NAV_TYPES);
			// connect the new node with the enclosing node (both ways)
			pGraph->Connect( nodeIndex, newNodeIndex );			
		}

		// store the new node in our map
		helpersToNodes[ pHelper ] = newNodeIndex;
	}

	// link the nodes
	CSmartObjectClass::THelperLinks::iterator itLinks, itLinksEnd = pClass->m_vHelperLinks.end();
	for ( itLinks = pClass->m_vHelperLinks.begin(); itLinks != itLinksEnd; ++itLinks )
	{
		unsigned fromNodeIndex = helpersToNodes[ itLinks->from ];
		unsigned toNodeIndex = helpersToNodes[ itLinks->to ];
		if ( fromNodeIndex != toNodeIndex )
		{
			unsigned link;
			pGraph->Connect( fromNodeIndex, toNodeIndex, 100.0f, 0.0f, &link );
			pGraph->GetLinkManager().SetRadius( link, itLinks->passRadius );
			pSmartObject->m_vNavNodes.push_back( link );
		}
	}
}

void CSmartObjectManager::UnregisterFromNavigation( CSmartObject* pSmartObject ) const
{
	// first check is the graph loaded
	CGraph* pGraph = gAIEnv.pGraph;
	CAllNodesContainer::Iterator it( pGraph->GetAllNodes(), IAISystem::NAV_TRIANGULAR );
	if ( !it.GetNode() )
		return;

	// then disable all links
	CGraphLinkManager& manager = pGraph->GetLinkManager();
	CSmartObject::VectorNavNodes::iterator itLinks, itLinksEnd = pSmartObject->m_vNavNodes.end();
	for ( itLinks = pSmartObject->m_vNavNodes.begin(); itLinks != itLinksEnd; ++itLinks )
		manager.ModifyRadius( *itLinks, -1.0f );
}

void CSmartObjectManager::GetTemplateIStatObj( IEntity* pEntity, std::vector< IStatObj* >& statObjects )
{
	statObjects.clear();

	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( !pSmartObject )
		return;

	CSmartObjectClasses& classes = pSmartObject->GetClasses();
	CSmartObjectClasses::iterator it, itEnd = classes.end();
	for ( it = classes.begin(); it != itEnd; ++it )
	{
		CSmartObjectClass* pClass = *it;
		if ( !pClass->m_pTemplateData )
			continue;
		IStatObj* pStatObj = pClass->m_pTemplateData->GetIStateObj();
		if ( !pStatObj )
			continue;
		std::vector< IStatObj* >::iterator itFind = std::find( statObjects.begin(), statObjects.end(), pStatObj );
		if ( itFind == statObjects.end() )
			statObjects.push_back( pStatObj );
	}
}

CSmartObject::MapTemplates& CSmartObject::GetMapTemplates()
{
	if ( m_pMapTemplates.get() )
		return *m_pMapTemplates.get();

	m_pMapTemplates.reset(new MapTemplates);

	CSmartObjectClasses::iterator itClasses, itClassesEnd = m_vClasses.end();
	for ( itClasses = m_vClasses.begin(); itClasses != itClassesEnd; ++itClasses )
	{
		CSmartObjectClass* pClass = *itClasses;
		if ( !pClass->m_NavUsersMaxSize )
			continue; // this class is not used for navigation

		CClassTemplateData* pTemplate = pClass->m_pTemplateData;
		if ( !pTemplate || pTemplate->helpers.empty() )
			continue; // ignore classes for which there's no template defined or there are no helpers in the template

		std::pair< MapTemplates::iterator, bool > insertResult = m_pMapTemplates->insert( std::make_pair( pTemplate, CSmartObject::CTemplateData() ) );
		CSmartObject::CTemplateData& data = insertResult.first->second;
		data.pClass = pClass;
		if ( insertResult.second )
		{
			// new entry - only copy the user
			data.userRadius = pClass->m_NavUsersMaxSize.radius;
			data.userBottom = pClass->m_NavUsersMaxSize.bottom;
			data.userTop = pClass->m_NavUsersMaxSize.top;
		}
		else
		{
			// an entry was found - find the maximum user size
			data.userRadius = max( data.userRadius, pClass->m_NavUsersMaxSize.radius );
			data.userBottom = min( data.userBottom, pClass->m_NavUsersMaxSize.bottom );
			data.userTop = max( data.userTop, pClass->m_NavUsersMaxSize.top );
		}
	}
	return *m_pMapTemplates.get();
}

bool CSmartObjectManager::ValidateTemplate( IEntity* pEntity, bool bStaticOnly, IEntity* pUserEntity /*=NULL*/, int fromTemplateHelperIdx /*=-1*/, int toTemplateHelperIdx /*=-1*/ )
{
	if(!IsInitialized())
		return false;
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( pSmartObject )
		return ValidateTemplate( pSmartObject, bStaticOnly, pUserEntity, fromTemplateHelperIdx, toTemplateHelperIdx );
	return true;
}

bool CSmartObjectManager::ValidateTemplate( CSmartObject* pSmartObject, bool bStaticOnly, IEntity* pUserEntity /*=NULL*/, int fromTemplateHelperIdx /*=-1*/, int toTemplateHelperIdx /*=-1*/ )
{
	if ( m_bRecalculateUserSize )
		RecalculateUserSize();

	CSmartObject::MapTemplates& templates = pSmartObject->GetMapTemplates();
	if ( templates.empty() )
		return true;

	if ( !bStaticOnly && pSmartObject->m_eValidationResult == eSOV_Unknown )
		ValidateTemplate( pSmartObject, true, pUserEntity );

	const Matrix34& wtm = pSmartObject->GetEntity()->GetWorldTM();
	bool result = true;
	pSmartObject->m_eValidationResult = eSOV_Valid;

	CSmartObject::MapTemplates::iterator itTemplates, itTemplatesEnd = templates.end();
	for ( itTemplates = templates.begin(); itTemplates != itTemplatesEnd; ++itTemplates )
	{
		CClassTemplateData* pTemplate = itTemplates->first;

		CSmartObject::CTemplateData& data = itTemplates->second;
		if ( pTemplate->helpers.size() != data.helperInstances.size() )
			data.helperInstances.resize( pTemplate->helpers.size() );
		CSmartObject::TTemplateHelperInstances::iterator itInstances = data.helperInstances.begin();

		CClassTemplateData::TTemplateHelpers::iterator it, itEnd = pTemplate->helpers.end();
		int idx = 0;
		for ( it = pTemplate->helpers.begin(); it != itEnd; ++it, ++itInstances, ++idx )
		{
			CClassTemplateData::CTemplateHelper& helper = *it;

			if (!bStaticOnly)
			{
				// When testing dynamic helpers, check if a pair is specified and only check that pair.
				if (fromTemplateHelperIdx != -1 && toTemplateHelperIdx != -1)
				{
					if (idx != fromTemplateHelperIdx && idx != toTemplateHelperIdx)
						continue;
				}
			}

			if ( !bStaticOnly && itInstances->validationResult == eSOV_InvalidStatic )
			{
				pSmartObject->m_eValidationResult = eSOV_InvalidStatic;
				result = false;
				continue;
			}

			SSOTemplateArea area;
			area.pt = wtm * helper.qt.t;
			area.radius = helper.radius;
			area.projectOnGround = helper.project;

			SSOUser user = { pUserEntity, data.userRadius, data.userTop-data.userBottom, data.userBottom };
			if ( !ValidateSmartObjectArea( area, user, bStaticOnly ? AICE_ALL : AICE_ALL_INLUDING_LIVING, itInstances->position ) )
			{
				itInstances->validationResult = bStaticOnly ? eSOV_InvalidStatic : eSOV_InvalidDynamic;
				pSmartObject->m_eValidationResult = bStaticOnly ? eSOV_InvalidStatic : eSOV_InvalidDynamic;
				result = false;
			}
			else
				itInstances->validationResult = eSOV_Valid;
		}
	}

	return result;
}

void CSmartObjectManager::DrawTemplate( IEntity* pEntity )
{
	CSmartObject* pSmartObject = IsSmartObject( pEntity );
	if ( pSmartObject )
		DrawTemplate( pSmartObject, true );
}

struct TDebugLink
{
	SmartObjectHelper* pFromHelper;
	SmartObjectHelper* pToHelper;
	CSmartObject::CTemplateHelperInstance* pFromInstance;
	CSmartObject::CTemplateHelperInstance* pToInstance;
	float fFromRadius;
	float fToRadius;
	float fUserRadius;
	bool bTwoWay;
};

void CSmartObjectManager::DrawTemplate( CSmartObject* pSmartObject, bool bStaticOnly )
{
	if(!IsInitialized())
		return;

	bool valid = ValidateTemplate( pSmartObject, bStaticOnly );
	
	const Matrix34& wtm = pSmartObject->GetEntity()->GetWorldTM();
	ColorB colors[] = {
		0x40808080u, 0xff808080u, //eSOV_Unknown,
		0x408080ffu, 0xff8080ffu, //eSOV_InvalidStatic,
		0x4080ffffu, 0xff80ffffu, //eSOV_InvalidDynamic,
		0x4080ff80u, 0xff80ff80u, //eSOV_Valid,
	};
	CDebugDrawContext dc;
	Vec3 UP(0,0,1.f);

	typedef std::vector< TDebugLink > VectorDebugLinks;
	VectorDebugLinks links;

	CSmartObject::MapTemplates& templates = pSmartObject->GetMapTemplates();
	CSmartObject::MapTemplates::iterator itTemplates, itTemplatesEnd = templates.end();
	for ( itTemplates = templates.begin(); itTemplates != itTemplatesEnd; ++itTemplates )
	{
		CClassTemplateData* pTemplate = itTemplates->first;
		CSmartObject::CTemplateData& data = itTemplates->second;

		CSmartObjectClass::THelperLinks::iterator itLinks, itLinksEnd = data.pClass->m_vHelperLinks.end();
		for ( itLinks = data.pClass->m_vHelperLinks.begin(); itLinks != itLinksEnd; ++itLinks )
		{
			CSmartObjectClass::HelperLink& helperLink = *itLinks;
			SmartObjectHelper* pFromHelper = helperLink.from;
			SmartObjectHelper* pToHelper = helperLink.to;

			// check is this link already processed
			VectorDebugLinks::iterator it, itEnd = links.end();
			for ( it = links.begin(); it != itEnd; ++it )
			{
				if ( it->pFromHelper == pFromHelper && it->pToHelper == pToHelper )
					break;
				if ( it->pFromHelper == pToHelper && it->pToHelper == pFromHelper )
				{
					it->bTwoWay = true;
					break;
				}
			}
			if ( it == itEnd )
			{
				TDebugLink link =
				{
					pFromHelper,     //	SmartObjectHelper* pFromHelper;
					pToHelper,       //	SmartObjectHelper* pToHelper;
					NULL,            //	CSmartObject::CTemplateHelperInstance* pFromInstance;
					NULL,            //	CSmartObject::CTemplateHelperInstance* pToInstance;
					0,               //	float fFromRadius;
					0,               //	float fToRadius;
					data.userRadius, //	float fUserRadius;
					false            //	bool bTwoWay;
				};
				links.push_back( link );
			}
		}

		CClassTemplateData::TTemplateHelpers::iterator itHelpers = pTemplate->helpers.begin();
		CSmartObject::TTemplateHelperInstances::iterator itInstances, itInstancesEnd = data.helperInstances.end();
		for ( itInstances = data.helperInstances.begin(); itInstances != itInstancesEnd; ++itInstances, ++itHelpers )
		{
			float radius = itHelpers->radius + data.userRadius;
			Vec3 pos = itInstances->position;
			dc->DrawRangeCircle(pos, radius, radius, colors[2*itInstances->validationResult], colors[1+2*itInstances->validationResult], true );

			if ( bStaticOnly )
			{
				pos.z += (data.userTop + data.userBottom)*0.5f;
				dc->DrawCylinder( pos, UP, radius, data.userTop - data.userBottom, colors[2*itInstances->validationResult] );
			}

			VectorDebugLinks::iterator it, itEnd = links.end();
			for ( it = links.begin(); it != itEnd; ++it )
			{
				if ( !it->pFromInstance && it->pFromHelper->name == itHelpers->name )
				{
					it->pFromInstance = &*itInstances;
					it->fFromRadius = radius;
				}
				if ( !it->pToInstance && it->pToHelper->name == itHelpers->name )
				{
					it->pToInstance = &*itInstances;
					it->fToRadius = radius;
				}
			}
		}
	}

	// draw arrows to show smart object links
	VectorDebugLinks::iterator it, itEnd = links.end();
	for ( it = links.begin(); it != itEnd; ++it )
	{
		if ( !it->pFromInstance || !it->pToInstance )
			continue;
		Vec3 pos, length;
		pos = it->pFromInstance->position;
		length = it->pToInstance->position - pos;
		if ( length.GetLengthSquared2D() > sqr(it->fFromRadius + it->fToRadius) )
		{
			Vec3 n = length;
			n.z = 0;
			n.Normalize();
			pos += n * it->fFromRadius;
			length -= n * ( it->fFromRadius + it->fToRadius );
		}
		if ( it->bTwoWay )
		{
			length *= 0.5f;
			pos += length;
		}
		dc->DrawArrow(pos, length, 2 * it->fUserRadius, colors[2 * pSmartObject->m_eValidationResult] );
		if ( it->bTwoWay )
			dc->DrawArrow(pos, -length, 2 * it->fUserRadius, colors[2 * pSmartObject->m_eValidationResult] );
	}
}

#ifdef AUTO_SMART_OBJECT
void CSmartObjectManager::PrepareAutomatedSmartObject(CSmartObject *pUser, CSmartObject *pObject, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper, const Vec3 *pDestination)
{
	if(!pUser || !pObject || !pFromHelper || !pToHelper)
		return;

	//create new auto SO
	SAutoSmartObject newSO;
	IEntity *pSmartObjectEntity = pObject->GetEntity();
	Vec3 vSOPos = pSmartObjectEntity->GetWorldPos();
	Quat qRotInv = pSmartObjectEntity->GetRotation().GetInverted();
	//apply target data
	newSO.m_vAutoUseStart = vSOPos + pFromHelper->qt.t * qRotInv;
	newSO.m_vAutoUseEnd = vSOPos + pToHelper->qt.t * qRotInv;
	newSO.m_vDestination = pDestination?(*pDestination):Vec3(ZERO);
	newSO.m_pAutoUser = pUser->GetEntity();
	newSO.m_fStartTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();

	float fUserDistance = (pUser->GetEntity()->GetWorldPos() - newSO.m_vAutoUseStart).len();
	if(fUserDistance > EMPTY_SO_USE_DISTANCE)
		return;

	m_autoSmartObjectList.push_back(newSO);
}

void CSmartObjectManager::UpdateAutomatedSmartObject()
{
	if(m_autoSmartObjectList.empty())
		return;

	std::vector<SAutoSmartObject>::iterator it = m_autoSmartObjectList.begin();
	while(it != m_autoSmartObjectList.end())
	{
		SAutoSmartObject &pASO = *it;
		Vec3 vUserPos = pASO.m_pAutoUser->GetWorldPos();
		Vec3 vDirToStart = pASO.m_vAutoUseStart - vUserPos;
		Vec3 vDirToEnd = pASO.m_vAutoUseEnd - vUserPos;

		if(pASO.m_fStartTime - gEnv->pTimer->GetFrameStartTime().GetSeconds() > 30.0f)
		{
			//timeout
			pe_params_part params;
			params.flagsOR = geom_colltype0;
			pASO.m_pAutoUser->GetPhysics()->SetParams(&params);
			//remove ASO
			it = m_autoSmartObjectList.erase(it);
			continue;
		}

		//compute movement distance - terminate if arrived
		Vec3 vTempDir = vDirToEnd;
		vTempDir.z = 0.0f;
		float fDistToEnd = vTempDir.len();
		if(fDistToEnd < 0.1f)
		{
			it = m_autoSmartObjectList.erase(it);
			//turn physics back on
			pe_params_part params;
			params.flagsOR = geom_colltype0;
			pASO.m_pAutoUser->GetPhysics()->SetParams(&params);

			//retrigger original target
			IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
			pData->iValue = gEnv->pAISystem->AllocGoalPipeId();
			pData->point = pASO.m_vDestination;
			CAIActor *pActor = pASO.m_pAutoUser->GetAI()->CastToCAIActor();
			pActor->SetSignal(10, "ACT_GOTO", pASO.m_pAutoUser, pData);
			//************************

			continue;
		}

		//compare start distance to end distance
		Vec3 vDir = vDirToEnd;

		vTempDir = vDirToStart;
		vTempDir.z = 0.0f;
		float fDistToStart = vTempDir.len();
		if(fDistToStart < (fDistToEnd - 0.1f))
		{
			if(fDistToStart < 0.1f) //teminate path to start by resetting the start pos
				pASO.m_vAutoUseStart = Vec3(ZERO);
			//start is first target
			vDir = vDirToStart;
		}

		//turn off physics
		pe_params_part params;
		params.flagsAND = ~geom_colltype0;
		pASO.m_pAutoUser->GetPhysics()->SetParams(&params);

		//TO_DO : normalize speed with max distance
		Vec3 vNewPos = vUserPos + vDir * gEnv->pTimer->GetFrameTime() * 2.0f;
		Matrix34 tm = pASO.m_pAutoUser->GetWorldTM();
		tm.SetTranslation(vNewPos);
		pASO.m_pAutoUser->SetWorldTM(tm);

		//iterate
		++it;
	}
}
#endif

#define CLASS_TEMPLATES_FOLDER "Libs/SmartObjects/ClassTemplates/"

void CSmartObjectManager::LoadSOClassTemplates()
{
	m_mapClassTemplates.clear();

	string sLibPath = PathUtil::Make( CLASS_TEMPLATES_FOLDER, "*", "xml" );
	ICryPak *pack = gEnv->pCryPak;
	_finddata_t fd;
	intptr_t handle = pack->FindFirst( sLibPath, &fd );
	if ( handle < 0 )
		return;

	do
	{
		XmlNodeRef root = GetISystem()->LoadXmlFile( string(CLASS_TEMPLATES_FOLDER) + fd.name );
		if ( !root )
			continue;

		CClassTemplateData classTemplateData;
		classTemplateData.name =  PathUtil::GetFileName( fd.name );
		classTemplateData.name.MakeLower();
		CClassTemplateData* data =  & m_mapClassTemplates.insert(std::make_pair( classTemplateData.name, classTemplateData )).first->second;

		int count = root->getChildCount();
		for ( int i = 0; i < count; ++i )
		{
			XmlNodeRef node = root->getChild(i);
			if ( node->isTag("object") )
			{
				data->model = node->getAttr( "model" );
			}
			else if ( node->isTag("helper") )
			{
				CClassTemplateData::CTemplateHelper helper;
				helper.name = node->getAttr( "name" );
				node->getAttr( "pos", helper.qt.t );
				node->getAttr( "rot", helper.qt.q );
				node->getAttr( "radius", helper.radius );
				node->getAttr( "projectOnGround", helper.project );
				data->helpers.push_back( helper );
			}
		}
	} while ( pack->FindNext( handle, &fd ) >= 0 );

	pack->FindClose(handle);
}

void CSmartObjectManager::SetTemporarySmartObject( const QuatT& location, const char* listStates )
{
	if(!IsInitialized())
		return;
	g_tempSOLocation = location;	
	g_pTemporarySO->m_States.clear();
	CSmartObject::DoubleVectorStates states;
	String2States( listStates, states );
	g_pTemporarySO->m_States.insert( states.positive.begin(), states.positive.end() );
}

void CSmartObjectManager::RemoveSmartObject( IEntity *pEntity )
{
	if(!IsInitialized())
		return;

	//TODO remove this forwarding
	RemoveEntity( pEntity );
}

const char* CSmartObjectManager::GetSmartObjectStateName( int id ) const
{
	AIAssert(IsInitialized());
	return CSmartObject::CState::GetStateName( id );
}

void CSmartObjectManager::RegisterSmartObjectState( const char* sStateName )
{
	if ( *sStateName )
	{
		// it's enough to create only one temp. instance of CState
		CSmartObject::CState state( sStateName );
	}
}

void CSmartObjectManager::DrawSOClassTemplate( IEntity* pEntity )
{
	//TODO get rid of forwarding
	DrawTemplate( pEntity );
}

void CSmartObjectManager::GetSOClassTemplateIStatObj( IEntity* pEntity, IStatObj**& pStatObjects )
{
	if(!IsInitialized())
		return;

	pStatObjects = NULL;
	RescanSOClasses( pEntity );
	static std::vector< IStatObj* > statObjects;
	GetTemplateIStatObj( pEntity, statObjects );
	if ( !statObjects.empty() )
		pStatObjects = &statObjects[0];
}

void CSmartObjectManager::ReloadSmartObjectRules()
{
	CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
	AIAssert( gEnv->pEntitySystem );
	// reset states of smart objects to initial state
	ClearConditions();
	// ...and reload them
	LoadSmartObjectsLibrary();
	CTimeValue t2 = gEnv->pTimer->GetAsyncTime();
	AILogComment( "All smart object rules reloaded in %g mSec.", (t2-t1).GetMilliSeconds() );
}

int CSmartObjectManager::SmartObjectEvent( const char* sEventName, IEntity*& pUser, IEntity*& pObject, const Vec3* pExtraPoint /*= NULL*/, bool bHighPriority /*= false */ )
{
	//TODO remove forwarding to have just one function?
	return TriggerEvent( sEventName, pUser, pObject, NULL, pExtraPoint, bHighPriority );
}

SmartObjectHelper* CSmartObjectManager::GetSmartObjectHelper( const char* className, const char* helperName ) const
{
	AIAssert(IsInitialized());
	CSmartObjectClass* pClass = CSmartObjectClass::find( className );
	if ( pClass )
		return pClass->GetHelper( helperName );
	else
		return NULL;
}

bool CSmartObjectManager::LoadSmartObjectsLibrary()
{
	m_initialized = true;
	LoadSOClassTemplates();

	char szPath[512];
	sprintf(szPath,"%s",SMART_OBJECTS_XML);

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Navigation, 0, "Smart objects library (%s)", szPath);

	XmlNodeRef root = GetISystem()->LoadXmlFile( szPath );
	if ( !root )
		return false;

	m_mapHelpers.clear();

	SmartObjectHelper helper;
	SmartObjectCondition condition;
	int count = root->getChildCount();
	for ( int i = 0; i < count; ++i )
	{
		XmlNodeRef node = root->getChild(i);
		if ( node->isTag("SmartObject") )
		{
			condition.iTemplateId = 0;
			node->getAttr( "iTemplateId", condition.iTemplateId );

			condition.sName = node->getAttr( "sName" );
			condition.sDescription = node->getAttr( "sDescription" );
			condition.sFolder = node->getAttr( "sFolder" );
			condition.sEvent = node->getAttr( "sEvent" );

			node->getAttr( "iMaxAlertness", condition.iMaxAlertness );
			node->getAttr( "bEnabled", condition.bEnabled );

			condition.iRuleType = 0;
			node->getAttr( "iRuleType", condition.iRuleType );
			float fPassRadius = 0.0f; // default value if not found in XML
			node->getAttr( "fPassRadius", fPassRadius );
			if ( fPassRadius > 0 )
				condition.iRuleType = 1;

			condition.sEntranceHelper = node->getAttr( "sEntranceHelper" );
			condition.sExitHelper = node->getAttr( "sExitHelper" );

			condition.sUserClass = node->getAttr( "sUserClass" );
			condition.sUserState = node->getAttr( "sUserState" );
			condition.sUserHelper = node->getAttr( "sUserHelper" );

			condition.sObjectClass = node->getAttr( "sObjectClass" );
			condition.sObjectState = node->getAttr( "sObjectState" );
			condition.sObjectHelper = node->getAttr( "sObjectHelper" );

			node->getAttr( "fMinDelay", condition.fMinDelay );
			node->getAttr( "fMaxDelay", condition.fMaxDelay );
			node->getAttr( "fMemory", condition.fMemory );

			condition.fDistanceFrom = 0.0f; // default value if not found in XML
			node->getAttr( "fDistanceFrom", condition.fDistanceFrom );
			node->getAttr( "fDistanceLimit", condition.fDistanceTo );
			node->getAttr( "fOrientationLimit", condition.fOrientationLimit );
			node->getAttr( "bHorizLimitOnly", condition.bHorizLimitOnly );
			condition.fOrientationToTargetLimit = 360.0f; // default value if not found in XML
			node->getAttr( "fOrientToTargetLimit", condition.fOrientationToTargetLimit );

			node->getAttr( "fProximityFactor", condition.fProximityFactor );
			node->getAttr( "fOrientationFactor", condition.fOrientationFactor );
			node->getAttr( "fVisibilityFactor", condition.fVisibilityFactor );
			node->getAttr( "fRandomnessFactor", condition.fRandomnessFactor );

			node->getAttr( "fLookAtOnPerc", condition.fLookAtOnPerc );
			condition.sUserPreActionState = node->getAttr( "sUserPreActionState" );
			condition.sObjectPreActionState = node->getAttr( "sObjectPreActionState" );

			condition.sAction = node->getAttr( "sAction" );
			int iActionType;
			if (node->getAttr( "eActionType", iActionType ))
			{
				condition.eActionType = (EActionType) iActionType;
			}
			else
			{
				condition.eActionType = eAT_None;

				bool bSignal = !condition.sAction.empty() && condition.sAction[0] == '%';
				bool bAnimationSignal = !condition.sAction.empty() && condition.sAction[0] == '$';
				bool bAnimationAction = !condition.sAction.empty() && condition.sAction[0] == '@';
				bool bAction = !condition.sAction.empty() && !bSignal && !bAnimationSignal && !bAnimationAction;

				bool bHighPriority = condition.iMaxAlertness >= 100;
				node->getAttr( "bHighPriority", bHighPriority );

				if ( bHighPriority && bAction )
					condition.eActionType = eAT_PriorityAction;
				else if ( bAction )
					condition.eActionType = eAT_Action;
				else if ( bSignal )
					condition.eActionType = eAT_AISignal;
				else if ( bAnimationSignal )
					condition.eActionType = eAT_AnimationSignal;
				else if ( bAnimationAction )
					condition.eActionType = eAT_AnimationAction;

				if ( bSignal || bAnimationSignal || bAnimationAction )
					condition.sAction.erase( 0, 1 );
			}
			condition.iMaxAlertness %= 100;
			if ( !node->getAttr( "bEarlyPathRegeneration", condition.bEarlyPathRegeneration ) )
				condition.bEarlyPathRegeneration = false;

			condition.sUserPostActionState = node->getAttr( "sUserPostActionState" );
			condition.sObjectPostActionState = node->getAttr( "sObjectPostActionState" );

			node->getAttr( "fApproachSpeed", condition.fApproachSpeed );
			node->getAttr( "iApproachStance", condition.iApproachStance );
			condition.sAnimationHelper = node->getAttr( "sAnimationHelper" );
			condition.sApproachHelper = node->getAttr( "sApproachHelper" );
			condition.fStartWidth = 0;
			node->getAttr( "fStartWidth", condition.fStartWidth );
			node->getAttr( "fStartDirectionTolerance", condition.fDirectionTolerance );
			condition.fStartArcAngle = 0;
			node->getAttr( "fStartArcAngle", condition.fStartArcAngle );

			if ( !node->getAttr("iOrder", condition.iOrder) )
				condition.iOrder = i;

			AddSmartObjectCondition( condition );
		}
		else if ( node->isTag("Helper") )
		{
			string className = node->getAttr( "className" );
			helper.name = node->getAttr( "name" );
			Vec3 pos;
			node->getAttr( "pos", pos );
			Quat rot;
			node->getAttr( "rot", rot );
			helper.qt = QuatT( rot, pos );
			helper.description = node->getAttr( "description" );

			MapSOHelpers::iterator it = m_mapHelpers.insert( std::make_pair(className, helper) );

			CSmartObjectClass* pClass = GetSmartObjectClass( className );
			pClass->AddHelper( &it->second );
		}
		else if ( node->isTag("Class") )
		{
			string templateName = node->getAttr( "template" );
			if ( !templateName.empty() )
			{
				templateName.MakeLower();
				string className = node->getAttr( "name" );
				CSmartObjectClass* pClass = GetSmartObjectClass( className );
				if ( pClass )
				{
					MapClassTemplateData::iterator itFind = m_mapClassTemplates.find( templateName );
					if ( itFind != m_mapClassTemplates.end() )
					{
						CClassTemplateData* pTemplateData = &itFind->second;
						pClass->m_pTemplateData = pTemplateData;
						CClassTemplateData::TTemplateHelpers::iterator itHelpers, itHelpersEnd = pTemplateData->helpers.end();
						int idx = 0;
						for ( itHelpers = pTemplateData->helpers.begin(); itHelpers != itHelpersEnd; ++itHelpers, ++idx )
						{
							helper.name = itHelpers->name;
							helper.qt = itHelpers->qt;
							helper.description.clear();
							helper.templateHelperIndex = idx;
							MapSOHelpers::iterator it = m_mapHelpers.insert( std::make_pair(className, helper) );
							pClass->AddHelper( &it->second );
						}
					}
				}
			}
		}
	}

	SoftReset();
	return true;
}

void CSmartObjectManager::DebugDrawValidateSmartObjectArea() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("ValidateSmartObjectArea");
	if (ent)
	{
		const Vec3 & pos = ent->GetPos();
		const SSOTemplateArea templateArea = { pos, 1.0f, true };
		const SSOUser user = {NULL, 0.4f, 1.5f, 0.5f};
		const EAICollisionEntities collEntities = AICE_ALL;
		Vec3 floorPos = pos;

		bool ok = ValidateSmartObjectArea(templateArea, user, collEntities, floorPos);

		ColorB color;
		if (ok)
			color.Set(0, 255, 0);
		else
			color.Set(255, 0, 0);
		CDebugDrawContext dc;
		dc->DrawSphere(floorPos + Vec3(0, 0, user.groundOffset), user.radius, color);
		dc->DrawSphere(floorPos + Vec3(0, 0, user.groundOffset + user.height), user.radius, color);
		dc->DrawLine(floorPos, color, floorPos + Vec3(0, 0, user.groundOffset), color);
	}
}

float CSmartObjectManager::GetSmartObjectLinkCostFactor( const GraphNode* nodes[2], const CAIObject *pRequester ) const
{
	if (!pRequester)
		return -1;

	if(!IsInitialized())
		return -1;

	// the link must be only between two nodes created by the same smart object
	AIAssert( nodes[0]->GetSmartObjectNavData()->pSmartObject == nodes[1]->GetSmartObjectNavData()->pSmartObject );

	// also it must be between two helpers belonging to a same smart object class
	AIAssert( nodes[0]->GetSmartObjectNavData()->pClass == nodes[1]->GetSmartObjectNavData()->pClass );

	CSmartObject* pObject = nodes[0]->GetSmartObjectNavData()->pSmartObject;
	if ( pObject->GetValidationResult() == eSOV_InvalidStatic )
	{
		if (gAIEnv.CVars.DebugPathFinding)
		{
			AILogAlways("CAISystem::GetSmartObjectLinkCostFactor %s NAVSO %s is statically invalid.",
				pRequester->GetName(), pObject->GetEntity() ? pObject->GetEntity()->GetName() : "<unknown>");
		}
		return -1;
	}

	if (m_bannedNavSmartObjects.find(pObject) != m_bannedNavSmartObjects.end())
		return -1;

	CSmartObjectClass* pObjectClass = nodes[0]->GetSmartObjectNavData()->pClass;
	SmartObjectHelper* pFromHelper = nodes[0]->GetSmartObjectNavData()->pHelper;
	SmartObjectHelper* pToHelper = nodes[1]->GetSmartObjectNavData()->pHelper;

	const CPipeUser* pPipeUser = pRequester->CastToCPipeUser();

	IEntity* pEntity = NULL;
	if (!pPipeUser)
	{
		pEntity = pRequester->GetEntity();

		if(!pEntity)
		{
			return -1;
		}
	}

	CSmartObjectClass::HelperLink* vHelperLinks[1];
	if ( FindSmartObjectLinks( pPipeUser, pObject, pObjectClass, pFromHelper, pToHelper, vHelperLinks, 1, pEntity ) )
		return vHelperLinks[0]->condition->fProximityFactor;
	else if (gAIEnv.configuration.eCompatibilityMode != ECCM_CRYSIS2)
		pPipeUser->InvalidateSOLink( pObject, pFromHelper, pToHelper );

	if (gAIEnv.CVars.DebugPathFinding)
	{
		AILogAlways("CAISystem::GetSmartObjectLinkCostFactor %s Cannot find any links for NAVSO %s.",
			pRequester->GetName(), pObject->GetEntity() ? pObject->GetEntity()->GetName() : "<unknown>");
	}
	return -1;
}

int CSmartObjectManager::GetNavigationalSmartObjectActionType( CPipeUser* pPipeUser, const GraphNode* pFromNode, const GraphNode* pToNode )
{
	IEntity* pEntity = pPipeUser->GetEntity();
	CSmartObject* pUser = gAIEnv.pSmartObjectManager->IsSmartObject( pEntity );
	if(!pUser)
		return nSOmNone;

	CSmartObject* pObject = pFromNode->GetSmartObjectNavData()->pSmartObject;
	CSmartObjectClass* pObjectClass = pFromNode->GetSmartObjectNavData()->pClass;
	SmartObjectHelper* pFromHelper = pFromNode->GetSmartObjectNavData()->pHelper;
	SmartObjectHelper* pToHelper = pToNode->GetSmartObjectNavData()->pHelper;

	CSmartObjectClass::HelperLink* vHelperLinks[1];
	if ( !FindSmartObjectLinks( pPipeUser, pObject, pObjectClass, pFromHelper, pToHelper, vHelperLinks ) )
	{
		return nSOmNone;
	}

	switch( vHelperLinks[0]->condition->eActionType )
	{
	case eAT_AnimationSignal:
	case eAT_PriorityAnimationSignal:
		// there is an ending animation to be played
		return nSOmSignalAnimation;
	case eAT_AnimationAction:
	case eAT_PriorityAnimationAction:
		// there is an ending animation to be played
		return nSOmActionAnimation;
	case eAT_None:
		// directly passable link
		return nSOmStraight;
	default:
		// Only the above methods are supported. The definition of the navSO is likely bad.
		return nSOmNone;
	}
	return nSOmNone;
}

void CSmartObjectManager::DebugDrawBannedNavsos()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;

	// Draw banned smartobjects.
	for (SmartObjectFloatMap::iterator it = m_bannedNavSmartObjects.begin(); it != m_bannedNavSmartObjects.end(); ++it)
	{
		DrawTemplate(it->first, false);
		dc->Draw3dLabel(it->first->GetPos(), 1.2f, "%s\nBanned %.1fs", it->first->GetName(), it->second);
	}
}

bool CSmartObjectManager::PrepareNavigateSmartObject( CPipeUser* pPipeUser, const GraphNode* pFromNode, const GraphNode* pToNode )
{
	// Sanity check that the nodes are indeed smart object nodes (the node data is accessed later).
	if (pFromNode->navType != IAISystem::NAV_SMARTOBJECT || pToNode->navType != IAISystem::NAV_SMARTOBJECT)
	{
		if (gAIEnv.CVars.DebugPathFinding)
		{
			AILogAlways("CAISystem::PrepareNavigateSmartObject %s one of the input nodes is not SmartObject nav node.",
				pPipeUser->GetName());
		}
		return false;
	}

	IEntity* pEntity = pPipeUser->GetEntity();
	CSmartObject* pUser = gAIEnv.pSmartObjectManager->IsSmartObject( pEntity );
	if ( !pUser )
		return false;

	CSmartObject* pObject = pFromNode->GetSmartObjectNavData()->pSmartObject;
	CSmartObjectClass* pObjectClass = pFromNode->GetSmartObjectNavData()->pClass;
	SmartObjectHelper* pFromHelper = pFromNode->GetSmartObjectNavData()->pHelper;
	SmartObjectHelper* pToHelper = pToNode->GetSmartObjectNavData()->pHelper;

	// Check if the NAVSO is valid.
	if ( pObject->GetValidationResult() == eSOV_InvalidStatic )
	{
		if (gAIEnv.CVars.DebugPathFinding)
		{
			AILogAlways("CAISystem::PrepareNavigateSmartObject %s NAVSO %s is statically invalid.",
				pPipeUser->GetName(), pObject->GetEntity() ? pObject->GetEntity()->GetName() : "<unknown>");
		}
		return false;
	}

	if ( !ValidateTemplate(pObject, false, pPipeUser->GetEntity(),
		pFromHelper->templateHelperIndex, pToHelper->templateHelperIndex) )
	{
		if (gAIEnv.CVars.DebugPathFinding)
		{
			AILogAlways("CAISystem::PrepareNavigateSmartObject %s Template validation for NAVSO %s failed.",
				pPipeUser->GetName(), pObject->GetEntity() ? pObject->GetEntity()->GetName() : "<unknown>");
		}
		m_bannedNavSmartObjects[pObject] = gAIEnv.CVars.BannedNavSoTime;
		return false;
	}

	CSmartObjectClass::HelperLink* vHelperLinks[8];
	uint32 numVariations = FindSmartObjectLinks( pPipeUser, pObject, pObjectClass, pFromHelper, pToHelper, vHelperLinks, 8 );

	// remove non-animation rules
	int i = numVariations;
	while ( i-- )
	{
		if ( vHelperLinks[i]->condition->eActionType != eAT_AnimationSignal && vHelperLinks[i]->condition->eActionType != eAT_AnimationAction
			&& vHelperLinks[i]->condition->eActionType != eAT_PriorityAnimationSignal && vHelperLinks[i]->condition->eActionType != eAT_PriorityAnimationAction )
			std::swap( vHelperLinks[i], vHelperLinks[--numVariations] );
	}

	if ( !numVariations )
	{
		return false;
	}
	// randomly choose one of the available rules
	std::swap( vHelperLinks[0], vHelperLinks[Random( numVariations )] );

	// there is a matching smart object rule - use it!
	pPipeUser->m_State.actorTargetReq.Reset();
	pPipeUser->m_State.actorTargetReq.lowerPrecision = true;
	if ( gAIEnv.pSmartObjectManager->PrepareUseNavigationSmartObject( pPipeUser->m_State.actorTargetReq, pPipeUser->m_pendingNavSOStates, pUser, pObject, vHelperLinks[0]->condition, pFromHelper, pToHelper ) )
	{
		pPipeUser->m_State.actorTargetReq.approachLocation = pObject->GetHelperPos( pFromHelper );
		pPipeUser->m_State.actorTargetReq.approachDirection = pObject->GetOrientation( pFromHelper );

		pPipeUser->m_State.actorTargetReq.animLocation = pPipeUser->m_State.actorTargetReq. approachLocation;
		pPipeUser->m_State.actorTargetReq.animDirection = pPipeUser->m_State.actorTargetReq.approachDirection;
		if ( !vHelperLinks[0]->condition->sAnimationHelper.empty() )
		{
			// animation helper has been specified - override the animLocation and direction
			if ( vHelperLinks[0]->condition->sAnimationHelper.c_str()[0] == '<' )
			{
				pPipeUser->m_State.actorTargetReq.animLocation = pObject->GetPos();
				pPipeUser->m_State.actorTargetReq.useAssetAlignment = true;
				if ( vHelperLinks[0]->condition->sAnimationHelper == "<Auto>" )
					pPipeUser->m_State.actorTargetReq.animDirection = pObject->GetOrientation(NULL);
				else
					pPipeUser->m_State.actorTargetReq.animDirection = Quat::CreateRotationZ(gf_PI) * pObject->GetOrientation(NULL);
			}
			else
			{
				if ( const SmartObjectHelper* pAnimHelper = pObjectClass->GetHelper( vHelperLinks[0]->condition->sAnimationHelper.c_str() ) )
				{
					pPipeUser->m_State.actorTargetReq.animLocation = pObject->GetHelperPos( pAnimHelper );
					pPipeUser->m_State.actorTargetReq.animDirection = pObject->GetOrientation( pAnimHelper );
				}
			}
		}

		pPipeUser->m_navSOEarlyPathRegen = vHelperLinks[0]->condition->bEarlyPathRegeneration;

		switch( vHelperLinks[0]->condition->eActionType )
		{
		case eAT_AnimationSignal:
		case eAT_PriorityAnimationSignal:
			// there is an ending animation to be played
			pPipeUser->m_eNavSOMethod = nSOmSignalAnimation;
			return true;
		case eAT_AnimationAction:
		case eAT_PriorityAnimationAction:
			// there is an ending animation to be played
			pPipeUser->m_eNavSOMethod = nSOmActionAnimation;
			return true;
		default:
			// Only the above methods are supported. The definition of the navSO is likely bad.
			AIAssert(0);
		}
	}

	return false;
}

void CSmartObjectManager::MapPathTypeToSoUser( const string &soUser, const string &pathType )
{
	m_MappingSOUserPathType[soUser] = pathType;
}

bool CSmartObjectManager::ValidateSmartObjectArea( const SSOTemplateArea &templateArea, const SSOUser &user, EAICollisionEntities collEntities, Vec3 &groundPos )
{
	groundPos = templateArea.pt;

	static float upDist = 1.0f;
	static float downDist = 1.0f;

	if (templateArea.projectOnGround && 
		!GetFloorPos(groundPos, templateArea.pt, upDist, downDist, walkabilityDownRadius, collEntities))
		return false;

	IPhysicalEntity* pSkipEntity = user.entity ? user.entity->GetPhysics() : NULL;

	Lineseg seg(groundPos + Vec3(0.0f, 0.0f, user.groundOffset), groundPos + Vec3(0.0f, 0.0f, user.groundOffset + user.height));
	if (OverlapCylinder(seg, user.radius + templateArea.radius, collEntities, pSkipEntity,geom_colltype_player))
		return false;

	return true;
}

int CSmartObjectManager::FindSmartObjectLinks( const CPipeUser* pPipeUser, CSmartObject* pObject, CSmartObjectClass* pObjectClass, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper, CSmartObjectClass::HelperLink** pvHelperLinks, int iCount /*= 1*/, IEntity* pAdditionalEntity /*= NULL */ ) const
{
	AIAssert(IsInitialized());

	// hidden objects can not be used!
	if ( pObject->IsHidden() )
		return 0;

	IEntity* pEntity = pAdditionalEntity;
	if(pPipeUser)
		pEntity = pPipeUser->GetEntity();

	if( !pEntity )
		return 0;

	CSmartObject* pUser = IsSmartObject( pEntity );
	if ( !pUser )
		return 0;

	int numFound = 0;

	// check which user's classes can use the target object
	CSmartObjectClasses& vClasses = pUser->GetClasses();
	CSmartObjectClasses::iterator itClasses, itClassesEnd = vClasses.end();
	for ( itClasses = vClasses.begin(); iCount && itClasses != itClassesEnd; ++itClasses )
	{
		CSmartObjectClass* pClass = *itClasses;
		if ( pClass->IsSmartObjectUser() )
		{
			int count = pObjectClass->FindHelperLinks( pFromHelper, pToHelper, pClass, 1.0f, pUser->GetStates(), pObject->GetStates(),
				pvHelperLinks + numFound, iCount );
			numFound += count;
			iCount -= count;
		}
	}

	return numFound;
}

const AgentPathfindingProperties* CSmartObjectManager::GetPFPropertiesOfSoUser( const string& soUser )
{
	std::map<string, string>::iterator it = m_MappingSOUserPathType.find(soUser);

	if (m_MappingSOUserPathType.end() != it)
	{
		return GetAISystem()->GetPFPropertiesOfPathType(it->second);
	}

	return GetAISystem()->GetPFPropertiesOfPathType("AIPATH_DEFAULT");
}

void CSmartObjectManager::UpdateBannedSOs( float frameDeltaTime )
{
	// Update banned SOs
	SmartObjectFloatMap::iterator it = m_bannedNavSmartObjects.begin();
	while (it != m_bannedNavSmartObjects.end())
	{
		it->second -= frameDeltaTime;
		if (it->second <= 0.0f)
			it = m_bannedNavSmartObjects.erase(it);
		else
			++it;
	}
}

void CSmartObjectManager::ResetBannedSOs()
{
	m_bannedNavSmartObjects.clear();
}

bool CSmartObjectManager::IsInitialized()const
{
	if(!m_initialized)
		GetAISystem()->Warning(__FUNCTION__, "Something is trying to access smart object system before it is initialized!");
	return m_initialized;
}

float CSmartObjectManager::CalculateDot(CCondition* condition, const Vec3& dir, CSmartObject* user) const
{
	if (condition->bHorizLimitOnly)
	{
		Vec3 dirOnFloor(dir.x, dir.y, 0.0f);
		dirOnFloor.Normalize();
		return dirOnFloor.Dot(user->GetOrientation(condition->pUserHelper));
	}
	else
	{
		return dir.normalized().Dot(user->GetOrientation(condition->pUserHelper));
	}
}

//===================================================================
// GetNavNodes
//===================================================================
unsigned CSmartObject::GetNavNodes( unsigned nodeIndices[2], const SmartObjectHelper* pHelper )
{
	std::pair< MapNavNodeIds::iterator, bool > result = m_mapNavNodes.insert( std::make_pair(pHelper, 0) );
	if ( result.second )
	{
		// new entry - find enclosing node in store the result
		result.first->second = gAIEnv.pGraph->GetEnclosing(
			pHelper ? GetHelperPos(pHelper) : GetPos(), SMART_OBJECT_ENCLOSING_NAV_TYPES );
	}
	nodeIndices[0] = result.first->second;
	return nodeIndices[0] ? 1 : 0;
}

Vec3 CSmartObject::MeasureUserSize() const
{
	Vec3 result( ZERO );

	CAIObject* pObject = GetAI();
	if ( !pObject || pObject->GetType() != AIOBJECT_PUPPET )
		return result;

	IAIActorProxy* pProxy = pObject->GetProxy();
	if ( !pProxy )
		return result;

	EStance stances[] =
	{
		STANCE_STAND,
		STANCE_CROUCH,
		STANCE_RELAXED,
		STANCE_STEALTH,
		STANCE_LOW_COVER,
		STANCE_HIGH_COVER,
	};
	unsigned numStances = sizeof(stances) / sizeof(EStance);

	AABB stanceSize( AABB::RESET );
	AABB colliderSize( AABB::RESET );

	for ( unsigned i = 0; i < numStances; ++i )
	{
		SAIBodyInfo bi;
		if ( pProxy->QueryBodyInfo(SAIBodyInfoQuery(stances[i], 0.0f, 0.0f, true), bi) && bi.colliderSize.min.z >= 0.0f )
		{
			stanceSize.Add( bi.stanceSize );
			colliderSize.Add( bi.colliderSize );
		}
	}

	if ( !stanceSize.IsEmpty() && !colliderSize.IsEmpty() )
	{
		result.x = max( colliderSize.GetSize().x, colliderSize.GetSize().y ) * 0.5f; // radius
		result.y = colliderSize.GetSize().z; // height
		result.z = stanceSize.GetSize().z - colliderSize.GetSize().z; // distance from ground
	}

	CCCPOINT(CSmartObject_MeasureUserSize);
	return result;
}


//===================================================================
// CQueryEvent::Serialize
//===================================================================
void CQueryEvent::Serialize(TSerialize ser)
{
	ser.BeginGroup("QueryEvent");

	CSmartObjectManager::SerializePointer(ser,"pUser",pUser);
	CSmartObjectManager::SerializePointer(ser,"pObject",pObject);
	CSmartObjectManager::SerializePointer(ser,"pRule",pRule);
		// TO DO: serialize the following when they'll be used
		// pChainedUserEvent;
		// pChainedObjectEvent;

	ser.EndGroup();
}
