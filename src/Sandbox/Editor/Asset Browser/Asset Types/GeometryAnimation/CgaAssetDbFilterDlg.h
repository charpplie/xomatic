#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "IAssetItemDatabase.h"

// CCgaAssetDbFilterDlg dialog

class CCgaAssetDbFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CCgaAssetDbFilterDlg)

public:
	CCgaAssetDbFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCgaAssetDbFilterDlg();

	void UpdateFilterUI();
	void ApplyFilter();
	void SetAssetViewer(struct IAssetViewer* pViewer)
	{
		m_pAssetViewer = pViewer;
	}

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_MODEL_DB_FILTER };

protected:
	struct IAssetViewer* m_pAssetViewer;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_cbMinTris;
	CComboBox m_cbMaxTris;
	CComboBox m_cbMinMtls;
	CComboBox m_cbMaxMtls;
	afx_msg void OnCbnSelchangeComboMinTriangles();
	afx_msg void OnCbnSelchangeComboMaxTriangles();
	afx_msg void OnCbnSelchangeComboMinMaterials();
	afx_msg void OnCbnSelchangeComboMaxMaterials();
	afx_msg void OnBnClickedCheckHideLods();
	virtual BOOL OnInitDialog();
};
