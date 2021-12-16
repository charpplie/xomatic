////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   SlotsView.cpp
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: View used to hold a list of animation slots.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/SlotsView.h"

IMPLEMENT_DYNCREATE( CSlotsView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CSlotsView, CDockingBaseView )
	ON_WM_SIZE()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


CSlotsView::CSlotsView()
: m_totalSlotsHeight( 0 )
, m_pListener( NULL )
, m_activeSlots( 0 )
, m_batchCreatingSlots( false )
{
	m_scrollHelper.SetAllowed( CScrollHelper::HORIZONTAL, false );
}

CSlotsView::~CSlotsView()
{

}

bool CSlotsView::IsValidDropPoint( const CPoint& screenPoint, const CString& animationName ) const
{	
	const CSlotControl* pSlot = GetSlotAtPoint( screenPoint );	
	bool validDropPoint = ( pSlot != NULL );

	return validDropPoint;
}

CSlotControl* CSlotsView::GetSlotAtPoint( const CPoint& screenPoint )
{
	CWnd* pWindowUnderPoint = WindowFromPoint( screenPoint );
	CWnd* pParentWindow = pWindowUnderPoint->GetParent();

	for ( size_t i = 0; i < m_activeSlots; i++ )
	{
		CSlotControl* pSlot = m_slots[ i ];
		if ( pSlot == pParentWindow )
		{
			return pSlot;
		}
	}

	return NULL;
}

const CSlotControl* CSlotsView::GetSlotAtPoint( const CPoint& screenPoint ) const
{
	CWnd* pWindowUnderPoint = WindowFromPoint( screenPoint );
	CWnd* pParentWindow = pWindowUnderPoint->GetParent();

	for ( size_t i = 0; i < m_activeSlots; i++ )
	{
		const CSlotControl* pSlot = m_slots[ i ];
		if ( pSlot == pParentWindow )
		{
			return pSlot;
		}
	}

	return NULL;
}

void CSlotsView::DoDrop( const CPoint& screenPoint, const CString& animationName )
{
	CSlotControl* pSlot = GetSlotAtPoint( screenPoint );
	assert( pSlot != NULL );
	if ( pSlot == NULL )
	{
		return;
	}

	pSlot->SetAnimationName( animationName );
}

void CSlotsView::SetSlotStatus( size_t slotId, CSlotControl::SlotStatus status, const CString& message )
{
	if ( m_activeSlots <= slotId )
	{
		return;
	}

	CSlotControl* pSlot = m_slots[ slotId ];
	pSlot->SetStatus( status, message );
}

CString CSlotsView::GetAnimationName( size_t slotId ) const
{
	if ( m_activeSlots <= slotId )
	{
		return "";
	}

	CSlotControl* pSlot = m_slots[ slotId ];
	return pSlot->GetAnimationName();
}

void CSlotsView::SetAnimationName( size_t slotId, const CString& animationName )
{
	if ( m_activeSlots <= slotId )
	{
		return;
	}

	CSlotControl* pSlot = m_slots[ slotId ];
	pSlot->SetAnimationName( animationName );
}

size_t CSlotsView::GetSlotCount() const
{
	return m_activeSlots;
}

BOOL CSlotsView::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	m_scrollHelper.SetWnd( this );

	return TRUE;
}

void CSlotsView::OnDestroy()
{
	DestroySlots();
}

void CSlotsView::RemoveAllSlots()
{
	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CSlotControl* pSlot = m_slots[ i ];
		pSlot->ShowWindow( SW_HIDE );
	}

	m_activeSlots = 0;

	RecalculateSlotsLayout();

	m_scrollHelper.SetDesiredClientSize( CRect( 0, 0, 100, 100 ) );
}

void CSlotsView::DestroySlots()
{
	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		CSlotControl* pSlot = m_slots[ i ];
		pSlot->SetParent( NULL );
		pSlot->DestroyWindow();
		delete pSlot;
	}

	m_slots.clear();
	m_activeSlots = 0;
}

void CSlotsView::BatchSlotCreationStart()
{
	assert( ! m_batchCreatingSlots );
	m_batchCreatingSlots = true;
}

void CSlotsView::BatchSlotCreationEnd()
{
	assert( m_batchCreatingSlots );
	m_batchCreatingSlots = false;
	RecalculateSlotsLayout();
}

void CSlotsView::CreateSlot( const CString& slotName, const CString& description, const CString& animationName )
{
	bool needToCreateSlot = ( m_slots.size() <= m_activeSlots );
	CSlotControl* pSlot = NULL;
	if ( needToCreateSlot )
	{
		pSlot = new CSlotControl( this );
		m_slots.push_back( pSlot );
	}
	else
	{
		pSlot = m_slots[ m_activeSlots ];
		pSlot->ShowWindow( SW_SHOW );
	}

	m_activeSlots++;

	pSlot->SetName( slotName );
	pSlot->SetAnimationName( animationName );
	pSlot->SetDescription( description );
	pSlot->SetAnimationNameChangeListener( this );

	RecalculateSlotsLayout();

	m_scrollHelper.ResetScroll();
	m_scrollHelper.SetDesiredClientSize( CRect( 0, 0, 100, m_totalSlotsHeight ) );
}

void CSlotsView::OnSize( UINT nType, int cx, int cy )
{
	RecalculateSlotsLayout();

	CXTResizeDialog::OnSize( nType, cx, cy );

	m_scrollHelper.OnSize( nType, cx, cy );
}

void CSlotsView::RecalculateSlotsLayout()
{
	CRect rc;
	GetClientRect( &rc );

	int slotWidth = rc.Width();

	const int SLOT_MARGIN = 5;
	const int SLOT_HEIGHT = 20;
	const int SLOT_SEPARATION = SLOT_HEIGHT + SLOT_MARGIN;

	int offset = SLOT_MARGIN - m_scrollHelper.GetScrollAmount( CScrollHelper::VERTICAL );

	for ( size_t i = 0; i < m_activeSlots; i++ )
	{
		CSlotControl* pSlot = m_slots[ i ];

		int slotPosition = offset + ( i * SLOT_SEPARATION );
		if ( ! m_batchCreatingSlots )
		{
			pSlot->MoveWindow( 0, slotPosition, slotWidth, SLOT_HEIGHT );
		}
	}

	m_totalSlotsHeight = SLOT_SEPARATION * m_activeSlots + SLOT_MARGIN;
}

void CSlotsView::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{	
	CXTResizeDialog::OnVScroll( nSBCode, nPos, pScrollBar );

	m_scrollHelper.OnVScroll( nSBCode, nPos, pScrollBar );
}

void CSlotsView::SetListener( ISlotsViewListener* pListener )
{
	m_pListener = pListener;
}

void CSlotsView::OnSlotAnimationNameChanged( CSlotControl* pSlotControl )
{
	if ( m_pListener == NULL )
	{
		return;
	}

	m_pListener->OnSlotAnimationNameChanged();
}