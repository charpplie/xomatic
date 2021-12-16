////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_RefBrowser.cpp
//  Version:     v1.00
//  Created:     17/01/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTree_RefBrowser.h"
#include "CustomFileDialog.h"

IMPLEMENT_DYNAMIC(CSelectionTree_RefBrowser, CDialog)

BEGIN_MESSAGE_MAP(CSelectionTree_RefBrowser, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_BST_REFTREE, OnSelChanged)
	ON_BN_CLICKED(IDC_BST_SHOWCONTENTS, OnShowContents)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

CSelectionTree_RefBrowser::CSelectionTree_RefBrowser(CWnd* pParent /*=NULL*/)
	:	CDialog( CSelectionTree_RefBrowser::IDD, pParent )
	, m_refType( eSTTI_Undefined )
{
}

CSelectionTree_RefBrowser::~CSelectionTree_RefBrowser()
{
}

void CSelectionTree_RefBrowser::Init( ESelectionTreeTypeId type )
{
	m_refType = type;
}

const char* CSelectionTree_RefBrowser::GetSelectedRef()
{
	return m_SelectedRef.length() > 0 ? m_SelectedRef.c_str() : NULL;
}

void CSelectionTree_RefBrowser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BST_REFTREE, m_RefTree);
}


BOOL CSelectionTree_RefBrowser::OnInitDialog()
{
	CDialog::OnInitDialog();
	CButton* rb = ( CButton* )GetDlgItem( IDC_BST_SHOWCONTENTS );
	rb->ShowWindow(false);
	LoadReferences();
	return TRUE;
}

void CSelectionTree_RefBrowser::OnSelChanged( NMHDR *pNMHDR, LRESULT *pResult )
{
	HTREEITEM item = m_RefTree.GetSelectedItem();
	if( item )
	{
		m_SelectedRef = m_RefNames[ item ];
	}
	else
	{
		m_SelectedRef = "";
	}
}

void CSelectionTree_RefBrowser::OnShowContents()
{
	CButton* rb = ( CButton* )GetDlgItem( IDC_BST_SHOWCONTENTS );
	LoadReferences();
}

void CSelectionTree_RefBrowser::LoadReferences()
{
	assert( m_refType != eSTTI_Undefined );
	m_RefTree.DeleteAllItems();

	TSelectionTreeInfoList infoList;
	GetIEditor()->GetSelectionTreeManager()->GetInfoList( infoList );

	for ( TSelectionTreeInfoList::const_iterator it = infoList.begin(); it != infoList.end(); ++it )
	{
		const SSelectionTreeInfo& info = *it;

		if ( !info.IsTree )
		{
			const int blockCount = info.GetBlockCountById( m_refType );
			for ( int i = 0; i < blockCount; ++i )
			{
				SSelectionTreeBlockInfo blockInfo;
				bool ok = info.GetBlockById( blockInfo, m_refType, i);
				assert(ok);
				if ( ok )
				{
					HTREEITEM item = m_RefTree.InsertItem( blockInfo.Name , 0, 0, TVI_ROOT, TVI_SORT );
					m_RefNames[ item ] = blockInfo.Name;
				}
			}
		}
	}
}
