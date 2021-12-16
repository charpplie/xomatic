////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TestView.cpp
//  Version:     v1.00
//  Created:     28/07/2009 by Pau Novau
//  Description: View used to hold Ui for playing with motion params by the
//               user.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/TestView.h"

#include "LMGEditor/TestJoystickControl.h"
#include "LMGEditor/TestSliderControl.h"
#include "LMGEditor/TestNameControl.h"

IMPLEMENT_DYNCREATE( CTestView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CTestView, CDockingBaseView )
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
END_MESSAGE_MAP()

enum
{
	ID_LMG_CONTROLS = 6600
};

CTestView::CTestView()
{

}

CTestView::~CTestView()
{
	for ( size_t i = 0; i < m_controls.size(); i++ )
	{
		CWnd* pControl = m_controls[ i ];
		pControl->DestroyWindow();
		delete pControl;
	}

	m_controls.clear();
}

BOOL CTestView::OnInitDialog()
{
	__super::OnInitDialog();

	CRect rc;
	GetClientRect( &rc );

	m_scrollHelper.SetWnd( this );

	return TRUE;
}

void CTestView::ClearTestView()
{
	for ( size_t i = 0; i < m_controls.size(); i++ )
	{
		CWnd* pControl = m_controls[ i ];
		pControl->DestroyWindow();
		delete pControl;
	}

	m_controls.clear();

	RecalculateCurrentLayoutSize();
}

CTestJoystickControl* CTestView::CreateJoystickControl()
{
	CTestJoystickControl* pJoystickControl = new CTestJoystickControl();
	m_controls.push_back( pJoystickControl );

	pJoystickControl->Create( WS_CHILD | WS_VISIBLE, CRect( 0, 0, 128, 128 ), this, ID_LMG_CONTROLS + m_controls.size() - 1 );

	return pJoystickControl;
}

CTestSliderControl* CTestView::CreateSliderControl()
{
	CTestSliderControl* pSliderControl = new CTestSliderControl();

	m_controls.push_back( pSliderControl );

	pSliderControl->Create( WS_CHILD | WS_VISIBLE, CRect( 0, 0, 128, 20 ), this, ID_LMG_CONTROLS + m_controls.size() - 1 );

	return pSliderControl;
}

void CTestView::CreateNameControl( int left, int top, int width, const CString& name )
{
	CTestNameControl* pNameControl = new CTestNameControl( name );
	m_controls.push_back( pNameControl );

	CRect rc( left, top - 20, left + width, top );

	pNameControl->Create( WS_CHILD | WS_VISIBLE, rc, this, ID_LMG_CONTROLS + m_controls.size() - 1 );
}

void CTestView::RecalculateCurrentLayoutSize()
{
	m_scrollHelper.ResetScroll();

	CPoint p( 0, 0 );
	ClientToScreen( &p );
	p.x = -p.x;
	p.y = -p.y;

	CRect rc( 0, 0, 64, 64 );
	for ( size_t i = 0; i < m_controls.size(); i++ )
	{
		CWnd* pControl = m_controls[ i ];
		CRect controlRect;
		pControl->GetWindowRect( &controlRect );
		controlRect.OffsetRect( p );

		rc.UnionRect( rc, controlRect );
	}

	m_scrollHelper.SetDesiredClientSize( rc );
}

void CTestView::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	__super::OnHScroll( nSBCode, nPos, pScrollBar );

	m_scrollHelper.OnHScroll( nSBCode, nPos, pScrollBar );
}

void CTestView::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	__super::OnVScroll( nSBCode, nPos, pScrollBar );

	m_scrollHelper.OnVScroll( nSBCode, nPos, pScrollBar );
}

void CTestView::OnSize( UINT nType, int cx, int cy )
{
	__super::OnSize( nType, cx, cy );

	m_scrollHelper.OnSize( nType, cx, cy );
}