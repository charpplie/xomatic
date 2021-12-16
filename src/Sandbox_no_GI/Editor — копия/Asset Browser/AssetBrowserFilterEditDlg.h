#pragma once
#include "afxwin.h"
#include "Include/IAssetDisplayDatabase.h"
// CAssetBrowserFilterEditDlg dialog

class CAssetBrowserFilterEditDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserFilterEditDlg)

	typedef std::vector<std::pair<string,SAssetField::EAssetFilterCondition>>	TSearchFilterConditions;
	typedef std::set<SAssetField::EAssetFilterCondition>											TStringOnlyConditionsSet;

public:
	CAssetBrowserFilterEditDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserFilterEditDlg();

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_FILTER_EDIT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

	class CAssetBrowserSearchDlg*		m_pSearchDlg;
	struct SAssetField*							m_pFilterField;
	static TSearchFilterConditions	m_scConditions;
	static TStringOnlyConditionsSet m_ssStringConditions;
	CButton m_chkFieldname;
	CComboBox m_cbCondition;
	CComboBox m_cbValue;
	CComboBox m_cbMaxValue;
	afx_msg void OnBnClickedCheckRange();
	afx_msg void OnBnClickedCheckFieldLabel();

public:
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnCbnSelchangeComboCondition();
	afx_msg void OnCbnEditchangeComboFilterValue();
	afx_msg void OnCbnEditchangeComboFilterMaxvalue();
	afx_msg void OnCbnSelchangeComboFilterValue();
	afx_msg void OnCbnSelchangeComboFilterMaxvalue();
	virtual BOOL OnInitDialog();
};
