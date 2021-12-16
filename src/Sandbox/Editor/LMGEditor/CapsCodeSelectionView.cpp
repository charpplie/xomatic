////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CapsCodeSelectionView.cpp
//  Version:     v1.00
//  Created:     02/09/2009 by Pau Novau
//  Description: View that provides a list of caps codes for the user to
//               choose from.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/CapsCodeSelectionView.h"

IMPLEMENT_DYNCREATE( CCapsCodeSelectionView, CNameCodeSelectionView )

CCapsCodeSelectionView::CCapsCodeSelectionView()
: m_pCapsCodeChangedListener( NULL )
{

}

CCapsCodeSelectionView::~CCapsCodeSelectionView()
{

}

void CCapsCodeSelectionView::SetCapsCodeChangedListener( ICapsCodeChangedListener* pCapsCodeChangedListener )
{
	m_pCapsCodeChangedListener = pCapsCodeChangedListener;
}


void CCapsCodeSelectionView::OnSelectionChangedListener( const CString& code )
{
	if ( m_pCapsCodeChangedListener != NULL )
	{
		m_pCapsCodeChangedListener->OnCapsCodeChanged( code );
	}
}