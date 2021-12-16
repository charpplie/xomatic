////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   SlotControl.cpp
//  Version:     v1.00
//  Created:     28/07/2009 by Pau Novau
//  Description: Control that stores a named animation slot with description
//               and status information.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/SlotControl.h"

#include "LMGEditor/UndoSlotChange.h"

IMPLEMENT_DYNAMIC( CSlotControl, CXTResizeDialog )

BEGIN_MESSAGE_MAP( CSlotControl, CXTResizeDialog )
	ON_EN_CHANGE( IDC_SLOT_ANIMATION, OnAnimationNameChange )
END_MESSAGE_MAP()

HBITMAP CSlotControl::ms_statusImages[ 3 ] = { 0, 0, 0 };


CSlotControl::CSlotControl( CWnd* pParent )
: CXTResizeDialog( IDD_ANIMATION_SLOT, pParent )
, m_animationNameChangeListener( NULL )
{
	bool statusImagesInitialized = ( ms_statusImages[ 0 ] != 0 );
	if ( ! statusImagesInitialized )
	{
		int slotStatusToBitmapResourceId[ 3 ] =
		{
			IDB_ANIMATION_SLOT_OK,
			IDB_ANIMATION_SLOT_ERROR,
			IDB_ANIMATION_SLOT_ERROR
		};

		for ( int i = 0; i < 3; i++ )
		{
			int bitmapResourceId = slotStatusToBitmapResourceId[ i ];
			HBITMAP statusBitmap = ::LoadBitmap( AfxGetResourceHandle(), MAKEINTRESOURCE( bitmapResourceId ) );
			ms_statusImages[ i ] = statusBitmap;
		}
	}

	Create( IDD_ANIMATION_SLOT, pParent );
}


BOOL CSlotControl::OnInitDialog()
{
	__super::OnInitDialog();

	SetResize( IDC_SLOT_ANIMATION, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	SetResize( IDC_SLOT_STATUSIMAGE, SZ_TOP_RIGHT, SZ_TOP_RIGHT );

	if ( m_tooltip.Create( this ) == FALSE )
	{
		return FALSE;
	}
	m_tooltip.Activate( TRUE );

	return TRUE;
}

void CSlotControl::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_SLOT_NAME, m_slotName );
	DDX_Control( pDX, IDC_SLOT_ANIMATION, m_animationName );
	DDX_Control( pDX, IDC_SLOT_STATUSIMAGE, m_statusImage );
}

void CSlotControl::SetName( const CString& slotName )
{
	m_slotName.SetWindowText( slotName );
}

CString CSlotControl::GetName() const
{
	CString name;
	m_slotName.GetWindowText( name );

	return name;
}

void CSlotControl::SetAnimationName( const CString& animationName )
{
	m_animationName.SetWindowText( animationName );
}

CString CSlotControl::GetAnimationName() const
{
	CString animationName;
	m_animationName.GetWindowText( animationName );

	return animationName;
}

const CString& CSlotControl::GetOldAnimationName() const
{
	return m_oldAnimationName;
}

void CSlotControl::SetDescription( const CString& description )
{
	m_tooltip.AddTool( this, description );
}

BOOL CSlotControl::PreTranslateMessage( MSG* pMsg )
{
	if ( m_tooltip.GetSafeHwnd() != 0 )
	{
		m_tooltip.RelayEvent( pMsg );
	}

	bool letParentHandleMessage = false;
	if ( WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST )
	{
		int key = pMsg->wParam;
		if ( key != VK_TAB )
		{
			letParentHandleMessage = true;
		}
	}

	if ( pMsg->message == WM_MOUSEWHEEL )
	{
		letParentHandleMessage = true;
	}

	if ( letParentHandleMessage )
	{
		if ( GetParent() != NULL )
		{
			if ( GetParent()->PreTranslateMessage( pMsg ) )
			{
				return TRUE;
			}
		}
	}

	return __super::PreTranslateMessage( pMsg );
}

void CSlotControl::SetAnimationNameChangeListener( ISlotControlAnimationNameListener* animationNameListener )
{
	m_animationNameChangeListener = animationNameListener;
}

void CSlotControl::OnAnimationNameChange()
{
	RecordAnimationNameUndo();
	m_animationName.EmptyUndoBuffer();

	if ( m_animationNameChangeListener == NULL )
	{
		return;
	}

	m_animationNameChangeListener->OnSlotAnimationNameChanged( this );
}

void CSlotControl::SetStatus( SlotStatus status, const CString& message )
{
	HBITMAP statusBitmap = ms_statusImages[ status ];
	m_statusImage.SetBitmap( statusBitmap );

	m_tooltip.AddTool( &m_statusImage, message );
}

void CSlotControl::RecordAnimationNameUndo()
{
	bool animationNameHasChanged = ( GetOldAnimationName().Compare( GetAnimationName() ) != 0 );
	if ( ! animationNameHasChanged )
	{
		return;
	}

	CUndo undo( "Slot change" );
	CUndo::Record( new CUndoSlotChange( this, GetOldAnimationName() ) );
	m_oldAnimationName = GetAnimationName();
}