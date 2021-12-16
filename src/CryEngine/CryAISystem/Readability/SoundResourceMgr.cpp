/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   SoundResourceSet.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include "CoopReadabilitiesSystem.h"
#include "SoundResourceMgr.h"
#include "SoundResourceSet.h"

using namespace Readabilities;

CSoundResourceMgr::CSoundResourceMgr( CCoopReadabilitiesSystem* pSystem )
: m_pSystem( pSystem )
{

}

// ------------------------------------------------------------
// Description:
//	Reset instance to state just after loading static data,
//	that is, clearing 'busy' and 'used' states on all sound resources.
// ------------------------------------------------------------
void CSoundResourceMgr::Reset()
{
	tResourceSets::iterator itSet = m_mapResourceSets.begin();
	tResourceSets::iterator itEnd = m_mapResourceSets.end();
	for( ; itSet != itEnd; ++itSet )
	{
		itSet->second.Reset();
	}
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CSoundResourceSet* CSoundResourceMgr::LoadFromXmlNode( const string& sSetName, XmlNodeRef& xmlNode )
{
	CSoundResourceSet* pSet = CreateSet( sSetName );
	bool bSuccess = pSet->LoadFromXmlNode( xmlNode );

	if( bSuccess == false )
	{
		RemoveSet( pSet );
		pSet = NULL;
	}

	return pSet;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//	Newly created set or pointer to the one already existing
// ------------------------------------------------------------
CSoundResourceSet* CSoundResourceMgr::CreateSet( const string& sSetName )
{
	std::pair< tResourceSets::iterator, bool > result = m_mapResourceSets.insert( std::make_pair( sSetName, CSoundResourceSet( sSetName ) ) );

	return &(result.first->second);
}

void CSoundResourceMgr::RemoveSet( CSoundResourceSet* pSet )
{
	m_mapResourceSets.erase( pSet->GetName() );
}