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

#include "stdafx.h"
#include "..\LMGEditor/TestSliderControl.h"

IMPLEMENT_DYNCREATE( CTestSliderControl, CTestBaseControl )

CTestSliderControl::CTestSliderControl()
: m_isVertical( true )
, m_isHorizontal( true )
, m_min( -1, -1 )
, m_max( 1, 1 )
{
	SetValue( Vec2( 0, 0 ) );
}


CTestSliderControl::~CTestSliderControl()
{

}



Vec2 CTestSliderControl::GetValue() const
{
	Vec2 logicalValue = GetKnobLogicalPosition();
	Vec2 range = m_max - m_min;
	Vec2 value = m_min + ( Vec2( range.x * logicalValue.x, range.y * logicalValue.y ) );
	return value;
}


void CTestSliderControl::SetValue( const Vec2& value )
{
	Vec2 range = m_max - m_min;
	range.x = ( range.x <= 0 ) ? 1 : range.x;
	range.y = ( range.y <= 0 ) ? 1 : range.y;
	
	Vec2 v = ( value - m_min );
	Vec2 logicalPosition = Vec2( v.x / range.x, v.y / range.y );

	SetKnobLogicalPosition( logicalPosition );
}


Vec2 CTestSliderControl::GetValueRelativeToRange() const
{
	return GetKnobLogicalPosition();
}


void CTestSliderControl::SetVertical( bool isVertical )
{
	m_isVertical = isVertical;
}


bool CTestSliderControl::IsVertical() const
{
	return m_isVertical;
}



void CTestSliderControl::SetHorizontal( bool isHorizontal )
{
	m_isHorizontal = isHorizontal;
}


bool CTestSliderControl::IsHorizontal() const
{
	return m_isHorizontal;
}



void CTestSliderControl::SetRange( const Vec2& minRange, const Vec2& maxRange )
{
	SetHorizontal( true );
	SetVertical( true );

	m_min = minRange;
	m_max = maxRange;

	Vec2 range = m_max - m_min;
	
	if ( range.x <= 0 )
	{
		SetHorizontal( false );
	}

	if ( range.y <= 0 )
	{
		SetVertical( false );
	}
}




Vec2 CTestSliderControl::FilterKnobPosition( const Vec2& unfliteredPosition )
{
	Vec2 filteredPosition = CTestBaseControl::FilterKnobPosition( unfliteredPosition );
	
	if ( ! IsHorizontal() )
	{
		filteredPosition.x = 0.5f;
	}

	if ( ! IsVertical() )
	{
		filteredPosition.y = 0.5f;
	}

	return filteredPosition;
}


bool CTestSliderControl::CloseEnoughForSelection( const CPoint& clientPoint1, const CPoint& clientPoint2 ) const
{
	return true;
}


void CTestSliderControl::Draw( Gdiplus::Graphics& graphics )
{
	DrawDefaultBackground( graphics );
	
	Vec2 value = GetValue();

	CRect rect;
	GetClientRect( &rect );
	Gdiplus::Rect rc( rect.left, rect.top, rect.Width() - 1, rect.Height() - 1 );

	bool verticalSlider = ( IsVertical() && ! IsHorizontal() );
	bool horizontalSlider = ( IsHorizontal() && ! IsVertical() );
	if ( verticalSlider )
	{
		DrawDefaultKnob( graphics );
	}
	else if ( horizontalSlider )
	{
		float zeroLogicalPos = 0;
		if ( m_min.x <= 0 && m_max.x <= 0 )
		{
			zeroLogicalPos = 1;
		}
		else if ( 0 <= m_min.x && 0 <= m_max.x )
		{
			zeroLogicalPos = 0;
		}
		else
		{
			float negativeRange = -m_min.x;
			float positiveRange = m_max.x;
			float totalRange = negativeRange + positiveRange;
			zeroLogicalPos = negativeRange / totalRange;
		}
		
		Gdiplus::SolidBrush sliderBrush( Gdiplus::Color( 172, 172, 172 ) );

		CPoint clientKnobPosition = LogicalPositionToClientPosition( GetKnobLogicalPosition() );
		CPoint zeroClientPosition = LogicalPositionToClientPosition( Vec2( zeroLogicalPos, 0 ) );

		int sliderLeft = std::min< int >( clientKnobPosition.x, zeroClientPosition.x );
		int sliderWidth = std::abs( clientKnobPosition.x - zeroClientPosition.x );

		sliderWidth = std::max< int >( sliderWidth, 1 );

		graphics.FillRectangle( &sliderBrush, sliderLeft, 0, sliderWidth, rect.Height() - 1 );

		Gdiplus::SolidBrush sliderBrush2( Gdiplus::Color( 232, 232, 232 ) );
		int hightlightX = clamp_tpl< int >( clientKnobPosition.x - 2, 0, rect.Width() - 4 );
		graphics.FillRectangle( &sliderBrush2, hightlightX, 0, 4, rect.Height() - 1 );
	}
	else
	{
		DrawDefaultKnob( graphics );
	}

	DrawDefaultBackgroundFrame( graphics );

}

