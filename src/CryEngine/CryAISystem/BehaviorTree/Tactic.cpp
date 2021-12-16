/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   Tactic.cpp
$Id$
$DateTime$
Description: A tactic is just a container of subtactics. A tactic is
only marked as "running" when all its subtactics are ready to run.
It will also monitoize the resources on it and stop itself if needed.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "Tactic.h"
#include "SubTactic.h"
#include "TacticsManager.h"
#include "TacticInfo.h"
#include "SlotGroup.h"
#include "Slot.h"
#include "BehaviorTree.h"
#include "ProfileDictionary.h"
#include "../CryCommon/IPersonalBehaviorTree.h"
#include "DebugDrawContext.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CTactic::CTactic( CTacticInfo const* pInfo, CTacticsManager* pManager, bool bIsInstance )
: m_bIsNewlyCreated ( true )
, m_bIsInstance( bIsInstance )
, m_bObsolete( false )
, m_bNeedNewInstance( false )
, m_State( ETS_WAITING_FOR_MORE_CANDIDATES )
, m_CurrentGroup( NULL_TACTIC_GROUP )
, m_pInfo( pInfo )
, m_pManager( pManager )
, m_pSlots( NULL )
{
	assert( pInfo != NULL );
	assert( pManager != NULL );

	m_pSlots = new CSlotGroup;

	std::vector<SSubTacticInfo *>::const_iterator itC = m_pInfo->m_vecSubTacticsInfo.begin();
	std::vector<SSubTacticInfo *>::const_iterator itEnd = m_pInfo->m_vecSubTacticsInfo.end();

	for( ; itC != itEnd; ++itC )
	{
		CreateSubTactic( *itC );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CTactic::~CTactic()
{
	SAFE_DELETE( m_pSlots );

	std::vector<CSubTactic *>::iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::iterator itEnd = m_vecSubTactics.end();

	for( ; itC != itEnd; ++itC )
	{
		SAFE_DELETE( *itC );
	}

	m_vecSubTactics.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CTactic::Reset()
{
	m_State = ETS_WAITING_FOR_MORE_CANDIDATES;
	m_bIsNewlyCreated = true;

	std::vector<CSubTactic *>::iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::iterator itEnd = m_vecSubTactics.end();

	for( ; itC != itEnd; ++itC )
	{
		( *itC )->Reset();
	}

	m_CurrentGroup = NULL_TACTIC_GROUP;
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CTactic::DebugDraw( float xPos, float yPos ) const
{
	CDebugDrawContext dc;

	uint32 uRes = 1 + m_vecSubTactics.size();
	//	float fColor[4] = { 1.0f, 0.1f, 0.1f, 1.0f };

	string sState;
	switch( m_State )
	{
	case ETS_WAITING_FOR_MORE_CANDIDATES:
		sState = "Waiting for candidates";
		break;

	case ETS_WAITING_FOR_ACKNOWELDGE:
		sState = "Waiting for acknowledge";
		break;

	case ETS_RUNNING:
		sState = "Running";
		break;
	}

	dc->Draw2dLabel( xPos, yPos, 1.5f, ColorB(255, 26, 26), false, "%s | %s:", GetName().c_str(), sState.c_str() );

	std::vector<CSubTactic *>::const_iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::const_iterator itEnd = m_vecSubTactics.end();

	float yThisPos = yPos;
	for( uint32 i = 0; itC != itEnd; ++itC, ++i )
	{
		uint32 uLines = ( *itC )->DebugDraw( xPos + 50, yThisPos );
		yThisPos += ( (1 + uLines) * 20 );
		uRes += uLines;
	}

	return( uRes );
}

// Description:
//
// Arguments:
//
// Return:
//
const string& CTactic::GetName() const
{
	return( m_pInfo->GetName() );
}

// Description:
//
// Arguments:
//
// Return:
//
CTacticInfo const* CTactic::GetInfo() const
{
	return( m_pInfo );
}

// Description:
//
// Arguments:
//
// Return:
//
ETacticState CTactic::GetState() const
{
	return( m_State );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::IsInstance() const
{
	return( m_bIsInstance );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::IsObsolete() const
{
	return( m_bObsolete );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::NeedNewInstance() const
{
	return( m_bNeedNewInstance );
}

// Description:
//
// Arguments:
//
// Return:
//
void CTactic::SetRequestNewInstance( bool bRequest, TBSSProfileUserId requestingUser )
{
	// Check exceed count first (would one more creation exceed the limit?)
	if ( m_bNeedNewInstance == false && bRequest == true && requestingUser != g_uBTUserId_Invalid )
	{
		TTacticGroupId groupId = m_pManager->GetUserGroupId( requestingUser );
		if ( groupId != NULL_TACTIC_GROUP )
		{
			const uint32 uCount = m_pManager->GetRunningGroupInstancesCount( m_pInfo, groupId );
			bRequest = ( ExceedsMaxGroupInstances(uCount+1) == false );
		}
	}

	m_bNeedNewInstance = bRequest;
}

// Description:
//
// Arguments:
//
// Return:
//
TTacticGroupId CTactic::GetCurrentGroup() const
{
	return( m_CurrentGroup );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CTactic::FindResource( TBSSProfileUserId userId ) const
{
	CSubTactic *pRet = NULL;
	std::vector<CSubTactic *>::const_iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::const_iterator itEnd = m_vecSubTactics.end();

	for( ; itC != itEnd && pRet == NULL; ++itC )
	{
		if( (*itC)->IsResourceAssigned(userId) == true )
		{
			pRet = *itC;
		}
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::IsActive() const
{
	return( m_State == ETS_RUNNING );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CTactic::GetSubTactic( const string& sName )
{
	CSubTactic *pRet = NULL;
	std::vector<CSubTactic *>::iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::iterator itEnd = m_vecSubTactics.end();

	for( ; itC != itEnd && pRet == NULL; ++itC )
	{
		if( (*itC)->GetName().compareNoCase(sName) == 0 )
		{
			pRet = *itC;
		}
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CTactic::GetSubTactic( const SSubTacticInfo* pSubInfo )
{
	CSubTactic *pRet = NULL;
	std::vector<CSubTactic *>::iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::iterator itEnd = m_vecSubTactics.end();

	for( ; itC != itEnd && pRet == NULL; ++itC )
	{
		if( (*itC)->GetInfo() == pSubInfo )
		{
			pRet = *itC;
		}
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CTactic::CreateSubTactic( SSubTacticInfo const* pSubTacticInfo )
{
	// Create slot map for subtactic
	TSubTacticRequirements::const_iterator itReq = pSubTacticInfo->Requirements.begin();
	TSubTacticRequirements::const_iterator itEnd = pSubTacticInfo->Requirements.end();
	TSlotMap Slots;
	for( ; itReq != itEnd; ++itReq )
	{
		Slots[itReq->first] = m_pSlots->AddSlot();
	}

	CSubTactic *pRet = new CSubTactic( this, pSubTacticInfo, Slots );
	m_vecSubTactics.push_back( pRet );
	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::DeleteResource( TBSSProfileUserId userId, bool bOnlyIfNotInLastRun )
{
	bool bRet = false;
	std::vector<CSubTactic *>::iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::iterator itEnd = m_vecSubTactics.end();

	bool bHasCandidates = false;
	for( ; itC != itEnd; ++itC )
	{
		bool bThisHasCandidates = false;
		bRet |= ( *itC )->DeleteResource( userId, bThisHasCandidates, bOnlyIfNotInLastRun );

		if (bThisHasCandidates)
			bHasCandidates = true;
	}

	if (bRet && !bHasCandidates)
	{
		// Drop the group
		m_CurrentGroup = 0;
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CTactic::RunTreeForMyResources( bool bOnlyNewAssigned, TBSSProfileUserId ignoreUser )
{
	std::list<TBSSProfileUserId> listUniqueResources;

	if( bOnlyNewAssigned == true )
	{
		m_pSlots->GetNewAssignedResources( listUniqueResources );
	}
	else
	{
		m_pSlots->GetCandidateResources( listUniqueResources );
	}

	CProfileDictionary *pDictionary = GetAISystem()->GetBTProfileDictionary();
	CRY_ASSERT(pDictionary);

	// First clear their list, the ones that are not shared
	std::list<TBSSProfileUserId>::const_iterator it = listUniqueResources.begin();
	std::list<TBSSProfileUserId>::const_iterator itEnd = listUniqueResources.end();

	for( ; it != itEnd; ++it )
	{
		if( *it == ignoreUser )
		{
			continue;
		}

		SBTProfileUser *pUser = pDictionary->GetProfileUser(*it);
		if( pUser != NULL )
		{
			pUser->RunTree();
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::CheckIfBestResources()
{
	bool bRet = m_pSlots->AssignResources();

	if( bRet == true )
	{
		RunTreeForMyResources();
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::CheckUserGroup( TBSSProfileUserId userId )
{
	bool bMatchesGroup = true;

	const CTacticsManager *pManager = GetAISystem()->GetBTProfileDictionary()->GetTacticsManager();
	assert(pManager);

	TTacticGroupId groupId = pManager->GetUserGroupId(userId);
	if (NULL_TACTIC_GROUP == m_CurrentGroup)
	{
		const uint32 uCount = pManager->GetRunningGroupInstancesCount( m_pInfo, groupId );
		if ( ExceedsMaxGroupInstances( uCount+1 ) == true )
		{
			// Not allowed to claim if you will produce excessive amount of instances
			bMatchesGroup = false;
		}
		else
		{
			m_CurrentGroup = groupId;
		}
	}
	else if (NULL_TACTIC_GROUP == groupId || m_CurrentGroup != groupId)
	{
		bMatchesGroup = false;
	}

	return bMatchesGroup;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTactic::ExceedsMaxGroupInstances(uint32 uNumInstances) const
{
	return( m_pInfo && m_pInfo->ExceedsMaxGroupInstances(uNumInstances) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CTactic::ChangeState( ETacticState NewState )
{
	// Notify subtactics
	std::vector<CSubTactic *>::iterator itC = m_vecSubTactics.begin();
	std::vector<CSubTactic *>::iterator itEnd = m_vecSubTactics.end();

	for( ; itC != itEnd; ++itC )
	{
		( *itC )->ChangeState( NewState );
	}

	m_State = NewState;

	// Now move resources as needed
	switch( m_State )
	{
	case ETS_WAITING_FOR_MORE_CANDIDATES:
		{
			if( m_pSlots->DeAssignResources() == true )
			{
				RunTreeForMyResources();
			}
		}

		break;

	case ETS_WAITING_FOR_ACKNOWELDGE:
		{
			if( m_pSlots->AssignResources() == true )
			{
				RunTreeForMyResources();
			}
		}

		break;

	case ETS_RUNNING:
		{
			if( m_pManager->NotifyTacticRunning(m_pInfo) == true )
			{
				// An instance was created, run tree for non-assigned candidates
				RunTreeForMyResources();
			}
		}

		break;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CTactic::Update()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	m_bObsolete = false;

	// Don't update if newly created
	if ( m_bIsNewlyCreated )
	{
		m_bIsNewlyCreated = false;
		return;
	}

	switch( m_State )
	{
	case ETS_WAITING_FOR_MORE_CANDIDATES:
		{
			if( m_pSlots->HaveEnoughCandidates() == true )
			{
				ChangeState( ETS_WAITING_FOR_ACKNOWELDGE );
			}
			else if( m_bIsInstance == true )
			{
				m_bObsolete = ( m_pInfo->UsesGroup() == true || m_pSlots->HaveCandidates() == false );
			}
		}

		break;

	case ETS_WAITING_FOR_ACKNOWELDGE:
		{
			if( m_pSlots->HaveEnoughCandidates() == false )
			{
				ChangeState( ETS_WAITING_FOR_MORE_CANDIDATES );
			}
			else if( m_pSlots->HaveEnoughAssigned() == false )
			{
				if( m_pInfo->UsesGroup() )
				{
					m_bObsolete = true;
				}
				else
				{
					// Odd case, force re-assignment of resources
					ChangeState( ETS_WAITING_FOR_ACKNOWELDGE );
				}
			}

			if( m_pSlots->HaveEnoughRunning() == true )
			{
				ChangeState( ETS_RUNNING );
			}
			else if( m_pInfo->UsesGroup() )
			{
				// Most likely we have not good candidates to run, so try again
				Reset();
			}
		}

		break;

	case ETS_RUNNING:
		{
			if( m_pSlots->HaveEnoughRunning() == false )
			{
				if( m_pSlots->HaveEnoughCandidates() == true )
				{
					ChangeState( ETS_WAITING_FOR_ACKNOWELDGE );
				}
				else
				{
					ChangeState( ETS_WAITING_FOR_MORE_CANDIDATES );
				}
			}
			else
			{
				if( CheckIfBestResources() == true )
				{
					ChangeState( ETS_WAITING_FOR_ACKNOWELDGE );
				}
			}
		}

		break;
	}
}
