#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////

#include "IAssetItemDatabase.h"

// CTextureAssetDbFilterDlg dialog

class CTextureAssetDbFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CTextureAssetDbFilterDlg)

public:
	CTextureAssetDbFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTextureAssetDbFilterDlg();

	void SetAssetViewer(struct IAssetViewer* pViewer)
	{
		m_pAssetViewer = pViewer;
	}

	void UpdateFilterUI();
	void ApplyFilter();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_TEXTURE_DB_FILTER };

protected:
	struct IAssetViewer* m_pAssetViewer;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedCheckMinimumWidth();
	afx_msg void OnBnClickedCheckMaximumWidth();
	afx_msg void OnBnClickedCheckMinimumHeight();
	afx_msg void OnBnClickedCheckMaximumHeight();
	afx_msg void OnBnClickedCheckDiffuseMaps();
	afx_msg void OnBnClickedCheckNormalMaps();
	afx_msg void OnCbnSelchangeComboMinimumWidth();
	afx_msg void OnCbnSelchangeComboMaximumWidth();
	afx_msg void OnCbnSelchangeComboMinimumHeight();
	afx_msg void OnCbnSelchangeComboMaximumHeight();
	afx_msg void OnCbnSelchangeComboMinimumMips();
	afx_msg void OnCbnSelchangeComboMaximumMips();
	CComboBox m_cbMinWidth;
	CComboBox m_cbMaxWidth;
	CComboBox m_cbMinHeight;
	CComboBox m_cbMaxHeight;
	CComboBox m_cbMinMips;
	CComboBox m_cbMaxMips;
	CComboBox m_cbType;
	afx_msg void OnCbnSelchangeComboTextureType();
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeComboTextureUsage();
	CComboBox m_cbUsage;
};
