/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   EntityGroup.cpp
Description: 
---------------------------------------------------------------------
History:
- 11:02:2008 : Created by mieszko
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include "EntityGroup.h"
#include "GroupMember.h"
#include "GroupSystem.h"
#include "FormationDataManager.h"
#include "../TacticalPointSystem/TacticalPointSystem.h"
#include "DebugDrawContext.h"
#include "GameUtils.h"

/*$1- SFormationSpec methods implementation ----------------------------------*/

// Description:
//
// Arguments:
//
// Return:
//
void SFormationSpec::CalculateQueryIds()
{
	ITacticalPointSystem*		pTPS = gAIEnv.pTacticalPointSystem;

	m_nComputePosQuery = pTPS->GetQueryID( m_psComputePosQuery );
	m_nTestComputedPosQuery = pTPS->GetQueryID( m_psTestComputedPosQuery );
	m_nTestCurMemberPosQuery = pTPS->GetQueryID( m_psTestCurMemberPosQuery );
	m_nTestPerFrameQuery = pTPS->GetQueryID( m_psTestPerFrameQuery );
}

// Description:
//
// Arguments:
//
// Return:
//
void SFormationSpec::Serialize( TSerialize ser )
{
	ser.Value( "m_fPositionTimeout", m_fPositionTimeout );
	ser.Value( "m_fPerFrameTestFailureDelay", m_fPerFrameTestFailureDelay );
	ser.Value( "m_bFloatingFormationPos", m_bFloatingFormationPos );
	ser.Value( "m_bUseBestPoint", m_bUseBestPoint );

	// TODO: serialize strings:
	/*if( ser.IsReading() )
	{
		ser.Value( "m_psComputePosQuery", m_psComputePosQuery );
		ser.Value( "m_psTestComputedPosQuery", m_psTestComputedPosQuery );
		ser.Value( "m_psTestCurMemberPosQuery", m_psTestCurMemberPosQuery );
		ser.Value( "m_psTestPerFrameQuery", m_psTestPerFrameQuery );

		CalculateQueryIds();
	}
	else
	{
		ser.WriteStringValue( "m_psComputePosQuery", m_psComputePosQuery );
		ser.WriteStringValue( "m_psTestComputedPosQuery", m_psTestComputedPosQuery );
		ser.WriteStringValue( "m_psTestCurMemberPosQuery", m_psTestCurMemberPosQuery );
		ser.WriteStringValue( "m_psTestPerFrameQuery", m_psTestPerFrameQuery );
	}		*/
}

/*$1- CEntityGroup methods implementation ------------------------------------*/

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CEntityGroup::CEntityGroup()
: m_bEmpty(true)
, m_bFormationChanged(false)
, m_nId(0)
, m_pLeader(NULL)
, m_pFormation(NULL)
, m_vCurrOrientation(Vec3Constants<float>::fVec3_Zero)
{

}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CEntityGroup::~CEntityGroup()
{
	Destroy();
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::Update( float fDeltaTime )
{
	if (m_bFormationChanged)
	{
		m_bFormationChanged = false;
		ApplyFormationChange();
	}
	else
	{
		RecalcPoints(fDeltaTime);
	}

	// Base urgency is leader's urgency if set to match leader's speed
	bool bMatchLeader = false;
	float fBaseUrgency = 0.0f;
	if ( m_formationSpec.m_bMatchLeaderSpeed && m_pLeader )
	{
		SOBJECTSTATE *pLeaderState = m_pLeader->GetActor()->GetState();
		CRY_ASSERT(pLeaderState);
		fBaseUrgency = pLeaderState->fMovementUrgency;
		bMatchLeader = true;
	}

	// Set movement urgency override
	TMemberContainer::iterator itMember = m_vecMembers.begin();
	TMemberContainer::iterator itMemberEnd = m_vecMembers.end();
	for (; itMember != itMemberEnd; ++itMember)
	{
		CGroupMember *pMember = *itMember;
		CRY_ASSERT(pMember);
		if (pMember->IsLeader())
			continue;
		
		SOBJECTSTATE *pMemberState = pMember->GetActor()->GetState();
		CRY_ASSERT(pMemberState);
		pMemberState->fMovementUrgency = (bMatchLeader ? fBaseUrgency : pMemberState->fMovementUrgency) + pMember->GetFormationSpeed();
	}

	TMemberContainer::iterator itC = m_vecMembers.begin();
	TMemberContainer::iterator itEnd = m_vecMembers.end();
	while( itC != itEnd )
	{
		( *itC )->Update( fDeltaTime );

		++itC;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::ApplyFormationChange()
{
	RecalcPoints(0.0f, true);

	TMemberContainer::iterator itC = m_vecMembers.begin();
	TMemberContainer::iterator itEnd = m_vecMembers.end();
	while( itC != itEnd )
	{
		( *itC )->OnFormationChanged();

		++itC;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::Destroy()
{
	m_pLeader = NULL;

	TMemberContainer::iterator itC = m_vecMembers.begin();
	TMemberContainer::iterator itEnd = m_vecMembers.end();
	while( itC != itEnd )
	{
		( *itC )->SetGroup( NULL );

		++itC;
	}

	m_vecMembers.clear();

	m_bEmpty = true;
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::Serialize( TSerialize ser )
{
	ser.Value( "m_nId", m_nId );
	m_formationSpec.Serialize( ser );

	int nSize = m_vecMembers.size();
	ser.Value( "nSize", nSize );

	if( nSize > 0 )
	{
		if( ser.IsWriting() )
		{
			EntityId nMemberId = m_pLeader->GetEntityId();
			ser.Value( "leaderId", nMemberId );

			TMemberContainer::iterator itC = m_vecMembers.begin();
			TMemberContainer::iterator itEnd = m_vecMembers.end();
			for( ; itC != itEnd; ++itC )
			{
				nMemberId = ( *itC )->GetEntityId();
				ser.Value( "memberId", nMemberId );
			}
		}
		else
		{
			EntityId nMemberId = 0;
			EntityId nLeaderId = 0;
			CGroupMember*		pMember = NULL;
			CGroupSystem*		pGroupSystem = gAIEnv.pGroupSystem;

			Destroy();

			ser.Value( "leaderId", nLeaderId );

			for( int i = 0; i < nSize; ++i )
			{
				ser.Value( "memberId", nMemberId );
				pMember = pGroupSystem->GetGroupMember( nMemberId );
				m_vecMembers.push_back( pMember );
			}

			pMember = pGroupSystem->GetGroupMember( nLeaderId );
			SetLeader( pMember );

			m_bEmpty = false;
		}
	}
	else
	{
		m_bEmpty = true;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::AddMember( CGroupMember* pNewMember )
{
	if( pNewMember->GetGroup() != this )
	{
		m_vecMembers.push_back( pNewMember );
		pNewMember->SetGroup( this );

		if( m_vecMembers.size() == 1 )
		{
			SetLeader( pNewMember );
		}
	}

	UpdateFormation();

	m_bEmpty = false;
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::OnMemberRemoved( CGroupMember* pMember )
{
	assert(pMember);

	const int nMemberSize = (int)m_vecMembers.size();

	pMember->SetGroup( NULL );

	if( pMember->IsLeader() )
	{
		pMember->SetIsLeader( false );
		if( nMemberSize > 0 )
		{
			SetLeader( *(m_vecMembers.begin()) );
		}
		else
		{
			SetLeader( NULL );
			m_bEmpty = true;
		}
	}

	if( nMemberSize > 0 )
	{
		UpdateFormation();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::RemoveMember( CGroupMember* pMember )
{
	assert(pMember);

	if( pMember && pMember->GetGroup() == this )
	{
		TMemberContainer::iterator itC = m_vecMembers.begin();
		TMemberContainer::iterator itEnd = m_vecMembers.end();
		while( itC != itEnd )
		{
			if( *itC == pMember )
			{
				m_vecMembers.erase(itC);
				OnMemberRemoved(pMember);
				break;
			}

			++itC;
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::OnObjectRemoved( CAIObject *pObject )
{
	assert(pObject);

	CAIActor *pObjectActor = pObject->CastToCAIActor();

	TMemberContainer::iterator itMember = m_vecMembers.begin();
	TMemberContainer::iterator itMemberEnd = m_vecMembers.end();
	for (; itMember != itMemberEnd; )
	{
		CGroupMember *pMember = (*itMember);
		assert(pMember);

		// Remove the member if the actor is being removed
		CAIActor *pActor = pMember->GetActor();
		if (pObjectActor == pActor)
		{
			// Delete the member
			m_vecMembers.erase(itMember);
			OnMemberRemoved(pMember);
			break;
		}

		++itMember;
	}
}

// Description:
//		Sets a new group's leader
// Arguments:
//
// Return:
//	false - if given CGroupMember instance isn't in this group
//
bool CEntityGroup::SetLeader( CGroupMember* pNewLeader )
{
	if( pNewLeader == NULL || pNewLeader->GetGroup() == this )
	{
		if( m_pLeader )
		{
			m_pLeader->SetIsLeader( false );
		}

		m_pLeader = pNewLeader;

		if( pNewLeader )
		{
			m_pLeader->SetIsLeader( true );
		}

		m_vCurrOrientation = Vec3Constants<float>::fVec3_Zero;

		return( true );
	}
	else
	{
		return( false );
	}
}

// Description:
//		
// Arguments:
//
// Return:
//
void CEntityGroup::SetMatchLeaderSpeed( bool bMatch )
{
	m_formationSpec.m_bMatchLeaderSpeed = bMatch;
}

// Description:
//		
// Arguments:
//
// Return:
//
void CEntityGroup::SetFormationSpec( SFormationSpec& formSpec )
{
	m_formationSpec = formSpec;

	UpdateFormation();
}

// Description:
//		
// Arguments:
//
// Return:
//
void CEntityGroup::DebugDraw() const
{
	if( m_pLeader == NULL || m_formationSpec.m_psFormationSet == NULL )
	{
		return;
	}

	const CFormationData*		pFormation = CFormationDataManager::GetInstance()->GetFormationData(
			m_formationSpec.m_psFormationSet,
			m_vecMembers.size() );

	if( pFormation == NULL )
	{
		return;
	}

	CDebugDrawContext dc;

	CAIActor *pLeader = m_pLeader->GetActor();
	IEntity *pLeaderEntity = pLeader ? pLeader->GetEntity() : NULL;
	const Vec3& vLeaderWorldPos = pLeaderEntity ? pLeaderEntity->GetWorldPos() : Vec3Constants<float>::fVec3_Zero;

	const int nPointSize = (int)m_vecPoints.size();
	TOrderContainer::const_iterator itOrder = m_vecOrder.begin();
	TOrderContainer::const_iterator itOrderEnd = m_vecOrder.end();
	TMemberContainer::const_iterator itMember = m_vecMembers.begin();
	TMemberContainer::const_iterator itMemberEnd = m_vecMembers.end();
	for (int nMemberNum = 0; itMember != itMemberEnd && itOrder != itOrderEnd; ++itMember, ++nMemberNum)
	{
		CGroupMember *pMember = *itMember;
		assert(pMember);

		// Skip the leader
		if (pMember == m_pLeader)
			continue;

		const int nOrderLoc = *itOrder;
		const bool bValidOrder = (nOrderLoc >= 0 && nOrderLoc < nPointSize);
		assert(bValidOrder);
		if (bValidOrder)
		{
			const Vec3& vPoint = m_vecPoints[nOrderLoc];
			
			Vec3 vPos = vLeaderWorldPos + vPoint;
			Vec3 vRefPos = pMember->GetFormationRef();

			dc->Draw3dLabel( vPos, 1.0f, "Position %d occupied by %s (member %d)", nOrderLoc, pMember->GetActor()->GetName(), nMemberNum);

			if( vRefPos.GetSquaredDistance2D( vPos ) > 1.0f )
			{ 
				// something isn't right
				dc->DrawSphere( vRefPos, 3.0f, ColorF(1.f, 0.05f, 0.1f, 0.4f) );		
				dc->DrawSphere( vPos, 3.0f, ColorF(0.8f, 0.75f, 0.1f, 0.4f) );		
			}
			else
			{
				dc->DrawSphere( vPos, 3.0f, ColorF(0.f, 0.75f, 0.1f, 0.4f) );		
			}
		}

		// Advance iterators
		++itOrder;
	}
}

// Description:
//   Recalculates the formation companion position
// Arguments:
//
// Return:
//
Vec3 CEntityGroup::GetOffsetPoint( const Vec3& vRelPos ) const
{
	// Calc rot and pos from the leader
	const Vec3& vUp( Vec3Constants<float>::fVec3_OneZ );
	const Vec3& vLook = m_vCurrOrientation;
	const Vec3 vRight( vLook ^ vUp );

	Matrix34 Mat;
	Mat.SetColumn( 0, vRight );
	Mat.SetColumn( 1, vLook );
	Mat.SetColumn( 2, vRight ^ vLook );
	Mat.SetTranslation( Vec3Constants<float>::fVec3_Zero );

	// Transform the relative point
	Vec3 vRes = Mat.TransformPoint( vRelPos );

	return( vRes );
}

// Description:
//
// Arguments:
//
// Return:
//
void CEntityGroup::UpdateFormation()
{
	if( m_formationSpec.m_psFormationSet == NULL )
	{
		return;
	}

	m_pFormation = CFormationDataManager::GetInstance()->GetFormationData(
			m_formationSpec.m_psFormationSet,
			m_vecMembers.size() );
	m_vCurrOrientation = Vec3Constants<float>::fVec3_Zero;

	if( m_pFormation == NULL )
	{
		return;
	}

	m_bFormationChanged = true;
}

// Description:
//   Recalculates the formation points (relative to formation's leader)
// Arguments:
//
// Return:
//
void CEntityGroup::RecalcPoints( float fDeltaTime, bool bForceBestUse /*= false*/ )
{
	if( m_pFormation == NULL )
	{
		return;
	}

	CAIActor* pLeader = m_pLeader->GetActor();
	assert(pLeader);
	const Vec3& vLeaderPos = pLeader->GetEntity()->GetWorldPos();
	const Vec3& vLeaderDir = pLeader->GetMoveDir().GetNormalized();

	// if formation's move direction haven't changed, just update ref points' positions
	if( m_vCurrOrientation.Dot(vLeaderDir) >= 1.0f )
	{
		UpdateRefPoints( pLeader );
		return;
	}

	// Interpolate into new orientation
	if (fDeltaTime > FLT_EPSILON)
	{
		Interpolate(m_vCurrOrientation, vLeaderDir, 0.5f, fDeltaTime);
		m_vCurrOrientation.normalize();
	}
	else if (m_vCurrOrientation.IsZero())
	{
		m_vCurrOrientation = vLeaderDir;
	}

	const TPointContainer& points = m_pFormation->GetPoints();
	const int nNotLeaderMembers = (int)m_vecMembers.size() - 1;
	assert( points.size() == nNotLeaderMembers );

	TPointContainer vecTempPoints;
	vecTempPoints.reserve(nNotLeaderMembers);

	TDistanceVector vecTempDistances;
	vecTempDistances.reserve(nNotLeaderMembers * nNotLeaderMembers);

	// Clean out old points
	m_vecPoints.clear();
	m_vecPoints.reserve(nNotLeaderMembers);

	TPointContainer::const_iterator itPoint = points.begin();
	TPointContainer::const_iterator itPointEnd = points.end();
	TMemberContainer::const_iterator itMember = m_vecMembers.begin();
	TMemberContainer::const_iterator itMemberEnd = m_vecMembers.end();
	for (; itMember != itMemberEnd && itPoint != itPointEnd; ++itMember)
	{
		CGroupMember *pMember = *itMember;
		assert(pMember);

		// Skip the leader
		if (pMember == m_pLeader)
			continue;

		// Translate the formation point to world space, but relative to leader position
		Vec3 vPoint = GetOffsetPoint(*itPoint);
		m_vecPoints.push_back(vPoint);

		// Fill arrays for handier access
		vecTempPoints.push_back(vLeaderPos + vPoint);

		// Advance iterators
		++itPoint;
	}

	// Calculate and cache distances to points
	itMember = m_vecMembers.begin();
	itMemberEnd = m_vecMembers.end();
	for (int i = 0; itMember != itMemberEnd && i < nNotLeaderMembers; ++i, ++itMember)
	{
		CGroupMember *pMember = *itMember;
		assert(pMember);

		CAIActor *pActor = pMember->GetActor();
		IEntity *pEntity = pActor ? pActor->GetEntity() : NULL;
		assert(pEntity);

		itPoint = vecTempPoints.begin();
		itPointEnd = vecTempPoints.end();
		for (int j = 0; itPoint != itPointEnd && j < nNotLeaderMembers; ++j, ++itPoint)
		{
			const Vec3 &vPoint = *itPoint;
			const float fDistance = pEntity ? pEntity->GetWorldPos().GetDistance(vPoint) : FLT_MAX;

			vecTempDistances.push_back(fDistance);
		}
	}

	if (nNotLeaderMembers == 1)
	{
		m_vecOrder.clear();
		m_vecOrder.push_back(0);
	}
	else if (bForceBestUse || m_formationSpec.m_bUseBestPoint)
	{
		RecalcOrderList(nNotLeaderMembers, vecTempDistances);
	}

	UpdateRefPoints( pLeader );
}

// Description:
//   Recalculates the order list for which memebers use which formation offsets
// Arguments:
//
// Return:
//
void CEntityGroup::RecalcOrderList( int nNotLeaderMembers, TDistanceVector& vecDistances )
{
	m_vecOrder.clear();
	m_vecOrder.resize(nNotLeaderMembers);

	std::vector<TOrderContainer> vecMemberOrders;
	vecMemberOrders.resize(nNotLeaderMembers);

	int nRandom = 0;
	int nBestIndex = -1;
	float fBestScore = 100000000.f;
	
	for( int z = 0; z < nNotLeaderMembers; ++z )
	{
		TOrderContainer &vecOrder = vecMemberOrders[z];
		vecOrder.resize(nNotLeaderMembers);
		const float fScore = GetBestStartingFrom( z, nNotLeaderMembers, vecDistances, vecOrder );

#ifdef _DEBUG
		char buf[255] = {" \n"};
		for( int i = 0; i < nNotLeaderMembers; i++ )
		{
			sprintf( buf, "%s, %d", buf, vecMemberOrders[z][i] );				
		}
		sprintf( buf, "%s, score: %f", buf, fScore );			
#endif // _DEBUG

		if( fScore < fBestScore )
		{
			fBestScore = fScore;
			nBestIndex = z;
		}
	}

	assert( nBestIndex >= 0 && "Unable to find best member-formation point matching!" );
	if (nBestIndex >= 0 && nBestIndex < nNotLeaderMembers)
	{
		m_vecOrder.insert( m_vecOrder.begin(), vecMemberOrders[nBestIndex].begin(), vecMemberOrders[nBestIndex].end() );
	}
}

// Description:
//   Updates group members' reference point positions
// Arguments:
//
// Return:
//
void CEntityGroup::UpdateRefPoints( CAIActor* pLeader )
{
	assert(pLeader);
	if (!pLeader || m_vecOrder.empty())
		return;

	IEntity *pLeaderEntity = pLeader->GetEntity();
	const Vec3& vLeaderWorldPos = pLeaderEntity ? pLeaderEntity->GetWorldPos() : Vec3Constants<float>::fVec3_Zero;

	const int nPointSize = (int)m_vecPoints.size();
	TOrderContainer::const_iterator itOrder = m_vecOrder.begin();
	TOrderContainer::const_iterator itOrderEnd = m_vecOrder.end();
	TMemberContainer::const_iterator itMember = m_vecMembers.begin();
	TMemberContainer::const_iterator itMemberEnd = m_vecMembers.end();
	for (; itMember != itMemberEnd && itOrder != itOrderEnd; ++itMember)
	{
		CGroupMember *pMember = *itMember;
		assert(pMember);

		// Skip the leader
		if (pMember == m_pLeader)
			continue;

		const int nOrderLoc = *itOrder;
		const bool bValidOrder = (nOrderLoc >= 0 && nOrderLoc < nPointSize);
		assert(bValidOrder);
		if (bValidOrder)
		{
			const Vec3& vPoint = m_vecPoints[nOrderLoc];
			pMember->SetFormationRef(vLeaderWorldPos + vPoint);
		}

		// Advance iterators
		++itOrder;
	}
}

// Description:
//   Gets the score of the member
// Arguments:
//
// Return
//
float CEntityGroup::GetBestStartingFrom( int nMember, int nNotLeaderMembers, TDistanceVector& vecDistances, TOrderContainer& vecOrder ) const
{
	float fScore = 0.f;
	const int nVecDistancesSize = (int)vecDistances.size();
	static std::vector<bool> vecTaken;
	vecTaken.clear();
	vecTaken.resize( nNotLeaderMembers );

	for( int i = 0; i < nNotLeaderMembers; ++i )
	{
		int nThisMember = (i+nMember) % nNotLeaderMembers;
		int nStart = nThisMember * nNotLeaderMembers;
		float fMin = 1000000000.f;
		int nIndex = -1;

		// for every point
		for( int k = 0; k < nNotLeaderMembers; ++k )
		{
			if( vecTaken[k] )
			{
				continue;
			}

			int nDistanceOffset = k + nStart;
			if( nDistanceOffset < nVecDistancesSize && vecDistances[nDistanceOffset] < fMin )
			{
				fMin = vecDistances[nDistanceOffset];
				if( nIndex >= 0 )
					vecTaken[ nIndex ] = false;
				nIndex = k;
				vecTaken[ k ] = true;
			}
		}

		assert( nIndex >= 0 );
		vecOrder[nThisMember] = nIndex;		
		fScore += fMin;
	}		

	return fScore;
}
