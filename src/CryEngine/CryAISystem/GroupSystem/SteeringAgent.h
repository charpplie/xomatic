/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   SteeringAgent.h
Description: 
---------------------------------------------------------------------
History:
- 11:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _STEERING_AGENT_H_
	#define _STEERING_AGENT_H_

class CGroupMember;

struct SForce
{
	Vec3	m_vPos;
	float m_fForce;
	float m_fRadius;
	float m_fRadius2;
	float m_fInnerRadius;
	bool	m_bCollision;
	float m_fTimeout;
	float m_fOrientation;
	bool	m_bEllipse;

	SForce()
	:
	m_vPos( ZERO ),
	m_fForce( ZERO ),
	m_fRadius( ZERO ),
	m_fRadius2( ZERO ),
	m_fOrientation( ZERO ),
	m_fInnerRadius( ZERO ),
	m_bCollision( ZERO ),
	m_fTimeout( ZERO ),
	m_bEllipse( false )
	{
	}

	/*$off*/
	SForce ( const Vec3& vPos, float fForce, float fRadius, float fInnerRadius = 0.0f, bool bCollision = false, float fTimeout = 0.0f) :
	m_vPos( vPos ),
	m_fForce( fForce ),
	m_fRadius( fRadius ),
	m_fRadius2( fRadius ),
	m_fInnerRadius( fInnerRadius ),
	m_bCollision( bCollision ),
	m_fTimeout( bCollision ),
	m_bEllipse ( false )
	{}
	/*$on*/
	void	DebugDraw( ColorB color ) const;
	bool	IsCircleInside( const Vec3& point, float Radius ) const;
};

class CSteeringAgent
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CSteeringAgent();
		~			CSteeringAgent();
		bool	Init( CGroupMember* pGroupMember, const Vec3& vStartingPos, float fRadius );
		void	Update( float fDeltaTime );
		void	Enable();
		void	Disable();
		bool	IsEnabled() const;

		/*$1- Util ---------------------------------------------------------------*/
		void	SetLeaderPosition( const Vec3& vLeaderPosition );
		void	SetLeaderOrientation( const Vec3& vLookingDir );
		void	SetDestination( const Vec3& vDestination );
		Vec3	GetPos() const;
		void	SetPos( const Vec3& vPos );

		/*$1- Debug  -------------------------------------------------------------*/
		void	DebugDraw() const;

	private:

		/*$1- Util ---------------------------------------------------------------*/
		Vec3		AddForce( const SForce& sForce, const Vec3& vPos ) const;
		Vec3		CalcForcesAtPos( const Vec3& vPos, bool bCollisionForces = false );
		Vec3		FilterSpeed( const Vec3& vSpeed, bool bBreakMax = false ) const;
		SForce	GenerateCollisionForce( int iPos );
		void		GenerateAllCollisions( float fDeltaTime );
		Vec3		GetCollisionPoint( int iPos ) const;

	private:

		/*$1- Members ------------------------------------------------------------*/
		Vec3								m_vPos;
		Vec3								m_vSpeed;
		Vec3								m_vLeaderPos;
		Vec3								m_vLastLeaderPos;
		Vec3								m_vLeaderLooking;
		Vec3								m_vDestinationPos;
		float								m_fLeaderPredictionTimeout;
		float								m_fRadius;
		float								m_fMaxSpeed;
		float								m_fMaxColSpeed;
		float								m_fMinSpeed;
		float								m_fBreaks;
		float								m_fMaxAccel;
		SForce							m_LeaderPredictionForce;
		std::vector<SForce> m_vecForces;
		std::vector<SForce> m_vecCollisionForces;
		CGroupMember*				m_pGroupMember;

		/*$1- Static debug data --------------------------------------------------*/
		bool m_bInit;
		bool m_bEnabled;
};

// Description:
//
// Arguments:
//
// Return:
//
inline Vec3 CSteeringAgent::GetPos() const
{
	assert( m_bInit != NULL );
	return( m_vPos );
}

// Description:
//
// Arguments:
//
// Return:
//
inline void CSteeringAgent::SetPos( const Vec3& vPos )
{
	assert( m_bInit != NULL );
	m_vPos = vPos;
}

// Description:
//
// Arguments:
//
// Return:
//
inline void CSteeringAgent::SetLeaderOrientation( const Vec3& vLookingDir )
{
	m_vLeaderLooking( vLookingDir.x, vLookingDir.y, 0.0f );
	m_vLeaderLooking.Normalize();
}

#endif // _STEERING_AGENT_H_
