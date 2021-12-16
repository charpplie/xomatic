/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   GroupMember.h
Description: 
---------------------------------------------------------------------
History:
- 06:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _GROUP_MEMBER_H_
	#define _GROUP_MEMBER_H_

	#include "SteeringAgent.h"

class CEntityGroup;
class CAIObject;

class CGroupMember
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CGroupMember( CGroupSystem* pGroup );
		~			CGroupMember();
		bool	Init( EntityId id );
		void	Update( float fDeltaTime );
		void	Clear();

		/*$1- Util ---------------------------------------------------------------*/
		void									Serialize( TSerialize ser );
		void									Enable();
		void									Disable();
		CAIActor*							GetActor();
		CAIActor const*				GetActor() const;
		EntityId							GetEntityId() const;
		void									SetGroup( CEntityGroup* pGroup );
		void									SetIsLeader( bool bLeader );
		const CGroupMember*		GetLeader() const;
		CEntityGroup*					GetGroup();
		CEntityGroup const*		GetGroup() const;
		float									GetFormationSpeed() const;
		void									SetFormationSpeed( float fSpeed );
		Vec3									GetFormationPos() const;
		Vec3									GetFormationRef() const;
		CAIObject*						GetFormationRefAIObject();
		CAIObject const*			GetFormationRefAIObject() const;
		bool									IsLeader() const;
		CAIObject*						GetFormationPosAIObject();
		CAIObject const*			GetFormationPosAIObject() const;
		Vec3									GetSteeringPos() const;
		void									ForceFormationPosRegeneration();
		bool									IsUsingSteering() const;
		void									UseSteering( bool bUseSteering, float m_fDistanceToTriggerSteering = 5.0f );
		void									SetFormationRef( const Vec3& vRefPos );
		void									OnFormationChanged();

		/*$1- Debug  -------------------------------------------------------------*/
		void	DebugDraw() const;

	private:

		/*$1- Util ---------------------------------------------------------------*/
		Vec3	FindProperPosition();
		bool	TestCurrentPosition();				// const;
		bool	TestCurrentFormationPoint();	// const;
		bool	RunPerFrameQuery();
		bool	TestCurPointWithQuery( int nQuery );
		Vec3	CalculateNewFormationPos();
		void	SetLastLeaderPos( Vec3 vPos );
		
	private:

		/*$1- Members ------------------------------------------------------------*/
		float							m_fTimeout;
		float							m_fPerFrameDelay;
		float							m_fDistanceToTriggerSteering;
		float							m_fSpeed;
		bool							m_bLeader;
		CGroupMember*			m_pLeader;
		Vec3							m_vCurPoint;
		CWeakRef<CAIActor>	 m_refAIActor;
		bool							m_bLastFrameUsedSteering;

		CStrongRef<CAIObject> m_refAiDummy;
		CStrongRef<CAIObject> m_refAiRefDummy;
		Vec3							m_vLastLeaderPos;
		Vec3							m_vLastCheckedLeaderPos;
		float							m_fLeaderDistTraveled;
		bool							m_bErraticMovement;
		Vec3							m_vLastGoodPoint;
		CEntityGroup*			m_pGroup;
		CGroupSystem*			m_pGroupSystem;
		CSteeringAgent*		m_pSteeringAgent;
		EntityId					m_EntityId;

		/*$1- Static debug data --------------------------------------------------*/
		bool	m_bInit;
};

/*$1- GetEntityId ------------------------------------------------------------*/
inline EntityId CGroupMember::GetEntityId() const
{
	assert( m_bInit == true );
	return( m_EntityId );
}

/*$1- SetIsLeader ------------------------------------------------------------*/
inline void CGroupMember::SetIsLeader( bool bLeader )
{
	assert( m_bInit == true );
	m_bLeader = bLeader;
}

/*$1- SetIsLeader ------------------------------------------------------------*/
inline const CGroupMember*CGroupMember::GetLeader() const
{
	return m_pLeader;
}

/*$1- GetGroup ---------------------------------------------------------------*/
inline CEntityGroup* CGroupMember::GetGroup()
{
	assert( m_bInit == true );
	return( m_pGroup );
}

/*$1- GetGroup ---------------------------------------------------------------*/
inline CEntityGroup const* CGroupMember::GetGroup() const
{
	assert( m_bInit == true );
	return( m_pGroup );
}

/*$1- GetFormationSpeed ------------------------------------------------------*/
inline float CGroupMember::GetFormationSpeed() const
{
	assert( m_bInit == true );
	return( m_fSpeed );
}

/*$1- SetFormationSpeed ------------------------------------------------------*/
inline void	CGroupMember::SetFormationSpeed( float fSpeed )
{
	assert( m_bInit == true );
	m_fSpeed = fSpeed;
}

/*$1- GetFormationPos --------------------------------------------------------*/
inline Vec3 CGroupMember::GetFormationPos() const
{
	assert( m_bInit == true );
	return( m_vLastGoodPoint );
}

/*$1- GetFormationPos --------------------------------------------------------*/
inline CAIObject const* CGroupMember::GetFormationRefAIObject() const
{
	assert( m_bInit == true );
	return( m_refAiRefDummy.GetAIObject() );
}

/*$1- GetFormationPos --------------------------------------------------------*/
inline CAIObject* CGroupMember::GetFormationRefAIObject() 
{
	assert( m_bInit == true );
	return( m_refAiRefDummy.GetAIObject() );
}

/*$1- GetFormationPos --------------------------------------------------------*/
inline Vec3 CGroupMember::GetFormationRef() const
{
	assert( m_bInit == true );
	return( m_refAiRefDummy.GetAIObject()->GetPos() );
}

/*$1- GetFormationPos --------------------------------------------------------*/
inline void CGroupMember::SetFormationRef( const Vec3& vRefPos )
{
	m_refAiRefDummy.GetAIObject()->SetPos( vRefPos );
}

/*$1- IsLeader ---------------------------------------------------------------*/
inline bool CGroupMember::IsLeader() const
{
	assert( m_bInit == true );
	return( m_bLeader );
}

/*$1- IsUsingSteering --------------------------------------------------------*/
inline bool CGroupMember::IsUsingSteering() const
{
	assert( m_bInit == true );
	return( m_pSteeringAgent != NULL );
}

/*$1- GetFormationPosAIObject ------------------------------------------------*/
inline CAIObject* CGroupMember::GetFormationPosAIObject()
{
	assert( m_bInit == true );
	return( m_refAiDummy.GetAIObject() );
}

/*$1- GetFormationPosAIObject ------------------------------------------------*/
inline CAIObject const* CGroupMember::GetFormationPosAIObject() const
{
	assert( m_bInit == true );
	return( m_refAiDummy.GetAIObject() );
}

/*$1- GetSteeringPos ---------------------------------------------------------*/
inline Vec3 CGroupMember::GetSteeringPos() const
{
	assert( m_bInit == true );
	return( m_pSteeringAgent->GetPos() );
}

/*$1- GetActor ---------------------------------------------------------------*/
inline CAIActor* CGroupMember::GetActor()
{
	assert( m_bInit == true );
	return( m_refAIActor.GetAIObject() );
}

/*$1- GetActor ---------------------------------------------------------------*/
inline CAIActor const* CGroupMember::GetActor() const
{
	assert( m_bInit == true );
	return( m_refAIActor.GetAIObject() );
}

/*$1- GetActor ---------------------------------------------------------------*/
inline void CGroupMember::SetLastLeaderPos( Vec3 vPos )
{
	m_vLastCheckedLeaderPos = m_vLastLeaderPos = vPos;
	m_fLeaderDistTraveled = 0.f;
}


#endif // _GROUP_MEMBER_H_
