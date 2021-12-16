////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CUiMotionParamJoystick.cpp
//  Version:     v1.00
//  Created:     09/09/2009 by Pau Novau
//  Description: Maps Ui values from a joystick to a motion param.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/UiMotionParamJoystick.h"

#include "..\LMGEditor/TestJoystickControl.h"


CUiMotionParamJoystick::CUiMotionParamJoystick( Mapping mapping, CTestJoystickControl* pJoystickControl, float lengthMin, float lengthMax  )
: m_mapping( mapping )
, m_pJoystickControl( pJoystickControl )
, m_lengthMin( lengthMin )
, m_lengthMax( lengthMax )
{
	assert( m_pJoystickControl != NULL );
}


float CUiMotionParamJoystick::GetValue( float minParamValue, float maxParamValue, float minAssetValue, float maxAssetValue ) const
{
	float paramRange = maxParamValue - minParamValue;
	float assetRange = maxAssetValue - minAssetValue;
	float lengthRange = m_lengthMax - m_lengthMin;

	switch ( m_mapping )
	{
	case LENGTH_TIMES_PARAM_RANGE:
		return minParamValue + ( m_pJoystickControl->GetLength() * paramRange );

	case LENGTH_TIMES_ASSET_RANGE:
		return minAssetValue + ( m_pJoystickControl->GetLength() * assetRange );

	case LENGTH_TIMES_RANGE:
		return m_lengthMin + ( m_pJoystickControl->GetLength() * lengthRange );

	case ANGLE_Y_AXIS:
		return m_pJoystickControl->GetRadiansToYAxis();

	default:
		break;
	}

	return 0;
}