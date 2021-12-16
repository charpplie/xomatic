/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   TacticInfo.cpp
$Id$
$DateTime$
Description: Read-only configuration storage attached to each subtactic.
It just contains all the data need by a subtatic to run, like min and max
resources allowed.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "TacticInfo.h"
#include "ProfileDictionary.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SSubTacticInfo::SSubTacticInfo( const string& sName, CTacticInfo const* pInfo )
: pInfo( pInfo )
, sName( sName )
{
	assert( sName.empty() == false );
	assert( pInfo != NULL );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SSubTacticRequirement::SSubTacticRequirement()
: uMinResourcesToStart( 1 )
, uMinResources( 1 )
, uMaxResources( 0 )
{
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CTacticInfo::CTacticInfo( const char* sName )
: m_bUsesGroup( false )
, m_uMaxInstances( 1 )
, m_uMaxGroupInstances( 1 )
, m_uLastBTProfileId( g_uBTProfileId_Invalid )
, m_eDestructor( EBWNER_INVALID )
, m_pLastSubInfo( NULL )
, m_sName( sName )
{
	assert( sName != NULL );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CTacticInfo::~CTacticInfo()
{
	for( uint32 uIndex = 0; uIndex < m_vecSubTacticsInfo.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecSubTacticsInfo[uIndex] );
	}

	m_vecSubTacticsInfo.clear();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
const string& CTacticInfo::GetName() const
{
	return( m_sName );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
const SSubTacticInfo* CTacticInfo::GetSubTacticInfo( const string& sName ) const
{
	const SSubTacticInfo *pRes = NULL;

	for( uint32 uIndex = 0; uIndex < m_vecSubTacticsInfo.size(); ++uIndex )
	{
		if( m_vecSubTacticsInfo[uIndex]->sName.compareNoCase(sName) == 0 )
		{
			pRes = m_vecSubTacticsInfo[uIndex];
			break;
		}
	}

	return( pRes );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
const CTacticInfo::TBTProfileReferences& CTacticInfo::GetBTProfileReferences() const
{
	return( m_vecBTProfileReferences );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
const bool CTacticInfo::UsesGroup() const
{
	return( m_bUsesGroup );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
const bool CTacticInfo::ExceedsMaxGroupInstances( uint32 uNumInstances ) const
{
	return ( m_uMaxGroupInstances > 0 && uNumInstances > m_uMaxGroupInstances );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CTacticInfo::LoadCell( uint32 uCell, const string& sData, bool bIsRequirements )
{
	return( bIsRequirements ? LoadCell_Requirement(uCell, sData) : LoadCell_Tactic(uCell, sData) );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CTacticInfo::FindOrCreateSubInfo( const string& sName, SSubTacticInfo *& pOutSubTacticInfo )
{
	pOutSubTacticInfo = NULL;

	TSubTacticVector::iterator itC = m_vecSubTacticsInfo.begin();
	TSubTacticVector::iterator itEnd = m_vecSubTacticsInfo.end();

	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->sName.compareNoCase(sName) == 0 )
		{
			pOutSubTacticInfo = *itC;
			return;
		}
	}

	// Make a new one
	SSubTacticInfo *pInfo = new SSubTacticInfo( sName, this );
	m_vecSubTacticsInfo.push_back( pInfo );
	pOutSubTacticInfo = m_vecSubTacticsInfo.back();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CTacticInfo::LoadCell_Tactic( uint32 uCell, const string& sData )
{
	bool bRet = true;

	switch( uCell )
	{
	case 2:
		{
			if( sData.compareNoCase("max") == 0 )
			{
				m_uMaxInstances = 0;
			}
			else
			{
				m_uMaxInstances = atoi( sData );
			}
		}

		break;

	case 3:
		{
			m_bUsesGroup = atoi( sData ) ? true : false;
		}

		break;

	case 4:
		{
			if( sData.compareNoCase("max") == 0 )
			{
				m_uMaxGroupInstances = 0;
			}
			else
			{
				m_uMaxGroupInstances = atoi( sData );
			}
		}

		break;

	case 5:
		{
			if( sData.compareNoCase("DESTROY_TACTIC") == 0 )
			{
				m_eDestructor = EBWNER_DESTROY_TACTIC;
			}
			else
			{
				m_eDestructor = EBWNER_INVALID;
			}
		}

		break;

	case 6:
		{
			FindOrCreateSubInfo( sData, m_pLastSubInfo );
			CRY_ASSERT( m_pLastSubInfo );
		}

		break;

	default:
		m_pLastSubInfo = NULL;
		bRet = false;
		break;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CTacticInfo::LoadCell_Requirement( uint32 uCell, const string& sData )
{
	bool bRet = true;

	switch( uCell )
	{
	case 1:
		{
			FindOrCreateSubInfo( sData, m_pLastSubInfo );
			CRY_ASSERT( m_pLastSubInfo );
		}

		break;

	case 2:
		{
			m_uLastBTProfileId = GetAISystem()->GetBTProfileDictionary()->GetProfileId( sData );
			if( m_uLastBTProfileId == g_uBTProfileId_Invalid )
			{
				AIError(
					"CTacticInfo::LoadCell_Requirement() Requirement for tactic %s uses undefined tree %s",
					m_pLastSubInfo->sName.c_str(),
					sData.c_str() );
			}
			stl::push_back_unique(m_vecBTProfileReferences, m_uLastBTProfileId);
		}

		break;

	case 3:
		{
			m_pLastSubInfo->Requirements[m_uLastBTProfileId].uMinResourcesToStart = MAX( atoi(sData), 1 );
		}

		break;

	case 4:
		{
			m_pLastSubInfo->Requirements[m_uLastBTProfileId].uMinResources = MIN(
				(uint32)atoi(sData),
				m_pLastSubInfo->Requirements[m_uLastBTProfileId].uMinResourcesToStart );
		}

		break;

	case 5:
		{
			m_pLastSubInfo->Requirements[m_uLastBTProfileId].uMaxResources = MAX(
				(uint32)atoi(sData),
				m_pLastSubInfo->Requirements[m_uLastBTProfileId].uMinResources );
		}

		break;

	case 6:
		{
			m_pLastSubInfo->Requirements[m_uLastBTProfileId].sQSortCallback = sData;
		}

		break;

	default:
		m_pLastSubInfo = NULL;
		m_uLastBTProfileId = g_uBTProfileId_Invalid;
		bRet = false;
		break;
	}

	return( bRet );
}
