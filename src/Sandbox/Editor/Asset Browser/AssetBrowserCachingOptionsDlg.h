#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "Asset Browser/AssetBrowserManager.h"

// CAssetBrowserCachingOptionsDlg dialog

class CAssetBrowserCachingOptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserCachingOptionsDlg)

public:
	CAssetBrowserCachingOptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserCachingOptionsDlg();

	TAssetDatabases GetSelectedDatabases();
	bool IsForceCache();
	UINT GetThumbSize();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_CACHING_OPTIONS };

protected:
	bool m_bForceCache;
	UINT m_thumbSize;
	std::vector<struct IAssetItemDatabase*> m_databases, m_selectedDBs;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CListBox m_lstDatabases;
	afx_msg void OnBnClickedOk();
	CComboBox m_cbThumbSize;
};
