////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestSliderControl.cpp
//  Version:     v1.00
//  Created:     07/09/2009 by Pau Novau
//  Description: Control that implements a slider used in the test view.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Test_Slider_Control__h__
#define __Test_Slider_Control__h__
#pragma once

#include "TestBaseControl.h"

class CTestSliderControl
	: public CTestBaseControl
{
	DECLARE_DYNCREATE( CTestSliderControl )

public:
	CTestSliderControl();
	virtual ~CTestSliderControl();

	Vec2 GetValue() const;
	void SetValue( const Vec2& value );

	Vec2 GetValueRelativeToRange() const;

	void SetVertical( bool isVertical );
	bool IsVertical() const;

	void SetHorizontal( bool isHorizontal );
	bool IsHorizontal() const;

	void SetRange( const Vec2& minRange, const Vec2& maxRange );


protected:
	virtual Vec2 FilterKnobPosition( const Vec2& unfliteredPosition );

	virtual void Draw( Gdiplus::Graphics& graphics );

	virtual bool CloseEnoughForSelection( const CPoint& clientPoint1, const CPoint& clientPoint2 ) const;

private:
	bool m_isVertical;
	bool m_isHorizontal;

	Vec2 m_min;
	Vec2 m_max;
};

#endif