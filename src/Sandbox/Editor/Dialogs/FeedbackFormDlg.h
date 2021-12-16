#pragma once

class CFeedbackFormDlg : public CDialog
{
	DECLARE_DYNAMIC(CFeedbackFormDlg)

public:
	CFeedbackFormDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFeedbackFormDlg();

	void SetFeatureName(const char* pFeatureName);

// Dialog Data
	enum { IDD = IDD_FEEDBACK };

protected:
	CString m_featureName;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
