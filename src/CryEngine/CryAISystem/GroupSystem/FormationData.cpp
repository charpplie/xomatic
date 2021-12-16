/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   FormationData.cpp
Description: 
---------------------------------------------------------------------
History:
- 02:04:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"
#include "FormationData.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CFormationData::CFormationData() : m_nSize( 0 )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CFormationData::~CFormationData()
{
	
}

// Description:
//
// Arguments:
//
// Return:
//
void CFormationData::Clear()
{
	m_nSize = 0;
	m_vecPoints.resize( 0 );
}

// Description:
//
// Arguments:
//
// Return:
//
void CFormationData::SetPoint( uint32 nIndex, float x, float y )
{
	if( nIndex >= m_nSize )
	{
		GetAISystem()->Error( "<CFormationData> ", "SetPoint() trying to set formation point out of range." );

		return;
	}

	// Creating 3d point from 2d offset
	m_vecPoints[ nIndex ].Set( x, y, 0.0f );
}
