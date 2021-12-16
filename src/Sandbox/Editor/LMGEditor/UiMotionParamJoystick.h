////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CUiMotionParamJoystick.h
//  Version:     v1.00
//  Created:     09/09/2009 by Pau Novau
//  Description: Maps Ui values from a joystick to a motion param.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Ui_Motion_Param_Joystick__h__
#define __Ui_Motion_Param_Joystick__h__
#pragma once

#include "..\LMGEditor/IUiMotionParamMapping.h"

class CTestJoystickControl;

class CUiMotionParamJoystick
	: public IUiMotionParamMapping
{
public:
	CUiMotionParamJoystick( Mapping mapping, CTestJoystickControl* pJoystickControl, float lengthMin, float lengthMax );

	virtual float GetValue( float minParamValue, float maxParamValue, float minAssetValue, float maxAssetValue ) const;

private:
	Mapping m_mapping;
	CTestJoystickControl* m_pJoystickControl;
	float m_lengthMin;
	float m_lengthMax;
};

#endif
