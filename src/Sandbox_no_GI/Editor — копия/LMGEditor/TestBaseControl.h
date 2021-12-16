////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestBaseControl.h
//  Version:     v1.00
//  Created:     04/09/2009 by Pau Novau
//  Description: Control that provides default drag behaviour for sliders or 
//               joysticks in the test view.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Test_Base_Control__h__
#define __Test_Base_Control__h__
#pragma once

class CTestBaseControl
	: public CWnd
{
	DECLARE_DYNCREATE( CTestBaseControl )

public:
	CTestBaseControl();
	virtual ~CTestBaseControl();

	BOOL Create( DWORD dwStyle, const CRect& rect, CWnd* pParent, UINT nID );

	const Vec2& GetKnobLogicalPosition() const;
	void SetKnobLogicalPosition( const Vec2& position );

protected:
	Vec2 ClientPositionToLogicalPosition( const CPoint& clientPosition ) const;
	CPoint LogicalPositionToClientPosition( const Vec2& logicalPosition ) const;

	virtual Vec2 FilterKnobPosition( const Vec2& unfliteredPosition );
	
	static void RegisterWndClass();

	afx_msg void OnPaint();
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	DECLARE_MESSAGE_MAP()

	virtual void Draw( Gdiplus::Graphics& graphics );
	void DrawDefaultBackground( Gdiplus::Graphics& graphics );
	void DrawDefaultBackgroundFrame( Gdiplus::Graphics& graphics );
	void DrawDefaultKnob( Gdiplus::Graphics& graphics );
	
	virtual bool CloseEnoughForSelection( const CPoint& clientPoint1, const CPoint& clientPoint2 ) const;

private:
	bool m_dragging;
	CPoint m_dragOffset;

	Vec2 m_knobLogicalPosition;
};

#endif