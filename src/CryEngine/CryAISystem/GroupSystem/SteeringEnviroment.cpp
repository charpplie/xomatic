/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   SteeringEnviroment.cpp
Description: 
---------------------------------------------------------------------
History:
- 11:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#include "StdAfx.h"
#include "SteeringEnviroment.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSteeringEnviroment::CSteeringEnviroment() : m_bInit( false )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CSteeringEnviroment::~CSteeringEnviroment()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CSteeringEnviroment::Init()
{
	m_bInit = true;
	return( true );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CSteeringEnviroment::DebugDraw() const
{
}
