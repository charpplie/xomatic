////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestJoystickControl.h
//  Version:     v1.00
//  Created:     04/09/2009 by Pau Novau
//  Description: Control that displays a knob that the user can drag.
//               The vector returned has always a length between [0, 1].
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Test_Joystick_Control__h__
#define __Test_Joystick_Control__h__
#pragma once

#include "TestBaseControl.h"

class CTestJoystickControl
	: public CTestBaseControl
{
	DECLARE_DYNCREATE( CTestJoystickControl )
public:
	CTestJoystickControl();
	virtual ~CTestJoystickControl();

	float GetLength() const;
	float GetRadiansToYAxis() const;

	Vec2 GetTransformedLogicalKnobPosition() const;
	Vec2 GetValue() const;
	void SetValue( const Vec2& value );

protected:
	virtual Vec2 FilterKnobPosition( const Vec2& unfliteredPosition );

	virtual void Draw( Gdiplus::Graphics& graphics );

};

#endif
