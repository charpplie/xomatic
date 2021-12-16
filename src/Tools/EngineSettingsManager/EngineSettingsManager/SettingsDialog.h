#pragma once


#include "Resource.h"
#include "EngineSettingsManager.h"


class CSettingsDialog : public CDialog
{
	DECLARE_DYNAMIC(CSettingsDialog)

public:
	CSettingsDialog(CWnd* pParent = NULL);
	enum { IDD = IDD_SETTINGSDIALOG };

	void Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager);

	CString GetRootPath() const;
	bool IsRootPathGood() const;

protected:
	virtual void OnOK() {}
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	afx_msg void OnBrowseButtonClicked();
	DECLARE_MESSAGE_MAP()

private:
	void UpdateEngineStateView();

private:
	CEngineSettingsManager* m_settingsManager;
	CEdit m_eRootPath;
	CStatic m_imgRCState;
	CStatic m_lRCState;
	bool m_bRCFound;
};
