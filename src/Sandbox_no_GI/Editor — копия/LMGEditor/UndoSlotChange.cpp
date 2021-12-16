////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   UndoSlotChange.cpp
//  Version:     v1.00
//  Created:     17/09/2009 by Pau Novau
//  Description: Undo object for text changes in animation name slots.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/UndoSlotChange.h"

#include "LMGEditor/SlotControl.h"

CUndoSlotChange::CUndoSlotChange( CSlotControl* pSlotControl, const CString& animationNameToSave )
: m_pSlotControl( pSlotControl )
{
	if ( ! IsValidSlotControl() )
	{
		return;
	}

	m_storedAnimationName = animationNameToSave;
}

void CUndoSlotChange::Undo( bool storeDataForRedo )
{
	CString currentAnimationName = GetSlotAnimationName();

	SetSlotAnimationName( m_storedAnimationName );

	m_storedAnimationName = currentAnimationName;
}

void CUndoSlotChange::Redo()
{
	CString currentAnimationName = GetSlotAnimationName();

	SetSlotAnimationName( m_storedAnimationName );

	m_storedAnimationName = currentAnimationName;
}

bool CUndoSlotChange::IsValidSlotControl() const
{
	if ( m_pSlotControl == NULL )
	{
		return false;
	}

	bool isValidWindow = IsWindow( m_pSlotControl->GetSafeHwnd() );
	return isValidWindow;
}

CString CUndoSlotChange::GetSlotAnimationName() const
{
	if ( ! IsValidSlotControl() )
	{
		return CString();
	}

	return m_pSlotControl->GetAnimationName();
}

void CUndoSlotChange::SetSlotAnimationName( const CString& animationName )
{
	if ( ! IsValidSlotControl() )
	{
		return;
	}

	m_pSlotControl->SetAnimationName( animationName );
}