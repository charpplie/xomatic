////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   UiMotionParamMappings.h
//  Version:     v1.00
//  Created:     05/08/2009 by Pau Novau
//  Description: Container that creates and stores mappings between ui and
//               motion params.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/UiMotionParamMappings.h"

#include "..\LMGEditor/UiMotionParamSlider.h"
#include "..\LMGEditor/UiMotionParamJoystick.h"


CUiMotionParamMappings::CUiMotionParamMappings()
{
	for ( int i = 0; i < eMotionParamID_COUNT; i++ )
	{
		m_uiMotionParamMappings[ i ] = NULL;
	}
}


CUiMotionParamMappings::~CUiMotionParamMappings()
{
	ClearMappings();
}


void CUiMotionParamMappings::CreateMapping( EMotionParamID paramId, IUiMotionParamMapping::Mapping mapping, CTestSliderControl* pSliderControl )
{
	IUiMotionParamMapping* pMapping = new CUiMotionParamSlider( mapping, pSliderControl );
	SetMapping( paramId, pMapping );
}


void CUiMotionParamMappings::CreateMapping( EMotionParamID paramId, IUiMotionParamMapping::Mapping mapping, CTestJoystickControl* pJoystickControl, float lengthMin, float lengthMax )
{
	IUiMotionParamMapping* pMapping = new CUiMotionParamJoystick( mapping, pJoystickControl, lengthMin, lengthMax );
	SetMapping( paramId, pMapping );
}


void CUiMotionParamMappings::SetMapping( EMotionParamID paramId, IUiMotionParamMapping* pMotionParamMapping )
{
	RemoveMapping( paramId );
	m_uiMotionParamMappings[ paramId ] = pMotionParamMapping;
}


void CUiMotionParamMappings::RemoveMapping( EMotionParamID paramId )
{
	delete m_uiMotionParamMappings[ paramId ];
	m_uiMotionParamMappings[ paramId ] = NULL;
}


void CUiMotionParamMappings::ClearMappings()
{
	for ( int i = 0; i < eMotionParamID_COUNT; i++ )
	{
		delete m_uiMotionParamMappings[ i ];
		m_uiMotionParamMappings[ i ] = NULL;
	}
}


IUiMotionParamMapping* CUiMotionParamMappings::GetMapping( EMotionParamID paramId )
{
	return m_uiMotionParamMappings[ paramId ];
}