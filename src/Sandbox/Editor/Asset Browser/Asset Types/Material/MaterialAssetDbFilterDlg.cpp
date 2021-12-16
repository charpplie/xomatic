////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "MaterialAssetDbFilterDlg.h"
#include "IAssetViewer.h"

// CMaterialAssetDbFilterDlg dialog

IMPLEMENT_DYNAMIC(CMaterialAssetDbFilterDlg, CDialog)

CMaterialAssetDbFilterDlg::CMaterialAssetDbFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMaterialAssetDbFilterDlg::IDD, pParent)
{
}

CMaterialAssetDbFilterDlg::~CMaterialAssetDbFilterDlg()
{
}

void CMaterialAssetDbFilterDlg::UpdateFilterUI()
{
}

void CMaterialAssetDbFilterDlg::ApplyFilter()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();

	m_pAssetViewer->ApplyFilters(filters);
}

void CMaterialAssetDbFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMaterialAssetDbFilterDlg, CDialog)
END_MESSAGE_MAP()


// CMaterialAssetDbFilterDlg message handlers

BOOL CMaterialAssetDbFilterDlg::OnInitDialog()
{
	__super::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
