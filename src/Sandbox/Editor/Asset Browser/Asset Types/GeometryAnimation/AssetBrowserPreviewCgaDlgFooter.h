#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

class CAssetBrowserPreviewCgaDlgFooter : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewCgaDlgFooter)

public:
	CAssetBrowserPreviewCgaDlgFooter(CWnd* pParent = NULL);
	virtual ~CAssetBrowserPreviewCgaDlgFooter();

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