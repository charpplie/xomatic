/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySequenceSet.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include <algorithm>

#include "CoopReadabilitiesSystem.h"
#include "ReadabilitySequenceSet.h"

#include "BehaviorTree/dag.h"
#include "BehaviorTree/dag_node.h"

using namespace Readabilities;

CReadabilitySequenceSet::CReadabilitySequenceSet( const string& sSetName )
: m_sSetName( sSetName )
, m_nSize( 0 )
, m_bBlank( true )
, m_nAvialableLeft( 0 )
, m_pSoundResourceSet( NULL )
{
	m_itFirstFree = m_sequences.begin();
}

CReadabilitySequenceSet::CReadabilitySequenceSet( const string& sSetName, int nSize, CSoundResourceSet* pSoundResourceSet )
: m_sSetName( sSetName )
, m_nSize( 0 )	// will be set properly in call to ResizeSequenceContainer
, m_bBlank( false )
{
	ResizeSequenceContainer( nSize );

	SetSoundResourceSet( pSoundResourceSet );
}

// ------------------------------------------------------------
// Description:
//   Initializes Blank instance with given parameters
// Arguments:
//
// Return:
//	true if initialization succeeded
// ------------------------------------------------------------
bool CReadabilitySequenceSet::Init( int nSize, CSoundResourceSet* pSoundResourceSet )
{
	// if this instance isn't Blank we will return false;
	bool bRet = m_bBlank == false;

	if( m_bBlank == true )
	{
		ResizeSequenceContainer( nSize );
		
		SetSoundResourceSet( pSoundResourceSet );

		m_bBlank = false;
	}
	else
	{
		GetAISystem()->Warning( "<CReadabilitySequenceSet> ", "Trying to reinitialize CReadabilitySequenceSet instance!" );
	}

	return bRet;
}

// ------------------------------------------------------------
// Description:
//   Clears 'busy' and 'used' state on all sequences
// ------------------------------------------------------------
void CReadabilitySequenceSet::Reset()
{
	if( m_bBlank == false )
	{
		m_available.clear();

		tSequences::iterator itSequence = m_sequences.begin();
		tSequences::iterator itEnd = m_sequences.end();
		for( ; itSequence != itEnd; ++itSequence )
		{
			itSequence->Reset();
			m_available.insert( &*itSequence );
		}

		m_nAvialableLeft = m_available.size();
	}
}

// ------------------------------------------------------------
// Description:
//   Clears off all containers 
// ------------------------------------------------------------
void CReadabilitySequenceSet::Clear()
{
	if( m_bBlank == false )
	{
		m_available.clear();
		m_sequences.clear();
	}
}

// ------------------------------------------------------------
// Description:
//   Loadinf straight from xmlNode
// ------------------------------------------------------------
bool CReadabilitySequenceSet::LoadFromXmlNode( XmlNodeRef& xmlNode )
{
	CDag	dag("set");
	// make sure empty rows in excel xml won't interrupt reading data in
	dag.SetSkipEmptyRows( true );

	uint32		uRows = 0;
	uint32		uColumns = 0;

	xmlNode->getAttr( "ss:ExpandedRowCount", uRows );
	xmlNode->getAttr( "ss:ExpandedColumnCount", uColumns );

	bool bSuccess = dag.LoadSheet( "dag", uRows, uColumns, xmlNode );

	if( bSuccess == true )
	{
		// there is data to grab, so we clean out the old data
		Clear();

		CDagNode* pRoot = dag.GetRoot();
		uint32 uChildCount = pRoot->GetChildCount();
		
		CReadabilitySequence* pSequence = NULL;

		// now that number of children is known we can initialize sequence set
		Init( uChildCount, NULL );

		// every child of this root node represents separate readability sequence		
		for( uint32 i = 0; i < uChildCount && bSuccess == true; ++i )
		{
			pSequence = GetNewSequence();
			bSuccess &= pSequence->LoadFromDagNode( pRoot->GetChild( i ) );
			if( bSuccess == true )
			{
				
			}
		}
	}
	else
	{
		GetAISystem()->Error( "<CReadabilitySequenceSet::LoadFromXmlNode> ", "Failed to load readability set %s", m_sSetName.c_str() );
	}

	return bSuccess;
}

// ------------------------------------------------------------
// Description:
//		Removes given sequence from available sequences list and
//		block given readability sequence
// Arguments:
//	pSession - session governing playing sequence
//	pSequence	- sequence to be blocked
// ------------------------------------------------------------
void CReadabilitySequenceSet::StartUsingSequence( CReadabilitySession* pSession, CReadabilitySequence* pSequence )
{
	pSequence->Block( pSession->GetId(), true );

	uint32 nRemoved = m_available.erase( pSequence );
	assert( nRemoved <= 1 && "Removed more than one element from available readability sequences set!" );
	m_nAvialableLeft -= nRemoved;

	m_currentlyPlaying.insert( pSequence );
}

// ------------------------------------------------------------
// Description:
//   Removes given sequence from available sequences list
// Arguments:
//	pSession - session governing playing sequence
//	pSequence	- sequence to be blocked
// ------------------------------------------------------------
void CReadabilitySequenceSet::StopUsingSequence( CReadabilitySession* pSession, CReadabilitySequence* pSequence )
{
	pSequence->Block( pSession->GetId(), false );
	m_currentlyPlaying.erase( pSequence );
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CReadabilitySequence*	CReadabilitySequenceSet::GetNewSequence()
{
	assert( m_bBlank == false && m_sequences.size() > 0 && "Trying to get new CReadabilitySequence from blank set!" );
	assert( m_sequences.size() && m_sequences.end() - m_itFirstFree >= 0 && "CReadabilitySequenceSet Free iterator is invalid!" );

	CReadabilitySequence*	pSequence = NULL;

	// add new sequences only if there's still space in vector (no growing!)
	if( m_sequences.size() > 0 && m_itFirstFree != m_sequences.end() )
	{		
		// use first free space
		pSequence = &(*m_itFirstFree);
		pSequence->SetParentSet( this );
		++m_itFirstFree;

		// add it to available sequences. 
		// NOTE: it's still blank.
		std::pair< tSequenceSet::iterator, bool > result = m_available.insert( pSequence );

		// it was really added
		if( result.second == true )
		{
			++m_nAvialableLeft;
		}
	}
	else
	{
		GetAISystem()->Warning( "<CReadabilitySequenceSet> ", "Tried to add new sequence to a full CReadabilitySequenceSet instance!" );
	}
	
	return pSequence;
}

// ------------------------------------------------------------
// Description:
//	Returns an available sequence (one that can be played, ie that's not busy or already played) 
//   NOTE: we're not marking selected 
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CReadabilitySequence*	CReadabilitySequenceSet::GetAvailableSequence( int nMaxActors )
{
	CReadabilitySequence* pRetSequence = NULL;

	assert( m_bBlank == false && "CReadabilitySequenceSet instance not initialized!" );
	//assert( false && "m_nAvialableLeft not initialized!" );
	assert( m_available.size() == m_nAvialableLeft && "Available sequences number is invalid!" );

	if( nMaxActors < 2 )
	{
		GetAISystem()->Warning( "<CReadabilitySequenceSet> ", "System currently doesn't handle readabilities for one actor." );
	}
	else if( m_nAvialableLeft < 1 )
	{
		GetAISystem()->Warning( "<CReadabilitySequenceSet> ", "No more available sequences in set \"%s\"", m_sSetName.c_str() );
	}
	else if( m_nAvialableLeft == 1 )
	{
		pRetSequence = *m_available.begin();
		// check if there's at least one path of not used resources in this sequence
		bool bPlayable = CheckSequencePlayability( pRetSequence ); 
		if( bPlayable == false )
		{
			// unfortunately nothing to return;
			pRetSequence = NULL;
		}
	}
	else
	{	
		// there's more than one sequence available so we're picking one by random				
		// NOTE: Naive, inefficient implementation
		do
		{			
			int nIndex = cry_rand() % m_nAvialableLeft;
			tSequenceSet::iterator itSearch = m_available.begin();
			while( nIndex-- > 0 )
				++itSearch;
			
			pRetSequence = *itSearch;

			// unfortunately it can happen that a sequence selected from
			// list of available is no more available. We need to double check
			// on that doing a test, and id it fails, select again
		} while ( CheckSequencePlayability( pRetSequence ) == false && m_nAvialableLeft > 0 );

		// unfortunately no available sequence has been found
		if( m_nAvialableLeft == 0 )
		{
			pRetSequence = NULL;
		}
	}
	
	assert( (pRetSequence == NULL || pRetSequence->IsBlocked() == false) && "Selected available sequence is already blocked!" );

	return pRetSequence;
}

void CReadabilitySequenceSet::SetSoundResourceSet( CSoundResourceSet* pSoundResourceSet )
{
	m_pSoundResourceSet = pSoundResourceSet;

	std::vector<CReadabilitySequence>::iterator itC = m_sequences.begin();
	std::vector<CReadabilitySequence>::iterator itEnd = m_sequences.end();
	for( ; itC != itEnd; ++itC )
	{
		itC->SetSoundResourceSet( pSoundResourceSet );
	}
}


void CReadabilitySequenceSet::ResizeSequenceContainer( int nSize )
{
	m_nSize = nSize;
	m_sequences.resize( nSize );

	// this iterator is safe. This is the only place m_sequences is allowed to grow
	m_itFirstFree = m_sequences.begin();
}

// ------------------------------------------------------------
// Description:
//		Checks if given sequence contains at least one path of not used resources
//		NOTE: if given sequence fails the test it's also removed from available list 
//		(if it's there)
// Arguments:
//		pSequence	- sequence to check
// Return:
//		test result
// ------------------------------------------------------------
bool CReadabilitySequenceSet::CheckSequencePlayability( CReadabilitySequence* pSequence )
{
	bool bRet = pSequence->CheckPlayability();

	if( bRet == false )
	{		
		tSequenceSet::iterator itSet = m_available.find( pSequence );
		// if this sequence is in available set
		if( itSet != m_available.end() )
		{
			// this sequence isn't available any more. Mark it.
			m_available.erase( itSet );
			--m_nAvialableLeft;
		}
	}

	return bRet;
}