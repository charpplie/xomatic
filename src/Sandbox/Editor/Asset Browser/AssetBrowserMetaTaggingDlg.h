#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "AssetBrowserCommon.h"

class CAutoCompleteEdit : public CEdit
{
	DECLARE_DYNAMIC(CAutoCompleteEdit)

public:
	CAutoCompleteEdit();
	~CAutoCompleteEdit();

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt,	UINT nFlags);
};

// CAssetBrowserMetaTagging dialog

class CAssetBrowserMetaTaggingDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserMetaTaggingDlg)

public:
	CAssetBrowserMetaTaggingDlg(TAssetItems* assetlist, CWnd* pParent = NULL);    // standard constructor
	CAssetBrowserMetaTaggingDlg(const CString& filename, CWnd* pParent = NULL);
	virtual ~CAssetBrowserMetaTaggingDlg();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_METATAGGING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	afx_msg void OnOK();
	afx_msg void OnEditChanged();

	bool Validated();

	DECLARE_MESSAGE_MAP()

	CAutoCompleteEdit m_descriptionEdit;
	CEdit m_filenameEdit;

private:
	std::vector<CStatic*> m_staticControls;
	std::vector<CComboBox*> m_comboControls;
	std::vector<CString> m_assetList;
	bool m_bAttemptAutocomplete;
};
