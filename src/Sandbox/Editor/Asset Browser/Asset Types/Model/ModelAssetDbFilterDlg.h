#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "IAssetItemDatabase.h"

// CModelAssetDbFilterDlg dialog

class CModelAssetDbFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CModelAssetDbFilterDlg)

public:
	CModelAssetDbFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModelAssetDbFilterDlg();

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
