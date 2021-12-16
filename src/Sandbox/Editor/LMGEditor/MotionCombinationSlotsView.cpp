////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   MotionCombinationSlotsView.cpp
//  Version:     v1.00
//  Created:     03/09/2009 by Pau Novau
//  Description: View that shows a user expandable list of view combination 
//               animation slots.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/MotionCombinationSlotsView.h"

IMPLEMENT_DYNCREATE( CMotionCombinationSlotsView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CMotionCombinationSlotsView, CDockingBaseView )
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_BN_CLICKED( ID_ADD_MOTION_COMBINATION_SLOT, OnAddMotionCombination )
	ON_BN_CLICKED( IDC_CHECK_ENABLE_MOTION_COMBINATION_EDITING, OnEnableEditStatusChanged )
END_MESSAGE_MAP()



CMotionCombinationSlotsView::CMotionCombinationSlotsView()
: m_totalSlotsHeight( 0 )
, m_pListener( NULL )
{
	m_scrollHelper.SetAllowed( CScrollHelper::HORIZONTAL, false );
}

CMotionCombinationSlotsView::~CMotionCombinationSlotsView()
{

}


BOOL CMotionCombinationSlotsView::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	m_scrollHelper.SetWnd( this );

	RecalculateSlotsLayout();
	m_scrollHelper.SetDesiredClientSize( CRect( 0, 0, 100, m_totalSlotsHeight ) );

	OnEnableEditStatusChanged();

	return TRUE;
}

void CMotionCombinationSlotsView::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, ID_ADD_MOTION_COMBINATION_SLOT, m_addMotionCombinationButton );
	DDX_Control( pDX, IDC_CHECK_ENABLE_MOTION_COMBINATION_EDITING, m_enableCheckBox );
}

void CMotionCombinationSlotsView::OnDestroy()
{
	DestroySlots();
}


void CMotionCombinationSlotsView::RemoveAllSlots()
{
	DestroySlots();

	RecalculateSlotsLayout();

	m_scrollHelper.SetDesiredClientSize( CRect( 0, 0, 100, 100 ) );
}


void CMotionCombinationSlotsView::DestroySlots()
{
	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CMotionCombinationSlotControl* pSlot = m_slots[ i ];
		pSlot->SetParent( NULL );
		pSlot->DestroyWindow();
		delete pSlot;
	}

	m_slots.clear();
}

bool CMotionCombinationSlotsView::IsValidDropPoint( const CPoint& screenPoint, const CString& animationName ) const
{
	if ( ! IsEditingEnabled() )
	{
		return false;
	}

	const CMotionCombinationSlotControl* pSlot = GetSlotAtPoint( screenPoint );	
	bool validDropPoint = ( pSlot != NULL );

	return validDropPoint;
}

CMotionCombinationSlotControl* CMotionCombinationSlotsView::GetSlotAtPoint( const CPoint& screenPoint )
{
	CWnd* pWindowUnderPoint = WindowFromPoint( screenPoint );
	CWnd* pParentWindow = pWindowUnderPoint->GetParent();

	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CMotionCombinationSlotControl* pSlot = m_slots[ i ];
		if ( pSlot == pParentWindow )
		{
			return pSlot;
		}
	}

	return NULL;
}

const CMotionCombinationSlotControl* CMotionCombinationSlotsView::GetSlotAtPoint( const CPoint& screenPoint ) const
{
	CWnd* pWindowUnderPoint = WindowFromPoint( screenPoint );
	CWnd* pParentWindow = pWindowUnderPoint->GetParent();

	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CMotionCombinationSlotControl* pSlot = m_slots[ i ];
		if ( pSlot == pParentWindow )
		{
			return pSlot;
		}
	}

	return NULL;
}

void CMotionCombinationSlotsView::DoDrop( const CPoint& screenPoint, const CString& animationName )
{
	if ( ! IsEditingEnabled() )
	{
		return;
	}

	CMotionCombinationSlotControl* pSlot = GetSlotAtPoint( screenPoint );
	assert( pSlot != NULL );
	if ( pSlot == NULL )
	{
		return;
	}

	pSlot->SetMotionCombinationName( animationName );
}



void CMotionCombinationSlotsView::OnMotionCombinationSlotNameChanged( CMotionCombinationSlotControl* pMotionCombinationSlotControl )
{
	if ( m_pListener != NULL )
	{
		m_pListener->OnMotionCombinationSlotNameChanged();
	}
}

void CMotionCombinationSlotsView::OnRemoveButtonClicked( CMotionCombinationSlotControl* pMotionCombinationSlotControl )
{
	std::vector< CMotionCombinationSlotControl* >::iterator it = std::find( m_slots.begin(), m_slots.end(), pMotionCombinationSlotControl );
	bool slotFound = ( it != m_slots.end() );
	if ( ! slotFound )
	{
		return;
	}

	pMotionCombinationSlotControl->SetParent( NULL );
	pMotionCombinationSlotControl->DestroyWindow();
	delete pMotionCombinationSlotControl;

	m_slots.erase( it );

	RecalculateSlotsLayout();
	
	m_scrollHelper.SetDesiredClientSize( CRect( 0, 0, 100, m_totalSlotsHeight ) );

	if ( m_pListener != NULL )
	{
		m_pListener->OnMotionCombinationSlotRemoved();
	}
}

void CMotionCombinationSlotsView::CreateMotionCombinationSlot( const CString& motionCombinationName )
{
	CMotionCombinationSlotControl* pSlot = new CMotionCombinationSlotControl( this );
	pSlot->SetMotionCombinationName( motionCombinationName );
	pSlot->SetListener( this );
	pSlot->EnableWindow( IsEditingEnabled() );

	m_slots.push_back( pSlot );

	RecalculateSlotsLayout();

	m_scrollHelper.SetDesiredClientSize( CRect( 0, 0, 100, m_totalSlotsHeight ) );
}

void CMotionCombinationSlotsView::OnAddMotionCombination()
{
	CreateMotionCombinationSlot();
}

void CMotionCombinationSlotsView::OnEnableEditStatusChanged()
{
	bool editingEnabled = IsEditingEnabled();

	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CMotionCombinationSlotControl* pSlot = m_slots[ i ];
		pSlot->EnableWindow( editingEnabled );
	}

	m_addMotionCombinationButton.EnableWindow( editingEnabled );
}

void CMotionCombinationSlotsView::SetMotionCombinationSlotName( size_t slotId, const CString& motionCombinationName )
{
	if ( m_slots.size() <= slotId )
	{
		return;
	}

	CMotionCombinationSlotControl* pSlot = m_slots[ slotId ];
	pSlot->SetMotionCombinationName( motionCombinationName );
}

CString CMotionCombinationSlotsView::GetMotionCombinationName( size_t slotId ) const
{
	if ( m_slots.size() <= slotId )
	{
		return "";
	}

	CMotionCombinationSlotControl* pSlot = m_slots[ slotId ];
	return pSlot->GetMotionCombinationName();
}

size_t CMotionCombinationSlotsView::GetSlotCount() const
{
	return m_slots.size();
}


void CMotionCombinationSlotsView::OnSize( UINT nType, int cx, int cy )
{
	RecalculateSlotsLayout();

	CXTResizeDialog::OnSize( nType, cx, cy );

	m_scrollHelper.OnSize( nType, cx, cy );
}

void CMotionCombinationSlotsView::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	CXTResizeDialog::OnVScroll( nSBCode, nPos, pScrollBar );

	m_scrollHelper.OnVScroll( nSBCode, nPos, pScrollBar );
}


void CMotionCombinationSlotsView::RecalculateSlotsLayout()
{
	CRect rc;
	GetClientRect( &rc );

	const int SLOT_MARGIN = 5;
	int offset = SLOT_MARGIN - m_scrollHelper.GetScrollAmount( CScrollHelper::VERTICAL );

	int slotWidth = rc.Width();

	m_totalSlotsHeight = 0;

	if ( m_enableCheckBox.GetSafeHwnd() != 0 )
	{
		m_enableCheckBox.MoveWindow( 0, offset, slotWidth, 20 );
		offset += 20;
		m_totalSlotsHeight += 25;
	}
	
	const int SLOT_HEIGHT = 30;
	const int SLOT_SEPARATION = SLOT_HEIGHT + SLOT_MARGIN;

	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CMotionCombinationSlotControl* pSlot = m_slots[ i ];

		int slotPosition = offset + ( i * SLOT_SEPARATION );
		pSlot->MoveWindow( 0, slotPosition, slotWidth, SLOT_HEIGHT );
	}

	m_totalSlotsHeight += SLOT_SEPARATION * m_slots.size() + SLOT_MARGIN;

	if ( m_addMotionCombinationButton.GetSafeHwnd() != 0 )
	{
		m_addMotionCombinationButton.MoveWindow( slotWidth - 20 - SLOT_MARGIN, offset + ( SLOT_SEPARATION * m_slots.size() + SLOT_MARGIN ), 20, 20 );
		m_totalSlotsHeight += 20 + SLOT_MARGIN;
	}
}

bool CMotionCombinationSlotsView::IsEditingEnabled() const
{
	if ( m_enableCheckBox.GetSafeHwnd() == 0 )
	{
		return false;
	}

	bool editingEnabled = ( m_enableCheckBox.GetCheck() == BST_CHECKED );
	return editingEnabled;
}

void CMotionCombinationSlotsView::SetListener( IMotionCombinationSlotsViewListener* pListener )
{
	m_pListener = pListener;
}