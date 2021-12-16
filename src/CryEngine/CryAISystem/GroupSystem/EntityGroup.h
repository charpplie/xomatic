/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   EntityGroup.h
Description: 
---------------------------------------------------------------------
History:
- 11:02:2008 : Created by mieszko
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _ENTITYGROUP_H_
	#define _ENTITYGROUP_H_

typedef unsigned int	EntityGroupId;
class									CGroupMember;
class									CFormationData;

/*$1- Helper structure implementation ----------------------------------------*/
struct SFormationSpec
{

	/*$1- Params ---------------------------------------------------------------*/
	float m_fPositionTimeout;
	float m_fPerFrameTestFailureDelay;
	bool	m_bFloatingFormationPos;
	bool	m_bMatchLeaderSpeed;
	bool	m_bUseBestPoint;

	/*$1- Query names ----------------------------------------------------------*/
	char*		m_psComputePosQuery;
	char*		m_psTestComputedPosQuery;
	char*		m_psTestCurMemberPosQuery;
	char*		m_psTestPerFrameQuery;
	char*		m_psFormationSet;

	/*$1- Calculated query ids -------------------------------------------------*/
	int m_nComputePosQuery;
	int m_nTestComputedPosQuery;
	int m_nTestCurMemberPosQuery;
	int m_nTestPerFrameQuery;

	/*$1- Construction ---------------------------------------------------------*/
	SFormationSpec()
	:
	m_psComputePosQuery( NULL ),
	m_psTestComputedPosQuery( NULL ),
	m_psTestCurMemberPosQuery( NULL ),
	m_psTestPerFrameQuery( NULL ),
	m_psFormationSet( NULL ),
	m_fPositionTimeout( 3600.0f ),
	m_fPerFrameTestFailureDelay( 0.0f ),
	m_bFloatingFormationPos( true ),
	m_bMatchLeaderSpeed( false ),
	m_bUseBestPoint( true )
	{
	}

	void	CalculateQueryIds();

	void	Serialize( TSerialize ser );
};

/*$2- Class implementation -------------------------------------------------- */

class CEntityGroup
{
		friend class											CGroupSystem;
		typedef std::vector<CGroupMember *> TMemberContainer;
		typedef std::vector<int> TOrderContainer;
		typedef std::vector<float> TDistanceVector;
		typedef std::vector<Vec3> TPointContainer;

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CEntityGroup();
		~ CEntityGroup();

		/*$1- Util ---------------------------------------------------------------*/
		void	Update( float fDeltaTime );
		void	Destroy();
		void	Serialize( TSerialize ser );
		void	OnObjectRemoved( CAIObject *pObject );

		void	AddMember( CGroupMember* pNewMember );
		void	RemoveMember( CGroupMember* pMember );
		int		GetMemberCount() const;

		void	SetMatchLeaderSpeed( bool bMatch );

		/** Sets a new group's leader. If this CGroupMember instance isn't in this 
		* group false will be returned */
		bool	SetLeader( CGroupMember* pNewLeader );
		CGroupMember const* GetLeader() const
		{
			return( m_pLeader );
		}

		CGroupMember*		GetLeader() { return( m_pLeader ); }

		EntityGroupId GetGroupId() const
		{
			return( m_nId );
		}

		void	SetFormationSpec( SFormationSpec& formSpec );
		SFormationSpec const* GetFormationSpec() const
		{
			return( &m_formationSpec );
		}

		bool	IsEmpty() const { assert( m_bEmpty == m_vecMembers.size() < 1 ); return( m_bEmpty ); }

		/*$1- Debug  -------------------------------------------------------------*/
		void	DebugDraw() const;

	protected:
		TMemberContainer*		GetMembers()										{ return( &m_vecMembers ); }

		void								SetGroupId( EntityGroupId nId ) { m_nId = nId; }

		void								OnMemberRemoved( CGroupMember* pMember );
		Vec3								GetOffsetPoint( const Vec3& vRelPos ) const;
		void								UpdateFormation();
		void								ApplyFormationChange();
		void								RecalcPoints(float fDeltaTime, bool bForceBestUse = false);
		void								RecalcOrderList(int nNotLeaderMembers, TDistanceVector& vecDistances);
		void								UpdateRefPoints( CAIActor* pLeader );
		float								GetBestStartingFrom( int nMember, int nNotLeaderMembers, TDistanceVector& vecDistances, TOrderContainer& vecOrder ) const;

	private:

		/*$1- Members ------------------------------------------------------------*/
		bool m_bEmpty;
		bool m_bFormationChanged;
		EntityGroupId m_nId;
		CGroupMember* m_pLeader;
		const CFormationData* m_pFormation;
		Vec3 m_vCurrOrientation;
		TPointContainer m_vecPoints;
		TOrderContainer m_vecOrder;
		TMemberContainer m_vecMembers;
		SFormationSpec m_formationSpec;

/*$1- Static debug data ------------------------------------------------------*/
};

/*$1- GetMemberCount ---------------------------------------------------------*/
inline int CEntityGroup::GetMemberCount() const
{
	return( m_vecMembers.size() );
}

#endif // _ENTITYGROUP_H_
