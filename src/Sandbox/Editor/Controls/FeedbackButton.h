#pragma once

class CFeedbackButton : public CStatic
{
	DECLARE_DYNAMIC(CFeedbackButton)

public:
	CFeedbackButton();
	virtual ~CFeedbackButton();

	void Init(const char* pFeatureName);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClicked();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	CString m_featureName;
	CToolTipCtrl m_toolTip;

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};