////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   BlendCodeSelectionView.cpp
//  Version:     v1.00
//  Created:     29/07/2009 by Pau Novau
//  Description: View that provides a list of blend codes for the user to
//               choose from.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/BlendCodeSelectionView.h"

IMPLEMENT_DYNCREATE( CBlendCodeSelectionView, CNameCodeSelectionView )

CBlendCodeSelectionView::CBlendCodeSelectionView()
: m_pBlendCodeChangedListener( NULL )
{

}

CBlendCodeSelectionView::~CBlendCodeSelectionView()
{

}

void CBlendCodeSelectionView::SetBlendCodeChangedListener( IBlendCodeChangedListener* pBlendCodeChangedListener )
{
	m_pBlendCodeChangedListener = pBlendCodeChangedListener;
}


void CBlendCodeSelectionView::OnSelectionChangedListener( const CString& code )
{
	if ( m_pBlendCodeChangedListener != NULL )
	{
		m_pBlendCodeChangedListener->OnBlendCodeChanged( code );
	}
}