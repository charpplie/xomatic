#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewTextureDlg.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	The asset texture dialog panel, used for previewing textures
//								in the asset preview dialog
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "afxwin.h"
#include "Controls/ImageHistogramCtrl.h"

class CAssetBrowserPreviewTextureDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewTextureDlg)

public:
	class	CAssetTextureItem* m_pTexture;

	void Init();
	void ComputeHistogram();

	CAssetBrowserPreviewTextureDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserPreviewTextureDlg();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_PREVIEW_TEXTURE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnBnClickedButtonShowAlpha();
	afx_msg void OnBnClickedButtonShowRgb();
	afx_msg void OnBnClickedButtonShowRgba();
	afx_msg void OnBnClickedButtonZoom1to1();

	DECLARE_MESSAGE_MAP()

public:
	CComboBox m_cbBackColor;
	COLORREF m_customBackColor;
	CComboBox m_cbMips;
	CImageHistogramCtrl m_histogram;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCheckSmoothTexture();
	afx_msg void OnCbnSelchangeComboTextureMipLevel();
	afx_msg void OnCbnSelendokComboTexturePreviewBackcolor();
	afx_msg void OnBnClickedCheckMoreTextureInfo();
};