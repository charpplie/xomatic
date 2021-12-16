#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "IAssetItemDatabase.h"

class CAssetBrowserFiltersDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserFiltersDlg)

public:
	CAssetBrowserFiltersDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserFiltersDlg();

	void CreateFilterGroupsRollup();
	void DestroyFilterGroupsRollup();
	void RefreshVisibleAssetDbsRollups();
	void UpdateAllFiltersUI();
	class CGeneralAssetDbFilterDlg* GetGeneralDlg()
	{
		return m_pGeneralDlg;
	}

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_FILTERS };

protected:
	CRollupCtrl m_rollupCtrl;
	std::map<IAssetItemDatabase*, CDialog*> m_filterDlgs;
	class CGeneralAssetDbFilterDlg* m_pGeneralDlg;
	std::vector<int> m_panelIds;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
};