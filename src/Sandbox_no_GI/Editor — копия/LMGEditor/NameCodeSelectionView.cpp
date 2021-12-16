////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   NameCodeSelectionView.cpp
//  Version:     v1.00
//  Created:     02/09/2009 by Pau Novau
//  Description: Control that displays a dropdown list and stores a string 
//               value associated to each name displayed.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/NameCodeSelectionView.h"

IMPLEMENT_DYNCREATE( CNameCodeSelectionView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CNameCodeSelectionView, CDockingBaseView )
	ON_CBN_SELCHANGE( IDC_LMG_BLENDCODE, OnSelectionChanged )
END_MESSAGE_MAP()


CNameCodeSelectionView::CNameCodeSelectionView()
{

}

CNameCodeSelectionView::~CNameCodeSelectionView()
{

}

BOOL CNameCodeSelectionView::OnInitDialog()
{
	__super::OnInitDialog();

	SetResize( IDC_LMG_BLENDCODE, SZ_TOP_LEFT, SZ_TOP_RIGHT );

	return TRUE;
}


void CNameCodeSelectionView::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_LMG_BLENDCODE, m_names );
}


void CNameCodeSelectionView::AddNameCodePair( const CString& name, const CString& code )
{
	int itemIndex = m_names.AddString( name );

	size_t codeIndex = m_codes.size();
	m_codes.push_back( code );

	m_names.SetItemData( itemIndex, ( DWORD_PTR )( codeIndex ) );
}

void CNameCodeSelectionView::Clear()
{
	m_names.ResetContent();
	m_codes.clear();
}


void CNameCodeSelectionView::SetCurrentNameByCode( const CString& codeToSelect )
{
	int codeIndexToSearch = -1;
	for ( size_t i = 0; i < m_codes.size() && codeIndexToSearch == -1; i++ )
	{
		const CString& code = m_codes[ i ];
		if ( code.CompareNoCase( codeToSelect ) == 0 )
		{
			codeIndexToSearch = ( int )( i );
		}
	}

	if ( codeIndexToSearch == -1 )
	{
		return;
	}

	for ( int i = 0; i < m_names.GetCount(); i++ )
	{
		size_t codeIndex = m_names.GetItemData( i );
		if ( codeIndex == codeIndexToSearch )
		{
			bool isSelected = ( m_names.GetCurSel() == i );
			if ( ! isSelected )
			{
				m_names.SetCurSel( i );
			}

			return;
		}
	}
}


void CNameCodeSelectionView::OnSelectionChanged()
{
	int selectedIndex = m_names.GetCurSel();
	if ( selectedIndex == CB_ERR )
	{
		return;
	}

	size_t codeIndex = m_names.GetItemData( selectedIndex );

	assert( codeIndex < m_codes.size() );
	const CString& code = m_codes[ codeIndex ];	

	OnSelectionChangedListener( code );
}

