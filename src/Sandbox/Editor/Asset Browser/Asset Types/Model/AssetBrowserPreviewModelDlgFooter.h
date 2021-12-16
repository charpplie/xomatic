#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewModelDlg.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Defines the preview model dialog panel, used for previewing
//								the current selected model asset
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

class CAssetBrowserPreviewModelDlgFooter : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewModelDlgFooter)

public:
	CAssetBrowserPreviewModelDlgFooter(CWnd* pParent = NULL);
	virtual ~CAssetBrowserPreviewModelDlgFooter();

	void Init();
	void Reset();

	enum { IDD = IDD_ASSET_BROWSER_PREVIEW_MODEL_FOOTER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnLodLevelChanged();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	DECLARE_MESSAGE_MAP()

protected:
	CComboBox m_CBLodLevel;
	CSliderCtrl m_sliderAmbient;
	int m_minLodDefault;
};