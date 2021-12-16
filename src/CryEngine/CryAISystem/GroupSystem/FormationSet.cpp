/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   FormationSet.cpp
Description: 
---------------------------------------------------------------------
History:
- 02:04:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"
#include "FormationSet.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CFormationSet::CFormationSet( int nMaxSize, const char* pszName )
{
	m_vecFormations.resize( nMaxSize ); 

	if( pszName )
	{
		m_sName = pszName;
	}
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CFormationSet::CFormationSet()
	: m_sName( "" )
{	
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CFormationSet::~CFormationSet()
{
}

// Description:
//   
// Arguments:
//
// Return:
//
void CFormationSet::SetName( const char* pszName )
{
	if( pszName )
	{
		m_sName = pszName;
	}
}

// Description:
//   
// Arguments:
//
// Return:
//
bool CFormationSet::CreateFormationSpec( uint32 nFormationSize, std::list<float> lstCoords )
{
	if( nFormationSize < 2 )
	{
		GetAISystem()->Error( "<CFormationSet> ", "CreateFormationSpec() trying to create formation of less than 2." );

		return( false );
	}

	CFormationData& formation = m_vecFormations[nFormationSize-1];
	formation.Clear();
	formation.SetSize( nFormationSize );

	int i = 1;	
	std::list<float>::iterator itC = lstCoords.begin();
	std::list<float>::iterator itEnd = lstCoords.end();
	float x;
	for( ; itC != itEnd; ++itC, ++i )
	{
		if( i % 2 != 0)
		{
			x = *itC;
		}
		else
		{
			formation.SetPoint( i/2 - 1, x, *itC );
		}		
	}

	return( true );
}
