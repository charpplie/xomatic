/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   SubTactic.cpp
$Id$
$DateTime$
Description: Subtatic is basically a CSlot (see Slot.cpp) with its
own resources and a configuration (CTacticInfo).
---------------------------------------------------------------------
History:
- 13:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "SubTactic.h"
#include "Tactic.h"
#include "TacticInfo.h"
#include "Slot.h"
#include "ProfileDictionary.h"
#include "DebugDrawContext.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSubTactic::CSubTactic( CTactic* pTactic, SSubTacticInfo const* pInfo, TSlotMap& Slots )
: m_pTactic( pTactic )
, m_pInfo( pInfo )
, m_LastRun( 0 )
{
	assert( pTactic != NULL );
	assert( pInfo != NULL );

	m_Slots = Slots;

	Reset();
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic::~CSubTactic()
{
}

// Description:
//
// Arguments:
//
// Return:
//
void CSubTactic::Reset()
{
	TSlotMap::iterator itSlot = m_Slots.begin();
	TSlotMap::iterator itEnd = m_Slots.end();
	for( ; itSlot != itEnd; ++itSlot )
	{
		CSlot *pSlot = itSlot->second;
		CRY_ASSERT( pSlot );

		TSubTacticRequirements::const_iterator itRequirement = m_pInfo->Requirements.find( itSlot->first );
		CRY_ASSERT( itRequirement != m_pInfo->Requirements.end() );

		SSubTacticRequirement const& Requirement = itRequirement->second;

		pSlot->SetMinMax( Requirement.uMinResourcesToStart, Requirement.uMaxResources );
		pSlot->SetSortScriptCode( Requirement.sQSortCallback );
		pSlot->Reset();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CSubTactic::DebugDraw( float xPos, float yPos ) const
{
	CDebugDrawContext dc;

	uint32 uRes = 1 + m_Slots.size();

	//	float		fColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CProfileDictionary *pProfileDictionary = GetAISystem()->GetBTProfileDictionary();

	string sTreeName;
	string sRunning;
	string sAssigned;
	string sCandidates;
	string sIdealCandidates;

	TSlotMap::const_iterator itSlot = m_Slots.begin();
	TSlotMap::const_iterator itEnd = m_Slots.end();
	uint32 uCount = 0;
	for( ; itSlot != itEnd; ++itSlot, ++uCount )
	{
		CSlot *pSlot = itSlot->second;
		CRY_ASSERT( pSlot );

		TSubTacticRequirements::const_iterator itRequirement = m_pInfo->Requirements.find( itSlot->first );
		CRY_ASSERT( itRequirement != m_pInfo->Requirements.end() );

		SSubTacticRequirement const& Requirement = itRequirement->second;

		pProfileDictionary->GetProfileName( itRequirement->first, sTreeName );
		pSlot->GetRunningAsString( sRunning );
		pSlot->GetAssignedAsString( sAssigned );
		pSlot->GetCandidatesAsString( sCandidates );
		pSlot->GetIdealCandidatesAsString( sIdealCandidates );

		string sName = GetName();
		if (UsesGroup())
		{
			string sGroupNumber;
			sGroupNumber.Format("[%u]", m_pTactic->GetCurrentGroup());
			sName += sGroupNumber;
		}

		dc->Draw2dLabel(
			xPos,
			yPos + ((1 + uCount) * 20),
			1.5f,
			ColorB(255, 255, 255),
			false,
			"%s [%s] (%d//%d/%d) Running: [%s] Assigned: [%s] Ideal_Candidates: [%s] Candidates: [%s]",
			sName.c_str(),
			sTreeName.c_str(),
			Requirement.uMinResourcesToStart,
			Requirement.uMinResources,
			Requirement.uMaxResources,
			sRunning.c_str(),
			sAssigned.c_str(),
			sIdealCandidates.c_str(),
			sCandidates.c_str() );
	}

	return( uRes );
}

// Description:
//
// Arguments:
//
// Return:
//
const string& CSubTactic::GetName() const
{
	return( m_pInfo->sName );
}

// Description:
//
// Arguments:
//
// Return:
//
SSubTacticInfo const* CSubTactic::GetInfo() const
{
	return( m_pInfo );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::UsesGroup() const
{
	return( m_pInfo && m_pInfo->pInfo && m_pInfo->pInfo->UsesGroup() );
}

// Description:
//
// Arguments:
//
// Return:
//
CTactic const* CSubTactic::GetTactic() const
{
	return( m_pTactic );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSubTactic::RequestNewTacticInstance( TBSSProfileUserId userId ) const
{
	assert(m_pTactic);
	m_pTactic->SetRequestNewInstance(true, userId);
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::DeleteResource( TBSSProfileUserId userId, bool &bHasCandidates, bool bOnlyIfNotInLastRun )
{
	assert( userId != g_uBTUserId_Invalid );

	const uint32 uBTProfileId = GET_PROFILE_FROM_USERID(userId);
	assert( uBTProfileId > g_uBTProfileId_Invalid );

	bool bRet = false;

	TSlotMap::iterator itSlot = m_Slots.find( uBTProfileId );
	if( itSlot != m_Slots.end() )
	{
		if( bOnlyIfNotInLastRun == true )
		{
			if( m_LastRun != userId )
			{
				bRet = itSlot->second->Delete( userId );
			}

			m_LastRun = 0;
		}
		else
		{
			bRet = itSlot->second->Delete( userId );
		}
	}

	bHasCandidates = (bRet && UsesGroup());
	if (bHasCandidates)
	{
		// Check count for group control
		TSlotMap::const_iterator itCounterSlot = m_Slots.begin();
		TSlotMap::const_iterator itCounterSlotEnd = m_Slots.end();
		for (; bHasCandidates && itCounterSlot != itCounterSlotEnd; ++itCounterSlot)
		{
			bHasCandidates &= itCounterSlot->second->HaveCandidates();
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::CheckUserGroup( TBSSProfileUserId userId  )
{
	return( m_pTactic->CheckUserGroup( userId ) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::AddCandidate( TBSSProfileUserId userId, bool &bMatchesGroup )
{
	assert( userId != g_uBTUserId_Invalid );

	const uint32 uBTProfileId = GET_PROFILE_FROM_USERID(userId);
	assert( uBTProfileId > g_uBTProfileId_Invalid );

	bool bRes = false;

	// Check group Id if this tactic only works in a group and we have a group potential
	const bool bUsesGroup = UsesGroup();
	bMatchesGroup = false;
	if (bUsesGroup)
	{
		bMatchesGroup = CheckUserGroup(userId);
	}

	// Attempt to add candidate if it matches the group
	if (!bUsesGroup || bMatchesGroup)
	{
		TSlotMap::iterator itSlot = m_Slots.find( uBTProfileId );
		if( itSlot != m_Slots.end() )
		{
			bRes = itSlot->second->AddCandidate( userId );
		}
	}

	return( bRes );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSubTactic::OnResourceLeave( TBSSProfileUserId userId )
{
	assert( userId != g_uBTUserId_Invalid );

	const uint32 uBTProfileId = GET_PROFILE_FROM_USERID(userId);
	assert( uBTProfileId > g_uBTProfileId_Invalid );

	TSlotMap::iterator itSlot = m_Slots.find( uBTProfileId );
	if( itSlot != m_Slots.end() )
	{
		itSlot->second->RemoveResourceRunning( userId );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CSubTactic::OnResourceEnter( TBSSProfileUserId userId )
{
	assert( userId != g_uBTUserId_Invalid );

	const uint32 uBTProfileId = GET_PROFILE_FROM_USERID(userId);
	assert( uBTProfileId > g_uBTProfileId_Invalid );

	TSlotMap::iterator itSlot = m_Slots.find( uBTProfileId );
	if( itSlot != m_Slots.end() )
	{
		itSlot->second->SetResourceRunning( userId );

		// Tree should run for everyone else
		CRY_ASSERT( m_pTactic );
		m_pTactic->RunTreeForMyResources( false, userId );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::IsResourceAssigned( TBSSProfileUserId userId ) const
{
	assert( userId != g_uBTUserId_Invalid );

	const uint32 uBTProfileId = GET_PROFILE_FROM_USERID(userId);
	assert( uBTProfileId > g_uBTProfileId_Invalid );

	bool bRes = false;
	TSlotMap::const_iterator itSlot = m_Slots.find( uBTProfileId );
	if( itSlot != m_Slots.end() )
	{
		bRes = itSlot->second->AssignedExists( userId );
	}

	return( bRes );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::IsRunning() const
{
	return( m_pTactic->GetState() == ETS_RUNNING );
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CSubTactic::GetAssignedUsers( std::vector<TBSSProfileUserId>* pListToFill ) const
{
	assert( pListToFill != NULL );
	assert( pListToFill->size() == 0 );

	TSlotMap::const_iterator itSlot = m_Slots.begin();
	TSlotMap::const_iterator itEnd = m_Slots.end();
	for( ; itSlot != itEnd; ++itSlot )
	{
		itSlot->second->GetAssignedResources( *pListToFill );
	}

	return( pListToFill->size() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSubTactic::ChangeState( ETacticState ENewState )
{
	TSlotMap::const_iterator itSlot = m_Slots.begin();
	TSlotMap::const_iterator itEnd = m_Slots.end();
	for( ; itSlot != itEnd; ++itSlot )
	{
		CSlot*	pSlot = itSlot->second;
		CRY_ASSERT( pSlot );

		TSubTacticRequirements::const_iterator itRequirement = m_pInfo->Requirements.find( itSlot->first );
		CRY_ASSERT( itRequirement != m_pInfo->Requirements.end() );

		SSubTacticRequirement const&	Requirement = itRequirement->second;

		switch( ENewState )
		{
		case ETS_WAITING_FOR_MORE_CANDIDATES:
			{
				pSlot->SetMinMax( Requirement.uMinResourcesToStart, Requirement.uMaxResources );
			}

			break;

		case ETS_WAITING_FOR_ACKNOWELDGE:
			{
			}

			break;

		case ETS_RUNNING:
			{
				pSlot->SetMinMax( Requirement.uMinResources, Requirement.uMaxResources );
			}

			break;
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSubTactic::Run( TBSSProfileUserId userId, bool &bMatchesGroup )
{
	assert( userId != g_uBTUserId_Invalid );

	bool bRet = false;
	bool bNewCandidate = AddCandidate( userId, bMatchesGroup );

	m_LastRun = userId;

	switch( m_pTactic->GetState() )
	{
	case ETS_WAITING_FOR_MORE_CANDIDATES:
		{
			bRet = false;
		}

		break;

	case ETS_WAITING_FOR_ACKNOWELDGE:
		{
			bool bCanEnter = false;
			if( IsResourceAssigned(userId) )
			{
				bCanEnter = (!UsesGroup() || CheckUserGroup(userId));
			}

			if ( bCanEnter == true )
			{
				OnResourceEnter( userId );
			}
		}

		break;

	case ETS_RUNNING:
		{
			bRet = IsResourceAssigned( userId );
		}

		break;
	}

	return( bRet );
}
