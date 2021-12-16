#pragma once
#include "afxwin.h"
#include <vector>
#include "Include/IAssetDisplay.h"
#include "Include/IAssetDisplayDatabase.h"
#include "Controls/ScrollableWindow.h"

class CAssetBrowserDialog;
class CAssetBrowserFilterEditDlg;

#define ID_TIMER_SEARCH_BY_FILENAME 1

const UINT kAssetBrowser_TimerSearchByFilenameDelay = 500;

// CAssetBrowserSearchDlg dialog

class CAssetBrowserSearchDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserSearchDlg)

public:

	CAssetBrowserDialog* m_pAssetBrowserDlg;

	CAssetBrowserSearchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserSearchDlg();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_SEARCH_BAR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheckShowFilters();
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedCheckEnableFilters();

	void CreateFilterDialogs();
	void DestroyFilterDialogs();
	void FillDatabases();

	std::vector<CAssetBrowserFilterEditDlg*> m_filterDlgs;
	CEdit m_edSearchText;
	CFont m_fntSearchText;
	IAssetDisplayDatabase::TAssetFields m_fields;
	CScrollableWindow m_fieldEditorsContainer;
	CSize m_clientSize;

	virtual BOOL OnInitDialog();
protected:
	virtual void OnCancel();
	virtual void OnOK();
public:
	CComboBox m_cbFilenameFilterCondition;
	CComboBox m_cbDatabase;
	CComboBox m_cbThumbSize;

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEnChangeEditSearchText();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCbnSelchangeComboFilenameCondition();
	afx_msg void OnCbnSelchangeComboDatabase();
	afx_msg void OnCbnSelchangeComboThumbSize();
};
