////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestJoystickControl.cpp
//  Version:     v1.00
//  Created:     04/09/2009 by Pau Novau
//  Description: Control that displays a knob that the user can drag.
//               The vector returned has always a length between [0, 1].
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "LMGEditor/TestJoystickControl.h"

#define CLAMP_MIN_FACTOR 0.3f

IMPLEMENT_DYNCREATE( CTestJoystickControl, CTestBaseControl )

CTestJoystickControl::CTestJoystickControl()
{
	SetValue( Vec2( 0, 0 ) );
}


CTestJoystickControl::~CTestJoystickControl()
{

}



float CTestJoystickControl::GetLength() const
{
	Vec2 value = GetValue();
	float length = value.GetLength();
	return length;
}


float CTestJoystickControl::GetRadiansToYAxis() const
{
	const Vec2 Y_AXIS( 0, 1 );
	Vec2 value = GetTransformedLogicalKnobPosition();
	float angle = acos( value.GetNormalizedSafe().Dot( Y_AXIS ) );
	return ( value.x < 0 ) ? angle : -angle;
}



Vec2 CTestJoystickControl::GetValue() const
{
	Vec2 v = GetTransformedLogicalKnobPosition();
	Vec2 b = v.GetNormalizedSafe( Vec2( 0, 1 ) ) * CLAMP_MIN_FACTOR;
	Vec2 l = v - b;
	Vec2 out = l / ( 1 - CLAMP_MIN_FACTOR );
	return out;
}

Vec2 CTestJoystickControl::GetTransformedLogicalKnobPosition() const
{
	Vec2 knobPosition = GetKnobLogicalPosition();
	Vec2 v = ( knobPosition * 2 ) - Vec2( 1, 1 );
	return v;
}

void CTestJoystickControl::SetValue( const Vec2& value )
{
	Vec2 v = value;
	if ( v.GetLength() < CLAMP_MIN_FACTOR )
	{
		v.NormalizeSafe( Vec2( 0, 1 ) );
		v *= CLAMP_MIN_FACTOR;
	}

	Vec2 logicalPostion = Vec2( 0.5, 0.5 ) + ( v * 0.5 );

	SetKnobLogicalPosition( logicalPostion );
}



Vec2 CTestJoystickControl::FilterKnobPosition( const Vec2& unfliteredPosition )
{
	Vec2 center( 0.5f, 0.5f );
	Vec2 v = ( unfliteredPosition - center ) * 2;

	if ( 1 < v.GetLength() )
	{
		v.Normalize();
	}

	if ( v.GetLength() < CLAMP_MIN_FACTOR )
	{
		v.NormalizeSafe( Vec2( 0, 1 ) );
		v *= CLAMP_MIN_FACTOR;
	}

	v = v / 2;
	Vec2 filteredPosition = center + v;
	return filteredPosition;
}



void CTestJoystickControl::Draw( Gdiplus::Graphics& graphics )
{
	DrawDefaultBackground( graphics );

	CRect rect;
	GetClientRect( &rect );
	Gdiplus::Rect rc( rect.left, rect.top, rect.Width() - 1, rect.Height() - 1 );

	Gdiplus::SolidBrush validAreaBrush( Gdiplus::Color( 132, 132, 132 ) );
	graphics.FillEllipse( &validAreaBrush, rc );

	float centreX = 0.5f * rc.Width;
	float centreY = 0.5f * rc.Height;

	Gdiplus::Pen gridPen( Gdiplus::Color( 100, 100, 100 ) );

	float bottom = rc.Height - 1;
	float top = 1;
	float left = 1;
	float right = rc.Width - 1;
	graphics.DrawLine( &gridPen, centreX, top, centreX, bottom );
	graphics.DrawLine( &gridPen, left, centreY, right, centreY );
	const int NUM_FINS = ( rc.Width < 60 ) ? 10 : 20;
	for ( unsigned int i = 0; i <= NUM_FINS; i++ )
	{
		float posX = i * ( ( float )( rc.Width ) / NUM_FINS );
		float posY = i * ( ( float )( rc.Height ) / NUM_FINS );

		graphics.DrawLine( &gridPen, centreX - 3, posY, centreX + 3, posY );
		graphics.DrawLine( &gridPen, posX, centreY - 3, posX, centreY + 3 );
	}



	int sideX = ( int )( CLAMP_MIN_FACTOR * rect.Width() * 0.5f );
	int sideY = ( int )( CLAMP_MIN_FACTOR * rect.Height() * 0.5f );
	Gdiplus::Rect invalidMinRect( centreX - sideX, centreY - sideY, sideX * 2, sideY * 2 );
	Gdiplus::SolidBrush backgroundBrush( Gdiplus::Color( 128, 128, 128 ) );
	graphics.FillEllipse( &backgroundBrush, invalidMinRect );

	DrawDefaultKnob( graphics );

	DrawDefaultBackgroundFrame( graphics );

}

