/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   FormationDataManager.cpp
Description: 
---------------------------------------------------------------------
History:
- 02:04:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"
#include "FormationDataManager.h"

//----------------------------------------------------------------------------------------------//

CFormationDataManager * CFormationDataManager::s_pInstance = NULL;

//----------------------------------------------------------------------------------------------//


// Description:
//   Constructor
// Arguments:
//
// Return:
//
CFormationDataManager::CFormationDataManager()
: bInitialized( false )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CFormationDataManager::~CFormationDataManager()
{
}

// Description:
//   
// Arguments:
//
// Return:
//
CFormationDataManager * CFormationDataManager::GetInstance() 
{ 
	if (!s_pInstance) 
		s_pInstance = new CFormationDataManager();

	return s_pInstance;
}

// Description:
//   
// Arguments:
//
// Return:
//
void CFormationDataManager::ClearFormationData()
{
	m_mFormationSets.clear();
}

// Description:
//   
// Arguments:
//
// Return:
//
void CFormationDataManager::PushFormationSet( CFormationSet& formationSet )
{
	m_mFormationSets[ formationSet.GetName() ] = formationSet;
}

// Description:
//   
// Arguments:
//
// Return:
//
const CFormationData*	CFormationDataManager::GetFormationData( string sFormationSetName, int nMembersCount ) const
{
	if( nMembersCount < 2 )
	{
		return NULL;
	}

	tFormationSetsMap::const_iterator it = m_mFormationSets.find( sFormationSetName );
	
	return ( it != m_mFormationSets.end() ? it->second.GetFormationDataForSize( nMembersCount ) : NULL );		
}