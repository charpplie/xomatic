/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   GroupMember.cpp
Description: 
---------------------------------------------------------------------
History:
- 06:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include "GroupSystem.h"
#include "GroupMember.h"
#include "SteeringAgent.h"
#include "../Puppet.h"
#include "../TacticalPointSystem/TacticalPointSystem.h"
#include "DebugDrawContext.h"

#define DIST_THRESHOLD	( 2.0f )

// this define is used to temporarily disable steering for group members.
// steering problems need to be handled properly later on.
#define DISABLE_GROUP_STEERING

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CGroupMember::CGroupMember( CGroupSystem* pGroupSystem )
: m_bInit( false )
, m_pGroupSystem( pGroupSystem )
, m_EntityId( 0 )
, m_pGroup( NULL )
, m_bLeader( false )
, m_pLeader( NULL )
, m_vCurPoint( 0.f, 0.f, 0.f )
, m_vLastLeaderPos( 0.f, 0.f, 0.f )
, m_vLastGoodPoint( 0.f, 0.f, 0.f )
, m_pSteeringAgent( NULL )
, m_fTimeout( 0.f )
, m_fPerFrameDelay( 0.f )
, m_fDistanceToTriggerSteering( 0.0f )
, m_bLastFrameUsedSteering( false )
, m_vLastCheckedLeaderPos( 0, 0, 0 )
, m_bErraticMovement( false )
, m_fSpeed( 0.0f )
{
	assert( pGroupSystem != NULL );

	GetAISystem()->CreateDummyObject( m_refAiDummy, "Formation pos" );
	m_refAiDummy->SetSubType( CAIObject::STP_FORMATION );

	GetAISystem()->CreateDummyObject( m_refAiRefDummy, "Formation Reference" );
	m_refAiRefDummy->SetSubType( CAIObject::STP_FORMATION );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CGroupMember::~CGroupMember()
{
	// Member is removed from its group by CGroupSystem
	Clear();

	m_refAiDummy.Release();
	m_refAiRefDummy.Release();
}

// Description:
//   Initialization method, called also when re-using this GroupMember instance with different EntityId
//	 (it's the only way of passing EntityId inside an instance)
// Arguments:
//
// Return:
//
bool CGroupMember::Init( EntityId id )
{
	assert( id > 0 );

	Clear();

	m_EntityId = id;

	IEntity*	pEntity = gEnv->pEntitySystem->GetEntity( m_EntityId );
	IAIObject *pAIObject = ( pEntity ? pEntity->GetAI() : NULL );
	if( pAIObject != NULL )
	{
		CAIActor *pAIActor = pAIObject->CastToCAIActor();
		m_refAIActor = ( pAIActor ? GetWeakRef( pAIActor ) : NILREF );
		m_vLastGoodPoint = pAIObject->GetPos();
	}

	m_bInit = true;


	return( true );
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupMember::Update( float fDeltaTime )
{
	m_fTimeout += fDeltaTime;

	if( m_pGroup == NULL )
	{
		return;
	}

	m_pLeader = m_pGroup->GetLeader();

	if( m_bLeader == false && m_pLeader != NULL )
	{
		Vec3	vLeaderPos = m_pLeader->GetActor()->GetEntity()->GetWorldPos();

		bool	bNeedRecalc = ( m_fTimeout > m_pGroup->GetFormationSpec()->m_fPositionTimeout );	//|| !RunPerFrameQuery();
		if( bNeedRecalc == false )
		{
			Vec3	vPos = m_pLeader->GetActor()->GetEntity()->GetWorldPos();
			float fDist2 = m_vLastLeaderPos.GetDistance( vPos );
			m_fLeaderDistTraveled += m_vLastCheckedLeaderPos.GetDistance( vPos );
			m_vLastCheckedLeaderPos = vPos;
			m_bErraticMovement = ( m_fLeaderDistTraveled / fDist2 ) > 2.f;
			bNeedRecalc = fDist2 > ( DIST_THRESHOLD );
		}
		else
		{
			m_bErraticMovement = false;
		}

		bNeedRecalc = bNeedRecalc || ( m_fLeaderDistTraveled > 0.f && m_pLeader->GetActor()->GetVelocity().GetLength2D() <= 0.f );

		if( m_bErraticMovement == false && bNeedRecalc == false && RunPerFrameQuery() == false )
		{
			m_fPerFrameDelay += fDeltaTime;
			bNeedRecalc = ( m_fPerFrameDelay > m_pGroup->GetFormationSpec()->m_fPerFrameTestFailureDelay );
		}

		if( bNeedRecalc == true )
		{
			m_fTimeout = m_fPerFrameDelay = 0.f;
			if( m_fLeaderDistTraveled > 0.f )
				FindProperPosition();
		}
		else if( m_pGroup->GetFormationSpec()->m_bFloatingFormationPos && !m_bErraticMovement )
		{
			Vec3	point = m_vCurPoint + vLeaderPos;
			if( gAIEnv.pNavigation->IsPointInForbiddenRegion(point) == false )
			{
				m_vLastGoodPoint = point;
			}
		}

		Vec3 vActorPos = GetActor()->GetEntity()->GetWorldPos();
		float fDist2 = min(m_vLastGoodPoint.GetSquaredDistance2D( vLeaderPos ), vActorPos.GetSquaredDistance2D( vLeaderPos ));
		fDist2 = min(fDist2, vActorPos.GetSquaredDistance2D( m_vLastGoodPoint ));

		//ICVar *pSpeed=gEnv->pConsole->GetCVar("cl_CompanionMoveRate");
		//pSpeed->Set(1.0f);

		if( m_pSteeringAgent != NULL && m_pSteeringAgent->IsEnabled() == true && fDist2 < m_fDistanceToTriggerSteering )
		{
			if( m_bLastFrameUsedSteering == false )
			{
				m_bLastFrameUsedSteering = true;
				m_pSteeringAgent->SetPos( m_vLastGoodPoint );
				GetActor()->CastToCPuppet()->SetSpeedFalloffRadius(0.0f, 0.0f, 0.0f);
			}

			m_pSteeringAgent->SetLeaderPosition( vLeaderPos );
			m_pSteeringAgent->SetLeaderOrientation( m_pLeader->GetActor()->GetViewDir() );
			m_pSteeringAgent->SetDestination( m_vLastGoodPoint );
			m_pSteeringAgent->Update( fDeltaTime );
			m_refAiDummy.GetAIObject()->SetPos( m_pSteeringAgent->GetPos() );

			if( /*pSpeed != NULL &&*/ (m_pSteeringAgent->GetPos()-m_vLastGoodPoint).GetLength2D() > 0.5f )
			{
				//pSpeed->Set(1.9f);
				ForceFormationPosRegeneration();
			}
			/*
			else
			{
				pSpeed->Set(1.2f);
			}*/
		}
		else
		{
			if( m_bLastFrameUsedSteering == true )
			{
				m_bLastFrameUsedSteering = false;
				GetActor()->CastToCPuppet()->SetSpeedFalloffRadius(2.0f, 5.0f, 10.0f);
			}
			m_refAiDummy.GetAIObject()->SetPos( m_vLastGoodPoint );
		}		
	}
	//else if( m_bLeader == true )
	//{ // this is group leader's update
	//	if( bNeedRecalc == false )
	//	{
	//		Vec3	vPos = m_pLeader->GetActor()->GetEntity()->GetWorldPos();
	//		float fDist2 = m_vLastLeaderPos.GetDistance( vPos );
	//		m_fLeaderDistTraveled += m_vLastCheckedLeaderPos.GetDistance( vPos );
	//		m_vLastCheckedLeaderPos = vPos;
	//		m_bErraticMovement = ( m_fLeaderDistTraveled / fDist2 ) > 2.f;
	//		bNeedRecalc = fDist2 > ( DIST_THRESHOLD );
	//	}
	//	else
	//	{
	//		m_bErraticMovement = false;
	//	}
	//}
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupMember::Clear()
{
	m_EntityId = 0;
	m_bInit = false;
	m_refAIActor.Reset();

	SAFE_DELETE( m_pSteeringAgent );
}

// Description:
//   Serializes most of instances data. Rest (like leader, group information) is serialized by CEntityGroup
// Arguments:
//
// Return:
//
void CGroupMember::Serialize( TSerialize ser )
{

}


// Description:
//   
// Arguments:
//
// Return:
//
void CGroupMember::SetGroup( CEntityGroup* pGroup )
{
	assert( m_bInit == true );
	m_pGroup = pGroup;

	if( pGroup != NULL ) 
	{
		// get leader and save its position
		m_pLeader = m_pGroup->GetLeader();

		if( m_pLeader != NULL )
		{
			SetLastLeaderPos( m_pLeader->GetActor()->GetEntity()->GetWorldPos() );
		}
		else
		{
			SetLastLeaderPos( Vec3(0,0,0) );
		}

		// set position of this instance
		const Vec3& pos = m_refAIActor.GetAIObject()->GetPos();
		m_vCurPoint =  pos - m_vLastLeaderPos;
		m_vLastGoodPoint = pos;

		// update steering vehicle position
		if( m_pSteeringAgent != NULL )
		{
			m_pSteeringAgent->SetPos( m_vLastGoodPoint );
		}
	}
}

// Description:
//   
// Arguments:
//
// Return:
//
void CGroupMember::DebugDraw() const
{
	if( GetActor() != NULL && m_pGroup != NULL )
	{
		if( m_pSteeringAgent != NULL && m_pSteeringAgent->IsEnabled() == true )
		{
			m_pSteeringAgent->DebugDraw();
		}

		CDebugDrawContext dc;

		Vec3	v3Pos = GetActor()->GetPhysicsPos();
		v3Pos.z += 1.0f;

		if( m_bLeader == true )
		{
			dc->DrawSphere( v3Pos, 0.3f, ColorB(255, 255, 0) );
			dc->Draw3dLabel( v3Pos, 1.0f, "Leader" );
		}
		else
		{
			dc->DrawSphere( v3Pos, 0.3f, ColorB(255, 0, 255) );
			dc->Draw3dLabel( v3Pos, 1.0f, "Group member" );

			Vec3						v3LeaderPos;
			CGroupMember*		pLeader = this->m_pGroup->GetLeader();
			if( pLeader && pLeader->GetActor() )
			{
				v3LeaderPos = pLeader->GetActor()->GetPhysicsPos();
			}
			else
			{
				v3LeaderPos = m_vLastLeaderPos;
			}

			v3LeaderPos.z += 1.0f;

//			Vec3	vFormationPos = m_vLastGoodPoint;
			//vFormationPos.z += 1.0f;
			dc->DrawLine(
					v3Pos,
					ColorB(255, 0, 255),
					m_vLastGoodPoint,
					ColorB(77, 255, 77),
					2.0f );

			dc->DrawSphere( m_vLastGoodPoint, 0.3f, ColorF(0.3f, 1.f, 0.3f, 1.f) );
			dc->Draw3dLabel( m_vLastGoodPoint, 1.0f, "Formation pos of %s", GetActor()->GetName() );

			dc->DrawLine(
					m_vLastGoodPoint,
					ColorB(77, 255, 77),
					v3LeaderPos,
					ColorB(255, 255, 0),
					2.0f );
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupMember::ForceFormationPosRegeneration()
{
	m_pLeader = m_pGroup->GetLeader();
	m_fTimeout = m_fPerFrameDelay = 0.f;
	SetLastLeaderPos( m_pLeader->GetActor()->GetEntity()->GetWorldPos() );

	CalculateNewFormationPos();
}

void CGroupMember::OnFormationChanged()
{
	ForceFormationPosRegeneration();
}

// Description:
//   Calculates a point where this team member should be placed
// Arguments:
//
// Return:
//
Vec3 CGroupMember::FindProperPosition()
{
	if( m_pGroup->GetFormationSpec()->m_psComputePosQuery == NULL )
	{
		//return m_vLastGoodPoint;
	}

	SetLastLeaderPos( m_pLeader->GetActor()->GetEntity()->GetWorldPos() );

	// check if currently stored point is any good
	if( TestCurrentFormationPoint() == false )
	{
		// if it's not, check if current standing position will do
		if( TestCurrentPosition() == true )
		{
			// if so - update formation point to be at current spot
			// - will this work ok with movement mechanics?
			const Vec3& pos = m_refAIActor.GetAIObject()->GetPos();
			m_vCurPoint =  pos - m_vLastLeaderPos;
			m_vLastGoodPoint = pos;

			if( m_pSteeringAgent != NULL )
			{
				m_pSteeringAgent->SetPos( m_vLastGoodPoint );
			}
		}
		else
		{
			// else - calculate new point
			CalculateNewFormationPos();
		}
	}

	return( m_vLastGoodPoint );
}

// Description:
//   Creates a formation dummy
// Arguments:
//
// Return:
//
bool CGroupMember::TestCurrentPosition()				//const
{
	int		queryID = m_pGroup->GetFormationSpec()->m_nTestCurMemberPosQuery;
	bool	bFoundValid = false;

	if( queryID )
	{
		// Do query
		Vec3										point = m_vCurPoint + m_pLeader->GetActor()->GetEntity()->GetWorldPos();
		ITacticalPointSystem*		pTPS = gAIEnv.pTacticalPointSystem;

		QueryContext context;
		InitQueryContextFromPuppet(GetActor()->CastToCPuppet(), context);

		
		// run this query
		//int nOptionUsed = pTPS->Query(queryID, context, point, bFoundValid);
		assert(false);
		// (MATT) TODO Not yet supported in new interface {2009/11/22}
	}

	return( bFoundValid );
}

// Description:
//
// Arguments:
//
// Return:
//	true if currently stored formation position is still valid
bool CGroupMember::TestCurrentFormationPoint()	// const
{
	int queryID = m_pGroup->GetFormationSpec()->m_nTestComputedPosQuery;
	return( TestCurPointWithQuery(queryID) );
}

// Description:
//
// Arguments:
//
// Return:
//	
bool CGroupMember::RunPerFrameQuery()
{
	int queryID = m_pGroup->GetFormationSpec()->m_nTestPerFrameQuery;
	return( TestCurPointWithQuery(queryID) );
}

// Description:
//
// Arguments:
//
// Return:
//	
bool CGroupMember::TestCurPointWithQuery( int queryID )
{
	bool	bFoundValid = false;

	if( queryID )
	{
		// Do query
		Vec3										point = m_vCurPoint + m_pLeader->GetActor()->GetEntity()->GetWorldPos();
		ITacticalPointSystem*		pTPS = gAIEnv.pTacticalPointSystem;

		QueryContext context;
		InitQueryContextFromPuppet(GetActor()->CastToCPuppet(), context);

		//int nOptionUsed = pTPS->Query(queryID, context, point, bFoundValid);
		assert(false);
		// (MATT) TODO Not yet supported in new interface {2009/11/22}
	}

	return( queryID == 0 || bFoundValid );
}

// Description:
//
// Arguments:
//
// Return:
//	
Vec3 CGroupMember::CalculateNewFormationPos()
{
	int queryID = m_pGroup->GetFormationSpec()->m_nComputePosQuery;

	if( queryID != 0 && GetActor() != NULL && GetActor()->CastToCPuppet() != NULL &&
			GetActor()->CastToCPuppet()->CastToIPuppet() )
	{
		// Do query
		CTacticalPoint					point;		
		CTacticalPointSystem *pTPS = gAIEnv.pTacticalPointSystem;

		// run this query
		QueryContext context;
		InitQueryContextFromPuppet(GetActor()->CastToCPuppet(), context);

		int	nOptionUsed = pTPS->SyncQuery(queryID,context,point);

		// and if any point found
		if( point.IsValid() )
		{
			m_vLastGoodPoint = point.GetPos();
			m_vCurPoint = m_vLastGoodPoint - m_vLastLeaderPos;
			if( m_pGroup->GetFormationSpec()->m_bFloatingFormationPos == false && point.GetType() == CTacticalPoint::eTPT_HideSpot )
			{				
				GetActor()->CastToCPipeUser()->m_CurrentHideObject.Invalidate();
				GetActor()->CastToCPipeUser()->m_CurrentHideObject.Set(point.GetHidespot(), point.GetPos(), point.GetHidespot()->info.dir);				
			}

			IAISignalExtraData *pData = GetAISystem()->CreateSignalExtraData();
			pData->point = point.GetPos();
			GetAISystem()->SendSignal( SIGNALFILTER_SENDER, 1, "OnFormationPosChanged", GetActor(), pData );
		}
	}

	return( m_vLastGoodPoint );
}

// Description:
//
// Arguments:
//
// Return:
//	
void CGroupMember::UseSteering( bool bUseSteering, float fDistanceToTriggerSteering )
{
#ifdef DISABLE_GROUP_STEERING
	if( true )
		return;
#endif // DISABLE_GROUP_STEERING

	m_fDistanceToTriggerSteering = fDistanceToTriggerSteering;

	if( bUseSteering != IsUsingSteering() )
	{
		if( bUseSteering == true )
		{
			assert( m_pSteeringAgent == NULL );
			m_pSteeringAgent = new CSteeringAgent();
			m_pSteeringAgent->Init( this, GetActor()->GetPhysicsPos(), 0.3f );
			m_pSteeringAgent->Enable();
		}
		else
		{
			assert( m_pSteeringAgent != NULL );
			m_pSteeringAgent->Disable();
		}
	}
}
