/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ActorResourceMgr.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include <algorithm>

#include "CoopReadabilitiesSystem.h"
#include "ActorResourceMgr.h"

using namespace Readabilities;

// ---------------------------------------------------------------- //
// std::for_each helper class      																	//
// ---------------------------------------------------------------- //
struct SNotBusyCounter
{	
	SNotBusyCounter( int& nExternalCounter ) : nCount( nExternalCounter )
	{}

	void operator ( ) ( const SActorResource* pRes ) 
	{
		if( pRes->bBusy == false )
		{
			++nCount;
		}
	}

	int& nCount;
};

// ---------------------------------------------------------------- //
// CActorResourceMgr implementation																	//
// ---------------------------------------------------------------- //

CActorResourceMgr::CActorResourceMgr( CCoopReadabilitiesSystem* pSystem )
: m_pSystem( pSystem )
{

}

CActorResourceMgr::~CActorResourceMgr()
{

}

// ------------------------------------------------------------
// Description:
//	Clears all the data allowing actors register for readabilities
//	as soon as new game session starts
// ------------------------------------------------------------
void CActorResourceMgr::Reset()
{
	m_allActors.clear();
	m_mapSetToActorRefs.clear();
}

void CActorResourceMgr::RegisterForReadabilitySet( const string& sSetName, EntityId nEntityId )
{
	// first check if this actor has already been registered with this CActorResourceMgr instance
	tActorResources::iterator	itActor = m_allActors.find( nEntityId );
	if( itActor == m_allActors.end() )
	{
		SActorResource actorRes;
		actorRes.nEntityId = nEntityId;

		itActor = m_allActors.insert( std::make_pair( nEntityId, actorRes ) ).first;
	}

	SActorResource* pActorResource = &itActor->second;

	// get access to present or just created actor resource pointers set
	// (feature of std::map)
	tSetToActorRefs::iterator itSet = m_mapSetToActorRefs.insert( std::make_pair( sSetName, tActorResourceRefs() ) ).first;		
	
	// add this actor as a resource to this set. No check for being registered before is done
	// since tActorResourceRefs is a std::set and takes care of that on its own
	itSet->second.insert( pActorResource );
}

void CActorResourceMgr::UnregisterFromReadabilitySet( const string& sSetName, EntityId nEntityId )
{
	// find resource representing this entity
	tActorResources::iterator	itActor = m_allActors.find( nEntityId );
	
	// if this entity was registered:
	if( itActor != m_allActors.end() )
	{
		// find named readability
		tSetToActorRefs::iterator itSet = m_mapSetToActorRefs.find( sSetName );
		if( itSet != m_mapSetToActorRefs.end() )
		{
			// and if it exists remove given actor resource pointer from it
			itSet->second.erase( &(itActor->second) );
		}
	}
}

bool CActorResourceMgr::IsRegisteredFor( const string& sSetName, EntityId nEntityId ) const
{
	bool bRegistered = false;

	const SActorResource* pActor = GetActorResource( nEntityId );
	if( pActor != NULL )
	{
		bRegistered = IsRegisteredFor( sSetName, const_cast<SActorResource*>(pActor) );
	}
	
	return bRegistered;
}

bool CActorResourceMgr::IsRegisteredFor( const string& sSetName, SActorResource* pActor ) const
{
	bool bRegistered = false;
	// find named readability
	tSetToActorRefs::const_iterator itSet = m_mapSetToActorRefs.find( sSetName );
	if( itSet != m_mapSetToActorRefs.end() )
	{
		// and check if given actor is in it			
		tActorResourceRefs::const_iterator itFind = itSet->second.find( pActor );
		bRegistered = ( itFind != itSet->second.end() );
	}

	return bRegistered;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//	number of actors registered for given readability set
// -----------------------------------------------------------
int CActorResourceMgr::GetRegisteredForSetCount( const string& sSetName ) const
{
	int nCount = 0;
	tSetToActorRefs::const_iterator itSet = m_mapSetToActorRefs.find( sSetName );

	if( itSet != m_mapSetToActorRefs.end() )
	{
		nCount = (int)itSet->second.size();
	}
	else
	{
		GetAISystem()->Warning( "<CActorResourceMgr> ", "No resources registered for readability \"%s\"", sSetName.c_str() );
	}

	return nCount;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//	number of actors registered for given readability set and not being marked as 'busy'
// -----------------------------------------------------------
int CActorResourceMgr::GetNotBusyForSetCount( const string& sSetName ) const
{
	int nCount = 0;
	tSetToActorRefs::const_iterator itSet = m_mapSetToActorRefs.find( sSetName );
		
	if( itSet != m_mapSetToActorRefs.end() )
	{
		const tActorResourceRefs& actorSet = itSet->second;
		// will store result in our return variable (i.e. nCount is passed by reference)
		std::for_each( actorSet.begin(), actorSet.end(), SNotBusyCounter( nCount ) );		
	}
	else
	{
		GetAISystem()->Warning( "<CActorResourceMgr> ", "No resources registered for readability \"%s\"", sSetName.c_str() );
	}

	return nCount;
}

SActorResource* CActorResourceMgr::GetActorResource( EntityId nEntityId )
{
	SActorResource* pActor = NULL;
	tActorResources::iterator	itActor = m_allActors.find( nEntityId );

	// if this entity was registered:
	if( itActor != m_allActors.end() )
	{
		pActor = &(itActor->second);
	}

	return pActor;
}

const SActorResource*	CActorResourceMgr::GetActorResource( EntityId nEntityId ) const
{
	const SActorResource* pActor = NULL;
	tActorResources::const_iterator	itActor = m_allActors.find( nEntityId );

	// if this entity was registered:
	if( itActor != m_allActors.end() )
	{
		pActor = &(itActor->second);
	}

	return pActor;
}

// ------------------------------------------------------------
// Description:
//  Fills given vector with pointers to actor that are registered for given set and are not busy.
//	Pointer to sorting function has been passed then the resulting vector is also sorted (if not 
//	NO default sort is performed).
// Arguments:
//	sSetName	- set name
//	outCandidates - vector that will be filled with available candidates' pointers. Previous 
//									content of this vector will be overwritten
//	pSortFun	- pointer to sort function. If not specified no default sort is performed.
// ------------------------------------------------------------
void CActorResourceMgr::GetCandidates( const string& sSetName, std::vector< SActorResource* >& outCandidates, tSortFuncPtr pSortFun )
{
	tSetToActorRefs::iterator itSet = m_mapSetToActorRefs.find( sSetName );

	if( itSet != m_mapSetToActorRefs.end() && itSet->second.size() > 0 )
	{
		// get proper actor set
		tActorResourceRefs& resourceSet = itSet->second;
		tActorResourceRefs::iterator itElement = resourceSet.begin();
		tActorResourceRefs::iterator itEnd = resourceSet.end();

		// resize outgoing candidate vector (for efficiency) 
		outCandidates.resize( resourceSet.size() );
	
		int nFreeCount = 0;

		// iterate through all candidates in this actor set 
		for( std::vector< SActorResource* >::iterator itCandidates = outCandidates.begin(); itElement != itEnd; ++itElement )
		{
			// if given actor resource if free than push it to resulting candidate list
			if( (*itElement)->bBusy == false )
			{
				(*itCandidates) = (*itElement);
				++itCandidates;
				++nFreeCount;
			}			
		}

		// shrink candidate vector to available resources cound
		outCandidates.resize( nFreeCount );
		
		// and sort resulting vector
		if( pSortFun != NULL )
		{
			std::sort( outCandidates.begin(), outCandidates.end(), pSortFun );
		}
	}
}