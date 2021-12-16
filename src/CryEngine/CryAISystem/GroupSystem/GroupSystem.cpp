/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   GroupSystem.h
Description: Main interface to the whole Group System. The system
provides a way to create formation points (TPS queries) around a leader
and follow those points using a mixture of pathfinding and steering
behaviors.
---------------------------------------------------------------------
History:
- 06:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include "GroupSystem.h"
#include "GroupMember.h"
#include "DebugDrawContext.h"

#include "IVisualLog.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CGroupSystem::CGroupSystem() : m_bInit( true ), m_bDebug( false )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CGroupSystem::~CGroupSystem()
{
	Shutdown();
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::Init()
{
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::Shutdown()
{
	Clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::Clear()
{
	assert( m_bInit == true );

	{
		std::vector<CEntityGroup *>::iterator itC = m_vecGroups.begin();
		std::vector<CEntityGroup *>::iterator itEnd = m_vecGroups.end();
		for( ; itC != itEnd; ++itC )
		{
			delete *itC;
		}
	}

	m_vecGroups.clear();

	{
		std::vector<CGroupMember *>::iterator itC = m_vecMembers.begin();
		std::vector<CGroupMember *>::iterator itEnd = m_vecMembers.end();
		for( ; itC != itEnd; ++itC )
		{
			delete *itC;
		}
	}
	
	m_vecMembers.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::SetDebug( bool bDebug )
{
	m_bDebug = bDebug;
}

// Helper function used in CGroupSystem::Update()
static bool s_EntityGroupIsEmptyDelete( const CEntityGroup* pEntityGroup )
{
	bool bEmpty = pEntityGroup->IsEmpty();
	
	if (bEmpty)
	{
		delete pEntityGroup;
	}
	
	return bEmpty;
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::Update( float fDeltaTime )
{
	if( m_vecGroups.empty() )
	{
		return;
	}

	const std::vector<CEntityGroup *>::iterator itBegin = m_vecGroups.begin();
	const std::vector<CEntityGroup *>::iterator itEnd = m_vecGroups.end();

	for( std::vector<CEntityGroup *>::iterator itC = itBegin; itC != itEnd; ++itC )
	{
		( *itC )->Update( fDeltaTime );
	}

	// Remove and delete any empty entity groups
	// NOTE: s_EntityGroupIsEmptyDelete deletes empty EntityGroup instances
	m_vecGroups.erase( std::remove_if( itBegin, itEnd, s_EntityGroupIsEmptyDelete ), itEnd );
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::OnEditorSetGameMode( bool bGameMode )
{
	if( bGameMode == false )
	{
		Clear();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::OnObjectRemoved( CAIObject *pObject )
{
	assert(pObject);

	std::vector<CEntityGroup*>::iterator itGroup = m_vecGroups.begin();
	std::vector<CEntityGroup*>::iterator itGroupEnd = m_vecGroups.end();
	for (; itGroup != itGroupEnd; ++itGroup)
	{
		CEntityGroup *pGroup = (*itGroup);
		assert(pGroup);

		pGroup->OnObjectRemoved(pObject);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CEntityGroup* CGroupSystem::CreateGroup( EntityId idEntity )	// group id?
{
	static EntityGroupId s_nGroupId = 0;
	
	CEntityGroup*												pGroup = new CEntityGroup();	
	CGroupMember*												pMember = CreateGroupMember( idEntity );
	CEntityGroup*												pOldGroup = pMember->GetGroup();

	// if this member already has a group
	if( pOldGroup != NULL )
	{
		pOldGroup->RemoveMember( pMember );
	}

	pGroup->SetGroupId( ++s_nGroupId );

	// this will set pMember as a leader as its the first this group's member
	m_vecGroups.push_back(pGroup);
	pGroup->AddMember( pMember );

	return( pGroup );
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::DestroyGroup( EntityGroupId idGroup )
{
	std::vector<CEntityGroup*>::iterator itC = m_vecGroups.begin();
	std::vector<CEntityGroup*>::iterator itEnd = m_vecGroups.end();
	while( itC != itEnd )
	{
		if( (*itC)->GetGroupId() == idGroup )
		{
			break;
		}

		++itC;
	}

	if( itC != itEnd )
	{
		delete (*itC);
		m_vecGroups.erase( itC );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CEntityGroup* CGroupSystem::GetGroup( EntityGroupId idGroup )
{
	std::vector<CEntityGroup*>::iterator itC = m_vecGroups.begin();
	std::vector<CEntityGroup*>::iterator itEnd = m_vecGroups.end();
	while( itC != itEnd )
	{
		if( (*itC)->GetGroupId() == idGroup )
		{
			break;
		}

		++itC;
	}

	return( itC != itEnd ? (*itC) : NULL );
}

// Description:
//
// Arguments:
//
// Return:
//
CEntityGroup* CGroupSystem::MergeGroups( EntityGroupId idGroupA, EntityGroupId idGroupB )
{
	return( NULL );
}

// Description:
//	Adds a given member to the group with a given id. If member is a part of some other group
//	then he is forced to leave it (his potential leadership is handled by CEntityGroup).
// Arguments:
//
// Return:
//	false if group with given id doesn't exist
//	true otherwise
bool CGroupSystem::AddMemberToGroup( EntityGroupId idGroup, CGroupMember* pMember )
{
	CEntityGroup*		pGroup = GetGroup( idGroup );

	if( pGroup == NULL )											// group doesn't exis
	{
		GetAISystem()->Warning( "<CGroupSystem>", "AddMemberToGroup() no group with given id" );
		return( false );
	}
	else if( pGroup == pMember->GetGroup() )	// member already in this group
	{
		GetAISystem()->Warning( "<CGroupSystem>", "AddMemberToGroup() reading member to his group" );
		return( true );
	}

	if( pMember->GetGroup() != NULL )
	{
		pMember->GetGroup()->RemoveMember( pMember );
	}

	pGroup->AddMember( pMember );

	return( true );
}

// Description:
//
// Arguments:
//
// Return:
//
CGroupMember* CGroupSystem::CreateGroupMember( EntityId idEntity )
{
	assert( m_bInit == true );
	assert( idEntity > 0 );

	CGroupMember*		pMember = GetGroupMember( idEntity );

	if( pMember == NULL )
	{
		pMember = new CGroupMember( this );
		if( pMember->Init(idEntity) == true )
		{
			m_vecMembers.push_back( pMember );
		}
		else
		{
			SAFE_DELETE( pMember );
		}
	}

	return( pMember );
}

// Description:
//
// Arguments:
//
// Return:
//
CGroupMember* CGroupSystem::GetGroupMember( EntityId idEntity )
{
	CGroupMember*													pPersonal = NULL;
	std::vector<CGroupMember *>::iterator itC = m_vecMembers.begin();
	std::vector<CGroupMember *>::iterator itEnd = m_vecMembers.end();

	while( itC != itEnd )
	{
		CGroupMember*		pTemp = (CGroupMember*) *itC;
		if( pTemp->GetEntityId() == idEntity )
		{
			pPersonal = pTemp;
			break;
		}

		++itC;
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::DestroyGroupMember( EntityId idEntity )
{
	CGroupMember* pMember = GetGroupMember( idEntity );
	if( pMember )
	{
		DestroyGroupMember( pMember );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::DestroyGroupMember( CGroupMember* pMember )
{
	std::vector<CGroupMember *>::iterator itC = m_vecMembers.begin();
	std::vector<CGroupMember *>::iterator itEnd = m_vecMembers.end();

	while( itC != itEnd )
	{
		CGroupMember*		pTemp = (CGroupMember*) *itC;
		if( pTemp == pMember )
		{
			if( pMember->GetGroup() != NULL )
			{
				pMember->GetGroup()->RemoveMember( pMember );
			}

			m_vecMembers.erase( itC );
			SAFE_DELETE( pMember );

			break;
		}

		++itC;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CGroupSystem::UpdateDraw()
{
	IVisualLog* pVLog = gEnv->pVisualLog;
	assert(pVLog);

	CDebugDrawContext dc;

	SVisualLogParams	visLogParams( ColorF(1, 1, 1, 1), 1.2f, 3, false );

	// printing all groups	
	dc->TextToScreen( 1, 15, "----------------------------" );
	dc->TextToScreen( 1, 16, "---- Groups and members-----" );
	dc->TextToScreen( 1, 17, "----------------------------" );
	dc->TextToScreen( 1, 19, "Total of %d groups and %d members", m_vecGroups.size(), m_vecMembers.size() );
	pVLog->Log( visLogParams, "----------- Groups and members: -----------" );

	int y = 21;
	{
		std::vector<CEntityGroup*>::iterator itC = m_vecGroups.begin();
		std::vector<CEntityGroup*>::iterator itEnd = m_vecGroups.end();
		for( ; itC != itEnd; ++itC, ++y )
		{
			const char*		pName = "NULL";
			if( (*itC)->GetLeader() && (*itC)->GetLeader()->GetActor() )
			{
				pName = (*itC)->GetLeader()->GetActor()->GetName();
			}

			dc->TextToScreen( 1.f, (float)++y, "[%d]: Leader: %s, size: %d", (*itC)->GetGroupId(), pName, (*itC)->GetMemberCount() );

			CEntityGroup::TMemberContainer *pMembers = (*itC)->GetMembers();
			{
				CEntityGroup::TMemberContainer::iterator itC2 = pMembers->begin();
				CEntityGroup::TMemberContainer::iterator itEnd2 = pMembers->end();
				for( ; itC2 != itEnd2; ++itC2 )
				{
					dc->TextToScreen( 8.f, (float)(++y), "%s", (*itC2)->GetActor()->GetName() );
				}
			}

			(*itC)->DebugDraw();
		}
	}

	{
		std::vector<CGroupMember *>::iterator itC = m_vecMembers.begin();
		std::vector<CGroupMember *>::iterator itEnd = m_vecMembers.end();

		while( itC != itEnd )
		{
			CGroupMember*		pTemp = (CGroupMember*) *itC;
			pTemp->DebugDraw();
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
void CGroupSystem::Serialize( TSerialize ser )
{
	ser.BeginGroup( "AI_GroupSystem" );
	{
		ser.BeginGroup( "AI_GroupMembers" );
		{
			int nMemberCount = 0;
			ser.Value( "nMemberCount", nMemberCount );
			if( ser.IsReading() && nMemberCount > 0 )
			{
				for( int i = 0; i < nMemberCount; ++i )
				{
					m_vecMembers.push_back( new CGroupMember(this) );
				}
			}

			std::vector<CGroupMember *>::iterator itC = m_vecMembers.begin();
			std::vector<CGroupMember *>::iterator itEnd = m_vecMembers.end();
			for( ; itC != itEnd; ++itC )
			{
				( *itC )->Serialize( ser );
			}
		}

		ser.EndGroup();

		ser.BeginGroup( "AI_EntityGroups" );
		{
			int nGroupCount = 0;
			ser.Value( "nGroupCount", nGroupCount );
			if( ser.IsReading() )
			{
				m_vecGroups.resize( nGroupCount );
			}

			std::vector<CEntityGroup*>::iterator itC = m_vecGroups.begin();
			std::vector<CEntityGroup*>::iterator itEnd = m_vecGroups.end();
			for( ; itC != itEnd; ++itC )
			{
				(*itC)->Serialize( ser );
			}
		}

		ser.EndGroup();
	}

	ser.EndGroup();
}
