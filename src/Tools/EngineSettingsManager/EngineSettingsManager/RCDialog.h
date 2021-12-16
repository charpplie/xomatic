#pragma once


#include "Resource.h"


class CRCDialog : public CDialog
{
	DECLARE_DYNAMIC(CRCDialog)

public:
	CRCDialog(CWnd* pParent = NULL);
	enum { IDD = IDD_RC_DIALOG };

	void Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager);

	bool IsShowWindow();
	bool IsHideCustom();
	CString GetParams();

protected:
	virtual void OnOK() {}
	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()

private:
	CEngineSettingsManager* m_settingsManager;

	CButton m_bShowWindow;
	CButton m_bHideCustom;
	CEdit m_eParams;
};
