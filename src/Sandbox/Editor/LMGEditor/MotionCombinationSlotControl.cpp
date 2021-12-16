////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   MotionCombinationSlotControl.cpp
//  Version:     v1.00
//  Created:     03/09/2009 by Pau Novau
//  Description: Control that contains a single motion combination slot.
//               It contains the name of the animation and a "-" button
//               to ask the parent control to remove itself from it.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/MotionCombinationSlotControl.h"

IMPLEMENT_DYNAMIC( CMotionCombinationSlotControl, CXTResizeDialog )

BEGIN_MESSAGE_MAP( CMotionCombinationSlotControl, CXTResizeDialog )
	ON_EN_CHANGE( IDC_MOTION_COMBINATION_NAME, OnMotionCombinationNameChanged )
	ON_BN_CLICKED( ID_REMOVE_SLOT, OnRemoveButtonClicked )
	ON_WM_ENABLE()
END_MESSAGE_MAP()



CMotionCombinationSlotControl::CMotionCombinationSlotControl( CWnd* pParent )
: m_pMotionCombinationSlotListener( NULL )
{
	Create( IDD_LMG_MOTION_COMBINATION_SLOT, pParent );
}

void CMotionCombinationSlotControl::SetMotionCombinationName( const CString& motionCombinationName )
{
	m_motionCombinationName.SetWindowText( motionCombinationName );
}

CString CMotionCombinationSlotControl::GetMotionCombinationName() const
{
	CString motionCombinationName;
	m_motionCombinationName.GetWindowText( motionCombinationName );

	return motionCombinationName;
}

void CMotionCombinationSlotControl::SetListener( IMotionCombinationSlotControlListener* pMotionCombinationSlotListener )
{
	m_pMotionCombinationSlotListener = pMotionCombinationSlotListener;
}

BOOL CMotionCombinationSlotControl::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	SetResize( IDC_MOTION_COMBINATION_NAME, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	SetResize( ID_REMOVE_SLOT, SZ_TOP_RIGHT, SZ_TOP_RIGHT );

	return TRUE;
}

void CMotionCombinationSlotControl::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_MOTION_COMBINATION_NAME, m_motionCombinationName );
	DDX_Control( pDX, ID_REMOVE_SLOT, m_removeMotionCombination );
}

void CMotionCombinationSlotControl::OnMotionCombinationNameChanged()
{
	if ( m_pMotionCombinationSlotListener == NULL )
	{
		return;
	}

	m_pMotionCombinationSlotListener->OnMotionCombinationSlotNameChanged( this );
}

void CMotionCombinationSlotControl::OnRemoveButtonClicked()
{
	if ( m_pMotionCombinationSlotListener == NULL )
	{
		return;
	}

	m_pMotionCombinationSlotListener->OnRemoveButtonClicked( this );
}

void CMotionCombinationSlotControl::OnEnable( BOOL enable )
{
	m_removeMotionCombination.EnableWindow( enable );
	m_motionCombinationName.EnableWindow( enable );
}