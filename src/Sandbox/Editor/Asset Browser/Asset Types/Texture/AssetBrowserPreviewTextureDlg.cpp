////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewTextureDlg.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Implementation of AssetBrowserPreviewTextureDlg.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Asset Browser/AssetBrowserPreviewDlg.h"
#include "AssetBrowserPreviewTextureDlg.h"
#include "AssetTextureItem.h"
#include "CustomColorDialog.h"

const int kExpandedTexturePreviewPanelHeight = 111;
const int kCollapsedTexturePreviewPanelHeight = 25;

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewTextureDlg, CDialog)

CAssetBrowserPreviewTextureDlg::CAssetBrowserPreviewTextureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewTextureDlg::IDD, pParent)
{
	m_pTexture = NULL;
	m_customBackColor = RGB(255, 0, 255);
}

CAssetBrowserPreviewTextureDlg::~CAssetBrowserPreviewTextureDlg()
{
}

void CAssetBrowserPreviewTextureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TEXTURE_MIP_LEVEL, m_cbMips);
	DDX_Control(pDX, IDC_COMBO_TEXTURE_PREVIEW_BACKCOLOR, m_cbBackColor);
}


BEGIN_MESSAGE_MAP(CAssetBrowserPreviewTextureDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_ALPHA, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowAlpha)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_RGB, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgb)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_RGBA, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgba)
	ON_BN_CLICKED(IDC_BUTTON_ZOOM_1TO1, &CAssetBrowserPreviewTextureDlg::OnBnClickedButtonZoom1to1)
	ON_BN_CLICKED(IDC_CHECK_SMOOTH_TEXTURE, &CAssetBrowserPreviewTextureDlg::OnBnClickedCheckSmoothTexture)
	ON_CBN_SELCHANGE(IDC_COMBO_TEXTURE_MIP_LEVEL, &CAssetBrowserPreviewTextureDlg::OnCbnSelchangeComboTextureMipLevel)
	ON_CBN_SELENDOK(IDC_COMBO_TEXTURE_PREVIEW_BACKCOLOR, &OnCbnSelendokComboTexturePreviewBackcolor)
	ON_BN_CLICKED(IDC_CHECK_MORE_TEXTURE_INFO, &CAssetBrowserPreviewTextureDlg::OnBnClickedCheckMoreTextureInfo)
END_MESSAGE_MAP()

// CAssetBrowserPreviewTextureDlg message handlers

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowAlpha()
{
	GetDlgItem(IDC_CHECK_SMOOTH_TEXTURE)->EnableWindow(TRUE);
	m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_Alpha;
	GetParent()->Invalidate();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgb()
{
	GetDlgItem(IDC_CHECK_SMOOTH_TEXTURE)->EnableWindow(TRUE);
	m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGB;
	GetParent()->Invalidate();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonShowRgba()
{
	GetDlgItem(IDC_CHECK_SMOOTH_TEXTURE)->EnableWindow(FALSE);
	m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGBA;
	GetParent()->Invalidate();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedButtonZoom1to1()
{
	CRect rc;

	GetParent()->GetDlgItem(IDC_STATIC_ASSET_BROWSER_PREVIEW_VIEWPORT)->GetClientRect(&rc);
	m_pTexture->PreviewCenterAndScaleToFit(rc);
	GetParent()->Invalidate();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedCheckSmoothTexture()
{
	m_pTexture->m_bPreviewSmooth = IsDlgButtonChecked(IDC_CHECK_SMOOTH_TEXTURE) == BST_CHECKED;
	GetParent()->Invalidate();
}

void CAssetBrowserPreviewTextureDlg::OnCbnSelchangeComboTextureMipLevel()
{
}

void CAssetBrowserPreviewTextureDlg::Init()
{
	if (m_pTexture)
	{
		m_cbMips.ResetContent();

		for (size_t i = 0; i < m_pTexture->m_nMips; ++i)
		{
			CString str;

			str.Format("Mip%d", i);
			m_cbMips.AddString(str);
		}

		if (IsDlgButtonChecked(IDC_CHECK_MORE_TEXTURE_INFO))
		{
			ComputeHistogram();
		}

		if (IsDlgButtonChecked(IDC_BUTTON_SHOW_ALPHA))
		{
			m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_Alpha;
		}
		else if (IsDlgButtonChecked(IDC_BUTTON_SHOW_RGB))
		{
			m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGB;
		}
		else if (IsDlgButtonChecked(IDC_BUTTON_SHOW_RGBA))
		{
			m_pTexture->m_previewDrawingMode = CAssetTextureItem::eAssetTextureDrawing_RGBA;
		}
	}

	GetDlgItem(IDC_CHECK_SMOOTH_TEXTURE)->EnableWindow(TRUE);
	CheckDlgButton(IDC_CHECK_SMOOTH_TEXTURE, BST_CHECKED);
}

void CAssetBrowserPreviewTextureDlg::ComputeHistogram()
{
	CImageEx img;
	CString str = m_pTexture->GetRelativePath();

	str += m_pTexture->GetFilename();

	if (!CImageUtil::LoadImage(str.GetBuffer(), img))
	{
		return;
	}

	unsigned int* pData = img.GetData();

	if (!pData)
	{
		img.Release();
		return;
	}

	if (0 == img.GetWidth() || 0 == img.GetHeight())
	{
		img.Release();
		return;
	}

	if (pData)
	{
		CImageHistogramCtrl::EImageFormat fmt;

		fmt = CImageHistogramCtrl::eImageFormat_32BPP_BGRA;
		m_histogram.ComputeHistogram((BYTE*)pData, img.GetWidth(), img.GetHeight(), fmt);
	}

	img.Release();
}

BOOL CAssetBrowserPreviewTextureDlg::OnInitDialog()
{
	CRect rc;

	CDialog::OnInitDialog();
	CheckDlgButton(IDC_BUTTON_SHOW_RGB, BST_CHECKED);
	CheckDlgButton(IDC_BUTTON_SHOW_RGBA, BST_UNCHECKED);
	CheckDlgButton(IDC_BUTTON_SHOW_ALPHA, BST_UNCHECKED);
	Init();
	m_histogram.Create(NULL, "Histogram", WS_CHILD | WS_VISIBLE, CRect(10, 30, 266, 110), this, 1);
	GetClientRect(&rc);
	SetWindowPos(0, 0, 0, rc.Width(), kCollapsedTexturePreviewPanelHeight, SWP_NOMOVE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAssetBrowserPreviewTextureDlg::OnCbnSelendokComboTexturePreviewBackcolor()
{
	switch (m_cbBackColor.GetCurSel())
	{
	case 0:
	{
		m_pTexture->m_previewBackColor = RGB(255, 255, 255);
		break;
	}

	case 1:
	{
		m_pTexture->m_previewBackColor = RGB(0, 0, 0);
		break;
	}

	case 2:
	{
		m_pTexture->m_previewBackColor = RGB(128, 128, 128);
		break;
	}

	case 3:
	{
		CCustomColorDialog dlg(m_customBackColor);

		if (dlg.DoModal() == IDOK)
		{
			m_pTexture->m_previewBackColor = m_customBackColor = dlg.GetColor();
		}

		break;
	}
	}

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewTextureDlg::OnBnClickedCheckMoreTextureInfo()
{
	CRect rc;

	GetClientRect(&rc);

	if (IsDlgButtonChecked(IDC_CHECK_MORE_TEXTURE_INFO))
	{
		ComputeHistogram();
		SetWindowPos(0, 0, 0, rc.Width(), kExpandedTexturePreviewPanelHeight, SWP_NOMOVE);
	}
	else
	{
		SetWindowPos(0, 0, 0, rc.Width(), kCollapsedTexturePreviewPanelHeight, SWP_NOMOVE);
	}

	((CAssetBrowserPreviewDlg*)GetParent())->ResizeControls();
	OnBnClickedButtonZoom1to1();
}