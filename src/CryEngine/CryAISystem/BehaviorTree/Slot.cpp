/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   Slot.h
$Id$
$DateTime$
Description: Slots deal with resources (in this case, entities) and
distribute them following some rules and steps:
1. You can add / remove "candidates" to enter a certain slot
2. Each slot have a Minimum, Maximum, and Minimum-to-run candidates
3. Once there are enough candidates, they will be "assigned" and
the slot will be considered to be "running". The resources will
then markes as "used". No other slot can use them.
4. The resources will be sorted before assigned, using a user-specified
LUA code and a simple qsort.
5. If the resources in the slot go below "Min-to-run" the slot will
"stop" and "free" the resources.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "Slot.h"
#include "SlotGroup.h"
#include "SlotData.h"

#include "ProfileDictionary.h"

#define LAST_SORT_SCRIPT_VAR "LastSortCheck"

/*$1-  -----------------------------------------------------------------------*/
static const char* s_sUserTableAlias1 = "userTbl";
static const char* s_sUserTableAlias2 = "userTbl2";
static int s_iScriptCode;

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSlot::CSlot( CSlotGroup* pGroup )
: m_uMin( 1 )
, m_uMax( 1 )
, m_iSortCode( 0 )
, m_bCandidatesOrdered( false )
, m_pGroup( pGroup )
, m_pCandidates( NULL )
, m_pIdealCandidates( NULL )
, m_pAssigned( NULL )
, m_pRunning( NULL )
{
	assert( pGroup != NULL );

	m_pCandidates = new CSlotData;
	m_pIdealCandidates = new CSlotData;
	m_pAssigned = new CSlotData;
	m_pRunning = new CSlotData;
}

// Description:
//
// Arguments:
//
// Return:
//
CSlot::~CSlot()
{
	SAFE_DELETE( m_pCandidates );
	SAFE_DELETE( m_pIdealCandidates );
	SAFE_DELETE( m_pAssigned );
	SAFE_DELETE( m_pRunning );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::Reset()
{
	m_pCandidates->Clear();
	m_pAssigned->Clear();
	m_pRunning->Clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetIdealCandidatesAsString( string& sString ) const
{
	m_pIdealCandidates->GetAsString( sString );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetCandidatesAsString( string& sString ) const
{
	m_pCandidates->GetAsString( sString );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetAssignedAsString( string& sString ) const
{
	m_pAssigned->GetAsString( sString );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetRunningAsString( string& sString ) const
{
	m_pRunning->GetAsString( sString );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::AddCandidate( TSlotType typeId )
{
	return( m_pCandidates->Add(typeId) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::Delete( TSlotType typeId )
{
	bool bRet = false;

	bRet |= m_pCandidates->Delete( typeId );
	bRet |= m_pAssigned->Delete( typeId );
	bRet |= m_pRunning->Delete( typeId );

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::DeleteAssigned( TSlotType typeId )
{
	return( m_pAssigned->Delete(typeId) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::HaveCandidates() const
{
	return( m_pCandidates->Count() > 0 );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::HaveEnoughCandidates() const
{
	return( m_pCandidates->Count() >= m_uMin );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::HaveEnoughAssigned() const
{
	return( m_pAssigned->Count() >= m_uMin );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::HaveEnoughRunning() const
{
	return( m_pRunning->Count() >= m_uMin );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::CandidateExists( TSlotType typeId ) const
{
	return( m_pCandidates->Exists(typeId) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::AssignedExists( TSlotType typeId ) const
{
	return( m_pAssigned->Exists(typeId) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::RunningExists( TSlotType typeId ) const
{
	return( m_pRunning->Exists(typeId) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::StartAssignProcess()
{
	m_bCandidatesOrdered = false;
	m_pIdealCandidates->Clear();
}

// Description:
//
// Arguments:
//
// Return:
//    True is something changed in assigned list
bool CSlot::EndAssignProcess()
{
	assert( m_pIdealCandidates->Count() <= m_uMax );
	return( m_pIdealCandidates->DeleteSharedAndCopyNew(m_pAssigned) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::AssignForcedResources()
{
	bool bRet = false;

	if( m_uMin > 0 && m_pIdealCandidates->Count() == 0 && m_pCandidates->Count() == m_uMin )
	{
		m_pIdealCandidates->CopyFrom( m_pCandidates, m_uMin );
		bRet = true;
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::AssignMaxResources()
{
	bool bRet = false;
	uint32 uLeftToFill = m_uMax - m_pIdealCandidates->Count();
	uint32 uAvailableCandidates = m_pCandidates->Count() - m_pIdealCandidates->Count();

	if( uLeftToFill > 0 && uAvailableCandidates > 0 )
	{
		if( m_bCandidatesOrdered == false )
		{
			SortCandidates();
		}

		bRet = AssignCandidates( m_uMax - m_pIdealCandidates->Count() );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::AssignMinResources()
{
	bool bRet = false;

	if( m_pIdealCandidates->Count() < m_uMin )
	{
		if( m_bCandidatesOrdered == false )
		{
			SortCandidates();
		}

		bRet = AssignCandidates( m_uMin - m_pIdealCandidates->Count() );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::AssignCandidates( uint32 uAmount )
{
	bool bRet = false;
	uint32 uCount = 0;
	TSlotTypeList::const_iterator it = m_pCandidates->GetResources().begin();

	while( it != m_pCandidates->GetResources().end() && (uAmount > 0 && uCount < uAmount) )
	{
		if( m_pGroup->IsResourceAssigned(*it, this) == false )
		{
			if( m_pIdealCandidates->Add(*it) )
			{
				bRet |= true;
				++uCount;
			}
		}

		++it;
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::DeAssignResources( bool bOnlyAssigned )
{
	bool bRet = false;

	bRet |= m_pAssigned->Clear();

	if( bOnlyAssigned == false )
	{
		bRet |= m_pRunning->Clear();
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::SetResourceRunning( TSlotType typeId )
{
	bool bRet = false;

	if( m_pAssigned->Exists(typeId) == true )
	{
		bRet = true;
		m_pRunning->Add( typeId );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::RemoveResourceRunning( TSlotType typeId )
{
	return( m_pRunning->Delete(typeId) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetNewAssignedResources( TSlotTypeList& vecResources ) const
{
	vecResources.insert(
		vecResources.end(),
		m_pIdealCandidates->GetResources().begin(),
		m_pIdealCandidates->GetResources().end() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetCandidateResources( TSlotTypeList& vecResources ) const
{
	vecResources.insert( vecResources.end(), m_pCandidates->GetResources().begin(), m_pCandidates->GetResources().end() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::GetAssignedResources( TSlotTypeVector& vecResources ) const
{
	vecResources.insert( vecResources.end(), m_pAssigned->GetResources().begin(), m_pAssigned->GetResources().end() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::SetSortScriptCode( const string& sCode )
{
	assert( sCode.empty() == false );

	string sFinalCode;
	sFinalCode.Format(
		"local a = userTbl; local b = userTbl2; return %s;",
		sCode.c_str() );
	m_iSortCode = gEnv->pScriptSystem->PreCacheBuffer( sFinalCode.c_str(), sFinalCode.size(), "AI Candidate Sort Script" );

	assert( m_iSortCode > 0 );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSlot::SetMinMax( uint32 uMin, uint32 uMax )
{
	m_uMin = uMin;
	m_uMax = uMax;

	if( m_uMax != 0 && m_uMin > m_uMax )
	{
		assert( 0 && "Inconsistency in CSlot min and max properties" );
		m_uMax = m_uMin;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool SortCandidate( TSlotType a, TSlotType b )
{
	bool bRet = false;
	IScriptSystem *pScript = gEnv->pScriptSystem;

	// Set the 2 user tables
	{
		SBTProfileUser *pUser = GetAISystem()->GetBTProfileDictionary()->GetProfileUser(a);
		if (pUser && pUser->m_pUser)
		{
			IScriptTable *pTable = pUser->m_pUser->GetBTUserTable();
			pScript->SetGlobalValue(s_sUserTableAlias1, pTable);
		}
	}
	{
		SBTProfileUser *pUser = GetAISystem()->GetBTProfileDictionary()->GetProfileUser(b);
		if (pUser && pUser->m_pUser)
		{
			IScriptTable *pTable = pUser->m_pUser->GetBTUserTable();
			pScript->SetGlobalValue(s_sUserTableAlias2, pTable);
		}
	}

	pScript->BeginPreCachedBuffer( s_iScriptCode );
	if( pScript->EndCall(bRet) == false )
	{
		AIError( "CSlot::SortCandidate - Error executing script code for sorting tactic candidates" );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSlot::SortCandidates()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	bool bRet = false;

	if( m_pCandidates->Count() > m_uMin && m_iSortCode > 0 )
	{
		s_iScriptCode = m_iSortCode;
		m_pCandidates->Sort( SortCandidate );
		bRet = true;
	}

	m_bCandidatesOrdered = true;

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CSlot::GetMin() const
{
	return( m_uMin );
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CSlot::GetMax() const
{
	return( m_uMax );
}
