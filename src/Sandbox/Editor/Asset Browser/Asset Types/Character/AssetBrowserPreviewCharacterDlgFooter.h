#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

class CAssetBrowserPreviewCharacterDlgFooter : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewCharacterDlgFooter)

public:
	CAssetBrowserPreviewCharacterDlgFooter(CWnd* pParent = NULL);
	virtual ~CAssetBrowserPreviewCharacterDlgFooter();

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