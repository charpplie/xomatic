/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   SteeringAgent.cpp
Description: 
---------------------------------------------------------------------
History:
- 11:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include "SteeringAgent.h"
#include "GroupMember.h"
#include "DebugDrawContext.h"

#define STEER_COLLISION_POINTS	4

//====================================================================
// PointInEllipse (helper func, move to a proper place)
//====================================================================
bool IsPointInsideEllipse( const Vec3 point, const Vec3& ellipse_pos, float radx, float rady, float orientation )
{
	// Quick discard ---
	float major = max( radx, rady );
	if( ellipse_pos.GetSquaredDistance2D(point) > (major * major) )
	{
		return( false );
	}

	// More specific discard ---
	float minor = min( radx, rady );
	float e = sqrtf( (1.0f - ((minor * minor) / (major * major))) );
	float d = major * e;
	Vec3	foci = Vec3( d * cosf(orientation), d * sinf(orientation), 0 );
	Vec3	foci0 = ellipse_pos + foci;
	Vec3	foci1 = ellipse_pos + foci.Flip();

	if( foci0.GetDistance(point) + foci1.GetDistance(point) > (major * 2) )
	{
		return( false );
	}

	return( true );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSteeringAgent::CSteeringAgent() : m_bInit( false ), m_bEnabled( false ), m_vSpeed( ZERO ), m_vLeaderPos( ZERO ), m_vLastLeaderPos( ZERO )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CSteeringAgent::~CSteeringAgent()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSteeringAgent::Init( CGroupMember* pGroupMember, const Vec3& vStartingPos, float fRadius )
{
	assert( fRadius >= 0.0f );
	assert( pGroupMember != NULL );

	m_vPos = vStartingPos;
	m_fRadius = fRadius;
	m_bInit = true;
	m_fMaxSpeed = 2.8f;
	m_fMaxColSpeed = 3.8f;
	m_fMinSpeed = 0.05f;
	m_fBreaks = 0.09f;
	m_fMaxAccel = 0.03f;
	m_fLeaderPredictionTimeout = 0.0f;
	m_vecCollisionForces.resize( STEER_COLLISION_POINTS );
	m_pGroupMember = pGroupMember;

	return( m_bInit );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSteeringAgent::Disable()
{
	assert( m_bInit == true );
	m_bEnabled = false;
}

// Description:
//
// Arguments:
//
// Return:
//
void CSteeringAgent::Enable()
{
	assert( m_bInit == true );
	m_bEnabled = true;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSteeringAgent::IsEnabled() const
{
	assert( m_bInit == true );
	return( m_bEnabled );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSteeringAgent::SetLeaderPosition( const Vec3& vLeaderPosition )
{
	if( m_vLastLeaderPos == Vec3(0, 0, 0) )
	{
		m_vLastLeaderPos = m_vLeaderPos;
	}
	else
	{
		m_vLastLeaderPos = m_vLeaderPos;
	}

	m_vLeaderPos = vLeaderPosition;
}

// Description:
//
// Arguments:
//
// Return:
//
void CSteeringAgent::SetDestination( const Vec3& vDestination )
{
	m_vDestinationPos = vDestination;
}

// Description:
//
// Arguments:
//
// Return:
//
void CSteeringAgent::Update( float fDeltaTime )
{
	Vec3	vNewSpeed = m_vSpeed + CalcForcesAtPos( m_vPos, false );
	vNewSpeed = FilterSpeed( vNewSpeed );
	vNewSpeed += CalcForcesAtPos( m_vPos, true );
	vNewSpeed = FilterSpeed( vNewSpeed, true );

	/*float fAccel = abs(vNewSpeed.GetLength() - m_vSpeed.GetLength() );
	if( fAccel > m_fMaxAccel )
	{
		vNewSpeed.normalize();
		vNewSpeed = vNewSpeed.scale( m_fMaxAccel );
		}*/
	m_vSpeed = vNewSpeed;
	m_vPos += m_vSpeed.scale( fDeltaTime );

	// Gather all forces again
	GenerateAllCollisions( fDeltaTime );
	m_vecForces = m_vecCollisionForces;

	m_vecForces.push_back(
			SForce(
				m_vLeaderPos,
				-m_fMaxColSpeed,
				min(1.2f, m_vDestinationPos.GetDistance(m_vLeaderPos) - 0.2f),
				0.0f,
				true,
				0.0f) );

	//m_vecForces.push_back( SForce(m_vLeaderPos, -0.4f, 1.0f, 0.0f, true) );
	// Add a force in the predicted position for the leader
	Vec3	vDiff = m_vLeaderPos - m_vLastLeaderPos;
	vDiff.z = 0.0f;

	float fLen = vDiff.NormalizeSafe();
	if( fLen > 0.01f )
	{
		float fOrientation = Vec2( vDiff.x, vDiff.y ).atan2();
		float fRad = min( fLen * 5.0f, m_vPos.GetDistance(m_vLeaderPos)*2.0f );
		vDiff = vDiff.scale( fRad );
		fRad += 5.0f;

		m_LeaderPredictionForce.m_vPos = vDiff + m_vLeaderPos;
		m_LeaderPredictionForce.m_fForce = -m_fMaxColSpeed;
		m_LeaderPredictionForce.m_fRadius = fRad;
		m_LeaderPredictionForce.m_fRadius2 = fRad * 0.4f;
		m_LeaderPredictionForce.m_bCollision = true;
		m_LeaderPredictionForce.m_bEllipse = true;
		m_LeaderPredictionForce.m_fRadius = fRad;
		m_LeaderPredictionForce.m_fOrientation = fOrientation;
		m_fLeaderPredictionTimeout = 2.5f;
	}

	if( m_fLeaderPredictionTimeout > 0.0f )
	{
		m_fLeaderPredictionTimeout -= fDeltaTime;
		m_vecForces.push_back( m_LeaderPredictionForce );
	}
	else
	{
		if( m_LeaderPredictionForce.m_fForce != 0.0f )
		{
			m_LeaderPredictionForce.m_fForce = 0.0f;
			m_pGroupMember->ForceFormationPosRegeneration();
		}
	}

	m_vecForces.push_back( SForce(m_vDestinationPos, m_fMaxColSpeed, 100.0f, 0.2f) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSteeringAgent::GenerateAllCollisions( float fDeltaTime )
{
	std::vector<SForce>::iterator itC = m_vecCollisionForces.begin();
	std::vector<SForce>::iterator itEnd = m_vecCollisionForces.end();
	int iPos = 0;

	while( itC != itEnd )
	{
		const SForce&		sForce = ( SForce ) * itC;
		if( m_vPos.GetSquaredDistance2D(sForce.m_vPos) > (sForce.m_fRadius * sForce.m_fRadius) )
		{
			// workaround: [15:02:08 ricardo]: OMG change this
			m_vecCollisionForces[iPos] = GenerateCollisionForce( iPos );
		}

		++itC;
		++iPos;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
SForce CSteeringAgent::GenerateCollisionForce( int iPos )
{
	// Check collisions around
	SForce sForce;
	Vec3 vColPoint = GetCollisionPoint( iPos );

	ListPositions boundary;
	bool bValid = !gAIEnv.pNavigation->IsPointInForbiddenRegion( vColPoint );

	if( bValid == true )
	{
		bValid = CheckWalkability( m_vPos, vColPoint, m_fRadius, false, boundary, AICE_ALL );
	}

	if( bValid == false )
	{
		float r = vColPoint.GetDistance( m_vPos );
		sForce.m_vPos = vColPoint;
		sForce.m_fForce = -m_fMaxColSpeed;
		sForce.m_fRadius = r + 0.1f;
		sForce.m_bCollision = true;
	}

	return( sForce );
}

// Description:
//
// Arguments:
//
// Return:
//
Vec3 CSteeringAgent::GetCollisionPoint( int iPos ) const
{
	// Check collisions around
	Vec3 vColPoint = m_vPos;
	float fdist = 0.35f;
	float frand = 0.015f;

	switch( iPos )
	{
		case 0:
			vColPoint += Vec3( fdist + frand, fdist, 0 );
			break;

		case 1:
			vColPoint += Vec3( fdist, -fdist + frand, 0 );
			break;

		case 2:
			vColPoint += Vec3( -fdist - frand, fdist, 0 );
			break;

		case 3:
			vColPoint += Vec3( -fdist + frand, -fdist, 0 );
			break;
	}

	return( vColPoint );
}

// Description:
//
// Arguments:
//
// Return:
//
Vec3 CSteeringAgent::CalcForcesAtPos( const Vec3& vPos, bool bCollisionForces )
{
	Vec3																speed( 0, 0, 0 );
	std::vector<SForce>::const_iterator itC = m_vecForces.begin();
	std::vector<SForce>::const_iterator itEnd = m_vecForces.end();

	while( itC != itEnd )
	{
		const SForce&		force = (const SForce) *itC;
		if( force.m_fForce != 0.0f && force.m_bCollision == bCollisionForces )
		{
			speed += AddForce( force, vPos );
		}

		++itC;
	}

	return( speed );
}

// Description:
//
// Arguments:
//
// Return:
//
Vec3 CSteeringAgent::FilterSpeed( const Vec3& vSpeed, bool bBreakMax ) const
{
	Vec3	speed( vSpeed );

	// Clamp speed to m_fMaxSpeed
	float len = vSpeed.GetLength();
	float fMaxSpeed = m_fMaxSpeed;
	if( bBreakMax == true )
	{
		fMaxSpeed = m_fMaxColSpeed;
	}

	if( len > fMaxSpeed )
	{
		speed.normalize();
		speed = speed.scale( fMaxSpeed );
	}

	// Apply min speed
	if( len < m_fMinSpeed )
	{
		speed.zero();
	}
	else
	{
		// or apply breaks
		Vec3	vBreaks( m_vSpeed );
		vBreaks.normalize();
		vBreaks = vBreaks.scale( m_fBreaks );
		speed -= vBreaks;
	}

	speed.z = 0.0f;

	return( speed );
}

// Description:
//
// Arguments:
//
// Return:
//
Vec3 CSteeringAgent::AddForce( const SForce& sForce, const Vec3& vPos ) const
{
	Vec3	vDir( 0, 0, 0 );

	if( sForce.IsCircleInside(vPos, m_fRadius) == true )
	{
		vDir = ( sForce.m_vPos - vPos );
		vDir.Normalize();

		float force = 0.0f;
		if( sForce.m_fForce > 0 )
		{
			force = /*( dist2 / (rad2 - inrad2) ) **/ sForce.m_fForce;
		}
		else
		{
			force = /*( 1.0f - (dist2 / (rad2 - inrad2)) ) **/ sForce.m_fForce;
			vDir.Flip();
		}

		vDir.scale( force );
	}

	vDir.z = 0;

	return( vDir );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSteeringAgent::DebugDraw() const
{
	if( m_bEnabled == false )
	{
		return;
	}

	if( m_LeaderPredictionForce.m_fForce != 0.0f )
	{
		m_LeaderPredictionForce.DebugDraw(ColorB(255, 255, 255, 128) );
	}

	CDebugDrawContext dc;
	dc->DrawCircleOutline( m_vPos, m_fRadius, ColorB(0, 0, 255, 128) );
	dc->Draw3dLabel( m_vPos, 2.0f, "%0.3f", m_vSpeed.GetLength() );
	dc->DrawArrow(
		m_vPos,
		Vec3(m_vSpeed.x * 3.0f, m_vSpeed.y * 3.0f, m_vSpeed.z * 3.0f),
		0.3f,
		ColorB(0, 0, 255, 128) );

	std::vector<SForce>::const_iterator itC = m_vecForces.begin();
	std::vector<SForce>::const_iterator itEnd = m_vecForces.end();

	while( itC != itEnd )
	{
		const SForce&		f = ( SForce ) * itC;
		if( f.m_fForce > 0.0f )
		{
			f.DebugDraw( ColorB(0, 196, 255, 128) );
		}
		else
		{
			f.DebugDraw( ColorB(255, 196, 0, 128) );
		}

		dc->Draw3dLabel( f.m_vPos, 1.0f, "%0.1f", f.m_fForce );
		++itC;
	}

	ColorB yellow(255, 255, 0);

	for( int i = 0; i < STEER_COLLISION_POINTS; ++i )
	{
		dc->DrawSphere( GetCollisionPoint(i), 0.1f, yellow );
	}

	dc->DrawSphere( m_vLeaderPos + m_vLeaderLooking, 0.1f, yellow );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SForce::DebugDraw( ColorB color ) const
{
	CDebugDrawContext dc;

	if( m_bEllipse == false )
	{
		dc->DrawCircleOutline( m_vPos, m_fRadius, color );
		if( m_fInnerRadius > 0.0f )
		{
			dc->DrawCircleOutline( m_vPos, m_fInnerRadius, color );
		}
	}
	else
	{
		dc->DrawEllipseOutline( m_vPos, m_fRadius, m_fRadius2, m_fOrientation, color );

		if( m_fInnerRadius > 0.0f )
		{
			dc->DrawCircleOutline( m_vPos, m_fInnerRadius, color );
		}
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SForce::IsCircleInside( const Vec3& point, float Radius ) const
{
	if( m_bEllipse == false )
	{
		float dist2 = point.GetSquaredDistance2D( m_vPos ) - ( Radius * Radius );
		return( dist2 < (m_fRadius * m_fRadius) && dist2 > (m_fInnerRadius * m_fInnerRadius) );
	}
	else
	{
		return( IsPointInsideEllipse(point, m_vPos, m_fRadius, m_fRadius2, m_fOrientation) );
	}
}
