////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewModelDlg.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//	Description: Implementation of AssetBrowserPreviewModelDlg.h
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "AssetBrowserPreviewModelDlg.h"
#include "AssetModelItem.h"

// CAssetBrowserPreviewModelDlg dialog

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewModelDlg, CDialog)

CAssetBrowserPreviewModelDlg::CAssetBrowserPreviewModelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewModelDlg::IDD, pParent)
{
	m_pModel = NULL;
}

CAssetBrowserPreviewModelDlg::~CAssetBrowserPreviewModelDlg()
{
}

void CAssetBrowserPreviewModelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAssetBrowserPreviewModelDlg, CDialog)
	ON_BN_CLICKED( IDC_CHECK_WIREFRAME, OnBnClickedButtonWireframe )
	ON_BN_CLICKED( IDC_BUTTON_RESET_VIEW, OnBnClickedButtonResetView )
END_MESSAGE_MAP()


// CAssetBrowserPreviewModelDlg message handlers
void CAssetBrowserPreviewModelDlg::Init()
{
	if( m_pModel )
	{
		CheckDlgButton( IDC_CHECK_WIREFRAME, m_pModel->m_bWireframe );
	}
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonWireframe()
{
	if( m_pModel )
	{
		m_pModel->m_bWireframe = IsDlgButtonChecked( IDC_CHECK_WIREFRAME ) == BST_CHECKED;
	}

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonResetView()
{
	if( m_pModel )
	{
		m_pModel->m_camZoom = 1.0f;
		m_pModel->m_translateX = 0.0f;
		m_pModel->m_translateY = 0.0f;
		m_pModel->m_rotationX = AssetBrowser::kDefaultModelRotationAngleX;
		m_pModel->m_rotationY = AssetBrowser::kDefaultModelRotationAngleY;
	}

	GetParent()->RedrawWindow();
}