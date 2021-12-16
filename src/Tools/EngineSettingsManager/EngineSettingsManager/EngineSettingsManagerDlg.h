#pragma once


#include "SettingsTab.h"
#include "EngineSettingsManager.h"
#include <XTToolkitPro.h>

class CEngineSettingsManagerDlg : public CDialog
{
public:
	CEngineSettingsManagerDlg(CWnd* pParent = NULL);
	~CEngineSettingsManagerDlg();

	enum { IDD = IDD_ENGINESETTINGSMANAGER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	afx_msg void OnSelchangingTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnOkButtonClicked();
	afx_msg void OnSkinChanged();
	afx_msg void LoadCrySkin();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

private:
	CEngineSettingsManager* m_settingsManager;

	HICON m_hIcon;
	CSettingsTab m_tcSettings;
};
