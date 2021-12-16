#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "IAssetItemDatabase.h"

class CGeneralAssetDbFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(CGeneralAssetDbFilterDlg)

public:
	typedef std::map<CString/*preset name*/,SFieldFiltersPreset> TPresetNamePresetMap;

	CGeneralAssetDbFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGeneralAssetDbFilterDlg();

	void UpdateFilterUI();
	void ApplyFilter();
	void SetAssetViewer(struct IAssetViewer* pViewer)
	{
		m_pAssetViewer = pViewer;
	}

	bool LoadFilterPresets();
	bool SaveFilterPresets();
	void SaveCurrentPreset();
	void FillPresetList();
	void FillDatabases();
	bool LoadFilterPresetsTo(TPresetNamePresetMap& rOutFilters, bool bClearMap = true);
	bool SaveFilterPresetsFrom(TPresetNamePresetMap& rFilters);
	bool AddFilterPreset(const char* pPresetName);
	bool UpdateFilterPreset(const char* pPresetName);
	bool DeleteFilterPreset(const char* pPresetName);
	SFieldFiltersPreset* GetFilterPresetByName(const char* pPresetName);
	void SelectPresetByName(const char* pPresetName);
	void UpdateVisibleDatabases();
	void SelectDatabase(const char* pDbName);

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_GENERAL_DB_FILTER };

protected:
	struct IAssetViewer* m_pAssetViewer;
	TPresetNamePresetMap m_filterPresets;
	CString m_currentPresetName;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeComboPreset();
	afx_msg void OnBnClickedButtonSavePreset();
	afx_msg void OnBnClickedButtonRemovePreset();
	afx_msg void OnBnClickedCheckUsedInLevel();
	afx_msg void OnCbnSelchangeComboMinimumFilesize();
	afx_msg void OnCbnSelchangeComboMaximumFilesize();
	afx_msg void OnLvnItemchangedListDatabases(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonUpdateUsedInLevel();

protected:
	CComboBox m_cbPresets;
	CComboBox m_cbMinFilesize;
	CComboBox m_cbMaxFilesize;
	CListCtrl m_lstDatabases;
};
