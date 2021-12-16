////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   AnimationsListView.cpp
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: View that lists animations that can be used to create an
//               Lmg animation.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/AnimationsListView.h"

#include "ICryAnimation.h"

const int IDC_REPORT_CONTROL = 1;

IMPLEMENT_DYNCREATE( CAnimationsListView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CAnimationsListView, CDockingBaseView )
	ON_EN_CHANGE( IDC_ANIMATION_FILTER, OnAnimationNameFilterChange )
	ON_BN_CLICKED( IDC_RESET_FILTER_TEXT, ResetFilterText )
END_MESSAGE_MAP()

CAnimationsListView::CAnimationsListView()
{

}

CAnimationsListView::~CAnimationsListView()
{

}

void CAnimationsListView::AddDragDropListener( IAnimationNameDragDropListener* pDragDropListener )
{
	m_animationsReport.AddDragDropListener( pDragDropListener );
}

void CAnimationsListView::SetSelectionChangeListener( IAnimationNameSelectionChangedListener* pSelectionChangedListener )
{
	m_animationsReport.SetSelectionChangedListener( pSelectionChangedListener );
}

BOOL CAnimationsListView::OnInitDialog()
{
	__super::OnInitDialog();

	CRect rc;
	GetClientRect( rc );
	rc.top += 25;

	m_animationsReport.Create( WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, this, IDC_REPORT_CONTROL );

	SetResize( IDC_ANIMATION_FILTER, SZ_TOP_LEFT, SZ_TOP_RIGHT );
	SetResize( IDC_REPORT_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	SetResize( IDC_RESET_FILTER_TEXT, SZ_TOP_RIGHT, SZ_TOP_RIGHT );

	return TRUE;
}

void CAnimationsListView::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_ANIMATION_FILTER, m_animationNameFilter );
	DDX_Control( pDX, IDC_RESET_FILTER_TEXT, m_resetFilterTextButton );
}

void CAnimationsListView::UpdateAnimations( IAnimationSet* pAnimationSet )
{
	m_animationsReport.UpdateAnimations( pAnimationSet );
	ResetFilterText();
}

void CAnimationsListView::Clear()
{
	m_animationsReport.Clear();
	ResetFilterText();
}

void CAnimationsListView::OnAnimationNameFilterChange()
{
	CString filterText;
	m_animationNameFilter.GetWindowText( filterText );
	m_animationsReport.FilterByName( filterText );
}

void CAnimationsListView::OnResetFilterTextButtonClicked()
{
	ResetFilterText();
}

void CAnimationsListView::ResetFilterText()
{
	m_animationNameFilter.SetWindowText( "" );
}
