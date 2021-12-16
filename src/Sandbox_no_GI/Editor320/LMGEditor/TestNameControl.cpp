////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestNameControl.cpp
//  Version:     v1.00
//  Created:     07/09/2009 by Pau Novau
//  Description: Control used to draw names of controls ( e.g. joysticks ) 
//               in the test view.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/TestNameControl.h"

IMPLEMENT_DYNCREATE( CTestNameControl, CWnd )

BEGIN_MESSAGE_MAP( CTestNameControl, CWnd )
	ON_WM_PAINT()
END_MESSAGE_MAP()

CTestNameControl::CTestNameControl()
{

}

CTestNameControl::CTestNameControl( CString name )
: m_name( name )
{

}

CTestNameControl::~CTestNameControl()
{

}

BOOL CTestNameControl::Create( DWORD dwStyle, const CRect& rect, CWnd* pParent, UINT nID )
{
	RegisterWindowClass();

	BOOL createSuccess = CWnd::Create( "CTestNameControl", "", dwStyle, rect, pParent, nID );
	return createSuccess;
}


void CTestNameControl::RegisterWindowClass()
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

	wndClass.lpszClassName = _T( "CTestNameControl" );

	AfxRegisterClass( &wndClass );
}

void CTestNameControl::OnPaint()
{
	CRect rect;
	GetClientRect( &rect );

	Gdiplus::Rect rc( rect.left, rect.top, rect.Width(), rect.Height() );

	CPaintDC paintDC( this );
	Gdiplus::Graphics graphics( paintDC );
	
	Gdiplus::SolidBrush backgroundBrush( Gdiplus::Color( 0, 0, 0 ) );
	graphics.FillRectangle( &backgroundBrush, rc );

	CStringW text;
	text = m_name;

	Gdiplus::RectF textArea( rect.left, rect.top, rect.Width(), rect.Height() );
	Gdiplus::Font font( L"Tahoma", 9.0f );
	Gdiplus::SolidBrush textBrush( Gdiplus::Color( 255, 255, 255 ) );
	Gdiplus::StringFormat format;
	format.SetTrimming( Gdiplus::StringTrimmingEllipsisCharacter );
	format.SetAlignment( Gdiplus::StringAlignmentNear );
	graphics.DrawString( text, text.GetLength(), &font, textArea, &format, &textBrush );
}
