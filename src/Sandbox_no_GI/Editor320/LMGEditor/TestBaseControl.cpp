////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestBaseControl.cpp
//  Version:     v1.00
//  Created:     04/09/2009 by Pau Novau
//  Description: Control that provides default drag behaviour for sliders or 
//               joysticks in the test view.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/TestBaseControl.h"

IMPLEMENT_DYNCREATE( CTestBaseControl, CWnd )

BEGIN_MESSAGE_MAP( CTestBaseControl, CWnd )
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CTestBaseControl::CTestBaseControl()
: m_dragging( false )
, m_dragOffset( 0, 0 )
, m_knobLogicalPosition( 0, 0 )
{

}


CTestBaseControl::~CTestBaseControl()
{

}



void CTestBaseControl::RegisterWndClass()
{
	WNDCLASS wndClass;

	memset( &wndClass, 0, sizeof( WNDCLASS ) );

	wndClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;

	wndClass.lpfnWndProc = ::DefWindowProc; 
	wndClass.hInstance = AfxGetInstanceHandle();
	wndClass.hIcon = NULL;
	wndClass.hCursor = AfxGetApp()->LoadStandardCursor( IDC_ARROW );
	wndClass.hbrBackground = NULL;
	wndClass.lpszMenuName = NULL;

	wndClass.lpszClassName = _T( "CTestBaseControl" );

	AfxRegisterClass( &wndClass );
}



BOOL CTestBaseControl::Create( DWORD dwStyle, const CRect& rect, CWnd* pParent, UINT nID )
{
	RegisterWndClass();

	BOOL createSuccess = CWnd::Create( _T( "CTestBaseControl" ), "", dwStyle, rect, pParent, nID );
	return createSuccess;
}



const Vec2& CTestBaseControl::GetKnobLogicalPosition() const
{
	return m_knobLogicalPosition;
}


void CTestBaseControl::SetKnobLogicalPosition( const Vec2& position )
{
	m_knobLogicalPosition = FilterKnobPosition( position );

	if ( GetSafeHwnd() != 0 )
	{
		Invalidate();
	}
}


Vec2 CTestBaseControl::FilterKnobPosition( const Vec2& unfliteredPosition )
{
	Vec2 filteredPosition;
	filteredPosition.x = clamp_tpl< float >( unfliteredPosition.x, 0, 1 );
	filteredPosition.y = clamp_tpl< float >( unfliteredPosition.y, 0, 1 );
	
	return filteredPosition;
}


Vec2 CTestBaseControl::ClientPositionToLogicalPosition( const CPoint& clientPosition ) const
{
	CRect rcClient;
	GetClientRect( rcClient );

	int width = std::max( rcClient.Width(), 1 );
	int height = std::max( rcClient.Height(), 1 );

	Vec2 logicalPosition;
	logicalPosition.x = ( float )( clientPosition.x ) / width;
	logicalPosition.y = ( float )( height - clientPosition.y ) / height;

	return logicalPosition;
}


CPoint CTestBaseControl::LogicalPositionToClientPosition( const Vec2& logicalPosition ) const
{
	CRect rcClient;
	GetClientRect( rcClient );

	int width = rcClient.Width();
	int height = rcClient.Height();

	CPoint clientPosition;
	clientPosition.x = ( int )( logicalPosition.x * width );
	clientPosition.y = ( int )( ( 1 - logicalPosition.y ) * height );

	return clientPosition;
}


void CTestBaseControl::OnPaint()
{
	CRect rect;
	GetClientRect( &rect );

	if ( rect.Width() == 0 || rect.Height() == 0 )
	{
		return;
	}

	Gdiplus::Rect rc( rect.left, rect.top, rect.Width(), rect.Height() );

	Gdiplus::Bitmap backBufferBitmap( rect.Width(), rect.Height() );

	Gdiplus::Graphics* pBackBufferGraphics = Gdiplus::Graphics::FromImage( &backBufferBitmap );

	{
		Gdiplus::Graphics& graphics = *pBackBufferGraphics;

		Draw( graphics );
	}

	CPaintDC paintDC( this );
	Gdiplus::Graphics mainGraphics( paintDC );
	mainGraphics.DrawImage( &backBufferBitmap, rc );

	delete pBackBufferGraphics;
}


void CTestBaseControl::OnMouseMove( UINT nFlags, CPoint point )
{
	if ( ! m_dragging )
	{
		return;
	}

	Vec2 logicalPosition = ClientPositionToLogicalPosition( point + m_dragOffset );
	SetKnobLogicalPosition( logicalPosition );
}


void CTestBaseControl::OnLButtonDown( UINT nFlags, CPoint point )
{
	CPoint knobClientPosition = LogicalPositionToClientPosition( GetKnobLogicalPosition() );

	m_dragging = CloseEnoughForSelection( point, knobClientPosition );
	if ( ! m_dragging )
	{
		return;
	}

	m_dragOffset = knobClientPosition - point;

	SetCapture();
}

bool CTestBaseControl::CloseEnoughForSelection( const CPoint& clientPoint1, const CPoint& clientPoint2 ) const
{
	int DEFAULT_THRESHOLD = 10;

	CRect point1Rect( clientPoint1.x - DEFAULT_THRESHOLD, clientPoint1.y - DEFAULT_THRESHOLD, clientPoint1.x + DEFAULT_THRESHOLD, clientPoint1.y + DEFAULT_THRESHOLD );

	bool isPointNear = point1Rect.PtInRect( clientPoint2 );

	return isPointNear;
}

void CTestBaseControl::OnLButtonUp( UINT nFlags, CPoint point )
{
	m_dragging = false;
	ReleaseCapture();
}

void CTestBaseControl::Draw( Gdiplus::Graphics& graphics )
{
	DrawDefaultBackground( graphics );
	DrawDefaultKnob( graphics );
}

void CTestBaseControl::DrawDefaultBackground( Gdiplus::Graphics& graphics )
{
	CRect rect;
	GetClientRect( &rect );

	Gdiplus::Rect rc( rect.left, rect.top, rect.Width(), rect.Height() );

	Gdiplus::SolidBrush backgroundBrush( Gdiplus::Color( 128, 128, 128 ) );
	graphics.FillRectangle( &backgroundBrush, rc );
}

void CTestBaseControl::DrawDefaultBackgroundFrame( Gdiplus::Graphics& graphics )
{
	CRect rect;
	GetClientRect( &rect );

	Gdiplus::Rect rc( rect.left, rect.top, rect.Width(), rect.Height() );

	Gdiplus::Pen framePen( Gdiplus::Color( 32, 32, 32 ) );
	Gdiplus::Rect rcFrame( rect.left, rect.top, rect.Width() - 1, rect.Height() - 1);
	graphics.DrawRectangle( &framePen, rcFrame );
}

void CTestBaseControl::DrawDefaultKnob( Gdiplus::Graphics& graphics )
{
	CPoint p = LogicalPositionToClientPosition( GetKnobLogicalPosition() );
	Gdiplus::Rect knob( p.x - 5, p.y - 5, 10, 10 );

	Gdiplus::SolidBrush knobBackGroundBrush( Gdiplus::Color( 172, 172, 172 ) );
	graphics.FillEllipse( &knobBackGroundBrush, knob );

	Gdiplus::Pen knobFramePen( Gdiplus::Color( 0, 0, 0 ) );
	graphics.DrawEllipse( &knobFramePen, knob );
}