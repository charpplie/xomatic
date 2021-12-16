/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySequenceMgr.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include "CoopReadabilitiesSystem.h"
#include "ReadabilitySequenceMgr.h"

using namespace Readabilities;

CReadabilitySequenceMgr::CReadabilitySequenceMgr( CCoopReadabilitiesSystem* pSystem )
: m_pSystem( pSystem )
{

}

CReadabilitySequenceSet* CReadabilitySequenceMgr::LoadFromXmlNode( const string& sSetName, XmlNodeRef& xmlNode )
{
	CReadabilitySequenceSet* pSet = CreateSet( sSetName );

	bool bRet = pSet->LoadFromXmlNode( xmlNode );

	if( bRet == false )
	{
		// if load failed remove this (potentialy invalid) set
		RemoveSet( pSet );
		pSet = NULL;

		GetAISystem()->Error( "<CReadabilitySequenceMgr::LoadFromXmlNode> ", "Failed to load readability set %s", sSetName.c_str() );
	}

	return pSet;
}

// ------------------------------------------------------------
// Description:
//	Reset instance to state just after loading static data,
//	that is, retain sequence sets and its data, but remove any signs
//	of them being used before.
// ------------------------------------------------------------
void CReadabilitySequenceMgr::Reset()
{
	tReadabilitySetsMap::iterator itSet = m_sets.begin();
	tReadabilitySetsMap::iterator itEnd = m_sets.end();

	for( ; itSet != itEnd; ++itSet )
	{
		itSet->second.Reset();
	}
}

CReadabilitySequenceSet* CReadabilitySequenceMgr::CreateSet( const string& sSetName )
{	
	std::pair< tReadabilitySetsMap::iterator, bool > result = m_sets.insert( std::make_pair( sSetName, CReadabilitySequenceSet( sSetName ) ) );
	CReadabilitySequenceSet* pRetSet = &(result.first->second);

	return pRetSet;
}

CReadabilitySequenceSet* CReadabilitySequenceMgr::GetSet( const string& sSetName )
{
	CReadabilitySequenceSet* pRetSet = NULL;
	tReadabilitySetsMap::iterator itSet = m_sets.find( sSetName );

	if( itSet != m_sets.end() )
	{
		pRetSet = &(itSet->second);
	}

	return pRetSet;
}

const CReadabilitySequenceSet* CReadabilitySequenceMgr::GetSet( const string& sSetName ) const
{
	const CReadabilitySequenceSet* pRetSet = NULL;
	tReadabilitySetsMap::const_iterator itSet = m_sets.find( sSetName );

	if( itSet != m_sets.end() )
	{
		pRetSet = &(itSet->second);
	}

	return pRetSet;
}

void CReadabilitySequenceMgr::RemoveSet( CReadabilitySequenceSet* pSet )
{
	m_sets.erase( pSet->GetName() );
}