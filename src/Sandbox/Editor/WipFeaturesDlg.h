#pragma once
#include "afxcmn.h"

#ifdef USE_WIP_FEATURES_MANAGER

// CWipFeaturesDlg dialog

class CWipFeaturesDlg : public CDialog
{
	DECLARE_DYNAMIC(CWipFeaturesDlg)

public:
	CWipFeaturesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWipFeaturesDlg();

// Dialog Data
	enum { IDD = IDD_WIP_FEATURES };

	void FillFeatures();
	void GatherSelectedFeatures( std::vector<int>& rFeatIds );

	std::vector<int> m_selectedFeatureIds;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl m_lstFeatures;
	afx_msg void OnBnClickedButtonShow();
	afx_msg void OnBnClickedButtonHide();
	afx_msg void OnBnClickedButtonEnable();
	afx_msg void OnBnClickedButtonDisable();
	afx_msg void OnBnClickedButtonSafemode();
	afx_msg void OnBnClickedButtonNormalmode();
	afx_msg void OnBnClickedCancel();
};

#endif // USE_WIP_FEATURES_MANAGER
