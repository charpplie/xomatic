#pragma once


#include "Resource.h"


class CEditorDialog : public CDialog
{
	DECLARE_DYNAMIC(CEditorDialog)

public:
	CEditorDialog(CWnd* pParent = NULL);
	enum { IDD = IDD_EDITOR_DIALOG };

	void Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager);

	bool IsPrefer32Bit();
	CString GetLevelExportKeyFromUi();

protected:
	virtual void OnOK() {}
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnExportKey0Changed();
	afx_msg void OnExportKey1Changed();
	afx_msg void OnExportKey2Changed();
	afx_msg void OnExportKey3Changed();
	afx_msg void OnResetButtonClicked();

	DECLARE_MESSAGE_MAP()

	void OnExportKeyChanged( int keyId );
	void SetLevelExportControlsVisible();

private:
	CEngineSettingsManager* m_pSettingsManager;

	CButton m_bPrefer32Bit;
	CEdit m_exportKey[ 4 ];
	CButton m_exportKeyReset;
	CStatic m_exportKeyStatic;
};
